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

enum sig_id {
	ENGINE,
	BRAKE,
	TRAMIS,
	SIG_MAX
};

#define NODE_MAX 2

struct sig_spec {
	uint8_t can_nsid;  /* can non-secured id */
	uint8_t can_sid;   /* can secured id */
	uint8_t src_id;       /* node dispathing this signal */
	uint8_t dst_id;       /* node receiving this signal */
	uint8_t presc;     /* prescaler */
};

#endif /* MACAN_CONFIG_H */

