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
#include <nettle/aes.h>
#include "common.h"
#include "helper.h"
#include "aes_keywrap.h"
#include "macan_private.h"
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>

#define NODE_COUNT 64

uint8_t ltk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

struct sess_key {
	bool valid;
	uint8_t key[16];
};

struct sess_key skey_map[NODE_COUNT - 1][NODE_COUNT] = {
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
};

void generate_skey(struct sess_key *skey)
{
	int i;

	skey->valid = true;

	for (i = 0; i < 16; i++)
		skey->key[i] = rand(); /* FIXME: Does this use /dev/random? Probably not - it should. */
}

uint8_t lookup_skey(uint8_t src, uint8_t dst, struct sess_key **key_ret)
{
	uint8_t tmp;
	struct sess_key *key;

	assert(src != dst);

	if (src > dst) {
		tmp = src;
		src = dst;
		dst = tmp;
	}

	key = &skey_map[src][dst];
	*key_ret = key;

	if (!key->valid) {
		generate_skey(key);
		return 1;
	}
	return 0;
}

void send_skey(struct macan_ctx *ctx, int s, struct aes_ctx * cipher, uint8_t dst_id, uint8_t fwd_id, uint8_t *chal)
{
	uint8_t wrap[32];
	uint8_t plain[24];
	struct sess_key *key;
	struct can_frame cf = {0};
	struct macan_sess_key skey;
	int i;

	/* ToDo: solve name inconsistency - key */
	if (lookup_skey(dst_id, fwd_id, &key)) {
		send_challenge(ctx, s, fwd_id, dst_id, NULL);
	}

	memcpy(plain, chal, 6);
	plain[6] = fwd_id;
	plain[7] = dst_id;
	memcpy(plain + 8, key, 16);
	aes_wrap(cipher, 24, wrap, plain);

	printf("send KEY (wrap, plain):\n");
	print_hexn(wrap, 32);
	print_hexn(plain, 24);

	skey.flags_and_dst_id = FL_SESS_KEY << 6;
	skey.flags_and_dst_id |= dst_id & 0x3F;

	cf.can_id = CANID(ctx, ctx->config->key_server_id);
	cf.can_dlc = 8;

	for (i = 0; i < 6; i++) {
		skey.seq_and_len = i << 4; // seq
		skey.seq_and_len |= (i == 5) ? 2 : 6; // len
		memcpy(skey.data, wrap + (6 * i), 6);
		memcpy(cf.data, &skey, 8);

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
	struct aes_ctx cipher;
	struct macan_challenge *chal;
	uint8_t dst_id, fwd_id;
	uint8_t *chg;

	aes_set_encrypt_key(&cipher, 16, ltk);
	chal = (struct macan_challenge *)cf->data;

	dst_id = canid2ecuid(ctx, cf->can_id);
	fwd_id = chal->fwd_id;
	chg = chal->chg;

	send_skey(ctx, s, &cipher, dst_id, fwd_id, chg);
}

void can_recv_cb(struct macan_ctx *ctx, int s, struct can_frame *cf)
{
	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;

	/* Reject non-crypt frames */
	if (canid2ecuid(ctx, cf->can_id) == -1)
		return;
	if (GET_DST_ID(cryf->flags_and_dst_id) != ctx->config->key_server_id)
		return;

	/* ToDo: do some check on challenge message, the only message recepted by KS */
	ks_receive_challenge(ctx, s, cf);
}

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_ctx ctx;
	struct macan_config *config = NULL;

	char opt;
	while ((opt = getopt(argc, argv, "c:")) != -1) {
		switch (opt) {
		case 'c': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			config = dlsym(handle, "config");
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

        config->node_id = config->key_server_id;
	srand(time(NULL));
	s = helper_init();
	macan_init(&ctx, config);

	while (1) {
		helper_read_can(&ctx, s, can_recv_cb);

		usleep(250);
	}

	return 0;
}
