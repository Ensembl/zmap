/*  File: zmapWindowItem.c
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to manipulate canvas items.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapSequence.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>


/* Why are these here....try to get rid of them.... */
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerFeatureSet_I.h>





static void getVisibleCanvas(ZMapWindow window,
			     double *screenx1_out, double *screeny1_out,
			     double *screenx2_out, double *screeny2_out) ;






/*
 *                        External functions.
 */



gboolean zmapWindowItemGetStrandFrame(FooCanvasItem *item, ZMapStrand *set_strand, ZMapFrame *set_frame)
{
  ZMapWindowContainerFeatureSet container;
  ZMapFeature feature ;
  gboolean result = FALSE ;

  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature && feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  *set_strand = container->strand ;
  *set_frame  = container->frame ;

  result = TRUE ;

  return result ;
}


/* function is called from 2 places only, may be good to make callers use the id2c list directly
   this is a quick mod to preserve the existing interface */

GList *zmapWindowItemListToFeatureList(GList *item_list)
{
  GList *feature_list = NULL;
  ID2Canvas id2c;

  for(;item_list;item_list = item_list->next)
  {
  	id2c = (ID2Canvas) item_list->data;
  	feature_list = g_list_append(feature_list,(gpointer) id2c->feature_any);
  }

  return feature_list;
}


/* as above but replaces composite features with the underlying */
GList *zmapWindowItemListToFeatureListExpanded(GList *item_list, int expand)
{
  GList *feature_list = NULL;
  ID2Canvas id2c;
  ZMapFeature feature;

  for(;item_list;item_list = item_list->next)
  {
  	id2c = (ID2Canvas) item_list->data;
	feature = (ZMapFeature) id2c->feature_any;

	if(expand && feature->children)
	{
		GList * l;

		for(l = feature->children; l; l = l->next)
		{
			feature_list = g_list_prepend(feature_list,(gpointer) l->data);
		}
	}
	else
	{
		feature_list = g_list_prepend(feature_list,(gpointer) id2c->feature_any);
	}
  }

  return feature_list;
}






/*
 *                     Feature Item highlighting....
 */

/* Highlight the given feature. */
void zMapWindowHighlightFeature(ZMapWindow window, ZMapFeature feature, gboolean highlight_same_names, gboolean replace)
{
  FooCanvasItem *feature_item ;
  ZMapStrand set_strand = ZMAPSTRAND_NONE;
  ZMapFrame set_frame = ZMAPFRAME_NONE;

  if(zMapStyleIsStrandSpecific(*feature->style))
	set_strand = feature->strand;
  if(zMapStyleIsFrameSpecific(*feature->style) && IS_3FRAME_COLS(window->display_3_frame))
	set_frame = zmapWindowFeatureFrame(feature);

  if ((feature_item = zmapWindowFToIFindFeatureItem(window, window->context_to_item,
						    set_strand, set_frame, feature)))
    zmapWindowHighlightObject(window, feature_item, replace, highlight_same_names, FALSE) ;

  return ;
}



/* Unhighlight the given feature. */
gboolean zMapWindowUnhighlightFeature(ZMapWindow window, ZMapFeature feature)
{
  gboolean result = FALSE ;
  FooCanvasItem *feature_item ;
  ZMapStrand set_strand = ZMAPSTRAND_NONE ;
  ZMapFrame set_frame = ZMAPFRAME_NONE ;

  if (zMapStyleIsStrandSpecific(*feature->style))
    set_strand = feature->strand;
  if (zMapStyleIsFrameSpecific(*feature->style) && IS_3FRAME_COLS(window->display_3_frame))
    set_frame = zmapWindowFeatureFrame(feature);

  if ((feature_item = zmapWindowFToIFindFeatureItem(window, window->context_to_item,
						    set_strand, set_frame, feature))
      && feature_item == zmapWindowFocusGetHotItem(window->focus))
    {
      zmapWindowFocusRemoveFocusItem(window->focus, feature_item) ;
      result = TRUE ;
    }

  return result ;
}




/* Highlight a feature or list of related features (e.g. all hits for same query sequence). */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names, gboolean sub_part)
{
  zmapWindowHighlightObject(window, item, replace_highlight_item, highlight_same_names, sub_part) ;

  return ;
}




void zmapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item,
			       gboolean replace_highlight_item, gboolean highlight_same_names, gboolean sub_part)
{
  ZMapWindowCanvasItem canvas_item ;
  ZMapFeature feature ;


  canvas_item = zMapWindowCanvasItemIntervalGetObject(item) ;
  zMapAssert(ZMAP_IS_CANVAS_ITEM(canvas_item)) ;


  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature((FooCanvasItem *)canvas_item);
  zMapAssert(feature) ;


  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if (replace_highlight_item)
    zmapWindowUnHighlightFocusItems(window) ;


  /* For some types of feature we want to highlight all the ones with the same name in that column. */
  switch (feature->type)
    {
    case ZMAPSTYLE_MODE_ALIGNMENT:
      {
	if (highlight_same_names)
	  {
	    GList *set_items ;
	    char *set_strand, *set_frame ;

	    set_strand = set_frame = "*" ;

	    set_items = zmapWindowFToIFindSameNameItems(window,window->context_to_item, set_strand, set_frame, feature) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    if (set_items)
	      zmapWindowFToIPrintList(set_items) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    zmapWindowFocusAddItems(window->focus, set_items, item) ; // item is the hot one

	    g_list_free(set_items) ;
	  }
	else
	  {
	    zmapWindowFocusAddItem(window->focus, item , feature) ;
	  }

	break ;
      }
    default:
      {
	/* Try highlighting both the item and its column. */
        zmapWindowFocusAddItem(window->focus, item, feature);
      }
      break ;
    }



  /* Highlight DNA and Peptide sequences corresponding to feature (if visible),
   * note we only do this if the feature is forward or non-stranded, makes no sense otherwise. */
  if (zMapStyleIsStrandSpecific(*feature->style) && ZMAPFEATURE_REVERSE(feature))
    {
      /* Deselect any already selected sequence as focus item is now the reverse strand item. */
      zmapWindowItemUnHighlightDNA(window, item) ;

      zmapWindowItemUnHighlightTranslations(window, item) ;

      zmapWindowItemUnHighlightShowTranslations(window, item) ;
    }
  else
    {
	ZMapFrame frame = ZMAPFRAME_NONE;
	;
      zmapWindowItemHighlightDNARegion(window, TRUE, sub_part, item,
				       ZMAPFRAME_NONE, ZMAPSEQUENCE_NONE, feature->x1, feature->x2, 0) ;

      zmapWindowItemHighlightTranslationRegions(window, TRUE, sub_part, item,
						ZMAPFRAME_NONE, ZMAPSEQUENCE_NONE, feature->x1, feature->x2, 0) ;


      /* PROBABLY NEED TO DISABLE THIS UNTIL I CAN GET IT ALL WORKING...... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Not completely happy with this...seems a bit hacky, try it and if users want to do
       * translations non-transcripts then I'll change it. */
      if (ZMAPFEATURE_IS_TRANSCRIPT(feature))
	zmapWindowItemHighlightShowTranslationRegion(window, TRUE, sub_part, item,
						     ZMAPFRAME_NONE, ZMAPSEQUENCE_NONE, feature->x1, feature->x2) ;
      else
	zmapWindowItemUnHighlightShowTranslations(window, item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      zmapWindowItemHighlightShowTranslationRegion(window, TRUE, sub_part, item,
						   frame, ZMAPSEQUENCE_NONE, feature->x1, feature->x2, 0) ;



    }




  zmapWindowHighlightFocusItems(window);

  return ;
}




void zmapWindowHighlightFocusItems(ZMapWindow window)
{
  FooCanvasItem *hot_item ;
  FooCanvasGroup *hot_column = NULL;

  /* If any other feature(s) is currently in focus, revert it to its std colours */
  if ((hot_column = zmapWindowFocusGetHotColumn(window->focus)))
    zmapWindowFocusHighlightHotColumn(window->focus) ;

  if ((hot_item = zmapWindowFocusGetHotItem(window->focus)))
    zmapWindowFocusHighlightFocusItems(window->focus, window) ;


  return ;
}



void zmapWindowUnHighlightFocusItems(ZMapWindow window)
{
//  FooCanvasItem *hot_item ;
//  FooCanvasGroup *hot_column ;

  /* If any other feature(s) is currently in focus, revert it to its std colours */
//  hot_column = zmapWindowFocusGetHotColumn(window->focus);
// if (hot_column)
//  zmapWindowFocusUnHighlightHotColumn(window) ;     // done by reset

/* this sets the feature, loosing the currently selected one */
//if ((hot_item = zmapWindowFocusGetHotItem(window->focus)))
//    zmapWindowFocusUnhighlightFocusItems(window->focus, window) ;

//  if (hot_column || hot_item)

/* don't need any of the above! */
    zmapWindowFocusReset(window->focus) ;

  return ;
}


/*
 *                         Functions to get hold of items in various ways.
 */


/*

   (TYPE)zmapWindowItemGetFeatureAny(ITEM, STRUCT_TYPE)

#define zmapWindowItemGetFeatureSet(ITEM)   (ZMapFeatureContext)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_CONTEXT)
#define zmapWindowItemGetFeatureAlign(ITEM) (ZMapFeatureAlign)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_ALIGN)
#define zmapWindowItemGetFeatureBlock(ITEM) (ZMapFeatureBlock)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_BLOCK)
#define zmapWindowItemGetFeatureSet(ITEM)   (ZMapFeatureSet)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_FEATURESET)
#define zmapWindowItemGetFeature(ITEM)      (ZMapFeature)zmapWindowItemGetFeatureAnyType((ITEM), ZMAPFEATURE_STRUCT_FEATURE)
#define zmapWindowItemGetFeatureAny(ITEM)   zmapWindowItemGetFeatureAnyType((ITEM), -1)
 */


ZMapFeatureBlock zmapWindowItemGetFeatureBlock(FooCanvasItem *item)
{
  ZMapFeatureBlock block = NULL ;

  block = (ZMapFeatureBlock)zmapWindowItemGetFeatureAnyType(item, ZMAPFEATURE_STRUCT_BLOCK) ;

  return block ;
}


ZMapFeature zmapWindowItemGetFeature(FooCanvasItem *item)
{
  ZMapFeature feature = NULL ;

  feature = (ZMapFeature)zmapWindowItemGetFeatureAnyType(item, ZMAPFEATURE_STRUCT_FEATURE) ;

  return feature ;
}


ZMapFeatureAny zmapWindowItemGetFeatureAny(FooCanvasItem *item)
{
  ZMapFeatureAny feature_any = NULL ;

  /* -1 means don't check */
  feature_any = zmapWindowItemGetFeatureAnyType(item, -1) ;

  return feature_any ;
}


ZMapFeatureAny zmapWindowItemGetFeatureAnyType(FooCanvasItem *item, ZMapFeatureStructType expected_type)
{
  ZMapFeatureAny feature_any = NULL ;
  ZMapWindowCanvasItem feature_item ;



  /* THE FIRST AND SECOND if's SHOULD BE MOVED TO THE items subdirectory which should have a
   * function that returns the feature given an item or a subitem... */
  if (ZMAP_IS_CONTAINER_GROUP(item))
    {
      zmapWindowContainerGetFeatureAny((ZMapWindowContainerGroup)item, &feature_any) ;
    }
  else if ((feature_item = zMapWindowCanvasItemIntervalGetObject(item)))
    {
      feature_any = (ZMapFeatureAny)zMapWindowCanvasItemGetFeature(FOO_CANVAS_ITEM(feature_item)) ;
    }
  else
    {
      zMapLogMessage("Unexpected item [%s]", G_OBJECT_TYPE_NAME(item)) ;
    }

  if (feature_any && expected_type != -1)
    {
      if (feature_any->struct_type != expected_type)
	{
	  zMapLogCritical("Unexpected feature type [%d] (not %d) attached to item [%s]",
			  feature_any->struct_type, expected_type, G_OBJECT_TYPE_NAME(item)) ;
	  feature_any = NULL;
	}
    }

  return feature_any ;
}


#warning this function does nothing but is called five times and should be removed
/* Get "parent" item of feature, for simple features, this is just the item itself but
 * for compound features we need the parent group.
 *  */
FooCanvasItem *zmapWindowItemGetTrueItem(FooCanvasItem *item)
{
  FooCanvasItem *true_item = NULL ;

  true_item = item;

  return true_item ;
}






/*
 * THESE FUNCTIONS ALL FEEL LIKE THEY SHOULD BE IN THE items directory somewhere.....
 * THERE IS MUCH CODE THAT SHOULD BE MOVED FROM HERE INTO THERE.....
 */


/* Returns the container parent of the given canvas item which may not be feature, it might be
   some decorative box, e.g. as in the alignment colinear bars. */
FooCanvasGroup *zmapWindowItemGetParentContainer(FooCanvasItem *feature_item)
{
  FooCanvasGroup *parent_container = NULL ;

  parent_container = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(feature_item);

  return parent_container ;
}



/* Finds the feature item in a window corresponding to the supplied feature item...which is
 * usually one from a different window....
 *
 * This routine can return NULL if the user has two different sequences displayed and hence
 * there will be items in one window that are not present in another.
 */
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowContainerFeatureSet container;

  /* Retrieve the feature item info from the canvas item. */
  feature = zMapWindowCanvasItemGetFeature(item) ;
  zMapAssert(feature);

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  matching_item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
						    container->strand, container->frame,
						    feature) ;

  return matching_item ;
}


