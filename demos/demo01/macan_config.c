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

#define SIG_TIME 4
#define TIME_DELTA 2000   /* tolerated time divergency from TS in usecs */
#define TIME_DIV 500000
#define TIME_TIMEOUT 5000000	/* usec */
#define SKEY_TIMEOUT 6000000000u /* usec */
#define SKEY_CHG_TIMEOUT 30000000u /* usec */
#define ACK_TIMEOUT 10000000	  /* usec */

/* ToDo: more recipients */
const struct macan_sig_spec demo_sig_spec[] = {
	[TIME_DUMMY3] = {.can_nsid = 0, .can_sid = 0, .src_id = TIME_SERVER, .dst_id = 3, .presc = 0},
	[TIME_DUMMY2] = {0, 0, TIME_SERVER, 2, 0},
        [ENGINE]      = {0, 10, 2, 3, 5},
        [BRAKE]       = {1, 11, 2, 3, 7},
};

const uint32_t ecu2canid_map[] = {
	[KEY_SERVER] = 0,
	[TIME_SERVER] = 1,
	[NODE1] = 2,
	[NODE2] = 3,
};

const struct macan_config config = {
	.node_id = NODE_ID,
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
