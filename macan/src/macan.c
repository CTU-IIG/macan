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
#include <macan.h>
#include "macan_private.h"
#include "debug.h"
#include "cryptlib.h"
#include "endian.h"

static inline struct com_part *get_cpart(struct macan_ctx *ctx, macan_ecuid i)
{
	if (i < ctx->config->node_count) {
		return ctx->cpart[i];
	} else {
		return NULL;
	}
}

/**
 * Register a callback function.
 *
 * The callback should serve signal reception.
 *
 * @param sig_num  signal id number
 * @param fnc      pointer to the signal callback function
 */
int macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, macan_sig_cback fnc, macan_sig_cback invalid_cmac)
{
	ctx->sighand[sig_num]->cback = fnc;
	ctx->sighand[sig_num]->invalid_cback = invalid_cmac;

	return 0;
}

/**
 * Check we have authenticated channel with dst.
 *
 * Checks if has the session key and if the communication partner
 * has acknowledged the communication with an ACK message.
 */
static
bool is_channel_ready(struct macan_ctx *ctx, macan_ecuid dst)
{
#ifdef VW_COMPATIBLE
	/* VW compatible -> ACK is disabled, channel is ready */
	return true;
#endif

	struct com_part *cp = get_cpart(ctx, dst);
	if (cp == NULL)
		return false;

	uint32_t both = 1U << dst | 1U << ctx->config->node_id;
	return (cp->group_field & both) == both;
}

static
void send_auth_req(struct macan_ctx *ctx, macan_ecuid dst_id, uint8_t sig_num, uint8_t prescaler)
{
	uint64_t t;
	uint8_t plain[8];
	struct macan_key skey;
	struct can_frame cf = {0};
	struct macan_sig_auth_req areq;

	t = macan_get_time(ctx);
	skey = get_cpart(ctx, dst_id)->skey;

	memcpy(plain, &htole32(t), 4);
	plain[4] = ctx->config->node_id;
	plain[5] = dst_id;
	plain[6] = sig_num;
	plain[7] = prescaler;

	areq.flags_and_dst_id = FL_AUTH_REQ << 6 | dst_id;
	areq.sig_num = sig_num;
	areq.prescaler = prescaler;
	macan_sign(&skey, areq.cmac, plain, sizeof(plain));

	cf.can_id = CANID(ctx, ctx->config->node_id);
	cf.can_dlc = 7;
	memcpy(&cf.data, &areq, 7);

	macan_send(ctx, &cf);
}

static
void request_signals(struct macan_ctx *ctx)
{
	uint8_t i;

	if (!ctx->time.ready)
		return;

	/* ToDo: repeat auth_req for the case the signal source will restart */
	for (i = 0; i < ctx->config->sig_count; i++) {
		const struct macan_sig_spec *sigspec = &ctx->config->sigspec[i];
		struct sig_handle *sighand = ctx->sighand[i];

		if (sigspec->dst_id != ctx->config->node_id ||
		    !is_channel_ready(ctx, sigspec->src_id))
			continue;

		if (!(sighand->flags & AUTHREQ_SENT)) {
			sighand->flags |= AUTHREQ_SENT;
			print_msg(ctx, MSG_REQUEST,"Sending req auth for signal #%d\n",i);
			send_auth_req(ctx, sigspec->src_id, i, sigspec->presc);
		}
	}
}

/**
 * Sends an ACK message.
 */
static void send_ack(struct macan_ctx *ctx, macan_ecuid dst_id)
{
#ifdef VW_COMPATIBLE
	/* VW compatible -> ACK is disabled, don't do anything */
	return;
#endif

	struct macan_ack ack = { .flags_and_dst_id = (uint8_t)(FL_ACK << 6 | (dst_id & 0x3f)), .group = {0}, .cmac = {0}};
	uint8_t plain[8] = {0};
	struct can_frame cf = {0};
	struct com_part *cpart = get_cpart(ctx, dst_id);

	if (!cpart ||
	    !is_skey_ready(ctx, dst_id) ||
	    !ctx->time.ready)
		return;

	uint32_t group_field = htole32(cpart->group_field);
	memcpy(&ack.group, &group_field, 3);
	uint32_t time = htole32((uint32_t)macan_get_time(ctx));

	memcpy(plain, &time, 4);
	plain[4] = dst_id;
	memcpy(plain + 5, &group_field, 3);

#ifdef DEBUG_TS
	memcpy(ack.cmac, &time, 4);
#else
	macan_sign(&cpart->skey, ack.cmac, plain, sizeof(plain));
#endif
	cf.can_id = CANID(ctx, ctx->config->node_id);
	cf.can_dlc = 8;
	memcpy(cf.data, &ack, 8);

	if (!macan_send(ctx, &cf)) {
		fail_printf(ctx, "%s\n","failed to send some bytes of ack");
	}

	return;
}

