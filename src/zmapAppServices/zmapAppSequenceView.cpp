/*  File: zmapAppSequenceView.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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

#include <string>
#include <map>
#include <string.h>

#include <zmapAppServices_P.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapAppServices.hpp>
#include <ZMap/zmapControl.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapUtilsDebug.hpp>

using namespace std;


#define XPAD 4
#define YPAD 4


/* Columns in the list of sources */
enum class SourceColumn {NAME, TYPE, LOAD, TOTAL} ;

/* Data we need in callbacks. */
typedef struct MainFrameStructName
{
  GtkWidget *toplevel ;

  // Sequence details
  GtkWidget *sequence_widg ;
  GtkWidget *start_widg ;
  GtkWidget *end_widg ;

  // Config file
  GtkWidget *config_widg ;

  // Config file chooser
  GtkWidget *chooser_widg ;

  // 'Recent' button
  GtkWidget *recent_widg ;

  // Info label
  GtkWidget *info_widg ;

  ZMapFeatureSequenceMap orig_sequence_map ;
  gboolean display_sequence ;
  bool import ; // importing into existing view
  bool show_all ; // if true, show all sources; otherwise, filter them

  ZMapAppGetSequenceViewCB user_func ;
  gpointer user_data ;
  ZMapAppClosedSequenceViewCB close_func ;
  gpointer close_data ;

  // A tree view and model for showing the list of sources
  GtkTreeView *sources_tree ;
  GtkTreeModel *sources_model ;
  GtkTreeModel *sources_model_filtered ;

  // Buttons for interacting with the sources list
  GtkEntry *search_entry ;
  //GtkEntry *filter_entry ; // could add filter but needs work to work with child rows
  
  // Is a source window displayed ?
  GtkWidget *source_window ;

} MainFrameStruct, *MainFrame ;



static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *seqdata_out,
                            ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                            ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                            ZMapFeatureSequenceMap sequence_map, 
                            gboolean display_sequence, gboolean import) ;
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;
static GtkWidget *makeButtonBox(MainFrame main_data) ;
static void setSequenceEntries(MainFrame main_data) ;

static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void containerDestroyCB(GtkWidget *widget, gpointer cb_data) ;
static void destroyData(MainFrame main_data) ;

static void applyCB(GtkWidget *widget, gpointer cb_data) ;
#ifndef __CYGWIN__
static void chooseConfigCB(GtkFileChooserButton *widget, gpointer user_data) ;
#endif
static void defaultsCB(GtkWidget *widget, gpointer cb_data) ;
static void createSourceCB(GtkWidget *widget, gpointer cb_data) ;
static void removeSourceCB(GtkWidget *widget, gpointer cb_data) ;
static void editSourceButtonCB(GtkWidget *widget, gpointer cb_data) ;
static void clearRecentCB(GtkWidget *button, gpointer data) ;
static void saveCB(GtkWidget *widget, gpointer cb_data) ;
static void closeCB(GtkWidget *widget, gpointer cb_data) ;
static void saveSourcesToConfig(ZMapFeatureSequenceMap sequence_map,
                                char **filepath_inout, 
                                GError **error) ;
static void configChangedCB(GtkWidget *widget, gpointer user_data) ;

static list<ZMapConfigSource> getSelectedSources(GtkTreeView *sources_tree,
                                                 ZMapFeatureSequenceMap sequence_map,
                                                 const char *err_msg) ;
static list<GQuark> getSelectedSourceNames(GtkTreeView *sources_tree,
                                           const char *err_msg) ;
static void selectSource(const char *source_name, 
                         GtkTreeView *tree,
                         GtkTreeModel *model,
                         const bool deselect_others = false) ;

static void createNewSourceCB(const char *source_name, const std::string &url, 
                              const char *featuresets, const char *biotypes, 
                              const string &file_type, const int num_fields,
                              gpointer user_data, GError **error) ;
static void cancelNewSourceCB(GtkWidget *toplevel, gpointer cb_data) ;

static void updateInfoLabel(MainFrame main_data, ZMapFeatureSequenceMap sequence_map) ;

static gboolean tree_view_search_equal_func_cb(GtkTreeModel *model, gint column, const gchar *key,
                                               GtkTreeIter *iter, gpointer user_data) ;

//static void filter_entry_activate_cb(GtkEntry *entry, gpointer user_data) ;
//static void clear_button_cb(GtkButton *button, gpointer user_data) ;
static gboolean columnMatchesText(GtkTreeModel *model, GtkTreeIter *iter, SourceColumn col_id, const char *text) ;
static void select_invert_button_cb(GtkButton *button, gpointer user_data) ;
static void select_none_button_cb(GtkButton *button, gpointer user_data) ;
static void select_all_button_cb(GtkButton *button, gpointer user_data) ;
static void set_load_cb(GtkButton *button, gpointer user_data) ;
static void set_unload_cb(GtkButton *button, gpointer user_data) ;
//static gboolean tree_model_filter_visible_cb(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) ;


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

  toplevel = zMapGUIToplevelNew(NULL, "Choose source(s) and sequence to load") ;

  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 0) ;

  container = makePanel(toplevel, &seq_data,
                        user_func, user_data, close_func, close_data,
                        sequence_map, display_sequence, true) ;

  gtk_container_add(GTK_CONTAINER(toplevel), container) ;

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
                     GTK_SIGNAL_FUNC(toplevelDestroyCB), seq_data) ;

  gtk_widget_show_all(toplevel) ;

  return toplevel ;
}


/* As for zMapAppGetSequenceView() except that returns a GtkWidget that can be
 * incorporated into a window. 
 * If edit_sequence the user will be allowed to enter sequence details (e.g. in order
 * to create a new view). Otherwise they will not be able to enter new sequene details
 * and the existing sequence map will be used (e.g. for importing data into an existing view).
 */
GtkWidget *zMapCreateSequenceViewWidg(ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                                      ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                                      ZMapFeatureSequenceMap sequence_map, 
                                      gboolean display_sequence, gboolean import,
                                      GtkWidget *toplevel)
{
  GtkWidget *container = NULL ;
  MainFrame main_data = NULL ;

  container = makePanel(toplevel, (void **)&main_data,
                        user_func, user_data, close_func, close_data,
                        sequence_map, display_sequence, import) ;

  gtk_signal_connect(GTK_OBJECT(container), "destroy",
                     GTK_SIGNAL_FUNC(containerDestroyCB), main_data) ;


  return container ;
}




/*
 *                   Internal routines.
 */


