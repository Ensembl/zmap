/*  File: zmapWindow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Provides interface functions for controlling a data
 *              display window.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jan  7 13:36 2005 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.57 2005-01-10 09:56:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>


/* This struct is used to pass data to realizeHandlerCB
 * via the g_signal_connect on the canvas's expose_event. */
typedef struct _RealizeDataStruct {
  ZMapWindow          window;
  void               *view;
  ZMapFeatureContext  current_features;
  ZMapFeatureContext  new_features;
  GData              *types;
} RealizeDataStruct, *RealizeData;



static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean canvasEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;

static void clickCB             (ZMapWindow window, void *caller_data, void *window_data) ;
static gboolean rightClickCB    (ZMapWindow window, ZMapFeatureItem featureItem);

static void hideUnhideColumns   (ZMapWindow window);
static gboolean getConfiguration(ZMapWindow window) ;
static void sendClientEvent     (ZMapWindow window, ZMapFeatureContext current_features,
				 ZMapFeatureContext new_features, GData *types);


static void moveWindow(ZMapWindow window, guint state, guint keyval) ;
static void scrollWindow(ZMapWindow window, guint state, guint keyval) ;
static void changeRegion(ZMapWindow window, guint keyval) ;



/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapWindowCallbacks window_cbs_G = NULL ;

/* Callbacks to us from the level below, ie zmapWindowDrawFeatures */
ZMapFeatureCallbacksStruct feature_cbs_G = { clickCB, rightClickCB };



/* This routine must be called just once before any other windows routine, it is undefined
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 * 
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of windows terminate routine. */
void zMapWindowInit(ZMapWindowCallbacks callbacks)
{
  zMapAssert(!window_cbs_G) ;

  zMapAssert(callbacks && callbacks->scroll && callbacks->click && callbacks->visibilityChange
	     && callbacks->destroy) ;


  window_cbs_G = g_new0(ZMapWindowCallbacksStruct, 1) ;

  window_cbs_G->scroll  = callbacks->scroll ;
  window_cbs_G->click   = callbacks->click ;
  window_cbs_G->setZoomStatus = callbacks->setZoomStatus;

  window_cbs_G->visibilityChange = callbacks->visibilityChange ;

  window_cbs_G->destroy = callbacks->destroy ;

  zmapFeatureInit(&feature_cbs_G);

  return ;
}


/* We will need to allow caller to specify a routine that gets called whenever the user
 * scrolls.....needed to update the navigator...... */
