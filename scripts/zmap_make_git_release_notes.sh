#!/bin/bash
#
# Fetch git commits since the supplied commit until the release head
# for the given repository.
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

. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }




CMDSTRING='<ZMap dir> <since_commit> <until_commit> <output_file>'
DESCSTRING=`cat <<DESC

The ZMap dir gives the git directory containing the zmap code.

since_commit and until_commit give the start/end for listing commits.

The output file is where the RT release notes should be stored.
DESC`

ZMAP_ONLY=no
ZMAP_BASEDIR=''
output_file=''


zmap_message_out "Starting processing git commits..."


#while getopts ":o:" opt ;
#  do
#  case $opt in
#      o  ) output_file=$OPTARG ;;
#      \? ) 
#zmap_message_exit "Bad command line flag
#
#Usage:
#
#$0 $CMDSTRING
#
#$DESCSTRING"
#;;
#  esac
#done


# get the ZMap dir, since/until commits and output file - mandatory.
#
shift $(($OPTIND - 1))

ZMAP_BASEDIR=$1
since_commit=$2
until_commit=$3
output_file=$4



if [ -z "$ZMAP_BASEDIR" ] ; then
    zmap_message_exit 'No directory specified.'
fi

if [ -z "$since_commit" ] ; then
    zmap_message_exit 'No since_commit specified.'
fi

if [ -z "$until_commit" ] ; then
    zmap_message_exit 'No until_commit specified.'
fi

if [ -z "$output_file" ] ; then
    zmap_message_exit 'No git output file specified.'
fi


#
# ok....off we go....
#


# Go to respository directory...
zmap_cd $ZMAP_BASEDIR


rm -f $output_file || zmap_message_exit "Cannot rm $output_file file."
touch $output_file || zmap_message_exit "Cannot touch $output_file file."
chmod u+rw $output_file || zmap_message_exit "Cannot make $output_file r/w"

zmap_message_out "Using $output_file as git commits output file."



# Write header.
#
cat >> $output_file <<EOF

Git commits for $git_repository from commit "$since_commit" until commit "$until_commit"

EOF

zmap_message_out "Fetching commits for repository $git_repository from commit $commit onwards..."

git log --first-parent --date=short --pretty='format:%h %ad %s' $since_commit..$until_commit  \
    >> $output_file  || zmap_message_exit "Failed to retrieve git commits"

zmap_message_out "Finished fetching git commits..."


# Write footer
#
cat >> $output_file <<EOF


End of git commits

EOF


zmap_message_out "Finished processing git commits..."


exit 0
