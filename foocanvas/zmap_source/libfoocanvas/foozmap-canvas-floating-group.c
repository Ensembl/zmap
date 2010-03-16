/*  File: foozmap-canvas-floating-group.c
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
 * Last edited: Mar 23 16:47 2008 (rds)
 * Created: Thu Jan 24 08:36:25 2008 (rds)
 * CVS info:   $Id: foozmap-canvas-floating-group.c 1.2 2010-03-12 18:21:36 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include "foozmap-canvas-floating-group.h"

enum
  {
    PROP_0,
    PROP_FLOAT_MIN_X,
    PROP_FLOAT_MAX_X,
    PROP_FLOAT_MIN_Y,
    PROP_FLOAT_MAX_Y,
    PROP_FLOAT_AXIS
  };

static void foo_canvas_float_group_class_init (FooCanvasFloatGroupClass *class);
static void foo_canvas_float_group_init (FooCanvasFloatGroup *group);
static void foo_canvas_float_group_set_property (GObject *gobject, guint param_id,
						 const GValue *value, GParamSpec *pspec);
static void foo_canvas_float_group_get_property(GObject *gobject, guint param_id,
						GValue *value, GParamSpec *pspec);
static void foo_canvas_float_group_destroy(GtkObject *object);
static void foo_canvas_float_group_realize (FooCanvasItem *item);
static void foo_canvas_float_group_draw(FooCanvasItem  *item, 
					GdkDrawable    *drawable,
					GdkEventExpose *expose);
static void foo_canvas_float_group_update(FooCanvasItem *item, 
					  double i2w_dx, 
					  double i2w_dy, 
					  int flags);
static void save_scroll_region(FooCanvasItem *item);
static GType float_group_axis_get_type (void);


static FooCanvasItemClass *parent_class_G;

/**
 * foo_canvas_float_group_get_type:
 *
 * Registers the &FooCanvasGroup class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value:  The type ID of the &FooCanvasGroup class.
 **/
GType
foo_canvas_float_group_get_type (void)
{
  static GType group_type = 0;
  
  if (!group_type) {
    static const GTypeInfo group_info = {
      sizeof (FooCanvasFloatGroupClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) foo_canvas_float_group_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (FooCanvasFloatGroup),
      0,              /* n_preallocs */
      (GInstanceInitFunc) foo_canvas_float_group_init
      
      
    };
    
    group_type = g_type_register_static (foo_canvas_group_get_type (),
					 "FooCanvasFloatGroup",
					 &group_info,
					 0);
  }
  
  return group_type;
}

