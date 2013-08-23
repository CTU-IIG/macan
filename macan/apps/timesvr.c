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
#include "common.h"
#include "aes_keywrap.h"
#ifdef TC1798
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
#endif /* TC1798 */
#include "helper.h"
#include "macan.h"
#include "macan_config.h"

/* ToDo
 *   implement groups
 *   some error processing
 */

#define NODE_ID NODE_TS
#define TS_TEST_ERR 500000

uint64_t last_usec;

/* ltk stands for long term key; it is a key shared with the key server */
uint8_t ltk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

/**
 * ts_receive_challenge() - serves the request for signed time
 * @s:  socket handle
 * @cf: received can frame with TS challenge
 *
 * This function responds to a challenge message from a general node (i.e. the
 * request for a signed time). It prepares a message containing the recent time
 * and signs it. Subsequently, the message is sent.
 */
int ts_receive_challenge(struct macan_ctx *ctx, int s, struct can_frame *cf)
{
	struct can_frame canf;
	struct macan_challenge *ch = (struct macan_challenge *)cf->data;
	uint8_t *skey;
	uint8_t plain[10];
	uint8_t dst_id;
	struct com_part **cpart;

	cpart = ctx->cpart;
	dst_id = cf->can_id;

	if (!is_skey_ready(ctx, dst_id)) {
		printf("cannot send time, because dont have key\n");
		return -1;
	}

	skey = cpart[dst_id]->skey;

	memcpy(plain, ch->chg, 6);
	memcpy(plain + 6, &last_usec, 4);

	canf.can_id = SIG_TIME;
	canf.can_dlc = 8;
	memcpy(canf.data, &last_usec, 4);
	sign(skey, canf.data + 4, plain, 10);

	write(s, &canf, sizeof(canf));

	printf("signed time sent\n");
	return 0;
}

void can_recv_cb(struct macan_ctx *ctx, int s, struct can_frame *cf)
{
	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;

	/* ToDo: make sure all branches end ASAP */
	/* ToDo: macan or plain can */
	/* ToDo: crypto frame or else */
	if (cf->can_id == NODE_ID)
		return;
	if (cryf->dst_id != NODE_ID)
		return;

	switch (cryf->flags) {
	case 1:
		if (cf->can_id == NODE_KS) {
			receive_challenge(ctx, s, cf);
		} else
		{
			ts_receive_challenge(ctx, s, cf);
		}
		break;
	case 2:
		if (cf->can_id == NODE_KS) {
			receive_skey(ctx, cf);
			break;
		}
		break;
	case 3:
		if (cf->can_dlc == 7)
			receive_auth_req(ctx, cf);
		else
			receive_sig(ctx, cf);
		break;
	}
}

/**
 * broadcast_time() - broadcasts unsigned time
 * @s:    socket handle
 * @freq: broadcast frequency
 *
 */
void broadcast_time(int s, uint64_t *bcast_time)
{
	struct can_frame cf;
	uint64_t usec;

	if (*bcast_time + 1000000 > read_time())
		return;

	*bcast_time = read_time();

	usec = read_time() + TS_TEST_ERR;
	last_usec = usec;

	cf.can_id = SIG_TIME;
	cf.can_dlc = 4;
	memcpy(cf.data, &usec, 4);

	write(s, &cf, sizeof(cf));
}

void operate_ts(struct macan_ctx *ctx, int s)
{
	uint64_t bcast_time = read_time();

	while(1) {
		helper_read_can(ctx, s, can_recv_cb);
		macan_request_keys(ctx, s);
		broadcast_time(s, &bcast_time);

		usleep(250);
	}
}

int main(int argc, char *argv[])
{
	struct macan_ctx ctx;
	int s;

	s = helper_init();
	macan_init(&ctx, demo_sig_spec);
	macan_set_ltk(&ctx, ltk);
	operate_ts(&ctx, s);

	return 0;
}

