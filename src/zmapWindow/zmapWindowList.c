/*  File: zmapWindowList.c
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
 * and was written by
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *         Roy Storey   (Sanger Institute, UK) rds@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Displays a list of features from which the user may select
 *
 *
 * Exported functions:
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <glib.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindowFeatureList.h>
#include <zmapWindow_P.h>

#include <items/zmapWindowCanvasItem_I.h> /* for debugguing */

#define ZMAP_WINDOW_LIST_OBJ_KEY "ZMapWindowList"


/* ZMapWindowListStruct structure declaration
 *
 */
typedef struct _ZMapWindowListStruct
{
  ZMapWindow     zmap_window ; /*!< pointer to the zmapWindow that created us.   */
  char          *title ;  /*!< Title for the window  */

  GtkWidget     *toplevel ;
  GtkWidget     *scrolledWindow ;
  GtkWidget     *view  ;  /*!< The treeView so we can get store, selection ... */

  GHashTable    *context_to_item;

  ZMapFeatureContextMap context_map ;

  /* enable getting the hash */
  ZMapWindowListGetFToIHash get_hash_func;
  gpointer                  get_hash_data;
  /* enable searching the hash */
  ZMapWindowListSearchHashFunc search_hash_func;
  gpointer                     search_hash_data;
  GDestroyNotify               search_hash_free;


  GtkTreeModel *tree_model ;

  gboolean zoom_to_item ;
  gboolean reusable ;

  int cb_action ;					    /* transient: filled in for callback. */

  ZMapWindowFeatureItemList zmap_tv;
} ZMapWindowListStruct, *ZMapWindowList ;

typedef struct
{
  ZMapWindowList window_list;
  ZMapFeature    feature;
  GtkTreeIter   *iterator;
} TreeViewContextMenuDataStruct, *TreeViewContextMenuData;


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
static ZMapWindowList window_list_window_create(ZMapWindowList               window_list_in,
						FooCanvasItem               *current_item,
						char                        *title,
						ZMapWindow                   zmap_window,
						GList *items,
						ZMapWindowListGetFToIHash    get_hash_func,
						gpointer                     get_hash_data,
						ZMapFeatureContextMap context_map,
						ZMapWindowListSearchHashFunc search_hash_func,
						gpointer                     search_hash_data,
						GDestroyNotify               search_hash_free,
						gboolean                     zoom_to_item);

static void drawListWindow(ZMapWindowList windowList);
static GtkWidget *makeMenuBar(ZMapWindowList wlist);

static void select_item_in_view(ZMapWindowList window_list, FooCanvasItem *item);

static gboolean row_popup_cb(GtkWidget      *widget,
			     GdkEventButton *event,
			     gpointer        user_data) ;

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



static void feature_menu_cb(int menu_item_id, gpointer callback_data);
static ZMapGUIMenuItem getFeatureOps(ZMapWindowList window_list,
				     ZMapFeature    feature,
				     GtkTreeIter   *iterator,
				     gpointer       cb_data);
static void make_list_menu_item(ZMapWindowList  window_list,
				GdkEventButton *button,
				ZMapFeature     feature,
				GtkTreeIter    *iterator);

static GHashTable *default_get_ftoi_hash(gpointer user_data);
static GList *default_search_hash_func(ZMapWindow window, GHashTable *hash_table, gpointer user_data);
static GList *invoke_search_func(ZMapWindow window, ZMapWindowListGetFToIHash get_hash_func, gpointer get_hash_data,
				 ZMapWindowListSearchHashFunc search_hash_func, gpointer search_hash_data) ;

static void window_list_truncate(ZMapWindowList window_list);
static void window_list_populate(ZMapWindowList window_list, GList *item_list);


