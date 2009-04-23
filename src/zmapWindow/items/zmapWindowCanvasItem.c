/*  File: zmapWindowCanvasItem.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Apr 16 15:37 2009 (rds)
 * Created: Wed Dec  3 09:00:20 2008 (rds)
 * CVS info:   $Id: zmapWindowCanvasItem.c,v 1.1 2009-04-23 09:12:46 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowGlyphItem.h>
#include <zmapWindow_P.h>	/* ITEM_FEATURE_DATA, ITEM_FEATURE_TYPE */

enum {
  WINDOW_CANVAS_ITEM_0,		/* zero == invalid property id */
  WINDOW_CANVAS_ITEM_INTERVAL_TYPE,
  WINDOW_CANVAS_ITEM_AUTO_RESIZE_BG,
  WINDOW_CANVAS_ITEM_FEATURE,
  WINDOW_CANVAS_ITEM_USER_HIDDEN,
  WINDOW_CANVAS_ITEM_CODE_HIDDEN,
};

/* Some convenience stuff */
#define GCI_UPDATE_MASK (FOO_CANVAS_UPDATE_REQUESTED | FOO_CANVAS_UPDATE_DEEP)
#define GCI_EPSILON 1e-18


typedef struct 
{
  ZMapWindowCanvasItem parent;
  ZMapFeature          feature;
  ZMapStyleColourType  style_colour_type;
  GdkColor            *default_fill_colour;
  ZMapWindowCanvasItemClass klass;
}EachIntervalDataStruct, *EachIntervalData;


static void zmap_window_canvas_item_class_init  (ZMapWindowCanvasItemClass class);
static void zmap_window_canvas_item_init        (ZMapWindowCanvasItem      group);
static void zmap_window_canvas_item_set_property(GObject               *object, 
						 guint                  param_id,
						 const GValue          *value,
						 GParamSpec            *pspec);
static void zmap_window_canvas_item_get_property(GObject               *object,
						 guint                  param_id,
						 GValue                *value,
						 GParamSpec            *pspec);
static void zmap_window_canvas_item_destroy     (GObject *object);


static void zmap_window_canvas_item_post_create(ZMapWindowCanvasItem canvas_item);
static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem  canvas_item,
					       FooCanvasItem        *interval,
					       ZMapWindowItemFeature unused,
					       ZMapStyleColourType   colour_type,
					       GdkColor             *default_fill);

/* FooCanvasItem interface methods */
static void   zmap_window_canvas_item_update      (FooCanvasItem *item,
						   double           i2w_dx,
						   double           i2w_dy,
						   int              flags);
static void   zmap_window_canvas_item_realize     (FooCanvasItem *item);
static void   zmap_window_canvas_item_unrealize   (FooCanvasItem *item);
static void   zmap_window_canvas_item_map         (FooCanvasItem *item);
static void   zmap_window_canvas_item_unmap       (FooCanvasItem *item);
static void   zmap_window_canvas_item_draw        (FooCanvasItem *item, GdkDrawable *drawable,
						   GdkEventExpose *expose);
static double zmap_window_canvas_item_point       (FooCanvasItem *item, double x, double y,
						   int cx, int cy,
						   FooCanvasItem **actual_item);
static void   zmap_window_canvas_item_translate   (FooCanvasItem *item, double dx, double dy);
static void   zmap_window_canvas_item_bounds      (FooCanvasItem *item, double *x1, double *y1,
						   double *x2, double *y2);
#ifdef WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED
static void window_canvas_item_bounds (FooCanvasItem *item, 
				       double *x1, double *y1, 
				       double *x2, double *y2);
#endif /* WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED */
static void update_background_box_size(ZMapWindowCanvasItem canvas_item,
				       double x1, double y1,
				       double x2, double y2);

static gboolean zmap_window_canvas_item_check_data(ZMapWindowCanvasItem canvas_item, GError **error);

/* A couple of accessories */
#ifdef WINDOW_CANVAS_ITEM_INVOKE_UPDATE_REQ
static void window_canvas_item_invoke_update (FooCanvasItem *item,
					      double i2w_dx,
					      double i2w_dy,
					      int flags);
#endif /* WINDOW_CANVAS_ITEM_INVOKE_UPDATE_REQ */
static double window_canvas_item_invoke_point (FooCanvasItem *item, 
					       double x, double y, 
					       int cx, int cy,
					       FooCanvasItem **actual_item);
static gboolean feature_is_drawable(ZMapFeature          feature_any, 
				    ZMapFeatureTypeStyle feature_style, 
				    ZMapStyleMode       *mode_to_use,
				    GType               *type_to_use);
static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data);


static FooCanvasItemClass *group_parent_class_G;

static GQuark zmap_window_canvas_item_get_domain(void)
{
  GQuark domain;

  domain = g_quark_from_string(ZMAP_WINDOW_CANVAS_ITEM_NAME);

  return domain;
}

/**
 * zmap_window_canvas_item_get_type:
 *
 * Registers the &ZMapWindowCanvasItem class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value:  The type ID of the &ZMapWindowCanvasItem class.
 **/
