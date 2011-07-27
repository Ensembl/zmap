/*  File: zmapWindowColBump.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Column bumping functions.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowFeatures.h>
#include <zmapWindowCanvas.h>


typedef struct
{
  ZMapWindow                    window;
  ZMapWindowContainerFeatureSet container;
  GQuark                        style_id;
  ZMapStyleBumpMode             overlap_mode;
  ZMapStyleColumnDisplayState   display_state;
  unsigned int                  match_threshold;
  gboolean                      bump_all;
} BumpPropertiesStruct, *BumpProperties;

/* For straight forward bumping. */
typedef struct
{
  GHashTable          *pos_hash ;
  GList               *pos_list ;

  double               offset;
  double               incr ;
  double spacing ;

  int                  start;
  int                  end ;

  BumpPropertiesStruct bump_prop_data;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
  double incr ;
  int column;
} BumpColRangeStruct, *BumpColRange ;

typedef struct
{
  double x, width;
}OffsetWidthStruct, *OffsetWidth;


/* THERE IS A BUG IN THE CODE THAT USES THIS...IT MOVES FEATURES OUTSIDE OF THE BACKGROUND SO
 * IT ALL LOOKS NAFF, PROBABLY WE GET AWAY WITH IT BECAUSE COLS ARE SO WIDELY SPACED OTHERWISE
 * THEY MIGHT OVERLAP. I THINK THE MOVING CODE MAY BE AT FAULT. */
/* For complex bump users seem to want columns to overlap a bit, if this is set to 1.0 there is
 * no overlap, if you set it to 0.5 they will overlap by a half and so on. */
#define COMPLEX_BUMP_COMPRESS 1.0

typedef struct
{
  ZMapWindow window ;
  ZMapWindowContainerFeatureSet container;
} AddGapsDataStruct, *AddGapsData ;

typedef gboolean (*OverLapListFunc)(GList *curr_features, GList *new_features) ;

typedef GList* (*GListTraverseFunc)(GList *list) ;

typedef struct
{
  BumpProperties  bump_properties;

  GHashTable     *name_hash ;
  GList          *bumpcol_list ;
  double          curr_offset ;
  double          incr ;
  OverLapListFunc overlap_func ;
  gboolean        protein ;
  GString        *temp_buffer;

  ZMapWindowContainerFeatureSet feature_set ;

} ComplexBumpStruct, *ComplexBump ;


