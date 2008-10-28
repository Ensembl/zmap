#!/bin/bash

if [ $# -lt 2 ]
then echo "usage: `basename $0` clientuser command" >&2
     exit 2
fi

CLIENTUSER="$1"
shift

# FD 4 becomes stdin too
exec 4>&0

HOST_UNIX=`hostname`/unix
DISPLAY_QUERY=$DISPLAY
DISPLAY_QUERY=`echo $DISPLAY_QUERY | sed -e "s!localhost!$HOST_UNIX!"`

xauth list "$DISPLAY_QUERY" | sed -e 's/^/add /' | {
    
    # FD 3 becomes xauth output
    # FD 0 becomes stdin again
    # FD 4 is closed
    exec 3>&0 0>&4 4>&-

    su -s/bin/bash "$CLIENTUSER" -c "\
xauth -q <&3
xauth list '$DISPLAY'
DISPLAY=$DISPLAY $* 3>&-
xauth remove $DISPLAY
xauth list $DISPLAY
echo Finished"

}

exit

FB_DISPLAY=:4

# If there's no display, we need to make one
if [ "x$DISPLAY" == "x" ]; then

    # Start the server
    Xvfb $FB_DISPLAY -screen 0 1024x768x24 -terminate &
    # need to fix font path issues (-fp ?)
    # grep FontPath /etc/X11/xorg.conf
else
    FB_DSPLAY=$DISPLAY
fi

# set display
export DISPLAY=$FB_DISPLAY



# So what we need to do is figure out what the display is/isn't



export X
