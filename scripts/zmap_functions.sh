#!/bin/echo DOT SCRIPT please source

# A few absolute requirements for non-failure
if [ "x$SCRIPT_NAME" == "x" ]; then
    SCRIPT_NAME=$(basename $0)
fi

if [ "x$ECHO" == "x" ]; then
    ECHO=/bin/echo
fi

# message functions

# Usage: zmap_message_out Your Message
function zmap_message_out
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*"
}

# Usage: zmap_message_err Your Message
function zmap_message_err
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*" >&2
}

# Usage: zmap_message_exit Your Message
function zmap_message_exit
{
    zmap_message_err $*
    exit 1;
}

function zmap_message_file
{
    if [ "x" != "x$1" ]; then
	local file=$1
	shift
	$ECHO "[$SCRIPT_NAME ($now)] $*" >> $file
    fi
}

function zmap_create_file
{
    if [ "x" != "x$1" ]; then
	$ECHO -n "" > $1
    fi
}

# last absolute requirements

if [ "x$BASE_DIR" == "x" ]; then
    zmap_message_exit "$0 needs to set BASE_DIR!"
fi

function zmap_check
{
    $ECHO > /dev/null
}

# common variables
ZMAP_TRUE=yes
ZMAP_FALSE=no
# default assignments...
zmap_check ${RM:=rm}
zmap_check ${MKDIR:=mkdir}


# common functions

# Usage: zmap_cd <dir>
function zmap_cd
{
    if [ "x$SILENT_CD" != "x$ZMAP_TRUE" ]; then
	zmap_message_out "cd $1"
    fi

    cd $1 || zmap_message_exit "Failed to cd to $1"

    if [ "x$SILENT_CD" != "x$ZMAP_TRUE" ]; then
	zmap_message_out "Now in $(pwd)"
    fi
}

function zmap_mkdir
{
    $MKDIR -p $1 || zmap_message_exit "Failed to mkdir $1"
}

function zmap_clean_dir
{
    if [ -d $1 ]; then
	save_dir=$(pwd)
	zmap_cd $1
	zmap_message_out "Running $RM -rf ./*"
	$RM -rf ./* || zmap_message_exit "Failed to remove files in $1"
	zmap_cd $save_dir
    fi
}

# Usage: zmap_pathmunge <dir> [prepend]
function zmap_pathmunge
{
    # check for not existing
    if ! echo $PATH | /bin/egrep -q "(^|:)$1($|:)" ; then
        if [ "$2" = "prepend" ] ; then
            PATH=$1:$PATH
        else
            PATH=$PATH:$1
        fi
    fi
}

# Usage: zmap_goto_cvs_module_root
function zmap_goto_cvs_module_root
{
    if [ -f CVS/Repository ]; then 
	LOCATION=$(cat CVS/Repository)
	current=$(pwd)
	zmap_message_out "Navigating out of $LOCATION"
	while [ "x$LOCATION" != "x." ];
	  do
	  CVS_MODULE=$(basename $LOCATION)
	  LOCATION=$(dirname $LOCATION)
	  zmap_message_out "Leaving $CVS_MODULE."
	  CVS_MODULE_LOCAL=$(basename $current)
	  current=$(dirname $current)
	  zmap_cd $current
	done
	zmap_cd $CVS_MODULE_LOCAL
    else
	zmap_message_exit "Failed to find Repository file in CVS directory."
    fi
}

# Usage: zmap_trap_handle
function zmap_trap_handle
{
    zmap_message_exit "Signal caught!";
}

trap 'zmap_trap_handle;' INT
trap 'zmap_trap_handle;' TERM
trap 'zmap_trap_handle;' QUIT