typedef struct
{
  BumpProperties bump_properties;
  double         incr ;
  double         offset ;
  double         width; 	/* this will be removed! */
  GList          *feature_list ;
  ZMapWindowContainerFeatureSet feature_set ;
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


typedef gboolean (*MakeNameListNamePredicateFunc)(FooCanvasItem *item, ComplexBump complex_bump);
typedef GQuark   (*MakeNameListNameKeyFunc)      (ZMapFeature feature, ComplexBump complex_bump);

typedef struct
{
  MakeNameListNamePredicateFunc predicate_func;
  MakeNameListNameKeyFunc       quark_gen_func;
} MakeNameListsHashDataStruct, *MakeNameListsHashData;


static void bumpColCB(gpointer data, gpointer user_data) ;


//static void compareListOverlapCB(gpointer data, gpointer user_data) ;
//static gint overlap_cmp(gconstpointer a,gconstpointer b);

static gboolean featureListCB(gpointer data, gpointer user_data) ;

static gint sortByScoreCB(gconstpointer a, gconstpointer b) ;

static gint sortByStrandSpanCB(gconstpointer a, gconstpointer b) ;
static gint sortByOverlapCB(gconstpointer a, gconstpointer b, gpointer user_data) ;
static void bestFitCB(gpointer data, gpointer user_data) ;
static void listScoreCB(gpointer data, gpointer user_data) ;
static void makeNameListCB(gpointer data, gpointer user_data) ;

static void makeNameListStrandedCB(gpointer data, gpointer user_data) ;


static void setOffsetCB(gpointer data, gpointer user_data) ;
static void setAllOffsetCB(gpointer data, gpointer user_data) ;
static void getMaxWidth(gpointer data, gpointer user_data) ;
static void sortListPosition(gpointer data, gpointer user_data) ;


static void addGapsCB(gpointer data, gpointer user_data) ;
#ifdef NEVER
static void removeGapsCB(gpointer data, gpointer user_data) ;
#endif

static gboolean listsOverlap(GList *curr_features, GList *new_features) ;
static gboolean listsOverlapNoInterleave(GList *curr_features, GList *new_features) ;
static void reverseOffsets(GList *bumpcol_list) ;
static void moveItemsCB(gpointer data, gpointer user_data) ;
static void moveItemCB(gpointer data, gpointer user_data) ;
#ifdef NEVER
static void freeExtraItems(gpointer data, gpointer user_data) ;
#endif
static void showItems(gpointer data, gpointer user_data) ;
static gboolean removeNameListsByRange(GList **names_list, int start, int end) ;
static void testRangeCB(gpointer data, gpointer user_data) ;
static void hideItemsCB(gpointer data, gpointer user_data_unused) ;
static void hashDataDestroyCB(gpointer data) ;
static void listDataDestroyCB(gpointer data, gpointer user_data) ;
static void getListFromHash(gpointer key, gpointer value, gpointer user_data) ;
/*static void setStyleBumpCB(ZMapFeatureTypeStyle style, gpointer user_data) ;*/

static gint findItemInQueueCB(gconstpointer a, gconstpointer b) ;


static void collection_add_colinear_cb(gpointer data, gpointer user_data) ;
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

static void invoke_bump_to_initial(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				   ZMapContainerLevelType level, gpointer user_data);
static void invoke_bump_to_unbump(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data);










/* Merely a cover function for the real bumping code function zmapWindowColumnBumpRange(). */
void zmapWindowColumnBump(FooCanvasItem *column_item, ZMapStyleBumpMode bump_mode)
{
  ZMapWindowCompressMode compress_mode ;
  ZMapWindow window ;

  g_return_if_fail(ZMAP_IS_CONTAINER_FEATURESET(column_item));

  window = g_object_get_data(G_OBJECT(column_item), ZMAP_WINDOW_POINTER) ;
  zMapAssert(window) ;

  if (zmapWindowMarkIsSet(window->mark))
    compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
  else
    compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

  zmapWindowColumnBumpRange(column_item, bump_mode, compress_mode) ;

  return ;
}

void zmapWindowContainerShowAllHiddenFeatures(ZMapWindowContainerFeatureSet container_set)
{
  gboolean hidden_features ;

  g_object_get(G_OBJECT(container_set),
	       "hidden-bump-features", &hidden_features,
	       NULL) ;

  if (hidden_features)
    {
      FooCanvasGroup *column_features;

      column_features = (FooCanvasGroup *)(zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_set));

      g_list_foreach(column_features->item_list, showItems, container_set) ;

      g_object_set(G_OBJECT(container_set),
		   "hidden-bump-features", FALSE,
		   NULL) ;
    }

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
void zmapWindowColumnBumpRange(FooCanvasItem *bump_item, ZMapStyleBumpMode bump_mode,
			       ZMapWindowCompressMode compress_mode)
{
  BumpColStruct bump_data = {NULL} ;
  FooCanvasGroup *column_features ;
  ZMapWindowContainerFeatureSet container = NULL;
  BumpPropertiesStruct bump_properties = {NULL};
  ZMapStyleBumpMode historic_bump_mode;
  ZMapWindow window;
  gboolean column = FALSE ;
  gboolean bumped = TRUE ;
  gboolean mark_set;
  int start, end ;
  double width, bump_spacing = 0.0 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_return_if_fail(bump_mode != ZMAPBUMP_INVALID);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Decide if the column_item is a column group or a feature within that group. */
  if (ZMAP_IS_CANVAS_ITEM(bump_item))
    {
      column = FALSE;
      container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(bump_item);
    }
  else if (ZMAP_IS_CONTAINER_FEATURESET(bump_item))
    {
      column = TRUE;
      container = (ZMapWindowContainerFeatureSet)(bump_item);
    }
  else
    zMapAssertNotReached();

  window = g_object_get_data(G_OBJECT(container), ZMAP_WINDOW_POINTER) ;
  zMapAssert(window) ;

//  historic_bump_mode = zMapWindowContainerFeatureSetGetContainerBumpMode(container) ;
  historic_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container) ;

  // if bumping from one mode to another just clear up with am unbump first, it's tidier this way
  // mh17: ideally i'd prefer to have a separate unbump function, can hack it out later?
  if(historic_bump_mode > ZMAPBUMP_UNBUMP && historic_bump_mode != bump_mode && bump_mode != ZMAPBUMP_UNBUMP)
      zmapWindowColumnBumpRange(bump_item,ZMAPBUMP_UNBUMP,compress_mode);

  if (bump_mode == ZMAPBUMP_INVALID)      // this is set to 'rebump' the columns
    bump_mode = historic_bump_mode ;

  //  RT 171529
  if(bump_mode == ZMAPBUMP_UNBUMP && bump_mode == historic_bump_mode)
      return;

  column_features = (FooCanvasGroup *)zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container) ;

  /* always reset the column */
  zMapWindowContainerFeatureSetRemoveSubFeatures(ZMAP_CONTAINER_FEATURESET(container)) ;

  zmapWindowContainerShowAllHiddenFeatures(container);

  zmapWindowContainerFeatureSetSortFeatures(container, 0);

  zmapWindowBusy(window, TRUE) ;

  /* Need to know if mark is set for limiting feature display for several modes/feature types. */
  mark_set = zmapWindowMarkIsSet(window->mark) ;


  bump_properties.container = container ;
  bump_properties.window = window ;
  bump_properties.overlap_mode = bump_mode ;


  /* If user clicked on the column, not a feature within a column then we need to bump all styles
   * and features within the column, otherwise we just bump the specific features. */
  if (column)
    {
      bump_properties.bump_all      = TRUE ;
      g_object_get(G_OBJECT(container),
		   "unique-id", &bump_properties.style_id,
		   NULL) ;

      bump_properties.display_state = zmapWindowContainerFeatureSetGetDisplay(container);

      zmapWindowContainerFeatureSetJoinAligns(container, &(bump_properties.match_threshold));

      zMapWindowContainerFeatureSetSetBumpMode(container,bump_mode);
      /*g_object_set(G_OBJECT(container),ZMAPSTYLE_PROPERTY_BUMP_MODE,bump_mode,NULL); // complains */
    }
  else
    {

      ZMapFeatureTypeStyle style;
      ZMapFeature feature ;

      feature = zmapWindowItemGetFeature(bump_item);
      zMapAssert(feature) ;

      bump_properties.bump_all = FALSE;
      bump_properties.style_id = feature->style_id;

      style = feature->style;       /* if it's displayed it has a style */

//      zMapStyleGetJoinAligns(style, &(bump_properties.match_threshold));
      bump_properties.match_threshold = zMapStyleGetWithinAlignError(style);

      zMapWindowContainerFeatureSetSetBumpMode(container,bump_mode);
    }

  /* If range set explicitly or a mark is set on the window, then only bump within the range of mark
   * or the visible section of the window. */
  if (compress_mode == ZMAPWINDOW_COMPRESS_INVALID)
    {
      if (mark_set)
	zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;
      else
	{
	  start = window->min_coord ;
	  end   = window->max_coord ;
	}
    }
  else
    {
      if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE)
	{
	  double wx1, wy1, wx2, wy2 ;


	  zmapWindowItemGetVisibleCanvas(window,
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
	  zmapWindowMarkGetSequenceRange(window->mark, &start, &end) ;
	}
      else
	{
	  start = window->min_coord ;
	  end   = window->max_coord ;
	}
    }

  bump_data.start = start ;
  bump_data.end = end ;


  width = zmapWindowContainerFeatureSetGetWidth(container);
  bump_data.spacing = bump_spacing = zmapWindowContainerFeatureGetBumpSpacing(container) ;

  bump_data.incr = width + bump_spacing + 1 ;		    /* adding one because it makes the spacing work... */

  bump_data.bump_prop_data = bump_properties; /* struct copy! */

  /* All bump modes except ZMAPBUMP_COMPLEX share common data/code as they are essentially
   * simple variants. The complex mode requires more processing so uses its own structs/lists. */
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

	complex.temp_buffer     = g_string_sized_new(255); /* Should be enough for most... */

	complex.bump_properties = &(bump_data.bump_prop_data); /* point to the bump_properties */

	/* Make a hash table of feature names which are hashed to lists of features with that name. */
	complex.name_hash       = g_hash_table_new_full(NULL, NULL, /* NULL => use direct hash/comparison. */
							NULL, hashDataDestroyCB) ;

	if (bump_mode == ZMAPBUMP_NAME_BEST_ENDS ||
	    bump_mode == ZMAPBUMP_NAME_COLINEAR  ||
	    bump_mode == ZMAPBUMP_NAME_NO_INTERLEAVE)
	  g_list_foreach(column_features->item_list, makeNameListStrandedCB, &complex) ;
	else
	  {
	    g_list_foreach(column_features->item_list, makeNameListCB, &complex) ;
	  }

	zMapPrintTimer(NULL, "Made names list") ;

	/* Extract the lists of features into a list of lists for sorting. */
	g_hash_table_foreach(complex.name_hash, getListFromHash, &names_list) ;

	zMapPrintTimer(NULL, "Made list of lists") ;

	/* Sort each sub list of features by position for simpler position sorting later. */
	g_list_foreach(names_list, sortListPosition, NULL) ;

	zMapPrintTimer(NULL, "Sorted list of lists by position") ;


	/* For proteins sort the top list using the combined normalised scores of the sublists so higher
	 * scoring matches come first, for dna do the same but per strand. */
	if (complex.protein)
	  {
	    names_list = g_list_sort(names_list, sortByScoreCB) ;
	  }
	else
	  {
	    names_list = g_list_sort(names_list, sortByStrandSpanCB) ;
	  }

	zMapPrintTimer(NULL, "Sorted by score or strand") ;

	list_length = g_list_length(names_list) ;

	/* Remove any lists that do not bump within the range set by the user. */
	if ((compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE || (compress_mode == ZMAPWINDOW_COMPRESS_MARK))
	    || (bump_mode == ZMAPBUMP_NAME_BEST_ENDS || bump_mode == ZMAPBUMP_NAME_COLINEAR))
	  {
	    RangeDataStruct range = {FALSE} ;

	    range.start = start ;
	    range.end   = end ;

	    names_list = g_list_sort_with_data(names_list, sortByOverlapCB, &range) ;
	  }

	list_length = g_list_length(names_list) ;

	/* If "column compress" is turned on then remove any alignments that don't have a match
	 * displayed within the mark or window. */
	if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE || compress_mode == ZMAPWINDOW_COMPRESS_MARK)
	  {
	    if (removeNameListsByRange(&names_list, start, end))
	      g_object_set(G_OBJECT(container),
			   "hidden-bump-features", TRUE,
			   NULL) ;
	  }

	list_length = g_list_length(names_list) ;

	zMapPrintTimer(NULL, "Removed features not in range") ;

	/* Roy put comments and logic into removeNonColinearExtensions() but the problem is not
	 * in that function, I can't reproduce the problem at the moment....I've left his comments
	 * here to remind me. */
	/* There's a problem with logic here. see removeNonColinearExtensions! */
	/* Logic Error!
	 * zmapWindowColumnBumpRange() doesn't always remove the names lists not within the range.
	 * This means there will be names list not in range therefore not found by findRangeListItems()!
	 * calling zmapWindowColumnBump(item, ZMAPBUMP_NAME_COLINEAR) when marked is a sure way to
	 * find this out! RDS.
	 */

	/* Truncate groups of matches where they become non-colinear outside the range if set,
	 * this stops excessively long extensions of groups outside the mark, especially for proteins. */
	if (mark_set && bump_mode == ZMAPBUMP_NAME_COLINEAR
	    && bump_properties.display_state != ZMAPSTYLE_COLDISPLAY_SHOW)
	  g_list_foreach(names_list, removeNonColinearExtensions, &bump_data) ;


	list_length = g_list_length(names_list) ;


	if (!names_list)
	  {
	    bumped = FALSE ;
	  }
	else
	  {
	    AddGapsDataStruct gaps_data = {window, container} ;


	    /* TRY JUST ADDING GAPS  IF A MARK IS SET */
	    if (mark_set && historic_bump_mode != bump_mode)
	      {
		/* What we need to do here is as in the pseudo code.... */
		g_list_foreach(names_list, addGapsCB, &gaps_data) ;

		zMapPrintTimer(NULL, "added gaps") ;
	      }



	    /* Merge the lists into the min. number of non-overlapping lists of features arranged
	     * by name and to some extent by score. */
	    complex.curr_offset = 0.0 ;
	    complex.incr = (width * COMPLEX_BUMP_COMPRESS) ;

	    complex.feature_set = container ;

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
	    if (zmapWindowContainerFeatureSetGetStrand(container) == ZMAPSTRAND_REVERSE)
	      reverseOffsets(complex.bumpcol_list) ;

	    /* Bump all the features to their correct offsets. */
	    g_list_foreach(complex.bumpcol_list, moveItemsCB, NULL) ;

	    zMapPrintTimer(NULL, "bumped features to offsets") ;


          if ((mark_set || zmapWindowContainerFeatureSetGetBumpUnmarked(container))
	      && bump_mode != ZMAPBUMP_NAME_INTERLEAVE)
	      {
		/* NOTE THERE IS AN ISSUE HERE...WE SHOULD ADD COLINEAR STUFF FOR ALIGN FEATURES
		 * THIS IS NOT EXPLICIT IN THE CODE WHICH IS NOT CORRECT....NEED TO THINK THIS
		 * THROUGH, DON'T JUST MAKE THESE COMPLEX BUMP MODES SPECIFIC TO ALIGNMENTS,
		 * ITS MORE COMPLICATED THAN THAT... */

		/* WE SHOULD ONLY BE DOING THIS FOR ALIGN FEATURES...TEST AT THIS LEVEL.... */

            /* NOTE for name_no_interleave we have several groups in a column
             * ref to collection_add_colinear_cb() and called functions
             */
                g_list_foreach(complex.bumpcol_list, collection_add_colinear_cb, &complex);

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


  zmapWindowBusy(window, FALSE) ;

  zMapPrintTimer(NULL, "finished bump") ;

  return ;
}

void zmapWindowColumnBumpAllInitial(FooCanvasItem *column_item)
{
  ZMapWindowContainerGroup container_strand;

  /* Get the strand level container */
  if((container_strand = zmapWindowContainerUtilsItemGetParentLevel(column_item, ZMAPCONTAINER_LEVEL_STRAND)))
    {
      /* container execute */
      zmapWindowContainerUtilsExecute(container_strand,
				      ZMAPCONTAINER_LEVEL_FEATURESET,
				      invoke_bump_to_initial, NULL);
      /* happy days */

    }

  return ;
}

void zmapWindowColumnUnbumpAll(FooCanvasItem *column_item)
{
  ZMapWindowContainerGroup container_strand;

  /* Get the strand level container */
  if((container_strand = zmapWindowContainerUtilsItemGetParentLevel(column_item, ZMAPCONTAINER_LEVEL_STRAND)))
    {
      /* container execute */
      zmapWindowContainerUtilsExecute(container_strand,
                              ZMAPCONTAINER_LEVEL_FEATURESET,
                              invoke_bump_to_unbump, NULL);
      /* happy days */

    }

  return ;
}




/*
 *                       Internal functions.
 */



static ColinearityType colinear_compare_features_cb(ZMapFeature feature_a,
						    ZMapFeature feature_b,
						    gpointer    user_data)
{
  ComplexCol column_data = (ComplexCol)user_data;
  ColinearityType type = COLINEAR_INVALID;

  type = featureHomolIsColinear(column_data->bump_properties->window,
				column_data->bump_properties->match_threshold,
				feature_a, feature_b);

  return type ;
}


static void collection_add_colinear_cb(gpointer data, gpointer user_data)
{
  ComplexCol column_data = (ComplexCol)data;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ComplexBump bump_data  = (ComplexBump)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  ZMapFeatureBlock block;
  ZMapWindowContainerBlock container_block;
  int block_offset = 0;

      /* features are block displayed relative so we have to do the same for gylphs etc
       * really these should be part of the feature not the featureset
       */
            /* get block via strand from container */
  container_block = (ZMapWindowContainerBlock)
                  zmapWindowContainerUtilsGetParentLevel((ZMapWindowContainerGroup) column_data->feature_set, ZMAPCONTAINER_LEVEL_BLOCK);

            /* get the feature corresponding to tbe canvas block */
  zMapAssert(ZMAP_IS_CONTAINER_BLOCK(container_block));
  zmapWindowContainerGetFeatureAny( (ZMapWindowContainerGroup) container_block, (ZMapFeatureAny *) &block);
  if(block)
      block_offset = block->block_to_sequence.block.x1;

      /* NOTE: this does colinear lines, homology markers and nc-splice */
  zMapWindowContainerFeatureSetAddColinearMarkers(column_data->feature_set, column_data->feature_list,
						  colinear_compare_features_cb, column_data, block_offset) ;


  return ;
}


#warning this_needs_to_go
static void invoke_realize_and_map(FooCanvasItem *item)
{
  if(FOO_IS_CANVAS_GROUP(item))
    {
      g_list_foreach(FOO_CANVAS_GROUP(item)->item_list, (GFunc)invoke_realize_and_map, NULL);

      if(FOO_CANVAS_ITEM_GET_CLASS(item)->realize)
	FOO_CANVAS_ITEM_GET_CLASS(item)->realize(item);

      if(FOO_CANVAS_ITEM_GET_CLASS(item)->map)
	(FOO_CANVAS_ITEM_GET_CLASS(item)->map)(item);
    }
  else
    {

      if( FOO_CANVAS_ITEM_GET_CLASS(item)->realize)
	FOO_CANVAS_ITEM_GET_CLASS(item)->realize(item);

      if(FOO_CANVAS_ITEM_GET_CLASS(item)->map)
	(FOO_CANVAS_ITEM_GET_CLASS(item)->map)(item);
    }

  return ;
}

static double bump_overlap(BumpCol bump_data,BumpColRange new_range)
{
  GList *l;
  BumpColRange curr_range;

  // pos list is sorted by column
  for(l = g_list_first(bump_data->pos_list);l;l = g_list_next(l))
    {
      curr_range = (BumpColRange) l->data;

      if(new_range->column == curr_range->column)
        {
           // can overlap with this feature: same column
           if(!(new_range->y1 > curr_range->y2 || new_range->y2 < curr_range->y1))
             {
               new_range->column++;
               new_range->offset += new_range->incr;
               // got an overlap, try next column
             }
        }
      else if(new_range->column < curr_range->column)
           break;
    }
    // either we found a space or ran out of features ie no overlap

    bump_data->pos_list = g_list_insert_before(bump_data->pos_list, l, (gpointer) new_range) ;

    return(new_range->offset);
}

/* Is called once for each item in a column and sets the horizontal position of that
 * item under various "bumping" modes. */
static void bumpColCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  BumpCol bump_data   = (BumpCol)user_data ;
  ZMapWindowContainerFeatureSet container ;
  ZMapFeatureTypeStyle style ;
  ZMapFeature feature ;
  double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0 ;
  gpointer y1_ptr = 0 ;
  gpointer key = NULL, value = NULL ;
  double offset = 0.0, dx = 0.0 ;
  ZMapStyleBumpMode bump_mode ;
  gboolean ignore_mark;
  gboolean proceed = FALSE;

  proceed = zmapWindowItemIsShown(item);

  if(proceed)
    {
      /* Get what we need. */
      feature = zmapWindowItemGetFeature(item);
      if(!feature)
	proceed = FALSE;

      bump_mode = bump_data->bump_prop_data.overlap_mode ;
      container = bump_data->bump_prop_data.container;
      style = feature->style;
    }

  if(proceed)
    {
      if (bump_data->bump_prop_data.display_state == ZMAPSTYLE_COLDISPLAY_SHOW)
	ignore_mark = TRUE ;
      else
	ignore_mark = FALSE ;

      /* try a range restriction... */
      if (bump_mode != ZMAPBUMP_UNBUMP && !(ignore_mark))
	{
	  if (feature->x2 < bump_data->start || feature->x1 > bump_data->end)
	    {
	      g_object_set(G_OBJECT(container),
			   "hidden-bump-features", TRUE,
			   NULL) ;

	      foo_canvas_item_hide(item) ;

	      proceed = FALSE;
	    }
	}

      if (!(bump_data->bump_prop_data.bump_all) &&
	  !(bump_data->bump_prop_data.style_id == feature->style_id))
	proceed = FALSE;
    }

  if(proceed)
    {
      /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
       * calculated them anyway. */
      if(!(item->object.flags & FOO_CANVAS_ITEM_MAPPED))
	{
	  /* Unmapped items return empty area bounds! */
	  invoke_realize_and_map(item);
	}

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

	    new_range         = g_new0(BumpColRangeStruct, 1) ;
	    new_range->y1     = y1 ;
	    new_range->y2     = y2 ;
	    new_range->offset = 0.0 ;
	    new_range->incr   = bump_data->incr ;
          new_range->column = 0;

//        g_list_foreach(bump_data->pos_list, compareListOverlapCB, new_range) ;
          offset = bump_overlap(bump_data,new_range);
	    break ;
	  }
	case ZMAPBUMP_NAME:
	  {
	    /* Bump features that have the same name, i.e. are the same feature, so that each
	     * vertical subcolumn is composed of just one feature in different positions. */
	    ZMapFeature feature ;

	    feature = zmapWindowItemGetFeature(item);

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
	  {
	    offset = bump_data->offset ;
	    bump_data->offset += bump_data->incr ;

	    break ;
	  }
	case ZMAPBUMP_ALL:
	  {
	    offset = bump_data->offset ;
	    bump_data->incr = x2 - x1 + 1 ;
	    bump_data->offset += (bump_data->incr + bump_data->spacing) ;

	    break ;
	  }
	case ZMAPBUMP_NAVIGATOR:
	  {
	    /* Bump features over if they overlap at all. */
	    BumpColRange new_range ;

	    new_range         = g_new0(BumpColRangeStruct, 1) ;
	    new_range->y1     = y1 ;
	    new_range->y2     = y2 ;
	    new_range->offset = 0.0 ;
	    new_range->incr   = x2 - x1 + 1.0;

/*	    g_list_foreach(bump_data->pos_list, compareListOverlapCB, new_range) ;
	    bump_data->pos_list = g_list_append(bump_data->pos_list, new_range) ;
	    offset = new_range->offset ;
 */
	    offset = bump_overlap(bump_data,new_range);
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
	} /* switch(bump_mode) */

    }

  if (proceed)
    {
	if (feature->type != ZMAPSTYLE_MODE_GRAPH && !zMapFeatureSequenceIsPeptide(feature))
	{
	  /* Some features are drawn with different widths to indicate things like score. In this case
	   * their offset needs to be corrected to place them centrally. (We always do this which
	   * seems inefficient but its a toss up whether it would be quicker to test (dx == 0). */
	  dx = (zMapStyleGetWidth(style) - (x2 - x1 - 1)) / 2 ;
	  offset += dx ;
	}

      /* Not having something like this appears to be part of the cause of the oddness. Not all though */
      if (offset < 0.0)
	offset = 0.0;

      /* This does a item_get_bounds... don't we already have them?
       * Might be missing something though. Why doesn't the "Bump" bit calculate offsets? */
      my_foo_canvas_item_goto(item, &(offset), NULL) ;
    }

  return ;
}

