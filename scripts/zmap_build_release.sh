#!/bin/bash
#
# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in RELEASE/xxxxx  file/directory.
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


BUILD_PREFIX='RELEASE'
SEQTOOLS_DIR='RELEASE'
#ERROR_ID='-a edgrif@sanger.ac.uk'
ERROR_ID=''


# Script takes 1 required arg which is the name of the release branch,
# and 1 optional arg which is the symlink name of the seqtools build
# (without the "BUILD." prefix)
#
if (( $# < 1 )) ; then
  echo "script needs release branch name:  $0 <release_branch_name> [<seqtools_dir>]"
  exit 1
else
  RELEASE_BRANCH=$1
fi

if (( $# > 1 )) ; then
  SEQTOOLS_DIR=$2
fi


./build_run.sh $ERROR_ID -b $RELEASE_BRANCH -d -g -m -s $SEQTOOLS_DIR $BUILD_PREFIX || RC=1


exit $RC
