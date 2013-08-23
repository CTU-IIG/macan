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
#include "macan_config.h"
#include "macan.h"

#ifdef TC1798
# define NODE_ID 3
# define CAN_IF 1 /* Hth on TC1798, iface name on pc */
#endif

#define SIG_DONTSIGN -1
#define SIG_SIGNONCE 0

#define AUTHREQ_SENT 1

void init_cpart(struct com_part **cpart, uint8_t i)
{
	cpart[i] = malloc(sizeof(struct com_part));
	memset(cpart[i], 0, sizeof(struct com_part));
	cpart[i]->wait_for = 1 << i | 1 << NODE_ID;
}

/**
 * macan_init()
 */
int macan_init(struct macan_ctx *ctx, const struct macan_sig_spec *sig_spec)
{
	uint8_t cp;
	int i;

	ctx->cpart = malloc(NODE_COUNT * sizeof(struct com_part *));
	memset(ctx->cpart, 0, NODE_COUNT * sizeof(struct com_part *));
	ctx->sighand = malloc(SIG_COUNT * sizeof(struct sig_handle *));
	memset(ctx->sighand, 0, SIG_COUNT * sizeof(struct sig_handle *));

	for (i = 0; i < SIG_COUNT; i++) {
		if (sig_spec[i].src_id == NODE_ID) {
			cp = sig_spec[i].dst_id;

			if (ctx->cpart[cp] == NULL) {
				init_cpart(ctx->cpart, cp);
			}
		}
		if (sig_spec[i].dst_id == NODE_ID) {
			cp = sig_spec[i].src_id;

			if (ctx->cpart[cp] == NULL) {
				init_cpart(ctx->cpart, cp);
			}
		}

		ctx->sighand[i] = malloc(sizeof(struct sig_handle));
		ctx->sighand[i]->presc = SIG_DONTSIGN;
		ctx->sighand[i]->presc_cnt = 0;
		ctx->sighand[i]->flags = 0;
		ctx->sighand[i]->cback = NULL;
	}

	return 0;
}

void macan_set_ltk(struct macan_ctx *ctx, uint8_t *key)
{
	memcpy(ctx->ltk, key, 16);
}

/* ToDo: reconsider name or move signal requests */
int macan_wait_for_key_acks(struct macan_ctx *ctx, int s, const struct macan_sig_spec *sig_spec, uint64_t *ack_time)
{
	int i;
	int r = 0;
	uint8_t cp, presc;
	struct com_part **cpart;
	struct sig_handle **sighand;

	cpart = ctx->cpart;
	sighand = ctx->sighand;

	if (*ack_time + ACK_TIMEOUT > read_time())
		return -1;
	*ack_time = read_time();

	for (i = 2; i < NODE_COUNT; i++) {
		if (cpart[i] == NULL)
			continue;

		if (!is_skey_ready(ctx, i))
			continue;

		if (!is_channel_ready(ctx, i)) {
			r++;
			send_ack(ctx, s, i);
			continue;
		}
	}

	for (i = 0; i < SIG_COUNT; i++) {
		if (sig_spec[i].dst_id != NODE_ID)
			continue;

		cp = sig_spec[i].src_id;
		presc = sig_spec[i].presc;

		if (!is_channel_ready(ctx, cp))
			continue;

		if (!(sighand[i]->flags & AUTHREQ_SENT)) {
			sighand[i]->flags |= AUTHREQ_SENT;
			send_auth_req(ctx, s, cp, i, presc);
		}
	}

	return r;
}

/**
 * write() - sends a can frame
 * @s:   ignored
 * @cf:  a can frame
 * @len: ignored
 *
 * write implemented on top of AUTOSAR functions.
 */
#ifdef TC1798
int write(int s, struct can_frame *cf, int len)
{
	/* ToDo: consider some use of PduIdType */
	Can_PduType pdu_info = {17, cf->can_dlc, cf->can_id, cf->data};
	Can_Write(CAN_IF, &pdu_info);

	/* ToDo: redo */
	uint64_t wait = read_time();
	while (wait + 1000 > read_time()) {};

	return 16;
}
#endif

