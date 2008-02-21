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
    if ! echo $PATH | egrep -q "(^|:)$1($|:)" ; then
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

# Usage: zmap_dump_environment <filename>
function zmap_dump_environment
{
    local dump_file=$1
    [ "x$dump_file" != "x" ] || dump_file=zmap.env.log
    set > $dump_file
}

# prepend PREFIX to all Variables required.
# Usage: zmap_register_prefix_in_env <PREFIX>
function zmap_register_prefix_in_env
{
    local prefix=$1
    [ "x$prefix" != "x" ] || prefix=/non-existent
    if [ -d $prefix ]; then

	# PATH first
	zmap_pathmunge $prefix/bin prepend

	# PKG_CONFIG (only if not set!)
	if [ "x$PKG_CONFIG" == "x" ]; then
	    PKG_CONFIG=$prefix/bin/pkg-config
	fi

	# PKG_CONFIG_PATH
       	if [ "x$PKG_CONFIG_PATH" != "x" ]; then
	    PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"
	else
	    PKG_CONFIG_PATH="$prefix/lib/pkgconfig"
	fi

	# LD_LIBRARY_PATH
	if [ "x$LD_LIBRARY_PATH" != "x" ]; then
	    LD_LIBRARY_PATH="$prefix/lib:$LD_LIBRARY_PATH"
	else
	    LD_LIBRARY_PATH="$prefix/lib"
	fi

	# LDFLAGS
	if [ "x$LDFLAGS" != "x" ]; then
	    LDFLAGS="-L$prefix/lib $LDFLAGS"
	else
	    LDFLAGS="-L$prefix/lib"
	fi

	if [ "x$LD_RUN_PATH" != "x" ]; then
	    LD_RUN_PATH="$prefix/lib:$LD_RUN_PATH"
	else
	    LD_RUN_PATH=$prefix/lib
	fi

	# CPPFLAGS
	if [ "x$CPPFLAGS" != "x" ]; then
	    CPPFLAGS="-I$prefix/include $CPPFLAGS"
	else
	    CPPFLAGS="-I$prefix/include"
	fi

	export PKG_CONFIG
	export PKG_CONFIG_PATH
	export LD_LIBRARY_PATH
	export LDFLAGS
	export CPPFLAGS

    else
	zmap_message_err "$prefix doesn't exist. Are you sure?"
    fi
}

function zmap_register_prefix_as_rpath
{
    local prefix=$1
    [ "x$prefix" != "x" ] || prefix=/non-existent
    if [ -d $prefix ]; then

	# LDFLAGS
	if [ "x$LDFLAGS" != "x" ]; then
	    LDFLAGS="-Xlinker -rpath -Xlinker $prefix/lib $LDFLAGS"
	else
	    LDFLAGS="-Xlinker -rpath -Xlinker $prefix/lib"
	fi

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



# This should be the last thing.
set -o history
