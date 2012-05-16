use strict;
use warnings;

use ExtUtils::MakeMaker;
#use ExtUtils::PkgConfig; # better version than below, no doubt, swap at some time.
use File::Copy;
use Cwd;

use Data::Dumper;
use Getopt::Long;


{ # scope these

my $opt = {
    help                => \&usage,
    pkgconfig           => 'pkg-config',
    'with-x-libs'       => '/usr/X11R6/lib',
    'with-x-inc'        => '/usr/X11R6/include',
    'with-symbols'      => 1,
    'with-zmap-libs'    => 'lib',
    'with-zmap-inc'     => 'include',
    'with-macosx-sdk-path' => 0,
    };

Getopt::Long::GetOptions(
                         $opt, 
                         "help",
                         "pkgconfig=s",
                         "with-x-libs=s",
                         "with-x-inc=s",
                         "with-zmap-libs=s",
                         "with-zmap-inc=s",
                         "with-symbols",
                         "with-macosx-sdk-path=s",
                         ) || die usage();

my @libraries = qw(-lX11 
                   -lZMap
                   -lZMapUtils
                   );
my @includes  = qw();

configure($opt, \@includes, \@libraries);

ExtUtils::MakeMaker::WriteMakefile(
    'NAME'		    => 'X11::XRemote',
    'VERSION_FROM'	=> 'lib/X11/XRemote.pm', 
    'PREREQ_PM'		=> {}, # e.g., Module::Name => 1.1
    ($] >= 5.005 ?    
     ('ABSTRACT_FROM' => 'lib/X11/XRemote.pm', 
      'AUTHOR'        => 'ZMap developers <zmap@sanger.ac.uk>'
      ) : ()
     ),
    'LIBS'		=> join(' ', @libraries),
    'DEFINE'		=> '-DHAVE_CONFIG_H -DXTHREADS',
    'INC'		=> join(' ', @includes),
    'OBJECT'		=> '$(O_FILES)', # link all the C files too
);

#end scope
}

#===========================================================================
# Name:    usage
# Purpose: Print Usage & exit with error status.
#===========================================================================
sub usage{
    print STDERR <<"MESSAGE";
usage: perl $0 [options]

available options:
    --pkconfig <path>       location of pkgconfig
    --with-x-libs <path>    location of X11 libraries
    --with-x-inc <path>     location of X11 includes
    --with-zmap-libs <path> location of ZMap libraries
    --with-zmap-inc <path>  location of ZMap includes
    --with-symbols          compile with -g

    --help                  Prints this help message...

For further info see one of INSTALL, INSTALL.pod or INSTALL.html.

MESSAGE
exit(1);
}

#===========================================================================
# Name:    configure
# Purpose: work out includes and libraries
#===========================================================================
sub configure{
    my ($opt, $inc, $lib) = @_;

    #my $pkgconfig_query = "glib-2.0";
    #my $pkgconfig_query = "gdk-2.0 atk";
    my $pkgconfig_query = "gtk+-2.0";

    if(exists($opt->{'pkgconfig'}) && $pkgconfig_query){
        print STDERR "using " . $opt->{'pkgconfig'} . "\n";
        print STDERR "looking for $pkgconfig_query libraries\n";
        my $out1 = get_output($opt->{'pkgconfig'} . " --libs $pkgconfig_query");
        unshift(@$lib, $out1);

        print STDERR "looking for $pkgconfig_query cflags\n";
        my $out2 = get_output($opt->{'pkgconfig'} . " --cflags $pkgconfig_query");
        unshift(@$inc, $out2);
        
    }
    if(exists($opt->{'with-x-libs'})){
        unshift(@$lib, '-L' . $opt->{'with-x-libs'});
    }
    if(exists($opt->{'with-x-inc'})){
        unshift(@$inc, '-I' . $opt->{'with-x-inc'});
    }
    if(exists($opt->{'with-zmap-libs'})){
        unshift(@$lib, '-L' . $opt->{'with-zmap-libs'});
    }else{
        warn "Without specifying --with-zmap-libs= you probably won't find the -lZMap.\n";
        #unshift(@$lib, '-L../../../../build/alpha/lib');
    }
    if(exists($opt->{'with-zmap-inc'})){
        unshift(@$inc, '-I' . $opt->{'with-zmap-inc'});
    }else{
        unshift(@$inc, '-I../../../../include'); # default, should be in ZMap checkout
    }
    if(exists($opt->{'with-symbols'})){
        unshift(@$inc, '-g');
    }
    if(exists($opt->{'with-macosx-sdk-path'})){
	if(-d $opt->{'with-macosx-sdk-path'}){
	    unshift(@$inc, '-arch ppc');
	    unshift(@$inc, '-arch i386');
	    unshift(@$inc, '-isysroot '. $opt->{'with-macosx-sdk-path'});
	    unshift(@$inc, '-Wl,-undefined,dynamic_lookup');
	}   
    }
}

sub get_output{
    my ($cmd) = @_;

    local *PIPE;
    my $str;
    eval{
        open(PIPE, "$cmd 2>&1 |") or die "Failed to open command: $cmd : $!";
        my @out;
        while (<PIPE>){
            chomp;
            push @out, $_;
        }
        close PIPE or die "Error (exit $?) running '$cmd' : ",
            map("$_\n", @out);
        $str = join " ", @out;
    };
    if ($@) {
        warn $@;
        return "";
    } else {
        warn "$cmd = $str";
        return $str;
    }
}



package MY; # so that "SUPER" works right

sub clean {
    my $inherited = shift->SUPER::clean(@_);
    $inherited . "\trm -f XRemote.c\n\trm -rf PREFIX/\n\tmkdir PREFIX\n";
}
