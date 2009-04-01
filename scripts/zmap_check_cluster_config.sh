#!/bin/bash

########################
# Find out where BASE is
# _must_ be first part of script
SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }

zmap_message_out "Start of script."

zmap_message_out "Running in $INITIAL_DIR on $(hostname)"

zmap_message_out "build_config.sh defines ZMAP_BUILD_MACHINES=\"$ZMAP_BUILD_MACHINES\""

zmap_remove_cluster_config

zmap_write_cluster_config

zmap_read_cluster_config

zmap_message_out "Cluster nodes that are available makes ZMAP_BUILD_MACHINES=\"$ZMAP_BUILD_MACHINES\""

zmap_message_out "End of script."
