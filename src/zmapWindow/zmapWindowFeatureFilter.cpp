/*  File: zmapWindowFeatureFilter.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: dialog for filtering column features against other
 *              features using various criteria.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <ZMap/zmapString.hpp>
#include <zmapWindowContainerBlock.hpp>
#include <zmapWindow_P.hpp>


typedef struct FilterDataStructType
{
  ZMapWindow window ;
  ZMapWindowContainerBlock block_container ;
  
  ZMapWindowCallbackCommandFilter command_filter_data ;

  GtkWidget *top_level ;
  GtkWidget *filter_label ;
  GtkWidget *filter_frame ;
  GtkWidget *match_type_combo ;
  GtkWidget *bases_spin_button ;
  GtkWidget *selected_part_combo ;
  GtkWidget *feature_part_combo ;
  GtkWidget *feature_action_combo ;
  GtkWidget *feature_target_combo ;

  GtkWidget *column_combo ;

  int column_count ;

} FilterDataStruct, *FilterData ;

// min/max/incr for spin button to retrieve +/- base slop factor to be used in matching.   
#define BASES_MIN   0.0
#define BASES_MAX  50.0
#define BASES_INCR  1.0

static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void resetFilterCB(GtkWidget *widget, gpointer cb_data) ;
static void allCB(GtkWidget *widget, gpointer cb_data) ;
static void columnChangedCB(GtkComboBox *widget,  gpointer user_data) ;
static void closeCB(GtkWidget *widget, gpointer cb_data) ;
static void unfilterCB(GtkWidget *widget, gpointer cb_data) ;
static void filterCB(GtkWidget *widget, gpointer cb_data) ;
static void makeColumnCombo(FilterData filter_data) ;
static void makeComboEntryCB(gpointer data, gpointer user_data) ;

static void setNewTargetCol(FilterData filter_data) ;
static const char *getFrameStr(const char *target_str, const char *target_column_str,
                               const char *filter_str, const char *filter_column_str) ;



// 
//                Globals
//    



// 
//                External routines.
//    




// 
//                Package routines.
//    

void zmapWindowCreateFeatureFilterWindow(ZMapWindow window, ZMapWindowCallbackCommandFilter command_filter_data)
{
  GtkWidget *toplevel, *top_vbox, *vbox, *hbox, *frame, *reset_hbox ;
  GtkWidget *label, *combo = NULL, *spin, *entry, *separator ;
  GtkTable *table ;
  GtkWidget *button_box, *close_button, *focus_button, *all_button, *unfilter_button, *filter_button ;
  FilterData filter_data ;
  const char *title = "Feature Filtering" ;
  const char *frame_str ;
  guint col1_left_attach, col1_right_attach, col2_left_attach, col2_right_attach, top_attach, bottom_attach ;


  filter_data = g_new0(FilterDataStruct, 1) ;

  filter_data->window = window ;
  filter_data->command_filter_data = command_filter_data ;
  filter_data->block_container
    = ZMAP_CONTAINER_BLOCK(zmapWindowContainerUtilsItemGetParentLevel(FOO_CANVAS_ITEM(command_filter_data->filter_column),
                                                                      ZMAPCONTAINER_LEVEL_BLOCK)) ;

  filter_data->top_level = toplevel = zMapGUIToplevelNew(NULL, title) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy", GTK_SIGNAL_FUNC(destroyCB), filter_data) ;

  top_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), top_vbox) ;


  // Top frame showing what's being filtered.
  frame = gtk_frame_new("Current filter/target features:") ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_container_add(GTK_CONTAINER(top_vbox), frame) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;


  frame_str = getFrameStr(command_filter_data->target_str, command_filter_data->target_column_str,
                          command_filter_data->filter_str, command_filter_data->filter_column_str) ;

  filter_data->filter_label = label = gtk_label_new(frame_str) ;
  gtk_container_add(GTK_CONTAINER(vbox), label) ;

  g_free((void *)frame_str) ;


  // Frame with all filtering options.
  filter_data->filter_frame = frame = gtk_frame_new("Change filter options:") ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, TRUE, TRUE, 0) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 5) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  // Table of labels/combo boxes for filtering options.
  table = (GtkTable *)gtk_table_new(4, 2, FALSE) ;
  gtk_table_set_row_spacings(table, 10) ;
  gtk_table_set_col_spacings(table, 10) ;
  gtk_container_add(GTK_CONTAINER(vbox), (GtkWidget *)table) ;

  // There's a lot of row/column shenanigans, easier to change with some variables.
  col1_left_attach = 1 ;
  col1_right_attach = 2 ;
  col2_left_attach = 2 ;
  col2_right_attach = 3 ;
  top_attach = 1 ;
  bottom_attach = 2 ;

  label = gtk_label_new("Type of Match:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;

  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;

  filter_data->match_type_combo = combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetMatchTypeEnum2LongText(ZMAP_CANVAS_MATCH_PARTIAL));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetMatchTypeEnum2LongText(ZMAP_CANVAS_MATCH_FULL));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;
  gtk_table_attach_defaults(table, combo, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;


  top_attach++ ;
  bottom_attach++ ;

  label = gtk_label_new("+/- Bases for Matching:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;
  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;
  filter_data->bases_spin_button = spin = gtk_spin_button_new_with_range(BASES_MIN, BASES_MAX, BASES_INCR) ;
  gtk_table_attach_defaults(table, spin, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;


  top_attach++ ;
  bottom_attach++ ;

  label = gtk_label_new("Part of Filter Feature to match from:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;
  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;

  filter_data->selected_part_combo = combo = gtk_combo_box_new_text() ;
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_PARTS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_GAPS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_CDS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_FEATURE));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;
  gtk_table_attach_defaults(table, combo, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;


  top_attach++ ;
  bottom_attach++ ;

  label = gtk_label_new("Part of Target Features to match to:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;
  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;

  filter_data->feature_part_combo = combo = gtk_combo_box_new_text() ;
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_PARTS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_GAPS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_CDS));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetFilterTypeEnum2LongText(ZMAP_CANVAS_FILTER_FEATURE));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;
  gtk_table_attach_defaults(table, combo, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;

  top_attach++ ;
  bottom_attach++ ;

  label = gtk_label_new("Action to perform on Features:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;
  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;

  filter_data->feature_action_combo = combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetActionTypeEnum2LongText(ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetActionTypeEnum2LongText(ZMAP_CANVAS_ACTION_HIDE));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetActionTypeEnum2LongText(ZMAP_CANVAS_ACTION_SHOW));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;
  gtk_table_attach_defaults(table, combo, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;

  top_attach++ ;
  bottom_attach++ ;

  label = gtk_label_new("Features/Columns to filter:") ;
  gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5) ;
  gtk_table_attach_defaults(table, label, col1_left_attach, col1_right_attach, top_attach, bottom_attach) ;

  filter_data->feature_target_combo = combo = gtk_combo_box_new_text();
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetTargetTypeEnum2LongText(ZMAP_CANVAS_TARGET_ALL));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetTargetTypeEnum2LongText(ZMAP_CANVAS_TARGET_NOT_FILTER_COLUMN));
  gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
                            zMapWindowContainerFeatureSetTargetTypeEnum2LongText(ZMAP_CANVAS_TARGET_NOT_FILTER_FEATURES));
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0) ;
  gtk_table_attach_defaults(table, combo, col2_left_attach, col2_right_attach, top_attach, bottom_attach) ;


  // Frame with reset filter/target feature controls
  reset_hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(top_vbox), reset_hbox, TRUE, TRUE, 0) ;

  frame = gtk_frame_new("Change filter feature:") ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(reset_hbox), frame, TRUE, TRUE, 0) ;

  hbox = gtk_hbox_new(FALSE, 5) ;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 5) ;
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;

  
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  // THIS DOESN'T WORK...I THINK IT'S POSSIBLE TO HAVE A STOCK IMAGE
  // WITH YOUR OWN TEXT BUT IT'S SOME WORK.....WRITE A FUNCTION LATER...
  focus_button = gtk_button_new_from_stock(GTK_STOCK_ADD) ;
  GtkWidget *image ;
  
  image = gtk_button_get_image (GTK_BUTTON(focus_button)) ;

  gtk_button_set_label(GTK_BUTTON(focus_button), "Re-focus") ;


  gtk_button_set_image (GTK_BUTTON(focus_button),
                      image);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  focus_button = gtk_button_new_with_label("Set to current highlight") ;
  gtk_container_add(GTK_CONTAINER(hbox), focus_button) ;
  gtk_signal_connect(GTK_OBJECT(focus_button), "clicked", GTK_SIGNAL_FUNC(resetFilterCB), filter_data) ;


  frame = gtk_frame_new("Change target features:") ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(reset_hbox), frame, TRUE, TRUE, 0) ;

  hbox = gtk_hbox_new(FALSE, 5) ;
  gtk_container_set_border_width(GTK_CONTAINER(hbox), 5) ;
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;


  all_button = gtk_toggle_button_new_with_label("Filter All Columns") ;
  gtk_container_add(GTK_CONTAINER(hbox), all_button) ;
  gtk_signal_connect(GTK_OBJECT(all_button), "clicked", GTK_SIGNAL_FUNC(allCB), filter_data) ;

  separator = gtk_vseparator_new() ;
  gtk_container_add(GTK_CONTAINER(hbox), separator) ;
 
  combo = filter_data->column_combo = gtk_combo_box_new_text() ;
  makeColumnCombo(filter_data) ;
  gtk_container_add(GTK_CONTAINER(hbox), combo) ;
  gtk_signal_connect(GTK_OBJECT(combo), "changed", GTK_SIGNAL_FUNC(columnChangedCB), filter_data) ;


  // Frame with Close button etc. across the bottom.   
  frame = gtk_frame_new(NULL) ;
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(top_vbox), frame, TRUE, TRUE, 0) ;

  button_box = gtk_hbutton_box_new();
  gtk_container_add(GTK_CONTAINER(frame), button_box) ;
  gtk_box_set_spacing(GTK_BOX(button_box), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width(GTK_CONTAINER (button_box), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  close_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
  gtk_box_pack_start(GTK_BOX(button_box), close_button, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(close_button), "clicked", GTK_SIGNAL_FUNC(closeCB), filter_data);

  unfilter_button = gtk_button_new_with_label("Unfilter") ;
  gtk_box_pack_start(GTK_BOX(button_box), unfilter_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(unfilter_button), "clicked", GTK_SIGNAL_FUNC(unfilterCB), filter_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  filter_button = gtk_button_new_from_stock(GTK_STOCK_APPLY) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  filter_button = gtk_button_new_with_label("Filter") ;
  gtk_box_pack_end(GTK_BOX(button_box), filter_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(filter_button), "clicked", GTK_SIGNAL_FUNC(filterCB), filter_data) ;
  GTK_WIDGET_SET_FLAGS(filter_button, GTK_CAN_DEFAULT) ;    /* set filter button as default. */
  gtk_window_set_default(GTK_WINDOW(toplevel), filter_button) ;


  gtk_widget_show_all(toplevel) ;

  return ;
  
}