/* Finds the feature item child in a window corresponding to the supplied feature item..which is
 * usually one from a different window....
 * A feature item child is something like the feature item representing an exon within a transcript. */
FooCanvasItem *zMapWindowFindFeatureItemChildByItem(ZMapWindow window, FooCanvasItem *item,
						    int child_start, int child_end)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowContainerFeatureSet container;

  zMapAssert(window && item && child_start > 0 && child_end > 0 && child_start <= child_end) ;


  /* Retrieve the feature item info from the canvas item. */
  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;

  container = (ZMapWindowContainerFeatureSet)zmapWindowContainerCanvasItemGetContainer(item) ;

  /* Find the item that matches */
  matching_item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
						container->strand, container->frame, feature) ;

  return matching_item ;
}


/*
 *                  Testing items visibility and scrolling to those items.
 */

/* Checks to see if the item really is visible.  In order to do this
 * all the item's parent groups need to be examined.
 *
 *  mh17:not called from anywhere
 */
gboolean zmapWindowItemIsVisible(FooCanvasItem *item)
{
  gboolean visible = FALSE;

  visible = zmapWindowItemIsShown(item);

#ifdef RDS_DONT
  /* we need to check out our parents :( */
  /* we would like not to do this */
  if(visible)
    {
      FooCanvasGroup *parent = NULL;
      parent = FOO_CANVAS_GROUP(item->parent);
      while(visible && parent)
        {
          visible = zmapWindowItemIsShown(FOO_CANVAS_ITEM(parent));
          /* how many parents we got? */
          parent  = FOO_CANVAS_GROUP(FOO_CANVAS_ITEM(parent)->parent);
        }
    }
#endif

  return visible;
}


