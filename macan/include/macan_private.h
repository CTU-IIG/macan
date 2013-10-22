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
#include <can_frame.h>

/* MaCAN message definitions */

struct macan_crypt_frame {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
};

struct macan_challenge {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t fwd_id : 8;
	uint8_t chg[6];
};

struct macan_sess_key {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t seq : 4;
	uint8_t len : 4;
	uint8_t data[6];
};

struct macan_ack {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t group[3];
	uint8_t cmac[4];
};

struct macan_sig_auth_req {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t prescaler;
	uint8_t cmac[4];
};

struct macan_signal {
	uint8_t sig[4];
	uint8_t cmac[4];
};

struct macan_signal_ex {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t signal[2];
	uint8_t cmac[4];
};

/**
 * Timekeeping structure
 */
struct macan_time {
	uint64_t offs;       /* contains the time difference between local time
   			        and TS time;
				i.e. TS_time = Local_time + offs */
	uint64_t chal_ts;    /* local timestamp when request for signed time was sent  */
	uint8_t chg[6];      /* challenge to the time server */
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
	uint8_t skey[16];	/* Session key (from key server) */
	uint64_t valid_until;	/* Local time of key expiration */
	uint8_t chg[6];		/* Challenge for communication with key server */
	uint8_t flags;
	uint32_t group_field;	/* Bitmask of known key sharing */
	uint32_t wait_for;	/* The value of group_field we are waiting for  */
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
	const struct macan_sig_spec *sigspec;  /* static configuration given, see demo */
	struct com_part **cpart;               /* vector of communication partners, e.g. stores keys */
	struct sig_handle **sighand;           /* stores signals settings, e.g prescaler, callback */
	uint8_t ltk[16];                       /* key shared with the key server */
	struct macan_time time;                /* used to manage time of the protocol */
	uint64_t timeout_ack;                  /* timeout for sending ACK messages */
};

void unwrap_key(uint8_t *key, size_t len, uint8_t *dst, uint8_t *src);
int check_cmac(struct macan_ctx *ctx, uint8_t *skey, const uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len);
int init();
void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len);
void receive_sig(struct macan_ctx *ctx, const struct can_frame *cf);
int macan_write(struct macan_ctx *ctx, int s, uint8_t dst_id, uint8_t sig_num, uint32_t signal);
int is_channel_ready(struct macan_ctx *ctx, uint8_t dst);
int is_skey_ready(struct macan_ctx *ctx, uint8_t dst_id);
void receive_auth_req(struct macan_ctx *ctx, const struct can_frame *cf);
void send_auth_req(struct macan_ctx *ctx, int s,uint8_t dst_id,uint8_t sig_num,uint8_t prescaler);
void receive_challenge(struct macan_ctx *ctx, int s, const struct can_frame *cf);
void send_challenge(int s, uint8_t dst_id, uint8_t fwd_id, uint8_t *chg);
int receive_skey(struct macan_ctx *ctx, const struct can_frame *cf);
void gen_challenge(uint8_t *chal);
extern uint8_t *key_ptr;
extern uint8_t keywrap[32];
extern uint8_t g_chg[6];
extern uint8_t seq;
void send_ack(struct macan_ctx *ctx, int s,uint8_t dst_id);
int receive_ack(struct macan_ctx *ctx, const struct can_frame *cf);
extern uint8_t skey[24];
#ifdef __CPU_TC1798__
int write(int s, struct can_frame *cf, int len);
#endif
uint64_t read_time();
uint64_t get_macan_time();
void receive_time(struct macan_ctx *ctx, int s, const struct can_frame *cf);
void receive_signed_time(struct macan_ctx *ctx, int s, const struct can_frame *cf);

#endif /* MACAN_PRIVATE_H */
