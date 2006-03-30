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
 * Last edited: Mar 30 16:19 2006 (edgrif)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.45 2006-03-30 15:24:23 edgrif Exp $
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
  ZMapWindow  zmapWindow ; /*!< pointer to the zmapWindow that created us.   */
  char          *title ;  /*!< Title for the window  */

  GtkWidget     *view  ;  /*!< The treeView so we can get store, selection ... */
  GtkWidget     *toplevel ;

  GtkTreeModel *treeModel ;

  int cb_action ;					    /* transient: filled in for callback. */
} ZMapWindowListStruct, *ZMapWindowList ;



/* For the menu */
enum {
  WINLISTGFF,
  WINLISTXFF,
  WINLISTUNK,

  WINLIST_RESULTS_SHOW,
  WINLIST_RESULTS_HIDE,
  WINLIST_SELECTION_SHOW,
  WINLIST_SELECTION_HIDE,

  WINLISTHELP,
  WINLISTABOUT,
  /* Space to add more */
  WINLIST_NONE
};



/* creator functions ... */
static void drawListWindow(ZMapWindowList windowList, GtkTreeModel *treeModel,
			   FooCanvasItem *current_item);
static GtkWidget *makeMenuBar(ZMapWindowList wlist);

static void selectItemInView(ZMapWindow window, GtkTreeView *treeView, FooCanvasItem *item);

/* callbacks for the gui... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void testButtonCB      (GtkWidget *widget, gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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

static gboolean operateTreeModelForeachFunc(GtkTreeModel *model,
                                            GtkTreePath *path,
                                            GtkTreeIter *iter,
                                            gpointer data);
static void operateSelectionForeachFunc(GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data);

/* callbacks for the menu */
static void quitMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void exportCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void orderByCB (gpointer data, guint cb_action, GtkWidget *widget);
static void searchCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void operateCB (gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);




