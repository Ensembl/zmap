#!/bin/bash
#
# This file get run on the $SRC_MACHINE as checkout.sh from cron.sh
#
# It may also be run as ./build_bootstrap.sh by a user possibly with arguments
#
# The logic is as follows. 
# 1. Checkout a full copy of ZMap module from cvs
# 2. possibly tag/version increment zmap version in cvs
# 3. Do all the builds requested.
#    This involves ssh to each host
#    building docs on this machine?
#
#
#


RC=0

# Provide a cleanup function to remove the gen_checkout_script and other tmp files.
#
function zmap_message_rm_exit
{
    zmap_message_err "Removing $FILES_TO_REMOVE"
    rm -f $FILES_TO_REMOVE
    zmap_message_exit "$@"
}



# default branch
BRANCH='develop'

GIT_VERSION_INFO=''

FILES_TO_REMOVE=



# I don't know why history recording is turned on... ???
. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }


zmap_message_out "Start of build bootstrap, running in $PWD"


# Get the options the user may have requested
#
zmap_message_out "About to parse options: $*"

usage="$0 -b <branch> -d -f <zmap feature dir> -g -r -t -u VARIABLE=VALUE"
while getopts ":b:df:grtu" opt ; do
    case $opt in
	b  ) BRANCH=$OPTARG ;;
	d  ) ZMAP_MASTER_RT_RELEASE_NOTES="yes"   ;;
	f  ) ZMAP_MASTER_BUILD_COPY_DIR=$OPTARG ;;
	g  ) GIT_VERSION_INFO="yes" ;;
	r  ) ZMAP_MASTER_INC_REL_VERSION="yes"    ;;
	t  ) ZMAP_MASTER_TAG_CVS="yes"            ;;
	u  ) ZMAP_MASTER_INC_UPDATE_VERSION="yes" ;;
	\? ) zmap_message_rm_exit "$usage"
    esac
done

shift $(($OPTIND - 1))


# try to get rid of this, replace with definitive command line args instead...
#
# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi



# For use when developing so not every edit in the scripts dir needs a cvs commit...
# It should be a scp location e.g. 'hostname:/path/to/scripts' 

# By setting  it all  .sh files will  be copied into  the ZMap/scripts
# directory and overwrite any that might be there.

# For Production it should be unset and root_develop.sh should not exist!
ZMAP_MASTER_BUILD_DEVELOPMENT_DIR=""



if [ -f root_develop.sh ]; then
    zmap_message_out "Development preamble..."
    . ./root_develop.sh
    zmap_message_out ZMAP_MASTER_BUILD_DEVELOPMENT_DIR is '$ZMAP_MASTER_BUILD_DEVELOPMENT_DIR'
fi

chmod g+w $0 || zmap_message_err "Failed to chmod g+w $0"




#------------------------------------------------------------------------------
# Generate a checkout script inline to do the checking out, this will then get
# copied to the various hosts to do the checking out on those hosts.
#
gen_checkout_script=zmap_checkout_$$.sh

zmap_tmp_dir=""


# The 'EOF' means _no_ substitution takes place.
#
(
cat <<'EOF'
#!/bin/bash
#
# Generated! Can be found in build_bootstrap.sh
#
# By default this script does a repository checkout, alternative is to copy an 
# existing zmap directory, e.g. a feature branch directory.
#


ECHO=echo

CVS_MODULE=ZMap
CVS_ROOT=":ext:cvs.internal.sanger.ac.uk:/repos/cvs/zmap"
CVS_RSH="ssh"

BRANCH='develop'

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

zmap_message_out "checking that zmap_message_out works in generated script." 


_checkout_message_out "Start of checkout script (created by build_bootstrap)."

TODAY=`date +"%a %b %e %Y"`
_checkout_message_out "Today is $TODAY"


# Get the options the user may have requested
usage="$0 -b <branch> -f <zmap directory>"
while getopts ":b:f:" opt ; do
    case $opt in
	b  ) BRANCH=$OPTARG ;;
	f  ) ZMAP_MASTER_BUILD_COPY_DIR=$OPTARG ;;
	\? ) zmap_message_rm_exit "$usage"
    esac
