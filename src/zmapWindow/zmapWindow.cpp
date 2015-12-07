/*  File: zmapWindow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Provides interface functions for controlling a data
 *              display window.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <math.h>
#include <string.h>
#include <unistd.h>
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapFASTA.hpp>
#include <ZMap/zmapPeptide.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapSOParser.hpp>
#include <zmapWindowState.hpp>
#include <zmapWindowContainers.hpp>
#include <zmapWindowCanvasDraw.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindowCanvasFeatureset.hpp>
#include <zmapWindow_P.hpp>
#include <ZMap/zmapGFF.hpp>



/* If zoom factor less than this then we don't do it. */
#define ZOOM_SENSITIVITY 5.0


/* Local struct to hold current features and new_features obtained from a server and
 * relevant types. */
typedef struct FeatureSetsStateStructType
{
  ZMapFeatureContext current_features ;
  ZMapFeatureContext new_features ;
  ZMapFeature highlight_feature ;
  gboolean splice_highlight_on ;
  GHashTable *all_styles ;
  GHashTable *new_styles ;
  GHashTable *featuresets_2_stylelist ;
  GHashTable *featureset_2_column;
  GHashTable *source_2_sourcedata;
  GHashTable *columns;
  GList *masked;
  ZMapWindowState state ;                                   /* Can be NULL! */
} FeatureSetsStateStruct, *FeatureSetsState ;


/* Used for passing information to the locked display hash callback functions. */
typedef enum {ZMAP_LOCKED_ZOOMING, ZMAP_LOCKED_MOVING, ZMAP_LOCKED_ZOOM_TO } ZMapWindowLockActionType ;
typedef struct
{
  ZMapWindow window ;
  ZMapWindowLockActionType type;
  /* Either  */
  double start ;
  double end ;
  /* Or depending on type */
  double zoom_factor ;
  double position ;
  /* or both */

  gboolean border;
  int border_size;

} LockedDisplayStruct, *LockedDisplay ;


typedef struct ScrollToStructType
{
  int x ;
  int y ;
} ScrollToStruct, *ScrollTo ;



/* Used for showing ruler on all windows that are in a vertically locked group. */
typedef enum
  {
    ZMAP_LOCKED_RULER_SETUP,
    ZMAP_LOCKED_RULER_MOVING,
    ZMAP_LOCKED_RULER_REMOVE,

    ZMAP_LOCKED_MARK_GUIDE_SETUP,
    ZMAP_LOCKED_MARK_GUIDE_MOVING,
    ZMAP_LOCKED_MARK_GUIDE_REMOVE,
  } ZMapWindowLockRulerActionType ;

typedef struct
{
  ZMapWindowLockRulerActionType action ;

  double origin_y ;

  double world_x, world_y ;
  char *tip_text ;
} LockedRulerStruct, *LockedRuler ;





/* This struct is used to pass data to realizeHandlerCB
 * via the g_signal_connect on the canvas's expose_event. */
typedef struct _RealiseDataStruct
{
  ZMapWindow window ;
  FeatureSetsState feature_sets ;
} RealiseDataStruct, *RealiseData ;



typedef struct
{
  double rootx1, rootx2, rooty1, rooty2 ;
} MaxBoundsStruct, *MaxBounds ;


typedef struct
{
  unsigned int in_mark_move_region : 1;
  unsigned int activated : 1;
  double mark_y1, mark_y2;
  double mark_x1, mark_x2;
  double *closest_to;
  GdkCursor *arrow_cursor;
}MarkRegionUpdateStruct;



struct fooBug
{
  void *key;
  char *id;
};





static ZMapWindow myWindowCreate(GtkWidget *parent_widget,
                                 ZMapFeatureSequenceMap sequence, void *app_data,
                                 GList *feature_set_names,
                                 GtkAdjustment *hadjustment,
                                 GtkAdjustment *vadjustment,
                                 int *int_values) ;
static void myWindowZoom(ZMapWindow window, double zoom_factor, double curr_pos) ;
static void myWindowMove(ZMapWindow window, double start, double end) ;

static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEvent *event, gpointer data) ;



//static gboolean pressCB(GtkWidget *widget, GdkEventButton *event, gpointer user_data) ;
//static gboolean motionCB(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) ;
//static gboolean releaseCB(GtkWidget *widget, GdkEventButton *event, gpointer user_data) ;

static void dragDataGetCB(GtkWidget *widget,
                          GdkDragContext *drag_context,
                          GtkSelectionData *selectionData,
                          guint info,
                          guint time,
                          gpointer user_data) ;

static gboolean keyboardEvent(ZMapWindow window, GdkEventKey *key_event) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean canvasRootEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void canvasSizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static gboolean windowGeneralEventCB(GtkWidget *wigdet, GdkEvent *event, gpointer data);

static void setZoomFromLayoutSize(ZMapWindow window,
                                  int *layout_win_width_out, int *layout_win_height_out,
                                  int *layout_binwin_width_out, int *layout_binwin_height_out) ;



static void resetCanvas(ZMapWindow window, gboolean free_child_windows, gboolean keep_revcomp_safe_windows) ;
static gboolean getConfiguration(ZMapWindow window) ;
static void sendClientEvent(ZMapWindow window, FeatureSetsState feature_sets, gpointer loaded_cb_user_data) ;

static void moveWindow(ZMapWindow window, GdkEventKey *key_event) ;
static void scrollWindow(ZMapWindow window, GdkEventKey *key_event) ;
static void changeRegion(ZMapWindow window, guint keyval) ;

static void zoomWindow(ZMapWindow window, GdkEventKey *key_event) ;

static void setCurrLock(ZMapWindowLockType window_locking, ZMapWindow window,
                        GtkAdjustment **hadjustment, GtkAdjustment **vadjustment) ;
static void copyLockWindow(ZMapWindow original_window, ZMapWindow new_window) ;
static void lockWindow(ZMapWindow window, ZMapWindowLockType window_locking) ;
static void unlockWindow(ZMapWindow window, gboolean no_destroy_if_empty) ;
static void removeLastLockWindow(GHashTable *sibling_locked_windows) ;

static GtkAdjustment *copyAdjustmentObj(GtkAdjustment *orig_adj) ;

static void lockedDisplayCB(gpointer key, gpointer value, gpointer user_data) ;
static void scrollToCB(gpointer key, gpointer value, gpointer user_data) ;

static void zoomToRubberBandArea(ZMapWindow window);

static void getMaxBounds(gpointer data, gpointer user_data) ;

static void jumpFeature(ZMapWindow window, guint keyval) ;
static void jumpColumn(ZMapWindow window, guint keyval) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void swapColumns(ZMapWindow window, guint keyval);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void unhideItemsCB(gpointer data, gpointer user_data) ;


#if POINTLESS
static gboolean possiblyPopulateWithChildData(ZMapWindow window,
                                              FooCanvasItem *feature_item,
//                                              FooCanvasItem *highlight_item,
                                              ZMapFeatureSubpartType *sub_type,
                                              int *selected_start, int *selected_end, int *selected_length) ;

static gboolean possiblyPopulateWithFullData(ZMapWindow window,
                                             ZMapFeature feature,
                                             FooCanvasItem *feature_item,
//                                             FooCanvasItem *highlight_item,
                                             int *feature_start, int *feature_end,
                                             int *feature_length,
                                             int *selected_start, int *selected_end,
                                             int *selected_length);
#endif


static gboolean checkItem(FooCanvasItem *item, gpointer user_data) ;


static void popUpMenu(GdkEventKey *key_event, ZMapWindow window, FooCanvasItem *focus_item) ;

static void printStats(ZMapWindowContainerGroup container_parent, FooCanvasPoints *points,
                       ZMapContainerLevelType level, gpointer user_data) ;

static void revCompRequest(ZMapWindow window) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void spliceRequest(ZMapWindow window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#if ZOOM_SCROLL
static void fc_begin_update_cb(FooCanvas *canvas, gpointer user_data);
static void fc_end_update_cb(FooCanvas *canvas, gpointer user_data);
static void fc_begin_map_cb(FooCanvas *canvas, gpointer user_data) ;
static void fc_end_map_cb(FooCanvas *canvas, gpointer user_data) ;
static void fc_draw_background_cb(FooCanvas *canvas, int x, int y, int width, int height, gpointer user_data);
static void fc_drawn_items_cb(FooCanvas *canvas, int x, int y, int width, int height, gpointer user_data);
#endif

static void canvasSetCursor(ZMapWindow window,
                            gboolean external_cursor, GdkCursor *cursor,
                            const char *file, const char *func) ;

static void lockedRulerCB(gpointer key, gpointer value_unused, gpointer user_data) ;
static void setupRuler(ZMapWindow       window,
                       FooCanvasItem  **horizon,
                       FooCanvasGroup **tooltip,
                       double           x_coord,
                       double           y_coord);
static void moveRuler(FooCanvasItem  *horizon,
                      FooCanvasGroup *tooltip,
                      char      *tip_text,
                      double     world_x,
                      double     world_y);
static void removeRuler(FooCanvasItem *horizon, FooCanvasGroup *tooltip);
static char * tooltipTextCreate(ZMapWindow window, double wx, double wy) ;

static gboolean within_x_percent(ZMapWindow window, double percent, double y, gboolean in_top);
static gboolean real_recenter_scroll_window(ZMapWindow window, unsigned int one_to_hundred, double world_y, gboolean in_top);
static gboolean recenter_scroll_window(ZMapWindow window, double *event_y_in_out);

static ZMapFeatureContextExecuteStatus undisplayFeaturesCB(GQuark key,
                                                           gpointer data, gpointer user_data,
                                                           char **err_out) ;
static void invokeVisibilityChange(ZMapWindow window) ;

//static void foo_bug_print(void *key, const char *where) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printWindowSizeDebug(char *prefix, ZMapWindow window,
                                 GtkWidget *widget, GtkAllocation *allocation) ;

static void printAdjusters(ZMapWindow window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/*
 *                  Globals
 */

/* Callbacks we make back to the level above us. This structure is static
 * because the callback routines are set just once for the lifetime of the
 * process. */
static ZMapWindowCallbacks window_cbs_G = NULL ;


static gboolean window_rev_comp_save_state_G = TRUE;
static gboolean window_rev_comp_save_bumped_G = TRUE;
static gboolean window_rev_comp_save_zoom_G = TRUE;
static gboolean window_split_save_bumped_G = TRUE;


/* Debugging canvas... */
static gboolean busy_debug_G = FALSE ;
static gboolean mouse_debug_G = FALSE ;

struct fooBug foo_wins [128];
int n_foo_wins = 0;






/* This routine must be called just once before any other windows routine, it is undefined
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 *
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of windows terminate routine. */
void zMapWindowInit(ZMapWindowCallbacks callbacks)
{
  if (window_cbs_G
      || !callbacks
      || !callbacks->enter
      || !callbacks->leave
      || !callbacks->scroll
      || !callbacks->focus
      || !callbacks->select
      || !callbacks->setZoomStatus
      || !callbacks->splitToPattern
      || !callbacks->visibilityChange
      || !callbacks->command
      || !callbacks->drawn_data)
    return ;

  window_cbs_G = g_new0(ZMapWindowCallbacksStruct, 1) ;

  window_cbs_G->enter = callbacks->enter ;
  window_cbs_G->leave = callbacks->leave ;
  window_cbs_G->scroll = callbacks->scroll ;
  window_cbs_G->focus = callbacks->focus ;
  window_cbs_G->select = callbacks->select ;
  window_cbs_G->setZoomStatus = callbacks->setZoomStatus;
  window_cbs_G->splitToPattern = callbacks->splitToPattern;
  window_cbs_G->visibilityChange = callbacks->visibilityChange ;
  window_cbs_G->command = callbacks->command ;
  window_cbs_G->drawn_data = callbacks->drawn_data;
  window_cbs_G->merge_new_feature = callbacks->merge_new_feature;

  window_cbs_G->remote_request_func = callbacks->remote_request_func ;
  window_cbs_G->remote_request_func_data = callbacks->remote_request_func_data ;

  return ;
}


ZMapWindowCallbacks zmapWindowGetCBs(void)
{
  return window_cbs_G ;
}


ZMapStyleTree* zMapWindowGetStyles(ZMapWindow window)
{
  ZMapStyleTree *result = NULL ;

  if (window && window->context_map)
    result = &window->context_map->styles ;

  return result ;
}

ZMapFeatureContext zMapWindowGetContext(ZMapWindow window)
{
  ZMapFeatureContext result = NULL ;

  if (window)
    result = window->feature_context ;

  return result ;
}


ZMapWindow zMapWindowCreate(GtkWidget *parent_widget,
                            ZMapFeatureSequenceMap sequence, void *app_data,
                            GList *feature_set_names, int *int_values)
{
  ZMapWindow window ;

  window = myWindowCreate(parent_widget, sequence, app_data,
                          feature_set_names,
                          NULL, NULL, int_values) ;

  return window ;
}


/* I DON'T THINK WE NEED THE TWO STYLES PARAMS HERE ACTUALLY.... */
/* Makes a new window that is a copy of the existing one, zoom factor and all.
 *
 * NOTE that not all fields are copied here as some need to be done when we draw
 * the actual features (e.g. anything that refers to canvas items). */
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, ZMapFeatureSequenceMap sequence,
                          void *app_data, ZMapWindow original_window,
                          ZMapFeatureContext feature_context,
                          ZMapWindowLockType window_locking)
{
  ZMapWindow new_window = NULL ;
  GtkAdjustment *hadjustment = NULL, *vadjustment = NULL ;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
  int x, y ;

  zmapWindowBusy(original_window, TRUE) ;

  if (window_locking != ZMAP_WINLOCK_NONE)
    {
      setCurrLock(window_locking, original_window,
                  &hadjustment, &vadjustment) ;
    }

  /* There is an assymetry when splitting windows, when we do a vsplit we lose the scroll
   * position when we the do the windowcreate so we need to get it here so we can
   * reset the scroll to where it should be. */
  foo_canvas_get_scroll_offsets(original_window->canvas, &x, &y) ;

  new_window = myWindowCreate(parent_widget, sequence, app_data,
                              original_window->feature_set_names,
                              hadjustment, vadjustment,
                              original_window->int_values) ;
  if (!new_window)
    return new_window ;

  zmapWindowBusy(new_window, TRUE) ;


  /* Lock windows together for scrolling/zooming if requested. */
  if (window_locking != ZMAP_WINLOCK_NONE)
    {
      copyLockWindow(original_window, new_window) ;
    }


  /* Is there a remote client ? */
  new_window->xremote_client = original_window->xremote_client ;

  /* A new window will have new canvas items so we need a new hash. */
  new_window->context_to_item = zmapWindowFToICreate() ;

  new_window->canvas_maxwin_size = original_window->canvas_maxwin_size ;
  new_window->min_coord = original_window->min_coord ;
  new_window->max_coord = original_window->max_coord ;
  new_window->display_origin = original_window->display_origin ;
  new_window->display_coordinates = original_window->display_coordinates ;

  new_window->seqLength = original_window->seqLength ;

  /* Set revcomp'd features. */
  new_window->display_forward_coords = original_window->display_forward_coords ;
  zmapWindowScaleCanvasSetRevComped(new_window->ruler, zMapWindowGetFlag(original_window, ZMAPFLAG_REVCOMPED_FEATURES)) ;
    /* Sets display_forward too... */

  zmapWindowZoomControlCopyTo(original_window->zoom, new_window->zoom);

  /* I'm a little uncertain how much of the below is really necessary as we are
   * going to call the draw features code anyway. */
  /* Set the zoom factor, there is no call to get hold of pixels_per_unit so we dive.
   * into the struct. */
  zmapWindowSetPixelxy(new_window,
       original_window->canvas->pixels_per_unit_x,original_window->canvas->pixels_per_unit_y) ;

  zmapWindowScaleCanvasSetPixelsPerUnit(new_window->ruler,
                                        original_window->canvas->pixels_per_unit_x,
                                        original_window->canvas->pixels_per_unit_y);

  /* Copy to scroll region from original to new (possibly complete
   * with border, possibly to canvas height region size). Required
   * for the scroll_to call next too. */
  scroll_x1 = scroll_x2 = scroll_y1 = scroll_y2 = 0.0;
  zmapWindowGetScrollableArea(original_window,  &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
  zmapWindowSetScrollableArea(new_window,  &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2,"zMapWindowCopy") ;

  /* Reset our scrolled position otherwise we can end up jumping to the top of the window. */
  foo_canvas_scroll_to(original_window->canvas, x, y) ;
  foo_canvas_scroll_to(new_window->canvas, x, y) ;


  /* some of the above functions set the window sizes of the layout so copy across the original
   * sizes otherwise we do not how to zoom/split this copy window correctly... */
  new_window->layout_window_width = original_window->layout_window_width ;
  new_window->layout_window_height = original_window->layout_window_height ;
  new_window->layout_bin_window_width = original_window->layout_bin_window_width ;
  new_window->layout_bin_window_height = original_window->layout_bin_window_height ;




  /* You cannot just draw the features here as the canvas needs to be realised so we send
   * an event to get the data drawn which means that the canvas is guaranteed to be
   * realised by the time we draw into it. */
  {
    ZMapWindowState state;

    state = zmapWindowStateCreate();
    zmapWindowStateSaveMark(state, original_window);

    zmapWindowStateSaveFocusItems(state, original_window);

    if (window_split_save_bumped_G)
      zmapWindowStateSaveBumpedColumns(state, original_window);

    /* should we be passing in a copy of the full set of original styles ? */
    zMapWindowDisplayData(new_window, state, feature_context, feature_context,
                          original_window->context_map,
                          NULL, NULL, original_window->splice_highlight_on, NULL) ;
  }

  zmapWindowBusy(new_window, FALSE) ;
  zmapWindowBusy(original_window, FALSE) ;

  return new_window ;
}




/* This function shouldn't be called directly, instead use the macro zMapWindowBusy()
 * defined in the public header zmapWindow.h */
void zMapWindowBusyFull(ZMapWindow window, gboolean busy, const char *file, const char *func)
{
  GdkCursor *cursor ;

  if (busy)
    cursor = window->window_busy_cursor ;
  else
    cursor = window->normal_cursor ;

  zmapWindowBusyInternal(window, FALSE, cursor, file, func) ;

  return ;
}




void zMapWindowSetCursor(ZMapWindow window, GdkCursor *cursor)
{
#ifdef __GNUC__
  zmapWindowBusyInternal(window,  TRUE, cursor,__FILE__, (char *)__PRETTY_FUNCTION__) ;
#else
  zmapWindowBusyInternal(window, TRUE, cursor, __FILE__, NULL) ;
#endif

  return ;
}





/* This routine is called by the code in zmapView.c that manages the slave threads.
 * It has to determine whether or not the canvas has got far enough along in its
 * creation to have a valid height.  If so, it calls the code to notify the GUI
 * that data exists and work needs to be done.  If not, it attaches a callback
 * routine, exposeHandlerCB to the expose_event for the canvas so that when that
 * occurs, exposeHandlerCB can issue the call to sendClientEvent.
 * We'd have used the realise signal if we could, but somehow the canvas manages to
 * achieve realised status (ie GTK_WIDGET_REALIZED yields TRUE) without having a
 * valid vertical dimension.
 *
 * Rob
 *
 *    Yes...if you read the docs then you'll know that realised != sized.
 *    It's the map-event signal that is needed....though whether that's really what's wanted
 *    I'm not sure....better revisit this.... Ed
 *
 *
 *  */
void zMapWindowDisplayData(ZMapWindow window, ZMapWindowState state,
                           ZMapFeatureContext current_features, ZMapFeatureContext new_features,
                           ZMapFeatureContextMap context_map,
                           GList *masked, ZMapFeature highlight_feature, gboolean splice_highlight,
                           gpointer loaded_cb_user_data)
{
  FeatureSetsState feature_sets ;

  zMapReturnIfFail(window) ;

  /* There is some assymetry here, we don't need to copy the feature context but we do need
   * to copy the styles because they need to be cached in window but _not_ at this point,
   * that should be done later when this event arrives in window. */
  feature_sets = g_new0(FeatureSetsStateStruct, 1) ;
  feature_sets->current_features = current_features ;
  feature_sets->new_features = new_features ;
  feature_sets->highlight_feature = highlight_feature ;
  feature_sets->splice_highlight_on = splice_highlight ;


  window->context_map = context_map;
  feature_sets->masked = masked;

//  printf("\nzMapWindowDisplay data:\n");
//        zMap_g_hashlist_print(new_featuresets_2_stylelist) ;

  feature_sets->state = state ;

  /* We either turn the busy cursor on here if there is already a window or we do it in the expose
   * handler exposeHandlerCB() which is called when the window is first realised, its turned off
   * again in zmapWindowDrawFeatures(). */
  if (GTK_WIDGET(window->canvas)->allocation.height > 1 && GTK_WIDGET(window->canvas)->window)
    {
      sendClientEvent(window, feature_sets, loaded_cb_user_data) ;
    }
  else if (!window->exposeHandlerCB)
    {
      RealiseData realiseData ;

      realiseData = g_new0(RealiseDataStruct, 1) ;    /* freed in exposeHandlerCB() */

      realiseData->window = window ;
      realiseData->feature_sets = feature_sets ;

      window->exposeHandlerCB = g_signal_connect(GTK_OBJECT(window->canvas), "expose_event",
                                                 GTK_SIGNAL_FUNC(exposeHandlerCB),
                                                 (gpointer)realiseData) ;
    }
  else
    {
      /* There's a problem here.
       *-------------------------
       * first call to displaydata sets the first exposeHandler
       * second call sets the second...
       * first handler gets run, removes the second handler (window->exposeHandlerCB id)
       * meanwhile when the first handler finishes it destroys its data.
       * first one gets run on next expose. BANG.
       */
      printf("If we've merged some contexts before being exposed\n");
    }

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapFeatureContextExecuteStatus undisplayFeaturesCB(GQuark key,
                                                           gpointer data,
                                                           gpointer user_data,
                                                           char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeature feature;
  FooCanvasItem *feature_item;
  ZMapStrand column_strand;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        /* CHRIST, WHAT IS THIS CODE....?????????? */

        zMapLogWarning("FeatureSet %s", g_quark_to_string(feature_any->unique_id));
        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        feature = (ZMapFeature)feature_any;

        /* which column drawn in depends on style. */
        column_strand = zmapWindowFeatureStrand(window, feature);

#if MH17_FToIHash_does_this_mapping
      /* if the feature context is from a request from otterlace then
       * the display column has not been set, we need to lookup
       */
        ZMapFeatureSetDesc gffset;
        ZMapFeatureSet fset;

        fset = (ZMapFeatureSet) (feature_any->parent);
        if(!fset->column_id)
          {
            gffset = g_hash_table_lookup(window->context_map->featureset_2_column, GUINT_TO_POINTER(fset->unique_id));
            if(gffset)
              fset->column_id = gffset->column_id;
          }
#endif

        /* MH17: we get locus features inserted mysteriously if a feature has a locus id
         * but they don't always appear in zmap in which case there is no column id
         * This is true when otterlace sends a single feature to delete and then we fail to find
         * the extra locus feature
         *
         * regardless of that if we have features that are not displayed due to config this could also fail
         * so if not column_id defined log a warning and fail silently.
         *
         * locus is used in the naviagtor, we hope dealt with via another call.
         */
        if ((feature_item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
                                                          column_strand,
                                                          ZMAPFRAME_NONE,
                                                          feature)))
          {
            zMapWindowFeatureRemove(window, feature_item, feature, FALSE);
            status = ZMAP_CONTEXT_EXEC_STATUS_OK;
          }
        else
          {
            zMapLogWarning("Failed to find feature \"%s\"", g_quark_to_string(feature->original_id));
            status = ZMAP_CONTEXT_EXEC_STATUS_ERROR ;
          }

        break;
      }
    default:
      {
        /* nothing to do for most of it while we only have single blocks and aligns... */
        break;
      }
    }

  return status ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void zMapWindowUnDisplayData(ZMapWindow window,
                             ZMapFeatureContext current_features,
                             ZMapFeatureContext new_features)
{
  /* we have no issues here with realising, hopefully */

  zMapFeatureContextExecute((ZMapFeatureAny)new_features,
    ZMAPFEATURE_STRUCT_FEATURE,
    undisplayFeaturesCB,
    window);

  return ;
}


#ifdef NOT_USED
void zMapWindowRemoveFeatureset(ZMapWindow window, ZMapFeatureSet featureset)
{
#warning: "what's going on here, this function takes a feature, not a feature set..."

  FooCanvasItem *item = zmapWindowFToIFindFeatureItem(window, window->context_to_item,
      ZMAPSTRAND_NONE, ZMAPFRAME_NONE, featureset) ;


  if (item)
    zMapWindowFeaturesetItemRemoveSet(item, featureset, TRUE) ;

  return ;
}
#endif


/* completely reset window. */
void zMapWindowReset(ZMapWindow window)
{
  zmapWindowBusy(window, TRUE) ;

  resetCanvas(window, TRUE, TRUE) ;

  /* Need to reset feature context pointer and any other things..... */

  zmapWindowBusy(window, FALSE) ;

  return ;
}



/* Force a canvas redraw via a circuitous route....
 *
 * You should note that it is not possible to do this via the foocanvas function
 * foo_canvas_update_now() even if you set the need_update flag to TRUE, this
 * would only work if you set the update flag for every group/item to TRUE as well !
 *
 * Instead we in effect send an expose event (via gdk_window_invalidate_rect()) to
 * the canvas window. Note that this is not straightforward as there are two
 * windows: layout->bin_window which equates to the scroll_region of the canvas
 * and widget->window which is the window you actually see. To get the redraw
 * we invalidate the latter.
 *
 *  */
void zMapWindowRedraw(ZMapWindow window)
{
  GdkRectangle expose_area ;
  GtkAllocation *allocation ;

  zMapReturnIfFail(window) ;

  /* Get the size of the canvas's on screen window, i.e. the section of canvas you
   * can actually see. */
  allocation = &(GTK_WIDGET(&(window->canvas->layout))->allocation) ;

  /* Set up the area of this window to be invalidated (i.e. to be redrawn). */
  expose_area.x = expose_area.y = 0 ;
  expose_area.width = allocation->width ;
  expose_area.height = allocation->height ;

  /* Invalidate the displayed canvas window causing to be redrawn. */
  gdk_window_invalidate_rect(GTK_WIDGET(&(window->canvas->layout))->window, &expose_area, TRUE) ;

  return ;
}



/* Show stats for window.
 *
 *  */
void zMapWindowStats(ZMapWindow window, GString *text)
{
  if (!text || !window)
    return ;

  zmapWindowContainerUtilsExecute(window->feature_root_group,
                                  ZMAPCONTAINER_LEVEL_FEATURESET,
                                  printStats, text);

  return ;
}



/* Should only be called by zmapViewResetWindows() because
 * all windows need to be saved/reset together */
void zMapWindowFeatureSaveState(ZMapWindow window, gboolean features_are_revcomped)
{
  zMapReturnIfFail(window) ;
  zmapWindowBusy(window, TRUE) ;

  zMapStartTimer("WindowFeatureRedraw","");

/* MH17 NOTE this state struct gets freed sometime later after being passsed around via some other data structs
 * it's set here so that zMapWindwoFeatureRedraw() (below) can access it
 * ref to zmapViewReverseComplement() and RT 229703
 * to be tidy after passing the pointer somewhere else we will zero this pointer
 */
  window->state = zmapWindowStateCreate();

  /* Note that currently we lose the 3 frame state and other state such as columns */
  window->display_3_frame = DISPLAY_3FRAME_NONE ;
  window->show_3_frame_reverse = FALSE ;

  /* I think its ok to do this here ? this blanks out the info panel, we could hold on to the
   * originally highlighted feature...but only if its still visible if it ends up on the
   * reverse strand...for now we just blank it.... */
  zmapWindowUpdateInfoPanel(window, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, TRUE, FALSE, FALSE, NULL) ;

  zmapWindowStateSavePosition(window->state, window);
  zmapWindowStateSaveFocusItems(window->state, window) ;

  if (window_rev_comp_save_state_G)
    {
      zmapWindowStateSaveMark(window->state, window);
    }

  if(window_rev_comp_save_bumped_G)
    {
      zmapWindowStateSaveBumpedColumns(window->state, window);
    }

  if(window_rev_comp_save_zoom_G)
    {
      zmapWindowStateSaveZoom(window->state, zMapWindowGetZoomFactor(window));
    }

  zMapStopTimer("WindowFeatureRedraw","Revcomp");

  return ;
}

/* Should only be called by zmapViewResetWindows() because
 * all windows need to be saved/reset together */
void zMapWindowFeatureReset(ZMapWindow window, gboolean features_are_revcomped)
{
  gboolean free_child_windows = features_are_revcomped;
  gboolean free_revcomp_safe_windows = FALSE;

  zMapReturnIfFail(window) ;

  resetCanvas(window, free_child_windows, free_revcomp_safe_windows) ; /* Resets scrolled region and much else. */
  zMapStopTimer("WindowFeatureRedraw","ResetCanvas");

  if(window->strand_separator_context)
    zMapFeatureContextDestroy(window->strand_separator_context, TRUE);
  window->strand_separator_context = NULL;
  zMapStopTimer("WindowFeatureRedraw","Separator");

  zMapStopTimer("WindowFeatureRset","");

  zmapWindowBusy(window, FALSE) ;

  return ;
}

void zMapWindowFeatureRedraw(ZMapWindow window, ZMapFeatureContext feature_context,
                             gboolean features_are_revcomped)
{
//  int x, y ;
//  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
//  gboolean free_child_windows = FALSE, free_revcomp_safe_windows = FALSE ;

  zMapReturnIfFail(window) ;

  zmapWindowBusy(window, TRUE) ;

  zMapStartTimer("WindowFeatureRedraw","");


  /* You cannot just draw the features here as the canvas needs to be realised so we send
   * an event to get the data drawn which means that the canvas is guaranteed to be
   * realised by the time we draw into it. */
  zMapWindowDisplayData(window, window->state, feature_context, feature_context,
                        window->context_map,
                        NULL, NULL, FALSE, NULL) ;

  window->state = NULL;/* see comment in zmapWindowFeatureReset() above */

  zMapStopTimer("WindowFeatureRedraw","Display");


  zMapStopTimer("WindowFeatureRedraw","");

  zmapWindowBusy(window, FALSE) ;

  return ;
}


