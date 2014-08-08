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






CMDSTRING='[ -o <directory>  -z ] <date> <ZMap directory>'
DESCSTRING=`cat <<DESC

   -o   specify an alternative output directory for the release notes.

   -z   only output zmap RT tickets.

The date must be in the format "dd/mm/yyyy"

ZMap directory must be the base ZMap directory of the build directory so that the docs
and the code match.
DESC`

ZMAP_ONLY=no
ZMAP_BASEDIR=''
output_file=''


# Note that the zmap user must have permissions within RT to see and query these queues
# or no data will come back.
RT_QUEUES="zmap seqtools"

zmap_message_out "Starting processing RT tickets"

zmap_message_out "Running in $INITIAL_DIR on $(hostname)"

zmap_message_out "Parsing cmd line options: '$*'"


while getopts ":o:z" opt ;
  do
  case $opt in
      o  ) output_file=$OPTARG ;;
      z  ) ZMAP_ONLY=yes ;;
      \? ) 
zmap_message_exit "Bad command line flag

Usage:

$0 $CMDSTRING

$DESCSTRING"
;;
  esac
done


if [ -n "$output_file" ] ; then
    tmp_file=`readlink -m $output_file`

    output_file=$tmp_file

    echo "output_file = $output_file"
fi

if [ "x$ZMAP_ONLY" == "xyes" ]; then
    RT_QUEUES="zmap"
fi



# get the date and the ZMap dir - mandatory.
#
shift $(($OPTIND - 1))

date=$1
ZMAP_BASEDIR=$2


if [ -z "$date" ] ; then
    zmap_message_exit 'No date specified.'
fi

if [ -z "$date" ] ; then
    zmap_message_exit 'No directory specified.'
fi




#
# ok....off we go....
#




# Go to respository directory...
zmap_cd $ZMAP_BASEDIR



RT_PREV_DATE=$date
RT_CURRENT_DATE=$(date "+%d/%m/%Y")


zmap_message_out "Getting tickets for period $RT_PREV_DATE to $RT_CURRENT_DATE."


if  [ -n "$output_file" ] ; then
    RELEASE_NOTES_OUTPUT="$output_file"
else
    RELEASE_NOTES_OUTPUT="$ZMAP_BASEDIR/src/$ZMAP_RELEASE_DOCS_DIR/$ZMAP_RT_RESOLVED_FILE_NAME"
fi

rm -f $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot rm $RELEASE_NOTES_OUTPUT file."
touch $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot touch $RELEASE_NOTES_OUTPUT file."
chmod u+rw $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot make $RELEASE_NOTES_OUTPUT r/w"

zmap_message_out "Using $RELEASE_NOTES_OUTPUT as release notes output file."




#
# Output all resolved RT tickets since given date.
#
#


# Write header.
#
cat >> $RELEASE_NOTES_OUTPUT <<EOF

Request Tracker Tickets Resolved

This report covers $RT_PREV_DATE to $RT_CURRENT_DATE

EOF



RTSERVER=tviewsrv
#RTSERVER=deskpro16113
RTHTTPSERVER="https://rt.sanger.ac.uk"
RTUSER=zmap
RTRESULTS=rt_tickets.out
RTERROR=rt_error.log
RTFAILED=no

#zmap_message_out "pwd =" $(pwd)

zmap_message_out "Getting Request Tracker tickets using $RTSERVER as node to launch query."

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

# Really ensure /software/acedb/bin is in the path
PATH=/software/acedb/bin:$PATH

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
cat $RTRESULTS >> $RELEASE_NOTES_OUTPUT || zmap_message_exit "Failed to store RT results in $RELEASE_NOTES_OUTPUT"

rm $RTRESULTS



# Must have succeeded in getting tickets.
#
zmap_message_out "Finished getting Request tracker tickets"

cat >> $RELEASE_NOTES_OUTPUT <<EOF

End of Request Tracker tickets

EOF



# NOT DOING THIS AT THE MOMENT.....
#
#
#zmap_message_out "Now processing RT tickets"
#
## Here are the results...good for debugging....
##
##zmap_message_out "Here are RT tickets"
##cat $RTRESULTS
##zmap_message_out "End of RT tickets"
#
#
## This goes directly into the html file
#$BASE_DIR/process_rt_tickets_file.pl $RTRESULTS >> $RELEASE_NOTES_OUTPUT || \
#    zmap_message_exit "Failed processing RT tickets"
#
## End of RT tickets
##
#cat >> $RELEASE_NOTES_OUTPUT <<EOF
#
#<!-- End of tickets  --!>
#
#EOF
#
#zmap_message_out "Finished processing RT tickets"
#
#



zmap_message_out "Finished processing of RT tickets"

exit 0
