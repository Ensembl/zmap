#!/bin/bash
#
# Script to clone our repository and switch to the named branch.
# The named branch must exist in the repository or the script
# will exit with non-zero status and the cloned repository will
# be removed.
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
BRANCH='develop'
REPOS_NAME='ZMap'
ORIG_DIR=`pwd`

# Do args.
#
usage="$SCRIPT_NAME [ -b <git branch>  -n <repository directory name> ]   <where to put repository directory>
Note: default checkout branch is 'develop'"

while getopts ":b:n:" opt ; do
    case $opt in
	b  ) BRANCH=$OPTARG ;;
	n  ) REPOS_NAME=$OPTARG ;;
	\? ) zmap_message_exit "Bad arg flag: $usage" ;;
    esac
done


# Process final arg, should be target directory for clone repository....
#
shift $(($OPTIND - 1))

if [ $# == 1 ]; then
    TARGET_DIR=$1
else
  zmap_message_exit "No target directory specified:
$usage"
fi


if [ ! -d "$TARGET_DIR" ] ; then
  zmap_message_exit "$TARGET_DIR is not a directory."
fi

REPOS_PATH="$TARGET_DIR/$REPOS_NAME"

if [ -d "$REPOS_PATH" ] ; then
  zmap_message_exit "$REPOS_PATH already exists."
fi



# Step 1, clone the repository....
#
git clone git.internal.sanger.ac.uk:/repos/git/annotools/zmap.git $REPOS_PATH

cd $REPOS_PATH || zmap_message_exit "Failed to cd $REPOS_PATH"


# Step 2, is the requested branch in the remote repository ?
#
# git branch lists branches, -r lists remote branches:
#
# tviewsrv[edgrif]59: git branch -r
#  origin/HEAD -> origin/develop
#  origin/develop
#  origin/master
#  origin/production
#
# we look for our branch in this list....
#
found_branch=''

git_branch=`git branch -r | 
while read first second rest
  do

    zmap_message_err "comparing $first and origin/$BRANCH..."

    if [ "$first" == "origin/$BRANCH" ] ; then

        found_branch=$first

        echo $BRANCH

        break

    fi

  done` || { zmap_message_err 'git branch failed' ; exit 1 ; }


if [ -z "$git_branch" ] ; then

    cd $ORIG_DIR || zmap_message_exit "Failed to cd to original directory $ORIG_DIR, clone $REPOS_PATH not removed."

    # Play safe with removal.
    if [ -d "$REPOS_PATH" ] ; then
       rm -rf $REPOS_PATH
    fi

  zmap_message_exit "$BRANCH not in repository, cloned repository removed, no branch switching done."    

fi


# If the branch exists then we check it out to make it the current working branch.
git checkout $BRANCH || zmap_message_exit "git checkout failed for branch $BRANCH"


exit $RC
