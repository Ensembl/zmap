/*  Last edited: May 14 10:34 2004 (rnc) */
/*  file: zmapcontrol.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

#include <zmapcontrol.h>
#include <zmapsplit.h>
static void *navAssoc, *winAssoc;


/* function prototypes ***********************************************/

static void        createZMap       (ZMapWindow *window, ZMapCallbackData *zMapCallbackData);
static void        createZMapWindow (ZMapWindow *window, ZMapCallbackData *zMapCallbackData);
static void        zMapZoomToolbar  (ZMapWindow *window, ZMapCallbackData *zMapCallbackData);
static void        drawNavigator    (ZMapWindow *window);
static void        drawWindow       (ZMapPane *pane);
static void        navUpdate        (GtkAdjustment *adj, gpointer p);
static void        navChange        (GtkAdjustment *adj, gpointer p);
static void        drawNavigatorWind(ZMapPane *pane);
static void        zMapScaleColumn  (ZMapPane *pane, float *offset, int frame);
static void        drawBox          (FooCanvasItem *group, double x, double y, char *line_colour, char *fill_colour);
static void        drawLine         (FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
				     char *colour, double thickness);
static void drawGene(ZMapWindow *window);
static void columnOfBoxes(ZMapWindow *window);
static float zmMainScale(FooCanvas *canvas, float offset, int start, int end);

/* functions ********************************************************/

BOOL Quit(GtkWidget *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;

  if (window->panesTree)
    g_node_destroy(window->panesTree);

  return TRUE;
}



/* zMapDisplay
 * Main entry point for the zmap code.  Called by zMapCall() zmapcalls.c.
 * The first two params are callback routines to allow zmap to 
 * interrogate a data-source, the third being a void pointer to a
 * structure used in the process.  Although zmap doesn't need to know
 * directly about this structure, it needs to pass the pointer back
 * during callbacks, so AceDB can use it. 
 *
 * We create the display window, then call the Activate 
 * callback routine to get the data, passing it a ZMapRegion in
 * which to create fmap-flavour segs for us to display, then
 * build the columns in the display.
 */

BOOL zMapDisplay(Activate_cb act_cb,
		 Calc_cb calc_cb,
		 void *region,
		 char *seqspec, 
		 char *fromspec, 
		 BOOL isOldGraph)
{

  ZMapWindow *window = (ZMapWindow*)messalloc(sizeof(ZMapWindow));
  Coord x1, x2;
    
  window->handle = handleCreate();
  window->firstTime = TRUE;                 /* used in addPane() */
  ZMapCallbackData *zMapCBData = halloc(sizeof(ZMapCallbackData), window->handle);

  zMapCBData->calc_cb   = calc_cb;
  zMapCBData->seqRegion = region;

  /* make the window in which to display the data */
  createZMap(window, zMapCBData);

  /* Make zmapRegion  - just zmap stuff, no AceDB at all. */
  window->focuspane->zMapRegion = srCreate(window->handle);
  
  /* Poke stuff in.  For xace this will be srActivate(). */
  (*act_cb)(region, seqspec, window->focuspane->zMapRegion, &x1, &x2, &window->handle);
  
  /* stick the data into the display window */
  makezMapDefaultColumns(window->focuspane);
  buildCols(window->focuspane);

  window->origin = srInvarCoord(window->focuspane->zMapRegion, x1);
  window->focuspane->centre = srInvarCoord(window->focuspane->zMapRegion, x1);
  window->focuspane->basesPerLine = 100;
    
  gtk_widget_show_all(window->window);

  drawNavigator(window);

  drawWindow(window->focuspane);

  return TRUE;

}


/* zmRecalculate *******************************************************/

