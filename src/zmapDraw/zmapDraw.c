/*  Last edited: Sep  1 16:35 2004 (rnc) */
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




/* function prototypes ***********************************************/


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void drawGene (FooCanvas *canvas) ;
static void zMapPick (int box, double x, double y);
static void navPick  (int box, double x, double y);
static void navDrag  (float *x, float *y, gboolean isDone);
static void navResize(void);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/* functions ********************************************************/

void zmapDisplayText(FooCanvasGroup *group, char *text, char *colour, double x, double y)
{
  FooCanvasItem *item = foo_canvas_item_new(group,
					    FOO_TYPE_CANVAS_TEXT,
					    "x", x, "y", y, "text", text,
					    "fill_color", colour,
					    NULL);
  return;
}



FooCanvasItem *zmapDrawBox (FooCanvasItem *group, 
			    double x1, double y1, double x2, double y2, 
			    GdkColor *line_colour, GdkColor *fill_colour)
{
  FooCanvasItem *item;
  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1"               , (double)x1 ,
			     "y1"               , (double)y1 ,
			     "x2"               , (double)x2 ,
			     "y2"               , (double)y2 ,
			     "outline_color_gdk", line_colour,
			     "fill_color_gdk"   , fill_colour,
			     "width_units"      , (double)1.0,
			     NULL);
  return item;                                                                       
}



void zmapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
		  GdkColor *colour, double thickness)
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
			"fill_color_gdk" , colour,
			"width_units", thickness,
			NULL);
		    
 /* free the points array */
  foo_canvas_points_free(points);

  return;
}
                                                                                

float zmapDrawScale(FooCanvas *canvas, float offset, int start, int end)
{
  FooCanvasItem *group;
  GtkWidget *parent;
  GtkRequisition req;   // think this is redundant
  float mag = 1.0, cutoff, y;  // these may be too
  int seqUnit;          // spacing, in bases, of tick on scalebar
  int subunit;
  int seqPos = 1;       // where we are, in bases, in the sequence
  int width = 0;
  int scalePos;         // where we are on the scalebar
  int type;             // the nomial (ie whether we're working in K, M, etc)
  int unitType;         // index into unitName array
  char cp[20], unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;
  int scaleUnit = 100;  // arbitrary spacing of ticks on the scalebar
  int ticks;            // number of ticks on the scalebar
  double x1, x2, y1, y2, height;  // canvas attributes
  GdkColor black;
  
  gdk_color_parse("black", &black);

  //  parent = gtk_widget_get_parent(GTK_WIDGET(canvas));         // get the scrolled window
  //  gtk_widget_size_request(parent, &req);     
  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);
  height = y2 - y1;

  ticks = height/scaleUnit;
  seqUnit = (end - start)/ticks;

  mag = height / (end - start);
  if (mag == 0.0) mag = 1.0;
  cutoff = 5/mag;

  for (type = 1, unitType = 0 ; 
       seqUnit > 0 && 1000 * type < seqUnit && unitType < 5; 
       unitType++, type *= 1000) ;
  /*
  if (x>0)                          // ie not reversed - this bit needs attention
    x = ((start/unit)+1)*unit;
  else
    x = ((start/unit)-1)*unit;  
  */
  seqPos = start;

  seqUnit = ((seqUnit + type/2)/type) * type;  // round the unit sensibly

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			foo_canvas_group_get_type(),
			"x",(double)offset,
			"y",(double)5.0,
			NULL);
  /*
  for (x = start, count = 1 ; x < end ; x += 10, count++)
    {
      zmapDrawLine(FOO_CANVAS_GROUP(group), offset-5, x, offset, x, "black", 1.0);
      char text[25];
      sprintf(text,"%d", x*10);
      if (count == 1)
	zmapDisplayText(FOO_CANVAS_GROUP(group), text, offset - 25, x); 
      if (count > 9) count = 0;
    }
  */

  for (scalePos = 0; scalePos < height; scalePos += scaleUnit, seqPos += seqUnit)
    {
      y = (float)(seqPos - start)*mag;
      zmapDrawLine(FOO_CANVAS_GROUP(group), offset-5, scalePos, offset, scalePos, &black, 1.0);
      buf[0] = unitName[unitType] ; buf[1] = 0 ;
      sprintf (cp, "%d%s", seqPos/type, buf) ;
      if (width < strlen (cp))
        width = strlen (cp) ;
      zmapDisplayText(FOO_CANVAS_GROUP(group), cp, "black", offset-20, scalePos-0.5); 

    }		     
  /*  for (x = ((start/unit)-1)*unit ; x < end ; x += subunit)
    {
      y = (float)(x-start)*mag;
      zmapDrawLine(FOO_CANVAS_GROUP(group), offset+0.5, y, offset+1.0, y, "black", 1.0);
    }
  */			     
  zmapDrawLine(FOO_CANVAS_GROUP(group), offset+1, 0, offset+1, height, &black, 1.0); // long vertical line
  return offset + width + 4 ;

}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* all of this commented out because it is not called. */


void zmRegBox(ZMapPane pane, int box, ZMapColumn *col, void *arg)
{

  zMapPaneSetBox2Col(pane, col, box);
  zMapPaneSetBox2Seg(pane, arg, box);

  return;
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

gboolean zmIsOnScreen(ZMapPane pane, Coord coord1, Coord coord2)
{
  if (zmScreenCoord(pane, coord2) < 0)
    return FALSE;

  if (zmScreenCoord(pane, coord1) > zMapPaneGetHeight(pane))
    return FALSE;

  return TRUE;
}


/* internal functions **************************************************/

/* zmRecalculate *******************************************************/

/* we will need something like this, but not yet */
static gboolean zmRecalculate(ZMapWindow window, ZMapCallbackData *zMapCBData)
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
      return FALSE; *//* already covers area. */
/*  
  min -= 100000;
  max += 100000; *//* TODO elaborate this */

  if (min < 0)
    min = 0;
  if (max > zMapWindowGetRegionLength(window))
    max = zMapWindowGetRegionLength(window);

  (*calc_cb)(zMapCBData->seqRegion, min, max, 
	     zMapWindowGetRegionReverse(window));  

  buildCols(zMapWindowGetFocuspane(window));
  
  return TRUE;
}


static void zMapPick(int box, double x, double y)
{
  ZMapColumn *col;
  void *seg;
  static ZMapPane *oldWindow = NULL;
  static int oldBox = 0;

  /* don't actually know what this is doing
  ** and not convinced I need to know today...  */
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



#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/************************** end of file *********************************/
