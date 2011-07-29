#!/usr/local/bin/perl -w

use strict;
use warnings;

{
    my $block = {};

    sub process_line{
	my ($hash, $line) = @_;
	
	my @parts = split(/:/, $line, 2);

	my $key   = shift(@parts);
	my $value = "@parts";

	$value =~ s!^\s!!;
	$value =~ s!\s$!!;

	if($key eq 'id'){
	    my $id = 0;
	    if($block->{$key}){
		push(@{$hash->{$block->{'Queue'}}}, $block);
		$block = {};
	    }
	    if($value =~ m!ticket/(\d+)!){
		$id = $1;
		$block->{$key} = $id;
	    }
	}elsif($key ne ''){
	    $block->{$key} = $value;
	}
    }
}

if ($ARGV[0]){
    open(my $fh, $ARGV[0]) || die "Failed";
    my $hash = {};
    while(<$fh>){
	process_line($hash, $_);
    }
    close $fh;

    make_html($hash);
}


sub make_html
  {
  my ($hash) = @_;

  # We want zmap before acedb
  foreach my $q (sort { $b cmp $a } keys %$hash) {
    my $a = $hash->{$q};

    print qq{\n<fieldset><legend>$q</legend>\n\n<ul>\n};

    for (my $i = 0; $i < scalar(@$a); $i++) {
      my $block = $a->[$i];
      my $id  = $block->{'id'};
      my $queue = $block->{'Queue'};
      my $sbj = $block->{'Subject'};
      my $req = $block->{'Requestors'};
      my $own = $block->{'Owner'};
      my $res = $block->{'Resolved'};

      print qq{<li>$queue - Ticket No.};
      print qq{<a href="https://rt.sanger.ac.uk/rt/Ticket/Display.html?id=$id">$id</a>, Resolved ($res)\n};
      print qq{<p>$sbj</p>\n};
      print qq{<p>Requestor: $req</p>\n};
      print qq{<p>Owner: $own</p>\n};
      print qq{</li>\n};
      }

    print qq{</ul>\n};
    print qq{\n</fieldset><br />\n};
    }
  }

