/*  File: zmapStyle.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  and formerly Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the zmapStyle GObject
 *
 * Exported functions: See ZMap/zmapStyle.h
 *
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapStyle_I.h>
#include <ZMap/zmapConfigIni.h>   // for zMapConfigLegacyStyles() only can remove when system has moved on

/*
 * Some notes on implementation by mh17
 *
 * There is a write up in ZMap/docs/Design_notes/modules/zmapFeature.shtml
 * (or wherever your source tree is)
 *
 * Styles are GObjects and implement the g_object_get/set interface
 * but internally (esp. in this file) the code uses the ZMapFeatureTypeStyleStruct directly
 *
 * We use a mechanical process of accesing data, using an array of ZMapStyleParamStruct
 * to define offsets to struct members, and an array of is_set flags is maintained auotmatically
 */






/* all the parameters of a style, closely follows the style struct members
 *-----------------------------------------------------------------------
 * NB: this must be fully populated and
 * in the same order as the ZMapStyleParamId enum defined in zmapStyle.h
 *
 * it would be even better if we assigned each one to an array
 * using the id as an index
 *-----------------------------------------------------------------------
 */
ZMapStyleParamStruct zmapStyleParams_G[_STYLE_PROP_N_ITEMS] =
{
    { STYLE_PROP_NONE, STYLE_PARAM_TYPE_INVALID, ZMAPSTYLE_PROPERTY_INVALID
            "", "", 0,0 },                                           // (0 not used)

    { STYLE_PROP_IS_SET, STYLE_PARAM_TYPE_FLAGS, ZMAPSTYLE_PROPERTY_IS_SET,
            "isset", "parameter set flags",
            offsetof(zmapFeatureTypeStyleStruct,is_set),0 },          /* (internal/ automatic property) */

    { STYLE_PROP_NAME, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_NAME,
            "name", "Name",
            offsetof(zmapFeatureTypeStyleStruct, original_id ),0 },                         /* Name as text. */
            // review zMapStyleGetName() if you change this
    { STYLE_PROP_ORIGINAL_ID, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_ORIGINAL_ID,
            "original-id", "original id",
            offsetof(zmapFeatureTypeStyleStruct, original_id ),0 },
    { STYLE_PROP_UNIQUE_ID, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_UNIQUE_ID,
            "unique-id", "unique id",
            offsetof(zmapFeatureTypeStyleStruct, unique_id) ,0},

    { STYLE_PROP_PARENT_STYLE, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_PARENT_STYLE,
            "parent-style", "parent style unique id",
            offsetof(zmapFeatureTypeStyleStruct, parent_id),0},
    { STYLE_PROP_DESCRIPTION, STYLE_PARAM_TYPE_STRING, ZMAPSTYLE_PROPERTY_DESCRIPTION,
            "description", "Description",
            offsetof(zmapFeatureTypeStyleStruct, description) ,0},
    { STYLE_PROP_MODE, STYLE_PARAM_TYPE_MODE, ZMAPSTYLE_PROPERTY_MODE,
            "mode", "Mode",
            offsetof(zmapFeatureTypeStyleStruct, mode) ,0},

    { STYLE_PROP_COLOURS,        STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_COLOURS,
            "main colours", "Colours used for normal feature display.",
            offsetof(zmapFeatureTypeStyleStruct,colours ),0 },
    { STYLE_PROP_FRAME0_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME0_COLOURS,
            "frame0-colour-normal", "Frame0 normal colour",
            offsetof(zmapFeatureTypeStyleStruct, frame0_colours),0 },
    { STYLE_PROP_FRAME1_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME1_COLOURS,
            "frame1-colour-normal", "Frame1 normal colour",
            offsetof(zmapFeatureTypeStyleStruct,frame1_colours ),0 },
    { STYLE_PROP_FRAME2_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME2_COLOURS,
            "frame2-colour-normal", "Frame2 normal colour",
            offsetof(zmapFeatureTypeStyleStruct,frame2_colours ) ,0},
    { STYLE_PROP_REV_COLOURS,    STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_REV_COLOURS,
            "main-colour-normal", "Reverse Strand normal colour",
            offsetof(zmapFeatureTypeStyleStruct, strand_rev_colours) ,0 },

    { STYLE_PROP_COLUMN_DISPLAY_MODE, STYLE_PARAM_TYPE_COLDISP, ZMAPSTYLE_PROPERTY_DISPLAY_MODE,
            "display-mode", "Display mode",
            offsetof(zmapFeatureTypeStyleStruct, col_display_state ),0 },

    { STYLE_PROP_BUMP_DEFAULT, STYLE_PARAM_TYPE_BUMP, ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE,
            "bump-mode", "The Default Bump Mode",
            offsetof(zmapFeatureTypeStyleStruct, default_bump_mode),0 },
    { STYLE_PROP_BUMP_MODE, STYLE_PARAM_TYPE_BUMP, ZMAPSTYLE_PROPERTY_BUMP_MODE,
            "initial-bump_mode", "Current bump mode",
            offsetof(zmapFeatureTypeStyleStruct, initial_bump_mode),0 },
    { STYLE_PROP_BUMP_FIXED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_BUMP_FIXED,
            "bump-fixed", "Style cannot be changed once set.",
            offsetof(zmapFeatureTypeStyleStruct, bump_fixed) ,0},
    { STYLE_PROP_BUMP_SPACING, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_BUMP_SPACING,
            "bump-spacing", "space between columns in bumped columns",
            offsetof(zmapFeatureTypeStyleStruct, bump_spacing) ,0},
    { STYLE_PROP_BUMP_STYLE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_BUMP_STYLE,
            "bump to different style", "bump to different style",
            offsetof(zmapFeatureTypeStyleStruct, bump_style),0 },

    { STYLE_PROP_FRAME_MODE, STYLE_PARAM_TYPE_3FRAME, ZMAPSTYLE_PROPERTY_FRAME_MODE,
            "3 frame display mode", "Defines frame sensitive display in 3 frame mode.",
            offsetof(zmapFeatureTypeStyleStruct, frame_mode),0 },

    { STYLE_PROP_MIN_MAG, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MIN_MAG,
            "min-mag", "minimum magnification",
            offsetof(zmapFeatureTypeStyleStruct, min_mag),0 },
    { STYLE_PROP_MAX_MAG, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MAX_MAG,
            "max-mag", "maximum magnification",
            offsetof(zmapFeatureTypeStyleStruct, max_mag),0 },

    { STYLE_PROP_WIDTH, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_WIDTH,
            "width", "width",
            offsetof(zmapFeatureTypeStyleStruct, width),0 },

    { STYLE_PROP_SCORE_MODE, STYLE_PARAM_TYPE_SCORE, ZMAPSTYLE_PROPERTY_SCORE_MODE,
            "score-mode", "Score Mode",
            offsetof(zmapFeatureTypeStyleStruct, score_mode) ,0},
    { STYLE_PROP_MIN_SCORE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MIN_SCORE,
            "min-score", "minimum-score",
            offsetof(zmapFeatureTypeStyleStruct, min_score),0 },
    { STYLE_PROP_MAX_SCORE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MAX_SCORE,
            "max-score", "maximum score",
            offsetof(zmapFeatureTypeStyleStruct, max_score),0 },
    { STYLE_PROP_SCORE_SCALE, STYLE_PARAM_TYPE_GRAPH_SCALE, ZMAPSTYLE_PROPERTY_SCORE_SCALE,
            "graph-scale", "Graph Scale",
            offsetof(zmapFeatureTypeStyleStruct, score_scale) , 0 },

    { STYLE_PROP_SUMMARISE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_SUMMARISE,
            "summarise featureset at low zoom", "summarise featureset at low zoom",
            offsetof(zmapFeatureTypeStyleStruct, summarise), 0 },

    { STYLE_PROP_COLLAPSE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_COLLAPSE,
            "collapse identical features into one", "collapse identical features into one",
            offsetof(zmapFeatureTypeStyleStruct, collapse), 0 },


    { STYLE_PROP_GFF_SOURCE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_GFF_SOURCE,
            "gff source", "GFF Source",
            offsetof(zmapFeatureTypeStyleStruct, gff_source) ,0 },
    { STYLE_PROP_GFF_FEATURE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_GFF_FEATURE,
            "gff feature", "GFF feature",
            offsetof(zmapFeatureTypeStyleStruct, gff_feature),0 },

    { STYLE_PROP_DISPLAYABLE, STYLE_PARAM_TYPE_BOOLEAN,ZMAPSTYLE_PROPERTY_DISPLAYABLE,
            "displayable", "Is the style Displayable",
            offsetof(zmapFeatureTypeStyleStruct, displayable),0 },
    { STYLE_PROP_SHOW_WHEN_EMPTY, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,
            "show when empty", "Does the Style get shown when empty",
            offsetof(zmapFeatureTypeStyleStruct, show_when_empty ) ,0},
    { STYLE_PROP_SHOW_TEXT, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_TEXT,
            "show-text",  "Show as Text",
            offsetof(zmapFeatureTypeStyleStruct, showText) ,0 },
    { STYLE_PROP_SUB_FEATURES, STYLE_PARAM_TYPE_SUB_FEATURES, ZMAPSTYLE_PROPERTY_SUB_FEATURES,
            "sub-features",  "Sub-features (glyphs)",
            offsetof(zmapFeatureTypeStyleStruct, sub_features) ,0 },

    { STYLE_PROP_STRAND_SPECIFIC, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,
            "strand specific", "Strand Specific ?",
            offsetof(zmapFeatureTypeStyleStruct, strand_specific) ,0 },
    { STYLE_PROP_SHOW_REV_STRAND, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,
            "show-reverse-strand", "Show Rev Strand ?",
            offsetof(zmapFeatureTypeStyleStruct, show_rev_strand) ,0 },
    { STYLE_PROP_HIDE_FWD_STRAND, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,
            "hide-forward-strand", "Hide Fwd Strand when RevComp'd?",
            offsetof(zmapFeatureTypeStyleStruct, hide_fwd_strand) ,0 },
    { STYLE_PROP_SHOW_ONLY_IN_SEPARATOR, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,
            "only in separator", "Show Only in Separator",
            offsetof(zmapFeatureTypeStyleStruct, show_only_in_separator),0 },
    { STYLE_PROP_DIRECTIONAL_ENDS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,
            "directional-ends", "Display pointy \"short sides\"",
            offsetof(zmapFeatureTypeStyleStruct, directional_end),0 },

    { STYLE_PROP_FOO, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_FOO,
            "as Foo Canvas Items", "use old technology",
            offsetof(zmapFeatureTypeStyleStruct, foo),0 },

    { STYLE_PROP_FILTER, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_FILTER,
            "filter by score", "filter column by score",
            offsetof(zmapFeatureTypeStyleStruct, filter),0 },

    { STYLE_PROP_GLYPH_NAME, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME,
            "glyph-name", "Glyph name used to reference glyphs config stanza",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_name),ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_SHAPE, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE,
             "glyph-type", "Type of glyph to show.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph), ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_NAME_5, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_5,
            "glyph-name for 5' end", "Glyph name used to reference glyphs config stanza",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_5),ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_SHAPE_5, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5,
             "glyph-type-5", "Type of glyph to show at 5' end.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph5), ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_NAME_3, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_3,
            "glyph-name for 3' end", "Glyph name used to reference glyphs config stanza",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_3),ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_SHAPE_3, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3,
             "glyph-type-3", "Type of glyph to show at 3' end.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph3), ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_ALT_COLOURS, STYLE_PARAM_TYPE_COLOUR,ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS,
            "alternate glyph colour", "Colours used to show glyphs when below thrashold.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_alt_colours) ,ZMAPSTYLE_MODE_GLYPH  },

    { STYLE_PROP_GLYPH_THRESHOLD, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD,
            "glyph-threshold", "Glyph threshold for alternate coloura",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_threshold) ,ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_STRAND, STYLE_PARAM_TYPE_GLYPH_STRAND, ZMAPSTYLE_PROPERTY_GLYPH_STRAND,
            "glyph-strand", "What to do for the reverse strand",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_strand) ,ZMAPSTYLE_MODE_GLYPH },

    { STYLE_PROP_GLYPH_ALIGN, STYLE_PARAM_TYPE_GLYPH_ALIGN, ZMAPSTYLE_PROPERTY_GLYPH_ALIGN,
            "glyph-align", "where to centre the glyph",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.glyph_align) ,ZMAPSTYLE_MODE_GLYPH },



    // mode dependant data

    { STYLE_PROP_GRAPH_MODE, STYLE_PARAM_TYPE_GRAPH_MODE, ZMAPSTYLE_PROPERTY_GRAPH_MODE,
            "graph-mode", "Graph Mode",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.mode) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_BASELINE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,
            "graph baseline", "Sets the baseline for graph values.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.baseline),ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_SCALE, STYLE_PARAM_TYPE_GRAPH_SCALE, ZMAPSTYLE_PROPERTY_GRAPH_SCALE,
            "graph-scale", "Graph Scale",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.scale) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_DENSITY, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY,
            "graph-density", "Density plot",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.density) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_DENSITY_FIXED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_FIXED,
            "graph-density-fixed", "anchor bins to pixel boundaries",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.fixed) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_DENSITY_MIN_BIN, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_MIN_BIN,
            "graph-density-min-bin", "min bin size in pixels",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.min_bin) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_DENSITY_STAGGER, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_STAGGER,
            "graph-density-stagger", "featureset/ column offset",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.stagger) ,ZMAPSTYLE_MODE_GRAPH },


    { STYLE_PROP_ALIGNMENT_PARSE_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,
            "parse gaps", "Parse Gaps ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.parse_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_SHOW_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,
            "show gaps", "Show Gaps ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.show_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT  },
    { STYLE_PROP_ALIGNMENT_ALWAYS_GAPPED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_ALWAYS_GAPPED,
            "always gapped", "Always Gapped ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.always_gapped) ,ZMAPSTYLE_MODE_ALIGNMENT  },
    { STYLE_PROP_ALIGNMENT_UNIQUE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_UNIQUE,
            "unique", "Don't display same name as joined up",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.unique) ,ZMAPSTYLE_MODE_ALIGNMENT  },
    { STYLE_PROP_ALIGNMENT_BETWEEN_ERROR,STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,
            "alignment between error", "Allowable alignment error between HSP's",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.between_align_error)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,
            "Allow misalign", "Allow match -> ref sequences to be different lengths.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.allow_misalign)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_PFETCHABLE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,
            "Pfetchable alignments", "Use pfetch proxy to get alignments ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.pfetchable)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_BLIXEM, STYLE_PARAM_TYPE_BLIXEM, ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM,
            "Blixemable alignments","Use blixem to view sequence of alignments ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.blixem_type)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_PERFECT_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,
            "perfect alignment indicator colour", "Colours used to show two alignments have exactly contiguous coords.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.perfect)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,
            "colinear alignment indicator colour", "Colours used to show two alignments have gapped contiguous coords.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.colinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,
            "noncolinear alignment indicator colour", "Colours used to show two alignments have overlapping coords.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.noncolinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_UNMARKED_COLINEAR, STYLE_PARAM_TYPE_COLDISP, ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR,
            "paint colinear lines when unmarked", "paint colinear lines when unmarked ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.unmarked_colinear), ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_GAP_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_GAP_COLOURS,
            "gap between spilt read", "Colours used to show gap between two parts of a split read.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.perfect)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_COMMON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_COMMON_COLOURS,
            "common part of squashed split read", "Colours used to show part of a squashed split read that is common to all source features.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.colinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_MIXED_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_MIXED_COLOURS,
            "mixed part of squashed split read that", "Colours used to show  part of a squashed split read that is not common to all source features.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.noncolinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_MASK_SETS, STYLE_PARAM_TYPE_QUARK_LIST_ID, ZMAPSTYLE_PROPERTY_ALIGNMENT_MASK_SETS,
            "mask featureset against others", "mask featureset against others",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.mask_sets), ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_SQUASH, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_SQUASH,
            "squash overlapping split reads into one", "squash overlapping split reads into one",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.squash),ZMAPSTYLE_MODE_ALIGNMENT },


    { STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_NON_CODING_COLOURS,
            "non-coding exon region colour", "Colour used to highlight UTR section of an exon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.non_coding), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_CODING_COLOURS,
            "coding exon region colour", "Colour used to highlight coding section of an exon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.coding), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_SPLIT_CODON_5_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_5_COLOURS,
            "coding exon split codon colour", "Colour used to highlight split codon 5' coding section of an exon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.split_codon_5), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_SPLIT_CODON_3_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_3_COLOURS,
            "coding exon split codon colour", "Colour used to highlight split codon 3' coding section of an exon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.split_codon_3), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_IN_FRAME_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_IN_FRAME_CODING_COLOURS,
            "in-frame coding exon region colour", "Colour used to highlight coding section of an exon that is in frame.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.in_frame_coding), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_START_CODON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_START_CODON_COLOURS,
            "start codon colour", "Colour used to highlight start codon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.start_codon), ZMAPSTYLE_MODE_SEQUENCE },
    { STYLE_PROP_SEQUENCE_STOP_CODON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_STOP_CODON_COLOURS,
            "stop codon colour", "Colour used to highlight stop codon.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.sequence.stop_codon), ZMAPSTYLE_MODE_SEQUENCE },


    { STYLE_PROP_TRANSCRIPT_CDS_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS,
            "CDS colours", "Colour used to CDS section of a transcript.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.transcript.CDS_colours), ZMAPSTYLE_MODE_TRANSCRIPT},

    { STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ASSEMBLY_PATH_NON_COLOURS,
            "non-path colours", "Colour used for unused part of an assembly path sequence.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.assembly_path.non_path_colours) ,ZMAPSTYLE_MODE_ASSEMBLY_PATH  },


    { STYLE_PROP_TEXT_FONT, STYLE_PARAM_TYPE_STRING, ZMAPSTYLE_PROPERTY_TEXT_FONT,
            "Text Font", "Font to draw text with.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.text.font)  ,ZMAPSTYLE_MODE_TEXT }

};



static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class);
static void zmap_feature_type_style_init      (ZMapFeatureTypeStyle      style);
static void zmap_feature_type_style_set_property(GObject *gobject,
                                     guint param_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
#if 0
static void zmap_feature_type_style_copy_set_property(GObject *gobject,
                                     guint param_id,
                                     const GValue *value,
                                     GParamSpec *pspec);
#endif
static void zmap_feature_type_style_set_property_full(ZMapFeatureTypeStyle style,
                                     ZMapStyleParam param,
                                     const GValue *value,
                                     gboolean copy);
static void zmap_feature_type_style_get_property(GObject *gobject,
                                     guint param_id,
                                     GValue *value,
                                     GParamSpec *pspec);
static void zmap_feature_type_style_dispose (GObject *object);
static void zmap_feature_type_style_finalize(GObject *object);

static ZMapFeatureTypeStyle styleCreate(guint n_parameters, GParameter *parameters) ;

static gboolean setColours(ZMapStyleColour colour, char *border, char *draw, char *fill) ;
static gboolean parseColours(ZMapFeatureTypeStyle style, guint param_id, GValue *value) ;
//static gboolean isColourSet(ZMapFeatureTypeStyle style, int param_id, char *subpart) ;
static gboolean validSplit(char **strings,
                     ZMapStyleColourType *type_out, ZMapStyleDrawContext *context_out, char **colour_out) ;
static void formatColours(GString *colour_string, char *type, ZMapStyleColour colour) ;



/* Class pointer for styles. */
static GObjectClass *style_parent_class_G ;


static ZMapStyleFullColour zmapStyleFullColour(ZMapFeatureTypeStyle style, ZMapStyleParamId id);
static gchar *zmapStyleValueColour(ZMapStyleFullColour this_colour);
static gboolean parseSubFeatures(ZMapFeatureTypeStyle style,gchar *str);
static gchar *zmapStyleValueSubFeatures(GQuark *quarks);





// G_BOXED data type for glyph styles
// defined here as glyphs include styles not versa vice
// can we do a G_OBJECT_TYPE() on these to check??

gpointer glyph_shape_copy(gpointer src)
{
  ZMapStyleGlyphShape dest = NULL;

  dest = g_new0(ZMapStyleGlyphShapeStruct,1);
  memcpy((void *) dest,(void *) src,sizeof(ZMapStyleGlyphShapeStruct));

  return(dest);
}

void glyph_shape_free(gpointer thing)
{
      g_free(thing);
}

GType zMapStyleGlyphShapeGetType (void)
{
  static GType type = 0;

  if(!type)
  {
      type = g_boxed_type_register_static("glyph-shape",glyph_shape_copy,glyph_shape_free);
  }
  return(type);
}




/*!
 * The Style is now a GObject, this function returns the class "type" of the
 * style in the GObject sense.
 *
 * @return  GType         The Style GObject type.
 *  */
GType zMapFeatureTypeStyleGetType(void)
{
  static GType type = 0;

  if (type == 0)
    {
      static const GTypeInfo info =
      {
        sizeof (zmapFeatureTypeStyleClass),
        (GBaseInitFunc)      NULL,
        (GBaseFinalizeFunc)  NULL,
        (GClassInitFunc)     zmap_feature_type_style_class_init,
        (GClassFinalizeFunc) NULL,
        NULL /* class_data */,
        sizeof (zmapFeatureTypeStyleStruct),
        0 /* n_preallocs */,
        (GInstanceInitFunc)  zmap_feature_type_style_init,
        NULL
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                             "ZMapFeatureTypeStyle",
                             &info, (GTypeFlags)0);
    }

  return type;

}


ZMapFeatureTypeStyle zMapStyleCreate(char *name, char *description)
{
  ZMapFeatureTypeStyle style = NULL ;
  GParameter params[2] ;
  guint num_params ;

  zMapAssert(name && *name && (!description || *description)) ;

  /* Reset params memory.... */
  memset(&params, 0, sizeof(params)) ;
  num_params = 0 ;

  g_value_init(&(params[0].value), G_TYPE_STRING) ;
  params[0].name = "name" ;
  g_value_set_string(&(params[0].value), name) ;
  num_params++ ;

  if (description && *description)
    {
      g_value_init(&(params[1].value), G_TYPE_STRING) ;
      params[1].name = "description" ;
      g_value_set_string(&(params[1].value), description) ;
      num_params++ ;
    }

  style = styleCreate(num_params, params) ;

  return style ;
}



ZMapFeatureTypeStyle zMapStyleCreateV(guint n_parameters, GParameter *parameters)
{
  ZMapFeatureTypeStyle style = NULL;

  style = styleCreate(n_parameters, parameters) ;

    if(style->mode ==ZMAPSTYLE_MODE_BASIC || style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
	/* default summarise to 1000 to get round lack of configuration, can aloways set to zero if wanted */
	  if(!zMapStyleIsPropertySetId(style,STYLE_PROP_SUMMARISE))
	  {
		  zmapStyleSetIsSet(style,STYLE_PROP_SUMMARISE);
		  style->summarise = 1000.0 ;
	  }
    }


  return style ;
}



/*
 * return a safe copy of src.
 * we could be dumb and memcpy the whole struct and then duplicate strings and other ref'd data
 * but this way is future proof as we do each param by data type
 * see also zMapStyleMerge()
 */
ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src)
{
  ZMapFeatureTypeStyle dest = NULL ;
  ZMapStyleParam param = zmapStyleParams_G;
  int i;

  if(!(src || !ZMAP_IS_FEATURE_STYLE(src)))
      return(dest);

  dest = g_object_new(ZMAP_TYPE_FEATURE_STYLE, NULL);

  for(i = 1;i < _STYLE_PROP_N_ITEMS;i++,param++)
    {

      if(zMapStyleIsPropertySetId(src, param->id))      // something to copy?
        {

          switch(param->type)
            {
            case STYLE_PARAM_TYPE_STRING:
              {
                gchar **srcstr, **dststr;

                srcstr = (gchar **) (((void *) src) + param->offset);
                dststr = (gchar **) (((void *) dest) + param->offset);

                *dststr = g_strdup(*srcstr);
                break;
              }
            case STYLE_PARAM_TYPE_QUARK_LIST_ID:
              {
                GList **sl,**dl;

                sl = (GList **) (((void *) src) + param->offset);
                dl = (GList **) (((void *) dest) + param->offset);
                *dl = g_list_copy(*sl);   // ok as the list is shallow
                break;
              }
            case STYLE_PARAM_TYPE_COLOUR:
                  // included explicitly to flag up that it has internal is_set flags
                  // merge handles this differently but copy is ok
            case STYLE_PARAM_TYPE_FLAGS:
                  // copy this as normal, then we don't have to set the bits one by one
             default:
              {
                void *srcval,*dstval;

                srcval = ((void *) src) + param->offset;
                dstval = ((void *) dest) + param->offset;

                memcpy(dstval,srcval,param->size);
              }
              break;
            }

//          zmapStyleSetIsSet(curr_style,param->id);
        }
    }

  return dest ;
}


void zMapStyleDestroy(ZMapFeatureTypeStyle style)
{
  g_object_unref(G_OBJECT(style));

  return ;
}




static void zmap_feature_type_style_dispose (GObject *object)
{
  GObjectClass *parent_class = G_OBJECT_CLASS(style_parent_class_G);
  ZMapFeatureTypeStyle style = ZMAP_FEATURE_STYLE(object);

  if(zMapStyleIsPropertySetId(style,STYLE_PROP_ALIGNMENT_MASK_SETS) && style->mode_data.alignment.mask_sets)
    {
      g_list_free(style->mode_data.alignment.mask_sets);
      style->mode_data.alignment.mask_sets = NULL;
    }

  if (parent_class->dispose)
    (*parent_class->dispose)(object);

  return ;
}

static void zmap_feature_type_style_finalize(GObject *object)
{
  ZMapFeatureTypeStyle style = ZMAP_FEATURE_STYLE(object);
  GObjectClass *parent_class = G_OBJECT_CLASS(style_parent_class_G);

  if (style->description)
    g_free(style->description);
  style->description = NULL;

  if (parent_class->finalize)
    (*parent_class->finalize)(object);

  return ;
}




/*!
 * This is just like g_object_get() except that it returns a boolean
 * to indicate success or failure. Note that if you supply several
 * properties you will need to look in the logs to see which one failed.
 *
 * @param   style          The style.
 * @param   first_property_name  The start of the property name/variable pointers list
 *                               (see g_object_get() for format of list).
 * @return  gboolean       TRUE if all properties retrieved, FALSE otherwise.
 *  */
gboolean zMapStyleGet(ZMapFeatureTypeStyle style, char *first_property_name, ...)
{
  gboolean result = FALSE ;
  va_list args1, args2 ;

  /* Our zmap_feature_type_style_get_property() function will return whether it's suceeded
   * in this variable BUT we must init it to FALSE because if g_object_get() detects an error
   * it will return _before_ our get function is run. */
  g_object_set_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC, GINT_TO_POINTER(result)) ;

  va_start(args1, first_property_name) ;
  G_VA_COPY(args2, args1) ;

  g_object_get_valist(G_OBJECT(style), first_property_name, args2) ;

  va_end(args1) ;
  va_end(args2) ;

  result = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC)) ;

  return result ;
}


