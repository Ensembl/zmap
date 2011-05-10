#!/bin/bash

# ================= README ==================

# Build zmap for all architectures specified in $ZMAP_BUILD_MACHINES.
#
# Builds/logs are in ONE_OFF.BUILD file/directory.
#
# Main Build Parameters:
#
# Version incremented        yes
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
CVS_CHECKOUT_SCRIPT=$BASE_DIR/prefix/scripts/build_bootstrap.sh

# Output place for build
BUILDS_DIR=$BASE_DIR/BUILDS
BUILD_PREFIX='RELEASE.BUILD'

# GLOBAL_LOG= The place to hold the log file
GLOBAL_LOG=$BUILDS_DIR/$BUILD_PREFIX.LOG

# ERROR_RECIPIENT= Someone to email
ERROR_RECIPIENT=zmapdev@sanger.ac.uk

# ENSURE_UP_TO_DATE= cvs update the directory where $CVS_CHECKOUT_SCRIPT is [ yes | no ]
ENSURE_UP_TO_DATE=yes

# OUTPUT dir
OUTPUT=$BUILDS_DIR/$BUILD_PREFIX

# Give user a chance to cancel.
SLEEP=15




# ================== MAIN PART ================== 

MAIL_SUBJECT="ZMap Build Failed (control script)"

if ! echo $GLOBAL_LOG | egrep -q "(^)/" ; then
    GLOBAL_LOG=$(pwd)/$GLOBAL_LOG
fi

mkdir -p $OUTPUT
rm -f $GLOBAL_LOG



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


# ================== RUN INFO ================== 
cat <<EOF
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Welcome to the '$0' build script

We are running on '$(hostname)' and are going to run the 
bootstrap build script '$CVS_CHECKOUT_SCRIPT'
on '$SRC_MACHINE' (via ssh).

The global output log is '$GLOBAL_LOG'

Errors will be reported to '$ERROR_RECIPIENT'

EOF


# If we need to update from cvs...
if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
    old_dir=$(pwd)
    new_dir=$(dirname  $CVS_CHECKOUT_SCRIPT)
    up2date=$(basename $CVS_CHECKOUT_SCRIPT)
    cd $new_dir
    export CVS_RSH=ssh
    cat <<EOF
Now we are going to cvs update '$CVS_CHECKOUT_SCRIPT' in $(pwd)

EOF
    cvs update -C $up2date || { echo "Failed to update $CVS_CHECKOUT_SCRIPT"; exit 1; }
    cd $old_dir
    echo "cvs updated."
fi

[ -f $CVS_CHECKOUT_SCRIPT ] || { echo "Failed to find $CVS_CHECKOUT_SCRIPT"; exit 1; }


cat <<EOF

After a short pause ($SLEEP seconds), we will begin.

First some questions:

Does password-less ssh login work?

Have you committed all your code?

Have you tested it?


[last opportunity to hit Ctrl-C]...

EOF


sleep $SLEEP

echo "Now running..."
# A one step copy, run, cleanup!
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
./root_checkout.sh -d -t -u || _rm_exit; \
:                                     ; \
rm -f root_checkout.sh  RELEASE_LOCATION='$OUTPUT' ZMAP_MASTER_RT_RELEASE_NOTES=yes ZMAP_MASTER_FORCE_RELEASE_NOTES=yes  ZMAP_MASTER_BUILD_DIST=yes  || exit 1;   \
"' > $GLOBAL_LOG 2>&1


# ================== ERROR HANDLING ================== 

# whatever is supposed to happen here doesn't work even if the build
# fails we get the succeeded message.

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
	cat $TMP_LOG | mailx -s "ZMap $BUILD_PREFIX Failed (control script)" $ERROR_RECIPIENT
    fi
    rm -f $TMP_LOG

    RC=1
else
    if [ "x$ERROR_RECIPIENT" != "x" ]; then
	tail $GLOBAL_LOG | mailx -s "ZMap $BUILD_PREFIX Succeeded" $ERROR_RECIPIENT
    fi
fi


exit $RC
# ================== END OF SCRIPT ================== 
