/*  Last edited: Oct  3 14:00 2006 (edgrif) */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */


/* This code is all over the place, it works for the simple stick shaped glyphs
 * but nothing else. Lots of stuff is #defined out and it all needs some sorting
 * out. */



/* Glyphs made from lines, they are constant scale so show at all zoom levels.
 *
 * FooCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Ed Griffiths <edgrif@sanger.ac.uk>
 */

#include <config.h>
#include <math.h>
#include <string.h>
#include "libfoocanvas.h"
#include "foozmap-canvas-line-glyph.h"

#define noVERBOSE

#define NUM_STATIC_POINTS    256	/* number of static points to use to avoid allocating arrays */


#define GROW_BOUNDS(bx1, by1, bx2, by2, x, y) {	\
	if (x < bx1)				\
		bx1 = x;			\
						\
	if (x > bx2)				\
		bx2 = x;			\
						\
	if (y < by1)				\
		by1 = y;			\
						\
	if (y > by2)				\
		by2 = y;			\
}


enum {
	PROP_0,
	PROP_X,
	PROP_Y,
	PROP_POINTS,
	PROP_FILL_COLOR,
	PROP_FILL_COLOR_GDK,
	PROP_FILL_COLOR_RGBA,
	PROP_FILL_STIPPLE,
	PROP_WIDTH_PIXELS,
	PROP_CAP_STYLE,
	PROP_JOIN_STYLE,
	PROP_LINE_STYLE
};


static void foo_canvas_line_glyph_class_init   (FooCanvasLineGlyphClass *class_glyph);
static void foo_canvas_line_glyph_init         (FooCanvasLineGlyph      *line_glyph);
static void foo_canvas_line_glyph_destroy      (GtkObject            *object);
static void foo_canvas_line_glyph_set_property (GObject              *object,
						guint                 param_id,
						const GValue         *value,
						GParamSpec           *pspec);
static void foo_canvas_line_glyph_get_property (GObject              *object,
						guint                 param_id,
						GValue               *value,
						GParamSpec           *pspec);

static void   foo_canvas_line_glyph_update      (FooCanvasItem *item,
						 double i2w_dx, double i2w_dy,
						 int flags);
static void   foo_canvas_line_glyph_realize     (FooCanvasItem *item);
static void   foo_canvas_line_glyph_unrealize   (FooCanvasItem *item);
static void   foo_canvas_line_glyph_draw        (FooCanvasItem *item, GdkDrawable *drawable,
						 GdkEventExpose   *event);
static double foo_canvas_line_glyph_point       (FooCanvasItem *item, double x, double y,
						 int cx, int cy, FooCanvasItem **actual_item);
static void   foo_canvas_line_glyph_translate   (FooCanvasItem *item, double dx, double dy);
static void   foo_canvas_line_glyph_bounds      (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2);


static FooCanvasItemClass *parent_class;


