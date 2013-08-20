OUTDIR=build/linux/
KW=../keywrap/
INC=-Iinclude
LIB=-lnettle -lrt
#DEBUG=-DDEBUG_PRINT
.PHONY: test node2 node3

LIB_SRC=src/aes_keywrap.c src/common.c src/aes_cmac.c src/macan.c src/macan_cfg.c

all: clean chk_build_folder node2 node3 keysvr timesvr test

chk_build_folder:
	mkdir -p ${OUTDIR}

debug:
	$(MAKE) DEBUG=-DDEBUG all

node2:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node2 -DCAN_IF=\"can2\" -DNODE_ID=2 ${INC} ${LIB} src/node/node.c $(LIB_SRC)

node3:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node3 -DCAN_IF=\"can3\" -DNODE_ID=3 ${INC} ${LIB} src/node/node.c $(LIB_SRC)

keysvr:
	gcc -Wall -ggdb -o${OUTDIR}keysvr -DCAN_IF=\"can0\" -DNODE_ID=NODE_KS ${INC} ${LIB} src/keysvr/keysvr.c $(LIB_SRC)

timesvr:
	gcc -Wall -ggdb -o${OUTDIR}timesvr -DCAN_IF=\"can1\" -DNODE_ID=NODE_TS -DNODE_OTHER=3 ${INC} ${LIB} -lrt src/timesvr/timesvr.c $(LIB_SRC)

test:
# 	gcc -Wall -ggdb -o${OUTDIR}/test_timesvr -DCAN_IF=\"can2\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} -lrt test/test_timesvr.c $(LIB_SRC)

clean:
	rm -f ${OUTDIR}node2
	rm -f ${OUTDIR}node3
	rm -f ${OUTDIR}keysvr
	rm -f ${OUTDIR}timesvr
	rm -f ${OUTDIR}test_timesvr
	if [ -d ${OUTDIR} ]; then rmdir ${OUTDIR}; fi;
