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
#include <nettle/aes.h>
#endif /* __CPU_TC1798__ */
#include "helper.h"
#include "macan_private.h"
#include "macan_config.h"
#include "can_recv_cb.h"

#define TIME_EMIT_SIG 1000000

static struct macan_ctx macan_ctx;

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
	ClearEndinit();
	// set all led pins as outputs
	P4_IOCR0.U = 0x80808080;
	P4_IOCR4.U = 0x80808080;
	// write output
	P4_OUT.U = ~value;
	SetEndinit();
}

void handle_io(void)
{
	P7_IOCR4.B.PC5 = 0x2; // P7.5 is input with pull-up switch
	int pressed = !P7_IN.B.P5;

	P7_IOCR0.B.PC0 = 0x8; // P7.0 is output (red LED)
	P7_OUT.B.P0 = pressed;
	led_set(pressed ? 0xf0 : 0x0f);

	//Adc_ReadGroup(0, 0);
}

void io_init(void)
{
  //Adc_Init(Adc_ConfigRoot);
}

#else
void io_init(void)
{

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
		handle_io();
#else
		helper_read_can(ctx, s, can_recv_cb);
#endif /* __CPU_TC1798__ */

		macan_request_keys(ctx, s);
		macan_wait_for_key_acks(ctx, s);
		if (signal_time < read_time()) {
			signal_time = read_time() + TIME_EMIT_SIG;
			macan_send_sig(ctx, s, SIGNAL_B, 10);
		}

#ifndef __CPU_TC1798__
		usleep(250);
#endif /* __CPU_TC1798__ */
	}
}

void sig_callback(uint8_t sig_num, uint32_t sig_val)
{
	printf("received authorized signal(%"PRIu8") = %"PRIu32"\n", sig_num, sig_val);
}

int main()
{
	int s;

	s = helper_init();
	io_init();
	macan_init(&macan_ctx, &config);
	macan_reg_callback(&macan_ctx, SIGNAL_A, sig_callback);
	operate_ecu(&macan_ctx, s);

	return 0;
}
