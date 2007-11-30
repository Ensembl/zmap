#!/bin/bash

BASE_DIR=$1
PACKAGE=$2

# load the global config 
. $BASE_DIR/build_config.sh
# and the functions
. $BASE_DIR/build_functions.sh

SILENT_CD="no"
CVS_MODULE=AceConn

build_set_vars_for_prefix

# cd to the libfoocanvas directory
build_cd $BASE_DIR/$BUILD_DIR

rm -f $PACKAGE/configure.in $PACKAGE/configure

if [ ! -f $PACKAGE/configure.in ]; then

    [ "x$CVS_RSH"  != "x" ] || CVS_RSH=ssh

#   [ "x$CVS_ROOT" != "x" ] || CVS_ROOT=:ext:sanger_cvs:/repos/cvs/acedb
    [ "x$CVS_ROOT" != "x" ] || CVS_ROOT=:ext:cvs.internal.sanger.ac.uk:/repos/cvs/acedb
    
    build_message_out "cvs checking out $CVS_MODULE from Repository ($CVS_ROOT)"
    cvs -d$CVS_ROOT co -d$PACKAGE $CVS_MODULE || build_message_exit "Failed to checkout $CVS_MODULE from Repository ($CVS_ROOT)"
fi

build_cd $PACKAGE

if [ -f Makefile ]; then 
    $MAKE clean || build_message_err "$MAKE clean failed"
fi

if [ ! -f configure ]; then
    
    [ "x$LIBTOOLIZE" != "x" ]  || LIBTOOLIZE=libtoolize 
    [ "x$AUTOHEADER" != "x" ]  || AUTOHEADER=autoheader 
    [ "x$ACLOCAL" != "x" ]     || ACLOCAL=aclocal       
    [ "x$AUTOMAKE" != "x" ]    || AUTOMAKE=automake     
    [ "x$AUTOCONF" != "x" ]    || AUTOCONF=autoconf
    [ "x$AUTOUPDATE" != "x" ]  || AUTOUPDATE=autoupdate    
    
    build_message_out "Running $AUTOUPDATE"
    $AUTOUPDATE                || build_message_exit "Failed running $AUTOUPDATE"
    build_message_out "Running $ACLOCAL"
    $ACLOCAL                   || build_message_exit "Failed running $ACLOCAL (first time)"
    build_message_out "Running $AUTOHEADER"
    $AUTOHEADER --warnings=all || build_message_exit "Failed running $AUTOHEADER"
    build_message_out "Running $LIBTOOLIZE"
    $LIBTOOLIZE --force --copy || build_message_exit "Failed running $LIBTOOLIZE"
    build_message_out "Running $ACLOCAL (again)"
    $ACLOCAL                   || build_message_exit "Failed running $ACLOCAL (second time)"
    build_message_out "Running $AUTOMAKE"
    $AUTOMAKE --gnu     \
	--add-missing   \
	--copy                 || build_message_exit "Failed running $AUTOMAKE"
    build_message_out "Running $AUTOCONF"
    $AUTOCONF --warnings=all   || build_message_exit "Failed running $AUTOCONF"

fi

exit 0;
