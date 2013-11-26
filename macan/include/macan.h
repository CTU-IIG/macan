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

#include <macan_private.h>

/*
 * Frame types for sending signals:
 * - Standard frame without signature (max 64 bit signals) - ID = can_nsid
 * - Standard frame with signature (max 32 bit signals) - ID = can_sid
 * - Crypt frame (max 16 bit signals) - ID CAN_ID(src-id)
 *
 * !can_nsid && !can_sid => crypt frame
 * !can_nsid && can_sid => std. frame w. sign
 * can_nsid && !can_sid => std. frame wo. sign + crypt frame (depending on presc value)
 * can_nsid && can_sid => std. frame wo. sign + std. frame w. sign (depending on presc value)
 */
struct macan_sig_spec {
	uint8_t can_nsid;  /* can non-secured id */
	uint8_t can_sid;   /* can secured id */
	uint8_t src_id;    /* node dispathing this signal */
	uint8_t dst_id;    /* node receiving this signal */
	uint8_t presc;     /* prescaler */
};

/* Node specification */
struct macan_node_spec {
    uint16_t can_id; /* CAN-ID node uses for its frames */
    uint8_t ecu_id; /* ECU-ID used for addressing in MaCAN protocol (crypt_id in JSON config from VW) */
    char *name;
};

/* signal callback signature */
typedef void (*sig_cback)(uint8_t sig_num, uint32_t sig_val);

/* MaCAN API functions */
int  macan_init(struct macan_ctx *ctx, const struct macan_sig_spec *sigspec, const struct macan_node_spec *nodespec);
void macan_set_ltk(struct macan_ctx *ctx, uint8_t *key);
void macan_request_keys(struct macan_ctx *ctx, int s);
int  macan_wait_for_key_acks(struct macan_ctx *ctx, int s);
int  macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, sig_cback fnc);
void macan_send_sig(struct macan_ctx *ctx, int s, uint8_t sig_num, uint32_t signal);
int  macan_process_frame(struct macan_ctx *ctx, int s, const struct can_frame *cf);

#endif /* MACAN_H */
