/*  File: zmapWindowStyles.cpp
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 * Description: Implements showing the preferences for all styles
 *              for a zmapWindow instance.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <zmapWindow_P.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapStyleTree.hpp>

#include <gbtools/gbtools.hpp>

#include <vector>
#include <gdk/gdkkeysyms.h>


#define STYLES_CHAPTER "Styles"
#define STYLES_PAGE "Styles"


typedef struct EditStylesDialogStruct_
{
  GtkWidget *dialog ;
  ZMapWindow window ;
  ZMapStyleTree *styles_tree ;
  GtkTreeView *tree_view ;
  GtkTreeModel *tree_model ;
  GtkTreePath *selected_tree_path ;
  GQuark orig_style_id ;
} EditStylesDialogStruct, *EditStylesDialog ;



typedef enum {STYLE_COLUMN, PARENT_COLUMN, FEATURESETS_COLUMN, N_COLUMNS} TreeColumn ;


static void saveCB(void *user_data) ;
static void cancelCB(void *user_data_unused) ;

static void addContentTree(EditStylesDialog data, GtkBox *box) ;
static void addContentButtons(EditStylesDialog data, GtkBox *box) ;
static void treeNodeCreateWidgets(EditStylesDialog data, ZMapStyleTree *node, GtkTreeStore *store, GtkTreeIter *parent) ;
void editStylesDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data) ;
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data) ;
static EditStylesDialog createStylesDialog(ZMapWindow window, 
                                           const gboolean add_edit_buttons,
                                           const char *title,
                                           GCallback response_cb,
                                           const GQuark orig_style_id) ;

static void destroyCB(GtkWidget *widget, gpointer user_data) ;
static gboolean keyPressCB(GtkWidget *widget, GdkEventKey *event, gpointer user_data) ;
GQuark getSelectedStyleID(EditStylesDialog data) ;
GQuark showChooseStyleDialog(ZMapWindow window, const char *title, const GQuark default_style_id) ;


/* 
 *                   External interface routines
 */

/* Entry point to show the Edit Styles dialog */
void zMapWindowShowStylesDialog(ZMapWindow window)
{
  zMapReturnIfFail(window) ;

  EditStylesDialog data = createStylesDialog(window, 
                                             TRUE, 
                                             "Edit Styles",
                                             G_CALLBACK(editStylesDialogResponseCB),
                                             0) ;

  if (data && data->dialog)
    {
      gtk_dialog_add_button(GTK_DIALOG(data->dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_APPLY) ;

      gtk_dialog_set_default_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_APPLY) ;
      
      gtk_widget_show_all(data->dialog) ;
      zMapGUIRaiseToTop(data->dialog);
    }
}


/* Entry point to show the Choose Style dialog. Returns the unique id of the chosen style or 0 if
 * cancelled. Starts up with the given featuresets' style selected if it exists */
GQuark zMapWindowChooseStyleDialog(ZMapWindow window, GList *feature_sets)
{
  GQuark result = 0 ;
  zMapReturnValIfFail(window, result) ;

  /* If all the feature sets have the same style then use that as the default style */
  GQuark default_style_id = 0 ;
  gboolean same_style = TRUE ;

  for (GList *item = feature_sets; item && same_style; item = item->next)
    {
      ZMapFeatureSet feature_set = (ZMapFeatureSet)(item->data);

      if (feature_set->style)
        {
          if (!default_style_id)
            {
              default_style_id = feature_set->style->unique_id ;
            }
          else if (default_style_id != feature_set->style->unique_id)
            {
              same_style = FALSE ;
              default_style_id = 0 ;
            }
        }
    }

  result = showChooseStyleDialog(window, "Choose style", default_style_id) ;

  return result ;
}


/* Entry point to show the Choose Style dialog. Returns the id of the chosen style or 0 if
 * cancelled. Starts up with the given featureset's style selected if it exists */
