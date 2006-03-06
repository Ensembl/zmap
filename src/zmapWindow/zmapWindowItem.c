/*  File: zmapWindowItem.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Functions to manipulate canvas items.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Mar  6 09:51 2006 (edgrif)
 * Created: Thu Sep  8 10:37:24 2005 (edgrif)
 * CVS info:   $Id: zmapWindowItem.c,v 1.16 2006-03-06 11:48:23 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>

/* Used to hold highlight information for the hightlight callback function. */
typedef struct
{
  ZMapWindow window ;
  gboolean rev_video ;
} HighlightStruct, *Highlight ;


static void highlightItem(ZMapWindow window, FooCanvasItem *item);
static void highlightFuncCB(gpointer data, gpointer user_data);
static void setItemColourRevVideo(ZMapWindow window, FooCanvasItem *item);
static void setItemColourOriginal(ZMapWindow window, FooCanvasItem *item);

static void checkScrollRegion(ZMapWindow window, double start, double end) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* I THOUGHT I NEEDED THIS BUT I DON'T JUST YET, I THINK WE WILL SO I'VE LEFT THIS
 * SKELETON CODE HERE FOR NOW.... */

/* Find the nearest canvas feature item in a given feature set to a given coord.
 * 
 * If the coord is not within the bounds of the given feature set then NULL is
 * returned.
 * 
 *  */
FooCanvasItem *zmapWindowFindNearestItem(FooCanvasGroup *column_group, double x, double y)
{
  FooCanvasItem *item = NULL ;
  double x1, x2, y1, y2 ;


  zMapAssert(column_group && FOO_IS_CANVAS_GROUP(column_group) && x >= 0 && y >= 0) ;

  
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(column_group), &x1, &y1, &x2, &y2) ;

  /* Safety check really...caller should be sending us the exact event coords for the column
   * group. */
  if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
    {
      FooCanvasItem *closest_item ;
      

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      GList *item_list



	void        g_list_foreach                  (GList *list,
						     GFunc func,
						     gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




    }


  return item ;
}



void checkCoords(gpointer data, gpointer user_data)
{




}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Highlight a feature, note how this function should just take _any_ feature/item but it doesn't 
 * and so needs redoing....sigh....it should also be called something like focusOnItem() */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item)
{                                               
  ZMapWindowItemFeatureType item_feature_type ;
  FooCanvasItem *fresh_focus_item = NULL;

  /* If any other feature is currently in focus, revert it to its std colours */
  if ((fresh_focus_item = zmapWindowItemHotFocusItem(window)))
    {
      highlightItem(window, fresh_focus_item) ;
      zmapWindowItemRemoveFocusItem(window, fresh_focus_item);
    }

  /* Highlight the new item. */
  highlightItem(window, item) ;

  /* Make this item the new focus item. */
  zmapWindowItemAddFocusItem(window, item);


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  /* Only raise this item to the top if it is the whole feature. */
  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    foo_canvas_item_raise_to_top(item) ;
  else if (item_feature_type == ITEM_FEATURE_CHILD)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'D LIKE TO RAISE THE WHOLE GROUP BUT THIS DOESN'T WORK SO WE JUST RAISE THE
       * CHILD ITEM...I DON'T LIKE THE FEEL OF THIS...DOES IT REALLY WORK.... */
      foo_canvas_item_raise_to_top(item->parent) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      foo_canvas_item_raise_to_top(item) ;
    }

  /* Make sure the canvas gets redisplayed immediately. */
  foo_canvas_update_now(window->canvas) ;


  return ;
}



