/*  File: foozmap-canvas-text.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#ifndef FOO_CANVAS_ZMAP_TEXT_H
#define FOO_CANVAS_ZMAP_TEXT_H


#include <libzmapfoocanvas/foo-canvas.h>


G_BEGIN_DECLS

/* Extended FooCanvasText Item.
 * Specialised to call a callback before drawing.
 */

#define FOO_TYPE_CANVAS_ZMAP_TEXT            (foo_canvas_zmap_text_get_type ())
#define FOO_CANVAS_ZMAP_TEXT(obj)            (GTK_CHECK_CAST ((obj), FOO_TYPE_CANVAS_ZMAP_TEXT, FooCanvasZMapText))
#define FOO_CANVAS_ZMAP_TEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), FOO_TYPE_CANVAS_ZMAP_TEXT, FooCanvasZMapTextClass))
#define FOO_IS_CANVAS_ZMAP_TEXT(obj)         (GTK_CHECK_TYPE ((obj), FOO_TYPE_CANVAS_ZMAP_TEXT))
#define FOO_IS_CANVAS_ZMAP_TEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), FOO_TYPE_CANVAS_ZMAP_TEXT))
#define FOO_CANVAS_ZMAP_TEXT_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), FOO_TYPE_CANVAS_ZMAP_TEXT, FooCanvasZMapTextClass))


typedef struct _FooCanvasZMapText FooCanvasZMapText;
typedef struct _FooCanvasZMapTextClass FooCanvasZMapTextClass;

typedef struct _FooCanvasZMapTextPrivate FooCanvasZMapTextPrivate;

typedef struct _ZMapTextDrawDataStruct *ZMapTextDrawData ;

typedef struct
{
  double spacing;		/* spacing between lines in canvas coord space. N.B. double! */
  int ch_width, ch_height;	/* individual cell dimensions (canvas size)*/
  int width, height;		/* table dimensions in number of cells */
  int untruncated_width;
  gboolean truncated;
}ZMapTextTableStruct, *ZMapTextTable;

typedef struct _ZMapTextDrawDataStruct
{
  double zx, zy;		/* zoom factor */
  double x1, x2, y1, y2;	/* scroll region */
  double wx, wy;		/* world position of item */
  ZMapTextTableStruct table;
} ZMapTextDrawDataStruct ;

struct _FooCanvasZMapText 
{
	FooCanvasText text;

	FooCanvasZMapTextPrivate *priv;	
};

struct _FooCanvasZMapTextClass
{
	FooCanvasItemClass parent_class;
};

typedef gint (* FooCanvasZMapAllocateCB)(FooCanvasItem   *item,
					 ZMapTextDrawData draw_data,
					 gint             max_width,
					 gint             buffer_size,
					 gpointer         user_data);

typedef gint (* FooCanvasZMapFetchTextCB)(FooCanvasItem   *item, 
					  ZMapTextDrawData draw_data,
					  char            *buffer, 
					  gint             buffer_size, 
					  gpointer         user_data);


typedef gboolean (* FooCanvasTextLineFunc)(FooCanvasItem   *item, /* foocanvas item */
					   PangoLayoutIter *iter, /* pango layout iter used for foreach iteration */
					   PangoLayoutLine *line, /* current line of iteration, saves pango_layout_iter_get_line() */
					   PangoRectangle  *logical_rect, /* logical rect of line as from pango_layout_iter_get_line_extents() */
					   int              iter_line_index, /* index of line in layout, pango_layout_get_line() compatible */
					   double           wx,	/* world x */
					   double           wy,	/* world y */
					   gpointer         user_data);	/* user data. */

/* Standard Gtk function */
GtkType foo_canvas_zmap_text_get_type (void) G_GNUC_CONST;

void foo_canvas_pango2item(FooCanvas *canvas, int px, int py, double *ix, double *iy);

int foo_canvas_zmap_text_calculate_zoom_buffer_size(FooCanvasItem   *item,
						    ZMapTextDrawData draw_data,
						    int              max_buffer_size);

int foo_canvas_item_world2text_index(FooCanvasItem *item, double x, double y);
int foo_canvas_item_item2text_index(FooCanvasItem *item, double x, double y);
gboolean foo_canvas_item_text_index2item(FooCanvasItem *item, 
					 int index, 
					 double *item_coords_out);

G_END_DECLS

#endif
