/*  File: zmapControlImportFile.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
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
 * and then tweaked. There may be some common code & functions
 * but maybe this file will be volatile for a while
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <iostream>
#include <string.h>

#include <ZMap/zmapString.hpp>    /* testing only. */

#include <config.h>

#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <zmapControl_P.hpp>

#include <ZMap/zmapDataSource.hpp>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapDataSourceRequest.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


using namespace std ;
using namespace ZMapDataSource ;


/* number of optional dialog entries for ZMAPSOURCE_FILE_NONE (is really 8 so i allowed a few spare) */
#define N_ARGS 16


/*
 * was used for input script description, args etc
 */
typedef struct
{
  ZMapSourceFileType file_type;
  char * script;
  gchar ** args;   /* an allocated null terminated array of arg strings */
  gchar ** allocd; /* args for freeing on destroy */
} ZMapImportScriptStruct, *ZMapImportScript;

/* Data we need in callbacks. */
typedef struct MainFrameStruct_
{
  GtkWidget *toplevel ;
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;
  /*GtkWidget *whole_widg ;*/
  GtkWidget *file_widg ;
  GtkWidget *dataset_widg;
  GtkWidget *req_sequence_widg ;
  GtkWidget *req_start_widg ;
  GtkWidget *req_end_widg ;
  GtkWidget *source_widg;
  GtkWidget *assembly_widg;
  GtkWidget *strand_widg ;
  GtkWidget *map_widg ;


  ZMapSourceFileType file_type;

  gboolean is_otter;
  char *chr;

  ZMapFeatureSequenceMap sequence_map;

  ZMap zmap ;

} MainFrameStruct, *MainFrame ;




static MainFrame makePanel(GtkWidget *toplevel,
                           ZMap zmap,
                           ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *seq, int start, int end);
static void makeButtonBox(GtkWidget *toplevel, MainFrame main_frame) ;
static void importDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;

#ifndef __CYGWIN__
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
#endif
static void closeDialog(MainFrame main_frame) ;
static void fileChangedCB(GtkWidget *widget, gpointer user_data);


static void ImportFile(MainFrame main_frame) ;



static void importNewDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data) ;
static void newImportFile(MainFrame main_frame) ;
static void newCallBackFunc(DataSourceFeatures *features_source, void *user_data) ;
static void newSequenceCallBackFunc(DataSourceSequence *features_source, void *user_data) ;





/*
 *                   External interface routines.
 */

// None currently.



/*
 *                   Package routines.
 */



/* Display a dialog to get from the reader a file to be displayed
 * with a optional start/end and various mapping parameters
 */
void zmapControlImportFile(ZMap zmap,
                           ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel ;
  MainFrame main_frame ;

  zmap->import_file_dialog = toplevel = zMapGUIDialogNew(NULL, "Please choose a file to import.", NULL, NULL) ;

  /* Make sure the dialog is destroyed if the zmap window is closed */
  /*! \todo For some reason this doesn't work and the dialog hangs around
   * after zmap->toplevel is destroyed */
  if (zmap && zmap->toplevel && GTK_IS_WINDOW(zmap->toplevel))
    gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(zmap->toplevel)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  main_frame = makePanel(toplevel, zmap, sequence_map, req_start, req_end) ;

  g_signal_connect(G_OBJECT(toplevel), "response", G_CALLBACK(importDialogResponseCB), main_frame) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), main_frame) ;

  gtk_widget_set_size_request(toplevel, 500, -1) ;

  gtk_widget_show_all(toplevel) ;

  return ;
}




// Test dialog for new server stuff....
//
//
void zmapControlNewImportFile(ZMap zmap,
                              ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel ;
  MainFrame main_frame ;

  toplevel = zMapGUIDialogNew(NULL, "Please choose a file to import.", NULL, NULL) ;

  /* Make sure the dialog is destroyed if the zmap window is closed */
  /*! \todo For some reason this doesn't work and the dialog hangs around
   * after zmap->toplevel is destroyed */
  if (zmap && zmap->toplevel && GTK_IS_WINDOW(zmap->toplevel))
    gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(zmap->toplevel)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  main_frame = makePanel(toplevel, zmap, sequence_map, req_start, req_end) ;

  g_signal_connect(G_OBJECT(toplevel), "response", G_CALLBACK(importNewDialogResponseCB), main_frame) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), main_frame) ;

  gtk_widget_set_size_request(toplevel, 500, -1) ;

  gtk_widget_show_all(toplevel) ;

  return ;
}




/*
 *                   Internal routines.
 */


/* To fit in w/ ZMap practice we have to handle multiple views and config files
 * so this all has to be allocated */
