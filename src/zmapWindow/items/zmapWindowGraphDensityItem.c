/*  File: zmapWindowGraphItem.c
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
 * CVS info:   $Id: zmapWindowGlyphItem.c,v 1.14 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>



#include <math.h>
#include <string.h>
#include <ZMap/zmapUtilsLog.h>
#include <zmapWindowBasicFeature.h>
#include <zmapWindowGraphDensityItem_I.h>


static void zmap_window_graph_density_item_class_init  (ZMapWindowGraphDensityItemClass density_class);
static void zmap_window_graph_density_item_init        (ZMapWindowGraphDensityItem      group);
static void zmap_window_graph_density_item_set_property(GObject               *object,
                                       guint                  param_id,
                                       const GValue          *value,
                                       GParamSpec            *pspec);
static void zmap_window_graph_density_item_get_property(GObject               *object,
                                       guint                  param_id,
                                       GValue                *value,
                                       GParamSpec            *pspec);

void  zmap_window_graph_density_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
double  zmap_window_graph_density_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item);
void  zmap_window_graph_density_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
void  zmap_window_graph_density_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose);


static void zmap_window_graph_density_item_destroy     (GObject *object);


static ZMapWindowGraphDensityItemClass density_class_G = NULL;


GType zMapWindowGraphDensityItemGetType(void)
{
  static GType group_type = 0 ;

  if (!group_type)
    {
      static const GTypeInfo group_info = {
      sizeof (zmapWindowGraphDensityItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_graph_density_item_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowGraphDensityItem),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_graph_density_item_init,
      NULL
      };

      group_type = g_type_register_static(foo_canvas_item_get_type(),
                                ZMAP_WINDOW_GRAPH_DENSITY_ITEM_NAME,
                                &group_info,
                                0) ;
    }

  return group_type;
}



/*
 * just in case we wanted to overlay two or more line graphs we need to allow for more than one
 * graph per column, so we specify therse by col_id (which includes strand) and style_id (which specifies colours)
 * (NOTE 2 diff styles could look the same but their id's would be diff)
 */
ZMapWindowCanvasItem zMapWindowGraphDensityItemGetDensityItem(FooCanvasGroup *parent, GQuark id, int start,int end,int width)
{
      ZMapWindowGraphDensityItem di = NULL;
      FooCanvasItem *foo,*interval;

            /* class not intialised till we make an item */
      if(density_class_G && density_class_G->density_items)
            di = (ZMapWindowGraphDensityItem) g_hash_table_lookup( density_class_G->density_items, GUINT_TO_POINTER(id));
      if(di)
      {
            return((ZMapWindowCanvasItem) &(di->__parent__));
      }
      else
      {
            /* need a wrapper to get ZWCI with a foo_density inside it */
            foo = foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_BASIC_FEATURE,
                        "x", 0.0,
                        "y", start,
                        NULL);

            interval = foo_canvas_item_new((FooCanvasGroup *) foo, ZMAP_TYPE_WINDOW_GRAPH_DENSITY,NULL);

            di = (ZMapWindowGraphDensityItem) interval;
            di->id = id;
            g_hash_table_insert(density_class_G->density_items,GUINT_TO_POINTER(id),(gpointer) di);

            /* set our bounding box at the whole column */
            /* direct access is kosher as this is an item implementation and we inherit foo */
            interval->y1 = start;
            interval->y2 = end;
            interval->x1 = 0.0;
            interval->x2 = width;
      }
      return ((ZMapWindowCanvasItem) foo);
}




static void zmap_window_graph_density_item_class_init(ZMapWindowGraphDensityItemClass density_class)
{
  ZMapWindowCanvasItemClass canvas_class ;
  GObjectClass *gobject_class ;
  FooCanvasItemClass *item_class;

  density_class_G = density_class;
  density_class_G->density_items = g_hash_table_new(NULL,NULL);

  gobject_class = (GObjectClass *) density_class;
  canvas_class  = (ZMapWindowCanvasItemClass) density_class;
  item_class = (FooCanvasItemClass *) density_class;

  gobject_class->set_property = zmap_window_graph_density_item_set_property;
  gobject_class->get_property = zmap_window_graph_density_item_get_property;

  gobject_class->dispose = zmap_window_graph_density_item_destroy;

//  canvas_class->add_interval = zmap_window_graph_density_item_add_interval;
  canvas_class->check_data   = NULL;

  item_class->update = zmap_window_graph_density_item_update;
  item_class->bounds = zmap_window_graph_density_item_bounds;
  item_class->point  = zmap_window_graph_density_item_point;
  item_class->draw   = zmap_window_graph_density_item_draw;

  zmapWindowItemStatsInit(&(canvas_class->stats), ZMAP_TYPE_WINDOW_GRAPH_DENSITY) ;

  return ;
}

static void zmap_window_graph_density_item_init (ZMapWindowGraphDensityItem graph)
{

  return ;
}



