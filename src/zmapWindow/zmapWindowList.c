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
 * Last edited: Sep 25 12:38 2005 (rds)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.36 2005-09-25 11:42:09 rds Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

enum { 
  LIST_COL_NAME, 
  LIST_COL_TYPE, 
  LIST_COL_STRAND, 
  LIST_COL_START, 
  LIST_COL_END, 
  LIST_COL_FEATURE_TYPE, 
  LIST_COL_FEATURE_ITEM, 
  LIST_COL_NUMBER
};


/* ZMapWindowListStruct structure declaration
 *
 * Used to pass a couple of variables and a GtkTreeStore list
 * of feature details through the g_datalist_foreach() call 
 * and into the addItemToList() function where the list is loaded.
 */
typedef struct _ZMapWindowListStruct
{
  ZMapWindow  zMpwindow;
  FooCanvasItem *item  ;  /*!< hold ptr to the original item. DO NOT FREE!  */
  ZMapStrand    strand ;  /*!< up or down strand (enum). */
  double        x_coord;  /*!< X coordinate of selected column. */
  gboolean      useTree;
  union{
    GtkTreeStore *tree;  /*!< list of features in selected column. */
    GtkListStore *list;
  }store; 
  char *title;
} ZMapWindowListStruct, *ZMapWindowList ; /*!< pointer to ZMapWindowListStruct. */


/* function prototypes ***************************************************/
static void addItemToList             (GQuark key_id, gpointer data, gpointer user_data);
static GtkTreeIter *findItemInList(ZMapWindow zmapWindow,
				   GtkTreeModel *sort_model, FooCanvasItem *item);
static int  tree_selection_changed_cb(GtkTreeSelection *selection, gpointer data);
static void recalcScrollRegion        (ZMapWindow window, double start, double end);

static void createStore   (ZMapWindowList windowList);
static void populateStore (ZMapWindowList windowList);
static void drawListWindow(ZMapWindowList windowList);
static GtkWidget *createModelView(ZMapWindowList windowList);
static GtkTreeModel *getModel    (ZMapWindowList windowList);

static gint sortByFunc (GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      userdata);

static void columnClickedCB    (GtkTreeViewColumn *col, gpointer user_data);
static void closeButtonCB      (GtkWidget *widget, gpointer user_data);
static void destroyListWindowCB(GtkWidget *window, gpointer user_data);
static void view_RowActivatedCB(GtkTreeView *treeView,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata);

/* functions *************************************************************/


/* Displays a list of selectable features
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 * 
 */
void zMapWindowCreateListWindow(ZMapWindow zmapWindow, FooCanvasItem *item)
{
  ZMapWindowList windowList;

  windowList = g_new0(ZMapWindowListStruct, 1) ;

  windowList->zMpwindow = zmapWindow ;
  windowList->item      = item;
  windowList->useTree   = TRUE;

  /* as this function is a mile long, lets split it out into functions
   * a little.. 
   */
  createStore(windowList);

  populateStore(windowList);

  drawListWindow(windowList);

  /* Isn't that easier. */


  /* finished with list now. Are we sure? */
#ifdef ISSUE111111111111111
  g_object_unref(G_OBJECT(winList->store));
  g_free(winList) ;
#endif

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
  ZMapFeature feature    = (ZMapFeature)data ;
  ZMapWindowList winList = (ZMapWindowList)user_data;

  /* Only do our current strand. As ZMAPSTRAND_FORWARD = 1 &&
   * ZMAPSTRAND_REVERSE = 2 it's conceiveable we could show both and
   * bit and here */
  if (winList->strand == feature->strand)
    {
      FooCanvasItem *item ;
      GtkTreeIter iter1;

      item = zmapWindowFToIFindFeatureItem(winList->zMpwindow->context_to_item, feature) ;
      zMapAssert(item) ;

#ifdef PRINT_LIKE_ITS_TEXT
      printf("%s, %s, %s, %d, %d, %s\n", (char *)g_quark_to_string(feature->original_id),
             zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
             zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
             feature->x1,
             feature->x2,
             zMapStyleGetName(zMapFeatureGetStyle(feature))
             );
#endif
      if(winList->useTree)
        {
          gtk_tree_store_append(GTK_TREE_STORE(winList->store.tree), &iter1, NULL);
          gtk_tree_store_set(GTK_TREE_STORE(winList->store.tree), &iter1, 
                             LIST_COL_NAME,   (char *)g_quark_to_string(feature->original_id),
                             LIST_COL_STRAND, zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
                             LIST_COL_START,  feature->x1,
                             LIST_COL_END,    feature->x2,
                             LIST_COL_TYPE,   zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
                             LIST_COL_FEATURE_TYPE, zMapStyleGetName(zMapFeatureGetStyle(feature)),
                             LIST_COL_FEATURE_ITEM, item,
                             -1) ;
        }
      else
        {
          gtk_list_store_append(GTK_LIST_STORE(winList->store.list), &iter1);
          gtk_list_store_set(GTK_LIST_STORE(winList->store.list), &iter1, 
                             LIST_COL_NAME,   (char *)g_quark_to_string(feature->original_id),
                             LIST_COL_STRAND, zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
                             LIST_COL_START,  feature->x1,
                             LIST_COL_END,    feature->x2,
                             LIST_COL_TYPE,   zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
                             LIST_COL_FEATURE_TYPE, zMapStyleGetName(zMapFeatureGetStyle(feature)),
                             LIST_COL_FEATURE_ITEM, item,
                             -1) ;
        }
    }

  return ;
}