/* Checks to see if the item is shown.  An item may still not be
 * visible as any one of its parents might be hidden. If a
 * definitive answer is required use zmapWindowItemIsVisible instead. */
gboolean zmapWindowItemIsShown(FooCanvasItem *item)
{
  gboolean visible = FALSE;

  zMapAssert(FOO_IS_CANVAS_ITEM(item)) ;

  g_object_get(G_OBJECT(item),
               "visible", &visible,
               NULL);

  return visible;
}



/* Tests to see whether an item is visible on the screen. If "completely" is TRUE then the
 * item must be completely on the screen otherwise FALSE is returned. */
gboolean zmapWindowItemIsOnScreen(ZMapWindow window, FooCanvasItem *item, gboolean completely)
{
  gboolean is_on_screen = FALSE ;
  double screenx1_out, screeny1_out, screenx2_out, screeny2_out ;
  double itemx1_out, itemy1_out, itemx2_out, itemy2_out ;

  /* Work out which part of the canvas is visible currently. */
  getVisibleCanvas(window, &screenx1_out, &screeny1_out, &screenx2_out, &screeny2_out) ;

  /* Get the items world coords. */
  my_foo_canvas_item_get_world_bounds(item, &itemx1_out, &itemy1_out, &itemx2_out, &itemy2_out) ;

  /* Compare them. */
  if (completely)
    {
      if (itemx1_out >= screenx1_out && itemx2_out <= screenx2_out
	  && itemy1_out >= screeny1_out && itemy2_out <= screeny2_out)
	is_on_screen = TRUE ;
      else
	is_on_screen = FALSE ;
    }
  else
    {
      if (itemx2_out < screenx1_out || itemx1_out > screenx2_out
	  || itemy2_out < screeny1_out || itemy1_out > screeny2_out)
	is_on_screen = FALSE ;
      else
	is_on_screen = TRUE ;
    }

  return is_on_screen ;
}


#if 0
only ever use by centre on sub_part below
scroll down for alternate code (1 lines)
/* if(!zmapWindowItemRegionIsVisible(window, item))
 *   zmapWindowItemCentreOnItem(window, item, changeRegionSize, boundarySize);
 */
gboolean zmapWindowItemRegionIsVisible(ZMapWindow window, FooCanvasItem *item)
{
  gboolean visible = FALSE;
  double wx1, wx2, wy1, wy2;
  double ix1, ix2, iy1, iy2;
  double dummy_x = 0.0;
  double feature_x1 = 0.0, feature_x2 = 0.0 ;
  ZMapFeature feature;

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);
  foo_canvas_item_i2w(item, &dummy_x, &iy1);
  foo_canvas_item_i2w(item, &dummy_x, &iy2);

  feature = (ZMapFeature) zmapWindowItemGetFeatureAnyType(item, -1) ;
  zMapAssert(feature) ;

  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2);

  /* Get scroll region (clamped to sequence coords) */
  zmapWindowGetScrollRegion(window, &wx1, &wy1, &wx2, &wy2);

  wx2 = feature_x2 + 1;
  if(feature_x1 >= wx1 && feature_x2 <= wx2  &&
     iy1 >= wy1 && iy2 <= wy2)
    {
      visible = TRUE;
    }

  return visible;
}
#endif


/* Scroll to the specified item.
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  zMapAssert(window && item) ;

  if ((result = zmapWindowItemIsShown(item)))
    {
      zmapWindowItemCentreOnItem(window, item, FALSE, 100.0) ;

      result = TRUE ;
    }

  return result ;
}


/* Moves to the given item and optionally changes the size of the scrolled region
 * in order to display the item. */
void zmapWindowItemCentreOnItem(ZMapWindow window, FooCanvasItem *item,
                                gboolean alterScrollRegionSize,
                                double boundaryAroundItem)
{
  zmapWindowItemCentreOnItemSubPart(window, item, alterScrollRegionSize, boundaryAroundItem, 0.0, 0.0) ;

  return ;
}


