#!/bin/bash

trap '' INT
trap '' TERM
trap '' QUIT

# ================= README ==================

# Make sure all the doc you want to release is cvs committed first.

# ================== CONFIG ================== 
# Configuration variables

# ENSURE_UP_TO_DATE= cvs update the directory where scripts are [ yes | no ]
ENSURE_UP_TO_DATE=yes
# Or for development, uncomment this
ENSURE_UP_TO_DATE=no

# ================== MAIN PART ================== 

# Errors before here only end up in stdout/stderr
# Errors after here should be mailed to $ERROR_RECIPIENT
if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
    old_dir=$(pwd)
    new_dir=$(dirname  $0)
    up2date=$(basename $0)
    cd $new_dir
    export CVS_RSH=ssh
    cvs update -C .  || { 
	echo "Failed to cvs update $0"
	exit 1; 
    }
    cd $old_dir
fi


SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
    BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
    BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh     || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh       || { echo "Failed to load build_config.sh";   exit 1; }


zmap_message_out "Running $SCRIPT_DIR/zmap_make_docs ..."

$SCRIPT_DIR/zmap_make_docs.sh  || zmap_message_exit "Failed to successfully run zmap_make_docs.sh"

zmap_message_out "Running $SCRIPT_DIR/zmap_update_web.sh ..."

$SCRIPT_DIR/zmap_update_web.sh || zmap_message_exit "Failed to successfully run zmap_update_web.sh"


# ================== END OF SCRIPT ================== 
