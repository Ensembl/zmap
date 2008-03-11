#!/bin/bash

SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }


restore_dir=$(pwd)
backup=$(date +%Y%m%d)
PREFIX=/software/acedb
BACKUP=foocanvas-backup-$backup-$$.tar.gz

zmap_cd $PREFIX/lib

tar -zcf $PREFIX/src/$BACKUP libfoocanvas.*

zmap_cd $restore_dir

if [ -f $restore_dir/autogen.sh ]; then

    # We need to find freetype2.pc in here. No idea why.
    PKG_CONFIG_PATH="/usr/lib/pkgconfig:$PKG_CONFIG_PATH"

    zmap_register_prefix_in_env $GTK_PREFIX
    zmap_register_prefix_as_rpath $GTK_PREFIX

    ./autogen.sh || zmap_message_exit "autogen.sh failed."

    ./configure --prefix=$PREFIX || zmap_message_exit "configure failed"

    make || zmap_message_exit "make failed"
    
    make install || zmap_message_exit "make install failed"

    make dist || zmap_message_exit "make dist failed"
    
    make distclean || zmap_message_exit "make distclean failed"
fi

