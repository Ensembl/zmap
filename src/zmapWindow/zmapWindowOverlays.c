/*  File: zmapWindowOverlays.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 19 13:47 2009 (rds)
 * Created: Mon Mar 12 12:28:18 2007 (rds)
 * CVS info:   $Id: zmapWindowOverlays.c,v 1.13 2010-03-04 15:13:16 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <gdk/gdk.h>
#include <string.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <ZMap/zmapUtils.h>

#define OVERLAY_DEFAULT_ITEM_TYPE_MASK (1 | 2)//(ITEM_FEATURE_SIMPLE | ITEM_FEATURE_CHILD)

#define OVERLAY_DEFAULT_SUB_TYPE_MASK (ZMAPFEATURE_SUBPART_EXON_CDS | ZMAPFEATURE_SUBPART_EXON | ZMAPFEATURE_SUBPART_MATCH)

typedef struct _ZMapWindowOverlayStruct
{
  ZMapMagic       magic;
  double           parent_item_points[4];
  double           parent_world_points[4];
  FooCanvasItem   *subject;       /* can be NULL */
  FooCanvasItem   *alternative_limit;
  FooCanvasGroup  *masks_parent;
  FooCanvasPoints *points;
  GdkBitmap       *stipple;
  GdkColor         stipple_colour;
  GdkFunction      gc_function;
  unsigned int     stipple_colour_set   : 1 ;
  unsigned int     border_colour_set    : 1 ;
  unsigned int     stipple_colour_final : 1 ;
  int              item_type_mask;
  int              sub_type_mask;	/* The ItemFeatureSubpartTypes we want highlighting (as a mask) */
  ZMapWindowOverlaySizeRequestCB request_cb;
  gpointer                       request_data;
}ZMapWindowOverlayStruct;

#ifdef USE_EMPTY_BITMAP
#define empty_bitmap_width 16
#define empty_bitmap_height 4
static char empty_bitmap_bits[] =
  {
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00
  } ;
#endif

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


ZMAP_MAGIC_NEW(overlay_magic_G, ZMapWindowOverlayStruct) ;
static gboolean overlay_debug_G = FALSE;


ZMapWindowOverlay zmapWindowOverlayCreate(FooCanvasItem *parent_container,
                                          FooCanvasItem *subject)
{
  ZMapWindowOverlay overlay;

  if(!(overlay = g_new0(ZMapWindowOverlayStruct, 1)))
    zMapAssertNotReached();
  else
    {
      overlay->magic = overlay_magic_G;

      overlay->gc_function = GDK_COPY;
      
      if((overlay->masks_parent = (FooCanvasGroup *)zmapWindowContainerGetUnderlay((ZMapWindowContainerGroup)(parent_container))))
        updateBounds(overlay);

      if(subject && FOO_IS_CANVAS_ITEM(subject))
        {
          overlay->subject = subject;
        }

      zmapWindowOverlaySetGdkBitmap(overlay, gdk_bitmap_create_from_data(NULL, &overlay_bitmap_bits[0],
                                                                         overlay_bitmap_width, overlay_bitmap_height));
      if(overlay_debug_G)
        printOverlay(overlay);

      overlay->alternative_limit = NULL;

      zmapWindowOverlaySetItemTypeMask(overlay, OVERLAY_DEFAULT_ITEM_TYPE_MASK);
      zmapWindowOverlaySetSubTypeMask(overlay, OVERLAY_DEFAULT_SUB_TYPE_MASK);
    }

  return overlay;
}

void zmapWindowOverlaySetLimitItem(ZMapWindowOverlay overlay, FooCanvasItem *limit_item)
{
  overlay->alternative_limit = limit_item;

  updateBounds(overlay);

  return ;
}

FooCanvasItem *zmapWindowOverlayLimitItem(ZMapWindowOverlay overlay)
{
  ZMAP_MAGIC_ASSERT(overlay_magic_G, overlay->magic) ;

  return overlay->alternative_limit;
}

void zmapWindowOverlayUpdate(ZMapWindowOverlay overlay)
{
  ZMAP_MAGIC_ASSERT(overlay_magic_G, overlay->magic) ;

  updateBounds(overlay);

  return ;
}

