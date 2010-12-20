/*  File: zmapWindowLongItem_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Dec 15 15:19 2010 (edgrif)
 * Created: Fri Jan 16 13:56:52 2009 (rds)
 * CVS info:   $Id: zmapWindowLongItem_I.h,v 1.4 2010-12-20 12:20:03 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_WINDOW_LONG_ITEM_I_H__
#define __ZMAP_WINDOW_LONG_ITEM_I_H__

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowLongItem.h>



typedef struct _zmapWindowLongItemClassStruct
{
  FooCanvasItemClass __parent__;

} zmapWindowLongItemClassStruct;

typedef struct _zmapWindowLongItemStruct
{
  FooCanvasItem __parent__;

  FooCanvasItem      *long_item;
  FooCanvasItemClass *long_item_class; /* save get_class calls */
  GObjectClass       *object_class; /* save get_parent_class calls */

  struct
  {
    union
    {
      FooCanvasPoints *points;

      double box_coords[4];
    } shape;

    gboolean has_points;

    double extent_box[4];

  } shape;


} zmapWindowLongItemStruct;


#endif /* __ZMAP_WINDOW_LONG_ITEM_I_H__ */