/**
 * check_cmac() - checks a message authenticity
 * @skey:  128-bit session key
 * @cmac4: points to CMAC message part, i.e. 4 bytes CMAC
 * @plain: plain text to be CMACked and checked against
 * @len:   length of plain text in bytes
 *
 * The function computes CMAC of the given plain text and compares
 * it against cmac4. Returns 1 if CMACs matches.
 */
int check_cmac(struct macan_ctx *ctx, uint8_t *skey, const uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len)
#ifdef TC1798
{
	uint8_t cmac[16];
	uint64_t time;
	uint32_t *ftime = (uint32_t *)fill_time;
	int i;

	if (!fill_time) {
		aes_cmac(skey, len, cmac, plain);

		return memchk(cmac4, cmac, 4);
	}

	time = get_macan_time(ctx) / TIME_DIV;

	for (i = 0; i >= -1; i--) {
		*ftime = time + i;
		aes_cmac(skey, len, cmac, plain);

		if (memchk(cmac4, cmac, 4) == 1)
			return 1;
	}

	return 0;
}
#else
{
	struct aes_ctx cipher;
	uint8_t cmac[16];
	uint64_t time;
	uint32_t *ftime = (uint32_t *)fill_time;
	int i;

	aes_set_encrypt_key(&cipher, 16, skey);

	if (!fill_time) {
		aes_cmac(&cipher, len, cmac, plain);

		return memchk(cmac4, cmac, 4);
	}

	time = get_macan_time(ctx) / TIME_DIV;

	for (i = 0; i >= -1; i--) {
		*ftime = time + i;
		aes_cmac(&cipher, len, cmac, plain);

		if (memchk(cmac4, cmac, 4) == 1)
			return 1;
	}

	return 0;
}
#endif /* TC1798 */

/**
 * sign() - signs a message with CMAC
 * @skey:  128-bit session key
 * @cmac4: 4 bytes of the CMAC signature will be written to
 * @plain: a plain text to sign
 * @len:   length of the plain text
 */