/* Returns TRUE if this window is locked with another window for its zooming/scrolling. */
gboolean zMapWindowIsLocked(ZMapWindow window)
{
  return window->locked_display  ;
}



/* Unlock this window so it is not zoomed/scrolled with other windows in its group. */
void zMapWindowUnlock(ZMapWindow window)
{
  unlockWindow(window, FALSE) ;

  return ;
}





void zMapWindowBack(ZMapWindow window)
{
  ZMapWindowState prev_state;

  zMapReturnIfFail(window) ;

  /* I used FALSE here as I wanted to get scroll region in window zoom
   * However I think that logic isn't completely necessary to fix RT #55388
   * Anyway we still need to update the view via visibility change...
   */
  if (zmapWindowStateGetPrevious(window, &prev_state, FALSE))
    {
      ZMapWindowVisibilityChangeStruct change = {};

      zmapWindowStateRestore(prev_state, window);

      zmapWindowStateQueueRemove(window->history, prev_state);

      prev_state = zmapWindowStateDestroy(prev_state);

      change.zoom_status = zMapWindowGetZoomStatus(window);


      /* somehow this does not paint till we get a window/widget resise if back from big zoom in a vsplit window */
      /* unless we call this, but we also need to handle the blank bits at the edges */
      /*! \todo #warning still doesn-t work, possibly a race condition, it-s not 100% producable */
        /* only appears to apply to v-split */
        //sleep(4);/* still goes wrong as before */
      zMapWindowRedraw(window);

      /* also need to do locked windows */
      /* NOTE this hash table has window as key and value, maybe a GList would have been better
       * but we will call WindowRedraw with 3 args and the first will be valid
       * C handles passing a diff number of args, it's not a compiler choice
       */
      if (window->sibling_locked_windows)
        g_hash_table_foreach(window->sibling_locked_windows, (GHFunc) zMapWindowRedraw, NULL) ;


      foo_canvas_get_scroll_region(window->canvas,
                                   NULL, &(change.scrollable_top),
                                   NULL, &(change.scrollable_bot));

      zmapWindowClampedAtStartEnd(window, &(change.scrollable_top), &(change.scrollable_bot));

      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&change);
    }

  return  ;
}




void zMapWindowZoom(ZMapWindow window, double zoom_factor)
{
  zmapWindowZoom(window, zoom_factor, TRUE) ;

  return ;
}


void zMapWindowZoomToMin(ZMapWindow window)
{
  zmapWindowZoomControlHandleResize(window) ;

  return ;
}

/*
 * These two functions return and set the value for the origin
 * of the user display coordinates, and is defined in terms
 * of the internal ZMap chromosome coordinates.
 */
double zMapWindowGetDisplayOrigin(ZMapWindow window)
{
  double result = 0.0 ;
  zMapReturnValIfFail(window, result ) ;
  result = window->display_origin ;
  return result ;
}

void zMapWindowSetDisplayOrigin(ZMapWindow window, double origin)
{
  zMapReturnIfFail(window) ;
  window->display_origin = origin ;
}

/*
 * This fuction has to force a redraws of the scale bar using the
 * flag set below since we are not changing the coordinate ranges.
 */
void zMapWindowToggleDisplayCoordinates(ZMapWindow window)
{
  double sx1 = 0.0,
    sy1 = 0.0,
    sx2 = 0.0,
    sy2 = 0.0 ;
  zMapReturnIfFail(window) ;

  if (window->display_coordinates == ZMAP_WINDOW_DISPLAY_CHROM)
    window->display_coordinates = ZMAP_WINDOW_DISPLAY_SLICE ;
  else if (window->display_coordinates == ZMAP_WINDOW_DISPLAY_SLICE)
    window->display_coordinates = ZMAP_WINDOW_DISPLAY_CHROM ;

  sy1 = window->min_coord ;
  sy2 = window->max_coord ;
  window->force_scale_redraw = TRUE ;
  zmapWindowGetScrollableArea(window, &sx1, &sy1, &sx2, &sy2) ;
  zmapWindowSetScrollableArea(window, NULL, &sy1, NULL, &sy2, NULL) ;
  window->force_scale_redraw = FALSE ;
}

void zMapWindowSetDisplayCoordinatesSlice(ZMapWindow window)
{
  zMapReturnIfFail(window) ;

  window->display_coordinates = ZMAP_WINDOW_DISPLAY_SLICE ;
}

void zMapWindowSetDisplayCoordinatesChrom(ZMapWindow window)
{
  zMapReturnIfFail(window) ;

  window->display_coordinates = ZMAP_WINDOW_DISPLAY_CHROM ;
}

void zMapWindowSetDisplayCoordinates(ZMapWindow window, ZMapWindowDisplayCoordinates display_coordinates)
{
  zMapReturnIfFail(window && (display_coordinates != ZMAP_WINDOW_DISPLAY_INVALID) ) ;
  window->display_coordinates = display_coordinates ;
}

ZMapWindowDisplayCoordinates zMapWindowGetDisplayCoordinates(ZMapWindow window)
{
  zMapReturnValIfFail(window, ZMAP_WINDOW_DISPLAY_INVALID) ;
  return window->display_coordinates ;
}



/* try out the new zoom window.... */
void zmapWindowZoom(ZMapWindow window, double zoom_factor, gboolean stay_centered)
{
  double sensitivity = 0.001 ;

  zMapReturnIfFail(window) ;

  if ((zoom_factor < (1.0 - sensitivity)) || (zoom_factor > (1.0 + sensitivity)))
    {
      static gboolean debug = FALSE ;
      GdkWindow *gdk_window ;
      Display *x_display ;
      int x, y ;
      double width, curr_pos = 0.0 ;
      GtkAdjustment *adjust ;
      int adjust_centre ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapDebugPrintf("\n\n\nAbout to Zoom %s.....",
      (zoom_factor > 1.0 ? "in" : "out")) ;

      printAdjusters(window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      if (debug)
        {
          gdk_window = gtk_widget_get_window(window->toplevel) ;
          x_display = GDK_DRAWABLE_XDISPLAY(gdk_window) ;

          XSynchronize(x_display, TRUE) ;
        }


      zmapWindowBusy(window, TRUE) ;/* just does the hour glass cursor */

      adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

      /* Record the current position. */
      /* In order to stay centred on wherever we are in the canvas while zooming, we get the
       * current position (offset, in canvas pixels), add half the page-size to get the centre
       * of the screen,then convert to world coords and store those.
       * After the zoom, we convert those values back to canvas pixels (changed by the call to
       * pixels_per_unit) and scroll to them.
       * We end up working this out again in myWindowZoom, if and only if we're horizontally
       * split windows, but I don't thik it'll be a big issue.
       */

      adjust_centre = (int)((adjust->page_size / 2) + 0.5) ;

      foo_canvas_get_scroll_offsets(window->canvas, &x, &y);
      y += adjust_centre ;
      foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapWindowPrintCanvas(window->canvas) ;
      fflush(stdout) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* possible bug here with width and scrolling, need to check. */
      if (window->locked_display)
        {
          ZMapWindowState prev_state;
          LockedDisplayStruct locked_data = { NULL };
          gboolean use_queue = TRUE;

          locked_data.window      = window ;
          locked_data.type        = ZMAP_LOCKED_ZOOMING;
          locked_data.zoom_factor = zoom_factor ;
          locked_data.position    = curr_pos ;

          if(use_queue && zmapWindowStateQueueIsRestoring(window->history) &&
             zmapWindowStateGetPrevious(window, &prev_state, FALSE))
            {
              double ry1, ry2;/* restore scroll region */
              zmapWindowStateGetScrollRegion(prev_state, NULL, &ry1, NULL, &ry2);
              locked_data.position = ((ry2 - ry1) / 2 ) + ry1;
            }

          g_hash_table_foreach(window->sibling_locked_windows, lockedDisplayCB, (gpointer)&locked_data) ;
        }
      else
        {
          foo_canvas_busy(window->canvas,TRUE);
          myWindowZoom(window, zoom_factor, curr_pos) ;
          foo_canvas_busy(window->canvas,FALSE);
        }


      /* We need to scroll to the previous position. This is dependent on
       * not having split horizontal windows. If we try to work out position
       * within myWindowZoom we end up getting the wrong position the second
       * times round and not scrolling to the right position.
       *  */
      if (stay_centered && window->curr_locking != ZMAP_WINLOCK_HORIZONTAL)
        {
          ScrollToStruct scroll_to_data ;

          foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y) ;

          scroll_to_data.x = x ;
          scroll_to_data.y = y - adjust_centre ;

          /* With multiple windows must scroll them all to get redraws in all of them. */
          if (window->sibling_locked_windows)
            g_hash_table_foreach(window->sibling_locked_windows, scrollToCB, (gpointer)&scroll_to_data) ;
          else
            scrollToCB(window, NULL, &scroll_to_data) ;
        }

      zmapWindowBusy(window, FALSE) ;

      if (debug)
        {
          XSynchronize(x_display, FALSE) ;
        }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printAdjusters(window) ;

      zMapDebugPrintf("Finished Zoom %s.....\n",
      (zoom_factor > 1.0 ? "in" : "out")) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }

  return ;
}


/* Returns FALSE if feature cannot be found on canvas. */
gboolean zMapWindowZoomToFeature(ZMapWindow window, ZMapFeature feature)
{
  gboolean result = FALSE ;
  FooCanvasItem *feature_item ;

  zMapReturnValIfFail(window, result) ;

  if ((feature_item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
    feature->strand,
    ZMAPFRAME_NONE,
    feature)))
    {
      zmapWindowZoomToItem(window, feature_item) ;
      result = TRUE ;
    }

  return result ;
}


void zMapWindowZoomToWorldPosition(ZMapWindow window, gboolean border,
   double rootx1, double rooty1,
                                   double rootx2, double rooty2)
{
  zmapWindowZoomToWorldPosition(window, border, rootx1, rooty1, rootx2, rooty2);

  return ;
}




/* try out the new zoom window.... */
void zMapWindowMove(ZMapWindow window, double start, double end)
{

  zMapReturnIfFail(window) ;

  if (window->locked_display && window->curr_locking != ZMAP_WINLOCK_HORIZONTAL)
    {
      LockedDisplayStruct locked_data = { NULL };

      locked_data.window = window ;
      locked_data.type   = ZMAP_LOCKED_MOVING;
      locked_data.start  = start;
      locked_data.end    = end;

      g_hash_table_foreach(window->sibling_locked_windows, lockedDisplayCB, (gpointer)&locked_data) ;
    }
  else
    myWindowMove(window, start, end) ;


  return;
}



/*!
 * Returns coords of currently exposed section of canvas, note that coords do not include
 * the blank boundary, only things that are drawn in the alignment.
 *
 *
 * @param window       The ZMapWindow.
 * @param x1_out       left coord.
 * @param y1_out       top coord.
 * @param x2_out       right coord.
 * @param y2_out       bottom coord.
 * @return             TRUE if window is valid and a position is returned, FALSE otherwise.
 *  */
gboolean zMapWindowCurrWindowPos(ZMapWindow window,
                                 double *x1_out, double *y1_out, double *x2_out, double *y2_out)
{
  gboolean result = TRUE ;
  GtkAdjustment *h_adjuster, *v_adjuster ;
  double left, top, right, bottom ;
  double x1, y1, x2, y2 ;

  zMapReturnValIfFail(window, result) ;

  h_adjuster =
    gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
  v_adjuster =
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  /* note in calculating the right/bottom, its possible for the scrolled window to be forced
   * to be bigger than the canvas. */
  left = h_adjuster->value ;
  top = v_adjuster->value ;
  right = left + MIN(h_adjuster->page_size, h_adjuster->upper)  ;
  bottom = top + MIN(v_adjuster->page_size, v_adjuster->upper) ;


  foo_canvas_window_to_world(window->canvas, left, top, x1_out, y1_out) ;

  foo_canvas_window_to_world(window->canvas, right, bottom, x2_out, y2_out) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(window->feature_root_group),
     &x1, &y1, &x2, &y2) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (*x1_out < x1)
    *x1_out = x1 ;
  if (*y1_out < y1)
    *y1_out = y1 ;
  if (*x2_out > x2)
    *x2_out = x2 ;
  if (*y2_out > y2)
    *y2_out = y2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return result ;
}


/*!
 * Returns the max coords of the alignment.
 *
 * @param window       The ZMapWindow.
 * @param x1_out       left coord.
 * @param y1_out       top coord.
 * @param x2_out       right coord.
 * @param y2_out       bottom coord.
 * @return             TRUE if window is valid and a position is returned, FALSE otherwise.
 *  */
gboolean zMapWindowMaxWindowPos(ZMapWindow window,
                                double *x1_out, double *y1_out, double *x2_out, double *y2_out)
{
  gboolean result = TRUE ;

  zMapReturnValIfFail(window && window->canvas, result) ;

  /* should I be getting the whole thing here including borders ???? */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(window->feature_root_group),
                             &dump_opts.x1, &dump_opts.y1, &dump_opts.x2, &dump_opts.y2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* THIS MAY NOT WORK AS THE SCALING FACTOR/PIXELS BIT MAY BE ALL WRONG.... */

  /* This doesn't do the borders. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(window->canvas->root),
                             x1_out, y1_out, x2_out, y2_out) ;

  return result ;
}



/*!
 * Scrolls canvas window vertically to window_y_pos. The window will not scroll beyond
 * its current top/bottom.
 *
 * @param window       The ZMapWindow to be scrolled.
 * @param window_y_pos The vertical position in pixels to move to in the ZMapWindow.
 * @return             nothing
 *  */
void zMapWindowScrollToWindowPos(ZMapWindow window, int window_y_pos)
{
  GtkAdjustment *v_adjuster ;
  double new_value ;
  double half_way ;

  zMapReturnIfFail(window) ;

  /* The basic idea is to find out from the canvas windows parent scrolled window adjuster
   * how much we need to move the canvas window to get to window_y_pos and then use the
   * adjuster to move the canvas window. */
  v_adjuster =
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  half_way = (v_adjuster->page_size / 2.0) ;

  new_value = v_adjuster->value + ((double)window_y_pos - (v_adjuster->value + half_way)) ;

  /* The gtk adjuster docs say that the adjuster is clamped to move only between its upper
   * and lower values. It successfully clamps to the upper value but _not_ the lower where
   * it seems to take no notice of its own page_size so we have to do the work here. */
  if (new_value + v_adjuster->page_size > v_adjuster->upper)
    new_value = new_value - ((new_value + v_adjuster->page_size) - v_adjuster->upper) ;

  gtk_adjustment_set_value(v_adjuster, new_value) ;

  return ;
}

void zMapWindowMergeInFeatureSetNames(ZMapWindow window, GList *feature_set_names)
{

  zMapReturnIfFail(window) ;

  /* This needs to do more that just concat!! ha, it'll break something down the line ... column ordering at least */
  /* mh17: as long as featuresets are not duplicated between servers there's no probs */
  /* mh17: but if we do an OTF alignment then we get duplicates -> don't call if existing connection */

  for(;feature_set_names;feature_set_names = feature_set_names->next)
  {
      /* usual lousy double scan of the list to add to the end */
      if(!g_list_find(window->feature_set_names,feature_set_names->data))
            window->feature_set_names = g_list_append(window->feature_set_names,feature_set_names->data);
  }
}


void zMapWindowDestroy(ZMapWindow window)
{
  zMapDebug("%s", "GUI: in window destroy...\n") ;
  zMapReturnIfFail(window) ;

  if (window->locked_display)
    unlockWindow(window, FALSE) ;

  /* free the array of feature list windows and the windows themselves */
  zmapWindowFreeWindowArray(&(window->featureListWindows), TRUE) ;

  /* free the array of search windows and the windows themselves */
  zmapWindowFreeWindowArray(&(window->search_windows), TRUE) ;


  /* free the array of dna windows and the windows themselves */
  zmapWindowFreeWindowArray(&(window->dna_windows), TRUE) ;


  /* free the array of dna windows and the windows themselves */
  zmapWindowFreeWindowArray(&(window->dnalist_windows), TRUE) ;


  /* free the array of editor windows and the windows themselves */
  zmapWindowFreeWindowArray(&(window->feature_show_windows), TRUE) ;

  if(window->style_window)
    zmapWindowStyleDialogDestroy(window);

  /* Get rid of the column configuration window. */
  zmapWindowColumnConfigureDestroy(window) ;

  /* Do this before the toplevel destroy as we need refer to canvas items for the destroy. */
  zmapWindowMarkDestroy(window->mark) ;
  window->mark = zmapWindowMarkCreate(window) ;    /* ??? why do we recreate it ?? */

  /* Destroy to avoid items trying to remove themselves from the focus as they are destroyed. */
  if (window->focus)
    {
      zmapWindowFocusDestroy(window->focus) ;
      window->focus = NULL ;
    }

  gtk_widget_destroy(window->toplevel) ;
  zmapWindowFToIDestroy(window->context_to_item) ;

  if(window->history)
    zmapWindowStateQueueDestroy(window->history);

  g_free(window) ;

  return ;
}



/* This function shouldn't be called directly, instead use the macro zMapWindowBusy()
 * defined in the public header zmapWindow.h */
void zmapWindowBusyInternal(ZMapWindow window,
                            gboolean external_cursor, GdkCursor *cursor,
                            const char *file, const char *func)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (busy)
    {
      canvas_set_busy_cursor(window, external_cursor, file, func) ;
    }
  else
    {
      canvas_unset_busy_cursor(window, file, func) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (!cursor)
    cursor = window->normal_cursor ;

  canvasSetCursor(window,
                  external_cursor, cursor,
                  file, func) ;

  return ;
}





/*
 *
 */
void zmapWindowGetScrollableArea(ZMapWindow window,
                                 double *x1_inout, double *y1_inout,
                                 double *x2_inout, double *y2_inout)
{
  zMapReturnIfFail(window) ;

  foo_canvas_get_scroll_region(FOO_CANVAS(window->canvas),
       x1_inout, y1_inout, x2_inout, y2_inout);

  if(y1_inout && y2_inout)
  {
    zmapWindowClampedAtStartEnd(window, y1_inout, y2_inout);
  }

  return ;
}

void zmapWindowSetScrollableArea(ZMapWindow window,
                                 double *x1_inout, double *y1_inout,
                                 double *x2_inout, double *y2_inout, const char *where)
{
  ZMapWindowVisibilityChangeStruct vis_change ;
  ZMapGUIClampType clamp = ZMAPGUI_CLAMP_INIT;
  double border, x1, x2, y1, y2, tmp_top, tmp_bot;

  zMapReturnIfFail(window) ;

  zmapWindowGetBorderSize(window, &border) ;

  foo_canvas_get_scroll_region(FOO_CANVAS(window->canvas), &x1, &y1, &x2, &y2) ;

  /* Read the input */
  x1 = (x1_inout ? *x1_inout : x1) ;
  x2 = (x2_inout ? *x2_inout : x2) ;
  y1 = (y1_inout ? *y1_inout : y1) ;
  y2 = (y2_inout ? *y2_inout : y2) ;

  clamp = zmapWindowClampedAtStartEnd(window, &y1, &y2) ;
  y1 -= (tmp_top = ((clamp & ZMAPGUI_CLAMP_START) ? border : 0.0)) ;
  y2 += (tmp_bot = ((clamp & ZMAPGUI_CLAMP_END)   ? border : 0.0)) ;

  window->scroll_initialised = TRUE;


  /* THIS OPTIMISATION IS ALL ABOUT WHAT HAS HAPPENED TO THE CODE...WE CHANGES SIZES ALL OVER
   * THE PLACE WHICH PROVOKES REDRAWS WHICH RESULTS IN THIS FUNCTION GETTING CALLED MANY
   * TIMES...STUPID, STUPID, STUPID......SO I RECORD SEPARATELY WHETHER WE HAVE BEEN REVCOMP'D
   * IN WHICH CASE WE _MUST_ REDRAW THE SCALE.
   * (sm23) Note that when the coordinate system is changed, we also have to redraw, even though
   * the other criteria below are not then satisfied; hence we have a flag to force the operation
   * to take place.
   */
  /* if the vertical scroll region has changed then draw the scale bar */
  /* formerly done by fc_end_update_cb(), which is silly: why redraw the scale bar on any update?? */
  /* NOTE also required stupid coding to handle spurious calls and bouncing event loops */
  if (   window->force_scale_redraw
         || (window->ruler_previous_revcomp != zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
      || (y1 != window->scroll_y1 || y2 != window->scroll_y2))
    {

      window->ruler_previous_revcomp = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) ;

      if (zmapWindowScaleCanvasDraw(window->ruler,
                                    y1, y2,
                                    window->min_coord, window->max_coord,
                                    window->sequence->start, window->sequence->end,
                                    window->display_coordinates,
                                    window->force_scale_redraw))
        {
          double max_x2 = zmapWindowScaleCanvasGetWidth(window->ruler) + 1 ;

          /* NOTE this canvas has nothing except the scale bar */
          zmapWindowScaleCanvasSetScroll(window->ruler, x1, y1, max_x2, y2) ;
        }


   }

  if (x1 != window->scroll_x1 || y1 != window->scroll_y1 || x2 != window->scroll_x2 || y2 != window->scroll_y2)
    {

      zmapWindowSetScrolledRegion(window, x1, x2, y1, y2) ;

      zMapWindowRedraw(window);/* expose the blank area off the edge */

      window->scroll_x1 = x1;
      window->scroll_y1 = y1;
      window->scroll_x2 = x2;
      window->scroll_y2 = y2;

      /* -----> letting the window creator know what's going on */
      vis_change.zoom_status    = zMapWindowGetZoomStatus(window) ;
      vis_change.scrollable_top = (y1 += tmp_top); /* should these be sequence clamped */
      vis_change.scrollable_bot = (y2 -= tmp_bot); /* or include the border? (SEQUENCE CLAMPED ATM) */

      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
    }

  return ;
}




/* Sets up data that is passed back to our caller to give them information about the feature
 * the user has selected, perhaps by clicking on it in the zmap window.
 *
 * highlight_item allows the caller to specify perhaps a child or parent of say a transcript
 * to be highlighted instead of the item originally clicked on. This function passes this
 * item back to the caller which then tries to highlight it in all windows. If its NULL
 * then the original item is passed back instead.
 *
 * To Reset the panel pass in a NULL pointer as feature_arg
 *
 * Where a sub-part of a feature was selected either sub_item will be non-NULL or
 * sub_start/end will be > 0.
 *
 *
 * NOTE (mh17) AFAICS the above comment don't apply to well
 * highlight item (is that full_item??) is generally NULL or the same as sub_item and is ignored.
 * highlight is handeld externallty by focus code
 * sub_item is the feature/item that is used
 *
 * to get this working with CanvasFeatureesets I'm adding a SubPartSpan arg and filling this in
 * from the caller. handle_button() is the only place where sub_item data is displayed
 * as full item is not used then i'm removing it
 *
 * NOTE if feature =List is not null them it is a list of many features selected by the lasoo
 * item is the first one and fulfils previous functinality
 * feature_list is freed by this function
 *
 * We have burgeoning parameters here which is a sign that some rationalisation is needed....
 * e.g. not all combinations are valid....
 *
 *
 */
void zmapWindowUpdateInfoPanel(ZMapWindow window,
                               ZMapFeature feature_arg,
                               GList *feature_list, FooCanvasItem *item, ZMapFeatureSubPart sub_part,
                               int sub_item_dna_start, int sub_item_dna_end,
                               int sub_item_coords_start, int sub_item_coords_end,
                               char *alternative_clipboard_text,
                               gboolean replace_highlight_item, gboolean highlight_same_names,
                               gboolean highlight_sub_part, ZMapWindowDisplayStyle display_style)
{
  static const int max_variation_str_len = 20 ;
  char *p = NULL ;
  GString *temp_string ;
  ZMapWindowCanvasItem top_canvas_item;
  ZMapFeature feature = NULL;
  ZMapFeatureTypeStyle style ;
  ZMapWindowSelectStruct select = {(ZMapWindowSelectType)0} ;
  FooCanvasGroup *feature_group;
  ZMapStrand query_strand = ZMAPSTRAND_NONE;
  char *feature_term ;
  int feature_total_length, feature_start, feature_end, feature_length ;
  int query_start, query_end, query_length ;
  int dna_start, dna_end ;
  int display_start, display_end ;
  int start, end ;
  ZMapWindowDisplayStyleStruct default_display_style = {ZMAPWINDOW_COORD_ONE_BASED,
                                                        ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                        ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS} ;

  zMapReturnIfFail(window) ;

  if (!display_style)
    display_style = &default_display_style ;

  select.type = ZMAPWINDOW_SELECT_SINGLE ;

  /* If feature_arg is NULL then this implies "reset the info data/panel". */
  if (!feature_arg || feature_arg->mode == ZMAPSTYLE_MODE_INVALID)
    {
      (*(window->caller_cbs->select))(window, window->app_data, NULL) ;

      return ;
    }



  /* Not sure if this is the place to do this.... */
  if (highlight_sub_part && sub_part)
    select.sub_part = sub_part ;



  /* This appears to be a bit of hack by Roy to get the zmapFeatureData class initialised, can't
   * help feeling we should be calling an init function somewhere. This class essentially
   * exports feature data in human readable form. */
  if (!g_type_class_peek(ZMAP_TYPE_FEATURE_DATA))
    g_type_class_ref(ZMAP_TYPE_FEATURE_DATA);



  /*
   * Need to merge sequence and non-sequence feature code....too much repetition...
   */

  if (feature_arg->mode == ZMAPSTYLE_MODE_SEQUENCE)
    {
      /* sequence like feature. */
      const char *seq_term ;


      feature = zMapWindowCanvasItemGetFeature(FOO_CANVAS_ITEM(item)) ;

      dna_start = sub_item_dna_start ;
      dna_end = sub_item_dna_end ;

      start = sub_item_coords_start ;
      end = sub_item_coords_end ;


      if (zMapFeatureSequenceIsDNA(feature))
        {
          seq_term = "DNA" ;
        }
      else
        {
          ZMapFrame frame ;

          seq_term = "Protein" ;

          frame = zMapFeatureFrameAtCoord(feature, start) ;

          select.feature_desc.sub_feature_term   =  g_strdup("Frame") ;
          select.feature_desc.sub_feature_index  =  g_strdup(zMapFeatureFrame2Str(frame)) ;
          select.feature_desc.sub_feature_start  = g_strdup_printf("%d", start) ;
          select.feature_desc.sub_feature_end    = g_strdup_printf("%d", end) ;
          select.feature_desc.sub_feature_length = g_strdup_printf("%d", end - start + 1) ;
        }


      zmapWindowCoordPairToDisplay(window, dna_start, dna_end, &display_start, &display_end) ;


      select.feature_desc.feature_name   = (char *)g_quark_to_string(feature->original_id) ;
      select.feature_desc.feature_term   = g_strdup_printf("%s", seq_term) ;
      select.feature_desc.feature_variation_string = NULL ;
      select.feature_desc.feature_start  = g_strdup_printf("%d", display_start) ;
      select.feature_desc.feature_end    = g_strdup_printf("%d", display_end) ;
      select.feature_desc.feature_length = g_strdup_printf("%d", dna_end - dna_start + 1) ;

      /* update the info panel */
      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;


      if (zMapFeatureSequenceIsPeptide(feature))
        {
          g_free(select.feature_desc.sub_feature_start);
          g_free(select.feature_desc.sub_feature_end);
          g_free(select.feature_desc.sub_feature_length);
        }

      g_free(select.feature_desc.feature_start);
      g_free(select.feature_desc.feature_end);
      g_free(select.feature_desc.feature_length);
    }
  else
    {
      /* non-sequence like feature. */


      /* AHA.....THIS IS WHERE IT ALL GOT MESSED UP, WE STILL NEED THE SUB_FEATURE..... */
      //      sub_feature = zMapWindowCanvasItemIntervalGetData(item);
      feature = zMapWindowCanvasItemGetFeature(item);


#if 0
      //fixed by adding ZMWCI->set_feature() and zMapWindowCanvasItemSetFeature()
      /* mh17 NOTE
       * GRAPH_DENSITY items set thier feature when point() is called
       * if the mouse is moving while we are processing a click then the feature
       * pointed at by the canvas item can change causing an assertion
       * here we choose to dislay info about the one clicked on
       * the assertion is the check that a sub-item (eg exon) is really part of the containing group
       * and we break that with composite items
       * we have to ensure that the feature_arg data is passed through with any later functions
       * that update the canvas item (highlighting the feature in another window maybe?)
       * so that implicates zmapView.c/select and likely zmapControl.c too
       * note that we cannot simply choose to use feature instead of feature_arg as the mouse may be still moving
       */
      if(ZMAP_IS_WINDOW_GRAPH_DENSITY_ITEM(item))
        feature = feature_arg;
      else
#endif
        top_canvas_item = zMapWindowCanvasItemIntervalGetObject(item);

      feature_group   = zmapWindowItemGetParentContainer(FOO_CANVAS_ITEM(top_canvas_item)) ;

      style = feature && feature->style ? *feature->style : NULL ;
      select.feature_desc.struct_type = feature->struct_type ;
      select.feature_desc.type        = feature->mode ;

      select.feature_desc.feature_description = zmapWindowFeatureDescription(feature) ;


#if !MH17_FEATURE_SET_NAME_IS_COLUMN_NAME
      {
        ZMapFeatureSetDesc gffset;
        ZMapFeatureSet feature_set = (ZMapFeatureSet)
          zMapFeatureGetParentGroup((ZMapFeatureAny)feature, ZMAPFEATURE_STRUCT_FEATURESET);

        if(feature_set)
          {
            select.feature_desc.feature_set =
              g_strdup(g_quark_to_string(feature_set->original_id)) ;

            gffset = (ZMapFeatureSetDesc)g_hash_table_lookup(window->context_map->featureset_2_column,
                                         GUINT_TO_POINTER(feature_set->unique_id));
            if(gffset)
              {
                // got the featureset_2_column mapping, look up the column
                std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = 
                  window->context_map->columns->find(gffset->column_id) ;

                ZMapFeatureColumn column = NULL ;
                
                if (col_iter != window->context_map->columns->end())
                  column = col_iter->second ;

                if(column)
                  {
                    select.feature_desc.feature_set = g_strdup(g_quark_to_string(column->column_id));
                  }
              }

            if (feature_set->description)
              select.feature_desc.feature_set_description = g_strdup(feature_set->description) ;
          }
      }
#else
      zmapWindowFeatureGetSetTxt(feature,
                                 &(select.feature_desc.feature_set),
                                 &(select.feature_desc.feature_set_description)) ;
#endif


      zmapWindowFeatureGetSourceTxt(feature,
                                    &(select.feature_desc.feature_source),
                                    &(select.feature_desc.feature_source_description)) ;

      /* zero all of this. */
      feature_total_length = feature_start = feature_end = feature_length
        = query_start = query_end = query_length = 0 ;

      feature_term = NULL ;

      // Get overall feature coords.   
      if (zMapFeatureGetInfo((ZMapFeatureAny)feature, NULL,
                             "total-length", &feature_total_length,
                             "start", &feature_start,
                             "end", &feature_end,
                             "length", &feature_length,
                             "term", &feature_term,
                             NULL))
        {
          if (feature_total_length)
            select.feature_desc.feature_total_length = g_strdup_printf("%d", feature_total_length) ;

          zmapWindowCoordPairToDisplay(window, feature_start, feature_end, &feature_start, &feature_end) ;

          select.feature_desc.feature_start  = g_strdup_printf("%d", feature_start) ;
          select.feature_desc.feature_end    = g_strdup_printf("%d", feature_end) ;

          if (feature_length)
            select.feature_desc.feature_length = g_strdup_printf("%d", feature_length) ;

          /* select.feature_desc.feature_term   = feature_term; */
          const char * feature_term_string = g_quark_to_string(feature->SO_accession) ;
          select.feature_desc.feature_term =  g_strdup(feature_term_string) ;
        }


      // For an alignment also get overall query coords.   
      if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
        {
          if (zMapFeatureGetInfo((ZMapFeatureAny)feature, NULL,
                                 "query-start",  &query_start,
                                 "query-end",    &query_end,
                                 "query-length", &query_length,
                                 "query-strand", &query_strand,
                                 NULL))
            {
              select.feature_desc.feature_query_start  = g_strdup_printf("%d", query_start) ;
              select.feature_desc.feature_query_end    = g_strdup_printf("%d", query_end) ;
              select.feature_desc.feature_query_length = g_strdup_printf("%d", query_length) ;
              select.feature_desc.feature_query_strand = (char *)zMapFeatureStrand2Str(query_strand) ;
            }

          if (feature_arg->feature.homol.align)
            {
              select.feature_desc.sub_feature_none_txt = g_strdup("GAPPED - NOT DISPLAYED") ;
            }
          else
            {
              if (zMapFeatureAlignmentIsGapped(feature_arg))
                select.feature_desc.sub_feature_none_txt = g_strdup("GAPPED - NO DATA") ;
              else
                select.feature_desc.sub_feature_none_txt = g_strdup("UNGAPPED") ;

            }
        }
      else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
        {
          // THIS SEEMS A BIT HOKEY...WHEN'S IT ACTUALLY DISPLAYED....   
          if (!(feature->feature.transcript.introns))
            select.feature_desc.sub_feature_none_txt = g_strdup("NO INTRONS") ;
        }


      // Do the feature sub-part if there is one.
      if (sub_part)
        {
          char *sub_feature_term ;
          const char *prop_index, *prop_start, *prop_end, *prop_length, *prop_term ;
          int sub_feature_index, sub_feature_start, sub_feature_end, sub_feature_length ;
          int sub_feature_query_start, sub_feature_query_end, sub_feature_query_length ;
          bool gapped_exon = FALSE ;


          sub_feature_index = sub_feature_start = sub_feature_end = sub_feature_length
            = sub_feature_query_start = sub_feature_query_end = sub_feature_query_length = 0 ;

          if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT
              && (sub_part->subpart == ZMAPFEATURE_SUBPART_GAP || sub_part->subpart == ZMAPFEATURE_SUBPART_MATCH))
            gapped_exon = TRUE ;

          if (gapped_exon)
            {
              prop_index = "exon-index" ;
              prop_start = "exon-start" ;
              prop_end = "exon-end" ;
              prop_length = "exon-length" ;
              prop_term = "exon-term" ;
            }
          else
            {
              prop_index = "index" ;
              prop_start = "start" ;
              prop_end = "end" ;
              prop_length = "length" ;
              prop_term = "term" ;
            }


          // Get the subpart coords, querying with a sub_part causes the sub-part data to be
          // returned rather than data for the overall feature.
          if (zMapFeatureGetInfo((ZMapFeatureAny)feature, sub_part,
                                 prop_index,  &sub_feature_index,
                                 prop_start,  &sub_feature_start,
                                 prop_end,    &sub_feature_end,
                                 prop_length, &sub_feature_length,
                                 prop_term,   &sub_feature_term,
                                 NULL))
            {
              zmapWindowCoordPairToDisplay(window, sub_feature_start, sub_feature_end,
                                           &sub_feature_start, &sub_feature_end) ;

              select.feature_desc.sub_feature_index = g_strdup_printf("%d", sub_feature_index) ;
              select.feature_desc.sub_feature_start = g_strdup_printf("%d", sub_feature_start) ;
              select.feature_desc.sub_feature_end = g_strdup_printf("%d", sub_feature_end) ;
              select.feature_desc.sub_feature_length = g_strdup_printf("%d", sub_feature_length) ;
              select.feature_desc.sub_feature_term = sub_feature_term ;
            }

          if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT || gapped_exon)
            {
              if (zMapFeatureGetInfo((ZMapFeatureAny)feature, sub_part,
                                     "query-start",  &sub_feature_query_start,
                                     "query-end",    &sub_feature_query_end,
                                     "query-length", &sub_feature_query_length,
                                     NULL))
                {
                  select.feature_desc.sub_feature_query_start = g_strdup_printf("%d", sub_feature_query_start) ;
                  select.feature_desc.sub_feature_query_end   = g_strdup_printf("%d", sub_feature_query_end) ;

                  // why is length not recorded here ?   
                }
            }

          // For a gapped exon record the exon that the match was in.
          if (gapped_exon)
            {
              int match_index = 0, match_start = 0, match_end = 0, match_length = 0 ;
              char *match_term = NULL ;

              if (zMapFeatureGetInfo((ZMapFeatureAny)feature, sub_part,
                                     "index",  &match_index,
                                     "start",  &match_start,
                                     "end",    &match_end,
                                     "length", &match_length,
                                     "term", &match_term,
                                     NULL))
                {
                  select.feature_desc.subpart_feature_index = g_strdup_printf("%d", match_index) ;
                  select.feature_desc.subpart_feature_start = g_strdup_printf("%d", match_start) ;
                  select.feature_desc.subpart_feature_end = g_strdup_printf("%d", match_end) ;
                  select.feature_desc.subpart_feature_length = g_strdup_printf("%d", match_length) ;
                  select.feature_desc.subpart_feature_term = match_term ;
                }
            }


        }
      else
        {
          // Not sure why we do this.....   
          select.feature_desc.sub_feature_term = (char *)"-";
        }

      zMapFeatureGetInfo((ZMapFeatureAny)feature, NULL,
                         "locus", &(select.feature_desc.feature_locus), NULL);

      if (ZMAPFEATURE_IS_BASIC(feature) && feature->feature.basic.flags.variation_str)
        {
          select.feature_desc.feature_name = g_strdup_printf("%s (%s)",
                                                             (char *)g_quark_to_string(feature->original_id),
                                                             (char *)(strlen(feature->feature.basic.variation_str)
                                                                      < max_variation_str_len
                                                                      ? feature->feature.basic.variation_str
                                                                      : "allele str") ) ;

          if (strlen(feature->feature.basic.variation_str) >= max_variation_str_len)
            {
              /* select.feature_desc.feature_variation_string = g_strdup(feature->feature.basic.variation_str) ; */

              temp_string = g_string_new("") ;
              p = feature->feature.basic.variation_str ;
              while (strlen(p) > max_variation_str_len)
                {
                  g_string_append_len(temp_string, p, max_variation_str_len) ;
                  g_string_append(temp_string, "\n") ;
                  p += max_variation_str_len ;
                }
              g_string_append(temp_string, p) ;
              select.feature_desc.feature_variation_string = g_string_free(temp_string, FALSE ) ;

            }

        }
      else
        {
          select.feature_desc.feature_name = g_strdup((char *)g_quark_to_string(feature->original_id)) ;
        }

      if (feature->mode == ZMAPSTYLE_MODE_BASIC && feature->feature.basic.known_name)
        select.feature_desc.feature_known_name = (char *)g_quark_to_string(feature->feature.basic.known_name) ;
      else if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT && feature->feature.transcript.known_name)
        select.feature_desc.feature_known_name = (char *)g_quark_to_string(feature->feature.transcript.known_name) ;

      select.feature_desc.feature_strand = (char *)zMapFeatureStrand2Str(zmapWindowStrandToDisplay(window, feature->strand)) ;

      if (zMapStyleIsFrameSpecific(style))
        select.feature_desc.feature_frame = zMapFeatureFrame2Str(zmapWindowFeatureFrame(feature)) ;

      if (feature->population)
        select.feature_desc.feature_population = g_strdup_printf("%d", feature->population) ;


      /* quality measures. */
      if (feature->flags.has_score)
        select.feature_desc.feature_score = g_strdup_printf("%g", (double) feature->score) ;

      if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
        {
          if (feature->feature.homol.percent_id)
            select.feature_desc.feature_percent_id = g_strdup_printf("%g%%",
                                                                     (double)(feature->feature.homol.percent_id)) ;

        }


      if (style)
        select.feature_desc.feature_type = (char *)zMapStyleMode2ExactStr(zMapStyleGetMode(style)) ;

      select.highlight_item = item ;

      select.feature_list = feature_list;


      select.replace_highlight_item = replace_highlight_item ;

      select.highlight_same_names = highlight_same_names ;

      select.highlight_sub_part = highlight_sub_part ;


      /* dis/enable the filter by score widget and set min and max */
      select.filter.enable = FALSE;

      if (style && zMapStyleIsFilter(style) && ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
        {
          FooCanvasItem *foo = (FooCanvasItem *) item;

          select.filter.min_val = zMapStyleGetMinScore(style);
          select.filter.max_val = zMapStyleGetMaxScore(style);
          select.filter.value = zMapWindowCanvasFeaturesetGetFilterValue(foo);
          select.filter.n_filtered = zMapWindowCanvasFeaturesetGetFilterCount(foo);
          select.filter.featureset = (ZMapWindowFeaturesetItem) foo;
          select.filter.column =  item;/* needed for re-bumping */
          select.filter.enable = TRUE;
          select.filter.window = window;

          select.filter.func = zMapWindowCanvasFeaturesetFilter;
        }


      /* We've set up the select data so now callback to the layer above with this data. */
      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

      /* Clear up.... */
      g_free(select.feature_desc.feature_name) ;
      g_free(select.feature_desc.sub_feature_start) ;
      g_free(select.feature_desc.sub_feature_end) ;
      g_free(select.feature_desc.sub_feature_query_start) ;
      g_free(select.feature_desc.sub_feature_query_end) ;
      g_free(select.feature_desc.feature_set) ;
      if (select.feature_desc.feature_set_description)    // mh17: can be null
        g_free(select.feature_desc.feature_set_description) ;
      g_free(select.feature_desc.feature_start) ;
      g_free(select.feature_desc.feature_end) ;
      g_free(select.feature_desc.feature_term) ;
      g_free(select.feature_desc.feature_query_start) ;
      g_free(select.feature_desc.feature_query_end) ;
      g_free(select.feature_desc.feature_query_length) ;
      g_free(select.feature_desc.feature_length) ;
      g_free(select.feature_desc.feature_description) ;

    }

  /* We wait until here to do this so we are only setting the
   * clipboard text once. i.e. for this window. And so that we have
   * updated the focus object correctly. */
  if (alternative_clipboard_text)
    {
      select.secondary_text = alternative_clipboard_text ;
    }
  else if (zMapFeatureSequenceIsDNA(feature))
    {
      char *dna_string, *seq_name;

      dna_string = zMapFeatureGetDNA((ZMapFeatureAny)feature, sub_item_dna_start, sub_item_dna_end, FALSE);

      seq_name = g_strdup_printf("%d-%d", display_start, display_end);

      select.secondary_text = zMapFASTAString(ZMAPFASTA_SEQTYPE_DNA,
                                              seq_name, "DNA", NULL,
                                              end - start + 1,
                                              dna_string);
      g_free(seq_name);
    }
  else if (zMapFeatureSequenceIsPeptide(feature))
    {
      ZMapPeptide translation;
      char *dna_string, *seq_name;

      /* Get peptide by translating the corresponding dna, necessary because
       * there might be trailing part codons etc. */
      dna_string  = zMapFeatureGetDNA((ZMapFeatureAny)feature, sub_item_dna_start, sub_item_dna_end, FALSE) ;
      seq_name    = g_strdup_printf("%d-%d (%d-%d)", start, end, display_start, display_end) ;
      translation = zMapPeptideCreate(seq_name, NULL, dna_string, NULL, TRUE) ;
      select.secondary_text = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA,
                                              seq_name, "Protein", NULL,
                                              zMapPeptideLength(translation),
                                              zMapPeptideSequence(translation)) ;

      g_free(seq_name);
      zMapPeptideDestroy(translation);
    }
  else
    {
      select.secondary_text = zmapWindowMakeFeatureSelectionTextFromSelection(window, display_style) ;
    }

  /* this puts the DNA in the clipbaord */
  if (select.secondary_text)
    {
      zMapGUISetClipboard(window->toplevel, GDK_SELECTION_PRIMARY, select.secondary_text) ;

      g_free(select.secondary_text) ;
    }

  if(feature_list)
    g_list_free(feature_list);

  return ;
}