/* Make the whole panel returning the top container of the panel. */
static GtkWidget *makePanel(GtkWidget *toplevel, gpointer *our_data,
                            ZMapAppGetSequenceViewCB user_func, gpointer user_data,
                            ZMapAppClosedSequenceViewCB close_func, gpointer close_data,
                            ZMapFeatureSequenceMap sequence_map, 
                            gboolean display_sequence, gboolean import)
{
  GtkWidget *frame = NULL ;
  GtkWidget *vbox, *main_frame, *button_box ;
  MainFrame main_data ;

  main_data = g_new0(MainFrameStruct, 1) ;

  if (our_data)
    *our_data = main_data ;

  main_data->user_func = user_func ;
  main_data->user_data = user_data ;
  main_data->close_func = close_func ;
  main_data->close_data = close_data ;
  main_data->orig_sequence_map = sequence_map ;
  main_data->display_sequence = display_sequence ;
  main_data->import = import ;
  main_data->show_all = !import ;                           // only show recent sources in import dialog
  main_data->toplevel = toplevel ;                          // n.b. toplevel can legitimately be NULL

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  main_frame = makeMainFrame(main_data, sequence_map) ;
  gtk_box_pack_start(GTK_BOX(vbox), main_frame, TRUE, TRUE, 0) ;

  button_box = makeButtonBox(main_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0) ;

  return frame ;
}


static void createTreeViewColumn(GtkTreeView *tree_view, 
                                 const char *title,
                                 GtkCellRenderer *renderer,
                                 const char *type,
                                 SourceColumn col_id)
{
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(title, 
                                                                       renderer, 
                                                                       type, 
                                                                       col_id,
                                                                       NULL);

  gtk_tree_view_column_set_resizable(column, TRUE) ;
  gtk_tree_view_column_set_reorderable(column, TRUE) ;
  gtk_tree_view_column_set_min_width(column, 30) ;
  gtk_tree_view_column_set_max_width(column, 800) ;

  gtk_tree_view_column_set_sort_column_id(column, (int)SourceColumn::NAME) ;

  gtk_tree_view_append_column(tree_view, column);

  return ;
}


/* Called by updateSourcesList to add a single source to the store and to recursively add any
 * child sources, if it has any */
static void updateSourcesListAddSource(MainFrame main_data, 
                                       GtkTreeStore *store, 
                                       ZMapConfigSource source,
                                       GtkTreeIter *parent_tree_iter)
{
  /* Only show recent sources, if applicable */
  if (source && (main_data->show_all || source->recent))
    {
      string source_type = source->type() ;

      /* Create a new row in the list store and set the values */
      GtkTreeIter tree_iter ;
      gtk_tree_store_append(store, &tree_iter, parent_tree_iter);

      gtk_tree_store_set(store, &tree_iter, 
                         SourceColumn::NAME, g_quark_to_string(source->name_),
                         SourceColumn::TYPE, source_type.c_str(),
                         SourceColumn::LOAD, !source->delayed,
                         -1);

      /* Also add child sources, if any */
      for (ZMapConfigSource child_source : source->children)
        {
          // HACKED FOR NOW...
          if (child_source)
            updateSourcesListAddSource(main_data, store, child_source, &tree_iter) ;
        }
    }

  return ;
}


/* Update the list of sources on the dialog following a change to the sources in the sequence map */
static void updateSourcesList(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  zMapReturnIfFail(main_data && sequence_map) ;

  if (main_data && main_data->sources_model && sequence_map && sequence_map->sources)
    {
      GtkTreeStore *store = GTK_TREE_STORE(main_data->sources_model) ;

      gtk_tree_store_clear(store) ;

      /* Loop through all of the sources */
      for (auto source_iter : *sequence_map->sources)
        {
          updateSourcesListAddSource(main_data, store, source_iter.second, NULL) ;
        }
    }  

  return ;
}


/* Set the 'load' flag for a particular row in the tree. Also sets the delayed flag in the
 * relevant source. Recurses through child paths, and sibling paths if do_siblings is true. */
static void treeIterSetLoadStatus(GtkTreeModel *model,
                                  GtkTreeIter *iter, 
                                  const gboolean load,
                                  const gboolean do_siblings,
                                  MainFrame main_data,
                                  ZMapFeatureSequenceMap sequence_map)
{
  zMapReturnIfFail(iter) ;

  // Update the load status for this tree row
  gtk_tree_store_set(GTK_TREE_STORE(model), iter, SourceColumn::LOAD, load, -1);

  // Find the source and update the delayed flag
  char *source_name = NULL ;
  gtk_tree_model_get(model, iter, SourceColumn::NAME, &source_name, -1) ;

  if (source_name)
    {
      ZMapConfigSource source = sequence_map->getSource(source_name) ;

      if (source)
        source->delayed = !load ;
    }

  // Recurse through child paths, and siblings if applicable
  GtkTreeIter child_iter ;
  if (gtk_tree_model_iter_children(model, &child_iter, iter))
    {
      // Pass do_siblings=TRUE to do all of this child's siblings
      treeIterSetLoadStatus(model, &child_iter, load, TRUE, main_data, sequence_map) ; 
    }

  if (do_siblings && gtk_tree_model_iter_next(model, iter))
    {
      treeIterSetLoadStatus(model, iter, load, do_siblings, main_data, sequence_map) ;
    }

  return ;
}


/* Called when the user toggles the 'load' button for a source. It finds the source at the given
 * path and toggles its 'delayed' flag to be opposite the value of the 'load' value. */
static void loadButtonToggledCB(GtkCellRendererToggle *toggle_renderer,
                                gchar *path_str,
                                gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;

  GtkTreePath *path = gtk_tree_path_new_from_string(path_str) ;
  GtkTreeIter iter ;
  
  // Get the tree row iter for the row that was clicked
  if (path && gtk_tree_model_get_iter(main_data->sources_model, &iter, path))
    {
      // Get the load status from this tree row
      gboolean cur_value = FALSE ;
      gtk_tree_model_get(main_data->sources_model, &iter, SourceColumn::LOAD, &cur_value, -1) ;

      // The new value will be the inverse of the current value
      gboolean load = !cur_value ;

      // Set the load status in the tree row for this row and any child rows
      GtkTreeIter iter ;
      if (gtk_tree_model_get_iter(main_data->sources_model, &iter, path))
        treeIterSetLoadStatus(main_data->sources_model, &iter, load, FALSE, main_data, main_data->orig_sequence_map) ;

      // Finally, update the info label, which displays info about which sources will be loaded
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
    }

  if (path)
    gtk_tree_path_free(path) ;
}


