/*  File: zmapWindowLongItem.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * Description: Implements code to clip items that would exceed the
 *              X Windows max window limit if they were drawn. Code
 *              keeps track of original size and restores size
 *              when canvas zoomed back out.
 *
 * Exported functions: See zmapWindowLongItem.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







#include <math.h>
#include <string.h>
#include <zmapWindowCanvas.h>
#include <zmapWindowLongItem_I.h>


enum
  {
    LONG_ITEM_PROP_0,		/* Zero == invalid for param ids */
    LONG_ITEM_PROP_CHILD,
    LONG_ITEM_PROP_SHAPE_POINTS,
    LONG_ITEM_PROP_SHAPE_X1,
    LONG_ITEM_PROP_SHAPE_Y1,
    LONG_ITEM_PROP_SHAPE_X2,
    LONG_ITEM_PROP_SHAPE_Y2,
    LONG_ITEM_PROP_SUB_GTYPE,
  };


static void zmap_window_long_item_class_init   (ZMapWindowLongItemClass class);
static void zmap_window_long_item_init         (ZMapWindowLongItem zmap_item);
static void zmap_window_long_item_destroy      (GtkObject *object);
static void zmap_window_long_item_set_property (GObject            *object,
						 guint               param_id,
						 const GValue       *value,
						 GParamSpec         *pspec);
static void zmap_window_long_item_get_property (GObject            *object,
						 guint               param_id,
						 GValue             *value,
						 GParamSpec         *pspec);

static void   zmap_window_long_item_update     (FooCanvasItem  *item,
						 double            i2w_dx,
						 double            i2w_dy,
						 int               flags);
static void   zmap_window_long_item_realize    (FooCanvasItem  *item);
static void   zmap_window_long_item_unrealize  (FooCanvasItem  *item);
static void   zmap_window_long_item_draw       (FooCanvasItem  *item,
						 GdkDrawable      *drawable,
						 GdkEventExpose   *expose);
static double zmap_window_long_item_point      (FooCanvasItem  *item,
						 double            x,
						 double            y,
						 int               cx,
						 int               cy,
						 FooCanvasItem **actual_item);
static void   zmap_window_long_item_translate  (FooCanvasItem  *item,
						 double            dx,
						 double            dy);
static void   zmap_window_long_item_bounds     (FooCanvasItem  *item,
						 double           *x1,
						 double           *y1,
						 double           *x2,
						 double           *y2);
static void set_child_item(FooCanvasItem *zmap_item, FooCanvasItem *long_item);
static void group_remove (FooCanvasGroup *group, FooCanvasItem *item);


/* 
 *            Local globals.
 */


static FooCanvasItemClass *parent_class_G;

/* X Windows has a hard limit of 65k * 65k for the biggest window size that can be created
 * but bizarrely the limit for drawing many types of data is 32k * 32k. This code clips
 * objects that would exceed this size. */
static double max_window_size_G = 32000.0 ;





GType zMapWindowLongItemGetType (void)
{
  static GType type = 0;
  
  if (type == 0)
    {
      static const GTypeInfo type_info = 
	{
	  sizeof(zmapWindowLongItemClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) zmap_window_long_item_class_init,
	  NULL,
	  NULL,
	  sizeof(zmapWindowLongItem),
	  0,
	  (GInstanceInitFunc)zmap_window_long_item_init,
	  NULL
	};
      
      type = g_type_register_static(foo_canvas_item_get_type(),
				    ZMAP_WINDOW_LONG_ITEM_NAME,
				    &type_info, 0);
    }
  
  return type;
}

double zmapWindowLongItemMaxWindowSize(void)
{
  double max;

  max = max_window_size_G;

  return max;
}

// has to be here as the macro is defined in a header internal to this file
// and we need to ask this question from zmapWindowDump.c
int zmapWindowIsLongItem(FooCanvasItem *foo)
{
  return(ZMAP_IS_WINDOW_LONG_ITEM(foo));
}


FooCanvasItem *zmapWindowGetLongItem(FooCanvasItem *foo)
{
  FooCanvasItem *long_item = NULL ;
  ZMapWindowLongItem zmap_long ;

  if ((zmap_long = ZMAP_WINDOW_LONG_ITEM(foo)))
    long_item = zmap_long->long_item ;

  return long_item ;
}