/* Moves to a subpart of an item, note the coords sub_start/sub_end need to be item coords,
 * NOT world coords. If sub_start == sub_end == 0.0 then the whole item is centred on. */
/* NOTE (mh17) must be sequence coordinates
 * (not world)
 */

void zmapWindowItemCentreOnItemSubPart(ZMapWindow window, FooCanvasItem *item,
				       gboolean alterScrollRegionSize,
				       double boundaryAroundItem,
				       double sub_start, double sub_end)
{
  double ix1, ix2, iy1, iy2;
  int cx1, cx2, cy1, cy2;
  int final_canvasx, final_canvasy, tmpx, tmpy, cheight, cwidth;
  static gboolean debug = FALSE ;


  /* OH GOSH....THIS FUNCTION IS HARD TO FOLLOW, SOME COMMENTING WOULD HAVE HELPED. */
  zMapAssert(window && item) ;

  if (zmapWindowItemIsShown(item))
    {
      /* THIS CODE IS NOT GREAT AND POINTS TO SOME FUNDAMENTAL ROUTINES/UNDERSTANDING THAT IS
       * MISSING.....WE NEED TO SORT OUT WHAT SIZE WE NEED TO CALCULATE FROM AS OPPOSED TO
       * WHAT THE CURRENTLY DISPLAYED OBJECT SIZE IS, THIS MAY HAVE BEEN TRUNCATED BY THE
       * THE LONG ITEM CLIP CODE OR JUST BY CHANGING THE SCROLLED REGION. */

      foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2) ;

#if NOT_RELEVANT
/* this will never be a group now */
      /* If the item is a group then we need to use its background to check in long items as the
       * group itself is not a long item. */
      if (FOO_IS_CANVAS_GROUP(item) && zmapWindowContainerUtilsIsValid(FOO_CANVAS_GROUP(item)))
	{
	  double height ;
	  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2) ;

	  height = iy2 - iy1 + 1;
	  if (iy1 > 0)

	    iy1 = 0 ;
	  if (iy2 < height)
	    iy2 = height ;
	}
      else
	{
	  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2) ;
	}
#endif


      /* If sub_part start/end are set then need to centre on that part of an item. */
      if (sub_start != 0.0 || sub_end != 0.0)
	{
#if USING_ITEM_COORDS
		zMapAssert(sub_start <= sub_end
		     && (sub_start >= iy1 && sub_start <= iy2)
		     && (sub_end >= iy1 && sub_end <= iy2)) ;

	  iy2 = iy1 ;
	  iy1 = iy1 + sub_start ;
	  iy2 = iy2 + sub_end ;
#else
	  iy1 = sub_start;
	  iy2 = sub_end;
#endif
	}

#if USING_ITEM_COORDS
	/* Fix the item coords to be in world coords for following calculations. */
      foo_canvas_item_i2w(item, &ix1, &iy1);
      foo_canvas_item_i2w(item, &ix2, &iy2);
