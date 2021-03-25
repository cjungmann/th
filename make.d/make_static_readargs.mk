# The readargs project (https://www.github.com/cjungmann/readargs.git)
# is setup to work as a share object library, but in many cases, it
# is better to build the library and attach it to a project statically.
# That avoids any issues with ensuring the availablility of the library.
#
# Thus, this makefile fragment will clone and build the project, then
# initialize a variable with link arguments that will add the library
# to your project
#
# Example:
#
# Initialize implicit rule default values:
# CFLAGS = -Wall -Werror -pedantic -ggdb
# LDLIBS = 
#
# all: Confirm_Readargs ${TARGET}
#
# include make_static_readargs.mk
# CFLAGS += ${RA_INC}
# LDLIBS += ${RA_LINK}
#
# ${TARGET}: ${MODULES}
#	${CC} -o $@ ${MODULES} ${LDLIBS}

RA_USE_SHARED ?= 0
RA_HEADERS != find /usr -name readargs.h 2>/dev/null; [ 1 -eq 1 ]
# RA_SLIBRARY != find /usr -name libreadargs.so 2>/dev/null; 
# RA_INSTALLED != ra=${RA_HEADERS}; if [ $$ra ]; then echo 1; else echo 0; fi;

RA_TARGETS != if [ ${RA_USE_SHARED} -eq 0 ]; then echo "readargs/libreadargs.a"; fi;

RA_INC != if [ ${RA_USE_SHARED} -eq 0 ]; then echo "-I${PWD}/readargs/src"; fi
RA_LINK != if [ ${RA_USE_SHARED} -eq 0 ]; \
	then echo -L${PWD}/readargs/ -l:libreadargs.a; \
	else echo -lreadargs; \
	 fi

Confirm_Readargs: ${RA_TARGETS}

readargs/libreadargs.a : readargs
	cd readargs; make

readargs:
	@echo "Clone readargs project"
	git clone https://www.github.com/cjungmann/readargs.git
