#!/bin/bash

# Need to log into each machine architexture for this...

# Configuration

# The current zmap build version X-X-XX
LATEST_BUILD=0-1-50

# Where to do all this
ZMAP_ACEDB_RELEASE_CONTAINER=/var/tmp

set -x

# No edit below here....

CURRENT=$(pwd)

cd $ZMAP_ACEDB_RELEASE_CONTAINER

RELEASE_4_9_40=~acedb/RELEASE.2008_02_21/
ZMAP_TARGET=~zmap/BUILDS/ZMap.$LATEST_BUILD.BUILD

ln -s $RELEASE_4_9_40 RELEASE.DEVELOPMENT

$CURRENT/zmap_fetch_acedbbinaries.sh $ZMAP_TARGET DEVELOPMENT ZMAP_ACEDB_RELEASE_CONTAINER=$ZMAP_ACEDB_RELEASE_CONTAINER || \
    { echo "Failed!" ; exit 1; }

rm -f RELEASE.DEVELOPMENT

cd $CURRENT
