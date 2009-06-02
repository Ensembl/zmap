/*  File: zmapWindowLong_Items_P.h
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
 * Last edited: May 14 08:43 2009 (rds)
 * Created: Tue May 12 15:54:30 2009 (rds)
 * CVS info:   $Id: zmapWindowLongItems_P.h,v 1.1 2009-06-02 11:20:24 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_LONG_ITEMS_P_H
#define ZMAP_WINDOW_LONG_ITEMS_P_H


#include <glib.h>
#include <libfoocanvas/libfoocanvas.h>
#include <ZMap/zmapUtils.h>

enum
  {
    ZMAP_WINDOW_MAX_WINDOW = 30000
  };

/* Struct for the long items object, internal to long item code.
 * 
 * There is one long items object per ZMapWindow, it keeps a record of all items that
 * may need to be clipped before they are drawn. We shouldn't need to do this but the
 * foocanvas does not take care of any clipping that might be required by the underlying
 * window system. In the case of XWindows this amounts to clipping anything longer than
 * 32K pixels in size as the XWindows protocol cannot handle anything longer.
 * 
 *  */
typedef struct _ZMapWindowLong_ItemsStruct
{
  GHashTable *long_feature_items; /* hash table of LongFeatureItem keyed on FooCanvasItem */

  double max_zoom_x, max_zoom_y ;

  int item_count ;		/* Used to check our code is
				 * correctly creating/destroying items. */
} ZMapWindowLong_ItemsStruct, *ZMapWindowLong_Items ;



ZMapWindowLong_Items zmapWindowLong_ItemCreate   (double max_zoom_x, double max_zoom_y);
void                zmapWindowLong_ItemSetMaxZoom(ZMapWindowLong_Items long_items, 
						  double max_zoom_x, double max_zoom_y);
void                zmapWindowLong_ItemCheck     (ZMapWindowLong_Items long_items, 
						  FooCanvasItem      *item, 
						  double start, double end);
gboolean            zmapWindowLong_ItemCoords    (ZMapWindowLong_Items long_items, 
						  FooCanvasItem      *item,
						  double *start_out, double *end_out);
void                zmapWindowLong_ItemCrop      (ZMapWindowLong_Items long_items, 
						  double ppux, double ppuy,
						  double x1,   double y1,
						  double x2,   double y2);
void                zmapWindowLong_ItemPrint     (ZMapWindowLong_Items long_items);
gboolean            zmapWindowLong_ItemRemove    (ZMapWindowLong_Items long_items, 
						  FooCanvasItem      *item);
void                zmapWindowLong_ItemFree      (ZMapWindowLong_Items long_items);
void                zmapWindowLong_ItemDestroy   (ZMapWindowLong_Items long_items);


#endif /* ZMAP_WINDOW_LONG_ITEMS_P_H */

