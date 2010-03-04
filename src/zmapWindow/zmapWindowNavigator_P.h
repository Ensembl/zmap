/*  File: zmapWindowNavigator_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 10 10:48 2009 (rds)
 * Created: Thu Sep  7 09:23:47 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigator_P.h,v 1.15 2010-03-04 15:13:15 mh17 Exp $
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_WINDOW_NAVIGATOR_P_H
#define ZMAP_WINDOW_NAVIGATOR_P_H

#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
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
#define LOCATOR_HIGHLIGHT "white"
#define LOCATOR_LINE_WIDTH 1

typedef struct _ZMapWindowNavigatorStruct
{
  ZMapWindowContainerGroup container_root ; /* what we'll raise and lower */
  ZMapWindowContainerGroup container_align; /* because I think we'll probably need it. */

  ZMapWindowFToIFactory item_factory;

  ZMapWindow      current_window; /* the current window... */

  FooCanvasItem  *locator;
  FooCanvasItem  *locator_drag;
  GdkColor        locator_fill_gdk;
  GdkColor        locator_border_gdk;
  GdkColor        locator_drag_gdk;
  GdkColor        locator_highlight;
  GdkBitmap      *locator_stipple;
  guint           locator_bwidth;
  ZMapSpanStruct  locator_x_coords; /* width */
  ZMapSpanStruct  locator_y_coords; /* height */

  GdkColor        root_background;
  GdkColor        align_background;
  GdkColor        block_background;
  GdkColor        strand_background;
  GdkColor        column_background;

  GHashTable     *ftoi_hash;
  GHashTable     *locus_display_hash;

  GQuark          locus_id;
  GList          *feature_set_names;

  GList          *hide_filter;
  GList          *available_filters;

  ZMapSpanStruct  full_span;    /* N.B. this is seqExtent !!! i.e. seq start -> seq end + 1!!! */

  double scaling_factor;        /* NAVIGTOR_SIZE / block length */

  double text_width, text_height;
  double left;

  double width, height;

  gboolean draw_locator, is_reversed, is_focus;

  gulong draw_expose_handler_id;
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
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuLocusColumnOps(int *start_index_inout,
							  ZMapGUIMenuItemCallbackFunc callback_func,
							  gpointer callback_data);
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuColumnOps(int *start_index_inout,
                                                     ZMapGUIMenuItemCallbackFunc callback_func,
                                                     gpointer callback_data);
ZMapGUIMenuItem zmapWindowNavigatorMakeMenuBump(int *start_index_inout,
                                                ZMapGUIMenuItemCallbackFunc callback_func,
                                                gpointer callback_data, ZMapStyleBumpMode curr_bump);

void zmapWindowNavigatorLocusRedraw(ZMapWindowNavigator navigate);

/* WIDGET STUFF */
void zmapWindowNavigatorSizeRequest(GtkWidget *widget, double x, double y);
void zmapWindowNavigatorFillWidget(GtkWidget *widget);
void zmapWindowNavigatorValueChanged(GtkWidget *widget, double top, double bottom);
void zmapWindowNavigatorTextSize(GtkWidget *widget, double *x, double *y);

#endif /*  ZMAP_WINDOW_NAVIGATOR_P_H  */

#ifdef FOO_CANVAS_ITEM_STYLEEE

#define ZMAP_TYPE_WINDOW_NAVIGATOR            (zMapWindowNavigatorGetType())
#define ZMAP_WINDOW_NAVIGATOR(obj)            (GTK_CHECK_CAST ((obj), ZMAP_TYPE_WINDOW_NAVIGATOR, ZMapWindowNavigatorG))
#define ZMAP_WINDOW_NAVIGATOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), ZMAP_TYPE_WINDOW_NAVIGATOR, ZMapWindowNavigatorGClass))
#define ZMAP_IS_WINDOW_NAVIGATOR(obj)         (GTK_CHECK_TYPE ((obj), ZMAP_TYPE_WINDOW_NAVIGATOR))
#define ZMAP_IS_WINDOW_NAVIGATOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), ZMAP_TYPE_WINDOW_NAVIGATOR))
#define ZMAP_WINDOW_NAVIGATOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), ZMAP_TYPE_WINDOW_NAVIGATOR, ZMapWindowNavigatorGClass))


typedef struct _ZMapWindowNavigatorG      ZMapWindowNavigatorG;
typedef struct _ZMapWindowNavigatorGClass ZMapWindowNavigatorGClass;

struct _ZMapWindowNavigatorG {
	FooCanvasItem item;
};

struct _ZMapWindowNavigatorGClass {
	FooCanvasItemClass parent_class;
};

#endif /* FOO_CANVAS_ITEM_STYLEEE */
