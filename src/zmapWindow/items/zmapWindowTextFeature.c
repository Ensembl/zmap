/*  File: zmapWindowTextFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Jun 11 15:21 2009 (rds)
 * Created: Tue Jan 13 13:41:57 2009 (rds)
 * CVS info:   $Id: zmapWindowTextFeature.c,v 1.5 2009-06-11 14:21:48 rds Exp $
 *-------------------------------------------------------------------
 */
#include <math.h>		/* pow(), sqrt() */
#include <zmapWindowTextFeature_I.h>

enum
  {
    PROP_0,
    PROP_TEXT
  };

static void zmap_window_text_feature_class_init  (ZMapWindowTextFeatureClass text_class);
static void zmap_window_text_feature_init        (ZMapWindowTextFeature      text);
static void zmap_window_text_feature_set_property(GObject               *object, 
						  guint                  param_id,
						  const GValue          *value,
						  GParamSpec            *pspec);
static void zmap_window_text_feature_get_property(GObject               *object,
						  guint                  param_id,
						  GValue                *value,
						  GParamSpec            *pspec);
#ifdef UPDATE_REQUIRED
static void zmap_window_text_feature_update      (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags);
#endif /* UPDATE_REQUIRED */

#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_text_feature_destroy     (GObject *object);
#endif /* EXTRA_DATA_NEEDS_FREE */

static void zmap_window_text_feature_set_colour(ZMapWindowCanvasItem  text,
						FooCanvasItem        *interval,
						ZMapWindowItemFeature sub_feature,
						ZMapStyleColourType   colour_type,
						GdkColor             *default_fill);
static FooCanvasItem *zmap_window_text_feature_add_interval(ZMapWindowCanvasItem  text,
							    ZMapWindowItemFeature sub_feature,
							    double top,  double bottom,
							    double left, double right);


static FooCanvasItemClass *canvas_parent_class_G;
static FooCanvasItemClass *group_parent_class_G;