static BOOL zmRecalculate(ZMapWindow *window, ZMapCallbackData *zMapCBData)
{
  /* derive the region for which we need data. */
  int min, max;

  Calc_cb calc_cb = zMapCBData->calc_cb;

  min = zmCoordFromScreen(window->focuspane, 0);
  max = zmCoordFromScreen(window->focuspane, window->focuspane->graphHeight);

  if (min < 0)
    min = 0;
  if (max > window->focuspane->zMapRegion->length)
    max = window->focuspane->zMapRegion->length;

  if (min >= window->focuspane->zMapRegion->area1 &&
      max <= window->focuspane->zMapRegion->area2)
    return FALSE; /* already covers area. */
  
  min -= 100000;
  max += 100000; /* TODO elaborate this */

  if (min < 0)
    min = 0;
  if (max > window->focuspane->zMapRegion->length)
    max = window->focuspane->zMapRegion->length;

  (*calc_cb)(zMapCBData->seqRegion, min, max, window->focuspane->zMapRegion->rootIsReverse);  

  buildCols(window->focuspane);
  
  return TRUE;
}


void zmRegBox(ZMapPane *pane, int box, ZMapColumn *col, void *arg)
{
  array(pane->box2col, box, ZMapColumn *) = col;
  array(pane->box2seg, box, void *) = arg;
}


static void zMapPick(int box, double x, double y)
{
  ZMapColumn *col;
  void *seg;
  static ZMapPane *oldWindow = NULL;
  static int oldBox = 0;
  
  if (oldWindow && oldBox)
    {
      col = arr(oldWindow->box2col, oldBox, ZMapColumn *);
      seg = arr(oldWindow->box2seg, oldBox, void *);
      if (col && seg && col->selectFunc)
	(*col->selectFunc)(oldWindow, col, seg, oldBox, x, y, FALSE);
      oldBox = 0;
      oldWindow = NULL;
    }
  
  if (graphAssFind(&winAssoc, &oldWindow))
    {
      oldBox = box;
      col = arr(oldWindow->box2col, oldBox, ZMapColumn *);
      seg = arr(oldWindow->box2seg, oldBox, void *);

      if (col && seg && col->selectFunc)
	(*col->selectFunc)(oldWindow, col, seg, oldBox, x, y, TRUE);
    }

}



