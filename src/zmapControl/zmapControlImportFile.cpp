/*  File: zmapControlImportFile.cpp
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
 * Exported functions: See zmapControl_P.hpp
 *
 * NOTE this file was initially copied from zmapAppSequenceView.c
 * and then tweaked. There may be some common code & functions
 * but maybe this file will be volatile for a while
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <iostream>

#include <gbtools/gbtools.hpp>
#include <ZMap/zmapString.hpp>    /* testing only. */
#include <config.h>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapFeatureLoadDisplay.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <ZMap/zmapDataStream.hpp>
#include <zmapControl_P.hpp>


using namespace std ;

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
  ZMap zmap ;

  GtkWidget *toplevel ;
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;
  /*GtkWidget *whole_widg ;*/
  GtkWidget *dataset_widg;
  GtkWidget *req_sequence_widg ;
  GtkWidget *req_start_widg ;
  GtkWidget *req_end_widg ;


  gboolean is_otter;
  char *chr;

  ZMapFeatureSequenceMap sequence_map;

} MainFrameStruct, *MainFrame ;




static void makePanel(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static GtkWidget *makeOptionsBox(MainFrame main_frame, const char *seq, int start, int end);

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void importFileCB(ZMapFeatureSequenceMap sequence_map, 
                         const bool recent_only,
                         gpointer cb_data) ;
static void fileImportCloseCB(GtkWidget *widget, gpointer cb_data) ;

static void clearRecentSources(ZMapFeatureSequenceMap sequence_map) ;



/*
 *                   External interface routines.
 */

/* Display a dialog to get from the reader a file to be displayed
 * with a optional start/end and various mapping parameters
 */
void zmapControlImportFile(ZMap zmap, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel ;
  MainFrame main_frame ;
  static bool first_time = true ;

  /* First time around, clear the 'recent' flag from all sources so that we don't show sources
   * that were loaded on startup. This won't be ideal if the user opens the import dialog before
   * all sources have finished loading. In that case though they can clear the list manually. */
  if (first_time)
    {
      clearRecentSources(sequence_map) ;
      first_time = false ;
    }

  main_frame = g_new0(MainFrameStruct, 1) ;

  main_frame->zmap = zmap ;

  zmap->import_file_dialog = main_frame->toplevel = toplevel
    = zMapGUIDialogNew(NULL, "Import data from a source", NULL, NULL) ;

  int width = 0 ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(toplevel, &width, &height) && GTK_IS_WINDOW(toplevel))
    gtk_window_set_default_size(GTK_WINDOW(toplevel), std::min(640.0, width * 0.5), std::min(400.0, height * 0.7)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  makePanel(main_frame, sequence_map, req_start, req_end) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), main_frame) ;

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
static void makePanel(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *vbox, *sequence_box, *options_box;
  const char *sequence = "";
  GtkDialog *dialog = GTK_DIALOG(main_frame->toplevel) ;

  vbox = dialog->vbox ;

  importGetConfig(main_frame, sequence_map->config_file );

  main_frame->sequence_map = sequence_map;

  if(sequence_map)
    sequence = sequence_map->sequence;/* request defaults to original */

  options_box = makeOptionsBox(main_frame, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, FALSE, FALSE, 0) ;

  sequence_box = zMapCreateSequenceViewWidg(importFileCB, main_frame,
                                            fileImportCloseCB, main_frame,
                                            sequence_map, TRUE, TRUE, main_frame->toplevel) ;
  gtk_box_pack_start(GTK_BOX(vbox), sequence_box, TRUE, TRUE, 0) ;

  return ;
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

  label = gtk_label_new( "Sequence " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Start " ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);

  label = gtk_label_new( "End " ) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5);
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, FALSE, 0) ;



  /* Entries.... */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  main_frame->req_sequence_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), sequence) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;
  gtk_widget_set_tooltip_text(main_frame->req_sequence_widg, 
                              "The sequence name to look for in the source, if different to the name in ZMap\n\ne.g.\nOtter 'chr1-38'\nTrackhub 'chr1'\nEnsembl '1'") ;

  main_frame->req_start_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (start ? start : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(main_frame->req_start_widg, 
                              "The range to import data for (when you open the Import dialog this defaults to the mark, if set, or the full ZMap range if not)") ;

  main_frame->req_end_widg = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), (end ? end : "")) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(main_frame->req_end_widg, 
                              "The range to import data for (when you open the Import dialog this defaults to the mark, if set, or the full ZMap range if not)") ;



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
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  // Currently unused.
  MainFrame main_data = (MainFrame)cb_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


