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
#include "can_frame.h"
#include "Std_Types.h"
#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "EcuM.h"
#include "Test_Print.h"
#include "Os.h"
#include "she.h"
#include "macan_config.h"
#include "macan.h"

#define f_STM 100000000
#define TIME_USEC (f_STM / 1000000)

/**
 * write() - sends a can frame
 * @s:   ignored
 * @cf:  a can frame
 * @len: ignored
 *
 * write implemented on top of AUTOSAR functions.
 */
int write(int s, struct can_frame *cf, int len)
{
	/* ToDo: consider some use of PduIdType */
	Can_PduType pdu_info = {17, cf->can_dlc, cf->can_id, cf->data};
	Can_Write(CAN_IF, &pdu_info);

	/* ToDo: redo */
	uint64_t wait = read_time();
	while (wait + 1000 > read_time()) {};

	return 16;
}

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
	uint8_t cmac[16];
	uint64_t time;
	uint32_t *ftime = (uint32_t *)fill_time;
	int i;

	if (!fill_time) {
		aes_cmac(skey, len, cmac, plain);

		return memchk(cmac4, cmac, 4);
	}

	time = get_macan_time(ctx) / TIME_DIV;

	for (i = 0; i >= -1; i--) {
		*ftime = time + i;
		aes_cmac(skey, len, cmac, plain);

		if (memchk(cmac4, cmac, 4) == 1)
			return 1;
	}

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
	uint8_t cmac[16];
	aes_cmac(skey, len, cmac, plain);

	memcpy(cmac4, cmac, 4);
}

/**
 * unwrap_key() - deciphers AES-WRAPed key
 */
void unwrap_key(const uint8_t *key, size_t len, uint8_t *dst, uint8_t *src)
{
	aes_set_key(key);
	aes_unwrap(key, len, dst, src, src);
}

uint64_t read_time()
{
	uint64_t time;
	uint32_t *time32 = (uint32_t *)&time;
	time32[0] = STM_TIM0.U;
	time32[1] = STM_TIM6.U;
	time /= TIME_USEC;

	return time;
}
