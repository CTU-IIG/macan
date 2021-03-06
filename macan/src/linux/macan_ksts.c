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

/* This file combines MaCAN key and time server in a single binary. */

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "helper.h"
#include "macan_private.h"
#include <linux/can/raw.h>

#define NODE_COUNT 64

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib> -k <ltk_lib> [-d <CAN interface>]\n", argv0);
}

int main(int argc, char *argv[])
{
	struct macan_config *config = NULL;
	char *error;
	static void *ltk_handle;
	int i;
	char *device = "can0";

	int opt;
	while ((opt = getopt(argc, argv, "c:d:k:")) != -1) {
		switch (opt) {
		case 'c': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			if(!handle) {
				fprintf(stderr, "%s\n", dlerror());
				exit(1);
			}
			dlerror(); /* Clear previous error (if any) */
			config = dlsym(handle, "config");
			if ((error = dlerror()) != NULL) {
				fprintf(stderr, "%s\n", error);
				exit(1);
			}
			break;
		}
		case 'd':
			device = optarg;
			break;
		case 'k':
			ltk_handle = dlopen(optarg, RTLD_LAZY);
			if(!ltk_handle) {
			   fprintf(stderr, "%s\n", dlerror());
			   exit(1);
			}
			break;
		default: /* '?' */
			print_help(argv[0]);
			exit(1);
		}
	}
	if (!config || !ltk_handle) {
		print_help(argv[0]);
		exit(1);
	}

	macan_ev_loop *loop = MACAN_EV_DEFAULT;

        /*************************/
        /* Initialize key server */
        /*************************/

	struct macan_node_config nc_ks = {
		.node_id = config->key_server_id
	};

	/* Import long-term keys from the supplied file (shared library) */
	const struct macan_key *ltks[NODE_COUNT];

	for (i = 0; i < config->node_count; i++) {
		if (i == config->key_server_id)
			continue;
		char node_id_str[30];
		sprintf(node_id_str, "macan_ltk_node%u", i);
		ltks[i] = dlsym(ltk_handle, node_id_str);
		error = dlerror();
		if(error != NULL) {
			fprintf(stderr,
				"Unable to load ltk key for node #%u from shared library\nReason: %s\n",
				i, error);
			return 1;
		}
	}

	struct macan_ctx *ctx_ks = macan_alloc_mem(config, &nc_ks);
	macan_init_ks(ctx_ks, loop, helper_init(device), ltks);
	ctx_ks->print_msg_enabled = true;
	ctx_ks->dump_disabled = true;

        /**************************/
        /* Initialize time server */
        /**************************/

	struct macan_node_config nc_ts = {
		.node_id = config->time_server_id,
		.ltk = ltks[config->time_server_id],
	};
	struct macan_ctx *ctx_ts = macan_alloc_mem(config, &nc_ts);
	int s = helper_init(device);
	int recv_own_msgs = 1; /* 0 = disabled (default), 1 = enabled */
	setsockopt(s, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof(recv_own_msgs));
	macan_init_ts(ctx_ts, loop, s);
	ctx_ts->print_msg_enabled = true;


	/**********************/
        /* Run the event loop */
        /**********************/

	macan_ev_run(loop);

	return 0;
}