void zmapWindowOverlaySetSubject(ZMapWindowOverlay overlay, FooCanvasItem *subject)
{

  if(FOO_IS_CANVAS_ITEM(subject))
    overlay->subject = subject;

  if(overlay->alternative_limit)
    updateBounds(overlay);

  return ;
}

void zmapWindowOverlaySetSizeRequestor(ZMapWindowOverlay overlay, 
                                       ZMapWindowOverlaySizeRequestCB request_cb,
                                       gpointer user_data)
{
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  overlay->subject = NULL;      /* Not sure we should be doing this!! */

  overlay->request_cb   = request_cb;
  overlay->request_data = user_data;

  if(overlay_debug_G)
    printOverlay(overlay);

  return ;
}

void zmapWindowOverlaySetColourReadOnly(ZMapWindowOverlay overlay)
{
  overlay->stipple_colour_final = TRUE;
  return ;
}

void zmapWindowOverlaySetColourReadWrite(ZMapWindowOverlay overlay)
{
  overlay->stipple_colour_final = FALSE;
  return ;
}

void zmapWindowOverlaySetItemTypeMask(ZMapWindowOverlay overlay, int item_type_mask)
{
  overlay->item_type_mask = item_type_mask;
  return ;
}

void zmapWindowOverlaySetSubTypeMask(ZMapWindowOverlay overlay, int sub_type_mask)
{
  overlay->sub_type_mask = sub_type_mask;
  return ;
}

int zmapWindowOverlayGetItemTypeMask(ZMapWindowOverlay overlay)
{
  return overlay->item_type_mask;
}

int zmapWindowOverlayGetSubTypeMask(ZMapWindowOverlay overlay)
{
  return overlay->sub_type_mask;
}

void zmapWindowOverlaySetGdkBitmap(ZMapWindowOverlay overlay, GdkBitmap *bitmap)
{
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  overlay->stipple = bitmap;

  return ;
}

void zmapWindowOverlaySetGdkColor(ZMapWindowOverlay overlay, char *colour)
{
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  if(!overlay->stipple_colour_final)
    {
      gdk_color_parse(colour, &(overlay->stipple_colour));
      overlay->border_colour_set = overlay->stipple_colour_set = TRUE;
    }

  return ;
}

void zmapWindowOverlaySetGdkColorFromGdkColor(ZMapWindowOverlay overlay, GdkColor *input)
{
  GdkColor *current;

  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

#ifdef ONLY_SINCE_2_12  
  char *color_spec = NULL;

  if((color_spec = gdk_color_to_string(input)))
    {
      zmapWindowOverlaySetGdkColor(overlay, color_spec);
      g_free(color_spec);
    }
  else
    zMapLogCritical("%s", "Failed to get colour string from input");
#endif

  if(!overlay->stipple_colour_final)
    {
      current = &(overlay->stipple_colour);
      
      current->pixel = input->pixel;
      current->red   = input->red;
      current->green = input->green;
      current->blue  = input->blue;

      overlay->border_colour_set = overlay->stipple_colour_set = TRUE;
    }

  return ;
}

void zmapWindowOverlayMask(ZMapWindowOverlay overlay)
{
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  zmapWindowOverlayMaskFull(overlay, overlay->request_cb, overlay->request_data);

  return ;
}