GType
zMapWindowCanvasItemGetType (void)
{
  static GType group_type = 0;
  
  if (!group_type) {
    static const GTypeInfo group_info = {
      sizeof (zmapWindowCanvasItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_canvas_item_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowCanvasItem),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_canvas_item_init,
      NULL
    };
    
    group_type = g_type_register_static (foo_canvas_group_get_type (),
					 ZMAP_WINDOW_CANVAS_ITEM_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
}

/* needs to be void zMapWindowCanvasItemCheckSize(ZMapWindowCanvasItem canvas_item, ZMapWindowLongItems long_items) */
void zMapWindowCanvasItemCheckSize(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeature feature;
  double x1, x2, y1, y2;

  feature = (ZMapFeature)canvas_item->feature;

  if(feature)
    {
      x1 = y1 = 0.0;
#warning FIX ME
      x2 = 16.0;
      y2 = (double)(feature->x2 - feature->x1 + 1.0);
      
      update_background_box_size(canvas_item, x1, 0.0, x2, 0.0);
    }

  return ;
}

ZMapWindowCanvasItem zMapWindowCanvasItemCreate(FooCanvasGroup      *parent,
						double               feature_start,
						ZMapFeature          feature,
						ZMapFeatureTypeStyle feature_style)
{
  ZMapWindowCanvasItem canvas_item = NULL;
  ZMapStyleMode mode;
  GType sub_type;

  if(feature_is_drawable(feature, feature_style, &mode, &sub_type))
    {
      FooCanvasItem *item;

      item = foo_canvas_item_new(parent, sub_type, 
				 "x", 0.0, 
				 "y", feature_start, 
				 NULL);
      
      if(item && ZMAP_IS_CANVAS_ITEM(item))
	{
	  GObject *object;
	  canvas_item = ZMAP_CANVAS_ITEM(item);
	  canvas_item->feature = feature;
	  canvas_item->style   = feature_style;

	  if(ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)
	    (* ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->post_create)(canvas_item);
	  
	  zMapWindowCanvasItemCheckSize(canvas_item);

	  /* This needs to be removed and replaced by zMapWindowCanvasItemGetFeature() */
	  object = G_OBJECT(item);
	  g_object_set_data(object, ITEM_FEATURE_DATA, feature);
	  g_object_set_data(object, ITEM_FEATURE_TYPE, GUINT_TO_POINTER(ITEM_FEATURE_SIMPLE));
	}
    }

  return canvas_item;
}

ZMapWindowCanvasItem zMapWindowCanvasItemDestroy(ZMapWindowCanvasItem canvas_item)
{
  /* We need to do this rather than g_object_unref as the underlying
   * FooCanvasItem objects are gtk objects... */
  gtk_object_destroy(GTK_OBJECT(canvas_item));

  canvas_item = NULL;

  return canvas_item;
}

ZMapFeature zMapWindowCanvasItemGetFeature(ZMapWindowCanvasItem canvas_item)
{
  ZMapFeature feature = NULL;

  if(canvas_item && ZMAP_IS_CANVAS_ITEM(canvas_item))
    feature = canvas_item->feature;

  return feature;
}

void zMapWindowCanvasItemSetIntervalType(ZMapWindowCanvasItem canvas_item, guint type)
{
  g_return_if_fail(ZMAP_IS_CANVAS_ITEM(canvas_item));

  g_object_set(G_OBJECT(canvas_item), 
	       "interval-type", type,
	       NULL);

  return ;
}

FooCanvasItem *zMapWindowCanvasItemAddInterval(ZMapWindowCanvasItem  canvas_item,
					       ZMapWindowItemFeature sub_feature,
					       double top,  double bottom, 
					       double left, double right)
{
  FooCanvasItem *interval = NULL;
  FooCanvasItem *bg;

  g_return_val_if_fail(ZMAP_IS_CANVAS_ITEM(canvas_item), interval);

  if(ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->add_interval)
    interval = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->add_interval)(canvas_item, sub_feature, 
								       top, bottom, left, right);

  if(interval && ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->set_colour)
    {
      ZMapStyleColourType colour_type = ZMAPSTYLE_COLOURTYPE_NORMAL;
      
      (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->set_colour)(canvas_item, interval, sub_feature,
							    colour_type, NULL);
    }

  if((bg = canvas_item->items[WINDOW_ITEM_BACKGROUND]))
    {
      FooCanvasRE *bg_rect = FOO_CANVAS_RE(bg);
      gboolean use_rect = FALSE;
      double y1, y2;

      if(use_rect)
	{
	  y1 = bg_rect->y1;
	  y2 = bg_rect->y2;
	}
      else
	{
	  g_object_get(G_OBJECT(bg), "y1", &y1, "y2", &y2, NULL);
	}

      if(top < y1)
	y1 = top;
     
      if(bottom > y2)
	y2 = bottom;

      if(y1 < 0.0)
	g_warning("unexpected value [%f] for y1.", y1);

      if(use_rect)
	{
	  bg_rect->y1 = y1;
	  bg_rect->y2 = y2;
	}
      else
	{
	  foo_canvas_item_set(bg, "y1", y1, "y2", y2, NULL);
	}
    }

  return interval;
}


void zMapWindowCanvasItemGetBumpBounds(ZMapWindowCanvasItem canvas_item,
				       double *x1_out, double *y1_out,
				       double *x2_out, double *y2_out)
{
  double x1, x2;

  zmap_window_canvas_item_bounds(FOO_CANVAS_ITEM(canvas_item), &x1, y1_out, &x2, y2_out);

  if(x1_out && x2_out)
    {
      double w;

      w = zMapStyleGetWidth(canvas_item->style);

      if(x1 + w > x2)
	x2 += w;

	
      if(x1_out)
	*x1_out = x1;
      if(x2_out)
	*x2_out = x2;
    }

  return ;
}

gboolean zMapWindowCanvasItemCheckData(ZMapWindowCanvasItem canvas_item, GError **error)
{
  gboolean result = TRUE;

  if(result && !canvas_item->feature)
    {
      *error = g_error_new(zmap_window_canvas_item_get_domain(), 1,
			   "No ZMapFeatureAny attached to item");
      
      result = FALSE;
    }

  if(result && ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->check_data)
    {
      result = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->check_data(canvas_item, error);
    }
  
  return result;
}

void zMapWindowCanvasItemClear(ZMapWindowCanvasItem canvas_item)
{
  if(ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->clear)
    {
      ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->clear(canvas_item);
    }
  
  return ;
}

static void zmap_canvas_item_purge_group(FooCanvasGroup *group)
{
  GList *list;

  g_return_if_fail(FOO_IS_CANVAS_GROUP(group));

  list = group->item_list;
  while(list)
    {
      FooCanvasItem *child;
      child = list->data;
      list = list->next;

      gtk_object_destroy(GTK_OBJECT(child));
    }

  g_list_free(group->item_list);
  group->item_list = group->item_list_end = NULL;

  return ;
}

void zMapWindowCanvasItemClearOverlay(ZMapWindowCanvasItem canvas_item)
{
  FooCanvasGroup *group;
  
  group = FOO_CANVAS_GROUP(canvas_item->items[WINDOW_ITEM_OVERLAY]);

  zmap_canvas_item_purge_group(group);

  return ;
}

void zMapWindowCanvasItemClearUnderlay(ZMapWindowCanvasItem canvas_item)
{
  FooCanvasGroup *group;
  
  group = FOO_CANVAS_GROUP(canvas_item->items[WINDOW_ITEM_UNDERLAY]);

  zmap_canvas_item_purge_group(group);

  return ;
}

FooCanvasItem *zMapWindowCanvasItemGetInterval(ZMapWindowCanvasItem canvas_item,
					       double x, double y)
{
  FooCanvasItem *matching_interval = NULL;
  FooCanvasItem *background;
  double ix1, ix2, iy1, iy2;
  double wx1, wx2, wy1, wy2;

  g_return_val_if_fail(ZMAP_IS_CANVAS_ITEM(canvas_item), matching_interval);

  if((background = canvas_item->items[WINDOW_ITEM_BACKGROUND]))
    {
      foo_canvas_item_get_bounds(background, &ix1, &iy1, &ix2, &iy2);

      wx1 = ix1;
      wy1 = iy1;
      wx2 = ix2;
      wy2 = iy2;

      foo_canvas_item_i2w(background, &wx1, &wy1);
      foo_canvas_item_i2w(background, &wx2, &wy2);

      /* filter by item's world coords */
      if((x >= wx1 && x <= wx2) && (y >= wy1 && y <= wy2))
	{
	  FooCanvasGroup *group;
	  FooCanvasItem *point_item;
	  FooCanvas *canvas;
	  GList *list;
	  double x1, x2, y1, y2;
	  double dist;
	  double gx, gy;
	  int cx, cy;
	  int has_point;

	  group = FOO_CANVAS_GROUP(canvas_item);

	  canvas = background->canvas;

	  gx = x - wx1;
	  gy = y - wy1;

	  foo_canvas_w2c(canvas, x, y, &cx, &cy);

	  x1 = cx - canvas->close_enough;
	  y1 = cy - canvas->close_enough;
	  x2 = cx + canvas->close_enough;
	  y2 = cy + canvas->close_enough;

	  /* Go through each interval (FooCanvasGroup's item list).
	   * Backgrounds/overlays/underlays not in this list! */
	  for (list = group->item_list; list; list = list->next) 
	    {
	      FooCanvasItem *child;
	      
	      child = list->data;

	      /* We actaully want the full width of the item here... lines and score by width columns... */
	      /* filtering only by y coords, effectively... */
	      if ((wx1 > x2) || (child->y1 > y2) || (wx2 < x1) || (child->y2 < y1))
		continue;
	      
	      point_item = NULL; /* cater for incomplete item implementations */

	      /* filter out unmapped items */
	      if ((child->object.flags & FOO_CANVAS_ITEM_MAPPED)
		  && FOO_CANVAS_ITEM_GET_CLASS (child)->point) 
		{
		  /* There's some quirks here to get round for lines and rectangles with no fill... */

		  /* We could just check with y coords of item, but prefer to use the
		   * foo_canvas_item_point() code. This ensures FooCanvasGroups can be
		   * intervals. Useful? */

		  if(FOO_IS_CANVAS_RE(child))
		    {
		      int save_fill_set;
		      save_fill_set = FOO_CANVAS_RE(child)->fill_set;
		      FOO_CANVAS_RE(child)->fill_set = 1;
		      /* foo_canvas_re_point() has a bug/feature where "empty" rectangles
		       * points only include the outline. Temporarily setting fill_set 
		       * subverts this... */
		      dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
		      FOO_CANVAS_RE(child)->fill_set = save_fill_set;
		    }
		  else if(FOO_IS_CANVAS_LINE(child))
		    {
		      dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
		      /* If the is distance is less than the width then it must be this line... */
		      if(dist < ix2 - ix1)
			dist = 0.0;
		    }
		  else if(FOO_IS_CANVAS_TEXT(child))
		    {
		      point_item = child;
		      dist = 0.0; /* text point is broken. the x factor is _not_ good enough. */
		    }
		  else		/* enables groups to be included in the ZMapWindowCanvasItem tree */
		    dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);

		  has_point = TRUE;
		} 
	      else
		has_point = FALSE;
	      
	      /* guessing that the x factor is OK here. RNGC. */
	      /* Well it's ok, but not ideal... RDS */
	      if (has_point && point_item)
		{
		  int small_enough;

		  small_enough = (int) (dist * canvas->pixels_per_unit_x + 0.5);

		  if(small_enough <= canvas->close_enough)
		    matching_interval = point_item;
		}
	    }
	}
    }

  return matching_interval;
}

