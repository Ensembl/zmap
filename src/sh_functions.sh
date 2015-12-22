#!/bin/echo DOT SCRIPT please source
#
# Functions common to all our build scripts....
#



# A few absolute requirements for non-failure
if [ "x$SCRIPT_NAME" == "x" ]; then
    SCRIPT_NAME=$(basename $0)
fi

if [ "x$ECHO" == "x" ]; then
    ECHO=/bin/echo
fi

if [ "x$zmap_host_name" == "x" ]; then
    zmap_host_name=$(hostname)
fi



# message functions

# Make a prefix.
# Usage your_var=$(zmap_make_prefix) 
function zmap_make_prefix
{
    now=$(date +%H:%M:%S)

    $ECHO "[$zmap_host_name $SCRIPT_NAME ($now)] $@"
}



# Usage: zmap_message_out Your Message
function zmap_message_out
{
    $ECHO "$(zmap_make_prefix) $@"
}

# Usage: zmap_message_exit Your Message
function zmap_message_exit
{
    zmap_message_err "$@"
    exit 1;
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



# common variables
ZMAP_TRUE=yes
ZMAP_FALSE=no




