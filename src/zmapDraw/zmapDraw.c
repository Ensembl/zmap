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
 * Description: Draw objects into the canvas, these may be unnecessary
 *              if they map closely enough to the foo_canvas calls.
 *              
 * Exported functions: See ZMap/zmapDraw.h
 *              
 * HISTORY:
 * Last edited: Apr  4 11:10 2005 (edgrif)
 * Created: Wed Oct 20 09:19:16 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.c,v 1.26 2005-04-05 14:19:28 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapDraw.h>


/* OK, THIS IS ALL HATEFUL, ITS FOR THE SCALE WHICH WILL SOON NOT BE DRAWN IN THE WINDOW
 * ANYWAY....SO ALL THIS WILL GO AWAY...... */

#define SCALE_LEFT  60.0
#define SCALE_RIGHT SCALE_LEFT + 10.0
#define SCALE_MID   SCALE_LEFT + ((SCALE_RIGHT - SCALE_LEFT) / 2)



FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y)
{
  FooCanvasItem *item = NULL ;

  item = foo_canvas_item_new(group,
			     FOO_TYPE_CANVAS_TEXT,
			     "x", x, "y", y,
			     "text", text,
			     "fill_color", colour,
			     NULL);

  return item ;
}



/* Note that we don't specify "width_units" or "width_pixels" for the outline
 * here because by not doing so we get a one pixel wide outline that is
 * completely enclosed within the item. If you specify either width, you end
 * up with items that are larger than you expect because the outline is drawn
 * centred on the edge of the rectangle. */
FooCanvasItem *zMapDrawBox(FooCanvasItem *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *border_colour, GdkColor *fill_colour)
{
  FooCanvasItem *item = NULL ;
  int line_width = 1 ;

  item = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
			     foo_canvas_rect_get_type(),
			     "x1", x1, "y1", y1,
			     "x2", x2, "y2", y2,
			     "outline_color_gdk", border_colour,
			     "fill_color_gdk", fill_colour,
			     NULL) ;

  return item;                                                                       
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, double thickness)
{
  FooCanvasItem *item = NULL ;
  FooCanvasPoints *points ;

  /* allocate a new points array */
  points = foo_canvas_points_new(2) ;
				                                            
  /* fill out the points */
  points->coords[0] = x1 ;
  points->coords[1] = y1 ;
  points->coords[2] = x2 ;
  points->coords[3] = y2 ;

  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_get_type(),
			     "points", points,
			     "fill_color_gdk", colour,
			     "width_units", thickness,
			     NULL);
		    
  /* free the points array */
  foo_canvas_points_free(points) ;

  return item ;
}


/* It may be good not to specify a width here as well (see zMapDrawBox) but I haven't
 * experimented yet. */
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, double thickness)
{
  FooCanvasItem *item = NULL ;

  /* draw the line */
  item = foo_canvas_item_new(group,
			     foo_canvas_line_get_type(),
			     "points", points,
			     "fill_color_gdk", colour,
			     "width_units", thickness,
			     NULL);
		    
  return item ;
}


                                                                                
/* This routine should not be enhanced or debugged (it does not draw the scale in a good
 * way, the units are often bizarre, e.g. 10001, 20001 etc.). In the future the scale
 * will be in a separate window. */
FooCanvasItem *zmapDrawScale(FooCanvas *canvas,
			     double offset, double zoom_factor, 
			     int start, int end, int *major_units_out, int *minor_units_out)
{
  FooCanvasItem *group = NULL ;
  float unit, subunit ;
  int iUnit, iSubunit, type, unitType ;
  int width = 0 ;
  char cp[20], unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;
  float cutoff;
  int pos;
  GdkColor black, white, yellow ;
  double x1, y1, x2, y2;


  gdk_color_parse("black", &black) ;
  gdk_color_parse("white", &white) ;
  gdk_color_parse("yellow", &yellow) ;

  /* work out units and subunits for the scale bar
   * cutoff defines, via the while loop, the units for our major and minor ticks. This code was
   * all copied directly from acedb - w7/mapcontrol.c - which seems to assume that a screen line
   * is about 13 pixels high and that a major scalebar tick every 5 lines is about right. We work
   * in pixels, which is why we use 65 as our seed.  It's 5 times the number of bases/pixel. When
   * I work out how to get the line height from the canvas directly, I'll change this bit. */

  cutoff = 65.0 / zoom_factor;    
  unit = subunit = 1.0 ;

  if (cutoff < 0)
    cutoff = -cutoff ;

  /* each time through this loop unit and subunit increase by 10 times until unit exceeds cutoff,
   * so we end up with a nice round number for our major ticks, and 10 minor ticks per major one. */
  while (unit < cutoff)
    {
      unit *= 2 ;
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
       iUnit > 0 && 1000 * type < iUnit && unitType < 5 ;
       unitType++, type *= 1000) ;

  /* If the scrolled_region has been cropped, we need to crop the scalebar too */
  foo_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);
  if (start < y1)
    start = y1;
  if (end > y2)
    end = y2;

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			      foo_canvas_group_get_type(),
			      "x", offset,
			      "y", 0.0,
			      NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* yellow bar separates forward from reverse strands. Draw first so scalebar text
   * overlies it. */
  zMapDrawBox(FOO_CANVAS_ITEM(group), 0.0, start, 3.0, end, &white, &yellow); 

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

										    
  /* major ticks and text */
  for (pos = start ; pos < end ; pos += iUnit)
    {
      zMapDrawLine(FOO_CANVAS_GROUP(group), SCALE_LEFT, pos, SCALE_RIGHT, pos, &black, 1.0);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      buf[0] = unitName[unitType] ;
      buf[1] = 0 ;
      sprintf(cp, "%d%s", pos/type, buf) ;
      if (width < strlen(cp))
        width = strlen(cp) ;

      zMapDisplayText(FOO_CANVAS_GROUP(group), cp, "black", ((SCALE_LEFT - 1.0) - (5.0 * width)), pos); 


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  
  /* draw the vertical line of the scalebar, note we should be drawing. */
  zMapDrawLine(FOO_CANVAS_GROUP(group), SCALE_RIGHT, start - 1, SCALE_RIGHT, end - 1, &black, 1.0);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* minor ticks */

  for (pos = start; pos < end; pos += iSubunit)
    {
      zMapDrawLine(FOO_CANVAS_GROUP(group), SCALE_MID, pos, SCALE_RIGHT, pos, &black, 1.0) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintGroup(group) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (major_units_out)
    *major_units_out = iUnit ;

  if (minor_units_out)
    *minor_units_out = iSubunit ;

  return group ;
}


