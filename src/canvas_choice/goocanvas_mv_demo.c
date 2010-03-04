/*  File: crcanvas_demo.c
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
 * Last edited: Jun 10 12:13 2009 (rds)
 * Created: Wed Mar 14 21:39:04 2007 (rds)
 * CVS info:   $Id: goocanvas_mv_demo.c,v 1.3 2010-03-04 14:49:03 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <goocanvas_mv_demo.h>

typedef struct
{
  demoGUI       gui;
  demoContainer container;
  GooCanvas     *canvas;
  GTimer       *timer;
}GooCanvasTestSuiteDataStruct, *GooCanvasTestSuiteData;

static int zmapGooCanvasTestSuite(int argc, char *argv[]);

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

  main_rc = zmapGooCanvasTestSuite(argc, argv);

  return main_rc;
}

/* INTERNALS */

/* setup... */
static int zmapGooCanvasTestSuite(int argc, char *argv[])
{
  GooCanvasTestSuiteData suite;

  gtk_init(&argc, &argv);

  if((suite = g_new0(GooCanvasTestSuiteDataStruct, 1)))
    {

      suite->gui       = demoGUICreate("GooCanvas Demo", &gui_funcs_G, suite);
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
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;
  GtkWidget *canvas = NULL;

  canvas = goo_canvas_new();

  suite->canvas = GOO_CANVAS(canvas);

  goo_canvas_set_bounds(suite->canvas, 0.0, 0.0, CANVAS_WIDTH, CANVAS_HEIGHT);

  return canvas;
}

static void random_items_cb(GtkWidget *button, gpointer user_data)
{
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;
  printf("Drawing ...\n");

  srand(time(NULL));

  demoContainerExercise(suite->container);

  return ;
}

static void multiply_zoom(GooCanvasTestSuiteData suite, double factor)
{
  double current;

  current = goo_canvas_get_scale(suite->canvas);

  goo_canvas_set_scale(suite->canvas, current * factor);

  printf("new zoom = %f\n", current * factor);

  return ;
}

/* called when zoom_in button is pressed */
static void zoom_in_cb(GtkWidget *button, gpointer user_data)
{
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;

  printf("Zoom In CB\n");

  multiply_zoom(suite, 2.0);

  return ;
}

/* called when zoom out button is pressed */
static void zoom_out_cb(GtkWidget *button, gpointer user_data)
{
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;

  printf("Zoom Out CB\n");

  multiply_zoom(suite, 0.5);

  return ;
}

/* called by the toplevel destroy... */
static void suite_destroy_notify_cb(gpointer user_data)
{
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;

  printf("Suite DestroyNotify CB\n");

  g_free(suite);

  return ;
}


static gboolean goocanvas_item_debug_G = FALSE;
/* canvas stuff */
static gpointer create_item_cb(gpointer parent_item_data, gpointer user_data)
{
#ifdef RDS_DONT_INCLUDE
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;
  GooCanvasBounds bounds = {0.0};
  GooCanvasItem *bounds_item = NULL;
#endif /* RDS_DONT_INCLUDE */
  GooCanvasItemModel *item = NULL;
  double x, y, width, height;

  if(goocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  x = (double)(rand() % (int)COLUMN_WIDTH);
  y = 0.0; //(double)(rand() % (int)CANVAS_HEIGHT);

  width  = COLUMN_WIDTH;
  height = ITEM_MAX_HEIGHT; //(double)(rand() % (int)ITEM_MAX_HEIGHT);

  if(width == 0.0)
    width += 1.0;
  if(height == 0.0)
    height += 1.0;

  item = goo_canvas_rect_model_new(parent_item_data,
                                   x, y, width, height, 
                                   "fill-color", "red",
                                   NULL);

  return item;
}

static gpointer create_group_cb(gpointer parent_item_data, double leftmost, gpointer user_data)
{
  GooCanvasItemModel *parent = parent_item_data;
  GooCanvasItemModel *group = NULL;

  if(goocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  if(!parent)
    {
      GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;
      parent = goo_canvas_group_model_new(NULL, NULL);
      goo_canvas_set_root_item_model(suite->canvas, parent);
    }

  if(parent)
    {
      group = goo_canvas_group_model_new(parent, NULL);
    }

  if(group)
    {
      goo_canvas_item_model_set_simple_transform(group, leftmost, 0.0, 1.0, 0.0);
    }

  return group;
}

static gpointer get_item_bounds(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data)
{
  GooCanvasTestSuiteData suite = (GooCanvasTestSuiteData)user_data;
  GooCanvasBounds bounds = {0.0};
  GooCanvasItem *item;

  item = goo_canvas_get_item(suite->canvas, parent_item_data);

  goo_canvas_item_get_bounds(item, &bounds);

  if(x1)
    *x1 = bounds.x1;
  if(x2)
    *x2 = bounds.x2;
  if(y1)
    *y1 = bounds.y1;
  if(y2)
    *y2 = bounds.y2;

  return NULL;
}

static gpointer hide_item_cb(gpointer parent_item_data, gpointer user_data)
{
  if(goocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  return NULL;
}

static gpointer move_item_cb(gpointer parent_item_data, gpointer user_data)
{
  if(goocanvas_item_debug_G)
    printf("%s\n", __PRETTY_FUNCTION__);

  return NULL;
}



