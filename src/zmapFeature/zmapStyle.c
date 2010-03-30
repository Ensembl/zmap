/*  File: zmapStyle.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  and formerly Ed Griffiths (edgrif@sanger.ac.uk)
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
 *    Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Implements the zmapStyle GObject
 *
 * Exported functions: See ZMap/zmapStyle.h
 *
 * CVS info:   $Id: zmapStyle.c,v 1.43 2010-03-30 13:59:46 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapStyle_I.h>


/*
 * Some notes on implementation by mh17
 *
 * There is a write up in ZMap/docs/Design_notes/modules/zmapFeature.shtml
 * (or wherever your source tree is)
 *
 * Styles are GObjects and implement the g_object_get/set interface
 * but internally (esp. in this file) the code used the ZMapFeatureTypeStyleStruct directly
 *
 * We use a mechanical process of accesing data, using an array opf ZMapStyleParamStruct
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
    // missing trailing fields default to zero. this is deliberate (in the interests of brevity)

    { STYLE_PROP_NONE, STYLE_PARAM_TYPE_INVALID, ZMAPSTYLE_PROPERTY_INVALID },      // (0 not used)

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
            "cur-bump_mode", "Current bump mode",
            offsetof(zmapFeatureTypeStyleStruct, curr_bump_mode),0 },
    { STYLE_PROP_BUMP_FIXED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_BUMP_FIXED,
            "bump-fixed", "Style cannot be changed once set.",
            offsetof(zmapFeatureTypeStyleStruct, bump_fixed) ,0},
    { STYLE_PROP_BUMP_SPACING, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_BUMP_SPACING,
            "bump-spacing", "space between columns in bumped columns",
            offsetof(zmapFeatureTypeStyleStruct, bump_spacing) ,0},

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
    { STYLE_PROP_STRAND_SPECIFIC, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC,
            "strand specific", "Strand Specific ?",
            offsetof(zmapFeatureTypeStyleStruct, strand_specific) ,0 },
    { STYLE_PROP_SHOW_REV_STRAND, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND,
            "show-reverse-strand", "Show Rev Strand ?",
            offsetof(zmapFeatureTypeStyleStruct, show_rev_strand) ,0 },
    { STYLE_PROP_SHOW_ONLY_IN_SEPARATOR, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR,
            "only in separator", "Show Only in Separator",
            offsetof(zmapFeatureTypeStyleStruct, show_only_in_separator),0 },
    { STYLE_PROP_DIRECTIONAL_ENDS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS,
            "directional-ends", "Display pointy \"short sides\"",
            offsetof(zmapFeatureTypeStyleStruct, directional_end),0 },
    { STYLE_PROP_DEFERRED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_DEFERRED,
            "deferred", "Load only when specifically asked",
            offsetof(zmapFeatureTypeStyleStruct, deferred) ,0},
    { STYLE_PROP_LOADED, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_LOADED,
            "loaded", "Style Loaded from server",
            offsetof(zmapFeatureTypeStyleStruct, loaded) ,0},


    // mode dependant data

    { STYLE_PROP_GRAPH_MODE, STYLE_PARAM_TYPE_GRAPHMODE, ZMAPSTYLE_PROPERTY_GRAPH_MODE,
            "graph-mode", "Graph Mode",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.mode) ,ZMAPSTYLE_MODE_GRAPH },
    { STYLE_PROP_GRAPH_BASELINE, STYLE_PARAM_TYPE_DOUBLE, ZMAPSTYLE_PROPERTY_GRAPH_BASELINE,
            "graph baseline", "Sets the baseline for graph values.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.graph.baseline),ZMAPSTYLE_MODE_GRAPH },


    { STYLE_PROP_GLYPH_MODE, STYLE_PARAM_TYPE_GLYPHMODE, ZMAPSTYLE_PROPERTY_GLYPH_MODE,
            "glyph-mode", "Glyph Mode",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.mode),ZMAPSTYLE_MODE_GLYPH },
    { STYLE_PROP_GLYPH_TYPE, STYLE_PARAM_TYPE_GLYPHTYPE, ZMAPSTYLE_PROPERTY_GLYPH_TYPE,
             "glyph-type", "Type of glyph to show.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.glyph.type),ZMAPSTYLE_MODE_GLYPH },


    { STYLE_PROP_ALIGNMENT_PARSE_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,
            "parse gaps", "Parse Gaps ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.parse_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_SHOW_GAPS, STYLE_PARAM_TYPE_BOOLEAN, ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,
            "show gaps", "Show Gaps ?",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.show_gaps) ,ZMAPSTYLE_MODE_ALIGNMENT  },
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
            "colinear alignment indicator colour", "Colours used to show two alignments have exactly contiguous coords.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.colinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS,
            "noncolinear alignment indicator colour", "Colours used to show two alignments have exactly contiguous coords.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.noncolinear)  ,ZMAPSTYLE_MODE_ALIGNMENT },
    { STYLE_PROP_ALIGNMENT_INCOMPLETE_GLYPH, STYLE_PARAM_TYPE_GLYPHTYPE, ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH,
            "colinear alignment glyph type", "Type of glyph to show when alignments are incomplete.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.incomplete_glyph_type) ,ZMAPSTYLE_MODE_ALIGNMENT  },
    { STYLE_PROP_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS, STYLE_PARAM_TYPE_COLOUR,ZMAPSTYLE_PROPERTY_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS,
             "colinear alignment glyph colour", "Colours used to show glyphs when alignments are incomplete.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.alignment.incomplete_glyph_colour) ,ZMAPSTYLE_MODE_ALIGNMENT  },


    { STYLE_PROP_TRANSCRIPT_CDS_COLOURS, STYLE_PARAM_TYPE_COLOUR, ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS,
            "CDS colours", "Colour used to CDS section of a transcript.",
            offsetof(zmapFeatureTypeStyleStruct, mode_data.transcript.CDS_colours)  ,ZMAPSTYLE_MODE_TRANSCRIPT},

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


ZMapStyleFullColour zmapStyleFullColour(ZMapFeatureTypeStyle style, ZMapStyleParamId id);
gchar *zmapStyleValueColour(ZMapStyleFullColour this_colour);


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
        sizeof (zmapFeatureTypeStyle),
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
                dststr = (gchar **) (((void *) src) + param->offset);

                *dststr = g_strdup(*srcstr);
                break;
              }
            case STYLE_PARAM_TYPE_COLOUR:
                  // COLOUR included explicitly to flag up that it has internal is_set flags
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
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
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


/* Checks a style to see if it contains the minimum necessary to be drawn,
 * we need this function because we no longer allow defaults because we
 * we want to do inheritance.
 *
 * Function returns FALSE if there style is not valid and the GError says
 * what the problem was.
 *  */
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
      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
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
          if (style->mode_data.glyph.mode < ZMAPSTYLE_GLYPH_SPLICE || style->mode_data.glyph.mode > ZMAPSTYLE_GLYPH_MARKER)
            {
            valid = FALSE ;
            code = 10 ;
            message = g_strdup("Bad glyph mode.") ;
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
      message = g_strdup("Style bump mode not set.") ;
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

      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
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
          if (style->mode_data.glyph.mode == ZMAPSTYLE_GLYPH_SPLICE
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
            else if(style->mode_data.glyph.mode == ZMAPSTYLE_GLYPH_MARKER)
            {
            }
            else
            {
               valid = FALSE ;
               code = 13 ;
            }
          break ;
        }
      default:
        {
          zMapAssertNotReached() ;
        }
      }
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
        {
          break ;
        }
      case ZMAPSTYLE_MODE_TRANSCRIPT:zMapStyleIsDisplayable(
        {
          break ;
        }
      case ZMAPSTYLE_MODE_ALIGNMENT:
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
        }zMapStyleIsDisplayable(
      default:
        {
          zMapAssertNotReached() ;
        }
      }
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
        style->curr_bump_mode = ZMAPBUMP_UNBUMP ;
      }

      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_BUMP_DEFAULT))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_BUMP_DEFAULT);
        style->default_bump_mode = ZMAPBUMP_UNBUMP ;
      }

      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_WIDTH))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_WIDTH);
        style->width = 1.0 ;
      }

      if (!zMapStyleIsPropertySetId(style,STYLE_PROP_FRAME_MODE))
      {
        zmapStyleSetIsSet(style,STYLE_PROP_FRAME_MODE);
        style->frame_mode = ZMAPSTYLE_3_FRAME_NEVER ;
      }


      switch (style->mode)
      {
      case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
      case ZMAPSTYLE_MODE_PEP_SEQUENCE:
      case ZMAPSTYLE_MODE_RAW_SEQUENCE:
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
          if(zMapStyleIsFrameSpecific(style))
            {
            zMapStyleSetColours(style, STYLE_PROP_FRAME0_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                            "red", "red", "red");
            zMapStyleSetColours(style, STYLE_PROP_FRAME1_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                            "blue", "blue", "blue");
            zMapStyleSetColours(style, STYLE_PROP_FRAME2_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                            "green", "green", "green");
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
    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
      full_colour = &(style->mode_data.alignment.perfect) ;
      break;
    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
      full_colour = &(style->mode_data.alignment.colinear) ;
      break;
    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
      full_colour = &(style->mode_data.alignment.noncolinear) ;
      break;
    case STYLE_PROP_ALIGNMENT_INCOMPLETE_GLYPH_COLOURS:
      full_colour = &(style->mode_data.alignment.incomplete_glyph_colour) ;
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



/* I NOW THINK THIS FUNCTION IS REDUNDANT.... mh17: not true it has a new lease of life */
/* As for zMapStyleGetColours() but defaults colours that are not set in the style according
 * to the style mode e.g. rules may be different for Transcript as opposed to Basic mode.
 *
 *  */
gboolean zMapStyleGetColoursDefault(ZMapFeatureTypeStyle style,
                            GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;

  zMapAssert(style) ;

  switch (style->mode)
    {
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_GRAPH:
      {
      /* Our rule is that missing colours will default to the fill colour so if the fill colour
       * is missing there is nothing we can do. */
      if (style->colours.normal.fields_set.fill)
        {
          result = TRUE ;     /* We know we can default to fill colour. */

          if (background)
            {
            *background = &(style->colours.normal.fill) ;
            }

          if (foreground)
            {
            if (style->colours.normal.fields_set.draw)
              *foreground = &(style->colours.normal.draw) ;
            else
              *foreground = &(style->colours.normal.fill) ;
            }

          if (outline)
            {
            if (style->colours.normal.fields_set.border)
              *outline = &(style->colours.normal.border) ;
            else
              *outline = &(style->colours.normal.fill) ;
            }
        }
      }
    default:
      {
      break ;
      }
    }

  return result ;
}



// function not called...
/* This function looks for cds colours but defaults to the styles normal colours if there are none. */
gboolean zMapStyleGetColoursCDSDefault(ZMapFeatureTypeStyle style,
                               GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;
  ZMapStyleColour cds_colours ;

  zMapAssert(style && style->mode == ZMAPSTYLE_MODE_TRANSCRIPT) ;

  cds_colours = &(style->mode_data.transcript.CDS_colours.normal) ;

  /* Our rule is that missing CDS colours will default to the CDS fill colour, if those
   * are missing they default to the standard colours. */
  if (cds_colours->fields_set.fill || cds_colours->fields_set.border)
    {
      result = TRUE ;                         /* We know we can default to fill colour. */

      if (background)
      {
        if (cds_colours->fields_set.fill)
          *background = &(cds_colours->fill) ;
      }

      if (foreground)
      {
        if (cds_colours->fields_set.draw)
          *foreground = &(cds_colours->draw) ;
      }

      if (outline)
      {
        if (cds_colours->fields_set.border)
          *outline = &(cds_colours->border) ;
      }
    }
  else
    {
      GdkColor *fill, *draw, *border ;

      if (zMapStyleGetColoursDefault(style, &fill, &draw, &border))
      {
        result = TRUE ;

        if (background)
          *background = fill ;

        if (foreground)
          *foreground = draw ;

        if (outline)
          *outline = border ;
      }
    }

  return result ;
}


/* This function looks for alignmemnt_incomplete_glyph colours but defaults to the styles normal colours if there are none. */
gboolean zMapStyleGetColoursGlyphDefault(ZMapFeatureTypeStyle style,
                               GdkColor **background, GdkColor **foreground, GdkColor **outline)
{
  gboolean result = FALSE ;
  ZMapStyleColour glyph_colours ;

  zMapAssert(style && style->mode == ZMAPSTYLE_MODE_ALIGNMENT) ;

  glyph_colours = &(style->mode_data.alignment.incomplete_glyph_colour.normal) ;

  /* Our rule is that missing CDS colours will default to the CDS fill colour, if those
   * are missing they default to the standard colours. */
  if (glyph_colours->fields_set.fill || glyph_colours->fields_set.border)
    {
      result = TRUE ;                         /* We know we can default to fill colour. */

      if (background)
      {
        if (glyph_colours->fields_set.fill)
          *background = &(glyph_colours->fill) ;
      }

      if (foreground)
      {
        if (glyph_colours->fields_set.draw)
          *foreground = &(glyph_colours->draw) ;
      }

      if (outline)
      {
        if (glyph_colours->fields_set.border)
          *outline = &(glyph_colours->border) ;
      }
    }
// these default to some odd values!, we prefer to default to previous hard coded values
// currently only called from homology glyphs, in case we used these for free glyphs return false to allow other defaults
#if MH17_DONT_USE
  else
    {
      GdkColor *fill, *draw, *border ;

      if (zMapStyleGetColoursDefault(style, &fill, &draw, &border))
      {
        result = TRUE ;

        if (background)
          *background = fill ;

        if (foreground)
          *foreground = draw ;

        if (outline)
          *outline = border ;
      }
    }
#endif
  return result ;
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

  case STYLE_PARAM_TYPE_FLAGS:     return(STYLE_IS_SET_SIZE); break;     // bitmap of is_set flags (array of uchar)
  case STYLE_PARAM_TYPE_QUARK:     return(sizeof(GQuark));    break;     // strings turned into integers by config file code
  case STYLE_PARAM_TYPE_BOOLEAN:   return(sizeof(gboolean));  break;
  case STYLE_PARAM_TYPE_UINT:      return(sizeof(guint));     break;     // we don't use INT !!
  case STYLE_PARAM_TYPE_DOUBLE:    return(sizeof(double));    break;
  case STYLE_PARAM_TYPE_STRING:    return(sizeof(gchar *));   break;     // but needs a strdup for copy
  case STYLE_PARAM_TYPE_COLOUR:    return(sizeof(ZMapStyleFullColourStruct)); break;      // a real structure
  case STYLE_PARAM_TYPE_SQUARK:    return(sizeof(GQuark));    break;     // gchar * stored as a quark eg name

      // enums: size not defined by ansi C
  case STYLE_PARAM_TYPE_MODE:      return(sizeof(ZMapStyleMode));       break;
  case STYLE_PARAM_TYPE_COLDISP:   return(sizeof(ZMapStyleColumnDisplayState)); break;
  case STYLE_PARAM_TYPE_BUMP:      return(sizeof(ZMapStyleBumpMode));   break;
  case STYLE_PARAM_TYPE_3FRAME:    return(sizeof(ZMapStyle3FrameMode)); break;
  case STYLE_PARAM_TYPE_SCORE:     return(sizeof(ZMapStyleScoreMode));  break;
  case STYLE_PARAM_TYPE_GRAPHMODE: return(sizeof(ZMapStyleGraphMode));  break;
  case STYLE_PARAM_TYPE_GLYPHMODE: return(sizeof(ZMapStyleGlyphMode));  break;
  case STYLE_PARAM_TYPE_GLYPHTYPE: return(sizeof(ZMapStyleGlyphType));  break;
  case STYLE_PARAM_TYPE_BLIXEM:    return(sizeof(ZMapStyleBlixemType)); break;

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

      gps = g_param_spec_string(param->name, param->nick,param->blurb,
                  "", ZMAP_PARAM_STATIC_RW);
      break;

    // enums treated as uint, but beware: ANSI C leave the size unspecifies
    // so it's compiler dependant and could be changed via optimisation flags

    case STYLE_PARAM_TYPE_MODE:                // ZMapStyleMode
    case STYLE_PARAM_TYPE_COLDISP:             // ZMapStyleColumnDisplayState
    case STYLE_PARAM_TYPE_BUMP:                // ZMapStyleBumpMode
    case STYLE_PARAM_TYPE_3FRAME:              // ZMapStyle3FrameMode
    case STYLE_PARAM_TYPE_SCORE:               // ZMapStyleScoreMode
    case STYLE_PARAM_TYPE_GRAPHMODE:           // ZMapStyleGraphMode
    case STYLE_PARAM_TYPE_GLYPHMODE:           // ZMapStyleGlyphMode
    case STYLE_PARAM_TYPE_GLYPHTYPE:           // ZMapStyleGlyphType
    case STYLE_PARAM_TYPE_BLIXEM:              // ZMapStyleBlixemType
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

#if 0
static void zmap_feature_type_style_copy_set_property(GObject *gobject,
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
#endif

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

      // enums treated as uint. This is a pain: can we know how big an enum is?
      // Some pretty choice code but it's not safe to do it the easy way
    case STYLE_PARAM_TYPE_MODE:                // ZMapStyleMode
      * (ZMapStyleMode *) (((void *) style) + param->offset)       = (ZMapStyleMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_COLDISP:             // ZMapStyleColumnDisplayState
      * (ZMapStyleColumnDisplayState *) (((void *) style) + param->offset) = (ZMapStyleColumnDisplayState) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_BUMP:                // ZMapStyleBumpMode
      * (ZMapStyleBumpMode *) (((void *) style) + param->offset)   = (ZMapStyleBumpMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_3FRAME:              // ZMapStyle3FrameMode
      * (ZMapStyle3FrameMode *) (((void *) style) + param->offset) = (ZMapStyle3FrameMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_SCORE:               // ZMapStyleScoreMode
      * (ZMapStyleScoreMode *) (((void *) style) + param->offset)  = (ZMapStyleScoreMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_GRAPHMODE:           // ZMapStyleGraphMode
      * (ZMapStyleGraphMode *) (((void *) style) + param->offset)  = (ZMapStyleGraphMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_GLYPHMODE:           // ZMapStyleG/* Parse out colour triplets from a colour keyword-value line.
      * (ZMapStyleGlyphMode *) (((void *) style) + param->offset)  = (ZMapStyleGlyphMode) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_GLYPHTYPE:           // ZMapStyleGlyphType
      * (ZMapStyleGlyphType *) (((void *) style) + param->offset)  = (ZMapStyleGlyphType) g_value_get_uint(value);
      break;
    case STYLE_PARAM_TYPE_BLIXEM:              // ZMapStyleBlixemType
      * (ZMapStyleBlixemType *) (((void *) style) + param->offset) = (ZMapStyleBlixemType) g_value_get_uint(value);
      break;

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
#if 0
      case STYLE_PROP_BUMP_MODE:
        p2 = &zmapStyleParams_G[STYLE_PROP_BUMP_DEFAULT];
        if(!(style->is_set[p2->flag_ind] & p2->flag_bit))
          {
            style->default_bump_mode = style->curr_bump_mode;
            style->is_set[p2->flag_ind] |= p2->flag_bit;
          }

        break;
#endif
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
  gchar * flags;
  ZMapStyleParam param = &zmapStyleParams_G[param_id];

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
      zMapLogWarning("Get style mode specific paramter %s ignored as mode is %s",param->name,zMapStyleMode2ExactStr(style->mode));
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
           g_value_set_string(value, strdup(colour));
           g_free(colour);
        }
      else
        {
           result = FALSE;
        }
      break;

      // enums treated as uint. This is a pain: can we know how big an enum is?
      // Some pretty choice code but it's not safe to do it the easy way
    case STYLE_PARAM_TYPE_MODE:                // ZMapStyleMode
      g_value_set_uint(value, (guint) (* (ZMapStyleMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_COLDISP:             // ZMapStyleColumnDisplayState
      g_value_set_uint(value, (guint) (* (ZMapStyleColumnDisplayState *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_BUMP:                // ZMapStyleBumpMode
      g_value_set_uint(value, (guint) (* (ZMapStyleBumpMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_3FRAME:              // ZMapStyle3FrameMode
      g_value_set_uint(value, (guint) (* (ZMapStyle3FrameMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_SCORE:               // ZMapStyleScoreMode
      g_value_set_uint(value, (guint) (* (ZMapStyleScoreMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_GRAPHMODE:           // ZMapStyleGraphMode
      g_value_set_uint(value, (guint) (* (ZMapStyleGraphMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_GLYPHMODE:           // ZMapStyleGlyphMode
      g_value_set_uint(value, (guint) (* (ZMapStyleGlyphMode *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_GLYPHTYPE:           // ZMapStyleGlyphType
      g_value_set_uint(value, (guint) (* (ZMapStyleGlyphType *) (((void *) style) + param->offset)));
      break;
    case STYLE_PARAM_TYPE_BLIXEM:             // ZMapStyleBlixemType
      g_value_set_uint(value, (guint) (* (ZMapStyleBlixemType *) (((void *) style) + param->offset)));
      break;

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


