#!/bin/echo dot-script source me, don't run me

if [ "x$BASE_DIR" == "x" ]; then
    /bin/echo "$0 needs to set BASE_DIR!" >&2
    exit 1;
fi

if [ "x$SCRIPT_NAME" == "x" ]; then
    SCRIPT_NAME=$(basename $0)
fi

# Usage: build_message_out Your Message
function build_message_out
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*"
}

# Usage: build_message_err Your Message
function build_message_err
{
    now=$(date +%H:%M:%S)
    $ECHO "[$SCRIPT_NAME ($now)] $*" >&2
}

# Usage: build_message_exit Your Message
function build_message_exit
{
    build_message_err $*
    if [ -f $BUILD_EXECUTE_CONFIG ]; then
	$RM -f $BUILD_EXECUTE_CONFIG
    fi
    exit 1;
}

# Usage: build_cd <directory>
function build_cd
{
    if [ "x$SILENT_CD" != "xyes" ]; then
	build_message_out "cd $1"
    fi
    cd $1 || build_message_exit "Failed to cd to $1"
}

# Usage: build_curl_get_file <url> <output-file>
function build_curl_get_file
{
    if [ "x$1" != "x" ]; then
	url=$1
	proxy=""
	outfile=""
	clean_cmd=""

	if [ "x$HTTP_PROXY" != "x" ]; then
	    proxy="-x $HTTP_PROXY"
	elif [ "x$http_proxy" != "x" ]; then
	    proxy="-x $http_proxy"
	fi

	if [ "x$2" != "x" ]; then
	    if [ "x$2" != "x-" ]; then
		outfile="-o $2"
		clean_cmd="$RM -f $2"
	    fi
	fi

	$CURL $proxy $outfile $url

	if [ $? != 0 ]; then
	    $clean_cmd
	    build_message_exit "Failed to download $package from $url"
	fi
    else
	build_message_out "Usage: build_curl_get_file <url> <output-file>"
    fi
}

# Usage: build_wget_get_file <url> <output-file>
function build_wget_get_file
{
    if [ "x$1" != "x" ]; then
	url=$1
	if [ "x$2" != "x" ]; then
	    package=$2
	    clean_cmd="$RM -f $package"
	    options=""

	    if [ "x$2" == "x-" ]; then
		clean_cmd=""
		options="-q"
	    fi

	    $WGET $options -O $package $url

	    if [ $? != 0 ]; then
		$clean_cmd
		build_message_exit "Failed to download $package from $url"    
	    fi
	else
	    build_message_out "Usage: build_wget_get_file <url> <output-file>"	
	fi
    else
	build_message_out "Usage: build_wget_get_file <url> <output-file>"	
    fi
}

# Usage: build_download_file <url> <file>
function build_download_file
{
    restore_dir=$(pwd)
    build_cd $BASE_DIR/$DIST_DIR

    if [ "x$1" != "x" ]; then
	url=$1


	if [ "x$2" != "x" ]; then
	    package=$2
	else
	    package=$(basename $url)
	fi

	if [ "x$package" == "x-" ]; then
	    if [ "x$USE_WGET" == "xyes" ]; then
		build_wget_get_file $url $package
	    else
		build_curl_get_file $url $package
	    fi
	elif [ ! -f $package ]; then
	    build_message_out "Downloading $package from $url"

	    if [ "x$USE_WGET" == "xyes" ]; then
		build_wget_get_file $url $package
	    else
		build_curl_get_file $url $package
	    fi

	    if [ ! -f $package ]; then
		build_message_exit "Failed to download $package"
	    else
		build_message_out "Download complete"
	    fi
	else
	    build_message_out "$package already downloaded"
	fi
    fi
    build_cd $restore_dir
}

