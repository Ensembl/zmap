/*  File: zmapWindowStyle.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code implementing the menus for sequence display.
 *
 * Exported functions: ZMap/zmapWindows.h
 *
 *-------------------------------------------------------------------
 */

/*
  called from zmapWindowMenu.c
  initially allows style fill and border colours to be set
  creating a child style if necessary
  then applies the new style to the featureset
  using the code in zmapWindowMenus.c

  new file started as a) windowMenus.c is quite long already and
  b) styles have a lot of other options we could set
*/


#include <ZMap/zmap.hpp>

#include <sstream>
#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp> /* zMap_g_hash_table_nth */
#include <zmapWindow_P.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>

using std::stringstream ;


#define XPAD 3
#define YPAD 3

typedef struct
{
  ZMapWindow window ;
  ZMapFeatureTypeStyle style ;            /* The style we're editing. This may become the parent
                                           * style if creating a new child style */
  ZMapFeatureSet feature_set ;            /* the feature set the user selected to change the
                                           * style for (may be null if editing a style directly) */
  GList *feature_sets ;                   /* all featuresets that have the original style */

  gboolean create_child ;

  gboolean create_new_style ;            /* True if we're creating a new style */
  GQuark new_style_name ;                /* New name if creating a new style */

  GQuark new_style_id ;                  /* unique id version of new_style_name */
  ZMapFeatureTypeStyleStruct orig_style_copy;  /* copy of original, used for Revert */

  GQuark last_edited_style_id;            /* if we've already changed a style remember its id here */

  GFunc created_cb_func ;                 /* if non-null, called when a new style is created */
  gpointer cb_data ;


  /*Widgets on the dialog */

  GtkWidget *toplevel ;
  GtkWidget *featureset_name_widget;      /* Name of the featureset whose style is being
                                           * edited */
  GtkWidget *orig_style_name_widget;      /* Widget showing original style name for this featureset */
  GtkWidget *new_style_name_widget;       /* User-editable style name for the new/existing style
                                           * to edit */

  GtkWidget *fill_widget ;                /* Fill colour button */ 
  GtkWidget *border_widget ;              /* Border colour button */ 
  GtkWidget *cds_fill_widget ;            /* CDS fill colour button */ 
  GtkWidget *cds_border_widget ;          /* CDS border colour button */
  GList *cds_widgets;                     /* list of all CDS widgets (so we can easily show/hide
                                           * them all) */ 

  GtkTreeView *tree_view ;
  GtkListStore *list_store ;              /* Stores list of featuresets */

  GtkWidget *stranded;
  GtkWidget *min_score;
  GtkWidget *max_score;


} StyleChangeStruct, *StyleChange;


static void createToplevel(StyleChange my_data) ;
static void createInfoWidgets(StyleChange my_data, GtkTable *table, const int cols, const int rows, int *row) ;
static void createColourWidgets(StyleChange my_data, GtkTable *table, int *row) ;
static void createCDSColourWidgets(StyleChange my_data, GtkTable *table, int *row) ;
static void createOtherWidgets(StyleChange my_data, GtkTable *table, int *row) ;

static void responseCB(GtkDialog *dialog, gint responseId, gpointer data) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;

static gboolean applyChanges(gpointer cb_data) ;
static gboolean revertChanges(gpointer cb_data) ;
static void renameStyle(StyleChange my_data, ZMapFeatureTypeStyle style, 
                        const char *new_style_name, const GQuark new_style_id) ;
static void updateStyle(StyleChange my_data, ZMapFeatureTypeStyle style) ;

static void createChildStyleCB(GtkButton *button, gpointer cb_data) ;
static void updateFeaturesetListStore(StyleChange my_data) ;

static void scoreCalulateCB(GtkButton *button, gpointer cb_data) ;


/* Entry point to display the Edit Style dialog. Edits the given style or creates a new one with
 * the given name (if non-0) as a child of the given style (or a new root style if the given
 * style is null). If a new style is created and feature_set is non-null then the style will be
 * applied only to that featureset; otherwise it will be applied to all featuresets that use the
 * original style. */
void zMapWindowShowStyleDialog(ZMapWindow window,
                               ZMapFeatureTypeStyle style,
                               const gboolean create_child,
                               ZMapFeatureSet feature_set,
                               GFunc created_cb_func,
                               gpointer cb_data)
{
  /* Check if the dialog data has already been created - if so, just update the existing data
   * from the given feature/style */
  if(zmapWindowStyleDialogSetStyle(window, style, feature_set, create_child))
    return;

  if (!style)
    {
      zMapWarning("%s", "Style is null") ;
      return ;
    }

  StyleChange my_data = g_new0(StyleChangeStruct,1);

  my_data->style = style ;
  my_data->window = window;
  my_data->created_cb_func = created_cb_func ;
  my_data->cb_data = cb_data ;
  my_data->create_child = create_child ;

  /* Add ptr so parent knows about us */
  my_data->window->style_window = (gpointer) my_data ;

  /* set up the top level dialog window */
  createToplevel(my_data) ;

  /* Create a table to lay out all the widgets in */
  const int rows = 7 ;
  const int cols = 5 ;

  GtkTable *table = GTK_TABLE(gtk_table_new(rows, cols, FALSE)) ;

  GtkWidget *top_vbox = GTK_DIALOG(my_data->toplevel)->vbox ;
  gtk_box_pack_start(GTK_BOX(top_vbox), GTK_WIDGET(table), TRUE, TRUE, ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH) ;

  /* Create the content */
  int row = 0 ;
  createInfoWidgets(my_data, table, cols, rows, &row) ;
  createColourWidgets(my_data, table, &row) ;
  createCDSColourWidgets(my_data, table, &row) ;
  createOtherWidgets(my_data, table, &row) ;

  gtk_widget_show_all(my_data->toplevel) ;

  if(style->mode != ZMAPSTYLE_MODE_TRANSCRIPT)        /* hide CDS widgets */
    {
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_hide_all, NULL) ;
    }

  zmapWindowStyleDialogSetStyle(my_data->window, style, feature_set, create_child);

  return ;
}


