/*  File: zmapList.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * and was written by
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Displays a list of features from which the user may select
 *
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Nov  8 15:07 2004 (edgrif)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.10 2004-11-09 14:46:29 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapWindowDrawFeatures.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>

enum { NAME, START, END, TYPE, FEATURE_TYPE, N_COLUMNS };



/* function prototypes ***************************************************/
static void addItemToList(GQuark key_id, gpointer data, gpointer user_data);
static int  tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void scrollToItem(FooCanvas *canvas, gchar *name, int start, gchar *type);
static void locateFeatureSet(GQuark key_id, gpointer data, gpointer user_data);
static void quitListCB   (GtkWidget *window, gpointer data);


/* functions *************************************************************/
void zMapWindowCreateListWindow(ZMapCanvasDataStruct *canvasData, ZMapFeatureSet feature_set)
{
  GtkWidget *treeView = gtk_tree_view_new();
  GtkWidget *featureList, *scrolledWindow;

  GtkTreeStore *list = gtk_tree_store_new(N_COLUMNS,
					  G_TYPE_STRING,
					  G_TYPE_INT,
					  G_TYPE_INT,
					  G_TYPE_STRING,
					  G_TYPE_INT);
  GtkTreeModel *sort_model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  /* set up the top level window */
  canvasData->window->featureListWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width(GTK_CONTAINER(canvasData->window->featureListWindow), 5) ;
  gtk_window_set_title(GTK_WINDOW(canvasData->window->featureListWindow), feature_set->source) ;
  gtk_widget_set_size_request(canvasData->window->featureListWindow, -1, 800);

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(canvasData->window->featureListWindow), scrolledWindow);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  
  /* Build and populate the list */
  g_datalist_foreach(&(feature_set->features),addItemToList , list) ;

  sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(list));

  featureList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_model));

  /* sort the list on the start coordinate */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort_model),
				       START,
				       GTK_SORT_ASCENDING);

  /* finished with list now */
  g_object_unref(G_OBJECT(list));

  /* render the columns */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name",
						     renderer,
						     "text", NAME,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  column = gtk_tree_view_column_new_with_attributes ("Start",
						     renderer,
						     "text", START,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);
  
  column = gtk_tree_view_column_new_with_attributes ("End",
						     renderer,
						     "text", END,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
   
  column = gtk_tree_view_column_new_with_attributes ("Type",
						     renderer,
						     "text", TYPE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
  
  column = gtk_tree_view_column_new_with_attributes ("Feature Type",
						     renderer,
						     "text", FEATURE_TYPE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
  
  /* Setup the selection handler */
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (featureList));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
		    G_CALLBACK (tree_selection_changed_cb),
		    canvasData);

  g_signal_connect(GTK_OBJECT(canvasData->window->featureListWindow), "destroy",
		   GTK_SIGNAL_FUNC(quitListCB), featureList);

  gtk_container_add(GTK_CONTAINER(scrolledWindow), featureList);

  gtk_widget_show_all(canvasData->window->featureListWindow);

  return;
}



static void addItemToList(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature   feature = (ZMapFeature)data ;
  GtkTreeStore *list    = (GtkTreeStore*)user_data;
  GtkTreeIter   iter1;


  gtk_tree_store_append(GTK_TREE_STORE(list), &iter1, NULL);
  gtk_tree_store_set   (GTK_TREE_STORE(list), &iter1, 
			NAME , feature->name,
			START, feature->x1,
			END  , feature->x2,
			TYPE , feature->method_name,
			FEATURE_TYPE, feature->type,
			-1 );

  return;
}



static int tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  int x, y, start = 0, end = 0, feature_type = 0;
  gchar *name, *type;
  GdkColor outline;
  GdkColor foreground;
  static gboolean FIRSTTIME = TRUE;

  /* At present, when we display the list, it will scroll the main display
  ** to the first feature on the list.  This is not good but it's not my
  ** top priority right now. */

  /* retrieve user's selection */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, NAME, &name, START, &start, 
			  END, &end, TYPE, &type, FEATURE_TYPE, &feature_type, -1);
    }
  
  if (start)
    {
      canvasData->feature->name        = name;
      canvasData->feature->x1          = start;
      canvasData->feature->x2          = end;
      canvasData->feature->method_name = type;
      canvasData->feature->type        = feature_type;
  
      scrollToItem(canvasData->canvas, name, start, type);

      gtk_widget_show_all(GTK_WIDGET(canvasData->window->parent_widget));
    }
  
  if (name)      
    g_free(name); 
    
  return TRUE;
}


static void scrollToItem(FooCanvas *canvas, gchar *name, int start, gchar *type)
{
  ZMapCanvasDataStruct *canvasData;
  FooCanvasItem *item;
  GString *string, *string_lower;
  double wx, wy;
  int cx, cy;
  char *dataKey = "canvasData";


  canvasData = g_object_get_data(G_OBJECT(canvas), dataKey);  
  zMapAssert(canvasData->feature_context);
  
  canvasData->feature_set = g_datalist_get_data(&(canvasData->feature_context->feature_sets), type);

  /* scroll up or down to user's selection */
  foo_canvas_w2c(canvas, 0.0, start, &cx, &cy); 
  foo_canvas_scroll_to(canvas, 0, cy);
      
  item = foo_canvas_get_item_at(canvasData->canvas, canvasData->x, start); 
      
  /* lowercase the type to match what's in canvasData->types */
  string = g_string_new(canvasData->feature->method_name);
  string_lower = g_string_ascii_down(string);

  canvasData->thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(canvasData->types), string->str) ;
  zMapAssert(canvasData->thisType);
      
  /* highlight the item */
  zmapWindowHighlightObject(item, canvasData);
  
  zMapFeatureClickCB(canvasData, canvasData->feature); /* show feature details on info_panel  */
      
  return;
}


/*
static void locateFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasDataStruct *canvasData = (ZMapCanvasDataStruct*)user_data;

  if (g_datalist_get_data(&(feature_set), type))
    canvasData->feature_set = feature_set; 

  return;
}
*/


static void quitListCB(GtkWidget *window, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(window));

  return;
}



