/*  File: zmapStyle.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Style and Style set handling functions.
 *
 * HISTORY:
 * Last edited: Jan 26 08:42 2010 (edgrif)
 * Created: Mon Feb 26 09:28:26 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.h,v 1.56 2010-04-20 12:00:37 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_H
#define ZMAP_STYLE_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapIO.h>



// glyph shape definitions
#define GLYPH_SHAPE_MAX_POINT       32          // max number of points ie 16 coordinates including breaks
#define GLYPH_SHAPE_MAX_COORD       16
#define GLYPH_COORD_INVALID         1000        // or maybe -128 and store them as char
                                                // but we store arc start+end as degrees (0-360)
#define GLYPH_CANVAS_COORD_INVALID  (0.0)       // for the translated points (values is irrelevant but 0 is tidy)

typedef enum
{
      GLYPH_DRAW_INVALID,
      GLYPH_DRAW_LINES,       // a series of connected lines
      GLYPH_DRAW_BROKEN,      // connected lines with gaps
      GLYPH_DRAW_POLYGON,     // a wiggly loop that can be filled
      GLYPH_DRAW_ARC          // a circle ellipse or fraction of

} ZMapStyleGlyphDrawType;

/*
 * this could be made dynamic but really we don't want to have 100 points in a glyph
 * they are meant to be small and quick to draw
 * 16 points is plenty and we don't expect a huge number of shapes
 */
typedef struct _ZMapStyleGlyphShapeStruct      // defined here so that config can use it
  {
    gint coords[GLYPH_SHAPE_MAX_POINT];        // defined in pairs (x,y)
    gint n_coords;
    /*
     * break between lines flagged by GLYPH_COORD_INVALID
     * for circles/ellipses we have two points and optionally two angles
     * a break between lines takes up one corrdinate pair
     */
    ZMapStyleGlyphDrawType type;

  } ZMapStyleGlyphShapeStruct;

typedef ZMapStyleGlyphShapeStruct *ZMapStyleGlyphShape;

// stuff for G_BOXED data type so we can use the above
GType zMapStyleGlyphShapeGetType (void);




// STYLE_PROP_ identifies each property and is used to index the is_set array

typedef enum
  {
    STYLE_PROP_NONE,                                /* zero is invalid */

    /* STYLE_PROP_IS_SET must be first */
    STYLE_PROP_IS_SET,                              /* (internal/ automatic property) */

    STYLE_PROP_NAME,                                /* Name as text. */
    STYLE_PROP_ORIGINAL_ID,                         /* Name as a GQuark. */
    STYLE_PROP_UNIQUE_ID,


    STYLE_PROP_PARENT_STYLE,
    STYLE_PROP_DESCRIPTION,
    STYLE_PROP_MODE,

    STYLE_PROP_COLOURS,
    STYLE_PROP_FRAME0_COLOURS,
    STYLE_PROP_FRAME1_COLOURS,
    STYLE_PROP_FRAME2_COLOURS,
    STYLE_PROP_REV_COLOURS,

    STYLE_PROP_COLUMN_DISPLAY_MODE,

    STYLE_PROP_BUMP_DEFAULT,
    STYLE_PROP_BUMP_MODE,
    STYLE_PROP_BUMP_FIXED,
    STYLE_PROP_BUMP_SPACING,

    STYLE_PROP_FRAME_MODE,

    STYLE_PROP_MIN_MAG,
    STYLE_PROP_MAX_MAG,

    STYLE_PROP_WIDTH,

    STYLE_PROP_SCORE_MODE,
    STYLE_PROP_MIN_SCORE,
    STYLE_PROP_MAX_SCORE,

    STYLE_PROP_GFF_SOURCE,
    STYLE_PROP_GFF_FEATURE,

    STYLE_PROP_DISPLAYABLE,
    STYLE_PROP_SHOW_WHEN_EMPTY,
    STYLE_PROP_SHOW_TEXT,
    STYLE_PROP_SUB_FEATURES,

    STYLE_PROP_STRAND_SPECIFIC,
    STYLE_PROP_SHOW_REV_STRAND,
    STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
    STYLE_PROP_DIRECTIONAL_ENDS,

    STYLE_PROP_DEFERRED,
    STYLE_PROP_LOADED,

    // mode dependant data


    STYLE_PROP_GLYPH_NAME,
    STYLE_PROP_GLYPH_SHAPE,
    STYLE_PROP_GLYPH_NAME_5,
    STYLE_PROP_GLYPH_SHAPE_5,
    STYLE_PROP_GLYPH_NAME_3,
    STYLE_PROP_GLYPH_SHAPE_3,
    STYLE_PROP_GLYPH_ALT_COLOURS,
    STYLE_PROP_GLYPH_THRESHOLD,
    STYLE_PROP_GLYPH_STRAND,
    STYLE_PROP_GLYPH_ALIGN,

    STYLE_PROP_GRAPH_MODE,
    STYLE_PROP_GRAPH_BASELINE,

    STYLE_PROP_ALIGNMENT_PARSE_GAPS,
    STYLE_PROP_ALIGNMENT_SHOW_GAPS,
    STYLE_PROP_ALIGNMENT_BETWEEN_ERROR,
    STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN,
    STYLE_PROP_ALIGNMENT_PFETCHABLE,
    STYLE_PROP_ALIGNMENT_BLIXEM,
    STYLE_PROP_ALIGNMENT_PERFECT_COLOURS,
    STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS,
    STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS,
    STYLE_PROP_ALIGNMENT_UNMARKED_COLINEAR,

    STYLE_PROP_TRANSCRIPT_CDS_COLOURS,

    STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS,

    STYLE_PROP_TEXT_FONT,

    _STYLE_PROP_N_ITEMS        // not a property but used in some macros

  } ZMapStyleParamId;



