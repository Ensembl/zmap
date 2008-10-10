/*  File: zmapWindowList.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Oct 10 09:18 2008 (rds)
 * Created: Thu Sep 16 10:17 2004 (rnc)
 * CVS info:   $Id: zmapWindowList.c,v 1.67 2008-10-10 08:24:59 rds Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowFeatureList.h>

#define ZMAP_WINDOW_LIST_OBJ_KEY "ZMapWindowList"


/* ZMapWindowListStruct structure declaration
 *
 */
typedef struct _ZMapWindowListStruct
{
  ZMapWindow  zmapWindow ; /*!< pointer to the zmapWindow that created us.   */
  char          *title ;  /*!< Title for the window  */

  GtkWidget     *toplevel ;
  GtkWidget     *scrolledWindow ;
  GtkWidget     *view  ;  /*!< The treeView so we can get store, selection ... */

  GHashTable *context_to_item;

  ZMapWindowRetrieveContextToItemHash hash_retriever;
  gpointer hash_retrieve_data;
  
  GtkTreeModel *tree_model ;

  gboolean zoom_to_item ;
  gboolean reusable ;

  int cb_action ;					    /* transient: filled in for callback. */

  ZMapWindowFeatureItemList zmap_tv;
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
static ZMapWindowList createList(void) ;
static ZMapWindowList listFeature(ZMapWindowList list, ZMapWindow zmapWindow, 
				  ZMapWindowRetrieveContextToItemHash hash_retriever,
				  gpointer retriever_data,
				  GList *itemList,
				  char *title,
				  FooCanvasItem *current_item, gboolean zoom_to_item) ;
static void drawListWindow(ZMapWindowList windowList);
static GtkWidget *makeMenuBar(ZMapWindowList wlist);

static void select_item_in_view(ZMapWindowList window_list, FooCanvasItem *item);


static void row_activated_cb(GtkTreeView *treeView,
			     GtkTreePath        *path,
			     GtkTreeViewColumn  *col,
			     gpointer            userdata);
static gboolean selection_func_cb(GtkTreeSelection *selection, 
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


static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *window, gpointer user_data);
static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget);
static void exportCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void searchCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void operateCB (gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);


static ZMapWindowList findReusableList(GPtrArray *window_list) ;
static gboolean windowIsReusable(void) ;


static GHashTable *access_window_context_to_item(gpointer user_data);





