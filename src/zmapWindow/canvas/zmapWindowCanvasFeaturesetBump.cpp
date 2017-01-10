/*  File: zmapWindowCanvasFeaturesetBump.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 * Description: NOTE this module implements the bumping of featuresets
 *              as foo canvas items
 *
 * Exported functions: See zmapWindowCanvasFeatureset.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>

#include <ZMap/zmapUtilsLog.hpp>
#include <ZMap/zmapUtilsDebug.hpp>
#include <ZMap/zmapSkipList.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasFeatureset_I.hpp>
#include <zmapWindowCanvasFeature_I.hpp>


/*
  an experiment w/ memory allocation
  compare g_new0/g_free with allocating 1000 at once an running a free list
  NOTE w/g_new0 and GList we get 2x alloc, 1 for data, 1 for the list

  bump overlap 0x82df270_basic_1000_basic_1000: 999 features 4012 comparisons in 15 columns, residual = 16 in 0.000 seconds (slow = 0)
  bump overlap 0x82df270_basic_10000_basic_10000: 9999 features 336738 comparisons in 89 columns, residual = 83 in 0.011 seconds (slow = 0)
  bump overlap 0x82df270_basic_100000_basic_100000: 99985 features 31285276 comparisons in 729 columns, residual = 702 in 0.740 seconds (slow = 0)

  bump overlap 0x82df270_basic_1000_basic_1000: 999 features 4012 comparisons in 15 columns, residual = 16 in 0.001 seconds (slow = 1)
  bump overlap 0x82df270_basic_10000_basic_10000: 9999 features 336738 comparisons in 89 columns, residual = 83 in 0.014 seconds (slow = 1)
  bump overlap 0x82df270_basic_100000_basic_100000: 99985 features 31285276 comparisons in 729 columns, residual = 702 in 0.977 seconds (slow = 1)


  # features  g_new0/g_free custom    diff   # columns list-size
  1k          1 ms          0ms       1ms    15        16
  10k         14ms          11ms      3ms    89        83
  100k        977ms         740ms     237ms  729       702

  So: this can make bumping work 30% or 25% faster depending on how you do the sums.
  Not earth shattering but significant.  memory alloc is not linear, but not as bad as my lovely de-overlap algorithm.
  A more salient point is the the GList implementation is a bit esoteric and contains covert performance pitfalls such as insert_before(NULL).

  Implementation by #define mostly, a bit messy but the calling code is identical.


  A more interesting comparison is with the original foo based code:
  # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins    26/09/2011
  1k features: featureset bump in 0.000 seconds
  10k features: featureset bump in 0.013 seconds
  30k features: featureset bump on 0.066 seconds
  100k features: featureset bump in 0.740 seconds

  deskpro17848[mh17]56: zmap --conf_file=ZMap_bins
  # /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins    26/09/2011
  1k features: traditional bump in 0.013 seconds	(>13x)
  10k features: traditional bump in 0.838 seconds	(64x)
  30k features: traditional bump on 8.936 seconds	(135x)
  100k features: tradtional bump in 329.828 seconds (445x)
*/

#define MODULE_STATS	0	/* NOTE this is for BUMP_OVERLAP, colinear is BUMP_ALL */

#define SLOW_BUT_EASY	0	/* 30% quicker if not */



typedef struct BumpColRangeStructName
{
#if !SLOW_BUT_EASY

  /* NOTE we don't use GList add/delete_link as that inolves an extra alloc and free for each list item */
  GList link;

  /* NOTE also that using a GList struct here does not save a whole heap of work */
#endif


  /* extent of a feature - may be simple or complex */
  ZMapSpanStruct span ;

  /* feature set id...we can also bump by feature set....assume no overlap.... */
  GQuark featureset_unique_id ;

  /* must use this as it's defined in zmapFeature,h */
  double offset ;			/* bump offset for sub column */
  double width;
  int column;			/* which one */

} BumpColRangeStruct, *BumpColRange ;


#if SLOW_BUT_EASY
//typedef  struct Glist *BCR;
#define BCR GList *

#define bump_col_range_alloc() g_new0(BumpColRangeStruct,1)
#define bump_col_range_freet(x) g_free(x)

