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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "common.h"
#include "helper.h"
#include "macan_private.h"
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>
#include "cryptlib.h"

#define NODE_COUNT 64

struct sess_key {
	bool valid;
	uint8_t key[16];
};

struct sess_key skey_map[NODE_COUNT - 1][NODE_COUNT] = {
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
};

static struct macan_ctx macan_ctx;
static void *ltk_handle;

void generate_skey(struct sess_key *skey)
{
	skey->valid = true;
	if(!gen_rand_data(skey->key, 16)) {
		print_msg(MSG_FAIL,"Failed to read enough random bytes.\n");
		exit(1);
	}
}

uint8_t lookup_skey(macan_ecuid src_id, macan_ecuid dst_id, struct sess_key **key_ret)
{
	macan_ecuid tmp;
	struct sess_key *key;

	assert(src_id != dst_id);

	if (src_id > dst_id) {
		tmp = src_id;
		src_id = dst_id;
		dst_id = tmp;
	}

	key = &skey_map[src_id][dst_id];
	*key_ret = key;

	if (!key->valid) {
		generate_skey(key);
		return 1;
	}
	return 0;
}

void send_skey(struct macan_ctx *ctx, int s, const struct macan_key *key, macan_ecuid dst_id, macan_ecuid fwd_id, uint8_t *chal)
{
	uint8_t wrap[32];
	uint8_t plain[24];
	struct sess_key *skey;
	struct can_frame cf = {0};
	struct macan_sess_key macan_skey;
	int i;

	/* ToDo: solve name inconsistency - key */
	if (lookup_skey(dst_id, fwd_id, &skey)) {
		send_challenge(ctx, s, fwd_id, dst_id, NULL);
	}

	memcpy(plain, skey->key, 16);
	plain[16] = dst_id;
	plain[17] = fwd_id;
	memcpy(plain + 18, chal, 6);
	macan_aes_wrap(key, 24, wrap, plain);

	print_msg(MSG_INFO,"send KEY (wrap, plain):\n");
	print_hexn(wrap, 32);
	print_hexn(plain, 24);

	macan_skey.flags_and_dst_id = (macan_ecuid)(FL_SESS_KEY << 6 | (dst_id & 0x3F));

	cf.can_id = CANID(ctx, ctx->config->key_server_id);
	cf.can_dlc = 8;

	for (i = 0; i < 6; i++) {
		macan_skey.seq_and_len = (uint8_t)((i << 4) /* seq */ | ((i == 5) ? 2 : 6) /* len */);
		memcpy(macan_skey.data, wrap + (6 * i), 6);
		memcpy(cf.data, &macan_skey, 8);

		/* ToDo: check all writes for success */
		write(s, &cf, sizeof(struct can_frame));
	}
}

/**
 * ks_receive_challenge() - responds to challenge message
 * @s:   socket fd
 * @cf:  received can frame
 *
 * This function responds with session key to the challenge sender
 * and also sends REQ_CHALLENGE to communication partner of the sender.
 */
void ks_receive_challenge(struct macan_ctx *ctx, int s, struct can_frame *cf)
{
	struct macan_challenge *chal;
	macan_ecuid dst_id, fwd_id;
	uint8_t *chg;
	macan_ecuid ecu_id;
	const struct macan_key *ltk;
	char node_id_str[100];
	int cnt;
	char *error;

	chal = (struct macan_challenge *)cf->data;

	if(!canid2ecuid(ctx, cf->can_id, &ecu_id)) {
		return;
	}

	dst_id = ecu_id;
	fwd_id = chal->fwd_id;
	chg = chal->chg;

	cnt = sprintf(node_id_str,"%s","macan_ltk_node");
	sprintf(node_id_str+cnt,"%u",dst_id);
	ltk = dlsym(ltk_handle,node_id_str);
	error = dlerror();
	if(error != NULL) {
		print_msg(MSG_FAIL,"Unable to load ltk key for node #%u from shared library\nReason: %s\n",dst_id,error);
		return;
	}
	print_hexn(ltk, 16);
	send_skey(ctx, s, ltk, dst_id, fwd_id, chg);
}

void can_recv_cb(int s, struct can_frame *cf)
{
	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;

	/* reject frames that can't be challenge */
	if (cf->can_dlc < 8)
		return;
	if (GET_DST_ID(cryf->flags_and_dst_id) != macan_ctx.config->key_server_id)
		return;
	/* check flags, must be challege */
	if (GET_FLAGS(cryf->flags_and_dst_id) != FL_CHALLENGE)
		return;

	/* ToDo: do some check on challenge message, the only message recepted by KS */
	ks_receive_challenge(&macan_ctx, s, cf);
}

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib> -k <ltk_lib>\n", argv0);
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
		case 'k':
			ltk_handle = dlopen(optarg, RTLD_LAZY);
			if(!ltk_handle) {
			   fprintf(stderr, "%s\n", dlerror());
			   exit(1);
			}
			break;
		default: /* '?' */
			print_help(argv[0]);
			exit(1);
		}
	}
	if (!config) {
		print_help(argv[0]);
		exit(1);
	}

	config->node_id = config->key_server_id;
	s = helper_init();
	macan_init(&macan_ctx, config);

	while (1) {
		helper_read_can(&macan_ctx, s, can_recv_cb);

		usleep(250);
	}

	return 0;
}
