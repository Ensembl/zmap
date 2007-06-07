/*  File: zmapFooUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Apr  2 13:17 2007 (rds)
 * Created: Mon Apr  2 12:37:41 2007 (rds)
 * CVS info:   $Id: zmapFooUtils.c,v 1.1 2007-06-07 13:06:55 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtilsFoo.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapUtils_P.h>

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
                                   g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                                   &font, &font_desc))
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

  if(width_out)
    *width_out  = width;
  if(height_out)
    *height_out = height;
  if(font_desc_out)
    *font_desc_out = font_desc;

  return success;
}