/* Get at parent... */
ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetObject(FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item = NULL;

  if(ZMAP_IS_CANVAS_ITEM(item))
    canvas_item = ZMAP_CANVAS_ITEM(item);
  else if(FOO_IS_CANVAS_ITEM(item) && item->parent)
    {
      if(ZMAP_IS_CANVAS_ITEM(item->parent))
	canvas_item = ZMAP_CANVAS_ITEM(item->parent);
    }

  return canvas_item;
}


void zMapWindowCanvasItemSetIntervalColours(ZMapWindowCanvasItem canvas_item,
					    ZMapStyleColourType  colour_type,
					    GdkColor            *default_fill_colour)
{
  EachIntervalDataStruct interval_data = {NULL};

  g_return_if_fail(ZMAP_IS_CANVAS_ITEM(canvas_item));

  if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
    foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(canvas_item));
  else
    foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(canvas_item));

  interval_data.parent              = canvas_item;
  interval_data.feature             = zMapWindowCanvasItemGetFeature(canvas_item);
  interval_data.style_colour_type   = colour_type;
  interval_data.default_fill_colour = default_fill_colour;
  interval_data.klass               = ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item);

  g_list_foreach(FOO_CANVAS_GROUP(canvas_item)->item_list, window_canvas_invoke_set_colours, &interval_data);

  return ;
}

