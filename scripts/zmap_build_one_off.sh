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


BUILD_PREFIX='ONE_OFF_BUILD'
OUTPUT=$BUILD_PREFIX
ERROR_RECIPIENT='edgrif@sanger.ac.uk'


./build_run.sh -a $ERROR_RECIPIENT -e -l $OUTPUT $BUILD_PREFIX || RC=1


exit $RC
