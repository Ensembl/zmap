#!/bin/bash

# This file get run on the $SRC_MACHINE as checkout.sh from cron.sh

# It may also be run as ./build_bootstrap.sh by a user possibly with arguments

# The logic is as follows. 
# 1. Checkout a full copy of ZMap module from cvs
# 2. possibly tag/version increment zmap version in cvs
# 3. 

# For use when developing so not every edit in the scripts dir needs a cvs commit...
# It should be a scp location e.g. 'hostname:/path/to/scripts' 

# By setting  it all  .sh files will  be copied into  the ZMap/scripts
# directory and overwrite any that might be there.
ZMAP_MASTER_BUILD_DEVELOPMENT_DIR=deskpro110:/var/tmp/altering_build/scripts
# For Production it should be unset
ZMAP_MASTER_BUILD_DEVELOPMENT_DIR=""

# ================== SETUP ================== 

# First we need to make a file to do the checking out
# This will then get copied to the various hosts to do the checking out on those hosts

gen_checkout_script=zmap_checkout_$$.sh
zmap_tmp_dir=""

# The 'EOF' means _no_ substitution takes place.
(
cat <<'EOF'
#!/bin/bash

# Generated! Can be found in build_bootstrap.sh

ECHO=echo

CVS_MODULE=ZMap
CVS_ROOT=":ext:cvs.internal.sanger.ac.uk:/repos/cvs/zmap"
CVS_RSH="ssh"

export CVS_ROOT CVS_RSH

if [ "x$SCRIPT_NAME" == "x" ]; then
    SCRIPT_NAME=$(basename $0)
fi

# Usage: _checkout_message_out Your Message
function _checkout_message_out
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*"
}

# Usage: _checkout_message_err Your Message
function _checkout_message_err
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*" >&2
}

# Usage: _checkout_message_exit Your Message
function _checkout_message_exit
{
    _checkout_message_err $*
    exit 1;
}

# Usage: _checkout_mk_cd_dir <directory>
function _checkout_mk_cd_dir
{
    if [ "x$1" != "x" ]; then
      mkdir $1 || _checkout_message_exit "Failed to mkdir $1"
      cd $1    || _checkout_message_exit "Failed to cd to $1"
      _checkout_message_out "Now in $(pwd)"
    else
      _checkout_message_exit "Usage: _checkout_mk_cd_dir <directory>"
    fi
}

_checkout_message_out "Start of script."

save_root=$(pwd)

_checkout_message_out "Running in $save_root on $(hostname)"

zmap_tmp_dir=zmap.$$

_checkout_message_out "zmap_tmp_dir set to $zmap_tmp_dir"

_checkout_mk_cd_dir $zmap_tmp_dir

if [ "x$gen_checkout_script" != "x" ]; then
  # This is being run from the master (this code was sourced from build_bootstrap.sh). 
  # The .master directory should _not_ have any runconfig etc run in it!
  _checkout_message_out "Running cvs checkout $CVS_MODULE"
  cvs -d$CVS_ROOT checkout -d $CVS_MODULE.master $CVS_MODULE || _checkout_message_exit "Failed to checkout $CVS_MODULE"
  _checkout_message_out "cp -r $CVS_MODULE.master $CVS_MODULE"
  cp -r $CVS_MODULE.master $CVS_MODULE
fi

# update this to be absolute
gen_checkout_script=$save_root/$gen_checkout_script

# update/set the following variables for use later.
ZMAP_BUILD_CONTAINER=$save_root/$zmap_tmp_dir
CHECKOUT_BASE=$ZMAP_BUILD_CONTAINER/$CVS_MODULE
SCRIPTS_DIR=$CHECKOUT_BASE/scripts
BASE_DIR=$SCRIPTS_DIR
SRC_DIR=$CHECKOUT_BASE/src
DOC_DIR=$CHECKOUT_BASE/doc

# This shouldn't fail and makes sure source can be tested.
echo >/dev/null

EOF
) > $gen_checkout_script