GType zMapWindowTextFeatureGetType(void)
{
  static GType group_type = 0;
  
  if (!group_type) {
    static const GTypeInfo group_info = {
      sizeof (zmapWindowTextFeatureClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_text_feature_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof (zmapWindowTextFeature),
      0,              /* n_preallocs */
      (GInstanceInitFunc) zmap_window_text_feature_init      
    };
    
    group_type = g_type_register_static (zMapWindowCanvasItemGetType(),
					 ZMAP_WINDOW_TEXT_FEATURE_NAME,
					 &group_info,
					 0);
  }
  
  return group_type;
}


static void zmap_window_text_feature_class_init  (ZMapWindowTextFeatureClass text_class)
{
  ZMapWindowCanvasItemClass canvas_class;
  FooCanvasItemClass *item_class;
  FooCanvasItemClass *text_item_class;
  GObjectClass *gobject_class;
  GType canvas_item_type, parent_type;
  
  gobject_class = (GObjectClass *) text_class;
  canvas_class  = (ZMapWindowCanvasItemClass)text_class;
  text_item_class = (FooCanvasItemClass *)text_class;

  canvas_item_type = g_type_from_name(ZMAP_WINDOW_TEXT_FEATURE_NAME);
  parent_type      = g_type_parent(canvas_item_type);

  canvas_parent_class_G = gtk_type_class(parent_type);

  gobject_class->set_property = zmap_window_text_feature_set_property;
  gobject_class->get_property = zmap_window_text_feature_get_property;
  
  group_parent_class_G = item_class = g_type_class_peek (foo_canvas_group_get_type());

  /* Hmm, why did I think we wanted to restore this???? zmap_window_canvas_item_bounds seems to work! */
#ifdef RDS_DONT_INCLUDE_UNUSED
  text_item_class->bounds    = item_class->bounds; /* restore bounds from foo_canvas_group */
#endif /* RDS_DONT_INCLUDE_UNUSED */

  g_object_class_install_property(gobject_class,
				  PROP_TEXT,
				  g_param_spec_string ("text",
						       "Text",
						       "Text to render",
						       NULL,
						       (G_PARAM_READABLE)));

  g_object_class_override_property(gobject_class, 1,
				   ZMAP_WINDOW_CANVAS_INTERVAL_TYPE);


  canvas_class->add_interval = zmap_window_text_feature_add_interval;
  canvas_class->set_colour   = zmap_window_text_feature_set_colour;

  return ;
}

static void zmap_window_text_feature_init        (ZMapWindowTextFeature      text)
{
  ZMapWindowCanvasItem canvas_item;

  canvas_item = ZMAP_CANVAS_ITEM(text);

  canvas_item->auto_resize_background = 1;
  return ;
}

static void zmap_window_text_feature_set_property(GObject               *object, 
						  guint                  param_id,
						  const GValue          *value,
						  GParamSpec            *pspec)
{
  return ;
}

static void zmap_window_text_feature_get_property(GObject               *object,
						  guint                  param_id,
						  GValue                *value,
						  GParamSpec            *pspec)
{
  switch(param_id)
    {
    case PROP_TEXT:
      {
	GList *item_list;
	char *text = NULL;

	item_list = FOO_CANVAS_GROUP(object)->item_list;

	while(text == NULL && item_list)
	  {
	    FooCanvasItem *item = FOO_CANVAS_ITEM(item_list->data);

	    if(FOO_IS_CANVAS_TEXT(item))
	      g_object_get(G_OBJECT(item),
			   "text", &text,
			   NULL);

	    item_list = item_list->next;
	  }

	g_value_set_string(value, text);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
    }

  return ;
}
#ifdef UPDATE_REQUIRED
static void zmap_window_text_feature_update      (FooCanvasItem *item,
						  double i2w_dx, double i2w_dy, 
						  int flags)
{
  return ;
}
#endif /* UPDATE_REQUIRED */

#ifdef EXTRA_DATA_NEEDS_FREE
static void zmap_window_text_feature_destroy     (GObject *object)
{
  return ;
}
#endif /* EXTRA_DATA_NEEDS_FREE */

typedef struct
{
  ZMapWindowCanvasItem parent;
  ZMapStyleColourType  colour_type;
  GdkColor            *default_fill;
} EachItemDataStruct, *EachItemData;

static void window_text_feature_item_set_colour(ZMapWindowCanvasItem  canvas_item,
						FooCanvasItem        *interval,
						ZMapWindowItemFeature unused,
						ZMapStyleColourType   colour_type,
						GdkColor             *default_fill)
{
  ZMapFeatureTypeStyle style;
  GdkColor *fill = NULL, *outline = NULL;

  g_return_if_fail(canvas_item != NULL);
  g_return_if_fail(interval    != NULL);

  if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->get_style)(canvas_item)))
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
  else if(FOO_IS_CANVAS_RE(interval) ||
	  FOO_IS_CANVAS_POLYGON(interval))
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

static void invoke_item_set_colour(gpointer item_data, gpointer user_data)
{
  FooCanvasItem *interval = FOO_CANVAS_ITEM(item_data);
  EachItemData each_item_data = (EachItemData)user_data;

  window_text_feature_item_set_colour(each_item_data->parent, interval, NULL, 
				      each_item_data->colour_type,
				      each_item_data->default_fill);

  return ;
}

