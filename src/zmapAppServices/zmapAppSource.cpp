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
#include <ZMap/zmapDataSource.hpp>
#include <ZMap/zmapConfigStrings.hpp>

#ifdef USE_ENSEMBL
#include <ZMap/zmapEnsemblUtils.hpp>

#endif

using namespace std ;
using namespace gbtools::trackhub ;


// unnamed namespace
namespace
{

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
  ZMapAppClosedSequenceViewCB close_func ;
  gpointer close_data ;

  ZMapAppSourceType default_type ;

  // General
  GtkWidget *name_widg ;

  // File
  list<GtkWidget*> file_widgets ;
  GtkWidget *path_widg ;
  char *filename ;

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
  Registry registry;
  list<GtkWidget*> trackhub_widgets;
  GtkWidget *trackdb_id_widg;
  GtkWidget *trackdb_name_widg;
  GtkWidget *trackdb_species_widg;
  GtkWidget *trackdb_assembly_widg;
  GtkWidget *search_widg;
  GtkWidget *browse_widg;
  GtkWidget *register_widg;
  GtkWidget *login_widg;

} MainFrameStruct, *MainFrame ;


/* Utility class to hold data about the users' trackhubs */
class UserTrackhubs
{
public:
  UserTrackhubs(GtkWindow *parent,
                MainFrame main_data, 
                list<TrackDb> &trackdbs_list, 
                GtkTreeView *list_widget) 
    : parent_(parent),
      main_data_(main_data),
      trackdbs_list_(trackdbs_list),
      list_widget_(list_widget)
  {} ;

  GtkWindow *parent_ ;
  MainFrame main_data_ ;
  list<TrackDb> &trackdbs_list_ ;
  GtkTreeView *list_widget_ ;
} ;


/* Utility class to pass data about a search-list to callback functions */
class SearchListData
{
public:
  SearchListData() 
    : search_entry(NULL),
      filter_entry(NULL),
      tree_model(NULL)
  {} ;

  GtkEntry *search_entry ;
  GtkEntry *filter_entry ;
  GtkTreeModel *tree_model ;
} ;


GtkWidget *makePanel(GtkWidget *toplevel,
                     gpointer *seqdata_out,
                     ZMapFeatureSequenceMap sequence_map,
                     ZMapAppCreateSourceCB user_func,
                     gpointer user_data,
                     ZMapAppClosedSequenceViewCB close_func,
                     gpointer close_data,
                     ZMapAppSourceType default_type,
                     MainFrame *main_data) ;
GtkWidget *makeButtonBox(MainFrame main_data) ;
void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
void cancelCB(GtkWidget *widget, gpointer cb_data) ;
void applyCB(GtkWidget *widget, gpointer cb_data) ;
GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;
gboolean applyFile(MainFrame main_data) ;
void setWidgetsVisibility(list<GtkWidget*> &widget_list, const gboolean visible) ;
void updatePanelFromSource(MainFrame main_data, ZMapConfigSource source) ;
gboolean applyTrackhub(MainFrame main_frame);

void trackhubSearchCB(GtkWidget *widget, gpointer cb_data);
void trackhubBrowseCB(GtkWidget *widget, gpointer cb_data);
void trackhubRegisterCB(GtkWidget *widget, gpointer cb_data);

template<class ListType, class ColType>
gboolean runListDialog(MainFrame main_data, 
                       const ListType &values_list, 
                       const ColType result_col,
                       GtkEntry *result_widg,
                       const char *title,
                       const gboolean allow_multiple) ;

#ifdef USE_ENSEMBL
void dbnameCB(GtkWidget *widget, gpointer cb_data) ;
void featuresetsCB(GtkWidget *widget, gpointer cb_data) ;
void biotypesCB(GtkWidget *widget, gpointer cb_data) ;
gboolean applyEnsembl(MainFrame main_data) ;
#endif /* USE_ENSEMBL */



/*
 *                   Internal routines.
 */

/* Make the whole panel returning the top container of the panel. */
GtkWidget *makePanel(GtkWidget *toplevel, 
                     gpointer *our_data,
                     ZMapFeatureSequenceMap sequence_map,
                     ZMapAppCreateSourceCB user_func,
                     gpointer user_data,
                     ZMapAppClosedSequenceViewCB close_func,
                     gpointer close_data,
                     ZMapAppSourceType default_type,
                     MainFrame *main_data_out)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box ;
  MainFrame main_data ;

  main_data = new MainFrameStruct ;

  main_data->sequence_map = sequence_map ;
  main_data->user_func = user_func ;
  main_data->user_data = user_data ;
  main_data->close_func = close_func ;
  main_data->close_data = close_data ;
  main_data->default_type = default_type ;
  main_data->filename = NULL ;

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
int comboGetIndex(MainFrame main_data, ZMapURLScheme scheme_in)
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


void updatePanelFromFileSource(MainFrame main_data, 
                               ZMapConfigSource source,
                               ZMapURL zmap_url)
{
  char *source_name = main_data->sequence_map->getSourceName(source) ;

  gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), source_name) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->path_widg), zmap_url->path) ;

  if (source_name)
    g_free(source_name) ;
}

void updatePanelFromTrackhubSource(MainFrame main_data, 
                                   ZMapConfigSource source,
                                   ZMapURL zmap_url)
{
  zMapReturnIfFail(main_data) ;

  char *source_name = main_data->sequence_map->getSourceName(source) ;
  const char *trackdb_id = "" ;

  if (zmap_url)
    trackdb_id = zmap_url->file ;

  gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), source_name) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->trackdb_id_widg), trackdb_id) ;

  g_free(source_name) ;
}


#ifdef USE_ENSEMBL
void updatePanelFromEnsemblSource(MainFrame main_data, 
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
void updatePanelFromSource(MainFrame main_data, ZMapConfigSource source)
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


GtkWidget* makeEntryWidget(const char *label_str, 
                           const char *default_value,
                           const char *tooltip,
                           GtkTable *table,
                           int *row,
                           const int col,
                           const int max_col, 
                           gboolean mandatory,
                           list<GtkWidget*> *widget_list,
                           const bool activates_default = true)
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


#ifndef __CYGWIN__
/* Called when user chooses a file via the file dialog. Updates the filename text entry box. */
static void chooseFileCB(GtkFileChooserButton *widget, gpointer user_data)
{
  MainFrame main_data = (MainFrame) user_data ;
  char *filename = NULL ;

  filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) ;

  if (filename)
    {
      gtk_entry_set_text(GTK_ENTRY(main_data->path_widg), g_strdup(filename)) ;

      if (main_data->filename)
        g_free(main_data->filename) ;

      main_data->filename = filename ;

      /* If the source name is not already set, set it by default to the basename of the path */
      const char *source_name = gtk_entry_get_text(GTK_ENTRY(main_data->name_widg)) ;

      if (!source_name || *source_name == '\0')
        gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), g_path_get_basename(filename));
    }
}
#endif