static void importGetConfig(MainFrame main_frame, char *config_file)
{
  ZMapConfigIniContext context;
  /* in parallel with the filetype enum */
  char *tmp_string;

  if ((context = zMapConfigIniContextProvide(config_file, ZMAPCONFIG_FILE_NONE)))
    {
      GKeyFile *gkf;
      gchar ** keys,**freethis;
      char *arg_str;
      gsize len;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                       ZMAPSTANZA_APP_CSVER, &tmp_string))
        {
          if(!g_ascii_strcasecmp(tmp_string,"Otter"))
            {
              char *chr;

              main_frame->is_otter = TRUE;

              if(zMapConfigIniContextGetString(context,ZMAPSTANZA_APP_CONFIG,ZMAPSTANZA_APP_CONFIG,ZMAPSTANZA_APP_CHR,&chr))
                {
                  main_frame->chr = chr;
                }
              else
                {
                  main_frame->chr = NULL;
                }
            }
        }

      if(zMapConfigIniHasStanza(context->config,ZMAPSTANZA_IMPORT_CONFIG,&gkf))
        {
          freethis = keys = g_key_file_get_keys(gkf,ZMAPSTANZA_IMPORT_CONFIG,&len,NULL);

          for(;len--;keys++)
            {
              arg_str = g_key_file_get_string(gkf,ZMAPSTANZA_IMPORT_CONFIG,*keys,NULL);

              if(!arg_str || !*arg_str)
                continue;

              /*argp = args = g_strsplit(arg_str, ";" , 0); */
              /* NB we strip leading & trailing whitespace from args later */

              /*ft = *argp++;
              while(*ft && *ft <= ' ')
                ft++;

              if(!g_ascii_strncasecmp(ft,"GFF",3))
                file_type = ZMAPSOURCE_FILE_GFF;
              else if(!g_ascii_strncasecmp(ft,"BAM",3))
                file_type = ZMAPSOURCE_FILE_BAM;
              else if(!g_ascii_strncasecmp(ft,"BIGWIG",6))
                file_type = ZMAPSOURCE_FILE_BIGWIG;
              else
                continue;*/

              /*s = scripts + file_type;
              s->script = g_strdup(*keys);
              s->args = argp;
              s->allocd = args;*/
            }

          if (freethis)
            g_strfreev(freethis) ;
        }

      zMapConfigIniContextDestroy(context) ;
      context = NULL ;
    }

  return ;
}


/* Make the whole panel returning the top container of the panel. */
static MainFrame makePanel(GtkWidget *toplevel,
                           ZMap zmap,
                           ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *vbox, *main_frame, *options_box;
  MainFrame main_data ;
  const char *sequence = "";
  GtkDialog *dialog = GTK_DIALOG(toplevel) ;

  vbox = dialog->vbox ;

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->toplevel = toplevel ;

  main_data->zmap = zmap ;

  importGetConfig(main_data, sequence_map->config_file ) ;

  main_frame = makeMainFrame(main_data, sequence_map) ;
  gtk_box_pack_start(GTK_BOX(vbox), main_frame, TRUE, TRUE, 0) ;

  main_data->sequence_map = sequence_map ;

  sequence = sequence_map->sequence ;

  options_box = makeOptionsBox(main_data, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, TRUE, TRUE, 0) ;

  makeButtonBox(toplevel, main_data) ;

  return main_data ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;
  const char *sequence = "", *dataset = "" ;
  char *start = NULL, *end = NULL ;

  if (sequence_map->sequence)
    sequence = sequence_map->sequence ;
  if (sequence_map->dataset)
    dataset = sequence_map->dataset ;
  if (sequence_map->start)
    start = g_strdup_printf("%d", sequence_map->start) ;
  if (sequence_map->end)
    end = g_strdup_printf("%d", sequence_map->end) ;

  frame = gtk_frame_new( "ZMap Sequence: " );
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

  label = gtk_label_new( "Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new("Dataset ") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->dataset_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), dataset) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (start ? start : "")) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (end ? end : "")) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);


  /* Free resources. */
  if (*start)
    g_free(start) ;
  if (*end)
    g_free(end) ;


  return frame ;
}


/* Make the option buttons frame. */
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *req_sequence, int req_start, int req_end)
{
  GtkWidget *frame = NULL ;
  GtkWidget *map_seq_button = NULL , *config_button = NULL ;
  GtkWidget *topbox = NULL, *hbox = NULL, *entrybox = NULL, *labelbox = NULL, *entry = NULL, *label = NULL ;
  const char *sequence = "", *file = "" ;
  char *start = NULL, *end = NULL ;
  char *home_dir ;

  if (req_sequence)
    {
      sequence = req_sequence ;
      if (req_start)
        start = g_strdup_printf("%d", req_start) ;
      if (req_end)
        end = g_strdup_printf("%d", req_end) ;
    }

  frame = gtk_frame_new("Import data") ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;


  /* Labels..... */
  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

#ifndef __CYGWIN__
  /* N.B. we use the gtk "built-in" file chooser stuff. Create a file-chooser button instead of
   * placing a label next to the file text-entry box */
  config_button = gtk_file_chooser_button_new("Choose a File to Import", GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(config_button), "file-set",
     GTK_SIGNAL_FUNC(chooseConfigCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(labelbox), config_button, FALSE, TRUE, 0) ;
  home_dir = (char *)g_get_home_dir() ;
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(config_button), home_dir) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(config_button), TRUE) ;
#else
  /* We can't have a file-chooser button, but we already have a text entry
   * box for the filename anyway, so just create a label for that where the
   * button would have been */
  label = gtk_label_new("File ") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
