/*  File: zmapFeatureTypes.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Functions for manipulating Type structs and sets of
 *              type structs.c
 *              
 * Exported functions: See ZMap/zmapFeature.h
 * HISTORY:
 * Last edited: Apr 21 17:06 2008 (rds)
 * Created: Tue Dec 14 13:15:11 2004 (edgrif)
 * CVS info:   $Id: zmapFeatureTypes.c,v 1.65 2008-04-21 16:07:26 rds Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>

/* This should go in the end..... */
#include <zmapFeature_P.h>

#include <zmapStyle_P.h>


/* Think about defaults, how should they be set, should we force user to set them ? */
#define ZMAPFEATURE_DEFAULT_WIDTH 10.0			    /* of features being displayed */


typedef struct
{
  GList **names ;
  GList **styles ;
  gboolean any_style_found ;
} CheckSetListStruct, *CheckSetList ;



typedef struct
{
  GData *curr_styles ;

} MergeStyleCBStruct, *MergeStyleCB ;


typedef struct
{
  gboolean error ;
  ZMapFeatureTypeStyle inherited_style ;
} InheritStyleCBStruct, *InheritStyleCB ;


typedef struct
{
  gboolean errors ;
  GData *style_set ;
  GData *inherited_styles ;
  ZMapFeatureTypeStyle prev_style ;
} InheritAllCBStruct, *InheritAllCB ;



static void doTypeSets(GQuark key_id, gpointer data, gpointer user_data) ;
static void typePrintFunc(GQuark key_id, gpointer data, gpointer user_data) ;

static void stylePrintFunc(gpointer data, gpointer user_data) ;

static void checkListName(gpointer data, gpointer user_data) ;
static gint compareNameToStyle(gconstpointer glist_data, gconstpointer user_data) ;

static void mergeStyle(GQuark style_id, gpointer data, gpointer user_data_unused) ;