/* and we will need a callback for focus events as well..... */
/* I think probably we should insist on being supplied with a sequence.... */
ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data)
{
  ZMapWindow window ;
  GtkWidget *canvas ;
  GdkColor color;

  /* No callbacks, then no window creation. */
  zMapAssert(window_cbs_G) ;

  zMapAssert(parent_widget && sequence && *sequence && app_data) ;

  window = g_new0(ZMapWindowStruct, 1) ;
  window->sequence = g_strdup(sequence) ;
  window->app_data = app_data ;
  window->parent_widget = parent_widget ;

  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  window->zoom_factor = 0.0 ;
  window->zoom_status = ZMAP_ZOOM_INIT ;
  window->canvas_maxwin_size = ZMAP_WINDOW_MAX_WINDOW ;

  window->columns = g_ptr_array_new();

  /* Some things for window can be specified in the configuration file. */
  getConfiguration(window) ;

  window->featureListWindows = g_ptr_array_new();
  g_datalist_init(&(window->featureItems));
  g_datalist_init(&(window->longItems));

  /* Set up a scrolled widget to hold the canvas. NOTE that this is our toplevel widget. */
  window->toplevel = window->scrolledWindow = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), window->scrolledWindow) ;
  g_signal_connect(GTK_OBJECT(window->scrolledWindow), "size-allocate",
		   GTK_SIGNAL_FUNC(sizeAllocateCB), (gpointer)window) ;

  /* This handler receives the feature data from the threads. */
  gtk_signal_connect(GTK_OBJECT(window->toplevel), "client_event", 
		     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;

  /* always show scrollbars, however big the display */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->scrolledWindow),
				 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS) ;

  /* Create the canvas, add it to the scrolled window so all the scrollbar stuff gets linked up
   * and set the background to be white. */
  canvas = foo_canvas_new() ;
  gtk_container_add(GTK_CONTAINER(window->scrolledWindow), canvas) ;
  gdk_color_parse("white", &color);
  gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &color);
  window->canvas = FOO_CANVAS(canvas);

  /* Make the canvas focussable, we want the canvas to be the focus widget of its "window"
   * otherwise keyboard input (i.e. short cuts) will be delivered to some other widget. */
  GTK_WIDGET_SET_FLAGS(canvas, GTK_CAN_FOCUS) ;

  /* I've tried to attach this after the default signal handler but it still seems to intercept
   * any calls to canvas items... */
  g_signal_connect_after(GTK_OBJECT(window->canvas), "event",
			 GTK_SIGNAL_FUNC(canvasEventCB), (gpointer)window) ;


  /* you have to set the step_increment manually or the scrollbar arrows don't work.*/
  GTK_LAYOUT(canvas)->vadjustment->step_increment = ZMAP_WINDOW_STEP_INCREMENT;
  GTK_LAYOUT(canvas)->hadjustment->step_increment = ZMAP_WINDOW_STEP_INCREMENT;
  GTK_LAYOUT(canvas)->vadjustment->page_increment = ZMAP_WINDOW_PAGE_INCREMENT;

  /* Get everything sized up....I don't know if this is really needed here or not. */
  gtk_widget_show_all(window->parent_widget) ;

  return(window) ; 
}


ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, char *sequence, 
			  void *app_data, ZMapWindow old)
{
  ZMapWindow new = zMapWindowCreate(parent_widget, sequence, app_data);
  int x, y;

  if (new)
    {
      new->zoom_factor        = old->zoom_factor;
      new->zoom_status        = old->zoom_status = ZMAP_ZOOM_MID;
      new->max_zoom           = old->max_zoom;
      new->canvas_maxwin_size = old->canvas_maxwin_size;
      new->border_pixels      = old->border_pixels;
      new->DNAwidth           = old->DNAwidth;
      new->text_height        = old->text_height;
      new->seqLength          = old->seqLength;
      new->seq_start          = old->seq_start;
      new->focusFeature       = old->focusFeature;
      new->focusType          = old->focusType;
      new->typeName           = old->typeName;
      new->focusQuark         = old->focusQuark;

      foo_canvas_get_scroll_offsets(old->canvas, &x, &y);
      foo_canvas_c2w(old->canvas, x, y, &new->current_x, &new->current_y);
      foo_canvas_get_scroll_region(old->canvas, &new->scroll_x1, &new->scroll_y1,
				   &new->scroll_x2, &new->scroll_y2);
    }
			      
  return new;
}



double zmapWindowCalcZoomFactor(ZMapWindow window)
{
  double zoom_factor = GTK_WIDGET(window->canvas)->allocation.height / window->seqLength;

  return zoom_factor;
}



void zMapWindowSetMinZoom(ZMapWindow window)
{
  window->min_zoom = zmapWindowCalcZoomFactor(window);

  return;
}


/* This routine is called by the code in zmapView.c that manages the slave threads. 
 * It has to determine whether or not the canvas has got far enough along in its
 * creation to have a valid height.  If so, it calls the code to notify the GUI
 * that data exists and work needs to be done.  If not, it attaches a callback
 * routine, exposeHandlerCB to the expose_event for the canvas so that when that
 * occurs, exposeHandlerCB can issue the call to sendClientEvent.
 * We'd have used the realize signal if we could, but somehow the canvas manages to
 * achieve realized status (ie GTK_WIDGET_REALIZED yields TRUE) without having a 
 * valid vertical dimension. 
 */
void zMapWindowDisplayData(ZMapWindow window, ZMapFeatureContext current_features,
			   ZMapFeatureContext new_features, GData *types,
			   void *zmap_view)
{
  RealizeData realizeData = g_new0(RealizeDataStruct, 1);  /* freed in exposeHandlerCB() */

  if (GTK_WIDGET(window->canvas)->allocation.height > 1
      && GTK_WIDGET(window->canvas)->window)
    {
      sendClientEvent(window, current_features, new_features, types);
    }
  else
    {
      realizeData->window           = window;
      realizeData->view             = zmap_view;
      realizeData->current_features = current_features;
      realizeData->new_features     = new_features;
      realizeData->types            = types;
      window->exposeHandlerCB = g_signal_connect(GTK_OBJECT(window->canvas), "expose_event",
						  GTK_SIGNAL_FUNC(exposeHandlerCB), (gpointer)realizeData);
    }
  return;
}


