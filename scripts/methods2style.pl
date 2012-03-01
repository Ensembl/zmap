#!/usr/local/bin/perl -w

use strict;
use warnings;
use lib '/software/anacode/otter/otter_production_main/PerlModules';
use Hum::Ace::MethodCollection;
use Getopt::Long;
use Data::Dumper;

{

    my $keyword_convert = { 
        name           => 'name',
        width          => 'width',
	remark         => 'description',
        show_up_strand => 'show_reverse_strand',
        frame_sensitive => 'frame_specific',
        strand_sensitive => 'strand_specific',
        bumpable       => 'overlap',
        color          => 'fill',
        cds_color      => '',
        no_display     => 'never_display',
#        show_text      => '',
#        coding         => '',
#        non_coding     => '',
#        mutable        => '',
#        right_priority => '',
#        zone_number    => '',
#        has_parent     => '',
#        max_mag        => '',
#        score_by_width => '',
#        score_bounds   => '',
#        column_name    => '',
#        blixem_n       => '',
#        blixem_x       => '',
#        gapped         => '',
    };

    my $colour_convert = {
      white => '#ffffff',
      black => '#000000',
      lightgray => '#c8c8c8',
      darkgray => '#646464',
      red => '#ff0000',
      green => '#00ff00',
      blue => '#0000ff',
      yellow => '#ffff00',
      cyan => '#00ffff',
      magenta => '#ff00ff',
      lightred => '#ffa0a0',
      lightgreen => '#a0ffa0',
      lightblue => '#a0c8ff',
      darkred => '#af0000',
      darkgreen => '#00af00',
      darkblue => '#0000af',
      palered => '#ffe6d2',
      palegreen => '#d2ffd2',
      paleblue => '#d2ebff',
      paleyellow => '#ffffc8',
      palecyan => '#c8ffff',
      palemagenta => '#ffc8ff',
      brown => '#a05000',
      orange => '#ff8000',
      paleorange => '#ffdc6e',
      purple => '#c000ff',
      violet => '#c8aaff',
      paleviolet => '#ebd7ff',
      gray => '#969696',
      palegray => '#ebebeb',
      cerise => '#ff0080',
      midblue => '#56b2de',
    } ;


    my $ACEDB_MAG_FACTOR = 8.0 ;



    sub Hum::Ace::Method::zmap_string{

        my ($self) = @_;

#        my $txt = qq`Type\n{\n`;
	my $txt ;

        # in the case of each of the following the arrays are lists of
        # perl methods avaiable for the method.  These get translated 
        # to be the correct zmap properties for the type.


        foreach my $txtprop(qw(name)){

            my $zmap  = $keyword_convert->{$txtprop} || next;

            my $value = lc($self->$txtprop())    || next;
            $txt .= qq`[$value]\n`;

            if ($self->name =~ m/halfwise/i || $self->name =~ m/genscan/i
                || $self->name =~ m/retained_intron/i || $self->name =~ m/cds/i
                || $self->name =~ m/estgene/i || $self->name =~ m/ensembl/i  )
	      {
	      $txt .= qq`mode = transcript\n`;
	      }
	    elsif ($self->name =~ m/trembl/i || $self->name =~ m/swissprot/i
                   || $self->name =~ m/vertebrate_rna/i || $self->name =~ m/refseq/i)
	      {
	      $txt .= qq`mode = alignment\n`;
	      }
	    elsif ($self->name =~ m/blast/i)
	      {
	      $txt .= qq`mode = alignment\n`;
	      }
	    elsif ($self->name =~ m/blast/i)
	      {
	      $txt .= qq`mode = alignment\n`;
	      }
            elsif ($self->name =~ m/ditags/i)
	      {
	      $txt .= qq`mode = alignment\n`;
	      }
            elsif ($self->name =~ m/est/i)
	      {
	      $txt .= qq`mode = alignment\n`;
	      }
            elsif ($self->name =~ m/coding/i)
	      {
	      $txt .= qq`mode = transcript\n`;
	      }
            elsif ($self->name =~ m/curated/i)
	      {
	      $txt .= qq`mode = transcript\n`;
	      }
            elsif ($self->name =~ m/transcript/i)
	      {
	      $txt .= qq`mode = transcript\n`;
	      }
            else
	      {
	      $txt .= qq`mode = basic\n`;
	      }

        }

#        foreach my $txtprop(qw(strand_sensitive frame_sensitive show_up_strand)){
#            my $zmap  = $keyword_convert->{$txtprop} || next;
#            $txt .= qq`$zmap = true\n`;
#        }

        foreach my $txtprop(qw(remark)){
            my $zmap  = $keyword_convert->{$txtprop} || next;
            my $value = $self->$txtprop()    || next;
            $txt .= qq`$zmap = $value\n`;
        }

        foreach my $txtprop(qw(color cds_color)){
            my $zmap  = $keyword_convert->{$txtprop} || next;
            my $value = $colour_convert->{lc($self->$txtprop())}    || next;
            $txt .= qq`$zmap = $value\n`;
	    $txt .= qq`border = #000000\n`;
        }

        foreach my $numprop(qw(width)){
            my $zmap  = $keyword_convert->{$numprop} || next;
            my $value = $self->$numprop()    || next;
            $txt .= "$zmap = " . ($self->$numprop * $ACEDB_MAG_FACTOR) . "\n";
        }

        foreach my $boolprop(qw(strand_sensitive frame_sensitive show_up_strand)){
            my $zmap = $keyword_convert->{$boolprop} || next;
#            my $TorF = ($self->$boolprop ? "true" : "false");
#            $txt .= qq`$zmap = $TorF\n`;

	    if ($self->$boolprop)
	      {
	      $txt .= qq`$zmap = true\n`;
	      }
        }

        # Hack, stuff mode in for now...
        $txt .= qq`overlap = complete\n`;

#        $txt .= qq`}\n`;
        $txt .= qq`\n`;

        return $txt;
    }


    # same as zmap_string but output in .ace format rather than ini-like format 
    sub Hum::Ace::Method::zmap_string_ace{

        my ($self) = @_;

#        my $txt = qq`Type\n{\n`;
	my $txt ;

        # in the case of each of the following the arrays are lists of
        # perl methods avaiable for the method.  These get translated 
        # to be the correct zmap properties for the type.


        foreach my $txtprop(qw(name)){

            my $zmap  = $keyword_convert->{$txtprop} || next;

            my $value = lc($self->$txtprop())    || next;
            $txt .= qq`ZMap_Style : "$value"\n`;

            if ($self->name =~ m/halfwise/i || $self->name =~ m/genscan/i
                || $self->name =~ m/retained_intron/i || $self->name =~ m/cds/i
                || $self->name =~ m/estgene/i || $self->name =~ m/ensembl/i  )
	      {
	      $txt .= qq`mode "transcript"\n`;
	      }
	    elsif ($self->name =~ m/trembl/i || $self->name =~ m/swissprot/i
                   || $self->name =~ m/vertebrate_rna/i || $self->name =~ m/refseq/i)
	      {
	      $txt .= qq`mode "alignment"\n`;
	      }
	    elsif ($self->name =~ m/blast/i)
	      {
	      $txt .= qq`mode "alignment"\n`;
	      }
	    elsif ($self->name =~ m/blast/i)
	      {
	      $txt .= qq`mode "alignment"\n`;
	      }
            elsif ($self->name =~ m/ditags/i)
	      {
	      $txt .= qq`mode "alignment"\n`;
	      }
            elsif ($self->name =~ m/est/i)
	      {
	      $txt .= qq`mode "alignment"\n`;
	      }
            elsif ($self->name =~ m/coding/i)
	      {
	      $txt .= qq`mode "transcript"\n`;
	      }
            elsif ($self->name =~ m/curated/i)
	      {
	      $txt .= qq`mode "transcript"\n`;
	      }
            elsif ($self->name =~ m/transcript/i)
	      {
	      $txt .= qq`mode "transcript"\n`;
	      }
            else
	      {
	      $txt .= qq`mode "basic"\n`;
	      }

        }

#        foreach my $txtprop(qw(strand_sensitive frame_sensitive show_up_strand)){
#            my $zmap  = $keyword_convert->{$txtprop} || next;
#            $txt .= qq`$zmap = true\n`;
#        }

        foreach my $txtprop(qw(remark)){
            my $zmap  = $keyword_convert->{$txtprop} || next;
            my $value = $self->$txtprop()    || next;
            $txt .= qq`$zmap "$value"\n`;
        }

        foreach my $txtprop(qw(color cds_color)){
            my $zmap  = $keyword_convert->{$txtprop} || next;
            my $value = $colour_convert->{lc($self->$txtprop())}    || next;
            $txt .= qq`colours normal $zmap "$value"\n`;
	    $txt .= qq`colours normal border "#000000"\n`;
        }

        foreach my $numprop(qw(width)){
            my $zmap  = $keyword_convert->{$numprop} || next;
            my $value = $self->$numprop()    || next;
            $txt .= "$zmap " . ($self->$numprop * $ACEDB_MAG_FACTOR) . "\n";
        }

        foreach my $boolprop(qw(strand_sensitive frame_sensitive show_up_strand)){
            my $zmap = $boolprop || next;
#            my $TorF = ($self->$boolprop ? "true" : "false");
#            $txt .= qq`$zmap = $TorF\n`;

	    if ($self->$boolprop)
	      {
	      $txt .= qq`$zmap\n`;
	      }
        }

        # Hack, stuff mode in for now...
        $txt .= qq`bump_initial bump_mode overlap\n`;

#        $txt .= qq`}\n`;
        $txt .= qq`\n`;

        return $txt;
    }
}


{
    my $method_file = "methods.ace";
    my $format = "keyfile";

    my $usage = sub { exit(exec('perldoc', $0)) };

    GetOptions('file=s' => \$method_file,
               'format=s' => \$format,
               'help|h' => $usage) || $usage->();
    
    my $collection = Hum::Ace::MethodCollection->new_from_file($method_file);
    foreach my $meth(@{$collection->get_all_Methods}){
#        print STDERR "Found Method " . $meth->name . "\n";
      if ($format =~ m/keyfile/i) {
        print $meth->zmap_string;
      }
      elsif ($format =~ m/ace/i) {
        print $meth->zmap_string_ace;
      }
      else {
        print "Error: unrecognised format $format";
      }
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

 methods2style.pl -file <methods.ace> [-format keyfile|ace] -help

 where <methods.ace> specifies full location of file to read and
 the -format argument indicates the type of output; 'keyfile' (default)
 for an ini-style file or 'ace' for a .ace file.
 -help shows this pod.

=cut
