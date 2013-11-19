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


BUILD_PREFIX='FEATURE'
SEQTOOLS_DIR='FEATURE'
#ERROR_ID='edgrif@sanger.ac.uk'
ERROR_ID=''

# Script takes 1 required arg which is the name of the feature branch,
# and 1 optional arg which is the symlink name of the seqtools build
# (without the "BUILD." prefix)
#
if (( $# < 1 )) ; then
  echo "script needs feature branch name:  $0 <feature_branch_name>"
  exit 1
else
  FEATURE_BRANCH=$1
fi

if (( $# > 1 )) ; then
  SEQTOOLS_DIR=$2
fi

./build_run.sh $ERROR_ID -b $FEATURE_BRANCH -d -g -m -s $SEQTOOLS_DIR $BUILD_PREFIX || RC=1

exit $RC
