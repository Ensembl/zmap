#!/bin/bash

# still loving libjpeg!

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh

# and the functions
. $BASE_DIR/build_functions.sh

build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

AUTOUPDATE=autoupdate

build_message_out "Running $AUTOUPDATE"
$AUTOUPDATE                       || build_message_err "Failed to $AUTOUPDATE!"
build_message_out "Running $AUTOHEADER"
$AUTOHEADER                       || build_message_err "Failed to $AUTOHEADER!"
#build_message_out "Running $ACLOCAL"
#$ACLOCAL -I $PREFIX/share/aclocal || build_message_err "Failed to $ACLOCAL!"
build_message_out "Running $AUTOMAKE"
$AUTOMAKE --gnu                   || build_message_err "Failed to $AUTOMAKE!"
build_message_out "Running $AUTOCONF"
$AUTOCONF                         || build_message_err "Failed to $AUTOCONF!"

exit 0;