/* All the properties that can be set on a style object. NOTE that I would have liked to have
 * done this as a list (as for other strings/enums below) from which to automatically generate
 * #defines of the form #define ZMAPSTYLE_PROPERTY_NAME "name", BUT it's not possible to have
 * a macro that contains the '#' character so I can't have #define in the output of the macro
 * so I can't do it...sigh.... */

/* General Style properties... */
#define ZMAPSTYLE_PROPERTY_INVALID                "invalid"
#define ZMAPSTYLE_PROPERTY_IS_SET                 "is-set"        // internal property
#define ZMAPSTYLE_PROPERTY_NAME                   "name"
#define ZMAPSTYLE_PROPERTY_ORIGINAL_ID            "original-id"
#define ZMAPSTYLE_PROPERTY_UNIQUE_ID              "unique-id"
#define ZMAPSTYLE_PROPERTY_PARENT_STYLE           "parent-style"
#define ZMAPSTYLE_PROPERTY_DESCRIPTION            "description"
#define ZMAPSTYLE_PROPERTY_MODE                   "mode"
/* ... colours  */
#define ZMAPSTYLE_PROPERTY_COLOURS                "colours"
#define ZMAPSTYLE_PROPERTY_FRAME0_COLOURS         "frame0-colours"
#define ZMAPSTYLE_PROPERTY_FRAME1_COLOURS         "frame1-colours"
#define ZMAPSTYLE_PROPERTY_FRAME2_COLOURS         "frame2-colours"
#define ZMAPSTYLE_PROPERTY_REV_COLOURS            "rev-colours"
/* ... zoom sensitive display */
#define ZMAPSTYLE_PROPERTY_DISPLAY_MODE           "display-mode"
#define ZMAPSTYLE_PROPERTY_MIN_MAG                "min-mag"
#define ZMAPSTYLE_PROPERTY_MAX_MAG                "max-mag"
/* ... bumping */
#define ZMAPSTYLE_PROPERTY_BUMP_MODE              "bump-mode"
#define ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE      "default-bump-mode"
#define ZMAPSTYLE_PROPERTY_BUMP_FIXED             "bump-fixed"
#define ZMAPSTYLE_PROPERTY_BUMP_SPACING           "bump-spacing"
/* ... score by width */
#define ZMAPSTYLE_PROPERTY_WIDTH                  "width"
#define ZMAPSTYLE_PROPERTY_SCORE_MODE             "score-mode"
#define ZMAPSTYLE_PROPERTY_MIN_SCORE              "min-score"
#define ZMAPSTYLE_PROPERTY_MAX_SCORE              "max-score"
/* ... meta */
#define ZMAPSTYLE_PROPERTY_GFF_SOURCE             "gff-source"
#define ZMAPSTYLE_PROPERTY_GFF_FEATURE            "gff-feature"
#define ZMAPSTYLE_PROPERTY_DISPLAYABLE            "displayable"
#define ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY        "show-when-empty"
#define ZMAPSTYLE_PROPERTY_SHOW_TEXT              "show-text"
#define ZMAPSTYLE_PROPERTY_SUB_FEATURES           "sub-features"        // styles to use for
/* ... stranding */
#define ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC        "strand-specific"
#define ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND    "show-reverse-strand"
#define ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR "show-only-in-separator"
#define ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS       "directional-ends"
/* ... frame sensitivity */
#define ZMAPSTYLE_PROPERTY_FRAME_MODE             "frame-mode"
/* ... deferred loading */
#define ZMAPSTYLE_PROPERTY_DEFERRED               "deferred"
#define ZMAPSTYLE_PROPERTY_LOADED                 "loaded"


