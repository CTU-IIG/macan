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
#include "macan.h"
#include <macan_private.h> 	/* FIXME: Needed for read_time - replace with macan_get_time */
#include "macan_config.h"

#define TIME_EMIT_SIG 1000000

static struct macan_ctx macan_ctx;

/* ltk stands for long term key; it is a key shared with the key server */
uint8_t ltk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

void can_recv_cb(int s, struct can_frame *cf)
{
	macan_process_frame(&macan_ctx, s, cf);
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
			macan_send_sig(ctx, s, SIGNAL_A, 10);
			macan_send_sig(ctx, s, SIGNAL_B, 200000);
			macan_send_sig(ctx, s, SIGNAL_C, 30);
			macan_send_sig(ctx, s, SIGNAL_D, 400000);
		}

#ifndef __CPU_TC1798__
		usleep(250);
#endif /* __CPU_TC1798__ */
	}
}

void sig_callback(uint8_t sig_num, uint32_t sig_val)
{
	//printf("received authorized signal(%"PRIu8") = %"PRIu32"\n", sig_num, sig_val);
}

int main(int argc, char *argv[])
{
	int s;

	s = helper_init();

	// put node id to config struct
	config.node_id = NODE_ID;

	macan_init(&macan_ctx, &config);
	macan_reg_callback(&macan_ctx, SIGNAL_A, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_B, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_C, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_D, sig_callback, NULL);
	operate_ecu(&macan_ctx, s);

	return 0;
}