/* Class initialization function for FooCanvasGroupClass */
static void foo_canvas_float_group_class_init (FooCanvasFloatGroupClass *class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;
  
  gobject_class = (GObjectClass *) class;
  object_class  = (GtkObjectClass *) class;
  item_class    = (FooCanvasItemClass *) class;
  
  parent_class_G  = gtk_type_class (foo_canvas_group_get_type ());
  
  gobject_class->set_property = foo_canvas_float_group_set_property;
  gobject_class->get_property = foo_canvas_float_group_get_property;
  
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_X,
				  g_param_spec_double("min-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_X,
				  g_param_spec_double("max-x",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MIN_Y,
				  g_param_spec_double("min-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_MAX_Y,
				  g_param_spec_double("max-y",
						      NULL, NULL,
						      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FLOAT_AXIS,
				  g_param_spec_enum ("float-axis", NULL, NULL,
						     float_group_axis_get_type(), 
						     ZMAP_FLOAT_AXIS_Y,
						     G_PARAM_READWRITE));


  object_class->destroy = foo_canvas_float_group_destroy;
  
  item_class->draw    = foo_canvas_float_group_draw;
  item_class->update  = foo_canvas_float_group_update;
  item_class->realize = foo_canvas_float_group_realize;
  
  return ;
}

/* Object initialization function for FooCanvasGroup */
static void foo_canvas_float_group_init (FooCanvasFloatGroup *group)
{
  FooCanvasItem *item;

  item = FOO_CANVAS_ITEM(group);

  group->zoom_x = 0.0;
  group->zoom_y = 0.0;
  group->scr_x1 = 0.0;
  group->scr_y1 = 0.0;
  group->scr_x2 = 0.0;
  group->scr_y2 = 0.0;

  if(item->canvas)
    {
      group->zoom_x = item->canvas->pixels_per_unit_x;
      group->zoom_y = item->canvas->pixels_per_unit_y;
    }

  /* y axis only! */
  group->float_axis = ZMAP_FLOAT_AXIS_Y;

  return ;
}

/* Set_property handler for canvas groups */
static void foo_canvas_float_group_set_property (GObject *gobject, guint param_id,
						 const GValue *value, GParamSpec *pspec)
{
  FooCanvasFloatGroup *group;
  double d;

  group = FOO_CANVAS_FLOAT_GROUP(gobject);

  switch (param_id) 
    {
    case PROP_FLOAT_MIN_X:
      group->scr_x1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(group->scr_x1), &d);
      group->min_x_set = TRUE;
      break;
    case PROP_FLOAT_MAX_X:
      group->scr_x2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &(group->scr_x2), &d);
      group->max_x_set = TRUE;
      break;
    case PROP_FLOAT_MIN_Y:
      group->scr_y1 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(group->scr_y1));
      group->min_y_set = TRUE;
      break;
    case PROP_FLOAT_MAX_Y:
      group->scr_y2 = g_value_get_double (value);
      foo_canvas_item_i2w(FOO_CANVAS_ITEM(gobject), &d, &(group->scr_y2));
      group->max_y_set = TRUE;
      break;
    case PROP_FLOAT_AXIS:
      {
	int axis = g_value_get_enum(value);
	group->float_axis = axis;
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }

  return ;
}

/* Get_property handler for canvas groups */
static void foo_canvas_float_group_get_property(GObject *gobject, guint param_id,
						GValue *value, GParamSpec *pspec)
{
  switch (param_id) 
    {
    case PROP_FLOAT_MIN_X:
      break;
    case PROP_FLOAT_MAX_X:
      break;
    case PROP_FLOAT_MIN_Y:
      break;
    case PROP_FLOAT_MAX_Y:
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, param_id, pspec);
      break;
    }

  return ;
}


static void foo_canvas_float_group_destroy(GtkObject *object)
{

  if(GTK_OBJECT_CLASS(parent_class_G)->destroy)
    (* GTK_OBJECT_CLASS(parent_class_G)->destroy)(object);
  return ;
}

static void foo_canvas_float_group_realize (FooCanvasItem *item)
{
  FooCanvasFloatGroup *group;

  group = FOO_CANVAS_FLOAT_GROUP(item);

  if(item->canvas)
    {
      group->zoom_x = item->canvas->pixels_per_unit_x;
      group->zoom_y = item->canvas->pixels_per_unit_y;
      save_scroll_region(item);
    }
  
  if (parent_class_G->realize)
    (* parent_class_G->realize) (item);
}

/* Draw handler for canvas groups */
static void foo_canvas_float_group_draw(FooCanvasItem  *item, 
					GdkDrawable    *drawable,
					GdkEventExpose *expose)
{
  FooCanvasFloatGroup *floating;
  FooCanvasGroup *group;
  double x1, x2, y1, y2, xpos, ypos;

  group = FOO_CANVAS_GROUP (item);
  floating = FOO_CANVAS_FLOAT_GROUP(item);

  /* If the group x,y is outside the scroll region, move it back in! */
  foo_canvas_get_scroll_region(item->canvas, &x1, &y1, &x2, &y2);
  xpos = group->xpos;
  ypos = group->ypos;
  /* convert x,y position to world coord space */
  foo_canvas_item_i2w(item, &xpos, &ypos);

  /* round down to whole bases... We need to do this in a few places! */
  x1 = (double)((int)x1);
  y1 = (double)((int)y1);

  /* conditionally update the x,y position of the group */
  if((floating->float_axis & ZMAP_FLOAT_AXIS_X) && (xpos != x1))
    xpos = ((x1 > floating->scr_x1) ? 
	    (x1) :
	    (double)((int)(floating->scr_x1)));
  if((floating->float_axis & ZMAP_FLOAT_AXIS_Y) && (ypos != y1))
    ypos = ((y1 > floating->scr_y1) ? 
	    (y1) :
	    (double)((int)(floating->scr_y1)));

  /* convert back to item coord space */
  foo_canvas_item_w2i(item, &xpos, &ypos);

  /* actually move the group [if necessary] */
  if(xpos != group->xpos || ypos != group->ypos)
    {
      /* round down.  If no floating is happening... */
      xpos = (double)((int)xpos);
      ypos = (double)((int)ypos);

      g_object_set(G_OBJECT(item), "x", xpos, "y", ypos, NULL);
    }

  /* parent->draw? */
  if(parent_class_G->draw)
    (*parent_class_G->draw)(item, drawable, expose);

  return ;
}