/* Interface to allow external callers to destroy the style dialog */
void zmapWindowStyleDialogDestroy(ZMapWindow window)
{
  StyleChange my_data = (StyleChange) window->style_window;

  gtk_widget_destroy(my_data->toplevel);
  window->style_window = NULL;
}


/* This updates the info in the Edit Style dialog with the given style, i.e. it takes all of the
 * style info from the given feature's style. It's called when the dialog is first opened or if
 * the user selects a different feature while the dialog is still open. */
gboolean zmapWindowStyleDialogSetStyle(ZMapWindow window, ZMapFeatureTypeStyle style_in, 
                                       ZMapFeatureSet feature_set, const gboolean create_child)
{
  zMapReturnValIfFail(window, FALSE) ;
  StyleChange my_data = (StyleChange) window->style_window;

  if(!my_data)
    return FALSE;

  ZMapFeatureTypeStyle style = style_in;
  GdkColor *fill_col = &my_data->window->canvas_background, *border_col = &my_data->window->canvas_background;

  if (!style)
    {
      zMapWarning("%s", "Style is null") ;
      return TRUE ;
    }

  my_data->style = style ;
  my_data->create_child = create_child ;
  my_data->feature_set = feature_set ;

  /* If creating a child and a featureset is given, use the featureset name as as default new style
   * name. */
  if (create_child && feature_set)
    my_data->new_style_name = feature_set->original_id ;
  else if (!create_child)
    my_data->new_style_name = my_data->style->original_id ;
  else
    my_data->new_style_name = 0 ;

  if (my_data->new_style_name)
    my_data->new_style_id = zMapStyleCreateID(g_quark_to_string(my_data->new_style_name)) ;

  /* Get the list of featuresets that use this style */
  if (my_data->feature_sets)
    g_list_free(my_data->feature_sets) ;

  my_data->feature_sets = zMapStyleGetFeaturesets(style, (ZMapFeatureAny)window->feature_context) ;

  /* Update the tree model which lists the featuresets */
  updateFeaturesetListStore(my_data) ;

  /* Make a copy of the original style so we can revert any changes we make */
  memcpy(&my_data->orig_style_copy, style,sizeof (ZMapFeatureTypeStyleStruct));

  const char *text = g_quark_to_string(style->unique_id) ;
  gtk_entry_set_text(GTK_ENTRY(my_data->orig_style_name_widget), text ? text : "");

  text = g_quark_to_string(my_data->new_style_name) ;
  gtk_entry_set_text(GTK_ENTRY(my_data->new_style_name_widget), text ? text : "");
  
  /* Update the colour buttons. */
  zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);

  gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->fill_widget), fill_col) ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->border_widget), border_col) ;

  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      /* Default to background colour */
      fill_col = &my_data->window->canvas_background ;
      border_col = &my_data->window->canvas_background;

      zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);

      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_fill_widget), fill_col) ;
      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_border_widget), border_col) ;
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_show_all, NULL) ;
    }
  else
    {
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_hide_all, NULL) ;
    }

  // set stranded status from style
  gtk_toggle_button_set_active((GtkToggleButton *) my_data->stranded, style->strand_specific);

  // set min/max score from style
  stringstream ss_min ;
  ss_min << style->min_score ;
  gtk_entry_set_text(GTK_ENTRY(my_data->min_score), ss_min.str().c_str()) ;

  stringstream ss_max ;
  ss_max << style->max_score ;
  gtk_entry_set_text(GTK_ENTRY(my_data->max_score), ss_max.str().c_str()) ;


  return TRUE;
}


