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
 * Last edited: Sep 29 10:59 2005 (rds)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.39 2005-09-29 13:23:06 rds Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

#define ZMAP_WINDOW_LIST_OBJ_KEY "ZMapWindowList"

/* ZMapWindowListStruct structure declaration
 *
 */
typedef struct _ZMapWindowListStruct
{
  ZMapWindow  zmapWindow; /*!< pointer to the zmapWindow that created us. We shouldn't depend on this....  */
  FooCanvasItem *item  ;  /*!< hold ptr to the original item. DO NOT FREE!  */
  ZMapStrand    strand ;  /*!< up or down strand (enum). */
  char          *title ;  /*!< Title for the window  */
  GtkWidget     *view  ;  /*!< The treeView so we can get store, selection ... */
} ZMapWindowListStruct, *ZMapWindowList ; /*!< pointer to ZMapWindowListStruct. */


/* function prototypes ***************************************************/
/* Functions to move into zmapWindow_P.h */


/* PRIVATE FUNCTIONS TO THIS IMPLEMENTATION */

/* creator functions ... */
static void populateStore        (ZMapWindowList windowList, GtkTreeModel *treeModel);
static void drawListWindow       (ZMapWindowList windowList, GtkTreeModel *treeModel);

static void selectItemInView(GtkTreeView *treeView, FooCanvasItem *item);

/* callbacks for the gui... */
static void closeButtonCB      (GtkWidget *widget, gpointer user_data);
static void destroyListWindowCB(GtkWidget *window, gpointer user_data);

static void columnClickedCB    (GtkTreeViewColumn *col, gpointer user_data);
static void view_RowActivatedCB(GtkTreeView *treeView,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata);
static gboolean selectionFuncCB(GtkTreeSelection *selection, 
                                GtkTreeModel     *model,
                                GtkTreePath      *path, 
                                gboolean          path_currently_selected,
                                gpointer          user_data);

/* functions *************************************************************/




/* Displays a list of selectable features
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 * 
 */
void zmapWindowListWindowCreate(ZMapWindow zmapWindow, 
                                FooCanvasItem *item, 
                                ZMapStrand strandMask)
{
  ZMapWindowList windowList = NULL;
  GtkTreeModel *treeModel   = NULL;

  windowList = g_new0(ZMapWindowListStruct, 1) ;

  windowList->zmapWindow = zmapWindow ;
  windowList->item       = item;
  if(strandMask)
    windowList->strand   = strandMask;

  treeModel = zmapWindowFeatureListCreateStore();

  populateStore(windowList, treeModel);

  drawListWindow(windowList, treeModel);

  /* finished with list now. Are we sure? */
#ifdef ISSUE111111111111111
  g_object_unref(G_OBJECT(winList->store));
  g_free(winList) ;
#endif

  return;
}


/***************** Internal functions ************************************/


/* Just a roundabout function to get the feature sets. Important bit
 * is the g_datalist_foreach @ the bottom */
static void populateStore(ZMapWindowList windowList, GtkTreeModel *treeModel)
{
  FooCanvasItem *item        = NULL;
  FooCanvasItem *parent_item = NULL;
  ZMapFeature feature        = NULL;
  ZMapFeatureSet feature_set = NULL;
  GData *feature_sets        = NULL;
  ZMapWindowItemFeatureType item_feature_type;

  item      = windowList->item;

  /* The foocanvas item we get passed may represent a feature or the column itself depending
   * where the user clicked. */
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  switch(item_feature_type)
    {
    case ITEM_SET:
      {
        feature_set     = g_object_get_data(G_OBJECT(item), "item_feature_data") ;
        feature_sets    = feature_set->features ;
        if(!windowList->strand)
          windowList->strand = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
                                                                 "item_feature_strand")) ;
      }
      break;
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
    case ITEM_FEATURE_CHILD:
      {
        /* Get hold of the feature corresponding to this item. */
        feature = g_object_get_data(G_OBJECT(item), "item_feature_data");  
        zMapAssert(feature);
        
        /* The item the user clicked on may have been a subpart of a feature, e.g. transcript, so
         * we need to find the parent item for highlighting etc. */
        parent_item = zmapWindowFToIFindFeatureItem(windowList->zmapWindow->context_to_item, feature) ;
        zMapAssert(parent_item) ;

        windowList->item = parent_item;
        feature_sets = feature->parent_set->features ;
#ifdef COPLETELY_UNUSED        
        double x1, y1, x2, y2;
        foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);	    /* world coords */
        /* I have no idea why this x coord is kept.... */
        windowList->x_coord = x1;