/* NOTE we expect an assignment left of these #defines; take care woth these macros */
#define bump_col_range_delete(glist, link, bcr)	g_list_delete_link(glist, link);  g_free(bcr);
#define bump_col_range_insert(glist, link, bcr)	g_list_insert_before(glist, link, bcr);

#define BCR_NEXT(x) 	(x->next)
#define BCR_DATA(x)	((BumpColRange) x->data)


#else /* !SLOW_BUT_EASY */


#define N_BUMP_COL_RANGE_ALLOC	1000	/* we expect to bump 10-200k features, normally 1-20k */

#define BCR_NEXT(x) 	((BumpColRange) x->link.next)
#define BCR_DATA(x)	(x)

/* item and bcr are both the same BumpColRangeStruct, link may be as well but not always */
#define bump_col_range_delete(glist,item,bcr) \
  (BumpColRange) g_list_remove_link(&glist->link,&item->link);  bump_col_range_free(bcr)


typedef BumpColRange BCR;


static BumpColRange bump_col_range_alloc(void) ;
static BumpColRange bump_col_range_insert(BumpColRange alist, BumpColRange link, BumpColRange bcr) ;
static BumpColRange bump_col_range_append(BumpColRange alist, BumpColRange last, BumpColRange bcr) ;
static void bump_col_range_free(BumpColRange bcr) ;

#endif



static BCR calcBumpNoOverlap(ZMapWindowCanvasFeature feature,BumpFeatureset bump_data, BCR pos_list) ;
static BCR calcBumpNoOverlapFeatureSet(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                       BumpFeatureset bump_data, BCR pos_list) ;

static gboolean featureTestMark(ZMapWindowCanvasFeature feature, double start, double end) ;

static void freeSubcolListCB(gpointer data, gpointer user_data) ;


/*
 *                  Globals
 */

#if !SLOW_BUT_EASY

BumpColRange bump_col_range_free_G = NULL;

GHashTable *sub_col_width_G = NULL;	 /* for complex overlap */

GHashTable *sub_col_offset_G = NULL;

GHashTable *sub_col_name_G = NULL ;

#endif



/*
 *                  External routines
 */


/* FeatureCanvasItems have a normal and bumped x coordinate
 * changing bump-mode causes a recalculate
 * simple and complex features have an extent which is used to decide on positioning
 * NOTE a CanvasFeatureSet is a container that may contain multiple types of feature
 */
/* COMPRESS_ALL appears to get set but never used
 * COMPRESS_MARK removes stuff outside the mark
 * COMPRESS_VISIBLE  is in the code as 'UnCompress'
 *
 * so FTM just implement MARK or visible sequence (ie in scroll region)
 * features outside the range are flagged as non-vis
 * it's a linear scan so we could just flag all regardless of scroll region
 * but it might be quicker to process the scroll region only
 *
 * erm... compress_mode is implied by start and end coords
 */
