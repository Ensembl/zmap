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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Provides interface functions for controlling a data
 *              display window.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Sep 21 10:21 2004 (rnc)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.30 2004-09-21 13:10:27 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapDraw.h>
#include <ZMap/zmapWindowDrawFeatures.h>

static void dataEventCB        (GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void canvasClickCB      (GtkWidget *widget, GdkEventClient *event, gpointer data) ;
static void clickCB            (ZMapWindow window, void *caller_data, ZMapFeature feature);
static gboolean rightClickCB   (ParamStruct *params, ZMapFeatureSet feature_set);
static void addItemToList      (GQuark key_id, gpointer data, gpointer user_data);
static void quitListCB         (GtkWidget *window, gpointer data);


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

  zMapAssert(callbacks && callbacks->scroll && callbacks->click && callbacks->destroy) ;


  window_cbs_G = g_new0(ZMapWindowCallbacksStruct, 1) ;

  window_cbs_G->scroll  = callbacks->scroll ;
  window_cbs_G->click   = callbacks->click ;
  window_cbs_G->destroy = callbacks->destroy ;

  zMapFeatureInit(&feature_cbs_G);

  return ;
}



/* We will need to allow caller to specify a routine that gets called whenever the user
 * scrolls.....needed to update the navigator...... */
/* and we will need a callback for focus events as well..... */
/* I think probably we should insist on being supplied with a sequence.... */
ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence, void *app_data)
{
  ZMapWindow window ;
  GtkWidget *vbox, *menubar, *button_frame, *connect_frame ;
  GtkWidget *canvas ;
  GtkAdjustment *adj ; 
  GdkColor color;

  /* No callbacks, then no window creation. */
  zMapAssert(window_cbs_G) ;

  zMapAssert(parent_widget && sequence && *sequence && app_data) ;

  window = g_new0(ZMapWindowStruct, 1) ;
  window->sequence = g_strdup(sequence) ;
  window->app_data = app_data ;
  window->parent_widget = parent_widget ;

  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  window->zoom_factor = 1 ;
  window->step_increment = 10;


  /* Set up a scrolled widget to hold the canvas. NOTE that this is our toplevel widget. */
  window->toplevel = window->scrolledWindow = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), window->scrolledWindow) ;


  /* this had disappeared...no idea why......... */
  gtk_signal_connect(GTK_OBJECT(window->toplevel), "client_event", 
		     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;


  /* always show scrollbars, however big the display */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->scrolledWindow),
				 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS) ;

  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* I think this is all broken, navChange certainly does not do the right things.... */

  /* Its also the wrong place....navChange should be when the view changes.... */

  g_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(navUpdate), (gpointer)(window));
  g_signal_connect(GTK_OBJECT(adj), "changed", GTK_SIGNAL_FUNC(navChange), (gpointer)(window)); 



#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  canvas = foo_canvas_new();
  /* add the canvas to the scrolled window */
  gtk_container_add(GTK_CONTAINER(window->scrolledWindow), canvas) ;

  gdk_color_parse("white", &color);
  gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &color);

  window->canvas = FOO_CANVAS(canvas);
  foo_canvas_set_scroll_region(window->canvas, 0.0, 0.0, 1000, 1000);

  g_signal_connect(GTK_OBJECT(window->canvas), "button_press_event",
		   GTK_SIGNAL_FUNC(canvasClickCB), (gpointer)window) ;

  window->group = foo_canvas_item_new(foo_canvas_root(window->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  /* you have to set the step_increment manually or the scrollbar arrows don't work.*/
  /* Using a member of ZMapPane means I can adjust it if necessary when we zoom. */
  GTK_LAYOUT(canvas)->vadjustment->step_increment = window->step_increment;
  GTK_LAYOUT(canvas)->hadjustment->step_increment = window->step_increment;


  /* This should all be in some kind of routine to draw the whole canvas in one go.... */

  /* should this be done by caller ??..... */
  gtk_widget_show_all(window->parent_widget) ;

  return(window) ;
}



/* This routine is called by the code that manages the slave threads, it makes
 * the call to tell the GUI code that there is something to do. This routine
 * does this by sending a synthesized event to alert the GUI that it needs to
 * do some work and supplies the data for the GUI to process via the event struct. */
void zMapWindowDisplayData(ZMapWindow window, void *data, GData *types)
{
  GdkEventClient event ;
  GdkAtom zmap_atom ;
  gint ret_val = 0 ;
  zmapWindowData window_data ;

  /* Set up struct to be passed to our callback. */
  window_data = g_new0(zmapWindowDataStruct, 1) ;
  window_data->window = window ;
  window_data->data = data ;
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


void zMapWindowReset(ZMapWindow window)
{

  /* Dummy code, need code to blank the window..... */

  zMapDebug("%s", "GUI: in window reset...\n") ;

  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			   "", -1) ;			    /* -1 => null terminated string. */

  return ;
}


void zMapWindowDestroy(ZMapWindow window)
{
  zMapDebug("%s", "GUI: in window destroy...\n") ;

  if (window->sequence)
    g_free(window->sequence) ;

  if (window->featureListWindow)
    gtk_widget_destroy(window->featureListWindow);

  g_free(window->params);
  g_free(window) ;

  return ;
}


/* Zooming: we don't zoom as such, we scale the group, doubling the
 * y axis but leaving x at 1.  This enlarges lengthways, while keeping
 * the width the same. */
void zMapWindowZoom(ZMapWindow window, double zoom_factor)
{
  int x, y, wy, z;
  GtkAdjustment *adjust;
  int direction = +1;

  if (zoom_factor < 1.0)
      direction = -1;

  foo_canvas_get_scroll_offsets(window->canvas, &x, &y);

  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
  wy = adjust->page_size;

  window->zoom_factor *= zoom_factor ;

  /* do we need to do anything else like redraw ?? */
  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, window->zoom_factor) ;

  z = (y * zoom_factor) + ((wy * zoom_factor / 4.0) * direction);
  foo_canvas_scroll_to(window->canvas, x, z );

  return ;
}

