#!/usr/bin/env bash
#
# Script to bootstrap and create the configure script, should be run when any
# control files have been changed including:
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
. $BASE_DIR/sh_functions.sh || { echo "Failed to load sh_functions.sh"; exit 1; }
set -o history


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

    # Copy the entire contents of the temp directory to the destination directory. Note that 
    # the dot on the end of the source directory is essential for including hidden files. We
    # need to include the hidden .git directory so that we can determine the correct git version
    # number for the library.
    cp -rf ./$tmp_dir/. ./$2

    rm -rf ./$tmp_dir

    zmap_message_out "Finished cloning $1 into $2"

}


# Attempts to check out the relevant commit of a lib for the current zmap commit.
# Takes a single argument which is the lib subdirectory to cd into (should be the root
# of the lib's git repo). This should be called from the zmap src directory which contains
# the lib subdirectory.
# Only does anything if checkout_old_commits is 'yes' (from -o arg).
#
function lib_checkout_old_commit
{
  if [ -e "./$1" ] ; then
    if [[ "$checkout_old_commits" == "yes" ]] ; then
      zmap_commit=`git log -1 --pretty='format:%h'`
      zmap_commit_date=`git show --name-only --raw --pretty='format:%ci' | head -n 1`
      echo "zmap commit '$zmap_commit' has date '$zmap_commit_date'"

      cd ./$1

      lib_commit=`git log -1 --pretty='format:%h' --before="$zmap_commit_date"`
      lib_commit_date=`git show --name-only --raw --pretty='format:%ci' | head -n 1`

      echo "Latest $1 commit before '$zmap_commit_date' is commit '$lib_commit' with date '$lib_commit_date'"

      if [ -n "$lib_commit" ] ; then
        echo "Checking out $lib_commit"
        git checkout $lib_commit
      else
        echo "Error finding lib commit !"
      fi

      cd ..
    fi
  fi
}

# Usage: config_set_ZMAP_ARCH <machine_name>
# Sets ZMAP_ARCH for the zmap naming conventions for architecture for the given machine 
# e.g. Linux_i686 (lucid 32-bit), Linux_x86_64 (lucid 64-bit), precise_x86_64 (precise 64-bit) etc.
function config_set_ZMAP_ARCH
{
    local opsys arch lsb_release separator host

    host=$1
    current_host=`hostname -s`
    separator="_"

    if [ "$host" == "$current_host" ] ; then
        opsys=`uname -s`
        arch=`uname -m`
    else
        opsys=`ssh $host uname -s`
        arch=`ssh $host uname -m`
    fi

    case $opsys in
        "Linux")
            if [ "$host" == "$current_host" ] ; then
                lsb_release=`lsb_release -cs`
            else
                lsb_release=`ssh $host lsb_release -cs`
            fi

            case $lsb_release in
                "lucid")
                    ZMAP_ARCH="Linux"$separator$arch
                    ;;
                *)
                    ZMAP_ARCH=$lsb_release$separator$arch
                    ;;
            esac
            ;;
        *)
            ZMAP_ARCH=$opsys$separator$arch
            ;;
    esac
}


checkout_only='no'
force_remake_all=''
gb_tools='maybe'
ensc_core='maybe'
run_autoupdate=''                                           # don't run autoupdate by default.
install_missing='-i'
verbose=''
version_arg=''
checkout_old_commits='no'

# Base location of our git repositories
git_host='git.internal.sanger.ac.uk:'
git_root='/repos/git/annotools'


# for ensembl development we sometimes need zmap to reference it but we don't want to build
# it locally....this is a slightly more complicated case then for other libs....for which we
# use a 'dummy' file to signal to configure that we want to do this.
ensembl_file='ensembl_ref_but_no_build'



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

# to move to v4 of zeromq simply change the value for zeromq in this array to 'zeromq' and rebuild.
#
declare -A repos=( [$aceconn_key]='AceConn' [$ensc_core_key]='ensc-core' [$gb_tools_key]='gbtools'
    [$htslib_key]='htslib' [$zeromq_key]='zeromq_v3' )

declare -A repos_url=( 
    [$aceconn_key]='https://github.com/Ensembl/AceConn.git'
    [$ensc_core_key]='https://github.com/Ensembl/ensc-core.git'
    [$gb_tools_key]='https://github.com/Ensembl/gbtools.git'
    [$htslib_key]='https://github.com/samtools/htslib.git'
    [$zeromq_key]='https://github.com/Ensembl/zeromq_v3.git' )

declare -A dir=( [$aceconn_key]='AceConn' [$ensc_core_key]='ensc-core' [$gb_tools_key]='gbtools'
    [$htslib_key]='htslib' [$zeromq_key]='zeromq' )

declare -A test_file=( [$aceconn_key]='configure.ac' [$ensc_core_key]='src/Makefile' [$gb_tools_key]='configure.ac'
    [$htslib_key]='Makefile' [$zeromq_key]='configure.in' )