GQuark zMapWindowChooseStyleDialog(ZMapWindow window, ZMapFeatureSet feature_set)
{
  GQuark result = 0 ;
  zMapReturnValIfFail(feature_set, result) ;

  char *title = NULL ;

  if (feature_set)
    title = g_strdup_printf("Choose Style for %s", g_quark_to_string(feature_set->original_id)) ;
  else
    title = g_strdup("Choose Style") ;

  GQuark default_style_id = 0 ;

  if (feature_set && feature_set->style)
    default_style_id = feature_set->style->unique_id ;

  result = showChooseStyleDialog(window, title, default_style_id) ;

  g_free(title) ;

  return result ;
}


/* Does the work to show the choose-style dialog */
GQuark showChooseStyleDialog(ZMapWindow window, const char *title, const GQuark default_style_id)
{
  GQuark result = 0 ;
  zMapReturnValIfFail(window, result) ;

  EditStylesDialog data = createStylesDialog(window,
                                             FALSE, 
                                             title,
                                             NULL,
                                             default_style_id) ;

  if (data && data->dialog)
    {
      gtk_dialog_add_button(GTK_DIALOG(data->dialog), GTK_STOCK_APPLY, GTK_RESPONSE_APPLY) ;
      gtk_dialog_add_button(GTK_DIALOG(data->dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL) ;

      gtk_dialog_set_default_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_APPLY) ;

      gtk_widget_show_all(data->dialog) ;
      
      gint response = gtk_dialog_run(GTK_DIALOG(data->dialog)) ;

      if (response == GTK_RESPONSE_APPLY)
        {
          result = getSelectedStyleID(data) ;
        }

      /* If response is 'none' that means it's already been destroyed */
      if (response != GTK_RESPONSE_NONE)
        {
          gtk_widget_destroy(data->dialog) ;
        }
    }

  return result ;
}



/* 
 *                      Internal routines
 */

/* Create the content for the styles dialog. */
static EditStylesDialog createStylesDialog(ZMapWindow window, 
                                           const gboolean add_edit_buttons, 
                                           const char *title,
                                           GCallback response_cb,
                                           const GQuark orig_style_id)
{
  EditStylesDialogStruct *data = g_new0(EditStylesDialogStruct, 1) ;
  data->window = window ;
  data->styles_tree = zMapWindowGetStyles(data->window) ;
  data->orig_style_id = orig_style_id ; 

  /* Sort styles alphabetically so they're easier to read  */
  data->styles_tree->sort() ;

  data->dialog = zMapGUIDialogNew(NULL, title, response_cb, data) ;

  g_signal_connect(GTK_OBJECT(data->dialog), "destroy", GTK_SIGNAL_FUNC(destroyCB), data) ;
  g_signal_connect(GTK_OBJECT(data->dialog), "key_press_event", GTK_SIGNAL_FUNC(keyPressCB), data) ;

  GtkBox *vbox = GTK_BOX(GTK_DIALOG(data->dialog)->vbox) ;

  addContentTree(data, vbox) ;

  if (add_edit_buttons)
    addContentButtons(data, vbox) ;

  int width ;
  int height ;
  gbtools::GUIGetTrueMonitorSizeFraction(data->dialog, 0.5, 0.8, &width, &height) ;
  width = std::min(width, 700) ;
  height = std::min(height, 800) ;

  gtk_window_set_default_size(GTK_WINDOW(data->dialog), width, height) ;

  return data ;
}


/* Get the currently-selected style and return its unique id */
GQuark getSelectedStyleID(EditStylesDialog data)
{
  GQuark result = 0 ;
  zMapReturnValIfFail(data && data->tree_view, result) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->tree_view)) ;
  
  if (selection)
    {
      GtkTreeIter iter;
      GtkTreeModel *model = NULL;
      
      gtk_tree_selection_get_selected(selection, &model, &iter) ;

      if (model)
        {
          char *style_name = NULL ;
          gtk_tree_model_get(model, &iter, STYLE_COLUMN, &style_name, -1) ;

          if (style_name)
            {
              result = zMapStyleCreateID(style_name) ;
              g_free(style_name) ;
            }
        }
    }    

  return result ;
}


static void treeViewAddColumn(GtkWidget *tree, GtkCellRenderer *renderer, 
                              const char *name, const char *type, TreeColumn column_id)
{
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes (name,
                                                                        renderer,
                                                                        type, column_id,
                                                                        NULL);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
}

