/*  File: zmapFeatureTypes.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions for manipulating Type structs and sets of
 *              type structs.c
 *
 * Exported functions: See ZMap/zmapFeature.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdio.h>
#include <memory.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsDebug.h>
#include <ZMap/zmapGLibUtils.h>

/* This should go in the end..... */
//#include <zmapFeature_P.h>
#include <ZMap/zmapConfigStyleDefaults.h>

#include <zmapStyle_I.h>


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
  ZMapStyleMergeMode merge_mode ;

  GHashTable *curr_styles ;
} MergeStyleCBStruct, *MergeStyleCB ;


typedef struct
{
  gboolean *error ;
  GHashTable *copy_set ;
} CopyStyleCBStruct, *CopyStyleCB ;


/*
typedef struct
{
  gboolean error ;
  ZMapFeatureTypeStyle inherited_style ;
} InheritStyleCBStruct, *InheritStyleCB ;


typedef struct
{
  gboolean errors ;
  GhashTable *style_set ;
  GhashTable *inherited_styles ;
  ZMapFeatureTypeStyle prev_style ;
} InheritAllCBStruct, *InheritAllCB ;

*/


static void checkListName(gpointer data, gpointer user_data) ;
static gint compareNameToStyle(gconstpointer glist_data, gconstpointer user_data) ;

static void mergeColours(ZMapStyleFullColour curr, ZMapStyleFullColour new) ;
static void mergeStyle(gpointer style_id, gpointer data, gpointer user_data_unused) ;
static gboolean destroyStyle(gpointer style_id, gpointer data, gpointer user_data_unused) ;

static void copySetCB(gpointer key_id, gpointer data, gpointer user_data) ;

static void setStrandFrameAttrs(ZMapFeatureTypeStyle type,
				gboolean *strand_specific_in,
				gboolean *show_rev_strand_in,
				ZMapStyle3FrameMode *frame_mode_in) ;
static void mergeColours(ZMapStyleFullColour curr, ZMapStyleFullColour new) ;

static ZMapFeatureTypeStyle inherit_parent(ZMapFeatureTypeStyle style, GHashTable *root_styles, int depth) ;
static void clear_inherited(gpointer key, gpointer item, gpointer user) ;



/* 
 *                   External interface
 */


/* Add a style to a set of styles, if the style is already there no action is taken
 * and FALSE is returned. */
gboolean zMapStyleSetAdd(GHashTable *style_set, ZMapFeatureTypeStyle style)
{
  gboolean result = FALSE ;
  GQuark style_id ;

  if (!style_set || !style) 
    return result ;

  style_id = zMapStyleGetUniqueID(style) ;

  if (!g_hash_table_lookup(style_set, GUINT_TO_POINTER(style_id)))
    {
      g_hash_table_insert(style_set, GUINT_TO_POINTER(style_id), style) ;

      result = TRUE ;
    }

  return result ;
}

void zMapStyleSetIsDefault(ZMapFeatureTypeStyle style)
{
  style->is_default = TRUE;
}

