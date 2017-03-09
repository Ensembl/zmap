/*  File: zmapAppmanage.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Top level of application, creates ZMaps.
 *              
 * Exported functions: None, all functions internal to zmapApp.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtils.hpp>
#include <zmapApp_P.hpp>


/* Enum to define columns in manage panel of main window. */
enum
  {
    ZMAPID_COLUMN,                                          /* zmap id, unique per zmap */
    ZMAPSTATE_COLUMN,                                       /* zmaps internal state */
    ZMAPSEQUENCE_COLUMN,                                    /* sequence displayed in the zmap. */
    ZMAP_PTR_COLUMN,                                        /* non-displayed zmap pointer. */
    VIEW_PTR_COLUMN,                                        /* non-displayed view pointer for this sequence. */
    ZMAP_N_COLUMNS
  };



/* Used to search of a zmap within the list of displayed zmaps. */
typedef struct SearchDataStructType
{
  ZMap orig_zmap ;
  ZMapView orig_view ;                                      /* Can be NULL if view not loaded yet. */

  char *zmap_iter_str ;
} SearchDataStruct, *SearchData ;



static void closeZMapViewCB(GtkWidget *widget, gpointer data) ;

static gboolean selectionCB(GtkTreeSelection *selection, 
                            GtkTreeModel     *model,
                            GtkTreePath      *path, 
                            gboolean          path_currently_selected,
                            gpointer          userdata);
static void viewOnDoubleClickCB(GtkTreeView        *treeview,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata);

static gboolean searchForZMapForeachFunc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) ;
static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) ;




/* 
 *                   Package routines
 */


GtkWidget *zmapMainMakeManage(ZMapAppContext app_context)
{
  GtkWidget *frame, *treeView ;
  GtkWidget *vbox, *scrwin, *hbox, *close_button ;
  GtkTreeStore *treeStore;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection  *selection;

  frame = gtk_frame_new("Manage ZMaps:") ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;
  
  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(vbox), 5);
  gtk_container_add(GTK_CONTAINER (frame), vbox);

  /* scrolled widget */
  scrwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
  gtk_widget_set_usize(scrwin, 600, 150) ;
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, FALSE, FALSE, 0) ;

  /* tree view widget */
  app_context->tree_store_widg = treeStore = gtk_tree_store_new(ZMAP_N_COLUMNS, 
                                                                G_TYPE_STRING,
                                                                G_TYPE_STRING,
                                                                G_TYPE_STRING,
                                                                G_TYPE_POINTER,
                                                                G_TYPE_POINTER) ;

  treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treeStore));
  gtk_container_add (GTK_CONTAINER (scrwin), treeView); /* tree view in scrolled widget */


  g_signal_connect(treeView, "row-activated", 
                   (GCallback)viewOnDoubleClickCB, (gpointer)app_context) ;

  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
  gtk_tree_selection_set_select_function(selection, selectionCB, (gpointer)app_context, NULL);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer),
                "foreground", "red",
                NULL);

  column = gtk_tree_view_column_new_with_attributes ("ZMap", renderer,
                                                     "text", ZMAPID_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);

  column = gtk_tree_view_column_new_with_attributes ("State", renderer,
                                                     "text", ZMAPSTATE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);

  column = gtk_tree_view_column_new_with_attributes ("Sequence", renderer,
                                                     "text", ZMAPSEQUENCE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);


  /* Buttons */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);


  close_button = gtk_button_new_with_label("Close Selected ZMap View") ;
  gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
                     GTK_SIGNAL_FUNC(closeZMapViewCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(hbox), close_button, FALSE, FALSE, 0) ;


  return frame ;
}


/* Updates the zmap id/state/sequence in the main window, this may mean creating
 * an entry in the list for a new zmap or simply updating an existing one. */
void zmapAppManagerUpdate(ZMapAppContext app_context, ZMap zmap, ZMapView view)
{
  GtkTreeIter iter ;
  SearchDataStruct search_data = {NULL} ;
  GtkTreeModel *tree_model = GTK_TREE_MODEL(app_context->tree_store_widg) ;


  /* Check if the zmap/view is already in the list of zmaps, if not add a new entry. */
  search_data.orig_zmap = zmap ;
  search_data.orig_view = view ;
  gtk_tree_model_foreach(tree_model,
                         (GtkTreeModelForeachFunc)searchForZMapForeachFunc,
                         &search_data) ;

  if (!search_data.zmap_iter_str)
    {
      gtk_tree_store_append(app_context->tree_store_widg, &iter, NULL) ;

      search_data.zmap_iter_str = gtk_tree_model_get_string_from_iter(tree_model, &iter) ;
    }


  /* Update the zmap entry with any state or sequence information. */
  if (!(search_data.zmap_iter_str)
      || !gtk_tree_model_get_iter_from_string(tree_model, &iter, search_data.zmap_iter_str))
    {
      zMapLogCritical("Could not find ZMap id = %s (%p) in main window list.", zMapGetZMapID(zmap), zmap) ;
    }
  else
    {
      char *seq_name = NULL ;

      if (view)
        seq_name = zMapViewGetSequence(view) ;

      /* Need the view in here too in the end.... */
      /* Now update our list of zmaps.... */
      gtk_tree_store_set(app_context->tree_store_widg, &iter,
                         ZMAPID_COLUMN, zMapGetZMapID(zmap),
                         ZMAPSTATE_COLUMN, zMapGetZMapStatus(zmap),
                         ZMAPSEQUENCE_COLUMN, 
                         (seq_name ? seq_name : "<none>"),
                         ZMAP_PTR_COLUMN, (gpointer)zmap,
                         VIEW_PTR_COLUMN, (gpointer)view,
                         -1) ;

      if (seq_name)
        g_free(seq_name) ;

      g_free(search_data.zmap_iter_str) ;
    }

  

  return ;
}