/* ZMapWindowCompressMode compress_mode = (ZMapWindowCompressMode) compress; */
gboolean zMapWindowCanvasFeaturesetBump(ZMapWindowFeaturesetItem featureset, ZMapStyleBumpMode bump_mode,
					int compress, BumpFeatureset bump_data)
{
  gboolean result = FALSE ;
  ZMapSkipList sl ;
  BCR pos_list = NULL ;
  BCR l ;
  int n ;
#if MODULE_STATS
  double time ;
#endif

  zMapReturnValIfFail((ZMAP_IS_WINDOW_FEATURESET_ITEM(featureset)), FALSE) ;

  /* do not bump if set is decoration and not actually features, eg is a background */
  zMapReturnValIfFailSafe(!(featureset->layer & ZMAP_CANVAS_LAYER_DECORATION), TRUE) ;


  //printf("\nbump %s to %d\n",g_quark_to_string(featureset->id), bump_mode);

#if MODULE_STATS
  time = zMapElapsedSeconds;
#endif

  /* prepare */

  if(!sub_col_width_G)
    sub_col_width_G = g_hash_table_new(NULL,NULL);
  else
    g_hash_table_remove_all(sub_col_width_G);

  if(!sub_col_offset_G)
    sub_col_offset_G = g_hash_table_new(NULL,NULL);
  else
    g_hash_table_remove_all(sub_col_offset_G);

  if(!sub_col_name_G)
    sub_col_name_G = g_hash_table_new(NULL,NULL);
  else
    g_hash_table_remove_all(sub_col_name_G);


  /* If there's an old subcol list then get rid of it. */
  if (featureset->sub_col_list)
    {
      /* N.B. in a later version of glib you can do the delete with a single call. */
      g_list_foreach(featureset->sub_col_list, freeSubcolListCB, NULL) ;

      g_list_free(featureset->sub_col_list) ;

      featureset->sub_col_list= NULL ;
    }




  bump_data->offset = 0.0;
  bump_data->incr = featureset->width + bump_data->spacing;

  /* use complex features if possible */
  if(zMapStyleIsUnique(featureset->style))
    bump_data->is_complex = FALSE ;
  else
    bump_data->is_complex = TRUE ;

  /* OK, THIS IS CHANGING, OTHER COLUMNS NOW NEED TO BE BUMPED IN A "COMPLEX" WAY... */
  /*
   * a complex feature is a transcript or joined up alignment
   * some alignments (eg repeats) should not be - this need to go into styles or config somewhere
   * alignments should be joind up in the feature context if they are a set
   * if not then we'll have complex features of 1 sub-feature
   * if we want bump modes with unjoined up alignemnts then the mode can clear this flag (as above)
   */

  if(bump_mode != ZMAPBUMP_UNBUMP)
    featureset->bump_overlap = 0;

  switch(bump_mode)
    {
    case ZMAPBUMP_UNBUMP:
      /* this is a bit hacky */
      bump_data->is_complex = FALSE;	/* reset compound feature handling */
      break;

    case ZMAPBUMP_NAME_INTERLEAVE:
    case ZMAPBUMP_NAME_NO_INTERLEAVE:
      /* patch both these to overlap, should just be no_interleave but I got it wrong last time round */
      /* temporasry situation till i fix the menu to nbe simple */
      bump_mode = ZMAPBUMP_OVERLAP;
      /* fall through */

    case ZMAPBUMP_START_POSITION:
    case ZMAPBUMP_NAME_COLINEAR:
    case ZMAPBUMP_NAME_BEST_ENDS:
      /* for alignments these all map to overlap, the alignments code shows the decorations regardless */
      /* but for historical accuray we use bump all which displays each composite feature in its own column */
      if(bump_mode != ZMAPBUMP_OVERLAP)
	bump_mode = ZMAPBUMP_ALL;
      /* fall through */

    case ZMAPBUMP_FEATURESET_NAME:
    case ZMAPBUMP_OVERLAP:
    case ZMAPBUMP_ALL:
      /* performance stats */
      bump_data->n_col = 0;
      bump_data->comps = 0;
      bump_data->features = 0;
      break;

      /* Not sure if this correct.....no action if alternating ? */
    case ZMAPBUMP_ALTERNATING:
      break ;

    default:
      zMapWarnIfReached() ;
      break;
    }

  /* in case we get a bump before a paint eg in initial display */
  if(!featureset->display_index || (featureset->link_sideways && !featureset->linked_sideways))
    zMapWindowCanvasFeaturesetIndex(featureset);


  /* process all features */
  for (sl = zMapSkipListFirst(featureset->display_index) ; sl ; sl = sl->next)
    {
      ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature)(sl->data) ;	/* base struct of all features */
      double extra ;

      /* Um....not sure how/why this can happen....is it for decorations or what ???? */
      if (!zmapWindowCanvasFeatureValid(feature))
        continue;

      //printf("bump feature %s %lx\n", g_quark_to_string(feature->feature->original_id),feature->flags);
      if(bump_mode == ZMAPBUMP_UNBUMP)
	{
	  /* just redisplays using normal coords */
	  /* in case of mangled alingments must reset the first exom
	   * ref to zMapWindowCanvasAlignmentGetFeatureExtent(),
	   * which extends the feature to catch colinear lines off screen
	   * NOTE we'd do better with some flag set by the alignment code to trigger this.
	   */
	  if(feature->type == FEATURE_ALIGN)
	    feature->y2 = feature->feature->x2;

	  if((feature->flags & FEATURE_SUMMARISED))		/* restore to previous state */
	    feature->flags |= FEATURE_HIDDEN;

	  feature->flags &= ~FEATURE_MARK_HIDE;
	  if(!(feature->flags & FEATURE_HIDE_REASON))
	    {
	      feature->flags &= ~FEATURE_HIDDEN;
	    }

	  continue;
	}

      if (!zMapWindowCanvasFeatureGetFeatureExtent(feature,
                                                   bump_data->is_complex, &bump_data->span, &bump_data->width))
	continue;

      if (bump_data->mark_set)
	{
	  if(bump_data->span.x2 < bump_data->start || bump_data->span.x1 > bump_data->end)
	    {
	      /* all the feature's exons are outside the mark on the same side */
	      feature->flags |= FEATURE_HIDDEN | FEATURE_MARK_HIDE;
	      continue;
	    }

	  if(featureTestMark(feature, bump_data->start, bump_data->end))
	    {
	      /* none of the feature's exons cross the mark */
	      feature->flags |= FEATURE_HIDDEN | FEATURE_MARK_HIDE;
	      continue;
	    }
	}

      if(( (feature->flags & FEATURE_HIDE_REASON) == FEATURE_SUMMARISED))
	{
	  /* bump shows all, that's what it's for */
	  /* however we still don't show hidden masked features */
	  feature->flags &= ~FEATURE_HIDDEN;
	}

      if(feature->flags & FEATURE_HIDDEN)
	continue;

      if (bump_data->is_complex && feature->left)
	{
	  /* already processed the series of features  */
	  continue;
	}

      extra = bump_data->span.x2 - bump_data->span.x1;
      if(extra > featureset->bump_overlap)
	featureset->bump_overlap = extra;

      switch(bump_mode)
	{
	case ZMAPBUMP_ALL:
	  /* NOTE this is the normal colinear bump, one set of features per column */
	  feature->bump_offset = bump_data->offset;
	  bump_data->offset += bump_data->width + bump_data->spacing;

	  break ;

          /* needs changing for new bumpstyle.... */
        case ZMAPBUMP_FEATURESET_NAME:
	case ZMAPBUMP_OVERLAP:
	  feature->bump_offset = bump_data->offset ;
	  bump_data->offset += bump_data->width + bump_data->spacing ;

          if (bump_mode == ZMAPBUMP_FEATURESET_NAME)
            pos_list = calcBumpNoOverlapFeatureSet(featureset, feature, bump_data, pos_list) ;
          else
            pos_list = calcBumpNoOverlap(feature, bump_data, pos_list) ;
	  break ;

	case ZMAPBUMP_ALTERNATING:
	  feature->bump_offset = bump_data->offset;
	  bump_data->offset = bump_data->incr - bump_data->offset;
	  break;

	default:
	  break;
	}


      if(bump_data->is_complex)
	{
	  double offset = feature->bump_offset;
	  int col = feature->bump_col;

	  for (feature = feature->right ; feature ; feature = feature->right)
	    {
	      feature->bump_offset = offset ;
	      feature->bump_col = col ;

	    }
	}
    }

  /* tidy up */

  featureset->bumped = TRUE;
  featureset->bump_width = bump_data->offset;
  featureset->bump_mode = bump_mode;

  switch(bump_mode)
    {
    case ZMAPBUMP_UNBUMP:
      featureset->bumped = FALSE;
      /* just redisplays using normal coords */
      break;

    case ZMAPBUMP_ALTERNATING:
      featureset->bump_width = bump_data->incr * 2 - bump_data->spacing;
      break;

    case ZMAPBUMP_ALL:
      featureset->bump_width = bump_data->offset;
      break;

    case ZMAPBUMP_FEATURESET_NAME:
    case ZMAPBUMP_OVERLAP:
      {
        double width = 0 ;

        /* free allocated memory left over */
        for (n = 0, l = pos_list ; l ; n++)
          {
            BumpColRange range = BCR_DATA(l) ;

            l = bump_col_range_delete(l, l, range) ;
          }

        /* get whole column width and set column offsets note that spacing is added to the right so
         * we remove the extra/redundant spacing for the last offset after the loop. */
        for (n = 0, featureset->bump_width = 0 ;
             (width = (double)GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G, GUINT_TO_POINTER(n)))) ;
             n++)
          {
            ZMapWindowCanvasSubCol sub_col_data ;


            g_hash_table_insert(sub_col_offset_G,
                                GUINT_TO_POINTER(n), GUINT_TO_POINTER((int)featureset->bump_width)) ;

            /* For featureset_name bumping insert subcol information into subcol list. */
            if (bump_mode == ZMAPBUMP_FEATURESET_NAME)
              {
                sub_col_data = g_new0(ZMapWindowCanvasSubColStruct, 1) ;

                sub_col_data->subcol_id = GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_name_G, GUINT_TO_POINTER(n))) ;
                sub_col_data->offset = featureset->bump_width ;
                sub_col_data->width = width + bump_data->spacing ;

                featureset->sub_col_list = g_list_append(featureset->sub_col_list, sub_col_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                {
                  char *subcol_name ;
                  double offset, width ;

                  subcol_name = g_quark_to_string(sub_col_data->subcol_id) ;
                  offset = sub_col_data->offset ;
                  width = sub_col_data->width ;

                  zMapDebugPrintf("Subcol \"%s\" is at %g with width %g", subcol_name, offset, width) ;
                }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

              }

            featureset->bump_width += width + bump_data->spacing ;
            //printf("bump: offset of %f = %f (%f)\n",featureset->bump_width, width,
            //bump_data->spacing);
          }
        if (featureset->bump_width)
          featureset->bump_width -= bump_data->spacing ;


        /* We need to post process the columns and adjust the width to be that of the widest
         * feature. */
        for (sl = zMapSkipListFirst(featureset->display_index) ; sl ; sl = sl->next)
          {
            ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature)(sl->data) ; /* base struct of all features */

            if (zmapWindowCanvasFeatureValid(feature) && !(feature->flags & FEATURE_HIDDEN))
              {
                width = (double)GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G,
                                                                     GUINT_TO_POINTER(feature->bump_col)));

                feature->bump_offset
                  = (double)GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_offset_G,
                                                                 GUINT_TO_POINTER(feature->bump_col))) ;

                /* printf("offset feature %s @ %p %f,%f %d = %f\n",
                   g_quark_to_string(feature->feature->unique_id), feature,
                   feature->y1,feature->y2,(int) feature->bump_col, width); */


                /* We should only do this bit for non-graph features..... */
                /*
                 * features are displayed relative to the centre of the column when unbumped
                 * so we have to offset the feature as if that is still the case
                 */
                //printf("bump: feature off %d = %f\n", feature->bump_col,feature->bump_offset);

                if (zMapStyleGetMode(featureset->style) != ZMAPSTYLE_MODE_GRAPH)
                  {
                    feature->bump_offset -= (featureset->width - width) / 2 ;
                  }
              }
          }




