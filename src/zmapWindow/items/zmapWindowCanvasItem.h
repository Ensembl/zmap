/*  File: zmapWindowCanvasItem.h
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
#ifndef ZMAP_WINDOW_CANVAS_ITEM_H
#define ZMAP_WINDOW_CANVAS_ITEM_H

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapStyle.h>



/* The type name for the ZMapWindowCanvasItem GType */
#define ZMAP_WINDOW_CANVAS_ITEM_NAME 	"ZMapWindowCanvasItem"

/* GParamSpec names */
#define ZMAP_WINDOW_CANVAS_INTERVAL_TYPE "interval-type"

#define ZMAP_TYPE_CANVAS_ITEM           (zMapWindowCanvasItemGetType())

#if GOBJ_CAST
#define ZMAP_CANVAS_ITEM(obj)         ((ZMapWindowCanvasItem) obj)
#define ZMAP_CANVAS_ITEM_CONST(obj)   ((ZMapWindowCanvasItem const) obj)
#else
#define ZMAP_CANVAS_ITEM(obj)	        (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CANVAS_ITEM, zmapWindowCanvasItem))
#define ZMAP_CANVAS_ITEM_CONST(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), ZMAP_TYPE_CANVAS_ITEM, zmapWindowCanvasItem const))
#endif

#define ZMAP_CANVAS_ITEM_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST((klass),  ZMAP_TYPE_CANVAS_ITEM, zmapWindowCanvasItemClass))
#define ZMAP_IS_CANVAS_ITEM(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), ZMAP_TYPE_CANVAS_ITEM))
#define ZMAP_CANVAS_ITEM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj),  ZMAP_TYPE_CANVAS_ITEM, zmapWindowCanvasItemClass))



/* Instance */
typedef struct _zmapWindowCanvasItemStruct  zmapWindowCanvasItem, *ZMapWindowCanvasItem ;


/* Class */
typedef struct _zmapWindowCanvasItemClassStruct  zmapWindowCanvasItemClass, *ZMapWindowCanvasItemClass ;


#include <zmapWindowCanvasFeatureset.h>	/* need typedefs to stop gcc from barfing */




/* Public funcs */
GType zMapWindowCanvasItemGetType(void);

gboolean zMapWindowCanvasItemIsConnected(ZMapWindowCanvasItem item);
void zMapWindowCanvasItemSetConnected(ZMapWindowCanvasItem item, gboolean val);


ZMapFeature zMapWindowCanvasItemGetFeature(FooCanvasItem *any_feature_item) ;


void zmapWindowCanvasItemGetColours(ZMapFeatureTypeStyle style, ZMapStrand strand, ZMapFrame frame,
      ZMapStyleColourType    colour_type,
      GdkColor **fill, GdkColor **draw, GdkColor **outline,
      GdkColor              *default_fill,
      GdkColor              *border);



FooCanvasItem *zMapWindowCanvasItemGetInterval(ZMapWindowCanvasItem canvas_item,
					       double x, double y,
					       ZMapFeatureSubPartSpan *sub_feature_out);
ZMapWindowCanvasItem zMapWindowCanvasItemIntervalGetObject(FooCanvasItem *item);
ZMapFeatureSubPartSpan zMapWindowCanvasItemIntervalGetData(FooCanvasItem *item, ZMapFeature feature, double x, double y);

gboolean zMapWindowCanvasItemIsMasked(ZMapWindowCanvasItem item,gboolean andHidden);

void zMapWindowCanvasItemSetIntervalColours(FooCanvasItem *canvas_item, ZMapFeature feature, ZMapFeatureSubPartSpan sub_feature,
					    ZMapStyleColourType colour_type,
					    int colour_flags,
					    GdkColor *default_fill_colour,
                                  GdkColor *border_colour) ;

gboolean zMapWindowCanvasItemSetFeature(ZMapWindowCanvasItem item, double x, double y);
gboolean zMapWindowCanvasItemSetFeaturePointer(ZMapWindowCanvasItem item, ZMapFeature feature);

gboolean zMapWindowCanvasItemSetStyle(ZMapWindowCanvasItem item, ZMapFeatureTypeStyle style);


gboolean zMapWindowCanvasItemSetFeaturePointer(ZMapWindowCanvasItem item, ZMapFeature feature);
gboolean zMapWindowCanvasItemShowHide(ZMapWindowCanvasItem item, gboolean show);



#endif /* ZMAP_WINDOW_CANVAS_ITEM_H */