/* Similar to makeEntryWidget but adds a file chooser button instead of a label */
GtkWidget* makeFileChooserWidget(const char *label_str,
                                 const char *default_value,
                                 const char *tooltip,
                                 GtkTable *table,
                                 int *row,
                                 const int col,
                                 const int max_col,
                                 gboolean mandatory,
                                 list<GtkWidget*> *widget_list,
                                 const bool activates_default,
                                 MainFrame main_data)
{
  /* Create a file chooser button instead of a label (this will just be a label on cygwin) */
  GtkWidget *config_button = NULL ;

#ifndef __CYGWIN__
  /* N.B. we use the gtk "built-in" file chooser stuff. Create a file-chooser button instead of
   * placing a label next to the file text-entry box */
  config_button = gtk_file_chooser_button_new("Choose a File to Import", GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(config_button), "file-set", GTK_SIGNAL_FUNC(chooseFileCB), (gpointer)main_data) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(config_button), TRUE) ;

  char *default_dir = NULL ;
  
  if (default_value)
    default_dir = g_path_get_basename(default_value) ;
  else
    default_dir = g_strdup(g_get_home_dir()) ;

  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(config_button), default_dir) ;

  g_free(default_dir) ;
#else
  /* We can't have a file-chooser button, but we already have a text entry
   * box for the filename anyway, so just create a label for that where the
   * button would have been */
  config_button = gtk_label_new(label_str) ;
  gtk_misc_set_alignment(GTK_MISC(config_button), 1.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(config_button), GTK_JUSTIFY_RIGHT) ;
#endif

  gtk_table_attach(table, config_button, col, col + 1, *row, *row + 1, GTK_FILL, GTK_SHRINK, xpad, ypad) ;

  if (mandatory)
    {
      GdkColor color ;
      gdk_color_parse("red", &color) ;
      gtk_widget_modify_fg(config_button, GTK_STATE_NORMAL, &color) ;
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
      gtk_widget_set_tooltip_text(config_button, tooltip) ;
      gtk_widget_set_tooltip_text(entry, tooltip) ;
    }

  if (widget_list)
    {
      widget_list->push_back(config_button) ;
      widget_list->push_back(entry) ;
    }

  return entry ;
}

/* Set the visibility of all the widgets in the given list */
void setWidgetsVisibility(list<GtkWidget*> &widget_list, const gboolean visible)
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
char* comboGetValue(GtkComboBox *combo)
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


void sourceTypeChangedCB(GtkComboBox *combo, gpointer data)
{
  MainFrame main_data = (MainFrame)data ;
  zMapReturnIfFail(main_data) ;

  char *value = comboGetValue(combo) ;

  if (value)
    {
      setWidgetsVisibility(main_data->file_widgets,     strcmp(value, SOURCE_TYPE_FILE) == 0) ;
      setWidgetsVisibility(main_data->trackhub_widgets, strcmp(value, SOURCE_TYPE_TRACKHUB) == 0) ;
#ifdef USE_ENSEMBL
      setWidgetsVisibility(main_data->ensembl_widgets,  strcmp(value, SOURCE_TYPE_ENSEMBL) == 0) ;
#endif /* USE_ENSEMBL */

      g_free(value) ;
    }
}


void createComboItem(MainFrame main_data,
                     GtkComboBox *combo, 
                     GtkListStore *store,
                     ZMapURLScheme scheme,
                     const char *text,
                     unsigned int &combo_index,
                     const bool active)
{
  GtkTreeIter iter;
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, GenericListColumn::NAME, text, -1);

  main_data->combo_indices[scheme] = combo_index ;
  ++combo_index ;

  if (active)
    gtk_combo_box_set_active_iter(combo, &iter);
}


GtkComboBox *createComboBox(MainFrame main_data)
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

  createComboItem(main_data, combo, store, 
                  SCHEME_FILE, SOURCE_TYPE_FILE, combo_index,
                  main_data->default_type == ZMapAppSourceType::FILE) ;

  createComboItem(main_data, combo, store, 
                  SCHEME_TRACKHUB, SOURCE_TYPE_TRACKHUB, combo_index,
                  main_data->default_type == ZMapAppSourceType::TRACKHUB) ;

#ifdef USE_ENSEMBL
  createComboItem(main_data, combo, store, 
                  SCHEME_ENSEMBL, SOURCE_TYPE_ENSEMBL, combo_index,
                  main_data->default_type == ZMapAppSourceType::ENSEMBL) ;
#endif

  g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(sourceTypeChangedCB), main_data);

  return combo ;
}


GtkWidget* makeButtonWidget(const char *stock,
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
void makeEnsemblWidgets(MainFrame main_data, 
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
#endif /* USE_ENSEMBL */


/* This is called after the trackdb_id has changed. It looks up the trackdb_id
 * in the registry and populates the other details from the results, or clears the detail boxes
 * if the trackdb_id is not found in the registry. */
void onTrackDbIdChanged(GtkEditable *editable, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(main_data && 
                   main_data->name_widg &&
                   main_data->trackdb_id_widg &&
                   main_data->trackdb_name_widg &&
                   main_data->trackdb_species_widg &&
                   main_data->trackdb_assembly_widg) ;

  const char *trackdb_id = gtk_entry_get_text(GTK_ENTRY(main_data->trackdb_id_widg)) ;

  // Find the trackdb in the registry, if it exists, and update the trackdb details. (These will
  // be empty strings in the TrackDb class if not found so will clear the boxes.)
  string err_msg;
  TrackDb trackdb = main_data->registry.searchTrackDb(trackdb_id, err_msg) ;
  
  if (!err_msg.empty())
    zMapLogWarning("%s", err_msg.c_str()) ;
      
  gtk_entry_set_text(GTK_ENTRY(main_data->trackdb_name_widg), trackdb.name().c_str()) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->trackdb_species_widg), trackdb.species().c_str()) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->trackdb_assembly_widg), trackdb.assembly().c_str()) ;

  // Use the trackdb name for the source name
  gtk_entry_set_text(GTK_ENTRY(main_data->name_widg), trackdb.name().c_str()) ;
}