void zMapWindowCanvasItemUnmark(ZMapWindowCanvasItem canvas_item)
{
  if(canvas_item->mark_item)
    {
      gtk_object_destroy(GTK_OBJECT(canvas_item->mark_item));
      
      canvas_item->mark_item = NULL;
    }

  return ;
}

void zMapWindowCanvasItemMark(ZMapWindowCanvasItem canvas_item,
			      GdkColor            *colour,
			      GdkBitmap           *bitmap)
{
  double x1, y1, x2, y2;

  if(canvas_item->mark_item)
    zMapWindowCanvasItemUnmark(canvas_item);

  if(canvas_item->items[WINDOW_ITEM_OVERLAY] &&
     canvas_item->items[WINDOW_ITEM_BACKGROUND])
    {
      FooCanvasGroup *parent;
      GdkColor outline;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_item->items[WINDOW_ITEM_BACKGROUND]), 
				 &x1, &y1, &x2, &y2);

      gdk_color_parse("red", &outline);

      parent = FOO_CANVAS_GROUP(canvas_item->items[WINDOW_ITEM_OVERLAY]);

      canvas_item->mark_item = foo_canvas_item_new(parent,
						   foo_canvas_rect_get_type(),
						   "outline_color_gdk", &outline,
						   "width_pixels", 1,
						   "fill_color_gdk", colour,
						   "fill_stipple",   bitmap,
						   "x1",             x1,
						   "x2",             x2,
						   "y1",             y1,
						   "y2",             y2,
						   NULL);
    }

  return ;
}

static void redraw_and_repick_if_mapped (FooCanvasItem *item)
{
  if (item->object.flags & FOO_CANVAS_ITEM_MAPPED) {
    foo_canvas_item_request_redraw (item);
    item->canvas->need_repick = TRUE;
  }
  return ;
}
static int is_descendant (FooCanvasItem *item, FooCanvasItem *parent)
{
  for (; item; item = item->parent)
    if (item == parent)
      return TRUE;
  
  return FALSE;
}

/* Adds an item to a group */
static void group_add (FooCanvasGroup *group, FooCanvasItem *item)
{
  g_object_ref (GTK_OBJECT (item));
  gtk_object_sink (GTK_OBJECT (item));
  
  if (!group->item_list) 
    {
      group->item_list = g_list_append (group->item_list, item);
      group->item_list_end = group->item_list;
    } 
  else
    group->item_list_end = g_list_append (group->item_list_end, item)->next;
  
  if (item->object.flags & FOO_CANVAS_ITEM_VISIBLE &&
      group->item.object.flags & FOO_CANVAS_ITEM_MAPPED) {
    if (!(item->object.flags & FOO_CANVAS_ITEM_REALIZED))
      (* FOO_CANVAS_ITEM_GET_CLASS (item)->realize) (item);
    
    if (!(item->object.flags & FOO_CANVAS_ITEM_MAPPED))
      (* FOO_CANVAS_ITEM_GET_CLASS (item)->map) (item);
  }
  return ;
}

/* Removes an item from a group */
static void group_remove (FooCanvasGroup *group, FooCanvasItem *item)
{
  GList *children;
  
  g_return_if_fail (FOO_IS_CANVAS_GROUP (group));
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  
  for (children = group->item_list; children; children = children->next)
    {
      if (children->data == item) 
	{
	  if (item->object.flags & FOO_CANVAS_ITEM_MAPPED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unmap) (item);
	  
	  if (item->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unrealize) (item);
	  
	  /* Unparent the child */
	  
	  item->parent = NULL;
	  /* item->canvas = NULL; */ /* Just to comment this, without rebuilding! */
	  g_object_unref (item);
	  
	  /* Remove it from the list */
	  
	  if (children == group->item_list_end)
	    group->item_list_end = children->prev;
	  
	  group->item_list = g_list_remove_link (group->item_list, children);
	  g_list_free (children);
	  break;
	}
    }

  return ;
}

void zMapWindowCanvasItemReparent(FooCanvasItem *item, FooCanvasGroup *new_group)
{
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  g_return_if_fail (FOO_IS_CANVAS_GROUP (new_group));
  
  /* Both items need to be in the same canvas */
  g_return_if_fail (item->canvas == FOO_CANVAS_ITEM (new_group)->canvas);
  
  /* The group cannot be an inferior of the item or be the item itself --
   * this also takes care of the case where the item is the root item of
   * the canvas.  */
  g_return_if_fail (!is_descendant (FOO_CANVAS_ITEM (new_group), item));
  
  /* Everything is ok, now actually reparent the item */
  
  g_object_ref (GTK_OBJECT (item)); /* protect it from the unref in group_remove */
  
  foo_canvas_item_request_redraw (item);
  
  group_remove (FOO_CANVAS_GROUP (item->parent), item);
  item->parent = FOO_CANVAS_ITEM (new_group);
  /* item->canvas is unchanged.  */
  group_add (new_group, item);
  
  /* Redraw and repick */
  
  redraw_and_repick_if_mapped (item);
  
  g_object_unref (GTK_OBJECT (item));

  return ;
}

/* INTERNALS */