/*!
 * This is just like g_object_set() except that it returns a boolean
 * to indicate success or failure. Note that if you supply several
 * properties you will need to look in the logs to see which one failed.
 *
 * @param   style          The style.
 * @param   first_property_name  The start of the property name/variable list
 *                               (see g_object_set() for format of list).
 * @return  gboolean       TRUE if all properties set, FALSE otherwise.
 *  */
gboolean zMapStyleSet(ZMapFeatureTypeStyle style, char *first_property_name, ...)
{
  gboolean result = FALSE ;
  va_list args1, args2 ;

  /* Our zmap_feature_type_style_set_property() function will return whether it's suceeded
   * in this variable BUT we must init it to FALSE because if g_object_set() detects an error
   * it will return _before_ our set function is run. */
  g_object_set_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC, GINT_TO_POINTER(result)) ;

  va_start(args1, first_property_name) ;
  G_VA_COPY(args2, args1) ;

  g_object_set_valist(G_OBJECT(style), first_property_name, args2) ;

  va_end(args1) ;
  va_end(args2) ;

  result = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC)) ;

  return result ;
}


gboolean zMapStyleIsPropertySet(ZMapFeatureTypeStyle style, char *property_name)
{
  gboolean is_set = FALSE ;
  GParamSpec *pspec ;
  ZMapStyleParam param;

  if (!(pspec = g_object_class_find_property(G_OBJECT_CLASS(ZMAP_FEATURE_STYLE_GET_CLASS(style)), property_name)))
    {
      zMapLogCritical("Style \"%s\", unknown property \"%s\"", zMapStyleGetName(style), property_name) ;
    }
  else
    {
      param = &zmapStyleParams_G [pspec->param_id];
      if((style->is_set[param->flag_ind] & param->flag_bit))
        is_set = TRUE;
    }
  return(is_set);
}