/* Called every time a zmap window signals that it has gone, this might be because
 * the user clicked the close button for that window or because the user clicked
 * quit/close on the app window which leads to all zmap windows being closed.
 * 
 * If view == NULL then all views for the given zmap will be removed...
 * 
 * 
 *  */
void zmapAppManagerRemove(ZMapAppContext app_context, ZMap zmap, ZMapView view)
{
  SearchDataStruct search_data = {NULL} ;
  GtkTreeModel *tree_model = GTK_TREE_MODEL(app_context->tree_store_widg) ;


  /* This has the potential to remove multiple rows, but currently
     doesn't as the first found one that matches, gets removed an
     returns true. See
     http://scentric.net/tutorial/sec-treemodel-remove-many-rows.html
     for an implementation of mutliple deletes */
  search_data.orig_zmap = zmap ;
  search_data.orig_view = view ;

  gtk_tree_model_foreach(tree_model,
                         (GtkTreeModelForeachFunc)removeZMapRowForeachFunc,
                         &search_data) ;

  if (app_context->selected_zmap == zmap)
    app_context->selected_zmap = NULL ;

  return ;
}






/* 
 *                  Internal routines
 */

static void viewOnDoubleClickCB(GtkTreeView        *treeview,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata)
{
  ZMapAppContext app_context = (ZMapAppContext)userdata ;

  /* probably will need to change later, but for now, seems
     reasonable. */
  gtk_tree_view_get_model(treeview);

  zMapManagerRaise(app_context->selected_zmap);
  
  return ;
}


static void closeZMapViewCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  if (app_context->selected_zmap)
    {
      zMapDebug("GUI: close thread for zmap id %s with connection pointer: %p\n",
                "zmapID", (void *)(app_context->selected_zmap)) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapManagerKill(app_context->zmap_manager, app_context->selected_zmap) ;      
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      zMapManagerDestroyView(app_context->zmap_manager, app_context->selected_zmap, app_context->selected_view) ;
    }

  return ;
}


static gboolean selectionCB(GtkTreeSelection *selection, 
                            GtkTreeModel     *model,
                            GtkTreePath      *path, 
                            gboolean          path_currently_selected,
                            gpointer          user_data)
{
  gboolean result = TRUE ;                                  /* Make sure event goes to tree widg. */
  GtkTreeIter iter ;
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  if (gtk_tree_model_get_iter(model, &iter, path))
    {
      ZMap zmap = NULL ;
      ZMapView view = NULL ;

      gtk_tree_model_get(model, &iter,
                         ZMAP_PTR_COLUMN, &zmap,
                         VIEW_PTR_COLUMN, &view,
                         -1) ;

      app_context->selected_zmap = zmap ;
      app_context->selected_view = view ;
    }

  return result ;
}




static gboolean searchForZMapForeachFunc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gboolean result = FALSE ;
  SearchData search_data = (SearchData)data ;
  ZMap row_zmap = NULL ;
  ZMapView row_view = NULL ;

  gtk_tree_model_get(model, iter,
                     ZMAP_PTR_COLUMN, &row_zmap,
                     VIEW_PTR_COLUMN, &row_view,
                     -1) ;

  if (search_data->orig_zmap == row_zmap && (!row_view || search_data->orig_view == row_view))
    {
      search_data->zmap_iter_str = gtk_tree_model_get_string_from_iter(model, iter) ;

      result = TRUE ;                                       /* Stop the foreach. */
    }

  return result ;
}


static gboolean removeZMapRowForeachFunc(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  gboolean result = FALSE ;
  SearchData search_data = (SearchData)data ;
  ZMap row_zmap ;
  ZMapView row_view = NULL ;

  gtk_tree_model_get(model, iter,
                     ZMAP_PTR_COLUMN, &row_zmap,
                     VIEW_PTR_COLUMN, &row_view,
                     -1) ;

  if (search_data->orig_zmap == row_zmap  && (!row_view || search_data->orig_view == row_view))
    {
      gtk_tree_store_remove(GTK_TREE_STORE(model), iter) ;

      /* Stop the search if a specific view was given, otherwise remove all views. */
      if (search_data->orig_view)
        result = TRUE ;
    }

  return result ;
}

