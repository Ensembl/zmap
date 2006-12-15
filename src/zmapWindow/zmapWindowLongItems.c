/*  File: zmapWindowLongItems.c
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: FooCanvas does not clip item extents to the maximum
 *              that XWindows (or any other windowing system) can
 *              handle. This means that if you zoom in far enough
 *              the display suddenly goes wierd as the sizes get
 *              too big for XWindows (anything over 32k in length).
 *              
 *              This code attempts to handle this. In fact there are
 *              versions of foocanvas that handle this kind of thing
 *              and perhaps we should swop to them.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Dec 15 11:39 2006 (rds)
 * Created: Thu Sep  7 14:56:34 2006 (edgrif)
 * CVS info:   $Id: zmapWindowLongItems.c,v 1.8 2006-12-15 11:40:03 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



typedef struct windowScrollRegionStruct_
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindow window;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  double x1, x2, y1, y2;
}windowScrollRegionStruct, *windowScrollRegion ;


/* Struct for the long items object, internal to long item code.
 * 
 * There is one long items object per ZMapWindow, it keeps a record of all items that
 * may need to be clipped before they are drawn. We shouldn't need to do this but the
 * foocanvas does not take care of any clipping that might be required by the underlying
 * window system. In the case of XWindows this amounts to clipping anything longer than
 * 32K pixels in size as the XWindows protocol cannot handle anything longer.
 * 
 *  */
typedef struct _ZMapWindowLongItemsStruct
{
  GList *items ;					    /* List of long feature items. */

  windowScrollRegionStruct last_cropped_region;

  double max_zoom ;

  int item_count ;					    /* Used to check our code is
							     * correctly creating/destroying items. */
} ZMapWindowLongItemsStruct ;



/* Represents a single long time that may need to clipped. */
typedef struct
{
  FooCanvasItem *item ;

  union
  {
    struct
    {
      double         start ;
      double         end ;
    } box ;

    FooCanvasPoints *points ;
  } pos ;

  struct
  {
    double y1;
    double y2;
  }extreme;

} LongFeatureItemStruct, *LongFeatureItem ;




static void cropLongItem(gpointer data, gpointer user_data) ;
static void freeLongItem(gpointer data, gpointer user_data_unused) ;
gint findLongItemCB(gconstpointer data, gconstpointer user_data) ;
static void printLongItem(gpointer data, gpointer user_data) ;
static void printLongItemsOperation(ZMapWindowLongItems long_items, 
                                    LongFeatureItem long_item, 
                                    char *operation);


/* Set TRUE for debugging output. */
static gboolean debug_G = FALSE ;



/* Simple constructor for a long item object.
 * Note how keeping the list inside means we don't have to have some calls having a pointer to
 * the list as a parameter. */
ZMapWindowLongItems zmapWindowLongItemCreate(double max_zoom)
{
  ZMapWindowLongItems long_item = NULL ;

  long_item = g_new0(ZMapWindowLongItemsStruct, 1) ;

  long_item->max_zoom = max_zoom ;

  return long_item ;
}


void zmapWindowLongItemSetMaxZoom(ZMapWindowLongItems long_item, double max_zoom)
{
  long_item->max_zoom = max_zoom ;

  return ;
}

void zmapWindowLongItemResized(ZMapWindowLongItems long_items, FooCanvasItem *item)
{
  double length, start, end ;
  LongFeatureItem new_item = NULL;
  gboolean created = FALSE;
  GList *list_long_item = NULL;

  foo_canvas_item_get_bounds(item, NULL, &start, NULL, &end);

  if((list_long_item = g_list_find_custom(long_items->items, item, findLongItemCB)))
    {
      new_item = (LongFeatureItem)(list_long_item->data);
      zMapAssert(new_item->item == item);

      if(FOO_IS_CANVAS_RE(item))
        {
          new_item->pos.box.start = start;
          new_item->pos.box.end   = end;
        }
      new_item->extreme.y1 = start;
      new_item->extreme.y2 = end;

    }
  else
    {
      zmapWindowLongItemCheck(long_items, item, start, end);
    }  

  return ;  
}

/* n.b. we could get the start/end from the item but perhaps it doesn't matter...and it is
 * more efficient + the user could always get the start/end themselves before calling us. */