/* Finds the feature item in a window corresponding to the supplied feature item..which is
 * usually one from a different window....
 * 
 * This routine can return NULL if the user has two different sequences displayed and hence
 * there will be items in one window that are not present in another.
 * 
 *  */
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item, feature) ;
    }
  else
    {
      ZMapWindowItemFeature item_subfeature_data ;

      item_subfeature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
								      ITEM_SUBFEATURE_DATA) ;

      matching_item = zmapWindowFToIFindItemChild(window->context_to_item, feature,
						  item_subfeature_data->start,
						  item_subfeature_data->end) ;
    }

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

  zMapAssert(window && item && child_start > 0 && child_end > 0 && child_start <= child_end) ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  /* Find the item that matches */
  matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item, feature) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I don't know what happened to this bit of code... */

  if (FOO_IS_CANVAS_GROUP( matching_item))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





  return matching_item ;
}


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

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;         /* this should never fail. */
  
  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2);

  wx1 = wx2 = wy1 = wy2 = 0.0;
  zmapWindowScrollRegionTool(window, &wx1, &wy1, &wx2, &wy2);
  wx2 = feature_x2 + 1;
  if(feature_x1 >= wx1 && feature_x2 <= wx2  &&
     iy1 >= wy1 && iy2 <= wy2)
    {
      visible = TRUE;
    }

  return visible;
}

void zmapWindowItemCentreOnItem(ZMapWindow window, FooCanvasItem *item,
                                gboolean alterScrollRegionSize,
                                double boundaryAroundItem)
{
  double ix1, ix2, iy1, iy2;
  int    cx1, cx2, cy1, cy2;

  foo_canvas_item_get_bounds(item, &ix1, &iy1, &ix2, &iy2);

  /* Fix the numbers to make sense. */
  foo_canvas_item_i2w(item, &ix1, &iy1);
  foo_canvas_item_i2w(item, &ix2, &iy2);

  if(boundaryAroundItem > 0.0)
    {
      ix1 -= boundaryAroundItem;
      iy1 -= boundaryAroundItem;
      ix2 += boundaryAroundItem;
      iy2 += boundaryAroundItem;
    }

  if(alterScrollRegionSize)
    {
      ix1 = ix2;
      zmapWindowScrollRegionTool(window, &ix1, &iy1, &ix2, &iy2);
    }
  else
    {
      if(!zmapWindowItemRegionIsVisible(window, item))
        {
          double wx1, wx2, wy1, wy2, nwy1, nwy2, tmpidiff, tmpwdiff;
          wx1 = wx2 = wy1 = wy2 = 0.0;
          zmapWindowScrollRegionTool(window, &wx1, &wy1, &wx2, &wy2);
          nwy1  = wy1; nwy2 = wy2;
          tmpwdiff = ((wy2 - wy1 + 1) / 2);
          tmpidiff = ( iy1 + ((iy2 - iy1 + 1) / 2));
          nwy1 -= (wy1 - tmpidiff) + (tmpwdiff);
          nwy2 -= (wy1 - tmpidiff) + (tmpwdiff);
          zMapWindowMove(window, nwy1, nwy2);
          /* zmapWindowScrollRegionTool(window, &wx1, &nwy1, &wx2, &nwy2);
           * zmapWindowLongItemCrop(window, wx1, nwy1, wx2, nwy2);*/
        }
     }
  foo_canvas_w2c(item->canvas, ix1, iy1, &cx1, &cy1); 
  foo_canvas_w2c(item->canvas, ix2, iy2, &cx2, &cy2); 
  printf("ix1, ix2, iy1, iy2 %f %f %f %f\n", ix1, ix2, iy1, iy2);
  printf("cx1, cx2, cy1, cy2 %d %d %d %d\n", cx1, cx2, cy1, cy2);
  {
    int cx, cy, tmpx, tmpy, cheight, cwidth;
    tmpx = cx2 - cx1; tmpy = cy2 - cy1;
    if(tmpx & 1)
      tmpx += 1;
    if(tmpy & 1)
      tmpy += 1;
    cx = cx1 + (tmpx / 2);
    cy = cy1 + (tmpy / 2);
    tmpx = GTK_WIDGET(window->canvas)->allocation.width;
    tmpy = GTK_WIDGET(window->canvas)->allocation.height;
    if(tmpx & 1)
      tmpx -= 1;
    if(tmpy & 1)
      tmpy -= 1;
    cwidth = tmpx / 2; cheight = tmpy / 2;
    printf("cwidth, cheight %d %d\n", cwidth, cheight);
    cx -= cwidth; cy -= cheight;
    printf("canvas x, y %d %d\n", cx, cy);
    foo_canvas_scroll_to(window->canvas, cx, cy);
  }

  return ;
}

