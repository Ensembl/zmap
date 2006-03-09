/*  File: zmapWindow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Provides interface functions for controlling a data
 *              display window.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Mar  9 11:29 2006 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.109 2006-03-09 11:29:46 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <math.h>
#include <string.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
 
/* Local struct to hold current features and new_features obtained from a server and
 * relevant types. */
typedef struct
{
  ZMapFeatureContext  current_features ;
  ZMapFeatureContext  new_features ;
} FeatureSetsStruct, *FeatureSets ;

/* Used for passing information to the locked display hash callback functions. */
typedef enum {ZMAP_LOCKED_ZOOMING, ZMAP_LOCKED_MOVING} ZMapWindowLockActionType ;
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
} LockedDisplayStruct, *LockedDisplay ;


/* This struct is used to pass data to realizeHandlerCB
 * via the g_signal_connect on the canvas's expose_event. */
typedef struct _RealiseDataStruct
{
  ZMapWindow window ;
  FeatureSets feature_sets ;
} RealiseDataStruct, *RealiseData ;


static ZMapWindow myWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data,
				 GtkAdjustment *hadjustment, GtkAdjustment *vadjustment) ;
static void myWindowZoom(ZMapWindow window, double zoom_factor, double curr_pos) ;
static void myWindowMove(ZMapWindow window, double start, double end) ;

static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static gboolean canvasRootEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void canvasSizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;

static void resetCanvas(ZMapWindow window) ;
static gboolean getConfiguration(ZMapWindow window) ;
static void sendClientEvent(ZMapWindow window, FeatureSets) ;

static void moveWindow(ZMapWindow window, guint state, guint keyval) ;
static void scrollWindow(ZMapWindow window, guint state, guint keyval) ;
static void changeRegion(ZMapWindow window, guint keyval) ;
static void printGroup(FooCanvasGroup *group, int indent) ;


static void setCurrLock(ZMapWindowLockType window_locking, ZMapWindow window, 
			GtkAdjustment **hadjustment, GtkAdjustment **vadjustment) ;
static void lockedDisplayCB(gpointer key, gpointer value, gpointer user_data) ;
static void copyLockWindow(ZMapWindow original_window, ZMapWindow new_window) ;
static void lockWindow(ZMapWindow window, ZMapWindowLockType window_locking) ;
static void unlockWindow(ZMapWindow window) ;
static GtkAdjustment *copyAdjustmentObj(GtkAdjustment *orig_adj) ;

static void zoomToRubberBandArea(ZMapWindow window);

static void printWindowSizeDebug(char *prefix, ZMapWindow window,
				 GtkWidget *widget, GtkAllocation *allocation) ;



/* Callbacks we make back to the level above us. This structure is static
 * because the callback routines are set just once for the lifetime of the
 * process. */
static ZMapWindowCallbacks window_cbs_G = NULL ;


/*! @defgroup zmapwindow   zMapWindow: The feature display window.
 * @{
 * 
 * \brief  Feature Display Window
 * 
 * zMapWindow routines create, modify, manipulate and destroy feature display windows.
 * 
 *
 *  */



/* This routine must be called just once before any other windows routine, it is undefined
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 * 
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of windows terminate routine. */
void zMapWindowInit(ZMapWindowCallbacks callbacks)
{
  zMapAssert(!window_cbs_G) ;

  zMapAssert(callbacks
	     && callbacks->enter && callbacks->leave
	     && callbacks->scroll && callbacks->focus && callbacks->select
	     && callbacks->visibilityChange && callbacks->destroy) ;

  window_cbs_G = g_new0(ZMapWindowCallbacksStruct, 1) ;

  window_cbs_G->enter  = callbacks->enter ;
  window_cbs_G->leave   = callbacks->leave ;
  window_cbs_G->scroll  = callbacks->scroll ;
  window_cbs_G->focus   = callbacks->focus ;
  window_cbs_G->select   = callbacks->select ;
  window_cbs_G->setZoomStatus = callbacks->setZoomStatus;

  window_cbs_G->visibilityChange = callbacks->visibilityChange ;

  window_cbs_G->destroy = callbacks->destroy ;

  return ;
}


ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data)
{
  ZMapWindow window ;

  window = myWindowCreate(parent_widget, sequence, app_data, NULL, NULL) ;

  return window ;
}


