/*  File: zmapAppSequenceView.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012-2015: Genome Research Ltd.
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
 * Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Posts a dialog for user to enter sequence, start/end
 *              and optionally a config file for the sequence. Once
 *              user has chosen then this code calls the function
 *              provided by the caller to get the sequence displayed.
 *
 * Exported functions: See ZMap/zmapAppServices.h
 *
 * NOTE this file has been copied and used as a basis for zmapControlImportFile.c
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <ZMap/zmapControl.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>


/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;
  GtkWidget *config_widg ;

  GtkWidget *chooser_widg ;

  ZMapFeatureSequenceMap orig_sequence_map ;
  ZMapFeatureSequenceMapStruct sequence_map ;
  gboolean display_sequence ;

  ZMapAppGetSequenceViewCB user_func ;
  gpointer user_data ;
  ZMapAppClosedSequenceViewCB close_func ;
  gpointer close_data ;

} MainFrameStruct, *MainFrame ;



static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                            ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                            ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                            ZMapFeatureSequenceMap sequence_map, gboolean display_sequence) ;
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeButtonBox(MainFrame main_data) ;
static void setSequenceEntries(MainFrame main_data) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void createViewCB(GtkWidget *widget, gpointer cb_data) ;
#ifndef __CYGWIN__
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
#endif
static void defaultsCB(GtkWidget *widget, gpointer cb_data) ;
static void createSourceCB(GtkWidget *widget, gpointer cb_data) ;
static void closeCB(GtkWidget *widget, gpointer cb_data) ;



/*
 *                   External interface routines.
 */



/* Display a dialog to get from the reader a sequence to be displayed with a start/end
 * and optionally a config file. */
GtkWidget *zMapAppGetSequenceView(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                                  ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                                  ZMapFeatureSequenceMap sequence_map, gboolean display_sequence)
{
  GtkWidget *toplevel = NULL ;
  GtkWidget *container ;
  gpointer seq_data = NULL ;


  zMapReturnValIfFail(user_func, NULL) ; 

  toplevel = zMapGUIToplevelNew(NULL, "Please specify sequence to be viewed.") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  container = makePanel(toplevel, &seq_data,
                        user_func, user_data,
                        close_func, close_data,
                        sequence_map, display_sequence) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  return toplevel ;
}


/* As for zMapAppGetSequenceView() except that returns a GtkWidget that can be
 * incorporated into a window. */
GtkWidget *zMapCreateSequenceViewWidg(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                                      ZMapFeatureSequenceMap sequence_map, gboolean display_sequence)
{
  GtkWidget *container = NULL ;

  container = makePanel(NULL, NULL, user_func, user_data, NULL, NULL, sequence_map, display_sequence) ;

  return container ;
}




/*
 *                   Internal routines.
 */


/* Make the whole panel returning the top container of the panel. */
static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *our_data,
                            ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                            ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                            ZMapFeatureSequenceMap sequence_map, gboolean display_sequence)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box ;
  MainFrame main_data ;

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->user_func = user_func ;
  main_data->user_data = user_data ;
  main_data->close_func = close_func ;
  main_data->close_data = close_data ;
  main_data->orig_sequence_map = sequence_map ;
  main_data->sequence_map = *sequence_map ;
  main_data->sequence_map.sequence = g_strdup(main_data->sequence_map.sequence) ;
  main_data->sequence_map.config_file = g_strdup(main_data->sequence_map.config_file) ;
  main_data->display_sequence = display_sequence ;

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

  button_box = makeButtonBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, TRUE, 0) ;

  return frame ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;

  frame = gtk_frame_new( "New Sequence: " );
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

  label = gtk_label_new( "Config File :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_data->sequence_widg = entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_data->start_widg = entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_data->end_widg = entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_data->config_widg = entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  /* Set the entry fields if required. */
  if (sequence_map && main_data->display_sequence)
    setSequenceEntries(main_data) ;

  return frame ;
}


