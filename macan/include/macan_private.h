/*
 *  Copyright 2014-2015 Czech Technical University in Prague
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
	uint8_t sig_val[2];
	uint8_t cmac[4];
};

/**
 * Timekeeping structure
 */
struct macan_timekeeping {
	uint64_t offs;      /* contains the time difference between local time and TS time
			       i.e. TS_time = Local_time + offs */
	uint32_t nonauth_ts;	/* Last received non-authenticated time */
	uint64_t nonauth_loc;	/* Local time when last non-authenticated time was received */

	uint64_t chal_ts;   /* local timestamp when request for signed time was sent  */
	uint8_t chg[6];	    /* challenge to the time server */
	bool ready;   	    /* set to true after first signed time message was received */
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
	bool key_received;	/* True iff any key was ever received from the key server */
	struct macan_key skey;	/* Session key (from key server) */
	uint64_t valid_until;	/* Local time of key expiration */
	bool awaiting_skey;	/* True iff challenge was sent and we wait for the session key */
	uint8_t chg[6];		/* Challenge for communication with key server */
	uint8_t flags;
	uint32_t group_field;	/* Bitmask of known key sharing */
	macan_ecuid ecu_id;	/* ECU-ID of communication partner */
	void (*skey_callback)(struct macan_ctx *ctx, macan_ecuid dst_id);
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

#define SIG_DONTSIGN 0
#define SIG_SIGNONCE -1

#define AUTHREQ_SENT 1

/**
 * MaCAN context
 *
 * Omnipresent structure representing the state of the MaCAN library.
 */
struct macan_ctx {
	const struct macan_config *config;     /* MaCAN configuration passed to macan_init() */
	const struct macan_node_config *node;  /* Node configuration */
	struct com_part **cpart;               /* vector of communication partners, e.g. stores keys */
	struct sig_handle **sighand;           /* stores signals settings, e.g prescaler, callback */
	struct macan_timekeeping time; 	       /* used to manage time of the protocol */
	uint8_t keywrap[32];		       /* Temporary storage for wrapped session key */
	unsigned rcvd_skey_seq;		       /* bitmap indicating which sess_key messages were received */
	int sockfd;			       /* Socket (or CAN interface id) used for CAN communication */
	macan_ev_loop *loop;
	macan_ev_can can_watcher;
	macan_ev_timer housekeeping;
	bool print_msg_enabled;
	union {
		struct { /* time server */
			macan_ev_timer time_bcast;
			uint64_t bcast_time;
			struct {
				bool pending;
				uint8_t chg[6];
			} *auth_req;	       /* Pending auth requests indexed by node ID */
		} ts;
		struct { /* key server */
			const struct macan_key * const *ltk;
			macan_ev_timer time_bcast;
			uint64_t bcast_time;
		} ks;
	};
};

#define CANID(ctx, ecuid) ((ctx)->config->canid->ecu[ecuid].canid)

bool macan_canid2ecuid(const struct macan_config *cfg, uint32_t canid, macan_ecuid *ecuid);
bool is_skey_ready(struct macan_ctx *ctx, macan_ecuid dst_id);
void receive_challenge(struct macan_ctx *ctx, const struct can_frame *cf);
uint64_t read_time(void);
uint64_t macan_get_time(struct macan_ctx *ctx);
bool is_32bit_signal(struct macan_ctx *ctx, uint8_t sig_num);
void print_frame(const struct macan_ctx *ctx, struct can_frame *cf, const char *prefix);
bool is_time_ready(struct macan_ctx *ctx);
struct com_part *canid2cpart(struct macan_ctx *ctx, uint32_t can_id);
bool gen_rand_data(void *dest, size_t len);
bool macan_read(struct macan_ctx *ctx, struct can_frame *cf);
bool macan_send(struct macan_ctx *ctx,  const struct can_frame *cf);
const char *macan_ecu_name(struct macan_ctx *ctx, macan_ecuid id);

static inline macan_ecuid macan_crypt_dst(const struct can_frame *cf)
{
	return cf->data[0] & 0x3f;
}

static inline unsigned macan_crypt_flags(const struct can_frame *cf)
{
	return (cf->data[0] & 0xc0) >> 6;
}

void __macan_init(struct macan_ctx *ctx, const struct macan_config *config, const struct macan_node_config *node, macan_ev_loop *loop, int sockfd);
void __macan_init_cpart(struct macan_ctx *ctx, macan_ecuid i);
void macan_housekeeping_cb(macan_ev_loop *loop, macan_ev_timer *w, int revents);
void macan_target_init(struct macan_ctx *ctx);


#endif /* MACAN_PRIVATE_H */
