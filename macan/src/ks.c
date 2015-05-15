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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "cryptlib.h"
#include "macan.h"
#include "macan_ev.h"
#include "macan_private.h"

#define NODE_COUNT 64

struct sess_key {
	bool valid;
	struct macan_key key;
};

static void generate_skey(struct macan_ctx *ctx, struct sess_key *skey)
{
	skey->valid = true;
	if(!gen_rand_data(skey->key.data, sizeof(skey->key.data))) {
		print_msg(ctx, MSG_FAIL,"Failed to read enough random bytes.\n");
		exit(1);
	}
}

static bool lookup_or_generate_skey(struct macan_ctx *ctx, macan_ecuid src_id, macan_ecuid dst_id, struct macan_key **key_ret)
{
	static struct sess_key skey_map[NODE_COUNT - 1][NODE_COUNT] = {{{0}}};

	struct sess_key *key;

	if (src_id > dst_id) {
		macan_ecuid tmp = src_id;
		src_id = dst_id;
		dst_id = tmp;
	}

	key = &skey_map[src_id][dst_id];
	*key_ret = &key->key;

	/* TODO: regenerate key when it expires */
	if (!key->valid) {
		generate_skey(ctx, key);
		return 1;
	}
	return 0;
}

static
void send_req_challenge(struct macan_ctx *ctx, macan_ecuid dst_id, macan_ecuid fwd_id)
{
	struct can_frame cf = {0};
	struct macan_req_challenge *rch = (struct macan_req_challenge*)&cf.data;

	rch->flags_and_dst_id = FL_REQ_CHALLENGE << 6 | (dst_id & 0x3f);
	rch->fwd_id = fwd_id;

	cf.can_id = CANID(ctx, ctx->config->key_server_id);
	cf.can_dlc = sizeof(*rch);
	macan_send(ctx, &cf);
}

static
void send_skey(struct macan_ctx *ctx,
	       const struct macan_key *ltk,
	       const struct macan_key *skey,
	       macan_ecuid dst_id,
	       macan_ecuid fwd_id,
	       uint8_t *chal)
{
	uint8_t wrap[32];
	uint8_t plain[24];
	struct can_frame cf = {0};
	struct macan_sess_key skey_frame;
	unsigned i;

	memcpy(plain, skey->data, sizeof(skey->data));
	plain[16] = dst_id;
	plain[17] = fwd_id;
	memcpy(plain + 18, chal, 6);
	macan_aes_wrap(ltk, 24, wrap, plain);

/* 	print_msg(ctx, MSG_INFO,"send KEY (wrap, plain):\n"); */
/* 	print_hexn(wrap, 32); */
/* 	print_hexn(plain, 24); */

	skey_frame.flags_and_dst_id = (macan_ecuid)(FL_SESS_KEY << 6 | (dst_id & 0x3F));

	cf.can_id = CANID(ctx, ctx->config->key_server_id);
	cf.can_dlc = sizeof(skey_frame);

	for (i = 0; i < 6; i++) {
		unsigned len = (i == 5) ? 2 : 6;
		skey_frame.seq_and_len = (uint8_t)((i << 4) /* seq */ | len);
		memcpy(skey_frame.data, wrap + (6 * i), len);
		memcpy(cf.data, &skey_frame, sizeof(skey_frame));

		/* ToDo: check all writes for success */
		macan_send(ctx, &cf);
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
void ks_receive_challenge(struct macan_ctx *ctx, struct can_frame *cf)
{
	struct macan_challenge *chal;
	macan_ecuid dst_id, fwd_id;
	uint8_t *chg;

	if (cf->can_dlc != 8 ||
	    !macan_canid2ecuid(ctx->config, cf->can_id, &dst_id))
		return;

	chal = (struct macan_challenge *)cf->data;

	fwd_id = chal->fwd_id;
	chg = chal->chg;

	if (fwd_id == dst_id ||
	    fwd_id >= ctx->config->node_count ||
	    dst_id == ctx->config->key_server_id)
		return;

	const struct macan_key *ltk = ctx->ks.ltk[dst_id];
	struct macan_key *skey;
	bool new_key = lookup_or_generate_skey(ctx, dst_id, fwd_id, &skey);
	send_skey(ctx, ltk, skey, dst_id, fwd_id, chg);
	if (new_key)
		send_req_challenge(ctx, fwd_id, dst_id);
}

static void
can_cb_ks (macan_ev_loop *loop, macan_ev_can *w, int revents)
{
	(void)loop; (void)revents; /* suppress warnings */
	struct macan_ctx *ctx = w->data;
	struct can_frame cf;

	macan_read(ctx, &cf);

	/* Simple sanity checks first */
	if (cf.can_dlc < 1 ||
	    macan_crypt_dst(&cf) != ctx->config->key_server_id ||
	    macan_crypt_flags(&cf) != FL_CHALLENGE)
		return;

	/* All other checks are done in ks_receive_challenge() */
	ks_receive_challenge(ctx, &cf);
}

int macan_init_ks(struct macan_ctx *ctx, macan_ev_loop *loop, int sockfd,
		  const struct macan_key * const *ltks)
{
	assert(ctx->node->node_id == ctx->config->key_server_id);

	__macan_init(ctx, loop, sockfd);

	macan_ev_canrx_setup(ctx, &ctx->can_watcher, can_cb_ks);

	ctx->ks.ltk = ltks;

	return 0;
}
