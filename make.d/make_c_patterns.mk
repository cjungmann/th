# This makefile clones a library and links several files
# into the ${SRC} directory.  It is a demonstration of
# prerequisite-less rule invoked by another rule's prerequisites.

# This example has a specific objective in addition to
# its instructive features.  The USAGE particularly informs
# usage of the specific repository.
# USAGE:
# 1. Include CP_PREPARE_SOURCES before the main target in your
#    default rule.
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
# Example:
#
# MODULES != ls -1 ${SRC}/*.c | sed 's/\.c/.o/g'
#
# all: CP_PREPARE_SOURCES ${TARGET}
#
# CP_NAMES = get_keypress prompter columnize
# include make_c_patterns.mk
# MODULES += ${CP_OBJECTS}
#
# ${TARGET}: ${MODULES}
#	linking-recipe

# Ignoring $(CP_NAMES:=.o) in favor of using != for BSD portability
CP_SOURCES != echo ${CP_NAMES} | sed -E 's/([^[:space:]]+)/${SRC}\/\1.c/g'
CP_HEADERS != echo ${CP_SOURCES} | sed -E 's/\.c/\.h/g'
CP_OBJECTS != echo ${CP_SOURCES} | sed -E 's/\.c/\.o/g'

# Detect if links must be established:
CP_LINKS_REGEX != echo ${CP_NAMES} | sed -E 's/([^[:space:]]+)/\1.o/g' | sed -E 's/[[:space:]]+/\\|/g'
CP_LINKS_FOUND != ls -1 ${SRC} | grep -E \(${CP_LINKS_REGEX}\); [ 1 -eq 1 ]
CP_NAMES_COUNT != echo ${CP_NAMES} | wc -w
CP_SOURCES_COUNT != ls -1 ${CP_OBJECTS} 2>/dev/null | wc -l
CP_SOURCE_LINKS_NEEDED !=if [ ${CP_NAMES_COUNT} -gt ${CP_SOURCES_COUNT} ]; then echo 1; else echo 0; fi;

# This variable will contain a list of names for making links
# if the links are not detected.  If not empty, it will be a
# list of artificial prerequisite that will trigger the links.
CP_LINK_PREREQS != if [ ${CP_SOURCE_LINKS_NEEDED} -eq 1 ]; then echo ${CP_NAMES}; fi;

# Create targets of C files if links needed.
# We need to do this to aid BSD make to identify these targets
CP_SOURCE_TARGETS != if [ ${CP_SOURCE_LINKS_NEEDED} -eq 1 ]; then echo ${CP_SOURCES}; fi;

# Initiate the link-making
CP_PREPARE_SOURCES: ${CP_LINK_PREREQS} ${CP_SOURCE_TARGETS}
CP_Prepare_Sources: ${CP_LINK_PREREQS} ${CP_SOURCE_TARGETS}

${CP_NAMES}: c_patterns
	@echo Making link from c_patterns to $@
	@ln -fs ${PWD}/c_patterns/$@.c ${PWD}/${SRC}/$@.c
	@ln -fs ${PWD}/c_patterns/$@.h ${PWD}/${SRC}/$@.h

pull_c_patterns: c_patterns
	git -C c_patterns pull origin

c_patterns:
	@echo "Clone c_patterns project and link requested files to src directory."
	git clone http://www.github.com/cjungmann/c_patterns.git