#endif

  label = gtk_label_new( "Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Assembly " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Source " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Strand (+/-) " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Map Features " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->file_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), file) ;
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
     GTK_SIGNAL_FUNC(fileChangedCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->req_sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->req_start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (start ? start : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->req_end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (end ? end : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->assembly_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->source_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->strand_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_max_length(GTK_ENTRY(entry), (gint)1) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(main_frame->strand_widg, FALSE );

  /* Make the default remap setting true if we're hooked up to otterlace (which does the
   * remapping), false otherwise. */
  main_frame->map_widg = map_seq_button = gtk_check_button_new () ;
  gtk_toggle_button_set_active((GtkToggleButton*)map_seq_button, main_frame->is_otter) ;
  gtk_box_pack_start(GTK_BOX(entrybox), map_seq_button, FALSE, TRUE, 0) ;


  if (*start)
    g_free(start) ;
  if (*end)
    g_free(end) ;


  return frame ;
}



/* Make the action buttons frame. */
static void makeButtonBox(GtkWidget *toplevel, MainFrame main_frame)
{

 GtkDialog *dialog = GTK_DIALOG(toplevel) ;

  gtk_dialog_add_button(dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL) ;
  gtk_dialog_add_button(dialog, GTK_STOCK_APPLY, GTK_RESPONSE_OK) ;

  gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK) ;

  return ;
}




/* \brief Handles the response to the import dialog */
static void importDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data)
{
  MainFrame main_frame = (MainFrame)data ;

  switch (response_id)
  {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
      ImportFile(main_frame) ;
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:
      break;

    default:
      break;
  };

  // Always close dialog, see if users ok with that.
  closeDialog(main_frame) ;

  return ;
}



/* Handles the response to the import dialog */
static void importNewDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data)
{
  MainFrame main_frame = (MainFrame)data ;

  switch (response_id)
  {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
      newImportFile(main_frame) ;
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      closeDialog(main_frame) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      break;

    default:
      break;
  };

  // Try this here....
  closeDialog(main_frame) ;

  return ;
}




/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;

  main_frame->zmap->import_file_dialog = NULL ;


  g_free(main_frame) ;

  return ;
}


/* Kill the dialog. */
static void closeDialog(MainFrame main_frame)
{
  gtk_widget_destroy(main_frame->toplevel) ;

  return ;
}


#ifndef __CYGWIN__
/* Called when user chooses a file via the file dialog. */
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;
  char *filename = NULL ;

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) ;

  gtk_entry_set_text(GTK_ENTRY(main_frame->file_widg), g_strdup(filename)) ;

  fileChangedCB ( main_frame->file_widg, user_data);
}
#endif

/*
 * Special cases:
 *
 * (1) Require strand if wre have a bigwig file
 * (2) Do _not_ specify source if we have a GFF file
 */
static void enable_widgets(MainFrame main_frame)
{
  if (!main_frame)
    return ;

  gboolean is_gff = main_frame->file_type == ZMAPSOURCE_FILE_GFF,
    is_bam = main_frame->file_type == ZMAPSOURCE_FILE_BAM,
    is_big = main_frame->file_type == ZMAPSOURCE_FILE_BIGWIG,
    is_otter = main_frame->is_otter ;

  /*
   * We only allow some of the widgets to be set under the
   * following conditions.
   */
  gtk_widget_set_sensitive(main_frame->strand_widg, is_big );
  gtk_widget_set_sensitive(main_frame->source_widg, !is_gff ) ;
  gtk_widget_set_sensitive(main_frame->assembly_widg, !is_gff && is_otter ) ;
  gtk_widget_set_sensitive(main_frame->map_widg, (is_bam || is_big) && is_otter) ;

  /*gtk_widget_set_sensitive(main_frame->args_widg, main_frame->file_type == ZMAPSOURCE_FILE_NONE ); */

  /*gtk_widget_set_sensitive(main_frame->map_widg, !(is_bam && main_frame->is_otter));*/
  /*gtk_widget_set_sensitive(main_frame->offset_widg, !(is_bam && main_frame->is_otter));*/
  /*gtk_widget_set_sensitive(main_frame->req_sequence_widg, !(main_frame->is_otter));*/
  /*gtk_widget_set_sensitive(main_frame->assembly_widg, (main_frame->is_otter));*/
}


/*
 * Callback for when the "file" text is changed.
 */
