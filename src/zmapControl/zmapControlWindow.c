/*  File: zmapTopWindow.c
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
 * Description: Creates the top level window of a ZMap.
 *              
 * Exported functions: See zmapTopWindow_P.h
 * HISTORY:
 * Last edited: May 27 12:19 2004 (edgrif)
 * Created: Fri May  7 14:43:28 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindow.c,v 1.3 2004-05-27 13:40:36 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap_P.h>


static void quitCB(GtkWidget *widget, gpointer cb_data) ;
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;



gboolean zmapControlWindowCreate(ZMap zmap, char *zmap_id)
{
  gboolean result = TRUE ;
  GtkWidget *toplevel, *vbox, *menubar, *button_frame, *connect_frame ;
  char *title ;

  title = g_strdup_printf("ZMap - %s", zmap_id) ;

  zmap->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)zmap) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapControlWindowMakeMenuBar(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  button_frame = zmapControlWindowMakeButtons(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_frame, FALSE, TRUE, 0);

  zmap->view_parent = connect_frame = zmapControlWindowMakeFrame(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;


  g_free(title) ;


  return result ;
}





void zmapControlWindowDestroy(ZMap zmap)
{
  /* We must disconnect the "destroy" callback otherwise we will enter quitCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(zmap->toplevel), (gpointer)zmap) ;

  gtk_widget_destroy(zmap->toplevel) ;

  return ;
}






/*
 *  ------------------- Internal functions -------------------
 */


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlTopLevelKillCB(zmap) ;

  return ;
}