#if MODULE_STATS
        time = zMapElapsedSeconds - time;
        printf("bump overlap %s: %d features %d comparisons in %d columns, residual = %d in %.3f seconds (slow = %d)\n", g_quark_to_string(featureset->id),bump_data->features,bump_data->comps,bump_data->n_col,n,time, SLOW_BUT_EASY);
#endif
        break;
      }

    default:
      break;
    }
  //	printf("bump 3: %s %d\n",g_quark_to_string(featureset->id),zMapSkipListCount(featureset->display_index));


  if (featureset->bump_width + featureset->dx > ZMAP_WINDOW_MAX_WINDOW)
    {
      zMapWarning("Cannot bump featureset \"%s\" - too many features to fit into the window.\n"
                  "Try setting the mark to a smaller region !", g_quark_to_string(featureset->id)) ;

      result = FALSE ;
    }
  else
    {
      /* need to request update and reposition: done by caller */

      result = TRUE ;
    }


  return result ;
}





/*
 *                    Internal routines.
 */


/* are any exons in the mark ? */
static gboolean featureTestMark(ZMapWindowCanvasFeature feature, double start, double end)
{
  ZMapWindowCanvasFeature f;

  for(f = feature; f->left; f = f->left)
    continue;

  for(; f; f = f->right)
    {
      if(f->feature->x2 < start)	/* zMapWindowCanvasFeaturesetGetFeatureExtent mangles f->y2 */
        continue;
      if(f->y1 > end)
        continue;
      return FALSE;
    }
  return TRUE;
}



