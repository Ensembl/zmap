#!/bin/bash

########################
# Find out where BASE is
# _must_ be first part of script
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

zmap_message_out "Start of script."

zmap_message_out "Running in $INITIAL_DIR on $(hostname)"

zmap_message_out "Parsing cmd line options: '$*'"

# Get the options the user may have requested

while getopts ":di" opt ; do
    case $opt in
	i  ) ZMAP_MAKE_INSTALL=$ZMAP_TRUE     ;;
	\? ) zmap_message_exit "usage notes"
    esac
done

shift $(($OPTIND - 1))

# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi

INSTALL_PREFIX=prefix_root

# We know that this is in cvs
zmap_cd $BASE_DIR

# We can then go to the correct place
zmap_goto_cvs_module_root

mkdir $INSTALL_PREFIX

zmap_message_out "CVS_MODULE = '$CVS_MODULE', Locally lives in (CVS_MODULE_LOCAL) '$CVS_MODULE_LOCAL'"

zmap_cd ..

# we want this for later so we can tar up the whole directory and copy
# back to where we were called from.
CVS_CHECKOUT_DIR=$(pwd)

zmap_message_out "Aiming for the src directory to do the build"
zmap_cd $CVS_MODULE_LOCAL/src

zmap_message_out "PATH =" $PATH

zmap_message_out "Running bootstrap"
./bootstrap  || zmap_message_exit $(hostname) "Failed Running bootstrap"

zmap_message_out "Running runconfig"
./runconfig --prefix=$CVS_CHECKOUT_DIR/$CVS_MODULE_LOCAL/$INSTALL_PREFIX || \
    zmap_message_exit $(hostname) "Failed Running runconfig"

if [ "x$ZMAP_MAKE" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running make"
    make         || zmap_message_exit $(hostname) "Failed Running make"
fi

if [ "x$ZMAP_MAKE_INSTALL" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running make install"
    make install || zmap_message_exit $(hostname) "Failed running make install"
fi

zmap_cd $CVS_CHECKOUT_DIR

zmap_cd $CVS_MODULE_LOCAL






if [ "x$TAR_TARGET" != "x" ]; then

    zmap_scp_path_to_host_path $TAR_TARGET

    zmap_message_out $TAR_TARGET_HOST $TAR_TARGET_PATH
    TAR_TARGET_CVS=$CVS_MODULE.$(hostname -s)
    UNAME_DIR=$(uname -ms | sed -e '/ /_/g')

    [ "x$TAR_TARGET_CVS"  != "x" ] || zmap_message_exit "No CVS set."

    tar -zcf - * | ssh $TAR_TARGET_HOST "/bin/bash -c '\
cd $TAR_TARGET_PATH    || exit 1; \
rm -rf $TAR_TARGET_CVS || exit 1; \
mkdir $TAR_TARGET_CVS  || exit 1; \
cd $TAR_TARGET_CVS     || exit 1; \
tar -zxf -             || exit 1; \
cd ..                  || exit 1; \
ln -s $TAR_TARGET_CVS/$INSTALL_PREFIX $UNAME_DIR'"

    if [ $? != 0 ]; then
	zmap_message_exit "Failed to copy!"
    else
	zmap_message_out "Sucessfully copied to $TAR_TARGET_HOST:$TAR_TARGET_PATH/$TAR_TARGET_CVS"
    fi
fi
