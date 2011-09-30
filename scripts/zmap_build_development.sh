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
#ERROR_ID='-a edgrif@sanger.ac.uk'
ERROR_ID=''

./build_run.sh $ERROR_ID -d -g -m -n $BUILD_PREFIX || RC=1

#./build_run.sh -a $ERROR_ID -d -g -m -n -t -z $SUB_DIR $BUILD_PREFIX || RC=1



exit $RC