static void destroyStyle(GQuark style_id, gpointer data, gpointer user_data_unused) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapFeatureTypeStyle createInheritedStyle(GData *style_set, char *parent_style) ;
static void inheritStyleCB(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static void inheritCB(GQuark key_id, gpointer data, gpointer user_data) ;
static gboolean doStyleInheritance(GData **style_set, GData **inherited_styles, ZMapFeatureTypeStyle curr_style) ;
static void inheritAllFunc(gpointer data, gpointer user_data) ;




/* Keep in step with enum.... */
char *style_col_state_str_G[]= {
  "ZMAPSTYLE_COLDISPLAY_INVALID",
  "ZMAPSTYLE_COLDISPLAY_HIDE",
  "ZMAPSTYLE_COLDISPLAY_SHOW_HIDE",
  "ZMAPSTYLE_COLDISPLAY_SHOW"} ;

char *style_mode_str_G[] = {
    "ZMAPSTYLE_MODE_INVALID",
    "ZMAPSTYLE_MODE_NONE",
    "ZMAPSTYLE_MODE_META",
    "ZMAPSTYLE_MODE_BASIC",
    "ZMAPSTYLE_MODE_TRANSCRIPT",
    "ZMAPSTYLE_MODE_ALIGNMENT",
    "ZMAPSTYLE_MODE_TEXT",
    "ZMAPSTYLE_MODE_GRAPH"} ;


char *style_overlapmode_str_G[] = {
    "ZMAPOVERLAP_START",
    "ZMAPOVERLAP_COMPLETE",
    "ZMAPOVERLAP_OVERLAP",
    "ZMAPOVERLAP_POSITION",
    "ZMAPOVERLAP_NAME",
    "ZMAPOVERLAP_COMPLEX",
    "ZMAPOVERLAP_NO_INTERLEAVE",
    "ZMAPOVERLAP_COMPLEX_RANGE",
    "ZMAPOVERLAP_ITEM_OVERLAP",
    "ZMAPOVERLAP_SIMPLE",
    "ZMAPOVERLAP_END"
} ;


char *style_scoremode_str_G[] = {
    "ZMAPSCORE_WIDTH",					    /* Use column width only - default. */
    "ZMAPSCORE_OFFSET",
    "ZMAPSCORE_HISTOGRAM",
    "ZMAPSCORE_PERCENT"
} ;



/*! @defgroup zmapstyles   zMapStyle: Feature Style handling for ZMap
 * @{
 * 
 * \brief  Feature Style handling for ZMap.
 * 
 * zMapStyle routines provide functions to create/modify/destroy individual
 * styles, the styles control how features are processed and displayed. They
 * control aspects such as foreground colour, column bumping mode etc.
 *
 *  */



/* Add a style to a set of styles, if the style is already there no action is taken
 * and FALSE is returned. */
gboolean zMapStyleSetAdd(GData **style_set, ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;
  GQuark style_id ;

  zMapAssert(style_set && style) ;

  style_id = zMapStyleGetUniqueID(style) ;

  if (!(zMapFindStyle(*style_set, style_id)))
    {
      g_datalist_id_set_data(style_set, style_id, style) ;
      result = TRUE ;
    }

  return result ;
}



/* Sets up all the inheritance for the set of styles.
 * 
 * The method is to record each style that we have inherited as we go, we then
 * check the set of inherited styles to see if we need to do one each time
 * we do through the list.
 * 
 * If there are errors in trying to inherit styles (e.g. non-existent parents)
 * then this function returns FALSE and there will be log messages identifying
 * the errors.
 * 
 *  */
gboolean zMapStyleInheritAllStyles(GData **style_set)
{
  gboolean result = FALSE ;
  GData *inherited_styles ;
  InheritAllCBStruct cb_data = {FALSE} ;

  g_datalist_init(&inherited_styles) ;
  cb_data.style_set = *style_set ;
  cb_data.inherited_styles = inherited_styles ;

  g_datalist_foreach(style_set, inheritCB, &cb_data) ;

  /* We always return the inherited set since the style set is inherited in place
   * but we signal that there were errors. */
  *style_set = cb_data.style_set ;
  result = !cb_data.errors ;
  g_datalist_clear(&inherited_styles) ;

  return result ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Creates a new style which has inherited properties from existing styles, the styles
 * are tied together in parent/child tree relationship by the parent_id tag in the style. */
ZMapFeatureTypeStyle zMapStyleCreateFull(GData *style_set, char *parent_style,
					 char *name, char *description, ZMapStyleMode mode,
					 char *outline, char *foreground, char *background,
					 double width)
{
  ZMapFeatureTypeStyle new_style = NULL ;
  ZMapFeatureTypeStyle inherited_style ;

  /* If we can create an inherited style then overload it with the supplied style params. */
  if ((inherited_style = createInheritedStyle(style_set, parent_style)))
    {
      ZMapFeatureTypeStyle tmp_style ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      tmp_style = zMapFeatureTypeCreate(name, description, mode,
					outline, foreground, background,
					width) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      tmp_style = zMapFeatureTypeCreate(name, description) ;
      zMapStyleSetMode(tmp_style, mode) ;
      zMapStyleSetColours(tmp_style, outline, foreground, background) ;
      zMapStyleSetWidth(tmp_style, width) ;


      /* This will leave the fully merged style in inherited_style, so we throw new_style
       * away. */
      if (zMapStyleMerge(inherited_style, tmp_style))
	new_style = inherited_style ;

      zMapFeatureTypeDestroy(tmp_style) ;
    }

  return new_style ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* Create a new type for displaying features. */
ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description)
{
  ZMapFeatureTypeStyle new_type = NULL ;
  char *name_lower ;

  zMapAssert(name && *name) ;

  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

  name_lower = g_ascii_strdown(name, -1) ;
  new_type->original_id = g_quark_from_string(name) ;
  new_type->unique_id = zMapStyleCreateID(name_lower) ;
  g_free(name_lower) ;

  if (description)
    {
      new_type->description = g_strdup(description) ;
      new_type->fields_set.description = TRUE ;
    }


  new_type->min_mag = new_type->max_mag = 0.0 ;

  /* This says that the default is to display but subject to zoom level etc. */
  new_type->opts.displayable = TRUE ;  
  new_type->col_display_state = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE ;


  return new_type ;
}


/*!
 * Copy an existing style. The copy will copy all dynamically allocated memory within
 * the struct as well.
 * 
 * Returns the new style or NULL if there is an error.
 * 
 * @param   style          The style to be copied.
 * @return  ZMapFeatureTypeStyle   the style copy or NULL
 *  */
ZMapFeatureTypeStyle zMapFeatureStyleCopy(ZMapFeatureTypeStyle style)
{
  ZMapFeatureTypeStyle new_style = NULL ;

  zMapAssert(style) ;

  new_style = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

  *new_style = *style ;					    /* n.b. struct copy. */
  
  if (new_style->description)
    new_style->description = g_strdup(new_style->description) ;

  return new_style ;
}


/*!
 * Overload one style with another. Values in curr_style are overwritten with those
 * in the new_style. new_style is not altered.
 * 
 * <b>NOTE</b> that both styles will have the same unique id so if you add the new_style
 * to a style set the reference to the old style will be removed.
 * 
 * Returns TRUE if merge ok, FALSE if there was a problem.
 * 
 * @param   curr_style          The style to be overwritten.
 * @param   new_style           The style to used for overwriting.
 * @return  gboolean            TRUE means successful merge.
 *  */
gboolean zMapStyleMerge(ZMapFeatureTypeStyle curr_style, ZMapFeatureTypeStyle new_style)
{
  gboolean result = TRUE ;				    /* There is nothing to fail currently. */


  zMapAssert(curr_style && new_style) ;

  curr_style->original_id = new_style->original_id ;
  curr_style->unique_id = new_style->unique_id ;

  if (new_style->fields_set.parent_style)
    {
      curr_style->fields_set.parent_style = TRUE ;
      curr_style->parent_id = new_style->parent_id ;
    }

  if (new_style->fields_set.description)
    {
      if (curr_style->fields_set.description)
	g_free(curr_style->description) ;

      curr_style->description = g_strdup(new_style->description) ;
      curr_style->fields_set.description = TRUE ;
    }

  if (new_style->fields_set.mode)
    {
      curr_style->mode = new_style->mode ;
      curr_style->fields_set.mode = TRUE ;
    }

  if (new_style->colours.normal.fields_set.fill)
    {
      curr_style->colours.normal.fill = new_style->colours.normal.fill ;
      curr_style->colours.normal.fields_set.fill = TRUE ;
    }
  if (new_style->colours.normal.fields_set.draw)
    {
      curr_style->colours.normal.draw = new_style->colours.normal.draw ;
      curr_style->colours.normal.fields_set.draw = TRUE ;
    }
  if (new_style->colours.normal.fields_set.border)
    {
      curr_style->colours.normal.border = new_style->colours.normal.border ;
      curr_style->colours.normal.fields_set.border = TRUE ;
    }

  if (new_style->fields_set.min_mag)
    {
      curr_style->min_mag = new_style->min_mag ;
      curr_style->fields_set.min_mag = TRUE ;
    }

  if (new_style->fields_set.max_mag)
    {
      curr_style->max_mag = new_style->max_mag ;
      curr_style->fields_set.max_mag = TRUE ;
    }

  if (new_style->fields_set.overlap_mode)
    {
      curr_style->curr_overlap_mode = new_style->curr_overlap_mode ;
      curr_style->fields_set.overlap_mode = TRUE ;
    }

  if (new_style->fields_set.overlap_default)
    {
      curr_style->default_overlap_mode = new_style->default_overlap_mode ;
      curr_style->fields_set.overlap_default = TRUE ;
    }

  if (new_style->fields_set.bump_spacing)
    {
      curr_style->bump_spacing = new_style->bump_spacing ;
      curr_style->fields_set.bump_spacing = TRUE ;
    }

  if (new_style->fields_set.width)
    {
      curr_style->width = new_style->width ;
      curr_style->fields_set.width = TRUE ;
    }

  if (new_style->fields_set.score_mode)
    {
      curr_style->score_mode = new_style->score_mode ;
      curr_style->fields_set.score_mode = TRUE ;
    }

  if (new_style->fields_set.min_score)
    {
      curr_style->min_score = new_style->min_score ;
      curr_style->fields_set.min_score = TRUE ;
    }

  if (new_style->fields_set.max_score)
    {
      curr_style->max_score = new_style->max_score ;
      curr_style->fields_set.max_score = TRUE ;
    }

  if (new_style->fields_set.gff_source)
    {
      curr_style->gff_source = new_style->gff_source ;
      curr_style->fields_set.gff_source = TRUE ;
    }

  if (new_style->fields_set.gff_feature)
    {
      curr_style->gff_feature = new_style->gff_feature ;
      curr_style->fields_set.gff_feature = TRUE ;
    }


  /* AGH THIS IS NOT GOOD....WE CAN TURN OFF/ON OPTIONS IN A WAY WE DON'T WANT TO...
   * 
   * need to revisit this for sure....
   *  */
  curr_style->opts = new_style->opts ;		    /* struct copy of all opts. */

  /* Now do mode specific stuff... */
  if (curr_style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      if (new_style->mode_data.transcript.CDS_colours.normal.fields_set.fill)
	{
	  curr_style->mode_data.transcript.CDS_colours.normal.fill = new_style->mode_data.transcript.CDS_colours.normal.fill ;
	  curr_style->mode_data.transcript.CDS_colours.normal.fields_set.fill = TRUE ;
	}
      if (new_style->mode_data.transcript.CDS_colours.normal.fields_set.draw)
	{
	  curr_style->mode_data.transcript.CDS_colours.normal.draw = new_style->mode_data.transcript.CDS_colours.normal.draw ;
	  curr_style->mode_data.transcript.CDS_colours.normal.fields_set.draw = TRUE ;
	}
      if (new_style->mode_data.transcript.CDS_colours.normal.fields_set.border)
	{
	  curr_style->mode_data.transcript.CDS_colours.normal.border = new_style->mode_data.transcript.CDS_colours.normal.border ;
	  curr_style->mode_data.transcript.CDS_colours.normal.fields_set.border = TRUE ;
	}






    }


  return result ;
}


#define ZMAPSTYLEPRINTOPT(STYLE_FIELD_DATA, STYLE_FIELD)	\
  printf("\tOpt " #STYLE_FIELD ": %s\n", ((STYLE_FIELD_DATA) ? "TRUE" : "FALSE"))

#define ZMAPSTYLEPRINTCOLOUR(GDK_COLOUR_PTR, COLOUR_NAME)	\
  printf("\tColour " #COLOUR_NAME ":\tpixel %d\tred %d green %d blue %d\n", \
	 (GDK_COLOUR_PTR).pixel, (GDK_COLOUR_PTR).red, (GDK_COLOUR_PTR).green, (GDK_COLOUR_PTR).blue)

/*!
 * Print out a style.
 *
 * @param   style               The style to be printed.
 * @return   <nothing>
 *  */
void zMapStylePrint(ZMapFeatureTypeStyle style, char *prefix)
{
  zMapAssert(style) ;

  printf("%s Style: %s (%s)\n", (prefix ? prefix : ""),
	 g_quark_to_string(style->original_id), g_quark_to_string(style->unique_id)) ;

  if (style->fields_set.parent_style)
    printf("\tParent style: %s\n", g_quark_to_string(style->parent_id)) ;

  if (style->fields_set.description)
    printf("\tDescription: %s\n", style->description) ;

  if (style->fields_set.mode)
    printf("Feature mode: %s\n", style_mode_str_G[style->mode]) ;

  if (style->colours.normal.fields_set.fill)
    ZMAPSTYLEPRINTCOLOUR(style->colours.normal.fill, background) ;

  if (style->colours.normal.fields_set.draw)
    ZMAPSTYLEPRINTCOLOUR(style->colours.normal.draw, foreground) ;

  if (style->colours.normal.fields_set.border)
    ZMAPSTYLEPRINTCOLOUR(style->colours.normal.border, outline) ;

  if (style->fields_set.min_mag)
    printf("\tMin mag: %g\n", style->min_mag) ;

  if (style->fields_set.max_mag)
    printf("\tMax mag: %g\n", style->max_mag) ;

  if (style->fields_set.overlap_mode)
    {
      printf("\tCurrent overlap mode: %s\n", style_overlapmode_str_G[style->curr_overlap_mode]) ;
    }

  if (style->fields_set.overlap_default)
    {
      printf("\tDefault overlap mode: %s\n", style_overlapmode_str_G[style->default_overlap_mode]) ;
    }

  if (style->fields_set.bump_spacing)
    printf("\tBump width: %g\n", style->bump_spacing) ;

  if (style->fields_set.width)
    printf("\tWidth: %g\n", style->width) ;

  if (style->fields_set.score_mode)
    printf("Score mode: %s\n", style_scoremode_str_G[style->score_mode]) ;

  if (style->fields_set.min_score)
    printf("\tMin score: %g\n", style->min_score) ;

  if (style->fields_set.max_score)
    printf("\tMax score: %g\n", style->max_score) ;

  if (style->fields_set.gff_source)
    printf("\tGFF source: %s\n", g_quark_to_string(style->gff_source)) ;

  if (style->fields_set.gff_feature)
    printf("\tGFF feature: %s\n", g_quark_to_string(style->gff_feature)) ;

  printf("Column display state: %s\n", style_col_state_str_G[style->col_display_state]) ;

  ZMAPSTYLEPRINTOPT(style->opts.displayable, displayable) ;
  ZMAPSTYLEPRINTOPT(style->opts.show_when_empty, show_when_empty) ;
  ZMAPSTYLEPRINTOPT(style->opts.showText, showText) ;
  ZMAPSTYLEPRINTOPT(style->opts.parse_gaps, parse_gaps) ;
  ZMAPSTYLEPRINTOPT(style->opts.align_gaps, align_gaps) ;
  ZMAPSTYLEPRINTOPT(style->opts.strand_specific, strand_specific) ;
  ZMAPSTYLEPRINTOPT(style->opts.show_rev_strand, show_rev_strand) ;
  ZMAPSTYLEPRINTOPT(style->opts.frame_specific, frame_specific) ;
  ZMAPSTYLEPRINTOPT(style->opts.show_only_as_3_frame, show_only_as_3_frame) ;
  ZMAPSTYLEPRINTOPT(style->opts.directional_end, directional_end) ;

  return ;
}



void zMapStyleSetParent(ZMapFeatureTypeStyle style, char *parent_name)
{
  zMapAssert(style && parent_name && *parent_name) ;

  style->fields_set.parent_style = TRUE ;
  style->parent_id = zMapStyleCreateID(parent_name) ;

  return ;
}

void zMapStyleSetDescription(ZMapFeatureTypeStyle style, char *description)
{
  zMapAssert(style && description && *description) ;

  style->fields_set.description = TRUE ;
  style->description = g_strdup(description) ;

  return ;
}

void zMapStyleSetWidth(ZMapFeatureTypeStyle style, double width)
{
  zMapAssert(style && width > 0.0) ;

  style->fields_set.width = TRUE ;
  style->width = width ;

  return ;
}

void zMapStyleSetBumpWidth(ZMapFeatureTypeStyle style, double bump_spacing)
{
  zMapAssert(style && bump_spacing >= 0.0) ;

  style->fields_set.bump_spacing = TRUE ;
  style->bump_spacing = bump_spacing ;

  return ;
}

double zMapStyleGetBumpWidth(ZMapFeatureTypeStyle style)
{
  double bump_spacing = 0.0 ;

  zMapAssert(style) ;

  if (style->fields_set.bump_spacing)
    bump_spacing = style->bump_spacing ;

  return  bump_spacing ;
}




/* Set magnification limits for displaying columns. */
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag)
{
  zMapAssert(style) ;

  if (min_mag && min_mag > 0.0)
    {
      style->min_mag = min_mag ;
      style->fields_set.min_mag = TRUE ;
    }

  if (max_mag && max_mag > 0.0)
    {
      style->max_mag = max_mag ;
      style->fields_set.max_mag = TRUE ;
    }


  return ;
}



gboolean zMapStyleIsMinMag(ZMapFeatureTypeStyle style, double *min_mag)
{
  gboolean mag_set = FALSE ;

  if (style->fields_set.min_mag)
    {
      mag_set = TRUE ;

      if (min_mag)
	*min_mag = style->min_mag ;
    }

  return mag_set ;
}


gboolean zMapStyleIsMaxMag(ZMapFeatureTypeStyle style, double *max_mag)
{
  gboolean mag_set = FALSE ;

  if (style->fields_set.max_mag)
    {
      mag_set = TRUE ;

      if (max_mag)
	*max_mag = style->max_mag ;
    }

  return mag_set ;
}




/* Set up graphing stuff, currently the basic code is copied from acedb but this will
 * change if we add different graphing types.... */
void zMapStyleSetGraph(ZMapFeatureTypeStyle style, ZMapStyleGraphMode mode,
		       double min_score, double max_score, double baseline)
{
  zMapAssert(style) ;

  style->mode_data.graph.mode = mode ;

  style->min_score = min_score ;
  style->max_score = max_score ;
  style->mode_data.graph.baseline = baseline ;

  /* normalise the baseline */
  if (style->min_score == style->max_score)
    style->mode_data.graph.baseline = style->min_score ;
  else
    style->mode_data.graph.baseline = (style->mode_data.graph.baseline - style->min_score) / (style->max_score - style->min_score) ;

  if (style->mode_data.graph.baseline < 0)
    style->mode_data.graph.baseline = 0 ;
  if (style->mode_data.graph.baseline > 1)
    style->mode_data.graph.baseline = 1 ;
      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* fmax seems only to be used to obtain the final column width in acedb, we can get this from its size... */

  bc->fmax = (bc->width * bc->histBase) + 0.2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  style->mode_data.graph.fields_set.mode = style->mode_data.graph.fields_set.baseline = TRUE ;

  style->fields_set.min_score = style->fields_set.max_score = TRUE ;

  return ;
}




/* Set score bounds for displaying column with width related to score. */
void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score)
{
  zMapAssert(style) ;

  style->min_score = min_score ;
  style->max_score = max_score ;

  style->fields_set.min_score = style->fields_set.max_score = TRUE ;

  return ;
}




/*!
 * \brief sets the end style of exons
 *
 * Controls the end of exons at the moment.  They are either pointy,
 * strand sensitive (when true) or square when false (default) */
void zMapStyleSetEndStyle(ZMapFeatureTypeStyle style, gboolean directional)
{
  zMapAssert(style);
  style->opts.directional_end = directional;
  return;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* NOT USING THIS CURRENTLY, WILL BE NEEDED IN FUTURE... */

/* Score stuff is all quite interdependent, hence this function to set it all up. */
void zMapStyleSetScore(ZMapFeatureTypeStyle style, char *score_str,
		       double min_score, double max_score)
{
  ZMapStyleScoreMode score = ZMAPSCORE_WIDTH ;

  zMapAssert(style) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* WE ONLY SCORE BY WIDTH AT THE MOMENT..... */
  if (bump_str && *bump_str)
    {
      if (g_ascii_strcasecmp(bump_str, "overlap") == 0)
	bump = ZMAPOVERLAP_OVERLAP ;
      else if (g_ascii_strcasecmp(bump_str, "position") == 0)
	bump = ZMAPOVERLAP_POSITION ;
      else if (g_ascii_strcasecmp(bump_str, "name") == 0)
	bump = ZMAPOVERLAP_NAME ;
      else if (g_ascii_strcasecmp(bump_str, "simple") == 0)
	bump = ZMAPOVERLAP_SIMPLE ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  style->score_mode = score ;

  /* Make sure we have some kind of score. */
  if (style->max_score == style->min_score)
    style->max_score = style->min_score + 1 ;

  /* Make sure we have kind of width. */
  if (!(style->width))
    style->width = 2.0 ;

  style->fields_set.score_mode = style->fields_set.min_score = style->fields_set.max_score = TRUE ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */








/* These attributes are not needed for many features and are not independent,
 * hence we set them in a special routine, none of this is very good as we don't have
 * a good way of enforcing stuff...so its all a bit heuristic. */
void zMapStyleSetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean strand_specific, gboolean frame_specific,
			     gboolean show_rev_strand, gboolean show_only_as_3_frame)
{
  zMapAssert(type) ;

  if (frame_specific && !strand_specific)
    strand_specific = TRUE ;

  if (show_rev_strand && !strand_specific)
    strand_specific = TRUE ;

  type->opts.strand_specific = strand_specific ;
  type->opts.frame_specific  = frame_specific ;
  type->opts.show_rev_strand = show_rev_strand ;
  type->opts.show_only_as_3_frame = show_only_as_3_frame ;

  return ;
}


/* These attributes are not needed for many features and are not independent,
 * hence we set them in a special routine, none of this is very good as we don't have
 * a good way of enforcing stuff...so its all a bit heuristic. */
void zMapStyleGetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean *strand_specific, gboolean *frame_specific,
			     gboolean *show_rev_strand, gboolean *show_only_as_3_frame)
{
  zMapAssert(type) ;

  if (strand_specific)
    *strand_specific = type->opts.strand_specific ;
  if (frame_specific)
    *frame_specific = type->opts.frame_specific ;
  if (show_rev_strand )
    *show_rev_strand = type->opts.show_rev_strand ;
  if (show_only_as_3_frame)
    *show_only_as_3_frame = type->opts.show_only_as_3_frame ;

  return ;
}


void zMapStyleSetGFF(ZMapFeatureTypeStyle style, char *gff_source, char *gff_feature)
{
  zMapAssert(style) ;

  if (gff_source && *gff_source)
    {
      style->gff_source = g_quark_from_string(gff_source) ;
      style->fields_set.gff_source = TRUE ;
    }

  if (gff_feature && *gff_feature)
    {
      style->gff_feature = g_quark_from_string(gff_feature) ;
      style->fields_set.gff_feature = TRUE ;
    }

  return ;
}



/* If caller specifies nothing or rubbish then style defaults to complete overlap. */
void zMapStyleSetBump(ZMapFeatureTypeStyle style, char *bump_str)
{
  ZMapStyleOverlapMode bump = ZMAPOVERLAP_COMPLETE ;

  if (bump_str && *bump_str)
    {
      if (g_ascii_strcasecmp(bump_str, "complete") == 0)
	bump = ZMAPOVERLAP_COMPLETE ;
      else if (g_ascii_strcasecmp(bump_str, "smartest") == 0)
	bump = ZMAPOVERLAP_ENDS_RANGE ;
      else if (g_ascii_strcasecmp(bump_str, "smart") == 0)
	bump = ZMAPOVERLAP_NO_INTERLEAVE ;
      else if (g_ascii_strcasecmp(bump_str, "interleave") == 0)
	bump = ZMAPOVERLAP_COMPLEX ;
      else if (g_ascii_strcasecmp(bump_str, "overlap") == 0)
	bump = ZMAPOVERLAP_OVERLAP ;
      else if (g_ascii_strcasecmp(bump_str, "position") == 0)
	bump = ZMAPOVERLAP_POSITION ;
      else if (g_ascii_strcasecmp(bump_str, "name") == 0)
	bump = ZMAPOVERLAP_NAME ;
      else if (g_ascii_strcasecmp(bump_str, "simple") == 0)
	bump = ZMAPOVERLAP_SIMPLE ;
    }

  zMapStyleSetOverlapMode(style, bump) ;

  return ;
}


void zMapStyleSetOverlapMode(ZMapFeatureTypeStyle style, ZMapStyleOverlapMode overlap_mode)
{
  zMapAssert(style && (overlap_mode > ZMAPOVERLAP_START && overlap_mode < ZMAPOVERLAP_END)) ;

  if (!style->fields_set.overlap_mode)
    {
      style->fields_set.overlap_mode = TRUE ;
      
      style->default_overlap_mode = overlap_mode ;
    }

  style->curr_overlap_mode = overlap_mode ;

  return ;
}


/* Reset overlap mode to default and returns the default mode. */
ZMapStyleOverlapMode zMapStyleResetOverlapMode(ZMapFeatureTypeStyle style)
{
  zMapAssert(style && style->fields_set.overlap_mode) ;

  style->curr_overlap_mode = style->default_overlap_mode ;

  return style->curr_overlap_mode ;
}


/* Re/init overlap mode. */
void zMapStyleInitOverlapMode(ZMapFeatureTypeStyle style,
			      ZMapStyleOverlapMode default_overlap_mode, ZMapStyleOverlapMode curr_overlap_mode)
{
  zMapAssert(style
	     && (default_overlap_mode >= ZMAPOVERLAP_START && default_overlap_mode < ZMAPOVERLAP_END)
	     && (curr_overlap_mode >= ZMAPOVERLAP_START && curr_overlap_mode < ZMAPOVERLAP_END)) ;

  if (curr_overlap_mode > ZMAPOVERLAP_START)
    {
      style->fields_set.overlap_mode = TRUE ;
      style->curr_overlap_mode = curr_overlap_mode ;
    }

  if (default_overlap_mode > ZMAPOVERLAP_START)
    {
      style->fields_set.overlap_default = TRUE ;
      style->default_overlap_mode = default_overlap_mode ;
    }
  
  return ;
}


ZMapStyleOverlapMode zMapStyleGetOverlapMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  zMapAssert(style) ;

  mode = style->curr_overlap_mode ;

  return mode;
}

ZMapStyleOverlapMode zMapStyleGetDefaultOverlapMode(ZMapFeatureTypeStyle style)
{
  ZMapStyleOverlapMode mode = ZMAPOVERLAP_COMPLETE;

  zMapAssert(style) ;

  mode = style->default_overlap_mode ;

  return mode;
}



/* Pretty brain dead but we need some way to deal with the situation where a style may differ in
 * case only betweens servers...perhaps we should not be merging....???
 * Caller must free returned string.
 *  */
char *zMapStyleCreateName(char *style)
{
  char *style_name ;
  GString *unique_style_name ;

  unique_style_name = g_string_new(style) ;
  unique_style_name = g_string_ascii_down(unique_style_name) ; /* does it in place */

  style_name = unique_style_name->str ;

  g_string_free(unique_style_name, FALSE) ;		    /* Do not free returned string. */

  return style_name ;
}


/* Like zMapStyleCreateName() but returns a quark representing the style name. */
GQuark zMapStyleCreateID(char *style)
{
  GQuark style_id = 0 ;
  char *style_name ;

  style_name = zMapStyleCreateName(style) ;

  style_id = g_quark_from_string(style_name) ;

  g_free(style_name) ;

  return style_id ;
}


char *zMapStyleGetName(ZMapFeatureTypeStyle style)
{
  char *style_name ;

  style_name = (char *)g_quark_to_string(style->original_id) ;

  return style_name ;
}

gboolean zMapStyleDisplayInSeparator(ZMapFeatureTypeStyle style)
{
  gboolean separator_style = FALSE;

  separator_style = style->opts.show_only_in_separator;
  
  return separator_style;
}


/* Destroy the type, freeing all resources. */
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type)
{
  zMapAssert(type) ;

  if (type->description)
    g_free(type->description) ;

  type->description = "I've been freed !!!" ;

  g_free(type) ;

  return ;
}




