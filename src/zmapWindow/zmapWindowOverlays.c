/*  File: zmapWindowOverlays.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun  7 15:07 2007 (rds)
 * Created: Mon Mar 12 12:28:18 2007 (rds)
 * CVS info:   $Id: zmapWindowOverlays.c,v 1.2 2007-06-07 14:10:19 rds Exp $
 *-------------------------------------------------------------------
 */

#include <gdk/gdk.h>
#include <string.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <ZMap/zmapUtils.h>

typedef struct _ZMapWindowOverlayStruct
{
  ZMapMagic       *magic;
  double           parent_item_points[4];
  double           parent_world_points[4];
  FooCanvasItem   *subject;       /* can be NULL */
  FooCanvasGroup  *masks_parent;
  FooCanvasPoints *points;
  GdkBitmap       *stipple;
  GdkColor         stipple_colour;
  GdkFunction      gc_function;
  ZMapWindowOverlaySizeRequestCB request_cb;
  gpointer                       request_data;
}ZMapWindowOverlayStruct;

#define empty_bitmap_width 16
#define empty_bitmap_height 4
static char empty_bitmap_bits[] =
  {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
  } ;

#define overlay_bitmap_width 16
#define overlay_bitmap_height 4
static char overlay_bitmap_bits[] =
  {
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF,
    0xFF, 0xFF
  } ;

static void updateBounds(ZMapWindowOverlay overlay);
static void printOverlay(ZMapWindowOverlay overlay);
static void printOverlayPoints(ZMapWindowOverlay overlay);


ZMAP_DEFINE_NEW_MAGIC(overlay_magic_G);

static gboolean overlay_debug_G = FALSE;

ZMapWindowOverlay zmapWindowOverlayCreate(FooCanvasItem *parent_container,
                                          FooCanvasItem *subject)
{
  ZMapWindowOverlay overlay;

  if(!(overlay = g_new0(ZMapWindowOverlayStruct, 1)))
    zMapAssertNotReached();
  else
    {
      overlay->magic = &overlay_magic_G;

      overlay->gc_function = GDK_COPY;
      
      if((overlay->masks_parent = zmapWindowContainerGetUnderlays(FOO_CANVAS_GROUP(parent_container))))
        updateBounds(overlay);

      if(subject && FOO_IS_CANVAS_ITEM(subject))
        {
          overlay->subject = subject;
        }

      if(overlay_debug_G)
        {
          zmapWindowOverlaySetGdkBitmap(overlay, gdk_bitmap_create_from_data(NULL, &overlay_bitmap_bits[0],
                                                                             overlay_bitmap_width, overlay_bitmap_height));
          zmapWindowOverlaySetGdkColor(overlay, "blue");

          printOverlay(overlay);
        }
      else
          zmapWindowOverlaySetGdkBitmap(overlay, gdk_bitmap_create_from_data(NULL, &empty_bitmap_bits[0],
                                                                             empty_bitmap_width, empty_bitmap_height));

    }

  return overlay;
}

void zmapWindowOverlayUpdate(ZMapWindowOverlay overlay)
{

  updateBounds(overlay);

  return ;
}

void zmapWindowOverlaySetSubject(ZMapWindowOverlay overlay, FooCanvasItem *subject)
{

  if(FOO_IS_CANVAS_ITEM(subject))
    overlay->subject = subject;

  return ;
}

void zmapWindowOverlaySetSizeRequestor(ZMapWindowOverlay overlay, 
                                       ZMapWindowOverlaySizeRequestCB request_cb,
                                       gpointer user_data)
{
  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  overlay->subject = NULL;      /* Not sure we should be doing this!! */

  overlay->request_cb   = request_cb;
  overlay->request_data = user_data;

  if(overlay_debug_G)
    printOverlay(overlay);

  return ;
}

void zmapWindowOverlaySetGdkBitmap(ZMapWindowOverlay overlay, GdkBitmap *bitmap)
{
  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  overlay->stipple = bitmap;

  return ;
}

void zmapWindowOverlaySetGdkColor(ZMapWindowOverlay overlay, char *colour)
{
  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  /* freeing here? */
  gdk_color_parse(colour, &(overlay->stipple_colour));

  return ;
}

void zmapWindowOverlaySetGdkColorFromGdkColor(ZMapWindowOverlay overlay, GdkColor *input)
{
  GdkColor *color, highlight;
  gboolean monkeying = FALSE;

  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);
  
  color = gdk_color_copy(input);

  if(monkeying)
    {
      gdk_color_parse("blue", &highlight);

      color->red   = highlight.red   & color->red;
      color->green = highlight.green & color->green;
      color->blue  = highlight.blue  & color->blue;
    }

  overlay->stipple_colour = *color;
  
  return ;
}


