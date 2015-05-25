#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "macan_private.h"
#include "can_frame.h"

#include "klee.h"

uint64_t read_time(void){
	uint64_t time;
	klee_make_symbolic(&time, sizeof(time), "time");
	return time;
}

bool gen_rand_data(void* dest, size_t len){
	//Amusingly enough, depending on the random number generator, 0 might not be in fact a possible result.
	memset(dest, 0, len);
	return true;
}

bool macan_read(struct macan_ctx* ctx, struct can_frame* cf){
	(void)ctx;
	//It is literally impossible for symbolic read to fail.
	klee_make_symbolic(cf, sizeof(struct can_frame), "incoming can frame");
	static int counter = 0;
	counter++;
	counter %= 10;
	return counter != 0;
}

//For the klee false target, this is a noop.
void macan_target_init(struct macan_ctx* ctx){
	(void)ctx;
}

//Not currently part of testing.
bool macan_send(struct macan_ctx* ctx, const struct can_frame* cf){
	(void)ctx, (void)cf;
	return true;
}
