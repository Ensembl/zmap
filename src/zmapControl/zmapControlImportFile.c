/*  File: zmapControlImportFile.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2012-2014: Genome Research Ltd.
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
 * Exported functions: See ZMap/zmapControlImportFile.h
 *
 * NOTE this file was initially copied from zmapAppSequenceView.c
 * and then tweaked. There may be some common code & functions
 * but maybe this file will be volatile for a while
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapString.h>    /* testing only. */

#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStanzaStructs.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapControlImportFile.h>
#include <zmapControl_P.h>


/* number of optional dialog entries for FILE_NONE (is really 8 so i allowed a few spare) */
#define N_ARGS 16


typedef enum
  {
    FILE_NONE,
    FILE_GFF,
    FILE_BAM,
    FILE_BIGWIG
  } fileType;

/*
 * was used for input script description, args etc
 */
typedef struct
{
  fileType file_type;
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


  fileType file_type;

  gboolean is_otter;
  char *chr;

  ZMapFeatureSequenceMap sequence_map;

  ZMapControlImportFileCB user_func ;
  gpointer user_data ;
} MainFrameStruct, *MainFrame ;




static MainFrame makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                      ZMapControlImportFileCB user_func, gpointer user_data,
                      ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, char *seq, int start, int end);
static void makeButtonBox(GtkWidget *toplevel, MainFrame main_frame) ;

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(gpointer cb_data) ;
#ifndef __CYGWIN__
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
#endif
static void closeCB(gpointer cb_data) ;

static void sequenceCB(GtkWidget *widget, gpointer cb_data) ;

static void fileChangedCB(GtkWidget *widget, gpointer user_data);




/*
 *                   External interface routines.
 */

/* \brief Handles the response to the import dialog */
void importDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data)
{
  switch (response_id)
  {
    case GTK_RESPONSE_ACCEPT:
    case GTK_RESPONSE_APPLY:
    case GTK_RESPONSE_OK:
      importFileCB(data) ;
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:
      closeCB(data) ;
      break;

    default:
      break;
  };
}


/* Display a dialog to get from the reader a file to be displayed
 * with a optional start/end and various mapping parameters
 */
void zMapControlImportFile(ZMapControlImportFileCB user_func, gpointer user_data,
    ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel ;
  gpointer seq_data = NULL ;
  MainFrame main_frame = NULL ;

  /* if (!user_func)
    return ; */
  zMapReturnIfFail(user_func && user_data) ;

  ZMap zmap = (ZMap)user_data;

  toplevel = zMapGUIDialogNew(NULL, "Please choose a file to import.", NULL, NULL) ;

  /* Make sure the dialog is destroyed if the zmap window is closed */
  /*! \todo For some reason this doesn't work and the dialog hangs around
   * after zmap->toplevel is destroyed */
  if (zmap && zmap->toplevel && GTK_IS_WINDOW(zmap->toplevel))
    gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(zmap->toplevel)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  main_frame = makePanel(toplevel, &seq_data, user_func, user_data, sequence_map, req_start, req_end) ;

  g_signal_connect(G_OBJECT(toplevel), "response", G_CALLBACK(importDialogResponseCB), main_frame) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

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
  ZMapImportScript s;
  ZMapConfigIniContext context;
  fileType file_type = FILE_NONE ;
  /* in parallel with the filetype enum */
  int i;
  char *tmp_string;
  /*ZMapImportScript scripts = main_frame->scripts;*/

  /* make some defaults for the file types we know to allow running unconfigured */
  /*for( i = 0;  i < N_FILE_TYPE; i++)
    {
      s = scripts + i;
      s->file_type = (fileType) i;
      s->script = NULL;
      s->args = NULL;
      s->allocd = NULL;
    }*/
  /*scripts[2].args = g_strsplit("--fruit=apple --car=jeep --weather=sunny", " ", 0); */

  if ((context = zMapConfigIniContextProvide(config_file)))
    {
      GKeyFile *gkf;
      gchar ** keys,**freethis;
      gchar **args, **argp;
      char *arg_str;
      char *ft;
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
                file_type = FILE_GFF;
              else if(!g_ascii_strncasecmp(ft,"BAM",3))
                file_type = FILE_BAM;
              else if(!g_ascii_strncasecmp(ft,"BIGWIG",6))
                file_type = FILE_BIGWIG;
              else
                continue;*/

              /*s = scripts + file_type;
              s->script = g_strdup(*keys);
              s->args = argp;
              s->allocd = args;*/
            }
        }
    }

  /*for( i = 0; i < N_FILE_TYPE; i++)
    {
      s = scripts + i;
      if(!s->script)
        s->script = g_strdup(default_scripts[i]);
    }
  */
  /*scripts[2].args = g_strsplit("--fruit=apple --car=jeep --weather=sunny", " ", 0); */

  return ;
}


