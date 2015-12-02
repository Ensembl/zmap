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

#include <zmapAppServices_P.hpp>

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <ZMap/zmapEnsemblUtils.hpp>

#include <gbtools/gbtools.hpp>

using namespace std ;


#define DEFAULT_ENSEMBL_HOST "ensembldb.ensembl.org"
#define DEFAULT_ENSEMBL_PORT "3306"
#define DEFAULT_ENSEMBL_USER "anonymous"
#define DEFAULT_ENSEMBL_PASS NULL
#define xpad 4
#define ypad 4


/* Generic definitions of columns for displaying a list of e.g. databases. For now just include a
 * single column to display the name */
typedef enum
  {
    NAME_COLUMN,       /* shows the value name */

    N_COLUMNS
  } DialogColumns ;



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
static void dbnameCB(GtkWidget *widget, gpointer cb_data) ;
static void featuresetsCB(GtkWidget *widget, gpointer cb_data) ;
static void biotypesCB(GtkWidget *widget, gpointer cb_data) ;
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

  toplevel = zMapGUIToplevelNew(NULL, "Create Source") ;

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
                                  GtkTable *table,
                                  int *row,
                                  const int col,
                                  const int max_col, 
                                  gboolean mandatory)
{
  GtkWidget *label = gtk_label_new(label_str) ;
  gtk_misc_set_alignment(GTK_MISC(label), 1.0, 0.5) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, col, col + 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;

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
                   (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), 
                   xpad, ypad) ;

  *row += 1;

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
  GtkWidget *frame = gtk_frame_new( "New File source: " ) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  const int rows = 5 ;
  const int cols = 6 ;
  GtkTable *table = GTK_TABLE(gtk_table_new(rows, cols, FALSE)) ;
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET(table)) ;

  int row = 0 ;
  int col = 0 ;

  /* Create the label/entry widgets */

  /* First column */
  main_data->name_widg = makeEntryWidget("Source name :", NULL, "REQUIRED: Please enter a name for the new source", 
                                         table, &row, col, col + 2, TRUE) ;
  main_data->dbname_widg = makeEntryWidget("DB name :", NULL, "REQUIRED: The database to load features from", 
                                           table, &row, col, col + 2, TRUE) ;

  /* Add a button next to the dbname widget to allow the user to search for a db in this host */
  GtkWidget *dbname_button = gtk_button_new() ;
  gtk_button_set_image(GTK_BUTTON(dbname_button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_tooltip_text(dbname_button, "Look up database for this host") ;
  gtk_signal_connect(GTK_OBJECT(dbname_button), "clicked", GTK_SIGNAL_FUNC(dbnameCB), main_data) ;
  gtk_table_attach(table, dbname_button, col + 2, col + 3, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;

  main_data->dbprefix_widg = makeEntryWidget("DB prefix :", NULL, "OPTIONAL: If specified, this prefix will be added to source names for features from this database", 
                                             table, &row, col, col + 2, FALSE) ;

  /* Second column */
  int max_row = row ;
  row = 0 ;
  col += 3 ;

  main_data->host_widg = makeEntryWidget("Host :", DEFAULT_ENSEMBL_HOST, NULL, table, &row, col, cols - 1, FALSE) ;
  main_data->port_widg = makeEntryWidget("Port :", DEFAULT_ENSEMBL_PORT, NULL, table, &row, col, cols - 1, FALSE) ;
  main_data->user_widg = makeEntryWidget("Username :", DEFAULT_ENSEMBL_USER, NULL, table, &row, col, cols - 1, FALSE) ;
  main_data->pass_widg = makeEntryWidget("Password :", DEFAULT_ENSEMBL_PASS, "Can be empty if not required", table, &row, col, cols - 1, FALSE) ;

  /* Rows at bottom spanning full width */
  col = 0 ;
  if (max_row > row)
    row = max_row ;

  main_data->featuresets_widg = makeEntryWidget("Featuresets :", NULL, "OPTIONAL: Semi-colon-separated list of featuresets to filter input by", 
                                                table, &row, col, cols - 1, FALSE) ;

  /* Add a button next to the featuresets widget to allow the user to search for featuresets in this database */
  GtkWidget *featuresets_button = gtk_button_new() ;
  gtk_button_set_image(GTK_BUTTON(featuresets_button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_tooltip_text(featuresets_button, "Look up featuresets for this database") ;
  gtk_signal_connect(GTK_OBJECT(featuresets_button), "clicked", GTK_SIGNAL_FUNC(featuresetsCB), main_data) ;
  gtk_table_attach(table, featuresets_button, cols - 1, cols, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;

  main_data->biotypes_widg = makeEntryWidget("Biotypes :", NULL, "OPTIONAL: Semi-colon-separated list of biotypes to filter input by", 
                                             table, &row, col, cols - 1, FALSE) ;

  /* Add a button next to the biotypes widget to allow the user to search for biotypes in this database */
  GtkWidget *biotypes_button = gtk_button_new() ;
  gtk_button_set_image(GTK_BUTTON(biotypes_button), gtk_image_new_from_stock(GTK_STOCK_FIND, GTK_ICON_SIZE_BUTTON));
  gtk_widget_set_tooltip_text(biotypes_button, "Look up biotypes for this database") ;
  gtk_signal_connect(GTK_OBJECT(biotypes_button), "clicked", GTK_SIGNAL_FUNC(biotypesCB), main_data) ;
  gtk_table_attach(table, biotypes_button, cols - 1, cols, row - 1, row, GTK_SHRINK, GTK_SHRINK, xpad, ypad) ;

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
  
  db_list = EnsemblGetDatabaseList(host, port, user, pass, error) ;

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
  
  db_list = EnsemblGetFeaturesetsList(host, port, user, pass, dbname, error) ;

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
  
  db_list = EnsemblGetBiotypesList(host, port, user, pass, dbname, error) ;

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


/* Callback used to determine if a search term matches a row in the tree. Returns false if it
 * matches, true otherwise. */
gboolean search_equal_func_cb(GtkTreeModel *model,
                              gint column,
                              const gchar *key,
                              GtkTreeIter *iter,
                              gpointer user_data)
{
  gboolean result = TRUE ;

  char *column_name = NULL ;

  gtk_tree_model_get(model, iter,
                     NAME_COLUMN, &column_name,
                     -1) ;

  if (column_name && 
      key && 
      strlen(column_name) > 0 && 
      strlen(key) > 0 &&
      my_strcasestr(column_name, key) != NULL)
    {
      result = FALSE ;
    }

  g_free(column_name) ;

  return result ;
}


static GtkTreeView* createListWidget(MainFrame main_data, list<string> *val_list, GtkEntry *search_entry)
{
  GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING) ;

  /* Loop through all of the database names */
  for (list<string>::const_iterator val_iter = val_list->begin(); val_iter != val_list->end(); ++val_iter)
    {
      /* Create a new row in the list store and set the name */
      GtkTreeIter store_iter ;
      gtk_list_store_append(store, &store_iter);
      gtk_list_store_set(store, &store_iter, NAME_COLUMN, val_iter->c_str(), -1);
    }

  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store))) ;

  gtk_tree_view_set_enable_search(tree_view, FALSE);
  gtk_tree_view_set_search_column(tree_view, NAME_COLUMN);
  gtk_tree_view_set_search_entry(tree_view, search_entry) ;
  gtk_tree_view_set_search_equal_func(tree_view, search_equal_func_cb, NULL, NULL) ;

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;
  gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_MULTIPLE);

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Database name", 
                                                                       renderer, 
                                                                       "text", 
                                                                       NAME_COLUMN, 
                                                                       NULL);

  gtk_tree_view_column_set_resizable(column, TRUE) ;
  gtk_tree_view_append_column(tree_view, column);

  return tree_view ;
}


