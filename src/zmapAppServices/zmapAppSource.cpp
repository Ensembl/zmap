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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <list>
#include <sstream>
#include <string.h>

#include <gbtools/gbtools.hpp>

#include <zmapAppServices_P.hpp>
#include <ZMap/zmap.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <ZMap/zmapUrl.hpp>

#ifdef USE_ENSEMBL
#include <ZMap/zmapEnsemblUtils.hpp>

#endif

using namespace std ;


#define xpad 4
#define ypad 4
#define SOURCE_TYPE_FILE "File/URL"

#ifdef USE_ENSEMBL
#define DEFAULT_ENSEMBL_HOST "ensembldb.ensembl.org"
#define DEFAULT_ENSEMBL_PORT1 "3306"
#define DEFAULT_ENSEMBL_USER "anonymous"
#define DEFAULT_ENSEMBL_PASS NULL
#define SOURCE_TYPE_ENSEMBL "Ensembl"
#define DEFAULT_COLUMNS_LIST "dna ; 3 frame ; 3 frame translation ; show translation ; "
#define DEFAULT_COLUMNS_LIST_ALL "all ; dna ; 3 frame ; 3 frame translation ; show translation ; "
#endif

#define SOURCE_TYPE_TRACKHUB "Track hub"

#define LOGIN_LABEL "Log in"
#define LOGOUT_LABEL "Log out"


/* Generic definitions of columns for displaying a list of e.g. databases. For now just include a
 * single column to display the name */
enum class GenericListColumn {NAME,  /*MUST BE LAST:*/N_COLS} ;

/* Columns for displaying a list of trackhub tracks */
enum class TrackListColumn {NAME, ID, SPECIES, ASSEMBLY, TRACKS,  /*MUST BE LAST:*/N_COLS} ;



/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;

  GtkComboBox *combo ;
  map<ZMapURLScheme, unsigned int> combo_indices ; /* keep track of which scheme is at which idx */

  ZMapFeatureSequenceMap sequence_map ;
  
  ZMapAppCreateSourceCB user_func ;
  gpointer user_data ;

  // General
  GtkWidget *name_widg ;

  // File
  list<GtkWidget*> file_widgets ;
  GtkWidget *path_widg ;

#ifdef USE_ENSEMBL
  // Ensembl
  list<GtkWidget*> ensembl_widgets ;
  GtkWidget *host_widg ;
  GtkWidget *port_widg ;
  GtkWidget *user_widg ;
  GtkWidget *pass_widg ;
  GtkWidget *dbname_widg ;
  GtkWidget *dbprefix_widg ;
  GtkWidget *featuresets_widg ;
  GtkWidget *biotypes_widg ;
  GtkWidget *dna_check ;
#endif

  // Trackhub
  gbtools::trackhub::Registry registry;
  list<gbtools::trackhub::TrackDb> user_trackdbs;
  list<GtkWidget*> trackhub_widgets;
  GtkWidget *trackdb_widg;
  GtkWidget *search_widg;
  GtkWidget *browse_widg;
  GtkWidget *register_widg;

} MainFrameStruct, *MainFrame ;


typedef struct SearcListDataStructName
{
  GtkEntry *search_entry ;
  GtkEntry *filter_entry ;
  GtkTreeModel *tree_model ;
} SearchListDataStruct, *SearchListData ;


static GtkWidget *makePanel(GtkWidget *toplevel,
                            gpointer *seqdata_out,
                            ZMapFeatureSequenceMap sequence_map,
                            ZMapAppCreateSourceCB user_func,
                            gpointer user_data,
                            MainFrame *main_data) ;
static GtkWidget *makeButtonBox(MainFrame main_data) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void cancelCB(GtkWidget *widget, gpointer cb_data) ;
static void applyCB(GtkWidget *widget, gpointer cb_data) ;
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;
static gboolean applyFile(MainFrame main_data) ;
static void setWidgetsVisibility(list<GtkWidget*> widget_list, const gboolean visible) ;
static void updatePanelFromSource(MainFrame main_data, ZMapConfigSource source) ;
static gboolean applyTrackhub(MainFrame main_frame);

static void trackhubSearchCB(GtkWidget *widget, gpointer cb_data);
static void trackhubBrowseCB(GtkWidget *widget, gpointer cb_data);
static void trackhubRegisterCB(GtkWidget *widget, gpointer cb_data);

#ifdef USE_ENSEMBL
static void dbnameCB(GtkWidget *widget, gpointer cb_data) ;
static void featuresetsCB(GtkWidget *widget, gpointer cb_data) ;
static void biotypesCB(GtkWidget *widget, gpointer cb_data) ;
static gboolean applyEnsembl(MainFrame main_data) ;
#endif /* USE_ENSEMBL */




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

  toplevel = zMapGUIToplevelNew(NULL, "Create Source") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  MainFrame main_data = NULL ;
  container = makePanel(toplevel, &seq_data, sequence_map, user_func, user_data, &main_data) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  /* Only show widgets for the relevant file type (ensembl by default if it exists, 
   * trackhub otherwise) */
  if (main_data)
    {
#ifdef USE_ENSEMBL
      setWidgetsVisibility(main_data->ensembl_widgets, TRUE) ;
      setWidgetsVisibility(main_data->file_widgets, FALSE) ;
      setWidgetsVisibility(main_data->trackhub_widgets, FALSE) ;
#else
      setWidgetsVisibility(main_data->trackhub_widgets, TRUE) ;
      setWidgetsVisibility(main_data->file_widgets, FALSE) ;
#endif
    }

  return toplevel ;
}


/* Show a dialog to edit an existing source */
GtkWidget *zMapAppEditSource(ZMapFeatureSequenceMap sequence_map,
                             ZMapConfigSource source,
                             ZMapAppCreateSourceCB user_func,
                             gpointer user_data)
{
  zMapReturnValIfFail(user_func, NULL) ;

  GtkWidget *toplevel = NULL ;
  GtkWidget *container ;
  gpointer seq_data = NULL ;

  toplevel = zMapGUIToplevelNew(NULL, "Edit Source") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  MainFrame main_data = NULL ;
  container = makePanel(toplevel, &seq_data, sequence_map, user_func, user_data, &main_data) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  /* Update the panel with the info from the given source. This toggles visibility of the
   * appropriate widgets so must be done after show_all */
  updatePanelFromSource(main_data, source) ;

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
                            gpointer user_data,
                            MainFrame *main_data_out)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box ;
  MainFrame main_data ;

  main_data = new MainFrameStruct ;

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

  if (main_data_out)
    *main_data_out = main_data ;

  return frame ;
}


/* Get the row index in the combo box for the given scheme */
static int comboGetIndex(MainFrame main_data, ZMapURLScheme scheme_in)
{
  int result = -1 ;

  /* We lump file/http/https/ftp all under a single 'file' option */
  ZMapURLScheme scheme = scheme_in ;

  if (scheme_in == SCHEME_HTTP || 
#ifdef HAVE_SSL
      scheme_in == SCHEME_HTTPS || 
#endif
      scheme_in == SCHEME_FTP)
    {
      scheme = SCHEME_FILE ;
    }

  auto map_iter = main_data->combo_indices.find(scheme) ;

  if (map_iter != main_data->combo_indices.end())
    result = map_iter->second ;

  return result ;
}