/* Sets the style in the given column to the style given by style_id  */
gboolean zmapWindowColumnAddStyle(const GQuark style_id,
                                  const GQuark column_id,
                                  ZMapFeatureContextMap context_map,
                                  ZMapWindow window)
{
  gboolean ok = FALSE ;
  zMapReturnValIfFail(style_id && column_id && context_map, ok) ;

  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;
  ZMapFeatureColumn column = NULL ;
  
  if (col_iter != context_map->columns->end())
    column = col_iter->second ;

  if(column)
    {
      gpointer styles_list_ptr = NULL ;
      GList *styles_list = NULL ;

      bool list_exists = g_hash_table_lookup_extended(context_map->column_2_styles, 
                                                      GUINT_TO_POINTER(column_id),
                                                      NULL,
                                                      &styles_list_ptr);

      if (list_exists && styles_list_ptr)
        styles_list = (GList*)styles_list_ptr ;

      /* Regenerate the column's style */
      g_list_free(column->style_table);
      column->style_table = NULL;
      column->style = NULL;        /* must clear this to trigger style table calculation */
      /* NOTE column->style_id is column specific not related to a featureset, set by config */

      zMapWindowGetSetColumnStyle(window, style_id);

      /* Check the style is not already in the list */
      if (!styles_list || !g_list_find(styles_list, GUINT_TO_POINTER(style_id)))
        {
          /* Update the column_2_styles table */
          styles_list = g_list_append(styles_list, GUINT_TO_POINTER(style_id)) ;

          if (list_exists)
            g_hash_table_replace(context_map->column_2_styles, GUINT_TO_POINTER(column_id), styles_list) ;
          else
            g_hash_table_insert(context_map->column_2_styles, GUINT_TO_POINTER(column_id), styles_list) ;
        
          ok = TRUE;
        }
    }

  return ok ;
}


/* Remvoes the given style's association with the given column. Returns true unless there was a
 * fatal error (note that we still return true if the style is not in the column and was not
 * therefore removed) */
gboolean zmapWindowColumnRemoveStyle(const GQuark style_id,
                                     const GQuark column_id,
                                     ZMapFeatureContextMap context_map,
                                     ZMapWindow window)
{
  gboolean ok = TRUE ;
  zMapReturnValIfFail(style_id && column_id && context_map, ok) ;

  ZMapFeatureColumn column = NULL ;
  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;

  if (col_iter != window->context_map->columns->end())
    column = col_iter->second ;

  if(column)
    {
      GList *styles_list = (GList *)g_hash_table_lookup(context_map->column_2_styles, GUINT_TO_POINTER(column_id));

      if(styles_list)
        {
          /* Regenerate the column's style */
          g_list_free(column->style_table);
          column->style_table = NULL;
          column->style = NULL;        /* must clear this to trigger style table calculation */
          /* NOTE column->style_id is column specific not related to a featureset, set by config */

          zMapWindowGetSetColumnStyle(window, style_id);

          /* Remove the style from the column_2_styles hash table */
          for(GList *c2s = styles_list ; c2s; c2s = c2s->next)
            {
              if(GPOINTER_TO_UINT(c2s->data) == style_id)
                {
                  /* Update the hash table with the new list (or remove it if it will be empty -
                   * note that we must not delete_link if we're removing it because destroyList is
                   * called on the list when it's removed from the table) */
                  if (g_list_length(styles_list) > 1)
                    {
                      styles_list = g_list_delete_link(styles_list, c2s) ;
                      g_hash_table_replace(context_map->column_2_styles, GUINT_TO_POINTER(column_id), styles_list) ;
                    }
                  else
                    {
                      g_hash_table_remove(context_map->column_2_styles, GUINT_TO_POINTER(column_id)) ;
                    }

                  break;
                }
            }

          ok = TRUE;
        }
    }

  return ok ;
}


