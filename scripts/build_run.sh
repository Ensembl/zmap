#!/bin/bash
#
# Do not use directly, instead use one of the zmap_build_XXXX scripts
# which call this script having set up the required parameters to
# do different sorts of builds.
#
# Here's some assumptions about how this script runs....
#
# Assumes it's running in it's parent "scripts" directory so that the
# scripts that it calls are all ./XXXXXXX
#
#


# Program stuff.
RC=0
PROGNAME=`basename $0`
GLOBAL_LOG=''


# Configuration variables
#

# SRC_MACHINE= The machine to log into to start everything going
SRC_MACHINE='tviewsrv'
#SRC_MACHINE='deskpro16113'

# SSH_ID is userid which we use to ssh in for other machines.
SSH_ID='zmap'

# ERROR_RECIPIENT= Someone to email
ERROR_RECIPIENT='zmapdev@sanger.ac.uk'

# default mail subject.
MAIL_SUBJECT="ZMap Build Failed (control script)"


# where build scripts are located.
BASE_DIR=~zmap
SCRIPTS_DIR="$BASE_DIR/BUILD_CHECKOUT/ZMap/scripts"

BUILD_SCRIPT="$SCRIPTS_DIR/build_bootstrap.sh"
FUNCTIONS_SCRIPT="$SCRIPTS_DIR/zmap_functions.sh"

# Our project space where we store builds.
PROJECT_DIR='/nfs/zmap'
BUILDS_DIR="$PROJECT_DIR/BUILDS"


# name of symbolic link from ~zmap to build dir in project directories in /nfs/zmap
LINK_PREFIX='BUILD'



# Various build params.
SLEEP=15
TAG_RELEASE_BUILD=no
TAG_CVS=''
INC_REL_VERSION=''
INC_UPDATE_VERSION=''
RT_TO_CVS=''
ERASE_SUBDIRS=''
INPUT_DIR=''
OUTPUT_DIR=''
BRANCH='develop'
CRON=''
GIT_FEATURE_INFO=''
ZMAP_MASTER_BUILD_DIST=''
ZMAP_MASTER_RT_RELEASE_NOTES=''
ZMAP_MASTER_FORCE_RELEASE_NOTES=''


# try to load useful shared shell functions...after this we will have access to
# common message funcs.
. $FUNCTIONS_SCRIPT || { echo "Aborted $PROGNAME - Failed to load common functions file: $FUNCTIONS_SCRIPT"; exit 1; }



# This script needs to explicitly send stuff to the log file whereas
# the script makes sure that output from scripts it calls goes to the logfile
# automatically. Hence here we have some variants on the zmap_message_ funcs.
#

# Usage:    message_out "Your Message"
function message_out
{
    zmap_message_out "$*"

    if [ -n "$GLOBAL_LOG" ] ; then
	zmap_message_out "$*" >> $GLOBAL_LOG
    fi
}

# Usage:    message_exit "Your Message"
function message_exit
{
    message_out $*

    if [ -n "$CRON" ] ; then
	zmap_message_out "$*" | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT; 
    fi

    exit 1
}


# Ok...off we go....
#
message_out "ZMap Build Started: $*"




# Do args.
#
usage="$PROGNAME [ -a <user_mail_id> -b <git branch> -c -d -e -g -i <input directory> -m -n -o <output directory> -t -r -u ]   <build prefix>"

while getopts ":a:b:cdegi:mno:rtu" opt ; do
    case $opt in
	a  ) ERROR_RECIPIENT=$OPTARG ;;
	b  ) BRANCH=$OPTARG ;;
	c  ) CRON='yes' ;;
	d  ) ZMAP_MASTER_BUILD_DIST='yes'   ;;
	e  ) ERASE_SUBDIRS='yes' ;;
	g  ) GIT_FEATURE_INFO='-g' ;;
	i  ) INPUT_DIR=$OPTARG ;;
	m  ) RT_TO_CVS='no' ;;
	n  ) ZMAP_MASTER_RT_RELEASE_NOTES='yes' ;;
	o  ) OUTPUT_DIR=$OPTARG ;;
	r  ) INC_REL_VERSION='-r'    ;;
	t  ) TAG_CVS='-t' ;;
	u  ) INC_UPDATE_VERSION='-u' ;;
	\? ) message_exit "Bad arg flag: $usage" ;;
    esac
