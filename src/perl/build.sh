#!/bin/bash
# uncomment set -x for debug
#set -x
#==========================================
# Find where we are
HERE=`dirname $0`
cd $HERE
#==========================================
# This works from the /ZMap/src/perl directory
HERE=`pwd`
 SRC=$HERE/..
ZMAP_INC="$SRC/include"
# oddly we need a .libs directory that contains symlinks to it's parent. Why?
ZMAP_LIB="$SRC/build/linux/.libs"

#==========================================
# chose a perl version
PERL_EXE=/usr/local/bin/perl


export CC=gcc

cd $HERE/X11-XRemote-0.01

if [ $1 ]; then
    PREFIX=$1
else
    PREFIX=`pwd`/PREFIX
fi

opsys=`uname`

case $opsys in
  OSF1 )
        locals=/nfs/disk100/acedb/ZMap/prefix/ALPHA/lib/pkgconfig
        PKG_CONFIG_PATH="/usr/local/gtk2/lib/pkgconfig"
        ZMAP_LIB="$SRC/build/alpha/lib"
        export PKG_CONFIG_PATH ;;
esac

$PERL_EXE Makefile.PL PREFIX=$PREFIX \
--with-zmap-inc "$ZMAP_INC" \
--with-zmap-libs "$ZMAP_LIB" \
--with-symbols || exit 1

make         || exit 1
make test    || exit 1
make install || exit 1
