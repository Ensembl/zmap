#!/bin/bash
#
# Get the git branch and tag/change info. for current branch.
#
# example output:
#
#     no flags         "Release_0_4_0-7-g6948"
#           -a         "Release_0_4_0"
#           -b         "develop-Release_0_4_0-7-g6948"
#
# (-a and -b can be combined)
#
# Note that all error output must be sent to stderr so as not to interfer
# with version information which is returned on stdout.
#

# Usage:    message_err "Your Message"
#
function message_err
{
    now=$(date +%H:%M:%S)
    echo "[$script_name ($now)] $*" >&2
}



RC=0

script_name=$(basename $0)

abbrev=''
branch=''

# do the command line thing....
#
usage="Usage:  $script_name -a -b"
while getopts ":ab" opt ; do
    case $opt in
	a  ) abbrev="--abbrev=0" ;;
	b  ) branch="true" ;;
	\? ) message_err "$usage"
             exit 1
    esac
done

shift $(($OPTIND - 1))


# Shouldn't be any other args
#
if [ $# -gt 0 ] ; then
  message_err "$usage"
  exit 1
fi




# Are we in a repository, if not then just return "NULL"
#
git status >/dev/null 2>&1 || { echo "NULL" ; exit 0 ; }


git_version=`git describe $abbrev` || { message_err 'git describe failed' ; exit 1 ; }


# git branch lists branches like this (* shows current branch):
#
# tviewsrv[edgrif]59: git branch
# * develop
#   master
#
# code finds the asterisk and then outputs the current branch
#
if [ -n "$branch" ] ; then

  git_branch=`git branch | 
  while read first second
    do

    if [ "$first" == '*' ] ; then
      echo $second
      break
    fi

    done` || { message_err 'git branch failed' ; exit 1 ; }
fi



if [ -n "$git_version" ] ; then

  if [ -n "$git_branch" ] ; then
    # branches can have a '/' in their names as a qualifier so change to underscore to avoid
    # problems with using the string in filenames etc.
    git_branch=$(echo $git_branch | sed -e 's/\//_/g')

    #echo "found git branch....$git_branch"

    echo -n "$git_branch-"
  fi

  echo "$git_version"
else
   message_err 'Could not find git version.' ; exit 1 ; 
fi

exit $RC 

