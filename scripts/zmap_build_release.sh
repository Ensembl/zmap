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


BUILD_PREFIX='RELEASE'
#SUB_DIR="$BUILD_PREFIX"
ERROR_ID='edgrif@sanger.ac.uk'


# For release builds increment the 'release' number.
#./build_run.sh -a $ERROR_ID -d -n -t -r -z $SUB_DIR $BUILD_PREFIX || RC=1
./build_run.sh -a $ERROR_ID -d -n -t -r $BUILD_PREFIX || RC=1


exit $RC
