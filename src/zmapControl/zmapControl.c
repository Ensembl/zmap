/*  File: zmapControl.c
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
 * Description: This is the ZMap interface code, it controls both
 *              the window code and the threaded server code.
 * Exported functions: See ZMap.h
 * HISTORY:
 * Last edited: Jul  2 19:06 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.14 2004-07-02 18:23:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>

#include <gtk/gtk.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>


/* ZMap debugging output. */
gboolean zmap_debug_G = TRUE ; 


static ZMap createZMap(void *app_data, ZMapCallbackFunc app_cb) ;
static void destroyZMap(ZMap zmap) ;

static void createZMapWindow(ZMap zmap) ;

static void killZMap(ZMap zmap) ;
static void viewKilledCB(ZMapView view, void *app_data) ;
static void killFinal(ZMap zmap) ;
static void killViews(ZMap zmap) ;

static gboolean findViewInZMap(ZMap zmap, ZMapView view) ;
static ZMapView addView(ZMap zmap, char *sequence) ;


/*
 *  ------------------- External functions -------------------
 *   (includes callbacks)
 */



/* Create a new zmap which is blank with no views. Returns NULL on failure.
 * Note how I casually assume that none of this can fail. */
ZMap zMapCreate(void *app_data, ZMapCallbackFunc app_zmap_destroyed_cb)
{
  ZMap zmap = NULL ;

  zmap = createZMap(app_data, app_zmap_destroyed_cb) ;

  /* Make the main/toplevel window for the ZMap. */
  zmapControlWindowCreate(zmap, zmap->zmap_id) ;

  zmap->state = ZMAP_INIT ;

  return zmap ;
}


ZMapView zMapAddView(ZMap zmap, char *sequence)
{
  ZMapView view ;

  zMapAssert(zmap && sequence && *sequence) ;

  view = addView(zmap, sequence) ;

  return view ;
}


gboolean zMapConnectView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  result = zMapViewConnect(view) ;
  
  return result ;
}


/* We need a load or connect call here..... */
gboolean zMapLoadView(ZMap zmap, ZMapView view)
{
  printf("not implemented\n") ;

  return FALSE ;
}

gboolean zMapStopView(ZMap zmap, ZMapView view)
{
  printf("not implemented\n") ;

  return FALSE ;
}



gboolean zMapDeleteView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  zmap->view_list = g_list_remove(zmap->view_list, view) ;

  zMapViewDestroy(view) ;
  
  return result ;
}



/* Reset an existing ZMap, this call will:
 * 
 *    - Completely reset a ZMap window to blank with no sequences displayed and no
 *      threads attached or anything.
 *    - Frees all ZMap window data
 *    - Kills all existings server threads etc.
 * 
 * After this call the ZMap will be ready for the user to specify a new sequence to be
 * loaded.
 * 
 *  */
gboolean zMapReset(ZMap zmap)
{
  gboolean result = FALSE ;

  if (zmap->state == ZMAP_VIEWS)
    {
      killViews(zmap) ;

      zmap->state = ZMAP_RESETTING ;

      result = TRUE ;
    }

  return result ;
}



/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id = NULL ;

  if (zmap->state != ZMAP_DYING)
    id = zmap->zmap_id ;

  return id ;
}


char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static char *zmapStates[] = {"ZMAP_INIT", "ZMAP_VIEWS", "ZMAP_RESETTING",
			       "ZMAP_DYING"} ;
  char *state ;

  zMapAssert(zmap->state >= ZMAP_INIT && zmap->state <= ZMAP_DYING) ;

  state = zmapStates[zmap->state] ;

  return state ;
}


/* Called to kill a zmap window and get all associated windows/threads destroyed.
 */
gboolean zMapDestroy(ZMap zmap)
{
  killZMap(zmap) ;

  return TRUE ;
}



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing this would just send a kill to all the views but not wait for the
 * the reply but is complicated by toplevel window potentially being removed before the views
 * windows... */




/* These functions are internal to zmapControl and get called as a result of user interaction
 * with the gui. */


/* Called when the user kills the toplevel window of the ZMap either by clicking the "quit"
 * button or by using the window manager frame menu to kill the window. */