void zMapStyleSetOverridden(ZMapFeatureTypeStyle style, gboolean truth)
{
  style->overridden = truth;
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
 * re-written by mh17 May 2010 to use a more efficient way of finding parent styles and inheriting then
 * ..anyway this version runs as fast as a single datalist or hash table function
 * & does strictly less inherit operations than the number of styles
 */
gboolean zMapStyleInheritAllStyles(GHashTable *style_set)
{
  gboolean result = TRUE ;
  GList *iter ;
  ZMapFeatureTypeStyle old, new ;

  g_hash_table_foreach(style_set, clear_inherited, style_set) ;

  /* we need to process the hash table and alter the data pointers */

  /* get a list of keys */
  zMap_g_hash_table_get_keys(&iter, style_set) ;

  /* lookup each one and replace in turn, update the output datalist */
  for( ; iter ; iter = iter->next)
    {
      /* lookup the style: must do this for each as
       * these may be changed by inheritance of previous styles 
       */ 
      old = (ZMapFeatureTypeStyle)g_hash_table_lookup(style_set, iter->data) ;

      /* recursively inherit parent while not already done */ 
      new = inherit_parent(old, style_set, 0) ;
      if(!new)
	result = FALSE ;
    }

  /* tidy up */
  g_list_free(iter) ;

  return result ;
}



gboolean zMapStyleSetSubStyles(GHashTable *style_set)
{
  gboolean result = TRUE ;
  GList *iter;
  ZMapFeatureTypeStyle style,sub;
  int i;

      // get a list of keys
  zMap_g_hash_table_get_keys(&iter,style_set);

  for(;iter;iter = iter->next)
    {
      style = (ZMapFeatureTypeStyle) g_hash_table_lookup(style_set,iter->data);

      for(i = 0;i < ZMAPSTYLE_SUB_FEATURE_MAX;i++)
      {
            if(style->sub_features[i])
            {
                  sub = g_hash_table_lookup(style_set,GUINT_TO_POINTER(style->sub_features[i]));
                  style->sub_style[i] = sub;
            }
      }
    }

      // tidy up
  g_list_free(iter);

  return result ;
}


/* Copies a set of styles.
 *
 * If there are errors in trying to copy styles then this function returns FALSE
 * and a GHashTable containing as many styles as it could copy, there will be
 * log messages identifying the errors. It returns TRUE if there were no errors
 * at all.
 *
 *  */
gboolean zMapStyleCopyAllStyles(GHashTable *style_set, GHashTable **copy_style_set_out)
{
  gboolean result = TRUE ;
  CopyStyleCBStruct cb_data = {NULL} ;

  cb_data.error = &result ;
  cb_data.copy_set = g_hash_table_new(NULL,NULL) ;

  if(style_set)
  {
      /* see zmapView.c zmapViewDrawDiffContext(): "there are no styles"
       * this would produce a glib warning: how could it ever have worked?
       * we return an empty hash as the caller expects a data struct
       * (instead of not calling this func)
       */
        g_hash_table_foreach(style_set, copySetCB, &cb_data) ;
  }

  *copy_style_set_out = cb_data.copy_set ;

  return result ;
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
  gboolean result = FALSE ; 		    /* There is nothing to fail currently. */
  ZMapStyleParam param = zmapStyleParams_G;
  int i;

  if (!curr_style || !new_style) 
    return result ;

  result = TRUE ; 

  for(i = 1;i < _STYLE_PROP_N_ITEMS;i++,param++)
    {
      if(zMapStyleIsPropertySetId(new_style,param->id))      /* something to merge? */
        {

          switch(param->type)
            {
            case STYLE_PARAM_TYPE_FLAGS:   /* must not merge this! */
              continue;

            case STYLE_PARAM_TYPE_QUARK_LIST_ID:
              {
                  GList **l = (GList **) (((void *) curr_style) + param->offset);
                  GList **ln = (GList **) (((void *) new_style) + param->offset);

                  /* free old list before overwriting */ 
                  if(*l)
                        g_list_free( *l);
                  *l = g_list_copy(*ln);
                  break;
              }

            case STYLE_PARAM_TYPE_STRING:
               {
                 gchar **str;

                 if(zMapStyleIsPropertySetId(curr_style,param->id))
                  {

                    str = (gchar **) (((void *) curr_style) + param->offset);
                    g_free(*str) ;
                  }

                str = (gchar **) (((void *) new_style) + param->offset);
                * (gchar **) (((void *) curr_style) + param->offset) = g_strdup(*str);
                break;
              }

            case STYLE_PARAM_TYPE_COLOUR:
              {
                ZMapStyleFullColour src_colour,dst_colour;

                src_colour = (ZMapStyleFullColour) (((void *) new_style) + param->offset);
                dst_colour = (ZMapStyleFullColour) (((void *) curr_style) + param->offset);

                mergeColours(dst_colour,src_colour);
                break;
              }

            default:
              {
                void *src,*dst;

                src = ((void *) new_style) + param->offset;
                dst = ((void *) curr_style) + param->offset;

                memcpy(dst,src,param->size);
                break;
              }
            }

          zmapStyleSetIsSet(curr_style,param->id);
        }
    }

  return result ;
}


/*---------------------------------
 * previous style access functions replaced with macros
 * they provided default value by we can make the opbject do that
 * external modules have read-only access.
 *---------------------------------
 * too complicated for macros
 */
GQuark zMapStyleGetSubFeature(ZMapFeatureTypeStyle style,ZMapStyleSubFeature i)
{
  GQuark x = 0;

  if(i > 0 && i < ZMAPSTYLE_SUB_FEATURE_MAX)
      x = style->sub_features[i];
  return(x);
}



/* Splice marker features for displaying splice junctions (e.g. results of gene finder programs). */
ZMapStyleGlyphShape zMapStyleGlyphShape5(ZMapFeatureTypeStyle style, gboolean reverse)
{
  ZMapStyleGlyphShape shape ;

  if (!reverse && zMapStyleIsPropertySetId(style, STYLE_PROP_GLYPH_SHAPE_5))
    shape = &style->mode_data.glyph.glyph5 ;
  else if (reverse && zMapStyleIsPropertySetId(style, STYLE_PROP_GLYPH_SHAPE_5_REV))
    shape = &style->mode_data.glyph.glyph5rev ;
  else
    shape = &style->mode_data.glyph.glyph ;

  return(shape);
}

ZMapStyleGlyphShape zMapStyleGlyphShape3(ZMapFeatureTypeStyle style, gboolean reverse)
{
  ZMapStyleGlyphShape shape ;

  if (!reverse && zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_3))
    shape = &style->mode_data.glyph.glyph3;
  else if (reverse && zMapStyleIsPropertySetId(style,STYLE_PROP_GLYPH_SHAPE_3_REV))
    shape = &style->mode_data.glyph.glyph3rev;
  else
    shape = &style->mode_data.glyph.glyph;

  return(shape);
}



