/*  File: zmapWindowDNAList.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Shows a list of dna locations that can be selected
 *              causing the zmapwindow to scroll to that location.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Oct 11 10:41 2006 (edgrif)
 * Created: Mon Oct  9 15:21:36 2006 (edgrif)
 * CVS info:   $Id: zmapWindowDNAList.c,v 1.1 2006-10-11 11:37:15 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapDNA.h>
#include <zmapWindow_P.h>




#define DNA_LIST_OBJ_KEY "ZMapWindowDNAList"


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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


static void selectItemInView(ZMapWindow window, GtkTreeView *treeView, FooCanvasItem *item);

/* callbacks for the gui... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void testButtonCB      (GtkWidget *widget, gpointer user_data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void columnClickedCB    (GtkTreeViewColumn *col, gpointer user_data);

static gboolean operateTreeModelForeachFunc(GtkTreeModel *model,
                                            GtkTreePath *path,
                                            GtkTreeIter *iter,
                                            gpointer data);
static void operateSelectionForeachFunc(GtkTreeModel *model,
                                        GtkTreePath *path,
                                        GtkTreeIter *iter,
                                        gpointer data);



static void exportCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void orderByCB (gpointer data, guint cb_action, GtkWidget *widget);
static void searchCB  (gpointer data, guint cb_action, GtkWidget *widget);
static void operateCB (gpointer data, guint cb_action, GtkWidget *widget);

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */








typedef struct _ZMapWindowListStruct
{
  ZMapWindow  window ;
  char       *title ; 

  GtkWidget     *view  ;
  GtkWidget     *toplevel ;

  GtkTreeModel *treeModel ;

  GList *dna_list ;

  int cb_action ;					    /* transient: filled in for
							       callback. */

} DNAWindowListDataStruct, *DNAWindowListData ;


static GtkWidget *makeMenuBar(DNAWindowListData wlist);
static void drawListWindow(DNAWindowListData windowList, GtkTreeModel *treeModel);




static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *window, gpointer user_data);
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

static void freeDNAMatchCB(gpointer data, gpointer user_data_unused) ;

