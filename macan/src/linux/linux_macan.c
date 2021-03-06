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

#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/can.h>
#include "macan_private.h"

/**
 * read_time() - returns time in microseconds
*/
uint64_t read_time()
{
	uint64_t time;
	struct timespec ts;
	static struct timespec buz;
	static uint8_t once = 0;

	/* ToDo: redo this */
	if (once == 0) {
		once = 1;
		clock_gettime(CLOCK_MONOTONIC_RAW, &buz);
	}

	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	ts.tv_sec -= buz.tv_sec;
	ts.tv_nsec -= buz.tv_nsec;
	time = (uint64_t) (ts.tv_sec * 1000000);
	time += (uint64_t) (ts.tv_nsec / 1000);

	return time;
}

/*
 * Generate random bytes
 *
 * @param[in] dest Pointer to location where to store bytes
 * @param[in] len  Number of random bytes to be written
 */
bool gen_rand_data(void *dest, size_t len)
{
	bool return_val = SUCCESS;
#ifndef WITH_AFL
	FILE *fp;

	if(!(fp = fopen("/dev/urandom","r"))) {
		return ERROR;
	}

	if(fread(dest, 1, len, fp) != len) {
		return_val = ERROR;
	}
	fclose(fp);
#else
	memset(dest, 0, len);
#endif
	return return_val;
}

bool macan_read(struct macan_ctx *ctx, struct can_frame *cf)
{
#ifndef WITH_AFL
	ssize_t rbyte;

	rbyte = read(ctx->sockfd, cf, sizeof(struct can_frame));
	if (rbyte == -1 && errno == EAGAIN) {
		return false;
	}

	if (rbyte != sizeof(*cf)) {
		perror("macan_read");
		abort();
	}
#else
	if (read(0, &cf->can_id, sizeof(cf->can_id)) <= 0) exit(0);
	if (read(0, &cf->can_dlc, sizeof(cf->can_dlc)) <= 0) exit(0);
	if (read(0, cf->data, sizeof(cf->data)) <= 0) exit(0);
#endif

	if (getenv("MACAN_DUMP") && !ctx->dump_disabled) {
		static char prefix[20];
		if (!prefix[0])
			snprintf(prefix, sizeof(prefix), "macan%05d", getpid());
		print_frame(ctx, cf, prefix);
	}
	return true;
}

int helper_init(const char *ifname)
{
	int s;
	int r;
	struct ifreq ifr;
	struct sockaddr_can addr;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("socket(PF_CAN, SOCK_RAW, CAN_RAW)");
		exit(1);
	}

	r = fcntl(s, F_SETFL, O_NONBLOCK);
	if (r != 0) {
		perror("fcntl(s, F_SETFL, O_NONBLOCK)");
		exit(1);
	}

#if 0
	int loopback = 0; /* 0 = disabled, 1 = enabled (default) */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
#endif
	strcpy(ifr.ifr_name, ifname);
	if (ioctl(s, SIOCGIFINDEX, &ifr) == -1) {
		perror(ifname);
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		exit(1);
	}

	return s;
}

bool macan_send(struct macan_ctx *ctx,  const struct can_frame *cf)
{
	ssize_t ret = write(ctx->sockfd, cf, sizeof(*cf));
	if (ret == -1)
		perror("macan_send");
	return (ret == sizeof(*cf));
}

void macan_target_init(struct macan_ctx *ctx)
{
	if (getenv("MACAN_DEBUG"))
		ctx->print_msg_enabled = true;
}
