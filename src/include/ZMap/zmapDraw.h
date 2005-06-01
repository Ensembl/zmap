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
 * Last edited: Jun  1 13:53 2005 (rds)
 * Created: Tue Jul 27 16:40:47 2004 (edgrif)
 * CVS info:   $Id: zmapDraw.h,v 1.16 2005-06-01 13:05:49 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DRAW_H
#define ZMAP_DRAW_H

#include <libfoocanvas/libfoocanvas.h>

#define MINVAL(x, y) ((x) < (y) ? (x) : (y))
#define MAXVAL(x, y) ((x) > (y) ? (x) : (y))

FooCanvasItem *zMapDrawLine(FooCanvasGroup *group, double x1, double y1, double x2, double y2, 
			    GdkColor *colour, double thickness) ;
FooCanvasItem *zMapDrawPolyLine(FooCanvasGroup *group, FooCanvasPoints *points,
				GdkColor *colour, double thickness) ;
FooCanvasItem *zMapDrawBox(FooCanvasItem *group, 
			   double x1, double y1, double x2, double y2, 
			   GdkColor *line_colour, GdkColor *fill_colour) ;
FooCanvasItem *zMapDisplayText(FooCanvasGroup *group, char *text, char *colour,
			       double x, double y) ;

FooCanvasItem *zMapDrawScale(FooCanvas *canvas, 
                             double offset, double zoom_factor, int start, int end,
			     int *major_units_out, int *minor_units_out);


/* This needs to be a bit cleverer, so you can't actually move the origin */
FooCanvasItem *zMapRubberbandCreate(FooCanvas *canvas);

void zMapRubberbandResize(FooCanvasItem *band, 
                          double origin_x, double origin_y, 
                          double current_x, double current_y
                          );

     
#endif /* ZMAP_DRAW_H */
