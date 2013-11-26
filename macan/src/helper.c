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
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include "common.h"
#include "aes_keywrap.h"
#ifdef __CPU_TC1798__
#include "Can.h"
#include "she.h"
#else
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif /* __CPU_TC1798__ */
#include <helper.h>
#include <macan.h>
#include <macan_private.h>

#ifndef __CPU_TC1798__
#include "target/linux/lib.h"

static int is_crypt_frame(struct macan_ctx *ctx, struct can_frame *cf)
{
	if (cf->can_dlc == 8 && (cf->data[0] & 0x3f) == ctx->config->node_id) { /* Frame for me */
		/* TODO: Check can_id in CAN-ID(EDU-ID) mapping */
		return 1;
	} else
		return 0;
}

void helper_read_can(struct macan_ctx *ctx, int s, void (*cback)(struct macan_ctx *ctx, int s, struct can_frame *cf))
{
	struct can_frame cf;
	int rbyte;

	rbyte = read(s, &cf, sizeof(struct can_frame));
	if (rbyte == -1 && errno == EAGAIN) {
		return;
	}

	if (rbyte != 16) {
		printf("ERROR recv not 16 bytes");
		exit(0);
	}

	char frame[80], comment[80];
	comment[0] = 0;
	sprint_canframe(frame, &cf, 0, 8);
	if (ctx && ctx->config) {
		if (cf.can_id == ctx->config->can_id_time)
			sprintf(comment, "time %ssigned", cf.can_dlc == 4 ? "un" : "");
		else if (is_crypt_frame(ctx, &cf)) {
			char *type;
			switch (cf.data[0] >> 6) {
			case FL_CHALLENGE:
				type = "challenge";
				break;
			case FL_SESS_KEY:
				type = "sess_key or ack";
			case FL_SIGNAL:
				type = "signal";
				break;
			default:
				type = "???";
			}
			sprintf(comment, "crypt for me: %s\n", type);
		}
	}
	printf("RECV %-20s %s\n", frame, comment);

	cback(ctx, s, &cf);
}
#endif /* __CPU_TC1798__ */

int helper_init()
#ifdef __CPU_TC1798__
{
	Can_SetControllerMode(CAN_CONTROLLER0, CAN_T_START);

	/* activate SHE */
	/* ToDo: revisit */
	SHE_CLC &= ~(0x1);
	while (SHE_CLC & 0x2) {};
	wait_until_done();

	return 0;
}
#else
{
	int s;
	int r;
	struct ifreq ifr;
	char *ifname = "can0";
	struct sockaddr_can addr;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return -1;
	}

	r = fcntl(s, F_SETFL, O_NONBLOCK);
	if (r != 0) {
		perror("ioctl fail");
		return -1;
	}

#if 0
	int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
#endif
	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	printf("%s at index %d\n", ifname, ifr.ifr_ifindex);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}

	return s;
}
#endif /* __CPU_TC1798__ */
