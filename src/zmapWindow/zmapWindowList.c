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
 * Last edited: Nov 19 13:20 2004 (rnc)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.18 2004-11-19 13:24:27 rnc Exp $
 *-------------------------------------------------------------------
 */


#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

enum { NAME, START, END, ID, TYPE, FEATURE_TYPE, X_COORD, N_COLUMNS };


/** ListColStruct structure declaration
 *
 * Used to pass a couple of variables and a GtkTreeStore list
 * of feature details through the g_datalist_foreach() call 
 * and into the addItemToList() function where the list is loaded.
 */
typedef struct _ListColStruct {
  ZMapStrand    strand;                      /*!< up or down strand (enum). */
  double        x_coord;                     /*!< X coordinate of selected column. */
  GtkTreeStore *list;                        /*!< list of features in selected column. */
  GString      *source;                      /*!< type/source/method */
} ListColStruct,
  *ListCol;                                  /*!< pointer to ListColStruct. */



/* function prototypes ***************************************************/
static void addItemToList             (GQuark key_id, gpointer data, gpointer user_data);
static int  tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data);
static void recalcScrollRegion        (ZMapWindow window, double start, double end);
static void quitListCB                (GtkWidget *window, gpointer data);


/* functions *************************************************************/
/** \Brief Displays a list of selectable features
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item hightlighted.
 */

void zMapWindowCreateListWindow(ZMapWindow zmapWindow, ZMapFeatureItem featureItem)
{
  GtkWidget *window, *featureList, *scrolledWindow;
  GtkTreeModel *sort_model;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;
  ListCol listCol = g_new0(ListColStruct, 1);
  ZMapFeatureTypeStyle thisType;
  double x1, y1, x2, y2;

  foo_canvas_item_get_bounds(featureItem->canvasItem, &x1, &y1, &x2, &y2); /* world coords */

  listCol->list = gtk_tree_store_new(N_COLUMNS,
				     G_TYPE_STRING,
				     G_TYPE_INT,
				     G_TYPE_INT,
				     G_TYPE_INT,
				     G_TYPE_STRING,
				     G_TYPE_INT,
				     G_TYPE_DOUBLE);

  listCol->x_coord = x1;
  listCol->source = g_string_new(featureItem->feature_set->source);
  listCol->source = g_string_ascii_down(listCol->source); /* does this in place */

  thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(zmapWindow->types), 
						       listCol->source->str);
  zMapAssert(thisType);

  /* need to know whether the column is up or down strand, 
   * so we load the right set of features */
  listCol->strand = featureItem->strand;

  /* set up the top level window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  g_ptr_array_add(zmapWindow->featureListWindows, (gpointer)window);

  gtk_container_border_width(GTK_CONTAINER(window), 5) ;
  gtk_window_set_title(GTK_WINDOW(window), featureItem->feature_set->source) ;
  gtk_window_set_default_size(GTK_WINDOW(window), -1, 600); 

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(window), scrolledWindow);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  
  /* Build and populate the list of features */
  g_datalist_foreach(&(featureItem->feature_set->features),addItemToList , listCol) ;

  sort_model = gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL(listCol->list));

  featureList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_model));

  /* sort the list on the start coordinate */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort_model),
				       START,
				       GTK_SORT_ASCENDING);

  /* finished with list now */
  g_object_unref(G_OBJECT(listCol->list));
  g_string_free(listCol->source, FALSE);
  g_free(listCol);

  /* render the columns. */
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

  gtk_tree_view_column_set_visible(column, TRUE);
   
  column = gtk_tree_view_column_new_with_attributes ("ID",
						     renderer,
						     "text", ID,
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
  
  column = gtk_tree_view_column_new_with_attributes ("X Coord",
						     renderer,
						     "text", X_COORD,
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
		   GTK_SIGNAL_FUNC(quitListCB), featureList);

  gtk_container_add(GTK_CONTAINER(scrolledWindow), featureList);

  gtk_widget_show_all(window);

  return;
}


/** \Brief Scroll to the selected item.
 * 
 * If necessary, recalculate the scroll region, then scroll to the item
 * and highlight it.
 */
