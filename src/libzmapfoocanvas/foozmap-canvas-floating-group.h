/*  File: foozmap-canvas-floating-group.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * Last edited: Dec 15 13:52 2010 (edgrif)
 * Created: Thu Jan 24 08:39:05 2008 (rds)
 * CVS info:   $Id: foozmap-canvas-floating-group.h,v 1.1 2010-12-23 09:37:04 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef FOO_CANVAS_FLOAT_GROUP_H
#define FOO_CANVAS_FLOAT_GROUP_H

#include <libzmapfoocanvas/foo-canvas.h>

G_BEGIN_DECLS

/* Extended FooCanvasGroup */

enum
{
  ZMAP_FLOAT_AXIS_NONE = 0,
  ZMAP_FLOAT_AXIS_X    = 1,
  ZMAP_FLOAT_AXIS_Y    = 2
};

#define FOO_TYPE_CANVAS_FLOAT_GROUP            (foo_canvas_float_group_get_type ())
#define FOO_CANVAS_FLOAT_GROUP(obj)            (GTK_CHECK_CAST ((obj), FOO_TYPE_CANVAS_FLOAT_GROUP, FooCanvasFloatGroup))
#define FOO_CANVAS_FLOAT_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), FOO_TYPE_CANVAS_FLOAT_GROUP, FooCanvasFloatGroupClass))
#define FOO_IS_CANVAS_FLOAT_GROUP(obj)         (GTK_CHECK_TYPE ((obj), FOO_TYPE_CANVAS_FLOAT_GROUP))
#define FOO_IS_CANVAS_FLOAT_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), FOO_TYPE_CANVAS_FLOAT_GROUP))
#define FOO_CANVAS_FLOAT_GROUP_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), FOO_TYPE_CANVAS_FLOAT_GROUP, FooCanvasFloatGroupClass))

typedef struct _FooCanvasFloatGroup FooCanvasFloatGroup;
typedef struct _FooCanvasFloatGroupClass FooCanvasFloatGroupClass;

struct _FooCanvasFloatGroup 
{
  FooCanvasGroup group;
  
  double zoom_x, zoom_y;
  /* world coords scroll region, also sets min/max x/y positions */
  double scr_x1, scr_y1, scr_x2, scr_y2;

  unsigned int min_x_set : 1;
  unsigned int max_x_set : 1;
  unsigned int min_y_set : 1;
  unsigned int max_y_set : 1;
  
  unsigned int float_axis : 2;
};

struct _FooCanvasFloatGroupClass 
{
  FooCanvasItemClass parent_class;
};


/* Standard Gtk function */
GType foo_canvas_float_group_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif
