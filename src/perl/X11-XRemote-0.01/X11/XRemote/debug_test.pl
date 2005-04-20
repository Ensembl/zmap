#!/usr/local/bin/perl -w

use strict;
#use lib './PREFIX/lib/perl5/site_perl/5.6.1/i386-linux';
#use lib './PREFIX/lib/perl5/site_perl/5.8.0/alpha-dec_osf-thread-multi-ld';
use lib './PREFIX/lib/site_perl/5.6.1/alpha-dec_osf';
#use lib './PREFIX/lib/perl5/site_perl/5.6.1/alpha-dec_osf';
use X11::XRemote;
use Getopt::Long;


my $win = undef;
my $cmd = 'zoom_in';
GetOptions('win=s' => \$win,
           'cmd=s' => \$cmd);
$win || warn "usage: $0 -win _ID_     \n";
my $conn = X11::XRemote->new(-server => 0, -id => $win);

print "delimiter should be '" . X11::XRemote::delimiter() . "'\n";
print "We have a $conn with '$win' we should format with string: '".$conn->format_string."'\n";

my @replies = $conn->send_commands(($cmd) x 2);


foreach my $rep(@replies){
    print "I got a reply of '$rep'\n";
}


