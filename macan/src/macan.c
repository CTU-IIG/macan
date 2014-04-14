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
 * Initialize communication partner.
 *
 * Allocates com_part and sets it to the default value, i.e. wait
 * for this node and its counterpart to share the key.
 */
void init_cpart(struct macan_ctx *ctx, macan_ecuid i)
{
	struct com_part **cpart = ctx->cpart;
	cpart[i] = malloc(sizeof(struct com_part));
	memset(cpart[i], 0, sizeof(struct com_part));
	cpart[i]->wait_for = htole32(1U << i | 1U << ctx->config->node_id);
	cpart[i]->ecu_id = i;
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
int macan_init(struct macan_ctx *ctx, const struct macan_config *config)
{
	unsigned i;
	uint8_t cp;

	memset(ctx, 0, sizeof(struct macan_ctx));
	ctx->config = config;
	ctx->cpart = malloc(config->node_count * sizeof(struct com_part *));
	memset(ctx->cpart, 0, config->node_count * sizeof(struct com_part *));
	ctx->sighand = malloc(config->sig_count * sizeof(struct sig_handle *));
	memset(ctx->sighand, 0, config->sig_count * sizeof(struct sig_handle *));

	if(config->node_id == config->time_server_id) {
		/* We are timeserver, we need to init communication with
		 * every other node */
		for(i = 0; i < config->node_count; i++) {
			if(i == config->key_server_id || i == config->time_server_id) {
				/* skip KS and TS */
				continue;
			}
			init_cpart(ctx, (uint8_t)i);
		}
	} else {
		/* We are normal node */
		for (i = 0; i < config->sig_count; i++) {
			if (config->sigspec[i].src_id == config->node_id) {
				cp = config->sigspec[i].dst_id;

				if (ctx->cpart[cp] == NULL) {
					init_cpart(ctx, cp);
				}
			}
			if (config->sigspec[i].dst_id == config->node_id) {
				cp = config->sigspec[i].src_id;

				if (ctx->cpart[cp] == NULL) {
					init_cpart(ctx, cp);
				}
			}

			ctx->sighand[i] = malloc(sizeof(struct sig_handle));
			ctx->sighand[i]->presc = SIG_DONTSIGN; //SIG_DONTSIGN;
			ctx->sighand[i]->presc_cnt = 0;
			ctx->sighand[i]->flags = 0;
			ctx->sighand[i]->cback = NULL;
		}

		/* also need to communicate with TS */
		init_cpart(ctx,config->time_server_id);
	}

	return 0;
}

/**
 * Establishes authenticated channel.
 *
 * ACK messages are broadcasted in order to create authenticated
 * communication channel. This function sends ACK messages, if channel
 * is not ready (given we already have received key from KS)
 *
 * @param *ctx pointer to MaCAN context
 * @param s socket file descriptor
 */
int macan_wait_for_key_acks(struct macan_ctx *ctx, int s)
{
	uint8_t i;
	int r = 0;
	struct com_part **cpart;

	cpart = ctx->cpart;

	if (ctx->ack_timeout_abs > read_time())
		return -1;
	ctx->ack_timeout_abs = read_time() + ctx->config->ack_timeout;

	for (i = 0; i < ctx->config->node_count; i++) {
		if (i == ctx->config->time_server_id)
			continue;

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

	return 0;
}
/**
 * Sends signal requests
 *
 * Signal request are send if channel and time is ready
 */
void macan_send_signal_requests(struct macan_ctx *ctx, int s)
{

	uint8_t cp, presc, i;
	const struct macan_sig_spec *sigspec;
	struct sig_handle **sighand;

	sighand = ctx->sighand;
	sigspec = ctx->config->sigspec;

	/* ToDo: repeat auth_req for the case the signal source will restart */
	for (i = 0; i < ctx->config->sig_count; i++) {
		if (sigspec[i].dst_id != ctx->config->node_id)
			continue;

		cp = sigspec[i].src_id;
		presc = sigspec[i].presc;

		if (!is_channel_ready(ctx, cp))
			continue;

		if(!is_time_ready(ctx)) // time must be ready before receiving signals
			continue;

		if(sigspec[i].src_id == ctx->config->time_server_id) // don't send requests for dummy time signals
			continue;

		if (!(sighand[i]->flags & AUTHREQ_SENT)) {
			sighand[i]->flags |= AUTHREQ_SENT;
			print_msg(MSG_REQUEST,"Sending req auth for signal #%d\n",i);
			send_auth_req(ctx, s, cp, i, presc);
		}
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
int macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, sig_cback fnc, sig_cback invalid_cmac)
{
	ctx->sighand[sig_num]->cback = fnc;
	ctx->sighand[sig_num]->invalid_cback = invalid_cmac;

	return 0;
}

/**
 * Sends an ACK message.
 */
void send_ack(struct macan_ctx *ctx, int s, uint8_t dst_id)
{
	if(ctx->config->ack_disable) {
		/* ACK is disabled, don't do anything */
		return;
	}

	struct macan_ack ack = { .flags_and_dst_id = (uint8_t)(FL_ACK << 6 | (dst_id & 0x3f)), .group = {0}, .cmac = {0}};
	uint8_t plain[8] = {0};
	uint32_t time;
	struct macan_key skey;
	struct can_frame cf = {0};
	volatile int res;

	if (!is_skey_ready(ctx, dst_id))
		return;

	if(!is_time_ready(ctx)) 
		return;

	skey = get_cpart(ctx, dst_id)->skey;
	memcpy(&ack.group, &(get_cpart(ctx, dst_id)->group_field), 3);
	time = (uint32_t)macan_get_time(ctx);

	memcpy(plain, &htole32(time), 4);
	plain[4] = dst_id;
	memcpy(plain + 5, ack.group, 3);

#ifdef DEBUG_TS
	memcpy(ack.cmac, &htole32(time), 4);
#else
	macan_sign(&skey, ack.cmac, plain, sizeof(plain));
#endif
	cf.can_id = CANID(ctx, ctx->config->node_id);
	cf.can_dlc = 8;
	memcpy(cf.data, &ack, 8);

	res = (int) write(s, &cf, sizeof(struct can_frame));
	if (res != 16) {
		fail_printf("%s\n","failed to send some bytes of ack");
	}

	return;
}

/**
 * Receives an ACK message.
 *
 * Returns 1 if the incoming ack does not contain this node in its
 * group field. Therefore, the updated ack should be broadcasted.
 * TODO: add groups communication
 * TODO: what if ack contains me
 */
int receive_ack(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct com_part *cp;
	struct macan_ack *ack = (struct macan_ack *)cf->data;
	uint8_t plain[8];
	struct macan_key skey;

	if(!(cp = canid2cpart(ctx, cf->can_id)))
		return -1;
	
	skey = cp->skey;

	plain[4] = ack->flags_and_dst_id & 0x3f;
	memcpy(plain + 5, ack->group, 3);

	/* ToDo: make difference between wrong CMAC and not having the key */
	if (!macan_check_cmac(ctx, &skey, ack->cmac, plain, plain, sizeof(plain))) {
		fail_printf("%s\n","error: ACK CMAC failed");
		return -1;
	}

	uint32_t is_present = htole32(1U << ctx->config->node_id);
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
	if(!gen_rand_data(chal, 6)) {
		print_msg(MSG_FAIL,"Failed to read enough random bytes.\n");
		exit(1);
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
int receive_skey(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_sess_key *sk;
	macan_ecuid fwd_id;
	static uint8_t keywrap[32];
	uint8_t unwrapped[24];
	uint8_t seq, len;

	sk = (struct macan_sess_key *)cf->data;

	/* it reads wrong values, dirty hack to read directly from CAN frame 
	 * bytes are in reverse order! */
	seq = (cf->data[1] & 0xF0) >> 4;
	len = (cf->data[1] & 0x0F);

	/* this is because of VW macan sends len 6 in last key packet */
	if(seq == 5) len = 2;

	if (!((seq <= 5) && ((seq != 5 && len == 6) || (seq == 5 && len == 2))))
		return RECEIVE_SKEY_ERR;

	memcpy(keywrap + 6 * seq, sk->data, len);

	if (seq == 5) {

		macan_unwrap_key(ctx->config->ltk, 32, unwrapped, keywrap);
		fwd_id = unwrapped[17];

		if (fwd_id >= ctx->config->node_count || get_cpart(ctx, fwd_id) == NULL) {
			fail_printf("unexpected fwd_id %#x\n", fwd_id);
			return RECEIVE_SKEY_ERR;
		}

		if(!memchk(unwrapped+18, get_cpart(ctx, fwd_id)->chg, 6)) {
			fail_printf("check cmac from %d\n", fwd_id);
			return RECEIVE_SKEY_ERR;
		}

		get_cpart(ctx, fwd_id)->valid_until = read_time() + ctx->config->skey_validity;
		memcpy(get_cpart(ctx, fwd_id)->skey.data, unwrapped, 16);
		
		// initialize group field - this will work only for ecu_id <= 23
		get_cpart(ctx, fwd_id)->group_field |= htole32(1U << ctx->config->node_id);

		// print key
		print_msg(MSG_OK,"KEY (%d -> %d) is ", ctx->config->node_id, fwd_id);
		print_hexn(get_cpart(ctx, fwd_id)->skey.data, 16);

		return fwd_id;	/* FIXME: fwd_id can be 0 as well. What our callers do with the returned value? */
	}

	return RECEIVE_SKEY_IN_PROGRESS;
}

/**
 * macan_send_challenge() - requests key from KS or signed time from TS
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
void macan_send_challenge(struct macan_ctx *ctx, int s, macan_ecuid dst_id, macan_ecuid fwd_id, uint8_t *chg)
{
	struct can_frame cf = {0};
	struct macan_challenge chal = { .flags_and_dst_id = (uint8_t)((FL_CHALLENGE << 6) | (dst_id & 0x3F)), .fwd_id = fwd_id };

	if (chg) {
		gen_challenge(chg);
		memcpy(chal.chg, chg, 6);
	}

	cf.can_id = CANID(ctx, ctx->config->node_id);
	cf.can_dlc = 8;
	memcpy(cf.data, &chal, sizeof(struct macan_challenge));

#ifdef DEBUG
    // Print info start
    printf(ANSI_COLOR_CYAN "SEND Challenge" ANSI_COLOR_RESET "\n"); 
    printf("dst_id: 0x%X, fwd_id: 0x%X, ",
            dst_id,
            fwd_id);
    if (chg) {
	    printf("chal: ");
	    print_hexn(chg,6);
    } else
	    printf("\n");
    // Print info end
#endif

	write(s, &cf, sizeof(struct can_frame));
}

/**
 * receive_challenge() - responds to REQ_CHALLENGE from KS
 */
void receive_challenge(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	macan_ecuid fwd_id;
	struct macan_challenge *ch = (struct macan_challenge *)cf->data;
	struct com_part **cpart;

	cpart = ctx->cpart;
	fwd_id = ch->fwd_id;
	if (fwd_id >= ctx->config->node_count)
		return;

	if (cpart[fwd_id] == NULL) {
		/* TODO: When can this happen? In key server only? */
		cpart[fwd_id] = malloc(sizeof(struct com_part));
		memset(cpart[fwd_id], 0, sizeof(struct com_part));

		cpart[fwd_id]->wait_for = htole32(1U << fwd_id | 1U << ctx->config->node_id);
	}

	cpart[fwd_id]->valid_until = read_time() + ctx->config->skey_chg_timeout;
	macan_send_challenge(ctx, s, ctx->config->key_server_id, ch->fwd_id, cpart[ch->fwd_id]->chg);
}

/**
 * Receive unsigned time.
 *
 * Receives time and checks whether local clock is in sync. If local clock
 * is out of sync, it sends a request for signed time.
 */
void receive_time(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	uint32_t time_ts;
	int64_t recent;
	uint64_t time_ts_us;
	int64_t delta;

	if (!is_skey_ready(ctx, ctx->config->time_server_id)) {
		return;
	}

	memcpy(&time_ts, cf->data, 4);
	time_ts = le32toh(time_ts);
	time_ts_us = (uint64_t)time_ts * ctx->config->time_div;
	recent = (int64_t)read_time() + ctx->time.offs; /* Estimated time on TS (us) */

	if (ctx->time.chal_ts) {
		/* Don't ask TS for authenticated time too often */
		if ((recent - ctx->time.chal_ts) < ctx->config->time_timeout)
			return;
	}

	delta = llabs((int64_t)recent - (int64_t)time_ts_us);

	if (delta > ctx->config->time_delta) {
		print_msg(MSG_WARN,"time out of sync (%lld us = %"PRIu64" - %"PRIu64")\n", delta, recent, time_ts_us);
		print_msg(MSG_REQUEST,"Requesting signed time\n");

		ctx->time.chal_ts = recent;
		macan_send_challenge(ctx, s, ctx->config->time_server_id, 0, ctx->time.chg);
	}
}

/**
 * Receive signed time.
 *
 * Receives time and sets local clock according to it.
 */
void receive_signed_time(struct macan_ctx *ctx, const struct can_frame *cf)
{
	uint32_t time_ts;
	uint8_t plain[12];
	struct macan_key skey;
	struct com_part **cpart;
	int64_t time_ts_us;

	cpart = ctx->cpart;

	memcpy(&time_ts, cf->data, 4);

	if (!is_skey_ready(ctx, ctx->config->time_server_id)) {
		ctx->time.chal_ts = 0;
		return;
	}

	skey = cpart[ctx->config->time_server_id]->skey;
	memcpy(plain, &time_ts, 4); // received time
	memcpy(plain + 4, ctx->time.chg, 6); // challenge
	memcpy(plain + 10, &htole32(CANID(ctx, ctx->config->time_server_id)),2);

	if (!macan_check_cmac(ctx, &skey, cf->data + 4, plain, NULL, sizeof(plain))) {
		/* not a fatal error, we possibly received signed time for different node */
		//fail_printf("check cmac time %d\n", time_ts);
		return;
	}

	/* cmac check ok, convert to our endianess */
	time_ts = le32toh(time_ts);
	
	time_ts_us = (int64_t)time_ts * ctx->config->time_div;

	ctx->time.offs += (time_ts_us - ctx->time.chal_ts);
	ctx->time.chal_ts = 0;
	ctx->time.is_time_ready = 1;

	print_msg(MSG_OK,"signed time = %d, offs %"PRIu64"\n",time_ts, ctx->time.offs);
}

/**
 * Send authorization request.
 *
 * Requests an authorized signal from communication partner.
 *
 * @param dst_id   com. partner id, the signal source
 * @param sig_num  signal id
 * @param prescaler
 */
void send_auth_req(struct macan_ctx *ctx, int s, macan_ecuid dst_id, uint8_t sig_num, uint8_t prescaler)
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

	write(s, &cf, sizeof(cf));
}

void receive_auth_req(struct macan_ctx *ctx, const struct can_frame *cf)
{
	struct macan_key skey;
	uint8_t plain[8];
	uint8_t sig_num;
	int can_sid,can_nsid;
	struct macan_sig_auth_req *areq;
	struct com_part *cp;
	struct sig_handle **sighand;

	if(!(cp = canid2cpart(ctx, cf->can_id)))
		return;

	sighand = ctx->sighand;

	areq = (struct macan_sig_auth_req *)cf->data;
	skey = cp->skey;

	if (areq->sig_num >= ctx->config->sig_count)
		return;

	plain[4] = (macan_ecuid)cp->ecu_id;
	plain[5] = ctx->config->node_id;
	plain[6] = areq->sig_num;
	plain[7] = areq->prescaler;

	/* Check CMAC only if can_dlc is 7 */
	if(cf->can_dlc == 7) {
		/* TODO: Why this? We should never react on non-auth request. VW_COMPAT?   */
		if (!macan_check_cmac(ctx, &skey, areq->cmac, plain, plain, sizeof(plain))) {
			printf("error: sig_auth cmac is incorrect\n");
			/* TODO: return missing? */
		}
	}

	print_msg(MSG_INFO,"Received auth request for signal #%d\n",areq->sig_num);
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
int macan_write(struct macan_ctx *ctx, int s, macan_ecuid dst_id, uint8_t sig_num, uint32_t signal)
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
		memcpy(plain, &htole32(signal), 4);
		memcpy(plain+8, &ctx->config->sigspec[sig_num].can_sid,2); 
		memcpy(sig32->sig, &htole32(signal), 4);
		cf.can_id = ctx->config->sigspec[sig_num].can_sid;
		plain_length = 10;
		cmac = sig32->cmac;
	} else {
		struct macan_signal_ex *sig16 = (struct macan_signal_ex *) sig;

		sig16->flags_and_dst_id = FL_SIGNAL << 6 | (dst_id & 0x3f);
		sig16->sig_num = sig_num;
		memcpy(&sig16->signal, &signal, 2);

		memcpy(plain, &htole32(t), 4);
		memcpy(plain + 4, &(ctx->config->node_id), 1);
		memcpy(plain + 5, &dst_id, 1);
		memcpy(plain + 6, &htole32(signal), 2);
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
	write(s, &cf, sizeof(cf));

	return 0;
}

/**
 * Dispatch a signal.
 *
 * The prescaler settings are considered and the signal is send.
 *
 * @param s        socket handle
 * @param sig_num  signal id
 * @param signal   signal value
 */
/* ToDo: return result */
void macan_send_sig(struct macan_ctx *ctx, int s, uint8_t sig_num, uint32_t signal)
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
		macan_write(ctx, s, dst_id, sig_num, signal);
		sighand[sig_num]->presc = SIG_DONTSIGN;
		break;
	default:
		if (sighand[sig_num]->presc_cnt > 0) {
			sighand[sig_num]->presc_cnt--;
		} else {
			macan_write(ctx, s, dst_id, sig_num, signal);
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

	cmac = sig16->cmac;
	memcpy(&sig_val, sig16->signal, 2);
	sig_val = le32toh(sig_val);

	/* Prepare plain text for CMAC */
	plain_length = 8;
	fill_time = plain;
	memcpy(plain + 4, &(sigspec->src_id), 1);
	memcpy(plain + 5, &(ctx->config->node_id), 1);
	memcpy(plain + 6, sig16->signal, 2);

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
		fail_printf("No key to check signal from %d\n", sigspec->src_id);
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
			fail_printf("CMAC error for signal %d\n", sig_num);
		return;
	}

	print_msg(MSG_SIGNAL,"Received signal #%d, value: %d\n", sig_num, sig_val);

	if (sighand && sighand->cback)
		sighand->cback((uint8_t)sig_num, sig_val);
}

/**
 * Check if has the session key.
 *
 * @param dst_id  id of a node to whom check if has the key
 * @return        1 if has the key, otherwise 0
 */
int is_skey_ready(struct macan_ctx *ctx, macan_ecuid dst_id)
{
	if (get_cpart(ctx, dst_id) == NULL)
		return 0;

	return (get_cpart(ctx, dst_id)->group_field & htole32(1U << ctx->config->node_id)) ? 1 : 0;
}

/**
 * Check if has the authorized channel.
 *
 * Checks if has the session key and if the communication partner
 * has acknowledged the communication with an ACK message.
 */
int is_channel_ready(struct macan_ctx *ctx, uint8_t dst)
{
	if(ctx->config->ack_disable) {
		/* ACK is disabled, channel is ready */	
		return 1;
	}

	if (get_cpart(ctx, dst) == NULL)
		return 0;

	uint32_t grp = (*((uint32_t *)&get_cpart(ctx, dst)->group_field)) & htole32(0x00ffffff);
	uint32_t wf = (*((uint32_t *)&get_cpart(ctx, dst)->wait_for)) & htole32(0x00ffffff);

	return ((grp & wf) == wf);
}

/*
 * Check whether we have clock synchronized with TS
 *
 * @param[in] ctx Macan context.
 * @return true if time is ready, false otherwise.
 */
bool is_time_ready(struct macan_ctx *ctx)
{
	return ctx->time.is_time_ready;	
}

/**
 * Request keys to all comunication partners.
 *
 * Requests keys and repeats until success.
 *
 * @param *ctx pointer to MaCAN context
 * @param s socket file descriptor
 */
void macan_request_keys(struct macan_ctx *ctx, int s)
{
	uint8_t i;
	struct com_part **cpart;

	cpart = ctx->cpart;

	for (i = 0; i < ctx->config->node_count; i++) {
		if (cpart[i] == NULL)
			continue;

		if (cpart[i]->valid_until > read_time())
			continue;

		print_msg(MSG_REQUEST,"Requesting skey for node #%d\n",i);
		macan_send_challenge(ctx, s, ctx->config->key_server_id, i, cpart[i]->chg);
		cpart[i]->valid_until = read_time() + ctx->config->skey_chg_timeout;
	}

	return;
}

/**
 * Get time of the MaCAN protocol.
 */
uint64_t macan_get_time(struct macan_ctx *ctx)
{
	return (read_time() + (uint64_t)ctx->time.offs) / ctx->config->time_div;
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
 */
int macan_process_frame(struct macan_ctx *ctx, int s, const struct can_frame *cf)
{
	// test if can_id is can_sid of any signal
	uint32_t sig32_num; 
	if(cansid2signum(ctx, cf->can_id,&sig32_num)) {
		receive_sig32(ctx, cf, sig32_num);
		return 1;
	}

	struct macan_crypt_frame *cryf = (struct macan_crypt_frame *)cf->data;
	int fwd;

	/* ToDo: make sure all branches end ASAP */
	/* ToDo: macan or plain can */
	/* ToDo: crypto frame or else */
	if(cf->can_id == CANID(ctx, ctx->config->node_id))
		return 1;
	if (cf->can_id == CANID(ctx,ctx->config->time_server_id)) {
		switch(cf->can_dlc) {
		case 4:
			receive_time(ctx, s, cf);
			return 1;
		case 8:
			receive_signed_time(ctx, cf);
			return 1;
		}
	}

	if (GET_DST_ID(cryf->flags_and_dst_id) != ctx->config->node_id)
		return 1;


	switch (GET_FLAGS(cryf->flags_and_dst_id)) {
	case FL_CHALLENGE:
		receive_challenge(ctx, s, cf);
		break;
	case FL_SESS_KEY_OR_ACK:
		if (cf->can_id == CANID(ctx, ctx->config->key_server_id)) {
			fwd = receive_skey(ctx, cf);
			if (fwd >= 0) {
				send_ack(ctx, s, (uint8_t)fwd);
			}
			break;
		}

		/* ToDo: what if ack CMAC fails, there should be no response */
		if (receive_ack(ctx, cf) == 1) {
			macan_ecuid ecu_id;
			if (canid2ecuid(ctx, cf->can_id, &ecu_id)) {
				send_ack(ctx, s, (uint8_t)ecu_id);
			}

		}
		break;
	case FL_SIGNAL_OR_AUTH_REQ:
		// can_dlc is 3 => req_auth without CMAC (sent by VW)
		// can_dlc is 7 => req_auth with CMAC (sent by CTU)
		if (cf->can_dlc == 3 || cf->can_dlc == 7)
			// length should be 7 bytes, but VW node is not sending CMAC!!
			receive_auth_req(ctx, cf);
		else
			receive_sig16(ctx, cf);
		break;
	}

	return 0;
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
 * Get signal number of 32bit signal from it's secure CAN-ID.
 * 
 * @param[in]  ctx     Macan context.
 * @param[in]  can_id  secure CAN-ID of signal
 * @param[out] sig_num Pointer where signal number will be written 
 * @return true if singal number was found, false otherwise.
 */
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

/*
 * Get node's ECU-ID from CAN-ID.
 *
 * @param[in]  ctx   Macan context.
 * @param[in]  canid CAN-ID of node
 * @param[out] ecuid Pointer where to save ECU-ID
 *
 * @return True if node with passed CAN-ID was found, false otherwise.
 */
bool canid2ecuid(struct macan_ctx *ctx, uint32_t can_id, macan_ecuid *ecu_id)
{
	macan_ecuid i;

	for (i = 0; i < ctx->config->node_count; i++) {
		if (ctx->config->ecu2canid[i] == can_id) {
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

	if(!canid2ecuid(ctx, can_id, &ecu_id)) {
		/* there is no node with given CAN-ID */
		return NULL;
	}
	
	/* if ECU-ID is not our communication partner
	 * it's cpart pointer is initialized to NULL,
	 * so we can directly return it. */
	return get_cpart(ctx, ecu_id);
}
