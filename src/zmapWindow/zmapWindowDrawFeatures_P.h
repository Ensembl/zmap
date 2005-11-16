/*  File: zmapWindowDrawFeatures_P.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 16 10:12 2005 (rds)
 * Created: Wed Nov 16 08:40:50 2005 (rds)
 * CVS info:   $Id: zmapWindowDrawFeatures_P.h,v 1.1 2005-11-16 10:43:00 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_DRAW_FEATURES_P_H
#define ZMAP_WINDOW_DRAW_FEATURES_P_H

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapUtilsDNA.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>
#include <math.h>


/* Used to pass data to canvas item menu callbacks. */
typedef struct
{
  gboolean item_cb ;					    /* TRUE => item callback,
							       FALSE => column callback. */

  ZMapWindow window ;
  FooCanvasItem *item ;

  ZMapFeatureSet feature_set ;				    /* Only used in column callbacks... */
} ItemMenuCBDataStruct, *ItemMenuCBData ;


/* these will go when scale is in separate window. */
#define SCALEBAR_OFFSET   0.0
#define SCALEBAR_WIDTH   50.0



/* parameters passed between the various functions drawing the features on the canvas, it's
 * simplest to cache the current stuff as we go otherwise the code becomes convoluted by having
 * to look up stuff all the time. */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow window ;
  FooCanvas *canvas ;

  /* Records which alignment, block, set, type we are processing. */
  ZMapFeatureContext full_context ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  GQuark curr_set_name ;

  GQuark curr_forward_col_id ;
  GQuark curr_reverse_col_id ;

  /* Records current positional information. */
  double curr_x_offset ;
  double curr_y_offset ;

  /* Records current canvas item groups, these are the direct parent groups of the display
   * types they contain, e.g. curr_root_group is the parent of the align */
  FooCanvasGroup *curr_root_group ;
  FooCanvasGroup *curr_align_group ;
  FooCanvasGroup *curr_block_group ;
  FooCanvasGroup *curr_forward_group ;
  FooCanvasGroup *curr_reverse_group ;
  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

  GdkColor colour_root ;
  GdkColor colour_alignment ;
  GdkColor colour_block ;
  GdkColor colour_mblock_for ;
  GdkColor colour_mblock_rev ;
  GdkColor colour_qblock_for ;
  GdkColor colour_qblock_rev ;
  GdkColor colour_mforward_col ;
  GdkColor colour_mreverse_col ;
  GdkColor colour_qforward_col ;
  GdkColor colour_qreverse_col ;

  double bump_for, bump_rev ;

} ZMapCanvasDataStruct, *ZMapCanvasData ;



/* Used to pass data to removeEmptyColumn function. */
typedef struct
{
  ZMapCanvasData canvas_data ;
  ZMapStrand strand ;
} RemoveEmptyColumnStruct, *RemoveEmptyColumn ;



typedef struct
{
  double offset ;				    /* I think we do need these... */
} PositionColumnStruct, *PositionColumn ;



static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;


static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext diff_context) ;
static void drawAlignments(GQuark key_id, gpointer data, gpointer user_data) ;
static void drawBlocks(gpointer data, gpointer user_data) ;
static void createSetColumn(gpointer data, gpointer user_data) ;
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    GQuark feature_set_id,
				    ZMapStrand strand,
				    double width,
				    double top, double bot, GdkColor *colour) ;
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data) ;
static void removeEmptyColumns(ZMapCanvasData canvas_data) ;
static void removeEmptyColumnCB(gpointer data, gpointer user_data) ;
static void positionColumns(ZMapCanvasData canvas_data) ;
static void positionColumnCB(gpointer data, gpointer user_data) ;



static ZMapWindowMenuItem makeMenuFeatureOps(int *start_index_inout,
					     ZMapWindowMenuItemCallbackFunc callback_func,
					     gpointer callback_data) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;
static ZMapWindowMenuItem makeMenuBump(int *start_index_inout,
				       ZMapWindowMenuItemCallbackFunc callback_func,
				       gpointer callback_data) ;
static void bumpMenuCB(int menu_item_id, gpointer callback_data) ;
static ZMapWindowMenuItem makeMenuDumpOps(int *start_index_inout,
					  ZMapWindowMenuItemCallbackFunc callback_func,
					  gpointer callback_data) ;
static void dumpMenuCB(int menu_item_id, gpointer callback_data) ;
static ZMapWindowMenuItem makeMenuDNAHomol(int *start_index_inout,
					   ZMapWindowMenuItemCallbackFunc callback_func,
					   gpointer callback_data) ;
static ZMapWindowMenuItem makeMenuProteinHomol(int *start_index_inout,
					   ZMapWindowMenuItemCallbackFunc callback_func,
					       gpointer callback_data) ;
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;

static void populateMenu(ZMapWindowMenuItem menu,
			 int *start_index_inout,
			 ZMapWindowMenuItemCallbackFunc callback_func,
			 gpointer callback_data) ;

static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window,
			 FooCanvasItem *item) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;
static gboolean dnaItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data);


static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			   FooCanvasItem *item, ZMapFeatureSet feature_set) ;

static ZMapWindowMenuItem makeMenuColumnOps(int *start_index_inout,
					    ZMapWindowMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;

static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline, GdkColor *background,
					ZMapWindow window) ;
static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, GdkColor *background,
					    GdkColor *block_background,
					    ZMapWindow window) ;
#ifdef RDS_DONT_INCLUDE
static FooCanvasItem *getBoundingBoxChild(FooCanvasGroup *parent_group) ;
#endif
static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item) ;

static void setColours(ZMapCanvasData canvas_data) ;


static void dna_redraw_callback(FooCanvasGroup *text_grp,
                                double zoom,
                                gpointer user_data);


static void dumpFASTA(ZMapWindow window) ;
static void dumpContext(ZMapWindow window) ;
static void pfetchEntry(ZMapWindow window, char *sequence_name) ;


static void dna_redraw_callback(FooCanvasGroup *text_grp,
                                double zoom,
                                gpointer user_data);
static void updateInfoGivenCoords(textGroupSelection select, 
                                  double worldCurrentX,
                                  double worldCurrentY);
static void pointerIsOverItem(gpointer data, gpointer user_data);
static textGroupSelection getTextGroupData(FooCanvasGroup *txtGroup);
ZMapDrawTextIterator getIterator(double win_min_coord, double win_max_coord,
                                 double offset_start,  double offset_end,
                                 double text_height, gboolean numbered);

#endif