/* GDestroyNotify() function for freeing the data part of a hash association. */
static void hashDataDestroyCB(gpointer data)
{
  g_free(data) ;

  return ;
}


#if MH17_NOT_USED
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

static gint overlap_cmp(gconstpointer a,gconstpointer b)
{
      BumpColRange ra = (BumpColRange) a;
      BumpColRange rb = (BumpColRange) b;

      if(ra->column < rb->column)
          return(-1);
      return(1);
}

#endif

/* GFunc callback func, called from g_list_foreach_find() to free list resources. */
static void listDataDestroyCB(gpointer data, gpointer user_data)
{
  g_free(data) ;

  return ;
}



/*
 *                       Functions for the complex bump...
 *
 * If you alter this code you should note the careful use of GList**, list addresses
 * can change so its vital to return the address of the list pointer, not the list
 * pointer itself.
 *
 */
static void make_or_append_in_hash(GHashTable *name_hash, GQuark name_quark, FooCanvasItem *item)
{
  gpointer key = NULL, value = NULL ;
  GList **feature_list_ptr ;
  GList *feature_list ;

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

  return ;
}

static gboolean can_bump_item(FooCanvasItem *item, ComplexBump complex, ZMapFeature *feature_out)
{
  gboolean bump_me = TRUE;

  /* don't bother if something is not displayed. */
  if(zmapWindowItemIsShown(item))
    {
      ZMapWindowContainerFeatureSet container ;
      ZMapFeatureTypeStyle style ;
      ZMapFeature feature ;

      feature = zmapWindowItemGetFeature(item);
      zMapAssert(feature) ;

      container = complex->bump_properties->container;
      style = zMapWindowContainerFeatureSetGetStyle(container);

      if ((!complex->bump_properties->bump_all) && (complex->bump_properties->style_id != feature->style_id))
      	bump_me = FALSE;

      /* Try doing this here.... */
      if((zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT) &&
	 (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL))
	      complex->protein = TRUE ;


      if(bump_me && feature_out)
	*feature_out = feature;
    }
  else
    bump_me = FALSE;

  return bump_me;
}

