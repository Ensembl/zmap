/*  File: zmapWindowUtils.c
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Utility functions for the zMapWindow code.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jul 18 10:17 2005 (edgrif)
 * Created: Thu Jan 20 14:43:12 2005 (edgrif)
 * CVS info:   $Id: zmapWindowUtils.c,v 1.13 2005-07-18 09:22:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>


/* Used to hold highlight information for the hightlight callback function. */
typedef struct
{
  ZMapWindow window ;
  gboolean rev_video ;
} HighlightStruct, *Highlight ;



static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean rev_video) ;
static void highlightFuncCB(gpointer data, gpointer user_data) ;
static void setItemColour(ZMapWindow window, FooCanvasItem *item, gboolean rev_video) ;

static void checkScrollRegion(ZMapWindow window, double start, double end) ;

static void cropLongItem(gpointer data, gpointer user_data) ;
static void freeLongItem(gpointer data, gpointer user_data_unused) ;



/* A couple of simple coord calculation routines, if these prove too expensive they
 * can be replaced with macros. */

/* This is the basic length calculation, obvious, but the "+ 1" is constantly overlooked. */
double zmapWindowExt(double start, double end)
{
  double extent ;

  zMapAssert(start <= end) ;

  extent = end - start + 1 ;

  return extent ;
}

/* Converts a sequence extent into a canvas extent.
 *
 * Less obvious as it covers the following slightly subtle problem:
 * 
 * sequence coords:           1  2  3  4  5  6  7  8                                         
 *                                                                                           
 * canvas coords:            |__|__|__|__|__|__|__|__|                                       
 *                                                                                           
 *                           |                       |                                       
 *                          1.0                     9.0                                      
 *                                                                                           
 * i.e. when we actually come to draw it we need to go one _past_ the sequence end           
 * coord because our drawing needs to draw in the whole of the last base.                    
 * 
 */
void zmapWindowSeq2CanExt(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout + 1 ;

  return ;
}

/* Converts a start/end pair of coords into a zero based pair of coords.
 *
 * For quite a lot of the canvas group stuff we need to take two coords defining a range
 * in some kind of parent system and convert that range into the same range but starting at zero,
 * e.g.  range  3 -> 6  becomes  0 -> 3  */
void zmapWindowExt2Zero(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  return ;
}


/* Converts a sequence extent into a zero based canvas extent.
 *
 * Combines zmapWindowSeq2CanExt() and zmapWindowExt2Zero(), used in positioning
 * groups/features a lot because in the canvas item coords are relative to their parent group
 * and hence zero-based. */
void zmapWindowSeq2CanExtZero(double *start_inout, double *end_inout)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *end_inout = *end_inout - *start_inout ;		    /* do this first before zeroing start ! */

  *start_inout = 0.0 ;

  *end_inout = *end_inout + 1 ;

  return ;
}


/* NOTE: offset may not be quite what you think, the routine recalculates */
void zmapWindowSeq2CanOffset(double *start_inout, double *end_inout, double offset)
{
  zMapAssert(start_inout && end_inout && *start_inout <= *end_inout) ;

  *start_inout -= offset ;

  *end_inout -= offset ;

  *end_inout += 1 ;

  return ;
}

int zmapWindowClampStartEnd(ZMapWindow window, double *top_inout, double *bot_inout)
{
  int clamp = ZMAP_WINDOW_CLAMP_INIT;
  double top, bot;

  top = *top_inout;
  bot = *bot_inout;

  if (top <= window->min_coord)
    {
      top    = window->min_coord ;
      clamp |= ZMAP_WINDOW_CLAMP_START;
    }

  if (bot >= window->max_coord)
    {
      bot    = window->max_coord ;
      clamp |= ZMAP_WINDOW_CLAMP_END;
    }

  *top_inout = top;
  *bot_inout = bot;

  return clamp;  
}




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


/* Clamps the span within the length of the sequence,
 * possibly shifting the span to keep it the same size. */
int zmapWindowClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  double top, bot;
  int clamp = ZMAP_WINDOW_CLAMP_INIT;

  top = *top_inout;
  bot = *bot_inout;

  if (top < window->min_coord)
    {
      if ((bot = bot + (window->min_coord - top)) > window->max_coord)
        {
          bot    = window->max_coord ;
          clamp |= ZMAP_WINDOW_CLAMP_END;
        }
      clamp |= ZMAP_WINDOW_CLAMP_START;
      top    = window->min_coord ;
    }
  else if (bot > window->max_coord)
    {
      if ((top = top - (bot - window->max_coord)) < window->min_coord)
        {
          clamp |= ZMAP_WINDOW_CLAMP_START;
          top    = window->min_coord ;
        }
      clamp |= ZMAP_WINDOW_CLAMP_END;
      bot    = window->max_coord ;
    }

  *top_inout = top;
  *bot_inout = bot;

  return ;
}


/* The zmapWindowLongItemXXXXX() functions manage the cropping of canvas items that
 * can exceed the X Windows size limit of 32k for graphical items. We have to do this
 * because the foocanvas does not handle this. */

/* n.b. we could get the start/end from the item but perhaps it doesn't matter...and it is
 * more efficient + the user could always get the start/end themselves before calling us. */
void zmapWindowLongItemCheck(ZMapWindow window, FooCanvasItem *item, double start, double end)
{
  double length ;
  double zoom;

  zoom   = zMapWindowGetZoomFactor(window);
  length = zmapWindowExt(start, end) * zoom;

  /* Only add the item if it can exceed the windows limit. */
  if (length > ZMAP_WINDOW_MAX_WINDOW)
    {
      ZMapWindowLongItem long_item ;

      long_item = g_new0(ZMapWindowLongItemStruct, 1) ;

      long_item->item = item ;

      if (FOO_IS_CANVAS_LINE(long_item->item))
	{
	  FooCanvasPoints *item_points ;

	  g_object_get(G_OBJECT(long_item->item),
		       "points", &item_points,
		       NULL) ;

	  long_item->pos.points = foo_canvas_points_new(ZMAP_WINDOW_INTRON_POINTS) ;

	  memcpy(long_item->pos.points, item_points, sizeof(FooCanvasPoints)) ;

	  memcpy(long_item->pos.points->coords,
		 item_points->coords,
		 ((item_points->num_points * 2) * sizeof(double))) ;
	}
      else
	{
	  long_item->pos.box.start = start ;
	  long_item->pos.box.end = end ;
	}

      window->long_items = g_list_append(window->long_items, long_item) ;
    }

  return ;
}

void zmapWindowLongItemCrop(ZMapWindow window)
{
  if (window->long_items)
    g_list_foreach(window->long_items, cropLongItem, window) ;

  return ;
}


/* Free all the long items by freeing individual structs then the list itself. */
void zmapWindowLongItemFree(GList *long_items)
{
  g_list_foreach(long_items, freeLongItem, NULL) ;

  g_list_free(long_items) ;

  return ;
}



/* Highlight a feature, note how this function should just take _any_ feature/item but it doesn't 
 * and so needs redoing....sigh....it should also be called something like focusOnItem() */
void zMapWindowHighlightObject(ZMapWindow window, FooCanvasItem *item)
{                                               
  ZMapWindowItemFeatureType item_feature_type ;

  /* If any other feature is currently in focus, revert it to its std colours */
  if (window->focus_item)
    highlightItem(window, window->focus_item, FALSE) ;

  /* Highlight the new item. */
  highlightItem(window, item, TRUE) ;
 
  /* Make this item the new focus item. */
  window->focus_item = item ;


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window->focus_item),
							"item_feature_type")) ;

  /* Only raise this item to the top if it is the whole feature. */
  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    foo_canvas_item_raise_to_top(window->focus_item) ;
  else if (item_feature_type == ITEM_FEATURE_CHILD)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'D LIKE TO RAISE THE WHOLE GROUP BUT THIS DOESN'T WORK SO WE JUST RAISE THE
       * CHILD ITEM...I DON'T LIKE THE FEEL OF THIS...DOES IT REALLY WORK.... */
      foo_canvas_item_raise_to_top(window->focus_item->parent) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      foo_canvas_item_raise_to_top(window->focus_item) ;
    }

  /* Make sure the canvas gets redisplayed immediately. */
  foo_canvas_update_now(window->canvas) ;


  return ;
}



void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}


