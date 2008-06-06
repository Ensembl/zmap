#!/bin/bash

# zmap_do_build.sh

#trap '' INT
#trap '' TERM
#trap '' QUIT

# Configuring variables

# ================== CONFIG ================== 
SRC_MACHINE=tviewsrv1
CVS_CHECKOUT_SCRIPT=./build_bootstrap.sh
GLOBAL_LOG=~/BUILDS/latest.build.log
ERROR_RECIPIENT=zmapdev@sanger.ac.uk
ENSURE_UP_TO_DATE=yes

# For development make sure these are set
#CVS_CHECKOUT_SCRIPT=./build_bootstrap.sh
#GLOBAL_LOG=~/BUILDS/OVERNIGHT.BUILD.LOG
#ERROR_RECIPIENT=
#ENSURE_UP_TO_DATE=no

# ================== MAIN PART ================== 
if ! echo $GLOBAL_LOG | egrep -q "(^)/" ; then
    GLOBAL_LOG=$(pwd)/$GLOBAL_LOG
fi

rm -f $GLOBAL_LOG

if [ "x$ENSURE_UP_TO_DATE" == "xyes" ]; then
    old_dir=$(pwd)
    new_dir=$(dirname  $CVS_CHECKOUT_SCRIPT)
    up2date=$(basename $CVS_CHECKOUT_SCRIPT)
    cd $new_dir
    export CVS_RSH=ssh
    cvs update -C $up2date || { echo "Failed to update $CVS_CHECKOUT_SCRIPT"; exit 1; }
    cd $old_dir
fi

# A one step copy, run, cleanup!
cat $CVS_CHECKOUT_SCRIPT | ssh $SRC_MACHINE '/bin/bash -c "\
function _rm_exit                       \
{                                       \
   echo Master Script Failed...;        \
   rm -f root_checkout.sh;              \
   exit 1;                              \
};                                      \
cd /var/tmp                || exit 1;   \
rm -f root_checkout.sh     || exit 1;   \
cat - > root_checkout.sh   || exit 1;   \
chmod 755 root_checkout.sh || _rm_exit; \
: Change the variables in next line   ; \
./root_checkout.sh -t -u   || _rm_exit; \
:                                     ; \
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
	cat $TMP_LOG | mailx -s "ZMap Build Failed (control script)" $ERROR_RECIPIENT
    fi
    rm -f $TMP_LOG
else
    if [ "x$ERROR_RECIPIENT" != "x" ]; then
	tail $GLOBAL_LOG | mailx -s "ZMap Build Succeeded" $ERROR_RECIPIENT
    fi
fi

# ================== END OF SCRIPT ================== 