/* Run a dialog that asks for the user credentials and logs in to the trackhub registry. Returns
 * true if logged in successfully */
bool runLoginDialog(MainFrame main_data)
{
  bool result = false ;

  // Create a dialog to ask for username and password
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Please log in",
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

  GtkWidget *label = gtk_label_new("Please log in to the Track Hub Registry.\n\nTo register for an account, visit:\nhttp://trackhubregistry.org/") ;
  gtk_table_attach(table, label, col, col + 2, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  ++row ;

  GtkWidget *user_entry = makeEntryWidget("User name: ", "", 
                                          "MANDATORY: Enter your user name",
                                          table, &row, col, col + 2, true, NULL, true) ;
  GtkWidget *pass_entry = makeEntryWidget("Password: ", "", 
                                          "MANDATORY: Enter your password",
                                          table, &row, col, col + 2, true, NULL, true) ;

  gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE) ; // password mode

  // Run the dialog
  gtk_widget_show_all(dialog) ;
  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;
      
  if (response == GTK_RESPONSE_OK)
    {
      const char *user = gtk_entry_get_text(GTK_ENTRY(user_entry)) ;
      const char *pass = gtk_entry_get_text(GTK_ENTRY(pass_entry)) ;

      // Attempt to log in to the registry.
      string err_msg;
      if (main_data->registry.login(user, pass, err_msg) && err_msg.empty())
        result = true ;
      else
        zMapCritical("%s", "Error logging in: %s", err_msg.c_str()) ;
    }

  gtk_widget_destroy(dialog) ;

  // Update the login button text
  if (main_data->registry.loggedIn())
    gtk_button_set_label(GTK_BUTTON(main_data->login_widg), LOGOUT_LABEL) ;
  else
    gtk_button_set_label(GTK_BUTTON(main_data->login_widg), LOGIN_LABEL) ;

  return result ;
}


/* Callback called when the user hits the login/logout button */
void loginButtonClickedCB(GtkWidget *button, gpointer data)
{
  MainFrame main_data = (MainFrame)data ;
  zMapReturnIfFail(main_data) ;

  const char* label = gtk_button_get_label(GTK_BUTTON(button)) ;

  if (strcmp(label, LOGIN_LABEL) == 0)
    {
      if (runLoginDialog(main_data))
        {
          // Change the button label to "logout" 
          gtk_button_set_label(GTK_BUTTON(button), LOGOUT_LABEL) ;
        }
    }
  else
    {
      // Log out. If successful, change the button label back to "login"
      string err_msg;
      if (main_data->registry.logout(err_msg) && err_msg.empty())
        {
          gtk_button_set_label(GTK_BUTTON(button), LOGIN_LABEL) ;
        }
      else
        {
          zMapCritical("%s", "Error logging out: %s", err_msg.c_str()) ;
        }
    }
}


void makeTrackhubWidgets(MainFrame main_data, 
                         ZMapFeatureSequenceMap sequence_map,
                         GtkTable *table,
                         const int rows,
                         const int cols,
                         int &row,
                         int &col)
{
  /* Display the trackDb details */
  main_data->trackdb_id_widg = makeEntryWidget("ID :", NULL, "REQUIRED: The track database ID from the Ensembl Track Hub Registry. Click on the Search button to look up a track hub.",
                                               table, &row, col, col + 6, TRUE, &main_data->trackhub_widgets);
  main_data->trackdb_name_widg = makeEntryWidget("Name :", NULL, "The hub name",
                                                 table, &row, col, col + 6, FALSE, &main_data->trackhub_widgets);
  main_data->trackdb_species_widg = makeEntryWidget("Species :", NULL, "The species this hub belongs to",
                                                    table, &row, col, col + 6, FALSE, &main_data->trackhub_widgets);
  main_data->trackdb_assembly_widg = makeEntryWidget("Assembly :", NULL, "The assembly this hub belongs to",
                                                     table, &row, col, col + 6, FALSE, &main_data->trackhub_widgets);

  /* Connect a signal to update the other details after the trackdb_id has changed */
  g_signal_connect(main_data->trackdb_id_widg, "changed", G_CALLBACK(onTrackDbIdChanged), main_data) ;
  
  /* Grey out the details boxes because they should not be edited directly */
  gtk_widget_set_sensitive(main_data->trackdb_name_widg, FALSE) ;
  gtk_widget_set_sensitive(main_data->trackdb_species_widg, FALSE) ;
  gtk_widget_set_sensitive(main_data->trackdb_assembly_widg, FALSE) ;

  col += 2;

  /* Search button */
  main_data->search_widg = makeButtonWidget(NULL, "Search", false, "Search for track hubs",
                                            GTK_SIGNAL_FUNC(trackhubSearchCB), main_data,
                                            table, row, col,
                                            main_data->trackhub_widgets);
  ++col;

  /* Register button */
  main_data->register_widg = makeButtonWidget(NULL, "Register", false, "Register a new track hub",
                                              GTK_SIGNAL_FUNC(trackhubRegisterCB), main_data,
                                              table, row, col,
                                              main_data->trackhub_widgets);
  ++col;

  /* View registered hubs button */
  main_data->browse_widg = makeButtonWidget(NULL, "My hubs", false, "View all of your registered track hubs",
                                            GTK_SIGNAL_FUNC(trackhubBrowseCB), main_data,
                                            table, row, col,
                                            main_data->trackhub_widgets);
  ++col;

  /* Login/logout button */
  main_data->login_widg = makeButtonWidget(NULL, LOGIN_LABEL, false, "Log in to the track hub registry",
                                           GTK_SIGNAL_FUNC(loginButtonClickedCB), main_data,
                                           table, row, col,
                                           main_data->trackhub_widgets);

  if (main_data->registry.loggedIn())
    gtk_button_set_label(GTK_BUTTON(main_data->login_widg), LOGOUT_LABEL) ;
}