void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len)
#ifdef TC1798
{
	uint8_t cmac[16];
	aes_cmac(skey, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}
#else
{
	struct aes_ctx cipher;
	uint8_t cmac[16];

	aes_set_encrypt_key(&cipher, 16, skey);
	aes_cmac(&cipher, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}
#endif /* TC1798 */

/**
 * unwrap_key() - deciphers AES-WRAPed key
 */
void unwrap_key(uint8_t *key, size_t len, uint8_t *dst, uint8_t *src)
#ifdef TC1798
{
	aes_set_key(key);
	aes_unwrap(key, len, dst, src, src);
}
#else
{
	struct aes_ctx cipher;

	aes_set_decrypt_key(&cipher, 16, key);
	aes_unwrap(&cipher, len, dst, src, src);
}
#endif /* TC1798 */


int macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, sig_cback fnc)
{
	ctx->sighand[sig_num]->cback = fnc;

	return 0;
}

void send_ack(struct macan_ctx *ctx, int s, uint8_t dst_id)
{
	struct macan_ack ack = {2, dst_id, {0}, {0}};
	uint8_t plain[8] = {0};
	uint32_t time;
	uint8_t *skey;
	struct can_frame cf;
	volatile int res;
	struct com_part **cpart;

	cpart = ctx->cpart;

	if (!is_skey_ready(ctx, dst_id))
		return;

	skey = cpart[dst_id]->skey;
	memcpy(&ack.group, &cpart[dst_id]->group_field, 3);
	time = get_macan_time(ctx) / TIME_DIV;

	memcpy(plain, &time, 4);
	plain[4] = ack.dst_id;
	memcpy(plain + 5, ack.group, 3);

#ifdef DEBUG_TS
	memcpy(ack.cmac, &time, 4);
#else
	sign(skey, ack.cmac, plain, sizeof(plain));
#endif

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(cf.data, &ack, 8);

	res = write(s, &cf, sizeof(struct can_frame));
	if (res != 16) {
		perror("send ack");
	}

	return;
}

/**
 * receive_ack()
 *
 * Returns 1 if the incoming ack does not contain this node in its
 * group field. Therefore, the updated ack should be broadcasted.
 */
int receive_ack(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint8_t id;
	struct com_part *cp;
	struct macan_ack *ack = (struct macan_ack *)cf->data;
	uint8_t plain[8];
	uint8_t *skey;
	struct com_part **cpart;

	cpart = ctx->cpart;

	id = cf->can_id;
	assert(id < NODE_COUNT);

	/* ToDo: overflow check */
	/* ToDo: what if ack contains me */
	/* ToDo: add groups communication */
	if (cpart[id] == NULL)
		return -1;

	cp = cpart[id];
	skey = cp->skey;

	plain[4] = ack->dst_id;
	memcpy(plain + 5, ack->group, 3);

#ifdef DEBUG_TS
#ifdef TC1798
	uint32_t cmac;
	memcpy_bw(&cmac, ack->cmac, 4);
	printf("time check: (local=%llu, in msg=%u)\n", get_macan_time(ctx) / TIME_DIV, cmac);
#else
	printf("time check: (local=%llu, in msg=%u)\n", get_macan_time(ctx) / TIME_DIV, *(uint32_t *)ack->cmac);
#endif
#endif

	/* ToDo: make difference between wrong CMAC and not having the key */
	if (!check_cmac(ctx, skey, ack->cmac, plain, plain, sizeof(plain))) {
		printf("error: ACK CMAC failed\n");
		return -1;
	}

	uint32_t is_present = 1 << NODE_ID;
	uint32_t ack_group = 0;
	memcpy(&ack_group, ack->group, 3);

	is_present &= ack_group;
	cp->group_field |= ack_group;

	if (is_present)
		return 0;

	return 1;
}

void gen_challenge(uint8_t *chal)
{
	int i;

	for (i = 0; i < 6; i++)
		chal[i] = rand();
}

/**
 * receive_skey() - session key reception
 *
 * Processes one frame of the session key transmission protocol.
 * Returns node id if a complete key was sucessfully received. Returns 0
 * if normal conditions. Returns -1 if key receive failed.
 */
int receive_skey(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_sess_key *sk;
	uint8_t fwd_id;
	static uint8_t keywrap[32];
	uint8_t skey[24];
	uint8_t seq, len;
	struct com_part **cpart;

	cpart = ctx->cpart;
	sk = (struct macan_sess_key *)cf->data;
	seq = sk->seq;
	len = sk->len;

	assert(0 <= seq && seq <= 5);
	assert((seq != 5 && len == 6) || (seq == 5 && len == 2));

	memcpy(keywrap + 6 * seq, sk->data, sk->len);

	if (seq == 5) {
		print_hexn(keywrap, 32);
		unwrap_key(ctx->ltk, 32, skey, keywrap);
		print_hexn(skey, 24);

		fwd_id = skey[6];
		if (fwd_id <= 0 || NODE_COUNT <= fwd_id || cpart[fwd_id] == NULL) {
			printf("receive session key \033[1;31mFAIL\033[0;0m: unexpected fwd_id\n");
			return -1;
		}

		if(!memchk(skey, cpart[fwd_id]->chg, 6)) {
			printf("receive session key \033[1;31mFAIL\033[0;0m: check cmac\n");
			return -1;
		}

		cpart[fwd_id]->valid_until = read_time() + SKEY_TIMEOUT;
		memcpy(cpart[fwd_id]->skey, skey + 8, 16);
		cpart[fwd_id]->group_field |= 1 << NODE_ID;
		printf("receive session key (%d->%"PRIu8") \033[1;32mOK\033[0;0m\n", NODE_ID, fwd_id);

		return fwd_id;
	}

	return 0;
}

/**
 * send_challenge() - requests key from KS or signed time from TS
 * @s:      socket fd
 * @dst_id: destination node id (e.g. KS)
 * @fwd_id: id of a node I wish to share a key with
 *          (i.e. communication partner)
 * @chg:    challenge (a 6-byte nonce) will be generated and written
 *          here
 *
 * This function sends the CHALLENGE message to the socket s. It is
 * used to request a key from KS or to request a signed time from TS.
 */
void send_challenge(int s, uint8_t dst_id, uint8_t fwd_id, uint8_t *chg)
{
	struct can_frame cf;
	struct macan_challenge chal = {1, dst_id, fwd_id, {0}};

	if (chg) {
		gen_challenge(chg);
		memcpy(chal.chg, chg, 6);
	}

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(cf.data, &chal, sizeof(struct macan_challenge));
	write(s, &cf, sizeof(struct can_frame));
}

/**
 * receive_challenge() - responds to REQ_CHALLENGE from KS
 */
void receive_challenge(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	uint8_t fwd_id;
	struct macan_challenge *ch = (struct macan_challenge *)cf->data;
	struct com_part **cpart;

	cpart = ctx->cpart;
	fwd_id = ch->fwd_id;
	assert(0 < fwd_id && fwd_id < NODE_COUNT);

	if (cpart[fwd_id] == NULL) {
		cpart[fwd_id] = malloc(sizeof(struct com_part));
		memset(cpart[fwd_id], 0, sizeof(struct com_part));

		cpart[fwd_id]->wait_for = 1 << fwd_id | 1 << NODE_ID;
	}

	cpart[fwd_id]->valid_until = read_time() + SKEY_CHG_TIMEOUT;
	send_challenge(s, NODE_KS, ch->fwd_id, cpart[ch->fwd_id]->chg);
}

void receive_time(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	uint32_t time_ts;
	uint64_t recent;

	if (!is_skey_ready(ctx, NODE_TS))
		return;

	memcpy(&time_ts, cf->data, 4);
	recent = read_time() + ctx->time.offs;

	if (ctx->time.chal_ts) {
		if ((recent - ctx->time.chal_ts) < TIME_TIMEOUT)
			return;
	}

	printf("time received = %u\n", time_ts);

	if (abs(recent - time_ts) > TIME_DELTA) {
		printf("error: time out of sync (%"PRIu64" = %"PRIu64" - %"PRIu32")\n", (uint64_t)abs(recent - time_ts), recent, time_ts);

		ctx->time.chal_ts = recent;
		send_challenge(s, NODE_TS, 0, ctx->time.chg);
	}
}

void receive_signed_time(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	uint32_t time_ts;
	uint8_t plain[10];
	uint8_t *skey;
	struct com_part **cpart;

	cpart = ctx->cpart;

	memcpy(&time_ts, cf->data, 4);
	printf("signed time received = %u\n", time_ts);

	if (!is_skey_ready(ctx, NODE_TS)) {
		ctx->time.chal_ts = 0;
		return;
	}

	skey = cpart[NODE_TS]->skey;

	memcpy(plain, ctx->time.chg, 6);
	memcpy(plain + 6, &time_ts, 4);
	if (!check_cmac(ctx, skey, cf->data + 4, plain, NULL, sizeof(plain))) {
		printf("CMAC \033[1;31merror\033[0;0m\n");
		return;
	}
	printf("CMAC OK\n");

	ctx->time.offs += (time_ts - ctx->time.chal_ts);
	ctx->time.chal_ts = 0;
}

/**
 * send_auth_req() - send authorization request
 */
void send_auth_req(struct macan_ctx *ctx, int s, uint8_t dst_id, uint8_t sig_num, uint8_t prescaler)
{
	uint64_t t;
	uint8_t plain[8];
	uint8_t *skey;
	struct can_frame cf;
	struct macan_sig_auth_req areq;
	struct com_part **cpart;

	cpart = ctx->cpart;
	t = get_macan_time(ctx) / TIME_DIV;
	skey = cpart[dst_id]->skey;

	memcpy(plain, &t, 4);
	plain[4] = NODE_ID;
	plain[5] = dst_id;
	plain[6] = sig_num;
	plain[7] = prescaler;

	areq.flags = 3;
	areq.dst_id = dst_id;
	areq.sig_num = sig_num;
	areq.prescaler = prescaler;
	sign(skey, areq.cmac, plain, sizeof(plain));

	cf.can_id = NODE_ID;
	cf.can_dlc = 7;
	memcpy(&cf.data, &areq, 7);

	write(s, &cf, sizeof(cf));
}

void receive_auth_req(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint8_t *skey;
	uint8_t plain[8];
	uint8_t sig_num;
	struct macan_sig_auth_req *areq;
	struct com_part **cpart;
	struct sig_handle **sighand;

	cpart = ctx->cpart;
	sighand = ctx->sighand;

	if (cpart[cf->can_id] == NULL)
		return;

	areq = (struct macan_sig_auth_req *)cf->data;
	skey = cpart[cf->can_id]->skey;

	plain[4] = cf->can_id;
	plain[5] = NODE_ID;
	plain[6] = areq->sig_num;
	plain[7] = areq->prescaler;

	if (!check_cmac(ctx, skey, areq->cmac, plain, plain, sizeof(plain))) {
		printf("error: sig_auth cmac is incorrect\n");
	}

	sig_num = areq->sig_num;

	/* ToDo: assert sig_num range */
	sighand[sig_num]->presc = areq->prescaler;
	sighand[sig_num]->presc_cnt = areq->prescaler - 1;
}

int macan_write(struct macan_ctx *ctx, int s, uint8_t dst_id, uint8_t sig_num, uint32_t signal)
{
	struct can_frame cf;
	uint8_t plain[8];
	uint32_t t;
	uint8_t *skey;
	struct com_part **cpart;
	struct macan_signal_ex sig = {
		3, dst_id, sig_num, {0}, {0}
	};

	cpart = ctx->cpart;

	if (!is_channel_ready(ctx, dst_id))
		return -1;

	skey = cpart[dst_id]->skey;
	t = get_macan_time(ctx) / TIME_DIV;

	memcpy(plain, &t, 4);
	plain[4] = NODE_ID;
	plain[5] = dst_id;
	memcpy(plain + 6, &signal, 2);

	memcpy(&sig.signal, &signal, 2);
#ifdef DEBUG_TS
	memcpy(sig.cmac, &time, 4);
#else
	sign(skey, sig.cmac, plain, sizeof(plain));
#endif

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(&cf.data, &sig, 8);

	/* ToDo: assure success */
	write(s, &cf, sizeof(cf));

	return 0;
}

/* ToDo: return result */
void macan_send_sig(struct macan_ctx *ctx, int s, uint8_t sig_num, const struct macan_sig_spec *sig_spec, uint16_t signal)
{
	uint8_t dst_id;
	struct sig_handle **sighand;

	sighand = ctx->sighand;

	dst_id = sig_spec[sig_num].dst_id;
	if (!is_channel_ready(ctx, dst_id))
		return;

	switch (sighand[sig_num]->presc) {
	case SIG_DONTSIGN:
		break;
	case SIG_SIGNONCE:
		macan_write(ctx, s, dst_id, sig_num, signal);
		sighand[sig_num]->presc = SIG_DONTSIGN;
		break;
	default:
		if (sighand[sig_num]->presc_cnt > 0) {
			sighand[sig_num]->presc_cnt--;
		} else {
			macan_write(ctx, s, dst_id, sig_num, signal);
			sighand[sig_num]->presc_cnt = sighand[sig_num]->presc - 1;
		}
		break;
	}
}

/* ToDo: receive signal frame with 32bits */
void receive_sig(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_signal_ex *sig;
	uint8_t plain[8];
	uint8_t sig_num;
	uint32_t sig_val = 0;
	uint8_t *skey;
	struct com_part **cpart;
	struct sig_handle **sighand;

	cpart = ctx->cpart;
	sighand = ctx->sighand;
	sig = (struct macan_signal_ex *)cf->data;

	plain[4] = cf->can_id;
	plain[5] = NODE_ID;
	memcpy(plain + 6, sig->signal, 2);

	skey = cpart[cf->can_id]->skey;

#ifdef DEBUG_TS
	printf("receive_sig: (local=%d, in msg=%d)\n", get_macan_time(ctx) / TIME_DIV, *(uint32_t *)sig->cmac);
#endif
	if (!check_cmac(ctx, skey, sig->cmac, plain, plain, sizeof(plain))) {
		printf("signal CMAC \033[1;31merror\033[0;0m");
		return;
	}

	sig_num = sig->sig_num;
	memcpy(&sig_val, sig->signal, 2);

	if (sighand[sig_num]->cback)
		sighand[sig_num]->cback(sig_num, sig_val);
}

int is_skey_ready(struct macan_ctx *ctx, uint8_t dst_id)
{
	if (ctx->cpart[dst_id] == NULL)
		return 0;

	return (ctx->cpart[dst_id]->group_field & 1 << NODE_ID);
}

int is_channel_ready(struct macan_ctx *ctx, uint8_t dst)
{
	struct com_part **cpart;

	cpart = ctx->cpart;

	if (cpart[dst] == NULL)
		return 0;

	uint32_t grp = (*((uint32_t *)&cpart[dst]->group_field)) & 0x00ffffff;
	uint32_t wf = (*((uint32_t *)&cpart[dst]->wait_for)) & 0x00ffffff;

	return ((grp & wf) == wf);
}

#define f_STM 100000000
#define TIME_USEC (f_STM / 1000000)

uint64_t read_time()
#ifdef TC1798
{
	uint64_t time;
	uint32_t *time32 = (uint32_t *)&time;
	time32[0] = STM_TIM0.U;
	time32[1] = STM_TIM6.U;
	time /= TIME_USEC;

	return time;
}
#else
{
	uint64_t time;
	struct timespec ts;
	static struct timespec buz;
	static uint8_t once = 0;

	/* ToDo: redo this */
	if (once == 0) {
		once = 1;
		clock_gettime(CLOCK_MONOTONIC_RAW, &buz);
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	ts.tv_sec -= buz.tv_sec;
	ts.tv_nsec -= buz.tv_nsec;
	time = ts.tv_sec * 1000000;
	time += ts.tv_nsec / 1000;

	return time;
}
#endif /* TC1798 */

/**
 * Request keys to all comunication partners.
 */
void macan_request_keys(struct macan_ctx *ctx, int s)
{
	int i;
	struct com_part **cpart;

	cpart = ctx->cpart;

	for (i = 0; i < NODE_COUNT; i++) {
		if (cpart[i] == NULL)
			continue;

		if (cpart[i]->valid_until > read_time())
			continue;

		send_challenge(s, NODE_KS, i, cpart[i]->chg);
		cpart[i]->valid_until = read_time() + SKEY_CHG_TIMEOUT;
	}

	return;
}

uint64_t get_macan_time(struct macan_ctx *ctx)
{
	return read_time() + ctx->time.offs;
}

int macan_process_frame(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;
	int fwd;

	/* ToDo: make sure all branches end ASAP */
	/* ToDo: macan or plain can */
	/* ToDo: crypto frame or else */
	if(cf->can_id == NODE_ID)
		return 1;
	if (cf->can_id == SIG_TIME) {
		switch(cf->can_dlc) {
		case 4:
			receive_time(ctx, s, cf);
			return 1;
		case 8:
			receive_signed_time(ctx, s, cf);
			return 1;
		}
	}

	if (cryf->dst_id != NODE_ID)
		return 1;

	switch (cryf->flags) {
	case 1:
		receive_challenge(ctx, s, cf);
		break;
	case 2:
		if (cf->can_id == NODE_KS) {
			fwd = receive_skey(ctx, cf);
			if (fwd > 1) {
				send_ack(ctx, s, fwd);
			}
			break;
		}

		/* ToDo: what if ack CMAC fails, there should be no response */
		if (receive_ack(ctx, cf) == 1)
			send_ack(ctx, s, cf->can_id);
		break;
	case 3:
		if (cf->can_dlc == 7)
			receive_auth_req(ctx, cf);
		else
			receive_sig(ctx, cf);
		break;
	}

	return 0;
}
