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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <nettle/aes.h>
#include "common.h"
#include "helper.h"
#include "aes_keywrap.h"
#include "macan_private.h"
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>
#include <poll.h>

#define NODE_COUNT 64

static struct macan_ctx macan_ctx;


void can_recv_cb(int s, struct can_frame *cf)
{
	print_frame(&macan_ctx, cf);
}

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_config *config = NULL;

	char opt;
	while ((opt = getopt(argc, argv, "c:")) != -1) {
		switch (opt) {
		case 'c': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			config = dlsym(handle, "config");
			break;
		}
		default: /* '?' */
			print_help(argv[0]);
			exit(1);
		}
	}
	if (!config) {
		print_help(argv[0]);
		exit(1);
	}

        config->node_id = -1;
	srand(time(NULL));
	s = helper_init();
	macan_init(&macan_ctx, config);

	while (1) {
		struct pollfd pfd = { .fd = s, .events = POLLIN };
		if (poll(&pfd, 1, -1) == -1)
			perror("poll");
		helper_read_can(&macan_ctx, s, can_recv_cb);
	}

	return 0;
}
