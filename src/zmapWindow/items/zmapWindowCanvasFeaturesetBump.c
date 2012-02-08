/*  File: zmapWindowCanvasFeaturesetBump.c
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun  3 09:51 2009 (rds)
 * Created: Fri Jan 16 11:20:07 2009 (rds)
 *-------------------------------------------------------------------
 */

 /* NOTE
  * this module implements the bumping of featuresets as foo canvas items
  */

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsLog.h>
#include <ZMap/zmapUtilsDebug.h>
#include <ZMap/zmapSkipList.h>
#include <zmapWindowCanvasFeatureset_I.h>
#include <zmapWindowCanvasItemFeatureSet_I.h>

typedef struct  _BumpColRangeStruct *BumpColRange;

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


A more interesting comparsion is with the original foo based code:
# /nfs/users/nfs_m/mh17/.ZMap/ZMap_bins    26/09/2011
1k features: featureset bump in 0.000 seconds
10k features: featureset bump in 0.013 seconds
30k features: featrueset bump on 0.066 seconds
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

typedef struct _BumpColRangeStruct
{
#if !SLOW_BUT_EASY
  GList link;			/* NOTE we don't use GList add/delete_link as that inolves an extra alloc and free for each list item */
  					/* NOTE also that using a GList struct here does not save a whole heap of work */
#endif

  ZMapSpanStruct span; 		/* extent of a feature - may be simple or complex */
  					/* must use this as it's defined in zmapFeature,h */
  double offset ;			/* bump offset for sub column */
  double width;
  int column;			/* which one */

} BumpColRangeStruct;


#if SLOW_BUT_EASY
//typedef  struct Glist *BCR;
#define BCR GList *

#define bump_col_range_alloc() g_new0(BumpColRangeStruct,1)
#define bump_col_range_freet(x) g_free(x)

/* NOTE we expect an assignment left of these #defines; take care woth these macros */
#define bump_col_range_delete(list, link, bcr)	g_list_delete_link(list, link);  g_free(bcr);
#define bump_col_range_insert(list, link, bcr)	g_list_insert_before(list, link, bcr);

/* GList rubbish: cannot insert after the last one, only before a NUll which means starting at the head of the list */
/* last is the last link in the list */
GList *bump_col_range_append(GList *list, GList *last, BumpColRange bcr)
{
	last = g_list_append(last,bcr);
	if(!list)
		list = last;
	return list;
}

#define BCR_NEXT(x) 	(x->next)
#define BCR_DATA(x)	((BumpColRange) x->data)

#else

typedef  BumpColRange BCR;

BumpColRange bump_col_range_free_G = NULL;
static void bump_col_range_free(BumpColRange bcr);

GHashTable *sub_col_width_G = NULL;	 /* for complex overlap */
GHashTable *sub_col_offset_G = NULL;

#define N_BUMP_COL_RANGE_ALLOC	1000	/* we expect to bump 10-200k features, normally 1-20k */

BumpColRange bump_col_range_alloc(void)
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
}



/* item and bcr are both the same BumpColRangeStruct, link may be as well but not always */
#define bump_col_range_delete(list,item,bcr)	(BumpColRange) g_list_remove_link(&list->link,&item->link);  bump_col_range_free(bcr)

/* disappointingly GLib does not provide a function to stitch a GList struct into a list although it provides the converse */
BumpColRange bump_col_range_insert(BumpColRange list, BumpColRange link, BumpColRange bcr)
{
	bcr->link.next = &link->link;
	bcr->link.prev = link->link.prev;

	if(link->link.prev)
		link->link.prev->next = (GList *) bcr;
	link->link.prev = (GList *) bcr;

	if(!bcr->link.prev)
		list = bcr;

	return list;
}

BumpColRange bump_col_range_append(BumpColRange list, BumpColRange last, BumpColRange bcr)
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

	if(!list)
		list = last;

	return list;
}

#define BCR_NEXT(x) 	((BumpColRange) x->link.next)
#define BCR_DATA(x)	(x)

#endif


static BCR bump_overlap(ZMapWindowCanvasFeature feature,BumpFeatureset bump_data, BCR pos_list);

/* FeatureCanvasItems have a normal and bumped x coordinate
 * changing bump-mode causes a recalculate
 * simple and complex features have an extent which is used to decide on positioning
 * NOTE a CanvasFeatureSet is a container that may contain multiple types of feature
 */
