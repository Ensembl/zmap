#!/bin/bash
#
# Script to add, commit and optionally push a file(s) into git.
#
#


# Find the scripts dir and load the common functions.
SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
SCRIPT_DIR=$(dirname $0)

if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }


RC=0
push='no'


usage="$SCRIPT_NAME [-p] \"your commit reason\" file(s)"
while getopts ":p" opt ; do
    case $opt in
	p  ) push='yes' ;;
	\? ) zmap_message_exit "Bad arg: $usage"
    esac
done

shift $(($OPTIND - 1))

# Get rest of args.
reason=$1
if [ -z "$reason" ] ; then
    zmap_message_exit "bad args, no commit reason:  $usage"
fi

shift
files=$*
if [ -z "$files" ] ; then
    zmap_message_exit "bad args, no files:  $usage"
fi


zmap_message_out "add, commit with push=$push for $files"

git add $files || zmap_message_exit "git add failed"

git commit -m "\"$reason\"" $files || zmap_message_exit "git commit failed"

if [ "$push" == 'yes' ] ; then
    git push  || zmap_message_exit "git push failed"
fi

zmap_message_out "add/commit (push=$push) done"

exit $RC
