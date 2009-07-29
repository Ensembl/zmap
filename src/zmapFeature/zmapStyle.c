/*  File: zmapStyle.c
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
 * Description: Contains functions for handling styles and sets of
 *              styles.
 *
 * Exported functions: See ZMap/zmapStyle.h
 * HISTORY:
 * Last edited: Jul 29 09:27 2009 (edgrif)
 * Created: Mon Feb 26 09:12:18 2007 (edgrif)
 * CVS info:   $Id: zmapStyle.c,v 1.35 2009-07-29 12:47:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapStyle_P.h>


/* 
 * PLEASE READ THIS......
 * 
 * The g_object/paramspec stuff all needs redoing so that it's table driven
 * otherwise we will all go mad.....it is way too error prone......
 * 
 * but it needs to be done carefully, we need the "set" flags to support
 * inheritance....and they g_object copy/set/get code in glib all expects
 * paramspecs etc so it's not straight forward...
 * 
 */




enum
  {
    STYLE_PROP_NONE,					    /* zero is invalid */

    STYLE_PROP_NAME,					    /* Name as text. */
    STYLE_PROP_ORIGINAL_ID,				    /* Name as a GQuark. */
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
    STYLE_PROP_STRAND_SPECIFIC,
    STYLE_PROP_SHOW_REV_STRAND,
    STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
    STYLE_PROP_DIRECTIONAL_ENDS,
    STYLE_PROP_DEFERRED,
    STYLE_PROP_LOADED,

    STYLE_PROP_GRAPH_MODE,
    STYLE_PROP_GRAPH_BASELINE,

    STYLE_PROP_GLYPH_MODE,

    STYLE_PROP_ALIGNMENT_PARSE_GAPS,
    STYLE_PROP_ALIGNMENT_SHOW_GAPS,
    STYLE_PROP_ALIGNMENT_BETWEEN_ERROR,
    STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN,
    STYLE_PROP_ALIGNMENT_PFETCHABLE,
    STYLE_PROP_ALIGNMENT_BLIXEM,
    STYLE_PROP_ALIGNMENT_PERFECT_COLOURS,
    STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS,
    STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS,

    STYLE_PROP_TRANSCRIPT_CDS_COLOURS,

    STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS,

    STYLE_PROP_TEXT_FONT,
  };




#define SETMODEFIELD(STYLE, COPY_STYLE, VALUE, FIELD_TYPE, FIELD_SET, FIELD, RESULT) \
  if (!COPY_STYLE || (COPY_STYLE)->FIELD_SET)		                             \
    {                                                                                \
      if (((STYLE)->FIELD = g_value_get_uint((VALUE))))			             \
	(RESULT) = (STYLE)->FIELD_SET = TRUE ;				             \
      else                                                                           \
	(RESULT) = (STYLE)->FIELD_SET = FALSE ;   				     \
    }

#define SETBOOLFIELD(STYLE, COPY_STYLE, VALUE, FIELD_SET, FIELD) \
  if (!COPY_STYLE || (COPY_STYLE)->FIELD_SET)	                 \
    {                                                            \
      (STYLE)->FIELD = g_value_get_boolean(value) ;		 \
      (STYLE)->FIELD_SET = TRUE ;				 \
    }



static void set_implied_mode(ZMapFeatureTypeStyle style, guint param_id);

static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class);
static void zmap_feature_type_style_init      (ZMapFeatureTypeStyle      style);
static void zmap_feature_type_style_set_property(GObject *gobject, 
						 guint param_id, 
						 const GValue *value, 
						 GParamSpec *pspec);
static void zmap_feature_type_style_get_property(GObject *gobject, 
						 guint param_id, 
						 GValue *value, 
						 GParamSpec *pspec);
static void zmap_feature_type_style_dispose (GObject *object);
static void zmap_feature_type_style_finalize(GObject *object);

static ZMapFeatureTypeStyle styleCreate(guint n_parameters, GParameter *parameters) ;

static gboolean setColours(ZMapStyleColour colour, char *border, char *draw, char *fill) ;
static gboolean parseColours(ZMapFeatureTypeStyle style, ZMapFeatureTypeStyle copy_style, guint param_id, GValue *value) ;
static gboolean isColourSet(ZMapFeatureTypeStyle style, int param_id, char *subpart) ;
static gboolean validSplit(char **strings,
			   ZMapStyleColourType *type_out, ZMapStyleDrawContext *context_out, char **colour_out) ;
static void formatColours(GString *colour_string, char *type, ZMapStyleColour colour) ;

static void badPropertyForMode(ZMapFeatureTypeStyle style, ZMapStyleMode required_mode, char *op, GParamSpec *pspec) ;


/* Class pointer for styles. */
static ZMapBaseClass style_parent_class_G = NULL ;





/*! @addtogroup zmapstyles
 * @{
 *  */


/*!
 * THIS STYLES STUFF NEEDS A REWRITE TO CHANGE THE STYLES STRUCT TO BE A LIST
 * OF PROPERTIES SO THAT WE CAN DO AWAY WITH THE MANY SWITCH STATEMENTS IN
 * THE CODE...IT'S ERROR PRONE AND TEDIOUS....
 *  */



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
      
      type = g_type_register_static (ZMAP_TYPE_BASE, 
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



ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle src)
{
  ZMapFeatureTypeStyle dest = NULL ;

  zMapStyleCCopy(src, &dest) ;

#ifdef RDS
  ZMapFeatureTypeStyle dest = NULL;
  ZMapBase dest_base;

  if((dest_base = zMapBaseCopy(ZMAP_BASE(src))))
    dest = ZMAP_FEATURE_STYLE(dest_base);
#endif

  return dest ;
}