/* initially this is canvas group and we avoid updating an empty list of casvas items
 * which would set the bounding box to 0
 */
void zmap_window_graph_density_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
}

/* how far are we from the cursor? */
/* can't return foo camvas item for the feature as they are not in the canvas, so return the density item */
double  zmap_window_graph_density_item_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item)
{
      double dist = 0.0;
      /*
       * need to scan internal list and apply close endough rules
       */
      return dist;
#if MH17_NO
foo_canvas_group_point (FooCanvasItem *item, double x, double y, int cx, int cy,
                  FooCanvasItem **actual_item)
{
      FooCanvasGroup *group;
      GList *list;
      FooCanvasItem *child, *point_item;
      int x1, y1, x2, y2;
      double gx, gy;
      double dist, best;
      int has_point;

      group = FOO_CANVAS_GROUP (item);

      x1 = cx - item->canvas->close_enough;
      y1 = cy - item->canvas->close_enough;
      x2 = cx + item->canvas->close_enough;
      y2 = cy + item->canvas->close_enough;

      best = 0.0;
      *actual_item = NULL;

      gx = x - group->xpos;
      gy = y - group->ypos;

      dist = 0.0; /* keep gcc happy */

      for (list = group->item_list; list; list = list->next) {
            child = list->data;

            if ((child->x1 > x2) || (child->y1 > y2) || (child->x2 < x1) || (child->y2 < y1))
                  continue;

            point_item = NULL; /* cater for incomplete item implementations */

            if ((child->object.flags & FOO_CANVAS_ITEM_MAPPED)
                && FOO_CANVAS_ITEM_GET_CLASS (child)->point) {
                  dist = foo_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
                  has_point = TRUE;
            } else
                  has_point = FALSE;
            /* guessing that the x factor is OK here. RNGC */
            if (has_point
                && point_item
                && ((int) (dist * item->canvas->pixels_per_unit_x + 0.5)
                  <= item->canvas->close_enough)) {
                  best = dist;
                  *actual_item = point_item;
            }
      }

      return best;
}
#endif
}

void  zmap_window_graph_density_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
      double minx,miny,maxx,maxy;
      FooCanvasGroup *group = FOO_CANVAS_GROUP (item);

      minx = item->x1;
      miny = item->y1;
      maxx = item->x2;
      maxy = item->y2;

      /* Make the bounds be relative to our parent's coordinate system */
      if (item->parent) {
            minx += group->xpos;
            miny += group->ypos;
            maxx += group->xpos;
            maxy += group->ypos;
      }

      *x1 = minx;
      *y1 = miny;
      *x2 = maxx;
      *y2 = maxy;

}

void  zmap_window_graph_density_item_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
      zMapLogWarning("density draw","");

      /* paint all the data in the exposed area */


#if 0
static void
foo_canvas_rect_draw (FooCanvasItem *item, GdkDrawable *drawable, GdkEventExpose *expose)
{
      FooCanvasRE *re;
      double x1, y1, x2, y2;
      int cx1, cy1, cx2, cy2;
      double i2w_dx, i2w_dy;

      re = FOO_CANVAS_RE (item);

      /* Get canvas pixel coordinates */
      i2w_dx = 0.0;
      i2w_dy = 0.0;
      foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

      x1 = re->x1 + i2w_dx;
      y1 = re->y1 + i2w_dy;
      x2 = re->x2 + i2w_dx;
      y2 = re->y2 + i2w_dy;

      foo_canvas_w2c (item->canvas, x1, y1, &cx1, &cy1);
      foo_canvas_w2c (item->canvas, x2, y2, &cx2, &cy2);

      if (re->fill_set) {
            if ((re->fill_color & 0xff) != 255) {
                  GdkRectangle *rectangles;
                  gint i, n_rectangles;
                  GdkRectangle draw_rect;
                  GdkRectangle part;

                  draw_rect.x = cx1;
                  draw_rect.y = cy1;
                  draw_rect.width = cx2 - cx1 + 1;
                  draw_rect.height = cy2 - cy1 + 1;

                  /* For alpha mode, only render the parts of the region
                     that are actually exposed */
                  gdk_region_get_rectangles (expose->region,
                                       &rectangles,
                                       &n_rectangles);

                  for (i = 0; i < n_rectangles; i++) {
                        if (gdk_rectangle_intersect (&rectangles[i],
                                               &draw_rect,
                                               &part)) {
                              render_rect_alpha (FOO_CANVAS_RECT (item),
                                             drawable,
                                             part.x, part.y,
                                             part.width, part.height,foo_canvas_get_color_pixel
                                             re->fill_color);
                        }
                  }

                  g_free (rectangles);
            } else {
                  if (re->fill_stipple)
                        foo_canvas_set_stipple_origin (item->canvas, re->fill_gc);

                  gdk_draw_rectangle (drawable,
                                  re->fill_gc,
                                  TRUE,
                                  cx1, cy1,
                                  cx2 - cx1 + 1,
                                  cy2 - cy1 + 1);
            }
      }

      if (re->outline_set) {
            if (re->outline_stipple)
                  foo_canvas_set_stipple_origin (item->canvas, re->outline_gc);

            gdk_draw_rectangle (drawable,
                            re->outline_gc,
                            FALSE,
                            cx1,
                            cy1,
                            cx2 - cx1,
                            cy2 - cy1);
      }
}
#endif
}