/* Makes a new window that is a copy of the existing one, zoom factor and all.
 *
 * NOTE that not all fields are copied here as some need to be done when we draw
 * the actual features (e.g. anything that refers to canvas items). */
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, char *sequence, 
			  void *app_data, ZMapWindow original_window,
			  ZMapFeatureContext feature_context,
			  ZMapWindowLockType window_locking)
{
  ZMapWindow new_window = NULL ;
  GtkAdjustment *hadjustment = NULL, *vadjustment = NULL ;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
  int x, y ;
  
  if (window_locking != ZMAP_WINLOCK_NONE)
    {
      setCurrLock(window_locking, original_window, 
		  &hadjustment, &vadjustment) ;
    }
  
  /* There is an assymetry when splitting windows, when we do a vsplit we lose the scroll
   * position when we the do the windowcreate so we need to get it here so we can
   * reset the scroll to where it should be. */
  foo_canvas_get_scroll_offsets(original_window->canvas, &x, &y) ;
  
  new_window = myWindowCreate(parent_widget, sequence, app_data, hadjustment, vadjustment) ;
  zMapAssert(new_window) ;

  /* Lock windows together for scrolling/zooming if requested. */
  if (window_locking != ZMAP_WINLOCK_NONE)
    {
      copyLockWindow(original_window, new_window) ;
    }
  
  /* A new window will have new canvas items so we need a new hash. */
  new_window->context_to_item = zmapWindowFToICreate() ;

  new_window->canvas_maxwin_size = original_window->canvas_maxwin_size ;
  new_window->min_coord = original_window->min_coord ;
  new_window->max_coord = original_window->min_coord ;

  new_window->seq_start = original_window->seq_start ;
  new_window->seq_end = original_window->seq_end ;
  new_window->seqLength = original_window->seqLength ;
  
  zmapWindowZoomControlCopyTo(original_window->zoom, new_window->zoom);

  /* I'm a little uncertain how much of the below is really necessary as we are
   * going to call the draw features code anyway. */
  /* Set the zoom factor, there is no call to get hold of pixels_per_unit so we dive.
   * into the struct. */
  foo_canvas_set_pixels_per_unit_xy(new_window->canvas,
				    original_window->canvas->pixels_per_unit_x,
				    original_window->canvas->pixels_per_unit_y) ;

  /* Need to do this so that the scroll_to call works */
  foo_canvas_get_scroll_region(original_window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
  zmapWindowClampStartEnd(original_window, &scroll_y1, &scroll_y2);
  foo_canvas_set_scroll_region(new_window->canvas, scroll_x1, scroll_y1, scroll_x2, scroll_y2) ;

  /* Reset our scrolled position otherwise we can end up jumping to the top of the window. */
  foo_canvas_scroll_to(original_window->canvas, x, y) ;
  foo_canvas_scroll_to(new_window->canvas, x, y) ;

  /* You cannot just draw the features here as the canvas needs to be realised so we send
   * an event to get the data drawn which means that the canvas is guaranteed to be
   * realised by the time we draw into it. */
  zMapWindowDisplayData(new_window, feature_context, feature_context) ;
  
  return new_window ;
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
 *    No, that's not the problem, it is realised, it just hasn't got sized properly yet. Ed
 * 
 *  */
void zMapWindowDisplayData(ZMapWindow window, ZMapFeatureContext current_features,
			   ZMapFeatureContext new_features)
{
  FeatureSets feature_sets ;

  feature_sets = g_new0(FeatureSetsStruct, 1) ;
  feature_sets->current_features = current_features ;
  feature_sets->new_features = new_features ;

  if (GTK_WIDGET(window->canvas)->allocation.height > 1
      && GTK_WIDGET(window->canvas)->window)
    {
      sendClientEvent(window, feature_sets) ;
    }
  else
    {
      RealiseData realiseData ;
      
      realiseData = g_new0(RealiseDataStruct, 1) ;	    /* freed in exposeHandlerCB() */

      realiseData->window = window ;
      realiseData->feature_sets = feature_sets ;
      window->exposeHandlerCB = g_signal_connect(GTK_OBJECT(window->canvas), "expose_event",
						 GTK_SIGNAL_FUNC(exposeHandlerCB),
						 (gpointer)realiseData) ;
    }

  return ;
}


/* completely reset window. */
void zMapWindowReset(ZMapWindow window)
{

  resetCanvas(window) ;

  /* Need to reset feature context pointer and any other things..... */

  return ;
}



/* Force a canvas redraw via a circuitous route....
 * 
 * You should note that it is not possible to do this via the foocanvas function
 * foo_canvas_update_now() even if you set the need_update flag to TRUE, this
 * would only work if you set the update flag for every group/item to TRUE as well !
 * 
 * Instead we in effect send an expose event (via dk_window_invalidate_rect()) to
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

  /* Get the size of the canvas's on screen window, i.e. the section of canvas you
   * can actually see. */
  allocation = &(GTK_WIDGET(&(window->canvas->layout))->allocation) ;

  /* Set up the area of this window to be invalidated (i.e. to be redrawn). */
  expose_area.x = expose_area.y = 0 ;
  expose_area.width = allocation->width - 1 ;
  expose_area.height = allocation->height - 1 ;

  /* Invalidate the displayed canvas window causing to be redrawn. */
  gdk_window_invalidate_rect(GTK_WIDGET(&(window->canvas->layout))->window, &expose_area, TRUE) ;

  return ;
}



/* OK, OK, THE 'REVERSED' PARAMETER IS A HACK...NEED TO THINK UP SOME BETTER WAY OF GETTING
 * A WINDOW TO MAINTAIN ITS POSITION AFTER REVCOMP...BUT IT ACTUALLY IS NOT THAT STRAIGHT
 * FORWARD.....ur and we also need to keep a record for the annotator of whether we are reversed
 * or not....UGH..... */
/* Draw the window with new features. */
void zMapWindowFeatureRedraw(ZMapWindow window, ZMapFeatureContext feature_context,
			     gboolean reversed)
{
  int x, y ;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

  if (reversed)
    {
      double tmp ;
      GtkAdjustment *adjust ;
      int new_y ;

      adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

      foo_canvas_get_scroll_offsets(window->canvas, &x, &y) ;

      new_y = adjust->upper - (y + adjust->page_size) ;

      y = new_y ;


      /* We need to get the current position, translate it to world coords, reverse it
       * and then scroll to that....needs some thought....  */

      /* Probably we should reverse the x position as well.... */

      foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

      scroll_y1 = window->seqLength - scroll_y1 ;
      scroll_y2 = window->seqLength - scroll_y2 ;

      tmp = scroll_y1 ;
      scroll_y1 = scroll_y2 ;
      scroll_y2 = tmp ;
    }


  resetCanvas(window) ;					    /* Resets scrolled region.... */

  if (reversed)
    foo_canvas_set_scroll_region(window->canvas, scroll_x1, scroll_y1, scroll_x2, scroll_y2) ;



  /* You cannot just draw the features here as the canvas needs to be realised so we send
   * an event to get the data drawn which means that the canvas is guaranteed to be
   * realised by the time we draw into it. */
  zMapWindowDisplayData(window, feature_context, feature_context) ;


  if (reversed)
    {
      foo_canvas_scroll_to(window->canvas, x, y) ;
    }


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
  unlockWindow(window) ;

  return ;
}




/* ugh, get rid of this.......... */
GtkWidget *zMapWindowGetWidget(ZMapWindow window)
{
  return GTK_WIDGET(window->canvas) ;
}





/* try out the new zoom window.... */
void zMapWindowZoom(ZMapWindow window, double zoom_factor)
{
  int x, y;
  double width, curr_pos = 0.0 ;
  GtkAdjustment *adjust;
  adjust = 
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  /* Record the current position. */
  /* In order to stay centred on wherever we are in the canvas while zooming, we get the 
   * current position (offset, in canvas pixels), add half the page-size to get the centre
   * of the screen,then convert to world coords and store those.    
   * After the zoom, we convert those values back to canvas pixels (changed by the call to
   * pixels_per_unit) and scroll to them. 
   * We end up working this out again in myWindowZoom, if and only if we're horizontally 
   * split windows, but I don't thik it'll be a big issue. 
   */
  foo_canvas_get_scroll_offsets(window->canvas, &x, &y);
  y += adjust->page_size / 2 ;
  foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;
  /* possible bug here with width and scrolling, need to check. */
  if (window->locked_display)
    {
      LockedDisplayStruct locked_data = { NULL };

      locked_data.window      = window ;
      locked_data.type        = ZMAP_LOCKED_ZOOMING;
      locked_data.zoom_factor = zoom_factor ;
      locked_data.position    = curr_pos ;
      g_hash_table_foreach(window->sibling_locked_windows, lockedDisplayCB, (gpointer)&locked_data) ;
    }
  else
    myWindowZoom(window, zoom_factor, curr_pos) ;

  /* We need to scroll to the previous position. This is dependent on
   * not having split horizontal windows. We only do this once per
   * potential multiple windows as the vertically split windows share
   * an adjuster.  If we try to work out position within myWindowZoom
   * we end up getting the wrong position the second ... times round
   * and not scrolling to the right position. 
   */
  if(window->curr_locking != ZMAP_WINLOCK_HORIZONTAL)
    {
      foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
      foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));
    }

  return;
}