/* I'm not convinced of this. */
void zMapWindowSiblingWasRemoved(ZMapWindow window)
{
  zMapReturnIfFail(window) ;

  /* Currently this is all we do here. */
  zmapWindowScaleCanvasOpenAndMaximise(window->ruler);

  return ;
}



void zMapWindowStateRecord(ZMapWindow window)
{
  ZMapWindowState state;

  if((state = zmapWindowStateCreate()))
    {
      zmapWindowStateSaveZoom(state, zMapWindowGetZoomFactor(window));
      zmapWindowStateSaveMark(state, window);
      zmapWindowStateSavePosition(state, window);

      if(!zmapWindowStateQueueStore(window, state, TRUE))
        state = zmapWindowStateDestroy(state);
#if 0
we-re recording the state not changing it
if this function needs calling then it should be done by the caller
NOTE StateRecord is only called by myWindowZoom (which calls SetScrollRegion which calls visibilityChange)
or when setting the mark

      else
{
  invokeVisibilityChange(window);
}
#endif
    }

  return ;
}



/* One of Malcolm's routines...breaking our naming conventions completely.... */
void foo_bug_set(void *key, const char *id)
{
#if 0
  struct fooBug * fb = foo_wins + n_foo_wins++;
  extern void (*foo_bug)(void *, char *);

  fb->key = key;
  fb->id = id;
  printf("foo bug set %s %d\n",fb->id, n_foo_wins - 1);
  foo_bug = foo_bug_print;
#endif
}




/*
 *                       Internal routines
 */

static void panedResizeCB(gpointer data, gpointer userdata)
{
  ZMapWindow window = (ZMapWindow)userdata;
  int *new_position = (int *)data, current_position;

  zMapReturnIfFail(window) ;

  current_position = gtk_paned_get_position(GTK_PANED( window->pane ));

  gtk_paned_set_position(GTK_PANED( window->pane ),
                         *new_position);

  return ;
}






/* We will need to allow caller to specify a routine that gets called whenever the user
 * scrolls.....needed to update the navigator...... */
/* and we will need a callback for focus events as well..... */
/* I think probably we should insist on being supplied with a sequence.... */
/* NOTE that not all fields are initialised here as some need to be done when we draw
 * the actual features. */
static ZMapWindow myWindowCreate(GtkWidget *parent_widget,
                                 ZMapFeatureSequenceMap sequence, void *app_data,
                                 GList *feature_set_names,
                                 GtkAdjustment *hadjustment,
                                 GtkAdjustment *vadjustment,
                                 int *int_values)
{
  ZMapWindow window = NULL ;
  GtkWidget *canvas, *eventbox ;

  /* No callbacks, then no window creation. */
  if (!window_cbs_G)
    return window ;

  if (!parent_widget || !sequence || !sequence->sequence || !app_data)
    return window ;

  window = g_new0(ZMapWindowStruct, 1) ;

  window->config.align_spacing = ALIGN_SPACING ;
  window->config.block_spacing = BLOCK_SPACING ;
#if USE_STRAND
  window->config.strand_spacing = STRAND_SPACING ;
#endif
  window->config.column_spacing = COLUMN_SPACING ;
  window->config.feature_spacing = FEATURE_SPACING ;
  window->config.feature_line_width = FEATURE_LINE_WIDTH ;


  window->caller_cbs = window_cbs_G ;

  /* If a remote_request_func was registered then we know a remote client was registered. */
  if (window->caller_cbs->remote_request_func)
    window->xremote_client = TRUE ;

  window->sequence = sequence;


  window->app_data = app_data ;
  window->parent_widget = parent_widget ;

  window->display_forward_coords = TRUE ;

  window->feature_set_names   = g_list_copy(feature_set_names);

  window->toplevel = eventbox = gtk_event_box_new();
  window->pane     = gtk_hpaned_new();

  gtk_container_set_border_width(GTK_CONTAINER(window->pane), 2);

  {                             /* A quick experiment with resource files */
    char *widget_name = NULL;
    widget_name = g_strdup_printf("zmap-view-%s", window->sequence->sequence);
    gtk_widget_set_name(GTK_WIDGET(window->toplevel), widget_name);
    g_free(widget_name);
  }

  g_signal_connect(GTK_OBJECT(window->toplevel), "event",
   GTK_SIGNAL_FUNC(windowGeneralEventCB), (gpointer)window) ;

  /* colours may be overwritten by configuration later. */
  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(window->canvas_background)) ;

  gdk_color_parse(ZMAP_WINDOW_ITEM_FILL_COLOUR, &(window->canvas_fill)) ;
  gdk_color_parse(ZMAP_WINDOW_ITEM_BORDER_COLOUR, &(window->canvas_border)) ;

  gdk_color_parse("green", &(window->align_background)) ;

  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  //  window->zoom_factor = 0.0 ;
  //  window->zoom_status = ZMAP_ZOOM_INIT ;
  window->canvas_maxwin_size = ZMAP_WINDOW_MAX_WINDOW ;

  window->min_coord = window->max_coord = 0.0 ;
  window->display_origin = 0.0 ;

  /* Some things for window can be specified in the configuration file. */
  getConfiguration(window) ;

  /* Add a hash table to map features to their canvas items. */
  window->context_to_item = zmapWindowFToICreate() ;

  window->int_values = int_values ;

  /* Init. lists of dialog windows attached to this zmap window. */
  window->featureListWindows = g_ptr_array_new() ;
  window->search_windows = g_ptr_array_new() ;
  window->dna_windows = g_ptr_array_new() ;
  window->dnalist_windows = g_ptr_array_new() ;
  window->edittable_features = FALSE ;    /* By default features are not edittable. */
  window->feature_show_windows = g_ptr_array_new() ;

  /* Init focus item/column stuff. */
  window->focus = zmapWindowFocusCreate(window) ;

  /* Init mark stuff. */
  window->mark = zmapWindowMarkCreate(window) ;

  /* Set up cursor stuff, note that user can set a normal cursor in the config file
   * but that otherwise we will default to the parent's widget. */
  window->window_busy_cursor = zMapGUICreateCursor(BUSY_CURSOR) ;
  window->cursor_busy_count = 0 ;


  /* Set up a scrolled widget to hold the canvas. NOTE that this is our toplevel widget. */
  window->scrolled_window = gtk_scrolled_window_new(hadjustment, vadjustment) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), window->toplevel) ;
  gtk_container_add(GTK_CONTAINER(window->toplevel), window->pane) ;
  gtk_paned_add2(GTK_PANED(window->pane), window->scrolled_window);



  /* ACTUALLY I'M NOT SURE WHY THE SCROLLED WINDOW IS GETTING THESE...WHY NOT JUST SEND
   * DIRECT TO CANVAS.... */
  /* This handler receives the feature data from the threads. */
  gtk_signal_connect(GTK_OBJECT(window->toplevel), "client_event",
                     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;

  /* always show scrollbars, however big the display */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->scrolled_window),
                                 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS) ;

  /* Create the canvas, add it to the scrolled window so all the scrollbar stuff gets linked up
   * and set the background to be white. */
  canvas = foo_canvas_new();
  window->canvas = FOO_CANVAS(canvas);

  g_signal_connect(canvas, "drag-data-get", G_CALLBACK(dragDataGetCB), window);

  foo_bug_set(canvas,"window");

  /* This will be removed when RT:1589 is resolved */
  g_object_set_data(G_OBJECT(canvas), ZMAP_WINDOW_POINTER, window);

  /* Definitively set the canvas to have a scroll region size, that WE
   * know and can test for.  If the foo_canvas default changes then
   * later our might fail. */
  zmapWindowSetScrolledRegion(window, 0.0, 0.0, ZMAP_CANVAS_INIT_SIZE, ZMAP_CANVAS_INIT_SIZE) ;

  gtk_container_add(GTK_CONTAINER(window->scrolled_window), canvas) ;

  gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &(window->canvas_background)) ;


  /* Make the canvas focussable, we want the canvas to be the focus widget of its "window"
   * otherwise keyboard input (i.e. short cuts) will be delivered to some other widget. */
  GTK_WIDGET_SET_FLAGS(canvas, GTK_CAN_FOCUS) ;



  if (!(window->zoom = zmapWindowZoomControlCreate(window)))
    {
      zMapExitMsg("%s", "Zoom Control create failed, see zmap log.") ;
      zMapLogFatal("%s", "Zoom Control create failed, see zmap log.") ;
    }


  /*
   * This is setting up the scale bar; it appears to need the
   * parent window.
   */
  ZMapWindowScaleCanvasCallbackListStruct callbacks = {NULL};
  GtkAdjustment *vadjust = NULL;

  vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
  callbacks.paneResize = panedResizeCB;
  callbacks.user_data  = (gpointer)window;

  window->ruler = zmapWindowScaleCanvasCreate(&callbacks);
  zmapWindowScaleCanvasInit(window->ruler, window->pane, vadjust);

  window->display_coordinates = ZMAP_WINDOW_DISPLAY_SLICE ;

  /* Attach callback to monitor size changes in canvas, this works but bizarrely
   * "configure-event" callbacks which are the pucker size change event are never called. */
  g_signal_connect(GTK_OBJECT(window->canvas), "size-allocate",
                   GTK_SIGNAL_FUNC(canvasSizeAllocateCB), (gpointer)window) ;


  /* NONE OF THIS SHOULD BE HARD CODED constants it should be based on window size etc... */
  /* you have to set the step_increment manually or the scrollbar arrows don't work.*/
  GTK_LAYOUT(canvas)->vadjustment->step_increment = ZMAP_WINDOW_STEP_INCREMENT;
  GTK_LAYOUT(canvas)->hadjustment->step_increment = ZMAP_WINDOW_STEP_INCREMENT;
  GTK_LAYOUT(canvas)->vadjustment->page_increment = ZMAP_WINDOW_PAGE_INCREMENT;


  gtk_widget_show_all(window->parent_widget) ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* OK...I'M NOT SURE WE SHOULD BE DOING THIS....THERE MUST BE A BETTER MECHANISM.... */

  /* IS THIS OUR PROBLEM WITH FREEZING....NO, IT'S NOT !! */

  /* We want the canvas to be the focus widget of its "window" otherwise keyboard input
   * (i.e. short cuts) will be delivered to some other widget. We do this here because
   * I think we may need a widget window to exist for this call to work. */
  gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






#if ZOOM_SCROLL
  /* These signals are now the way to handle the long items cropping.
   * begin update is the place to do this.
   * end update is for symmetry and informational purposes.
   */
  g_signal_connect(GTK_OBJECT(window->canvas), "begin-update",
   GTK_SIGNAL_FUNC(fc_begin_update_cb), (gpointer)window);
  g_signal_connect(GTK_OBJECT(window->canvas), "end-update",
   GTK_SIGNAL_FUNC(fc_end_update_cb), (gpointer)window);


  /* We need to get signalled when the canvas is busy and mapping can take significant time. */
  g_signal_connect(GTK_OBJECT(window->canvas), "begin-map",
   GTK_SIGNAL_FUNC(fc_begin_map_cb), (gpointer)window);
  g_signal_connect(GTK_OBJECT(window->canvas), "end-map",
   GTK_SIGNAL_FUNC(fc_end_map_cb), (gpointer)window);


  /* These really need to test for (canvas->root->object.flags & FOO_CANVAS_ITEM_MAPPED)
   * The reason for this is symmetry and not moving the draw-background signal in the
   * foocanvas code.
   */
  g_signal_connect(GTK_OBJECT(window->canvas), "draw-background",
   GTK_SIGNAL_FUNC(fc_draw_background_cb), (gpointer)window) ;
  g_signal_connect(GTK_OBJECT(window->canvas), "drawn-items",
   GTK_SIGNAL_FUNC(fc_drawn_items_cb), (gpointer)window);
#endif

  window->history = zmapWindowStateQueueCreate();

  return window ;
}


static ZMapFeatureContextExecuteStatus undisplayFeaturesCB(GQuark key,
                                                           gpointer data,
                                                           gpointer user_data,
                                                           char **err_out)
{
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeature feature;
  FooCanvasItem *feature_item;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapStrand column_strand;

  zMapReturnValIfFail(feature_any, status) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      zMapLogWarning("FeatureSet %s", g_quark_to_string(feature_any->unique_id));
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
        feature = (ZMapFeature)feature_any;

        /* which column drawn in depends on style. */
        column_strand = zmapWindowFeatureStrand(window, feature);

#if MH17_FToIHash_does_this_mapping
      /* if the feature context is from a request from otterlace then
       * the display column has not been set, we need to lookup
       */
          ZMapFeatureSetDesc gffset;
          ZMapFeatureSet fset;

      fset = (ZMapFeatureSet) (feature_any->parent);
      if(!fset->column_id)
      {
            gffset = g_hash_table_lookup(window->context_map->featureset_2_column, GUINT_TO_POINTER(fset->unique_id));
            if(gffset)
                  fset->column_id = gffset->column_id;
      }
#endif
      /* MH17: we get locus features inserted mysteriously if a feature has a locus id
       * but they don't always appear in zmap in whcih case there is no column id
       * This is true when otterlace sends a single feature to delete and then we fail to find
       * the extra locus feature
       *
       * regardless of that if we have features that are not displayed due to config this couls also fail
       * so if not column_id defined log a warnign adn fail silently.
       *
       * locus is used in the naviagtor, we hope dealt with via another call.
       */

        if ( /*fset->column_id && */
              (feature_item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
                                                          column_strand,
                                                          ZMAPFRAME_NONE,
                                                          feature)))
          {
            zMapWindowFeatureRemove(window, feature_item, feature, FALSE);
            status = ZMAP_CONTEXT_EXEC_STATUS_OK;
          }
        else
          zMapLogWarning("Failed to find feature \"%s\"", g_quark_to_string(feature->original_id));

        break;
      }
    default:
      /* nothing to do for most of it while we only have single blocks and aligns... */
      break;
    }

  return status;
}


static void invokeVisibilityChange(ZMapWindow window)
{
  ZMapWindowVisibilityChangeStruct change = {};

  zMapReturnIfFail(window) ;

  change.zoom_status = zMapWindowGetZoomStatus(window);

  foo_canvas_get_scroll_region(window->canvas,
                               NULL, &(change.scrollable_top),
                               NULL, &(change.scrollable_bot));

  zmapWindowClampedAtStartEnd(window, &(change.scrollable_top), &(change.scrollable_bot));

  (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&change);

  return ;
}

/* Zooming the canvas window in or out.
 * Note that zooming is only in the Y axis, the X axis is not zoomed at all as we don't need
 * to make the columns wider. This has required a local modification of the foocanvas.
 *
 * window        the ZMapWindow (i.e. canvas) to be zoomed.
 * zoom_factor   > 1.0 means zoom in, < 1.0 means zoom out.
 *
 *  */
static void myWindowZoom(ZMapWindow window, double zoom_factor, double curr_pos)
{
  GtkAdjustment *adjust;
  int x, y ;
  double x1, y1, x2, y2, width ;
  double new_canvas_span ;

  zMapReturnIfFail(window) ;

//printf("myWindowZoom 1\n");

  if (window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
    {
      adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
      foo_canvas_get_scroll_offsets(window->canvas, &x, &y) ;
      y += adjust->page_size / 2 ;
      foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;
    }

  zMapWindowStateRecord(window);

  /* We don't zoom in further than is necessary to show individual bases or further out than
   * is necessary to show the whole sequence, or if the sequence is so short that it can be
   * shown at max. zoom in its entirety. */

  if (zmapWindowZoomControlZoomByFactor(window, zoom_factor))
    {
      double half_new_span;

      x1 = y1 = x2 = y2 = 0.0;

      /* Calculate the extent of the new span, new span must not exceed maximum X window size
       * but we must display as much of the sequence as we can for zooming out. */
      zmapWindowGetScrollableArea(window, &x1, &y1, &x2, &y2);
      new_canvas_span = zmapWindowZoomControlLimitSpan(window, y1, y2);
      half_new_span   = (new_canvas_span / 2);

      y1 = (curr_pos - half_new_span);
      y2 = (curr_pos + half_new_span);

      zmapWindowClampSpan(window, &y1, &y2);



      /* Set the new scroll_region and the new zoom, keep these together to try
       * and avoid a double redraw as both these operations cause update requests
       * and if separated by other gtk calls will result in double draws.
       * If this doesn't work we'll need to provide a new single foocanvas call
       * to set both zoom and scroll region in one go. */
/* MH17: they cause expose events internally so we use foo_canvas_busy() around this function */

      zmapWindowSetPixelxy(window, 1.0, zMapWindowGetZoomFactor(window)) ;


      /* must set zoom before SetScroll as that set Scroll may redraw the ruler */
      zmapWindowScaleCanvasSetPixelsPerUnit(window->ruler, 1.0, zMapWindowGetZoomFactor(window));

      zmapWindowSetScrollableArea(window, &x1, &y1, &x2, &y2,"myWindowZoom");


      /* Now we've actually done the zoom on the canvas we can
       * redraw/show/hide everything that's zoom dependent.
       * E.G. Text, which is separated differently @ different zooms otherwise.
       *      zoom dependent features, magnification dependent columns
       */

      /* Firstly the scale bar, which will be changed soon. */
      zmapWindowDrawZoom(window);


      if(window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
        {
          foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
          foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));
        }

#if DOES_NOTHING
      /* We need to redo the highlighting in the columns with a zoom handler.  i.e. DNA column... */
      /* NOTE this function does nothing */
      zmapWindowReFocusHighlights(window);
#endif

      zMapWindowRedraw(window);    /* exposes the blank edges as well as the canvas */
    }

  return ;
}





