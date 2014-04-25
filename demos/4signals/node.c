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
#endif /* __CPU_TC1798__ */
#include "helper.h"
#include "macan.h"
#include <macan_private.h> 	/* FIXME: Needed for read_time - replace with macan_get_time */
#include "macan_config.h"

#include <ev.h>

#define TIME_EMIT_SIG 1000000

static struct macan_ctx macan_ctx;
extern const struct macan_key MACAN_CONFIG_LTK(NODE_ID);


static void
send_cb (struct ev_loop *loop, ev_timer *w, int revents)
{
	(void)loop; (void)revents;
	struct macan_ctx *ctx = w->data;

	macan_send_sig(ctx, SIGNAL_A, 10);
	macan_send_sig(ctx, SIGNAL_B, 200000);
	macan_send_sig(ctx, SIGNAL_C, 30);
	macan_send_sig(ctx, SIGNAL_D, 400000);
}


void sig_callback(uint8_t sig_num, uint32_t sig_val)
{
	printf("received authorized signal(%"PRIu8") = %"PRIu32"\n", sig_num, sig_val);
}

int main()
{
	int s;
	struct ev_loop *loop = MACAN_EV_DEFAULT;
	ev_timer sig_send;

	s = helper_init();

	// put node id to config struct
	config.node_id = NODE_ID;

	config.ltk = &MACAN_CONFIG_LTK(NODE_ID);

	macan_init(&macan_ctx, &config, loop, s);
	macan_reg_callback(&macan_ctx, SIGNAL_A, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_B, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_C, sig_callback, NULL);
	macan_reg_callback(&macan_ctx, SIGNAL_D, sig_callback, NULL);

	macan_ev_timer_init(&sig_send, send_cb, 1000, 1000);
	sig_send.data = &macan_ctx;
	macan_ev_timer_start(loop, &sig_send);

	macan_ev_run(loop);

	return 0;
}
