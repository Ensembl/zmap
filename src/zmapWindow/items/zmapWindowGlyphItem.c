/*  File: zmapWindowGlyphItem.c
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
 * Last edited: Jun  3 09:51 2009 (rds)
 * Created: Fri Jan 16 11:20:07 2009 (rds)
 * CVS info:   $Id: zmapWindowGlyphItem.c,v 1.9 2010-04-12 08:40:43 mh17 Exp $
 *-------------------------------------------------------------------
 */


#include <math.h>
#include <string.h>
#include <zmapWindowGlyphItem_I.h>
#include <ZMap/zmapUtilsLog.h>

/* Object argument IDs */
enum
  {
    PROP_0,
    PROP_ORIGIN_X_PIXELS,
    PROP_ORIGIN_Y_PIXELS,
    PROP_WIDTH_PIXELS,
    PROP_HEIGHT_PIXELS,
    PROP_GLYPH_SHAPE,
    PROP_FILL_COLOR,
    PROP_FILL_COLOR_GDK,
    PROP_FILL_COLOR_RGBA,
    PROP_OUTLINE_COLOR,
    PROP_OUTLINE_COLOR_GDK,
    PROP_OUTLINE_COLOR_RGBA,
    PROP_CAP_STYLE,
    PROP_JOIN_STYLE,
    PROP_LINE_STYLE,
    PROP_LINE_WIDTH,
  };


static void zmap_window_glyph_item_class_init   (ZMapWindowGlyphItemClass class);
static void zmap_window_glyph_item_init         (ZMapWindowGlyphItem glyph);
static void zmap_window_glyph_item_destroy      (GtkObject *object);
static void zmap_window_glyph_item_set_property (GObject            *object,
						 guint               param_id,
						 const GValue       *value,
						 GParamSpec         *pspec);
static void zmap_window_glyph_item_get_property (GObject            *object,
						 guint               param_id,
						 GValue             *value,
						 GParamSpec         *pspec);

static void   zmap_window_glyph_item_update     (FooCanvasItem  *item,
						 double            i2w_dx,
						 double            i2w_dy,
						 int               flags);
static void   zmap_window_glyph_item_realize    (FooCanvasItem  *item);
static void   zmap_window_glyph_item_unrealize  (FooCanvasItem  *item);
static void   zmap_window_glyph_item_draw       (FooCanvasItem  *item,
						 GdkDrawable      *drawable,
						 GdkEventExpose   *expose);
static double zmap_window_glyph_item_point      (FooCanvasItem  *item,
						 double            x,
						 double            y,
						 int               cx,
						 int               cy,
						 FooCanvasItem **actual_item);
static void   zmap_window_glyph_item_translate  (FooCanvasItem  *item,
						 double            dx,
						 double            dy);
static void   zmap_window_glyph_item_bounds     (FooCanvasItem  *item,
						 double           *x1,
						 double           *y1,
						 double           *x2,
						 double           *y2);

static FooCanvasItemClass *parent_class;


