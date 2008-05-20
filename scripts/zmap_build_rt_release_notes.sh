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

function set_cvs_prev_date
{
    if [ "x$1" != "x" ]; then
	local sep=$IFS
	IFS='/'
	set $1
	CVS_PREV_DATE="$3-$2-$1"
	IFS=$sep
    else
	zmap_message_exit "Failed to set RT_PREV_DATE"
    fi
}

function set_zmap_version_release_update_vars
{
    if [ "x$1" != "x" ]; then
	HEADER_FILE=$1
	if [ -f $HEADER_FILE ]; then
	    # grep the HEADER_FILE for #define ZMAP_VERSION 1
	    # set to split those and assign $3 to variable

	    ZMAP_VERSION=$(grep ZMAP_VERSION $HEADER_FILE)
	    set $ZMAP_VERSION
	    ZMAP_VERSION=$3	    
	    
	    ZMAP_RELEASE=$(grep ZMAP_RELEASE $HEADER_FILE)
	    set $ZMAP_RELEASE
	    ZMAP_RELEASE=$3

	    ZMAP_UPDATE=$(grep ZMAP_UPDATE $HEADER_FILE)
	    set $ZMAP_UPDATE
	    ZMAP_UPDATE=$3
	fi
    fi
}

CMDSTRING='[ -d<date> -n -z ] VAR=VALUE'
DESCSTRING=`cat <<DESC
   -d   specify date from which changes/tickets should be extracted,
        date must be in form "dd/mm/yyyy" (defaults to date in $ZMAP_RELEASE_NOTES_TIMESTAMP)

   -n   DO NOT put the release notes in cvs or update the date file there

   -z   do only zmap release notes (default is to include acedb etc. as well)

Command line variables:

ZMAP_RELEASE_NOTES_TIMESTAMP - file containing last release date [$ZMAP_RELEASE_NOTES_TIMESTAMP]
ZMAP_VERSION_HEADER - header containing the version #defines [$ZMAP_VERSION_HEADER]
ZMAP_WEBPAGE_HEADER - header containing the webpage #define [$ZMAP_WEBPAGE_HEADER]
DESC`

UPDATE_CVS=yes
UPDATE_HTML=yes
UPDATE_DATE=yes
UPDATE_DEFINE=yes
ZMAP_ONLY=no
RT_QUEUES="zmap acedb"

zmap_message_out "Start of script"

zmap_message_out "Running in $INITIAL_DIR on $(hostname)"

zmap_message_out "Parsing cmd line options: '$*'"


while getopts ":d:nz" opt ;
  do
  case $opt in
      d  ) RT_LAST_RUN=$OPTARG ;;
      n  ) UPDATE_CVS=no ;;
      z  ) ZMAP_ONLY=yes ;;
      \? ) 
zmap_message_exit "Bad command line flag

Usage:

$0 $CMDSTRING

$DESCSTRING"
;;
  esac
done

shift $(($OPTIND - 1))