gboolean zMapFeatureTypeSetAugment(GData **current, GData **new)
{
  gboolean result = FALSE ;

  zMapAssert(new && *new) ;

  if (!current || !*current)
    g_datalist_init(current) ;
  
  g_datalist_foreach(new, doTypeSets, (void *)current) ;

  result = TRUE ;					    /* currently shouldn't fail.... */

  return result ;
}


/* Check that every name has a style, if the style can't be found, remove the name from the list. */
gboolean zMapSetListEqualStyles(GList **feature_set_names, GList **styles)
{
  gboolean result = FALSE ;
  CheckSetListStruct check_list ;

  check_list.names = feature_set_names ;
  check_list.styles = styles ;
  check_list.any_style_found = FALSE ;

  g_list_foreach(*feature_set_names, checkListName, (gpointer)&check_list) ;

  if (check_list.any_style_found)
    result = TRUE ;

  return result ;
}


/* Merge new_styles into curr_styles. Rules are:
 * 
 * if new_style is not in curr_styles its simply added, otherwise new_style
 * overloads curr_style.
 * 
 *  */
GData *zMapStyleMergeStyles(GData *curr_styles, GData *new_styles)
{
  GData *merged_styles = NULL ;
  MergeStyleCBStruct merge_data = {NULL} ;

  merge_data.curr_styles = curr_styles ;

  g_datalist_foreach(&new_styles, mergeStyle, &merge_data) ;
  
  merged_styles = merge_data.curr_styles ;

  return merged_styles ;
}


