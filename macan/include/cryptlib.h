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

#include "macan_private.h"

#ifndef CRYPTLIB_H
#define CRYPTLIB_H

void macan_aes_cmac(const uint8_t *key, size_t length, uint8_t *dst, uint8_t *src);
void macan_aes_encrypt(const uint8_t *key, size_t len, uint8_t *dst, const uint8_t *src);
void macan_aes_decrypt(const uint8_t *key, size_t len, uint8_t *dst, const uint8_t *src);
void macan_aes_wrap(const uint8_t *key, size_t length, uint8_t *dst, const uint8_t *src);
int macan_aes_unwrap(const uint8_t *key, size_t length, uint8_t *dst, uint8_t *src, uint8_t *tmp);
int macan_check_cmac(struct macan_ctx *ctx, uint8_t *skey, const uint8_t *cmac4, uint8_t *plain, uint8_t *fill_time, uint8_t len);
void macan_sign(uint8_t *skey, uint8_t *cmac4, uint8_t *plain, uint8_t len);
void macan_unwrap_key(const uint8_t *key, size_t len, uint8_t *dst, uint8_t *src);

#endif /* CRYPTLIB_H */