/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] = {
 /* File */
 { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Search",        NULL,         searchCB,   0,          NULL,       NULL},
 { "/File/Preserve",      NULL,         preserveCB, 0,          NULL,       NULL},
 { "/File/Export",        NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Export/GFF ->", NULL,         exportCB,   WINLISTGFF, NULL,       NULL},
 { "/File/Export/XFF ->", NULL,         exportCB,   WINLISTXFF, NULL,       NULL},
 { "/File/Export/... ->", NULL,         exportCB,   WINLISTUNK, NULL,       NULL},
 { "/File/Close",         "<control>W", requestDestroyCB, 0,          NULL,       NULL},
 /* View */
 { "/_View",                 NULL, NULL,      0,                                 "<Branch>",    NULL},
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



/* Create a list display window, this function _always_ creates a new window. */
void zmapWindowListWindowCreate(ZMapWindow zmapWindow,
				ZMapWindowRetrieveContextToItemHash hash_retriever,
				gpointer retriever_data,
				GList *item_list,
				char *title,
				FooCanvasItem *current_item, 
				gboolean zoom_to_item)
{
  ZMapWindowList list = NULL ;

  list = listFeature(NULL, zmapWindow, hash_retriever, retriever_data,
		     item_list, title,
		     current_item, zoom_to_item) ;

  return ;
}



/* Displays a list of selectable features, will reuse an existing window if it can, otherwise
 * it creates a new one.
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 * 
 */
void zmapWindowListWindow(ZMapWindow window, 
			  ZMapWindowRetrieveContextToItemHash hash_retriever,
			  gpointer retriever_data,
			  GList *item_list,
			  char *title,
			  FooCanvasItem *current_item, 
			  gboolean zoom_to_item)
{
  ZMapWindowList list = NULL ;

  /* Look for a reusable window. */
  list = findReusableList(window->featureListWindows) ;

  /* now show the window, if we found a reusable one that will be reused. */
  list = listFeature(list, window, hash_retriever, retriever_data,
		     item_list, title,
		     current_item, zoom_to_item) ;

  zMapGUIRaiseToTop(list->toplevel);

  return ;
}

/* Reread its feature data from the feature structs, important when user has done a revcomp. */
void zmapWindowListWindowReread(GtkWidget *toplevel)
{  
  ZMapWindowList window_list ;
  
  window_list = g_object_get_data(G_OBJECT(toplevel), ZMAP_WINDOW_LIST_OBJ_KEY) ;
  
  window_list->context_to_item = (window_list->hash_retriever)(window_list->hash_retrieve_data);
  
  if(!zMapWindowFeatureItemListUpdateAll(window_list->zmap_tv, 
					 window_list->zmapWindow,
					 window_list->context_to_item))
    {
      zMapLogWarning("%s", "zmapWindowListWindowReread should now remove all and repopulate");
      /* As the warning says we need to remove all and repopulate the list. */

      /* This is more difficult than it might at first seem.  The
       * solution is to have a callback, registered by the original
       * caller, to regenerate the list which can be passed to the
       * list display widget...  */

#ifdef THOUGHTS_ON_WHAT_NEEDS_TO_HAPPEN
      GList *replace_list = NULL;
      RereadFunc callback = NULL;

      /* passing this callback and data needs thought. */
      if((callback = window_list->reread_list))
	{
	  /* This function needs to know how to recreate the list of canvas items */
	  replace_list = (callback)(window_list->reread_user_data);
	  /* remove all needs writing, could even be a zMapGUITreeViewRemoveAll() */
	  zMapWindowFeatureItemListRemoveAll(window_list->zmap_tv);
	  zMapWindowFeatureItemListAddItems(window_list->zmap_tv, 
					    window_list->zmapWindow,
					    replace_list);
	  zMapGUITreeViewAttach(ZMAP_GUITREEVIEW(window_list->zmap_tv));
	}

#endif /* THOUGHTS_ON_WHAT_NEEDS_TO_HAPPEN */
    }

  return ;
}





/*
 *                   Internal functions
 */


static ZMapWindowList createList(void)
{
  ZMapWindowList window_list = NULL;

  window_list = g_new0(ZMapWindowListStruct, 1) ;

  window_list->reusable = windowIsReusable() ;

  return window_list ;
}

static void destroyList(ZMapWindowList windowList)
{
  ZMapWindow zmap ;

  zmap = windowList->zmapWindow ;

  zMapGUITreeViewDestroy(ZMAP_GUITREEVIEW(windowList->zmap_tv));

  g_ptr_array_remove(zmap->featureListWindows, (gpointer)windowList->toplevel) ;
  
  g_free(windowList) ;

  return;
}



static void destroySubList(ZMapWindowList windowList)
{
  gtk_widget_destroy(windowList->view) ;

  return;
}


/* Construct a list window using a new toplevel window if list = NULL and the toplevel window of
 * list if not. */
static ZMapWindowList listFeature(ZMapWindowList list, ZMapWindow zmapWindow, 
				  ZMapWindowRetrieveContextToItemHash hash_retriever,
				  gpointer retriever_data,
				  GList *itemList,
				  char *title,
				  FooCanvasItem *current_item, gboolean zoom_to_item)
{
  ZMapWindowList window_list = NULL;

  if (!list)
    {
      window_list = createList() ;
    }
  else
    {
      window_list = list ;
      zMapGUITreeViewDestroy(ZMAP_GUITREEVIEW(window_list->zmap_tv));
      window_list->zmap_tv = NULL;
      destroySubList(list) ;
    }

  if(!hash_retriever)
    {
      hash_retriever = access_window_context_to_item;
      retriever_data = zmapWindow;
    }

  window_list->zmap_tv = zMapWindowFeatureItemListCreate(ZMAPSTYLE_MODE_INVALID);

  window_list->zmapWindow         = zmapWindow ;
  window_list->title              = title;
  window_list->context_to_item    = (hash_retriever)(retriever_data);
  window_list->hash_retriever     = hash_retriever;
  window_list->hash_retrieve_data = retriever_data;
  window_list->zoom_to_item       = zoom_to_item ;
  
  zMapWindowFeatureItemListAddItems(window_list->zmap_tv, 
				    window_list->zmapWindow,
				    itemList);

  g_object_get(G_OBJECT(window_list->zmap_tv),
	       "tree-model", &(window_list->tree_model),
	       "tree-view",  &(window_list->view),
	       NULL);

  select_item_in_view(window_list, current_item);

  g_object_set(G_OBJECT(window_list->zmap_tv),
	       "sort-column-name", "Start",
	       "selection-mode",   GTK_SELECTION_MULTIPLE,
	       "selection-func",   selection_func_cb,
	       "selection-data",   window_list,
	       NULL);

  /* I guess there is a good reason why selection works one way
   * and row activation another */

  g_signal_connect(G_OBJECT(window_list->view), "row-activated",
		   G_CALLBACK(row_activated_cb), window_list);

  zMapGUITreeViewAttach(ZMAP_GUITREEVIEW(window_list->zmap_tv));

  if (!list)
    {
      drawListWindow(window_list) ;
    }

  gtk_container_add(GTK_CONTAINER(window_list->scrolledWindow), window_list->view) ;

  /* Now show everything. */
  gtk_widget_show_all(window_list->toplevel) ;

  return window_list ;
}



static void drawListWindow(ZMapWindowList windowList)
{
  GtkWidget *window, *vBox, *subFrame, *scrolledWindow;
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
                   GTK_SIGNAL_FUNC(destroyCB), windowList);

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

  windowList->scrolledWindow = scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(scrolledWindow));

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
static gboolean selection_func_cb(GtkTreeSelection *selection, 
				  GtkTreeModel     *model,
				  GtkTreePath      *path, 
				  gboolean          path_currently_selected,
				  gpointer          user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data;
  gint rows_selected = 0;
  GtkTreeIter iter;
  
  if(((rows_selected = gtk_tree_selection_count_selected_rows(selection)) < 1) 
     && gtk_tree_model_get_iter(model, &iter, path))
    {
      GtkTreeView *treeView = NULL;
      FooCanvasItem *item ;

      treeView = gtk_tree_selection_get_tree_view(selection);
      
      item = zMapWindowFeatureItemListGetItem(windowList->zmap_tv, 
					      windowList->context_to_item,
					      &iter);

      if (!path_currently_selected && item)
        {
	  ZMapFeature feature = NULL ;
          ZMapWindow window = windowList->zmapWindow;
	  
	  feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);

          gtk_tree_view_scroll_to_cell(treeView, path, NULL, FALSE, 0.0, 0.0);

	  if (windowList->zoom_to_item)
	    zmapWindowZoomToWorldPosition(window, TRUE, 0.0, feature->x1, 100.0, feature->x2);
	  else
	    zmapWindowItemCentreOnItem(window, item, FALSE, FALSE) ;

          zMapWindowHighlightObject(window, item, TRUE, TRUE) ;
          zMapWindowUpdateInfoPanel(window, feature, item, NULL, TRUE, TRUE);
        }
    }
  
  return TRUE ;
}





/* Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure. 
 */
static void destroyCB(GtkWidget *widget, gpointer user_data)
{
  ZMapWindowList windowList = (ZMapWindowList)user_data ;

  zMapAssert(windowList) ;

  destroyList(windowList) ;

  return;
}


/* Stop this windows contents being overwritten with a new feature. */
static void preserveCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList window_list = (ZMapWindowList)data ;

  window_list->reusable = FALSE ;

  return ;
}


