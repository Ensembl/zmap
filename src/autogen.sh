#!/usr/bin/env bash
###############################################################################
# Simple script to bootstrap and create the configure script, should be run
# when any control files have been changed (e.g. new source files added which
# led to changes to Makefile.am files) including:
#
#    configure.ac
#    Makefile.am
#    Makefile.in
#
############################################################


SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

RC=0

# Load common functions/settings.
#
. $BASE_DIR/../scripts/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/../scripts/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }


# Cleans the sub-directory out and refetches the empty placeholder
# from the zmap repository.
#
# Args are:    lib_repository_name  lib_local_dir
#
function clean_lib
{
    zmap_message_out "Starting removing $1 from $2"

    rm -rf ./$2/*

    git checkout ./$2

    zmap_message_out "Finished removing $1 from $2"

}


# Sets up a sub-directory to contain the supplied git repository code.
#
# Args are:    full_git_repository_uri lib_local_dir git_branch
#
#
function fetch_lib
{
    tmp_dir='autogen_tmp_git_checkout_dir'

    zmap_message_out "Starting cloning $1 into $2"

    mkdir $tmp_dir

    git clone $3 $1 $tmp_dir || zmap_message_exit "could not clone $1 into $PWD/$tmp_dir"

    cp -rf ./$tmp_dir/* ./$2

    rm -rf ./$tmp_dir

    # SHOULD WE BE DOING THIS ????? NOT TOO SURE....
    # Make sure the placeholder files (.gitignore, README) are their original zmap versions
    #git checkout ./$2/

    zmap_message_out "Finished cloning $1 into $2"

}



force_remake_all=''
gb_tools='maybe'
ensc_core='maybe'
run_autoupdate=''					    # don't run autoupdate by default.
install_missing='-i'
verbose=''
version_arg=''

# Base location of our git repositories
git_host='git.internal.sanger.ac.uk'
git_root='/repos/git/annotools'


# Set up info. about each external library using bash associative arrays.
# To add more libraries you must invent a new key name (e.g. the name of your
# library) and use that name to add your values to each of the below arrays.
# (note order of keys below is of course irrelevant as it's an associative array)
#
aceconn_key='aceconn'
ensc_core_key='ensc_core'
gb_tools_key='gb_tools'
htslib_key='htslib'
zeromq_key='zeromq'

declare -A install=( [$aceconn_key]='maybe' [$ensc_core_key]='maybe' [$gb_tools_key]='maybe'
    [$htslib_key]='maybe' [$zeromq_key]='maybe' )

declare -A repos=( [$aceconn_key]='AceConn' [$ensc_core_key]='ensc-core' [$gb_tools_key]='gbtools'
    [$htslib_key]='htslib' [$zeromq_key]='zeromq' )

declare -A dir=( [$aceconn_key]='AceConn' [$ensc_core_key]='ensc-core' [$gb_tools_key]='gbtools'
    [$htslib_key]='htslib' [$zeromq_key]='zeromq' )

declare -A test_file=( [$aceconn_key]='configure.ac' [$ensc_core_key]='src/Makefile' [$gb_tools_key]='configure.ac'
    [$htslib_key]='Makefile' [$zeromq_key]='configure.ac' )

declare -A branch=( [$aceconn_key]='' [$ensc_core_key]='-b feature/zmap' [$gb_tools_key]=''
    [$htslib_key]='' [$zeromq_key]='' )





# Cmd line options....
#
# Do args.
#
usage="

Usage:
 $SCRIPT_NAME [ -a -d -e -f -g -h -i -n -u -v ]

   -a  Force checkout of aceconn (overwrite existing subdir)
   -d  Disable checkout of ensc-core (use existing subdir or local install)
   -e  Force checkout of ensc-core (overwrite existing subdir)
   -f  Force remake all
   -g  Force checkout of gbtools (overwrite existing subdir)
   -h  Show this usage info
   -i  Install missing
   -n  Disable checkout of gbtools repository (use existing subdir or local install)
   -u  Run autoupdate
   -v  Verbose

   -z  testing, no checkout of zeromq
"

while getopts ":azdefghinuv" opt ; do
    case $opt in
	a  ) install[$aceconn_key]='yes' ;;
	d  ) install[$ensc_core_key]='no' ;;
	e  ) install[$ensc_core_key]='yes' ;;
	f  ) force_remake_all='-f' ;;
	g  ) install[$gb_tools_key]='yes' ;;
	h  ) zmap_message_exit "$usage" ;;
	i  ) install_missing='-i' ;;
	n  ) install[$gb_tools_key]='no' ;;
	u  ) run_autoupdate_key]='yes' ;;
	v  ) verbose='-v' ;;
	z  ) install[$zeromq_key]='no' ;;

	\? ) zmap_message_exit "Bad arg flag: $usage" ;;
    esac
done


# Look for extraneous args....
#
shift $(($OPTIND - 1))

if [ $# != 0 ]; then
  zmap_message_exit "Too many args: $usage"
fi


# Off we go......
#
zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "build bootstrap starting...."



# Loop through external libraries trying to copy them into our src tree.
# Note loop only works because all associative arrays have the same set of
# keys, i.e. one for each library.
#

zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "starting installing external libraries:  ${!dir[*]}"

for i in "${!install[@]}"
  do

  zmap_message_out "starting install of $i....."

  if [[ "${install[$i]}" == "yes" ]] ; then

      clean_lib "${repos[$i]}" "${dir[$i]}"

  fi

  if [[ "${install[$i]}" == "yes" || "${install[$i]}" == "maybe" ]] ; then

      if [[ ! -f "./${dir[$i]}/${test_file[$i]}" ]] ; then

          fetch_lib "$git_host:$git_root/${repos[$i]}" "${dir[$i]}" "${branch[$i]}"

      fi

  fi

  # Special case post-processing for some libraries....
  case $i in

    # If the gbtools autogen.sh script exists then run that. This is necessary
    # for gbtools to create its gbtools_version.m4 file.
    $gb_tools_key )
    if [ -e "./${dir[$i]}/autogen.sh" ] ; then

        cur_dir=`pwd`
	cd $BASE_DIR/gbtools
	./autogen.sh
	cd $cur_dir
	
    fi ;;

    # We must have htslib (currently) or we fail.
    $htslib_key )
    if [[ ! -f "${dir[$i]}/${test_file[$i]}" ]] ; then

        zmap_message_exit "Aborting.....htslib is not available so ZMap cannot be built."

    fi ;;

#    # We must have zeromq (currently) or we fail.
#    $zeromq_key )
#    if [[ ! -f "${dir[$i]}/${test_file[$i]}" ]] ; then
#
#        zmap_message_exit "Aborting.....zeromq is not available so ZMap cannot be built."
#
#    fi ;;

  esac

  zmap_message_out "finished install of $i....."

  done

zmap_message_out "finished installing external libraries:  ${!dir[*]}"
zmap_message_out "-------------------------------------------------------------------"



if [[ ! -d "./$ensc_core/src" ]] ; then

    # The src subdir must exist even if we are not building ensc-core
    # gb10: not sure why but it seems it gets configured even though we
    # don't include it in SUBDIRS
    mkdir ./$ensc_core/src

fi

# Remove any files/dirs from previous builds, necessary because different
# systems have different versions of autoconf causing clashes over macros
# and so on.
#
autoconf_generated='./config ./config.h.in ./Makefile.in'
rm -rf $autoconf_generated


# set up zmap version number, this is arcane and horrible and all
# autoconf's fault. See http://sources.redhat.com/automake/automake.html#Rebuilding
# and the stuff about AC_INIT
# NOTE that zmap_version.m4 is referenced from configure.ac
#
version_macro_file='zmap_version.m4'
rm -f $version_macro_file


# For production branches we use the abbreviated version number (i.e. no git commit info.).
branch=`$BASE_DIR/../scripts/git_branch.sh`
if [ "$branch" = "production" ] ; then
  version_arg='-a'
else
  version_arg=''
fi

ZMAP_VERSION=`../scripts/git_version.sh $version_arg`

echo 'dnl zmap_version.m4 generated by autogen.sh script, do not hand edit.'  > $version_macro_file
echo "m4_define([VERSION_NUMBER], [$ZMAP_VERSION])" >> $version_macro_file
zmap_message_out "ZMap version is: $ZMAP_VERSION"

#
# Load SO files and generate a header file from them.
#
$BASE_DIR/../scripts/zmap_SO_header.pl || zmap_message_exit "zmap failed to generate SO header"



# SURELY THIS ISN'T NEEDED NOW....OR WE SHOULD DELETE THE CVSIGNORE STUFF FROM THE REPOSITORY.
# clean up first
# xargs: I've removed the -r because not all platforms have it.
# rm: I've added -r to deal with directories and taken away the -f so we can see any errors.
# I've changed from just cat'ing the root dir .cvsignore to all of them and then grep'ing them
# replacing .cvsignore: in the output from grep and removing the file! Ha ha.
#cat .cvsignore | xargs -t rm -r
find . -name '.cvsignore' | \
xargs grep '.' | \
sed -e 's/.cvsignore://' | \
while read file; do [ ! -f "$file" ] || rm -r $file; done


# Optionally run autoupdate.....
#
if [ -n "$run_autoupdate" ] ; then
  [ "x$AUTOUPDATE" != "x" ]  || AUTOUPDATE=autoupdate
  zmap_message_out "Running $(which $AUTOUPDATE)"
  $AUTOUPDATE || zmap_message_exit "Failed running $AUTOUPDATE"
  zmap_message_out "Finished $(which $AUTOUPDATE)"
fi



# zeromq should not be built using its own autogen.sh because this does not produce the right
# set up for the build to be run as part of the zmap build. However this means we need
# to create a config sub-directory so that its ./configure will run.
zeromq_dir='zeromq/config'
rm -rf $zeromq_dir  || zmap_message_exit "Could not remove tmp dir $zeromq_dir for zeromq build."
mkdir $zeromq_dir  || zmap_message_exit "Cannot make $zeromq_dir for zeromq build."




# Use autoreconf to run the autotools chain in the right order with
# the right args.
#
zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "About to run autoreconf to bootstrap autotools and our build system"

autoreconf $force_remake_all $verbose $install_missing -I ./ || zmap_message_exit "Failed running autoreconf"

zmap_message_out "Finished running autoreconf for bootstrap"
zmap_message_out "-------------------------------------------------------------------"


# finished !
zmap_message_out "build bootstrap finished...."
zmap_message_out "-------------------------------------------------------------------"


exit $RC