/* Make the label/entry fields frame. */
GtkWidget *makeMainFrame(MainFrame main_data, 
                         ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame = gtk_frame_new( "New source: " ) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  const int rows = 7 ;
  const int cols = 7 ;
  GtkTable *table = GTK_TABLE(gtk_table_new(rows, cols, FALSE)) ;
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(table)) ;

  int row = 0 ;
  int col = 0 ;

  main_data->combo = createComboBox(main_data) ;
  gtk_table_attach(table, gtk_label_new("Source type :"), col, col + 1, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  gtk_table_attach(table, GTK_WIDGET(main_data->combo), col + 1, col + 2, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
  ++row ;

  /* Get the default filename to display, if any */
  const char *filename = main_data->filename ;

  /* Create the label/entry widgets */

  /* First column */
  main_data->name_widg = makeEntryWidget("Source name :", NULL, "REQUIRED: Please enter a name for the new source", 
                                         table, &row, col, col + 2, TRUE, NULL) ;
  int path_widg_row = row;

  main_data->path_widg = makeFileChooserWidget("Path/URL :", filename, "REQUIRED: The file/URL to load features from",
                                               table, &row, col, col + 2, TRUE, &main_data->file_widgets, TRUE, main_data) ;

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
GtkWidget *makeButtonBox(MainFrame main_data)
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

  if (GTK_IS_WINDOW(main_data->toplevel))
    gtk_window_set_default(GTK_WINDOW(main_data->toplevel), ok_button) ;

  return frame ;
}



/* This function gets called whenever there is a gtk_widget_destroy() to the top level
 * widget. Sometimes this is because of window manager action, sometimes one of our exit
 * routines does a gtk_widget_destroy() on the top level widget.
 */
void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  /* Signal app we are going if we are a separate window. */
  if (main_data->close_func)
    (main_data->close_func)(main_data->toplevel, main_data->close_data) ;

  if (main_data->filename)
    g_free(main_data->filename) ;

  delete main_data ;

  return ;
}


/* Kill the dialog. */
void cancelCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  gtk_widget_destroy(main_data->toplevel) ;

  return ;
}


/* Create the new source. */
void applyCB(GtkWidget *widget, gpointer cb_data)
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
const char* getEntryText(GtkEntry *entry)
{
  const char *result = NULL ;

  if (entry)
    result = gtk_entry_get_text(entry) ;

  if (result && *result == 0)
    result = NULL ;

  return result ;
}



/* Get the script to use for the given file type. This should not be hard-coded really but long
 * term we will hopefully not need these scripts when zmap can do the remapping itself. Returns
 * null if not found or not executable */
static const char* fileTypeGetPipeScript(ZMapDataSourceType source_type, 
                                         string &err_msg)
{
  const char *result = NULL ;

  if (source_type == ZMapDataSourceType::HTS)
    result = "bam_get" ;
  else if (source_type == ZMapDataSourceType::BIGWIG)
    result = "bigwig_get" ;
  else
    err_msg = "File type does not have a remapping script" ;

  /* Check it's executable */
  if (result && !g_find_program_in_path(result))
    {
      err_msg = "Script '" ;
      err_msg += result ;
      err_msg += "' is not executable" ;
      result = NULL ;
    }

  return result ;
}


/* Returns true if zmap is running under otter (according to the config file) */
static bool runningUnderOtter(MainFrame main_frame)
{
  bool is_otter = false ;

  if (main_frame && main_frame->sequence_map && main_frame->sequence_map->config_file)
    {
      ZMapConfigIniContext context = zMapConfigIniContextProvide(main_frame->sequence_map->config_file, 
                                                                 ZMAPCONFIG_FILE_NONE);

      if (context)
        {
          char *tmp_string = NULL;
          
          if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                           ZMAPSTANZA_APP_CSVER, &tmp_string))
            {
              if(tmp_string && !g_ascii_strcasecmp(tmp_string,"Otter"))
                {
                  is_otter = TRUE;
                }

              if (tmp_string)
                {
                  g_free(tmp_string) ;
                  tmp_string = NULL ;
                }
            }
        }

    }      

  return is_otter ;
}


/* Create the url for normal file types */
static string constructRegularFileURL(const char *filename, string &err_msg)
{
  string url;

  // Prepend file:///, unless its an http/ftp url
  if (strncasecmp(filename, "http://", 7) != 0 &&
      strncasecmp(filename, "https://", 8) != 0 &&
      strncasecmp(filename, "ftp://", 6) != 0)
    {
      url += "file:///" ;
    }

  url += filename ;

  return url ;
}


/* Create the url for special file types that use a pipe script (this is because we use these
 * scripts for now to do remapping but when zmap can do its own remapping we can get rid of
 * this). Note that these scripts are only available when running under otter. */
static string constructPipeFileURL(MainFrame main_frame, 
                                   const char *source_name,
                                   const char *filename, 
                                   ZMapDataSourceType source_type,
                                   string &err_msg)
{
  string url;
  zMapReturnValIfFail(main_frame, url) ;

  bool done = false ;
  
  if (runningUnderOtter(main_frame))
    {
      /* Ask the user if they want to remap, and if ask, for the assembly */
      char *source_assembly = NULL ;
      GtkResponseType response = zMapGUIMsgGetText(GTK_WINDOW(main_frame->toplevel),
                                                   ZMAP_MSG_INFORMATION, 
                                                   "Is this source from another assembly?\n\nIf so, enter the assembly name to remap (leave blank to skip):",
                                                   TRUE,
                                                   &source_assembly) ;

      if (response == GTK_RESPONSE_OK && source_assembly && *source_assembly)
        {
          /* Use a pipe script that will remap the features if necessary. This is also required
           * in order to be able to blixem the columns because blixem doesn't support bam reading
           * at the moment, so the script command gets passed through to blixem. */
          const char *script = fileTypeGetPipeScript(source_type, err_msg) ;

          if (script)
            {
              /* The sequence name in the file may be different to the sequence name in
               * zmap. Ask the user to enter the lookup name, if different. */
              char *req_sequence = NULL ;
              response = zMapGUIMsgGetText(GTK_WINDOW(main_frame->toplevel),
                                           ZMAP_MSG_INFORMATION, 
                                           "ZMap will look for features in the file for the following sequence.\n\nIf the sequence name in the file is different, please enter the correct name here:",
                                           TRUE,
                                           &req_sequence) ;

              if (response == GTK_RESPONSE_OK && req_sequence && *req_sequence)
                {
                  GString *args = g_string_new("") ;

                  g_string_append_printf(args, "--file=%s&--chr=%s&--start=%d&--end=%d&--gff_seqname=%s&--gff_source=%s",
                                         filename, 
                                         main_frame->sequence_map->sequence,
                                         main_frame->sequence_map->start,
                                         main_frame->sequence_map->end,
                                         req_sequence, 
                                         source_name) ;

                  if (source_type == ZMapDataSourceType::BIGWIG)
                    {
                      // for bigwig, we need the strand for the script. ask the user.
                      bool forward_strand = zMapGUIMsgGetBoolFull(GTK_WINDOW(main_frame->toplevel), 
                                                                  ZMAP_MSG_INFORMATION,
                                                                  "Is this data from the forward or reverse strand?",
                                                                  "Forward", "Reverse") ;

                      g_string_append_printf(args, "&--strand=%d", (forward_strand ? 1 : -1)) ;
                    }

                  g_string_append_printf(args, "&--csver_remote=%s&--dataset=%s", 
                                         source_assembly, 
                                         main_frame->sequence_map->dataset) ;

                  done = true ;
                }
            }
        }
    }

  if (!done && err_msg.empty())
    {
      url = constructRegularFileURL(filename, err_msg) ;
    }

  return url ;
}


