# This makefile clones a library and links several files
# into the ${SRC} directory.  It is a demonstration of
# prerequisite-less rule invoked by another rule's prerequisites.

# This example has a specific objective in addition to
# its instructive features.  The USAGE particularly informs
# usage of the specific repository.
# USAGE:
# 1. Copy this makefile where your main makefile can include it.
# 2. Define a variable with a list of desired module names,
#    not including the .c or .h extensions.
# 3. Put the *include make_c_patterns.mk* statement
#    AFTER the main default rule (e.g. all: ${TARGET})
#    so make doesn't use included rules as the default rule.
# 4. Add ${CP_OBJECTS} to the prerequisites of the target
#    in which the c_patterns' modules are to be incorporated.
# 5. For cleaning targets, you can use ${CP_SOURCES} and ${CP_HEADERS}
#    to remove links to .c and .h files, respectively.
#
# Usage Example makefile fragment, assuming MODULES contains objects to link:
#
# MODULES != ls -1 ${SRC}/*.c | sed 's/\.c/.o/g'
# ...
# CP_NAMES = get_key-ress prompter columnize
# include make.d/make_c_patterns.mk
# MODULES += ${CP_OBJECTS}


CP_SOURCES != echo ${CP_NAMES} | sed -E 's/([^[:space:]]+)/${SRC}\/\1.c/g'
CP_HEADERS != echo ${CP_SOURCES} | sed -E 's/\.c/\.h/g'
CP_OBJECTS != echo ${CP_SOURCES} | sed -E 's/\.c/\.o/g'

CP_LN_RECIPE != echo ${CP_NAMES} | sed -E 's/([^[:space:]]+)/ln -sf c_patterns\/\1.c ${SRC}\/\1.c\\;/g'

${CP_SOURCES}: c_patterns
	@echo Making link from c_patterns to $@ for $(*F)
	ln -fs ${PWD}/c_patterns/$(*F).c ${PWD}/${SRC}/$(*F).c
	ln -fs ${PWD}/c_patterns/$(*F).h ${PWD}/${SRC}/$(*F).h

c_patterns:
	@echo "Clone c_patterns project and link requested files to src directory."
	git clone http://www.github.com/cjungmann/c_patterns.git

Confirm_C_Patterns: ${CP_SOURCES}
