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
 * Last edited: May 17 15:12 2004 (edgrif)
 * Created: Thu Jul 24 14:36:37 2003 (edgrif)
 * CVS info:   $Id: zmapAppconnect.c,v 1.8 2004-05-17 16:24:16 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapApp_P.h>


static void createThreadCB(GtkWidget *widget, gpointer data) ;



GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entry, *label, *create_button ;

  frame = gtk_frame_new( "New ZMap" );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), topbox);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Sequence:" ) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0) ;

  app_context->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0) ;

  create_button = gtk_button_new_with_label("Create ZMap") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
		     GTK_SIGNAL_FUNC(createThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(topbox), create_button, FALSE, FALSE, 0) ;

  /* set create button as default. */
  GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(app_context->app_widg), create_button) ;


  return frame ;
}



static void createThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  char *sequence ;
  char *row_text[ZMAP_NUM_COLS] = {"", "", ""} ;
  int row ;
  ZMap zmap ;

  sequence = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->sequence_widg)) ;
  if (sequence && strlen(sequence) == 0)		    /* gtk_entry returns "" for "no text". */
    sequence = NULL ;


  if (!zMapManagerAdd(app_context->zmap_manager, sequence, &zmap))
    {
      zMapWarning("%s", "Failed to create ZMap") ;
    }
  else
    {
      row_text[0] = zMapGetZMapID(zmap) ;

      /* awaiting new call to report some other way of showing what could be multiple sequences
       * per window..... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      row_text[1] = zMapGetSequence(zmap) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      row_text[1] = "<dummy>" ;

      row_text[2] = zMapGetZMapStatus(zmap) ;
      row_text[3] = "blah, blah, blah" ;

      
      row = gtk_clist_append(GTK_CLIST(app_context->clist_widg), row_text) ;
      gtk_clist_set_row_data(GTK_CLIST(app_context->clist_widg), row, (gpointer)zmap) ;
      

      zMapDebug("GUI: create thread number %d for zmap \"%s\" for sequence \"%s\"\n",
		(row + 1), row_text[0], row_text[1]) ;
    }

  return ;
}


