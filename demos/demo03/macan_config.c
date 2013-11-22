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

#include <macan.h>
#include <macan_config.h>

/* ToDo: more recipients */
struct macan_sig_spec demo_sig_spec[] = {
//	[TIME_DUMMY3] = {.can_nsid = 0,   .can_sid = 0,     .src_id = TIME_SERVER, .dst_id = NODE_VW,  .presc = 0},
//	[TIME_DUMMY2] = {.can_nsid = 0,   .can_sid = 0,     .src_id = TIME_SERVER, .dst_id = NODE_CTU, .presc = 0},
	[SIGNAL_A]    = {.can_nsid = 0,   .can_sid = 0xC1,  .src_id = NODE_VW,     .dst_id = NODE_CTU, .presc = 0}, // signal 1
	[SIGNAL_B]    = {.can_nsid = 0,   .can_sid = 0xD1,  .src_id = NODE_CTU,    .dst_id = NODE_VW,  .presc = 0}, // signal 2
};

struct macan_node_spec demo_node_spec[] = {
    [KEY_SERVER]  = {.can_id = 0x101, .ecu_id = 0x1},
    [TIME_SERVER] = {.can_id = 0x102, .ecu_id = 0x2},
    [NODE_VW]     = {.can_id = 0x103, .ecu_id = 0x3},
    [NODE_CTU]    = {.can_id = 0x104, .ecu_id = 0x4},
};
