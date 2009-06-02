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
 * Last edited: May 12 20:16 2009 (rds)
 * Created: Thu Sep  7 14:56:34 2006 (edgrif)
 * CVS info:   $Id: zmapWindowLongItems.c,v 1.1 2009-06-02 11:20:24 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapWindowLongItems_P.h>

/* struct to represent the scrolled region, plus a bit of cached canvas info */
typedef struct
{
  double pixels_per_unit_y;
  double item_warning_size;

  double x1, x2, y1, y2;
} WindowScrollRegionStruct, *WindowScrollRegion ;


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
    double y1 ;
    double y2 ;
  } extreme ;
  
} LongFeatureItemStruct, *LongFeatureItem ;


static void LongItemCrop(ZMapWindowLong_Items long_items, 
			 double ppux, double ppuy,
			 double x1,   double y1,
			 double x2,   double y2);

static void crop_long_item(gpointer key, 
                           gpointer value, 
                           gpointer user_data);

static void hash_foreach_print_long_item(gpointer key,
                                         gpointer value,
                                         gpointer user_data);

static void long_item_value_destroy(gpointer user_data);

static void save_long_item(LongFeatureItemStruct *long_item, double start, double end);

/* printing functions */
static void printLongItem(gpointer data, gpointer user_data) ;
static void printLongItemsOperation(ZMapWindowLong_Items long_items, 
                                    LongFeatureItem long_item, 
                                    char *operation);


/* Controlled by... TRUE == debugging*/
static gboolean long_item_debug_G = FALSE ;


/*!
 * \brief Simple constructor for a long item object. 
 *
 * \param               Maximum zoom for the canvas.
 * \return              The new ZMapWindowLong_Items object.
 */
ZMapWindowLong_Items zmapWindowLong_ItemCreate(double max_zoom_x, double max_zoom_y)
{
  ZMapWindowLong_Items long_item = NULL ;

  long_item = g_new0(ZMapWindowLong_ItemsStruct, 1) ;

  long_item->max_zoom_x = max_zoom_x ;
  long_item->max_zoom_y = max_zoom_y ;

  long_item->long_feature_items = g_hash_table_new_full(NULL, NULL, NULL, long_item_value_destroy);

  return long_item ;
}

/* This is dangerous.  We _should_ retest _all_ canvas items again. */
void zmapWindowLong_ItemSetMaxZoom(ZMapWindowLong_Items long_item, 
				  double max_zoom_x, double max_zoom_y)
{

  long_item->max_zoom_x = max_zoom_x ;
  long_item->max_zoom_y = max_zoom_y ;

  zMapLogWarning("%s", "Very probably need to zmapWindowLong_ItemCheck _every_ canvas item again.");

  return ;
}

/*!
 * \brief Used to check a FooCanvasItem * for inclusion as a long
 * item, by whether or not at maximum zoom it'll be bigger than 32767.
 *
 * N.B. We could get the start/end from the item but perhaps it
 * doesn't matter...and it is more efficient + the user could always
 * get the start/end themselves before calling us.
 *
 * \param           ZMapWindowLong_Items object
 * \param           FooCanvasItem * to check.
 * \param           minimum y coord
 * \param           maximum y coord
 * \return          void
 ************************************************* */
void zmapWindowLong_ItemCheck(ZMapWindowLong_Items long_items, FooCanvasItem *item, double start, double end)
{
  double length ;

  length = zmapWindowExt(start, end) * long_items->max_zoom_y ;
  
  /* Only add the item if it can exceed the windows limit. */
  if (length > ZMAP_WINDOW_MAX_WINDOW)
    {
      LongFeatureItem new_item = NULL;
      gpointer key, value;

      if((g_hash_table_lookup_extended(long_items->long_feature_items, item, &key, &value)))
        {
          new_item = (LongFeatureItem)value;

          zMapAssert(new_item->item == key);

	  if(new_item->extreme.y1 != start && new_item->extreme.y2 != end)
	    save_long_item(new_item, start, end);

        }
      else
        {
          new_item = g_new0(LongFeatureItemStruct, 1) ;
          new_item->item = item ;

	  save_long_item(new_item, start, end);

          g_hash_table_insert(long_items->long_feature_items, item, new_item);
          
          long_items->item_count++ ;

        }

      if (long_item_debug_G && !key)
        {
          printLongItem(new_item, NULL) ;
          printLongItemsOperation(long_items, new_item, "added");
        }
        
    }
  return ;
}

