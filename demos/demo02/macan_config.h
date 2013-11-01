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

#ifndef MACAN_CONFIG_H
#define MACAN_CONFIG_H

#include <macan.h>

#define SIG_TIME 4
#define TIME_DELTA 2000   /* tolerated time divergency from TS in usecs */
#define TIME_DIV 500000
#define TIME_TIMEOUT 5000000	/* usec */
#define SKEY_TIMEOUT 6000000000u /* usec */
#define SKEY_CHG_TIMEOUT 30000000u /* usec */
#define ACK_TIMEOUT 10000000	  /* usec */

enum sig_id {
	SIGNAL_A,
	SIGNAL_B,
	SIGNAL_C,
    SIGNAL_D,
    TIME_DUMMY2,
	TIME_DUMMY3,
	SIG_COUNT
};

enum node_id {
	KEY_SERVER,
	TIME_SERVER,
	NODE1,
	NODE2,
	NODE_COUNT
};

extern struct macan_sig_spec demo_sig_spec[];

#endif /* MACAN_CONFIG_H */