static void get_points_extent_box(FooCanvasPoints *points, 
				  double *x1_out, double *y1_out, 
				  double *x2_out, double *y2_out)
{
  double *coords;
  double x1, y1, x2, y2;
  int i;
  
  if(x1_out && y1_out && x2_out && y2_out &&
     (points->num_points > 0) && 
     (coords = points->coords))
    {
      x1 = x2 = coords[0];
      y1 = y2 = coords[1];
      coords+=2;

      for(i = 1; i < points->num_points; i++, coords+=2)
	{			
	  /* FooCanvas code has a GROW_BOUNDS Macro :) */
	  /* x first */
	  if (coords[0] < x1)
	    x1 = coords[0];
	  if (coords[0] > x2)
	    x2 = coords[0];

	  /* y coords */
	  if (coords[1] < y1)
	    y1 = coords[1];
	  if (coords[1] > y2)
	    y2 = coords[1];
	}

      *x1_out = x1;
      *y1_out = y1;
      *x2_out = x2;
      *y2_out = y2;
    }

  return ;
}

FooCanvasItem *zmapWindowLongItemCheckPoint(FooCanvasItem   *possibly_long_item)
{
  FooCanvasItem *result = NULL;

  if (FOO_IS_CANVAS_LINE(possibly_long_item) || FOO_IS_CANVAS_POLYGON(possibly_long_item))
    {
      FooCanvasPoints *points;
      g_object_get(G_OBJECT(possibly_long_item),
		   "points", &points,
		   NULL);
      result = zmapWindowLongItemCheckPointFull(possibly_long_item, points,
						0.0, 0.0, 0.0, 0.0);
      foo_canvas_points_free(points);
    }
  else if (FOO_IS_CANVAS_RE(possibly_long_item))
    {
      double x1, x2, y1, y2;
      g_object_get(G_OBJECT(possibly_long_item),
		   "x1", &x1,
		   "y1", &y1,
		   "x2", &x2,
		   "y2", &y2,
		   NULL);
      result = zmapWindowLongItemCheckPointFull(possibly_long_item, NULL,
						x1, y1, x2, y2);
    }
  else
    {
      result = possibly_long_item;
    }

  return result;
}


/* I think what this function is intended to do is this:
 * 
 * Takes an item and if the item is of a type that might exceed the X windows
 * big window limit it clips it and replaces the item in it's group with the
 * clipped copy. The clipped copy contains the original so it can be restored.
 * 
 *  */
FooCanvasItem *zmapWindowLongItemCheckPointFull(FooCanvasItem   *possibly_long_item,
						FooCanvasPoints *points,
						double x1, double y1, 
						double x2, double y2)
{
  FooCanvasItem *result = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This function originally had this hard-coded and the calls below to get the zoom
   * were commented out, I've reinstated the calls as it cannot be correct to have
   * this hard-coded. */

  double max_zoom_y = 17.0;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  double max_zoom_y ;

  gboolean has_points;

  /* Only lines, polygons, rectangle and ellipses can be long! */
  if (FOO_IS_CANVAS_LINE(possibly_long_item) || FOO_IS_CANVAS_POLYGON(possibly_long_item))
    {
      get_points_extent_box(points, &x1, &y1, &x2, &y2);

      if(ZMAP_IS_CANVAS(possibly_long_item->canvas))
	{
	  g_object_get(G_OBJECT(possibly_long_item->canvas),
		       "max-zoom-y", &max_zoom_y,
		       NULL);
	}

      /* check length... */
      if(((y2 - y1 + 1.0) * max_zoom_y) < max_window_size_G)
	{
	  /* too small, just return */
	  result = possibly_long_item;
	}

      has_points = TRUE;
    }
  else if (FOO_IS_CANVAS_RE(possibly_long_item))
    {
      if(ZMAP_IS_CANVAS(possibly_long_item->canvas))
	{
	  g_object_get(G_OBJECT(possibly_long_item->canvas),
		       "max-zoom-y", &max_zoom_y,
		       NULL);
	}

      /* check length... */
      if(((y2 - y1 + 1.0) * max_zoom_y) < max_window_size_G)
	{
	  /* too small, just return */
	  result = possibly_long_item;
	}

      has_points = FALSE;
    }
  else
    {
       /* No need to check here, or create, so just return. */
      result = possibly_long_item ;
    }

  if (!result)
    {
      ZMapWindowLongItem zmap_long;
      FooCanvasItem *zmap_item = NULL ;
      FooCanvasGroup *group;
      /* must be a long item. */      
      FooCanvasItem *definitely_long_item = possibly_long_item;

      /* need some magic here... */

      /* need to group remove the original item. */
      group = FOO_CANVAS_GROUP(definitely_long_item->parent);

      /* protect from group_remove's unref */
      g_object_ref(G_OBJECT(definitely_long_item));

      group_remove(group, definitely_long_item);

      /* create the object of LongItem type, which will add itself to the group. */
      zmap_item = foo_canvas_item_new(group, ZMAP_TYPE_WINDOW_LONG_ITEM, 
				      "child", definitely_long_item,
				      NULL);

      zmap_long = ZMAP_WINDOW_LONG_ITEM(zmap_item);

      /* we should now have enough references to drop ours. */
      g_object_unref(G_OBJECT(definitely_long_item));

      /* and records original shape */
      if((zmap_long->shape.has_points = has_points))
	{
	  zmap_long->shape.shape.points = foo_canvas_points_new(points->num_points);
	  memcpy(&(zmap_long->shape.shape.points->coords[0]),
		 &points->coords[0], points->num_points * 2 * sizeof(double));
	}
      else
	{
	  zmap_long->shape.shape.box_coords[0] = x1;
	  zmap_long->shape.shape.box_coords[1] = y1;
	  zmap_long->shape.shape.box_coords[2] = x2;
	  zmap_long->shape.shape.box_coords[3] = y2;
	}

      /* always save the maximum extents */
      zmap_long->shape.extent_box[0] = x1;
      zmap_long->shape.extent_box[1] = y1;
      zmap_long->shape.extent_box[2] = x2;
      zmap_long->shape.extent_box[3] = y2;
      
      /* Finally return the new object. */
      result = zmap_item;
    }

  return result ;
}

