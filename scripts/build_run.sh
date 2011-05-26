
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

# SSH_ID is userid which we use to ssh in for other machines.
SSH_ID='zmap'

# ERROR_RECIPIENT= Someone to email
ERROR_RECIPIENT='zmapdev@sanger.ac.uk'

# default mail subject.
MAIL_SUBJECT="ZMap Build Failed (control script)"

# where everything is located.
BASE_DIR=~zmap
SCRIPTS_DIR="$BASE_DIR/BUILD_SCRIPTS/ZMap/scripts"
BUILDS_DIR="$BASE_DIR/BUILDS"


# CVS_CHECKOUT_SCRIPT= The bootstrapping script that starts everything
BUILD_SCRIPT="$SCRIPTS_DIR/build_bootstrap.sh"
FUNCTIONS_SCRIPT="$SCRIPTS_DIR/zmap_functions.sh"


# Various build params.
OUTPUT=$BUILDS_DIR
SLEEP=15
TAG_RELEASE_BUILD=no
TAG_CVS=''
INC_REL_VERSION=''
INC_UPDATE_VERSION=''
RELEASES_DIR=''
RT_TO_CVS=''
FEATURE_DIR=''
BATCH=''
GIT_FEATURE_INFO=''
ZMAP_MASTER_BUILD_DIST=''



# try to load useful shared shell functions...after this we will have access to
# common message funcs.
. $FUNCTIONS_SCRIPT || { echo "Failed to load common functions file: $FUNCTIONS_SCRIPT"; exit 1; }


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

    if [ -n "$BATCH" ] ; then
	zmap_message_out "$*" | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT; 
    fi

    exit 1
}



# Do args.
#
usage="$PROGNAME [ -a <user_mail_id> -d -e -f <zmap feature branch> -g -l <release_dir in BUILDS> -m -n -t -r -u -z ]   <build prefix>"

while getopts ":a:bdf:gil:mrtuz:" opt ; do
    case $opt in
	a  ) ERROR_RECIPIENT=$OPTARG ;;
	b  ) BATCH='yes' ;;
	d  ) ZMAP_MASTER_BUILD_DIST='yes'   ;;
	f  ) FEATURE_DIR=$OPTARG ;;
	g  ) GIT_FEATURE_INFO='-g' ;;
	l  ) RELEASE_LOCATION=$OPTARG ;;
	m  ) RT_TO_CVS='no' ;;
	n  ) ZMAP_MASTER_RT_RELEASE_NOTES='yes'   ;;
	r  ) INC_REL_VERSION='-r'    ;;
	t  ) TAG_CVS='-t' ;;
	u  ) INC_UPDATE_VERSION='-u' ;;
	z  ) RELEASES_DIR=$OPTARG ;;
	\? ) message_exit "Bad arg flag: $usage" ;;
    esac
done


# Process final arg, should be build prefix....
#
shift $(($OPTIND - 1))

if [ $# == 1 ]; then
    BUILD_PREFIX=$1

    # redo mail subject now we have a name for the build.
    MAIL_SUBJECT="ZMap $BUILD_PREFIX Failed (control script)"
else
  message_exit "No build prefix specifed: $usage"
fi


# We do not know the directory for the logfile until here so cannot start logging
# until this point, from this point this script prints any messages to stdout
# _and_ to the logfile.
#
GLOBAL_LOG=$BUILDS_DIR/$BUILD_PREFIX.LOG


# remove any previous log.
rm -f $GLOBAL_LOG || message_exit "Cannot remove log from previous build: $GLOBAL_LOG"


message_out "ZMap $BUILD_PREFIX Build Started"


# do some set up and basic checks.......
#

if [ -n "$FEATURE_DIR" ] ; then
    if [ ! -d $FEATURE_DIR ] || [ ! -r $FEATURE_DIR ] ; then
	message_exit "$FEATURE_DIR is not a directory or is not readable."
    fi
fi

mkdir -p $OUTPUT || message_exit "Cannot mkdir $OUTPUT" 


# We don't support this option at the moment, it seems kind of useless
# given that we either check out a new version of the code or don't want
# the code updated at all.
#
#if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
#    old_dir=$(pwd)
#    new_dir=$(dirname  $BUILD_SCRIPT)
#    up2date=$(basename $BUILD_SCRIPT)
#    cd $new_dir
#    export CVS_RSH=ssh
#    cvs update -C $up2date || { 
#	echo "Failed to cvs update $BUILD_SCRIPT"
#	echo "Failed to cvs update $BUILD_SCRIPT" | \
#	    mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT; 
#	exit 1; 
#    }
#    cd $old_dir
#fi




# Plug together command options..........
#

# all single letter flags...
CMD_OPTIONS="$TAG_CVS $INC_REL_VERSION $INC_UPDATE_VERSION $GIT_FEATURE_INFO"

# Now flags with options
if [ -n "$FEATURE_DIR" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS -f $FEATURE_DIR"

fi

if [ -n "$RELEASE_LOCATION" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS RELEASE_LOCATION=$BUILDS_DIR/$RELEASE_LOCATION"

fi


if [ -n "$RELEASES_DIR" ] ; then

  CMD_OPTIONS="$CMD_OPTIONS ZMAP_RELEASES_DIR=$BUILDS_DIR/$RELEASES_DIR"

fi

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
if [ -z "$BATCH" ] ; then

    message_out "Build parameters are:"

    cat <<EOF
===================
        Running on: '$(hostname)'
      Build script: '$BUILD_SCRIPT'
      Build_prefix: '$BUILD_PREFIX'
   Command options: '$CMD_OPTIONS'
        Global log: '$GLOBAL_LOG'
Errors reported to: '$ERROR_RECIPIENT'
===================
EOF

fi


# If not run as batch then give user a chance to cancel, must be before we
# start ssh otherwise we will leave running processes all over the place.
# After this we trap any attempts to Cntl-C etc.
#
if [ -z "$BATCH" ] ; then
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
BASE_DIR='$SCRIPT_DIR' ./root_checkout.sh '$CMD_OPTIONS' || _rm_exit; \
:                                     ; \
rm -f root_checkout.sh || exit 1;   \
"' >> $GLOBAL_LOG 2>&1


# ================== ERROR HANDLING ================== 

# should we mail about build success or just for batch mode...???

if [ $? != 0 ]; then
    # There was an error, email someone about it!
    TMP_LOG=/tmp/zmap_fail.$$.log
    echo "ZMap Build Failed"                              > $TMP_LOG
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
    MAIL_SUBJECT="ZMap $BUILD_PREFIX Succeeded"

    if [ "x$ERROR_RECIPIENT" != "x" ]; then
	tail $GLOBAL_LOG | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT
    fi
fi


message_out $MAIL_SUBJECT


exit $RC
# ================== END OF SCRIPT ================== 
