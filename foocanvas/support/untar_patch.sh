#!/bin/sh
############################################################
# 
# script to untar and patch libfoocanvas for us.
#  Set the following:

FOOCANVAS_VERSION=20050211

#
#
############################################################

PACKAGE=libfoocanvas-${FOOCANVAS_VERSION}

THISSCRIPT=`basename $0`
SUPPORTDIR=`dirname $0` # directory we can cd to
 CALLEDDIR=`pwd`

cd $SUPPORTDIR
SUPPORTDIR=`pwd`        # and find the actual path

if [ ! -z $WORKSPACE ]; then
    CALLEDDIR=$WORKSPACE
fi

cd $CALLEDDIR

tar -zxf $SUPPORTDIR/${PACKAGE}.tar.gz

cd ${PACKAGE}/libfoocanvas

patch -p0 -u < $SUPPORTDIR/${PACKAGE}.patch

echo "run ./configure, make, make install in '$CALLEDDIR/$PACKAGE'"
