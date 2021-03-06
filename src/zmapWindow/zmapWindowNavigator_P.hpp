/*  File: zmapWindowNavigator_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_WINDOW_NAVIGATOR_P_H
#define ZMAP_WINDOW_NAVIGATOR_P_H

#include <zmapWindow_P.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <ZMap/zmapWindowNavigator.hpp>

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
#define LOCATOR_DRAG      "green"
#define LOCATOR_FILL      "white"
#define LOCATOR_HIGHLIGHT "cyan"
#define LOCATOR_LINE_WIDTH 1

typedef struct _ZMapWindowNavigatorStruct
{
  ZMapWindowContainerGroup container_root ;		    /* what we'll raise and lower */
  ZMapWindowContainerGroup container_align;		    /* because I think we'll probably need it. */
  ZMapWindowContainerGroup container_block;		    /* that's where the locator goes */

  ZMapWindow      current_window;			    /* the current window... */

  FooCanvas 	*canvas;
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

  ZMapWindowFeaturesetItem locus_featureset;

  ZMapSpanStruct  full_span;    /* N.B. this is seqExtent !!! i.e. seq start -> seq end + 1!!! */

  double scaling_factor;        /* NAVIGTOR_SIZE / block length */

  double text_width, text_height;
  double left;

  double width, height;

  gboolean draw_locator, is_reversed, is_focus;

  gulong draw_expose_handler_id;

  gboolean locator_click;
  double   click_correction;


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
void zMapWindowNavigatorSetWindowNavigator(GtkWidget *widget,ZMapWindowNavigator navigator);
void zmapWindowNavigatorSizeRequest(GtkWidget *widget, double x, double y, double start, double end);
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

#define MH17_DEBUG_NAV_FOOBAR 0

#endif /* FOO_CANVAS_ITEM_STYLEEE */
