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

#ifndef MACAN_EV_H
#define MACAN_EV_H

#include <ev.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MACAN_EV_DEFAULT EV_DEFAULT
#define MACAN_EV_READ	 EV_READ

typedef struct ev_loop  macan_ev_loop;
typedef struct ev_io    macan_ev_can;
typedef struct ev_timer macan_ev_timer;

static inline void
macan_ev_can_init(macan_ev_can *ev,
		  void (*cb) (macan_ev_loop *loop,  macan_ev_can *w, int revents),
		  int canfd, int events)
{
	ev_io_init(ev, cb, canfd, events);
}

static inline void
macan_ev_can_start(macan_ev_loop *loop, macan_ev_can *w)
{
	ev_io_start(loop, w);
}

static inline void
macan_ev_timer_init(macan_ev_timer *ev,
		    void (*cb) (macan_ev_loop *loop,  macan_ev_timer *w, int revents),
		    unsigned after_ms, unsigned repeat_ms)
{
	ev_timer_init(ev, cb, after_ms/1000.0, repeat_ms/1000.0);
}

static inline void
macan_ev_timer_start(macan_ev_loop *loop, macan_ev_timer *w)
{
	ev_timer_start(loop, w);
}

static inline void
macan_ev_timer_again(macan_ev_loop *loop, macan_ev_timer *w)
{
	ev_timer_again(loop, w);
}

static inline bool
macan_ev_run(macan_ev_loop *loop)
{
	return ev_run(loop, 0);
}

#ifdef __cplusplus
}
#endif

#endif