/* Class initialization function for ZMapWindowCanvasItemClass */
static void zmap_window_canvas_item_class_init (ZMapWindowCanvasItemClass class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  GType canvas_item_type, parent_type;
  int make_clickable_bmp_height  = 4;
  int make_clickable_bmp_width   = 16;
  char make_clickable_bmp_bits[] =
    {
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00,
      0x00, 0x00
    } ;

  gobject_class = (GObjectClass *) class;
  object_class  = (GtkObjectClass *) class;
  item_class    = (FooCanvasItemClass *) class;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_CANVAS_ITEM_NAME);
  parent_type      = g_type_parent(canvas_item_type);

  group_parent_class_G = gtk_type_class(parent_type);
  
  gobject_class->set_property = zmap_window_canvas_item_set_property;
  gobject_class->get_property = zmap_window_canvas_item_get_property;
  
  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_INTERVAL_TYPE,
				  g_param_spec_uint("interval-type", "interval type",
						    "The type to use when creating a new interval",
						    0, 127, 0, ZMAP_PARAM_STATIC_WO));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_AUTO_RESIZE_BG,
				  g_param_spec_uint("auto-resize", "auto resize background",
						    "Turn on to automatically resize the background for non-scaling types",
						    0, 127, 0, ZMAP_PARAM_STATIC_WO));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_FEATURE,
				  g_param_spec_pointer("feature", "feature",
						       "ZMapFeatureAny pointer",
						       ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_USER_HIDDEN,
				  g_param_spec_boolean("user-hidden", "user hidden",
						       "Item was hidden by a user action.",
						       FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class, WINDOW_CANVAS_ITEM_CODE_HIDDEN,
				  g_param_spec_boolean("code-hidden", "code hidden",
						       "Item was hidden by coding logic.",
						       FALSE, ZMAP_PARAM_STATIC_RW));


  gobject_class->dispose = zmap_window_canvas_item_destroy;

  item_class->update    = zmap_window_canvas_item_update;
  item_class->realize   = zmap_window_canvas_item_realize;
  item_class->unrealize = zmap_window_canvas_item_unrealize;
  item_class->map       = zmap_window_canvas_item_map;
  item_class->unmap     = zmap_window_canvas_item_unmap;

  item_class->draw      = zmap_window_canvas_item_draw;
  item_class->point     = zmap_window_canvas_item_point;
  item_class->translate = zmap_window_canvas_item_translate;
  item_class->bounds    = zmap_window_canvas_item_bounds;

  class->post_create = zmap_window_canvas_item_post_create;
  class->check_data  = zmap_window_canvas_item_check_data;
  class->set_colour  = zmap_window_canvas_item_set_colour;

  class->fill_stipple = gdk_bitmap_create_from_data(NULL, &make_clickable_bmp_bits[0],
						    make_clickable_bmp_width, make_clickable_bmp_height) ;
      

  return ;
}


/* Object initialization function for ZMapWindowCanvasItem */
static void zmap_window_canvas_item_init (ZMapWindowCanvasItem canvas_item)
{
  FooCanvasGroup *group;

  group = FOO_CANVAS_GROUP(canvas_item);
  group->xpos = 0.0;
  group->ypos = 0.0;

  canvas_item->auto_resize_background = 0;

  canvas_item->mark_item = NULL;

  return ;
}

/* Set_property handler for canvas groups */
static void zmap_window_canvas_item_set_property (GObject *gobject, guint param_id,
						  const GValue *value, GParamSpec *pspec)
{
  FooCanvasItem *item;
  FooCanvasGroup *group;
  gboolean moved;
  
  g_return_if_fail (FOO_IS_CANVAS_GROUP (gobject));
  
  item  = FOO_CANVAS_ITEM (gobject);
  group = FOO_CANVAS_GROUP (gobject);
  
  moved = FALSE;

  switch (param_id) 
    {
    case WINDOW_CANVAS_ITEM_AUTO_RESIZE_BG:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);
	
	canvas_item->auto_resize_background = g_value_get_uint(value);
      }
      break;
    case WINDOW_CANVAS_ITEM_INTERVAL_TYPE:
      {
	guint type = 0;

	type = g_value_get_uint(value);

	switch(type)
	  {
	  default:
	    g_warning("%s", "interval-type is expected to be overridden");
	    break;
	  }
      }
      break;
    case WINDOW_CANVAS_ITEM_FEATURE:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);

	canvas_item->feature = g_value_get_pointer(value);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }
  
  return ;
}

/* Get_property handler for canvas groups */
static void zmap_window_canvas_item_get_property (GObject *gobject, guint param_id,
						  GValue *value, GParamSpec *pspec)
{
  FooCanvasItem *item;
  FooCanvasGroup *group;
  
  g_return_if_fail (FOO_IS_CANVAS_GROUP (gobject));
  
  item  = FOO_CANVAS_ITEM (gobject);
  group = FOO_CANVAS_GROUP (gobject);
  
  switch (param_id) 
    {
    case WINDOW_CANVAS_ITEM_FEATURE:
      {
	ZMapWindowCanvasItem canvas_item;

	canvas_item = ZMAP_CANVAS_ITEM(gobject);

	g_value_set_pointer(value, canvas_item->feature);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }

  return ;
}

/* Destroy handler for canvas groups */
static void zmap_window_canvas_item_destroy (GObject *object)
{
  ZMapWindowCanvasItem canvas_item;
  int i;

  canvas_item = ZMAP_CANVAS_ITEM(object);

  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(canvas_item->items[i])
	{
	  gtk_object_destroy(GTK_OBJECT(canvas_item->items[i]));
	  canvas_item->items[i] = NULL;
	}
    }

  canvas_item->feature = NULL;
  canvas_item->style   = NULL;

  if(GTK_OBJECT_CLASS (group_parent_class_G)->destroy)
    (GTK_OBJECT_CLASS (group_parent_class_G)->destroy)(GTK_OBJECT(object));

  return ;
}

