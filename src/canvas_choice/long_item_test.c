/*  File: long_item_test.c
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

#include <long_item_test.h>
#include <errno.h>

typedef struct
{
  GtkWidget *toplevel;
  GtkWidget *x1, *x2, *y1, *y2;
  GooCanvas *canvas;
}LongItemTestDataStruct, *LongItemTestData;

static int longItemTestGUI(int argc, char **argv);

static GtkWidget *createCanvas(void);
static void drawAllItems(GooCanvas *canvas);
static void zoom_by(GooCanvas *canvas, double factor);

static void zoominCB(GtkWidget *widget, gpointer user_data);
static void zoomoutCB(GtkWidget *widget, gpointer user_data);
static void describeCB(GtkWidget *widget, gpointer user_data);
static void setregionCB(GtkWidget *widget, gpointer user_data);
static void quitCB(GtkWidget *widget, gpointer user_data);


int main(int argc, char *argv[])
{
  int main_rc;

  main_rc = longItemTestGUI(argc, argv);

  return main_rc;
}

static int longItemTestGUI(int argc, char **argv)
{
  LongItemTestData gui_data;
  GtkWidget *toplevel, *vbox, *canvas, *frame, 
    *button_bar, *zoom_in, *zoom_out, *exit, *describe,
    *scrolled_window, *hbox, *label, *entry, *set;
  GdkColor bg;
  int rc = EXIT_FAILURE;

  if((gui_data = g_new0(LongItemTestDataStruct, 1)))
    {
      gtk_init(&argc, &argv);

      gui_data->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE);
      gtk_window_set_title(GTK_WINDOW(toplevel), LONG_ITEM_TEST_TITLE);
      gtk_container_border_width(GTK_CONTAINER(toplevel), LONG_ITEM_TEST_BORDER);

      g_signal_connect(G_OBJECT(toplevel), "destroy", 
                       G_CALLBACK(quitCB), gui_data);
      /* vbox to put things in */
      vbox = gtk_vbox_new(FALSE, LONG_ITEM_TEST_BORDER);
      gtk_container_add(GTK_CONTAINER(toplevel), vbox);
      
      /* buttons */
      button_bar = gtk_hbutton_box_new();
      zoom_in    = gtk_button_new_with_label("Zoom x 2");
      zoom_out   = gtk_button_new_with_label("Zoom x 0.5");
      describe   = gtk_button_new_with_label("Canvas Info");
      exit       = gtk_button_new_with_label("Quit");

      gtk_container_add(GTK_CONTAINER(button_bar), zoom_in);
      gtk_container_add(GTK_CONTAINER(button_bar), zoom_out);
      gtk_container_add(GTK_CONTAINER(button_bar), describe);
      gtk_container_add(GTK_CONTAINER(button_bar), exit);

      gtk_box_pack_start(GTK_BOX(vbox), button_bar, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      frame = gtk_frame_new(LONG_ITEM_TEST_TITLE);
      gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, LONG_ITEM_TEST_BORDER);

      scrolled_window = gtk_scrolled_window_new(NULL, NULL);
      gtk_container_add(GTK_CONTAINER(frame), scrolled_window);
      
      canvas = createCanvas();
      gui_data->canvas = GOO_CANVAS(canvas);

      gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);
      gdk_color_parse("white", &bg);
      gtk_widget_modify_bg(canvas, GTK_STATE_NORMAL, &bg);

      drawAllItems(gui_data->canvas);

      g_signal_connect(G_OBJECT(zoom_in),  "clicked", G_CALLBACK(zoominCB),  gui_data);
      g_signal_connect(G_OBJECT(zoom_out), "clicked", G_CALLBACK(zoomoutCB), gui_data);
      g_signal_connect(G_OBJECT(describe), "clicked", G_CALLBACK(describeCB), gui_data);
      g_signal_connect(G_OBJECT(exit),     "clicked", G_CALLBACK(quitCB),    gui_data);

      
      hbox = gtk_hbox_new(FALSE, LONG_ITEM_TEST_BORDER);
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      label = gtk_label_new("Bounds");
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      label = gtk_label_new("x1:");
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      gui_data->x1 = entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), "0.0");
      gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      label = gtk_label_new("y1:");
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      gui_data->y1 = entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), "0.0");
      gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      label = gtk_label_new("x2:");
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      gui_data->x2 = entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), "512.0");
      gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      label = gtk_label_new("y2:");
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      gui_data->y2 = entry = gtk_entry_new();
      gtk_entry_set_text(GTK_ENTRY(entry), "1024.0");
      gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, LONG_ITEM_TEST_BORDER);

      set = gtk_button_new_with_label("Set Scroll Region");
      gtk_box_pack_start(GTK_BOX(hbox), set, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      g_signal_connect(G_OBJECT(set), "clicked", G_CALLBACK(setregionCB), gui_data);

      gtk_widget_show_all(toplevel);

      gtk_main();

      demoUtilsExit(EXIT_SUCCESS);
      
      rc = EXIT_FAILURE;
    }

  return rc;
}