static void zmap_window_text_feature_set_colour(ZMapWindowCanvasItem  text,
						FooCanvasItem        *interval,
						ZMapWindowItemFeature sub_feature,
						ZMapStyleColourType   colour_type,
						GdkColor             *default_fill)
{
  ZMapFeatureTypeStyle style;
  FooCanvasItem *bg_item, *underlay;
  GdkColor *normal_outline, *select_outline;
  GdkColor *text_fill, *back_fill;
  GdkColor *normal_fill;
  GdkColor *select_fill;
  GdkColor black, white;

  if(FOO_IS_CANVAS_TEXT(interval))
    {
      gdk_color_parse("black", &black);
      gdk_color_parse("white", &white);
      
      if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(text)->get_style)(text)))
	{
	  
	  if(!zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  &select_fill, NULL, &select_outline))
	    {
	      /* This should come from the style... */
	      select_fill    = &black;
	      select_outline = &white;
	    }
	  
	  if(!zMapStyleGetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  &normal_fill, NULL, &normal_outline))
	    {
	      normal_fill    = &white;
	      normal_outline = &black;
	    }
	}
      
      if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
	{
	  if(default_fill)
	    {
	      select_fill    = default_fill;
	      select_outline = &black;
	    }
	  
	  text_fill = select_outline;
	  back_fill = select_fill;
	}
      else
	{
	  text_fill = normal_outline;
	  back_fill = NULL;	   /* This makes an empty box. (No highlight) */
	}
      
      /* protect from color similarity... */
      if(back_fill)			
	{
	  double distance;
	  double red, green, blue;
	  red   = text_fill->red   - back_fill->red;
	  green = text_fill->green - back_fill->green;
	  blue  = text_fill->blue  - back_fill->blue;
	  
	  red   = pow(red, 2);
	  green = pow(green, 2);
	  blue  = pow(blue, 2);
	  
	  distance = sqrt(red + green + blue);
	  
	  if(distance < 5000.0)
	    {
	      /* g_warning("colours too similar, inverting..."); */
	      text_fill->red   = 65535 - back_fill->red;
	      text_fill->green = 65535 - back_fill->green;
	      text_fill->blue  = 65535 - back_fill->blue;
	    }
	}
      
      
      foo_canvas_item_set(interval,
			  "fill_color_gdk", text_fill,
			  NULL);
      
      if((bg_item = text->items[WINDOW_ITEM_BACKGROUND]))
	{
	  foo_canvas_item_set(bg_item,
			      "fill_color_gdk", back_fill,
			      NULL);
	}
    }
  else
    window_text_feature_item_set_colour(text, interval, sub_feature, colour_type, default_fill);

  if((underlay = text->items[WINDOW_ITEM_UNDERLAY]))
    {
      EachItemDataStruct each_item_data = {};
      GdkColor red;

      each_item_data.parent       = text;
      each_item_data.colour_type  = colour_type;
      each_item_data.default_fill = default_fill;

      if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED && default_fill)
	{
	  gdk_color_parse("red", &red);
	  each_item_data.default_fill = &red;
	}

      g_list_foreach(FOO_CANVAS_GROUP(underlay)->item_list, 
		     invoke_item_set_colour, &each_item_data);
    }


  return ;
}

static FooCanvasItem *zmap_window_text_feature_add_interval(ZMapWindowCanvasItem  text,
							    ZMapWindowItemFeature sub_feature,
							    double top,  double bottom,
							    double left, double right)
{
  ZMapFeatureTypeStyle style;
  FooCanvasItem *item = NULL;
  char *font_name = "Lucida Console";
  gboolean mark_real_extent = FALSE;

  if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(text)->get_style)(text)))
    {
      char *tmp_font;
      g_object_get(G_OBJECT(style),
		   "text-font", &tmp_font,
		   NULL);
      if(tmp_font != NULL)
	font_name = tmp_font;
    }

  /* Font should come from the style! */
  item = foo_canvas_item_new(FOO_CANVAS_GROUP(text),
			     foo_canvas_text_get_type(),
			     "font",   font_name,
			     "anchor", GTK_ANCHOR_NW,
			     NULL);

  if(mark_real_extent)
    {
      FooCanvasPoints points;
      double coords[8];
      int line_width = 1;

      left  = 0.0;
      right = 5.0;

      coords[0] = right;
      coords[1] = top;

      coords[2] = left;
      coords[3] = top;
      
      coords[4] = left;
      coords[5] = bottom;

      coords[6] = right;
      coords[7] = bottom;

      points.coords     = &coords[0];
      points.num_points = 4;
      points.ref_count  = 1;

      foo_canvas_item_new(FOO_CANVAS_GROUP(text->items[WINDOW_ITEM_UNDERLAY]),
			  foo_canvas_line_get_type(),
			  "width_pixels", line_width,
			  "points",       &points,
			  "join_style",   GDK_JOIN_BEVEL,
			  "cap_style",    GDK_CAP_BUTT,
			  NULL);
    }

  return item;
}