/*
void zMapWindowZoomIn(ZMapWindow window)
{
  int x, y, wy, z;
  GtkAdjustment *adjust;

  foo_canvas_get_scroll_offsets(window->canvas, &x, &y);

  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
  wy = adjust->page_size;

  zMapWindowZoom(window, 2.0);
*/  /* y and wy must be adjusted because we zoom by 2x.*/ /*
  z = (y * 2) + (wy / 2) * direction;
  foo_canvas_scroll_to(window->canvas, x, z );

  return;
}

*/

/* The vadjustment changes when the user stretches or shrinks the window and  
** the scroll_region y2 * zoom_factor equates to that when the whole scroll 
** region is visible.  If we're within spitting distance of all being on view,
** we recalculate the zoomfactor to achieve that directly.  Note that if all
** is already on view it still recalculates.  A disadvantage of this is that
** the display flashes as it redraws. The difficulty is, all these values are
** doubles, so determining the point at which to actually stop doing anything
** is a bit cumbersome.
*/
void zMapWindowZoomOut(ZMapWindow window)
{
  double window_y, x1, y1, x2, y2;
  GtkAdjustment *adjust;

  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
  window_y = adjust->page_size;

  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);

  if (y2 * window->zoom_factor > window_y)
    {
      if (y2 * window->zoom_factor * 0.5 > window_y)
	zMapWindowZoom(window, 0.5) ;
      else
	{
	  window->zoom_factor = window_y / y2;
	  zMapWindowZoom(window, 1.0);
	}
    }
  else
    {
      window->zoom_factor = window_y / y2;
      zMapWindowZoom(window, 1.0);
    }

  return;
}


/*
 *  ------------------- Internal functions -------------------
 */



/* Called when gtk detects the event sent by signalDataToGUI(), this routine calls
 * the data display routines of ZMap. */
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
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


      /* <<<<<<<<<<<<<  ROB, this is where calls to your drawing code need to go  >>>>>>>>>>>> */
      zmapWindowDrawFeatures(window, feature_context, window_data->types) ;


      g_free(window_data) ;				    /* Free the WindowData struct. */
    }
  else
    {
      zMapLogCritical("%s", "unknown client event in dataEventCB() handler\n") ;
    }


  return ;
}



static void clickCB(ZMapWindow window, void *caller_data, ZMapFeature feature)
{
  (*(window_cbs_G->click))(window, caller_data, feature);
  return;
}