static void addContentTree(EditStylesDialog data, GtkBox *box)
{
  zMapReturnIfFail(data && data->window) ;

  GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING) ;
  treeNodeCreateWidgets(data, data->styles_tree, store, NULL) ;

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));

  data->tree_model = GTK_TREE_MODEL(store) ;
  data->tree_view = GTK_TREE_VIEW(tree) ;

  GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
                    G_CALLBACK (tree_selection_changed_cb),
                    data);

  /* If the default path has been set then update the selection and expand and scroll to it */
  if (data->selected_tree_path)
    {
      gtk_tree_view_expand_to_path(data->tree_view, data->selected_tree_path) ;
      gtk_tree_view_scroll_to_cell(data->tree_view, data->selected_tree_path, NULL, FALSE, 0.0, 0.0) ;
      gtk_tree_selection_select_path(select, data->selected_tree_path) ;
    }

  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_container_add(GTK_CONTAINER(scrolled_window), tree) ;

  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new ();

  treeViewAddColumn(tree, text_renderer, "Style", "text", STYLE_COLUMN) ;
  treeViewAddColumn(tree, text_renderer, "Parent", "text", PARENT_COLUMN) ;
  treeViewAddColumn(tree, text_renderer, "Featuresets", "text", FEATURESETS_COLUMN) ;
  
  gtk_box_pack_start(box, scrolled_window, TRUE, TRUE, 0) ;
}


/* Get the style id of the style in the given row of the given tree model. Returns 0 if not found */
static GQuark getSelectedStyleID(GtkTreeModel *tree_model, GtkTreePath *tree_path)
{
  GQuark style_id = 0 ;
  zMapReturnValIfFail(tree_model && tree_path, style_id) ;

  GtkTreeIter iter ;
  gtk_tree_model_get_iter(tree_model, &iter, tree_path) ;

  char *style_name = NULL ;
  gtk_tree_model_get(tree_model, &iter, STYLE_COLUMN, &style_name, -1) ;

  if (style_name)
    {
      style_id = zMapStyleCreateID(style_name) ;
      g_free(style_name) ;
    }

  return style_id ;
}


/* Get the style that is in the currently selected row. Returns NULL if not found */
static ZMapFeatureTypeStyle getSelectedStyle(EditStylesDialog data)
{
  ZMapFeatureTypeStyle style = NULL ;

  if (data && data->selected_tree_path)
    {
      GQuark style_id = getSelectedStyleID(data->tree_model, data->selected_tree_path) ;
      
      style = data->styles_tree->find_style(style_id) ;
    }  

  return style ;
}


/* Callback called when the user clicks the edit button. Opens the Edit Style dialog on the
 * currently selected style */
static void edit_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(data && data->tree_model) ;

  ZMapFeatureTypeStyle style = getSelectedStyle(data) ;

  if (style)
    {
      zMapWindowShowStyleDialog(data->window, style, 0, NULL, NULL, NULL);
    }
  else
    {
      zMapWarning("%s", "Please select a style to edit") ;
    }
}


/* Callback called when the user clicks the edit button. Deletes the currently selected style
 * (asking for confirmation first) */
static void delete_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(data && data->tree_model) ;

  ZMapFeatureTypeStyle style = getSelectedStyle(data) ;

  if (style)
    {
      char *msg = g_strdup_printf("Are you sure you want to delete style '%s'?", g_quark_to_string(style->original_id)) ;

      if (zMapGUIMsgGetBool(NULL, ZMAP_MSG_WARNING, msg))
        {
          /* Remove the style from the tree in the context */
          data->styles_tree->remove_style(style) ;

          /* Remove it from our dialog's tree view */
          GtkTreeStore *tree_store = GTK_TREE_STORE(data->tree_model) ;
          GtkTreeIter iter ;

          gtk_tree_model_get_iter(data->tree_model, &iter, data->selected_tree_path) ;
          gtk_tree_store_remove(tree_store, &iter) ;

          /* Reset the selection to iter, which will point to the next row (if valid) */
          gtk_tree_path_free(data->selected_tree_path) ;
          data->selected_tree_path = NULL ;
              
          if (gtk_tree_store_iter_is_valid(tree_store, &iter))
            data->selected_tree_path = gtk_tree_model_get_path(data->tree_model, &iter) ;

          /* Destroy the style */
          zMapStyleDestroy(style);
        }
    }
  else
    {
      zMapWarning("%s", "Please select a style to delete") ;
    }
}



