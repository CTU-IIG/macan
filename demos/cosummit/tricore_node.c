/*
 *  Copyright 2014, 2015 Czech Technical University in Prague
 *
 *  Authors: Michal Sojka <sojkam1@fel.cvut.cz>
 *           Radek Matějka <radek.matejka@gmail.com>
 *           Ondřej Kulatý <kulatond@fel.cvut.cz>
 *
 *  This file is part of MaCAN.
 *
 *  MaCAN is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  MaCAN is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MaCAN.	If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include "common.h"
#ifdef __CPU_TC1798__
#include "can_frame.h"
#include "Std_Types.h"
#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "EcuM.h"
#include "Test_Print.h"
#include "Os.h"
#include "she.h"
#include <can_fifo.h>
#include <sfr/regtc1798.sfr>
#include <Adc.h>
#include <Adc_Cfg.h>
#else
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif /* __CPU_TC1798__ */
#include "helper.h"
#include "macan_private.h"
#include "macan_config.h"
#include "can_recv_cb.h"
#include <math.h>

#define TIME_EMIT_SIG 1000000

#define LED_EXP_BOARD_1_4_CFG	(P2_IOCR8.U)
#define LED_EXP_BOARD_4_8_CFG	(P2_IOCR12.U)
#define LED_EXP_BOARD_VAL	(P2_OUT.U)
#define LED_BLINKING_CFG	(P4_IOCR0.B.PC0)
#define LED_BLINKING_VAL	(P4_OUT.B.P0)
#define LED_ERROR_CFG		(P2_IOCR12.B.PC15)
#define LED_ERROR_VAL		(P2_OUT.B.P15)
#define LED_BUTTONS_CFG		(P2_IOCR8.U)
#define LED_BUTTONS_VAL		(P2_OUT.U)
#define LED_BUTTON1_CFG		(P2_ICR12.B.PC13)
#define LED_BUTTON1_VAL		(P2_OUT.B.P13)

#define LED_ONBOARD1_4_CFG		(P4_IOCR0.U)
#define LED_ONBOARD4_8_CFG		(P4_IOCR4.U)
#define LED_ONBOARD_VAL			(P4_OUT.U)

#define BTN_EXP_BOARD_CFG	(P4_IOCR4.U)
#define BTN_BUTTON1_CFG		(P4_IOCR4.B.PC4)
#define BTN_BUTTON1_VAL		(P4_IN.B.P4)

struct macan_ctx macan_ctx;
extern const struct macan_key macan_ltk_node3;

int button_pressed;

#ifdef __CPU_TC1798__
void ClearEndinit(void)
{
  uint32_t u32WdtCon0, u32WdtCon1;
  uint32_t pwd = 0;

  u32WdtCon0 = WDT_CON0.U;
  u32WdtCon1 = WDT_CON1.U;

  /* unlock */
  //pwd |= u32WdtCon0 & 0x1;
  //pwd |= u32WdtCon0 & 0x4;

  u32WdtCon0 &= 0xFFFFFF03;
  u32WdtCon0 |= 0xF0;
  u32WdtCon0 |= (u32WdtCon1 & 0xc);
  u32WdtCon0 ^= 0x2;
  WDT_CON0.U = u32WdtCon0;

  /* Modify access to u32WdtCon0 */
  u32WdtCon0 &= 0xFFFFFFF0;
  u32WdtCon0 |= 2;
  WDT_CON0.U = u32WdtCon0;
}

void SetEndinit(void)
{
  uint32_t u32WdtCon0, u32WdtCon1;

  u32WdtCon0 = WDT_CON0.U;
  u32WdtCon1 = WDT_CON1.U;

  u32WdtCon0 &= 0xFFFFFF03;
  u32WdtCon0 |= 0xF0;
  u32WdtCon0 |= (u32WdtCon1 & 0xc);
  u32WdtCon0 ^= 0x2;
  WDT_CON0.U = u32WdtCon0;

  /* Modify access to u32WdtCon0 */
  u32WdtCon0 &= 0xFFFFFFF0;
  u32WdtCon0 |= 3;
  WDT_CON0.U = u32WdtCon0;
}