# can specify a specific branch name or tag here by using the -b arg:
declare -A branch=( [$aceconn_key]='-b develop' [$ensc_core_key]='-b 0.1' [$gb_tools_key]='-b develop'
    [$htslib_key]='-b 1.3.2' [$zeromq_key]='-b develop' )

# github: had to add -b develop for gbtools git clone, somehow github doesn't just get develop....




# Cmd line options....
#
# Do args.
#

# NEW COMMAND LINE ARGS:
# NOT SURE ABOUT THIS CHANGE AS IT LOOKS LIKE FOR ENSEMBL THERE ARE COMPLICATIONS
# WITH HANDLING THE USING OF A LOCAL INSTALL.....THERE ARE 3 OPTIONS: install,
# don't install and don't install but reference....I THINK THE BELOW IS CORRECT.
#


usage="

Usage:
 $SCRIPT_NAME [ -n -a -e -g -h -z -d -c -o -r <repository path> -i -u -v ]

   Optional libraries options:

   -n  Disable checkout of any sub libraries

   -a  Force checkout of aceconn (overwrite existing subdir)
   -e  Force checkout of ensc-core
   -g  Force checkout of gbtools
   -h  Force checkout of htslib
   -z  Force checkout of zeromq

   -d  Force a reference to an existing ensembl checkout (mutually exclusive to -e)

   To select a subset of libraries specify -n and then specify the libs you want.

   -c  Checkout sub libraries then exit (no autoreconf step)

   -o  Checkout versions of sub libs that match the current zmap version
       (currently only gbtools and enscore)

   -r  specify a directory to check out the repositories from

   Build options:

   -f  Force remake all
   -i  Install missing
   -u  Run autoupdate
   -v  Verbose

"


if [[ -f $ensembl_file ]] ; then
  rm -f $ensembl_file
fi


while getopts ":naceghzdcor:fiuv" opt ; do
    case $opt in
        n  ) install[$aceconn_key]='no'
             install[$ensc_core_key]='no'
             install[$gb_tools_key]='no'
             install[$htslib_key]='no'
             install[$zeromq_key]='no' ;;

        a  ) install[$aceconn_key]='yes' ;;
        e  ) install[$ensc_core_key]='yes' ;;
        g  ) install[$gb_tools_key]='yes' ;;
        h  ) install[$htslib_key]='yes' ;;

        z  ) install[$zeromq_key]='yes' ;;

        d  ) touch $ensembl_file
             install[$ensc_core_key]='no' ;;

        c  ) checkout_only='yes' ;;
        o  ) checkout_old_commits='yes' ;;

        r  ) git_host=''
             git_root=$OPTARG ;;

        f  ) force_remake_all='-f' ;;
        i  ) install_missing='-i' ;;
        u  ) run_autoupdate_key]='yes' ;;
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


#echo "setting for git: $git_host$git_root"




# Off we go......
#
zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "build bootstrap started...."


# Loop through external libraries trying to copy them into our src tree.
# Note loop only works because all associative arrays have the same set of
# keys, i.e. one for each library.
#

zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "started installing external libraries from ${!dir[*]}"

lib_list=''

for i in "${!install[@]}"
  do

  if [[ "${install[$i]}" == "no" ]] ; then

    zmap_message_out "library $i not requested so will not be installed....."

  else

    zmap_message_out "started install of $i....."

    if [[ "${install[$i]}" == "yes" ]] ; then

        clean_lib "${repos[$i]}" "${dir[$i]}"

    fi

    if [[ "${install[$i]}" == "yes" || "${install[$i]}" == "maybe" ]] ; then

        if [[ ! -f "./${dir[$i]}/${test_file[$i]}" ]] ; then

            fetch_lib "${repos_url[$i]}" "${dir[$i]}" "${branch[$i]}"

        fi

    fi

    zmap_message_out "finished install of $i....."

    libs_list="$libs_list $i"

  fi

  done

zmap_message_out "finished installing external libraries:  $libs_list"
zmap_message_out "-------------------------------------------------------------------"



# Sometimes we want a tree containing any necessary subdirectories (aceconn etc)
# but don't want to run any autoconf stuff.
#
if [ "$checkout_only" = 'yes' ] ; then
  zmap_message_exit "Subdirectories installed, exiting before autoreconf."
fi


# Loop through external libraries trying to copy them into our src tree.
# Note loop only works because all associative arrays have the same set of
# keys, i.e. one for each library.
#

zmap_message_out "-------------------------------------------------------------------"
zmap_message_out "started post-processing of external libraries from  ${!dir[*]}"

