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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

#include <wh/graph.h>
#include <wh/gex.h>
#include <wzmap/seqregion.h>
#include <wzmap/zmapcontrol.h>

#if !defined(ZMAP)
BOOL zMapDisplay(KEY key, KEY from, BOOL isOldGraph, void *app_data)
{
  return FALSE;
}

#else

static void makezMapChrome(ZMapLook *look);
static ZMapRoot *zMapRootCreate(ZMapLook *look);
static ZMapWindow *zMapWindowCreate(ZMapRoot *root);
static void drawNavigator(ZMapRoot *root);
static void drawWindow(ZMapWindow *window);
static void navUpdate(GtkAdjustment *adj, gpointer p);
static void navChange(GtkAdjustment *adj, gpointer p);
static void drawNavigatorWind(ZMapWindow *window);
static void splitWindow(GtkWidget *widj, gpointer data);
void zMapScaleColumn(ZMapWindow *window, float *offset, int frame);

static void *navAssoc, *winAssoc;

/* Shim to translate from Acedb stuff. There should be no keys
   anywhere else in this file....
*/

BOOL sfDisplay(char *seqspec, char *fromspec, BOOL isOldGraph);

BOOL zMapDisplay(KEY key, KEY from, BOOL isOldGraph, void *app_data)
 {
  BOOL ret;
  
  char *keyspec = messalloc(strlen(className(key)) + strlen(name(key)) + 2);
  char *fromspec = messalloc(strlen(className(from)) + strlen(name(from)) + 2);
				  
  sprintf(keyspec, "%s:%s", className(key), name(key));
  sprintf(fromspec, "%s:%s", className(from), name(from));
  
  ret = sfDisplay(keyspec, fromspec, isOldGraph);

  messfree(keyspec);
  messfree(fromspec);
  return ret;
}


/* Coordinate stuff */

VisibleCoord zmVisibleCoord(ZMapRoot *root, Coord coord)
{
  return coord - srCoord(root->region, root->origin) + 1;
}

ScreenCoord zmScreenCoord(ZMapWindow *window, Coord coord)
{
  Coord basesFromCent = coord - srCoord(window->root->region, window->centre);
  float linesFromCent = ((float)basesFromCent)/((float)window->basesPerLine);

  return linesFromCent + (float)(window->graphHeight/2);
}

Coord zmCoordFromScreen(ZMapWindow *window, ScreenCoord coord)
{
  float linesFromCent = coord - (window->graphHeight/2);
  int basesFromCent = linesFromCent * window->basesPerLine;

  return srCoord(window->root->region, window->centre) + basesFromCent;
}

BOOL zmIsOnScreen(ZMapWindow *window, Coord coord1, Coord coord2)
{
  if (zmScreenCoord(window, coord2) < 0)
    return FALSE;

  if (zmScreenCoord(window, coord1) > window->graphHeight)
    return FALSE;

  return TRUE;
}

BOOL sfDisplay(char *seqspec, char *fromspec, BOOL isOldGraph)
{

  ZMapLook *look = messalloc(sizeof(ZMapLook));
  Coord x1, x2;
  int seqLen;
  SeqRegion *region;

  look->handle = handleCreate();
  
  makezMapChrome(look);
 
  /* Make region */
  region = look->root1->region = srCreate(look->handle);
  
  /* Poke in stuff */
  srActivate(seqspec, region, &x1, &x2);
  
  /* policy: we start be calculating three times the length of
     the initial object */
  seqLen = x2 - x1;
  
  srCalculate(region, x1 - seqLen, x2 + seqLen);
  makezMapDefaultColumns(look->root1->window1);
  buildCols(look->root1->window1);

  look->root1->origin = srInvarCoord(region, x1);
  look->root1->window1->centre = srInvarCoord(region, x1);
  look->root1->window1->basesPerLine = 100;
    
  gtk_widget_show_all(look->gexWindow);

  drawNavigator(look->root1);

  drawWindow(look->root1->window1);
 
  return TRUE;

}