/* try out the new zoom window.... */
void zMapWindowMove(ZMapWindow window, double start, double end)
{
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



void zMapWindowDestroy(ZMapWindow window)
{

  zMapDebug("%s", "GUI: in window destroy...\n") ;

  if (window->sequence)
    g_free(window->sequence) ;

  if (window->locked_display)
    unlockWindow(window) ;


  /* free the array of featureListWindows and the windows themselves */
  if (window->featureListWindows)
    {
      while (window->featureListWindows->len)
	{
	  GtkWidget *widget;

	  widget = g_ptr_array_index(window->featureListWindows, 0) ;
	  gtk_widget_destroy(widget);
	}

      g_ptr_array_free(window->featureListWindows, FALSE);
      window->featureListWindows = NULL ;
    }

  zmapWindowFToIDestroy(window->context_to_item) ;

  zmapWindowLongItemFree(window->long_items) ;

  gtk_widget_destroy(window->toplevel) ;

  g_free(window) ;
  
  return ;
}

/*! @} end of zmapwindow docs. */

/* 
 *
 * The whole idea of this function is to hide the border from the
 * user. In this way {s,g}etting the scroll region using this function
 * always returns the sequence coords and not the actual scroll region.
 * This is important when calling as 
 * zmapWindowScrollRegionTool(window, 
 *                            NULL, &(window->min_coord),
 *                            NULL, &(window->max_coord));
 * and trying to find reliably which part of the sequence we're showing
 * e.g.
 * x1 = x2 = y1 = y2 = 0.0;
 * zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);
 * y1 and y2 now have the displayed sequence start and end respectively.
 * useful for the scalebar and dna.
 *
 *
 *
 * call as zmapWindowScrollRegionTool(window, NULL, NULL, NULL, NULL)
 * will needlessly get and set the scroll region to exactly what it is now.

 * call as zmapWindowScrollRegionTool(window, NULL, -90000.00, NULL, 9000000000.00)
 * will, depending on zoom do eveything you expect!
 * i.e. do a get_scroll_region, filling in the NULLs with current values
 * and clamp the region within the seq or the 32K limit, whichever is
 * smaller in terms of pixels.

 * x1 = x2 = y1 = y2 = 0.0
 * call as zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2)
 * same as zmapWindowScrollRegionTool(window, NULL, NULL, NULL, NULL), 
 * but acts more like just a get_scroll_region to the caller!
 */

void zmapWindowScrollRegionTool(ZMapWindow window,
                                double *x1_inout, double *y1_inout,
                                double *x2_inout, double *y2_inout)
{
  ZMapWindowClampType clamp = ZMAP_WINDOW_CLAMP_INIT;
  double  x1,  x2,  y1,  y2;    /* New region coordinates */
  double wx1, wx2, wy1, wy2;    /* Current world coordinates */


  foo_canvas_get_scroll_region(FOO_CANVAS(window->canvas), /* OK */
                               &wx1, &wy1, &wx2, &wy2);

  /* Read the input */
  x1 = (x1_inout ? *x1_inout : wx1);
  x2 = (x2_inout ? *x2_inout : wx2);
  y1 = (y1_inout ? *y1_inout : wy1);
  y2 = (y2_inout ? *y2_inout : wy2);


  /* Catch this special case */
  if(x1 == 0.0 && x1 == x2 && x1 == y1 && x1 == y2)
    { 
      /* I think this is right. The return coords want to be sequence
       * coords, not actual world coords which will likely include a
       * border or two. */
      clamp = zmapWindowClampStartEnd(window, &wy1, &wy2); 
      x1 = wx1; x2 = wx2; y1 = wy1; y2 = wy2; 
    }
  else if (x1 == wx1 && x2 == wx2 && y1 == wy1 && y2 == wy2)
    {
      printf("Nothing's changed... returning ...\n");
      return;
    }
  else
    {
      /* BEGIN: main part of the setting of the scroll region */
      ZMapWindowVisibilityChangeStruct vis_change ;
      double zoom, border, tmp_top, tmp_bot;

      /* If the user called us with identical pairs of x or y coords
       * they will just get the current region coords for those pairs
       * back.  This is a bit hokey I guess*/
      if(x1 == x2){ x1 = wx1; x2 = wx2; }
      if(y1 == y2){ y1 = wy1; y2 = wy2; }

      zoom = zMapWindowGetZoomStatus(window) ;
      zmapWindowGetBorderSize(window, &border);

      zmapWindowLongItemCrop(window, x1, y1, x2, y2);

      clamp = zmapWindowClampStartEnd(window, &y1, &y2);
      y1   -= (tmp_top = ((clamp & ZMAP_WINDOW_CLAMP_START) ? border : 0.0));
      y2   += (tmp_bot = ((clamp & ZMAP_WINDOW_CLAMP_END)   ? border : 0.0));

      if(clamp ^ (ZMAP_WINDOW_CLAMP_END | ZMAP_WINDOW_CLAMP_START))
        {
          /* We've now broken the 32K/canvas_max_size limit. Need to check long items... */
          /*          printf("Broken 32K limit! ... Cropping Long Items.\n"); */
          /* We can't just do this as we need to check on zoom out! */
        }

      
      /* -----> and finally set the scroll region */
      foo_canvas_set_scroll_region(FOO_CANVAS(window->canvas), /* OK */
                                   x1, y1, x2, y2);

      /* -----> letting the window creator know what's going on */
      vis_change.zoom_status    = zoom;
      vis_change.scrollable_top = (y1 += tmp_top); /* should these be sequence clamped */
      vis_change.scrollable_bot = (y2 -= tmp_bot); /* or include the border? (SEQUENCE CLAMPED ATM) */

      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
      /*   END: main part of the setting of the scroll region */
    }

  /* Set the output */
  if(x1_inout)
    *x1_inout = x1;
  if(x2_inout)
    *x2_inout = x2;
  if(y1_inout)
    *y1_inout = y1;
  if(y2_inout)
    *y2_inout = y2;

  return ;
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapWindow_set_scroll_region(ZMapWindow window, double y1a, double y2a)
{
  ZMapWindowZoomControl control;
  double border, x1, x2, y1, y2, top, bot;
  ZMapWindowVisibilityChangeStruct vis_change ;
  ZMapWindowClampType clamp = ZMAP_WINDOW_CLAMP_INIT;

  control = window->zoom;
  zmapWindowSeq2CanExt(&y1a, &y2a);
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), 
                             &x1, &y1, &x2, &y2) ;
  
  /* Use this here as depending on whether this function get called
   * before or after the foo_canvas_set_pixels_per_unit_xy call alters 
   * whether we get the correct border or half/twice it when using
   * zMapDrawGetTextDimensions(). That is so long as 
   * zmapWindowZoomControlZoomByFactor() has been called!!!
   */
  zmapWindowGetBorderSize(window, &border);

  clamp = zmapWindowClampStartEnd(window, &y1a, &y2a);
  top = (y1a - ((clamp & ZMAP_WINDOW_CLAMP_START) ? border : 0));
  bot = (y2a + ((clamp & ZMAP_WINDOW_CLAMP_END)   ? border : 0));

  foo_canvas_set_scroll_region(window->canvas, /* OLD, so OK */
#ifdef RDS_THIS_SHOULD_BE_THIS
                               x1, top,
#else  /* NOT THIS HARD CODED 0.0  */
                               0.0, top,
#endif /* RDS_THIS_SHOULD_BE_THIS */
                               x2, bot);

  /* Call the visibility change callback to notify our caller that our zoom/position has
   * changed. */
  vis_change.zoom_status = zMapWindowGetZoomStatus(window) ;
  vis_change.scrollable_top = top ;
  vis_change.scrollable_bot = bot ;
  vis_change.strand = window->context_strand ;

  (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
    
  return ;
}
#endif


void zMapWindowUpdateInfoPanel(ZMapWindow window, ZMapFeature feature_arg, FooCanvasItem *item)
{
  ZMapWindowItemFeatureType type ;
  ZMapWindowItemFeature item_data ;
  ZMapFeature feature = NULL;
  char *subpart_text = NULL ;
  ZMapWindowSelectStruct select = {NULL} ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), ITEM_FEATURE_TYPE)) ;
  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);

  zMapAssert(feature_arg == feature);

  if (type == ITEM_FEATURE_CHILD)
    {
      item_data = g_object_get_data(G_OBJECT(item), ITEM_SUBFEATURE_DATA) ;
      zMapAssert(item_data) ;

      subpart_text = g_strdup_printf("  (%d %d)", item_data->start, item_data->end) ;
    }

  /* It would be nice for a user to be able to specify the format of this string. */
  select.primary_text = g_strdup_printf("%s : %s : %s   %s %d %d%s", 
				zMapFeatureType2Str(feature->type),
				zMapStyleGetName(zMapFeatureGetStyle(feature)),
				(char *)g_quark_to_string(feature->original_id),
				zMapFeatureStrand2Str(feature->strand),
				feature->x1,
				feature->x2,
				(subpart_text ? subpart_text : "")) ;

  select.secondary_text = select.primary_text;
  select.item = item ;
  
  (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

  if (subpart_text)
    g_free(subpart_text) ;
  g_free(select.primary_text) ;

  return;
}




