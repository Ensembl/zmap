/*  File: zmapWindowTextItem.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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

#ifndef ZMAP_WINDOW_TEXT_ITEM_H
#define ZMAP_WINDOW_TEXT_ITEM_H

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>

#define ZMAP_WINDOW_TEXT_ITEM_NAME "ZMapWindowTextItem"

#define ZMAP_TYPE_WINDOW_TEXT_ITEM           (zMapWindowTextItemGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_TEXT_ITEM(obj)       ((ZMapWindowTextItem) obj)
#define ZMAP_WINDOW_TEXT_ITEM_CONST(obj) ((ZMapWindowTextItem const) obj)
#else
#define ZMAP_WINDOW_TEXT_ITEM(obj)	     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItem))
#define ZMAP_WINDOW_TEXT_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItem const))
#endif
#define ZMAP_WINDOW_TEXT_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItemClass))
#define ZMAP_IS_WINDOW_TEXT_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_TEXT_ITEM))
#define ZMAP_WINDOW_TEXT_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_TEXT_ITEM, zmapWindowTextItemClass))


/* Property names. */
#define PROP_REFSEQ_START_STR           "refseq-start"
#define PROP_REFSEQ_END_STR             "refseq-end"
#define PROP_TEXT_LENGTH_STR            "text-length"
#define PROP_TEXT_ALLOCATE_FUNC_STR     "allocate-func"
#define PROP_TEXT_FETCH_TEXT_FUNC_STR   "fetch-text-func"
#define PROP_TEXT_CALLBACK_DATA_STR     "callback-data"
#define PROP_TEXT_SELECT_COLOR_GDK_STR  "select-color-gdk"
#define PROP_TEXT_REQUESTED_WIDTH_STR   "full-width"
#define PROP_TEXT_REQUESTED_HEIGHT_STR  "full-height"
#define PROP_WRAP_MODE_STR              "wrap-mode"
#define PROP_TEXT_CHANGED_STR           "text-changed"


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
}ZMapTextItemTableStruct, *ZMapTextItemTable ;


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
GType zMapWindowTextItemGetType(void) ;
gboolean zMapWindowTextItemText2WorldCoords(ZMapWindowTextItem text_item,
					    int text_start, int text_end,
					    double *x1, double *y1, double *x2, double *y2) ;
void zMapWindowTextItemSelect(ZMapWindowTextItem text_item,
			      int start, int end, gboolean deselect_first, gboolean emit_signal) ;
void zMapWindowTextItemDeselect(ZMapWindowTextItem text_item, gboolean emit_signal) ;
int zMapWindowTextItemCalculateZoomBufferSize(FooCanvasItem   *item,
					      ZMapTextItemDrawData draw_data, int max_buffer_size) ;



#endif /* ZMAP_WINDOW_TEXT_ITEM_H */
