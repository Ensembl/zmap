#!/bin/sh
# uncomment set -x for debug
#set -x
#==========================================
# Find where we are
HERE=`pwd`
 SRC=`dirname $HERE`
#==========================================
# This works from the /ZMap/src/perl directory
ZMAP_INC="$SRC/include"
ZMAP_LIB="$SRC/build/linux/lib"
#==========================================
# chose a perl version
    PERL_EXE=/usr/local/bin/perl
# That should be it
#==========================================
   PERL_ARCH=`$PERL_EXE -MConfig -e 'print $Config{archname}'`
PERL_VERSION=`$PERL_EXE -MConfig -e 'print $Config{version}'`
PERL_INSTALL=`$PERL_EXE -MConfig -e 'print $Config{installstyle}'`
#==========================================
#==========================================

cd X11-XRemote-0.01/X11/XRemote

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


ADD_PERL5LIB=$PREFIX/$PERL_INSTALL/site_perl/$PERL_VERSION/$PERL_ARCH

$PERL_EXE Makefile.PL PREFIX=$PREFIX \
--with-zmap-inc "-I$ZMAP_INC" \
--with-zmap-libs "-L$ZMAP_LIB" \
--with-symbols 

make
make install

PERL5LIB=$ADD_PERL5LIB:$PERL5LIB
export PERL5LIB

echo 
echo "Testing installation..."
echo "=================================="
$PERL_EXE ./debug_test.pl
echo "=================================="
echo
echo "Make sure $ADD_PERL5LIB is in your PERL5LIB"