void zmapWindowOverlayMask(ZMapWindowOverlay overlay)
{
  FooCanvasPoints *points = NULL;
  int i;

  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  if(overlay->points)
    {
      foo_canvas_points_free(overlay->points);
      overlay->points = NULL;
    }

  if(overlay->subject && (overlay->request_cb)(&points, overlay->subject, overlay->request_data))
    {
      zMapAssert(points);

      /* clip the points to be within the parent */
      /* x coords */
      for(i = 0; i < points->num_points * 2; i+=2)
        {
          if(points->coords[i] < overlay->parent_item_points[0])
            {
              printf("x clip %f to %f\n", points->coords[i], overlay->parent_item_points[0]);
              points->coords[i] = overlay->parent_item_points[0];
            }
          if(points->coords[i] > overlay->parent_item_points[2])
            {
              printf("x clip %f to %f\n", points->coords[i], overlay->parent_item_points[2]);
              points->coords[i] = overlay->parent_item_points[2];
            }
        }
      /* y coords */
      for(i = 1; i < points->num_points * 2; i+=2)
        {
          if(points->coords[i] < overlay->parent_item_points[1])
            {
              printf("y clip %f to %f\n", points->coords[i], overlay->parent_item_points[1]);
              points->coords[i] = overlay->parent_item_points[1];
            }
          if(points->coords[i] > overlay->parent_item_points[3])
            {
              printf("y clip %f to %f\n", points->coords[i], overlay->parent_item_points[3]);
              points->coords[i] = overlay->parent_item_points[3];          
            }
        }

      overlay->points = points;

    }

  if(overlay->points)
    {
      FooCanvasItem *mask;
      mask = foo_canvas_item_new(overlay->masks_parent,
                                 foo_canvas_polygon_get_type(),
                                 NULL);

      if(overlay_debug_G)
        printOverlayPoints(overlay);

      foo_canvas_item_set(FOO_CANVAS_ITEM(mask),
                          "points",         overlay->points,
                          "fill_stipple",   overlay->stipple,
                          "fill_color_gdk", &(overlay->stipple_colour),
                          "outline_color_gdk", &(overlay->stipple_colour),
                          "width_pixels", 1,
                          NULL);
      gdk_gc_set_function(FOO_CANVAS_POLYGON(mask)->fill_gc, overlay->gc_function);
    }


  return ;
}

void destroy_mask(gpointer item_data, gpointer unused_data)
{
  gtk_object_destroy(GTK_OBJECT(item_data));

  return ;
}

void zmapWindowOverlayUnmaskAll(ZMapWindowOverlay overlay)
{
  if(overlay->masks_parent)
    {
      g_list_foreach(overlay->masks_parent->item_list, destroy_mask, NULL);
      overlay->masks_parent->item_list = overlay->masks_parent->item_list_end = NULL;
    }

  return;
}

void zmapWindowOverlayUnmask(ZMapWindowOverlay overlay)
{
  static FooCanvasPoints  *points = NULL;

  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  if(points == NULL)
    points = foo_canvas_points_new(4);

  points->coords[0] = 
    points->coords[1] = 
    points->coords[2] = 
    points->coords[3] = 
    points->coords[4] = 
    points->coords[5] = 
    points->coords[6] = 
    points->coords[7] = 0.0;
    
  foo_canvas_item_set(FOO_CANVAS_ITEM(overlay->masks_parent),
                      "points", points,
                      NULL);

#ifdef RDS_BREAKING_STUFF
  static GdkBitmap *empty_overlay = NULL;

  if(empty_overlay == NULL)
    empty_overlay = gdk_bitmap_create_from_data(NULL, &empty_bitmap_bits[0],
                                                empty_bitmap_width, empty_bitmap_height) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(overlay->mask),
                      "fill_stipple", empty_overlay,
                      NULL);
#endif

  return ;
}

ZMapWindowOverlay zmapWindowOverlayDestroy(ZMapWindowOverlay overlay)
{
  zMapAssert(overlay);
  ZMAP_ASSERT_MAGICAL(overlay->magic, overlay_magic_G);

  g_object_unref(G_OBJECT(overlay->stipple));

  //gtk_object_destroy(GTK_OBJECT(overlay->mask));

  memset(overlay, 0, sizeof(ZMapWindowOverlayStruct));

  g_free(overlay);

  return overlay;
}

/* INTERNALS */

static void updateBounds(ZMapWindowOverlay overlay)
{
  FooCanvasItem *parent_container;

  if((parent_container = FOO_CANVAS_ITEM(zmapWindowContainerGetParent(FOO_CANVAS_ITEM(overlay->masks_parent)))))
    {
      foo_canvas_item_get_bounds(parent_container, 
                                 &(overlay->parent_world_points[0]),
                                 &(overlay->parent_world_points[1]),
                                 &(overlay->parent_world_points[2]),
                                 &(overlay->parent_world_points[3]));

      foo_canvas_item_i2w(parent_container, &(overlay->parent_world_points[0]), &(overlay->parent_world_points[1]));
      foo_canvas_item_i2w(parent_container, &(overlay->parent_world_points[2]), &(overlay->parent_world_points[3]));

      if(overlay->masks_parent)
        {
          memcpy(&(overlay->parent_item_points[0]), &(overlay->parent_world_points[0]), sizeof(double) * 4);
          
          foo_canvas_item_w2i(FOO_CANVAS_ITEM(overlay->masks_parent), &(overlay->parent_item_points[0]), &(overlay->parent_item_points[1]));
          foo_canvas_item_w2i(FOO_CANVAS_ITEM(overlay->masks_parent), &(overlay->parent_item_points[2]), &(overlay->parent_item_points[3]));
        }
    }

  return ;
}


static void printOverlay(ZMapWindowOverlay overlay)
{
  printf("Printing Overlay %p with magic %s\n", overlay, *(overlay->magic));

  printf("  wrld %f, %f -> %f, %f\n", 
         overlay->parent_world_points[0],
         overlay->parent_world_points[1],
         overlay->parent_world_points[2],
         overlay->parent_world_points[3]);

  printf("  item %f, %f -> %f, %f\n", 
         overlay->parent_item_points[0],
         overlay->parent_item_points[1],
         overlay->parent_item_points[2],
         overlay->parent_item_points[3]);

  printOverlayPoints(overlay);

  return ;
}

static void printOverlayPoints(ZMapWindowOverlay overlay)
{
  int i;

  printf("  points : ");

  if(overlay->points)
    {
      for (i = 0; i < overlay->points->num_points * 2; i+=2)
        printf("%f, %f ", overlay->points->coords[i], overlay->points->coords[i+1]);
    }
  else
    printf(" is NULL!");

  printf("\n");

  return ;
}
