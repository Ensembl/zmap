/*  File: zmapControlNavigator.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Implements the navigator window in a zmap.
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Jul  8 14:22 2004 (edgrif)
 * Created: Thu Jul  8 12:54:27 2004 (edgrif)
 * CVS info:   $Id: zmapControlNavigator.c,v 1.1 2004-07-15 09:38:37 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapControl_P.h>


/* Create the navigator window which is a scrolled window without scroll bars (?),
 * I am unsure why we use a scrolled window here....  with a canvas beneath it. */
GtkWidget *zmapControlCreateNavigator(FooCanvas **canvas_out)
{
  GtkWidget *navigator, *canvas ;
 
  navigator = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(navigator), 
				 GTK_POLICY_NEVER, GTK_POLICY_NEVER) ;
  gtk_widget_set_size_request(navigator, 100, -1) ;

  canvas = foo_canvas_new() ;
  foo_canvas_set_scroll_region(FOO_CANVAS(canvas), 0.0, 0.0, 200.0, 500.0) ;
  foo_canvas_item_new(foo_canvas_root(FOO_CANVAS(canvas)),
			foo_canvas_rect_get_type(),
			"x1",(double)0.0,
			"y1",(double)0.0,
			"x2",(double)200.0,
			"y2",(double)500.0,
			"fill_color", "white",
			NULL);

  gtk_container_add(GTK_CONTAINER(navigator), canvas);
  *canvas_out = FOO_CANVAS(canvas) ;

  return navigator ;
}



void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p;
  
  /* code needed.... */

  return ;
}



void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p ;
  ZMap zmap = pane->zmap ;
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (GTK_WIDGET_REALIZED(zmap->navview_frame))
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


/* Currently not called........... */
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



/* UNUSED CURRENTLY but needed for when user alters the hpane interactively....... */
static void navResize(void)
{

  /* code needed ????? */

  return ;
}




