/*  File: zmapWindowGraphItem_I.h
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H
#define ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H

#include <glib.h>
#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowCanvasItem_I.h>
#include <zmapWindowGraphDensityItem.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapSkipList.h>




/* cut down version of foo-canvas-RE for internal data
 * NOTE we also handle lines by mangling the coordinates
 */
/* NOTE: not a GObject, no GdkGC's */
/* this is probably the wrong header */
typedef struct _zmapWindowCanvasGraphSegment
{
      ZMapFeature feature;

      double x1, y1, x2, y2;        /* Corners of item (box or line) */
      double width;                 /* Outline width */


      guint fill_colour;             /* Fill color, RGBA */
      guint outline_colour;          /* Outline color, RGBA */
      gulong fill_pixel;            /* Fill color */
      gulong outline_pixel;         /* Outline color */

      /* Configuration flags */

      gboolean line;          /* line else is box */

      gboolean fill_set;    /* Is fill color set? */
      gboolean outline_set; /* Is outline color set? */
      gboolean width_pixels;      /* Is outline width specified in pixels or units? */
} ZMapWindowCanvasGraphSegmentStruct, *ZMapWindowCanvasGraphSegment;


ZMapWindowCanvasGraphSegment zmapWindowCanvasGraphSegmentAlloc(void);
void zmapWindowCanvasGraphSegmentFree(gpointer thing);	/* is used as a callback, needs to be generic */

typedef struct _zmapWindowGraphDensityItemClassStruct
{
  zmapWindowCanvasItemClass __parent__;

  GHashTable *density_items;         /* singleton canvas items per column, indexed by unique id */
  GList *graph_segment_free_list;
#define N_GS_ALLOC      100

} zmapWindowGraphDensityItemClassStruct;



typedef struct _zmapWindowGraphDensityItemStruct
{
  zmapWindowCanvasItem __parent__;

  GQuark id;
  ZMapFeatureTypeStyle style;


  GList *source_bins;
  gboolean source_sorted;
  gboolean re_bin;		/* re-calculate bins according to zoom */
  gboolean overlap;
  int max_overlap;		/* size of the longest item */

  GList *display_bins;
  ZMapSkipList display_index;

      /* graphics context for all contained features
       * each one has its own colours that we set on draw (eg for heatmaps)
       * foo_canvas_rect also has stipples but we don't use then ATM
       * it alsoe pretneds to do alpha but 'X doesn't do it'
       * crib foo-canvas-rect-elipse.c for inspiration
       */
  GdkGC *gc;               /* GC for filling */


} zmapWindowGraphDensityItemStruct;




#endif /* ZMAP_WINDOW_GRAPH_DENSITY_ITEM_I_H */