/* glyph properties - can be for mode glyph or as sub-features */
#define ZMAPSTYLE_PROPERTY_GLYPH_NAME             "glyph"
#define ZMAPSTYLE_PROPERTY_GLYPH_SHAPE            "glyph-shape"
#define ZMAPSTYLE_PROPERTY_GLYPH_NAME_5           "glyph-5"
#define ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5          "glyph-shape-5"
#define ZMAPSTYLE_PROPERTY_GLYPH_NAME_3           "glyph-3"
#define ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3          "glyph-shape-3"

#define ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS      "glyph-alt-colours"
#define ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD        "glyph-threshold"
#define ZMAPSTYLE_PROPERTY_GLYPH_STRAND           "glyph-strand"
#define ZMAPSTYLE_PROPERTY_GLYPH_ALIGN            "glyph-align"


/* graph properties. */
#define ZMAPSTYLE_PROPERTY_GRAPH_MODE      "graph-mode"
#define ZMAPSTYLE_PROPERTY_GRAPH_BASELINE  "graph-baseline"


/* alignment properties */
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS          "alignment-parse-gaps"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS           "alignment-show-gaps"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN          "alignment-join-align"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN      "alignment-allow-misalign"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE          "alignment-pfetchable"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM              "alignment-blixem"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS     "alignment-perfect-colours"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS    "alignment-colinear-colours"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS "alignment-noncolinear-colours"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR    "alignment-unmarked-colinear"

/* transcript properties */
#define ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS "transcript-cds-colours"

/* Text properties */
#define ZMAPSTYLE_PROPERTY_TEXT_FONT "text-font"

/* Assembly path properties */
#define ZMAPSTYLE_PROPERTY_ASSEMBLY_PATH_NON_COLOURS "non-assembly-colours"

/*
 * The following are a series of enums that define various properties of a style.
 * NOTE that it is imperative that each enum type has as it's first member
 * a ZMAP_xxxx_INVALID element whose value is zero to allow simple testing of the
 * form
 *          if (element)
 *            do_something ;
 *  */

#define ZMAP_STYLE_MODE_LIST(_)                                                                          \
_(ZMAPSTYLE_MODE_INVALID,       , "invalid"      , "invalid mode "                                 , "") \
_(ZMAPSTYLE_MODE_BASIC,         , "basic"        , "Basic box features "                           , "") \
_(ZMAPSTYLE_MODE_ALIGNMENT,     , "alignment"    , "Usual homology structure "                     , "") \
_(ZMAPSTYLE_MODE_TRANSCRIPT,    , "transcript"   , "Usual transcript like structure "              , "") \
_(ZMAPSTYLE_MODE_RAW_SEQUENCE,  , "raw-sequence" , "DNA Sequence "                                 , "") \
_(ZMAPSTYLE_MODE_PEP_SEQUENCE,  , "pep-sequence" , "Peptide Sequence "                             , "") \
_(ZMAPSTYLE_MODE_ASSEMBLY_PATH, , "assembly-path", "Assembly path "                                , "") \
_(ZMAPSTYLE_MODE_TEXT,          , "text"         , "Text only display "                            , "") \
_(ZMAPSTYLE_MODE_GRAPH,         , "graph"        , "Graphs of various types "                      , "") \
_(ZMAPSTYLE_MODE_GLYPH,         , "glyph"        , "Special graphics for particular feature types ", "") \
_(ZMAPSTYLE_MODE_META,          , "meta"         , "Meta object controlling display of features "  , "")

ZMAP_DEFINE_ENUM(ZMapStyleMode, ZMAP_STYLE_MODE_LIST);


#define ZMAP_STYLE_COLUMN_DISPLAY_LIST(_)                                                      \
_(ZMAPSTYLE_COLDISPLAY_INVALID,   , "invalid"  , "invalid mode  "                        , "") \
_(ZMAPSTYLE_COLDISPLAY_HIDE,      , "hide"     , "Never show. "                          , "") \
_(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE, , "show-hide", "Show according to zoom/mag, mark etc. ", "") \
_(ZMAPSTYLE_COLDISPLAY_SHOW,      , "show"     , "Always show. "                         , "")

ZMAP_DEFINE_ENUM(ZMapStyleColumnDisplayState, ZMAP_STYLE_COLUMN_DISPLAY_LIST);