/** \Brief Locates the selected item in the list.
 *
 * When the user clicks an item to see a list of features, 
 * we want to highlight that item in the resulting list.
 */
static GtkTreeIter *findItemInList(ZMapWindow zmapWindow, GtkTreeModel *sort_model, FooCanvasItem *item)
{
  GtkTreeIter    iter, *match_iter = NULL ;
  gboolean       valid;
  FooCanvasItem *listItem ;

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first(sort_model, &iter) ;

  while (valid)
    {
      gtk_tree_model_get(sort_model, &iter, 
			 LIST_COL_FEATURE_ITEM, &listItem,
			 -1) ;

      if (item == listItem)
	{
	  match_iter = g_new0(GtkTreeIter, 1) ;

	  *match_iter = iter ;				    /* n.b. struct copy. */
	  
	  break ;
	}

      valid = gtk_tree_model_iter_next(sort_model, &iter);
    }

  return match_iter ;
}




/** \Brief Extracts the selected feature from the list.
 *
 * Once the selected feature has been determined, calls 
 * a function to scroll to and highlight it.
 */
static int tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
  ZMapWindow window = (ZMapWindow)data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  int start = 0, end = 0;
  gchar *name, *type, *feature_type;
  FooCanvasItem *item ;

  /* retrieve user's selection */
  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {

      gtk_tree_model_get (model, &iter, 
                          LIST_COL_NAME, &name, 
                          LIST_COL_START, &start, 
                          LIST_COL_END, &end, 
                          LIST_COL_TYPE, &type, 
                          LIST_COL_FEATURE_TYPE, &feature_type, 
                          LIST_COL_FEATURE_ITEM, &item, 
                          -1);
    }

  zMapWindowScrollToItem(window, item) ;
    
  return TRUE ;
}

static void createStore(ZMapWindowList windowList)
{
  GtkTreeSortable *sortable = NULL;
  int colNo = 0;
  if(windowList->useTree)
    {
      windowList->store.tree
        = gtk_tree_store_new(LIST_COL_NUMBER,
                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                             G_TYPE_INT,    G_TYPE_INT,    G_TYPE_STRING,
                             G_TYPE_POINTER
                             );
      sortable = GTK_TREE_SORTABLE(windowList->store.tree);
    }
  else
    {
      windowList->store.list
        = gtk_list_store_new(LIST_COL_NUMBER,
                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                             G_TYPE_INT,    G_TYPE_INT,    G_TYPE_STRING,
                             G_TYPE_POINTER
                             );
      sortable = GTK_TREE_SORTABLE(windowList->store.list);
    }

  for(colNo = 0; colNo < LIST_COL_NUMBER; colNo++)
    {  
      gtk_tree_sortable_set_sort_func(sortable, 
                                      colNo, 
                                      sortByFunc, 
                                      GINT_TO_POINTER(colNo), 
                                      NULL);
    }
  /* sort the list on the start coordinate */
  gtk_tree_sortable_set_sort_column_id(sortable, 
                                       LIST_COL_START, 
                                       GTK_SORT_ASCENDING);

  return ;
}

