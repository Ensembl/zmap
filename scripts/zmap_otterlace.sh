#!/bin/sh
#
# You need to run this script from this directory or set BASE_DIR
# to point to the zmap scripts directory.
#
# You will probably want to run this script something like this:
#
# ./zmap_otterlace.sh ZMAP_BIN_DIR=~/ZMap/ZMap_View/ZMap/src/build/linux TK_OTTER_HOME=~/ZMap/otter
#
#

SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
    BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
    BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }


zmap_message_out "Start of script."


# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi

# otter_root is immutable
otter_root="/software/anacode/otter"

zmap_check ${RELEASE:="otter_production_main"}
zmap_check ${OTTER_HOME:="$otter_root/$RELEASE"}
zmap_check ${ZMAP_HOME:="$HOME"}
zmap_check ${ZMAP_BIN_DIR:="$OTTER_HOME/bin"}
zmap_check ${TK_OTTER_HOME:=$OTTER_HOME}
zmap_check ${PERL_HOME:=$OTTER_HOME}
zmap_check ${ACEDB_HOME:="/nfs/disk100/acedb/RELEASE.DEVELOPMENT/bin.LINUX_4"}

export OTTER_HOME ZMAP_HOME

zmap_message_out "Using OTTER_HOME '$OTTER_HOME'"
zmap_message_out "Using TK_OTTER_HOME '$TK_OTTER_HOME'"
zmap_message_out "Using ZMAP_HOME '$ZMAP_HOME'"
zmap_message_out "Using ZMAP_BIN_DIR '$ZMAP_BIN_DIR'"


otterlib="$OTTER_HOME/lib"
if [ -n "$LD_LIBRARY_PATH" ]
then
    LD_LIBRARY_PATH="$otterlib:$LD_LIBRARY_PATH"
else
    LD_LIBRARY_PATH="$otterlib"
fi
export LD_LIBRARY_PATH

otterbin="$ZMAP_BIN_DIR:$OTTER_HOME/bin:/software/anacode/bin:/software/pubseq/bin/EMBOSS-5.0.0/bin"
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

#cp -f "$OTTER_HOME/acedbrc" ~/.acedbrc

PERL5LIB="\
$TK_OTTER_HOME/ensembl-otter/tk:\
$OTTER_HOME/PerlModules:\
$OTTER_HOME/ensembl-ace:\
$OTTER_HOME/ensembl-otter/modules:\
$OTTER_HOME/ensembl-pipeline/modules:\
$OTTER_HOME/ensembl/modules:\
$OTTER_HOME/ensembl_head/modules:\
$OTTER_HOME/bioperl-0.7.2:\
$OTTER_HOME/bioperl-1.2.3-patched:\
$OTTER_HOME/biodas-1.02:\
$PERL_HOME/lib/site_perl"

export PERL5LIB

exec /software/bin/perl "$OTTER_HOME/ensembl-otter/tk/otterlace" $@