static BOOL zmRecalculate(ZMapRoot *root)
{
  /* derive the region for which we need data. */
  int min, max;

  min = zmCoordFromScreen(root->window1, 0);
  max = zmCoordFromScreen(root->window1, root->window1->graphHeight);
 
  if (root->window2)
    {
      if (zmCoordFromScreen(root->window2, 0) < min)
	min = zmCoordFromScreen(root->window2, 0);

      if (zmCoordFromScreen(root->window2,  root->window2->graphHeight) > max)
	max = zmCoordFromScreen(root->window2, root->window2->graphHeight);
    }

  if (min < 0)
    min = 0;
  if (max > root->region->length)
    max = root->region->length;

  if (min >= root->region->area1 &&
      max <= root->region->area2)
    return FALSE; /* already covers area. */
  
  min -= 100000;
  max += 100000; /* TODO elaborate this */

  if (min < 0)
    min = 0;
  if (max > root->region->length)
    max = root->region->length;
  
  srCalculate(root->region, min, max); 
  buildCols(root->window1);
  if (root->window2)
    buildCols(root->window2);
  
  return TRUE;
}

void zmRegBox(ZMapWindow *window, int box, ZMapColumn *col, void *arg)
{
  array(window->box2col, box, ZMapColumn *) = col;
  array(window->box2seg, box, void *) = arg;
}

static void zMapPick(int box, double x, double y)
{
  ZMapColumn *col;
  void *seg;
  static ZMapWindow *oldWindow = NULL;
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
static void drawWindow(ZMapWindow *window)
{
  float offset = 0;
  float maxOffset = 0;
  int frameCol, i, frame = -1;
  float oldPriority = -100000;
  
  graphActivate(window->graph);
  graphClear();
  graphColor(BLACK);
  graphRegister (PICK,(GraphFunc) zMapPick);
  
  window->box2col = arrayReCreate(window->box2col, 500, ZMapColumn *);
  window->box2seg = arrayReCreate(window->box2seg, 500, SEG *);
  if (window->drawHandle)
    handleDestroy(window->drawHandle);
  window->drawHandle = handleHandleCreate(window->root->look->handle);

  for (i = 0; i < arrayMax(window->cols); i++)
   
    { 
      ZMapColumn *col = arrp(window->cols, i, ZMapColumn);
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
	  col = arrp(window->cols, i, ZMapColumn);
	}
      else if ((frame == 2) && !col->isFrame)
	{
	  /* in frame mode, reach end of framed columns, done last frame. */
	  frame = -1;
	}
      else if (col->priority < oldPriority + 0.01001)
	offsetSave = offset;
     
      (*col->drawFunc)(window, col, &offset, frame);

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

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",            NULL,         0,                     0, "<Branch>" },
  { "/File/_Print",      "<control>P", gtk_ifactory_cb,       0 },
  { "/File/Print _Whole","<control>W", gtk_ifactory_cb,       0 },
  { "/File/P_reserve",   "<control>r", gtk_ifactory_cb,       0 },
  { "/File/_Recalculate",NULL,         gtk_ifactory_cb,       0 },
  { "/File/sep1",        NULL,         gtk_ifactory_cb,       0, "<Separator>" },
  { "/File/_Quit",       "<control>Q", gtk_ifactory_cb,       0 },
  { "/_Export",         		NULL, 0,               0, "<Branch>" },
  { "/Export/_Features", 		NULL, gtk_ifactory_cb, 0 },
  { "/Export/_Sequence",             	NULL, gtk_ifactory_cb, 0 },
  { "/Export/_Transalations",           NULL, gtk_ifactory_cb, 0 },
  { "/Export/_EMBL dump",               NULL, gtk_ifactory_cb, 0 },
  { "/_Help",            NULL,         0,                     0, "<LastBranch>" },
  { "/Help/_About",      NULL,         gtk_ifactory_cb,       0 },
};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);
  