/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] = {
 /* File */
 { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Search",        NULL,         searchCB,   0,          NULL,       NULL},
 { "/File/Preserve",      NULL,         preserveCB, 0,          NULL,       NULL},
 { "/File/Export",        NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Export/GFF v2", NULL,         exportCB,   WINLISTGFF, NULL,       NULL},
#ifdef MORE_FORMATS
 { "/File/Export/XFF",    NULL,         exportCB,   WINLISTXFF, NULL,       NULL},
#endif /* MORE_FORMATS */
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
void zmapWindowListWindowCreate(ZMapWindow                   window,
				FooCanvasItem               *current_item,
				char                        *title,
				ZMapWindowListGetFToIHash    get_hash_func,
				gpointer                     get_hash_data,
				ZMapFeatureContextMap context_map,
				ZMapWindowListSearchHashFunc search_hash_func,
				gpointer                     search_hash_data,
				GDestroyNotify               search_hash_free,
				gboolean                     zoom_to_item)
{
  ZMapWindowList window_list = NULL ;
  GList *items ;

  if (!get_hash_func)
    {
      get_hash_func = default_get_ftoi_hash ;
      get_hash_data = window ;
    }

  if (!search_hash_func)
    {
      search_hash_func = default_search_hash_func;
      search_hash_data = NULL;
      search_hash_free = NULL;
    }

  /* check for no data here.... */
  if (!(items = invoke_search_func(window,get_hash_func, get_hash_data, search_hash_func, search_hash_data)))
    {
      zMapWarning("%s", "Search found no items") ;
    }
  else
    {
      window_list = window_list_window_create(NULL, current_item, title, window,
					      items,
					      get_hash_func, get_hash_data, context_map,
					      search_hash_func, search_hash_data, search_hash_free,
					      zoom_to_item) ;
    }

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
void zmapWindowListWindow(ZMapWindow                   window,
			  FooCanvasItem               *current_item,
			  char                        *title,
			  ZMapWindowListGetFToIHash    get_hash_func,
			  gpointer                     get_hash_data,
			  ZMapFeatureContextMap context_map,
			  ZMapWindowListSearchHashFunc search_hash_func,
			  gpointer                     search_hash_data,
			  GDestroyNotify               search_hash_free,
			  gboolean                     zoom_to_item)
{
  ZMapWindowList window_list = NULL ;
  GList *items ;


  if (!get_hash_func)
    {
      get_hash_func = default_get_ftoi_hash ;
      get_hash_data = window ;
    }

  if (!search_hash_func)
    {
      search_hash_func = default_search_hash_func;
      search_hash_data = NULL;
      search_hash_free = NULL;
    }

  /* check for no data here.... */
  if (!(items = invoke_search_func(window,get_hash_func, get_hash_data, search_hash_func, search_hash_data)))
    {
      zMapWarning("%s", "Search found no items") ;
    }
  else
    {
      /* Look for a reusable window. */
      window_list = findReusableList(window->featureListWindows) ;

      /* now show the window, if we found a reusable one that will be reused. */
      window_list = window_list_window_create(window_list, current_item, title, window,
					      items,
					      get_hash_func, get_hash_data, context_map,
					      search_hash_func, search_hash_data, search_hash_free,
					      zoom_to_item) ;

      zMapGUIRaiseToTop(window_list->toplevel);


      g_list_free(items) ;
    }


  return ;
}

/* Reread its feature data from the feature structs, important when user has done a revcomp. */
void zmapWindowListWindowReread(GtkWidget *toplevel)
{
  ZMapWindowList window_list ;

  window_list = g_object_get_data(G_OBJECT(toplevel), ZMAP_WINDOW_LIST_OBJ_KEY) ;

  window_list->context_to_item = (window_list->get_hash_func)(window_list->get_hash_data);

  if(!zMapWindowFeatureItemListUpdateAll(window_list->zmap_tv,
					 window_list->zmap_window,
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

static void destroyList(ZMapWindowList window_list)
{
  ZMapWindow zmap ;

  zmap = window_list->zmap_window ;

  if (window_list->search_hash_free && window_list->search_hash_data)
    (window_list->search_hash_free)(window_list->search_hash_data);

  zMapGUITreeViewDestroy(ZMAP_GUITREEVIEW(window_list->zmap_tv));

  g_ptr_array_remove(zmap->featureListWindows, (gpointer)window_list->toplevel) ;

  g_free(window_list) ;

  return;
}



static void destroySubList(ZMapWindowList windowList)
{
  gtk_widget_destroy(windowList->view) ;

  return;
}

static gboolean row_popup_cb(GtkWidget      *widget,
			     GdkEventButton *event,
			     gpointer        user_data)
{
  ZMapWindowList window_list = (ZMapWindowList)user_data;
  gboolean handled = FALSE;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      if(event->button == 3)
	{
	  ZMapFeature feature;
	  GtkTreePath *path = NULL;

	  if(gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
					   event->x,
					   event->y,
					   &path,
					   NULL, NULL, NULL))
	    {
	      GtkTreeModel *model;
	      GtkTreeIter iter;

	      model = gtk_tree_view_get_model(GTK_TREE_VIEW(widget));

	      if(gtk_tree_model_get_iter(model, &iter, path))
		{
		  feature = zMapWindowFeatureItemListGetFeature(window_list->zmap_window,
                                                window_list->zmap_tv,
								window_list->context_to_item,
								&iter);

		  make_list_menu_item(window_list, event, feature, &iter);

		  gtk_tree_view_set_cursor(GTK_TREE_VIEW(widget), path, NULL, FALSE);

		  gtk_tree_path_free(path);
		}

	      handled = TRUE;
	    }
	}
      break;
    default:
      break;
    }

  return handled;
}

/* Construct a list window using a new toplevel window if list = NULL and the toplevel window of
 * list if not. */
static ZMapWindowList window_list_window_create(ZMapWindowList               window_list_in,
						FooCanvasItem               *current_item,
						char                        *title,
						ZMapWindow                   zmap_window,
						GList *items,
						ZMapWindowListGetFToIHash get_hash_func, gpointer get_hash_data,
						ZMapFeatureContextMap context_map,
						ZMapWindowListSearchHashFunc search_hash_func,
						gpointer search_hash_data,
						GDestroyNotify               search_hash_free,
						gboolean                     zoom_to_item)
{
  ZMapWindowList window_list = NULL;
  gboolean input_was_null = FALSE;

  if (!window_list_in)
    {
      input_was_null = TRUE;
      window_list    = createList() ;
    }
  else
    {
      window_list = window_list_in ;
      zMapGUITreeViewDestroy(ZMAP_GUITREEVIEW(window_list->zmap_tv));
      window_list->zmap_tv = NULL;
      destroySubList(window_list_in) ;
    }

  window_list_in = NULL;

  window_list->zmap_tv = zMapWindowFeatureItemListCreate(ZMAPSTYLE_MODE_INVALID);

  window_list->zmap_window     = zmap_window ;
  window_list->title           = title;
  window_list->context_to_item = (get_hash_func)(get_hash_data);
  window_list->get_hash_func   = get_hash_func;
  window_list->get_hash_data   = get_hash_data;
  window_list->search_hash_func = search_hash_func;
  window_list->search_hash_data = search_hash_data;
  window_list->search_hash_free = search_hash_free;
  window_list->zoom_to_item    = zoom_to_item ;

  window_list_truncate(window_list);

  window_list_populate(window_list, items) ;

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

  g_signal_connect(G_OBJECT(window_list->view), "button-press-event",
		   G_CALLBACK(row_popup_cb), window_list);

  zMapGUITreeViewAttach(ZMAP_GUITREEVIEW(window_list->zmap_tv));

  if (input_was_null)
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
  g_ptr_array_add(windowList->zmap_window->featureListWindows, (gpointer)window);
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

      item = zMapWindowFeatureItemListGetItem(windowList->zmap_window,
                                    windowList->zmap_tv,
					      windowList->context_to_item,
					      &iter);

      if (!path_currently_selected && item)
        {
	  ZMapFeature feature = NULL ;
        ZMapWindow window = windowList->zmap_window;
        ZMapWindowContainerFeatureSet cfs;
        cfs = (ZMapWindowContainerFeatureSet)
            zmapWindowContainerUtilsItemGetParentLevel(item,ZMAPCONTAINER_LEVEL_FEATURESET);

        zMapAssert(cfs);

	  feature = zmapWindowItemGetFeature(item);

        gtk_tree_view_scroll_to_cell(treeView, path, NULL, FALSE, 0.0, 0.0);

            /* if a feature is masked then make sure it's visible */
      if (zMapStyleGetMode(feature->style) == ZMAPSTYLE_MODE_ALIGNMENT &&
            (!feature->feature.homol.flags.displayed))
#if MH17_SHOW_ALL_MASKED
        {
            /* most disconcerting to ask for one feature and get hundreds */

            zMapWindowContainerFeatureSetShowHideMaskedFeatures(cfs,TRUE,FALSE);

            /* un/bumped features might be wider */
            zmapWindowFullReposition(window) ;
        }
#else
        {
            ZMapStyleBumpMode bump_mode = ZMAPBUMP_INVALID;
            GList *items;

            bump_mode = zMapWindowContainerFeatureSetGetContainerBumpMode(cfs);


            if(bump_mode > ZMAPBUMP_UNBUMP)
                  zmapWindowColumnBump(FOO_CANVAS_ITEM(cfs),ZMAPBUMP_UNBUMP);

                  /* we need to show all items of the same name */
            items = zmapWindowFToIFindSameNameItems(window,window->context_to_item,
                  zMapFeatureStrand2Str(zmapWindowContainerFeatureSetGetStrand(cfs)),
                  zMapFeatureFrame2Str (zmapWindowContainerFeatureSetGetFrame(cfs)),
                  feature) ;
            if(!items)
                  return(FALSE);

            for(;items;items = items->next)
            {
                  FooCanvasItem *feature_item = (FooCanvasItem *) items->data;

                 /* they want to see it so remove the mask */
                  feature = zmapWindowItemGetFeature(feature_item);
                  feature->feature.homol.flags.masked = FALSE;

/* MH17:
 *
 * there's some problem with gapped items that have to get redisplayed
 * (see addGapsCB in ColBump.c)
 * but the bump code should take care of this
 * NOTE here we only display one feature and not all with the same name
 * if an alignment is gapped then it does not display although red triangles do
 * foo_canvas_item_show does a repick, but that is a delayed action thing
 * the bump code should re-do items and features so this should work
 */

            /* MH17: not sure how much of this is needed
             * we have to cope with bumped and unbumped columns
             * bumping redisplays alignments and changes (re-allocates) features and items
             * but non bumped columns should have features displayed here
             */
                  foo_canvas_item_show(feature_item);
                  feature = zmapWindowItemGetFeature(feature_item);
                  feature->feature.homol.flags.displayed = TRUE;
            }

            if(bump_mode > ZMAPBUMP_UNBUMP)
            {
                  zmapWindowColumnBump(FOO_CANVAS_ITEM(cfs),bump_mode);

                  /* un/bumped features might be wider */
                  zmapWindowFullReposition(window) ;
            }
        }
#endif

        /* in case the col got bumped (by the user or the code above)
         * we have to find the item from the feature as the item is not the same one
         * see RT 205912 and 205744
         */
#warning possible memory allocation error here if the item was changed eg by bumping

        item = zmapWindowFToIFindFeatureItem(window,window->context_to_item,
                        zmapWindowContainerFeatureSetGetStrand(cfs),
                        zmapWindowContainerFeatureSetGetFrame(cfs),
                        feature) ;
        if(!item)
            return(FALSE);

        if (windowList->zoom_to_item)
	      zmapWindowZoomToWorldPosition(window, TRUE, 0.0, feature->x1, 100.0, feature->x2);
	  else
	      zmapWindowItemCentreOnItem(window, item, FALSE, FALSE) ;

        zMapWindowHighlightObject(window, item, TRUE, TRUE) ;
        zmapWindowUpdateInfoPanel(window, feature, item, NULL, 0, 0, 0, 0, NULL, TRUE, TRUE);
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

      if((item = zMapWindowFeatureItemListGetItem(window_list->zmap_window,
                                      window_list->zmap_tv,
						  window_list->context_to_item,
						  &iter)))
	{
	  zmapWindowFeatureShow(window_list->zmap_window, item) ;
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

  if((list_item = zMapWindowFeatureItemListGetItem(window_list->zmap_window,
                                       window_list->zmap_tv,
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

typedef struct
{
  GFunc    func;
  gpointer data;
} ExportDataStruct, *ExportData;

static void invoke_dump_function_cb(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *item = NULL;
  ZMapFeatureAny feature = NULL;

  if(FOO_IS_CANVAS_ITEM(list_data))
    {
      ExportData func_data = (ExportData)user_data;
      GFunc    feature_output_func = NULL;
      gpointer feature_output_data = NULL;

      item    = FOO_CANVAS_ITEM(list_data);
      feature = zmapWindowItemGetFeatureAny(item);

      feature_output_func = func_data->func;
      feature_output_data = func_data->data;

      (feature_output_func)(feature, feature_output_data);
    }

  return ;
}

static void add_dump_offset_coord_box(GtkWidget *vbox, gpointer user_data)
{
#ifdef WHEN_WE_NEED_TO
  ZMapWindowList window_list = (ZMapWindowList)user_data;

  if(GTK_IS_VBOX(vbox))
    {
      GtkWidget *entry_box, *label, *hbox, *our_vbox;

      entry_box = gtk_entry_new();

      label = gtk_label_new("Genomic offset: ");

      hbox = gtk_hbox_new(FALSE, 1);
      /* We should be able to get this from the window, but I bet we can't ATM. */
      gtk_entry_set_text(GTK_ENTRY(entry_box), "0");

      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
      gtk_box_pack_start(GTK_BOX(hbox), entry_box, TRUE, TRUE, 0);


      our_vbox = gtk_vbox_new(FALSE, 0);

      label = gtk_label_new("Enter the 'Genomic offset' for the start of the region "
			    "and the 'Name' of the file to save the output to.");

      gtk_box_pack_start(GTK_BOX(our_vbox), label, FALSE, FALSE, 5);
      gtk_box_pack_start(GTK_BOX(our_vbox), hbox, FALSE, FALSE, 5);


      gtk_box_pack_start(GTK_BOX(vbox), our_vbox, FALSE, FALSE, 0);

      gtk_box_reorder_child(GTK_BOX(vbox), our_vbox, 0);

    }
#endif /* WHEN_WE_NEED_TO */

  return ;
}

/** \Brief Will allow user to export the list as a file of specified type
 * Err nothing happens yet.
 */
static void exportCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  ZMapWindowList window_list = (ZMapWindowList)data;
  ZMapWindow window = window_list->zmap_window ;
  GtkWidget *toplevel = window_list->toplevel;
  gpointer  feature_out_data = NULL;
  GIOChannel           *file = NULL;
  GError         *file_error = NULL;
  char             *filepath = NULL;
  gboolean            result = FALSE;

  if (!(filepath = zmapGUIFileChooserFull(toplevel, "Feature Dump filename ?", NULL, "gff",
					  add_dump_offset_coord_box, window_list))
      || !(file = g_io_channel_new_file(filepath, "w", &file_error)))
    {
      if(file_error)
	{
	  zMapShowMsg(ZMAP_MSG_WARNING, "%s %s", "Feature dump failed.", file_error->message);

	  g_error_free(file_error);

	  file_error = NULL;
	  file = NULL;
	}
    }
  else
    result = TRUE;

  switch(cb_action)
    {
    case WINLISTGFF:
      if(result)
	{
	  GList *list = NULL;

	  list = invoke_search_func(window,window_list->get_hash_func, window_list->get_hash_data,
				    window_list->search_hash_func, window_list->search_hash_data) ;

	  if(list)
	    {
	      ExportDataStruct export_data = {NULL};
	      GError                *error = NULL;
	      ZMapFeatureAny   feature_any = NULL;

	      feature_any = zmapWindowItemGetFeatureAny(list->data);

	      /* We have to do this as the list is a list of FooCanvasItem * not ZMapFeatureAny. */

	      if (zMapGFFDumpForeachList(feature_any, window->context_map->styles, file, &error, NULL,
					 &(export_data.func), &(export_data.data)))
		{
		  feature_out_data = export_data.data;

		  g_list_foreach(list, invoke_dump_function_cb, &export_data);
		}

	      g_list_free(list);	/* free list */
	    }
	}
      break;
    case WINLISTUNK:
    case WINLISTXFF:
    default:
      zMapLogWarning("%s", "Export code not written.");
      break;
    }

  if (file && !file_error)
    g_io_channel_shutdown(file, TRUE, &file_error);

  if (feature_out_data)
    zMapFeatureListForeachDumperDestroy(feature_out_data);

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

      if((feature_item = zMapWindowFeatureItemListGetItem(window_list->zmap_window,
                                            window_list->zmap_tv,
							  window_list->context_to_item,
							  &iter)))
	{
	  zmapWindowCreateSearchWindow(window_list->zmap_window,
				       window_list->get_hash_func,
				       window_list->get_hash_data,
				       window_list->context_map,
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

      if((iter_item = zMapWindowFeatureItemListGetItem(window_list->zmap_window,
                                           window_list->zmap_tv,
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



/* Context Menu code */

static void feature_menu_cb(int menu_item_id, gpointer callback_data)
{
  TreeViewContextMenuData menu_data = (TreeViewContextMenuData)callback_data;

  switch(menu_item_id)
    {
    case 1:
      {
	ZMapFeature feature = menu_data->feature;
	GError *error = NULL;

	if(feature->url)
	  zMapLaunchWebBrowser(feature->url, &error);
      }
      break;
    default:
      break;
    }

  g_free(menu_data);
}

static ZMapGUIMenuItem getFeatureOps(ZMapWindowList window_list,
				     ZMapFeature    feature,
				     GtkTreeIter   *iterator,
				     gpointer       cb_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Visit URL", 1, feature_menu_cb, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    };
  ZMapGUIMenuItem menu_ptr = NULL;

  if(feature->url)
    {
      zMapGUIPopulateMenu(menu, NULL, NULL, cb_data);
      menu_ptr = menu;
    }

  return menu_ptr;
}

static void make_list_menu_item(ZMapWindowList  window_list,
				GdkEventButton *button,
				ZMapFeature     feature,
				GtkTreeIter    *iterator)
{
  TreeViewContextMenuData full_data;
  ZMapGUIMenuItem menu = NULL;
  GList *menu_sets = NULL;
  char *menu_title;

  menu_title = zMapFeatureName((ZMapFeatureAny)feature);

  full_data = g_new0(TreeViewContextMenuDataStruct, 1);
  full_data->window_list = window_list;
  full_data->feature     = feature;
  full_data->iterator    = iterator;

  if((menu = getFeatureOps(window_list, feature, iterator, full_data)))
    menu_sets  = g_list_append(menu_sets, menu);

  if(menu_sets != NULL)
    zMapGUIMakeMenu(menu_title, menu_sets, button);

  return ;
}

static void window_list_truncate(ZMapWindowList window_list)
{
  return ;
}

static void window_list_populate(ZMapWindowList window_list, GList *item_list)
{
  if(window_list->zmap_tv && window_list->zmap_window)
    {
      zMapWindowFeatureItemListAddItems(window_list->zmap_tv,
					window_list->zmap_window,
					item_list);
    }

  return ;
}


static GList *invoke_search_func(ZMapWindow window, ZMapWindowListGetFToIHash get_hash_func, gpointer get_hash_data,
				 ZMapWindowListSearchHashFunc search_hash_func, gpointer search_hash_data)
{
  GList *result = NULL ;
  GHashTable *ftoi_hash = NULL ;

  if (get_hash_func)
    ftoi_hash = (get_hash_func)(get_hash_data) ;

  if (ftoi_hash && search_hash_func)
    result = (search_hash_func)(window,ftoi_hash, search_hash_data) ;

  return result ;
}




static GHashTable *default_get_ftoi_hash(gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;
  GHashTable *context_to_item = NULL;

  if(window)
    context_to_item = window->context_to_item;

  return context_to_item;
}

static GList *default_search_hash_func(ZMapWindow window, GHashTable *hash_table, gpointer user_data)
{
  GList *result = NULL;

  return result;
}

/*************************** end of file *********************************/