/* Returns a GData of all predefined styles, the user should free the list AND the styles when
 * they have finished with them. */
GData *zMapStyleGetAllPredefined(void)
{
  GData *style_list = NULL ;
  ZMapFeatureTypeStyle curr = NULL ;
  static ZMapFeatureTypeStyleStruct predefined_styles[] =
    {
      {0},						    /* 3 Frame */
      {0},						    /* 3 Frame translation */
      {0},						    /* DNA */
      {0},						    /* Locus */
      {0},						    /* Gene Finder */
      {0},                                                  /* Scale bar */
      {0},                                                  /* show translation... */
      {0},                                                  /* strand separator */
      {0},                                                  /* dna match markers */
      {0}						    /* End value. */
    } ;

  curr = &(predefined_styles[0]) ;

  /* init if necessary. */
  if (!(curr->original_id))
    {
      /* 3 Frame */
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_3FRAME_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_META) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;
      zMapStyleSetDisplayable(curr, FALSE) ;


      /* 3 Frame Translation */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_3FT_NAME_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_PEP_SEQUENCE) ;
      zMapStyleSetDisplayable(curr, TRUE) ;
      zMapStyleSetDisplay(curr, ZMAPSTYLE_COLDISPLAY_HIDE) ;

      /* The translation width is the width for the whole column if
       * all three frames are displayed in one column.  When displayed
       * in the frame specfic mode the width of each of the columns
       * will be a third of this whole column value.  This is contrary
       * to all other columns.  This way the translation takes up the
       * same screen space whether it's displayed frame specific or
       * not.  I thought this was important considering the column is
       * very wide. */
      zMapStyleSetWidth(curr, 900.0) ;
      /* Despite seeming to be frame specific, they all get drawn in the same column at the moment so it's not frame specific! */
      zMapStyleSetStrandAttrs(curr, TRUE, TRUE, FALSE, TRUE) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL, "white", "black", NULL) ;
      zMapStyleSetBumpWidth(curr, 10.0) ;


      /* DNA */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_DNA_NAME_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_RAW_SEQUENCE) ;
      zMapStyleSetDisplayable(curr, TRUE) ;
      zMapStyleSetDisplay(curr, ZMAPSTYLE_COLDISPLAY_HIDE) ;
      zMapStyleSetWidth(curr, 300.0) ;
      zMapStyleSetStrandAttrs(curr, TRUE, FALSE, FALSE, FALSE) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL, "white", "black", NULL) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_SELECTED, "light green", "black", NULL) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;

      /* Locus */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_LOCUS_NAME_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_TEXT) ;
      zMapStyleSetDisplayable(curr, TRUE) ;
      zMapStyleSetDisplay(curr, ZMAPSTYLE_COLDISPLAY_HIDE) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL, "white", "black", NULL) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;

      /* GeneFinderFeatures */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_GFF_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_GFF_NAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_GFF_NAME_TEXT);
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_META) ;
      zMapStyleSetDisplayable(curr, FALSE) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;
      
      /* Scale bar */
      curr++;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME);
      curr->unique_id   = zMapStyleCreateID(ZMAP_FIXED_STYLE_SCALE_NAME);
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_SCALE_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_META) ;
      zMapStyleSetDisplayable(curr, FALSE) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;

      /* show translation in zmap */
      curr++;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME);
      curr->unique_id   = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME);
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_SHOWTRANSLATION_TEXT);
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_TEXT) ;
      zMapStyleSetWidth(curr, 300.0) ;
      zMapStyleSetDisplayable(curr, TRUE) ;
      zMapStyleSetDisplay(curr, ZMAPSTYLE_COLDISPLAY_HIDE) ;
      zMapStyleSetStrandAttrs(curr, TRUE, FALSE, FALSE, FALSE) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL, "white", "black", NULL) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_SELECTED, "light green", "black", NULL) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;

      /* strand separator */
      curr++;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_STRAND_SEPARATOR_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_META) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;
      zMapStyleSetDisplayable(curr, FALSE) ;

      /* Search results hits */
      curr++;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME) ;
      zMapStyleSetDescription(curr, ZMAP_FIXED_STYLE_SEARCH_MARKERS_TEXT) ;
      zMapStyleSetMode(curr, ZMAPSTYLE_MODE_BASIC) ;
      zMapStyleInitOverlapMode(curr, ZMAPOVERLAP_COMPLETE, ZMAPOVERLAP_COMPLETE) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL, "red", "black", NULL) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_STRAND, ZMAPSTYLE_COLOURTYPE_NORMAL, "green", "black", NULL) ;