static GtkWidget *createCanvas(void)
{
  GooCanvas *canvas;
  GtkWidget *canvas_widget = NULL;
  
  /* create */
  canvas = GOO_CANVAS(canvas_widget = goo_canvas_new());
  
  /* setup it up... */
  g_object_set(G_OBJECT(canvas),
               "x1", 0.0,
               "y1", 0.0,
               "x2", 512.0,
               "y2", 1024.0,
               "units", GTK_UNIT_PIXEL,
               "anchor", GTK_ANCHOR_NORTH_WEST,
               NULL);
  
  return canvas_widget;
}

typedef void (*RecurseNodeFunc)(GooCanvas *canvas, GooCanvasBounds *canvas_bounds,
                                GooCanvasItem *item, GooCanvasBounds *item_bounds,
                                gint depth, gpointer user_data);

typedef struct
{
  GooCanvas       *canvas;
  GooCanvasBounds *canvas_bounds;
  GooCanvasBounds *item_bounds;
  RecurseNodeFunc  cb_func;
  gpointer         cb_data;
  int              depth;
  gboolean         result;
}RecurseItemDataStruct, *RecurseItemData;

static void recurse_tree(GooCanvasItem *item, RecurseItemData rec_data)
{
  int i;

  if(!goo_canvas_item_is_container(item))
    {
      goo_canvas_item_get_bounds(item, rec_data->item_bounds);
      (rec_data->cb_func)(rec_data->canvas, rec_data->canvas_bounds, item, rec_data->item_bounds, rec_data->depth, rec_data->cb_data);
    }
  else
    {
      gint child_count;
      child_count = goo_canvas_item_get_n_children(item);

      rec_data->depth++;

      for(i = 0; i < child_count; i++)
        recurse_tree(goo_canvas_item_get_child(item, i), rec_data);

      rec_data->depth--;
    }

  return ;
}

static gboolean LongItemRecurseItemTree(GooCanvas *canvas, GooCanvasItem *item, RecurseNodeFunc user_func, gpointer user_data)
{
  RecurseItemDataStruct rec_data = {NULL};
  GooCanvasBounds item_bounds = {0.0}; /* save some stack space */
  GooCanvasBounds canvas_bounds = {0.0};

  rec_data.canvas        = canvas;
  rec_data.canvas_bounds = &canvas_bounds;
  rec_data.item_bounds   = &item_bounds;
  rec_data.cb_func       = user_func;
  rec_data.cb_data       = user_data;
  rec_data.depth         = 0;
  rec_data.result        = FALSE;

  goo_canvas_get_bounds(canvas, 
                        &(canvas_bounds.x1), &(canvas_bounds.y1),
                        &(canvas_bounds.x2), &(canvas_bounds.y2));

  recurse_tree(item, &rec_data);
  
  return rec_data.result;
}

static void describe_node_func(GooCanvas *canvas, GooCanvasBounds *cbounds,
                               GooCanvasItem *item, GooCanvasBounds *ibounds, 
                               gint depth, gpointer user_data)
{
  GString *string = (GString *)user_data;
  int i;

  for(i = 0; i < depth; i++)
    g_string_append_printf(string, " ");

  g_string_append_printf(string, 
                         "item bounds: %f, %f -> %f, %f\n", 
                         ibounds->x1,
                         ibounds->y1,
                         ibounds->x2,
                         ibounds->y2);
  return ;
}