/* Update the given entry with selected values from the given tree */
static gboolean setEntryFromSelection(GtkEntry *entry, GtkTreeView *tree_view)
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

          gtk_tree_model_get(model, &iter, NAME_COLUMN, &value_str, -1) ;

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


/* Create and run a dialog to show the given list of values in a gtk tree view and to set the
 * given entry widget with the result when the user selects a row and hits ok. If the user
 * selects multiple values then it sets a semi-colon-separated list in the entry widget. */
static gboolean runListDialog(MainFrame main_data, list<string> *values_list, GtkEntry *entry_widg)
{
  gboolean ok = FALSE ;
  zMapReturnValIfFail(main_data && values_list, ok) ;

  GtkWidget *dialog = gtk_dialog_new_with_buttons("Select database",
                                                  NULL,
                                                  (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
                                                  GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                                  GTK_STOCK_OK, GTK_RESPONSE_OK,
                                                  NULL);



  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK) ;
  int height = 0 ;

  if (gbtools::GUIGetTrueMonitorSize(dialog, NULL, &height))
    gtk_window_set_default_size(GTK_WINDOW(dialog), -1, height * 0.5) ;

  GtkBox *content = GTK_BOX(GTK_DIALOG(dialog)->vbox) ;

  GtkWidget *hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(content, hbox, FALSE, FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Search:"), FALSE, FALSE, 0) ;
  GtkEntry *search_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(search_entry), FALSE, FALSE, 0) ;

  GtkWidget *scrollwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(content, GTK_WIDGET(scrollwin), TRUE, TRUE, 0) ;

  GtkTreeView *list_widget = createListWidget(main_data, values_list, search_entry) ;
  gtk_container_add(GTK_CONTAINER(scrollwin), GTK_WIDGET(list_widget)) ;

  gtk_widget_show_all(dialog) ;

  gint response = gtk_dialog_run(GTK_DIALOG(dialog)) ;

  if (response == GTK_RESPONSE_OK)
    {
      if (setEntryFromSelection(entry_widg, list_widget))
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
      runListDialog(main_data, db_list, GTK_ENTRY(main_data->dbname_widg)) ;
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
          runListDialog(main_data, featuresets_list, GTK_ENTRY(main_data->featuresets_widg)) ;
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
          runListDialog(main_data, biotypes_list, GTK_ENTRY(main_data->biotypes_widg)) ;
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




