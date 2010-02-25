/*  File: zmapWindowCanvas_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul  9 18:24 2009 (rds)
 * Created: Wed Apr 29 14:45:15 2009 (rds)
 * CVS info:   $Id: zmapWindowCanvas_I.h,v 1.3 2010-02-25 14:14:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_CANVAS_I_H
#define ZMAP_WINDOW_CANVAS_I_H

#include <ZMap/zmapBase.h>
#include <zmapWindowCanvas.h>


typedef struct
{
  double x1, x2, y1, y2, ppux, ppuy;

} WindowCanvasLastExposeStruct, *WindowCanvasLastExpose ;


typedef struct _zmapWindowCanvasClassStruct
{
  FooCanvasClass __parent__;

} zmapWindowCanvasClassStruct;

typedef struct _zmapWindowCanvasStruct
{
  FooCanvas __parent__;

  GQueue    *busy_queue;
  GdkCursor *busy_cursor;
  GdkGC     *debug_gc;

  WindowCanvasLastExposeStruct last_cropped_region;

  double max_zoom_x;
  double max_zoom_y;

  double pixels_per_unit_x;
  double pixels_per_unit_y;

  unsigned int canvas_busy : 1;	/* flag for the expose_event, to avoid
				 * too many g_queue_get_length calls */

} zmapWindowCanvasStruct;



#endif /* ZMAP_WINDOW_CANVAS_I_H */
