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
 * Last edited: Jun 30 15:47 2004 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.13 2004-07-01 09:26:25 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <../acedb/regular.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapUtils.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapcommon.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
#include <ZMap/zmapFeature.h>

#include <zmapWindow_P.h>
#include <zmapcontrol.c>


static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;



ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence,
			    zmapVoidIntCallbackFunc app_routine, void *app_data)
{
  ZMapWindow window ;
  GtkWidget *toplevel, *vbox, *menubar, *button_frame, *connect_frame ;

  window = g_new(ZMapWindowStruct, sizeof(ZMapWindowStruct)) ;

  if (sequence)
    window->sequence = g_strdup(sequence) ;
  window->app_routine = app_routine ;
  window->app_data = app_data ;
  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;
  window->parent_widget = parent_widget ;

  /* Make the vbox the toplevel for now, it gets sent the client event, 
   * we may also want to register a "destroy" callback at some time as this
   * may be useful. */
  window->toplevel = toplevel = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), vbox) ;
  gtk_signal_connect(GTK_OBJECT(vbox), "client_event", 
		     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;

  connect_frame = zmapWindowMakeFrame(window) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;

  return(window) ;
}



/* This routine is called by the code that manages the slave threads, it makes
 * the call to tell the GUI code that there is something to do. This routine
 * does this by sending a synthesized event to alert the GUI that it needs to
 * do some work and supplies the data for the GUI to process via the event struct. */
void zMapWindowDisplayData(ZMapWindow window, void *data)
{
  GdkEventClient event ;
  GdkAtom zmap_atom ;
  gint ret_val = 0 ;
  zmapWindowData window_data ;

  /* Set up struct to be passed to our callback. */
  window_data = g_new(zmapWindowDataStruct, sizeof(zmapWindowDataStruct)) ;
  window_data->window = window ;
  window_data->data = data ;

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

  gtk_widget_destroy(window->toplevel) ;

  if (window->sequence)
    g_free(window->sequence) ;

  g_free(window) ;

  return ;
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




      g_free(window_data) ;				    /* Free the WindowData struct. */
    }
  else
    {
      zMapLogCritical("%s", "unknown client event in dataEventCB() handler\n") ;
    }


  return ;
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


ZMapWindow zMapWindowCreateZMapWindow(void)
{
  ZMapWindow window = (ZMapWindow)malloc(sizeof(ZMapWindowStruct));

  return window;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
STORE_HANDLE zMapWindowGetHandle(ZMapWindow window)
{
  return window->handle;
}


void zMapWindowSetHandle(ZMapWindow window)
{
  //  window->handle = handleCreate();
  window->handle = NULL;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void zMapWindowCreateRegion(ZMapWindow window)
{
  window->focuspane->zMapRegion = (ZMapRegion*)malloc(sizeof(ZMapRegion));
  return;
}


GNode *zMapWindowGetPanesTree(ZMapWindow window)
{
  return window->panesTree;
}

void zMapWindowSetPanesTree(ZMapWindow window, GNode *node)
{
  window->panesTree = node;
  return;
}

gboolean zMapWindowGetFirstTime(ZMapWindow window)
{
  return window->firstTime;
}

void zMapWindowSetFirstTime(ZMapWindow window, gboolean value)
{
  window->firstTime = value;
  return;
}

ZMapPane zMapWindowGetFocuspane(ZMapWindow window)
{
  return window->focuspane;
}

void zMapWindowSetFocuspane(ZMapWindow window, ZMapPane pane)
{
  window->focuspane = pane;
  return;
}

int zMapWindowGetHeight(ZMapWindow window)
{
  return window->focuspane->graphHeight;
}

int zMapWindowGetRegionLength(ZMapWindow window)
{
  return window->focuspane->zMapRegion->length;
}

Coord zMapWindowGetRegionArea(ZMapWindow window, int num)
{
  if (num == 1)
    return window->focuspane->zMapRegion->area1;
  else
    return window->focuspane->zMapRegion->area2;
}

void zMapWindowSetRegionArea(ZMapWindow window, Coord area, int num)
{
  if (num == 1)
    window->focuspane->zMapRegion->area1 = area;
  else
    window->focuspane->zMapRegion->area2 = area;

return;
}

gboolean zMapWindowGetRegionReverse(ZMapWindow window)
{
  return window->focuspane->zMapRegion->rootIsReverse;
}


GtkWidget *zMapWindowGetHbox(ZMapWindow window)
{
  return window->hbox;
}


void zMapWindowSetHbox(ZMapWindow window, GtkWidget *hbox)
{
  window->hbox = hbox;
  return;
}

GtkWidget *zMapWindowGetHpane(ZMapWindow window)
{
  return window->hpane;
}

void zMapWindowSetHpane(ZMapWindow window, GtkWidget *hpane)
{
  window->hpane = hpane;
  return;
}

GtkWidget *zMapWindowGetFrame(ZMapWindow window)
{
  return window->frame;
}

void zMapWindowSetFrame(ZMapWindow window, GtkWidget *frame)
{
  window->frame = frame;
  return;
}


GtkWidget *zMapWindowGetVbox(ZMapWindow window)
{
  return window->vbox;
}


void zMapWindowSetVbox(ZMapWindow window, GtkWidget *vbox)
{
  window->vbox = vbox;
  return;
}


void zMapWindowSetBorderWidth(GtkWidget *container, int width)
{
  gtk_container_border_width(GTK_CONTAINER(container), width);
  return;
}


GtkWidget *zMapWindowGetNavigator(ZMapWindow window)
{
  return window->navigator;
}


void zMapWindowSetNavigator(ZMapWindow window, GtkWidget *navigator)
{
  window->navigator = navigator;
  return;
}

FooCanvas *zMapWindowGetNavCanvas(ZMapWindow window)
{
  return window->navcanvas;
}


void zMapWindowSetNavCanvas(ZMapWindow window, FooCanvas *navcanvas)
{
  window->navcanvas = navcanvas;
  return;
}

GtkWidget *zMapWindowGetZoomVbox(ZMapWindow window)
{
  return window->zoomvbox;
}


void zMapWindowSetZoomVbox(ZMapWindow window, GtkWidget *vbox)
{
  window->zoomvbox = vbox;
  return;
}


ScreenCoord zMapWindowGetScaleOffset(ZMapWindow window)
{
  return window->scaleOffset;
}


void zMapWindowSetScaleOffset(ZMapWindow window, ScreenCoord offset)
{
  window->scaleOffset = offset;
  return;
}


Coord zMapWindowGetCoord(ZMapWindow window, char *field)
{
  if (field == "s")
    return window->navStart;
  else
    return window->navEnd;
}

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


int zMapWindowGetRegionSize (ZMapWindow window)
{
  return window->focuspane->zMapRegion->area2 - window->focuspane->zMapRegion->area1;
}


InvarCoord zMapWindowGetOrigin(ZMapWindow window)
{
  return window->origin;
}


// ZMapRegion functions

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


// ZMapPane functions

void zMapPaneNewBox2Col(ZMapPane pane, int elements)
{
  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), elements);

  return ;
}



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


