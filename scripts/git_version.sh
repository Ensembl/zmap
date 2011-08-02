#!/bin/bash
#
# Get the git branch and tag/change info. for current branch.
#
#
# Note that all error output must be sent to stderr so as not to interfer
# with version information which is returned on stdout.
#

# Usage: message_err Your Message
function message_err
{
    now=$(date +%H:%M:%S)
    echo "[$SCRIPT_NAME ($now)] $*" >&2
}



RC=0

SCRIPT_NAME=$(basename $0)


# Are we in a repository, if not then just return "NULL"
#
git status >/dev/null 2>&1 || { echo "NULL" ; exit 0 ; }


git_version=`git describe` || { message_err 'git describe failed' ; exit 1 ; }


# git branch lists branches like this (* shows current branch):
#
# tviewsrv[edgrif]59: git branch
# * develop
#   master
#
# code finds the asterisk and then outputs the current branch
#
git_branch=`git branch | 
while read first second
  do

  if [ "$first" == '*' ] ; then
      echo $second
      break
  fi

  done` || { message_err 'git branch failed' ; exit 1 ; }


if [ -n $git_version ] && [ -n $git_branch ] ; then
    echo "$git_branch-$git_version"
else
   message_err 'Could not find git version and branch.' ; exit 1 ; 
fi

exit $RC 

