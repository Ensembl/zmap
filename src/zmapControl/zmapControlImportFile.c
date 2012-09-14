/*  File: zmapControlImportFile.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Posts a dialog for user to enter sequence, start/end
 *              and optionally a config file for the sequence. Once
 *              user has chosen then this code calls the function
 *              provided by the caller to get the sequence displayed.
 *
 * Exported functions: See ZMap/zmapControlImportFile.h
 *
 * NOTE this file was initially copied from zmapAppSequenceView.c
 * and then tweaked. Ther emay be some common code & functions
 * but maybe this file will be volatile for a while
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStanzaStructs.h>
#include <zmapControl_P.h>
#include <ZMap/zmapControlImportFile.h>




/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;
  GtkWidget *file_widg ;

  GtkWidget *map_widg ;
  GtkWidget *offset_widg ;

  ZMapControlImportFileCB user_func ;
  gpointer user_data ;
} MainFrameStruct, *MainFrame ;



static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
			    ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame);
static GtkWidget *makeButtonBox(MainFrame main_frame) ;

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(GtkWidget *widget, gpointer cb_data) ;
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
static void closeCB(GtkWidget *widget, gpointer cb_data) ;





/* Display a dialog to get from the reader a file to be displayed
 * with a optional start/end and various mapping parameters
 */
void zMapControlImportFile(ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *toplevel, *container ;
  gpointer seq_data = NULL ;

  zMapAssert(user_func) ;

  toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "Please choose a file to import.") ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  container = makePanel(toplevel, &seq_data, user_func, user_data, sequence_map) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
		     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  return ;
}




/*
 *                   Internal routines.
 */


/* Make the whole panel returning the top container of the panel. */
static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *our_data,
			    ZMapControlImportFileCB user_func, gpointer user_data,
			    ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box, *options_box;
  MainFrame main_data ;

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->user_func = user_func ;
  main_data->user_data = user_data ;

  if (toplevel)
    {
      main_data->toplevel = toplevel ;
      *our_data = main_data ;
    }

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  main_frame = makeMainFrame(main_data, sequence_map) ;
  gtk_box_pack_start(GTK_BOX(vbox), main_frame, TRUE, TRUE, 0) ;

  options_box = makeOptionsBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, TRUE, TRUE, 0) ;

  button_box = makeButtonBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, TRUE, 0) ;

  return frame ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;
  char *sequence = "", *start = "", *end = "", *file = "" ;

  if (sequence_map)
    {
      if (sequence_map->sequence)
	sequence = sequence_map->sequence ;
      if (sequence_map->start)
	start = g_strdup_printf("%d", sequence_map->start) ;
      if (sequence_map->end)
	end = g_strdup_printf("%d", sequence_map->end) ;
    }

  frame = gtk_frame_new( "Sequence: " );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;


  /* Labels..... */
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

  label = gtk_label_new( "File :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), start) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), end) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->file_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), file) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  /* Free resources. */
  if (sequence_map)
    {
      if (*start)
	g_free(start) ;
      if (*end)
	g_free(end) ;
    }


  return frame ;
}


/* Make the option buttons frame. */
static GtkWidget *makeOptionsBox(MainFrame main_frame)
{
  GtkWidget *frame ;
  GtkWidget *options_box, *map_seq_button, *entry, *label ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  options_box = gtk_hbutton_box_new() ;
  gtk_container_border_width(GTK_CONTAINER(options_box), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), options_box) ;


  main_frame->map_widg = map_seq_button = gtk_check_button_new_with_label ("Map sequence") ;
  gtk_box_pack_start(GTK_BOX(options_box), map_seq_button, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Sequence offset:" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(options_box), label, FALSE, TRUE, 0) ;

  main_frame->offset_widg = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(options_box), entry, FALSE, TRUE, 0) ;

  return frame ;
}



