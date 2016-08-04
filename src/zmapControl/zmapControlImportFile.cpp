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

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapString.hpp>    /* testing only. */

#include <config.h>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapControlImportFile.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <zmapControl_P.hpp>


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

  ZMapControlImportFileCB user_func ;
  gpointer user_data ;
} MainFrameStruct, *MainFrame ;




static MainFrame makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                      ZMapControlImportFileCB user_func, gpointer user_data,
                      ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *seq, int start, int end);

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(ZMapFeatureSequenceMap sequence_map, gpointer cb_data) ;
static void closeCB(gpointer cb_data) ;

#ifdef NOT_USED
static void sequenceCB(GtkWidget *widget, gpointer cb_data) ;
#endif

static void fileChangedCB(GtkWidget *widget, gpointer user_data);




/*
 *                   External interface routines.
 */

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

  toplevel = zMapGUIDialogNew(NULL, "Import data from a source", NULL, NULL) ;

  /* Make sure the dialog is destroyed if the zmap window is closed */
  /*! \todo For some reason this doesn't work and the dialog hangs around
   * after zmap->toplevel is destroyed */
  if (zmap && zmap->toplevel && GTK_IS_WINDOW(zmap->toplevel))
    gtk_window_set_transient_for(GTK_WINDOW(toplevel), GTK_WINDOW(zmap->toplevel)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  main_frame = makePanel(toplevel, &seq_data, user_func, user_data, sequence_map, req_start, req_end) ;

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
static MainFrame makePanel(GtkWidget *toplevel, gpointer *our_data,
                           ZMapControlImportFileCB user_func, gpointer user_data,
                           ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *vbox, *sequence_box, *options_box;
  MainFrame main_data ;
  const char *sequence = "";
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

  main_data->sequence_map = sequence_map;
  if(sequence_map)
    sequence = sequence_map->sequence;/* request defaults to original */

  sequence_box = zMapCreateSequenceViewWidg(importFileCB, main_data, sequence_map, TRUE, FALSE) ;
  gtk_box_pack_start(GTK_BOX(vbox), sequence_box, TRUE, TRUE, 0) ;

  options_box = makeOptionsBox(main_data, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, TRUE, TRUE, 0) ;

  return main_data ;
}




/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;
  const char *sequence = "", *dataset = "" ;
  char *start = NULL, *end = NULL ;

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
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *req_sequence, int req_start, int req_end)
{
  GtkWidget *frame = NULL ;
  GtkWidget *map_seq_button = NULL ;
  GtkWidget *topbox = NULL, *hbox = NULL, *entrybox = NULL, *labelbox = NULL, *entry = NULL, *label = NULL ;
  const char *sequence = "" ;
  char *start = NULL, *end = NULL ;

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

  label = gtk_label_new( "Map Features " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;



  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

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



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{

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
static void importFileCB(ZMapFeatureSequenceMap sequence_map, gpointer cb_data)
{
  bool status = TRUE ;
  bool have_dataset = FALSE ;
  bool have_assembly = FALSE ;
  bool remap_features = TRUE ;
  MainFrame main_frame = NULL ;
  std::string err_msg("") ;
  char *sequence_txt = NULL ;
  const char *dataset_txt = NULL ;
  const char *file_txt = NULL ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  const char *source_txt = NULL ;
  const char *strand_txt = NULL ;
  const char *assembly_txt = NULL ;
  const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  int strand = 0 ;
  ZMapSourceFileType file_type = ZMAPSOURCE_FILE_NONE ;
  ZMapView view = NULL ;
  ZMap zmap = NULL ;

  main_frame = (MainFrame)cb_data ;
  zMapReturnIfFail(main_frame) ;
  file_type = main_frame->file_type;
  zmap = (ZMap) main_frame->user_data;
  zMapReturnIfFail(zmap) ;
  zMapReturnIfFail(sequence_map) ;

  /* Check that the zmap window hasn't been closed (currently the dialog
   * isn't closed automatically with it, so we'll have problems if we don't
   * exit early here) */
  if (!zmap->toplevel)
    return;

  view = zMapViewGetView(zmap->focus_viewwindow);

  /* Note gtk_entry returns the empty string "" _not_ NULL when there is no text. */
  sequence_txt = g_strdup(sequence_map->sequence) ;
  dataset_txt = sequence_map->dataset ;

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
          GError *g_error = NULL ;
          zMapViewSetUpServerConnection(view, server, req_start, req_end, false, &g_error) ;

          if (g_error)
            zMapWarning("Failed to set up server connection for file '%s': %s", file_txt, err_msg.c_str()) ;
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


