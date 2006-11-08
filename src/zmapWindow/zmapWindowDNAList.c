/*  File: zmapWindowDNAList.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Shows a list of dna locations that can be selected
 *              causing the zmapwindow to scroll to that location.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Oct 11 17:09 2006 (edgrif)
 * Created: Mon Oct  9 15:21:36 2006 (edgrif)
 * CVS info:   $Id: zmapWindowDNAList.c,v 1.3 2006-11-08 09:25:06 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapDNA.h>
#include <zmapWindow_P.h>


#define DNA_LIST_OBJ_KEY "ZMapWindowDNAList"



typedef struct _ZMapWindowListStruct
{
  ZMapWindow  window ;
  char       *title ; 

  GtkWidget     *view  ;
  GtkWidget     *toplevel ;

  GtkTreeModel *treeModel ;

  GList *dna_list ;

} DNAWindowListDataStruct, *DNAWindowListData ;


static GtkWidget *makeMenuBar(DNAWindowListData wlist);
static void drawListWindow(DNAWindowListData windowList, GtkTreeModel *treeModel);




static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *window, gpointer user_data);
static gboolean selectionFuncCB(GtkTreeSelection *selection, 
                                GtkTreeModel     *model,
                                GtkTreePath      *path, 
                                gboolean          path_currently_selected,
                                gpointer          user_data);
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
  windowCallbacks.columnClickedCB = NULL ;
  windowCallbacks.rowActivatedCB  = NULL ;
  windowCallbacks.selectionFuncCB = selectionFuncCB ;
  windowList->view = zmapWindowFeatureListCreateView(ZMAPWINDOWLIST_DNA_LIST,
						     treeModel, NULL, &windowCallbacks, windowList) ;

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



/* Request destroy of list window, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  DNAWindowListData wList = (DNAWindowListData)data;

  gtk_widget_destroy(GTK_WIDGET(wList->toplevel));

  return ;
}




/*
 * Show limited help.
 */
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  char *title = "DNA List Window" ;
  char *help_text =
    "The ZMap DNA List Window shows all the matches from your DNA search.\n"
    "You can click on a match and the corresponding ZMap window will scroll\n"
    "to the matchs location." ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}



/* Free allocated DNA match structs. */
static void freeDNAMatchCB(gpointer data, gpointer user_data_unused)
{
  ZMapDNAMatch match = (ZMapDNAMatch)data ;

  if (match->match)
    g_free(match->match) ;

  g_free(match) ;

  return ;
}




/*************************** end of file *********************************/