gboolean zMapStyleIsPropertySetId(ZMapFeatureTypeStyle style, ZMapStyleParamId id)
{
  gboolean is_set = FALSE ;
  ZMapStyleParam param;

  param = &zmapStyleParams_G [id];
  if((style->is_set[param->flag_ind] & param->flag_bit))
    is_set = TRUE;

  return(is_set);
}


// this could be done as a macro and  the assignment included
// if we are brave enough to match up the param flag_ind/bit and macro


void zmapStyleSetIsSet(ZMapFeatureTypeStyle style, ZMapStyleParamId id)
{
  ZMapStyleParam param;

  param = &zmapStyleParams_G [id];
  style->is_set[param->flag_ind] |= param->flag_bit;
}

void zmapStyleUnsetIsSet(ZMapFeatureTypeStyle style, ZMapStyleParamId id)
{
  ZMapStyleParam param;

  param = &zmapStyleParams_G [id];
  style->is_set[param->flag_ind] &= ~param->flag_bit;
}


/*!
 * Does a case <i>insensitive</i> comparison of the style name and
 * the supplied name, return TRUE if they are the same.
 *
 * @param   style          The style.
 * @param   name           The name to be compared..
 * @return  gboolean       TRUE if the names are the same.
 *  */
gboolean zMapStyleNameCompare(ZMapFeatureTypeStyle style, char *name)
{
  gboolean result = FALSE ;

  zMapAssert(style && name && *name) ;

  if (g_ascii_strcasecmp(g_quark_to_string(style->original_id), name) == 0)
    result = TRUE ;

  return result ;
}


/* Checks a style to see if it relates to a "real" feature, i.e. not a Meta or
 * or glyph like feature.
 *
 * Function returns FALSE if feature is not a real feature, TRUE otherwise.
 *  */
gboolean zMapStyleIsTrueFeature(ZMapFeatureTypeStyle style)
{
  gboolean valid = FALSE ;

  switch (style->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    case ZMAPSTYLE_MODE_SEQUENCE:

    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
      {
      valid = TRUE ;
      break ;
      }
    default:
      {
      break ;
      }
    }

  return valid ;
}


/* Checks style mode to see if it is one of the drawable types. */
gboolean zMapStyleHasDrawableMode(ZMapFeatureTypeStyle style)
{
  gboolean is_drawable = FALSE ;

  if (style->mode > ZMAPSTYLE_MODE_INVALID && style->mode < ZMAPSTYLE_MODE_META)
    is_drawable = TRUE ;

  return is_drawable ;
}


/* Checks a style to see if it contains the minimum necessary to be drawn,
 * we need this function because we no longer allow defaults because we
 * we want to do inheritance.
 *
 * Function returns FALSE if there style is not valid and the GError says
 * what the problem was.
 *  */
/* (mh17) NOTE this fucntion is only called from obscure places and is not run for the majority of drawing operations
 * so attempting to add style defaults here is doomed to failure
 */