/* Sets the style in the given feature_set to the style given by style_id  */
gboolean zmapWindowFeaturesetSetStyle(GQuark style_id, 
                                      ZMapFeatureSet feature_set,
                                      ZMapFeatureContextMap context_map,
                                      ZMapWindow window,
                                      const gboolean update_column,
                                      const gboolean destroy_canvas_items,
                                      const gboolean redraw)
{
  gboolean ok = TRUE ;
  zMapReturnValIfFail(style_id && feature_set && context_map && window, ok) ;

  ZMapFeatureTypeStyle style;
  FooCanvasItem *set_item, *canvas_item;
  int set_strand, set_frame;
  ID2Canvas id2c;
  char *err_msg = NULL ;

  style = context_map->styles.find_style(style_id);

  if(style && update_column)
    {
      ZMapFeatureSource s2s;

      /* now tweak featureset & column styles in various places */
      s2s = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,GUINT_TO_POINTER(feature_set->unique_id));
      ZMapFeatureSetDesc f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(context_map->featureset_2_column, GUINT_TO_POINTER(feature_set->unique_id));

      // gb10: Some featuresets don't have source data. I think this is ok.
      if(s2s)
        {
          s2s->style_id = style_id ;
        }

      // The featureset should have a mapping to a column
      if (f2c)
        {
          if (feature_set && feature_set->style)
            {
              ok = zmapWindowColumnRemoveStyle(feature_set->style->unique_id, f2c->column_id, context_map, window) ;

              if (!ok)
                err_msg = g_strdup_printf("Error removing old style '%s'", g_quark_to_string(feature_set->style->unique_id)) ;
            }

          if (ok)
            {
              ok = zmapWindowColumnAddStyle(style_id, f2c->column_id, context_map, window) ;

              if (!ok)
                err_msg = g_strdup_printf("Error adding style to column '%s'", g_quark_to_string(f2c->column_id)) ;
            }
        }
      else
        {
          ok = FALSE ;
        }
    }
  else if (style)
    {
      ok = TRUE ;
    }
  else
    {
      err_msg = g_strdup_printf("Couldn't find style with this id") ;
      ok = FALSE ;
    }

  if (ok && destroy_canvas_items)
    {
      /* get current style strand and frame status and operate on 1 or more columns
       * NOTE that the FToIhash has diff hash tables per strand and frame
       * for each one remove the FtoIhash and remove the set from the column
       * if the column is empty the destroy it
       * actaull it's easier just to cycle round all the possible strand and frame combos
       * 3 hash table lookups for each, 8x
       *
       * then set the new style and redisplay the featureset
       * strand and frame will be handled by the display code
       */

      /* yes really: reverse is bigger than forwards despite appearing on the left */
      for(set_strand = ZMAPSTRAND_NONE; set_strand <= ZMAPSTRAND_REVERSE; set_strand++)
        {
          /* yes really: frames are numbered 0,1,2 and have the values 1,2,3 */
          for(set_frame = ZMAPFRAME_NONE; set_frame <= ZMAPFRAME_2; set_frame++)
            {
              ZMapStrand strand = (ZMapStrand)set_strand ;
              ZMapFrame frame = (ZMapFrame)set_frame ;

              /* this is really frustrating:
               * every operation of the ftoi hash involves
               * repeating the same nested hash table lookups
               */



              /* does the set appear in a column ? */
              /* set item is a ContainerFeatureset */
              set_item = zmapWindowFToIFindSetItem(window,
                                                   window->context_to_item,
                                                   feature_set, strand, frame);
              if(!set_item)
                continue;

              /* find the canvas item (CanvasFeatureset) containing a feature in this set */
              id2c = zmapWindowFToIFindID2CFull(window, window->context_to_item,
                                                feature_set->parent->parent->unique_id,
                                                feature_set->parent->unique_id,
                                                feature_set->unique_id,
                                                strand, frame,0);

              canvas_item = NULL;
              if(id2c)
                {
                  ID2Canvas feat = (ID2Canvas)zMap_g_hash_table_nth(id2c->hash_table,0);
                  if(feat)
                    canvas_item = feat->item;
                }


              /* look it up again to delete it :-( */
              zmapWindowFToIRemoveSet(window->context_to_item,
                                      feature_set->parent->parent->unique_id,
                                      feature_set->parent->unique_id,
                                      feature_set->unique_id,
                                      strand, frame, TRUE);


              if(canvas_item)
                {
                  FooCanvasGroup *group = (FooCanvasGroup *) set_item;

                  /* remove this featureset from the CanvasFeatureset */
                  /* if it's empty it will perform hari kiri */
                  if(ZMAP_IS_WINDOW_FEATURESET_ITEM(canvas_item))
                    {
                      zMapWindowFeaturesetItemRemoveSet(canvas_item, feature_set, TRUE);
                    }

                  /* destroy set item if empty ? */
                  if(!group->item_list)
                    zmapWindowContainerGroupDestroy((ZMapWindowContainerGroup) set_item);
                }

              zmapWindowRemoveIfEmptyCol((FooCanvasGroup **) &set_item) ;

            }
        }
    }

  if (ok)
    {
      feature_set->style = style;
      zMapWindowSetFlag(window, ZMAPFLAG_SAVE_FEATURESET_STYLE, TRUE) ;

      if (redraw)
        {
          zmapWindowRedrawFeatureSet(window, feature_set);        /* does a complex context thing */

          zmapWindowColOrderColumns(window) ;        /* put this column (deleted then created) back into the right place */
          zmapWindowFullReposition(window->feature_root_group,TRUE, "window style") ;                /* adjust sizing and shuffle left / right */
        }
    }

  if (!ok)
    {
      zMapWarning("Error setting new style '%s' for featureset '%s': %s",
                  g_quark_to_string(style_id),
                  g_quark_to_string(feature_set->unique_id),
                  (err_msg ? err_msg : ""));
    }

  if (err_msg)
    g_free(err_msg) ;
  
  return ok ;
}


/************ INTERNAL FUNCTIONS **************/

/* Utility function to create the top level dialog */
static void createToplevel(StyleChange my_data)
{
  const char *title = "Edit Style" ;

  if (my_data->create_child)
    title = "Create Style" ;

  my_data->toplevel = zMapGUIDialogNew(NULL, "Edit Style", G_CALLBACK(responseCB), my_data) ;

  gtk_dialog_add_buttons(GTK_DIALOG(my_data->toplevel), 
                         GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_REJECT,
                         GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
                         GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                         GTK_STOCK_OK, GTK_RESPONSE_OK,
                         NULL);

  /* Set up the callbacks (note that response callback is already set by zMapGUIDialogNew) */
  g_signal_connect(GTK_OBJECT(my_data->toplevel), "destroy", GTK_SIGNAL_FUNC(destroyCB), (gpointer)my_data) ;

  /* Set up other dialog properties */
  gtk_window_set_keep_above((GtkWindow *) my_data->toplevel,TRUE);
  gtk_container_set_focus_chain (GTK_CONTAINER(my_data->toplevel), NULL);
  gtk_container_border_width(GTK_CONTAINER(my_data->toplevel), 5) ;
  gtk_dialog_set_default_response(GTK_DIALOG(my_data->toplevel), GTK_RESPONSE_APPLY) ;
  gtk_window_set_default_size(GTK_WINDOW(my_data->toplevel), 500, -1) ;
}


