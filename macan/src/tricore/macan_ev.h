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

#include <stdbool.h>
#include <stdint.h>

#define MACAN_EV_DEFAULT &macan_ev_loop_default;

#define MACAN_EV_READ	 1
#define MACAN_EV_TIMER	 2

struct macan_ev_loop;

typedef struct macan_ev_can {
	void (*cb) (struct macan_ev_loop *loop,  struct macan_ev_can *w, int revents);
	struct macan_ev_can *next;
	int canfd;
	struct can_frame *received;
	void *data;
} macan_ev_can;

typedef struct macan_ev_timer {
	void (*cb) (struct macan_ev_loop *loop,  struct macan_ev_timer *w, int revents);
	struct macan_ev_timer *next;
	unsigned after_us;
	unsigned repeat_us;
	uint64_t expire_us;
	void *data;
} macan_ev_timer;

typedef struct macan_ev_loop {
	macan_ev_can   *cans;
	macan_ev_timer *timers;
} macan_ev_loop;

extern macan_ev_loop macan_ev_loop_default;

void
macan_ev_can_init(macan_ev_can *ev,
		  void (*cb) (macan_ev_loop *loop,  macan_ev_can *w, int revents),
		  int canfd, int events);
void
macan_ev_can_start(macan_ev_loop *loop, macan_ev_can *w);

void
macan_ev_timer_init(macan_ev_timer *ev,
		    void (*cb) (macan_ev_loop *loop,  macan_ev_timer *w, int revents),
		    unsigned after_ms, unsigned repeat_ms);

void
macan_ev_timer_start(macan_ev_loop *loop, macan_ev_timer *w);

void
macan_ev_timer_again(macan_ev_loop *loop, macan_ev_timer *w);

bool
macan_ev_run(macan_ev_loop *loop);

void macan_ev_recv_cb(struct can_frame *cf, void *data);

#endif
