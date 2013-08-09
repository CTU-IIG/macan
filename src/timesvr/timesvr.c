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
#include "common.h"
#include "aes_keywrap.h"
#ifdef TC1798
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
#include <nettle/aes.h>
#include "aes_cmac.h"
#endif /* TC1798 */

/* ToDo
 *   implement groups
 *   some error processing
 */


#define WRITE_DELAY 0.5
#define NODE_KS 0
#define NODE_TS 1
/* ToDo: if NODE_ID OTHER error check */
#ifndef CAN_IF
# error CAN_IF not specified
#endif /* CAN_IF */

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

union macan_frame {
	struct crypt_frame cf;
	struct challenge chal;
	struct sess_key skey;
	struct ack ack;
};

/* ltk stands for long term key; it is a key shared with key server */
uint8_t ltk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

#define SIG_DONTSIGN -1
#define SIG_SIGNONCE 0

#define ENGINE 1
#define BRAKE 2
#define TRAMIS 3

int signal[] = {
	[ENGINE] = 7,
	[BRAKE]  = 5,
	[TRAMIS] = 8,
};

int signal_sign[] = {
	[ENGINE] = SIG_DONTSIGN,
	[BRAKE]  = SIG_DONTSIGN,
	[TRAMIS] = SIG_DONTSIGN,
};

uint32_t signal_cnt[] = {
	[ENGINE] = 0,
	[BRAKE] = 0,
	[TRAMIS] = 0,
};

/* channel states */
#define CS_UNINIT 0
#define CS_HAS_SKEY 1
#define CS_ACKED 2
#define CS_READY 3

/*
 * struct com_part - communication partner
 * @id:    6-bit id
 * @skey:  wrapped key or consequently session key
 * @stat:  comunication channel status
 */
struct com_part {
	uint8_t skey[16];
	uint32_t group_id;
	uint32_t wait_for;
};

struct com_part cpart[] =
{
	[NODE_KS]    = {{0}, 0, 0},
	[NODE_TS]    = {{0}, 0, 0},
	[NODE_OTHER] = {{0}, 0, 0x8 | 0x4},
	[NODE_ID]    = {{0}, 0, 0}
};

#ifdef TC1798
int write(int s, struct can_frame *cf, int len)
{
	/* ToDo: consider some use of PduIdType */
	Can_PduType PduInfo_test = {17, cf->can_dlc, cf->can_id, cf->data};
	Can_Write(CAN_IF, &PduInfo_test);

	return 16; /* :-) */
}
#endif

int check_cmac(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len)
#ifdef TC1798
{
	uint8_t cmac[16];
	aes_cmac(skey, len, cmac, plain);

	return memchk(cmac4, cmac, 4);
}
#else
{
	struct aes_ctx cipher;
	uint8_t cmac[16];

	aes_set_encrypt_key(&cipher, 16, skey);
	aes_cmac(&cipher, len, cmac, plain);

	return memchk(cmac4, cmac, 4);
}
#endif /* TC1798 */