/* Scroll to the specified item.
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  gboolean result = FALSE ;
  int cx1, cy1, cx2, cy2;
  double feature_x1 = 0.0, feature_x2 = 0.0 ;
  double x1, y1, x2, y2;
  ZMapFeature feature;

  zMapAssert(window && item) ;

  if(!(result = zmapWindowItemIsShown(item)))
    return result;

  zmapWindowItemCentreOnItem(window, item, FALSE, 100.0);

  /* highlight the item, which also does raise_to_top! */
  zMapWindowHighlightObject(window, item) ;
  
  /* Report the selected object to the layer above us. */
  if(window->caller_cbs->select != NULL)
    {
      ZMapWindowSelectStruct select = {NULL} ;
      /* Really we should create some text here as well.... */
      select.item = item ;
      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
    }


  return TRUE;

  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;         /* this should never fail. */
  
  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2);
  /* May need to move scroll region if object is outside it. */
  //  checkScrollRegion(window, feature_x1, feature_x2) ;
  
  /* scroll up or down to user's selection. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2); /* world coords */
  /* Fix the numbers to make sense. */
  foo_canvas_item_i2w(item, &x1, &y1);
  foo_canvas_item_i2w(item, &x2, &y2);

  if (y1 <= 0.0)    /* we might be dealing with a multi-box item, eg transcript */
    {
      double px1, py1, px2, py2;
      /* Is this used? */
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(item->parent),
				 &px1, &py1, &px2, &py2); 
      if (py1 > 0.0)
	y1 = py1;
    }
  
  /* convert to canvas pixels */
  foo_canvas_w2c(window->canvas, x1, y1, &cx1, &cy1); 
  foo_canvas_w2c(window->canvas, x2, y2, &cx2, &cy2); 
  {
    int cx, cy, tmpx, tmpy, cheight, cwidth;
    tmpx = cx2 - cx1; tmpy = cy2 - cy1;
    if(tmpx & 1)
      tmpx += 1;
    if(tmpy & 1)
      tmpy += 1;
    cx = cx1 + (tmpx / 2);
    cy = cy1 + (tmpy / 2);
    tmpx = GTK_WIDGET(window->canvas)->allocation.width;
    tmpy = GTK_WIDGET(window->canvas)->allocation.height;
    if(tmpx & 1)
      tmpx -= 1;
    if(tmpy & 1)
      tmpy -= 1;
    cwidth = tmpx / 2; cheight = tmpy / 2;
    cx -= cwidth; cy -= cheight;
    foo_canvas_scroll_to(window->canvas, cx, cy);
  }

  /* highlight the item, which also does raise_to_top! */
  zMapWindowHighlightObject(window, item) ;
  
  /* Report the selected object to the layer above us. */
  if(window->caller_cbs->select != NULL)
    {
      ZMapWindowSelectStruct select = {NULL} ;
      /* Really we should create some text here as well.... */
      select.item = item ;
      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
    }

  result = TRUE ;

  return result ;
}


gboolean zmapWindowItemAddFocusItem(ZMapWindow window, FooCanvasItem *item)
{
  gboolean unique = TRUE;
  GList *list = NULL;

  if((list = g_list_find(window->focusItemSet, item)) != NULL)
    unique = FALSE;

  if(unique)
    window->focusItemSet = g_list_append(window->focusItemSet, item);
  else
    {
      printf("I think the item should be moved to the last position");
      /* -- EASY METHOD?? --
       * zmapWindowItemRemoveFocusItem(window, item);
       * zmapWindowItemAddFocusItem(window, item);
       */
    }


  return unique;
}

FooCanvasItem *zmapWindowItemHotFocusItem(ZMapWindow window)
{
  FooCanvasItem *fresh = NULL;
  GList *last = NULL;

  if((last = g_list_last(window->focusItemSet)))
    fresh = (FooCanvasItem *)(last->data);

  return fresh;
}


/* I think this is incapable of failing. */
void zmapWindowItemRemoveFocusItem(ZMapWindow window, FooCanvasItem *item)
{
  window->focusItemSet = g_list_remove(window->focusItemSet, item);
  return ;
}