/*!
 * If given item is a long item then returns its original start/end coordsUsed to check a FooCanvasItem * for inclusion as a long
 * item, by whether or not at maximum zoom it'll be bigger than 32767.
 *
 * N.B. We could get the start/end from the item but perhaps it
 * doesn't matter...and it is more efficient + the user could always
 * get the start/end themselves before calling us.
 *
 * @param           ZMapWindowLong_Items object
 * @param           FooCanvasItem to check.
 * @param           minimum y coord returned
 * @param           maximum y coord returned
 * @return          TRUE if item found, FALSE otherwise.
 ************************************************* */
gboolean zmapWindowLong_ItemCoords(ZMapWindowLong_Items long_items, FooCanvasItem *item,
				  double *start_out, double *end_out)
{
  gboolean found = FALSE ;
  gpointer key, value;


  if ((g_hash_table_lookup_extended(long_items->long_feature_items, item, &key, &value)))
    {
      LongFeatureItem new_item = (LongFeatureItem)value ;

      *start_out = new_item->extreme.y1 ;
      *end_out = new_item->extreme.y2 ;
      found = TRUE ;
    }


  return found ;
}




/*!
 * \brief crop all long items we know about to the region. Not really
 * designed to be used, but if the expose-event callback is failing
 * then this will force the cropping.
 *
 * \param           ZMapWindowLong_Items object
 * \param           x1 coord
 * \param           y1 coord
 * \param           x2 coord
 * \param           y2 coord
 * \return          void
 *************************************************  */
void zmapWindowLong_ItemCrop(ZMapWindowLong_Items long_items, 
			    double ppux, double ppuy,
                            double x1,   double y1,
                            double x2,   double y2)
{

  LongItemCrop(long_items, ppux, ppuy, x1, y1, x2, y2);

  return ;
}

/*!
 * \brief print the details for the object supplied
 * \param       long item object
 * \return      void
 */
void zmapWindowLong_ItemPrint(ZMapWindowLong_Items long_items)
{

  g_hash_table_foreach(long_items->long_feature_items, hash_foreach_print_long_item, NULL) ;

  return ;
}


/*!
 * \brief remove a FooCanvasItem * from the long items
 * object. i.e. when destroying an item.
 *
 * \param                long item object
 * \param                FooCanvasItem * to remove
 * \return               TRUE if the item was removed, 
 *                       FALSE if the item could not be found.
 ************************************************** */
gboolean zmapWindowLong_ItemRemove(ZMapWindowLong_Items long_items, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  if((result = g_hash_table_remove(long_items->long_feature_items, item)))
    {
      long_items->item_count--;
      if (long_item_debug_G)
        {
          LongFeatureItemStruct tmp_long = {NULL};
          tmp_long.item = item;
          
          printLongItem(&tmp_long, NULL) ;
          
          printLongItemsOperation(long_items, &tmp_long, "removed");
        }
    }

  return result ;
}

/*!
 * \brief Free all of the items we know about.
 * 
 * \param            long item object
 * \return           void
 ************************************************** */
void zmapWindowLong_ItemFree(ZMapWindowLong_Items long_items)
{
  g_hash_table_destroy(long_items->long_feature_items);

  long_items->long_feature_items = g_hash_table_new_full(NULL, NULL, NULL, long_item_value_destroy);

  long_items->item_count = 0 ;

  return ;
}



/*!
 * \brief Destructor for a long item object. 
 *
 * \param            long item object
 * \return           void
 ************************************************** */
