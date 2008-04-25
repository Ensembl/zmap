#!/usr/local/bin/perl -w

use strict;
use warnings;

use Getopt::Long;

my $nm    = 'nm';
my $zmap_location = 'zmap';
my $stack = '-';
my $usage = sub { exit(exec('perldoc', $0)) };

GetOptions(
	   'nm=s'    => \$nm,
	   'zmap=s'  => \$zmap_location,
	   'stack=s' => \$stack,
	   'help'    => $usage,
	   ) or $usage->();

my $symbol_address_to_name = produce_symbol_hash($nm, $zmap_location, {});

open(my $fh, "$stack") || die "Failed to open $stack";

while(<$fh>){
    if(/([^\(]*zmap)(.*)?\[(0x([0-9a-f]+))\]/){
	my $address = $3;
	my $saved   = $address;
	my $f_name  = '';
	while(!($f_name = $symbol_address_to_name->{$address})){
	    $address = spin_back($address);
	}
	print "$1 [$saved] $f_name ($address)\n";
    }else{
	print;
    }
}

close($fh);

sub spin_back{
    my $address = oct($_[0]);
    $address--;
    $address = sprintf("0x%x", $address);
    #print $address. "\n";
    return $address;
}

sub produce_symbol_hash{
    my ($nm, $zmap, $symbol_hash) = @_;
    my $cmd  = "$nm -ve $zmap";
    
    open(my $fh, "$cmd |") || die "Failed to run $cmd";
    
    while(<$fh>){
	my @line = split();
	if(@line == 3){
	    $line[0] =~ s/^0//;
	    #print "0x$line[0] - $line[2]\n";
	    $symbol_hash->{'0x'.$line[0]} = $line[2];
	}elsif(@line == 2){
	    # no symbol address... ignore
	}
    }
    
    close ($fh);

    return $symbol_hash;
}

=pod

=head1 process_stack_dump

    script   to  process   a  stack   dump  produced   by   GNU  glibc
    backtrace_symbols_fd function.

=head1 DESCRIPTION

    GNU  glibc provides  a function  to print  a stack  trace to  a fd
    (backtrace_symbols_fd).  This function attempts to lookup the name
    of  the function  at the  address in  the stack.   However,  it is
    unable to find static functions.  This is when this script helps.

=head1 USAGE

    process_stack_dump.pl [options]

    -nm     Path to nm program
    -zmap   Path to the zmap to proccess
    -stack  Path to file with stack dump in.

=cut