ZMapRegion *zMapPaneGetZMapRegion(ZMapPane pane)
{
  return pane->zMapRegion;
}


FooCanvasItem *zMapPaneGetGroup(ZMapPane pane)
{
  return pane->group;
}


ZMapWindow zMapPaneGetZMapWindow(ZMapPane pane)
{
  return pane->window;
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


/* addPane is called each time we add a new pane. Creates a new frame, scrolled
 * window and canvas.  First time through it sticks them in the window->zoomvbox, 
 * thereafter it packs them into the lower part of the focus pane.
 *
 * Splitting goes like this: we make a (h or v) pane, add a new frame to the child1 
 * position of that, then reparent the scrolled window containing the canvas into 
 * the new frame. That's the shrinkPane() function. 
 * Then in addPane we add a new frame in the child2 position, and into that we
 * load a new scrolled window and canvas. 
*/
void addPane(ZMapWindow window, char orientation)
{
  ZMapPane pane = (ZMapPane)malloc(sizeof(ZMapPaneStruct));
  GtkAdjustment *adj; 
  GtkWidget *w;
  GNode *node = NULL;

  /* set up ZMapPane for this window */
  pane->window = window;
  pane->DNAwidth = 100;
  pane->step_increment = 10;

  /* create the bare graph & convert to a widget */
  /*    pane->graph = graphNakedCreate(TEXT_FIT, "", 20, 100, TRUE);
	pane->graphWidget = gexGraph2Widget(pane->graph);*/
  
  //  pane->cols = arrayHandleCreate(50, ZMapColumn, zMapWindowGetHandle(window));
  //  pane->cols = arrayCreate(50, ZMapColumn);
  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), 50);
  //  pane->box2col        = NULL;
  //  pane->box2seg        = NULL;
  //  pane->drawHandle     = NULL;
  pane->frame          = gtk_frame_new(NULL);
  pane->scrolledWindow = gtk_scrolled_window_new (NULL, NULL);

  /* The idea of the GNode tree is that panes split horizontally, ie
   * one above the other, end up as siblings in the tree, while panes
   * split vertically (side by side) are parents/children.  In theory
   * this enables us to get the sizing right. In practice it's not
   * perfect yet.*/
  if (zMapWindowGetFirstTime(window)) 
    { 
      pane->zoomFactor   = 1;
      g_node_append_data(zMapWindowGetPanesTree(window), pane);
    }
  else
    {
      pane->zoomFactor   = zMapWindowGetFocuspane(window)->zoomFactor;
      node = g_node_find (zMapWindowGetPanesTree(window),
			  G_IN_ORDER,
			  G_TRAVERSE_ALL,
			  zMapWindowGetFocuspane(window));
      if (orientation == 'h')
	  g_node_append_data(node->parent, pane);
      else
	  g_node_append_data(node, pane);
    }

  /* draw the canvas */
  gdk_rgb_init();
  w = foo_canvas_new();

  pane->canvas = FOO_CANVAS(w);
  foo_canvas_set_scroll_region(pane->canvas, 0.0, 0.0, 1000, 1000);
  pane->background = foo_canvas_item_new(foo_canvas_root(pane->canvas),
	 		foo_canvas_rect_get_type(),
			"x1",(double)0,
			"y1",(double)0,
			"x2",(double)1000,
			"y2",(double)1000,
		 	"fill_color", "white",
			"outline_color", "dark gray",
			NULL);
  
  /* when the user clicks a button in the view, call recordFocus() */
  g_signal_connect (GTK_OBJECT (pane->canvas), "button_press_event",
		    GTK_SIGNAL_FUNC (recordFocus), pane);

  pane->group = foo_canvas_item_new(foo_canvas_root(pane->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  /* add the canvas to the scrolled window */
  gtk_container_add(GTK_CONTAINER(pane->scrolledWindow),w);

  /* you have to set the step_increment manually or the scrollbar arrows don't work.*/
  /* Using a member of ZMapPane means I can adjust it if necessary when we zoom. */
  GTK_LAYOUT (w)->vadjustment->step_increment = pane->step_increment;
  GTK_LAYOUT (w)->hadjustment->step_increment = pane->step_increment;

  /* add the scrolled window to the frame */
  gtk_container_add(GTK_CONTAINER(pane->frame),pane->scrolledWindow);

  /* First time through, we add the frame to the main vbox. 
   * Subsequently it goes in the lower half of the current pane. */
  if (zMapWindowGetFirstTime(window))
    gtk_box_pack_start(GTK_BOX(zMapWindowGetZoomVbox(window)), pane->frame, TRUE, TRUE, 0);
  else
    gtk_paned_pack2(GTK_PANED(zMapWindowGetFocuspane(window)->pane), pane->frame, TRUE, TRUE);


  /* always show scrollbars, however big the display */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pane->scrolledWindow),
       GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pane->scrolledWindow)); 

  //  g_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(navUpdate), (gpointer)(pane));
  //  g_signal_connect(GTK_OBJECT(adj), "changed", GTK_SIGNAL_FUNC(navChange), (gpointer)(pane)); 

  /* focus on the new pane */
  recordFocus(NULL, NULL, pane);
  gtk_widget_grab_focus(pane->frame);

  /* if we do this first time, a little blank box appears before the main display */
  if (!zMapWindowGetFirstTime(window)) 
    gtk_widget_show_all (zMapWindowGetFrame(window));

  zMapWindowSetFirstTime(window, FALSE);

  zmMainScale(pane->canvas, 30, 0, 1000);
  return;
}