static GtkWidget* createListWidgetButtons(MainFrame main_data)
{
  zMapReturnValIfFail(main_data, NULL) ;

  GtkWidget *vbox = gtk_vbox_new(FALSE, XPAD) ;

  GtkWidget *hbox = gtk_hbox_new(FALSE, XPAD) ;


  /* 
   * Add buttons to control the selection 
   */
  
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Select:"), FALSE, FALSE, 0) ;

  /* Add a button to select all the visible rows */
  GtkWidget *button = gtk_button_new_with_mnemonic("Al_l") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_all_button_cb), main_data) ;
  gtk_widget_set_tooltip_text(button, "Select all visible sources") ;

  /* Add a button to unselect all visible rows */
  button = gtk_button_new_with_mnemonic("_None") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_none_button_cb), main_data) ;
  gtk_widget_set_tooltip_text(button, "Deselect all visible sources") ;

  /* Add a button to invert the current selection */
  button = gtk_button_new_with_mnemonic("_Invert") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_invert_button_cb), main_data) ;
  gtk_widget_set_tooltip_text(button, "Invert the current selection") ;


  /*
   * Add a section for actions that can be performed on the selection 
   */
  
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(" "), FALSE, FALSE, 0) ; //separator
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Action:"), FALSE, FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(" "), FALSE, FALSE, 0) ; //separator

  /* Add a button to tick mark the selected rows for loading */
  button = gtk_button_new_with_mnemonic("_Load") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(set_load_cb), main_data) ;
  gtk_widget_set_tooltip_text(button, "Mark the selected rows for loading") ;

  /* Add a button to tick mark the selected rows for loading */
  button = gtk_button_new_with_mnemonic("_Unload") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(set_unload_cb), main_data) ;
  gtk_widget_set_tooltip_text(button, "Unmark the selected rows for loading") ;

  
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(" "), FALSE, FALSE, 0) ; //separator

  /* 
   * Add search/filter boxes
   */
  
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Search:"), FALSE, FALSE, 0) ;
  main_data->search_entry = GTK_ENTRY(gtk_entry_new()) ;
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(main_data->search_entry), FALSE, FALSE, 0) ;
  gtk_widget_set_tooltip_text(GTK_WIDGET(main_data->search_entry), "Search for sources containing this text. Press the up/down arrows to progress to next/previous match.") ;

  //gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Filter:"), FALSE, FALSE, 0) ;
  //main_data->filter_entry = GTK_ENTRY(gtk_entry_new()) ;
  //gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(main_data->filter_entry), FALSE, FALSE, 0) ;
  //g_signal_connect(G_OBJECT(main_data->filter_entry), "activate", G_CALLBACK(filter_entry_activate_cb), main_data) ;
  //gtk_widget_set_tooltip_text(GTK_WIDGET(main_data->filter_entry), "Show only sources containing this text. Press enter to apply the filter.") ;

  /* Add a button to clear the search/filter boxes and sort order */
  //button = gtk_button_new_with_mnemonic("C_lear") ;
  //gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  //g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clear_button_cb), main_data) ;
  //gtk_widget_set_tooltip_text(button, "Clear the current search/filter") ;


  
  return vbox ;
}


/* Create the list widget to show all of the existing source names */
static GtkWidget* createListWidget(ZMapFeatureSequenceMap sequence_map, MainFrame main_data)
{
  GtkWidget *vbox = gtk_vbox_new(FALSE, XPAD) ;

  // Create some buttons for interacting with the tree (must do this to set up search_entry
  // etc. before using these in the tree view)
  GtkWidget *list_buttons = createListWidgetButtons(main_data) ;

  
  GtkTreeStore *store = gtk_tree_store_new((gint)(SourceColumn::TOTAL),
                                           G_TYPE_STRING, 
                                           G_TYPE_STRING,
                                           G_TYPE_BOOLEAN) ;

  GtkTreeView *tree_view = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(store))) ;

  if (main_data)
    {
      main_data->sources_tree = tree_view ;
      main_data->sources_model = GTK_TREE_MODEL(store) ;

      if (main_data->search_entry)
        {
          gtk_tree_view_set_enable_search(tree_view, FALSE);
          gtk_tree_view_set_search_column(tree_view, (int)SourceColumn::NAME);
          gtk_tree_view_set_search_entry(tree_view, main_data->search_entry) ;
          gtk_tree_view_set_search_equal_func(tree_view, tree_view_search_equal_func_cb, main_data, NULL) ;
        }

      //if (main_data->filter_entry)
      //  {
      //    GtkTreeModel *filtered = gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL) ;
      //
      //    gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
      //                                           tree_model_filter_visible_cb, 
      //                                           main_data,
      //                                           NULL);
      //    main_data->sources_model_filtered = filtered ;
      //  }
    }

  /* Update the values in the store with the sources from the sequence map */
  updateSourcesList(main_data, sequence_map) ;

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;
  gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_MULTIPLE);

  GtkCellRenderer *toggle_renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_activatable(GTK_CELL_RENDERER_TOGGLE(toggle_renderer), TRUE) ;
  g_signal_connect(G_OBJECT(toggle_renderer), "toggled", G_CALLBACK(loadButtonToggledCB), main_data) ;
  createTreeViewColumn(tree_view, "Load", toggle_renderer, "active", SourceColumn::LOAD) ;

  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
  createTreeViewColumn(tree_view, "Name", text_renderer,   "text",   SourceColumn::NAME) ;
  createTreeViewColumn(tree_view, "Type", text_renderer,   "text",   SourceColumn::TYPE) ;

  
  // Put the tree in a scrolled window below the buttons
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(tree_view)) ;

  // Add the scrolled window and buttons to the main container
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), list_buttons, FALSE, FALSE, 0) ;

  return vbox ;
}


static void onUpdateList(GtkWidget *button, gpointer data)
{
  MainFrame main_data = (MainFrame)data ;

  if (main_data)
    {
      main_data->show_all = !main_data->show_all ;
      updateSourcesList(main_data, main_data->orig_sequence_map) ;
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
    }
}


/* Utility to create a button with various properties */
static GtkWidget *createButton(const char *label, 
                               const char *stock,
                               const bool icon_only,
                               const char *tooltip,
                               GCallback cb_func,
                               gpointer cb_data,
                               GtkBox *container)
{
  GtkWidget *button = NULL ;

  if (label)
    {
      button = gtk_button_new_with_label(label) ;
      
      if (stock)
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON));
    }
  else if (icon_only)
    {
      button = gtk_button_new() ;
      gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(stock, GTK_ICON_SIZE_BUTTON));
    }
  else
    {
      button = gtk_button_new_from_stock(stock) ;
    }

  gtk_widget_set_tooltip_text(button, tooltip) ;
  g_signal_connect(G_OBJECT(button), "clicked", cb_func, cb_data) ;
  gtk_box_pack_start(container, button, FALSE, FALSE, 0) ;

  return button ;
}


