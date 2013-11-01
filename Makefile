DEMO_PATH=demos/$(DEMO)
OUTDIR=build/linux/$(DEMO)/
KW=../keywrap/
INC=-Imacan/include -I$(DEMO_PATH)
LIB=-lnettle -lrt
#DEBUG=-DDEBUG_PRINT
.PHONY: all demo01 demo02 node2 node3 chk_build_folder debug keysvr timesvr test clean
LIB_SRC=macan/src/target/linux/aes_keywrap.c macan/src/common.c macan/src/target/linux/aes_cmac.c $(DEMO_PATH)/macan_config.c macan/src/macan.c macan/src/helper.c macan/src/target/linux/linux_macan.c

all: clean demo01 demo02

### DEMO 01 ###

demo01: DEMO = demo01
demo01: demo01_chk_build_folder node2 node3 keysvr timesvr test

node2:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node2 -DNODE_ID=2 ${INC} ${LIB} ${DEMO_PATH}/node.c $(LIB_SRC)

node3:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node3 -DNODE_ID=3 ${INC} ${LIB} ${DEMO_PATH}/node.c $(LIB_SRC)

keysvr:
	gcc -Wall -ggdb -o${OUTDIR}keysvr -DNODE_ID=KEY_SERVER ${INC} ${LIB} macan/apps/keysvr.c $(LIB_SRC)

timesvr:
	gcc -Wall -ggdb -o${OUTDIR}timesvr -DNODE_ID=TIME_SERVER ${INC} ${LIB} -lrt macan/apps/timesvr.c $(LIB_SRC)

demo01_chk_build_folder:
	mkdir -p ${OUTDIR}

### DEMO 02 ###

demo02: DEMO = demo02
demo02: demo02_chk_build_folder demo02_all 

demo02_all:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node2 -DNODE_ID=2 ${INC} ${LIB} ${DEMO_PATH}/node.c $(LIB_SRC)
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node3 -DNODE_ID=3 ${INC} ${LIB} ${DEMO_PATH}/node.c $(LIB_SRC)
	gcc -Wall -ggdb -o${OUTDIR}keysvr -DNODE_ID=KEY_SERVER ${INC} ${LIB} macan/apps/keysvr.c $(LIB_SRC)
	gcc -Wall -ggdb -o${OUTDIR}timesvr -DNODE_ID=TIME_SERVER ${INC} ${LIB} -lrt macan/apps/timesvr.c $(LIB_SRC)

demo02_chk_build_folder:
	mkdir -p ${OUTDIR}

### COMMON ###

debug:
	$(MAKE) DEBUG=-DDEBUG all

test:
# 	gcc -Wall -ggdb -o${OUTDIR}/test_timesvr -DCAN_IF=\"can2\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} -lrt test/test_timesvr.c $(LIB_SRC)

clean:
	if [ -d ${OUTDIR} ]; then rm -rf ${OUTDIR}; fi;
#	rm -f ${OUTDIR}node2
#	rm -f ${OUTDIR}node3
#	rm -f ${OUTDIR}keysvr
#	rm -f ${OUTDIR}timesvr
#	rm -f ${OUTDIR}test_timesvr
#	if [ -d ${OUTDIR} ]; then rmdir ${OUTDIR}; fi;
