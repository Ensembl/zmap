/*  File: zmapWindowTextItem.h
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
 * Last edited: Jun 17 08:38 2009 (rds)
 * Created: Fri Jan 16 14:01:12 2009 (rds)
 * CVS info:   $Id: zmapWindowTextItem.h,v 1.1 2009-06-17 09:46:16 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_TEXT_ITEM_H
#define ZMAP_WINDOW_TEXT_ITEM_H

#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>

#define ZMAP_WINDOW_TEXT_ITEM_NAME "ZMapWindowTextItem"

#define ZMAP_TYPE_WINDOW_TEXT_ITEM           (zMapWindowTextItemGetType())
#define ZMAP_WINDOW_TEXT_ITEM(obj)	     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItem))
#define ZMAP_WINDOW_TEXT_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItem const))
#define ZMAP_WINDOW_TEXT_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItemClass))
#define ZMAP_IS_WINDOW_TEXT_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM))
#define ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItemClass))



/* Instance */
typedef struct _zmapWindowTextItemStruct  zmapWindowTextItem, *ZMapWindowTextItem ;


/* Class */
typedef struct _zmapWindowTextItemClassStruct  zmapWindowTextItemClass, *ZMapWindowTextItemClass ;


typedef struct
{
  double spacing;		/* spacing between lines in canvas coord space. N.B. double! */
  int ch_width, ch_height;	/* individual cell dimensions (canvas size)*/
  int width, height;		/* table dimensions in number of cells */
  int untruncated_width;
  gboolean truncated;
}ZMapTextItemTableStruct, *ZMapTextItemTable;

typedef struct _ZMapTextItemDrawDataStruct
{
  double zx, zy;		/* zoom factor */
  double x1, x2, y1, y2;	/* scroll region */
  double wx, wy;		/* world position of item */
  ZMapTextItemTableStruct table;
} ZMapTextItemDrawDataStruct, *ZMapTextItemDrawData ;


typedef gint (* ZMapTextItemAllocateCB)(FooCanvasItem   *item,
					ZMapTextItemDrawData draw_data,
					gint             max_width,
					gint             buffer_size,
					gpointer         user_data);

typedef gint (* ZMapTextItemFetchTextCB)(FooCanvasItem   *item, 
					 ZMapTextItemDrawData draw_data,
					 char            *buffer, 
					 gint             buffer_size, 
					 gpointer         user_data);


typedef gboolean (* ZMapWindowTextItemSelectionCB)(ZMapWindowTextItem text_item,
						   double x1, double y1,
						   double x2, double y2,
						   int start, int end);

/* Public funcs */
GType zMapWindowTextItemGetType(void);


void foo_canvas_pango2item(FooCanvas *canvas, int px, int py, double *ix, double *iy);

int zmap_window_text_item_calculate_zoom_buffer_size(FooCanvasItem   *item,
						     ZMapTextItemDrawData draw_data,
						     int              max_buffer_size);

int foo_canvas_item_world2text_index(FooCanvasItem *item, double x, double y);
int foo_canvas_item_item2text_index(FooCanvasItem *item, double x, double y);
gboolean foo_canvas_item_text_index2item(FooCanvasItem *item, 
					 int index, 
					 double *item_coords_out);

#endif /* ZMAP_WINDOW_TEXT_ITEM_H */