static void zmap_window_canvas_item_post_create(ZMapWindowCanvasItem canvas_item)
{
  FooCanvasGroup *group;
  FooCanvasItem *remove_me;
  int i;
  //#define DEBUG_BACKGROUND_BOX
#ifdef DEBUG_BACKGROUND_BOX
  GdkColor outline = {0}; 
  GdkColor fill    = {0};

  gdk_color_parse("orange", &outline);
  gdk_color_parse("gold", &fill);
#endif /* DEBUG_BACKGROUND_BOX */

  group = FOO_CANVAS_GROUP(canvas_item);

  /* This is fun.  We add more items to ourselves... The parent object's (FooCanvasGroup).
   * These _must_ be 'forgotten' by the group though. We do this by free'ing the item
   * list it holds.  We hold onto the new items created and as this object implements the
   * FooCanvasItem interface they'll get drawn how and in what order we want them to be.
   */
  /* background is drawn _first_, below _everything_ else!  */
  canvas_item->items[WINDOW_ITEM_BACKGROUND] = 
    foo_canvas_item_new(group, foo_canvas_rect_get_type(),
			"x1", 0.0, "y1", 0.0, 
			"x1", 0.0, "y2", 0.0,
#ifdef DEBUG_BACKGROUND_BOX
			"width_pixels",      1,
			"outline_color_gdk", &outline,
			"fill_color_gdk",    &fill,
#endif /* DEBUG_BACKGROUND_BOX */
			NULL);


  /* underlays will be drawn next. */
  canvas_item->items[WINDOW_ITEM_UNDERLAY] = 
    foo_canvas_item_new(group, foo_canvas_group_get_type(),
			"x", 0.0,
			"y", 0.0,
			NULL);
  
  /* If this were the draw cycle the item_list members would be drawn now */

  /* overlays last to always be seen */
  canvas_item->items[WINDOW_ITEM_OVERLAY] = 
    foo_canvas_item_new(group, foo_canvas_group_get_type(),
			"x", 0.0,
			"y", 0.0,
			NULL);

  g_list_free(group->item_list);
  group->item_list = group->item_list_end = NULL;

  for(i = 10; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(remove_me = canvas_item->items[i]))
	continue;
      else
	{
	  if (remove_me->object.flags & FOO_CANVAS_ITEM_MAPPED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (remove_me)->unmap) (remove_me);
	  
	  if (remove_me->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (remove_me)->unrealize) (remove_me);
	}
    }
  
  

  return ;
}


/* Update handler for canvas groups */
static void zmap_window_canvas_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasGroup *canvas_group;
  FooCanvasItem *update_me;
  FooCanvasRE *bg_rect;
  double xpos, ypos;
  int i;
  
  canvas_item = ZMAP_CANVAS_ITEM(item);

  /* chain up, to update all the interval items that are legitimately part of the group, first... */
  (* group_parent_class_G->update) (item, i2w_dx, i2w_dy, flags);

  canvas_group = FOO_CANVAS_GROUP(item);
  
  xpos = canvas_group->xpos;
  ypos = canvas_group->ypos;

  if((canvas_item->auto_resize_background) && 
     (FOO_IS_CANVAS_RE(canvas_item->items[WINDOW_ITEM_BACKGROUND])) &&
     (bg_rect = FOO_CANVAS_RE(canvas_item->items[WINDOW_ITEM_BACKGROUND])))
    {
      double x1, x2, y1, y2;

      /* VERY IMPORTANT
       * --------------
       * This _must_ _not_ make use of the zmap_window_canvas_item_bounds()
       * function so as _not_ to include the background!!! 
       */
      (* group_parent_class_G->bounds)(item, &x1, &y1, &x2, &y2);

      /* itemise coords for the background. */
      x1 -= xpos;
      y1 -= ypos;
      x2 -= xpos;
      y2 -= ypos;

      /* Set the coords _directly_ to avoid 'in update' checking. 
       * This seems safe. */
      bg_rect->x1 = x1;
      bg_rect->x2 = x2;
      bg_rect->y1 = y1;
      bg_rect->y2 = y2;
    }

  /* Now update the illegitimate children of the group. */
  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(update_me = canvas_item->items[i]))
	continue;

      if(FOO_CANVAS_ITEM_GET_CLASS(update_me)->update)
	(* FOO_CANVAS_ITEM_GET_CLASS(update_me)->update)(update_me, i2w_dx + xpos, i2w_dy + ypos, flags);

      item->x1 = MIN (update_me->x1, item->x1);
      item->y1 = MIN (update_me->y1, item->y1);
      item->x2 = MAX (update_me->x2, item->x2);
      item->y2 = MAX (update_me->y2, item->y2);
    }

  return ;
}

static void zmap_window_canvas_item_realize (FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *realize_me;
  int i;

  canvas_item = ZMAP_CANVAS_ITEM(item);

  (* group_parent_class_G->realize)(item);

  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(realize_me = canvas_item->items[i]))
	continue;
      else
	{
	  if(!realize_me->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS(realize_me)->realize)(realize_me);
	}
    }

  return ;
}


/* Unrealize handler for canvas groups */
static void zmap_window_canvas_item_unrealize (FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *unrealize_me;
  int i;

  canvas_item = ZMAP_CANVAS_ITEM(item);

  (* group_parent_class_G->unrealize) (item);

  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(unrealize_me = canvas_item->items[i]))
	continue;
      else if(unrealize_me->object.flags & FOO_CANVAS_ITEM_REALIZED)
	(* FOO_CANVAS_ITEM_GET_CLASS(unrealize_me)->unrealize)(unrealize_me);
    }

  return ;
}

/* Map handler for canvas groups */
static void zmap_window_canvas_item_map (FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *map_me;
  int i;

  canvas_item = ZMAP_CANVAS_ITEM(item);

  (* group_parent_class_G->map) (item);

  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(map_me = canvas_item->items[i]))
	continue;
      else
	{
	  if((map_me->object.flags & FOO_CANVAS_ITEM_VISIBLE) &&
	     !(map_me->object.flags & FOO_CANVAS_ITEM_MAPPED))
	    {
	      if(!(map_me->object.flags & FOO_CANVAS_ITEM_REALIZED))
		(* FOO_CANVAS_ITEM_GET_CLASS(map_me)->realize)(map_me);

	      if((FOO_CANVAS_ITEM_GET_CLASS(map_me)->map))
		(*FOO_CANVAS_ITEM_GET_CLASS(map_me)->map)(map_me);
	    }
	}
    }

  return ;
}

/* Unmap handler for canvas groups */
static void zmap_window_canvas_item_unmap (FooCanvasItem *item)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *unmap_me;
  int i;

  canvas_item = ZMAP_CANVAS_ITEM(item);

  (* group_parent_class_G->unmap) (item);

  for(i = 0; i < WINDOW_ITEM_COUNT; ++i)
    {
      if(!(unmap_me = canvas_item->items[i]))
	continue;
      else
	{
	  if((unmap_me->object.flags & FOO_CANVAS_ITEM_VISIBLE) &&
	     !(unmap_me->object.flags & FOO_CANVAS_ITEM_MAPPED))
	    {
	      if(!(unmap_me->object.flags & FOO_CANVAS_ITEM_REALIZED))
		(* FOO_CANVAS_ITEM_GET_CLASS(unmap_me)->realize)(unmap_me);

	      if((FOO_CANVAS_ITEM_GET_CLASS(unmap_me)->unmap))
		(*FOO_CANVAS_ITEM_GET_CLASS(unmap_me)->unmap)(unmap_me);
	    }
	}
    }
  
  return ;
}

