/*  File: zmapWindowColBump.c
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
 * Description: Column bumping functions.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: May  1 13:18 2009 (rds)
 * Created: Tue Sep  4 10:52:09 2007 (edgrif)
 * CVS info:   $Id: zmapWindowColBump.c,v 1.40 2009-05-01 12:19:33 rds Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

typedef struct
{
  GQuark                      style_id;
  ZMapStyleBumpMode           bump_mode;
  ZMapStyleColumnDisplayState display_state;
  unsigned int                match_threshold;
  gboolean                    bump_all;
} StylePropertiesStruct;

/* For straight forward bumping. */
typedef struct
{
  ZMapWindow window ;
  ZMapWindowItemFeatureSetData set_data ;

  GHashTable *pos_hash ;
  GList *pos_list ;

  double offset ;
  double incr ;

  int start, end ;

  StylePropertiesStruct style_prop;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
  double incr ;
} BumpColRangeStruct, *BumpColRange ;



/* THERE IS A BUG IN THE CODE THAT USES THIS...IT MOVES FEATURES OUTSIDE OF THE BACKGROUND SO
 * IT ALL LOOKS NAFF, PROBABLY WE GET AWAY WITH IT BECAUSE COLS ARE SO WIDELY SPACED OTHERWISE
 * THEY MIGHT OVERLAP. I THINK THE MOVING CODE MAY BE AT FAULT. */
/* For complex bump users seem to want columns to overlap a bit, if this is set to 1.0 there is
 * no overlap, if you set it to 0.5 they will overlap by a half and so on. */
#define COMPLEX_BUMP_COMPRESS 1.0

typedef struct
{
  ZMapWindow window ;
  GList **gaps_added_items ;
} AddGapsDataStruct, *AddGapsData ;

typedef gboolean (*OverLapListFunc)(GList *curr_features, GList *new_features) ;

typedef GList* (*GListTraverseFunc)(GList *list) ;

typedef struct
{
  ZMapWindow window ;
  FooCanvasGroup *column_group ;
  ZMapWindowItemFeatureSetData set_data ;


  gboolean             bump_all ;
  GHashTable          *name_hash ;
  GList               *bumpcol_list ;
  double               curr_offset ;
  double               incr ;
  OverLapListFunc      overlap_func ;
  gboolean             protein ;

  unsigned int         match_threshold;
  GQuark               style_id;
} ComplexBumpStruct, *ComplexBump ;


typedef struct
{
  ZMapWindow window ;
  FooCanvasGroup *column_group ;
  ZMapWindowItemFeatureSetData set_data ;

  double incr ;
  double offset ;
  double width ;
  GList *feature_list ;
  unsigned int   match_threshold;
} ComplexColStruct, *ComplexCol ;


typedef struct
{
  GList **name_list ;
  OverLapListFunc overlap_func ;
} FeatureDataStruct, *FeatureData ;

typedef struct
{
  gboolean overlap;
  int start, end ;
  int feature_diff ;
  ZMapFeature feature ;
  double width ;
} RangeDataStruct, *RangeData ;



typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;


typedef struct execOnChildrenStruct_
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;


/* For 3 frame display/normal display. */
typedef struct
{
  FooCanvasGroup *forward_group, *reverse_group ;
  ZMapFeatureBlock block ;
  ZMapWindow window ;

  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

  int frame3_pos ;
  ZMapFrame frame ;

} RedrawDataStruct, *RedrawData ;



typedef enum {COLINEAR_INVALID, COLINEAR_NOT, COLINEAR_IMPERFECT, COLINEAR_PERFECT} ColinearityType ;




static void bumpColCB(gpointer data, gpointer user_data) ;


