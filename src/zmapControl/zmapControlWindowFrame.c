/*  File: zmapWindowFrame.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jan  7 12:17 2005 (edgrif)
 * Created: Thu Apr 29 11:06:06 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindowFrame.c,v 1.14 2005-01-07 12:17:41 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapControl_P.h>


static void createNavViewWindow(ZMap zmap, GtkWidget *parent) ;

static void valueCB(void *user_data, double start, double end) ;


GtkWidget *zmapControlWindowMakeFrame(ZMap zmap)
{
  GtkWidget *frame ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  createNavViewWindow(zmap, frame) ;

  zmapControlNavigatorSetWindowCallback(zmap->navigator, valueCB, (void *)zmap) ;

  return frame ;
}



/* createNavViewWindow ***************************************************************
 * Creates the root node in the panesTree (which helps keep track of all the
 * display panels).  The root node has no data, only children.
 */
static void createNavViewWindow(ZMap zmap, GtkWidget *parent)
{
  GtkWidget *nav_top ;

  zmap->panesTree = g_node_new(NULL) ;

  /* Navigator and views are in an hpane, so the user can adjust the width
   * of the navigator and views. */
  zmap->hpane = gtk_hpaned_new() ;
  gtk_container_add(GTK_CONTAINER(parent), zmap->hpane) ;


  /* Make the navigator which shows user where they are on the sequence. */
  zmap->navigator = zmapControlNavigatorCreate(&nav_top) ;
  gtk_paned_pack1(GTK_PANED(zmap->hpane), nav_top, FALSE, TRUE) ;


  /* I'm not sure we actually need this intervening box, we could probably just set the size of
   * the hpane.... */
  zmap->pane_vbox = gtk_vbox_new(FALSE,0) ;

  gtk_paned_pack2(GTK_PANED(zmap->hpane), 
		  zmap->pane_vbox, TRUE, TRUE);

  gtk_widget_show_all(zmap->hpane);

  return ;
}


/* Gets called by navigator when user has moved window locator scroll bar. */
static void valueCB(void *user_data, double start, double end)
{
  ZMap zmap = (ZMap)user_data ;

  if (zmap->state == ZMAP_VIEWS)
    {
      ZMapWindow window = zMapViewGetWindow(zmap->focuspane->curr_view_window) ;

      zMapWindowMove(window, start, end) ;
    }  

  return ;
}
