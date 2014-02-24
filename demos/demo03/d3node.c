/*
 *  Copyright 2014 Czech Technical University in Prague
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

#define TIME_EMIT_SIG 1000000

static struct macan_ctx macan_ctx;
extern const uint8_t macan_ltk_node4;

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

/* will light up onboard LEDs according to bits in "value" argument*/
void led_set(uint8_t value) {
	P4_OUT.U = ~value;
}

void handle_io(void)
{
	static uint64_t last_blink = 0;

	button_pressed = !P7_IN.B.P5;
	P4_OUT.B.P7 = !button_pressed; // One blue LED shows the button state
	if ((int)read_time() - last_blink > 500000) {
		static int blink = 0;
		last_blink = read_time();
		P4_OUT.B.P6 = (blink = !blink);
	}
	//Adc_ReadGroup(0, 0);
}

void io_init(void)
{
	// Blue LEDs on P4
	P4_IOCR0.U = 0x80808080;
	P4_IOCR4.U = 0x80808080;
	led_set(0);

	P7_IOCR4.B.PC5 = 0x2; // P7.5 is input with pull-up switch
	P7_IOCR0.B.PC0 = 0x8; // P7.0 is output (red LED)
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

void can_recv_cb(int s, struct can_frame *cf)
{
	macan_process_frame(&macan_ctx, s, cf);
}

void operate_ecu(struct macan_ctx *ctx, int s)
{
	uint64_t signal_time = 0;

	while(1) {
#ifdef __CPU_TC1798__
		poll_can_fifo(can_recv_cb);
#else
		helper_read_can(ctx, s, can_recv_cb);
#endif /* __CPU_TC1798__ */
		handle_io();

		macan_request_keys(ctx, s);
		macan_wait_for_key_acks(ctx, s);
		macan_send_signal_requests(ctx, s);

		static int last_pressed = 0;
		if (signal_time < read_time() || last_pressed != button_pressed) {
			signal_time = read_time() + TIME_EMIT_SIG;
			last_pressed = button_pressed;
			macan_send_sig(ctx, s, SIGNAL_CTU, (uint32_t)button_pressed);
		}

#ifndef __CPU_TC1798__
		usleep(250);
#endif /* __CPU_TC1798__ */
	}
}

void sig_callback(uint8_t sig_num, uint32_t sig_val)
{
	printf("received authentic signal(%"PRIu8") = %#"PRIx32"\n", sig_num, sig_val);
#ifdef __CPU_TC1798__
	P4_OUT.U = ~(~P4_OUT.U & 0xf0 | sig_val & 0xf); // Blue LEDs
	P7_OUT.B.P0 = 0; // Red LED off
#endif
}

void sig_invalid(uint8_t sig_num, uint32_t sig_val)
{
	printf("received invalid signal(%"PRIu8") = %#"PRIx32"\n", sig_num, sig_val);
#ifdef __CPU_TC1798__
	P7_OUT.B.P0 = 1; // Red LED on

#endif
}

int main()
{
	int s;

	config.ltk = &macan_ltk_node4;

	s = helper_init();
	io_init();
	macan_init(&macan_ctx, &config);
	macan_reg_callback(&macan_ctx, SIGNAL_VW, sig_callback, sig_invalid);
	operate_ecu(&macan_ctx, s);

	return 0;
}