GtkType
foo_canvas_line_glyph_get_type (void)
{
	static GtkType line_type = 0;

	if (!line_type) {
		/* FIXME: Convert to gobject style.  */
		static const GtkTypeInfo line_info = {
			(char *)"FooCanvasLineGlyph",
			sizeof (FooCanvasLineGlyph),
			sizeof (FooCanvasLineGlyphClass),
			(GtkClassInitFunc) foo_canvas_line_glyph_class_init,
			(GtkObjectInitFunc) foo_canvas_line_glyph_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		line_type = gtk_type_unique (foo_canvas_item_get_type (), &line_info);
	}

	return line_type;
}

static void
foo_canvas_line_glyph_class_init (FooCanvasLineGlyphClass *class)
{
	GObjectClass *gobject_class;
	GtkObjectClass *object_class;
	FooCanvasItemClass *item_class;

	gobject_class = (GObjectClass *) class;
	object_class = (GtkObjectClass *) class;
	item_class = (FooCanvasItemClass *) class;

	parent_class = gtk_type_class (foo_canvas_item_get_type ());

	gobject_class->set_property = foo_canvas_line_glyph_set_property;
	gobject_class->get_property = foo_canvas_line_glyph_get_property;

        g_object_class_install_property
                (gobject_class,
                 PROP_X,
                 g_param_spec_double ("x", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_Y,
                 g_param_spec_double ("y", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0,
				      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_POINTS,
                 g_param_spec_boxed ("points", NULL, NULL,
				     FOO_TYPE_CANVAS_POINTS,
				     G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR,
                 g_param_spec_string ("fill-color", NULL, NULL,
                                      NULL,
                                      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_GDK,
                 g_param_spec_boxed ("fill-color-gdk", NULL, NULL,
				     GDK_TYPE_COLOR,
				     G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_COLOR_RGBA,
                 g_param_spec_uint ("fill-color-rgba", NULL, NULL,
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_FILL_STIPPLE,
                 g_param_spec_object ("fill-stipple", NULL, NULL,
                                      GDK_TYPE_DRAWABLE,
                                      G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_WIDTH_PIXELS,
                 g_param_spec_uint ("width-pixels", NULL, NULL,
				    0, G_MAXUINT, 0,
				    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_CAP_STYLE,
                 g_param_spec_enum ("cap-style", NULL, NULL,
                                    GDK_TYPE_CAP_STYLE,
                                    GDK_CAP_BUTT,
                                    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_JOIN_STYLE,
                 g_param_spec_enum ("join-style", NULL, NULL,
                                    GDK_TYPE_JOIN_STYLE,
                                    GDK_JOIN_MITER,
                                    G_PARAM_READWRITE));
        g_object_class_install_property
                (gobject_class,
                 PROP_LINE_STYLE,
                 g_param_spec_enum ("line-style", NULL, NULL,
                                    GDK_TYPE_LINE_STYLE,
                                    GDK_LINE_SOLID,
                                    G_PARAM_READWRITE));

	object_class->destroy = foo_canvas_line_glyph_destroy;

	item_class->update = foo_canvas_line_glyph_update;
	item_class->realize = foo_canvas_line_glyph_realize;
	item_class->unrealize = foo_canvas_line_glyph_unrealize;
	item_class->draw = foo_canvas_line_glyph_draw;
	item_class->point = foo_canvas_line_glyph_point;
	item_class->translate = foo_canvas_line_glyph_translate;
	item_class->bounds = foo_canvas_line_glyph_bounds;
}

static void
foo_canvas_line_glyph_init (FooCanvasLineGlyph *line)
{
        line->x = line->y = 0.0 ;
	line->width = 0.0;
	line->cap = GDK_CAP_BUTT;
	line->join = GDK_JOIN_MITER;
	line->line_style = GDK_LINE_SOLID;
}

static void
foo_canvas_line_glyph_destroy (GtkObject *object)
{
	FooCanvasLineGlyph *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE_GLYPH (object));

	line = FOO_CANVAS_LINE_GLYPH (object);

	/* remember, destroy can be run multiple times! */

	if (line->coords)
		g_free (line->coords);
	line->coords = NULL;

	if (line->stipple)
		g_object_unref (line->stipple);
	line->stipple = NULL;

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Computes the bounding box of the glyph.  Assumes that the number of
 * points in the line is not zero.
 */
static void
get_bounds (FooCanvasLineGlyph *line, double *bx1, double *by1, double *bx2, double *by2)
{
	double *coords;
	double x1, y1, x2, y2;
	double width;
	int i;

	if (!line->coords) {
	    *bx1 = *by1 = *bx2 = *by2 = 0.0;
	    return;
	}
	
	/* Find bounding box of line's points */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	x1 = x2 = line->coords[0];
	y1 = y2 = line->coords[1];

	for (i = 1, coords = line->coords + 2; i < line->num_points; i++, coords += 2)
		GROW_BOUNDS (x1, y1, x2, y2, coords[0], coords[1]);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	x1 = x2 = line->x + line->coords[0];
	y1 = y2 = line->y + line->coords[1];

	for (i = 1, coords = line->coords + 2; i < line->num_points; i++, coords += 2)
		GROW_BOUNDS (x1, y1, x2, y2, line->x + coords[0], line->y + coords[1]);



	/* Add possible over-estimate for wide lines */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if (line->width_pixels)
		width = line->width / line->item.canvas->pixels_per_unit_x;
	else
		width = line->width;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	width = line->width / line->item.canvas->pixels_per_unit_x;


	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* Forget this for now.... */

	/* For mitered lines, make a second pass through all the points.  Compute the location of
	 * the two miter vertex points and add them to the bounding box.
	 */

	if (line->join == GDK_JOIN_MITER)
		for (i = line->num_points, coords = line->coords; i >= 3; i--, coords += 2) {
			double mx1, my1, mx2, my2;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
			if (foo_canvas_get_miter_points (coords[0], coords[1],
							   coords[2], coords[3],
							   coords[4], coords[5],
							   width,
							   &mx1, &my1, &mx2, &my2)) {
				GROW_BOUNDS (x1, y1, x2, y2, mx1, my1);
				GROW_BOUNDS (x1, y1, x2, y2, mx2, my2);
			}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


			if (foo_canvas_get_miter_points (line->x + coords[0], line->y + coords[1],
							   line->x + coords[2], line->y + coords[3],
							   line->x + coords[4], line->y + coords[5],
							   width,
							   &mx1, &my1, &mx2, &my2)) {
				GROW_BOUNDS (x1, y1, x2, y2, mx1, my1);
				GROW_BOUNDS (x1, y1, x2, y2, mx2, my2);
			}


		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	/* Done */

	*bx1 = x1;
	*by1 = y1;
	*bx2 = x2;
	*by2 = y2;

}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

/* Computes the bounding box of the glyph.  Assumes that the number of
 * points in the line is not zero.
 */
static void
get_bounds (FooCanvasLineGlyph *line, double *bx1, double *by1, double *bx2, double *by2)
{
	double *coords;
	double x1, y1, x2, y2;
	double width;

	if (!line->coords) {
	    *bx1 = *by1 = *bx2 = *by2 = 0.0;
	    return;
	}
	
	/* Find bounding box of line's points */
	/* extract topleft/botright.... */
	coords = line->coords;
	x1 = line->x + coords[0];
	x2 = line->x + coords[2];
	if (coords[5] < coords[3])
	  {
	    y1 = line->y + coords[5];
	    y2 = line->y + coords[3];
	  }
	else
	  {
	    y1 = line->y + coords[3];
	    y2 = line->y + coords[5];
	  }


	/* Add possible over-estimate for wide lines */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if (line->width_pixels)
		width = line->width / line->item.canvas->pixels_per_unit_x;
	else
		width = line->width;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	width = line->width / line->item.canvas->pixels_per_unit_x;

	x1 -= width;
	y1 -= width;
	x2 += width;
	y2 += width;


#ifdef VERBOSE
	printf("get_bounds  %g, %g ---> %g, %g\n", x1, y1, x2, y2) ;
#endif


	/* Done */

	*bx1 = x1;
	*by1 = y1;
	*bx2 = x2;
	*by2 = y2;

}




/* Computes the bounding box of the line, in canvas coordinates.  Assumes that the number of points in the polygon is
 * not zero. 
 */
static void
get_bounds_canvas (FooCanvasLineGlyph *line,
		   double *bx1, double *by1, double *bx2, double *by2,
		   double i2w_dx, double i2w_dy)
{
	FooCanvasItem *item;
	double bbox_x0, bbox_y0, bbox_x1, bbox_y1;
	double glyph_x, glyph_y ;
	double width, height ;
	double *coords;

	item = FOO_CANVAS_ITEM (line);

	get_bounds (line, &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);

	bbox_x0 += i2w_dx; 
	bbox_y0 += i2w_dy; 
	bbox_x1 += i2w_dx; 
	bbox_y1 += i2w_dy; 

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* You can't just do this as it scales everything. */
	foo_canvas_w2c_rect_d (item->canvas,
				 &bbox_x0, &bbox_y0, &bbox_x1, &bbox_y1);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* Calculate position of glyph in its group.... */
	foo_canvas_w2c_d (item->canvas,
			  line->x + i2w_dx,
			  line->y + i2w_dy,
			  &glyph_x, &glyph_y);

	width = bbox_x1 - bbox_x0 ;
	height = bbox_y1 - bbox_y0 ;

	bbox_x0 = glyph_x; 
	bbox_x1 = glyph_x + width; 

	/* Have to take account of whether box is up or down. */
	coords = line->coords;
	if (coords[5] > coords[3])
	  {
	    bbox_y0 = glyph_y; 
	    bbox_y1 = glyph_y + height; 
	  }
	else
	  {
	    bbox_y0 = glyph_y - height; 
	    bbox_y1 = glyph_y; 
	  }

	/* include 1 pixel of fudge */
	*bx1 = bbox_x0 - 1;
	*by1 = bbox_y0 - 1;
	*bx2 = bbox_x1 + 1;
	*by2 = bbox_y1 + 1;
}



/* Convenience function to set the line's GC's foreground color */
static void
set_line_glyph_gc_foreground (FooCanvasLineGlyph *line)
{
	GdkColor c;

	if (!line->gc)
		return;

	c.pixel = line->fill_pixel;
	gdk_gc_set_foreground (line->gc, &c);
}

/* Recalculate the line's width and set it in its GC */
static void
set_line_glyph_gc_width (FooCanvasLineGlyph *line)
{
	int width;

	if (!line->gc)
		return;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if (line->width_pixels)
		width = (int) line->width;
	else
		width = (int) (line->width * line->item.canvas->pixels_per_unit_x + 0.5);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	width = (int) line->width;

	gdk_gc_set_line_attributes (line->gc,
				    width,
				    line->line_style,
				    line->cap,
				    line->join);
}

/* Sets the stipple pattern for the line */
static void
set_stipple (FooCanvasLineGlyph *line, GdkBitmap *stipple, int reconfigure)
{
	if (line->stipple && !reconfigure)
		g_object_unref (line->stipple);

	line->stipple = stipple;
	if (stipple && !reconfigure)
		g_object_ref (stipple);

	if (line->gc) {
		if (stipple) {
			gdk_gc_set_stipple (line->gc, stipple);
			gdk_gc_set_fill (line->gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (line->gc, GDK_SOLID);
	}
}

static void
foo_canvas_line_glyph_set_property (GObject              *object,
				guint                 param_id,
				const GValue         *value,
				GParamSpec           *pspec)
{
	FooCanvasItem *item;
	FooCanvasLineGlyph *line;
	FooCanvasPoints *points;
	GdkColor color = { 0, 0, 0, 0, };
	GdkColor *pcolor;
	gboolean color_changed;
	int have_pixel;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE_GLYPH (object));

	item = FOO_CANVAS_ITEM (object);
	line = FOO_CANVAS_LINE_GLYPH (object);

	color_changed = FALSE;
	have_pixel = FALSE;

	switch (param_id) {
	case PROP_X:
		line->x = g_value_get_double (value);

		foo_canvas_item_request_update (item);
		break;

	case PROP_Y:
		line->y = g_value_get_double (value);

		foo_canvas_item_request_update (item);
		break;

	case PROP_POINTS:
		points = g_value_get_boxed (value);


		if (line->coords) {
			g_free (line->coords);
			line->coords = NULL;
		}

		if (!points)
			line->num_points = 0;
		else {
			line->num_points = points->num_points;
			line->coords = g_new (double, 2 * line->num_points);
			memcpy (line->coords, points->coords, 2 * line->num_points * sizeof (double));
		}



		/* Since the line's points have changed, we need to recalculate the bounds.
		 */
		foo_canvas_item_request_update (item);
		break;

	case PROP_FILL_COLOR:
		if (g_value_get_string (value))
			gdk_color_parse (g_value_get_string (value), &color);
		line->fill_rgba = ((color.red & 0xff00) << 16 |
				   (color.green & 0xff00) << 8 |
				   (color.blue & 0xff00) |
				   0xff);
		color_changed = TRUE;
		break;

	case PROP_FILL_COLOR_GDK:
		pcolor = g_value_get_boxed (value);
		if (pcolor) {
			GdkColormap *colormap;
			color = *pcolor;

			colormap = gtk_widget_get_colormap (GTK_WIDGET (item->canvas));
			gdk_rgb_find_color (colormap, &color);

			have_pixel = TRUE;
		}

		line->fill_rgba = ((color.red & 0xff00) << 16 |
				   (color.green & 0xff00) << 8 |
				   (color.blue & 0xff00) |
				   0xff);
		color_changed = TRUE;
		break;

	case PROP_FILL_COLOR_RGBA:
		line->fill_rgba = g_value_get_uint (value);
		color_changed = TRUE;
		break;

	case PROP_FILL_STIPPLE:
		set_stipple (line, (GdkBitmap *) g_value_get_object (value), FALSE);
		foo_canvas_item_request_redraw (item);		
		break;

	case PROP_WIDTH_PIXELS:
		line->width = g_value_get_uint (value);
		set_line_glyph_gc_width (line);
		foo_canvas_item_request_update (item);
		break;

	case PROP_CAP_STYLE:
		line->cap = g_value_get_enum (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_JOIN_STYLE:
		line->join = g_value_get_enum (value);
		foo_canvas_item_request_update (item);
		break;

	case PROP_LINE_STYLE:
		line->line_style = g_value_get_enum (value);
		set_line_glyph_gc_width (line);
		foo_canvas_item_request_update (item);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}

	if (color_changed) {
		if (have_pixel)
			line->fill_pixel = color.pixel;
		else
			line->fill_pixel = foo_canvas_get_color_pixel (item->canvas,
									 line->fill_rgba);

		set_line_glyph_gc_foreground (line);

		foo_canvas_item_request_redraw (item);		
	}
}

/* Returns a copy of the line's points.
 */
static FooCanvasPoints *
get_points (FooCanvasLineGlyph *line)
{
	FooCanvasPoints *points;
	int start_ofs, end_ofs;

	if (line->num_points == 0)
		return NULL;

	start_ofs = end_ofs = 0;

	points = foo_canvas_points_new (line->num_points);

	memcpy (points->coords + 2 * start_ofs,
		line->coords + 2 * start_ofs,
		2 * (line->num_points - (start_ofs + end_ofs)) * sizeof (double));

	return points;
}

static void
foo_canvas_line_glyph_get_property (GObject              *object,
				guint                 param_id,
				GValue               *value,
				GParamSpec           *pspec)
{
	FooCanvasLineGlyph *line;

	g_return_if_fail (object != NULL);
	g_return_if_fail (FOO_IS_CANVAS_LINE_GLYPH (object));

	line = FOO_CANVAS_LINE_GLYPH (object);

	switch (param_id) {
	case PROP_X:
		g_value_set_double (value,  line->x);
		break;

	case PROP_Y:
		g_value_set_double (value,  line->y);
		break;

	case PROP_POINTS:
		g_value_set_boxed (value, get_points (line));
		break;

	case PROP_FILL_COLOR:
		//g_value_take_string (value,
		//		     g_strdup_printf ("#%02x%02x%02x",
		//				      line->fill_rgba >> 24,
		//				      (line->fill_rgba >> 16) & 0xff,
		//				      (line->fill_rgba >> 8) & 0xff));
		break;

	case PROP_FILL_COLOR_GDK: {
		FooCanvas *canvas = FOO_CANVAS_ITEM (line)->canvas;
		GdkColormap *colormap = gtk_widget_get_colormap (GTK_WIDGET (canvas));
		GdkColor color;

		gdk_colormap_query_color (colormap, line->fill_pixel, &color);
		g_value_set_boxed (value, &color);
		break;
	}

	case PROP_FILL_COLOR_RGBA:
		g_value_set_uint (value, line->fill_rgba);
		break;

	case PROP_FILL_STIPPLE:
		g_value_set_object (value, line->stipple);
		break;

	case PROP_WIDTH_PIXELS:
		g_value_set_uint (value, line->width);
		break;
		
	case PROP_CAP_STYLE:
		g_value_set_enum (value, line->cap);
		break;

	case PROP_JOIN_STYLE:
		g_value_set_enum (value, line->join);
		break;

	case PROP_LINE_STYLE:
		g_value_set_enum (value, line->line_style);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

static void
foo_canvas_line_glyph_update (FooCanvasItem *item, double i2w_dx, double i2w_dy, int flags)
{
	FooCanvasLineGlyph *line;
	double x1, y1, x2, y2;

	line = FOO_CANVAS_LINE_GLYPH (item);

	if (parent_class->update)
		(* parent_class->update) (item, i2w_dx, i2w_dy, flags);

	set_line_glyph_gc_foreground (line);
	set_line_glyph_gc_width (line);
	set_stipple (line, line->stipple, TRUE);
	
	get_bounds_canvas (line, &x1, &y1, &x2, &y2, i2w_dx, i2w_dy);
	foo_canvas_update_bbox (item, x1, y1, x2, y2);
}

static void
foo_canvas_line_glyph_realize (FooCanvasItem *item)
{
	FooCanvasLineGlyph *line;

	line = FOO_CANVAS_LINE_GLYPH (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	line->gc = gdk_gc_new (item->canvas->layout.bin_window);
#warning "FIXME: Need to recalc pixel values, set colours, etc."

#if 0
	(* FOO_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);
#endif
}

static void
foo_canvas_line_glyph_unrealize (FooCanvasItem *item)
{
	FooCanvasLineGlyph *line;

	line = FOO_CANVAS_LINE_GLYPH (item);

	g_object_unref (line->gc);
	line->gc = NULL;

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void
item_to_canvas (FooCanvas *canvas,
		double *item_coords, GdkPoint *canvas_coords, int num_points,
		int *num_drawn_points, double i2w_dx, double i2w_dy)
{
	int i;
	int old_cx, old_cy;
	int cx, cy;

	/* the first point is always drawn */
	foo_canvas_w2c (canvas,
			  item_coords[0] + i2w_dx,
			  item_coords[1] + i2w_dy,
			  &canvas_coords->x, &canvas_coords->y);


	old_cx = canvas_coords->x;
	old_cy = canvas_coords->y;
	canvas_coords++;
	*num_drawn_points = 1;

	for (i = 1; i < num_points; i++) {

		foo_canvas_w2c (canvas,
				  item_coords[i*2] + i2w_dx,
				  item_coords[i*2+1] + i2w_dy,
				  &cx, &cy);


		if (old_cx != cx || old_cy != cy) {
			canvas_coords->x = cx;
			canvas_coords->y = cy;
			old_cx = cx;
			old_cy = cy;
			canvas_coords++;
			(*num_drawn_points)++;
		}
	}
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void
item_to_canvas (FooCanvas *canvas,
		double x, double y,
		double *item_coords, GdkPoint *canvas_coords, int num_points,
		int *num_drawn_points, double i2w_dx, double i2w_dy)
{
	int i;
	int old_cx, old_cy;
	int cx, cy;


	int glyph_x, glypy_y ;

	/* Calculate position of glyph in its group.... */
	foo_canvas_w2c (canvas,
			  x + i2w_dx,
			  y + i2w_dy,
			  &glyph_x, &glypy_y);


	/* the first point is always drawn */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	foo_canvas_w2c (canvas,
			  item_coords[0] + i2w_dx,
			  item_coords[1] + i2w_dy,
			  &canvas_coords->x, &canvas_coords->y);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	canvas_coords->x = glyph_x + item_coords[0] ;
	canvas_coords->y = glypy_y + item_coords[1] ;

	old_cx = canvas_coords->x;
	old_cy = canvas_coords->y;
	canvas_coords++;
	*num_drawn_points = 1;

	for (i = 1; i < num_points; i++) {


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		foo_canvas_w2c (canvas,
				  item_coords[i*2] + i2w_dx,
				  item_coords[i*2+1] + i2w_dy,
				  &cx, &cy);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		cx = glyph_x + item_coords[i*2] ;
		cy = glypy_y + item_coords[i*2+1] ;


		if (old_cx != cx || old_cy != cy) {
			canvas_coords->x = cx;
			canvas_coords->y = cy;
			old_cx = cx;
			old_cy = cy;
			canvas_coords++;
			(*num_drawn_points)++;
		}
	}
}


static void
foo_canvas_line_glyph_draw (FooCanvasItem *item, GdkDrawable *drawable,
			    GdkEventExpose *event)
{
	FooCanvasLineGlyph *line;
	GdkPoint static_points[NUM_STATIC_POINTS];
	GdkPoint *points;
	int actual_num_points_drawn;
	double i2w_dx, i2w_dy;
	
	line = FOO_CANVAS_LINE_GLYPH (item);

	if (line->num_points == 0)
		return;

	/* Build array of canvas pixel coordinates */

	if (line->num_points <= NUM_STATIC_POINTS)
		points = static_points;
	else
		points = g_new (GdkPoint, line->num_points);

	i2w_dx = 0.0;
	i2w_dy = 0.0;
	foo_canvas_item_i2w (item, &i2w_dx, &i2w_dy);
 
	item_to_canvas (item->canvas, line->x, line->y, line->coords, points, line->num_points,
			&actual_num_points_drawn, i2w_dx, i2w_dy);

	if (line->stipple)
		foo_canvas_set_stipple_origin (item->canvas, line->gc);

	gdk_draw_lines (drawable, line->gc, points, actual_num_points_drawn);

	if (points != static_points)
		g_free (points);

}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static double
foo_canvas_line_glyph_point (FooCanvasItem *item, double x, double y,
			     int cx, int cy, FooCanvasItem **actual_item)
{
	FooCanvasLineGlyph *line;
	double *line_points = NULL, *coords;
	double static_points[2 * NUM_STATIC_POINTS];
	double poly[10];
	double best, dist;
	double dx, dy;
	double width;
	int num_points = 0, i;
	int changed_miter_to_bevel;

#ifdef VERBOSE
	g_print ("foo_canvas_line_glyph_point x, y = (%g, %g); cx, cy = (%d, %d)\n", x, y, cx, cy);
#endif

	line = FOO_CANVAS_LINE_GLYPH (item);

	*actual_item = item;

	best = 1.0e36;

	num_points = line->num_points;
	line_points = line->coords;


	/* Compute a polygon for each edge of the line and test the point against it.  The effective
	 * width of the line is adjusted so that it will be at least one pixel thick (so that zero
	 * pixel-wide lines can be picked up as well).
	 */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if (line->width_pixels)
		width = line->width / item->canvas->pixels_per_unit_x;
	else
		width = line->width;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	width = line->width / item->canvas->pixels_per_unit_x;

	if (width < (1.0 / item->canvas->pixels_per_unit_x))
		width = 1.0 / item->canvas->pixels_per_unit_x;

	changed_miter_to_bevel = 0;

	for (i = num_points, coords = line_points; i >= 2; i--, coords += 2) {
		/* If rounding is done around the first point, then compute distance between the
		 * point and the first point.
		 */

		if (((line->cap == GDK_CAP_ROUND) && (i == num_points))
		    || ((line->join == GDK_JOIN_ROUND) && (i != num_points))) {
			dx = coords[0] - x;
			dy = coords[1] - y;

			dist = sqrt (dx * dx + dy * dy) - width / 2.0;
			if (dist < FOO_CANVAS_EPSILON) {
				best = 0.0;
				goto done;
			} else if (dist < best)
				best = dist;
		}

		/* Compute the polygonal shape corresponding to this edge, with two points for the
		 * first point of the edge and two points for the last point of the edge.
		 */

		if (i == num_points)
			foo_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly, poly + 1, poly + 2, poly + 3);
		else if ((line->join == GDK_JOIN_MITER) && !changed_miter_to_bevel) {
			poly[0] = poly[6];
			poly[1] = poly[7];
			poly[2] = poly[4];
			poly[3] = poly[5];
		} else {
			foo_canvas_get_butt_points (coords[2], coords[3], coords[0], coords[1],
						      width, FALSE,
						      poly, poly + 1, poly + 2, poly + 3);

			/* If this line uses beveled joints, then check the distance to a polygon
			 * comprising the last two points of the previous polygon and the first two
			 * from this polygon; this checks the wedges that fill the mitered point.
			 */

			if ((line->join == GDK_JOIN_BEVEL) || changed_miter_to_bevel) {
				poly[8] = poly[0];
				poly[9] = poly[1];

				dist = foo_canvas_polygon_to_point (poly, 5, x, y);
				if (dist < FOO_CANVAS_EPSILON) {
					best = 0.0;
					goto done;
				} else if (dist < best)
					best = dist;

				changed_miter_to_bevel = FALSE;
			}
		}

		if (i == 2)
			foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly + 4, poly + 5, poly + 6, poly + 7);
		else if (line->join == GDK_JOIN_MITER) {
			if (!foo_canvas_get_miter_points (coords[0], coords[1],
							    coords[2], coords[3],
							    coords[4], coords[5],
							    width,
							    poly + 4, poly + 5, poly + 6, poly + 7)) {
				changed_miter_to_bevel = TRUE;
				foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
							      width, FALSE,
							      poly + 4, poly + 5, poly + 6, poly + 7);
			}
		} else
			foo_canvas_get_butt_points (coords[0], coords[1], coords[2], coords[3],
						      width, FALSE,
						      poly + 4, poly + 5, poly + 6, poly + 7);

		poly[8] = poly[0];
		poly[9] = poly[1];

		dist = foo_canvas_polygon_to_point (poly, 5, x, y);
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else if (dist < best)
			best = dist;
	}

	/* If caps are rounded, check the distance to the cap around the final end point of the line */

	if (line->cap == GDK_CAP_ROUND) {
		dx = coords[0] - x;
		dy = coords[1] - y;
		dist = sqrt (dx * dx + dy * dy) - width / 2.0;
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

done:

	if ((line_points != static_points) && (line_points != line->coords))
		g_free (line_points);

	return best;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static double
foo_canvas_line_glyph_point (FooCanvasItem *item, double x, double y,
			     int cx, int cy, FooCanvasItem **actual_item)
{
	FooCanvasLineGlyph *line;
	double *line_points = NULL, *coords;
	double static_points[2 * NUM_STATIC_POINTS];
	double poly[10];
	double best, dist;
	double dx, dy;
	double width;
	int num_points = 0, i;
	int changed_miter_to_bevel;

#ifdef VERBOSE
	g_print ("foo_canvas_line_glyph_point x, y = (%g, %g); cx, cy = (%d, %d)\n", x, y, cx, cy);
#endif

	line = FOO_CANVAS_LINE_GLYPH (item);

	*actual_item = item;

	best = 1.0e36;

	num_points = line->num_points;
	line_points = line->coords;


	/* Compute a polygon for each edge of the line and test the point against it.  The effective
	 * width of the line is adjusted so that it will be at least one pixel thick (so that zero
	 * pixel-wide lines can be picked up as well).
	 */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	if (line->width_pixels)
		width = line->width / item->canvas->pixels_per_unit_x;
	else
		width = line->width;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	width = line->width / item->canvas->pixels_per_unit_x;

	if (width < (1.0 / item->canvas->pixels_per_unit_x))
		width = 1.0 / item->canvas->pixels_per_unit_x;

	changed_miter_to_bevel = 0;

	for (i = num_points, coords = line_points; i >= 2; i--, coords += 2) {
		/* If rounding is done around the first point, then compute distance between the
		 * point and the first point.
		 */

		if (((line->cap == GDK_CAP_ROUND) && (i == num_points))
		    || ((line->join == GDK_JOIN_ROUND) && (i != num_points))) {

			dx = line->x + coords[0] - x;
			dy = line->y + coords[1] - y;

			dist = sqrt (dx * dx + dy * dy) - width / 2.0;
			if (dist < FOO_CANVAS_EPSILON) {
				best = 0.0;
				goto done;
			} else if (dist < best)
				best = dist;
		}

		/* Compute the polygonal shape corresponding to this edge, with two points for the
		 * first point of the edge and two points for the last point of the edge.
		 */

		if (i == num_points)
			foo_canvas_get_butt_points (line->x + coords[2], line->y + coords[3],
						    line->x + coords[0], line->y + coords[1],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly, poly + 1, poly + 2, poly + 3);
		else if ((line->join == GDK_JOIN_MITER) && !changed_miter_to_bevel) {
			poly[0] = poly[6];
			poly[1] = poly[7];
			poly[2] = poly[4];
			poly[3] = poly[5];
		} else {
			foo_canvas_get_butt_points (line->x + coords[2], line->y + coords[3],
						    line->x + coords[0], line->y + coords[1],
						      width, FALSE,
						      poly, poly + 1, poly + 2, poly + 3);

			/* If this line uses beveled joints, then check the distance to a polygon
			 * comprising the last two points of the previous polygon and the first two
			 * from this polygon; this checks the wedges that fill the mitered point.
			 */

			if ((line->join == GDK_JOIN_BEVEL) || changed_miter_to_bevel) {
				poly[8] = poly[0];
				poly[9] = poly[1];

				dist = foo_canvas_polygon_to_point (poly, 5, x, y);
				if (dist < FOO_CANVAS_EPSILON) {
					best = 0.0;
					goto done;
				} else if (dist < best)
					best = dist;

				changed_miter_to_bevel = FALSE;
			}
		}

		if (i == 2)
			foo_canvas_get_butt_points (line->x + coords[0], line->y + coords[1],
						    line->x + coords[2], line->y + coords[3],
						      width, (line->cap == GDK_CAP_PROJECTING),
						      poly + 4, poly + 5, poly + 6, poly + 7);
		else if (line->join == GDK_JOIN_MITER) {
			if (!foo_canvas_get_miter_points (line->x + coords[0], line->y + coords[1],
							    line->x + coords[2], line->y + coords[3],
							    line->x + coords[4], line->y + coords[5],
							    width,
							    poly + 4, poly + 5, poly + 6, poly + 7)) {
				changed_miter_to_bevel = TRUE;
				foo_canvas_get_butt_points (line->x + coords[0], line->y + coords[1],
							    line->x + coords[2], line->y + coords[3],
							      width, FALSE,
							      poly + 4, poly + 5, poly + 6, poly + 7);
			}
		} else
			foo_canvas_get_butt_points (line->x + coords[0], line->y + coords[1],
						    line->x + coords[2], line->y + coords[3],
						      width, FALSE,
						      poly + 4, poly + 5, poly + 6, poly + 7);

		poly[8] = poly[0];
		poly[9] = poly[1];

		dist = foo_canvas_polygon_to_point (poly, 5, x, y);
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else if (dist < best)
			best = dist;
	}

	/* If caps are rounded, check the distance to the cap around the final end point of the line */

	if (line->cap == GDK_CAP_ROUND) {
		dx = line->x + coords[0] - x;
		dy = line->y + coords[1] - y;
		dist = sqrt (dx * dx + dy * dy) - width / 2.0;
		if (dist < FOO_CANVAS_EPSILON) {
			best = 0.0;
			goto done;
		} else
			best = dist;
	}

done:

	if ((line_points != static_points) && (line_points != line->coords))
		g_free (line_points);


#ifdef VERBOSE
	printf("foo_canvas_line_glyph_point x, y = (%g, %g);  distance = %g\n", x, y, best) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	return best;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static double
foo_canvas_line_glyph_point (FooCanvasItem *item, double x, double y, int cx, int cy, FooCanvasItem **actual_item)
{
        double result = 0.0 ; 
	FooCanvasLineGlyph *line;
	double *coords = NULL;
	double x1, y1, x2, y2;
	double hwidth;
	double dx, dy;

#ifdef VERBOSE
	g_print ("foo_canvas_line_glyph_point x, y = (%g, %g); cx, cy = (%d, %d)\n", x, y, cx, cy);
#endif

	line = FOO_CANVAS_LINE_GLYPH (item);

	*actual_item = item;

	/* Find the bounds for the rectangle plus its outline width */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	x1 = re->x1;
	y1 = re->y1;
	x2 = re->x2;
	y2 = re->y2;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* extract topleft/botright.... */
	coords = line->coords;
	x1 = line->x + coords[0];
	x2 = line->x + coords[2];
	if (coords[5] < coords[3])
	  {
	    y1 = line->y + coords[5];
	    y2 = line->y + coords[3];
	  }
	else
	  {
	    y1 = line->y + coords[3];
	    y2 = line->y + coords[5];
	  }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

	/* This is all about the line width....add later... */

	if (re->outline_set) {
		if (re->width_pixels)
			hwidth = (re->width / item->canvas->pixels_per_unit_x) / 2.0;
		else
			hwidth = re->width / 2.0;

		x1 -= hwidth;
		y1 -= hwidth;
		x2 += hwidth;
		y2 += hwidth;
	} else
		hwidth = 0.0;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        hwidth = 0.0;


	/* Is point inside rectangle (which can be hollow if it has no fill set)? */

	if ((x >= x1) && (y >= y1) && (x <= x2) && (y <= y2)) {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		if (re->fill_set || !re->outline_set)
			result = 0.0;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		result = 0.0 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		dx = x - x1;
		tmp = x2 - x;
		if (tmp < dx)
			dx = tmp;

		dy = y - y1;
		tmp = y2 - y;
		if (tmp < dy)
			dy = tmp;

		if (dy < dx)
			dx = dy;

		dx -= 2.0 * hwidth;

		if (dx < 0.0)
			result = 0.0;
		else
			result = dx;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
	else
	{
	/* Point is outside rectangle */

	if (x < x1)
		dx = x1 - x;
	else if (x > x2)
		dx = x - x2;
	else
		dx = 0.0;

	if (y < y1)
		dy = y1 - y;
	else if (y > y2)
		dy = y - y2;
	else
		dy = 0.0;

	result = sqrt (dx * dx + dy * dy);
	}


#ifdef VERBOSE
	printf("x,y:  %g,%g      distance:  %g\n", x, y, result) ;
#endif


	return result ;
}


static void
foo_canvas_line_glyph_translate (FooCanvasItem *item, double dx, double dy)
{
        FooCanvasLineGlyph *line;
        int i;
        double *coords;

        line = FOO_CANVAS_LINE_GLYPH (item);

        for (i = 0, coords = line->coords; i < line->num_points; i++, coords += 2) {
                coords[0] += dx;
                coords[1] += dy;
        }

}

static void
foo_canvas_line_glyph_bounds (FooCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	FooCanvasLineGlyph *line;

	line = FOO_CANVAS_LINE_GLYPH (item);

	if (line->num_points == 0) {
		*x1 = *y1 = *x2 = *y2 = 0.0;
		return;
	}

	get_bounds (line, x1, y1, x2, y2);

#ifdef VERBOSE
	printf("Glyph Bounds: %f %f -> %f %f\n", *x1, *y1, *x2, *y2) ;
#endif

}
