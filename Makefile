.SHELL: ${/usr/bin/env bash}

TARGET = th

TH_HOME=/usr/local/bin
DB_HOME=/var/local/lib/th
DB_NAME=thesaurus
ETC_TARGET=/etc/th.conf
ETC_CONTENT="${DB_HOME}/${DB_NAME}"

CFLAGS = -Wall -Werror -std=c99 -pedantic -m64 -ggdb
SRC=./src/

LIB_SOURCE != ls -1 ${SRC}*.c
LIB_MODULES != ls -1 ${SRC}*.c | sed 's/\.c/\.o/g'
LIBS = -ldb -lreadargs

.PHONY: all
all: ${TARGET} thesaurus.db files/count_1w.txt

${TARGET}: ${LIB_MODULES}
	${CC} ${CFLAGS} -o $@ ${LIB_MODULES} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: thesaurus.db
thesaurus.db : files/mthesaur.txt
	@echo "Importing Moby Thesaurus into *th*"
	./th -Tv

files/mthesaur.txt:
	@echo "Downloading Moby Thesaurus source data from Gutenberg.org"
	install -d files
	wget -nc -P files ftp://ftp.ibiblio.org/pub/docs/books/gutenberg/3/2/0/3202/files.zip
	unzip -n files/files.zip

files/count_1w.txt:
	@echo "Downloading word count document from norvig.com"
	wget -nc -P files https://norvig.com/ngrams/count_1w.txt

install:
	install -D --mode=755 ${TARGET}        ${TH_HOME}
	install -D --mode=744 -t ${DB_HOME}  ${DB_NAME}.*
	echo -e "${ETC_CONTENT}" > "${ETC_TARGET}"

uninstall:
	rm -f "/usr/local/bin/${TARGET}"
	rm -f "${ETC_TARGET}"
	rm -rf "${DB_HOME}"

clean:
	rm -f th
	rm -f src/*.o
	rm -f src/bdb
	rm -f src/parse
	rm -f src/istringt
	rm -f src/ivtable
	rm -f src/aaa.*
	rm -f src/rrtable
	rm -f src/test.db
	rm -f src/thesaurus
	rm -f src/utils