/* Predicate for sorting featuresets alphabetically */
static int featuresetsCompareFunc(gconstpointer a, gconstpointer b)
{
  int result = 0 ;

  ZMapFeatureSet fs1 = (ZMapFeatureSet)a ;
  ZMapFeatureSet fs2 = (ZMapFeatureSet)b ;

  if (fs1 && fs1->original_id && fs2 && fs2->original_id)
    {
      result = strcmp(g_quark_to_string(fs1->original_id), 
                      g_quark_to_string(fs2->original_id));
    }
  
  return result ;
}


/* Update the list store which lists all the featuresets the style will be assigned to */
static void updateFeaturesetListStore(StyleChange my_data)
{
  zMapReturnIfFail(my_data) ;

  if (my_data->list_store)
    {
      /* Remove any existing contents first */
      GtkTreeIter iter ;
      if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(my_data->list_store), &iter))
        {
          do 
            {
              gtk_list_store_remove(my_data->list_store, &iter) ;
              
            } while (gtk_list_store_iter_is_valid(my_data->list_store, &iter)) ;
        }
      
      /* Add the rows. Set selected_path to the path that should be selected, i.e. that corresponds
       * to the given featureset, if any */
      GtkTreePath *selected_path = NULL ;

      if (my_data->create_child)
        {
          /* Just show the given featureset, if any */
          if (my_data->feature_set)
            {
              GtkTreeIter iter ;
              gtk_list_store_append(my_data->list_store, &iter);
              gtk_list_store_set(my_data->list_store, &iter, 0, g_quark_to_string(my_data->feature_set->original_id), -1);
            }
        }
      else
        {
          /* The list shows all affected featuresets if editing an existing style */
          for (GList *item = my_data->feature_sets ; item ; item = item->next)
            {
              ZMapFeatureSet feature_set = (ZMapFeatureSet)(item->data) ;
              
              GtkTreeIter iter ;
              gtk_list_store_append(my_data->list_store, &iter);
              gtk_list_store_set(my_data->list_store, &iter, 0, g_quark_to_string(feature_set->original_id), -1);
              
              if (feature_set == my_data->feature_set)
                selected_path = gtk_tree_model_get_path(GTK_TREE_MODEL(my_data->list_store), &iter) ;
            }
        }

      if (selected_path && my_data->tree_view)
        {
          GtkTreeSelection *selection = gtk_tree_view_get_selection(my_data->tree_view) ;
          gtk_tree_view_scroll_to_cell(my_data->tree_view, selected_path, NULL, FALSE, 0.0, 0.0) ;
          gtk_tree_selection_select_path(selection, selected_path) ;
        }
    }
}

static GtkWidget* createFeaturesetsWidget(StyleChange my_data)
{
  /* Put all the featuresets into a tree store */
  my_data->feature_sets = g_list_sort(my_data->feature_sets, featuresetsCompareFunc) ;

  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING) ;
  my_data->list_store = store ;

  /* Create the tree view */
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (store));
  my_data->tree_view = GTK_TREE_VIEW(tree) ;
  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new ();
  GtkTreeViewColumn *column = 
    gtk_tree_view_column_new_with_attributes("Featuresets", text_renderer, "text", 0, NULL);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  /* Fill in the rows in the tree store */
  updateFeaturesetListStore(my_data) ;

  /* Put the tree in a scrolled window */
  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_container_add(GTK_CONTAINER(scrolled_window), tree) ;

  return scrolled_window ;
}


/* Create the widgets that display details about the current featureset and style */
static void createInfoWidgets(StyleChange my_data, GtkTable *table, const int cols, const int rows, int *row)
{
  GtkWidget *label = NULL ;
  GtkWidget *entry = NULL ;
  const char *text = NULL ;
  const int fcol = 4 ;
  const int frow = *row ;

  /* The new style name */
  if (my_data->create_child)
    label = gtk_label_new("New style: ") ;
  else
    label = gtk_label_new("New name: ") ;

  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->new_style_name_widget = gtk_entry_new() ;
  text = g_quark_to_string(my_data->new_style_name) ;
  gtk_entry_set_text(GTK_ENTRY(entry), text ? text : "") ;
  gtk_table_attach(table, entry, 1, fcol, *row, *row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;

  /* The original/parent style name */
  if (my_data->create_child)
    label = gtk_label_new("Parent style: ") ;
  else
    label = gtk_label_new("Original name: ") ;

  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->orig_style_name_widget = gtk_entry_new() ;
  text = g_quark_to_string(my_data->orig_style_copy.original_id);
  gtk_entry_set_text(GTK_ENTRY(entry), text ? text : "") ;
  gtk_widget_set_sensitive(entry, FALSE) ;
  gtk_table_attach(table, entry, 1, fcol, *row, *row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;

  /* If editing an existing style, add an option to be able to create a new child style from it */
  if (!my_data->create_child)
    {
      GtkWidget *button = gtk_button_new_with_label("Create child style") ;
      gtk_table_attach(table, button, 0, 2, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD) ;
      g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(createChildStyleCB), my_data) ;
    }

  /* The list of the featuresets for this style */
  GtkWidget *tree = createFeaturesetsWidget(my_data) ;
  gtk_table_attach(table, tree, fcol, fcol + 1, frow, rows, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), XPAD, YPAD);
  *row += 1 ;
}


