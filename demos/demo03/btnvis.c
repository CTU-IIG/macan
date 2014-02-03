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

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>

void sigint()
{
	endwin();
	exit(0);
}

int main(int argc, char *argv[])
{
	/* don't complain about unused parameter */
	(void)argv; (void)argc;

	int s;
	int ret;
	struct ifreq ifr;
	char *ifname = "vw_can";
	struct sockaddr_can addr;
	struct can_frame cf;

	if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
		perror("Error while opening socket");
		return 1;
	}

	ret = fcntl(s, F_SETFL, O_NONBLOCK);
	if (ret != 0) {
		perror("ioctl fail");
		return 1;
	}

	strcpy(ifr.ifr_name, ifname);
	ioctl(s, SIOCGIFINDEX, &ifr);

	memset(&addr, 0, sizeof(addr));
	addr.can_family  = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("Error in socket bind");
		return 1;
	}

	signal(SIGINT, sigint);
	initscr(); cbreak(); noecho(); nodelay(stdscr, TRUE);
	nonl();
	start_color();
	use_default_colors();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(1, COLOR_WHITE, COLOR_RED);
	init_pair(2, COLOR_WHITE, COLOR_GREEN);
	refresh();

	mvaddstr(0, 0, "Press 1 - 4 to send signals");
	while (1) {
		char str[100];
		struct pollfd pfd[2] = { { .fd = 0, .events = POLLIN },
					 { .fd = s, .events = POLLIN } };
		ret = poll(pfd, 2, 50);

		ret = (int)read(s, &cf, sizeof(cf));
		if (ret < 0 && errno != EAGAIN) {
			endwin();
			perror("read");
			return 1;
		} else if (ret > 0 && cf.can_id == 0x2d1) {
			int x, y, on = !!(cf.data [0] & 1);
			//printf("btn %s\n",  ? "on" : "off");
			if (on)
				attrset(A_BLINK|A_BOLD|COLOR_PAIR(2));
			else
				attrset(A_NORMAL);
			for (x = 0; x < 11; x++)
				for (y = 0; y < 6; y++)
					mvaddch(y+2, x+4, on ? '.' : '.');
			attrset(A_NORMAL | COLOR_PAIR(0));
		}

		int ch = getch();
		if (ch != ERR) {
			sprintf(str, "ch = %c", ch);
			mvaddstr(15, 0, str);

			memset(&cf, 0, sizeof(cf));
			cf.can_id = 0x2c1;
			cf.can_dlc = 4;
			if (ch >= '1' && ch <= '4') {
				cf.data[0] = (__u8) (1 << (ch - '1'));
				write(s, &cf, sizeof(cf));
			}
		}

		refresh();
	}

	return 0;
}