libs_list=''
for i in "${!install[@]}"
  do
    # Special case post-processing for some libraries....
    case $i in

      # If the gbtools autogen.sh script exists then run that. This is necessary
      # for gbtools to create its gbtools_version.m4 file.
      $gb_tools_key )
        if [ -e "./${dir[$i]}/autogen.sh" ] ; then

          zmap_message_out "started post-processing of $i....."

          cur_dir=`pwd`

          lib_checkout_old_commit ${dir[$i]}

          cd ./${dir[$i]}
          ./autogen.sh
          cd $cur_dir
        
          zmap_message_out "finished post-processing of $i....."

          lib_list="$libs_list $i"
        fi
        ;;

      # If the ensc-core autogen.sh script exists then run that. This is necessary
      # for ensc-core to create its ensc_version.m4 file.
      $ensc_core_key )
        if [ -e "./${dir[$i]}/src/autogen.sh" ] ; then

          zmap_message_out "started post-processing of $i....."

          cur_dir=`pwd`

          lib_checkout_old_commit ${dir[$i]}

          cd ./${dir[$i]}/src
          ./autogen.sh
          cd $cur_dir
        
        
          zmap_message_out "finished post-processing of $i....."

          lib_list="$libs_list $i"
        fi
        ;;

      # zeromq has the -Werror compiler option on by default which means the build
      # will fail if we turn on all warnings for the compilation so we turn them off.
      $zeromq_key )

        cur_dir=`pwd`
        cd ./${dir[$i]}

        if [ -e "./${dir[$i]}/src/autogen.sh" ] ; then
          zmap_message_out "started post-processing of $i....."

          mv configure.in configure.in.copy || zmap_message_exit "Could not mv configure.in to configure.in.copy"
          cat configure.in.copy | sed 's/libzmq_werror="yes"/libzmq_werror="no"/' > configure.in ||  zmap_message_exit "Could not change libzmq_werror setting in configure.in"

        
          zmap_message_out "finished post-processing of $i....."

          lib_list="$libs_list $i"
        fi

        cd $cur_dir
        ;;

    esac
  done

zmap_message_out "finished post-processing of external libraries:  $libs_list"
zmap_message_out "-------------------------------------------------------------------"



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
branch=`$BASE_DIR/git_branch.sh`
if [ "$branch" = "production" ] ; then
  version_arg='-a'
else
  version_arg=''
fi

ZMAP_VERSION=`$BASE_DIR/git_version.sh $version_arg`

echo 'dnl zmap_version.m4 generated by autogen.sh script, do not hand edit.'  > $version_macro_file
echo "m4_define([VERSION_NUMBER], [$ZMAP_VERSION])" >> $version_macro_file
zmap_message_out "ZMap version is: $ZMAP_VERSION"

#
# Load SO files and generate a header file from them.
#
$BASE_DIR/zmap_SO_header.pl || zmap_message_exit "zmap failed to generate SO header"

#
# This should find the location of the local installation of qmake 
# and save it to the file specified.
#
config_set_ZMAP_ARCH `hostname -s`
zmap_message_out "ZMAP_ARCH is $ZMAP_ARCH"
arch_trusty="trusty_x86_64"
arch_tviewsrv="Linux_i686"
arch_mac="Darwin_x86_64"
function qt_location
{
  local return_value
  if [ "$ZMAP_ARCH" = "$arch_trusty" ] ; then
    return_value=`find /usr/local/[QTqt]*/5.5 -type f -executable -name qmake`
  elif [ "$ZMAP_ARCH" = "$arch_tviewsrv" ] ; then
    return_value=`find /usr/local/[QTqt]*/5.5 -type f -executable -name qmake`
  elif [ "$ZMAP_ARCH" = "$arch_mac" ] ; then
    return_value="qt_not_installed_yet_on_mac"
  fi
  echo "$return_value"
}
function qt_qmake_args
{
  local return_value
  if [ "$ZMAP_ARCH" = "$arch_trusty" ] ; then
    return_value="-r -spec linux-g++"
  elif [ "$ZMAP_ARCH" = "$arch_tviewsrv" ] ; then
    return_value="-r -spec linux-g++"
  elif [ "$ZMAP_ARCH" = "$arch_mac" ] ; then
    return_value="qt_not_installed_yet_on_mac"
  fi
  echo "$return_value"
}
QMAKE_LOCAL=$(qt_location)
QMAKE_ARGS=$(qt_qmake_args)
zmap_message_out "qmake is found in $QMAKE_LOCAL"
zmap_message_out "qmake args are $QMAKE_ARGS"
qmake_location='qmake.location'
rm -f $qmake_location
echo "QMAKE_LOCAL=$QMAKE_LOCAL" > $qmake_location 
echo "QMAKE_ARGS=$QMAKE_ARGS" >> $qmake_location



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

if [[ "${install[$zeromq_key]}" == "yes" ]] ; then

  zeromq_dir='zeromq/config'
  rm -rf $zeromq_dir  || zmap_message_exit "Could not remove tmp dir $zeromq_dir for zeromq build."
  mkdir $zeromq_dir  || zmap_message_exit "Cannot make $zeromq_dir for zeromq build."

fi



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