done


save_root=$(pwd)

_checkout_message_out "Running in $save_root on $(hostname)"

zmap_tmp_dir=zmap.$$

_checkout_message_out "zmap_tmp_dir set to $zmap_tmp_dir"

_checkout_mk_cd_dir $zmap_tmp_dir

if [ "x$gen_checkout_script" != "x" ]; then
  # This is being run from the master (this code was sourced from build_bootstrap.sh). 
  # The .master directory should _not_ have any runconfig etc run in it!

  cd $save_root || _checkout_message_out "Failed to cd to $save_root"
  _checkout_message_out "Removing previously not removed zmap.<pid> dirs in $save_root"
  rm -rf zmap.*

  # unfortunately this needs recreating...
  _checkout_mk_cd_dir $zmap_tmp_dir

  if [ "x$ZMAP_MASTER_BUILD_COPY_DIR" == "x" ]; then

    # Need -P prune flag to ensure we don't get a load of old empty directories.
#    _checkout_message_out "Running cvs checkout $CVS_MODULE"
#    cvs -d$CVS_ROOT checkout -P -d $CVS_MODULE.master $CVS_MODULE || _checkout_message_exit "Failed to checkout $CVS_MODULE"
#    MASTER_SRC_DIR=$CVS_MODULE.master
#
#    _checkout_message_out "done a cvs checkout"

    MASTER_SRC_DIR=$CVS_MODULE.master

    # clone the zmap repository and switch to named branch.
    _checkout_message_out "Running git clone of zmap.git into $MASTER_SRC_DIR"
    git clone git.internal.sanger.ac.uk:/repos/git/annotools/zmap.git $MASTER_SRC_DIR

    _checkout_message_out "switching to git branch $BRANCH"
    ( cd $MASTER_SRC_DIR ; git checkout $BRANCH )

  else

      _checkout_message_out "ZMAP_MASTER_BUILD_COPY_DIR=$ZMAP_MASTER_BUILD_COPY_DIR"
 
    MASTER_SRC_DIR=$ZMAP_MASTER_BUILD_COPY_DIR

    _checkout_message_out "just doing a copy of $ZMAP_MASTER_BUILD_COPY_DIR"
  fi

  _checkout_message_out "About to  cp -r $MASTER_SRC_DIR $CVS_MODULE"
  cp -r $MASTER_SRC_DIR $CVS_MODULE  || _checkout_message_exit "Failed to copy src directory $MASTER_SRC_DIR"
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
#
# end of inline generated script section.
#------------------------------------------------------------------------------



# add checkout script to list of files to remove on exit
zmap_edit_variable_add FILES_TO_REMOVE $gen_checkout_script


CHECKOUT_OPTS=''

if [ "x$ZMAP_MASTER_BUILD_COPY_DIR" != "x" ]; then
  CHECKOUT_OPTS="$CHECKOUT_OPTS -f $ZMAP_MASTER_BUILD_COPY_DIR"
fi

if [ -n "$BRANCH" ] ; then
  CHECKOUT_OPTS="$CHECKOUT_OPTS -b $BRANCH"
fi


# Now source the generated checkout script in this scripts process.
# This both checks out ZMap module and sets variables for use later by this script!
#
zmap_message_out "Starting running checkout script $gen_checkout_script $CHECKOUT_OPTS"
. ./$gen_checkout_script $CHECKOUT_OPTS ||  { zmap_message_out "Failed to load ./$gen_checkout_script" ; exit 1 ; }
zmap_message_out "Finished running checkout script $gen_checkout_script"



