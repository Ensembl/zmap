/*  File: zmapControlStyles.cpp
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
 * Description: Implements showing the preferences configuration
 *              window for a zmapControl instance.
 *
 * Exported functions: See zmapControl_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

//#include <ZMap/zmapUtils.hpp>
//#include <ZMap/zmapUtilsGUI.hpp>
//#include <ZMap/zmapGLibUtils.hpp>
#include <zmapControl_P.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapStyleTree.hpp>

#include <gbtools/gbtools.hpp>

#include <vector>


#define STYLES_CHAPTER "Styles"
#define STYLES_PAGE "Styles"


typedef struct GetStylesDataStruct_
{
  ZMapStyleTree &styles_tree ;
  GHashTable *styles ;
} GetStylesDataStruct, *GetStylesData ;


typedef struct EditStylesDialogStruct_
{
  GtkWidget *dialog ;
  ZMap zmap ;
  ZMapStyleTree styles_tree ;
  GtkTreeModel *tree_model ;
  GtkTreePath *selected_tree_path ;
} EditStylesDialogStruct, *EditStylesDialog ;



typedef enum {STYLE_COLUMN, PARENT_COLUMN, FEATURESETS_COLUMN, N_COLUMNS} TreeColumn ;


static void saveCB(void *user_data) ;
static void cancelCB(void *user_data_unused) ;

static void addContentTree(EditStylesDialog data, GtkBox *box) ;
static void addContentButtons(EditStylesDialog data, GtkBox *box) ;
static void treeNodeCreateWidgets(EditStylesDialog data, ZMapStyleTree *node, GtkTreeStore *store, GtkTreeIter *parent) ;
void editStylesDialogResponseCB(GtkDialog *dialog, gint response_id, gpointer data) ;
static void getStylesHierarchy(ZMap zmap, ZMapStyleTree &styles_tree) ;
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data) ;



/* 
 *                   External interface routines
 */

void zmapControlShowStyles(ZMap zmap)
{
  EditStylesDialogStruct *data = g_new0(EditStylesDialogStruct, 1) ;
  data->zmap = zmap ;

  data->dialog = zMapGUIDialogNew(NULL, "Edit Styles", G_CALLBACK(editStylesDialogResponseCB), data) ;

  gtk_dialog_add_button(GTK_DIALOG(data->dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE) ;
  gtk_dialog_set_default_response(GTK_DIALOG(data->dialog), GTK_RESPONSE_CLOSE) ;

  GtkBox *vbox = GTK_BOX(GTK_DIALOG(data->dialog)->vbox) ;
  addContentTree(data, vbox) ;
  addContentButtons(data, vbox) ;

  int width ;
  int height ;
  gbtools::GUIGetTrueMonitorSizeFraction(data->dialog, 0.5, 0.8, &width, &height) ;
  width = std::min(width, 700) ;
  height = std::min(height, 800) ;

  gtk_window_set_default_size(GTK_WINDOW(data->dialog), width, height) ;

  gtk_widget_show_all(data->dialog) ;
      
  zMapGUIRaiseToTop(data->dialog);
}



/* 
 *                      Internal routines
 */

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
  getStylesHierarchy(data->zmap, data->styles_tree) ;

  GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING) ;
  treeNodeCreateWidgets(data, &data->styles_tree, store, NULL) ;

  data->tree_model = GTK_TREE_MODEL(store) ;

  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));

  GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
  g_signal_connect (G_OBJECT (select), "changed",
                    G_CALLBACK (tree_selection_changed_cb),
                    data);


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
      GHashTable *styles = zMapViewGetStyles(data->zmap->focus_viewwindow) ;
      GQuark style_id = getSelectedStyleID(data->tree_model, data->selected_tree_path) ;
      
      style = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GINT_TO_POINTER(style_id)) ;
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
      ZMapWindow window = zMapViewGetWindow(data->zmap->focus_viewwindow) ;

      zMapWindowShowStyleDialog(window, style, 0, NULL);
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
          data->styles_tree.remove_style(style) ;

          GHashTable *styles = zMapViewGetStyles(data->zmap->focus_viewwindow) ;
          g_hash_table_remove(styles,GUINT_TO_POINTER(style->unique_id));

          zMapStyleDestroy(style);

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
        }
    }
  else
    {
      zMapWarning("%s", "Please select a style to delete") ;
    }
}


static void add_button_clicked_cb(GtkWidget *button, gpointer user_data)
{
  EditStylesDialog data = (EditStylesDialog)user_data ;
  zMapReturnIfFail(data && data->tree_model) ;

  ZMapWindow window = zMapViewGetWindow(data->zmap->focus_viewwindow) ;
  ZMapFeatureTypeStyle parent_style = getSelectedStyle(data) ;

  static int count = 1 ;
  char *new_style_name = g_strdup_printf("new-style-%d", count) ;

  zMapWindowShowStyleDialog(window, parent_style, g_quark_from_string(new_style_name), NULL) ;

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

      g_free(data) ;
    }

  return ;
}


/* For a given style, add it to the appropriate node in the style hierarchy tree */
static void populateStyleHierarchy(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)value ;
  GetStylesData data = (GetStylesData)user_data ;

  data->styles_tree.add_style(style, data->styles) ;
}


/* Get the styles as a tree representing the parent-child hierarchy */
static void getStylesHierarchy(ZMap zmap, ZMapStyleTree &styles_tree)
{
  GHashTable *styles = zMapViewGetStyles(zmap->focus_viewwindow) ;
  zMapReturnIfFail(zmap) ;

  if (styles)
    {
      GetStylesDataStruct data = {styles_tree, styles} ;

      g_hash_table_foreach(styles, populateStyleHierarchy, &data) ;
    }

  /* Sort styles alphabetically so they're easier to read  */
  styles_tree.sort() ;
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

      ZMapFeatureContext context = zMapViewGetContext(data->zmap->focus_viewwindow) ;

      GList *featuresets_list = zMapStyleGetFeaturesetsIDs(style, (ZMapFeatureAny)context) ;
      char *featuresets = zMap_g_list_quark_to_string(featuresets_list, ";") ;

      /* Add a row to the tree store for this node */
      GtkTreeIter iter ;
      gtk_tree_store_append(store, &iter, parent_iter);
      gtk_tree_store_set(store, &iter, 0, style_name, 1, parent_name, 2, featuresets, -1);

      g_free(featuresets) ;
      g_list_free(featuresets_list) ;

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