#ifdef MAKE__COLUMN_ITEM_HIGHLIGHT_FROM_CONFIG_WORK
#ifdef SETITEMCOLOUR_CODE
      /* Use default colour if set. */
      if (highlight && !fill_colour && highlight_colour)
	fill_colour = highlight_colour ;
#endif /* SETITEMCOLOUR_CODE */
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_SELECTED, "pink", "black", NULL) ;
      zMapStyleSetColours(curr, ZMAPSTYLE_COLOURTARGET_STRAND, ZMAPSTYLE_COLOURTYPE_SELECTED, "light green", "black", NULL) ;
#endif /* MAKE__COLUMN_ITEM_HIGHLIGHT_FROM_CONFIG_WORK */
      zMapStyleSetDisplayable(curr, TRUE) ;
      zMapStyleSetWidth(curr, 15.0) ;
      curr->opts.show_only_in_separator = TRUE;
    }

  curr = &(predefined_styles[0]) ;
  while ((curr->original_id))
    {
      ZMapFeatureTypeStyle style ;

      style = zMapFeatureStyleCopy(curr) ;

      g_datalist_id_set_data(&style_list, style->unique_id, style) ;

      curr++ ;
    }

  return style_list ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* NOT SURE WE NEED THIS AT THE MOMENT.....if will need to take a list of predefined ones as a param. */
