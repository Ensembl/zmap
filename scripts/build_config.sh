#!/bin/echo dot script please source

# First thing we do is get the location of this script.
# We do it this way as we will have been sourced. $0 is
# therefore the sourcing script and not this one!
# This relies on the caller having set -o history
CONFIG_DIR=""
HISTORY=$(history $HISTCMD)
if [ "x$HISTORY" != "x" ]; then
    for element in $HISTORY ; do
	CONFIG_DIR=$element
	if echo $CONFIG_DIR | egrep -q "build_config.sh$" ; then
	    break;
	fi
    done
    eval CONFIG_DIR=$(dirname $CONFIG_DIR)
fi

# fall back to base_dir
[ "x$CONFIG_DIR" != "x" ] || CONFIG_DIR=$BASE_DIR
# The above is because the old version of bash doesn't have...
# ${BASH_SOURCE} should be just what you're asking for.
# ${BASH_SOURCE%/*} should give you the path you need.
# ${BASH_SOURCE##*/} would give just the actual name of the file,
# just in case you need to know the name it was called by.

# Some simple functions to start with. skip over if just changing variables
# Usage: _config_message_err <message>
function _config_message_err
{
    now=$(date +%H:%M:%S)
    script=$(basename $0)
    echo "[$script ($now)] $*" >&2
}
# Usage: _config_message_exit <message>
function _config_message_exit
{
    now=$(date +%H:%M:%S)
    script=$(basename $0)
    echo "[$script ($now)] $*"
    exit 1;
}
# Usage: _config_set_ZMAP_HOST_TYPE
# ZMAP_HOST_TYPE will be set for you.
function _config_set_ZMAP_HOST_TYPE
{
    local uname_s uname_m
    ZMAP_HOST_TYPE=""
    uname_s=$(uname -s)
    case $uname_s in
	Linux)
	    uname_m=$(uname -m)
	    case $uname_m in
		x86_64)
		    ZMAP_HOST_TYPE=linux-64
		    ;;
		* )
		    ZMAP_HOST_TYPE=linux-i386
		    ;;
	    esac
	    ;;
	Darwin)
	    ZMAP_HOST_TYPE=mac
	    ;;
	*)
	    _config_message_exit "Unsupported host type '$host_type'"
	    ;;
    esac
}
# Usage: _config_source_file <file>
function _config_source_file
{
    [ "x$1" != "x" ] || _config_message_exit "Usage: _config_source_file <file>"

    if [ -f $1 ]; then
	. $1
    else
	_config_message_err "*** WARNING *** : config file $1 not found. This may be fine."
    fi
}

#_config_message_err $CONFIG_DIR

# ==========================================================
# General/Global Defaults.
#
# So to the variables...
# ==========================================================
# Include ones to be overwritten in host type and machine specific files
# yes = TRUTH
# no  = FALSEHOOD

# config for the master build script
#ZMAP_MASTER_HOST=tviewsrv
ZMAP_MASTER_HOST=deskpro021045


# disabling this until I can work out how to selectively include doxygen...
ZMAP_MASTER_BUILD_DOCS=no
ZMAP_MASTER_BUILD_DOXYGEN_DOCS=no
ZMAP_MASTER_CVS_RELEASE_NOTES=no
ZMAP_MASTER_RT_RELEASE_NOTES=no
ZMAP_MASTER_FORCE_RELEASE_NOTES=no

# disabling this too until I can get it all to work well.
ZMAP_MASTER_DOCS2WEB=no
ZMAP_MASTER_WEBPUBLISH=no

# testing does not work at all at the moment so I'm disabling it....
ZMAP_MASTER_RUN_TEST_SUITE=no


ZMAP_MASTER_REMOVE_FOLDER=yes
ZMAP_MASTER_NOTIFY_MAIL=zmapdev@sanger.ac.uk

ZMAP_MASTER_BUILD_DIST=no
ZMAP_MASTER_BUILD_CANVAS_DIST=no

ZMAP_CLUSTER_CONFIG_FILE=~zmap/cluster.config.sh


# temp for ubuntu builds...
#ZMAP_BUILD_MACHINES="tviewsrv mac18480i cbi4"
ZMAP_BUILD_MACHINES="deskpro021045 mac18480i lucid-dev64"


ZMAP_SSH_OPTIONS="-oStrictHostKeyChecking=no \
-oConnectTimeout=3 \
-oSetupTimeOut=3 \
-oPasswordAuthentication=no \
-oNumberOfPasswordPrompts=0"

ZMAP_MAKE=yes
ZMAP_MAKE_CHECK=yes
ZMAP_MAKE_INSTALL=yes
ZMAP_RELEASES_DIR=~zmap/BUILDS
ZMAP_RELEASE_LEVEL=DEVELOPMENT
# RELEASE_SYMLINK is made by zmap_get_release_symlink()
ZMAP_RELEASE_SYMLINK='~zmap/BUILD.${ZMAP_RELEASE_LEVEL}'


