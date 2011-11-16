/*  File: zmapUtilsFoo.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_UTILS_FOO_H
#define ZMAP_UTILS_FOO_H

#include <libzmapfoocanvas/libfoocanvas.h>

gboolean zMapFoocanvasGetTextDimensions(FooCanvas *canvas,
                                        PangoFontDescription **font_desc_out,
                                        double *width_out,
                                        double *height_out);
void zMap_foo_canvas_sort_items(FooCanvasGroup *group, GCompareFunc compare_func) ;
int zMap_foo_canvas_find_item(FooCanvasGroup *group, FooCanvasItem *item) ;
GList *zMap_foo_canvas_find_list_item(FooCanvasGroup *group, FooCanvasItem *item) ;

guint32 zMap_gdk_color_to_rgba(GdkColor *color);


#endif /* ZMAP_UTILS_FOO_H */