/* Move the window to a new part of the canvas, we need this because when the window is
 * zoomed in, it may not extend over the whole canvas so this kind of alternative scrolling.
 * is needed to reach parts of the canvas not currently mapped. */
static void myWindowMove(ZMapWindow window, double start, double end)
{
  zmapWindowBusy(window, TRUE) ;

  /* Clamp the start/end. */
  zmapWindowClampSpan(window, &start, &end);

  /* Code that looks so simple moves the canvas...  */
//  zmapWindowContainerRequestReposition(window->feature_root_group);

// actually not relevant here we are changing Y not X
//  zmapWindowFullReposition(window->feature_root_group,TRUE);

  zmapWindowSetScrollableArea(window, NULL, &start, NULL, &end,"myWindowMove");

  zmapWindowBusy(window, FALSE) ;

  return ;
}


/* This function resets the canvas to be empty, all of the canvas items we drew
 * are destroyed and all the associated resources free'd.
 *
 * NOTE that this _must_ not touch the feature context which belongs to the parent View. */
static void resetCanvas(ZMapWindow window, gboolean free_child_windows, gboolean free_revcomp_safe_windows)
{

  if (!window)
    return ;

  /* There is code here that should be shared with zmapwindowdestroy....
   * BUT NOTE THAT SOME THINGS ARE NOT SHARED...e.g. RECREATION OF THE FEATURELISTWINDOWS
   * ARRAY.... */

  /* I think we'll do the destroying of the child windows first... */
  /* Some of them might refer, or request to refer to
   * containers/feature to item hash or something we
   * destroy in the next bit. After all they are done first
   * in zmapwindowdestroy See RT 69860 */
  if (free_child_windows)
    {
      /* free the array of featureListWindows and the windows themselves */
      if (free_revcomp_safe_windows)
        zmapWindowFreeWindowArray(&(window->featureListWindows), FALSE) ;

      /* free the array of search windows and the windows themselves */
      zmapWindowFreeWindowArray(&(window->search_windows), FALSE) ;

      /* free the array of dna windows and the windows themselves */
      zmapWindowFreeWindowArray(&(window->dna_windows), FALSE) ;

      /* free the array of dna list windows and the windows themselves */
      zmapWindowFreeWindowArray(&(window->dnalist_windows), FALSE) ;

      /* free the array of editor windows and the windows themselves */
      zmapWindowFreeWindowArray(&(window->feature_show_windows), FALSE) ;

        if(window->style_window)
        zmapWindowStyleDialogDestroy(window);
    }

    zmapWindowFocusReset(window->focus);

/* mh17 band aid approach to fixing 3FT columns
 * these never come back if we revcomp when they are on display
 * & yet removing them first is ok
 * somewhere in the mas co container code there's some kind of broken
 */
//zmapWindowDrawRemove3FrameFeatures(window);

      // destroy focus before the canvas items via
      // zmapWindowContainerGroupDestroy(window->feature_root_group)
    zmapWindowFocusDestroy(window->focus) ;
    window->focus = NULL;

    if (window->feature_root_group)
    {
      /* Reset mark object, must come before root group etc. is destroyed as we need to remove
       * various canvas items. */
      zmapWindowMarkDestroy(window->mark) ;
      window->mark = zmapWindowMarkCreate(window) ;

      zmapWindowContainerGroupDestroy(window->feature_root_group) ;
      window->feature_root_group = NULL ;

    }


// ok?? #warning MH17 restoring bug to fix another one: need to verify both are gone
/*
 * on RevComp if you do not have 1-based coordinates then you jump to a
 * wildly different coordinate range. This means that the scroll reqgion is
 * completely wrong and cannot be restored.
 * due to coding choices elsewhere (zmapGUIutils.c/zMapGUICoordsClampToLimits()
 * on one of the start/end coordinates gets clamped resulting in top > bot
 * and its all a big mess
 * resetting the scroll region means create it from scratch when drawing features
 * which is what we need to do
 */
#ifdef CAUSED_RT_57193
  /* The windowstate code was saving the scrollregion and adjuster
   * position to enable resetting once rev-comped. However the sibling
   * windows position was changed by this as internally foo-canvas
   * does a scroll-to which alters the _shared_ adjuster. This
   * position was then saved as the next sibling went through this
   * code and restored to the start of the canvas when it came to
   * restoring it! */
  if (window->canvas)
    foo_canvas_set_scroll_region(window->canvas,
                                 0.0, 0.0,
                                 ZMAP_CANVAS_INIT_SIZE, ZMAP_CANVAS_INIT_SIZE) ;
#else
  /* better to use an explicit flag */
  /* we need to reset this on revcomp as the scroll region can be completely different */
  if(window->canvas)
    {
      /* if we revcomp then what happens is we redisplay whe whole sequence at the previous zoom level
       * and then draw the scale bar at that zoom which makes it think we have 17 million pixels
       * which is wrong
       * resetting the zoom means we loose it so here we need to revcomp the scroll region
       * using the same logic as it is used in zmapWindowState.c
       * here we set the scroll region to reflect what we are drawing
       * and window state restore then later adjusts everything using the
       * previous saved coordinates that are revcomped to agree with this
       */
      if(zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) && window->scroll_initialised && free_revcomp_safe_windows)
        {
          double y1,y2,x1,x2;

          foo_canvas_get_scroll_region(window->canvas,&x1,&y1,&x2,&y2);
          zmapWindowStateRevCompRegion(window, &y1, &y2);

          zmapWindowSetScrolledRegion(window, x1, x2, y1, y2) ;
        }
      else
        {
          window->scroll_initialised = FALSE;
        }
    }
#endif

  zmapWindowFToIDestroy(window->context_to_item) ;
  window->context_to_item = zmapWindowFToICreate() ;


  /* Recreate focus object. */
//  zmapWindowFocusDestroy(window->focus) ;
  window->focus = zmapWindowFocusCreate(window) ;

  return ;
}



/* Moves can either be of the scroll bar within the scrolled window or of where the whole
 * scrolled region is within the canvas. */
static void moveWindow(ZMapWindow window, GdkEventKey *key_event)
{
  zMapReturnIfFail(window && key_event) ;
  if (zMapGUITestModifiers(key_event, (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    changeRegion(window, key_event->keyval) ;
  else
    scrollWindow(window, key_event) ;

  return ;
}



/* Move the canvas within its current scroll region, i.e. this is exactly like the user scrolling
 * the canvas via the scrollbars. */
static void scrollWindow(ZMapWindow window, GdkEventKey *key_event)
{
  enum {OVERLAP_FACTOR = 10} ;
  int x_pos, y_pos ;
  int incr ;
  guint state, keyval ;

  zMapReturnIfFail(window && key_event) ;

  state = key_event->state ;
  keyval = key_event->keyval ;

  /* Retrieve current scroll position. */
  foo_canvas_get_scroll_offsets(window->canvas, &x_pos, &y_pos) ;


  switch (keyval)
    {
    case GDK_Home:
    case GDK_End:
      {
        GtkAdjustment *v_adjust, *h_adjust ;

        v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
        h_adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

        if (keyval == GDK_Home)
          {
            if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
              y_pos = v_adjust->lower ;
            else
              x_pos = h_adjust->lower ;
          }

        if (keyval == GDK_End)
          {
            if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
              y_pos = v_adjust->upper ;
            else
              x_pos = h_adjust->upper ;
          }

        break ;
      }
    case GDK_Page_Up:
    case GDK_Page_Down:
      {
        GtkAdjustment *adjust ;

        adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

        if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
          incr = adjust->page_size - (adjust->page_size / OVERLAP_FACTOR) ;
        else
          incr = (adjust->page_size - (adjust->page_size / OVERLAP_FACTOR)) / 2 ;

        if (keyval == GDK_Page_Up)
          y_pos -= incr ;
        else
          y_pos += incr ;

        break ;
      }
    case GDK_Up:
    case GDK_Down:
      {
        double x_world, y_world ;
        double new_y ;

        /* Retrieve our position in the world from the scroll postion. */
        foo_canvas_c2w(window->canvas, x_pos, y_pos, &x_world, &y_world) ;

        /* work out where we will be after we've scrolled the right amount. */
        if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
          new_y = 1000 ;    /* window->major_scale_units */
        else
          new_y = 100 ;    /* window->minor_scale_units */

        if (keyval == GDK_Up)
          new_y *= -1 ;

        new_y += y_world ;

        foo_canvas_w2c(window->canvas, 0.0, new_y, NULL, &incr) ;

        y_pos = incr ;

        break ;
      }
    case GDK_Left:
    case GDK_Right:
      {
        double incr ;

        if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
          incr = 10 ;
        else
          incr = 1 ;

        if (keyval == GDK_Left)
          x_pos -= incr ;
        else
          x_pos += incr ;

        break ;
      }
    }

  /* No need to clip to start/end here, this call will do it for us. */
  foo_canvas_scroll_to(window->canvas, x_pos, y_pos) ;

  return ;
}


/* Zooms the window a little or a lot. */
static void zoomWindow(ZMapWindow window, GdkEventKey *key_event)
{
  double zoom_factor = 0.0 ;

  switch (key_event->keyval)
    {
    case GDK_KP_Add:
    case GDK_plus:
    case GDK_equal:
      if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
        zoom_factor = 2.0 ;
      else
        zoom_factor = 1.1 ;
      break ;
    case GDK_KP_Subtract:
    case GDK_minus:
      if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
        zoom_factor = 0.5 ;
      else
        zoom_factor = 0.909090909 ;
      break ;
    }

  zMapWindowZoom(window, zoom_factor) ;

  return ;
}





/* Change the canvas scroll region, this may be necessary when the user has zoomed in so much
 * that the window has reached the maximum allowable by X Windows, therefore we have shrunk the
 * canvas scrolled region to accomodate further zooming. Hence this routine does a kind of
 * secondary and less interactive scrolling by moving the scrolled region. */
static void changeRegion(ZMapWindow window, guint keyval)
{
  double overlap_factor = 0.9 ;
  double x1, y1, x2, y2 ;
  double incr ;
  double window_size ;

  zMapReturnIfFail(window) ;

  /* THIS FUNCTION NEEDS REFACTORING SLIGHTLY */
  zmapWindowGetScrollableArea(window, &x1, &y1, &x2, &y2);

  /* There is no sense in trying to scroll the region if we already showing all of it already. */
  if (y1 > window->min_coord || y2 < window->max_coord)
    {
      window_size = y2 - y1 + 1 ;

      switch (keyval)
        {
        case GDK_Page_Up:
        case GDK_Page_Down:
          {
            incr = window_size * overlap_factor ;

            if (keyval == GDK_Page_Up)
              {
                y1 -= incr ;
                y2 -= incr ;
              }
            else
              {
                y1 += incr ;
                y2 += incr ;
              }

            break ;
          }
        case GDK_Up:
        case GDK_Down:
          {
            incr = window_size * (1 - overlap_factor) ;

            if (keyval == GDK_Up)
              {
                y1 -= incr ;
                y2 -= incr ;
              }
            else
              {
                y1 += incr ;
                y2 += incr ;
              }

            break ;
          }
        case GDK_Left:
        case GDK_Right:
          {
            printf("NOT IMPLEMENTED...\n") ;

            break ;
          }
        }

      if (y1 < window->min_coord)
        {
          y1 = window->min_coord ;
          y2 = y1 + window_size - 1 ;
        }
      if (y2 > window->max_coord)
        {
          y2 = window->max_coord ;
          y1 = y2 - window_size + 1 ;
        }

      zmapWindowSetScrollableArea(window, NULL, &y1, NULL, &y2,"changeRegion");

    }

  return ;
}

/* Because we can't depend on the canvas having a valid height when it's been realized,
 * we have to detect the invalid height and attach this handler to the canvas's
 * expose_event, such that when it does get called, the height is valid.  Then we
 * disconnect it to prevent it being triggered by any other expose_events.
 *
 * widget is the canvas, user_data is the realizeData structure
 */
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  gboolean result = FALSE ;                                 /* ie allow any other callbacks to run as well */
  RealiseData realiseData = (RealiseDataStruct*)user_data;

  zMapReturnValIfFail(realiseData, result) ;

  zmapWindowBusy(realiseData->window, TRUE) ;

  /* call the function given us by zmapView.c to set zoom_status
   * for all windows in this view. */
  (*(window_cbs_G->setZoomStatus))(realiseData->window, realiseData->window->app_data, NULL);

  /* disconnect signal handler before calling sendClientEvent otherwise when splitting
   * windows, the scroll-to which positions the new window in the same place on the
   * sequence as the previous one, will trigger another call to this function.  */
  g_signal_handler_disconnect(G_OBJECT(widget), realiseData->window->exposeHandlerCB);

  sendClientEvent(realiseData->window, realiseData->feature_sets, NULL) ;

  if(realiseData->window->curr_locking == ZMAP_WINLOCK_VERTICAL)
    {
      int zero = 0;
      panedResizeCB(&zero, (gpointer)(realiseData->window));
    }

  zmapWindowBusy(realiseData->window, FALSE) ;

  realiseData->window->exposeHandlerCB = 0 ;
  realiseData->window = NULL ;
  realiseData->feature_sets = NULL ;
  g_free(realiseData) ;

  return result ;
}



/* This routine sends a synthesized event to tell the ZMapWindow that there are new
 * features to draw. The features are passed in the data element of the event struct. */
static void sendClientEvent(ZMapWindow window, FeatureSetsState feature_sets, gpointer loaded_cb_user_data)
{
  GdkEventClient event ;
  zmapWindowData window_data ;


  /* Set up struct to be passed to our callback. */
  window_data = g_new0(zmapWindowDataStruct, 1) ;
  window_data->window = window ;
  window_data->data = feature_sets ;
  window_data->loaded_cb_user_data = loaded_cb_user_data ;

  event.type         = GDK_CLIENT_EVENT ;
  event.window       = window->toplevel->window ;  /* no window generates this event. */
  event.send_event   = TRUE ;                      /* we sent this event. */
  event.message_type = window->zmap_atom ;         /* This is our id for events. */
  event.data_format  = 8 ;           /* Not sure about data format here... */

  /* Load the pointer value, not what the pointer points to.... */
  {
    void **dummy ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    dummy = (void *)&window_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    dummy = (void **)&window_data ;
    memmove(&(event.data.b[0]), dummy, sizeof(void *)) ;
  }

#if MH17_NOT_NEEDED
  zMapLogWarning("%s", "About to do event sending");
#endif
  gdk_event_put((GdkEvent *)&event);

#if MH17_NOT_NEEDED
  zMapLogWarning("%s", "Got back from sending event");
#endif
  return ;
}


/* Called to service the event sent by sendClientEvent(), this seems contorted but we need
 * to be sure drawing of features occurs as part of the normal stream of events so that we
 * don't draw before the window has been displayed for instance. This routine calls
 * the data display routines of ZMap.
 *
 * This is a general handler that does stuff like handle "click to focus", it gets run
 * _BEFORE_ any canvas item handlers (there seems to be no way with the current
 * foocanvas/gtk to get an event run _after_ the canvas handlers, you cannot for instance
 * just use  g_signal_connect_after(). Therefore you can check on focus/mouse clicks etc
 * for debugging purposes from here.
 *
 */
static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  gboolean event_handled = FALSE ;
  int i ;

  zMapReturnValIfFail(event, event_handled) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
        /* we've had problems with event handling so I'm leaving these here for now in
         * case there is more trouble. */
        gpointer obj = window->canvas ;

        gtk_widget_add_events(GTK_WIDGET(window->toplevel), GDK_POINTER_MOTION_MASK) ;
        g_signal_connect(obj, "button-press-event",
                         pressCB, (gpointer)window) ;
        g_signal_connect(obj, "motion-notify-event",
                         motionCB, (gpointer)window) ;
        g_signal_connect(obj, "button-release-event",
                         releaseCB, (gpointer)window) ;

      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (event->type != GDK_CLIENT_EVENT)
    {
      zMapLogCritical("%s", "received non-GdkEventClient event in dataEventCB() handler") ;
    }
  else if (event->send_event != TRUE)
    {
      zMapLogCritical("%s", "received event not sent by \"client\" in dataEventCB() handler") ;
    }
  else if (event->message_type != gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zMapLogCritical("%s", "unknown client event in dataEventCB() handler") ;
    }
  else
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      FeatureSetsState feature_sets ;
      ZMapFeatureContext diff_context ;
      ZMapFeature highlight_feature ;
      GSignalMatchType signal_match_mask;
      GQuark signal_detail;
      gulong signal_id;


      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;
      window = window_data->window ;
      feature_sets = (FeatureSetsState)window_data->data ;

      zmapWindowBusy(window, TRUE) ;

      highlight_feature = feature_sets->highlight_feature ;

      /* ****Remember that someone needs to free the data passed over....****  */

      /* We need to validate the feature_context at this point, we should be sure it contains
       * some features before continuing. */

      if (feature_sets->new_features)
        diff_context = feature_sets->new_features ;
      else
        diff_context = feature_sets->current_features ;

      /* Draw the features on the canvas */
      zmapWindowDrawFeatures(window, feature_sets->current_features, diff_context, feature_sets->masked) ;

      if (feature_sets->state != NULL)
        {
          zmapWindowStateRestore(feature_sets->state, window);

          feature_sets->state = zmapWindowStateDestroy(feature_sets->state);
        }


      /*
       * Reread the data in any feature list windows we might have.
       * (sm23) The function zmapWindowListWindowReread() does not
       * work; indeed it gives a segmentation fault when executed
       * with a search result window open. I've disabled this for now.
       * Instead, the featureListWindows are destroyed, since they
       * no longer refer to valid foo canvas items.
       */
      if (window->featureListWindows)
        {
          for (i = 0 ; i < window->featureListWindows->len ; i++)
            {
              GtkWidget *widget = NULL ;

              widget = (GtkWidget *)g_ptr_array_index(window->featureListWindows, i) ;

              /* zmapWindowListWindowReread(widget) ;*/
              gtk_widget_destroy(GTK_WIDGET(widget));
            }
          /* destroy the featureListWindow objects (but not the array object itself) */
          g_ptr_array_set_size(window->featureListWindows, 0) ;
        }


      /* I DON'T KNOW WHAT THIS SECTION OF CODE IS SUPPOSED TO DO.... */

      /* adding G_SIGNAL_MATCH_DETAIL to mask results in failure here, despite using the same detail! */
      signal_detail = g_quark_from_string("event");
      signal_match_mask = (GSignalMatchType)(G_SIGNAL_MATCH_FUNC | G_SIGNAL_MATCH_DATA);
      /* We rely on function and data only :( */
      /* We should probably be using signal_id instead! */
      if(g_signal_handler_find(GTK_OBJECT(window->canvas),
                               signal_match_mask, /* mask to select handlers on */
                               0,                 /* signal id. */
                               signal_detail,     /* detail */
                               NULL,              /* closure */
                               (void *)canvasWindowEventCB, /* handler */
                               window             /* user data */
                       ) == 0)
        {
          zMapLogMessage("Signal(%s) handler(%p) for window(%p)->canvas(%p) not registered. registering...",
                         (char *)g_quark_to_string(signal_detail), canvasWindowEventCB,
                         window, window->canvas);

          /* On later versions of the mac I had to add this in to get motion events reported,
           * hopefully this won't mess up other platforms....need to check. */
          gtk_widget_add_events(GTK_WIDGET(window->toplevel), GDK_POINTER_MOTION_MASK) ;
                  gtk_widget_add_events(GTK_WIDGET(window->canvas), GDK_POINTER_MOTION_MASK) ;

          signal_id = g_signal_connect(GTK_OBJECT(window->canvas), g_quark_to_string(signal_detail),
               GTK_SIGNAL_FUNC(canvasWindowEventCB), (gpointer)window) ;
        }
      else
        {
//zMapLogMessage("%s", "event handler for canvas already registered.");
        }

      /* If features were already displayed and one was highlighted then rehighlight it. */
      if (highlight_feature)
        zMapWindowFeatureSelect(window, highlight_feature) ;

      /* Is this the place to do the splice ?? */
      if (feature_sets->splice_highlight_on)
        {
          zmapWindowDrawSplices(window, NULL, 0, 0) ;
        }


      /* Tell layer above that we have finished loading/displaying features. */
      (*(window_cbs_G->drawn_data))(window, window->app_data, window_data->loaded_cb_user_data, diff_context) ;


      g_free(feature_sets) ;
      g_free(window_data) ;    /* Free the WindowData struct. */

      zmapWindowBusy(window, FALSE) ;

      /* Now drawing is complete reset the zoom level. */
      setZoomFromLayoutSize(window, NULL, NULL, NULL, NULL) ;

      event_handled = TRUE ;
    }

  return event_handled ;
}


/* Read window settings from the configuration file, note that we read _only_ the first
 * stanza found in the configuration, subsequent ones are not read. */
static gboolean getConfiguration(ZMapWindow window)
{
  ZMapConfigIniContext context = NULL;
  gboolean result = FALSE ;

  zMapReturnValIfFail(window, result) ;

  /* Set default values */
  window->canvas_maxwin_size = ZMAP_WINDOW_MAX_WINDOW;
  window->keep_empty_cols = FALSE;
  window->display_forward_coords = TRUE;
  window->show_3_frame_reverse = FALSE;

  if ((context = zMapConfigIniContextProvide(window->sequence->config_file, ZMAPCONFIG_FILE_NONE)))
    {
      int tmp_int;
      gboolean tmp_bool;
      char *tmp_str ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                       ZMAPSTANZA_WINDOW_CURSOR, &tmp_str))
        window->normal_cursor = zMapGUICreateCursor(tmp_str) ;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_MAXSIZE, &tmp_int))
        window->canvas_maxwin_size = tmp_int;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_MAXBASES, &tmp_int))
        window->canvas_maxwin_bases = tmp_int;

      if(zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                        ZMAPSTANZA_WINDOW_COLUMNS, &tmp_bool))
        window->keep_empty_cols = tmp_bool;

      if(zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                        ZMAPSTANZA_WINDOW_FWD_COORDS, &tmp_bool))
        window->display_forward_coords = tmp_bool;

      if (zMapConfigIniContextGetBoolean(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                         ZMAPSTANZA_WINDOW_SHOW_3F_REV, &tmp_bool))
        window->show_3_frame_reverse = tmp_bool;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_A_SPACING, &tmp_int))
        window->config.align_spacing = (double)tmp_int;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_B_SPACING, &tmp_int))
        window->config.block_spacing = (double)tmp_int;

#if USE_STRAND
      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_S_SPACING, &tmp_int))
        window->config.strand_spacing = (double)tmp_int;
#endif

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_C_SPACING, &tmp_int))
        window->config.column_spacing = (double)tmp_int;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_F_SPACING, &tmp_int))
        window->config.feature_spacing = (double)tmp_int;

      if(zMapConfigIniContextGetInt(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                    ZMAPSTANZA_WINDOW_LINE_WIDTH, &tmp_int))
        window->config.feature_line_width = (double)tmp_int;

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                                        ZMAPSTANZA_WINDOW_BACKGROUND_COLOUR, &tmp_str))
        gdk_color_parse(tmp_str, &(window->canvas_background)) ;

      zMapConfigIniContextDestroy(context);

      result = TRUE ;
    }

  return result ;
}


static gboolean windowGeneralEventCB(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gboolean handled = FALSE;

  zMapReturnValIfFail(event, handled) ;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        ZMapWindow window = (ZMapWindow)data;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        printf("windowGeneralEventCB (%x): CLICK\n", window);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        (*(window_cbs_G->focus))(window, window->app_data, NULL) ;
      }
      break;
    default:
      handled = FALSE;
      break;
    }
  return handled;
}


/* This gets run _BEFORE_ any of the canvas item handlers which is good because we can use it
 * to handle more general events such as "click to focus" etc., _BUT_ be careful when adding
 * event handlers here, you could override a canvas item event handler and stop something
 * important working (e.g. dna text highlighting...!!) mh17: previous code, DNA now handled here
 *
 *
 * There is some slightly arcane handling of button 1 here:
 *
 * if the item under the cursor is a CanvasFeatureset then we ask it if it wants to
 * operate text selection and if so we pass coordinates through to that
 * NOTE we never handle this in canvasItem EventCB, as we have priority here
 * DNA event callbacks disappeared with ZMapWindowCanvasItems as groups
 *
 * If the user presses button 1 anywhere else we assume they are going to lasso an area for
 * zooming and we track mouse movements, if they have moved far enough then we do the zoom
 * on button release.
 *
 * If the user does lassoing, ruler or moving the mark then we return TRUE to say we have
 * handled the event, otherwise we pass the event on (so canvas items can receive events).
 *
 * NOTE...WITH THE INTRODUCTION OF MARK CODE HERE THIS FUNCTION IS NOW FAR TOO LONG,
 * SOME SUBROUTINES ARE NEEDED.....
 *
 */
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;    /* FALSE means other handlers run. */
  ZMapWindow window = (ZMapWindow)data ;
  static double origin_x, origin_y;    /* The world coords of the source of the button event */
  static gboolean dragging = FALSE, guide = FALSE ;    /* Rubber banding or ruler ? */
  static gboolean shift_on = FALSE ;    /* shift/rubber banding == feature selection. */
  static gboolean locked = FALSE ;    /* For ruler, are there locked windows ? */
  static gboolean in_window = FALSE ;    /* Still in window while cursor lining
       or rubber banding ? */
  double wx, wy;    /* These hold the current world coords of the event */
  static double window_x, window_y ;    /* Track number of pixels user moves mouse. */
  static MarkRegionUpdateStruct mark_updater = {0} ;
  static FooCanvasItem *seq_item = NULL;    /* if not NULL we are selecting text */
  static long seq_start, seq_end;    /* first coord is fixed, then we can move above or below */
  GdkEventButton *but_event ;
  GdkModifierType shift_mask = GDK_SHIFT_MASK,
    control_mask = GDK_CONTROL_MASK,
    alt_mask = GDK_MOD1_MASK,
    meta_mask = GDK_META_MASK,
    unwanted_masks = (GdkModifierType)(GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK
                                       | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK
                                       | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK),
    locks_mask ;

  zMapReturnValIfFail(event, event_handled) ;



#if MOUSE_DEBUG
  zMapLogWarning("canvas event %d",  event->type);