/* Is called once for each item in a column and constructs a hash table in which each entry
 * is a list of all the features with the same name. */
static void makeNameListCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ComplexBump complex = (ComplexBump)user_data ;
  GHashTable *name_hash = complex->name_hash ;
  ZMapFeature feature ;

  if(!can_bump_item(item, complex, &feature))
    return ;

  make_or_append_in_hash(name_hash, feature->original_id, item);

  return ;
}

static void makeNameListStrandedCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ComplexBump complex = (ComplexBump)user_data ;
  GHashTable *name_hash = complex->name_hash ;
  ZMapFeature feature ;
  GQuark name_quark ;

  if(!can_bump_item(item, complex, &feature))
    return ;

  /* Make a stranded name... */
  g_string_append_printf(complex->temp_buffer, "%s-%c-%c",
			g_quark_to_string(feature->original_id),
			(feature->strand == ZMAPSTRAND_NONE ? '.'
			 : feature->strand == ZMAPSTRAND_FORWARD ? '+'
			 : '-'),
			(feature->feature.homol.strand == ZMAPSTRAND_NONE ? '.'
			 : feature->feature.homol.strand == ZMAPSTRAND_FORWARD ? '+'
			 : '-')) ;

  name_quark = g_quark_from_string(complex->temp_buffer->str) ;

  make_or_append_in_hash(name_hash, name_quark, item);

  g_string_truncate(complex->temp_buffer, 0);

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

  feature = zmapWindowItemGetFeature(item);
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


