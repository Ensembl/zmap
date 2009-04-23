/*  File: zmapWindowGlyphItem_I.h
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
 * Last edited: Apr  6 14:44 2009 (rds)
 * Created: Fri Jan 16 13:56:52 2009 (rds)
 * CVS info:   $Id: zmapWindowGlyphItem_I.h,v 1.1 2009-04-23 09:12:46 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_GLYPH_ITEM_I_H
#define ZMAP_WINDOW_GLYPH_ITEM_I_H

#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>
#include <zmapWindowGlyphItem.h>

#define ZMAP_MAX_POINTS 25


typedef struct _zmapWindowGlyphItemClassStruct
{
  FooCanvasItemClass __parent__;

} zmapWindowGlyphItemClassStruct;

typedef struct _zmapWindowGlyphItemStruct
{
  FooCanvasItem __parent__;

  /* origin */
  double cx, wx;		/* canvas and world */
  double cy, wy;		/* canvas and world */

  /* dimension */
  double cw;			/* width, canvas only*/
  double ch;			/* height, canvas only */

 
  ZMapWindowGlyphItemStyle style;

  /* points */
  double *coords;
  int     num_points;

  /* colours etc... */
  GdkGC *line_gc;
  GdkGC *area_gc;

  guint32 line_rgba;
  guint32 area_rgba;

  gulong line_pixel;
  gulong area_pixel;

  unsigned int line_set : 1;
  unsigned int area_set : 1;

  GdkCapStyle  cap;	
  GdkJoinStyle join;	
  GdkLineStyle line_style;
  int          line_width;
} zmapWindowGlyphItemStruct;


#define ZMAP_IS_WINDOW_POLYGON_ITEM(item) FALSE

#endif /* ZMAP_WINDOW_GLYPH_ITEM_I_H */