void zmapWindowOverlayMaskFull(ZMapWindowOverlay              overlay,
                               ZMapWindowOverlaySizeRequestCB request_cb,
                               gpointer                       request_data)
{
  FooCanvasPoints *points = NULL;
  int i;

  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  if (overlay->points)
    {
      foo_canvas_points_free(overlay->points);
      overlay->points = NULL;
    }

  if (overlay->subject && request_cb && (request_cb)(&points, overlay->subject, request_data))
    {
      double xrange, yrange;
      gboolean xadd = FALSE, yadd = FALSE;
      zMapAssert(points);

      xrange = overlay->parent_item_points[2] - overlay->parent_item_points[0] + 1.0;
      yrange = overlay->parent_item_points[3] - overlay->parent_item_points[1] + 1.0;

      /* clip the points to be within the parent */
      /* x coords */
      for(i = 0; i < points->num_points * 2; i+=2)
        {
          /* This bit of code will probably come back and bite me, but
           * here's why it's written like this and is here.  */

          /* The request_cb above should really do it, as it should
           * know the position of the text item it wants to
           * highlight. However in the case of 3 frame translation
           * it's holding onto the wrong text_item, as the itemfactory
           * just blindly adds... */
          if(xadd || (xadd = (gboolean)((points->coords[i] < overlay->parent_item_points[0]) && 
                                        (points->coords[i] < xrange))))
            points->coords[i] += overlay->parent_item_points[0];

          /* clamp to size of the parent_item... */
          if(points->coords[i] < overlay->parent_item_points[0])
            points->coords[i] = overlay->parent_item_points[0];
          if(points->coords[i] > overlay->parent_item_points[2])
            points->coords[i] = overlay->parent_item_points[2];
        }

      /* y coords */
      for(i = 1; i < points->num_points * 2; i+=2)
        {
          yadd = FALSE;         /* silence compiler */
#ifdef REQUEST_CB_KNOWS_MORE_ABOUT_Y_COORDS
          if(yadd || (yadd = (gboolean)(points->coords[i] < yrange)))
            points->coords[i] += overlay->parent_item_points[1];
#endif

          /* clamp to size of the parent_item... */
          if(points->coords[i] < overlay->parent_item_points[1])
            points->coords[i] = overlay->parent_item_points[1];
          if(points->coords[i] > overlay->parent_item_points[3])
            points->coords[i] = overlay->parent_item_points[3];          
        }

      overlay->points = points;

    }

  if (overlay->points)
    {
      FooCanvasItem *mask;
      GdkGC *fill_gc = NULL;
      mask = foo_canvas_item_new(overlay->masks_parent,
                                 foo_canvas_polygon_get_type(),
                                 NULL);

      if(overlay_debug_G)
        printOverlayPoints(overlay);

      foo_canvas_item_set(FOO_CANVAS_ITEM(mask),
                          "points",         overlay->points,
			  "fill_stipple",   overlay->stipple,
                          "fill_color_gdk",    (overlay->stipple_colour_set ? &(overlay->stipple_colour) : NULL),
                          "outline_color_gdk", (overlay->border_colour_set  ? &(overlay->stipple_colour) : NULL),
                          "width_pixels", 1,
                          NULL);

      /* For reasons unknown we sometimes don't have a gc!
       * possibly when the column is hidden...
       */
      if((fill_gc = FOO_CANVAS_POLYGON(mask)->fill_gc))
        gdk_gc_set_function(fill_gc, overlay->gc_function);
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
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  if(overlay->masks_parent)
    {
      g_list_foreach(overlay->masks_parent->item_list, destroy_mask, NULL);
      overlay->masks_parent->item_list = overlay->masks_parent->item_list_end = NULL;
      zmapWindowOverlaySetLimitItem(overlay, NULL); 
      /* So that we don't cache beyond the life of overlay->subject */
      overlay->subject = NULL;
    }

  return;
}

void zmapWindowOverlayUnmask(ZMapWindowOverlay overlay)
{
  static FooCanvasPoints  *points = NULL;

  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

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
  zMapAssert(overlay && ZMAP_MAGIC_IS_VALID(overlay_magic_G, overlay->magic)) ;

  g_object_unref(G_OBJECT(overlay->stipple));

  //gtk_object_destroy(GTK_OBJECT(overlay->mask));

  memset(overlay, 0, sizeof(ZMapWindowOverlayStruct));

  g_free(overlay);

  return overlay;
}

/* INTERNALS */

static void updateBounds(ZMapWindowOverlay overlay)
{
  ZMapWindowContainerGroup container_group;
  FooCanvasItem *item;

  if(overlay->alternative_limit)
    item = overlay->alternative_limit;
  else if((container_group = zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(overlay->masks_parent))))
    item = (FooCanvasItem *)container_group;
  else
    zMapAssertNotReached();

  if(item)
    {
      foo_canvas_item_get_bounds(item, 
                                 &(overlay->parent_world_points[0]),
                                 &(overlay->parent_world_points[1]),
                                 &(overlay->parent_world_points[2]),
                                 &(overlay->parent_world_points[3]));
      
      foo_canvas_item_i2w(item, &(overlay->parent_world_points[0]), &(overlay->parent_world_points[1]));
      foo_canvas_item_i2w(item, &(overlay->parent_world_points[2]), &(overlay->parent_world_points[3]));
      
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
  printf("Printing Overlay %p with magic %s\n", overlay, overlay->magic);

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