# Here we copy from the development dir to the checked out one.  
if [ "x$ZMAP_MASTER_BUILD_DEVELOPMENT_DIR" != "x" ]; then
    zmap_message_out "*** WARNING: Developing! Using $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR ***"
    chmod 755 $BASE_DIR/*.sh
    scp -r $ZMAP_MASTER_BUILD_DEVELOPMENT_DIR/*.sh $BASE_DIR/
fi



# We're not going to actually do  a build here, but we will build docs
# if requested, and/or anything else that isn't architecture dependent

# Now we have a checkout we can source these
#. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
#set -o history
#. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }


zmap_message_out "MASTER VARIABLE SETTINGS:"
zmap_message_out "ZMAP_MASTER_HOST=$ZMAP_MASTER_HOST"
zmap_message_out "ZMAP_MASTER_NOTIFY_MAIL=$ZMAP_MASTER_NOTIFY_MAIL"
zmap_message_out "ZMAP_MASTER_BUILD_COPY_DIR=$ZMAP_MASTER_BUILD_COPY_DIR"
zmap_message_out "ZMAP_MASTER_BUILD_DEVELOPMENT_DIR=$ZMAP_MASTER_BUILD_DEVELOPMENT_DIR"
zmap_message_out "ZMAP_MASTER_BUILD_DOCS=$ZMAP_MASTER_BUILD_DOCS"
zmap_message_out "ZMAP_MASTER_BUILD_DOXYGEN_DOCS=$ZMAP_MASTER_BUILD_DOXYGEN_DOCS"
zmap_message_out "ZMAP_MASTER_CVS_RELEASE_NOTES=$ZMAP_MASTER_CVS_RELEASE_NOTES"
zmap_message_out "ZMAP_MASTER_RT_RELEASE_NOTES=$ZMAP_MASTER_RT_RELEASE_NOTES"
zmap_message_out "ZMAP_MASTER_FORCE_RELEASE_NOTES=$ZMAP_MASTER_FORCE_RELEASE_NOTES"
zmap_message_out "ZMAP_MASTER_DOCS2WEB=$ZMAP_MASTER_DOCS2WEB"
zmap_message_out "ZMAP_MASTER_WEBPUBLISH=$ZMAP_MASTER_WEBPUBLISH"
zmap_message_out "ZMAP_MASTER_BUILD_DIST=$ZMAP_MASTER_BUILD_DIST"
zmap_message_out "ZMAP_MASTER_BUILD_CANVAS_DIST=$ZMAP_MASTER_BUILD_CANVAS_DIST"
zmap_message_out "ZMAP_BUILD_MACHINES=$ZMAP_BUILD_MACHINES"
zmap_message_out "ZMAP_MASTER_RUN_TEST_SUITE=$ZMAP_MASTER_RUN_TEST_SUITE"
zmap_message_out "ZMAP_MASTER_INC_REL_VERSION=$ZMAP_MASTER_INC_REL_VERSION"
zmap_message_out "ZMAP_MASTER_INC_UPDATE_VERSION=$ZMAP_MASTER_INC_UPDATE_VERSION"


# We need to remove the previous one first...
# We've been  getting into trouble  with failed builds  leaving behind
# cluster config files that have odd hostnames for the mac in. When it
# looses network it thinks it's mac18480i.local rather than
#  mac18480i.int.sanger.ac.uk... Removing the file solves this.
zmap_remove_cluster_config

# We need to update the ZMAP_BUILD_MACHINES variable for cluster machines.
zmap_write_cluster_config

# And read what we've changed
zmap_read_cluster_config

# add cluster config file to list of files to remove on exit
zmap_edit_variable_add FILES_TO_REMOVE $ZMAP_CLUSTER_CONFIG_FILE


# Now we have all the configuration set up we need to
# check we can do the passwordless login to each of the machines...
# Stop failures later on.
zmap_message_out "Checking status of hosts [$ZMAP_BUILD_MACHINES]"
for host in $ZMAP_BUILD_MACHINES
  do
  # N.B. $0 below will be the login shell on host, not this script!
  # 3 seconds should be enough for any site machine.
  # StrictHostKeyChecking=no ignores new/changed hosts
  #  (copies/updates the key in ~/.ssh/known_hosts file)
  # PasswordAuthentication=no & NumberOfPasswordPrompts=0 makes sure 
  # we can do the password-less login. If not then it will fail 
  # instantly and the whole script will fail, not sit there waiting
  # for user input...
  # If it fails the key is to cat ~/.ssh/id_*.pub >> ~/.ssh/authorized_keys
  ssh -oStrictHostKeyChecking=no -oConnectTimeout=3 \
      -oSetupTimeOut=3 -oPasswordAuthentication=no \
      -oNumberOfPasswordPrompts=0 $host 'echo "[$0] `hostname` login ok"' \
      || zmap_message_rm_exit "Failed to login to $host..."
done
zmap_message_out "All hosts alive."






# For feature branch builds embed a feature branch ID in zmap code so it can be displayed to user.
if [ -n "$GIT_VERSION_INFO" ] ; then

    version_file="$SRC_DIR/zmapUtils/$ZMAP_VERSION_HEADER"

    zmap_message_out "Collecting GIT describe info..."

    GIT_VERSION_INFO=`cd $SCRIPTS_DIR ; $SCRIPTS_DIR/git_version.sh`

    zmap_message_out "Inserting GIT describe info..$GIT_VERSION_INFO into $version_file."

    $SCRIPTS_DIR/set_dev_description.pl $version_file $GIT_VERSION_INFO || zmap_message_exit "Failed to set git version in file $version_file"

fi



# Get current version stuff... Do this before building...
#
zmap_message_out "Fetching version using versioner script"
ZMAP_RELEASE_VERSION=$($SCRIPTS_DIR/versioner \
    -path $CHECKOUT_BASE/ \
    -show -V -quiet) || zmap_message_rm_exit "Failed to get zmap version"


zmap_message_out "*** INFORMATION: Version of zmap being built is $ZMAP_RELEASE_VERSION ***"


# LET'S TRY ALWAYS NAMING THE RELEASE DIRECTORY....
#if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
#    [ "x$RELEASE_LOCATION" == "x" ] && RELEASE_LOCATION=$ZMAP_RELEASES_DIR/ZMap.$ZMAP_RELEASE_VERSION.BUILD
#fi
RELEASE_LOCATION=$ZMAP_RELEASES_DIR/ZMap.$ZMAP_RELEASE_VERSION.BUILD

mkdir $RELEASE_LOCATION || zmap_message_rm_exit "Failed to create release directory $RELEASE_LOCATION"



# For feature branch builds embed a feature branch ID in zmap code so it can be displayed to user.
#if [ -n "$GIT_VERSION_INFO" ] ; then
#
#    version_file="$SRC_DIR/zmapUtils/$ZMAP_VERSION_HEADER"
#
#    zmap_message_out "Collecting GIT describe info..."
#
#    GIT_VERSION_INFO=`cd $SCRIPTS_DIR ; $SCRIPTS_DIR/git_version.sh`
#
#    zmap_message_out "Inserting GIT describe info..$GIT_VERSION_INFO into $version_file."
#
#    $SCRIPTS_DIR/set_dev_description.pl $version_file $GIT_VERSION_INFO || zmap_message_exit "Failed to set git version in file $version_file"
#
#fi




# Make the release notes, this needs to happen before building and before
# possibly freezing the cvs as we lock the release notes to the binary
# using a #define
#
if [ "x$ZMAP_MASTER_RT_RELEASE_NOTES" == "x$ZMAP_TRUE" ]; then

    NO_CVS=""
    if [ "x$ZMAP_MASTER_TAG_CVS" != "x$ZMAP_TRUE" ]; then
	NO_CVS='-n'
    fi

    FORCE_NOTES=""
    if [ "x$ZMAP_MASTER_FORCE_RELEASE_NOTES" == "x$ZMAP_TRUE" ]; then
        FORCE_NOTES='-f'
    fi

    $SCRIPTS_DIR/zmap_make_rt_release_notes.sh $NO_CVS $FORCE_NOTES $CHECKOUT_BASE || \
	zmap_message_exit "Failed to build release notes from Request Tracker"

    # We need to copy the changed web header into the master directory from the directory it was run in.
    # $SCRIPTS_DIR looks like /var/tmp/zmap.<rand>/ZMap/scripts and will edit the header found in there.
    # We do it here as it doesn't seem to be the responsibility of zmap_build_rt_release_notes.sh

    PATH_TO_MODIFIED_WEB_HEADER=$(find $ZMAP_BUILD_CONTAINER/$CVS_MODULE -name $ZMAP_WEBPAGE_HEADER | grep -v CVS)

    PATH_TO_MASTER_WEB_HEADER=$(find $ZMAP_BUILD_CONTAINER/$MASTER_SRC_DIR -name $ZMAP_WEBPAGE_HEADER | grep -v CVS)

    chmod u+w $PATH_TO_MASTER_WEB_HEADER || zmap_message_err  "Failed to chmod $PATH_TO_MASTER_WEB_HEADER"
    rm -f $PATH_TO_MASTER_WEB_HEADER     || zmap_message_exit "Failed to remove $PATH_TO_MASTER_WEB_HEADER"
    cp $PATH_TO_MODIFIED_WEB_HEADER $PATH_TO_MASTER_WEB_HEADER || zmap_message_exit "Failed to cp web header"

    if [ "x$ZMAP_MASTER_RT_TO_CVS" == "x$ZMAP_TRUE" ]; then
	# The release notes file will need editing. mail zmapdev about that.
	FILE_DATE=$(date "+%Y_%m_%d")
	RELEASE_NOTES_OUTPUT="${ZMAP_RELEASE_FILE_PREFIX}.${FILE_DATE}.${ZMAP_RELEASE_FILE_SUFFIX}"
	(cat <<EOF
ZMap Build Needs You.

ZMap $ZMAP_RELEASE_VERSION Release notes file needs editing.

Find the file here when the build finishes (assuming it does).

$RELEASE_LOCATION/ZMap/doc/IntWeb/Release_notes/$RELEASE_NOTES_OUTPUT

After modifying the file it needs  cvs updated and to be copied to the
website.   Use  the  $RELEASE_LOCATION/ZMap/scripts/zmap_update_web.sh
script, with no arguments to do the latter.

EOF
	) | mailx -s "Release Notes Created" $ZMAP_MASTER_NOTIFY_MAIL
    fi
fi



# If requested, tag cvs to 'freeze' the code.
# Logic is: To tag using  the version _in_  cvs. This would  have been
# incremented the last time this script was run!
#
if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Fetching cvs tag using versioner script"
    cvs_tag=$($SCRIPTS_DIR/versioner \
	-path $CHECKOUT_BASE/ \
	-show -quiet) || zmap_message_exit "Failed to get zmap version"

    zmap_message_out "Tagging cvs using versioner script"

    zmap_message_out "Will use cvs tag $cvs_tag"

    $SCRIPTS_DIR/versioner -tag -path $CHECKOUT_BASE || zmap_message_exit "Failed to cvs tag zmap"
fi




# ================== FARM OFF BUILDS ================== 


tar_target=$(hostname):$ZMAP_BUILD_CONTAINER


#if [ "x$ZMAP_MASTER_BUILD_COPY_DIR" == "x" ]; then
  MASTER_SRC_REMOTEPATH=$tar_target/$CVS_MODULE
#else
#  MASTER_SRC_REMOTEPATH=$(hostname):$MASTER_SRC_DIR
#fi





HOSTS_OK=0
HOSTS_FAILED=0
HOSTS_RUN=0

for host in $ZMAP_BUILD_MACHINES
  do
  zmap_message_out "Logging into $host to run build there."


  #-----------------------------------------------------------------------------------
  # Generate the build script inline.
  # N.B. Substitution _will_ occur in this HERE doc.
  #
  (cat $gen_checkout_script - <<EOF
# from 'for host in \$ZMAP_BUILD_MACHINES' loop

# Because we can't cvs checkout reliably on some machines!
# See RT ticket #58607
_checkout_message_out "scp -r $MASTER_SRC_REMOTEPATH ./"
scp -r $MASTER_SRC_REMOTEPATH/ ./


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

_checkout_message_out "Running ./zmap_compile_and_tar.sh $options TAR_TARGET=$tar_target"

\$SCRIPTS_DIR/zmap_compile_and_tar.sh $options TAR_TARGET=$tar_target || _checkout_message_exit "Failed to build"

\$SCRIPTS_DIR/zmap_fetch_acedbbinaries.sh $tar_target $ZMAP_ACEDB_RELEASE_DIR || _checkout_message_exit "Failed to get acedb binaries."

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
  # end of inline generated script/run
  #-----------------------------------------------------------------------------------


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
	  if [ "x$RELEASE_LOCATION" != "x" ]; then
	      echo "unless this is the only host to fail in which case try" >> fail.log
	      echo "$RELEASE_LOCATION/$host.log"                            >> fail.log
	  fi
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

# remove it from list of files to remove on exit
zmap_edit_variable_del FILES_TO_REMOVE $gen_checkout_script



if [ $HOSTS_FAILED == $HOSTS_RUN ]; then
    zmap_message_rm_exit "Build failed on _all_ hosts!"
fi





# ================== MASTER BUILD TASKS ================== 

# We need to run bootstrap and  configure to get the make file targets
# docs and dist.

zmap_message_out "Starting Master build tasks. make docs, dist etc..."

zmap_message_out "Path is" $PATH

zmap_cd $SRC_DIR

zmap_message_out "Running bootstrap"

./bootstrap || zmap_message_rm_exit "Failed to bootstrap"

zmap_message_out "Running runconfig"

./runconfig || zmap_message_rm_exit "Failed to runconfig"


if [ "x$ZMAP_MASTER_CVS_RELEASE_NOTES" == "x$ZMAP_TRUE" ]; then
    zmap_message_err "Need to code release notes bit..."

    # This is done in zmap_build_rt_release_notes.sh
    $SCRIPTS_DIR/zmap_make_cvs_release_notes.sh || zmap_message_rm_exit "Failed to successfully build release notes from cvs."
fi


if [ "x$ZMAP_MASTER_BUILD_DOCS" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running $SCRIPTS_DIR/zmap_make_docs ..."

    $SCRIPTS_DIR/zmap_make_docs.sh  || zmap_message_rm_exit "Failed to successfully run zmap_make_docs.sh"

    zmap_message_out "Running $SCRIPTS_DIR/zmap_update_web.sh ..."

    $SCRIPTS_DIR/zmap_update_web.sh || zmap_message_rm_exit "Failed to successfully run zmap_update_web.sh"
fi


if [ "x$ZMAP_MASTER_BUILD_DIST" == "x$ZMAP_TRUE" ]; then
    # make dist
    # put somewhere useful!
    zmap_message_out "Running make dist ..."

    make dist || zmap_message_rm_exit "Failed to make distribution file"

    zmap_message_out "mkdir $ZMAP_BUILD_CONTAINER/Dist"

    mkdir -p $ZMAP_BUILD_CONTAINER/Dist

    cp zmap*.tar.gz $ZMAP_BUILD_CONTAINER/Dist/ || zmap_message_rm_exit "Failed to copy distribution to Dist folder."
fi


# version inc...
if [ "x$ZMAP_MASTER_INC_REL_VERSION" == "x$ZMAP_TRUE" ]; then

    $SCRIPTS_DIR/versioner -path $CHECKOUT_BASE -increment -release -cvs || \
	zmap_message_rm_exit "Failed to update release version in cvs"
    zmap_message_out "ZMap Version - release number incremented."

elif [ "x$ZMAP_MASTER_INC_UPDATE_VERSION" == "x$ZMAP_TRUE" ]; then

    $SCRIPTS_DIR/versioner -path $CHECKOUT_BASE -increment -update -cvs || \
	zmap_message_rm_exit "Failed to update update version in cvs"
    zmap_message_out "ZMap Version - update number incremented."

fi


zmap_cd $save_root


TAR_FILE=$(pwd)/Complete_build.tar.gz
# add it to list of files to remove on exit
zmap_edit_variable_add FILES_TO_REMOVE $TAR_FILE

zmap_message_out "About to do: tar -zcf$TAR_FILE $zmap_tmp_dir"
tar -zcf$TAR_FILE $zmap_tmp_dir || zmap_message_rm_exit "Failed to create tar file of $zmap_tmp_dir"
#tar -zcf$TAR_FILE $zmap_tmp_dir || zmap_message_exit "Failed to create tar file of $zmap_tmp_dir"


# Turning this off for now as it makes life difficult for anacode.
#
#if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
#    zmap_tar_old_releases $ZMAP_RELEASES_DIR
#    zmap_delete_ancient_tars $ZMAP_RELEASES_DIR
#fi


if [ "x$RELEASE_LOCATION" == "x" ]; then
    # This should really be a fatal error
    RELEASE_LOCATION=$ZMAP_RELEASES_DIR/LATEST_BUILD
    zmap_message_err "*** WARNING: Defaulting release dir to $RELEASE_LOCATION ***"
fi


$SCRIPTS_DIR/zmap_handle_release_tar.sh -t $TAR_FILE -r $RELEASE_LOCATION || \
    zmap_message_rm_exit "Failed to release what we've built here today."
#$SCRIPTS_DIR/zmap_handle_release_tar.sh -t $TAR_FILE -r $RELEASE_LOCATION || \
#    zmap_message_err "Failed to release what we've built here today."



if [ "x$ZMAP_MASTER_TAG_CVS" == "x$ZMAP_TRUE" ]; then
    $SCRIPTS_DIR/zmap_symlink.sh -r $RELEASE_LOCATION -l $ZMAP_RELEASE_LEVEL || \
	zmap_message_rm_exit "Failed to update symlink"
fi


if [ "x$ZMAP_MASTER_RUN_TEST_SUITE" == "x$ZMAP_TRUE" ]; then
    # run the test suite
    zmap_message_out "Running test suite"

    $SCRIPTS_DIR/zmap_test_suite.sh RELEASE_LOCATION=$RELEASE_LOCATION || zmap_message_rm_exit "Failed to run the test suite"

    zmap_message_out "Finished the test suite"
fi


# Remove our temp directories.
if [ "x$ZMAP_MASTER_REMOVE_FOLDER" == "x$ZMAP_TRUE" ]; then
    rm -rf $zmap_tmp_dir || zmap_message_rm_exit "Failed to remove $zmap_tmp_dir"
fi


# And the tar file
rm -f $TAR_FILE || zmap_message_rm_exit "Failed to remove $TAR_FILE"

# remove it from list of files to remove on exit
zmap_edit_variable_del FILES_TO_REMOVE $TAR_FILE


# N.B. We now have no $SCRIPTS_DIR


# Looks like success... Checking versions match (non-fatal errors)
if [ -d $RELEASE_LOCATION ]; then

    zmap_uname_location=$RELEASE_LOCATION/$(uname -ms | sed -e "s/ /_/g")/bin/zmap

    if [ -x $zmap_uname_location ]; then
	zmap_message_out "Checking zmap binary version..."

	bin_version=$($zmap_uname_location --version) || zmap_message_err "*** CRITICAL: Cannot execute binary at '$zmap_uname_location' [1] *** "
	zmap_message_out "Binary reports version=$bin_version"

	bin_version=$(echo $bin_version | sed -e 's!\.!-!g; s!ZMap - !!')

	if [ "x$bin_version" != "x$ZMAP_RELEASE_VERSION" ]; then
	    zmap_message_err "*** WARNING: Executable reports _different_ version to Source Code! ***"
	    zmap_message_err "*** WARNING: Expected $ZMAP_RELEASE_VERSION, got $bin_version. ***"
	fi
    else
	zmap_message_err "*** CRITICAL: Cannot execute binary at '$zmap_uname_location' [2] ***"
    fi

else

    zmap_message_err "*** CRITICAL: Cannot find release location '$RELEASE_LOCATION' ***"

fi


# Nothing else needs this now
zmap_remove_cluster_config



zmap_message_out ""
zmap_message_out "Results:"
zmap_message_out "--------"
zmap_message_out "Build run on $ZMAP_BUILD_MACHINES"
zmap_message_out "Version according to binaries should be $ZMAP_RELEASE_VERSION"
zmap_message_out "Binaries, code, doc, dist etc can be found in $RELEASE_LOCATION"
zmap_message_out "For more information see $RELEASE_LOCATION/README"
zmap_message_out "--------"
zmap_message_out "Successfully reached last line of script!"

exit $RC
# ================== END OF SCRIPT ================== 