#define ZMAP_STYLE_BLIXEM_LIST(_)                                             \
_(ZMAPSTYLE_BLIXEM_INVALID, , "invalid" , "invalid  "                   , "") \
_(ZMAPSTYLE_BLIXEM_N,       , "blixem-n", "Blixem nucleotide sequence. ", "") \
_(ZMAPSTYLE_BLIXEM_X,       , "blixem-x", "Blixem peptide sequence. "   , "")

ZMAP_DEFINE_ENUM(ZMapStyleBlixemType, ZMAP_STYLE_BLIXEM_LIST) ;


/* Specifies how features in columns should be bumpped for compact display. */
#define ZMAP_STYLE_BUMP_MODE_LIST(_)                                              \
_(ZMAPBUMP_INVALID,               , "invalid",               "invalid",                       "invalid")					\
_(ZMAPBUMP_UNBUMP,                , "unbump",                "Unbump",                        "No bumping (default)") \
_(ZMAPBUMP_OVERLAP,               , "overlap",               "Overlap",                       "Bump any features overlapping each other.") \
_(ZMAPBUMP_NAVIGATOR,             , "navigator",             "Navigator Overlap",             "Navigator bump: special for zmap navigator bumping.") \
_(ZMAPBUMP_START_POSITION,        , "start-position",        "Start Position",                "Bump if features have same start coord.") \
_(ZMAPBUMP_ALTERNATING,           , "alternating",           "Alternating",                   "Alternate features between two sub_columns, e.g. to display assemblies.") \
_(ZMAPBUMP_ALL,                   , "all",                   "Bump All",                      "A sub-column for every feature.") \
_(ZMAPBUMP_NAME,                  , "name",                  "Name",                          "A sub-column for features with the same name.") \
_(ZMAPBUMP_NAME_INTERLEAVE,       , "name-interleave",       "Name Interleave",               "All features with same name in a single sub-column but several names interleaved in each sub-column, the most compact display.") \
_(ZMAPBUMP_NAME_NO_INTERLEAVE,    , "name-no-interleave",    "Name No Interleave",            "Display as for Interleave but no interleaving of different names.") \
_(ZMAPBUMP_NAME_COLINEAR,         , "name-colinear",         "Name & Colinear", "As for Name but colinear alignments shown.") \
_(ZMAPBUMP_NAME_BEST_ENDS,        , "name-best-ends",        "Name and Best 5'& 3' Matches",  "As for No Interleave but for alignments sorted by 5' and 3' best/biggest matches, one sub_column per match.")

//_(ZMAPBUMP_NAME_INTERLEAVE_COLINEAR,         , "name-colinear-interleave",         "Name Interleave & Colinear", "As for Name & Colinear but interleaved, the most compact display.")


/* We should do this automatically or not at all..... */
#define ZMAPBUMP_START ZMAPBUMP_UNBUMP
#define ZMAPBUMP_END ZMAPBUMP_NAME_BEST_ENDS

ZMAP_DEFINE_ENUM(ZMapStyleBumpMode, ZMAP_STYLE_BUMP_MODE_LIST) ;



#define ZMAP_STYLE_3_FRAME_LIST(_)                                                                \
_(ZMAPSTYLE_3_FRAME_INVALID, , "invalid", "invalid mode  "                                  , "") \
_(ZMAPSTYLE_3_FRAME_NEVER,   , "never"  , "Not frame sensitive.  "                          , "") \
_(ZMAPSTYLE_3_FRAME_AS_WELL, , "always" , "Display normally and as 3 cols in 3 frame mode. ", "") \
_(ZMAPSTYLE_3_FRAME_ONLY_3,  , "only-3" , "Only dislay in 3 frame mode as 3 cols. "         , "") \
_(ZMAPSTYLE_3_FRAME_ONLY_1,  , "only-1" , "Only display in 3 frame mode as 1 col. "         , "")

ZMAP_DEFINE_ENUM(ZMapStyle3FrameMode, ZMAP_STYLE_3_FRAME_LIST);


/* Specifies the style of graph. */
#define ZMAP_STYLE_GRAPH_MODE_LIST(_)                                           \
_(ZMAPSTYLE_GRAPH_INVALID,   , "invalid"  , "Initial setting. "           , "") \
_(ZMAPSTYLE_GRAPH_LINE,      , "line"     , "Just points joining a line. ", "") \
_(ZMAPSTYLE_GRAPH_HISTOGRAM, , "histogram", "Usual blocky like graph."    , "")

ZMAP_DEFINE_ENUM(ZMapStyleGraphMode, ZMAP_STYLE_GRAPH_MODE_LIST);



