/* Generated file: do not edit */
/* due to lack of heredoc syntax or docs on gcc */
char *default_styles = "\
# default styles for GFF based ZMap\n\
# also used as base styles for user config\n\
\n\
\n\
# glyphs are needed by styles, may be overridden by user config\n\
[glyphs]\n\
dn-tri = <0,4; -4,0 ;4,0; 0,4>\n\
up-tri = <0,-4 ;-4,0 ;4,0 ;0,-4>\n\
\n\
# up/dn tri offset to hit end of feature\n\
fwd5-tri = <0,0; -4,-4 ;4,-4; 0,0>\n\
fwd3-tri = <0,4; -4,0 ;4,0; 0,4>\n\
\n\
rev5-tri = <0,-4 ;-4,0 ;4,0 ;0,-4>\n\
rev3-tri = <0,0 ;-4,4 ;4,4 ;0,0>\n\
\n\
dn-hook = <0,0; 15,0; 15,10>\n\
up-hook = <0,0; 15,0; 15,-10>\n\
\n\
\n\
# generic styles lifted from otterlace\n\
\n\
[style-heatmap]\n\
description=intensity scale between fill and border colours, rebinned on zoom\n\
width=8\n\
graph-scale=log\n\
mode=graph\n\
graph-density-fixed=true\n\
strand-specific=true\n\
graph-density=true\n\
graph-mode=heatmap\n\
bump-style=line\n\
default-bump-mode=style\n\
colours=normal fill white; normal border black\n\
graph-baseline=0.000000\n\
max-score=1000.000000\n\
min-score=0.000000\n\
graph-density-stagger=9\n\
graph-density-min-bin=2\n\
\n\
[style-histogram]\n\
description=box graph, not rebinned on zoom\n\
width=31.000000\n\
colours=normal fill MistyRose1 ; normal border MistyRose3\n\
max-mag=1000.000000\n\
graph-baseline=0.000000\n\
max-score=1000.000000\n\
mode=graph\n\
min-score=0.000000\n\
graph-mode=histogram\n\
\n\
#default graph mode is histogram\n\
[style-graph]\n\
parent-style=histogram\n\
description=default graph mode\n\
\n\
[style-line]\n\
description=wiggle plot, rebinned on zoom\n\
width=40.000000\n\
graph-scale=log\n\
mode=graph\n\
graph-density-fixed=true\n\
graph-density=true\n\
graph-mode=line\n\
colours=normal border blue\n\
graph-baseline=0.000000\n\
max-score=1000.000000\n\
min-score=0.000000\n\
graph-density-stagger=20\n\
graph-density-min-bin=4\n\
\n\
\n\
# sub feature glyphs\n\
[style-homology-glyph]\n\
description=incomplete homology marker\n\
mode=glyph\n\
glyph-alt-colours=normal fill YellowGreen; normal border #81a634\n\
colours=normal fill #e74343; normal border #dc2323\n\
glyph-3-rev=rev3-tri\n\
score-mode=alt\n\
glyph-3=fwd3-tri\n\
glyph-5=fwd5-tri\n\
glyph-5-rev=rev5-tri\n\
glyph-threshold=3\n\
\n\
[style-nc-splice-glyph]\n\
description=non-conceus splice marker\n\
glyph-strand=flip-y\n\
colours=normal fill yellow ; normal border blue\n\
mode=glyph\n\
glyph-3=up-tri\n\
glyph-5=dn-tri\n\
\n\
\n\
[style-root]\n\
description=base style for all, implements default width and selected colours\n\
width=9.000000\n\
bump-mode=unbump\n\
default-bump-mode=overlap\n\
colours=selected fill gold ; selected border black\n\
bump-spacing=3.000000\n\
\n\
\n\
[style-alignment]\n\
description=default alignment style\n\
sub-features=homology:homology-glyph ; non-concensus-splice:nc-splice-glyph\n\
alignment-show-gaps=true\n\
alignment-parse-gaps=true\n\
mode=alignment\n\
strand-specific=true\n\
alignment-pfetchable=true\n\
colours=normal fill pink ; normal border #97737a\n\
default-bump-mode=all\n\
parent-style=root\n\
show-reverse-strand=true\n\
directional-ends=true\n\
max-score=100.000000\n\
min-score=70.000000\n\
score-mode=percent\n\
\n\
\n\
[style-basic]\n\
description=simple features\n\
colours=normal fill lavender ; normal border #898994\n\
parent-style=root\n\
mode=basic\n\
\n\
\n\
[style-assembly-path]\n\
description=assembly path\n\
colours=normal fill lavender ; normal border #898994\n\
parent-style=root\n\
mode=assembly-path\n\
\n\
\n\
[style-transcript]\n\
description=generic transcript\n\
transcript-cds-colours=normal fill white ; normal border SlateBlue ; selected fill gold\n\
width=7.000000\n\
colours=normal fill LightGray ; normal border SteelBlue ; selected fill #ecc806\n\
parent-style=root\n\
show-reverse-strand=true\n\
mode=transcript\n\
bump-mode=overlap\n\
strand-specific=true\n\
\n\
\n\
# alignment optional styles\n\
[style-gapped-align]\n\
description=alignment, shows gaps when not bumped\n\
alignment-pfetchable=false\n\
default-bump-mode=overlap\n\
colours=normal border CadetBlue\n\
parent-style=alignment\n\
alignment-join-align=0\n\
alignment-between-error=1\n\
alignment-always-gapped=true\n\
\n\
[style-dna-align]\n\
description=DNA alignment\n\
alignment-blixem=blixem-n\n\
mode=alignment\n\
alignment-join-align=0\n\
strand-specific=false\n\
show-text=true\n\
alignment-within-error=0\n\
parent-style=alignment\n\
max-score=100.000000\n\
min-score=70.000000\n\
score-mode=percent\n\
\n\
[style-pep-align]\n\
description=Protein alignment\n\
sub-features=homology:homology-glyph\n\
parent-style=alignment\n\
max-score=100.000000\n\
show-reverse-strand=true\n\
mode=alignment\n\
min-score=70.000000\n\
score-mode=percent\n\
frame-mode=always\n\
\n\
[style-masked-align]\n\
description=masked alignment, useful for EST\n\
colours=normal fill Purple ; normal border #3d1954\n\
parent-style=dna-align\n\
alignment-mask-sets=self\n\
default-bump-mode=all\n\
\n\
\n\
\n\
# former 'predefined' styles: note: names must be as in zmapConfigStyleDefaults.h\n\
\n\
#[style-Plain]\n\
#description=internal style used for decorations\n\
#mode=plain\n\
#colours=normal fill white; normal border black\n\
#displayable=true\n\
\n\
[style-3 Frame]\n\
description=3 frame display\n\
mode=meta\n\
bump-mode=unbump\n\
default_bump_mode=unbump\n\
displayable=false\n\
\n\
[style-sequence]\n\
description=text display for DNA and protein sequences\n\
mode=sequence\n\
displayable=true\n\
display-mode=hide\n\
bump-mode=unbump\n\
default-bump-mode=unbump\n\
bump-fixed=true\n\
width=30\n\
colours = normal fill white ; normal draw black ; selected fill red\n\
sequence-non-coding-colours = normal fill red ; normal draw black ; selected fill pink\n\
sequence-coding-colours = normal fill OliveDrab ; normal draw black ; selected fill pink\n\
sequence-split-codon-5-colours = normal fill orange ; normal draw black ; selected fill pink\n\
sequence-split-codon-3-colours = normal fill yellow ; normal draw black ; selected fill pink\n\
sequence-in-frame-coding-colours = normal fill green ; normal draw black ; selected fill pink\n\
\n\
[style-3 Frame Translation]\n\
description=3 frame translation display\n\
parent-style=sequence\n\
# width in characters\n\
width=10\n\
frame-mode=only-3\n\
\n\
[style-Show Translation]\n\
description=show translation in zmap display\n\
parent-style=sequence\n\
\n\
[style-DNA]\n\
description=dna display\n\
parent-style=sequence\n\
\n\
[style-Locus]\n\
description=locus name text column display\n\
mode=text\n\
displayable=true\n\
display-mode=hide\n\
bump-mode=unbump\n\
default-bump-mode=unbump\n\
strand-specific=true\n\
colours = normal fill white ; normal draw black\n\
width=20\n\
\n\
# due to the vagaries of ACEDB we only request GF features if\n\
# we hand it a style which has a hard coded name\n\
# despite the fact that we supply a different one in otter_styles.ini\n\
[style-GeneFinderFeatures]\n\
description=Gene Finder Features display\n\
mode=meta\n\
displayable=false\n\
display-mode=hide\n\
bump-mode=unbump\n\
default-bump-mode=unbump\n\
bump-fixed=true\n\
\n\
[style-Scale]\n\
description=scale bar display\n\
mode=plain\n\
colours=normal fill white; normal border black\n\
displayable=true\n\
\n\
[style-Strand Separator]\n\
description=strand separator display\n\
#ideally mode would be plain but we want to add basic features (search hit markers) to this\n\
mode=basic\n\
width=17.0\n\
show-when-empty=true\n\
displayable=true\n\
display-mode=show\n\
bump-mode=unbump\n\
default-bump-mode=unbump\n\
bump-fixed=true\n\
show-only-in-separator=true\n\
colours = normal fill yellow; selected fill yellow\n\
\n\
[style-Search Hit Marker]\n\
description=display location of matches to query\n\
mode=basic\n\
displayable=true\n\
display-mode=show\n\
bump-mode=unbump\n\
default-bump-mode=unbump\n\
bump-fixed=true\n\
width=11.0\n\
offset=3.0\n\
strand-specific=false\n\
show-only-in-separator=true\n\
colours = normal fill red ; normal draw black ; selected fill red; selected draw black\n\
rev-colours = normal fill green ; normal draw black ; selected fill green ; selected draw black\n\
\n\
[style-assembly]\n\
rev_colours = normal fill green ; normal draw black ; selected fill green ; selected draw black\n\
mode=assembly\n\
displayable=true\n\
display-mode=show\n\
width=10\n\
bump-mode=alternating\n\
default-bump-mode=alternating\n\
bump-fixed=true\n\
colours = normal fill gold ; normal border black ; selected fill orange ; selected border blue\n\
non-assembly-colours = normal fill brown ; normal border black ; selected fill red ; selected border black\n\
\n\
[style-Assembly Path]\n\
description=assembly path for displayed sequence\n\
parent-style=assembly\n\
\n\
[style-Scratch]\n\
description=scratch-pad column for creating/editing temporary features\n\
parent-style=transcript\n\
# width in characters\n\
width=10\n\
\n\
\n\
\n\
";
