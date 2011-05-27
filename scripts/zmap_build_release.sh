#!/bin/bash
#
# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in RELEASE/xxxxx  file/directory.
#
# Main Build Parameters:
#
# Version incremented        yes
#        Docs created        yes
#     Docs checked in        yes
#  
# Error reporting gets done by the build_run.
#

RC=0


BUILD_PREFIX='RELEASE_BUILD'
SUB_DIR="RELEASE"
ERROR_ID='edgrif@sanger.ac.uk'


# once I've tested the -m option needs to be removed.....
./build_run.sh -a $ERROR_ID -d -m -n -t -u -z $SUB_DIR $BUILD_PREFIX || RC=1


exit $RC