static void receive_ack(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct com_part *cp = canid2cpart(ctx, cf->can_id);
	struct macan_ack *ack = (struct macan_ack *)cf->data;
	uint8_t plain[8];

	if (!cp)
		return;

	plain[4] = ack->flags_and_dst_id & 0x3f;
	memcpy(plain + 5, ack->group, 3);

	if (!macan_check_cmac(ctx, &cp->skey, ack->cmac, plain, plain, sizeof(plain))) {
		if (is_skey_ready(ctx, cp->ecu_id))
			fail_printf(ctx, "%s\n","error: ACK CMAC failed");
		return;
	}

	uint32_t ack_group = 0;
	memcpy(&ack_group, ack->group, 3);
	ack_group = le32toh(ack_group);

	cp->group_field |= ack_group;

	if ((ack_group & (1U << ctx->config->node_id)) == 0)
		send_ack(ctx, cp->ecu_id);

	request_signals(ctx);
}

static
void gen_challenge(struct macan_ctx *ctx, uint8_t *chal)
{
	if(!gen_rand_data(chal, 6)) {
		print_msg(ctx, MSG_FAIL,"Failed to read enough random bytes.\n");
		exit(1);
	}
}

static
void macan_send_challenge(struct macan_ctx *ctx, macan_ecuid dst_id, macan_ecuid fwd_id, uint8_t *chg)
{
	struct can_frame cf = {0};
	struct macan_challenge chal = { .flags_and_dst_id = (uint8_t)((FL_CHALLENGE << 6) | (dst_id & 0x3F)), .fwd_id = fwd_id };

	if (chg) {
		memcpy(chal.chg, chg, 6);
		cf.can_dlc = 8;
	} else {
		cf.can_dlc = 2;
	}

	cf.can_id = CANID(ctx, ctx->config->node_id);
	memcpy(cf.data, &chal, sizeof(struct macan_challenge));

	macan_send(ctx, &cf);
}

static void request_time_auth(struct macan_ctx *ctx)
{
	macan_ecuid ts_id = ctx->config->time_server_id;

	if (ctx->config->node_id == ts_id ||
	    !is_skey_ready(ctx, ts_id) ||
	    ctx->time.nonauth_loc == 0)
		return;

	/* Don't ask TS for authenticated time too often */
	if (read_time() - ctx->time.chal_ts > ctx->config->time_timeout ||
	    ctx->time.chal_ts == 0) {
		print_msg(ctx, MSG_REQUEST,"Requesting time authentication\n");
		ctx->time.chal_ts = read_time();
		gen_challenge(ctx, ctx->time.chg);
		macan_send_challenge(ctx, ts_id, 0, ctx->time.chg);
	}
}

/**
 * Receive a session key.
 *
 * Processes one frame of the session key transmission protocol.
 * Returns node id if a complete key was sucessfully received.
 *
 * @return RECEIVE_SKEY_ERR (-1) if key receive failed OR
 *         ID of the node the key is shared with OR
 *         RECEIVE_SKEY_IN_PROGRESS (-2) if the reception process is in progress.
 */