/* We just dip into the foocanvas struct for some data, we use the interface calls where they
 * exist. */
void zmapWindowPrintCanvas(FooCanvas *canvas)
{
  double x1, y1, x2, y2 ;


  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);

  printf("Canvas stats:\n") ;

  printf("Zoom x,y:\n\t %f, %f\n", canvas->pixels_per_unit_x, canvas->pixels_per_unit_y) ;

  printf("Scroll region bounds:\n\t%f -> %f,  %f -> %f\n", x1, x2, y1, y2) ;

  zmapWindowPrintGroups(canvas) ;

  return ;
}


void zmapWindowPrintGroups(FooCanvas *canvas)
{
  FooCanvasGroup *root ;
  int indent ;
  
  printf("Groups:\n") ;

  root = foo_canvas_root(canvas) ;

  indent = 1 ;
  printGroup(root, indent) ;

  return ;
}


void zmapWindowPrintGroup(FooCanvasGroup *group)
{
  double x1, y1, x2, y2 ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(group), &x1, &y1, &x2, &y2) ;
  printf("Pos: %f, %f  Bounds: %f -> %f,  %f -> %f\n",
	 group->xpos, group->ypos, x1, x2, y1, y2) ;

  return ;
}






/*
 *  ------------------- Internal functions -------------------
 */


/* We will need to allow caller to specify a routine that gets called whenever the user
 * scrolls.....needed to update the navigator...... */
/* and we will need a callback for focus events as well..... */
/* I think probably we should insist on being supplied with a sequence.... */
/* NOTE that not all fields are initialised here as some need to be done when we draw
 * the actual features. */
static ZMapWindow myWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data,
				 GtkAdjustment *hadjustment, GtkAdjustment *vadjustment)
{
  ZMapWindow window ;
  GtkWidget *canvas ;

  /* No callbacks, then no window creation. */
  zMapAssert(window_cbs_G) ;

  zMapAssert(parent_widget && sequence && *sequence && app_data) ;

  window = g_new0(ZMapWindowStruct, 1) ;

  window->caller_cbs = window_cbs_G ;

  window->sequence = g_strdup(sequence) ;
  window->app_data = app_data ;
  window->parent_widget = parent_widget ;

  gdk_color_parse(ZMAP_WINDOW_ITEM_FILL_COLOUR, &(window->canvas_fill)) ;
  gdk_color_parse(ZMAP_WINDOW_ITEM_BORDER_COLOUR, &(window->canvas_border)) ;
  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(window->canvas_background)) ;
  gdk_color_parse("green", &(window->align_background)) ;

  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  //  window->zoom_factor = 0.0 ;
  //  window->zoom_status = ZMAP_ZOOM_INIT ;
  window->canvas_maxwin_size = ZMAP_WINDOW_MAX_WINDOW ;

  window->min_coord = window->max_coord = 0.0 ;

  /* Some things for window can be specified in the configuration file. */
  getConfiguration(window) ;

  /* Add a hash table to map features to their canvas items. */
  window->context_to_item = zmapWindowFToICreate() ;

  window->featureListWindows = g_ptr_array_new();


  /* Set up a scrolled widget to hold the canvas. NOTE that this is our toplevel widget. */
  window->toplevel = window->scrolled_window = gtk_scrolled_window_new(hadjustment, vadjustment) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), window->scrolled_window) ;


  g_signal_connect(GTK_OBJECT(window->scrolled_window), "size-allocate",
		   GTK_SIGNAL_FUNC(sizeAllocateCB), (gpointer)window) ;

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
  canvas = foo_canvas_new() ;
  window->canvas = FOO_CANVAS(canvas);

  gtk_container_add(GTK_CONTAINER(window->scrolled_window), canvas) ;


  gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &(window->canvas_background)) ;


  /* Make the canvas focussable, we want the canvas to be the focus widget of its "window"
   * otherwise keyboard input (i.e. short cuts) will be delivered to some other widget. */
  GTK_WIDGET_SET_FLAGS(canvas, GTK_CAN_FOCUS) ;


  window->zoom = zmapWindowZoomControlCreate(window);


  /* This is a general handler that does stuff like handle "click to focus", it gets run
   * _BEFORE_ any canvas item handlers (there seems to be no way with the current
   * foocanvas/gtk to get an event run _after_ the canvas handlers, you cannot for instance
   * just use  g_signal_connect_after(). */
  g_signal_connect(GTK_OBJECT(window->canvas), "event",
		   GTK_SIGNAL_FUNC(canvasWindowEventCB), (gpointer)window) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I need to think about whether this is any use, it doesn't get called unless an item
   * has already received the click, currently we have no use for this.... */

  /* Wierdly this doesn't get run _unless_ the click/event happens on a child of the
   * root first...sigh..... */
  g_signal_connect(GTK_OBJECT(foo_canvas_root(window->canvas)), "event",
		   GTK_SIGNAL_FUNC(canvasRootEventCB), (gpointer)window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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

  /* We want the canvas to be the focus widget of its "window" otherwise keyboard input
   * (i.e. short cuts) will be delivered to some other widget. We do this here because
   * I think we may need a widget window to exist for this call to work. */
  gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;


  return window ; 
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

  if(window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
    {
      adjust = 
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
      foo_canvas_get_scroll_offsets(window->canvas, &x, &y) ;
      y += adjust->page_size / 2 ;
      foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;
    }

  /* We don't zoom in further than is necessary to show individual bases or further out than
   * is necessary to show the whole sequence, or if the sequence is so short that it can be
   * shown at max. zoom in its entirety. */
  if (!zmapWindowZoomControlZoomByFactor(window, zoom_factor))
    {
      /* Currently we don't call the visibility change callback because nothing has changed,
       * we may want to revisit this decision. */
      return ;
    }
  else
    {
      FooCanvasGroup *canvas_root_group ;

      /* Calculate the extent of the new span, new span must not exceed maximum X window size
       * but we must display as much of the sequence as we can for zooming out. */
      foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);
      new_canvas_span = zmapWindowZoomControlLimitSpan(window, y1, y2);
      
      y1 = curr_pos - (new_canvas_span / 2) ;
      y2 = curr_pos + (new_canvas_span / 2) ;
      
      zmapWindowClampSpan(window, &y1, &y2);
      
      /* Set the new scroll_region and the new zoom. N.B. may need to do a "freeze" of the canvas here
       * to avoid a double redraw....but that might never happen actually, depends how much there is
       * in the Xlib buffer so not lets worry about it. */
      
      //  foo_canvas_set_scroll_region(window->canvas, x1, top, x2, bot) ;
      //  zmapWindow_set_scroll_region(window, top, bot);
      foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, zMapWindowGetZoomFactor(window)) ;
      
      //zmapWindowZoomControlGetScrollRegion(window, &x1, &y1, &x2, &y2);
        
      /* Set the scroll region. */
      zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2);

      /* Now we've actually done the zoom on the canvas we can
       * redraw/show/hide everything that's zoom dependent.
       * E.G. Text, which is separated differently @ different zooms otherwise.
       *      zoom dependent features, magnification dependent columns 
       */

      /* Firstly the scale bar, which will be changed soon. */
      zmapWindowDrawScaleBar(window, y1, y2);

      zmapWindowDrawZoom(window);
      
      zmapWindowLongItemCrop(window, x1, y1, x2, y2); /* Call this again because of backgrounds :(
						       */

      /* THIS IS A HACK....AS WE DO IT ABOVE ALSO, THE CALL ABOVE SHOULD GET US THE SIZE
       * WITHOUT CHANGING THE CANVAS STATE OTHERWISE WE REDRAW THE CANVAS TWICE.... */
      canvas_root_group = foo_canvas_root (window->canvas) ;
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_root_group), &x1, NULL, &x2, NULL) ;
      zmapWindowScrollRegionTool(window, &x1, &y1, &x2, &y2) ;
    }

  if(window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
    {
      foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
      foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));
    }
  
  return ;
}