gboolean zMapStyleIsDrawable(ZMapFeatureTypeStyle style, GError **error)
{
  gboolean valid = TRUE ;
  GQuark domain ;
  gint code = 0 ;
  char *message ;

  zMapAssert(style && error && !(*error)) ;

  domain = g_quark_from_string("ZmapStyle") ;

  /* These are the absolute basic requirements without which we can't display something.... */
  if (valid && (style->original_id == ZMAPSTYLE_NULLQUARK || style->unique_id == ZMAPSTYLE_NULLQUARK))
    {
      valid = FALSE ;
      code = 1 ;
      message = g_strdup_printf("Bad style ids, original: %d, unique: %d.",
                        style->original_id, style->unique_id) ;
    }

  if (valid && !zMapStyleIsPropertySetId(style,STYLE_PROP_MODE))
    {
      valid = FALSE ;
      code = 2 ;
      message = g_strdup("Style mode not set.") ;
    }

  if (valid && !zMapStyleIsPropertySetId(style,STYLE_PROP_COLUMN_DISPLAY_MODE))
    {
      valid = FALSE ;
      code = 6 ;
      message = g_strdup("Style display_state not set.") ;
    }

  /* Check modes/sub modes. */
  if (valid)
    {
      switch (style->mode)
      {
      case ZMAPSTYLE_MODE_BASIC:
      case ZMAPSTYLE_MODE_TRANSCRIPT:
      case ZMAPSTYLE_MODE_ALIGNMENT:
      case ZMAPSTYLE_MODE_ASSEMBLY_PATH:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      case ZMAPSTYLE_MODE_SEQUENCE:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_META:
        {
          /* We shouldn't be displaying meta styles.... */
          valid = FALSE ;
          code = 5 ;
          message = g_strdup("Meta Styles cannot be displayed.") ;
          break ;
        }
      case ZMAPSTYLE_MODE_TEXT:
      case ZMAPSTYLE_MODE_GRAPH:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_GLYPH:
        {
          if (style->mode_data.glyph.glyph.n_coords < 2 &&
              style->mode_data.glyph.glyph5.n_coords < 2 &&
              style->mode_data.glyph.glyph3.n_coords < 2)
            {
            valid = FALSE ;
            code = 10 ;
            message = g_strdup("No Glyph defined.") ;
            }
          break ;
        }
      default:
        {
          zMapAssertNotReached() ;
        }
      }
    }
  if (valid && !zMapStyleIsPropertySetId(style,STYLE_PROP_BUMP_MODE))
    {
      valid = FALSE ;
      code = 3 ;
      message = g_strdup("Style initial bump mode not set.") ;
    }
  if (valid && !zMapStyleIsPropertySetId(style,STYLE_PROP_WIDTH))
     {
      valid = FALSE ;
      code = 4 ;
      message = g_strdup("Style width not set.") ;
    }

  /* What colours we need depends on the mode.at least a fill colour or a border colour (e.g. for transparent objects). */
  if (valid)
    {
      switch (style->mode)
      {
      case ZMAPSTYLE_MODE_BASIC:
      case ZMAPSTYLE_MODE_TRANSCRIPT:
      case ZMAPSTYLE_MODE_ALIGNMENT:
      case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
        {
          if (!(style->colours.normal.fields_set.fill) && !(style->colours.normal.fields_set.border))
            {
            valid = FALSE ;
            code = 5 ;
            message = g_strdup("Style does not have a fill or border colour.") ;
            }
          break ;
        }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      case ZMAPSTYLE_MODE_SEQUENCE:

      case ZMAPSTYLE_MODE_TEXT:
        {
          if (!(style->colours.normal.fields_set.fill) || !(style->colours.normal.fields_set.draw))
            {
            valid = FALSE ;
            code = 5 ;
            message = g_strdup("Text style requires fill and draw colours.") ;
            }
          break ;
        }
      case ZMAPSTYLE_MODE_GRAPH:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_GLYPH:
        {
//#if 0
//          if (style->glyph_mode == ZMAPSTYLE_GLYPH_3FRAME_SPLICE
          if(style->frame_mode > ZMAPSTYLE_3_FRAME_NEVER
            && (!(style->frame0_colours.normal.fields_set.fill)
                || !(style->frame1_colours.normal.fields_set.fill)
                || !(style->frame2_colours.normal.fields_set.fill)))
            {
            valid = FALSE ;
            code = 11 ;
            message = g_strdup_printf("Splice mode requires all frame colours to be set unset frames are:%s%s%s",
                                (style->frame0_colours.normal.fields_set.fill ? "" : " frame0"),
                                (style->frame1_colours.normal.fields_set.fill ? "" : " frame1"),
                                (style->frame2_colours.normal.fields_set.fill ? "" : " frame2")) ;
            }
            else
            {
               valid = FALSE ;
               code = 13 ;
            }
//#endif
          break ;
        }
      default:
        {
          zMapAssertNotReached() ;
        }
      }
    }

#if 0 // see comment at the top of this function
  /* Now do some mode specific stuff.... */
  if (valid)
    {
      switch (style->mode)
      {
      case ZMAPSTYLE_MODE_META:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_BASIC:
      case ZMAPSTYLE_MODE_ALIGNMENT:
	  {
		/* default summarise to 1000 to get round lack of configuration */
		if(!zMapStyleIsPropertySetId(style,STYLE_PROP_SUMMARISE))
		{
		  zmapStyleSetIsSet(style,STYLE_PROP_SUMMARISE);
		  style->summarise = 1000.0 ;
		}
		break;
	  }

      case ZMAPSTYLE_MODE_TRANSCRIPT:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_TEXT:
        {
          break ;
        }
      case ZMAPSTYLE_MODE_GRAPH:
        {
          break ;
        }
      default:
        {
          zMapAssertNotReached() ;
        }
      }
    }
#endif

  /* Construct the error if there was one. */
  if (!valid)
    *error = g_error_new_literal(domain, code, message) ;


  return valid ;
}



/* Takes a style and defaults enough properties for the style to be used to
 * draw features (albeit boringly), returns FALSE if style is set to non-displayable (e.g. for meta-styles). */
gboolean zMapStyleMakeDrawable(ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;
  zMapAssert(style) ;

  if (style->displayable)
    {
      /* These are the absolute basic requirements without which we can't display something.... */
      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_MODE))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_MODE);
        style->mode = ZMAPSTYLE_MODE_BASIC ;
      }

      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_COLUMN_DISPLAY_MODE))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_COLUMN_DISPLAY_MODE);
        style->col_display_state = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE ;
      }
      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_BUMP_MODE))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_BUMP_MODE);
        style->initial_bump_mode = ZMAPBUMP_UNBUMP ;
      }
      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_BUMP_DEFAULT) && style->mode != ZMAPSTYLE_MODE_GLYPH)
      {
        /* MH17: as glyphs are sub-features we can't bump
         * this would break 'column default bump mode'
         * feature type glyphs can have default-bump-mode' set explicitly in the style
         */
        zmapStyleSetIsSet(style,STYLE_PROP_BUMP_DEFAULT);
        style->default_bump_mode = ZMAPBUMP_UNBUMP ;
      }

      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_WIDTH))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_WIDTH);
        style->width = 1.0 ;
      }

      /* hash table lookup of styles from a container featureset is random and the first non zero is chosen
       * so a glyph attached to a pep-align can set the main style be not 3 frame
      */
      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_FRAME_MODE) && style->mode != ZMAPSTYLE_MODE_GLYPH)
      {
        zmapStyleSetIsSet(style,STYLE_PROP_FRAME_MODE);
        style->frame_mode = ZMAPSTYLE_3_FRAME_NEVER ;
      }


      switch (style->mode)
      {
      case ZMAPSTYLE_MODE_ASSEMBLY_PATH:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      case ZMAPSTYLE_MODE_SEQUENCE:

      case ZMAPSTYLE_MODE_TEXT:
        {
          if (!(style->colours.normal.fields_set.fill))
            zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                          "white", NULL, NULL) ;

          if (!(style->colours.normal.fields_set.draw))
            zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                          NULL, "black", NULL) ;

          break ;
        }
      case ZMAPSTYLE_MODE_GLYPH:
        {
            // originally we had glyph_mode = 3frame_splice
            // and there were no others
            // now we have glyph shapes as configured
            // see zmapFeature.c/addFeatureModeCB() which patches up the style if it's not good enough
            // or rather it doesn't
            // So for backwards compatability if [ZMap] legacy_styles=true
            // we add in glyphs to the style

          if(style->unique_id == g_quark_from_string("gf_splice"))   // as in acedbServer.c styles dump
            {
              if(!zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE) &&
                  (!zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_3) ||
                   !zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_5))
                  )
                {
                  ZMapFeatureTypeStyle s_3frame;

                  s_3frame = zMapStyleLegacyStyle(ZMAPSTYLE_LEGACY_3FRAME);
                  if(s_3frame)
                    zMapStyleMerge(style,s_3frame);
                }
            }
        }
        break;
      default:
        {
          if (!(style->colours.normal.fields_set.fill) && !(style->colours.normal.fields_set.border))
            {
            zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                            NULL, NULL, "black") ;
            }

          break ;
        }
      }

      result = TRUE ;
    }

  return result ;
}



ZMapStyleFullColour zmapStyleFullColour(ZMapFeatureTypeStyle style, ZMapStyleParamId id)
{
  ZMapStyleFullColour full_colour;

  switch (id)
    {
    case STYLE_PROP_COLOURS:
      full_colour = &(style->colours) ;
      break;
    case STYLE_PROP_FRAME0_COLOURS:
      full_colour = &(style->frame0_colours) ;
      break;
    case STYLE_PROP_FRAME1_COLOURS:
      full_colour = &(style->frame1_colours) ;
      break;
    case STYLE_PROP_FRAME2_COLOURS:
      full_colour = &(style->frame2_colours) ;
      break;
    case STYLE_PROP_REV_COLOURS:
      full_colour = &(style->strand_rev_colours) ;
      break;

    case STYLE_PROP_GLYPH_ALT_COLOURS:
      full_colour = &(style->mode_data.glyph.glyph_alt_colours) ;
      break;

    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
    case STYLE_PROP_ALIGNMENT_GAP_COLOURS:
      full_colour = &(style->mode_data.alignment.perfect) ;
      break;
    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
    case STYLE_PROP_ALIGNMENT_COMMON_COLOURS:
	    full_colour = &(style->mode_data.alignment.colinear) ;
      break;
    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
    case STYLE_PROP_ALIGNMENT_MIXED_COLOURS:
      full_colour = &(style->mode_data.alignment.noncolinear) ;
      break;

    case STYLE_PROP_SEQUENCE_NON_CODING_COLOURS:
      full_colour = &(style->mode_data.sequence.non_coding) ;
      break;
    case STYLE_PROP_SEQUENCE_CODING_COLOURS:
      full_colour = &(style->mode_data.sequence.coding) ;
      break;
    case STYLE_PROP_SEQUENCE_SPLIT_CODON_5_COLOURS:
      full_colour = &(style->mode_data.sequence.split_codon_5) ;
      break;
    case STYLE_PROP_SEQUENCE_SPLIT_CODON_3_COLOURS:
      full_colour = &(style->mode_data.sequence.split_codon_3) ;
      break;
    case STYLE_PROP_SEQUENCE_IN_FRAME_CODING_COLOURS:
      full_colour = &(style->mode_data.sequence.in_frame_coding) ;
      break;
    case STYLE_PROP_SEQUENCE_START_CODON_COLOURS:
      full_colour = &(style->mode_data.sequence.start_codon) ;
      break;
    case STYLE_PROP_SEQUENCE_STOP_CODON_COLOURS:
      full_colour = &(style->mode_data.sequence.stop_codon) ;
      break;

    case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
      full_colour = &(style->mode_data.transcript.CDS_colours) ;
      break;

    case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
      full_colour = &(style->mode_data.assembly_path.non_path_colours) ;
      break;

    default:
      zMapAssertNotReached() ;
      break;
    }
  return(full_colour);
}




