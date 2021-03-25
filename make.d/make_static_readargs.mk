# The readargs project (https://www.github.com/cjungmann/readargs.git)
# is setup to work as a share object library, but in many cases, it
# is better to build the library and attach it to a project statically.
# That avoids any issues with ensuring the availablility of the library.
#
# Thus, this makefile fragment will clone and build the project, then
# initialize a variable with link arguments that will add the library
# to your project

RA_USE_SHARED ?= 0
RA_HEADERS != find /usr -name readargs.h 2>/dev/null; [ 1 -eq 1 ]
# RA_SLIBRARY != find /usr -name libreadargs.so 2>/dev/null; 
# RA_INSTALLED != ra=${RA_HEADERS}; if [ $$ra ]; then echo 1; else echo 0; fi;

RA_TARGETS != if [ ${RA_USE_SHARED} -eq 0 ]; then echo "readargs/libreadargs.a"; fi;

RA_INC != if [ ${RA_USE_SHARED} -eq 0 ]; then echo "-I${PWD}/readargs"; fi
RA_LINK != if [ ${RA_USE_SHARED} -eq 0 ]; \
	then echo -L${PWD}/readargs/ -l:libreadargs.a; \
	else echo -lreadargs; \
	 fi

Confirm_Readargs: ${RA_TARGETS}
	@echo RA_HEADERS is ${RA_HEADERS}
	@echo RA_INC is ${RA_INC}
	@echo RA_LINK is ${RA_LINK}

readargs/libreadargs.a : readargs
	cd readargs; make

readargs:
	@echo "Clone readargs project"
	git clone https://www.github.com/cjungmann/readargs.git