void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len)
#ifdef TC1798
{
	uint8_t cmac[16];
	aes_cmac(skey, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}
#else
{
	struct aes_ctx cipher;
	uint8_t cmac[16];

	aes_set_encrypt_key(&cipher, 16, skey);
	aes_cmac(&cipher, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}
#endif /* TC1798 */

void unwrap_key(uint8_t *key, size_t len, uint8_t *dst, uint8_t *src)
#ifdef TC1798
{
	aes_set_key(key);
	aes_unwrap(key, len, dst, src, src);
}
#else
{
	struct aes_ctx cipher;

	aes_set_decrypt_key(&cipher, 16, key);
	aes_unwrap(&cipher, len, dst, src, src);
}
#endif /* TC1798 */

int process_ack(struct can_frame *cf)
{
	uint8_t id;
	struct com_part *cp;
	struct ack *ack = (struct ack *)cf->data;
	uint8_t plain[8];
	uint32_t time = 0;
	uint8_t *skey;

	id = cf->can_id;
	assert(id < (sizeof(cpart) / sizeof(struct com_part)));

	/* ToDo: overflow check */
	/* ToDo: what if ack contains me */
	/* ToDo: add groups communication */
	cp = &cpart[id];
	skey = cp->skey;

	memcpy(plain, &time, 4);
	plain[4] = ack->dst_id;
	memcpy(plain + 5, ack->group, 3);

	if (!check_cmac(skey, ack->cmac, plain, sizeof(plain))) {
		printf("error: ACK CMAC failed\n");
		return -1;
	}

	/* ToDo: Redo and then redo again */
	uint32_t is_present = 1 << NODE_ID;
	uint32_t ack_group = 0;
	memcpy(&ack_group, ack->group, 3);

	is_present &= ack_group;
	cp->group_id |= ack_group;

	if (is_present)
		return 0;

	return 1;
}

void send_ack(int s, uint8_t dst_id)
{
	struct ack ack = {2, dst_id, {0}, {0}};
	uint8_t plain[8] = {0};
	uint32_t time = 0;
	uint8_t *skey;
	struct can_frame cf;
	volatile int res;

	skey = cpart[dst_id].skey;
	memcpy(&ack.group, &cpart[dst_id].group_id, 3);

	memcpy(plain, &time, 4);
	plain[4] = ack.dst_id;
	memcpy(plain + 5, ack.group, 3);

	sign(skey, ack.cmac, plain, sizeof(plain));

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(cf.data, &ack, 8);

	res = write(s, &cf, sizeof(struct can_frame));
	if (res != 16) {
		perror("send ack");
	}

	return;
}

/* ToDo: aggregate to struct */
uint8_t seq = 0;
uint8_t g_fwd_id = 0;
uint8_t g_chg[6];
uint8_t keywrap[32];
uint8_t *key_ptr = keywrap;
uint8_t skey[24];

void gen_challenge(uint8_t *chal)
{
	int i;

	for (i = 0; i < 6; i++)
		chal[i] = rand();
}

/**
 * process_skey() - session key reception
 *
 * Processes one frame of session key transmission protocol.
 */
int process_skey(struct can_frame *cf)
{
	struct sess_key *sk;

	sk = (struct sess_key *)cf->data;

	assert(sk->seq == seq);
	assert(sk->len >= 0  && sk->len <= 6);

	memcpy(key_ptr, sk->data, sk->len);
	key_ptr += sk->len;
	seq++;

	if (seq == 6) {
		seq = 0;
		key_ptr = keywrap;
		printf("sess key is complete\n");
		print_hexn(keywrap, 32);

		printf("unwrapping\n");
		unwrap_key(ltk, 32, skey, keywrap);
		print_hexn(skey, 24);

		if(!memchk(skey, g_chg, 6)) {
			printf("challenge fail!\n");
			return -1;
		}
		printf("challenge OK!\n");

		/* ToDo: check skey[6] */
		assert(g_fwd_id == skey[6]);
		memcpy(cpart[g_fwd_id].skey, skey + 8, 16);

		cpart[g_fwd_id].group_id |= 1 << NODE_ID;

		return g_fwd_id;
	}

	return -1;
}

void send_challenge(int s, uint8_t fwd_id)
{
	struct can_frame cf;
	struct challenge chal = {1, NODE_KS, fwd_id, {0}};

	gen_challenge(g_chg);
	memcpy(chal.chg, g_chg, 6);

	g_fwd_id = fwd_id;

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(cf.data, &chal, sizeof(struct challenge));
	write(s, &cf, sizeof(struct can_frame));
}

struct timespec ts_base;
uint64_t last_usec;

void process_challenge(int s, struct can_frame *cf)
{
	struct can_frame canf;
	struct challenge *ch = (struct challenge *)cf->data;
	uint8_t plain[10];
	uint8_t cmac4[4];

	memcpy(plain, ch->chg, 6);
	memcpy(plain + 6, &last_usec, 4);
	/* ToDo: this should not be ltk */

	canf.can_id = NODE_TS;
	canf.can_dlc = 8;
	memcpy(canf.data, &last_usec, 4);
	sign(ltk, canf.data + 4, plain, 10);

	write(s, &canf, sizeof(canf));

	printf("Signed ts sent.\n");
}

void send_auth_req(int s, uint8_t dst_id, uint8_t sig_num, uint8_t prescaler)
{
	uint32_t t = 0;
	uint8_t plain[8];
	uint8_t *skey;
	struct can_frame cf;
	struct sig_auth_req areq;

	skey = cpart[dst_id].skey;

	memcpy(plain, &t, 4);
	plain[4] = NODE_ID;
	plain[5] = dst_id;
	plain[6] = sig_num;
	plain[7] = prescaler;

	areq.flags = 3;
	areq.dst_id = dst_id;
	areq.sig_num = sig_num;
	areq.prescaler = prescaler;
	sign(skey, areq.cmac, plain, sizeof(plain));

	cf.can_id = NODE_ID;
	cf.can_dlc = 7;
	memcpy(&cf.data, &areq, 7);

	write(s, &cf, sizeof(cf));
}

void process_sig_auth(struct can_frame *cf)
{
	uint32_t t;
	uint8_t *skey;
	uint8_t plain[8];
	struct sig_auth_req *areq;

	areq = (struct sig_auth_req *)cf->data;
	/* ToDo: check */
	skey = cpart[cf->can_id].skey;

	memcpy(plain, &t, 4);
	plain[4] = cf->can_id;
	plain[5] = NODE_ID;
	plain[6] = areq->sig_num;
	plain[7] = areq->prescaler;

	if (!check_cmac(skey, areq->cmac, plain, sizeof(plain))) {
		printf("error: sig_auth cmac is incorrect\n");
	}

	/* ToDo: assert sig_num range */
	signal_sign[areq->sig_num] = areq->prescaler;
}

void process_sig_recv(struct can_frame *cf)
{
	struct signal_ex *sig;
	uint8_t plain[8];
	uint8_t *skey;
	uint32_t t = 0;

	sig = (struct signal_ex *)cf->data;

	memcpy(plain, &t, 4);
	plain[4] = cf->can_id;
	plain[5] = NODE_ID;
	memcpy(plain + 6, sig->signal, 2);

	skey = cpart[cf->can_id].skey;

	if (!check_cmac(skey, sig->cmac, plain, sizeof(plain))) {
		printf("CMAC error");
		return;
	}

	sig = (struct signal_ex *)cf->data;
	printf("received authorized sig=%i\n", sig->signal[0]);
}

int is_channel_ready(uint8_t dst)
{
	uint32_t grp = (*((uint32_t *)&cpart[dst].group_id)) & 0x00ffffff;
	uint32_t wf = (*((uint32_t *)&cpart[dst].wait_for)) & 0x00ffffff;

	return ((grp & wf) == wf);
}

void can_recv_cb(int s, struct can_frame *cf)
{
	struct crypt_frame *cryf = (struct crypt_frame *)cf->data;
	int fwd;

	/* ToDo: check if challege message */
	process_challenge(s, cf);
}

#ifndef TC1798
void read_can_main(int s)
{
	struct can_frame cf;
	int rbyte;

	rbyte = read(s, &cf, sizeof(struct can_frame));
	if (rbyte == -1 && errno == EAGAIN) {
		return;
	}

	if (rbyte != 16) {
		printf("ERROR recv not 16 bytes");
		exit(0);
	}

	printf("RECV ");
	print_hexn(&cf, sizeof(struct can_frame));
	can_recv_cb(s, &cf);
}
#endif

int macan_write(int s, uint8_t dst_id, uint8_t sig_num, uint32_t signal)
{
	struct can_frame cf;
	uint8_t plain[8];
	uint32_t t = 0;
	uint8_t *skey;
	struct signal_ex sig = {
		3, dst_id, sig_num, {0}, {0}
	};

	if (!is_channel_ready(dst_id))
		return -1;

	skey = cpart[dst_id].skey;

	memcpy(plain, &t, 4);
	plain[4] = NODE_ID;
	plain[5] = dst_id;
	memcpy(plain + 6, &signal, 2);

	memcpy(&sig.signal, &signal, 2);
	sign(skey, sig.cmac, plain, sizeof(plain));

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(&cf.data, &sig, 8);

	/* ToDo: assure success */
	write(s, &cf, sizeof(cf));

	return 0;
}

void dispatch_signal(int s, uint8_t dst_id, uint8_t sig_num, uint8_t signal)
{
	switch (signal_sign[sig_num]) {
	case SIG_DONTSIGN:
		/* ToDo: send standard can */
		break;
	case SIG_SIGNONCE:
		macan_write(s, dst_id, sig_num, signal);
		signal_sign[sig_num] = SIG_DONTSIGN;
		break;
	default:
		if (signal_cnt[sig_num] == signal_sign[sig_num]) {
			signal_cnt[sig_num] = 0;
			macan_write(s, dst_id, sig_num, signal);
		} else {
			signal_cnt[sig_num]++;
		}
		break;
	}
}

void read_signals()
{

}

void usec_since(uint64_t *usec)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

	*usec = (ts.tv_sec - ts_base.tv_sec) * 1000000;
	*usec += (ts.tv_nsec - ts_base.tv_nsec) / 1000;
}

void broadcast_time(int s)
{
	struct can_frame cf;
	uint64_t usec;

	usec_since(&usec);
	last_usec = usec;

	cf.can_id = NODE_ID;
	cf.can_dlc = 4;
	memcpy(cf.data, &usec, 4);

	write(s, &cf, sizeof(cf));
}

void operate_ts(int s)
{
	int scom = 0;

	while(1) {
		read_can_main(s);
		broadcast_time(s);

		usleep(500000);
	}
		//write_can(s, &cf, &write_pending);
}

int init()
#ifdef TC1798
{
	Test_Time_SetAlarm(0, 0, 100000, tim_handler);
	Can_SetControllerMode(CAN_CONTROLLER0, CAN_T_START);
	//Can_SetControllerMode(CAN_CONTROLLER1, CAN_T_START);

	/* activate SHE */
	/* ToDo: revisit */
	SHE_CLC &= ~(0x1);
	while (SHE_CLC & 0x2) {};
	wait_until_done();

	return 0;
}
#else
{
	int s;
	int r;
	struct ifreq ifr;
	char *ifname = CAN_IF;
	struct sockaddr_can addr;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	r = fcntl(s, F_SETFL, O_NONBLOCK);
	if (r != 0) {
		perror("ioctl fail");
		return -1;
	}

	int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}

	return s;
}
#endif

int main(int argc, char *argv[])
{
	int s;

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts_base);
	s = init();
	operate_ts(s);

	return 0;
}