/* Update the info label with info about how many sources are selected */
static void updateInfoLabel(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  zMapReturnIfFail(main_data && main_data->info_widg && sequence_map) ;

  unsigned int num_total = 0 ;     // total number of sources
  unsigned int num_with_data = 0 ; // number of sources with data
  unsigned int num_to_load = 0 ;   // number of sources with data that are selected for loading

  sequence_map->countSources(num_total, num_with_data, num_to_load, !main_data->show_all) ;

  stringstream text_ss ;

  if (num_total == 0)
    text_ss << "No sources" ;
  else if (num_with_data == 0)
    text_ss << "No sources have data" ;
  else if (num_to_load < 1)
    text_ss << "No sources will be loaded out of "              << num_with_data << " with data (" << num_total << " total)" ;
  else if (num_to_load == 1)
    text_ss << num_to_load << " source will be loaded out of "  << num_with_data << " with data (" << num_total << " total)" ;
  else
    text_ss << num_to_load << " sources will be loaded out of " << num_with_data << " with data (" << num_total << " total)";

  gtk_label_set_text(GTK_LABEL(main_data->info_widg), text_ss.str().c_str()) ;
}


/* Create a frame listing all of the available sources */
static GtkWidget *makeSourcesFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame = gtk_frame_new( "Sources: " );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  GtkWidget *topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  GtkWidget *list_widget = createListWidget(sequence_map, main_data) ;
  gtk_box_pack_start(GTK_BOX(topbox), list_widget, TRUE, TRUE, 0) ;
  
  // Create a label to show how many sources will be loaded
  main_data->info_widg = gtk_label_new("") ;
  gtk_box_pack_start(GTK_BOX(topbox), main_data->info_widg, FALSE, FALSE, 0) ;
  updateInfoLabel(main_data, sequence_map) ;

  return frame ;
}


static void makeLabel(const char *text, GtkWidget *parent)
{
  GtkWidget *label = gtk_label_new(text) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(parent), label, FALSE, FALSE, 0) ;

  return ;
}


static GtkWidget *makeEntry(GtkWidget *parent, const gboolean fill)
{
  GtkWidget *entry = gtk_entry_new() ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(parent), entry, FALSE, fill, 0) ;
  gtk_entry_set_activates_default(GTK_ENTRY(entry), TRUE) ;
  return entry ;
}


/* Create a frame listing the sequence name, start and end, so that the user can enter/modify
 * them */
static GtkWidget *makeSequenceFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox ;

  frame = gtk_frame_new( "Sequence: " );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 0) ;

  /* Box for labels */
  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  /* Box for entries */
  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, FALSE, FALSE, 0) ;

  /* Create the widgets */
  makeLabel("Sequence :", labelbox) ;
  makeLabel( "Start :", labelbox) ;
  makeLabel("End :", labelbox) ;
  makeLabel( "Config File :", labelbox) ;

  main_data->sequence_widg = makeEntry(entrybox, FALSE) ;
  main_data->start_widg = makeEntry(entrybox, FALSE) ;
  main_data->end_widg = makeEntry(entrybox, FALSE) ;
  main_data->config_widg = makeEntry(entrybox, FALSE) ;

  g_signal_connect(G_OBJECT(main_data->config_widg), "changed", G_CALLBACK(configChangedCB), main_data) ;

  /* Set the entry fields if required. */
  if (sequence_map && main_data->display_sequence)
    setSequenceEntries(main_data) ;

  return frame ;
}


/* Make the label/entry fields frame. */
static GtkWidget *makeMainFrame(MainFrame main_data, ZMapFeatureSequenceMap sequence_map)
{
  GtkWidget *frame = gtk_hbox_new(FALSE, 0) ;
  GtkBox *box = GTK_BOX(frame) ;

  GtkWidget *sources_frame = makeSourcesFrame(main_data, sequence_map) ;
  GtkWidget *sequence_frame = makeSequenceFrame(main_data, sequence_map) ;

  gtk_box_pack_start(box, sources_frame, TRUE, TRUE, 0) ;
  gtk_box_pack_start(box, sequence_frame, FALSE, FALSE, 0) ;

  return frame ;
}




/* Make the action buttons frame. */
static GtkWidget *makeButtonBox(MainFrame main_data)
{
  GtkBox *button_box ;
  GtkWidget *ok_button, *chooser_button;
  char *home_dir ;

  //
  // Container
  //
  button_box = GTK_BOX(gtk_hbutton_box_new()) ;
  gtk_container_border_width(GTK_CONTAINER(button_box), 5) ;

  //
  // Butotns to add/edit/delete/clear sources
  //
  createButton(NULL, GTK_STOCK_ADD, true, 
               "Create a new source", 
               G_CALLBACK(createSourceCB), (gpointer)main_data, button_box) ;
  
  createButton(NULL, GTK_STOCK_EDIT, true,
               "Edit the selected source(s)",
               G_CALLBACK(editSourceButtonCB), (gpointer)main_data, button_box) ;

  createButton(NULL, GTK_STOCK_DELETE, true, 
               "Delete the selected source(s).",
               G_CALLBACK(removeSourceCB), (gpointer)main_data, button_box) ;

  createButton(NULL, GTK_STOCK_CLEAR, true,
               "Clear the recent sources list. This will cause all of the current sources to be hidden when the 'Recent' box is ticked.",
               G_CALLBACK(clearRecentCB), (gpointer)main_data, button_box) ;

  /* Add a check button to filter by recent sources, and a button to clear the recent list */
  main_data->recent_widg = gtk_check_button_new_with_label("Recent") ;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(main_data->recent_widg), !main_data->show_all) ;
  gtk_widget_set_tooltip_text(main_data->recent_widg, "Tick this to filter the list by recent sources; un-tick to show all sources ") ;
  gtk_box_pack_start(button_box, main_data->recent_widg, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(main_data->recent_widg), "clicked", G_CALLBACK(onUpdateList), main_data) ;


  //
  // Save and load buttons
  //
  createButton(NULL, GTK_STOCK_SAVE, true, 
               "Save the source and sequence details to a configuration file",
               G_CALLBACK(saveCB), (gpointer)main_data, button_box) ;

