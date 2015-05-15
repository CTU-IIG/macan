#include <stdbool.h>
#include <stdlib.h>

#include "can_frame.h"
#include "macan_private.h"
#include "stm3210c_eval_lcd.h"
#include "stm32_eval.h"

#define CAN_BAUDRATE  125  /* 125kBps */

/* global time */
static uint64_t time = 0;

/* ================ PRIVATE FUNCTIONS ===================== */

/**
  * @brief  Configures CAN1 and CAN2.
  * @param  None
  * @retval None
  */
static void can_config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	CAN_InitTypeDef        CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;

	/* Configure CAN1 and CAN2 IOs **********************************************/
	/* GPIOB, GPIOD and AFIO clocks enable */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOB, ENABLE);
	 
	/* Configure CAN1 RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Configure CAN2 RX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure CAN1 TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Configure CAN2 TX pin */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Remap CAN1 and CAN2 GPIOs */
	GPIO_PinRemapConfig(GPIO_Remap2_CAN1 , ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_CAN2, ENABLE);

	/* Configure CAN1 and CAN2 **************************************************/  
	/* CAN1 and CAN2 Periph clocks enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);  

	/* CAN1 and CAN2 register init */
	CAN_DeInit(CAN1);
	CAN_DeInit(CAN2);

	/* Struct init*/
	CAN_StructInit(&CAN_InitStructure);

	/* CAN1 and CAN2  cell init */
	CAN_InitStructure.CAN_TTCM = DISABLE;
	CAN_InitStructure.CAN_ABOM = DISABLE;
	CAN_InitStructure.CAN_AWUM = DISABLE;
	CAN_InitStructure.CAN_NART = DISABLE;
	CAN_InitStructure.CAN_RFLM = DISABLE;
	CAN_InitStructure.CAN_TXFP = ENABLE;
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
	CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;  
	CAN_InitStructure.CAN_BS1 = CAN_BS1_3tq;
	CAN_InitStructure.CAN_BS2 = CAN_BS2_2tq;
 
#if CAN_BAUDRATE == 1000 /* 1MBps */
	CAN_InitStructure.CAN_Prescaler =6;
#elif CAN_BAUDRATE == 500 /* 500KBps */
	CAN_InitStructure.CAN_Prescaler =12;
#elif CAN_BAUDRATE == 250 /* 250KBps */
	CAN_InitStructure.CAN_Prescaler =24;
#elif CAN_BAUDRATE == 125 /* 125KBps */
	CAN_InitStructure.CAN_Prescaler =48;
#elif  CAN_BAUDRATE == 100 /* 100KBps */
	CAN_InitStructure.CAN_Prescaler =60;
#elif  CAN_BAUDRATE == 50 /* 50KBps */
	CAN_InitStructure.CAN_Prescaler =120;
#elif  CAN_BAUDRATE == 20 /* 20KBps */
	CAN_InitStructure.CAN_Prescaler =300;
#elif  CAN_BAUDRATE == 10 /* 10KBps */
	CAN_InitStructure.CAN_Prescaler =600;
#else
	#error "Please select first the CAN Baudrate in Private defines in main.c "
#endif  /* CAN_BAUDRATE == 1000 */

  
	/*Initializes the CAN1  and CAN2 */
	CAN_Init(CAN1, &CAN_InitStructure);
	CAN_Init(CAN2, &CAN_InitStructure);

	/* CAN1 filter init */
	CAN_FilterInitStructure.CAN_FilterNumber = 1;
	CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
	CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x6420;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
	CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
	CAN_FilterInit(&CAN_FilterInitStructure);

	/* CAN2 filter init */
	CAN_FilterInitStructure.CAN_FilterIdHigh =0x2460;
	CAN_FilterInitStructure.CAN_FilterNumber = 15;
	CAN_FilterInit(&CAN_FilterInitStructure);
}

/*
 * Configure timer to interrupt every 1/100 second.
 */
static void timer_config() {

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	TIM_TimeBaseInitTypeDef timerInitStructure; 
	timerInitStructure.TIM_Prescaler = 300;
	timerInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	timerInitStructure.TIM_Period = 2400;
	timerInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	timerInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &timerInitStructure);
	TIM_Cmd(TIM2, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    NVIC_InitTypeDef nvicStructure;
    nvicStructure.NVIC_IRQChannel = TIM2_IRQn;
    nvicStructure.NVIC_IRQChannelPreemptionPriority = 0;
    nvicStructure.NVIC_IRQChannelSubPriority = 0;
    nvicStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvicStructure);
}
/*
 * ISR routine for timer, increments time every second
 */
void TIM2_IRQHandler()
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

		/* increment time */ 
		time++;

		/* blink blue led to check it's working */
		STM_EVAL_LEDToggle(LED4);
    }
}

/* ================ PUBLIC FUNCTIONS ===================== */

/*
 * will blink led and never return
 */
void test_blink() {

	int i;
	while(1) {
		for(i = 0; i < 999999; i++);
		STM_EVAL_LEDToggle(LED3);
	}

}
/*
 * Initialize peripherals
 */
int helper_init() {

	/* init onboard LEDs */
	STM_EVAL_LEDInit(LED1);
	STM_EVAL_LEDInit(LED2);
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);

	STM_EVAL_LEDOn(LED1);

	/* LCD Initialization */
	STM3210C_LCD_Init();
	LCD_Clear(LCD_COLOR_BLACK);
	LCD_SetBackColor(LCD_COLOR_BLACK);
	LCD_SetTextColor(LCD_COLOR_WHITE);
  
	/* init CAN and Timer */
	can_config();
	timer_config();	
	
	LCD_DisplayStringLine(LCD_LINE_1, "MaCAN initialized.");

	return 0;
}

/**
 * macan_send() - sends a can frame
 */
bool macan_send(struct macan_ctx *ctx,  const struct can_frame *cf)
{

	CanTxMsg TxMessage;
	uint8_t ret;
	int i;

	TxMessage.StdId = cf->can_id;
	TxMessage.ExtId = 0x01;
	TxMessage.RTR = CAN_RTR_DATA;
	TxMessage.IDE = CAN_ID_STD;
	TxMessage.DLC = cf->can_dlc;  

	/* copy data */
	for(i = 0; i < cf->can_dlc; i++) {
		TxMessage.Data[i] = cf->data[i];
	}

	ret = CAN_Transmit(CAN1, &TxMessage);

	if(ret == CAN_TxStatus_NoMailBox) {
		return false;
	}

	return true;
}
/*
 * Generate random bytes
 *
 * @param[in] dest Pointer to location where to store bytes
 * @param[in] len  Number of random bytes to be written
 */
bool gen_rand_data(void *dest, size_t len)
{
	uint8_t *p = (uint8_t *) dest;

	srand(0);
	while(len--) {
		p[len] = (uint8_t) rand();
	}
	return SUCCESS;

}

/*
 * Reads current time
 */
uint64_t read_time()
{
	return time;
}

/*
 * Poll CAN1 FIFO0
 */
void poll_can_fifo(void (*cback)(struct can_frame *cf, void *data), void *data)
{
	
	CanRxMsg can_rx_msg; 
	struct can_frame cf;
	int i;

	while(CAN_MessagePending(CAN1,CAN_FIFO0)) {
		CAN_Receive(CAN1,CAN_FIFO0,&can_rx_msg);	
		cf.can_id = can_rx_msg.StdId;
		cf.can_dlc = can_rx_msg.DLC;
		memcpy(cf.data,can_rx_msg.Data,8);	
		cback(&cf, data);
	}	  
}

void macan_target_init(struct macan_ctx *ctx)
{}
