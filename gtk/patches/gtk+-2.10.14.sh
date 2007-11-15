#!/bin/bash

# still loving libjpeg!

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

autoheader || build_message_out "Failed to autoheader!"
#automake   || build_message_out "Failed to automake!"
autoconf   || build_message_out "Failed to autoconf!"

exit 0;
