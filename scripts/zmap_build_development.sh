#!/bin/bash
#
# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in ONE_OFF.BUILD file/directory.
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


BUILD_PREFIX='DEVELOPMENT'
SEQTOOLS_DIR='DEVELOPMENT'
ERROR_ID=''
GBTOOLS_BRANCH=''



# Script takes 1 optional arg which is the symlink name of the seqtools build
# (without the "BUILD." prefix)
#
if (( $# > 0 )) ; then
  SEQTOOLS_DIR=$1
fi

if (( $# > 1 )) ; then
  GBTOOLS_BRANCH="-t $2"
fi

if (( $# > 2 )) ; then
  ERROR_ID="-a $3"
fi


./build_run.sh -g -m -s $SEQTOOLS_DIR $GBTOOLS_BRANCH $BUILD_PREFIX $ERROR_ID || RC=1

exit $RC