static void describe_items(GooCanvas *canvas, GString *string)
{
  GooCanvasItemModel *root_model;
  GooCanvasItem *item;
  GooCanvasBounds bounds = {0.0};

  root_model = goo_canvas_get_root_item_model(canvas);

  item = goo_canvas_get_item(canvas, root_model);

  goo_canvas_item_get_bounds(item, &bounds);

  g_string_append_printf(string, "root item bounds: %f, %f -> %f, %f\n", bounds.x1, bounds.y1, bounds.x2, bounds.y2);

  LongItemRecurseItemTree(canvas, item, describe_node_func, string);

  return ;
}

static char *describe_canvas(GooCanvas *canvas)
{
  GooCanvasBounds bounds = {0.0};
  GString *string;
  gdouble zoom;
  char *ret_val;

  string = g_string_sized_new(512);

  g_string_append_printf(string, "GooCanvas (%p) state:\n", canvas);
  
  zoom = goo_canvas_get_scale(canvas);

  g_string_append_printf(string, "zoom = %f\n", zoom);

  goo_canvas_get_bounds(canvas, &(bounds.x1), &(bounds.y1), &(bounds.x2), &(bounds.y2));

  g_string_append_printf(string, "canvas bounds: %f, %f -> %f, %f\n", bounds.x1, bounds.y1, bounds.x2, bounds.y2);

  g_string_append_printf(string, "canvas pixel size: x = %f, y = %f\n", (bounds.x2 - bounds.x1 + 1) * zoom, (bounds.y2 - bounds.y1 + 1) * zoom);

  describe_items(canvas, string);

  ret_val = string->str;

  g_string_free(string, FALSE);

  return ret_val;
}

typedef struct
{
  GString *clip_path;
  
}ClipItemsDataStruct, *ClipItemsData;

static void set_clip_path(GooCanvas *canvas, GooCanvasBounds *cbounds,
                          GooCanvasItem *item, GooCanvasBounds *ibounds,
                          gint depth, gpointer user_data)
{
  GooCanvasItemModel *model;
  ClipItemsData clip_data = (ClipItemsData)user_data;

  if((model = goo_canvas_item_get_model(item)))
    g_object_set(G_OBJECT(model), 
                 "clip-path", clip_data->clip_path->str, 
                 "clip-fill-rule", CAIRO_FILL_RULE_EVEN_ODD,
                 NULL);
  else
    g_object_set(G_OBJECT(item), "clip-path", clip_data->clip_path->str, NULL);

  return ;
}

static void set_scroll_region(GooCanvas *canvas, GooCanvasBounds *bounds)
{
  ClipItemsDataStruct clip_data = {NULL};

  goo_canvas_set_bounds(canvas, bounds->x1, bounds->y1, bounds->x2, bounds->y2);

  clip_data.clip_path = g_string_sized_new(256);

  g_string_append_printf(clip_data.clip_path, 
                         "M %d %d h %d v %d h %d Z", 
                         (int)(bounds->x1), 
                         (int)(bounds->y1), 
                         (int)(bounds->x2 - bounds->x1 + 1), 
                         (int)(bounds->y2 - bounds->y1 + 1),
                         (int)(bounds->x2 - bounds->x1 + 1) * -1);

  printf("Using clip-path %s\n", clip_data.clip_path->str);

  LongItemRecurseItemTree(canvas, goo_canvas_get_root_item(canvas), set_clip_path, &clip_data);

  goo_canvas_request_redraw(canvas, bounds);

  g_string_free(clip_data.clip_path, TRUE);

  return ;
}

