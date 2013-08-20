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

/*
 * can.h
 *
 *  Created on: 7.8.2013
 *      Author: win
 */

#ifndef CAN_H_
#define CAN_H_

#ifdef TC1798
struct can_frame {
	uint32_t can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
	uint8_t can_dlc; /* data length code: 0 .. 8 */
	uint8_t data[8];
};
#else
#include <linux/can.h>
#endif

#endif /* CAN_H_ */
