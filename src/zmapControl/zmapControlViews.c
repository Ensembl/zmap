/*  File: zmapControlSplit.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Splits the zmap window to show either the same view twice
 *              or two different views.
 *              
 *              This is a complete rewrite of the original.
 *              
 * Exported functions: See zmapControl.h
 * HISTORY:
 * Last edited: Mar 20 18:33 2006 (rds)
 * Created: Mon Jan 10 10:38:43 2005 (edgrif)
 * CVS info:   $Id: zmapControlViews.c,v 1.12 2006-03-20 18:36:24 rds Exp $
 *-------------------------------------------------------------------
 */
 
#include <zmapControl_P.h>
#include <ZMap/zmapUtilsDebug.h>


/* Used to record which child we are of a pane widget. */
typedef enum {ZMAP_PANE_NONE, ZMAP_PANE_CHILD_1, ZMAP_PANE_CHILD_2} ZMapPaneChild ;


/* Used in doing a reverse lookup for the ZMapViewWindow -> container widget hash. */
typedef struct
{
  GtkWidget *widget ;
  ZMapViewWindow view_window ;
} ZMapControlWidget2ViewwindowStruct ;



/* GTK didn't introduce a function call to get at pane children until at least release 2.6,
 * I hope this is correct, no easy way to check exactly when function calls were introduced. */
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION == 6))
#define myGetChild(WIDGET, CHILD_NUMBER)         \
  (gtk_paned_get_child##CHILD_NUMBER(GTK_PANED(WIDGET)))
#else
#define myGetChild(WIDGET, CHILD_NUMBER)         \
  ((GTK_PANED(WIDGET))->child##CHILD_NUMBER)
#endif


static ZMapPaneChild whichChildOfPane(GtkWidget *child) ;
static void splitPane(GtkWidget *curr_frame, GtkWidget *new_frame, GtkOrientation orientation) ;
static GtkWidget *closePane(GtkWidget *close_frame) ;

static ZMapViewWindow widget2ViewWindow(GHashTable* hash_table, GtkWidget *widget) ;
static void findViewWindow(gpointer key, gpointer value, gpointer user_data) ;
static ZMapViewWindow closeWindow(ZMap zmap, GtkWidget *close_container) ;



/* New func for brand new view windows.... */
ZMapView zmapControlNewWindow(ZMap zmap, char *sequence, int start, int end)
{
  ZMapView zmap_view = NULL ;
  GtkWidget *curr_container, *view_container ;
  ZMapViewWindow view_window ;
  char *view_title ;
  GtkOrientation orientation = GTK_ORIENTATION_VERTICAL ;   /* arbitrary for first window. */


  /* If there is a focus window then that will be the one we split and we need to find out
   * the container parent of that canvas. */
  if (zmap->focus_viewwindow)
    curr_container = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;
  else
    curr_container = NULL ;


  view_title = sequence ;

  /* Add a new container that will hold the new view window. */
  view_container = zmapControlAddWindow(zmap, curr_container, orientation, view_title) ;

  view_window = zMapViewCreate(view_container, sequence, start, end, (void *)zmap) ;
  zmap_view = zMapViewGetView(view_window) ;


  /* Add to hash of viewwindows to frames */
  g_hash_table_insert(zmap->viewwindow_2_parent, view_window, view_container) ;


  /* If there is no current focus window we need to make this new one the focus,
   * if we don't of code will fail because it relies on having a focus window and
   * the user will not get visual feedback that a window is the focus window. */
  if (!zmap->focus_viewwindow)
    zmapControlSetWindowFocus(zmap, view_window) ;

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  zmapControlWindowSetGUIState(zmap) ;


  return zmap_view ;
}


/* Not a great name as it may not split and orientation may be ignored..... */
void zmapControlSplitWindow(ZMap zmap, GtkOrientation orientation)
{
  GtkWidget *curr_container, *view_container ;
  ZMapViewWindow view_window ;
  ZMapView zmap_view ;
  ZMapWindow zmap_window ;
  char *view_title ;
  ZMapWindowLockType window_locking = ZMAP_WINLOCK_NONE ;

  /* If there is a focus window then that will be the one we split and we need to find out
   * the container parent of that canvas. */
  if (zmap->focus_viewwindow)
    curr_container = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;
  else
    curr_container = NULL ;

  /* If we are adding a new view then there won't yet be a window for it, if we are splitting
   * an existing view then if it has a window we need to split that, otherwise we need to add
   * the first window to that view. */
  zmap_window = NULL ;

  if (zmap->focus_viewwindow)
    {
      zmap_view = zMapViewGetView(zmap->focus_viewwindow) ;
      zmap_window = zMapViewGetWindow(zmap->focus_viewwindow) ;
    }
  else
    {
      /* UGH, I don't like this, seems a bit addhoc to just grab the only view.... */
      zMapAssert((g_list_length(zmap->view_list) == 1)) ;
      zmap_view = (ZMapView)(g_list_first(zmap->view_list)->data) ;
    }

  view_title = zMapViewGetSequence(zmap_view) ;

  /* Add a new container that will hold the new view window. */
  view_container = zmapControlAddWindow(zmap, curr_container, orientation, view_title) ;


  /* Copy the focus view window. */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    window_locking = ZMAP_WINLOCK_HORIZONTAL ;
  else if (orientation == GTK_ORIENTATION_VERTICAL)
    window_locking = ZMAP_WINLOCK_VERTICAL ;

  view_window = zMapViewCopyWindow(zmap_view, view_container, zmap_window, window_locking) ;


  /* Add to hash of viewwindows to frames */
  g_hash_table_insert(zmap->viewwindow_2_parent, view_window, view_container) ;


  /* If there is no current focus window we need to make this new one the focus,
   * if we don't of code will fail because it relies on having a focus window and
   * the user will not get visual feedback that a window is the focus window. */
  if (!zmap->focus_viewwindow)
    zmapControlSetWindowFocus(zmap, view_window) ;

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}



/* Utility function to return number of views left. */
int zmapControlNumViews(ZMap zmap)
{
  int num_views = 0 ;

  if (zmap->view_list)
    num_views = g_list_length(zmap->view_list) ;

  return num_views ;
}



/* You need to remember that there may be more than one view in a zmap. This means that
 * while a particular view may have lost all its windows and need closing, there might
 * be other views that have windows that can be focussed on. */
void zmapControlRemoveWindow(ZMap zmap)
{
  GtkWidget *close_container ;
  ZMapViewWindow view_window, remaining_view ;
  ZMapView view ;
  gboolean remove ;
  int num_views, num_windows ;


  num_views = zmapControlNumViews(zmap) ;
  num_windows = zMapViewNumWindows(zmap->focus_viewwindow) ;

  /* We shouldn't get called if there are no views or if there is one view with one window left. */
  zMapAssert(num_views && zmap->focus_viewwindow
	     && !(num_views == 1 && num_windows == 1)) ;


  /* focus_viewwindow gets reset so hang on to view_window pointer and view.*/
  view_window = zmap->focus_viewwindow ;
  view = zMapViewGetView(view_window) ;


  close_container = g_hash_table_lookup(zmap->viewwindow_2_parent, view_window) ;

  /* Make sure we reset focus because we are removing the view it points to ! */
  zmap->focus_viewwindow = NULL ;

  if (num_windows > 1)
    zMapViewRemoveWindow(view_window) ;
  else
    zMapDeleteView(zmap, view) ;


  /* this needs to remove the pane.....AND  set a new focuspane....if there is one.... */
  remaining_view = closeWindow(zmap, close_container) ;


  /* Remove from hash of viewwindows to frames */
  remove = g_hash_table_remove(zmap->viewwindow_2_parent, view_window) ;
  zMapAssert(remove) ;


  /* Having removed one window we need to refocus on another, if there is one....... */
  if (remaining_view)
    {
      zmapControlSetWindowFocus(zmap, remaining_view) ;
      zMapWindowSiblingWasRemoved(zMapViewGetWindow(remaining_view));
    }

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}




/* 
 * view_window is the view to add, this must be supplied.
 * orientation is GTK_ORIENTATION_HORIZONTAL or GTK_ORIENTATION_VERTICAL
 * 
 *  */
GtkWidget *zmapControlAddWindow(ZMap zmap, GtkWidget *curr_frame, GtkOrientation orientation,
				char *view_title)
{
  GtkWidget *new_frame ;

  /* Supplying NULL will remove the title if its too big. */
  new_frame = gtk_frame_new(view_title) ;

  /* If there is a parent then add this pane as child two of parent, otherwise it means
   * this is the first pane and it it just gets added to the zmap vbox. */
  if (curr_frame)
    {
      /* Here we want to split the existing pane etc....... */
      splitPane(curr_frame, new_frame, orientation) ;
    }
  else
    gtk_box_pack_start(GTK_BOX(zmap->pane_vbox), new_frame, TRUE, TRUE, 0) ;

  return new_frame ;
}



/* Returns the viewWindow left in the other half of the pane, but note when its
 * last window, there is no viewWindow left over. */
static ZMapViewWindow closeWindow(ZMap zmap, GtkWidget *close_container)
{
  ZMapViewWindow remaining_view ;
  GtkWidget *pane_parent ;

  /* If parent is a pane then we need to remove that pane, otherwise we simply destroy the
   * container. */
  pane_parent = gtk_widget_get_parent(close_container) ;
  if (GTK_IS_PANED(pane_parent))
    {
      GtkWidget *keep_container ;

      keep_container = closePane(close_container) ;

      /* Set the focus to the window left over. */
      remaining_view = widget2ViewWindow(zmap->viewwindow_2_parent, keep_container) ;
    }
  else
    {
      gtk_widget_destroy(close_container) ;
      remaining_view = NULL ;
    }

  return remaining_view ;
}

static ZMapViewWindow widget2ViewWindow(GHashTable* hash_table, GtkWidget *widget)
{
  ZMapControlWidget2ViewwindowStruct widg2view ;

  widg2view.widget = widget ;
  widg2view.view_window = NULL ;

  g_hash_table_foreach(hash_table, findViewWindow, &widg2view) ;

  return widg2view.view_window ;
}


/* Test for value == user_data, i.e. have we found the widget given by user_data ? */
static void findViewWindow(gpointer key, gpointer value, gpointer user_data)
{
  ZMapControlWidget2ViewwindowStruct *widg2View = (ZMapControlWidget2ViewwindowStruct *)user_data ;

  if (value == widg2View->widget)
    {
      widg2View->view_window = key ;
    }

  return ;
}



/* Split a pane, actually we add a new pane into the selected window of the parent pane. */
static void splitPane(GtkWidget *curr_frame, GtkWidget *new_frame, GtkOrientation orientation)
{
  GtkWidget *pane_parent, *new_pane ;
  ZMapPaneChild curr_child = ZMAP_PANE_NONE ;


  /* Get current frames parent, if window is unsplit it will be a vbox, otherwise its a pane
   * and we need to know which child we are of the pane. */
  pane_parent = gtk_widget_get_parent(curr_frame) ;
  if (GTK_IS_PANED(pane_parent))
    {
      /* Which child are we of the parent pane ? */
      curr_child = whichChildOfPane(curr_frame) ;
    }


  /* Remove the current frame from its container so we can insert a new container as the child
   * of that container, we have to increase its reference counter to stop it being.
   * destroyed once its removed from its container. */
  curr_frame = gtk_widget_ref(curr_frame) ;
  gtk_container_remove(GTK_CONTAINER(pane_parent), curr_frame) ;

  /* Create the new pane, note that horizontal split => splitting the pane across the middle,
   * vertical split => splitting the pane down the middle. */
  if (orientation == GTK_ORIENTATION_HORIZONTAL)
    new_pane = gtk_vpaned_new() ;
  else
    new_pane = gtk_hpaned_new() ;

  /* Now insert the new pane into the vbox or pane parent. */
  if (!GTK_IS_PANED(pane_parent))
    {
      gtk_box_pack_start(GTK_BOX(pane_parent), new_pane, TRUE, TRUE, 0) ;
    }
  else
    {
      if (curr_child == ZMAP_PANE_CHILD_1)
	gtk_paned_pack1(GTK_PANED(pane_parent), new_pane, TRUE, TRUE) ;
      else
	gtk_paned_pack2(GTK_PANED(pane_parent), new_pane, TRUE, TRUE) ;
    }


  /* Add the frame views to the new pane. */
  gtk_paned_pack1(GTK_PANED(new_pane), curr_frame, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(new_pane), new_frame, TRUE, TRUE);

  /* Now dereference the original frame as its now back in a container. */
  gtk_widget_unref(curr_frame) ;

  return ;
}



/* Close the container in one half of a pane, reparent the container in the other half into
 * the that panes parent, get rid of the original pane.
 * Returns a new/current direct container of a view. */
static GtkWidget *closePane(GtkWidget *close_frame)
{
  GtkWidget *keep_container, *keep_frame = NULL ;
  GtkWidget *parent_pane, *parents_parent ;
  ZMapPaneChild close_child, parent_parent_child ;

  parent_pane = gtk_widget_get_parent(close_frame) ;
  zMapAssert(GTK_IS_PANED(parent_pane)) ;

  /* Find out which child of the pane the close_frame is and hence record the container
   * (n.b. might be another frame or might be a pane containing more panes/frames)
   * frame we want to keep. */
  close_child = whichChildOfPane(close_frame) ;
  if (close_child == ZMAP_PANE_CHILD_1)
    keep_container = myGetChild(parent_pane, 2) ;
  else
    keep_container = myGetChild(parent_pane, 1) ;


  /* Remove the keep_container from its container, we will insert it into the place where
   * its parent was originally. */
  keep_container = gtk_widget_ref(keep_container) ;
  zMapAssert(GTK_IS_PANED(keep_container) || GTK_IS_FRAME(keep_container)) ;
  gtk_container_remove(GTK_CONTAINER(parent_pane), keep_container) ;


  /* Find out what the parent of the parent_pane is, if its not a pane then we simply insert
   * keep_container into it, if it is a pane, we need to know which child of it our parent_pane
   * is so we insert the keep_container in the correct place. */
  parents_parent = gtk_widget_get_parent(parent_pane) ;
  if (GTK_IS_PANED(parents_parent))
    {
      parent_parent_child = whichChildOfPane(parent_pane) ;
    }


  /* Destroy the parent_pane, this will also destroy the close_frame as it is still a child. */
  gtk_widget_destroy(parent_pane) ;

  /* Put the keep_container into the parent_parent. */
  if (!GTK_IS_PANED(parents_parent))
    {
      gtk_box_pack_start(GTK_BOX(parents_parent), keep_container, TRUE, TRUE, 0) ;
    }
  else
    {
      if (parent_parent_child == ZMAP_PANE_CHILD_1)
	gtk_paned_pack1(GTK_PANED(parents_parent), keep_container, TRUE, TRUE) ;
      else
	gtk_paned_pack2(GTK_PANED(parents_parent), keep_container, TRUE, TRUE) ;
    }

  /* Now dereference the keep_container as its now back in a container. */
  gtk_widget_unref(keep_container) ;

  /* The keep_container may be a frame _but_ it may also be a pane and its children may be
   * panes so we have to go down until we find a child that is a frame to return as the
   * new current frame. (Note that we arbitrarily go down the child1 children until we find
   * a frame.) */
  while (GTK_IS_PANED(keep_container))
    {
      keep_container = myGetChild(keep_container, 1) ;
    }
  keep_frame = keep_container ;

  return keep_frame ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* replaced by "click to focus" */

/* The enter and leave stuff is slightly convoluted in that we don't want to unfocus
 * a window just because the pointer leaves it. We only want to unfocus a previous
 * window when we enter a different view window. Hence in the leave callback we just
 * record a window to be unfocussed if we subsequently enter a different view window. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


void zmapControlSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow)
{
  GtkWidget *viewwindow_frame ;
  ZMapView view ;
  ZMapWindow window ;
  GdkColor color ;
  double top, bottom ;

  zMapAssert(new_viewwindow) ;

  if (new_viewwindow != zmap->focus_viewwindow)
    {
      /* Unfocus the old window. */
      if (zmap->focus_viewwindow)
	{
	  GtkWidget *unfocus_frame ;
          
	  unfocus_frame = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;
          gtk_frame_set_shadow_type(GTK_FRAME(unfocus_frame), GTK_SHADOW_OUT) ;
          gtk_widget_set_name(GTK_WIDGET(unfocus_frame), "GtkFrame");
	}

      /* focus the new one. */
      zmap->focus_viewwindow = new_viewwindow ;
      view = zMapViewGetView(new_viewwindow) ;
      window = zMapViewGetWindow(new_viewwindow) ;

      viewwindow_frame = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;
      gtk_frame_set_shadow_type(GTK_FRAME(viewwindow_frame), GTK_SHADOW_IN);
      gtk_widget_set_name(GTK_WIDGET(viewwindow_frame), "zmap-focus-view");

      /* make sure zoom buttons etc. appropriately sensitised for this window. */
      zmapControlWindowSetGUIState(zmap) ;


      /* NOTE HOW NONE OF THE NAVIGATOR STUFF IS SET HERE....BUT NEEDS TO BE.... */
      zMapWindowGetVisible(window, &top, &bottom) ;
      zMapNavigatorSetView(zmap->navigator, zMapViewGetFeatures(view), top, bottom) ;
    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* now we've changed to "click to focus" this is redundant. */

/* When we get notified that the pointer has left a window, mark that window to
 * be unfocussed if we subsequently enter another view window. */
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow)
{
  zmap->unfocus_viewwindow = new_viewwindow ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Returns which child of a pane the given widget is, the child widget _must_ be in the pane
 * otherwise the function will abort. */
static ZMapPaneChild whichChildOfPane(GtkWidget *child)
{
  ZMapPaneChild pane_child = ZMAP_PANE_NONE ;
  GtkWidget *pane_parent ;

  pane_parent = gtk_widget_get_parent(child) ;
  zMapAssert(GTK_IS_PANED(pane_parent)) ;

  if (myGetChild(pane_parent, 1) == child)
    pane_child = ZMAP_PANE_CHILD_1 ;
  else if (myGetChild(pane_parent, 2) == child)
    pane_child = ZMAP_PANE_CHILD_2 ;
  zMapAssert(pane_child != ZMAP_PANE_NONE) ;		    /* Should never happen. */

  return pane_child ;
}

