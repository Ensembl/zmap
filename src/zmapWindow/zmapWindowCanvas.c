/*  File: zmapWindowCanvas.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2013: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions that wrap the foocanvas calls and control
 *              the interface between our structs and the foocanvas.
 *
 * Exported functions: See zmapWindow_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <zmapWindow_P.h>


void zmapWindowSetScrolledRegion(ZMapWindow window, double x1, double x2, double y1, double y2)
{
  FooCanvas *canvas = FOO_CANVAS(window->canvas) ;
  GtkLayout *layout = &(canvas->layout) ;

  foo_canvas_set_scroll_region(canvas, x1, y1, x2, y2) ;

  gtk_layout_get_size(layout, &(window->layout_actual_width), &(window->layout_actual_height)) ;

  return ;
}


void zmapWindowSetPixelxy(ZMapWindow window, double pixels_per_unit_x, double pixels_per_unit_y)
{
  FooCanvas *canvas = FOO_CANVAS(window->canvas) ;
  GtkLayout *layout = &(canvas->layout) ;

  foo_canvas_set_pixels_per_unit_xy(canvas, pixels_per_unit_x, pixels_per_unit_y) ;

  gtk_layout_get_size(layout, &(window->layout_actual_width), &(window->layout_actual_height)) ;

  return ;
}