# acedb stuff
ZMAP_ACEDB_RELEASE_CONTAINER=~acedb
if [ -z "$ZMAP_ACEDB_RELEASE_DIR" ] ; then
  ZMAP_ACEDB_RELEASE_DIR='DEVELOPMENT'
fi
ZMAP_ACEDB_BINARIES='tace xace sgifaceserver giface makeUserPasswd xremote'


# seqtools stuff
ZMAP_SEQTOOLS_RELEASE_CONTAINER=~zmap/SeqTools
if [ -z "$ZMAP_SEQTOOLS_RELEASE_DIR" ] ; then
  ZMAP_SEQTOOLS_RELEASE_DIR='BUILD.DEVELOPMENT'
fi
ZMAP_SEQTOOLS_BINARIES='blixem blixemh dotter belvu'

# AceConn stuff
ZMAP_ACECONN_RELEASE_CONTAINER=$ZMAP_ACEDB_RELEASE_CONTAINER
ZMAP_ACECONN_RELEASE_DIR=AceConn


ZMAP_RELEASE_NOTES_TIMESTAMP=LAST_RELEASE_DATE.txt
ZMAP_VERSION_HEADER=zmapUtils_P.h
ZMAP_WEBPAGE_HEADER=zmapWebPages.h
ZMAP_RELEASE_FILE_PREFIX=release_notes
ZMAP_RELEASE_FILE_SUFFIX=shtml


WEBPUBLISH=webpublish
ENSCRIPT_EXE=enscript
ENSCRIPT_OUTPUT_FLAG=-W
FOOCANVAS_DOC_TARGET=web/foocanvas
EXPAT_HTML_DOC_LOCATION=/usr/share/doc/libexpat1-dev/expat.html

# These need to be relative (sub dirs) to $ZMAP_BUILD_CONTAINER!
# The directory itself will not be copied, but _all_ of it's children will
#
ZMAP_WEBSITE_SOURCE_DIRS="docs/ZMap doc web"

# ZMAP_WEBSITE_SINGLE_FILES is a list of files which need to be copied across too.
# format is "from_location:to_location from_location1:to_location1 from_location2:to_location2"
# As with above from_location needs to relative to build container
# to_location needs to be in one of trees in ZMAP_WEBSITE_SOURCE_DIRS if it involves directories
ZMAP_WEBSITE_SINGLE_FILES="src/zmapServer/acedb/models.wrm:user_doc/models.wrm.shtml"

WEBROOT=/nfs/WWWdev/SANGER_docs/htdocs
WEBUSER=zmap
# Should be the machine where /nfs/WWWdev is exported!
WEBHOST=tviewsrv
#WEBHOST=deskpro16113
ZMAP_WEBSITE_TARGET=$WEBROOT/Software/analysis/ZMap

# bootstrap (autotools locations)
# better to rely on PATH though?

# LIBTOOLIZE=libtoolize
# AUTOHEADER=autoheader
# ACLOCAL=aclocal
# AUTOMAKE=automake
# AUTOCONF=autoconf
# AUTOUPDATE=autoupdate

NEED_LIBTOOLIZE=yes


# runconfig variables
#

# CC, CFLAGS & LDFLAGS are passed into configure on the command line as
# configure CFLAGS=$CFLAGS_args
# runconfig sorts this out!
CC_args="gcc"
CFLAGS_args="-Wall"
LDFLAGS_args=""

# actual arguments configure understands (see configure --help | less)
COMMON_CONFIGURE_ARGS="--enable-share --disable-static --enable-debug --enable-shtml-doc"
SPECIFIC_CONFIGURE_ARGS=""

#
FOOCANVAS_DEV=no
FOOCANVAS_PREFIX=/var/tmp/rds/software
# For machines that mount /software tree this should automatically be machine specific!
GTK_PREFIX=/software/acedb
USE_GPROF=no
UNIVERSAL_BUILD=no


# further runconfig stuff that reference above
PKG_CONFIG=$GTK_PREFIX/bin/pkg-config
ACLOCAL_FLAGS="-I $GTK_PREFIX/share/aclocal"


# Now source host specific variables/values
# First we source host type specific ones
# Then host name specific ones
_config_set_ZMAP_HOST_TYPE

if [ "x$CONFIG_DIR" != "x" ]; then
    _config_source_file $CONFIG_DIR/build_config.$ZMAP_HOST_TYPE.sh
    _config_source_file $CONFIG_DIR/build_config.$(hostname -s).sh
else
    _config_message_err "*** WARNING *** : \$CONFIG_DIR not set in $0"
fi


# Fix up the variables that need exporting/prepending...

export PKG_CONFIG_PATH
export ACLOCAL_FLAGS

# This should  be the last  thing as it  should never fail and  we can
# then test return code of source _this_script_
echo >/dev/null
