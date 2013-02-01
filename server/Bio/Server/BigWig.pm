

package Bio::Server::BigWig;

use strict;
use warnings;

use Carp;

my $bin_size = 40;

sub new {
    my ($pkg, @args) = @_;
    return bless { @args }, $pkg;
}

sub features {
    my ($self, $seq_id, $start, $end) = @_;

    my ( $bigwig ) =
        @{$self}{ qw( -bigwig ) };

    my $size = ($end + 1 - $start);
    my $bin_count = $size / $bin_size;
    return unless $bin_count;

    my ($summary) =
        $bigwig->features(
            -type   => 'summary',
            -seq_id => $seq_id,
            -start  => $start,
            -end    => $end,
        );

    my $features = [ ];
    my $index = 0;
    for my $bin ( @{$summary->statistical_summary($bin_count)} ) {
        if ($bin->{validCount} > 0) {
            my $score = $bin->{sumData};
            my $feature_start =
                $start + ( $index * ( $end + 1 - $start ) ) / $bin_count;
            my $feature_end = 
                ( $start + ( ( $index + 1 ) * ( $end + 1 - $start ) ) / $bin_count ) - 1;
            my $feature = {
                start  => $feature_start,
                end    => $feature_end,
                score  => $score,
            };
            push @{$features}, bless $feature, "Bio::Server::BigWig::Feature";
        }
        $index++;
    }

    return $features;
}

package Bio::Server::BigWig::Feature; ## no critic (Modules::ProhibitMultiplePackages)

sub start  { return shift->{start};  }
sub end    { return shift->{end};    }
sub score  { return shift->{score};  }

1;

__END__

=head1 NAME - Bio::Server::BigWig

=head1 AUTHOR

Ana Code B<email> anacode@sanger.ac.uk