static void makezMapChrome(ZMapLook *look)
{
  GtkAccelGroup *accel_group = gtk_accel_group_new ();
  
  look->root2 = NULL;
  look->root1 = zMapRootCreate(look);
  look->gexWindow = gexWindowCreate();
  gtk_window_set_default_size(GTK_WINDOW(look->gexWindow), 750, 550); 
  look->vbox = gtk_vbox_new(FALSE, 4);
  look->itemFactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<main>", accel_group);
  look->infoSpace = gtk_entry_new();
  look->pane = gtk_vpaned_new();

  gtk_accel_group_attach (accel_group, GTK_OBJECT (look->gexWindow));
  gtk_item_factory_create_items (look->itemFactory, nmenu_items, menu_items, NULL);
  gtk_container_add(GTK_CONTAINER(look->gexWindow), look->vbox);
  gtk_box_pack_start(GTK_BOX(look->vbox), 
		     gtk_item_factory_get_widget(look->itemFactory, "<main>"),
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(look->vbox), look->infoSpace, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(look->vbox), look->pane, TRUE, TRUE, 0);
  gtk_paned_pack1(GTK_PANED(look->pane), look->root1->vbox, TRUE, FALSE);
  gtk_paned_set_handle_size(GTK_PANED(look->pane), 0);
  gtk_paned_set_gutter_size(GTK_PANED(look->pane), 0);
}

 
static ZMapRoot *zMapRootCreate(ZMapLook *look)
{
  ZMapRoot *root = halloc(sizeof(ZMapRoot), look->handle);
  GtkWidget *toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_TEXT);
  
  root->look = look;
  root->window1 = zMapWindowCreate(root);
  root->window2 = NULL;
  root->navigator = graphNakedCreate(TEXT_FIT, "", 20, 100, FALSE) ;
  root->hbox = gtk_hbox_new(FALSE, 0);
  root->vbox = gtk_vbox_new(FALSE, 0);

  gtk_box_pack_start(GTK_BOX(root->vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(root->vbox), root->hbox, TRUE, TRUE, 0);
  
  root->splitButton = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), 
					      "Split", NULL, NULL, NULL, 
					      GTK_SIGNAL_FUNC(splitWindow), 
					      (gpointer)root);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Clear", NULL, NULL, NULL, 
			  NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Rev-Comp", NULL, NULL, NULL, 
			  NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "DNA", NULL, NULL, NULL, 
			  NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "GeneFind", NULL, NULL, NULL, 
			NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Origin", NULL, NULL, NULL, 			  NULL, NULL);
  root->pane = gtk_vpaned_new();
  gtk_paned_set_handle_size(GTK_PANED(root->pane), 0);
  gtk_paned_set_gutter_size(GTK_PANED(root->pane), 0);
   
  gtk_widget_set_usize(gexGraph2Widget(root->navigator), 100, -1);
  gtk_box_pack_start(GTK_BOX(root->hbox), gexGraph2Widget(root->navigator),
		     FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(root->hbox), root->pane, TRUE, TRUE, 0);
  gtk_paned_pack1(GTK_PANED(root->pane), root->window1->vbox, TRUE, FALSE);
  return root;
}

static void setBPL(ZMapWindow *window)
{
  GtkEditable *editable = GTK_EDITABLE(GTK_COMBO(window->combo)->entry);
  char buff[100];
  int pos = 0;
  
  sprintf(buff, "%d", window->basesPerLine);

  gtk_editable_delete_text(editable, 0, -1);
  gtk_editable_insert_text(editable, buff, strlen(buff), &pos);
}

static void zoomIn(GtkWidget *widj, gpointer data)
{
  ZMapWindow *window = (ZMapWindow *)data;
  
  if (window->basesPerLine == 1)
    return;
  window->basesPerLine /= 2;
  if (window->basesPerLine < 1)
    window->basesPerLine = 1;

  setBPL(window);
}
  
static void zoomOut(GtkWidget *widj, gpointer data)
{
  ZMapWindow *window = (ZMapWindow *)data;
  
  window->basesPerLine *= 2;
  if (zmRecalculate(window->root))
    drawNavigator(window->root);
  
  setBPL(window);
}
  
static void setMag(GtkWidget *widj, gpointer args)
{
  ZMapWindow *window = (ZMapWindow *)args;
  GtkEditable *edit = GTK_EDITABLE(GTK_COMBO(window->combo)->entry);
  gchar *val = gtk_editable_get_chars(edit, 0, -1);
  
  int mag;
  char *end = strstr(val, "bp/line");

  if (end)
    *end = 0;

  mag = atoi(val);
  
  if (mag)
    {
      window->basesPerLine = mag;
      setBPL(window);
    }
}
 
static void magChanged(GtkWidget *widj, gpointer args)
{
  ZMapWindow *window = (ZMapWindow *)args;
  drawWindow(window);
}
  