// called from the file import sub-dialog to say that it's closing, we then
// remove our reference to that dialog as we don't need to close it later.
static void fileImportCloseCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  // Reset record of our dialog in main zmap struct (used where user destroys zmap window while
  // this dialog is displayed).
  main_data->zmap->import_file_dialog = NULL ;

  return ;
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
      if (req_sequence_txt && *req_start_txt && *req_end_txt)
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


/* If we're using a pipe script, we need to create the source data for the named source so that we
 * can set is_seq=true for bam sources (otherwise we don't know when we come to parse the results
 * of the bam_get script that is_seq should be true).  We don't do this if we're loading the file
 * directly because the named source is not used (the source data is created when the features are
 * parsed from the file instead).
 * 
 * This is all a horrible hack for the bam_get stuff and should probably be dealt with elsewhere
 * but leaving it here for now to preserve the old behaviour. We will get rid of all the bam_get
 * stuff anyway when zmap can do its own remapping. */
static void createSourceData(ZMapView view, 
                             ZMapFeatureSequenceMap sequence_map, 
                             ZMapConfigSource source)
{
  zMapReturnIfFail(view && sequence_map) ;

  ZMapFeatureContextMap context_map = zMapViewGetContextMap(view) ;

  int status = 0 ;
  ZMapURL zmap_url = url_parse(source->url(), &status) ;

  if (context_map && sequence_map->runningUnderOtter() && zmap_url && zmap_url->path)
    {
      ZMapDataStreamType source_type = zMapDataStreamTypeFromFilename(zmap_url->path, NULL) ;

      if (source_type == ZMapDataStreamType::HTS || source_type == ZMapDataStreamType::BIGWIG)
        {
          const char *source_name = g_quark_to_string(source->name_) ;
          GQuark fset_id = zMapFeatureSetCreateID(source_name);
          ZMapFeatureSource source_data = context_map->getSource(fset_id);

          if (!source_data)
            {
              GQuark source_id = fset_id ;
              GQuark style_id = zMapStyleCreateID(NULL) ;
              GQuark source_text = source->name_ ;
              const bool is_seq = true ;
              
              context_map->createSource(fset_id,
                                        source_id, source_text, style_id,
                                        0, 0, is_seq) ;
            }
        }
    }
}


/* Recursively import a source and any child sources */
static void importSource(ZMapConfigSource server,
                         ZMapView view,
                         ZMapFeatureSequenceMap sequence_map,
                         const char *req_sequence,
                         const int req_start,
                         const int req_end,
                         const bool recent_only)
{
  if (!recent_only || server->recent)
    {
      createSourceData(view, sequence_map, server) ;

      GError *g_error = NULL ;
      zMapViewSetUpServerConnection(view, server, req_sequence, req_start, req_end, false, &g_error) ;

      if (g_error)
        {
          zMapWarning("Failed to set up server connection for '%s': %s", 
                      g_quark_to_string(server->name_),
                      (g_error ? g_error->message : "<no error>")) ;
        }

      // Recurse through children
      for (auto child : server->children)
        {
          importSource(child, view, sequence_map, req_sequence, req_start, req_end, recent_only) ;
        }
    }

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
static void importFileCB(ZMapFeatureSequenceMap sequence_map, const bool recent_only, gpointer cb_data)
{
  bool status = TRUE ;
  MainFrame main_frame = (MainFrame)cb_data ;
  std::string err_msg("") ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  ZMapView view = NULL ;
  ZMap zmap = main_frame->zmap ;

  view = zMapViewGetView(zmap->focus_viewwindow);

  req_sequence_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;

  /* Validate the user-specified arguments. These calls set status to false if there is a problem
   * and also set the err_msg. The start/end/strand functions also set the int from the string. 
   */
  validateReqSequence(status, err_msg, 
                      req_sequence_txt, 
                      req_start_txt, req_end_txt, 
                      req_start, req_end, start) ;

  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (status)
    {      
      /* If we have previously disabled popup warnings about failed sources, re-enable them now */
      zMapViewSetDisablePopups(view, false) ;

      for (auto &iter : *sequence_map->sources)
        {
          importSource(iter.second, view, sequence_map, req_sequence_txt, req_start, req_end, recent_only) ;
        }
    }
  else
    {
      zMapWarning("Error: \"%s\"", err_msg.c_str()) ;
    }

  return ;
}

  


// SCAFFOLD - THREAD TESTING
#include <ZMap/zmapDataSource.hpp>

using namespace ZMapDataSource ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
// SCAFFOLD - THREAD TESTING
static void importNewDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data) ;
static void newImportFile(MainFrame main_frame) ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void makeNewPanel(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end) ;
static void importNewFileCB(ZMapFeatureSequenceMap sequence_map, const bool recent_only, gpointer cb_data) ;
static void importNewSource(ZMapConfigSource server,
                            ZMapView view,
                            ZMapFeatureSequenceMap sequence_map,
                            const char *req_sequence,
                            const int req_start,
                            const int req_end,
                            const bool recent_only) ;
