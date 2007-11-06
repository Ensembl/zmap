#!/bin/sh
############################################################
# Script to test zmap with otterlace

# Change these few variables to point to the correct places

# This should be the current live code for otter.
# Try ls -lt /software/anacode/otter
RELEASE=otter_rel47_zmap

# Change the location of this to be where your zmap to test is.
ZMAP_TO_TEST=""
ZMAP_TO_TEST=/var/tmp/rds/ZMap/src/build/linux

############################################################
# Nothing much to change below here...

SOFTWARE=/software/anacode
OTTER_CONFIG=$HOME/.otter_config
OTTER_HOME=$SOFTWARE/otter/$RELEASE
export OTTER_HOME

if [ ! -d $OTTER_HOME ]; then
    echo
    echo "This executable is not available at the moment."
    echo "Please try the non-test version."
    echo
    exit 1
fi

script='otterlace'

#. $SOFTWARE/bin/setup_anacode_env

# This should be in setup_env
################## START OF SETUP_ENV ######################
PERL5LIB="\
$OTTER_HOME/tk:\
$OTTER_HOME/PerlModules:\
$OTTER_HOME/ensembl-ace:\
$OTTER_HOME/ensembl-otter/modules:\
$OTTER_HOME/ensembl-pipeline/modules:\
$OTTER_HOME/ensembl/modules:\
$OTTER_HOME/ensembl_head/modules:\
$OTTER_HOME/bioperl-0.7.2:\
$OTTER_HOME/bioperl-1.2.3-patched:\
$OTTER_HOME/biodas-1.02:\
$OTTER_HOME/perl"
export PERL5LIB

otterlib="$OTTER_HOME/lib"
if [ -n "$LD_LIBRARY_PATH" ]
then
    LD_LIBRARY_PATH="$otterlib:$LD_LIBRARY_PATH"
else
    LD_LIBRARY_PATH="$otterlib"
fi
export LD_LIBRARY_PATH

otterbin="$OTTER_HOME/bin"
if [ -n "$PATH" ]
then
    PATH="$otterbin:$PATH"
else
    PATH="$otterbin"
fi
export PATH

#PFETCH_SERVER_LIST=localhost:22400
#export PFETCH_SERVER_LIST

# Settings for wublast needed by local blast searches
WUBLASTFILTER=/software/anacode/bin/wublast/filter
export WUBLASTFILTER
WUBLASTMAT=/software/anacode/bin/wublast/matrix
export WUBLASTMAT

# Some setup for acedb
ACEDB_NO_BANNER=1
export ACEDB_NO_BANNER

#################### END OF SETUP_ENV #######################

# Now _prepend_ our path to the PATH setup by above.

#emboss_prefix=/nfs/team71/analysis/rds/emboss

if [ "x$emboss_prefix" != "x" ]; then
    PATH="$emboss_prefix/bin:$PATH"
    if [ "x$LD_LIBRARY_PATH" == "x" ]; then
	LD_LIBRARY_PATH="$emboss_prefix/lib"
    else
	LD_LIBRARY_PATH="$emboss_prefix/lib:$LD_LIBRARY_PATH"
    fi
fi

if [ "x$ZMAP_TO_TEST" != "x" ]; then
    PATH="$ZMAP_TO_TEST:$PATH"
fi


enable_zmap()
{
    echo "[client]"           >> $OTTER_CONFIG
    echo "show_zmap=enable"   >> $OTTER_CONFIG
    echo "zmap_main_window=1" >> $OTTER_CONFIG
}

if [ -f $OTTER_CONFIG ]; then
    # If it already exists
    SHOW_ZMAP=`grep show_zmap=enable $OTTER_CONFIG`
    if [ "x$SHOW_ZMAP" == "x" ]; then
	enable_zmap
    fi
else
	enable_zmap
fi

exec /software/bin/perl -w "$OTTER_HOME/tk/$script" $@