gboolean zMapWindowCanvasFeaturesetBump(ZMapWindowCanvasItem item, ZMapStyleBumpMode bump_mode, int compress, BumpFeatureset bump_data)
{
	/* COMPRESS_ALL appears to get set but never used
	 * COMPRESS_MARK removes stuff outside the mark
	 * COMPRESS_VISIBLE  is in the code as 'UnCompress'
	 *
	 * so FTM just implement MARK or visible sequence (ie in scroll region)
	 * features outsuide the range are flagged as non-vis
	 * it's a linear scan so we could just flags all regardless of scroll region
	 * but it might be quicker to process the scroll region only
	 *
	 * erm... compress_mode is implied by start and end coords
	 */
	/* ZMapWindowCompressMode compress_mode = (ZMapWindowCompressMode) compress; */

	ZMapSkipList sl;
	BCR pos_list = NULL;
	BCR l;
	int n;
	double width;
#if MODULE_STATS
	double time;
#endif

      /* transitional code: the column has 0 or more featureset items
       * which are ZMapWindowCanvasItems which are FooCanvasGroups
       * so we need the get the real ZMapWindowFeaturesetItem in the group's item_list
       */

	ZMapWindowFeaturesetItem featureset = NULL;
      ZMapWindowCanvasFeaturesetItem fs_item;
	FooCanvasGroup *group;


      if(!ZMAP_IS_WINDOW_CANVAS_FEATURESET_ITEM(item))
      	return FALSE;

	fs_item = (ZMapWindowCanvasFeaturesetItem) item;
	group = (FooCanvasGroup *) fs_item;

      if(group->item_list)
      	featureset = (ZMapWindowFeaturesetItem) group->item_list->data;

	if(!featureset)
		return FALSE;

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

	bump_data->offset = 0.0;
	bump_data->incr = featureset->width + bump_data->spacing;

	bump_data->complex = TRUE;	/* use complex features if possible */
	if(zMapStyleIsUnique(featureset->style))
		bump_data->complex = 0;

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
		bump_data->complex = FALSE;	/* reset compound feature handling */
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
		/* but for historical accuray we use bump all which display each composite feature in its own column */
		if(bump_mode != ZMAPBUMP_OVERLAP)
			bump_mode = ZMAPBUMP_ALL;
		/* fall through */

	case ZMAPBUMP_OVERLAP:
	case ZMAPBUMP_ALL:
		/* performance stats */
		bump_data->n_col = 0;
		bump_data->comps = 0;
		bump_data->features = 0;
		break;
	default:
		break;
	}

	/* in case we get a bump before a paint eg in initial display */
	if(!featureset->display_index || (featureset->link_sideways && !featureset->linked_sideways))
	  zMapWindowCanvasFeaturesetIndex(featureset);


	/* process all features */

	for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
	{
		ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;	/* base struct of all features */
		double extra;

		if(bump_mode == ZMAPBUMP_UNBUMP)
		{
			/* just redisplays using normal coords */

			if((feature->flags & FEATURE_SUMMARISED))		/* restore to previous state */
				feature->flags |= FEATURE_HIDDEN;

			feature->flags &= ~FEATURE_MARK_HIDE;
			if(!(feature->flags & FEATURE_HIDE_REASON))
			{
				feature->flags &= ~FEATURE_HIDDEN;
			}
			continue;
		}

		if(!zMapWindowCanvasFeaturesetGetFeatureExtent(feature, bump_data->complex, &bump_data->span, &bump_data->width))
			continue;

		if(bump_data->span.x2 < bump_data->start || bump_data->span.x1 > bump_data->end)
		{
			feature->flags |= FEATURE_HIDDEN | FEATURE_MARK_HIDE;
			continue;
		}

		if(( (feature->flags & FEATURE_HIDE_REASON) == FEATURE_SUMMARISED))
		{
			/* bump shows all, that's what it's for */
			/* however we still don't show hidden masked features */
			feature->flags &= ~FEATURE_HIDDEN;
		}

		if(feature->flags & FEATURE_HIDDEN)
			continue;

		if (bump_data->complex && feature->left)
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

		case ZMAPBUMP_OVERLAP:
			pos_list = bump_overlap(feature, bump_data, pos_list);
			break ;

		case ZMAPBUMP_ALTERNATING:
			feature->bump_offset = bump_data->offset;
			bump_data->offset = bump_data->incr - bump_data->offset;
			break;

		default:
			break;
		}

		if(bump_data->complex)
		{
			double offset = feature->bump_offset;
			int col = feature->bump_col;

			for(feature = feature->right ;feature; feature = feature->right)
			{
				feature->bump_offset = offset;
				feature->bump_col = col;

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

	case ZMAPBUMP_OVERLAP:
		/* free allocated memory left over */
		for(n = 0,l = pos_list;l;n++)
		{
			BumpColRange range = BCR_DATA(l);
			l = bump_col_range_delete(l,l,range);
		}

		/*  we need to post process the columns and adjust the width to be that of the widest feature */

			/* get whole column width  and set column offsets */
		for(n = 0, featureset->bump_width = 0; (width = (double) GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G, GUINT_TO_POINTER(n)))) ;n++)
		{
			g_hash_table_insert(sub_col_offset_G, GUINT_TO_POINTER(n), GUINT_TO_POINTER( (int) featureset->bump_width));
			featureset->bump_width += width + bump_data->spacing;
//zMapLogWarning("bump: offset of %d = %f (%f)",featureset->bump_width, width, bump_data->spacing);

		}

		for(sl = zMapSkipListFirst(featureset->display_index); sl; sl = sl->next)
		{
			ZMapWindowCanvasFeature feature = (ZMapWindowCanvasFeature) sl->data;	/* base struct of all features */

			if(!(feature->flags & FEATURE_HIDDEN))
			{
				width = (double) GPOINTER_TO_UINT( g_hash_table_lookup( sub_col_width_G, GUINT_TO_POINTER( feature->bump_col)));
				feature->bump_offset = (double) GPOINTER_TO_UINT( g_hash_table_lookup( sub_col_offset_G, GUINT_TO_POINTER( feature->bump_col)));
//printf("offset feature %s @ %p %f,%f %d = %f\n",g_quark_to_string(feature->feature->unique_id),feature,feature->y1,feature->y2,(int) feature->bump_col, width);
				/* features are displayed relative to the centre of the column when unbumped
				 * so we have to offset the feature as if that is the case
				 */
//zMapLogWarning("bump: feature of %d = %f",feature->bump_col,feature->bump_offset);

				feature->bump_offset -= (featureset->width - width) / 2;

			}
		}


