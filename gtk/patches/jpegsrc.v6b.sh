#!/bin/bash

# still loving libjpeg!

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

# copy the libtool files from $PREFIX
cp $PREFIX/share/libtool/config.sub $BASE_DIR/$BUILD_DIR/$PACKAGE/   || build_message_exit "Copy config.sub Failed"
cp $PREFIX/share/libtool/config.guess $BASE_DIR/$BUILD_DIR/$PACKAGE/ || build_message_exit "Copy config.guess Failed"

# some reason install won't create these!
mkdir -p $PREFIX/man/man{1,2,3,4,5,6,7,8,9} || build_message_exit "Man dir creation failed!"

exit 0;