/* ONLY EVER USED FOR OVERLAP MODE....
 *
 * scatter features so that they don't overlap.
 *
 * We know that they are fed in in start coordinate order so we keep
 * a pos_list of features per sub-column which get trimmed as the start
 * coordinate increases that way we know that we are unlikely to reach O(n**2).
 *
 * :-( tests reveal O(n**2) / 3
 *
 * obvious worst case is one feature per column it ought to be possible to do this more efficiently
 */
static BCR calcBumpNoOverlap(ZMapWindowCanvasFeature feature, BumpFeatureset bump_data, BCR pos_list)
{
  //  GList *l,*cur, *last = NULL;
  BCR l;
  BCR cur;
  BCR last = NULL;
  BumpColRange curr_range, new_range ;
  double width ;

  new_range = bump_col_range_alloc();
  new_range->span.x1 = bump_data->span.x1;
  new_range->span.x2 = bump_data->span.x2;

#if MODULE_STATS
  bump_data->features++;
#endif

  for (l = pos_list ; l ; )                                 /* pos list is sorted by column */
    {
      /* NOTE l is where new_range can get inserted before, so must be valid, cannot be if it's freed */
      cur = l;

      curr_range = BCR_DATA(cur);

      if(curr_range->span.x2 < new_range->span.x1)
	{
          l = BCR_NEXT(l);

          /* lazy evaluation of list trimming, no need to scan fwds if we find a space*/
          pos_list = bump_col_range_delete(pos_list,cur,curr_range);

          continue;
	}

      if (new_range->column < curr_range->column)	/* found a space */
        break;

      if(new_range->column == curr_range->column)
        {
#if MODULE_STATS
          bump_data->comps++;
#endif

          // can overlap with this feature: same column
          if( ! (new_range->span.x1 > curr_range->span.x2 || new_range->span.x2 < curr_range->span.x1))
            {
              new_range->column++;
              //            	new_range->offset = curr_range->offset + curr_range->width;
              // got an overlap, try next column

#if MODULE_STATS
              if(new_range->column > bump_data->n_col)
                bump_data->n_col = new_range->column;
#endif

            }
        }

      last = l;		/* remember last to insert at the end */
      l = BCR_NEXT(l);
    }
  // either we found a space or ran out of features ie no overlap

  if(l)
    {
      /* NOTE: {} needed due to macro call */
      pos_list = bump_col_range_insert(pos_list, l, new_range);
    }
  else
    {
      pos_list = bump_col_range_append(pos_list, last, new_range);
    }

  /* store the column for later calculation of the offset */
  feature->bump_col = (double) new_range->column;

  /* get the max width of a feature in each column */
  /* totally yuk casting here but bear with me */
  width = (double)GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G, GUINT_TO_POINTER(new_range->column))) ;

  if(width < bump_data->width)
    g_hash_table_replace(sub_col_width_G, GUINT_TO_POINTER(new_range->column),
                         GUINT_TO_POINTER((int)bump_data->width)) ;

  //zMapLogWarning("bump: width of %d = %f (%f)",new_range->column,width, bump_data->width);
  //  printf("feature %s @ %p %f,%f col %d\n",g_quark_to_string(feature->feature->unique_id),feature,feature->y1,feature->y2,new_range->column);

  return pos_list;
}