static bool receive_skey(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_sess_key *sk;
	uint8_t unwrapped[24];
	uint8_t seq, len;

	if (cf->can_dlc < 2)
		return RECEIVE_SKEY_ERR;

	sk = (struct macan_sess_key *)cf->data;

	/* it reads wrong values, dirty hack to read directly from CAN frame 
	 * bytes are in reverse order! */
	seq = (cf->data[1] & 0xF0) >> 4;
	len = (cf->data[1] & 0x0F);

	if (cf->can_dlc < 2 + len || seq > 5)
		return RECEIVE_SKEY_ERR;

	/* this is because of VW macan sends len 6 in last key packet */
	if(seq == 5) len = 2;

	if ((seq <  5 && len != 6) ||
	    (seq == 5 && len != 2) ||
	    (cf->can_dlc < 2 + len))
		return RECEIVE_SKEY_ERR;

	memcpy(ctx->keywrap + 6 * seq, sk->data, len);

	if (seq == 5) {
		macan_unwrap_key(ctx->config->ltk, sizeof(ctx->keywrap), unwrapped, ctx->keywrap);
		macan_ecuid fwd_id = unwrapped[17];
		struct com_part *cpart = get_cpart(ctx, fwd_id);

		if (cpart == NULL) {
			fail_printf(ctx, "unexpected fwd_id %#x\n", fwd_id);
			return RECEIVE_SKEY_ERR;
		}

		if (!memchk(unwrapped + 18, cpart->chg, 6)) {
			fail_printf(ctx, "wrong challenge for %d\n", fwd_id);
			return RECEIVE_SKEY_ERR;
		}

		cpart->awaiting_skey = false;
		cpart->valid_until = read_time() + ctx->config->skey_validity;

		if (memcmp(cpart->skey.data, unwrapped, 16) != 0) {
			/* Session key has changed */
			memcpy(cpart->skey.data, unwrapped, 16);

			// initialize group field - this will work only for ecu_id <= 23
			cpart->group_field = 1U << ctx->config->node_id;

			send_ack(ctx, fwd_id);
		}

		if (!ctx->time.ready && fwd_id == ctx->config->time_server_id)
			request_time_auth(ctx);

		if (cpart->skey_callback)
			cpart->skey_callback(ctx, fwd_id);

		return fwd_id;
	}

	return RECEIVE_SKEY_IN_PROGRESS;
}

/**
 * Receive unsigned time.
 *
 * Receives time and checks whether local clock is in sync. If local clock
 * is out of sync, it sends a request for signed time.
 */
static
void receive_time_nonauth(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_timekeeping *t = &ctx->time;
	uint32_t time_ts;
	uint64_t loc_us;	/* Local time in microseconds */
	uint64_t ts_us;		/* Time server time in microseconds */
	uint64_t delta;
	uint64_t now = read_time();

	memcpy(&time_ts, cf->data, 4);
	time_ts = le32toh(time_ts);
	ts_us = (uint64_t)time_ts * ctx->config->time_div;
	loc_us = now + t->offs; /* Estimated time on TS (us) */
	delta = (loc_us > ts_us) ? loc_us - ts_us : ts_us - loc_us;

	if (delta > ctx->config->time_delta || !t->ready) {
		if (t->ready)
			print_msg(ctx, MSG_WARN, "time out of sync by %llu us  (local:%"PRIu64", TS:%"PRIu64")\n",
				  delta, loc_us, ts_us);
		request_time_auth(ctx);
	}

	/* Store data that is needed in authenticated time handler */
	t->nonauth_ts = time_ts;
	t->nonauth_loc = now;
}

static void send_acks(struct macan_ctx *ctx)
{
        macan_ecuid i;
	for (i = 0; i < ctx->config->node_count; i++)
		if (is_skey_ready(ctx, i))
			send_ack(ctx, i);
}

/**
 * Receive signed time.
 *
 * Receives time and sets local clock according to it.
 */
static
void receive_time_auth(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_timekeeping *t = &ctx->time;
	uint32_t time_ts;
	uint8_t plain[12];
	struct macan_key skey;
	uint64_t time_ts_us;

	if (!is_skey_ready(ctx, ctx->config->time_server_id))
		return;

	memcpy(&time_ts, cf->data, 4);

	skey = ctx->cpart[ctx->config->time_server_id]->skey;
	memcpy(plain, &time_ts, 4); // received time
	memcpy(plain + 4, t->chg, 6); // challenge
	memcpy(plain + 10, &htole32(CANID(ctx, ctx->config->time_server_id)),2);

	if (!macan_check_cmac(ctx, &skey, cf->data + 4, plain, NULL, sizeof(plain))) {
		/* not a fatal error, we possibly received signed time for different node */
		return;
	}

	/* cmac check ok, convert to our endianess */
	time_ts = le32toh(time_ts);
	time_ts_us = time_ts * ctx->config->time_div;

	if (time_ts == t->nonauth_ts) {
		t->offs = (time_ts_us - t->nonauth_loc);
	} else {
		t->offs = (time_ts_us - read_time());
		print_msg(ctx, MSG_FAIL, "auth. time %u differ from non-auth. time %u\n",
			  time_ts, t->nonauth_ts);
	}
	t->ready = true;

	print_msg(ctx, MSG_OK,"signed time = %d, offs %"PRIu64"\n",time_ts, t->offs);

	/* Now, when time is synchronized, we can send acks for keys
	 * received so far and request signals from nodes we have
	 * session keys for. */
	send_acks(ctx);
	request_signals(ctx);
}

