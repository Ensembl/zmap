/*  File: crcanvas_demo.c
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
 * Last edited: Jun 10 12:12 2009 (rds)
 * Created: Wed Mar 14 21:39:04 2007 (rds)
 * CVS info:   $Id: crcanvas_demo.c,v 1.3 2009-12-15 13:49:10 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <crcanvas_demo.h>

typedef struct
{
  demoGUI       gui;
  demoContainer container;
  CrCanvas     *canvas;
  GTimer       *timer;
}CrCanvasTestSuiteDataStruct, *CrCanvasTestSuiteData;

static int zmapCrCanvasTestSuite(int argc, char *argv[]);

static GtkWidget *create_canvas_cb(GtkWidget *container, gpointer user_data);
static void random_items_cb(GtkWidget *button, gpointer user_data);
static void zoom_in_cb(GtkWidget *button, gpointer user_data);
static void zoom_out_cb(GtkWidget *button, gpointer user_data);

static void suite_destroy_notify_cb(gpointer user_data);

static gpointer create_item_cb(gpointer parent_item_data,  gpointer user_data);
static gpointer create_group_cb(gpointer parent_item_data, double leftmost, gpointer user_data);
#ifdef RDS_DONT_INCLUDE
static gpointer get_item_bounds(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data);
#endif /* RDS_DONT_INCLUDE */
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
  create_item_cb, 
  create_group_cb,
  hide_item_cb,
  move_item_cb, 
  NULL
};

/* main */
int main(int argc, char *argv[])
{
  int main_rc;

  main_rc = zmapCrCanvasTestSuite(argc, argv);

  return main_rc;
}

/* INTERNALS */

/* setup... */
static int zmapCrCanvasTestSuite(int argc, char *argv[])
{
  CrCanvasTestSuiteData suite;

  gtk_init(&argc, &argv);

  if((suite = g_new0(CrCanvasTestSuiteDataStruct, 1)))
    {
      
      suite->gui       = demoGUICreate("CrCanvas Demo", &gui_funcs_G, suite);
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
  CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;
  GtkWidget *canvas = NULL;

  canvas = cr_canvas_new("maintain-center", FALSE,
                         "maintain_aspect", FALSE,
                         "auto-scale", TRUE,
                         NULL);

  suite->canvas = CR_CANVAS(canvas);

  cr_canvas_set_scroll_region(suite->canvas, 
                              CANVAS_ORIGIN_X, CANVAS_ORIGIN_Y,
                              CANVAS_WIDTH  + CANVAS_ORIGIN_X, 
                              CANVAS_HEIGHT + CANVAS_ORIGIN_Y);
  cr_canvas_set_scroll_factor(CR_CANVAS(canvas), 3, 3);

  cr_canvas_set_min_scale_factor(suite->canvas, 0.001, 0.001);
  cr_canvas_set_max_scale_factor(suite->canvas, 100.0, 100.0);

  return canvas;
}

static void random_items_cb(GtkWidget *button, gpointer user_data)
{
  CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;
  printf("Drawing ...\n");

  demoContainerExercise(suite->container);

  return ;
}

static void multiply_zoom_y(CrCanvasTestSuiteData suite, double factor)
{
#ifdef RDS_DONT_INCLUDE
  CrItem *item;

  cairo_matrix_t matrix;
  double current_x_scale, current_y_scale;

  item = CR_ITEM(suite->canvas->root);
  matrix = *(cr_item_get_matrix(item));

  /* remove rotation - scale is limited as if there were no rotation */
  cairo_matrix_rotate(&matrix, -atan2(matrix.yx, matrix.yy));

  current_x_scale = current_y_scale = 1;
  cairo_matrix_transform_distance(&matrix, &current_x_scale, 
                                  &current_y_scale);
#endif

  /* Isn't this nice! */
  cr_canvas_zoom(suite->canvas, factor, 1.0);

  return ;
}

/* called when zoom_in button is pressed */
static void zoom_in_cb(GtkWidget *button, gpointer user_data)
{
  CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;

  printf("Zoom In CB\n");

  multiply_zoom_y(suite, 2.0);

  return ;
}

/* called when zoom out button is pressed */
static void zoom_out_cb(GtkWidget *button, gpointer user_data)
{
  CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;

  printf("Zoom Out CB\n");

  multiply_zoom_y(suite, 0.5);

  return ;
}

/* called by the toplevel destroy... */
static void suite_destroy_notify_cb(gpointer user_data)
{
#ifdef  RDS_DONT_INCLUDE
  CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;
#endif /* RDS_DONT_INCLUDE */
  printf("Suite DestroyNotify CB\n");

  return ;
}

/* canvas stuff */
static gpointer create_item_cb(gpointer parent_item_data, gpointer user_data)
{
  CrItem *item = NULL;
  double x, y, width, height;
  
  x = (double)(rand() % (int)COLUMN_WIDTH);
  y = (double)(rand() % (int)CANVAS_HEIGHT);

  width  = COLUMN_WIDTH;
  height = (double)(rand() & (int)ITEM_MAX_HEIGHT);

  if(width == 0.0)
    width += 1.0;
  if(height == 0.0)
    height += 1.0;

  item = cr_rectangle_new(parent_item_data, x, y, width, height, 
                          "fill_color", "red", 
                          "outline_color", "black",
                          NULL);

  //cr_item_add(parent_item_data, item);

  //g_object_unref(item);

  return item;
}

static gpointer create_group_cb(gpointer parent_item_data, double leftmost, gpointer user_data)
{
  CrItem *parent = parent_item_data;
  CrItem *new_item = NULL;
 
  if(!parent)
    {
      CrCanvasTestSuiteData suite = (CrCanvasTestSuiteData)user_data;
      parent = CR_CANVAS(suite->canvas)->root;
    }
 
  if(parent)
    {
      new_item  = cr_inverse_new(parent, leftmost, 0.0, NULL);
    }

  return new_item;
}
#ifdef RDS_DONT_INCLUDE
static gpointer get_item_bounds(gpointer parent_item_data, double *x1, double *y1, double *x2, double *y2, gpointer user_data)
{

  cr_item_get_bounds(parent_item_data, x1, y1, x2, y2);

  return NULL;
}
#endif /* RDS_DONT_INCLUDE */

static gpointer hide_item_cb(gpointer parent_item_data, gpointer user_data)
{
  return NULL;
}

static gpointer move_item_cb(gpointer parent_item_data, gpointer user_data)
{
  return NULL;
}