static void updatePanelFromFileSource(MainFrame main_data, 
                                      ZMapConfigSource source,
                                      ZMapURL zmap_url)
{
  char *source_name = main_data->sequence_map->getSourceName(source) ;

  gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), source_name) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->path_widg), zmap_url->path) ;

  if (source_name)
    g_free(source_name) ;
}

static void updatePanelFromTrackhubSource(MainFrame main_data, 
                                          ZMapConfigSource source,
                                          ZMapURL zmap_url)
{
  char *source_name = main_data->sequence_map->getSourceName(source) ;
  const char *trackdb_id = "" ;

  if (zmap_url)
    trackdb_id = zmap_url->file ;

  gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), source_name) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->trackdb_widg), trackdb_id) ;

  g_free(source_name) ;
}


#ifdef USE_ENSEMBL
static void updatePanelFromEnsemblSource(MainFrame main_data, 
                                               ZMapConfigSource source,
                                               ZMapURL zmap_url)
{
  char *source_name = main_data->sequence_map->getSourceName(source) ;
  const char *host = zmap_url->host ;
  char *port = g_strdup_printf("%d", zmap_url->port) ;
  const char *user = zmap_url->user ;
  const char *pass = zmap_url->passwd ;
  char *dbname = zMapURLGetQueryValue(zmap_url->query, "db_name") ;
  char *dbprefix = zMapURLGetQueryValue(zmap_url->query, "db_prefix") ;
  char *featuresets = g_strdup(source->featuresets) ;
  char *featuresets_ptr = featuresets ;
  const char *biotypes = source->biotypes ;
  gboolean load_dna = FALSE ;

  /* load_dna is true if featuresets starts with "all". Extract "all" (which would have been
   * added in because of the flag) from the featuresets list and set the flag instead.
   * Also extract the default columns list if it's there because we probably added this
   * in as well. */
  const unsigned int len_all = strlen(DEFAULT_COLUMNS_LIST_ALL) ;
  const unsigned int len_default = strlen(DEFAULT_COLUMNS_LIST) ;

  if (featuresets && 
      strlen(featuresets) >= len_all && 
      strncasecmp(featuresets, DEFAULT_COLUMNS_LIST_ALL, len_all) == 0)
    {
      load_dna = TRUE ;
      featuresets = featuresets + len_all ; // skip to the next char after the default cols
    }
  else if (featuresets && 
           strlen(featuresets) >= len_default && 
           strncasecmp(featuresets, DEFAULT_COLUMNS_LIST, len_default) == 0)
    {
      load_dna = TRUE ;
      featuresets = featuresets + len_default ; // skip to the next char after the default cols
    }

  if (source_name && main_data->name_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), source_name) ;
  if (host && main_data->host_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->host_widg), host) ;
  if (port && main_data->port_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->port_widg), port) ;
  if (user && main_data->user_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->user_widg), user) ;
  if (pass && main_data->pass_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->pass_widg), pass) ;
  if (dbname && main_data->dbname_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->dbname_widg), dbname) ;
  if (dbprefix && main_data->dbprefix_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->dbprefix_widg), dbprefix) ;
  if (featuresets && main_data->featuresets_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->featuresets_widg), featuresets) ;
  if (biotypes && main_data->biotypes_widg)
    gtk_entry_set_text(GTK_ENTRY(main_data->biotypes_widg), biotypes) ;
  if (main_data->dna_check)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(main_data->dna_check), load_dna) ;

  if (source_name)
    g_free(source_name) ;
  if (port)
    g_free(port) ;
  if (dbname)
    g_free(dbname) ;
  if (dbprefix)
    g_free(dbprefix) ;
  if (featuresets_ptr)
    g_free(featuresets_ptr) ;
}
#endif


/* After creating the panel, you can update it from an existing source (for editing an existing
 * source) using this function */
static void updatePanelFromSource(MainFrame main_data, ZMapConfigSource source)
{
  zMapReturnIfFail(main_data) ;

  /* When editing an existing source we currently identify it by name, so we must disable the 
   * name widget so that the user cannot edit it. Also disable changing the type of source */
  if (main_data->name_widg)
    gtk_widget_set_sensitive(main_data->name_widg, FALSE) ;

  if (main_data->combo)
    gtk_widget_set_sensitive(GTK_WIDGET(main_data->combo), FALSE) ;

  /* Parse values out of the url */
  int status = 0 ;
  ZMapURL zmap_url = url_parse(source->url, &status) ;

  /* Update the combo box with this scheme */
  if (main_data->combo)
    {
      /* set to -1 (none) first to make sure we emit a "changed" signal */
      gtk_combo_box_set_active(main_data->combo, -1) ;
      gtk_combo_box_set_active(main_data->combo, comboGetIndex(main_data, zmap_url->scheme)) ;
    }

  if (zmap_url)
    {
      if (zmap_url->scheme == SCHEME_FILE || 
          zmap_url->scheme == SCHEME_HTTP || 
#ifdef HAVE_SSL
          zmap_url->scheme == SCHEME_HTTPS || 
#endif
          zmap_url->scheme == SCHEME_FTP || 
          zmap_url->scheme == SCHEME_PIPE)
        {
          updatePanelFromFileSource(main_data, source, zmap_url);
        }
      else if (zmap_url->scheme == SCHEME_TRACKHUB)
        {
          updatePanelFromTrackhubSource(main_data, source, zmap_url);
        }
#ifdef USE_ENSEMBL
      else if (zmap_url->scheme == SCHEME_ENSEMBL)
        {
          updatePanelFromEnsemblSource(main_data, source, zmap_url);
        }
#endif
      else
        {
          zMapWarnIfReached();
        }
    }
}


static GtkWidget* makeEntryWidget(const char *label_str, 
                                  const char *default_value,
                                  const char *tooltip,
                                  GtkTable *table,
                                  int *row,
                                  const int col,
                                  const int max_col, 
                                  gboolean mandatory,
                                  list<GtkWidget*> *widget_list,
                                  const bool activates_default = false)
{
  GtkWidget *label = gtk_label_new(label_str) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, col, col + 1, *row, *row + 1, GTK_FILL, GTK_SHRINK, xpad, ypad) ;

  if (mandatory)
    {
      GdkColor color ;
      gdk_color_parse("red", &color) ;
      gtk_widget_modify_fg(label, GTK_STATE_NORMAL, &color) ;
    }

  GtkWidget *entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;

  gtk_table_attach(table, entry, col + 1, max_col, *row, *row + 1, 
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 
                   GTK_SHRINK, 
                   xpad, ypad) ;

  *row += 1;

  if (default_value)
    gtk_entry_set_text(GTK_ENTRY(entry), default_value) ;

  if (activates_default)
    gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;

  if (tooltip)
    {
      gtk_widget_set_tooltip_text(label, tooltip) ;
      gtk_widget_set_tooltip_text(entry, tooltip) ;
    }

  if (widget_list)
    {
      widget_list->push_back(label) ;
      widget_list->push_back(entry) ;
    }

  return entry ;
}


/* Set the visibility of all the widgets in the given list */
static void setWidgetsVisibility(list<GtkWidget*> widget_list, const gboolean visible)
{
  for (list<GtkWidget*>::const_iterator iter = widget_list.begin(); iter != widget_list.end(); ++iter)
    {
      if (visible)
        gtk_widget_show_all(*iter) ;
      else
        gtk_widget_hide_all(*iter) ;
    }
}