static void drawWindow(ZMapPane *pane)
{
  float offset = 5;
  float maxOffset = 0;
  int   frameCol, i, frame = -1;
  float oldPriority = -100000;
  
  graphActivate(pane->graph); 
  graphClear();
  graphColor(BLACK);
  graphRegister (PICK,(GraphFunc) zMapPick);
 
  pane->box2col = arrayReCreate(pane->box2col, 500, ZMapColumn *);
  pane->box2seg = arrayReCreate(pane->box2seg, 500, SEG *);
  if (pane->drawHandle)
    handleDestroy(pane->drawHandle);
  pane->drawHandle = handleHandleCreate(pane->window->handle);

  for (i = 0; i < arrayMax(pane->cols); i++)
   
    { 
      ZMapColumn *col = arrp(pane->cols, i, ZMapColumn);
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
	  col = arrp(pane->cols, i, ZMapColumn);
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

  graphTextBounds(maxOffset, 0);
  
  graphRedraw();
  
}
  
	      
static void gtk_ifactory_cb (gpointer             callback_data,
		 guint                callback_action,
		 GtkWidget           *widget)
{
  g_message ("ItemFactory: activated \"%s\"", gtk_item_factory_path_from_widget (widget));
}



/* createZMap ***********************************************************
 * Draws the basics of the window - the trim around it, etc.  
 * Creates the main menu and a text entry box, then callse createZMapWindow.
 */
static void createZMap(ZMapWindow *window, ZMapCallbackData *zMapCallbackData)
{
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  static GtkItemFactoryEntry menu_items[] =
  {
    { "/_File",            NULL,         0,                     0, "<Branch>" },
    { "/File/_Print",      "<control>P", gtk_ifactory_cb,       0 },
    { "/File/Print _Whole","<control>W", gtk_ifactory_cb,       0 },
    { "/File/P_reserve",   "<control>r", gtk_ifactory_cb,       0 },
    { "/File/_Recalculate",NULL,         gtk_ifactory_cb,       0 },
    { "/File/sep1",        NULL,         gtk_ifactory_cb,       0, "<Separator>" },
    { "/File/_Quit",       "<control>Q", gtk_ifactory_cb,       0 },
    { "/_Export",         	    	 NULL, 0,               0, "<Branch>" },
    { "/Export/_Features", 		 NULL, gtk_ifactory_cb, 0 },
    { "/Export/_Sequence",             	 NULL, gtk_ifactory_cb, 0 },
    { "/Export/_Transalations",          NULL, gtk_ifactory_cb, 0 },
    { "/Export/_EMBL dump",              NULL, gtk_ifactory_cb, 0 },
    { "/_Help",            NULL,         0,                     0, "<LastBranch>" },
    { "/Help/_About",      NULL,         gtk_ifactory_cb,       0 },
  };

  static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);

  
  /* create top level window */
  window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(GTK_OBJECT(window->window), "destroy", 
      GTK_SIGNAL_FUNC(Quit), (gpointer) window);

  gtk_widget_set_size_request(window->window, 1000, 750); 
  window->vbox1 = gtk_vbox_new(FALSE, 4);
  window->itemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
  window->infoSpace = gtk_entry_new();

  gtk_window_add_accel_group (GTK_WINDOW(window->window), accel_group);
  gtk_item_factory_create_items (window->itemFactory, nmenu_items, menu_items, NULL);
  gtk_container_add(GTK_CONTAINER(window->window), window->vbox1);
  gtk_box_pack_start(GTK_BOX(window->vbox1), 
		     gtk_item_factory_get_widget(window->itemFactory, "<main>"),
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(window->vbox1), window->infoSpace, FALSE, FALSE, 0);

  createZMapWindow(window, zMapCallbackData);
}


 
/* createZMapWindow ***************************************************************
 * Creates the root node in the panesTree (which helps keep track of all the
 * display panels).  The root node has no data, only children.
 * 
 * Puts an hbox into vbox1, then packs 2 toolbars into the hbox.  We may not want
 * to keep it like that.  Then puts an hpane below that and stuffs the navigator
 * in as child1.  Calls zMapZoomToolbar to build the rest and puts what it does
 * in as child2.
 */

static void createZMapWindow(ZMapWindow *window, ZMapCallbackData *zMapCallbackData)
{
  GtkWidget *toolbar = gtk_toolbar_new();
  zMapCallbackData->window = window;
  ZMapPane *pane = NULL;
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
 
  window->panesTree = g_node_new(pane);

  window->hbox = gtk_hbox_new(FALSE, 0);
  window->hpane = gtk_hpaned_new();
                                                                                           
  /* Next into the vbox we pack an hbox to hold two toolbars.  This is
   * so the Close Pane button can be far away from the rest, which I like. */
  gtk_box_pack_start(GTK_BOX(window->vbox1), hbox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
                                                                                           
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "H-Split", "2 panes, one above the other" , NULL, NULL,
                          GTK_SIGNAL_FUNC(splitPane),
                          (gpointer)window);
                                                                                           
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Clear", NULL, NULL, NULL,
                          NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Rev-Comp", NULL, NULL, NULL,
                          NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "DNA", NULL, NULL, NULL,
                          NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "GeneFind", NULL, NULL, NULL,
                          NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Origin", NULL, NULL, NULL,
                          NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "V-Split", "2 panes, side by side", NULL, NULL,
                          GTK_SIGNAL_FUNC(splitHPane),
                          (gpointer)window);
                                                                                           
  toolbar = gtk_toolbar_new();
  gtk_box_pack_end(GTK_BOX(hbox), toolbar, FALSE, FALSE, 0);
                                                                                           
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Close\nPane", NULL, NULL, NULL,
                          GTK_SIGNAL_FUNC(closePane),
                          (gpointer)window);
                                                                                           
  /* After the toolbars comes an hpane, so the user can adjust the width
   * of the navigator pane */
  gtk_box_pack_start(GTK_BOX(window->vbox1), window->hpane, TRUE, TRUE, 0);
                                                                                           
  /* leave window->navigator in place until navcanvas works */
  window->navigator = graphNakedCreate(TEXT_FIT, "", 20, 100, FALSE) ;
  gtk_widget_set_size_request(gexGraph2Widget(window->navigator), 50, -1);
  gtk_paned_pack1(GTK_PANED(window->hpane), gexGraph2Widget(window->navigator),
                  TRUE, TRUE);
                                                                                           
  /* create the splittable pane and pack it into the hpane as child2. */
  zMapZoomToolbar(window, zMapCallbackData);
  gtk_widget_set_size_request(window->zoomvbox, 750, -1);
  gtk_paned_pack2(GTK_PANED(window->hpane), window->zoomvbox, TRUE, TRUE);

  return;
}


