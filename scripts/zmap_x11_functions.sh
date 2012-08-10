#!/bin/echo dot script. please source

X11_ENVIRONMENT_FILE=/var/tmp/zmap.$$.env
XSERVER_PATH=Xvnc
XSERVER_OPTIONS="-terminate -desktop ${USER}-running-$SCRIPT_NAME -depth 24 -geometry 1200x900"
XSERVER_ADD_OPTIONS="-rfbauth $HOME/.vnc/passwd"
VNC_PASSWD=tesswheat
# ah, test suite

# Do we need this?
function zmap_x11_restore_enviroment
{
    zmap_message_out "restore environment?"
    # . $X11_ENVIRONMENT_FILE
    rm -f $X11_ENVIRONMENT_FILE
}

function zmap_x11_message_exit
{
    zmap_message_err "$@"
    zmap_x11_restore_enviroment
    exit 1;
}

function zmap_x11_save_enviroment
{
    zmap_message_out "save environment?"
    # declare -p DISPLAY XAUTHORITY > $X11_ENVIRONMENT_FILE
}

function zmap_x11_examine_environment
{
    NO_X_CONN=0
    NO_DISPLAY=0
    NO_SSH=0

    [ -n "$XAUTHORITY" ] || {
	zmap_message_out "XAUTHORITY unset, setting..."
	XAUTHORITY=$HOME/.Xauthority
	zmap_message_out "set to '$XAUTHORITY'"
    }
    [ -O $XAUTHORITY ]   || {
	zmap_message_out "Cannot access $XAUTHORITY"
	XAUTHORITY=$HOME/.Xauthority
	zmap_message_out "set to '$XAUTHORITY'"
	# No valid connection can be made
	NO_X_CONN=1
	# need to start an xserver
	DISPLAY=""
	unset DISPLAY
    }

    if [ -z "$DISPLAY" ]; then
	# We''l need to start an xserver
	zmap_message_out "No DISPLAY variable set."
	NO_DISPLAY=1
	DISPLAY=:4
	export DISPLAY
    else
	if [ "x$DISPLAY" == ":0.0" ]; then
	    zmap_message_out "Looks like xserver is accessible."
	    NO_SSH=1
	else
	    zmap_message_out "Looks like xserver is accessible. (Forwarded)"
	    NO_SSH=0
	fi
    fi
}

function zmap_x11_get_xserver
{
    if [ "x$NO_DISPLAY" == "x1" ]; then
	zmap_message_out "Starting Xserver [$XSERVER_PATH]"
	$XSERVER_PATH $DISPLAY $XSERVER_OPTIONS $XSERVER_ADD_OPTIONS &
    else
	zmap_message_out "Using current xserver"
    fi
}

# print out lots of information here.
# DISPLAY=$DISPLAY
# XAUTHORITY=$XAUTHORITY
function zmap_x11_check_xserver
{
    zmap_message_out "Variables look like:"
    declare -p DISPLAY XAUTHORITY
    if [ "x$NO_DISPLAY" == "x1" ]; then
	XSERVER_PID=`pidof $XSERVER_PATH`
	[ "x$XSERVER_PID" == "x" ] && zmap_message_exit "Server failed to start..."
	zmap_message_out "X Server pid = '$XSERVER_PID'"
    fi
}