/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] = {
 /* File */
 { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Search",        NULL,         searchCB,   0,          NULL,       NULL},
 { "/File/Export",        NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Export/GFF ->", NULL,         exportCB,   WINLISTGFF, NULL,       NULL},
 { "/File/Export/XFF ->", NULL,         exportCB,   WINLISTXFF, NULL,       NULL},
 { "/File/Export/... ->", NULL,         exportCB,   WINLISTUNK, NULL,       NULL},
 { "/File/Close",         "<control>W", quitMenuCB, 0,          NULL,       NULL},
 /* View */
 { "/_View",                 NULL, NULL,      0,                                 "<Branch>",    NULL},
 { "/View/Order By",         NULL, NULL,      0,                                 "<Branch>",    NULL},
 { "/View/Order By/Name",    NULL, orderByCB, ZMAP_WINDOW_LIST_COL_NAME,         NULL,          NULL},
 { "/View/Order By/Type",    NULL, orderByCB, ZMAP_WINDOW_LIST_COL_TYPE,         NULL,          NULL},
 { "/View/Order By/Start",   NULL, orderByCB, ZMAP_WINDOW_LIST_COL_START,        NULL,          NULL},
 { "/View/Order By/End",     NULL, orderByCB, ZMAP_WINDOW_LIST_COL_END,          NULL,          NULL},
 { "/View/Order By/Strand",  NULL, orderByCB, ZMAP_WINDOW_LIST_COL_STRAND,       NULL,          NULL},
 { "/View/Order By/Phase",   NULL, orderByCB, ZMAP_WINDOW_LIST_COL_PHASE,        NULL,          NULL},
 { "/View/Order By/Score",   NULL, orderByCB, ZMAP_WINDOW_LIST_COL_SCORE,        NULL,          NULL},
 { "/View/Order By/Method",  NULL, orderByCB, ZMAP_WINDOW_LIST_COL_FEATURE_TYPE, NULL,          NULL},
 { "/View/Order By/1------", NULL, NULL,      0,                                 "<Separator>", NULL},
 { "/View/Order By/Reverse", NULL, orderByCB, ZMAP_WINDOW_LIST_COL_NUMBER,       NULL,          NULL},
 /* Operate */
 { "/_Operate",                  NULL, NULL,      0,                      "<Branch>",                   NULL},
 { "/Operate/On Results",        NULL, NULL,      0,                      "<Branch>",                   NULL},
 { "/Operate/On Results/Show",   NULL, operateCB, WINLIST_RESULTS_SHOW,   "<RadioItem>",                NULL},
 { "/Operate/On Results/Hide",   NULL, operateCB, WINLIST_RESULTS_HIDE,   "/Operate/On Results/Show",   NULL},
 { "/Operate/On Selection",      NULL, NULL,      0,                      "<Branch>",                   NULL},
 { "/Operate/On Selection/Show", NULL, operateCB, WINLIST_SELECTION_SHOW, "<RadioItem>",                NULL},
 { "/Operate/On Selection/Hide", NULL, operateCB, WINLIST_SELECTION_HIDE, "/Operate/On Selection/Show", NULL},
 /* Help */
 { "/_Help",             NULL, NULL,       0,            "<LastBranch>", NULL},
 { "/Help/Feature List", NULL, helpMenuCB, WINLISTHELP,  NULL,           NULL},
 { "/Help/1-----------", NULL, NULL,       0,            "<Separator>",  NULL},
 { "/Help/About",        NULL, helpMenuCB, WINLISTABOUT, NULL,           NULL}
};



/* CODE ------------------------------------------------------------------ */

/* Displays a list of selectable features
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 * 
 */
void zmapWindowListWindowCreate(ZMapWindow zmapWindow, 
				GList *itemList,
				char *title,
				FooCanvasItem *current_item)
{
  ZMapWindowList window_list = NULL;

  window_list = g_new0(ZMapWindowListStruct, 1) ;

  window_list->zmapWindow = zmapWindow ;
  window_list->title      = title;

  window_list->treeModel = zmapWindowFeatureListCreateStore(FALSE) ;

  zmapWindowFeatureListPopulateStoreList(window_list->treeModel, itemList) ;

  drawListWindow(window_list, window_list->treeModel, current_item) ;

  /* The view now holds a reference so we can get rid of our own reference */
  g_object_unref(G_OBJECT(window_list->treeModel)) ;

  return ;
}


/* Reread its feature data from the feature structs, important when user has done a revcomp. */
void zmapWindowListWindowReread(GtkWidget *toplevel)
{  
  ZMapWindowList window_list ;

  window_list = g_object_get_data(G_OBJECT(toplevel), ZMAP_WINDOW_LIST_OBJ_KEY) ;

  zmapWindowFeatureListRereadStoreList(GTK_TREE_VIEW(window_list->view), window_list->zmapWindow) ;

  return ;
}



/***************** Internal functions ************************************/

static void drawListWindow(ZMapWindowList windowList, GtkTreeModel *treeModel,
			   FooCanvasItem *current_item)
{
  GtkWidget *window, *vBox, *subFrame, *scrolledWindow;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GtkWidget *button, *buttonBox;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zmapWindowFeatureListCallbacksStruct windowCallbacks = { NULL, NULL, NULL };
  char *frame_label = NULL;

  /* Create window top level */
  windowList->toplevel = window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

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
                   GTK_SIGNAL_FUNC(destroyListWindowCB), windowList);

  /* Start drawing things in it. */
  vBox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vBox);

  gtk_box_pack_start(GTK_BOX(vBox), makeMenuBar(windowList), FALSE, FALSE, 0);

  frame_label = g_strdup_printf("Feature set %s", windowList->title);
  subFrame = gtk_frame_new(NULL);
  if(frame_label)
    g_free(frame_label);

  gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start(GTK_BOX(vBox), subFrame, TRUE, TRUE, 0);

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(scrolledWindow));

  /* Get our treeView */
  windowCallbacks.columnClickedCB = NULL; /* G_CALLBACK(columnClickedCB); */
  windowCallbacks.rowActivatedCB  = G_CALLBACK(view_RowActivatedCB);
  windowCallbacks.selectionFuncCB = selectionFuncCB;
  windowList->view = zmapWindowFeatureListCreateView(treeModel, NULL, &windowCallbacks, windowList);

  selectItemInView(windowList->zmapWindow, GTK_TREE_VIEW(windowList->view), current_item) ;

  gtk_container_add(GTK_CONTAINER(scrolledWindow), windowList->view) ;


  /* Testing only... */

  /* Our Button(s) */
  subFrame = gtk_frame_new("");
  //  gtk_frame_set_shadow_type(GTK_FRAME(subFrame), GTK_SHADOW_IN);
  gtk_box_pack_start(GTK_BOX(vBox), subFrame, FALSE, FALSE, 0);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  buttonBox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(buttonBox), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  button = gtk_button_new_with_label("Reread");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     GTK_SIGNAL_FUNC(testButtonCB), (gpointer)(windowList));
  GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT); 
  
  gtk_container_add(GTK_CONTAINER(buttonBox), button) ;
  gtk_container_add(GTK_CONTAINER(subFrame), buttonBox) ;      
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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



