/*  Last edited: Jun 28 13:59 2004 (rnc) */
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

static void        createZMap       (ZMapWindow window);
static void        zMapZoomToolbar  (ZMapWindow window);
static void        drawNavigator    (ZMapWindow window);
static void        drawWindow       (ZMapPane pane);
static void        drawGene         (ZMapWindow window);
static void        columnOfBoxes    (ZMapWindow window);
static void        navScale         (FooCanvas *canvas, float offset, int start, int end);

/* functions ********************************************************/

BOOL Quit(GtkWidget *widget, gpointer data)
{
  ZMapWindow window = (ZMapWindow)data;

  if (zMapWindowGetPanesTree(window))
    g_node_destroy(zMapWindowGetPanesTree(window));

  return TRUE;
}



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

BOOL zMapDisplay(ZMap        zmap,
		 Activate_cb act_cb,
		 Calc_cb     calc_cb,
		 void       *region,
		 char       *seqspec, 
		 char       *fromspec, 
		 BOOL        isOldGraph)
{
  ZMapWindow window = zmap->zMapWindow;

  //  Coord x1, x2;    
  //  zMapWindowSetHandle(window);

  zMapWindowSetFrame(window, zmap->view_parent);        
  zMapWindowSetFirstTime(window, TRUE);                 /* used in addPane() */

  /* make the window in which to display the data */
  createZMapWindow(window);

  // for now, just default the areas to 0;
  zMapWindowCreateRegion(window);
  zMapWindowSetRegionArea(window, 0, 1);
  zMapWindowSetRegionArea(window, 0, 2);

  drawNavigator(window);

  drawWindow(zMapWindowGetFocuspane(window));

  return TRUE;

}


/* zmRecalculate *******************************************************/

static BOOL zmRecalculate(ZMapWindow window, ZMapCallbackData *zMapCBData)
{
  /* derive the region for which we need data. */
  int min, max;

  Calc_cb calc_cb = zMapCBData->calc_cb;

  min = zmCoordFromScreen(zMapWindowGetFocuspane(window), 0);
  max = zmCoordFromScreen(zMapWindowGetFocuspane(window), 
			  zMapWindowGetHeight(window));

  if (min < 0)
    min = 0;
  if (max > zMapWindowGetRegionLength(window))
    max = zMapWindowGetRegionLength(window);

  if (min >= zMapWindowGetRegionArea(window, 1) &&
      max <= zMapWindowGetRegionArea(window, 2))
    return FALSE; /* already covers area. */
  
  min -= 100000;
  max += 100000; /* TODO elaborate this */

  if (min < 0)
    min = 0;
  if (max > zMapWindowGetRegionLength(window))
    max = zMapWindowGetRegionLength(window);

  (*calc_cb)(zMapCBData->seqRegion, min, max, 
	     zMapWindowGetRegionReverse(window));  

  buildCols(zMapWindowGetFocuspane(window));
  
  return TRUE;
}


void zmRegBox(ZMapPane pane, int box, ZMapColumn *col, void *arg)
{
  zMapPaneSetBox2Col(pane, col, box);
  zMapPaneSetBox2Seg(pane, arg, box);

  return;
}


static void zMapPick(int box, double x, double y)
{
  ZMapColumn *col;
  void *seg;
  static ZMapPane *oldWindow = NULL;
  static int oldBox = 0;
  
  if (oldWindow && oldBox)
    {
      col = zMapPaneGetBox2Col(*oldWindow, oldBox);
      seg = zMapPaneGetBox2Seg(*oldWindow, oldBox);
      if (col && seg && col->selectFunc)
	(*col->selectFunc)(*oldWindow, col, seg, oldBox, x, y, FALSE);
      oldBox = 0;
      oldWindow = NULL;
    }
  
  //  if (graphAssFind(&winAssoc, &oldWindow))
  if (oldWindow)
    {
      oldBox = box;
      col = zMapPaneGetBox2Col(*oldWindow, oldBox);
      seg = zMapPaneGetBox2Seg(*oldWindow, oldBox);

      if (col && seg && col->selectFunc)
	(*col->selectFunc)(*oldWindow, col, seg, oldBox, x, y, TRUE);
    }

}



