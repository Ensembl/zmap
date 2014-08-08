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

. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }




CMDSTRING='[ -o <directory> ] <commit> <repository> <ZMap directory>'
DESCSTRING=`cat <<DESC

   -o   specify an alternative output directory for the git change notes.


The commit is the short git hash from which you want other commits listed.

The repository is the repository from which to recover the commits.

ZMap directory must be the base ZMap directory of the build directory so that the docs
and the code match.
DESC`

ZMAP_ONLY=no
ZMAP_BASEDIR=''
output_file=''


zmap_message_out "Starting fetching git commits..."

zmap_message_out "Running in $INITIAL_DIR on $(hostname)"

zmap_message_out "Parsing cmd line options: '$*'"


while getopts ":o:" opt ;
  do
  case $opt in
      o  ) output_file=$OPTARG ;;
      \? ) 
zmap_message_exit "Bad command line flag

Usage:

$0 $CMDSTRING

$DESCSTRING"
;;
  esac
done


if [ -n "$output_file" ] ; then
    tmp_file=`readlink -m $output_file`

    output_file=$tmp_file

    echo "output_file = $output_file"
fi


# get the commit, repository and the ZMap dir - mandatory.
#
shift $(($OPTIND - 1))

commit=$1
git_repository=$2
ZMAP_BASEDIR=$3




if [ -z "$commit" ] ; then
    zmap_message_exit 'No commit specified.'
fi

if [ -z "$git_repository" ] ; then
    zmap_message_exit 'No git repository specified.'
fi

if [ -z "$ZMAP_BASEDIR" ] ; then
    zmap_message_exit 'No directory specified.'
fi




#
# ok....off we go....
#

zmap_message_out "Fetching commits for repository $git_repository from commit $commit onwards..."


# Go to respository directory...
zmap_cd $ZMAP_BASEDIR


# sort out the output file.
if  [ -n "$output_file" ] ; then
    GIT_NOTES_OUTPUT="$output_file"
else
    GIT_NOTES_OUTPUT="$ZMAP_BASEDIR/$ZMAP_RELEASE_DOCS_DIR/$ZMAP_GIT_COMMITS_FILE_NAME"
fi

rm -f $GIT_NOTES_OUTPUT || zmap_message_exit "Cannot rm $GIT_NOTES_OUTPUT file."
touch $GIT_NOTES_OUTPUT || zmap_message_exit "Cannot touch $GIT_NOTES_OUTPUT file."
chmod u+rw $GIT_NOTES_OUTPUT || zmap_message_exit "Cannot make $GIT_NOTES_OUTPUT r/w"

zmap_message_out "Using $GIT_NOTES_OUTPUT as git commits output file."





# Write header.
#
cat >> $GIT_NOTES_OUTPUT <<EOF

git commits for $git_repository from commit $commit 

EOF


git log $1 --first-parent --pretty='format:%s' $commit..  \
    >> $GIT_NOTES_OUTPUT  || zmap_message_exit "Failed to retrieve git commits"


# Write footer
#
cat >> $GIT_NOTES_OUTPUT <<EOF


End of git commits

EOF



zmap_message_out "Finished fetching git commits..."



exit 0