/* Make the action buttons frame. */
static GtkWidget *makeButtonBox(MainFrame main_frame)
{
  GtkWidget *frame ;
  GtkWidget *button_box, *create_button, *config_button, *close_button ;
  char *home_dir ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  button_box = gtk_hbutton_box_new() ;
  gtk_container_border_width(GTK_CONTAINER(button_box), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), button_box) ;

  create_button = gtk_button_new_with_label("Import File") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
		     GTK_SIGNAL_FUNC(importFileCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(button_box), create_button, FALSE, TRUE, 0) ;

  /* N.B. we use the gtk "built-in" file chooser stuff. */
  config_button = gtk_file_chooser_button_new("Choose a File to Import", GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(config_button), "file-set",
		     GTK_SIGNAL_FUNC(chooseConfigCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(button_box), config_button, FALSE, TRUE, 0) ;
  home_dir = (char *)g_get_home_dir() ;
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(config_button), home_dir) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(config_button), TRUE) ;

  /* Only add a close button and set a default button if this is a standalone dialog. */
  if (main_frame->toplevel)
    {
      close_button = gtk_button_new_with_label("Close") ;
      gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
			 GTK_SIGNAL_FUNC(closeCB), (gpointer)main_frame) ;
      gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, TRUE, 0) ;

      /* set create button as default. */
      GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
      gtk_window_set_default(GTK_WINDOW(main_frame->toplevel), create_button) ;
    }

  return frame ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  MainFrame main_frame = (MainFrame)cb_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Nothing to do currently. */

  return ;
}


/* Kill the dialog. */
static void closeCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;

  gtk_widget_destroy(main_frame->toplevel) ;

  return ;
}


/* Called when user chooses a file via the file dialog. */
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame)user_data ;
  char *filename ;

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) ;

  gtk_entry_set_text(GTK_ENTRY(main_frame->file_widg), filename) ;

  g_free(filename) ;

  return ;
}


/* Ok...check the users entries and then call the callback function provided.
 *
 * Note that valid entries are:
 *
 *       sequence & start & end with optional config file
 *
 *       config file (which contains sequence, start, end)
 *
 *  */
static void importFileCB(GtkWidget *widget, gpointer cb_data)
{

  MainFrame main_frame = (MainFrame)cb_data ;
  gboolean status = TRUE ;
  char *err_msg = NULL ;
  char *sequence = "", *start_txt, *end_txt, *file_txt, *offset_txt ;
  int start = 1, end = 0 ;
  gboolean map_seq = FALSE;
  int seq_offset = 0;

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->sequence_widg)) ;
  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->start_widg)) ;
  end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->end_widg)) ;
  file_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->file_widg)) ;

  offset_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->offset_widg)) ;

  if (!*file_txt)
    {
      status = FALSE ;
      err_msg = "Please choose a file to import." ;
    }
 else if (*sequence && *start_txt && *end_txt)
    {
      if (status)
	{
	  if (!(*start_txt) || !zMapStr2Int(start_txt, &start) || start < 1)
	    {
	      status = FALSE ;
	      err_msg = "Invalid start specified." ;
	    }
	}

      if (status)
	{
	  if (!(*end_txt) || !zMapStr2Int(end_txt, &end) || end <= start)
	    {
	      status = FALSE ;
	      err_msg = "Invalid end specified." ;
	    }
	}
    }

  if(status)
  {
	if ((*offset_txt) && !zMapStr2Int(offset_txt, &seq_offset))
		{
		status = FALSE ;
		err_msg = "Invalid offset specified." ;
		}

	map_seq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(main_frame->map_widg));
  }

  if (!status)
    {
      zMapWarning("Error: \"%s\"", err_msg) ;
    }
  else
    {
	char *config_str;
	GList * servers;
	ZMapConfigSource server;
	GList *req_featuresets = NULL;
	ZMapView view;
	ZMap zmap = (ZMap) main_frame->user_data;

	zMapWarning("parameters: %s %d %d %s %d %d",sequence,start,end,file_txt,map_seq,seq_offset);

	config_str = g_strdup_printf("[ZMap]\nsources = temp\n\n[temp]\nfeaturesets=\nurl=file:///%s\n",file_txt);

	servers = zmapViewGetIniSources(NULL, config_str, NULL);
	zMapAssert(servers);

	server = (ZMapConfigSource) servers->data;

	view = zMapViewGetView(zmap->focus_viewwindow);

	if( !zMapViewRequestServer(view, NULL, NULL, req_featuresets, (gpointer) server, start, end, FALSE, TRUE))
	{
		zMapWarning("could not request %s",file_txt);
	}

	zMapConfigSourcesFreeList(servers);
	g_free(config_str);
    }
  return ;
}