static void drawWindow(ZMapPane pane)
{
  float offset = 5;
  float maxOffset = 0;
  int   frameCol, i, frame = -1;
  float oldPriority = -100000;
  
  //  graphActivate(pane->graph); 
  //  graphClear();
  //  graphColor(BLACK);
  //  graphRegister (PICK,(GraphFunc) zMapPick);
 
  //  pane->box2col = arrayReCreate(pane->box2col, 500, ZMapColumn *);
  //  pane->box2seg = arrayReCreate(pane->box2seg, 500, SEG *);

  zMapPaneFreeBox2Col(pane);
  zMapPaneFreeBox2Seg(pane);
  zMapPaneNewBox2Col(pane, 500);
  zMapPaneNewBox2Seg(pane, 500);

  //  if (pane->drawHandle)
  //    handleDestroy(pane->drawHandle);
  //  pane->drawHandle = handleHandleCreate(zMapWindowGetHandle(pane->window));
  //  pane->drawHandle = NULL;

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

  //  graphTextBounds(maxOffset, 0);
  
  //  graphRedraw();
  
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

void createZMapWindow(ZMapWindow window)
{
  ZMapPane pane = NULL;
 
  zMapWindowSetPanesTree(window, g_node_new(pane));
  zMapWindowSetHpane(window, gtk_hpaned_new());
                                                                                           
  /* After the toolbars comes an hpane, so the user can adjust the width
   * of the navigator pane */
  gtk_container_add(GTK_CONTAINER(zMapWindowGetFrame(window)), zMapWindowGetHpane(window));
                                                                                           
  zMapWindowSetNavigator(window, gtk_scrolled_window_new(NULL, NULL));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(zMapWindowGetNavigator(window)), 
				 GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_widget_set_size_request(zMapWindowGetNavigator(window), 100, -1);

  /* leave window->navigator in place until navcanvas works */
  //  zMapWindowSetNavigator(window, graphNakedCreate(TEXT_FIT, "", 20, 100, FALSE)) ;
  //  gtk_widget_set_size_request(gexGraph2Widget(zMapWindowGetNavigator(window)), 50, -1);

  gtk_paned_pack1(GTK_PANED(zMapWindowGetHpane(window)), 
		  zMapWindowGetNavigator(window),
                  TRUE, TRUE);
                                                                                           
  /* create the splittable pane and pack it into the hpane as child2. */
  zMapZoomToolbar(window);

  gtk_widget_set_size_request(zMapWindowGetZoomVbox(window), 750, -1);
  gtk_paned_pack2(GTK_PANED(zMapWindowGetHpane(window)), 
		  zMapWindowGetZoomVbox(window)
		  , TRUE, TRUE);

  return;
}


/* zMapZoomToolbar ***************************************************************
 * Builds toolbar with Zoom buttons & magnification combo box.  GraphNakedCreate
 * sets up a bare graph.  Calls addPane to add the actual data display panel.
 * I suspect we'll end up putting the zoom buttons on the top menu line.
 */
static void zMapZoomToolbar(ZMapWindow window)
{
  
  GtkWidget *toolbar = gtk_toolbar_new();
  
  zMapWindowSetZoomVbox(window, gtk_vbox_new(FALSE,0));


  addPane(window, 'v');
  drawGene(window);
  //  columnOfBoxes(window);

  return;
}


/* This is just a noddy function I used to draw a small box on the canvas */
void drawBox (FooCanvasItem *group, double x1, double y1, 
	      double x2, double y2, 
	      char *line_colour, char *fill_colour)
{
  foo_canvas_item_new(FOO_CANVAS_GROUP(group),
                        foo_canvas_rect_get_type(),
                        "x1"           , (double)x1 ,
                        "y1"           , (double)y1 ,
                        "x2"           , (double)x2,
                        "y2"           , (double)y2,
                        "outline_color", line_colour          ,
                        "fill_color"   , fill_colour          ,
                        "width_units"  , (double)1,
                        NULL);
                                                                                
         
  return;                                                                       
}

/* This is just a noddy function I used to draw a line  on the canvas */
void drawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
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
                                                                                

static void drawGene(ZMapWindow window)
{
  FooCanvasItem *group;
  ZMapPane pane = zMapWindowGetFocuspane(window);
  FooCanvas *canvas = zMapPaneGetCanvas(pane);

  group = foo_canvas_item_new(foo_canvas_root(canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);

  //group = window->focuspane->group;

  /*  drawBox(group, 0.0, 220.0 ,"light blue", "white");
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
  */
  return;
}

               

static void columnOfBoxes(ZMapWindow window)
{
  /*  int space[12] = {1,6,9,4,5,2,8,3,7,1,6};
  int i, j;
  double y = 0;
  FooCanvasItem *group;
  ZMapPane pane = zMapWindowGetFocuspane(window);
  FooCanvas *canvas = zMapPaneGetCanvas(pane);

  group = foo_canvas_item_new(foo_canvas_root(canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);
  */
  //group = window->focuspane->group;
  /*
  for (i=0; i<20; i++)
    {
      for (j=0; j<10; j++)
	{
	  drawBox(group, 60.0, y + space[j], "black", "blue");
	  y += space[j];
	}
    }

  group = foo_canvas_item_new(foo_canvas_root(canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);
  */
  //group = window->focuspane->group;
  /*
  for (i=0, y=0; i<20; i++)
    {
      for (j=1; j<11; j++)
	{
	  drawBox(group, 90.0, y + space[j], "black", "purple");
	  y += space[j];
	}
    }

  group = foo_canvas_item_new(foo_canvas_root(canvas),
                        foo_canvas_group_get_type(),
                        "x", (double)100,
                        "y", (double)100 ,
                        NULL);
  */
  //group = window->focuspane->group;
  /*
  for (i=0, y=0; i<20; i++)
    {
      for (j=2; j<12; j++)
	{
	  drawBox(group, 120.0, y + space[j], "black", "green");
	  y += space[j];
	}
    }
  */
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


float zmMainScale(FooCanvas *canvas, float offset, int start, int end)
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




static void navScale(FooCanvas *canvas, float offset, int start, int end)
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




static int dragBox;

/* I believe navDrag is only called by navPick.  Since I don't 
** know what navPick is doing, and don't now know what
** navDrag is doing, I'm going to comment most of it out. */
static void navDrag(float *x, float *y, BOOL isDone)
{
  static BOOL isDragging = FALSE;
  static float oldY;
  ZMapWindow window;
  ZMapPane pane;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  int height;
  
  //  graphFitBounds(NULL, &height);
  //  graphAssFind(&navAssoc, &window);

  /*  if (dragBox == zMapWindowGetFocuspane(window)->dragBox)
    {
      pane = zMapWindowGetFocuspane(window);
      *x = zMapWindowGetScaleOffset(window) - 0.3;
    }
  else
    return;

  startWind =  zmCoordFromScreen(pane, 0);
  endWind =  zmCoordFromScreen(pane, pane->graphHeight);
  
  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
  endWindf   = zMapWindowGetScreenCoord(window, endWind, height);
  */
  //  startWindf = height * (startWind - zMapWindowGetCoord(window, "s"))
  //                      / (zMapWindowGetCoord(window, "e") - zMapWindowGetCoord(window, "s"));

  //  endWindf = height * (endWind - zMapWindowGetCoord(window, "s"))
  //    / (zMapWindowGetCoord(window, "e") - zMapWindowGetCoord(window, "s"));
  /*
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
      pane->centre = srInvarCoord(zMapWindowGetFocuspane(window)->zMapRegion, 
				  srCoord(zMapWindowGetFocuspane(window)->zMapRegion, pane->centre) -
				  (oldY - *y) * (float)(zMapWindowGetCoord(window, "e") - zMapWindowGetCoord(window, "s"))/(float)height);
  */
      /* TO DO: how do I get a zMapCBData into navDrag?
	 ANS: when I convert the graphRegister to a g_signal_connect I can do that. 
      if (zmRecalculate(window, zMapCBData))
	drawNavigator(window);
      */
      printf("Well, I'm in navDrag\n");
      
      // we don't have a pane or window, so can't do anything with them here.
      //      drawWindow(pane); 
      //      graphActivate(zMapWindowGetNavigator(window));
      //    }
  
}

/* not sure what navPick is supposed to do, so not
** going to give it a signal_connect for now. Params
** are all wrong, anyway. */
static void navPick(int box, double x, double y)
{
  ZMapWindow window;

  //  graphAssFind(&navAssoc, &window);

  //  if (box == zMapWindowGetFocuspane(window)->dragBox)
    {
      dragBox = box;
      //      graphBoxDrag(box, navDrag);
    }
}

static void navResize(void)
{
  ZMapWindow window;
  
  //  if (graphAssFind(&navAssoc, &window))
    drawNavigator(window);
}

void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p;
  
  drawNavigator(zMapPaneGetZMapWindow(pane));
}



static void drawNavigator(ZMapWindow window)
{
  int height;
  ScreenCoord startCalcf, endCalcf;
  int areaSize;
  GtkWidget *w;
  GtkRequisition req;
  
  //  graphActivate(zMapWindowGetNavigator(window));
  //  graphAssociate(&navAssoc, window);
  //  graphRegister (PICK,(GraphFunc) navPick) ;
  //  graphRegister(RESIZE, (GraphFunc) navResize);
  //  graphClear();

  //  graphFitBounds(NULL, &height);
  
  areaSize = zMapWindowGetRegionSize(window);
  if (areaSize < 1) areaSize = 1;

  zMapWindowSetCoord(window, "s", areaSize/2);
  zMapWindowSetCoord(window, "e", areaSize/2);
  startCalcf = zMapWindowGetScreenCoord1(window, height);
  endCalcf   = zMapWindowGetScreenCoord2(window, height);

  w = foo_canvas_new();

  zMapWindowSetNavCanvas(window, FOO_CANVAS(w));
  foo_canvas_set_scroll_region(zMapWindowGetNavCanvas(window), 0.0, 0.0, 200.0, 500.0);
 
  foo_canvas_item_new(foo_canvas_root(zMapWindowGetNavCanvas(window)),
			foo_canvas_rect_get_type(),
			"x1",(double)0.0,
			"y1",(double)0.0,
			"x2",(double)200.0,
			"y2",(double)500.0,
			"fill_color", "white",
			NULL);

  //  g_signal_connect(GTK_OBJECT(zMapWindowGetNavCanvas(window)), "event", 
  //		   GTK_SIGNAL_FUNC(navPick), (gpointer) zMapWindowGetNavCanvas(window));

  //  graphColor(BLACK);

  zMapWindowSetScaleOffset(window, 
			   zmMainScale(zMapWindowGetNavCanvas(window),
				       1.0, zMapWindowGetCoord(window, "s"), 
				       zMapWindowGetCoord(window, "e")));
  drawNavigatorWind(zMapWindowGetFocuspane(window));

  gtk_container_add(GTK_CONTAINER(zMapWindowGetNavigator(window)), w);

  navScale(FOO_CANVAS(w), 10, 0, 1000);

  //  graphRedraw();
}


void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p;
  ZMapWindow window = zMapPaneGetZMapWindow(pane);
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (!GTK_WIDGET_REALIZED(zMapWindowGetFrame(window)))
    return;

  //  graphActivate(zMapWindowGetNavigator(window));
  //  graphFitBounds(NULL, &height);
  //  graphBoxDim(pane->scrollBox, &x1, &y1, &x2, &y2);

  startWind =  zmCoordFromScreen(pane, 0);
  endWind =  zmCoordFromScreen(pane, zMapPaneGetHeight(pane));
  
  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
  endWindf = zMapWindowGetScreenCoord(window, endWind, height);
  lenWindf = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;

  //  graphBoxShift(pane->scrollBox, x1, startScreenf);
}



/* Coordinate stuff ****************************************************/

VisibleCoord zmVisibleCoord(ZMapWindow window, Coord coord)
{
  ZMapPane    pane   = zMapWindowGetFocuspane(window);
  ZMapRegion *region = zMapPaneGetZMapRegion(pane);

  return coord - srCoord(region, zMapWindowGetOrigin(window)) + 1;
}


ScreenCoord zmScreenCoord(ZMapPane pane, Coord coord)
{
  ZMapRegion *region  = zMapPaneGetZMapRegion(pane);
  Coord basesFromCent = coord - srCoord(region, zMapPaneGetCentre(pane));
  float linesFromCent = ((float)basesFromCent)/zMapPaneGetBPL(pane);

  return linesFromCent + (float)(zMapPaneGetHeight(pane)/2);
}


Coord zmCoordFromScreen(ZMapPane pane, ScreenCoord coord)
{
  float linesFromCent = coord - (zMapPaneGetHeight(pane)/2);
  int basesFromCent = linesFromCent * zMapPaneGetBPL(pane);

  return srCoord(zMapPaneGetZMapRegion(pane), zMapPaneGetCentre(pane)) + basesFromCent;
}


BOOL zmIsOnScreen(ZMapPane pane, Coord coord1, Coord coord2)
{
  if (zmScreenCoord(pane, coord2) < 0)
    return FALSE;

  if (zmScreenCoord(pane, coord1) > zMapPaneGetHeight(pane))
    return FALSE;

  return TRUE;
}

/************************** end of file *********************************/