#ifndef __CYGWIN__
  /* N.B. we use the gtk "built-in" file chooser stuff. */
  main_data->chooser_widg = chooser_button = gtk_file_chooser_button_new("Choose A Config File",
                                                                         GTK_FILE_CHOOSER_ACTION_OPEN) ;
  gtk_signal_connect(GTK_OBJECT(chooser_button), "file-set",
                     GTK_SIGNAL_FUNC(chooseConfigCB), (gpointer)main_data) ;
  gtk_box_pack_start(button_box, chooser_button, FALSE, FALSE, 0) ;
  home_dir = (char *)g_get_home_dir() ;
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser_button), home_dir) ;
  gtk_file_chooser_set_show_hidden(GTK_FILE_CHOOSER(chooser_button), TRUE) ;
#else
  main_data->chooser_widg = chooser_button = NULL ;
#endif
  gtk_widget_set_tooltip_text(chooser_button, "Load source/sequence details from a configuration file") ;

  /* If a sequence is provided then make a button to set it in the entries fields,
   * saves the user tedious typing. */
  if (main_data->orig_sequence_map->sequence && !main_data->import)
    {
      createButton("Set Defaults", NULL, false,
                   "Set sequence details from default values",
                   G_CALLBACK(defaultsCB), (gpointer)main_data, button_box) ;
    }

  //
  // Close and OK buttons
  //
  /* Only add a close button and set a default button if this is a standalone dialog. */
  if (main_data->toplevel)
    {
      createButton("Close", NULL, false, NULL, 
                   G_CALLBACK(closeCB), (gpointer)main_data, button_box) ;
    }

  ok_button = createButton(NULL, GTK_STOCK_OK, false,
                           "Load the specified region for the selected source(s)",
                           G_CALLBACK(applyCB), (gpointer)main_data, button_box) ;

  //
  // Set the default button
  //
  GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT) ;

  if (GTK_IS_WINDOW(main_data->toplevel))
    gtk_window_set_default(GTK_WINDOW(main_data->toplevel), ok_button) ;
  

  return GTK_WIDGET(button_box) ;
}


// For zMapAppGetSequenceView(), this function gets called whenever there is a
// gtk_widget_destroy() to the top level widget. Sometimes this is because of window manager
// action, sometimes one of our exit routines does a gtk_widget_destroy() on the top level widget.
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  destroyData(main_data) ;

  return ;
}

// For zMapAppGetSequenceViewWidg(), this function gets called whenever a parent widget of us is
// being destroyed by a gtk_widget_destroy(). Sometimes this is because of window manager action,
// sometimes one of our exit routines does a gtk_widget_destroy() on the top level widget.
static void containerDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  destroyData(main_data) ;

  return ;
}


// Common cleanup routine.
static void destroyData(MainFrame main_data)
{
  // Signal app we are going
  if (main_data->close_func)
    (main_data->close_func)(main_data->toplevel, main_data->close_data) ;

  // If there's a source window showing then destroy it.
  if (main_data->source_window)
    {
      gtk_widget_destroy(main_data->source_window) ;
      main_data->source_window = NULL ;
    }

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
  if (main_data->orig_sequence_map->config_file)
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(main_data->chooser_widg), main_data->orig_sequence_map->config_file) ;
#endif

  return ;
}


/* Save the source and sequence details to a configuration file */
static void saveCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  /* Cache the filepath so we can offer the same file by default the next time */
  static char *filepath = NULL ;

  GError *error = NULL ;

  if (!filepath && main_data->orig_sequence_map && main_data->orig_sequence_map->config_file)
    filepath = g_strdup(main_data->orig_sequence_map->config_file) ;

  saveSourcesToConfig(main_data->orig_sequence_map, &filepath, &error) ;

  if (error)
    {
      zMapCritical("Error saving file: %s", error->message) ;
      g_error_free(error) ;
      error = NULL ;
    }

  return ;
}


/* Callback called when the create-source dialog has been ok'd to do the work to create the new
 * source from the user-provided info. Returns true if successfully created the source.  */
static void createNewSourceCB(const char *source_name, const std::string &url, 
                              const char *featuresets, const char *biotypes, 
                              const string &file_type, const int num_fields,
                              gpointer user_data, GError **error)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(main_data && main_data->orig_sequence_map) ;

  ZMapFeatureSequenceMap sequence_map = main_data->orig_sequence_map ;
  GError *tmp_error = NULL ;

  // source window will have gone away.
  main_data->source_window = NULL ;

  sequence_map->createSource(source_name, url, featuresets, biotypes, file_type, num_fields, false, true, &tmp_error) ;

  /* Update the list of sources shown in the dialog to include the new source,
   * and add the new source to the current selection */
  if (!tmp_error)
    {
      updateSourcesList(main_data, sequence_map) ;
      selectSource(source_name, main_data->sources_tree, main_data->sources_model) ;
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return ;
}


static void cancelNewSourceCB(GtkWidget *toplevel, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  // source window will have gone away.
  main_data->source_window = NULL ;

  return ;
}


/* Callback called when the create-source dialog has been ok'd to do the work to create the new
 * source from the user-provided info. Returns true if successfully created the source.  */
static void editSourceCB(const char *source_name, 
                         const std::string &url, 
                         const char *featuresets,
                         const char *biotypes,
                         const string &file_type, 
                         const int num_fields,
                         gpointer user_data,
                         GError **error)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(source_name && main_data && main_data->orig_sequence_map) ;

  ZMapFeatureSequenceMap sequence_map = main_data->orig_sequence_map ;
  GError *tmp_error = NULL ;

  sequence_map->updateSource(source_name, url, featuresets, biotypes, file_type, num_fields, &tmp_error) ;

  /* Update the list of sources shown in the dialog to include the new source */
  if (!tmp_error)
    {
      updateSourcesList(main_data, sequence_map) ;
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;
}


/* Set up a new source. */
static void createSourceCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data) ;

  // Only one source window at a time.
  if (!(main_data->source_window))
    {
      /* When importing via the import dialog we probably(?) want the default import type to FILE. */
      if (main_data->import)
        main_data->source_window = zMapAppCreateSource(main_data->orig_sequence_map,
                                                       createNewSourceCB, main_data, cancelNewSourceCB, main_data,
                                                       ZMapAppSourceType::FILE) ;
      else
        main_data->source_window = zMapAppCreateSource(main_data->orig_sequence_map,
                                                       createNewSourceCB, main_data, cancelNewSourceCB, main_data) ;
    }

  return ;
}