void zmapWindowLong_ItemDestroy(ZMapWindowLong_Items long_item)
{
  zMapAssert(long_item) ;

  zmapWindowLong_ItemFree(long_item) ;

  g_hash_table_destroy(long_item->long_feature_items);

  g_free(long_item) ;

  return ;
}


/* 
 *                  Internal routines.
 */

/* Cropping starts from here... We check the region with that of the
 * last region and whether or not extra items were added.  If either
 * are true we need to run through the hash of long items cropping 
 */
static void LongItemCrop(ZMapWindowLong_Items long_items, 
			 double ppux, double ppuy,
			 double x1, double y1,
			 double x2, double y2)
{
  if (long_items->long_feature_items)
    {
      WindowScrollRegionStruct func_data = {0.0};

      func_data.x1 = x1;
      func_data.x2 = x2;
      func_data.y1 = y1;
      func_data.y2 = y2;

      func_data.pixels_per_unit_y = ppuy;
      func_data.item_warning_size = ((ZMAP_WINDOW_MAX_WINDOW + 50.0) / func_data.pixels_per_unit_y);

      g_hash_table_foreach(long_items->long_feature_items, crop_long_item, &func_data);
    }

  return ;
}

/*
 * Actually do the cropping.  A GHFunc()
 * 
 * @param             FooCanvasItem * key of hash.
 * @param             LongFeatureItem to crop
 * @param             The region to crop to 
 * @return            void
 */
