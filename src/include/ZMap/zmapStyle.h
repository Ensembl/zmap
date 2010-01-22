/*  File: zmapStyle.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Jan 22 08:59 2010 (edgrif)
 * Created: Mon Feb 26 09:28:26 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.h,v 1.46 2010-01-22 13:00:42 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_STYLE_H
#define ZMAP_STYLE_H


#include <glib-object.h>
#include <gtk/gtk.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapIO.h>


/* All the properties that can be set on a style object. NOTE that I would have liked to have
 * done this as a list (as for other strings/enums below) from which to automatically generate
 * #defines of the form #define ZMAPSTYLE_PROPERTY_NAME "name", BUT it's not possible to have
 * a macro that contains the '#' character so I can't have #define in the output of the macro
 * so I can't do it...sigh.... */

/* General Style properties... */
#define ZMAPSTYLE_PROPERTY_INVALID                "invalid"
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
#define ZMAPSTYLE_PROPERTY_FRAME_MODE             "frame-mode"
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
/* ... stranding */
#define ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC        "strand-specific"
#define ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND    "show-reverse-strand"
#define ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR "show-only-in-separator"
#define ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS       "directional-ends"
/* ... deferred loading */
#define ZMAPSTYLE_PROPERTY_DEFERRED               "deferred"          
#define ZMAPSTYLE_PROPERTY_LOADED                 "loaded"

/* graph properties. */
#define ZMAPSTYLE_PROPERTY_GRAPH_MODE      "graph-mode"
#define ZMAPSTYLE_PROPERTY_GRAPH_BASELINE  "graph-baseline"

/* glyph properties. */
#define ZMAPSTYLE_PROPERTY_GLYPH_MODE      "glyph-mode"
#define ZMAPSTYLE_PROPERTY_GLYPH_TYPE      "glyph-type"

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
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH    "alignment-incomplete-glyph"
#define ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS    "alignment-incomplete-glyph-colours"
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
_(ZMAPBUMP_NAME_COLINEAR,         , "name-colinear",         "Name No Interleave & Colinear", "As for No Interleave but for alignments only colinear shown.") \
_(ZMAPBUMP_NAME_BEST_ENDS,        , "name-best-ends",        "Name and Best 5'& 3' Matches",  "As for No Interleave but for alignments sorted by 5' and 3' best/biggest matches, one sub_column per match.")


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


/* Specifies the sub-mode of glyph.
 * SPLICE is partly hard coded due the nature of the data
 * MARKER can be attached to any data via style mode=glyph and glyph type defined as in GLYPH_TYPE below
 */

#define ZMAP_STYLE_GLYPH_MODE_LIST(_)                            \
_(ZMAPSTYLE_GLYPH_INVALID, , "invalid", "Initial setting. ", "") \
_(ZMAPSTYLE_GLYPH_SPLICE,  , "splice" , ""                 , "") \
_(ZMAPSTYLE_GLYPH_MARKER,  , "marker" , "Zoom free marker" , "")

ZMAP_DEFINE_ENUM(ZMapStyleGlyphMode, ZMAP_STYLE_GLYPH_MODE_LIST);


/* Specifies the style of glyph for an incomplete alignment marker. (not a free standing glyph) */
// refer to enum in zmapWindowGlyphItem.h
#define ZMAP_STYLE_GLYPH_TYPE_LIST(_)                            \
_(ZMAPSTYLE_GLYPH_TYPE_INVALID, ,                "invalid", "Initial setting. ", "") \
_(ZMAPSTYLE_GLYPH_TYPE_SLASH_FORWARDS,         , "slash-forwards" , "slash forwards glyph"        , "")\
_(ZMAPSTYLE_GLYPH_TYPE_SLASH_REVERSE,          , "slash-reverse" , "slash reverse glyph"          , "")\
_(ZMAPSTYLE_GLYPH_TYPE_WALKING_STICK_FORWARD,  , "walking-stick-forwards" , "walking-stick-forwards glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_WALKING_STICK_REVERSE,  , "walking-stick-reverse" , "walking-stick-reverse glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_TRIANGLE_FORWARD,       , "triangle-forward" , "triangle-forward glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_TRIANGLE_REVERSE,       , "triangle-reverse" , "triangle-reverse glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_TRIANGLE,               , "triangle" , "triangle glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_DIAMOND,                , "diamond" , "diamond glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_ASTERISK,               , "asterisk" , "asterisk glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_CROSS,                  , "cross" , "cross glyph"                 , "")\
_(ZMAPSTYLE_GLYPH_TYPE_CIRCLE,                 , "circle" , "circle glyph"                 , "")

ZMAP_DEFINE_ENUM(ZMapStyleGlyphType, ZMAP_STYLE_GLYPH_TYPE_LIST);


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


/* Specifies the target type of the colour. */
#define ZMAP_STYLE_COLOUR_TARGET_LIST(_)                                                             \
_(ZMAPSTYLE_COLOURTARGET_INVALID,           , "invalid"          , "Normal colour "            , "") \
_(ZMAPSTYLE_COLOURTARGET_NORMAL,            , "normal"           , "Normal colour "            , "") \
_(ZMAPSTYLE_COLOURTARGET_FRAME0,            , "frame0"           , "Frame 1 colour "           , "") \
_(ZMAPSTYLE_COLOURTARGET_FRAME1,            , "frame1"           , "Frame 2 colour "           , "") \
_(ZMAPSTYLE_COLOURTARGET_FRAME2,            , "frame2"           , "Frame 3 colour "           , "") \
_(ZMAPSTYLE_COLOURTARGET_CDS,               , "cds"              , "Colour to apply to CDS "   , "") \
_(ZMAPSTYLE_COLOURTARGET_NON_ASSEMBLY_PATH, , "non-assembly"     , "Colour to non-path sections of assembly ", "")  \
_(ZMAPSTYLE_COLOURTARGET_STRAND,            , "strand"           , "Colour to apply to Strand ", "")

ZMAP_DEFINE_ENUM(ZMapStyleColourTarget, ZMAP_STYLE_COLOUR_TARGET_LIST) ;


/* Specifies how wide features should be in relation to their score. */
#define ZMAP_STYLE_SCORE_MODE_LIST(_)                                          \
_(ZMAPSCORE_INVALID,   , "invalid"  , "Use column width only - default. ", "") \
_(ZMAPSCORE_WIDTH,     , "width"    , "Use column width only - default. ", "") \
_(ZMAPSCORE_OFFSET,    , "offset"   , ""                                 , "") \
_(ZMAPSCORE_HISTOGRAM, , "histogram", ""                                 , "") \
_(ZMAPSCORE_PERCENT,   , "percent"  , ""                                 , "")

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
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2GlyphMode,       ZMapStyleGlyphMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2DrawContext,     ZMapStyleDrawContext) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ColourType,      ZMapStyleColourType) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ColourTarget,    ZMapStyleColourTarget) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2ScoreMode,       ZMapStyleScoreMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2BumpMode,     ZMapStyleBumpMode) ;
ZMAP_ENUM_FROM_STRING_DEC(zMapStyleStr2GlyphType,       ZMapStyleGlyphType) ;