/* Finds the feature item in a window corresponding to the supplied feature item..which is
 * usually one from a different window.... */
FooCanvasItem *zMapWindowFindFeatureItemByItem(ZMapWindow window, FooCanvasItem *item)
{
  FooCanvasItem *matching_item = NULL ;
  ZMapFeature feature ;
  ZMapWindowItemFeatureType item_feature_type ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), "item_feature_data");  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							"item_feature_type")) ;

  if (item_feature_type == ITEM_FEATURE_SIMPLE || item_feature_type == ITEM_FEATURE_PARENT)
    {
      matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item, feature) ;
    }
  else
    {
      ZMapWindowItemFeature item_subfeature_data ;

      item_subfeature_data = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
								      "item_subfeature_data") ;

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
  feature = g_object_get_data(G_OBJECT(item), "item_feature_data");  
  zMapAssert(feature) ;

  /* Find the item that matches */
  matching_item = zmapWindowFToIFindFeatureItem(window->context_to_item, feature) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I don't know what happened to this bit of code... */

  if (FOO_IS_CANVAS_GROUP( matching_item))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





  return matching_item ;
}



/* this is going to need align name, block name and much else besides.... */

FooCanvasItem *zMapWindowFindFeatureItemByName(ZMapWindow window, char *style,
					       ZMapFeatureType feature_type, char *feature,
					       ZMapStrand strand, int start, int end,
					       int query_start, int query_end)
{
  FooCanvasItem *item = NULL ;
  char *feature_name ;
  GQuark feature_id ;
  char *style_name ;
  GQuark style_id ;

  zMapAssert(window && style && feature_type && feature && start >= 1 && start <= end) ;

  /* Make a string name and see if the system knows about it, if not then forget it. */
  if ((feature_name = zMapFeatureCreateName(feature_type, feature, strand, start, end,
					    query_start, query_end))
      && (feature_id = g_quark_try_string(feature_name))
      && (style_name = zMapStyleCreateName(style))
      && (style_id = g_quark_try_string(style_name)))
    {

      /* NEEDS TO BE REDONE TO USE NEW HASH FUNCTION...PROBABLY A NEW HASH FUNCTION
       * WILL NEED TO BE WRITTEN.... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      item = zmapWindowFToIFindFeatureItem(window->feature_to_item, style_id, feature_id) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      printf("function needs recoding\n") ;

    }

  return item ;
}


/* Scroll to the specified item.
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, FooCanvasItem *item)
{
  gboolean result = FALSE ;
  int cx, cy, height ;
  double feature_x1 = 0.0, feature_x2 = 0.0 ;
  double x1, y1, x2, y2 ;
  ZMapFeature feature ;
  ZMapWindowSelectStruct select = {NULL} ;

  zMapAssert(window && item) ;

  /* Really we should create some text here as well.... */
  select.item = item ;

  feature = g_object_get_data(G_OBJECT(item), "item_feature_data");  
  zMapAssert(feature) ;					    /* this should never fail. */

  /* Get the features canvas coords (may be very different for align block features... */
  zMapFeature2MasterCoords(feature, &feature_x1, &feature_x2) ;

  /* May need to move scroll region if object is outside it. */
  checkScrollRegion(window, feature_x1, feature_x2) ;

  /* scroll up or down to user's selection. */
  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2); /* world coords */
	  
  if (y1 <= 0.0)    /* we might be dealing with a multi-box item, eg transcript */
    {
      double px1, py1, px2, py2;
	      
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(item->parent),
				 &px1, &py1, &px2, &py2); 
      if (py1 > 0.0)
	y1 = py1;
    }
	  
  /* Note that because we zoom asymmetrically, we only convert the y coord 
   * to canvas coordinates, leaving the x as is.  */
  foo_canvas_w2c(window->canvas, 0.0, y1, &cx, &cy); 

  height = GTK_WIDGET(window->canvas)->allocation.height;
  foo_canvas_scroll_to(window->canvas, (int)x1, cy - height/3);             /* canvas pixels */
	  
  foo_canvas_item_raise_to_top(item);
	  
  /* highlight the item */
  zMapWindowHighlightObject(window, item) ;
	  
  /* Report the selected object to the layer above us. */
  (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;


  result = TRUE ;

  return result ;
}


