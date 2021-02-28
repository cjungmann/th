TARGET = th

PREFIX ?= /usr/local
CFLAGS = -Wall -Werror -std=c99 -pedantic -m64 -ggdb
SRC = src

TH_HOME = ${PREFIX}/bin
DB_HOME = /var/local/lib/th
DB_NAME = thesaurus
ETC_TARGET = /etc/th.conf
ETC_CONTENT = ${DB_HOME}/${DB_NAME}

# Bolster modules list with files from github.com/cjungmann/c_patterns.git:
EXTRA_SOURCE != if ! [ -e ${SRC}/get_keypress.c ]; then echo ${SRC}/get_keypress.c ${SRC}/columnize.c ${SRC}/prompter.c; fi

LIB_SOURCE != ls -1 ${SRC}/*.c
LIB_SOURCE := ${EXTRA_SOURCE} ${LIB_SOURCE}
LIB_MODULES != echo ${LIB_SOURCE} | sed 's/\.c/\.o/g'
LIBS = -ldb -lreadargs

# (for FreeBSD, which uses a too old version)
# Check for and use version 5 of Berkeley Database
DB_DEFAULT_OK != if grep -q "VERSION_MAJOR[[:space:]]\+[[:digit:]]" /usr/include/db.h; then echo 1; else echo 0; fi
DB_EXPLICIT_DB5 != if v=$$( find /usr -name db.h 2>/dev/null | grep db5 ); then echo "$$v"; fi; [ 1 -eq 1 ]
DB_NEED_DB5 != d=${DB_DEFAULT_OK}; e=${DB_EXPLICIT_DB5}; if [ "$$d" -eq 0 ] && [ -z "$$e" ]; then echo 1; else echo 0; fi
DB5_WARNING != n=${DB_NEED_DB5}; if [ "$$n" -eq 1 ]; then echo "@echo You need Berkeley db version 5.  Please install db5, then remake.; exit 1"; fi

# Add -I include path, if found
DB5_INCLUDE != o=${DB_DEFAULT_OK}; v=${DB_EXPLICIT_DB5}; if [ "$$o" -eq 0 ] && [ -n "$$v" ]; then echo -I"$${v%/*}"; fi
# Add -L library path, if found
DB5_LIBRARY_PATH != if v=$$( find /usr -name libdb.so 2>/dev/null | grep db5 ); then echo "$$v"; fi; [ 1 -eq 1 ]
DB5_LIB != o=${DB_DEFAULT_OK}; v=${DB5_LIBRARY_PATH}; if [ "$$o" -eq 0 ] && [ -n "$$v" ]; then echo -L"$${v%/*}"; fi

PULL_C_PATTERNS != if [ -d "c_patterns" ] ; then echo "cd c_patterns; git pull --ff-only origin"; fi; [ 1 -eq 1 ]

CFLAGS := ${CFLAGS} ${DB5_INCLUDE}

.PHONY: all
all: preamble ${TARGET} thesaurus.db files/count_1w.txt # dict.db

${TARGET}: ${LIB_MODULES}
	${CC} ${CFLAGS} ${DB5_LIB} -o $@ ${LIB_MODULES} ${LIBS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

.PHONY: test
test:
	@echo DB_DEFAULT_OK is ${DB_DEFAULT_OK}
	@echo DB_EXPLICIT_DB5 is ${DB_EXPLICIT_DB5}
	@echo DB_NEED_DB5 is ${DB_NEED_DB5}
	@echo DB5_WARNING is ${DB5_WARNING}
	@echo DB5_INCLUDE param is ${DB5_INCLUDE}
	@echo DB5_LIB param is ${DB5_LIB}
	@echo PULL_C_PATTERNS param is ${PULL_C_PATTERNS}


.PHONY: preamble
preamble:
	${DB5_WARNING}
	${PULL_C_PATTERNS}

# Link from c_patterns
${SRC}/get_keypress.c:
	@echo "Getting lastest c_patterns modules."
	git clone http://www.github.com/cjungmann/c_patterns.git
	cp -s ${PWD}/c_patterns/get_keypress.* src
	cp -s ${PWD}/c_patterns/columnize.* src
	cp -s ${PWD}/c_patterns/prompter.* src

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
	install -D --mode=755    ${TARGET}     ${TH_HOME}
	install -D --mode=744 -t ${DB_HOME}    ${DB_NAME}.*
	echo -e ${ETC_CONTENT} > ${ETC_TARGET}

uninstall:
	rm -f  ${TH_HOME}/${TARGET}
	rm -f  ${ETC_TARGET}
	rm -rf ${DB_HOME}

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
	rm -f src/prompter.*
	rm -rf c_patterns

