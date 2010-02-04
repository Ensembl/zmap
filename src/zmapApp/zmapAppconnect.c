/*  File: zmapappconnect.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Description: 
 * Exported functions: See zmapApp_P.h
 * HISTORY:
 * Last edited: Feb  4 16:20 2010 (edgrif)
 * Created: Thu Jul 24 14:36:37 2003 (edgrif)
 * CVS info:   $Id: zmapAppconnect.c,v 1.25 2010-02-04 16:57:32 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapApp_P.h>


/* dummychange */

static void createThreadCB(GtkWidget *widget, gpointer data) ;



GtkWidget *zmapMainMakeConnect(ZMapAppContext app_context)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *create_button ;

  frame = gtk_frame_new( "New ZMap" );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);


  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;


  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;


  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Sequence :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start :" ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;

  label = gtk_label_new( "End :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;



  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  app_context->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  app_context->start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  app_context->end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;


  create_button = gtk_button_new_with_label("Create ZMap") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
		     GTK_SIGNAL_FUNC(createThreadCB), (gpointer)app_context) ;
  gtk_box_pack_start(GTK_BOX(topbox), create_button, FALSE, TRUE, 0) ;


  /* set create button as default. */
  GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(app_context->app_widg), create_button) ;


  return frame ;
}


/* sequence etc can be unspecified to create a blank zmap. */
void zmapAppCreateZMap(ZMapAppContext app_context, char *sequence, int start, int end)
{
  ZMap zmap ;
  GtkTreeIter iter1;
  ZMapManagerAddResult add_result ;

  /* Set the text.  This is done even if it's already set, ah well! */
  if (sequence)
    gtk_entry_set_text(GTK_ENTRY(app_context->sequence_widg), sequence);

  add_result = zMapManagerAdd(app_context->zmap_manager, sequence, start, end, &zmap) ;
  if (add_result == ZMAPMANAGER_ADD_DISASTER)
    {
      zMapWarning("%s", "Failed to create ZMap and then failed to clean up properly,"
                  " save your work and exit now !") ;
    }
  else if (add_result == ZMAPMANAGER_ADD_FAIL)
    {
      zMapWarning("%s", "Failed to create ZMap") ;
    }
  else
    {
      /* If we tried to load a sequence but couldn't connect then warn user, otherwise
       * we just created the requested blank zmap. */
      if (sequence && add_result == ZMAPMANAGER_ADD_NOTCONNECTED)
	zMapWarning("%s", "ZMap added but could not connect to server, try \"Reload\".") ;

      gtk_tree_store_append (app_context->tree_store_widg, &iter1, NULL);
      gtk_tree_store_set (app_context->tree_store_widg, &iter1,
                          ZMAPID_COLUMN, zMapGetZMapID(zmap),
                          ZMAPSEQUENCE_COLUMN,"<dummy>" ,
                          ZMAPSTATE_COLUMN, zMapGetZMapStatus(zmap),
                          ZMAPLASTREQUEST_COLUMN, "blah, blah, blaaaaaa",
                          ZMAPDATA_COLUMN, (gpointer)zmap,
                          -1);
#ifdef RDS_NEVER_INCLUDE_THIS_CODE      
      zMapDebug("GUI: create thread number %d for zmap \"%s\" for sequence \"%s\"\n",
                (row + 1), row_text[0], row_text[1]) ;
#endif /* RDS_NEVER_INCLUDE_THIS_CODE */
    }

  return ;
}





/* 
 *         Internal routines. 
 */

static void createThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  char *sequence = "", *start_txt, *end_txt ;
  int start = 1, end = 0 ;

  sequence = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->sequence_widg)) ;
  if (sequence && strlen(sequence) == 0)		    /* gtk_entry returns "" for "no text". */
    sequence = NULL ;

  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->start_widg)) ;
  if ((start_txt && strlen(start_txt) >= 1))		    /* gtk_entry returns "" for "no text". */
    {
      if (!zMapStr2Int(start_txt, &start))
	start = 1 ;
    }
  else
    start = 1 ;


  end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->end_widg)) ;
  if ((end_txt && strlen(end_txt) >= 1))		    /* gtk_entry returns "" for "no text". */
    {
      if (!zMapStr2Int(end_txt, &end))
	end = 0 ;
    }
  else
    end = 0 ;


  /* N.B. currently the user is allowed to create a blank zmap, may need to revisit this... */
  zmapAppCreateZMap(app_context, sequence, start, end) ;



  return ;
}


