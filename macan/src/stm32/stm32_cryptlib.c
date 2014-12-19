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
#include "aes.h"
#include "memxor.h"
#include <common.h>
#include "cryptlib.h"

static void lshift(uint8_t *dst, const uint8_t *src);
static void generate_subkey(const struct macan_key *key, uint8_t *key1, uint8_t *key2);
/**
 * lshift() - left shift 16 bytes by one bit
 * Can be used in-place.
 *
 * @param[out] dst Destination pointer
 * @param[in]  src Source pointer
 */
static void lshift(uint8_t *dst, const uint8_t *src)
{
	/* TODO: This is not Linux specific - move it to the common code */
	/* Ondrej K.: The function itself, is not Linux specific, but is used exclusively
	 * by function generate_subkey which is Linux specific. So why to move it
	 * to common code? I think it should be internal in this file. No need to use
	 * it anywhere else. */
	int i;

	for(i = 0; i < 15; i++) {
		dst[i] = (uint8_t)((src[i] << 1) | (src[i+1] >> 7));
	}
	dst[15] = (uint8_t)(src[15] << 1);
}

/**
 * generate_subkey() - generates K1 and K2 for AES-CMAC
 */
static void generate_subkey(const struct macan_key *key, uint8_t *key1, uint8_t *key2)
{
	const uint8_t zero[16] = { 0 };
	uint8_t rb[16] = { 0 };
	uint8_t l[16];

	rb[15] = 0x87;
	macan_aes_encrypt(key, 16, l, zero);

	lshift(key1, l);
	if (l[0] & 0x80) {
		memxor(key1, rb, 16);
		//key1[15] = 0x87; // to work with VW
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
void macan_aes_cmac(const struct macan_key *key, size_t length, uint8_t *dst, uint8_t *src)
{
	uint8_t key1[16], key2[16];
	uint8_t lblock[16] = { 0 };
	size_t block_size = 16;
	uint8_t lblen;
	size_t itcnt;
	size_t i;
	int pad_flag = 0;

	memset(dst, 0, 16);

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
		macan_aes_encrypt(key, (unsigned)block_size, dst, dst);
	}

	generate_subkey(key, key1, key2);

	if (pad_flag) {
		memcpy(lblock, src, lblen);
		lblock[lblen] = 0x80;
		memxor(dst, lblock, block_size);
		memxor(dst, key2, block_size);
		macan_aes_encrypt(key, (unsigned)block_size, dst, dst);
	} else {
		memxor(dst, src, block_size);
		memxor(dst, key1, block_size);
		macan_aes_encrypt(key, (unsigned)block_size, dst, dst);
	}
}

/*
 * macan_aes_encrypt() - encrypt block of data
 */
void macan_aes_encrypt(const struct macan_key *key, size_t len, uint8_t *dst, const uint8_t *src)
{
	struct aes_ctx cipher;

	aes_set_encrypt_key(&cipher, 16, key->data);
	aes_encrypt(&cipher, (unsigned)len, dst, src);
}

/*
 * macan_aes_decrypt() - decrypt block of data
 */
void macan_aes_decrypt(const struct macan_key *key, size_t len, uint8_t *dst, const uint8_t *src)
{
	struct aes_ctx cipher;

	aes_set_decrypt_key(&cipher, 16, key->data);
	aes_decrypt(&cipher, (unsigned)len, dst, src);
}