#endif
        /* need to know whether the column is up or down strand, 
         * so we load the right set of features */
        if(!windowList->strand)
          windowList->strand = feature->strand ;
        feature_set        = zMapFeatureGetSet(feature) ;
      }
      break;
    default:
      windowList->title = "Error";
      printf("Error here!\n");
      break;
    }

  /* Build and populate the list of features */
  if(feature_sets)
    {
      /* This needs setting here so we can draw it later. */
      windowList->title = zMapFeatureSetGetName(feature_set) ;
      zmapWindowFeatureListPopulateStoreDataList(treeModel, windowList->zmapWindow, windowList->strand, feature_sets);
    }

  return ;
}

static void drawListWindow(ZMapWindowList windowList, GtkTreeModel *treeModel)
{
  GtkWidget *window, *vBox, *subFrame, *scrolledWindow;
  GtkWidget *button, *buttonBox;
  zmapWindowFeatureListCallbacksStruct windowCallbacks = { NULL, NULL, NULL };
  char *frame_label = NULL;

  /* Create window top level */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Set it up graphically nice */
  gtk_window_set_title(GTK_WINDOW(window), windowList->title) ;
  gtk_window_set_default_size(GTK_WINDOW(window), -1, 600); 
  gtk_container_border_width(GTK_CONTAINER(window), 5) ;

  /* Add ptrs so parent knows about us, and we know parent */
  g_ptr_array_add(windowList->zmapWindow->featureListWindows, (gpointer)window);
  g_object_set_data(G_OBJECT(window), 
                    ZMAP_WINDOW_LIST_OBJ_KEY, 
                    (gpointer)windowList);

  /* And a destroy function */
  g_signal_connect(GTK_OBJECT(window), "destroy",
                   GTK_SIGNAL_FUNC(destroyListWindowCB), window);

  /* Start drawing things in it. */
  vBox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vBox);

  frame_label = g_strdup_printf("Feature set %s", windowList->title);
  subFrame = gtk_frame_new(frame_label);
  if(frame_label)
    g_free(frame_label);

  gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vBox), subFrame, TRUE, TRUE, 0);

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(scrolledWindow));

  /* Get our treeView */
  windowCallbacks.columnClickedCB = G_CALLBACK(columnClickedCB);
  windowCallbacks.rowActivatedCB  = G_CALLBACK(view_RowActivatedCB);
  windowCallbacks.selectionFuncCB = selectionFuncCB;
  windowList->view = zmapWindowFeatureListCreateView(treeModel, &windowCallbacks, windowList);

  selectItemInView(GTK_TREE_VIEW(windowList->view), windowList->item);

  gtk_container_add(GTK_CONTAINER(scrolledWindow), windowList->view);

  /* Our Button(s) */
  subFrame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(vBox), subFrame, FALSE, FALSE, 0);

  buttonBox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(buttonBox), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  button = gtk_button_new_with_label("Close");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(closeButtonCB), (gpointer)(window));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT); 
  
  gtk_container_add(GTK_CONTAINER(buttonBox), button) ;
  gtk_container_add(GTK_CONTAINER(subFrame), buttonBox) ;      

  /* sort the list on the start coordinate
   * We do this here so adding to the list isn't slowed, although the
   * list should be reasonably sorted anyway */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treeModel), 
                                       ZMAP_WINDOW_LIST_COL_START, 
                                       GTK_SORT_ASCENDING);

  /* Now show everything. */
  gtk_widget_show_all(window) ;

  return ;
}

/** \Brief Extracts the selected feature from the list.
 *
 * Once the selected feature has been determined, calls 
 * a function to scroll to and highlight it.
 */