/* Returns the selected string value in a combo box as a newly-allocated free. Returns null if
 * not found. */
static char* comboGetValue(GtkComboBox *combo)
{
  char *result = NULL ;

  GtkTreeIter iter;
  
  if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
      
      gtk_tree_model_get(model, &iter, GenericListColumn::NAME, &result, -1);
    }

  return result ;
}


static void sourceTypeChangedCB(GtkComboBox *combo, gpointer data)
{
  MainFrame main_data = (MainFrame)data ;
  zMapReturnIfFail(main_data) ;

  char *value = comboGetValue(combo) ;

  if (value)
    {
      if (strcmp(value, SOURCE_TYPE_FILE) == 0)
        {
          setWidgetsVisibility(main_data->file_widgets, TRUE) ;
          setWidgetsVisibility(main_data->trackhub_widgets, FALSE) ;
#ifdef USE_ENSEMBL
          setWidgetsVisibility(main_data->ensembl_widgets, FALSE) ;
#endif /* USE_ENSEMBL */
        }
      else if (strcmp(value, SOURCE_TYPE_TRACKHUB) == 0)
        {
          setWidgetsVisibility(main_data->trackhub_widgets, TRUE) ;
          setWidgetsVisibility(main_data->file_widgets, FALSE) ;
#ifdef USE_ENSEMBL
          setWidgetsVisibility(main_data->ensembl_widgets, FALSE) ;
#endif
        }
#ifdef USE_ENSEMBL
      else if (strcmp(value, SOURCE_TYPE_ENSEMBL) == 0)
        {
          setWidgetsVisibility(main_data->ensembl_widgets, TRUE) ;
          setWidgetsVisibility(main_data->trackhub_widgets, FALSE) ;
          setWidgetsVisibility(main_data->file_widgets, FALSE) ;
        }
#endif /* USE_ENSEMBL */

      g_free(value) ;
    }
}


static GtkComboBox *createComboBox(MainFrame main_data)
{
  GtkComboBox *combo = NULL ;

  /* Create a combo box to choose the type of source to create */
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
  combo = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(store)));
  g_object_unref(store) ;

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, NULL);

  /* Add the rows to the combo box */
  unsigned int combo_index = 0 ;

  GtkTreeIter iter;

  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, GenericListColumn::NAME, SOURCE_TYPE_FILE, -1);
  gtk_combo_box_set_active_iter(combo, &iter);

  main_data->combo_indices[SCHEME_FILE] = combo_index ;
  ++combo_index ;

  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, SOURCE_TYPE_TRACKHUB, -1);
  gtk_combo_box_set_active_iter(combo, &iter);

  main_data->combo_indices[SCHEME_TRACKHUB] = combo_index ;
  ++combo_index ;

#ifdef USE_ENSEMBL
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, GenericListColumn::NAME, SOURCE_TYPE_ENSEMBL, -1);
  gtk_combo_box_set_active_iter(combo, &iter);

  main_data->combo_indices[SCHEME_ENSEMBL] = combo_index ;
  ++combo_index ;
#endif /* USE_ENSEMBL */

  g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(sourceTypeChangedCB), main_data);

  return combo ;
}


static GtkWidget* makeButtonWidget(const char *stock,
                                   const char *label,
                                   const bool icon_only,
                                   const char *tooltip,
                                   GtkSignalFunc cb_func,
                                   gpointer cb_data,
                                   GtkTable *table,
                                   const int row,
                                   const int col,
                                   list<GtkWidget*> &widgets_list)
{
  GtkWidget *button = NULL ;

  if (stock && icon_only)
    {
      button = gtk_button_new() ;
      gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON));
    }
  else if (stock)
    {
      button = gtk_button_new_from_stock(stock) ;
    }
  else
    {
      button = gtk_button_new_with_label(label) ;
    }

  gtk_widget_set_tooltip_text(button, tooltip) ;

  gtk_signal_connect(GTK_OBJECT(button), "clicked", cb_func, cb_data) ;

  gtk_table_attach(table, button, col, col + 1, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;

  widgets_list.push_back(button);

  return button;
}


