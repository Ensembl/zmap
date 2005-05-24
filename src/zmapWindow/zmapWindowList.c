/*  File: zmapWindowList.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Displays a list of features from which the user may select
 *
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: May 24 22:12 2005 (rds)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.29 2005-05-24 21:41:20 rds Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>



/* I have changed this function quite a lot and it could do with more tidying up still,
 * there is much that is not very tidy about it at all. EG */



enum { NAME, STRAND, START, END, ID, TYPE, FEATURE_TYPE, X_COORD, FEATURE_ITEM, N_COLUMNS };


/* ListColStruct structure declaration
 *
 * Used to pass a couple of variables and a GtkTreeStore list
 * of feature details through the g_datalist_foreach() call 
 * and into the addItemToList() function where the list is loaded.
 */
typedef struct
{
  ZMapWindow    window ;
  ZMapStrand    strand;					    /*!< up or down strand (enum). */
  double        x_coord;				    /*!< X coordinate of selected column. */
  GtkTreeStore *list;					    /*!< list of features in selected column. */
} ListColStruct, *ListCol ;				    /*!< pointer to ListColStruct. */



/* function prototypes ***************************************************/
static void addItemToList             (GQuark key_id, gpointer data, gpointer user_data);
static GtkTreeIter findItemInList     (ZMapWindow zmapWindow, GtkTreeModel *sort_model, FooCanvasItem *item);
static int  tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void recalcScrollRegion        (ZMapWindow window, double start, double end);
static void quitListCB                (GtkWidget *window, gpointer data);


/* functions *************************************************************/


/* Displays a list of selectable features
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 */


void zMapWindowCreateListWindow(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  GtkWidget *window, *featureList, *vbox, *hbox, *closeButton, *scrolledWindow;
  GtkTreeModel *sort_model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  ListCol listCol ;
  ZMapFeature feature ;
  double x1, y1, x2, y2;
  GData *feature_sets ;
  GtkTreeIter iter;
  GtkTreePath *path;

  listCol = g_new0(ListColStruct, 1) ;

  listCol->window = zmapWindow ;

  /* Get hold of the feature corresponding to this item. */
  feature = g_object_get_data(G_OBJECT(item), "feature");  
  zMapAssert(feature) ;


  foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);	    /* world coords */

  listCol->list = gtk_tree_store_new(N_COLUMNS,
				     G_TYPE_STRING,
				     G_TYPE_STRING,
				     G_TYPE_INT,
				     G_TYPE_INT,
				     G_TYPE_INT,
				     G_TYPE_STRING,
				     G_TYPE_STRING,
				     G_TYPE_DOUBLE,
				     G_TYPE_POINTER) ;

  /* I have no idea why this x coord is kept.... */
  listCol->x_coord = x1;

  /* need to know whether the column is up or down strand, 
   * so we load the right set of features */
  listCol->strand = feature->strand ;


  /* set up the top level window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_object_set_data(G_OBJECT(window), "zmapWindow", zmapWindow) ;

  g_ptr_array_add(zmapWindow->featureListWindows, (gpointer)window);

  gtk_container_border_width(GTK_CONTAINER(window), 5) ;
  gtk_window_set_title(GTK_WINDOW(window), g_quark_to_string(feature->style)) ;
  gtk_window_set_default_size(GTK_WINDOW(window), -1, 600); 

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_box_pack_start(GTK_BOX(vbox), scrolledWindow, TRUE, TRUE, 0);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  closeButton = gtk_button_new_with_label("Close") ;
  g_object_set_data(G_OBJECT(closeButton), "zmapWindow", zmapWindow) ;
  gtk_box_pack_start(GTK_BOX(hbox), closeButton, TRUE, TRUE, 0) ;


  /* Build and populate the list of features */
  feature_sets = zMapFeatureFindSetInContext(zmapWindow->feature_context, feature->style) ;
  g_datalist_foreach(&feature_sets, addItemToList, listCol) ;

  sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(listCol->list));

  featureList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_model));

  /* connect close button signal now that featureList exists */
  g_signal_connect(GTK_OBJECT(closeButton), "clicked",
		   GTK_SIGNAL_FUNC(quitListCB), window);

  /* sort the list on the start coordinate */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort_model),
				       START,
				       GTK_SORT_ASCENDING);

  /* render the columns. */
  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes ("Name",
						     renderer,
						     "text", NAME,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  column = gtk_tree_view_column_new_with_attributes ("Strand",
						     renderer,
						     "text", STRAND,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);
  
  gtk_tree_view_column_set_visible(column, TRUE);
   
  column = gtk_tree_view_column_new_with_attributes ("Start",
						     renderer,
						     "text", START,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);
  
  gtk_tree_view_column_set_visible(column, TRUE);
   
  column = gtk_tree_view_column_new_with_attributes ("End",
						     renderer,
						     "text", END,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, TRUE);
   
  column = gtk_tree_view_column_new_with_attributes ("ID",
						     renderer,
						     "text", ID,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
   
  column = gtk_tree_view_column_new_with_attributes ("Feature Type",
						     renderer,
						     "text", FEATURE_TYPE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, TRUE);
  
  column = gtk_tree_view_column_new_with_attributes ("Type",
						     renderer,
						     "text", TYPE,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, TRUE);
  
  column = gtk_tree_view_column_new_with_attributes ("X Coord",
						     renderer,
						     "text", X_COORD,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
   
  column = gtk_tree_view_column_new_with_attributes ("Feature Item",
						     renderer,
						     "text", FEATURE_ITEM,
						     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (featureList), column);

  gtk_tree_view_column_set_visible(column, FALSE);
   
  /* Setup the selection handler */
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (featureList));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);


  g_signal_connect (G_OBJECT (select), "changed",
		    G_CALLBACK (tree_selection_changed_cb),
		    zmapWindow);

  g_signal_connect(GTK_OBJECT(window), "destroy",
		   GTK_SIGNAL_FUNC(quitListCB), window);

  gtk_container_add(GTK_CONTAINER(scrolledWindow), featureList);

  /* find the item the user clicked on. */
  iter = findItemInList(zmapWindow, sort_model, item);

  /* If the clicked object was an exon or intron, then that won't have been
   * found, as the list holds only the transcript name, so look again. */
  if (!iter.stamp)
    {
      FooCanvasItem *transcript_item = zmapWindowFToIFindItem(listCol->window->feature_to_item,
						   feature->style, feature->unique_id) ;
      zMapAssert(transcript_item) ;
      iter = findItemInList(zmapWindow, sort_model, transcript_item);
    }

  /* finished with list now */
  g_object_unref(G_OBJECT(listCol->list));
  g_free(listCol) ;

  /* highlight the name and scroll to it if necessary. */
  path = gtk_tree_model_get_path(sort_model, &iter);

  if (path)
    {
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(featureList),
			       path,
			       NULL,
			       FALSE);
      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(featureList),
				   path,
				   NULL,
				   TRUE,
				   0.3,  
				   0.0); 
      gtk_tree_path_free(path);
    }

  gtk_widget_show_all(window);

  return;
}