void drawNavigatorWind(ZMapPane pane)
{
  ZMapWindow window = pane->window;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  ScreenCoord startScreenf, endScreenf;
  ScreenCoord pos;
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pane->scrolledWindow));


  //  graphFitBounds(NULL, &height);
  
  startWind =  zmCoordFromScreen(pane, 0);
  endWind   =  zmCoordFromScreen(pane, pane->graphHeight);
  
  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
  endWindf   = zMapWindowGetScreenCoord(window, endWind, height);
  lenWindf   = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
  endScreenf   = startWindf + lenWindf * (adj->page_size + adj->value - adj->lower)/(adj->upper - adj->lower) ;
  
  //  graphColor(BLACK);
  
  if (pane == window->focuspane)
    pos = zMapWindowGetScaleOffset(window);
  else
    pos = zMapWindowGetScaleOffset(window);

  /* for now we'll not actually draw anything */
  //  pane->dragBox = graphBoxStart();
  //  graphLine(pos, startWindf, pos, endWindf);
  //  pane->scrollBox = graphBoxStart();
  //  graphColor(GREEN);
  //  graphFillRectangle(pos - 0.3, startScreenf, pos + 0.5, endScreenf);
  //  graphBoxEnd();
  //  graphBoxSetPick(pane->scrollBox, FALSE);
  //  graphBoxEnd();
  //  graphBoxDraw(pane->dragBox, -1, LIGHTGRAY);

  return;
}

/****************** end of file ************************************/