static
void receive_auth_req(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint8_t sig_num;
	int can_sid,can_nsid;
	struct macan_sig_auth_req *areq;
	struct com_part *cp;
	struct sig_handle **sighand;

	if(!(cp = canid2cpart(ctx, cf->can_id)))
		return;

	sighand = ctx->sighand;

	areq = (struct macan_sig_auth_req *)cf->data;

	if (areq->sig_num >= ctx->config->sig_count)
		return;

	/* Do not check CMAC when compatible with VW (it does not use CMAC in it's sig requests */
#ifndef VW_COMPATIBLE
	uint8_t plain[8];
	struct macan_key skey;

	skey = cp->skey;
	plain[4] = (macan_ecuid)cp->ecu_id;
	plain[5] = ctx->config->node_id;
	plain[6] = areq->sig_num;
	plain[7] = areq->prescaler;

	if (!macan_check_cmac(ctx, &skey, areq->cmac, plain, plain, sizeof(plain))) {
		printf("error: sig_auth cmac is incorrect\n");
		return;
	}
#endif

	print_msg(ctx, MSG_INFO,"Received auth request for signal #%d\n",areq->sig_num);
#ifdef DEBUG
	printf(ANSI_COLOR_CYAN "RECV Auth Req\n" ANSI_COLOR_RESET);
	printf("sig_num: 0x%X\n",areq->sig_num);
#endif
	sig_num = areq->sig_num;
    
	can_sid = ctx->config->sigspec[sig_num].can_sid;
	can_nsid = ctx->config->sigspec[sig_num].can_nsid;
    
	if((can_nsid == 0 && can_sid == 0) ||
	   (can_nsid == 0 && can_sid != 0)) {
		// ignore prescaler
		sighand[sig_num]->presc = 1;
		sighand[sig_num]->presc_cnt = 0;
	} else {
		sighand[sig_num]->presc = areq->prescaler;
		sighand[sig_num]->presc_cnt = (uint8_t)(areq->prescaler - 1);
	}

}

/**
 * Send MaCAN signal frame.
 *
 * Signs signal using CMAC and transmits it.
 */
static
int __macan_send_sig(struct macan_ctx *ctx, macan_ecuid dst_id, uint8_t sig_num, uint32_t sig_val)
{
	struct can_frame cf = {0};
	uint8_t plain[10],sig[8];
	uint8_t plain_length;
	uint32_t t;
	struct macan_key skey;
	uint8_t *cmac;

	if (!is_channel_ready(ctx, dst_id))
		return -1;

	skey = get_cpart(ctx, dst_id)->skey;
	t = (uint32_t)macan_get_time(ctx);
	

	if(is_32bit_signal(ctx,sig_num)) {
		/* changed to be compatible with VW macangw */
		memcpy(plain+4, &htole32(t), 4);
		struct macan_signal *sig32 = (struct macan_signal *) sig;
		// changed to be compatible with macangw
		memcpy(plain, &htole32(sig_val), 4);
		memcpy(plain+8, &ctx->config->sigspec[sig_num].can_sid,2); 
		memcpy(sig32->sig, &htole32(sig_val), 4);
		cf.can_id = ctx->config->sigspec[sig_num].can_sid;
		plain_length = 10;
		cmac = sig32->cmac;
	} else {
		struct macan_signal_ex *sig16 = (struct macan_signal_ex *) sig;

		sig16->flags_and_dst_id = FL_SIGNAL << 6 | (dst_id & 0x3f);
		sig16->sig_num = sig_num;
		memcpy(&sig16->sig_val, &sig_val, 2);

		memcpy(plain, &htole32(t), 4);
		memcpy(plain + 4, &(ctx->config->node_id), 1);
		memcpy(plain + 5, &dst_id, 1);
		memcpy(plain + 6, &htole32(sig_val), 2);
		plain_length = 8;

		cf.can_id = CANID(ctx, ctx->config->node_id);
		cmac = sig16->cmac;
	}


#ifdef DEBUG_TS
	memcpy(cmac, &time, 4);
#else
	macan_sign(&skey, cmac, plain, plain_length);
#endif

	cf.can_dlc = 8;
	memcpy(&cf.data, &sig, 8);

	/* ToDo: assure success */
	macan_send(ctx, &cf);

	return 0;
}

