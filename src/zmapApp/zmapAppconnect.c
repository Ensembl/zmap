/*  Last edited: Feb 14 16:33 2012 (edgrif) */
/*  File: zmapappconnect.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *          Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See zmapApp_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>



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


/* Moving code to manager so need to somehow make sure that we signal app back so it
 * can update it's display...... */

/* Make a new zmap, if sequence is unspecified then the zmap will be blank and view
 * will not be set. */
gboolean zmapAppCreateZMap(ZMapAppContext app_context, ZMapFeatureSequenceMap sequence_map,
			   ZMap *zmap_inout, ZMapView *view_out, char **err_msg_out)
{
  gboolean result = FALSE ;
  ZMap zmap = *zmap_inout ;
  ZMapView view = NULL ;
  GtkTreeIter iter1;
  ZMapManagerAddResult add_result ;


  if (sequence_map->sequence && (sequence_map->start < 1 || sequence_map->end < sequence_map->start))
    {
      *err_msg_out = g_strdup_printf("Bad start/end coords: %d, %d", sequence_map->start, sequence_map->end) ;

      zMapWarning("%s", *err_msg_out) ;
    }
  else
    {
      /* Set the text.  This is done even if it's already set, ah well! */
      if (sequence_map->sequence)
	gtk_entry_set_text(GTK_ENTRY(app_context->sequence_widg), sequence_map->sequence);


      add_result = zMapManagerAdd(app_context->zmap_manager, sequence_map, &zmap, &view) ;

      if (add_result == ZMAPMANAGER_ADD_DISASTER)
	{
	  *err_msg_out = g_strdup_printf("%s", "Failed to create ZMap and then failed to clean up properly,"
				     " save your work and exit now !") ;

	  zMapWarning("%s", *err_msg_out) ;
	}
      else if (add_result == ZMAPMANAGER_ADD_FAIL)
	{
	  *err_msg_out = g_strdup_printf("%s", "Failed to create ZMap") ;

	  zMapWarning("%s", *err_msg_out) ;
	}
      else
	{
	  /* agh....ok this not connected bit needs sorting out..... */


	  /* If we tried to load a sequence but couldn't connect then warn user, otherwise
	   * we just created the requested blank zmap. */
	  if (sequence_map->sequence && add_result == ZMAPMANAGER_ADD_NOTCONNECTED)
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

	  *zmap_inout = zmap ;

	  if (view)
	    *view_out = view ;

	  result = TRUE ;
	}
    }

  return result ;
}





/*
 *         Internal routines.
 */

static void createThreadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  char *sequence, *start_txt, *end_txt ;
  int start = 0, end = 0 ;
  ZMapFeatureSequenceMap seq_map ;
  char *err_msg = NULL ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;

  /* Note: gtk_entry returns "" for "no text" so must test contents of string returned,
   * zMapStr2Int() only sets integer if string is valid. */

  if (!(sequence = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->sequence_widg))))
    sequence = NULL ;

  if ((start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->start_widg))))
    zMapStr2Int(start_txt, &start) ;

  if ((end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(app_context->end_widg))))
    zMapStr2Int(end_txt, &end) ;

  /* MH17: this is a bodge FTM, we need a dataset widget as well */
  seq_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  seq_map->dataset = app_context->default_sequence->dataset;
  seq_map->sequence = sequence;
  seq_map->start = start;
  seq_map->end = end;

  /* N.B. currently the user is allowed to create a blank zmap, may need to revisit this...
   * 
   * Note also that we discard the returned zmap. Some time we may need to record it
   * for xremote purposes.
   *  */
  if (!zmapAppCreateZMap(app_context, seq_map, &zmap, &view, &err_msg))
    zMapWarning("%s", err_msg) ;


  return ;
}