/* Move the window to a new part of the canvas, we need this because when the window is
 * zoomed in, it may not extend over the whole canvas so this kind of alternative scrolling.
 * is needed to reach parts of the canvas not currently mapped. */
static void myWindowMove(ZMapWindow window, double start, double end)
{
  FooCanvasGroup *super_root = NULL;
  double x1, x2;
  x1 = x2 = 0.0;
  /* Clamp the start/end. */
  if (start < window->min_coord)
    start = window->min_coord ;
  if (end > window->max_coord)
    end = window->max_coord ;

  zmapWindowScrollRegionTool(window, &x1, &start, &x2, &end);

  if((super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item, 0,0,0,0,0))))
    zmapWindowContainerMoveEvent(super_root, window);

  /* need to redo some of the large objects.... */
  zmapWindowLongItemCrop(window, x1, start, x2, end);

  zmapWindowDrawScaleBar(window, start, end);
  foo_canvas_update_now(window->canvas) ;

  return ;
}





/* This function resets the canvas to be empty, all of the canvas items we drew
 * are destroyed and all the associated resources free'd.
 * 
 * NOTE that this does not touch the feature context. */
static void resetCanvas(ZMapWindow window)
{

  zMapAssert(window) ;


  /* There is code here that should be shared with zmapwindowdestroy....
   * BUT NOTE THAT SOME THINGS ARE NOT SHARED...e.g. RECREATION OF THE FEATURELISTWINDOWS
   * ARRAY.... */

  
  if (window->long_items)
    {
      zmapWindowLongItemFree(window->long_items) ;
      window->long_items = NULL ;
    }

  if (window->feature_root_group)
    {
      zmapWindowContainerDestroy(window->feature_root_group) ;
      window->feature_root_group = NULL ;
    }

  if (window->scaleBarGroup)
    {
      gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));
      window->scaleBarGroup = NULL ;
    }


  if (window->canvas)
    foo_canvas_set_scroll_region(window->canvas, 0.0, 0.0, 100.0, 100.0) ;
							    /* Seems to be default size....?? */

  zmapWindowFToIDestroy(window->context_to_item) ;
  window->context_to_item = zmapWindowFToICreate() ;


  /* free the array of featureListWindows and the windows themselves */
  if (window->featureListWindows)
    {
      /* Slightly tricky here, when the list widget gets destroyed it removes itself
       * from the array so the array shrinks by one, hence we just continue to access
       * the first element until all elements are gone. */
      while (window->featureListWindows->len)
	{
	  GtkWidget *widget ;

	  widget = g_ptr_array_index(window->featureListWindows, 0) ;

	  gtk_widget_destroy(widget);
	}
    }
  
  g_list_free(window->focusItemSet);  
  window->focusItemSet = NULL ;
  window->alignment_start = 0 ;

  return ; 
}



/* Recursive function to print out all the child groups of the supplied group. */
static void printGroup(FooCanvasGroup *group, int indent)
{
  int i ;
  GList *list ;

  for (i = 0 ; i < indent ; i++)
    printf("\t") ;

  /* Print this group. */
  zmapWindowPrintGroup(group) ;

  /* Print all the child groups of this group. */
  list = g_list_first(group->item_list) ;
  do
    {
      if (FOO_IS_CANVAS_GROUP(list->data))
	printGroup(FOO_CANVAS_GROUP(list->data), indent + 1) ;
    }
  while ((list = g_list_next(list))) ;

  return ;
}



/* Moves can either be of the scroll bar within the scrolled window or of where the whole
 * scrolled region is within the canvas. */
static void moveWindow(ZMapWindow window, guint state, guint keyval)
{

  if ((state & GDK_CONTROL_MASK) && (state & GDK_MOD1_MASK))
    changeRegion(window, keyval) ;
  else
    scrollWindow(window, state, keyval) ;

  return ;
}



/* Move the canvas within its current scroll region, i.e. this is exactly like the user scrolling
 * the canvas via the scrollbars. */
static void scrollWindow(ZMapWindow window, guint state, guint keyval)
{
  enum {OVERLAP_FACTOR = 10} ;
  int x_pos, y_pos ;
  int incr ;

  /* Retrieve current scroll position. */
  foo_canvas_get_scroll_offsets(window->canvas, &x_pos, &y_pos) ;

  switch (keyval)
    {
    case GDK_Page_Up:
    case GDK_Page_Down:
      {
	GtkAdjustment *adjust ;

	adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

	if (state & GDK_CONTROL_MASK)
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
	if (state & GDK_CONTROL_MASK)
	  foo_canvas_w2c(window->canvas,
			 0.0, 1000,//(double)window->major_scale_units,
			 NULL, &incr) ;
	else
	  foo_canvas_w2c(window->canvas,
			 0.0, 100,//(double)window->minor_scale_units,
			 NULL, &incr) ;

	if (keyval == GDK_Up)
	  y_pos -= incr ;
	else
	  y_pos += incr ;

	break ;
      }
    case GDK_Left:
    case GDK_Right:
      {
	if (keyval == GDK_Left)
	  x_pos-- ;
	else
	  x_pos++ ;

	break ;
      }
    }

  /* No need to clip to start/end here, this call will do it for us. */
  foo_canvas_scroll_to(window->canvas, x_pos, y_pos) ;

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
#ifdef RDS_DONT_INCLUDE
  ZMapWindowVisibilityChangeStruct vis_change ;
#endif /* RDS_DONT_INCLUDE */
  /* THIS FUNCTION NEEDS REFACTORING SLIGHTLY */
  foo_canvas_get_scroll_region(window->canvas, /* ARRRRRRRRRRRRGH!!!! */
                               &x1, &y1, &x2, &y2);

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

      //      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2) ;
      //zmapWindow_set_scroll_region(window, y1, y2);

      zmapWindowScrollRegionTool(window, NULL, &y1, NULL, &y2);
      
    }

  return ;
}


