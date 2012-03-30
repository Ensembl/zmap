#!/usr/bin/env perl

use Getopt::Long;

{
    my $method_file = "";
    my $usage = sub { exit(exec('perldoc', $0)) };

    GetOptions('file=s' => \$method_file,
               'help|h' => $usage) || $usage->();

    open (file, $method_file) || die $!;
    my @lines = <file>;

    foreach (@lines) {
      my $orig_line = $_;
      my $new_line = $orig_line;
      $new_line =~ s/^Method : \"(.*)\".*$/Style \"$1\"/;
      print $orig_line;

      if ($new_line ne $orig_line) {
        print $new_line;
      }
    }
}

__END__

=pod

=head1 NAME

 add_style_tag.pl

=head1 DESCRIPTION

 currently  a simple script to parse a .ace file containing methods
 and output (STDOUT) the same file but adding a Style tag to each Method
 class.

=head1 USAGE

 methods_add_style_tag.pl -file <methods.ace> -help

 where <methods.ace> specifies full location of file to read
 -help shows this pod.