/* make the menu from the global defined above !
 */
GtkWidget *makeMenuBar(ZMapWindowList wlist)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)wlist) ;

  gtk_window_add_accel_group(GTK_WINDOW(wlist->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}


/* GUI Callbacks -------------------------------------------------- */

/* Extracts the selected feature from the list.
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
  gint rows_selected = 0;
  GtkTreeIter iter;
  
  if(((rows_selected = zmapWindowFeatureListCountSelected(selection)) < 1) 
     && gtk_tree_model_get_iter(model, &iter, path))
    {
      GtkTreeView *treeView = NULL;
      ZMapFeature feature = NULL ;
      FooCanvasItem *item ;

      treeView = gtk_tree_selection_get_tree_view(selection);
      
      gtk_tree_model_get(model, &iter, 
                         ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
                         -1) ;
      zMapAssert(feature) ;

      item = zmapWindowFToIFindFeatureItem(windowList->zmapWindow->context_to_item, feature) ;
      zMapAssert(item) ;

      if(!path_currently_selected && item)
        {
          gtk_tree_view_scroll_to_cell(treeView, path, NULL, FALSE, 0.0, 0.0);
          zMapWindowScrollToItem(windowList->zmapWindow, item);
        }
    }
  
  return TRUE ;
}



/* handles user clicks on the column title widgets
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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Ends up calling destroyListWindowCB via g_signal "destroy"
 * Just for the close button.
 */
static void testButtonCB(GtkWidget *widget, gpointer user_data)
{
  ZMapWindowList wList = (ZMapWindowList)user_data;

  zmapWindowFeatureListRereadStoreList(GTK_TREE_VIEW(wList->view), wList->zmapWindow) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure. 
 */
static void destroyListWindowCB(GtkWidget *widget, gpointer user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data;

  if(windowList != NULL)
    {
      ZMapWindow zmapWindow = NULL;

      zmapWindow = windowList->zmapWindow;

      if(zmapWindow != NULL)
        g_ptr_array_remove(zmapWindow->featureListWindows, (gpointer)windowList->toplevel) ;
    }

  gtk_widget_destroy(GTK_WIDGET(windowList->toplevel));

  return;
}



/* handles the row double click event
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
      ZMapWindowList winList = (ZMapWindowList)userdata ;
      ZMapFeature feature = NULL ;
      FooCanvasItem *item ;

      gtk_tree_model_get(model, &iter, 
                         ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
                         -1) ;
      zMapAssert(feature) ;

      item = zmapWindowFToIFindFeatureItem(winList->zmapWindow->context_to_item, feature) ;
      zMapAssert(item) ;

      zmapWindowEditorCreate(winList->zmapWindow, item) ;
    }
  return ;
}


static void operateSelectionForeachFunc(GtkTreeModel *model,
                                        GtkTreePath  *path,
                                        GtkTreeIter  *iter,
                                        gpointer      data)
{
  /* we ignore the return as we can't duck out of a selectionForeach
   * :( */
  operateTreeModelForeachFunc(model, path, 
                              iter, data) ;
  return ;
}


static gboolean operateTreeModelForeachFunc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter,
                                            gpointer data)
{
  ZMapWindowList cb_data = (ZMapWindowList)data ;
  gboolean stop = FALSE ;
  ZMapFeature feature = NULL ;
  FooCanvasItem *listItem = NULL;
  gint action = 0;

  action = cb_data->cb_action ;

  gtk_tree_model_get(model, iter, 
		     ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
		     -1) ;
  zMapAssert(feature) ;

  listItem = zmapWindowFToIFindFeatureItem(cb_data->zmapWindow->context_to_item, feature) ;
  zMapAssert(listItem) ;


  if (listItem)
    {
      switch(action)
	{
	case WINLIST_SELECTION_HIDE:
	case WINLIST_RESULTS_HIDE:
	  foo_canvas_item_hide(listItem);
	  break;

	case WINLIST_SELECTION_SHOW:
	case WINLIST_RESULTS_SHOW:
	  foo_canvas_item_show(listItem);
	  break;

	default:
	  break;
	}
    }

  return stop ;
}


