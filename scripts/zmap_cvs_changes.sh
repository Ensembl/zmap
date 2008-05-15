#!/bin/bash
#
# Runs the cvs2cl tool (available from http://www.red-bean.com/cvs2cl/) to give
# changes in all the directories we are interested in.
#
# Note that you can get help on cvs2cl using:   "perldoc cvs2cl"
#
# Script should be invoked like this: cvschanges '2004-03-01<2004-04-01' target_dir
#
# Some day I will add user friendly options to do months automatically....note from
# the above that you need to include the first day of the _next_ month to get all of
# the current month...a cvs date range oddity.
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


# set up usage strings.
cmdstring='[-a -o -z  -ddate_range -ttarget_release_notes_directory]'
descstring=`cat <<DESC
   -a   do acedb cvs changes

   -o   output filename (default is XXXX.ChangeLog

   -z   do zmap cvs changes

   -d   date_range
        should be in the format "2004-03-01<2004-04-01", note that you need
        to specify one day _past_ the final day you want.

   -t   target_release_notes_directory
        specifies the directory where the release notes will be copied to.
DESC`


changes_dir=`pwd`
changename='ChangeLog'

zmap_dirs='doc src'
acedb_dirs='w*'


cvs=$ZBReleaseBase/ZMap
dirs=$zmap_dirs
change_suffix=$cvs
changes_filename="$cvs.$changename"


#
# Get the cmd line stuff...
#

while getopts ":ao:zd:t:" opt ; do
  case $opt in
    a  ) cvs="acedb"
         dirs=$acedb_dirs
         change_suffix=$cvs
         changes_filename="$cvs.$changename" ;;
    o  ) filename_set=$OPTARG ;;
    z  ) cvs="zmap"
         dirs=$zmap_dirs
         change_suffix=$cvs
         changes_filename="$cvs.$changename" ;;
    d  ) date=$OPTARG;;
    t  ) cvs_dir=$OPTARG;;
    \? ) 
zmap_message_exit "Bad Command Line flag

$0 $cmdstring

$descstring"

;;
  esac
done


if [ ! -z "$filename_set" ] ; then
  changes_filename=$filename_set
fi


changes_file="$changes_dir/$changes_filename"

# Need to make sure the date stuff is passed correctly to cvs, format must be:
#
# cvs2cl.pl  -l "-d'2004-03-01<2004-04-01'"  [other args]
#

date="-d'"$date"'"

zmap_cd $cvs_dir

zmap_message_out "Issuing: cvs2cl --chrono -f $changes_file -l $date $dirs"

$BASE_DIR/cvs2cl --chrono -f $changes_file  -l $date $dirs


exit 0
