#!/bin/bash

# still loving libjpeg!

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

build_set_vars_for_prefix

export ACLOCAL_FLAGS="-I $PREFIX/share/aclocal"

build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

./autogen.sh $CONFIGURE_OPTS || build_message_exit "Failed to run autogen.sh"

exit 0;