#ifdef USE_ENSEMBL
static void makeEnsemblWidgets(MainFrame main_data, 
                               ZMapFeatureSequenceMap sequence_map,
                               GtkTable *table,
                               const int rows,
                               const int cols,
                               int &row,
                               int &col)
{
  main_data->dbname_widg = makeEntryWidget("Database :", NULL, "REQUIRED: The database to load features from", 
                                           table, &row, col, col + 2, TRUE, &main_data->ensembl_widgets) ;

  /* Add a button next to the dbname widget to allow the user to search for a db in this host */
  makeButtonWidget(GTK_STOCK_FIND, NULL,
                   true,
                   "Look up database for this host", 
                   GTK_SIGNAL_FUNC(dbnameCB), main_data,
                   table, row - 1, col + 2,
                   main_data->ensembl_widgets) ;

  main_data->dbprefix_widg = makeEntryWidget("DB prefix :", NULL, "OPTIONAL: If specified, this prefix will be added to source names for features from this database", 
                                             table, &row, col, col + 2, FALSE, &main_data->ensembl_widgets) ;

  /* Second column */
  int max_row = row ;
  row = 0 ;
  col += 3 ;

  main_data->host_widg = makeEntryWidget("Host :", DEFAULT_ENSEMBL_HOST, NULL, table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;
  main_data->port_widg = makeEntryWidget("Port :", DEFAULT_ENSEMBL_PORT1, NULL, table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;
  main_data->user_widg = makeEntryWidget("Username :", DEFAULT_ENSEMBL_USER, NULL, table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;
  main_data->pass_widg = makeEntryWidget("Password :", DEFAULT_ENSEMBL_PASS, "Can be empty if not required", table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;

  /* Rows at bottom spanning full width */
  col = 0 ;
  if (max_row > row)
    row = max_row ;

  main_data->featuresets_widg = makeEntryWidget("Featuresets :", NULL, "OPTIONAL: Semi-colon-separated list of featuresets to filter input by", 
                                                table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;

  /* Add a button next to the featuresets widget to allow the user to search for featuresets in this database */
  GtkWidget *featuresets_button = gtk_button_new() ;
  gtk_button_set_image(GTK_BUTTON(featuresets_button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_tooltip_text(featuresets_button, "Look up featuresets for this database") ;
  gtk_signal_connect(GTK_OBJECT(featuresets_button), "clicked", GTK_SIGNAL_FUNC(featuresetsCB), main_data) ;
  gtk_table_attach(table, featuresets_button, cols - 1, cols, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  main_data->ensembl_widgets.push_back(featuresets_button) ;

  main_data->biotypes_widg = makeEntryWidget("Biotypes :", NULL, "OPTIONAL: Semi-colon-separated list of biotypes to filter input by", 
                                             table, &row, col, cols - 1, FALSE, &main_data->ensembl_widgets) ;

  /* Add a button next to the biotypes widget to allow the user to search for biotypes in this database */
  GtkWidget *biotypes_button = gtk_button_new() ;
  gtk_button_set_image(GTK_BUTTON(biotypes_button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_tooltip_text(biotypes_button, "Look up biotypes for this database") ;
  gtk_signal_connect(GTK_OBJECT(biotypes_button), "clicked", GTK_SIGNAL_FUNC(biotypesCB), main_data) ;
  gtk_table_attach(table, biotypes_button, cols - 1, cols, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  main_data->ensembl_widgets.push_back(biotypes_button) ;
  ++row ;

  /* Add a check button to specify whether to load DNA */
  col = 0 ;
  main_data->dna_check = gtk_check_button_new_with_label("Load DNA") ;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(main_data->dna_check), FALSE) ;
  gtk_widget_set_tooltip_text(main_data->dna_check, "Tick this button to load DNA, if available (not advised for very large regions)") ;
  gtk_table_attach(table, main_data->dna_check, col, col + 1, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  main_data->ensembl_widgets.push_back(main_data->dna_check) ;
  ++row ;
}
#endif


static void makeTrackhubWidgets(MainFrame main_data, 
                                ZMapFeatureSequenceMap sequence_map,
                                GtkTable *table,
                                const int rows,
                                const int cols,
                                int &row,
                                int &col)
{
  /* Display the trackDb details */
  /* For now just display the trackdb id (we'll want to improve this to display more details
   * about the track) */
  main_data->trackdb_widg = makeEntryWidget("TrackDb ID :", NULL, "REQUIRED: The track database ID from the Ensembl Track Hub Registry",
                                            table, &row, col, col + 2, TRUE, &main_data->trackhub_widgets);
  
  /* Search button */
  main_data->search_widg = makeButtonWidget(NULL, "Search", false, "Search for track hubs",
                                            GTK_SIGNAL_FUNC(trackhubSearchCB), main_data,
                                            table, row, col,
                                            main_data->trackhub_widgets);
  ++col;

  /* Browse button */
  main_data->browse_widg = makeButtonWidget(NULL, "Browse", false, "Browse registered track hubs",
                                            GTK_SIGNAL_FUNC(trackhubBrowseCB), main_data,
                                            table, row, col,
                                            main_data->trackhub_widgets);
  ++col;

  /* Register button */
  main_data->register_widg = makeButtonWidget(NULL, "Register", false, "Register a new track hub",
                                              GTK_SIGNAL_FUNC(trackhubRegisterCB), main_data,
                                              table, row, col,
                                              main_data->trackhub_widgets);
  ++col;
}


/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_data, 
                                ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame = gtk_frame_new( "New source: " ) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  const int rows = 7 ;
  const int cols = 6 ;
  GtkTable *table = GTK_TABLE(gtk_table_new(rows, cols, FALSE)) ;
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(table)) ;

  int row = 0 ;
  int col = 0 ;

  main_data->combo = createComboBox(main_data) ;
  gtk_table_attach(table, gtk_label_new("Source type :"), col, col + 1, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  gtk_table_attach(table, GTK_WIDGET(main_data->combo), col + 1, col + 2, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  ++row ;

  /* Create the label/entry widgets */

  /* First column */
  main_data->name_widg = makeEntryWidget("Source name :", NULL, "REQUIRED: Please enter a name for the new source", 
                                         table, &row, col, col + 2, TRUE, NULL) ;
  int path_widg_row = row;
  main_data->path_widg = makeEntryWidget("Path/URL :", NULL, "REQUIRED: The file/URL to load features from", 
                                         table, &row, col, col + 2, TRUE, &main_data->file_widgets) ;

#ifdef USE_ENSEMBL
  /* Place this widget on the same row as path_widg as they will be interchangeable */
  row = path_widg_row;

  makeEnsemblWidgets(main_data, sequence_map, table, rows, cols, row, col) ;
#endif /* USE_ENSEMBL */

  /* Place this widget on the same row as path_widg as they will be interchangeable */
  row = path_widg_row;
  makeTrackhubWidgets(main_data, sequence_map, table, rows, cols, row, col) ;

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

  delete main_data ;

  return ;
}


/* Kill the dialog. */
static void cancelCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  gtk_widget_destroy(main_data->toplevel) ;

  return ;
}


/* Create the new source. */
static void applyCB(GtkWidget *widget, gpointer cb_data)
{
  gboolean ok = FALSE ;
  MainFrame main_frame = (MainFrame)cb_data ;

  char *source_type = comboGetValue(main_frame->combo) ;

  if (source_type)
    {
      if (strcmp(source_type, SOURCE_TYPE_FILE) == 0)
        {
          ok = applyFile(main_frame) ;
        }
      else if (strcmp(source_type, SOURCE_TYPE_TRACKHUB) == 0)
        {
          ok = applyTrackhub(main_frame) ;
        }
#ifdef USE_ENSEMBL
      else if (strcmp(source_type, SOURCE_TYPE_ENSEMBL) == 0)
        {
          ok = applyEnsembl(main_frame) ;
        }
#endif /* USE_ENSEMBL */

      g_free(source_type) ;
    }

  if (ok)
    gtk_widget_destroy(main_frame->toplevel); 

  return ;
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


static gboolean applyFile(MainFrame main_frame)
{
  gboolean ok = FALSE ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *path = getEntryText(GTK_ENTRY(main_frame->path_widg)) ;

  if (!source_name)
    {
      zMapWarning("%s", "Please enter a source name") ;
    }
  else if (!path)
    {
      zMapWarning("%s", "Please enter a path/URL") ;
    }
  else
    {
      GError *tmp_error = NULL ;
      
      // Prepend file:///, unless its an http/ftp url
      string url("");

      if (strncasecmp(path, "http://", 7) != 0 &&
          strncasecmp(path, "https://", 8) != 0 &&
          strncasecmp(path, "ftp://", 6) != 0)
        {
          url += "file:///" ;
        }

      url += path ;

      (main_frame->user_func)(source_name, url, NULL, NULL, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapWarning("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  return ok ;
}


/* 
 * Ensembl functions
 */

#ifdef USE_ENSEMBL

static string constructUrl(const char *host, const char *port,
                           const char *user, const char *pass,
                           const char *dbname, const char *dbprefix)
{
  stringstream url ;

  url << "ensembl://" << user << ":" << pass << "@" << host << ":" << port ;

  const char *separator= "?";

  if (dbname)
    {
      url << separator << "db_name=" << dbname ;
      separator = "&" ;
    }

  if (dbprefix)
    {
      url << separator << "db_prefix=" << dbprefix ;
      separator = "&" ;
    }

  return url.str() ;
}


static gboolean applyEnsembl(MainFrame main_frame)
{
  gboolean ok = FALSE ;
  char *tmp = NULL ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *host = getEntryText(GTK_ENTRY(main_frame->host_widg)) ;
  const char *port = getEntryText(GTK_ENTRY(main_frame->port_widg)) ;
  const char *user = getEntryText(GTK_ENTRY(main_frame->user_widg)) ;
  const char *pass = getEntryText(GTK_ENTRY(main_frame->pass_widg)) ;
  const char *dbname = getEntryText(GTK_ENTRY(main_frame->dbname_widg)) ;
  const char *dbprefix = getEntryText(GTK_ENTRY(main_frame->dbprefix_widg)) ;
  const char *featuresets = getEntryText(GTK_ENTRY(main_frame->featuresets_widg)) ;
  const char *biotypes = getEntryText(GTK_ENTRY(main_frame->biotypes_widg)) ;
  const gboolean load_dna = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(main_frame->dna_check)) ;

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

      /* If loading dna (which for now implies 3ft etc too) then prefix the featuresets list with
         the dna, 3ft etc. column names. */
      if (load_dna)
        {
          if (featuresets)
            {
              tmp = g_strdup_printf("%s%s", DEFAULT_COLUMNS_LIST, featuresets) ;
              featuresets = tmp ;
            }
          else
            {
              /* Normally if we specify any featuresets then zmap will load only those and will
               * exclude all non-named featuresets. However, here the user hasn't really named
               * any so we still want to load all other featuresets. For now, prefix the list
               * with "all" to indicate this is the intent. Really we need a better way to do this... */
              featuresets = g_strdup(DEFAULT_COLUMNS_LIST_ALL) ;
            }
        }

      (main_frame->user_func)(source_name, url, featuresets, biotypes, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapWarning("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  if (tmp)
    g_free(tmp) ;

  return ok ;
}


/* Get the list of databases available from the host specified in the entry widgets */
static list<string>* mainFrameGetDatabaseList(MainFrame main_data, GError **error)
{
  list<string> *db_list = NULL ;
  
  zMapReturnValIfFail(main_data && 
                      main_data->host_widg && main_data->port_widg && 
                      main_data->user_widg && main_data->pass_widg,
                      db_list) ;

  const char *host = gtk_entry_get_text(GTK_ENTRY(main_data->host_widg)) ;
  const int port = atoi(gtk_entry_get_text(GTK_ENTRY(main_data->port_widg))) ;
  const char *user = gtk_entry_get_text(GTK_ENTRY(main_data->user_widg)) ;
  const char *pass = gtk_entry_get_text(GTK_ENTRY(main_data->pass_widg)) ;
  
  db_list = zMapEnsemblGetDatabaseList(host, port, user, pass, error) ;

  return db_list ;
}


/* Get the list of featuresets available from the database specified in the entry widgets */
static list<string>* mainFrameGetFeaturesetsList(MainFrame main_data, GError **error)
{
  list<string> *db_list = NULL ;
  
  zMapReturnValIfFail(main_data && 
                      main_data->host_widg && main_data->port_widg && 
                      main_data->user_widg && main_data->pass_widg && main_data->dbname_widg,
                      db_list) ;

  const char *host = gtk_entry_get_text(GTK_ENTRY(main_data->host_widg)) ;
  const int port = atoi(gtk_entry_get_text(GTK_ENTRY(main_data->port_widg))) ;
  const char *user = gtk_entry_get_text(GTK_ENTRY(main_data->user_widg)) ;
  const char *pass = gtk_entry_get_text(GTK_ENTRY(main_data->pass_widg)) ;
  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;
  
  db_list = zMapEnsemblGetFeaturesetsList(host, port, user, pass, dbname, error) ;

  return db_list ;
}


/* Get the list of biotypes available from the database specified in the entry widgets */
static list<string>* mainFrameGetBiotypesList(MainFrame main_data, GError **error)
{
  list<string> *db_list = NULL ;
  
  zMapReturnValIfFail(main_data && 
                      main_data->host_widg && main_data->port_widg && 
                      main_data->user_widg && main_data->pass_widg && main_data->dbname_widg,
                      db_list) ;

  const char *host = gtk_entry_get_text(GTK_ENTRY(main_data->host_widg)) ;
  const int port = atoi(gtk_entry_get_text(GTK_ENTRY(main_data->port_widg))) ;
  const char *user = gtk_entry_get_text(GTK_ENTRY(main_data->user_widg)) ;
  const char *pass = gtk_entry_get_text(GTK_ENTRY(main_data->pass_widg)) ;
  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;
  
  db_list = zMapEnsemblGetBiotypesList(host, port, user, pass, dbname, error) ;

  return db_list ;
}


/* Case insensitive version of strstr */
static char* my_strcasestr(const char *haystack, const char *needle)
{
  char *result = 0 ;

  if (haystack && needle && *haystack && *needle)
    {
      char *h = g_ascii_strdown(haystack, -1) ;
      char *n = g_ascii_strdown(needle, -1) ;
      
      result = strstr(h, n) ;

      g_free(h) ;
      g_free(n) ;
    }

  return result ;
}


/* Return true if the given search term matches the value in the given tree row. */
template<typename ColType>
gboolean treeRowContainsText(GtkTreeModel *model,
                             const gchar *search_term,
                             GtkTreeIter *iter,
                             const ColType col_id)
{
  gboolean result = FALSE ;

  char *column_name = NULL ;

  gtk_tree_model_get(model, iter,
                     col_id, &column_name,
                     -1) ;

  if (column_name && 
      search_term && 
      strlen(column_name) > 0 && 
      strlen(search_term) > 0 &&
      my_strcasestr(column_name, search_term) != NULL)
    {
      result = TRUE ;
    }

  g_free(column_name) ;

  return result ;
}

/* Callback used to determine if a search term matches a row in the tree. Returns false if it
 * matches, true otherwise. */
gboolean generic_search_equal_func_cb(GtkTreeModel *model,
                                      gint column,
                                      const gchar *key,
                                      GtkTreeIter *iter,
                                      gpointer user_data)
{
  return !treeRowContainsText(model, key, iter, GenericListColumn::NAME) ;
}

gboolean track_search_equal_func_cb(GtkTreeModel *model,
                                    gint column,
                                    const gchar *key,
                                    GtkTreeIter *iter,
                                    gpointer user_data)
{
  return 
    !treeRowContainsText(model, key, iter, TrackListColumn::NAME) &&
    !treeRowContainsText(model, key, iter, TrackListColumn::SPECIES) &&
    !treeRowContainsText(model, key, iter, TrackListColumn::ASSEMBLY) ;
}


/* Callback used to determine if a given row in the filtered tree model is visible */
static gboolean generic_list_filter_visible_cb(GtkTreeModel *model, 
                                               GtkTreeIter *iter, 
                                               gpointer user_data)
{
  gboolean result = FALSE ;

  GtkEntry *search_entry = GTK_ENTRY(user_data) ;
  zMapReturnValIfFail(search_entry, result) ;

  const char *search_term = gtk_entry_get_text(search_entry) ;
  char *column_name = NULL ;

  gtk_tree_model_get(model, iter,
                     GenericListColumn::NAME, &column_name,
                     -1) ;

  if (!search_term || *search_term == 0)
    {
      /* If text isn't specified, show everything */
      result = TRUE ;
    }
  else
    {
      result = treeRowContainsText(model, search_term, iter, GenericListColumn::NAME) ;
    }

  return result ;
}


/* Callback used to determine if a given row in the filtered tree model is visible */
static gboolean track_list_filter_visible_cb(GtkTreeModel *model, 
                                             GtkTreeIter *iter, 
                                             gpointer user_data)
{
  gboolean result = FALSE ;

  GtkEntry *search_entry = GTK_ENTRY(user_data) ;
  zMapReturnValIfFail(search_entry, result) ;

  const char *search_term = gtk_entry_get_text(search_entry) ;
  char *column_name = NULL ;

  gtk_tree_model_get(model, iter,
                     GenericListColumn::NAME, &column_name,
                     -1) ;

  if (!search_term || *search_term == 0)
    {
      /* If text isn't specified, show everything */
      result = TRUE ;
    }
  else
    {
      result = 
        treeRowContainsText(model, search_term, iter, TrackListColumn::NAME) ||
        treeRowContainsText(model, search_term, iter, TrackListColumn::SPECIES) ||
        treeRowContainsText(model, search_term, iter, TrackListColumn::ASSEMBLY) ;
    }

  return result ;
}


/* Utility to create a text column in a tree view */
template<typename ColType>
static void createTreeViewTextColumn(GtkTreeView *tree_view, 
                                     const char *title, 
                                     const ColType col_id,
                                     const bool hide = false)
{
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new() ;

  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(title, 
                                                                       renderer, 
                                                                       "text", 
                                                                       (int)col_id, 
                                                                       NULL);

  gtk_tree_view_column_set_resizable(column, TRUE) ;
  gtk_tree_view_column_set_clickable(column, TRUE) ;
  gtk_tree_view_column_set_reorderable(column, TRUE) ;

  if (hide)
    gtk_tree_view_column_set_visible(column, FALSE) ;

  gtk_tree_view_append_column(tree_view, column);

}


static void listStorePopulate(GtkListStore *store,
                              const list<string> *vals_list)
{
  /* Loop through all of the names */
  for (const auto &name : *vals_list)
    {
      /* Create a new row in the list store and set the column values */
      GtkTreeIter store_iter ;
      gtk_list_store_append(store, &store_iter);

      gtk_list_store_set(store, &store_iter, 
                         GenericListColumn::NAME, name.c_str(), 
                         -1);
    }
}

static void listStorePopulate(GtkListStore *store,
                            const list<gbtools::trackhub::TrackDb> *trackdb_list)
{
  /* Loop through all of the trackdbs */
  for (const auto &trackdb : *trackdb_list)
    {
      /* Create a new row in the list store and set the column values */
      GtkTreeIter store_iter ;
      gtk_list_store_append(store, &store_iter);

      /* We'll show the number of tracks as num-with-data/total */
      stringstream num_tracks;
      num_tracks << trackdb.num_with_data() << "/" << trackdb.num_tracks();

      gtk_list_store_set(store, &store_iter, 
                         TrackListColumn::NAME, trackdb.name().c_str(), 
                         TrackListColumn::ID, trackdb.id().c_str(), 
                         TrackListColumn::SPECIES, trackdb.species().c_str(), 
                         TrackListColumn::ASSEMBLY, trackdb.assembly().c_str(), 
                         TrackListColumn::TRACKS, num_tracks.str().c_str(), 
                         -1);
    }
}


template<typename ListType>
static void treeViewRefresh(GtkTreeView *tree_view, 
                            const ListType *val_list)
{
  GtkListStore *store = NULL ;
  GtkTreeModel *model = gtk_tree_view_get_model(tree_view) ;
  GtkTreeModel *real_model = model ;

  // Check if it's a filtered model and if so get the real (unfiltered) model
  if (GTK_IS_TREE_MODEL_FILTER(model))
    real_model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model)) ;

  if (real_model && GTK_IS_LIST_STORE(real_model))
    {
      store = GTK_LIST_STORE(real_model) ;

      gtk_list_store_clear(store) ;
      listStorePopulate(store, val_list) ;
    }
  else
    {
      zMapWarning("%s", "Program error: cannot refresh list: expected list store") ;
      zMapWarnIfReached() ;
    }
}


/* Create a tree view widget to show name values in the given list */
static GtkTreeView* createListWidget(MainFrame main_data, 
                                     const list<string> *val_list,
                                     SearchListData search_data, 
                                     const gboolean allow_multiple)
{
  /* Create the data store */
  GtkListStore *store = gtk_list_store_new((int)GenericListColumn::N_COLS, G_TYPE_STRING) ;

  /* Create a filtered version of the data store */
  GtkTreeModel *filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL) ;
  search_data->tree_model = filtered ;

  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
                                         generic_list_filter_visible_cb, 
                                         search_data->filter_entry,
                                         NULL);

  listStorePopulate(store, val_list) ;

  /* Create the tree widget */
  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(filtered))) ;

  /* Set various properties on the tree widget */
  gtk_tree_view_set_enable_search(tree_view, FALSE);
  gtk_tree_view_set_search_column(tree_view, (int)GenericListColumn::NAME);
  gtk_tree_view_set_search_entry(tree_view, search_data->search_entry) ;
  gtk_tree_view_set_search_equal_func(tree_view, generic_search_equal_func_cb, NULL, NULL) ;

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;
  gtk_tree_selection_set_mode(tree_selection, allow_multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

  /* Create the columns */
  createTreeViewTextColumn(tree_view, "Name", GenericListColumn::NAME);
  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", 
                                                                       renderer, 
                                                                       "text", 
                                                                       GenericListColumn::NAME, 
                                                                       NULL);

  gtk_tree_view_column_set_resizable(column, TRUE) ;
  gtk_tree_view_append_column(tree_view, column);

  return tree_view ;
}


/* Create a tree view widget to show trackdb values in the given list */
static GtkTreeView* createListWidget(MainFrame main_data, 
                                     const list<gbtools::trackhub::TrackDb> *trackdb_list, 
                                     SearchListData search_data, 
                                     const gboolean allow_multiple)
{
  /* Create the data store */
  int num_cols = (int)(TrackListColumn::N_COLS) ;

  GtkListStore *store = gtk_list_store_new(num_cols, 
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING,
                                           G_TYPE_STRING) ;

  /* Create a filtered version of the data store */
  GtkTreeModel *filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL) ;
  search_data->tree_model = filtered ;

  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
                                         track_list_filter_visible_cb, 
                                         search_data->filter_entry,
                                         NULL);

  listStorePopulate(store, trackdb_list) ;

  /* Create the tree widget */
  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(filtered))) ;

  /* Set various properties on the tree widget */
  gtk_tree_view_set_enable_search(tree_view, FALSE);
  gtk_tree_view_set_search_column(tree_view, (int)TrackListColumn::NAME);
  gtk_tree_view_set_search_entry(tree_view, search_data->search_entry) ;
  gtk_tree_view_set_search_equal_func(tree_view, track_search_equal_func_cb, NULL, NULL) ;

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;
  gtk_tree_selection_set_mode(tree_selection, allow_multiple ? GTK_SELECTION_MULTIPLE : GTK_SELECTION_SINGLE);

  /* Create the columns */
  createTreeViewTextColumn(tree_view, "Name", TrackListColumn::NAME) ;
  createTreeViewTextColumn(tree_view, "ID", TrackListColumn::ID, true) ;
  createTreeViewTextColumn(tree_view, "Species", TrackListColumn::SPECIES) ;
  createTreeViewTextColumn(tree_view, "Assembly", TrackListColumn::ASSEMBLY) ;
  createTreeViewTextColumn(tree_view, "Tracks", TrackListColumn::TRACKS) ;

  return tree_view ;
}


/* Update the given entry with the selected value(s) from the given tree. Uses the given column
 * in the tree for the result */
template<typename ColType>
static gboolean setEntryFromSelection(GtkTreeView *tree_view,
                                      GtkEntry *entry, 
                                      ColType col_id)
{
  gboolean ok = FALSE ;

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;

  if (tree_selection)
    {
      /* Loop through all rows in the selection and compile values into a semi-colon-separated
       * list */
      string result ;
      GtkTreeModel *model = NULL ;
      GList *rows = gtk_tree_selection_get_selected_rows(tree_selection, &model) ;
          
      for (GList *row = rows; row; row = g_list_next(row))
        {
          GtkTreePath *path = (GtkTreePath*)(row->data) ;
          GtkTreeIter iter ;
          gtk_tree_model_get_iter(model, &iter, path) ;

          char *value_str = NULL ;

          gtk_tree_model_get(model, &iter, col_id, &value_str, -1) ;

          if (value_str)
            {
              if (result.size() > 0)
                result += "; " ;
                
              result += value_str ;

              g_free(value_str) ;
              value_str = NULL ;
            }
        }

      gtk_entry_set_text(entry, result.c_str()) ;
      ok = TRUE ;
    }
  else
    {
      zMapCritical("%s", "Please select a database name") ;
      ok = FALSE ;
    }

  return ok ;
}


/* Callback called when the user hits the enter key in the filter text entry box */
static void filter_entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
  SearchListData search_data = (SearchListData)user_data ;
  zMapReturnIfFail(search_data->tree_model) ;

  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(search_data->tree_model) ;

  /* Refilter the filtered model */
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;
}


static void clear_button_cb(GtkButton *button, gpointer user_data)
{
  SearchListData search_data = (SearchListData)user_data ;
  zMapReturnIfFail(search_data->tree_model) ;

  if (search_data->search_entry)
    gtk_entry_set_text(search_data->search_entry, "") ;

  if (search_data->filter_entry)
    gtk_entry_set_text(search_data->filter_entry, "") ;

  /* Refilter the filtered model */
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(search_data->tree_model) ;
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;
}

/* Create and run a dialog to show the given list of values in a gtk tree view and to set the
 * given entry widget with the result when the user selects a row and hits ok. If the user
 * selects multiple values then it sets a semi-colon-separated list in the entry widget.
 * This function can take lists of different types of values and will call an overload of
 * createListWidget for the relevant type. 
 * top_panel is an optional extra panel to display at the top of the dialog */
template<typename ListType, typename ColType>
static gboolean runListDialog(MainFrame main_data, 
                              list<ListType> *values_list, 
                              ColType result_col,
                              GtkEntry *result_widg,
                              const char *title,
                              const char allow_multiple,
                              GtkWidget *top_panel = NULL)
{
  gboolean ok = FALSE ;
  zMapReturnValIfFail(main_data && values_list, ok) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons(title,
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                  GTK_STOCK_REFRESH, GTK_RESPONSE_APPLY,
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(dialog, NULL, &height))
    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, height * 0.5) ;

  GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

  /* Add caller-specified top panel, if any */
  if (top_panel)
    gtk_box_pack_start(content, top_panel, FALSE, FALSE, 0) ;

  /* Search/filter toolbar */
  GtkWidget *hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(content, hbox, FALSE, FALSE, 0) ;

  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Search:"), FALSE, FALSE, 0) ;
  GtkEntry *search_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(search_entry), FALSE, FALSE, 0) ;

  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Filter:"), FALSE, FALSE, 0) ;
  GtkEntry *filter_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(filter_entry), FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(GTK_WIDGET(search_entry), "Search for values containing this text. Press the up/down arrows to progress to next/previous match.") ;

  GtkWidget *button = gtk_button_new_with_mnemonic("C_lear") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(button, "Clear the search/filter boxes") ;
  gtk_widget_set_tooltip_text(GTK_WIDGET(filter_entry), "Show only values containing this text. Press enter to apply the filter.") ;

  SearchListDataStruct search_data = {search_entry, filter_entry, NULL} ;
  g_signal_connect(G_OBJECT(filter_entry), "activate", G_CALLBACK(filter_entry_activate_cb), &search_data) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clear_button_cb), &search_data) ;


  /* Scrolled list of all values */
  GtkWidget *scrollwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(content, GTK_WIDGET(scrollwin), TRUE, TRUE, 0) ;

  GtkTreeView *list_widget = createListWidget(main_data, values_list, &search_data, allow_multiple) ;
  gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(list_widget)) ;

  gtk_widget_show_all(dialog) ;

  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (response == GTK_RESPONSE_OK)
    {
      if (setEntryFromSelection(list_widget, result_widg, result_col))
        {
          gtk_widget_destroy(dialog) ;
          ok = TRUE ;
        }
    }
  else if (response == GTK_RESPONSE_APPLY)
    {
      treeViewRefresh(list_widget, values_list) ;
    }
  else
    {
      gtk_widget_destroy(dialog) ;
      ok = TRUE ;
    }

  return ok ;
}


