/*  File: zmapStyle.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk), Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>   // for zMapConfigLegacyStyles() only can remove when system has moved on
#include <zmapStyle_P.hpp>



/*
 * Some notes on implementation by mh17
 *
 * There is a write up in zmapFeature.shtml
 * (or wherever your source tree is)
 *
 * Styles are GObjects and implement the g_object_get/set interface
 * but internally (esp. in this file) the code uses the ZMapFeatureTypeStyleStruct directly
 *
 * We use a mechanical process of accesing data, using an array of ZMapStyleParamStruct
 * to define offsets to struct members, and an array of is_set flags is maintained auotmatically
 */


#define PARAM_ID_IS_VALID(PARAM_ID)             \
  ((PARAM_ID) > 0 && (PARAM_ID) < _STYLE_PROP_N_ITEMS)


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

static gboolean setColours(ZMapStyleColour colour, const char *border, const char *draw, const char *fill_str) ;
static gboolean parseColours(ZMapFeatureTypeStyle style, guint param_id, GValue *value) ;
//static gboolean isColourSet(ZMapFeatureTypeStyle style, int param_id, char *subpart) ;
static gboolean validSplit(char **strings,
                     ZMapStyleColourType *type_out, ZMapStyleDrawContext *context_out, char **colour_out) ;
static void formatColours(GString *colour_string, const char *type, ZMapStyleColour colour) ;



static ZMapStyleFullColour zmapStyleFullColour(ZMapFeatureTypeStyle style, ZMapStyleParamId id);
static gchar *zmapStyleValueColour(ZMapStyleFullColour this_colour);
static gboolean parseSubFeatures(ZMapFeatureTypeStyle style,gchar *str);
static gchar *zmapStyleValueSubFeatures(GQuark *quarks);
const char *zmapStyleParam2Name(ZMapStyleParamId id) ;

static gboolean styleMergeParam( ZMapFeatureTypeStyle dest, ZMapFeatureTypeStyle src, ZMapStyleParamId id) ;

static gpointer glyph_shape_copy(gpointer src) ;
static void glyph_shape_free(gpointer thing) ;

static void zmap_bin_to_hex(gchar *dest,guchar *src, int len) ;

/*
 *                    Globals
 */


/* all the parameters of a style, closely follows the style struct members
 *
 * NB: this must be fully populated and IN THE SAME ORDER as the
 * ZMapStyleParamId enum defined in zmapStyle.h
 *
 * it would be even better if we assigned each one to an array
 * using the id as an index
 *
 */
ZMapStyleParamStruct zmapStyleParams_G[_STYLE_PROP_N_ITEMS] =
{
  { STYLE_PROP_NONE, STYLE_PARAM_TYPE_INVALID,
    ZMAPSTYLE_PROPERTY_INVALID, "", "",
    0, ZMAPSTYLE_MODE_INVALID,
    0, 0, 0,
    NULL}, // (0 not used)

  { STYLE_PROP_IS_SET, STYLE_PARAM_TYPE_FLAGS,
    ZMAPSTYLE_PROPERTY_IS_SET, "isset", "parameter set flags",
    offsetof(ZMapFeatureTypeStyleStruct,is_set),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},          /* (internal/ automatic property) */

  { STYLE_PROP_NAME, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_NAME,
    "name", "Name",
    offsetof(ZMapFeatureTypeStyleStruct, original_id ),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL}, /* Name as text. */
  // review zMapStyleGetName() if you change this

  { STYLE_PROP_ORIGINAL_ID, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_ORIGINAL_ID,
    "original-id", "original id",
    offsetof(ZMapFeatureTypeStyleStruct, original_id ),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_UNIQUE_ID, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_UNIQUE_ID,
    "unique-id", "unique id",
    offsetof(ZMapFeatureTypeStyleStruct, unique_id) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_PARENT_STYLE, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_PARENT_STYLE,
    "parent-style", "parent style unique id",
    offsetof(ZMapFeatureTypeStyleStruct, parent_id),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_DESCRIPTION, STYLE_PARAM_TYPE_STRING, ZMAPSTYLE_PROPERTY_DESCRIPTION,
    "description", "Description",
    offsetof(ZMapFeatureTypeStyleStruct, description) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_MODE, STYLE_PARAM_TYPE_MODE, ZMAPSTYLE_PROPERTY_MODE,
    "mode", "Mode",
    offsetof(ZMapFeatureTypeStyleStruct, mode) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_COLOURS,        STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_COLOURS,
    "main colours", "Colours used for normal feature display.",
    offsetof(ZMapFeatureTypeStyleStruct,colours ),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_FRAME0_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME0_COLOURS,
    "frame0-colour-normal", "Frame0 normal colour",
    offsetof(ZMapFeatureTypeStyleStruct, frame0_colours),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_FRAME1_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME1_COLOURS,
    "frame1-colour-normal", "Frame1 normal colour",
    offsetof(ZMapFeatureTypeStyleStruct,frame1_colours ),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_FRAME2_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_FRAME2_COLOURS,
    "frame2-colour-normal", "Frame2 normal colour",
    offsetof(ZMapFeatureTypeStyleStruct,frame2_colours ) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_REV_COLOURS,    STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_REV_COLOURS,
    "main-colour-normal", "Reverse Strand normal colour",
    offsetof(ZMapFeatureTypeStyleStruct, strand_rev_colours) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_COLUMN_DISPLAY_MODE, STYLE_PARAM_TYPE_COLDISP, ZMAPSTYLE_PROPERTY_DISPLAY_MODE,
    "display-mode", "Display mode",
    offsetof(ZMapFeatureTypeStyleStruct, col_display_state ),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_BUMP_DEFAULT, STYLE_PARAM_TYPE_BUMP, ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE,
    "bump-mode", "The Default Bump Mode",
    offsetof(ZMapFeatureTypeStyleStruct, default_bump_mode),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_BUMP_MODE, STYLE_PARAM_TYPE_BUMP, ZMAPSTYLE_PROPERTY_BUMP_MODE,
    "initial-bump_mode", "Current bump mode",
    offsetof(ZMapFeatureTypeStyleStruct, initial_bump_mode),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_BUMP_FIXED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_BUMP_FIXED,
    "bump-fixed", "Style cannot be changed once set.",
    offsetof(ZMapFeatureTypeStyleStruct, bump_fixed) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_BUMP_SPACING, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_BUMP_SPACING,
    "bump-spacing", "space between columns in bumped columns",
    offsetof(ZMapFeatureTypeStyleStruct, bump_spacing) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_BUMP_STYLE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_BUMP_STYLE,
    "bump to different style", "bump to different style",
    offsetof(ZMapFeatureTypeStyleStruct, bump_style),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_FRAME_MODE, STYLE_PARAM_TYPE_3FRAME, ZMAPSTYLE_PROPERTY_FRAME_MODE,
    "3 frame display mode", "Defines frame sensitive display in 3 frame mode.",
    offsetof(ZMapFeatureTypeStyleStruct, frame_mode),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_MIN_MAG, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MIN_MAG,
    "min-mag", "minimum magnification",
    offsetof(ZMapFeatureTypeStyleStruct, min_mag),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_MAX_MAG, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MAX_MAG,
    "max-mag", "maximum magnification",
    offsetof(ZMapFeatureTypeStyleStruct, max_mag),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_WIDTH, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_WIDTH,
    "width", "width",
    offsetof(ZMapFeatureTypeStyleStruct, width),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_SCORE_MODE, STYLE_PARAM_TYPE_SCORE, ZMAPSTYLE_PROPERTY_SCORE_MODE,
    "score-mode", "Score Mode",
    offsetof(ZMapFeatureTypeStyleStruct, score_mode) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_SCORE_SCALE, STYLE_PARAM_TYPE_SCALE, ZMAPSTYLE_PROPERTY_SCORE_SCALE,
    "score-scale", "Score scaling",
    offsetof(ZMapFeatureTypeStyleStruct, score_scale) , ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_MIN_SCORE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MIN_SCORE,
    "min-score", "minimum-score",
    offsetof(ZMapFeatureTypeStyleStruct, min_score),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_MAX_SCORE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_MAX_SCORE,
    "max-score", "maximum score",
    offsetof(ZMapFeatureTypeStyleStruct, max_score),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_SUMMARISE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_SUMMARISE,
    "summarise featureset at low zoom", "summarise featureset at low zoom",
    offsetof(ZMapFeatureTypeStyleStruct, summarise), ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_COLLAPSE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_COLLAPSE,
    "collapse identical features into one", "collapse identical features into one",
    offsetof(ZMapFeatureTypeStyleStruct, collapse), ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},


  { STYLE_PROP_GFF_SOURCE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_GFF_SOURCE,
    "gff source", "GFF Source",
    offsetof(ZMapFeatureTypeStyleStruct, gff_source) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_GFF_FEATURE, STYLE_PARAM_TYPE_SQUARK, ZMAPSTYLE_PROPERTY_GFF_FEATURE,
    "gff feature", "GFF feature",
    offsetof(ZMapFeatureTypeStyleStruct, gff_feature),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_DISPLAYABLE, STYLE_PARAM_TYPE_BOOLEAN,ZMAPSTYLE_PROPERTY_DISPLAYABLE,
    "displayable", "Is the style Displayable",
    offsetof(ZMapFeatureTypeStyleStruct, displayable),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_SHOW_WHEN_EMPTY, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY,
    "show when empty", "Does the Style get shown when empty",
    offsetof(ZMapFeatureTypeStyleStruct, show_when_empty ) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_SHOW_TEXT, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_TEXT,
    "show-text",  "Show as Text",
    offsetof(ZMapFeatureTypeStyleStruct, showText) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_SUB_FEATURES, STYLE_PARAM_TYPE_SUB_FEATURES, ZMAPSTYLE_PROPERTY_SUB_FEATURES,
    "sub-features",  "Sub-features (glyphs)",
    offsetof(ZMapFeatureTypeStyleStruct, sub_features) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_STRAND_SPECIFIC, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,
    "strand specific", "Strand Specific ?",
    offsetof(ZMapFeatureTypeStyleStruct, strand_specific) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_SHOW_REV_STRAND, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,
    "show-reverse-strand", "Show Rev Strand ?",
    offsetof(ZMapFeatureTypeStyleStruct, show_rev_strand) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_HIDE_FWD_STRAND, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,
    "hide-forward-strand", "Hide Fwd Strand when RevComp'd?",
    offsetof(ZMapFeatureTypeStyleStruct, hide_fwd_strand) ,ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_SHOW_ONLY_IN_SEPARATOR, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,
    "only in separator", "Show Only in Separator",
    offsetof(ZMapFeatureTypeStyleStruct, show_only_in_separator),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_DIRECTIONAL_ENDS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,
    "directional-ends", "Display pointy \"short sides\"",
    offsetof(ZMapFeatureTypeStyleStruct, directional_end),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_COL_FILTER, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_COL_FILTER,
    "column filtering", "Column can be filtered",
    offsetof(ZMapFeatureTypeStyleStruct, col_filter_sensitive), ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},
  { STYLE_PROP_COL_FILTER_TOLERANCE, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_COL_FILTER_TOLERANCE,
    "filter tolerance", "Tolerance allowable in coords match for filtering.",
    offsetof(ZMapFeatureTypeStyleStruct, col_filter_tolerance), ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_FOO, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_FOO,
    "as Foo Canvas Items", "use old technology",
    offsetof(ZMapFeatureTypeStyleStruct, foo),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_FILTER, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_FILTER,
    "filter by score", "filter column by score",
    offsetof(ZMapFeatureTypeStyleStruct, filter),ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},

  { STYLE_PROP_OFFSET, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_OFFSET,
    "offset contents by x pixels", "offset contents by x pixels",
    offsetof(ZMapFeatureTypeStyleStruct, offset), ZMAPSTYLE_MODE_INVALID, 0, 0, 0, NULL},


  /*
   * mode dependant data
   */

  /* Glyph */

  { STYLE_PROP_GLYPH_NAME, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME,
    "glyph-name", "Glyph name used to reference glyphs config stanza",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_name),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE,
    "glyph-type", "Type of glyph to show.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_5, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_5,
    "glyph-name for 5' end", "Glyph name used to reference glyphs config stanza",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_5),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_5, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5,
    "glyph-type-5", "Type of glyph to show at 5' end.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph5), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_5_REV, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_5_REV,
    "glyph-name for 5' end reverse strand", "Glyph name used to reference glyphs config stanza",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_5_rev),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_5_REV, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5_REV,
    "glyph-type-5-rev", "Type of glyph to show at 5' end on reverse strand.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph5rev), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_3, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_3,
    "glyph-name for 3' end", "Glyph name used to reference glyphs config stanza",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_3),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_3, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3,
    "glyph-type-3", "Type of glyph to show at 3' end.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph3), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_3_REV, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_3_REV,
    "glyph-name for 3' end reverse strand", "Glyph name used to reference glyphs config stanza",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_name_3_rev),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_3_REV, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3_REV,
    "glyph-type-3-rev", "Type of glyph to show at 3' end on reverse strand.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph3rev), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_JUNCTION_5, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_JUNCTION_5,
    ZMAPSTYLE_PROPERTY_GLYPH_NAME_JUNCTION_5, "Glyph name used to reference glyph in config stanza.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.junction_name_5),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_JUNCTION_5, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_JUNCTION_5,
    ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_JUNCTION_5, "Type of glyph to show for 5' junction.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.junction_5), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_NAME_JUNCTION_3, STYLE_PARAM_TYPE_QUARK, ZMAPSTYLE_PROPERTY_GLYPH_NAME_JUNCTION_3,
    ZMAPSTYLE_PROPERTY_GLYPH_NAME_JUNCTION_3, "Glyph name used to reference in config stanza.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.junction_name_3),ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_SHAPE_JUNCTION_3, STYLE_PARAM_TYPE_GLYPH_SHAPE, ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_JUNCTION_3,
    ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_JUNCTION_3, "Type of glyph to show at 3' junction.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.junction_3), ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_ALT_COLOURS, STYLE_PARAM_TYPE_COLOUR,ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS,
    "alternate glyph colour", "Colours used to show glyphs when below thrashold.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_alt_colours), ZMAPSTYLE_MODE_GLYPH,
    0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_THRESHOLD, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD,
    "glyph-threshold", "Glyph threshold for alternate coloura",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_threshold) ,ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_STRAND, STYLE_PARAM_TYPE_GLYPH_STRAND, ZMAPSTYLE_PROPERTY_GLYPH_STRAND,
    "glyph-strand", "What to do for the reverse strand",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_strand) ,ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  { STYLE_PROP_GLYPH_ALIGN, STYLE_PARAM_TYPE_GLYPH_ALIGN, ZMAPSTYLE_PROPERTY_GLYPH_ALIGN,
    "glyph-align", "where to centre the glyph",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.glyph.glyph_align) ,ZMAPSTYLE_MODE_GLYPH, 0, 0, 0, NULL},

  /* Graph */

  { STYLE_PROP_GRAPH_MODE, STYLE_PARAM_TYPE_GRAPH_MODE, ZMAPSTYLE_PROPERTY_GRAPH_MODE,
    "graph-mode", "Graph Mode",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.mode) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_BASELINE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,
    "graph baseline", "Sets the baseline for graph values.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.baseline),ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_SCALE, STYLE_PARAM_TYPE_SCALE, ZMAPSTYLE_PROPERTY_GRAPH_SCALE,
    "graph-scale", "Graph Scaling",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.scale), ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_FILL, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_GRAPH_FILL,
    "graph-fill", "Graph fill mode",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.fill_flag) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_DENSITY, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY,
    "graph-density", "Density plot",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.density) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_DENSITY_FIXED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_FIXED,
    "graph-density-fixed", "anchor bins to pixel boundaries",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.fixed) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_DENSITY_MIN_BIN, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_MIN_BIN,
    "graph-density-min-bin", "min bin size in pixels",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.min_bin) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_DENSITY_STAGGER, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_GRAPH_DENSITY_STAGGER,
    "graph-density-stagger", "featureset/ column offset",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.stagger) ,ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},
  { STYLE_PROP_GRAPH_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_GRAPH_COLOURS,
    "graph colours", "Colours used to show the various types of graph.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.graph.colours), ZMAPSTYLE_MODE_GRAPH, 0, 0, 0, NULL},

  { STYLE_PROP_ALIGNMENT_PARSE_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,
    "parse gaps", "Parse Gaps ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.parse_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT, 0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_SHOW_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,
    "show gaps", "Show Gaps ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.show_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_ALWAYS_GAPPED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_ALWAYS_GAPPED,
    "always gapped", "Always Gapped ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.always_gapped) ,ZMAPSTYLE_MODE_ALIGNMENT ,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_UNIQUE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_UNIQUE,
    "unique", "Don't display same name as joined up",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.unique) ,ZMAPSTYLE_MODE_ALIGNMENT , 0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_BETWEEN_ERROR,STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN,
    "alignment between error", "Allowable alignment error between HSP's",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.between_align_error)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,
    "Allow misalign", "Allow match -> ref sequences to be different lengths.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.allow_misalign)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_PFETCHABLE, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE,
    "Pfetchable alignments", "Use pfetch proxy to get alignments ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.pfetchable)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_BLIXEM, STYLE_PARAM_TYPE_BLIXEM, ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM,
    "Blixemable alignments","Use blixem to view sequence of alignments ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.blixem_type)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_PERFECT_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS,
    "perfect alignment indicator colour", "Colours used to show two alignments have exactly contiguous coords.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.perfect)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS,
    "colinear alignment indicator colour", "Colours used to show two alignments have gapped contiguous coords.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.colinear)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,
    "noncolinear alignment indicator colour", "Colours used to show two alignments have overlapping coords.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.noncolinear)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_UNMARKED_COLINEAR, STYLE_PARAM_TYPE_COLDISP, ZMAPSTYLE_PROPERTY_ALIGNMENT_UNMARKED_COLINEAR,
    "paint colinear lines when unmarked", "paint colinear lines when unmarked ?",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.unmarked_colinear), ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_GAP_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_GAP_COLOURS,
    "gap between spilt read", "Colours used to show gap between two parts of a split read.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.perfect)  ,ZMAPSTYLE_MODE_ALIGNMENT, 0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_COMMON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_COMMON_COLOURS,
    "common part of squashed split read", "Colours used to show part of a squashed split read that is common to all source features.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.colinear)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_MIXED_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_MIXED_COLOURS,
    "mixed part of squashed split read that", "Colours used to show  part of a squashed split read that is not common to all source features.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.noncolinear)  ,ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_MASK_SETS, STYLE_PARAM_TYPE_QUARK_LIST_ID, ZMAPSTYLE_PROPERTY_ALIGNMENT_MASK_SETS,
    "mask featureset against others", "mask featureset against others",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.mask_sets), ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_SQUASH, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_SQUASH,
    "squash overlapping split reads into one", "squash overlapping split reads into one",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.squash),ZMAPSTYLE_MODE_ALIGNMENT, 0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_JOIN_OVERLAP, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_OVERLAP,
    "join overlapping reads into one", "join overlapping reads into one",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.join_overlap),ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_JOIN_THRESHOLD, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_THRESHOLD,
    "allow bases around splice junction when joining", "allow bases around splice junction when joining",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.join_overlap),ZMAPSTYLE_MODE_ALIGNMENT,
    0, 0, 0, NULL},
  { STYLE_PROP_ALIGNMENT_JOIN_MAX, STYLE_PARAM_TYPE_UINT, ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_MAX,
    "join overlapping reads into one", "join overlapping reads into one",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.alignment.join_max),ZMAPSTYLE_MODE_ALIGNMENT, 0, 0, 0, NULL},

  { STYLE_PROP_TRANSCRIPT_TRUNC_LEN, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_TRANSCRIPT_TRUNC_LEN,
    "show dotted intron at edge of partial trancript", "show dotted intron at edge of partial trancript",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.transcript.truncated_intron_length),ZMAPSTYLE_MODE_TRANSCRIPT,
    0, 0, 0, NULL},


  { STYLE_PROP_SEQUENCE_NON_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_NON_CODING_COLOURS,
    "non-coding exon region colour", "Colour used to highlight UTR section of an exon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.non_coding), ZMAPSTYLE_MODE_SEQUENCE, 0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_CODING_COLOURS,
    "coding exon region colour", "Colour used to highlight coding section of an exon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.coding), ZMAPSTYLE_MODE_SEQUENCE, 0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_SPLIT_CODON_5_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_5_COLOURS,
    "coding exon split codon colour", "Colour used to highlight split codon 5' coding section of an exon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.split_codon_5), ZMAPSTYLE_MODE_SEQUENCE,
    0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_SPLIT_CODON_3_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_SPLIT_CODON_3_COLOURS,
    "coding exon split codon colour", "Colour used to highlight split codon 3' coding section of an exon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.split_codon_3), ZMAPSTYLE_MODE_SEQUENCE,
    0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_IN_FRAME_CODING_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_IN_FRAME_CODING_COLOURS,
    "in-frame coding exon region colour", "Colour used to highlight coding section of an exon that is in frame.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.in_frame_coding), ZMAPSTYLE_MODE_SEQUENCE,
    0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_START_CODON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_START_CODON_COLOURS,
    "start codon colour", "Colour used to highlight start codon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.start_codon), ZMAPSTYLE_MODE_SEQUENCE,
    0, 0, 0, NULL},
  { STYLE_PROP_SEQUENCE_STOP_CODON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_SEQUENCE_STOP_CODON_COLOURS,
    "stop codon colour", "Colour used to highlight stop codon.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.sequence.stop_codon), ZMAPSTYLE_MODE_SEQUENCE, 0, 0, 0, NULL},

  { STYLE_PROP_TRANSCRIPT_CDS_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS,
    "CDS colours", "Colour used to CDS section of a transcript.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.transcript.CDS_colours), ZMAPSTYLE_MODE_TRANSCRIPT,
    0, 0, 0, NULL},

  { STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ASSEMBLY_PATH_NON_COLOURS,
    "non-path colours", "Colour used for unused part of an assembly path sequence.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.assembly_path.non_path_colours),
    ZMAPSTYLE_MODE_ASSEMBLY_PATH , 0, 0, 0, NULL},

  { STYLE_PROP_TEXT_FONT, STYLE_PARAM_TYPE_STRING, ZMAPSTYLE_PROPERTY_TEXT_FONT,
    "Text Font", "Font to draw text with.",
    offsetof(ZMapFeatureTypeStyleStruct, mode_data.text.font)  ,ZMAPSTYLE_MODE_TEXT, 0, 0, 0, NULL}

  /* No guard entry ?? */

} ;