void zMapWindowReset(ZMapWindow window)
{

  /* Dummy code, need code to blank the window..... */

  zMapDebug("%s", "GUI: in window reset...\n") ;


  return ;
}


GtkWidget *zMapWindowGetWidget(ZMapWindow window)
{
  return GTK_WIDGET(window->canvas) ;
}


ZMapWindowZoomStatus zMapWindowGetZoomStatus(ZMapWindow window)
{
  return window->zoom_status ;
}


void zmapWindowSetPageIncr(ZMapWindow window)
{
  GTK_LAYOUT(window->canvas)->vadjustment->page_increment = GTK_WIDGET(window->canvas)->allocation.height - 50;
  return;
}


void zMapWindowSetZoomStatus(ZMapWindow window)
{
  if (window->zoom_factor < window->min_zoom)
    window->zoom_status = ZMAP_ZOOM_MIN;
  else if (window->zoom_factor > window->max_zoom)
    window->zoom_status = ZMAP_ZOOM_MAX;
  else
    window->zoom_status = ZMAP_ZOOM_MID;

  return;
}

void zMapWindowDestroy(ZMapWindow window)
{
  int i;
  GtkWidget *widget;

  zMapDebug("%s", "GUI: in window destroy...\n") ;

  if (window->sequence)
    g_free(window->sequence) ;

  /* free the array of featureListWindows and the windows themselves */
  if (window->featureListWindows)
    {
      for (i = 0; i < window->featureListWindows->len; i++)
	{
	  widget = g_ptr_array_index(window->featureListWindows, i);
	  gtk_widget_destroy(widget);
	}
      g_ptr_array_free(window->featureListWindows, FALSE);
    }
  
  if (window->columns)
    g_ptr_array_free(window->columns, TRUE);                
  
  g_datalist_clear(&(window->featureItems));
  g_datalist_clear(&(window->longItems));
  g_free(window) ;
  
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
void zMapWindowZoom(ZMapWindow window, double zoom_factor)
{
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT ;
  GtkAdjustment *adjust;
  ZMapCanvasData canvasData ;
  int x, y ;
  double width, curr_pos = 0.0 ;
  double new_zoom, canvas_zoom ;
  double x1, y1, x2, y2 ;
  double max_win_span, seq_span, new_win_span, new_canvas_span ;
  double top, bot ;
  ZMapWindowVisibilityChangeStruct vis_change ;


  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)) ;

  canvasData = g_object_get_data(G_OBJECT(window->canvas), "canvasData");


  /* We don't zoom in further than is necessary to show individual bases or further out than
   * is necessary to show the whole sequence, or if the sequence is so short that it can be
   * shown at max. zoom in its entirety. */
  if (window->zoom_status == ZMAP_ZOOM_FIXED
      || (zoom_factor > 1.0 && window->zoom_status == ZMAP_ZOOM_MAX)
      || (zoom_factor < 1.0 && window->zoom_status == ZMAP_ZOOM_MIN))
    {
      /* Currently we don't call the visibility change callback because nothing has changed,
       * we may want to revisit this decision. */

      return ;
    }


  /* Record the current position. */
  /* In order to stay centred on wherever we are in the canvas while zooming, we get the 
   * current position (offset, in canvas pixels), add half the page-size to get the centre
   * of the screen,then convert to world coords and store those.    
   * After the zoom, we convert those values back to canvas pixels (changed by the call to
   * pixels_per_unit) and scroll to them. */
  foo_canvas_get_scroll_offsets(window->canvas, &x, &y);
  y += (adjust->page_size - 1) / 2 ;
  foo_canvas_c2w(window->canvas, x, y, &width, &curr_pos) ;

  /* Calculate the zoom. */
  new_zoom = window->zoom_factor * zoom_factor ;
  if (new_zoom < window->min_zoom)
    {
      window->zoom_factor = window->min_zoom ;
      window->zoom_status = zoom_status = ZMAP_ZOOM_MIN ;
    }
  else if (new_zoom > window->max_zoom)
    {
      window->zoom_factor = window->max_zoom ;
      window->zoom_status = zoom_status = ZMAP_ZOOM_MAX ;
    }
  else
    {
      window->zoom_factor = new_zoom ;
      window->zoom_status = zoom_status = ZMAP_ZOOM_MID ;
    }
  canvas_zoom = 1 / window->zoom_factor ;


  /* Calculate limits to what we can show. */
  max_win_span = (double)(window->canvas_maxwin_size) ;
  seq_span = window->seqLength * window->zoom_factor ;

  /* Calculate the extent of the new span, new span must not exceed maximum X window size
   * but we must display as much of the sequence as we can for zooming out. */
  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);
  new_win_span = y2 - y1 + 1 ;
  new_win_span *= window->zoom_factor ;

  new_win_span = (new_win_span >= max_win_span ?  max_win_span
		  : (seq_span > max_win_span ? max_win_span
		     : seq_span)) ;

  /* Calculate the position of the new span clamping it within the extent of the sequence,
   * note that if we end up going off the top or bottom we must slide the whole span down
   * or up to ensure we expand the sequence view as we zoom out. */
  new_canvas_span = new_win_span * canvas_zoom ;

  top = curr_pos - (new_canvas_span / 2) ;
  bot = curr_pos + (new_canvas_span / 2) ;

  if (top < window->seq_start)
    {
      if ((bot = bot + (window->seq_start - top)) > window->seq_end)
	bot = window->seq_end ;
      top = window->seq_start ;
    }
  else if (bot > window->seq_end)
    {
      if ((top = top - (bot - window->seq_end)) < window->seq_start)
	top = window->seq_start ;
      bot = window->seq_end ;
    }

  /* Set the new scroll_region and the new zoom. N.B. may need to do a "freeze" of the canvas here
   * to avoid a double redraw....but that might never happen actually, depends how much there is
   * in the Xlib buffer so not lets worry about it. */
  foo_canvas_set_scroll_region(window->canvas, x1, top, x2, bot) ;
  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;


  /* Scroll to the previous position. */
  foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
  foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));

  /* Now hide/show any columns that have specific show/hide zoom factors set. */
  if (window->columns)
    hideUnhideColumns(window) ;
  
  /* Redraw the scale bar. NB On splitting a window, scaleBarGroup is null at this point. */
  if (FOO_IS_CANVAS_ITEM (window->scaleBarGroup))
    gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));

  window->scaleBarGroup = zmapDrawScale(window->canvas,
					window->scaleBarOffset, 
					window->zoom_factor,
					top, bot,
					&(window->major_scale_units), &(window->minor_scale_units));

  window->scroll_x1 = x1;
  window->scroll_y1 = top;
  window->scroll_x2 = x2;
  window->scroll_y2 = bot;


  /* There is a hard limit on the absolute size of an xwindow of 32k pixels, some objects which
   * span the whole of a sequence may exceed this as we zoom in and therefore we need to crop
   * them. */
  g_datalist_foreach(&(window->longItems), zmapWindowCropLongFeature, window);


  /* Call the visibility change callback to notify our caller that our zoom/position has
   * changed. */
  vis_change.zoom_status = zoom_status ;
  vis_change.scrollable_top = top ;
  vis_change.scrollable_bot = bot ;
  (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;

  return ;
}


