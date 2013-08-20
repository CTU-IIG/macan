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

#define NODE_KS 0
#define NODE_TS 1
#define SIG_TIME 4
#define TIME_DELTA 1000   /* tolerated time divergency from TS in usecs */
#define TIME_DIV 5000	  /* usec */
#define TIME_TIMEOUT 5000000	/* usec */
#define SKEY_TIMEOUT 6000000000	/* usec */
#define SKEY_CHG_TIMEOUT 10000000 /* usec */
#define ACK_TIMEOUT 2000000	  /* usec */

/* MaCAN API functions */

int macan_init(int s, const struct macan_sig_spec *sig_spec);
void macan_send_sig(int s, uint8_t sig_num, const struct macan_sig_spec *sig_spec, uint8_t signal);
int macan_wait_for_key_acks(int s, uint64_t *ack_time);

void manage_key(int s);
int init();
#if !defined(TC1798)
void read_can_main(int s);
#endif
int check_cmac(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len);
void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len);
void receive_sig(struct can_frame *cf);
int macan_write(int s, uint8_t dst_id, uint8_t sig_num, uint32_t signal);
int is_channel_ready(uint8_t dst);
int is_skey_ready(uint8_t dst_id);
void receive_auth_req(struct can_frame *cf);
void send_auth_req(int s,uint8_t dst_id,uint8_t sig_num,uint8_t prescaler);
void receive_challenge(int s,struct can_frame *cf);
void send_challenge(int s, uint8_t dst_id, uint8_t fwd_id, uint8_t *chg);
int receive_skey(struct can_frame *cf);
void gen_challenge(uint8_t *chal);
extern uint8_t *key_ptr;
extern uint8_t keywrap[32];
extern uint8_t g_chg[6];
extern uint8_t seq;
void send_ack(int s,uint8_t dst_id);
int receive_ack(struct can_frame *cf);
extern uint8_t skey[24];
#if defined(TC1798)
int write(int s,struct can_frame *cf,int len);
#endif
uint64_t read_time();
uint64_t get_macan_time();
void receive_time(int s, struct can_frame *cf);
void receive_signed_time(int s, struct can_frame *cf);

#endif /* MACAN_H */
