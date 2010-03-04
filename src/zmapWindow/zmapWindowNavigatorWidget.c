/*  File: zmapWindowNavigatorWidget.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 16 10:05 2010 (edgrif)
 * Created: Mon Sep 18 17:18:37 2006 (rds)
 * CVS info:   $Id: zmapWindowNavigatorWidget.c,v 1.14 2010-03-04 15:13:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindowCanvas.h>
#include <zmapWindowNavigator_P.h>

#define NAVIGATOR_WIDTH 100.0

#define ZMAP_NAVIGATOR_CLASS_DATA "ZMAP_WINDOW_NAV_CLASS_DATA"

typedef struct _ZMapNavigatorClassDataStruct
{
  int width, height;
  double text_height;
  ZMapWindowNavigatorCallbackStruct callbacks;
  gpointer user_data;
  double container_width, container_height;
  FooCanvasItem *top_bg, *bot_bg;
  GdkColor original_colour;
  GtkTooltips *tooltips; /* Tooltips */
} ZMapNavigatorClassDataStruct, *ZMapNavigatorClassData;

static void navCanvasSizeAllocateCB(GtkWidget     *allocatee, 
                                    GtkAllocation *allocation, 
                                    gpointer       user_data);
static double getZoomFactor(GtkWidget *widget, double border, int size);
static void fetchScrollCoords(ZMapNavigatorClassData class_data,
                              double border, 
                              double *x1, double *y1, 
                              double *x2, double *y2);
static gboolean getTextOnCanvasDimensions(FooCanvas *canvas, 
                                          double *width_out,
                                          double *height_out);
static void destroyClassData(gpointer user_data);

static void setupTooltips(GtkWidget *widget, ZMapNavigatorClassData class_data);

static gboolean navigator_debug_G = FALSE;

/* ------------ CREATE --------------- */

GtkWidget *zMapWindowNavigatorCreateCanvas(ZMapWindowNavigatorCallback callbacks, gpointer user_data)
{
  ZMapNavigatorClassData class_data = NULL;
  GtkWidget *canvas_widget = NULL;
  FooCanvas *canvas = NULL;

  canvas_widget = zMapWindowCanvasNew(1.0);
  canvas = FOO_CANVAS(canvas_widget);

  foo_canvas_set_scroll_region(canvas, 0.0, 0.0, 0.0, 0.0);

  if((class_data = g_new0(ZMapNavigatorClassDataStruct, 1)))
    {
      class_data->height      = 0;
      class_data->width       = 0;
      class_data->text_height = 0;

      class_data->container_width  = 0.0; //NAVIGATOR_WIDTH;
      class_data->container_height = (double)(NAVIGATOR_SIZE);

      if(callbacks)
        {
          class_data->callbacks.valueCB = callbacks->valueCB;
          class_data->callbacks.widthCB = callbacks->widthCB;
          class_data->user_data         = user_data;
        }

      getTextOnCanvasDimensions(canvas, NULL, &(class_data->text_height));

      g_signal_connect(GTK_OBJECT(canvas_widget), "size-allocate",
                       GTK_SIGNAL_FUNC(navCanvasSizeAllocateCB), (gpointer)class_data) ;

      g_object_set_data_full(G_OBJECT(canvas_widget),
                             ZMAP_NAVIGATOR_CLASS_DATA,
                             (gpointer)class_data,
                             destroyClassData);
      class_data->tooltips = gtk_tooltips_new();
      setupTooltips(canvas_widget, class_data);

      /* struct copy */
      class_data->original_colour = gtk_widget_get_style(canvas_widget)->bg[GTK_STATE_NORMAL];

      class_data->top_bg = foo_canvas_item_new(foo_canvas_root(canvas),
					       FOO_TYPE_CANVAS_RECT,
					       "x1",             0.0,
					       "y1",             0.0,
					       "x2",             10.0,
					       "y2",             10.0,
					       "width_pixels",   0,
					       "fill_color_gdk", &(class_data->original_colour),
					       NULL);

      class_data->bot_bg = foo_canvas_item_new(foo_canvas_root(canvas),
					       FOO_TYPE_CANVAS_RECT,
					       "x1",             0.0,
					       "y1",             0.0,
					       "x2",             10.0,
					       "y2",             10.0,
					       "width_pixels",   0,
					       "fill_color_gdk", &(class_data->original_colour),
					       NULL);

      foo_canvas_item_lower_to_bottom(class_data->top_bg);
      foo_canvas_item_lower_to_bottom(class_data->bot_bg);
    }


  return canvas_widget ;
}