/* Make the whole panel returning the top container of the panel. */
static MainFrame makePanel(GtkWidget *toplevel, gpointer *our_data,
                           ZMapControlImportFileCB user_func, gpointer user_data,
                           ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *vbox, *main_frame, *options_box;
  MainFrame main_data ;
  char *sequence = "";
  GtkDialog *dialog = GTK_DIALOG(toplevel) ;

  vbox = dialog->vbox ;

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->user_func = user_func ;
  main_data->user_data = user_data ;

  importGetConfig(main_data, sequence_map->config_file );

  if (toplevel)
    {
      main_data->toplevel = toplevel ;
      *our_data = main_data ;
    }

  main_frame = makeMainFrame(main_data, sequence_map) ;
  gtk_box_pack_start(GTK_BOX(vbox), main_frame, TRUE, TRUE, 0) ;

  main_data->sequence_map = sequence_map;
  if(sequence_map)
    sequence = sequence_map->sequence;/* request defaults to original */

  options_box = makeOptionsBox(main_data, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, TRUE, TRUE, 0) ;

  makeButtonBox(toplevel, main_data) ;

  return main_data ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *button ;
  char *sequence = "", *start = "", *end = "", *dataset = "" ;

  if (sequence_map)
    {
      if (sequence_map->sequence)
        sequence = sequence_map->sequence ;
      if (sequence_map->dataset)
        dataset = sequence_map->dataset ;
      if (sequence_map->start)
        start = g_strdup_printf("%d", sequence_map->start) ;
      if (sequence_map->end)
        end = g_strdup_printf("%d", sequence_map->end) ;
    }

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
  gtk_entry_set_text(GTK_ENTRY(entry), start) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  main_frame->end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), end) ;
  //  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_sensitive(GTK_WIDGET(entry),FALSE);

  /*
  ZMap zmap = (ZMap) main_frame->user_data;
  ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow);

  if(zMapWindowMarkIsSet(window))
    {
      hbox = gtk_hbox_new(FALSE, 0) ;
      gtk_container_border_width(GTK_CONTAINER(hbox), 0);
      gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

      main_frame->whole_widg = button = gtk_button_new_with_label("Whole Sequence") ;
      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
                         GTK_SIGNAL_FUNC(sequenceCB), (gpointer)main_frame) ;

      main_frame->mark_widg = button = gtk_button_new_with_label("Use Mark") ;
      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
      GTK_SIGNAL_FUNC(sequenceCB), (gpointer)main_frame) ;
    }
  */

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
static GtkWidget *makeOptionsBox(MainFrame main_frame, char *req_sequence, int req_start, int req_end)
{
  GtkWidget *frame = NULL ;
  GtkWidget *map_seq_button = NULL , *config_button = NULL ;
  GtkWidget *topbox = NULL, *hbox = NULL, *entrybox = NULL, *labelbox = NULL, *entry = NULL, *label = NULL ;
  char *sequence = "", *start = "", *end = "", *file = "" ;
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

  /*
  label = gtk_label_new( "Script " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  */

  /*
  label = gtk_label_new( "Extra Parameters " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  */

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

  /*label = gtk_label_new( "Style " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;*/

  label = gtk_label_new( "Map Features " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  /*label = gtk_label_new( "Sequence Offset " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;*/



  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->file_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), file) ;
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
     GTK_SIGNAL_FUNC(fileChangedCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  /*
  main_frame->script_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
     GTK_SIGNAL_FUNC(scriptChangedCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  */

  /*
  main_frame->args_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  */

  main_frame->req_sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  main_frame->req_start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), start) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  main_frame->req_end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), end) ;
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

  /*main_frame->style_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ; */


  main_frame->map_widg = map_seq_button = gtk_check_button_new () ;
  gtk_toggle_button_set_active((GtkToggleButton*)map_seq_button, FALSE) ;
  gtk_box_pack_start(GTK_BOX(entrybox), map_seq_button, FALSE, TRUE, 0) ;

  /*main_frame->offset_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;*/


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
  gtk_dialog_add_button(dialog, GTK_STOCK_OPEN, GTK_RESPONSE_OK) ;

  gtk_dialog_set_default_response(dialog, GTK_RESPONSE_OK) ;


  /*
  GtkDialog *dialog = GTK_DIALOG(toplevel) ;

  gtk_dialog_add_button(dialog, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL) ;
  gtk_dialog_add_button(dialog, GTK_STOCK_OPEN, GTK_RESPONSE_OK) ;

  create_button = gtk_button_new_with_label("Import File") ;
  gtk_signal_connect(GTK_OBJECT(create_button), "clicked",
     GTK_SIGNAL_FUNC(importFileCB), (gpointer)main_frame) ;
  gtk_box_pack_start(GTK_BOX(button_box), create_button, FALSE, TRUE, 0) ;


  if (main_frame->toplevel)
    {
      close_button = gtk_button_new_with_label("Close") ;
      gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
 GTK_SIGNAL_FUNC(closeCB), (gpointer)main_frame) ;
      gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, TRUE, 0) ;

      GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
      gtk_window_set_default(GTK_WINDOW(main_frame->toplevel), create_button) ;
    }

  return frame ;
  */
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;
  int i;

  /*for(i = 0; i < N_FILE_TYPE;i++)
    {
      if(main_frame->scripts[i].script)
        g_free(main_frame->scripts[i].script);

      if(main_frame->scripts[i].allocd)
        g_strfreev(main_frame->scripts[i].allocd);
    }
  */

  return ;
}


