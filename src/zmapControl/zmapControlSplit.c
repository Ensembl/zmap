/*  Last edited: Dec  2 16:08 2004 (rnc) */
/*  file: zmapsplit.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 *      Rob Clack    (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */
 
#include <zmapControl_P.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapUtilsDebug.h>

/* function prototypes **************************************************/

static void  shrinkPane      (ZMap zmap) ;
static void  resizePanes     (ZMap zmap) ;
static void  resizeOnePane   (GNode     *node, gpointer user_data);
static void  shrinkHPane     (ZMap zmap);
static int   unfocus         (GNode     *node, gpointer data);
static void  exciseNode      (GNode     *node);

/* end of prototypes ***************************************************/

/*! @file zmapsplit.c
 *  @brief Handles splitting the data display into multiple panes.
 *
 *  @author Rob Clack (rnc@sanger.ac.uk)
 *  @detailed
 *  zmapsplit looks after splitting the display into two or more panes.
 *  It uses gtk panes and effectively allows the user to split the display
 *  into as many horizontal and vertical panes as she wants.  In reality
 *  there are still quite a few wrinkles and more than a few splits is not
 *  really that practical.
 */


/* zmapAddPane is called each time we add a new pane. Creates a new frame, scrolled
 * window and canvas.  First time through it sticks them in the window->zoomvbox, 
 * thereafter it packs them into the lower part of the focus pane.
 *
 * Splitting goes like this: we make a (h or v) pane, add a new frame to the child1 
 * position of that, then reparent the scrolled window containing the canvas into 
 * the new frame. That's the shrinkPane() function. 
 * Then in zmapAddPane we add a new frame in the child2 position, and into that we
 * load a new scrolled window and canvas. 
 */
