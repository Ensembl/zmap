/*  Last edited: Apr 13 15:01 2004 (rnc) */
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
 
#include <zmapsplit.h>

/* function prototypes **************************************************/

static void  shrinkPane      (ZMapWindow *window);
static void  resizePanes     (ZMapWindow *window);
static void  resizeOnePane   (GNode *node, gpointer user_data);
static void  scaleCanvas     (ZMapWindow *window, double zoomFactor);
static void  shrinkHPane     (ZMapWindow *window);
static void  resizeHPanes    (ZMapWindow *window);
static int   unfocus         (GNode *node, gpointer data);
static int   resizeOneHPane  (GNode *node, gpointer data);
static void  exciseNode      (GNode *node);

/* end of prototypes ***************************************************/

/* I can't make up my mind what to do about the mix of static/non-static
 * functions in this file.  Normally I'd have static in one file and non
 * in another, or at least group them within the file, but in this case
 * I have the zooming/scaling functions together, and the focus/unfocus
 * ones together, which seems logical, so what to do? 
 */

/* Zooming: we don't zoom as such, we scale the group, doubling the
 * y axis but leaving x at 1.  This enlarges lengthways, while keeping
 * the width the same.  I'm leaving the zoomFactor in the ZMapPane
 * structure for now in case I need it for when the user specifies
 * the bases-per-line magnification, which I've not looked at yet. */
static void scaleCanvas(ZMapWindow *window, double zoomFactor)
{
  double matrix[6];
  
  art_affine_scale(matrix, 1.0, zoomFactor);
  gnome_canvas_item_affine_relative(window->focuspane->group, matrix);
  return;
}


void zoomIn(GtkWindow *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;

  window->focuspane->zoomFactor *= 2;
  scaleCanvas(window, 2.0);
  return;
}


void zoomOut(GtkWindow *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;

  window->focuspane->zoomFactor *= 0.5;
  scaleCanvas(window, 0.5);
  return;
}


/* we probably won't do it like this in zmap, but for the purposes
 * of the prototype it's helpful to have unfocussed backgrounds grey */
static int unfocus(GNode *node, gpointer data)
{
  ZMapPane *pane = (ZMapPane*)node->data;

  if (pane) /* skip the actual root node which is not a valid widget */
    {
      gtk_frame_set_shadow_type(GTK_FRAME(pane->frame), GTK_SHADOW_NONE);

      gnome_canvas_item_set(pane->background,
			    "fill_color", "light grey",
			    NULL);
    }
  return 0;  
}

int recordFocus(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  ZMapPane *pane = (ZMapPane*)data;
  ZMapWindow *window = pane->window;
  GNode *node;
 
  /* point the parent window's focuspane pointer at this pane */
  if (pane)
    window->focuspane = pane;
  else /* if pane is null, arbitrarily focus on the first valid pane */
    {
      node = g_node_first_child(window->panesTree);
      window->focuspane = node->data;
    }
  
  g_node_traverse(window->panesTree, 
		  G_IN_ORDER,
		  G_TRAVERSE_ALL, 
		  -1,
		  unfocus, 
		  NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(pane->frame), GTK_SHADOW_IN);
  gnome_canvas_item_set(pane->background,
			"fill_color", "white",
			NULL);
  return 0;
}


/* shrinkPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new pane and frame and reposition
 * the scrolled window into that.  Later on we resize all panes. */
static void shrinkPane(ZMapWindow *window)
{
  ZMapPane *pane = (ZMapPane*)malloc(sizeof(ZMapPane));
  GNode *node = NULL;

  /* create a new vpane and hook the old scrolled window up to it */
  pane->window = window;
  pane->pane         = gtk_vpaned_new();
  pane->frame        = gtk_frame_new(NULL);;
  pane->scrolledWindow      = window->focuspane->scrolledWindow;
  pane->canvas       = window->focuspane->canvas;
  pane->background   = window->focuspane->background;
  
  /* when the user clicks a button in the canvas, call recordFocus() */
  g_signal_connect (GTK_OBJECT (pane->canvas), "button_press_event",
		    GTK_SIGNAL_FUNC (recordFocus), pane); 

  gtk_widget_reparent(GTK_WIDGET(window->focuspane->scrolledWindow), pane->frame);
  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(window->focuspane->frame), pane->pane);

  /* remove the old ZMapPane from windowsTree, and add the new one */
  node = g_node_find(window->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     (gpointer)window->focuspane);

  g_node_append_data(node->parent, pane);
  exciseNode(node);

  /* do we really need this? */
  gtk_widget_hide(window->focuspane->frame);
  window->focuspane = pane;

  return;
}


void splitPane(GtkWidget *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;

  /* shrink the old pane */
  shrinkPane(window);
  /* add a new pane below to the old one */
  addPane   (window, 'h');
  /* resize all the panes to fit the height */
  resizePanes(window);

  return;
}

/* resizing the panes doesn't really work too well, but it's not too
 * bad for a few panes and time is short. */
static void resizePanes(ZMapWindow *window)
{
  GtkRequisition req;
  GNode *node = NULL;
  GNode *last = NULL;
  guint panes = 0;

  /* count how many panes there are in this column */
  node = g_node_find(window->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     window->focuspane);

  last =  g_node_last_child(node->parent);
  /* first child is number 0 */
  panes = g_node_child_position(node->parent,last);

  if (panes == 0)
    panes = 1;

  /* get size of the window & divide it by the number of panes.
   * Note there's a small kludge here to adjust for a better fit.
   * Don't know why it needs the -8. */
  gtk_widget_size_request(GTK_WIDGET(window->vbox1), &req);
  req.height = (req.height/panes) - 8;

  g_node_children_foreach(window->panesTree, 
		  G_TRAVERSE_LEAFS, 
		  resizeOnePane, 
		  &req);
  return;
}


