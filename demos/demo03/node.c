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
#include "aes_keywrap.h"
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
#else
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <nettle/aes.h>
#include "aes_cmac.h"
#endif /* __CPU_TC1798__ */
#include "helper.h"
#include "macan_private.h"
#include "macan_config.h"

#define TIME_EMIT_SIG 1000000

void can_recv_cb(struct macan_ctx *ctx, int s, struct can_frame *cf)
{
	macan_process_frame(ctx, s, cf);
}

void operate_ecu(struct macan_ctx *ctx, int s)
{
	uint64_t signal_time = 0;

	while(1) {
#ifdef __CPU_TC1798__
		poll_can_fifo(ctx, can_recv_cb);
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
	(*( unsigned int *) 0xf0001010u) = 0x80808080; // register P4_IOCR0
	(*( unsigned int *) 0xf0001014u) = 0x80808080; // register P4_IOCR4
	// write output
	(*( unsigned int *) 0xf0001004u) = ~value; // register P4_OMR
	SetEndinit();
}
/* test if button is pressed, returns 0 if pressed
 * Pozor: uz jsem nestihl otestovat jestli tato funkce funguje.
 * */
int is_button_pressed() {
	ClearEndinit();
	// init P4.7 as input
	(*( unsigned int *) 0xf0001014u) = 0x00202020; // register P4_IOCR0
	SetEndinit();
	return (*( unsigned int *) 0xf0001024u) & 0x00000080; // register P4_IN
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_ctx ctx;

	s = helper_init();
	macan_init(&ctx, &config);
	macan_reg_callback(&ctx, SIGNAL_A, sig_callback);
	operate_ecu(&ctx, s);

	return 0;
}
