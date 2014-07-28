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

#include "macan_config.h"

/* ToDo: more recipients */
struct macan_sig_spec demo_sig_spec[] = {
	[SIGNAL_A]    = {.can_nsid = 0,   .can_sid = 200, .src_id = NODE1,       .dst_id = NODE2, .presc = 0}
};

const struct macan_can_ids demo_can_ids = {
	.time = 0x000,
	.ecu = (struct macan_ecu[]){
		[KEY_SERVER]  = {0x100, "KS"},
		[TIME_SERVER] = {0x101, "TS"},
		[NODE1]       = {0x102, "N1"},
		[NODE2]       = {0x103, "N2"},
	},
};

struct macan_config config = {
	.sig_count         = SIG_COUNT,
	.sigspec           = demo_sig_spec,
	.node_count        = NODE_COUNT,
	.canid         	   = &demo_can_ids,
	.key_server_id     = KEY_SERVER,
	.time_server_id    = TIME_SERVER,
	.time_div          = 1000000,
	.skey_validity     = 60000000,
	.skey_chg_timeout  = 5000000,
	.time_timeout      = 1000000,
	.time_delta        = 1000000,
};
