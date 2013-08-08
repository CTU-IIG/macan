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
#include <nettle/aes.h>
#include <nettle/memxor.h>
#include "aes_keywrap.h"
#include "common.h"

void test()
{
	struct aes_ctx cipher;
	uint8_t key[] = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
	};
	unsigned char msg[24] = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07
	};
	uint8_t ciphertext[32];
	uint8_t out[24];
	uint8_t tmp[32];

	/* wrap 128 bits with 128-bit KEK */
	uint8_t cipher128_chk[24] = { 0x1F, 0xA6, 0x8B, 0x0A, 0x81, 0x12, 0xB4, 0x47,
				      0xAE, 0xF3, 0x4B, 0xD8, 0xFB, 0x5A, 0x7B, 0x82,
				      0x9D, 0x3E, 0x86, 0x23, 0x71, 0xD2, 0xCF, 0xE5 };
	aes_set_encrypt_key(&cipher, 16, key);
	aes_wrap(&cipher, 16, ciphertext, msg);
	print_hexn(ciphertext, 24);
	eval("wrap 128 bits with 128-bit KEK", memchk(ciphertext, cipher128_chk, 24));

	/* unwrap 128 bits with 128-bit KEK */
	aes_set_decrypt_key(&cipher, 16, key);
	aes_unwrap(&cipher, 24, out, ciphertext, tmp);
	eval("unwrap 128 bits with 128-bit KEK", memchk(out, msg, 16));

	/* wrap 128 bits with 192-bit KEK */
	uint8_t cipher192_chk[24] = {
		0x96, 0x77, 0x8B, 0x25, 0xAE, 0x6C, 0xA4, 0x35,
		0xF9, 0x2B, 0x5B, 0x97, 0xC0, 0x50, 0xAE, 0xD2,
		0x46, 0x8A, 0xB8, 0xA1, 0x7A, 0xD8, 0x4E, 0x5D
	};
	aes_set_encrypt_key(&cipher, 24, key);
	aes_wrap(&cipher, 16, ciphertext, msg);
	print_hexn(ciphertext, 24);
	eval("wrap 128 bits with 192-bit KEK", memchk(ciphertext, cipher192_chk, 24));

	/* unwrap 128 bits with 192-bit KEK */
	aes_set_decrypt_key(&cipher, 24, key);
	aes_unwrap(&cipher, 24, out, ciphertext, tmp);
	eval("unwrap 128 bits with 192-bit KEK", memchk(out, msg, 16));

	/* wrap 192 bits with 192-bit KEK */
	uint8_t cipher192t192k_chk[32] = {
		0x03, 0x1D, 0x33, 0x26, 0x4E, 0x15, 0xD3, 0x32,
		0x68, 0xF2, 0x4E, 0xC2, 0x60, 0x74, 0x3E, 0xDC,
		0xE1, 0xC6, 0xC7, 0xDD, 0xEE, 0x72, 0x5A, 0x93,
		0x6B, 0xA8, 0x14, 0x91, 0x5C, 0x67, 0x62, 0xD2
	};

	aes_set_encrypt_key(&cipher, 24, key);
	aes_wrap(&cipher, 24, ciphertext, msg);
	print_hexn(ciphertext, 32);
	eval("wrap 192 bits with 192-bit KEK", memchk(ciphertext, cipher192t192k_chk, 32));

	/* unwrap 192 bits with 192-bit KEK */
	aes_set_decrypt_key(&cipher, 24, key);
	aes_unwrap(&cipher, 32, out, ciphertext, ciphertext);
	print_hexn(ciphertext, 32);
	eval("unwrap 192 bits with 192-bit KEK", memchk(out, msg, 24));
}

int main(int argc, char *argv[])
{
	test();

	return 0;
}

