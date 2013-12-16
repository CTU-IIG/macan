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
#include "macan_config.h"

#define SIG_TIME 0x102 
#define TIME_DELTA 100000   /* tolerated time divergency from TS in usecs */
#define TIME_DIV 1000000
#define TIME_TIMEOUT 5000000	/* usec */
#define SKEY_TIMEOUT 500000000u /* usec */
#define SKEY_CHG_TIMEOUT 500000000u /* usec */
#define ACK_TIMEOUT 10000000	  /* usec */

/* ToDo: more recipients */
struct macan_sig_spec demo_sig_spec[] = {
	[SIGNAL_VW]    = {.can_nsid = 0,   .can_sid = 0x2C1,  .src_id = NODE_VW,     .dst_id = NODE_CTU, .presc = 1},
	[SIGNAL_CTU]    = {.can_nsid = 0,   .can_sid = 0x2D1,  .src_id = NODE_CTU,    .dst_id = NODE_VW,  .presc = 0},
	/* FIXME: Why we need time dummy signals? Time should be handled automatically. */
	[TIME_DUMMY2] = {.can_nsid = 0,   .can_sid = 0,     .src_id = TIME_SERVER, .dst_id = NODE_VW,  .presc = 0},
	[TIME_DUMMY3] = {.can_nsid = 0,   .can_sid = 0,     .src_id = TIME_SERVER, .dst_id = NODE_CTU, .presc = 0},
};

/* struct macan_node_spec demo_node_spec[] = { */
/*     [KEY_SERVER]  = {.can_id = 0x101, .ecu_id = 0x1, .name = "ks"}, */
/*     [TIME_SERVER] = {.can_id = 0x102, .ecu_id = 0x2, .name = "ts"},  */
/*     [NODE_VW]     = {.can_id = 0x103, .ecu_id = 0x3, .name = "node_vw"}, */
/*     [NODE_CTU]    = {.can_id = 0x104, .ecu_id = 0x4, .name = "node_ctu"}, */
/* }; */

const uint32_t ecu2canid_map[] = {
	[KEY_SERVER] = 0x101,
	[TIME_SERVER] = 0x102,
	[NODE_VW] = 0x103,
	[NODE_CTU] = 0x104,
};


struct macan_config config = {
	.node_id = NODE_CTU,
	.ltk = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
	.sig_count = SIG_COUNT,
	.sigspec = demo_sig_spec,
	.node_count = NODE_COUNT,
	.ecu2canid = ecu2canid_map,
	.key_server_id = KEY_SERVER,
	.time_server_id = TIME_SERVER,
	.can_id_time = SIG_TIME,
	.time_div = TIME_DIV,
	.ack_timeout = ACK_TIMEOUT,
	.skey_validity = SKEY_TIMEOUT,
	.skey_chg_timeout = SKEY_CHG_TIMEOUT,
	.time_timeout = ACK_TIMEOUT,
	.time_delta = TIME_DELTA,
};