static BCR calcBumpNoOverlapFeatureSet(ZMapWindowFeaturesetItem featureset, ZMapWindowCanvasFeature feature,
                                       BumpFeatureset bump_data, BCR pos_list)
{
  //  GList *l,*cur, *last = NULL;
  BCR l;
  BCR cur;
  BCR last = NULL;
  BumpColRange curr_range, new_range ;
  double width ;


  new_range = bump_col_range_alloc();
  new_range->featureset_unique_id = feature->feature->parent->unique_id ;


#if MODULE_STATS
  bump_data->features++;
#endif

  for (l = pos_list ; l ; )                                 /* pos list is sorted by column */
    {
      /* NOTE l is where new_range can get inserted before, so must be valid, cannot be if it's freed */
      cur = l;

      curr_range = BCR_DATA(cur);

      if (curr_range->featureset_unique_id != new_range->featureset_unique_id)
	{
          last = l;		/* remember last to insert at the end */
          l = BCR_NEXT(l);

          continue;
	}
      else
        {
          break ;
        }

      last = l ;		/* remember last to insert at the end */
      l = BCR_NEXT(l) ;
    }
  // either we found a space or ran out of features ie no overlap



  /* Post process.... */
  if(l)
    {
      /* need to free allocated new_range.... */
      bump_col_range_free(new_range) ;

      new_range = curr_range ;
    }
  else
    {
      if (last)
        new_range->column = last->column + 1 ;

      pos_list = bump_col_range_append(pos_list, last, new_range) ;



      /* OK HERE WE SHOULD INSERT THE SUBCOL NAME INTO THE SUBCOL HASH/LIST..... */
      g_hash_table_insert(sub_col_name_G,
                          GUINT_TO_POINTER(new_range->column),
                          GUINT_TO_POINTER(feature->feature->parent->original_id)) ;
    }


  /* store the column for later calculation of the offset */
  feature->bump_col = (double) new_range->column ;

  /* get the max width of a feature in each column */
  /* totally yuk casting here but bear with me */
  width = (double)GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G, GUINT_TO_POINTER(new_range->column))) ;

  if (width < bump_data->width)
    g_hash_table_replace(sub_col_width_G, GUINT_TO_POINTER(new_range->column),
                         GUINT_TO_POINTER((int)bump_data->width)) ;

  //zMapLogWarning("bump: width of %d = %f (%f)",new_range->column,width, bump_data->width);
  //  printf("feature %s @ %p %f,%f col %d\n",g_quark_to_string(feature->feature->unique_id),feature,feature->y1,feature->y2,new_range->column);

  return pos_list;
}