void zMapStyleSetParent(ZMapFeatureTypeStyle style, char *parent_name)
{

  zmapStyleSetIsSet(style,STYLE_PROP_PARENT_STYLE);
  style->parent_id = zMapStyleCreateID(parent_name) ;

  return ;
}

void zMapStyleSetDescription(ZMapFeatureTypeStyle style, char *description)
{

  zmapStyleSetIsSet(style,STYLE_PROP_DESCRIPTION);
  style->description = g_strdup(description) ;

  return ;
}

void zMapStyleSetWidth(ZMapFeatureTypeStyle style, double width)
{

  zmapStyleSetIsSet(style,STYLE_PROP_WIDTH);
  style->width = width ;

  return ;
}





/* Set magnification limits for displaying columns. */
void zMapStyleSetMag(ZMapFeatureTypeStyle style, double min_mag, double max_mag)
{

  if (min_mag && min_mag > 0.0)
    {
      style->min_mag = min_mag ;
      zmapStyleSetIsSet(style,STYLE_PROP_MIN_MAG);
    }

  if (max_mag && max_mag > 0.0)
    {
      style->max_mag = max_mag ;
      zmapStyleSetIsSet(style,STYLE_PROP_MAX_MAG);
    }


  return ;
}


// these next two functions are not called
gboolean zMapStyleIsMinMag(ZMapFeatureTypeStyle style, double *min_mag)
{
  gboolean mag_set = FALSE ;

  if (zMapStyleIsPropertySetId(style,STYLE_PROP_MIN_MAG))
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

  if (zMapStyleIsPropertySetId(style,STYLE_PROP_MIN_MAG))
    {
      mag_set = TRUE ;

      if (max_mag)
	*max_mag = style->max_mag ;
    }

  return mag_set ;
}


void zMapStyleSetMode(ZMapFeatureTypeStyle style, ZMapStyleMode mode)
{
  style->mode = mode;
  zmapStyleSetIsSet(style,STYLE_PROP_MODE);

  return ;
}


gboolean zMapStyleHasMode(ZMapFeatureTypeStyle style)
{
  gboolean result ;

  result = zMapStyleIsPropertySetId(style, STYLE_PROP_MODE) ;

  return result ;
}