static void fileChangedCB(GtkWidget *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;
  char *filename ;
  char *extent;
  ZMapSourceFileType file_type = ZMAPSOURCE_FILE_NONE ;
  char *source_txt = NULL;


  /*
   * Inspect the file extension to determine type.
   * Case is ignored.
   */
  filename = (char *) gtk_entry_get_text(GTK_ENTRY(widget)) ;
  extent = filename + strlen(filename);

  while(*extent != '.' && extent > filename)
    extent--;
  if (!g_ascii_strcasecmp(extent,".gff"))
    file_type = ZMAPSOURCE_FILE_GFF ;
  else if (!g_ascii_strcasecmp(extent,".bam"))
    file_type = ZMAPSOURCE_FILE_BAM ;
  else if (!g_ascii_strcasecmp(extent,".sam"))
    file_type = ZMAPSOURCE_FILE_BAM ;
  else if(!g_ascii_strcasecmp(extent,".bigwig"))
    file_type = ZMAPSOURCE_FILE_BIGWIG ;

  /*
   * Set the file type in the struct.
   */
  main_frame->file_type = file_type;

  gtk_entry_set_text(GTK_ENTRY(main_frame->strand_widg), "")  ;
  gtk_entry_set_text(GTK_ENTRY(main_frame->source_widg), "")  ;
  gtk_entry_set_text(GTK_ENTRY(main_frame->assembly_widg), "")  ;

  /* Make the default remap setting true if we're hooked up to otterlace (which does the
   * remapping), false otherwise. */
  gtk_toggle_button_set_active((GtkToggleButton*)main_frame->map_widg, main_frame->is_otter) ;

  /*
   *Try to get a source name for non-GFF types.
   */
  if (main_frame->file_type == ZMAPSOURCE_FILE_GFF)
    {
      gtk_entry_set_text(GTK_ENTRY(main_frame->source_widg), "") ;
    }
  else
    {
      char *name = extent;
      while(name > filename && *name != '/')
        name--;
      if(*name == '/')
        name++;
      source_txt = g_strdup_printf("%.*s", (int)(extent - name), name) ;
      gtk_entry_set_text(GTK_ENTRY(main_frame->source_widg), source_txt) ;
    }

  /* add featureset_2_style entry to the view */
  /*
  if(main_frame->file_type == ZMAPSOURCE_FILE_BAM)
    {
      GQuark f_id;

      f_id = zMapFeatureSetCreateID("BAM");
      src = zMapViewGetFeatureSetSource(view, f_id);
      if(src)
        {
          style_txt = g_strdup_printf("%s",g_quark_to_string(src->style_id));
          gtk_entry_set_text(GTK_ENTRY(main_frame->style_widg), style_txt) ;
        }
    }
  */

  /*
   * Have a go at extracting the strand from the file name
   * in the case of bigwig type.
   */
  if(main_frame->file_type == ZMAPSOURCE_FILE_BIGWIG)
    {
      /*GQuark f_id;*/
      const char * strand_txt = NULL ;

      /*f_id = zMapFeatureSetCreateID("bigWig");
      src = zMapViewGetFeatureSetSource(view, f_id);
      if(src)
        {
          style_txt = g_strdup_printf("%s",g_quark_to_string(src->style_id));
          gtk_entry_set_text(GTK_ENTRY(main_frame->style_widg), style_txt) ;
        }*/

      strand_txt = g_strstr_len(filename,-1,"inus") ? "-" : "+";
      gtk_entry_set_text(GTK_ENTRY(main_frame->strand_widg), strand_txt) ;

    }

  enable_widgets(main_frame);

  return ;
}


/* Get the script to use for the given file type. This should not be hard-coded really but long
 * term we will hopefully not need these scripts when zmap can do the remapping itself. Returns
 * null if not found or not executable */
static const char* fileTypeGetPipeScript(ZMapSourceFileType file_type, 
                                         std::string &script_err)
{
  const char *result = NULL ;

  if (file_type == ZMAPSOURCE_FILE_BAM)
    result = "bam_get" ;
  else if (file_type == ZMAPSOURCE_FILE_BIGWIG)
    result = "bigwig_get" ;
  else
    script_err = "File type does not have a remapping script" ;

  /* Check it's executable */
  if (result && !g_find_program_in_path(result))
    {
      script_err = "Script '" ;
      script_err += result ;
      script_err += "' is not executable" ;
      result = NULL ;
    }

  return result ;
}


/* Various functions to validate the input values on the dialog. They set the status to false
 * if there is a problem. */
static void validateFile(bool &status, 
                         std::string &err_msg,
                         const char *file_txt)
{
  if (status)
    {
      if (!(*file_txt))
        {
          status = FALSE ;
          err_msg = "Please choose a file to import." ;
        }
    }
}

static void validateSequence(bool &status, 
                             std::string &err_msg,
                             const char *sequence_txt, 
                             const char *start_txt, 
                             const char *end_txt,
                             int &start,
                             int &end)
{
  if (status)
    {
      if (*sequence_txt && *start_txt && *end_txt)
        {
          if (!zMapStr2Int(start_txt, &start) || start < 1)
            {
              status = FALSE ;
              err_msg = "Invalid start specified." ;
            }
          else if (!zMapStr2Int(end_txt, &end) || end <= start)
            {
              status = FALSE ;
              err_msg = "Invalid end specified." ;
            }
        }
    }
}

static void validateReqSequence(bool &status,
                                std::string &err_msg,
                                const char *req_sequence_txt, 
                                const char *req_start_txt, 
                                const char *req_end_txt,
                                int &req_start,
                                int &req_end,
                                const int start)
{
  /*
   * Request sequence name, start and end.
   */
  if (status)
    {
      if (*req_sequence_txt && *req_start_txt && *req_end_txt)
        {
          if (!zMapStr2Int(req_start_txt, &req_start) || req_start < 1)
            {
              status = FALSE ;
              err_msg = "Invalid request start specified." ;
            }
          else if (!zMapStr2Int(req_end_txt, &req_end) || req_end <= start)
            {
              status = FALSE ;
              err_msg = "Invalid request end specified." ;
            }
        }
      else
        {
          status = FALSE;
          err_msg = "Please specify request sequence start and end";
        }
    }
}

