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

if [ ! -f $PACKAGE/configure.in ]; then

    [ "x$CVS_RSH"  != "x" ] || CVS_RSH=ssh
    [ "x$CVS_ROOT" != "x" ] || CVS_ROOT=:ext:cvs.internal.sanger.ac.uk:/repos/cvs/acedb
    
    build_message_out "cvs checking out $CVS_MODULE from Repository ($CVS_ROOT)"
    cvs -d$CVS_ROOT co -d$PACKAGE $CVS_MODULE || build_message_exit "Failed to checkout $CVS_MODULE from Repository ($CVS_ROOT)"
fi

build_cd $PACKAGE

if [ ! -f configure ]; then
    
    [ "x$LIBTOOLIZE" != "x" ]  || LIBTOOLIZE=libtoolize 
    [ "x$AUTOHEADER" != "x" ]  || AUTOHEADER=autoheader 
    [ "x$ACLOCAL" != "x" ]     || ACLOCAL=aclocal       
    [ "x$AUTOMAKE" != "x" ]    || AUTOMAKE=automake     
    [ "x$AUTOCONF" != "x" ]    || AUTOCONF=autoconf
    [ "x$AUTOUPDATE" != "x" ]  || ACLOCAL=autoupdate    
    
    $AUTOUPDATE                || build_message_exit "Failed running $AUTOUPDATE"
    $ACLOCAL                   || build_message_exit "Failed running $ACLOCAL"
    $AUTOHEADER --warnings=all || build_message_exit "Failed running $AUTOHEADER"
    $LIBTOOLIZE --force        || build_message_exit "Failed running $LIBTOOLIZE"
    $AUTOMAKE --gnu     \
	--add-missing   \
	--copy                 || build_message_exit "Failed running $AUTOMAKE"
    $AUTOCONF --warnings=all   || build_message_exit "Failed running $AUTOCONF"

fi

exit 0;
