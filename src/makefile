#
# ZMap main makefile
#
# Can be used to build both the ZMap library and the ZMap application.
#


# Add directories to this list as they are sorted out.
MAKE_DIRS = zmapmanager

all: makebindir
	@for d in $(MAKE_DIRS); do \
	(cd $$d ; echo ; echo 'make '$@' in '$$d ; $(MAKE) $@ ; echo) ; \
	done

clean: 
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
MAKE_DIR = $(ROOT_DIRECTORY)/zmapMake
include $(MAKE_DIR)/common.make




# actually this is all more trouble than its worth really...it would be better
# to have it in a build script really or a totally separate make file...as is
# done in acedb....Think about how to do this more elegantly...
#
# Dependency used by all makes to check that environment variable ZMAP_PLATFORM
# has been set and that the platform defs file actually exists in the make
# defs directory.
#
platform:
	@if [ "$(ZMAP_PLATFORM)" -a -f $(MAKEDEFS_FILE) ] ; \
	then \
	  if test ! -d $(BINDIR) ; \
	  then \
	    echo ; \
	    echo "making "$(BINDIR)" directory" ; \
	    mkdir $(BINDIR) ; \
	  fi \
	else \
	  echo ; \
	  echo 'To compile you must set the environment variable "ZMAP_PLATFORM"' ; \
	  echo 'to one of the following:' ; \
	  echo ; cd $(MAKEDEFS_DIR) ; \
	  for i in `ls *$(MAKEDEFS_SUFFIX)` ; do \
	    echo `basename $$i $(MAKEDEFS_SUFFIX)` ; \
	  done ; \
	  echo ; exit 1 ; \
	fi


