
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



# Usage:    message_out "Your Message"
function message_out
{
    now=$(date +%H:%M:%S)
    echo "[$PROGNAME ($now)] $*"

    if [ -n $BATCH ] ; then
	echo $* | mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT; 
    fi
}

# Usage:    message_exit "Your Message"
function message_exit
{
    message_out $*
    exit 1
}



RC=0

PROGNAME=`basename $0`


echo "args: $*"


# ================== CONFIG ================== 
# Configuration variables

# where everything is located.
BASE_DIR=~zmap

# SRC_MACHINE= The machine to log into to start everything going
SRC_MACHINE=tviewsrv

# CVS_CHECKOUT_SCRIPT= The bootstrapping script that starts everything
CVS_CHECKOUT_SCRIPT=$BASE_DIR/BUILD_SCRIPTS/ZMap/scripts/build_bootstrap.sh

# Output place for build
BUILDS_DIR="$BASE_DIR/BUILDS"


# ERROR_RECIPIENT= Someone to email
ERROR_RECIPIENT='zmapdev@sanger.ac.uk'
MAIL_SUBJECT="ZMap Build Failed (control script)"


# OUTPUT dir
OUTPUT=$BUILDS_DIR

# Give user a chance to cancel.
SLEEP=15

# Is build a tag/release build ?
TAG_RELEASE_BUILD=no


TAG_CVS=''
INC_REL_VERSION=''
INC_UPDATE_VERSION=''
RELEASES_DIR=''
RT_TO_CVS=''
FEATURE_DIR=''
BATCH=''
GIT_FEATURE_INFO=''


# Do args.
#
usage="$PROGNAME [ -a <user_mail_id> -d -e -f <zmap feature branch> -g -l <release_dir in BUILDS> -m -n -t -r -u -z ]   <build prefix>"

while getopts ":a:bdf:gil:mrtuz:" opt ; do
    case $opt in
	a  ) ERROR_RECIPIENT=$OPTARG ;;
	b  ) BATCH='yes' ;;
	d  ) ZMAP_MASTER_BUILD_DIST=$ZMAP_TRUE   ;;
	f  ) FEATURE_DIR=$OPTARG ;;
	g  ) GIT_FEATURE_INFO='-g' ;;
	l  ) RELEASE_LOCATION=$OPTARG ;;
	m  ) RT_TO_CVS='no' ;;
	n  ) ZMAP_MASTER_RT_RELEASE_NOTES=$ZMAP_TRUE   ;;
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

  MAIL_SUBJECT="ZMap $BUILD_PREFIX Failed (control script)"
else
  message_exit "No build prefix specifed: $usage"
fi


# GLOBAL_LOG= The place to hold the log file
GLOBAL_LOG=$BUILDS_DIR/$BUILD_PREFIX.LOG



# Plug together options..........
#

# simple flags first...
CMD_OPTIONS="$TAG_CVS $INC_REL_VERSION $INC_UPDATE_VERSION $GIT_FEATURE_INFO"


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





if [ -z $BATCH ] ; then

  cat <<EOF

'$0' build script running on '$(hostname)', 

about to run build script '$CVS_CHECKOUT_SCRIPT' on '$SRC_MACHINE' (via ssh)

build_prefix is '$BUILD_PREFIX'

with command options '$CMD_OPTIONS'

global output log is '$GLOBAL_LOG'

errors will be reported to '$ERROR_RECIPIENT'

EOF

fi




# OK, off we go......
#


# do some checks.......
#

if ! echo $GLOBAL_LOG | egrep -q "(^)/" ; then
    GLOBAL_LOG=$(pwd)/$GLOBAL_LOG
fi


if [ -n "$FEATURE_DIR" ] ; then
    if [ ! -d $FEATURE_DIR ] || [ ! -r $FEATURE_DIR ] ; then
	message_exit "$FEATURE_DIR is not a directory or is not readable."
    fi
fi

mkdir -p $OUTPUT || message_exit "Cannot mkdir $OUTPUT" 

rm -f $GLOBAL_LOG || message_exit "Cannot rm -f $GLOBAL_LOG"



# make sure a couple of things are sane.
SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi


if ! echo $CVS_CHECKOUT_SCRIPT | egrep -q "(^)/" ; then
    CVS_CHECKOUT_SCRIPT=$INITIAL_DIR/$SCRIPT_DIR/$CVS_CHECKOUT_SCRIPT
fi


# We don't support this option at the moment, it seems kind of useless
# given that we either check out a new version of the code or don't want
# the code updated at all.
#
#if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
#    old_dir=$(pwd)
#    new_dir=$(dirname  $CVS_CHECKOUT_SCRIPT)
#    up2date=$(basename $CVS_CHECKOUT_SCRIPT)
#    cd $new_dir
#    export CVS_RSH=ssh
#    cvs update -C $up2date || { 
#	echo "Failed to cvs update $CVS_CHECKOUT_SCRIPT"
#	echo "Failed to cvs update $CVS_CHECKOUT_SCRIPT" | \
#	    mailx -s "$MAIL_SUBJECT" $ERROR_RECIPIENT; 
#	exit 1; 
#    }
#    cd $old_dir
#fi



# If not run as batch then give user a chance to cancel, must be before we
# start ssh otherwise we will leave running processes all over the place.
# After this we trap any attempts to Cntl-C etc.
#
if [ -z $BATCH ] ; then
    message_out "Pausing for $SLEEP seconds, hit Cntl-C to abort cleanly."

    sleep $SLEEP
fi



# trap attempts to kill script to prevent dangling processes on other machines.
#
trap '' INT
trap '' TERM
trap '' QUIT



echo "Now running and cannot be cleanly aborted..."
echo


# A one step copy, run, cleanup!
# The /bin/kill -9 -$$; line is there to make sure no processes are left behind if the
# root_checkout.sh looses one... In testing, but appears kill returns non-zero in $?
# actually it's probably the bash process returning the non-zero, but the next test
# appears to succeed if we enter _rm_exit(), which is what we want. 
# We use ssh -x so that during testing FowardX11=no is forced so that zmap_test_suite.sh
# doesn't try to use the forwarded X11 connection, which will be the user's own not zmap's
cat $CVS_CHECKOUT_SCRIPT | ssh zmap@$SRC_MACHINE '/bin/bash -c "\
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
./root_checkout.sh '$CMD_OPTIONS' || _rm_exit; \
:                                     ; \
rm -f root_checkout.sh || exit 1;   \
"' > $GLOBAL_LOG 2>&1


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


echo $MAIL_SUBJECT
echo


exit $RC
# ================== END OF SCRIPT ================== 
