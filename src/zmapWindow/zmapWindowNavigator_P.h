/*  File: zmapWindowNavigator_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Jan 17 11:04 2007 (rds)
 * Created: Thu Sep  7 09:23:47 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigator_P.h,v 1.8 2007-01-23 18:01:57 rds Exp $
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_WINDOW_NAVIGATOR_P_H
#define ZMAP_WINDOW_NAVIGATOR_P_H

#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemFactory.h>
#include <ZMap/zmapWindowNavigator.h>

#define NAVIGATOR_SIZE 25000

#define SHIFT_COLUMNS_LEFT   (6.0)
#define ROOT_CHILD_SPACING   (2.0)
#define ALIGN_CHILD_SPACING  (2.0)
#define BLOCK_CHILD_SPACING  (2.0)
#define STRAND_CHILD_SPACING (2.0)
#define SET_CHILD_SPACING    (2.0)

#define USE_BACKGROUNDS   FALSE
#define ROOT_BACKGROUND   "red"
#define ALIGN_BACKGROUND  "green"
#define BLOCK_BACKGROUND  "white"
#define STRAND_BACKGROUND "blue"
#define COLUMN_BACKGROUND "grey"

#define LOCATOR_BORDER    "darkred"
#define LOCATOR_DRAG      "white"
#define LOCATOR_FILL      "grey"
#define LOCATOR_LINE_WIDTH 1

typedef struct _ZMapWindowNavigatorStruct
{
  FooCanvasGroup *container_root ; /* what we'll raise and lower */
  FooCanvasGroup *container_align; /* because I think we'll probably need it. */

  ZMapWindowFToIFactory item_factory;

  ZMapWindow      current_window; /* the current window... */

  FooCanvasGroup *locator_group;
  FooCanvasItem  *locator;
  FooCanvasItem  *locator_drag;
  GdkColor        locator_fill_gdk;
  GdkColor        locator_border_gdk;
  GdkColor        locator_drag_gdk;
  GdkBitmap      *locator_stipple;
  guint           locator_bwidth;
  double          locator_x1, locator_x2; /* width */
  ZMapSpanStruct  locator_span;           /* height */

  GdkColor        root_background;
  GdkColor        align_background;
  GdkColor        block_background;
  GdkColor        strand_background;
  GdkColor        column_background;

  GHashTable     *ftoi_hash;
  GHashTable     *locus_display_hash;

  GList          *feature_set_names;

  ZMapSpanStruct  full_span;    /* N.B. this is seqExtent !!! i.e. seq start -> seq end + 1!!! */

  double scaling_factor;        /* NAVIGTOR_SIZE / block length */

  double text_width, text_height;
  double left;

  gboolean draw_locator, is_reversed;

}ZMapWindowNavigatorStruct;


typedef struct
{
  ZMapWindowNavigator navigate;
  gboolean item_cb ;					    /* TRUE => item callback,
							       FALSE => column callback. */
  FooCanvasItem *item ;

  ZMapFeatureSet feature_set ;				    /* Only used in column callbacks... */
} NavigateMenuCBDataStruct, *NavigateMenuCBData ;

void zmapWindowNavigatorPositioning(ZMapWindowNavigator navigate);

void zmapWindowNavigatorGoToLocusExtents(ZMapWindowNavigator navigate, FooCanvasItem *item);

/* Menu prototypes... */
void zmapWindowNavigatorShowSameNameList(ZMapWindowNavigator navigate, FooCanvasItem *item);
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuLocusOps(int *start_index_inout,
                                                    ZMapGUIMenuItemCallbackFunc callback_func,
                                                    gpointer callback_data);
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuColumnOps(int *start_index_inout,
                                                     ZMapGUIMenuItemCallbackFunc callback_func,
                                                     gpointer callback_data);
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuBump(int *start_index_inout,
                                                ZMapGUIMenuItemCallbackFunc callback_func,
                                                gpointer callback_data, ZMapStyleOverlapMode curr_overlap);


/* WIDGET STUFF */
void zmapWindowNavigatorSizeRequest(GtkWidget *widget, double x, double y);
void zmapWindowNavigatorFillWidget(GtkWidget *widget);
void zmapWindowNavigatorValueChanged(GtkWidget *widget, double top, double bottom);
void zmapWindowNavigatorTextSize(GtkWidget *widget, double *x, double *y);

#endif /*  ZMAP_WINDOW_NAVIGATOR_P_H  */

