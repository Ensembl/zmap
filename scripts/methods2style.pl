#!/usr/local/bin/perl -w

use strict;
use warnings;
use lib 'PerlModules';
use Hum::Ace::MethodCollection;
use Getopt::Long;
use Data::Dumper;

{

    my $convert = { 
        width          => 'width',
        name           => 'name',
        show_up_strand => 'show_reverse',
        bumpable       => '',
        show_text      => '',
        coding         => '',
        non_coding     => '',
        color          => 'background',
        cds_color      => 'foreground',
        mutable        => '',
        right_priority => '',
        zone_number    => '',
        has_parent     => '',
        max_mag        => '',
        score_by_width => '',
        score_bounds   => '',
        column_name    => '',
        blixem_n       => '',
        blixem_x       => '',
        no_display     => '',
        gapped         => 'gapped_align',
    };

    sub Hum::Ace::Method::zmap_string{
        my ($self) = @_;
        my $txt = qq`Type\n{\n`;

        # in the case of each of the following the arrays are lists of
        # perl methods avaiable for the method.  These get translated 
        # to be the correct zmap properties for the type.

        foreach my $txtprop(qw(name color cds_color)){
            my $zmap  = $convert->{$txtprop} || next;
            my $value = $self->$txtprop()    || next;
            $txt .= qq`$zmap = "$value"\n`;
        }
        foreach my $numprop(qw(width)){
            my $zmap  = $convert->{$numprop} || next;
            my $value = $self->$numprop()    || next;
            $txt .= "$zmap = " . $self->$numprop . "\n";
        }
        foreach my $boolprop(qw()){
            my $zmap = $convert->{$boolprop} || next;
            my $TorF = ($self->$boolprop ? "true" : "false");
            $txt .= qq`$zmap = $TorF\n`;
        }

        $txt .= qq`}\n`;

        return $txt;
    }
}


{
    my $method_file = "methods.ace";

    my $usage = sub { exit(exec('perldoc', $0)) };

    GetOptions('file=s' => \$method_file,
               'help|h' => $usage) || $usage->();
    
    my $collection = Hum::Ace::MethodCollection->new_from_file($method_file);
    foreach my $meth(@{$collection->get_all_Methods}){
        print STDERR "Found Method " . $meth->name . "\n";
        print $meth->zmap_string;
    }
}

__END__

=pod

=head1 NAME

 methods2style.pl

=head1 DESCRIPTION

 currently  a simple  script to  parse a  methods.ace file  and output
 (STDOUT) a Style file to be useful for ZMap.

=head1 USAGE

 methods2style.pl -file <methods.ace> -help

 where <methods.ace> specifies full location of file to read.
 -help shows this pod.

=cut