/* zMapZoomToolbar ***************************************************************
 * Builds toolbar with Zoom buttons & magnification combo box.  GraphNakedCreate
 * sets up a bare graph.  Calls addPane to add the actual data display panel.
 * I suspect we'll end up putting the zoom buttons on the top menu line.
 */
static void zMapZoomToolbar(ZMapWindow *window, ZMapCallbackData *zMapCallbackData)
{
  
  GtkWidget *toolbar = gtk_toolbar_new();
  
  window->zoomvbox = gtk_vbox_new(FALSE,0);

  /* build the zoom toolbar */
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Zoom In", NULL, NULL, NULL, 
			  GTK_SIGNAL_FUNC(zoomIn), (gpointer) window);

  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Zoom Out", NULL, NULL, NULL, 
			  GTK_SIGNAL_FUNC(zoomOut), (gpointer) window);
  //			  GTK_SIGNAL_FUNC(zoomOut), (gpointer)zMapCallbackData);

  /* add zoom toolbar and scrolling window to vbox */
  gtk_box_pack_start(GTK_BOX(window->zoomvbox), toolbar, FALSE, FALSE, 0);

  addPane(window, 'v');

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
void addPane(ZMapWindow *window, char orientation)
{
  ZMapPane *pane = (ZMapPane*)malloc(sizeof(ZMapPane));
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
  
  pane->cols = arrayHandleCreate(50, ZMapColumn, window->handle);
  pane->box2col        = NULL;
  pane->box2seg        = NULL;
  pane->drawHandle     = NULL;
  pane->frame          = gtk_frame_new(NULL);
  pane->scrolledWindow = gtk_scrolled_window_new (NULL, NULL);

  /* The idea of the GNode tree is that panes split horizontally, ie
   * one above the other, end up as siblings in the tree, while panes
   * split vertically (side by side) are parents/children.  In theory
   * this enables us to get the sizing right. In practice it's not
   * perfect yet.*/
  if (window->firstTime) 
    { 
      pane->zoomFactor   = 1;
      g_node_append_data(window->panesTree, pane);
    }
  else
    {
      pane->zoomFactor   = window->focuspane->zoomFactor;
      node = g_node_find (window->panesTree,
			  G_IN_ORDER,
			  G_TRAVERSE_ALL,
			  window->focuspane);
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
  if (window->firstTime)
    gtk_box_pack_start(GTK_BOX(window->zoomvbox), pane->frame, TRUE, TRUE, 0);
  else
    gtk_paned_pack2(GTK_PANED(window->focuspane->pane), pane->frame, TRUE, TRUE);


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
  if (!window->firstTime) 
    gtk_widget_show_all (window->window);

  window->firstTime = FALSE;

  zmMainScale(pane->canvas, 30, 0, 1000);
  drawGene(window);
  columnOfBoxes(window);

  return;
}


/* This is just a noddy function I used to draw a small box on the canvas */
static void drawBox (FooCanvasItem *group, double x, double y, char *line_colour, char *fill_colour)
{
  foo_canvas_item_new(FOO_CANVAS_GROUP(group),
                        foo_canvas_rect_get_type(),
                        "x1"           , (double)x ,
                        "y1"           , (double)y ,
                        "x2"           , (double)x+14.0,
                        "y2"           , (double)y+3.0,
                        "outline_color", line_colour          ,
                        "fill_color"   , fill_colour          ,
                        "width_units"  , (double)1,
                        NULL);
                                                                                
         
  return;                                                                       
}

/* This is just a noddy function I used to draw a line  on the canvas */
static void drawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
		     char *colour, double thickness)
{
  FooCanvasPoints *points;
									       
 /* allocate a new points array */
  points = foo_canvas_points_new (2);
				                                            
 /* fill out the points */
  points->coords[0] = x1;
  points->coords[1] = y1;
  points->coords[2] = x2;
  points->coords[3] = y2;
 /* draw the line */
  foo_canvas_item_new(group,
			foo_canvas_line_get_type(),
			"points"     , points,
			"fill_color" , colour,
			"width_units", thickness,
			NULL);
		    
 /* free the points array */
  foo_canvas_points_free(points);

  return;
}
                                                                                

