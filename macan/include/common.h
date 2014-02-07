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

#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <string.h>

#define print_hex(key) print_hexn(key, 16)
#define check128(a, b) memchk(a, b, 16)

/* color definitions for output */
#define ANSI_COLOR_RED     "\033[1;31m"
#define ANSI_COLOR_GREEN   "\033[1;32m"
#define ANSI_COLOR_YELLOW  "\033[1;33m"
#define ANSI_COLOR_BLUE    "\033[1;34m"
#define ANSI_COLOR_MAGENTA "\033[1;35m"
#define ANSI_COLOR_CYAN    "\033[1;36m"
#define ANSI_COLOR_LGRAY   "\033[0;37m"
#define ANSI_COLOR_DGRAY   "\033[1;30m"
#define ANSI_COLOR_DBLUE   "\033[0;34m"
#define ANSI_COLOR_DCYAN   "\033[0;36m"
#define ANSI_COLOR_RESET   "\033[0;0m"

typedef enum {
	MSG_OK,
	MSG_WARN,
	MSG_FAIL,
	MSG_REQUEST,
	MSG_INFO,
	MSG_SIGNAL
} msg_type;

void eval(const char *tname,int b);
int memchk(const uint8_t *a, const uint8_t *b, size_t len);
void print_hexn(const void *data, size_t len);
void memcpy_bw(void *dst, const void *src, size_t len);
void print_msg(msg_type type, const char *format, ...);
void lshift(uint8_t *dst, const uint8_t *src);

#endif /* COMMON_H */