/* widget is the scrolled_window, user_data is the zmapWindow */
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("sizeAllocateCB: x: %d, y: %d, height: %d, width: %d\n", 
	 alloc->x, alloc->y, alloc->height, alloc->width); 
#endif

  if (window->seqLength) /* when window first drawn, seqLength = 0 */
    {
      zmapWindowZoomControlHandleResize(window);
      /* call the function given us by zmapView.c to set zoom buttons for this window */
      /*      (*(window_cbs_G->setZoomButtons))(realiseData->window, realiseData->view);*/
    }
  else if(window->zoom == NULL)
    /* First time through we need to create the zoom controller */
    zMapWarning("%s\n", "sizeAllocate Should already have a zoom control.");
    //    window->zoom = zmapWindowZoomControlCreate(window);

  return ;
}



/* Because we can't depend on the canvas having a valid height when it's been realized,
 * we have to detect the invalid height and attach this handler to the canvas's 
 * expose_event, such that when it does get called, the height is valid.  Then we
 * disconnect it to prevent it being triggered by any other expose_events. 
 */
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *expose, gpointer user_data)
{
  /* widget is the canvas, user_data is the realizeData structure */
  RealiseData realiseData = (RealiseDataStruct*)user_data;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("exposeHandlerCB\n");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* call the function given us by zmapView.c to set zoom_status
   * for all windows in this view. */
  (*(window_cbs_G->setZoomStatus))(realiseData->window, realiseData->window->app_data, NULL);

  zMapAssert(GTK_WIDGET_REALIZED(widget));

  /* disconnect signal handler before calling sendClientEvent otherwise when splitting
   * windows, the scroll-to which positions the new window in the same place on the
   * sequence as the previous one, will trigger another call to this function.  */
  g_signal_handler_disconnect(G_OBJECT(widget), realiseData->window->exposeHandlerCB);

  sendClientEvent(realiseData->window, realiseData->feature_sets) ;

  realiseData->window->exposeHandlerCB = 0 ;

  g_free(realiseData);

  return FALSE ;  /* ie allow any other callbacks to run as well */
}



/* This routine sends a synthesized event to alert the GUI that it needs to
 * do some work and supplies the data for the GUI to process via the event struct. */
static void sendClientEvent(ZMapWindow window, FeatureSets feature_sets)
{
  GdkEventClient event ;
  gint ret_val = 0 ;
  zmapWindowData window_data ;


  /* Set up struct to be passed to our callback. */
  window_data = g_new0(zmapWindowDataStruct, 1) ;
  window_data->window = window ;
  window_data->data = feature_sets ;

  event.type = GDK_CLIENT_EVENT ;
  event.window = NULL ;					    /* no window generates this event. */
  event.send_event = TRUE ;				    /* we sent this event. */
  event.message_type = window->zmap_atom ;		    /* This is our id for events. */
  event.data_format = 8 ;				    /* Not sure about data format here... */

  /* Load the pointer value, not what the pointer points to.... */
  {
    void **dummy ;

    dummy = (void *)&window_data ;
    memmove(&(event.data.b[0]), dummy, sizeof(void *)) ;
  }

  gtk_signal_emit_by_name(GTK_OBJECT(window->toplevel), "client_event",
			  &event, &ret_val) ;

  return ;
}





/* Called when gtk detects the event sent by signalDataToGUI(), this routine calls
 * the data display routines of ZMap. */
static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  gboolean event_handled = FALSE ;
#ifdef RDS_DONT_INCLUDE
  ZMapWindow window = (ZMapWindow)cb_data ;
#endif
  if (event->type != GDK_CLIENT_EVENT)
    zMapLogFatal("%s", "dataEventCB() received non-GdkEventClient event") ;
  
  if (event->send_event == TRUE && event->message_type == gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      FeatureSets feature_sets ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      gpointer data = NULL ;
      ZMapFeatureContext feature_context ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      ZMapFeatureContext diff_context ;


      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;

      window = window_data->window ;
      feature_sets = window_data->data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      data = (gpointer)(window_data->data) ;
      feature_context = (ZMapFeatureContext)data ;	    /* Data from a server... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* ****Remember that someone needs to free the data passed over....****  */


      /* We need to validate the feature_context at this point, we should be sure it contains
       * some features before continuing. */

      if (feature_sets->new_features)
	diff_context = feature_sets->new_features ;
      else
	diff_context = feature_sets->current_features ;


      /* Draw the features on the canvas */
      zmapWindowDrawFeatures(window, feature_sets->current_features, diff_context) ;


      g_free(feature_sets) ;
      g_free(window_data) ;				    /* Free the WindowData struct. */

      event_handled = TRUE ;
    }
  else
    {
      zMapLogCritical("%s", "unknown client event in dataEventCB() handler\n") ;

      event_handled = FALSE ;
    }


  return event_handled ;
}


/* Read logging information from the configuration, note that we read _only_ the first
 * logging stanza found in the configuration, subsequent ones are not read. */
