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

#include <macan.h>
#include "macan_config.h"

struct macan_sig_spec demo_sig_spec[] = {
	[SIGNAL_LED]    = {.can_nsid = 0,   .can_sid = 0x10,  .src_id = NODE_PC,     .dst_id = NODE_TC, .presc = 1},
	[SIGNAL_SIN1]   = {.can_nsid = 0,   .can_sid = 0x20,  .src_id = NODE_TC,     .dst_id = NODE_PC,  .presc = 1},
	[SIGNAL_SIN2]   = {.can_nsid = 0x21,.can_sid = 0x00,  .src_id = NODE_TC,     .dst_id = NODE_PC,  .presc = 0},
};

const struct macan_can_ids demo_can_ids = {
	.time = 0x000,
	.ecu = (struct macan_ecu[]){
		[KEY_SERVER] = {0x101,"KS"},
		[TIME_SERVER] = {0x102,"TS"},
		[NODE_PC] = {0x103,"PC"},
		[NODE_TC] = {0x104,"TC"},
	},
};

struct macan_config config = {
	.sig_count = SIG_COUNT,
	.sigspec = demo_sig_spec,
	.node_count = NODE_COUNT,
	.canid = &demo_can_ids,
	.key_server_id = KEY_SERVER,
	.time_server_id = TIME_SERVER,
	.time_div = 1000000,
	.skey_validity = 500000000u,
	.skey_chg_timeout = 5000000u,
	.time_req_sep = 1000000,
	.time_delta = 1000000,   /* tolerated time divergency from TS in usecs */
};