gboolean zMapStyleCCopy(ZMapFeatureTypeStyle src, ZMapFeatureTypeStyle *dest_out)
{
  gboolean copied = FALSE ;
  ZMapBase dest ;

  g_object_set_data(G_OBJECT(src), ZMAPSTYLE_OBJ_COPY, GINT_TO_POINTER(TRUE)) ;

  if (dest_out && (copied = zMapBaseCCopy(ZMAP_BASE(src), &dest)))
    *dest_out = ZMAP_FEATURE_STYLE(dest);

  (*dest_out)->mode_data = src->mode_data;

  return copied ;
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





/* I see no easy way to do this with the "get" methodology of g_object_get(). Nor
 * do I see a good way to make use of g_object stuff to help....
 * 
 * NOTE that property_name for all properties colours is simply the property
 * name, e.g. "min_mag". 
 *
 * Colours have subparts however so the subpart must be specified in the form:
 * 
 * "<type> <context>", e.g. "normal draw"
 *
 * and then the function returns TRUE/FALSE to show if that particular colour is set.
 * 
 *  */
gboolean zMapStyleIsPropertySet(ZMapFeatureTypeStyle style, char *property_name, char *property_subpart)
{
  gboolean is_set = FALSE ;
  GParamSpec *pspec ;

  if (!(pspec = g_object_class_find_property(G_OBJECT_CLASS(ZMAP_FEATURE_STYLE_GET_CLASS(style)), property_name)))
    {
      zMapLogCritical("Style \"%s\", unknown property \"%s\"", zMapStyleGetName(style), property_name) ;
    }
  else
    {
      switch(pspec->param_id)
	{
	case STYLE_PROP_NAME:
	case STYLE_PROP_ORIGINAL_ID:
	case STYLE_PROP_UNIQUE_ID:
	  {
	    is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_COLUMN_DISPLAY_MODE:
	  {
	    if (style->fields_set.col_display_state)
	      is_set = TRUE ;
	    break;
	  }

	case STYLE_PROP_COLOURS:
	case STYLE_PROP_FRAME0_COLOURS:
	case STYLE_PROP_FRAME1_COLOURS:
	case STYLE_PROP_FRAME2_COLOURS:
	case STYLE_PROP_REV_COLOURS:
	  {
	    is_set = isColourSet(style, pspec->param_id, property_subpart) ;

	    break;
	  }
	case STYLE_PROP_PARENT_STYLE:
	  {
	    if (style->fields_set.parent_id)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_DESCRIPTION:
	  {
	    if (style->fields_set.description)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_MODE:
	  {
	    if (style->fields_set.mode)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_MIN_SCORE:
	  {
	    if (style->fields_set.min_score)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_MAX_SCORE:
	  {
	    if (style->fields_set.max_score)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_BUMP_MODE:
	  {
	    if (style->fields_set.curr_bump_mode)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_BUMP_DEFAULT:
	  {
	    if (style->fields_set.default_bump_mode)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_BUMP_FIXED:
	  {
	    if (style->fields_set.bump_fixed)
	      is_set = TRUE ;
	    break; 
	  }
	case STYLE_PROP_BUMP_SPACING:
	  {
	    if (style->fields_set.bump_spacing)
	      is_set = TRUE ;
	    break; 
	  }
	case STYLE_PROP_FRAME_MODE:
	  {
	    if (style->fields_set.frame_mode)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_MIN_MAG:
	  {
	    if (style->fields_set.min_mag)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_MAX_MAG:
	  {
	    if (style->fields_set.max_mag)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_WIDTH:
	  {
	    if (style->fields_set.width)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_SCORE_MODE:
	  {
	    if (style->fields_set.score_mode)
	      is_set = TRUE ;
	    break; 
	  }
	case STYLE_PROP_GFF_SOURCE:
	  {
	    if (style->fields_set.gff_source)	
	      is_set = TRUE ;

	    break;
	  }

	case STYLE_PROP_GFF_FEATURE:
	  {
	    if (style->fields_set.gff_feature)
	      is_set = TRUE ;

	    break;
	  }
	case STYLE_PROP_DISPLAYABLE:
	  {
	    if (style->fields_set.displayable)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_SHOW_WHEN_EMPTY:
	  {
	    if (style->fields_set.show_when_empty)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_SHOW_TEXT:
	  {
	    if (style->fields_set.showText)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_STRAND_SPECIFIC:
	  {
	    if (style->fields_set.strand_specific)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_SHOW_REV_STRAND:
	  {
	    if (style->fields_set.show_rev_strand)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_SHOW_ONLY_IN_SEPARATOR:
	  {
	    if (style->fields_set.show_only_in_separator)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_DIRECTIONAL_ENDS:
	  {
	    if (style->fields_set.directional_end)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_DEFERRED:
	  {
	    if (style->fields_set.deferred)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_LOADED:
	  {
	    if (style->fields_set.loaded)
	      is_set = TRUE ;
	    break;
	  }

	case STYLE_PROP_GRAPH_MODE:
	  {
	    if (style->mode_data.graph.fields_set.mode)
	      is_set = TRUE ;
	    break;
	  }

	case STYLE_PROP_GRAPH_BASELINE:
	  {
	    if (style->mode_data.graph.fields_set.baseline)
	      is_set = TRUE ;
	    break;
	  }

	case STYLE_PROP_GLYPH_MODE:
	  {
	    if (style->mode_data.glyph.fields_set.mode)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
	  {
	    if (style->mode_data.alignment.fields_set.parse_gaps)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
	  {
	    if (style->mode_data.alignment.fields_set.show_gaps)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_PFETCHABLE:
	  {
	    if (style->mode_data.alignment.fields_set.pfetchable)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_BLIXEM:
	  {
	    if (style->mode_data.alignment.fields_set.blixem)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
	  {
	    if (style->mode_data.alignment.fields_set.between_align_error)
	      is_set = TRUE ;
	    break;
	  }
	case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
	  {
	    if (style->mode_data.alignment.fields_set.allow_misalign)
	      is_set = TRUE ;
	    break;
	  }

	case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
	case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
	case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
	  {
	    is_set = isColourSet(style, pspec->param_id, property_subpart) ;

	    break;
	  }
	case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
	  {
	    is_set = isColourSet(style, pspec->param_id, property_subpart) ;

	    break;
	  }
	case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
	  {
	    is_set = isColourSet(style, pspec->param_id, property_subpart) ;

	    break;
	  }
	default:
	  {
	    G_OBJECT_WARN_INVALID_PROPERTY_ID(style, pspec->param_id, pspec);
	    break;
	  }
	}
    }

  return is_set ;
}


GQuark zMapStyleGetID(ZMapFeatureTypeStyle style)
{
  GQuark original_id ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_ORIGINAL_ID, &original_id,
	       NULL) ;

  return original_id ;
}

GQuark zMapStyleGetUniqueID(ZMapFeatureTypeStyle style)
{
  GQuark unique_id ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_UNIQUE_ID, &unique_id,
	       NULL) ;

  return unique_id ;
}

char *zMapStyleGetName(ZMapFeatureTypeStyle style)
{
  char *name ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_NAME, &name,
	       NULL) ;

  return name ;
}

char *zMapStyleGetDescription(ZMapFeatureTypeStyle style)
{
  char *description = NULL ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DESCRIPTION, &description,
	       NULL) ;

  return description ;
}

ZMapStyleBumpMode zMapStyleGetBumpMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleBumpMode mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_BUMP_MODE, &mode,
	       NULL) ;

  return mode;
}

ZMapStyleBumpMode zMapStyleGetDefaultBumpMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleBumpMode mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, &mode,
	       NULL) ;

  return mode;
}


void zMapStyleDestroy(ZMapFeatureTypeStyle style)
{
  g_object_unref(G_OBJECT(style));

  return ;
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
  ZMapStyleMode mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MODE, &mode,
	       NULL) ;

  switch (mode)
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
  if (valid && (style->original_id == ZMAPFEATURE_NULLQUARK || style->unique_id == ZMAPFEATURE_NULLQUARK))
    {
      valid = FALSE ;
      code = 1 ;
      message = g_strdup_printf("Bad style ids, original: %d, unique: %d.", 
				style->original_id, style->unique_id) ;
    }

  if (valid && !(style->fields_set.mode))
    {
      valid = FALSE ;
      code = 2 ;
      message = g_strdup("Style mode not set.") ;
    }

  if (valid && !(style->fields_set.col_display_state))
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
	    if (style->mode_data.glyph.mode != ZMAPSTYLE_GLYPH_SPLICE)
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

  if (valid && !(style->fields_set.curr_bump_mode))
    {
      valid = FALSE ;
      code = 3 ;
      message = g_strdup("Style bump mode not set.") ;
    }

  if (valid && !(style->fields_set.width))
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
	case ZMAPSTYLE_MODE_TRANSCRIPT:
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
	  }
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

  if (zMapStyleIsDisplayable(style))
    {
      /* These are the absolute basic requirements without which we can't display something.... */
      if (!(style->fields_set.mode))
	{
	  style->fields_set.mode = TRUE ;
	  style->mode = ZMAPSTYLE_MODE_BASIC ;
	}

      if (!(style->fields_set.col_display_state))
	{
	  style->fields_set.col_display_state = TRUE ;
	  style->col_display_state = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE ;
	}

      if (!(style->fields_set.curr_bump_mode))
	{
	  style->fields_set.curr_bump_mode = style->fields_set.default_bump_mode = TRUE ;
	  style->curr_bump_mode = style->default_bump_mode = ZMAPBUMP_UNBUMP ;
	}

      if (!(style->fields_set.default_bump_mode))
	{
	  style->fields_set.default_bump_mode = TRUE ;
	  style->default_bump_mode = ZMAPBUMP_UNBUMP ;
	}

      if (!(style->fields_set.width))
	{
	  style->fields_set.width = TRUE ;
	  style->width = 1.0 ;
	}

      if (!(style->fields_set.frame_mode))
	{
	  style->fields_set.frame_mode = TRUE ;
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
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "white", NULL, NULL) ;

	    if (!(style->colours.normal.fields_set.draw))
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  NULL, "black", NULL) ;

	    break ;
	  }
	case ZMAPSTYLE_MODE_GLYPH:
	  {
	    if(zMapStyleIsFrameSpecific(style))
	      {
		zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME0, ZMAPSTYLE_COLOURTYPE_NORMAL,
				    "red", "red", "red");
		zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME1, ZMAPSTYLE_COLOURTYPE_NORMAL,
				    "blue", "blue", "blue");
		zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME2, ZMAPSTYLE_COLOURTYPE_NORMAL,
				    "green", "green", "green");
	      }
	  }
	  break;
	default:
	  {
	    if (!(style->colours.normal.fields_set.fill) && !(style->colours.normal.fields_set.border))
	      {
		zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				    NULL, NULL, "black") ;
	      }

	    break ;
	  }
	}

      result = TRUE ;
    }

  return result ;
}




void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode)
{
  g_object_set(G_OBJECT(style),
	       ZMAPSTYLE_PROPERTY_MODE, mode,
	       NULL);

  return ;
}


gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style)
{
  gboolean result ;

  result = zMapStyleIsPropertySet(style, ZMAPSTYLE_PROPERTY_MODE, NULL) ;

  return result ;
}


ZMapStyleMode zMapStyleGetMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleMode mode ;
  
  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MODE, &mode,
	       NULL) ;

  return mode;
}



void zMapStyleSetGlyphMode(ZMapFeatureTypeStyle style, ZMapStyleGlyphMode glyph_mode)
{
  zMapAssert(style) ;

  switch (glyph_mode)
    {
    case ZMAPSTYLE_GLYPH_SPLICE:
      style->mode_data.glyph.mode = glyph_mode ;
      style->mode_data.glyph.fields_set.mode = TRUE ;

      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  return ;
}

ZMapStyleGlyphMode zMapStyleGetGlyphMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleGlyphMode glyph_mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_GLYPH_MODE, &glyph_mode,
	       NULL) ;

  return glyph_mode ;
}



void zMapStyleSetPfetch(ZMapFeatureTypeStyle style, gboolean pfetchable)
{
  zMapAssert(style) ;

  style->mode_data.alignment.state.pfetchable = pfetchable ;

  return ;
}





/* I'm going to try these more generalised functions.... */

gboolean zMapStyleSetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     char *fill, char *draw, char *border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style) ;

  switch (target)
    {
    case ZMAPSTYLE_COLOURTARGET_NORMAL:
      full_colour = &(style->colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME0:
      full_colour = &(style->frame0_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME1:
      full_colour = &(style->frame1_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME2:
      full_colour = &(style->frame2_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_CDS:
      full_colour = &(style->mode_data.transcript.CDS_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_NON_ASSEMBLY_PATH:
      full_colour = &(style->mode_data.assembly_path.non_path_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_STRAND:
      full_colour = &(style->strand_rev_colours);
      break;
    default:
      zMapAssertNotReached() ;
      break ;
    }

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

  return result ;
}

gboolean zMapStyleGetColours(ZMapFeatureTypeStyle style, ZMapStyleColourTarget target, ZMapStyleColourType type,
			     GdkColor **fill, GdkColor **draw, GdkColor **border)
{
  gboolean result = FALSE ;
  ZMapStyleFullColour full_colour = NULL ;
  ZMapStyleColour colour = NULL ;

  zMapAssert(style && (fill || draw || border)) ;

  switch (target)
    {
    case ZMAPSTYLE_COLOURTARGET_NORMAL:
      full_colour = &(style->colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME0:
      full_colour = &(style->frame0_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME1:
      full_colour = &(style->frame1_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_FRAME2:
      full_colour = &(style->frame2_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_CDS:
      full_colour = &(style->mode_data.transcript.CDS_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_NON_ASSEMBLY_PATH:
      full_colour = &(style->mode_data.assembly_path.non_path_colours) ;
      break ;
    case ZMAPSTYLE_COLOURTARGET_STRAND:
      full_colour = &(style->strand_rev_colours) ;
      break ;
    default:
      zMapAssertNotReached() ;
      break ;
    }

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




/* I NOW THINK THIS FUNCTION IS REDUNDANT.... */
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
    case ZMAPSTYLE_MODE_GRAPH:
      {
	/* Our rule is that missing colours will default to the fill colour so if the fill colour
	 * is missing there is nothing we can do. */
	if (style->colours.normal.fields_set.fill)
	  {
	    result = TRUE ;				    /* We know we can default to fill
							       colour. */

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
      result = TRUE ;				    /* We know we can default to fill colour. */

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



char *zMapStyleGetGFFSource(ZMapFeatureTypeStyle style)
{
  char *gff_source ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_GFF_SOURCE, &gff_source,
	       NULL) ;

  return gff_source ;
}

char *zMapStyleGetGFFFeature(ZMapFeatureTypeStyle style)
{
  char *gff_feature = NULL ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_GFF_SOURCE, &gff_feature,
	       NULL) ;

  return gff_feature ;
}


gboolean zMapStyleIsDirectionalEnd(ZMapFeatureTypeStyle style)
{
   gboolean ends ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS, &ends,
	       NULL) ;

  return ends ;
}


unsigned int zmapStyleGetWithinAlignError(ZMapFeatureTypeStyle style)
{
  unsigned int error ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN, &error,
	       NULL) ;

  return error ;
}


gboolean zMapStyleIsParseGaps(ZMapFeatureTypeStyle style)
{
  gboolean parse_gaps ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS, &parse_gaps,
	       NULL) ;

  return parse_gaps ;
}

gboolean zMapStyleIsShowGaps(ZMapFeatureTypeStyle style)
{
  gboolean show_gaps ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS, &show_gaps,
	       NULL) ;

  return show_gaps ;
}

void zMapStyleSetShowGaps(ZMapFeatureTypeStyle style, gboolean show_gaps)
{
  zMapAssert(style) ;

  g_object_set(G_OBJECT(style),
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS, show_gaps,
	       NULL);

  return ;
}




void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean parse_gaps, gboolean show_gaps)
{
  zMapAssert(style);

  g_object_set(G_OBJECT(style),
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS,   parse_gaps,
	       ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS,   show_gaps,
	       NULL);

  return ;
}


void zMapStyleGetGappedAligns(ZMapFeatureTypeStyle style, gboolean *parse_gaps, gboolean *show_gaps)
{
  zMapAssert(style) ;

  if (style->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
      style->mode_data.alignment.fields_set.parse_gaps)
    {
      g_object_get(style,
		   ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS, parse_gaps,
		   ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS, show_gaps,
		   NULL) ;
    }

  return ;
}



void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, unsigned int between_align_error)
{
  zMapAssert(style);
  
  if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      style->mode_data.alignment.between_align_error = between_align_error ;
      style->mode_data.alignment.fields_set.between_align_error = TRUE ;
    }

  return ;
}


/* Returns TRUE and returns the between_align_error if join_aligns is TRUE for the style,
 * otherwise returns FALSE. */
gboolean zMapStyleGetJoinAligns(ZMapFeatureTypeStyle style, unsigned int *between_align_error)
{
  gboolean result = FALSE ;

  zMapAssert(style);

  if (style->mode == ZMAPSTYLE_MODE_ALIGNMENT &&
      style->mode_data.alignment.fields_set.between_align_error)
    {
      g_object_get(style,
		   ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN, &between_align_error,
		   NULL) ;

      result = TRUE ;
    }

  return result ;
}


void zMapStyleSetDisplayable(ZMapFeatureTypeStyle style, gboolean displayable)
{
  zMapAssert(style) ;

  style->opts.displayable = displayable ;

  return ;
}

gboolean zMapStyleIsDisplayable(ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DISPLAYABLE, &result,
	       NULL) ;

  return result ;
}


void zMapStyleSetDeferred(ZMapFeatureTypeStyle style, gboolean deferred)
{
  g_object_set(G_OBJECT(style),
	       ZMAPSTYLE_PROPERTY_DEFERRED, deferred,
	       NULL);
  return ;
}

gboolean zMapStyleIsDeferred(ZMapFeatureTypeStyle style)
{
  gboolean result ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DEFERRED, &result,
	       NULL) ;

  return result ;
}

void zMapStyleSetLoaded(ZMapFeatureTypeStyle style, gboolean loaded)
{
  g_object_set(G_OBJECT(style),
	       ZMAPSTYLE_PROPERTY_LOADED, loaded,
	       NULL);

  return ;
}

gboolean zMapStyleIsLoaded(ZMapFeatureTypeStyle style)
{
  gboolean result ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_LOADED, &result,
	       NULL) ;

  return result ;
}


/* Controls whether the feature set is displayed. */
void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show)
{
  zMapAssert(style
	     && col_show > ZMAPSTYLE_COLDISPLAY_INVALID && col_show <= ZMAPSTYLE_COLDISPLAY_SHOW) ;

  style->fields_set.col_display_state = TRUE ;
  style->col_display_state = col_show ;

  return ;
}

ZMapStyleColumnDisplayState zMapStyleGetDisplay(ZMapFeatureTypeStyle style)
{
  ZMapStyleColumnDisplayState mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &mode,
	       NULL) ;

  return mode ;
}


gboolean zMapStyleIsHidden(ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;
  ZMapStyleColumnDisplayState mode ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &mode,
	       NULL) ;

  if (mode == ZMAPSTYLE_COLDISPLAY_HIDE)
    result = TRUE ;

  return result ;
}


/* Controls whether the feature set is displayed initially. */
void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty)
{
  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(style));

  style->opts.show_when_empty = show_when_empty ;

  return ;
}

