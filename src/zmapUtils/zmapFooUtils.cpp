/*  File: zmapFooUtils.c
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
 * Description: Some utility routines for operating on the Foocanvas
 *              which were not implemented by the foocanvas itself.
 *
 * Exported functions: See ZMap/zmapUtilsFoo.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>






#include <ZMap/zmapUtilsFoo.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <zmapUtils_P.hpp>



/* Allows caller to sort canvas items, perhaps by position, NOTE that the last item _must_ be reset. */
void zMap_foo_canvas_sort_items(FooCanvasGroup *group, GCompareFunc compare_func)
{
  group->item_list = g_list_sort(group->item_list, compare_func) ;

  group->item_list_end = g_list_last(group->item_list) ;

  return ;
}


int zMap_foo_canvas_find_item(FooCanvasGroup *group, FooCanvasItem *item)
{
  int index = 0 ;

  index = g_list_index(group->item_list, item) ;

  return index ;
}


GList *zMap_foo_canvas_find_list_item(FooCanvasGroup *group, FooCanvasItem *item)
{
  GList *list_item = NULL ;

  list_item = g_list_find(group->item_list, item) ;

  return list_item ;
}


gboolean zMapFoocanvasGetTextDimensions(FooCanvas *canvas,
                                        PangoFontDescription **font_desc_out,
                                        double *width_out,
                                        double *height_out)
{
  FooCanvasItem *tmp_item = NULL;
  FooCanvasGroup *root_grp = NULL;
  static PangoFontDescription *font_desc = NULL;
  PangoLayout *layout = NULL;
  PangoFont *font = NULL;
  gboolean success = FALSE;
  double width, height;
  int  iwidth, iheight;
  width = height = 0.0;
  iwidth = iheight = 0;

  if(!(root_grp = foo_canvas_root(canvas)))
    zMapLogFatal("%s", "No canvas root!");

  /*
    layout = gtk_widget_create_pango_layout(GTK_WIDGET (canvas), NULL);
    pango_layout_set_font_description (layout, font_desc);
    pango_layout_set_text(layout, text, -1);
    pango_layout_set_alignment(layout, align);
    pango_layout_get_pixel_size(layout, &iwidth, &iheight);
  */

  if(font_desc == NULL)
    {
      if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(canvas),
                                   g_list_append(NULL, (void *)ZMAP_ZOOM_FONT_FAMILY),
                                   ZMAP_ZOOM_FONT_SIZE, PANGO_WEIGHT_NORMAL,
                                   &font, &font_desc))
        {
          zMapLogWarning("%s", "Couldn't get fixed width font");
        }
      else
        {
          tmp_item = foo_canvas_item_new(root_grp,
                                         foo_canvas_text_get_type(),
                                         "x",         0.0,
                                         "y",         0.0,
                                         "text",      "A",
                                         "font_desc", font_desc,
                                         NULL);
      	   layout = FOO_CANVAS_TEXT(tmp_item)->layout;
      	   pango_layout_get_pixel_size(layout, &iwidth, &iheight);
      	   width  = (double)iwidth;
          height = (double)iheight;

      	   gtk_object_destroy(GTK_OBJECT(tmp_item));
      	   success = TRUE;
        }
    }

  if(width_out)
    *width_out  = width;
  if(height_out)
    *height_out = height;
  if(font_desc_out)
    *font_desc_out = font_desc;

  return success;
}


/* where to put this? the foo canvas has it in-line ijn a few places */
guint32 zMap_gdk_color_to_rgba(GdkColor *color)
{
  guint32 rgba = 0;

  rgba = ((color->red & 0xff00) << 16  |
        (color->green & 0xff00) << 8 |
        (color->blue & 0xff00)       |
        0xff);

  return rgba;
}

