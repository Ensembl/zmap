#!/bin/bash


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

#function zmap_get_release_symlink()
#{
#    eval ZMAP_RELEASE_SYMLINK=$ZMAP_RELEASE_SYMLINK
#}


usage="$0 -l <link_name> -r <release_location>"
while getopts ":l:r:" opt ; do
    case $opt in
	l  ) ZMAP_RELEASE_SYMLINK=$OPTARG  ;;
	r  ) RELEASE_LOCATION=$OPTARG    ;;
	\? ) zmap_message_exit "$usage"
    esac
done


#shift $(($OPTIND - 1))

# including VARIABLE=VALUE settings from command line
#if [ $# -gt 0 ]; then
#    eval "$*"
#fi


[ "x$ZMAP_RELEASE_SYMLINK" != "x" ] || zmap_message_exit "No release symlink specified: Usage = $usage"

[ "x$RELEASE_LOCATION" != "x" ] || zmap_message_exit "No release location specified: Usage = $usage"


zmap_cd $RELEASE_LOCATION


zmap_message_out "Linking to $ZMAP_RELEASE_SYMLINK"

SYMLINK_DIR=$(dirname $ZMAP_RELEASE_SYMLINK)
SYMLINK_NAME=$(basename $ZMAP_RELEASE_SYMLINK)

zmap_cd $SYMLINK_DIR

zmap_message_out "Looking for existing symlink..."
EXISTS=$(find . -maxdepth 1 -name $SYMLINK_NAME)

if [ "x$EXISTS" != "x" ]; then
    zmap_message_out "Symlink $SYMLINK_NAME already exists in $SYMLINK_DIR"
    zmap_message_out "Removing..."
    rm $SYMLINK_NAME || zmap_message_exit "rm $SYMLINK_NAME failed!"
else
    zmap_message_out "Didn't find existing symlink. Continuing..."
fi

zmap_message_out "Creating new symlink..."
zmap_message_out "ln -s $RELEASE_LOCATION $SYMLINK_NAME"

ln -s $RELEASE_LOCATION $SYMLINK_NAME || zmap_message_exit "Failed to Create symlink"

zmap_message_out "Reached end of script"

exit 0