/* Menu Callbacks ---------------------------------------- */

/** \Brief Ends up calling destroyListWindowCB via g_signal "destroy"
 * Just for the close button.
 */
static void quitMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList = (ZMapWindowList)data;

  g_signal_emit_by_name(GTK_WIDGET(wList->toplevel), "destroy", NULL, NULL);

  return ;
}


/** \Brief Will allow user to export the list as a file of specified type 
 * Err nothing happens yet.
 */
static void exportCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList = (ZMapWindowList)data;
  GtkWidget *window    = NULL;
  window = wList->toplevel;
  switch(cb_action)
    {
    case WINLISTUNK:
    case WINLISTXFF:
    case WINLISTGFF:
    default:
      printf("No way to do this yet.\n");
      break;
    }
  return ;
}

/** \Brief Another way to order the list.  
 * Menu select sorts on that column ascending, except "Reverse" 
 * which reverses the current sort.
 */
static void orderByCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList    = (ZMapWindowList)data;
  GtkTreeView *treeView   = NULL;
  GtkTreeModel *treeModel = NULL;
  GtkTreeViewColumn *col  = NULL;
  GtkTreePath *path       = NULL;
  int sortable_id = 0, column_id = 0;
  GtkSortType order = 0, neworder;

  treeView  = GTK_TREE_VIEW(wList->view);
  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(treeView), &path, &col);  
  if(path)
    gtk_tree_path_free(path);
  if(!col)
    return ;
  column_id = gtk_tree_view_column_get_sort_column_id(col);
  gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(treeModel), 
                                       &sortable_id, &order);

  neworder = (order == GTK_SORT_ASCENDING) ? GTK_SORT_DESCENDING : GTK_SORT_ASCENDING;

  switch(cb_action)
    {
    case ZMAP_WINDOW_LIST_COL_NAME:
    case ZMAP_WINDOW_LIST_COL_TYPE:
    case ZMAP_WINDOW_LIST_COL_START:
    case ZMAP_WINDOW_LIST_COL_END:
    case ZMAP_WINDOW_LIST_COL_STRAND:
    case ZMAP_WINDOW_LIST_COL_PHASE:
    case ZMAP_WINDOW_LIST_COL_SCORE:
    case ZMAP_WINDOW_LIST_COL_FEATURE_TYPE:
      gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treeModel), 
                                           cb_action, 
                                           GTK_SORT_ASCENDING);
      break;
    case ZMAP_WINDOW_LIST_COL_NUMBER:
      gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treeModel), 
                                           sortable_id, 
                                           neworder);
      break;
    default:
      break;
    }
  return ;
}