/* Move the window to a new part of the canvas, we need this because when the window is
 * zoomed in, it may not extend over the whole canvas so this kind of alternative scrolling.
 * is needed to reach parts of the canvas not currently mapped. */
void zMapWindowMove(ZMapWindow window, double start, double end)
{
  double x1, y1, x2, y2 ;
  ZMapWindowVisibilityChangeStruct vis_change ;

  /* Clamp the start/end. */
  if (start < window->seq_start)
    start = window->seq_start ;
  if (end > window->seq_end)
    end = window->seq_end ;

  /* We don't need the y1/y2 probably, but we do need the x1, x2.... */
  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);

  /* The test here is almost not worth doing as these are doubles. */
  if (start != y1 && end != y2)
    {
      foo_canvas_set_scroll_region(window->canvas, x1, start, x2, end) ;
    }

  /* Call the visibility change callback to notify our caller that our zoom/position has
   * changed, note there is some redundancy here if the call to this routine came as a.
   * result of the user interaction then there may be no need to do this....think about this. */
  vis_change.zoom_status = window->zoom_status ;
  vis_change.scrollable_top = start ;
  vis_change.scrollable_bot = end ;
  (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;

  return ;
}



/* We just dip into the foocanvas struct for some data, we use the interface calls where they
 * exist. */
