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
#include <inttypes.h>
#include "common.h"
#include "aes_keywrap.h"
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <nettle/aes.h>
#include "aes_cmac.h"
#include "macan_private.h"

/**
 * checks a message authenticity
 *
 * The function computes CMAC of the given plain text and compares
 * it against cmac4. Returns 1 if CMACs matches.
 *
 * @param skey  128-bit session key
 * @param cmac4: points to CMAC message part, i.e. 4 bytes CMAC
 * @param plain: plain text to be CMACked and checked against
 * @param len:   length of plain text in bytes
 */
int check_cmac(struct macan_ctx *ctx, uint8_t *skey, const uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len)
{
	struct aes_ctx cipher;
	uint8_t cmac[16];
	uint64_t time;
	uint32_t *ftime = (uint32_t *)fill_time;
	int i,ret;

	aes_set_encrypt_key(&cipher, 16, skey);

	if (!fill_time) {
		aes_cmac(&cipher, len, cmac, plain);
		ret = memchk(cmac4, cmac, 4);
#define DEBUG
#ifdef DEBUG
		if(ret == 0) {
			// check failed, print more info
			printf(ANSI_COLOR_RED "CMAC check failed\n" ANSI_COLOR_RESET);
			printf("plain: "); print_hexn(plain,len);
			printf("session key: "); print_hexn(&cipher,16);
			printf("expected cmac: "); print_hexn(cmac4,4);
			printf("computed cmac: "); print_hexn(&cmac,4);
		}
#endif
		return ret;
	}

	time = macan_get_time(ctx);

	for (i = -1; i <= 1; i++) {
		*ftime = time + i;
		aes_cmac(&cipher, len, cmac, plain);

		if (memchk(cmac4, cmac, 4) == 1) {
			return 1;
		}
	}

	printf(ANSI_COLOR_RED "CMAC check failed\n" ANSI_COLOR_RESET);
	printf("plain: "); print_hexn(plain,len);
	printf("session key: "); print_hexn(&cipher,16);
	printf("expected cmac: "); print_hexn(cmac4,4);
	printf("time ±1: %"PRIu64"\n", time);

	return 0;
}

/**
 * sign() - signs a message with CMAC
 * @skey:  128-bit session key
 * @cmac4: 4 bytes of the CMAC signature will be written to
 * @plain: a plain text to sign
 * @len:   length of the plain text
 */
void sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len)
{
	struct aes_ctx cipher;
	uint8_t cmac[16];

	aes_set_encrypt_key(&cipher, 16, skey);
	aes_cmac(&cipher, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}

/**
 * unwrap_key() - deciphers AES-WRAPed key
 */
void unwrap_key(const uint8_t *key, size_t len, uint8_t *dst, uint8_t *src)
{
	struct aes_ctx cipher;

	aes_set_decrypt_key(&cipher, 16, key);
	aes_unwrap(&cipher, len, dst, src, src);
}

uint64_t read_time()
{
	uint64_t time;
	struct timespec ts;
	static struct timespec buz;
	static uint8_t once = 0;

	/* ToDo: redo this */
	if (once == 0) {
		once = 1;
		clock_gettime(CLOCK_MONOTONIC_RAW, &buz);
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	ts.tv_sec -= buz.tv_sec;
	ts.tv_nsec -= buz.tv_nsec;
	time = ts.tv_sec * 1000000;
	time += ts.tv_nsec / 1000;

	return time;
}