static gboolean rightClickCB(ParamStruct *params, ZMapFeatureSet feature_set)
{
  /* user selects new feature in this column */
  zMapWindowCreateListWindow(params, feature_set);

  return TRUE;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Previous dummy routine...... */

/* Called when gtk detects the event sent by signalDataToGUI(), in the end this
 * routine will call zmap routines to display data etc. */
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  ZMapWindow window = (ZMapWindow)cb_data ;

  if (event->type != GDK_CLIENT_EVENT)
    zMapLogFatal("%s", "dataEventCB() received non-GdkEventClient event") ;
  
  if (event->send_event == TRUE && event->message_type == gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      char *string = NULL ;

      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;

      window = window_data->window ;

      string = (char *)(window_data->data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapDebug("%s", "GUI: got dataEvent, contents: \"%s\"\n", string) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Dummy test code.... */
      gtk_text_buffer_insert(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			     NULL, string, -1) ;	    /* -1 => null terminated string. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			       string, -1) ;		    /* -1 => null terminated string. */

      g_free(string) ;
      g_free(window_data) ;
    }
  else
    {
      zMapDebug("%s", "unknown client event in zmapevent handler\n") ;
    }


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void canvasClickCB(GtkWidget *widget, GdkEventClient *event, gpointer data)
{
  ZMapWindow window = (ZMapWindow)data ;


  /* Call the callers routine for button clicks. */
  window_cbs_G->click(window, window->app_data, NULL) ;


  return ;
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


InvarCoord zMapWindowGetOrigin(ZMapWindow window)
{
  return window->origin;
}


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





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* ZMapPane functions */

void zMapPaneNewBox2Col(ZMapPane pane, int elements)
{
  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), elements);

  return ;
}




/* MOVED FROM zmapWindow.c, THEY BE IN THE WRONG PLACE OR NOT EVEN NEEDED...... */




GArray *zMapPaneSetBox2Col(ZMapPane pane, ZMapColumn *col, int index)
{
  return g_array_insert_val(pane->box2col, index, col);
}

ZMapColumn *zMapPaneGetBox2Col(ZMapPane pane, int index)
{
  return &g_array_index(pane->box2col, ZMapColumn, index);
}


void zMapPaneFreeBox2Col(ZMapPane pane)
{
  if (pane->box2col)
    g_array_free(pane->box2col, TRUE);
  return;
}


void zMapPaneNewBox2Seg(ZMapPane pane, int elements)
{
  pane->box2seg = g_array_sized_new(FALSE, TRUE, sizeof(ZMapFeatureStruct), elements);

  return ;
}

GArray *zMapPaneSetBox2Seg(ZMapPane pane, ZMapColumn *seg, int index)
{
  return g_array_insert_val(pane->box2seg, index, seg);
}

ZMapFeature zMapPaneGetBox2Seg(ZMapPane pane, int index)
{
  return &g_array_index(pane->box2seg, ZMapFeatureStruct, index);
}

void zMapPaneFreeBox2Seg(ZMapPane pane)
{
  if (pane->box2seg)
    g_array_free(pane->box2seg, TRUE);
  return;
}




FooCanvasItem *zMapPaneGetGroup(ZMapPane pane)
{
  return pane->group;
}


FooCanvas *zMapPaneGetCanvas(ZMapPane pane)
{
  return pane->canvas;
}

GPtrArray *zMapPaneGetCols(ZMapPane pane)
{
  return &pane->cols;
}


int          zMapPaneGetDNAwidth       (ZMapPane pane)
{
  return pane->DNAwidth;
}


void zMapPaneSetDNAwidth       (ZMapPane pane, int width)
{
  pane->DNAwidth = 100;
  return;
}


void zMapPaneSetStepInc        (ZMapPane pane, int incr)
{
  pane->step_increment = incr;
  return;
}


int zMapPaneGetHeight(ZMapPane pane)
{
  return pane->graphHeight;
}

InvarCoord zMapPaneGetCentre(ZMapPane pane)
{
  return pane->centre;
}


float zMapPaneGetBPL (ZMapPane pane)
{
  return pane->basesPerLine;
}


/* all the zmappanexxx calls can go now, just reference window struct directly... */
void drawWindow(ZMapWindow window)
{
  float offset = 5;
  float maxOffset = 0;
  int   frameCol, i, frame = -1;
  float oldPriority = -100000;
  

  zMapPaneFreeBox2Col(pane);
  zMapPaneFreeBox2Seg(pane);
  zMapPaneNewBox2Col(pane, 500);
  zMapPaneNewBox2Seg(pane, 500);


  for (i = 0; i < zMapPaneGetCols(pane)->len; i++)
  
    { 
      ZMapColumn *col = g_ptr_array_index(zMapPaneGetCols(pane), i);
      float offsetSave = -1;
     
      /* frame : -1 -> No frame column.
	         0,1,2 -> current frame.
      */
      
      if ((frame == -1) && col->isFrame)
	{
	  /* First framed column, move into frame mode. */
	  frame = 0;
	  frameCol = i;
	}
      else if ((frame == 0 || frame == 1) && !col->isFrame)
	{
	  /* in frame mode and reached end of framed columns: backtrack */
	  frame++;
	  i = frameCol;
	  col = g_ptr_array_index(zMapPaneGetCols(pane), i);
	}
      else if ((frame == 2) && !col->isFrame)
	{
	  /* in frame mode, reach end of framed columns, done last frame. */
	  frame = -1;
	}
      else if (col->priority < oldPriority + 0.01001)
	offsetSave = offset;
     
      (*col->drawFunc)(pane, col, &offset, frame);

       oldPriority = col->priority;
       if (offset > maxOffset)
	maxOffset = offset;

      if (offsetSave > 0)
	offset = offsetSave;
      
    }
  return;
}
  

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/****************** end of file ************************************/
