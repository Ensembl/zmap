#!/bin/bash

# ================= README ==================

# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in ONE_OFF.BUILD file/directory.
#
# Main Build Parameters:
#
# Version incremented        yes
#        Docs created        no
#     Docs checked in        no
#  
# Note: Error reporting gets done by the build_run in a unified way.


RC=0


BUILD_PREFIX='DEVELOPMENT_BUILD'
ZMAP_RELEASES_DIR="DEVELOPMENT"
ERROR_ID='edgrif@sanger.ac.uk'


./build_run.sh -a $ERROR_ID -e -m -t -u -z $ZMAP_RELEASES_DIR $BUILD_PREFIX || RC=1


exit $RC
