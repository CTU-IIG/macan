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

#define NODE_KS 0
#define NODE_TS 1
#define SIG_TIME 4
#define TIME_DELTA 1000   /* tolerated time divergency from TS in usecs */
#define TIME_DIV 5000
#define TIME_TIMEOUT 5000000

struct challenge {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t fwd_id : 8;
	uint8_t chg[6];
};

struct sess_key {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t seq : 4;
	uint8_t len : 4;
	uint8_t data[6];
};

struct ack {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t group[3];
	uint8_t cmac[4];
};

struct crypt_frame {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
};

struct sig_auth_req {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t prescaler;
	uint8_t cmac[4];
};

struct signal {
	uint8_t sig[4];
	uint8_t cmac[4];
};

struct signal_ex {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t signal[2];
	uint8_t cmac[4];
};

struct macan_time {
	uint64_t sync;       /* contains the time difference between local time
   			        and TS time;
				i.e. TS_time = Local_time + sync */
	uint64_t chal_ts;    /* local timestamp when request for signed time was sent  */
	uint8_t chg[6];
};

/*
 * struct com_part - communication partner
 * @id:    6-bit id
 * @skey:  wrapped key or consequently session key
 * @stat:  comunication channel status
 */
struct com_part {
	uint8_t skey[16];
	uint8_t flags;
	uint32_t group_id;
	uint32_t wait_for;
};

int macan_init(int s);
int init();
#if !defined(TC1798)
void read_can_main(int s);
#endif
int check_cmac(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len);
void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len);
void receive_sig(struct can_frame *cf);
void send_sig(int s,uint8_t sig_num,uint8_t signal);
int is_channel_ready(uint8_t dst);
int macan_write(int s,uint8_t dst_id,uint8_t sig_num,uint32_t signal);
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

