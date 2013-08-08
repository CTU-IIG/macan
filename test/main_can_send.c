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
#include "Std_Types.h"
#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "EcuM.h"
#include "Test_Print.h"
#include "Os.h"
#include "she.h"

char Option[80];
uint8 TempUSage = 0;
sint32 desired_offset;
float32 desired_speed;
char s[10];

struct challenge {
        uint8_t flags : 2;
        uint8_t dst_id : 6;
        uint8_t fwd_id : 8;
        uint8_t chal[6];
};

struct challenge ch = { 0, 2, 0, {4, 4, 4, 4, 3, 2} };

/* PDU info used by Controller 1 */
Can_PduType PduInfo_test[] =
{
	{50, 8, 1, &ch }
};

const struct EcuM_ConfigType_Tag*   EcuMConfigPtr;

#ifdef COMMENT
void main(void)
{
	unsigned int Counter;

	/* MCU Clock / System Clock Initialization Clk: 60Mhz. */
	/*
	Mcu_InitClock(0);
	while (Mcu_GetPllStatus() == 0);
	Mcu_DistributePllClock();
	*/

	/* Configure CAN Module Interrupt Priority */
	/* Irq_CanInit(); */
	
	/* Enable Global Interrupt Flag. */
	/* EnableAllInterrupts(); */
	
	/* Port Initialization */
	
	/* CAN driver initialization */

	/* void SendMessage(void) */
	/* EnableAllInterrupts(); */
	/* Can_EnableControllerInterrupts(CAN_CONTROLLER0); */

	/* Set the state of CAN controller 0 and 1 to Start */
	Can_SetControllerMode(CAN_CONTROLLER0, CAN_T_START);
	//Can_SetControllerMode(CAN_CONTROLLER1, CAN_T_START);

	/* activate SHE */
	/* ToDo: revisit */
	SHE_CLC &= ~(0x1);
	while (SHE_CLC & 0x2) {};
	wait_until_done();

	//while (1) {

	Can_Write(2, &PduInfo_test[0]);

	while(1) {};
		/* Can_Write(3, &PduInfo_test[0]); */
	
		/* Delay */
		/* Counter = 0x5FFFF; */
		/* while(Counter--) ; */
	
		//Can_MainFunction_Write();
		//Can_MainFunction_Read();
	//}

	/* Can_Demo() */;
}
#endif

