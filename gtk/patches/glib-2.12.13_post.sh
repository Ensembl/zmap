#!/bin/bash

# This fix is all about compiling universal version of glib2 on powerpc.

# The problem is G_ATOMIC_POWERPC is defined in config.h during the
# configure process.  When it comes to compile the i386 version of
# gatomic.c the powerpc assembler code is used and this is clearly
# wrong. The result is the following error:
# error: unknown register name 'cr0' in 'asm'

# Protecting the G_ATOMIC_POWERPC with the builtin __ppc__ seemed
# like the best thing to do. config.h should end up with the following
# block when G_ATOMIC_POWERPC is defined...

#ifdef __ppc__
#define G_ATOMIC_POWERPC
#endif

# I've also incorporated macports' configure.patch
# http://trac.macports.org/ticket/15816
# N.B. this has been altered to fit this version of glib!

# There's still some worrying warnings about g_atomic_int functions
# having differing signedness...
# Possibly due to bigendian test returning no @ configure time???

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh

# and the functions
. $BASE_DIR/build_functions.sh

build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

# copy a working copy
cp config.h config.h.w
# protect the G_ATOMIC_POWERPC
sed -e 's/\(#define G_ATOMIC_POWERPC\)/#ifdef __ppc__\n\1\n#endif/' > config.h < config.h.w

exit 0;
