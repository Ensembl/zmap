/*  File: zmapDraw.h
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
 * Description: Defines interface to drawing routines for features
 *              in the ZMap.
 *              
 * HISTORY:
 * Last edited: Oct  8 11:30 2004 (rnc)
 * Created: Tue Jul 27 16:40:47 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.h,v 1.9 2004-10-13 12:29:55 rnc Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_H
#define ZMAP_DRAW_H

#include <gtk/gtk.h>
#include <libfoocanvas/libfoocanvas.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapFeature.h>

#define BORDER 5.0


float zmapDrawScale(FooCanvas *canvas, GtkWidget *scrolledWindow,
		    float offset, double zoom_factor, int start, int end) ;
void  zmapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
		   GdkColor *colour, double thickness);
FooCanvasItem *zmapDrawBox(FooCanvasItem *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *line_colour, GdkColor *fill_colour);
void  zmapDisplayText(FooCanvasGroup *group, char *text, char *colour, double x, double y);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Rob, I don't know what all these do but they are commented out for the time being
 * as they are not called anywhere........They also break the layering as they have
 * ZMapPane references which is all wrong at this level. */

void zmRegBox(ZMapPane pane, int box, ZMapColumn *col, void *seg);


/* Column drawing code ************************************/
void  zMapFeatureColumn (ZMapPane   pane, ZMapColumn *col,
			 float     *offset, int frame);
void  buildCols         (ZMapPane   pane);
void  makezMapDefaultColumns(ZMapPane  pane);


void  nbcInit           (ZMapPane  pane, ZMapColumn *col);
void  nbcSelect         (ZMapPane  pane, ZMapColumn *col,
			 void     *seg, int box, double x, double y, gboolean isSelect);
void  zMapGeneDraw      (ZMapPane  pane, ZMapColumn *col, float *offset, int frame);
void  geneSelect        (ZMapPane  pane, ZMapColumn *col,
			 void     *arg, int box, double x, double y, gboolean isSelect);

gboolean     zmIsOnScreen     (ZMapPane    pane,   Coord coord1, Coord coord2);
VisibleCoord zmVisibleCoord   (ZMapWindow  window, Coord coord);
ScreenCoord  zmScreenCoord    (ZMapPane    pane,   Coord coord);
Coord        zmCoordFromScreen(ZMapPane    pane,   ScreenCoord coord);
gboolean     Quit             (GtkWidget  *widget, gpointer data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


     
#endif /* ZMAP_DRAW_H */