void zmapWindowLongItemCheck(ZMapWindowLongItems long_items, FooCanvasItem *item, double start, double end)
{
  double length ;
  length = zmapWindowExt(start, end) * long_items->max_zoom ;

  
  /* Only add the item if it can exceed the windows limit. */
  if (length > ZMAP_WINDOW_MAX_WINDOW)
    {
      LongFeatureItem new_item = NULL;
      gboolean created = FALSE;
      GList *list_long_item = NULL;

      if((list_long_item = g_list_find_custom(long_items->items, item, findLongItemCB)))
        {
          new_item = (LongFeatureItem)(list_long_item->data);
          zMapAssert(new_item->item == item);
        }
      else
        {
          new_item = g_new0(LongFeatureItemStruct, 1) ;
          new_item->item = item ;
          created = TRUE;

          if (FOO_IS_CANVAS_LINE(new_item->item) || FOO_IS_CANVAS_POLYGON(new_item->item))
            {
              FooCanvasPoints *item_points ;
              
              g_object_get(G_OBJECT(new_item->item),
                           "points", &item_points,
                           NULL) ;
              if(created)
                {
                  new_item->pos.points = foo_canvas_points_new(item_points->num_points) ;
                  
                  memcpy(new_item->pos.points, item_points, sizeof(FooCanvasPoints)) ;
                  
                }
              else if(new_item->pos.points->num_points != item_points->num_points)
                {
                  foo_canvas_points_free(new_item->pos.points);
                  new_item->pos.points = foo_canvas_points_new(item_points->num_points);
                  memcpy(new_item->pos.points, item_points, sizeof(FooCanvasPoints)) ;
                }
              
              memcpy(new_item->pos.points->coords,
                     item_points->coords,
                     ((item_points->num_points * 2) * sizeof(double))) ;
            }
          else
            {
              new_item->pos.box.start = start ;
              new_item->pos.box.end = end ;
            }
        }

      if(start < new_item->extreme.y1)
        new_item->extreme.y1 = start;
      if(end   > new_item->extreme.y2)
        new_item->extreme.y2 = end;
      
      if(created)
        {
          long_items->items = g_list_append(long_items->items, new_item) ;
          
          long_items->item_count++ ;
        }

      if (debug_G)
        {
          printLongItem(new_item, NULL) ;
          
          printLongItemsOperation(long_items, new_item, created ? "added" : "altered");
        }
        
    }
  return ;
}

void zmapWindowLongItemCrop(ZMapWindowLongItems long_items, 
                            double x1, double y1,
                            double x2, double y2)
{
  if (long_items->items)
    {
      windowScrollRegionStruct func_data = {0.0, 0.0, 0.0, 0.0}, *last_region;

      last_region = &(long_items->last_cropped_region);

      func_data.x1 = x1;
      func_data.x2 = x2;
      func_data.y1 = y1;
      func_data.y2 = y2;

      if(!((last_region->y1 == y1) &&
           (last_region->y2 == y2)))
        {
          if(debug_G)
            printf("Cropping Long Items: Region %f %f %f %f\n", x1, y1, x2, y2);
          
          if(long_items->items)
            {
              LongFeatureItem first_long_item = (LongFeatureItem)(long_items->items->data);
              FooCanvasItem *item = NULL;
              double ppuy = 0.0, pixspan;

              item    = FOO_CANVAS_ITEM(first_long_item->item);
              ppuy    = FOO_CANVAS(item->canvas)->pixels_per_unit_y;
              pixspan = (ppuy * (y2 - y1));

              /* We have a reasonable margin (rounding) here... and 30050 is still way less than 1 << 15 */
              /* If... this is still failing, its almost certainly the navigator extending slightly each time... */
              zMapAssert(ZMAP_WINDOW_MAX_WINDOW && (pixspan <= ZMAP_WINDOW_MAX_WINDOW + 50.0));
            }

          g_list_foreach(long_items->items, cropLongItem, &func_data) ;
      }

      last_region->x1 = x1;
      last_region->x2 = x2;
      last_region->y1 = y1;
      last_region->y2 = y2;
    }

  return ;
}



void zmapWindowLongItemPrint(ZMapWindowLongItems long_items)
{

  if (long_items->items)
    g_list_foreach(long_items->items, printLongItem, NULL) ;

  return ;
}

/* Returns TRUE if the item was removed, FALSE if the item could not be
 * found in the list. */