static void take_class_properties(ZMapWindowLongItemClass long_class, GType child_type, int *param_id_start_end)
{
  GObjectClass *gobject_class, *child_class;
  GParamSpec **child_pspecs, **first_pspec;
  int param_start_id;
  guint child_pspec_count;

  if(!(child_class = g_type_class_peek(child_type)))
    child_class = g_type_class_ref(child_type);
  
  if(child_class && param_id_start_end)
    {
      gobject_class = (GObjectClass *)long_class;

      param_start_id = *param_id_start_end + 1;
      
      child_pspecs = first_pspec = g_object_class_list_properties(child_class, &child_pspec_count);

      for(; child_pspec_count > 0; child_pspec_count--, child_pspecs++)
	{
	  GParamSpec *current, *new_pspec;
	  const char *name, *nick, *blurb;

	  current = *child_pspecs;

	  name  = g_param_spec_get_name(current);
	  nick  = g_param_spec_get_nick(current);
	  blurb = g_param_spec_get_nick(current);

	  if(!(g_object_class_find_property(gobject_class, name)))
	    {
	      new_pspec = g_param_spec_internal(G_PARAM_SPEC_TYPE(current),
						name, nick, blurb, current->flags);

	      new_pspec->value_type = current->value_type;
	      
	      g_object_class_install_property(gobject_class,
					      param_start_id,
					      new_pspec);
	      param_start_id++;
	    }
	}

      if(first_pspec)
	g_free(first_pspec);
      
      *param_id_start_end = param_start_id;
    }

  return ;
}

static void zmap_window_long_item_map (FooCanvasItem *item)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(parent_class_G->map)
    (parent_class_G->map)(item);

  /* check flags here... */
  if(zmap_item->long_item_class->map)
    (zmap_item->long_item_class->map)(zmap_item->long_item);
  
  return ;
}

static void zmap_window_long_item_unmap (FooCanvasItem *item)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  /* check flags here... */
  if(zmap_item->long_item_class->unmap)
    (zmap_item->long_item_class->unmap)(zmap_item->long_item);
  
  if(parent_class_G->unmap)
    (parent_class_G->unmap)(item);

  return ;
}