#endif

  /* We record whether we are inside the window to enable user to cancel certain mouse related
   * actions by moving outside the window. */
  if (event->type == GDK_ENTER_NOTIFY)
    {
      in_window = TRUE ;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      in_window = FALSE ;
    }
  else if (event->type == GDK_FOCUS_CHANGE)
    {
      /* We can get here if we left-click and then right-click before releasing the left, and we
       * need to cancel rubber banding or it will continue with normal mouse motion (i.e. without
       * the mouse button being held) and the next left-click will do the zoom, probably zooming to 
       * an unexpected region. */
      dragging = FALSE ;
      if (window->rubberband)
        {
          gtk_object_destroy(GTK_OBJECT(window->rubberband)) ;
          window->rubberband = NULL ;
        }
    }
  else if (event->type == GDK_SCROLL)
    {
      /* Cancel dragging if scrolling has started or we can end up zooming to an unexpected region */
      dragging = FALSE ;
      if (window->rubberband)
        {
          gtk_object_destroy(GTK_OBJECT(window->rubberband)) ;
          window->rubberband = NULL ;
        }
    }
  else if (event->type == GDK_BUTTON_PRESS)
    {
      /* This hack is really only needed for the mac.... */
      in_window = TRUE ;
    }

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_MOTION_NOTIFY || event->type ==  GDK_BUTTON_RELEASE)
    {
      but_event = (GdkEventButton *)event ;


      /* In order to make the modifier only checks work we need to OR in the unwanted masks that might be on.
       * This includes the shift lock and num lock. Depending on the setup of X these might be mapped
       * to other things which is why MODs 2-5 are included This in theory should include the new (since 2.10)
       * GDK_SUPER_MASK, GDK_HYPER_MASK and GDK_META_MASK */
      if ((locks_mask = (GdkModifierType)(but_event->state & unwanted_masks)))
        {
          shift_mask = (GdkModifierType)(shift_mask | locks_mask) ;
          control_mask = (GdkModifierType)(control_mask | locks_mask) ;
          alt_mask = (GdkModifierType)(alt_mask | locks_mask) ;
          meta_mask = (GdkModifierType)(meta_mask | locks_mask) ;
        }
    }



  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        FooCanvasItem *item ;

        zMapDebugPrint(mouse_debug_G, "Start: button_press %d", but_event->button) ;

        /* work out the world of where we are */
        foo_canvas_window_to_world(window->canvas,
                                   but_event->x, but_event->y,
                                   &wx, &wy);

        if(!gtk_widget_has_focus (GTK_WIDGET(window->canvas)))/* avoid thrashing the scroll region and navigator */
          {
            /* We want the canvas to be the focus widget of its "window" otherwise keyboard input
             * (i.e. short cuts) will be delivered to some other widget. */
            gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;

            /* Report to our parent that we have been clicked in, we change "click" to "focus"
             * because that is what this is for.... */
            /* NOTE this goes up to zmapControl.c and shows the navigator for the clicked on view */
            (*(window_cbs_G->focus))(window, window->app_data, NULL) ;

            if (TRUE)
              {
                /* NOTE this set the locator according to the selected window and its scroll region */
                invokeVisibilityChange(window);
              }
          }

        /* Button 1 is for selecting features OR lasso for zoom/mark.
         * Button 2 is handled, we display ruler and maybe centre on that position,
         * Button 3 is used only with modifiers are passed on as they may be
         *          clicks on canvas items/columns. */
        switch (but_event->button)
          {
          case 1:
            {
              GdkModifierType shift_mask = GDK_SHIFT_MASK ;
              /* refer to handleButton() in zmapWindowfeature.c re shift/num lock */

              shift_on = zMapGUITestModifiers(but_event, shift_mask) ;

              origin_x = wx;
              origin_y = wy;

              if ((item = foo_canvas_get_item_at(window->canvas, origin_x, origin_y))
                          && ZMAP_IS_WINDOW_FEATURESET_ITEM(item)
                          && zMapWindowCanvasFeaturesetGetSeqCoord((ZMapWindowFeaturesetItem)item, TRUE,
                           origin_x, origin_y, &seq_start, &seq_end))
                {
                  /* get start coordinate via subpartspan from x and y
                   * set end coordinate to be the same
                   * set a flag to say selecting text and remember the canvas item
                   * highlight the region
                   */

                  //if(zMapWindowCanvasFeaturesetGetSeqCoord((ZMapWindowFeaturesetItem) item, TRUE, origin_x, origin_y, &seq_start, &seq_end))
                  {
                    seq_item = item;
                    /*
                     * Although we start with a base Foo item the highlight code does a FToI lookup on the feature
                     * to get the full ZMapCanvasItem which is used to drive SeqDispSelByRegion()
                     */
                    zmapWindowHighlightSequenceItem(window, seq_item, seq_start, seq_end, 0);

                    /* NOTE we set feature list to NULL here, update info panel must handle */
                    zmapWindowUpdateInfoPanel(window, zMapWindowCanvasItemGetFeature(seq_item), NULL, seq_item,
                                              NULL, seq_start, seq_end, seq_start, seq_end,
                                              NULL, FALSE, FALSE, FALSE, NULL) ;

                    event_handled = TRUE;
                  }
                }
              else
                {
                  if (dragging || guide)
                    {

                      /* It's possible to press another mouse button while holding the middle
                       * one down especially if it's a standard PC mouse with the wheel in the middle
                       * instead of a proper button. */
                      event_handled = TRUE ;
                    }
                  else
                    {
                      /* Pucka button press that we need to handle. */

                      /* Record where are we in the window at the start of mouse/button movement. */
                      window_x = but_event->x ;
                      window_y = but_event->y ;

                      if (mark_updater.in_mark_move_region)
                        {
                          mark_updater.activated = TRUE;
                          setupRuler(window, &(window->mark_guide_line), NULL, wx, wy);
                        }
                      else
                        {
                          /* Show a rubber band for zooming/marking. */
                          dragging = TRUE;

                          if (!window->rubberband)
                            window->rubberband = zMapDrawRubberbandCreate(window->canvas, &window->colour_rubber_band);
                        }

                      /* At this stage we don't know if we are rubber banding etc. so pass the
                       * press on. */
                      event_handled = FALSE ;
                    }
                }
              break ;
            }
          case 2:
            {
              if (dragging || guide)
                {
                  /* It's possible to press another mouse button while holding the middle
                   * one down especially if it's a standard PC mouse with the wheel in the middle
                   * instead of a proper button then you try to do two things at once. */
                  event_handled = TRUE ;
                }
              else if (zMapGUITestModifiers(but_event, alt_mask)
                       ||zMapGUITestModifiers(but_event, meta_mask))
                {
                  event_handled = TRUE ;
                }
              else
                {

                  /* Show a ruler and our exact position. */
                  guide = TRUE ;

                  /* always clear this if set. */
                  if(mark_updater.in_mark_move_region)
                    {
                      mark_updater.in_mark_move_region = FALSE;
                      mark_updater.closest_to = NULL;

                      gdk_window_set_cursor(GTK_WIDGET(window->canvas)->window, window->normal_cursor) ;
                      gdk_cursor_unref(mark_updater.arrow_cursor);
                      mark_updater.arrow_cursor = NULL;
                    }

                  /* If there are locked, _vertical_ split windows then also show the ruler in all
                   * of them. */
                  if (window->locked_display && window->curr_locking == ZMAP_WINLOCK_VERTICAL)
                    {
                      LockedRulerStruct locked_data = {(ZMapWindowLockRulerActionType)0} ;

                      locked_data.action   = ZMAP_LOCKED_RULER_SETUP ;
                      locked_data.world_x  = wx ;
                      locked_data.world_y  = wy ;
                      locked_data.origin_y = origin_y ;

                      g_hash_table_foreach(window->sibling_locked_windows, lockedRulerCB, (gpointer)&locked_data) ;

                      locked = TRUE ;
                    }
                  else
                    {
                      setupRuler(window, &(window->horizon_guide_line), &(window->tooltip), wx, wy) ;
                    }

                  event_handled = TRUE ;    /* We _ARE_ handling */
                }

              break ;
            }
          case 3:
            {
              if (dragging || guide)
                {
                  /* It's possible to press another mouse button while holding the middle
                   * one down especially if it's a standard PC mouse with the wheel in the middle
                   * instead of a proper button then you try to do two things at once. */
                  event_handled = TRUE ;
                }
              else
                {
                  /* Nothing to do, menu callbacks are set on canvas items, not here. */

                  if (mark_updater.in_mark_move_region)
                    {
                      mark_updater.in_mark_move_region = FALSE;
                      mark_updater.closest_to = NULL;

                      gdk_window_set_cursor(GTK_WIDGET(window->canvas)->window, window->normal_cursor) ;
                      gdk_cursor_unref(mark_updater.arrow_cursor);
                      mark_updater.arrow_cursor = NULL;

                      event_handled = TRUE;             /* We _ARE_ handling */
                    }
                }

              break ;
            }
          default:
            {
              /* We don't do anything for any buttons > 3. */
              break ;
            }
          }


        zMapDebugPrint(mouse_debug_G, "Leave: button_press %d - return %s",
                       but_event->button,
                       event_handled ? "TRUE" : "FALSE") ;

        break ;
      }
    case GDK_2BUTTON_PRESS:
      {
        /* We don't currently do anything with double clicks but the debug info. is useful. */
        GdkEventButton *but_event = (GdkEventButton *)event ;

        event_handled = FALSE ;

        zMapDebugPrint(mouse_debug_G, "Start: button_2press %d", but_event->button) ;

        zMapDebugPrint(mouse_debug_G, "Leave: button_2press %d - return %s",
                       but_event->button,
                       event_handled ? "TRUE" : "FALSE") ;
        break ;
      }
    case GDK_MOTION_NOTIFY:
      {
        /* interestingly we don't check the button number here.... */
        GdkEventMotion *mot_event = (GdkEventMotion *)event ;

        /* work out the world of where we are */
        foo_canvas_window_to_world(window->canvas,
                                   mot_event->x, mot_event->y,
                                   &wx, &wy);

        zMapDebugPrint(mouse_debug_G, "%s", "Start: motion") ;


        if (but_event->button == 2
            && (zMapGUITestModifiers(but_event, alt_mask) || zMapGUITestModifiers(but_event, meta_mask)))
          {
            event_handled = TRUE ;
          }
        else if(seq_item)
          {
            zMapWindowCanvasFeaturesetGetSeqCoord((ZMapWindowFeaturesetItem) seq_item, FALSE,  wx, wy, &seq_start, &seq_end);

            zmapWindowHighlightSequenceItem(window, seq_item, seq_start, seq_end, 0);

            /* NOTE we set feature list to NULL here, update info panel must handle */
            zmapWindowUpdateInfoPanel(window, zMapWindowCanvasItemGetFeature(seq_item),
                                      NULL, seq_item, NULL, seq_start, seq_end, seq_start, seq_end,
                                      NULL,  FALSE, FALSE, FALSE, NULL) ;
          }
        else if ((dragging && window->rubberband) || guide)
          {
            if (dragging && window->rubberband)
              {
                /* I wanted to change the cursor for this,
                 * but foo_canvas_item_grab/ungrab specifically the ungrab didn't work */
                zMapDrawRubberbandResize(window->rubberband, origin_x, origin_y, wx, wy);
                /* zMapWindowRedraw(window) ;*/  /* sm23 */

                /* gb10: this is a bit messy but I'm not sure how to get around it. We need to
                 * pass this event on to canvasItemEventCB to see if we should initiate drag and
                 * drop. But we don't know at this point whether we should be doing rubberbanding
                 * or not. It works ok to start the rubberbanding but to return false here to let
                 * drag-and-drop start, which then stops us getting more motion notify events and
                 * therefore stops the rubberbanding. */
                event_handled = FALSE;
              }
            else if (guide)
              {
                double y1, y2 ;
                char *tip = NULL ;

                foo_canvas_get_scroll_region(window->canvas, /* ok, but like to change */
                                             NULL, &y1, NULL, &y2);

                zmapWindowClampedAtStartEnd(window, &y1, &y2);

                /* We floor the value here as it works with the way we draw our bases. */
                /* This test is FLAWED ATM it needs to test for displayed seq start & end */
                if (y1 <= wy && y2 >= wy)
                  {
                    tip = tooltipTextCreate(window, wx, wy) ;
                  }


                /* If we are a locked, _vertical_ split window then also show the ruler in the
                 * other windows. */
                if (locked)
                  {
                    LockedRulerStruct locked_data = {(ZMapWindowLockRulerActionType)0} ;

                    locked_data.action   = ZMAP_LOCKED_RULER_MOVING ;
                    locked_data.world_x  = wx ;
                    locked_data.world_y  = wy ;
                    locked_data.tip_text = tip ;

                    g_hash_table_foreach(window->sibling_locked_windows, lockedRulerCB, (gpointer)&locked_data) ;
                  }
                else
                  {
                    moveRuler(window->horizon_guide_line, window->tooltip, tip, wx, wy) ;
                    /*zMapWindowRedraw(window) ; */ /* sm23 */
                  }

                if (tip)
                  g_free(tip);

                event_handled = TRUE ;    /* We _ARE_ handling */
              }
            else
              {
                event_handled = FALSE ;
              }

            zMapDebugPrint(mouse_debug_G, "%s", "End: motion") ;
          }
        else if ((!mark_updater.in_mark_move_region) && zmapWindowMarkIsSet(window->mark))
          {
            double world_dy;
            int canvas_dy = 5;

            zmapWindowMarkGetWorldRange(window->mark,
                                        &(mark_updater.mark_x1),
                                        &(mark_updater.mark_y1),
                                        &(mark_updater.mark_x2),
                                        &(mark_updater.mark_y2));

            world_dy = canvas_dy / window->canvas->pixels_per_unit_y;

            if((wy > mark_updater.mark_y1 - world_dy) && (wy < mark_updater.mark_y1 + world_dy))
              {
                mark_updater.in_mark_move_region = TRUE;
                mark_updater.closest_to = &(mark_updater.mark_y1);
                      }
            else if((wy > mark_updater.mark_y2 - world_dy) && (wy < mark_updater.mark_y2 + world_dy))
              {
                mark_updater.in_mark_move_region = TRUE;
                mark_updater.closest_to = &(mark_updater.mark_y2);
              }

            if(mark_updater.in_mark_move_region)
              {
                mark_updater.arrow_cursor = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
                gdk_window_set_cursor(GTK_WIDGET(window->canvas)->window, mark_updater.arrow_cursor);
                event_handled = TRUE;
              }
            else
              {
                event_handled = FALSE;
              }
          }
        else if(mark_updater.in_mark_move_region && mark_updater.activated)
          {
            /* Now we can move... But must return TRUE. */

            foo_canvas_window_to_world(window->canvas,
                                       mot_event->x, mot_event->y,
                                       &wx, mark_updater.closest_to);
            wy = *(mark_updater.closest_to);
            moveRuler(window->mark_guide_line, NULL, NULL, wx, wy);
            /* zMapWindowRedraw(window) ; */ /* sm23 */

            event_handled = TRUE;
          }
        else if (mark_updater.in_mark_move_region && (!mark_updater.activated)
                         && zmapWindowMarkIsSet(window->mark))
          {
            double world_dy;
            int canvas_dy = 5;

            zmapWindowMarkGetWorldRange(window->mark,
                                        &(mark_updater.mark_x1),
                                        &(mark_updater.mark_y1),
                                        &(mark_updater.mark_x2),
                                        &(mark_updater.mark_y2));

            world_dy = canvas_dy / window->canvas->pixels_per_unit_y;

            if ((!((wy > mark_updater.mark_y1 - world_dy) && (wy < mark_updater.mark_y1 + world_dy)))
                        && (!((wy > mark_updater.mark_y2 - world_dy) && (wy < mark_updater.mark_y2 + world_dy))))
              {
                mark_updater.in_mark_move_region = FALSE;
                mark_updater.closest_to = NULL;

                gdk_window_set_cursor(GTK_WIDGET(window->canvas)->window, window->normal_cursor) ;
                gdk_cursor_unref(mark_updater.arrow_cursor);
                mark_updater.arrow_cursor = NULL;

                event_handled = TRUE;
              }
            else
              {
                event_handled = FALSE;
              }
          }

        break;
      }

    case GDK_BUTTON_RELEASE:
      {
        GdkEventButton *but_event = (GdkEventButton *)event ;

        /* work out the world of where we are */
        foo_canvas_window_to_world(window->canvas,
                                   but_event->x, but_event->y,
                                   &wx, &wy);
        /* interestingly we don't check the button number here.... */

        zMapDebugPrint(mouse_debug_G, "Start: button_release %d", but_event->button) ;

        if (but_event->button == 2
            && (zMapGUITestModifiers(but_event, alt_mask) || zMapGUITestModifiers(but_event, meta_mask)))
          {
            /* We need the clipboard contents but.....which one ?
             *
             * Cntl-C seems always to go to GDK_SELECTION_CLIPBOARD
             *
             * cursor selecting with mouse seems to go to GDK_SELECTION_PRIMARY
             *  */
            double but_wx, but_wy ;
            gboolean result ;

            foo_canvas_window_to_world(window->canvas,
                                       but_event->x, but_event->y,
                                       &but_wx, &but_wy) ;

            result = zmapWindowZoomFromClipboard(window, but_wx, but_wy) ;

            event_handled = TRUE ;
          }
        else if (seq_item)
          {
            zMapWindowCanvasFeaturesetGetSeqCoord((ZMapWindowFeaturesetItem)seq_item, FALSE,
                                                  wx, wy, &seq_start, &seq_end);

            zmapWindowHighlightSequenceItem(window, seq_item, seq_start, seq_end, 0);

            /* NOTE we set feature list to NULL here, update info panel must handle */
            zmapWindowUpdateInfoPanel(window, zMapWindowCanvasItemGetFeature(seq_item),
                                      NULL, seq_item, NULL, seq_start, seq_end, seq_start, seq_end,
                                      NULL, FALSE, FALSE, FALSE, NULL) ;

            seq_item = NULL;
            event_handled = TRUE;    /* We _ARE_ handling */
          }
        else if (dragging)
          {
            if (window->rubberband)
              {
                foo_canvas_item_hide(window->rubberband);

                /* mouse must still be within window to zoom, outside means user is cancelling motion,
                 * and motion must be more than 10 pixels in either direction to zoom. */
                if (in_window)
                  {
                    if (fabs(but_event->x - window_x) > ZMAP_WINDOW_MIN_LASSO
                        || fabs(but_event->y - window_y) > ZMAP_WINDOW_MIN_LASSO)
                      {
                        /* User has moved pointer quite a lot between press and release so do our
                         * stuff. */

                        if (!shift_on)
                          {
                            zoomToRubberBandArea(window) ;

                            event_handled = TRUE;    /* We _ARE_ handling */
                          }
                        else
                          {
                            /* make a list of the foo canvas items */
                            GList *feature_list;
                            FooCanvasItem *item;
                            double rootx1, rootx2, rooty1, rooty2 ;
                            ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED,
                                                                          ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                                          ZMAPWINDOW_PASTE_TYPE_SELECTED} ;


                            /* Get size of item and convert to world coords, bounds are relative to item's parent */
                            foo_canvas_item_get_bounds(window->rubberband, &rootx1, &rooty1, &rootx2, &rooty2) ;
                            //    foo_canvas_item_i2w(window->rubberband, &rootx1, &rooty1) ;
                            //    foo_canvas_item_i2w(window->rubberband, &rootx2, &rooty2) ;

                            /* find the canvvas iten (a canvas featureset) at the centre of the lassoo */
                            // can-t do this as the canvas featureset point function returns n/a
                            // if there's no feature under the cursor no longer true....
                            if ((item = foo_canvas_get_item_at(window->canvas,
                                                               (rootx2 + rootx1) / 2, (rooty2 + rooty1) / 2)))
                              {
                                /* only finds features in a canvas featureset, old foo gives nothing */
                                feature_list = zMapWindowFeaturesetFindItemAndFeatures(&item,
                                                                                       rooty1, rooty2,
                                                                                       rootx1, rootx2) ;
                              }


                            /* this is how features get highlit */
                            if (item)
                              zmapWindowUpdateInfoPanel(window, zMapWindowCanvasItemGetFeature(item),
                                                        feature_list, item, NULL, 0, 0, 0, 0, NULL,
                                                        !shift_on, FALSE, FALSE, &display_style) ;

                            event_handled = TRUE;    /* We _ARE_ handling */
                          }
                      }
                    else
                      {
                        /* User hasn't really moved which means they meant to select a feature, not
                         * lasso an area so pass event on and destroy rubberband object. */

                        /* Must get rid of rubberband item as it is not needed. */
                        // mh17: was this a memory leak from the other bits of the if ???
                        //    gtk_object_destroy(GTK_OBJECT(window->rubberband)) ;origin_
                        //    window->rubberband = NULL ;

                        event_handled = FALSE ;
                      }
                  }

                /* Must get rid of rubberband item as it is not needed. */
                gtk_object_destroy(GTK_OBJECT(window->rubberband)) ;
                window->rubberband = NULL ;
              }
            else
              {
                event_handled = FALSE ;
              }

            dragging = FALSE ;
          }
        else if (guide)
          {
            /* If we are a locked, _vertical_ split window then also show the ruler in the
             * other windows. */
            if (locked)
              {
                LockedRulerStruct locked_data = {(ZMapWindowLockRulerActionType)0} ;

                locked_data.action  = ZMAP_LOCKED_RULER_REMOVE ;

                g_hash_table_foreach(window->sibling_locked_windows, lockedRulerCB, (gpointer)&locked_data) ;
              }
            else
              {
                removeRuler(window->horizon_guide_line, window->tooltip) ;
              }

            /* If windows are locked then this should scroll all of them.... */
            if (in_window)
              {
                double y;
                gboolean moved = FALSE;
                y = but_event->y;

                moved = recenter_scroll_window(window, &y);

                zMapWindowScrollToWindowPos(window, y) ;

                if(moved)
                  zMapWindowRedraw(window); /* this does zmapWindowUninterruptExpose()! */
              }

            guide = FALSE ;
            locked = FALSE ;

            event_handled = TRUE ;    /* We _ARE_ handling */
          }
        else if (mark_updater.activated)
          {
            mark_updater.activated = FALSE;
            mark_updater.in_mark_move_region = FALSE;

            removeRuler(window->mark_guide_line, NULL);

            if (in_window && mark_updater.mark_y1 < mark_updater.mark_y2)
              {
                zmapWindowClampedAtStartEnd(window, &(mark_updater.mark_y1), &(mark_updater.mark_y2));
                zMapWindowStateRecord(window);
                zmapWindowMarkSetWorldRange(window->mark,
                                            mark_updater.mark_x1,
                                            mark_updater.mark_y1,
                                            mark_updater.mark_x2,
                                            mark_updater.mark_y2);
              }

            gdk_window_set_cursor(GTK_WIDGET(window->canvas)->window, window->normal_cursor) ;
            gdk_cursor_unref(mark_updater.arrow_cursor);
            mark_updater.arrow_cursor = NULL;
            mark_updater.closest_to = NULL;

            event_handled = TRUE;
          }

        shift_on = FALSE ;


        zMapDebugPrint(mouse_debug_G, "Leave: button_release %d - return %s",
                       but_event->button,
                       event_handled ? "TRUE" : "FALSE") ;

        if (event_handled)
          {
            /* If we have handled the event, we must make sure the dragging flags are reset in
             * the feature handler (because a press usually causes the feature handler to be
             * called but a release sometimes doesn't) */
            zmapWindowFeatureItemEventButRelease(event) ;
          }

        break;
      }

    case GDK_KEY_PRESS:
      {
        GdkEventKey *key_event = (GdkEventKey *)event ;

        event_handled = keyboardEvent(window, key_event) ;

        break ;
      }

    default:
      {
        event_handled = FALSE ;

        break ;
      }

    }


  if (mouse_debug_G)
    fflush(stdout) ;


  return event_handled ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/*
 * A trio of event handlers, they don't interfer with other handlers, just
 * report that they were called...helpful for debugging. Just undef in the
 * places they appear in this file and set break points.
 */

static gboolean pressCB(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)user_data ;
  static double origin_x, origin_y;    /* The world coords of the source of
       the button 1 event */

  zMapDebugPrint(mouse_debug_G, "%s",  "in press") ;

  foo_canvas_window_to_world(window->canvas,
     event->x, event->y,
     &origin_x, &origin_y);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Don't handle if its text because the text item callbacks handle lasso'ing of
   * text. */
  {
    FooCanvasItem *item ;

    if ((item = foo_canvas_get_item_at(window->canvas, origin_x, origin_y))
                  && ZMAP_IS_WINDOW_TEXT_ITEM(item))
      return FALSE ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return event_handled ;
}

static gboolean motionCB(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
  gboolean event_handled = FALSE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapDebugPrint(mouse_debug_G, "%s",  "in motion") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (event->state & GDK_BUTTON1_MASK)
    zMapDebugPrint(mouse_debug_G, "%s",  "in motion with button press") ;


  return event_handled ;
}

static gboolean releaseCB(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gboolean event_handled = FALSE ;

  zMapDebugPrint(mouse_debug_G, "%s",  "in release") ;


  return event_handled ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Called when the destination of a drag from the grid calls us back to request the data.
 * Fills in seletionData with data of the type indicated by info. */
static void dragDataGetCB(GtkWidget *widget,
                          GdkDragContext *drag_context,
                          GtkSelectionData *selectionData,
                          guint info,
                          guint time,
                          gpointer user_data)
{
  zMapReturnIfFail(selectionData && user_data) ;

  if (info == TARGET_STRING)
    {
      /* Get all of the focused items and send the features as gff */
      ZMapWindow window = (ZMapWindow)user_data ;
      GList *feature_list = NULL ;
      GError *tmp_error = NULL ;
      GString *result = g_string_new(NULL) ;

      if (window && window->focus)
        feature_list = zmapWindowFocusGetFeatureList(window->focus) ;

      if (feature_list)
        {
          /* Swop to other strand..... */
          if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
            zMapFeatureContextReverseComplement(window->feature_context) ;

          zMapGFFDumpList(feature_list, window->context_map->styles, NULL, NULL, result, &tmp_error) ;

          /* And swop it back again. */
          if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
            zMapFeatureContextReverseComplement(window->feature_context) ;
        }

      if (result)
        {
          gtk_selection_data_set_text(selectionData, result->str, -1);
          g_string_free(result, TRUE) ;
        }
    }
}




static void zoomToRubberBandArea(ZMapWindow window)
{

  zMapReturnIfFail(window) ;

  if (window->rubberband)
    {
      double rootx1=0.0, rootx2=0.0, rooty1=0.0, rooty2=0.0 ;
      gboolean border = FALSE ;

      /* Get size of item and convert to world coords. */
      foo_canvas_item_get_bounds(window->rubberband, &rootx1, &rooty1, &rootx2, &rooty2) ;
      foo_canvas_item_i2w(window->rubberband, &rootx1, &rooty1) ;
      foo_canvas_item_i2w(window->rubberband, &rootx2, &rooty2) ;

      if (rooty1 <  window->min_coord || rooty2 > window->max_coord)
        border = TRUE ;

      zmapWindowZoomToWorldPosition(window, border, rootx1, rooty1, rootx2, rooty2) ;
    }

  return ;
}



/* Zoom to a single item. */
void zmapWindowZoomToItem(ZMapWindow window, FooCanvasItem *item)
{
  double rootx1=0.0, rootx2=0.0, rooty1=0.0, rooty2=0.0;
  gboolean border = TRUE ;

  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
  {
          /* feature has been set by caller */
          zMapWindowCanvasFeaturesetGetFeatureBounds(item, &rootx1, &rooty1, &rootx2, &rooty2);
  }
  else
  {
          /* Get size of item and convert to world coords. */
        foo_canvas_item_get_bounds(item, &rootx1, &rooty1, &rootx2, &rooty2) ;
        foo_canvas_item_i2w(item, &rootx1, &rooty1) ;
        foo_canvas_item_i2w(item, &rootx2, &rooty2) ;
  }

  zmapWindowZoomToWorldPosition(window, border, rootx1, rooty1, rootx2, rooty2) ;

  return ;
}


/* Get max bounds of a set of items.... */
void zmapWindowGetMaxBoundsItems(ZMapWindow window, GList *items,
                                 double *rootx1, double *rooty1, double *rootx2, double *rooty2)
{

  MaxBoundsStruct max_bounds = {0.0} ;
  if (!rootx1 || !rootx2 || !rooty1 || !rooty2)
    return ;

  g_list_foreach(items, getMaxBounds, &max_bounds) ;

  *rootx1 = max_bounds.rootx1 ;
  *rooty1 = max_bounds.rooty1 ;
  *rootx2 = max_bounds.rootx2 ;
  *rooty2 = max_bounds.rooty2 ;

  return ;
}


/* Zoom to the max extent of a set of items. */
void zmapWindowZoomToItems(ZMapWindow window, GList *items)
{
  MaxBoundsStruct max_bounds = {0.0} ;
  gboolean border = TRUE ;

  g_list_foreach(items, getMaxBounds, &max_bounds) ;

  zmapWindowZoomToWorldPosition(window, border, max_bounds.rootx1, max_bounds.rooty1,
                                max_bounds.rootx2, max_bounds.rooty2) ;

  return ;
}



/* MH17:
 * the original code here causes at least 4 expose events for the zoom
 * and has been refactored to reduce this (NOTE the foo canvas also causes a double expose
 * dur to expose callung update which mey indirectly queue another expose
 * we still call myWindwoZoom and myWindowMove but wrap them in foo_canvay_busy() calls
 * these two functions must not call foo_canvasbusy() themeselves but thier callers may
 */

#if 0
/* original code */
void zmapWindowZoomToWorldPosition(ZMapWindow window, gboolean border,
   double rootx1, double rooty1, double rootx2, double rooty2)
{
  GtkAdjustment *v_adjuster ;
  /* size of bound area */
  double ydiff;
  double area_middle;
  int win_height, canvasx, canvasy, beforex, beforey;
  /* Zoom factor */
  double zoom_by_factor, target_zoom_factor, current;
  double wx1, wx2, wy1, wy2;
  int two_times, border_size = ZMAP_WINDOW_FEATURE_ZOOM_BORDER;

  v_adjuster = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
  win_height = v_adjuster->page_size;

  /* If we want a border add it...
   * Actually take it away from the current height of the canvas, this
   * way that pixel size will automatically be ratioed by the zoom.
   */
  if(border && ((two_times = (border_size * 2)) < win_height))
    win_height -= two_times;

  current = zMapWindowGetZoomFactor(window) ;

  /* make them canvas so we can scroll there */
  foo_canvas_w2c(window->canvas, rootx1, rooty1, &beforex, &beforey);

  /* work out the zoom factor to show all the area (vertically) and
     calculate how much we need to zoom by to achieve that. */
  ydiff = rooty2 - rooty1;

  if (ydiff >= ZOOM_SENSITIVITY)
    {
      area_middle = rooty1 + (ydiff / 2.0) ; /* So we can make this the centre later */

      target_zoom_factor = (double)(win_height / ydiff);

      zoom_by_factor = (target_zoom_factor / current);

      /* actually do the zoom, but we'll organise for the scroll_to'ing! */
      zmapWindowZoom(window, zoom_by_factor, FALSE);

      /* Now we need to find where the original top of the area is in
       * canvas coords after the effect of the zoom. Hence the w2c calls
       * below.
       * And scroll there:
       * We use this rather than zMapWindowScrollTo as we may have zoomed
       * in so far that we can't just sroll the current canvas buffer to
       * where we clicked. We actually need to check we haven't zoomed off.
       * If we have then we need to move there first, otherwise scroll_to
       * doesn't do anything.
       */
      foo_canvas_get_scroll_region(window->canvas, &wx1, &wy1, &wx2, &wy2); /* ok, but can we refactor? */
      if (rooty1 > wy1 && rooty2 < wy2)
{                           /* We're still in the same area, */
  foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);

          /* If we had a border we need to take this into account,
           * otherwise the feature just ends up at the top of the
           * window with 2 border widths at the bottom! */
          if(border)
            canvasy -= border_size;

  if(beforey != canvasy)
    foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
}
      else
{                           /* This takes a lot of time.... */
  double half_win_span = ((window->canvas_maxwin_size / target_zoom_factor) / 2.0);
  double min_seq = area_middle - half_win_span;
  double max_seq = area_middle + half_win_span - 1;

  /* unfortunately freeze/thaw child-notify doesn't stop flicker */
  /* can we do something else to make it busy?? */
  zMapWindowMove(window, min_seq, max_seq);

  foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);

          /* Do the right thing with the border again. */
          if(border)
            canvasy -= border_size;

          /* No need to worry about border here as we're using the centre of the window. */
  foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
}
    }

  return ;
}

#endif

/*
previous code was structured to zoom each windonw the scroll each window
to avoid multiple expose events we have to (zoom & scroll) each window
and instead of zooming to the current position we zoom the middle of the seq region specified
*/


