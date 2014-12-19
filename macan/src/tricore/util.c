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

#include <stdint.h>
#include "helper.h"
#include <Can.h>
#include "she.h"

void clear_endinit(void)
{
  uint32_t u32WdtCon0, u32WdtCon1;
  uint32_t pwd = 0;

  u32WdtCon0 = WDT_CON0.U;
  u32WdtCon1 = WDT_CON1.U;

  /* unlock */
  //pwd |= u32WdtCon0 & 0x1;
  //pwd |= u32WdtCon0 & 0x4;

  u32WdtCon0 &= 0xFFFFFF03;
  u32WdtCon0 |= 0xF0;
  u32WdtCon0 |= (u32WdtCon1 & 0xc);
  u32WdtCon0 ^= 0x2;
  WDT_CON0.U = u32WdtCon0;

  /* Modify access to u32WdtCon0 */
  u32WdtCon0 &= 0xFFFFFFF0;
  u32WdtCon0 |= 2;
  WDT_CON0.U = u32WdtCon0;
}

void set_endinit(void)
{
  uint32_t u32WdtCon0, u32WdtCon1;

  u32WdtCon0 = WDT_CON0.U;
  u32WdtCon1 = WDT_CON1.U;

  u32WdtCon0 &= 0xFFFFFF03;
  u32WdtCon0 |= 0xF0;
  u32WdtCon0 |= (u32WdtCon1 & 0xc);
  u32WdtCon0 ^= 0x2;
  WDT_CON0.U = u32WdtCon0;

  /* Modify access to u32WdtCon0 */
  u32WdtCon0 &= 0xFFFFFFF0;
  u32WdtCon0 |= 3;
  WDT_CON0.U = u32WdtCon0;
}

int helper_init(const char *ifname)
{
	Can_SetControllerMode(CAN_CONTROLLER0, CAN_T_START);

	/* activate SHE */
	/* ToDo: revisit */
	SHE_CLC &= ~(0x1);
	while (SHE_CLC & 0x2) {};
	wait_until_done();

	return 0;
}
