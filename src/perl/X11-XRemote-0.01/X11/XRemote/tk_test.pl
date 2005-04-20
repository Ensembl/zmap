#!/usr/local/bin/perl -w

#!/nfs/team71/analysis/rds/tmp/DEBUG_PERL/bin/perl -w




use strict;
use lib '/nfs/team71/analysis/rds/workspace/tk';
use lib './PREFIX/lib/site_perl/5.6.1/alpha-dec_osf';
#use lib './PREFIX/lib/perl5/site_perl/5.8.0/alpha-dec_osf-thread-multi-ld';
#use lib './PREFIX/lib/perl5/site_perl/5.8.6/alpha-dec_osf-thread-multi';

# make a TK window
use CanvasWindow::MainWindow;
use CanvasWindow::XRemoteTest;

my $mw = CanvasWindow::MainWindow->new('X11 XRemote Testing');
my $tw = CanvasWindow::XRemoteTest->new($mw);

Tk::MainLoop();

