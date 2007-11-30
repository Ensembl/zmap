#!/bin/bash

########################
# Find out where BASE is
# _must_ be first part of script
SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR

############
# Need help?
HELP=`echo $* | grep help`
if [ "x$HELP" != "x" ]; then
    cat $BASE_DIR/README
    exit 0;
fi

#################################
# source the config and functions
. $BASE_DIR/build_config.sh

. $BASE_DIR/build_functions.sh

# we keep some state between runs
build_create_load_status_file

# in case they aren't there.
build_create_subdirs

# make command line configuration work
if [ $# > 0 ]; then
    eval "$*"
fi

# we save some config for sub-processes
build_save_execution_config

# N.B. alter variables to incorporate $PREFIX
# this should just do the right thing
build_set_vars_for_prefix

# Some state information to terminal
build_message_out "Running $0 $*"
build_message_out "Configuration:"
build_message_out "    BASE_DIR=$BASE_DIR DIST_DIR=$DIST_DIR BUILD_DIR=$BUILD_DIR"
build_message_out "    PREFIX=$PREFIX"
build_message_out "    CONFIGURE_OPTS=$CONFIGURE_OPTS"
build_message_out "    CLEAN_BUILD_DIR=$CLEAN_BUILD_DIR"
build_message_out "    CLEAN_DIST_DIR=$CLEAN_DIST_DIR"
build_message_out ""
build_message_out "    UNIVERSAL_BUILD=$UNIVERSAL_BUILD"

build_message_out ""                                                    
build_message_out "If the configuration above is wrong, edit $BASE_DIR/build_config.sh or "
build_message_out "run this script again as $0 VARIABLE=value"
build_message_out ""                                                    
build_message_out "e.g. $0 PREFIX=/usr/local CONFIGURE_OPTS=--disable-docs"
build_message_out ""
build_message_out ""
build_message_out "The GTK+ site has this README with regards to dependencies"                                   
build_message_out ""
build_message_out ""

build_download_file $PACKAGE_gtk_URL/dependencies/README "-"

build_message_out ""
build_message_out "Is everything configured?"
build_message_out ""

# pause for thought?
if [ "x$SLEEP" != "x0" ]; then 
    build_message_out "pausing for thought..."
    build_message_out "pass SLEEP=0/edit build_config.sh to stop this wait..." 
fi
sleep $SLEEP

# restart doing something
build_message_out ""
build_message_out ""
build_message_out "***** Building... *****"
build_message_out ""
build_message_out ""

# And now to the meat of the script
if [ "x$CLEAN_BUILD_DIR" == "xyes" ]; then
    build_clean_build_dir
fi

if [ "x$CLEAN_DIST_DIR" == "xyes" ]; then
    build_clean_dist_dir
fi

# Go through the list of packages to install
# downloading (if necessary), untarring, patching (if necessary),
# ./configure'ing, make'ing and make installing

for CURRENT_PACKAGE_NAME in $BUILD_LIST_OF_PACKAGES;
  do

    # create the 5 package config variables...
    TEMP_URL="PACKAGE_${CURRENT_PACKAGE_NAME}_URL"
    TEMP_NAME="PACKAGE_${CURRENT_PACKAGE_NAME}_NAME"
    TEMP_VERSION="PACKAGE_${CURRENT_PACKAGE_NAME}_VERSION"
    TEMP_EXT="PACKAGE_${CURRENT_PACKAGE_NAME}_EXT"
    TEMP_INSTALLED="PACKAGE_${CURRENT_PACKAGE_NAME}_INSTALLED"
    
    # Watch these evals
    eval "CURRENT_PACKAGE=\${$TEMP_NAME}-\${$TEMP_VERSION}"
    # special case libjpeg
    if [ "x$CURRENT_PACKAGE_NAME" == "xlibjpeg" ]; then
      # loving libjpeg!
	eval "CURRENT_PACKAGE=\${$TEMP_NAME}.\${$TEMP_VERSION}"
    fi
    eval "CURRENT_PACKAGE_DIST=${CURRENT_PACKAGE}.\${$TEMP_EXT}"
    eval "CURRENT_PACKAGE_URL=\${$TEMP_URL}"
    eval "CURRENT_PACKAGE_INSTALLED=\${$TEMP_INSTALLED}"

    # So now we have #
    # CURRENT_PACKAGE_NAME - variable safe name
    # CURRENT_PACKAGE      - package-version (except libjpeg's package.version)
    # CURRENT_PACKAGE_DIST - package-verson.ext
    # CURRENT_PACKAGE_URL  - http://.../package-version.ext
    # CURRENT_PACKAGE_INSTALLED - yes?

    build_message_out ""
    build_message_out "*** $CURRENT_PACKAGE ***"
    build_message_out ""
    
    # lets download the file
    build_download_file ${CURRENT_PACKAGE_URL}/$CURRENT_PACKAGE_DIST
    
    # we might only want to get the dists for building later.
    if [ "x$GET_ONLY" != "xyes" ]; then
	# untar into a directory we can cd to later
	# currently using $CURRENT_PACKAGE as directory name
	build_untar_file $CURRENT_PACKAGE_DIST $CURRENT_PACKAGE 

	# we might need to patch and/or run a shell script here.
	# build_run_patch doesn't care if the files don't exist.
	build_run_pre_patch $CURRENT_PACKAGE
	
	# If the package isn't installed, build it, else move on.
	if [ "x$CURRENT_PACKAGE_INSTALLED" != "xyes" ]; then
	    build_build_package $CURRENT_PACKAGE $CURRENT_PACKAGE_NAME
	    $ECHO "$TEMP_INSTALLED=yes" >> $BUILD_STATUS_FILE
	else
	    build_message_out "$CURRENT_PACKAGE is already installed"
	fi
    fi

    # and that's it for another package
done

build_create_load_status_file

if [ "x$PACKAGE_gtk_INSTALLED" == "xyes" ]; then
    build_message_out "GTK+v2 is now installed in $PREFIX"
    exit 0;
fi

exit 1;