// these sub features are used where hard coded _if_ the styles are provided
// some may have hard coded defaults
// NOTE: MAX is used as an end marker
#define ZMAP_STYLE_SUB_FEATURE_LIST(_)                            \
_(ZMAPSTYLE_SUB_FEATURE_INVALID, , "invalid", "Not used. ", "") \
_(ZMAPSTYLE_SUB_FEATURE_HOMOLOGY,  , "homology" , "Incomplete-homology-marker" , "") \
_(ZMAPSTYLE_SUB_FEATURE_NON_CONCENCUS_SPLICE,  , "non-concensus-splice" , "Non concensus splice marker" , "") \
_(ZMAPSTYLE_SUB_FEATURE_TRUNCATED,  , "truncated" , "Truncated transcript" , "")\
_(ZMAPSTYLE_SUB_FEATURE_POLYA,  , "polyA" , "Poly A tail on RNA seq"   , "") \
_(ZMAPSTYLE_SUB_FEATURE_MAX , ,"do-not-use" ,"" , "")

ZMAP_DEFINE_ENUM(ZMapStyleSubFeature, ZMAP_STYLE_SUB_FEATURE_LIST);

#define ZMAPSTYLE_LEGACY_3FRAME     "gf_splice"       // to match what was set up in acedb

/*
 * specifies strand specific processing
 */
#define ZMAP_STYLE_GLYPH_STRAND_LIST(_)                            \
_(ZMAPSTYLE_GLYPH_STRAND_INVALID, , "invalid", "Initial setting. ", "") \
_(ZMAPSTYLE_GLYPH_STRAND_FLIP_X,  , "flip-x" , ""                 , "") \
_(ZMAPSTYLE_GLYPH_STRAND_FLIP_Y,  , "flip-y" , "" , "")

ZMAP_DEFINE_ENUM(ZMapStyleGlyphStrand, ZMAP_STYLE_GLYPH_STRAND_LIST);

/*
 * specifies glyph alignment: NB glyph shape should be chosen appropriately!
 */
#define ZMAP_STYLE_GLYPH_ALIGN_LIST(_)                            \
_(ZMAPSTYLE_GLYPH_ALIGN_INVALID, , "invalid", "Initial setting. ", "") \
_(ZMAPSTYLE_GLYPH_ALIGN_LEFT,  , "left" , ""                 , "") \
_(ZMAPSTYLE_GLYPH_ALIGN_CENTRE,, "centre" , ""                 , "") \
_(ZMAPSTYLE_GLYPH_ALIGN_RIGHT, , "right" , "" , "")

ZMAP_DEFINE_ENUM(ZMapStyleGlyphAlign, ZMAP_STYLE_GLYPH_ALIGN_LIST);


/* Specifies type of colour, e.g. normal or selected. */
#define ZMAP_STYLE_COLOUR_TYPE_LIST(_)                  \
_(ZMAPSTYLE_COLOURTYPE_INVALID,  , "invalid" , " ", "") \
_(ZMAPSTYLE_COLOURTYPE_NORMAL,   , "normal"  , " ", "") \
_(ZMAPSTYLE_COLOURTYPE_SELECTED, , "selected", " ", "")

ZMAP_DEFINE_ENUM(ZMapStyleColourType, ZMAP_STYLE_COLOUR_TYPE_LIST) ;


/* For drawing/colouring the various parts of a feature. */
#define ZMAP_STYLE_DRAW_CONTEXT_LIST(_)                                 \
_(ZMAPSTYLE_DRAW_INVALID, , "invalid", "invalid, initial setting ", "") \
_(ZMAPSTYLE_DRAW_FILL,    , "fill"   , " "                        , "") \
_(ZMAPSTYLE_DRAW_DRAW,    , "draw"   , " "                        , "") \
_(ZMAPSTYLE_DRAW_BORDER,  , "border" , ""                         , "")

ZMAP_DEFINE_ENUM(ZMapStyleDrawContext, ZMAP_STYLE_DRAW_CONTEXT_LIST) ;


/* Specifies how wide features should be in relation to their score. */
#define ZMAP_STYLE_SCORE_MODE_LIST(_)                                          \
_(ZMAPSCORE_INVALID,   , "invalid"  , "Use column width only - default. ", "") \
_(ZMAPSCORE_WIDTH,     , "width"    , "Use column width only - default. ", "") \
_(ZMAPSCORE_HEIGHT,    , "height"   , "scale height of glyph. ", "") \
_(ZMAPSCORE_SIZE,      , "size"     , "scale size of glyph. ", "") \
_(ZMAPSCORE_OFFSET,    , "offset"   , ""                                 , "") \
_(ZMAPSCORE_HISTOGRAM, , "histogram", ""                                 , "") \
_(ZMAPSCORE_PERCENT,   , "percent"  , ""                                 , "")\
_(ZMAPSTYLE_SCORE_ALT, , "alt" , "alternate colour for glyph" , "")