/* Draw any items that have gaps but were drawn as a simple block as a series
 * of blocks showing the internal gaps of the alignment. */
static void addGapsCB(gpointer data, gpointer user_data)
{
  GList *feature_list = *((GList **)(data)) ;
  AddGapsData gaps_data = (AddGapsData)user_data ;
  ZMapWindow window = gaps_data->window ;
  GList *list_item ;

  list_item = feature_list ;
  do
    {
      FooCanvasItem *item = (FooCanvasItem *)(list_item->data) ;
      ZMapFeature feature = NULL ;
      ZMapWindowContainerFeatureSet container = NULL;
      ZMapFeatureTypeStyle style = NULL;

      /* Get the feature. */
      feature = zmapWindowItemGetFeature(item);
      zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;


      /* Get the features active style (the one on the canvas. */
      container = gaps_data->container;
      style = feature->style;
      /* Only display gaps on bumping and if the alignments have gaps. */
      if (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT)
	{
	  if (feature->feature.homol.align)
	    {
	      gboolean prev_align_gaps ;

	      prev_align_gaps = zMapStyleIsShowGaps(style) ;

	      /* Turn align gaps "on" in the style and then draw.... */
	      zMapStyleSetShowGaps(style, TRUE) ;

	      item = zMapWindowFeatureReplace(window, item, feature, FALSE) ;
							    /* replace feature with itself. */
	      zMapAssert(item) ;

	      list_item->data = item ;			    /* replace item in list ! */
#ifdef NEVER_INCLUDE
	      container->gaps_added_items = g_list_append(container->gaps_added_items, item) ; /* Record in a our list
									    of gapped items. */
#endif
	      /* Now reset align_gaps to whatever it was. */
	      zMapStyleSetShowGaps(style, prev_align_gaps) ;
	    }
	}

    } while ((list_item = g_list_next(list_item))) ;

  return ;
}


