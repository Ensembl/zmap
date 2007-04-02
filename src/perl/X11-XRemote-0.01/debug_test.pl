#!/usr/local/bin/perl -w

use strict;
use warnings;
use X11::XRemote;

use Getopt::Long;


my $win = undef;
my $cmd = 'zoom_in';
my $deb = 1;

GetOptions('win=s' => \$win,
           'cmd=s' => \$cmd,
           'deb=i' => \$deb);

$win || warn "usage: $0 -win _ID_ \n";

my $conn = X11::XRemote->new(-server => 0,
                             -id     => $win,
                             -_DEBUG => $deb);
my $del  = X11::XRemote::delimiter();
my $fstr = X11::XRemote::format_string();

print "X11::XRemote delimiter is '$del'\n";
print "X11::XRemote format string is '$fstr'\n";

if($conn){
    print "We have a $conn\n";
    print "It will control window with id $win\n" if $win;
    my @reps = $conn->send_commands(($cmd) x 2);
    foreach my $rep(@reps){
        print "I got a reply of '$rep'\n";
    }
}else{
    print "Failed to make a new X11::XRemote Object\n";
}

print "Everything looked ok...\n";
