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
#include <string.h>
#include "common.h"
#include <unistd.h>
#include <macan.h>
#include <helper.h>
#include <dlfcn.h>

/* ToDo
 *   implement groups
 *   some error processing
 */

static struct macan_ctx macan_ctx;

void print_help(char *argv0)
{
	fprintf(stderr, "Usage: %s -c <config_shlib> -k <key_shlib>\n", argv0);
}

int main(int argc, char *argv[])
{
	int s;
	struct macan_config *config = NULL;

	int opt;
	while ((opt = getopt(argc, argv, "c:k:")) != -1) {
		switch (opt) {
		case 'c': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			config = dlsym(handle, "config");
			break;
		}
		case 'k': {
			void *handle = dlopen(optarg, RTLD_LAZY);
			if(!handle) {
			   fprintf(stderr, "%s\n", dlerror());
			   exit(1);
			}
			char str[100];
			int cnt = sprintf(str,"%s","macan_ltk_node");
			sprintf(str+cnt,"%u",config->time_server_id);
			config->ltk = dlsym(handle,"macan_ltk_node1");
			char *error = dlerror();
			if(error != NULL) {
				print_msg(NULL, MSG_FAIL,"Unable to load ltk key from shared library\nReason: %s\n",error);
				exit(1);
			}
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
        config->node_id = config->time_server_id;

	s = helper_init();
	macan_ev_loop *loop = MACAN_EV_DEFAULT;
	macan_init_ts(&macan_ctx, config, loop, s);
	macan_ev_run(loop);

	return 0;
}