void zmapWindowShowItem(FooCanvasItem *item)
{
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowItemFeature item_subfeature_data ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							ITEM_FEATURE_TYPE)) ;

  item_subfeature_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;


  printf("\nItem:\n"
	 "Name: %s, type: %s,  style: %s,  x1: %d,  x2: %d,  "
	 "item_x1: %d,  item_x1: %d\n",
	 (char *)g_quark_to_string(feature->original_id),
	 zMapFeatureType2Str(feature->type),
	 zMapStyleGetName(zMapFeatureGetStyle(feature)),
	 feature->x1,
	 feature->x2,
	 item_subfeature_data->start, item_subfeature_data->end) ;



  return ;
}


/* Prints out an items coords in local coords, good for debugging.... */
void zmapWindowPrintLocalCoords(char *msg_prefix, FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("%s:\t%f,%f -> %f,%f\n",
	 (msg_prefix ? msg_prefix : ""),
	 x1, y1, x2, y2) ;


  return ;
}



/* Prints out an items coords in world coords, good for debugging.... */
void zmapWindowPrintItemCoords(FooCanvasItem *item)
{
  double x1, y1, x2, y2 ;

  /* Gets bounding box in parents coord system. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2) ;

  printf("P %f, %f, %f, %f -> ", x1, y1, x2, y2) ;

  foo_canvas_item_i2w(item, &x1, &y1) ;
  foo_canvas_item_i2w(item, &x2, &y2) ;

  printf("W %f, %f, %f, %f\n", x1, y1, x2, y2) ;

  return ;
}


/* Converts given world coords to an items coord system and prints them. */
void zmapWindowPrintW2I(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_w2i(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  world(%f, %f)  ->  item(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;

  return ;
}

/* Converts coords in an items coord system into world coords and prints them. */
/* Prints out item coords position in world and its parents coords.... */
void zmapWindowPrintI2W(FooCanvasItem *item, char *text, double x1_in, double y1_in)
{
  double x1 = x1_in, y1 = y1_in;

  my_foo_canvas_item_i2w(item, &x1, &y1) ;

  if (!text)
    text = "Item" ;

  printf("%s -  item(%f, %f)  ->  world(%f, %f)\n", text, x1_in, y1_in, x1, y1) ;


  return ;
}


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
      
      if (modFeature->type == ZMAPFEATURE_TRANSCRIPT)
	{
	  zMapAssert(origFeature);
	  
	  foo_canvas_item_set(item->parent, "y", top, NULL);
	}
      else
	{
	  foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
	}

      zMapWindowUpdateInfoPanel(window, modFeature, item);      
    }
  return;
}


void zMapWindowMoveSubFeatures(ZMapWindow window, 
			       ZMapFeature originalFeature, 
			       ZMapFeature modifiedFeature,
			       GArray *origArray, GArray *modArray,
			       gboolean isExon)
{
  FooCanvasItem *item = NULL, *intron, *intron_box;
  FooCanvasPoints *points ;
  ZMapSpanStruct origSpan, modSpan;
  ZMapWindowItemFeatureType itemFeatureType;
  int i, offset;
  double top, bottom, left, right, middle;
  double x1, y1, x2, y2, transcriptOrigin, transcriptBottom;
  ZMapWindowItemFeature box_data, intron_data ;


  offset = ((ZMapFeatureBlock)(modifiedFeature->parent->parent))->block_to_sequence.q1;
  transcriptOrigin = modifiedFeature->x1;
  transcriptBottom = modifiedFeature->x2;
  zmapWindowSeq2CanOffset(&transcriptOrigin, &transcriptBottom, offset);
  
  for (i = 0; i < modArray->len; i++)
    {
      /* get the FooCanvasItem using original feature */
      origSpan = g_array_index(origArray, ZMapSpanStruct, i);
      
      item = zmapWindowFToIFindItemChild(window->context_to_item,
					 originalFeature, origSpan.x1, origSpan.x2);
      
      /* coords are relative to start of transcript. */
      modSpan  = g_array_index(modArray, ZMapSpanStruct, i);
      top = (double)modSpan.x1 - transcriptOrigin;
      bottom = (double)modSpan.x2 - transcriptOrigin;

      if (isExon)
	{
	  foo_canvas_item_set(item, "y1", top, "y2", bottom, NULL);
	  
	  box_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);
	  box_data->start = top + transcriptOrigin;
	  box_data->end = bottom + transcriptOrigin;
	}
      else
	{
	  itemFeatureType = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE));

	  if (itemFeatureType == ITEM_FEATURE_BOUNDING_BOX)
	    {
	      intron_box = item;
	      foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
	      box_data = g_object_get_data(G_OBJECT(intron_box), ITEM_SUBFEATURE_DATA);
	      intron = box_data->twin_item;
	      
	      box_data->start = top + transcriptOrigin;
	      box_data->end = bottom + transcriptOrigin;
	    }
	  else
	    {
	      intron = item;
	      intron_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA);
	      intron_box = intron_data->twin_item;
	      foo_canvas_item_get_bounds(intron_box, &x1, &y1, &x2, &y2);
	      
	      intron_data->start = top + transcriptOrigin;
	      intron_data->end = bottom + transcriptOrigin;
	    }

	  points = foo_canvas_points_new(ZMAP_WINDOW_INTRON_POINTS) ;

	  left = (x2 - x1)/2;
	  right = x2;
	  middle = top + (bottom - top + 1)/2;

	  points->coords[0] = left;
	  points->coords[1] = top;
	  points->coords[2] = right;
	  points->coords[3] = middle;
	  points->coords[4] = left;
	  points->coords[5] = bottom;

	  foo_canvas_item_set(intron_box, "y1", top, "y2", bottom, NULL);
	  foo_canvas_item_set(intron, "points", points, NULL);

	  foo_canvas_points_free(points);
	}
    }
  return;
}




