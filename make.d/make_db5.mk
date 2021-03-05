# This is primarily for BSD in which there may be some confusion
# over the Berkeley DB library.  Sometimes, I'm not sure how it
# happens, an old version of libdb.so and db.h are installed.  This
# makefile detects that situation and finds version 5 of libdb,
# warning the user of the need to install it if it is missing.

# USAGE:
# 1. Include this makefile
# 2. (Optional) Add Confirm_DB5 prerequisite to default rule
# 3. Add DB5_INC to CFLAGS, ie:
#    CFLAGS += ${DB5_INC}

DB_INSTALLED != find /usr -name db.h 2>/dev/null | grep /include/db.h
DB_IS_DB5 != a=${DB_INSTALLED}; if grep -q DB_VERSION_MAJOR "$$a"; then echo 1; else echo 0; fi; [ 1 -eq 1 ]

DB5_ALTERNATIVE != find /usr -name db.h 2>/dev/null | grep /include/db5/db.h
DB5_ALTERNATE_INC != d=${DB5_ALTERNATIVE}; echo "$${d%/*}"

DB5_LIBPATH != find /usr -name libdb.so 2>/dev/null | grep lib/db5
DB5_ALTERNATE_LIB != d=${DB5_LIBPATH}; echo "$${d%/*}"

DB5_USE_ALTERNATIVE != a=${DB5_ALTERNATIVE}; if [ -f "$$a" ]; then echo 1; else echo 0; fi; [ 1 -eq 1 ];

DB5_NEEDED != d=${DB_IS_DB5}; d5=${DB5_USE_ALTERNATIVE}; if [ $$d -eq 0 ] && [ $$d5 -eq 0 ]; then echo 1; else echo 0; fi; [ 1 -eq 1 ]

DB5_INC != d=${DB_IS_DB5}; dp=${DB5_ALTERNATE_INC}; if [ $$d -eq 0 ] && [ $$dp ]; then echo "-I$$dp"; fi; [ 1 -eq 1 ]

DB5_LIB != d=${DB_IS_DB5}; dp=${DB5_ALTERNATE_LIB}; if [ $$d -eq 0 ] && [ $$dp ]; then echo "-L$$dp"; fi; [ 1 -eq 1 ]
DB5_LIB += -ldb

DB5_MESSAGE != d=${DB5_NEEDED}; if [ $$d -eq 1 ]; then echo "@echo \\\"Use your package manager to install version 5 of the Berkeley Database.\\\"; exit 1"; else echo "@echo Appropriate version of Berkeley Database found at " ${DB5_ALTERNATE_LIB} ; fi; [ 1 -eq 1 ]


Confirm_DB5:
	${DB5_MESSAGE}

DB5_debug:
	@echo DB_INSTALLED is ${DB_INSTALLED}
	@echo DB_IS_DB5 is ${DB_IS_DB5}
	@echo DB5_ALTERNATIVE is ${DB5_ALTERNATIVE}
	@echo DB5_ALTERNATE_INC is ${DB5_ALTERNATE_INC}
	@echo DB5_NEEDED is ${DB5_NEEDED}
	@echo DB5_LIBPATH is ${DB5_LIBPATH}
	@echo DB5_ALTERNATE_LIB = ${DB5_ALTERNATE_LIB}
	@echo DB5_INC is ${DB5_INC}
	@echo DB5_LIB IS ${DB5_LIB}