ZMAP_DEFINE_ENUM(ZMapStyleScoreMode, ZMAP_STYLE_SCORE_MODE_LIST) ;


#define ZMAP_STYLE_MERGE_MODE_LIST(_)                                                                \
_(ZMAPSTYLE_MERGE_INVALID,  , "invalid" , "invalid mode  "                                     , "") \
_(ZMAPSTYLE_MERGE_PRESERVE, , "preserve", "If a style already exists, do nothing. "            , "") \
_(ZMAPSTYLE_MERGE_REPLACE,  , "replace" , "Replace existing styles with the new one. "         , "") \
_(ZMAPSTYLE_MERGE_MERGE,    , "merge"   , "Merge existing styles with new ones by overriding. ", "")

ZMAP_DEFINE_ENUM(ZMapStyleMergeMode, ZMAP_STYLE_MERGE_MODE_LIST) ;





/* Note the naming here in the macros. ZMAP_TYPE_FEATURE_TYPE_STYLE seemed confusing... */

#define ZMAP_TYPE_FEATURE_STYLE           (zMapFeatureTypeStyleGetType())
#define ZMAP_FEATURE_STYLE(obj)	          (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyle))
#define ZMAP_FEATURE_STYLE_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyle const))
#define ZMAP_FEATURE_STYLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyleClass))
#define ZMAP_IS_FEATURE_STYLE(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_FEATURE_STYLE))
#define ZMAP_FEATURE_STYLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_FEATURE_STYLE, zmapFeatureTypeStyleClass))


/* Instance */
typedef struct _zmapFeatureTypeStyleStruct  zmapFeatureTypeStyle, *ZMapFeatureTypeStyle ;


/* Class */
typedef struct _zmapFeatureTypeStyleClassStruct  zmapFeatureTypeStyleClass, *ZMapFeatureTypeStyleClass ;





/* Public funcs */
GType zMapFeatureTypeStyleGetType(void);



ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2Mode,            ZMapStyleMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ColDisplayState, ZMapStyleColumnDisplayState) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr23FrameMode,      ZMapStyle3FrameMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2GraphMode,       ZMapStyleGraphMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2DrawContext,     ZMapStyleDrawContext) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ColourType,      ZMapStyleColourType) ;
//ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ColourTarget,    ZMapStyleColourTarget) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ScoreMode,       ZMapStyleScoreMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2BumpMode,     ZMapStyleBumpMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2GlyphStrand,     ZMapStyleGlyphStrand) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2SubFeature,     ZMapStyleSubFeature) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2GlyphAlign,     ZMapStyleGlyphAlign) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2BlixemType,     ZMapStyleBlixemType) ;

/* Enum -> String function decs: const char *zMapStyleXXXXMode2ExactStr(ZMapStyleXXXXXMode mode);  */
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapStyleMode2ExactStr,            ZMapStyleMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColDisplayState2ExactStr, ZMapStyleColumnDisplayState) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyle3FrameMode2ExactStr, ZMapStyle3FrameMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGraphMode2ExactStr,       ZMapStyleGraphMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleDrawContext2ExactStr,     ZMapStyleDrawContext) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColourType2ExactStr,      ZMapStyleColourType) ;
//ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColourTarget2ExactStr,    ZMapStyleColourTarget) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleScoreMode2ExactStr,       ZMapStyleScoreMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleBumpMode2ExactStr,     ZMapStyleBumpMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGlyphStrand2ExactStr,     ZMapStyleGlyphStrand) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleSubFeature2ExactStr,     ZMapStyleSubFeature) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGlyphAlign2ExactStr,     ZMapStyleGlyphAlign) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleBlixemType2ExactStr,     ZMapStyleBlixemType) ;


ZMAP_ENUM_TO_SHORT_TEXT_DEC(zmapStyleBumpMode2ShortText, ZMapStyleBumpMode) ;



ZMapFeatureTypeStyle zMapStyleCreate(char *name, char *description) ;
ZMapFeatureTypeStyle zMapStyleCreateV(guint n_parameters, GParameter *parameters) ;
ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src);
gboolean zMapStyleCCopy(ZMapFeatureTypeStyle src, ZMapFeatureTypeStyle *dest_out);
void zMapStyleDestroy(ZMapFeatureTypeStyle style);