/* Create colour-button widgets for the standard feature colours */
static void createColourWidgets(StyleChange my_data, GtkTable *table, int *row)
{
  zMapReturnIfFail(my_data && my_data->window) ;

  GdkColor *fill_col = &my_data->window->canvas_background;
  GdkColor *border_col = &my_data->window->canvas_background;
  ZMapFeatureTypeStyle style = my_data->style;
  const int xpad = ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING ;
  const int ypad = ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING ;

  /* Make colour buttons. */
  zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);

  GtkWidget *label = gtk_label_new("Fill:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  GtkWidget *button = my_data->fill_widget = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), fill_col) ;
  gtk_table_attach(table, button, 1, 2, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad);

  label = gtk_label_new("Border:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 2, 3, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  button = my_data->border_widget = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), border_col) ;
  gtk_table_attach(table, button, 3, 4, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad);

  /* add a final dummy column to expand into any extra space */
  gtk_table_attach(table, gtk_label_new(""), 4, 5, *row, *row + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);

  *row += 1 ;
}


/* Create colour-button widgets for CDS colours */
static void createCDSColourWidgets(StyleChange my_data, GtkTable *table, int *row)
{
  ZMapFeatureTypeStyle style = my_data->style;
  GdkColor *fill_col = &my_data->window->canvas_background ;
  GdkColor *border_col = &my_data->window->canvas_background ;
  const int xpad = ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING ;
  const int ypad = ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING ;

  zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);
  
  GtkWidget *label = gtk_label_new("CDS Fill:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  my_data->cds_widgets = g_list_append(my_data->cds_widgets, label) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  GtkWidget *button = my_data->cds_fill_widget = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), fill_col) ;
  my_data->cds_widgets = g_list_append(my_data->cds_widgets, button) ;
  gtk_table_attach(table, button, 1, 2, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad);

  label = gtk_label_new("CDS Border:") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  my_data->cds_widgets = g_list_append(my_data->cds_widgets, label) ;
  gtk_table_attach(table, label, 2, 3, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  button = my_data->cds_border_widget = gtk_color_button_new() ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(button), border_col) ;
  my_data->cds_widgets = g_list_append(my_data->cds_widgets, button) ;
  gtk_table_attach(table, button, 3, 4, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, xpad, ypad);

  /* add a final dummy column to expand into any extra space */
  gtk_table_attach(table, gtk_label_new(""), 4, 5, *row, *row + 1, GTK_EXPAND, GTK_SHRINK, 0, 0);

  *row += 1 ;
}


/* Create widgets for other parameters */
static void createOtherWidgets(StyleChange my_data, GtkTable *table, int *row)
{
  //
  // Tick button to indicate whether features should be displayed stranded or not
  //
  GtkWidget *button = my_data->stranded = gtk_check_button_new_with_label("Stranded");
  gtk_table_attach(table, button, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;

  //
  // Text entries to enter max/min score and a button to calculate them from loaded features
  //
  GtkWidget *entry = NULL ;
  gtk_table_attach(table, gtk_label_new("Score:"), 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->min_score = gtk_entry_new() ;
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 5) ;
  gtk_table_attach(table, entry, 1, 2, *row, *row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->max_score = gtk_entry_new() ;
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 5) ;
  gtk_table_attach(table, entry, 2, 3, *row, *row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);

  GtkWidget *calc_btn = gtk_button_new_with_label("Auto") ;
  gtk_widget_set_tooltip_text(calc_btn, "Use an automatic min/max score based on all features that use this style") ;
  g_signal_connect(G_OBJECT(calc_btn), "clicked", G_CALLBACK(scoreCalulateCB), my_data) ;
  gtk_table_attach(table, calc_btn, 3, 4, *row, *row + 1, GTK_FILL, GTK_SHRINK, XPAD, YPAD);

  *row += 1 ;


#if 0
  /* if alignment... */
  button = gtk_check_button_new_with_label("Always Gapped");
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(strandedCB), my_data);
  gtk_table_attach(table, button, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;
#endif

}


/* Called when the user hits a button on the Edit Style dialog */
static void responseCB(GtkDialog *dialog, gint responseId, gpointer data)
{
  gboolean destroy = FALSE ;

  switch (responseId)
  {
    case GTK_RESPONSE_OK:
      /* Only destroy the window if applying the changes was successful */
      if (applyChanges(data))
        destroy = TRUE ;
      break ;
      
    case GTK_RESPONSE_APPLY:
      applyChanges(data) ;
      destroy = FALSE;
      break ;

    case GTK_RESPONSE_REJECT:
      revertChanges(data) ;
      destroy = FALSE ;
      break ;

    case GTK_RESPONSE_CANCEL:
      destroy = TRUE ;
      break;
      
    default:
      break;
  };
  
  if (destroy)
    {
      gtk_widget_destroy(GTK_WIDGET(dialog));
    }
}


/* Destroy the data associated with the Edit Style dialog */
static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  StyleChange my_data = (StyleChange) cb_data;

  my_data->window->style_window = NULL;

  g_free(my_data);
}


/* Callback to open a create-style dialog passing  the current style as the parent */
static void createChildStyleCB(GtkButton *button, gpointer cb_data)
{
  StyleChange my_data = (StyleChange)cb_data ;

  zMapWindowShowStyleDialog(my_data->window, my_data->style, TRUE, my_data->feature_set,
                            my_data->created_cb_func, my_data->cb_data) ;
  
}

/* Callback to calculate the score range based on features in the style */
static void scoreCalulateCB(GtkButton *button, gpointer cb_data)
{
  StyleChange my_data = (StyleChange)cb_data ;

  zmapWindowUpdateStyleFromFeatures(my_data->window, my_data->style) ;

  stringstream min_ss;
  min_ss << my_data->style->min_score ;
  gtk_entry_set_text(GTK_ENTRY(my_data->min_score), min_ss.str().c_str()) ;
      
  stringstream max_ss;
  max_ss << my_data->style->max_score ;
  gtk_entry_set_text(GTK_ENTRY(my_data->max_score), max_ss.str().c_str()) ;
}


/* Update any featuresets affected by changes to the given style */
static void updateFeaturesets(StyleChange my_data, ZMapFeatureTypeStyle style)
{
  /* redraw affected featuresets */
  if (style && my_data && my_data->window && my_data->window->feature_context)
    {
      if (my_data->create_child)
        {
          /* just assign the new style to the specified featureset, if any */
          if (my_data->feature_set)
            zmapWindowFeaturesetSetStyle(style->unique_id, my_data->feature_set, my_data->window->context_map, my_data->window);
        }
      else
        {
          /* update all affected featuresets */
          GList *featuresets_list = my_data->feature_sets ;
  
          for (GList *featureset_item = featuresets_list ; featureset_item ; featureset_item = featureset_item->next)
            {
              ZMapFeatureSet feature_set = (ZMapFeatureSet)(featureset_item->data) ;
              zmapWindowFeaturesetSetStyle(style->unique_id, feature_set, my_data->window->context_map, my_data->window);
            }
        }
    }

  zmapWindowColOrderColumns(my_data->window) ;        /* put this column (deleted then created) back into the right place */
  zmapWindowFullReposition(my_data->window->feature_root_group,TRUE, "window style") ;                /* adjust sizing and shuffle left / right */
}


/* Called when the user hits Revert on the Edit Style dialog. This throws away any changes since
 * they first opened the dialog, i.e. it reverts to the original style that was in the featureset. */
static gboolean revertChanges(gpointer cb_data)
{
  gboolean ok = TRUE ;
  StyleChange my_data = (StyleChange) cb_data;
  ZMapFeatureTypeStyle style = my_data->style;
  GQuark id;

  /* If we've created a child style, just remove the child and replace it with the parent */
  if(my_data->create_child)
    {
      ZMapStyleTree &styles = my_data->window->context_map->styles;
      id = style->parent_id;

      /* free the created style: cannot be ref'd anywhere else */
      styles.remove_style(style) ;

      zMapStyleDestroy(style);

      /* find the parent */
      style = styles.find_style(id);
    }
  else                                /* restore the existing style */
    {
      /* Overwrite the current style from the copy of the original style */
      memcpy((void *) style, (void *) &my_data->orig_style_copy, sizeof (ZMapFeatureTypeStyleStruct));
    }

  updateFeaturesets(my_data, style) ;

  /* update this dialog */
  zmapWindowStyleDialogSetStyle(my_data->window, my_data->style, my_data->feature_set, my_data->create_child);

  return ok;
}


/* Called when the user applies changes on the Edit Style dialog. It updates the featureset's
 * style, creating a new child style if necessary  */
static gboolean applyChanges(gpointer cb_data)
{
  gboolean ok = TRUE ;
  StyleChange my_data = (StyleChange) cb_data;
  ZMapStyleTree &styles = my_data->window->context_map->styles;
  GQuark new_style_id = 0 ;
  ZMapFeatureTypeStyle style = NULL ;

  /* See if the new style already exists */
  const char *new_style_name = gtk_entry_get_text(GTK_ENTRY(my_data->new_style_name_widget)) ;

  if (!new_style_name || *new_style_name == '\0')
    {
      zMapWarning("%s", "No style name given") ;
      ok = FALSE ;
    }

  if (ok)
    {
      new_style_id = zMapStyleCreateID(new_style_name) ;
      style = styles.find_style(new_style_id);

      /* If creating a new child we must not have an existing style */
      if (style && my_data->create_child)
        {
          zMapWarning("Style '%s' already exists", new_style_name) ;
          ok = FALSE ;
        }
    }

  if (ok && !my_data->create_child && !style)
    {
      /* If the style wasn't found, we must be renaming it. Use the original style struct */
      style = my_data->style ;
    }

  if (!style && !my_data->create_child)
    {
      /* Shouldn't get here because my_data->style should be set, but double check */
      zMapWarning("%s", "No style to edit") ;
      ok = FALSE ;
    }

  if (ok)
    {
      /* Check whether we've already applied changes to this style */
      const gboolean already_edited_style = (new_style_id == my_data->last_edited_style_id) ;

      /* We "own" this style if there's only a single featureset affected by it (and it's the
       * same as the given featureset, if specified) */
      gboolean own_style = TRUE ;

      if (!style)
        own_style = TRUE ; /* creating new style so ok */
      else if (!my_data->feature_sets)
        own_style = TRUE ; /* not affecting any styles so ok */
      else if (g_list_length(my_data->feature_sets) < 1)
        own_style = TRUE ; /* not affecting any styles so ok */
      else if (g_list_length(my_data->feature_sets) > 1)
        own_style = FALSE ; /* affecting more than 1 style so not ok */
      else if (!my_data->feature_set)
        own_style = TRUE ; /* only affecting 1 style and unspecified which so ok */
      else if (my_data->feature_sets->data == my_data->feature_set)
        own_style = TRUE ; /* only affecting the specified style so ok */
      else 
        own_style = FALSE ; /* affecting a style different to the specified one so not ok */

      /* We make a new child style if the style doesn't already exist */
      if (!style)
        {
          /* make new style with the specified name and merge in the parent */
          ZMapFeatureTypeStyle parent = my_data->style;
          ZMapFeatureTypeStyle tmp_style;

          style = zMapStyleCreate(new_style_name, new_style_name);

          /* merge is written upside down
           * we have to copy the parent and merge the child onto it
           * then delete the style we just created
           */

          tmp_style = zMapFeatureStyleCopy(parent) ;

          if (zMapStyleMerge(tmp_style, style))
            {
              if (parent)
                tmp_style->parent_id = parent->unique_id ;

              styles.remove_style(style) ;
              zMapStyleDestroy(style);

              styles.add_style(tmp_style) ;
              style = tmp_style;

              g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_PARENT_STYLE, parent->unique_id , NULL);
            }
          else
            {
              zMapWarning("Failed to create new style '%s'", new_style_name);
              zMapStyleDestroy(tmp_style);
              zMapStyleDestroy(style);
              ok = FALSE;
            }
        }
      else if (!own_style && !already_edited_style)
        {
          /* We're overwriting a style that isn't "owned" by this featureset. The user may
           * have accidentally entered a style name that already exists - check if they want
           * to overwrite it. Only do this if they haven't already confirmed that it's ok! */
          char *msg = g_strdup_printf("You are changing a style that affects multiple featuresets.\n\nAre you sure you want to overwrite it?") ;

          ok = zMapGUIMsgGetBool(GTK_WINDOW(my_data->toplevel), ZMAP_MSG_INFORMATION, msg) ;

          g_free(msg) ;
        }
    }

  if (ok)
    {
      /* Remember that we've edited this style */
      my_data->last_edited_style_id = new_style_id ;

      /* Set the new style name, if necessary */
      renameStyle(my_data, style, new_style_name, new_style_id) ;

      /* apply the chosen colours etc */
      updateStyle(my_data, style);

      /* set the style
       * if we are only changing colours we could be more efficient
       * but this will work for all cases and means no need to write more code
       * we do this _once_ at user/ mouse click speed
       */
      updateFeaturesets(my_data, style) ;

      if (my_data->create_child && my_data->created_cb_func)
        my_data->created_cb_func(style, my_data->cb_data) ;

      /* If the user continues to edit they're now editing an existing style, so reset the
       * dialog contents to reflect that */
      zMapWindowShowStyleDialog(my_data->window, style, FALSE, my_data->feature_set,
                                my_data->created_cb_func, my_data->cb_data) ;
      }

  return ok;
}


