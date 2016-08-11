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

#include <gbtools/gbtools.hpp>

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
  //GtkWidget *req_sequence_widg ;
  GtkWidget *req_start_widg ;
  GtkWidget *req_end_widg ;


  gboolean is_otter;
  char *chr;

  ZMapFeatureSequenceMap sequence_map;

  ZMapControlImportFileCB user_func ;
  gpointer user_data ;
} MainFrameStruct, *MainFrame ;




static MainFrame makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                      ZMapControlImportFileCB user_func, gpointer user_data,
                      ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *seq, int start, int end);

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(ZMapFeatureSequenceMap sequence_map, 
                         const bool recent_only,
                         gpointer cb_data) ;

#ifdef NOT_USED
static void sequenceCB(GtkWidget *widget, gpointer cb_data) ;
#endif

static void clearRecentSources(ZMapFeatureSequenceMap sequence_map) ;



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

  /* First tiem around, clear the 'recent' flag from all sources so that we don't show sources
   * that were loaded on startup. This won't be ideal if the user opens the import dialog before
   * all sources have finished loading. In that case though they can clear the list manually. */
  static bool first_time = true ;
  if (first_time)
    {
      clearRecentSources(sequence_map) ;
      first_time = false ;
    }

  toplevel = zMapGUIDialogNew(NULL, "Import data from a source", NULL, NULL) ;

  int width = 0 ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(toplevel, &width, &height) && GTK_IS_WINDOW(toplevel))
    gtk_window_set_default_size(GTK_WINDOW(toplevel), std::min(640.0, width * 0.5), std::min(400.0, height * 0.7)) ;

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


/* Utility to clear the 'recent' flag from all sources */
static void clearRecentSources(ZMapFeatureSequenceMap sequence_map)
{
  if (sequence_map && sequence_map->sources)
    {
      for (auto iter : *sequence_map->sources)
        iter.second->recent = false ;
    }
}


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

  options_box = makeOptionsBox(main_data, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, FALSE, FALSE, 0) ;

  sequence_box = zMapCreateSequenceViewWidg(importFileCB, main_data, sequence_map, TRUE, TRUE, toplevel) ;
  gtk_box_pack_start(GTK_BOX(vbox), sequence_box, TRUE, TRUE, 0) ;

  return main_data ;
}


/* Make the option buttons frame. */
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *req_sequence, int req_start, int req_end)
{
  GtkWidget *frame = NULL ;
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

  frame = gtk_frame_new("Import settings") ;
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

  //label = gtk_label_new( "Sequence " ) ;
  //tk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  //gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;



  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  //main_frame->req_sequence_widg = entry = gtk_entry_new() ;
  //gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  //gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  //gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  //gtk_widget_set_tooltip_text(main_frame->req_sequence_widg, 
  //                            "The sequence name to look for in the source (if different to the name in ZMap)") ;

  main_frame->req_start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (start ? start : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(main_frame->req_start_widg, 
                              "The range to import data for (defaults to the mark, if set, or the full ZMap range if not)") ;

  main_frame->req_end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (end ? end : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(main_frame->req_end_widg, 
                              "The range to import data for (defaults to the mark, if set, or the full ZMap range if not)") ;



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



static void validateReqSequence(bool &status,
                                std::string &err_msg,
                                //const char *req_sequence_txt, 
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
      if (/*req_sequence_txt &&*/ *req_start_txt && *req_end_txt)
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


/* Recursively import a source and any child sources */
static void importSource(ZMapConfigSource server,
                         ZMapView view,
                         const int req_start,
                         const int req_end,
                         const bool recent_only)
{
  if (!recent_only || server->recent)
    {
      GError *g_error = NULL ;
      zMapViewSetUpServerConnection(view, server, req_start, req_end, false, &g_error) ;

      if (g_error)
        {
          zMapWarning("Failed to set up server connection for '%s': %s", 
                      g_quark_to_string(server->name_),
                      (g_error ? g_error->message : "<no error>")) ;
        }

      // Recurse through children
      for (auto child : server->children)
        {
          importSource(child, view, req_start, req_end, recent_only) ;
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
static void importFileCB(ZMapFeatureSequenceMap sequence_map, 
                         const bool recent_only,
                         gpointer cb_data)
{
  bool status = TRUE ;
  MainFrame main_frame = (MainFrame)cb_data ;
  zMapReturnIfFail(main_frame) ;

  std::string err_msg("") ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  //const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  ZMapView view = NULL ;
  ZMap zmap = NULL ;


  zmap = (ZMap) main_frame->user_data;
  zMapReturnIfFail(zmap) ;
  zMapReturnIfFail(sequence_map) ;

  /* Check that the zmap window hasn't been closed (currently the dialog
   * isn't closed automatically with it, so we'll have problems if we don't
   * exit early here) */
  if (!zmap->toplevel)
    return;

  view = zMapViewGetView(zmap->focus_viewwindow);

  //req_sequence_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;

  /* Validate the user-specified arguments. These calls set status to false if there is a problem
   * and also set the err_msg. The start/end/strand functions also set the int from the string. 
   */
  validateReqSequence(status, err_msg, 
                      //req_sequence_txt, 
                      req_start_txt, req_end_txt, 
                      req_start, req_end, start) ;

  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (status)
    {
      
//      if (create_source_data)
//        {
//          /* If we're using a pipe script, we need to create the source data for the named source
//           * so that we can set is_seq=true for bam sources (otherwise we don't know when we come
//           * to parse the results of the bam_get script that is_seq should be true).
//           * We don't do this if we're loading the file directly because the named source is not used
//           * (the source data is created when the features are parsed from the file instead) */
//          ZMapFeatureContextMap context_map = zMapViewGetContextMap(view) ;
//
//          GQuark fset_id = zMapFeatureSetCreateID(source_txt);
//          ZMapFeatureSource source_data = context_map->getSource(fset_id);
//
//          if (!source_data)
//            {
//              GQuark source_id = fset_id ;
//              GQuark style_id = zMapStyleCreateID(NULL) ;
//              GQuark source_text = g_quark_from_string(source_txt) ;
//              const bool is_seq = (file_type == ZMAPSOURCE_FILE_BAM || file_type == ZMAPSOURCE_FILE_BIGWIG) ;
//
//              context_map->createSource(fset_id,
//                                        source_id, source_text, style_id,
//                                        0, 0, is_seq) ;
//            }
//        }

      for (auto &iter : *sequence_map->sources)
        {
          importSource(iter.second, view, req_start, req_end, recent_only) ;
        }
    }
  else
    {
      zMapWarning("Error: \"%s\"", err_msg.c_str()) ;
    }

  return ;
}

  