/**
 * Dispatch a signal.
 *
 * The prescaler settings are considered and the signal is send.
 *
 * @param sig_num  signal id
 * @param sig_val   signal value
 */
/* ToDo: return result */
void macan_send_sig(struct macan_ctx *ctx, uint8_t sig_num, uint32_t sig_val)
{
	macan_ecuid dst_id;
	struct sig_handle **sighand;
	const struct macan_sig_spec *sigspec;

	sighand = ctx->sighand;
	sigspec = ctx->config->sigspec;

	dst_id = sigspec[sig_num].dst_id;
	if (!is_channel_ready(ctx, dst_id)) {
		//printf("Channel not ready\n"); /* FIXME: return error instead of printing this */
		return;
	}

	switch (sighand[sig_num]->presc) {
	case SIG_DONTSIGN:
		break;
	case SIG_SIGNONCE:
		__macan_send_sig(ctx, dst_id, sig_num, sig_val);
		sighand[sig_num]->presc = SIG_DONTSIGN;
		break;
	default:
		if (sighand[sig_num]->presc_cnt > 0) {
			sighand[sig_num]->presc_cnt--;
		} else {
			__macan_send_sig(ctx, dst_id, sig_num, sig_val);
			sighand[sig_num]->presc_cnt = (uint8_t)(sighand[sig_num]->presc - 1);
		}
		break;
	}
}

static void __receive_sig(struct macan_ctx *ctx, uint32_t sig_num, uint32_t sig_val, uint8_t *cmac,
			  uint8_t plain[10], uint8_t *fill_time, uint8_t plain_length);

/**
 * Receive signal.
 *
 * Receives signal, checks its CMAC and calls appropriate callback.
 */
/* ToDo: receive signal frame with 32bits */
static
void receive_sig32(struct macan_ctx *ctx, const struct can_frame *cf, uint32_t sig_num)
{
	uint8_t plain[10];
	uint8_t *fill_time;
	uint32_t sig_val = 0;
	uint8_t *cmac;
	uint8_t plain_length;

	// we have received 32 bit signal
	struct macan_signal *sig32 = (struct macan_signal *)cf->data;

	if (sig_num >= ctx->config->sig_count)
		return;

	if (ctx->config->sigspec->dst_id != ctx->config->node_id)
		return; /* Ignore signals for other nodes. We don't
			 * have a session key to check its CMAC. */

	cmac = sig32->cmac;
	memcpy(&sig_val, sig32->sig, 4);
	sig_val = le32toh(sig_val);

	/* Prepare plain text for CMAC */
	plain_length = 10;
	memcpy(plain,&sig32->sig,4);
	fill_time = plain+4;
	memcpy(plain + 8,&(cf->can_id),2);


	__receive_sig(ctx, sig_num, sig_val, cmac, plain, fill_time, plain_length);
}

static
void receive_sig16(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint8_t plain[10];
	uint8_t *fill_time;
	uint32_t sig_num;
	uint32_t sig_val = 0;
	uint8_t *cmac;
	uint8_t plain_length;
	const struct macan_sig_spec *sigspec;

	struct macan_signal_ex *sig16 = (struct macan_signal_ex *)cf->data;
	sig_num = sig16->sig_num;

	if (sig_num >= ctx->config->sig_count)
		return;

	sigspec = &ctx->config->sigspec[sig_num];

	if (sigspec->dst_id != ctx->config->node_id)
		return; /* Ignore signals for other nodes. We don't
			 * have a session key to check its CMAC. */

	cmac = sig16->cmac;
	memcpy(&sig_val, sig16->sig_val, 2);
	sig_val = le32toh(sig_val);

	/* Prepare plain text for CMAC */
	plain_length = 8;
	fill_time = plain;
	memcpy(plain + 4, &(sigspec->src_id), 1);
	memcpy(plain + 5, &(ctx->config->node_id), 1);
	memcpy(plain + 6, sig16->sig_val, 2);

	__receive_sig(ctx, sig_num, sig_val, cmac, plain, fill_time, plain_length);
}

