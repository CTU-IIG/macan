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

#ifndef MACAN_H
#define MACAN_H

#include <stdint.h>
#include <stdbool.h>
#include "macan_ev.h"
#include "can_frame.h"

#ifdef __cplusplus
extern "C" {
#endif


struct macan_ctx;
struct can_frame;

/**
 * Type for ECU-ID
 */
typedef uint8_t macan_ecuid;

/**
 * Frame types for sending signals:
 * - Standard frame without signature (max 64 bit signals) - ID = can_nsid
 * - Standard frame with signature (max 32 bit signals) - ID = can_sid
 * - Crypt frame (max 16 bit signals) - ID CAN_ID(src-id)
 *
 * Possible values:
 * - !can_nsid && !can_sid => crypt frame
 * - !can_nsid && can_sid => std. frame w. sign
 * - can_nsid && !can_sid => std. frame wo. sign + crypt frame (depending on presc value)
 * - can_nsid && can_sid => std. frame wo. sign + std. frame w. sign (depending on presc value)
 */
struct macan_sig_spec {
	uint16_t can_nsid;  /**< CAN non-secured ID */
	uint16_t can_sid;   /**< CAN secured ID */
	uint8_t src_id;     /**< ECU-ID of node dispatching this signal */
	uint8_t dst_id;     /**< ECU-ID of node receiving this signal */
	uint8_t presc;      /**< Prescaler if > 0, zero means on-demand signal (see sig_auth_req frame) */
};

struct macan_ecu {
	uint32_t canid;
	const char *name;
};

struct macan_can_ids {
	uint32_t time;         /**< CAN-ID used to distribute time */
	struct macan_ecu *ecu; /**< Mapping from ECU-ID to crypt-frame CAN-ID */
};

/**
 * MaCAN network configuration
 */
struct macan_config {
	uint32_t sig_count;                   /**< Number of signals in sig_spec */
	const struct macan_sig_spec *sigspec; /**< Signal specification */
	uint8_t node_count;                   /**< Number of nodes (ECUs) in our network */
	const struct macan_can_ids *canid;    /**< CAN-IDs used by MaCAN */
	macan_ecuid key_server_id;            /**< ECU-ID of the key server */
	macan_ecuid time_server_id;           /**< ECU-ID of the time server */
	uint32_t time_div;                    /**< Number of microseconds in one MaCAN time unit */
	uint64_t skey_validity;               /**< Session key expiration time (microseconds) */
	uint32_t skey_chg_timeout;            /**< Timeout for waiting for session key (microseconds) */
	uint32_t time_req_sep;                /**< Minimum time between requests for authenticated time from one node (microseconds) */
	uint32_t time_delta;                  /**< Maximum time difference between our clock and TS (microseconds) */
};

/**
 * MaCAN node-specific configuration
 */
struct macan_node_config {
	macan_ecuid node_id;                  /**< Our ECU-ID (0-63) */
	const struct macan_key *ltk;          /**< Long-term key map (for communication with KS) */
};

/**
 * Structure for keeping session and long-term keys
 */
struct macan_key {
	uint8_t data[16];
};

enum macan_signal_status {
	MACAN_SIGNAL_NOAUTH,	/* Non-authenticated signal */
	MACAN_SIGNAL_AUTH,	/* Authenticity check succeeded */
	MACAN_SIGNAL_INVALID,	/* Authenticity check failed */
};

/**
 * signal callback signature
 */
typedef void (*macan_sig_cback)(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status status);

enum macan_process_status {
	MACAN_FRAME_UNKNOWN,
	MACAN_FRAME_PROCESSED,
	MACAN_FRAME_CHALLENGE,
};

/* MaCAN API functions */

struct macan_ctx *macan_alloc_mem(const struct macan_config *config,
				  const struct macan_node_config *node);
int  macan_init(struct macan_ctx *ctx, macan_ev_loop *loop, int sockfd);
int  macan_init_ks(struct macan_ctx *ctx, macan_ev_loop *loop, int sockfd, const struct macan_key * const *ltks);
int  macan_init_ts(struct macan_ctx *ctx, macan_ev_loop *loop, int sockfd);

void macan_request_keys(struct macan_ctx *ctx);
int  macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, macan_sig_cback fnc, macan_sig_cback invalid_cmac);
void macan_send_sig(struct macan_ctx *ctx, uint8_t sig_num, uint32_t signal);
enum macan_process_status macan_process_frame(struct macan_ctx *ctx, const struct can_frame *cf);
void macan_request_key(struct macan_ctx *ctx, macan_ecuid fwd_id);

void macan_ev_timer_setup(struct macan_ctx *ctx, macan_ev_timer *ev,
			  void (*cb) (macan_ev_loop *loop,  macan_ev_timer *w, int revents),
			  unsigned after_ms, unsigned repeat_ms);
void macan_ev_canrx_setup(struct macan_ctx *ctx, macan_ev_can *ev,
			  void (*cb) (macan_ev_loop *loop,  macan_ev_can *w, int revents));
void macan_request_expired_keys(struct macan_ctx *ctx);

bool macan_ev_run(macan_ev_loop *loop);

#ifdef __cplusplus
}
#endif

#endif /* MACAN_H */