/* Utility to get the colors from the color buttons. Returns true if ok. */
static gboolean getColors(GtkWidget *fill_widget, GtkWidget *border_widget,
                          GdkColor *fill, GdkColor *border)
{
  gboolean ok = FALSE ;

  if (fill_widget && border_widget && 
      GTK_IS_COLOR_BUTTON(fill_widget) && GTK_IS_COLOR_BUTTON(border_widget))
    {
      if (fill)
        gtk_color_button_get_color(GTK_COLOR_BUTTON(fill_widget), fill) ;

      if (border)
        gtk_color_button_get_color(GTK_COLOR_BUTTON(border_widget), border) ;

      ok = TRUE ;
    }

  return ok ;
}

/* Set a new name/id for the given style */
static void renameStyle(StyleChange my_data, ZMapFeatureTypeStyle style, 
                        const char *new_style_name, const GQuark new_style_id)
{
  zMapReturnIfFail(style && new_style_name && new_style_id) ;

  style->original_id = g_quark_from_string(new_style_name) ;
  style->unique_id = new_style_id ;
}


/* Set the properties in the given style from the user-selected options from the dialog */
static void updateStyle(StyleChange my_data, ZMapFeatureTypeStyle style)
{
  /* Set whether strand-specific */
  gboolean stranded = gtk_toggle_button_get_active((GtkToggleButton *) my_data->stranded) ;
  g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, stranded, NULL);

  /* Set the min/max score. Note that these will be 0.0 if an invalid string is given */
  const char *txt = gtk_entry_get_text(GTK_ENTRY(my_data->min_score)) ;
  double min_score = strtod(txt, NULL) ;
  g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_MIN_SCORE, min_score, NULL);

  txt = gtk_entry_get_text(GTK_ENTRY(my_data->max_score)) ;
  double max_score = strtod(txt, NULL) ;
  g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_MAX_SCORE, max_score, NULL);

  /* Set the colours */
  GdkColor fill, border ;

  if (getColors(my_data->fill_widget, my_data->border_widget, &fill, &border))
    {
      zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                          &fill, NULL, &border) ;
    }


  /* Set the CDS colours, if it's a transcript */
  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT &&
     getColors(my_data->cds_fill_widget, my_data->cds_border_widget, &fill, &border))
    {
      zMapStyleSetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
                          &fill, NULL, &border) ;
    }

}

