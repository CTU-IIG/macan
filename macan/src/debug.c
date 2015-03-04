/*
 *  Copyright 2014, 2015 Czech Technical University in Prague
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

#include <macan_debug.h>
#include <stdio.h>
#include <stdarg.h>
#include "macan_private.h"
#include "linux/lib.h"		/* FIXME: target specific file should not be here */
#include <string.h>
#include <inttypes.h>

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

#define GET_SEQ(byte) (((byte) & 0xF0) >> 4)
#define GET_LEN(byte) ((byte) & 0x0F)

static void get_ecuid_str(const struct macan_ctx *ctx, char *str, size_t size, macan_ecuid id)
{
	if (id < ctx->config->node_count) {
		if (ctx->config->canid->ecu[id].name) {
			snprintf(str, size, "%s", ctx->config->canid->ecu[id].name);
		} else {
			if (id == ctx->config->key_server_id)
				snprintf(str, size, "KS");
			else if (id == ctx->config->time_server_id)
				snprintf(str, size, "TS");
			else
				snprintf(str, size, "%02u", id);
		}
		if (id == ctx->node->node_id) {
			size_t l = strlen(str);
			snprintf(str + l, size-l, "(me)");
		}
	} else
		snprintf(str, size, "%02u (bad!)", id);
}

void print_frame(const struct macan_ctx *ctx, struct can_frame *cf, const char *prefix)
{
	char frame[80], comment[80];
	macan_ecuid src;
	const char *color = "";
	comment[0] = 0;
	sprint_canframe(frame, cf, 0, 8);
	if (ctx) {
		if (cf->can_id == ctx->config->canid->time) {
			uint32_t time;
			memcpy(&time, cf->data, 4); /* FIXME: Handle endian */
			switch (cf->can_dlc) {
			case 4: color = ANSI_COLOR_LGRAY;
				sprintf(comment, "time %u", time);
				break;
			case 8: color = ANSI_COLOR_DGRAY;
				sprintf(comment, "authenticated time %u", time);
				break;
			default:
				sprintf(comment, "broken time!!!");
			}
		}
		else if (macan_canid2ecuid(ctx->config, cf->can_id, &src)) {
			/* Crypt frame */
			if (cf->can_dlc < 2) {
				sprintf(comment, "broken crypt frame");
			} else {
				char type[80];
				switch (macan_crypt_flags(cf)) {
				case FL_REQ_CHALLENGE: {
					struct macan_req_challenge *chg = (struct macan_req_challenge*)cf->data;
					char fwdstr[20];
					get_ecuid_str(ctx, fwdstr, sizeof(fwdstr), chg->fwd_id);
					sprintf(type, "req challenge fwd_id=%s%s", fwdstr,
						cf->can_dlc == 2 ? "" : " wrong length");
					break;
				}
				case FL_CHALLENGE: {
					struct macan_challenge *chg = (struct macan_challenge*)cf->data;
					char fwdstr[20];
					get_ecuid_str(ctx, fwdstr, sizeof(fwdstr), chg->fwd_id);
					sprintf(type, "challenge fwd_id=%s", fwdstr);
					color = ANSI_COLOR_DCYAN;
					break;
				}
				case FL_SESS_KEY_OR_ACK:
					if (src == ctx->config->key_server_id) {
						struct macan_sess_key *sk = (struct macan_sess_key*)cf->data;
						color = ANSI_COLOR_DBLUE;
						sprintf(type, "sess_key seq=%d len=%d", GET_SEQ(sk->seq_and_len), GET_LEN(sk->seq_and_len));
					} else {
						struct macan_ack *ack = (struct macan_ack*)cf->data;
						color = ANSI_COLOR_BLUE;
						char *p = type + sprintf(type, "ack group=");
						char delim = '[';
						int i;
						for (i = 0; i < 24; i++)
							if (ack->group[i/8] & (0x01 << (i%8))) {
								p += sprintf(p, "%c%d", delim, i);
								delim = ' ';
							}
						strcpy(p, "]");
					}
					break;
				case FL_SIGNAL_OR_AUTH_REQ:
					if (cf->can_dlc == 8) {
						struct macan_signal_ex *sig = (struct macan_signal_ex*)cf->data;
						sprintf(type, "signal #%d", sig->sig_num);
					} else {
						struct macan_sig_auth_req *ar = (struct macan_sig_auth_req*)cf->data;
						color = ANSI_COLOR_MAGENTA;
						const char *auth_req_type;
						switch (cf->can_dlc) {
						case 3: auth_req_type = "NO MAC"; break;
						case 7: auth_req_type = "+ MAC"; break;
						default: auth_req_type = "BROKEN!!!"; break;
						}
						sprintf(type, "auth req %s signal=#%d presc=%d", auth_req_type,
							ar->sig_num, ar->prescaler);
					}
					break;
				default:
					strcpy(type, "???");
				}
				char srcstr[8], dststr[8];
				macan_ecuid dst = macan_crypt_dst(cf);

				get_ecuid_str(ctx, srcstr, sizeof(srcstr), src);
				get_ecuid_str(ctx, dststr, sizeof(dststr), dst);

				sprintf(comment, "crypt %s->%s (%d->%d): %s", srcstr, dststr, src, dst, type);
			}
		} else {
			unsigned i;
			for (i = 0; i < ctx->config->sig_count; i++) {
				const struct macan_sig_spec *ss = &ctx->config->sigspec[i];
				if (cf->can_id == ss->can_nsid)
					sprintf(comment, "non-secure signal #%d", i);
				else if (cf->can_id == ss->can_sid)
					sprintf(comment, "secure signal #%d", i);
			}
		}
	}
	uint64_t time = read_time();
	printf("%s%s%4"PRIu64".%03"PRIu64" %-20s %s" ANSI_COLOR_RESET "\n", prefix, color, time/1000000, (time/1000)%1000, frame, comment);
}