static void resizeOnePane(GNode *node, gpointer data)
{
  ZMapPane *pane = (ZMapPane*)node->data;
  GtkRequisition *req = (GtkRequisition*)data;

  if (pane)  
    gtk_widget_set_size_request (GTK_WIDGET (pane->frame), req->width, req->height);

  return;
}  


/* shrinkHPane is not quite the right name for this function.  We don't
 * actually shrink the pane, we add a new, half-size one and reposition
 * the scrolled window into that */
static void shrinkHPane(ZMapWindow *window)
{
  ZMapPane *pane = (ZMapPane*)malloc(sizeof(ZMapPane));
  GNode *node = NULL;

  /* create a new hpane and hook the old scrolled window up to it */
  pane->window = window;
  pane->pane         = gtk_hpaned_new();
  pane->frame        = gtk_frame_new(NULL);
  pane->scrolledWindow      = window->focuspane->scrolledWindow;
  pane->canvas       = window->focuspane->canvas;
  pane->background   = window->focuspane->background;
  
  /* when the user clicks a button in the canvas, call recordFocus() */
  g_signal_connect (GTK_OBJECT (pane->canvas), "button_press_event",
		    GTK_SIGNAL_FUNC (recordFocus), pane); 

  gtk_widget_reparent(GTK_WIDGET(window->focuspane->scrolledWindow), pane->frame);
  gtk_paned_pack1(GTK_PANED(pane->pane), pane->frame, TRUE, TRUE);
  gtk_container_add(GTK_CONTAINER(window->focuspane->frame), pane->pane);

  /* add new pane to windowsTree. */
  node = g_node_find(window->panesTree,
		     G_IN_ORDER,
		     G_TRAVERSE_ALL,
		     window->focuspane);
  g_node_append_data(node->parent, pane);
  exciseNode(node);

  /* do we really need this? */  
  gtk_widget_hide(window->focuspane->frame);
  window->focuspane = pane;

  return;
}


void splitHPane(GtkWidget *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;

  /* slim the old pane */
  shrinkHPane(window);
  /* add a new pane next to the old one */
  addPane   (window, 'v');
  /* resize all the panes to fit the width */
  resizeHPanes(window);

  return;
}


/* This wrapper for resizeOnePane is needed because
 * the callback function for g_node_traverse() is
 * required to return a BOOL, while that for
 * g_node_children_foreach() is not. */
static int resizeOneHPane(GNode *node, gpointer data)
{
  resizeOnePane(node, data);
  return 1;
}  


static void resizeHPanes(ZMapWindow *window)
{
  GtkRequisition req;
  guint panes = 0;

  /* count how many columns there are */
  panes = g_node_n_nodes(window->panesTree, 
			G_TRAVERSE_NON_LEAFS);
  if (panes == 0)
    panes = 1;

  /* get size of the window & divide it by the number of panes.
   * Note there's a small kludge here to adjust for a better fit.
   * Don't know why it needs the -8. */
  gtk_widget_size_request(GTK_WIDGET(window->zoomvbox), &req);
  req.width = (req.width/panes) - 8;

  g_node_traverse(g_node_get_root(window->panesTree), 
		  G_IN_ORDER,
		  G_TRAVERSE_NON_LEAFS, 
		  -1,
		  resizeOneHPane, 
		  &req);
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


void closePane(GtkWidget *widget, gpointer data)
{
  ZMapWindow *window = (ZMapWindow*)data;
  GNode *node = NULL;
  GNode *next = NULL;
  GNode *parent = NULL;
  GtkWidget *pane_up1, *pane_up2, *frame = NULL;
  GList *children = NULL;


  /* count panes but forget the root node which isn't one */
  guint panes = g_node_n_nodes(window->panesTree, G_TRAVERSE_ALL) - 1;
  int discard;  

  if (panes == 1)
      Quit(NULL, window);
  else
    {
      /* now decide where to focus after closing the pane */
      node = g_node_find(window->panesTree, 
			 G_IN_ORDER,
			 G_TRAVERSE_ALL,
			 window->focuspane);
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
	  parent = g_node_get_root(window->panesTree);
	  next = parent->children;
	}

      exciseNode(node);

      /* This is cumbersome! The problem is that each time we split the window,
      * we add a pane to the current frame, then delete the corresponding node
      * from the panesTree.  When we delete panes from the display, we also need
      * to remove these extra frames as they become empty. */
      pane_up1 = gtk_widget_get_ancestor(window->focuspane->frame, GTK_TYPE_PANED);
      frame    = gtk_widget_get_ancestor(pane_up1, GTK_TYPE_FRAME);
      pane_up2 = gtk_widget_get_ancestor(frame, GTK_TYPE_PANED);

      gtk_object_destroy(GTK_OBJECT(window->focuspane->canvas));
      gtk_widget_destroy(window->focuspane->scrolledWindow);
      gtk_widget_destroy(window->focuspane->frame);
      free(window->focuspane);
            
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

      /* In theory, by this point all redundant panes and frames have been destroyed.
       * In reality, I have seen 'hanging' panes (grey squares of window lacking
       * a canvas and scroll bars) after closing multiply-split windows, but life 
       * is short! */
      discard = recordFocus(NULL, NULL, next->data);
      gtk_widget_grab_focus(((ZMapPane*)next->data)->frame);
      
      resizePanes(window);                                                                                
    }
  return;
}

/********************* end of file ********************************/
