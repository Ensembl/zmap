#!/usr/bin/env perl -w

use strict;
use warnings;
use Hum::Ace::MethodCollection;
use Getopt::Long;
use Data::Dumper;

{

    my $keyword_convert = { 
        name           => 'name',
        width          => 'width',
	remark         => 'description',
        show_up_strand => 'show-reverse-strand',
        frame_sensitive => 'frame-specific',
        strand_sensitive => 'strand-specific',
        bumpable       => 'overlap',
        color          => 'colours',
        cds_color      => 'transcript-cds-colours',
        no_display     => 'never-display',
        score_method   => 'score-mode',
        score_bounds   => 'score_bounds',
        score_max      => 'max-score',
        score_min       => 'min-score',
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

    my $keyword_convert_ace = { 
        name           => 'name',
        width          => 'width',
	remark         => 'description',
        show_up_strand => 'show_up_strand',
        frame_sensitive => 'frame_sensitive',
        strand_sensitive => 'strand_sensitive',
        bumpable       => 'overlap',
        color          => 'colours',
        cds_color      => 'transcript cds_colour',
        no_display     => 'never_display',
        score_method   => 'score_style',
        score_bounds   => 'score_bounds',
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

    my $ACEDB_MAG_FACTOR = 6.0 ;
    my $ACEDB_MAG_FACTOR_ALIGN = 16.0 ;


    sub Hum::Ace::Method::zmap_string{

        my ($self) = @_;

#        my $txt = qq`Type\n{\n`;
	my $txt = '';

        # For transcript modes, the colours will be for the border, 
        # otherwise they will the fill colour (default).
        my $colour_type = 'fill';

        # use a different magnification factor for alignments
        my $mag_factor = $ACEDB_MAG_FACTOR;

        # in the case of each of the following the arrays are lists of
        # perl methods avaiable for the method.  These get translated 
        # to be the correct zmap properties for the type.


        foreach my $txtprop(qw(name)){

            my $zmap  = $keyword_convert->{$txtprop} || next;

            my $value = lc($self->$txtprop())    || next;

            # Determine the mode from the name.
            if ($self->name =~ m/^halfwise$/i ||
                $self->name =~ m/^genscan$/i ||
                $self->name =~ m/^retained_intron$/i ||
                $self->name =~ m/cds/i ||
                $self->name =~ m/^estgene$/i ||
                $self->name =~ m/^ensembl$/i  ||
                $self->name =~ m/^coding$/i ||
                $self->name =~ m/^curated$/i ||
                $self->name =~ m/transcript/i ||
                $self->name =~ m/history/i ||
                $self->name =~ m/^jigsaw$/i ||
                $self->name =~ m/^rnaseq\..*$/i ||
                $self->name =~ m/^rnaseq_hillier\..*$/i ||
                $self->name =~ m/^mgene$/i ||
                $self->name =~ m/^genefinder$/i ||
                $self->name =~ m/^fgenesh$/i ||
                $self->name =~ m/^genblastg$/i ||
                $self->name =~ m/^twinscan$/i ||
                $self->name =~ m/^three_prime_utr$/i ||
                $self->name =~ m/pseudogene/i)
	      {
                $txt .= qq`[$value]\n`;
                $txt .= qq`mode=transcript\n`;
                $colour_type = 'border';
	      }
	    elsif ($self->name =~ m/trembl/i ||
                   $self->name =~ m/swissprot/i ||
                   $self->name =~ m/vertebrate_rna/i || 
                   $self->name =~ m/refseq/i ||
                   $self->name =~ m/blast/i ||
                   $self->name =~ m/ditags/i ||
                   $self->name =~ m/est/i)
	      {
                $txt .= qq`[$value]\n`;
                $txt .= qq`mode=alignment\n`;
                $mag_factor = $ACEDB_MAG_FACTOR_ALIGN;
	      }
            elsif ($self->name =~ m/^gf_splice$/i ||
                   $self->name =~ m/^root$/i ||
                   $self->name =~ m/^feat$/i ||
                   $self->name =~ m/^DNA$/i ||
                   $self->name =~ m/^Show Translation$/i ||
                   $self->name =~ m/^3 Frame$/i ||
                   $self->name =~ m/^3 Frame Translation$/i ||
                   $self->name =~ m/^gene_finder_feat$/i ||
                   $self->name =~ m/^GeneFinderFeatures$/i ||
                   $self->name =~ m/^ATG$/i ||
                   $self->name =~ m/^GF_ATG$/i ||
                   $self->name =~ m/^GF_coding_seg$/i ||
                   $self->name =~ m/^hexExon$/i ||
                   $self->name =~ m/^hexExon_span$/i ||
                   $self->name =~ m/^hexIntron$/i ||
                   $self->name =~ m/^heatmap$/i ||
                   $self->name =~ m/^histogram$/i ||
                   $self->name =~ m/^homology-glyph$/i ||
                   $self->name =~ m/^line$/i ||
                   $self->name =~ m/^nc-splice-glyph$/i)
              {
                return $txt; # inclued in hard-coded styles; nothing to do
              }
            else
	      {
                $txt .= qq`[$value]\n`;
                $txt .= qq`mode=basic\n`;
	      }
        }

#        foreach my $txtprop(qw(strand_sensitive frame_sensitive show_up_strand)){
#            my $zmap  = $keyword_convert->{$txtprop} || next;
#            $txt .= qq`$zmap = true\n`;
#        }

        foreach my $txtprop(qw(remark)){
            my $zmap  = $keyword_convert->{$txtprop} || next;
            my $value = $self->$txtprop()    || next;
            $txt .= qq`$zmap="$value"\n`;
        }

        foreach my $txtprop(qw(color cds_color)) {
            my $zmap  = $keyword_convert->{$txtprop};
            my $index = $self->$txtprop();
            my $value;

            if ($index) {
                $value = $colour_convert->{lc($index)};
            }

            # Put all values on the same line, separated by semi colons
            if ($zmap && ($value || $txtprop =~ m/^color/i)) {
                $txt .= qq'$zmap=';
            }

            if ($zmap && $value) {
                $txt .= qq`normal $colour_type $value ; `;

                # if setting the border colour, set the same border colour
                # when selected, and set the fill to white.
                if ($colour_type =~ m/border/i){
                  $txt .= qq`selected border $value ; `;
                  $txt .= qq'normal fill #ffffff ; ';
                }
            }

            if ($txtprop =~ m/^color/i && (!$value || $colour_type !~ m/border/i)){
                # Set a default black border for any features that don't have it set
                $txt .= qq'normal border #000000 ; ';
                $txt .= qq'selected border #000000 ; ';
            }
                
            # specify the fill colour when the feature is selected
            # to do: what should selection colour be? I've hard-coded 
            # values for now (light yellow). Note: only specify the
            # selection colour for "cds_color" if the value exists 
            # otherwise it is n/a, but for the general color property
            # we must always set it.
	    if ($txtprop =~ m/^color/i){
		$txt .= qq`selected fill #ffddcc ; `; 
	    }
            elsif ($value) {
                $txt .= qq`selected fill #ffddcc ; `; 
            }

            if ($zmap && ($value || $txtprop =~ m/^color/i)) {
                $txt .= qq'\n';
              }
        }

        foreach my $numprop(qw(width)){
            my $zmap  = $keyword_convert->{$numprop} || next;
            my $value = $self->$numprop()    || next;
            $txt .= "$zmap=" . ($self->$numprop * $mag_factor) . "\n";
        }

        foreach my $boolprop(qw(score_method)){
            my $zmap = $keyword_convert->{$boolprop} || next;
            my $value = $self->$boolprop() || next;
            $txt .= "$zmap=$value\n";
        }

        foreach my $arrprop(qw(score_bounds)){
            my $zmap = $keyword_convert->{$arrprop} || next;
            if (my @values = $self->$arrprop()){
                $txt .= "min-score=$values[0]\n";
                $txt .= "max-score=$values[1]\n";
            }
        }

        foreach my $boolprop(qw(strand_sensitive frame_sensitive show_up_strand)){
            my $zmap = $keyword_convert->{$boolprop} || next;
            my $TorF = ($self->$boolprop ? "true" : "false");
            
            if ($zmap =~ m/strand-specific/i) {
                $txt .= qq'$zmap=true\n';
            }
            else {
                $txt .= qq`$zmap=$TorF\n`;
            }

            if ($zmap =~ m/frame-specific/i && $self->$boolprop) {
                $txt .= qq'frame-mode=always\n';
            }
        }

        # Hack, stuff mode in for now...
        $txt .= qq`bump-mode=unbump\n`;
        $txt .= qq`default-bump-mode=overlap\n`;

#        $txt .= qq`}\n`;
        $txt .= qq`\n`;

        return $txt;
    }
}


{
    # Hard-coded predefined styles that we shove into the styles.ini file:
    my $predefined_styles="    
#
# Extra acedb styles
#

[root]
width=9.000000
default-bump-mode=overlap
colours=selected fill gold ; selected border black
bump-spacing=3.000000
bump-mode=unbump

[feat]
colours=normal fill lavender ; normal border #898994
parent-style=root
mode=basic
bump-mode=unbump

[DNA]
colours=selected fill green

[Show Translation]

[3 Frame]

[3 Frame Translation]
colours=selected fill green

[gene_finder_feat]
width=11.000000
parent-style=feat
strand-specific=true
frame-mode=only-3

[GeneFinderFeatures]

[ATG]
colours=normal fill yellow2
parent-style=gene_finder_feat

[GF_ATG]
colours=normal fill IndianRed
parent-style=gene_finder_feat
max-score=3.000000
min-score=0.000000

[GF_coding_seg]
colours=normal fill gray
parent-style=gene_finder_feat
max-score=8.000000
min-score=2.000000
score-mode=width

[GF_splice]
width=30.000000
mode=glyph
strand-specific=true
hide-forward-strand=true
colours=normal fill grey
max-score=4.000000
glyph-mode=splice
glyph-5=up-hook
frame0-colours=normal fill blue; normal border blue; selected fill blue; selected border blue
show-when-empty=false
frame1-colours=normal fill green; normal border green; selected fill green; selected border green
frame2-colours=normal fill red; normal border red; selected fill red; selected border red
frame-mode=only-1
show-reverse-strand=false
min-score=-2.000000
score-mode=width
glyph-3=dn-hook
bump-mode=unbump

[hexExon]
colours=normal fill orange
parent-style=gene_finder_feat
max-score=100.000000
min-score=10.000000
score-mode=width

[hexExon_span]
parent-style=gene_finder_feat

[hexIntron]
colours=normal fill DarkOrange
parent-style=gene_finder_feat
max-score=100.000000
min-score=10.000000
score-mode=width

[heatmap]
width=8
graph-scale=log
mode=graph
graph-density-fixed=true
strand-specific=true
graph-density=true
graph-mode=heatmap
bump-style=line
default-bump-mode=style
colours=normal fill white; normal border black
graph-baseline=0.000000
max-score=1000.000000
min-score=0.000000
graph-density-stagger=9
graph-density-min-bin=2

[histogram]
width=31.000000
colours=normal fill MistyRose1 ; normal border MistyRose3
max-mag=1000.000000
graph-baseline=0.000000
max-score=1000.000000
mode=graph
min-score=0.000000
graph-mode=histogram

[homology-glyph]
mode=glyph
glyph-alt-colours=normal fill YellowGreen; normal border #81a634
colours=normal fill #e74343; normal border #dc2323
glyph-3-rev=rev3-tri
score-mode=alt
glyph-3=fwd3-tri
glyph-5=fwd5-tri
glyph-5-rev=rev5-tri
glyph-threshold=3

[line]
width=40.000000
graph-scale=log
mode=graph
graph-density-fixed=true
graph-density=true
graph-mode=line
colours=normal border blue
graph-baseline=0.000000
max-score=1000.000000
min-score=0.000000
graph-density-stagger=20
graph-density-min-bin=4

[nc-splice-glyph]
glyph-strand=flip-y
colours=normal fill yellow ; normal border blue
mode=glyph
glyph-3=up-tri
glyph-5=dn-tri



#
# standard styles
#
";

    my $method_file = "methods.ace";

    my $usage = sub { exit(exec('perldoc', $0)) };

    GetOptions('file=s' => \$method_file,
               'help|h' => $usage) || $usage->();
    
    my $collection = Hum::Ace::MethodCollection->new_from_file($method_file);
    my $txt;

    $txt .= qq`$predefined_styles\n`;

    foreach my $meth(@{$collection->get_all_Methods}){
#        print STDERR "Found Method " . $meth->name . "\n";
      $txt .= $meth->zmap_string;
      }
 
  print $txt;
}

__END__

=pod

=head1 NAME

 methods2style.pl

=head1 DESCRIPTION

 Currently  a simple  script to  parse a  methods.ace file  and output
 (STDOUT) a Style file to be useful for ZMap.
 Requires the PerlModules directory, which should be distributed along
 with this script, to be included in your PERL5LIB environment variable.

=head1 USAGE

 methods2style.pl -file <methods.ace> -help

 where <methods.ace> specifies full location of file to read
 -help shows this pod.

=cut