# source the generated checkout script.
# This both checks out ZMap module and sets variables for use later by this script!
. ./$gen_checkout_script ||  { echo "Failed to load ./$gen_checkout_script"; exit 1; }

if [ "x$ZMAP_MASTER_BUILD_DEVELOPMENT_DIR" != "x" ]; then
    # Here we copy from the development dir to the checked out one.  
    _checkout_message_out "*** WARNING: Developing! Using $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR ***"
    chmod 755 $BASE_DIR/*.sh
    scp -r $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR/*.sh $BASE_DIR/
fi

# We're not going to actually do  a build here, but we will build docs
# if requested, and/or anything else that isn't architecture dependent

# Now we have a checkout we can source these
. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }

zmap_message_out "About to parse options: $*"

# Get the options the user may have requested
usage="$0 -d -t -r -u VARIABLE=VALUE"
while getopts ":dtru" opt ; do
    case $opt in
	d  ) ZMAP_MASTER_RELEASE_NOTES=$ZMAP_TRUE      ;;
	t  ) ZMAP_MASTER_TAG_CVS=$ZMAP_TRUE            ;;
	r  ) ZMAP_MASTER_INC_REL_VERSION=$ZMAP_TRUE    ;;
	u  ) ZMAP_MASTER_INC_UPDATE_VERSION=$ZMAP_TRUE ;;
	\? ) zmap_message_exit "$usage"
    esac
done

shift $(($OPTIND - 1))

# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi


# We need to do version stuff... Do this before building...
zmap_message_out "Fetching version using versioner script"
ZMAP_RELEASE_VERSION=$($SCRIPTS_DIR/versioner \
    -path $CHECKOUT_BASE/ \
    -show -V -quiet) || zmap_message_exit "Failed to get zmap version"

zmap_message_out "*** INFORMATION: Version of zmap being built is $ZMAP_RELEASE_VERSION ***"

# If requested, tag cvs to 'freeze' the code.
# Logic is: To tag using  the version _in_  cvs. This would  have been
# incremented the last time this script was run!
if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Fetching cvs tag using versioner script"
    cvs_tag=$($SCRIPTS_DIR/versioner \
	-path $CHECKOUT_BASE/ \
	-show -quiet) || zmap_message_exit "Failed to get zmap version"

    zmap_message_out "Tagging cvs using versioner script"

    zmap_message_out "Will use cvs tag $cvs_tag"

    $SCRIPTS_DIR/versioner -path $CHECKOUT_BASE || zmap_message_exit "Failed to cvs tag zmap"
fi


# ================== FARM OFF BUILDS ================== 

tar_target=$(hostname):$ZMAP_BUILD_CONTAINER

HOSTS_OK=0
HOSTS_FAILED=0
HOSTS_RUN=0

for host in $ZMAP_BUILD_MACHINES
  do
  zmap_message_out "Logging into $host to run build there."

  # N.B. Substitution _will_ occur in this HERE doc.
  (cat $gen_checkout_script - <<EOF
# from 'for host in \$ZMAP_BUILD_MACHINES' loop

# Because we can't cvs checkout reliably on some machines!
# See RT ticket #58607
_checkout_message_out "scp -r $tar_target/$CVS_MODULE.master ./$CVS_MOUDLE"

scp -r $tar_target/$CVS_MODULE.master ./$CVS_MODULE

[ ! -f \$SRC_DIR/configure ] || _checkout_message_exit "\$SRC_DIR should not contain configure yet!"

# We copy these again because they didn't make it into the source directory...
if [ "x$ZMAP_MASTER_BUILD_DEVELOPMENT_DIR" != "x" ]; then
  _checkout_message_out "*** WARNING : If this is in production! Edit ZMAP_MASTER_BUILD_DEVELOPMENT_DIR in build_bootstrap.sh ***"
  chmod 755 \$BASE_DIR/*.sh
  scp -r $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR/*.sh \$SCRIPTS_DIR/
  # Copying runconfig might be a little unexpected here... Possibly remove after first commit... 
  # chmod 755 \$SRC_DIR/runconfig
  # scp -r $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR/../src/runconfig \$SRC_DIR/
  _checkout_message_out "*** WARNING : If this is in production! Edit ZMAP_MASTER_BUILD_DEVELOPMENT_DIR in build_bootstrap.sh ***"
fi

_checkout_message_out "Running ./zmapbuild_and_tar.sh $options"

\$SCRIPTS_DIR/zmapbuild_and_tar.sh $options TAR_TARGET=$tar_target || _checkout_message_exit "Failed to build"

# Now we can clean up.
cd \$ZMAP_BUILD_CONTAINER

check_dir=\$(pwd)
check_base=\$(basename \$check_dir)

# Make sure we're in the right place!
if [ \$check_base == \$zmap_tmp_dir ]; then
  _checkout_message_out "Cleaning up $check_dir"
  rm -rf ./* || _checkout_message_exit "Failed removing files."
  cd ..      || _checkout_message_exit "Failed cd .."
  _checkout_message_out "Removing Directory \$zmap_tmp_dir"
  rmdir \$zmap_tmp_dir
fi

EOF
) | ssh $host '/bin/bash -c "\
function _rm_exit                       \
{                                       \
   echo Checkout/Build Step Failed...;  \
   rm -f host_checkout.sh;              \
   exit 1;                              \
};                                      \
cd /var/tmp                || exit 1;   \
rm -f host_checkout.sh     || exit 1;   \
cat - > host_checkout.sh   || exit 1;   \
chmod 755 host_checkout.sh || _rm_exit; \
./host_checkout.sh         || _rm_exit; \
rm -f host_checkout.sh     || exit 1;   \
"' > $host.log 2>&1

  if [ $? != 0 ]; then
      zmap_message_err "Build on $host failed!"
      let HOSTS_FAILED=HOSTS_FAILED+1
      if [ "x$ZMAP_MASTER_NOTIFY_MAIL" != "x" ]; then
	  # mail tail $host.log to zmapdev@sanger.ac.uk
	  echo "ZMap Build Failed"                                   > fail.log
	  echo ""                                                   >> fail.log
 	  echo "Tail of log:"                                       >> fail.log
	  echo ""                                                   >> fail.log
	  tail $host.log                                            >> fail.log
	  echo ""                                                   >> fail.log
	  echo "Full log can be found $(hostname):$(pwd)/$host.log" >> fail.log
	  cat fail.log | mailx -s "ZMap Build Failed on $host" $ZMAP_MASTER_NOTIFY_MAIL
	  rm -f fail.log
      fi
      # Is it worth continuing?
      # We need to do some cleanup?
  else
      zmap_message_out "Build completed on $host."
      let HOSTS_OK=HOSTS_OK+1
  fi

  let HOSTS_RUN=HOSTS_RUN+1
done

zmap_message_out "Removing $gen_checkout_script. No longer needed."

rm -f $gen_checkout_script

if [ $HOSTS_FAILED == $HOSTS_RUN ]; then
    zmap_message_exit "Build failed on _all_ hosts!"
fi

# ================== MASTER BUILD TASKS ================== 

# We need to run bootstrap and  configure to get the make file targets
# docs and dist.

zmap_message_out "Starting Master build tasks. make docs, dist etc..."

zmap_message_out "Path is" $PATH

zmap_cd $SRC_DIR

zmap_message_out "Running bootstrap"

./bootstrap || zmap_message_exit "Failed to bootstrap"

zmap_message_out "Running runconfig"

./runconfig || zmap_message_exit "Failed to runconfig"


if [ "x$ZMAP_MASTER_CVS_RELEASE_NOTES" == "x$ZMAP_TRUE" ]; then
    zmap_message_err "Need to code release notes bit..."

    $SCRIPTS_DIR/zmap_build_cvs_release_notes.sh
fi

if [ "x$ZMAP_MASTER_RT_RELEASE_NOTES" == "x$ZMAP_TRUE" ]; then
    zmap_message_err "Need to code release notes bit..."

    $SCRIPTS_DIR/zmap_build_rt_release_notes.sh
fi

if [ "x$ZMAP_MASTER_BUILD_DOCS" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running $SCRIPTS_DIR/zmap_make_docs $ZMAP_BUILD_CONTAINER..."

    $SCRIPTS_DIR/zmap_make_docs.sh $ZMAP_BUILD_CONTAINER  || zmap_message_exit "Failed to successfully run zmap_make_docs.sh"

    zmap_message_out "Running $SCRIPTS_DIR/zmap_update_web.sh $ZMAP_BUILD_CONTAINER..."

    $SCRIPTS_DIR/zmap_update_web.sh $ZMAP_BUILD_CONTAINER || zmap_message_exit "Failed to successfully run zmap_update_web.sh"
fi


if [ "x$ZMAP_MASTER_BUILD_DIST" == "x$ZMAP_TRUE" ]; then
    # make dist
    # put somewhere useful!
    zmap_message_out "Running make dist ..."

    make dist || zmap_message_exit "Failed to make distribution file"

    zmap_message_out "mkdir $ZMAP_BUILD_CONTAINER/Dist"

    mkdir -p $ZMAP_BUILD_CONTAINER/Dist

    cp zmap*.tar.gz $ZMAP_BUILD_CONTAINER/Dist/ || zmap_message_exit "Failed to copy distribution to Dist folder."
fi

if [ "x$ZMAP_MASTER_BUILD_CANVAS_DIST" == "x$ZMAP_TRUE" ]; then
    # Need to write this !

    zmap_message_out "Need to make dist in foocanvas..."

    zmap_cd $CHECKOUT_BASE

fi


# version inc...
if [ "x$ZMAP_MASTER_INC_REL_VERSION" == "x$ZMAP_TRUE" ]; then
    $SCRIPTS_DIR/versioner -path $CHECKOUT_BASE -increment -release -cvs || \
	zmap_message_exit "Failed to update release version in cvs"
elif [ "x$ZMAP_MASTER_INC_UPDATE_VERSION" == "x$ZMAP_TRUE" ]; then
    $SCRIPTS_DIR/versioner -path $CHECKOUT_BASE -increment -update -cvs || \
	zmap_message_exit "Failed to update update version in cvs"
fi

zmap_cd $save_root

TAR_FILE=$(pwd)/Complete_build.tar.gz

tar -zcf$TAR_FILE $zmap_tmp_dir || zmap_message_exit "Failed to create tar file of $zmap_tmp_dir"

if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
    RELEASE_LOCATION=$ZMAP_RELEASES_DIR/ZMap.$ZMAP_RELEASE_VERSION.BUILD
fi

if [ "x$RELEASE_LOCATION" == "x" ]; then
    # This should really be a fatal error
    RELEASE_LOCATION=$ZMAP_RELEASES_DIR/LATEST_BUILD
    zmap_message_err "*** WARNING: Defaulting release dir to $RELEASE_LOCATION ***"
fi

$SCRIPTS_DIR/zmap_handle_release_tar.sh $TAR_FILE $RELEASE_LOCATION || \
    zmap_message_exit "Failed to release what we've built here today."

if [ "x$ZMAP_MASTER_REMOVE_FOLDER" == "x$ZMAP_TRUE" ]; then
    rm -rf $zmap_tmp_dir || zmap_message_exit "Failed to remove $zmap_tmp_dir"
fi

rm -f $TAR_FILE

# Looks like success...

zmap_message_out ""
zmap_message_out "Results:"
zmap_message_out "--------"
zmap_message_out "Build run on $ZMAP_BUILD_MACHINES"
zmap_message_out "Version according to binaries should be $ZMAP_RELEASE_VERSION"
zmap_message_out "Binaries, code, doc etc can be found in $RELEASE_LOCATION"
zmap_message_out "For more information see $RELEASE_LOCATION/README"
zmap_message_out "--------"
zmap_message_out "Successfully reached last line of script!"
# ================== END OF SCRIPT ================== 