static GtkTreeModel *getModel(ZMapWindowList windowList)
{
  GtkTreeModel *model = NULL;
  if(windowList->useTree)
    model = GTK_TREE_MODEL(windowList->store.tree);
  else
    model = GTK_TREE_MODEL(windowList->store.list);
  return model;
}

static GtkWidget *createModelView(ZMapWindowList windowList)
{
  GtkTreeModel *treeModel;
  GtkWidget    *treeView;
  GtkTreeSelection  *select;
  GtkCellRenderer   *renderer;
  int colNo = 0;
  char *column_titles[LIST_COL_NUMBER] = {"Name", "Type", "Strand", "Start", "End", "Method", "FEATURE_ITEM"};

  /* Get our model */
  treeModel = getModel(windowList);
  /* Create the view from the model */
  treeView  = gtk_tree_view_new_with_model(treeModel);

  /* A renderer for the columns. */
  renderer = gtk_cell_renderer_text_new ();
  /* Add it to all of them, not sure we need to add it to all, just the visible ones... */
  for(colNo = 0; colNo < LIST_COL_NUMBER; colNo++)
    {
      GtkTreeViewColumn *column = NULL;
      column = gtk_tree_view_column_new_with_attributes(column_titles[colNo],
                                                        renderer,
                                                        "text", colNo,
                                                        NULL);
      /* The order of the next two calls IS IMPORTANT.  With the
       * signal_connect FIRST the callback is called BEFORE the
       * sorting happens. A user can then manipulate how the sort is
       * done, but the other way round the sorting is done FIRST and
       * the callback is useless to effect the sort without needlessly
       * sorting again!! This gives me a headache.
       */
      gtk_signal_connect(GTK_OBJECT(column), "clicked",
                         GTK_SIGNAL_FUNC(columnClickedCB), windowList);
      gtk_tree_view_column_set_sort_column_id(column, colNo);
      /* set the pointer and data rows to be invisible */
      if(colNo > LIST_COL_FEATURE_TYPE)
        gtk_tree_view_column_set_visible(column, FALSE);

      /* Add the column to the view now it's all set up */
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
    }

  gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeView), TRUE);

  /* Setup the selection handler */
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeView));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

#ifdef NO_SELECT
  g_signal_connect (G_OBJECT (select), "changed",
		    G_CALLBACK (tree_selection_changed_cb),
		    windowList->zMpwindow);
#endif

  return treeView;
}

static void populateStore(ZMapWindowList windowList)
{
  FooCanvasItem *item        = NULL;
  FooCanvasItem *parent_item = NULL;
  ZMapFeature feature        = NULL;
  ZMapFeatureSet feature_set = NULL;
  GData *feature_sets        = NULL;
  ZMapWindowItemFeatureType item_feature_type;
  double x1, y1, x2, y2;

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
        parent_item = zmapWindowFToIFindFeatureItem(windowList->zMpwindow->context_to_item, feature) ;
        zMapAssert(parent_item) ;
        
        feature_sets = feature->parent_set->features ;
        
        foo_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);	    /* world coords */
        /* I have no idea why this x coord is kept.... */
        windowList->x_coord = x1;

        /* need to know whether the column is up or down strand, 
         * so we load the right set of features */
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
      g_datalist_foreach(&feature_sets, addItemToList, windowList) ;
    }

  /* If the user clicked on an item, find it in our tree list. */
#ifdef ISSUE111111111111111
  if (parent_item)
    {
      GtkTreeIter *iter ;
      GtkTreePath *path ;
      iter = findItemInList(windowList->zMpwindow, windowList->store, parent_item) ;
      zMapAssert(iter) ;

      /* highlight the name and scroll to it if necessary. */
      path = gtk_tree_model_get_path(windowList->store, iter) ;
      zMapAssert(path) ;
      gtk_tree_view_set_cursor(GTK_TREE_VIEW(featureList),
			       path,
			       NULL,
			       FALSE) ;

      gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(featureList),
				   path,
				   NULL,
				   TRUE,
				   0.3,  
				   0.0) ;
      gtk_tree_path_free(path) ;

      g_free(iter) ;
    }
#endif

  return ;
}