/* Draw handler for canvas groups */
static void zmap_window_canvas_item_draw (FooCanvasItem *item, GdkDrawable *drawable,
					  GdkEventExpose *expose)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasItem *draw_me = NULL;
  int i;

  g_return_if_fail(ZMAP_IS_CANVAS_ITEM(item));

  canvas_item = ZMAP_CANVAS_ITEM(item);

  for(i = (item->object.flags & FOO_CANVAS_ITEM_VISIBLE ? 0 : WINDOW_ITEM_COUNT); 
      i < WINDOW_ITEM_COUNT; ++i)
    {
      if(i == WINDOW_ITEM_OVERLAY)
	(* group_parent_class_G->draw) (item, drawable, expose);
      
      if(!(draw_me = canvas_item->items[i]))
	continue;
      else
	{
	  if((draw_me->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	    {
	      if(!(draw_me->object.flags & FOO_CANVAS_ITEM_REALIZED))
		(* FOO_CANVAS_ITEM_GET_CLASS(draw_me)->realize)(draw_me);
	      
	      if(!(draw_me->object.flags & FOO_CANVAS_ITEM_MAPPED))
		(*FOO_CANVAS_ITEM_GET_CLASS(draw_me)->map)(draw_me);

	      if((FOO_CANVAS_ITEM_GET_CLASS(draw_me)->draw))
		(*FOO_CANVAS_ITEM_GET_CLASS(draw_me)->draw)(draw_me, drawable, expose);
	    }
	}
    }

  return;
}

/* Point handler for canvas groups */
static double zmap_window_canvas_item_point (FooCanvasItem *item, double x, double y, int cx, int cy,
					     FooCanvasItem **actual_item)
{
  FooCanvasGroup *group;
  FooCanvasItem *child, *point_item;
  int x1, y1, x2, y2;
  double gx, gy;
  double dist, best;

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

  if((child = ZMAP_CANVAS_ITEM(item)->items[WINDOW_ITEM_BACKGROUND]))
    {
      if ((child->object.flags & FOO_CANVAS_ITEM_MAPPED)
	  && FOO_CANVAS_ITEM_GET_CLASS (child)->point) 
	{
	  dist = window_canvas_item_invoke_point (child, gx, gy, cx, cy, &point_item);
	  if(point_item && ((int)(dist * item->canvas->pixels_per_unit_x + 0.5) <= item->canvas->close_enough))
	    {
	      best = dist;
	      *actual_item = point_item;
	    }
	} 
    }
  
  return best;
}

static void zmap_window_canvas_item_translate (FooCanvasItem *item, double dx, double dy)
{
  FooCanvasGroup *group;
  
  group = FOO_CANVAS_GROUP (item);
  
  group->xpos += dx;
  group->ypos += dy;

  return ;
}

/* Bounds handler for canvas groups */
static void zmap_window_canvas_item_bounds (FooCanvasItem *item, 
					    double *x1_out, double *y1_out, 
					    double *x2_out, double *y2_out)
{
  ZMapWindowCanvasItem canvas_item;
  FooCanvasGroup *group;
  FooCanvasItem *background;

  if(ZMAP_IS_CANVAS_ITEM(item))
    {
      double x1, x2, y1, y2;
      double gx = 0.0, gy = 0.0;

      group = FOO_CANVAS_GROUP(item);
      canvas_item = ZMAP_CANVAS_ITEM(item);

      gx = group->xpos;
      gy = group->ypos;
      
      if((background = canvas_item->items[WINDOW_ITEM_BACKGROUND]))
	{
	  gboolean fix_undrawn_have_zero_size = TRUE;
	  
	  (* FOO_CANVAS_ITEM_GET_CLASS(background)->bounds)(background, &x1, &y1, &x2, &y2);
	  
	  /* test this! */
	  if(fix_undrawn_have_zero_size && x1 == x2 && y1 == y2 && x1 == y1 && x1 == 0.0)
	    {
	      FooCanvasRE *rect = FOO_CANVAS_RE(background);
	      x1 = rect->x1;
	      y1 = rect->y1;
	      x2 = rect->x2;
	      y2 = rect->y2;
	      g_warning("saved from zero sized bounds...");
	    }

	}
      
      if(x1_out)
	*x1_out = x1 + gx;
      if(x2_out)
	*x2_out = x2 + gx;

      if(y1_out)
	*y1_out = y1 + gy;
      if(y2_out)
	*y2_out = y2 + gy;

    }

  return ;
}

#ifdef WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED
static void window_canvas_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  (* group_parent_class_G->bounds)(item, x1, y1, x2, y2);
  return ;
}
#endif /* WINDOW_CANVAS_ITEM_BOUNDS_REQUIRED */

static void zmap_window_canvas_item_set_colour(ZMapWindowCanvasItem  canvas_item,
					       FooCanvasItem        *interval,
					       ZMapWindowItemFeature unused,
					       ZMapStyleColourType   colour_type,
					       GdkColor             *default_fill)
{
  ZMapFeatureTypeStyle style;
  GdkColor *fill = NULL, *outline = NULL;

  g_return_if_fail(canvas_item != NULL);
  g_return_if_fail(interval    != NULL);

  if((style = canvas_item->style))
    {
      zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, colour_type,
			  &fill, NULL, &outline);
    }

  if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED && default_fill)
    fill = default_fill;

  if(FOO_IS_CANVAS_LINE(interval)       || 
     FOO_IS_CANVAS_LINE_GLYPH(interval) ||
     FOO_IS_CANVAS_TEXT(interval))
    {
      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);
    }
  else if(FOO_IS_CANVAS_RE(interval)      ||
	  FOO_IS_CANVAS_POLYGON(interval) ||
	  ZMAP_IS_WINDOW_GLYPH_ITEM(interval))
    {
      if(!outline)
	g_object_get(G_OBJECT(interval),
		     "outline_color_gdk", &outline,
		     NULL);
      foo_canvas_item_set(interval,
			  "fill_color_gdk",    fill,
			  "outline_color_gdk", outline,
			  NULL);
    }
  else
    {
      g_warning("Interval has unknown FooCanvasItem type.");
    }

  return ;
}

