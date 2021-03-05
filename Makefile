TARGET = th

# Set implicit variables (info make -n implicit\ variables)
PREFIX ?= /usr/local
CFLAGS = -Wall -Werror -std=c99 -pedantic -m64 -ggdb
LDFLAGS =
LDLIBS = -lreadargs
SRC = src

# Set installation location variables
DB_NAME = thesaurus
DB_HOME = /var/local/lib/th
ETC_TARGET = /etc/th.conf

MODULES != ls -1 ${SRC}/*.c | sed 's/\.c/.o/g'

all: Confirm_DB5 Confirm_C_Patterns ${TARGET} ${DB_NAME}.db dict.db
	@echo "Making \"all\""

CP_NAMES = get_keypress prompter columnize read_file_lines
include make.d/make_c_patterns.mk
MODULES += ${CP_OBJECTS}

include make.d/make_db5.mk
CFLAGS += ${DB5_INC}
LDLIBS += ${DB5_LIB}

${TARGET} : ${MODULES}
	${CC} -o $@ $? ${LDFLAGS} ${LDLIBS}

%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<

install: ${DB_NAME}.db
	install -d ${DB_HOME}
	install -D --mode=744  ${DB_NAME}.* ${DB_HOME}
	install -D --mode=755  ${TARGET}    ${PREFIX}/bin
	echo -e ${DB_HOME}/${DB_NAME} > ${ETC_TARGET}

uninstall:
	rm -f ${ETC_TARGET}
	rm -f ${PREFIX}/bin/${TARGET}
	rm -rf ${DB_HOME}

clean:
	rm -f ${TARGET}
	rm -rf ${SRC}/*.o

full_clean:
	rm -f ${TARGET}
	rm -rf ${SRC}/*.o
	rm -rf ${CP_SOURCES} ${CP_HEADERS}

${DB_NAME}.db: files/mthesaur.txt
	@echo "Importing Moby Thesaurus"
	./th -Tv

files/mthesaur.txt:
	@echo "Downloading Moby Thesaurus"
	install -d files
	wget -nc -P files ftp://ftp.ibiblio.org/pub/docs/books/gutenberg/3/2/0/3202/files.zip
	unzip -n files/files.zip