ZMapWindowCanvasGraphSegment zmapWindowCanvasGraphSegmentAlloc(void)
{
      GList *l;
      ZMapWindowCanvasGraphSegment gs;

      if(!density_class_G->graph_segment_free_list)
      {
            int i;
            gs = g_new(ZMapWindowCanvasGraphSegmentStruct,N_GS_ALLOC);

            for(i = 0;i < N_GS_ALLOC;i++)
                  zmapWindowCanvasGraphSegmentFree(gs++);
      }
      zMapAssert(density_class_G->graph_segment_free_list);

      l = density_class_G->graph_segment_free_list;
      gs = (ZMapWindowCanvasGraphSegment) l->data;
      density_class_G->graph_segment_free_list = g_list_delete_link(density_class_G->graph_segment_free_list,l);

      /* thse can get re-allocated so must zero */
      memset((gpointer) gs,sizeof(ZMapWindowCanvasGraphSegmentStruct),0);
      return(gs);
}

void zmapWindowCanvasGraphSegmentFree(ZMapWindowCanvasGraphSegment gs)
{
      density_class_G->graph_segment_free_list =
            g_list_prepend(density_class_G->graph_segment_free_list, (gpointer) gs);
}




static void zmap_window_graph_density_item_set_property(GObject               *object,
                                       guint                  param_id,
                                       const GValue          *value,
                                       GParamSpec            *pspec)

{
  ZMapWindowCanvasItem canvas_item;
  ZMapWindowGraphDensityItem density;

  g_return_if_fail(ZMAP_IS_WINDOW_GRAPH_DENSITY_ITEM(object));

  density = ZMAP_WINDOW_GRAPH_DENSITY_ITEM(object);
  canvas_item = ZMAP_CANVAS_ITEM(object);

  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}


static guint32 gdk_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
        (color->green & 0xff00) << 8 |
        (color->blue & 0xff00)       |
        0xff);

  return rgba;
}

/* no return, as all this data in internal to the column density item
 * bins get squashed according to zoom
 */
void zMapWindowGraphDensityAddItem(ZMapWindowCanvasItem canvas_item, ZMapFeature feature, double y1, double y2, double x1, double x2)
{
  ZMapWindowCanvasGraphSegment gs;
  ZMapFeatureTypeStyle style = feature->style;
  ZMapFrame frame;
  ZMapStrand strand;
  ZMapWindowGraphDensityItem density_item;
  GdkColor *fill = NULL,*draw = NULL, *outline = NULL;
  FooCanvasItem *foo;

  zMapAssert(style);

  gs = zmapWindowCanvasGraphSegmentAlloc();
  gs->feature = feature;
  gs->x1 = x1;
  gs->y1 = y1;
  gs->x2 = x2;
  gs->y2 = y2;
  gs->line = FALSE;

  gs->width = 1;
  gs->width_pixels = TRUE;

  frame = zMapFeatureFrame(canvas_item->feature);
  strand = canvas_item->feature->strand;

  zmapWindowCanvasItemGetColours(style, strand, frame, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill, &draw, &outline, NULL, NULL);

  density_item = (ZMapWindowGraphDensityItem) canvas_item;
  foo = (FooCanvasItem *) canvas_item;
  zMapAssert(foo->canvas);

  if(fill)
  {
      gs->fill_set = TRUE;
      gs->fill_colour = gdk_color_to_rgba(fill);
      gs->fill_pixel = foo_canvas_get_color_pixel(foo->canvas, gs->fill_colour);
  }
  if(outline)
  {
      gs->outline_set = TRUE;
      gs->outline_colour = gdk_color_to_rgba(outline);
      gs->outline_pixel = foo_canvas_get_color_pixel(foo->canvas, gs->outline_colour);
  }


  /* even if they come in order we still have to sort them to be sure so just add to the front */
  density_item->source_bins = g_list_prepend(density_item->source_bins,gs);
}


static void zmap_window_graph_density_item_get_property(GObject               *object,
                                       guint                  param_id,
                                       GValue                *value,
                                       GParamSpec            *pspec)
{
  return ;
}


static void zmap_window_graph_density_item_destroy     (GObject *object)
{

  ZMapWindowGraphDensityItem density;

  g_return_if_fail(ZMAP_IS_WINDOW_GRAPH_DENSITY_ITEM(object));

  density = ZMAP_WINDOW_GRAPH_DENSITY_ITEM(object);
  g_hash_table_remove(density_class_G->density_items,GUINT_TO_POINTER(density->id));

  return ;
}



