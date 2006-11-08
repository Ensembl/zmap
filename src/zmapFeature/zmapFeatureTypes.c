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
 * Last edited: Nov  7 17:27 2006 (edgrif)
 * Created: Tue Dec 14 13:15:11 2004 (edgrif)
 * CVS info:   $Id: zmapFeatureTypes.c,v 1.35 2006-11-08 11:55:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>
#include <zmapFeature_P.h>


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
  GList *curr_styles ;

} MergeStyleCBStruct, *MergeStyleCB ;



static void doTypeSets(GQuark key_id, gpointer data, gpointer user_data) ;
static void typePrintFunc(GQuark key_id, gpointer data, gpointer user_data) ;

static void stylePrintFunc(gpointer data, gpointer user_data) ;

static void checkListName(gpointer data, gpointer user_data) ;
static gint compareNameToStyle(gconstpointer glist_data, gconstpointer user_data) ;

static void mergeStyle(gpointer data, gpointer user_data_unused) ;
static void destroyStyle(gpointer data, gpointer user_data_unused) ;


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







/* Create a new type for displaying features. */
ZMapFeatureTypeStyle zMapFeatureTypeCreate(char *name, char *description, ZMapStyleMode mode,
					   char *outline, char *foreground, char *background,
					   double width)
{
  ZMapFeatureTypeStyle new_type = NULL ;
  char *name_lower ;

  /* I am unsure if this is a good thing to do here, I'm attempting to make sure the type is
   * "sane" but no more than that. */
  zMapAssert(name && *name && width >= 0.0) ;

  /* Is this overridden by acedb stuff ?? Should rationalise all this..... */
  if (width == 0)
    width = 5 ;

  new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

  name_lower = g_ascii_strdown(name, -1) ;
  new_type->original_id = g_quark_from_string(name) ;
  new_type->unique_id = zMapStyleCreateID(name_lower) ;
  g_free(name_lower) ;

  if (description)
    new_type->description = g_strdup(description) ;

  new_type->mode = mode ;

  new_type->width = width ;

  zMapStyleSetColours(new_type, outline, foreground, background);

  /* By default we always parse homology gaps, important for stuff like passing this
   * information to blixem. */
  new_type->opts.parse_gaps = TRUE ;

  new_type->overlap_mode = ZMAPOVERLAP_COMPLETE ;

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
 * Merge one style into another. Values in curr_style are overwritten with those
 * in the new_style. new_style is not altered.
 * 
 * Returns TRUE if merege ok, FALSE if there was a problem, e.g. different style names.
 * 
 * @param   curr_style          The style to be overwritten.
 * @param   new_style           The style to used for overwriting.
 * @return  gboolean            TRUE means successful merge.
 *  */
gboolean zMapStyleMerge(ZMapFeatureTypeStyle curr_style, ZMapFeatureTypeStyle new_style)
{
  gboolean result = FALSE ;

  zMapAssert(curr_style && new_style) ;

  if (new_style->unique_id == curr_style->unique_id)
    {
      /* You can't just do a simple struct copy here so we do it by steam.... */
      if (new_style->description)
	{
	  g_free(curr_style->description) ;
	  curr_style->description = g_strdup(new_style->description) ;
	}

      curr_style->mode = new_style->mode ;

      curr_style->opts = new_style->opts ;		    /* struct copy ok here I think.... */

      if (new_style->colours.background_set)
	curr_style->colours.background = new_style->colours.background ;
      if (new_style->colours.foreground_set)
	curr_style->colours.foreground = new_style->colours.foreground ;
      if (new_style->colours.outline_set)
	curr_style->colours.outline = new_style->colours.outline ;

      curr_style->overlap_mode = new_style->overlap_mode ;
      curr_style->min_mag = new_style->min_mag ;
      curr_style->max_mag = new_style->max_mag ;
      curr_style->width = new_style->width ;
      curr_style->bump_width = new_style->bump_width ;
      curr_style->score_mode = new_style->score_mode ;
      curr_style->min_score = new_style->min_score ;
      curr_style->max_score = new_style->max_score ;
      curr_style->gff_source = new_style->gff_source ;
      curr_style->gff_feature = new_style->gff_feature ;
    }

  return result ;
}






void zMapStyleSetColours(ZMapFeatureTypeStyle style, 
                         char *outline, 
                         char *foreground, 
                         char *background)
{
  zMapAssert(style);

  /* Set some default colours.... */
  if (!outline)
    outline = "black" ;
#ifdef RDS_NO_DEFAULT_COLOURS
  if (!foreground)
    foreground = "white" ;
  if (!background)
    background = "white" ;
#endif

  if(outline && *outline)
    {
      gdk_color_parse(outline, &style->colours.outline) ;
      style->colours.outline_set = TRUE;
    }
  if(foreground && *foreground)
    {
      gdk_color_parse(foreground, &style->colours.foreground) ;
      style->colours.foreground_set = TRUE;
    }
  if(background && *background)
    {
      gdk_color_parse(background, &style->colours.background) ;
      style->colours.background_set = TRUE;
    }
  
  return ;
}

/* Set magnification limits for displaying columns. */
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag)
{
  zMapAssert(style) ;

  if (min_mag && min_mag > 0.0)
    style->min_mag = min_mag ;

  if (max_mag && max_mag > 0.0)
    style->max_mag = max_mag ;

  return ;
}