static void validateFileType(bool &status, 
                             std::string &err_msg,
                             ZMapSourceFileType file_type,
                             const char *source_txt,
                             const char *assembly_txt,
                             const char *dataset_txt, 
                             const bool remap_features,
                             bool &have_assembly,
                             bool &have_dataset,
                             char *sequence_txt)
{
  /*
   * Error conditions and data combinations are complex enough here to be treated
   * seperately for each data type.
   */
  if (status)
    {
      if (file_type == ZMAPSOURCE_FILE_GFF)
        {
          /* we do not want the source for a gff file */
          if (status && *source_txt)
            {
              status = FALSE ;
              err_msg = "Source should not be specified for GFF file." ;
            }

          /* we do not want the assembly for a gff file */
          if (status && (*assembly_txt))
            {
              status = FALSE ;
              err_msg = "Assembly should not be specified for GFF file." ;
            }

        }
      else if ((file_type == ZMAPSOURCE_FILE_BAM) || (file_type == ZMAPSOURCE_FILE_BIGWIG))
        {
          /* we must have a source for these data types */
          if (!*source_txt)
            {
              status = FALSE ;
              err_msg = "Source must be specified for this type." ;
            }

          /*
           * actually this needs to be modified to require neither
           * or both of assembly and dataset...
           */
          if (*assembly_txt)
            have_assembly = TRUE ;
          if (*dataset_txt)
            have_dataset = TRUE ;

          /*
           * Some error checking of conditions for remapping. If we request remapping
           * then there must be a dataset and assembly given. Then the call to
           * the scripts with remapping data can be done. Otherwise these data are
           * to be ignored.
           */
          if (remap_features)
            {
              if ((!have_assembly) || (!have_dataset))
                {
                  status = FALSE ;
                  err_msg = "Both the 'assembly' and 'dataset' are required for remapping." ;
                }

            }
          else
            {
              char *pos = strchr(sequence_txt, '-') ;

              if (pos)
                *pos = '\0' ;
            }

        }
      else
        {
          status = FALSE ;
          err_msg = "Unknown file type." ;
        }

    } /* if (status) */

}

static void validateStrand(bool &status, 
                           std::string &err_msg,
                           ZMapSourceFileType file_type,
                           const char *strand_txt,
                           int &strand)
{
  /* strand is required for bigwig only */
  if (status && (file_type == ZMAPSOURCE_FILE_BIGWIG))
    {
      if (strlen(strand_txt) == 1)
        {
          if ((strchr(strand_txt, '+')))
            strand = 1 ;
          else if ((strchr(strand_txt, '-')))
            strand = -1 ;
          else
            {
              status = FALSE ;
              err_msg = "Strand must be + or -";
            }
        }
      else
        {
          status = FALSE ;
          err_msg = "Invalid value for strand.";
        }
    }
  else if (status)
    {
      if (strlen(strand_txt) != 0)
        {
          status = FALSE ;
          err_msg = "Strand should not be specified for this type." ;
        }
    }
}


/* Ok...check the users entries and then call the callback function provided.
 *
 * Note that valid entries are:
 *
 *       sequence & start & end with optional config file
 *
 *       config file (which contains sequence, start, end)
 *
 * (sm23)  One more thing remains after what's just been done, that is,
 * to deal with the "assembly" argument to the scripts being optional,
 * it should be omitted when no assembly is given by the user, and no
 * remapping should be done.
 *
 */
