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
BUILD_PREFIX=""

while getopts ":dip:" opt ; do
    case $opt in
	i  ) ZMAP_MAKE_INSTALL=$ZMAP_TRUE     ;;
	p  ) BUILD_PREFIX=$OPTARG             ;;
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

# Hack this for now...we know we are just above ZMap anyway....
# We can then go to the correct place
#zmap_goto_cvs_module_root
#cd ..
#CVS_MODULE=ZMap
#CVS_MODULE_LOCAL=ZMap
zmap_goto_git_root


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


if [ "x$ZMAP_MAKE_CHECK" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running make check"

    CHECK_LOG_FILE=$(pwd)/$SCRIPT_NAME.check.log

    MAKE_CHECK_FAILED=no

    make check check_zmap_LOG_FILE=$CHECK_LOG_FILE || MAKE_CHECK_FAILED=$ZMAP_TRUE

    if [ "x$MAKE_CHECK_FAILED" == "x$ZMAP_TRUE" ]; then

	TMP_LOG=/var/tmp/zmap_fail.$$.log

	echo "ZMap Tests Failure"                            > $TMP_LOG
	echo ""                                             >> $TMP_LOG
	echo "Tail of make check log:"                      >> $TMP_LOG
	echo ""                                             >> $TMP_LOG
	tail $CHECK_LOG_FILE                                >> $TMP_LOG
	echo ""                                             >> $TMP_LOG
	echo "Full log is here $(hostname):$CHECK_LOG_FILE" >> $TMP_LOG

	ERROR_RECIPIENT=$ZMAP_MASTER_NOTIFY_MAIL

	if [ "x$ERROR_RECIPIENT" != "x" ]; then
	    zmap_message_out "Mailing log to $ERROR_RECIPIENT"

	    cat $TMP_LOG | mailx -s "ZMap Tests Failure on $(hostname)" $ERROR_RECIPIENT
	fi
	rm -f $TMP_LOG

	zmap_message_err $(hostname) "Failed Running make check"
    fi
fi


if [ "x$ZMAP_MAKE_INSTALL" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running make install"
    make install || zmap_message_exit $(hostname) "Failed running make install"
fi


# Copy the builds to /software for certain types of build:
#   production builds get installed in /software/annotools/
#   release builds get installed in /software/annotools/test
#   develop builds get installed in /software/annotools/dev
#
zmap_message_out "Checking whether to install on /software for $BUILD_PREFIX build"

if [[ $BUILD_PREFIX == "PRODUCTION" || $BUILD_PREFIX == "RELEASE" || $BUILD_PREFIX == "DEVELOPMENT" ]]
then
  zmap_message_out "Installing on /software"
  source_dir=$CVS_CHECKOUT_DIR/$CVS_MODULE_LOCAL/$INSTALL_PREFIX # where to copy the installed files from

  dev_machine=lucid-dev32               # machine with write access to the project software area
  software_root_dir="/software/noarch"  # root directory for project software
  arch_subdir=""                        # the subdirectory for the current machine architecture
  annotools_subdir="annotools"          # the subdirectory for annotools

  opsys=`uname -s`
  
  case $opsys in
    "Linux")
      case `uname -m` in
        "ia64")
          arch_subdir="linux-ia64"
          ;;
  
        "x86_64")
          arch_subdir="linux-x86_64"
          ;;
  
        "i686")
          arch_subdir="linux-i386"
          ;;
  
        *)
          ;;
      esac
      ;;
  
    "Darwin")
      arch_subdir="macosx-10-i386"
      ;;
  
    *)
      ;;
  esac
  
  if [[ $arch_subdir != "" ]]
  then
    # production builds are installed in the project area directly
    # release and develop builds are stored in 'test' and 'dev' subdirectories
    build_subdir=""
    if [[ $BUILD_PREFIX == "DEVELOPMENT" ]]
    then
        build_subdir="/dev"
    elif [[ $BUILD_PREFIX == "RELEASE" ]]
    then
      build_subdir="/test"
    fi

    # Set the destination for the current host machine type
    project_area=$software_root_dir/$arch_subdir/$annotools_subdir$build_subdir

    # Set the destination for precise builds
    precise_subdir="precise-x86_64"
    precise_project_area=$software_root_dir/$precise_subdir/$annotools_subdir$build_subdir

    # Do the copy
    zmap_message_out "Running: ssh $dev_machine '/bin/bash -c $cmd'"
    cmd="cp -r $source_dir/* $project_area"
    ssh $dev_machine '/bin/bash -c $cmd'
    wait
  
    # We don't currently build on ubuntu precise because the ubuntu lucid build works there.
    # This means we need to copy the linux build to precise (note that we only have 64-bit 
    # precise machines).
    if [ $arch_subdir == "linux-x86_64" ]
    then
      zmap_message_out "Running: ssh $dev_machine '/bin/bash -c $cmd'"
      cmd="cp -r $source_dir/* $precise_project_area"
      ssh $dev_machine '/bin/bash -c $cmd'
      wait  
    fi    
  fi
else
  zmap_message_out "NOT installing on /software"
fi



zmap_cd $CVS_CHECKOUT_DIR

zmap_cd $CVS_MODULE_LOCAL



if [ "x$TAR_TARGET" != "x" ]; then

    zmap_scp_path_to_host_path $TAR_TARGET

    zmap_message_out $TAR_TARGET_HOST $TAR_TARGET_PATH
    TAR_TARGET_CVS=$CVS_MODULE.$(hostname -s)
    UNAME_DIR=$(uname -ms | sed -e 's/ /_/g')

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