/* Class pointer for styles. */
static GObjectClass *style_parent_class_G ;

static GQuark splice_style_id_G = 0 ;







/*
 *                  External Interface Routines
 */


GType zMapStyleGlyphShapeGetType(void)
{
  static GType type = 0 ;

  if (!type)
    {
      type = g_boxed_type_register_static("glyph-shape", glyph_shape_copy, glyph_shape_free) ;
    }

  return type ;
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
          sizeof (ZMapFeatureTypeStyleClassStruct),
          (GBaseInitFunc)      NULL,
          (GBaseFinalizeFunc)  NULL,
          (GClassInitFunc)     zmap_feature_type_style_class_init,
          (GClassFinalizeFunc) NULL,
          NULL /* class_data */,
          sizeof (ZMapFeatureTypeStyleStruct),
          0 /* n_preallocs */,
          (GInstanceInitFunc)  zmap_feature_type_style_init,
          NULL
        };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "ZMapFeatureTypeStyle",
                                     &info, (GTypeFlags)0);

      /* try to avoid quarking this for every feature..... */
      splice_style_id_G = g_quark_from_string(ZMAPSTYLE_LEGACY_3FRAME);
    }

  return type;

}


ZMapFeatureTypeStyle zMapStyleCreate(const char *name, const char *description)
{
  ZMapFeatureTypeStyle style = NULL ;
  GParameter params[2] ;
  guint num_params ;

  if (!(name && *name && (!description || *description)) )
    return style ;

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

  if ((style = styleCreate(n_parameters, parameters)))
    {

      /* should be in styleCreate function surely..... */
      if (style->mode ==ZMAPSTYLE_MODE_BASIC || style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
        {
          /* default summarise to 1000 to get round lack of configuration, can always set to zero if wanted */
          if (!zMapStyleIsPropertySetId(style,STYLE_PROP_SUMMARISE))
            {
              zmapStyleSetIsSet(style,STYLE_PROP_SUMMARISE);
              style->summarise = 1000.0 ;
            }
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
  ZMapStyleParam param = zmapStyleParams_G ;
  int i ;
  gboolean status = TRUE ;


  zMapReturnValIfFailSafe((ZMAP_IS_FEATURE_STYLE(src)), dest) ;

  dest = (ZMapFeatureTypeStyle)g_object_new(ZMAP_TYPE_FEATURE_STYLE, NULL) ;

  for (i = 1 ; i < _STYLE_PROP_N_ITEMS ; i++, param++)
    {
      if (zMapStyleIsPropertySetId(src, param->id)
          && !(status = styleMergeParam(dest, src, param->id)))
        {
          zMapLogCritical("Copy of style %s failed.", g_quark_to_string(src->original_id)) ;

          break ;
        }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Big change so leaving this code in until I've tested further..... */

      if (zMapStyleIsPropertySetId(src, param->id))      // something to copy?
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

                break;
              }
            }


          /* Um....does this not need doing ??....not sure.... */
  //          zmapStyleSetIsSet(curr_style,param->id);
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  if (!status)
    {
      zMapStyleDestroy(dest) ;
      dest = NULL ;
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
 * Get the property with the given param_id and return its value
 *
 * @param   style          The style.
 * @param   param_id       The id of the parameter.
 * @param   value          GValue in which to populate the result.
 * @return  gboolean       true if succeeded
 *  */
gboolean zMapStyleGetValue(ZMapFeatureTypeStyle style, ZMapStyleParamId param_id, GValue *value)
{
  gboolean ok = FALSE ;

  if (style && zMapStyleIsPropertySetId(style, param_id))
    {
      ok = TRUE ;
      const char *param_name = zmapStyleParam2Name(param_id) ;
      GParamSpec *pspec = g_object_class_find_property(G_OBJECT_CLASS(ZMAP_FEATURE_STYLE_GET_CLASS(style)), param_name) ;
      ZMapStyleParam param = &zmapStyleParams_G [pspec->param_id];

      /* Not sure if there's a better way to do this but we need to init the gvalue before we can
       * call get_property */
      switch(param->type)
        {
        case STYLE_PARAM_TYPE_BOOLEAN:
          g_value_init(value, G_TYPE_BOOLEAN) ;
          break;

        case STYLE_PARAM_TYPE_DOUBLE:
          g_value_init(value, G_TYPE_DOUBLE) ;
          break;

        case STYLE_PARAM_TYPE_STRING:        // fall through; gchar *
        case STYLE_PARAM_TYPE_SQUARK:        // fall through; gchar * stored as a quark
        case STYLE_PARAM_TYPE_FLAGS:         // fall through; bitmap of is_set flags (array of uchar)
        case STYLE_PARAM_TYPE_COLOUR:        // fall through; ZMapStyleFullColourStruct
        case STYLE_PARAM_TYPE_SUB_FEATURES:  // fall through; GQuark[]
        case STYLE_PARAM_TYPE_QUARK_LIST_ID:
          g_value_init(value, G_TYPE_STRING) ;
          break;

        case STYLE_PARAM_TYPE_GLYPH_SHAPE:
          g_value_init(value, zMapStyleGlyphShapeGetType()) ;
          break ;

        case STYLE_PARAM_TYPE_QUARK:        // fall through
        case STYLE_PARAM_TYPE_MODE:         // fall through
        case STYLE_PARAM_TYPE_COLDISP:      // fall through
        case STYLE_PARAM_TYPE_BUMP:         // fall through
        case STYLE_PARAM_TYPE_3FRAME:       // fall through
        case STYLE_PARAM_TYPE_SCORE:        // fall through
        case STYLE_PARAM_TYPE_GRAPH_MODE:   // fall through
        case STYLE_PARAM_TYPE_SCALE:        // fall through
        case STYLE_PARAM_TYPE_BLIXEM:       // fall through
        case STYLE_PARAM_TYPE_GLYPH_STRAND: // fall through
        case STYLE_PARAM_TYPE_GLYPH_ALIGN:  // fall through
        case STYLE_PARAM_TYPE_UINT:
          g_value_init(value, G_TYPE_UINT) ;
          break;

        default:
          ok = FALSE ;
          zMapWarnIfReached() ;
          break;
        }

      /* Now we've set the correct value type we can just call get_property */
      if (ok)
        g_object_get_property(G_OBJECT(style), param_name, value) ;
    }

  return ok ;
}


/*!
 * Get the property with the given param_id and return its value as a string
 *
 * @param   style          The style.
 * @param   param_id       The id of the parameter.
 * @return  char*          Newly allocated string.
 *  */
char* zMapStyleGetValueAsString(ZMapFeatureTypeStyle style, ZMapStyleParamId param_id)
{
  char *result = NULL ;

  if (style && zMapStyleIsPropertySetId(style, param_id))
    {
      const char *param_name = zmapStyleParam2Name(param_id) ;
      GParamSpec *pspec = g_object_class_find_property(G_OBJECT_CLASS(ZMAP_FEATURE_STYLE_GET_CLASS(style)), param_name) ;
      ZMapStyleParam param = &zmapStyleParams_G [pspec->param_id];

      /* Not sure if there's a better way to do this but we need to init the gvalue before we can
       * call get_property */
      switch(param->type)
        {
        case STYLE_PARAM_TYPE_BOOLEAN:
          {
            gboolean value = (* (gboolean *) (((size_t) style) + param->offset));
            result = g_strdup_printf("%s", value ? "true" : "false") ;
            break;
          }

        case STYLE_PARAM_TYPE_DOUBLE:
          {
            double value = (* (double *) (((size_t) style) + param->offset));
            result = g_strdup_printf("%f", value) ;
            break;
          }

        case STYLE_PARAM_TYPE_STRING:              // gchar *
          {
            const gchar *value = (gchar *) (((size_t) style) + param->offset);
            result = g_strdup(value) ;
            break;
          }

        case STYLE_PARAM_TYPE_QUARK: //fall through
        case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
          {
            GQuark value = (* (GQuark *) (((size_t) style) + param->offset));
            result = g_strdup(g_quark_to_string(value)) ;
            break;
          }

        case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)
          {
            result = (gchar*)g_malloc(STYLE_IS_SET_SIZE * 2 + 1);
            zmap_bin_to_hex(result,(guchar*)(((size_t) style) + param->offset), STYLE_IS_SET_SIZE);
            break;
          }

        case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
          {
            result = zmapStyleValueColour((ZMapStyleFullColour) (((size_t) style) + param->offset));
            break;
          }

        case STYLE_PARAM_TYPE_SUB_FEATURES:        // GQuark[]
          {
            result = zmapStyleValueSubFeatures((GQuark *)(((size_t) style) + param->offset));
            break;
          }

        case STYLE_PARAM_TYPE_QUARK_LIST_ID:
          {
            result = zMap_g_list_quark_to_string(*(GList **)(((size_t) style) + param->offset), NULL);
            break;
          }

        case STYLE_PARAM_TYPE_GLYPH_SHAPE:
          {
            //! \todo Implement get-value for glyph-shape param
            //g_value_set_boxed(value, shape);
            break;
          }

          // enums treated as uint. This is a pain: can we know how big an enum is? (NO)
          // Some pretty choice code but it's not safe to do it the easy way
#define STYLE_GET_PROP2(s_param,s_type,s_func)        \
          { \
            case s_param : \
              guint value = (guint) (* (s_type *) (((size_t) style) + param->offset)); \
              result = g_strdup(s_func((s_type)value)) ;                \
              break ;                                 \
          }

          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_MODE            , ZMapStyleMode,               zMapStyleMode2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_COLDISP         , ZMapStyleColumnDisplayState, zmapStyleColDisplayState2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_BUMP            , ZMapStyleBumpMode,           zmapStyleBumpMode2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_3FRAME          , ZMapStyle3FrameMode,         zmapStyle3FrameMode2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_SCORE           , ZMapStyleScoreMode,          zmapStyleScoreMode2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_GRAPH_MODE      , ZMapStyleGraphMode,          zmapStyleGraphMode2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_SCALE           , ZMapStyleScale,              zmapStyleScale2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_BLIXEM          , ZMapStyleBlixemType,         zmapStyleBlixemType2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_GLYPH_STRAND    , ZMapStyleGlyphStrand,        zmapStyleGlyphStrand2ShortText);
          STYLE_GET_PROP2 (STYLE_PARAM_TYPE_GLYPH_ALIGN     , ZMapStyleGlyphAlign,         zmapStyleGlyphAlign2ShortText);

        case STYLE_PARAM_TYPE_UINT:
          {
            guint value = (* (guint *) ((size_t)style + param->offset));
            result = g_strdup_printf("%d", value) ;
            break;
          }

        default:
          {
            zMapWarnIfReached() ;
            break;
          }
        }
    }

  return result ;
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


/* This is just like g_object_set() except that it returns a boolean
 * to indicate success or failure. Note that if you supply several
 * properties you will need to look in the logs to see which one failed.
 *
 * @param   style          The style.
 * @param   first_property_name  The start of the property name/variable list
 *                               (see g_object_set() for format of list).
 * @return  gboolean       TRUE if all properties set, FALSE otherwise.
 *  */
gboolean zMapStyleSet(ZMapFeatureTypeStyle style, const char *first_property_name, ...)
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


/* Copy a single property from one style to the another.
 * Returns TRUE if the property is set in src_style and could be merged in to dest_style, FALSE
 * otherwise.
 * Note, caller does not have to check whether the property is set in src_style as this function
 * does that anyway.
 */
gboolean zMapStyleMergeProperty(ZMapFeatureTypeStyle dest_style, ZMapFeatureTypeStyle src_style, ZMapStyleParamId id)
{
  gboolean result = FALSE ;

  zMapReturnValIfFailSafe((ZMAP_IS_FEATURE_STYLE(dest_style) && ZMAP_IS_FEATURE_STYLE(src_style)
                           && PARAM_ID_IS_VALID(id)
                           && zMapStyleIsPropertySetId(src_style, id)), result) ;

  result = styleMergeParam(dest_style, src_style, id) ;

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
  if(style && (style->is_set[param->flag_ind] & param->flag_bit))
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

/* Converts a numerical paramid to it's corresponding string name. */
const char *zmapStyleParam2Name(ZMapStyleParamId id)
{
  const char *param_name = NULL ;

  zMapReturnValIfFailSafe((PARAM_ID_IS_VALID(id)), param_name) ;

  param_name = zmapStyleParams_G[id].name ;

  return param_name ;
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

  if (!style || !name || !*name)
    return result ;

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
/* (mh17) NOTE THIS FUNCTION IS ONLY CALLED FROM OBSCURE PLACES AND IS NOT RUN FOR THE MAJORITY OF DRAWING OPERATIONS
 * SO ATTEMPTING TO ADD STYLE DEFAULTS HERE IS DOOMED TO FAILURE
 */
gboolean zMapStyleIsDrawable(ZMapFeatureTypeStyle style, GError **error)
{
  gboolean valid = TRUE ;
  GQuark domain ;
  gint code = 0 ;
  char *message ;

  zMapReturnValIfFail(((style && error && !(*error))), FALSE) ;

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
            /* THIS IS RUBBISH...SHOULD HAVE A FLAG SET WHEN A GLYPH STYLE IS SET UP.... */

            if (style->mode_data.glyph.glyph.n_coords < 2
                && style->mode_data.glyph.glyph5.n_coords < 2
                && style->mode_data.glyph.glyph3.n_coords < 2)
              {
                valid = FALSE ;
                code = 10 ;
                message = g_strdup("No Glyph defined.") ;
              }
            break ;
          }
        default:
          {
            valid = FALSE ;
            message = g_strdup("Invalid style mode.") ;
            zMapWarnIfReached() ;
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
            if (!(style->colours.normal.fields_set.fill_col) && !(style->colours.normal.fields_set.border_col))
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
            if (!(style->colours.normal.fields_set.fill_col) || !(style->colours.normal.fields_set.draw_col))
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
                       && (!(style->frame0_colours.normal.fields_set.fill_col)
                   || !(style->frame1_colours.normal.fields_set.fill_col)
                   || !(style->frame2_colours.normal.fields_set.fill_col)))
              {
                valid = FALSE ;
                code = 11 ;
                message = g_strdup_printf("Splice mode requires all frame colours to be set unset frames are:%s%s%s",
                                          (style->frame0_colours.normal.fields_set.fill_col ? "" : " frame0"),
                                          (style->frame1_colours.normal.fields_set.fill_col ? "" : " frame1"),
                                          (style->frame2_colours.normal.fields_set.fill_col ? "" : " frame2")) ;
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
            valid = FALSE ;
            message = g_strdup("Invalid style mode.") ;
            zMapWarnIfReached() ;
          }
        }
    }


  /* Construct the error if there was one. */
  if (!valid)
    *error = g_error_new_literal(domain, code, message) ;


  return valid ;
}



/* Takes a style and defaults enough properties for the style to be used to
 * draw features (albeit boringly), returns FALSE if style is set to non-displayable (e.g. for meta-styles). */
gboolean zMapStyleMakeDrawable(char *config_file, ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;
  if (!style)
    return result ;

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
        case ZMAPSTYLE_MODE_SEQUENCE:
        case ZMAPSTYLE_MODE_TEXT:
          {
            if (!(style->colours.normal.fields_set.fill_col))
              zMapStyleSetColoursStr(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                                     "white", NULL, NULL) ;

            if (!(style->colours.normal.fields_set.draw_col))
              zMapStyleSetColoursStr(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
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

            if (zMapStyleIsSpliceStyle(style))
              {
                if(!zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE) &&
                   (!zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_3) ||
                    !zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_5))
                   )
                  {
                    ZMapFeatureTypeStyle s_3frame;

                    s_3frame = zMapStyleLegacyStyle(config_file, ZMAPSTYLE_LEGACY_3FRAME) ;
                    if(s_3frame)
                      zMapStyleMerge(style,s_3frame);
                  }
              }

            break;
          }
        default:
          {
            if (!(style->colours.normal.fields_set.fill_col) && !(style->colours.normal.fields_set.border_col))
              {
                zMapStyleSetColoursStr(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                                       NULL, NULL, "black") ;
              }

            break ;
          }
        }

      result = TRUE ;
    }

  return result ;
}