gboolean zMapStyleGetShowWhenEmpty(ZMapFeatureTypeStyle style)
{
  gboolean show ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, &show,
	       NULL) ;

  return show ;
}



double zMapStyleGetWidth(ZMapFeatureTypeStyle style)
{
  double width = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_WIDTH, &width,
	       NULL) ;

  return width ;
}

double zMapStyleGetMaxScore(ZMapFeatureTypeStyle style)
{
  double max_score = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MAX_SCORE, &max_score,
	       NULL) ;

  return max_score ;
}

double zMapStyleGetMinScore(ZMapFeatureTypeStyle style)
{
  double min_score = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MIN_SCORE, &min_score,
	       NULL) ;

  return min_score ;
}


double zMapStyleGetMinMag(ZMapFeatureTypeStyle style)
{
  double min_mag = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MIN_MAG, &min_mag,
	       NULL) ;

  return min_mag ;
}


double zMapStyleGetMaxMag(ZMapFeatureTypeStyle style)
{
  double max_mag = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_MAX_MAG, &max_mag,
	       NULL) ;

  return max_mag ;
}


double zMapStyleBaseline(ZMapFeatureTypeStyle style)
{
  double baseline = 0.0 ;

  g_object_get(style,
	       ZMAPSTYLE_PROPERTY_GRAPH_BASELINE, &baseline,
	       NULL) ;

  return baseline ;
}




/*! @} end of zmapstyles docs. */





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
	  if (!(style->fields_set.displayable))
	    g_object_set(G_OBJECT(style),
			 ZMAPSTYLE_PROPERTY_DISPLAYABLE, TRUE,
			 NULL) ;
	}
    }

  return style ;
}

/* We need this function to "guess" the mode for the correct acces of
 * the mode_data union in the get/set_property functions... */

/* Without this, the copy of styles runs through each of the
 * properties doing a get_property. While doing this the 
 * code would access style.mode_data.text.font regardless
 * of the style->mode, which could actually be an alignment
 * and mean that style.mode_data.text.font was completely 
 * invalid as we've filled style.mode_data.alignment.parse_gaps
 * etc... */
