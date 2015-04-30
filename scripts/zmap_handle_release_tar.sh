#!/bin/bash

SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
if ! echo $SCRIPT_DIR | egrep -q "(^)/" ; then
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR
else
   BASE_DIR=$SCRIPT_DIR
fi

. $BASE_DIR/zmap_functions.sh || { echo "Failed to load zmap_functions.sh"; exit 1; }
set -o history
. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }

zmap_read_cluster_config

# ================== FUNCTIONS ==================

# Usage: zmap_untar_file <tarfile.tar.gz> <output_dir>
function zmap_untar_file
{
    untar_tmp="untar_tmp.$$"
    restore_dir=$(pwd)

    if [ "x$1" != "x" ]; then
        #
        # Get the arguments
        #
	package=$1
	output_dir=$package

	if ! echo $package | egrep -q "(^)/" ; then
	    package=$restore_dir/$package
	fi

	if [ "x$2" != "x" ]; then
	    output_dir=$2
	fi

	if ! echo $output_dir | egrep -q "(^)/" ; then
	    output_dir=/var/tmp/$output_dir
	fi

        #
        # Get the tar options
        #
	if echo $package | egrep -q "tar.gz$" ; then
	    tar_options="-zxf"
	fi

	if echo $package | egrep -q ".tgz$" ; then
	    tar_options="-zxf"
	fi

	if echo $package | egrep -q ".tar.bz2$" ; then
	    tar_options="-jxf"
	fi

        #
        # Make a temp dir on the destination location to untar to
        #
        output_dir_parent=`dirname $output_dir`
        untar_tmp_path=$output_dir_parent/$untar_tmp
	mkdir -p $untar_tmp_path

        #
        # Copy and untar the package in the temp destination
        #
        zmap_message_out "Copying $package to $untar_tmp_path"
        cp $package $untar_tmp_path

        package_name=`basename $package`
        zmap_cd $untar_tmp_path
	zmap_message_out "Untarring $package_name to $untar_tmp_path"
	tar $tar_options $package_name

	if [ $? != 0 ]; then
	    zmap_cd $output_dir_parent
	    rm -rf $untar_tmp
	    zmap_message_exit "Failed to untar $package_name. Removed $untar_tmp_path."
	else
            #
            # Get the name of the build directory. There should only be one item
            # in the tmp directory (after removing the tar file), so we can just
            #use ls to get the build dir name.
            #
            rm -f $package_name
	    package_dir=$(ls)

            #
            # Move the build dir to the output_dir location (remove the output
            # dir first if it already exists).
            #
	    if [ -d $output_dir ]; then
	        zmap_message_out "$output_dir already exists... Removing"
	        rm -rf $output_dir
	    fi

	    if [ ! -d $output_dir ]; then
		mv -f $package_dir $output_dir
	    else
		cp -r $package_dir/* $output_dir/
	    fi

            #
            # Remove the temp untar dir
            #
	    zmap_cd $output_dir_parent
	    rm -rf $untar_tmp_path
	    zmap_message_out "$package_name successfully untarred to $output_dir"
	fi
    fi

    zmap_cd $restore_dir

}

# ================== MAIN PART ==================

usage="$0 -t <tar_file> -r <release_location> VARIABLE=VALUE"
while getopts ":t:r:" opt ; do
    case $opt in
	t  ) TAR_FILE=$OPTARG            ;;
	r  ) RELEASE_LOCATION=$OPTARG    ;;
	\? ) zmap_message_exit "$usage"
    esac
done


shift $(($OPTIND - 1))

# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi

[ "x$TAR_FILE" != "x" ]         || zmap_message_exit "No tar file specified: Usage = $usage"
[ "x$RELEASE_LOCATION" != "x" ] || zmap_message_exit "No release location specified: Usage = $usage"

zmap_untar_file $TAR_FILE $RELEASE_LOCATION

#ZMAP_RELEASE_VERSION=$($SCRIPT_DIR/versioner \
#    -path $SCRIPT_DIR/../ \
#    -show -V -quiet) || zmap_message_exit "Failed to get zmap version"
ZMAP_RELEASE_VERSION=$($SCRIPT_DIR/git_version.sh -a)



zmap_cd $RELEASE_LOCATION

# Now make a README

# EOF = variable substitution
cat > README <<EOF
ZMap Release Version $ZMAP_RELEASE_VERSION
-------------------------------------

Info:

...


Manifest:
-/
  ZMap/        - cvs checkout (modified with build)
  ZMap.master/ - cvs checkout (as per tag)
  Dist/        - contains distribution tar.gz
  Docs/        - Some documentation
  Website/     - website
EOF

for host in $ZMAP_BUILD_MACHINES;
  do
  config_set_ZMAP_ARCH $host
  HOST_UNAME=$ZMAP_ARCH
  cat >> README <<EOF
  ZMap.$host/  - build dir from $host
  $HOST_UNAME/ - prefix install for OS $HOST_UNAME
  $host.log    - log file from build on $host
EOF
done

# 'EOF' = no substitution
cat >> README <<'EOF'


--------------------------------------------------------------------
ZMap is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
--------------------------------------------------------------------
This file is part of the ZMap genome database package
originally written by:

Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk

Description:

Created: Tue Mar  4 10:32:15 2008 (rds)
CVS info:   $Id: zmap_handle_release_tar.sh,v 1.8 2009-02-12 09:26:05 rds Exp $
--------------------------------------------------------------------


EOF





# ================== END OF SCRIPT ==================