void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode)
{
  zMapAssert(style
	     && (mode >= ZMAPSTYLE_MODE_NONE || mode <= ZMAPSTYLE_MODE_TEXT)) ;

  style->mode = mode ;

  return ;
}



gboolean zMapStyleFormatMode(char *mode_str, ZMapStyleMode *mode_out)
{
  gboolean result = FALSE ;
  ZMapFeatureStr2EnumStruct mode_table [] = {{"NONE", ZMAPSTYLE_MODE_NONE},
					     {"BASIC", ZMAPSTYLE_MODE_BASIC},
					     {"TRANSCRIPT", ZMAPSTYLE_MODE_TRANSCRIPT},
					     {"ALIGNMENT", ZMAPSTYLE_MODE_ALIGNMENT},
					     {"TEXT", ZMAPSTYLE_MODE_TEXT},
					     {NULL, 0}} ;

  zMapAssert(mode_str && *mode_str && mode_out) ;

  result = zmapStr2Enum(&(mode_table[0]), mode_str, (int *)mode_out) ;

  return result ;
}


/* Set score bounds for displaying column with width related to score. */
void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score)
{
  zMapAssert(style) ;

  style->min_score = min_score ;

  style->max_score = max_score ;

  return ;
}


void zMapStyleSetHide(ZMapFeatureTypeStyle style, gboolean hide)
{
  zMapAssert(style) ;

  style->opts.hide_always = hide ;

  return ;
}



/* Controls whether the feature set is displayed initially. */
void zMapStyleSetHideInitial(ZMapFeatureTypeStyle style, gboolean hide_initially)
{
  zMapAssert(style) ;

  style->opts.hide_initially = hide_initially ;

  return ;
}

gboolean zMapStyleGetHideInitial(ZMapFeatureTypeStyle style)
{
  zMapAssert(style) ;

  return style->opts.hide_initially ;
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
    style->gff_source = g_quark_from_string(gff_source) ;

  if (gff_feature && *gff_feature)
    style->gff_feature = g_quark_from_string(gff_feature) ;

  return ;
}



/* If caller specifies nothing or rubbish then style defaults to complete overlap. */
void zMapStyleSetBump(ZMapFeatureTypeStyle style, char *bump_str)
{
  ZMapStyleOverlapMode bump = ZMAPOVERLAP_COMPLETE ;

  zMapAssert(style) ;

  if (bump_str && *bump_str)
    {
      if (g_ascii_strcasecmp(bump_str, "smart") == 0)
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

  style->overlap_mode = bump ;

  return ;
}



void zMapStyleSetOverlapMode(ZMapFeatureTypeStyle style, ZMapStyleOverlapMode overlap_mode)
{
  zMapAssert(overlap_mode > ZMAPOVERLAP_START && overlap_mode < ZMAPOVERLAP_END) ;

  style->overlap_mode = overlap_mode ;

  return ;
}


ZMapStyleOverlapMode zMapStyleGetOverlapMode(ZMapFeatureTypeStyle style)
{
  return style->overlap_mode ;
}




void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean show_gaps, gboolean parse_gaps,
			      unsigned int within_align_error)
{
  zMapAssert(style);

  style->opts.align_gaps = show_gaps ;
  style->opts.parse_gaps = parse_gaps ;
  style->within_align_error = within_align_error ;

  return ;
}


