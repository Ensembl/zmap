/*  File: zmapAppSource.cpp
 *  Author: Gemma Guest (gb10@sanger.ac.uk)
 *  Copyright (c) 2015: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>

#define DEFAULT_ENSEMBL_HOST "ensembldb.ensembl.org"
#define DEFAULT_ENSEMBL_PORT "3306"
#define DEFAULT_ENSEMBL_USER "anonymous"

/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;

  GtkWidget *name_widg ;
  GtkWidget *host_widg ;
  GtkWidget *port_widg ;
  GtkWidget *user_widg ;
  GtkWidget *pass_widg ;
  GtkWidget *dbname_widg ;
  GtkWidget *dbprefix_widg ;
  GtkWidget *featuresets_widg ;
  GtkWidget *biotypes_widg ;

  GtkWidget *chooser_widg ;

  ZMapFeatureSequenceMap sequence_map ;
  
  ZMapAppCreateSourceCB user_func ;
  gpointer user_data ;

} MainFrameStruct, *MainFrame ;



static GtkWidget *makePanel(GtkWidget *toplevel,
                            gpointer *seqdata_out,
                            ZMapFeatureSequenceMap sequence_map,
                            ZMapAppCreateSourceCB user_func,
                            gpointer user_data) ;
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeButtonBox(MainFrame main_data) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void cancelCB(GtkWidget *widget, gpointer cb_data) ;
static void applyCB(GtkWidget *widget, gpointer cb_data) ;



/*
 *                   External interface routines.
 */



/* Show a dialog to create a new source */
GtkWidget *zMapAppCreateSource(ZMapFeatureSequenceMap sequence_map,
                               ZMapAppCreateSourceCB user_func,
                               gpointer user_data)
{
  zMapReturnValIfFail(user_func, NULL) ;

  GtkWidget *toplevel = NULL ;
  GtkWidget *container ;
  gpointer seq_data = NULL ;

  toplevel = zMapGUIToplevelNew(NULL, "Please specify sequence to be viewed.") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  container = makePanel(toplevel, &seq_data, sequence_map, user_func, user_data) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  return toplevel ;
}



/*
 *                   Internal routines.
 */


/* Make the whole panel returning the top container of the panel. */
static GtkWidget *makePanel(GtkWidget *toplevel, 
                            gpointer *our_data,
                            ZMapFeatureSequenceMap sequence_map,
                            ZMapAppCreateSourceCB user_func,
                            gpointer user_data)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box ;
  MainFrame main_data ;

  main_data = g_new0(MainFrameStruct, 1) ;

  main_data->sequence_map = sequence_map ;
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

  button_box = makeButtonBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_box, TRUE, TRUE, 0) ;

  return frame ;
}


/* Make an entry widget and a label and add them to the given boxes. Returns the entry widget. */
static GtkWidget* makeEntryWidget(const char *label_str, 
                                  const char *default_value,
                                  const char *tooltip,
                                  GtkWidget *labelbox,
                                  GtkWidget *entrybox)
{
  GtkWidget *label = gtk_label_new(label_str) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  GtkWidget *entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  if (default_value)
    gtk_entry_set_text(GTK_ENTRY(entry), default_value) ;

  if (tooltip)
    {
      gtk_widget_set_tooltip_text(label, tooltip) ;
      gtk_widget_set_tooltip_text(entry, tooltip) ;
    }

  return entry ;
}


/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox ;

  frame = gtk_frame_new( "New File source: " );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  /* a vbox for the labels */
  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  /* a vbox for the entries */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  /* Create the label/entry widgets */
  main_data->name_widg = makeEntryWidget("Source name :", NULL, "A name for the new source", labelbox, entrybox) ;
  main_data->host_widg = makeEntryWidget("Host :", DEFAULT_ENSEMBL_HOST, NULL, labelbox, entrybox) ;
  main_data->port_widg = makeEntryWidget("Port :", DEFAULT_ENSEMBL_PORT, NULL, labelbox, entrybox) ;
  main_data->user_widg = makeEntryWidget("Username :", DEFAULT_ENSEMBL_USER, NULL, labelbox, entrybox) ;
  main_data->pass_widg = makeEntryWidget("Password :", NULL, "Can be empty if not required", labelbox, entrybox) ;
  main_data->dbname_widg = makeEntryWidget("DB name :", NULL, "The database to load features from", labelbox, entrybox) ;
  main_data->dbprefix_widg = makeEntryWidget("DB prefix :", NULL, "An optional prefix to add to source names for features from this database", labelbox, entrybox) ;
  main_data->featuresets_widg = makeEntryWidget("Featuresets :", NULL, "If specified, only load these featuresets", labelbox, entrybox) ;
  main_data->biotypes_widg = makeEntryWidget("Biotypes :", NULL, "If specified, only load these biotypes", labelbox, entrybox) ;

  return frame ;
}


