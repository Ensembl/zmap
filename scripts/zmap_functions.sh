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
    $ECHO "[$SCRIPT_NAME ($now)] $@"
}

# Usage: zmap_message_err Your Message
function zmap_message_err
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $@" >&2
    
}

# Usage: zmap_message_exit Your Message
function zmap_message_exit
{
    zmap_message_err "$@"
    exit 1;
}

# Usage: zmap_here_message_exit <<EOF
# Your Message
# EOF
function zmap_here_message_exit
{
    IFS='.'
    while read -t1 line;
      do
      $ECHO "$line" >&2
    done;
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

# Usage: zmap_tar_dir <directory>
function zmap_tar_dir
{
    if [ "x$1" != "x" ]; then
	restore_dir=$(pwd)
	parent_dir=$(dirname $1)
	tar_name=$(basename $1)
	
	zmap_cd $parent_dir
	
	tar -zcf$tar_name.tar.gz $tar_name || zmap_message_exit "Failed to tar $tar_name directory in $parent_dir"

	zmap_cd $tar_name
	rm -rf *
	cd ..
	rmdir $tar_name

	zmap_cd $restore_dir
    else
	zmap_message_exit "Usage: zmap_tar_dir <directory>"
    fi
}

# Usage: zmap_tar_old_releases <RELEASES_DIR>
function zmap_tar_old_releases
{
    if [ "x$1" != "x" ]; then
	local releases_dir=$1
	previous_releases=$(find $releases_dir -type d -maxdepth 1 -name 'ZMap*BUILD')
	
	for prev_release in $previous_releases;
	  do
	  zmap_tar_dir $prev_release
	done
    fi
}

# Usage: zmap_delete_ancient_tars <RELEASES_DIR>
function zmap_delete_ancient_tars
{
    if [ "x$1" != "x" ]; then
	local releases_dir=$1
	restore_dir=$(pwd)
	
	zmap_cd $releases_dir
	
	# ls -t below requires that they are all in time order,
	# which of course they should be, but...
	touch_files_for_time_order=""
	if [ "x$touch_files_for_time_order" != "x" ]; then
	    all_releases=$(ls ZMap*BUILD.tar.gz)
	    for file in $all_releases; 
	      do 
	      zmap_message_out "Touching file '$file'"; 
	      sleep 10; 
	      touch $file; 
	    done
	fi

	all_releases=$(ls -t ZMap*BUILD.tar.gz)
	let counter=1;
	for release in $all_releases;
	  do
	  let counter=$counter+1
	  if [ $counter -gt 12 ]; then
	      zmap_message_out "Removing ancient release $release"
	      # When I'm confident it works...
	      rm -f $release
	  else
	      zmap_message_out "Keeping release $release"
	  fi
	done

	zmap_cd $restore_dir
    fi
}

# Usage: zmap_scp_path_to_host_path <scp-path>
function zmap_scp_path_to_host_path
{
    if [ "x$1" != "x" ]; then
	scp_path=$1
	local IFS=::
	for p in $scp_path 
	  do
	  if [ "x$TAR_TARGET_HOST" == "x" ]; then 
	      TAR_TARGET_HOST=$p
	  elif [ "x$TAR_TARGET_PATH" == "x" ]; then
	      TAR_TARGET_PATH=$p
	  fi
	done

	[ "x$TAR_TARGET_HOST" != "x" ] || zmap_message_exit "No Host set."
	[ "x$TAR_TARGET_PATH" != "x" ] || zmap_message_exit "No Path set."

    else
	zmap_message_exit "Usage: zmap_scp_path_to_host_path <scp-path>"
    fi
}

# We need  to be able to login  to a cluster machine  using the alias,
# but later access that specific node.  These functions allow that.

# Usage: zmap_write_cluster_config
function zmap_write_cluster_config
{
    if [ ! -f $ZMAP_CLUSTER_CONFIG_FILE ]; then
	if [ "x$ZMAP_BUILD_MACHINES" != "x" ]; then
	    HOSTNAMES=""
	    for cluster in $ZMAP_BUILD_MACHINES ; do
		TMP=$(ssh $ZMAP_SSH_OPTIONS $cluster 'hostname' 2>/dev/null)
		HOSTNAMES="$HOSTNAMES $TMP"
	    done
	    ZMAP_BUILD_MACHINES=$HOSTNAMES
	fi
	echo "ZMAP_BUILD_MACHINES=\"$ZMAP_BUILD_MACHINES\"" > $ZMAP_CLUSTER_CONFIG_FILE
    fi
}

# Usage: zmap_read_cluster_config
function zmap_read_cluster_config
{
    if [ -f $ZMAP_CLUSTER_CONFIG_FILE ]; then 
	. $ZMAP_CLUSTER_CONFIG_FILE || zmap_message_err "Failed reading $ZMAP_CLUSTER_CONFIG_FILE"
    else
	zmap_message_err "$ZMAP_CLUSTER_CONFIG_FILE does not exist"
    fi
}

# Usage: zmap_remove_cluster_config
function zmap_remove_cluster_config
{
    if [ -f $ZMAP_CLUSTER_CONFIG_FILE ]; then 
	rm -f $ZMAP_CLUSTER_CONFIG_FILE
    fi
}

# zmap_edit_variable_add &  zmap_edit_variable_del are cover functions
# for  zmap_edit_variable  which  adds  or  removes a  string  to  the
# variable value as if the value is a space separated array.
# e.g. a list of files that must be removed.

# Usage: zmap_edit_variable_add VariableName VariableValue
function zmap_edit_variable_add
{
    zmap_edit_variable $1 add $2
}

# Usage: zmap_edit_variable_del VariableName VariableValue
function zmap_edit_variable_del
{
    zmap_edit_variable $1 remove $2
}

# Usage: zmap_edit_variable VariableName [add|remove] VariableValue
function zmap_edit_variable
{
    name=$1
    add_remove=$2
    value=$3

    eval "VARIABLE=\${$name}"
    eval "VARIABLE_OUT="

    #echo "$name=$VARIABLE"

    for part in $VARIABLE
      do

      if [ "x$part" != "x$value" ]; then
	  if [ "x$VARIABLE_OUT" != "x" ]; then
	      VARIABLE_OUT="$VARIABLE_OUT $part"
	  else
	      VARIABLE_OUT="$part"
	  fi
      fi

    done

    if [ "x$add_remove" == "xadd" ]; then
	if [ "x$VARIABLE_OUT" != "x" ]; then
	    VARIABLE_OUT="$VARIABLE_OUT $value"
	else
	    VARIABLE_OUT="$value"
	fi    
    fi

    #echo "variable out = '$VARIABLE_OUT'"

    eval $name='$VARIABLE_OUT'
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