/* Does style represent the splice style used in acedb to represent gene finder output. */
gboolean zMapStyleIsSpliceStyle(ZMapFeatureTypeStyle style)
{
#if 0
  // calling this several times for every feature has got to be quite slow

  gboolean is_splice = FALSE ;

  is_splice = (style->unique_id == g_quark_from_string(ZMAPSTYLE_LEGACY_3FRAME)) ;

  return is_splice ;
#else
  return style->unique_id == splice_style_id_G;
#endif
}



ZMapStyleFullColour zmapStyleFullColour(ZMapFeatureTypeStyle style, ZMapStyleParamId id)
{
  ZMapStyleFullColour full_colour = NULL ;

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

    case STYLE_PROP_GRAPH_COLOURS:
      full_colour = &(style->mode_data.graph.colours) ;
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
      zMapWarnIfReached() ;
      break;
    }
  return(full_colour);
}




ZMapStyleColour zmapStyleColour(ZMapStyleFullColour full_colour,ZMapStyleColourType type)
{
  ZMapStyleColour colour = NULL ;

  switch (type)
    {
    case ZMAPSTYLE_COLOURTYPE_NORMAL:
      colour = &(full_colour->normal) ;
      break ;
    case ZMAPSTYLE_COLOURTYPE_SELECTED:
      colour = &(full_colour->selected) ;
      break ;
    default:
      zMapWarnIfReached() ;
      break ;
    }
  return(colour);
}