/* A list of matches is not necessarily colinear as it may include matches to homologs
 * within a sequence for instance, or just bad matches. This function looks at the
 * colinearity of a list and truncates it once it becomes non-colinear to leave a list
 * that is only of colinear matches. */
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


  /* GOSH...THIS TEST SHOULD HAVE BEEN DONE MUCH HIGHER UP.....REVISIT THIS..... */
  /* We only do aligns, anything else we hide... */
  result = getFeatureFromListItem(list_item, &first_item, &first_feature) ;
  zMapAssert(result) ;

  if (first_feature->type != ZMAPSTYLE_MODE_ALIGNMENT)
    {
      g_list_foreach(name_list, hideItemsCB, NULL) ;

      return ;
    }


  /* Find first and last item(s) that is within the marked range. */
  result = findRangeListItems(list_item, mark_start, mark_end, &first_list_item, &last_list_item) ;
  zMapAssert(result) ;


  list_item = first_list_item ;

  /* Why do we do this again....?????? */
  result = getFeatureFromListItem(list_item, &first_item, &first_feature) ;
  zMapAssert(result) ;

  /* Go from both ends of the list looking for non-colinear matches. */
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


  /* Front of list may have been reset if we found non-coinearity so reset in callers data struct. */
  *name_list_ptr = name_list ;

  return ;
}




