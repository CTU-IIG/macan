#include <macan.h>
#include "macan_private.h"
#include "helper.h"

/**********************/
/* MaCAN onfiguration */
/**********************/

enum sig_id {
	SIGNAL_0,
	SIG_COUNT
};

enum node_id {
	KEY_SERVER,
	TIME_SERVER,
	SENDER,
	RECEIVER,
	NODE_COUNT
};

struct macan_sig_spec test_sig_spec[] = {
	[SIGNAL_0]    = {.can_nsid = 0,   .can_sid = 0x516,   .src_id = SENDER, .dst_id = RECEIVER, .presc = 0},
};

const uint32_t ecu2canid_map[] = {
	[KEY_SERVER]  = 0x100,
	[TIME_SERVER] = 0x101,
	[SENDER]      = 0x102,
	[RECEIVER]    = 0x103,
};

struct macan_config config = {
	.sig_count         = SIG_COUNT,
	.sigspec           = test_sig_spec,
	.node_count        = NODE_COUNT,
	.ecu2canid         = ecu2canid_map,
	.key_server_id     = KEY_SERVER,
	.time_server_id    = TIME_SERVER,
	.time_div          = 1000000,
	.ack_timeout       = 1000000,
	.skey_validity     = 60000000,
	.skey_chg_timeout  = 5000000,
	.time_timeout      = 1000000,
	.time_bcast_period = 1000000,
	.time_delta        = 1000000,

	.node_id	   = 0xff, /* Invalid ID - to be replaced before macan initialization */
};

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

static void sig_callback(uint8_t sig_num, uint32_t sig_val)
{
	(void)sig_num, (void)sig_val;
	exit(0);
}

static void send_cb(struct ev_loop *loop, ev_timer *w, int revents)
{
	(void)loop; (void)revents;
	struct macan_ctx *ctx = w->data;
	static uint32_t i = 0;

	macan_send_sig(ctx, SIGNAL_0, i++);
}

static void print_frame_cb (macan_ev_loop *loop, macan_ev_can *w, int revents)
{
	(void)loop; (void)revents; (void)w; /* suppress warnings */
	struct can_frame cf;
	struct macan_ctx ctx = { .sockfd = w->fd, .config = &config };

	macan_read(&ctx, &cf);
	print_frame(&ctx, &cf);
}

struct node {
	struct macan_ctx ctx;
	struct macan_config cfg;
} node[NODE_COUNT];

int main(int argc, char *argv[])
{
	(void)argc; (void)argv;
	struct ev_loop *loop = MACAN_EV_DEFAULT;
	ev_timer sig_send;
	int i;

	for (i = 0; i < NODE_COUNT; i++) {
		int s = helper_init();
		node[i].cfg = config;
		node[i].cfg.node_id = (macan_ecuid)i;
		node[i].cfg.ltk = ltk[i];
		switch (i) {
		case KEY_SERVER:
			macan_init_ks(&node[i].ctx, &node[i].cfg, loop, s, ltk);
			break;
		case TIME_SERVER:
			macan_init_ts(&node[i].ctx, &node[i].cfg, loop, s);
			break;
		case SENDER:
		case RECEIVER:
		default:
			macan_init(&node[i].ctx, &node[i].cfg, loop, s);
		}
	}

	macan_reg_callback(&node[RECEIVER].ctx, SIGNAL_0, sig_callback, NULL);

	macan_ev_timer_init(&sig_send, send_cb, 100, 100);
	sig_send.data = &node[SENDER].ctx;
	macan_ev_timer_start(loop, &sig_send);

	macan_ev_can can_print;
	macan_ev_can_init (&can_print, print_frame_cb, helper_init(), EV_READ);
	macan_ev_can_start (loop, &can_print);

	macan_ev_run(loop);

	return 0;
}