/* This is called after a new style has been added to update the tree with the new style */
static void updateNewStyle(gpointer cb_data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)cb_data ;
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(style && data && data->window) ;

  GtkTreeStore *tree_store = GTK_TREE_STORE(data->tree_model) ;
  ZMapFeatureContext context = zMapWindowGetContext(data->window) ;

  /* Add the style to the hierarchy */
  data->styles_tree->add_style(style) ;

  /* Add a new row to the tree store under the currently-selected row (or to the root if
   * no row is selected) */
  GtkTreeIter parent_iter ;
  GtkTreeIter iter ;
  gtk_tree_model_get_iter(data->tree_model, &parent_iter, data->selected_tree_path) ;
  gtk_tree_store_append(tree_store, &iter, &parent_iter) ;

  const char *parent_name = "" ;
  if (style->parent_id)
    {
      ZMapFeatureTypeStyle parent_style = data->styles_tree->find_style(style->parent_id) ;
      parent_name = g_quark_to_string(parent_style->original_id) ;
    }

  GList *featuresets_list = zMapStyleGetFeaturesetsIDs(style, (ZMapFeatureAny)context) ;
  char *featuresets = zMap_g_list_quark_to_string(featuresets_list, ";") ;

  g_free(featuresets) ;
  g_list_free(featuresets_list) ;

  gtk_tree_store_set(tree_store, &iter, 
                     0, g_quark_to_string(style->original_id), 
                     1, parent_name, 
                     2, featuresets, 
                     -1);
}


static void add_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(data && data->tree_model) ;

  ZMapFeatureTypeStyle parent_style = getSelectedStyle(data) ;

  static int count = 1 ;
  char *new_style_name = g_strdup_printf("new-style-%d", count) ;

  zMapWindowShowStyleDialog(data->window, parent_style, g_quark_from_string(new_style_name), NULL, updateNewStyle, data) ;

  g_free(new_style_name) ;
}


static void addContentButtons(EditStylesDialog data, GtkBox *box)
{
  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 0)) ;
  gtk_box_pack_start(box, GTK_WIDGET(hbox), FALSE, FALSE, 0) ;

  GtkWidget *button = gtk_button_new_with_label("Edit") ;
  gtk_box_pack_start(hbox, button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(edit_button_clicked_cb), data) ;

  button = gtk_button_new_with_label("Delete") ;
  gtk_box_pack_start(hbox, button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(delete_button_clicked_cb), data) ;

  button = gtk_button_new_with_label("Add") ;
  gtk_box_pack_start(hbox, button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(add_button_clicked_cb), data) ;
}


static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(data) ;

  GtkTreeIter iter;
  GtkTreeModel *model;

  if (data->selected_tree_path)
    {
      gtk_tree_path_free(data->selected_tree_path) ;
      data->selected_tree_path = NULL ;
    }

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      data->selected_tree_path = gtk_tree_model_get_path(data->tree_model, &iter);
    }
}

/* Handles the response to the feature-show dialog */
void editStylesDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data)
{
  switch (response_id)
  {
    case GTK_RESPONSE_APPLY: //fall through
    case GTK_RESPONSE_ACCEPT:
      saveCB(data) ;
      break;

    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
    case GTK_RESPONSE_REJECT:
      cancelCB(data) ;
      break;

    default:
      break;
  };
}


static void saveCB(void *user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;

  if (data)
    {
      gboolean ok = TRUE ;

      /* only destroy the dialog if the save succeeded */
      if (ok)
        gtk_widget_destroy(data->dialog) ;
    }

  return ;
}

static void cancelCB(void *user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;

  if (data)
    {
      gtk_widget_destroy(data->dialog) ;
    }

  return ;
}