/* I'm going to try these more generalised functions.... */

gboolean zMapStyleSetColoursStr(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
                                const char *fill_str, const char *draw, const char *border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  if (!style)
    return result ;

  full_colour = zmapStyleFullColour(style, target);
  colour = zmapStyleColour(full_colour, type);

  if (!(result = setColours(colour, border, draw, fill_str)))
    {
      zMapLogCritical("Style \"%s\", bad colours specified:%s\"%s\"%s\"%s\"%s\"%s\"",
                      zMapStyleGetName(style),
                      border ? "  border " : "",
                      border ? border : "",
                      draw ? "  draw " : "",
                      draw ? draw : "",
                      fill_str ? "  fill_str " : "",
                      fill_str ? fill_str : "") ;
    }
  else
    {
      zmapStyleSetIsSet(style,target);
    }

  return result ;
}


gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
                             GdkColor *fill_col, GdkColor *draw, GdkColor *border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;


  full_colour = zmapStyleFullColour(style, target);
  colour = zmapStyleColour(full_colour, type);

  if (fill_col)
    {
      colour->fields_set.fill_col = TRUE ;
      colour->fill_col = *fill_col ;
    }

  if (draw)
    {
      colour->fields_set.draw_col = TRUE ;
      colour->draw_col = *draw ;
    }

  if (border)
    {
      colour->fields_set.border_col = TRUE ;
      colour->border_col = *border ;
    }

  zmapStyleSetIsSet(style, target) ;

  return result ;
}


gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleParamId target, ZMapStyleColourType type,
     GdkColor **fill_col, GdkColor **draw, GdkColor **border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  if (!(style && (fill_col || draw || border)) )
    return result ;

  if (! zMapStyleIsPropertySetId(style,target))
    return(result);

  full_colour = zmapStyleFullColour(style,target);
  colour = zmapStyleColour(full_colour,type);

  if (fill_col)
    {
      if (colour->fields_set.fill_col)
        {
          *fill_col = &(colour->fill_col) ;
          result = TRUE ;
        }
    }

  if (draw)
    {
      if (colour->fields_set.draw_col)
        {
          *draw = &(colour->draw_col) ;
          result = TRUE ;
        }
    }

  if (border)
    {
      if (colour->fields_set.border_col)
        {
          *border = &(colour->border_col) ;
          result = TRUE ;
        }
    }

  return result ;
}


