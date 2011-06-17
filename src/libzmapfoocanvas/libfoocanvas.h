/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#ifndef LIBFOOCANVAS_H
#define LIBFOOCANVAS_H

#include <libzmapfoocanvas/foo-canvas.h>
#include <libzmapfoocanvas/foo-canvas-line.h>
#include <libzmapfoocanvas/foo-canvas-text.h>
#include <libzmapfoocanvas/foo-canvas-polygon.h>
#include <libzmapfoocanvas/foo-canvas-pixbuf.h>
#include <libzmapfoocanvas/foo-canvas-widget.h>
#include <libzmapfoocanvas/foo-canvas-rect-ellipse.h>
#include <libzmapfoocanvas/foo-canvas-util.h>

#include <libzmapfoocanvas/foozmap-canvas-line-glyph.h>
#include <libzmapfoocanvas/foozmap-canvas-text.h>
#include <libzmapfoocanvas/foozmap-canvas-floating-group.h>

G_BEGIN_DECLS

GType foo_canvas_points_get_type (void);
#define FOO_TYPE_CANVAS_POINTS foo_canvas_points_get_type()

G_END_DECLS

#endif /* LIBFOOCANVAS_H */