ZMapStyleColour zmapStyleColour(ZMapStyleFullColour full_colour,ZMapStyleColourType type)
{
  ZMapStyleColour colour;

  switch (type)
    {
    case ZMAPSTYLE_COLOURTYPE_NORMAL:
      colour = &(full_colour->normal) ;
      break ;
    case ZMAPSTYLE_COLOURTYPE_SELECTED:
      colour = &(full_colour->selected) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }
    return(colour);
}


/* I'm going to try these more generalised functions.... */

gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
                       char *fill, char *draw, char *border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style) ;

  full_colour = zmapStyleFullColour(style, target);
  colour = zmapStyleColour(full_colour, type);

  if (!(result = setColours(colour, border, draw, fill)))
    {
      zMapLogCritical("Style \"%s\", bad colours specified:%s\"%s\"%s\"%s\"%s\"%s\"",
                  zMapStyleGetName(style),
                  border ? "  border " : "",
                  border ? border : "",
                  draw ? "  draw " : "",
                  draw ? draw : "",
                  fill ? "  fill " : "",
                  fill ? fill : "") ;
    }
  else
    {
      zmapStyleSetIsSet(style,target);
    }

  return result ;
}

gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
                       GdkColor **fill, GdkColor **draw, GdkColor **border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style && (fill || draw || border)) ;

  if (! zMapStyleIsPropertySetId(style,target))
    return(result);

  full_colour = zmapStyleFullColour(style,target);
  colour = zmapStyleColour(full_colour,type);

  if (fill)
    {
      if (colour->fields_set.fill)
      {
        *fill = &(colour->fill) ;
        result = TRUE ;
      }
    }

  if (draw)
    {
      if (colour->fields_set.draw)
      {
        *draw = &(colour->draw) ;
        result = TRUE ;
      }
    }

  if (border)
    {
      if (colour->fields_set.border)
      {
        *border = &(colour->border) ;
        result = TRUE ;
      }
    }

  return result ;
}


gboolean zMapStyleColourByStrand(ZMapFeatureTypeStyle style)
{
  gboolean colour_by_strand = FALSE;

  if(style->strand_rev_colours.normal.fields_set.fill ||
     style->strand_rev_colours.normal.fields_set.draw ||
     style->strand_rev_colours.normal.fields_set.border)
    colour_by_strand = TRUE;

  return colour_by_strand;
}



gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  gboolean is_colour = FALSE ;

  zMapAssert(style) ;

  switch(colour_context)
    {
    case ZMAPSTYLE_DRAW_FILL:
      is_colour = style->colours.normal.fields_set.fill ;
      break ;
    case ZMAPSTYLE_DRAW_DRAW:
      is_colour = style->colours.normal.fields_set.draw ;
      break ;
    case ZMAPSTYLE_DRAW_BORDER:
      is_colour = style->colours.normal.fields_set.border ;
      break ;
    default:
      zMapAssertNotReached() ;
    }

  return is_colour ;
}

GdkColor *zMapStyleGetColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  GdkColor *colour = NULL ;

  zMapAssert(style) ;

  if (zMapStyleIsColour(style, colour_context))
    {
      switch(colour_context)
      {
      case ZMAPSTYLE_DRAW_FILL:
        colour = &(style->colours.normal.fill) ;
        break ;
      case ZMAPSTYLE_DRAW_DRAW:
        colour = &(style->colours.normal.draw) ;
        break ;
      case ZMAPSTYLE_DRAW_BORDER:
        colour = &(style->colours.normal.fill) ;
        break ;
      default:
        zMapAssertNotReached() ;
      }
    }

  return colour ;
}




/*
 *                    ZMapStyle package only functions.
 */


gboolean zmapStyleIsValid(ZMapFeatureTypeStyle style)
{
  gboolean valid = FALSE ;

  if (style->original_id && style->unique_id)
    valid = TRUE ;

  return valid ;
}


/*
 *                    Internal functions.
 */



static gboolean setColours(ZMapStyleColour colour, char *border, char *draw, char *fill)
{
  gboolean status = TRUE ;
  ZMapStyleColourStruct tmp_colour = {{0}} ;


  zMapAssert(colour) ;

  if (status && border && *border)
    {
      if ((status = gdk_color_parse(border, &(tmp_colour.border))))
      {
        colour->fields_set.border = TRUE ;
        colour->border = tmp_colour.border ;

      }
    }
  if (status && draw && *draw)
    {
      if ((status = gdk_color_parse(draw, &(tmp_colour.draw))))
      {
        colour->fields_set.draw = TRUE ;
        colour->draw = tmp_colour.draw ;
      }
    }
  if (status && fill && *fill)
    {
      if ((status = gdk_color_parse(fill, &(tmp_colour.fill))))
      {
        colour->fields_set.fill = TRUE ;
        colour->fill = tmp_colour.fill ;
      }
    }

  return status ;
}



// store coordinate pairs in the struct and work out type
ZMapStyleGlyphShape zMapStyleGetGlyphShape(gchar *shape, GQuark id)
{
  gchar **spec,**segments,**s,**points,**p,*q;
  gboolean syntax = FALSE;
  gint x,y,n;
  gint *cp;
  ZMapStyleGlyphShape glyph_shape = g_new0(ZMapStyleGlyphShapeStruct,1);

  while(*shape && *shape <= ' ')
      shape++;

  spec = g_strsplit_set(shape,">)",2);   // strip trailing text including terminator

  if(*shape == '<')
      glyph_shape->type = GLYPH_DRAW_LINES;
  else if(*shape == '(')
      glyph_shape->type = GLYPH_DRAW_ARC;
  else
      syntax = TRUE;

  glyph_shape->id = id;
  glyph_shape->n_coords = 0;
  cp = glyph_shape->coords;

  if(!syntax)
    {
      segments = g_strsplit(shape+1,"/",0);      // get line breaks this way, then they don't have to be space delimited

      for(s = segments;*s;s++)
        {
           if(glyph_shape->n_coords)
             {
               glyph_shape->n_coords++;
               *cp++ = GLYPH_COORD_INVALID;
               *cp++ = GLYPH_COORD_INVALID;
               glyph_shape->type = GLYPH_DRAW_BROKEN;
             }

           points = g_strsplit_set(*s,";",0);

           for(p = points;*p;p++)
            {
              q = *p;
              if(!*q)
                continue; // multiple whitespace

              if((n = sscanf(q," %d , %d ",&x,&y)) == 2)  // should handle whitespace combinations
                {
                  glyph_shape->n_coords++;
                  *cp++ = x;
                  *cp++ = y;
                }
              else
                {
                  syntax = TRUE;
                  break;
                }
            }
        }
    }
  if(glyph_shape->type == GLYPH_DRAW_ARC)
    {
      // coords are TL and BR of bounding box, 0,0 = anchor point
      if(glyph_shape->n_coords == 2)
        {
          // NB: GDK angles are in 1/64 of a degree
          *cp++ = 0;          // whole circle
          *cp++ = 360;
          glyph_shape->n_coords = 3;
        }
      if(glyph_shape->n_coords != 3)
        {
          syntax = TRUE;
        }
    }
  if(spec)
      g_strfreev(spec);
  if(segments)
      g_strfreev(segments);
  if(points)
      g_strfreev(points);

  if(!syntax)
    {

      if(glyph_shape->type == GLYPH_DRAW_LINES && x == glyph_shape->coords[0] && y == glyph_shape->coords[1])
        {
          glyph_shape->type = GLYPH_DRAW_POLYGON;
        }
      return(glyph_shape);
    }

  g_free(glyph_shape);
  return(NULL);
}


// allow old ACEDB interface to use new configurable glyph styles
// we invent two styles pre3viosuly hard coded in bits
// only do this if [ZMap] legacy_styles=TRUE
ZMapFeatureTypeStyle zMapStyleLegacyStyle(char *name)
{
      static ZMapFeatureTypeStyle s_homology = NULL;
      static ZMapFeatureTypeStyle s_3frame = NULL;
      static int got = 0;
      char *hn;

      hn = (char *) zmapStyleSubFeature2ExactStr(ZMAPSTYLE_SUB_FEATURE_HOMOLOGY);

      if(!got)
      {
            got = 1;

            if(zMapConfigLegacyStyles())  // called here as we want to do it only once
            {
                  s_homology = zMapStyleCreate(hn, "homology - legacy style");

                  g_object_set(G_OBJECT(s_homology),
                        ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_GLYPH,

                        ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, "up-tri",
                        ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5, zMapStyleGetGlyphShape("<0,-4 ;-4,0 ;4,0 ;0,-4>", g_quark_from_string("up-tri")),
                        ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, "dn_tri",
                        ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3, zMapStyleGetGlyphShape("<0,4; -4,0 ;4,0; 0,4>",g_quark_from_string("dn-tri")),
                        ZMAPSTYLE_PROPERTY_SCORE_MODE, ZMAPSTYLE_SCORE_ALT,
                        ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD, 5,
                        ZMAPSTYLE_PROPERTY_COLOURS, "normal fill red; normal border black",
                        ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS, "normal fill green; normal border black",
                        NULL);

                  s_3frame = zMapStyleCreate(ZMAPSTYLE_LEGACY_3FRAME,"3-Frame - legacy style");

                  g_object_set(G_OBJECT(s_3frame),
                        ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_GLYPH,
                        // these have been swapped from the original
                        // GeneFinder uses 5' and 3' as Intron-centric
                        ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, "up-hook",
                        ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5, zMapStyleGetGlyphShape("<0,0; 15,0; 15,-10>",g_quark_from_string("up-hook")),
                        ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, "dn-hook",
                        ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3, zMapStyleGetGlyphShape("<0,0; 15,0; 15,10>",g_quark_from_string("dn_hook")),

                        ZMAPSTYLE_PROPERTY_FRAME_MODE, ZMAPSTYLE_3_FRAME_ONLY_1,
                        ZMAPSTYLE_PROPERTY_SCORE_MODE, ZMAPSCORE_WIDTH,

                        ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,FALSE,
                        ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,TRUE,                        ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,TRUE,

                        ZMAPSTYLE_PROPERTY_WIDTH,30.0,
                        ZMAPSTYLE_PROPERTY_MIN_SCORE,-2.0,
                        ZMAPSTYLE_PROPERTY_MAX_SCORE,4.0,

                        ZMAPSTYLE_PROPERTY_COLOURS, "normal fill grey",
                        ZMAPSTYLE_PROPERTY_FRAME0_COLOURS, "normal fill red; normal border red",
                        ZMAPSTYLE_PROPERTY_FRAME1_COLOURS, "normal fill green; normal border green",
                        ZMAPSTYLE_PROPERTY_FRAME2_COLOURS, "normal fill blue; normal border blue",
                        NULL);
            }
      }
      if(!strcmp(name,hn))
            return(s_homology);
      if(!strcmp(name,ZMAPSTYLE_LEGACY_3FRAME))
            return(s_3frame);
      return(NULL);
}