/* Edit an existing source. */
static void editSourceButtonCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data) ;

  const char *err_msg = NULL;

  list<ZMapConfigSource> sources = getSelectedSources(main_data->sources_tree, 
                                                      main_data->orig_sequence_map,
                                                      err_msg) ;

  if (sources.size() == 1)
    zMapAppEditSource(main_data->orig_sequence_map, sources.front(), editSourceCB, main_data) ;
  else if (sources.size() > 1)
    zMapCritical("%s", "Please only select one source") ;
  else if (err_msg)
    zMapCritical("%s", err_msg) ;
  else
    zMapCritical("%s", "Program error: failed to get sources") ;
    
  return ;
}


/* Remove the selected source(s). */
static void removeSourceCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;
  zMapReturnIfFail(main_data && main_data->sources_tree && main_data->orig_sequence_map) ;

  const char *err_msg = NULL;

  list<GQuark> source_names = getSelectedSourceNames(main_data->sources_tree, 
                                                     err_msg) ;
  
  if (source_names.size() > 0)
    {
      for (GQuark source_name : source_names)
        {
          GError *error = NULL ;
          main_data->orig_sequence_map->removeSource(g_quark_to_string(source_name), &error) ;

          if (error)
            {
              zMapWarning("Error removing source '%s': %s", g_quark_to_string(source_name), error->message) ;
              g_error_free(error) ;
              error = NULL ;
            }
        }

      /* Update the list widget */
      updateSourcesList(main_data, main_data->orig_sequence_map) ;
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
      main_data->orig_sequence_map->setFlag(ZMAPFLAG_SAVE_SOURCES, TRUE) ;
    }
  else if (err_msg)
    {
      zMapCritical("%s", err_msg) ;
    }
  else
    {
      zMapCritical("%s", "Program error: failed to get sources") ;
    }

  return ;
}

/* Callback when the user hits the 'clear' button to clear all recent sources */
static void clearRecentCB(GtkWidget *button, gpointer data)
{
  MainFrame main_data = (MainFrame)data ;

  if (main_data)
    {
      for (auto iter : *main_data->orig_sequence_map->sources)
        iter.second->recent = false ;

      updateSourcesList(main_data, main_data->orig_sequence_map) ;
      updateInfoLabel(main_data, main_data->orig_sequence_map) ;
    }

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

  if (main_data->orig_sequence_map->sequence)
    sequence = main_data->orig_sequence_map->sequence ;
  if (main_data->orig_sequence_map->start)
    start = g_strdup_printf("%d", main_data->orig_sequence_map->start) ;
  if (main_data->orig_sequence_map->end)
    end = g_strdup_printf("%d", main_data->orig_sequence_map->end) ;
  if (main_data->orig_sequence_map->config_file)
    config_file = main_data->orig_sequence_map->config_file ;

  gtk_entry_set_text(GTK_ENTRY(main_data->sequence_widg), sequence) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->start_widg), (start ? start : "")) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->end_widg), (end ? end : "")) ;
  gtk_entry_set_text(GTK_ENTRY(main_data->config_widg), config_file) ;

  if (start)
    g_free(start) ;
  if (end)
    g_free(end) ;

  /* Don't allow editing of the sequence if we're importing into an existing view */
  if (main_data->import)
    {
      gtk_widget_set_sensitive(main_data->sequence_widg, FALSE) ;
      gtk_widget_set_sensitive(main_data->start_widg, FALSE) ;
      gtk_widget_set_sensitive(main_data->end_widg, FALSE) ;
      gtk_widget_set_sensitive(main_data->config_widg, FALSE) ;
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
 * The user must have selected which source(s) from the list they want to load.
 *
 * If the user has specified a new sequence, we create a new sequence map for it
 * (e.g. if the user is creating a new view).
 * If importing into an existing sequence, the user callback on the original sequence 
 * map.
 *
 *  */
static void applyCB(GtkWidget *widget, gpointer cb_data)
{
  MainFrame main_data = (MainFrame)cb_data ;

  const bool recent_only = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(main_data->recent_widg)) ;

  // Warn the user if they are about to load a lot of data
  unsigned int warning_limit = 40 ;
  
  unsigned int num_total = 0 ;     // total number of sources
  unsigned int num_with_data = 0 ; // number of sources with data
  unsigned int num_to_load = 0 ;   // number of sources with data that are selected for loading

  main_data->orig_sequence_map->countSources(num_total, num_with_data, num_to_load, !main_data->show_all) ;
  bool ok = true ;
  
  if (num_to_load > warning_limit)
    {
      stringstream ss ;
      ss << "You are about to load " << num_to_load << " sources.\n\nYou will not be able to cancel loading once it starts.\n\nAre you sure you want to continue?" ;
      ok = zMapGUIMsgGetBool(NULL, ZMAP_MSG_WARNING, ss.str().c_str()) ;
    }

  if (ok && main_data->import)
    {
      /* Just call user callback with original sequence map. */
      (main_data->user_func)(main_data->orig_sequence_map, recent_only, main_data->user_data) ;
    }
  else if (ok)
    {
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
      else if (!(*sequence) && !(*start_txt) && !(*end_txt))
        {
          status = FALSE ;
          err_msg = "Please enter the sequence name and coordinates" ;
        }
      else if (!(*sequence))
        {
          status = FALSE ;
          err_msg = "Please enter the sequence name" ;
        }
      else if (!(*start_txt) || !(*end_txt))
        {
          status = FALSE ;
          err_msg = "Please enter the start and end coordinates" ;
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
          err_msg = "Please enter the sequence name and coordinates,\n"
            "OR a config file containing these details (but not both)." ;
        }


      if (!status)
        {
          zMapWarning("%s", err_msg) ;
        }
      else
        {
          /* when we get here we should either have only a config file specified or
           * a sequence/start/end and optionally a config file. */
          ZMapFeatureSequenceMap seq_map ;
          gboolean sequence_ok = FALSE ;
          const char *err_msg = NULL ;

          seq_map = new ZMapFeatureSequenceMapStruct ;

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
              delete seq_map ;
              ok = false ;
            }
          else
            {
              /* Call back with users parameters for new sequence display. */
              (main_data->user_func)(seq_map, recent_only, main_data->user_data) ;
            }
        }

      if (tmp_error)
        g_error_free(tmp_error) ;
    }

  if (ok)
    gtk_widget_destroy(main_data->toplevel) ;
  
  return ;
}