void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, gboolean join_aligns, unsigned int between_align_error)
{
  zMapAssert(style);

  style->opts.join_aligns = join_aligns ;
  style->between_align_error = between_align_error ;

  return ;
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




/* Destroy the type, freeing all resources. */
void zMapFeatureTypeDestroy(ZMapFeatureTypeStyle type)
{
  zMapAssert(type) ;

  if (type->description)
    g_free(type->description) ;

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
GList *zMapStyleMergeStyles(GList *curr_styles, GList *new_styles)
{
  GList *merged_styles = NULL ;
  MergeStyleCBStruct merge_data = {NULL} ;

  merge_data.curr_styles = curr_styles ;

  g_list_foreach(new_styles, mergeStyle, &merge_data) ;
  
  merged_styles = merge_data.curr_styles ;

  return merged_styles ;
}


/* Returns a Glist of all predefined styles, the user should free the list AND the styles when
 * they have finished with them. */
GList *zMapStyleGetAllPredefined(void)
{
  GList *style_list = NULL ;
  ZMapFeatureTypeStyle curr = NULL ;
  static ZMapFeatureTypeStyleStruct predefined_styles[] =
    {
      {0},						    /* 3 Frame */
      {0},						    /* 3 Frame translation */
      {0},						    /* DNA */
      {0},						    /* Locus */
      {0},						    /* Gene Finder */
      {0},                                                  /* Scale bar */
      {0}						    /* End value. */
    } ;

  curr = &(predefined_styles[0]) ;

  /* init if necessary. */
  if (!(curr->original_id))
    {
      /* 3 Frame */
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME) ;
      curr->description = ZMAP_FIXED_STYLE_3FRAME_TEXT ;
      curr->opts.hide_always = TRUE ;
      curr->overlap_mode = ZMAPOVERLAP_COMPLETE ;

      /* 3 Frame Translation */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;
      curr->description = ZMAP_FIXED_STYLE_3FT_NAME_TEXT ;
      curr->opts.hide_initially = TRUE ;
      zMapStyleSetStrandAttrs(curr, TRUE, TRUE, FALSE, TRUE) ;
      curr->overlap_mode = ZMAPOVERLAP_COMPLETE ;
      
      /* DNA */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;
      curr->description = ZMAP_FIXED_STYLE_DNA_NAME_TEXT ;
      curr->opts.hide_initially = TRUE ;
      curr->width = 10.0 ;
      zMapStyleSetStrandAttrs(curr, TRUE, FALSE, FALSE, FALSE) ;
      zMapStyleSetColours(curr, NULL, "black", "white") ;
      curr->overlap_mode = ZMAPOVERLAP_COMPLETE ;

      /* Locus */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME) ;
      curr->description = ZMAP_FIXED_STYLE_LOCUS_NAME_TEXT ;
      curr->opts.hide_initially = TRUE ;
      curr->overlap_mode = ZMAPOVERLAP_COMPLETE ;


      /* GeneFinderFeatures */
      curr++ ;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_GFF_NAME) ;
      curr->unique_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_GFF_NAME) ;
      curr->description = ZMAP_FIXED_STYLE_GFF_NAME_TEXT ;
      curr->opts.hide_always = TRUE ;
      curr->overlap_mode = ZMAPOVERLAP_COMPLETE ;
      
      /* Scale bar */
      curr++;
      curr->original_id = g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME);
      curr->unique_id   = zMapStyleCreateID(ZMAP_FIXED_STYLE_SCALE_NAME);
      curr->description = ZMAP_FIXED_STYLE_SCALE_TEXT;
      curr->opts.hide_always = TRUE;
      curr->overlap_mode     = ZMAPOVERLAP_COMPLETE ;
    }

  curr = &(predefined_styles[0]) ;
  while ((curr->original_id))
    {
      ZMapFeatureTypeStyle style ;

      style = zMapFeatureStyleCopy(curr) ;

      style_list = g_list_append(style_list, style) ;

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
void zMapStyleDestroyStyles(GList *styles)
{

  g_list_foreach(styles, destroyStyle, NULL) ;

  g_list_free(styles) ;

  return ;
}







/* Read the type/method/source (call it what you will) information from the given file
 * which currently must reside in the users $HOME/.ZMap directory. */
GList *zMapFeatureTypeGetFromFile(char *types_file_name)
{
  GList *types = NULL ;
  gboolean result = FALSE ;
  ZMapConfigStanzaSet types_list = NULL ;
  ZMapConfig config ;


  if ((config = zMapConfigCreateFromFile(types_file_name)))
    {
      ZMapConfigStanza types_stanza ;
      ZMapConfigStanzaElementStruct types_elements[]
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
	   {"gapped_align", ZMAPCONFIG_BOOL, {NULL}},
	   {"gapped_error", ZMAPCONFIG_INT, {NULL}},
	   {"read_gaps"   , ZMAPCONFIG_BOOL, {NULL}},
	   {"directional_end", ZMAPCONFIG_BOOL, {NULL}},
           {"hide_initially", ZMAPCONFIG_BOOL, {NULL}},
	   {NULL, -1, {NULL}}} ;

      /* Init fields that cannot default to string NULL. */
      zMapConfigGetStructFloat(types_elements, "width") = ZMAPFEATURE_DEFAULT_WIDTH ;

      types_stanza = zMapConfigMakeStanza("Type", types_elements) ;

      if (!zMapConfigFindStanzas(config, types_stanza, &types_list))
	result = FALSE ;
      else
	result = TRUE ;
    }

  /* Set up connections to the named types. */
  if (result)
    {
      int num_types = 0 ;
      ZMapConfigStanza next_types ;

      types = NULL ;

      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      next_types = NULL ;
      while (result
	     && ((next_types = zMapConfigGetNextStanza(types_list, next_types)) != NULL))
	{
	  char *name ;

	  /* Name must be set so if its not found then don't make a struct.... */
	  if ((name = zMapConfigGetElementString(next_types, "name")))
	    {
	      ZMapFeatureTypeStyle new_type ;
#ifdef RDS_DONT_INCLUDE
	      gboolean strand_specific, frame_specific, show_rev_strand ;
	      int min_mag, max_mag ;
#endif /* RDS_DONT_INCLUDE */
	      ZMapStyleMode mode = ZMAPSTYLE_MODE_NONE ;

	      
	      if (!zMapStyleFormatMode(zMapConfigGetElementString(next_types, "description"), &mode))
		zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			       "the stanza has been ignored.", types_file_name) ;

	      new_type = zMapFeatureTypeCreate(name,
					       zMapConfigGetElementString(next_types, "description"),
					       mode,
					       zMapConfigGetElementString(next_types, "outline"),
					       zMapConfigGetElementString(next_types, "foreground"),
					       zMapConfigGetElementString(next_types, "background"),
					       zMapConfigGetElementFloat(next_types, "width")) ;

	      zMapStyleSetMag(new_type,
			      zMapConfigGetElementInt(next_types, "minmag"),
			      zMapConfigGetElementInt(next_types, "maxmag")) ;

	      zMapStyleSetStrandAttrs(new_type,
				      zMapConfigGetElementBool(next_types, "strand_specific"),
				      zMapConfigGetElementBool(next_types, "frame_specific"),
				      zMapConfigGetElementBool(next_types, "show_reverse"),
				      zMapConfigGetElementBool(next_types, "show_only_as_3_frame")) ;

	      zMapStyleSetBump(new_type, zMapConfigGetElementString(next_types, "bump")) ;

              /* Not good to hard code the TRUE here, but I guess blixem requires the gaps array. */
              zMapStyleSetGappedAligns(new_type, 
                                       zMapConfigGetElementBool(next_types, "gapped_align"),
                                       TRUE,
				       zMapConfigGetElementBool(next_types, "gapped_error")) ;
              zMapStyleSetHideInitial(new_type, zMapConfigGetElementBool(next_types, "hide_initially"));
              zMapStyleSetEndStyle(new_type, zMapConfigGetElementBool(next_types, "directional_end"));
	      types = g_list_append(types, new_type) ;

	      num_types++ ;
	    }
	  else
	    {
	      zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			     "the stanza has been ignored.", types_file_name) ;
	    }
	}

      /* Found no valid types.... */
      if (!num_types)
	result = FALSE ;
    }

  /* clean up. */
  if (types_list)
    zMapConfigDeleteStanzaSet(types_list) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug.... */
  printAllTypes(types, "getTypesFromFile()") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return types ;
}