void zmapWindowPrintCanvas(FooCanvas *canvas)
{
  double x1, y1, x2, y2 ;


  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);

  printf("Canvas -\tzoom_x: %f\tzoom_y: %f\t"
	 "scroll_x1: %f\tscroll_x2: %f\tscroll_y1: %f\tscroll_y2: %f\t"
	 "\n", canvas->pixels_per_unit_x, canvas->pixels_per_unit_y,
	 x1, x2, y1, y2) ;

  return ;
}



GQuark zMapWindowGetFocusQuark(ZMapWindow window)
{
  return window->focusQuark;
}


gchar *zMapWindowGetTypeName(ZMapWindow window)
{
  return window->typeName;
}


void zmapWindowCropLongFeature(GQuark quark, gpointer data, gpointer user_data)
{
  ZMapWindowLongItem longItem = (ZMapWindowLongItem)data;
  ZMapWindow window = (ZMapWindow)user_data;
  double start, end;

  start = longItem->start;
  end  = longItem->end;

  if ((longItem->start <= window->scroll_y1    && longItem->end >= window->scroll_y2)
      || (longItem->start <= window->scroll_y1 && longItem->end >= window->scroll_y1)
      || (longItem->start <= window->scroll_y2 && longItem->end >= window->scroll_y2))
    {
      if (longItem->start < window->scroll_y1)
	start = window->scroll_y1 - 10;
      if (longItem->end > window->scroll_y2)
	end = window->scroll_y2 + 10;

      if (longItem->end > longItem->start)
	foo_canvas_item_set(longItem->canvasItem,
			    "y1", start,
			    "y2", end,
			    NULL);
    }
  return;
}



/*
 *  ------------------- Internal functions -------------------
 */


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

	adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)) ;

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
			 0.0, (double)window->major_scale_units,
			 NULL, &incr) ;
	else
	  foo_canvas_w2c(window->canvas,
			 0.0, (double)window->minor_scale_units,
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
  ZMapWindowVisibilityChangeStruct vis_change ;

  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);

  /* There is no sense in trying to scroll the region if we already showing all of it already. */
  if (y1 > window->seq_start || y2 < window->seq_end)
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

      if (y1 < window->seq_start)
	{
	  y1 = window->seq_start ;
	  y2 = y1 + window_size - 1 ;
	}
      if (y2 > window->seq_end)
	{
	  y2 = window->seq_end ;
	  y1 = y2 - window_size + 1 ;
	}

      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2) ;

      /* Call the visibility change callback to notify our caller that our zoom/position has
       * changed, note there is some redundancy here if the call to this routine came as a.
       * result of the user interaction then there may be no need to do this....think about this. */
      vis_change.zoom_status = window->zoom_status ;
      vis_change.scrollable_top = y1 ;
      vis_change.scrollable_bot = y2 ;
      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
    }

  return ;
}



static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data)
{
  /* widget is the scrolled_window, user_data is the zmapWindow */

  ZMapWindow window = (ZMapWindow)user_data;


  printf("sizeAllocateCB: x: %d, y: %d, height: %d, width: %d\n", 
	 alloc->x, alloc->y, alloc->height, alloc->width); 

  if (window->seqLength) /* when window first drawn, seqLength = 0 */
    {
      if (window->zoom_status == ZMAP_ZOOM_MIN)
	window->zoom_factor = zmapWindowCalcZoomFactor(window);

      zMapWindowSetMinZoom(window); 
      zMapWindowSetZoomStatus(window);
      zmapWindowSetPageIncr(window); 

      /* call the function given us by zmapView.c to set zoom buttons for this window */
      /*      (*(window_cbs_G->setZoomButtons))(realizeData->window, realizeData->view);*/

    }

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
  RealizeData realizeData = (RealizeDataStruct*)user_data;

  printf("exposeHandlerCB\n");
  /* call the function given us by zmapView.c to set zoom_status
   * for all windows in this view. */
  (*(window_cbs_G->setZoomStatus))(realizeData->window, realizeData->view, NULL);

  zMapAssert(GTK_WIDGET_REALIZED(widget));

  /* disconnect signal handler before calling sendClientEvent otherwise when splitting
   * windows, the scroll-to which positions the new window in the same place on the
   * sequence as the previous one, will trigger another call to this function.  */
  g_signal_handler_disconnect(G_OBJECT(widget), realizeData->window->exposeHandlerCB);

  sendClientEvent(realizeData->window, 
		  realizeData->current_features, 
		  realizeData->new_features, 
		  realizeData->types);

  realizeData->window->exposeHandlerCB = 0 ;
  g_free(realizeData);

  return FALSE;  /* ie allow any other callbacks to run as well */
}