static void saveSourcesToConfig(ZMapFeatureSequenceMap sequence_map,
                                char **filepath_inout, 
                                GError **error)
{
  GError *tmp_error = NULL ;
  char *output_file = NULL ;
  char *input_file = NULL ;
  ZMapConfigIniFileType file_type ;
  ZMapConfigIniContext context = NULL ;

  input_file = NULL ;
  file_type = ZMAPCONFIG_FILE_USER ;

  if (!sequence_map || !sequence_map->sources)
    {
      g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_NO_SOURCES, "There are no sources") ;
    }

  if (!tmp_error && filepath_inout && *filepath_inout)
    {
      output_file = g_strdup(*filepath_inout) ;
    }

  if (!tmp_error && !output_file)
    {
      output_file = zmapGUIFileChooser(NULL, "Configuration Export filename ?", NULL, "ini") ;

      if (!output_file)
        g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_CHECK_FILE, "No filename specified") ;
    }

  if (!tmp_error)
    {
      /* Read the context from the original input config file (if there was one - otherwise this
       * will return an empty context. Note that it will also include any system config files too.) */
      context = zMapConfigIniContextProvide(input_file, file_type) ;

      if (!context)
        g_set_error(&tmp_error, ZMAP_APP_SERVICES_ERROR, ZMAPAPPSERVICES_ERROR_CONTEXT, "Error creating context") ;
    }

  if (!tmp_error)
    {
      zMapConfigIniContextSetFile(context, file_type, output_file) ;
      zMapConfigIniContextCreateKeyFile(context, file_type) ;

      if (sequence_map->updateContext(context, file_type))
        {
          zMapConfigIniContextSetUnsavedChanges(context, file_type, TRUE) ;
          zMapConfigIniContextSave(context, file_type) ;

          sequence_map->setFlag(ZMAPFLAG_SAVE_SOURCES, FALSE) ;
        }

      /* Destroy the context */
      zMapConfigIniContextDestroy(context) ;
      context = NULL ;
    }

  if (output_file && filepath_inout && !*filepath_inout)
    {
      /* We've created the filepath in this function so set the output value from it */
      if (*filepath_inout)
        g_free(*filepath_inout) ;

      *filepath_inout = output_file ;
    }
  else if (output_file)
    {
      /* Return value wasn't requested so free this */
      g_free(output_file) ;
    }

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return ;
}


/*
 * Callback for when the config file text is changed.
 */
static void configChangedCB(GtkWidget *widget, gpointer user_data)
{
  MainFrame main_frame = (MainFrame) user_data ;
  const char *filename = gtk_entry_get_text(GTK_ENTRY(widget)) ;
 
  if (main_frame && main_frame->orig_sequence_map)
    {
      main_frame->orig_sequence_map->addSourcesFromConfig(filename, NULL, NULL) ;

      /* Update the sources list */
      updateSourcesList(main_frame, main_frame->orig_sequence_map) ;
      updateInfoLabel(main_frame, main_frame->orig_sequence_map) ;
    }

  return ;
}


/* Select a source in the sources tree. Optionally deselect other sources first. */
static void selectSource(const char *source_name, 
                         GtkTreeView *tree,
                         GtkTreeModel *model,
                         const bool deselect_others)
{
  zMapReturnIfFail(source_name && tree && model) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(tree) ;

  if (!selection)
    {
      zMapLogWarning("%s", "Could not select source: failed to get tree view selection") ;
    }
  else
    {
      /*! \todo select_iter actually the deselects others anyway so we need to reselect the
       * original selection  */
      //if (deselect_others)
      //  gtk_tree_selection_unselect_all(selection) ;

      /* Find the tree row with this source name */
      GtkTreeIter iter ;

      if (gtk_tree_model_get_iter_first(model, &iter))
        {
          bool found = false ;

          do
            {
              char *cur_source_name = NULL ;

              gtk_tree_model_get (model, &iter,
                                  SourceColumn::NAME, &cur_source_name,
                                  -1);
              
              if (strcmp(cur_source_name, source_name) == 0)
                {
                  found = true ;
                  break ;
                }
            } while (gtk_tree_model_iter_next(model, &iter)) ;
      
          if (found)
            gtk_tree_selection_select_iter(selection, &iter) ;
        }
    }

  return ;
}


/* Utility to get the sources that are selected in the list box */
static list<ZMapConfigSource> getSelectedSources(GtkTreeView *sources_tree,
                                                 ZMapFeatureSequenceMap sequence_map,
                                                 const char *err_msg)
{
  list<ZMapConfigSource> result ;

  zMapReturnValIfFail(sources_tree && sequence_map, result) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(sources_tree) ;

  if (!selection)
    {
      err_msg = "No source selected" ;
    }
  else
    {
      GtkTreeModel *model = NULL ; 
      GList *rows = gtk_tree_selection_get_selected_rows(selection, &model) ;

      if (g_list_length(rows) < 1)
        {
          err_msg = "No source selected" ;
        }
      else
        {
          for (GList *row_item = rows; row_item; row_item = row_item->next)
            {
              GtkTreePath *path = (GtkTreePath*)(row_item->data) ;
              GtkTreeIter iter ;
              gtk_tree_model_get_iter(model, &iter, path) ;

              char *source_name = NULL ;
              gtk_tree_model_get(model, &iter, SourceColumn::NAME, &source_name, -1) ;

              if (source_name)
                {
                  ZMapConfigSource source = sequence_map->getSource(source_name) ;
                  result.push_back(source) ;
                  g_free(source_name) ;
                }
              else
                {
                  err_msg = "Program error: failed to get source name" ;
                  break ;
                }
            }
        }
    }

  return result ;
}


/* Utility to get the sources that are selected in the list box */
static list<GQuark> getSelectedSourceNames(GtkTreeView *sources_tree,
                                           const char *err_msg)
{
  list<GQuark> result ;

  zMapReturnValIfFail(sources_tree, result) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(sources_tree) ;

  if (!selection)
    {
      err_msg = "No source selected" ;
    }
  else
    {
      GtkTreeModel *model = NULL ; 
      GList *rows = gtk_tree_selection_get_selected_rows(selection, &model) ;

      if (g_list_length(rows) < 1)
        {
          err_msg = "No source selected" ;
        }
      else
        {
          for (GList *row_item = rows; row_item; row_item = row_item->next)
            {
              GtkTreePath *path = (GtkTreePath*)(row_item->data) ;
              GtkTreeIter iter ;
              gtk_tree_model_get_iter(model, &iter, path) ;

              char *source_name = NULL ;
              gtk_tree_model_get(model, &iter, SourceColumn::NAME, &source_name, -1) ;

              if (source_name)
                {
                  result.push_back(g_quark_from_string(source_name)) ;
                  g_free(source_name) ;
                }
              else
                {
                  err_msg = "Program error: failed to get source name" ;
                  break ;
                }
            }
        }
    }

  return result ;
}


/* Callback used to determine if a search term matches a row in the tree. Returns false if it
 * matches, true otherwise. */
