#include "cfg.h"

/**********************/
/* MaCAN onfiguration */
/**********************/

const struct macan_sig_spec test_sig_spec[] = {
	[SIGNAL_0]    = {.can_nsid = 0,   .can_sid = 0x516,   .src_id = ECU_I, .dst_id = ECU_J, .presc = 0},
};

const struct macan_can_ids test_can_ids = {
	.time = 0x000,
	.ecu = (struct macan_ecu[]){
		[KEY_SERVER]  = {0x100,"KS"},
		[TIME_SERVER] = {0x101,"TS"},
		[ECU_I]       = {0x102,"I"},
		[ECU_J]       = {0x103,"J"},
	},
};

const struct macan_config config = {
	.sig_count         = SIG_COUNT,
	.sigspec           = test_sig_spec,
	.node_count        = NODE_COUNT,
	.canid		   = &test_can_ids,
	.key_server_id     = KEY_SERVER,
	.time_server_id    = TIME_SERVER,
	.time_div          = 1000000,
	.skey_validity     = 60000000,
	.skey_chg_timeout  = 5000000,
	.time_req_sep      = 1000000,
	.time_delta        = 1000000,
};