void zmapWindowShowItem(FooCanvasItem *item)
{
  ZMapFeature feature ;
  ZMapFeatureTypeStyle type ;
  ZMapWindowItemFeatureType item_feature_type ;
  ZMapWindowItemFeature item_subfeature_data ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), "item_feature_data");  
  zMapAssert(feature) ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							"item_feature_type")) ;

  item_subfeature_data = g_object_get_data(G_OBJECT(item), "item_subfeature_data") ;


  printf("\nItem:\n"
	 "Name: %s, type: %s,  style: %s,  x1: %d,  x2: %d,  "
	 "item_x1: %d,  item_x1: %d\n",
	 (char *)g_quark_to_string(feature->original_id),
	 zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
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

  my_foo_canvas_item_i2w(item, &x1, &y1) ;
  my_foo_canvas_item_i2w(item, &x2, &y2) ;

  printf("item coords:\t%f,%f -> %f,%f\n", x1, y1, x2, y2) ;


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


/* Either wants SeqCoords (start, end) _or_ (0.0, 0.0) in which case it'll use
 * window->min_coord, window->max_coord
 */
void zmapWindowDrawScaleBar(ZMapWindow window, double start, double end)
{
  double c_start = start;
  double c_end   = end;        /* Canvas start and end */

  if (FOO_IS_CANVAS_ITEM( (window->scaleBarGroup) ))
    gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));

  /* This isn't very good, but won't be needed when in separate canvas/window/pane */
  if(start > 0.0 && end > start)
      zmapWindowSeq2CanExt(&c_start, &c_end);
  else
    {
      c_start = window->min_coord;
      c_end   = window->max_coord;
    }

  window->scaleBarGroup = zMapDrawScale(window->canvas, 
                                        zMapWindowGetZoomFactor(window),
                                        (int)c_start,
                                        (int)c_end);
  return ;
}


/* I'M TRYING THESE TWO FUNCTIONS BECAUSE I DON'T LIKE THE BIT WHERE IT GOES TO THE ITEMS
 * PARENT IMMEDIATELY....WHAT IF THE ITEM IS ALREADY A GROUP ?????? */

/**
 * foo_canvas_item_w2i:
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
 * foo_canvas_item_i2w:
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
      gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));

      zmapWindowDrawScaleBar(window, top, bot);

      /* agh, this seems to be here because we move the scroll region...we need a function
       * to do this all....... */
      zmapWindowLongItemCrop(window) ;

      /* Call the visibility change callback to notify our caller that our zoom/position has
       * changed. */
      vis_change.zoom_status = zMapWindowGetZoomStatus(window) ;
      vis_change.scrollable_top = y1 ;
      vis_change.scrollable_bot = y2 ;
      (*(window->caller_cbs->visibilityChange))(window, window->app_data, (void *)&vis_change) ;

    }


  return ;
}


static void highlightItem(ZMapWindow window, FooCanvasItem *item, gboolean rev_video)
{
  if (FOO_IS_CANVAS_GROUP(item))
    {
      HighlightStruct highlight = {NULL, FALSE} ;
      FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;

      highlight.window = window ;
      highlight.rev_video = rev_video ;

      g_list_foreach(group->item_list, highlightFuncCB, (void *)&highlight) ;
    }
  else
    {
      ZMapWindowItemFeatureType item_feature_type ;

      item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

      if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX
	  || item_feature_type == ITEM_FEATURE_CHILD)
	{
	  ZMapWindowItemFeature item_data ;

	  item_data = g_object_get_data(G_OBJECT(item), "item_subfeature_data") ;

	  if (item_data->twin_item)
	    setItemColour(window, item_data->twin_item, rev_video) ;
	}

      setItemColour(window, item, rev_video) ;
    }

  return ;
}


/* This is a g_datalist callback function. */
static void highlightFuncCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  Highlight highlight = (Highlight)user_data ;

  setItemColour(highlight->window, item, highlight->rev_video) ;

  return ;
}


