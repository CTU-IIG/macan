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
#include "macan_config.h"
#include "macan.h"

void can_recv_cb(int s, struct can_frame *cf);

struct sig_spec macan_sig_spec[] = {
        [ENGINE] = {0, 10, 2, 3, 5},
        [BRAKE]  = {1, 11, 2, 3, 7},
        [TRAMIS] = {2, 12, 3, 2, 5}
};

extern uint8_t ltk[];

/* ToDo: generate this table */
struct com_part cpart[] =
{
	[0]    = {{0}, 0, 0, 0x8 | 0x4},
	[1]    = {{0}, 0, 0, 0x8 | 0x4},
	[2]    = {{0}, 0, 0, 0x8 | 0x4},
	[3]    = {{0}, 0, 0, 0x8 | 0x4}
};

#define SIG_DONTSIGN -1
#define SIG_SIGNONCE 0

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

/**
 * macan_init()
 *
 * Request keys to all comunication partners.
 */
int macan_init(int s)
{
	uint8_t src_id;
	static int i = -1;

	if (i == SIG_MAX)
		return 1;

	if (i != -1) {
		src_id = macan_sig_spec[i].src_id;
		if (!(cpart[src_id].group_id & (1 << NODE_ID)))
			return 0;
	}

	for (i++; i < SIG_MAX; i++) {
		src_id = macan_sig_spec[i].src_id;
		if (macan_sig_spec[i].dst_id != NODE_ID ||
			cpart[src_id].group_id & (1 << NODE_ID) )
			continue;

		send_challenge(s, NODE_KS, macan_sig_spec[i].src_id, g_chg);
		return 0;
	}

	return 0;
}

/**
 * write() - sends a can frame
 * @s:   ignored
 * @cf:  a can frame
 * @len: ignored
 *
 * write implemented on top of AUTOSAR functions.
 * */
#ifdef TC1798
int write(int s, struct can_frame *cf, int len)
{
	/* ToDo: consider some use of PduIdType */
	Can_PduType pdu_info = {17, cf->can_dlc, cf->can_id, cf->data};
	Can_Write(CAN_IF, &pdu_info);

	return 16;
}
#endif

/**
 * check_cmac() - checks a message authenticity
 * @skey:  128-bit session key
 * @cmac4: points to CMAC message part, i.e. 4 bytes CMAC
 * @plain: plain text to be CMACked and checked against
 * @len:   length of plain text in bytes
 *
 * The function computes CMAC of the given plain text and compares
 * it against cmac4. Returns 1 if CMACs matches.
 */
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

/**
 * sign() - signs a message with CMAC
 * @skey:  128-bit session key
 * @cmac4: 4 bytes of the CMAC signature will be written to
 * @plain: a plain text to sign
 * @len:   length of the plain text
 */
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

/**
 * unwrap_key() - deciphers AES-WRAPed key
 */
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

/**
 * receive_ack()
 *
 * Returns 1 if the incoming ack does not contain this node in its
 * group field. Therefore, the updated ack should be broadcasted.
 */
int receive_ack(struct can_frame *cf)
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

	/* ToDo: make difference if wrong CMAC and I dont have the key*/
	if (!check_cmac(skey, ack->cmac, plain, sizeof(plain))) {
		printf("error: ACK CMAC failed\n");
		return -1;
	}

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
 * receive_skey() - session key reception
 *
 * Processes one frame of the session key transmission protocol.
 * Returns node id if a complete key was sucessfully received, otherwise
 * returns -1.
 */
int receive_skey(struct can_frame *cf)
{
	struct sess_key *sk;
	uint8_t fwd_id;
	static uint8_t keywrap[32];
	uint8_t seq, len;

	sk = (struct sess_key *)cf->data;
	seq = sk->seq;
	len = sk->len;

	assert(0 <= seq && seq <= 5);
	assert((seq != 5 && len == 6) || (seq == 5 && len == 2));

	memcpy(keywrap + 6 * seq, sk->data, sk->len);

	if (seq == 5) {
		seq = 0;
		print_hexn(keywrap, 32);

		unwrap_key(ltk, 32, skey, keywrap);
		print_hexn(skey, 24);

		if(!memchk(skey, g_chg, 6)) {
			printf("receive session key \033[1;31mFAIL\033[0;0m\n");
			return -1;
		}

		/* ToDo: check skey[6] */
		fwd_id = skey[6];
		memcpy(cpart[fwd_id].skey, skey + 8, 16);
		cpart[fwd_id].group_id |= 1 << NODE_ID;
		printf("receive session key (%d->%d) \033[1;32mOK\033[0;0m\n", NODE_ID, fwd_id);

		return fwd_id;
	}

	return -1;
}

/**
 * send_challenge() - requests key from KS or signed time from TS
 * @s:      socket fd
 * @dst_id: destination node id (e.g. KS)
 * @fwd_id: id of a node I wish to share a key with
 *          (i.e. communication partner)
 * @chg:    challenge (a 6-byte nonce) will be generated and written
 *          here
 *
 * This function sends the CHALLENGE message to the socket s. It is
 * used to request a key from KS or to request a signed time from TS.
 */
void send_challenge(int s, uint8_t dst_id, uint8_t fwd_id, uint8_t *chg)
{
	struct can_frame cf;
	struct challenge chal = {1, dst_id, fwd_id, {0}};

	gen_challenge(chg);
	memcpy(chal.chg, chg, 6);

	cf.can_id = NODE_ID;
	cf.can_dlc = 8;
	memcpy(cf.data, &chal, sizeof(struct challenge));
	write(s, &cf, sizeof(struct can_frame));
}

/**
 * receive_challenge() - responds to REQ_CHALLENGE from KS
 */
void receive_challenge(int s, struct can_frame *cf)
{
	assert(cf->can_id == NODE_KS);

	struct challenge *ch = (struct challenge *)cf->data;

	send_challenge(s, NODE_KS, ch->fwd_id, g_chg);
}

/**
 *
 */
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

void receive_auth_req(struct can_frame *cf)
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

void send_sig(int s, uint8_t sig_num, uint8_t signal)
{
	uint8_t dst_id;

	dst_id = macan_sig_spec[sig_num].dst_id;

	if (is_channel_ready(dst_id)) {
		macan_write(s, dst_id, sig_num, signal);
	} else
	{
		/* ToDo: send standard can */
	}

	/* ToDo: prescaler counting
	case SIG_DONTSIGN:
		break;
	case SIG_SIGNONCE:
		macan_write(s, dst_id, sig_num, signal);
		signal_sign[sig_num] = SIG_DONTSIGN;
		break;
	default:
		if (signal_cnt[sig_num] == signal_sign[sig_num]) {
			signal_cnt[sig_num] = 0;
		} else {
			signal_cnt[sig_num]++;
		}
		break;
	}
	*/
}

void receive_sig(struct can_frame *cf)
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

/* ToDo: what if overflow? */
#define f_STM 100000000
#define TIME_USEC (f_STM / 1000000)

void read_time(uint64_t *time)
#ifdef TC1798
{
	uint32_t *time32 = (uint32_t *)time;
	time32[0] = STM_TIM0.U;
	time32[1] = STM_TIM6.U;
	*time /= TIME_USEC;
}
#else
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	*time = ts.tv_sec * 1000000;
	*time += ts.tv_nsec / 1000;
}

#endif /* TC1798 */

int init()
#ifdef TC1798
{
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

