#!/bin/csh
# 
# Simply utility script to copy the contents of one directory to another.
# This is provided so that we can easily perform a copy to our project area
# on /software via an ssh call.
#

set msg_prefix="(`basename $0` on `hostname`)"

if ($#argv < 2 ) then
    echo "$msg_prefix Error: invalid number of arguments"
    exit 1
endif

set source_dir=$1
set dest_dir=$2
    
if (-d $source_dir && -d $dest_dir ) then
    cp -r $source_dir/* $dest_dir
else
    echo "$msg_prefix Warning: error copying contents of '$source_dir' to '$dest_dir'"
    exit 1
endif

exit 0