ZMapFeatureTypeStyle zMapStyleGetPredefined(char *style_name)
{
  ZMapFeatureTypeStyle style = NULL, curr = NULL ;

  style_id = zMapStyleCreateID(style_name) ;
  curr = &(predefined_styles[0]) ;
  while ((curr->original_id))
    {
      if (style_id == curr->unique_id)
	{
	  style = curr ;
	  break ;
	}
      else
	curr++ ;
    }

  return style ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* need a func to free a styles list here..... */
void zMapStyleDestroyStyles(GData **styles)
{
  g_datalist_foreach(styles, destroyStyle, NULL) ;

  g_datalist_clear(styles) ;

  return ;
}







/* Read the type/method/source (call it what you will) information from the given file
 * which currently must reside in the users $HOME/.ZMap directory. */
GData *zMapFeatureTypeGetFromFile(char *styles_file_name)
{
  GData *styles = NULL ;
  gboolean result = FALSE ;
  ZMapConfigStanzaSet styles_list = NULL ;
  ZMapConfig config ;


  if ((config = zMapConfigCreateFromFile(styles_file_name)))
    {
      ZMapConfigStanza styles_stanza ;
      ZMapConfigStanzaElementStruct styles_elements[]
	= {{"name"        , ZMAPCONFIG_STRING, {NULL}},
	   {"description" , ZMAPCONFIG_STRING, {NULL}},
	   {"outline"     , ZMAPCONFIG_STRING, {NULL}},
	   {"mode"        , ZMAPCONFIG_STRING, {NULL}},
	   {"foreground"  , ZMAPCONFIG_STRING, {NULL}},
	   {"background"  , ZMAPCONFIG_STRING, {NULL}},
	   {"width"       , ZMAPCONFIG_FLOAT , {NULL}},
	   {"strand_specific", ZMAPCONFIG_BOOL, {NULL}},
	   {"frame_specific",  ZMAPCONFIG_BOOL, {NULL}},
	   {"show_reverse", ZMAPCONFIG_BOOL  , {NULL}},
	   {"show_only_as_3_frame", ZMAPCONFIG_BOOL  , {NULL}},
	   {"minmag"      , ZMAPCONFIG_INT, {NULL}},
	   {"maxmag"      , ZMAPCONFIG_INT, {NULL}},
	   {"bump"        , ZMAPCONFIG_STRING, {NULL}},
	   {"bump_spacing"  , ZMAPCONFIG_FLOAT , {NULL}},
	   {"gapped_align", ZMAPCONFIG_BOOL, {NULL}},
	   {"gapped_error", ZMAPCONFIG_INT, {NULL}},
	   {"read_gaps"   , ZMAPCONFIG_BOOL, {NULL}},
	   {"directional_end", ZMAPCONFIG_BOOL, {NULL}},
           {"hidden_init", ZMAPCONFIG_BOOL, {NULL}},
	   {NULL, -1, {NULL}}} ;

      /* Init fields that cannot default to string NULL. */
      zMapConfigGetStructFloat(styles_elements, "width") = ZMAPFEATURE_DEFAULT_WIDTH ;

      styles_stanza = zMapConfigMakeStanza("Type", styles_elements) ;

      if (!zMapConfigFindStanzas(config, styles_stanza, &styles_list))
	result = FALSE ;
      else
	result = TRUE ;
    }

  /* Set up connections to the named styles. */
  if (result)
    {
      int num_styles = 0 ;
      ZMapConfigStanza next_styles ;

      styles = NULL ;

      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      next_styles = NULL ;
      while (result
	     && ((next_styles = zMapConfigGetNextStanza(styles_list, next_styles)) != NULL))
	{
	  char *name ;

	  /* Name must be set so if its not found then don't make a struct.... */
	  if ((name = zMapConfigGetElementString(next_styles, "name")))
	    {
	      ZMapFeatureTypeStyle new_type ;
#ifdef RDS_DONT_INCLUDE
	      gboolean strand_specific, frame_specific, show_rev_strand ;
	      int min_mag, max_mag ;
#endif /* RDS_DONT_INCLUDE */
	      ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;

	      
	      if (!zMapStyleFormatMode(zMapConfigGetElementString(next_styles, "description"), &mode))
		zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			       "the stanza has been ignored.", styles_file_name) ;


	      new_type = zMapFeatureTypeCreate(name,
					       zMapConfigGetElementString(next_styles, "description")) ;

	      zMapStyleSetMode(new_type, mode) ;

	      zMapStyleSetColours(new_type, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  zMapConfigGetElementString(next_styles, "foreground"),
				  zMapConfigGetElementString(next_styles, "background"),
				  zMapConfigGetElementString(next_styles, "outline")) ;

	      zMapStyleSetWidth(new_type, zMapConfigGetElementFloat(next_styles, "width")) ;

	      zMapStyleSetMag(new_type,
			      zMapConfigGetElementInt(next_styles, "minmag"),
			      zMapConfigGetElementInt(next_styles, "maxmag")) ;

	      zMapStyleSetStrandAttrs(new_type,
				      zMapConfigGetElementBool(next_styles, "strand_specific"),
				      zMapConfigGetElementBool(next_styles, "frame_specific"),
				      zMapConfigGetElementBool(next_styles, "show_reverse"),
				      zMapConfigGetElementBool(next_styles, "show_only_as_3_frame")) ;

	      zMapStyleSetBump(new_type, zMapConfigGetElementString(next_styles, "bump")) ;
	      zMapStyleSetBumpWidth(new_type, zMapConfigGetElementFloat(next_styles, "bump_spacing")) ;

              /* Not good to hard code the TRUE here, but I guess blixem requires the gaps array. */
              zMapStyleSetGappedAligns(new_type, 
                                       zMapConfigGetElementBool(next_styles, "gapped_align"),
				       zMapConfigGetElementBool(next_styles, "gapped_error")) ;

	      if (zMapConfigGetElementBool(next_styles, "hidden_init"))
		zMapStyleSetDisplay(new_type, ZMAPSTYLE_COLDISPLAY_HIDE) ;

              zMapStyleSetEndStyle(new_type, zMapConfigGetElementBool(next_styles, "directional_end"));

	      g_datalist_id_set_data(&styles, new_type->unique_id, new_type) ;

	      num_styles++ ;
	    }
	  else
	    {
	      zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			     "the stanza has been ignored.", styles_file_name) ;
	    }
	}

      /* Found no valid styles.... */
      if (!num_styles)
	result = FALSE ;
    }

  /* clean up. */
  if (styles_list)
    zMapConfigDeleteStanzaSet(styles_list) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug.... */
  printAllTypes(styles, "getTypesFromFile()") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return styles ;
}


