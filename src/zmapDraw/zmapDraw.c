/*  Last edited: Jul 15 12:01 2004 (edgrif) */
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

#include <glib.h>
#include <ZMap/zmapDraw.h>
#include <../zmapWindow/seqregion.h>			    /* Hack to compile for now... */





/* function prototypes ***********************************************/

static void drawGene (FooCanvas *canvas) ;
static void zMapPick (int box, double x, double y);
static void navPick  (int box, double x, double y);
static void navDrag  (float *x, float *y, gboolean isDone);
static void navResize(void);

/* functions ********************************************************/

void displayText(FooCanvasGroup *group, char *text, double x, double y)
{
  FooCanvasItem *item = foo_canvas_item_new(group,
						FOO_TYPE_CANVAS_TEXT,
						"x", x, "y", y, "text", text,
						NULL);
  return;
}


void zmRegBox(ZMapPane pane, int box, ZMapColumn *col, void *arg)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* hack to get everything to compile......... */

  zMapPaneSetBox2Col(pane, col, box);
  zMapPaneSetBox2Seg(pane, arg, box);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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



/* Coordinate stuff ****************************************************/
/* commenting out as I'm not convinced we're going to use this
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

gboolean zmIsOnScreen(ZMapPane pane, Coord coord1, Coord coord2)
{
  if (zmScreenCoord(pane, coord2) < 0)
    return FALSE;

  if (zmScreenCoord(pane, coord1) > zMapPaneGetHeight(pane))
    return FALSE;

  return TRUE;
}
******** end of copout commenting out */


/* internal functions **************************************************/

/* zmRecalculate *******************************************************/

/* we will need something like this, but not yet
static gboolean zmRecalculate(ZMapWindow window, ZMapCallbackData *zMapCBData)
{
*/  /* derive the region for which we need data. */
/*  int min, max;

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
      return FALSE; *//* already covers area. */
/*  
  min -= 100000;
  max += 100000; *//* TODO elaborate this */
/*
  if (min < 0)
    min = 0;
  if (max > zMapWindowGetRegionLength(window))
    max = zMapWindowGetRegionLength(window);

  (*calc_cb)(zMapCBData->seqRegion, min, max, 
	     zMapWindowGetRegionReverse(window));  

  buildCols(zMapWindowGetFocuspane(window));
  
  return TRUE;
}
*/


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void drawGene(FooCanvas *canvas)
{
  FooCanvasItem *group;

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

               

static int dragBox;

/* I believe navDrag is only called by navPick.  Since I don't 
** know what navPick is doing, and don't now know what
** navDrag is doing, I'm going to comment most of it out. */
static void navDrag(float *x, float *y, gboolean isDone)
{
  static gboolean isDragging = FALSE;
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


/************************** end of file *********************************/