void zmapWindowNavigatorValueChanged(GtkWidget *widget, double top, double bottom)
{
  ZMapNavigatorClassData class_data = NULL;

  class_data = g_object_get_data(G_OBJECT(widget), ZMAP_NAVIGATOR_CLASS_DATA);
  zMapAssert(class_data);

  if(navigator_debug_G)
    printf("%s to %f %f\n", __PRETTY_FUNCTION__, top, bottom);

  if(class_data->callbacks.valueCB)
    (class_data->callbacks.valueCB)(class_data->user_data, top, bottom);

  return ;
}

void zmapWindowNavigatorWidthChanged(GtkWidget *widget, double left, double right)
{
  ZMapNavigatorClassData class_data = NULL;

  class_data = g_object_get_data(G_OBJECT(widget), ZMAP_NAVIGATOR_CLASS_DATA);
  zMapAssert(class_data);

  if(navigator_debug_G)
    printf("%s to %f %f\n", __PRETTY_FUNCTION__, left, right);

  if(class_data->callbacks.widthCB)
    (class_data->callbacks.widthCB)(class_data->user_data, left, right);

  return ;
}

void zMapWindowNavigatorPackDimensions(GtkWidget *widget, double *width, double *height)
{
  FooCanvas *canvas = NULL;
  FooCanvasGroup *root = NULL;
  double a, b, x, y;
  
  if(FOO_IS_CANVAS(widget))
    {
      canvas = FOO_CANVAS(widget);
      root   = FOO_CANVAS_GROUP(foo_canvas_root(canvas));

      foo_canvas_get_scroll_region(canvas, &a, &b, &x, &y);

      if(a != 0.0)
        x -= a;
      if(b != 0.0)
        y -= b;
      if(x != 0.0)
        x += SHIFT_COLUMNS_LEFT;
    }

  if(*width)
    *width = x;
  if(*height)
    *height = y;

  return ;
}

/* Set the size in the class data. This is the size requested to be
 * filled in the FillWidget method, it might not get the full size. */
void zmapWindowNavigatorSizeRequest(GtkWidget *widget, double x, double y)
{
  ZMapNavigatorClassData class_data = NULL;
  FooCanvas  *canvas = NULL;

  canvas = FOO_CANVAS(widget);

  class_data = g_object_get_data(G_OBJECT(canvas), ZMAP_NAVIGATOR_CLASS_DATA);

  zMapAssert(class_data);
  
  class_data->container_width  = x;
  class_data->container_height = y;

  return ;
}

void zmapWindowNavigatorFillWidget(GtkWidget *widget)
{
  ZMapNavigatorClassData class_data = NULL;
  FooCanvas  *canvas = NULL;
  double curr_pixels = 0.0, 
    target_pixels    = 0.0;

  canvas = FOO_CANVAS(widget);

  class_data = g_object_get_data(G_OBJECT(canvas), ZMAP_NAVIGATOR_CLASS_DATA);

  zMapAssert(class_data);

  curr_pixels   = canvas->pixels_per_unit_y;
  target_pixels = getZoomFactor(GTK_WIDGET(canvas), class_data->text_height, NAVIGATOR_SIZE);

  {
      double x1, x2, y1, y2, x3;
      double border = class_data->text_height / target_pixels;
          
      if(curr_pixels != target_pixels)
        foo_canvas_set_pixels_per_unit_xy(canvas, 1.0, target_pixels);

      fetchScrollCoords(class_data, border, &x1, &y1, &x2, &y2);
      foo_canvas_set_scroll_region(canvas, x1, y1, x2, y2);

      /* use widget->allocation.width instead of x2 as that might not be full width. */
      x3 = widget->allocation.width + (0 - x1);
      if(x3 < x2)
	x3 = x2;

      foo_canvas_item_set(class_data->top_bg,
			  "x1", x1,
			  "y1", y1,
			  "x2", x3,
			  "y2", 0.0,
			  NULL);
      foo_canvas_item_lower_to_bottom(class_data->top_bg);
      foo_canvas_item_set(class_data->bot_bg,
			  "x1", 0.0,
			  "y1", (double)NAVIGATOR_SIZE,
			  "x2", x3,
			  "y2", y2,
			  NULL);
      foo_canvas_item_lower_to_bottom(class_data->bot_bg);

      zmapWindowNavigatorWidthChanged(widget, x1, x2);
  }

  return ;
}

void zmapWindowNavigatorTextSize(GtkWidget *widget, double *x, double *y)
{
  ZMapNavigatorClassData class_data = NULL;
  FooCanvas *canvas = NULL;
  double width, height, ppux, ppuy;

  canvas = FOO_CANVAS(widget);

  class_data = g_object_get_data(G_OBJECT(canvas), ZMAP_NAVIGATOR_CLASS_DATA);

  zMapAssert(class_data);

  ppux = canvas->pixels_per_unit_x;
  ppuy = canvas->pixels_per_unit_y;

  width  = 1.0 / ppux;          /* Not accurate!! */
  height = class_data->text_height / ppuy;

  if(x)
    *x = width;
  if(y)
    *y = height;

  return ;
}