/** \Brief Will allow user get some help
 * Err nothing happens yet.
 */
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList = (ZMapWindowList)data;
  GtkWidget *window    = NULL;
  window = wList->toplevel;
  switch(cb_action)
    {
    case WINLISTHELP:
    case WINLISTABOUT:
    default:
      printf("No help yet.\n");
      break;
    }
  return ;
}

/** \Brief Will allow user to open the search window
 * Either uses the currently selected row as a feature input to the
 * searchWindow or the first item in the list.  The searchWindow
 * requires a ZMapFeatureAny as input so this is what we give it.
 *
 */
static void searchCB  (gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList = (ZMapWindowList)data;
  GtkTreeView *treeView   = NULL;
  GtkTreeModel *treeModel = NULL;
  GtkTreeViewColumn *col  = NULL;
  GtkTreePath *path       = NULL;
  ZMapFeatureAny feature  = NULL;
  GtkTreeIter iter;

  treeView  = GTK_TREE_VIEW(wList->view);
  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(treeView), &path, &col);  
  if(!path)
    {
      if(!(gtk_tree_model_get_iter_first(treeModel, &iter)))
         return ;               /* We can't continue if the list is empty */
    }
  else
    {
      gtk_tree_model_get_iter(GTK_TREE_MODEL(treeModel), &iter, path);
      gtk_tree_path_free(path);
    }

  gtk_tree_model_get(treeModel, &iter, 
		     ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
		     -1);
  zMapAssert(feature) ;

  zmapWindowCreateSearchWindow(wList->zmapWindow, feature);

  return ;
}


static void operateCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList    = (ZMapWindowList)data ;
  GtkTreeView *treeView   = NULL ;
  GtkTreeModel *treeModel = NULL ;

  treeView  = GTK_TREE_VIEW(wList->view);
  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));


  wList->cb_action = cb_action ;

  /* take advantage of the model and selection foreach
   * functions. Although they do not have the same prototypes 
   * they take the same arguments allowing one to call the 
   * other...The user data will likely need to be changed if 
   * we do anything more complicated in them though.
   */
  switch(cb_action)
    {
    case WINLIST_SELECTION_SHOW:
    case WINLIST_SELECTION_HIDE:
      {
        GtkTreeSelection *selection = NULL;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW (treeView));
        gtk_tree_selection_selected_foreach(selection, 
                                            operateSelectionForeachFunc, 
                                            wList) ;
      }
      break;
    case WINLIST_RESULTS_SHOW:
    case WINLIST_RESULTS_HIDE:
      gtk_tree_model_foreach(treeModel, 
                             operateTreeModelForeachFunc,
                             wList) ;
      break;
    default:
      break;
    }

  return ;
}

/** \Brief Select/Highlight the row with the item 
 * This has the side effect of zooming to the item on the canvas (selectFuncCB). 
 * While this isn't necessarily a good/bad thing it does take time.  
 * We can't really attach the callback after doing this, which would stop this,
 * as it's done in the createView function.  Merits of this, is it a feature??
 */
static void selectItemInView(ZMapWindow window, GtkTreeView *treeView, FooCanvasItem *item)
{
  GtkTreeModel *treeModel = NULL;
  GtkTreeSelection *selection = NULL;
  GtkTreeIter iter;
  gboolean valid;

  treeModel = gtk_tree_view_get_model(GTK_TREE_VIEW(treeView));
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first(treeModel, &iter) ;

  while (valid)
    {
      ZMapFeature feature = NULL ;
      FooCanvasItem *listItem ;

      gtk_tree_model_get(treeModel, &iter, 
			 ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
			 -1);

      listItem = zmapWindowFToIFindFeatureItem(window->context_to_item, feature) ;
      zMapAssert(listItem) ;

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