static void splitWindow(GtkWidget *widj, gpointer data)
{
  ZMapRoot *root = (ZMapRoot *)data;

  if (root->window2)
    return;

  gtk_widget_set_sensitive(root->splitButton, FALSE);
  root->window2 = zMapWindowCreate(root);
  root->window2->centre = root->window1->centre;
  root->window2->centre = root->window1->centre;
  root->window2->DNAwidth = root->window1->DNAwidth;
  root->window2->cols = arrayCopy(root->window1->cols);

 
  gtk_paned_pack2(GTK_PANED(root->pane), root->window2->vbox, TRUE, FALSE); 
  gtk_paned_set_handle_size(GTK_PANED(root->pane), 6);
  gtk_paned_set_gutter_size(GTK_PANED(root->pane), 8);
   

  gtk_widget_show_all(GTK_WIDGET(root->pane));
  
  drawNavigator(root);
  drawWindow(root->window1);
  drawWindow(root->window2);
}
 

static ZMapWindow *zMapWindowCreate(ZMapRoot *root)
{
  
  ZMapWindow *window = halloc(sizeof(ZMapWindow), root->look->handle);
  GtkAdjustment *adj; 
  GtkWidget *toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_TEXT);
  GList *bpList = NULL;
  
  window->look = root->look;
  window->root = root;
  window->DNAwidth = 100;
  window->graph = graphNakedCreate(TEXT_FIT, "", 20, 100, TRUE);
  window->graphWidget = gexGraph2Widget(window->graph);
  window->scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  window->cols = arrayHandleCreate(50, ZMapColumn, root->look->handle);
  window->box2col = NULL;
  window->box2seg = NULL;
  window->drawHandle = NULL;
  window->vbox = gtk_vbox_new(FALSE, 0);
  window->combo = gtk_combo_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->scrolledWindow),
				 GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(window->scrolledWindow), window->graphWidget);
  
  adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow)); 
  gtk_signal_connect(GTK_OBJECT(adj), "value_changed", GTK_SIGNAL_FUNC(navUpdate), (gpointer)(window));
  gtk_signal_connect(GTK_OBJECT(adj), "changed", GTK_SIGNAL_FUNC(navChange), (gpointer)(window));

  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Zoom In", NULL, NULL, NULL, 
			  GTK_SIGNAL_FUNC(zoomIn), (gpointer) window);

  bpList = g_list_append(bpList, "1 bp/line");
  bpList = g_list_append(bpList, "10 bp/line");
  bpList = g_list_append(bpList, "100 bp/line");
  bpList = g_list_append(bpList, "1000 bp/line");
  
  gtk_combo_set_popdown_strings(GTK_COMBO(window->combo), bpList);
  gtk_combo_disable_activate(GTK_COMBO(window->combo));
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(window->combo)->entry), "activate",
		     (GtkSignalFunc)setMag, (gpointer)window);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(window->combo)->entry), "changed",
		     (GtkSignalFunc)magChanged, (gpointer)window);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(window->combo)->list), "selection_changed",
		     (GtkSignalFunc)setMag, (gpointer)window);

  gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_WIDGET, window->combo,
			     NULL, NULL, NULL, NULL, NULL, NULL);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Zoom Out", NULL, NULL, NULL, 
			  GTK_SIGNAL_FUNC(zoomOut), (gpointer)window);
  
  gtk_box_pack_start(GTK_BOX(window->vbox), toolbar, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(window->vbox), window->scrolledWindow, TRUE, TRUE, 0);

  graphActivate(window->graph);
  graphAssociate(&winAssoc, window);
  graphTextBounds(0, 200);
  graphFitBounds(NULL, &window->graphHeight);

  return window;
}

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
      unit *= 2.5000001 ;	/* safe rounding */
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

static void navDrag(float *x, float *y, BOOL isDone)
{
  static BOOL isDragging = FALSE;
  static float oldY;
  ZMapRoot *root;
  ZMapWindow *window;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  int height;
  
  graphFitBounds(NULL, &height);
  graphAssFind(&navAssoc, &root);

  if (dragBox == root->window1->dragBox)
    {
      window = root->window1;
      *x = root->scaleOffset - 0.3;
    }
  else if (root->window2 && dragBox == root->window2->dragBox)
    {
      window = root->window2;
      *x = root->scaleOffset + 0.7;
    }
  else
    return;

  startWind =  zmCoordFromScreen(window, 0);
  endWind =  zmCoordFromScreen(window, window->graphHeight);
  
  startWindf = height * (startWind - root->navStart)/(root->navEnd - root->navStart);
  endWindf = height * (endWind - root->navStart)/(root->navEnd - root->navStart);
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
      window->centre = srInvarCoord(root->region, srCoord(root->region, window->centre) -
				    (oldY - *y) * (float)(root->navEnd - root->navStart)/(float)height);
      if (zmRecalculate(root))
	drawNavigator(root);
      drawWindow(window); 
      graphActivate(root->navigator);
    }
  
}

