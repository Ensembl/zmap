/*  Last edited: Jul  9 09:28 2004 (rnc) */
/*  file: zmapcontrol.h
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

#ifndef ZMAPCONTROL_H
#define ZMAPCONTROL_H

#include <gtk/gtk.h>
#include <libfoocanvas/libfoocanvas.h>

#include <ZMap/zmapControl.h>

#include <ZMap/zmapWindow.h>

#include <ZMap/zmapFeature.h>





/* function prototypes ************************************/

void zmRegBox(ZMapPane pane, int box, ZMapColumn *col, void *seg);

/* Column drawing code ************************************/

void  zMapFeatureColumn(ZMapPane   pane, ZMapColumn *col,
			float     *offset, int frame);
void  buildCols        (ZMapPane   pane);
void  makezMapDefaultColumns(ZMapPane  pane);
float zmapDrawScale     (FooCanvas *canvas, float offset, int start, int end);
void  nbcInit           (ZMapPane  pane, ZMapColumn *col);
void  nbcSelect         (ZMapPane  pane, ZMapColumn *col,
			 void     *seg, int box, double x, double y, gboolean isSelect);
void  zMapGeneDraw      (ZMapPane  pane, ZMapColumn *col, float *offset, int frame);
void  geneSelect        (ZMapPane  pane, ZMapColumn *col,
			 void     *arg, int box, double x, double y, gboolean isSelect);
void  zmapDrawLine          (FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			 char *colour, double thickness);
void  zmapDrawBox           (FooCanvasItem *group, double x1, double y1, 
			 double x2, double y2, char *line_colour, char *fill_colour);
void  zmapDisplayText       (FooCanvasGroup *group, char *text, double x, double y);

/* other routines *****************************************/

gboolean         zmIsOnScreen     (ZMapPane    pane,   Coord coord1, Coord coord2);
VisibleCoord zmVisibleCoord   (ZMapWindow  window, Coord coord);
ScreenCoord  zmScreenCoord    (ZMapPane    pane,   Coord coord);
Coord        zmCoordFromScreen(ZMapPane    pane,   ScreenCoord coord);
gboolean         Quit             (GtkWidget  *widget, gpointer data);

     
#endif
/************************** end of file **********************************/