GType zMapWindowGlyphItemGetType (void)
{
  static GType type = 0;

  if (!type) {
    /* FIXME: Convert to gobject style.  */
    static const GTypeInfo type_info = {
      sizeof(zmapWindowGlyphItemClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_window_glyph_item_class_init,
      NULL,
      NULL,
      sizeof(zmapWindowGlyphItem),
      0,
      (GInstanceInitFunc)zmap_window_glyph_item_init,
      NULL
    };

    type = g_type_register_static(foo_canvas_item_get_type(),
				  ZMAP_WINDOW_GLYPH_ITEM_NAME,
				  &type_info, 0);
  }

  return type;
}

int zmapWindowIsGlyphItem(FooCanvasItem *foo)
{
      return(ZMAP_IS_WINDOW_GLYPH_ITEM(foo));
}


// add a glyph to the canvas... most info is in the style, so this function does the work instead of the many callers
// score can trigger a change in size or colour depending on config
ZMapWindowGlyphItem zMapWindowGlyphItemCreate(FooCanvasGroup *parent,
      ZMapFeatureTypeStyle style, int which,
      double x_coord, double y_coord, double score)
{
  ZMapWindowGlyphItem glyph = NULL;
  ZMapStyleGlyphShape shape;
  GdkColor *draw = NULL, *fill = NULL, *border = NULL;

  shape =   (which == 5) ? zMapStyleGlyphShape5(style) :
            (which == 3) ? zMapStyleGlyphShape3(style) :
            zMapStyleGlyphShape(style);

  if(shape && shape->type != GLYPH_DRAW_INVALID && shape->n_coords)
    {
      // if the glyph is a real feature it will be coloured later by the canvas item code and can be frame specific
      zMapStyleGetColours(style, STYLE_PROP_GLYPH_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                      &fill, &draw, &border);

      glyph = ZMAP_WINDOW_GLYPH_ITEM (foo_canvas_item_new(parent, ZMAP_TYPE_WINDOW_GLYPH_ITEM,
                        "x",           x_coord,
                        "y",           y_coord,
                        "glyph-shape", shape,
                        "fill-color-gdk",fill,
                        "outline-color-gdk",border,
                        // colours set via zmap_window_canvas_item_set_colour()
                        // line styles as default
                        NULL));
    }
  else
    {
      zMapLogWarning("Cannot draw invalid glyph in Style %s",zMapStyleGetName(style));
      return(NULL);
    }
  return (glyph);
}


static void glyph_set_gc_line_attributes (ZMapWindowGlyphItem glyph)
{
  GdkGC *gc;

  gc = glyph->line_gc;

  if(gc)
    {
      gdk_gc_set_line_attributes (gc,
				  glyph->line_width,
				  glyph->line_style,
				  glyph->cap,
				  glyph->join);
    }

  return ;
}

static void glyph_set_gc_line_color (ZMapWindowGlyphItem glyph)
{
  GdkGC *gc;

  gc = glyph->line_gc;

  if(gc)
    {
      GdkColor c;

      c.pixel = glyph->line_pixel;

      gdk_gc_set_foreground (gc, &c);
      gdk_gc_set_fill (gc, GDK_SOLID);
    }

  return ;
}

static void glyph_set_gc_area_color (ZMapWindowGlyphItem glyph)
{
  GdkGC *gc;

  gc = glyph->area_gc;

  if(gc)
    {
      GdkColor c;

      c.pixel = glyph->area_pixel;

      gdk_gc_set_foreground (gc, &c);
      gdk_gc_set_fill (gc, GDK_SOLID);
    }

  return ;
}

static guint32 glyph_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
	  (color->green & 0xff00) << 8 |
	  (color->blue & 0xff00)       |
	  0xff);

  return rgba;
}

static void item_to_canvas(FooCanvas *canvas, double *coords, GdkPoint *points, int count,
			   double i2w_dx, double i2w_dy)
{
  int i, cx, cy;

  foo_canvas_w2c(canvas, i2w_dx, i2w_dy, &cx, &cy);

  for(i = 0; i < count; i++, points++)
    {
      points->x = coords[i*2]   + cx;
      points->y = coords[i*2+1] + cy;
    }

  return ;
}

static gboolean glyph_set_color_property(ZMapWindowGlyphItem glyph_item, guint param_id, const GValue *value,
					 guint32 *rgba_out, gulong *pixel_out)
{
  FooCanvasItem *item;
  GdkColor  color = {0,0,0,0};
  GdkColor *pcolor;
  guint32 rgba;
  gulong pixel;
  gboolean set = FALSE;

  item = FOO_CANVAS_ITEM(glyph_item);

  switch (param_id)
    {
    case PROP_FILL_COLOR:
    case PROP_OUTLINE_COLOR:
      {
	const char *color_spec;
	color_spec = g_value_get_string(value);

	if (color_spec != NULL && gdk_color_parse(color_spec, &color))
	  set = TRUE;
	else
	  set = FALSE;

	rgba  = glyph_color_to_rgba(&color);
	pixel = foo_canvas_get_color_pixel(item->canvas, rgba);
      }
      break;
    case PROP_FILL_COLOR_GDK:
    case PROP_OUTLINE_COLOR_GDK:
      {
	pcolor = g_value_get_boxed (value);

	if (pcolor)
	  {
	    set   = TRUE;
          rgba = glyph_color_to_rgba(pcolor);
          pixel = foo_canvas_get_color_pixel(item->canvas, rgba);
	  }
	else
	  set = FALSE;


      }
      break;
    case PROP_FILL_COLOR_RGBA:
    case PROP_OUTLINE_COLOR_RGBA:
      set   = TRUE;
      rgba  = g_value_get_uint (value);
      pixel = foo_canvas_get_color_pixel(item->canvas, rgba);
      break;
    default:
      break;
    } /* switch(param_id) */

  if(rgba_out)
    *rgba_out = rgba;
  if(pixel_out)
    *pixel_out = pixel;

  return set;
}