/* In the end this is called by the external StyleCreateNNN() functions. */
static ZMapFeatureTypeStyle styleCreate(guint n_parameters, GParameter *parameters)
{
  ZMapFeatureTypeStyle style = NULL ;

  if ((style = g_object_newv(ZMAP_TYPE_FEATURE_STYLE, n_parameters, parameters)))
    {
      /* We need to check that the style has a name etc because the newv call can create
       * the object but fail on setting properties. */
      if (!zmapStyleIsValid(style))
      {
        zMapStyleDestroy(style) ;
        style = NULL ;
      }
      else
      {
        /* I think we have to say that styles are displayable by default otherwise other bits of code
         * become impossible... */
        if (!zMapStyleIsPropertySetId(style,STYLE_PROP_DISPLAYABLE))
          {
            style->displayable = TRUE;
            zmapStyleSetIsSet(style,STYLE_PROP_DISPLAYABLE);
          }
      }
    }

  return style ;
}


// NOTE: this should contain _ALL_ ParamTypes

guint zmapStyleParamSize(ZMapStyleParamType type)
{
  switch(type)
  {

  case STYLE_PARAM_TYPE_FLAGS:       return(STYLE_IS_SET_SIZE); break;     // bitmap of is_set flags (array of uchar)
  case STYLE_PARAM_TYPE_QUARK:       return(sizeof(GQuark));    break;     // strings turned into integers by config file code
  case STYLE_PARAM_TYPE_BOOLEAN:     return(sizeof(gboolean));  break;
  case STYLE_PARAM_TYPE_UINT:        return(sizeof(guint));     break;     // we don't use INT !!
  case STYLE_PARAM_TYPE_DOUBLE:      return(sizeof(double));    break;
  case STYLE_PARAM_TYPE_STRING:      return(sizeof(gchar *));   break;     // but needs a strdup for copy
  case STYLE_PARAM_TYPE_COLOUR:      return(sizeof(ZMapStyleFullColourStruct)); break;      // a real structure
  case STYLE_PARAM_TYPE_SQUARK:      return(sizeof(GQuark));    break;     // gchar * stored as a quark eg name

    // enums: size not defined by ansi C
  case STYLE_PARAM_TYPE_MODE:        return(sizeof(ZMapStyleMode));       break;
  case STYLE_PARAM_TYPE_COLDISP:     return(sizeof(ZMapStyleColumnDisplayState)); break;
  case STYLE_PARAM_TYPE_BUMP:        return(sizeof(ZMapStyleBumpMode));   break;
  case STYLE_PARAM_TYPE_3FRAME:      return(sizeof(ZMapStyle3FrameMode)); break;
  case STYLE_PARAM_TYPE_SCORE:       return(sizeof(ZMapStyleScoreMode));  break;
  case STYLE_PARAM_TYPE_GRAPH_MODE:  return(sizeof(ZMapStyleGraphMode));  break;
  case STYLE_PARAM_TYPE_GRAPH_SCALE: return(sizeof(ZMapStyleGraphScale));  break;
  case STYLE_PARAM_TYPE_BLIXEM:      return(sizeof(ZMapStyleBlixemType)); break;
  case STYLE_PARAM_TYPE_GLYPH_STRAND:return(sizeof(ZMapStyleGlyphStrand)); break;
  case STYLE_PARAM_TYPE_GLYPH_ALIGN: return(sizeof(ZMapStyleGlyphAlign)); break;

  case STYLE_PARAM_TYPE_GLYPH_SHAPE: return(sizeof(ZMapStyleGlyphShapeStruct)); break;
  case STYLE_PARAM_TYPE_SUB_FEATURES:return(sizeof(GQuark) * ZMAPSTYLE_SUB_FEATURE_MAX); break;

  case STYLE_PARAM_TYPE_QUARK_LIST_ID: return(sizeof (GList *)); break;

  case STYLE_PARAM_TYPE_INVALID:
  default:
    zMapAssertNotReached();
    break;
  }
  return(0);
}


// called once
void zmap_param_spec_init(ZMapStyleParam param)
{
  GParamSpec *gps = NULL;

  switch(param->type)
  {
    case STYLE_PARAM_TYPE_BOOLEAN:

      gps = g_param_spec_boolean(param->name, param->nick,param->blurb,
                  FALSE, ZMAP_PARAM_STATIC_RW);
      break;

    case STYLE_PARAM_TYPE_DOUBLE:

      gps = g_param_spec_double(param->name, param->nick,param->blurb,
                  -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
                  ZMAP_PARAM_STATIC_RW);
      param->size = sizeof(double);
      break;

    case STYLE_PARAM_TYPE_QUARK:              // quark is a string turned into a number but we handle the number internally
                                              // config file feeds this data in as a number due to old interface
      gps = g_param_spec_uint(param->name, param->nick,param->blurb,
                  0, G_MAXUINT, 0, ZMAP_PARAM_STATIC_RW);
      param->size = sizeof(GQuark);
      break;

    case STYLE_PARAM_TYPE_STRING:              // gchar *
    case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
    case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)

      gps = g_param_spec_string(param->name, param->nick,param->blurb,
                  "", ZMAP_PARAM_STATIC_RW);
      break;

    case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
    case STYLE_PARAM_TYPE_SUB_FEATURES:        // GQuark[]
    case STYLE_PARAM_TYPE_QUARK_LIST_ID:       // GList * of GQuark
      gps = g_param_spec_string(param->name, param->nick,param->blurb,
                  "", ZMAP_PARAM_STATIC_RW);
      break;

    case STYLE_PARAM_TYPE_GLYPH_SHAPE:         // ZMapStyleGlyphShapeStruct

      gps = g_param_spec_boxed (param->name, param->nick,param->blurb,
                  zMapStyleGlyphShapeGetType(), G_PARAM_READWRITE);
      break;


    // enums treated as uint, but beware: ANSI C leave the size unspecifies
    // so it's compiler dependant and could be changed via optimisation flags

    case STYLE_PARAM_TYPE_MODE:                // ZMapStyleMode
    case STYLE_PARAM_TYPE_COLDISP:             // ZMapStyleColumnDisplayState
    case STYLE_PARAM_TYPE_BUMP:                // ZMapStyleBumpMode
    case STYLE_PARAM_TYPE_3FRAME:              // ZMapStyle3FrameMode
    case STYLE_PARAM_TYPE_SCORE:               // ZMapStyleScoreMode
    case STYLE_PARAM_TYPE_GRAPH_MODE:          // ZMapStyleGraphMode
    case STYLE_PARAM_TYPE_GRAPH_SCALE:         // ZMapStyleGraphScale
    case STYLE_PARAM_TYPE_BLIXEM:              // ZMapStyleBlixemType
    case STYLE_PARAM_TYPE_GLYPH_STRAND:        // ZMapStyleGlyphStrand
    case STYLE_PARAM_TYPE_GLYPH_ALIGN:         // ZMapStyleGlyphAlign

    case STYLE_PARAM_TYPE_UINT:

      gps = g_param_spec_uint(param->name, param->nick,param->blurb,
                  0, G_MAXUINT, 0,        // we don't use these, but if we wanted to we could do it in ....object_set()
                  ZMAP_PARAM_STATIC_RW); // RO is available but we don't use it
      break;

    default:
      zMapAssertNotReached();
      break;
  }

      // point this param at its is_set bit
  param->flag_ind = param->id / 8;
  param->flag_bit = 1 << (param->id % 8);

  param->size = zmapStyleParamSize(param->type);
  param->spec = gps;
}

/* When adding new properties be sure to have only "[a-z]-" in the name and make sure
 * to add set/get property entries in those functions.
 *  */
static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class)
{
  GObjectClass *gobject_class;
  ZMapStyleParam param;
  int i;

  gobject_class = (GObjectClass *)style_class;

  gobject_class->set_property = zmap_feature_type_style_set_property;
  gobject_class->get_property = zmap_feature_type_style_get_property;

// may need to tweak the GValueTable for this ?? ref to zmapBase.c
// gobject_class->copy_set_proprty = zmap_feature_type_style_copy_set_property;

  style_parent_class_G = g_type_class_peek_parent(style_class);

  for(i = 1, param = &zmapStyleParams_G[i];i < _STYLE_PROP_N_ITEMS;i++,param++)
    {
      zmap_param_spec_init(param);

      g_object_class_install_property(gobject_class,param->id,param->spec);
    }

  gobject_class->dispose  = zmap_feature_type_style_dispose;
  gobject_class->finalize = zmap_feature_type_style_finalize;


  return ;
}


static void zmap_feature_type_style_init(ZMapFeatureTypeStyle style)
{
  zmapStyleSetIsSet(style,STYLE_PROP_IS_SET);

      // default values, they don't count as being set for inheritance
      // but will be returned if a paramter is not set
      // ** only need to set if non zero **


  return ;
}




/* This function is called both for straight SETTING of object properties and for COPYING
 * of objects. During a copy, the "get_property" routine of one object is called to retrieve
 * a value and the "set_property" routine of the new object is called to set that value.
 *
 * For copies, the original style is stored on the style so we can retrieve it and do
 * a "deep" copy because bizarrely gobjects interface does not give you access to the
 * original style !
 *
 * We return a status code in the property ZMAPSTYLE_OBJ_RC so that callers can detect
 * whether this function succeeded or not.
 *  */

