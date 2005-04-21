#!/bin/sh
#set -x
#==========================================
# Find where we are
HERE=`pwd`
 SRC=`dirname $HERE`
#==========================================
# This works for alpha builds at the moment
#
ZMAP_INC="$SRC/include"
ZMAP_LIB="$SRC/build/alpha/lib"
#==========================================
# chose a perl version
    PERL_EXE=/usr/local/bin/perl
   PERL_ARCH=`$PERL_EXE -MConfig -e 'print $Config{archname}'`
PERL_VERSION=`$PERL_EXE -MConfig -e 'print $Config{version}'`
#==========================================
#==========================================

cd X11-XRemote-0.01/X11/XRemote

if [ $1 ]; then
    PREFIX=$1
else
    PREFIX=`pwd`/PREFIX
fi
ADD_PERL5LIB=$PREFIX/lib/perl5/site_perl/$PERL_VERSION/$PERL_ARCH:\
$PREFIX/lib/site_perl/$PERL_VERSION/$PERL_ARCH

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
./debug_test.pl
echo "=================================="
echo
echo "Make sure $ADD_PERL5LIB is in your PERL5LIB"