#endif


      if (boundaryAroundItem > 0.0)
	{
	  /* sadly I don't know what PPU stand for.... */

	  /* think about PPU for X & Y!! */
	  ix1 -= boundaryAroundItem;
	  iy1 -= boundaryAroundItem;
	  ix2 += boundaryAroundItem;
	  iy2 += boundaryAroundItem;
	}


      if (alterScrollRegionSize)
	{
	  /* this doeesn't work. don't know why. */
	  double sy1, sy2;
	  sy1 = iy1; sy2 = iy2;
	  zmapWindowClampSpan(window, &sy1, &sy2);
	  zMapWindowMove(window, sy1, sy2);
	}
      else
	{
        double sx1, sx2, sy1, sy2, tmps, tmpi, diff;
	      /* Get scroll region (clamped to sequence coords) */
	  zmapWindowGetScrollRegion(window, &sx1, &sy1, &sx2, &sy2);

//	  if (!zmapWindowItemRegionIsVisible(window, item))
	  if(iy1 < sy1 || iy2 > sy2)	/* as iy1 < iy2 that's all we need to test */
	    {

	      /* we now have scroll region coords */
	      /* set tmp to centre of regions ... */
	      tmps = sy1 + ((sy2 - sy1) / 2);
	      tmpi = iy1 + ((iy2 - iy1) / 2);

	      /* find difference between centre points */
	      diff = tmps - tmpi;

	      /* alter scroll region to match that */
	      sy1 -= diff; sy2 -= diff;

	      /* clamp in case we've moved outside the sequence */
	      zmapWindowClampSpan(window, &sy1, &sy2);

	      /* Do the move ... */
#warning this assumes the scroll region is big enough for the DNA match, but some people have searched for big sequences
	      zMapWindowMove(window, sy1, sy2);
	    }
	}



      /* THERE IS A PROBLEM WITH HORIZONTAL SCROLLING HERE, THE CODE DOES NOT SCROLL FAR
       * ENOUGH TO THE RIGHT...I'M NOT SURE WHY THIS IS AT THE MOMENT. */


      /* get canvas coords to work with */
      foo_canvas_w2c(item->canvas, ix1, iy1, &cx1, &cy1);
      foo_canvas_w2c(item->canvas, ix2, iy2, &cx2, &cy2);

      if (debug)
	{
	  printf("[zmapWindowItemCentreOnItem] ix1=%f, ix2=%f, iy1=%f, iy2=%f\n", ix1, ix2, iy1, iy2);
	  printf("[zmapWindowItemCentreOnItem] cx1=%d, cx2=%d, cy1=%d, cy2=%d\n", cx1, cx2, cy1, cy2);
	}

      /* This should possibly be a function...YEH, WHAT DOES IT DO.....????? */
      tmpx = cx2 - cx1;
      tmpy = cy2 - cy1;

      if(tmpx & 1)
	tmpx += 1;
      if(tmpy & 1)
	tmpy += 1;

      final_canvasx = cx1 + (tmpx / 2);
      final_canvasy = cy1 + (tmpy / 2);

      tmpx = GTK_WIDGET(window->canvas)->allocation.width;
      tmpy = GTK_WIDGET(window->canvas)->allocation.height;

      if(tmpx & 1)
	tmpx -= 1;
      if(tmpy & 1)
	tmpy -= 1;

      cwidth = tmpx / 2;
      cheight = tmpy / 2;
      final_canvasx -= cwidth;
      final_canvasy -= cheight;

      if(debug)
	{
	  printf("[zmapWindowItemCentreOnItem] cwidth=%d  cheight=%d\n", cwidth, cheight);
	  printf("[zmapWindowItemCentreOnItem] scrolling to"
		 " canvas x=%d y=%d\n",
		 final_canvasx, final_canvasy);
	}

      /* Scroll to the item... */
      foo_canvas_scroll_to(window->canvas, final_canvasx, final_canvasy);
    }

  return ;
}





/* Scrolls to an item if that item is not visible on the scren.
 *
 * NOTE: scrolling is only done in the axis in which the item is completely
 * invisible, the other axis is left unscrolled so that the visible portion
 * of the feature remains unaltered.
 *  */
void zmapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  double screenx1_out, screeny1_out, screenx2_out, screeny2_out ;
  double itemx1_out, itemy1_out, itemx2_out, itemy2_out ;
  gboolean do_x = FALSE, do_y = FALSE ;
  double x_offset = 0.0, y_offset = 0.0;
  int curr_x, curr_y ;

  /* Work out which part of the canvas is visible currently. */
  getVisibleCanvas(window, &screenx1_out, &screeny1_out, &screenx2_out, &screeny2_out) ;

  /* Get the items world coords. */
  my_foo_canvas_item_get_world_bounds(item, &itemx1_out, &itemy1_out, &itemx2_out, &itemy2_out) ;

  /* Get the current scroll offsets in world coords. */
  foo_canvas_get_scroll_offsets(window->canvas, &curr_x, &curr_y) ;
  foo_canvas_c2w(window->canvas, curr_x, curr_y, &x_offset, &y_offset) ;

  /* Work out whether any scrolling is required. */
  if (itemx1_out > screenx2_out || itemx2_out < screenx1_out)
    {
      do_x = TRUE ;

      if (itemx1_out > screenx2_out)
	{
	  x_offset = itemx1_out ;
	}
      else if (itemx2_out < screenx1_out)
	{
	  x_offset = itemx1_out ;
	}
    }

  if (itemy1_out > screeny2_out || itemy2_out < screeny1_out)
    {
      do_y = TRUE ;

      if (itemy1_out > screeny2_out)
	{
	  y_offset = itemy1_out ;
	}
      else if (itemy2_out < screeny1_out)
	{
	  y_offset = itemy1_out ;
	}
    }

  /* If we need to scroll then do it. */
  if (do_x || do_y)
    {
      foo_canvas_w2c(window->canvas, x_offset, y_offset, &curr_x, &curr_y) ;

      foo_canvas_scroll_to(window->canvas, curr_x, curr_y) ;
    }

  return ;
}




