/*
 *  Copyright 2015 Czech Technical University in Prague
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

#include "macan.h"
#include "helper.h"
#include <linux/can.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int s = helper_init("can0");
	fcntl(s, F_SETFL, 0); // Unset O_NONBLOCK flag

	while (1) {
		struct can_frame cf;
		ssize_t ret = read(s, &cf, sizeof(cf));
		if (ret == -1) {
			perror("read");
			exit(1);
		}
		if (ret != sizeof(cf))
			continue;
		write(1, &cf.can_id, sizeof(cf.can_id));
		write(1, &cf.can_dlc, sizeof(cf.can_dlc));
		write(1, cf.data, sizeof(cf.data));
	}
	return 0;
}