void zMapFeatureTypePrintAll(GData *type_set, char *user_string)
{
  printf("\nTypes at %s\n", user_string) ;

  g_datalist_foreach(&type_set, typePrintFunc, NULL) ;

  return ;
}


void zMapFeatureStylePrintAll(GList *styles, char *user_string)
{
  printf("\nTypes at %s\n", user_string) ;

  g_list_foreach(styles, stylePrintFunc, NULL) ;

  return ;
}





/*! @} end of zmapstyles docs. */






/* 
 *                  Internal routines
 */



static void doTypeSets(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle new_type = (ZMapFeatureTypeStyle)data ;
  GData **current_out = (GData **)user_data ;
  GData *current = *current_out ;

  if (!g_datalist_id_get_data(&current, key_id))
    {
      /* copy the struct and then add it to the current set. */
      ZMapFeatureTypeStyle type ;

      type = zMapFeatureStyleCopy(new_type) ;

      g_datalist_id_set_data(&current, key_id, type) ;
    }

  *current_out = current ;

  return ;
}


static void typePrintFunc(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapStylePrint(style, "Style:") ;

  return ;
}


static void stylePrintFunc(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapStylePrint(style, "Style:") ;

  return ;
}


/* A GFunc() called from zMapSetListEqualStyles() to check that a given feature_set name
 * has a corresponding style. */
static void checkListName(gpointer data, gpointer user_data)
{
  CheckSetList check_list = (CheckSetList)user_data ;
  GList *style_item ;
  ZMapFeatureTypeStyle style ;

  if ((style_item = g_list_find_custom(*(check_list->styles), data, compareNameToStyle)))
    {
      style = (ZMapFeatureTypeStyle)(style_item->data) ;
      check_list->any_style_found = TRUE ;
    }
  else
    *(check_list->names) = g_list_remove(*(check_list->names), data) ;

  return ;
}


/* GCompareFunc () calling the given function which should return 0 when the desired element is found. 
   The function takes two gconstpointer arguments, the GList element's data and the given user data.*/
static gint compareNameToStyle(gconstpointer glist_data, gconstpointer user_data)
{
  gint result = -1 ;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)glist_data ;
  GQuark set_name = GPOINTER_TO_INT(user_data) ;  

  if (set_name == style->unique_id)
    result = 0 ;

  return result ;
}



/* Either merges a new style or adds it to current list. */
static void mergeStyle(GQuark style_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle new_style = (ZMapFeatureTypeStyle)data ;
  MergeStyleCB merge_data = (MergeStyleCB)user_data ;
  GData *curr_styles = merge_data->curr_styles ;
  ZMapFeatureTypeStyle curr_style = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (zMapStyleNameCompare(new_style, "genomic_canonical") || zMapStyleNameCompare(new_style, "orfeome"))
    printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* If we find the style then merge it, if not then add a copy to the curr_styles. */
  if ((curr_style = zMapFindStyle(curr_styles, new_style->unique_id)))
    {
      zMapStyleMerge(curr_style, new_style) ;
    }
  else
    {
      g_datalist_id_set_data(&curr_styles, new_style->unique_id, zMapFeatureStyleCopy(new_style)) ;

      merge_data->curr_styles = curr_styles ;
    }

  return ;
}