static void set_implied_mode(ZMapFeatureTypeStyle style, guint param_id)
{
  switch(param_id)
    {
    case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
    case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
    case STYLE_PROP_ALIGNMENT_PFETCHABLE:
    case STYLE_PROP_ALIGNMENT_BLIXEM:
    case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
    case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
      if(style->implied_mode == ZMAPSTYLE_MODE_INVALID)
	style->implied_mode = ZMAPSTYLE_MODE_ALIGNMENT;
      break;
    case STYLE_PROP_GRAPH_MODE:
    case STYLE_PROP_GRAPH_BASELINE:
      if(style->implied_mode == ZMAPSTYLE_MODE_INVALID)
	style->implied_mode = ZMAPSTYLE_MODE_GRAPH;
      break;
    case STYLE_PROP_GLYPH_MODE:
      if(style->implied_mode == ZMAPSTYLE_MODE_INVALID)
	style->implied_mode = ZMAPSTYLE_MODE_GLYPH;
      break;
    case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
      if(style->implied_mode == ZMAPSTYLE_MODE_INVALID)
	style->implied_mode = ZMAPSTYLE_MODE_TRANSCRIPT;
      break;
    case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
      if(style->implied_mode == ZMAPSTYLE_MODE_INVALID)
	style->implied_mode = ZMAPSTYLE_MODE_ASSEMBLY_PATH;
      break;
    default:
      break;
    }

  return ;
}

/* When adding new properties be sure to have only "[a-z]-" in the name and make sure
 * to add set/get property entries in those functions.
 *  */
static void zmap_feature_type_style_class_init(ZMapFeatureTypeStyleClass style_class)
{
  GObjectClass *gobject_class;
  ZMapBaseClass zmap_class;
  
  gobject_class = (GObjectClass *)style_class;
  zmap_class    = ZMAP_BASE_CLASS(style_class);

  gobject_class->set_property = zmap_feature_type_style_set_property;
  gobject_class->get_property = zmap_feature_type_style_get_property;

  zmap_class->copy_set_property = zmap_feature_type_style_set_property;

  style_parent_class_G = g_type_class_peek_parent(style_class);


  /* Style name/id. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_NAME,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_NAME, "name",
						      "Name", "",
						      ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ORIGINAL_ID,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_ORIGINAL_ID, "original-id",
						    "original id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RO));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_UNIQUE_ID,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_UNIQUE_ID, "unique-id",
						    "unique id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RO));


  /* Parent id */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_PARENT_STYLE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_PARENT_STYLE, "parent-style",
						    "Parent style unique id", 
						    0, G_MAXUINT, 0, 
						    ZMAP_PARAM_STATIC_RW));
  /* description */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DESCRIPTION,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_DESCRIPTION, "description",
						      "Description", "",
						      ZMAP_PARAM_STATIC_RW));

  /* Mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_MODE, "mode", "mode", 
						    ZMAPSTYLE_MODE_INVALID, 
						    ZMAPSTYLE_MODE_META, 
						    ZMAPSTYLE_MODE_INVALID, 
						    ZMAP_PARAM_STATIC_RW));

  /* Colours... */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_COLOURS, "main colours",
						     "Colours used for normal feature display.",
						      "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME0_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_FRAME0_COLOURS, "frame0-colour-normal",
						     "Frame0 normal colour",
						     "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME1_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_FRAME1_COLOURS, "frame1-colour-normal",
						     "Frame1 normal colour",
						     "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME2_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_FRAME2_COLOURS, "frame2-colour-normal",
						     "Frame2 normal colour",
						     "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_REV_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_REV_COLOURS, "main-colour-normal",
						     "Reverse Strand normal colour",
						     "", ZMAP_PARAM_STATIC_RW));

  /* Display mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_COLUMN_DISPLAY_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_DISPLAY_MODE, "display-mode",
						    "Display mode", 
						    ZMAPSTYLE_COLDISPLAY_INVALID, 
						    ZMAPSTYLE_COLDISPLAY_SHOW, 
						    ZMAPSTYLE_COLDISPLAY_INVALID, 
						    ZMAP_PARAM_STATIC_RW));


  /* bump mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_BUMP_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_BUMP_MODE, "bump-mode",
						    "The Bump Mode", 
						    ZMAPBUMP_INVALID, 
						    ZMAPBUMP_END, 
						    ZMAPBUMP_INVALID, 
						    ZMAP_PARAM_STATIC_RW));
  /* bump default */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_BUMP_DEFAULT,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_DEFAULT_BUMP_MODE, "default-bump-mode",
						    "The Default Bump Mode", 
						    ZMAPBUMP_INVALID, 
						    ZMAPBUMP_END, 
						    ZMAPBUMP_INVALID, 
						    ZMAP_PARAM_STATIC_RW));
  /* bump fixed */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_BUMP_FIXED,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_BUMP_FIXED, "bump-fixed",
						       "Style cannot be changed once set.",
						       FALSE, ZMAP_PARAM_STATIC_RW));
  /* bump spacing */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_BUMP_SPACING,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_BUMP_SPACING, "bump-spacing",
						      "space between columns in bumped columns", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));

  /* Frame mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_FRAME_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_FRAME_MODE, "3 frame display mode",
						    "Defines frame sensitive display in 3 frame mode.",
						    ZMAPSTYLE_3_FRAME_INVALID, 
						    ZMAPSTYLE_3_FRAME_ONLY_1, 
						    ZMAPSTYLE_3_FRAME_INVALID,
						    ZMAP_PARAM_STATIC_RW));


  /* min mag */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MIN_MAG,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_MIN_MAG, "min-mag",
						      "minimum magnification", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* max mag */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAX_MAG,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_MAX_MAG, "max-mag",
						      "maximum magnification", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));

  /* width */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_WIDTH,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_WIDTH, "width",
						      "Width", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* score mode */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SCORE_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_SCORE_MODE, "score-mode",
						    "Score Mode",
						    ZMAPSCORE_INVALID,
						    ZMAPSCORE_PERCENT,
						    ZMAPSCORE_INVALID,
						    ZMAP_PARAM_STATIC_RW));
  /* min score */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MIN_SCORE,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_MIN_SCORE, "min-score",
						      "minimum score", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* max score */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_MAX_SCORE,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_MAX_SCORE, "max-score",
						      "maximum score", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));
  /* gff_source */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GFF_SOURCE,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_GFF_SOURCE, "gff source",
						      "GFF Source", "",
						      ZMAP_PARAM_STATIC_RW));
  /* gff_feature */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GFF_FEATURE,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_GFF_FEATURE, "gff feature",
						      "GFF Feature", "",
						      ZMAP_PARAM_STATIC_RW));

  /* style->opts properties */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DISPLAYABLE,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_DISPLAYABLE, "displayable",
						       "Is the style Displayable",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_WHEN_EMPTY,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_WHEN_EMPTY, "show when empty",
						       "Does the Style get shown when empty",
						       FALSE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_TEXT,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_TEXT, "show-text",
						       "Show as Text",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_STRAND_SPECIFIC,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, "strand specific",
						       "Strand Specific ?",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_REV_STRAND,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_REVERSE_STRAND, "show-reverse-strand",
						       "Show Rev Strand ?",
						       TRUE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_SHOW_ONLY_IN_SEPARATOR,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_SHOW_ONLY_IN_SEPARATOR, "only in separator",
						       "Show Only in Separator",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DIRECTIONAL_ENDS,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_DIRECTIONAL_ENDS, "directional-ends",
						       "Display pointy \"short sides\"",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_DEFERRED,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_DEFERRED, "deferred",
						       "Load only when specifically asked",
						       TRUE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_LOADED,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_LOADED, "loaded",
						       "Style Loaded from server",
						       TRUE, ZMAP_PARAM_STATIC_RW));

  /* Graph mode. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GRAPH_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_GRAPH_MODE, "graph-mode",
						    "Graph Mode",
						    ZMAPSTYLE_GRAPH_INVALID,
						    ZMAPSTYLE_GRAPH_HISTOGRAM,
						    ZMAPSTYLE_GRAPH_INVALID,
						    ZMAP_PARAM_STATIC_RW));
  /* Graph baseline value. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GRAPH_BASELINE,
				  g_param_spec_double(ZMAPSTYLE_PROPERTY_GRAPH_BASELINE, "graph baseline",
						      "Sets the baseline for graph values.", 
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      ZMAP_PARAM_STATIC_RW));


  /* Glyph mode. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_GLYPH_MODE,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_GLYPH_MODE, "glyph-mode",
						    "Glyph Mode",
						    ZMAPSTYLE_GLYPH_INVALID,
						    ZMAPSTYLE_GLYPH_SPLICE,
						    ZMAPSTYLE_GLYPH_INVALID,
						    ZMAP_PARAM_STATIC_RW));

  /* Parse out gap data from data source, selectable because data can be very large. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_PARSE_GAPS,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_ALIGNMENT_PARSE_GAPS, "parse gaps",
						       "Parse Gaps ?",
						       TRUE, ZMAP_PARAM_STATIC_RW));

  /* display gaps, selectable because data can be very large. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_SHOW_GAPS,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_ALIGNMENT_SHOW_GAPS, "show gaps",
						       "Show Gaps ?",
						       TRUE, ZMAP_PARAM_STATIC_RW));



  /* Aligment between aligns allowable error. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_BETWEEN_ERROR,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_ALIGNMENT_JOIN_ALIGN, "alignment between error",
						    "Allowable alignment error between HSP's",
						    0,
						    G_MAXUINT,
						    0,
						    ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN,
						       "Allow misalign",
						       "Allow match -> ref sequences to be different lengths.",
						       FALSE, ZMAP_PARAM_STATIC_RW));
  /* Use pfetch proxy ? */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_PFETCHABLE,
				  g_param_spec_boolean(ZMAPSTYLE_PROPERTY_ALIGNMENT_PFETCHABLE, "Pfetchable alignments",
						       "Use pfetch proxy to get alignments ?",
						       TRUE, ZMAP_PARAM_STATIC_RW));

  /* feature is blixemable ? */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_BLIXEM,
				  g_param_spec_uint(ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM, "Blixemable alignments",
						    "Use blixem to view sequence of alignments ?",
						    ZMAPSTYLE_BLIXEM_INVALID, 
						    ZMAPSTYLE_BLIXEM_X, 
						    ZMAPSTYLE_BLIXEM_INVALID, 
						    ZMAP_PARAM_STATIC_RW)) ;

  /* These three colours are used to join HSP's with colour lines to show colinearity. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_PERFECT_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_ALIGNMENT_PERFECT_COLOURS, "perfect alignment indicator colour",
						     "Colours used to show two alignments have exactly contiguous coords.",
						      "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_ALIGNMENT_COLINEAR_COLOURS, "colinear alignment indicator colour",
						     "Colours used to show two alignments have exactly contiguous coords.",
						      "", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_ALIGNMENT_NONCOLINEAR_COLOURS, "noncolinear alignment indicator colour",
						     "Colours used to show two alignments have exactly contiguous coords.",
						      "", ZMAP_PARAM_STATIC_RW));

  /* Transcript CDS colours. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_TRANSCRIPT_CDS_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS, "CDS colours",
						     "Colour used to CDS section of a transcript.",
						      "", ZMAP_PARAM_STATIC_RW));

  /* Assembly path colours. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_ASSEMBLY_PATH_NON_COLOURS, "non-path colours",
						     "Colour used for unused part of an assembly path sequence.",
						      "", ZMAP_PARAM_STATIC_RW));

  /* Text Font. */
  g_object_class_install_property(gobject_class,
				  STYLE_PROP_TEXT_FONT,
				  g_param_spec_string(ZMAPSTYLE_PROPERTY_TEXT_FONT, "Text Font",
						      "Font to draw text with.",
						      "", ZMAP_PARAM_STATIC_RW));

  

  gobject_class->dispose  = zmap_feature_type_style_dispose;
  gobject_class->finalize = zmap_feature_type_style_finalize;


  return ;
}

