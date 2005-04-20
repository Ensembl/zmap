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
PERL=/usr/local/bin/perl

#==========================================
#==========================================


cd X11-XRemote-0.01/X11/XRemote

if [ $1 ]; then
    PREFIX=$1
else
    PREFIX=`pwd`/PREFIX
fi

$PERL Makefile.PL PREFIX=$PREFIX \
--with-zmap-inc "-I$ZMAP_INC" \
--with-zmap-libs "-L$ZMAP_LIB" \
--with-symbols 

make
make install

PERL5LIB=$PREFIX:$PERL5LIB
export PERL5LIB
echo "A nice Tk window should appear. If not something failed."
./tk_test.pl
echo "close the Tk window please..."
echo "Make sure $PREFIX is in your PERL5LIB"