/* Class initialization function */
static void zmap_window_long_item_class_init (ZMapWindowLongItemClass long_class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  int curr_param_id;

  gobject_class = (GObjectClass *) long_class;
  object_class = (GtkObjectClass *) long_class;
  item_class = (FooCanvasItemClass *) long_class;
  
  parent_class_G = gtk_type_class (foo_canvas_item_get_type ());
  
  gobject_class->set_property = zmap_window_long_item_set_property;
  gobject_class->get_property = zmap_window_long_item_get_property;

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_CHILD,
				  g_param_spec_object("child", "child",
						     "The child",
						     FOO_TYPE_CANVAS_ITEM,
						     G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SHAPE_POINTS,
				  g_param_spec_boxed("item-points", "item points",
						     "The original points the shape needs",
						     FOO_TYPE_CANVAS_POINTS,
						     G_PARAM_READABLE | G_PARAM_WRITABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SHAPE_X1,
				  g_param_spec_double("item-x1", "item x1",
						      "The original x1 coord",
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SHAPE_Y1,
				  g_param_spec_double("item-y1", "item y1",
						      "The original y1 coord",
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SHAPE_X2,
				  g_param_spec_double("item-x2", "item x2",
						      "The original x2 coord",
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SHAPE_Y2,
				  g_param_spec_double("item-y2", "item y2",
						      "The original y2 coord",
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0, 
						      G_PARAM_WRITABLE | G_PARAM_READABLE));

  g_object_class_install_property(gobject_class, LONG_ITEM_PROP_SUB_GTYPE,
				  g_param_spec_gtype("item-type", "item type",
						     "The gtype of the item",
						     FOO_TYPE_CANVAS_ITEM,
						     G_PARAM_WRITABLE | G_PARAM_READABLE));

  /* install all the possible properties of child objects. */
  curr_param_id = LONG_ITEM_PROP_SUB_GTYPE;
  take_class_properties(long_class, FOO_TYPE_CANVAS_RECT,    &curr_param_id);
  take_class_properties(long_class, FOO_TYPE_CANVAS_ELLIPSE, &curr_param_id);
  take_class_properties(long_class, FOO_TYPE_CANVAS_LINE,    &curr_param_id);
  take_class_properties(long_class, FOO_TYPE_CANVAS_POLYGON, &curr_param_id);

  object_class->destroy = zmap_window_long_item_destroy;
  
  item_class->map       = zmap_window_long_item_map;
  item_class->unmap     = zmap_window_long_item_unmap;
  item_class->update    = zmap_window_long_item_update;
  item_class->realize   = zmap_window_long_item_realize;
  item_class->unrealize = zmap_window_long_item_unrealize;
  item_class->draw      = zmap_window_long_item_draw;
  item_class->point     = zmap_window_long_item_point;
  item_class->translate = zmap_window_long_item_translate;
  item_class->bounds    = zmap_window_long_item_bounds;
  
  return ;
}

/* Object initialization function for the text item */
static void zmap_window_long_item_init (ZMapWindowLongItem zmap_long)
{

  return ;
}

static void zmap_window_long_item_destroy (GtkObject *object)
{
  ZMapWindowLongItem zmap_item;

  zmap_item = ZMAP_WINDOW_LONG_ITEM(object);

  if(zmap_item->shape.has_points)
    {
      if(zmap_item->shape.shape.points)
	foo_canvas_points_free(zmap_item->shape.shape.points);
      zmap_item->shape.shape.points = NULL;
      zmap_item->shape.has_points = FALSE;
    }

  if (GTK_OBJECT_CLASS (parent_class_G)->destroy)
    (* GTK_OBJECT_CLASS (parent_class_G)->destroy) (object);

  return ;
}

/* Set_arg handler for the text item */
static void zmap_window_long_item_set_property (GObject            *object,
						guint               param_id,
						const GValue       *value,
						GParamSpec         *pspec)
{
  ZMapWindowLongItem zmap_item;

  zmap_item = ZMAP_WINDOW_LONG_ITEM(object);

  switch(param_id)
    {
    case LONG_ITEM_PROP_CHILD:
      if(G_VALUE_HOLDS_OBJECT(value))
	set_child_item(FOO_CANVAS_ITEM(object), g_value_get_object(value));
      break;
    case LONG_ITEM_PROP_SHAPE_POINTS:
      /* use code foo canvas does here */
      break;
    case LONG_ITEM_PROP_SHAPE_X1:
      if(G_VALUE_HOLDS_DOUBLE(value))
	zmap_item->shape.shape.box_coords[0] = g_value_get_double(value);

      g_object_set(G_OBJECT(zmap_item->long_item), 
		   "x1", zmap_item->shape.shape.box_coords[0],
		   NULL);
      break;
    case LONG_ITEM_PROP_SHAPE_X2:
      if(G_VALUE_HOLDS_DOUBLE(value))
	zmap_item->shape.shape.box_coords[1] = g_value_get_double(value);

      g_object_set(G_OBJECT(zmap_item->long_item), 
		   "y1", zmap_item->shape.shape.box_coords[1],
		   NULL);
      break;
    case LONG_ITEM_PROP_SHAPE_Y1:
      if(G_VALUE_HOLDS_DOUBLE(value))
	zmap_item->shape.shape.box_coords[2] = g_value_get_double(value);

      g_object_set(G_OBJECT(zmap_item->long_item), 
		   "x2", zmap_item->shape.shape.box_coords[2],
		   NULL);
      break;
    case LONG_ITEM_PROP_SHAPE_Y2:
      if(G_VALUE_HOLDS_DOUBLE(value))
	zmap_item->shape.shape.box_coords[3] = g_value_get_double(value);

      g_object_set(G_OBJECT(zmap_item->long_item), 
		   "y2", zmap_item->shape.shape.box_coords[3],
		   NULL);
      break;
    case LONG_ITEM_PROP_SUB_GTYPE:
      break;
    default:
      {
	GParamSpec *child_pspec;

	if((child_pspec = g_object_class_find_property(zmap_item->object_class,
						       g_param_spec_get_name(pspec))))
	  {
	    zmap_item->object_class->set_property(G_OBJECT(zmap_item->long_item), 
						  child_pspec->param_id, 
						  value, child_pspec);
	  }
      }
      break;
    }

  return ;
}

/* Get_arg handler for the text item */
static void zmap_window_long_item_get_property (GObject            *object,
						guint               param_id,
						GValue             *value,
						GParamSpec         *pspec)
{
  ZMapWindowLongItem zmap_item;

  zmap_item = ZMAP_WINDOW_LONG_ITEM(object);

  switch(param_id)
    {
    case LONG_ITEM_PROP_SHAPE_POINTS:
      /* use code foo canvas does here */
      break;
    case LONG_ITEM_PROP_SHAPE_X1:
      if(G_VALUE_HOLDS_DOUBLE(value))
	g_value_set_double(value, zmap_item->shape.shape.box_coords[0]);
      break;
    case LONG_ITEM_PROP_SHAPE_X2:
      if(G_VALUE_HOLDS_DOUBLE(value))
	g_value_set_double(value, zmap_item->shape.shape.box_coords[1]);
      break;
    case LONG_ITEM_PROP_SHAPE_Y1:
      if(G_VALUE_HOLDS_DOUBLE(value))
	g_value_set_double(value, zmap_item->shape.shape.box_coords[2]);
      break;
    case LONG_ITEM_PROP_SHAPE_Y2:
      if(G_VALUE_HOLDS_DOUBLE(value))
	g_value_set_double(value, zmap_item->shape.shape.box_coords[3]);
      break;
    case LONG_ITEM_PROP_SUB_GTYPE:
      if(G_VALUE_HOLDS_GTYPE(value))
	g_value_set_gtype(value, G_OBJECT_TYPE(zmap_item->long_item));
      break;
    default:
      {
	GParamSpec *child_pspec;

	if((child_pspec = g_object_class_find_property(zmap_item->object_class,
						       g_param_spec_get_name(pspec))))
	  {
	    zmap_item->object_class->get_property(G_OBJECT(zmap_item->long_item),
						  child_pspec->param_id, 
						  value, child_pspec);
	  }
      }
      break;
    }
  
  return ;
}

static void crop_item_with_points(ZMapWindowLongItem zmap_item,
				  double scroll_x1, double scroll_y1,
				  double scroll_x2, double scroll_y2)
{
  FooCanvasPoints points;
  double *coords;
  int i, last;

  points.num_points = zmap_item->shape.shape.points->num_points;
  points.ref_count  = 1;
  coords            = g_new(double, points.num_points * 2);
  points.coords     = coords;

  last = points.num_points * 2;

  memcpy(coords, zmap_item->shape.shape.points->coords, sizeof(double) * last);
  
  for(i = 1; i < last; i+=2)	/* ONLY Y COORDS!!! */
    {
      if(coords[i] < scroll_y1)
	{
	  coords[i] = scroll_y1 - 1.0;
	}
      
      if(coords[i] > scroll_y2)
	{
	  coords[i] = scroll_y2 + 1.0;
	}
    }
  
  foo_canvas_item_set(zmap_item->long_item,
		      "points", &points,
		      NULL);

  g_free(coords);

  return ;
}

static void crop_long_item(ZMapWindowLongItem zmap_item)
{
  FooCanvasItem *crop_item;
  FooCanvas *foo_canvas;
  double x1, y1, x2, y2;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2;
  double iwx1, iwy1, iwx2, iwy2;

  iwx1 = x1 = zmap_item->shape.extent_box[0];
  iwy1 = y1 = zmap_item->shape.extent_box[1];
  iwx2 = x2 = zmap_item->shape.extent_box[2];
  iwy2 = y2 = zmap_item->shape.extent_box[3];

  crop_item  = zmap_item->long_item;
  foo_canvas = crop_item->canvas;

  /* x unused ATM */
  scroll_x1 = foo_canvas->scroll_x1;
  scroll_x2 = foo_canvas->scroll_x2;
  
  scroll_y1 = foo_canvas->scroll_y1;
  scroll_y2 = foo_canvas->scroll_y2;
  
  foo_canvas_item_w2i(crop_item, &scroll_x1, &scroll_y1);
  foo_canvas_item_w2i(crop_item, &scroll_x2, &scroll_y2);
      
  if (!(iwy2 < scroll_y1) && !(iwy1 > scroll_y2) && ((iwy1 < scroll_y1) || (iwy2 > scroll_y2)))
    {
      if(iwy1 < scroll_y1)
	{
	  y1 = scroll_y1 - 1.0;
	}
      
      if(iwy2 > scroll_y2)
	{
	  y2 = scroll_y2 + 1.0;
	}

      if(zmap_item->shape.has_points)
	{
	  crop_item_with_points(zmap_item, 
				scroll_x1, scroll_y1,
				scroll_x2, scroll_y2);
	}
      else
	{
	  foo_canvas_item_set(zmap_item->long_item,
			      "x1", x1,
			      "y1", y1,
			      "x2", x2,
			      "y2", y2,
			      NULL);
	}
    }
  
  return ;
}

static void   zmap_window_long_item_update     (FooCanvasItem  *item,
						double            i2w_dx,
						double            i2w_dy,
						int               flags)
{
  ZMapWindowLongItem zmap_item;
  gboolean canvas_in_update = TRUE; /* set to FALSE to disable the monkeying, expect warnings though. */
  gboolean need_update = FALSE;
  int item_flags;


  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(canvas_in_update)		/* actually we might not want to do this. */
    canvas_in_update = item->canvas->doing_update;

  if(canvas_in_update)
    {
      /* Stops the g_return_if_fail(!item->cavnas->doing_update) failing!!! */
      item->canvas->doing_update = FALSE;

      /* monkey with item->object.flags */
      item_flags = item->parent->object.flags;

      /* Stops us getting all the way up the tree to foo_canvas_request_update. */
      if((need_update = 
	  ((item_flags & FOO_CANVAS_ITEM_NEED_UPDATE) == FOO_CANVAS_ITEM_NEED_UPDATE)) == FALSE)
	item->parent->object.flags |= FOO_CANVAS_ITEM_NEED_UPDATE;
    }

  /* Here we _always_ restore the shape of the child item! */
  if(canvas_in_update && zmap_item->shape.has_points)
    {
      /* set the points */
      foo_canvas_item_set(zmap_item->long_item,
			  "points", zmap_item->shape.shape.points,
			  NULL);
    }
  else if(canvas_in_update)
    {
      /* set the x1, x2, y1, y2 */
      foo_canvas_item_set(zmap_item->long_item,
			  "x1" , zmap_item->shape.shape.box_coords[0],
			  "y1" , zmap_item->shape.shape.box_coords[1],
			  "x2" , zmap_item->shape.shape.box_coords[2],
			  "y2" , zmap_item->shape.shape.box_coords[3],
			  NULL);
    }

  if((canvas_in_update == TRUE) && 
     (flags & ZMAP_CANVAS_UPDATE_CROP_REQUIRED))
    {
      /* crop to scroll region */
      crop_long_item(zmap_item);
    }

  if(canvas_in_update)
    {
      /* Need to reset everything */
      item->canvas->doing_update = TRUE;
      item->parent->object.flags |= item_flags;
      if(!need_update)
	item->parent->object.flags &= ~FOO_CANVAS_ITEM_NEED_UPDATE;
    }

  if(zmap_item->long_item_class->update)
    (zmap_item->long_item_class->update)(zmap_item->long_item, i2w_dx, i2w_dy, flags);

  if(parent_class_G->update)
    (parent_class_G->update)(item, i2w_dx, i2w_dy, flags);

  item->x1 = zmap_item->long_item->x1;
  item->y1 = zmap_item->long_item->y1;
  item->x2 = zmap_item->long_item->x2;
  item->y2 = zmap_item->long_item->y2;

  return ;
}

static void zmap_window_long_item_realize (FooCanvasItem *item)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(parent_class_G->realize)
    (parent_class_G->realize)(item);

  /* check flags here... */
  if(zmap_item->long_item_class->realize)
    (zmap_item->long_item_class->realize)(zmap_item->long_item);
  
  return ;
}

static void zmap_window_long_item_unrealize (FooCanvasItem *item)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  /* check flags here... */
  if(zmap_item->long_item_class->unrealize)
    (zmap_item->long_item_class->unrealize)(zmap_item->long_item);

  if(parent_class_G->unrealize)
    (parent_class_G->unrealize)(item);
  
  return ;
}

static void zmap_window_long_item_draw (FooCanvasItem  *item, 
					GdkDrawable    *drawable,
					GdkEventExpose *expose)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(zmap_item->long_item_class->draw)
    (zmap_item->long_item_class->draw)(zmap_item->long_item, drawable, expose);
  
  return ;
}

static double zmap_window_long_item_point (FooCanvasItem *item, double x, double y,
					    int cx, int cy, FooCanvasItem **actual_item)
{
  ZMapWindowLongItem zmap_item;
  double dist = 1.0e36; 

  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(zmap_item->long_item_class->point)
    dist = (zmap_item->long_item_class->point)(zmap_item->long_item, x, y, cx, cy, actual_item);
  
  /* In the case the dist is small_enough, actual_item needs to be the toplevel one. */
  if(actual_item)
    *actual_item = item;

  return dist;
}

static void zmap_window_long_item_translate (FooCanvasItem *item, double dx, double dy)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(zmap_item->long_item_class->translate)
    (zmap_item->long_item_class->translate)(zmap_item->long_item, dx, dy);
  
  return ;
}

