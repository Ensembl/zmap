#!/bin/bash
#
# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in FEATURE_XXX.BUILD file/directory.
#
# Main Build Parameters:
#
# Version incremented        no
#        Docs created        no
#     Docs checked in        no
#  
# Error reporting gets done by the build_run.
#

RC=0


# Script takes 2 args, first is the directory to copy the source
# code from, second is name to give directory under ~zmap...
if (( $# != 2 )) ; then
  echo "script needs two args:  $0 <src_dir> <dest_dir>"
  exit 1
else
  SRC_DIR=$1
  DEST_DIR=$2
fi


BUILD_PREFIX='FEATURE'
ERROR_RECIPIENT='zmapdev@sanger.ac.uk'


./build_run.sh -a $ERROR_RECIPIENT -d -i $SRC_DIR -o $DEST_DIR -g $BUILD_PREFIX || RC=1


exit $RC