# including VARIABLE=VALUE settings from command line
if [ $# -gt 0 ]; then
    eval "$*"
fi


# We know that this is in cvs
zmap_cd $BASE_DIR

# We can then go to the correct place
zmap_goto_cvs_module_root

# Get the path of some files
ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP=$(find . -name $ZMAP_RELEASE_NOTES_TIMESTAMP | grep -v CVS)
ZMAP_PATH_TO_RELEASE_NOTES_HTML_DIR=$(find . -name 'Release_notes' -type d | grep -v CVS)
ZMAP_PATH_TO_VERSION_HEADER=$(find . -name $ZMAP_VERSION_HEADER | grep -v CVS)
ZMAP_PATH_TO_WEBPAGE_HEADER=$(find . -name $ZMAP_WEBPAGE_HEADER | grep -v CVS)

zmap_message_out '$ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP =' $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP
zmap_message_out '$ZMAP_PATH_TO_RELEASE_NOTES_HTML_DIR =' $ZMAP_PATH_TO_RELEASE_NOTES_HTML_DIR
zmap_message_out '$ZMAP_PATH_TO_VERSION_HEADER =' $ZMAP_PATH_TO_VERSION_HEADER
zmap_message_out '$ZMAP_PATH_TO_WEBPAGE_HEADER =' $ZMAP_PATH_TO_WEBPAGE_HEADER

# Set all the dates that we need
# First the previous date!
[ "x$RT_LAST_RUN" != "x" ] || RT_LAST_RUN=$(cat $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP) 
[ "x$RT_LAST_RUN" != "x" ] || zmap_message_exit $(hostname) "From date not set."

RT_PREV_DATE=$RT_LAST_RUN

RT_TODAY=$(date "+%d/%m/%Y")
HUMAN_TODAY=$(date "+%e %B %Y")

set_cvs_prev_date $RT_PREV_DATE

CVS_TOMORROW=$(date -d"tomorrow" "+%Y-%m-%d")
CVS_DATE="$CVS_PREV_DATE<$CVS_TOMORROW"
CVS_YEAR=$(date "+%Y")
CVS_YEAR='\d{4}-\d{2}-\d{2}'

FILE_DATE=$(date "+%Y_%m_%d")

zmap_message_out '$RT_TODAY ='     $RT_TODAY
zmap_message_out '$HUMAN_TODAY ='  $HUMAN_TODAY
zmap_message_out '$RT_LAST_RUN ='  $RT_LAST_RUN
zmap_message_out '$CVS_TOMORROW =' $CVS_TOMORROW
zmap_message_out '$CVS_DATE ='     $CVS_DATE

RELEASE_NOTES_OUTPUT="${ZMAP_RELEASE_FILE_PREFIX}.${FILE_DATE}.${ZMAP_RELEASE_FILE_SUFFIX}"

zmap_message_out "Using $RELEASE_NOTES_OUTPUT as release notes output file."

rm -f $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot rm $RELEASE_NOTES_OUTPUT file."
touch $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot touch $RELEASE_NOTES_OUTPUT file."
chmod u+rw $RELEASE_NOTES_OUTPUT || zmap_message_exit "Cannot make $RELEASE_NOTES_OUTPUT r/w"

if [ "x$ZMAP_ONLY" == "xyes" ]; then
    RT_QUEUES="zmap"
fi

if [ "x$RT_LAST_RUN" == "x$RT_TODAY" ]; then
    zmap_message_exit "Not enough time has passed since $RT_LAST_RUN. Today is $RT_TODAY"
fi

set_zmap_version_release_update_vars $ZMAP_PATH_TO_VERSION_HEADER


#
# Write the proper header for sanger pages
# oops some stuff to construct here...the date !!
#
cat >> $RELEASE_NOTES_OUTPUT <<EOF
<!-- File automatically generated. Do not edit. --!>

<!--#set var="banner" value="ZMap Release Notes For '$HUMAN_TODAY'" -->
<!--#include virtual="/perl/header" -->
<!--#set var="author" value="edgrif@sanger.ac.uk" -->

<!-- The release version, etc... --!>

<h5>Release Version: ZMap $ZMAP_VERSION.$ZMAP_RELEASE.$ZMAP_UPDATE</h5>


<h5>Release Date: $HUMAN_TODAY</h5>

<p>
(Last release was on $RT_PREV_DATE)
</p>

<h5>Request Tracker Tickets Resolved</h5>
<br />
<!-- Now the resolved tickets... --!>

EOF


RTSERVER=scratchy
RTHTTPSERVER="https://rt.sanger.ac.uk/rt"
RTUSER=zmap
RTRESULTS=rt_tickets.out
RTERROR=rt_error.log
RTFAILED=no

zmap_message_out "pwd =" $(pwd)
zmap_message_out "ssh $RTSERVER to get the request tracker tickets that have been resolved."


# This looks complicated, but the basic method of action
# is cat stdin (The HERE doc) and the getRTtickets over
# the ssh connection to the bash "one-liner" to execute 
# it there.  Why? so that we don't have to checkout zmap
# on scratchy, nor do we need to have getRTtickets in a
# central place where it must always be cvs up-to-date.
# This way it gets sucked from this directory which is 
# either the cvs up-to-date checkout in the build that's
# going on, or the development version if it needs to
# change.  

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
	zmap_message_err $error
    fi
fi

zmap_message_out "Finished getting Request tracker tickets"

cat >> $RELEASE_NOTES_OUTPUT <<EOF
<!-- Finished getting the resolved request tracker tickets --!>
EOF

zmap_message_out "Processing RT tickets"
# This goes directly into the html file
$BASE_DIR/process_rt_tickets_file.pl $RTRESULTS >> $RELEASE_NOTES_OUTPUT || \
    zmap_message_exit "Failed processing RT tickets"

cat >> $RELEASE_NOTES_OUTPUT <<EOF

<!-- End of tickets  --!>

<fieldset>
<legend>ZMap Changes/Fixes [from cvs]</legend>

EOF

TMP_CHANGES_FILE=zmap.changefile
zmap_message_out "Getting zmap changes into '$TMP_CHANGES_FILE'"

$BASE_DIR/zmap_cvs_changes.sh -o$TMP_CHANGES_FILE -z -d $CVS_DATE -t . || \
    zmap_message_exit "cvschanges for zmap didn't complete"

zmap_message_out "Finished getting zmap changes"

cat >> $RELEASE_NOTES_OUTPUT <<EOF
<ul>
  <li><!-- --- Start editing ZMap changes here... --- --!>
    <a href="http://cvs.internal.sanger.ac.uk/cgi-bin/viewcvs.cgi/ZMap/?root=zmap">Browse CVS</a>
EOF

cat $TMP_CHANGES_FILE > /dev/null || zmap_message_exit "$TMP_CHANGES_FILE doesn't exist!"

zmap_message_out "Processing cvs changes..."

# process using perl one-liner
perl -lne "s!.*\*!  </li>\n  <li>!; print if !/$CVS_YEAR/" $TMP_CHANGES_FILE >> $RELEASE_NOTES_OUTPUT

rm -f $TMP_CHANGES_FILE  || zmap_message_exit "Couldn't remove $TMP_CHANGES_FILE! Odd considering rm -f use."

cat >> $RELEASE_NOTES_OUTPUT <<EOF
    <!-- --- End editing ZMap changes here... --- --!>
  </li>
</ul>
</fieldset>
<br />
EOF

if [ "x$ZMAP_ONLY" != "xyes" ]; then

    cat >> $RELEASE_NOTES_OUTPUT <<EOF
<fieldset>
<legend>Aceb Changes/Fixes [from cvs]</legend>
EOF

    TMP_CHANGES_FILE=acedb.changefile
    zmap_message_out "Getting acedb changes into '$TMP_CHANGES_FILE'"

    $BASE_DIR/zmap_cvs_changes.sh -o$TMP_CHANGES_FILE -a -d$CVS_DATE -t $ZMAP_ACEDB_RELEASE_CONTAINER/RELEASE.DEVELOPMENT.BUILD || \
	zmap_message_exit "cvschanges for acedb didn't complete"

    zmap_message_out "Finished getting acedb changes"
    
    cat >> $RELEASE_NOTES_OUTPUT <<EOF
<ul>
  <li><!-- --- Start editing ACeDB changes here... --- --!>
    <a href="http://cvs.internal.sanger.ac.uk/cgi-bin/viewcvs.cgi/acedb/?root=acedb">Browse CVS</a>
EOF

    cat $TMP_CHANGES_FILE > /dev/null || zmap_message_exit "$TMP_CHANGES_FILE doesn't exist!"
    
    zmap_message_out "Processing cvs changes..."

    # process using perl one-liner
    perl -lne "s!.*\*!  </li>\n  <li>!; print if !/$CVS_YEAR/" $TMP_CHANGES_FILE >> $RELEASE_NOTES_OUTPUT

    rm -f $TMP_CHANGES_FILE  || zmap_message_exit "Couldn't remove $TMP_CHANGES_FILE! Odd considering rm -f use."

    cat >> $RELEASE_NOTES_OUTPUT <<EOF
    <!-- --- End editing ACeDB changes here... --- --!>
  </li>
</ul>
</fieldset>
<br />
EOF

# end of ZMAP_ONLY=yes section
fi

cat >> $RELEASE_NOTES_OUTPUT <<EOF

<!--#include virtual="/perl/footer" -->

EOF


# 
# End of generation of the HTML file 
# 

# save this for the #define replacing...
RELEASE_FILE=$RELEASE_NOTES_OUTPUT

zmap_message_out $(pwd)

if [ "x$UPDATE_HTML" == "xyes" ]; then
    
    mv $RELEASE_NOTES_OUTPUT $ZMAP_PATH_TO_RELEASE_NOTES_HTML_DIR/ || \
	zmap_message_exit "Failed to move the html file to the release_notes dir"
    
    RELEASE_NOTES_OUTPUT=$ZMAP_PATH_TO_RELEASE_NOTES_HTML_DIR/$RELEASE_NOTES_OUTPUT

    zmap_message_out cvs commit -m "new zmap release notes for $HUMAN_TODAY" $RELEASE_NOTES_OUTPUT 

    if [ "x$UPDATE_CVS" == "xyes" ]; then
	zmap_message_out "updating html in cvs..."

	# ask cvs if it knows of the file
	# cvs_unknown will have data if nothing is known.
	cvs_unknown=$(cvs status $RELEASE_NOTES_OUTPUT 2>/dev/null | grep Status | grep Unknown)
	if [ "x$cvs_unknown" != "x" ]; then
	    zmap_message_out "Need to cvs add $RELEASE_NOTES_OUTPUT"
	    cvs add $RELEASE_NOTES_OUTPUT || \
		zmap_message_exit "cvs add failed"
	fi

	cvs commit -m "new zmap release notes for $HUMAN_TODAY" $RELEASE_NOTES_OUTPUT || \
	    zmap_message_exit "cvs commit failed for $RELEASE_NOTES_OUTPUT"
    fi
    
fi

if [ "x$UPDATE_DEFINE" == "xyes" ]; then
    
    cvs edit $ZMAP_PATH_TO_WEBPAGE_HEADER || zmap_message_exit "failed to cvs edit $ZMAP_PATH_TO_WEBPAGE_HEADER"
    perl -i -lne 's!((.*)(ZMAPWEB_RELEASE_NOTES\s)(.*))!$2$3"'$RELEASE_FILE'"!; print ;' $ZMAP_PATH_TO_WEBPAGE_HEADER || \
	zmap_message_exit "failed to edit webpages header"

    zmap_message_out cvs commit -m "update release notes file to $RELEASE_FILE" $ZMAP_PATH_TO_WEBPAGE_HEADER
    if [ "x$UPDATE_CVS" == "xyes" ]; then
	zmap_message_out "updating webpages header in cvs..."
	cvs commit -m "update release notes file to $RELEASE_FILE" $ZMAP_PATH_TO_WEBPAGE_HEADER || \
	    zmap_message_exit "cvs commit failed for $ZMAP_PATH_TO_WEBPAGE_HEADER"
    fi
fi

if [ "x$UPDATE_DATE" == "xyes" ]; then

    cvs edit $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP || \
	zmap_message_exit "failed to cvs edit $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP"

    echo $RT_TODAY > $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP || \
	zmap_message_exit "Failed to write to last release file"

    zmap_message_out cvs commit -m "update last release notes to $RT_TODAY" $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP

    if [ "x$UPDATE_CVS" == "xyes" ]; then
	zmap_message_out "updating the last run date in cvs..."
    	cvs commit -m "update last release notes to $RT_TODAY" $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP || \
	    zmap_message_exit "cvs commit failed for $ZMAP_PATH_TO_RELEASE_NOTES_TIMESTAMP"
    fi
    
fi


zmap_message_out "Reached end of script"
exit 0;