static void zmap_window_long_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  ZMapWindowLongItem zmap_item;
  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  if(zmap_item->long_item_class->bounds)
    (zmap_item->long_item_class->bounds)(zmap_item->long_item, x1, y1, x2, y2);

  return ;
}



static void group_remove (FooCanvasGroup *group, FooCanvasItem *item)
{
  GList *children;
  
  g_return_if_fail (FOO_IS_CANVAS_GROUP (group));
  g_return_if_fail (FOO_IS_CANVAS_ITEM (item));
  
  for (children = group->item_list; children; children = children->next)
    {
      if (children->data == item) 
	{
#ifdef NO_NEED_FOR_THIS_HERE
	  if (item->object.flags & FOO_CANVAS_ITEM_MAPPED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unmap) (item);
	  
	  if (item->object.flags & FOO_CANVAS_ITEM_REALIZED)
	    (* FOO_CANVAS_ITEM_GET_CLASS (item)->unrealize) (item);
#endif  
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


static void set_child_item(FooCanvasItem *item, FooCanvasItem *long_item)
{
  ZMapWindowLongItem zmap_item;

  g_return_if_fail(ZMAP_IS_WINDOW_LONG_ITEM(item));
  g_return_if_fail(FOO_IS_CANVAS_ITEM(long_item));

  zmap_item = ZMAP_WINDOW_LONG_ITEM(item);

  g_object_ref(G_OBJECT(long_item));

  zmap_item->long_item       = long_item;
  /* Something somewhere needs long_item->parent != NULL */
  long_item->parent          = item->parent; /* this seems legit? */
 
  zmap_item->long_item_class = FOO_CANVAS_ITEM_GET_CLASS(long_item);

  zmap_item->object_class    = G_OBJECT_GET_CLASS(long_item);

  if(zmap_item->object_class->set_property == NULL)
    {
      while(zmap_item->object_class->set_property == NULL)
	{
	  zmap_item->object_class = 
	    (GObjectClass *)g_type_class_peek_parent(zmap_item->object_class);
	}
    }

  return ;
}
