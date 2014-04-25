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
#include "common.h"
#include "macan_private.h"
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>
#include "helper.h"

#define NODE_COUNT 64
static struct macan_ctx macan_ctx;



void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib> -k <ltk_lib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_config *config = NULL;
	char *error;
	static void *ltk_handle;
	int i;

	int opt;
	while ((opt = getopt(argc, argv, "c:k:")) != -1) {
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

	const struct macan_key *ltks[NODE_COUNT];

	for (i = 0; i < config->node_count; i++) {
		if (i == config->key_server_id)
			continue;
		char node_id_str[30];
		sprintf(node_id_str, "macan_ltk_node%u", i);
		ltks[i] = dlsym(ltk_handle, node_id_str);
		error = dlerror();
		if(error != NULL) {
			print_msg(&macan_ctx, MSG_FAIL,
				  "Unable to load ltk key for node #%u from shared library\nReason: %s\n",
				  i, error);
			return 1;
		}
	}

	config->node_id = config->key_server_id;
	s = helper_init();

	macan_ev_loop *loop = MACAN_EV_DEFAULT;

	macan_init_ks(&macan_ctx, config, loop, s, ltks);



	macan_ev_run(loop);

	return 0;
}
