/*  File: crcanvas_demo.c
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

#include <foocanvas_demo.h>

typedef struct
{
  demoGUI       gui;
  demoContainer container;
  FooCanvas     *canvas;
  GTimer       *timer;
}FooCanvasTestSuiteDataStruct, *FooCanvasTestSuiteData;

static int zmapFooCanvasTestSuite(int argc, char *argv[]);

static GtkWidget *create_canvas_cb(GtkWidget *container, gpointer user_data);
static void random_items_cb(GtkWidget *button, gpointer user_data);
static void zoom_in_cb(GtkWidget *button, gpointer user_data);
static void zoom_out_cb(GtkWidget *button, gpointer user_data);

static void suite_destroy_notify_cb(gpointer user_data);

static gpointer create_item_cb(gpointer parent_item_data,  gpointer user_data);
static gpointer create_group_cb(gpointer parent_item_data, double leftmost, gpointer user_data);
static gpointer get_item_bounds(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data);
static gpointer hide_item_cb(gpointer parent_item_data,    gpointer user_data);
static gpointer move_item_cb(gpointer parent_item_data,    gpointer user_data);


static demoGUICallbackSetStruct gui_funcs_G = {
  create_canvas_cb, 
  G_CALLBACK(random_items_cb),
  G_CALLBACK(zoom_in_cb), 
  G_CALLBACK(zoom_out_cb), 
  NULL, NULL, suite_destroy_notify_cb
};
static demoContainerCallbackSetStruct container_funcs_G = {
  create_item_cb, create_group_cb,
  get_item_bounds,
  hide_item_cb,   move_item_cb, NULL
};

/* main */
int main(int argc, char *argv[])
{
  int main_rc;

  main_rc = zmapFooCanvasTestSuite(argc, argv);

  return main_rc;
}

/* INTERNALS */

/* setup... */
static int zmapFooCanvasTestSuite(int argc, char *argv[])
{
  FooCanvasTestSuiteData suite;

  gtk_init(&argc, &argv);

  if((suite = g_new0(FooCanvasTestSuiteDataStruct, 1)))
    {

      suite->gui       = demoGUICreate("FooCanvas Demo", &gui_funcs_G, suite);
      suite->container = demoContainerCreate(NULL, &container_funcs_G, suite);

      gtk_main();

      demoUtilsExit(EXIT_SUCCESS);

      return EXIT_FAILURE;
    }

  return EXIT_FAILURE;
}

/* The real code... */

/* to create our canvas! */
static GtkWidget *create_canvas_cb(GtkWidget *container, gpointer user_data)
{
  FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;
  GtkWidget *canvas = NULL;

  canvas = foo_canvas_new();

  suite->canvas = FOO_CANVAS(canvas);

  foo_canvas_set_scroll_region(suite->canvas, 0.0, 0.0, 10 * (COLUMN_WIDTH + COLUMN_SPACE), CANVAS_HEIGHT);

  return canvas;
}

static void random_items_cb(GtkWidget *button, gpointer user_data)
{
  FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;
  printf("Drawing ...\n");

  srand(time(NULL));

  demoContainerExercise(suite->container);

  return ;
}

static void multiply_zoom(FooCanvasTestSuiteData suite, double factor)
{
  FooCanvas *canvas = FOO_CANVAS(suite->canvas);
  double current;

  current = canvas->pixels_per_unit_y;

  foo_canvas_set_pixels_per_unit_xy(canvas, canvas->pixels_per_unit_x, current * factor);

  return ;
}

/* called when zoom_in button is pressed */
static void zoom_in_cb(GtkWidget *button, gpointer user_data)
{
  FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;

  printf("Zoom In CB\n");

  multiply_zoom(suite, 2.0);

  return ;
}

/* called when zoom out button is pressed */
static void zoom_out_cb(GtkWidget *button, gpointer user_data)
{
  FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;

  printf("Zoom Out CB\n");

  multiply_zoom(suite, 0.5);

  return ;
}

/* called by the toplevel destroy... */
static void suite_destroy_notify_cb(gpointer user_data)
{
  FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;

  printf("Suite DestroyNotify CB\n");

  g_free(suite);

  return ;
}


static gboolean foocanvas_item_debug_G = FALSE;
/* canvas stuff */
static gpointer create_item_cb(gpointer parent_item_data, gpointer user_data)
{
  FooCanvasItem *item = NULL;
  double x, y, width, height;

  if(foocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  x = (double)(rand() % (int)COLUMN_WIDTH);
  y = (double)(rand() % (int)CANVAS_HEIGHT);

  width  = COLUMN_WIDTH;
  height = (double)(rand() % (int)ITEM_MAX_HEIGHT);

  item = foo_canvas_item_new(parent_item_data,
                             foo_canvas_rect_get_type(),
                             "x1", x, 
                             "y1", y, 
                             "x2", x + width, 
                             "y2", y + height, 
                             "fill-color", "red",
                             "outline-color", "black",
                             NULL);

  return item;
}

static gpointer create_group_cb(gpointer parent_item_data, double leftmost, gpointer user_data)
{
  FooCanvasItem *parent = parent_item_data;
  FooCanvasGroup *group = NULL;

  if(foocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  if(!parent)
    {
      FooCanvasTestSuiteData suite = (FooCanvasTestSuiteData)user_data;
      parent = FOO_CANVAS_ITEM(foo_canvas_root(suite->canvas));
    }

  if(parent)
    {
      group = FOO_CANVAS_GROUP(foo_canvas_item_new(FOO_CANVAS_GROUP(parent), 
                                                   foo_canvas_group_get_type(),
                                                   "x", leftmost, 
                                                   "y", 0.0, 
                                                   NULL));
    }

  return group;
}

static gpointer get_item_bounds(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data)
{

  foo_canvas_item_get_bounds(parent_item_data, x1, y1, x2, y2);

  return NULL;
}

static gpointer hide_item_cb(gpointer parent_item_data, gpointer user_data)
{
  if(foocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  return NULL;
}

static gpointer move_item_cb(gpointer parent_item_data, gpointer user_data)
{
  if(foocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  return NULL;
}