#if MH17_NOT_USED
/* moves a feature to new coordinates */
void zMapWindowMoveItem(ZMapWindow window, ZMapFeature origFeature,
			ZMapFeature modFeature, FooCanvasItem *item)
{
  double top, bottom, offset;

  if (FOO_IS_CANVAS_ITEM (item))
    {
      offset = ((ZMapFeatureBlock)(modFeature->parent->parent))->block_to_sequence.q1;
      top = modFeature->x1;
      bottom = modFeature->x2;
      zmapWindowSeq2CanOffset(&top, &bottom, offset);

      if (modFeature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
	{
	  zMapAssert(origFeature);

	  foo_canvas_item_set(item->parent, "y", top, NULL);
	}
      else
	{
	  foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
	}

      zmapWindowUpdateInfoPanel(window, modFeature, NULL, item, NULL, 0, 0, NULL, TRUE, TRUE) ;
    }
  return;
}
#endif




/* Returns the sequence coords that correspond to the given _world_ foocanvas coords.
 *
 * NOTE that although we only return y coords, we need the world x coords as input
 * in order to find the right foocanvas item from which to get the sequence coords.
 *
 * Not so easy, we are poking around in the dark....
 *  */
gboolean zmapWindowWorld2SeqCoords(ZMapWindow window, FooCanvasItem *foo,
				   double wx1, double wy1, double wx2, double wy2,
				   FooCanvasGroup **block_grp_out, int *y1_out, int *y2_out)
{
  gboolean result = FALSE ;
  FooCanvasItem *item ;
  double mid_x, mid_y ;

  /* Try to get an item at the mid point... we have to start somewhere... */
  mid_x = (wx1 + wx2) / 2 ;
  mid_y = (wy1 + wy2) / 2 ;

  item = foo;
  zMapAssert(item);	/* focus item passed in, we always have focus if we get here */
#if 0
  if(!item)		/* should never be called: legacy code */
  	item = foo_canvas_get_item_at(window->canvas, mid_x, mid_y);
#endif
  if(item)
    {
      FooCanvasGroup *block_container ;
      ZMapFeatureBlock block ;

      /* Getting the block struct as well is a bit belt and braces...we could return it but
       * its redundant info. really. */
      if ((block_container = FOO_CANVAS_GROUP(zmapWindowContainerUtilsItemGetParentLevel(item, ZMAPCONTAINER_LEVEL_BLOCK)))
	  && (block = zmapWindowItemGetFeatureBlock((FooCanvasItem *)block_container)))
	{
	  double offset ;

	  offset = (double)(block->block_to_sequence.block.x1 - 1) ; /* - 1 for 1 based coord system. */

	  my_foo_canvas_world_bounds_to_item(FOO_CANVAS_ITEM(block_container), &wx1, &wy1, &wx2, &wy2) ;

	  if (block_grp_out)
	    *block_grp_out = block_container ;

	  if (y1_out)
	    *y1_out = floor(wy1 + offset + 0.5) ;
	  if (y2_out)
	    *y2_out = floor(wy2 + offset + 0.5) ;

	  result = TRUE ;
	}
      else
	zMapLogWarning("%s", "No Block Container");
    }

  return result ;
}


gboolean zmapWindowItem2SeqCoords(FooCanvasItem *item, int *y1, int *y2)
{
  gboolean result = FALSE ;



  return result ;
}


/**
 * my_foo_canvas_item_get_world_bounds:
 * @item: A canvas item.
 * @rootx1_out: X left coord of item (output value).
 * @rooty1_out: Y top coord of item (output value).
 * @rootx2_out: X right coord of item (output value).
 * @rootx2_out: Y bottom coord of item (output value).
 *
 * Returns the _world_ bounds of an item, no function exists in FooCanvas to do this.
 **/
void my_foo_canvas_item_get_world_bounds(FooCanvasItem *item,
					 double *rootx1_out, double *rooty1_out,
					 double *rootx2_out, double *rooty2_out)
{
  double rootx1, rootx2, rooty1, rooty2 ;

  g_return_if_fail (FOO_IS_CANVAS_ITEM (item)) ;
  g_return_if_fail (rootx1_out != NULL || rooty1_out != NULL || rootx2_out != NULL || rooty2_out != NULL ) ;

  foo_canvas_item_get_bounds(item, &rootx1, &rooty1, &rootx2, &rooty2) ;
  foo_canvas_item_i2w(item, &rootx1, &rooty1) ;
  foo_canvas_item_i2w(item, &rootx2, &rooty2) ;

  if (rootx1_out)
    *rootx1_out = rootx1 ;
  if (rooty1_out)
    *rooty1_out = rooty1 ;
  if (rootx2_out)
    *rootx2_out = rootx2 ;
  if (rooty2_out)
    *rooty2_out = rooty2 ;

  return ;
}




/**
 * my_foo_canvas_world_bounds_to_item:
 * @item: A canvas item.
 * @rootx1_out: X left coord of item (input/output value).
 * @rooty1_out: Y top coord of item (input/output value).
 * @rootx2_out: X right coord of item (input/output value).
 * @rootx2_out: Y bottom coord of item (input/output value).
 *
 * Returns the given _world_ bounds in the given item coord system, no function exists in FooCanvas to do this.
 **/
void my_foo_canvas_world_bounds_to_item(FooCanvasItem *item,
					double *rootx1_inout, double *rooty1_inout,
					double *rootx2_inout, double *rooty2_inout)
{
  double rootx1, rootx2, rooty1, rooty2 ;

  g_return_if_fail (FOO_IS_CANVAS_ITEM (item)) ;
  g_return_if_fail (rootx1_inout != NULL || rooty1_inout != NULL || rootx2_inout != NULL || rooty2_inout != NULL ) ;

  rootx1 = rootx2 = rooty1 = rooty2 = 0.0 ;

  if (rootx1_inout)
    rootx1 = *rootx1_inout ;
  if (rooty1_inout)
    rooty1 = *rooty1_inout ;
  if (rootx2_inout)
    rootx2 = *rootx2_inout ;
  if (rooty2_inout)
    rooty2 = *rooty2_inout ;

  foo_canvas_item_w2i(item, &rootx1, &rooty1) ;
  foo_canvas_item_w2i(item, &rootx2, &rooty2) ;

  if (rootx1_inout)
    *rootx1_inout = rootx1 ;
  if (rooty1_inout)
    *rooty1_inout = rooty1 ;
  if (rootx2_inout)
    *rootx2_inout = rootx2 ;
  if (rooty2_inout)
    *rooty2_inout = rooty2 ;

  return ;
}





/* I'M TRYING THESE TWO FUNCTIONS BECAUSE I DON'T LIKE THE BIT WHERE IT GOES TO THE ITEMS
 * PARENT IMMEDIATELY....WHAT IF THE ITEM IS ALREADY A GROUP ?????? THE ORIGINAL FUNCTION
 * CANNOT BE USED TO CONVERT A GROUPS POSITION CORRECTLY......S*/

/**
 * my_foo_canvas_item_w2i:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from world coordinates to item-relative
 * coordinates.
 **/
void my_foo_canvas_item_w2i (FooCanvasItem *item, double *x, double *y)
{
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* orginal code... */
  item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!FOO_IS_CANVAS_GROUP (item))
    item = item->parent;

  while (item) {
    if (FOO_IS_CANVAS_GROUP (item)) {
      *x -= FOO_CANVAS_GROUP (item)->xpos;
      *y -= FOO_CANVAS_GROUP (item)->ypos;
    }

    item = item->parent;
  }

  return ;
}