/* This routine sends a synthesized event to alert the GUI that it needs to
 * do some work and supplies the data for the GUI to process via the event struct. */
static void sendClientEvent(ZMapWindow window, ZMapFeatureContext current_features, 
			    ZMapFeatureContext new_features, GData *types)
{
  GdkEventClient event ;
  GdkAtom zmap_atom ;
  gint ret_val = 0 ;
  zmapWindowData window_data ;


  /* NEED TO SET UP NEW FEATURES HERE.... */

  /* Set up struct to be passed to our callback. */
  window_data = g_new0(zmapWindowDataStruct, 1) ;
  window_data->window = window ;
  window_data->data = current_features ;
  window_data->types = types;

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



static void hideUnhideColumns(ZMapWindow window)
{
  int i;
  ZMapCol column;
  double min_mag;

  for ( i = 0; i < window->columns->len; i++ )
    {
      column = g_ptr_array_index(window->columns, i);

      /* type->min_mag is in bases per line, but window->zoom_factor is pixels per base */
      min_mag = (column->type->min_mag ? window->max_zoom/column->type->min_mag : 0.0);

      if (window->zoom_factor > min_mag)
	{
	  if (column->forward)
	    foo_canvas_item_show(column->column);
	  else
	    if (column->type->showUpStrand)
	      foo_canvas_item_show(column->column);
	}
      else
	  foo_canvas_item_hide(column->column);
    }
  return;
}





/* Called when gtk detects the event sent by signalDataToGUI(), this routine calls
 * the data display routines of ZMap. */
static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)cb_data ;

  if (event->type != GDK_CLIENT_EVENT)
    zMapLogFatal("%s", "dataEventCB() received non-GdkEventClient event") ;
  
  if (event->send_event == TRUE && event->message_type == gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      gpointer data = NULL ;
      ZMapFeatureContext feature_context ;

      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;

      window = window_data->window ;

      data = (gpointer)(window_data->data) ;

      /* Can either get data from my dummied up GFF routine or if you set up an acedb server
       * you can get data from there.... just undef the one you want... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      feature_context = testGetGFF() ;			    /* Data read from a file... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      feature_context = (ZMapFeatureContext)data ;	    /* Data from a server... */


      /* ****Remember that someone needs to free the data passed over....****  */


      /* Draw the features on the canvas */
      zmapWindowDrawFeatures(window, feature_context, window_data->types) ;


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



static void clickCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapFeature feature = (ZMapFeature)window_data ;

  (*(window_cbs_G->click))(window, caller_data, feature);

  return;
}