void zMapStyleSetPfetch(ZMapFeatureTypeStyle style, gboolean pfetchable)
{

  style->mode_data.alignment.pfetchable = pfetchable ;
  zmapStyleSetIsSet(style,STYLE_PROP_ALIGNMENT_PFETCHABLE);

  return ;
}





void zMapStyleSetShowGaps(ZMapFeatureTypeStyle style, gboolean show_gaps)
{

  if (style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      style->mode_data.alignment.show_gaps = show_gaps;
      zmapStyleSetIsSet(style,STYLE_PROP_ALIGNMENT_SHOW_GAPS);
    }
  return ;
}




void zMapStyleSetGappedAligns(ZMapFeatureTypeStyle style, gboolean parse_gaps, gboolean show_gaps)
{

  if (style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      style->mode_data.alignment.show_gaps = show_gaps;
      zmapStyleSetIsSet(style,STYLE_PROP_ALIGNMENT_SHOW_GAPS);
      style->mode_data.alignment.parse_gaps = parse_gaps;
      zmapStyleSetIsSet(style,STYLE_PROP_ALIGNMENT_PARSE_GAPS);
    }
  return ;
}



void zMapStyleSetJoinAligns(ZMapFeatureTypeStyle style, unsigned int between_align_error)
{
  if(style->mode == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      zmapStyleSetIsSet(style,STYLE_PROP_ALIGNMENT_BETWEEN_ERROR);
      style->mode_data.alignment.between_align_error = between_align_error ;
    }

  return ;
}


void zMapStyleSetDisplayable(ZMapFeatureTypeStyle style, gboolean displayable)
{
  style->displayable = displayable ;
  zmapStyleSetIsSet(style,STYLE_PROP_DISPLAYABLE);

  return ;
}


/* Controls whether the feature set is displayed. */
void zMapStyleSetDisplay(ZMapFeatureTypeStyle style, ZMapStyleColumnDisplayState col_show)
{
  if (!(style && col_show > ZMAPSTYLE_COLDISPLAY_INVALID && col_show <= ZMAPSTYLE_COLDISPLAY_SHOW))
    return ;

  zmapStyleSetIsSet(style,STYLE_PROP_COLUMN_DISPLAY_MODE);
  style->col_display_state = col_show ;

  return ;
}


/* Controls whether the feature set is displayed initially. */
void zMapStyleSetShowWhenEmpty(ZMapFeatureTypeStyle style, gboolean show_when_empty)
{
  g_return_if_fail(ZMAP_IS_FEATURE_STYLE(style));

  style->show_when_empty = show_when_empty ;
  zmapStyleSetIsSet(style,STYLE_PROP_SHOW_WHEN_EMPTY);

  return ;
}






/* Set up graphing stuff, currentlyends the basic code is copied from acedb but this will
 * change if we add different graphing types.... */
void zMapStyleSetGraph(ZMapFeatureTypeStyle style, ZMapStyleGraphMode mode,
		       double min_score, double max_score, double baseline)
{

  style->mode_data.graph.mode = mode ;
  zmapStyleSetIsSet(style,STYLE_PROP_GRAPH_MODE);
  style->mode_data.graph.baseline = baseline ;
  zmapStyleSetIsSet(style,STYLE_PROP_GRAPH_BASELINE);

  style->min_score = min_score ;
  zmapStyleSetIsSet(style,STYLE_PROP_MIN_SCORE);
  style->max_score = max_score ;
  zmapStyleSetIsSet(style,STYLE_PROP_MAX_SCORE);


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

  return ;
}




/* Set score bounds for displaying column with width related to score. */
void zMapStyleSetScore(ZMapFeatureTypeStyle style, double min_score, double max_score)
{

  style->min_score = min_score ;
  zmapStyleSetIsSet(style,STYLE_PROP_MIN_SCORE);
  style->max_score = max_score ;
  zmapStyleSetIsSet(style,STYLE_PROP_MAX_SCORE);

  return ;
}




/*!
 * \brief sets the end style of exons
 *
 * Controls the end of exons at the moment.  They are either pointy,
 * strand sensitive (when true) or square when false (default) */