# Usage: build_untar_file <tarfile.tar.gz> <output_dir>
function build_untar_file
{
    untar_tmp="build_untar_tmp"
    restore_dir=$(pwd)
    build_cd $BASE_DIR/$BUILD_DIR

    if [ "x$1" != "x" ]; then
	package=$1
	output_dir=$package

	$MKDIR $untar_tmp
	build_cd $untar_tmp

	if [ "x$2" != "x" ]; then
	    output_dir=$2
	fi

	output_dir=$BASE_DIR/$BUILD_DIR/$output_dir

	if [ "x$LEAVE_PREVIOUS_BUILD" != "xyes" ]; then
	    if [ -d $output_dir ]; then
		build_message_out "$output_dir already exists... Removing"
		$RM -rf $output_dir
	    fi
	fi

	tar_options="-zxf"
	tmp=$(basename $package .tar.gz)
	if [ "$tmp" != "$package" ]; then
	    tar_options="-zxf"
	fi
	tmp=$(basename $package .tgz)
	if [ "$tmp" != "$package" ]; then
	    tar_options="-zxf"
	fi
	tmp=$(basename $package .tar.bz2)
	if [ "$tmp" != "$package" ]; then
	    tar_options="-jxf"
	fi

	build_message_out "Untarring $package to $output_dir"
	$GTAR $tar_options $BASE_DIR/$DIST_DIR/$package

	if [ $? != 0 ]; then
	    build_cd $BASE_DIR/$BUILD_DIR
	    $RM -rf $untar_tmp
	    build_message_exit "Failed to untar $package"
	else
	    package_dir=$(ls)
	    if [ ! -d $output_dir ]; then
		mv -f $package_dir $output_dir
	    else
		cp -r $package_dir/* $output_dir/
	    fi
	    build_cd $BASE_DIR/$BUILD_DIR
	    $RM -rf $untar_tmp
	fi
	build_message_out "$package untarred to $output_dir"
    fi

    build_cd $restore_dir
}

# Usage: build_build_package <package_dir> <package_name>
function build_build_package
{
    restore_dir=$(pwd)

    if [ "x$1" != "x" ]; then
	if [ "x$2" != "x" ]; then
	    package=$1
	    temp="PACKAGE_${2}_CONFIGURE_OPTS"
	    eval "PACKAGE_CONFIGURE_OPTS=\${$temp}"

	    temp="PACKAGE_${2}_POSTCONFIGURE"
	    eval "PACKAGE_POSTCONFIGURE=\${$temp}"
	    
	    build_cd $BASE_DIR/$BUILD_DIR/$package

	    if [ "x$CLEAN_PACKAGE_DIR" == "xyes" ]; then
		build_message_out "make distclean in $package directory"
		if [ -f Makefile ]; then
		    $MAKE distclean || build_message_err "$MAKE distclean failed."
		else
		    build_message_out "No Makefile exists, not running $MAKE distclean"
		fi
	    fi

	    # The configure step
	    build_message_out "Running " $CONFIGURE $CONFIGURE_OPTS $PACKAGE_CONFIGURE_OPTS
	    $CONFIGURE $CONFIGURE_OPTS $PACKAGE_CONFIGURE_OPTS || build_message_exit "Failed $CONFIGURE"

	    # some packages need a post configure fix up
	    if [ "x$PACKAGE_POSTCONFIGURE" != "x" ]; then
		$PACKAGE_POSTCONFIGURE || build_message_exit "Post Configure for $package failed"
	    else
		build_run_post_patch $package || build_mesage_exit "build_run_post_patch failed"
	    fi
	    
	    # run make
	    $MAKE || build_message_exit "Failed $MAKE"

	    # run make install
	    $MAKE_INSTALL || build_message_exit "Failed $MAKE_INSTALL"
	else
	    build_message_err "Usage: build_build_package <package_dir> <package_name>"
	fi
    else
	build_message_err "Usage: build_build_package <package_dir> <package_name>"
    fi
    build_cd $restore_dir
}

# Usage: build_clean_dist_dir
function build_clean_dist_dir
{
    restore_dir=$(pwd)
    build_message_out "cleaning dist dir"
    build_cd $BASE_DIR/$DIST_DIR
    $RM -rf -- *
    build_cd $restore_dir
}

# Usage: build_clean_build_dir
function build_clean_build_dir
{
    restore_dir=$(pwd)
    build_message_out "cleaning build dir"
    build_cd $BASE_DIR/$BUILD_DIR
    $RM -rf -- *
    build_cd $restore_dir
}

function build_ungtar_file
{
    build_message_exit "Needs writing!"
}

# Usage: build_set_vars_for_prefix
function build_set_vars_for_prefix
{
    if [ "x$PREFIX" != "x" ]; then

	PATH="$PREFIX/bin:$PATH"

	if [ "x$PKG_CONFIG_PATH" != "x" ]; then
	    PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
	else
	    PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig"
	fi

	if [ "x$CONFIGURE_OPTS" != "x" ]; then
	    CONFIGURE_OPTS="--prefix=$PREFIX $CONFIGURE_OPTS"
	else
	    CONFIGURE_OPTS="--prefix=$PREFIX"
	fi

	if [ "x$LD_LIBRARY_PATH" != "x" ]; then
	    LD_LIBRARY_PATH="$PREFIX/lib:$LD_LIBRARY_PATH"
	else
	    LD_LIBRARY_PATH="$PREFIX/lib"
	fi

	if [ "x$LDFLAGS" != "x" ]; then
	    LDFLAGS="-L$PREFIX/lib $LDFLAGS"
	else
	    LDFLAGS="-L$PREFIX/lib"
	fi
	if [ "x$CPPFLAGS" != "x" ]; then
	    CPPFLAGS="-I$PREFIX/include $CPPFLAGS"
	else
	    CPPFLAGS="-I$PREFIX/include"
	fi

	if [ "x$PKG_CONFIG" = "x" ]; then
	    PKG_CONFIG=$PREFIX/bin/pkg-config
	fi

	export LDFLAGS
	export CPPFLAGS
	export LIBTOOLIZE=$PREFIX/bin/libtoolize
	export PATH
	export PKG_CONFIG_PATH
	export PKG_CONFIG
	export LD_LIBRARY_PATH
    fi
}

# Usage: build_create_load_status_file
function build_create_load_status_file
{
    build_message_out "Using $BUILD_STATUS_FILE"
    if [ ! -f $BUILD_STATUS_FILE ]; then
	$ECHO '#!/bin/echo dot-script source me' > $BUILD_STATUS_FILE
	$ECHO '# auto generated do not edit!'   >> $BUILD_STATUS_FILE
    fi
    . $BUILD_STATUS_FILE
}

# Usage: build_save_execution_config
function build_save_execution_config
{
    $RM -f $BUILD_EXECUTE_CONFIG

    if [ ! -f $BUILD_EXECUTE_CONFIG ]; then
	$ECHO '#!/bin/echo dot-script source me' > $BUILD_EXECUTE_CONFIG
	$ECHO '# auto generated do not edit!'   >> $BUILD_EXECUTE_CONFIG
    fi

    $ECHO "PREFIX='$PREFIX'"       >> $BUILD_EXECUTE_CONFIG
    $ECHO "DIST_DIR='$DIST_DIR'"   >> $BUILD_EXECUTE_CONFIG
    $ECHO "BUILD_DIR='$BUILD_DIR'" >> $BUILD_EXECUTE_CONFIG
    $ECHO "PATCH_DIR='$PATCH_DIR'" >> $BUILD_EXECUTE_CONFIG

    $ECHO "SLEEP='$SLEEP'"         >> $BUILD_EXECUTE_CONFIG
    $ECHO "SILENT_CD='$SILENT_CD'" >> $BUILD_EXECUTE_CONFIG
    $ECHO "USE_WGET='$USE_WGET'"   >> $BUILD_EXECUTE_CONFIG

    $ECHO "BUILD_STATUS_FILE='$BUILD_STATUS_FILE'" >> $BUILD_EXECUTE_CONFIG
    
    # useful program
    $ECHO "ECHO='$ECHO'"                 >> $BUILD_EXECUTE_CONFIG
    $ECHO "WGET='$WGET'"                 >> $BUILD_EXECUTE_CONFIG
    $ECHO "CURL='$CURL'"                 >> $BUILD_EXECUTE_CONFIG
    $ECHO "RM='$RM'"                     >> $BUILD_EXECUTE_CONFIG
    $ECHO "GTAR='$GTAR'"                 >> $BUILD_EXECUTE_CONFIG
    $ECHO "MKDIR='$MKDIR'"               >> $BUILD_EXECUTE_CONFIG
    $ECHO "RMDIR='$RMDIR'"               >> $BUILD_EXECUTE_CONFIG
    $ECHO "CONFIGURE='$CONFIGURE'"       >> $BUILD_EXECUTE_CONFIG
    $ECHO "MAKE='$MAKE'"                 >> $BUILD_EXECUTE_CONFIG
    $ECHO "MAKE_INSTALL='$MAKE_INSTALL'" >> $BUILD_EXECUTE_CONFIG
    $ECHO "PKG_CONFIG='$PKG_CONFIG'"     >> $BUILD_EXECUTE_CONFIG
    $ECHO "PATCH='$PATCH'"               >> $BUILD_EXECUTE_CONFIG
    $ECHO "PERL='$PERL'"                 >> $BUILD_EXECUTE_CONFIG
    
    # auto tools programs
    $ECHO "AUTOCONF='$AUTOCONF'"         >> $BUILD_EXECUTE_CONFIG
    $ECHO "AUTOMAKE='$AUTOMAKE'"         >> $BUILD_EXECUTE_CONFIG
    $ECHO "AUTOHEADER='$AUTOHEADER'"     >> $BUILD_EXECUTE_CONFIG
    $ECHO "AUTOUPDATE='$AUTOUPDATE'"     >> $BUILD_EXECUTE_CONFIG
    $ECHO "ACLOCAL='$ACLOCAL'"           >> $BUILD_EXECUTE_CONFIG
    $ECHO "LIBTOOLIZE='$LIBTOOLIZE'"     >> $BUILD_EXECUTE_CONFIG
}

# Usage: build_create_subdirs
function build_create_subdirs
{
    restore_dir=$(pwd)
    build_cd $BASE_DIR
    if [ ! -d $DIST_DIR ]; then
	$MKDIR -p $DIST_DIR
    fi
    if [ ! -d $BUILD_DIR ]; then
	$MKDIR -p $BUILD_DIR
    fi
    if [ ! -d $PATCH_DIR ]; then
	$MKDIR -p $PATCH_DIR
    fi
    cd $restore_dir
}

# Usage: build_run_pre_patch <package-version>
function build_run_pre_patch
{
    if [ "x$1" != "x" ]; then
	package_version=$1
	patch_shell_script=${BASE_DIR}/${PATCH_DIR}/${package_version}.sh
	patch_patch_file=${BASE_DIR}/${PATCH_DIR}/${package_version}.patch

	# patch first...
	if [ -f $patch_patch_file ]; then
	    restore_dir=$(pwd)
	    build_cd $BASE_DIR/$BUILD_DIR/$package_version
	    build_message_out "CWD = $(pwd)"
	    build_message_out "Running $PATCH -p0 -u $patch_patch_file"
	    $PATCH -p0 -s -u < $patch_patch_file \
		|| build_message_exit "Failed to patch $package_version with $patch_patch_file"
	    build_cd $restore_dir
	else
	    build_message_out "No patch file ($patch_patch_file) exists."
	fi

	# If the patch touched the configure script, a bootstrap is needed!
	if [ -x $patch_shell_script ]; then
	    build_message_out "Running $patch_shell_script $BASE_DIR $package_version"
	    $patch_shell_script $BASE_DIR $package_version \
		|| build_message_exit "Failed running $patch_shell_script"
	elif [ -f $patch_shell_script ]; then 
	    build_message_err "$patch_shell_script exists but is not executable!"
	else
	    build_message_out "No patch shell script ($patch_shell_script) exists."
	fi
	####################################################
    else
	build_message_err "Usage: build_run_pre_patch <current_package>"
    fi
}

function build_run_post_patch()
{
    if [ "x$1" != "x" ]; then
	package_version=$1
	patch_shell_script=${BASE_DIR}/${PATCH_DIR}/${package_version}_post.sh

	if [ -x $patch_shell_script ]; then
	    build_message_out "Running $patch_shell_script $BASE_DIR $package_version"
	    $patch_shell_script $BASE_DIR $package_version \
		|| build_message_exit "Failed running $patch_shell_script"
	elif [ -f $patch_shell_script ]; then 
	    build_message_err "$patch_shell_script exists but is not executable!"
	else
	    build_message_out "No patch shell script ($patch_shell_script) exists."
	fi
	####################################################
    else
	build_message_err "Usage: build_run_post_patch <current_package>"
    fi
}

function build_patch_libtool_dylib()
{
    build_message_out "patch litool."

    if [ "x$UNIVERSAL_BUILD" == "xyes" ]; then
	LIBTOOL_FILES=$(find . -name libtool)
	
	build_message_out "Patching $LIBTOOL_FILES"

	for libtool_file in $LIBTOOL_FILES
	do
	    libtool_dir=$(dirname $libtool_file)
	    build_message_out "Backing up $libtool_file"
	    
	    cp $libtool_file $libtool_dir/libtool.old               || build_message_err "Failed to backup $libtool_file"
	    
	    $PERL -pi -e "s@-dynamiclib@$CFLAGS \$&@" $libtool_file || build_message_err "Failed patching $libtool_file"
	    
	    if [ "x$1" = "xwithbundle" ]; then
		$PERL -pi -e "s@-bundle@$CFLAGS \$&@" $libtool_file || build_message_err "Failed patching $libtool_file"
	    fi
	done
    fi
}

function build_trap_handle() 
{
    build_message_exit "Signal caught";
}

trap 'build_trap_handle;' INT
trap 'build_trap_handle;' TERM
trap 'build_trap_handle;' QUIT

