/*  File: zmapWindowStyle.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
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

#include <string.h>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp> /* zMap_g_hash_table_nth */
#include <zmapWindow_P.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>

#define XPAD 3
#define YPAD 3

typedef struct
{
  ItemMenuCBData menu_data;               /* which featureset etc */
  ZMapFeatureTypeStyleStruct orig_style_copy;  /* copy of original, used for Revert */

  GQuark last_edited_style_id;            /* if we've already changed a style remember its id here */
  gboolean refresh;                       /* clicked on another column */

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

  GtkWidget *stranded;


} StyleChangeStruct, *StyleChange;


static void createToplevel(StyleChange my_data) ;
static void createInfoWidgets(StyleChange my_data, GtkTable *table, const int cols, int *row) ;
static void createColourWidgets(StyleChange my_data, GtkTable *table, int *row) ;
static void createCDSColourWidgets(StyleChange my_data, GtkTable *table, int *row) ;
static void createOtherWidgets(StyleChange my_data, GtkTable *table, int *row) ;

static void responseCB(GtkDialog *dialog, gint responseId, gpointer data) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;

static gboolean applyChanges(gpointer cb_data) ;
static gboolean revertChanges(gpointer cb_data) ;
static void updateStyle(StyleChange my_data, ZMapFeatureTypeStyle style) ;


/* Entry point to display the Edit Style dialog */
void zmapWindowShowStyleDialog( ItemMenuCBData menu_data )
{
  StyleChange my_data = g_new0(StyleChangeStruct,1);

  /* Check if the dialog data has already been created - if so, nothing to do */
  if(zmapWindowStyleDialogSetFeature(menu_data->window, menu_data->item, menu_data->feature))
    return;

  my_data->menu_data = menu_data;
  ZMapFeatureTypeStyle style = menu_data->feature_set->style;

  /* Add ptr so parent knows about us */
  my_data->menu_data->window->style_window = (gpointer) my_data ;

  /* set up the top level dialog window */
  createToplevel(my_data) ;

  /* Create a table to lay out all the widgets in */
  const int rows = 6 ;
  const int cols = 5 ;

  GtkTable *table = GTK_TABLE(gtk_table_new(rows, cols, FALSE)) ;

  GtkWidget *top_vbox = GTK_DIALOG(my_data->toplevel)->vbox ;
  gtk_box_pack_start(GTK_BOX(top_vbox), GTK_WIDGET(table), TRUE, TRUE, ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH) ;

  /* Create the content */
  int row = 0 ;
  createInfoWidgets(my_data, table, cols, &row) ;
  createColourWidgets(my_data, table, &row) ;
  createCDSColourWidgets(my_data, table, &row) ;
  createOtherWidgets(my_data, table, &row) ;

  gtk_widget_show_all(my_data->toplevel) ;

  if(style->mode != ZMAPSTYLE_MODE_TRANSCRIPT)        /* hide CDS widgets */
    {
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_hide_all, NULL) ;
    }

  zmapWindowStyleDialogSetFeature(menu_data->window, menu_data->item, menu_data->feature);

  return ;
}


/* Interface to allow external callers to destroy the style dialog */
void zmapWindowStyleDialogDestroy(ZMapWindow window)
{
  StyleChange my_data = (StyleChange) window->style_window;

  gtk_widget_destroy(my_data->toplevel);
  window->style_window = NULL;
}


/* This updates the info in the Edit Style dialog with the given feature, i.e. it takes all of the
 * style info from the given feature's style. It's called when the dialog is first opened or if
 * the user selects a different feature while the dialog is still open. */