static void drawAllItems(GooCanvas *canvas)
{
  GooCanvasItemModel *root, *group_model, *rect_model;
  GooCanvasBounds bounds[] = {
    {0.0, -512.0, 20.0, 512.0},
    {30.0, 0.0, 20.0, 1024.0},
#ifdef RDS_DONT_INCLUDE
    {0.0, 0.0, 20.0, 1024.0},
    {0.0, 0.0, 20.0, 1024.0},
    {0.0, 0.0, 20.0, 1024.0},
    {0.0, 0.0, 20.0, 1024.0},
    {0.0, 0.0, 20.0, 1024.0},
    {0.0, 0.0, 20.0, 1024.0},
#endif
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0, 0.0}
  }, *bounds_ptr;

  if((root = goo_canvas_group_model_new(NULL, NULL)))
    {
      if((group_model = goo_canvas_group_model_new(root, NULL)))
        {
          bounds_ptr = &bounds[0];
  
          while(bounds_ptr && bounds_ptr->y2 != 0.0)
            {
              rect_model = goo_canvas_rect_model_new(group_model,
                                                     bounds_ptr->x1, bounds_ptr->y1,
                                                     bounds_ptr->x2, bounds_ptr->y2,
                                                     "fill-color", "blue",
                                                     NULL);
              bounds_ptr++;
            }
        }

      goo_canvas_set_root_item_model(canvas, root);
    }

  return ;  
}

static void zoom_by(GooCanvas *canvas, double factor)
{
  double current;
  char *description;

  current = goo_canvas_get_scale(canvas);

  current *= factor;

  goo_canvas_set_scale(canvas, current);

  goo_canvas_scroll_to(canvas, 0.0, 0.0);

  if((description = describe_canvas(canvas)))
    {
      printf("%s\n", description);
      g_free(description);
    }

  return ;
}

static void zoominCB(GtkWidget *widget, gpointer user_data)
{
  LongItemTestData gui_data = (LongItemTestData)user_data;

  zoom_by(gui_data->canvas, 2.0);

  return ;
}

static void zoomoutCB(GtkWidget *widget, gpointer user_data)
{
  LongItemTestData gui_data = (LongItemTestData)user_data;
  
  zoom_by(gui_data->canvas, 0.5);

  return ;
}

static void describeCB(GtkWidget *widget, gpointer user_data)
{
  LongItemTestData gui_data = (LongItemTestData)user_data;
  char *description;
  GtkWidget *dialog, *button;
  gint result;

  if((description = describe_canvas(gui_data->canvas)))
    {
      dialog = gtk_dialog_new_with_buttons("Message",
                                           GTK_WINDOW(gui_data->toplevel),
                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_STOCK_OK,
                                           GTK_RESPONSE_NONE,
                                           NULL);
      
      button = gtk_button_new_with_label(description);
      
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), button, FALSE, FALSE, LONG_ITEM_TEST_BORDER);
      
      gtk_widget_show_all(dialog);

      result = gtk_dialog_run(GTK_DIALOG(dialog));
      
      gtk_widget_destroy(dialog);

      g_free(description);
    }

  return ;
}

static gboolean getEntryCoord(GtkWidget *entry, double *double_out)
{
  gboolean result = FALSE ;
  char *end_ptr, *str ;
  double ret_val ;

  errno = 0 ;

  str = (char *)gtk_entry_get_text(GTK_ENTRY(entry));

  ret_val = strtod(str, &end_ptr) ;

  if (*end_ptr != '\0' || errno != 0)
    result = FALSE ;
  else
    {
      result = TRUE ;
      if (double_out)
        *double_out = ret_val ;
    }
  
  return result ;
}

static void setregionCB(GtkWidget *widget, gpointer user_data)
{
  LongItemTestData gui_data = (LongItemTestData)user_data;
  GooCanvasBounds bounds = {0.0};

  if(getEntryCoord(gui_data->x1, &(bounds.x1)) &&
     getEntryCoord(gui_data->x2, &(bounds.x2)) &&
     getEntryCoord(gui_data->y1, &(bounds.y1)) &&
     getEntryCoord(gui_data->y2, &(bounds.y2)))
    set_scroll_region(gui_data->canvas, &bounds);
  else
    printf("!! Failed to set scrolled region !!\n");

  return ;
}

static void quitCB(GtkWidget *widget, gpointer user_data)
{
  LongItemTestData gui_data = (LongItemTestData)user_data;

  g_free(gui_data);

  demoUtilsExit(EXIT_SUCCESS);

  return ;
}
