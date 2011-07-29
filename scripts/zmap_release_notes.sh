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


RC=0
date=''
output_file=''


CMDSTRING='[ -d <date> -o <file> ] <ZMap directory>'
DESCSTRING=`cat <<DESC
   -d   specify date from which changes/tickets should be extracted,
        date must be in form "dd/mm/yyyy" (defaults to date in $ZMAP_RELEASE_NOTES_TIMESTAMP)

   -o   specify an alternative output file for the release notes.

ZMap directory must be the base ZMap directory of the build directory so that the docs
and the code match.
DESC`

while getopts ":d:o:" opt ;
  do
  case $opt in
      d  ) date="-d $OPTARG" ;;
      o  ) output_file="-o $OPTARG" ;;
      \? ) 
zmap_message_exit "Bad command line flag

Usage:

$0 $CMDSTRING

$DESCSTRING"
;;
  esac
done

shift $(($OPTIND - 1))

# get ZMap dir
#
if [ $# -ne 1 ]; then
  zmap_message_exit  "bad args: $*"
else
  zmap_basedir=$1
fi


# If we aren't running as zmap then we do not have permissions required to
# quiz RT for ticket details.
#
userid=`whoami`
if [ "$userid" != "zmap" ] ; then
    echo "Warning: you are not running as user zmap so you do not have permissions for querying RT tickets."
    echo "Do you wish to continue anyway ? (y/n)"
    read reply
    if [ "$reply" != "y" ] ; then
	echo 'aborted.'
	exit 1
    fi

fi

exit

./zmap_make_rt_release_notes.sh  -n -f -x $date $output_file $zmap_basedir || RC=1


exit $RC