/* Class initialization function for the glyph item */
static void
zmap_window_glyph_item_class_init (ZMapWindowGlyphItemClass class)
{
  GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  FooCanvasItemClass *item_class;

  gobject_class = (GObjectClass *) class;
  object_class = (GtkObjectClass *) class;
  item_class = (FooCanvasItemClass *) class;

  parent_class = gtk_type_class (foo_canvas_item_get_type ());

  gobject_class->set_property = zmap_window_glyph_item_set_property;
  gobject_class->get_property = zmap_window_glyph_item_get_property;


  g_object_class_install_property(gobject_class,
				  PROP_ORIGIN_X_PIXELS,
				  g_param_spec_double ("x", NULL, NULL,
						       -G_MAXDOUBLE, G_MAXDOUBLE, 0,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_ORIGIN_Y_PIXELS,
				  g_param_spec_double ("y", NULL, NULL,
						       -G_MAXDOUBLE, G_MAXDOUBLE, 0,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_GLYPH_SHAPE,
				  g_param_spec_boxed ("glyph_shape", NULL, NULL,
						      zMapStyleGlyphShapeGetType(),
						      G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,
				  PROP_FILL_COLOR,
				  g_param_spec_string ("fill-color", NULL, NULL,
						       NULL,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FILL_COLOR_GDK,
				  g_param_spec_boxed ("fill-color-gdk", NULL, NULL,
						      GDK_TYPE_COLOR,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_FILL_COLOR_RGBA,
				  g_param_spec_uint ("fill-color-rgba", NULL, NULL,
						     0, G_MAXUINT, 0,
						     G_PARAM_READWRITE));


  g_object_class_install_property(gobject_class,
				  PROP_OUTLINE_COLOR,
				  g_param_spec_string ("outline-color", NULL, NULL,
						       NULL,
						       G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_OUTLINE_COLOR_GDK,
				  g_param_spec_boxed ("outline-color-gdk", NULL, NULL,
						      GDK_TYPE_COLOR,
						      G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_OUTLINE_COLOR_RGBA,
				  g_param_spec_uint ("outline-color-rgba", NULL, NULL,
						     0, G_MAXUINT, 0,
						     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,
				  PROP_LINE_WIDTH,
				  g_param_spec_uint ("line-width", NULL, NULL,
						     0, G_MAXUINT, 0,
						     G_PARAM_READWRITE));

  g_object_class_install_property(gobject_class,
				  PROP_CAP_STYLE,
				  g_param_spec_enum ("cap-style", NULL, NULL,
						     GDK_TYPE_CAP_STYLE,
						     GDK_CAP_BUTT,
						     G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_JOIN_STYLE,
				  g_param_spec_enum ("join-style", NULL, NULL,
						     GDK_TYPE_JOIN_STYLE,
						     GDK_JOIN_MITER,
						     G_PARAM_READWRITE));
  g_object_class_install_property(gobject_class,
				  PROP_LINE_STYLE,
				  g_param_spec_enum ("line-style", NULL, NULL,
						     GDK_TYPE_LINE_STYLE,
						     GDK_LINE_SOLID,
						     G_PARAM_READWRITE));


  object_class->destroy = zmap_window_glyph_item_destroy;

  item_class->update    = zmap_window_glyph_item_update;
  item_class->realize   = zmap_window_glyph_item_realize;
  item_class->unrealize = zmap_window_glyph_item_unrealize;
  item_class->draw      = zmap_window_glyph_item_draw;
  item_class->point     = zmap_window_glyph_item_point;
  item_class->translate = zmap_window_glyph_item_translate;
  item_class->bounds    = zmap_window_glyph_item_bounds;

  return ;
}

/* Object initialization function for the text item */
static void zmap_window_glyph_item_init (ZMapWindowGlyphItem glyph)
{
  glyph->cx = 0.0;
  glyph->cy = 0.0 ;
  glyph->wx = 0.0;
  glyph->wy = 0.0 ;
  glyph->cap  = GDK_CAP_BUTT;
  glyph->join = GDK_JOIN_MITER;
  glyph->line_width = 0.0;
  glyph->line_style = GDK_LINE_SOLID;

  glyph->shape.n_coords = 0;
  glyph->shape.type = GLYPH_DRAW_INVALID;

  return ;
}

/* Destroy handler for the glyph item */
static void zmap_window_glyph_item_destroy (GtkObject *object)
{
  ZMapWindowGlyphItem glyph_item;

  glyph_item = ZMAP_WINDOW_GLYPH_ITEM(object);
  if(glyph_item->coords)
    {
      g_free(glyph_item->coords);
      glyph_item->coords = NULL;
    }

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);

  return ;
}

/* Set_arg handler for the glyph item */
static void zmap_window_glyph_item_set_property (GObject            *object,
						 guint               param_id,
						 const GValue       *value,
						 GParamSpec         *pspec)
{
  ZMapWindowGlyphItem glyph_item;
  FooCanvasItem *item;
  int update = 1;
  int redraw = 0;
  int coord_update = 0;

  g_return_if_fail(object != NULL);
  g_return_if_fail(ZMAP_IS_WINDOW_GLYPH_ITEM(object));

  glyph_item = ZMAP_WINDOW_GLYPH_ITEM(object);
  item = FOO_CANVAS_ITEM(object);

  switch(param_id)
    {
    case PROP_OUTLINE_COLOR:
    case PROP_OUTLINE_COLOR_GDK:
    case PROP_OUTLINE_COLOR_RGBA:
      {
	glyph_item->line_set = glyph_set_color_property(glyph_item, param_id, value,
							&(glyph_item->line_rgba),
							&(glyph_item->line_pixel));
	glyph_set_gc_line_color(glyph_item);
	redraw = 1;
      }
      break;

    case PROP_FILL_COLOR:
    case PROP_FILL_COLOR_GDK:
    case PROP_FILL_COLOR_RGBA:
      {
	glyph_item->area_set = glyph_set_color_property(glyph_item, param_id, value,
							&(glyph_item->area_rgba),
							&(glyph_item->area_pixel));
	glyph_set_gc_area_color(glyph_item);
	redraw = 1;
      }
      break;
    case PROP_ORIGIN_X_PIXELS:
      glyph_item->wx = g_value_get_double(value);
      coord_update   = 1;
      break;
    case PROP_ORIGIN_Y_PIXELS:
      glyph_item->wy = g_value_get_double(value);
      coord_update   = 1;
      break;

    case PROP_GLYPH_SHAPE:
      memcpy(&glyph_item->shape,g_value_get_boxed(value),sizeof(ZMapStyleGlyphShapeStruct));
      coord_update = redraw = 1;
      break;

      /* Line properties */
    case PROP_LINE_WIDTH:
      glyph_item->line_width = g_value_get_uint (value);
      break;
    case PROP_CAP_STYLE:
      glyph_item->cap = g_value_get_enum (value);
      break;
    case PROP_JOIN_STYLE:
      glyph_item->join = g_value_get_enum (value);
      break;
    case PROP_LINE_STYLE:
      glyph_item->line_style = g_value_get_enum (value);
      break;
    default:
      break;
    }

  if(coord_update && glyph_item->coords)
    {
      g_free(glyph_item->coords);
      glyph_item->coords = NULL;
      glyph_item->num_points = 0;
    }

  if(update)
    foo_canvas_item_request_update(item);

  if(redraw)
    foo_canvas_item_request_redraw(item);

  return ;
}

/* Get_arg handler for the glyph item */
static void zmap_window_glyph_item_get_property (GObject            *object,
						 guint               param_id,
						 GValue             *value,
						 GParamSpec         *pspec)
{
  return ;
}

static void get_bbox_bounds(ZMapWindowGlyphItem glyph_item, double canvas_dx, double canvas_dy,
			    double *x1, double *y1, double *x2, double *y2)
{
  if(x1 && x2 && y1 && y2)
    {
      double pointx = 0.0, pointy = 0.0;
      double *pc;
      int i;

      // cx,cy is the anchor point where the glyph is displayed
      // coords are where the points are drawn, which may be offset in any direction

      pointx = glyph_item->cx + canvas_dx;
      pointy = glyph_item->cy + canvas_dy;
      *x1 = *x2 = pointx;
      *y1 = *y2 = pointy;

      for(i = 0,pc = glyph_item->coords;pc && i < glyph_item->shape.n_coords;i++)
      {
            pointx = *pc++;
            pointy = *pc++;

            /* grow bounds... */
            *x1 = MIN(pointx, *x1);
            *y1 = MIN(pointy, *y1);
            *x2 = MAX(pointx, *x2);
            *y2 = MAX(pointy, *y2);
      }
    }
  return ;
}

static void get_bbox_bounds_canvas(ZMapWindowGlyphItem glyph_item, double i2w_dx, double i2w_dy,
				   double *x1, double *y1, double *x2, double *y2)
{
  /* Get the canvas offset (i2w_d* come from cummulative group positions) of the item's space. */
  foo_canvas_w2c_d(FOO_CANVAS_ITEM(glyph_item)->canvas, i2w_dx, i2w_dy, &i2w_dx, &i2w_dy);

  get_bbox_bounds(glyph_item, i2w_dx, i2w_dy, x1, y1, x2, y2);

  return ;
}
#ifdef ROTATE_REQUIRED
static void rotate(double x_origin, double y_origin, int rotation,
		   float *x_in_out, float* y_in_out)
{
  float x, y, dx, dy;
  double angle= 0.0;
  double cos_a, sin_a;

  if(x_in_out && y_in_out)
    {
      x = *x_in_out;
      y = *y_in_out;

      dx = x - x_origin;
      dy = y - y_origin;

      if(dx == 0 && dy == 0)
	return;

      angle = rotation * M_PI / 180;

      cos_a = cos(angle);
      sin_a = sin(angle);

      x = (cos_a * dx) - (sin_a * dy) + x_origin;
      y = (sin_a * dx) + (cos_a * dy) + y_origin;

      *x_in_out = x;
      *y_in_out = y;
    }

  return ;
}
#endif /* ROTATE_REQUIRED */


// set the points to draw centred on the glyph's sequence/column coordinates
// if we scale the glyph it's done here and NB we have to scale +ve and -ve coords for all shaoes
//
static void glyph_fill_points(ZMapWindowGlyphItem glyph)
{
  int i,j,coord;

  if(glyph->coords)
    {
      g_free(glyph->coords);
      glyph->coords = NULL;
    }

  glyph->num_points = glyph->shape.n_coords;

  switch(glyph->shape.type)
  {
  case GLYPH_DRAW_ARC:
    if(glyph->num_points > 2)       // bounding box, don't include the angles, these points get transformed
      glyph->num_points = 2;        // x-ref to _draw()
    // fall through

  case GLYPH_DRAW_LINES:
  case GLYPH_DRAW_BROKEN:
  case GLYPH_DRAW_POLYGON:

    glyph->coords = g_new0(double,glyph->shape.n_coords * 2);

    for(i = j = 0;i < glyph->shape.n_coords;i++)
      {
        coord = glyph->shape.coords[j];
        if(coord != GLYPH_COORD_INVALID)
            glyph->coords[j] = glyph->cx + coord;     // points are centred around the anchor of 0,0
        else
            glyph->coords[j] = GLYPH_CANVAS_COORD_INVALID; // zero
        j++;

        coord = glyph->shape.coords[j];
        if(coord != GLYPH_COORD_INVALID)
            glyph->coords[j] = glyph->cy + coord;     // points are centred around the anchor of 0,0
        else
            glyph->coords[j] = GLYPH_CANVAS_COORD_INVALID; // zero
        j++;
      }
    break;
  default:
    // we get a warning message in _draw() so be silent here
    break;
  }

  // set cw and ch??

  return ;
}

/* Update handler for the text item */
static void zmap_window_glyph_item_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
  ZMapWindowGlyphItem glyph;
  double x1, x2, y1, y2;

  glyph = ZMAP_WINDOW_GLYPH_ITEM(item);

  if (parent_class->update)
    (* parent_class->update) (item, i2w_dx, i2w_dy, flags);

  glyph_set_gc_line_attributes(glyph);
  glyph_set_gc_line_color(glyph);
  glyph_set_gc_area_color(glyph);

  glyph->cx = glyph->wx * item->canvas->pixels_per_unit_x;
  glyph->cy = glyph->wy * item->canvas->pixels_per_unit_y;

  glyph_fill_points(glyph);

  get_bbox_bounds_canvas(glyph, i2w_dx, i2w_dy, &x1, &y1, &x2, &y2);

  foo_canvas_update_bbox(item, x1, y1, x2, y2);

  return ;
}

/* Realize handler for the text item */
static void zmap_window_glyph_item_realize (FooCanvasItem *item)
{
  ZMapWindowGlyphItem glyph;

  glyph = ZMAP_WINDOW_GLYPH_ITEM(item);

  if (parent_class->realize)
    (* parent_class->realize) (item);

  glyph->line_gc = gdk_gc_new(item->canvas->layout.bin_window);

  glyph->area_gc = gdk_gc_new(item->canvas->layout.bin_window);

  return ;
}

/* Unrealize handler for the text item */
static void zmap_window_glyph_item_unrealize (FooCanvasItem *item)
{
  ZMapWindowGlyphItem glyph;

  glyph = ZMAP_WINDOW_GLYPH_ITEM(item);

  g_object_unref(glyph->line_gc);
  glyph->line_gc = NULL;

  g_object_unref(glyph->area_gc);
  glyph->area_gc = NULL;

  if (parent_class->unrealize)
    (* parent_class->unrealize) (item);

  return ;
}

/* Draw handler for the glyph item */
// x-ref with zmapWindowDump.c/dumpGlyph()
static void zmap_window_glyph_item_draw (FooCanvasItem  *item,
					 GdkDrawable    *drawable,
					 GdkEventExpose *expose)
{
  ZMapWindowGlyphItem glyph;
  GdkPoint points[GLYPH_SHAPE_MAX_POINT];
  double i2w_dx, i2w_dy;
  int start,end;

  glyph = ZMAP_WINDOW_GLYPH_ITEM (item);

  if(!glyph->num_points)
    return;

      /*
       * mangle all the points
       * - for arcs the angled are not included
       * - for broken lines the breaks are 0,0, but we skip over these anyway
       */
  i2w_dx = 0.0;
  i2w_dy = 0.0;
  foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);
  item_to_canvas (item->canvas, glyph->coords, points, glyph->num_points, i2w_dx, i2w_dy);

  switch(glyph->shape.type)
    {
    case GLYPH_DRAW_LINES:
      gdk_draw_lines (drawable, glyph->line_gc, points, glyph->num_points);
      break;
    case GLYPH_DRAW_BROKEN:
      /*
       * in the shape structure the array of coords has invalid values at the break
       * and we draw lines between the points in between
       * NB: GDK uses points we have coordinate pairs
       */
      for(start = 0;start < glyph->shape.n_coords;start = end+1)
        {
          for(end = start;end < glyph->shape.n_coords && glyph->shape.coords[end+end] != GLYPH_COORD_INVALID; end++)
            continue;

          gdk_draw_lines (drawable, glyph->line_gc, points + start , end - start);
        }
      break;

    case GLYPH_DRAW_POLYGON:
      if (glyph->area_set)
        {
          /* foo_canvas_set_stipple_origin removed */
          gdk_draw_polygon (drawable, glyph->area_gc, TRUE, points, glyph->num_points);
        }

      if (glyph->line_set)
        {
          /* foo_canvas_set_stipple_origin removed */
          gdk_draw_polygon (drawable, glyph->line_gc, FALSE, points, glyph->num_points);
        }
      break;

    case GLYPH_DRAW_ARC:
      {
        double x1, y1, x2, y2;
        int a1,a2;

        x1 = points[0].x;
        y1 = points[0].y;
        x2 = points[1].x;
        y2 = points[1].y;
        a1 = (int) glyph->shape.coords[4] * 64;
        a2 = (int) glyph->shape.coords[5] * 64;

        if(glyph->area_set)
          {
            gdk_draw_arc(drawable, glyph->area_gc, TRUE,
                  (int) x1, (int) y1, (int) (x2 - x1), (int) (y2 - y1), a1,a2);
          }
        if(glyph->line_set)
          {
            gdk_draw_arc(drawable, glyph->line_gc, FALSE,
                  (int) x1, (int) y1,  (int) (x2 - x1), (int) (y2 - y1), a1,a2);
          }
      }
      break;

    default:
      g_warning("Unknown Glyph Style");
      break;
    }

  return ;
}

/* Point handler for the text item */
static double zmap_window_glyph_item_point (FooCanvasItem *item, double x, double y,
					    int cx, int cy, FooCanvasItem **actual_item)
{
  ZMapWindowGlyphItem glyph;
  GdkPoint points[GLYPH_SHAPE_MAX_COORD], *tmp_points;
  double coords[GLYPH_SHAPE_MAX_POINT];
  double *tmp_coords, i2w_dx, i2w_dy;
  double dist, best;
  int i;

  dist = best = 1.0e36;

  g_return_val_if_fail(ZMAP_IS_WINDOW_GLYPH_ITEM(item), dist);

  glyph = ZMAP_WINDOW_GLYPH_ITEM(item);

  i2w_dx = 0.0;
  i2w_dy = 0.0;
  foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);

  /* Build array of canvas pixel coordinates */

  item_to_canvas (item->canvas, glyph->coords, points, glyph->num_points, i2w_dx, i2w_dy);

  tmp_points = points;
  tmp_coords = coords;

  for(i = 0; i < glyph->num_points; i++, tmp_coords+=2, tmp_points++)
    {
      tmp_coords[0] = tmp_points->x;
      tmp_coords[1] = tmp_points->y;
    }

      /*
       * we have lines polygons and circles
       * circles are defined here by a bounding rectangle so should work
       * small arcs not handled - waiting on source code to see how to fix this
       */
  best = foo_canvas_polygon_to_point(coords, glyph->num_points, (double)cx, (double)cy);

  dist = best;

  *actual_item = item;

  return dist;
}

static void zmap_window_glyph_item_translate (FooCanvasItem *item, double dx, double dy)
{

  return ;
}

/* Bounds handler for the text item */
static void zmap_window_glyph_item_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
  double cx1, cx2, cy1, cy2;
  double i2w_dx = 0.0, i2w_dy = 0.0;

  get_bbox_bounds(ZMAP_WINDOW_GLYPH_ITEM(item), i2w_dx, i2w_dy,
		  &cx1, &cy1, &cx2, &cy2);

  if(x1)
    *x1 = cx1 / item->canvas->pixels_per_unit_x;

  if(x2)
    *x2 = cx2 / item->canvas->pixels_per_unit_x;

  if(y1)
    *y1 = cy1 / item->canvas->pixels_per_unit_y;

  if(y2)
    *y2 = cy2 / item->canvas->pixels_per_unit_y;

  return ;
}


