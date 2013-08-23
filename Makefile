OUTDIR=build/linux/
KW=../keywrap/
INC=-Imacan/include -Idemo
LIB=-lnettle -lrt
#DEBUG=-DDEBUG_PRINT
.PHONY: test node2 node3

LIB_SRC=macan/src/target/linux/aes_keywrap.c macan/src/common.c macan/src/target/linux/aes_cmac.c macan/src/macan.c demo/macan_config.c macan/src/helper.c macan/src/target/linux/linux_macan.c

all: clean chk_build_folder node2 node3 keysvr timesvr test

chk_build_folder:
	mkdir -p ${OUTDIR}

debug:
	$(MAKE) DEBUG=-DDEBUG all

node2:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node2 -DNODE_ID=2 ${INC} ${LIB} demo/node.c $(LIB_SRC)

node3:
	gcc -Wall -ggdb ${DEBUG} -o${OUTDIR}node3 -DNODE_ID=3 ${INC} ${LIB} demo/node.c $(LIB_SRC)

keysvr:
	gcc -Wall -ggdb -o${OUTDIR}keysvr -DNODE_ID=KEY_SERVER ${INC} ${LIB} macan/apps/keysvr.c $(LIB_SRC)

timesvr:
	gcc -Wall -ggdb -o${OUTDIR}timesvr -DNODE_ID=TIME_SERVER ${INC} ${LIB} -lrt macan/apps/timesvr.c $(LIB_SRC)

test:
# 	gcc -Wall -ggdb -o${OUTDIR}/test_timesvr -DCAN_IF=\"can2\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} -lrt test/test_timesvr.c $(LIB_SRC)

clean:
	rm -f ${OUTDIR}node2
	rm -f ${OUTDIR}node3
	rm -f ${OUTDIR}keysvr
	rm -f ${OUTDIR}timesvr
	rm -f ${OUTDIR}test_timesvr
	if [ -d ${OUTDIR} ]; then rmdir ${OUTDIR}; fi;
