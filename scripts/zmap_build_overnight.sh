#!/bin/bash

# ================= README ==================

# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# This script is run _every_ night as a cron job so be careful
# when altering it.
#
# Builds/logs are in OVERNIGHT.BUILD file/directory.
#
# Main Build Parameters:
#
# Version incremented        no
#        Docs created        no
#     Docs checked in        no
#  



trap '' INT
trap '' TERM
trap '' QUIT


RC=0

# ================== CONFIG ================== 
# Configuration variables

# where everything is located.
BASE_DIR=~zmap

# SRC_MACHINE= The machine to log into to start everything going
SRC_MACHINE=tviewsrv

# CVS_CHECKOUT_SCRIPT= The bootstrapping script that starts everything
CVS_CHECKOUT_SCRIPT=$BASE_DIR/BUILD_SCRIPTS/ZMap/scripts/build_bootstrap.sh

# Output place for build
BUILDS_DIR=$BASE_DIR/BUILDS
BUILD_PREFIX='OVERNIGHT_BUILD'

# GLOBAL_LOG= The place to hold the log file
GLOBAL_LOG=$BUILDS_DIR/$BUILD_PREFIX.LOG

# ERROR_RECIPIENT= Someone to email
ERROR_RECIPIENT=zmapdev@sanger.ac.uk

# ENSURE_UP_TO_DATE= cvs update the directory where $CVS_CHECKOUT_SCRIPT is [ yes | no ]
ENSURE_UP_TO_DATE=yes

# OUTPUT dir
OUTPUT=$BUILDS_DIR/$BUILD_PREFIX




# ================== MAIN PART ==================

MAIL_SUBJECT="ZMap $BUILD_PREFIX Failed (control script)"

if ! echo $GLOBAL_LOG | egrep -q "(^)/" ; then
    GLOBAL_LOG=$(pwd)/$GLOBAL_LOG
fi

mkdir -p $OUTPUT
rm -f $GLOBAL_LOG


# DISABLED...NOT REALLY CORRECT ANYWAY..........
#
# Errors before here only end up in stdout/stderr
# Errors after here should be mailed to $ERROR_RECIPIENT
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


# A one step copy, run, cleanup!
# The /bin/kill -9 -$$; line is there to make sure no processes are left behind if the
# root_checkout.sh looses one... In testing, but appears kill returns non-zero in $?
# actually it's probably the bash process returning the non-zero, but the next test
# appears to succeed if we enter _rm_exit(), which is what we want. 
# We use ssh -x so that during testing FowardX11=no is forced so that zmap_test_suite.sh
# doesn't try to use the forwarded X11 connection, which will be the user's own not zmap's
cat $CVS_CHECKOUT_SCRIPT | ssh -x $SRC_MACHINE '/bin/bash -c "\
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
: Change the variables in next line             ; \
./root_checkout.sh RELEASE_LOCATION='$OUTPUT' || _rm_exit; \
:                                               ; \
rm -f root_checkout.sh     || exit 1;   \
"' > $GLOBAL_LOG 2>&1


# ================== ERROR HANDLING ================== 

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

exit $RC
# ================== END OF SCRIPT ================== 
