/*  File: zmapappmanage.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Top level of application, creates ZMaps.
 *              
 * Exported functions: None, all functions internal to zmapApp.
 * HISTORY:
 * Last edited: May 17 15:20 2004 (edgrif)
 * Created: Thu Jul 24 14:36:47 2003 (edgrif)
 * CVS info:   $Id: zmapAppmanage.c,v 1.8 2004-05-17 16:25:43 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <ZMap/zmapUtils.h>
#include <zmapApp_P.h>


static void stopThreadCB(GtkWidget *widget, gpointer cb_data) ;
static void killThreadCB(GtkWidget *widget, gpointer data) ;
static void checkThreadCB(GtkWidget *widget, gpointer cb_data) ;
static void selectRow(GtkCList *clist, gint row, gint column, GdkEventButton *event,
		      gpointer cb_data) ;
static void unselectRow(GtkCList *clist, gint row, gint column, GdkEventButton *event,
			gpointer cb_data) ;


static char *column_titles[ZMAP_NUM_COLS] = {"ZMap", "Sequence", "Status", "Last Request"} ;



GtkWidget *zmapMainMakeManage(ZMapAppContext app_context)
{
  GtkWidget *frame ;
  GtkWidget *vbox, *scrwin, *clist, *hbox, *stop_button, *kill_button, *check_button ;

  frame = gtk_frame_new("Manage ZMaps") ;
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 ) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;
  
  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(vbox), 5);
  gtk_container_add(GTK_CONTAINER (frame), vbox);

  scrwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC) ;
  gtk_widget_set_usize(scrwin, 600, 150) ;
  gtk_box_pack_start(GTK_BOX(vbox), scrwin, FALSE, FALSE, 0) ;



  /* Apparently gtkclist is now deprecated, not sure what one uses instead....sigh... */
  app_context->clist_widg = clist = gtk_clist_new_with_titles(4, column_titles) ;
  gtk_signal_connect(GTK_OBJECT(clist), "select_row",
		     GTK_SIGNAL_FUNC(selectRow), (gpointer)app_context) ;
  gtk_signal_connect(GTK_OBJECT(clist), "unselect_row",
		     GTK_SIGNAL_FUNC(unselectRow), (gpointer)app_context) ;
  gtk_container_add (GTK_CONTAINER (scrwin), clist);
  /* Sizing is all a hack, could do with much improvement. */
  gtk_clist_set_column_width(GTK_CLIST(clist), 1, 100) ;
  gtk_clist_set_column_width(GTK_CLIST(clist), 2, 150) ;
  gtk_clist_set_column_width(GTK_CLIST(clist), 3, 150) ;


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


static void stopThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  int row ;

  if (app_context->selected_zmap)
    {
      zMapManagerReset(app_context->selected_zmap) ;
    }

  return ;
}

static void killThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  int row ;

  if (app_context->selected_zmap)
    {
      row = gtk_clist_find_row_from_data(GTK_CLIST(app_context->clist_widg),
					 app_context->selected_zmap) ;

      zMapDebug("GUI: kill thread for row %d with connection pointer: %x\n",
		 row, app_context->selected_zmap) ;

      zMapManagerKill(app_context->zmap_manager, app_context->selected_zmap) ;
    }


  return ;
}


/* Does not work yet... */
static void checkThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  printf("not implemented yet\n") ;

  return ;
}


static void selectRow(GtkCList *clist, gint row, gint column, GdkEventButton *event,
		      gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  app_context->selected_zmap = (ZMap)gtk_clist_get_row_data(clist, row) ;

  return ;
}

static void unselectRow(GtkCList *clist, gint row, gint column, GdkEventButton *event,
			gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;

  app_context->selected_zmap = NULL ;

  return ;
}