static void zmap_feature_type_style_init(ZMapFeatureTypeStyle      style)
{
  style->implied_mode = ZMAPSTYLE_MODE_INVALID;

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
  ZMapFeatureTypeStyle style, copy_style = NULL ;
  gboolean result = TRUE ;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));

  style = ZMAP_FEATURE_STYLE(gobject);

  /* Try to retrieve the style to be copied, returns NULL if we have been called just to set
   * values rather than copy. */
  copy_style = g_object_get_data(gobject, ZMAPBASECOPY_PARAMDATA_KEY) ;

  if (copy_style && style->mode == ZMAPSTYLE_MODE_INVALID)
    {
      /* This should only happen _once_ for each style copy, not param_count times. */
      if (copy_style->mode != ZMAPSTYLE_MODE_INVALID)
	style->mode = style->implied_mode = copy_style->mode;
      else			/* the best we can do...  */
	style->mode = style->implied_mode = copy_style->implied_mode;

      if (style->mode != ZMAPSTYLE_MODE_INVALID)
	style->fields_set.mode = 1;
    }
  else if (style->implied_mode == ZMAPSTYLE_MODE_INVALID)
    {
      set_implied_mode(style, param_id);
    }


  switch(param_id)
    {
      /* Name and ID shouldn't be set in a copy...check this.... */
    case STYLE_PROP_NAME:
      {
	const char *name;

	if ((name = g_value_get_string(value)))
	  {
	    style->original_id = g_quark_from_string(name);
	    style->unique_id   = zMapStyleCreateID((char *)name);
	  }
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_ORIGINAL_ID:
      {
	GQuark id;

	if ((id = g_value_get_uint(value)) > 0)
	  style->original_id = id;
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_UNIQUE_ID:
      {
	GQuark id;

	if ((id = g_value_get_uint(value)) > 0)
	  style->unique_id = id;
	else
	  result = FALSE ;

	break;
      }

    case STYLE_PROP_PARENT_STYLE:
      {
	GQuark parent_id;

	if (!copy_style || copy_style->fields_set.parent_id)
	  {
	    if ((parent_id = g_value_get_uint(value)) > 0)
	      {
		style->parent_id = parent_id;
		style->fields_set.parent_id = TRUE;
	      }
	    else
	      result = FALSE ;
	  }

	break;
      }
    case STYLE_PROP_DESCRIPTION:
      {
	const char *description;
	gboolean set = FALSE ;

	if (!copy_style || copy_style->fields_set.description)
	  {
	    if ((description = g_value_get_string(value)))
	      {
		if (style->description)
		  g_free(style->description);

		style->description = g_value_dup_string(value);
		set = TRUE ;
	      }
	    else if (style->description)
	      {
		/* Free if we did have a value, but now don't */
		g_free(style->description);
		style->description = NULL;
	      }

	    /* record if we're set or not */
	    style->fields_set.description = set;
	  }

	break;
      }

    case STYLE_PROP_MODE:
      {
	SETMODEFIELD(style, copy_style, value, ZMapStyleMode, fields_set.mode, mode, result) ;

	style->implied_mode = style->mode;
	break;
      }

    case STYLE_PROP_COLOURS:
    case STYLE_PROP_FRAME0_COLOURS:
    case STYLE_PROP_FRAME1_COLOURS:
    case STYLE_PROP_FRAME2_COLOURS:
    case STYLE_PROP_REV_COLOURS:
      {
	result = parseColours(style, copy_style, param_id, (GValue *)value) ;

	break;
      }


    case STYLE_PROP_COLUMN_DISPLAY_MODE:
      {
	SETMODEFIELD(style, copy_style, value, ZMapStyleColumnDisplayState, fields_set.col_display_state, col_display_state, result) ;

	break;
      }
      
    case STYLE_PROP_BUMP_MODE:
      {
	SETMODEFIELD(style, copy_style, value, ZMapStyleBumpMode, fields_set.curr_bump_mode, curr_bump_mode, result) ;

	break;
      }
    case STYLE_PROP_BUMP_DEFAULT:
      {
	SETMODEFIELD(style, copy_style, value, ZMapStyleBumpMode, fields_set.default_bump_mode, default_bump_mode, result) ;

	break;
      }
    case STYLE_PROP_BUMP_FIXED:
      {
	if (copy_style || !zmapStyleBumpIsFixed(style))
	  {
	    SETBOOLFIELD(style, copy_style, value, fields_set.bump_fixed, opts.bump_fixed) ;
	  }

	break;
      }
    case STYLE_PROP_BUMP_SPACING:
      {
	if (!copy_style || copy_style->fields_set.bump_spacing)
	  {
	    style->bump_spacing = g_value_get_double(value) ;
	    style->fields_set.bump_spacing = TRUE ;
	  }
	break; 
      }

    case STYLE_PROP_FRAME_MODE:
      {
	if (!copy_style || copy_style->fields_set.frame_mode)
	  {
	    ZMapStyle3FrameMode frame_mode ;

	    frame_mode = g_value_get_uint(value) ;

	    if (frame_mode > ZMAPSTYLE_3_FRAME_INVALID && frame_mode <= ZMAPSTYLE_3_FRAME_ONLY_1)
	      {
		style->fields_set.frame_mode = TRUE ;
		style->frame_mode = frame_mode ;
	      }
	  }

	break;
      }

    case STYLE_PROP_MIN_MAG:
      {
	if (!copy_style || copy_style->fields_set.min_mag)
	  {
	    if ((style->min_mag = g_value_get_double(value)) != 0.0)
	      style->fields_set.min_mag = 1;
	  }
	break;
      }
    case STYLE_PROP_MAX_MAG:
      {
	if (!copy_style || copy_style->fields_set.max_mag)
	  {
	    if ((style->max_mag = g_value_get_double(value)) != 0.0)
	      style->fields_set.max_mag = TRUE ;
	  }
	break;
      }
    case STYLE_PROP_WIDTH:
      {
	if (!copy_style || copy_style->fields_set.width)
	  {
	    if ((style->width = g_value_get_double(value)) != 0.0)
	      style->fields_set.width = TRUE ;
	  }
	break;
      }

    case STYLE_PROP_SCORE_MODE:
      {
	if (!copy_style || copy_style->fields_set.score_mode)
	  {
	    ZMapStyleScoreMode score_mode ;

	    score_mode = g_value_get_uint(value) ;

	    style->fields_set.score_mode = TRUE ;
	  }
	break; 
      }
    case STYLE_PROP_MIN_SCORE:
      {
	if (!copy_style || copy_style->fields_set.min_score)
	  {
	    if ((style->min_score = g_value_get_double(value)) != 0.0)
	      style->fields_set.min_score = TRUE ;
	  }
	break;
      }
    case STYLE_PROP_MAX_SCORE:
      {
	if (!copy_style || copy_style->fields_set.max_score)
	  {
	    if ((style->max_score = g_value_get_double(value)) != 0.0)
	      style->fields_set.max_score = TRUE ;
	  }
	break;
      }

    case STYLE_PROP_GFF_SOURCE:
      {
	if (!copy_style || copy_style->fields_set.gff_source)
	  {
	    const char *gff_source_str = NULL;

	    if ((gff_source_str = g_value_get_string(value)) != NULL)
	      {
		style->gff_source = g_quark_from_string(gff_source_str);
		style->fields_set.gff_source = TRUE ;
	      }
	  }
	break;
      }
    case STYLE_PROP_GFF_FEATURE:
      {
	if (!copy_style || copy_style->fields_set.gff_feature)
	  {
	    const char *gff_feature_str = NULL;

	    if ((gff_feature_str = g_value_get_string(value)) != NULL)
	      {
		style->gff_feature = g_quark_from_string(gff_feature_str);
		style->fields_set.gff_feature = TRUE ;
	      }
	  }
	break;
      }

    case STYLE_PROP_DISPLAYABLE:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.displayable, opts.displayable) ;

	break;
      }
    case STYLE_PROP_SHOW_WHEN_EMPTY:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.show_when_empty, opts.show_when_empty) ;

	break;
      }
    case STYLE_PROP_SHOW_TEXT:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.showText, opts.showText) ;

	break;
      }

    case STYLE_PROP_STRAND_SPECIFIC:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.strand_specific, opts.strand_specific) ;

	break;
      }
    case STYLE_PROP_SHOW_REV_STRAND:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.show_rev_strand, opts.show_rev_strand) ;

	break;
      }

    case STYLE_PROP_SHOW_ONLY_IN_SEPARATOR:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.show_only_in_separator, opts.show_only_in_separator) ;

	break;
      }
    case STYLE_PROP_DIRECTIONAL_ENDS:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.directional_end, opts.directional_end) ;

	break;
      }

    case STYLE_PROP_DEFERRED:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.deferred, opts.deferred) ;

	break;
      }
    case STYLE_PROP_LOADED:
      {
	SETBOOLFIELD(style, copy_style, value, fields_set.loaded, opts.loaded) ;

	break;
      }

    case STYLE_PROP_GRAPH_MODE:
    case STYLE_PROP_GRAPH_BASELINE:
      {
	/* Handle all graph specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_GRAPH)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_GRAPH, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_GRAPH_MODE:
		{
		  SETMODEFIELD(style, copy_style, value, ZMapStyleGraphMode, 
			       mode_data.graph.fields_set.mode, mode_data.graph.mode, result) ;
		  break ;
		}
	      case STYLE_PROP_GRAPH_BASELINE:
		{
		  if (!copy_style || copy_style->mode_data.graph.fields_set.baseline)
		    {
		      double baseline ;
		
		      /* Test value returned ?? */
		      baseline = g_value_get_double(value) ;
		
		      style->mode_data.graph.baseline = baseline ;
		      style->mode_data.graph.fields_set.baseline = TRUE ;
		    }
		  break; 
		}

	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		break;
	      }
	  }

	break ;
      }

    case STYLE_PROP_GLYPH_MODE:
      {
	/* Handle all glyph specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_GLYPH)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_GLYPH, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_GLYPH_MODE:
		{
		  SETMODEFIELD(style, copy_style, value, ZMapStyleGlyphMode, 
			       mode_data.glyph.fields_set.mode, mode_data.glyph.mode, result) ;
		  break ;
		}
	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
	    result = FALSE ;
		break;
	      }
	  }

	break ;
      }

    case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
    case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
    case STYLE_PROP_ALIGNMENT_PFETCHABLE:
    case STYLE_PROP_ALIGNMENT_BLIXEM:
    case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
    case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
      {
	/* Handle all alignment specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_ALIGNMENT)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_ALIGNMENT, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
		{
		  SETBOOLFIELD(style, copy_style, value, mode_data.alignment.fields_set.parse_gaps,
			       mode_data.alignment.state.parse_gaps) ;
		  
		  break;
		}
	      case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
		{
		  SETBOOLFIELD(style, copy_style, value, mode_data.alignment.fields_set.show_gaps,
			       mode_data.alignment.state.show_gaps) ;
		  
		  break;
		}
	      case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
		{
		  if (!copy_style || copy_style->mode_data.alignment.fields_set.between_align_error)
		    {
		      unsigned int error ;
		      
		      error = g_value_get_uint(value) ;
		      
		      style->mode_data.alignment.between_align_error = error ;
		      style->mode_data.alignment.fields_set.between_align_error = TRUE ;
		    }
		  break; 
		}
	      case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
		{
		  SETBOOLFIELD(style, copy_style, value,
			       mode_data.alignment.fields_set.allow_misalign, 
			       mode_data.alignment.state.allow_misalign) ;
		  
		  break ;
		}
	      case STYLE_PROP_ALIGNMENT_PFETCHABLE:
		{
		  SETBOOLFIELD(style, copy_style, value,
			       mode_data.alignment.fields_set.pfetchable, 
			       mode_data.alignment.state.pfetchable) ;
		  
		  break ;
		}
	      case STYLE_PROP_ALIGNMENT_BLIXEM:
		{
		  SETMODEFIELD(style, copy_style, value, ZMapStyleBlixemType,
			       mode_data.alignment.fields_set.blixem, mode_data.alignment.blixem_type, 
			       result) ;
		  
		  break ;
		}
	      case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
	      case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
	      case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
		{
		  result = parseColours(style, copy_style, param_id, (GValue *)value) ;
		  
		  break;
		}
	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
	    result = FALSE ;
		break;
	      }	/* switch(param_id) */
	  } /* else */

	break;
      }	/* case STYLE_PROP_ALIGNMENT_PARSE_GAPS..STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS */


    case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
      {
	/* Handle all assembly path specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_ASSEMBLY_PATH)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_ASSEMBLY_PATH, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
		result = parseColours(style, copy_style, param_id, (GValue *)value) ;

		break ;

	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		result = FALSE ;
		
		break;
	      }
	  }

	break;
      }

    case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
      {
	/* Handle all transcript specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_TRANSCRIPT)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_TRANSCRIPT, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
		result = parseColours(style, copy_style, param_id, (GValue *)value) ;

		break ;

	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		result = FALSE ;
		
		break;
	      }
	  }

	break;
      }

    case STYLE_PROP_TEXT_FONT:
      {
	/* Handle all text specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_TEXT &&
	    style->implied_mode != ZMAPSTYLE_MODE_RAW_SEQUENCE &&
	    style->implied_mode != ZMAPSTYLE_MODE_PEP_SEQUENCE)
	  {
	    if (!copy_style)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_TEXT, "set", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_TEXT_FONT:

		if (!copy_style || copy_style->mode_data.text.fields_set.font)
		  {
		    char *font_name ;
		
		    font_name = g_value_dup_string(value) ;
		
		    style->mode_data.text.font = font_name;
		    style->mode_data.text.fields_set.font = 1;
		  }

		break ;

	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		result = FALSE ;
		break;
	      }
	  }

	break;
      }

    default:
      {
	G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
	result = FALSE ;
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
  gboolean copy = FALSE ;

  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(gobject));

  style = ZMAP_FEATURE_STYLE(gobject);

  copy = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(style), ZMAPSTYLE_OBJ_COPY)) ;


  if (style->implied_mode == ZMAPSTYLE_MODE_INVALID)
    set_implied_mode(style, param_id);


  switch(param_id)
    {
      /* I think a style always has to have a name so no need to check the value... */
    case STYLE_PROP_NAME:
      {
	g_value_set_string(value, g_quark_to_string(style->original_id));
	break;
      }

    case STYLE_PROP_ORIGINAL_ID:
      {
	g_value_set_uint(value, style->original_id);

	break;
      }

    case STYLE_PROP_UNIQUE_ID:
      {
	g_value_set_uint(value, style->unique_id);

	break;
      }

    case STYLE_PROP_COLUMN_DISPLAY_MODE:
      {
	if (style->fields_set.col_display_state)
	  g_value_set_uint(value, style->col_display_state);
	else
	  result = FALSE ;

	break;
      }

    case STYLE_PROP_PARENT_STYLE:
      {
	if (style->fields_set.parent_id)
	  g_value_set_uint(value, style->parent_id);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_DESCRIPTION:
      {
	if (style->fields_set.description)
	  g_value_set_string(value, style->description);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_MODE:
      {
	if (style->fields_set.mode)
	  g_value_set_uint(value, style->mode);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_COLOURS:
    case STYLE_PROP_FRAME0_COLOURS:
    case STYLE_PROP_FRAME1_COLOURS:
    case STYLE_PROP_FRAME2_COLOURS:
    case STYLE_PROP_REV_COLOURS:
      {
	/* We allocate memory here to hold the colour string, it's g_object_get caller's 
	 * responsibility to g_free the string... */
	ZMapStyleFullColour this_colour = NULL ;
	GString *colour_string = NULL ;

	colour_string = g_string_sized_new(500) ;

	switch(param_id)
	  {
	  case STYLE_PROP_COLOURS:
	    this_colour = &(style->colours) ;
	    break;
	  case STYLE_PROP_FRAME0_COLOURS:
	    this_colour = &(style->frame0_colours) ;
	    break;
	  case STYLE_PROP_FRAME1_COLOURS:
	    this_colour = &(style->frame1_colours) ;
	    break;
	  case STYLE_PROP_FRAME2_COLOURS:
	    this_colour = &(style->frame2_colours) ;
	    break;
	  case STYLE_PROP_REV_COLOURS:
	    this_colour = &(style->strand_rev_colours) ;
	    break;
	  default:
	    break;
	  }

	formatColours(colour_string, "normal", &(this_colour->normal)) ;
	formatColours(colour_string, "selected", &(this_colour->selected)) ;
  
	if (colour_string->len)
	  g_value_set_string(value, g_string_free(colour_string, FALSE)) ;
	else
	  {
	    g_string_free(colour_string, TRUE) ;
	    result = FALSE ;
	  }

	break;
      }

    case STYLE_PROP_MIN_SCORE:
      {
	if (style->fields_set.min_score)
	  g_value_set_double(value, style->min_score);      
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_MAX_SCORE:
      {
	if (style->fields_set.max_score)
	  g_value_set_double(value, style->max_score);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_BUMP_MODE:
      {
	if (style->fields_set.curr_bump_mode)
	  g_value_set_uint(value, style->curr_bump_mode);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_BUMP_DEFAULT:
      {
	if (style->fields_set.default_bump_mode)
	  g_value_set_uint(value, style->default_bump_mode);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_BUMP_FIXED:
      {
	if (style->fields_set.bump_fixed)
	  g_value_set_boolean(value, style->opts.bump_fixed);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_BUMP_SPACING:
      {
	if (style->fields_set.bump_spacing)
	  g_value_set_double(value, style->bump_spacing);
	else
	  result = FALSE ;

	break; 
      }
    case STYLE_PROP_FRAME_MODE:
      {
	if (style->fields_set.frame_mode)
	  g_value_set_uint(value, style->frame_mode) ;
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_MIN_MAG:
      {
	if (style->fields_set.min_mag)
	  g_value_set_double(value, style->min_mag);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_MAX_MAG:
      {
	if (style->fields_set.max_mag)
	  g_value_set_double(value, style->max_mag);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_WIDTH:
      {
	if (style->fields_set.width)
	  g_value_set_double(value, style->width);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_SCORE_MODE:
      {
	if (style->fields_set.score_mode)
	  g_value_set_uint(value, style->score_mode);
	else
	  result = FALSE ;

	break; 
      }
    case STYLE_PROP_GFF_SOURCE:
      {
	if (style->fields_set.gff_source)	
	  g_value_set_string(value, g_quark_to_string(style->gff_source));
	else
	  result = FALSE ;

	break;
      }

    case STYLE_PROP_GFF_FEATURE:
      {
	if (style->fields_set.gff_feature)
	  g_value_set_string(value, g_quark_to_string(style->gff_feature));
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_DISPLAYABLE:
      {
	if (style->fields_set.displayable)
	  g_value_set_boolean(value, style->opts.displayable);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_SHOW_WHEN_EMPTY:
      {
	if (style->fields_set.show_when_empty)
	  g_value_set_boolean(value, style->opts.show_when_empty);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_SHOW_TEXT:
      {
	if (style->fields_set.showText)
	  g_value_set_boolean(value, style->opts.showText);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_STRAND_SPECIFIC:
      {
	if (style->fields_set.strand_specific)
	  g_value_set_boolean(value, style->opts.strand_specific);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_SHOW_REV_STRAND:
      {
	if (style->fields_set.show_rev_strand)
	  g_value_set_boolean(value, style->opts.show_rev_strand);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_SHOW_ONLY_IN_SEPARATOR:
      {
	if (style->fields_set.show_only_in_separator)
	  g_value_set_boolean(value, style->opts.show_only_in_separator);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_DIRECTIONAL_ENDS:
      {
	if (style->fields_set.directional_end)
	  g_value_set_boolean(value, style->opts.directional_end);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_DEFERRED:
      {
	if (style->fields_set.deferred)
	  g_value_set_boolean(value, style->opts.deferred);
	else
	  result = FALSE ;

	break;
      }
    case STYLE_PROP_LOADED:
      {
	if (style->fields_set.loaded)
	  g_value_set_boolean(value, style->opts.loaded);
	else
	  result = FALSE ;

	break;
      }

    case STYLE_PROP_GRAPH_MODE:
    case STYLE_PROP_GRAPH_BASELINE:
      {
	/* Handle all graph specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_GRAPH)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_GRAPH, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_GRAPH_MODE:
		{
		  if (style->mode_data.graph.fields_set.mode)
		    g_value_set_uint(value, style->mode_data.graph.mode);
		  else
		    result = FALSE ;
		  break;
		}

	      case STYLE_PROP_GRAPH_BASELINE:
		{
		  if (style->mode_data.graph.fields_set.baseline)
		    g_value_set_double(value, style->mode_data.graph.baseline) ;      
		  else
		    result = FALSE ;

		  break;
		}

	      default:
		{
		  G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		  result = FALSE ;
		  break;
		}
	      }
	  }

	break ;
      }


    case STYLE_PROP_GLYPH_MODE:
      {
	/* Handle all glyph specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_GLYPH)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_GLYPH, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_GLYPH_MODE:
		{
		  if (style->mode_data.glyph.fields_set.mode)
		    g_value_set_uint(value, style->mode_data.glyph.mode);
		  else
		    result = FALSE ;

		  break ;
		}
	      default:
		{
		  G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		  result = FALSE ;
		  break;
		}
	      }
	  }

	break ;
      }

    case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
    case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
    case STYLE_PROP_ALIGNMENT_PFETCHABLE:
    case STYLE_PROP_ALIGNMENT_BLIXEM:
    case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
    case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
      {
	/* alignment mode_data access */

	if (style->implied_mode != ZMAPSTYLE_MODE_ALIGNMENT)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_ALIGNMENT, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_ALIGNMENT_PARSE_GAPS:
		{
		  if (style->mode_data.alignment.fields_set.parse_gaps)
		    g_value_set_boolean(value, style->mode_data.alignment.state.parse_gaps);
		  else
		    result = FALSE ;

		  break;
		}
	      case STYLE_PROP_ALIGNMENT_SHOW_GAPS:
		{
		  if (style->mode_data.alignment.fields_set.show_gaps)
		    g_value_set_boolean(value, style->mode_data.alignment.state.show_gaps);
		  else
		    result = FALSE ;

		  break;
		}
	      case STYLE_PROP_ALIGNMENT_PFETCHABLE:
		{
		  if (style->mode_data.alignment.fields_set.pfetchable)
		    g_value_set_boolean(value, style->mode_data.alignment.state.pfetchable);
		  else
		    result = FALSE ;

		  break;
		}
	      case STYLE_PROP_ALIGNMENT_BLIXEM:
		{
		  if (style->mode_data.alignment.fields_set.blixem)
		    g_value_set_uint(value, style->mode_data.alignment.blixem_type);
		  else
		    result = FALSE ;

		  break;
		}
	      case STYLE_PROP_ALIGNMENT_BETWEEN_ERROR:
		{
		  if (style->mode_data.alignment.fields_set.between_align_error)
		    g_value_set_uint(value, style->mode_data.alignment.between_align_error);
		  else
		    result = FALSE ;

		  break;
		}
	      case STYLE_PROP_ALIGNMENT_ALLOW_MISALIGN:
		{
		  if (style->mode_data.alignment.fields_set.allow_misalign)
		    g_value_set_boolean(value, style->mode_data.alignment.state.allow_misalign);
		  else
		    result = FALSE ;

		  break;
		}
		
	      case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
	      case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
	      case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
		{
		  /* We allocate memory here to hold the colour string, it's g_object_get caller's 
		   * responsibility to g_free the string... */
		  ZMapStyleFullColour this_colour = NULL ;
		  GString *colour_string = NULL ;
		  
		  colour_string = g_string_sized_new(500) ;
		  
		  switch(param_id)
		    {
		    case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
		      this_colour = &(style->mode_data.alignment.perfect) ;
		      break;
		    case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
		      this_colour = &(style->mode_data.alignment.colinear) ;
		      break;
		    case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
		      this_colour = &(style->mode_data.alignment.noncolinear) ;
		      break;
		    default:
		      break;
		    }
		  
		  formatColours(colour_string, "normal", &(this_colour->normal)) ;
		  formatColours(colour_string, "selected", &(this_colour->selected)) ;
		  
		  if (colour_string->len)
		    g_value_set_string(value, g_string_free(colour_string, FALSE)) ;
		  else
		    {
		      g_string_free(colour_string, TRUE) ;
		      result = FALSE ;
		    }
		  
		  break;
		}
	      default:
		{
		  G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		  result = FALSE ;
		  break;
		}
	      }	/* switch(param_id) */
	  } /* else */

	break;			/* case STYLE_PROP_ALIGNMENT_PARSE_GAPS .. STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS */
      }

      /* transcript mode_data access */
    case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
      {
	/* Handle all transcript specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_TRANSCRIPT)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_TRANSCRIPT, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    /* We allocate memory here to hold the colour string, it's g_object_get caller's 
	     * responsibility to g_free the string... */
	    ZMapStyleFullColour this_colour = NULL ;
	    GString *colour_string = NULL ;
	    
	    colour_string = g_string_sized_new(500) ;
	    
	    switch(param_id)
	      {
	      case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
		this_colour = &(style->mode_data.transcript.CDS_colours) ;
		break;
	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		break;
	      }
	    
	    formatColours(colour_string, "normal", &(this_colour->normal)) ;
	    formatColours(colour_string, "selected", &(this_colour->selected)) ;
	    
	    if (colour_string->len)
	      g_value_set_string(value, g_string_free(colour_string, FALSE)) ;
	    else
	      {
		g_string_free(colour_string, TRUE) ;
		result = FALSE ;
	      }
	  }
	break;
      }

      /* assembly path mode_data access */
    case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
      {
	/* Handle all assembly path specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_ASSEMBLY_PATH)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_ASSEMBLY_PATH, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    /* We allocate memory here to hold the colour string, it's g_object_get caller's 
	     * responsibility to g_free the string... */
	    ZMapStyleFullColour this_colour = NULL ;
	    GString *colour_string = NULL ;
	    
	    colour_string = g_string_sized_new(500) ;
	    
	    switch(param_id)
	      {
	      case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
		this_colour = &(style->mode_data.assembly_path.non_path_colours) ;
		break;
	      default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		break;
	      }
	    
	    formatColours(colour_string, "normal", &(this_colour->normal)) ;
	    formatColours(colour_string, "selected", &(this_colour->selected)) ;
	    
	    if (colour_string->len)
	      g_value_set_string(value, g_string_free(colour_string, FALSE)) ;
	    else
	      {
		g_string_free(colour_string, TRUE) ;
		result = FALSE ;
	      }
	  }
	break;
      }
    case STYLE_PROP_TEXT_FONT:
      {
	/* Handle all text specific options. */

	if (style->implied_mode != ZMAPSTYLE_MODE_TEXT &&
	    style->implied_mode != ZMAPSTYLE_MODE_RAW_SEQUENCE &&
	    style->implied_mode != ZMAPSTYLE_MODE_PEP_SEQUENCE)
	  {
	    if (!copy)
	      {
		badPropertyForMode(style, ZMAPSTYLE_MODE_TEXT, "get", pspec) ;
		result = FALSE ;
	      }
	  }
	else
	  {
	    switch(param_id)
	      {
	      case STYLE_PROP_TEXT_FONT:
		if (style->mode_data.text.fields_set.font)
		  g_value_set_string(value, style->mode_data.text.font);
		else
		  result = FALSE ;

		break ;
	      default:
		{
		  G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
		  result = FALSE ;

		  break;
		}
	      }
	  }
	break;
      }

    default:
      {
	G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
	result = FALSE ;

	break;
      }
    }

  if (!style->fields_set.mode)
    style->implied_mode = ZMAPSTYLE_MODE_INVALID;


  /* Now set the result so we can return it to the user. */
  g_object_set_data(G_OBJECT(style), ZMAPSTYLE_OBJ_RC, GINT_TO_POINTER(result)) ;
 
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
static gboolean parseColours(ZMapFeatureTypeStyle style, ZMapFeatureTypeStyle copy_style,
			     guint param_id, GValue *value)
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
	      ZMapStyleFullColour full_colour = NULL, copy_full_colour = NULL ;
	      ZMapStyleColour type_colour = NULL, copy_type_colour = NULL ;

	      switch (param_id)
		{
		case STYLE_PROP_COLOURS:
		  full_colour = &(style->colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->colours) ;
		  break;
		case STYLE_PROP_FRAME0_COLOURS:
		  full_colour = &(style->frame0_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->frame0_colours) ;
		  break;
		case STYLE_PROP_FRAME1_COLOURS:
		  full_colour = &(style->frame1_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->frame1_colours) ;
		  break;
		case STYLE_PROP_FRAME2_COLOURS:
		  full_colour = &(style->frame2_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->frame2_colours) ;
		  break;
		case STYLE_PROP_REV_COLOURS:
		  full_colour = &(style->strand_rev_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->strand_rev_colours) ;
		  break;
		case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
		  full_colour = &(style->mode_data.alignment.perfect) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->mode_data.alignment.perfect) ;
		  break;
		case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
		  full_colour = &(style->mode_data.alignment.colinear) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->mode_data.alignment.colinear) ;
		  break;
		case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
		  full_colour = &(style->mode_data.alignment.noncolinear) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->mode_data.alignment.noncolinear) ;
		  break;
		case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
		  full_colour = &(style->mode_data.transcript.CDS_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->mode_data.transcript.CDS_colours) ;
		  break;
		case STYLE_PROP_ASSEMBLY_PATH_NON_COLOURS:
		  full_colour = &(style->mode_data.assembly_path.non_path_colours) ;
		  if (copy_style)
		    copy_full_colour = &(copy_style->mode_data.assembly_path.non_path_colours) ;
		  break;
		default:
		  zMapAssertNotReached() ;
		  break;
		}
		    
	      switch (type)
		{
		case ZMAPSTYLE_COLOURTYPE_NORMAL:
		  type_colour = &(full_colour->normal) ;
		  if (copy_style)
		    copy_type_colour = &(copy_full_colour->normal) ;
		  break ;
		case ZMAPSTYLE_COLOURTYPE_SELECTED:
		  type_colour = &(full_colour->selected) ;
		  if (copy_style)
		    copy_type_colour = &(copy_full_colour->selected) ;
		  break ;
		default:
		  zMapAssertNotReached() ;
		  break;
		}

	      switch (context)
		{
		case ZMAPSTYLE_DRAW_FILL:
		  if (!copy_style || copy_type_colour->fields_set.fill)
		    {
		      if (!(type_colour->fields_set.fill = gdk_color_parse(colour, &(type_colour->fill))))
			result = FALSE ;
		    }
		  break ;
		case ZMAPSTYLE_DRAW_DRAW:
		  if (!copy_style || copy_type_colour->fields_set.draw)
		    {
		      if (!(type_colour->fields_set.draw = gdk_color_parse(colour, &(type_colour->draw))))
			result = FALSE ;
		    }
		  break ;
		case ZMAPSTYLE_DRAW_BORDER:
		  if (!copy_style || copy_type_colour->fields_set.border)
		    {
		      if (!(type_colour->fields_set.border = gdk_color_parse(colour, &(type_colour->border))))
			result = FALSE ;
		    }
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

      switch (param_id)
	{
	case STYLE_PROP_COLOURS:
	  this_colour = &(style->colours) ;
	  break;
	case STYLE_PROP_FRAME0_COLOURS:
	  this_colour = &(style->frame0_colours) ;
	  break;
	case STYLE_PROP_FRAME1_COLOURS:
	  this_colour = &(style->frame1_colours) ;
	  break;
	case STYLE_PROP_FRAME2_COLOURS:
	  this_colour = &(style->frame2_colours) ;
	  break;
	case STYLE_PROP_REV_COLOURS:
	  this_colour = &(style->strand_rev_colours) ;
	  break;
	case STYLE_PROP_ALIGNMENT_PERFECT_COLOURS:
	  this_colour = &(style->mode_data.alignment.perfect) ;
	  break;
	case STYLE_PROP_ALIGNMENT_COLINEAR_COLOURS:
	  this_colour = &(style->mode_data.alignment.colinear) ;
	  break;
	case STYLE_PROP_ALIGNMENT_NONCOLINEAR_COLOURS:
	  this_colour = &(style->mode_data.alignment.noncolinear) ;
	  break;
	case STYLE_PROP_TRANSCRIPT_CDS_COLOURS:
	  this_colour = &(style->mode_data.transcript.CDS_colours) ;
	  break;
	default:
	  zMapAssertNotReached() ;
	  break;
	}

      switch (type)
	{
	case ZMAPSTYLE_COLOURTYPE_NORMAL:
	  type_colour = &(this_colour->normal) ;
	  break ;
	case ZMAPSTYLE_COLOURTYPE_SELECTED:
	  type_colour = &(this_colour->selected) ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	  break;
	}

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


static void badPropertyForMode(ZMapFeatureTypeStyle style, ZMapStyleMode required_mode,
			       char *op, GParamSpec *pspec)
{
  zMapLogCritical("Style \"%s\", attempt to %s property \"%s\""
		  " which requires style mode \"%s\""
		  " but mode is \"%s\"",
		  zMapStyleGetName(style), op, pspec->name,
		  zMapStyleMode2ExactStr(required_mode),
		  zMapStyleMode2ExactStr(zMapStyleGetMode(style))) ;

  return ;
}