done


# Process final arg, should be build prefix....
#
shift $(($OPTIND - 1))

if [ $# == 1 ]; then
    BUILD_PREFIX=$1

    # redo _default_ mail subject now we have a name for the build.
    MAIL_SUBJECT="ZMap $BUILD_PREFIX Build Failed (control script)"
else
  message_exit "No build prefix specifed: $usage"
fi


# The parent directory for the build.
#
# NOTE, if you change the way the parent directory is named then you
# should edit the crontab for the overnight build to update the directory there too.
#
PARENT_BUILD_DIR="$BUILDS_DIR/$BUILD_PREFIX"_BUILDS
if [ ! -d $PARENT_BUILD_DIR ] || [ ! -r $PARENT_BUILD_DIR ] ; then
    message_exit "$PARENT_BUILD_DIR is not a directory or is not readable."
fi


# We link from zmap home dir to our projects directory to make it easy for
# users to pick up the latest development/production/etc builds.
#
LINK_NAME="$BASE_DIR/$LINK_PREFIX.$BUILD_PREFIX"



# We do not know the directory for the logfile until here so cannot start logging
# until this point, from this point this script prints any messages to stdout
# _and_ to the logfile.
#
# Actually when run from cron output is already redirected to correct file.
#
# NOTE, if you change the name of the log file then you should edit the crontab
# for the overnight build to update the name there too.
#
GLOBAL_LOG="$PARENT_BUILD_DIR/$BUILD_PREFIX"_BUILD.LOG


# remove any previous log.
rm -f $GLOBAL_LOG || message_exit "Cannot remove log from previous build: $GLOBAL_LOG"


message_out "ZMap Build is $BUILD_PREFIX"


# For some types of builds we do not want to keep the old builds. e.g. overnight builds.
#
if [ -n "$ERASE_SUBDIRS" ] ; then
    sub_dirs_pattern="$PARENT_BUILD_DIR/ZMap.develop-Release_*"
    message_out "Removing previous builds with name: $sub_dirs_pattern"
    rm -rf $sub_dirs_pattern || message_exit "Failed to remove previous builds: $sub_dirs_pattern."
fi


if [ -n "$INPUT_DIR" ] ; then
    if [ ! -d $INPUT_DIR ] || [ ! -r $INPUT_DIR ] ; then
	message_exit "$INPUT_DIR is not a directory or is not readable."
    fi
fi


if [ -n "$OUTPUT_DIR" ] ; then
    mkdir $OUTPUT_DIR || message_exit "$INPUT_DIR is not a directory or is not readable."
fi



# Plug together command options..........
#

# all single letter flags...
CMD_OPTIONS="$TAG_CVS $INC_REL_VERSION $INC_UPDATE_VERSION $GIT_FEATURE_INFO"

# Now flags with options
if [ -n "$INPUT_DIR" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS -f $INPUT_DIR"

fi


if [ -n "$BRANCH" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS -b $BRANCH"

fi


# now env. variables. I'd like to supplant these with cmd line flags.

CMD_OPTIONS="$CMD_OPTIONS ZMAP_RELEASES_DIR=$PARENT_BUILD_DIR"

CMD_OPTIONS="$CMD_OPTIONS ZMAP_LINK_NAME=$LINK_NAME"


if [ -n "$RT_TO_CVS" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS ZMAP_MASTER_RT_TO_CVS=$RT_TO_CVS"

fi


if [ -n "$ZMAP_MASTER_RT_RELEASE_NOTES" ] ; then

    CMD_OPTIONS="$CMD_OPTIONS ZMAP_MASTER_RT_RELEASE_NOTES=$ZMAP_MASTER_RT_RELEASE_NOTES"

    # for now we force the release notes production....
    CMD_OPTIONS="$CMD_OPTIONS ZMAP_MASTER_FORCE_RELEASE_NOTES=yes"

fi


if [ -n "$ZMAP_MASTER_BUILD_DIST" ] ; then

    CMD_OPTIONS="$CMD_OPTIONS ZMAP_MASTER_BUILD_DIST=$ZMAP_MASTER_BUILD_DIST"

fi




#
# OK, off we go......
#

# Give some build info....
#
if [ -z "$CRON" ] ; then

    message_out "=================== Build parameters:"
    message_out "        Running on: $(hostname)"
    message_out "      Build branch: $BRANCH"
    message_out "      Build script: $BUILD_SCRIPT"
    message_out "      Build prefix: $BUILD_PREFIX"
    message_out "   Build directory: $PARENT_BUILD_DIR"
    message_out "    Link directory: $LINK_NAME"
    message_out "   Command options: $CMD_OPTIONS"
    message_out "        Global log: $GLOBAL_LOG"
    message_out "Errors reported to: $ERROR_RECIPIENT"
    message_out "==================="

fi



# If not run as cron then give user a chance to cancel, must be before we
# start ssh otherwise we will leave running processes all over the place.
# (After this we trap any attempts to Cntl-C etc.)
#
if [ -z "$CRON" ] ; then
    message_out "Pausing for $SLEEP seconds, hit Cntl-C to abort cleanly."

    sleep $SLEEP
fi


# trap any attempts by user to kill script to prevent dangling processes on other machines.
#
trap '' INT
trap '' TERM
trap '' QUIT

message_out "Build now running and cannot be cleanly aborted..."


# A one step copy, run, cleanup!
# The /bin/kill -9 -$$; line is there to make sure no processes are left behind if the
# root_checkout.sh looses one... In testing, but appears kill returns non-zero in $?
# actually it's probably the bash process returning the non-zero, but the next test
# appears to succeed if we enter _rm_exit(), which is what we want. 
# We use ssh -x so that during testing FowardX11=no is forced so that zmap_test_suite.sh
# doesn't try to use the forwarded X11 connection, which will be the user's own not zmap's
cat $BUILD_SCRIPT | ssh "$SSH_ID@$SRC_MACHINE" '/bin/bash -c "\
function _rm_exit                       \
{                                       \
   echo Master Script Failed...;        \
   rm -f root_checkout.sh;              \
   /bin/kill -9 -$$;                    \
   exit 1;                              \
};                                      \
cd /var/tmp                || exit 1;   \
rm -f root_checkout.sh     || exit 1;   \
cat - > root_checkout.sh   || exit 1;   \
chmod 755 root_checkout.sh || _rm_exit; \
: Change the variables in next line   ; \
 BASE_DIR='$SCRIPTS_DIR' ./root_checkout.sh '$CMD_OPTIONS' || _rm_exit; \
:                                     ; \
rm -f root_checkout.sh || exit 1;   \
"' >> $GLOBAL_LOG 2>&1


# ================== ERROR HANDLING ================== 

# should we mail about build success or just for cron mode...???

if [ $? != 0 ]; then
    # There was an error, email someone about it!
    TMP_LOG=/tmp/zmap_fail.$$.log
    echo "ZMap $BUILD_PREFIX Build Failed"                > $TMP_LOG
    echo ""                                              >> $TMP_LOG
    echo "Tail of log:"                                  >> $TMP_LOG
    echo ""                                              >> $TMP_LOG
    tail $GLOBAL_LOG                                     >> $TMP_LOG
    echo ""                                              >> $TMP_LOG
    echo "Full log can be found $(hostname):$GLOBAL_LOG" >> $TMP_LOG
    if [ "x$ERROR_RECIPIENT" != "x" ]; then
	cat $TMP_LOG | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT
    fi
    rm -f $TMP_LOG

    RC=1
else
    MAIL_SUBJECT="ZMap $BUILD_PREFIX Build Succeeded"

    if [ "x$ERROR_RECIPIENT" != "x" ]; then
	tail $GLOBAL_LOG | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT
    fi
fi


message_out $MAIL_SUBJECT


exit $RC
# ================== END OF SCRIPT ================== 