// 
//                Internal routines.
//    

// Called when user closes window and indirectly when they click "Close" button.
static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;

  g_free(filter_data) ;

  return ;
}

// Close button.
static void closeCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;

  // Results in destroyCB() being called for cleanup.
  gtk_widget_destroy(filter_data->top_level) ;

  return ;
}

// Reset filter feature button.
static void resetFilterCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;
  FooCanvasGroup *focus_column = NULL ;
  FooCanvasItem *focus_item = NULL ;
  GList *filterfeatures = NULL ;
  const char *frame_str ;
  char *err_msg = NULL ;


  if (!zmapWindowFocusGetFeatureListFull(filter_data->window, TRUE,
                                         &focus_column, &focus_item, &filterfeatures,
                                         &err_msg))
    {
      zMapWarning("%s", err_msg) ;
      
      g_free(err_msg) ;
    }
  else
    {
      ZMapFeature feature ;
      

      feature = (ZMapFeature)(filterfeatures->data) ;
      command_filter_data->filter_column_str = zMapFeatureName(feature->parent) ;
      if (g_list_length(filterfeatures) == 1)
        {
          command_filter_data->filter_str = zMapFeatureName((ZMapFeatureAny)feature) ;
        }
      else
        {
          command_filter_data->filter_str = "all" ;
        }
    }
  
  frame_str = getFrameStr(command_filter_data->target_str, command_filter_data->target_column_str,
                          command_filter_data->filter_str, command_filter_data->filter_column_str) ;

  gtk_label_set_text(GTK_LABEL(filter_data->filter_label), frame_str) ;
  
  g_free((void *)frame_str) ;
  
  command_filter_data->filter_features = filterfeatures ;

  return ;
}