/* Returns a new pane that can be used as the parent of any child to be inserted into the new pane. */
ZMapPane zmapAddPane(ZMap zmap, char orientation)
{
  ZMapPane pane ;
  GNode *node = NULL;

  /* set up ZMapPane for this window */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;
  pane->frame = gtk_frame_new(NULL) ;
  pane->view_parent_box = gtk_vbox_new(FALSE, 0) ;



  /* This is glitchy, there may not be a focuspane yet....this kind of thing implies
   * that the panes stuff is not really sorted out..... */
  if (zmap->focuspane)
    pane->curr_view_window = zmap->focuspane->curr_view_window ;


  /* add the view_parent_box to the frame, the view_parent_box will be the actual parent
   * of the view. */
  gtk_container_add(GTK_CONTAINER(pane->frame), pane->view_parent_box) ;


  /* The idea of the GNode tree is that panes split horizontally, ie
   * one above the other, end up as siblings in the tree, while panes
   * split vertically (side by side) are parents/children.  In theory
   * this enables us to get the sizing right. In practice it's not
   * perfect yet.*/
  if (zmap->firstTime) 
    { 
      g_node_append_data(zmap->panesTree, pane) ;
    }
  else
    {
      node = g_node_find (zmap->panesTree,
			  G_IN_ORDER,
			  G_TRAVERSE_ALL,
			  zmap->focuspane) ;
      if (orientation == 'h')
	g_node_append_data(node->parent, pane) ;
      else
	g_node_append_data(node, pane) ;
    }


  /* First time through, we add the frame to the main vbox. 
   * Subsequently it goes in the lower half of the current pane. */
  if (zmap->firstTime)
    gtk_box_pack_start(GTK_BOX(zmap->pane_vbox), pane->frame, TRUE, TRUE, 0);
  else
    gtk_paned_pack2(GTK_PANED(zmap->focuspane->pane), pane->frame, TRUE, TRUE);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* focus on the new pane */
  zmapRecordFocus(pane) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return pane ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* The old version............ */

void addPane(ZMap zmap, char orientation)
{
  ZMapPane pane ;
  GtkAdjustment *adj; 
  GtkWidget *w;
  GNode *node = NULL;


  /* set up ZMapPane for this window */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pane->DNAwidth = 100;
  pane->step_increment = 10;
  pane->box2col = g_array_sized_new(FALSE, TRUE, sizeof(ZMapColumn), 50);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  pane->frame = gtk_frame_new(NULL) ;

  pane->view_parent_box = gtk_vbox_new(FALSE, 0) ;


  /* add the view_parent_box to the frame, the view parent will be the actual parent
   * of the view. */
  gtk_container_add(GTK_CONTAINER(pane->frame), pane->view_parent_box) ;





  /* The idea of the GNode tree is that panes split horizontally, ie
   * one above the other, end up as siblings in the tree, while panes
   * split vertically (side by side) are parents/children.  In theory
   * this enables us to get the sizing right. In practice it's not
   * perfect yet.*/
  if (zmap->firstTime) 
    { 
      g_node_append_data(zmap->panesTree, pane);
    }
  else
    {
      node = g_node_find (zmap->panesTree,
			  G_IN_ORDER,
			  G_TRAVERSE_ALL,
			  zmap->focuspane);
      if (orientation == 'h')
	  g_node_append_data(node->parent, pane);
      else
	  g_node_append_data(node, pane);
    }


  /* First time through, we add the frame to the main vbox. 
   * Subsequently it goes in the lower half of the current pane. */
  if (zmap->firstTime)
    gtk_box_pack_start(GTK_BOX(zmap->pane_vbox), pane->frame, TRUE, TRUE, 0);
  else
    gtk_paned_pack2(GTK_PANED(zmap->focuspane->pane), pane->frame, TRUE, TRUE);



  /* focus on the new pane */
  zmapRecordFocus(pane);
  gtk_widget_grab_focus(pane->frame);



  /* if we do this first time, a little blank box appears before the main display */
  if (!zmap->firstTime) 
    gtk_widget_show_all(zmap->navview_frame) ;

  zmap->firstTime = FALSE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This should all be in some kind of routine to draw the whole canvas in one go.... */
  zmMainScale(pane->canvas, 30, 0, 1000);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Returns a widget that can be used as the parent of any child to be inserted into the new pane. */
GtkWidget *splitPane(ZMap zmap)
{
  ZMapPane new_pane ;

  /* shrink the old pane */
  shrinkPane(zmap) ;

  /* add a new pane below to the old one */
  new_pane = zmapAddPane(zmap, 'h') ;

  /* focus on the new pane */
  zmapRecordFocus(new_pane) ;

  /*  zmapControlWindowSetZoomButtons(zmap, ZMAP_ZOOM_MID);*/

  gtk_widget_set_sensitive(zmap->close_but, TRUE);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* resize all the panes to fit the height */
  resizePanes(zmap) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return new_pane->view_parent_box ;
}


/* Returns a widget that can be used as the parent of any child to be inserted into the new pane. */
GtkWidget *splitHPane(ZMap zmap)
{
  ZMapPane new_pane ;

  /* slim the old pane */
  shrinkHPane(zmap);

  /* add a new pane next to the old one */
  new_pane = zmapAddPane(zmap, 'v');

  /* focus on the new pane */
  zmapRecordFocus(new_pane) ;

  zmapControlWindowSetZoomButtons(zmap, ZMAP_ZOOM_MID);

  gtk_widget_set_sensitive(zmap->close_but, TRUE);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* resize all the panes to fit the width */
  resizeHPanes(zmap);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return new_pane->view_parent_box ;
}




void closePane(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;
  GNode *node = NULL;
  GNode *next = NULL;
  GNode *parent = NULL;
  GtkWidget *pane_up1, *pane_up2, *frame = NULL;
  GList *children = NULL;


  /* count panes but forget the root node which isn't one */
  guint panes = g_node_n_nodes(zmap->panesTree, G_TRAVERSE_ALL) - 1;

  if (panes == 1)
    return;
  else
    {
      /* now decide where to focus after closing the pane */
      node = g_node_find(zmap->panesTree, 
			 G_IN_ORDER,
			 G_TRAVERSE_ALL,
			 zmap->focuspane) ;
      if (node->prev)
	next = node->prev;
      else if (node->next)
	next = node->next;
      else if (node->children)
	next = node->children;
      else if (node->parent)
	next = node->parent;
      else
	{
	  printf("Don't know where to focus now!\n");
	  parent = g_node_get_root(zmap->panesTree);
	  next = parent->children;
	}

      exciseNode(node);

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
      gtk_widget_destroy(zmap->focuspane->view_parent_box);
      gtk_widget_destroy(zmap->focuspane->frame);
      free(zmap->focuspane) ;
            
      /* In theory, by this point all redundant panes and frames have been destroyed.
       * In reality, I have seen 'hanging' panes (grey squares of window lacking
       * a canvas and scroll bars) after closing multiply-split windows, but life 
       * is short! */
      zmapRecordFocus(next->data);
      gtk_widget_grab_focus(((ZMapPane)next->data)->frame);

      
      resizePanes(zmap);                                                                                
    }

  if (panes == 2)  /* that is, there were 2 panes until we just closed one */
    gtk_widget_set_sensitive(zmap->close_but, FALSE);

  return ;
}




void zmapRecordFocus(ZMapPane pane)
{
  ZMap zmap = pane->zmap ;
  GNode *node;
  ZMapWindow window;
  ZMapWindowZoomStatus zoom_status;
  GdkColor color;
 
  /* point the parent window's focuspane pointer at this pane */
  if (pane)
    zmap->focuspane = pane ;
  else /* if pane is null, arbitrarily focus on the first valid pane */
    {
      node = g_node_first_child(zmap->panesTree) ;
      zmap->focuspane = node->data ;
    }
  
  g_node_traverse(zmap->panesTree, 
		  G_IN_ORDER,
		  G_TRAVERSE_ALL, 
		  -1,
		  unfocus, 
		  NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(pane->frame), GTK_SHADOW_IN);

  gdk_color_parse("dark blue", &color);
  gtk_widget_modify_bg(GTK_WIDGET(pane->frame), GTK_STATE_NORMAL, &color);

  /* make sure the zoom buttons are appropriately sensitised for this window. */
  if (pane->curr_view_window)
    {
      window = zMapViewGetWindow(pane->curr_view_window);
      zoom_status = zMapWindowGetZoomStatus(window);
      zmapControlWindowSetZoomButtons(zmap, zoom_status);
    }

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

      gdk_color_parse("dark grey", &color);
      gtk_widget_modify_bg(GTK_WIDGET(pane->frame), GTK_STATE_NORMAL, &color);
    }
  return 0;  
}



/* shrinkPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new pane and frame and reposition
 * the scrolled window into that.  Later on we resize all panes. */
static void shrinkPane(ZMap zmap)
{
  ZMapPane pane ;
  GNode *node = NULL;
  GdkColor color;

  /* create a new vpane and hook the old scrolled window up to it */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;
  pane->pane = gtk_vpaned_new() ;
  pane->frame = gtk_frame_new(NULL) ; 

  gdk_color_parse("dark grey", &color);
  gtk_widget_modify_bg(GTK_WIDGET(zmap->focuspane->frame), GTK_STATE_NORMAL, &color);

  /* I think this is correct ??? */
  pane->view_parent_box = zmap->focuspane->view_parent_box ;

  pane->curr_view_window = zmap->focuspane->curr_view_window ;


  /* reparent the scrolled window into the new frame & pack1 the frame into the new vpane,
  ** then add the new vpane to the frame of the pane which currently has focus, ie that being split.  */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->scrolledWindow), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->view_parent_box), pane->frame);

  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(zmap->focuspane->frame), pane->pane);

  /* remove the old ZMapPane from panesTree, and add the new one */
  node = g_node_find(zmap->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     (gpointer)zmap->focuspane) ;
  g_node_append_data(node->parent, pane);

  exciseNode(node);

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


/* shrinkHPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new, half-size one and reposition
 * the scrolled window into that */
static void shrinkHPane(ZMap zmap)
{
  ZMapPane pane ;
  GNode *node = NULL;

  /* create a new hpane and hook the old scrolled window up to it */
  pane = g_new0(ZMapPaneStruct, 1) ;
  pane->zmap = zmap ;
  pane->pane = gtk_hpaned_new() ;
  pane->frame = gtk_frame_new(NULL) ;

  /* I think this is correct ??? */
  pane->view_parent_box = zmap->focuspane->view_parent_box ;

  pane->curr_view_window = zmap->focuspane->curr_view_window ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->scrolledWindow), pane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  gtk_widget_reparent(GTK_WIDGET(zmap->focuspane->view_parent_box), pane->frame);

  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(zmap->focuspane->frame), pane->pane);

  /* add new pane to windowsTree. */
  node = g_node_find(zmap->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     zmap->focuspane);
  g_node_append_data(node->parent, pane);

  exciseNode(node);

  /* do we really need this? */  
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gtk_widget_hide(zmap->focuspane->frame);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  zmap->focuspane = pane ;

  return;
}



static void exciseNode(GNode *node)
{
  GNode *parent = NULL;
  GNode *children = NULL;

  /* excising the node from the tree means first
   * finding the parent to hang any children from,
   * then unlinking either side of the redundant
   * node and finally hooking everything back up. 
   * First, decide who gets to be daddy. */
  if (node->prev)
    parent = node->prev;
  else if (node->next)
    parent = node->next;
  else
    parent = node->parent;

  g_node_unlink(node);
  
  /* when you unlink a node, if its children have siblings,
   * they become its children, so you have to append them
   * too.  Since this could be any depth, you have to loop.  */
  while (node->children)
    {
      children = node->children;
      g_node_unlink(node->children);
      g_node_append(parent, children);
    }
  
  g_node_destroy(node);

  return;
}


/********************* end of file ********************************/