/* Kill the dialog. */
static void closeCB(gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;

  gtk_widget_destroy(main_frame->toplevel) ;

  return ;
}


/* set start, end as requested */
static void sequenceCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_frame = (MainFrame)cb_data ;
  int start, end;
  char buf[32];

  /* NOTE we get -fsd coords from this function if revcomped */
  /*if(widget == main_frame->mark_widg)
    {
      ZMap zmap = (ZMap) main_frame->user_data;
      ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow);

      zMapWindowGetMark(window, &start, &end);

      if(start < 0)
        start = -start;
      if(end < 0)
        end = -end;

      start += main_frame->sequence_map->start;
      end   += main_frame->sequence_map->start;
    }*/
  /*else*/
  /*if(widget == main_frame->whole_widg)
    {
      start = main_frame->sequence_map->start;
      end   = main_frame->sequence_map->end;
    }
  else
    {
      return;
    }
  */

  sprintf(buf,"%d",start);
  gtk_entry_set_text(GTK_ENTRY( main_frame->req_start_widg), buf) ;
  sprintf(buf,"%d",end);
  gtk_entry_set_text(GTK_ENTRY( main_frame->req_end_widg),   buf) ;
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

  gboolean is_gff = main_frame->file_type == FILE_GFF,
    is_bam = main_frame->file_type == FILE_BAM,
    is_big = main_frame->file_type == FILE_BIGWIG,
    is_otter = main_frame->is_otter ;

  /*
   * We only allow some of the widgets to be set under the
   * following conditions.
   */
  gtk_widget_set_sensitive(main_frame->strand_widg, is_big );
  gtk_widget_set_sensitive(main_frame->source_widg, !is_gff ) ;
  gtk_widget_set_sensitive(main_frame->assembly_widg, !is_gff && is_otter ) ;
  gtk_widget_set_sensitive(main_frame->map_widg, (is_bam || is_big) && is_otter) ;

  /*gtk_widget_set_sensitive(main_frame->args_widg, main_frame->file_type == FILE_NONE ); */

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
  fileType file_type = FILE_NONE ;
  char *source_txt = NULL;
  char *style_txt = NULL;
  /*ZMapFeatureSource src;
  ZMapView view;
  ZMap zmap = (ZMap) main_frame->user_data;*/

  /*
  char *args_txt = "";
  ZMapImportScript scripts;
  */

  /* Check that the zmap window has not been closed (currently
   * the import dialog isn't closed with it, which will cause
   * problems if we don't exit early here )
   */
  /*if (!zmap->toplevel)
    return;
  view = zMapViewGetView(zmap->focus_viewwindow);*/

  /*
   * Inspect the file extension to determine type.
   * Case is ignored.
   */
  filename = (char *) gtk_entry_get_text(GTK_ENTRY(widget)) ;
  extent = filename + strlen(filename);

  while(*extent != '.' && extent > filename)
    extent--;
  if (!g_ascii_strcasecmp(extent,".gff"))
    file_type = FILE_GFF ;
  else if (!g_ascii_strcasecmp(extent,".bam"))
    file_type = FILE_BAM ;
  else if (!g_ascii_strcasecmp(extent,".sam"))
    file_type = FILE_BAM ;
  else if(!g_ascii_strcasecmp(extent,".bigwig"))
    file_type = FILE_BIGWIG ;

  /*
   * Set the file type in the struct.
   */
  main_frame->file_type = file_type;

  gtk_entry_set_text(GTK_ENTRY(main_frame->strand_widg), "")  ;
  gtk_entry_set_text(GTK_ENTRY(main_frame->source_widg), "")  ;
  gtk_entry_set_text(GTK_ENTRY(main_frame->assembly_widg), "")  ;

  gtk_toggle_button_set_active((GtkToggleButton*)main_frame->map_widg, FALSE) ;

  /*
   *Try to get a source name for non-GFF types.
   */
  if (main_frame->file_type == FILE_GFF)
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
  if(main_frame->file_type == FILE_BAM)
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
  if(main_frame->file_type == FILE_BIGWIG)
    {
      /*GQuark f_id;*/
      char * strand_txt = NULL ;

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
static void importFileCB(gpointer cb_data)
{
  static const char * string_zmap = "[ZMap]\nsources = temp\n\n[temp]\nfeaturesets=\n" ;
  static const char * format_string_gff = "%surl=file:///%s" ;
  static const char * format_string_bam01 =
    "%surl=pipe:///bam_get?--chr=%s&--csver_remote=%s&--dataset=%s&--start=%i&--end=%i&--gff_seqname=%s&--file=%s&--gff_source=%s&--gff_version=3" ;
  static const char * format_string_bam02 =
    "%surl=pipe:///bam_get?--chr=%s&--start=%i&--end=%i&--gff_seqname=%s&--file=%s&--gff_source=%s&--gff_version=3" ;
  static const char * format_string_bigwig01 =
    "%surl=pipe:///bigwig_get?--chr=%s&--csver_remote=%s&--dataset=%s&--start=%i&--end=%i&--gff_seqname=%s&--file=%s&--gff_source=%s&--strand=%i&--gff_version=3" ;
  static const char * format_string_bigwig02 =
    "%surl=pipe:///bigwig_get?--chr=%s&--start=%i&--end=%i&--gff_seqname=%s&--file=%s&--gff_source=%s&--strand=%i&--gff_version=3" ;
  gboolean status = TRUE,
    have_dataset = FALSE,
    have_assembly = FALSE,
    remap_features = TRUE ;
  MainFrame main_frame = NULL ;
  char *err_msg = NULL,
    *sequence_txt = NULL,
    *dataset_txt = NULL,
    *start_txt = NULL,
    *end_txt = NULL,
    *file_txt = NULL,
    *req_start_txt= NULL,
    *req_end_txt = NULL,
    *source_txt = NULL,
    *strand_txt = NULL,
    *assembly_txt = NULL,
    *req_sequence_txt = NULL ;
  int start = 0,
    end = 0,
    req_start = 0,
    req_end = 0,
    strand = 0 ;
  fileType file_type = FILE_NONE ;
  ZMapView view = NULL ;
  ZMap zmap = NULL ;

  main_frame = (MainFrame)cb_data ;
  zMapReturnIfFail(main_frame) ;
  file_type = main_frame->file_type;
  zmap = (ZMap) main_frame->user_data;
  zMapReturnIfFail(zmap) ;

  /* Check that the zmap window hasn't been closed (currently the dialog
   * isn't closed automatically with it, so we'll have problems if we don't
   * exit early here) */
  if (!zmap->toplevel)
    return;

  view = zMapViewGetView(zmap->focus_viewwindow);

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->sequence_widg)) ;
  dataset_txt = (char*) gtk_entry_get_text(GTK_ENTRY(main_frame->dataset_widg)) ;
  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->start_widg)) ;
  end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->end_widg)) ;

  file_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->file_widg)) ;
  req_sequence_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;
  source_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->source_widg)) ;
  assembly_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->assembly_widg)) ;
  strand_txt = (char *)gtk_entry_get_text(GTK_ENTRY(main_frame->strand_widg)) ;
  remap_features = gtk_toggle_button_get_active((GtkToggleButton*)main_frame->map_widg) ;

  if (status)
    {
      if (!(*file_txt))
        {
          status = FALSE ;
          err_msg = "Please choose a file to import." ;
        }
    }

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

  /*
   * Error conditions and data combinations are complex enough here to be treated
   * seperately for each data type.
   */
  if (status)
    {

      if (file_type == FILE_GFF)
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
      else if (main_frame->is_otter && ((file_type == FILE_BAM) || (file_type == FILE_BIGWIG)))
        {
          /* note that we must be hooked up to otter to do these types,
             hence "main_frame->is_otter" is required */

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

            }

          /* strand is required for bigwig only */
          if (status && (file_type == FILE_BIGWIG))
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
          else if (status && (file_type == FILE_BAM))
            {
              if (strlen(strand_txt) != 0)
                {
                  status = FALSE ;
                  err_msg = "Strand should not be specified for this type." ;
                }
            }

        }
      else
        {
          status = FALSE ;
          err_msg = "Unknown file type." ;
        }

    } /* if (status) */


  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (!status)
    {
      zMapWarning("Error: \"%s\"", err_msg) ;
    }
  else
    {
      char *config_str = NULL ;
      GList * servers = NULL ;
      ZMapConfigSource server = NULL ;
      GList *req_featuresets = NULL ;

      /*if (main_frame->sequence_map && (seq_offset || map_seq))
        seq_offset += main_frame->sequence_map->start;*/

      /*
       * (sm23) In the original version, this was only done for non-GFF
       * sources. No idea why!
       */
      if (file_type != FILE_GFF)
        {
          /* add featureset_2_style entry to the view */
          ZMapFeatureSource src;
          GQuark f_id;

          f_id = zMapFeatureSetCreateID(source_txt);
          src = zMapViewGetFeatureSetSource(view, f_id);
          if (!src)
            {
              src = g_new0(ZMapFeatureSourceStruct,1);
              src->source_text = g_quark_from_string(source_txt);
              src->source_id = f_id;
              zMapViewSetFeatureSetSource(view, f_id, src);
            }

          src->style_id = zMapStyleCreateID(NULL);
          src->is_seq = TRUE;
        }


      if (file_type == FILE_GFF)
        {
          config_str = g_strdup_printf(format_string_gff, string_zmap, file_txt) ;
        }
      else if (file_type == FILE_BAM)
        {
          if (remap_features)
            {
              config_str = g_strdup_printf(format_string_bam01,
                                           string_zmap,
                                           sequence_txt,
                                           assembly_txt,
                                           dataset_txt,
                                           req_start,
                                           req_end,
                                           req_sequence_txt,
                                           file_txt,
                                           source_txt) ;
            }
          else
            {
              config_str = g_strdup_printf(format_string_bam02,
                                           string_zmap,
                                           sequence_txt,
                                           req_start,
                                           req_end,
                                           req_sequence_txt,
                                           file_txt,
                                           source_txt) ;
            }
        }
      else if (file_type == FILE_BIGWIG)
        {
          if (remap_features)
            {
              config_str = g_strdup_printf(format_string_bigwig01,
                                           string_zmap,
                                           sequence_txt,
                                           assembly_txt,
                                           dataset_txt,
                                           req_start,
                                           req_end,
                                           req_sequence_txt,
                                           file_txt,
                                           source_txt,
                                           strand) ;
            }
          else
            {
              config_str = g_strdup_printf(format_string_bigwig02,
                                           string_zmap,
                                           sequence_txt,
                                           req_start,
                                           req_end,
                                           req_sequence_txt,
                                           file_txt,
                                           source_txt,
                                           strand) ;
            }
        }

      if (*config_str)
        {
          servers = zmapViewGetIniSources(NULL, config_str, NULL) ;

          server = (ZMapConfigSource) servers->data ;

          if (zMapViewRequestServer(view, NULL, req_featuresets, (gpointer)server, req_start, req_end, FALSE, TRUE, TRUE))
            zMapViewShowLoadStatus(view);
          else
            zMapWarning("could not request %s",file_txt);

          zMapConfigSourcesFreeList(servers);

          g_free(config_str);
        }
    }

  return ;
}



  /*
  if (status)
    {
      if ((*offset_txt) && !zMapStr2Int(offset_txt, &seq_offset))
        {
          status = FALSE ;
          err_msg = "Invalid offset specified." ;
        }
    }
  */

  /*
   * This was used to get the "mapping" flag.
   */
  /*if (status)
    {
      map_seq = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(main_frame->map_widg));
    }
  */


