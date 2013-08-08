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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <nettle/aes.h>
#include "common.h"
#include "aes_keywrap.h"

/**
 * ToDo
 *   redo assertions, they should not stop the program
 */

/* ToDo: this should be moved to some config .h */
#define WRITE_DELAY 500000
#define NODE_KS 0
#define NODE_TS 1
#define NODE_ID NODE_KS

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

struct crypt_frame {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
};

uint8_t ltk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

#define NODE_TOTAL 4
uint8_t skey_map[NODE_TOTAL - 1][NODE_TOTAL][16] = {
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
	{{0},{0},{0},{0}},
};

void generate_skey(uint8_t *skey)
{
	int i;

	for (i = 0; i < 16; i++)
		skey[i] = rand();
}

uint8_t lookup_skey(uint8_t src, uint8_t dst, uint8_t **skey)
{
	int i;
	uint8_t tmp;

	assert(src != dst);

	if (src > dst) {
		tmp = src;
		src = dst;
		dst = tmp;
	}

	*skey = skey_map[src][dst];

	/* ToDo: something faster? */
	for (i = 0; i < 16; i++) {
		if ((*skey)[i]) {
			break;
		}
	}

	if (i == 16) {
		generate_skey(*skey);
		return 1;
	}

	return 0;
}

void send_req_chal(int s, int dst_id, int fwd_id)
{
	struct can_frame cf;
	struct challenge chal = {1, dst_id, fwd_id, {0}};

	cf.can_id = NODE_KS;
	cf.can_dlc = 2;
	memcpy(cf.data, &chal, sizeof(struct challenge));

	/* ToDo: ensure */
	usleep(WRITE_DELAY);
	write(s, &cf, sizeof(struct can_frame));
}

void send_skey(int s, struct aes_ctx * cipher, uint8_t dst_id, uint8_t fwd_id, uint8_t *chal)
{
	uint8_t wrap[32];
	uint8_t plain[24];
	uint8_t *key;
	struct can_frame cf;
	struct sess_key skey;
	int i;

	/* ToDo: solve name inconsistency - key */
	if (lookup_skey(dst_id, fwd_id, &key))
		send_req_chal(s, fwd_id, dst_id);

	memcpy(plain, chal, 6);
	plain[6] = fwd_id;
	plain[7] = dst_id;
	memcpy(plain + 8, key, 16);
	printf("plain is as:\n");
	print_hexn(plain, 24);

	aes_wrap(cipher, 24, wrap, plain);
	printf("wrapped as:\n");
	print_hexn(wrap, 32);

	skey.flags = 2;
	skey.dst_id = dst_id; /* ToDo: ecui or j, reconsider names */

	cf.can_id = NODE_KS;
	cf.can_dlc = 8;

	for (i = 0; i < 6; i++) {
		skey.seq = i;
		skey.len = (i == 5) ? 2 : 6;
		memcpy(skey.data, wrap + (6 * i), 6);
		memcpy(cf.data, &skey, 8);

		/* ToDo: check for success */
		usleep(WRITE_DELAY);
		write(s, &cf, sizeof(struct can_frame));
	}

}

/**
 * process_challenge() - responds to challenge message
 * @s:   socket fd
 * @cf:  received can frame
 *
 * This function responds with session key to the challenge sender
 * and also sends REQ_CHALLENGE to communication partner of the sender.
 */
void process_challenge(int s, struct can_frame *cf)
{
	struct aes_ctx cipher;
	struct challenge *chal;
	uint8_t dst_id, fwd_id;
	uint8_t *chg;

	aes_set_encrypt_key(&cipher, 16, ltk);
	chal = (struct challenge *)cf->data;

	dst_id = cf->can_id;
	fwd_id = chal->fwd_id;
	chg = chal->chg;

	send_skey(s, &cipher, dst_id, fwd_id, chg);
}

void can_recv_cb(int s, struct can_frame *cf)
{
	struct crypt_frame *cryf = (struct crypt_frame *)cf->data;

	if (cryf->dst_id != NODE_ID)
		return;
	//printf("flags=%d, dst_id=%d\n", cryf->flags, a->dst_id);

	/* ToDo: do some check on challenge message, the only message recepted by KS */
	process_challenge(s, cf);
}

void read_can_main(int s)
{
	struct can_frame cf;
	int rbyte;

	/* ToDo: what is read */
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

int main(int argc, char *argv[])
{
	int s;
	struct sockaddr_can addr;
	struct ifreq ifr;

	char *ifname = "can0";

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
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

	while (1) {
		read_can_main(s);

		usleep(250);
	}

	return 0;
}

