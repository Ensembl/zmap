#!/usr/bin/perl -w
#
# Script to put supplied dev string into the zmapUtils_P.h header
# where it will be picked up and displayed by the zmap code.
#
#
#
#

use strict;
use warnings;


$_ = @ARGV;

if ($_ != 2)
  {
    die "Usage: prog <version_file> <dev_string>\n" ;
  }

my $file = $ARGV[0] ;
my $new_file = "${file}.new";
my $pattern = "ZMAP_DEVELOPMENT_ID" ;
my $dev_string = $ARGV[1] ;


open(my $fhIN, "< $file") or die "ERROR $0: Couldn't open version file '$file' : $! \n";
open(my $fhOUT, "> $new_file") or die "ERROR $0: Couldn't open new version file '$file': $! \n";

while (<$fhIN>) {

  # look for ZMAP_DEVELOPMENT_ID and add git branch/version.
  if (/$pattern/)
    {
    print $fhOUT "#define $pattern \"$dev_string\"\n" ;
    }
  else
    {
    print $fhOUT $_;
    }

  }

close $fhIN;
close $fhOUT;


# Replace old version file with new one.
unlink $file;
rename $new_file, $file;

exit 0 ;