/* I'M TRYING THESE TWO FUNCTIONS BECAUSE I DON'T LIKE THE BIT WHERE IT GOES TO THE ITEMS
 * PARENT IMMEDIATELY....WHAT IF THE ITEM IS ALREADY A GROUP ?????? */

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


gboolean zmapWindowItemIsVisible(FooCanvasItem *item)
{
  gboolean visible = FALSE;

  visible = zmapWindowItemIsShown(item);

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

  return visible;
}

gboolean zmapWindowItemIsShown(FooCanvasItem *item)
{
  gboolean visible = FALSE;
  
  zMapAssert(item != NULL);

  g_object_get(G_OBJECT(item), 
               "visible", &visible,
               NULL);

  return visible;
}





/* 
 *                  Internal routines.
 */




/* THIS FUNCTION HAS TOTALLY THE WRONG NAME, IT __SETS__ THE SCROLL REGION.... */
/** \Brief Recalculate the scroll region.
 *
 * If the selected feature is outside the current scroll region, recalculate
 * the region to be the same size but with the selecte feature in the middle.
 */
static void checkScrollRegion(ZMapWindow window, double start, double end)
{
  double x1, y1, x2, y2 ;

  x1 = x2 = 0.0;
  y1 = start;
  y2 = end;
  zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);
  return ;
  /* NOTE THAT THIS ROUTINE NEEDS TO CALL THE VISIBILITY CHANGE CALLBACK IF WE MOVE
   * THE SCROLL REGION TO MAKE SURE THAT ZMAPCONTROL UPDATES ITS SCROLLBARS.... */

  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);  /* world coords */
  if ( start < y1 || end > y2)
    {
      ZMapWindowVisibilityChangeStruct vis_change ;
      int top, bot ;
      double height ;


      height = y2 - y1;

      y1 = start - (height / 2.0) ;

      if (y1 < window->min_coord)
	y1 = window->min_coord ;

      y2 = y1 + height;

      /* this shouldn't happen */
      if (y2 > window->max_coord)
	y2 = window->max_coord ;

      /* This should probably call zmapWindow_set_scroll_region */
      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2);

      /* UGH, I'M NOT SURE I LIKE THE LOOK OF ALL THIS INT CONVERSION STUFF.... */

      /* redraw the scale bar */
      top = (int)y1;                   /* zmapDrawScale expects integer coordinates */
      bot = (int)y2;
      /* gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup)); */

      zmapWindowDrawScaleBar(window, top, bot);

      /* agh, this seems to be here because we move the scroll region...we need a function
       * to do this all....... */
      //      zmapWindowLongItemCrop(window) ;

      /* Call the visibility change callback to notify our caller that our zoom/position has
       * changed. */
      vis_change.zoom_status = zMapWindowGetZoomStatus(window) ;
      vis_change.scrollable_top = y1 ;
      vis_change.scrollable_bot = y2 ;
      (*(window->caller_cbs->visibilityChange))(window, window->app_data, (void *)&vis_change) ;

    }


  return ;
}

