OUTDIR=build/linux/
KW=../keywrap/
INC=-Iinclude
LIB=-lnettle -lrt
.PHONY: test node2 node3

all: clean chk_build_folder node2 node3 keysvr timesvr test

chk_build_folder:
	mkdir -p ${OUTDIR}

node2:
	gcc -Wall -ggdb -o${OUTDIR}node2 -DCAN_IF=\"can2\" -DNODE_ID=2 ${INC} ${LIB} src/node/node.c src/aes_keywrap.c src/common.c src/aes_cmac.c src/macan.c

node3:
	gcc -Wall -ggdb -o${OUTDIR}node3 -DCAN_IF=\"can3\" -DNODE_ID=3 ${INC} ${LIB} src/node/node.c src/aes_keywrap.c src/common.c src/aes_cmac.c src/macan.c

keysvr:
	gcc -Wall -ggdb -o${OUTDIR}keysvr -DCAN_IF=\"can0\" -DNODE_ID=NODE_KS ${INC} ${LIB} src/keysvr/keysvr.c src/common.c src/aes_keywrap.c src/aes_cmac.c src/macan.c

timesvr:
	gcc -Wall -ggdb -o${OUTDIR}timesvr -DCAN_IF=\"can1\" -DNODE_OTHER=3 ${INC} ${LIB} -lrt src/timesvr/timesvr.c src/common.c src/aes_keywrap.c src/aes_cmac.c

test:
	gcc -Wall -ggdb -o${OUTDIR}/test_timesvr -DCAN_IF=\"can2\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} -lrt test/test_timesvr.c src/common.c src/aes_keywrap.c src/aes_cmac.c

clean:
	rm -f ${OUTDIR}node2
	rm -f ${OUTDIR}node3
	rm -f ${OUTDIR}keysvr
	rm -f ${OUTDIR}timesvr
	rm -f ${OUTDIR}test_timesvr
	if [ -d ${OUTDIR} ]; then rmdir ${OUTDIR}; fi;