gboolean zMapStyleColourByStrand(ZMapFeatureTypeStyle style)
{
  gboolean colour_by_strand = FALSE;

  if(style->strand_rev_colours.normal.fields_set.fill_col ||
     style->strand_rev_colours.normal.fields_set.draw_col ||
     style->strand_rev_colours.normal.fields_set.border_col)
    colour_by_strand = TRUE;

  return colour_by_strand;
}



gboolean zMapStyleIsColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  gboolean is_colour = FALSE ;

  if (!style)
    return is_colour ;

  switch(colour_context)
    {
    case ZMAPSTYLE_DRAW_FILL:
      is_colour = style->colours.normal.fields_set.fill_col ;
      break ;
    case ZMAPSTYLE_DRAW_DRAW:
      is_colour = style->colours.normal.fields_set.draw_col ;
      break ;
    case ZMAPSTYLE_DRAW_BORDER:
      is_colour = style->colours.normal.fields_set.border_col ;
      break ;
    default:
      zMapWarnIfReached() ;
    }

  return is_colour ;
}

GdkColor *zMapStyleGetColour(ZMapFeatureTypeStyle style, ZMapStyleDrawContext colour_context)
{
  GdkColor *colour = NULL ;

  if (!style)
    return colour ;

  if (zMapStyleIsColour(style, colour_context))
    {
      switch(colour_context)
        {
        case ZMAPSTYLE_DRAW_FILL:
          colour = &(style->colours.normal.fill_col) ;
          break ;
        case ZMAPSTYLE_DRAW_DRAW:
          colour = &(style->colours.normal.draw_col) ;
          break ;
        case ZMAPSTYLE_DRAW_BORDER:
          colour = &(style->colours.normal.fill_col) ;
          break ;
        default:
          zMapWarnIfReached() ;
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


/* Copy a style param from one style to another. Takes no action unless the param is
 * set in src. */
static gboolean styleMergeParam( ZMapFeatureTypeStyle dest, ZMapFeatureTypeStyle src, ZMapStyleParamId id)
{
  gboolean result = FALSE ;

  if (zMapStyleIsPropertySetId(src, id))
    {
      ZMapStyleParam param = &(zmapStyleParams_G[id]) ;

      switch(param->type)
        {
        case STYLE_PARAM_TYPE_STRING:
          {
            gchar **srcstr, **dststr;

            srcstr = (gchar **) (((size_t)src) + param->offset);
            dststr = (gchar **) (((size_t)dest) + param->offset);

            *dststr = g_strdup(*srcstr);

            break;
          }
        case STYLE_PARAM_TYPE_QUARK_LIST_ID:
          {
            GList **sl,**dl;

            sl = (GList **) (((size_t) src) + param->offset);
            dl = (GList **) (((size_t) dest) + param->offset);

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

            srcval = (void*) ((size_t)src + param->offset );
            dstval = (void*) ((size_t)dest + param->offset );

            memcpy(dstval, srcval, param->size) ;

            break;
          }
        }

      zmapStyleSetIsSet(dest, param->id) ;

      result = TRUE ;
    }

  return result ;
}


static gboolean setColours(ZMapStyleColour colour, const char *border, const char *draw, const char *fill_str)
{
  gboolean status = FALSE ;
  ZMapStyleColourStruct tmp_colour = {{0}} ;

  if (!colour)
    return status ;

  status = TRUE ;

  if (status && border && *border)
    {
      if ((status = gdk_color_parse(border, &(tmp_colour.border_col))))
        {
          colour->fields_set.border_col = TRUE ;
          colour->border_col = tmp_colour.border_col ;

        }
    }
  if (status && draw && *draw)
    {
      if ((status = gdk_color_parse(draw, &(tmp_colour.draw_col))))
        {
          colour->fields_set.draw_col = TRUE ;
          colour->draw_col = tmp_colour.draw_col ;
        }
    }
  if (status && fill_str && *fill_str)
    {
      if ((status = gdk_color_parse(fill_str, &(tmp_colour.fill_col))))
        {
          colour->fields_set.fill_col = TRUE ;
          colour->fill_col = tmp_colour.fill_col ;
        }
    }

  return status ;
}


// store coordinate pairs in the struct and work out type
ZMapStyleGlyphShape zMapStyleGetGlyphShape(const gchar *shape, GQuark id)
{
  ZMapStyleGlyphShape glyph_shape = NULL ;
  gchar **spec = NULL, **segments = NULL, **s = NULL, **points = NULL, **p = NULL, *q = NULL ;
  gboolean syntax = FALSE ;
  gint x,y,n ;
  gint *cp ;


  glyph_shape = g_new0(ZMapStyleGlyphShapeStruct,1) ;

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

  if (!syntax)
    {
      int minx = 0, maxx = 0, miny = 0,maxy = 0;/* anchor size to origin */
      int i,coord;

      for (i = 0; i < glyph_shape->n_coords * 2;i++)
        {
          coord = glyph_shape->coords[i];

          if (coord == GLYPH_COORD_INVALID)
            coord = (int) GLYPH_CANVAS_COORD_INVALID; // zero, will be ignored

          if (i & 1)/* Y coord */
            {
              if(miny > coord)
                miny = coord;

              if(maxy < coord)
                maxy = coord;
            }
          else        // X coord
            {
              if(minx > coord)
                minx = coord;

              if(maxx < coord)
                maxx = coord;
            }
        }

      glyph_shape->width  = maxx - minx + 1;
      glyph_shape->height = maxy - miny + 1;

      if (glyph_shape->type == GLYPH_DRAW_LINES && x == glyph_shape->coords[0] && y == glyph_shape->coords[1])
        {
          glyph_shape->type = GLYPH_DRAW_POLYGON;
        }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      return(glyph_shape) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      syntax = TRUE ;

    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_free(glyph_shape);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!syntax)
    {
      g_free(glyph_shape) ;
      glyph_shape = NULL ;
    }

  return glyph_shape ;
}


// allow old ACEDB interface to use new configurable glyph styles
// we invent two styles pre3viosuly hard coded in bits
// only do this if [ZMap] legacy_styles=TRUE
ZMapFeatureTypeStyle zMapStyleLegacyStyle(char *config_file, const char *name)
{
  static ZMapFeatureTypeStyle s_homology = NULL;
  static ZMapFeatureTypeStyle s_3frame = NULL;
  static gboolean got = FALSE ;
  char *hn;

  hn = (char *) zmapStyleSubFeature2ExactStr(ZMAPSTYLE_SUB_FEATURE_HOMOLOGY);

  if (!got)
    {
      got = TRUE ;

      if (zMapConfigLegacyStyles(config_file))  // called here as we want to do it only once
        {
          /* Triangle markers at start/end of incomplete homology features. */
          s_homology = zMapStyleCreate(hn, "homology - legacy style");

          g_object_set(G_OBJECT(s_homology),
                       ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_GLYPH,
                       ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, "up-tri",
                       ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5, zMapStyleGetGlyphShape("<0,-4 ;-4,0 ;4,0 ;0,-4>",
                                                                                g_quark_from_string("up-tri")),
                       ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, "dn-tri",
                       ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3, zMapStyleGetGlyphShape("<0,4; -4,0 ;4,0; 0,4>",
                                                                                g_quark_from_string("dn-tri")),
                       ZMAPSTYLE_PROPERTY_SCORE_MODE, ZMAPSTYLE_SCORE_ALT,
                       ZMAPSTYLE_PROPERTY_GLYPH_THRESHOLD, 5,
                       ZMAPSTYLE_PROPERTY_COLOURS, "normal fill red; normal border black",
                       ZMAPSTYLE_PROPERTY_GLYPH_ALT_COLOURS, "normal fill green; normal border black",
                       NULL);

          /* Markers for splice features, produced by genefinders.
           * The default shapes are |_ and same thing upside down, the arms are 10 pixels long.
           * The horizontal line is drawn between the two bases that flank the splice. */
          s_3frame = zMapStyleCreate(ZMAPSTYLE_LEGACY_3FRAME,"Splice Markers - legacy style for gene finder features");

          g_object_set(G_OBJECT(s_3frame),
               ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_GLYPH,

               ZMAPSTYLE_PROPERTY_GLYPH_NAME_3, "up-hook",
               ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_3,
               zMapStyleGetGlyphShape(ZMAPSTYLE_SPLICE_GLYPH_3, g_quark_from_string("up-hook")),

               ZMAPSTYLE_PROPERTY_GLYPH_NAME_5, "dn-hook",
               ZMAPSTYLE_PROPERTY_GLYPH_SHAPE_5,
               zMapStyleGetGlyphShape(ZMAPSTYLE_SPLICE_GLYPH_5, g_quark_from_string("dn-hook")),

               ZMAPSTYLE_PROPERTY_FRAME_MODE, ZMAPSTYLE_3_FRAME_ONLY_1,
               ZMAPSTYLE_PROPERTY_SCORE_MODE, ZMAPSCORE_WIDTH,
               ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,FALSE,
               ZMAPSTYLE_PROPERTY_HIDE_FORWARD_STRAND,TRUE,
                       ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,TRUE,

               /* sets horizontal scale of splices. */
               ZMAPSTYLE_PROPERTY_WIDTH, 100.0,
               ZMAPSTYLE_PROPERTY_MIN_SCORE, -2.0,
               ZMAPSTYLE_PROPERTY_MAX_SCORE, 4.0,

               /* Frame specific colouring and default fill colour on selection. */
               ZMAPSTYLE_PROPERTY_COLOURS, "normal fill grey",
               ZMAPSTYLE_PROPERTY_FRAME0_COLOURS,
               "normal fill blue; normal border blue; selected fill pink",
               ZMAPSTYLE_PROPERTY_FRAME1_COLOURS,
               "normal fill green; normal border green; selected fill pink",
               ZMAPSTYLE_PROPERTY_FRAME2_COLOURS,
               "normal fill red; normal border red; selected fill pink",
               NULL) ;
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

  if ((style = (ZMapFeatureTypeStyle)g_object_newv(ZMAP_TYPE_FEATURE_STYLE, n_parameters, parameters)))
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

size_t zmapStyleParamSize(ZMapStyleParamType type)
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
    case STYLE_PARAM_TYPE_SCALE:       return(sizeof(ZMapStyleScale));  break;
    case STYLE_PARAM_TYPE_BLIXEM:      return(sizeof(ZMapStyleBlixemType)); break;
    case STYLE_PARAM_TYPE_GLYPH_STRAND:return(sizeof(ZMapStyleGlyphStrand)); break;
    case STYLE_PARAM_TYPE_GLYPH_ALIGN: return(sizeof(ZMapStyleGlyphAlign)); break;

    case STYLE_PARAM_TYPE_GLYPH_SHAPE: return(sizeof(ZMapStyleGlyphShapeStruct)); break;
    case STYLE_PARAM_TYPE_SUB_FEATURES:return(sizeof(GQuark) * ZMAPSTYLE_SUB_FEATURE_MAX); break;

    case STYLE_PARAM_TYPE_QUARK_LIST_ID: return(sizeof (GList *)); break;

    case STYLE_PARAM_TYPE_INVALID:
    default:
      zMapWarnIfReached();
      break;
    }
  return (size_t) 0 ;
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
                                zMapStyleGlyphShapeGetType(), (GParamFlags)G_PARAM_READWRITE);
      break;


      // enums treated as uint, but beware: ANSI C leave the size unspecifies
      // so it's compiler dependant and could be changed via optimisation flags

    case STYLE_PARAM_TYPE_MODE:                // ZMapStyleMode
    case STYLE_PARAM_TYPE_COLDISP:             // ZMapStyleColumnDisplayState
    case STYLE_PARAM_TYPE_BUMP:                // ZMapStyleBumpMode
    case STYLE_PARAM_TYPE_3FRAME:              // ZMapStyle3FrameMode
    case STYLE_PARAM_TYPE_SCORE:               // ZMapStyleScoreMode
    case STYLE_PARAM_TYPE_SCALE:               // ZMapStyleScale
    case STYLE_PARAM_TYPE_GRAPH_MODE:          // ZMapStyleGraphMode
    case STYLE_PARAM_TYPE_BLIXEM:              // ZMapStyleBlixemType
    case STYLE_PARAM_TYPE_GLYPH_STRAND:        // ZMapStyleGlyphStrand
    case STYLE_PARAM_TYPE_GLYPH_ALIGN:         // ZMapStyleGlyphAlign

    case STYLE_PARAM_TYPE_UINT:

      gps = g_param_spec_uint(param->name, param->nick,param->blurb,
      0, G_MAXUINT, 0,        // we don't use these, but if we wanted to we could do it in ....object_set()
      ZMAP_PARAM_STATIC_RW); // RO is available but we don't use it
      break;

    default:
      zMapWarnIfReached();
      break;
    }

  // point this param at its is_set bit
  param->flag_ind = param->id / 8;
  param->flag_bit = 1 << (param->id % 8);

  param->size = zmapStyleParamSize(param->type);
  param->spec = gps;

  return ;
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

  style_parent_class_G = (GObjectClass*)g_type_class_peek_parent(style_class);

  for (i = 1, param = &zmapStyleParams_G[i] ; i < _STYLE_PROP_N_ITEMS ; i++, param++)
    {
      zmap_param_spec_init(param) ;

      g_object_class_install_property(gobject_class, param->id, param->spec) ;
    }

  gobject_class->dispose = zmap_feature_type_style_dispose ;
  gobject_class->finalize = zmap_feature_type_style_finalize ;


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

  return ;
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



/* OH GOSH....THIS ISN'T THOUGHT THROUGH...IT ALLOWS THE CALLER TO SET
 * NON-SENSICAL STATE IN THE STYLE, e.g. min_score > max_score AND SO ON....
 *
 * bother......WHOEVER DID THIS SHOULD HAVE THOUGHT IT THROUGH MORE.....
 *  */
static void zmap_feature_type_style_set_property_full(ZMapFeatureTypeStyle style,
      ZMapStyleParam param,
      const GValue *value,
      gboolean copy)
{
  gboolean result = TRUE ;

  /* if we are in a copy constructor then only set properties that have been set in the source
   * (except for the is_set array of course, which must be copied first) */

  /* will we ever be in a copy constructor?
   * we use zMapStyleCopy() rather than a g_object(big_complex_value)
   * if someone programmed a GObject copy function what would happen?
   * ... an exercise for the reader
   */
  if (copy && param->id != STYLE_PROP_IS_SET)
    {
      /* if not we are in deep doo doo
       * This relies on STYLE_PROP_IS_SET being the first installed or
       * lowest numbered one and therefore the first one copied
       * if GLib changes such that we get an Assert here then we need to
       * implement a copy constructor that forces this
       */

      /* only copy paramters that have been set in the source
       * as we copied the is_set array we can read our own copy of it */
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

  if (param->mode && style->mode != param->mode)
    {
      zMapLogWarning("Style %s: set style mode specific paramter %s ignored as mode is %s",
     g_quark_to_string(style->original_id),
     param->name, zMapStyleMode2ExactStr(style->mode)) ;

      return;
    }


  /* I'M GUESSING FROM THE COMMENT STYLE THAT THIS IS MALCOLM'S CODE....WHAT'S WRONG
   * WITH THIS IS THAT IT SETS THE STYLE WITHOUT CHECKING THAT THE VALUE IS RATIONAL,
   * SANE, ALLOWED....ETC ETC......
   *  */
  // set the value
  switch(param->type)
    {
    case STYLE_PARAM_TYPE_BOOLEAN:
      * (gboolean *) (((size_t) style) + param->offset) = g_value_get_boolean(value);
      break;

    case STYLE_PARAM_TYPE_UINT:
      * (guint *) (((size_t) style) + param->offset) = g_value_get_uint(value);
      break;

    case STYLE_PARAM_TYPE_DOUBLE:
      * (double *) (((size_t) style) + param->offset)   = g_value_get_double(value);
      break;

    case STYLE_PARAM_TYPE_STRING:              // gchar *
      * (gchar **) (((size_t) style) + param->offset)    = g_strdup( g_value_get_string(value));
      break;

    case STYLE_PARAM_TYPE_QUARK:
      * (GQuark *) (((size_t) style) + param->offset)   =  g_value_get_uint(value);
      break;

    case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
      * (GQuark *) (((size_t) style) + param->offset)   = g_quark_from_string(g_value_get_string(value));
      break;

    case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)
      zmap_hex_to_bin((guchar*)(((size_t) style) + param->offset), (gchar *) g_value_get_string(value), STYLE_IS_SET_SIZE);
      break;

    case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
      result = parseColours(style, param->id, (GValue *) value) ;
      break;

    case STYLE_PARAM_TYPE_SUB_FEATURES:        // GQuark[]
      result = parseSubFeatures(style, (gchar *) g_value_get_string(value) );
      break;

    case STYLE_PARAM_TYPE_QUARK_LIST_ID:
      * (GList **) (((size_t) style) + param->offset)   = zMapConfigString2QuarkGList( (gchar *) g_value_get_string(value) ,TRUE);
      break;

    case STYLE_PARAM_TYPE_GLYPH_SHAPE:          // copy structure into ours
      memcpy((void*)(((size_t) style) + param->offset),g_value_get_boxed(value),sizeof(ZMapStyleGlyphShapeStruct));
      break;

      // enums treated as uint. This is a pain: can we know how big an enum is?
      // Some pretty choice code but it's not safe to do it the easy way
#define STYLE_SET_PROP(s_param, s_type)\
      case s_param : *(s_type *)  (((size_t) style) + param->offset) = (s_type) g_value_get_uint(value); \
      break

      STYLE_SET_PROP (STYLE_PARAM_TYPE_MODE,            ZMapStyleMode);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_COLDISP,         ZMapStyleColumnDisplayState);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_BUMP,            ZMapStyleBumpMode);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_3FRAME,          ZMapStyle3FrameMode);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_SCORE,           ZMapStyleScoreMode);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_GRAPH_MODE,      ZMapStyleGraphMode);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_SCALE,           ZMapStyleScale);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_BLIXEM,          ZMapStyleBlixemType);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_GLYPH_STRAND,    ZMapStyleGlyphStrand);

      STYLE_SET_PROP (STYLE_PARAM_TYPE_GLYPH_ALIGN,     ZMapStyleGlyphAlign);

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
#if CAUSES_NON_INHERITANCE
// make Drawable will set this after inheritance
        p2 = &zmapStyleParams_G[STYLE_PROP_BUMP_DEFAULT];
        if(!(style->is_set[p2->flag_ind] & p2->flag_bit))
          {
            style->default_bump_mode = style->initial_bump_mode;
            style->is_set[p2->flag_ind] |= p2->flag_bit;
          }
