/*  File: zmapappconnect.c
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 12 15:06 2003 (edgrif)
 * Created: Thu Jul 24 14:36:37 2003 (edgrif)
 * CVS info:   $Id: zmapAppconnect.c,v 1.1 2003-11-13 14:58:39 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <zmapApp_P.h>


static void createThreadCB(GtkWidget *widget, gpointer data) ;



GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entry, *label, *create_button ;

  frame = gtk_frame_new( "New Connection" );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), topbox);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Machine:" ) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0) ;

  app_context->machine_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Port:" ) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0) ;

  app_context->port_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Sequence:" ) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0) ;

  app_context->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0) ;

  create_button = gtk_button_new_with_label("Create Thread") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
		     GTK_SIGNAL_FUNC(createThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(topbox), create_button, FALSE, FALSE, 0) ;
  GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
  gtk_widget_grab_default(create_button) ;


  return frame ;
}



static void createThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  char *machine ;
  char *port_str ;
  int port ;
  char *sequence ;
  char *row_text[ZMAP_NUM_COLS] = {"", "", ""} ;
  int row ;
  ZMapWinConn zmapconn ;

  machine = gtk_entry_get_text(GTK_ENTRY(app_context->machine_widg)) ;
  port_str = gtk_entry_get_text(GTK_ENTRY(app_context->port_widg)) ;
  port = atoi(port_str) ;
  sequence = gtk_entry_get_text(GTK_ENTRY(app_context->sequence_widg)) ;


  zmapconn = zMapManagerAdd(app_context->zmap_manager, machine, port, sequence) ;
  /* ERROR HANDLING ETC.... */


  row_text[0] = machine ;
  row_text[1] = port_str ;
  row_text[2] = sequence ;

  row = gtk_clist_append(GTK_CLIST(app_context->clist_widg), row_text) ;
  gtk_clist_set_row_data(GTK_CLIST(app_context->clist_widg), row, (gpointer)zmapconn) ;


  ZMAP_DEBUG(("GUI: create thread number %d to access: %s on port %d\n",
		  (row + 1), machine ? machine : "localhost", port)) ;

  return ;
}