void zmapControlTopLevelKillCB(ZMap zmap)
{
  /* this is not strictly correct, there may be complications if we are resetting, sort this
   * out... */

  if (zmap->state != ZMAP_DYING)
    killZMap(zmap) ;

  return ;
}



/* We need a callback for adding a new view here....scaffold the interface for now, just send some
 * text from a "new" button. */




void zmapControlLoadCB(ZMap zmap)
{

  /* We can only load something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error 
       * if no view is selected....perhaps there should always be a selected view. */

      /* Probably should also allow "load"...perhaps time to call this "Reload"... */
      if (zmap->curr_view)
	{
	  gboolean status = TRUE ;

	  if (zmap->curr_view && zMapViewGetStatus(zmap->curr_view) == ZMAPVIEW_INIT)
	    status = zMapViewConnect(zmap->curr_view) ;

	  if (status && zMapViewGetStatus(zmap->curr_view) == ZMAPVIEW_RUNNING)
	    zMapViewLoad(zmap->curr_view, "") ;
	}
    }

  return ;
}


void zmapControlResetCB(ZMap zmap)
{

  /* We can only reset something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error 
       * if no view is selected....perhaps there should always be a selected view. */
      if (zmap->curr_view && zMapViewGetStatus(zmap->curr_view) == ZMAPVIEW_RUNNING)
	{
	  zMapViewReset(zmap->curr_view) ;
	}
    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* this is not right, we may not need the resetting state at top level at all in fact..... */
  zmap->state = ZMAP_RESETTING ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


/* Put new data into a View. */
void zmapControlNewCB(ZMap zmap, char *testing_text)
{
  gboolean status = FALSE ;

  if (zmap->state == ZMAP_INIT || zmap->state == ZMAP_VIEWS)
    {
      ZMapView view ;

      view = addView(zmap, testing_text) ;
    }

  return ;
}


/* For now, just moving functions from zmapWindow/zmapcontrol.c */

/* zMapDisplay
 * Main entry point for the zmap code.  Called by zMapWindowCreate.
 * The first param is the display window, then two callback routines to 
 * allow zmap to interrogate a data-source. Then a void pointer to a
 * structure used in the process.  Although zmap doesn't need to know
 * directly about this structure, it needs to pass the pointer back
 * during callbacks, so AceDB can use it. 
 *
 * This will all have to change, now we're acedb-independent.
 *
 * We create the display window, then call the Activate 
 * callback routine to get the data, passing it a ZMapRegion in
 * which to create fmap-flavour segs for us to display, then
 * build the columns in the display.
 */

gboolean zMapDisplay(ZMap        zmap,
		 Activate_cb act_cb,
		 Calc_cb     calc_cb,
		 void       *region,
		 char       *seqspec, 
		 char       *fromspec, 
		 gboolean        isOldGraph)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindow window = zmap->zMapWindow;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  //  Coord x1, x2;    
  //  zMapWindowSetHandle(window);

  zmap->frame = zmap->view_parent ;        

  zmap->firstTime = TRUE ;				    /* used in addPane() */

  /* make the window in which to display the data */
  createZMapWindow(zmap);

  drawNavigator(zmap);

  drawWindow(zmap->focuspane) ;

  return TRUE;

}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Has to become redundant.......I hope......... */