static void drawGene(ZMapWindow *window)
{
  FooCanvasItem *group;

  group = foo_canvas_item_new(foo_canvas_root(window->focuspane->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  //group = window->focuspane->group;

  drawBox(group, 0.0, 220.0 ,"light blue", "white");
  drawBox(group, 0.0, 260.0 ,"light blue", "white");
  drawBox(group, 0.0, 300.0 ,"light blue", "white");
  drawBox(group, 0.0, 320.0 ,"light blue", "white");
  drawBox(group, 0.0, 360.0 ,"light blue", "white");

  drawLine(FOO_CANVAS_GROUP(group),  7.0, 223.0, 14.0, 240.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 14.0, 240.0,  7.0, 260.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group),  7.0, 263.0, 14.0, 280.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 14.0, 280.0,  7.0, 300.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group),  7.0, 303.0, 14.0, 310.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 14.0, 310.0,  7.0, 320.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group),  7.0, 323.0, 14.0, 340.0, "light blue", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 14.0, 340.0,  7.0, 360.0, "light blue", 1.0);
  
  drawBox(group, 20.0, 20.0  ,"red", "white");
  drawBox(group, 20.0, 60.0  ,"red", "white");
  drawBox(group, 20.0, 100.0 ,"red", "white");
  drawBox(group, 20.0, 120.0 ,"red", "white");
  drawBox(group, 20.0, 160.0 ,"red", "white");

  drawLine(FOO_CANVAS_GROUP(group), 27.0,  23.0, 34.0,  40.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0,  40.0, 27.0,  60.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0,  63.0, 34.0,  80.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0,  80.0, 27.0, 100.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0, 103.0, 34.0, 110.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 110.0, 27.0, 120.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0, 123.0, 34.0, 140.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 140.0, 27.0, 160.0, "red", 1.0);
 
  drawBox(group, 20.0, 320.0 ,"red", "white");
  drawBox(group, 20.0, 360.0 ,"red", "white");
  drawBox(group, 20.0, 380.0 ,"red", "white");
  drawBox(group, 20.0, 420.0 ,"red", "white");
  drawBox(group, 20.0, 460.0 ,"red", "white");

  drawLine(FOO_CANVAS_GROUP(group), 27.0, 323.0, 34.0, 340.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 340.0, 27.0, 360.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0, 363.0, 34.0, 370.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 370.0, 27.0, 380.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0, 383.0, 34.0, 400.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 400.0, 27.0, 420.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 27.0, 423.0, 34.0, 440.0, "red", 1.0);
  drawLine(FOO_CANVAS_GROUP(group), 34.0, 440.0, 27.0, 460.0, "red", 1.0);
 
  return;
}

               