/* Destroy the given style. */
static void destroyStyle(GQuark style_id, gpointer data, gpointer user_data_unused)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapFeatureTypeDestroy(style) ;

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapFeatureTypeStyle createInheritedStyle(GData *style_set, char *parent_style)

{
  ZMapFeatureTypeStyle inherited_style = NULL ;
  GQuark parent_id ;

  if ((parent_id = zMapStyleCreateID(parent_style)))
    {
      GQueue *style_queue ;
      gboolean parent, error ;
      ZMapFeatureTypeStyle curr_style ;
      GQuark curr_id ;

      style_queue = g_queue_new() ;

      curr_id = parent_id ;
      parent = TRUE ;
      error = FALSE ;
      do
	{
	  if ((curr_style = zMapFindStyle(style_set, curr_id)))
	    {
	      g_queue_push_head(style_queue, curr_style) ;

	      if (!curr_style->fields_set.parent_style)
		parent = FALSE ;
	      else
		curr_id = curr_style->parent_id ;
	    }
	  else
	    {
	      error = TRUE ;
	    }
	} while (parent && !error) ;

      if (!error && !g_queue_is_empty(style_queue))
	{
	  InheritStyleCBStruct new_style = {FALSE, NULL} ;

	  g_queue_foreach(style_queue, inheritStyleCB, &new_style) ;

	  if (!(new_style.error))
	    inherited_style = new_style.inherited_style ;
	}

      g_queue_free(style_queue) ;			    /* We only hold pointers here so no
							       data to free. */
    }

  return inherited_style ;
}


/* A GFunc to merge styles.
 * 
 * Note that if there is an error at any stage in processing the styles then we return NULL. */
static void inheritStyleCB(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle child_style = (ZMapFeatureTypeStyle)data ;
  InheritStyleCB inherited = (InheritStyleCB)user_data ;

  if (!(inherited->inherited_style))
    {
      inherited->inherited_style = child_style ;
    }
  else
    {
      ZMapFeatureTypeStyle curr_style = inherited->inherited_style ;

      if (zMapStyleMerge(curr_style, child_style))
	inherited->inherited_style = curr_style ;
      else
	{
	  zMapFeatureTypeDestroy(curr_style) ;
	  inherited->inherited_style = NULL ;
	  inherited->error = TRUE ;
	}
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* Functions to sort out the inheritance of styles by copying and overloading. */

/* A GDataForeachFunc() to ..... */
static void inheritCB(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle curr_style = (ZMapFeatureTypeStyle)data ;
  InheritAllCB cb_data = (InheritAllCB)user_data ;

  zMapAssert(key_id == curr_style->unique_id) ;

  /* If we haven't done this style yet then we need to do its inheritance. */
  if (!(g_datalist_id_get_data(&(cb_data->inherited_styles), key_id)))
    {
      if (!doStyleInheritance(&(cb_data->style_set), &(cb_data->inherited_styles), curr_style))
	{
	  cb_data->errors = TRUE ;
	}

      /* record that we have now done this style. */
      g_datalist_id_set_data(&(cb_data->inherited_styles), key_id, curr_style) ;
    }

  return ;
}



static gboolean doStyleInheritance(GData **style_set_inout, GData **inherited_styles_inout,
				   ZMapFeatureTypeStyle style)
{
  gboolean result = TRUE ;
  GData *style_set, *inherited_styles ;
  GQueue *style_queue ;
  gboolean parent ;
  ZMapFeatureTypeStyle curr_style ;
  GQuark prev_id, curr_id ;

  style_set = *style_set_inout ;
  inherited_styles = *inherited_styles_inout ;

  style_queue = g_queue_new() ;

  prev_id = curr_id = style->unique_id ;
  parent = TRUE ;
  do
    {
      if ((curr_style = zMapFindStyle(style_set, curr_id)))
	{
	  g_queue_push_head(style_queue, curr_style) ;

	  if (!curr_style->fields_set.parent_style)
	    parent = FALSE ;
	  else
	    {
	      prev_id = curr_id ;
	      curr_id = curr_style->parent_id ;
	    }
	}
      else
	{
	  result = FALSE ;

	  zMapLogWarning("Style \"%s\" has the the parent style \"%s\" "
			 "but the latter cannot be found in the styles set so cannot be inherited.",
			 g_quark_to_string(prev_id), g_quark_to_string(curr_id)) ;
	}
    } while (parent && result) ;

  /* If we only recorded the current style in our inheritance queue then there is no inheritance tree
   * so no need to do the inheritance. */
  if (result && g_queue_get_length(style_queue) > 1)
    {
      InheritAllCBStruct new_style = {FALSE} ;

      new_style.style_set = style_set ;
      new_style.inherited_styles = inherited_styles ;
      g_queue_foreach(style_queue, inheritAllFunc, &new_style) ;
      style_set = new_style.style_set ;
      inherited_styles = new_style.inherited_styles ;

      result = !new_style.errors ;
    }

  g_queue_free(style_queue) ;			    /* We only hold pointers here so no
						       data to free. */

  *style_set_inout = style_set ;
  *inherited_styles_inout = inherited_styles ;

  return result ;
}


/* A GFunc to take a style and replace it with one inherited from its parent.
 * 
 * Note that if there is an error at any stage in processing the styles then we return NULL. */
static void inheritAllFunc(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle curr_style = (ZMapFeatureTypeStyle)data ;
  InheritAllCB inherited = (InheritAllCB)user_data ;

  if (!inherited->errors)
    {
      ZMapFeatureTypeStyle prev_style = inherited->prev_style, tmp_style ;

      if (!prev_style)
	{
	  inherited->prev_style = curr_style ;
	}
      else
	{
	  tmp_style = zMapFeatureStyleCopy(prev_style) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMapStylePrint(tmp_style, "Parent") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



	  if (zMapStyleMerge(tmp_style, curr_style))
	    {


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      zMapStylePrint(tmp_style, "child") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



	      inherited->prev_style = tmp_style ;
	      
	      /* The g_datalist call overwrites the old style reference with the new one, we then
	       * delete the old one. */
	      g_datalist_id_set_data(&(inherited->style_set), tmp_style->unique_id, tmp_style) ;
	      tmp_style = curr_style ;			    /* Make sure we destroy the old style. */

	    }
	  else
	    {
	      inherited->errors = TRUE ;

	      zMapLogWarning("Merge of style \"%s\" into style \"%s\" failed.",
			     g_quark_to_string(curr_style->original_id), g_quark_to_string(tmp_style->original_id)) ;
	    }

	  zMapFeatureTypeDestroy(tmp_style) ;
	}
    }

  return ;
}


