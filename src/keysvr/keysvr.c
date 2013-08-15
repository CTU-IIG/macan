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
#include "macan.h"

/**
 * ToDo
 *   redo assertions, they should not stop the program
 */

/* ToDo: this should be moved to some config .h */
#define WRITE_DELAY 500000

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
		send_challenge(s, fwd_id, dst_id, NULL);

	memcpy(plain, chal, 6);
	plain[6] = fwd_id;
	plain[7] = dst_id;
	memcpy(plain + 8, key, 16);
	aes_wrap(cipher, 24, wrap, plain);

	printf("send KEY (wrap, plain):\n");
	print_hexn(wrap, 32);
	print_hexn(plain, 24);

	skey.flags = 2;
	skey.dst_id = dst_id;

	cf.can_id = NODE_KS;
	cf.can_dlc = 8;

	for (i = 0; i < 6; i++) {
		skey.seq = i;
		skey.len = (i == 5) ? 2 : 6;
		memcpy(skey.data, wrap + (6 * i), 6);
		memcpy(cf.data, &skey, 8);

		/* ToDo: check all writes for success */
		usleep(WRITE_DELAY);
		write(s, &cf, sizeof(struct can_frame));
	}
}

/**
 * ks_receive_challenge() - responds to challenge message
 * @s:   socket fd
 * @cf:  received can frame
 *
 * This function responds with session key to the challenge sender
 * and also sends REQ_CHALLENGE to communication partner of the sender.
 */
void ks_receive_challenge(int s, struct can_frame *cf)
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

	/* ToDo: do some filter here */
	if (cf->can_id == SIG_TIME)
		return;
	if (cryf->dst_id != NODE_KS)
		return;

	/* ToDo: do some check on challenge message, the only message recepted by KS */
	ks_receive_challenge(s, cf);
}

int main(int argc, char *argv[])
{
	int s;

	s = init();

	while (1) {
		read_can_main(s);

		usleep(250);
	}

	return 0;
}

