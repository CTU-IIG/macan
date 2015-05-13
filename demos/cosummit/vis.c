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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <macan.h>
#include "helper.h"
#include "macan_config.h"
#include <inttypes.h>
#include "macan_private.h"

struct macan_ctx macan_ctx;
extern const struct macan_key macan_ltk_node3;

void sigint()
{
	endwin();
	exit(0);
}

void sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status s)
{
	(void)sig_num; (void)s;
	int x, y, on = !!sig_val;
	//printf("btn %s\n",  ? "on" : "off");
	if (on)
		attrset(A_BLINK|A_BOLD|COLOR_PAIR(2));
	else
		attrset(A_NORMAL);
	for (x = 0; x < 11; x++)
		for (y = 0; y < 6; y++)
			mvaddch(y+2, x+4, on ? '.' : '.');
	attrset(A_NORMAL | COLOR_PAIR(0));
	refresh();
}

void sig_invalid(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status s)
{
	(void)sig_num; (void)sig_val; (void)s;
	//printf("received invalid signal(%"PRIu8") = %#"PRIx32"\n", sig_num, sig_val);
}

static void stdin_cb (macan_ev_loop *loop, ev_io *w, int revents)
{
	(void)loop; (void)revents;
	struct macan_ctx *ctx = (struct macan_ctx*)w->data;

	int ch = getch();

	if (ch != ERR) {
		char str[100];
		sprintf(str, "ch = %c", ch);
		//mvaddstr(15, 0, str);

		if (ch >= '1' && ch <= '4')
			macan_send_sig(ctx, SIGNAL_LED, 1U << (ch - '1'));

		//refresh();
	}
}

extern const struct macan_key macan_ltk_node2;

int main(int argc, char *argv[])
{
	/* don't complain about unused parameter */
	(void)argv; (void)argc;

	int s;
	macan_ev_loop *loop = MACAN_EV_DEFAULT;
	ev_io stdin_watcher;


	s = helper_init("can0");

	struct macan_node_config node = {
		.node_id = NODE_PC,
		.ltk = &macan_ltk_node2,
	};

	struct macan_ctx *macan_ctx = macan_alloc_mem(&config, &node);
	macan_init(macan_ctx, loop, s);
	macan_reg_callback(macan_ctx, SIGNAL_SIN1, sig_callback, sig_invalid);

	signal(SIGINT, sigint);
	initscr(); cbreak(); noecho(); nodelay(stdscr, TRUE);
	nonl();
	start_color();
	use_default_colors();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(1, COLOR_WHITE, COLOR_RED);
	init_pair(2, COLOR_WHITE, COLOR_GREEN);
	mvaddstr(0, 0, "Press 1 - 4 to send signals");
	refresh();

	ev_io_init(&stdin_watcher, stdin_cb, 0/*stdin fd*/, EV_READ);
	stdin_watcher.data = &macan_ctx;
	ev_io_start(loop, &stdin_watcher);

	macan_ev_run(loop);

	return 0;
}