#if SLOW_BUT_EASY

/* GList rubbish: cannot insert after the last one, only before a NUll which means starting at the
 * head of the list
 * last is the last link in the list */
static GList *bump_col_range_append(GList *glist, GList *last, BumpColRange bcr)
{
  last = g_list_append(last,bcr);
  if(!glist)
    glist = last;
  return glist;
}


#else /* SLOW_BUT_EASY */


static BumpColRange bump_col_range_alloc(void)
{
  BumpColRange bcr;
  int i;

  if(!bump_col_range_free_G)
    {

      bcr = g_new(BumpColRangeStruct,N_BUMP_COL_RANGE_ALLOC);

      for(i = 0;i < N_BUMP_COL_RANGE_ALLOC;i++)
        bump_col_range_free(bcr++);
    }

  bcr = bump_col_range_free_G;
  if(bcr)
    {
      bump_col_range_free_G = (BumpColRange) bcr->link.next;

      /* these can get re-allocated so must zero */
      memset((gpointer) bcr,0,sizeof(BumpColRangeStruct));
    }


  return(bcr);
}


/* NOTE for the free list we don't use the prev pointer so ne need to set it */
static void bump_col_range_free(BumpColRange bcr)
{
  bcr->link.next = (GList *) bump_col_range_free_G;
  bump_col_range_free_G = bcr;

  return ;
}



/* disappointingly GLib does not provide a function to stitch a GList struct into a list although
 * it provides the converse */
static BumpColRange bump_col_range_insert(BumpColRange alist, BumpColRange link, BumpColRange bcr)
{
  bcr->link.next = &link->link;
  bcr->link.prev = link->link.prev;

  if(link->link.prev)
    link->link.prev->next = (GList *) bcr;
  link->link.prev = (GList *) bcr;

  if(!bcr->link.prev)
    alist = bcr;

  return alist;
}

static BumpColRange bump_col_range_append(BumpColRange alist, BumpColRange last, BumpColRange bcr)
{
  if(!last)
    {
      last = bcr;
    }
  else
    {
      last->link.next = (GList *) bcr;
      bcr->link.prev = &last->link;
    }

  if(!alist)
    alist = last;

  return alist;
}

#endif



/* A GFunc() routine to free the subcol list, currently very simple. */
static void freeSubcolListCB(gpointer data, gpointer user_data_unused)
{
  g_free(data) ;

  return ;
}
