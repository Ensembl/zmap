#!/bin/bash

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

build_set_vars_for_prefix

# This is all quite tedious and just really to create a configure
# script for the foocanvas as they haven't got a distribution. We also
# need to run this as we patch the foocanvas, so we could do with 
# recreating configure script anyway.

# Need to add the m4 files we've installed into the equation
export ACLOCAL_FLAGS="-I $PREFIX/share/aclocal"

# cd to the libfoocanvas directory
build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

# some macs have a good version of automake installed here
if [ -x /sw/bin/automake ]; then
    PATH=/sw/bin:$PATH
fi

# do a pre check because ./autogen.sh isn't too helpful on exiting as
# soon as it finds out something is gonna fail.
build_message_out "Checking for automake..."

AM_VERSION=$(automake --version | grep 'GNU automake' | awk '{print $4}')
AM_MAJOR=$(echo -n $AM_VERSION | awk -F. '{print $1}')
AM_MINOR=$(echo -n $AM_VERSION | awk -F. '{print $2}')
AM_SUB=$(echo -n $AM_VERSION | awk -F. '{print $3}')

if test $AM_MAJOR -ge 1 -a $AM_MINOR -ge 9 ; then
    build_message_out "Automake $AM_VERSION looks ok"
else
    build_message_exit "Error automake $AM_VERSION looks older than 1.9. Impending ./autogen.sh will fail!";
fi

build_message_out "Running ./autogen.sh $CONFIGURE_OPTS"
build_message_out "./autogen.sh runs gnome-autogen.sh which doesn't have helpful error messages..."
build_message_out "If ./autogen.sh fails, check for the _first_ error below here (likely old automake version)."

./autogen.sh $CONFIGURE_OPTS || build_message_exit "Failed to run autogen.sh! Please check for the _first_ error."

exit 0;