static void ImportFile(MainFrame main_frame)
{
  bool status = TRUE ;
  bool have_dataset = FALSE ;
  bool have_assembly = FALSE ;
  bool remap_features = TRUE ;
  ZMapFeatureSequenceMap sequence_map = NULL ;
  std::string err_msg("") ;
  char *sequence_txt = NULL ;
  const char *dataset_txt = NULL ;
  const char *start_txt = NULL ;
  const char *end_txt = NULL ;
  const char *file_txt = NULL ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  const char *source_txt = NULL ;
  const char *strand_txt = NULL ;
  const char *assembly_txt = NULL ;
  const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int end = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  int strand = 0 ;
  ZMapSourceFileType file_type = ZMAPSOURCE_FILE_NONE ;
  ZMapView view = NULL ;
  ZMap zmap = NULL ;

  file_type = main_frame->file_type;
  zmap = (ZMap) main_frame->zmap ;
  sequence_map = main_frame->sequence_map ;

  view = zMapViewGetView(zmap->focus_viewwindow);

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence_txt = g_strdup(gtk_entry_get_text(GTK_ENTRY(main_frame->sequence_widg))) ;
  dataset_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->dataset_widg)) ;
  start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->start_widg)) ;
  end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->end_widg)) ;

  file_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->file_widg)) ;
  req_sequence_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;
  source_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->source_widg)) ;
  assembly_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->assembly_widg)) ;
  strand_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->strand_widg)) ;
  remap_features = gtk_toggle_button_get_active((GtkToggleButton*)main_frame->map_widg) ;

  /* Validate the user-specified arguments. These calls set status to false if there is a problem
   * and also set the err_msg. The start/end/strand functions also set the int from the string. 
   */
  validateFile(status, err_msg, file_txt) ;
  validateSequence(status, err_msg, sequence_txt, start_txt, end_txt, start, end) ;
  validateReqSequence(status, err_msg, req_sequence_txt, req_start_txt, req_end_txt, req_start, req_end, start) ;
  validateFileType(status, err_msg, file_type, source_txt, 
                   assembly_txt, dataset_txt, remap_features, 
                   have_assembly, have_dataset, sequence_txt) ;
  validateStrand(status, err_msg, file_type, strand_txt, strand) ;

  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (status)
    {
      bool create_source_data = FALSE ;

      /* Create the server */
      ZMapConfigSource server = NULL ;

      if (file_type == ZMAPSOURCE_FILE_GFF)
        {
          /* Just load the file directly */
          server = sequence_map->createFileSource(source_txt, file_txt) ;
        }
      else
        {
          /* Use a pipe script that will remap the features if necessary. This is also required
           * in order to be able to blixem the columns because blixem doesn't support bam reading
           * at the moment, so the script command gets passed through to blixem. */
          std::string script_err("");
          const char *script = fileTypeGetPipeScript(file_type, script_err) ;

          GString *args = g_string_new("") ;

          g_string_append_printf(args, "--file=%s&--chr=%s&--start=%d&--end=%d&--gff_seqname=%s&--gff_source=%s",
                                 file_txt, sequence_txt, req_start, req_end, req_sequence_txt, source_txt) ;

          if (file_type == ZMAPSOURCE_FILE_BIGWIG)
            g_string_append_printf(args, "&--strand=%d", strand) ;

          if (remap_features)
            g_string_append_printf(args, "&--csver_remote=%s&--dataset=%s", assembly_txt, dataset_txt) ;

          if (script)
            {
              /* Ok, create the server for this pipe scripe */
              server = sequence_map->createPipeSource(source_txt, file_txt, script, args->str) ;
              create_source_data = TRUE ;
            }
          else if (!remap_features)
            {
              /* No script; if not remapping features, we can load directly with htslib
               * instead. There are a couple of problems with this, though:
               * - the given source name will be ignored and therefore the source_data won't
               *   be linked correctly. This means it won't be recognised as a bam.
               * - blixem won't work (until it can load bam files directly, or unless the user
               *   has preconfigured the source in the blixem config). */
#ifdef USE_HTSLIB
              zMapLogMessage("%s", "Pipe script not available; using htslib") ;
              server = sequence_map->createFileSource(source_txt, file_txt) ;
#else
              err_msg = "Cannot load features; pipe script not available: " ;
              err_msg += script_err ;
#endif
            }
          else
            {
              err_msg = "Cannot remap features: " ;
              err_msg += script_err ;
            }

          g_string_free(args, TRUE) ;
        }

      if (create_source_data)
        {
          /* If we're using a pipe script, we need to create the source data for the named source
           * so that we can set is_seq=true for bam sources (otherwise we don't know when we come
           * to parse the results of the bam_get script that is_seq should be true).
           * We don't do this if we're loading the file directly because the named source is not used
           * (the source data is created when the features are parsed from the file instead) */
          ZMapFeatureContextMap context_map = zMapViewGetContextMap(view) ;

          GQuark fset_id = zMapFeatureSetCreateID(source_txt);
          ZMapFeatureSource source_data = context_map->getSource(fset_id);

          if (!source_data)
            {
              GQuark source_id = fset_id ;
              GQuark style_id = zMapStyleCreateID(NULL) ;
              GQuark source_text = g_quark_from_string(source_txt) ;
              const bool is_seq = (file_type == ZMAPSOURCE_FILE_BAM || file_type == ZMAPSOURCE_FILE_BIGWIG) ;

              context_map->createSource(fset_id,
                                        source_id, source_text, style_id,
                                        0, 0, is_seq) ;
            }
        }

      if (server)
        {
          GList *req_featuresets = NULL ;
          
          if (zMapViewRequestServer(view, NULL, req_featuresets, NULL,
                                    (gpointer)server, req_start, req_end, FALSE, TRUE, TRUE))
            zMapViewShowLoadStatus(view);
          else
            zMapWarning("Request to server failed for %s",file_txt);
        }
      else
        {
          zMapWarning("Failed to set up source for file '%s': %s", file_txt, err_msg.c_str()) ;
        }
    }
  else
    {
      zMapWarning("Error: \"%s\"", err_msg.c_str()) ;
    }

  if (sequence_txt)
    g_free(sequence_txt) ;

  return ;
}