static void setItemColour(ZMapWindow window, FooCanvasItem *item, gboolean rev_video)
{
  ZMapWindowItemFeatureType item_feature_type ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
    {
      GdkColor *colour ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

      /* I'm disabling this for now.... */
      if (rev_video)
	colour = &(window->canvas_border) ;
      else
	colour = &(window->canvas_background) ;

      foo_canvas_item_set(FOO_CANVAS_ITEM(item),
			  "outline_color_gdk", colour,
			  NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  else
    {
      ZMapFeature feature ;
      ZMapFeatureTypeStyle style ;
      GdkColor rev_video_colour ;
      GdkColor *fill_colour ;

      /* Retrieve the feature from the canvas item. */
      feature = g_object_get_data(G_OBJECT(item), "item_feature_data") ;
      zMapAssert(feature) ;

      style = zMapFeatureGetStyle(feature) ;
      zMapAssert(style) ;

      /* there is a problem here with rev. video stuff, some features are drawn with
       * background, some not. */

      if (rev_video)
	{
	  /* set foreground GdkColor to be the inverse of current settings */
	  rev_video_colour.red   = (65535 - style->background.red) ;
	  rev_video_colour.green = (65535 - style->background.green) ;
	  rev_video_colour.blue  = (65535 - style->background.blue) ;
	  
	  fill_colour = &rev_video_colour ;
	}
      else
	{
	  fill_colour = &(style->background) ;
	}
      

      foo_canvas_item_set(FOO_CANVAS_ITEM(item),
			  "fill_color_gdk", fill_colour,
			  NULL) ;
    }


  return ;
}


/* A GFunc list callback function, called to free data attached to each list member. */
static void freeLongItem(gpointer data, gpointer user_data_unused)
{
  ZMapWindowLongItem long_item = (ZMapWindowLongItem)data ;

  g_free(long_item) ;

  return ;
}

/* A GFunc list callback function, called to check whether a canvas item needs to be cropped. */
static void cropLongItem(gpointer data, gpointer user_data)
{
  ZMapWindowLongItem long_item = (ZMapWindowLongItem)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
  double start, end, dummy_x ;

  zMapAssert(FOO_IS_CANVAS_ITEM(long_item->item)) ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("\nScroll region: %f -> %f\n", scroll_y1, scroll_y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Reset to original coords because we may be zooming out, you could be more clever
   * about this but is it worth the convoluted code ? */
  if (FOO_IS_CANVAS_LINE(long_item->item))
    {

      foo_canvas_item_set(long_item->item,
			  "points", long_item->pos.points,
			  NULL) ;

      start = long_item->pos.points->coords[1] ;
      end = long_item->pos.points->coords[((long_item->pos.points->num_points * 2) - 1)] ;
    }
  else
    {
      foo_canvas_item_set(long_item->item,
			  "y1", long_item->pos.box.start,
			  "y2", long_item->pos.box.end,
			  NULL) ;

      start = long_item->pos.box.start ;
      end  = long_item->pos.box.end ;
    }


  dummy_x = 0 ;
  my_foo_canvas_item_i2w(long_item->item, &dummy_x, &start) ;
  my_foo_canvas_item_i2w(long_item->item, &dummy_x, &end) ;


  /* Now clip anything that overlaps the boundaries of the scrolled region. */
  if (!(end < scroll_y1) && !(start > scroll_y2)
      && ((start < scroll_y1) || (end > scroll_y2)))
    {
      if (start < scroll_y1)
	start = scroll_y1 ;

      if (end > scroll_y2)
	end = scroll_y2 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("item global/local: %f -> %f  ", start, end) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      my_foo_canvas_item_w2i(long_item->item, &dummy_x, &start) ;
      my_foo_canvas_item_w2i(long_item->item, &dummy_x, &end) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("     %f -> %f\n", start, end) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      if (FOO_IS_CANVAS_LINE(long_item->item))
	{
	  FooCanvasPoints *item_points ;

	  g_object_get(G_OBJECT(long_item->item),
		       "points", &item_points,
		       NULL) ;

	  item_points->coords[1] = start ;
	  item_points->coords[((item_points->num_points * 2) - 1)] = end ;
	  
	  foo_canvas_item_set(long_item->item,
			      "points", item_points,
			      NULL) ;
	}
      else
	{
	  foo_canvas_item_set(long_item->item,
			      "y1", start,
			      "y2", end,
			      NULL) ;
	}
    }

  return ;
}