static void compareListOverlapCB(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void addToList(gpointer data, gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gboolean featureListCB(gpointer data, gpointer user_data) ;

static gint sortByScoreCB(gconstpointer a, gconstpointer b) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gint sortBySpanCB(gconstpointer a, gconstpointer b) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static gint sortByStrandSpanCB(gconstpointer a, gconstpointer b) ;
static gint sortByOverlapCB(gconstpointer a, gconstpointer b, gpointer user_data) ;
static void bestFitCB(gpointer data, gpointer user_data) ;
static gboolean itemGetCoords(FooCanvasItem *item, double *x1_out, double *x2_out) ;
static void listScoreCB(gpointer data, gpointer user_data) ;
static void makeNameListCB(gpointer data, gpointer user_data) ;

static void makeNameListStrandedCB(gpointer data, gpointer user_data) ;


static void setOffsetCB(gpointer data, gpointer user_data) ;
static void setAllOffsetCB(gpointer data, gpointer user_data) ;
static void getMaxWidth(gpointer data, gpointer user_data) ;
static void sortListPosition(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void addBackgrounds(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void addMultiBackgrounds(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void addGapsCB(gpointer data, gpointer user_data) ;
static void removeGapsCB(gpointer data, gpointer user_data) ;


static void NEWaddMultiBackgrounds(gpointer data, gpointer user_data) ;



static FooCanvasItem *makeMatchItem(ZMapWindowLongItems long_items,
				    FooCanvasGroup *parent, ZMapDrawObjectType shape,
				    double x1, double y1, double x2, double y2,
				    GdkColor *colour, gpointer item_data, gpointer event_data) ;
static gboolean listsOverlap(GList *curr_features, GList *new_features) ;
static gboolean listsOverlapNoInterleave(GList *curr_features, GList *new_features) ;
static void reverseOffsets(GList *bumpcol_list) ;
static void moveItemsCB(gpointer data, gpointer user_data) ;
static void moveItemCB(gpointer data, gpointer user_data) ;
static void freeExtraItems(gpointer data, gpointer user_data) ;
static void showItems(gpointer data, gpointer user_data) ;
static gboolean removeNameListsByRange(GList **names_list, int start, int end) ;
static void testRangeCB(gpointer data, gpointer user_data) ;
static void hideItemsCB(gpointer data, gpointer user_data_unused) ;
static void hashDataDestroyCB(gpointer data) ;
static void listDataDestroyCB(gpointer data, gpointer user_data) ;
static void getListFromHash(gpointer key, gpointer value, gpointer user_data) ;
static void setStyleBumpCB(ZMapFeatureTypeStyle style, gpointer user_data) ;
static gboolean bumpBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean bumpBackgroundDestroyCB(FooCanvasItem *feature_item, gpointer data) ;



#ifdef UNUSED_FUNCTIONS
static gint compareNameToColumn(gconstpointer list_data, gconstpointer user_data) ;
#endif
static gint findItemInQueueCB(gconstpointer a, gconstpointer b) ;

static void removeNonColinearExtensions(gpointer data, gpointer user_data) ;
static gboolean getFeatureFromListItem(GList *list_item, FooCanvasItem **item_out, ZMapFeature *feature_out) ;
static gboolean findRangeListItems(GList *search_start, int seq_start, int seq_end,
				   GList **first_out, GList **last_out) ;
static GList *removeNonColinear(GList *first_list_item, ZMapGListDirection direction, BumpCol bump_data) ;

static ColinearityType featureHomolIsColinear(ZMapWindow window,  unsigned int match_threshold,
					      ZMapFeature feat_1, ZMapFeature feat_2) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printChild(gpointer data, gpointer user_data) ;
static void printQuarks(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapStyleBumpMode hack_initial_mode(ZMapFeatureTypeStyle style);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void invoke_bump_to_initial(FooCanvasGroup *container, FooCanvasPoints *points, 
				   ZMapContainerLevelType level, gpointer user_data);


/* Merely a cover function for the real bumping code function zmapWindowColumnBumpRange(). */
void zmapWindowColumnBump(FooCanvasItem *column_item, ZMapStyleBumpMode bump_mode)
{
  ZMapWindowItemFeatureSetData set_data ;
  ZMapWindowCompressMode compress_mode ;

  set_data = g_object_get_data(G_OBJECT(column_item), ITEM_FEATURE_SET_DATA);

  if (zmapWindowMarkIsSet(set_data->window->mark))
    compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
  else
    compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

  zmapWindowColumnBumpRange(column_item, bump_mode, compress_mode) ;

  return ;
}



/* Bumps either the whole column represented by column_item, or if the item is a feature item
 * in the column then bumps just features in that column that share the same style. This
 * allows the user control over whether to bump all features in a column or just some features
 * in a column.
 * 
 * NOTE: this function bumps an individual column but it DOES NOT move any other columns,
 * to do that you need to use zmapWindowColumnReposition(). The split is made because
 * we don't always want to reposition all the columns following a bump, e.g. when we
 * are creating a zmap window.
 *  */
void zmapWindowColumnBumpRange(FooCanvasItem *column_item, ZMapStyleBumpMode bump_mode,
			       ZMapWindowCompressMode compress_mode)
{
  BumpColStruct bump_data = {NULL} ;
  FooCanvasGroup *column_features ;
  StylePropertiesStruct styles_prop = {0};
  ZMapWindowItemFeatureType feature_type ;
  ZMapContainerLevelType container_type ;
  FooCanvasGroup *column_group =  NULL ;
  ZMapWindowItemFeatureSetData set_data ;
  ZMapStyleBumpMode historic_bump_mode;
  gboolean column = FALSE ;
  gboolean bumped = TRUE ;
  gboolean mark_set;
  int start, end ;
  double bump_spacing = 0.0 ;
  double width ;

  g_return_if_fail(bump_mode != ZMAPBUMP_INVALID);

  /* Decide if the column_item is a column group or a feature within that group. */
  if ((feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column_item), ITEM_FEATURE_TYPE)))
      != ITEM_FEATURE_INVALID)
    {
      column = FALSE ;
      column_group = zmapWindowContainerGetParentContainerFromItem(column_item) ;
    }
  else if ((container_type = zmapWindowContainerGetLevel(FOO_CANVAS_GROUP(column_item)))
	   == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      column = TRUE ;
      column_group = FOO_CANVAS_GROUP(column_item) ;
    }
  else
    zMapAssertNotReached() ;

  column_features = zmapWindowContainerGetFeatures(column_group) ;

  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  bump_data.set_data = set_data ;

  zMapWindowBusy(set_data->window, TRUE) ;

  bump_data.window = set_data->window ;

  /* Need to know if mark is set for limiting feature display for several modes/feature types. */
  mark_set = zmapWindowMarkIsSet(set_data->window->mark) ;


  /* We need this to know whether to remove and add Gaps */
  historic_bump_mode = zmapWindowItemFeatureSetGetBumpMode(set_data) ;

  /* We may have created extra items for some bump modes to show all matches from the same query
   * etc. so now we need to get rid of them before redoing the bump. */
  if (set_data->extra_items)
    {
      g_list_foreach(set_data->extra_items, freeExtraItems, NULL) ;

      g_list_free(set_data->extra_items) ;
      set_data->extra_items = NULL ;
    }


  /* Some items may have had their gaps added for some bump modes, they need to have
   * them removed. */
  if (set_data->gaps_added_items && historic_bump_mode != bump_mode)
    {
      g_list_foreach(set_data->gaps_added_items, removeGapsCB, set_data->window) ;

      g_list_free(set_data->gaps_added_items) ;
      set_data->gaps_added_items = NULL ;
    }


  /* Some features may have been hidden for bumping, unhide them now. */
  if (set_data->hidden_bump_features)
    {
      g_list_foreach(column_features->item_list, showItems, set_data) ;
      set_data->hidden_bump_features = FALSE ;
    }

  /* If user clicked on the column, not a feature within a column then we need to bump all styles
   * and features within the column, otherwise we just bump the specific features. */
  if (column)
    {
      styles_prop.bump_all = TRUE ;
      styles_prop.style_id = set_data->unique_id;
      styles_prop.display_state = zmapWindowItemFeatureSetGetDisplay(set_data);
     
      zmapWindowItemFeatureSetJoinAligns(set_data, &(styles_prop.match_threshold));

      zmapWindowStyleTableForEach(set_data->style_table, setStyleBumpCB, GINT_TO_POINTER(bump_mode)) ;
    }				 
  else
    {
      ZMapFeatureTypeStyle style;
      ZMapFeature feature ;

      feature = g_object_get_data(G_OBJECT(column_item), ITEM_FEATURE_DATA) ;
      zMapAssert(feature) ;

      styles_prop.bump_all = FALSE;
      styles_prop.style_id = feature->style_id;

      if((style = zmapWindowStyleTableFind(set_data->style_table, feature->style_id)))
	{
	  g_object_get(G_OBJECT(style),
		       ZMAPSTYLE_PROPERTY_DISPLAY_MODE, &(styles_prop.display_state),
		       NULL);

	  zMapStyleGetJoinAligns(style, &(styles_prop.match_threshold));
	  
	  zMapStyleSetBumpMode(style, bump_mode);
	}
      else
	zMapLogCritical("Missing style '%s'", g_quark_to_string(feature->style_id));
    }
  
  styles_prop.bump_mode = bump_mode;

  /* If range set explicitly or a mark is set on the window, then only bump within the range of mark
   * or the visible section of the window. */
  if (compress_mode == ZMAPWINDOW_COMPRESS_INVALID)
    {
      if (mark_set)
	zmapWindowMarkGetSequenceRange(set_data->window->mark, &start, &end) ;
      else
	{
	  start = set_data->window->min_coord ;
	  end = set_data->window->max_coord ;
	}
    }
  else
    {
      if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE)
	{
	  double wx1, wy1, wx2, wy2 ;
        
        
	  zmapWindowItemGetVisibleCanvas(set_data->window, 
					 &wx1, &wy1,
					 &wx2, &wy2);
        
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("Visible %f, %f  -> %f, %f\n", wx1, wy1, wx2, wy2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
        
	  /* should really clamp to seq. start/end..... */
	  start = (int)wy1 ;
	  end = (int)wy2 ;
	}
      else if (compress_mode == ZMAPWINDOW_COMPRESS_MARK)
	{
	  zMapAssert(mark_set) ;

	  /* we know mark is set so no need to check result of range check. But should check
	   * that col to be bumped and mark are in same block ! */
	  zmapWindowMarkGetSequenceRange(set_data->window->mark, &start, &end) ;
	}
      else
	{
	  start = set_data->window->min_coord ;
	  end = set_data->window->max_coord ;
	}
    }

  bump_data.start = start ;
  bump_data.end = end ;


  /* All bump modes except ZMAPBUMP_COMPLEX share common data/code as they are essentially
   * simple variants. The complex mode requires more processing so uses its own structs/lists. */
  width = zmapWindowItemFeatureSetGetWidth(set_data);
  bump_spacing = zmapWindowItemFeatureGetBumpSpacing(set_data) ;


  bump_data.style_prop = styles_prop; /* struct copy! */

  bump_data.incr = width + bump_spacing ;

  switch (bump_mode)
    {
    case ZMAPBUMP_UNBUMP:
      bump_data.incr = 0.0 ;
      break ;
    case ZMAPBUMP_ALL:
      break ;
    case ZMAPBUMP_START_POSITION:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, hashDataDestroyCB) ;
      break ;
    case ZMAPBUMP_OVERLAP:
      break ;
    case ZMAPBUMP_NAME:
      bump_data.pos_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash */
						 NULL, hashDataDestroyCB) ;
      break ;
    case ZMAPBUMP_NAVIGATOR:
      break;
    case ZMAPBUMP_ALTERNATING:
      break;

    case ZMAPBUMP_NAME_INTERLEAVE:
    case ZMAPBUMP_NAME_NO_INTERLEAVE:
    case ZMAPBUMP_NAME_COLINEAR:
    case ZMAPBUMP_NAME_BEST_ENDS:
      {
	ComplexBumpStruct complex = {NULL} ;
	GList *names_list = NULL ;
	int list_length ;

	complex.bump_all  = styles_prop.bump_all;
	complex.style_id  = styles_prop.style_id;
	complex.match_threshold = 1; /* fix this! */

	/* Make a hash table of feature names which are hashed to lists of features with that name. */
	complex.name_hash = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash/comparison. */
						  NULL, hashDataDestroyCB) ;


	if (bump_mode == ZMAPBUMP_NAME_BEST_ENDS || bump_mode == ZMAPBUMP_NAME_COLINEAR
	    || bump_mode == ZMAPBUMP_NAME_NO_INTERLEAVE)
	  g_list_foreach(column_features->item_list, makeNameListStrandedCB, &complex) ;
	else
	  g_list_foreach(column_features->item_list, makeNameListCB, &complex) ;

	zMapPrintTimer(NULL, "Made names list") ;


	/* Extract the lists of features into a list of lists for sorting. */
	g_hash_table_foreach(complex.name_hash, getListFromHash, &names_list) ;

	zMapPrintTimer(NULL, "Made list of lists") ;

	/* Sort each sub list of features by position for simpler position sorting later. */
	g_list_foreach(names_list, sortListPosition, NULL) ;

	zMapPrintTimer(NULL, "Sorted list of lists by position") ;


	/* Sort the top list using the combined normalised scores of the sublists so higher
	 * scoring matches come first. */

	/* Lets try different sorting for proteins vs. dna. */
	if (complex.protein)
	  names_list = g_list_sort(names_list, sortByScoreCB) ;
	else
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  names_list = g_list_sort(names_list, sortBySpanCB) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	names_list = g_list_sort(names_list, sortByStrandSpanCB) ;

	zMapPrintTimer(NULL, "Sorted by score or strand") ;

	list_length = g_list_length(names_list) ;

	/* Remove any lists that do not bump with the range set by the user. */
	if ((compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE || (compress_mode == ZMAPWINDOW_COMPRESS_MARK))
	    || (bump_mode == ZMAPBUMP_NAME_BEST_ENDS || bump_mode == ZMAPBUMP_NAME_COLINEAR))
	  {
	    RangeDataStruct range = {FALSE} ;

	    range.start = start ;
	    range.end = end ;

	    names_list = g_list_sort_with_data(names_list, sortByOverlapCB, &range) ;
	  }

	list_length = g_list_length(names_list) ;

	if ((compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE || compress_mode == ZMAPWINDOW_COMPRESS_MARK))
	  {
	    if (removeNameListsByRange(&names_list, start, end))
	      set_data->hidden_bump_features = TRUE ;
	  }

	list_length = g_list_length(names_list) ;

	zMapPrintTimer(NULL, "Removed features not in range") ;

	/* There's a problem with logic here. see removeNonColinearExtensions! */
	/* Remove non-colinear matches outside range if set. */
	if (mark_set && bump_mode == ZMAPBUMP_NAME_COLINEAR
	    && styles_prop.display_state != ZMAPSTYLE_COLDISPLAY_SHOW)
	  g_list_foreach(names_list, removeNonColinearExtensions, &bump_data) ;


	list_length = g_list_length(names_list) ;


	if (!names_list)
	  {
	    bumped = FALSE ;
	  }
	else
	  {
	    AddGapsDataStruct gaps_data = {set_data->window, &(set_data->gaps_added_items)} ;


	    /* TRY JUST ADDING GAPS  IF A MARK IS SET */
	    if (mark_set && historic_bump_mode != bump_mode)
	      {
		/* What we need to do here is as in the pseudo code.... */
		g_list_foreach(names_list, addGapsCB, &gaps_data) ;

		zMapPrintTimer(NULL, "added gaps") ;
	      }



	    /* Merge the lists into the min. number of non-overlapping lists of features arranged
	     * by name and to some extent by score. */
	    complex.window = set_data->window ;
	    complex.column_group = column_group ;
	    complex.set_data = set_data ;

	    complex.curr_offset = 0.0 ;
	    complex.incr = (width * COMPLEX_BUMP_COMPRESS) ;

	    if (bump_mode == ZMAPBUMP_NAME_INTERLEAVE)
	      complex.overlap_func = listsOverlap ;
	    else if (bump_mode == ZMAPBUMP_NAME_NO_INTERLEAVE || bump_mode == ZMAPBUMP_NAME_BEST_ENDS
		     || bump_mode == ZMAPBUMP_NAME_COLINEAR)
	      complex.overlap_func = listsOverlapNoInterleave ;

	    if (bump_mode == ZMAPBUMP_NAME_BEST_ENDS || bump_mode == ZMAPBUMP_NAME_COLINEAR)
	      g_list_foreach(names_list, setAllOffsetCB, &complex) ;
	    else
	      g_list_foreach(names_list, setOffsetCB, &complex) ;

	    zMapPrintTimer(NULL, "offsets set for subsets of features") ;
	


	    /* we reverse the offsets for reverse strand cols so as to mirror the forward strand. */
	    set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
	    if (set_data->strand == ZMAPSTRAND_REVERSE)
	      reverseOffsets(complex.bumpcol_list) ;


	    /* Bump all the features to their correct offsets. */
	    g_list_foreach(complex.bumpcol_list, moveItemsCB, NULL) ;

	    zMapPrintTimer(NULL, "bumped features to offsets") ;


	    /* TRY JUST ADDING GAPS  IF A MARK IS SET */
	    if (mark_set && bump_mode != ZMAPBUMP_NAME_INTERLEAVE)
	      {
		/* NOTE THERE IS AN ISSUE HERE...WE SHOULD ADD COLINEAR STUFF FOR ALIGN FEATURES
		 * THIS IS NOT EXPLICIT IN THE CODE WHICH IS NOT CORRECT....NEED TO THINK THIS
		 * THROUGH, DON'T JUST MAKE THESE COMPLEX BUMP MODES SPECIFIC TO ALIGNMENTS,
		 * ITS MORE COMPLICATED THAN THAT... */


		/* for no interleave add background items.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		g_list_foreach(complex.bumpcol_list, addBackgrounds, &set_data->extra_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		g_list_foreach(complex.bumpcol_list, addMultiBackgrounds, &set_data->extra_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		/* WE SHOULD ONLY BE DOING THIS FOR ALIGN FEATURES...TEST AT THIS LEVEL.... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		g_list_foreach(complex.bumpcol_list, NEWaddMultiBackgrounds, &set_data->extra_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		g_list_foreach(complex.bumpcol_list, NEWaddMultiBackgrounds, set_data) ;


		zMapPrintTimer(NULL, "added inter align bars etc.") ;
	      }

	  }

	/* Clear up. */
	if (complex.bumpcol_list)
	  g_list_foreach(complex.bumpcol_list, listDataDestroyCB, NULL) ;
	if (complex.name_hash)
	  g_hash_table_destroy(complex.name_hash) ;
	if (names_list)
	  g_list_free(names_list) ;

	 
	break ;
      }
    default:
      zMapAssertNotReached() ;
      break ;
    }


  /* bump all the features for all modes except complex ones and then clear up. */
  if (bumped
      && (bump_mode != ZMAPBUMP_NAME_INTERLEAVE && bump_mode != ZMAPBUMP_NAME_NO_INTERLEAVE
	  && bump_mode != ZMAPBUMP_NAME_BEST_ENDS && bump_mode != ZMAPBUMP_NAME_COLINEAR))
    {
      g_list_foreach(column_features->item_list, bumpColCB, (gpointer)&bump_data) ;

      if (bump_mode == ZMAPBUMP_START_POSITION || bump_mode == ZMAPBUMP_NAME)
	g_hash_table_destroy(bump_data.pos_hash) ;
      else if (bump_mode == ZMAPBUMP_OVERLAP)
	g_list_foreach(bump_data.pos_list, listDataDestroyCB, NULL) ;
    }


  zMapWindowBusy(set_data->window, FALSE) ;

  zMapPrintTimer(NULL, "finished bump") ;

  return ;
}

void zmapWindowColumnBumpAllInitial(FooCanvasItem *column_item)
{
  FooCanvasGroup *strand_container = NULL;

  /* Get the strand level container */
  if((strand_container = zmapWindowContainerGetParentLevel(column_item, ZMAPCONTAINER_LEVEL_STRAND)))
    {
      /* container execute */
      zmapWindowContainerExecute(strand_container, 
				 ZMAPCONTAINER_LEVEL_FEATURESET,
				 invoke_bump_to_initial, NULL);
      /* happy days */

    }

  return ;
}



/* 
 *                Internal routines.
 */


/* Is called once for each item in a column and sets the horizontal position of that
 * item under various "bumping" modes. */
static void bumpColCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  BumpCol bump_data   = (BumpCol)user_data ;
  FooCanvasGroup *column_group =  NULL ;
  ZMapWindowItemFeatureSetData set_data ;
  ZMapFeatureTypeStyle style ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0, dx = 0.0 ;
  ZMapStyleBumpMode bump_mode ;
  gboolean ignore_mark;


  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  bump_mode = bump_data->style_prop.bump_mode ;

  if (bump_data->style_prop.display_state == ZMAPSTYLE_COLDISPLAY_SHOW)
    ignore_mark = TRUE ;
  else
    ignore_mark = FALSE ;


  /* try a range restriction... */
  if (bump_mode != ZMAPBUMP_UNBUMP && !(ignore_mark))
    {
      if (feature->x2 < bump_data->start || feature->x1 > bump_data->end)
	{
	  bump_data->set_data->hidden_bump_features = TRUE ;
	  
	  foo_canvas_item_hide(item) ;
      
	  return ;
	}
    }


  /* If we not bumping all features, then only bump the features who have the bumped style. */
  column_group = zmapWindowContainerGetParentContainerFromItem(item) ;

  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  style = zmapWindowItemFeatureSetStyleFromID(set_data, feature->style_id) ;

  if (!(bump_data->style_prop.bump_all) && bump_data->style_prop.style_id != feature->style_id)
    return ;


  /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
   * calculated them anyway. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;
  
  switch (bump_mode)
    {
    case ZMAPBUMP_START_POSITION:
      {
	/* Bump features over if they have the same start coord. */

	y1_ptr = GINT_TO_POINTER((int)y1) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash, y1_ptr, &key, &value))
	  {
	    offset = *(double *)value + bump_data->incr ;
	    
	    *(double *)value = offset ;
	  }
	else
	  {
	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, y1_ptr, value) ;
	  }
	
	break ;
      }


    case ZMAPBUMP_OVERLAP:
      {
	/* Bump features over if they overlap at all. */
	BumpColRange new_range ;

	new_range = g_new0(BumpColRangeStruct, 1) ;
	new_range->y1 = y1 ;
	new_range->y2 = y2 ;
	new_range->offset = 0.0 ;
	new_range->incr = bump_data->incr ;

	g_list_foreach(bump_data->pos_list, compareListOverlapCB, new_range) ;

	bump_data->pos_list = g_list_append(bump_data->pos_list, new_range) ;

	offset = new_range->offset ;
	
	break ;
      }
    case ZMAPBUMP_NAME:
      {
	/* Bump features that have the same name, i.e. are the same feature, so that each
	 * vertical subcolumn is composed of just one feature in different positions. */
	ZMapFeature feature ;

	feature = (ZMapFeature)(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA)) ;

	if (g_hash_table_lookup_extended(bump_data->pos_hash,
					 GINT_TO_POINTER(feature->original_id), &key, &value))
	  {
	    offset = *(double *)value ;
	  }
	else
	  {
	    offset = bump_data->offset ;

	    value = g_new(double, 1) ;
	    *(double *)value = offset ;
	    
	    g_hash_table_insert(bump_data->pos_hash, GINT_TO_POINTER(feature->original_id), value) ;

	    bump_data->offset += bump_data->incr ;
	  }
	
	break ;
      }
    case ZMAPBUMP_UNBUMP:
    case ZMAPBUMP_ALL:
      {
	offset = bump_data->offset ;
	bump_data->offset += bump_data->incr ;

	break ;
      }
    case ZMAPBUMP_NAVIGATOR:
      {
	/* Bump features over if they overlap at all. */
	BumpColRange new_range ;

	new_range = g_new0(BumpColRangeStruct, 1) ;
	new_range->y1 = y1 ;
	new_range->y2 = y2 ;
	new_range->offset = 0.0 ;
	new_range->incr = x2 - x1 + 1.0;//bump_data->incr ;

	g_list_foreach(bump_data->pos_list, compareListOverlapCB, new_range) ;

	bump_data->pos_list = g_list_append(bump_data->pos_list, new_range) ;

	offset = new_range->offset ;
	
	break ;
      }
      break;
    case ZMAPBUMP_ALTERNATING:
      {
        /* first time through ->offset == 0.0 this is where we need to draw _this_ feature.*/
        /* next time needs to be bumped to the next column (width of _this_ feature) */
        if((offset = bump_data->offset) == 0.0)
          bump_data->offset = x2 - x1 + 1.0;
        else
          bump_data->offset = 0.0; /* back to the first column */
      }
      break;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  if (feature->type != ZMAPSTYLE_MODE_GRAPH)
    {
      /* Some features are drawn with different widths to indicate things like score. In this case
       * their offset needs to be corrected to place them centrally. (We always do this which
       * seems inefficient but its a toss up whether it would be quicker to test (dx == 0). */
      dx = (zMapStyleGetWidth(style) - (x2 - x1)) / 2 ;
      offset += dx ;
    }      

  /* Not having something like this appears to be part of the cause of the oddness. Not all though */
  if (offset < 0.0)
    offset = 0.0;
  
  /* This does a item_get_bounds... don't we already have them? 
   * Might be missing something though. Why doesn't the "Bump" bit calculate offsets? */
  my_foo_canvas_item_goto(item, &(offset), NULL) ; 
  

  return ;
}