// Called when the "Filter all columns" button is toggled.   
static void allCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;

      // need to reset title......   !!!!!


  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    {
      const char *frame_str ;

      command_filter_data->target_column_str = "all" ;
      command_filter_data->target_str = "all" ;

      frame_str = getFrameStr(command_filter_data->target_str, command_filter_data->target_column_str,
                              command_filter_data->filter_str, command_filter_data->filter_column_str) ;

      gtk_label_set_text(GTK_LABEL(filter_data->filter_label), frame_str) ;
  
      g_free((void *)frame_str) ;

      command_filter_data->target_column = NULL ;           // Trigger for "do all cols"

      gtk_widget_set_sensitive(filter_data->column_combo, FALSE) ;
    }
  
  else
    {
      setNewTargetCol(filter_data) ;
      
      gtk_widget_set_sensitive(filter_data->column_combo, TRUE) ;  
    }
  

  return ;
}




// Unfilter current target features.
static void unfilterCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;
  ZMapWindowCallbacks window_cbs_G ;

  filter_data->command_filter_data->selected = ZMAP_CANVAS_FILTER_NONE ;

  filter_data->command_filter_data->filter = ZMAP_CANVAS_FILTER_NONE ;

  /* send command up to view so it can be run on all windows for this view.... */
  window_cbs_G = zmapWindowGetCBs() ;

  (*(window_cbs_G->command))(filter_data->window, filter_data->window->app_data, filter_data->command_filter_data) ;

  return ;
}