static void __receive_sig(struct macan_ctx *ctx, uint32_t sig_num, uint32_t sig_val, uint8_t *cmac,
			  uint8_t plain[10], uint8_t *fill_time, uint8_t plain_length)
{
	struct macan_key skey;
	struct com_part *cp;
	const struct macan_sig_spec *sigspec = &ctx->config->sigspec[sig_num];
	struct sig_handle *sighand;

	sighand = ctx->sighand[sig_num];

	if (!is_skey_ready(ctx, sigspec->src_id)) {
		fail_printf(ctx, "No key to check signal #%d from %d\n", sig_num, sigspec->src_id);
		return;
	}

	cp = get_cpart(ctx, sigspec->src_id);
	if (!cp)
		return;
	skey = cp->skey;

	if (!macan_check_cmac(ctx, &skey, cmac, plain, fill_time, plain_length)) {
		if (sighand && sighand->invalid_cback)
			sighand->invalid_cback((uint8_t)sig_num, (uint32_t)sig_val);
		else
			fail_printf(ctx, "CMAC error for signal #%d\n", sig_num);
		return;
	}

	print_msg(ctx, MSG_SIGNAL,"Received signal #%d, value: %d\n", sig_num, sig_val);

	if (sighand && sighand->cback)
		sighand->cback((uint8_t)sig_num, sig_val);
}

/**
 * Check if we have a the session key for communication with dst_id
 *
 * @param dst_id  id of a node to whom check if has the key
 * @return        True if has the key, false otherwise
 */
bool is_skey_ready(struct macan_ctx *ctx, macan_ecuid dst_id)
{
	struct com_part *cpart = get_cpart(ctx, dst_id);

	if (cpart == NULL)
		return false;

	return !!(cpart->group_field & (1U << ctx->config->node_id));
}

void macan_request_key(struct macan_ctx *ctx, macan_ecuid fwd_id)
{
	struct com_part *cpart = ctx->cpart[fwd_id];

	if (cpart && !cpart->awaiting_skey) {
		print_msg(ctx, MSG_REQUEST,"Requesting skey for node #%d\n",fwd_id);
		gen_challenge(ctx, cpart->chg);
		macan_send_challenge(ctx, ctx->config->key_server_id, fwd_id, cpart->chg);
		cpart->awaiting_skey = true;

		/* Timeout for receiving a new session key */
		cpart->valid_until = read_time() + ctx->config->skey_chg_timeout;
	}

	return;
}

void macan_request_expired_keys(struct macan_ctx *ctx)
{
	uint8_t i;
	struct com_part **cpart = ctx->cpart;

	for (i = 0; i < ctx->config->node_count; i++)
		if (cpart[i] && cpart[i]->valid_until < read_time())
			macan_request_key(ctx, i);
}

/**
 * Get time of the MaCAN protocol.
 */
uint64_t macan_get_time(struct macan_ctx *ctx)
{
	return (read_time() + (uint64_t)ctx->time.offs) / ctx->config->time_div;
}

/*
 * Get signal number of 32bit signal from it's secure CAN-ID.
 *
 * @param[in]  ctx     Macan context.
 * @param[in]  can_id  secure CAN-ID of signal
 * @param[out] sig_num Pointer where signal number will be written
 * @return true if singal number was found, false otherwise.
 */
static
bool cansid2signum(struct macan_ctx *ctx, uint32_t can_id, uint32_t *sig_num)
{
	uint32_t i;

	for(i = 0; i < ctx->config->sig_count; i++) {
		if(ctx->config->sigspec[i].can_sid == can_id) {
			if(sig_num != NULL) {
				*sig_num = i;
			}
			return SUCCESS;
		}
	}
	return ERROR;
}

/**
 * Process can frame.
 *
 * This function should be called for incoming can frames. It extracts MaCAN messages
 * and operates the MaCAN library.
 *
 * @param *ctx pointer to MaCAN context
 * @param s socket file descriptor 
 * @param *cf pointer to can frame with received contents
 *
 * @returns One when the frame was a MaCAN frame, zero otherwise.
 */