/* Do the right thing with groups and items */
static void highlightItem(ZMapWindow window, FooCanvasItem *item)
{
  if (FOO_IS_CANVAS_GROUP(item))
    {
      HighlightStruct highlight = {NULL, FALSE} ;
      FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;

      highlight.window = window ;
      /* highlight.rev_video = rev_video ; */
      
      g_list_foreach(group->item_list, highlightFuncCB, (void *)&highlight) ;
    }
  else
    {
      ZMapWindowItemFeatureType item_feature_type ;

      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;

      if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX
	  || item_feature_type == ITEM_FEATURE_CHILD)
	{
	  ZMapWindowItemFeature item_data ;

	  item_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;

	  if (item_data->twin_item)
	    setItemColourRevVideo(window, item_data->twin_item) ;
	}

      setItemColourRevVideo(window, item);

    }

  return ;
}


/* This is a g_datalist callback function. */
static void highlightFuncCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  Highlight highlight = (Highlight)user_data ;

  setItemColourRevVideo(highlight->window, item) ;

  return ;
}

static void setItemColourOriginal(ZMapWindow window, FooCanvasItem *item)
{
  ZMapFeature feature ;
  GdkColor *fill_colour = NULL;
  ZMapFeatureTypeStyle style ;
  
  /* Retrieve the feature from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  style = zMapFeatureGetStyle(feature) ;
  zMapAssert(style) ;
  
  g_object_get(G_OBJECT(item), 
               "fill_color_gdk", &fill_colour,
               NULL);

  if(fill_colour)
    {
      GdkColor *bg, *fg;
      bg = &(style->background);
      fg = &(style->foreground);
      /* currently we only need to check foreground and background */
      if(fill_colour->red   == (65535 - bg->red) &&
         fill_colour->green == (65535 - bg->green) &&
         fill_colour->blue  == (65535 - bg->blue))
        {
          setItemColourRevVideo(window, item);
        }
      else if(fill_colour->red   == (65535 - fg->red) &&
              fill_colour->green == (65535 - fg->green) &&
              fill_colour->blue  == (65535 - fg->blue))
        {
          setItemColourRevVideo(window, item);
        }
      else if((fill_colour->red   == bg->red &&
               fill_colour->green == bg->green &&
               fill_colour->blue  == bg->blue) ||
              (fill_colour->red   == fg->red &&
               fill_colour->green == fg->green &&
               fill_colour->blue  == fg->blue))
        {
          printf("Original colour, no need to do anything.\n");
        }
      else
        {
          printf("Some kind of issue here colour is neither"
                 " original nor rev video of either foreground"
                 " of background colour.\n");
        }
    }
  
  return ;  
}

static void setItemColourRevVideo(ZMapWindow window, FooCanvasItem *item)
{
  GdkColor *fill_colour = NULL;
  ZMapWindowItemFeatureType item_feature_type ;
  
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  
  if (item_feature_type != ITEM_FEATURE_BOUNDING_BOX)
    {
  
      g_object_get(G_OBJECT(item), 
                   "fill_color_gdk", &fill_colour,
                   NULL);
      /* there is a problem here with rev. video stuff, some features are drawn with
       * background, some not. */
      /* If fill_colour == NULL then it's transparent! This isn't as
       * true as I thought! The pixel has the colour of the item below
       * it! */
      if(fill_colour != NULL)
        {
          fill_colour->red   = (65535 - fill_colour->red) ;
          fill_colour->green = (65535 - fill_colour->green) ;
          fill_colour->blue  = (65535 - fill_colour->blue) ;
          
          foo_canvas_item_set(FOO_CANVAS_ITEM(item),
                              "fill_color_gdk", fill_colour,
                              NULL); 
        }
    }
  return ;
}