/* Make the action buttons frame. */
static GtkWidget *makeButtonBox(MainFrame main_data)
{
  GtkWidget *frame ;
  GtkWidget *button_box, *create_button, *chooser_button, *defaults_button, *source_button, *close_button ;
  char *home_dir ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  button_box = gtk_hbutton_box_new() ;
  gtk_container_border_width(GTK_CONTAINER(button_box), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), button_box) ;

  create_button = gtk_button_new_with_label("Create ZMap") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
                     GTK_SIGNAL_FUNC(createViewCB), (gpointer)main_data) ;
  gtk_box_pack_start(GTK_BOX(button_box), create_button, FALSE, TRUE, 0) ;

#ifndef __CYGWIN__
  /* N.B. we use the gtk "built-in" file chooser stuff. */
  main_data->chooser_widg = chooser_button = gtk_file_chooser_button_new("Choose A Config File",
                                                                         GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(chooser_button), "file-set",
                     GTK_SIGNAL_FUNC(chooseConfigCB), (gpointer)main_data) ;
  gtk_box_pack_start(GTK_BOX(button_box), chooser_button, FALSE, TRUE, 0) ;
  home_dir = (char *)g_get_home_dir() ;
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_button), home_dir) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(chooser_button), TRUE) ;
#else
  main_data->chooser_widg = chooser_button = NULL ;
#endif

  /* If a sequence is provided then make a button to set it in the entries fields,
   * saves the user tedious typing. */
  if ((main_data->sequence_map.sequence))
    {
      defaults_button = gtk_button_new_with_label("Set Defaults") ;
      gtk_signal_connect(GTK_OBJECT(defaults_button), "clicked",
                         GTK_SIGNAL_FUNC(defaultsCB), (gpointer)main_data) ;
      gtk_box_pack_start(GTK_BOX(button_box), defaults_button, FALSE, TRUE, 0) ;
    }

  
  source_button = gtk_button_new_with_label("Set up source") ;
  gtk_signal_connect(GTK_OBJECT(source_button), "clicked", 
                     GTK_SIGNAL_FUNC(createSourceCB), (gpointer)main_data) ;
  gtk_box_pack_start(GTK_BOX(button_box), source_button, FALSE, TRUE, 0) ;


  /* Only add a close button and set a default button if this is a standalone dialog. */
  if (main_data->toplevel)
    {
      close_button = gtk_button_new_with_label("Close") ;
      gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
                         GTK_SIGNAL_FUNC(closeCB), (gpointer)main_data) ;
      gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, TRUE, 0) ;

      /* set create button as default. */
      GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
      gtk_window_set_default(GTK_WINDOW(main_data->toplevel), create_button) ;
    }

  return frame ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  /* Signal app we are going if we are a separate window. */
  if (main_data->close_func)
    (main_data->close_func)(main_data->toplevel, main_data->close_data) ;

  /* Free resources. */
  if (main_data->sequence_map.sequence)
    g_free(main_data->sequence_map.sequence) ;
  if (main_data->sequence_map.config_file)
    g_free(main_data->sequence_map.config_file) ;

  g_free(main_data) ;

  return ;
}


/* Kill the dialog. */
static void closeCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  gtk_widget_destroy(main_data->toplevel) ;

  return ;
}


/* Set default values in dialog. */
static void defaultsCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  setSequenceEntries(main_data) ;

#ifndef __CYGWIN__
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(main_data->chooser_widg), main_data->sequence_map.config_file) ;
#endif

  return ;
}


/* Callback called when the create-source dialog has been ok'd to do the work to create the new
 * source from the user-provided info. Returns true if successfully created the source.  */
static void createNewSourceCB(const char *source_name, 
                              const std::string &url, 
                              const char *featuresets,
                              const char *biotypes,
                              gpointer user_data,
                              GError **error)
{
  ZMapFeatureSequenceMap sequence_map = (ZMapFeatureSequenceMap)user_data ;
  zMapReturnIfFail(sequence_map) ;

  GError *tmp_error = NULL ;

  ZMapConfigSource source = g_new0(ZMapConfigSourceStruct, 1) ;

  source->url = g_strdup(url.c_str()) ;
      
  if (featuresets && *featuresets)
    source->featuresets = g_strdup(featuresets) ;
      
  /* Add the new source to the view */
  std::string source_name_str(source_name) ;
  sequence_map->AddSource(source_name_str, source, &tmp_error) ;

  if (tmp_error)
    {
      zMapConfigSourceDestroy(source) ;
      source = NULL ;

      g_propagate_error(error, tmp_error) ;
    }
}