enum macan_process_status macan_process_frame(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint32_t sig32_num;
	macan_ecuid src;

	if(cf->can_id == CANID(ctx, ctx->config->node_id))
		return MACAN_FRAME_PROCESSED; /* Frame sent by us */

	if (cf->can_id == ctx->config->canid->time) {
		switch(cf->can_dlc) {
		case 4:
			receive_time_nonauth(ctx, cf);
			return MACAN_FRAME_PROCESSED;
		case 8:
			receive_time_auth(ctx, cf);
			return MACAN_FRAME_PROCESSED;
		default:
			return MACAN_FRAME_PROCESSED;
		}
	}

	if (cf->can_dlc < 1)	/* MaCAN frames have at least 1 byte */
		return MACAN_FRAME_UNKNOWN;

	if(cansid2signum(ctx, cf->can_id,&sig32_num)) {
		receive_sig32(ctx, cf, sig32_num);
		return MACAN_FRAME_PROCESSED;
	}

	if (macan_canid2ecuid(ctx->config, cf->can_id, &src) == ERROR)
		return MACAN_FRAME_UNKNOWN;

	if (macan_crypt_dst(cf) != ctx->config->node_id)
		return MACAN_FRAME_PROCESSED;

	switch (macan_crypt_flags(cf)) {
	case FL_CHALLENGE:
		/* Only key and time servers need to handle this. */
		return MACAN_FRAME_CHALLENGE;
	case FL_REQ_CHALLENGE:
		if (cf->can_dlc == 2 && src == ctx->config->key_server_id) {
			/* REQ_CHALLENGE from KS */
			macan_ecuid fwd_id = cf->data[1];

			if (fwd_id >= ctx->config->node_count)
				return MACAN_FRAME_UNKNOWN;

			macan_request_key(ctx, fwd_id);
			return MACAN_FRAME_PROCESSED;
		}
		return MACAN_FRAME_UNKNOWN;
	case FL_SESS_KEY_OR_ACK:
		if (src == ctx->config->key_server_id)
			receive_skey(ctx, cf);
		else
			receive_ack(ctx, cf);
		return MACAN_FRAME_PROCESSED;
	case FL_SIGNAL_OR_AUTH_REQ:
		// can_dlc is 3 => req_auth without CMAC (sent by VW)
		// can_dlc is 7 => req_auth with CMAC (sent by CTU)
		if (cf->can_dlc == 3 || cf->can_dlc == 7)
			// length should be 7 bytes, but VW node is not sending CMAC!!
			receive_auth_req(ctx, cf);
		else
			receive_sig16(ctx, cf);
		return MACAN_FRAME_PROCESSED;
	}

	return MACAN_FRAME_UNKNOWN;
}

/* 
 * Check if signal with given sig_num is 32bit or not
 *
 * @param[in] ctx     Macan context.
 * @param[in] sig_num Signal number
 * @return true if signal is 32bit, false otherwise.
 */
bool is_32bit_signal(struct macan_ctx *ctx, uint8_t sig_num)
{
    return (ctx->config->sigspec[sig_num].can_sid != 0 ? SUCCESS : ERROR);
}

/*
 * Get node's ECU-ID from CAN-ID.
 *
 * @param[in]  cfg   Macan configuration (ctx->config).
 * @param[in]  canid CAN-ID of node
 * @param[out] ecuid Pointer where to save ECU-ID
 *
 * @return True if node with passed CAN-ID was found, false otherwise.
 */
bool macan_canid2ecuid(const struct macan_config *cfg, uint32_t can_id, macan_ecuid *ecu_id)
{
	macan_ecuid i;

	for (i = 0; i < cfg->node_count; i++) {
		if (cfg->canid->ecu[i] == can_id) {
			if(ecu_id != NULL) {
				*ecu_id = i;
			}
			return SUCCESS; 
		}
	}
	return ERROR;
}

/*
 * Get information about communication partner from given CAN-ID
 *
 * @param[in]  ctx    Macan context.
 * @param[in]  can_id CAN-ID of communication partner
 *
 * @return Pointer to com_part of communication partner or pointer
 * to NULL if given CAN-ID does not belong to any communication partner
 */
