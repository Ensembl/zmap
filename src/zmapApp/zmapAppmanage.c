/*  File: zmapappmanage.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *          Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Top level of application, creates ZMaps.
 *              
 * Exported functions: None, all functions internal to zmapApp.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <stdio.h>

#include <ZMap/zmapUtils.h>
#include <zmapApp_P.h>


static void stopThreadCB(GtkWidget *widget, gpointer cb_data) ;
static void killThreadCB(GtkWidget *widget, gpointer data) ;
static void checkThreadCB(GtkWidget *widget, gpointer cb_data) ;

static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata);

static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          userdata);

static void view_onDoubleClick (GtkTreeView        *treeview,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata);


GtkWidget *zmapMainMakeManage(ZMapAppContext app_context)
{
  GtkWidget *frame, *treeView ;
  GtkWidget *vbox, *scrwin, *hbox, *stop_button, *kill_button, *check_button ;
  GtkTreeStore *treeStore;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;
  GtkTreeSelection  *selection;

  frame = gtk_frame_new("Manage ZMaps") ;
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 ) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;
  
  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(vbox), 5);
  gtk_container_add(GTK_CONTAINER (frame), vbox);

  /* scrolled widget */
  scrwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
  gtk_widget_set_usize(scrwin, 600, 150) ;
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, FALSE, FALSE, 0) ;
  /* tree view widget */
  app_context->tree_store_widg = 
    treeStore = gtk_tree_store_new(ZMAP_N_COLUMNS, 
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_STRING,
                                   G_TYPE_POINTER
                                   );
  treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(treeStore));
  gtk_container_add (GTK_CONTAINER (scrwin), treeView); /* tree view in scrolled widget */

  /* Setup the tree view events */
  g_signal_connect(treeView, "button-press-event", 
                   (GCallback)view_onButtonPressed, (gpointer)app_context);
  /* This will need to change if we have a true tree display
     implemented as double click usually expands/collapses a tree */
  g_signal_connect(treeView, "row-activated", 
                   (GCallback)view_onDoubleClick, (gpointer)app_context);
  selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
  gtk_tree_selection_set_select_function(selection, selectionFunc, (gpointer)app_context, NULL);

  //  g_object_unref (G_OBJECT (treeStore)); /* treeView has ref, get rid of ours */

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer),
                "foreground", "red",
                NULL);

  column = gtk_tree_view_column_new_with_attributes ("ZMap", renderer,
                                                     "text", ZMAPID_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  column = gtk_tree_view_column_new_with_attributes ("Sequence", renderer,
                                                     "text", ZMAPSEQUENCE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  column = gtk_tree_view_column_new_with_attributes ("State", renderer,
                                                     "text", ZMAPSTATE_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
  column = gtk_tree_view_column_new_with_attributes ("Last request", renderer,
                                                     "text", ZMAPLASTREQUEST_COLUMN,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);


  /* Apparently gtkclist is now deprecated, not sure what one uses instead....sigh... */


  /* Buttons */

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_container_add(GTK_CONTAINER(vbox), hbox);

  stop_button = gtk_button_new_with_label("Stop") ;
  gtk_signal_connect(GTK_OBJECT(stop_button), "clicked",
		     GTK_SIGNAL_FUNC(stopThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0) ;

  kill_button = gtk_button_new_with_label("Kill") ;
  gtk_signal_connect(GTK_OBJECT(kill_button), "clicked",
		     GTK_SIGNAL_FUNC(killThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(hbox), kill_button, FALSE, FALSE, 0) ;

  check_button = gtk_button_new_with_label("Status") ;
  gtk_signal_connect(GTK_OBJECT(check_button), "clicked",
		     GTK_SIGNAL_FUNC(checkThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(hbox), check_button, FALSE, FALSE, 0) ;

  return frame ;
}

static void view_onDoubleClick (GtkTreeView        *treeview,
                                GtkTreePath        *path,
                                GtkTreeViewColumn  *col,
                                gpointer            userdata)
{
  ZMapAppContext app_context = (ZMapAppContext)userdata ;
  GtkTreeModel *model;

  /* probably will need to change later, but for now, seems
     reasonable. */
  model = gtk_tree_view_get_model(treeview);

  zMapManagerRaise(app_context->selected_zmap);
  
  return ;
}

static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
  gboolean event_handled = FALSE ;

  /* single click with the right mouse button? */
  if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
    {
      GtkTreePath *path;

      if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview),
                                        (gint) event->x, 
                                        (gint) event->y,
                                        &path, NULL, NULL, NULL))
        {
          view_popup_menu(treeview, event, userdata);
          event_handled = TRUE; /* we handled this */
        }
    }
  
  return event_handled ; /* we could not be bothered to handle this */
}

static void view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_with_label("Do something");

  g_signal_connect(menuitem, "activate",
                   (GCallback) view_popup_menu_onDoSomething, userdata);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  /* Note: event can be NULL here when called from view_onPopupMenu;
   *  gdk_event_get_time() accepts a NULL argument */
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                 (event != NULL) ? event->button : 0,
                 gdk_event_get_time((GdkEvent*)event));
}

static void view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata)
{
#ifdef RDS_DONT_INCLUDE
  ZMapAppContext app_context = (ZMapAppContext)userdata ;
  //  zMapManagerRaise(app_context->selected_zmap);
  g_print ("Do something!\n");
#endif /* remove compiler warnings */
}

static void stopThreadCB(GtkWidget *widget, gpointer cb_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  printf("not implemented yet\n") ;


  if (app_context->selected_zmap)
    {
      zMapManagerReset(app_context->selected_zmap) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}

static void killThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  if (app_context->selected_zmap)
    {
      zMapDebug("GUI: kill thread for zmap id %s with connection pointer: %p\n",
                "zmapID", (void *)(app_context->selected_zmap)) ;
      zMapManagerKill(app_context->zmap_manager, app_context->selected_zmap) ;      
    }

  return ;
}


/* Does not work yet... */
static void checkThreadCB(GtkWidget *widget, gpointer cb_data)
{
#ifdef RDS_DONT_INCLUDE
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  printf("not implemented yet\n") ;
#endif /* remove compiler warning */
  return ;
}

static gboolean selectionFunc(GtkTreeSelection *selection, 
                              GtkTreeModel     *model,
                              GtkTreePath      *path, 
                              gboolean          path_currently_selected,
                              gpointer          user_data)
{
  GtkTreeIter iter;
  ZMapAppContext app_context = (ZMapAppContext)user_data;

  if (gtk_tree_model_get_iter(model, &iter, path))
    {
      ZMap zmap;

      gtk_tree_model_get(model, &iter, ZMAPDATA_COLUMN, &zmap, -1);

      if (!path_currently_selected)
        {
          app_context->selected_zmap = zmap;
          g_print ("%s is going to be selected.\n", "zmap");
          
        }
      else
        {
          app_context->selected_zmap = NULL;
          g_print ("%s is going to be unselected.\n", "zmap");
        }

      //      g_free(zmap); // ????
    }

  return TRUE; /* allow selection state to change */
}