static void drawListWindow(ZMapWindowList windowList)
{
  GtkWidget *window, *vBox, *subFrame, *scrolledWindow;
  GtkWidget *button, *buttonBox, *treeView;
  char *frame_label = NULL;
  int spacing = 10;

  /* Create window top level */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  /* Set it up graphically nice */
  gtk_window_set_title(GTK_WINDOW(window), windowList->title) ;
  gtk_window_set_default_size(GTK_WINDOW(window), -1, 600); 
  gtk_container_border_width(GTK_CONTAINER(window), 5) ;
  /* Add ptrs so parent knows about us, and we know parent */
  g_ptr_array_add(windowList->zMpwindow->featureListWindows, (gpointer)window);
  g_object_set_data(G_OBJECT(window), "zmapWindow", windowList->zMpwindow) ;
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
  treeView = createModelView(windowList);
  gtk_container_add(GTK_CONTAINER(scrolledWindow), treeView);
  /* Allow users to edit from this list... Need to do it here so we get teh right userdata passed */
  gtk_signal_connect(GTK_OBJECT(treeView), "row-activated", 
                     GTK_SIGNAL_FUNC(view_RowActivatedCB), window);


  /* Our Button(s) */
  subFrame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_NONE);
  gtk_box_pack_start(GTK_BOX(vBox), subFrame, FALSE, FALSE, 0);

  buttonBox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX (buttonBox), spacing);
  gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 5);

  button = gtk_button_new_with_label("Close");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(closeButtonCB), (gpointer)(window));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT); 
  
  gtk_container_add(GTK_CONTAINER(buttonBox), button) ;
  gtk_container_add(GTK_CONTAINER(subFrame), buttonBox) ;      

  /* Now show everything. */
  gtk_widget_show_all(window) ;

  return ;
}
static void columnClickedCB(GtkTreeViewColumn *col, gpointer user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data;
  GtkTreeModel *model       = NULL;
  int sortable_id = 0, column_id = 0;
  GtkSortType order = 0;

  model = getModel(windowList);
  column_id = gtk_tree_view_column_get_sort_column_id(col);
  gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &sortable_id, &order);

  printf("Well on the way to sorting columns %d, %d by order %d\n", column_id, sortable_id, order);

  return ;
}


static gint sortByFunc (GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      userdata)
{
  gint answer = 0;
  gint col    = GPOINTER_TO_INT(userdata);
  printf("Want to sort by column %d\n", col);
  /* remember to g_free() any strings! */
  switch(col)
    {
    case LIST_COL_END:
      {
        int endA, endB;
        
        gtk_tree_model_get(model, a, col, &endA, -1);
        gtk_tree_model_get(model, b, col, &endB, -1);
        answer = ( (endA == endB) ? 0 : ((endA < endB) ? -1 : 1 ) );
      }
      break;
    case LIST_COL_START:
      {
        int startA, startB;
        
        gtk_tree_model_get(model, a, col, &startA, -1);
        gtk_tree_model_get(model, b, col, &startB, -1);
        answer = ( (startA == startB) ? 0 : ((startA < startB) ? -1 : 1 ) );
      }
      break;
    default:
      printf("Can't sort by that!\n");
      break;
    }
  return answer;
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
  ZMapWindow zmapWindow = NULL;

  zmapWindow = g_object_get_data(G_OBJECT(window), "zmapWindow");  

  if(zmapWindow != NULL)
    g_ptr_array_remove(zmapWindow->featureListWindows, (gpointer)window);

  gtk_widget_destroy(GTK_WIDGET(window));

  return;
}
static void view_RowActivatedCB(GtkTreeView       *treeView,
                                GtkTreePath       *path,
                                GtkTreeViewColumn *col,
                                gpointer          userdata)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;
  g_print ("A row has been double-clicked!\n");

  model = gtk_tree_view_get_model(treeView);

  if (gtk_tree_model_get_iter(model, &iter, path))
    {
      GtkWidget *window     = (GtkWidget *)userdata;
      ZMapWindow zmapWindow = NULL;
      FooCanvasItem *item   = NULL;

      zmapWindow = g_object_get_data(G_OBJECT(window), "zmapWindow");  

      gtk_tree_model_get(model, &iter, 
                         LIST_COL_FEATURE_ITEM, &item,
                         -1);

      //      g_print ("Double-clicked row contains name %s\n", name);
      if(item)
        zmapWindowEditorCreate(zmapWindow, item);
    }
  return ;
}


/*************************** end of file *********************************/
