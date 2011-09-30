/*  File: zmapWindowTextFeature.c
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
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>





#include <math.h>		/* pow(), sqrt() */
#include <zmapWindowTextFeature_I.h>

enum
  {
    PROP_0,
    PROP_TEXT
  };



typedef struct
{
  ZMapWindowCanvasItem parent;
  ZMapStyleColourType  colour_type;
  GdkColor            *default_fill;
  GdkColor            *border;
} EachItemDataStruct, *EachItemData;



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

static void zmap_window_text_feature_set_colour(ZMapWindowCanvasItem   text,
						FooCanvasItem         *interval,
						ZMapFeature			feature,
						ZMapFeatureSubPartSpan sub_feature,
						ZMapStyleColourType    colour_type,
						int colour_flags,
						GdkColor              *default_fill,
                                    GdkColor              *border);
static FooCanvasItem *zmap_window_text_feature_add_interval(ZMapWindowCanvasItem   text,
							    ZMapFeatureSubPartSpan sub_feature,
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

      /* mh17 for a locus feature we want the canvas item to be
       * the size of the displayed test not the extent of the feature
       * This is for a single line short text feature not a sequence that wraps round
       */

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


static void window_text_feature_item_set_colour(ZMapWindowCanvasItem   canvas_item,
						FooCanvasItem         *interval,
						ZMapFeature			feature,
						ZMapFeatureSubPartSpan unused,
						ZMapStyleColourType    colour_type,
						int colour_flags,
                                    GdkColor              *default_fill,
                                    GdkColor              *border)
{
  ZMapFeatureTypeStyle style;
  GdkColor *fill = NULL, *outline = NULL;

  g_return_if_fail(canvas_item != NULL);
  g_return_if_fail(interval    != NULL);

  if((style = (ZMAP_CANVAS_ITEM_GET_CLASS(canvas_item)->get_style)(canvas_item)))
    {
      zMapStyleGetColours(style, STYLE_PROP_COLOURS, colour_type,
			  &fill, NULL, &outline);
    }

  if(colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
  {
      if(default_fill)
            fill = default_fill;
      if(border)
            outline = border;
  }

  if(FOO_IS_CANVAS_LINE(interval)
     || FOO_IS_CANVAS_LINE_GLYPH(interval)
     || FOO_IS_CANVAS_TEXT(interval))
    {
      foo_canvas_item_set(interval,
			  "fill_color_gdk", fill,
			  NULL);
    }
  else if (FOO_IS_CANVAS_RE(interval) || FOO_IS_CANVAS_POLYGON(interval))
    {
      if (!outline)
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


static void zmap_window_text_feature_set_colour(ZMapWindowCanvasItem   text_item,
						FooCanvasItem         *interval,
						ZMapFeature			feature,
						ZMapFeatureSubPartSpan sub_feature,
						ZMapStyleColourType    colour_type,
						int colour_flags,
						GdkColor              *default_fill,
                                    GdkColor              *border)
{
  ZMapFeatureTypeStyle style;

  GdkColor *select_fill = NULL, *select_draw = NULL, *select_border = NULL ;
  GdkColor *normal_fill = NULL, *normal_draw = NULL, *normal_border = NULL ;

  GdkColor *text = NULL, *text_background = NULL ;
  GdkColor black, white;


  /* ALL THIS COLOUR DEFAULTING SHOULD BE DONE IN THE PREDEFINED FUNC. FOR PREDEFINED STYLES
   * AND IN THE MAKEDRAWABLE FUNC FOR OTHER COLS....NOT SCATTERED AROUND LIKE THIS..... */


  if (FOO_IS_CANVAS_TEXT(interval))
    {
      gdk_color_parse("black", &black);
      gdk_color_parse("white", &white);

      if ((style = (ZMAP_CANVAS_ITEM_GET_CLASS(text_item)->get_style)(text_item)))
	{

	  if (!zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
				   &select_fill, &select_draw, &select_border))
	    {
	      /* This should come from the style... SO WHY DO IT THEN....?????? */
	      select_fill    = &black;
	      select_border = &white;
	    }

	  if (!zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				   &normal_fill, &normal_draw, &normal_border))
	    {
	      normal_fill    = &white;
	      normal_border = &black;
	    }
	}

      if (colour_type == ZMAPSTYLE_COLOURTYPE_SELECTED)
	{
	  if (default_fill)
	    {
	      select_fill = default_fill;
	      select_border = &black;
	    }

	  text = select_border;
	  text_background = select_fill;
	}
      else
	{
	  text = normal_border;
	  text_background = NULL;	   /* This makes an empty box. (No highlight) */
	}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* protect from color similarity... */
      if (text_background)
	{
	  double distance;
	  double red, green, blue;
	  red   = text->red   - text_background->red;
	  green = text->green - text_background->green;
	  blue  = text->blue  - text_background->blue;

	  red   = pow(red, 2);
	  green = pow(green, 2);
	  blue  = pow(blue, 2);

	  distance = sqrt(red + green + blue);

	  if(distance < 5000.0)
	    {
	      /* g_warning("colours too similar, inverting..."); */
	      text->red   = 65535 - text_background->red;
	      text->green = 65535 - text_background->green;
	      text->blue  = 65535 - text_background->blue;
	    }
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      foo_canvas_item_set(interval,
			  "fill_color_gdk", text,
			  NULL);

    }
  else
    {
      window_text_feature_item_set_colour(text_item, interval, feature, sub_feature, colour_type, colour_flags, default_fill,border) ;
    }

  return ;
}

static FooCanvasItem *zmap_window_text_feature_add_interval(ZMapWindowCanvasItem   text,
							    ZMapFeatureSubPartSpan sub_feature,
							    double top,  double bottom,
							    double left, double right)
{
  ZMapFeatureTypeStyle style;
  FooCanvasItem *item = NULL;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *font_name = "Lucida Console";
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  char *font_name = "monotype" ;


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

  return item;
}