static void myWindowZoomTo(ZMapWindow window, double zoom_factor, double start, double end, gboolean border, int border_size)
{
  double curr_pos = (start + end) / 2;
//static void myWindowZoom(ZMapWindow window, double zoom_factor, double curr_pos)
//{
  GtkAdjustment *adjust;
  int x, y ;
  double x1, y1, x2, y2, width ;
  double new_canvas_span ;

  zMapReturnIfFail(window) ;

//printf("myWindowZoomTo 1\n");

  if(window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
    {
      adjust =
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
      foo_canvas_get_scroll_offsets(window->canvas, &x, &y) ;
      y += adjust->page_size / 2 ;
      foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;
    }

  zMapWindowStateRecord(window);


  /* We don't zoom in further than is necessary to show individual bases or further out than
   * is necessary to show the whole sequence, or if the sequence is so short that it can be
   * shown at max. zoom in its entirety. */

  if (zmapWindowZoomControlZoomByFactor(window, zoom_factor))
    {
      double half_new_span;

      x1 = y1 = x2 = y2 = 0.0;

      /* Calculate the extent of the new span, new span must not exceed maximum X window size
       * but we must display as much of the sequence as we can for zooming out. */
      zmapWindowGetScrollableArea(window, &x1, &y1, &x2, &y2);
      new_canvas_span = zmapWindowZoomControlLimitSpan(window, y1, y2);/* try out the new zoom window.... */
      half_new_span   = (new_canvas_span / 2);

      y1 = (curr_pos - half_new_span);
      y2 = (curr_pos + half_new_span);

      zmapWindowClampSpan(window, &y1, &y2);



      /* Set the new scroll_region and the new zoom, keep these together to try
       * and avoid a double redraw as both these operations cause update requests
       * and if separated by other gtk calls will result in double draws.
       * If this doesn't work we'll need to provide a new single foocanvas call
       * to set both zoom and scroll region in one go. */
/* MH17: they cause expose events internally so we use foo_canvas_busy() around this function */
      zmapWindowSetPixelxy(window, 1.0, zMapWindowGetZoomFactor(window)) ;

      /* must set zoom before SetScroll as that set Scroll may redraw the ruler */
      zmapWindowScaleCanvasSetPixelsPerUnit(window->ruler, 1.0, zMapWindowGetZoomFactor(window));

      zmapWindowSetScrollableArea(window, &x1, &y1, &x2, &y2,"myWindowZoom");


      /* Now we've actually done the zoom on the canvas we can
       * redraw/show/hide everything that's zoom dependent.
       * E.G. Text, which is separated differently @ different zooms otherwise.
       *      zoom dependent features, magnification dependent columns
       */

      /* Firstly the scale bar, which will be changed soon. */
      zmapWindowDrawZoom(window);


        if(window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
        {
                foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
                foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));
        }

#if DOES_NOTHING
        /* We need to redo the highlighting in the columns with a zoom handler.  i.e. DNA column... */
        /* NOTE this function does nothing */
        zmapWindowReFocusHighlights(window);
#endif

        zMapWindowRedraw(window);/* exposes the blank edges as well as the canvas */
    }

  return ;
}




void zmapWindowZoomToWorldPosition(ZMapWindow window, gboolean border,
                                   double rootx1, double rooty1, double rootx2, double rooty2)
{
  GtkAdjustment *v_adjuster ;
  /* size of bound area */
  double ydiff;
  double area_middle;
  int win_height;
  int canvasx, canvasy;
  int beforex, beforey;
  /* Zoom factor */
  double zoom_by_factor, target_zoom_factor, current;
  double wx1, wx2, wy1, wy2;
  int two_times, border_size = ZMAP_WINDOW_FEATURE_ZOOM_BORDER;

  zMapReturnIfFail(window) ;

  /*
   * (sm23) Adjustment to y coordinates of region to which to zoom in order
   * to account for cases where the request is off the front or back of the
   * current view.
   */
  rooty1 = rooty1 < window->min_coord ? window->min_coord : rooty1 ;
  rooty2 = rooty2 > window->max_coord ? window->max_coord : rooty2 ;

  v_adjuster = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
  win_height = v_adjuster->page_size;

  /* If we want a border add it...
   * Actually take it away from the current height of the canvas, this
   * way that pixel size will automatically be ratioed by the zoom.
   */
  if(border && ((two_times = (border_size * 2)) < win_height))
    win_height -= two_times;

  current = zMapWindowGetZoomFactor(window) ;

  /* make them canvas so we can scroll there */
  foo_canvas_w2c(window->canvas, rootx1, rooty1, &beforex, &beforey);

  /* work out the zoom factor to show all the area (vertically) and
   * calculate how much we need to zoom by to achieve that.
   *
   * (sm23) Note that these values are most often going to be the
   * feature start and end coordinates, and these may be equal.
   * Previously the code was doing nothing if the difference was
   * less than ZOOM_SENSITIVITY, which is clearly wrong.
   */
  ydiff = rooty2 - rooty1;
  ydiff = ydiff < ZOOM_SENSITIVITY ? ZOOM_SENSITIVITY : ydiff ;
  area_middle = rooty1 + (ydiff / 2.0) ; /* So we can make this the centre later */
  target_zoom_factor = (double)(win_height / ydiff);
  zoom_by_factor = (target_zoom_factor / current);

  /* Actually do the zoom */
  /* possible bug here with width and scrolling, need to check. */
  if (window->locked_display)
    {
      LockedDisplayStruct locked_data = { NULL };

      locked_data.window      = window ;
      locked_data.type        = ZMAP_LOCKED_ZOOM_TO;
      locked_data.zoom_factor = zoom_by_factor ;
      locked_data.position    = area_middle ;
      locked_data.border = border;
      locked_data.start = rooty1;
      locked_data.end = rooty2;
      locked_data.border_size = border_size;

      g_hash_table_foreach(window->sibling_locked_windows, lockedDisplayCB, (gpointer)&locked_data) ;
    }
  else
    {
      foo_canvas_busy(window->canvas,TRUE);
      myWindowZoomTo(window, zoom_by_factor, rooty1, rooty2, border, border_size) ;
      foo_canvas_busy(window->canvas,FALSE);
    }


  /* Now we need to find where the original top of the area is in
   * canvas coords after the effect of the zoom. Hence the w2c calls
   * below.
   * And scroll there:
   * We use this rather than zMapWindowScrollTo as we may have zoomed
   * in so far that we can't just sroll the current canvas buffer to
   * where we clicked. We actually need to check we haven't zoomed off.
   * If we have then we need to move there first, otherwise scroll_to
   * doesn't do anything.
   */
  foo_canvas_get_scroll_region(window->canvas, &wx1, &wy1, &wx2, &wy2); /* ok, but can we refactor? */
  if (rooty1 > wy1 && rooty2 < wy2)
    {                           /* We're still in the same area, */
      foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);

      /* If we had a border we need to take this into account,
       * otherwise the feature just ends up at the top of the
       * window with 2 border widths at the bottom! */
      if (border)
        canvasy -= border_size;

      foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
    }
  else
    { /* This takes a lot of time.... */
      double half_win_span = ((window->canvas_maxwin_size / target_zoom_factor) / 2.0);
      double min_seq = area_middle - half_win_span;
      double max_seq = area_middle + half_win_span - 1;

      /* unfortunately freeze/thaw child-notify doesn't stop flicker */
      /* can we do something else to make it busy?? */
      zMapWindowMove(window, min_seq, max_seq);

      foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);

      /* Do the right thing with the border again. */
      if (border)
        canvasy -= border_size;

      /* No need to worry about border here as we're using the centre of the window. */
      foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
    }

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS FUNCTION IS NOT CURRENTLY USED BUT I'VE KEPT IT IN CASE WE NEED A
 * HANDLER THAT WILL RESPOND TO EVENTS ON ALL ITEMS.... */

/* Handle any events that are not intercepted by items or columns on the canvas,
 * I'm not sure how often this will be called as probably it will be covered mostly
 * by the column or item things...or the features within columns. */
static gboolean canvasRootEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  gboolean event_handled = FALSE ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindow window = (ZMapWindow)data ;
  printf("ROOT canvas event handler\n") ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
        printf("ROOT group event handler - CLICK\n") ;
        /* Call the callers routine for button clicks. */
        window_cbs_G->click(window, window->app_data, NULL) ;
        event_handled = FALSE ;
        break ;
      }
    case GDK_BUTTON_RELEASE:
      {
        printf("GENERAL canvas event handler: button release\n") ;
        /* Call the callers routine for button clicks. */
        window_cbs_G->click(window, window->app_data, NULL) ;
        event_handled = FALSE ;
        break ;
      }
    default:
      {
        event_handled = FALSE ;
        break ;
      }
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return event_handled ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* NOTE: This routine only gets called when the canvas widgets parent requests a resize, not when
 * the canvas changes its own size through zooming.
 *
 * We _may_ need to take action when the canvas window changes size:
 *
 * If we are at min zoom and the window gets larger then we need to redo the min zoom so
 * the sequence once again occupies the whole window....
 *
 * ...otherwise we don't need to do anything, the scrolled window/canvas interface takes
 * care of sorting out the scroll bars.
 *
 * NOTES:
 *
 * - foocanvas uses GtkLayout which has two windows, the one that's visible on the screen
 * and the larger scrolling one that sits "behind" it with the actual data. alloc_XXX is
 * the size of the visible window, actual_XXX is the size of the scrolling window.
 *
 *  -  you cannot use the sizes of the layout widgets actual windows because when this
 * routine is called they have not yet been set to the new sizes specified in the allocation.
 *
 * - the allocation struct passed in seems always to have the same values as the allocation
 * struct embedded in the widget passed in (the foocanvas/layout).....duh....
 *
 */
static void canvasSizeAllocateCB(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data ;
  int layout_win_width, layout_win_height,  layout_binwin_width, layout_binwin_height ;

  zMapReturnIfFail(window) ;


  /* Window size has changed so we need to set zoom accordingly. */
  setZoomFromLayoutSize(window,
                        &layout_win_width, &layout_win_height,
                        &layout_binwin_width, &layout_binwin_height) ;


  /* Record size of window allocated to visible layout window. */
  window->layout_window_width = layout_win_width ;
  window->layout_window_height = layout_win_height ;
  window->layout_bin_window_width = layout_binwin_width ;
  window->layout_bin_window_height = layout_binwin_height ;


  return ;
}


/* Gets current canvas/layout widget window sizes and compares them to previous sizes.
 * If the layout view window has got bigger than the layout bin window then we need
 * to actually zoom the sequence to fill the new bigger window. If the layout view
 * window is still smaller than the layout bin window then we just need to record
 * that the minimum zoom has changed.
 *
 * Optionally returns the layout window sizes but note that you must specify
 * all the layout_* variables or none of them. */
static void setZoomFromLayoutSize(ZMapWindow window,
                                  int *layout_win_width_out, int *layout_win_height_out,
                                  int *layout_binwin_width_out, int *layout_binwin_height_out)
{
  int layout_win_width, layout_win_height,  layout_binwin_width, layout_binwin_height ;

  zMapReturnIfFail(window) ;

  /* Get allocated size of canvas layout widget windows. */
  zmapWindowGetCanvasLayoutSize(window->canvas,
                                &layout_win_width, &layout_win_height, &layout_binwin_width, &layout_binwin_height) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    /* debug info...leaving this in for now in case we need further fixing... */
    unsigned int layout_width, layout_height ;
    int layout_win_width, layout_win_height,  layout_binwin_width, layout_binwin_height ;

    layout_alloc_width = widget->allocation.width ;
    layout_alloc_height = widget->allocation.height ;
    gtk_layout_get_size(layout, &layout_width, &layout_height) ;
    gdk_drawable_get_size(GDK_DRAWABLE(widget->window), &layout_win_width, &layout_win_height) ;
    gdk_drawable_get_size(GDK_DRAWABLE(layout->bin_window), &layout_binwin_width, &layout_binwin_height) ;

    zMapDebugPrintf("\nLayout width/height - alloc: %d, %d\tactual: %u, %u\twin: %d, %d\tbin_win: %d, %d\n",
                    layout_alloc_width, layout_alloc_height, layout_width, layout_height,
                    layout_win_width, layout_win_height,  layout_binwin_width, layout_binwin_height) ;

    zMapDebugPrintf("ZMapWindow width/height - alloc: %d, %d\twindow: %d, %d\tbin_win: %d, %d\n",
                    window->layout_alloc_width, window->layout_alloc_height,
                    window->layout_actual_width, window->layout_actual_height,
                    window->bin_window_width, window->bin_window_height) ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* If the window has got bigger than (essentially) the length of the displayed sequence
   * then we need to zoom so sequence fills whole window.
   * (Need to test seqLength because this routine can be called before we have loaded
   * the sequence.) */
  if ((window->seqLength))
    {
      ZMapWindowVisibilityChangeStruct vis_change ;
      double start, end;

      if (window->layout_bin_window_height < layout_win_height)
        {
          zmapWindowZoomControlHandleResize(window);
        }
      else
        {
          zmapWindowZoomControlRegisterResize(window) ;
        }


      zmapWindowGetScrollableArea(window, NULL, &start, NULL, &end);

      vis_change.zoom_status    = zMapWindowGetZoomStatus(window) ;
      vis_change.scrollable_top = start ;
      vis_change.scrollable_bot = end ;
      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
   }

  /* Optionally return the layout window sizes. */
  if (layout_win_width_out)
    {
      *layout_win_width_out = layout_win_width ;
      *layout_win_height_out = layout_win_height ;
      *layout_binwin_width_out = layout_binwin_width ;
      *layout_binwin_height_out = layout_binwin_height ;
    }

   return ;
}





/*
 *   A set of routines for handling locking/unlocking of scrolling/zooming between several windows.
 */


/* Checks to see if the new locking is different in orientation from the existing locking,
 * if it is we need to remove this window from its locking group and put it in new one.
 * Returns adjusters appropriate for the requested locking, n.b. the unlocked adjuster
 * will be set to NULL. */
static void setCurrLock(ZMapWindowLockType window_locking, ZMapWindow window,
GtkAdjustment **hadjustment, GtkAdjustment **vadjustment)
{
  zMapReturnIfFail(window) ;

  if (!window->locked_display)
    {
      /* window is not locked, so create a lock set for it. */

      lockWindow(window, window_locking) ;
    }
  else
    {
      if (window_locking != window->curr_locking)
        {
          /* window is locked but requested lock is different orientation from existing one
           * so we need to relock the window in a new orientation. */
          unlockWindow(window, FALSE) ;

          lockWindow(window, window_locking) ;
        }
      else
        {
          /* Window is locked and requested orientation is same as existing one so continue
           * with this locking scheme. */
          ;
        }
    }

  /* Return appropriate adjusters for this locking. */
  *hadjustment = *vadjustment = NULL ;
  if (window_locking == ZMAP_WINLOCK_HORIZONTAL)
    *hadjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
  else if (window_locking == ZMAP_WINLOCK_VERTICAL)
    *vadjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  return ;
}


/* May need to make this routine more flexible..... */
static void lockWindow(ZMapWindow window, ZMapWindowLockType window_locking)
{
  zMapReturnIfFail(window) ;

  if (!(!window->locked_display && window->curr_locking == ZMAP_WINLOCK_NONE
      && !window->sibling_locked_windows) )
    return ;

  window->locked_display = TRUE ;
  window->curr_locking = window_locking ;

  window->sibling_locked_windows = g_hash_table_new(g_direct_hash, g_direct_equal) ;
  g_hash_table_insert(window->sibling_locked_windows, window, window) ;

  return ;
}


static void copyLockWindow(ZMapWindow original_window, ZMapWindow new_window)
{

  zMapReturnIfFail(original_window) ;

  zMapReturnIfFail(new_window) ;

  if (!(original_window->locked_display && original_window->curr_locking != ZMAP_WINLOCK_NONE
        && original_window->sibling_locked_windows) )
    return ;
  if (!(!new_window->locked_display && new_window->curr_locking == ZMAP_WINLOCK_NONE
        && !new_window->sibling_locked_windows) )
    return ;

  new_window->locked_display = original_window->locked_display ;
  new_window->curr_locking = original_window->curr_locking ;
  new_window->sibling_locked_windows = original_window->sibling_locked_windows ;
  g_hash_table_insert(new_window->sibling_locked_windows, new_window, new_window) ;

  return ;
}


static void unlockWindow(ZMapWindow window, gboolean no_destroy_if_empty)
{
  gboolean removed ;

  zMapReturnIfFail(window) ;

  zMapReturnIfFail((window->locked_display
                    && window->curr_locking != ZMAP_WINLOCK_NONE && window->sibling_locked_windows)) ;

  if ((removed = g_hash_table_remove(window->sibling_locked_windows, window)))
    {
      if (!g_hash_table_size(window->sibling_locked_windows))
        {
          if (!no_destroy_if_empty)
            g_hash_table_destroy(window->sibling_locked_windows) ;
        }
      else
        {
          /* we only need to allocate a new adjuster if the hash table is not empty...otherwise we can
           * keep our existing adjuster. */
          GtkAdjustment *adjuster ;
          GtkAdjustment *v_adjuster ;

          /* Make a copy any shared adjusters according to orientation of locking */
          if (window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
            adjuster = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
          else
            adjuster = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

          adjuster = copyAdjustmentObj(adjuster) ;

          if (window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
            {
              gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window), adjuster) ;
            }
          else
            {
              gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window), adjuster) ;
            }


          /* And reset vertical ajuster in scale to ensure it is valid. */
          v_adjuster = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

          zmapWindowScaleCanvasSetVAdjustment(window->ruler, adjuster);
        }

      if (g_hash_table_size(window->sibling_locked_windows) == 1)
        {
          removeLastLockWindow(window->sibling_locked_windows) ;

          g_hash_table_destroy(window->sibling_locked_windows) ;
        }

      window->locked_display= FALSE ;
      window->curr_locking = ZMAP_WINLOCK_NONE ;
      window->sibling_locked_windows = NULL ;
    }

  return ;
}


static void removeLastLockWindow(GHashTable *sibling_locked_windows)
{
  GList *keys ;


  keys = g_hash_table_get_keys(sibling_locked_windows) ;

  if (g_list_length(keys) == 1)
    {
      ZMapWindow window ;

      window = (ZMapWindow)(keys->data) ;

      window->locked_display= FALSE ;
      window->curr_locking = ZMAP_WINLOCK_NONE ;
      window->sibling_locked_windows = NULL ;
    }


  return ;
}




static GtkAdjustment *copyAdjustmentObj(GtkAdjustment *orig_adj)
{
  GtkAdjustment *copy_adj = NULL ;

  zMapReturnValIfFail(orig_adj, copy_adj) ;

  copy_adj = GTK_ADJUSTMENT(gtk_adjustment_new(orig_adj->value,
       orig_adj->lower,
       orig_adj->upper,
       orig_adj->step_increment,
       orig_adj->page_increment,
       orig_adj->page_size)) ;

  return copy_adj ;
}


static void lockedDisplayCB(gpointer key, gpointer value, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)key ;
  LockedDisplay locked_data = (LockedDisplay)user_data ;

  zMapReturnIfFail(window && locked_data) ;

  foo_canvas_busy(window->canvas,TRUE);

  if(locked_data->type == ZMAP_LOCKED_ZOOMING)
    myWindowZoom(window, locked_data->zoom_factor, locked_data->position) ;
  else if(locked_data->type == ZMAP_LOCKED_MOVING)
    myWindowMove(window, locked_data->start, locked_data->end);
  else if(locked_data->type == ZMAP_LOCKED_ZOOM_TO)
    myWindowZoomTo(window, locked_data->zoom_factor, locked_data->start, locked_data->end, locked_data->border, locked_data->border_size) ;
  else
    zMapWarning("locked display did not have a type... (%d)", "", (int) locked_data->type);

  foo_canvas_busy(window->canvas,FALSE);

  return ;
}


static void scrollToCB(gpointer key, gpointer value_unused, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)key ;
  ScrollTo scroll_to_data = (ScrollTo)user_data ;

  zMapReturnIfFail(window && scroll_to_data) ;

  foo_canvas_scroll_to(window->canvas, scroll_to_data->x, scroll_to_data->y) ;

  return ;
}