gboolean zMapStyleIsPropertySet(ZMapFeatureTypeStyle style, char *property_name) ;
gboolean zMapStyleIsPropertySetId(ZMapFeatureTypeStyle style, ZMapStyleParamId id);
void zmapStyleSetIsSet(ZMapFeatureTypeStyle style, ZMapStyleParamId id);
void zmapStyleUnsetIsSet(ZMapFeatureTypeStyle style, ZMapStyleParamId id);

gboolean zMapStyleGet(ZMapFeatureTypeStyle style, char *first_property_name, ...) ;
gboolean zMapStyleSet(ZMapFeatureTypeStyle style, char *first_property_name, ...) ;


gboolean zMapStyleNameCompare(ZMapFeatureTypeStyle style, char *name) ;
gboolean zMapStyleIsTrueFeature(ZMapFeatureTypeStyle style) ;

ZMapStyleGlyphShape zMapStyleGetGlyphShape(gchar *shape);
ZMapFeatureTypeStyle zMapStyleLegacyStyle(char *name);

unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetSubFeature(ZMapFeatureTypeStyle style,ZMapStyleSubFeature i);

gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
			     char *fill, char *draw, char *border) ;
void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show) ;
void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode) ;
ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style) ;
const gchar *zMapStyleGetName(ZMapFeatureTypeStyle style) ;
ZMapStyleScoreMode zMapStyleGetScoreMode(ZMapFeatureTypeStyle style);
ZMapStyleBumpMode zMapStyleGetBumpMode(ZMapFeatureTypeStyle style) ;
void zMapStyleSetBumpMode(ZMapFeatureTypeStyle style, ZMapStyleBumpMode bump_mode) ;
const gchar *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style) ;
const gchar *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style) ;
void zMapStyleSetDescription(ZMapFeatureTypeStyle style, char *description) ;
ZMapStyleBumpMode zMapStyleGetDefaultBumpMode(ZMapFeatureTypeStyle style);
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;
double zMapStyleGetBumpSpace(ZMapFeatureTypeStyle style) ;
ZMapStyleColumnDisplayState zMapStyleGetDisplay(ZMapFeatureTypeStyle style) ;

ZMapStyleGlyphShape zMapStyleGlyphShape(ZMapFeatureTypeStyle style);
ZMapStyleGlyphShape zMapStyleGlyphShape5(ZMapFeatureTypeStyle style);
ZMapStyleGlyphShape zMapStyleGlyphShape3(ZMapFeatureTypeStyle style);

void zMapStyleSetShowGaps(ZMapFeatureTypeStyle style, gboolean show_gaps) ;

void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, unsigned int between_align_error) ;
GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMinMag(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMaxMag(ZMapFeatureTypeStyle style) ;
void zMapStyleGetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean *strand_specific, gboolean *show_rev_strand, ZMapStyle3FrameMode *frame_mode) ;
double zMapStyleGetMaxScore(ZMapFeatureTypeStyle style) ;
double zMapStyleGetMinScore(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleGetShowWhenEmpty(ZMapFeatureTypeStyle style);
gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
			     GdkColor **fill, GdkColor **draw, GdkColor **border) ;
gboolean zMapStyleGetColoursDefault(ZMapFeatureTypeStyle style,
                            GdkColor **background, GdkColor **foreground, GdkColor **outline);
char *zMapStyleGetDescription(ZMapFeatureTypeStyle style) ;
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;

int zMapStyleGlyphThreshold(ZMapFeatureTypeStyle style);

ZMapStyleGlyphStrand zMapStyleGlyphStrand(ZMapFeatureTypeStyle style);
ZMapStyleGlyphAlign zMapStyleGetAlign(ZMapFeatureTypeStyle style);

void zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, gboolean *parse_gaps, gboolean *show_gaps) ;

double zMapStyleGetBumpWidth(ZMapFeatureTypeStyle style) ;
void zMapStyleSetParent(ZMapFeatureTypeStyle style, char *parent_name) ;
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag) ;
void zMapStyleSetGraph(ZMapFeatureTypeStyle style, ZMapStyleGraphMode mode, double min, double max, double baseline) ;
void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score) ;
void zMapStyleSetStrandSpecific(ZMapFeatureTypeStyle type, gboolean strand_specific) ;
void zMapStyleSetStrandShowReverse(ZMapFeatureTypeStyle type, gboolean show_reverse) ;
void zMapStyleSetFrameMode(ZMapFeatureTypeStyle type, ZMapStyle3FrameMode frame_mode) ;
void zMapStyleSetGFF(ZMapFeatureTypeStyle style, char *gff_source, char *gff_feature) ;
void zMapStyleSetDisplayable(ZMapFeatureTypeStyle style, gboolean displayable) ;
void zMapStyleSetDeferred(ZMapFeatureTypeStyle style, gboolean deferred) ;
void zMapStyleSetLoaded(ZMapFeatureTypeStyle style, gboolean loaded) ;
void zMapStyleSetEndStyle(ZMapFeatureTypeStyle style, gboolean directional) ;

