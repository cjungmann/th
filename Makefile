.SHELL: ${/usr/bin/env bash}

TARGET = th

TH_HOME=/usr/local/bin
DB_HOME=/var/local/lib/th
DB_NAME=thesaurus
ETC_TARGET=/etc/th.conf
ETC_CONTENT="${DB_HOME}/${DB_NAME}"

CFLAGS = -Wall -Werror -std=c99 -pedantic -m64 -ggdb
SRC=./src/

EXTRA_SOURCE != if ! [ -e ${SRC}get_keypress.c ]; then echo ${SRC}get_keypress.c ${SRC}columnize.c; fi

LIB_SOURCE != ls -1 ${SRC}*.c
LIB_SOURCE := ${EXTRA_SOURCE} ${LIB_SOURCE}
LIB_MODULES != echo ${LIB_SOURCE} | sed 's/\.c/\.o/g'
LIBS = -ldb -lreadargs

.PHONY: all
all: ${TARGET} thesaurus.db dict.db files/count_1w.txt

${TARGET}: ${LIB_MODULES}
	${CC} ${CFLAGS} -o $@ ${LIB_MODULES} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

# Link from c_patterns
${SRC}get_keypress.c:
	@echo "Getting lastest c_patterns modules."
	git clone http://www.github.com/cjungmann/c_patterns.git
	cp -s ${PWD}/c_patterns/get_keypress.* src
	cp -s ${PWD}/c_patterns/columnize.* src

# Download and import thesaurus entries
thesaurus.db : files/mthesaur.txt
	@echo "Importing Moby Thesaurus into *th*"
	./th -Tv

files/mthesaur.txt:
	install -d files
	@echo "Downloading Moby Thesaurus source data from Gutenberg.org"
	wget -nc -P files ftp://ftp.ibiblio.org/pub/docs/books/gutenberg/3/2/0/3202/files.zip
	unzip -n files/files.zip

# Download and import word frequency entries
files/count_1w.txt:
	install -d files
	@echo "Downloading word count document from norvig.com"
	wget -nc -P files https://norvig.com/ngrams/count_1w.txt

# Download and import dictionary entries
.PHONY: cleand
cleand:
	rm -rf dict.db
	# rm -rf files/gcide
	rm -rf files/dict_raw.xml
	rm -rf files/dict.xml

dict.db: files/dict.xml
	touch dict.db

files/dict.xml : files/gcide/CIDE.A
	scripts/gcide_preprocess_to_dict_xml files/gcide files/dict.xml

files/gcide/CIDE.A:
	install -d files
	@echo "Downloading GCide dictionary from gnu.org"
	wget -nc "ftp://ftp.gnu.org/gnu/gcide/gcide-0.52.tar.xz"
	unxz gcide-0.52.tar.xz
	tar -xf gcide-0.52.tar
	mv gcide-0.52 files/gcide

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

	rm -f src/get_keypress.*
	rm -f src/columnize.*
	rm -rf c_patterns