gboolean zmapWindowStyleDialogSetFeature(ZMapWindow window, FooCanvasItem *foo, ZMapFeature feature)
{
  StyleChange my_data = (StyleChange) window->style_window;
  ZMapFeatureSet feature_set = (ZMapFeatureSet) feature->parent;
  ZMapFeatureTypeStyle style;
  GdkColor colour = {0} ;
  GdkColor *fill_col = &colour, *border_col = &colour;
  GQuark default_style_name ;

  if(!my_data)
    return FALSE;

  my_data->menu_data->item = foo;
  my_data->menu_data->feature_set = feature_set;
  my_data->menu_data->feature = feature;
  style = feature_set->style;

  /* Make a copy of the original style so we can revert any changes we make */
  memcpy(&my_data->orig_style_copy, style,sizeof (ZMapFeatureTypeStyleStruct));

  /* Update the featureset name */
  const char *text = g_quark_to_string(feature_set->original_id) ;
  gtk_entry_set_text(GTK_ENTRY(my_data->featureset_name_widget), text ? text : "") ;

  /* Update the default style name */
  if (style->unique_id == feature_set->unique_id)
    {
      /* The current style name is the same as the featureset name. This means that this
       * featureset probably "owns" this style, so by default we want to overwrite it */
      default_style_name = style->unique_id;
    }
  else
    {
      /* The current style name is different to the featureset name. This means that this
       * featureset probably DOESN'T own the style and therefore we want to create a new child
       * style named after the featureset name */
      default_style_name = feature_set->original_id;
    }

  text = g_quark_to_string(style->unique_id) ;
  gtk_entry_set_text(GTK_ENTRY(my_data->orig_style_name_widget), text ? text : "");

  text = g_quark_to_string(default_style_name) ;
  gtk_entry_set_text(GTK_ENTRY(my_data->new_style_name_widget), text ? text : "");
  
  /* Update the colour buttons. */
  zMapStyleGetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);

  gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->fill_widget), fill_col) ;
  gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->border_widget), border_col) ;

  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      zMapStyleGetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, &border_col);

      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_fill_widget), fill_col) ;
      gtk_color_button_set_color(GTK_COLOR_BUTTON(my_data->cds_border_widget), border_col) ;
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_show_all, NULL) ;
    }
  else
    {
      g_list_foreach(my_data->cds_widgets, (GFunc)gtk_widget_hide_all, NULL) ;
    }

  gtk_toggle_button_set_active((GtkToggleButton *) my_data->stranded, style->strand_specific);

  return TRUE;
}


