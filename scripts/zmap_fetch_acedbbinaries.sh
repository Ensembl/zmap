#!/bin/bash

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


# Usage
# zmap_fetch_acedbbinaries.sh <Target Release Dir> [acedb level]

# Script to copy  acedb binaries from an acedb  release directory to a
# local directory.

# In the build this is the local install prefix.

# We only copy one architecture binaries as build_bootstrap.sh runs this on multiple machines.
# We can copy from remote or local machine to remote or local machine.
# Depending on $ZMAP_MASTER_HOST and first arg (remote = hostname:/path/to/target)

# The source is $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.<level>/bin.<hosttype>/
# For one off runs/emergency fixing try
# ln -s ~/work/acedb RELEASE.DEVELOPMENT
# zmap_fetch_acedbbinaries.sh /target/dir DEVELOPMENT ZMAP_ACEDB_RELEASE_CONTAINER=~/work/acedb

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
    zmap_message_err "  ZMAP_ACEDB_VERSION_DIR the part of the release dir after the _ e.g. 4 in _4 [$ZMAP_ACEDB_VERSION_DIR]"
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

TARGET_RELEASE_DIR=$1

# Get second parameter, defaulting to DEVELOPMENT

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

# Get our machine type. Acedb uses upper cased name.
 ZMAP_ARCH=$(uname -ms | sed -e 's/ /_/g')
ACEDB_ARCH=$(uname | tr [:lower:] [:upper:])
# We could use ACEDB_MACHINE from acedb .cshrc

# acedb now has UNIVERSAL builds for mac. We need to check out if we're doing that
# The only that changes is the _4, _64, _UNIVERSAL so we set that here.

ACEDB_VERSION=$ZMAP_ACEDB_VERSION_DIR
[ "x$ACEDB_VERSION" != "x" ] || ACEDB_VERSION=4

zmap_message_out "Using '$ZMAP_ARCH' for zmap architecture dir."
zmap_message_out "Using '$ACEDB_ARCH' for acedb architecture dir."
zmap_message_out "Using '$ACEDB_VERSION' for acedb version."

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
SOURCE=${ZMAP_ACEDB_RELEASE_CONTAINER}/RELEASE.${ACEDB_BUILD_LEVEL}/bin.${ACEDB_ARCH}_${ACEDB_VERSION}


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

# copy all the files from acedb release
zmap_message_out "Copying $ZMAP_ACEDB_BINARIES ..."
zmap_message_out "Using: Source = $SOURCE, Target = $TARGET"


# ================ COPYING! ======================

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

# ============== END ==============
zmap_message_out "Copied $ZMAP_ACEDB_BINARIES !"

exit 0