ZMapWindow zMapWindowCreateZMapWindow(void)
{
  ZMapWindow window = (ZMapWindow)malloc(sizeof(ZMapWindowStruct));

  return window;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/* createZMapWindow ***************************************************************
 * Creates the root node in the panesTree (which helps keep track of all the
 * display panels).  The root node has no data, only children.
 * 
 * Puts an hbox into vbox1, then packs 2 toolbars into the hbox.  We may not want
 * to keep it like that.  Then puts an hpane below that and stuffs the navigator
 * in as child1.  Calls zMapZoomToolbar to build the rest and puts what it does
 * in as child2.
 */

static void createZMapWindow(ZMap zmap)
{
  ZMapPane pane = NULL;
 
  zmap->panesTree = g_node_new(pane);

  zmap->hpane = gtk_hpaned_new() ;
                                                                                           
  /* After the toolbars comes an hpane, so the user can adjust the width
   * of the navigator pane */
  gtk_container_add(GTK_CONTAINER(zmap->frame), zmap->hpane) ;
                                                                                           
  zmap->navigator = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(zmap->navigator), 
				 GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_widget_set_size_request(zmap->navigator, 100, -1);


  gtk_paned_pack1(GTK_PANED(zmap->hpane), 
		  zmap->navigator,
                  TRUE, TRUE);
                                                                                           
  /* create the splittable pane and pack it into the hpane as child2. */
  zmap->displayvbox = gtk_vbox_new(FALSE,0) ;

  addPane(zmap, 'v');

  gtk_widget_set_size_request(zmap->displayvbox, 750, -1);
  gtk_paned_pack2(GTK_PANED(zmap->hpane), 
		  zmap->displayvbox, TRUE, TRUE);

  return;
}



void drawWindow(ZMapPane pane)
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
void addPane(ZMap zmap, char orientation)
{
  ZMapPane pane = (ZMapPane)malloc(sizeof(ZMapPaneStruct));
  GtkAdjustment *adj; 
  GtkWidget *w;
  GNode *node = NULL;

  /* set up ZMapPane for this window */
  pane->zmap = zmap ;
  pane->DNAwidth = 100;
  pane->step_increment = 10;

  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), 50);
  pane->frame          = gtk_frame_new(NULL);
  pane->scrolledWindow = gtk_scrolled_window_new (NULL, NULL);

  /* The idea of the GNode tree is that panes split horizontally, ie
   * one above the other, end up as siblings in the tree, while panes
   * split vertically (side by side) are parents/children.  In theory
   * this enables us to get the sizing right. In practice it's not
   * perfect yet.*/
  if (zmap->firstTime) 
    { 
      pane->zoomFactor   = 1;
      g_node_append_data(zmap->panesTree, pane);
    }
  else
    {
      pane->zoomFactor  = zmap->focuspane->zoomFactor ;
      node = g_node_find (zmap->panesTree,
			  G_IN_ORDER,
			  G_TRAVERSE_ALL,
			  zmap->focuspane);
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
  if (zmap->firstTime)
    gtk_box_pack_start(GTK_BOX(zmap->displayvbox), pane->frame, TRUE, TRUE, 0);
  else
    gtk_paned_pack2(GTK_PANED(zmap->focuspane->pane), pane->frame, TRUE, TRUE);


  /* always show scrollbars, however big the display */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(pane->scrolledWindow),
       GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pane->scrolledWindow)); 

  g_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(navUpdate), (gpointer)(pane));
  g_signal_connect(GTK_OBJECT(adj), "changed", GTK_SIGNAL_FUNC(navChange), (gpointer)(pane)); 

  /* focus on the new pane */
  recordFocus(NULL, NULL, pane);
  gtk_widget_grab_focus(pane->frame);

  /* if we do this first time, a little blank box appears before the main display */
  if (!zmap->firstTime) 
    gtk_widget_show_all(zmap->frame) ;

  zmap->firstTime = FALSE ;

  zmMainScale(pane->canvas, 30, 0, 1000);
  return;
}



void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p;
  
  drawNavigator(pane->zmap) ;
}



void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p ;
  ZMap zmap = pane->zmap ;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (GTK_WIDGET_REALIZED(zmap->frame))
    {

      //  graphActivate(zMapWindowGetNavigator(window));
      //  graphFitBounds(NULL, &height);
      //  graphBoxDim(pane->scrollBox, &x1, &y1, &x2, &y2);
      
      //  startWind =  zmCoordFromScreen(pane, 0);
      //  endWind =  zmCoordFromScreen(pane, zMapPaneGetHeight(pane));
      
      //  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
      //  endWindf = zMapWindowGetScreenCoord(window, endWind, height);
      //  lenWindf = endWindf - startWindf;
      
      //  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
      
      //  graphBoxShift(pane->scrollBox, x1, startScreenf);
    }

  return ;
}




void drawNavigator(ZMap zmap)
{
  GtkWidget *w;
  GtkRequisition req;
  
  w = foo_canvas_new();

  zmap->navcanvas = FOO_CANVAS(w) ;
  foo_canvas_set_scroll_region(zmap->navcanvas, 0.0, 0.0, 200.0, 500.0);
 
  foo_canvas_item_new(foo_canvas_root(zmap->navcanvas),
			foo_canvas_rect_get_type(),
			"x1",(double)0.0,
			"y1",(double)0.0,
			"x2",(double)200.0,
			"y2",(double)500.0,
			"fill_color", "white",
			NULL);

  gtk_container_add(GTK_CONTAINER(zmap->navigator), w);

}