void zmapWindowFetchData(ZMapWindow window,
                         ZMapFeatureBlock block, GList *featureset_name_list,
                         gboolean use_mark, gboolean is_column)
{
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapWindowCallbackCommandGetFeaturesStruct get_data = {ZMAPWINDOW_CMD_INVALID} ;
  ZMapWindowCallbackGetFeatures fetch_data;
  gboolean load = TRUE;
  int start, end ;

  zMapReturnIfFail(window) ;

  if (featureset_name_list)
    {
      fetch_data = &get_data ;

      fetch_data->cmd   = ZMAPWINDOW_CMD_GETFEATURES ;
      fetch_data->block = block ;

      if (!use_mark || (use_mark && !(zmapWindowMarkIsSet(window->mark))))
        {
          fetch_data->start = block->block_to_sequence.block.x1 ;
          fetch_data->end   = block->block_to_sequence.block.x2 ;
          //zMapLogWarning("fetch all: %d %d %d",use_mark,fetch_data->start,fetch_data->end);

        }
      else if (zmapWindowMarkIsSet(window->mark) && zmapWindowMarkGetSequenceRange(window->mark, &start, &end))
        {
          fetch_data->start = start;
          fetch_data->end   = end;
          //zMapLogWarning("fetch mark: %d %d %d",use_mark,fetch_data->start,fetch_data->end);
        }
      else
        {
          load = FALSE;
        }

      if (load)
        {
          FooCanvasItem *block_group;

          if((block_group = zmapWindowFToIFindItemFull(window, window->context_to_item,
                                                       block->parent->unique_id,
                                                       block->unique_id, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
            {

              /*! \todo #warning need to optimise requests so as to not req data we already have */
              /* for the moment just re-request */
              /*
                ideally we should optimise the requests to only cover empty regions
                but we need to cater for repeat loaded features anyway at the edges
                so just request whatever region is requested regardless
              */
            }
        }

      if (load)
        {
          /* the user requests columns but zmap requests featuresets, so expand the list */
          GList *fset_list = NULL;
          GList *col_list;

          if (is_column)/* user requests a column */
            {
              for (col_list = featureset_name_list ; col_list ; col_list = col_list->next)
                {
                  GList *set_list ;
                  GQuark orig_id ;
                  GQuark unique_id ;

                  orig_id = GPOINTER_TO_UINT(col_list->data) ;
                  unique_id = zMapFeatureSetCreateID((char *)g_quark_to_string(orig_id)) ;

                  /* must copy as this is a static list */
                  if (window->context_map && (set_list = window->context_map->getColumnFeatureSets(unique_id, TRUE)))
                    {
                      fset_list
                        = g_list_concat(g_list_copy(set_list), fset_list) ;
                    }
                }
            }
          else/* zmap requests data via column menu */
            {
              fset_list = featureset_name_list;
            }

          /* mh17 NOTE
           *
           * this block copied to zmapWindowMenus.c/add_column_featuresets()
           */
          /* expand any virtual featursets into real ones */
          {
            GList *virtual_featureset ;

            for(virtual_featureset = fset_list ; virtual_featureset ; virtual_featureset = virtual_featureset->next)
              {
                GList *real = (GList *)g_hash_table_lookup(window->context_map->virtual_featuresets, virtual_featureset->data) ;

                if (real)
                  {
                    GList *copy_list = g_list_copy(real);/* so it can be freed with the rest later */
                    GList *l = virtual_featureset;

                    copy_list->prev = virtual_featureset->prev;
                    if(copy_list->prev)
                      copy_list->prev->next = copy_list;
                    else
                      fset_list = copy_list;

                    copy_list = g_list_last(copy_list);
                    copy_list->next = virtual_featureset->next;
                    if(copy_list->next)
                      copy_list->next->prev = copy_list;

                    virtual_featureset = copy_list;

                    g_list_free_1(l);
                  }
              }
          }


          fetch_data->feature_set_ids = fset_list;

          (*(window_cbs_G->command))(window, window->app_data, fetch_data) ;
        }
    }

  return ;
}




/* Handles all keyboard events for the ZMap window, as per gtk callback rules
 * returns TRUE if it handled the event, FALSE otherwise. See Coordinates.shtml */
static gboolean keyboardEvent(ZMapWindow window, GdkEventKey *key_event)
{
  gboolean event_handled = FALSE ;

  zMapReturnValIfFail(key_event, event_handled) ;

  switch (key_event->keyval)
    {

    case GDK_Return:
    case GDK_KP_Enter:
      {
        /* User can press "return" and if there is a highlighted feature we display its details. */
        FooCanvasItem *focus_item ;

        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          {
            zmapWindowFeatureShow(window, focus_item, FALSE) ;
          }
        else
          {
            zMapMessage("%s", "No feature selected.") ;
          }

        event_handled = TRUE ;

        break ;
      }

    case GDK_Up:
    case GDK_Down:
    case GDK_Left:
    case GDK_Right:
      /* Note that we fall through if shift mask is not on.... */

      if (zMapGUITestModifiers(key_event, GDK_SHIFT_MASK))
        {
          /* Trial code to shift columns and features via cursor keys..... */
          if (key_event->keyval == GDK_Up || key_event->keyval == GDK_Down)
            jumpFeature(window, key_event->keyval) ;
          else
            jumpColumn(window, key_event->keyval) ;

          break ;
        }

    case GDK_Page_Up:
    case GDK_Page_Down:
    case GDK_Home:
    case GDK_End:
      {
        moveWindow(window, key_event) ;

        event_handled = TRUE ;
        break ;
      }

    case GDK_plus:
    case GDK_minus:
    case GDK_equal:
    case GDK_KP_Add:
    case GDK_KP_Subtract:
      {
        zoomWindow(window, key_event) ;

        event_handled = TRUE ;
        break ;
      }

    case GDK_1:
    zMapLogWarning("point 1","");
    break;
    case GDK_2:
    zMapLogWarning("point 2","");
    break;
    case GDK_3:
    zMapLogWarning("point 3","");
    break;

    case GDK_Delete:
    case GDK_BackSpace:    /* Good for Macs which have no delete key. */
      {
        /* Hide/Show features, Delete will hide any highlighted features, shift-delete shows them
         * again. */
        FooCanvasGroup *focus_column ;

        if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
          {
            ZMapWindowContainerFeatureSet focus_container;

            focus_container = (ZMapWindowContainerFeatureSet)focus_column;

            /* Treat the annotation column differently */
            if (zmapWindowContainerFeaturesetGetColumnUniqueId(focus_container)
                == zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME))
              {
                zmapWindowScratchClear(window) ;
              }
            else
              {
                if (zMapGUITestModifiers(key_event, GDK_SHIFT_MASK))
                  {
                    GList *hidden_items = NULL ;

                    if ((hidden_items = zmapWindowContainerFeatureSetPopHiddenStack(focus_container)))
                      {
                        g_list_foreach(hidden_items, unhideItemsCB, window) ;
                        g_list_free(hidden_items) ;
                      }
                  }
                else
                  {
                    GList *hidden_items = NULL ;

                    zmapWindowFocusHideFocusItems(window->focus, &hidden_items) ;

                    /* NOTE that this does not delete any lines joining homols etc...that
                     * would require a callback into the alignment object code I think... */

                    zmapWindowContainerFeatureSetPushHiddenStack(focus_container, hidden_items) ;
                  }

                /* We don't bump here because user may be deleting a series of features so it
                 * be irritating to keep bumping.
                 * MH17: use control key to prevent bump
                 */

                if (!zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
                  {
                    ZMapWindowCompressMode compress_mode;

                    if (zmapWindowMarkIsSet(window->mark))
                      compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
                    else
                      compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

                    zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(focus_column), ZMAPBUMP_INVALID, compress_mode) ;
                  }
              }

            /* Make sure selected features are shown or hidden. */
            zmapWindowFullReposition(window->feature_root_group,TRUE, "key_del") ;
          }

        event_handled = TRUE ;

        break ;
      }


      /* alphabetic order from here on.... */

    case GDK_a:
    case GDK_A:
      {
        ZMapWindowAlignSetType requested_homol_set = ZMAPWINDOW_ALIGNCMD_INVALID ;
        FooCanvasItem *focus_item  ;

        /* User just pressed a key so pass in focus item if there is one. */
        focus_item = zmapWindowFocusGetHotItem(window->focus) ;

        if (key_event->state & GDK_CONTROL_MASK)
          requested_homol_set = ZMAPWINDOW_ALIGNCMD_MULTISET ;
        else if (key_event->keyval == GDK_a)
          {
            requested_homol_set = ZMAPWINDOW_ALIGNCMD_SET ;
            if(!focus_item)
              focus_item = (FooCanvasItem *) zmapWindowFocusGetHotColumn(window->focus);
          }
        else if (key_event->keyval == GDK_A)
          requested_homol_set = ZMAPWINDOW_ALIGNCMD_FEATURES ;

        zmapWindowCallBlixem(window, focus_item, requested_homol_set, NULL, NULL, 0.0, 0.0) ;

        break ;
      }

    case GDK_b:
    case GDK_B:
      {
        /* Toggle bumping for selected column, if column is unbumped it will be
         * bumped in default mode for column. */
        FooCanvasGroup *focus_column ;

        if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
          {
            ZMapWindowContainerFeatureSet focus_container;
            ZMapStyleBumpMode curr_bump_mode, bump_mode ;
            ZMapWindowCompressMode compress_mode ;

            focus_container = (ZMapWindowContainerFeatureSet)focus_column;

            curr_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(focus_container);

            if (curr_bump_mode != ZMAPBUMP_UNBUMP)
              bump_mode = ZMAPBUMP_UNBUMP ;
            else
              bump_mode = zmapWindowContainerFeatureSetGetDefaultBumpMode(focus_container) ;

            if (key_event->keyval == GDK_B)
              {
                compress_mode = ZMAPWINDOW_COMPRESS_VISIBLE ;
              }
            else
              {
                if (zmapWindowMarkIsSet(window->mark))
                  compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
                else
                  compress_mode = ZMAPWINDOW_COMPRESS_ALL ;
              }

            zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(focus_column), bump_mode, compress_mode) ;

            zmapWindowFullReposition(window->feature_root_group,TRUE, "key b") ;
          }

        event_handled = TRUE ;
        break ;
      }

    case GDK_c:
    case GDK_C:
      {
        /* Compress visible columns so that any not showing features are hidden. */
        FooCanvasGroup *focus_column ;

        if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
          {
            ZMapWindowCompressMode compress_mode ;

            if (key_event->keyval == GDK_C)
              compress_mode = ZMAPWINDOW_COMPRESS_VISIBLE ;
            else
              {
                if (zmapWindowMarkIsSet(window->mark))
                  compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
                else
                  compress_mode = ZMAPWINDOW_COMPRESS_ALL ;
              }

            zmapWindowColumnsCompress(FOO_CANVAS_ITEM(focus_column), window, compress_mode) ;
          }

        event_handled = TRUE ;

        break ;
      }


    case GDK_d:
    case GDK_D:
      {
        /* Use the current focus item and display translation of it. */
        FooCanvasItem *focus_item ;

        /* If there is a focus item use that.  */
        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          {
            ZMapFeatureAny context ;
            ZMapFeature feature ;
            gboolean is_transcript ;
            char *dna ;
            gboolean spliced = TRUE, cds = FALSE ;

            feature = zmapWindowItemGetFeature(focus_item);
            context = zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                ZMAPFEATURE_STRUCT_CONTEXT);

            is_transcript = ZMAPFEATURE_IS_TRANSCRIPT(feature) ;

            if (is_transcript)
              {
                if (zMapGUITestModifiers(key_event, GDK_SHIFT_MASK))
                  cds = TRUE ;
                else if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
                  spliced = FALSE ;

                dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds) ;
              }
            else
              {
                dna = zMapFeatureGetFeatureDNA(feature) ;
              }

            if (!dna)
              {
                const char *err_msg ;

                if (is_transcript && (cds && !(feature->feature.transcript.flags.cds)))
                  err_msg = "Transcript has no CDS" ;
                else
                  err_msg = "No DNA loaded in view" ;

                zMapWarning("%s", err_msg) ;
              }
            else
              {
                zMapGUISetClipboard(window->toplevel, GDK_SELECTION_PRIMARY, dna) ;

                g_free(dna) ;
              }
          }

        event_handled = TRUE ;

        break ;
      }


#if MH17_DONT_INCLUDE
// NOTE this code will probably break due to later mods, list will be of FToi hash values not ZMapCanvasItems
    case GDK_e:

      if(zmap_development_G)
        {
            // this is test code to exercise the focus functions
            // it only affects the current window and callbacks to the view are not used
          zmapWindowFocusAddItemsType(window->focus,
                  zmapWindowFocusGetFocusItemsType(window->focus,WINDOW_FOCUS_GROUP_FOCUS),
                  zmapWindowFocusGetHotItem(window->focus),
                  WINDOW_FOCUS_GROUP_EVIDENCE);
          zmapWindowFocusReset(window->focus);
        }
      break;
    case GDK_E:   // can only rub out one at a time :-(
      if(zmap_development_G)
        {
  // this is test code to exercise the focus functions
  // it only affects the current window and callbacks to the view are not used
          zmapWindowFocusRemoveFocusItemType(window->focus,
                  zmapWindowFocusGetHotItem(window->focus),
                  WINDOW_FOCUS_GROUP_EVIDENCE, TRUE) ;
          zmapWindowFocusReset(window->focus);
        }
      break;
#endif



    case GDK_f:
    case GDK_F:
      {
        FooCanvasGroup *focus_column ;

        if (!(focus_column = zmapWindowFocusGetHotColumn(window->focus)))
          {
            /* We can't do anything if there is no highlight feature. */

            zMapMessage("%s", "No features selected.") ;
          }
        else
          {
            ZMapWindowCallbackCommandFilterStruct filter_data = {ZMAPWINDOW_CMD_COLFILTER, FALSE} ;
            ZMapWindowCallbacks window_cbs_G ;

            if (window->filter_feature_set)
              {
                window_cbs_G = zmapWindowGetCBs() ;

                if (window->filter_on)
                  {

                    /* Not sure yet what to put here...check out correct values..... */
                    filter_data.selected = ZMAP_CANVAS_FILTER_NONE ;
                    filter_data.filter = ZMAP_CANVAS_FILTER_NONE ;
                    filter_data.action = ZMAP_CANVAS_ACTION_SHOW ;
                    filter_data.target_type = ZMAP_CANVAS_TARGET_NOT_FILTER_FEATURES ;
                    filter_data.filter_column = (ZMapWindowContainerFeatureSet)focus_column ;
                    filter_data.target_column = window->filter_feature_set ;

                    (*(window_cbs_G->command))(window, window->app_data, &filter_data) ;

                    window->filter_on = FALSE ;
                  }
                else
                  {
                    FooCanvasItem *focus_item ;
                    GList *highlight_features = NULL ;

                    if (!((focus_item = zmapWindowFocusGetHotItem(window->focus))
                          && (highlight_features = zmapWindowFocusGetFeatureList(window->focus))))
                      {
                        /* We can't do anything if there is no highlight feature. */

                        zMapMessage("%s", "No features selected.") ;
                      }
                    else
                      {
                        /* focus list is not always position sorted so do that now.... */
                        highlight_features = g_list_sort(highlight_features, zMapFeatureSortFeatures) ;

                        if (zmapWindowMarkIsSet(window->mark) && zmapWindowMarkGetSequenceRange(window->mark,
                                                                                                &(filter_data.seq_start),
                                                                                                &(filter_data.seq_end)))
                          {
                            /* Exclude any features not overlapping the given region.... */
                            highlight_features = zMapFeatureGetOverlapFeatures(highlight_features,
                                                                               filter_data.seq_start, filter_data.seq_end,
                                                                               ZMAPFEATURE_OVERLAP_ALL) ;
                          }

                        if (highlight_features)
                          {
                            /* Record the current hot item. */
                            window->focus_column = focus_column ;
                            window->focus_item = focus_item ;
                            window->focus_feature = zMapWindowCanvasItemGetFeature(focus_item) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                            /* OK, NEED TO GET THIS STUFF FROM STUFF STORED ON WINDOW STRUCT.... */

                            filter_data.selected = ZMAP_CANVAS_FILTER_PARTS ;
                            filter_data.filter = ZMAP_CANVAS_FILTER_PARTS ;
                            filter_data.action = ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE ;
                            filter_data.target_type = ZMAP_CANVAS_TARGET_ALL ;

                            filter_data.do_filter = TRUE ;
                            filter_data.filter_column = (ZMapWindowContainerFeatureSet)focus_column ;
                            filter_data.filter_features = highlight_features ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                            /* How does target col get set ??????? */
                            filter_data.target_column = window->filter_feature_set ;

                            filter_data.selected = window->filter_selected ;
                            filter_data.filter = window->filter_filter ;
                            filter_data.action = window->filter_action ;
                            filter_data.target_type = window->filter_target ;




                            filter_data.do_filter = TRUE ;
                            filter_data.filter_column = (ZMapWindowContainerFeatureSet)focus_column ;
                            filter_data.filter_features = highlight_features ;

                            (*(window_cbs_G->command))(window, window->app_data, &filter_data) ;

                            if (filter_data.result)
                              window->filter_on = TRUE ;

                            g_list_free(highlight_features) ;
                          }
                      }
                  }
              }
          }

        event_handled = TRUE ;

        break;
      }



    case GDK_h:
      {
        /* Flip flop highlighting.... */
        FooCanvasGroup *focus_column = NULL ;
        FooCanvasItem *focus_item = NULL  ;
        ZMapFeature focus_feature = NULL ;

        focus_column = zmapWindowFocusGetHotColumn(window->focus) ;
        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          focus_feature = zMapWindowCanvasItemGetFeature(focus_item) ;

        if (focus_column)
          {
            /* If something is focussed then un-focus it (only need to test column, that implies feature). */
            window->focus_column = focus_column ;
            window->focus_item = focus_item ;
            window->focus_feature = focus_feature ;

            zmapWindowUnHighlightFocusItems(window) ;
          }
        else
          {
            if (window->focus_column)
              {
                if (window->focus_feature)
                  {
                    /* If there's a recorded focus then use that. */
                    gboolean replace_highlight = TRUE, highlight_same_names = TRUE ;
                    ZMapFeatureSubPart sub_feature = NULL ;
                    int start = window->focus_feature->x1, end = window->focus_feature->x2;
                    gboolean control = FALSE;
                    ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED,
                                                                  ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                                  ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS} ;

                    /* refocus on existing highlight group..... */
                    if (window->focus_feature->mode != ZMAPSTYLE_MODE_ALIGNMENT
                        || zMapStyleIsUnique(*window->focus_feature->style))
                      highlight_same_names = FALSE ;

                    /* Pass information about the object clicked on back to the application. */
                    zmapWindowUpdateInfoPanel(window, window->focus_feature, NULL, window->focus_item, sub_feature,
                                              start, end, start, end,
                                              NULL, replace_highlight, highlight_same_names,
                                              control, &display_style) ;
                  }
                else
                  {
                    zmapWindowFocusSetHotColumn(window->focus, window->focus_column, NULL) ;

                    zmapWindowFocusHighlightHotColumn(window->focus) ;
                  }
              }
            else
              {
                /* A bit cruddy really, highlight the first forward strand column. */

                focus_column = zmapWindowGetFirstColumn(window, ZMAPSTRAND_FORWARD) ;

                zmapWindowFocusSetHotColumn(window->focus, focus_column, NULL) ;

                zmapWindowFocusHighlightHotColumn(window->focus) ;
              }
          }
        break ;
      }

    case GDK_H:
      {
        /* Restore focus to any stored column/feature. */
        if (window->focus_column)
          {
            if (window->focus_item)
              {
                gboolean replace_highlight = TRUE, highlight_same_names = TRUE ;
                ZMapFeatureSubPart sub_feature = NULL ;
                int start = window->focus_feature->x1, end = window->focus_feature->x2;
                gboolean control = FALSE;
                ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED,
                                                              ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                              ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS} ;

                /* refocus on existing highlight group..... */
                if (window->focus_feature->mode != ZMAPSTYLE_MODE_ALIGNMENT
                    || zMapStyleIsUnique(*window->focus_feature->style))
                  highlight_same_names = FALSE ;

                /* Pass information about the object clicked on back to the application. */
                zmapWindowUpdateInfoPanel(window, window->focus_feature, NULL, window->focus_item, sub_feature,
                                          start, end, start, end,
                                          NULL, replace_highlight, highlight_same_names, control, &display_style) ;
              }
            else
              {
                zmapWindowFocusSetHotColumn(window->focus, window->focus_column, NULL) ;

                zmapWindowFocusHighlightHotColumn(window->focus) ;
              }
          }

        break;
      }

    case GDK_k:
    case GDK_K:
      {
        if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
          {
            /* Copy the selected feature to the scratch column */
            FooCanvasItem *item = zmapWindowFocusGetHotItem(window->focus);
            ZMapFeature feature = NULL;

            if (item && (feature = zmapWindowItemGetFeature(item)))
              {
                /* \todo It would be good to have a shortcut do copy just the subfeature
                 * but I'm not sure how to find it without mouse click coords */
//double y_pos = (double)((feature->x1 + feature->x2) / 2) ;
//                if (zMapGUITestModifiers(key_event, GDK_SHIFT_MASK))
//                  zmapWindowScratchCopyFeature(window, feature, item, 0, y_pos, TRUE); /* subfeature */
//                else
                zmapWindowScratchCopyFeature(window, feature, item, 0, 0, FALSE); /* whole feature */
              }
          }

        break;
      }

    case GDK_m:
    case GDK_M:
      {
        /* Toggle marking/unmarking an item for later zooming/column bumping etc. */
        zmapWindowToggleMark(window, key_event->keyval == GDK_M);
      }
      break;

      /* N/n see GDK_p for combined code. */

    case GDK_o:
    case GDK_O:
      {
        /* User can press "O" to get menu instead of mouse click. */
        FooCanvasItem *focus_item = NULL ;

        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          focus_item = zmapWindowItemGetTrueItem(focus_item) ;
        else
          focus_item = FOO_CANVAS_ITEM(zmapWindowFocusGetHotColumn(window->focus)) ;

        if (focus_item)
          popUpMenu(key_event, window, focus_item) ;

        break ;
      }

    case GDK_n:
    case GDK_N:
    case GDK_p:
    case GDK_P:
    {
        /* Use the current focus item and display translation of it. */
        FooCanvasItem *focus_item ;

        /* If there is a focus item use that.  */
        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          {
            ZMapFeatureAny context;
            ZMapFeature feature ;
            char *fasta;
            ZMapSequenceType sequence_type ;

            if (key_event->keyval == GDK_n || key_event->keyval == GDK_N)
              sequence_type = ZMAPSEQUENCE_DNA ;
            else
              sequence_type = ZMAPSEQUENCE_PEPTIDE ;

            feature = zmapWindowItemGetFeature(focus_item);
            context = zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                ZMAPFEATURE_STRUCT_CONTEXT);

            if ((fasta = zmapWindowFeatureTranscriptFASTA(feature, sequence_type, TRUE, TRUE)))
              {
                char *seq_name = NULL, *gene_name = NULL, *title;

                seq_name  = (char *)g_quark_to_string(context->original_id) ;
                gene_name = (char *)g_quark_to_string(feature->original_id) ;

                title = g_strdup_printf("ZMap - %s%s%s",
                                        seq_name,
                                        gene_name ? ":" : "",
                                        gene_name ? gene_name : "");
                                        zMapGUIShowText(title, fasta, FALSE);

                                        g_free(title);
                                        g_free(fasta);
              }

          }

        break ;
      }

    case GDK_r:
      {
        /* Reverse complement. */
        revCompRequest(window) ;

        break ;
      }

    case GDK_s:
    case GDK_S:
      {
        /* If user presses 's'/'S' we toggle the splice highlight according to the
         * current state of splice highlighting. If splices are on, they're turned off,
         * if they are on then splice highlighting is done based on currently highlighted feature. */
        ZMapWindowCallbackCommandFilterStruct filter_data = {ZMAPWINDOW_CMD_COLFILTER, FALSE} ;
        ZMapWindowCallbacks window_cbs_G ;


        window_cbs_G = zmapWindowGetCBs() ;

        if (window->splice_highlight_on)
          {
            FooCanvasGroup *focus_column ;

            if (!(focus_column = zmapWindowFocusGetHotColumn(window->focus)))
              {
                /* We can't do anything if there is no highlight feature. */

                zMapMessage("%s", "No features selected.") ;
              }
            else
              {
                filter_data.filter = ZMAP_CANVAS_FILTER_PARTS ;
                filter_data.action = ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE ;
                filter_data.target_type = ZMAP_CANVAS_TARGET_ALL ;
                filter_data.selected = ZMAP_CANVAS_FILTER_NONE ;
                filter_data.filter = ZMAP_CANVAS_FILTER_NONE ;
                filter_data.filter_column = (ZMapWindowContainerFeatureSet)focus_column ;

                (*(window_cbs_G->command))(window, window->app_data, &filter_data) ;

                window->splice_highlight_on = FALSE ;
              }
          }
        else
          {
            FooCanvasGroup *focus_column ;
            FooCanvasItem *focus_item ;
            GList *highlight_features = NULL ;

            if (!((focus_column = zmapWindowFocusGetHotColumn(window->focus))
                  && (focus_item = zmapWindowFocusGetHotItem(window->focus))
                  && (highlight_features = zmapWindowFocusGetFeatureList(window->focus))))
              {
                /* We can't do anything if there is no highlight feature. */

                zMapMessage("%s", "No features selected.") ;
              }
            else
              {
                /* focus list is not always position sorted so do that now.... */
                highlight_features = g_list_sort(highlight_features, zMapFeatureSortFeatures) ;

                if (zmapWindowMarkIsSet(window->mark) && zmapWindowMarkGetSequenceRange(window->mark,
                                                                                        &(filter_data.seq_start),
                                                                                        &(filter_data.seq_end)))
                  {
                    /* Exclude any features not overlapping the given region.... */
                    highlight_features = zMapFeatureGetOverlapFeatures(highlight_features,
                                                                       filter_data.seq_start, filter_data.seq_end,
                                                                       ZMAPFEATURE_OVERLAP_ALL) ;
                  }

                if (highlight_features)
                  {
                    /* Record the current hot item. */
                    window->focus_column = focus_column ;
                    window->focus_item = focus_item ;
                    window->focus_feature = zMapWindowCanvasItemGetFeature(focus_item) ;

                    /* do the splice stuff... */
                    filter_data.selected = ZMAP_CANVAS_FILTER_PARTS ;
                    filter_data.filter = ZMAP_CANVAS_FILTER_PARTS ;
                    filter_data.action = ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE ;
                    filter_data.target_type = ZMAP_CANVAS_TARGET_ALL ;

                    filter_data.do_filter = TRUE ;
                    filter_data.filter_column = (ZMapWindowContainerFeatureSet)focus_column ;
                    filter_data.filter_features = highlight_features ;

                    (*(window_cbs_G->command))(window, window->app_data, &filter_data) ;

                    if (filter_data.result)
                      window->splice_highlight_on = TRUE ;

                    g_list_free(highlight_features) ;
                  }
              }
          }

        event_handled = TRUE ;

        break;
      }

    case GDK_t:
    case GDK_T:
      {
        /* Show translation of an item. */
        FooCanvasItem *focus_item ;

        /* If there is a focus item use that.  */
        if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
          {
            if (key_event->keyval == GDK_t)
              zmapWindowItemShowTranslation(window, focus_item);
            else
              zmapWindowItemShowTranslationRemove(window, focus_item);
          }
        else
          {
            zMapMessage("%s", "No feature selected.") ;
          }

        break;
      }


    case GDK_u:
    case GDK_U:
      {
        /* 'u' for update....rebumps a column. */
        FooCanvasGroup *focus_column ;

        if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
          {
            ZMapWindowContainerFeatureSet focus_container;
            ZMapStyleBumpMode curr_bump_mode ;

            focus_container = (ZMapWindowContainerFeatureSet)focus_column;

            curr_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(focus_container) ;

            zmapWindowColumnBump(FOO_CANVAS_ITEM(focus_column), ZMAPBUMP_UNBUMP) ;

            zmapWindowColumnBump(FOO_CANVAS_ITEM(focus_column), curr_bump_mode) ;

            zmapWindowFullReposition(window->feature_root_group,TRUE, "key u") ;
          }

        event_handled = TRUE ;

        break ;
      }

    case GDK_w:
    case GDK_W:
      {
        /* Zoom out to show whole sequence. */
        zMapWindowZoomToMin(window) ;

        event_handled = TRUE ;
        break ;
      }

      /* testing... */
#if 0
    case GDK_x:
      {     FooCanvasItem *focus_item ;
            if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
            {
                  int i;

                  zMapStartTimer("Test map","100000");

                  for(i = 0;i < 100000;i++)
                  {
                        foo_canvas_item_unmap(focus_item);
                        foo_canvas_item_map(focus_item);
                  }
                  zMapStopTimer("Test map","done");
                  /* takes 8ms for 100k */
            }
      }
      break;
#endif

    case GDK_Y:
    case GDK_y:
      {
        zmapWindowScratchRedo(window);
        break;
      }

    case GDK_Z:
    case GDK_z:
      {
        if (zMapGUITestModifiers(key_event, GDK_CONTROL_MASK))
          {
            zmapWindowScratchUndo(window);
          }
        else
          {
            /* Zoom to an item or subpart of an item. */
            FooCanvasItem *focus_item ;
            gboolean mark_set ;


            /* If there is a focus item(s) we zoom to that, if not we zoom to
             * any marked feature or area.  */
            if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
              {
                ZMapFeature feature ;

                feature = zmapWindowItemGetFeature(focus_item);

                /* If there is a marked feature(s), for "z" we zoom just to the highlighted
                 * feature, for "Z" we zoom to the whole transcript/HSP's */
                if (key_event->keyval == GDK_z)
                  {
                    GList *focus_items ;

                    if ((focus_items = zmapWindowFocusGetFocusItemsType(window->focus, WINDOW_FOCUS_GROUP_FOCUS)))
                      {
                        zmapWindowZoomToItems(window, focus_items) ;

                        g_list_free(focus_items) ;
                      }
                  }
                else
                  {
                    if (feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
                      {
                        zmapWindowZoomToItem(window, zmapWindowItemGetTrueItem(focus_item)) ;
                      }
                    else if (feature->mode == ZMAPSTYLE_MODE_ALIGNMENT)
                      {
                        GList *glist = NULL;
                        ZMapStrand set_strand ;
                        ZMapFrame set_frame ;
                        gboolean result ;

                        result = zmapWindowItemGetStrandFrame(focus_item, &set_strand, &set_frame) ;

                        glist = zmapWindowFToIFindSameNameItems(window,window->context_to_item,
                                                               zMapFeatureStrand2Str(set_strand),
                                                               zMapFeatureFrame2Str(set_frame),
                                                               feature) ;

                        zmapWindowZoomToItems(window, glist) ;

                        g_list_free(glist) ;
                      }
                    else
                      {
                        zmapWindowZoomToItem(window, focus_item) ;
                      }
                  }
              }
            else if ((mark_set = zmapWindowMarkIsSet(window->mark)))
              {
                double rootx1, rooty1, rootx2, rooty2 ;

                zmapWindowMarkGetWorldRange(window->mark, &rootx1, &rooty1, &rootx2, &rooty2) ;

                zmapWindowZoomToWorldPosition(window, TRUE, rootx1, rooty1, rootx2, rooty2) ;
              }

            event_handled = TRUE ;
          }
        break ;
      }

    default:
      {
        event_handled = FALSE ;
        break ;
      }
    }

  return event_handled ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printWindowSizeDebug(char *prefix, ZMapWindow window,
 GtkWidget *widget, GtkAllocation *allocation)
{
  FooCanvas *canvas = FOO_CANVAS(widget) ;
  GtkLayout *layout = &(canvas->layout) ;

  zMapUtilsDebugPrintf(stderr, "%s ----\n", prefix) ;

  zMapUtilsDebugPrintf(stderr, "alloc_req: %d, %d\tallocation_widg: %d, %d\twindow: %d, %d\n",
       allocation->width, allocation->height,
       widget->allocation.width, widget->allocation.height,
       window->window_width, window->window_height) ;

  zMapUtilsDebugPrintf(stderr, "layout: %d, %d\tcanvas: %d, %d\n",
       layout->width, layout->height,
       window->canvas_width, window->canvas_height) ;

  zMapUtilsDebugPrintf(stderr, "scroll: %f, %f, %f, %f\t\t%f, %f\n\n",
       canvas->scroll_x1, canvas->scroll_y1,
       canvas->scroll_x2, canvas->scroll_y2,
       floor ((canvas->scroll_x2 - canvas->scroll_x1) * canvas->pixels_per_unit_x + 0.5),
       floor ((canvas->scroll_y2 - canvas->scroll_y1) * canvas->pixels_per_unit_y + 0.5)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Checks to see if current item has coords outside of current min/max. */
static void getMaxBounds(gpointer data, gpointer user_data)
{
  ID2Canvas id2c = (ID2Canvas) data;
  FooCanvasItem *item = (FooCanvasItem *)id2c->item ;
  MaxBounds max_bounds = (MaxBounds)user_data ;
  double rootx1, rootx2, rooty1, rooty2 ;

  zMapReturnIfFail(data && user_data) ;

  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
  {
          /* this is a ZMapWindowCanvasItem ie a foo canvas group  */
          zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem) item, (ZMapFeature) id2c->feature_any);
          zMapWindowCanvasFeaturesetGetFeatureBounds(item, &rootx1, &rooty1, &rootx2, &rooty2);
  }
  else
  {
        /* Get size of item and convert to world coords. */
        foo_canvas_item_get_bounds(item, &rootx1, &rooty1, &rootx2, &rooty2) ;
        foo_canvas_item_i2w(item, &rootx1, &rooty1) ;
        foo_canvas_item_i2w(item, &rootx2, &rooty2) ;
  }

  if (max_bounds->rootx1 == 0.0  && max_bounds->rooty1 == 0.0
      && max_bounds->rootx2 == 0.0 && max_bounds->rooty2 == 0.0)
    {
      max_bounds->rootx1 = rootx1 ;
      max_bounds->rooty1 = rooty1 ;
      max_bounds->rootx2 = rootx2 ;
      max_bounds->rooty2 = rooty2 ;
    }
  else
    {
      if (rootx1 < max_bounds->rootx1)
        max_bounds->rootx1 = rootx1 ;
      if (rooty1 < max_bounds->rooty1)
        max_bounds->rooty1 = rooty1 ;
      if (rootx2 > max_bounds->rootx2)
        max_bounds->rootx2 = rootx2 ;
      if (rooty2 > max_bounds->rooty2)
        max_bounds->rooty2 = rooty2 ;
    }

  return ;
}


/* Jump to the previous/next feature according to whether up or down arrow was pressed.
 *
 * NOTE: this function does NOT scroll to the feature, it simply highlights it. In
 * particular if something is not in the scrolled region because the user has zoomed
 * right in, that item will not be visible and will not be highlighted.
 *
 * This is in contrast to jumpColumn where we do scroll. It seems to make more sense
 * for features not to scroll otherwise the user will suddenly be taken somewhere
 * far away from what they were just looking at.
 *
 *  */
static void jumpFeature(ZMapWindow window, guint keyval)
{
  FooCanvasItem *focus_item ;
  FooCanvasGroup *focus_column ;
  gboolean move_focus = FALSE, highlight_item = FALSE ;

  zMapReturnIfFail(window) ;

  /* Is there is a highlighted feature then move to next one,
   * if not default to the first feature in the leftmost forward strand column. */
  if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
    {
      move_focus = TRUE ;

      focus_item = zmapWindowItemGetTrueItem(focus_item) ;
    }
  else
    {
      ZMapWindowContainerGroup focus_container;


      if (!(focus_column = zmapWindowFocusGetHotColumn(window->focus)))
        {
          focus_column = zmapWindowGetFirstColumn(window, ZMAPSTRAND_FORWARD) ;
        }

      if (ZMAP_IS_CONTAINER_GROUP(focus_column))
        {
          focus_container = ZMAP_CONTAINER_GROUP(focus_column);

          if (keyval == GDK_Down)
            focus_item = zmapWindowContainerGetNthFeatureItem(focus_container, ZMAPCONTAINER_ITEM_FIRST) ;
          else
            focus_item = zmapWindowContainerGetNthFeatureItem(focus_container, ZMAPCONTAINER_ITEM_LAST) ;

          /* If the item is a valid feature item then we get it highlighted, otherwise we must search
           * for the next valid one. */
          if (checkItem(focus_item, GINT_TO_POINTER(TRUE)))
            highlight_item = TRUE ;
          else
            move_focus = TRUE ;
        }
    }

  /* Unhighlight any features/columns currently highlighted. */
  zmapWindowUnHighlightFocusItems(window) ;

  /* If we need to jump features then do it. */
  if (move_focus)
    {
      ZMapContainerItemDirection direction ;

      if (keyval == GDK_Up)
        direction = ZMAPCONTAINER_ITEM_PREV ;
      else
        direction = ZMAPCONTAINER_ITEM_NEXT ;


      if ((focus_item = zmapWindowContainerGetNextFeatureItem(focus_item, direction, TRUE,
                                                              checkItem, GINT_TO_POINTER(TRUE))))
        {
          move_focus = TRUE ;
          highlight_item = TRUE ;
        }

    }


  /* if we need to highlight a feature then do it. */
  if (highlight_item)
    {
      gboolean replace_highlight = TRUE, highlight_same_names = FALSE ;

      ZMapFeature feature ;

      zmapWindowHighlightObject(window, focus_item, TRUE, FALSE, FALSE) ;

      feature = zmapWindowItemGetFeature(focus_item);


      /* Pass information about the object clicked on back to the application. */
      zmapWindowUpdateInfoPanel(window, feature, NULL, focus_item, NULL, 0, 0, 0, 0,
                                NULL, replace_highlight, highlight_same_names, FALSE, NULL) ;
    }


  return ;
}


/* Jump to the previous/next column according to which arrow key was pressed. */
static void jumpColumn(ZMapWindow window, guint keyval)
{
  /*! \todo This was commented out "temporarily" by Malcolm (probably because changes to
   * the drawing code broke it) and was never put back in. It doesn't compile any more so
   * it needs fixing up. */
#if 0
  FooCanvasGroup *focus_column;
  gboolean move_focus = FALSE, highlight_column = FALSE ;

  /* If there is no focus column we default to the leftmost forward strand column. */
  if ((focus_column = zmapWindowFocusGetHotColumn(window->focus)))
    {
      move_focus = TRUE ;
    }
  else
    {
      ZMapStrand strand ;

      if (keyval == GDK_Right)
        strand = ZMAPSTRAND_FORWARD ;
      else
        strand = ZMAPSTRAND_REVERSE ;

      focus_column = zmapWindowGetFirstColumn(window, strand) ;

      if (checkItem(FOO_CANVAS_ITEM(focus_column), GINT_TO_POINTER(FALSE)))
        highlight_column = TRUE ;
      else
        move_focus = TRUE ;
    }

  /* If we need to jump columns then do it. */
  if (move_focus)
    {
      ZMapWindowContainerGroup container_strand;
      ZMapStrand strand ;
      ZMapContainerItemDirection direction ;

      container_strand = zmapWindowContainerUtilsItemGetParentLevel((FooCanvasItem *)focus_column,
    ZMAPCONTAINER_LEVEL_STRAND) ;
      strand = zmapWindowContainerGetStrand((ZMapWindowContainerGroup)container_strand) ;

      if (keyval == GDK_Left)
        direction = ZMAPCONTAINER_ITEM_PREV ;
      else
        direction = ZMAPCONTAINER_ITEM_NEXT ;

      /* Move to next column in strand group, if we run out of columns wrap around to the
       * opposite strand. */
      if ((focus_column = FOO_CANVAS_GROUP(zmapWindowContainerGetNextFeatureItem(FOO_CANVAS_ITEM(focus_column),
                                                                         direction, FALSE,
                                                                         checkItem,
                                                                         GINT_TO_POINTER(FALSE)))))
        {
          highlight_column = TRUE ;
        }
      else
        {
          ZMapWindowContainerGroup container_block;

          container_block = zmapWindowContainerUtilsGetParentLevel(container_strand,
                                                                   ZMAPCONTAINER_LEVEL_BLOCK) ;

          if (strand == ZMAPSTRAND_FORWARD)
            strand = ZMAPSTRAND_REVERSE ;
          else
            strand = ZMAPSTRAND_FORWARD ;

          container_strand =
            (ZMapWindowContainerGroup)zmapWindowContainerBlockGetContainerStrand((ZMapWindowContainerBlock)container_block, strand) ;

          if (keyval == GDK_Left)
            focus_column = FOO_CANVAS_GROUP(zmapWindowContainerGetNthFeatureItem(container_strand,
         ZMAPCONTAINER_ITEM_LAST)) ;
          else
            focus_column = FOO_CANVAS_GROUP(zmapWindowContainerGetNthFeatureItem(container_strand,
         ZMAPCONTAINER_ITEM_FIRST)) ;

          /* Found a column but check it and move on to the next if not visible. */
          if (checkItem(FOO_CANVAS_ITEM(focus_column), GINT_TO_POINTER(FALSE)))
            {
              highlight_column = TRUE ;
            }
          else
            {
              if ((focus_column = FOO_CANVAS_GROUP(zmapWindowContainerGetNextFeatureItem(FOO_CANVAS_ITEM(focus_column),
                                                                         direction, FALSE,
                                                                         checkItem,
                                                                         GINT_TO_POINTER(FALSE)))))
                {
                  highlight_column = TRUE ;
                }
            }
        }
    }

  /* If we need to highlight a column then do it but also scroll to the column if its off screen. */
  if (highlight_column)
    {
      ZMapWindowSelectStruct select = {0} ;
/*      ZMapFeatureSet feature_set = NULL ;*/
      ZMapWindowContainerFeatureSet container_set = (ZMapWindowContainerFeatureSet) focus_column;
      ZMapFeatureColumn column;
      GQuark set_id;

      zmapWindowUnHighlightFocusItems(window) ;

      zmapWindowFocusSetHotColumn(window->focus, focus_column,NULL) ;

      zmapWindowFocusHighlightHotColumn(window->focus) ;

      zmapWindowScrollToItem(window, FOO_CANVAS_ITEM(focus_column)) ;

      set_id = zmapWindowContainerFeaturesetGetColumnUniqueId(container_set);
      // why unique_id, why not original_id ??
      select.secondary_text =
      select.feature_desc.feature_set = (char *)g_quark_to_string(set_id) ;

      column = g_hash_table_lookup(window->context_map->columns,GUINT_TO_POINTER(set_id));
      select.secondary_text = column->column_desc;
      /* column must exist and it will have some description, defaulting to the name of the column
       * ideally we  should default it to the best featureset description on config or data load
       */
      select.type = ZMAPWINDOW_SELECT_SINGLE;

      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

      g_free(select.secondary_text) ;

    }
#endif
  return ;
}





