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
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include "common.h"
#include <unistd.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "macan_private.h"
#include "helper.h"

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
	FILE *fp;
	bool return_val = SUCCESS;

	if(!(fp = fopen("/dev/urandom","r"))) {
		return ERROR;
	}

	if(fread(dest, 1, len, fp) != len) {
		return_val = ERROR;
	}
	fclose(fp);
	return return_val;
}

bool macan_read(struct macan_ctx *ctx, struct can_frame *cf)
{
	ssize_t rbyte;

	rbyte = read(ctx->sockfd, cf, sizeof(struct can_frame));
	if (rbyte == -1 && errno == EAGAIN) {
		return false;
	}

	if (rbyte != 16) {
		print_msg(ctx, MSG_FAIL, "ERROR recv not 16 bytes but %ld\n", rbyte);
		abort();
	}

	if (getenv("MACAN_DUMP")) {
		static char prefix[20];
		if (!prefix[0])
			snprintf(prefix, sizeof(prefix), "macan%05d", getpid());
		print_frame(ctx->config, cf, prefix);
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

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return -2;
	}

	return s;
}

bool macan_send(struct macan_ctx *ctx,  const struct can_frame *cf)
{
	ssize_t ret = write(ctx->sockfd, cf, sizeof(*cf));
	return (ret == sizeof(*cf));
}