void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean parse_gaps, gboolean show_gaps) ;

gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error) ;
void zMapStyleSetBumpSpace(ZMapFeatureTypeStyle style, double bump_spacing) ;
void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty) ;
void zMapStyleSetPfetch(ZMapFeatureTypeStyle style, gboolean pfetchable) ;
void zMapStyleSetWidth(ZMapFeatureTypeStyle style, double width) ;


gboolean zMapStyleIsDrawable(ZMapFeatureTypeStyle style, GError **error) ;
gboolean zMapStyleMakeDrawable(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleGetColoursCDSDefault(ZMapFeatureTypeStyle style,
				       GdkColor **background, GdkColor **foreground, GdkColor **outline);
gboolean zMapStyleGetColoursGlyphDefault(ZMapFeatureTypeStyle style,
                               GdkColor **background, GdkColor **foreground, GdkColor **outline);
gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context) ;
gboolean zMapStyleIsBackgroundColour(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsForegroundColour(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsOutlineColour(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleColourByStrand(ZMapFeatureTypeStyle style);


ZMapStyleGlyphStrand zMapStyleGlyphStrand(ZMapFeatureTypeStyle style);

gboolean zMapStyleIsDirectionalEnd(ZMapFeatureTypeStyle style) ;


gboolean zMapStyleIsDisplayable(ZMapFeatureTypeStyle style) ;


gboolean zMapStyleIsDeferred(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsLoaded(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsHidden(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsStrandSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsShowReverseStrand(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsFrameOneColumn(ZMapFeatureTypeStyle style) ;

double zMapStyleBaseline(ZMapFeatureTypeStyle style) ;

gboolean zMapStyleIsMinMag(ZMapFeatureTypeStyle style, double *min_mag) ;
gboolean zMapStyleIsMaxMag(ZMapFeatureTypeStyle style, double *max_mag) ;

/* Lets change all these names to just be zmapStyle, i.e. lose the featuretype bit..... */

ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description) ;

gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style);

gboolean zMapStyleIsParseGaps(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleIsShowGaps(ZMapFeatureTypeStyle style) ;


char *zMapStyleCreateName(char *style_name) ;
GQuark zMapStyleCreateID(char *style_name) ;


ZMapFeatureTypeStyle zMapStyleGetPredefined(char *style_name) ;
gboolean zMapFeatureTypeSetAugment(GData **current, GData **new) ;

void zMapStyleInitBumpMode(ZMapFeatureTypeStyle style,
			      ZMapStyleBumpMode default_bump_mode, ZMapStyleBumpMode curr_bump_mode) ;
ZMapStyleBumpMode zMapStyleResetBumpMode(ZMapFeatureTypeStyle style) ;


ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle style) ;
gboolean zMapStyleMerge(ZMapFeatureTypeStyle curr_style, ZMapFeatureTypeStyle new_style) ;





gboolean zMapStyleDisplayInSeparator(ZMapFeatureTypeStyle style);

/* Style set functions... */

gboolean zMapStyleSetAdd(GData **style_set, ZMapFeatureTypeStyle style) ;
gboolean zMapStyleCopyAllStyles(GData **style_set, GData **copy_style_set_out) ;
gboolean zMapStyleInheritAllStyles(GData **style_set) ;
gboolean zMapStyleNameExists(GList *style_name_list, char *style_name) ;
ZMapFeatureTypeStyle zMapFindStyle(GData *styles, GQuark style_id) ;
GList *zMapStylesGetNames(GData *styles) ;
GData *zMapStyleGetAllPredefined(void) ;
GData *zMapStyleMergeStyles(GData *curr_styles, GData *new_styles, ZMapStyleMergeMode merge_mode) ;
void zMapStyleDestroyStyles(GData **styles) ;



/* Debug functions. */


void zMapStylePrint(ZMapIOOut dest, ZMapFeatureTypeStyle style, char *prefix, gboolean full) ;
void zMapStyleSetPrintAll(ZMapIOOut dest, GData *type_set, char *user_string, gboolean full) ;
void zMapStyleSetPrintAllStdOut(GData *type_set, char *user_string, gboolean full) ;

void zMapStyleListPrintAll(ZMapIOOut dest, GList *styles, char *user_string, gboolean full) ;




#endif /* ZMAP_STYLE_H */