/* GFunc() to hide the given item and record it in the user hidden list. */
static void unhideItemsCB(gpointer data, gpointer user_data)
{
  ID2Canvas id2c = (ID2Canvas) data;
  FooCanvasItem *item = id2c->item;
  ZMapWindow window = (ZMapWindow)user_data ;

  zMapReturnIfFail(data) ;

  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
    {
      zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem) id2c->item, (ZMapFeature) id2c->feature_any);
      zMapWindowCanvasItemShowHide((ZMapWindowCanvasItem)id2c->item, TRUE);
    }
  else
    {
      foo_canvas_item_show(item) ;
    }

  zmapWindowHighlightObject(window, item, FALSE, TRUE, FALSE) ;

  return ;
}







/* ACTUALLY THIS DOESN'T SEEM TO WORK PROPERLY ON THE FEATURE CHECK.... */
/* A zmapWindowContainerItemTestCallback() that tests a canvas item to make sure it is visible
 * and optionally that it represents a feature. */
static gboolean checkItem(FooCanvasItem *item, gpointer user_data)
{
  gboolean status = FALSE ;
  gboolean check_for_feature = GPOINTER_TO_INT(user_data) ;
  ZMapFeature feature = NULL ;

  if (zmapWindowItemIsShown((FooCanvasItem *)(item))
      && (!check_for_feature || (feature = zmapWindowItemGetFeature(item))))
    status = TRUE ;

  feature = zMapWindowCanvasItemGetFeature(item) ;

  return status ;
}




/* Function to pop up the appropriate menu for either a column or a feature in response
 * to a key press rather than a button press. */
static void popUpMenu(GdkEventKey *key_event, ZMapWindow window, FooCanvasItem *focus_item)
{
  gboolean is_feature = FALSE ;
  GdkEventButton button_event ;
  double x1, y1, x2, y2 ;
  double vis_can_x1, vis_can_y1, vis_can_x2, vis_can_y2 ;
  double worldx, worldy, winx, winy ;
  Window root_window, item_window, child = 0 ;
  Display *display ;
  Bool result ;
  int dest_x = 0, dest_y= 0 ;

  zMapReturnIfFail(window && focus_item) ;

  /* We are only going to pop up the menu if the item as at least partly visible. */
  if (zmapWindowItemIsOnScreen(window, focus_item, FALSE))
    {
      /* Is the item a feature or a column ? */
      if(ZMAP_IS_CANVAS_ITEM(focus_item))
        {
          is_feature = TRUE ;
        }
      else if (ZMAP_IS_CONTAINER_GROUP(focus_item) &&
       zmapWindowContainerUtilsGetLevel(focus_item) == ZMAPCONTAINER_LEVEL_FEATURESET)
        {
          is_feature = FALSE ;
        }
      else
        {
          zMapWarnIfReached() ;
        }


      /* Calculate canvas window coords for menu position from part of item which is visible. */
      zmapWindowItemGetVisibleWorld(window, &vis_can_x1, &vis_can_y1, &vis_can_x2, &vis_can_y2) ;

      my_foo_canvas_item_get_world_bounds(focus_item, &x1, &y1, &x2, &y2) ;

      if (x1 < vis_can_x1)
        x1 = vis_can_x1 ;

      if (y1 < vis_can_y1)
        y1 = vis_can_y1 ;

      worldx = x1 ;
      worldy = y1 ;

      foo_canvas_world_to_window(focus_item->canvas, worldx, worldy, &winx, &winy) ;


      /* Convert canvas window coords to root window coords for final menu position. */
      root_window = GDK_WINDOW_XID(gtk_widget_get_root_window(GTK_WIDGET(focus_item->canvas))) ;

      display = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(GTK_WIDGET(focus_item->canvas))) ;

      item_window = GDK_WINDOW_XID(focus_item->canvas->layout.bin_window) ;

      result = XTranslateCoordinates(display, item_window, root_window, winx, winy, &dest_x, &dest_y, &child) ;


      /* Fake a button event....because that's what the menu code uses, we only have to fill in
       * these fields. */
      button_event.x_root = dest_x ;
      button_event.y_root = dest_y ;
      button_event.button = 3 ;
      button_event.time = key_event->time ;


      /* Now call appropriate menu routine. */
      /* focus item is either a CanvasItem or a column ie a window container featureset */
      if (is_feature)
        {
          zmapMakeItemMenu(&button_event, window, focus_item) ;
        }
      else
        {
                ZMapWindowContainerFeatureSet container_set =
                            (ZMapWindowContainerFeatureSet) focus_item;

                zmapMakeColumnMenu(&button_event, window, focus_item, container_set, NULL) ;
        }
    }

  return ;
}



static void printStats(ZMapWindowContainerGroup container_parent, FooCanvasPoints *points,
                       ZMapContainerLevelType level, gpointer user_data)
{
  ZMapFeatureAny any_feature = NULL ;
  GString *text = (GString *) user_data ;

  /* Note strand groups do not have features.... */
  if ((any_feature = zmapWindowItemGetFeatureAny((FooCanvasItem *)container_parent)))
    {
      switch (any_feature->struct_type)
        {
        case ZMAPFEATURE_STRUCT_CONTEXT:
        case ZMAPFEATURE_STRUCT_ALIGN:
        case ZMAPFEATURE_STRUCT_BLOCK:
        case ZMAPFEATURE_STRUCT_FEATURESET:
          {
            ZMapWindowStats stats ;

            stats = (ZMapWindowStats)g_object_get_data(G_OBJECT(container_parent), ITEM_FEATURE_STATS) ;
            zmapWindowStatsPrint(text, stats) ;

            break ;
          }
        default:
          break;
        }
    }

  return ;
}


/* Called when user presses a short cut key, problematic because actually its the layer
 * above that needs to do the reverse complement. */
static void revCompRequest(ZMapWindow window)
{
  ZMapWindowCallbackCommandRevCompStruct rev_comp ;
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;

  rev_comp.cmd = ZMAPWINDOW_CMD_REVERSECOMPLEMENT ;

  (*(window_cbs_G->command))(window, window->app_data, &rev_comp) ;

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Busy cursor stuff.....we should also really block interaction with the canvas.... */
static void canvas_set_busy_cursor(ZMapWindow window, GdkCursor *external_cursor, const char *file, const char *func)
{
  char *file_name ;
  GdkCursor *busy_cursor ;

  zMapReturnIfFail(window) ;

  file_name = g_path_get_basename(file) ;    /* Clip full pathname for shorter debug messages. */


  if (external_cursor)
    busy_cursor = external_cursor  = window->window_external_cursor ;
  else
    busy_cursor = window->window_busy_cursor ;

  if (!busy_cursor)
    {
      zMapLogWarning("%s", "Cannot set Busy cursor as it has not been created yet !") ;

      zMapDebugPrint(busy_debug_G, "%s - %s: window %p, busy cursor has not been created !",
                     file_name, func, window) ;
    }
  else if (!(window->toplevel->window))
    {
      zMapDebugPrint(busy_debug_G, "%s - %s: window %p cursor NOT SET, no window",
                     file_name, func, window) ;
    }
  else
    {
      gdk_window_set_cursor(window->toplevel->window, busy_cursor) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* There is a problem with getting the new cursor shown in a timely way, seems
       * hard (impossible ?) to get this working really well. I've tried flush/sync
       * and round trip requests but all to no avail. It also feels like we may only need to
       * do this the first time we set the cursor as that's the most important for
       * the user to see. */

      if (window->cursor_busy_count == 0)
        {
          guint width, height ;

          gdk_display_get_maximal_cursor_size(gdk_display_get_default(), &width, &height) ;

          gdk_display_flush(gdk_display_get_default()) ;

          gdk_display_sync(gdk_display_get_default()) ;
          gtk_main_iteration() ;
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      window->cursor_busy_count++ ;

      zMapDebugPrint(busy_debug_G, "%s - %s: window %p busy cursor %s, %d",
                     file_name, func,
                     window,
                     (window->cursor_busy_count == 1 ? "turned ON" : "incremented"),
                     window->cursor_busy_count) ;
    }


  g_free(file_name) ;

  return ;
}



static void canvas_unset_busy_cursor(ZMapWindow window, const char *file, const char *func)
{
  char *file_name ;

  zMapReturnIfFail(window) ;

  file_name = g_path_get_basename(file) ;

  if (!(window->normal_cursor))
    {
      zMapLogWarning("%s", "Cannot reset to normal cursor as it has not been created yet !") ;

      zMapDebugPrint(busy_debug_G, "%s - %s: window %p, normal cursor has not been created !",
             file_name, func, window) ;
    }
  else if (!(window->toplevel->window))
    {
      zMapDebugPrint(busy_debug_G, "%s - %s: window %p cursor NOT SET, no window",
             file_name, func, window) ;
    }
  else
    {
      window->cursor_busy_count-- ;

      if (window->cursor_busy_count < 0)
        {
          zMapDebugPrint(busy_debug_G, "%s - %s: window %p, bad cursor busy count (%d)!",
                         file_name, func, window, window->cursor_busy_count) ;

          zMapLogWarning("bad cursor busy count (%d)!", window->cursor_busy_count);
                  window->cursor_busy_count = 0;
        }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* In theory we should reset when we reach zero, be better to  */

      if (window->cursor_busy_count == 0)
        {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* TESTING...... */


          gdk_window_set_cursor(window->toplevel->window, window->normal_cursor) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      zMapDebugPrint(busy_debug_G, "%s - %s: window %p busy cursor %s, %d",
                     file_name, func,
                     window,
                     (window->cursor_busy_count == 0 ? "turned OFF" : "decremented"),
                     window->cursor_busy_count) ;
    }

  g_free(file_name) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Cursor setting....we should also really block interaction with the canvas....
 *
 * Use zmapWindowBusy() macro to call this function in all internal code.
 *
 * Use zMapWindowSetCursor() for all external calls to set cursor.
 *
 * Note that if cursor is NULL then this "unsets" the existing cursor. For
 * internal cursors we reset to whatever the windows normal cursor was
 * (see zmapWindowBusy() macro stuff).
 *
 *
 *
 */
static void canvasSetCursor(ZMapWindow window,
                            gboolean external_cursor, GdkCursor *cursor,
                            const char *file, const char *func)
{
  char *file_name ;
  GdkCursor *new_cursor ;


  /* We should detect when the cursor type is the same and just not set it..... */


  file_name = g_path_get_basename(file) ;    /* Clip full pathname for shorter debug messages. */

  if (external_cursor)
    new_cursor= window->external_cursor = cursor ;
  else
    new_cursor = cursor ;

  /* if (!cursor || (cursor && cursor->type == GDK_LEFT_PTR))
    printf("found it\n") ; */

  if (!zMapGUISetCursor(window->toplevel, new_cursor))
    {
      zMapLogWarning("%s", "Cannot set cursor at ZMapWindow top level, top level has no window.") ;

      zMapDebugPrint(busy_debug_G, "%s - %s: window %p cursor NOT SET, no window",
             file_name, func, window) ;
    }
  else if (!external_cursor)
    {
      if (new_cursor == window->window_busy_cursor)
        {
          window->cursor_busy_count++ ;

          zMapDebugPrint(busy_debug_G, "%s - %s: window %p busy cursor %s, %d",
                         file_name, func,
                         window,
                         (window->cursor_busy_count == 1 ? "turned ON" : "incremented"),
                         window->cursor_busy_count) ;
        }
      else
        {
          window->cursor_busy_count-- ;

          if (window->cursor_busy_count < 0)
            {
              zMapDebugPrint(busy_debug_G, "%s - %s: window %p, bad cursor busy count (%d)!",
                             file_name, func, window, window->cursor_busy_count) ;

              zMapLogWarning("bad cursor busy count (%d), count reset to zero !", window->cursor_busy_count) ;

              window->cursor_busy_count = 0 ;
            }

          zMapDebugPrint(busy_debug_G, "%s - %s: window %p busy cursor %s, %d",
                         file_name, func,
                         window,
                         (window->cursor_busy_count == 0 ? "turned OFF" : "decremented"),
                         window->cursor_busy_count) ;
        }

    }


  g_free(file_name) ;

  return ;
}




#if ZOOM_SCROLL

/* foo canvas signal handlers  */
static void fc_begin_update_cb(FooCanvas *canvas, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  double x1, x2, y1, y2;

  if (canvas != window->canvas)
    {
      return ;
    }
  else
    {
      zmapWindowBusy(window, TRUE) ;
    }

  return ;
}

static void fc_end_update_cb(FooCanvas *canvas, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

  if (canvas == window->canvas && window->feature_context)/* we can get called before the feature context is set */
    {
      double x1, x2, y1, y2;
        int seq_start,seq_end;
        int scroll_start, scroll_end;
      int scroll_x, scroll_y, x, y;

      foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);

        zMapFeatureContextGetMasterAlignSpan(window->feature_context,&seq_start,&seq_end);
        scroll_start = y1;
        scroll_end = y2;
        if(scroll_start < seq_start)
                scroll_start = seq_start;
        if(scroll_end > seq_end)
                scroll_end = seq_end;

        foo_canvas_get_scroll_offsets(canvas, &scroll_x, &scroll_y);
        if(zmapWindowScaleCanvasDraw(window->ruler,  scroll_start, scroll_end, seq_start, seq_end))
          {
              zmapWindowScaleCanvasMaximise(window->ruler, y1, y2);

                /* Cause a never ending loop ? */

              /* The zmapWindowScaleCanvasMaximise does a set scroll
               * region on the ruler canvas which has the side effect
               * of a scroll_to call in the canvas. As that canvas
               * and this canvas share adjusters, this canvas scrolls
               * too.  This isn't really desireable when doing a
               * zmapWindowZoomToWorldPosition for example, so we do
               * a scroll to back to our original position. */
              foo_canvas_get_scroll_offsets(canvas, &x, &y);
              if(y != scroll_y)
                foo_canvas_scroll_to(canvas, scroll_x, scroll_y);
          }

      zmapWindowBusy(window, FALSE) ;
    }

  return ;
}


static void fc_begin_map_cb(FooCanvas *canvas, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

  if (canvas != window->canvas)
    {
      return ;
    }
  else
    {
      zMapDebugPrint(foo_debug_G, "%s",  "Entered") ;

      zmapWindowBusy(window, TRUE) ;

      zMapDebugPrint(foo_debug_G, "%s",  "Exitted") ;
    }

  return ;
}

static void fc_end_map_cb(FooCanvas *canvas, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

  if (canvas != window->canvas)
    {
      return ;
    }
 else
    {
      zMapDebugPrint(foo_debug_G, "%s",  "Entered") ;

      zmapWindowBusy(window, FALSE) ;

      zMapDebugPrint(foo_debug_G, "%s",  "Exitted") ;
    }

  return ;
}


static void fc_draw_background_cb(FooCanvas *canvas, int x, int y, int width, int height, gpointer user_data)
{
  zMapDebugPrint(foo_debug_G, "%s",  "Entered") ;

  if (canvas->root->object.flags & FOO_CANVAS_ITEM_MAPPED)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMapWindow window = (ZMapWindow)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  zMapDebugPrint(foo_debug_G, "%s",  "Exitted") ;

  return ;
}

static void fc_drawn_items_cb(FooCanvas *canvas, int x, int y, int width, int height, gpointer user_data)
{
  zMapDebugPrint(foo_debug_G, "%s",  "Entered") ;

  if(canvas->root->object.flags & FOO_CANVAS_ITEM_MAPPED)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMapWindow window = (ZMapWindow)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  zMapDebugPrint(foo_debug_G, "%s",  "Exitted") ;

  return ;
}

/* end of the foo canvas signal handlers stuff */

#endif


/* Callbacks to manipulate rulers in other windows. */







/*
 *               Set of functions to setup, move and remove the ruler.
 */


static void lockedRulerCB(gpointer key, gpointer value_unused, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)key ;
  LockedRuler locked_data = (LockedRuler)user_data ;

  zMapReturnIfFail(window && locked_data) ;

  switch(locked_data->action)
    {
    case ZMAP_LOCKED_RULER_SETUP:
      setupRuler(window, &(window->horizon_guide_line),
                 &(window->tooltip), locked_data->world_x, locked_data->world_y) ;
      break;
    case ZMAP_LOCKED_RULER_MOVING:
      moveRuler(window->horizon_guide_line,
                window->tooltip,
                locked_data->tip_text,
                locked_data->world_x,
                locked_data->world_y) ;
      break;
    case ZMAP_LOCKED_RULER_REMOVE:
      removeRuler(window->horizon_guide_line, window->tooltip) ;
      break;

    case ZMAP_LOCKED_MARK_GUIDE_SETUP:
      setupRuler(window, &(window->mark_guide_line), NULL, locked_data->world_x, locked_data->origin_y);
      break;
    case ZMAP_LOCKED_MARK_GUIDE_MOVING:
      break;
    case ZMAP_LOCKED_MARK_GUIDE_REMOVE:
      removeRuler(window->mark_guide_line, NULL);
      break;
    default:
      zMapWarnIfReached();
      break;
    }

  return ;
}

/*
 * This allocates a new string that must be freed in the caller.
 * Arguments are the world coordinates.
 */
static char * tooltipTextCreate(ZMapWindow window, double wx, double wy)
{
  char * text = NULL ;
  int display_coord = 0 ;
  ZMapWindowDisplayCoordinates display_coordinates = ZMAP_WINDOW_DISPLAY_INVALID ;

  if (!window)
    return text ;
  display_coordinates = window->display_coordinates ;
  /*display_coordinates = ZMAP_WINDOW_DISPLAY_CHROM ; */

  if (window->sequence)
    {
      display_coord = (int)floor(wy);
      switch (display_coordinates)
        {
          case ZMAP_WINDOW_DISPLAY_SLICE:
            display_coord = zmapWindowCoordToDisplay(window, display_coord) ;
            text = g_strdup_printf("%d", display_coord) ;
            break ;
          case ZMAP_WINDOW_DISPLAY_CHROM:
            display_coord = zmapWindowWorldToSequenceForward(window, display_coord) ;
            if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
              display_coord = -display_coord ;
            text = g_strdup_printf("%d", display_coord) ;
            break ;
          case ZMAP_WINDOW_DISPLAY_OTHER:
          case ZMAP_WINDOW_DISPLAY_INVALID:
            text = NULL ;
            break ;
          default:
            break ;
        }

    }



  return text ;
}


static void setupRuler(ZMapWindow       window,
       FooCanvasItem  **horizon,
       FooCanvasGroup **tooltip,
       double           x_coord,
       double           y_coord)
{

  zMapReturnIfFail(window) ;

  if(horizon && !*horizon)
    *horizon = zMapDrawHorizonCreate(window->canvas, &window->colour_horizon) ;

  if (tooltip && !*tooltip)
    *tooltip = zMapDrawToolTipCreate(window->canvas);

  if(horizon)
    zMapDrawHorizonReposition(*horizon, y_coord) ;


  if(tooltip)
    {
      char * tip_text = tooltipTextCreate(window, x_coord, y_coord) ;
      if (tip_text)
        zMapDrawToolTipSetPosition(*tooltip, x_coord, y_coord, tip_text) ;
      else
        foo_canvas_item_hide(FOO_CANVAS_ITEM(tooltip)) ;
    }

  return ;
}




static void moveRuler(FooCanvasItem  *horizon,
      FooCanvasGroup *tooltip,
      char      *tip_text,
      double     world_x,
      double     world_y)
{
  if(horizon)
    zMapDrawHorizonReposition(horizon, world_y) ;

  if(tooltip)
    {
      if (tip_text)
        zMapDrawToolTipSetPosition(tooltip, world_x, world_y, tip_text) ;
      else
        foo_canvas_item_hide(FOO_CANVAS_ITEM(tooltip)) ;
    }

  return ;
}


static void removeRuler(FooCanvasItem *horizon, FooCanvasGroup *tooltip)
{

  if(horizon)
    foo_canvas_item_hide(horizon) ;
  if(tooltip)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(tooltip)) ;

  return ;
}



static gboolean within_x_percent(ZMapWindow window, double percent, double y, gboolean in_top)
{
  double scr_y1, scr_y2, ten_top, ten_bot, range;
  gboolean in_ten = FALSE;

  zMapReturnValIfFail(window, in_ten) ;

  foo_canvas_get_scroll_region(window->canvas, NULL, &scr_y1, NULL, &scr_y2);

  range = scr_y2 - scr_y1;

  ten_top = (range * percent) + scr_y1;
  ten_bot = scr_y2 - (range * percent);

  if(in_top && y < ten_top)
    in_ten = TRUE;
  else if(!in_top && y > ten_bot)
    in_ten = TRUE;

  return in_ten;
}

static gboolean real_recenter_scroll_window(ZMapWindow window, unsigned int one_to_hundred, double world_y, gboolean in_top)
{
  double percentage = 0.1;
  gboolean within_range = FALSE;
  gboolean moved = FALSE;

  if(one_to_hundred < 1)
    one_to_hundred = 1;

  percentage = one_to_hundred / 100.0;

  if((within_range = within_x_percent(window, percentage, world_y, in_top)) == TRUE)
    {
      double sx1, sx2, sy1, sy2, tmps, diff;

      /* Get scroll region (clamped to sequence coords) */
      zmapWindowGetScrollableArea(window, &sx1, &sy1, &sx2, &sy2);
      /* we now have scroll region coords */
      /* set tmp to centre of regions ... */
      tmps = sy1 + ((sy2 - sy1) / 2);
      /* find difference between centre points */
      if((diff = tmps - world_y) > 5.0 || (diff < 5.0))
        {
          /* alter scroll region to match that */
          sy1 -= diff; sy2 -= diff;
          /* clamp in case we've moved outside the sequence */
          zmapWindowClampSpan(window, &sy1, &sy2);
          /* Do the move ... */
          zMapWindowMove(window, sy1, sy2);
          moved = TRUE;
        }
      else
        moved = FALSE;
    }

  return moved;
}

static gboolean recenter_scroll_window(ZMapWindow window, double *event_y_in_out)
{
  GtkAdjustment *vadjust;
  double world_x, world_y;
  int height;
  gboolean moved = FALSE, top_half = FALSE;

  zMapReturnValIfFail(window, moved) ;

  foo_canvas_window_to_world(window->canvas,
                             0.0, *event_y_in_out,
                             &world_x, &world_y);

  height  = GTK_WIDGET(window->canvas)->allocation.height;

  vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));

  if((*event_y_in_out - vadjust->value) < (height / 2.0))
    top_half = TRUE;

  if(real_recenter_scroll_window(window, 10, world_y, top_half))
    {
      double window_x, window_y;
      moved = TRUE;
      foo_canvas_world_to_window(window->canvas,
                                 world_x, world_y,
                                 &window_x, &window_y);
      *event_y_in_out = window_y; /* trying this... */
    }

  return moved;
}



/*!
 * \brief Redraw the background for the given strand/frame of the given featureset
 */
static void updateColumnBackground(ZMapWindow window,
                                   ZMapFeatureSet feature_set,
                                   gboolean highlight_filtered_columns,
                                   ZMapStrand strand,
                                   ZMapFrame frame)
{
  FooCanvasItem *foo = NULL ;

  zMapReturnIfFail(window) ;

  foo = zmapWindowFToIFindSetItem(window,
                                  window->context_to_item,
                                  feature_set,
                                  strand,
                                  frame);

  if (foo && foo->parent)
    {
      ZMapWindowContainerGroup column = (ZMapWindowContainerGroup)foo->parent;
      GdkColor *fill_col = &window->canvas_background;

      int n_filtered = zMapWindowFeaturesetItemGetNFiltered(foo);

      /* Update the 'filtered' flag in the column based on whether this
       * column should be highlighted as filtered */
      if (highlight_filtered_columns && n_filtered > 0)
        column->flags.filtered = 1;
      else
        column->flags.filtered = 0;

      /* Get the filter colour, if applicable */
      if (column->flags.filtered)
        zMapWindowGetFilteredColour(window, &fill_col);

      zmapWindowDrawSetGroupBackground(window, column,
       0, 1, 1.0,
       ZMAP_CANVAS_LAYER_COL_BACKGROUND, fill_col, NULL);

      foo_canvas_item_request_redraw(foo->parent);
    }
}


/*!
 * \brief Redraw the background for the given featureset in the given window
 *
 * Updates the column in all strands/frames
 */
void zMapWindowUpdateColumnBackground(ZMapWindow window,
                                      ZMapFeatureSet feature_set,
                                      gboolean highlight_filtered_columns)
{

  zMapReturnIfFail(window) ;

  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_FORWARD, ZMAPFRAME_0);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_FORWARD, ZMAPFRAME_1);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_FORWARD, ZMAPFRAME_2);

  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_REVERSE, ZMAPFRAME_NONE);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_REVERSE, ZMAPFRAME_0);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_REVERSE, ZMAPFRAME_1);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_REVERSE, ZMAPFRAME_2);

  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_NONE, ZMAPFRAME_NONE);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_NONE, ZMAPFRAME_0);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_NONE, ZMAPFRAME_1);
  updateColumnBackground(window, feature_set, highlight_filtered_columns, ZMAPSTRAND_NONE, ZMAPFRAME_2);

  /* Re-highlight the hot column, if any */
  zmapWindowFocusHighlightHotColumn(window->focus);


  return ;
}


gboolean zMapWindowGetFlag(ZMapWindow window, ZMapFlag flag)
{                                               
  gboolean result = FALSE ;
  
  if (window && window->sequence)
    result = window->sequence->getFlag(flag) ;

  return result ;
}

void zMapWindowSetFlag(ZMapWindow window, ZMapFlag flag, const gboolean value)
{                                               
  if (window && window->sequence)
    window->sequence->setFlag(flag, value) ;
}


/*
 * STUFF LEFT TO DO.....
 *
 * DEFINE HIGHLIGHT_DATA STRUCT
 * CHANGE FUNC PROTO FOR GETEVIDENCE...AND MAKE IT CALL BACK TO THIS FUNC SUPPLYING
 * THE EVIDENCE LIST AND WE SHOULD BE DONE.....
 *
 * BUT MAY NEED A SPECIAL CALLBACK IN FEATURESHOW TO HANDLE THIS CALLBACK
 * SEPARATELY FROM THE EXISTING FEATURESHOW CALLBACK......
*/

/* This code is a callback, called from zmapWindowFeatureGetEvidence() which
 * contacts our peer to get evidence lists of features.
 * As far as I can tell this code takes a feature and then tries to highlight all
 * the evidence for that feature by searching columns for named evidence for the
 * feature. */
void zmapWindowHighlightEvidenceCB(GList *evidence, gpointer user_data)
{
  ZMapWindowHighlightData highlight_data = (ZMapWindowHighlightData)user_data ;
  GList *evidence_items = NULL ;


  /* search for the features named in the list and find thier canvas items */
  for ( ; evidence ; evidence = evidence->next)
    {
      GList *items,*items_free ;
      GQuark wildcard = g_quark_from_string("*");
      char *feature_name;
      GQuark feature_search_id;

      /* need to add a * to the end to match strand and frame name mangling */
      feature_name = g_strdup_printf("%s*",g_quark_to_string(GPOINTER_TO_UINT(evidence->data)));
      feature_name = zMapFeatureCanonName(feature_name);    /* done in situ */
      feature_search_id = g_quark_from_string(feature_name);

      /* catch NULL names due to ' getting escaped int &apos; */
      if(feature_search_id == wildcard)
        continue;

      items_free = zmapWindowFToIFindItemSetFull(highlight_data->window,
                                                 highlight_data->window->context_to_item,
                                                 highlight_data->feature->parent->parent->parent->unique_id,
                                                 highlight_data->feature->parent->parent->unique_id,
                                                 wildcard,0,"*","*",
                                                 feature_search_id,
                                                 NULL,NULL);


      /* zMapLogWarning("evidence %s returns %d features", feature_name, g_list_length(items_free));*/
      g_free(feature_name);

      for(items = items_free; items; items = items->next)
        {
          /* NOTE: need to filter by transcript start and end coords in case of repeat alignments */
          /* Not so - annotators want to see duplicated features' data */

          evidence_items = g_list_prepend(evidence_items,items->data);
        }

      g_list_free(items_free);
    }

  /* menu_data->item is the transcript and would be the
   * focus hot item if this was a focus highlight
   * in this call it's irrelevant so don't set the hot item, pass NULL instead
   */

  zmapWindowFocusAddItemsType(highlight_data->window->focus, evidence_items,
                              NULL /* menu_data->item */, WINDOW_FOCUS_GROUP_EVIDENCE);


  g_list_free(evidence);
  g_list_free(evidence_items);


  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printAdjusters(ZMapWindow window)
{
  GtkAdjustment *v_adjust, *h_adjust ;

  v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
  h_adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  zMapDebugPrintf("\nv_adjust = %p, h_adjust = %p\n", v_adjust, h_adjust) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


