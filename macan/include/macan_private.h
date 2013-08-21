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

#ifndef MACAN_PRIVATE_H
#define MACAN_PRIVATE_H

#include <stdint.h>
#include <can_frame.h>

/* MaCAN message definitions */

struct macan_challenge {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t fwd_id : 8;
	uint8_t chg[6];
};

struct macan_sess_key {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t seq : 4;
	uint8_t len : 4;
	uint8_t data[6];
};

struct macan_ack {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t group[3];
	uint8_t cmac[4];
};

struct macan_crypt_frame {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
};

struct macan_sig_auth_req {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t prescaler;
	uint8_t cmac[4];
};

struct macan_signal {
	uint8_t sig[4];
	uint8_t cmac[4];
};

struct macan_signal_ex {
	uint8_t flags : 2;
	uint8_t dst_id : 6;
	uint8_t sig_num;
	uint8_t signal[2];
	uint8_t cmac[4];
};

/**
 * Timekeeping structure
 */

struct macan_time {
	uint64_t offs;       /* contains the time difference between local time
   			        and TS time;
				i.e. TS_time = Local_time + offs */
	uint64_t chal_ts;    /* local timestamp when request for signed time was sent  */
	uint8_t chg[6];
};

/**
 * Communication partner
 *
 * Stores run-time information about a communication partner.
 *
 * @param id    6-bit id
 * @param skey  wrapped key or consequently session key
 * @param stat  comunication channel status
 */
struct com_part {
	uint8_t skey[16];	/* Session key (from key server) */
	uint64_t valid_until;	/* Local time of key expiration */
	uint8_t chg[6];		/* Challenge for communication with key server */
	uint8_t flags;
	uint32_t group_field;	/* Bitmask of known key sharing */
	uint32_t wait_for;	/* The value of group_field we are waiting for  */
};

struct sig_handle {
	int presc;
	uint8_t presc_cnt;
	uint8_t flags;
	void (*cback)(uint8_t sig_num, uint32_t sig_val);
};

#endif