/* GDestroyNotify() function for freeing the data part of a hash association. */
static void hashDataDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}


/* GFunc callback func, called from g_list_foreach_find() to test whether current
 * element matches supplied overlap coords. */
static void compareListOverlapCB(gpointer data, gpointer user_data)
{
  BumpColRange curr_range = (BumpColRange)data ;
  BumpColRange new_range = (BumpColRange)user_data ;

  /* Easier to test no overlap and negate. */
  if (!(new_range->y1 > curr_range->y2 || new_range->y2 < curr_range->y1))
    {
      new_range->offset = curr_range->offset + curr_range->incr ;
    }

  return ;
}


/* GFunc callback func, called from g_list_foreach_find() to free list resources. */
static void listDataDestroyCB(gpointer data, gpointer user_data)
{
  g_free(data) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printItem(FooCanvasItem *item, int indent, char *prefix)
{
  int i ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("%s", prefix) ;

  zmapWindowPrintItemCoords(item) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* UNUSED */
static void addToList(gpointer data, gpointer user_data)
{
  FooCanvasGroup *grp = FOO_CANVAS_GROUP(data);
  GList **list = (GList **)user_data;
  *list = g_list_append(*list, grp);
  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







/* 
 *                       Functions for the complex bump...
 * 
 * If you alter this code you should note the careful use of GList**, list addresses
 * can change so its vital to return the address of the list pointer, not the list
 * pointer itself.
 * 
 */

/* Is called once for each item in a column and constructs a hash table in which each entry
 * is a list of all the features with the same name. */
static void makeNameListCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ComplexBump complex = (ComplexBump)user_data ;
  GHashTable *name_hash = complex->name_hash ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  gpointer key = NULL, value = NULL ;
  GList **feature_list_ptr ;
  GList *feature_list ;

  /* don't bother if something is not displayed. */
  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  /* If we are not bumping all the features then only bump those for the specified style. */
  {
    FooCanvasGroup *column_group =  NULL ;
    ZMapWindowItemFeatureSetData set_data ;
    ZMapFeatureTypeStyle style ;

    column_group = zmapWindowContainerGetParentContainerFromItem(item) ;

    set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
    zMapAssert(set_data) ;

    if (!(complex->bump_all) && complex->style_id != feature->style_id)
      return ;

    if(!complex->bump_all && 
       (style = zmapWindowItemFeatureSetStyleFromID(set_data, feature->style_id)))
      {
	/* Try doing this here.... */
	if (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT
	    && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	  complex->protein = TRUE ;
      }
  }





  /* If a list of features with this features name already exists in the hash then simply
   * add this feature to it. Otherwise make a new entry in the hash. */
  if (g_hash_table_lookup_extended(name_hash,
				   GINT_TO_POINTER(feature->original_id), &key, &value))
    {
      feature_list_ptr = (GList **)value ;

      feature_list = *feature_list_ptr ;

      feature_list = g_list_append(feature_list, item) ;

      *feature_list_ptr = feature_list ;
    }
  else
    {
      feature_list_ptr = (GList **)g_malloc0(sizeof(GList **)) ;

      feature_list = NULL ;

      feature_list = g_list_append(feature_list, item) ;

      *feature_list_ptr = feature_list ;

      g_hash_table_insert(name_hash, GINT_TO_POINTER(feature->original_id), feature_list_ptr) ;
    }

  
  return ;
}



static void makeNameListStrandedCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ComplexBump complex = (ComplexBump)user_data ;
  GHashTable *name_hash = complex->name_hash ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  gpointer key = NULL, value = NULL ;
  GList **feature_list_ptr ;
  GList *feature_list ;
  char *feature_strand_name = NULL ;
  GQuark name_quark ;

  /* don't bother if something is not displayed. */
  if(!(zmapWindowItemIsShown(item)))
    return ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  /* If we not bumping all features, then only bump the features who have the bumped style. */
  {
    FooCanvasGroup *column_group =  NULL ;
    ZMapWindowItemFeatureSetData set_data ;
    ZMapFeatureTypeStyle style ;

    column_group = zmapWindowContainerGetParentContainerFromItem(item) ;

    set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
    zMapAssert(set_data) ;

    if (!(complex->bump_all) && complex->style_id != feature->style_id)
      return ;

    if(!complex->bump_all && 
       (style = zmapWindowItemFeatureSetStyleFromID(set_data, feature->style_id)))
      {
	/* Try doing this here.... */
	if (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT
	    && feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	  complex->protein = TRUE ;
      }
  }

  /* Make a stranded name... */
  feature_strand_name = g_strdup_printf("%s-%s",
					g_quark_to_string(feature->original_id),
					(feature->strand == ZMAPSTRAND_NONE ? "none"
					 : feature->strand == ZMAPSTRAND_FORWARD ? "+"
					 : "-")) ;
  name_quark = g_quark_from_string(feature_strand_name) ;


  /* If a list of features with this features name already exists in the hash then simply
   * add this feature to it. Otherwise make a new entry in the hash. */
  if (g_hash_table_lookup_extended(name_hash,
				   GINT_TO_POINTER(name_quark), &key, &value))
    {
      feature_list_ptr = (GList **)value ;

      feature_list = *feature_list_ptr ;

      feature_list = g_list_append(feature_list, item) ;

      *feature_list_ptr = feature_list ;
    }
  else
    {
      feature_list_ptr = (GList **)g_malloc0(sizeof(GList **)) ;

      feature_list = NULL ;

      feature_list = g_list_append(feature_list, item) ;

      *feature_list_ptr = feature_list ;

      g_hash_table_insert(name_hash, GINT_TO_POINTER(name_quark), feature_list_ptr) ;
    }

  g_free(feature_strand_name) ;

  
  return ;
}





/* GHFunc(), gets the list stored in the hash and adds it to the supplied list of lists.
 * Essentially this routine makes a list out of a hash. */
static void getListFromHash(gpointer key, gpointer value, gpointer user_data)
{
  GList **this_list_ptr = (GList **)value ;
  GList **names_list_out = (GList **)user_data ;
  GList *names_list = *names_list_out ;

  names_list = g_list_append(names_list, this_list_ptr) ;

  *names_list_out = names_list ;

  return ;
}


/* GFunc() to sort each sub list of features in this list by position. */
static void sortListPosition(gpointer data, gpointer user_data)
{
  GList **name_list_ptr = (GList **)data ;
  GList *name_list = *name_list_ptr ;			    /* Single list of named features. */

  /* sort the sublist of features. */
  name_list = zmapWindowItemSortByPostion(name_list) ;

  *name_list_ptr = name_list ;

  return ;
}



/* This may be a candidate to be a more general function to filter sets of features by range....
 * Checks all the features in each feature list within names_list to see if they overlap the
 * given feature item, removes from names_list any that don't.
 * Returns TRUE if any items were _removed_. */
static gboolean removeNameListsByRange(GList **names_list, int start, int end)
{
  gboolean result = FALSE ;
  GList *new_list, *list_item ;
  RangeDataStruct range_data ;

  range_data.overlap = FALSE ;

  range_data.start = start ;
  range_data.end = end ;

  new_list = NULL ;
  list_item = *names_list ;
  while (list_item)
    {
      /* testRangeCB() sets overlap to TRUE when it detects one. */
      g_list_foreach(*((GList **)(list_item->data)), testRangeCB, &range_data) ;

      /* Tricky list logic here, g_list_delete_link() has the effect of moving us on to the next
       * list item so we mustn't do g_list_next() as well. */
      if (!range_data.overlap)
	{
	  /* Now hide these items as they don't overlap... */
	  g_list_foreach(*((GList **)(list_item->data)), hideItemsCB, NULL) ;

	  list_item = g_list_delete_link(list_item, list_item) ;

	  result = TRUE ;
	}
      else
	{
	  new_list = list_item ;			    /* Hang on to valid list ptr. */
	  list_item = g_list_next(list_item) ;
	  range_data.overlap = FALSE ;
	}
    }


  *names_list = g_list_first(new_list) ;		    /* Hand back the _start_ of the list. */

  return result ;
}



/* GFunc() to test whether a set of features overlaps the range given in user_data, sets
 * TRUE when an overlap is detected. Features that don't ovelap are hidden. */
static void testRangeCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  RangeData range_data = (RangeData)user_data ;
  ZMapFeature feature ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  if (!(feature->x1 > range_data->end || feature->x2 < range_data->start))
    range_data->overlap = TRUE ;

  return ;
}


/* GFunc() to hide all items in a list. */
static void hideItemsCB(gpointer data, gpointer user_data_unused)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  foo_canvas_item_hide(item) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* UNUSED */
/* GFunc() to add a background item to each set of items in a list. */
static void addBackgrounds(gpointer data, gpointer user_data)
{
  ComplexCol col_data = (ComplexCol)data ;
  GList **extras_ptr = (GList **)user_data ;
  GList *extra_items = *extras_ptr ;
  GList *name_list = col_data->feature_list ;			    /* Single list of named features. */
  GList *first, *last ;
  FooCanvasItem *first_item, *last_item, *background ;
  static gboolean colour_init = FALSE ;
  static GdkColor perfect, colinear, noncolinear ;
  char *perfect_colour = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;
  double x1, y1, x2, y2 ;

  if (!colour_init)
    {
      gdk_color_parse(perfect_colour, &perfect) ;
      gdk_color_parse(colinear_colour, &colinear) ;
      gdk_color_parse(noncolinear_colour, &noncolinear) ;

      colour_init = TRUE ;
    }


  /* SHOULD FILTER ON TYPE AS WELL...WE DON'T WANT ALL TYPES TO HAVE BACKGROUNDS....???? */
  if (g_list_length(name_list) > 1)
    {
      GList *backgrounds = NULL ;
      GList *list_item ;
      GQuark first_id = 0 ;
      ZMapFeatureTypeStyle first_style = NULL ;
      double mid, half_width = COLUMN_BACKGROUND_SPACING ;

      first = last = NULL ;
      list_item = g_list_first(name_list) ;

      while (list_item)
	{
	  FooCanvasItem *item ;
	  ZMapFeature feature ;
	  GQuark feature_id = 0 ;

	  item = (FooCanvasItem *)list_item->data ;
	  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
	  zMapAssert(feature) ;
	  feature_id = feature->original_id ;

	  if (!first)
	    {
	      first = last = list_item ;
	      first_item = (FooCanvasItem *)first->data ;
	      
	      /* N.B. this works because the items have already been positioned in the column. */
	      foo_canvas_item_get_bounds(first_item, &x1, &y1, &x2, NULL) ;
	      mid = (x2 + x1) * 0.5 ;

	      first_id = feature_id ;
	      first_style = feature->style ;
	      list_item = g_list_next(list_item) ;
	    }
	  else
	    {
	      if (feature_id == first_id)
		{
		  last = list_item ;
		  list_item = g_list_next(list_item) ;
		}

	      if (feature_id != first_id || !list_item)
		{
		  /* Only do boxes where there is more than one item. */
		  if (first != last)
		    {
		      ZMapWindowItemFeatureBumpData bump_data ;

		      last_item = (FooCanvasItem *)last->data ;

		      foo_canvas_item_get_bounds(last_item, NULL, NULL, NULL, &y2);

		      bump_data = g_new0(ZMapWindowItemFeatureBumpDataStruct, 1) ;
		      bump_data->first_item = first_item ;
		      bump_data->feature_id = first_id ;
		      bump_data->style = first_style ;

		      background = zMapDrawBox(FOO_CANVAS_GROUP(first_item->parent),
					       (mid - half_width), y1, (mid + half_width), y2,
					       &perfect, &perfect, 0.0) ;

		      zmapWindowLongItemCheck(col_data->window->long_items, background, y1, y2) ;

		      extra_items = g_list_append(extra_items, background) ;

		      g_object_set_data(G_OBJECT(background), ITEM_FEATURE_TYPE,
					GINT_TO_POINTER(ITEM_FEATURE_GROUP_BACKGROUND)) ;
		      g_object_set_data(G_OBJECT(background), ITEM_FEATURE_BUMP_DATA, bump_data) ;
		      g_signal_connect(GTK_OBJECT(background), "event",
				       GTK_SIGNAL_FUNC(bumpBackgroundEventCB), col_data->window) ;
		      g_signal_connect(GTK_OBJECT(background), "destroy",
				       GTK_SIGNAL_FUNC(bumpBackgroundDestroyCB), col_data->window) ;


		      /* Make sure these backgrounds are drawn behind the features items. */
		      foo_canvas_item_lower_to_bottom(background) ;

		      backgrounds = g_list_append(backgrounds, background) ;
		    }
		  
		  /* N.B. ensure we don't move on in the list so that we pick up this item
		   * as the next "first" item... */
		  first = NULL ;
		}

	    }
	}


      /* Need to add all backgrounds to col list here and lower them in group.... */
      name_list = g_list_concat(name_list, backgrounds) ;


      col_data->feature_list = name_list ;

      *extras_ptr = extra_items ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Draw any items that have gaps but were drawn as a simple block as a series
 * of blocks showing the internal gaps of the alignment. */
static void addGapsCB(gpointer data, gpointer user_data)
{
  GList *feature_list = *((GList **)(data)) ;
  AddGapsData gaps_data = (AddGapsData)user_data ;
  ZMapWindow window = gaps_data->window ;
  GList *gaps_added_items = *(gaps_data->gaps_added_items) ;
  GList *list_item ;

  list_item = feature_list ;
  do
    {
      FooCanvasItem *item = (FooCanvasItem *)(list_item->data) ;
      ZMapFeature feature = NULL ;
      ZMapWindowItemFeatureSetData set_data = NULL;
      ZMapFeatureTypeStyle style = NULL;
      FooCanvasGroup *set_group ;

      /* Get the feature. */
      feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
      zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;


      /* Get the features active style (the one on the canvas. */
      set_group = zmapWindowContainerGetParentContainerFromItem(item) ;
      zMapAssert(set_group) ;
      set_data = g_object_get_data(G_OBJECT(set_group), ITEM_FEATURE_SET_DATA) ;
      zMapAssert(set_data) ;

      style = zmapWindowItemFeatureSetStyleFromID(set_data, feature->style_id) ;

      /* Only display gaps on bumping and if the alignments have gaps. */
      if (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT)
	{
	  if (feature->feature.homol.align)
	    {
	      gboolean prev_align_gaps ;

	      prev_align_gaps = zMapStyleIsAlignGaps(style) ;

	      /* Turn align gaps "on" in the style and then draw.... */
	      zMapStyleSetAlignGaps(style, TRUE) ;

	      item = zMapWindowFeatureReplace(window, item, feature, FALSE) ;
							    /* replace feature with itself. */
	      zMapAssert(item) ;

	      list_item->data = item ;			    /* replace item in list ! */

	      gaps_added_items = g_list_append(gaps_added_items, item) ; /* Record in a our list
									    of gapped items. */

	      /* Now reset align_gaps to whatever it was. */
	      zMapStyleSetAlignGaps(style, prev_align_gaps) ;
	    }
	}

    } while ((list_item = g_list_next(list_item))) ;


  /* Return the gaps_added items... */
  *(gaps_data->gaps_added_items) = gaps_added_items ;

  return ;
}



/* Redraw item drawn with gaps as a single block.... */
static void removeGapsCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapFeature feature ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;
  zMapAssert(feature->type == ZMAPSTYLE_MODE_ALIGNMENT) ;

  /* Note the logic here....if we are in this routine its because the feature _is_
   * and alignment AND its style is set to no gaps, so unlike when we draw we don't
   * have to set any flags.... */
#ifdef RT_63281
  if(FOO_CANVAS_ITEM(item)->object.flags & FOO_CANVAS_ITEM_VISIBLE)
    {
#endif /* RT_63281 */

      item = zMapWindowFeatureReplace(window, item, feature, FALSE) ;
      /* replace feature with itself. */
      zMapAssert(item) ;

#ifdef RT_63281
    }
#endif /* RT_63281 */

  return ;
}





static void removeNonColinearExtensions(gpointer data, gpointer user_data)
{
  GList **name_list_ptr = (GList **)data ;
  GList *name_list = *name_list_ptr ;			    /* Single list of named features. */
  BumpCol bump_data = (BumpCol)user_data ;
  GList *list_item ;
  gboolean result ;
  GList *first_list_item, *last_list_item ;
  FooCanvasItem *first_item ;
  ZMapFeature first_feature ;
  int mark_start = bump_data->start, mark_end = bump_data->end ;


  /* Get the very first list item. */
  list_item = g_list_first(name_list) ;

  /* exclude any non-alignment cols here by returning...n.b. all their items should be hidden.... */
  result = getFeatureFromListItem(list_item, &first_item, &first_feature) ;
  zMapAssert(result) ;

  /* We only do aligns, anything else we hide... */
  if (first_feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
    {
      /* Now hide these items as they don't overlap... */
      g_list_foreach(name_list, hideItemsCB, NULL) ;

      return ;
    }


  /* Find first and last item(s) that is within the marked range. */
  result = findRangeListItems(list_item, mark_start, mark_end, &first_list_item, &last_list_item) ;
  /* Logic Error!  
   * zmapWindowColumnBumpRange() doesn't always remove the names lists not within the range.
   * This means there will be names list not in range therefore not found by findRangeListItems()!
   * calling zmapWindowColumnBump(item, ZMAPBUMP_NAME_COLINEAR) when marked is a sure way to 
   * find this out! RDS.
   */
  zMapAssert(result) ;

  list_item = first_list_item ;

  result = getFeatureFromListItem(list_item, &first_item, &first_feature) ;
  zMapAssert(result) ;

  name_list = removeNonColinear(first_list_item, ZMAP_GLIST_REVERSE, bump_data) ;

  {
    GList *list_item ;
    FooCanvasItem *item ;
    ZMapFeature feature ;

    list_item = name_list ;

    result = getFeatureFromListItem(list_item, &item, &feature) ;
    if (!result)
      printf("found bad list\n") ;
  }
      
  name_list = removeNonColinear(last_list_item, ZMAP_GLIST_FORWARD, bump_data) ;

  {
    GList *list_item ;
    FooCanvasItem *item ;
    ZMapFeature feature ;

    list_item = name_list ;

    result = getFeatureFromListItem(list_item, &item, &feature) ;
    if (!result)
      printf("found bad list\n") ;
  }


  /* Reset potentially altered list in callers data struct. */
  if (*name_list_ptr != name_list)
    printf("found not equal name lists...\n") ;

  *name_list_ptr = name_list ;

  return ;
}


/* 
 * There was/is a problem with this function.  Before FooCanvasItems
 * are mapped calling foo_canvas_item_get_bounds will return 0.0 for
 * all coordinates.  This was causing the bug
 * https://rt.sanger.ac.uk/rt/Ticket/Display.html?id=68459
 *

 * Ed, I've changed the function to stop the crashing, but there is a
 * slight problem with the alignment (vertical) of same name items
 * now.  I haven't investigated the cause, but I do have a fool proof
 * way to repoduce using the following database
 * ~rds/acedb_sessions/ib2_update_info_crash/

 * Mark AC019068.1-001 (longest Known-CDS transcript)
 * Turn On the EST Mouse Column
 * Bump the EST Mouse Column
 * Rev-Comp
 * Turn On the EST Mouse Column and Em:CO798510.1 has a green line 
 * (longest one) between one HSP and another, but the another is not
 * in the same column...

 */

/* GFunc() to add background items indicating goodness of match to each set of items in a list.
 * 
 * Note that each set of items may represent more than one match, the matches won't overlap but
 * there may be more than one. Hence the more complicated code.
 * 
 * THIS FUNCTION HAS SOME HARD CODED STUFF AND QUITE A BIT OF CODE THAT CAN BE REFACTORED TO
 * SIMPLIFY THINGS, I'VE CHECKED IT IN NOW BECAUSE I WANT IT TO BE IN CVS OVER XMAS.
 * 
 *  */
static void NEWaddMultiBackgrounds(gpointer data, gpointer user_data)
{
  ComplexCol col_data = (ComplexCol)data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GList **extras_ptr = (GList **)user_data ;
  GList *extra_items = *extras_ptr ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapWindowItemFeatureSetData set_data = (ZMapWindowItemFeatureSetData)user_data ;
  GList *extra_items = set_data->extra_items ;

  GList *name_list = col_data->feature_list ;			    /* Single list of named features. */
  static gboolean colour_init = FALSE ;
  static GdkColor perfect, colinear, noncolinear ;
  char *perfect_colour = ZMAP_WINDOW_MATCH_PERFECT ;
  char *colinear_colour = ZMAP_WINDOW_MATCH_COLINEAR ;
  char *noncolinear_colour = ZMAP_WINDOW_MATCH_NOTCOLINEAR ;
  GList *backgrounds = NULL ;
  GList *list_item ;
  double width, mid, half_width ;
  FooCanvasItem *item ;
  FooCanvasItem *background ;
  ZMapFeature prev_feature = NULL, curr_feature = NULL ;
  ZMapFeatureTypeStyle prev_style = NULL, curr_style = NULL ;
  GQuark prev_id = 0, curr_id = 0 ;
  int prev_end = 0, curr_start = 0 ;
  double x1, x2, prev_y1, prev_y2, curr_y1, curr_y2 ;
  double start_x1, start_x2, end_x1, end_x2 ;

  /* IN THIS FUNCTION _DO_NOT_ CALL
   * FOO_CANVAS_ITEM_LOWER_TO_BOTTOM. THIS IS A _VERY_ TIME CONSUMING
   * CALL WHICH SEARCHES THE LIST FOR THE ITEM BEFORE LOWERING
   * IT!!! */

  if (!colour_init)
    {
      gdk_color_parse(perfect_colour, &perfect) ;
      gdk_color_parse(colinear_colour, &colinear) ;
      gdk_color_parse(noncolinear_colour, &noncolinear) ;

      colour_init = TRUE ;
    }

  /* Get the first item. */
  list_item = g_list_first(name_list) ;
  item      = FOO_CANVAS_ITEM(list_item->data);

  /* We only pay attention to the x coords so we can find the midpoint*/
  foo_canvas_item_get_bounds(item, &x1, NULL, &x2, NULL) ;
  curr_feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(curr_feature) ;


  /* THIS IS WRONG IT NEEDS TO CALCULATE OFFSET (+ 1) CORRECTLY!! */
  /* THIS IS _NOT_ THE ONLY PLACE! */
  curr_y1 = curr_feature->x1 + 1;
  curr_y2 = curr_feature->x2;


  /* Only makes sense to add this colinear bars for alignment features.... */
  if (curr_feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
    return ;

  curr_id = curr_feature->original_id ;
  curr_style = zmapWindowItemFeatureSetStyleFromID(set_data, curr_feature->style_id) ;


  /* Calculate horizontal mid point of this column of matches from the first item,
   * N.B. this works because the items have already been positioned in the column. */
  mid = (x2 + x1) * 0.5 ;

  width = zMapStyleGetWidth(curr_style) ;
  half_width = width * 0.5 ;

  col_data->width = width ;

  /* CODE HERE WORKS BUT IS NOT CORRECT IN THAT IT IS USING THE CANVAS BOX COORDS WHEN IT
   * SHOULD BE USING THE FEATURE->X1/X2 COORDS...i'LL FIX IT LATER... */
  do
    {
      unsigned int match_threshold ;
      GdkColor *box_colour ;
      ZMapWindowItemFeatureBumpData bump_data ;
      int query_seq_end, align_end ;
      gboolean incomplete ;

      /* mark start of curr item if its incomplete. */
      incomplete = FALSE ;
      if (curr_feature->feature.homol.y1 > curr_feature->feature.homol.y2)
	{
	  if (col_data->window->revcomped_features)
	    {
	      query_seq_end = 1 ;
	      align_end = curr_feature->feature.homol.y2 ;

	      if (query_seq_end < align_end)
		incomplete = TRUE ;
	    }
	  else
	    {
	      query_seq_end = curr_feature->feature.homol.length ;
	      align_end = curr_feature->feature.homol.y1 ;

	      if (query_seq_end > align_end)
		incomplete = TRUE ;
	    }
	}
      else
	{
	  if (col_data->window->revcomped_features)
	    {
	      query_seq_end = curr_feature->feature.homol.length ;
	      align_end = curr_feature->feature.homol.y2 ;

	      if (query_seq_end > align_end)
		incomplete = TRUE ;
	    }
	  else
	    {
	      query_seq_end = 1 ;
	      align_end = curr_feature->feature.homol.y1 ;

	      if (query_seq_end < align_end)
		incomplete = TRUE ;
	    }
	}

      if (incomplete)
	{
	  double box_start, box_end, mid ;

	  bump_data = g_new0(ZMapWindowItemFeatureBumpDataStruct, 1) ;
	  bump_data->first_item = item ;
	  bump_data->feature_id = curr_id ;
	  bump_data->style = curr_style ;

	  {
	    double test_1, test_2 ;

	    foo_canvas_item_get_bounds(item, &test_1, NULL, &test_2, NULL) ;
	  }

	  itemGetCoords(item, &start_x1, &start_x2) ;
	  mid = start_x1 + ((start_x2 - start_x1) / 2) ;
	  
	  box_start = curr_y1 - 10 ;
	  box_end = curr_y1 + 10 ;

	  box_colour = &noncolinear ;
	  background = makeMatchItem(col_data->window->long_items,
				     FOO_CANVAS_GROUP(item->parent), ZMAPDRAW_OBJECT_BOX,
				     mid - 1, box_start,
				     mid + 1, box_end,
				     box_colour, bump_data, col_data->window) ;

	  backgrounds = g_list_append(backgrounds, background) ;
		      
	  extra_items = g_list_append(extra_items, background) ;
	}


      /* make curr in to prev item */
      prev_feature = curr_feature ;
      prev_id = curr_feature->original_id ;

      if (col_data->window->revcomped_features)
	prev_end = curr_feature->feature.homol.y1 ;
      else
	prev_end = curr_feature->feature.homol.y2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      prev_style = curr_feature->style ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      prev_style = curr_style ;

      prev_y1 = curr_y1 ;
      prev_y2 = curr_y2 ;


      /* Mark in between matches for their colinearity. */
      while ((list_item = g_list_next(list_item)))
	{
	  /* get new curr item */
	  item = (FooCanvasItem *)list_item->data ;
	  /* As the comment says at the top this is completely wrong and if the item isn't mapped curr_y1/y2 end up as 0! */
	  /* foo_canvas_item_get_bounds(item, NULL, &curr_y1, NULL, &curr_y2); */

	  curr_feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
	  zMapAssert(curr_feature) ;
	  curr_id = curr_feature->original_id ;
	  /* THIS IS WRONG IT NEEDS TO CALCULATE OFFSET (+ 1) CORRECTLY!! */
	  curr_y1 = curr_feature->x1 + 1;
	  curr_y2 = curr_feature->x2;

	  if (col_data->window->revcomped_features)
	    curr_start = curr_feature->feature.homol.y2 ;
	  else
	    curr_start = curr_feature->feature.homol.y1 ;

	  curr_style = zmapWindowItemFeatureSetStyleFromID(set_data, curr_feature->style_id) ;

	  /* We only do aligns (remember that there can be different types in a single col)
	   * and only those for which joining of homols was requested. */
	  if (zMapStyleGetMode(curr_style) != ZMAPSTYLE_MODE_ALIGNMENT
	      || !(zMapStyleGetJoinAligns(curr_style, &match_threshold)))
	    continue ;


	  /* If we've changed to a new match then break, otherwise mark for colinearity. */
	  if (curr_id != prev_id)
	    break ;
	  else
	    {
	      ColinearityType colinearity ;

	      /* Checks that adjacent matches do not overlap and then checks for colinearity.
	       * Note that matches are most likely to overlap where a false end to an HSP has been made. */
	      colinearity = featureHomolIsColinear(col_data->window, col_data->match_threshold, 
						   prev_feature, curr_feature) ;

	      if (colinearity != COLINEAR_INVALID)
		{
		  double real_start, real_end ;

		  if (colinearity == COLINEAR_NOT)
		    box_colour = &noncolinear ;
		  else if (colinearity == COLINEAR_IMPERFECT)
		    box_colour = &colinear ;
		  else
		    box_colour = &perfect ;


		  /* make line + box but up against alignment boxes. */
		  real_start = prev_y2 - 1 ;
		  real_end = curr_y1 + 1 ;

		  bump_data = g_new0(ZMapWindowItemFeatureBumpDataStruct, 1) ;
		  bump_data->first_item = item ;
		  bump_data->feature_id = prev_id ;
		  bump_data->style = prev_style ;



		  /* Make line... */
		  background = makeMatchItem(col_data->window->long_items,
					     FOO_CANVAS_GROUP(item->parent), ZMAPDRAW_OBJECT_LINE,
					     mid, real_start, mid, real_end,
					     box_colour, bump_data, col_data->window) ;


		  extra_items = g_list_append(extra_items, background) ;

		  backgrounds = g_list_append(backgrounds, background) ;


		  /* Make invisible box.... */
		  bump_data = g_new0(ZMapWindowItemFeatureBumpDataStruct, 1) ;
		  bump_data->first_item = item ;
		  bump_data->feature_id = prev_id ;
		  bump_data->style = prev_style ;

		  background = makeMatchItem(col_data->window->long_items,
					     FOO_CANVAS_GROUP(item->parent), ZMAPDRAW_OBJECT_BOX,
					     (mid - half_width), real_start, (mid + half_width), real_end,
					     NULL,
					     bump_data, col_data->window) ;

		  extra_items = g_list_append(extra_items, background) ;

		  backgrounds = g_list_append(backgrounds, background) ;

		}
	      

	      /* make curr into prev */
	      prev_feature = curr_feature ;
	      prev_id = curr_feature->original_id ;

	      if (col_data->window->revcomped_features)
		prev_end = curr_feature->feature.homol.y1 ;
	      else
		prev_end = curr_feature->feature.homol.y2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      prev_style = curr_feature->style ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	      prev_style = curr_style ;

	      prev_y1 = curr_y1 ;
	      prev_y2 = curr_y2 ;
	    }
	}
      

      /* Mark start/end of final feature if its incomplete. */
      incomplete = FALSE ;
      if (prev_feature->feature.homol.y1 > prev_feature->feature.homol.y2)
	{
	  if (col_data->window->revcomped_features)
	    {
	      query_seq_end = prev_feature->feature.homol.length ;
	      align_end = prev_feature->feature.homol.y1 ;

	      if (query_seq_end > align_end)
		incomplete = TRUE ;
	    }
	  else
	    {
	      query_seq_end = 1 ;
	      align_end = prev_feature->feature.homol.y2 ;

	      if (query_seq_end < align_end)
		incomplete = TRUE ;
	    }

	}
      else
	{
	  if (col_data->window->revcomped_features)
	    {
	      query_seq_end = 1 ;
	      align_end = prev_feature->feature.homol.y1 ;

	      if (query_seq_end < align_end)
		incomplete = TRUE ;
	    }
	  else
	    {
	      query_seq_end = prev_feature->feature.homol.length ;
	      align_end = prev_feature->feature.homol.y2 ;

	      if (query_seq_end > align_end)
		incomplete = TRUE ;
	    }
	}

      if (incomplete)
	{
	  double box_start, box_end, mid ;

	  box_start = prev_y2 - 10 ;
	  box_end = prev_y2 + 10 ;

	  bump_data = g_new0(ZMapWindowItemFeatureBumpDataStruct, 1) ;
	  bump_data->first_item = item ;
	  bump_data->feature_id = prev_feature->original_id ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  bump_data->style = prev_feature->style ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  bump_data->style = prev_style ;

	  itemGetCoords(item, &end_x1, &end_x2) ;
	  mid = end_x1 + ((end_x2 - end_x1) / 2) ;

	  box_colour = &noncolinear ;
	  background = makeMatchItem(col_data->window->long_items,
				     FOO_CANVAS_GROUP(item->parent), ZMAPDRAW_OBJECT_BOX,
				     mid - 1, box_start,
				     mid + 1, box_end,
				     box_colour, bump_data, col_data->window) ;

	  backgrounds = g_list_append(backgrounds, background) ;

	  extra_items = g_list_append(extra_items, background) ;
	}

    } while (list_item) ;


  /* Need to add all backgrounds to col list here and lower them in group.... */
  name_list = g_list_concat(name_list, backgrounds) ;

  col_data->feature_list = name_list ;

  set_data->extra_items = extra_items ;

  return ;
}




static FooCanvasItem *makeMatchItem(ZMapWindowLongItems long_items,
				    FooCanvasGroup *parent, ZMapDrawObjectType shape,
				    double x1, double y1, double x2, double y2,
				    GdkColor *colour,
				    gpointer item_data, gpointer event_data)
{
  FooCanvasItem *match_item ;

  if (shape == ZMAPDRAW_OBJECT_LINE)
    {
      match_item = zMapDrawLineFull(parent, FOO_CANVAS_GROUP_BOTTOM,
				    x1, y1, x2, y2, 
				    colour, 1.0) ;
    }
  else
    {
      match_item = zMapDrawBoxFull(parent, FOO_CANVAS_GROUP_BOTTOM,
				   x1, y1, x2, y2,
				   colour, colour, 0.0) ;
    }

  g_object_set_data(G_OBJECT(match_item), ITEM_FEATURE_TYPE,
		    GINT_TO_POINTER(ITEM_FEATURE_GROUP_BACKGROUND)) ;
  g_object_set_data(G_OBJECT(match_item), ITEM_FEATURE_BUMP_DATA, item_data) ;
  g_signal_connect(GTK_OBJECT(match_item), "event",
		   GTK_SIGNAL_FUNC(bumpBackgroundEventCB), event_data) ;
  g_signal_connect(GTK_OBJECT(match_item), "destroy",
		   GTK_SIGNAL_FUNC(bumpBackgroundDestroyCB), event_data) ;


  zmapWindowLongItemCheck(long_items, match_item, y1, y2) ;


  return match_item ;
}


static void freeExtraItems(gpointer data, gpointer user_data_unused)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;

  gtk_object_destroy(GTK_OBJECT(item)) ;

  return ;
}



/* We only reshow items if they were hidden by our code, any user hidden items are kept hidden.
 * Code is slightly complex here because the user hidden items are a stack of lists of hidden items. */
static void showItems(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowItemFeatureSetData set_data = (ZMapWindowItemFeatureSetData)user_data ;
  GList *found_item = NULL ;

  if (!(found_item = g_queue_find_custom(set_data->user_hidden_stack, item, findItemInQueueCB)))
    foo_canvas_item_show(item) ;

  return ;
}

static gint findItemInQueueCB(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  GList *item_list = (GList *)a ;
  FooCanvasItem *item = (FooCanvasItem *) b ;
  GList *found_item = NULL ;

  if ((found_item = g_list_find(item_list, item)))
    result = 0 ;

  return result ;
}







/* GCompareFunc() to sort two lists of features by the average score of all their features. */
static gint sortByScoreCB(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;					    /* make a == b default. */
  GList **feature_list_out_1 = (GList **)a ;
  GList *feature_list_1 = *feature_list_out_1 ;		    /* Single list of named features. */
  GList **feature_list_out_2 = (GList **)b ;
  GList *feature_list_2 = *feature_list_out_2 ;		    /* Single list of named features. */
  double list_score_1 = 0.0, list_score_2 = 0.0 ;

  /* Add up the scores for all features in the sublists. */
  g_list_foreach(feature_list_1, listScoreCB, &list_score_1) ;
  g_list_foreach(feature_list_2, listScoreCB, &list_score_2) ;

  /* average them by the number of features. */
  list_score_1 = list_score_1 / (double)(g_list_length(feature_list_1)) ;
  list_score_2 = list_score_2 / (double)(g_list_length(feature_list_2)) ;

  /* Highest scores go first. */
  if (list_score_1 > list_score_2)
    result = -1 ;
  else if (list_score_1 < list_score_2)
    result = 1 ;

  return result ;
}




/* GFunc() to return a features score. */
static void listScoreCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  double score = *((double *)user_data) ;

  /* don't bother if something is not displayed. */
  if((zmapWindowItemIsShown(item)))
    {
      ZMapWindowItemFeatureType item_feature_type ;
      ZMapFeature feature ;

      /* Sanity checks... */
      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
      zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

      feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
      zMapAssert(feature) ;
      
      /* Can only compare by score if features have a score... */
      if (feature->flags.has_score)
	{
	  score += feature->score ;
	      
	  *((double *)user_data) = score ;
	}
    }

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* UNUSED */
/* GCompareFunc() to sort two lists of features by the overall span of all their features. */
static gint sortBySpanCB(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;					    /* make a == b default. */
  GList **feature_list_out_1 = (GList **)a ;
  GList *feature_list_1 = *feature_list_out_1 ;		    /* Single list of named features. */
  GList **feature_list_out_2 = (GList **)b ;
  GList *feature_list_2 = *feature_list_out_2 ;		    /* Single list of named features. */
  FooCanvasItem *item ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  int list_span_1 = 0, list_span_2 = 0 ;
  

  feature_list_1 = g_list_first(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_1 = feature->x1 ;

  feature_list_1 = g_list_last(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_1 = feature->x1 - list_span_1 + 1 ;


  feature_list_2 = g_list_first(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_2 = feature->x1 ;

  feature_list_2 = g_list_last(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_2 = feature->x1 - list_span_2 + 1 ;
  
  /* Highest spans go first. */
  if (list_span_1 > list_span_2)
    result = -1 ;
  else if (list_span_1 < list_span_2)
    result = 1 ;

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* GCompareFunc() to sort two lists of features by strand and then the overall span of all their features. */
static gint sortByStrandSpanCB(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;					    /* make a == b default. */
  GList **feature_list_out_1 = (GList **)a ;
  GList *feature_list_1 = *feature_list_out_1 ;		    /* Single list of named features. */
  GList **feature_list_out_2 = (GList **)b ;
  GList *feature_list_2 = *feature_list_out_2 ;		    /* Single list of named features. */
  FooCanvasItem *item ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  int list_span_1 = 0, list_span_2 = 0 ;
  ZMapStrand strand_1, strand_2 ;

  feature_list_1 = g_list_first(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_1 = feature->x1 ;
  strand_1 = feature->strand ;

  feature_list_1 = g_list_last(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_1 = feature->x1 - list_span_1 + 1 ;


  feature_list_2 = g_list_first(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_2 = feature->x1 ;
  strand_2 = feature->strand ;

  feature_list_2 = g_list_last(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  list_span_2 = feature->x1 - list_span_2 + 1 ;
  

  /* Strand first, then span. */
  if (strand_1 < strand_2)
    result = -1 ;
  else if (strand_1 > strand_2)
    result = 1 ;
  else
    {
      /* Highest spans go first. */
      if (list_span_1 > list_span_2)
	result = -1 ;
      else if (list_span_1 < list_span_2)
	result = 1 ;
    }

  return result ;
}


/* GCompareFunc() to sort two lists of features by strand and then by how close their 5' and 3'
 * matches are to the range start/end. */
static gint sortByOverlapCB(gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint result = 0 ;					    /* make a == b default. */
  GList **feature_list_out_1 = (GList **)a ;
  GList *feature_list_1 = *feature_list_out_1 ;		    /* Single list of named features. */
  GList **feature_list_out_2 = (GList **)b ;
  GList *feature_list_2 = *feature_list_out_2 ;		    /* Single list of named features. */
  RangeData range = (RangeData)user_data ;
  FooCanvasItem *item ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  int top_1, top_2, bot_1, bot_2 ;
  ZMapStrand strand_1, strand_2 ;

  feature_list_1 = g_list_first(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  top_1 = feature->x1 ;
  strand_1 = feature->strand ;
  bot_1 = feature->x2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  feature_list_1 = g_list_last(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  bot_1 = feature->x2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  feature_list_2 = g_list_first(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  top_2 = feature->x1 ;
  strand_2 = feature->strand ;
  bot_2 = feature->x2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  feature_list_2 = g_list_last(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  bot_2 = feature->x2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  

  /* Strand first, then overlap. */
  if (strand_1 < strand_2)
    result = -1 ;
  else if (strand_1 > strand_2)
    result = 1 ;
  else
    {
      RangeDataStruct range_1 = {FALSE}, range_2 = {FALSE} ;

      range_1.start = range_2.start = range->start ;
      range_1.end = range_2.end = range->end ;

      /* Find the closest 5' or 3' match (for forward or reverse aligns) to the start or end
       * respectively of the mark range, */
      g_list_foreach(feature_list_1, bestFitCB, &range_1) ;

      g_list_foreach(feature_list_2, bestFitCB, &range_2) ;

      /* sort by nearest to 5' or 3' and then by longest match after that. */
      if (range_1.feature_diff < range_2.feature_diff)
	result = -1 ;
      else if (range_1.feature_diff == range_2.feature_diff)
	{
	  if (abs(range_1.feature->x1 - range_1.feature->x2) > abs(range_2.feature->x1 - range_2.feature->x2))
	    result = -1 ;
	  else
	    result = 1 ;
	}
      else
	result = 1 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* LEAVE UNTIL I'VE FNISHED TESTING THE BUMP STUFF.... */
      if (strand_1 == ZMAPSTRAND_FORWARD)
	{
	  fit_1 = abs(range->start - top_1) + abs(range->start - bot_1) ;
	  fit_2 = abs(range->start - top_2) + abs(range->start - bot_2) ;
	}
      else if (strand_1 == ZMAPSTRAND_REVERSE)
	{
	  fit_1 = abs(range->end - top_1) + abs(range->end - bot_1) ;
	  fit_2 = abs(range->end - top_2) + abs(range->end - bot_2) ;
	}
      else
	{
	  fit_1 = abs(range->start - top_1) + abs(range->end - bot_1) ;
	  fit_2 = abs(range->start - top_2) + abs(range->end - bot_2) ;
	}

      /* Best fit to range goes first. */
      if (fit_1 < fit_2)
	result = -1 ;
      else if (fit_1 > fit_2)
	result = 1 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return result ;
}


/* A GFunc() to find the best fitting match to the start or end of the range in a list. */
static void bestFitCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  RangeData range = (RangeData)user_data ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapFeature feature ;
  int diff ;
  
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Sort forward matches by closest to 5' and reverse by closest to 3' */
  if (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE)
    diff = abs(range->start - feature->x1) ;
  else
    diff = abs(range->end - feature->x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Sort all matches by closest to 5' end. */
  diff = abs(range->start - feature->x1) ;



  if (!(range->feature))
    {
      range->feature_diff = diff ;
      range->feature = feature ;
    }
  else if (diff < range->feature_diff)
    {
      range->feature_diff = diff ;
      range->feature = feature ;
    }

  return ;
}







#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* GFunc() to return a features span. */
static void listSpanCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  double span = *((double *)user_data) ;

  /* don't bother if something is not displayed. */
  if((zmapWindowItemIsShown(item)))
    {
      ZMapWindowItemFeatureType item_feature_type ;
      ZMapFeature feature ;

      /* Sanity checks... */
      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
      zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT) ;

      feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
      zMapAssert(feature) ;
      
      /* Can only compare by span if features have a span... */
      if (feature->flags.has_score)
	{
	  score += feature->score ;
	      
	  *((double *)user_data) = score ;
	}
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* GFunc() to try to combine lists of features, if the lists have any overlapping features,
 * then the new list must be placed at a new offset in the column. */
static void setOffsetCB(gpointer data, gpointer user_data)
{
  GList **name_list_ptr = (GList **)data ;
  GList *name_list = *name_list_ptr ;			    /* Single list of named features. */
  ComplexBump complex = (ComplexBump)user_data ;
  FeatureDataStruct feature_data ;

  feature_data.name_list = &name_list ;
  feature_data.overlap_func = complex->overlap_func ;

  /* If theres already a list of columns then try to add this set of name features to one of them
   * otherwise create a new column. */
  if (!complex->bumpcol_list
      || zMap_g_list_cond_foreach(complex->bumpcol_list, featureListCB, &feature_data))
    {
      ComplexCol col ;
      double max_width = 0.0 ;

      g_list_foreach(name_list, getMaxWidth, &max_width) ;

      col = g_new0(ComplexColStruct, 1) ;
      col->window = complex->window ;
      col->column_group = complex->column_group ;
      col->set_data = complex->set_data ;
      col->offset = complex->curr_offset ;
      col->feature_list = name_list ;

      complex->bumpcol_list = g_list_append(complex->bumpcol_list, col) ;

      complex->curr_offset += max_width ;

    }

  return ;
}

/* Check to see if two lists overlap, if they don't then merge the two lists.
 * NOTE that this is non-destructive merge, features are not moved from one list to the
 * other, see the glib docs for _list_concat(). */
static gboolean featureListCB(gpointer data, gpointer user_data)
{
  gboolean call_again = TRUE ;
  ComplexCol col = (ComplexCol)data ;
  FeatureData feature_data = (FeatureData)user_data ;
  GList *name_list = *(feature_data->name_list) ;


  if (!(feature_data->overlap_func)(col->feature_list, name_list))
    {
      col->feature_list = g_list_concat(col->feature_list, name_list) ;

      /* Having done the merge we _must_ resort the list by position otherwise subsequent
       * overlap tests will fail. */
      col->feature_list = zmapWindowItemSortByPostion(col->feature_list) ;

      call_again = FALSE ;
    }

  return call_again ;
}



/* GFunc() to try to combine lists of features, if the lists have any overlapping features,
 * then the new list must be placed at a new offset in the column. */
static void setAllOffsetCB(gpointer data, gpointer user_data)
{
  GList **name_list_ptr = (GList **)data ;
  GList *name_list = *name_list_ptr ;			    /* Single list of named features. */
  ComplexBump complex = (ComplexBump)user_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  FeatureDataStruct feature_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  feature_data.name_list = &name_list ;
  feature_data.overlap_func = complex->overlap_func ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* If theres already a list of columns then try to add this set of name features to one of them
   * otherwise create a new column. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (!complex->bumpcol_list
      || zMap_g_list_cond_foreach(complex->bumpcol_list, feature2ListCB, &feature_data))
    {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      ComplexCol col ;
      double max_width = 0.0 ;

      g_list_foreach(name_list, getMaxWidth, &max_width) ;


      col = g_new0(ComplexColStruct, 1) ;
      col->window = complex->window ;
      col->column_group = complex->column_group ;
      col->set_data = complex->set_data ;
      col->offset = complex->curr_offset ;
      col->feature_list = name_list ;

      complex->bumpcol_list = g_list_append(complex->bumpcol_list, col) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      complex->curr_offset += complex->incr ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      complex->curr_offset += max_width ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


/* GFunc() to find max width of canvas items in a list. */
static void getMaxWidth(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  double *max_width = (double *)user_data ;
  double x1, x2, width ;
  ZMapFeature feature = NULL ;
  ZMapWindowItemFeatureType feature_type ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (zmapWindowItemIsCompound(item))
    {
      true_item = zmapWindowItemGetNthChild(FOO_CANVAS_GROUP(item), 0) ;
    }
  else
    true_item = item ;


  if (FOO_IS_CANVAS_GROUP(true_item))
    {
      printf("we shouldn't be here\n") ;
    }


  if (FOO_IS_CANVAS_POLYGON(true_item))
    {
      FooCanvasPoints *points = NULL ;
      int i ;

      g_object_get(G_OBJECT(true_item),
		   "points", &points,
		   NULL) ;

      x1 = x2 = points->coords[0] ;
      for (i = 0 ; i < points->num_points ; i += 2)
	{
	  if (points->coords[i] < x1)
	    x1 = points->coords[i] ;
	  if (points->coords[i] > x2)
	    x2 = points->coords[i] ;
	}
    }
  else if (FOO_IS_CANVAS_RE(true_item))
    {
      g_object_get(G_OBJECT(true_item),
		   "x1", &x1,
		   "x2", &x2,
		   NULL) ;
    }
  else
    {
      printf("agh....can't cope with this\n") ;
    }

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  itemGetCoords(item, &x1, &x2) ;

  width = x2 - x1 + 1.0 ;

  if (width > *max_width)
    *max_width = width ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Check to see if two lists overlap, if they don't then merge the two lists.
 * NOTE that this is non-destructive merge, features are not moved from one list to the
 * other, see the glib docs for _list_concat(). */
static gboolean featureList2CB(gpointer data, gpointer user_data)
{
  gboolean call_again = TRUE ;
  ComplexCol col = (ComplexCol)data ;
  FeatureData feature_data = (FeatureData)user_data ;
  GList *name_list = *(feature_data->name_list) ;


  if (!(feature_data->overlap_func)(col->feature_list, name_list))
    {
      col->feature_list = g_list_concat(col->feature_list, name_list) ;

      /* Having done the merge we _must_ resort the list by position otherwise subsequent
       * overlap tests will fail. */
      col->feature_list = zmapWindowItemSortByPostion(col->feature_list) ;

      call_again = FALSE ;
    }

  return call_again ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* Compare two lists of features, return TRUE if any two features in the two lists overlap,
 * return FALSE otherwise.
 * 
 * NOTE that this function _completely_ relies on the two feature lists having been sorted
 * into ascending order on their coord positions, it will fail dismally if this is not true.
 *  */
static gboolean listsOverlap(GList *curr_features, GList *new_features)
{
  gboolean overlap = FALSE ;
  GList *curr_ptr, *new_ptr ;

  curr_ptr = g_list_first(curr_features) ;
  new_ptr = g_list_first(new_features) ;
  do
    {
      FooCanvasItem *curr_item = (FooCanvasItem *)(curr_ptr->data) ;
      FooCanvasItem *new_item = (FooCanvasItem *)(new_ptr->data) ;
      ZMapFeature curr_feature ;
      ZMapFeature new_feature ;

      curr_feature = g_object_get_data(G_OBJECT(curr_item), ITEM_FEATURE_DATA) ;
      zMapAssert(curr_feature) ;

      new_feature = g_object_get_data(G_OBJECT(new_item), ITEM_FEATURE_DATA) ;
      zMapAssert(new_feature) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("%s (%d, %d) vs. %s (%d, %d)",
	     g_quark_to_string(curr_feature->original_id), curr_feature->x1, curr_feature->x2,
	     g_quark_to_string(new_feature->original_id), new_feature->x1, new_feature->x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (new_feature->x2 < curr_feature->x1)
	{
	  new_ptr = g_list_next(new_ptr) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("-->  no overlap\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else if (new_feature->x1 > curr_feature->x2)
	{
	  curr_ptr = g_list_next(curr_ptr) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("-->  no overlap\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else
	{
	  overlap = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("-->  OVERLAP\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}

    } while (!overlap && curr_ptr && new_ptr) ;


  return overlap ;
}



/* Compare two lists of features, return TRUE if the two lists of features are interleaved
 * in position, return FALSE otherwise.
 * 
 * NOTE that this function _completely_ relies on the two feature lists having been sorted
 * into ascending order on their coord positions, it will fail dismally if this is not true.
 *  */
static gboolean listsOverlapNoInterleave(GList *curr_features, GList *new_features)
{
  gboolean overlap = TRUE ;
  GList *curr_ptr, *new_ptr ;
  FooCanvasItem *curr_item ;
  FooCanvasItem *new_item ;
  ZMapFeature curr_first, curr_last ;
  ZMapFeature new_first, new_last ;



  curr_ptr = g_list_first(curr_features) ;
  curr_item = (FooCanvasItem *)(curr_ptr->data) ;
  curr_first = g_object_get_data(G_OBJECT(curr_item), ITEM_FEATURE_DATA) ;
  zMapAssert(curr_first) ;

  curr_ptr = g_list_last(curr_features) ;
  curr_item = (FooCanvasItem *)(curr_ptr->data) ;
  curr_last = g_object_get_data(G_OBJECT(curr_item), ITEM_FEATURE_DATA) ;
  zMapAssert(curr_last) ;

  new_ptr = g_list_first(new_features) ;
  new_item = (FooCanvasItem *)(new_ptr->data) ;
  new_first = g_object_get_data(G_OBJECT(new_item), ITEM_FEATURE_DATA) ;
  zMapAssert(new_first) ;

  new_ptr = g_list_last(new_features) ;
  new_item = (FooCanvasItem *)(new_ptr->data) ;
  new_last = g_object_get_data(G_OBJECT(new_item), ITEM_FEATURE_DATA) ;
  zMapAssert(new_last) ;

  /* We just want to make sure the two lists do not overlap in any of their positions. */
  if (new_first->x1 > curr_last->x2
      || new_last->x2 < curr_first->x1)
    overlap = FALSE ;

  return overlap ;
}



/* Reverses the offsets for the supplied list...for displaying on the reverse strand. */
static void reverseOffsets(GList *bumpcol_list)
{
  GList *first = g_list_first(bumpcol_list) ;
  GList *last = g_list_last(bumpcol_list) ;
  ComplexCol col_data_first, col_data_last ;
  int i ;
  int length = g_list_length(bumpcol_list) / 2 ;

  for (i = 0 ; i < length ; i++)
    {
      double tmp ;

      col_data_first = (ComplexCol)(first->data) ;
      col_data_last = (ComplexCol)(last->data) ;

      tmp = col_data_first->offset ;
      col_data_first->offset = col_data_last->offset ;
      col_data_last->offset = tmp ;

      first = g_list_next(first) ;
      last = g_list_previous(last) ;
    }

  return ;
}




/* These two callbacks go through the list of lists of features moving all their canvas items
 * into the correct place within their columns. */
static void moveItemsCB(gpointer data, gpointer user_data)
{
  ComplexCol col_data = (ComplexCol)data ;

  g_list_foreach(col_data->feature_list, moveItemCB, col_data) ;

  return ;
}


static void moveItemCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ComplexCol col_data = (ComplexCol)user_data ;
  ZMapFeature feature ;
  double dx = 0.0, offset = 0.0 ;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0 ;
  ZMapFeatureTypeStyle style ;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;

  /* Get hold of the style. */
  style = zmapWindowItemFeatureSetStyleFromID(col_data->set_data, feature->style_id) ;

  /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
   * calculated them anyway. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  /* Some features are drawn with different widths to indicate things like score. In this case
   * their offset needs to be corrected to place them centrally. (We always do this which
   * seems inefficient but its a toss up whether it would be quicker to test (dx == 0). */
  dx = ((zMapStyleGetWidth(style) * COMPLEX_BUMP_COMPRESS) - (x2 - x1)) / 2 ;

  offset = col_data->offset + dx ;

  my_foo_canvas_item_goto(item, &offset, NULL) ; 

  return ;
}


/* Called for styles in a column hash set of styles, just sets bump mode. */
static void setStyleBumpCB(ZMapFeatureTypeStyle style, gpointer user_data)
{
  ZMapStyleBumpMode bump_mode = GPOINTER_TO_INT(user_data) ;

  zMapStyleSetBumpMode(style, bump_mode) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* UNUSED */
static gint compareNameToColumn(gconstpointer list_data, gconstpointer user_data)
{
  gint result = -1 ;
  FooCanvasGroup *col_group = (FooCanvasGroup *)list_data ;
  GQuark col_name = GPOINTER_TO_INT(user_data) ;
  ZMapFeatureSet feature_set ;

  /* We can have empty columns with no feature set so maynot be able to get feature set name.... */
  if ((feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_DATA)))
    {
      if (feature_set->unique_id == col_name)
	result = 0 ;
    }

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/*
 *   Functions to draw as "normal" (i.e. not read frame) the frame sensitive columns
 *   from the canvas.
 */

/* Callback for any events that happen on bump background items.
 * 
 *  */
static gboolean bumpBackgroundEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindowStruct*)data ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;

	/* Button 1 is handled, button 3 is not as there is no applicable item menu,
	 * 2 is left for a general handler which could be the root handler. */
	if (but_event->button == 1)
	  {
	    ZMapWindowItemFeatureType item_feature_type ;
	    ZMapWindowItemFeatureBumpData bump_data ;
	    ZMapWindowSelectStruct select = {0} ;
	    gboolean replace_highlight = TRUE ;
	    ZMapFeature feature ;

	    if (zMapGUITestModifiers(but_event, GDK_SHIFT_MASK))
	      {
		if (zmapWindowFocusIsItemInHotColumn(window->focus, item))
		  replace_highlight = FALSE ;
	      }

	    item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
								  ITEM_FEATURE_TYPE)) ;
	    zMapAssert(item_feature_type == ITEM_FEATURE_GROUP_BACKGROUND) ;


	    /* Retrieve the feature item info from the canvas item. */
	    bump_data = (ZMapWindowItemFeatureBumpData)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_BUMP_DATA) ;  
	    zMapAssert(bump_data) ;

	    /* Pass back details for display to the user to our caller. */
	    select.feature_desc.feature_name = (char *)g_quark_to_string(bump_data->feature_id) ;

	    select.feature_desc.feature_set = (char *)g_quark_to_string(zMapStyleGetID(bump_data->style)) ;

	    feature = g_object_get_data(G_OBJECT(bump_data->first_item), ITEM_FEATURE_DATA) ;
	    zMapAssert(feature) ;

	    select.secondary_text = zmapWindowFeatureSetDescription((ZMapFeatureSet)feature) ;

	    select.highlight_item = bump_data->first_item ;
	    select.replace_highlight_item = replace_highlight ;
            select.type = ZMAPWINDOW_SELECT_SINGLE;

	    (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

	    g_free(select.secondary_text) ;

	    event_handled = TRUE ;
	  }

	break ;
      }
    default:
      {
	/* By default we _don't_ handle events. */
	event_handled = FALSE ;

	break ;
      }
    }
  
  return event_handled ;
}




/* Callback for destroy of feature items... */
static gboolean bumpBackgroundDestroyCB(FooCanvasItem *feature_item, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also get run. */
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindow window = (ZMapWindowStruct*)data ;
  ZMapWindowItemFeatureBumpData bump_data ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_TYPE)) ;
  zMapAssert(item_feature_type == ITEM_FEATURE_GROUP_BACKGROUND) ;

  /* Retrieve the feature item info from the canvas item. */
  bump_data = (ZMapWindowItemFeatureBumpData)g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_BUMP_DATA) ;  
  zMapAssert(bump_data) ;
  g_free(bump_data) ;

  zmapWindowLongItemRemove(window->long_items, feature_item) ;  /* Ignore boolean result. */

  return event_handled ;
}



/* Get the x coords of a drawn feature item, this is more complicated for a compound item where
 * we have to get a child and then transform its coords back to the compound items space.
 * the parent. */
static gboolean itemGetCoords(FooCanvasItem *item, double *x1_out, double *x2_out)
{
  gboolean result = TRUE ;
  FooCanvasItem *true_item ;
  double x1, x2, parent_x1, parent_x2 ;

  if (zmapWindowItemIsCompound(item))
    {
      true_item = zmapWindowItemGetNthChild(FOO_CANVAS_GROUP(item), 0) ;

      g_object_get(G_OBJECT(item),
		   "x", &parent_x1,
		   "y", &parent_x2,
		   NULL) ;
    }
  else
    true_item = item ;


  if (FOO_IS_CANVAS_POLYGON(true_item))
    {
      FooCanvasPoints *points = NULL ;
      int i ;

      g_object_get(G_OBJECT(true_item),
		   "points", &points,
		   NULL) ;

      x1 = x2 = points->coords[0] ;
      for (i = 0 ; i < points->num_points ; i += 2)
	{
	  if (points->coords[i] < x1)
	    x1 = points->coords[i] ;
	  if (points->coords[i] > x2)
	    x2 = points->coords[i] ;
	}
    }
  else if (FOO_IS_CANVAS_RE(true_item))
    {
      g_object_get(G_OBJECT(true_item),
		   "x1", &x1,
		   "x2", &x2,
		   NULL) ;
    }
  else if(FOO_IS_CANVAS_TEXT(true_item))
    {
      
    }
  else
    {
      zMapAssertNotReached() ;
    }

  if (item != true_item)
    {
      x1 += parent_x1 ;
      x2 += parent_x1 ;
    }

  *x1_out = x1 ;
  *x2_out = x2 ;

  return result ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* 
 *             debug functions, please leave here for future use.
 */


static void printChild(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZMapFeatureAny any_feature ;

  any_feature = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_DATA) ;

  printf("%s ", g_quark_to_string(any_feature->unique_id)) ;

  return ;
}



static void printPtr(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;

  printf("%p ", container) ;

  return ;
}


static void printQuarks(gpointer data, gpointer user_data)
{


  printf("quark str: %s\n", g_quark_to_string(GPOINTER_TO_INT(data))) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static gboolean getFeatureFromListItem(GList *list_item, FooCanvasItem **item_out, ZMapFeature *feature_out)
{
  gboolean result = FALSE ;

  if (list_item)
    {
      FooCanvasItem *item ;
      ZMapFeature feature ;

      item = (FooCanvasItem *)list_item->data ;
      if (FOO_IS_CANVAS_ITEM(item))
	{
	  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
	  if (zMapFeatureIsValidFull((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURE))
	    {
	      result = TRUE ;
	      *item_out = item ;
	      *feature_out = feature ;
	    }
	}
    }

  return result ;
}


/* N.B. This function only searches forward in the list from the given search_start,
 * it also assumes that the list contains matches for a single query sequence. */
static gboolean findRangeListItems(GList *search_start, int seq_start, int seq_end,
				   GList **first_out, GList **last_out)
{
  gboolean result = FALSE, search_result ;
  GList *list_item = search_start, *first = NULL, *last = NULL ;
  FooCanvasItem *item ;
  ZMapFeature feature ;



  /* Find first item that is within the sequence coord range. */
  do
    {
      search_result = getFeatureFromListItem(list_item, &item, &feature) ;
      zMapAssert(search_result) ;			    /* canvas item _must_ have feature attached. */

      if (feature->x1 > seq_end || feature->x2 < seq_start)
	{
	  continue ;
	}
      else
	{
	  first = list_item ;
	  break ;
	}
    }
  while ((list_item = g_list_next(list_item))) ;


  /* If we found the first item then find the last item within the given sequence coord range. */
  if (first)
    {
      GList *prev = NULL ;

      /* Note that we find the last one by going past it and then stepping one back. */
      do
	{
	  search_result = getFeatureFromListItem(list_item, &item, &feature) ;
	  zMapAssert(search_result) ;			    /* canvas item _must_ have feature attached. */

	  if (feature->x1 <= seq_end)
	    {
	      prev = list_item ;	      
	    }
	  else if (feature->x1 > seq_end)
	    {
	      break ;
	    }
	}
      while ((list_item = g_list_next(list_item))) ;

      last = prev ;
    }

  if (first && last)
    {
      result = TRUE ;
      *first_out = first ;
      *last_out = last ;
    }
  else
    {
      printf("found one without start/end feature in range...\n") ;
#ifdef DEBUG_NAMES_LIST_OUT_OF_RANGE
      printf("Feature '%s'\n", g_quark_to_string(feature->unique_id));
      printf("  ft: start = %d, end = %d\n", feature->x1, feature->x2);
      printf("  seq start = %d, end = %d\n", seq_start, seq_end);
      printf("  list length = %d, real = %d\n", g_list_length(search_start), g_list_length(g_list_first(search_start)));
#endif /* DEBUG_NAMES_LIST_OUT_OF_RANGE */
    }

  return result ;
}




static GList *removeNonColinear(GList *first_list_item, ZMapGListDirection direction, BumpCol bump_data)
{
  GList *final_list = NULL ;
  GList *next_item ;
  GList *prev_list_item, *curr_list_item ;
  FooCanvasItem *prev_item, *curr_item ;
  ZMapFeature prev_feature, curr_feature ;
  gboolean result, still_colinear ;

  still_colinear = TRUE ;


  /* Set up previous item. */
  prev_list_item = first_list_item ;
  result = getFeatureFromListItem(first_list_item, &prev_item, &prev_feature) ;
  zMapAssert(result) ;					    /* canvas item _must_ have feature attached. */


  /* Now chonk through the rest of the items.... */
  next_item = first_list_item ;
  while (TRUE)
    {
      /* Hideous coding but we can't use func. pointers because g_list_previous/next are macros... */
      if (direction == ZMAP_GLIST_FORWARD)
	{
	  if (!(next_item = g_list_next(next_item)))
	    break ;
	}
      else
	{
	  if (!(next_item = g_list_previous(next_item)))
	    break ;
	}


      curr_list_item = next_item ;
      result = getFeatureFromListItem(next_item, &curr_item, &curr_feature) ;
      zMapAssert(result) ;

      if (still_colinear)
	{
	  ColinearityType colinearity ;

	  /* Swap to simplify later coding. */
	  if (direction == ZMAP_GLIST_REVERSE)
	    {
	      ZMapFeature tmp ;

	      tmp = prev_feature ;
	      prev_feature = curr_feature ;
	      curr_feature = tmp ;
	    }

	  colinearity = featureHomolIsColinear(bump_data->window, bump_data->style_prop.match_threshold, 
					       prev_feature, curr_feature) ;
	  if (colinearity == COLINEAR_INVALID || colinearity == COLINEAR_NOT)
	    still_colinear = FALSE ;


	  /* swap back... */
	  if (direction == ZMAP_GLIST_REVERSE)
	    {
	      ZMapFeature tmp ;

	      tmp = prev_feature ;
	      prev_feature = curr_feature ;
	      curr_feature = tmp ;
	    }
	}

      if (still_colinear)
	{
	  prev_item = curr_item ;
	  prev_feature = curr_feature ;
	}
      else
	{
	  /* surely we need direction here...sigh.... */
	  if (direction == ZMAP_GLIST_FORWARD)
	    {
	      foo_canvas_item_hide(curr_item) ;
	      next_item = g_list_delete_link(curr_list_item, curr_list_item) ;
	      next_item = prev_list_item ;

	      {
		GList *list_item ;
		FooCanvasItem *item ;
		ZMapFeature feature ;
		
		list_item = next_item ;

		result = getFeatureFromListItem(list_item, &item, &feature) ;
	      }

	    }
	  else
	    {
	      foo_canvas_item_hide(curr_item) ;
	      next_item = g_list_delete_link(curr_list_item, curr_list_item) ;
	      next_item = prev_list_item ;
	      
	      {
		GList *list_item ;
		FooCanvasItem *item ;
		ZMapFeature feature ;
		
		list_item = next_item ;

		result = getFeatureFromListItem(list_item, &item, &feature) ;
	      }

	    }
	}
    }


  /* Return the new list, note that first_list_item should never be deleted. */
  final_list = g_list_first(first_list_item) ;

  return final_list ;
}


/* Function to check whether the homol blocks for two features are colinear.
 * 
 * features are alignment features and have a match threshold
 * 
 * features are on same strand
 * 
 * features are in correct order, i.e. feat_1 is 5' of feat_2
 * 
 * features have the same style
 * 
 * Returns COLINEAR_INVALID if the features overlap or feat_2 is 5' of feat_1,
 * otherwise returns COLINEAR_NOT if homols are not colinear,
 * COLINEAR_IMPERFECT if they are colinear but there is missing sequence in the match,
 * COLINEAR_PERFECT if they are colinear and there is no missing sequence within the
 * threshold given in the style.
 * 
 *  */
static ColinearityType featureHomolIsColinear(ZMapWindow window,  unsigned int match_threshold,
					      ZMapFeature feat_1, ZMapFeature feat_2)
{
  ColinearityType colinearity = COLINEAR_INVALID ;
  int diff ;

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feat_1, ZMAPFEATURE_STRUCT_FEATURE));
  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feat_2, ZMAPFEATURE_STRUCT_FEATURE));

  zMapAssert(feat_1->style_id == feat_2->style_id) ;

  zMapAssert(feat_1->original_id == feat_2->original_id);
  zMapAssert(feat_1->strand == feat_2->strand);

  if(0)
    zMapLogQuark(feat_1->original_id) ;

  if (feat_1->x2 < feat_2->x1)
    {
      int prev_end = 0, curr_start = 0 ;

      /* When revcomp'd homol blocks come in reversed order but their coords are not reversed
       * by the revcomp so we must compare top of first with bottom of second etc. */
      if (feat_1->feature.homol.y1 < feat_1->feature.homol.y2)
	{
	  if (window->revcomped_features)
	    prev_end = feat_2->feature.homol.y2 ;
	  else
	    prev_end = feat_1->feature.homol.y2 ;

	  if (window->revcomped_features)
	    curr_start = feat_1->feature.homol.y1 ;
	  else
	    curr_start = feat_2->feature.homol.y1 ;
	}
      else
	{
	  if (window->revcomped_features)
	    prev_end = feat_1->feature.homol.y1 ;
	  else
	    prev_end = feat_2->feature.homol.y1 ;

	  if (window->revcomped_features)
	    curr_start = feat_2->feature.homol.y2 ;
	  else
	    curr_start = feat_1->feature.homol.y2 ;
	}

      /* Watch out for arithmetic here, remember that if block coords are one apart
       * in the _right_ direction then it's a perfect match. */
      diff = abs(prev_end - (curr_start - 1)) ;
      if (diff > match_threshold)
	{
	  if (curr_start < prev_end)
	    colinearity = COLINEAR_NOT ;
	  else
	    colinearity = COLINEAR_IMPERFECT ;
	}
      else
	colinearity = COLINEAR_PERFECT ;
    }

  return colinearity ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Checks to see if the start or end of a match is truncated. */
gboolean featureHomolIsTruncated(ZMapWindow window, ZMapFeature feature, gboolean is_start)
{
  gboolean incomplete = FALSE ;
  gboolean is_revcomped = window->revcomped_features ;


  if (feature->feature.homol.y1 > feature->feature.homol.y2)
    {
      if (window->revcomped_features)
	{
	  query_seq_end = 1 ;
	  align_end = feature->feature.homol.y2 ;

	  if (query_seq_end < align_end)
	    incomplete = TRUE ;
	}
      else
	{
	  query_seq_end = feature->feature.homol.length ;
	  align_end = feature->feature.homol.y1 ;

	  if (query_seq_end > align_end)
	    incomplete = TRUE ;
	}
    }
  else
    {
      if (window->revcomped_features)
	{
	  query_seq_end = feature->feature.homol.length ;
	  align_end = feature->feature.homol.y2 ;

	  if (query_seq_end > align_end)
	    incomplete = TRUE ;
	}
      else
	{
	  query_seq_end = 1 ;
	  align_end = feature->feature.homol.y1 ;

	  if (query_seq_end < align_end)
	    incomplete = TRUE ;
	}
    }

  return incomplete ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* We don't have enough state in the styles when it comes to bump mode.  
 * Here I wanted to get the initial bump mode from the style as I don't
 * want to unbump the transcripts. */
static ZMapStyleBumpMode hack_initial_mode(ZMapFeatureTypeStyle style)
{
  ZMapStyleBumpMode initial_mode = ZMAPBUMP_UNBUMP;
  ZMapStyleMode style_mode = ZMAPSTYLE_MODE_INVALID;

  style_mode = zMapStyleGetMode(style);

  switch(style_mode)
    {
    case ZMAPSTYLE_MODE_TRANSCRIPT:
      initial_mode = zMapStyleGetBumpMode(style);
      break;
    case ZMAPSTYLE_MODE_INVALID:
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_RAW_SEQUENCE:
    case ZMAPSTYLE_MODE_PEP_SEQUENCE:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_META:
    default:
      initial_mode = ZMAPBUMP_UNBUMP;
      break;
    }

  return initial_mode;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void invoke_bump_to_initial(FooCanvasGroup *container, FooCanvasPoints *points, 
				   ZMapContainerLevelType level, gpointer user_data)
{

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ZMapStyleBumpMode default_mode, current_mode, initial_mode;
	ZMapWindowItemFeatureSetData set_data ;

	set_data = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA) ;

	current_mode = zmapWindowItemFeatureSetGetBumpMode(set_data);
	default_mode = zmapWindowItemFeatureSetGetDefaultBumpMode(set_data);

#warning DISCUSS_WITH_ED
#warning NEED_INITIAL_BUMP_MODE
#ifdef CHECK_THIS_OUT
	initial_mode = hack_initial_mode(style);
#endif

	/* Probably need to zmapWIndowItemFeatureSetHackInitialBumpModes(set_data); */

#warning HACKED_FOR_NOW_TRANSCRIPTS_WILL_UNBUMP_TOO
	initial_mode = current_mode;

	zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container), initial_mode, ZMAPWINDOW_COMPRESS_ALL);
      }
      break;
    default:
      break;
    }

  return ;
}