gboolean zmapWindowLongItemRemove(ZMapWindowLongItems long_items, FooCanvasItem *item)
{
  gboolean result = FALSE ;
  GList *list_item ;

  if ((list_item = g_list_find_custom(long_items->items, item, findLongItemCB)))
    {
      gpointer data = list_item->data ;

      long_items->items = g_list_remove(long_items->items, list_item->data) ;

      freeLongItem(data, NULL) ;

      long_items->item_count-- ;

      result = TRUE ;
      
      if (debug_G)
	{
          LongFeatureItemStruct tmp_long = {NULL};
	  tmp_long.item = item;

          printLongItem(&tmp_long, NULL) ;

          printLongItemsOperation(long_items, &tmp_long, "removed");
	}
    }

  return result ;
}



/* Free all the long items by freeing individual structs then the list itself. */
void zmapWindowLongItemFree(ZMapWindowLongItems long_items)
{

  g_list_foreach(long_items->items, freeLongItem, NULL) ;

  g_list_free(long_items->items) ;

  long_items->items = NULL ;

  long_items->item_count = 0 ;

  return ;
}



/* Destructor a long item object. */
void zmapWindowLongItemDestroy(ZMapWindowLongItems long_item)
{
  zMapAssert(long_item) ;

  if (long_item->items)
    zmapWindowLongItemFree(long_item) ;

  g_free(long_item) ;

  return ;
}








/* 
 *                  Internal routines.
 */



/* A GFunc list callback function, called to free data attached to each list member. */
static void freeLongItem(gpointer data, gpointer user_data_unused)
{
  LongFeatureItem long_item = (LongFeatureItem)data ;

  if(long_item->item)
    {
      if(FOO_IS_CANVAS_LINE(long_item->item) || FOO_IS_CANVAS_POLYGON(long_item->item))
        {
          /* What happens about the points list ??????????????????? */
          if(long_item->pos.points)
            foo_canvas_points_free(long_item->pos.points);
        }
    }
  g_free(long_item) ;

  return ;
}

/* A GFunc list callback function, called to check whether a canvas item needs to be cropped. */
static void cropLongItem(gpointer data, gpointer user_data)
{
  LongFeatureItem long_item = (LongFeatureItem)data ;
  windowScrollRegion func_data = (windowScrollRegion)user_data;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
  double start, end, dummy_x, ppuy ;
  gboolean compare_item_coords = TRUE;

  zMapAssert(FOO_IS_CANVAS_ITEM(long_item->item)) ;

  scroll_x1 = func_data->x1;
  scroll_x2 = func_data->x2;
  scroll_y1 = func_data->y1;
  scroll_y2 = func_data->y2;

  if(debug_G)
    ppuy = long_item->item->canvas->pixels_per_unit_y;

  /* Reset to original coords because we may be zooming out, you could be more clever
   * about this but is it worth the convoluted code ? */
  if(FOO_IS_CANVAS_LINE(long_item->item) || FOO_IS_CANVAS_POLYGON(long_item->item))
    {
      foo_canvas_item_set(long_item->item,
                          "points", long_item->pos.points,
                          NULL);
    }
  else
    {
      foo_canvas_item_set(long_item->item,
                          "y1", long_item->pos.box.start,
                          "y2", long_item->pos.box.end,
                          NULL);
    }

  start = long_item->extreme.y1;
  end   = long_item->extreme.y2;

  dummy_x = 0 ;

  if(compare_item_coords)
    {
      /* Trying this out... Should check if it really is any faster, but
       * I'm guessing 2 calls instead of 4 should be */
      foo_canvas_item_w2i(long_item->item, &dummy_x, &scroll_y1);
      foo_canvas_item_w2i(long_item->item, &dummy_x, &scroll_y2);
    }
  else
    {
      foo_canvas_item_i2w(long_item->item, &dummy_x, &start);
      foo_canvas_item_i2w(long_item->item, &dummy_x, &end);
    }

  /* Now clip anything that overlaps the boundaries of the scrolled region. */
  if (!(end < scroll_y1) && !(start > scroll_y2)
      && ((start < scroll_y1) || (end > scroll_y2)))
    {
      if(debug_G)
        {
          printLongItem(long_item, NULL);
          if (!(g_object_get_data(G_OBJECT(long_item->item), ITEM_FEATURE_DATA)))
            printf("  world: region %f -> %f, item %f -> %f", scroll_y1, scroll_y2, start, end);
        }

      if (start < scroll_y1)
	start = scroll_y1;

      if (end > scroll_y2)
	end = scroll_y2;

      if(debug_G && (!(g_object_get_data(G_OBJECT(long_item->item), ITEM_FEATURE_DATA))))
        printf(" [cropped 2 %f %f, pixelsize %f]\n", start, end, (end - start + 1.0) * ppuy);

      if(!compare_item_coords)
        {
          foo_canvas_item_w2i(long_item->item, &dummy_x, &start);
          foo_canvas_item_w2i(long_item->item, &dummy_x, &end);
        }

      if(FOO_IS_CANVAS_POLYGON(long_item->item) ||
         FOO_IS_CANVAS_LINE(long_item->item))
        {
          FooCanvasPoints *item_points ;
          int i, last_idx;
          double tmpy;
	  g_object_get(G_OBJECT(long_item->item),
		       "points", &item_points,
                       NULL) ;
          last_idx = item_points->num_points * 2;
          for(i = 1; i < last_idx; i+=2) /* ONLY Y COORDS!!! */
            {
              tmpy = item_points->coords[i];
              if(tmpy < start)
                item_points->coords[i] = start;
              if(tmpy > end)
                item_points->coords[i] = end;
            }
	  foo_canvas_item_set(long_item->item,
			      "points", item_points,
			      NULL);
        }
      else if(FOO_IS_CANVAS_RE(long_item->item))
	{
	  foo_canvas_item_set(long_item->item,
			      "y1", start,
			      "y2", end,
			      NULL) ;
	}
      else
        zMapAssertNotReached();

      /* We do this to make sure the next update cycle redraws us. */
      foo_canvas_item_request_redraw(long_item->item);
    }
  else if(debug_G)
    {
      printLongItem(long_item, NULL);
      if(!(g_object_get_data(G_OBJECT(long_item->item), ITEM_FEATURE_DATA)))
        printf("  world: region %f -> %f, item %f -> %f [failed to make the grade]\n", scroll_y1, scroll_y2, start, end);
    }

  return ;
}