/* We only reshow items if they were hidden by our code, any user hidden items are kept hidden.
 * Code is slightly complex here because the user hidden items are a stack of lists of hidden items. */
static void showItems(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)user_data ;
  GList *found_item = NULL ;
  GQueue *user_hidden_items = NULL ;
  ZMapWindowCanvasItem canvas_item = ZMAP_CANVAS_ITEM(item);

  g_object_get(G_OBJECT(container),
	       "user-hidden-items", &user_hidden_items,
	       NULL) ;

  if(!zMapWindowCanvasItemIsMasked(canvas_item,TRUE))  /* don't display masked features if hidden */
  {
      if (!(found_item = g_queue_find_custom(user_hidden_items, item, findItemInQueueCB)))
            foo_canvas_item_show(item) ;
  }

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
      ZMapFeature feature ;

      /* Sanity checks... */
      feature = zmapWindowItemGetFeature(item);
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


/* GCompareFunc() to sort two lists of features by strand and then the overall span of all their features. */
static gint sortByStrandSpanCB(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;					    /* make a == b default. */
  GList **feature_list_out_1 = (GList **)a ;
  GList *feature_list_1 = *feature_list_out_1 ;		    /* Single list of named features. */
  GList **feature_list_out_2 = (GList **)b ;
  GList *feature_list_2 = *feature_list_out_2 ;		    /* Single list of named features. */
  FooCanvasItem *item ;
  ZMapFeature feature ;
  int list_span_1 = 0, list_span_2 = 0 ;
  ZMapStrand strand_1, strand_2 ;

  feature_list_1 = g_list_first(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  list_span_1 = feature->x1 ;
  strand_1 = feature->strand ;

  feature_list_1 = g_list_last(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  list_span_1 = feature->x1 - list_span_1 + 1 ;


  feature_list_2 = g_list_first(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  list_span_2 = feature->x1 ;
  strand_2 = feature->strand ;

  feature_list_2 = g_list_last(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  feature = zmapWindowItemGetFeature(item);
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
  ZMapFeature feature ;
  int top_1, top_2, bot_1, bot_2 ;
  ZMapStrand strand_1, strand_2 ;

  feature_list_1 = g_list_first(feature_list_1) ;
  item = (FooCanvasItem *)feature_list_1->data ;
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  top_1 = feature->x1 ;
  strand_1 = feature->strand ;
  bot_1 = feature->x2 ;

  feature_list_2 = g_list_first(feature_list_2) ;
  item = (FooCanvasItem *)feature_list_2->data ;
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;
  top_2 = feature->x1 ;
  strand_2 = feature->strand ;
  bot_2 = feature->x2 ;

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
  ZMapFeature feature ;
  int diff ;

  feature = zmapWindowItemGetFeature(item);
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

static ComplexCol ComplexBumpComplexColCreate(ComplexBump complex, GList *name_list)
{
  ComplexCol col ;
  OffsetWidthStruct x1_width = {G_MAXDOUBLE, 0.0};

  col = g_new0(ComplexColStruct, 1) ;

  col->bump_properties = complex->bump_properties ;
  col->offset = complex->curr_offset ;
  col->feature_list = name_list ;
  col->feature_set = complex->feature_set ;

  g_list_foreach(name_list, getMaxWidth, &x1_width) ;

  complex->bumpcol_list = g_list_append(complex->bumpcol_list, col) ;
  complex->curr_offset += x1_width.width ;

  if(x1_width.x != G_MAXDOUBLE)
    col->offset -= x1_width.x ;

  return col ;
}

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
      ComplexBumpComplexColCreate(complex, name_list);
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

  ComplexBumpComplexColCreate(complex, name_list);

  return ;
}


/* GFunc() to find max width of canvas items in a list. */
static void getMaxWidth(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  OffsetWidth x1_width = (OffsetWidth)user_data;
  double x1, x2, width ;

  zMapWindowCanvasItemGetBumpBounds(ZMAP_CANVAS_ITEM(item), &x1, NULL, &x2, NULL);

  width = x2 - x1 + 1.0 ;

  if (width > x1_width->width)
    x1_width->width = width ;

  if(x1 < x1_width->x)
    x1_width->x = x1;

  return ;
}






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

      curr_feature = zmapWindowItemGetFeature(curr_item);
      zMapAssert(curr_feature) ;

      new_feature = zmapWindowItemGetFeature(new_item);
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
  curr_first = zmapWindowItemGetFeature(curr_item);
  zMapAssert(curr_first) ;

  curr_ptr = g_list_last(curr_features) ;
  curr_item = (FooCanvasItem *)(curr_ptr->data) ;
  curr_last = zmapWindowItemGetFeature(curr_item);
  zMapAssert(curr_last) ;

  new_ptr = g_list_first(new_features) ;
  new_item = (FooCanvasItem *)(new_ptr->data) ;
  new_first = zmapWindowItemGetFeature(new_item);
  zMapAssert(new_first) ;

  new_ptr = g_list_last(new_features) ;
  new_item = (FooCanvasItem *)(new_ptr->data) ;
  new_last = zmapWindowItemGetFeature(new_item);
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
  double width, spacing;
  ZMapFeatureTypeStyle style ;

  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;

  /* Get hold of the style. */
  style = feature->style;

  /* x1, x2 always needed so might as well get y coords as well because foocanvas will have
   * calculated them anyway. */
  if(!(item->object.flags & FOO_CANVAS_ITEM_MAPPED))
    {
      /* Unmapped items return empty area bounds! */
      invoke_realize_and_map(item);
    }

  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  zMapStyleGet(style,
	       ZMAPSTYLE_PROPERTY_BUMP_SPACING, &spacing,
	       ZMAPSTYLE_PROPERTY_WIDTH, &width,
	       NULL);

  /* Some features are drawn with different widths to indicate things like score. In this case
   * their offset needs to be corrected to place them centrally. (We always do this which
   * seems inefficient but its a toss up whether it would be quicker to test (dx == 0). */
  dx = (((width) * COMPLEX_BUMP_COMPRESS) - (x2 - x1)) / 2 ;

  offset = col_data->offset + dx ;
  if(offset < 0.0)
    offset = 0.0;
  my_foo_canvas_item_goto(item, &offset, NULL) ;


  return ;
}





/*
 *   Functions to draw as "normal" (i.e. not read frame) the frame sensitive columns
 *   from the canvas.
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/*
 *             debug functions, please leave here for future use.
 */


static void printChild(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZMapFeatureAny any_feature ;

  any_feature = zmapWindowItemGetFeatureAny(container);

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
	  feature = zmapWindowItemGetFeature(item);
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

	  colinearity = featureHomolIsColinear(bump_data->bump_prop_data.window,
					       bump_data->bump_prop_data.match_threshold,
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
 * Assumptions:
 *
 * - features are in the same feature set.
 * - features are alignment features and have a match threshold
 * - features are on same strand
 * - features are in correct order, i.e. feat_1 is 5' of feat_2
 *    and they do not overlap in their _reference_ coords.
 *
 * Returns COLINEAR_INVALID if the features overlap or feat_2 is 5' of feat_1,
 * otherwise returns COLINEAR_NOT if homols are not colinear,
 * COLINEAR_IMPERFECT if they are colinear but there is missing sequence in the match,
 * COLINEAR_PERFECT if they are colinear and there is no missing sequence within the
 * threshold given in the style.
 *
 * NOTE this function also flags the start and end of a series of alignments with COLINEAR_INVALID
 */
static ColinearityType featureHomolIsColinear(ZMapWindow window,  unsigned int match_threshold,
					      ZMapFeature feat_1, ZMapFeature feat_2)
{
  ColinearityType colinearity = COLINEAR_INVALID ;
  int diff ;

      /* we allow comparisons withthe start and end of the list */
  if(!feat_1 || !feat_2)
      return(colinearity);

  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feat_1, ZMAPFEATURE_STRUCT_FEATURE)) ;
  zMapAssert(zMapFeatureIsValidFull((ZMapFeatureAny)feat_2, ZMAPFEATURE_STRUCT_FEATURE)) ;

/*
 * as we have multiple featuresets in a column this occurs, esp on 'name no interleave'
*  zMapAssert(feat_1->parent == feat_2->parent) ;
*/
 if(feat_1->parent != feat_2->parent)
      return(colinearity);

/*
 * RT 184372 'reliaable crash on bumping repeats'
 * this appears to be due to trf features bei8ng simple repeats which are basic features not alignments
 * mostly it doesn;t seem to be a problem but for one dataset it asserts here
 * see comment around line 696, not easy to see what to test up there, so handle the situation here.
 */  if(feat_1->type != ZMAPSTYLE_MODE_ALIGNMENT)
      return(colinearity);

  zMapAssert(feat_1->type == ZMAPSTYLE_MODE_ALIGNMENT && feat_1->type == feat_2->type) ;

// mh17: in case of 'Name No Interleave' we have gropups of features in the same column with different names
// in which case this will fail (i've never liked asserts())
//  zMapAssert(feat_1->original_id == feat_2->original_id) ;
//  zMapAssert(feat_1->strand == feat_2->strand) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I'd like to do this BUT this function gets called when we are removing non-colinear features
   * in which case we sometimes fail the below.... */

  zMapAssert(feat_1->feature.homol.strand == feat_2->feature.homol.strand) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if(feat_1->original_id == feat_2->original_id && feat_1->strand == feat_2->strand)      // as in the commented out asserts above
    {
      /* Only markers for features that don't overlap on reference sequence. */
      if (feat_1->x2 < feat_2->x1)
      {
            int prev_end = 0, curr_start = 0 ;
            ZMapStrand reference, match ;
            ZMapFeature top, bottom ;

            reference = feat_1->strand ;
            match = feat_1->feature.homol.strand ;

            /* When match is from reverse strand of homol then homol blocks are in reversed order
            * but coords are still _forwards_. Revcomping reverses order of homol blocks
            * but as before coords are still forwards. */
            if ((reference == ZMAPSTRAND_FORWARD && match == ZMAPSTRAND_FORWARD)
	      || (reference == ZMAPSTRAND_REVERSE && match == ZMAPSTRAND_REVERSE))
	      {
	      top = feat_1 ;
	      bottom = feat_2 ;
	      }
            else
	      {
	      top = feat_2 ;
	      bottom = feat_1 ;
	      }

            prev_end = top->feature.homol.y2 ;
            curr_start = bottom->feature.homol.y1 ;

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
    }
  return colinearity ;
}


// (menu item commented out)
static void invoke_bump_to_initial(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				   ZMapContainerLevelType level, gpointer user_data)
{

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ZMapWindowContainerFeatureSet container_set;
	ZMapStyleBumpMode default_mode, current_mode, initial_mode;

	container_set = (ZMapWindowContainerFeatureSet)container;
	current_mode  = zmapWindowContainerFeatureSetGetBumpMode(container_set);
	default_mode  = zmapWindowContainerFeatureSetGetDefaultBumpMode(container_set);

	initial_mode  = default_mode;

	zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container), initial_mode, ZMAPWINDOW_COMPRESS_ALL);
      }
      break;
    default:
      break;
    }

  return ;
}



static void invoke_bump_to_unbump(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data)
{

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
      zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container), ZMAPBUMP_UNBUMP, ZMAPWINDOW_COMPRESS_ALL);
      }
      break;
    default:
      break;
    }

  return ;
}
