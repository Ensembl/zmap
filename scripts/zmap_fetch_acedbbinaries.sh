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


# We only copy one architecture binaries as build_bootstrap.sh runs this on multiple machines.
# We can copy from remote or local machine to local machine


# ================== OPTIONS ======================

# Get first parameter

if [ "x$1" == "x" ]; then
    zmap_message_err "Usage: $0 <Target Release Dir> [DEVELOPMENT|SUPPORTED|EXPERIMENTAL]"
    zmap_message_err "     Target Release Dir is the directory holding Linux,Darwin etc.."
    zmap_message_err "     DEVELOPMENT is the default acedb build level"
    zmap_message_exit "."
fi

if [ ! -d $1 ]; then
    zmap_message_exit "Target Release dir ($1) _must_ exist as a directory."
fi

TARGET_RELEASE_DIR=$1

# Get second parameter, defaulting to DEVELOPMENT

ACEDB_BUILD_LEVEL=DEVELOPMENT

if [ "x$2" == "x" ]; then
    zmap_message_err "Defaulting to DEVELOPMENT for acedb build level"
else
    case $2 in
	"DEVELOPMENT"  ) ACEDB_BUILD_LEVEL=$2;;
	"SUPPORTED"    ) ACEDB_BUILD_LEVEL=$2;;
	"EXPERIMENTAL" ) ACEDB_BUILD_LEVEL=$2;;
	* ) zmap_message_exit "build level should be one of DEVELOPMENT|SUPPORTED|EXPERIMENTAL" ;;
    esac
fi

# ===================== MAIN PART ======================

# Get our machine type. Acedb uses upper cased name.
 ZMAP_ARCH=$(uname)
ACEDB_ARCH=$(echo $ZMAP_ARCH | tr [:lower:] [:upper:])
# We could use ACEDB_MACHINE from acedb .cshrc

zmap_message_out "Using '$ZMAP_ARCH' for zmap architecture dir."
zmap_message_out "Using '$ACEDB_ARCH' for acedb architecture dir."


# make sure the target of the symlinks exists
symlink=$(readlink $TARGET_RELEASE_DIR/$ZMAP_ARCH) || \
    zmap_message_exit "$TARGET_RELEASE_DIR/$ZMAP_ARCH is either not a symlink, or it's target does not exist."

zmap_message_out "$TARGET_RELEASE_DIR/$ZMAP_ARCH points at $symlink"

if [ "x$ZMAP_MASTER_HOST" != "x" ]; then
    ssh $ZMAP_MASTER_HOST "readlink $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL > /dev/null || exit 1" || \
	zmap_message_exit "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL is either not a symlink, or it's target does not exist."
else
    symlink=$(readlink $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL) || \
	zmap_message_exit "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL is either not a symlink, or it's target does not exist."

    zmap_message_out "$ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.$ACEDB_BUILD_LEVEL points at $symlink"
fi

# Finalise the source and target directories.

TARGET=${TARGET_RELEASE_DIR}/${ZMAP_ARCH}/bin
SOURCE=${ZMAP_ACEDB_RELEASE_CONTAINER}/RELEASE.${ACEDB_BUILD_LEVEL}/bin.${ACEDB_ARCH}_4


if [ "x$ZMAP_MASTER_HOST" != "x" ]; then
    ssh $ZMAP_MASTER_HOST "[ -d $SOURCE ] || exit 1" || \
	zmap_message_exit "$SOURCE _must_ be a directory that exists on $ZMAP_MASTER_HOST!"
else
    [ -d $SOURCE ] || zmap_message_exit "$SOURCE _must_ be a directory that exists!"
fi

if [ ! -d $TARGET ]; then
    zmap_message_exit "$TARGET _must_ be a directory that exists!"
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
  [ -f $TARGET/$binary ] || zmap_message_err "$binary wasn't written to $TARGET/$binary"
  [ -x $TARGET/$binary ] || zmap_message_err "$binary is _not_ executable."

done

# ============== END ==============
zmap_message_out "Copied $ZMAP_ACEDB_BINARIES !"

exit 0

