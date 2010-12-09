#!/bin/bash
#
#
# Usage
# zmap_fetch_acedbbinaries.sh <Target Release Dir> [acedb level]
#
# Script to copy acedb source and binaries from an acedb release directory to a
# local directory.
#
# In the build this is the local install prefix.
#
# We only copy one architecture binaries as build_bootstrap.sh runs this on multiple machines.
# We can copy from remote or local machine to remote or local machine.
# Depending on $ZMAP_MASTER_HOST and first arg (remote = hostname:/path/to/target)
#
# The source is $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.<level>/bin.<hosttype>/
# For one off runs/emergency fixing try
# ln -s ~/work/acedb RELEASE.DEVELOPMENT
# zmap_fetch_acedbbinaries.sh /target/dir DEVELOPMENT ZMAP_ACEDB_RELEASE_CONTAINER=~/work/acedb
#
#

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


zmap_message_out "Starting to copy acedb source/binaries and Seqtools dist file."


if [ "x$ACEDB_MACHINE" == "x" ]; then
    zmap_message_err "The ENV variable ACEDB_MACHINE is not set so the acedb binaries cannot be found."
    zmap_message_exit "."
fi


# ================== OPTIONS ======================

# Get first parameter
let shift_count=0
if [ "x$1" == "x" ]; then
    zmap_message_err "Usage: $0 <Target Release Dir> [DEVELOPMENT|SUPPORTED|EXPERIMENTAL]"
    zmap_message_err "     Target Release Dir is the directory holding Linux,Darwin etc.."
    zmap_message_err "     DEVELOPMENT is the default acedb build level"
    zmap_message_err ""
    zmap_message_err "Command line variables:"
    zmap_message_err "  ZMAP_ACEDB_RELEASE_CONTAINER path to find RELEASE.<level> [$ZMAP_ACEDB_RELEASE_CONTAINER]"
    zmap_message_exit "."
else
    let shift_count=$shift_count+1
fi


if [ ! -d $1 ]; then
    if echo $1 | egrep -q ':'; then 
	zmap_message_err "Checking $1 as a remote location..."
	zmap_scp_path_to_host_path $1
	# zmap_scp_path_to_host_path sets TAR_TARGET_HOST & TAR_TARGET_PATH
	ssh $TAR_TARGET_HOST "[ -d $TAR_TARGET_PATH ] || exit 1" || \
	    zmap_message_exit "Target Release dir ($TAR_TARGET_PATH) _must_ exist as a directory on $TAR_TARGET_HOST."
    else
	zmap_message_exit "Target Release dir ($1) _must_ exist as a directory."
    fi
fi


# First parameter is target directory.
#
TARGET_RELEASE_DIR=$1


# Get second parameter, defaulting to DEVELOPMENT
#
ACEDB_BUILD_LEVEL=DEVELOPMENT

if [ "x$2" == "x" ]; then
    zmap_message_err "Defaulting to DEVELOPMENT for acedb build level"
else
#    case $2 in
#	"DEVELOPMENT"  ) ACEDB_BUILD_LEVEL=$2;;
#	"SUPPORTED"    ) ACEDB_BUILD_LEVEL=$2;;
#	"EXPERIMENTAL" ) ACEDB_BUILD_LEVEL=$2;;
#	* ) zmap_message_exit "build level should be one of DEVELOPMENT|SUPPORTED|EXPERIMENTAL" ;;
#    esac

    ACEDB_BUILD_LEVEL=$2

    let shift_count=$shift_count+1
fi

shift $shift_count

# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi


# ===================== MAIN PART ======================


# Get machine architecture in zmap format.
ZMAP_ARCH=$(uname -ms | sed -e 's/ /_/g')


# We get machine architecture for acedb from $ACEDB_MACHINE env. variable.

zmap_message_out "Using '$ZMAP_ARCH' for zmap architecture dir."
zmap_message_out "Using '$ACEDB_MACHINE' for acedb architecture dir."

# make sure the target of the symlinks exists
if [ "x$TAR_TARGET_HOST" != "x" ]; then
    ssh $TAR_TARGET_HOST "readlink $TAR_TARGET_PATH/$ZMAP_ARCH > /dev/null || exit 1" || \
	zmap_message_exit "$TAR_TARGET_PATH/$ZMAP_ARCH is either not a symlink, or it's target does not exist on $TAR_TARGET_HOST."
else
    symlink=$(readlink $TARGET_RELEASE_DIR/$ZMAP_ARCH) || \
	zmap_message_exit "$TARGET_RELEASE_DIR/$ZMAP_ARCH is either not a symlink, or it's target does not exist."

    zmap_message_out "$TARGET_RELEASE_DIR/$ZMAP_ARCH points at $symlink"
fi