static gboolean zmap_window_canvas_item_check_data(ZMapWindowCanvasItem canvas_item, GError **error)
{
  FooCanvasGroup *group;
  gboolean result = TRUE;
  GList *list;
   
  group = FOO_CANVAS_GROUP(canvas_item);
  
  list = group->item_list;

  while(result && list)
    {
      gpointer data;

      if(!(data = g_object_get_data(G_OBJECT(list->data), ITEM_SUBFEATURE_DATA)))
	{
	  *error = g_error_new(zmap_window_canvas_item_get_domain(), 2,
			       "Interval has no '%s' data", ITEM_SUBFEATURE_DATA);
	  result = FALSE;
	}

      list = list->next;
    }

  return result;
}

static void update_background_box_size(ZMapWindowCanvasItem canvas_item,
				       double x1, double y1,
				       double x2, double y2)
{
  FooCanvasItem *background;

  g_return_if_fail(ZMAP_IS_CANVAS_ITEM(canvas_item));

  if((background = canvas_item->items[WINDOW_ITEM_BACKGROUND]))
    foo_canvas_item_set(background,
			"x1", x1, "x2", x2,
			"y1", y1, "y2", y2,
			NULL);

  return ;
}



/*
 * This routine invokes the update method of the item
 * Please notice, that we take parent to canvas pixel matrix as argument
 * unlike virtual method ::update, whose argument is item 2 canvas pixel
 * matrix
 *
 * I will try to force somewhat meaningful naming for affines (Lauris)
 * General naming rule is FROM2TO, where FROM and TO are abbreviations
 * So p2cpx is Parent2CanvasPixel and i2cpx is Item2CanvasPixel
 * I hope that this helps to keep track of what really happens
 *
 */
#ifdef WINDOW_CANVAS_ITEM_INVOKE_UPDATE_REQ
static void window_canvas_item_invoke_update (FooCanvasItem *item,
					      double i2w_dx,
					      double i2w_dy,
					      int flags)
{
  int child_flags;
  
  child_flags = flags;
  
  /* apply object flags to child flags */
  child_flags &= ~FOO_CANVAS_UPDATE_REQUESTED;
  
  if (item->object.flags & FOO_CANVAS_ITEM_NEED_UPDATE)
    child_flags |= FOO_CANVAS_UPDATE_REQUESTED;
  
  if (item->object.flags & FOO_CANVAS_ITEM_NEED_DEEP_UPDATE)
    child_flags |= FOO_CANVAS_UPDATE_DEEP;
  
  if (child_flags & GCI_UPDATE_MASK) {
    if (FOO_CANVAS_ITEM_GET_CLASS (item)->update)
      FOO_CANVAS_ITEM_GET_CLASS (item)->update (item, i2w_dx, i2w_dy, child_flags);
  }
  
  /* If this fail you probably forgot to chain up to
   * FooCanvasItem::update from a derived class */
  g_return_if_fail (!(item->object.flags & FOO_CANVAS_ITEM_NEED_UPDATE));

  return;
}
#endif /* WINDOW_CANVAS_ITEM_INVOKE_UPDATE_REQ */

/*
 * This routine invokes the point method of the item.
 * The arguments x, y should be in the parent item local coordinates.
 */

static double window_canvas_item_invoke_point (FooCanvasItem *item, 
					       double x, double y, 
					       int cx, int cy,
					       FooCanvasItem **actual_item)
{
  /* Calculate x & y in item local coordinates */
  
  if (FOO_CANVAS_ITEM_GET_CLASS (item)->point)
    return FOO_CANVAS_ITEM_GET_CLASS (item)->point (item, x, y, cx, cy, actual_item);
  
  return 1e18;
}


static gboolean feature_is_drawable(ZMapFeature          feature_any, 
				    ZMapFeatureTypeStyle feature_style, 
				    ZMapStyleMode       *mode_to_use,
				    GType               *type_to_use)
{
  GType type;
  ZMapStyleMode mode;
  gboolean result = FALSE;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature;
	feature = (ZMapFeature)feature_any;
	
	mode = feature->type;
	mode = zMapStyleGetMode(feature_style);

	switch(mode)
	  {
	  case ZMAPSTYLE_MODE_TRANSCRIPT:
	    type   = zMapWindowTranscriptFeatureGetType();
	    break;
	  case ZMAPSTYLE_MODE_ALIGNMENT:
	    type   = zMapWindowAlignmentFeatureGetType();
	    break;
	  case ZMAPSTYLE_MODE_TEXT:
	    type   = zMapWindowTextFeatureGetType();
	    break;
	  case ZMAPSTYLE_MODE_BASIC:
	  default:
	    type   = zMapWindowBasicFeatureGetType();
	    break;
	  }
	
	result = TRUE;
      }
      break;
    default:
      break;
    }


  if(mode_to_use && result)
    *mode_to_use = mode;
  if(type_to_use && result)
    *type_to_use = type;

  return result;
}


static void window_canvas_invoke_set_colours(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *interval        = FOO_CANVAS_ITEM(list_data);
  EachIntervalData interval_data = (EachIntervalData)user_data;

  if(interval_data->feature)
    {
      ZMapWindowItemFeature sub_feature = NULL;

      sub_feature = g_object_get_data(G_OBJECT(interval), ITEM_SUBFEATURE_DATA);

      if(interval_data->klass->set_colour)
	interval_data->klass->set_colour(interval_data->parent, interval, sub_feature, 
					 interval_data->style_colour_type, 
					 interval_data->default_fill_colour);
    }

  return ;
}