/* handles the row double click event
 * When a user double clicks on a row of the feature list 
 * the feature editor window pops up. This sorts that out.
 */
static void row_activated_cb(GtkTreeView       *treeView,
			     GtkTreePath       *path,
			     GtkTreeViewColumn *col,
			     gpointer          userdata)
{
  GtkTreeModel *model;
  GtkTreeIter   iter;

  model = gtk_tree_view_get_model(treeView);

  if (gtk_tree_model_get_iter(model, &iter, path))
    {
      ZMapWindowList window_list = (ZMapWindowList)userdata ;
      FooCanvasItem *item ;

      if((item = zMapWindowFeatureItemListGetItem(window_list->zmap_tv,
						  window_list->context_to_item,
						  &iter)))
	{
	  zmapWindowFeatureShow(window_list->zmapWindow, item) ;
	}
      else
	{
	  zMapShowMsg(ZMAP_MSG_INFORMATION, "%s", 
		      "Row data is stale, unable to carry out action");
	}
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


static gboolean operateTreeModelForeachFunc(GtkTreeModel *model,
					    GtkTreePath  *path,
					    GtkTreeIter  *iter,
                                            gpointer      data)
{
  ZMapWindowList window_list = (ZMapWindowList)data ;
  FooCanvasItem *list_item = NULL;
  gboolean stop = FALSE ;
  gint action = 0;

  action = window_list->cb_action ;

  if((list_item = zMapWindowFeatureItemListGetItem(window_list->zmap_tv,
						   window_list->context_to_item,
						   iter)))
    {
      switch(action)
	{
	case WINLIST_SELECTION_HIDE:
	case WINLIST_RESULTS_HIDE:
	  foo_canvas_item_hide(list_item);
	  break;
	case WINLIST_SELECTION_SHOW:
	case WINLIST_RESULTS_SHOW:
	  foo_canvas_item_show(list_item);
	  break;
	default:
	  break;
	}
    }
  else
    {
      zMapLogWarning("%s", "stale data.");
    }
  return stop ;
}



/* Request destroy of list window, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList wList = (ZMapWindowList)data;

  gtk_widget_destroy(GTK_WIDGET(wList->toplevel));

  return ;
}


/** \Brief Will allow user to export the list as a file of specified type 
 * Err nothing happens yet.
 */
static void exportCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList window_list = (ZMapWindowList)data;
  GtkWidget          *window = NULL;

  window = window_list->toplevel;

  switch(cb_action)
    {
    case WINLISTUNK:
    case WINLISTXFF:
    case WINLISTGFF:
    default:
      zMapLogWarning("%s", "Export code not written.");
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
  ZMapWindowList window_list = (ZMapWindowList)data;
  GtkTreeModel *model;
  GtkTreeView *view;
  GtkTreePath *path;
  GtkTreeViewColumn *column;
  GtkTreeIter iter;
  gboolean empty_list = FALSE;

  view  = zMapGUITreeViewGetView(ZMAP_GUITREEVIEW( window_list->zmap_tv ));
  model = zMapGUITreeViewGetModel(ZMAP_GUITREEVIEW( window_list->zmap_tv ));

  gtk_tree_view_get_cursor(view, &path, &column);  

  if(path)
    {
      gtk_tree_model_get_iter(model, &iter, path);
      gtk_tree_path_free(path);
    }
  else
    {
      if(!(gtk_tree_model_get_iter_first(model, &iter)))
	{
	  empty_list = TRUE;
	}
    }

  if(!empty_list)
    {
      FooCanvasItem *feature_item;

      if((feature_item = zMapWindowFeatureItemListGetItem(window_list->zmap_tv,
							  window_list->context_to_item,
							  &iter)))
	{
	  zmapWindowCreateSearchWindow(window_list->zmapWindow, 
				       window_list->hash_retriever,
				       window_list->hash_retrieve_data,
				       feature_item) ;      
	}
      else
	{
	  zMapShowMsg(ZMAP_MSG_INFORMATION, "%s",
		      "Current row data is stale, unable to carry out action");
	}
    }

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
static void select_item_in_view(ZMapWindowList window_list, FooCanvasItem *input_item)
{
  GtkTreeModel *model = NULL;
  GtkWidget    *view  = NULL;
  GtkTreeIter iter;
  gboolean valid;

  g_object_get(G_OBJECT(window_list->zmap_tv),
	       "tree-view",  &view, 
	       "tree-model", &model,
	       NULL);

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first(model, &iter) ;

  while (valid)
    {
      GtkTreeSelection *selection;
      FooCanvasItem *iter_item ;

      selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));

      if((iter_item = zMapWindowFeatureItemListGetItem(window_list->zmap_tv,
						       window_list->context_to_item,
						       &iter)))
	{
	  if (iter_item == input_item)
	    {
	      gtk_tree_selection_select_iter(selection, &iter);
	      valid = FALSE;
	    }
	  else
	    valid = gtk_tree_model_iter_next(model, &iter);
	}
      else
	valid = gtk_tree_model_iter_next(model, &iter);
    }

  return ;
}



/* Find a list window in the list of list windows currently displayed that
 * is marked as reusable. */
static ZMapWindowList findReusableList(GPtrArray *window_list)
{
  ZMapWindowList reusable_window = NULL ;
  int i ;

  if (window_list && window_list->len)
    {
      for (i = 0 ; i < window_list->len ; i++)
	{
	  GtkWidget *list_widg ;
	  ZMapWindowList list ;

	  list_widg = (GtkWidget *)g_ptr_array_index(window_list, i) ;
	  list = g_object_get_data(G_OBJECT(list_widg), ZMAP_WINDOW_LIST_OBJ_KEY) ;
	  if (list->reusable)
	    {
	      reusable_window = list ;
	      break ;
	    }
	}
    }

  return reusable_window ;
}



/* Hard coded for now...will be settable from config file. */
static gboolean windowIsReusable(void)
{
  gboolean reusable = TRUE ;

  return reusable ;
}

static GHashTable *access_window_context_to_item(gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  GHashTable *context_to_item;

  context_to_item = window->context_to_item;

  return context_to_item;
}




/*************************** end of file *********************************/