/* Make the action buttons frame. */
static GtkWidget *makeButtonBox(MainFrame main_data)
{
  GtkWidget *frame ;
  GtkWidget *button_box, *cancel_button, *ok_button ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  button_box = gtk_hbutton_box_new() ;
  gtk_container_border_width(GTK_CONTAINER(button_box), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), button_box) ;

  cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL) ;
  gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked",
                     GTK_SIGNAL_FUNC(cancelCB), (gpointer)main_data) ;
  gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, TRUE, 0) ;

  ok_button = gtk_button_new_from_stock(GTK_STOCK_OK) ;
  gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
                     GTK_SIGNAL_FUNC(applyCB), (gpointer)main_data) ;
  gtk_box_pack_start(GTK_BOX(button_box), ok_button, FALSE, TRUE, 0) ;

  /* set create button as default. */
  GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(main_data->toplevel), ok_button) ;

  return frame ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  g_free(main_data) ;

  return ;
}


/* Kill the dialog. */
static void cancelCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  gtk_widget_destroy(main_data->toplevel) ;

  return ;
}


static void appendUrlValue(std::string &url, const char *prefix, const char *value)
{
  if (prefix)
    url += prefix ;

  if (value)
    url += value ;
}


static std::string constructUrl(const char *host, const char *port,
                                const char *user, const char *pass,
                                const char *dbname, const char *dbprefix)
{
  std::string url = "ensembl://" ;

  appendUrlValue(url, NULL, user) ;
  appendUrlValue(url, ":", pass) ;
  appendUrlValue(url, "@", host) ;
  appendUrlValue(url, ":", port) ;

  const char *separator= "?";
      
  if (dbname)
    {
      url += separator ;
      url += "db_name=" ;
      url += dbname ;
      separator = "&" ;
    }

  if (dbprefix)
    {
      url += separator ;
      url += "db_prefix=" ;
      url += dbprefix ;
      separator = "&" ;
    }

  return url ;
}


/* Utility to get entry text but to return null is place of empty strings */
static const char* getEntryText(GtkEntry *entry)
{
  const char *result = NULL ;

  if (entry)
    result = gtk_entry_get_text(entry) ;

  if (result && *result == 0)
    result = NULL ;

  return result ;
}


/* Create the new source. */
static void applyCB(GtkWidget *widget, gpointer cb_data)
{
  gboolean ok = FALSE ;
  MainFrame main_frame = (MainFrame)cb_data ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *host = getEntryText(GTK_ENTRY(main_frame->host_widg)) ;
  const char *port = getEntryText(GTK_ENTRY(main_frame->port_widg)) ;
  const char *user = getEntryText(GTK_ENTRY(main_frame->user_widg)) ;
  const char *pass = getEntryText(GTK_ENTRY(main_frame->pass_widg)) ;
  const char *dbname = getEntryText(GTK_ENTRY(main_frame->dbname_widg)) ;
  const char *dbprefix = getEntryText(GTK_ENTRY(main_frame->dbprefix_widg)) ;
  const char *featuresets = getEntryText(GTK_ENTRY(main_frame->featuresets_widg)) ;
  const char *biotypes = getEntryText(GTK_ENTRY(main_frame->featuresets_widg)) ;

  if (!source_name)
    {
      zMapWarning("%s", "Please enter a source name") ;
    }
  else if (!host)
    {
      zMapWarning("%s", "Please enter a host") ;
    }
  else if (!dbname)
    {
      zMapWarning("%s", "Please enter a database name") ;
    }
  else
    {
      GError *tmp_error = NULL ;
      
      std::string url = constructUrl(host, port, user, pass, dbname, dbprefix) ;

      (main_frame->user_func)(source_name, url, featuresets, biotypes, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapWarning("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  if (ok)
    gtk_widget_destroy(main_frame->toplevel); 

  return ;
}