void handle_io(void)
{
	static uint64_t last_blink = 0;

	button_pressed = !BTN_BUTTON1_VAL;
	LED_BUTTON1_VAL = button_pressed; // One blue LED shows the button state
	if ((int)read_time() - last_blink > 500000) {
		static int blink = 0;
		last_blink = read_time();
		LED_BLINKING_VAL = (blink = !blink);
	}
	//Adc_ReadGroup(0, 0);
}

void io_init(void)
{
	/* Initialize all onboard LEDs to be outputs and log. 0
	 * Some of the GPIOS (P4.4 - P4.7) are shared with Buttons on
	 * the expansion board, so they will be reinitialized later and
	 * will by lightning when the button is pressed.
	 */
	LED_ONBOARD1_4_CFG = 0x80808080;
	LED_ONBOARD4_8_CFG = 0x80808080;
	LED_ONBOARD_VAL = 0xFFFFFFFF;
	LED_BLINKING_CFG = 0x8;
	LED_BLINKING_VAL = 0x1;

	/*
	 * Initialize all LEDs on the Expansion board as output with
	 * log. 0 value.
	 */
	LED_EXP_BOARD_1_4_CFG = 0x80808080;
	LED_EXP_BOARD_4_8_CFG = 0x80808080;
	LED_EXP_BOARD_VAL = 0x0;
	LED_BUTTONS_CFG = 0x80808080;

	/*
	 * Initialize Button 1 on the expansion board as input,
	 * pull-up.
	 */
	BTN_BUTTON1_CFG = 0x2;
  //Adc_Init(Adc_ConfigRoot);
}

#else
void io_init(void)
{

}

void handle_io(void)
{
	button_pressed = (int)(read_time() / 1000000U / 3U);
}
#endif


macan_ev_timer timeout;

static void
btn_chk_cb (macan_ev_loop *loop, macan_ev_timer *w, int revents)
{
	(void)loop; (void)revents;
	struct macan_ctx *ctx = w->data;
	static int t = 0;

	handle_io();

	int val = (int)((button_pressed ? -1 : +1) * 100*sin(2.0*3.14159265*(0.3*t/20.0)));

	macan_send_sig(ctx, SIGNAL_SIN1, (uint32_t)val);
	macan_send_sig(ctx, SIGNAL_SIN2, (uint32_t)val);

	t++;
	macan_ev_timer_again(loop, &timeout); /* Reset timeout */
}

void sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status s)
{
	printf("received signal(%"PRIu8") = %#"PRIx32" status: %d\n", sig_num, sig_val, s);
#ifdef __CPU_TC1798__
	LED_BUTTONS_VAL = (LED_BUTTONS_VAL & 0xfffff0ff) | (sig_val & 0xf) << 8; // Blue LEDs
	LED_ERROR_VAL = 0; // Red LED off
#endif
}

void sig_invalid(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status s)
{
	printf("received invalid signal(%"PRIu8") = %#"PRIx32" status: %d\n", sig_num, sig_val, s);
#ifdef __CPU_TC1798__
	LED_ERROR_VAL = 1; // Red LED on

#endif
}

int main()
{
	int s; macan_ev_loop *loop = MACAN_EV_DEFAULT;
	macan_ev_timer btn_chk;
	struct macan_node_config node = {
		.node_id = NODE_TC,
		.ltk = &macan_ltk_node3,
	};

	s = helper_init("can0");
	io_init();
	macan_init(&macan_ctx, &config, &node, loop, s);
	macan_reg_callback(&macan_ctx, SIGNAL_LED, sig_callback, sig_invalid);

	macan_ev_timer_setup (&macan_ctx, &btn_chk, btn_chk_cb, 0, 50);

	macan_ev_run(loop);

	return 0;
}