static void newCallBackFunc(DataSourceFeatures *features_source, void *user_data) ;

static void newSequenceCallBackFunc(DataSourceSequence *sequence_source, void *user_data) ;


// SCAFFOLD - THREAD TESTING
// Test dialog for new server stuff....
//
//

void zmapControlNewImportFile(ZMap zmap, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *toplevel ;
  MainFrame main_frame ;
  static bool first_time = true ;

  /* First time around, clear the 'recent' flag from all sources so that we don't show sources
   * that were loaded on startup. This won't be ideal if the user opens the import dialog before
   * all sources have finished loading. In that case though they can clear the list manually. */
  if (first_time)
    {
      clearRecentSources(sequence_map) ;
      first_time = false ;
    }

  main_frame = g_new0(MainFrameStruct, 1) ;

  main_frame->zmap = zmap ;

  zmap->import_file_dialog = main_frame->toplevel = toplevel
    = zMapGUIDialogNew(NULL, "Import data from a source", NULL, NULL) ;

  int width = 0 ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(toplevel, &width, &height) && GTK_IS_WINDOW(toplevel))
    gtk_window_set_default_size(GTK_WINDOW(toplevel), std::min(640.0, width * 0.5), std::min(400.0, height * 0.7)) ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  makeNewPanel(main_frame, sequence_map, req_start, req_end) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), main_frame) ;

  gtk_widget_show_all(toplevel) ;

  return ;
}




/* Make the whole panel returning the top container of the panel. */
static void makeNewPanel(MainFrame main_frame, ZMapFeatureSequenceMap sequence_map, int req_start, int req_end)
{
  GtkWidget *vbox, *sequence_box, *options_box;
  const char *sequence = "";
  GtkDialog *dialog = GTK_DIALOG(main_frame->toplevel) ;

  vbox = dialog->vbox ;

  importGetConfig(main_frame, sequence_map->config_file );

  main_frame->sequence_map = sequence_map;

  if(sequence_map)
    sequence = sequence_map->sequence;/* request defaults to original */

  options_box = makeOptionsBox(main_frame, sequence, req_start, req_end) ;
  gtk_box_pack_start(GTK_BOX(vbox), options_box, FALSE, FALSE, 0) ;

  sequence_box = zMapCreateSequenceViewWidg(importNewFileCB, main_frame,
                                            fileImportCloseCB, main_frame,
                                            sequence_map, TRUE, TRUE, main_frame->toplevel) ;
  gtk_box_pack_start(GTK_BOX(vbox), sequence_box, TRUE, TRUE, 0) ;

  return ;
}





