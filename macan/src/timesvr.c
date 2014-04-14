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
#endif /* TC1798 */
#include "helper.h"
#include <macan.h>
#include <macan_private.h>
#include <dlfcn.h>
#include <cryptlib.h>

/* ToDo
 *   implement groups
 *   some error processing
 */

#define TS_TEST_ERR 500000

uint64_t macan_time;
static struct macan_ctx macan_ctx;

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
	struct macan_key skey;
	uint8_t plain[12];
	macan_ecuid dst_id;
	struct com_part *cp;

	if(!(cp = canid2cpart(ctx, cf->can_id)))
		return -1;

	dst_id = (uint8_t) (cp->ecu_id);

	if (!is_skey_ready(ctx, dst_id)) {
		print_msg(MSG_FAIL,"cannot send time, because don't have key\n");
		return -1;
	}

	skey = cp->skey;

	memcpy(plain, &macan_time, 4);
	memcpy(plain + 4, ch->chg, 6);
	memcpy(plain + 10, &CANID(ctx, ctx->config->time_server_id), 2);

	canf.can_id = CANID(ctx,ctx->config->time_server_id);
	canf.can_dlc = 8;
	memcpy(canf.data, &macan_time, 4);
	macan_sign(&skey, canf.data + 4, plain, 12);

	write(s, &canf, sizeof(canf));

	print_msg(MSG_INFO,"signed time sent\n");
	return 0;
}

void can_recv_cb(int s, struct can_frame *cf)
{
	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;
	struct macan_ctx *ctx = &macan_ctx;

	/* ToDo: make sure all branches end ASAP */
	/* ToDo: macan or plain can */
	/* ToDo: crypto frame or else */
	if (cf->can_id == CANID(ctx, ctx->config->time_server_id))
		return;
	if (GET_DST_ID(cryf->flags_and_dst_id) != ctx->config->time_server_id)
		return;

	switch (GET_FLAGS(cryf->flags_and_dst_id)) {
	case FL_CHALLENGE:
		if (cf->can_id == CANID(ctx, ctx->config->key_server_id)) {
			receive_challenge(ctx, s, cf);
		} else
		{
			ts_receive_challenge(ctx, s, cf);
		}
		break;
	case FL_SESS_KEY:
		if (cf->can_id == CANID(ctx, ctx->config->key_server_id)) {
			receive_skey(ctx, cf);
			break;
		}
		break;
	case FL_SIGNAL_OR_AUTH_REQ:
		if (cf->can_dlc == 7)
			receive_auth_req(ctx, cf);
		else
			receive_sig16(ctx, cf);
		break;
	}
}

/**
 * broadcast_time() - broadcasts unsigned time
 * @s:    socket handle
 * @freq: broadcast frequency
 *
 */
void broadcast_time(struct macan_ctx *ctx, int s, uint64_t *bcast_time)
{
	struct can_frame cf = {0};
	uint64_t usec;

	if (*bcast_time + ctx->config->time_bcast_period > read_time())
		return;

	*bcast_time = read_time();

	usec = read_time();
	macan_time = usec / ctx->config->time_div;

	cf.can_id = CANID(ctx,ctx->config->time_server_id);
	cf.can_dlc = 4;
	memcpy(cf.data, &macan_time, 4);

	write(s, &cf, sizeof(cf));
}

void operate_ts(struct macan_ctx *ctx, int s)
{
	uint64_t bcast_time = read_time();

	while(1) {
		helper_read_can(ctx, s, can_recv_cb);
		macan_request_keys(ctx, s);
		broadcast_time(ctx, s, &bcast_time);

		usleep(250);
	}
}

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib> -k <key_shlib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_config *config = NULL;

	int opt;
	while ((opt = getopt(argc, argv, "c:k:")) != -1) {
		switch (opt) {
		case 'c': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			config = dlsym(handle, "config");
			break;
		}
		case 'k': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			if(!handle) {
			   fprintf(stderr, "%s\n", dlerror());
			   exit(1);
			}
			char str[100];
			int cnt = sprintf(str,"%s","macan_ltk_node");
			sprintf(str+cnt,"%u",config->time_server_id);
			config->ltk = dlsym(handle,"macan_ltk_node1");
			char *error = dlerror();
			if(error != NULL) {
				print_msg(MSG_FAIL,"Unable to load ltk key from shared library\nReason: %s\n",error);
				exit(1);
			}
			break;
		}
		default: /* '?' */
			print_help(argv[0]);
			exit(1);
		}
	}
	if (!config) {
		print_help(argv[0]);
		exit(1);
	}
        config->node_id = config->time_server_id;

	s = helper_init();
	macan_init(&macan_ctx, config);
	operate_ts(&macan_ctx, s);

	return 0;
}