// Filter the features, gets all user selected options and applies
// them to the current filter/target features.
static void filterCB(GtkWidget *widget, gpointer cb_data)
{
  FilterData filter_data = (FilterData)cb_data ;
  ZMapWindow window = filter_data->window ;
  ZMapWindowCallbacks window_cbs_G ;
  char *match_txt, *selected_txt, *filter_txt, *action_txt , *target_txt ;
  ZMapWindowContainerMatchType match_type ;
  ZMapWindowContainerFilterType selected_type ;
  ZMapWindowContainerFilterType filter_type ;
  ZMapWindowContainerActionType action_type ;
  ZMapWindowContainerTargetType target_type ;


  // Retrieve all the users filter settings.
  match_txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->match_type_combo)) ;
  selected_txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->selected_part_combo)) ;
  filter_txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->feature_part_combo)) ;
  action_txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->feature_action_combo)) ;
  target_txt = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->feature_target_combo)) ;

  match_type = zMapWindowContainerFeatureSetMatchTypeLongText2Enum(match_txt) ;
  selected_type = zMapWindowContainerFeatureSetFilterTypeLongText2Enum(selected_txt) ;
  filter_type = zMapWindowContainerFeatureSetFilterTypeLongText2Enum(filter_txt) ;
  action_type = zMapWindowContainerFeatureSetActionTypeLongText2Enum(action_txt) ;
  target_type = zMapWindowContainerFeatureSetTargetTypeLongText2Enum(target_txt) ;

  filter_data->command_filter_data->match_type = match_type ;
  
  filter_data->command_filter_data->selected = selected_type ;

  filter_data->command_filter_data->filter = filter_type ;
  
  filter_data->command_filter_data->action = action_type ;

  filter_data->command_filter_data->target_type = target_type ;

  filter_data->command_filter_data->base_allowance
    = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(filter_data->bases_spin_button)) ;

  /* send command up to view so it can be run on all windows for this view.... */
  window_cbs_G = zmapWindowGetCBs() ;

  (*(window_cbs_G->command))(window, window->app_data, filter_data->command_filter_data) ;


  // Needs revisiting and combining in some way with identical code in zmapWindowMenus.cpp   
  /* Hacky showing of errors, may not work when there are multiple windows.....
   * needs wider fix to window command interface....errors should be reported in view
   * for each window as it's processed... */
  if (!filter_data->command_filter_data->result)
    {
      const char *error, *err_msg ;

      if (filter_data->command_filter_data->filter_result == ZMAP_CANVAS_FILTER_NOT_SENSITIVE)
        error = "is non-filterable (check styles configuration file)." ;
      else
        error = "have no matches" ;

      err_msg = g_strdup_printf("%s: %s", filter_data->command_filter_data->target_str, error) ;

      zMapMessage("%s", err_msg) ;

      g_free((void *)err_msg) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  // I THINK WE DON'T WANT TO DO THIS....AS USER MAY TRY A NEW FILTER....AGH, NOT SO EASY AS WE
  // MAY NEED TO REDO ALL SORTS OF FIELDS IF USER HAS CHANGED WHICH FEATURE IS HIGHLIGHTED ETC ETC.   

  // THIS ALL NEEDS A LOT MORE THOUGHT......   

  g_free(filter_data->command_filter_data) ;
  g_free(filter_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


// Make the column choice combo.
static void makeColumnCombo(FilterData filter_data)
{
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;
  GList *column_ids ;
  gboolean unique = FALSE ;
  ZMapStrand strand = ZMAPSTRAND_FORWARD ;
  gboolean visible = TRUE ;
      
  if ((column_ids = zmapWindowContainerBlockColumnList(ZMAP_CONTAINER_BLOCK(filter_data->block_container),
                                                       unique, strand, visible)))
    {
      filter_data->column_count = 0 ;

      g_list_foreach(column_ids, makeComboEntryCB, filter_data) ;
    }

  return ;
}

// called by makeColumnCombo() for each column entry.
static void makeComboEntryCB(gpointer data, gpointer user_data)
{
  GQuark col_id = GPOINTER_TO_INT(data) ;
  FilterData filter_data = (FilterData)user_data ;
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;
  const char *col_str, *set_str ;

  col_str = g_quark_to_string(col_id) ;
  set_str = command_filter_data->target_column_str ;

  gtk_combo_box_append_text(GTK_COMBO_BOX(filter_data->column_combo), col_str) ;

  if (g_ascii_strcasecmp(col_str, set_str) == 0)
    {
      gtk_combo_box_set_active(GTK_COMBO_BOX(filter_data->column_combo), filter_data->column_count) ;
    }

  (filter_data->column_count)++ ;

  return ;
}

// Called when user changes the Target Column.
static void columnChangedCB(GtkComboBox *widget,  gpointer user_data)
{
  FilterData filter_data = (FilterData)user_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;
  char *col_text ;
  const char *frame_str ;
  ZMapWindowContainerFeatureSet target_col ;

  col_text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->column_combo)) ;

  command_filter_data->target_column_str = col_text ;

  frame_str = getFrameStr(command_filter_data->target_str, command_filter_data->target_column_str,
                          command_filter_data->filter_str, command_filter_data->filter_column_str) ;

  gtk_label_set_text(GTK_LABEL(filter_data->filter_label), frame_str) ;
  
  g_free((void *)frame_str) ;

  // reset target column...........
  if (target_col = zmapWindowContainerBlockFindColumn(filter_data->block_container,
                                                      command_filter_data->target_column_str,
                                                      zmapWindowContainerFeatureSetGetStrand(command_filter_data->target_column)))
    command_filter_data->target_column = target_col ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  setNewTargetCol(filter_data) ;
  

  return ;
}