static gboolean tree_view_search_equal_func_cb(GtkTreeModel *model,
                                               gint column,
                                               const gchar *key,
                                               GtkTreeIter *iter,
                                               gpointer user_data)
{
  gboolean result = TRUE ;

  gboolean found = FALSE ;

  /* Get the column to search */
  SourceColumn col_id = SourceColumn::NAME ;
  found = columnMatchesText(model, iter, col_id, key) ;
  
  // The return value is false if it matches, true otherwise
  result = !found ;

  return result ;
}

/* Callback called when the user hits the enter key in the filter text entry box */
//static void filter_entry_activate_cb(GtkEntry *entry, gpointer user_data)
//{
//  MainFrame main_data = (MainFrame)user_data ;
//  zMapReturnIfFail(main_data && main_data->sources_tree) ;
//
//  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(main_data->sources_model_filtered) ;
//
//  /* Refilter the filtered model */
//  if (filter)
//    gtk_tree_model_filter_refilter(filter) ;
//
//  /* If the entry is empty then use the unfiltered tree model so that it can be
//   * reordered. Otherwise use the filtered model (which will disable reordering). */
//  const char *text = gtk_entry_get_text(entry) ;
//
//  if (!text || *text == 0 || !filter)
//    gtk_tree_view_set_model(main_data->sources_tree, main_data->sources_model) ;
//  else
//    gtk_tree_view_set_model(main_data->sources_tree, main_data->sources_model_filtered) ;
//}


/* Clear the text in the search/filter boxes */
//static void clearSearchFilter(MainFrame main_data)
//{
//  zMapReturnIfFail(main_data) ;
//
//  if (main_data->search_entry)
//    gtk_entry_set_text(main_data->search_entry, "") ;
//
////  if (main_data->filter_entry)
////    gtk_entry_set_text(main_data->filter_entry, "") ;
//
//  /* Refilter the filtered model */
//  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(main_data->sources_model_filtered) ;
//  if (filter)
//    gtk_tree_model_filter_refilter(filter) ;
//
//  /* Use the unfiltered tree model so that it can be reordered */
//  gtk_tree_view_set_model(main_data->sources_tree, main_data->sources_model) ;
//
//  /* Clear the sort column so that the tree can be reordered again manually. Note that this does
//   * not revert the sort order - use the Revert button for that. */
//  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(main_data->sources_model), 
//                                       GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
//                                       GTK_SORT_ASCENDING) ;
//}


/* Callback to clear the current search/filter */
//static void clear_button_cb(GtkButton *button, gpointer user_data)
//{
//  MainFrame main_data = (MainFrame)user_data ;
//  zMapReturnIfFail(main_data && main_data->sources_tree) ;
//
//  clearSearchFilter(main_data) ;
//}


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


/* Utility to see if the value in the given column matches the given text */
static gboolean columnMatchesText(GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  SourceColumn col_id, 
                                  const char *text)
{
  gboolean result = FALSE ;

  // Get the string value for this column from the selected row in the drop-down box
  char *value = NULL ;
  gtk_tree_model_get(model, iter, col_id, &value, -1) ;

  if (text &&
      value && 
      *text != 0 &&
      *value != 0 && 
      my_strcasestr(value, text) != NULL)
    {
      result = TRUE ;
    }

  g_free(value) ;

  return result ;
}


/* Called when the user hits the select-all button. */
static void select_all_button_cb(GtkButton *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(main_data && main_data->sources_model) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(main_data->sources_tree) ;

  if (selection)
    {
      gtk_tree_selection_select_all(selection) ;
    }
}

/* Called when the user hits the select-none button. */
static void select_none_button_cb(GtkButton *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(main_data && main_data->sources_model) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(main_data->sources_tree) ;

  if (selection)
    {
      gtk_tree_selection_unselect_all(selection) ;
    }
}


/* Called when the user hits the invert-selection button. */
static void select_invert_button_cb(GtkButton *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;
  zMapReturnIfFail(main_data && main_data->sources_tree) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(main_data->sources_tree) ;

  // Loop through all rows in the tree model and toggle the selection status
  GtkTreeIter iter ;

  for (GtkTreePath *path = gtk_tree_path_new_first(); 
       gtk_tree_model_get_iter(main_data->sources_model, &iter, path);
       gtk_tree_path_next(path))
    {
      if (gtk_tree_selection_path_is_selected(selection, path))
        gtk_tree_selection_unselect_path(selection, path) ;
      else
        gtk_tree_selection_select_path(selection, path) ;
    }
}


/* Set the load status for the currently selected tree rows */
static void treeSelectionSetLoadStatus(MainFrame main_data, const bool load)
{
  // Loop through the selected rows
  GtkTreeSelection *selection = gtk_tree_view_get_selection(main_data->sources_tree) ;
  GList *rows = gtk_tree_selection_get_selected_rows(selection, &main_data->sources_model) ;

  for (GList *row_item = rows; row_item; row_item = row_item->next)
    {
      GtkTreePath *selected_path = (GtkTreePath*)(row_item->data) ;
      GtkTreeIter iter ;
          
      if (gtk_tree_model_get_iter(main_data->sources_model, &iter, selected_path))
        {
          treeIterSetLoadStatus(main_data->sources_model, &iter, load, FALSE, main_data, main_data->orig_sequence_map) ;
        }
    }
  
  // Finally, update the info label, which displays info about which sources will be loaded
  updateInfoLabel(main_data, main_data->orig_sequence_map) ;
}


/* Called when the user hits the set-load-status button. */
static void set_load_cb(GtkButton *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;

  treeSelectionSetLoadStatus(main_data, true) ;
}


/* Called when the user hits the set-unload-status button. */
static void set_unload_cb(GtkButton *button, gpointer user_data)
{
  MainFrame main_data = (MainFrame)user_data ;

  treeSelectionSetLoadStatus(main_data, false) ;
}


/* Callback used to determine if a given row in the filtered tree model is visible */
//static gboolean tree_model_filter_visible_cb(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
//{
//  gboolean result = FALSE ;
//
//  MainFrame main_data = (MainFrame)user_data ;
//  GtkEntry *search_entry = main_data->filter_entry ;
//  zMapReturnValIfFail(search_entry, result) ;
//
//  const char *text = gtk_entry_get_text(search_entry) ;
//
//  if (!text || *text == 0)
//    {
//      /* If text isn't specified, show everything */
//      result = TRUE ;
//    }
//  else
//    {
//      SourceColumn col_id = SourceColumn::NAME ;
//      result = columnMatchesText(model, iter, col_id, text) ;
//    }
//
//  return result ;
//}
