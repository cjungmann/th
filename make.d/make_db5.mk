# This is primarily for BSD in which there may be some confusion
# over the Berkeley DB library.  Sometimes, I'm not sure how it
# happens, an old version of libdb.so and db.h are installed.  This
# makefile detects that situation and finds version 5 of libdb,
# warning the user of the need to install it if it is missing.

# USAGE:
# 1. Include this makefile after the default rule.
# 2. (Optional) Add Confirm_DB5 prerequisite to default rule.
#    This will generate an error and exit if libdb-5.so is not
#    available.  Omit this step if you want to ignore a missing
#    library.
# 3. Use output variables DB5_INC and DB5_LINK for compiling and
#    linking, respectively.  Generally, you'll concatenate each
#    to its respective default values, CFLAGS and LDLIBS
#
# Example:
#
# Initialize implicit rule default values:
# CFLAGS = -Wall -Werror -pedantic -ggdb
# LDLIBS = 
#
# all: Confirm_DB5 ${TARGET}
#
# include make_db5.mk
# CFLAGS += ${DB5_INC}
# LDLIBS += ${DB5_LINK}
#
# ${TARGET}: ${MODULES}
#	${CC} -o $@ $? ${DB5_LINK}
# 

# The first set of variables are used to query
# the environment, setting flags for "output"
# variables DB5_INC and DB5_LINK:
DB_INSTALLED != find /usr -name db.h 2>/dev/null | grep /include/db.h
DB_IS_DB5 != a=${DB_INSTALLED}; if grep -q DB_VERSION_MAJOR "$$a"; then echo 1; else echo 0; fi; [ 1 -eq 1 ]
DB5_ALTERNATIVE != find /usr -name db.h 2>/dev/null | grep /include/db5/db.h
DB5_ALTERNATIVE_PATH != d=${DB5_ALTERNATIVE}; echo "$${d%/*}"
DB5_USE_ALTERNATIVE != a=${DB5_ALTERNATIVE}; if [ -f "$$a" ]; then echo 1; else echo 0; fi; [ 1 -eq 1 ];
DB5_ALTERNATIVE_LIB != find /usr -name libdb.so 2>/dev/null | grep "lib/db5"; [ 1 -eq 1 ]

DB5_NEEDED != d=${DB_IS_DB5}; d5=${DB5_USE_ALTERNATIVE}; if [ $$d -eq 0 ] && [ $$d5 -eq 0 ]; then echo 1; else echo 0; fi; [ 1 -eq 1 ]

# Feedback variable with contain a command if the builder
# needs to take some action, or empty if everything is good.
DB5_MESSAGE != d=${DB5_NEEDED}; if [ $$d -eq 1 ]; then echo "@echo \\\"Use your package manager to install version 5 of the Berkeley Database.\\\"; exit 1"; else echo "@echo Appropriate version of Berkeley Database found."; fi; [ 1 -eq 1 ]

# Build output variable
DB5_INC != d=${DB_IS_DB5}; dp=${DB5_ALTERNATIVE_PATH}; if [ $$d -eq 0 ] && [ $$dp ]; then echo "-I$$dp"; fi;

# Build output variable
DB5_LINK != du=${DB5_USE_ALTERNATIVE}; dal=${DB5_ALTERNATIVE_LIB}; if [ $$du -eq 1 ] && [ "$$dal" ]; then echo "-L$${dal%/*}"; fi;
DB5_LINK += -ldb

# This rule, if included as a prerequisite in the default rule,
# will display a message if necessary.
.PHONY: Confirm_DB5
Confirm_DB5:
	${DB5_MESSAGE}