/* Set up a new source. */
static void createSourceCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  zMapAppCreateSource(&main_data->sequence_map, createNewSourceCB, main_data->orig_sequence_map) ;

  return ;
}


#ifndef __CYGWIN__
/* Called when user chooses a file via the file dialog. */
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  char *filename = NULL ;

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) ;

  gtk_entry_set_text(GTK_ENTRY(main_data->config_widg), filename) ;

  g_free(filename) ;

  return ;
}
#endif


/* Insert start/end etc. into entry fields for sequence to be displayed. */
static void setSequenceEntries(MainFrame main_data)
{
  const char *sequence = "", *config_file = "" ;
  char *start = NULL, *end = NULL ;

  if (main_data->sequence_map.sequence)
    sequence = main_data->sequence_map.sequence ;
  if (main_data->sequence_map.start)
    start = g_strdup_printf("%d", main_data->sequence_map.start) ;
  if (main_data->sequence_map.end)
    end = g_strdup_printf("%d", main_data->sequence_map.end) ;
  if (main_data->sequence_map.config_file)
    config_file = main_data->sequence_map.config_file ;

  gtk_entry_set_text(GTK_ENTRY(main_data->sequence_widg), sequence) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->start_widg), (start ? start : "")) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->end_widg), (end ? end : "")) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->config_widg), config_file) ;

  if (start)
    g_free(start) ;
  if (end)
    g_free(end) ;

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
static void createViewCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  gboolean status = TRUE ;
  const char *err_msg = NULL ;
  const char *sequence = "", *start_txt, *end_txt, *config_txt ;
  int start = 1, end = 0 ;
  GError *tmp_error = NULL ;


  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence = (char *)gtk_entry_get_text(GTK_ENTRY(main_data->sequence_widg)) ;
  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_data->start_widg)) ;
  end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_data->end_widg)) ;
  config_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_data->config_widg)) ;


  if (!(*sequence) && !(*start_txt) && !(*end_txt) && *config_txt)
    {
      /* Just a config file specified, try that. */
      status = TRUE ;
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

      if (status)
        {
          if (!(*config_txt))
            config_txt = NULL ;    /* No file specified. */
        }
    }
  else
    {
      status = FALSE ;
      err_msg = "You must specify\n"
        "either just a config file containing a sequence and start,end\n"
        "or a sequence, start, end and optionally a config file." ;
    }


  if (!status)
    {
      zMapWarning("Error: \"%s\"", err_msg) ;
    }
  else
    {
      /* when we get here we should either have only a config file specified or
       * a sequence/start/end and optionally a config file. */
      ZMapFeatureSequenceMap seq_map ;
      gboolean sequence_ok = FALSE ;
      const char *err_msg = NULL ;

      seq_map = g_new0(ZMapFeatureSequenceMapStruct,1) ;

      seq_map->config_file = g_strdup(config_txt) ;

      if (main_data->orig_sequence_map)
        seq_map->sources = main_data->orig_sequence_map->sources ;

      if (*sequence)
        {
          sequence_ok = TRUE ;
          seq_map->sequence = g_strdup(sequence) ;
          seq_map->start = start ;
          seq_map->end = end ;
        }
      else
        {
          zMapAppGetSequenceConfig(seq_map, &tmp_error);
                  
          if (tmp_error)
            {
              sequence_ok = FALSE ;
              err_msg = tmp_error->message ;
            }
          else if (!seq_map->sequence || !seq_map->start || !seq_map->end)
            {
              err_msg = "Cannot load sequence from config file, check sequence, start and end specified." ;
              sequence_ok = FALSE ;
            }
          else
            {
              sequence_ok = TRUE ;
            }
        }

      if (!sequence_ok)
        {
          zMapWarning("%s", err_msg) ;
          g_free(seq_map) ;
        }
      else
        {
          /* Call back with users parameters for new sequence display. */
          (main_data->user_func)(seq_map, main_data->user_data) ;
        }
    }

  if (tmp_error)
    g_error_free(tmp_error) ;

  return ;
}


