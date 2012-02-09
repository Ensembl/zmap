#!/bin/bash

########################
# Find out where BASE is
SCRIPT_NAME=$(basename $0)
INITIAL_DIR=$(pwd)
 SCRIPT_DIR=$(dirname $0)
   BASE_DIR=$INITIAL_DIR/$SCRIPT_DIR

. $BASE_DIR/zmap_functions.sh

# Name of master X11 performance file
MASTER_X11PERF_FILE='/nfs/team71/acedb/zmap/PERFORMANCE/X11PERF_REFERENCE.txt'
TMP_X11PERF_FILE='x11perf_tmp.txt'


# name of the output file
MACHINE_INFO_FILE="machine_info.txt"



# message to user to say hi

zmap_message_out "Creating machine info file..."

# create the output file
zmap_create_file $MACHINE_INFO_FILE

# hostname
this_host=`hostname`
zmap_message_file $MACHINE_INFO_FILE "ZMap machine info file for : $this_host"

# OS etc...
zmap_message_file $MACHINE_INFO_FILE "Kernel/Arch/OS:"
zmap_message_file $MACHINE_INFO_FILE "   " `uname -a`

# CPU (important bits)
zmap_message_file $MACHINE_INFO_FILE "CPU:"
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/cpuinfo | grep -i 'model name'`
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/cpuinfo | grep -i 'mhz'`
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/cpuinfo | grep -i 'cache size'`
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/cpuinfo | grep -i 'bogomips'`

# Memory (important bits)
zmap_message_file $MACHINE_INFO_FILE "Memory:"
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/meminfo | grep -i 'memtotal'`
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/meminfo | grep -i 'memfree'`
zmap_message_file $MACHINE_INFO_FILE "   " `cat /proc/meminfo | grep -i 'vmalloc'`

# This user's processes
zmap_message_file $MACHINE_INFO_FILE "Processes:"
ps uwx >>$MACHINE_INFO_FILE

# How long has this machine been on?
# What's its load average?
zmap_message_file $MACHINE_INFO_FILE "Lifestyle:"
zmap_message_file $MACHINE_INFO_FILE "   " `uptime`


# Do some x11 testing
#
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -rect10`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -seg10`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -line500`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -f8text`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -scroll10`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -copypixwin500`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -putimagexy500`
zmap_message_file $TMP_X11PERF_FILE "   " `x11perf -move`

x11perfcomp -r $MASTER_X11PERF_FILE $TMP_X11PERF_FILE >>$MACHINE_INFO_FILE



# finished... Now cat the output to terminal

zmap_message_out "Finished creating file."
zmap_message_out $MACHINE_INFO_FILE "contains:"
zmap_message_out "--------------------------------------------------------------"

cat $MACHINE_INFO_FILE

zmap_message_out "--------------------------------------------------------------"

zmap_message_out "Mailing file to zmap developers"

mailx -s "[$SCRIPT_NAME] $this_host machine info" annotools@sanger.ac.uk < $MACHINE_INFO_FILE || zmap_message_exit "Failed to mail information to developers"

rm -f $MACHINE_INFO_FILE $TMP_X11PERF_FILE