/* Sets the style in the given feature_set to the style given by style_id  */
void zmapWindowFeaturesetSetStyle(GQuark style_id, 
                                  ZMapFeatureSet feature_set,
                                  ZMapFeatureContextMap context_map,
                                  ZMapWindow window)
{
  ZMapFeatureTypeStyle style;
  FooCanvasItem *set_item, *canvas_item;
  int set_strand, set_frame;
  ID2Canvas id2c;
  int ok = FALSE;

  style = (ZMapFeatureTypeStyle)g_hash_table_lookup( context_map->styles, GUINT_TO_POINTER(style_id));

  if(style)
    {
      ZMapFeatureColumn column;
      ZMapFeatureSource s2s;
      ZMapFeatureSetDesc f2c;
      GList *c2s;

      /* now tweak featureset & column styles in various places */
      s2s = (ZMapFeatureSource)g_hash_table_lookup(context_map->source_2_sourcedata,GUINT_TO_POINTER(feature_set->unique_id));
      f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(context_map->featureset_2_column,GUINT_TO_POINTER(feature_set->unique_id));
      if(s2s && f2c)
        {
          s2s->style_id = style->unique_id;
          column = (ZMapFeatureColumn)g_hash_table_lookup(context_map->columns,GUINT_TO_POINTER(f2c->column_id));
          if(column)
            {

              c2s = (GList *)g_hash_table_lookup(context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
              if(c2s)
                {
                  g_list_free(column->style_table);
                  column->style_table = NULL;
                  column->style = NULL;        /* must clear this to trigger style table calculation */
                  /* NOTE column->style_id is column specific not related to a featureset, set by config */

                  for( ; c2s; c2s = c2s->next)
                    {
                      if(GPOINTER_TO_UINT(c2s->data) == feature_set->style->unique_id)
                        {
                          c2s->data = GUINT_TO_POINTER(style->unique_id);
                          break;
                        }
                    }
                  ok = TRUE;
                }

              zMapWindowGetSetColumnStyle(window, feature_set->unique_id);
              window->flags[ZMAPFLAG_CHANGED_FEATURESET_STYLE] = TRUE ;
            }
        }
    }

  if(!ok)
    {
      zMapWarning("cannot set new style","");
      return;
    }


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


  feature_set->style = style;

  zmapWindowRedrawFeatureSet(window, feature_set);        /* does a complex context thing */

  zmapWindowColOrderColumns(window) ;        /* put this column (deleted then created) back into the right place */
  zmapWindowFullReposition(window->feature_root_group,TRUE, "window style") ;                /* adjust sizing and shuffle left / right */

  return ;
}


/************ INTERNAL FUNCTIONS **************/

/* Utility function to create the top level dialog */
static void createToplevel(StyleChange my_data)
{
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
  gtk_window_set_default_size(GTK_WINDOW(my_data->toplevel), 500, -1) ;
  gtk_dialog_set_default_response(GTK_DIALOG(my_data->toplevel), GTK_RESPONSE_APPLY) ;
}


/* Create the widgets that display details about the current featureset and style */
static void createInfoWidgets(StyleChange my_data, GtkTable *table, const int cols, int *row)
{
  ZMapFeatureTypeStyle style = my_data->menu_data->feature_set->style;
  ZMapFeatureSet feature_set = my_data->menu_data->feature_set ;
  GQuark default_style_name = 0 ;

  /* The name of the featureset being edited (not editable) */
  GtkWidget *label = gtk_label_new("Featureset: ") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  GtkWidget *entry = my_data->featureset_name_widget = gtk_entry_new() ;

  const char *text = g_quark_to_string(feature_set->original_id) ;
  gtk_entry_set_text(GTK_ENTRY(entry), text ? text : "") ;
  gtk_widget_set_sensitive(entry, FALSE) ;
  gtk_table_attach(table, entry, 1, cols, *row, *row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;

  /* The name of the style to edit (editable) */
  if (style->unique_id == my_data->menu_data->feature_set->unique_id)
    {
      /* The current style name is the same as the featureset name. This means that this
       * featureset probably "owns" this style, so by default we want to overwrite it */
      default_style_name = style->unique_id;
    }
  else
    {
      /* The current style name is different to the featureset name. This means that this
       * featureset probably DOESN'T own the style and therefore we want to create a new child
       * style named after the featureset name */
      default_style_name = my_data->menu_data->feature_set->original_id;
    }

  label = gtk_label_new("Original style: ") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->orig_style_name_widget = gtk_entry_new() ;
  text = g_quark_to_string(my_data->orig_style_copy.unique_id);
  gtk_entry_set_text(GTK_ENTRY(entry), text ? text : "") ;
  gtk_widget_set_sensitive(entry, FALSE) ;
  gtk_table_attach(table, entry, 1, cols, *row, *row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;

  label = gtk_label_new("New style: ") ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_table_attach(table, label, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);

  entry = my_data->new_style_name_widget = gtk_entry_new() ;
  text = g_quark_to_string(default_style_name) ;
  gtk_entry_set_text(GTK_ENTRY(entry), text ? text : "") ;
  gtk_widget_set_tooltip_text(entry, "If this is different to the original style name, then a new child style will be created; otherwise the original style will be overwritten.") ;
  gtk_table_attach(table, entry, 1, cols, *row, *row + 1, (GtkAttachOptions)(GTK_EXPAND | GTK_FILL), GTK_SHRINK, XPAD, YPAD);
  *row += 1 ;
}


/* Create colour-button widgets for the standard feature colours */
static void createColourWidgets(StyleChange my_data, GtkTable *table, int *row)
{
  GdkColor fill_colour;
  GdkColor border_colour;
  GdkColor *fill_col = &fill_colour;
  GdkColor *border_col = &border_colour;
  ZMapFeatureTypeStyle style = my_data->menu_data->feature_set->style;
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
  ZMapFeatureTypeStyle style = my_data->menu_data->feature_set->style;
  GdkColor fill_colour;
  GdkColor border_colour;
  GdkColor *fill_col = &fill_colour;
  GdkColor *border_col = &border_colour;
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
  GtkWidget *button = my_data->stranded = gtk_check_button_new_with_label("Stranded");
  gtk_table_attach(table, button, 0, 1, *row, *row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD);
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

  my_data->menu_data->window->style_window = NULL;

  g_free(my_data->menu_data);
  g_free(my_data);
}


/* Called when the user hits Revert on the Edit Style dialog. This throws away any changes since
 * they first opened the dialog, i.e. it reverts to the original style that was in the featureset. */
static gboolean revertChanges(gpointer cb_data)
{
  gboolean ok = TRUE ;
  StyleChange my_data = (StyleChange) cb_data;
  ItemMenuCBData menu_data = my_data->menu_data;
  ZMapFeatureSet feature_set = menu_data->feature_set;
  ZMapFeatureTypeStyle style = NULL;
  GQuark id;

  /* If the current style is different to the original then we have created a child style. Just
     remove the child and replace it with the parent */
  if(feature_set->style->unique_id != my_data->orig_style_copy.unique_id)   
    {
      GHashTable *styles = my_data->menu_data->window->context_map->styles;
      style = feature_set->style;
      id = style->parent_id;

      /* free the created style: cannot be ref'd anywhere else */
      g_hash_table_remove(styles,GUINT_TO_POINTER(feature_set->style->unique_id));
      zMapStyleDestroy(style);

      /* find the parent */
      style = (ZMapFeatureTypeStyle)g_hash_table_lookup(styles, GUINT_TO_POINTER(id));
    }
  else                                /* restore the existing style */
    {
      /* Overwrite the current style from the copy of the original style */
      style = feature_set->style;
      memcpy((void *) style, (void *) &my_data->orig_style_copy, sizeof (ZMapFeatureTypeStyleStruct));
    }

  /* update the column */
  if (style)
    zmapWindowFeaturesetSetStyle(style->unique_id, feature_set, menu_data->context_map, menu_data->window);

  /* update this dialog */
  zmapWindowStyleDialogSetFeature(menu_data->window, menu_data->item, menu_data->feature);

  return ok;
}


/* Called when the user applies changes on the Edit Style dialog. It updates the featureset's
 * style, creating a new child style if necessary  */
static gboolean applyChanges(gpointer cb_data)
{
  gboolean ok = TRUE ;
  StyleChange my_data = (StyleChange) cb_data;
  ZMapFeatureSet feature_set = my_data->menu_data->feature_set;
  GHashTable *styles = my_data->menu_data->window->context_map->styles;

  /* See if the new style already exists */
  GQuark new_style_id = zMapStyleCreateID(gtk_entry_get_text(GTK_ENTRY(my_data->new_style_name_widget))) ;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)g_hash_table_lookup(my_data->menu_data->context_map->styles, GUINT_TO_POINTER(new_style_id));

  /* It's possible the styles table has our style id but that it points to a different style,
   * e.g. a default style. We only want to update it if it's the exact style we're looking for. */
  if (style && style->unique_id != new_style_id)
    style = NULL ;

  /* If the style id is the same as the featureset id then we can assume that this featureset "owns" the style */
  const gboolean own_style = (new_style_id == feature_set->unique_id);

  /* Check whether we're editing the original style for this featureset or assigning a different one */
  //const gboolean using_original_style = (new_style_id == my_data->orig_style_copy.unique_id);

  /* Check whether we've already applied changes to this style */
  const gboolean already_edited_style = (new_style_id == my_data->last_edited_style_id) ;

  /* We make a new child style if the style doesn't already exist */
  if (!style)
    {
      /* make new style with the specified name and merge in the parent */
      ZMapFeatureTypeStyle parent = feature_set->style;
      ZMapFeatureTypeStyle tmp_style;

      const char *name = g_quark_to_string(new_style_id) ;
      style = zMapStyleCreate(name, name);

      /* merge is written upside down
       * we have to copy the parent and merge the child onto it
       * then delete the style we just created
       */

      tmp_style = zMapFeatureStyleCopy(parent) ;

      if (zMapStyleMerge(tmp_style, style))
        {
          g_hash_table_insert(styles,GUINT_TO_POINTER(style->unique_id),tmp_style);
          zMapStyleDestroy(style);
          style = tmp_style;

          g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_PARENT_STYLE, parent->unique_id , NULL);
        }
      else
        {
          zMapWarning("Cannot create new style %s",name);
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
      char *msg = g_strdup_printf("Style '%s' already exists.\n\nAre you sure you want to overwrite it?", 
                                  g_quark_to_string(new_style_id)) ;

      ok = zMapGUIMsgGetBool(GTK_WINDOW(my_data->toplevel), ZMAP_MSG_INFORMATION, msg) ;

      g_free(msg) ;
    }

  if (ok)
    {
      /* Remember that we've edited this style */
      my_data->last_edited_style_id = new_style_id ;

      /* apply the chosen colours etc */
      updateStyle(my_data, style);

      /* set the style
       * if we are only changing colours we could be more efficient
       * but this will work for all cases and means no need to write more code
       * we do this _once_ at user/ mouse click speed
       */
      zmapWindowFeaturesetSetStyle(style->unique_id, feature_set, my_data->menu_data->context_map, my_data->menu_data->window);
    }

  return ok;
}


/* Get the color spec string for the given color buttons. The result should be free'd by the caller
 * with g_free. Returns null if there was a problem. */
static char* getColorSpecStr(GtkWidget *fill_widget, GtkWidget *border_widget)
{
  char *colour_spec = NULL ;

  if (fill_widget && border_widget && 
      GTK_IS_COLOR_BUTTON(fill_widget) && GTK_IS_COLOR_BUTTON(border_widget))
    {
      GdkColor colour ;

      gtk_color_button_get_color(GTK_COLOR_BUTTON(fill_widget), &colour) ;
      char *fill_str = gdk_color_to_string(&colour) ;

      gtk_color_button_get_color(GTK_COLOR_BUTTON(border_widget), &colour) ;
      char *border_str = gdk_color_to_string(&colour) ;

      colour_spec = zMapStyleMakeColourString(fill_str, "black", border_str,
                                              fill_str, "black", border_str) ;
    }

  return colour_spec ;
}


/* Set the properties in the given style from the user-selected options from the dialog */
static void updateStyle(StyleChange my_data, ZMapFeatureTypeStyle style)
{
  /* Set whether strand-specific */
  gboolean stranded = gtk_toggle_button_get_active((GtkToggleButton *) my_data->stranded) ;
  g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_STRAND_SPECIFIC, stranded, NULL);


  /* Set the colours */
  char *colour_spec = getColorSpecStr(my_data->fill_widget, my_data->border_widget) ;

  if (colour_spec)
    {
      g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_COLOURS, colour_spec, NULL);
      g_free(colour_spec) ;
      colour_spec = NULL ;
    }


  /* Set the CDS colours, if it's a transcript */
  if(style->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      colour_spec = getColorSpecStr(my_data->cds_fill_widget, my_data->cds_border_widget) ;

      if (colour_spec)
        {
          g_object_set(G_OBJECT(style), ZMAPSTYLE_PROPERTY_TRANSCRIPT_CDS_COLOURS, colour_spec, NULL);
          g_free(colour_spec) ;
          colour_spec = NULL ;
        }
    }

}

