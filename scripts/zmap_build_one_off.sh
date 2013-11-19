#!/bin/bash
#
# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in ONE_OFF.BUILD file/directory.
#
# Main Build Parameters:
#
# Update from repository before building   yes
#                    Version incremented    no
#                           Docs created    no
#                        Docs checked in    no
#
# Error reporting gets done by the build_run.
#

RC=0

BRANCH='develop'
SEQTOOLS_DIR='DEVELOPMENT'
BUILD_PREFIX='ONE_OFF'
ERROR_RECIPIENT='edgrif@sanger.ac.uk'

# Two optional args: 1st specifies the FULL branch name and the 2nd is
# the symlink id of the seqtools build to use (excluding the "BUILD."
# prefix e.g. "RELEASE" or "FEATURE.sqlite").
#
if (( $# > 0 )) ; then
  BRANCH=$1
fi

if (( $# > 1 )) ; then
  SEQTOOLS_DIR=$2
fi

./build_run.sh -a $ERROR_RECIPIENT -b $BRANCH -d -g $SEQTOOLS_BUILD $BUILD_PREFIX || RC=1

exit $RC
