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

struct macan_sig_spec {
	uint8_t can_nsid;  /* can non-secured id */
	uint8_t can_sid;   /* can secured id */
	uint8_t src_id;    /* node dispathing this signal */
	uint8_t dst_id;    /* node receiving this signal */
	uint8_t presc;     /* prescaler */
};

/* signal callback signature */
typedef void (*sig_cback)(uint8_t sig_num, uint32_t sig_val);

/* MaCAN API functions */
int  macan_init(struct macan_ctx *ctx, const struct macan_sig_spec *sigspec);
void macan_set_ltk(struct macan_ctx *ctx, uint8_t *key);
void macan_request_keys(struct macan_ctx *ctx, int s);
int  macan_wait_for_key_acks(struct macan_ctx *ctx, int s);
int  macan_reg_callback(struct macan_ctx *ctx, uint8_t sig_num, sig_cback fnc);
void macan_send_sig(struct macan_ctx *ctx, int s, uint8_t sig_num, uint16_t signal);
int  macan_process_frame(struct macan_ctx *ctx, int s, const struct can_frame *cf);

#endif /* MACAN_H */

