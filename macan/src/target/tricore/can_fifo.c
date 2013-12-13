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

#include "can_fifo.h"
#include <stdint.h>
#include <can.h>
#include <can_frame.h>
#include <CanIf_Cbk.h>

struct Can_HardwareObject
{
  CAN_MOFCR0_type    MOFCR;     /* Function Control Register      */
  CAN_MOFGPR0_type   MOFGPR;    /* FIFO/Gateway Pointer Register  */
  CAN_MOIPR0_type    MOIPR;     /* Interrupt Pointer Register     */
  CAN_MOAMR0_type    MOAMR;     /* Acceptance Mask Register       */
  uint8              MODAT[8];  /* Message Data 0..7              */
  CAN_MOAR0_type     MOAR;      /* Arbitration Register           */
  CAN_MOCTR0_type    MOCTR;     /* Control Register               */
};

#define CAN_EXTENDED_MSB_SET (0x80000000U)
#define CAN_HWOBJ ((struct Can_HardwareObject volatile *)(void*) &CAN_MOFCR0)
#define CAN_MOAR_ID_STD_SHIFT  (18U)

void poll_can_fifo(struct macan_ctx *ctx, void (*cback)(struct macan_ctx *ctx, int s, struct can_frame *cf))
{
	uint32_t i;
	Can_IdType can_id;
	struct can_frame cf;

	i = CAN_MOFGPR0.B.SEL - 1;

	/* ToDo: free MOs, i.e. avoid MSGLOST */
	while (i != CAN_MOFGPR0.B.CUR) {
		if (CAN_HWOBJ[i].MOAR.B.IDE)
			can_id = CAN_HWOBJ[i].MOAR.B.ID | CAN_EXTENDED_MSB_SET;
		else
			can_id = CAN_HWOBJ[i].MOAR.B.ID >> CAN_MOAR_ID_STD_SHIFT;

		cf.can_id = can_id;
		cf.can_dlc = CAN_HWOBJ[i].MOFCR.B.DLC;
		memcpy(cf.data, (uint8 *) CAN_HWOBJ[i].MODAT, 8);

		cback(ctx, 0, &cf);

		i++;
		if (i > 32) {
			i = 0;
		}
	}

	CAN_MOFGPR0.B.SEL = i + 1;
}