/* Compares canvas item supplied via user_data with canvas item held in data of list item
 * and returns 0 if they are the same. */
gint findLongItemCB(gconstpointer data, gconstpointer user_data)
{
  gint result = -1 ;
  FooCanvasItem *list_item = ((LongFeatureItem)data)->item ;
  FooCanvasItem *item = (FooCanvasItem *)user_data ;

  if (list_item == item)
    result = 0 ;

  return result ;
}



/* A GFunc list callback function to print the feature name.... */
static void printLongItem(gpointer data, gpointer user_data)
{
  LongFeatureItem long_item = (LongFeatureItem)data ;
  ZMapFeatureAny any_feature = NULL ;
  FooCanvasGroup *parent = NULL ;


  if ((any_feature = g_object_get_data(G_OBJECT(long_item->item), ITEM_FEATURE_DATA)))
    {
      printf("feature name: %s", g_quark_to_string(any_feature->unique_id)) ;
    }
  else if(zmapWindowContainerIsValid(long_item->item))
    {
      parent = zmapWindowContainerGetParent(long_item->item) ;

      if ((any_feature = g_object_get_data(G_OBJECT(parent), ITEM_FEATURE_DATA)))
	{
	  printf("feature name: %s", g_quark_to_string(any_feature->unique_id)) ;
	}
      else
	printf("ugh, no feature data...BAD....") ;
    }
  else
    printf("Double ugh, no feature data, no container data ... BAD ....");

  if (any_feature && (strstr(g_quark_to_string(any_feature->unique_id), "wublast")))
    {
      parent = zmapWindowContainerGetParent(long_item->item) ;
      parent = zmapWindowContainerGetSuperGroup(parent) ;
      printf(" with super parent: %p", parent) ;
      parent = zmapWindowContainerGetFeatures(parent) ;
      printf(" and parent: %p\n", parent) ;
    }
  else
    printf("\n") ;


  return ;
}

static void printLongItemsOperation(ZMapWindowLongItems long_items, 
                                    LongFeatureItem long_item, 
                                    char *operation)
{
  char *type = NULL;

  if(FOO_IS_CANVAS_LINE(long_item->item))
    type = "line";
  else if(FOO_IS_CANVAS_POLYGON(long_item->item))
    type = "poly";
  else
    type = "other";

  printf("Long Item [%s] %s: %d\tList length: %d\n", 
         type, operation, long_items->item_count, 
         g_list_length(long_items->items));

  return ;
}
