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
 * Last edited: Jan 24 11:11 2005 (edgrif)
 * Created: Mon Jan 10 10:38:43 2005 (edgrif)
 * CVS info:   $Id: zmapControlViews.c,v 1.1 2005-01-24 11:52:41 edgrif Exp $
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



/* GTK didn't introduce a function call to do this until at least release 2.6, I hope this 
 * is correct, no easy way to check exactly when function calls were introduced. */
#if ((GTK_MAJOR_VERSION == 2) && (GTK_MINOR_VERSION == 6))
#define myGetChild(WIDGET, CHILD_NUMBER)         \
  (gtk_paned_get_child##CHILD_NUMBER(GTK_PANED(WIDGET)))
#else
#define myGetChild(WIDGET, CHILD_NUMBER)         \
  ((GTK_PANED(WIDGET))->child##CHILD_NUMBER)
#endif


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void  shrinkPane      (ZMap zmap) ;
static void  resizePanes     (ZMap zmap) ;
static void  resizeOnePane   (GNode     *node, gpointer user_data);
static void  shrinkHPane     (ZMap zmap);
static int   unfocus         (GNode     *node, gpointer data);
static void  exciseNode      (GNode     *node);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static ZMapPaneChild whichChildOfPane(GtkWidget *child) ;
static void splitPane(GtkWidget *curr_frame, GtkWidget *new_frame, GtkOrientation orientation) ;
static GtkWidget *closePane(GtkWidget *close_frame) ;



static ZMapViewWindow widget2ViewWindow(GHashTable* hash_table, GtkWidget *widget) ;
static void findViewWindow(gpointer key, gpointer value, gpointer user_data) ;




/* Not a great name as it may not split and orientation may be ignored..... */
void zmapControlSplitInsertWindow(ZMap zmap, GtkOrientation orientation)
{
  GtkWidget *curr_view, *view_container ;
  ZMapViewWindow view_window ;
  ZMapView zmap_view ;
  ZMapWindow zmap_window ;

  if (zmap->focus_viewwindow)
    {
      curr_view = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;
      zmap_view = zMapViewGetView(zmap->focus_viewwindow) ;
      zmap_window = zMapViewGetWindow(zmap->focus_viewwindow) ;
    }
  else
    {
      curr_view = NULL ;

      /* UGH, I don't like this, seems a bit addhoc to just grab the only view.... */
      zMapAssert((g_list_length(zmap->view_list) == 1)) ;
      zmap_view = (ZMapView)(g_list_first(zmap->view_list)->data) ;

      zmap_window = NULL ;
    }

  view_container = zmapControlAddWindow(zmap, curr_view, orientation) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  view_window = zMapViewAddWindow(zmap_view, view_container, zmap_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!zmap_window)
    view_window = zMapViewMakeWindow(zmap_view, view_container) ;
  else
    view_window = zMapViewCopyWindow(zmap_view, view_container, zmap_window) ;


  /* Add to hash of viewwindows to frames */
  g_hash_table_insert(zmap->viewwindow_2_parent, view_window, view_container) ;


  /* If there is no current focus window we need to make this new one the focus,
   * if we don't of code will fail because it relies on having a focus window and
   * the user will not get visual feedback that a window is the focus window. */
  if (!zmap->focus_viewwindow)
    zmapControlSetWindowFocus(zmap, view_window) ;


  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  return ;
}



void zmapControlRemoveWindow(ZMap zmap)
{
  GtkWidget *close_container, *view_container ;
  ZMapViewWindow view_window ;
  gboolean remove ;

  /* We shouldn't get called if there are no windows. */
  zMapAssert((g_list_length(zmap->view_list) >= 1) && zmap->focus_viewwindow) ;

  view_window = zmap->focus_viewwindow ;		    /* focus_viewwindow gets reset so hang
							       on view_window.*/

  /* It may be that the window to be removed was also due to be unfocussed so it needs to be
   * removed from the unfocus slot. */
  if (view_window == zmap->unfocus_viewwindow)
    zmap->unfocus_viewwindow = NULL ;

  close_container = g_hash_table_lookup(zmap->viewwindow_2_parent, view_window) ;

  zMapViewRemoveWindow(view_window) ;

  /* this needs to remove the pane.....AND  set a new focuspane.... */
  zmapControlCloseWindow(zmap, close_container) ;

  /* Remove from hash of viewwindows to frames */
  remove = g_hash_table_remove(zmap->viewwindow_2_parent, view_window) ;
  zMapAssert(remove) ;

  /* Having removed one window we need to refocus on another, if there is one....... */
  if (zmap->focus_viewwindow)
    zmapControlSetWindowFocus(zmap, zmap->focus_viewwindow) ;

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  return ;
}




/* 
 * view_window is the view to add, this must be supplied.
 * orientation is GTK_ORIENTATION_HORIZONTAL or GTK_ORIENTATION_VERTICAL
 * 
 *  */
GtkWidget *zmapControlAddWindow(ZMap zmap, GtkWidget *curr_frame, GtkOrientation orientation)
{
  GtkWidget *new_frame ;

  /* We could put the sequence name as the label ??? Maybe good to do...think about it.... */
  new_frame = gtk_frame_new(NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Making the border huge does _not_ make the red bit huge when we highlight so that's not
   * much good..... */

  gtk_container_set_border_width(GTK_CONTAINER(new_frame), 30) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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



void zmapControlCloseWindow(ZMap zmap, GtkWidget *close_container)
{
  GtkWidget *pane_parent ;

  /* If parent is a pane then we need to remove that pane, otherwise we simply destroy the
   * container. */
  pane_parent = gtk_widget_get_parent(close_container) ;
  if (GTK_IS_PANED(pane_parent))
    {
      GtkWidget *keep_container ;

      keep_container = closePane(close_container) ;

      /* Set the focus to the window left over. */
      zmap->focus_viewwindow = widget2ViewWindow(zmap->viewwindow_2_parent, keep_container) ;
    }
  else
    {
      gtk_widget_destroy(close_container) ;
      zmap->focus_viewwindow = NULL ;
    }

  return ;
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
 * the that panes parent, get rid of the original pane. Returns the container that we keep. */
static GtkWidget *closePane(GtkWidget *close_frame)
{
  GtkWidget *keep_frame ;
  GtkWidget *parent_pane, *parents_parent ;
  ZMapPaneChild close_child, parent_parent_child ;

  parent_pane = gtk_widget_get_parent(close_frame) ;
  zMapAssert(GTK_IS_PANED(parent_pane)) ;

  /* Find out which child of the pane the close_frame is and hence record the frame we want to keep. */
  close_child = whichChildOfPane(close_frame) ;
  if (close_child == ZMAP_PANE_CHILD_1)
    keep_frame = myGetChild(parent_pane, 2) ;
  else
    keep_frame = myGetChild(parent_pane, 1) ;


  /* This seems tacky to be at this level......... */


  /* Remove the keep_frame from its container, we will insert it into the place where
   * its parent was originally. */
  keep_frame = gtk_widget_ref(keep_frame) ;
  gtk_container_remove(GTK_CONTAINER(parent_pane), keep_frame) ;


  /* Find out what the parent of the parent_pane is, if its not a pane then we simply insert
   * keep_frame into it, if it is a pane, we need to know which child of it our parent_pane
   * is so we insert the keep_frame in the correct place. */
  parents_parent = gtk_widget_get_parent(parent_pane) ;
  if (GTK_IS_PANED(parents_parent))
    {
      parent_parent_child = whichChildOfPane(parent_pane) ;
    }


  /* Destroy the parent_pane, this will also destroy the close_frame as it is still a child. */
  gtk_widget_destroy(parent_pane) ;


  /* Put the keep frame into the parent_parent. */
  if (!GTK_IS_PANED(parents_parent))
    {
      gtk_box_pack_start(GTK_BOX(parents_parent), keep_frame, TRUE, TRUE, 0) ;
    }
  else
    {
      if (parent_parent_child == ZMAP_PANE_CHILD_1)
	gtk_paned_pack1(GTK_PANED(parents_parent), keep_frame, TRUE, TRUE) ;
      else
	gtk_paned_pack2(GTK_PANED(parents_parent), keep_frame, TRUE, TRUE) ;

    }


  /* Now dereference the keep_frame as its now back in a container. */
  gtk_widget_unref(keep_frame) ;

  return keep_frame ;
}



/* The enter and leave stuff is slightly convoluted in that we don't want to unfocus
 * a window just because the pointer leaves it. We only want to unfocus a previous
 * window when we enter a different view window. Hence in the leave callback we just
 * record a window to be unfocussed if we subsequently enter a different view window. */
void zmapControlSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow)
{
  GtkWidget *viewwindow_frame ;
  ZMapView view ;
  ZMapWindow window ;
  ZMapWindowZoomStatus zoom_status ;
  GdkColor color ;
  double top, bottom ;

  view = zMapViewGetView(new_viewwindow) ;
  window = zMapViewGetWindow(new_viewwindow) ;

  if (zmap->unfocus_viewwindow)
    {
      GtkWidget *unfocus_frame ;

      unfocus_frame = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->unfocus_viewwindow) ;

      gtk_frame_set_shadow_type(GTK_FRAME(unfocus_frame), GTK_SHADOW_OUT) ;

      gtk_widget_modify_bg(GTK_WIDGET(unfocus_frame), GTK_STATE_NORMAL, NULL) ;
      gtk_widget_modify_fg(GTK_WIDGET(unfocus_frame), GTK_STATE_NORMAL, NULL) ;

      zmap->unfocus_viewwindow = NULL ;
    }

  zmap->focus_viewwindow = new_viewwindow ;

  viewwindow_frame = g_hash_table_lookup(zmap->viewwindow_2_parent, zmap->focus_viewwindow) ;

  gtk_frame_set_shadow_type(GTK_FRAME(viewwindow_frame), GTK_SHADOW_IN);

  gdk_color_parse("red", &color);
  gtk_widget_modify_bg(GTK_WIDGET(viewwindow_frame), GTK_STATE_NORMAL, &color);
  gtk_widget_modify_fg(GTK_WIDGET(viewwindow_frame), GTK_STATE_NORMAL, &color);


  /* make sure the zoom buttons are appropriately sensitised for this window. */
  zoom_status = zMapWindowGetZoomStatus(window) ;
  zmapControlWindowSetZoomButtons(zmap, zoom_status) ;
      


  /* NOTE HOW NONE OF THE NAVIGATOR STUFF IS SET HERE....BUT NEEDS TO BE.... */
  zMapWindowGetVisible(window, &top, &bottom) ;
  zMapNavigatorSetView(zmap->navigator, zMapViewGetFeatures(view), top, bottom) ;

  return ;
}


/* When we get notified that the pointer has left a window, mark that window to
 * be unfocussed if we subsequently enter another view window. */
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow)
{
  zmap->unfocus_viewwindow = new_viewwindow ;

  return ;
}



/* Returns which child of a pane the given widget is, the child widget _must_ be in the pane
 * otherwise the function will abort. */
static ZMapPaneChild whichChildOfPane(GtkWidget *child)
{
  ZMapPaneChild pane_child = ZMAP_PANE_NONE ;
  GtkWidget *pane_parent ;

  pane_parent = gtk_widget_get_parent(child) ;
  zMapAssert(GTK_IS_PANED(pane_parent)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* These calls look like they are only in a later version of GTK2 so I have to access the
   * paned struct directly here...sigh.... */
  if (gtk_paned_get_child1(GTK_PANED(pane_parent)) == child)
    close_child = ZMAP_PANE_CHILD_1 ;
  else if (gtk_paned_get_child2(GTK_PANED(pane_parent)) == child)
    close_child = ZMAP_PANE_CHILD_2 ;
  zMapAssert(close_child != ZMAP_PANE_NONE) ;

  if ((GTK_PANED(pane_parent))->child1 == child)
    close_child = ZMAP_PANE_CHILD_1 ;
  else if ((GTK_PANED(pane_parent))->child2 == child)
    close_child = ZMAP_PANE_CHILD_2 ;
  zMapAssert(close_child != ZMAP_PANE_NONE) ;		    /* Should never happen. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (myGetChild(pane_parent, 1) == child)
    pane_child = ZMAP_PANE_CHILD_1 ;
  else if (myGetChild(pane_parent, 2) == child)
    pane_child = ZMAP_PANE_CHILD_2 ;
  zMapAssert(pane_child != ZMAP_PANE_NONE) ;		    /* Should never happen. */

  return pane_child ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapControlUnSetWindowFocus(ZMap zmap, ZMapViewWindow new_viewwindow)
{
  GtkWidget *viewwindow_frame ;
  ZMapWindow window ;
  ZMapWindowZoomStatus zoom_status ;
  GdkColor color ;

  viewwindow_frame = g_hash_table_lookup(zmap->viewwindow_2_parent, new_viewwindow) ;

  gtk_frame_set_shadow_type(GTK_FRAME(viewwindow_frame), GTK_SHADOW_OUT) ;

  gtk_widget_modify_bg(GTK_WIDGET(viewwindow_frame), GTK_STATE_NORMAL, NULL) ;
  gtk_widget_modify_fg(GTK_WIDGET(viewwindow_frame), GTK_STATE_NORMAL, NULL) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE




/* Returns a widget that can be used as the parent of any child to be inserted into the new pane. */
GtkWidget *splitHPane(ZMap zmap)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapPane new_pane ;

  /* slim the old pane */
  shrinkHPane(zmap);

  /* add a new pane next to the old one */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  new_pane = zmapAddPane(zmap, 'v');
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* focus on the new pane */
  zmapRecordFocus(new_pane) ;


  /* I'm not happy this is right Rob, shouldn't this actually be obtained from a new view or an
   * existing one.... */
  zmapControlWindowSetZoomButtons(zmap, ZMAP_ZOOM_MID);



  gtk_widget_set_sensitive(zmap->close_but, TRUE);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* resize all the panes to fit the width */
  resizeHPanes(zmap);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return new_pane->frame ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return NULL ;
}




void closePane(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;
  GtkWidget *pane_up1, *pane_up2, *frame = NULL;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This is cumbersome! The problem is that each time we split the window,
   * we add a pane to the current frame, then delete the corresponding node
   * from the panesTree.  When we delete panes from the display, we also need
   * to remove these extra frames as they become empty. */
  pane_up1 = gtk_widget_get_ancestor(zmap->focuspane->frame, GTK_TYPE_PANED);
  frame    = gtk_widget_get_ancestor(pane_up1, GTK_TYPE_FRAME);
  pane_up2 = gtk_widget_get_ancestor(frame, GTK_TYPE_PANED);

  /* THIS SHOULD BE A CALL TO VIEW....IT BEGS THE QUESTION OF WHETHER DESTROYING A PANE
   * MEANS DESTROYING THE ATTACHED VIEW AS WELL........ */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_object_destroy(GTK_OBJECT(zmap->focuspane->canvas));
  gtk_widget_destroy(zmap->focuspane->scrolledWindow);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  
  children = gtk_container_get_children(GTK_CONTAINER(pane_up1));
  if (children == NULL)
    gtk_widget_destroy(pane_up1);

  children = gtk_container_get_children(GTK_CONTAINER(frame));
  if (children == NULL)
    gtk_widget_destroy(frame);
				
  if (pane_up2) /* can't be certain there will be one */
    {	    
      children = gtk_container_get_children(GTK_CONTAINER(pane_up2));
      if (children == NULL)
	gtk_widget_destroy(pane_up2);
    }
  if (children)
    g_list_free(children);
  
  /* Remove entry from the list of windows. */
  /* Destroy any featureListWindows for this window at the same time. */
  {
    ZMapViewWindow view_window = zmap->focuspane->curr_view_window;
    GList *list;
    
    zMapWindowDestroyLists(zMapViewGetWindow(view_window));
    
    list = g_list_remove(zMapViewGetWindowList(view_window),
			 zmap->focuspane->curr_view_window); 
    /* Update the ZMapWindow->window_list with the new list address. */
    zMapViewSetWindowList(view_window, list);
  }
       
  /* UM, WE DON'T SEEM TO DESTROY THE PANE HERE ???????????? */
  gtk_widget_destroy(zmap->focuspane->frame);
  free(zmap->focuspane) ;
            

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* In theory, by this point all redundant panes and frames have been destroyed.
   * In reality, I have seen 'hanging' panes (grey squares of window lacking
   * a canvas and scroll bars) after closing multiply-split windows, but life 
   * is short! */
  zmapRecordFocus(next->data);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  

  resizePanes(zmap);                                                                                

  if (panes == 2)  /* that is, there were 2 panes until we just closed one */
    gtk_widget_set_sensitive(zmap->close_but, FALSE);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}











/* static functions **********************************************/


static int unfocus(GNode *node, gpointer data)
{
  ZMapPane pane = (ZMapPane)node->data;
  GdkColor color;

  if (pane) /* skip the actual root node which is not a valid widget */
    {
      gtk_frame_set_shadow_type(GTK_FRAME(pane->frame), GTK_SHADOW_NONE);

      gtk_widget_modify_bg(GTK_WIDGET(pane->frame), GTK_STATE_NORMAL, NULL) ;
    }
  return 0;  
}



/* shrinkPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new pane and frame and reposition
 * the scrolled window into that.  Later on we resize all panes. */
static void shrinkPane(ZMap zmap)
{
  ZMapPane pane ;
  GdkColor color;

  /* create a new vpane and hook the old scrolled window up to it */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pane->pane = gtk_vpaned_new() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* I think this is correct ??? */
  pane->frame = zmap->focuspane->frame ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gdk_color_parse("dark grey", &color);
  gtk_widget_modify_bg(GTK_WIDGET(zmap->focuspane->frame), GTK_STATE_NORMAL, &color);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  pane->curr_view_window = zmap->focuspane->curr_view_window ;


  /* reparent the scrolled window into the new frame & pack1 the frame into the new vpane,
   * then add the new vpane to the frame of the pane which currently has focus, ie that being split.  */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->scrolledWindow), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->view_parent_box), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(zmap->focuspane->frame), pane->pane);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* remove the old ZMapPane from panesTree, and add the new one */
  node = g_node_find(zmap->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     (gpointer)zmap->focuspane) ;
  g_node_append_data(node->parent, pane);

  exciseNode(node);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* do we really need this? */
  /*  gtk_widget_hide(zMapWindowGetFocuspane(window)->frame); */
  zmap->focuspane = pane ;

  return;
}


/* resizing the panes doesn't really work too well, but it's not too
 * bad for a few panes and time is short. */
static void resizePanes(ZMap zmap)
{
  GtkRequisition req;
  GNode *node = NULL;
  GNode *last = NULL;
  guint panes = 0;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* count how many panes there are in this column */
  node = g_node_find(zmap->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     zmap->focuspane) ;

  last =  g_node_last_child(node->parent);
  /* first child is number 0 */
  panes = g_node_child_position(node->parent,last);

  if (panes == 0)
    panes = 1;

  /* get size of the window & divide it by the number of panes.
   * Note there's a small kludge here to adjust for a better fit.
   * Don't know why it needs the -8. */
  gtk_widget_size_request(GTK_WIDGET(zmap->pane_vbox), &req);
  req.height = (req.height/panes) - 8;

  g_node_children_foreach(zmap->panesTree, 
		  G_TRAVERSE_LEAFS, 
		  resizeOnePane, 
		  &req);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return;
}


static void resizeOnePane(GNode *node, gpointer data)
{
  ZMapPane pane = (ZMapPane)node->data;
  GtkRequisition *req = (GtkRequisition*)data;

  if (pane)  
    gtk_widget_set_size_request (GTK_WIDGET (pane->frame), req->width, req->height);

  return;
}  



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* shrinkHPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new, half-size one and reposition
 * the scrolled window into that */
static void shrinkHPane(ZMap zmap)
{
  ZMapPane pane ;

  /* create a new hpane and hook the old scrolled window up to it */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pane->pane = gtk_hpaned_new() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* I think this is correct ??? */
  pane->frame = zmap->focuspane->frame ;

  pane->curr_view_window = zmap->focuspane->curr_view_window ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->scrolledWindow), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->view_parent_box), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(zmap->focuspane->frame), pane->pane);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* add new pane to windowsTree. */
  node = g_node_find(zmap->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     zmap->focuspane);
  g_node_append_data(node->parent, pane);

  exciseNode(node);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* do we really need this? */  
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_hide(zmap->focuspane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  zmap->focuspane = pane ;

  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/********************* end of file ********************************/