/* Called by zmapControlSplit.c when a split window is closed */
void zMapWindowDestroyLists(ZMapWindow window)
{
  int i;
  gint ret_val = 0;
  gpointer data;

  for (i = 0; i < window->featureListWindows->len; i++)
    {
      data = g_ptr_array_index(window->featureListWindows, i);
      gtk_signal_emit_by_name(GTK_OBJECT(data), "destroy", NULL, &ret_val);
    }
  g_ptr_array_free(window->featureListWindows, TRUE);

  return;
}



/***************** Internal functions ************************************/
/** \Brief Load an item into the list.
 *
 * Loads the list, ensuring separation of forward and reverse strand features.
 * The x coordinate is in canvasData, but is only guaranteed while no other
 * column has its own list, so we put it into the list for this column now.
 */
static void addItemToList(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ListCol     listCol = (ListColStruct*)user_data;
  GtkTreeIter iter1;



  if (listCol->strand == feature->strand)
    {
      FooCanvasItem *item ;
      char strand[8];

      item = zmapWindowFToIFindItem(listCol->window->feature_to_item,
				    feature->style, feature->unique_id) ;
      zMapAssert(item) ;


      gtk_tree_store_append(GTK_TREE_STORE(listCol->list), &iter1, NULL);
      gtk_tree_store_set(GTK_TREE_STORE(listCol->list), &iter1, 
			 NAME, (char *)g_quark_to_string(feature->original_id),
			 STRAND, zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
			 START, feature->x1,
			 END, feature->x2,
			 ID, key_id,
			 TYPE, g_quark_to_string(feature->style),
			 FEATURE_TYPE, zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
			 X_COORD, listCol->x_coord,
			 FEATURE_ITEM, item,
			 -1) ;
    }

  return ;
}



/** \Brief Locates the selected item in the list.
 *
 * When the user clicks an item to see a list of features, 
 * we want to highlight that item in the resulting list.
 */
static GtkTreeIter findItemInList(ZMapWindow zmapWindow, GtkTreeModel *sort_model, FooCanvasItem *item)
{
  GtkTreeIter    iter, match_iter = {NULL};
  gboolean       valid;
  FooCanvasItem *listItem;

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first (sort_model, &iter);

  while (valid)
    {
      gtk_tree_model_get (sort_model, &iter, 
                          FEATURE_ITEM, &listItem,
                          -1);

      if (item == listItem)
	match_iter = iter;

      valid = gtk_tree_model_iter_next (sort_model, &iter);
    }

  g_free(listItem);

  return match_iter;
}




/** \Brief Extracts the selected feature from the list.
 *
 * Once the selected feature has been determined, calls 
 * a function to scroll to and highlight it.
 */
static int tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  ZMapWindow window = (ZMapWindowStruct*)data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  int start = 0, end = 0;
  double x_coord;
  gchar *name, *type, *feature_type;
  GQuark id;
  FooCanvasItem *item ;

  /* retrieve user's selection */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {

      gtk_tree_model_get (model, &iter, NAME, &name, START, &start, END, &end, 
			  ID, &id, TYPE, &type, FEATURE_TYPE, &feature_type, 
			  X_COORD, &x_coord, FEATURE_ITEM, &item, -1) ;
    }

  zMapWindowScrollToItem(window, item) ;
    
  return TRUE ;
}


/** \Brief Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure. 
 */
static void quitListCB(GtkWidget *widget, gpointer data)
{
  ZMapWindow zmapWindow;
  GtkWidget *window = (GtkWidget*)data;

  zmapWindow = g_object_get_data(G_OBJECT(window), "zmapWindow");  
  g_ptr_array_remove(zmapWindow->featureListWindows, (gpointer)window);
  gtk_widget_destroy(GTK_WIDGET(window));

  return;
}


/*************************** end of file *********************************/
