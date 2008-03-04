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

zmap_message_out "Start of script."

zmap_message_out "Running in $INITIAL_DIR on $(hostname) under $(uname)"

zmap_message_out "cmd line options = '$*'"

ZMAP_BUILD_CONTAINER=$1

zmap_check ${ZMAP_BUILD_CONTAINER:=$INITIAL_DIR}

zmap_goto_cvs_module_root 

zmap_cd src

if [ "x$ZMAP_MASTER_BUILD_DOXYGEN_DOCS" == "x$ZMAP_TRUE" ]; then
    zmap_message_out "Running make docs ..."

    make docs || zmap_message_exit "Failed making documentation."
fi

if [ "x$ENSCRIPT_EXE" != "x" ]; then

    zmap_check ${ENSCRIPT_OUTPUT_FLAG:=-w}

    CANVAS_DOCS_OUT_DIR=$ZMAP_BUILD_CONTAINER/$FOOCANVAS_DOC_TARGET
    
    mkdir -p $CANVAS_DOCS_OUT_DIR || zmap_message_exit "Failed creating output directory."
    
    # Because we cd out of this dir we need CANVAS_DOCS_OUT_DIR to be absolute
    # This is how we do that.
    if ! echo $CANVAS_DOCS_OUT_DIR | egrep "(^)/" ; then
	this_dir=$(pwd)
	zmap_cd $CANVAS_DOCS_OUT_DIR
	CANVAS_DOCS_OUT_DIR=$(pwd)
	zmap_cd $this_dir
    fi

    foo_headers_dir=`find $GTK_PREFIX/include -name libfoocanvas`
    
    if [ "x$foo_headers_dir" != "x" ]; then

	this_dir=$(pwd)

	zmap_cd $foo_headers_dir
    
	foo_headers=`find . -name 'foo*.h'`
    
	for header in $foo_headers ;
	  do
	  header=$(basename $header)
	  zmap_message_out "Creating enscript doc for FooCanvas file $header"
	  $ENSCRIPT_EXE --color $ENSCRIPT_OUTPUT_FLAG html -Ec -o $CANVAS_DOCS_OUT_DIR/$header.shtml $header || zmap_message_exit "Failed to $enscript $header"
	done
    
	zmap_cd $this_dir
    else
	zmap_message_err "Failed to find libfoocanvas directory under $GTK_PREFIX/include. Is it installed there?"
    fi
fi

exit 0;