struct com_part *canid2cpart(struct macan_ctx *ctx, uint32_t can_id)
{
	macan_ecuid ecu_id;

	if(!macan_canid2ecuid(ctx->config, can_id, &ecu_id)) {
		/* there is no node with given CAN-ID */
		return NULL;
	}

	/* if ECU-ID is not our communication partner
	 * it's cpart pointer is initialized to NULL,
	 * so we can directly return it. */
	return get_cpart(ctx, ecu_id);
}

void
macan_housekeeping_cb(macan_ev_loop *loop, macan_ev_timer *w, int revents)
{
	(void)loop; (void)revents; /* suppress warnings */
	struct macan_ctx *ctx = w->data;

	macan_request_expired_keys(ctx);
}

static void
can_rx_cb(macan_ev_loop *loop, macan_ev_can *w, int revents)
{
	(void)loop; (void)revents; /* suppress warnings */
	struct macan_ctx *ctx = w->data;
	struct can_frame cf;

	while (macan_read(ctx, &cf))
		macan_process_frame(ctx, &cf);
}

void __macan_init_cpart(struct macan_ctx *ctx, macan_ecuid i)
{
	if (ctx->cpart[i] == NULL) {
		ctx->cpart[i] = calloc(1, sizeof(struct com_part));
		ctx->cpart[i]->ecu_id = i;
	}
}

void __macan_init(struct macan_ctx *ctx, const struct macan_config *config, macan_ev_loop *loop, int sockfd)
{
	memset(ctx, 0, sizeof(struct macan_ctx));
	ctx->config = config;
	ctx->cpart = calloc(config->node_count, sizeof(struct com_part *));
	ctx->sighand = calloc(config->sig_count, sizeof(struct sig_handle *));
	ctx->sockfd = sockfd;
	ctx->loop = loop;
}

/**
 * Initialize MaCAN context.
 *
 * This function fills the macan_ctx structure. This function should be
 * called before using the MaCAN library.
 *
 * @param *ctx pointer to MaCAN context
 * @param *config pointer to configuration
 */
int macan_init(struct macan_ctx *ctx, const struct macan_config *config, macan_ev_loop *loop, int sockfd)
{
	assert(config->node_id != config->key_server_id);
	assert(config->node_id != config->time_server_id);

	__macan_init(ctx, config, loop, sockfd);
	unsigned i;

	/* Initialzize all possible communication partners based on configured signals */
	for (i = 0; i < config->sig_count; i++) {
		if (config->sigspec[i].src_id == config->node_id) {
			__macan_init_cpart(ctx, config->sigspec[i].dst_id);
		}
		if (config->sigspec[i].dst_id == config->node_id)
			__macan_init_cpart(ctx, config->sigspec[i].src_id);

		ctx->sighand[i] = malloc(sizeof(struct sig_handle));
		ctx->sighand[i]->presc = SIG_DONTSIGN; //SIG_DONTSIGN;
		ctx->sighand[i]->presc_cnt = 0;
		ctx->sighand[i]->flags = 0;
		ctx->sighand[i]->cback = NULL;
	}

	/* Also need to communicate with TS */
	__macan_init_cpart(ctx,config->time_server_id);

	/* Initialize event handlers */
	macan_ev_canrx_setup (ctx, &ctx->can_watcher, can_rx_cb);
	macan_ev_timer_setup (ctx, &ctx->housekeeping, macan_housekeeping_cb, 0, 1000);

	return 0;
}

void
macan_ev_timer_setup(struct macan_ctx *ctx, macan_ev_timer *ev,
		    void (*cb) (macan_ev_loop *loop,  macan_ev_timer *w, int revents),
		    unsigned after_ms, unsigned repeat_ms)
{
	macan_ev_timer_init(ev, cb, after_ms, repeat_ms);
	ev->data = ctx;
	macan_ev_timer_start(ctx->loop, ev);
}

void
macan_ev_canrx_setup(struct macan_ctx *ctx, macan_ev_can *ev,
		   void (*cb) (macan_ev_loop *loop,  macan_ev_can *w, int revents))
{
	macan_ev_can_init(ev, cb, ctx->sockfd, MACAN_EV_READ);
	ev->data = ctx;
	macan_ev_can_start (ctx->loop, ev);
}
