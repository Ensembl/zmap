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
 * Last edited: Mar 10 10:40 2005 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.65 2005-03-16 15:56:10 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>


/* Local struct to hold current features and new_features obtained from a server and
 * relevant types. */
typedef struct
{
  ZMapFeatureContext  current_features ;
  ZMapFeatureContext  new_features ;
  GData *types ;
} FeatureSetsStruct, *FeatureSets ;



/* This struct is used to pass data to realizeHandlerCB
 * via the g_signal_connect on the canvas's expose_event. */
typedef struct _RealiseDataStruct
{
  ZMapWindow window ;
  FeatureSets feature_sets ;
} RealiseDataStruct, *RealiseData ;



static gboolean dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void sizeAllocateCB(GtkWidget *widget, GtkAllocation *alloc, gpointer user_data) ;
static gboolean exposeHandlerCB(GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static gboolean canvasRootEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;

static gboolean getConfiguration(ZMapWindow window) ;
static void sendClientEvent     (ZMapWindow window, FeatureSets) ;

static void moveWindow(ZMapWindow window, guint state, guint keyval) ;
static void scrollWindow(ZMapWindow window, guint state, guint keyval) ;
static void changeRegion(ZMapWindow window, guint keyval) ;

void hideAlignmentCols(GQuark key_id, gpointer data, gpointer user_data) ;


static void printGroup(FooCanvasGroup *group, int indent) ;

/* These structure is static because the callback routines are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapWindowCallbacks window_cbs_G = NULL ;




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
	     && callbacks->scroll && callbacks->click
	     && callbacks->visibilityChange && callbacks->destroy) ;

  window_cbs_G = g_new0(ZMapWindowCallbacksStruct, 1) ;

  window_cbs_G->enter  = callbacks->enter ;
  window_cbs_G->leave   = callbacks->leave ;
  window_cbs_G->scroll  = callbacks->scroll ;
  window_cbs_G->click   = callbacks->click ;
  window_cbs_G->setZoomStatus = callbacks->setZoomStatus;

  window_cbs_G->visibilityChange = callbacks->visibilityChange ;

  window_cbs_G->destroy = callbacks->destroy ;

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

  window->caller_cbs = window_cbs_G ;

  window->sequence = g_strdup(sequence) ;
  window->app_data = app_data ;
  window->parent_widget = parent_widget ;

  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  window->zoom_factor = 0.0 ;
  window->zoom_status = ZMAP_ZOOM_INIT ;
  window->canvas_maxwin_size = ZMAP_WINDOW_MAX_WINDOW ;

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


  /* This is a general handler that does stuff like handle "click to focus", it gets run
   * _before_ any canvas item handlers. */
  g_signal_connect(GTK_OBJECT(window->canvas), "event",
		   GTK_SIGNAL_FUNC(canvasWindowEventCB), (gpointer)window) ;


  /* Wierdly this doesn't get run _unless_ the click/event happens on a child of the
   * root first...sigh..... */
  g_signal_connect(GTK_OBJECT(foo_canvas_root(window->canvas)), "event",
		   GTK_SIGNAL_FUNC(canvasRootEventCB), (gpointer)window) ;

  
  /* NONE OF THIS SHOULD BE HARD CODED................. */
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


/* Makes a new window that is a copy of the existing one, zoom factor and all.
 * Actually this is rubbish....if its a copy why are we putting in the features ? */
ZMapWindow zMapWindowCopy(GtkWidget *parent_widget, char *sequence, 
			  void *app_data, ZMapWindow copy_window,
			  ZMapFeatureContext feature_context, GData *types)
{
  ZMapWindow new = NULL ;

  if ((new = zMapWindowCreate(parent_widget, sequence, app_data)))
    {
      double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
      int x, y ;

      new->zoom_factor        = copy_window->zoom_factor;
      new->zoom_status        = copy_window->zoom_status = ZMAP_ZOOM_MID;
      new->max_zoom           = copy_window->max_zoom;
      new->canvas_maxwin_size = copy_window->canvas_maxwin_size;
      new->border_pixels      = copy_window->border_pixels;
      new->DNAwidth           = copy_window->DNAwidth;
      new->text_height        = copy_window->text_height;
      new->seqLength          = copy_window->seqLength;
      new->seq_start          = copy_window->seq_start;
      new->focusFeature       = copy_window->focusFeature;
      new->focusType          = copy_window->focusType;
      new->typeName           = copy_window->typeName;
      new->focusQuark         = copy_window->focusQuark;


      /* I'm a little uncertain how much of the below is really necessary as we are
       * going to call the draw features code anyway. */
      /* Set the zoom factor, there is no call to get hold of pixels_per_unit so we dive.
       * into the struct. */
      foo_canvas_set_pixels_per_unit_xy(new->canvas,
					copy_window->canvas->pixels_per_unit_x,
					copy_window->canvas->pixels_per_unit_y) ;

      foo_canvas_get_scroll_region(copy_window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
      foo_canvas_set_scroll_region(new->canvas, scroll_x1, scroll_y1, scroll_x2, scroll_y2) ;

      foo_canvas_get_scroll_offsets(copy_window->canvas, &x, &y) ;
      foo_canvas_scroll_to(new->canvas, x, y) ;


      /* You cannot just draw the features here as the canvas needs to be realised so we send
       * an event to get the data drawn which means that the canvas is guaranteed to be
       * realised by the time we draw into it. */
      zMapWindowDisplayData(new, feature_context, feature_context, types) ;
    }
			      
  return new ;
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
 * We'd have used the realise signal if we could, but somehow the canvas manages to
 * achieve realised status (ie GTK_WIDGET_REALIZED yields TRUE) without having a 
 * valid vertical dimension. 
 *
 * No, that's not the problem, it is realised, it just hasn't got sized properly yet.
 * 
 *  */
void zMapWindowDisplayData(ZMapWindow window, ZMapFeatureContext current_features,
			   ZMapFeatureContext new_features, GData *types)
{
  FeatureSets feature_sets ;

  feature_sets = g_new0(FeatureSetsStruct, 1) ;
  feature_sets->current_features = current_features ;
  feature_sets->new_features = new_features ;
  feature_sets->types = types ;

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
  /* WHY IS THIS  - 50, you should have documented this Rob........ */

  GTK_LAYOUT(window->canvas)->vadjustment->page_increment
    = GTK_WIDGET(window->canvas)->allocation.height - 50 ;

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
  int x, y ;
  double width, curr_pos = 0.0 ;
  double new_zoom, canvas_zoom ;
  double x1, y1, x2, y2 ;
  double max_win_span, seq_span, new_win_span, new_canvas_span ;
  double top, bot ;
  ZMapWindowVisibilityChangeStruct vis_change ;


  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* These don't work but I may not be using them correctly, more research needed, look
   * at canvas interface..... */
  gtk_widget_freeze_child_notify(GTK_WIDGET(window->canvas)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  foo_canvas_set_scroll_region(window->canvas, x1, top, x2, bot) ;
  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_thaw_child_notify(GTK_WIDGET(window->canvas)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Scroll to the previous position. */
  foo_canvas_w2c(window->canvas, width, curr_pos, &x, &y);
  foo_canvas_scroll_to(window->canvas, x, y - (adjust->page_size/2));


  /* Now hide/show any columns that have specific show/hide zoom factors set. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (window->columns)
    zmapHideUnhideColumns(window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  /* N.B. We could pass something else in other than the window as user_date.... */
  if (window->alignments)
    g_datalist_foreach(&window->alignments, hideAlignmentCols, window) ;



  

  /* Redraw the scale bar. NB On splitting a window, scaleBarGroup is null at this point. */
  if (FOO_IS_CANVAS_ITEM (window->scaleBarGroup))
    gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));

  window->scaleBarGroup = zmapDrawScale(window->canvas,
					window->scaleBarOffset, 
					window->zoom_factor,
					top, bot,
					&(window->major_scale_units), &(window->minor_scale_units));


  /* Presumeably this only needs to be done if the extent of the features is longer than the
   * scrolled region ? Really this call to g_datalist should be within a function.... */
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


  /* Redraw the scale bar. */
  zMapAssert(FOO_IS_CANVAS_ITEM (window->scaleBarGroup)) ;

  gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup)) ;

  window->scaleBarGroup = zmapDrawScale(window->canvas,
					window->scaleBarOffset, 
					window->zoom_factor,
					start, end,
					&(window->major_scale_units), &(window->minor_scale_units)) ;


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
  ZMapWindowLongItem longItem = (ZMapWindowLongItem)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;
  double start, end ;

  foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;

  start = longItem->start;
  end  = longItem->end;

  if ((longItem->start <= scroll_y1 && longItem->end >= scroll_y2)
      || (longItem->start <= scroll_y1 && longItem->end >= scroll_y1)
      || (longItem->start <= scroll_y2 && longItem->end >= scroll_y2))
    {
      if (longItem->start < scroll_y1)
	start = scroll_y1 - 10;
      if (longItem->end > scroll_y2)
	end = scroll_y2 + 10;

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



/* Recursive function to print out all the groups below the supplied group. */
static void printGroup(FooCanvasGroup *group, int indent)
{
  int i ;
  GList *list ;

  for (i = 0 ; i < indent ; i++)
    printf("\t") ;

  zmapWindowPrintGroup(group) ;

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
       * changed, note there is some redundancy here if the call to this routine came as a
       * result of the user interaction then there may be no need to do this....think about this. */
      vis_change.zoom_status = window->zoom_status ;
      vis_change.scrollable_top = y1 ;
      vis_change.scrollable_bot = y2 ;
      (*(window_cbs_G->visibilityChange))(window, window->app_data, (void *)&vis_change) ;
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (window->seqLength) /* when window first drawn, seqLength = 0 */
    {
      if (window->zoom_status == ZMAP_ZOOM_MIN)
	window->zoom_factor = zmapWindowCalcZoomFactor(window);

      zMapWindowSetMinZoom(window); 
      zMapWindowSetZoomStatus(window);
      zmapWindowSetPageIncr(window); 

      /* call the function given us by zmapView.c to set zoom buttons for this window */
      /*      (*(window_cbs_G->setZoomButtons))(realiseData->window, realiseData->view);*/

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
  ZMapWindow window = (ZMapWindow)cb_data ;

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
      zmapWindowDrawFeatures(window, feature_sets->current_features, diff_context,
			     feature_sets->types) ;


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


/* This gets run _BEFORE_ any of the canvas item handlers which is good because we can use it
 * handle more general events such as "click to focus" etc. */
static gboolean canvasWindowEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  gboolean event_handled ;
  ZMapWindow window = (ZMapWindow)data ;


  /* MUST be false if the other per-item event handlers are to run.  */
  event_handled = FALSE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("GENERAL canvas event handler\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	printf("GENERAL event handler - CLICK\n") ;

	/* We want the canvas to be the focus widget of its "window" otherwise keyboard input
	 * (i.e. short cuts) will be delivered to some other widget. */
	gtk_widget_grab_focus(GTK_WIDGET(window->canvas)) ;

	/* Report to our parent that we have been clicked in, we change "click" to "focus"
	 * because that is what this if for.... */
	(*(window_cbs_G->click))(window, window->app_data, NULL) ;


	/* Oh dear, we would like to do a general back ground menu here but can't as we need
	 * to pass the event along in case it goes to a canvas item....aggghhhh */

	break ;
      }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    case GDK_BUTTON_RELEASE:
      {
	/* Call the callers routine for button clicks. */
	window_cbs_G->click(window, window->app_data, NULL) ;
	
	event_handled = FALSE ;
	break ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
	    moveWindow(window, key_event->state, key_event->keyval) ;
	    event_handled = TRUE ;
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


	
	break ;
      }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Disabled because we now do "click to focus" */
    case GDK_ENTER_NOTIFY:
      {
	event_handled = TRUE ;

	break ;
      }
    case GDK_LEAVE_NOTIFY:
      {
	event_handled = TRUE ;

	break ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    default:
      {
	/* By default we _don't_handle events. */
	event_handled = FALSE ;

	break ;
      }
    }


  return event_handled ;
}



/* Handle any events that are not intercepted by items or columns on the canvas,
 * I'm not sure how often this will be called as probably it will be covered mostly
 * by the column or item things...or the features within columns. */
static gboolean canvasRootEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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


void hideAlignmentCols(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data ;
  ZMapWindowAlignment alignment = (ZMapWindowAlignment)data ;
  ZMapWindowAlignmentBlock block ;

  /* Hack this for now to simply get a block..... */
  block = (ZMapWindowAlignmentBlock)g_datalist_get_data(&(alignment->blocks),
							"dummy") ;


  zmapWindowAlignmentHideUnhideColumns(block) ;

  return ;
}