/* Called when the user hits the button to search for a database name. If the user selects a name
 * successfully then this updates the value in the dbname_widg */
static void dbnameCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data) ;

  /* Connect to the host and get the list of databases */
  GError *error = NULL ;
  list<string> *db_list = mainFrameGetDatabaseList(main_data, &error) ;

  if (db_list)
    {
      runListDialog(main_data, db_list, GenericListColumn::NAME, 
                    GTK_ENTRY(main_data->dbname_widg), "Select database", FALSE) ;

      delete db_list ;
    }
  else
    {
      zMapCritical("Could not get database list: %s", error ? error->message : "") ;
    }

  return ;
}


/* Called when the user hits the button to search for featureset names. If the user selects featuresets
 * successfully then this updates the value in the featuresets_widg */
static void featuresetsCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->dbname_widg) ;

  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;

  if (dbname && *dbname)
    {
      /* Connect to the database and get the list of featuresets */
      GError *error = NULL ;
      list<string> *featuresets_list = mainFrameGetFeaturesetsList(main_data, &error) ;

      if (featuresets_list)
        {
          runListDialog(main_data, featuresets_list, GenericListColumn::NAME,
                        GTK_ENTRY(main_data->featuresets_widg), "Select featureset(s)", TRUE) ;
          delete featuresets_list ;
        }
      else
        {
          zMapCritical("Could not get featuresets list: %s", error ? error->message : "") ;
        }
    }
  else
    {
      zMapCritical("%s", "Please specify a database first") ;
    }

  return ;
}