static gboolean getConfiguration(ZMapWindow window)
{
  gboolean result = FALSE ;
  ZMapConfig config ;
  ZMapConfigStanzaSet window_list = NULL ;
  ZMapConfigStanza window_stanza ;
  char *window_stanza_name = ZMAP_WINDOW_CONFIG ;
  ZMapConfigStanzaElementStruct window_elements[] = {{"canvas_maxsize", ZMAPCONFIG_INT, {NULL}},
						     {"canvas_maxbases", ZMAPCONFIG_INT, {NULL}},
						     {"keep_empty_columns", ZMAPCONFIG_BOOL, {NULL}},
						     {NULL, -1, {NULL}}} ;

  zMapConfigGetStructInt(window_elements, "canvas_maxsize") = ZMAP_WINDOW_MAX_WINDOW ;
  zMapConfigGetStructBool(window_elements, "keep_empty_columns") = FALSE ;

  if ((config = zMapConfigCreate()))
    {
      window_stanza = zMapConfigMakeStanza(window_stanza_name, window_elements) ;

      if (zMapConfigFindStanzas(config, window_stanza, &window_list))
	{
	  ZMapConfigStanza next_window ;
	  
	  /* Get the first window stanza found, we will ignore any others. */
	  next_window = zMapConfigGetNextStanza(window_list, NULL) ;
	  
	  window->canvas_maxwin_size = zMapConfigGetElementInt(next_window, "canvas_maxsize") ;

	  window->canvas_maxwin_bases = zMapConfigGetElementInt(next_window, "canvas_maxbases") ;

	  window->keep_empty_cols = zMapConfigGetElementBool(next_window, "keep_empty_columns") ;

	  
	  zMapConfigDeleteStanzaSet(window_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(window_stanza) ;
      
      zMapConfigDestroy(config) ;

      result = TRUE ;
    }

  return result ;
}


/* This gets run _BEFORE_ any of the canvas item handlers which is good because we can use it
 * handle more general events such as "click to focus" etc. */
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  gboolean event_handled ;
  ZMapWindow window = (ZMapWindow)data ;
  static double origin_x, origin_y; /* The world coords of the source of the button 1 event */
  static gboolean dragging = FALSE, guide = FALSE;

  double wx, wy; /* These hold the current world coords of the event */

  event_handled = FALSE ; /* FALSE means other handlers run. */

  /* PLEASE be very careful when altering this function, as I've
   * already messed stuff up when working on it! The event_handled
   * boolean _SHOULD_ be set to true any time we handle the event.
   * While this sounds obvious I fell over it when implementing the
   * motion as well as button down and release. If there is a track 
   * of events, such as button/key down .. motion .. button release
   * then the event_handled should be true for the whole of the life
   * of the track of events.  All of the statics above could/probably
   * should be replaced with a stuct... please think about this if 
   * adding any more!
   */

  /* We need to check that canvas is mapped here (slow connections) */

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	printf("GENERAL event handler - CLICK\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* We want the canvas to be the focus widget of its "window" otherwise keyboard input
	 * (i.e. short cuts) will be delivered to some other widget. */
	gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;

	/* Report to our parent that we have been clicked in, we change "click" to "focus"
	 * because that is what this is for.... */
	(*(window_cbs_G->focus))(window, window->app_data, NULL) ;

	
	/* Button 2 is handled, we centre on that position, 1 and 3 are passed on as they may
	 * be clicks on canvas items/columns. */
	switch (but_event->button)
	  {
	    /* We don't do anything for button 1 or any buttons > 3. */
	  default:
	  case 1:
	    {
              foo_canvas_window_to_world(window->canvas, 
                                         but_event->x, but_event->y, 
                                         &origin_x, &origin_y);
              event_handled = FALSE; /* false so item events
                                      * happen. Unless we _ARE_
                                      * handling here! 
                                      */
              if(but_event->state & GDK_SHIFT_MASK)
                {
                  guide = TRUE;
                  if(!window->horizon_guide_line)
                    window->horizon_guide_line = zMapDrawHorizonCreate(window->canvas);
                  if(!window->tooltip)
                    window->tooltip = zMapDrawToolTipCreate(window->canvas);
                  zMapDrawHorizonReposition(window->horizon_guide_line, origin_y);
                  event_handled = TRUE ; /* We _ARE_ handling */
                }
              else if(but_event->state & GDK_CONTROL_MASK)
                {
                  dragging = TRUE;  /* we can be dragging */
                  if(!window->rubberband)
                    window->rubberband = zMapDrawRubberbandCreate(window->canvas);
                  event_handled = TRUE ; /* We _ARE_ handling */
                }
	      break ;
	    }
	  case 2:
	    {
	      zMapWindowScrollToWindowPos(window, but_event->y) ;

	      event_handled = TRUE ;
	      break ;
	    }
	  case 3:
	    {
	      /* Oh dear, we would like to do a general back ground menu here but can't as we need
	       * to pass the event along in case it goes to a canvas item....aggghhhh */

	      event_handled = FALSE ;
	      break ;
	    }
	  }

	break ;
      }
    case GDK_KEY_PRESS:
      {
	GdkEventKey *key_event = (GdkEventKey *)event ;

	switch (key_event->keyval)
	  {
	  case GDK_Page_Up:
	  case GDK_Page_Down:
	  case GDK_Up:
	  case GDK_Down:
	  case GDK_Left:
	  case GDK_Right:
	    {
	      moveWindow(window, key_event->state, key_event->keyval) ;
	      event_handled = TRUE ;
	      break ;
	    }
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      case GDK_Home:
		kval = HOME_KEY ; break ;
	      case GDK_End:
		kval = END_KEY ; break ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  default:
	    event_handled = FALSE ;
	    break ;
	  }
	
	break ;
      }
    case GDK_MOTION_NOTIFY:
      {
	GdkEventMotion *mot_event = (GdkEventMotion *)event ;
        event_handled = FALSE ;

        if(!(mot_event->state & GDK_BUTTON1_MASK))
          break;

        /* work out the world of where we are */
        foo_canvas_window_to_world(window->canvas,
                                   mot_event->x, mot_event->y,
                                   &wx, &wy);
        if(dragging)
          {
            /* I wanted to change the cursor for this, 
             * but foo_canvas_item_grab/ungrab specifically the ungrab didn't work */
            zMapDrawRubberbandResize(window->rubberband, origin_x, origin_y, wx, wy);
            event_handled = TRUE; /* We _ARE_ handling */
          }
        else if(guide)
          {
            int bp = 0;
            double y1, y2;
            foo_canvas_get_scroll_region(window->canvas, /* ok, but like to change */
                                         NULL, &y1, NULL, &y2);
            zmapWindowClampStartEnd(window, &y1, &y2); /* errrr. I think ScrollRegionTool does this */
            zMapDrawHorizonReposition(window->horizon_guide_line, wy);
            /* We floor the value here as it works with the way we draw our bases. */
            /* This test is FLAWED ATM it needs to test for displayed seq start & end */
            if(y1 <= wy && y2 >= wy)
              {
                char *tip = NULL;
                bp = (int)floor(wy);
                tip = g_strdup_printf("%d bp", bp);
                zMapDrawToolTipSetPosition(window->tooltip, wx, wy, tip);
                g_free(tip);
              }
            else
              foo_canvas_item_hide(FOO_CANVAS_ITEM(window->tooltip));
            event_handled = TRUE ; /* We _ARE_ handling */
          }
        break;
      }
    case GDK_BUTTON_RELEASE:
      {
        event_handled = FALSE;
        if(dragging)
          {
            foo_canvas_item_hide(window->rubberband);
            zoomToRubberBandArea(window);
            event_handled = TRUE; /* We _ARE_ handling */
          }
        else if(guide)
          {
            foo_canvas_item_hide(window->horizon_guide_line);
            foo_canvas_item_hide(FOO_CANVAS_ITEM( window->tooltip ));
            event_handled = TRUE; /* We _ARE_ handling */
          }
        dragging = guide = FALSE;
        
        break;
      }
    default:
      {
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
}

#define ZOOM_SENSITIVITY 5.0
static void zoomToRubberBandArea(ZMapWindow window)
{
  GtkAdjustment *v_adjuster ;
  /* The bands bounds */
  double rootx1, rootx2, rooty1, rooty2;
  /* size of bound area */
  double ydiff;
  double area_middle;
  int win_height, canvasx, canvasy, beforex, beforey;
  /* Zoom factor */
  double zoom_by_factor, target_zoom_factor, current;
  double wx1, wx2, wy1, wy2;

  if(!window->rubberband)
    return ;

  v_adjuster = 
    gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window));
  win_height = v_adjuster->page_size;

  /* this returns world coords */
  foo_canvas_item_get_bounds(window->rubberband, &rootx1, &rooty1, &rootx2, &rooty2);
  /* make them canvas so we can scroll there */
  foo_canvas_w2c(window->canvas, rootx1, rooty1, &beforex, &beforey);

  /* work out the zoom factor to show all the area (vertically) and
     calculate how much we need to zoom by to achieve that. */
  ydiff = rooty2 - rooty1;
  if(ydiff < ZOOM_SENSITIVITY)
    return ;
  area_middle = rooty1 + (ydiff / 2.0); /* So we can make this the centre later */
  target_zoom_factor = (double)(win_height / ydiff);
  current        = zMapWindowGetZoomFactor(window);
  zoom_by_factor = (target_zoom_factor / current);


  /* actually do the zoom */
  zMapWindowZoom(window, zoom_by_factor);

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
  if(rooty1 > wy1 && rooty2 < wy2)
    {                           /* We're still in the same area, */
      foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);
      if(beforey != canvasy)
        foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
    }
  else
    {                           /* This takes a lot of time.... */
      double half_win_span = (window->canvas_maxwin_size / 2.0);
      double min_seq = area_middle - half_win_span;
      double max_seq = area_middle + half_win_span - 1;
      /* unfortunately freeze/thaw child-notify doesn't stop flicker */
      /* can we do something else to make it busy?? */
      zMapWindowMove(window, min_seq, max_seq);
      foo_canvas_w2c(window->canvas, rootx1, rooty1, &canvasx, &canvasy);
      foo_canvas_scroll_to(FOO_CANVAS(window->canvas), canvasx, canvasy);
    }
  return ;
}

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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {

	printf("ROOT group event handler - CLICK\n") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* Call the callers routine for button clicks. */
	window_cbs_G->click(window, window->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	
	event_handled = FALSE ;
	break ;
      }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    case GDK_BUTTON_RELEASE:
      {
	printf("GENERAL canvas event handler: button release\n") ;

	/* Call the callers routine for button clicks. */
	window_cbs_G->click(window, window->app_data, NULL) ;


	
	event_handled = FALSE ;
	break ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    default:
      {
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
}

/* NOTE: This routine only gets called when the canvas widgets parent requests a resize, not when
 * the canvas changes its own size through zooming.
 * 
 * We need to take action whether the canvas window changes size OR the underlying bin_window
 * where all the features get drawn to, changes size. In either case there are changes that
 * must be made to zoomControl and to the user interface, e.g. if the canvas window gets smaller
 * then we need to allow the user to be able to zoom out more.
 * 
 * Notes:
 * This is essentially a time when visibility may have changed or need to be
 * changed. zmapZoomControlHandleResize below works out whether the
 * zoom factor needs changing based on the window height.  Currently
 * this calls zMapWindowZoom (if required).
 *
 */
static void canvasSizeAllocateCB(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data ;
  FooCanvas *canvas = FOO_CANVAS(widget) ;
  GtkLayout *layout = &(canvas->layout) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* to debug insert calls before after the below code like this: */
  printWindowSizeDebug("PRE", window, widget, allocation) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (window->window_height != widget->allocation.height
      || window->canvas_height != layout->height)
    {
      if (window->canvas_width != 0 && window->canvas_height != 0)
	{
	  ZMapWindowVisibilityChangeStruct vis_change ;
	  double start, end;

	  zmapWindowZoomControlHandleResize(window);

	  foo_canvas_get_scroll_region(window->canvas, NULL, &start, NULL, &end);
	  vis_change.zoom_status    = zMapWindowGetZoomStatus(window) ;
	  vis_change.scrollable_top = start ;
	  vis_change.scrollable_bot = end ;
	  (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
	}

      window->window_width  = widget->allocation.width ;
      window->window_height = widget->allocation.height ;
      window->canvas_width  = layout->width ;
      window->canvas_height = layout->height ;
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
	  unlockWindow(window) ;

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
  zMapAssert(!window->locked_display && window->curr_locking == ZMAP_WINLOCK_NONE
	     && !window->sibling_locked_windows) ;

  window->locked_display = TRUE ;
  window->curr_locking = window_locking ;

  window->sibling_locked_windows = g_hash_table_new(g_direct_hash, g_direct_equal) ;
  g_hash_table_insert(window->sibling_locked_windows, window, window) ;

  return ;
}


static void copyLockWindow(ZMapWindow original_window, ZMapWindow new_window)
{
  zMapAssert(original_window->locked_display && original_window->curr_locking != ZMAP_WINLOCK_NONE
	     && original_window->sibling_locked_windows) ;
  zMapAssert(!new_window->locked_display && new_window->curr_locking == ZMAP_WINLOCK_NONE
	     && !new_window->sibling_locked_windows) ;

  new_window->locked_display = original_window->locked_display ;
  new_window->curr_locking = original_window->curr_locking ;
  new_window->sibling_locked_windows = original_window->sibling_locked_windows ;
  g_hash_table_insert(new_window->sibling_locked_windows, new_window, new_window) ;

  return ;
}


static void unlockWindow(ZMapWindow window)
{
  gboolean removed ;

  zMapAssert(window->locked_display && window->curr_locking != ZMAP_WINLOCK_NONE
	     && window->sibling_locked_windows) ;


  removed = g_hash_table_remove(window->sibling_locked_windows, window) ;
  zMapAssert(removed) ;

  if (!g_hash_table_size(window->sibling_locked_windows))
    {
      g_hash_table_destroy(window->sibling_locked_windows) ;
    }
  else
    {
      /* we only need to allocate a new adjuster if the hash table is not empty...otherwise we can
       * keep our existing adjuster. */
      GtkAdjustment *adjuster ;

      if (window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
	adjuster = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;
      else
	adjuster = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

      adjuster = copyAdjustmentObj(adjuster) ;

      if (window->curr_locking == ZMAP_WINLOCK_HORIZONTAL)
	gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window), adjuster) ;
      else
	gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window), adjuster) ;
    }

  window->locked_display= FALSE ;
  window->curr_locking = ZMAP_WINLOCK_NONE ;
  window->sibling_locked_windows = NULL ;

  return ;
}


static GtkAdjustment *copyAdjustmentObj(GtkAdjustment *orig_adj)
{
  GtkAdjustment *copy_adj ;

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
  LockedDisplay locked_data = (LockedDisplay)user_data ;
  ZMapWindow window = (ZMapWindow)key ;

  if(locked_data->type == ZMAP_LOCKED_ZOOMING)
    myWindowZoom(window, locked_data->zoom_factor, locked_data->position) ;
  else if(locked_data->type == ZMAP_LOCKED_MOVING)
    myWindowMove(window, locked_data->start, locked_data->end);
  else
    zMapWarning("%s", "locked display did not have a type...");

  return ;
}



static void printWindowSizeDebug(char *prefix, ZMapWindow window,
				 GtkWidget *widget, GtkAllocation *allocation)
{
  FooCanvas *canvas = FOO_CANVAS(widget) ;
  GtkLayout *layout = &(canvas->layout) ;

  printf("%s ----\n", prefix) ;

  printf("alloc_req: %d, %d\tallocation_widg: %d, %d\twindow: %d, %d\n",
	 allocation->width, allocation->height,
	 widget->allocation.width, widget->allocation.height,
	 window->window_width, window->window_height) ;

  printf("layout: %d, %d\tcanvas: %d, %d\n",
	 layout->width, layout->height,
	 window->canvas_width, window->canvas_height) ;

  printf("scroll: %f, %f, %f, %f\t\t%f, %f\n\n",
	 canvas->scroll_x1, canvas->scroll_y1,
	 canvas->scroll_x2, canvas->scroll_y2,
	 floor ((canvas->scroll_x2 - canvas->scroll_x1) * canvas->pixels_per_unit_x + 0.5),
	 floor ((canvas->scroll_y2 - canvas->scroll_y1) * canvas->pixels_per_unit_y + 0.5)) ;

  return ;
}
