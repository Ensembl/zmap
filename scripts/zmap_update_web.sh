#!/bin/bash
#
# Noddy script to run configure as alphas + Roys machine are far from standard.
#

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

zmap_message_out "Running in $INITIAL_DIR on $(hostname) under $(uname)"

zmap_message_out "cmd line options = '$*'"

ZMAP_BUILD_CONTAINER=$1

zmap_check ${ZMAP_BUILD_CONTAINER:=$INITIAL_DIR}

# These defaults should probably _not_ be here!
zmap_check ${ZMAP_WEBSITE_TARGET:=/nfs/WWWdev/SANGER_docs/htdocs/Software/analysis/ZMap}
zmap_check ${WEBUSER:=zmap}
zmap_check ${WEBHOST:=cbi4}

zmap_cd $ZMAP_BUILD_CONTAINER

zmap_message_out "Using $ZMAP_WEBSITE_TARGET as website directory."

if [[ ! -d $ZMAP_WEBSITE_TARGET || ! -r $ZMAP_WEBSITE_TARGET || ! -w $ZMAP_WEBSITE_TARGET || ! -x $ZMAP_WEBSITE_TARGET ]] ; then
    zmap_message_err  "Directory must exist and be readable/writeable/executable!"
    zmap_message_exit "$ZMAP_WEBSITE_TARGET failed on one of these!"
fi

if [ "x$ZMAP_MASTER_DOCS2WEB" == "x$ZMAP_TRUE" ]; then

    LOCAL_WEBSITE_TARGET=webroot

    mkdir -p $LOCAL_WEBSITE_TARGET

    for dir in $ZMAP_WEBSITE_SOURCE_DIRS; 
      do

      if ! echo $dir | egrep "(^)/" ; then
	  this_dir=$(pwd)
	  zmap_cd $dir
	  dir=$(pwd)
	  zmap_cd $this_dir
      fi
      
      zmap_message_out "Copying ($dir/*) to temporary local target ($LOCAL_WEBSITE_TARGET/). "
      zmap_message_out "Files:"
      find $dir/ -mindepth 1
      
      # First we copy to the local one. We can release this later
      cp -r $dir/* $LOCAL_WEBSITE_TARGET
      
    done
    
    zmap_message_out "Now copying the whole local webroot to $ZMAP_WEBSITE_TARGET"
    zmap_message_out "Running scp -r $LOCAL_WEBSITE_TARGET/* $WEBUSER@$WEBHOST:$ZMAP_WEBSITE_TARGET/"

    if [ "x$PRODUCTION" == "xyes" ]; then
	scp -r $LOCAL_WEBSITE_TARGET/* $WEBUSER@$WEBHOST:$ZMAP_WEBSITE_TARGET/ || zmap_message_exit "Secure Copy Failed."
    fi
fi

if [ "x$ZMAP_MASTER_WEBPUBLISH" == "x$ZMAP_TRUE" ]; then
    zmap_check ${WEBPUBLISH:=webpublish}
    # -t is for test apparently....
    $WEBPUBLISH -t $ZMAP_WEBSITE_TARGET || zmap_message_exit "$WEBPUBLISH failed."
fi


exit 0;
