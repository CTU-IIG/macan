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
#include <stdint.h>
#include <macan_private.h>

int canid2ecuid(struct macan_ctx *ctx, uint32_t canid)
{
	int i;

	for (i = 0; i < ctx->config->node_count; i++) {
		if (ctx->config->ecu2canid[i] == canid)
			return i;
	}
	return -1;
}

void print_hexn(void *data, size_t len)
{
	int i;
	uint8_t *d = data;

	for (i = 0; i < len; i++) {
		printf("%02x", d[i]);
		if ((i % 4) == 3)
			printf(" ");
	}
	printf("\n");
}

int memchk(uint8_t *a, uint8_t *b, size_t len)
{
	int i;

	for (i = 0; i < len; i++) {
		if (a[i] != b[i])
			return 0;
	}

	return 1;
}

void eval(const char *tname, int b)
{
	static uint32_t tcnt = 1;

	printf("\033[1;30m%i) %-32s\033[0m: ", tcnt, tname);
	if (b)
		printf("\033[1;32mOK\033[0m\n");
	else
		printf("\033[1;31mFAIL\033[0m\n");

	tcnt++;
}

/**
 * memcpy_bw() - memcpy byte-wise */
void memcpy_bw(void *dst, const void *src, size_t len) {
        int i;
        uint8_t *dst8 = dst;
        const uint8_t *src8 = src;

        for (i = len; i--;)
                dst8[i] = src8[i];
}