/* I'll need to put this somewhere else soon enough */
void navScale(FooCanvas *canvas, float offset, int start, int end)
{
  int x, width = 5, count;
  FooCanvasItem *group;

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			foo_canvas_group_get_type(),
			"x",(double)offset,
			"y",(double)0.0,
			NULL);
 
  for (x = start, count = 1 ; x < end ; x += 10, count++)
    {
      drawLine(FOO_CANVAS_GROUP(group), offset-5, x, offset, x, "black", 1.0);
      char text[25];
      sprintf(text,"%dk", x);
      if (count == 1)
	displayText(FOO_CANVAS_GROUP(group), text, offset + 20, x); 
      if (count > 9) count = 0;
    }
			     
  drawLine(FOO_CANVAS_GROUP(group), offset+1, 0, offset+1, end, "black", 1.0);
  return;

}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* UNUSED CURRENTLY..... */
static void navResize(void)
{
  ZMapWindow window;
  
  //  if (graphAssFind(&navAssoc, &window))
    drawNavigator(window);
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS PROBABLY NEEDS TO BE REWRITTEN AND PUT IN ZMAPDRAW.C, THE WHOLE CONCEPT OF GRAPHHEIGHT IS
 * ALMOST CERTAINLY DEFUNCT NOW....... */

/* Not entirely convinced this is the right place for these
** public functions accessing private structure members
*/
int zMapWindowGetHeight(ZMapWindow window)
{
  return zmap->focuspane->graphHeight;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




// ZMapPane functions

void zMapPaneNewBox2Col(ZMapPane pane, int elements)
{
  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), elements);

  return ;
}




/* MOVED FROM zmapWindow.c, THEY BE IN THE WRONG PLACE OR NOT EVEN NEEDED...... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapPane zMapWindowGetFocuspane(ZMapWindow window)
{
  return window->focuspane;
}

void zMapWindowSetFocuspane(ZMapWindow window, ZMapPane pane)
{
  window->focuspane = pane;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GNode *zMapWindowGetPanesTree(ZMapWindow window)
{
  return window->panesTree;
}

void zMapWindowSetPanesTree(ZMapWindow window, GNode *node)
{
  window->panesTree = node;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapWindowGetFirstTime(ZMapWindow window)
{
  return window->firstTime;
}

void zMapWindowSetFirstTime(ZMapWindow window, gboolean value)
{
  window->firstTime = value;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetHpane(ZMapWindow window)
{
  return window->hpane;
}

void zMapWindowSetHpane(ZMapWindow window, GtkWidget *hpane)
{
  window->hpane = hpane;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetNavigator(ZMapWindow window)
{
  return window->navigator;
}


void zMapWindowSetNavigator(ZMapWindow window, GtkWidget *navigator)
{
  window->navigator = navigator;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetFrame(ZMapWindow window)
{
  return window->frame;
}

void zMapWindowSetFrame(ZMapWindow window, GtkWidget *frame)
{
  window->frame = frame;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetVbox(ZMapWindow window)
{
  return window->vbox;
}


void zMapWindowSetVbox(ZMapWindow window, GtkWidget *vbox)
{
  window->vbox = vbox;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
FooCanvas *zMapWindowGetNavCanvas(ZMapWindow window)
{
  return window->navcanvas;
}


void zMapWindowSetNavCanvas(ZMapWindow window, FooCanvas *navcanvas)
{
  window->navcanvas = navcanvas;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetDisplayVbox(ZMapWindow window)
{
  return window->displayvbox;
}


void zMapWindowSetDisplayVbox(ZMapWindow window, GtkWidget *vbox)
{
  window->displayvbox = vbox;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
GtkWidget *zMapWindowGetHbox(ZMapWindow window)
{
  return window->hbox;
}


void zMapWindowSetHbox(ZMapWindow window, GtkWidget *hbox)
{
  window->hbox = hbox;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapWindow zMapPaneGetZMapWindow(ZMapPane pane)
{
  return pane->window;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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



/*
 *  ------------------- Internal functions -------------------
 */