//
// gateway to new source interface for Steve
//
//
//
static void newImportFile(MainFrame main_frame)
{
  bool status = TRUE ;
  bool have_dataset = FALSE ;
  bool have_assembly = FALSE ;
  bool remap_features = TRUE ;
  ZMapFeatureSequenceMap sequence_map = NULL ;
  std::string err_msg("") ;
  char *sequence_txt = NULL ;
  const char *dataset_txt = NULL ;
  const char *start_txt = NULL ;
  const char *end_txt = NULL ;
  const char *file_txt = NULL ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  const char *source_txt = NULL ;
  const char *strand_txt = NULL ;
  const char *assembly_txt = NULL ;
  const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int end = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  int strand = 0 ;
  ZMapSourceFileType file_type = ZMAPSOURCE_FILE_NONE ;
  ZMapView view = NULL ;
  ZMap zmap = NULL ;

  // Error handling seems flakey....

  file_type = main_frame->file_type;
  zmap = (ZMap) main_frame->zmap ;
  sequence_map = main_frame->sequence_map ;


  // HACKY BUT THIS ISN'T PERMANENT CODE....
  /* Check that the zmap window hasn't been closed (currently the dialog
   * isn't closed automatically with it, so we'll have problems if we don't
   * exit early here) */
  if (!zmap->toplevel)
    return;

  view = zMapViewGetView(zmap->focus_viewwindow);

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence_txt = g_strdup(gtk_entry_get_text(GTK_ENTRY(main_frame->sequence_widg))) ;
  dataset_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->dataset_widg)) ;
  start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->start_widg)) ;
  end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->end_widg)) ;

  file_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->file_widg)) ;
  req_sequence_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;
  source_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->source_widg)) ;
  assembly_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->assembly_widg)) ;
  strand_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->strand_widg)) ;
  remap_features = gtk_toggle_button_get_active((GtkToggleButton*)main_frame->map_widg) ;

  /* Validate the user-specified arguments. These calls set status to false if there is a problem
   * and also set the err_msg. The start/end/strand functions also set the int from the string. 
   */
  validateFile(status, err_msg, file_txt) ;
  validateSequence(status, err_msg, sequence_txt, start_txt, end_txt, start, end) ;
  validateReqSequence(status, err_msg, req_sequence_txt, req_start_txt, req_end_txt, req_start, req_end, start) ;
  validateFileType(status, err_msg, file_type, source_txt, 
                   assembly_txt, dataset_txt, remap_features, 
                   have_assembly, have_dataset, sequence_txt) ;
  validateStrand(status, err_msg, file_type, strand_txt, strand) ;

  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (status)
    {
      bool create_source_data = FALSE ;

      /* Create the source */
      ZMapConfigSource source = NULL ;

      if (file_type == ZMAPSOURCE_FILE_GFF)
        {
          /* Just load the file directly */
          source = sequence_map->createFileSource(source_txt, file_txt) ;
        }
      else
        {
          /* Use a pipe script that will remap the features if necessary. This is also required
           * in order to be able to blixem the columns because blixem doesn't support bam reading
           * at the moment, so the script command gets passed through to blixem. */
          std::string script_err("");
          const char *script = fileTypeGetPipeScript(file_type, script_err) ;

          GString *args = g_string_new("") ;

          g_string_append_printf(args, "--file=%s&--chr=%s&--start=%d&--end=%d&--gff_seqname=%s&--gff_source=%s",
                                 file_txt, sequence_txt, req_start, req_end, req_sequence_txt, source_txt) ;

          if (file_type == ZMAPSOURCE_FILE_BIGWIG)
            g_string_append_printf(args, "&--strand=%d", strand) ;

          if (remap_features)
            g_string_append_printf(args, "&--csver_remote=%s&--dataset=%s", assembly_txt, dataset_txt) ;

          if (script)
            {
              /* Ok, create the source for this pipe scripe */
              source = sequence_map->createPipeSource(source_txt, file_txt, script, args->str) ;
              create_source_data = TRUE ;
            }
          else if (!remap_features)
            {
              /* No script; if not remapping features, we can load directly with htslib
               * instead. There are a couple of problems with this, though:
               * - the given source name will be ignored and therefore the source_data won't
               *   be linked correctly. This means it won't be recognised as a bam.
               * - blixem won't work (until it can load bam files directly, or unless the user
               *   has preconfigured the source in the blixem config). */
#ifdef USE_HTSLIB
              zMapLogMessage("%s", "Pipe script not available; using htslib") ;
              source = sequence_map->createFileSource(source_txt, file_txt) ;
#else
              err_msg = "Cannot load features; pipe script not available: " ;
              err_msg += script_err ;
#endif
            }
          else
            {
              err_msg = "Cannot remap features: " ;
              err_msg += script_err ;
            }

          g_string_free(args, TRUE) ;
        }

      if (create_source_data)
        {
          /* If we're using a pipe script, we need to create the source data for the named source
           * so that we can set is_seq=true for bam sources (otherwise we don't know when we come
           * to parse the results of the bam_get script that is_seq should be true).
           * We don't do this if we're loading the file directly because the named source is not used
           * (the source data is created when the features are parsed from the file instead) */
          ZMapFeatureContextMap context_map = zMapViewGetContextMap(view) ;

          GQuark fset_id = zMapFeatureSetCreateID(source_txt);
          ZMapFeatureSource source_data = context_map->getSource(fset_id);

          if (!source_data)
            {
              GQuark source_id = fset_id ;
              GQuark style_id = zMapStyleCreateID(NULL) ;
              GQuark source_text = g_quark_from_string(source_txt) ;
              const bool is_seq = (file_type == ZMAPSOURCE_FILE_BAM || file_type == ZMAPSOURCE_FILE_BIGWIG) ;

              context_map->createSource(fset_id,
                                        source_id, source_text, style_id,
                                        0, 0, is_seq) ;
            }
        }

      // Create a server connection for the source and start getting the data.
      if (source)
        {
          GList *req_featuresets = NULL ;
          const string url(source->url) ;
          ZMapFeatureContextMap context_map ;
          const string config_file = string("") ;
          const string version = string("") ;
          ZMapFeatureContext context ;


          // Try a feature request
          context_map = zMapViewGetContextMap(view) ;
          context = zMapViewCreateContext(view, req_featuresets, NULL) ;

          DataSourceFeatures *my_feature_request = new DataSourceFeatures(sequence_map, req_start, req_end,
                                                                          url, config_file, version,
                                                                          context, &(context_map->styles)) ;

          if (!(my_feature_request->SendRequest(newCallBackFunc, view)))
            {
              // Really need to be able to get some error state here....should throw from the obj ?

              delete my_feature_request ;
            }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          // Try a sequence request
          GList *dna_req_featuresets = NULL ;
          GQuark dna_id = g_quark_from_string("dna") ;

          dna_req_featuresets = g_list_append(dna_req_featuresets, GINT_TO_POINTER(dna_id)) ;

          context_map = zMapViewGetContextMap(view) ;
          context = zMapViewCreateContext(view, dna_req_featuresets, NULL) ;

          const string acedb_url("acedb://any:any@gen1b:20000?use_methods=false&gff_version=2") ;

          DataSourceSequence *my_sequence_request = new DataSourceSequence(sequence_map, req_start, req_end,
                                                                           acedb_url, config_file, version,
                                                                           context, &(context_map->styles)) ;

          if (!(my_sequence_request->SendRequest(newSequenceCallBackFunc, my_sequence_request)))
            {
              // Really need to be able to get some error state here....should throw from the obj ?

              delete my_sequence_request ;
            }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



        }
      else
        {
          zMapWarning("Failed to set up source for file '%s': %s", file_txt, err_msg.c_str()) ;
        }
    }
  else
    {
      zMapWarning("Error: \"%s\"", err_msg.c_str()) ;
    }

  if (sequence_txt)
    g_free(sequence_txt) ;

  return ;
}