#endif
        break;

      case STYLE_PROP_MODE:
// set non zero default vaues for mode data
        if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
          {
            style->mode_data.alignment.unmarked_colinear = ZMAPSTYLE_COLDISPLAY_HIDE;
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
static void zmap_feature_type_style_get_property(GObject *gobject, guint param_id, GValue *value, GParamSpec *pspec)
{
  zMapReturnIfFail(ZMAP_IS_FEATURE_STYLE(gobject));

  ZMapFeatureTypeStyle style;
  gboolean result = TRUE ;
  gchar *colour = NULL;
  gchar *subs = NULL;
  gchar * flags;
  ZMapStyleParam param = &zmapStyleParams_G[param_id];
  gchar *shape = NULL;

  style = ZMAP_FEATURE_STYLE(gobject);


  /* if the property has not been set just return
   * by implication mode dependant data has not been set
   * if not related to the styles mode so we have nothing to do
   * we could test param->mode and style->mode but is_set cover all possibilities
   */
  if(!(style->is_set[param->flag_ind] & param->flag_bit))
    return ;

  if (param->mode && param->mode != style->mode)
    {
      zMapLogWarning("Get style mode specific parameter %s ignored as mode is %s",
                     param->name, zMapStyleMode2ExactStr(style->mode)) ;

      return ;
    }

  // get the value
  switch(param->type)
    {
    case STYLE_PARAM_TYPE_BOOLEAN:
      g_value_set_boolean(value, * (gboolean *) (((size_t) style) + param->offset));
      break;
    case STYLE_PARAM_TYPE_DOUBLE:
      g_value_set_double(value, * (double *) (((size_t) style) + param->offset));
      break;

    case STYLE_PARAM_TYPE_STRING:              // gchar *
      g_value_set_string(value, (gchar *) (((size_t) style) + param->offset));
      break;

    case STYLE_PARAM_TYPE_QUARK:
      g_value_set_uint(value, (guint) (* (GQuark *) (((size_t) style) + param->offset)));
      break;

    case STYLE_PARAM_TYPE_SQUARK:              // gchar * stored as a quark
      g_value_set_string(value, g_quark_to_string(* (GQuark *) (((size_t) style) + param->offset)));
      break;

    case STYLE_PARAM_TYPE_FLAGS:               // bitmap of is_set flags (array of uchar)
      flags = (gchar*)g_malloc(STYLE_IS_SET_SIZE * 2 + 1);

      zmap_bin_to_hex(flags,(guchar*)(((size_t) style) + param->offset), STYLE_IS_SET_SIZE);

      g_value_set_string(value,flags);
      g_free(flags);
      break;

       case STYLE_PARAM_TYPE_COLOUR:              // ZMapStyleFullColourStruct
      colour = zmapStyleValueColour((ZMapStyleFullColour) (((size_t) style) + param->offset));
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
      subs = zmapStyleValueSubFeatures((GQuark *)(((size_t) style) + param->offset));
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
        str = zMap_g_list_quark_to_string(*(GList **)(((size_t) style) + param->offset), NULL);
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
      case s_param : g_value_set_uint(value, (guint) (* (s_type *) (((size_t) style) + param->offset))); \
      break


      STYLE_GET_PROP (STYLE_PARAM_TYPE_MODE            , ZMapStyleMode);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_COLDISP         , ZMapStyleColumnDisplayState);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_BUMP            , ZMapStyleBumpMode);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_3FRAME          , ZMapStyle3FrameMode);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_SCORE           , ZMapStyleScoreMode);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_GRAPH_MODE      , ZMapStyleGraphMode);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_SCALE           , ZMapStyleScale);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_BLIXEM          , ZMapStyleBlixemType);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_GLYPH_STRAND    , ZMapStyleGlyphStrand);
      STYLE_GET_PROP (STYLE_PARAM_TYPE_GLYPH_ALIGN     , ZMapStyleGlyphAlign);

    case STYLE_PARAM_TYPE_UINT:
      g_value_set_uint(value, * (guint *) ((size_t)style + param->offset));
      break;

    default:
      result = FALSE ;
      zMapWarnIfReached() ;
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
                                 zmapStyleSubFeature2ExactStr((ZMapStyleSubFeature)i),
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
  char *str = NULL ;
  char **colour_strings = NULL ;

  if ((str = (char *)g_value_get_string(value))
      && (colour_strings = g_strsplit(str, ";", -1)))
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

              full_colour = zmapStyleFullColour(style, (ZMapStyleParamId)param_id);
              type_colour = zmapStyleColour(full_colour,type);

              switch (context)
                {
                case ZMAPSTYLE_DRAW_FILL:
                  if (!(type_colour->fields_set.fill_col = gdk_color_parse(colour, &(type_colour->fill_col))))
                    result = FALSE ;
                  break ;
                case ZMAPSTYLE_DRAW_DRAW:
                  if (!(type_colour->fields_set.draw_col = gdk_color_parse(colour, &(type_colour->draw_col))))
                    result = FALSE ;
                  break ;
                case ZMAPSTYLE_DRAW_BORDER:
                  if (!(type_colour->fields_set.border_col = gdk_color_parse(colour, &(type_colour->border_col))))
                    result = FALSE ;
                  break ;
                default:
                          result = FALSE ;
                          zMapWarnIfReached() ;
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
          is_set = type_colour->fields_set.fill_col ;
          break ;
        case ZMAPSTYLE_DRAW_DRAW:
          is_set = type_colour->fields_set.draw_col ;
          break ;
        case ZMAPSTYLE_DRAW_BORDER:
          is_set = type_colour->fields_set.border_col ;
          break ;
        default:
          zMapWarnIfReached() ;
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
                  valid = FALSE ;
                  zMapWarnIfReached() ;
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
static void formatColours(GString *colour_string, const char *type, ZMapStyleColour colour)
{
  if (colour->fields_set.fill_col)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                           type, "fill",
                           colour->fill_col.red, colour->fill_col.green, colour->fill_col.blue) ;

  if (colour->fields_set.draw_col)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                           type, "draw",
                           colour->draw_col.red, colour->draw_col.green, colour->draw_col.blue) ;

  if (colour->fields_set.border_col)
    g_string_append_printf(colour_string, "%s %s #%04X%04X%04X ; ",
                           type, "border",
                           colour->border_col.red, colour->border_col.green, colour->border_col.blue) ;

  return ;
}


// G_BOXED data type for glyph styles
// defined here as glyphs include styles not versa vice
// can we do a G_OBJECT_TYPE() on these to check??

static gpointer glyph_shape_copy(gpointer src)
{
  ZMapStyleGlyphShape dest = NULL;

  dest = g_new0(ZMapStyleGlyphShapeStruct,1);
  memcpy((void *) dest,(void *) src,sizeof(ZMapStyleGlyphShapeStruct));

  return(dest);
}

static void glyph_shape_free(gpointer thing)
{
  g_free(thing);
}





