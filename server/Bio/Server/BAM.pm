

package Bio::Server::BAM;

use strict;
use warnings;

use Carp;

sub new {
    my ($pkg, @args) = @_;
    return bless { @args }, $pkg;
}

sub features {
    my ($self, $seq_id, $start, $end) = @_;

    my ( $sam ) =
        @{$self}{qw( -sam )};

	# why copy these? look at Sam.pm

    my $features = [ $sam->features(
            -type   => 'match',
            -seq_id => $seq_id,
            -start  => $start,
            -end    => $end,
        )
	];

    return $features;
}

1;

__END__

=head1 NAME - Bio::Server::BAM

=head1 AUTHOR

Ana Code B<email> anacode@sanger.ac.uk