static ZMap createZMap(void *app_data, ZMapCallbackFunc app_cb)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new(ZMapStruct, sizeof(ZMapStruct)) ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("%d", zmap_num) ;

  zmap->curr_view = NULL ;
  zmap->view_list = NULL ;

  zmap->app_data = app_data ;
  zmap->app_zmap_destroyed_cb = app_cb ;

  return zmap ;
}


/* This is the strict opposite of createZMap(), should ONLY be called once all of the Views
 * and other resources have gone. */
static void destroyZMap(ZMap zmap)
{
  g_free(zmap->zmap_id) ;

  g_free(zmap) ;

  return ;
}




/* This routine gets called when, either via a direct call or a callback (user action) the ZMap
 * needs to be destroyed. It does not do the whole destroy but instead signals all the Views
 * to die, when they die they call our view_detroyed_cb and this will eventually destroy the rest
 * of the ZMap (top level window etc.) when all the views have gone. */
static void killZMap(ZMap zmap)
{
  /* no action, if dying already.... */
  if (zmap->state != ZMAP_DYING)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* set our state to DYING....so we don't respond to anything anymore.... */
      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Call the application callback first so that there will be no more interaction with us */
      (*(zmap->app_zmap_destroyed_cb))(zmap, zmap->app_data) ;

      /* If there are no views we can just go ahead and kill everything, otherwise we just
       * signal all the views to die. */
      if (zmap->state == ZMAP_INIT || zmap->state == ZMAP_RESETTING)
	{
	  killFinal(zmap) ;
	}
      else if (zmap->state == ZMAP_VIEWS)
	{
	  killViews(zmap) ;
	}


      /* set our state to DYING....so we don't respond to anything anymore.... */
      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;
    }

  return ;
}


static ZMapView addView(ZMap zmap, char *sequence)
{
  ZMapView view = NULL ;

  if ((view = zMapViewCreate(zmap->view_parent, sequence, zmap, viewKilledCB)))
    {

      /* HACK FOR NOW, WE WON'T ALWAYS WANT TO MAKE THE LATEST VIEW TO BE ADDED THE "CURRENT" ONE. */
      zmap->curr_view = view ;

      /* add to list of views.... */
      zmap->view_list = g_list_append(zmap->view_list, view) ;
      
      zmap->state = ZMAP_VIEWS ;
    }
  
  return view ;
}



/* Gets called when a ZMapView dies, this is asynchronous because the view has to kill threads
 * and wait for them to die and also because the ZMapView my die of its own accord.
 * BUT NOTE that when this routine is called by the last ZMapView within the fmap to say that
 * it has died then we either reset the ZMap to its INIT state or if its dying we kill it. */
static void viewKilledCB(ZMapView view, void *app_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* A View has died so we should clean up its stuff.... */
  zmap->view_list = g_list_remove(zmap->view_list, view) ;


  /* Something needs to happen about the current view, a policy decision here.... */
  if (view == zmap->curr_view)
    zmap->curr_view = NULL ;


  /* If the last view has gone AND we are dying then we can kill the rest of the ZMap
   * and clean up, otherwise we just set the state to "reset". */
  if (!zmap->view_list)
    {
      if (zmap->state == ZMAP_DYING)
	{
	  killFinal(zmap) ;
	}
      else
	zmap->state = ZMAP_INIT ;
    }

  return ;
}


/* This MUST only be called once all the views have gone. */
static void killFinal(ZMap zmap)
{
  zMapAssert(zmap->state != ZMAP_VIEWS) ;

  /* Free the top window */
  if (zmap->toplevel)
    {
      zmapControlWindowDestroy(zmap) ;
      zmap->toplevel = NULL ;
    }

  destroyZMap(zmap) ;

  return ;
}





static void killViews(ZMap zmap)
{
  GList* list_item ;

  zMapAssert(zmap->view_list) ;

  list_item = zmap->view_list ;
  do
    {
      ZMapView view ;

      view = list_item->data ;

      zMapViewDestroy(view) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  zmap->curr_view = NULL ;

  return ;
}


/* Find a given view within a given zmap. */
static gboolean findViewInZMap(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;
  GList* list_ptr ;

  if ((list_ptr = g_list_find(zmap->view_list, view)))
    result = TRUE ;

  return result ;
}


