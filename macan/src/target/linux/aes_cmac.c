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
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <nettle/aes.h>
#include <nettle/memxor.h>
#include <common.h>

/**
 * lshift() - left shift by one
 *
 * Can be used in-place.
 */
void lshift(uint8_t *dst, uint8_t *src)
{
	int i;

	for(i = 0; i < 15; i++) {
		dst[i] = (src[i] << 1) | (src[i+1] >> 7);
	}
	dst[15] = src[15] << 1;
}

/**
 * generate_subkey() - generates K1 and K2 for AES-CMAC
 */
void generate_subkey(uint8_t *key, uint8_t *key1, uint8_t *key2)
{
	const uint8_t zero[16] = { 0 };
	uint8_t rb[16] = { 0 };
	uint8_t l[16];
	struct aes_ctx cipher;

	rb[15] = 0x87;
	aes_set_encrypt_key(&cipher, 16, key);
	aes_encrypt(&cipher, 16, l, zero);

	lshift(key1, l);
	if (l[0] & 0x80) {
		memxor(key1, rb, 16);
		key1[15] = 0x87; // to work with VW
	}

	lshift(key2, key1);
	if (key1[0] & 0x80) {
		memxor(key2, rb, 16);
	}
}

/**
 * aes_cmac - calculates CMAC
 * @ctx:    cipher context
 * @length: length of src
 * @dst:    CMAC will be written to (16 bytes)
 * @src:    message to be signed
 *
 * This function calculates cipher-based message authentication code (CMAC) of
 * the given message. Further see RFC 4493.
 */
void aes_cmac(struct aes_ctx *ctx, size_t length, uint8_t *dst, const uint8_t *src)
{
	uint8_t *key;
	uint8_t key1[16], key2[16];
	uint8_t lblock[16] = { 0 };
	size_t block_size = 16;
	uint8_t lblen;
	int itcnt;
	int i;
	int pad_flag = 0;

	memset(dst, 0, 16);
	key = (uint8_t *) ctx->keys;

	if (length == 0) {
		pad_flag = 1;
	}

	itcnt = length / block_size;
	lblen = length % 16;

	if (lblen == 0)
		itcnt--;
	else
		pad_flag = 1;

  	for (i = 0; i < itcnt; i++, src += block_size) {
      		memxor(dst, src, block_size);
      		aes_encrypt(ctx, block_size, dst, dst);
    	}

	generate_subkey(key, key1, key2);

#ifdef DEBUG	
	printf("AES Key1: ");
	print_hexn(key1,16);
	printf("AES Key2: ");
	print_hexn(key2,16);
#endif

	if (pad_flag) {
		memcpy(lblock, src, lblen);
		lblock[lblen] = 0x80;
		memxor(dst, lblock, block_size);
		memxor(dst, key2, block_size);
      		aes_encrypt(ctx, block_size, dst, dst);
	} else {
		memxor(dst, src, block_size);
		memxor(dst, key1, block_size);
      		aes_encrypt(ctx, block_size, dst, dst);
	}
}