/* Called when the user hits the button to search for biotypes. If the user selects biotypes
 * successfully then this updates the value in the biotypes_widg */
static void biotypesCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->dbname_widg) ;

  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;

  if (dbname && *dbname)
    {
      /* Connect to the database and get the list of biotypes */
      GError *error = NULL ;
      list<string> *biotypes_list = mainFrameGetBiotypesList(main_data, &error) ;

      if (biotypes_list)
        {
          runListDialog(main_data, biotypes_list, GenericListColumn::NAME,
                        GTK_ENTRY(main_data->biotypes_widg), "Select biotype(s)", TRUE) ;
          delete biotypes_list ;
        }
      else
        {
          zMapCritical("Could not get biotypes list: %s", error ? error->message : "") ;
        }
    }
  else
    {
      zMapCritical("%s", "Please specify a database first") ;
    }

  return ;
}


#endif /* USE_ENSEMBL */


static gboolean applyTrackhub(MainFrame main_frame)
{
  gboolean ok = FALSE ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *trackdb_id = getEntryText(GTK_ENTRY(main_frame->trackdb_widg)) ;

  if (!source_name)
    {
      zMapWarning("%s", "Please enter a source name") ;
    }
  else
    {
      GError *tmp_error = NULL ;
      
      std::stringstream url ;
      url << "trackhub:///" << trackdb_id ;

      (main_frame->user_func)(source_name, url.str(), NULL, NULL, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapWarning("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  return ok ;
}


/* Show a dialog where the user can search for trackhubs */
static void trackhubSearchCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->search_widg) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Search for Track Hubs",
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;
  GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

  GtkTable *table = GTK_TABLE(gtk_table_new(4, 4, FALSE)) ;
  gtk_box_pack_start(content, GTK_WIDGET(table), FALSE, FALSE, 0) ;

  int row = 0 ;
  int col = 0 ;

  GtkWidget *search_entry = makeEntryWidget("Search text: ", "", 
                                            "MANDATORY: Enter the query string to search for. This can be simple text,\nor more complex queries can be made using wildcards,\nregular expressions, logical and fuzzy operators, proximity searches and grouping.\nSee the documentation at http://trackhubregistry.org/docs/search/advanced",
                                            table, &row, col, col + 2, true, NULL, true) ;
  GtkWidget *species_entry = makeEntryWidget("Species: ", "", "OPTIONAL: Enter the species",
                                             table, &row, col, col + 2, true, NULL, true) ;
  GtkWidget *assembly_entry = makeEntryWidget("Assembly: ", "", "OPTIONAL: Enter the assembly",
                                              table, &row, col, col + 2, true, NULL, true) ;
  GtkWidget *hub_entry = makeEntryWidget("Hub: ", "", "OPTIONAL: Enter the hub name",
                                         table, &row, col, col + 2, true, NULL, true) ;

  /* Run the dialog */
  gtk_widget_show_all(dialog) ;
  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (response == GTK_RESPONSE_OK)
    {
      string search_text(gtk_entry_get_text(GTK_ENTRY(search_entry))) ;
      string species_text(gtk_entry_get_text(GTK_ENTRY(species_entry))) ;
      string assembly_text(gtk_entry_get_text(GTK_ENTRY(assembly_entry))) ;
      string hub_text(gtk_entry_get_text(GTK_ENTRY(hub_entry))) ;

      list<gbtools::trackhub::TrackDb> results = main_data->registry.search(search_text,
                                                                            species_text,
                                                                            assembly_text,
                                                                            hub_text) ;

      runListDialog(main_data, 
                    &results, 
                    TrackListColumn::ID,
                    GTK_ENTRY(main_data->trackdb_widg), 
                    "Select track hub",
                    false) ;
    }

  gtk_widget_destroy(dialog) ;
}


static void loginButtonClickedCB(GtkWidget *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  const char* label = gtk_button_get_label(GTK_BUTTON(button)) ;

  if (strcmp(label, LOGIN_LABEL) == 0)
    {
      // Ask for username and password
      GtkWidget *dialog = gtk_dialog_new_with_buttons("Please log in",
                                                      NULL,
                                                      (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                      NULL);

      gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;
      GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;
  
      gtk_box_pack_start(content, gtk_label_new("User name: "), FALSE, FALSE, 0) ;
      GtkEntry *user_entry = GTK_ENTRY(gtk_entry_new()) ;
      gtk_box_pack_start(content, GTK_WIDGET(user_entry), FALSE, FALSE, 0) ;

      gtk_box_pack_start(content, gtk_label_new("Password: "), FALSE, FALSE, 0) ;
      GtkEntry *pass_entry = GTK_ENTRY(gtk_entry_new()) ;
      gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE) ; // password mode
      gtk_box_pack_start(content, GTK_WIDGET(pass_entry), FALSE, FALSE, 0) ;

      gtk_widget_show_all(dialog) ;
      gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;
      
      if (response == GTK_RESPONSE_OK)
        {
          const char *user = gtk_entry_get_text(GTK_ENTRY(user_entry)) ;
          const char *pass = gtk_entry_get_text(GTK_ENTRY(pass_entry)) ;

          // Attempt to log in to the registry.
          if (main_data->registry.login(user, pass))
            {
              // Update the list of trackdbs
              main_data->user_trackdbs = main_data->registry.retrieveHub() ;
              
              // Change the button label to "logout" 
              gtk_button_set_label(GTK_BUTTON(button), LOGOUT_LABEL) ;
            }
          else
            {
              zMapWarning("%s", "Error logging in") ;
            }
        }

      gtk_widget_destroy(dialog) ;
    }
  else
    {
      // Log out. If successful, change the button label back to "login"
      if (main_data->registry.logout())
        {
          main_data->user_trackdbs.clear() ;
          gtk_button_set_label(GTK_BUTTON(button), LOGIN_LABEL) ;
        }
      else
        {
          zMapWarning("%s", "Error logging out") ;
        }
    }
}


/* Show a dialog where the user can browse trackhubs that they have previously registered */
static void trackhubBrowseCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->browse_widg) ;


  // Create a button where the user can log in / log out
  GtkWidget *login_btn = gtk_button_new_with_label(LOGIN_LABEL) ;

  if (main_data->registry.loggedIn())
    gtk_button_set_label(GTK_BUTTON(login_btn), LOGOUT_LABEL) ;

  g_signal_connect(G_OBJECT(login_btn), "clicked", G_CALLBACK(loginButtonClickedCB), main_data) ;

  // Run a dialog that will show search results in a list
  runListDialog(main_data, 
                &main_data->user_trackdbs, 
                TrackListColumn::ID,
                GTK_ENTRY(main_data->trackdb_widg), 
                "Select track hub",
                false,
                login_btn) ;

}


