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

#ifndef MACAN_PRIVATE_H
#define MACAN_PRIVATE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <can_frame.h>
#include <macan.h>

/* Return values of functions returning bool */
#define SUCCESS true
#define ERROR	false

/* MaCAN message definitions */

#define FL_REQ_CHALLENGE      0U
#define FL_CHALLENGE	      1U
#define FL_SESS_KEY	      2U
#define FL_ACK		      2U
#define FL_SESS_KEY_OR_ACK    2U
#define FL_SIGNAL	      3U
#define FL_AUTH_REQ	      3U
#define FL_SIGNAL_OR_AUTH_REQ 3U

/* return values for receive_skey */
#define RECEIVE_SKEY_ERR -1
#define RECEIVE_SKEY_IN_PROGRESS -2

struct macan_req_challenge {
	uint8_t flags_and_dst_id;
	macan_ecuid fwd_id;
};

struct macan_challenge {
	uint8_t flags_and_dst_id;
	macan_ecuid fwd_id;
	uint8_t chg[6];
};

struct macan_sess_key {
	uint8_t flags_and_dst_id;
	uint8_t seq_and_len;
	uint8_t data[6];
};

struct macan_ack {
	uint8_t flags_and_dst_id;
	uint8_t group[3];
	uint8_t cmac[4];
};

struct macan_sig_auth_req {
	uint8_t flags_and_dst_id;
	uint8_t sig_num;
	uint8_t prescaler;
	uint8_t cmac[4];
};

struct macan_signal {
	uint8_t sig[4];
	uint8_t cmac[4];
};

struct macan_signal_ex {
	uint8_t flags_and_dst_id;
	uint8_t sig_num;
	uint8_t signal[2];
	uint8_t cmac[4];
};

/**
 * Timekeeping structure
 */
struct macan_time {
	int64_t offs;        /* contains the time difference between local time
   			        and TS time
				i.e. TS_time = Local_time + offs */
	int64_t chal_ts;    /* local timestamp when request for signed time was sent  */
	uint8_t chg[6];      /* challenge to the time server */
	uint8_t is_time_ready; /* set to 1 when signed time was received */
};

/**
 * Communication partner
 *
 * Stores run-time information about a communication partner.
 *
 * @param id    6-bit id
 * @param skey  wrapped key or consequently session key
 * @param stat  comunication channel status
 */
struct com_part {
	struct macan_key skey;	/* Session key (from key server) */
	uint64_t valid_until;	/* Local time of key expiration */
	uint8_t chg[6];		/* Challenge for communication with key server */
	uint8_t flags;
	uint32_t group_field;	/* Bitmask of known key sharing */
	uint32_t wait_for;	/* The value of group_field we are waiting for  */
	macan_ecuid ecu_id; /* ECU-ID of communication partner */
};

/**
 * Signal handle.
 *
 * Keeps a signal prescaler and a signal callback function.
 */
struct sig_handle {
	int presc;            /* prescaler settings for the signal, note the macros
	                         SIG_DONTSIGN and SIG_SIGNONCE */
	uint8_t presc_cnt;    /* prescaler counter counts the signal transmit attempts down and
	                         allows the transmit to happen only if presc_cnt == 0 */
	uint8_t flags;        /* mark AUTHREQ_SENT if signal request AUTH_REQ was sent */
	void (*cback)(uint8_t sig_num, uint32_t sig_val);
	void (*invalid_cback)(uint8_t sig_num, uint32_t sig_val);
};

#define SIG_DONTSIGN -1
#define SIG_SIGNONCE 0

#define AUTHREQ_SENT 1

/**
 * MaCAN context
 *
 * Omnipresent structure representing the state of the MaCAN library.
 */
struct macan_ctx {
	const struct macan_config *config;     /* MaCAN configuration passed to macan_init() */
	struct com_part **cpart;               /* vector of communication partners, e.g. stores keys */
	struct sig_handle **sighand;           /* stores signals settings, e.g prescaler, callback */
	struct macan_time time;                /* used to manage time of the protocol */
	uint64_t ack_timeout_abs;	       /* timeout for sending ACK messages ??? */
};

#define CANID(ctx, ecuid) ((ctx)->config->ecu2canid[ecuid])

bool macan_canid2ecuid(struct macan_ctx *ctx, uint32_t canid, macan_ecuid *ecuid);
int init(void);
void receive_sig16(struct macan_ctx *ctx, const struct can_frame *cf);
int macan_write(struct macan_ctx *ctx, int s, macan_ecuid dst_id, uint8_t sig_num, uint32_t signal);
int is_channel_ready(struct macan_ctx *ctx, uint8_t dst);
int is_skey_ready(struct macan_ctx *ctx, macan_ecuid dst_id);
void receive_auth_req(struct macan_ctx *ctx, const struct can_frame *cf);
void send_auth_req(struct macan_ctx *ctx, int s, macan_ecuid dst_id,uint8_t sig_num,uint8_t prescaler);
void receive_challenge(struct macan_ctx *ctx, int s, const struct can_frame *cf);
int receive_skey(struct macan_ctx *ctx, const struct can_frame *cf);
void gen_challenge(uint8_t *chal);
extern uint8_t *key_ptr;
extern uint8_t keywrap[32];
extern uint8_t g_chg[6];
extern uint8_t seq;
void send_ack(struct macan_ctx *ctx, int s,macan_ecuid dst_id);
int receive_ack(struct macan_ctx *ctx, const struct can_frame *cf);
extern uint8_t skey[24];
#ifdef __CPU_TC1798__
int write(int s, struct can_frame *cf, int len);
#endif
uint64_t read_time(void);
uint64_t macan_get_time(struct macan_ctx *ctx);
void receive_time(struct macan_ctx *ctx, int s, const struct can_frame *cf);
void receive_signed_time(struct macan_ctx *ctx, const struct can_frame *cf);
bool is_32bit_signal(struct macan_ctx *ctx, uint8_t sig_num);
bool cansid2signum(struct macan_ctx *ctx, uint32_t can_id, uint32_t *sig_num);
void print_frame(struct macan_ctx *ctx, struct can_frame *cf);
bool is_time_ready(struct macan_ctx *ctx);
struct com_part *canid2cpart(struct macan_ctx *ctx, uint32_t can_id);
bool gen_rand_data(void *dest, size_t len);
void macan_send_signal_requests(struct macan_ctx *ctx, int s);

static inline macan_ecuid macan_crypt_dst(const struct can_frame *cf)
{
	return cf->data[0] & 0x3f;
}

static inline unsigned macan_crypt_flags(const struct can_frame *cf)
{
	return (cf->data[0] & 0xc0) >> 6;
}

#endif /* MACAN_PRIVATE_H */