#if MODULE_STATS
		time = zMapElapsedSeconds - time;
		printf("bump overlap %s: %d features %d comparisons in %d columns, residual = %d in %.3f seconds (slow = %d)\n", g_quark_to_string(featureset->id),bump_data->features,bump_data->comps,bump_data->n_col,n,time, SLOW_BUT_EASY);
#endif
		break;

	default:
		break;
	}
//	printf("bump 3: %s %d\n",g_quark_to_string(featureset->id),zMapSkipListCount(featureset->display_index));

	if(featureset->bump_width + featureset->dx > ZMAP_WINDOW_MAX_WINDOW)
	{
		zMapWarning("Cannot bump - too many features to fit into the window.\nTry setting the mark to a smaller region","");

		/* need to hide summarised features again */
		zMapWindowCanvasFeaturesetBump(item, ZMAPBUMP_UNBUMP, compress, bump_data);

		return FALSE;
	}


	return TRUE;

	/* need to request update and reposition: done by caller */
}



/* scatter features so that they don't overlap
 * We know that they are fed in in start coordinate order
 * so we keep a pos_list of features per sub-column
 * which get trimmed as the start coordinate increases
 * that way we know that we are unlikely to reach O(n**2)
 * :-( tests reval O(n**2) / 3
 * obvious worst case is one feature per column
 * it ought to be possible to do this more efficiently
 */
BCR bump_overlap(ZMapWindowCanvasFeature feature, BumpFeatureset bump_data, BCR pos_list)
{
//  GList *l,*cur, *last = NULL;
  BCR l;
  BCR cur;
  BCR last = NULL;
  BumpColRange curr_range,new_range;
  double width;

  new_range = bump_col_range_alloc();
  new_range->span.x1 = bump_data->span.x1;
  new_range->span.x2 = bump_data->span.x2;

#if MODULE_STATS
	bump_data->features++;
#endif

  for(l = pos_list;l;)	/* pos list is sorted by column */
    {
	cur = l;
	/* NOTE l is where new_range can get inserted before, so must be valid, cannot be if it's freed */
	curr_range = BCR_DATA(cur);

	if(curr_range->span.x2 < new_range->span.x1)
	{
		l = BCR_NEXT(l);
		/* lazy evaluation of list trimming, no need to scan fwds if we find a space*/
		pos_list = bump_col_range_delete(pos_list,cur,curr_range);
		continue;
	}

      if(new_range->column < curr_range->column)	/* found a space */
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
  width = (double) GPOINTER_TO_UINT(g_hash_table_lookup(sub_col_width_G, GUINT_TO_POINTER(new_range->column)));
  if(width < bump_data->width)
  	g_hash_table_replace(sub_col_width_G, GUINT_TO_POINTER(new_range->column), GUINT_TO_POINTER((int)  bump_data->width));

//zMapLogWarning("bump: width of %d = %f (%f)",new_range->column,width, bump_data->width);
//  printf("feature %s @ %p %f,%f col %d\n",g_quark_to_string(feature->feature->unique_id),feature,feature->y1,feature->y2,new_range->column);
  return pos_list;
}