static void navPick(int box, double x, double y)
{
  ZMapRoot *root;

  graphAssFind(&navAssoc, &root);

  if (box == root->window1->dragBox || 
      (root->window2 && box == root->window2->dragBox))
    {
      dragBox = box;
      graphBoxDrag(box, navDrag);
    }
}

static void navResize(void)
{
  ZMapRoot *root;
  
  if (graphAssFind(&navAssoc, &root))
    drawNavigator(root);
}

static void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapWindow *window = (ZMapWindow *)p;
  
  drawNavigator(window->root);
}

static void drawNavigator(ZMapRoot *root)
{
  int height;
  ScreenCoord startCalcf, endCalcf;
  int areaSize;
  
  graphActivate(root->navigator);
  graphAssociate(&navAssoc, root);
  graphRegister (PICK,(GraphFunc) navPick) ;
  graphRegister(RESIZE, (GraphFunc) navResize);
  graphClear();

  graphFitBounds(NULL, &height);

  areaSize = root->region->area2 - root->region->area1;

  root->navStart = root->region->area1 - areaSize/2;
  root->navEnd = root->region->area2 + areaSize/2;

  startCalcf = height * (root->region->area1 - root->navStart)/(root->navEnd - root->navStart);
  endCalcf = height * (root->region->area2 - root->navStart)/(root->navEnd - root->navStart);
  
  graphColor(LIGHTGRAY);
  graphFillRectangle(0, startCalcf, 100.0, endCalcf);
  graphColor(BLACK);

  root->scaleOffset= zmDrawScale(1.0, root->navStart, root->navEnd);
  drawNavigatorWind(root->window1);
  if (root->window2)
    drawNavigatorWind(root->window2);

  graphRedraw();
}

static void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapWindow *window = (ZMapWindow *)p;
  ZMapRoot *root = window->root;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (!GTK_WIDGET_REALIZED(root->look->gexWindow))
    return;

  graphActivate(root->navigator);
  graphFitBounds(NULL, &height);
  graphBoxDim(window->scrollBox, &x1, &y1, &x2, &y2);

  startWind =  zmCoordFromScreen(window, 0);
  endWind =  zmCoordFromScreen(window, window->graphHeight);
  
  startWindf = height * (startWind - root->navStart)/(root->navEnd - root->navStart);
  endWindf = height * (endWind - root->navStart)/(root->navEnd - root->navStart);
  lenWindf = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;

  graphBoxShift(window->scrollBox, x1, startScreenf);
}



static void drawNavigatorWind(ZMapWindow *window)
{
  ZMapRoot *root = window->root;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, endWindf, lenWindf;
  ScreenCoord startScreenf, endScreenf;
  ScreenCoord pos;
  
  GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolledWindow));
  graphFitBounds(NULL, &height);
  
  startWind =  zmCoordFromScreen(window, 0);
  endWind =  zmCoordFromScreen(window, window->graphHeight);
  
  startWindf = height * (startWind - root->navStart)/(root->navEnd - root->navStart);
  endWindf = height * (endWind - root->navStart)/(root->navEnd - root->navStart);
  lenWindf = endWindf - startWindf;
  
  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
  endScreenf = startWindf + lenWindf * (adj->page_size + adj->value - adj->lower)/(adj->upper - adj->lower) ;
  
  graphColor(BLACK);
  
  if (window == root->window1)
    pos = root->scaleOffset;
  else
    pos = root->scaleOffset + 1.0;

  window->dragBox = graphBoxStart();
  graphLine(pos, startWindf, pos, endWindf);
  window->scrollBox = graphBoxStart();
  graphColor(GREEN);
  graphFillRectangle(pos - 0.3, startScreenf, pos + 0.5, endScreenf);
  graphBoxEnd();
  graphBoxSetPick(window->scrollBox, FALSE);
  graphBoxEnd();
  graphBoxDraw(window->dragBox, -1, LIGHTGRAY);

}
  
#endif