gboolean zMapWindowScrollToItem(ZMapWindow window, gchar *type, GQuark feature_id) 
{
  int cx, cy;
  double x1, y1, x2, y2;
  ZMapFeatureItem featureItem = NULL;
  ZMapFeature feature;
  ZMapFeatureTypeStyle thisType;
  gboolean result;
  G_CONST_RETURN gchar *quarkString;

  if (!(quarkString = g_quark_to_string(feature_id)))
    {
      zMapLogWarning("Quark %d not a valid quark\n", feature_id) ;
      result = FALSE;
    }
  else
    {
      featureItem = (ZMapFeatureItem)g_datalist_id_get_data(&(window->featureItems), feature_id);
      if (featureItem)
	{
	  feature = (ZMapFeature)g_datalist_id_get_data(&(featureItem->feature_set->features), featureItem->feature_key);
	  zMapAssert(feature);
	  
	  thisType = (ZMapFeatureTypeStyle)g_datalist_get_data(&(window->types), type);
	  zMapAssert(thisType);
	  
	  /* The selected object might be outside the current scroll region. */
	  recalcScrollRegion(window, feature->x1, feature->x2);
	  
	  /* featureItem holds canvasItem and ptr to the feature_set containing the feature. */
	  featureItem =  g_datalist_id_get_data(&(window->featureItems), feature_id);
	  feature =  g_datalist_id_get_data(&(featureItem->feature_set->features), feature_id);
	  
	  /* scroll up or down to user's selection.  
	  ** Note that because we zoom asymmetrically, we only convert the y coord 
	  ** to canvas coordinates, leaving the x as is.  */
	  foo_canvas_item_get_bounds(featureItem->canvasItem, &x1, &y1, &x2, &y2); /* world coords */
	  
	  if (y1 <= 0.0)    /* we might be dealing with a multi-box item, eg transcript */
	    {
	      double px1, py1, px2, py2;
	      
	      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(featureItem->canvasItem->parent),
					 &px1, &py1, &px2, &py2); 
	      if (py1 > 0.0)
		y1 = py1;
	    }
	  
	  foo_canvas_w2c(window->canvas, 0.0, y1, &cx, &cy); 
	  foo_canvas_scroll_to(window->canvas, (int)x1, cy);                       /* canvas pixels */
	  
	  foo_canvas_item_raise_to_top(featureItem->canvasItem);
	  
	  /* highlight the item */
	  zmapWindowHighlightObject(featureItem->canvasItem, window, thisType);
	  
	  zMapFeatureClickCB(window, feature); /* show feature details on info_panel  */
	  
	  window->focusFeature = featureItem->canvasItem;
	  window->focusType = thisType;
	  
	  result =  TRUE;
	}
    }  /* else silently ignore the fact we've not found it. */

  return result;
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
      gtk_tree_store_append(GTK_TREE_STORE(listCol->list), &iter1, NULL);
      gtk_tree_store_set   (GTK_TREE_STORE(listCol->list), &iter1, 
			    NAME , feature->name,
			    START, feature->x1,
			    END  , feature->x2,
			    ID   , key_id,
			    TYPE , listCol->source->str,
			    FEATURE_TYPE, feature->type,
			    X_COORD, listCol->x_coord,
			    -1 );
    }
  return;
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
  int start = 0, end = 0, feature_type = 0;
  double x_coord;
  gchar *name, *type;
  GQuark id;
  gboolean ITEM_FOUND;  

  /* At present, when we display the list, it will scroll the main display
  ** to the first feature on the list.  This is not good but it's not my
  ** top priority right now. */

  /* retrieve user's selection */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, NAME, &name, START, &start, END, &end, 
			  ID, &id, TYPE, &type, FEATURE_TYPE, &feature_type, 
			  X_COORD, &x_coord, -1);
    }

  if (start)
    {
      if ((ITEM_FOUND = zMapWindowScrollToItem(window, type, id)))  /* extra parens to zap 
								     * compiler warning */
	gtk_widget_show_all(GTK_WIDGET(window->parent_widget));
      else
	zMapShowMsg(ZMAP_MSG_CRASH, "Quark %d of type %s not found in list of known features\n", id, type);
    }
  
  if (name) g_free(name); 
  if (type) g_free(type);
    
  return TRUE;
}


/** \Brief Recalculate the scroll region.
 *
 * If the selected feature is outside the current scroll region, recalculate
 * the region to be the same size but with the selecte feature in the middle.
 */
static void recalcScrollRegion(ZMapWindow window, double start, double end)
{
  double x1, y1, x2, y2, height;
  int top, bot;


 foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);  /* world coords */

  if ( start < y1 || end > y2)
    {
      height = y2 - y1;
      y1 = start - (height/2.0);
      if (y1 < window->seq_start)
	y1 = window->seq_start;

      y2 = y1 + height;

      /* this shouldn't happen */
      if (y2 > window->seq_end)
	  y2 = window->seq_end;

      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2);

      /* redraw the scale bar */
      top = (int)y1;                   /* zmapDrawScale expects integer coordinates */
      bot = (int)y2;
      gtk_object_destroy(GTK_OBJECT(window->scaleBarGroup));
      window->scaleBarGroup = zmapDrawScale(window->canvas, 
						window->scaleBarOffset, 
						window->zoom_factor,
						top,
						bot);
    }

  return;
}



/** \Brief Destroy the list window.
 */
static void quitListCB(GtkWidget *window, gpointer data)
{
  gtk_widget_destroy(GTK_WIDGET(window));

  return;
}



