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

/* Glyphs made from lines, they are constant scale so show at all zoom levels.
 *
 * FooCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 *
 * Author: Ed Griffiths <edgrif@sanger.ac.uk>
 */

#ifndef FOOZMAP_CANVAS_LINE_GLYPH_H
#define FOOZMAP_CANVAS_LINE_GLYPH_H


#include <libfoocanvas/foo-canvas.h>


G_BEGIN_DECLS


/* Line glyph item for the canvas.  This is a glyph made up from lines with configurable width, cap/join styles.
 *
 * The following object arguments are available:
 *
 * name			type			read/write	description
 * ------------------------------------------------------------------------------------------

 * CURRENTLY NOT TRUE...ITS THE POSITION OF THE LEFT/MIDDLE OF FEATURE....
 * x			double		        RW	        X position of middle of glyph.
 * y			double		        RW	        Y position of middle of glyph.



 * points		FooCanvasPoints*	RW		Pointer to a FooCanvasPoints structure.
 *								This can be created by a call to
 *								foo_canvas_points_new() (in foo-canvas-util.h).
 *								X coordinates are in the even indices of the
 *								points->coords array, Y coordinates are in
 *								the odd indices.
 * fill_color		string			W		X color specification for line.
 * fill_color_gdk	GdkColor*		RW		Pointer to an allocated GdkColor.
 * fill_stipple		GdkBitmap*		RW		Stipple pattern for the line.
 * width_pixels		uint			R		Width of the line in pixels. The line width
 *								will not be scaled when the canvas zoom factor changes.
 * cap_style		GdkCapStyle		RW		Cap ("endpoint") style for the line.
 * join_style		GdkJoinStyle		RW		Join ("vertex") style for the line.
 * line_style		GdkLineStyle		RW		Line dash style
 */


#define FOO_TYPE_CANVAS_LINE_GLYPH            (foo_canvas_line_glyph_get_type ())

#if GOBJ_CAST
#define FOO_CANVAS_LINE_GLYPH(obj)            (FooCanvasLineGlyph) (obj)
#else
#define FOO_CANVAS_LINE_GLYPH(obj)            (GTK_CHECK_CAST ((obj), FOO_TYPE_CANVAS_LINE_GLYPH, FooCanvasLineGlyph))
#endif

#define FOO_CANVAS_LINE_GLYPH_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), FOO_TYPE_CANVAS_LINE_GLYPH, FooCanvasLineGlyphClass))
#define FOO_IS_CANVAS_LINE_GLYPH(obj)         (GTK_CHECK_TYPE ((obj), FOO_TYPE_CANVAS_LINE_GLYPH))
#define FOO_IS_CANVAS_LINE_GLYPH_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), FOO_TYPE_CANVAS_LINE_GLYPH))
#define FOO_CANVAS_LINE_GLYPH_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), FOO_TYPE_CANVAS_LINE_GLYPH, FooCanvasLineGlyphClass))


typedef struct _FooCanvasLineGlyph FooCanvasLineGlyph;
typedef struct _FooCanvasLineGlyphClass FooCanvasLineGlyphClass;

struct _FooCanvasLineGlyph {
	FooCanvasItem item;

        double x, y ;  		/* Position of item */

	double *coords;		/* Array of coordinates for the line_glyph's points.  X coords are in the
				 * even indices, Y coords are in the odd indices. This is derived
				 * from x,y and offsets. */

	GdkGC *gc;		/* GC for drawing line_glyph */

	GdkBitmap *stipple;	/* Stipple pattern */

	double width;		/* Width of the line_glyph */

	GdkCapStyle cap;	/* Cap style for line_glyph */
	GdkJoinStyle join;	/* Join style for line_glyph */
	GdkLineStyle line_style;/* Style for the line_glyph */

	gulong fill_pixel;	/* Color for line_glyph */

	guint32 fill_rgba;	/* RGBA color for outline */ /*AA*/

	int num_points;		/* Number of points in the line_glyph */
	guint fill_color;	/* Fill color, RGBA */

};

struct _FooCanvasLineGlyphClass {
	FooCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType foo_canvas_line_glyph_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif
