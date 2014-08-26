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

. $BASE_DIR/build_config.sh   || { echo "Failed to load build_config.sh";   exit 1; }






CMDSTRING='<ZMap dir> <date> <output_file>'
DESCSTRING=`cat <<DESC

The ZMap dir gives the git directory containing the zmap code.

The date must be in the format "dd/mm/yyyy"

The output file is where the RT release notes should be stored.
DESC`

ZMAP_ONLY=no
ZMAP_BASEDIR=''
output_file=''


# Note that the zmap user must have permissions within RT to see and query the queue
# or no data will come back.
RT_QUEUES="zmap"

zmap_message_out "Starting processing RT tickets"



# get the date and the output file - mandatory.
#
shift $(($OPTIND - 1))

ZMAP_BASEDIR=$1
date=$2
output_file=$3

if [ -z "$ZMAP_BASEDIR" ] ; then
    zmap_message_exit 'No directory specified.'
fi

if [ -z "$date" ] ; then
    zmap_message_exit 'No date specified.'
fi

if [ -z "$output_file" ] ; then
    zmap_message_exit 'No output file specified.'
fi



#
# ok....off we go....
#


# Get build directory (parent of $ZMAP_BASEDIR) for logging file.
build_dir=$(dirname $ZMAP_BASEDIR)


# Go to respository directory...
zmap_cd $ZMAP_BASEDIR



RT_PREV_DATE=$date
RT_CURRENT_DATE=$(date "+%d/%m/%Y")



rm -f $output_file || zmap_message_exit "Cannot rm $output_file file."
touch $output_file || zmap_message_exit "Cannot touch $output_file file."
chmod u+rw $output_file || zmap_message_exit "Cannot make $output_file r/w"

zmap_message_out "Using $output_file as release notes output file."




#
# Output all resolved RT tickets since given date.
#
#

zmap_message_out "Getting tickets for period $RT_PREV_DATE to $RT_CURRENT_DATE."


# Write header.
#
cat >> $output_file <<EOF

Request Tracker Tickets Resolved from $RT_PREV_DATE to $RT_CURRENT_DATE

EOF



RTSERVER=tviewsrv
#RTSERVER=deskpro16113
RTHTTPSERVER="https://rt.sanger.ac.uk"
RTUSER=zmap
RTRESULTS="$build_dir/rt_tickets.out"
RTERROR="$build_dir/rt_error.log"
RTFAILED=no

#zmap_message_out "pwd =" $(pwd)


# This looks complicated, but the basic method of action is cat stdin (The HERE
# doc) and the getRTtickets over the ssh connection to the bash "one-liner" to
# execute it there.
#
# Why? so that we don't have to checkout zmap on the RT machine,
# nor do we need to have getRTtickets in a central place where it must always be
# git up-dated. This way it gets sucked from this directory which is either
# the git up-to-date checkout in the build that's going on, or the development
# version if it needs to change.

# Variables are expanded here, so escape $ for vars set
# inside.
(cat - $BASE_DIR/getRTtickets <<EOF
#!/bin/bash
# --- automatically added by $0 ---

start_date=$RT_PREV_DATE
OPS_QUEUES="$RT_QUEUES"

# make sure that the ~/.rtrc contains the necessary
RC_FILE=~/.rtrc
touch \$RC_FILE      || { echo "Failed to touch \$RC_FILE"; exit 1; }
chmod 600 \$RC_FILE  || { echo "Failed to chmod \$RC_FILE"; exit 1; }

has_passwd=\$(grep passwd \$RC_FILE)
if [ "x\$has_passwd" == "x" ]; then
   echo "\$RC_FILE has no passwd, but this is required so $0 won't hang" >&2
   exit 1
fi

perl -i -lne 'print if !/server/;' \$RC_FILE
perl -i -lne 'print if !/user/;'   \$RC_FILE

echo "server $RTHTTPSERVER" >> \$RC_FILE
echo "user $RTUSER"         >> \$RC_FILE

# Ensure /software/bin is in the path for rt command.
PATH=/software/bin:$PATH

# --- end of addition by $0 ---
EOF
) | ssh $RTSERVER '/bin/bash -c "\
: N.B. This is a comment ;              \
: N.B. No variables are expanded here ; \
function _rm_exit                       \
{                                       \
   echo getRTtickets Step Failed...>&2; \
   rm -f host_checkout.sh;              \
   exit 1;                              \
};                                      \
cd /var/tmp               || exit 1;    \
rm -f getRTtickets.sh     || exit 1;    \
cat - > getRTtickets.sh   || exit 1;    \
chmod 700 getRTtickets.sh || _rm_exit;  \
./getRTtickets.sh         || _rm_exit;  \
rm -f getRTtickets.sh     || exit 1;    \
"' > $RTRESULTS 2>$RTERROR || RTFAILED=yes


# If we definitely failed then log it
if [ "x$RTFAILED" == "xyes" ]; then
    zmap_message_err "Looks like getRTtickets failed..."
    error=$(cat $RTERROR)
    # hopefully the error will be in here.
    if [ "x$error" != "x" ]; then
	zmap_message_exit $error
    else
	# If not...
	zmap_message_exit "Failed to getRTtickets. See $RTRESULTS"
    fi
fi

zmap_message_out "Finished getting Request tracker tickets"

# Something in the getRTtickets script could have failed
# without exiting the script. Check this and log it.
# If it was fatal then the getRTtickets script should
# be made to do the right thing.
if [ -f $RTERROR ]; then
    error=$(cat $RTERROR)
    if [ "x$error" != "x" ]; then
	zmap_message_exit $error
    fi
fi


# Put tickets where they should be.....
#
#
cat $RTRESULTS >> $output_file || zmap_message_exit "Failed to store RT results in $output_file"

rm $RTRESULTS



# Must have succeeded in getting tickets.
#


cat >> $output_file <<EOF

End of Request Tracker tickets

EOF


zmap_message_out "Finished processing RT tickets"

exit 0