/* Create a valid url for the given file (which should either be a filename on the local system
 * or a remote file url e.g. starting http:// */
static string constructFileURL(MainFrame main_frame, 
                               const char *source_name,
                               const char *filename, 
                               GError **error)
{
  string url("");

  GError *g_error = NULL ;
  ZMapDataSourceType source_type = zMapDataSourceTypeFromFilename(filename, &g_error) ;

  if (g_error)
    {
      g_propagate_error(error, g_error) ;
    }
  else
    {
      string err_msg;

      // Some file types currently have special treatment
      switch (source_type)
        {
        case ZMapDataSourceType::GIO: //fall through
        case ZMapDataSourceType::HTS: //fall through
        case ZMapDataSourceType::BCF:
        case ZMapDataSourceType::BIGBED: //fall through
          url = constructRegularFileURL(filename, err_msg) ;
          break ;

        case ZMapDataSourceType::BED:    //fall through
        case ZMapDataSourceType::BIGWIG:
          
          url = constructPipeFileURL(main_frame, source_name, filename, source_type, err_msg) ;
          break ;

        case ZMapDataSourceType::UNK:
          // Shouldn't get here because g_error should have been set
          zMapWarnIfReached() ;
          break ;
        }

      if (!err_msg.empty())
        {
          g_set_error(&g_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_URL, "%s", err_msg.c_str()) ;
          g_propagate_error(error, g_error) ;
        }
    }

  return url ;
}


