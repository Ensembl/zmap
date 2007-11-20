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

# cd to the g2 directory
build_cd $BASE_DIR/$BUILD_DIR/$PACKAGE

# These should be a patch of some kind, or submitted to the g2 developers...
build_message_out "Copying our autotools files into current directory tree"
cp -f $BASE_DIR/../g2/Makefile.am .                     || build_message_exit "Failed to copy Makefile.am"
cp -f $BASE_DIR/../g2/configure.ac .                    || build_message_exit "Failed to copy configure.ac"
cp -f $BASE_DIR/../g2/src_Makefile.am ./src/Makefile.am || build_message_exit "Failed to copy src/Makefile.am"

build_message_out "Need to create missing files..."
touch ./NEWS ./AUTHORS ./ChangeLog ./doc/Makefile.am ./g2_perl/Makefile.am ./perl/Makefile.am

build_message_out "Need to remove obsolete files..."
rm -f ./configure.in ./Makefile.in ./src/Makefile.in ./configure

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