static gboolean selectionFuncCB(GtkTreeSelection *selection, 
                                GtkTreeModel     *model,
                                GtkTreePath      *path, 
                                gboolean          path_currently_selected,
                                gpointer          user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data;
  GtkTreeIter iter;

  if(gtk_tree_model_get_iter(model, &iter, path))
    {
      GtkTreeView *treeView = NULL;
      FooCanvasItem *item ;
      treeView = gtk_tree_selection_get_tree_view(selection);
      
      gtk_tree_model_get(model, &iter, 
                         ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, &item,
                         -1);
      if(!path_currently_selected && item)
        {
          gtk_tree_view_scroll_to_cell(treeView, path, NULL, FALSE, 0.0, 0.0);
          zMapWindowScrollToItem(windowList->zmapWindow, item);
        }
    }

  return TRUE ;
}


/** \Brief handles user clicks on the column title widgets
 * Makes the column sort by this column or reverses the order if 
 * already sorted by this column.
 */
static void columnClickedCB(GtkTreeViewColumn *col, gpointer user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data;
  GtkTreeModel *model       = NULL;
  int sortable_id = 0, column_id = 0;
  GtkSortType order = 0, neworder;

  model     = gtk_tree_view_get_model(GTK_TREE_VIEW(windowList->view));
  column_id = gtk_tree_view_column_get_sort_column_id(col);
  gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &sortable_id, &order);

  neworder = (order == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;
  /* The sort indicator looks wrong to me, points up on descending numbers down the list??? */
  /*  gtk_tree_view_column_set_sort_order (col, order);  doesn't alter anything*/
  /*  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), sortable_id, order); breaks sorting,  */
  printf("Well on the way to sorting columns %d, %d by order %d, neworder %d\n", column_id, sortable_id, order, neworder);

  return ;
}

/** \Brief Ends up calling destroyListWindowCB via g_signal "destroy"
 * Just for the close button.
 */
static void closeButtonCB(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window = (GtkWidget *)user_data;
  g_signal_emit_by_name(window, "destroy", NULL, NULL);
  return ;
}

/** \Brief Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure. 
 */
static void destroyListWindowCB(GtkWidget *widget, gpointer user_data)
{
  GtkWidget *window = (GtkWidget*)user_data;
  ZMapWindowList windowList = NULL;

  windowList = g_object_get_data(G_OBJECT(window), ZMAP_WINDOW_LIST_OBJ_KEY);

  if(windowList != NULL)
    {
      ZMapWindow zmapWindow = NULL;
      zmapWindow = windowList->zmapWindow;
      if(zmapWindow != NULL)
        g_ptr_array_remove(zmapWindow->featureListWindows, (gpointer)window);
    }

  gtk_widget_destroy(GTK_WIDGET(window));

  return;
}
/** \Brief handles the row double click event
 * When a user double clicks on a row of the feature list 
 * the feature editor window pops up. This sorts that out.
 */
static void view_RowActivatedCB(GtkTreeView       *treeView,
                                GtkTreePath       *path,
                                GtkTreeViewColumn *col,
                                gpointer          userdata)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model(treeView);

  if (gtk_tree_model_get_iter(model, &iter, path))
    {
      ZMapWindowList winList = (ZMapWindowList)userdata;
      FooCanvasItem *item    = NULL;

      gtk_tree_model_get(model, &iter, 
                         ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, &item,
                         -1);
      if(item)
        zmapWindowEditorCreate(winList->zmapWindow, item);
    }
  return ;
}

static void selectItemInView(GtkTreeView *treeView, FooCanvasItem *item)
{
  GtkTreeModel *treeModel = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeIter    iter;

  gboolean       valid;

  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first(treeModel, &iter) ;

  while (valid)
    {
      FooCanvasItem *listItem = NULL;
      gtk_tree_model_get(treeModel, &iter, 
			 ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, &listItem,
			 -1) ;

      if (item == listItem)
	{
	  gtk_tree_selection_select_iter(selection, &iter);
          valid = FALSE;
	}
      else
        valid = gtk_tree_model_iter_next(treeModel, &iter);
    }
  return ;
}


/*************************** end of file *********************************/



#ifdef REMOVE_THIS
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
#endif