gboolean applyFile(MainFrame main_frame)
{
  gboolean ok = FALSE ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *path = getEntryText(GTK_ENTRY(main_frame->path_widg)) ;

  if (!source_name)
    {
      zMapCritical("%s", "Please enter a source name") ;
    }
  else if (!path)
    {
      zMapCritical("%s", "Please enter a path/URL") ;
    }
  else
    {
      GError *tmp_error = NULL ;
      string url = constructFileURL(main_frame, source_name, path, &tmp_error) ;

      if (!tmp_error)
        (main_frame->user_func)(source_name, url, NULL, NULL, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapCritical("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  return ok ;
}


/* 
 * Ensembl functions
 */

#ifdef USE_ENSEMBL

string constructUrl(const char *host, const char *port,
                    const char *user, const char *pass,
                    const char *dbname, const char *dbprefix)
{
  string result ;
  stringstream url ;

  url << "ensembl://" ;
  
  if (user)
    url << user ;

  url << ":" ;

  if (pass)
    url << pass ;

  url << "@" ;

  if (host)
    url << host ;

  url << ":" ;

  if (port)
    url << port ;

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

  result = url.str() ;
  return result ;
}


gboolean applyEnsembl(MainFrame main_frame)
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
      zMapCritical("%s", "Please enter a source name") ;
    }
  else if (!host)
    {
      zMapCritical("%s", "Please enter a host") ;
    }
  else if (!dbname)
    {
      zMapCritical("%s", "Please enter a database name") ;
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
        zMapCritical("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  if (tmp)
    g_free(tmp) ;

  return ok ;
}


/* Get the list of databases available from the host specified in the entry widgets */
list<string> mainFrameGetDatabaseList(MainFrame main_data, GError **error)
{
  list<string> db_list ;
  
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
list<string> mainFrameGetFeaturesetsList(MainFrame main_data, GError **error)
{
  list<string> db_list ;
  
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
list<string> mainFrameGetBiotypesList(MainFrame main_data, GError **error)
{
  list<string> db_list ;
  
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


/* Called when the user hits the button to search for a database name. If the user selects a name
 * successfully then this updates the value in the dbname_widg */
void dbnameCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data) ;

  /* Connect to the host and get the list of databases */
  GError *error = NULL ;
  list<string> db_list = mainFrameGetDatabaseList(main_data, &error) ;

  if (!db_list.empty())
    {
      runListDialog(main_data, db_list, GenericListColumn::NAME, 
                    GTK_ENTRY(main_data->dbname_widg), "Select database", FALSE) ;
    }
  else
    {
      zMapCritical("Could not get database list: %s", error ? error->message : "") ;
    }

  return ;
}


/* Called when the user hits the button to search for featureset names. If the user selects featuresets
 * successfully then this updates the value in the featuresets_widg */
void featuresetsCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->dbname_widg) ;

  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;

  if (dbname && *dbname)
    {
      /* Connect to the database and get the list of featuresets */
      GError *error = NULL ;
      list<string> featuresets_list = mainFrameGetFeaturesetsList(main_data, &error) ;

      if (!featuresets_list.empty())
        {
          runListDialog(main_data, featuresets_list, GenericListColumn::NAME,
                        GTK_ENTRY(main_data->featuresets_widg), "Select featureset(s)", TRUE) ;
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
void biotypesCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->dbname_widg) ;

  const char *dbname = gtk_entry_get_text(GTK_ENTRY(main_data->dbname_widg)) ;

  if (dbname && *dbname)
    {
      /* Connect to the database and get the list of biotypes */
      GError *error = NULL ;
      list<string> biotypes_list = mainFrameGetBiotypesList(main_data, &error) ;

      if (!biotypes_list.empty())
        {
          runListDialog(main_data, biotypes_list, GenericListColumn::NAME,
                        GTK_ENTRY(main_data->biotypes_widg), "Select biotype(s)", TRUE) ;
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

/* Case insensitive version of strstr */
char* my_strcasestr(const char *haystack, const char *needle)
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
template<class ColType>
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
gboolean track_list_filter_visible_cb(GtkTreeModel *model, 
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


/* Update the given entry with the selected value(s) from the given tree. Uses the given column
 * in the tree for the result */
template<class ColType>
gboolean setEntryFromSelection(GtkTreeView *tree_view,
                               GtkEntry *entry, 
                               const ColType col_id)
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
void filter_entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
  SearchListData *search_data = (SearchListData*)user_data ;
  zMapReturnIfFail(search_data && search_data->tree_model) ;

  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(search_data->tree_model) ;

  /* Refilter the filtered model */
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;
}


void clear_button_cb(GtkButton *button, gpointer user_data)
{
  SearchListData *search_data = (SearchListData*)user_data ;
  zMapReturnIfFail(search_data && search_data->tree_model) ;

  if (search_data->search_entry)
    gtk_entry_set_text(search_data->search_entry, "") ;

  if (search_data->filter_entry)
    gtk_entry_set_text(search_data->filter_entry, "") ;

  /* Refilter the filtered model */
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(search_data->tree_model) ;
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;
}


/* Callback used to determine if a given row in the filtered tree model is visible */
gboolean generic_list_filter_visible_cb(GtkTreeModel *model, 
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


void listStorePopulate(GtkListStore *store,
                       const list<string> &vals_list)
{
  /* Loop through all of the names */
  for (const auto &name : vals_list)
    {
      /* Create a new row in the list store and set the column values */
      GtkTreeIter store_iter ;
      gtk_list_store_append(store, &store_iter);

      gtk_list_store_set(store, &store_iter, 
                         GenericListColumn::NAME, name.c_str(), 
                         -1);
    }
}

void listStorePopulate(GtkListStore *store,
                       const list<TrackDb> &trackdb_list)
{
  /* Loop through all of the trackdbs */
  for (const auto &trackdb : trackdb_list)
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


/* Utility to create a text column in a tree view */
template<class ColType>
void createTreeViewTextColumn(GtkTreeView *tree_view, 
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


/* Create a tree view widget to show name values in the given list */
GtkTreeView* createListWidget(MainFrame main_data, 
                              const list<string> &val_list,
                              SearchListData &search_data, 
                              const gboolean allow_multiple)
{
  /* Create the data store */
  GtkListStore *store = gtk_list_store_new((int)GenericListColumn::N_COLS, G_TYPE_STRING) ;

  /* Create a filtered version of the data store */
  GtkTreeModel *filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL) ;
  search_data.tree_model = filtered ;

  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
                                         generic_list_filter_visible_cb, 
                                         search_data.filter_entry,
                                         NULL);

  listStorePopulate(store, val_list) ;

  /* Create the tree widget */
  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(filtered))) ;

  /* Set various properties on the tree widget */
  gtk_tree_view_set_enable_search(tree_view, FALSE);
  gtk_tree_view_set_search_column(tree_view, (int)GenericListColumn::NAME);
  gtk_tree_view_set_search_entry(tree_view, search_data.search_entry) ;
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
GtkTreeView* createListWidget(MainFrame main_data, 
                              const list<TrackDb> &trackdb_list, 
                              SearchListData &search_data, 
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
  search_data.tree_model = filtered ;

  gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
                                         track_list_filter_visible_cb, 
                                         search_data.filter_entry,
                                         NULL);

  listStorePopulate(store, trackdb_list) ;

  /* Create the tree widget */
  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(filtered))) ;

  /* Set various properties on the tree widget */
  gtk_tree_view_set_enable_search(tree_view, FALSE);
  gtk_tree_view_set_search_column(tree_view, (int)TrackListColumn::NAME);
  gtk_tree_view_set_search_entry(tree_view, search_data.search_entry) ;
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


template<class ListType>
GtkWidget* createListDialog(MainFrame main_data, 
                            const ListType &values_list, 
                            const char *title,
                            const gboolean allow_multiple,
                            SearchListData &search_data,
                            GtkTreeView **list_widget_out)
{
  GtkWidget *dialog = NULL ;
  zMapReturnValIfFail(main_data, dialog) ;

  dialog = gtk_dialog_new_with_buttons(title,
                                       NULL,
                                       (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT),
                                       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK, GTK_RESPONSE_OK,
                                       NULL);

  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;
  int width = 0 ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(dialog, &width, &height))
    gtk_window_set_default_size(GTK_WINDOW(dialog), width * 0.3, height * 0.5) ;

  GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

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

  search_data.search_entry = search_entry ;
  search_data.filter_entry = filter_entry ;
  g_signal_connect(G_OBJECT(filter_entry), "activate", G_CALLBACK(filter_entry_activate_cb), &search_data) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clear_button_cb), &search_data) ;


  /* Scrolled list of all values */
  GtkWidget *scrollwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(content, GTK_WIDGET(scrollwin), TRUE, TRUE, 0) ;

  GtkTreeView *list_widget = createListWidget(main_data, values_list, search_data, allow_multiple) ;
  gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(list_widget)) ;

  if (list_widget_out)
    *list_widget_out = list_widget ;

  return dialog ;
}


/* Create and run a dialog to show the given list of values in a gtk tree view and to set the
 * given entry widget with the result when the user selects a row and hits ok. If the user
 * selects multiple values then it sets a semi-colon-separated list in the entry widget.
 * This function can take lists of different types of values and will call an overload of
 * createListWidget for the relevant type. */
template<class ListType, class ColType>
gboolean runListDialog(MainFrame main_data, 
                       const ListType &values_list, 
                       const ColType result_col,
                       GtkEntry *result_widg,
                       const char *title,
                       const gboolean allow_multiple)
{
  gboolean ok = FALSE ;
  zMapReturnValIfFail(main_data, ok) ;

  GtkTreeView *list_widget = NULL ;
  SearchListData search_data ;
  GtkWidget *dialog = createListDialog(main_data, values_list, title, allow_multiple, search_data, &list_widget) ;

  if (dialog)
    {
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
      else
        {
          gtk_widget_destroy(dialog) ;
          ok = TRUE ;
        }
    }
  else
    {
      zMapCritical("%s", "Failed to create list dialog") ;
    }

  return ok ;
}


