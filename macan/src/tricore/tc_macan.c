/*
 *  Copyright 2014, 2015 Czech Technical University in Prague
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

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>
#include "common.h"
#include "can_frame.h"
#include "Std_Types.h"
#include "Mcu.h"
#include "Port.h"
#include "Can.h"
#include "EcuM.h"
#include "Test_Print.h"
#include "Os.h"
#include "she.h"
#include "macan_config.h"
#include "macan.h"
#include "macan_private.h"
#include "macan_bench.h"

#define f_STM 100000000
#define TIME_USEC (f_STM / 1000000)
#define TIME_NSEC (f_STM / 100000000)
#define CAN_IF 1

/**
 * write() - sends a can frame
 * @s:   ignored
 * @cf:  a can frame
 * @len: ignored
 *
 * write implemented on top of AUTOSAR functions.
 */
bool macan_send(struct macan_ctx *ctx,  const struct can_frame *cf)
{
	/* ToDo: consider some use of PduIdType */
	Can_PduType pdu_info = {17, cf->can_dlc, cf->can_id, cf->data};
	Can_Write(CAN_IF, &pdu_info);

	/* ToDo: redo */
	uint64_t wait = read_time();
	while (wait + 1000 > read_time()) {};

	return 16;
}

uint64_t read_time()
{
	uint64_t time = 0;
	uint32_t *time32 = (uint32_t *)&time;
	time32[0] = STM_TIM0.U;
	time32[1] = STM_CAP.U;

	return time * 41 >> 12;	/* ≅ time/100; 100 = TIME_USEC */
}

/*
 * Generate random bytes
 *
 * @param[in] dest Pointer to location where to store bytes
 * @param[in] len  Number of random bytes to be written
 */
bool gen_rand_data(void *dest, size_t len)
{
	uint8_t *p = (uint8_t *) dest;

	srand(time(NULL));
	while(len--) {
		p[len] = (uint8_t) rand();
	}
	return SUCCESS;

}
/*
 * used for benchmarking only
 * not used in MaCAN
 */
static uint64_t bench_time;
static uint64_t read_time_bench()
{
	uint64_t time;
	uint32_t *time32 = (uint32_t *)&time;
	time32[0] = STM_TIM0.U;
	time32[1] = STM_TIM6.U;

	return time;
}
void bench_tick(void)
{
	bench_time = read_time_bench();
}
void bench_tack(char *s)
{
	uint64_t stop = read_time_bench();

	printf("%s: %.2f us\n",s,(stop - bench_time) / 100.0);
}

void macan_target_init(struct macan_ctx *ctx)
{}
