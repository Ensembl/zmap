#
# ZMap main makefile
#
# Can be used to build both the ZMap library and the ZMap application.
#


# Add directories to this list as they are sorted out.
MAKE_DIRS = zmapmanager

all: platform
	@for d in $(MAKE_DIRS); do \
	(cd $$d ; echo ; echo 'make '$@' in '$$d ; $(MAKE) $@ ; echo) ; \
	done

clean: forcemake platform
	@for d in $(MAKE_DIRS); do \
	(cd $$d ; echo ; echo 'make '$@' in '$$d ; $(MAKE) $@ ; echo) ; \
	done


# bootstrap stuff, without these make can't find the files to include which define
# all the macros/rules and dependencies.
#
# Must come after any default rule specified in this file as it includes
# some of its own commonly used rules.
#
ROOT_DIRECTORY = .
MAKEDEFS_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKEDEFS_DIR)/common.make
include $(MAKEDEFS_FILE)