gboolean applyTrackhub(MainFrame main_frame)
{
  gboolean ok = FALSE ;

  const char *source_name = getEntryText(GTK_ENTRY(main_frame->name_widg)) ;
  const char *trackdb_id = getEntryText(GTK_ENTRY(main_frame->trackdb_id_widg)) ;

  if (!source_name)
    {
      zMapCritical("%s", "Please enter a source name") ;
    }
  else
    {
      GError *tmp_error = NULL ;
      
      std::stringstream url ;
      url << "trackhub:///" << trackdb_id ;

      (main_frame->user_func)(source_name, url.str(), NULL, NULL, main_frame->user_data, &tmp_error) ;

      if (tmp_error)
        zMapCritical("Failed to create new source: %s", tmp_error->message) ;

      ok = TRUE ;
    }

  return ok ;
}


/* Show a dialog where the user can search for trackhubs */
void trackhubSearchCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->search_widg) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Search for Track Hubs",
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT),
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
                                            "MANDATORY: Enter the query string to search for. This can be simple text, or more complex queries can be made using wildcards, regular expressions, logical and fuzzy operators, proximity searches and grouping. See the documentation at http://trackhubregistry.org/docs/search/advanced",
                                            table, &row, col, col + 2, true, NULL, true) ;
  GtkWidget *species_entry = makeEntryWidget("Species: ", "", "OPTIONAL: Enter the species",
                                             table, &row, col, col + 2, false, NULL, true) ;
  GtkWidget *assembly_entry = makeEntryWidget("Assembly: ", "", "OPTIONAL: Enter the assembly",
                                              table, &row, col, col + 2, false, NULL, true) ;
  GtkWidget *hub_entry = makeEntryWidget("Hub: ", "", "OPTIONAL: Enter the hub name",
                                         table, &row, col, col + 2, false, NULL, true) ;

  /* Run the dialog */
  gtk_widget_show_all(dialog) ;
  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (response == GTK_RESPONSE_OK)
    {
      const char *search_text = gtk_entry_get_text(GTK_ENTRY(search_entry)) ;
      const char *species_text = gtk_entry_get_text(GTK_ENTRY(species_entry)) ;
      const char *assembly_text = gtk_entry_get_text(GTK_ENTRY(assembly_entry)) ;
      const char *hub_text = gtk_entry_get_text(GTK_ENTRY(hub_entry)) ;
      string err_msg;

      list<TrackDb> results = main_data->registry.search(search_text,
                                                         species_text,
                                                         assembly_text,
                                                         hub_text,
                                                         err_msg) ;
      if (err_msg.empty())
        {
          runListDialog(main_data, 
                        results, 
                        TrackListColumn::ID,
                        GTK_ENTRY(main_data->trackdb_id_widg), 
                        "Select track hub",
                        FALSE) ;
        }
      else
        {
          zMapCritical("Search failed: %s", err_msg.c_str()) ;
        }
    }

  gtk_widget_destroy(dialog) ;
}


/* Callback called when the user hits the delete-trackhub button */
void deleteButtonClickedCB(GtkWidget *button, gpointer data)
{
  UserTrackhubs *user_data = (UserTrackhubs*)data ;
  zMapReturnIfFail(user_data && user_data->main_data_ && user_data->list_widget_) ;

  MainFrame main_data = user_data->main_data_ ;
  GtkTreeView *tree_view = user_data->list_widget_ ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view) ;

  if (selection)
    {
      GtkTreeModel *model = gtk_tree_view_get_model(tree_view) ;
      GList *rows = gtk_tree_selection_get_selected_rows(selection, &model) ;

      stringstream msg_ss ;
      msg_ss << "You are about to delete " << g_list_length(rows) << " trackDbs from the registry.\n\nContinue?" ;

      if (zMapGUIMsgGetBool(NULL, ZMAP_MSG_CRITICAL, msg_ss.str().c_str()))
        {
          for (GList *item = rows; item; item = item->next)
            {
              GtkTreePath *path = (GtkTreePath*)(item->data) ;
              GtkTreeIter iter ;
              gtk_tree_model_get_iter(model, &iter, path) ;

              char *trackdb_id = NULL ;
              gtk_tree_model_get(model, &iter,
                                 TrackListColumn::ID, &trackdb_id,
                                 -1);

              if (trackdb_id)
                {
                  string err_msg;
                  main_data->registry.deleteTrackDb(trackdb_id, err_msg) ;

                  if (!err_msg.empty())
                    zMapCritical("%s", "Error deleting trackDb: %s", err_msg.c_str()) ;

                  g_free(trackdb_id) ;
                }
            }
        }
    }
  else
    {
      zMapCritical("%s", "Please select a track hub to delete") ;
    }
}


/* Show a dialog where the user can browse trackhubs that they have previously registered */
void trackhubBrowse(MainFrame main_data)
{
  bool ok = false ;

  zMapReturnIfFail(main_data && main_data->browse_widg) ;

  // Log in first, if not already logged in
  if (main_data->registry.loggedIn() || runLoginDialog(main_data))
    {
      // If the user is logged in, get the list of trackdbs they have registered, otherwise leave it
      // as an empty list for now
      list<TrackDb> trackdbs_list ;
      string err_msg;

      if (main_data->registry.loggedIn())
        trackdbs_list = main_data->registry.retrieveHub("", err_msg) ;

      if (err_msg.empty())
        {
          // Create a dialog that will show the results in a list
          GtkTreeView *list_widget = NULL ;
          SearchListData search_data ;
          GtkWidget *dialog = createListDialog(main_data, 
                                               trackdbs_list, 
                                               "Select track hub",
                                               false,
                                               search_data,
                                               &list_widget) ;

          GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;


          // Add a button where the user can delete a selected hub
          UserTrackhubs user_data(GTK_WINDOW(dialog), main_data, trackdbs_list, list_widget) ;

          GtkWidget *delete_btn = gtk_button_new_with_label("Delete") ;
          g_signal_connect(G_OBJECT(delete_btn), "clicked", G_CALLBACK(deleteButtonClickedCB), &user_data) ;
          gtk_box_pack_start(content, delete_btn, FALSE, FALSE, 0) ;


          // Run the dialog
          gtk_widget_show_all(dialog) ;
          gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

          if (response == GTK_RESPONSE_OK)
            {
              if (setEntryFromSelection(list_widget, GTK_ENTRY(main_data->trackdb_id_widg), TrackListColumn::ID))
                {
                  ok = true ;
                }
            }

          gtk_widget_destroy(dialog) ;
        }
      else
        {
          zMapCritical("%s", err_msg.c_str()) ;
        }
    }
}


void trackhubBrowseCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  trackhubBrowse(main_data) ;
}


/* Show a dialog where the user can register a trackhubs */
void trackhubRegister(MainFrame main_data)
{
  bool ok = false ;
  zMapReturnIfFail(main_data && main_data->search_widg) ;

  // Log in first, if not already logged in
  if (main_data->registry.loggedIn() || runLoginDialog(main_data))
    {
      GtkWidget *dialog = gtk_dialog_new_with_buttons("Register a Track Hub",
                                                      NULL,
                                                      (GtkDialogFlags)(GTK_DIALOG_DESTROY_WITH_PARENT),
                                                      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                      GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                      NULL);

      gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;

      GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

      GtkTable *table = GTK_TABLE(gtk_table_new(4, 4, FALSE)) ;
      gtk_box_pack_start(content, GTK_WIDGET(table), FALSE, FALSE, 0) ;

      int row = 0 ;
      int col = 0 ;

      GtkWidget *url_entry = makeEntryWidget("URL: ", "", 
                                             "MANDATORY: Enter the URL of the hub",
                                             table, &row, col, col + 2, true, NULL, true) ;
      GtkWidget *assemblies_entry = makeEntryWidget("Assemblies: ", "", 
                                                    "OPTIONAL: If your hub genome subdirectory names are not valid UCSC DB names, then you must provide a map from these names to their corresponding INSDC accessions here as a semi-colon separated list of key=value pairs, e.g. araTha1=GCA_000001735.1 ; ricCom1=GCA_000151685.2",
                                                    table, &row, col, col + 2, false, NULL, true) ;
      GtkWidget *type_entry = makeEntryWidget("Type: ", "", 
                                              "OPTIONAL: Enter the type (one of genomics/epigenomics/transcriptomics/proteomics)",
                                              table, &row, col, col + 2, false, NULL, true) ;

      GtkWidget *public_btn = gtk_check_button_new_with_label("Public") ;
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(public_btn), FALSE) ;
      gtk_widget_set_tooltip_text(public_btn, "Tick this box if the track hub should be visible to other users in Track Hub Registry search results") ;
      gtk_table_attach(table, public_btn, col, col + 1, row, row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;
      ++row ;
  
      gtk_widget_show_all(dialog) ;

      gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

      if (response == GTK_RESPONSE_OK)
        {
          const char *url = gtk_entry_get_text(GTK_ENTRY(url_entry)) ;
          const char *assemblies = gtk_entry_get_text(GTK_ENTRY(assemblies_entry)) ;
          const char *type = gtk_entry_get_text(GTK_ENTRY(type_entry)) ;
          bool is_public = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(public_btn)) ;

          if (url && assemblies)
            {
              map<string, string> assemblies_map ;

              if (assemblies)
                {
                  // Split key-value pairs into map
                  char **tokens = g_strsplit(assemblies, ";", -1) ;
                  for (char *token = *tokens; token; ++token)
                    {
                      char **key_val = g_strsplit(token, "=", -1) ;

                      // We should have 2 and only 2 entries in the key_val list
                      if (key_val && key_val[0] && key_val[1] && !key_val[2])
                        assemblies_map[key_val[0]] = key_val[1] ;
                      else
                        zMapCritical("%s", "Invalid key=value pair in assembly list") ;
                    }
                }

              // Register the hub
              string err_msg;
              main_data->registry.registerHub(url, assemblies_map, type, is_public, err_msg) ;

              if (err_msg.empty())
                ok = true ;
              else
                zMapCritical("Error registering hub: %s", err_msg.c_str()) ;
            }
          else
            {
              zMapCritical("%s", "Please enter the URL and list of assemblies") ;
            }
        }

      gtk_widget_destroy(dialog) ;  
    }

  // If successful, show the registered hub to the user. (For now,
  // just open the registered hubs dialog - should really add a filter
  // to this to only show the recently registered hub.)
  if (ok)
    trackhubBrowse(main_data) ;
}


void trackhubRegisterCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  trackhubRegister(main_data) ;
}

} //unnamed namespace



/*
 *                   External interface routines.
 */



/* Show a dialog to create a new source */
GtkWidget *zMapAppCreateSource(ZMapFeatureSequenceMap sequence_map,
                               ZMapAppCreateSourceCB user_func,
                               gpointer user_data,
                               ZMapAppClosedSequenceViewCB close_func,
                               gpointer close_data,
                               ZMapAppSourceType default_type)
{
  zMapReturnValIfFail(user_func, NULL) ;

  GtkWidget *toplevel = NULL ;
  GtkWidget *container ;
  gpointer seq_data = NULL ;

  toplevel = zMapGUIToplevelNew(NULL, "Create Source") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  MainFrame main_data = NULL ;
  container = makePanel(toplevel, &seq_data, sequence_map, user_func, user_data, close_func, close_data, default_type, &main_data) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  /* Only show widgets for the relevant file type (use given default) */
  if (main_data)
    {
      setWidgetsVisibility(main_data->trackhub_widgets, default_type == ZMapAppSourceType::TRACKHUB) ;
      setWidgetsVisibility(main_data->file_widgets,     default_type == ZMapAppSourceType::FILE) ;
#ifdef USE_ENSEMBL
      setWidgetsVisibility(main_data->ensembl_widgets,  default_type == ZMapAppSourceType::ENSEMBL) ;
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
  container = makePanel(toplevel, &seq_data, sequence_map, user_func, user_data, NULL, NULL, ZMapAppSourceType::NONE, &main_data) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  /* Update the panel with the info from the given source. This toggles visibility of the
   * appropriate widgets so must be done after show_all */
  updatePanelFromSource(main_data, source) ;

  return toplevel ;
}

