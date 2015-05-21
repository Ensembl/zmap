#!/bin/bash
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
aceconn='maybe'
ensc_core='maybe'
htslib='maybe'
zeromq='maybe'
run_autoupdate=''					    # don't run autoupdate by default.
install_missing='-i'
verbose=''
version_arg=''

# Base location of our git repositories
git_host='git.internal.sanger.ac.uk'
git_root='/repos/git/annotools'

# gbtools repository info
gb_tools_repos='gbtools'
gb_tools_dir='gbtools'
gb_tools_branch='' # set this to '-b <branch>' to use a branch other than the default (develop)

# aceconn repository info
aceconn_repos='AceConn'
aceconn_dir='AceConn'
aceconn_branch='' # set this to '-b <branch>' to use a branch other than the default (develop)

# ensembl repository info
ensc_core_repos='ensc-core'
ensc_core_dir='ensc-core'
# For now we hard-code ensembl to use the feature/zmap branch because we have
# some customisations (e.g. disabling certain stuff that we don't require)
# which we can't push to the default branch
ensc_core_branch='-b feature/zmap' 

# htslib repository info
htslib_repos='htslib'
htslib_dir='htslib'
htslib_branch='' # set this to '-b <branch>' to use a branch other than the default (develop)

# zeromq repository info
zeromq_repos='zeromq'
zeromq_dir='zeromq'
zeromq_branch='' # set this to '-b <branch>' to use a branch other than the default (develop)


# Cmd line options....
#
# Do args.
#
usage="

Usage:
 $SCRIPT_NAME [ -a -d -e -f -g -h -i -n -u -v ]

   -a  Force checkout of aceconn (overwrite existing subdir)
   -z  testing, no checkout of aceconn

   -d  Disable checkout of ensc-core (use existing subdir or local install)
   -e  Force checkout of ensc-core (overwrite existing subdir)
   -f  Force remake all
   -g  Force checkout of gbtools (overwrite existing subdir)
   -h  Show this usage info
   -i  Install missing
   -n  Disable checkout of gbtools repository (use existing subdir or local install)
   -u  Run autoupdate
   -v  Verbose

"

while getopts ":azdefghinuv" opt ; do
    case $opt in
	a  ) aceconn='yes' ;;
	z  ) aceconn='no' ;;
	d  ) ensc_core='no' ;;
	e  ) ensc_core='yes' ;;
	f  ) force_remake_all='-f' ;;
	g  ) gb_tools='yes' ;;
	h  ) zmap_message_exit "$usage" ;;
	i  ) install_missing='-i' ;;
	n  ) gb_tools='no' ;;
	u  ) run_autoupdate='yes' ;;
	v  ) verbose='-v' ;;
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
zmap_message_out "bootstrap starting...."
zmap_message_out "-------------------------------------------------------------------"



# OK, OK.....THIS ALL NEEDS TO BE IN A LOOP OVER THE EXTERNAL LIBS WE HAVE.....

# set up gbtools, this is our general tools package.
#
if [[ "$gb_tools" == "yes" ]] ; then

    clean_lib $gb_tools_repos $gb_tools_dir

fi

if [[ "$gb_tools" == "yes" || "$gb_tools" == "maybe" ]] ; then

  if [[ ! -f "./$gb_tools_dir/configure.ac" ]] ; then
  
      fetch_lib $git_host:$git_root/$gb_tools_repos $gb_tools_dir "$gb_tools_branch"

  fi

fi


# If the gbtools autogen.sh script exists then run that. This is necessary
# for gbtools to create its gbtools_version.m4 file.
#
if [ -e "$BASE_DIR/gbtools/autogen.sh" ] ; then
  cur_dir=`pwd`
  cd $BASE_DIR/gbtools
  ./autogen.sh
  cd $cur_dir
fi



# set up aceconn package, this is for connecting to the Ace server.
#
if [[ "$aceconn" == "yes" ]] ; then

    clean_lib $aceconn_repos $aceconn_dir

fi

if [[ "$aceconn" == "yes" || "$aceconn" == "maybe" ]] ; then

  if [[ ! -f "./$aceconn_dir/configure.ac" ]] ; then
  
      fetch_lib $git_host:$git_root/$aceconn_repos $aceconn_dir "$aceconn_branch"

  fi

fi



# Set up ensc-core subdirectory. This is the ensembl C API code.
#

if [[ "$ensc_core" == "yes" ]] ; then

    clean_lib $ensc_core_repos $ensc_core_dir

fi

if [[ "$ensc_core" == "yes" || "$ensc_core" == "maybe" ]] ; then
    
    if [[ ! -f "./$ensc_core/src/Makefile" ]] ; then

        fetch_lib $git_host:$git_root/$ensc_core_repos $ensc_core_dir "$ensc_core_branch"

    fi

fi


# Set up htslib subdirectory. This is the BAM/CRAM/VCF reading/writing C code.
#

if [[ "$htslib" == "yes" ]] ; then

    clean_lib $htslib_repos $htslib_dir

fi

if [[ "$htslib" == "yes" || "$htslib" == "maybe" ]] ; then
    
    if [[ ! -f "./$htslib/Makefile" ]] ; then

        fetch_lib $git_host:$git_root/$htslib_repos $htslib_dir "$htslib_branch"

    fi

fi



# Set up zeromq subdirectory. This is the message passing package for RemoteControl.
#

if [[ "$zeromq" == "yes" ]] ; then

    clean_lib $zeromq_repos $zeromq_dir

fi

if [[ "$zeromq" == "yes" || "$zeromq" == "maybe" ]] ; then
    
    if [[ ! -f "./$zeromq/configure.ac" ]] ; then

        fetch_lib $git_host:$git_root/$zeromq_repos $zeromq_dir "$zeromq_branch"

    fi

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
zmap_message_out "-------------------------------------------------------------------"

autoreconf $force_remake_all $verbose $install_missing -I ./ || zmap_message_exit "Failed running autoreconf"
zmap_message_out "Finished running autoreconf for bootstrap"

# finished !
zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "bootstrap finished...."
zmap_message_out "-------------------------------------------------------------------"


exit $RC