static void navCanvasSizeAllocateCB(GtkWidget     *allocatee, 
                                    GtkAllocation *allocation, 
                                    gpointer       user_data)
{
  ZMapNavigatorClassData class_data = (ZMapNavigatorClassData)user_data;
  int new_width, new_height;

  new_width  = allocation->width;
  new_height = allocation->height;
  
  if(new_height != class_data->height)
    {
      class_data->height = new_height;
      zmapWindowNavigatorFillWidget(allocatee);
    }

  class_data->width = new_width;

  //printf("navCanvasSizeAllocateCB: width = %d, height = %d\n", new_width, new_height);

  return ;
}

/* 
 * #define BORDER (12.0)
 * #define FIXED_HEIGHT 32000
 * zoom = getZoomFactor(widget, BORDER, FIXED_HEIGHT);
 */
static double getZoomFactor(GtkWidget *widget, double border, int size)
{
  double zoom = 0.0, widget_height = 0.0, border_height = 0.0;

  if((widget_height = GTK_WIDGET(widget)->allocation.height) > 0.0)
    {
      border_height = border * 2.0;
      /* rearrangement of
       *              canvas height
       * zf = ------------------------------
       *      seqLength + (text_height / zf)
       */
      zoom = (widget_height - border_height) / (size + 1);
    }
#ifdef RDS_DONT_INCLUDE_UNUSED
  else
    zMapAssertNotReached();
#endif

  return zoom;
}

static void fetchScrollCoords(ZMapNavigatorClassData class_data,
                              double border, 
                              double *x1, double *y1, 
                              double *x2, double *y2)
{
  double border_x = 2.0;
  double border_y = border;
  double max_x, max_y, container_width, container_height;

  zMapAssert(x1 && x2 && y1 && y2);
  
  max_x = 32000.0;             /* only canvas limit */
  max_y = (double)(NAVIGATOR_SIZE);

  container_width  = class_data->container_width;
  container_height = class_data->container_height;

  *x1 = *y1 = *x2 = *y2 = 0.0;

  if(container_width > 0.0)
    {
      *x1 -= border_x;
      *x2 += (container_width > max_x ? max_x : container_width) + border_x;
    }
  if(container_height > 0.0)
    {
      *y1 -= border_y;
      *y2 += (container_height > max_y ? max_y : container_height) + border_y;
    }

  return ;
}

static gboolean getTextOnCanvasDimensions(FooCanvas *canvas, 
                                          double *width_out,
                                          double *height_out)
{
  FooCanvasItem *tmp_item = NULL;
  FooCanvasGroup *root_grp = NULL;
  PangoFontDescription *font_desc = NULL;
  PangoLayout *layout = NULL;
  PangoFont *font = NULL;
  gboolean success = FALSE;
  double width, height;
  int  iwidth, iheight;
  width = height = 0.0;
  iwidth = iheight = 0;

  root_grp = foo_canvas_root(canvas);

  /*

  layout = gtk_widget_create_pango_layout(GTK_WIDGET (canvas), NULL);
  pango_layout_set_font_description (layout, font_desc);
  pango_layout_set_text(layout, text, -1);
  pango_layout_set_alignment(layout, align);
  pango_layout_get_pixel_size(layout, &iwidth, &iheight);

  */

  if(!zMapGUIGetFixedWidthFont(GTK_WIDGET(canvas), 
                               g_list_append(NULL, "Monospace"), 10, PANGO_WEIGHT_NORMAL,
                               &font, &font_desc))
    printf("Couldn't get fixed width font\n");
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

  return success;
}

static void destroyClassData(gpointer user_data)
{
  ZMapNavigatorClassData class_data = (ZMapNavigatorClassData)user_data;

  class_data->width = 
    class_data->height = 0;
  class_data->text_height = 
    class_data->container_width = 
    class_data->container_height = 0.0;
  class_data->callbacks.valueCB  = NULL;
  class_data->user_data = NULL;

  g_free(class_data);

  return ;
}

static void setupTooltips(GtkWidget *widget, ZMapNavigatorClassData class_data)
{
  gtk_tooltips_set_tip(class_data->tooltips, widget,
                       "ZMap Window Navigator.\n"
                       "The box shows the extent of the sequence displayed in the main window. "
                       "Use this to alter the bounds of the display in the main window "
                       "and the scrollbar to the right of the main window to quickly scroll through that data.",
                       ""
                       );

  return ;
}
