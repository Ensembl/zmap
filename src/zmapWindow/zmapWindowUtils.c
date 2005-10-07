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
 * Last edited: Oct  7 17:56 2005 (edgrif)
 * Created: Thu Jan 20 14:43:12 2005 (edgrif)
 * CVS info:   $Id: zmapWindowUtils.c,v 1.21 2005-10-07 17:18:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>





static void cropLongItem(gpointer data, gpointer user_data) ;
static void freeLongItem(gpointer data, gpointer user_data_unused) ;

static void moveExonsIntrons(ZMapWindow window, 
			     ZMapFeature origFeature,
			     GArray *origArray, 
			     GArray *modArray,
			     int transcriptOrigin,
			     gboolean isExon);



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

ZMapWindowClampType zmapWindowClampStartEnd(ZMapWindow window, double *top_inout, double *bot_inout)
{
  ZMapWindowClampType clamp = ZMAP_WINDOW_CLAMP_INIT;
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


/* Clamps the span within the length of the sequence,
 * possibly shifting the span to keep it the same size. */
ZMapWindowClampType zmapWindowClampSpan(ZMapWindow window, double *top_inout, double *bot_inout)
{
  double top, bot;
  ZMapWindowClampType clamp = ZMAP_WINDOW_CLAMP_INIT;

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

  return clamp;
}


/* Make a rectangle as big as its parent group, good for when you have one as a background
 * to items in a group. */
void zmapWindowMaximiseRectangle(FooCanvasItem *rectangle)
{
  FooCanvasGroup *parent_group ;
  double x1, y1, x2, y2 ;

  zMapAssert(FOO_IS_CANVAS_RE(rectangle)) ;

  parent_group = FOO_CANVAS_GROUP(rectangle->parent) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(parent_group), &x1, &y1, &x2, &y2) ;

  zmapWindowExt2Zero(&x1, &x2) ;
  zmapWindowExt2Zero(&y1, &y2) ;
  foo_canvas_item_set(rectangle,
		      "x1", x1,
		      "y1", y1,
		      "x2", x2,
		      "y2", y2,
		      NULL) ;

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

  zoom = zMapWindowGetZoomMax(window) ;

  length = zmapWindowExt(start, end) * zoom ;

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



void zMapWindowGetVisible(ZMapWindow window, double *top_out, double *bottom_out)
{
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  *top_out = scroll_y1 ;
  *bottom_out = scroll_y2 ;

  return ;
}


/* Either wants SeqCoords (start, end) _or_ (0.0, 0.0) in which case it'll use
 * window->min_coord, window->max_coord
 */
void zmapWindowDrawScaleBar(ZMapWindow window, double start, double end)
{
  double c_start = start;
  double c_end   = end;        /* Canvas start and end */
  ZMapWindowClampType clmp;

  if (window->scaleBarGroup && (FOO_IS_CANVAS_ITEM( (window->scaleBarGroup) )))
    gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));

  /* This isn't very good, but won't be needed when in separate canvas/window/pane */
  if(start <= 0.0 )
    
    //      zmapWindowSeq2CanExt(&c_start, &c_end);
    //else
    {
      c_start = window->min_coord;
      c_end   = window->max_coord;
    }
  /* SHOULD NOT NEED TO BE THIS!!!!!! */
  clmp = zmapWindowClampStartEnd(window, &c_start, &c_end);

  window->scaleBarGroup = zMapDrawScale(window->canvas, 
                                        zMapWindowGetZoomFactor(window),
                                        c_start,
                                        c_end);
  return ;
}











/* 
 *                  Internal routines.
 */

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
