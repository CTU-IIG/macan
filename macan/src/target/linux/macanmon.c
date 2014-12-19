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
#include "common.h"
#include "helper.h"
#include "macan_private.h"
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>

#define NODE_COUNT 64

static struct macan_ctx macan_ctx;

static void
print_frame_cb (macan_ev_loop *loop, macan_ev_can *w, int revents)
{
	(void)loop; (void)revents; (void)w; /* suppress warnings */
	struct can_frame cf;

	while (macan_read(&macan_ctx, &cf))
		print_frame(macan_ctx.config, &cf, "");
}



void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_config *config = NULL;

	int opt;
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

        config->node_id = 0xff;	/* We do not send anything */
	srand((unsigned)time(NULL));

	macan_ev_can can_watcher;
	macan_ev_loop *loop = MACAN_EV_DEFAULT;

	s = helper_init("can0");

	macan_ctx.sockfd = s;
	macan_ctx.config = config;
	macan_ctx.loop = loop;

	macan_ev_canrx_setup (&macan_ctx, &can_watcher, print_frame_cb);
	macan_ev_can_start (loop, &can_watcher);

	macan_ev_run(loop);

	return 0;
}