void zMapStyleSetEndStyle(ZMapFeatureTypeStyle style, gboolean directional)
{
  style->directional_end = directional;
  zmapStyleSetIsSet(style,STYLE_PROP_DIRECTIONAL_ENDS);

  return;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* NOT USING THIS CURRENTLY, WILL BE NEEDED IN FUTURE... */

/* Score stuff is all quite interdependent, hence this function to set it all up. */
void zMapStyleSetScore(ZMapFeatureTypeStyle style, char *score_str,
		       double min_score, double max_score)
{
  ZMapStyleScoreMode score = ZMAPSCORE_WIDTH ;

  if (!style) 
    return ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* WE ONLY SCORE BY WIDTH AT THE MOMENT..... */
  if (bump_str && *bump_str)
    {
      if (g_ascii_strcasecmp(bump_str, "bump") == 0)
	bump = ZMAPBUMP_OVERLAP ;
      else if (g_ascii_strcasecmp(bump_str, "position") == 0)
	bump = ZMAPBUMP_POSITION ;
      else if (g_ascii_strcasecmp(bump_str, "name") == 0)
	bump = ZMAPBUMP_NAME ;
      else if (g_ascii_strcasecmp(bump_str, "simple") == 0)
	bump = ZMAPBUMP_SIMPLE ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  style->score_mode = score ;

  /* Make sure we have some kind of score. */
  if (style->max_score == style->min_score)
    style->max_score = style->min_score + 1 ;

  /* Make sure we have kind of width. */
  if (!(style->width))
    style->width = 2.0 ;

  zmapStyleSetIsSet(style,STYLE_PROP_MIN_SCORE);
  zmapStyleSetIsSet(style,STYLE_PROP_MAX_SCORE);
  zmapStyleSetIsSet(style,STYLE_PROP_SCORE_MODE);

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void zMapStyleSetStrandSpecific(ZMapFeatureTypeStyle type, gboolean strand_specific)
{
  setStrandFrameAttrs(type, &strand_specific, NULL, NULL) ;

  return ;
}

void zMapStyleSetStrandShowReverse(ZMapFeatureTypeStyle type, gboolean show_reverse)
{
  setStrandFrameAttrs(type, NULL, &show_reverse, NULL) ;

  return ;
}

void zMapStyleSetFrameMode(ZMapFeatureTypeStyle type, ZMapStyle3FrameMode frame_mode)
{
  setStrandFrameAttrs(type, NULL, NULL, &frame_mode) ;

  return ;
}


/* These attributes are not independent hence the bundling into this call, only one input can be
 * set at a time. */
static void setStrandFrameAttrs(ZMapFeatureTypeStyle type,
                        gboolean *strand_specific_in,
                        gboolean *show_rev_strand_in,
                        ZMapStyle3FrameMode *frame_mode_in)
{
  if (strand_specific_in)
    {
      zmapStyleSetIsSet(type,STYLE_PROP_STRAND_SPECIFIC);

      if (*strand_specific_in)
      {
        type->strand_specific = TRUE ;
      }
      else
      {
        type->strand_specific = FALSE ;

        zmapStyleUnsetIsSet(type,STYLE_PROP_SHOW_REV_STRAND);

        if (zMapStyleIsPropertySetId(type,STYLE_PROP_FRAME_MODE))
          type->frame_mode = ZMAPSTYLE_3_FRAME_NEVER ;
      }
    }
  else if (show_rev_strand_in)
    {
      zmapStyleSetIsSet(type,STYLE_PROP_SHOW_REV_STRAND);
      type->show_rev_strand = *show_rev_strand_in ;

      if (*show_rev_strand_in)
      {
         zmapStyleSetIsSet(type,STYLE_PROP_STRAND_SPECIFIC);
         type->strand_specific = TRUE ;
      }
    }
  else if (frame_mode_in)
    {
      if (*frame_mode_in != ZMAPSTYLE_3_FRAME_NEVER)
      {
        zmapStyleSetIsSet(type,STYLE_PROP_STRAND_SPECIFIC);
        type->strand_specific = TRUE ;

        zmapStyleSetIsSet(type,STYLE_PROP_FRAME_MODE);
        type->frame_mode = *frame_mode_in ;
      }
      else
      {
        zmapStyleUnsetIsSet(type,STYLE_PROP_STRAND_SPECIFIC);
        type->strand_specific = FALSE ;

        if (zMapStyleIsPropertySetId(type,STYLE_PROP_SHOW_REV_STRAND) && type->show_rev_strand)
            zmapStyleUnsetIsSet(type,STYLE_PROP_SHOW_REV_STRAND);

        zmapStyleSetIsSet(type,STYLE_PROP_FRAME_MODE);
        type->frame_mode = *frame_mode_in ;
      }
    }

  return ;
}

void zMapStyleGetStrandAttrs(ZMapFeatureTypeStyle type,
			     gboolean *strand_specific, gboolean *show_rev_strand, ZMapStyle3FrameMode *frame_mode)
{

  if (strand_specific && zMapStyleIsPropertySetId(type,STYLE_PROP_STRAND_SPECIFIC))
    *strand_specific = type->strand_specific ;
  if (show_rev_strand && zMapStyleIsPropertySetId(type,STYLE_PROP_SHOW_REV_STRAND))
    *show_rev_strand = type->show_rev_strand ;
  if (frame_mode && zMapStyleIsPropertySetId(type,STYLE_PROP_FRAME_MODE))
    *frame_mode = type->frame_mode ;

  return ;
}


gboolean zMapStyleIsFrameSpecific(ZMapFeatureTypeStyle style)
{
  gboolean frame_specific = FALSE ;

  if (zMapStyleIsPropertySetId(style,STYLE_PROP_FRAME_MODE) && style->frame_mode != ZMAPSTYLE_3_FRAME_NEVER)
    frame_specific = TRUE ;

  return frame_specific ;
}



void zMapStyleSetGFF(ZMapFeatureTypeStyle style, char *gff_source, char *gff_feature)
{

  if (gff_source && *gff_source)
    {
      style->gff_source = g_quark_from_string(gff_source) ;
      zmapStyleSetIsSet(style,STYLE_PROP_GFF_SOURCE);
    }

  if (gff_feature && *gff_feature)
    {
      style->gff_feature = g_quark_from_string(gff_feature) ;
      zmapStyleSetIsSet(style,STYLE_PROP_GFF_SOURCE);
    }

  return ;
}



#if NOT_USED
void zMapStyleSetBumpMode(ZMapFeatureTypeStyle style, ZMapStyleBumpMode bump_mode)
{
  if(!(bump_mode >= ZMAPBUMP_START && bump_mode <= ZMAPBUMP_END))
      bump_mode  = ZMAPBUMP_UNBUMP;       /* better than going Assert */

  if (!style->bump_fixed)
    {
      // MH17: this looked wrong, refer to old version
      if(!zMapStyleIsPropertySetId(style,STYLE_PROP_BUMP_DEFAULT))
	{
	  zmapStyleSetIsSet(style,STYLE_PROP_BUMP_DEFAULT);
	  style->default_bump_mode = bump_mode ;
	}

      style->initial_bump_mode = bump_mode ;
      zmapStyleSetIsSet(style,STYLE_PROP_BUMP_MODE);
    }

  return ;
}
#endif

void zMapStyleSetBumpSpace(ZMapFeatureTypeStyle style, double bump_spacing)
{
  zmapStyleSetIsSet(style,STYLE_PROP_BUMP_SPACING);
  style->bump_spacing = bump_spacing ;

  return ;
}




/* Re/init bump mode. */
void zMapStyleInitBumpMode(ZMapFeatureTypeStyle style,
			   ZMapStyleBumpMode default_bump_mode, ZMapStyleBumpMode curr_bump_mode)
{
  if (!(style && (default_bump_mode ==  ZMAPBUMP_INVALID
      || (default_bump_mode >= ZMAPBUMP_START && default_bump_mode <= ZMAPBUMP_END))
      && (curr_bump_mode ==  ZMAPBUMP_INVALID
      || (curr_bump_mode >= ZMAPBUMP_START && curr_bump_mode <= ZMAPBUMP_END)) ) )
    return ;

  if (!style->bump_fixed)
    {

      if (curr_bump_mode != ZMAPBUMP_INVALID)
	{
        zmapStyleSetIsSet(style,STYLE_PROP_BUMP_MODE);
	  style->initial_bump_mode = curr_bump_mode ;
	}

      if (default_bump_mode != ZMAPBUMP_INVALID)
	{
        zmapStyleSetIsSet(style,STYLE_PROP_BUMP_DEFAULT);
	  style->default_bump_mode = default_bump_mode ;
	}
    }

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
GHashTable *zMapStyleMergeStyles(GHashTable *curr_styles, GHashTable *new_styles, ZMapStyleMergeMode merge_mode)
{
  GHashTable *merged_styles = NULL ;
  MergeStyleCBStruct merge_data = {FALSE} ;

  merge_data.merge_mode = merge_mode ;
  merge_data.curr_styles = curr_styles ;

  g_hash_table_foreach(new_styles, mergeStyle, &merge_data) ;

  merged_styles = merge_data.curr_styles ;

  return merged_styles ;
}




/* need a func to free a styles list here..... */
void zMapStyleDestroyStyles(GHashTable *styles)
{
  g_hash_table_foreach_remove(styles, destroyStyle, NULL) ;

  return ;
}




/*
 *                  Internal routines
 */

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
static void mergeStyle(gpointer style_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle new_style = (ZMapFeatureTypeStyle)data ;
  MergeStyleCB merge_data = (MergeStyleCB)user_data ;
  GHashTable *curr_styles = merge_data->curr_styles ;
  ZMapFeatureTypeStyle curr_style = NULL ;


  /* If we find the style then process according to merge_mode, if not then add a copy to the curr_styles. */
  if ((curr_style = g_hash_table_lookup(curr_styles, GUINT_TO_POINTER(new_style->unique_id))))
    {
      switch (merge_data->merge_mode)
	{
	case ZMAPSTYLE_MERGE_PRESERVE:
	  {
	    /* Leave the existing style untouched. */
	    break ;
	  }
	case ZMAPSTYLE_MERGE_REPLACE:
	  {
	    /* Remove the existing style and put the new one in its place. */
	    ZMapFeatureTypeStyle copied_style = NULL ;

          copied_style = zMapFeatureStyleCopy(new_style);
          g_hash_table_replace(curr_styles,GUINT_TO_POINTER(new_style->unique_id),copied_style);

	    zMapStyleDestroy(curr_style) ;

          merge_data->curr_styles = curr_styles ;

          break ;
	  }
	case ZMAPSTYLE_MERGE_MERGE:
	  {
	    /* Merge the existing and new styles. */
	    zMapStyleMerge(curr_style, new_style) ;

	    break ;
	  }
	default:
	  {
	    zMapLogFatalLogicErr("switch(), unknown value: %d", merge_data->merge_mode) ;

	    break ;
	  }

	}
    }
  else
    {
      ZMapFeatureTypeStyle copied_style = NULL ;

      copied_style = zMapFeatureStyleCopy(new_style);
	g_hash_table_insert(curr_styles, GUINT_TO_POINTER(new_style->unique_id), copied_style) ;
	merge_data->curr_styles = curr_styles ;
    }

  return ;
}



/* Destroy the given style. */
static gboolean destroyStyle(gpointer style_id, gpointer data, gpointer user_data_unused)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  zMapStyleDestroy(style);

  return TRUE;
}



/* A GHashTableForeachFunc() to copy styles into a GHashTable */
static void copySetCB(gpointer key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle curr_style = (ZMapFeatureTypeStyle)data ;
  CopyStyleCB cb_data = (CopyStyleCB)user_data ;
  ZMapFeatureTypeStyle copy_style ;
  GHashTable *style_set = cb_data->copy_set ;

  if (GPOINTER_TO_UINT(key_id) != curr_style->unique_id) 
    return ;

  if ((copy_style = zMapFeatureStyleCopy(curr_style)))
    g_hash_table_insert(style_set, key_id, copy_style) ;
  else
    *(cb_data->error) = TRUE ;

  cb_data->copy_set = style_set ;

  return ;
}




/* actually the return value comment is not true....see code towards
 * end of function....should correct that and test. */
/* inherit the parent style, recursively inheriting its ancestors first
 * replaces and frees the old style with the new one and returns the new
 * or NULL on failure */
static ZMapFeatureTypeStyle inherit_parent(ZMapFeatureTypeStyle style, GHashTable *root_styles, int depth)
{
  ZMapFeatureTypeStyle parent, tmp_style = NULL;


  /* I REALLY HATE THIS...RETURNS ALL OVER THE PLACE...... */

  if (depth > 20)
    {
      zMapLogWarning("Style %s has more than 20 ancestors - suspected mutually recursive definition of parent style",
		     g_quark_to_string(style->original_id));

      style->inherited = TRUE;      // prevent repeated messages

      return(NULL);
    }

  if (zMapStyleIsPropertySetId(style, STYLE_PROP_PARENT_STYLE) && !style->inherited)
    {
      if (style->parent_id == style->unique_id)
        {
	  zMapLogWarning("Style %s is its own parent", g_quark_to_string(style->original_id)) ;
	  style->inherited = TRUE ;

	  return(NULL) ;
        }

      if (!(parent = (ZMapFeatureTypeStyle) g_hash_table_lookup(root_styles,GUINT_TO_POINTER(style->parent_id))))
        {
          zMapLogWarning("Style \"%s\" has the parent style \"%s\" "
			 "but the latter cannot be found in the styles set so cannot be inherited.",
			 g_quark_to_string(style->original_id), g_quark_to_string(style->parent_id)) ;

          return(NULL);
        }

      if(!parent->inherited)
	parent = inherit_parent(parent,root_styles,depth+1);

      if(parent)
        {
          tmp_style = zMapFeatureStyleCopy(parent) ;

          if (zMapStyleMerge(tmp_style, style))
            {
              tmp_style->inherited = TRUE;

#if MH17_CHECK_INHERITANCE
	      printf("%s inherited  %s, (%d/%d -> %d)\n",
		     g_quark_to_string(style->unique_id),
		     g_quark_to_string(parent->unique_id),
		     parent->default_bump_mode, style->default_bump_mode, tmp_style->default_bump_mode);
#endif
	      // keep this up to date
              g_hash_table_replace(root_styles,GUINT_TO_POINTER(style->unique_id),tmp_style);
              zMapStyleDestroy(style);
            }
          else
            {
              zMapLogWarning("Merge of style \"%s\" into style \"%s\" failed.",
			     g_quark_to_string(parent->original_id),
			     g_quark_to_string(style->original_id)) ;

	      zMapStyleDestroy(tmp_style);
	      tmp_style = style;
            }
        }
    }
  else
    {
      // nothing to do return: input style
      style->inherited = TRUE;
      tmp_style = style;
    }

  return tmp_style;
}



// add a style to a hash table
static void clear_inherited(gpointer key, gpointer item, gpointer user)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle) item;

  style->inherited = FALSE;
}


static void mergeColours(ZMapStyleFullColour curr, ZMapStyleFullColour new)
{

  if (new->normal.fields_set.fill)
    {
      curr->normal.fill = new->normal.fill ;
      curr->normal.fields_set.fill = TRUE ;
    }
  if (new->normal.fields_set.draw)
    {
      curr->normal.draw = new->normal.draw ;
      curr->normal.fields_set.draw = TRUE ;
    }
  if (new->normal.fields_set.border)
    {
      curr->normal.border = new->normal.border ;
      curr->normal.fields_set.border = TRUE ;
    }


  if (new->selected.fields_set.fill)
    {
      curr->selected.fill = new->selected.fill ;
      curr->selected.fields_set.fill = TRUE ;
    }
  if (new->selected.fields_set.draw)
    {
      curr->selected.draw = new->selected.draw ;
      curr->selected.fields_set.draw = TRUE ;
    }
  if (new->selected.fields_set.border)
    {
      curr->selected.border = new->selected.border ;
      curr->selected.fields_set.border = TRUE ;
    }


  return ;
}






