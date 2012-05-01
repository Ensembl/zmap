/*  File: zmapWindowLongItem.h
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

#ifndef __ZMAP_WINDOW_LONG_ITEM_H__
#define __ZMAP_WINDOW_LONG_ITEM_H__

#define ZMAP_WINDOW_LONG_ITEM_NAME "ZMapWindowLongItem"

#define ZMAP_TYPE_WINDOW_LONG_ITEM           (zMapWindowLongItemGetType())

#if GOBJ_CAST
#define ZMAP_WINDOW_LONG_ITEM(obj)       ((ZMapWindowLongItem) obj)
#define ZMAP_WINDOW_LONG_ITEM_CONST(obj) ((ZMapWindowLongItem const) obj)
#else
#define ZMAP_WINDOW_LONG_ITEM(obj)	     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_LONG_ITEM, zmapWindowLongItem))
#define ZMAP_WINDOW_LONG_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_WINDOW_LONG_ITEM, zmapWindowLongItem const))
#endif

#define ZMAP_WINDOW_LONG_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_WINDOW_LONG_ITEM, zmapWindowLongItemClass))
#define ZMAP_IS_WINDOW_LONG_ITEM(obj)        (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_WINDOW_LONG_ITEM))
#define ZMAP_WINDOW_LONG_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_WINDOW_LONG_ITEM, zmapWindowLongItemClass))


/* Instance */
typedef struct _zmapWindowLongItemStruct  zmapWindowLongItem, *ZMapWindowLongItem ;


/* Class */
typedef struct _zmapWindowLongItemClassStruct  zmapWindowLongItemClass, *ZMapWindowLongItemClass ;


/* Public funcs */
GType zMapWindowLongItemGetType(void);

FooCanvasItem *zmapWindowLongItemCheckPoint(FooCanvasItem   *possibly_long_item);
FooCanvasItem *zmapWindowLongItemCheckPointFull(FooCanvasItem   *possibly_long_item,
						FooCanvasPoints *points,
						double x1, double y1,
						double x2, double y2);

int zmapWindowIsLongItem(FooCanvasItem *foo);
FooCanvasItem *zmapWindowGetLongItem(FooCanvasItem *foo);


#endif /* __ZMAP_WINDOW_LONG_ITEM_H__ */
