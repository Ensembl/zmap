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


BUILD_PREFIX='PRODUCTION'
#ERROR_ID='-a edgrif@sanger.ac.uk'
ERROR_ID=''

./build_run.sh $ERROR_ID -b production -d -m $BUILD_PREFIX || RC=1


exit $RC