static gboolean rightClickCB(ZMapWindow window, ZMapFeatureItem featureItem)
{
  /* user selects new feature in this column */
  zMapWindowCreateListWindow(window, featureItem);

  return TRUE;
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
						     {NULL, -1, {NULL}}} ;

  /* Set default values in stanza, keep this in synch with initialisation of window_elements array. */
  window_elements[0].data.i = ZMAP_WINDOW_MAX_WINDOW ;

  if ((config = zMapConfigCreate()))
    {
      window_stanza = zMapConfigMakeStanza(window_stanza_name, window_elements) ;

      if (zMapConfigFindStanzas(config, window_stanza, &window_list))
	{
	  ZMapConfigStanza next_window ;
	  
	  /* Get the first window stanza found, we will ignore any others. */
	  next_window = zMapConfigGetNextStanza(window_list, NULL) ;
	  
	  window->canvas_maxwin_size = zMapConfigGetElementInt(next_window, "canvas_maxsize") ;
	  
	  zMapConfigDeleteStanzaSet(window_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(window_stanza) ;
      
      zMapConfigDestroy(config) ;

      result = TRUE ;
    }

  return result ;
}


/* Handle any events that are not intercepted by items on the canvas, items could be columns
 * or the features within columns. */
static gboolean canvasEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)data ;

  switch (event->type)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* SOMETIME WE WILL WANT TO DO A MENU HERE FOR THE BACKGROUND.... */
    case GDK_BUTTON_PRESS:
      {
	printf("basic canvas event handler: button release\n") ;

	/* Call the callers routine for button clicks. */
	window_cbs_G->click(window, window->app_data, NULL) ;
	
	event_handled = FALSE ;
	break ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    case GDK_KEY_PRESS:
      {
	GdkEventKey *key_event = (GdkEventKey *)event ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	printf("basic canvas event handler: key press\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	switch (key_event->keyval)
	  {
	  case GDK_Page_Up:
	  case GDK_Page_Down:
	  case GDK_Up:
	  case GDK_Down:
	  case GDK_Left:
	  case GDK_Right:
	    moveWindow(window, key_event->state, key_event->keyval) ;
	    break ;

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

	event_handled = TRUE ;
	
	break ;
      }
    case GDK_ENTER_NOTIFY:
      {
	/* We want the canvas to be the focus widget of its "window" otherwise keyboard input
	 * (i.e. short cuts) will be delivered to some other widget. */
	gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;

	event_handled = TRUE ;

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
STORE_HANDLE zMapWindowGetHandle(ZMapWindow window)
{
  return window->handle;
}


void zMapWindowSetHandle(ZMapWindow window)
{
  window->handle = NULL;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void zMapWindowSetBorderWidth(GtkWidget *container, int width)
{
  gtk_container_border_width(GTK_CONTAINER(container), width);
  return;
}


ScreenCoord zMapWindowGetScaleOffset(ZMapWindow window)
{
  return 0;
}


void zMapWindowSetScaleOffset(ZMapWindow window, ScreenCoord offset)
{
  return;
}


Coord zMapWindowGetCoord(ZMapWindow window, char *field)
{
  /*  if (field == "s")
    return window->navStart;
  else
    return window->navEnd;
  */
  return 0;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapWindowSetCoord(ZMapWindow window, char *field, int size)
{
  if (field == "s")
    window->navStart = window->focuspane->zMapRegion->area1 - size;
  else
    window->navEnd = window->focuspane->zMapRegion->area2 + size;
  
  if (window->navStart == window->navEnd)
    window->navEnd = window->navStart + 1;
  
  return;
}


ScreenCoord zMapWindowGetScreenCoord1(ZMapWindow window, int height)
{
  return zMapWindowGetScreenCoord(window, window->focuspane->zMapRegion->area1, height);
}


ScreenCoord zMapWindowGetScreenCoord2(ZMapWindow window, int height)
{
  return zMapWindowGetScreenCoord(window, window->focuspane->zMapRegion->area2, height);
}

ScreenCoord zMapWindowGetScreenCoord(ZMapWindow window, Coord coord, int height)
{
  return height * (coord - window->navStart) / (window->navEnd - window->navStart);
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* ZMapRegion functions */

GPtrArray *zMapRegionNewMethods(ZMapRegion *region)
{
  return region->methods = g_ptr_array_new();
}

GPtrArray *zMapRegionGetMethods(ZMapRegion *region)
{
  return region->methods;
}

GPtrArray *zMapRegionGetOldMethods(ZMapRegion *region)
{
  return region->oldMethods;
}

void zMapRegionFreeMethods(ZMapRegion *region)
{
  g_ptr_array_free(region->methods, TRUE);
  return;
}

void zMapRegionFreeOldMethods(ZMapRegion *region)
{
  g_ptr_array_free(region->oldMethods, TRUE);
  return;
}

GArray *zMapRegionNewSegs(ZMapRegion *region)
{
  return region->segs = g_array_new(FALSE, FALSE, sizeof(ZMapFeatureStruct)) ;
}

GArray *zMapRegionGetSegs(ZMapRegion *region)
{
  return region->segs;
}

void zMapRegionFreeSegs(ZMapRegion *region)
{
  free(region->segs);
  return;
}


GArray *zMapRegionGetDNA(ZMapRegion *region)
{
  return region->dna;
}

void zMapRegionFreeDNA(ZMapRegion *region)
{
  g_array_free(region->dna, TRUE);
  return; 

}




/****************** end of file ************************************/
