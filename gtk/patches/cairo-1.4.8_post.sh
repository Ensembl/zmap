#!/bin/bash

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

build_set_vars_for_prefix

# Only for universal builds
if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
    build_patch_libtool_dylib
    
    build_message_out "config.h and Makefile need patching..."
    
    $PERL -pi~ -e '\
s|/\* #undef FLOAT_WORDS_BIGENDIAN \*/|#ifdef __ppc__\n#define FLOAT_WORDS_BIGENDIAN 1\n#endif|g;\
s|/\* #undef WORDS_BIGENDIAN \*/|#ifdef __ppc__\n#define WORDS_BIGENDIAN 1\n#endif|g' config.h \
    || build_message_exit "Failed running $PERL on cairo's config.h"

    $PERL -pi~ -e '\
s|DIST_SUBDIRS = pixman src boilerplate test perf doc|DIST_SUBDIRS = pixman src test perf doc|g;\
s|am__append_1 = boilerplate test|am__append_1 = test|g' Makefile \
	|| build_message_exit "Failed running $PERL on cairo's Makefile"

fi


exit 0;