/**
 * my_foo_canvas_item_i2w:
 * @item: A canvas item.
 * @x: X coordinate to convert (input/output value).
 * @y: Y coordinate to convert (input/output value).
 *
 * Converts a coordinate pair from item-relative coordinates to world
 * coordinates.
 **/
void my_foo_canvas_item_i2w (FooCanvasItem *item, double *x, double *y)
{
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (x != NULL);
  g_return_if_fail (y != NULL);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Original code... */
  item = item->parent;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!FOO_IS_CANVAS_GROUP (item))
    item = item->parent;

  while (item) {
    if (FOO_IS_CANVAS_GROUP (item)) {
      *x += FOO_CANVAS_GROUP (item)->xpos;
      *y += FOO_CANVAS_GROUP (item)->ypos;
    }

    item = item->parent;
  }

  return ;
}

/* A major problem with foo_canvas_item_move() is that it moves an item by an
 * amount rather than moving it somewhere in the group which is painful for operations
 * like unbumping a column. */
void my_foo_canvas_item_goto(FooCanvasItem *item, double *x, double *y)
{
  double x1, y1, x2, y2 ;
  double dx, dy ;

  if (x || y)
    {
      x1 = y1 = x2 = y2 = 0.0 ;
      dx = dy = 0.0 ;

      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

      if (x)
	dx = *x - x1 ;
      if (y)
	dy = *y - y1 ;

      foo_canvas_item_move(item, dx, dy) ;
    }

  return ;
}



/* This function returns the visible _world_ coords. Be careful how you use
 * the returned coords as there may not be any of our items at the extreme
 * extents of this range (e.g. x = 0.0 !!). */
void zmapWindowItemGetVisibleWorld(ZMapWindow window,
				   double *wx1, double *wy1,
				   double *wx2, double *wy2)
{

  getVisibleCanvas(window, wx1, wy1, wx2, wy2);

  return ;
}




/*
 *                  Internal routines.
 */













/* Get the visible portion of the canvas. */
static void getVisibleCanvas(ZMapWindow window,
			     double *screenx1_out, double *screeny1_out,
			     double *screenx2_out, double *screeny2_out)
{
  GtkAdjustment *v_adjust, *h_adjust ;
  double x1, x2, y1, y2 ;

  /* Work out which part of the canvas is visible currently. */
  v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
  h_adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  x1 = h_adjust->value ;
  x2 = x1 + h_adjust->page_size ;

  y1 = v_adjust->value ;
  y2 = y1 + v_adjust->page_size ;

  foo_canvas_window_to_world(window->canvas, x1, y1, screenx1_out, screeny1_out) ;
  foo_canvas_window_to_world(window->canvas, x2, y2, screenx2_out, screeny2_out) ;

  return ;
}