#if [ "x$ZMAP_MASTER_HOST" != "x" ]; then
#    ssh $ZMAP_MASTER_HOST "readlink $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL > /dev/null || exit 1" || \
#	zmap_message_exit "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL is either not a symlink, or it's target does not exist."
#else
#    symlink=$(readlink $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL)
#    zmap_message_out "symlink = '$symlink'"
#    symlink=$(readlink $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL) || \
#	zmap_message_exit "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL is either not a symlink, or it's target does not exist."
#
#    zmap_message_out "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL points at $symlink"
#fi

symlink=$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL

zmap_message_out "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL points at $symlink"


# Finalise the source and target directories.

TARGET=${TARGET_RELEASE_DIR}/${ZMAP_ARCH}/bin

RELEASE_SRC=RELEASE.${ACEDB_BUILD_LEVEL}.BUILD
SOURCE=${ZMAP_ACEDB_RELEASE_CONTAINER}/RELEASE.${ACEDB_BUILD_LEVEL}/bin.$ACEDB_MACHINE


# Blixem is now in a new place.....
#
#BLXNEW_SOURCE=${ZMAP_ACEDB_RELEASE_CONTAINER}/RELEASE.${ACEDB_BUILD_LEVEL}/bin.${ACEDB_MACHINE}_BLXNEW

DIST_DIR="$TAR_TARGET_PATH/Dist"

zmap_mkdir $DIST_DIR

if [ "x$ZMAP_MASTER_HOST" != "x" ]; then
    ssh $ZMAP_MASTER_HOST "[ -d $SOURCE ] || exit 1" || \
	zmap_message_exit "$SOURCE _must_ be a directory that exists on $ZMAP_MASTER_HOST!"
else
    [ -d $SOURCE ] || zmap_message_exit "$SOURCE _must_ be a directory that exists!"
fi

if [ "x$TAR_TARGET_HOST" != "x" ]; then
    ssh $TAR_TARGET_HOST "[ -d $TAR_TARGET_PATH/$ZMAP_ARCH/bin ] || exit 1" || \
	zmap_message_exit "$TARGET _must_ be a directory that exists on $TAR_TARGET_HOST!"
else
    [ -d $TARGET ] || zmap_message_exit "$TARGET _must_ be a directory that exists!"
fi


# ================ COPYING! ======================

#
# copy acedb source code, we only do this once from the master host.
#

zmap_message_out "Copying acedb source..."

if [ "x$ZMAP_MASTER_HOST" != "x" ]; then

    zmap_cd $ZMAP_ACEDB_RELEASE_CONTAINER


    release_file=`ls $RELEASE_SRC/ACEDB-*`			    # Should match just one file name 
    release_file=`basename $release_file`

    tar_file="$DIST_DIR/$release_file.src.tar"	# Put tar file in Dist directory.

    zmap_message_out "Running tar -cvf $tar_file $RELEASE_SRC/w*"
    tar -cvf $tar_file $RELEASE_SRC/w* || zmap_message_exit "Failed to make tar file $tar_file of acedb source in $RELEASE_SRC"
    gzip $tar_file || zmap_message_exit "Failed to gzip $tar_file of acedb source in $RELEASE_SRC"

fi


#
# Do the standard acedb binaries.
#

zmap_message_out "Copying acedb binaries: $ZMAP_ACEDB_BINARIES..."
zmap_message_out "Using: Source = $SOURCE, Target = $TARGET"

for binary in $ZMAP_ACEDB_BINARIES;
  do
  # Copy from remote to local.
  if [ "x$ZMAP_MASTER_HOST" != "x" ]; then
      zmap_message_out "Running scp $ZMAP_MASTER_HOST:$SOURCE/$binary $TARGET/$binary"
      scp $ZMAP_MASTER_HOST:$SOURCE/$binary $TARGET/$binary || zmap_message_exit "Failed to copy $binary"
  else
      zmap_message_out "Running cp $SOURCE/$binary $TARGET/$binary"
      cp $SOURCE/$binary $TARGET/$binary || zmap_message_exit "Failed to copy $binary"
  fi

  # check locally written files.
  if [ "x$TAR_TARGET_HOST" == "x" ]; then
      zmap_message_out "Testing $binary was copied..."
      [ -f $TARGET/$binary ] || zmap_message_err "$binary wasn't written to $TARGET/$binary"
      [ -x $TARGET/$binary ] || zmap_message_err "$binary is _not_ executable."
  fi
done


#
# The new blixem has a dist file so just copy this across.
#

zmap_message_out "Copying Seqtools dist file..."

seqtools_dist_dir="$ZMAP_SEQTOOLS_RELEASE_CONTAINER/$ZMAP_SEQTOOLS_RELEASE_DIR/Dist"
seqtools_dist_file=`ls $seqtools_dist_dir/seqtools*.tar.gz` # Should match only one file.

if [ "x$ZMAP_MASTER_HOST" != "x" ]; then

    zmap_message_out "Running cp $seqtools_dist_file $DIST_DIR"
    cp $seqtools_dist_file $DIST_DIR || zmap_message_exit "Failed to copy $seqtools_dist_file"

fi


zmap_message_out "Finished copying acedb source/binaries and Seqtools dist file."


# ============== END ==============


exit 0

