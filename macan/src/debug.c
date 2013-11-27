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

#include <debug.h>
#include <stdio.h>
#include <stdarg.h>
#include "macan_private.h"
#include "target/linux/lib.h"
#include <string.h>

void debug_printf(const char* format, ...)
{
    if (format)
    {
        va_list argp;
        va_start(argp, format);
        vfprintf(stderr, format, argp);
        va_end(argp);
    }
}


void print_frame(struct macan_ctx *ctx, struct can_frame *cf)
{
	char frame[80], comment[80];
	int8_t src;
	comment[0] = 0;
	sprint_canframe(frame, cf, 0, 8);
	if (ctx && ctx->config) {
		if (cf->can_id == ctx->config->can_id_time)
			sprintf(comment, "time %ssigned", cf->can_dlc == 4 ? "un" : "");
		else if ((src = canid2ecuid(ctx, cf->can_id)) >= 0) {
			/* Crypt frame */
			if (cf->can_dlc < 8) {
				sprintf(comment, "broken crypt frame");
			} else {
				struct macan_crypt_frame *crypt = (struct macan_crypt_frame*)cf->data;
				char *type;
				switch (crypt->flags) {
				case FL_CHALLENGE:
					type = "challenge";
					break;
				case FL_SESS_KEY:
					if (src == ctx->config->key_server_id)
						type = "sess_key";
					else
						type = "ack";
					break;
				case FL_SIGNAL:
					type = "signal";
					break;
				default:
					type = "???";
				}
				char srcstr[5], dststr[5];
				if (src == ctx->config->key_server_id)       strcpy(srcstr, "KS");
				else if (src == ctx->config->time_server_id) strcpy(srcstr, "TS");
				else sprintf(srcstr, "%d", src);
				if (src == ctx->config->node_id) strcat(srcstr, "me");

				int8_t dst = crypt->dst_id;
				if (dst == ctx->config->key_server_id)       strcpy(dststr, "KS");
				else if (dst == ctx->config->time_server_id) strcpy(dststr, "TS");
				else sprintf(dststr, "%d", dst);
				if (dst == ctx->config->node_id) strcat(dststr, "me");


				sprintf(comment, "crypt %s->%s (%d->%d): %s", srcstr, dststr, src, dst, type);
			}
		}
	}
	printf("RECV %-20s %s\n", frame, comment);
}