static void crop_long_item(gpointer key, gpointer value, gpointer user_data)
{
  FooCanvasItem *canvas_item = FOO_CANVAS_ITEM(key);
  LongFeatureItem long_item = (LongFeatureItem)value;
  WindowScrollRegion region = (WindowScrollRegion)user_data;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2;
  double start, end, dummy_x;
  gboolean compare_item_coords = TRUE;

  zMapAssert(canvas_item == long_item->item);

  scroll_x1 = region->x1;
  scroll_x2 = region->x2;
  scroll_y1 = region->y1;
  scroll_y2 = region->y2;

  /* Reset to original coords because we may be zooming out, you could be more clever
   * about this but is it worth the convoluted code ? */
  if(FOO_IS_CANVAS_LINE(canvas_item) || FOO_IS_CANVAS_POLYGON(canvas_item))
    {
      foo_canvas_item_set(canvas_item,
                          "points", long_item->pos.points,
                          NULL);
    }
  else
    {
      foo_canvas_item_set(canvas_item,
                          "y1", long_item->pos.box.start,
                          "y2", long_item->pos.box.end,
                          NULL);
    }

  start = long_item->extreme.y1;
  end   = long_item->extreme.y2;

  dummy_x = 0;

  if(compare_item_coords)
    {
      /* Trying this out... Should check if it really is any faster, but
       * I'm guessing 2 calls instead of 4 should be */
      foo_canvas_item_w2i(canvas_item, &dummy_x, &scroll_y1);
      foo_canvas_item_w2i(canvas_item, &dummy_x, &scroll_y2);
    }
  else
    {
      foo_canvas_item_i2w(canvas_item, &dummy_x, &start);
      foo_canvas_item_i2w(canvas_item, &dummy_x, &end);
    }

  /* Now clip anything that overlaps the boundaries of the scrolled region. */
  if (!(end < scroll_y1) && !(start > scroll_y2) && ((start < scroll_y1) || (end > scroll_y2)))
    {
      if(long_item_debug_G)
        {
          printLongItem(long_item, NULL);
          if (long_item_debug_G)
            printf("  world: region %f -> %f, item %f -> %f", scroll_y1, scroll_y2, start, end);
        }

      if (start < scroll_y1)
	start = scroll_y1;

      if (end > scroll_y2)
	end = scroll_y2;

      if(long_item_debug_G)
        printf(" [cropped 2 %f %f, pixelsize %f]\n", start, end, (end - start + 1.0) * region->pixels_per_unit_y);

      zMapAssert(end - start < region->item_warning_size);

      if(!compare_item_coords)
        {
          foo_canvas_item_w2i(canvas_item, &dummy_x, &start);
          foo_canvas_item_w2i(canvas_item, &dummy_x, &end);
        }

      if (FOO_IS_CANVAS_POLYGON(canvas_item) || FOO_IS_CANVAS_LINE(canvas_item))
        {
          FooCanvasPoints *item_points ;
          int i, last_idx;
          double tmpy;
	  g_object_get(G_OBJECT(canvas_item),
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
	  foo_canvas_item_set(canvas_item,
			      "points", item_points,
			      NULL);
        }
      else if(FOO_IS_CANVAS_RE(canvas_item))
	{
	  foo_canvas_item_set(canvas_item,
			      "y1", start,
			      "y2", end,
			      NULL) ;
	}
      else
        zMapAssertNotReached();

      /* We do this to make sure the next update cycle redraws us. */
      //foo_canvas_item_request_redraw(canvas_item);
    }
  else if(long_item_debug_G)
    {
      printLongItem(long_item, NULL);
      printf("  world: region %f -> %f, item %f -> %f [failed to make the grade]\n", scroll_y1, scroll_y2, start, end);
    }

  return ;
}

/* 
 * print stuff. A GHFunc()
 *
 */
static void hash_foreach_print_long_item(gpointer key,
                                         gpointer value,
                                         gpointer user_data)
{
  printLongItem(value, NULL);

  return ;
}

/* 
 * Destroy the values of the hash. A GDestroyNotify()
 *
 */
static void long_item_value_destroy(gpointer user_data)
{
  LongFeatureItem long_item = (LongFeatureItem)user_data;
  FooCanvasItem *item = long_item->item;

  if(item)
    {
      if(FOO_IS_CANVAS_LINE(item) || FOO_IS_CANVAS_POLYGON(item))
        {
          if(long_item->pos.points)
            foo_canvas_points_free(long_item->pos.points);
        }
    }

  g_free(long_item) ;
  
  return ;
}

static void save_long_item(LongFeatureItemStruct *long_item, double start, double end)
{
  FooCanvasItem *item;
  zMapAssert(long_item->item);

  item = long_item->item;

  if (FOO_IS_CANVAS_LINE(item) || FOO_IS_CANVAS_POLYGON(item))
    {
      FooCanvasPoints *item_points ;
      double *coords_ptr;
      g_object_get(G_OBJECT(item),
		   "points", &item_points,
		   NULL) ;

      if(item_points != NULL)
	{
	  /* Not quite sure why we memcpy here... The g_object_get, should by convention
	   * have already copied the item_points. */
	  long_item->pos.points = foo_canvas_points_new(item_points->num_points) ;
	  
	  coords_ptr = long_item->pos.points->coords;

	  memcpy(long_item->pos.points, item_points, sizeof(FooCanvasPoints)) ;

	  long_item->pos.points->coords = coords_ptr;

	  memcpy(long_item->pos.points->coords,
		 item_points->coords,
		 ((item_points->num_points * 2) * sizeof(double))) ;

	  foo_canvas_points_free(item_points);
	}
    }
  else if(FOO_IS_CANVAS_RE(item))
    {
      long_item->pos.box.start = start ;
      long_item->pos.box.end   = end ;
    }
  else if(!FOO_IS_CANVAS_ITEM(item))
    zMapAssertNotReached();

  long_item->extreme.y1 = start;
  long_item->extreme.y2 = end;

  return ;
}


/* Some boring print routines for debugging */

static void printLongItem(gpointer data, gpointer user_data)
{
  LongFeatureItem long_item = (LongFeatureItem)data ;
  GString *string = g_string_sized_new(128);
  
  g_string_append_printf(string, "%s: ", "printLongItem");

  zmapWindowItemDebugItemToString(long_item->item, string);

  printf("%s\n", string->str);

  g_string_free(string, TRUE);

  return ;
}

static void printLongItemsOperation(ZMapWindowLong_Items long_items, 
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
         g_hash_table_size(long_items->long_feature_items));

  return ;
}