static void importNewFileCB(ZMapFeatureSequenceMap sequence_map, const bool recent_only, gpointer cb_data)
{
  bool status = TRUE ;
  MainFrame main_frame = (MainFrame)cb_data ;
  std::string err_msg("") ;
  const char *req_start_txt= NULL ;
  const char *req_end_txt = NULL ;
  const char *req_sequence_txt = NULL ;
  int start = 0 ;
  int req_start = 0 ;
  int req_end = 0 ;
  ZMapView view = NULL ;
  ZMap zmap = main_frame->zmap ;

  view = zMapViewGetView(zmap->focus_viewwindow);

  req_sequence_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_sequence_widg)) ;
  req_start_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_start_widg)) ;
  req_end_txt = gtk_entry_get_text(GTK_ENTRY(main_frame->req_end_widg)) ;

  /* Validate the user-specified arguments. These calls set status to false if there is a problem
   * and also set the err_msg. The start/end/strand functions also set the int from the string. 
   */
  validateReqSequence(status, err_msg, 
                      req_sequence_txt, 
                      req_start_txt, req_end_txt, 
                      req_start, req_end, start) ;

  /*
   * If we got this far, then attempt to do something.
   *
   */
  if (status)
    {      
      /* If we have previously disabled popup warnings about failed sources, re-enable them now */
      zMapViewSetDisablePopups(view, false) ;

      for (auto &iter : *sequence_map->sources)
        {
          importNewSource(iter.second, view, sequence_map, req_sequence_txt, req_start, req_end, recent_only) ;
        }
    }
  else
    {
      zMapWarning("Error: \"%s\"", err_msg.c_str()) ;
    }

  return ;
}



/* Recursively import a source and any child sources */
static void importNewSource(ZMapConfigSource server,
                            ZMapView view,
                            ZMapFeatureSequenceMap sequence_map,
                            const char *req_sequence,
                            const int req_start,
                            const int req_end,
                            const bool recent_only)
{

  if (!recent_only || server->recent)
    {
      createSourceData(view, sequence_map, server) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      GError *g_error = NULL ;
      zMapViewSetUpServerConnection(view, server, req_sequence, req_start, req_end, false, &g_error) ;

      if (g_error)
        {
          zMapWarning("Failed to set up server connection for '%s': %s", 
                      g_quark_to_string(server->name_),
                      (g_error ? g_error->message : "<no error>")) ;
        }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      // DON'T THINK WE NEED THIS STEP FOR THIS CODE
      // Recurse through children
      for (auto child : server->children)
        {
          importSource(child, view, sequence_map, req_sequence, req_start, req_end, recent_only) ;
        }



      // Create a server connection for the source and start getting the data.
      if (server)
        {
          GList *req_featuresets = NULL ;
          ZMapFeatureContextMap context_map ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          const string url(server->url()) ;
          const string config_file = string("") ;
          const string version = string("") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          ZMapFeatureContext context ;


          // Try a feature request
          context_map = zMapViewGetContextMap(view) ;
          context = zMapViewCreateContext(view, req_featuresets, NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          DataSourceFeatures *my_feature_request = new DataSourceFeatures(sequence_map, req_start, req_end,
                                                                          url, config_file, version,
                                                                          context, &(context_map->styles)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
          DataSourceFeatures *my_feature_request = new DataSourceFeatures(sequence_map, req_start, req_end,
                                                                          server,
                                                                          context, &(context_map->styles)) ;


          if (!(my_feature_request->SendRequest(newCallBackFunc, view)))
            {
              // Really need to be able to get some error state here....should throw from the obj ?

              delete my_feature_request ;
            }



          // Try a sequence request
          GList *dna_req_featuresets = NULL ;
          GQuark dna_id = g_quark_from_string("dna") ;

          dna_req_featuresets = g_list_append(dna_req_featuresets, GINT_TO_POINTER(dna_id)) ;

          context_map = zMapViewGetContextMap(view) ;
          context = zMapViewCreateContext(view, dna_req_featuresets, NULL) ;

          const string acedb_url("acedb://any:any@gen1b:20000?use_methods=false&gff_version=2") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          DataSourceSequence *my_sequence_request = new DataSourceSequence(sequence_map, req_start, req_end,
                                                                           acedb_url, config_file, version,
                                                                           context, &(context_map->styles)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
          DataSourceSequence *my_sequence_request = new DataSourceSequence(sequence_map, req_start, req_end,
                                                                           server,
                                                                           context, &(context_map->styles)) ;


          if (!(my_sequence_request->SendRequest(newSequenceCallBackFunc, my_sequence_request)))
            {
              // Really need to be able to get some error state here....should throw from the obj ?

              delete my_sequence_request ;
            }



        }


    }



  return ;
}







#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

// old code, take the bits i need...

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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */









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