/* Enum -> String function decs: const char *zMapStyleXXXXMode2ExactStr(ZMapStyleXXXXXMode mode);  */
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapStyleMode2ExactStr,            ZMapStyleMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColDisplayState2ExactStr, ZMapStyleColumnDisplayState) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyle3FrameMode2ExactStr, ZMapStyle3FrameMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGraphMode2ExactStr,       ZMapStyleGraphMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGlyphMode2ExactStr,       ZMapStyleGlyphMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleDrawContext2ExactStr,     ZMapStyleDrawContext) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColourType2ExactStr,      ZMapStyleColourType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleColourTarget2ExactStr,    ZMapStyleColourTarget) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleScoreMode2ExactStr,       ZMapStyleScoreMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleBumpMode2ExactStr,     ZMapStyleBumpMode) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zmapStyleGlyphType2ExactStr,       ZMapStyleGlyphType) ;


ZMAP_ENUM_TO_SHORT_TEXT_DEC(zmapStyleBumpMode2ShortText, ZMapStyleBumpMode) ;



ZMapFeatureTypeStyle zMapStyleCreate(char *name, char *description) ;
ZMapFeatureTypeStyle zMapStyleCreateV(guint n_parameters, GParameter *parameters) ;
ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src);
gboolean zMapStyleCCopy(ZMapFeatureTypeStyle src, ZMapFeatureTypeStyle *dest_out);
void zMapStyleDestroy(ZMapFeatureTypeStyle style);

gboolean zMapStyleIsPropertySet(ZMapFeatureTypeStyle style, char *property_name, char *property_subpart) ;

gboolean zMapStyleGet(ZMapFeatureTypeStyle style, char *first_property_name, ...) ;
gboolean zMapStyleSet(ZMapFeatureTypeStyle style, char *first_property_name, ...) ;


gboolean zMapStyleNameCompare(ZMapFeatureTypeStyle style, char *name) ;
gboolean zMapStyleIsTrueFeature(ZMapFeatureTypeStyle style) ;



unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style) ;
GQuark zMapStyleGetID(ZMapFeatureTypeStyle style) ;
void zMapStyleSetGlyphMode(ZMapFeatureTypeStyle style, ZMapStyleGlyphMode glyph_mode) ;
gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     char *fill, char *draw, char *border) ;
void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show) ;
void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode) ;
ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetName(ZMapFeatureTypeStyle style) ;
ZMapStyleBumpMode zMapStyleGetBumpMode(ZMapFeatureTypeStyle style) ;
void zMapStyleSetBumpMode(ZMapFeatureTypeStyle style, ZMapStyleBumpMode bump_mode) ;
char *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style) ;
char *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style) ;
void zMapStyleSetDescription(ZMapFeatureTypeStyle style, char *description) ;
ZMapStyleBumpMode zMapStyleGetDefaultBumpMode(ZMapFeatureTypeStyle style);
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;
double zMapStyleGetBumpSpace(ZMapFeatureTypeStyle style) ;
ZMapStyleColumnDisplayState zMapStyleGetDisplay(ZMapFeatureTypeStyle style) ;


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
gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     GdkColor **fill, GdkColor **draw, GdkColor **border) ;
gboolean zMapStyleGetColoursDefault(ZMapFeatureTypeStyle style, 
                            GdkColor **background, GdkColor **foreground, GdkColor **outline);
char *zMapStyleGetDescription(ZMapFeatureTypeStyle style) ;
double zMapStyleGetWidth(ZMapFeatureTypeStyle style) ;

ZMapStyleGlyphType zMapStyleGlyphType(ZMapFeatureTypeStyle style);
ZMapStyleGlyphType zMapStyleGlyphMode(ZMapFeatureTypeStyle style);


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

void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type) ;
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
