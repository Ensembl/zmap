/*  File: zmapDraw.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Dec 13 10:25 2004 (rnc)
 * Created: Wed Oct 20 09:19:16 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.c,v 1.19 2004-12-13 10:26:57 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapDraw.h>





/* function prototypes ***********************************************/




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
			     "x1"               , x1 ,
			     "y1"               , y1 ,
			     "x2"               , x2 ,
			     "y2"               , y2 ,
			     "outline_color_gdk", line_colour,
			     "fill_color_gdk"   , fill_colour,
			     "width_units"      , 1.0,
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


                                                                                

FooCanvasItem *zmapDrawScale(FooCanvas *canvas,
			     double offset, double zoom_factor, 
			     int start, int end)
{
  FooCanvasItem *group;
  float unit, subunit ;
  int iUnit, iSubunit, type, unitType ;
  int width = 0 ;
  char cp[20], unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;
  float cutoff;
  int seqPos = 1;       /* where we are, in bases, in the sequence */
  double scalePos;
  GdkColor black, white, yellow;
  int top, bottom;
  double x1, y1, x2, y2;

  gdk_color_parse("black", &black);
  gdk_color_parse("white", &white);
  gdk_color_parse("yellow", &yellow);

  /* work out units and subunits for the scale bar */
  /* cutoff defines, via the while loop, the units for our major and minor ticks. This code was
  ** all copied directly from acedb - w7/mapcontrol.c - which seems to assume that a screen line
  ** is about 13 pixels high and that a major scalebar tick every 5 lines is about right. We work
  ** in pixels, which is why we use 65 as our seed.  It's 5 times the number of bases/pixel. When
  ** I work out how to get the line height from the canvas directly, I'll change this bit. */

  cutoff = 65.0 / zoom_factor;    
  unit = subunit = 1.0 ;

  if (cutoff < 0)
    cutoff = -cutoff ;

  /* each time through this loop unit and subunit increase by 10 times until unit exceeds cutoff,
  ** so we end up with a nice round number for our major ticks, and 10 minor ticks per major one. */
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

  iUnit = unit + 0.5 ;
  iSubunit = subunit + 0.5 ;

  /* calculate nomial ie thousands, millions, etc */
  for (type = 1, unitType = 0 ; 
       iUnit > 0 && 1000 * type < iUnit && unitType < 5; 
       unitType++, type *= 1000) ;

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			      foo_canvas_group_get_type(),
			      "x",(double)offset,
			      "y",(double)0.0,
			      NULL);

  /* If the scrolled_region has been cropped, we need to crop the scalebar too */
  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);
  if (start < y1)
    start = y1;
  if (end > y2)
    end = y2;

  /* yellow bar separates forward from reverse strands. Draw first so scalebar text
  ** overlies it. */
  zmapDrawBox(FOO_CANVAS_ITEM(group), 0.0, start, 3.0, end, &white, &yellow); 
										    
  /* major ticks and text */
  for (seqPos = start, scalePos = start; 
       seqPos < end; 
       seqPos += iUnit, scalePos += iUnit)
    {
      zmapDrawLine(FOO_CANVAS_GROUP(group), 30.0, scalePos, 40.0, scalePos, &black, 1.0);
      buf[0] = unitName[unitType] ; buf[1] = 0 ;
      sprintf (cp, "%d%s", seqPos/type, buf) ;
      if (width < strlen (cp))
        width = strlen (cp) ;
      zmapDisplayText(FOO_CANVAS_GROUP(group), cp, "black", (29.0 - (5.0 * width)), scalePos); 
    }		     
  
  /* draw the vertical line of the scalebar. */
  zmapDrawLine(FOO_CANVAS_GROUP(group), 40.0, start, 40.0, end, &black, 1.0);

  /* minor ticks */
  for (seqPos = start, scalePos = start; seqPos < end; seqPos += iSubunit, scalePos += iSubunit)
      zmapDrawLine(FOO_CANVAS_GROUP(group), 35.0, scalePos, 40.0, scalePos, &black, 1.0);

  return group;
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
  
  /*  if (graphAssFind(&winAssoc, &oldWindow)) */
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
  
  /*  graphFitBounds(NULL, &height);
    graphAssFind(&navAssoc, &window);

    if (dragBox == zMapWindowGetFocuspane(window)->dragBox)
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
  
    startWindf = height * (startWind - zMapWindowGetCoord(window, "s"))
                        / (zMapWindowGetCoord(window, "e") - zMapWindowGetCoord(window, "s"));

    endWindf = height * (endWind - zMapWindowGetCoord(window, "s"))
      / (zMapWindowGetCoord(window, "e") - zMapWindowGetCoord(window, "s"));
  
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
      
      /* we don't have a pane or window, so can't do anything with them here.
      **      drawWindow(pane); 
      **      graphActivate(zMapWindowGetNavigator(window));
      **    } */
  
}

/* not sure what navPick is supposed to do, so not
** going to give it a signal_connect for now. Params
** are all wrong, anyway. */
static void navPick(int box, double x, double y)
{
  ZMapWindow window;

  /*  graphAssFind(&navAssoc, &window);

  **  if (box == zMapWindowGetFocuspane(window)->dragBox)
  **  {
  **    dragBox = box;
  **    graphBoxDrag(box, navDrag);
  **  } */
}



#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/************************** end of file *********************************/
