OUTDIR=build/linux/
KW=../keywrap/
INC=-Iinclude
LIB=-lnettle

all: chk_build_folder node keysvr
chk_build_folder:
	mkdir -p ${OUTDIR}

node: ${OUTDIR}node
${OUTDIR}node:
	gcc -Wall -ggdb -o${OUTDIR}node -DCAN_IF=\"can1\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} src/node/node.c src/aes_keywrap.c src/common.c src/aes_cmac.c

node2: ${OUTDIR}node2
${OUTDIR}node2:
	gcc -Wall -ggdb -o${OUTDIR}node -DCAN_IF=\"can1\" -DNODE_ID=3 -DNODE_OTHER=2 ${INC} ${LIB} src/node/node.c src/aes_keywrap.c src/common.c src/aes_cmac.c

keysvr:
	gcc -Wall -ggdb -o${OUTDIR}keysvr ${INC} ${LIB} src/keysvr/keysvr.c src/common.c src/aes_keywrap.c
clean:
	rm -f ${OUTDIR}node
	rm -f ${OUTDIR}keysvr
	if [ -d ${OUTDIR} ]; then rmdir ${OUTDIR}; fi;