static void foo_canvas_float_group_update(FooCanvasItem *item, 
					  double i2w_dx, 
					  double i2w_dy, 
					  int flags)
{
  FooCanvasFloatGroup *group;
  gboolean force_intersect = TRUE;

  group = FOO_CANVAS_FLOAT_GROUP(item);

  if(parent_class_G->update)
    (*parent_class_G->update)(item, i2w_dx, i2w_dy, flags);

  if(force_intersect && (item->object.flags & FOO_CANVAS_ITEM_VISIBLE))
    {
      int cx1, cx2, cy1, cy2;

      foo_canvas_w2c(item->canvas, group->scr_x1, group->scr_y1, &cx1, &cy1);
      foo_canvas_w2c(item->canvas, group->scr_x2, group->scr_y2, &cx2, &cy2);

      /* These must be set in order to make the group intersect with any
       * rectangle within the whole of the scroll region */
      if(group->float_axis & ZMAP_FLOAT_AXIS_X)
	{
	  item->x1 = cx1; //group->scr_x1;
	  item->x2 = cx2; //group->scr_x2;
	}
      if(group->float_axis & ZMAP_FLOAT_AXIS_Y)
	{
	  item->y1 = cy1; //group->scr_y1;
	  item->y2 = cy2; //group->scr_y2;
	}
    }

  /* unfortunately we need to run through this again :o( */
  if(item->object.flags & FOO_CANVAS_ITEM_VISIBLE)
    { 
      FooCanvasGroup *real_group = FOO_CANVAS_GROUP(item);
      FooCanvasItem  *i;
      GList *list;
      for (list = real_group->item_list; list; list = list->next)
	{
	  i = list->data;
	  if(FOO_CANVAS_ITEM_GET_CLASS(i)->update)
	    FOO_CANVAS_ITEM_GET_CLASS(i)->update(i, i2w_dx, i2w_dy, flags);
	  item->x1 = MIN(item->x1, i->x1);
	  item->y1 = MIN(item->y1, i->y1);
	  item->x2 = MAX(item->x2, i->x2);
	  item->y2 = MAX(item->y2, i->y2);
	}
    }

  return ;
}

static void save_scroll_region(FooCanvasItem *item)
{
  FooCanvasFloatGroup *group;
  double x1, y1, x2, y2;
  double *x1_ptr, *y1_ptr, *x2_ptr, *y2_ptr;

  group = FOO_CANVAS_FLOAT_GROUP(item);

  foo_canvas_get_scroll_region(item->canvas, &x1, &y1, &x2, &y2);
  
  x1_ptr = &(group->scr_x1);
  y1_ptr = &(group->scr_y1);
  x2_ptr = &(group->scr_x2);
  y2_ptr = &(group->scr_y2);

  if(item->canvas->pixels_per_unit_x <= group->zoom_x)
    {
      if(!group->min_x_set)
	group->scr_x1 = x1;
      else
	x1_ptr = &x1;
      if(!group->max_x_set)
	group->scr_x2 = x2;
      else
	x2_ptr = &x2;
    }

  if(item->canvas->pixels_per_unit_y <= group->zoom_y)
    {
      if(!group->min_y_set)
	group->scr_y1 = y1;
      else
	y1_ptr = &y1;
      if(!group->max_y_set)
	group->scr_y2 = y2;
      else
	y2_ptr = &y2;
    }
  
  foo_canvas_item_w2i(item, x1_ptr, y1_ptr);
  foo_canvas_item_w2i(item, x2_ptr, y2_ptr);

  return ;
}


static GType float_group_axis_get_type (void)
{
  static GType etype = 0;
  if (etype == 0) {
    static const GEnumValue values[] = {
      { ZMAP_FLOAT_AXIS_X, "ZMAP_FLOAT_AXIS_X", "x-axis" },
      { ZMAP_FLOAT_AXIS_Y, "ZMAP_FLOAT_AXIS_Y", "y-axis" },
      { 0, NULL, NULL }
    };
    etype = g_enum_register_static ("FooCanvasFloatGroupAxis", values);
  }
  return etype;
}