static void destroyCB(GtkWidget *widget, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;

  if (data)
    g_free(data) ;
}


static void treeNodeCreateWidgets(EditStylesDialog data, ZMapStyleTree *node, GtkTreeStore *store, GtkTreeIter *parent_iter)
{
  /* Add a row for this node into the parent iterator (or root of the store if parent_iter is
   * null). Note that the root node in the tree doesn't have a style so nothing to add. */
  ZMapFeatureTypeStyle style = node->get_style() ;

  if (style)
    {
      char *style_name = g_strdup(g_quark_to_string(style->original_id)) ;
      char *parent_name = style->parent_id ? g_strdup(g_quark_to_string(style->parent_id)) : NULL ;

      ZMapFeatureContext context = zMapWindowGetContext(data->window) ;

      GList *featuresets_list = zMapStyleGetFeaturesetsIDs(style, (ZMapFeatureAny)context) ;
      char *featuresets = zMap_g_list_quark_to_string(featuresets_list, ";") ;

      /* Add a row to the tree store for this node */
      GtkTreeIter iter ;
      gtk_tree_store_append(store, &iter, parent_iter);
      gtk_tree_store_set(store, &iter, 0, style_name, 1, parent_name, 2, featuresets, -1);

      g_free(featuresets) ;
      g_list_free(featuresets_list) ;

      /* If this style is the original style id then by default select it */
      if (style->unique_id == data->orig_style_id)
        data->selected_tree_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter) ;

      /* Process the child nodes */
      std::vector<ZMapStyleTree*> children = node->get_children() ;

      for (std::vector<ZMapStyleTree*>::iterator child = children.begin(); child != children.end(); ++child)
        treeNodeCreateWidgets(data, *child, store, &iter) ;
    }
  else
    {
      /* Just process the children */
      std::vector<ZMapStyleTree*> children = node->get_children() ;

      for (std::vector<ZMapStyleTree*>::iterator child = children.begin(); child != children.end(); ++child)
        treeNodeCreateWidgets(data, *child, store, parent_iter) ;
    }
}


static gboolean expandCollapseSelectedRow(EditStylesDialog data, const gboolean expand, const gboolean expand_all)
{
  gboolean handled = FALSE ;
  
  if (data && data->tree_view && data->selected_tree_path)
    {
      if (expand)
        gtk_tree_view_expand_row(data->tree_view, data->selected_tree_path, expand_all) ;
      else
        gtk_tree_view_collapse_row(data->tree_view, data->selected_tree_path) ;

      handled = TRUE ;
    }

  return handled ;
}

static gboolean expandCollapseAll(EditStylesDialog data, const gboolean expand)
{
  gboolean handled = FALSE ;

  if (data && data->tree_view)
    {
      if (expand)
        gtk_tree_view_expand_all(data->tree_view) ;
      else
        gtk_tree_view_collapse_all(data->tree_view) ;
    }  

  return handled ;
}

static gboolean keyPressCB(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  gboolean handled = FALSE ;
  EditStylesDialog data = (EditStylesDialog)user_data ;

  zMapReturnValIfFail(data, handled) ;
  
  const gboolean ctrlModifier = (event->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK;
  const gboolean shiftModifier = (event->state & GDK_SHIFT_MASK) == GDK_SHIFT_MASK;
  //const gboolean metaModifier = (event->state & GDK_META_MASK) == GDK_META_MASK;

  switch (event->keyval)
    {
    case GDK_Return:
      gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_APPLY) ;
      handled = TRUE ;
      break ;
      
    case GDK_Escape:
      gtk_dialog_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_CANCEL) ;
      handled = TRUE ;
      break ;
      
    case GDK_Left:
      if (ctrlModifier)
        handled = expandCollapseAll(data, FALSE) ;
      else
        handled = expandCollapseSelectedRow(data, FALSE, shiftModifier) ;

      break ;

    case GDK_Right:
      if (ctrlModifier)
        handled = expandCollapseAll(data, TRUE);
      else
        handled = expandCollapseSelectedRow(data, TRUE, shiftModifier) ;

      break ;

    default:
      break ;
    }

  return handled ;
}
