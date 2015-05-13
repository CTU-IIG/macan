#include <macan.h>
#include "macan_private.h"
#include "helper.h"
#include "cfg.h"

extern struct macan_config config;

const struct macan_key *ltk[NODE_COUNT] = {
	&(struct macan_key) { .data = { 0x0f,0x85,0xb9,0x4e,0x1d,0xc1,0xdb,0x8b,0x4d,0x35,0xf3,0x84,0x28,0x3e,0x0f,0xba } },
	&(struct macan_key) { .data = { 0xae,0x27,0x97,0x20,0x20,0x79,0x3e,0x5a,0x48,0x0d,0x2c,0xa6,0xa5,0x47,0x15,0x45 } },
	&(struct macan_key) { .data = { 0x0b,0x49,0x6b,0xfc,0x4a,0x24,0x6a,0xd5,0xaa,0x5f,0xfc,0x7e,0x7d,0x99,0x6b,0x78 } },
	&(struct macan_key) { .data = { 0x34,0xdb,0x79,0xcf,0x34,0x61,0x25,0x26,0x1c,0x3a,0xe7,0xe8,0xec,0x54,0x36,0xaa } },
};

/********/
/* Test */
/********/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void request_key(macan_ev_loop *loop, ev_timer *w, int revents)
{
	(void)loop; (void)revents;
	struct macan_ctx *ctx = w->data;

	macan_request_key(ctx, ECU_J);
}

static void do_exit(macan_ev_loop *loop, ev_timer *w, int revents)
{
	(void)loop; (void)revents; (void)w;
	exit(0);
}

static void sig_callback(uint8_t sig_num, uint32_t sig_val, enum macan_signal_status s)
{
	(void)sig_num, (void)sig_val; (void)s;
}


struct macan_node_config node_config[NODE_COUNT];

int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
	macan_ev_loop *loop = MACAN_EV_DEFAULT;
	ev_timer ev_request_key, ev_exit;

	macan_ecuid i, ecu_from = 0, ecu_to = NODE_COUNT - 1;

	for (i = ecu_from; i <= ecu_to; i++) {
		node_config[i].node_id = i;
		node_config[i].ltk = ltk[i];
		struct macan_ctx *ctx = macan_alloc_mem(&config, &node_config[i]);
		switch (i) {
		case KEY_SERVER:
			macan_init_ks(ctx, loop, helper_init("vcanj"), ltk);
			break;
		case TIME_SERVER:
			macan_init_ts(ctx, loop, helper_init("vcanj"));
			break;
		case ECU_I:
			macan_init(ctx, loop, helper_init("vcani"));
			macan_ev_timer_setup(ctx, &ev_request_key, request_key, 100, 0);

			/* This is run as a part of automated test
			 * suite - we need to exit automatically.
			 * Timeout of 200ms should be enough. */
			macan_ev_timer_setup(ctx, &ev_exit, do_exit, 200, 0);
			break;
		case ECU_J:
			macan_init(ctx, loop, helper_init("vcanj"));
			macan_reg_callback(ctx, SIGNAL_0, sig_callback, NULL);
			break;
		}
	}


	macan_ev_run(loop);

	return 0;
}