static void zmap_feature_type_style_set_property(GObject *gobject,
						 guint param_id,
						 const GValue *value,
						 GParamSpec *pspec)
{
  ZMapFeatureTypeStyle style;
  ZMapStyleParam param;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));
  style = ZMAP_FEATURE_STYLE(gobject);
  param = &zmapStyleParams_G[param_id];

  zmap_feature_type_style_set_property_full(style,param,value,FALSE);
}






// is_set flags are stored as bits in a char array
// for g_object we extract this as a string of hex digits in upper case
static void zmap_hex_to_bin(guchar *dest, gchar *src, int len)
{
  gchar hex[] = "0123456789ABCDEF";
  guchar val;
  gint digit;
  int i;

  for(i = 0; i < len; i++)
  {
      digit = '0';
      if(*src)
        digit = *src++;

      digit &= 0x5f;    // uppercase
      val = digit - hex[digit];
      val <<= 4;

      digit = '0';
      if(*src)
        digit = *src++;

      digit &= 0x5f;    // uppercase
      val += digit - hex[digit];
  }
}


static void zmap_bin_to_hex(gchar *dest,guchar *src, int len)
{
  gchar hex[] = "0123456789ABCDEF";
  guchar val;
  gchar msd,lsd;
  int i;

  for(i = 0; i < len; i++)
  {
      val = *src++;

      lsd = hex[(val & 0xf)];
      val >>= 4;
      msd = hex[(val &0xf)];

      *dest++ = msd;
      *dest++ = lsd;
  }
  *dest = 0;
}

static void zmap_feature_type_style_set_property_full(ZMapFeatureTypeStyle style,
						      ZMapStyleParam param,
						      const GValue *value,
						      gboolean copy)
{
  gboolean result = TRUE ;

  // if we are in a copy constructor then only set properties that have been set in the source
  // (except for the is_set array of course, which must be copied first)

  /* will we ever be in a copy constructor?
   * we use zMapStyleCopy() rather than a g_object(big_complex_value)
   * if someone programmed a GObject copy function what would happen?
   * ... an exercise for the reader
   */
  if(copy && param->id != STYLE_PROP_IS_SET)
    {
      ZMapStyleParam isp = &zmapStyleParams_G[STYLE_PROP_IS_SET];

      zMapAssert((style->is_set[isp->flag_ind] & isp->flag_bit));
      /* if not we are in deep doo doo
       * This relies on STYLE_PROP_IS_SET being the first installed or
       * lowest numbered one and therefore the first one copied
       * if GLib changes such that we get an Assert here then we need to
       * implement a copy constructor that forces this
       */

      // only copy paramters that have been set in the source
      // as we copied the is_set array we can read our own copy of it
      if(!(style->is_set[param->flag_ind] & param->flag_bit))
        return;
    }
  /* if the property is mode specific and invalid don't set it
   * ideally all styles should have a mode set
   * but if the mode is set in a parent style it may not be set
   * explicitly in this style
   * So we imply the mode here and if it does not match a parent
   * The we log a warning in the merge
   */
  if(param->mode && !style->mode)   // ie not set, could test the is_set bit but this is equivalent
    {
      style->mode = param->mode;
      zmapStyleSetIsSet(style,STYLE_PROP_MODE);
    }

  if(param->mode && style->mode != param->mode)
    {
      zMapLogWarning("Set style mode specific paramter %s ignored as mode is %s",param->name,zMapStyleMode2ExactStr(style->mode));
      return;
    }

  // set the value
  switch(param->type)
    {
    case STYLE_PARAM_TYPE_BOOLEAN:
      * (gboolean *) (((void *) style) + param->offset) = g_value_get_boolean(value);
      break;
    case STYLE_PARAM_TYPE_DOUBLE:
      * (double *) (((void *) style) + param->offset)   = g_value_get_double(value);
      break;

    case STYLE_PARAM_TYPE_STRING:              // gchar *
      * (gchar **) (((void *) style) + param->offset)    = g_strdup( g_value_get_string(value));
      break;

    case STYLE_PARAM_TYPE_QUARK:
      * (GQuark *) (((void *) style) + param->offset)   =  g_value_get_uint(value);
      break;

    case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
      * (GQuark *) (((void *) style) + param->offset)   = g_quark_from_string(g_value_get_string(value));
      break;

    case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)
      zmap_hex_to_bin((((void *) style) + param->offset), (gchar *) g_value_get_string(value), STYLE_IS_SET_SIZE);
      break;

    case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
      result = parseColours(style, param->id, (GValue *) value) ;
      break;

    case STYLE_PARAM_TYPE_SUB_FEATURES:        // GQuark[]
      result = parseSubFeatures(style, (gchar *) g_value_get_string(value) );
      break;

    case STYLE_PARAM_TYPE_QUARK_LIST_ID:
      * (GList **) (((void *) style) + param->offset)   = zMapConfigString2QuarkList( (gchar *) g_value_get_string(value) ,TRUE);
      break;

    case STYLE_PARAM_TYPE_GLYPH_SHAPE:          // copy structure into ours
      memcpy((((void *) style) + param->offset),g_value_get_boxed(value),sizeof(ZMapStyleGlyphShapeStruct));
      break;

      // enums treated as uint. This is a pain: can we know how big an enum is?
      // Some pretty choice code but it's not safe to do it the easy way
#define STYLE_SET_PROP(s_param, s_type)					\
      case s_param : *(s_type *)  (((void *) style) + param->offset) = (s_type) g_value_get_uint(value); \
      break

      STYLE_SET_PROP (STYLE_PARAM_TYPE_MODE,            ZMapStyleMode);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_COLDISP,         ZMapStyleColumnDisplayState);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_BUMP,            ZMapStyleBumpMode);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_3FRAME,          ZMapStyle3FrameMode);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_SCORE,           ZMapStyleScoreMode);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_GRAPH_MODE,      ZMapStyleGraphMode);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_GRAPH_SCALE,     ZMapStyleGraphScale);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_BLIXEM,          ZMapStyleBlixemType);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_GLYPH_STRAND,    ZMapStyleGlyphStrand);
      STYLE_SET_PROP (STYLE_PARAM_TYPE_GLYPH_ALIGN,     ZMapStyleGlyphAlign);

    case STYLE_PARAM_TYPE_UINT:
      * (guint *) (((void *) style) + param->offset) = g_value_get_uint(value);
      break;

    default:
      break;
    }

  // now set the is_set bit
  style->is_set[param->flag_ind] |= param->flag_bit;


  // do the param specific stuff
  {
    ZMapStyleParam p2;

    switch(param->id)
      {
      case STYLE_PROP_NAME:
        p2 = &zmapStyleParams_G[STYLE_PROP_ORIGINAL_ID];    // sourced from same data
        style->is_set[p2->flag_ind] |= p2->flag_bit;

        /* strictly speaking, shoould we regenerate the unique id as well?
         * previous code did not, we always create a style with a name, so this should be safe
         */
        style->unique_id   = zMapStyleCreateID((char *)g_quark_to_string(style->original_id));
        p2 = &zmapStyleParams_G[STYLE_PROP_UNIQUE_ID];
        style->is_set[p2->flag_ind] |= p2->flag_bit;

        break;

      case STYLE_PROP_ORIGINAL_ID:
        p2 = &zmapStyleParams_G[STYLE_PROP_NAME];           // sourced from same data
        style->is_set[p2->flag_ind] |= p2->flag_bit;
        break;

      case STYLE_PROP_BUMP_MODE:
        p2 = &zmapStyleParams_G[STYLE_PROP_BUMP_DEFAULT];
        if(!(style->is_set[p2->flag_ind] & p2->flag_bit))
          {
            style->default_bump_mode = style->initial_bump_mode;
            style->is_set[p2->flag_ind] |= p2->flag_bit;
          }

        break;

      case STYLE_PROP_MODE:
	// set non zero default vaues for mode data
        if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
	  {
            style->mode_data.alignment.unmarked_colinear = TRUE;
	  }
        break;
      default:
        break;
      }
  }

  /* Now set the result so we can return it to the user. */
  g_object_set_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC, GINT_TO_POINTER(result)) ;


  return ;
}



/* Called for all "get" calls to retrieve style properties.
 *
 * Note that the get might be called as part of a style copy in which case the gobject
 * code will try to set _all_ properties so we have to deal with this for the mode
 * specific properties by ignoring inappropriate ones.
 *
 * We return a status code in the property ZMAPSTYLE_OBJ_RC so that callers can detect
 * whether this function succeeded or not.
 *
 * this function pays no attention to the field_set data...is this correct ???? */
static void zmap_feature_type_style_get_property(GObject *gobject,
                                     guint param_id,
                                     GValue *value,
                                     GParamSpec *pspec)
{
  ZMapFeatureTypeStyle style;
  gboolean result = TRUE ;
  gchar *colour = NULL;
  gchar *subs = NULL;
  gchar * flags;
  ZMapStyleParam param = &zmapStyleParams_G[param_id];
  gchar *shape = NULL;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));

  style = ZMAP_FEATURE_STYLE(gobject);


      /* if the property has not been set just return
       * by implication mode dependant data has not been set
       * if not related to the styles mode so we have nothing to do
       * we could test param->mode and style->mode but is_set cover all possibilities
       */
  if(!(style->is_set[param->flag_ind] & param->flag_bit))
        return;

  if(param->mode && param->mode != style->mode)
    {
      zMapLogWarning("Get style mode specific parameter %s ignored as mode is %s",param->name,zMapStyleMode2ExactStr(style->mode));
      return;
    }

      // get the value
  switch(param->type)
  {
    case STYLE_PARAM_TYPE_BOOLEAN:
      g_value_set_boolean(value, * (gboolean *) (((void *) style) + param->offset));
      break;
    case STYLE_PARAM_TYPE_DOUBLE:
      g_value_set_double(value, * (double *) (((void *) style) + param->offset));
      break;

    case STYLE_PARAM_TYPE_STRING:              // gchar *
      g_value_set_string(value, (gchar *) (((void *) style) + param->offset));
      break;

    case STYLE_PARAM_TYPE_QUARK:
      g_value_set_uint(value, (guint) (* (GQuark *) (((void *) style) + param->offset)));
      break;

    case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
      g_value_set_string(value, g_quark_to_string(* (GQuark *) (((void *) style) + param->offset)));
      break;

    case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)
      flags = g_malloc(STYLE_IS_SET_SIZE * 2 + 1);

      zmap_bin_to_hex(flags,(((void *) style) + param->offset), STYLE_IS_SET_SIZE);

      g_value_set_string(value,flags);
      g_free(flags);
      break;

    case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
      colour = zmapStyleValueColour((ZMapStyleFullColour) (((void *) style) + param->offset));
      if(colour)
        {
//           g_value_set_string(value, strdup(colour));
           g_value_set_string(value, colour);
           g_free(colour);
        }
      else
        {
           result = FALSE;
        }
      break;

    case STYLE_PARAM_TYPE_SUB_FEATURES:        // GQuark[]
      subs = zmapStyleValueSubFeatures((GQuark *)(((void *) style) + param->offset));
      if(subs)
        {
//           g_value_set_string(value, strdup(subs));
           g_value_set_string(value, subs);
           g_free(colour);
        }
      else
        {
           result = FALSE;
        }
      break;

    case STYLE_PARAM_TYPE_QUARK_LIST_ID:
      {
        gchar *str;
        str = zMap_g_list_quark_to_string(* (GList **) (((void *) style) + param->offset));
        g_value_set_string(value, str);
        g_free(str);
      }
      break;

    case STYLE_PARAM_TYPE_GLYPH_SHAPE:
      g_value_set_boxed(value, shape);
      break;


      // enums treated as uint. This is a pain: can we know how big an enum is? (NO)
      // Some pretty choice code but it's not safe to do it the easy way
