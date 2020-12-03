.SHELL: ${/usr/bin/env bash}

TARGET = words

CFLAGS = -Wall -Werror -std=c99 -pedantic -m64 -ggdb
SRC=./src/

LIB_SOURCE != ls -1 ${SRC}*.c
LIB_MODULES != ls -1 ${SRC}*.c | sed 's/\.c/\.o/g'
LIBS = -ldb -lreadargs


all: ${TARGET}

${TARGET}: ${LIB_MODULES}
	${CC} ${CFLAGS} -o $@ ${LIB_MODULES} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	rm -f src/*.o
	rm -f src/bdb
	rm -f src/istringt