static void setNewTargetCol(FilterData filter_data)
{
  ZMapWindowCallbackCommandFilter command_filter_data = filter_data->command_filter_data ;
  char *col_text ;
  const char *frame_str ;
  ZMapWindowContainerFeatureSet target_col ;
  ZMapStrand target_strand ;

  col_text = gtk_combo_box_get_active_text(GTK_COMBO_BOX(filter_data->column_combo)) ;

  command_filter_data->target_column_str = col_text ;

  frame_str = getFrameStr(command_filter_data->target_str, command_filter_data->target_column_str,
                          command_filter_data->filter_str, command_filter_data->filter_column_str) ;

  gtk_label_set_text(GTK_LABEL(filter_data->filter_label), frame_str) ;
  
  g_free((void *)frame_str) ;


  if ((command_filter_data->target_column))
    {
      target_strand = zmapWindowContainerFeatureSetGetStrand(command_filter_data->target_column) ;
    }
  else
    {
      ZMapFeature feature ;

      feature = (ZMapFeature)(command_filter_data->filter_features->data) ;

      target_strand = feature->strand ;
    }
  

  // reset target column...........
  if (target_col = zmapWindowContainerBlockFindColumn(filter_data->block_container,
                                                      command_filter_data->target_column_str,
                                                      target_strand))
    command_filter_data->target_column = target_col ;

  return ;
}





// Standard string showing what's filtered against what, g_free() when finished with.   
static const char *getFrameStr(const char *target_str, const char *target_column_str,
                               const char *filter_str, const char *filter_column_str)
{
  const char *frame_str ;
  char *tmp_str ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (strlen(filter_str) > STR_ABBREV_MAX_LEN)
    filter_str = tmp_str = (char *)zMapStringAbbrevStr(filter_str, NULL, STR_ABBREV_MAX_LEN) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if ((tmp_str = (char *)zMapStringAbbrevStr(filter_str)))
    filter_str = tmp_str ;

  frame_str = g_strdup_printf("\"%s\" feature(s) in target column \"%s\" "
                              "against filter feature \"%s\" in column \"%s\"",
                              target_str, target_column_str, filter_str, filter_column_str) ;

  if (tmp_str)
    g_free(tmp_str) ;

  return frame_str ;
}