void zMapFeatureTypeGetColours(ZMapFeatureTypeStyle style, 
                               GdkColor **background, 
                               GdkColor **foreground, 
                               GdkColor **outline)
{
  if(background)
    {
      if(style->colours.background_set)
        *background = &(style->colours.background);
      else
        *background = NULL;
    }

  if(foreground)
    {
      if(style->colours.foreground_set)
        *foreground = &(style->colours.foreground);
      else
        *foreground = NULL;
    }

  if(outline)
    {
      if(style->colours.outline_set)
        *outline = &(style->colours.outline);
      else
        *outline = NULL;
    }

  return ;
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
  char *style_name = (char *)g_quark_to_string(key_id) ;
  
  printf("\t%s: \t%f \t%s\n", style_name, style->width,
	 (style->opts.show_rev_strand ? "show_rev_strand" : "!show_rev_strand")) ;

  return ;
}


static void stylePrintFunc(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  char *style_name = (char *)g_quark_to_string(style->original_id) ;
  
  printf("\t%s: \t%f \t%s\n", style_name, style->width,
	 (style->opts.show_rev_strand ? "show_rev_strand" : "!show_rev_strand")) ;

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
static void mergeStyle(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle new_style = (ZMapFeatureTypeStyle)data ;
  MergeStyleCB merge_data = (MergeStyleCB)user_data ;
  GList *curr_styles = merge_data->curr_styles ;
  ZMapFeatureTypeStyle curr_style = NULL ;

  /* If we find the style then merge it, if not then add a copy to the curr_styles. */
  if ((curr_style = zMapFindStyle(curr_styles, new_style->unique_id)))
    {
      zMapStyleMerge(curr_style, new_style) ;
    }
  else
    {
      curr_styles = g_list_append(curr_styles, zMapFeatureStyleCopy(new_style)) ;
    }

  return ;
}



/* Destroy the given style. */
static void destroyStyle(gpointer data, gpointer user_data_unused)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapFeatureTypeDestroy(style) ;

  return ;
}