static void columnOfBoxes(ZMapWindow *window)
{
  int space[12] = {1,6,9,4,5,2,8,3,7,1,6};
  int i, j;
  double y = 0;
  FooCanvasItem *group;

  group = foo_canvas_item_new(foo_canvas_root(window->focuspane->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  //group = window->focuspane->group;

  for (i=0; i<20; i++)
    {
      for (j=0; j<10; j++)
	{
	  drawBox(group, 60.0, y + space[j], "black", "blue");
	  y += space[j];
	}
    }

  group = foo_canvas_item_new(foo_canvas_root(window->focuspane->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  //group = window->focuspane->group;

  for (i=0, y=0; i<20; i++)
    {
      for (j=1; j<11; j++)
	{
	  drawBox(group, 90.0, y + space[j], "black", "purple");
	  y += space[j];
	}
    }

  group = foo_canvas_item_new(foo_canvas_root(window->focuspane->canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  //group = window->focuspane->group;

  for (i=0, y=0; i<20; i++)
    {
      for (j=2; j<12; j++)
	{
	  drawBox(group, 120.0, y + space[j], "black", "green");
	  y += space[j];
	}
    }
  return;
}


static void displayText(FooCanvasGroup *group, char *text, double x, double y)
{
  FooCanvasItem *item = foo_canvas_item_new(group,
						FOO_TYPE_CANVAS_TEXT,
						"x", x, "y", y, "text", text,
						NULL);
  return;
}


/* This is commented out because I started trying to draw a scalebar after
 * I first put in some of the gnomecanvas stuff, but never succeeded completely.
 * However, when I went away to work on splitting the window, I didn't want to
 * throw this away completely. */
static float zmMainScale(FooCanvas *canvas, float offset, int start, int end)
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
      sprintf(text,"%d", x*10);
      if (count == 1)
	displayText(FOO_CANVAS_GROUP(group), text, offset + 20, x); 
      if (count > 9) count = 0;
    }
			     
  drawLine(FOO_CANVAS_GROUP(group), offset+1, 0, offset+1, end, "black", 1.0);
  return offset + width + 4 ;

}




/* This is the original zmDrawScale I inherited which draws a neat scalebar
 * in the navigator.  It used to do so in the main display panel, until I 
 * started using the canvas. */
float zmDrawScale(float offset, int start, int end)
{
  int height;
  int unit, subunit, type, unitType ;
  int x, width = 0 ;
  float mag, cutoff;
  char *cp, unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;

  graphFitBounds(NULL, &height); 

  mag = (float)height/((float)(end - start));
  cutoff = 5/mag;
  unit = subunit = 1.0 ;

  if (cutoff < 0)
    cutoff = -cutoff ;

  while (unit < cutoff)
    { unit *= 2 ;
      subunit *= 5 ;
      if (unit >= cutoff)
	break ;
      unit *= 2.5000001 ;	
      if (unit >= cutoff)
	break ;
      unit *= 2 ;
      subunit *= 2 ;
    }
  subunit /= 10 ;
  if (subunit < 1.0)
    subunit = 1.0 ;

  for (type = 1, unitType = 0 ; unit > 0 && 1000 * type < unit && unitType < 5; 
       unitType++, type *= 1000) ;
  
  if (x>0)
    x = ((start/unit)+1)*unit;
  else
    x = ((start/unit)-1)*unit;  
  
  for (; x < end ; x += unit)
    {
      ScreenCoord y = (float)(x-start)*mag;
      graphLine (offset, y, offset, y) ;
      buf[0] = unitName[unitType] ; buf[1] = 0 ;
      cp = messprintf ("%d%s", x/type, buf) ;
      if (width < strlen (cp))
        width = strlen (cp) ;
      graphText (cp, offset+1.5, y-0.5) ;
    }


  for (x = ((start/unit)-1)*unit ; x < end ; x += subunit)
    {
      ScreenCoord y = (float)(x-start)*mag;
      graphLine (offset+0.5, y, offset+1, y) ;
    }
			     
  graphLine (offset+1, 0, offset+1, height) ;
  return offset + width + 4 ;

}


static int dragBox;

/* I believe is only called by navPick */
static void navDrag(float *x, float *y, BOOL isDone)
{
  static BOOL isDragging = FALSE;
  static float oldY;
  ZMapWindow *window;
  ZMapPane *pane;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  int height;
  
  graphFitBounds(NULL, &height);
  graphAssFind(&navAssoc, &window);

  if (dragBox == window->focuspane->dragBox)
    {
      pane = window->focuspane;
      *x = window->scaleOffset - 0.3;
    }
  else
    return;

  startWind =  zmCoordFromScreen(pane, 0);
  endWind =  zmCoordFromScreen(pane, pane->graphHeight);
  
  startWindf = height * (startWind - window->navStart)/(window->navEnd - window->navStart);
  endWindf = height * (endWind - window->navStart)/(window->navEnd - window->navStart);
  lenWindf = endWindf - startWindf;
  
  
  if (!isDragging)
    {
      oldY = *y;
      isDragging = TRUE;
    }
 
  if (*y < 0.0)
    *y = 0.0;
  else if (*y > height - lenWindf)
    *y = height - lenWindf;

  if (isDone)
    {
      isDragging = FALSE;
      pane->centre = srInvarCoord(window->focuspane->zMapRegion, 
				  srCoord(window->focuspane->zMapRegion, pane->centre) -
				  (oldY - *y) * (float)(window->navEnd - window->navStart)/(float)height);

      /* TO DO: how do I get a zMapCBData into navDrag?
	 ANS: when I convert the graphRegister to a g_signal_connect I can do that. 
      if (zmRecalculate(window, zMapCBData))
	drawNavigator(window);
      */
      printf("Well, I'm in navDrag\n");

      drawWindow(pane); 
      graphActivate(window->navigator);
    }
  
}

static void navPick(int box, double x, double y)
{
  ZMapWindow *window;

  graphAssFind(&navAssoc, &window);

  if (box == window->focuspane->dragBox)
    {
      dragBox = box;
      graphBoxDrag(box, navDrag);
    }
}

static void navResize(void)
{
  ZMapWindow *window;
  
  if (graphAssFind(&navAssoc, &window))
    drawNavigator(window);
}

static void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapPane *pane = (ZMapPane *)p;
  
  drawNavigator(pane->window);
}



static void drawNavigator(ZMapWindow *window)
{
  int height;
  ScreenCoord startCalcf, endCalcf;
  int areaSize;
  
  graphActivate(window->navigator);
  graphAssociate(&navAssoc, window);
  graphRegister (PICK,(GraphFunc) navPick) ;
  graphRegister(RESIZE, (GraphFunc) navResize);
  graphClear();

  graphFitBounds(NULL, &height);
  
  areaSize = window->focuspane->zMapRegion->area2 - window->focuspane->zMapRegion->area1;
  if (areaSize < 1) areaSize = 1;

  window->navStart = window->focuspane->zMapRegion->area1 - areaSize/2;
  window->navEnd = window->focuspane->zMapRegion->area2 + areaSize/2;
  if (window->navStart == window->navEnd) window->navEnd = window->navStart + 1;

  startCalcf = height * (window->focuspane->zMapRegion->area1 - window->navStart)/(window->navEnd - window->navStart);
  endCalcf = height * (window->focuspane->zMapRegion->area2 - window->navStart)/(window->navEnd - window->navStart);
  
    graphColor(LIGHTGRAY);
    graphFillRectangle(0, startCalcf, 100.0, endCalcf);
      /*
  foo_canvas_item_new(foo_canvas_window(window->navcanvas),
			foo_canvas_rect_get_type(),
			"x1",(double)0,
			"y1",(double)startCalcf,
			"x2",(double)100,
			"y2",(double)endCalcf,
			"fill_color", "lightgray",
			NULL);
      */
  graphColor(BLACK);

  window->scaleOffset = zmDrawScale(1.0, window->navStart, window->navEnd);
  drawNavigatorWind(window->focuspane);

  graphRedraw();
}


static void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapPane *pane = (ZMapPane *)p;
  ZMapWindow *window = pane->window;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (!GTK_WIDGET_REALIZED(window->window))
    return;

  graphActivate(window->navigator);
  graphFitBounds(NULL, &height);
  graphBoxDim(pane->scrollBox, &x1, &y1, &x2, &y2);

  startWind =  zmCoordFromScreen(pane, 0);
  endWind =  zmCoordFromScreen(pane, pane->graphHeight);
  
  startWindf = height * (startWind - window->navStart)/(window->navEnd - window->navStart);
  endWindf = height * (endWind - window->navStart)/(window->navEnd - window->navStart);
  lenWindf = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;

  graphBoxShift(pane->scrollBox, x1, startScreenf);
}



static void drawNavigatorWind(ZMapPane *pane)
{
  ZMapWindow *window = pane->window;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  ScreenCoord startScreenf, endScreenf;
  ScreenCoord pos;
  
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pane->scrolledWindow));
  graphFitBounds(NULL, &height);
  
  startWind =  zmCoordFromScreen(pane, 0);
  endWind =  zmCoordFromScreen(pane, pane->graphHeight);
  
  startWindf = height * (startWind - window->navStart)/(window->navEnd - window->navStart);
  endWindf = height * (endWind - window->navStart)/(window->navEnd - window->navStart);
  lenWindf = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
  endScreenf = startWindf + lenWindf * (adj->page_size + adj->value - adj->lower)/(adj->upper - adj->lower) ;
  
  graphColor(BLACK);
  
  if (pane == window->focuspane)
    pos = window->scaleOffset;
  else
    pos = window->scaleOffset + 1.0;

  pane->dragBox = graphBoxStart();
  graphLine(pos, startWindf, pos, endWindf);
  pane->scrollBox = graphBoxStart();
  graphColor(GREEN);
  graphFillRectangle(pos - 0.3, startScreenf, pos + 0.5, endScreenf);
  graphBoxEnd();
  graphBoxSetPick(pane->scrollBox, FALSE);
  graphBoxEnd();
  graphBoxDraw(pane->dragBox, -1, LIGHTGRAY);

}

/* Coordinate stuff ****************************************************/

VisibleCoord zmVisibleCoord(ZMapWindow *window, Coord coord)
{
  return coord - srCoord(window->focuspane->zMapRegion, window->origin) + 1;
}


ScreenCoord zmScreenCoord(ZMapPane *pane, Coord coord)
{
  Coord basesFromCent = coord - srCoord(pane->window->focuspane->zMapRegion, pane->centre);
  float linesFromCent = ((float)basesFromCent)/((float)pane->basesPerLine);

  return linesFromCent + (float)(pane->graphHeight/2);
}


Coord zmCoordFromScreen(ZMapPane *pane, ScreenCoord coord)
{
  float linesFromCent = coord - (pane->graphHeight/2);
  int basesFromCent = linesFromCent * pane->basesPerLine;

  return srCoord(pane->zMapRegion, pane->centre) + basesFromCent;
}


BOOL zmIsOnScreen(ZMapPane *pane, Coord coord1, Coord coord2)
{
  if (zmScreenCoord(pane, coord2) < 0)
    return FALSE;

  if (zmScreenCoord(pane, coord1) > pane->graphHeight)
    return FALSE;

  return TRUE;
}

/************************** end of file *********************************/