/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] = {
 /* File */
 { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Close",         "<control>W", requestDestroyCB, 0,          NULL,       NULL},
 /* Help */
 { "/_Help",             NULL, NULL,       0,            "<LastBranch>", NULL},
 { "/Help/Overview", NULL, helpMenuCB, 0,  NULL,           NULL}
};




/* Displays a list of selectable DNA locations
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 * 
 */
void zmapWindowDNAListCreate(ZMapWindow zmapWindow, GList *dna_list, char *title, ZMapFeatureBlock block)
{
  DNAWindowListData window_list ;

  window_list = g_new0(DNAWindowListDataStruct, 1) ;

  window_list->window = zmapWindow ;
  window_list->title = title;
  window_list->dna_list = dna_list ;

  window_list->treeModel = zmapWindowFeatureListCreateStore(ZMAPWINDOWLIST_DNA_LIST) ;

  zmapWindowFeatureListPopulateStoreList(window_list->treeModel, ZMAPWINDOWLIST_DNA_LIST, dna_list, block) ;

  drawListWindow(window_list, window_list->treeModel) ;

  /* The view now holds a reference so we can get rid of our own reference */
  g_object_unref(G_OBJECT(window_list->treeModel)) ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Reread its feature data from the feature structs, important when user has done a revcomp. */
void zmapWindowListWindowReread(GtkWidget *toplevel)
{  
  DNAWindowListData window_list ;

  window_list = g_object_get_data(G_OBJECT(toplevel), DNA_LIST_OBJ_KEY) ;

  zmapWindowFeatureListRereadStoreList(GTK_TREE_VIEW(window_list->view), window_list->window) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/***************** Internal functions ************************************/

static void drawListWindow(DNAWindowListData windowList, GtkTreeModel *treeModel)
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
  g_ptr_array_add(windowList->window->dnalist_windows, (gpointer)window);
  g_object_set_data(G_OBJECT(window), 
                    DNA_LIST_OBJ_KEY, 
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

  scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(subFrame), GTK_WIDGET(scrolledWindow));


  /* Get our treeView */
  windowCallbacks.columnClickedCB = NULL; /* G_CALLBACK(columnClickedCB); */
  windowCallbacks.rowActivatedCB  = G_CALLBACK(view_RowActivatedCB);
  windowCallbacks.selectionFuncCB = selectionFuncCB;
  windowList->view = zmapWindowFeatureListCreateView(ZMAPWINDOWLIST_DNA_LIST,
						     treeModel, NULL, &windowCallbacks, windowList);

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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* sort the list on the start coordinate
   * We do this here so adding to the list isn't slowed, although the
   * list should be reasonably sorted anyway */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(treeModel), 
                                       ZMAP_WINDOW_LIST_COL_START, 
                                       GTK_SORT_ASCENDING);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Now show everything. */
  gtk_widget_show_all(window) ;

  return ;
}



/* make the menu from the global defined above !
 */
GtkWidget *makeMenuBar(DNAWindowListData wlist)
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



/* Finds dna selected and scrolls to it. */
static gboolean selectionFuncCB(GtkTreeSelection *selection, 
                                GtkTreeModel     *model,
                                GtkTreePath      *path, 
                                gboolean          path_currently_selected,
                                gpointer          user_data)
{
  DNAWindowListData windowList = (DNAWindowListData)user_data;
  gint rows_selected = 0;
  GtkTreeIter iter;
  
  if(((rows_selected = zmapWindowFeatureListCountSelected(selection)) < 1) 
     && gtk_tree_model_get_iter(model, &iter, path))
    {
      GtkTreeView *treeView = NULL;
      ZMapFeatureBlock block = NULL ;
      int start = 0, end = 0 ;

      treeView = gtk_tree_selection_get_tree_view(selection);
      
      gtk_tree_model_get(model, &iter, 
			 ZMAP_WINDOW_LIST_DNA_START, &start,
			 ZMAP_WINDOW_LIST_DNA_END, &end,
                         ZMAP_WINDOW_LIST_DNA_BLOCK, &block,
                         -1) ;
      zMapAssert(block) ;

      if (!path_currently_selected)
        {
	  double grp_start, grp_end ;
          ZMapWindow window = windowList->window;
	  FooCanvasItem *item ;

          gtk_tree_view_scroll_to_cell(treeView, path, NULL, FALSE, 0.0, 0.0);

	  grp_start = (double)start ;
	  grp_end = (double)end ;
	  zmapWindowSeq2CanOffset(&grp_start, &grp_end, block->block_to_sequence.q1) ;

	  item = zmapWindowFToIFindItemFull(window->context_to_item,
					    block->parent->unique_id, block->unique_id,
					    0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0) ;

	  zmapWindowItemCentreOnItemSubPart(window, item,
					    FALSE,
					    0.0,
					    grp_start, grp_end) ;
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
  DNAWindowListData windowList = (DNAWindowListData)user_data;
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

  zmapWindowFeatureListRereadStoreList(GTK_TREE_VIEW(wList->view), wList->window) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure. 
 */
static void destroyCB(GtkWidget *widget, gpointer user_data)
{
  DNAWindowListData windowList = (DNAWindowListData)user_data;

  if(windowList != NULL)
    {
      ZMapWindow zmapWindow = NULL;

      zmapWindow = windowList->window;

      if(zmapWindow != NULL)
        g_ptr_array_remove(zmapWindow->dnalist_windows, (gpointer)windowList->toplevel) ;

      /* Free all the dna stuff... */
      g_list_foreach(windowList->dna_list, freeDNAMatchCB, NULL) ;
      g_list_free(windowList->dna_list) ;

    }


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
      DNAWindowListData winList = (DNAWindowListData)userdata ;
      ZMapFeature feature = NULL ;
      ZMapStrand set_strand ;
      ZMapFrame set_frame ;
      FooCanvasItem *item ;

      gtk_tree_model_get(model, &iter, 
                         ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
			 ZMAP_WINDOW_LIST_COL_SET_STRAND, &set_strand,
			 ZMAP_WINDOW_LIST_COL_SET_FRAME, &set_frame,
                         -1) ;
      zMapAssert(feature) ;

      item = zmapWindowFToIFindFeatureItem(winList->window->context_to_item,
					   set_strand, set_frame,
					   feature) ;
      zMapAssert(item) ;

      zmapWindowEditorCreate(winList->window, item, winList->window->edittable_features) ;
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
  DNAWindowListData cb_data = (DNAWindowListData)data ;
  gboolean stop = FALSE ;
  ZMapFeature feature = NULL ;
  ZMapStrand set_strand ;
  ZMapFrame set_frame ;
  FooCanvasItem *listItem = NULL;
  gint action = 0;

  action = cb_data->cb_action ;

  gtk_tree_model_get(model, iter, 
		     ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
		     ZMAP_WINDOW_LIST_COL_SET_STRAND, &set_strand,
		     ZMAP_WINDOW_LIST_COL_SET_FRAME, &set_frame,
		     -1) ;
  zMapAssert(feature) ;

  listItem = zmapWindowFToIFindFeatureItem(cb_data->window->context_to_item,
					   set_strand, set_frame,
					   feature) ;
  zMapAssert(listItem) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return stop ;
}



/* Request destroy of list window, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  DNAWindowListData wList = (DNAWindowListData)data;

  gtk_widget_destroy(GTK_WIDGET(wList->toplevel));

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/** \Brief Will allow user to export the list as a file of specified type 
 * Err nothing happens yet.
 */
static void exportCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  DNAWindowListData wList = (DNAWindowListData)data;
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/*
 * Show limited help.
 */
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  char *title = "DNA List Window" ;
  char *help_text =
    "The ZMap DNA List Window shows all the matches from your DNA search.\n"
    "You can click on a match and the corresponding ZMap window will scroll\n"
    "to the DNA matchs location." ;

  zMapGUIShowText(title, help_text, FALSE) ;

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
  DNAWindowListData wList = (DNAWindowListData)data;
  GtkTreeView *treeView   = NULL;
  GtkTreeModel *treeModel = NULL;
  GtkTreeViewColumn *col  = NULL;
  GtkTreePath *path       = NULL;
  ZMapFeatureAny feature  = NULL;
  ZMapStrand set_strand ;
  ZMapFrame set_frame ;
  GtkTreeIter iter;
  FooCanvasItem *feature_item ;

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
		     ZMAP_WINDOW_LIST_COL_SET_STRAND, &set_strand,
		     ZMAP_WINDOW_LIST_COL_SET_FRAME, &set_frame,
		     -1);
  zMapAssert(feature) ;

  feature_item = zmapWindowFToIFindFeatureItem(wList->window->context_to_item,
					       set_strand, set_frame,
					       (ZMapFeature)feature) ;

  zmapWindowCreateSearchWindow(wList->window, feature_item) ;


  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void operateCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  DNAWindowListData wList    = (DNAWindowListData)data ;
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
      ZMapStrand set_strand ;
      ZMapFrame set_frame ;
      FooCanvasItem *listItem ;

      gtk_tree_model_get(treeModel, &iter, 
			 ZMAP_WINDOW_LIST_COL_FEATURE, &feature,
			 ZMAP_WINDOW_LIST_COL_SET_STRAND, &set_strand,
			 ZMAP_WINDOW_LIST_COL_SET_FRAME, &set_frame,
			 -1);

      listItem = zmapWindowFToIFindFeatureItem(window->context_to_item,
					       set_strand, set_frame,
					       feature) ;
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void freeDNAMatchCB(gpointer data, gpointer user_data_unused)
{
  ZMapDNAMatch match = (ZMapDNAMatch)data ;

  if (match->match)
    g_free(match->match) ;

  g_free(match) ;

  return ;
}




/*************************** end of file *********************************/
