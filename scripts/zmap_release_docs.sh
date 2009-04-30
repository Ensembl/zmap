#!/bin/bash

trap '' INT
trap '' TERM
trap '' QUIT

# ================= README ==================

# Script to make all the documentation and update the website with it.
# Really just  a convenience  script which runs  the scripts  that the
# build does  in the right order  at the right  time, without emailing
# errors or complaining too much.

# Things to note.

# Any passwords requested will be USER=zmap passwords

# Make sure all the doc you want to release is cvs committed first.

# Doxygen produces a lot of messages while it's working.

# ================== CONFIG ================== 
# Configuration variables

# ENSURE_UP_TO_DATE= cvs update the directory where scripts are [ yes | no ]
ENSURE_UP_TO_DATE=yes
# Or for development, uncomment this
#ENSURE_UP_TO_DATE=no

# ================== MAIN PART ================== 

SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
    BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
    BASE_DIR=$SCRIPT_DIR
fi


if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
    old_dir=$(pwd)
    cd $BASE_DIR
    export CVS_RSH=ssh
    cvs update -C .  || { 
	echo "Failed to cvs update $BASE_DIR"
	exit 1; 
    }
    cd $old_dir
fi


. $BASE_DIR/zmap_functions.sh     || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh       || { echo "Failed to load build_config.sh";   exit 1; }


zmap_cd $BASE_DIR

zmap_cd ../src

# Firstly we need to get a Makefile for the make docs target.

# bootstrap and runconfig are directory sensitive in their effects and
# requirements. So we need to cd to src directory first. 

zmap_message_out "Running bootstrap"

./bootstrap || zmap_message_exit "Failed to bootstrap"

zmap_message_out "Running runconfig"

./runconfig || zmap_message_exit "Failed to runconfig"

# Now do what we came here to do.

zmap_message_out "Running $BASE_DIR/zmap_make_docs ..."

$BASE_DIR/zmap_make_docs.sh  || zmap_message_exit "Failed to successfully run zmap_make_docs.sh"

zmap_message_out "Running BASE_DIR/zmap_update_web.sh ..."

$BASE_DIR/zmap_update_web.sh || zmap_message_exit "Failed to successfully run zmap_update_web.sh"


# ================== END OF SCRIPT ================== 
