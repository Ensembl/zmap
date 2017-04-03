/*  File: zmapUtilsFoo.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
guint32 zMap_gdk_color_to_rgba(GdkColor *color);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#endif /* ZMAP_UTILS_FOO_H */
