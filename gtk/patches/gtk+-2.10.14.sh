#!/bin/bash

# still loving libjpeg!

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh

# and the functions
. $BASE_DIR/build_functions.sh

build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

$AUTOHEADER || build_message_out "Failed to autoheader!"
$AUTOMAKE   || build_message_out "Failed to automake!"
$AUTOCONF   || build_message_out "Failed to autoconf!"

exit 0;
