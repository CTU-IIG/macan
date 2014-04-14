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

#ifndef MACAN_H
#define MACAN_H

#include <stdint.h>

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
	uint16_t can_nsid;  /**< can non-secured id */
	uint16_t can_sid;   /**< can secured id */
	uint8_t src_id;     /**< ECU-ID of node dispathing this signal */
	uint8_t dst_id;     /**< ECU-ID of node receiving this signal */
	uint8_t presc;      /**< prescaler */
};

/**
 * Main configuration
 */
struct macan_config {
	macan_ecuid node_id;                  /**< Our ECU-ID (0-63) */
	const struct macan_key *ltk;          /**< Long-term key map (for communication with KS) */
	uint32_t sig_count;                   /**< Number of sinals in sig_spec */
	const struct macan_sig_spec *sigspec; /**< Signal specification */
	uint8_t node_count;                   /**< Number of nodes (ECUs) in our network */
	const uint32_t *ecu2canid;            /**< Mapping from ECU-ID to crypt-frame CAN-ID */
	macan_ecuid key_server_id;            /**< ECU-ID of the key server */
	macan_ecuid time_server_id;           /**< ECU-ID of the time server */
	uint32_t time_div;                    /**< Number of microseconds in one MaCAN time unit */
	uint32_t ack_timeout;                 /**< Timeout for waiting for key acknowledge (microseconds) */
	uint64_t skey_validity;               /**< Session key expiration time (microseconds) */
	uint32_t skey_chg_timeout;            /**< Timeout for waiting for session key (microseconds) */
	uint32_t time_timeout;                /**< Timeout for authenticated time (microseconds) */
	uint32_t time_bcast_period;           /**< Timeserver broadcast period for plain time (microseconds) */
	uint32_t time_delta;                  /**< Maximum time divergency between our clock and TS (microseconds) */
	int ack_disable;                      /**< Disable ACK messages (to be compatible with VW implementation) */
};

/**
 * Structure for keeping LTK
 */
struct macan_key {
	uint8_t data[16];
};

/**
 * signal callback signature
 */
typedef void (*sig_cback)(uint8_t sig_num, uint32_t sig_val);

/* MaCAN API functions */


int  macan_init(struct macan_ctx *ctx, const struct macan_config *config);
void macan_request_keys(struct macan_ctx *ctx, int s);
int  macan_wait_for_key_acks(struct macan_ctx *ctx, int s);
int  macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, sig_cback fnc, sig_cback invalid_cmac);
void macan_send_sig(struct macan_ctx *ctx, int s, uint8_t sig_num, uint32_t signal);
int  macan_process_frame(struct macan_ctx *ctx, int s, const struct can_frame *cf);
void macan_send_challenge(struct macan_ctx *ctx, int s, macan_ecuid dst_id, macan_ecuid fwd_id, uint8_t *chg);

#endif /* MACAN_H */