#define STYLE_GET_PROP(s_param,s_type)\
      case s_param : g_value_set_uint(value, (guint) (* (s_type *) (((void *) style) + param->offset)));\
      break

    STYLE_GET_PROP (STYLE_PARAM_TYPE_MODE            , ZMapStyleMode);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_COLDISP         , ZMapStyleColumnDisplayState);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_BUMP            , ZMapStyleBumpMode);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_3FRAME          , ZMapStyle3FrameMode);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_SCORE           , ZMapStyleScoreMode);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_GRAPH_MODE      , ZMapStyleGraphMode);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_GRAPH_SCALE     , ZMapStyleGraphScale);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_BLIXEM          , ZMapStyleBlixemType);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_GLYPH_STRAND    , ZMapStyleGlyphStrand);
    STYLE_GET_PROP (STYLE_PARAM_TYPE_GLYPH_ALIGN     , ZMapStyleGlyphAlign);

    case STYLE_PARAM_TYPE_UINT:
      g_value_set_uint(value, * (guint *) (((void *) style) + param->offset));
      break;

    default:
      zMapAssertNotReached();
      break;
  }


  /* Now set the result so we can return it to the user. */
  g_object_set_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC, GINT_TO_POINTER(result)) ;

  return;
}


gchar *zmapStyleValueColour(ZMapStyleFullColour this_colour)
{
      /* We allocate memory here to hold the colour string, it's g_object_get caller's
       * responsibility to g_free the string... */

      GString *colour_string = NULL ;
      gchar *colour = NULL;

      colour_string = g_string_sized_new(500) ;

      formatColours(colour_string, "normal", &(this_colour->normal)) ;
      formatColours(colour_string, "selected", &(this_colour->selected)) ;

      if (colour_string->len)
        colour = g_string_free(colour_string, FALSE) ;

      return(colour);
}


static gchar *zmapStyleValueSubFeatures(GQuark *quarks)
{

      GString *subs_string = NULL ;
      gchar *subs = NULL;
      int i;

      subs_string = g_string_sized_new(500) ;

      for(i = 1;i < ZMAPSTYLE_SUB_FEATURE_MAX;i++)
      {
            if(quarks[i])
            {
                g_string_append_printf(subs_string, "%s:%s ; ",
                     zmapStyleSubFeature2ExactStr(i),
                     g_quark_to_string(quarks[i]));
            }
      }
      if (subs_string->len)
        subs = g_string_free(subs_string, FALSE) ;

      return(subs);
}

static gboolean parseSubFeatures(ZMapFeatureTypeStyle style,gchar *str)
{
  gchar ** sub_strings,**ss;
  gchar **sub_feature;
  ZMapStyleSubFeature sf_id;
  gchar *name,*value;
  gboolean result = FALSE;

  sub_strings = g_strsplit(str,";",-1);
  if(sub_strings)
  {
    for(ss = sub_strings;*ss;ss++)
    {
      if(!**ss)
            continue;
      sub_feature = g_strsplit(*ss,":",3);
      if(sub_feature)
      {
        name = sub_feature[0];
        if(name)
        {
          name = g_strstrip(name);
          value = sub_feature[1];
          if(value)
          {
            sf_id = zMapStyleStr2SubFeature(name);
            if(sf_id)
            {
              value = g_strstrip(value);
              style->sub_features[sf_id] = g_quark_from_string(value);
              result = TRUE;
            }
          }
        }
        g_strfreev(sub_feature);
      }
    }

    g_strfreev(sub_strings);
  }
  return(result);
}

/* Parse out colour triplets from a colour keyword-value line.
 *
 * The line format is:
 *
 * NNN_colours = <normal | selected> <fill | draw | border> <colour> ;
 *
 * the colour spec may be repeated up to 6 times to specify fill, draw or border colours
 * for normal or selected features. Each triplet is separated by a ";".
 *
 * NOTE that in the triplet, the colour specifier may consist of more than one word
 * (e.g. "dark slate gray") so the g_strsplit_set() is done into just three fields
 * and all text in the last field is stored as the colour specifier.
 *
 *  */
static gboolean parseColours(ZMapFeatureTypeStyle style, guint param_id, GValue *value)
{
  gboolean result = TRUE ;
  char *string = NULL ;
  char **colour_strings = NULL ;

  if ((string = (char *)g_value_get_string(value))
      && (colour_strings = g_strsplit(string, ";", -1)))
    {
      char **curr_str ;
      int num_specs = 0 ;

      curr_str = colour_strings ;

      /* Parse up to 6 colour specs.  */
      while (num_specs < ZMAPSTYLE_MAX_COLOUR_SPECS && *curr_str && **curr_str)
      {
        char **col_spec = NULL ;
        ZMapStyleColourType type = ZMAPSTYLE_COLOURTYPE_INVALID ;
        ZMapStyleDrawContext context = ZMAPSTYLE_DRAW_INVALID ;
        char *colour = NULL ;

        /* Chops leading/trailing blanks off colour triplet, splits and then validates the
         * triplet. If all is ok then try and parse the colour into the appropriate style
         * field. */
        if ((*curr_str = g_strstrip(*curr_str)) && **curr_str
            && (col_spec = g_strsplit_set(*curr_str, " \t", 3))
            && validSplit(col_spec, &type, &context, &colour))
          {
            ZMapStyleFullColour full_colour = NULL;
            ZMapStyleColour type_colour = NULL;

            full_colour = zmapStyleFullColour(style,param_id);
            type_colour = zmapStyleColour(full_colour,type);

            switch (context)
            {
            case ZMAPSTYLE_DRAW_FILL:
              if (!(type_colour->fields_set.fill = gdk_color_parse(colour, &(type_colour->fill))))
                result = FALSE ;
              break ;
            case ZMAPSTYLE_DRAW_DRAW:
              if (!(type_colour->fields_set.draw = gdk_color_parse(colour, &(type_colour->draw))))
                result = FALSE ;
              break ;
            case ZMAPSTYLE_DRAW_BORDER:
              if (!(type_colour->fields_set.border = gdk_color_parse(colour, &(type_colour->border))))
                result = FALSE ;
              break ;
            default:
              zMapAssertNotReached() ;
              break;
            }
          }
        else
          {
            result = FALSE ;
          }

        if (col_spec)
          g_strfreev(col_spec)  ;

        curr_str++ ;
        num_specs++ ;
      }
    }

  if (colour_strings)
    g_strfreev(colour_strings)  ;

  return result ;
}


#ifdef MH17_NOT_USED
static gboolean isColourSet(ZMapFeatureTypeStyle style, int param_id, char *subpart)
{
  gboolean is_set = FALSE ;
  char *full_colour ;
  char **col_spec = NULL ;
  ZMapStyleColourType type = ZMAPSTYLE_COLOURTYPE_INVALID ;
  ZMapStyleDrawContext context = ZMAPSTYLE_DRAW_INVALID ;
  char *dummy_colour = NULL ;

  full_colour = g_strdup_printf("%s dummy_colour", subpart) ;

  if ((full_colour = g_strstrip(full_colour))
      && (col_spec = g_strsplit_set(full_colour, " \t", 3))
      && validSplit(col_spec, &type, &context, &dummy_colour))
    {
      ZMapStyleFullColour this_colour = NULL ;
      ZMapStyleColour type_colour = NULL ;

      this_colour = zmapStyleFullColour(style,param_id);
      type_colour = zmapStyleColour(this_colour,type);

      switch (context)
      {
      case ZMAPSTYLE_DRAW_FILL:
        is_set = type_colour->fields_set.fill ;
        break ;
      case ZMAPSTYLE_DRAW_DRAW:
        is_set = type_colour->fields_set.draw ;
        break ;
      case ZMAPSTYLE_DRAW_BORDER:
        is_set = type_colour->fields_set.border ;
        break ;
      default:
        zMapAssertNotReached() ;
        break;
      }
    }

  return is_set ;
}
#endif

/* strings should be a NULL terminated array of three string pointers, the
 * contents of the three strings must be:
 *
 *  first string:     "normal" or "selected"
 *
 * second string:     "fill" or "draw" or "border"
 *
 *  third string:     A colour spec in X11 rgb format (not validated by this routine)
 *
 */
static gboolean validSplit(char **strings,
                     ZMapStyleColourType *type_out, ZMapStyleDrawContext *context_out, char **colour_out)
{
  gboolean valid = TRUE ;
  ZMapStyleColourType type = ZMAPSTYLE_COLOURTYPE_INVALID ;
  ZMapStyleDrawContext context = ZMAPSTYLE_DRAW_INVALID ;
  char *colour = NULL ;
  char **curr ;
  int num ;

  num = 0 ;
  curr = strings ;
  while (valid && curr && *curr)
    {
      switch (num)
      {
      case 0:
        {
          if ((type = zMapStyleStr2ColourType(*curr)))
            num++ ;
          else
            valid = FALSE ;
          break ;
        }
      case 1:
        {
          if ((context = zMapStyleStr2DrawContext(*curr)))
            num++ ;
          else
            valid = FALSE ;
          break ;
        }
      case 2:
        {
          colour = *curr ;
          num++ ;
          break ;
        }
      default:
        {
          zMapAssertNotReached() ;
          break ;
        }
      }

      if (valid)
      curr++ ;
    }


  if (valid && num == 3)
    {
      *type_out = type ;
      *context_out = context ;
      *colour_out = colour ;
    }

  return valid ;
}


/* Format colours into standard triplet format. */
static void formatColours(GString *colour_string, char *type, ZMapStyleColour colour)
{
  if (colour->fields_set.fill)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                     type, "fill",
                     colour->fill.red, colour->fill.green, colour->fill.blue) ;

  if (colour->fields_set.draw)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                     type, "draw",
                     colour->draw.red, colour->draw.green, colour->draw.blue) ;

  if (colour->fields_set.border)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                     type, "border",
                     colour->border.red, colour->border.green, colour->border.blue) ;

  return ;
}