/* Show a dialog where the user can register a trackhubs */
static void trackhubRegisterCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->search_widg) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Register a Track Hub",
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;

  GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

  GtkBox *vbox = GTK_BOX(gtk_vbox_new(FALSE, 0)) ;
  gtk_box_pack_start(content, GTK_WIDGET(vbox), FALSE, FALSE, 0) ;

  gtk_box_pack_start(vbox, gtk_label_new("URL: "), FALSE, FALSE, 0) ;
  GtkEntry *url_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(vbox, GTK_WIDGET(url_entry), FALSE, FALSE, 0) ;

  gtk_box_pack_start(vbox, gtk_label_new("Assemblies: "), FALSE, FALSE, 0) ;
  GtkEntry *assemblies_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(vbox, GTK_WIDGET(assemblies_entry), FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(GTK_WIDGET(assemblies_entry), 
                              "Semi-colon separated list of key=value pairs, e.g. araTha1=GCA_000001735.1") ;

  gtk_box_pack_start(vbox, gtk_label_new("Type: "), FALSE, FALSE, 0) ;
  GtkEntry *type_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(vbox, GTK_WIDGET(type_entry), FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(GTK_WIDGET(type_entry), "genomics/epigenomics/transcriptomics/proteomics") ;

  GtkWidget *public_btn = gtk_check_button_new_with_label("Public") ;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(public_btn), FALSE) ;
  gtk_box_pack_start(vbox, GTK_WIDGET(public_btn), FALSE, FALSE, 0) ;

  gtk_widget_show_all(dialog) ;

  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (response == GTK_RESPONSE_OK)
    {
      const char *url = gtk_entry_get_text(url_entry) ;
      const char *assemblies = gtk_entry_get_text(assemblies_entry) ;
      const char *type = gtk_entry_get_text(type_entry) ;
      bool is_public = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(public_btn)) ;

      if (url && assemblies)
        {
          //main_data.registry.registerHub(url, assemblies, type, is_public) ;
          gtk_widget_destroy(dialog) ;  
        }
      else
        {
          zMapWarning("%s", "Please enter the URL and list of assemblies") ;
        }
    }
  else
    {
      gtk_widget_destroy(dialog) ;  
    }
}

