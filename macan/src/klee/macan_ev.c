/*
 *  Copyright 2014 Czech Technical University in Prague
 *
 *  Authors: Michal Sojka <sojkam1@fel.cvut.cz>
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

#include "macan_ev.h"
#include "macan_private.h"


macan_ev_loop macan_ev_loop_default;

/*
	KLEE VERSION COPIED FROM STM32 + changes.
*/

#define HOW_MANY_ITERATIONS 3


void
macan_ev_can_init(macan_ev_can *ev,
		  void (*cb) (macan_ev_loop *loop,  macan_ev_can *w, int revents),
		  int canfd, int events)
{
	(void)events;
	ev->cb = cb;
	ev->canfd = canfd;
}

void
macan_ev_can_start(macan_ev_loop *loop, macan_ev_can *w)
{
	w->next = loop->cans;
	loop->cans = w;
}

void
macan_ev_timer_init(macan_ev_timer *ev,
		    void (*cb) (macan_ev_loop *loop,  macan_ev_timer *w, int revents),
		    unsigned after_ms, unsigned repeat_ms)
{
	ev->cb = cb;
	ev->after_us = after_ms * 1000;
	ev->repeat_us = repeat_ms * 1000;
}

void
macan_ev_timer_start(macan_ev_loop *loop, macan_ev_timer *w)
{
	w->expire_us = read_time() + w->after_us;
	w->next = loop->timers;
	loop->timers = w;
}

void
macan_ev_timer_again(macan_ev_loop *loop, macan_ev_timer *w)
{
	(void)loop;
	w->expire_us = read_time() + w->repeat_us;
}



bool
macan_ev_run(macan_ev_loop *loop)
{

	//original version looped infinitely here, we only try to process N frames in row.

	for (int i = 0; i < HOW_MANY_ITERATIONS; ++i){
		uint64_t now = read_time();

		for (macan_ev_timer *t = loop->timers; t; t = t->next) {
			if (now >= t->expire_us) {
				t->cb(loop, t, MACAN_EV_TIMER);
				t->expire_us = now + t->repeat_us;
			}
		}
		loop->cans->cb(NULL, loop->cans, 0);
	}
	return true;
}