static void newCallBackFunc(DataSourceFeatures *features_source, void *user_data)
{
  ZMapView view = (ZMapView)user_data ;
  ZMapFeatureContext context ;
  ZMapStyleTree *styles ;
  int error_rc = 0 ;
  const char *err_msg = NULL ;
  DataSourceReplyType reply ;

  cout << "in features app callback" << endl ;

  // Get the reply data......
  if ((reply = features_source->GetReply(&context, &styles)) == DataSourceReplyType::GOT_DATA)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      // GError *error = NULL ;
      ZMapFeatureContextMergeStats merge_stats = NULL ;
      bool acedb_source = false ;
      GList *masked = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      // zMapFeatureDumpStdOutFeatures(context, styles, &error) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMapFeatureContextMergeCode merge_results = ZMAPFEATURE_CONTEXT_ERROR ;
              
      merge_results = zmapJustMergeContext(view,
                                           &context, &merge_stats,
                                           &masked, acedb_source, TRUE) ;

      // Do something with merge stats ??

      g_free(merge_stats) ;

      if (merge_results == ZMAPFEATURE_CONTEXT_OK)
        {
          diff_context = context ;

          zmapJustDrawContext(view, diff_context, masked, NULL, NULL) ;

          result = TRUE ;
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      ZMapFeatureContextMergeCode merge_results ;

      merge_results = zMapViewContextMerge(view, context) ;

    }
  else if (reply == DataSourceReplyType::GOT_ERROR)
    {
      if (features_source->GetError(&err_msg, &error_rc))
        zMapWarning("Source failed to get features with: \"%s\"", err_msg) ;
    }
  else if (reply == DataSourceReplyType::WAITING)
    {
      zMapWarning("%s", "We are waiting...") ;
    }

  delete features_source ;

  return ;
}


static void newSequenceCallBackFunc(DataSourceSequence *sequence_source, void *user_data)
{
  ZMapFeatureContext context ;
  ZMapStyleTree *styles ;
  int error_rc = 0 ;
  const char *err_msg = NULL ;
  DataSourceReplyType reply ;

  cout << "in sequence app callback" << endl ;


  // Get the reply data......
  if ((reply = sequence_source->GetReply(&context, &styles)) == DataSourceReplyType::GOT_DATA)
    {
      GError *error = NULL ;

      zMapFeatureDumpStdOutFeatures(context, styles, &error) ;
    }
  else if (reply == DataSourceReplyType::GOT_ERROR)
    {
      if (sequence_source->GetError(&err_msg, &error_rc))
        zMapWarning("Source failed to get features with: \"%s\"", err_msg) ;
    }
  else if (reply == DataSourceReplyType::WAITING)
    {
      zMapWarning("%s", "We are waiting...") ;
    }

  delete sequence_source ;

  return ;
}
