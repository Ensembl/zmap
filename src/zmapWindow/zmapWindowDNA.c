/*  File: zmapWindowDNA.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * Description: dialog for searching for dna strings.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSequence.h>
#include <ZMap/zmapDNA.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapString.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowCanvasItem.h>

typedef struct
{
  ZMapWindow window ;

  GtkWidget *toplevel ;
  GtkWidget *dna_entry ;
  GtkWidget *strand_entry ;
  GtkWidget *frame_entry ;

  GtkWidget *forward_widget ;
  GtkWidget *reverse_widget ;
  char *forward_colour_str ;
  char *reverse_colour_str ;

  gboolean keep_previous_hits ;

  ZMapFeatureBlock block ;
  ZMapSequenceType sequence_type ;

  int search_start ;
  int search_end ;

  int screen_search_start ;
  int screen_search_end ;

  int max_errors ;
  int max_Ns ;

} DNASearchDataStruct, *DNASearchData ;


static void dnaMatchesToFeatures(ZMapWindow window, GList *match_list, ZMapFeatureTypeStyle orig_style,
				 ZMapFeatureSet *feature_set_out, ZMapFeatureTypeStyle *style_out) ;
static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void clearCB(GtkWidget *widget, gpointer cb_data) ;
static void colourSetCB(GtkColorButton *widget, gpointer user_data) ;
static void keepHitsCB(GtkToggleButton *toggle_button, gpointer user_data) ;
static void searchCB(GtkWidget *widget, gpointer cb_data) ;
static void startSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static void endSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static void errorSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static void nSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;

static GtkWidget *makeMenuBar(DNASearchData search_data) ;

static GtkWidget *makeSpinPanel(DNASearchData search_data,
				char *title,
				char *strand_title,
				char *frame_title,
				char *spin_label, int min, int max, int init, GtkSignalFunc func,
				char *spin_label2, int min2, int max2, int init2, GtkSignalFunc func2) ;

static void remapCoords(gpointer data, gpointer user_data) ;
static void printCoords(gpointer data, gpointer user_data) ;

static void matches_to_features(gpointer list_data, gpointer user_data);
static void remove_current_matches_from_display(DNASearchData search_data);

static void setColoursInStyle(DNASearchData search_data, ZMapFeatureTypeStyle style) ;




static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Close",      "<control>W",  requestDestroyCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/Overview",   NULL,          helpCB,      0, NULL,            NULL}
};

static gboolean window_dna_debug_G = FALSE;


/* gets incremented for each DNA search added to the seperator
 * used to name featuresets for each search so that they can have diff styles
 * it's not feasable for anyone to wrap round
 * ie nominally with 1 second to do a search it would take 68 years.
 * NOTE this number is used for all ZMaps views and windows, they are all distinct
 */
static int DNA_group_G = 0;





/*
 *                     External routines.
 */

void zmapWindowCreateSequenceSearchWindow(ZMapWindow window, FooCanvasItem *feature_item,
					  ZMapSequenceType sequence_type)
{
  GtkWidget *toplevel, *vbox, *menubar, *topbox, *hbox, *frame, *entry, *label, *button,
    *start_end, *errors,
    *buttonBox, *search_button, *forward_colour_button, *reverse_colour_button, *clear_button ;
  GdkColor colour = {0} ;
  DNASearchData search_data ;
  ZMapFeatureAny feature_any ;
  ZMapFeatureBlock block ;
  int max_errors, max_Ns ;
  char *text, *frame_label, *frame_text ;
  int screen_search_end, screen_search_start;
  gboolean proceed = TRUE;

  /* Need to check that there is any dna...n.b. we need the item that was clicked for us to check
   * the dna..... */
  if (ZMAP_IS_CONTAINER_GROUP(feature_item))
    {
      zmapWindowContainerGetFeatureAny((ZMapWindowContainerGroup)feature_item, &feature_any);
    }
  else if (ZMAP_IS_CANVAS_ITEM(feature_item))
    {
      feature_any = (ZMapFeatureAny)zMapWindowCanvasItemGetFeature(feature_item) ;
    }
  else
    {
      zMapAssertNotReached();
    }

  if(feature_any == NULL)
    {
      proceed = FALSE;
      zMapLogCritical("No feature attached to item (%p)", feature_item);
    }

  if(proceed)
    block = (ZMapFeatureBlock)zMapFeatureGetParentGroup(feature_any,
						      ZMAPFEATURE_STRUCT_BLOCK) ;

  if (block->sequence.type == ZMAPSEQUENCE_NONE)
    {
      proceed = FALSE;
      zMapWarning("Cannot search DNA in this block \"%s\", "
		  "either it has no DNA or the DNA was not fetched from the server.",
		  g_quark_to_string(block->original_id)) ;

    }

  if (proceed)
    {
      search_data = g_new0(DNASearchDataStruct, 1) ;

      search_data->forward_colour_str = g_strdup("red") ;
      search_data->reverse_colour_str = g_strdup("green") ;

      search_data->window = window ;
      search_data->block  = block ;
      search_data->sequence_type = sequence_type ;
      search_data->search_start  = block->block_to_sequence.block.x1 ;
      search_data->search_end    = block->block_to_sequence.block.x2 ;

      /* Get block coords in screen coords, saving for min & max of spin buttons */
      zmapWindowCoordPairToDisplay(window, search_data->search_start, search_data->search_end,
				   &screen_search_start, &screen_search_end) ;


      /* Set the initial screen start & end based on no mark */
      search_data->screen_search_start = screen_search_start;
      search_data->screen_search_end   = screen_search_end;

      /* Update the start & end according to the mark */
      if (zmapWindowMarkIsSet(search_data->window->mark))
	{
	  zmapWindowMarkGetSequenceRange(search_data->window->mark,
					 &(search_data->screen_search_start),
					 &(search_data->screen_search_end));

	  zmapWindowCoordPairToDisplay(window,
				       search_data->screen_search_start,
				       search_data->screen_search_end,
				       &(search_data->screen_search_start),
				       &(search_data->screen_search_end)) ;
	}

      /* Clamp to length of sequence, useless to do that but possible.... */
      max_errors = max_Ns = block->block_to_sequence.block.x2 - block->block_to_sequence.block.x1 + 1 ;


      /* set up the top level window */
      if (sequence_type == ZMAPSEQUENCE_DNA)
	text = "DNA Search" ;
      else
	text = "Peptide Search" ;
      search_data->toplevel = toplevel = zMapGUIToplevelNew(NULL, text) ;

      g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		       GTK_SIGNAL_FUNC(destroyCB), (gpointer)search_data) ;

      gtk_container_set_focus_chain (GTK_CONTAINER(toplevel), NULL);

      gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
      gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, -1) ;


      /* Add ptrs so parent knows about us */
      g_ptr_array_add(window->dna_windows, (gpointer)toplevel) ;


      vbox = gtk_vbox_new(FALSE, 0) ;
      gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;
      gtk_container_set_focus_chain (GTK_CONTAINER(vbox), NULL);

      menubar = makeMenuBar(search_data) ;
      gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);


      /* Make the box for dna text. */
      hbox = gtk_hbox_new(FALSE, 0) ;
      gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

      if (sequence_type == ZMAPSEQUENCE_DNA)
	text = "Enter DNA: " ;
      else
	text = "Enter Peptide: " ;
      frame = gtk_frame_new(text) ;
      gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.5);
      gtk_container_border_width(GTK_CONTAINER(frame), 5);
      gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0) ;

      topbox = gtk_vbox_new(FALSE, 5) ;
      gtk_container_set_focus_chain (GTK_CONTAINER(topbox), NULL);
      gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
      gtk_container_add (GTK_CONTAINER (frame), topbox) ;


      search_data->dna_entry = entry = gtk_entry_new() ;
      gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
      gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
      gtk_box_pack_start(GTK_BOX(topbox), entry, FALSE, FALSE, 0) ;


      /* Make the start/end boxes. */
      hbox = gtk_hbox_new(FALSE, 0) ;
      gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
      if (sequence_type == ZMAPSEQUENCE_PEPTIDE)
	{
	  frame_label = "Frame :" ;
	  frame_text = "Set Strand/Frame/Start/End coords of search: " ;
	}
      else
	{
	  frame_label = NULL ;
	  frame_text = "Set Strand/Start/End coords of search: " ;
	}

      start_end = makeSpinPanel(search_data,
				frame_text,
				"Strand :",
				frame_label,
				"Start :", screen_search_start, screen_search_end,
				search_data->screen_search_start, GTK_SIGNAL_FUNC(startSpinCB),
				"End :", screen_search_start, screen_search_end,
				search_data->screen_search_end, GTK_SIGNAL_FUNC(endSpinCB)) ;
      gtk_box_pack_start(GTK_BOX(hbox), start_end, TRUE, TRUE, 0) ;
      gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
      gtk_container_set_focus_chain (GTK_CONTAINER(start_end), NULL);

      /* Make the error boxes for dna search. */
      if (sequence_type == ZMAPSEQUENCE_DNA)
	{
	  hbox = gtk_hbox_new(FALSE, 0) ;
	  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
	  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	  errors = makeSpinPanel(search_data,
				 "Set Maximum Acceptable Error Rates: ",
				 NULL,
				 NULL,
				 "Mismatches :", 0, max_errors, 0, GTK_SIGNAL_FUNC(errorSpinCB),
				 "N bases :", 0, max_Ns, 0, GTK_SIGNAL_FUNC(nSpinCB)) ;
	  gtk_box_pack_start(GTK_BOX(hbox), errors, TRUE, TRUE, 0) ;
	}

      /* Make colour buttons. */
      frame = gtk_frame_new("Set Hit Properties:") ;
      gtk_container_set_border_width(GTK_CONTAINER(frame),
				     ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
      gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0) ;

      hbox = gtk_hbox_new(FALSE, 0) ;
      gtk_container_add(GTK_CONTAINER(frame), hbox) ;
      gtk_box_set_spacing(GTK_BOX(hbox), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING) ;
      gtk_container_set_border_width(GTK_CONTAINER(hbox), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);


      label = gtk_label_new("Forward Strand\nColour:") ;
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

      search_data->forward_widget = forward_colour_button = gtk_color_button_new() ;
      gdk_color_parse(search_data->forward_colour_str, &colour) ;
      gtk_color_button_set_color(GTK_COLOR_BUTTON(forward_colour_button), &colour) ;
      g_signal_connect(G_OBJECT(forward_colour_button), "color-set",
		       G_CALLBACK(colourSetCB), search_data) ;
      gtk_box_pack_start(GTK_BOX(hbox), forward_colour_button, FALSE, FALSE, 0) ;

      label = gtk_label_new("Reverse Strand\nColour:") ;
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

      search_data->reverse_widget = reverse_colour_button = gtk_color_button_new() ;
      gdk_color_parse(search_data->reverse_colour_str, &colour) ;
      gtk_color_button_set_color(GTK_COLOR_BUTTON(reverse_colour_button), &colour) ;
      g_signal_connect(G_OBJECT(reverse_colour_button), "color-set",
		       G_CALLBACK(colourSetCB), search_data) ;
      gtk_box_pack_start(GTK_BOX(hbox), reverse_colour_button, FALSE, FALSE, 0) ;

      label = gtk_label_new("Keep Previous\nSearch:") ;
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

      button = gtk_check_button_new() ;
      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0) ;
      g_signal_connect(G_OBJECT(button), "toggled",
		       G_CALLBACK(keepHitsCB), search_data) ;



      /* Make control buttons along bottom of dialog. */
      frame = gtk_frame_new(NULL) ;
      gtk_container_set_border_width(GTK_CONTAINER(frame),
				     ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
      gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0) ;


      buttonBox = gtk_hbutton_box_new();
      gtk_container_add(GTK_CONTAINER(frame), buttonBox);
      gtk_box_set_spacing(GTK_BOX(buttonBox), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
      gtk_container_set_border_width(GTK_CONTAINER (buttonBox), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

      clear_button = gtk_button_new_from_stock(GTK_STOCK_CLEAR);
      gtk_box_pack_start(GTK_BOX(buttonBox), clear_button, FALSE, FALSE, 0);
      g_signal_connect(G_OBJECT(clear_button), "clicked",
		       G_CALLBACK(clearCB), search_data);

      search_button = gtk_button_new_from_stock(GTK_STOCK_FIND) ;
      gtk_box_pack_end(GTK_BOX(buttonBox), search_button, FALSE, FALSE, 0) ;
      gtk_signal_connect(GTK_OBJECT(search_button), "clicked",
			 GTK_SIGNAL_FUNC(searchCB), (gpointer)search_data) ;
      GTK_WIDGET_SET_FLAGS(search_button, GTK_CAN_DEFAULT) ; /* set search button as default. */
      gtk_window_set_default(GTK_WINDOW(toplevel), search_button) ;


      gtk_widget_show_all(toplevel) ;
    }

  return ;
}

/*
 *                 Internal routines
 */

static void dnaMatchesToFeatures(ZMapWindow window, GList *match_list, ZMapFeatureTypeStyle orig_style,
				 ZMapFeatureSet *feature_set_out, ZMapFeatureTypeStyle *style_out)
{
  ZMapFeatureSet separator_featureset = NULL ;
  ZMapFeatureTypeStyle style ;
  ZMapFeatureSetDesc f2c;
  ZMapFeatureSource src;
  GList *list;
  ZMapFeatureColumn column;
  zmapWindowFeatureSetStyleStruct fstyle = {NULL} ;
  gchar *name = g_strdup_printf("%s_%d", ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME, DNA_group_G++);


  separator_featureset = zMapFeatureSetCreate(name, NULL);

  style = zMapFeatureStyleCopy(orig_style);

  separator_featureset->style = style ;

  /* set up featureset2_column and anything else needed */
  f2c = g_hash_table_lookup(window->context_map->featureset_2_column, GUINT_TO_POINTER(separator_featureset->unique_id));
  if(!f2c)	/* these just accumulate  and should be removed from the hash table on clear */
    {
      f2c = g_new0(ZMapFeatureSetDescStruct,1);

      f2c->column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME);
      f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME);
      f2c->feature_src_ID = g_quark_from_string(name);
      f2c->feature_set_text = ZMAP_FIXED_STYLE_SEARCH_MARKERS_TEXT;
      g_hash_table_insert(window->context_map->featureset_2_column, GUINT_TO_POINTER(separator_featureset->unique_id), f2c);
    }

  src = g_hash_table_lookup(window->context_map->source_2_sourcedata, GUINT_TO_POINTER(separator_featureset->unique_id));
  if(!src)
    {
      src = g_new0(ZMapFeatureSourceStruct,1);
      src->source_id = f2c->feature_src_ID;
      src->source_text = g_quark_from_string(ZMAP_FIXED_STYLE_SEARCH_MARKERS_TEXT);
      src->style_id = style->unique_id;
      src->maps_to = f2c->column_id;
      g_hash_table_insert(window->context_map->source_2_sourcedata, GUINT_TO_POINTER(separator_featureset->unique_id), src);
    }

  list = g_hash_table_lookup(window->context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id));
  if(!list)
    {
      list = g_list_prepend(list,GUINT_TO_POINTER(src->style_id));
      g_hash_table_insert(window->context_map->column_2_styles,GUINT_TO_POINTER(f2c->column_id), list);
    }

  column = g_hash_table_lookup(window->context_map->columns,GUINT_TO_POINTER(f2c->column_id));
  if(!column)
    {
      column = g_new0(ZMapFeatureColumnStruct,1);
      column->unique_id = f2c->column_id;
      column->style_table = g_list_prepend(NULL, (gpointer)  style);
      /* the rest shoudl get filled in elsewhere */
      g_hash_table_insert(window->context_map->columns, GUINT_TO_POINTER(f2c->column_id), column);
    }

  fstyle.feature_set   = separator_featureset;
  fstyle.feature_style = style;

  g_list_foreach(match_list, matches_to_features, &fstyle);

  if (feature_set_out)
    *feature_set_out = separator_featureset ;

  if (style_out)
    *style_out = style ;

  return ;
}


static GtkWidget *makeMenuBar(DNASearchData search_data)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)search_data) ;

  gtk_window_add_accel_group(GTK_WINDOW(search_data->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}



static GtkWidget *makeSpinPanel(DNASearchData search_data,
				char *title,
				char *combo_label,
				char *combo_label2,
				char *spin_label, int min, int max, int init, GtkSignalFunc func,
				char *spin_label2, int min2, int max2, int init2, GtkSignalFunc func2)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *label, *error_spinbox, *n_spinbox ;


  frame = gtk_frame_new(title);
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.5 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_container_set_focus_chain (GTK_CONTAINER(frame), NULL);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(topbox), NULL);
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  if (combo_label)
    {
      GtkWidget *combo = NULL, *entry ;

      label = gtk_label_new(combo_label) ;
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

      combo = gtk_combo_box_entry_new_text();
      gtk_container_set_focus_chain (GTK_CONTAINER(combo), NULL);

      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "*");
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "+");
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "-");

      search_data->strand_entry = entry = GTK_BIN(combo)->child;
      gtk_entry_set_text(GTK_ENTRY(entry), "*") ;
      gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0) ;
    }

  if (combo_label2)
    {
      GtkWidget *combo = NULL, *entry ;

      label = gtk_label_new(combo_label2) ;
      gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
      gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

      combo = gtk_combo_box_entry_new_text();
      gtk_container_set_focus_chain (GTK_CONTAINER(combo), NULL);

      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "*");
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "0");
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "1");
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), "2");

      search_data->frame_entry = entry = GTK_BIN(combo)->child;
      gtk_entry_set_text(GTK_ENTRY(entry), "*") ;
      gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
      gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, TRUE, 0) ;
    }

  label = gtk_label_new(spin_label) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  if (min > max)
    zMapUtilsSwop(int, min, max) ;

  error_spinbox = gtk_spin_button_new_with_range(min, max, 1.0) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(error_spinbox), init) ;
  gtk_signal_connect(GTK_OBJECT(error_spinbox), "value-changed",
		     GTK_SIGNAL_FUNC(func), (gpointer)search_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), error_spinbox, FALSE, FALSE, 0) ;

  label = gtk_label_new(spin_label2) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;


  if (min2 > max2)
    zMapUtilsSwop(int, min2, max2) ;

  n_spinbox = gtk_spin_button_new_with_range(min2, max2, 1.0) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(n_spinbox), init2) ;
  gtk_signal_connect(GTK_OBJECT(n_spinbox), "value-changed",
		     GTK_SIGNAL_FUNC(func2), (gpointer)search_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), n_spinbox, FALSE, FALSE, 0) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);


  return frame ;
}

static void searchCB(GtkWidget *widget, gpointer cb_data)
{
  DNASearchData search_data = (DNASearchData)cb_data ;
  char *query_txt = NULL, *query_buf = NULL ;
  char *err_text = NULL ;
  char *dna ;
  int start, end, dna_len ;
  char *strand_str = NULL, *frame_str = NULL ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;
  ZMapFrame frame = ZMAPFRAME_NONE ;


  /* If we are revcomp'd these coords will be the wrong way round. */
  search_data->search_start = zmapWindowCoordFromDisplay(search_data->window, search_data->screen_search_start) ;
  search_data->search_end = zmapWindowCoordFromDisplay(search_data->window, search_data->screen_search_end) ;
  if (search_data->search_start > search_data->search_end)
    zMapUtilsSwop(int, search_data->search_start, search_data->search_end) ;


  /* Set the strand and frame. */
  if (search_data->strand_entry && (strand_str = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->strand_entry))))
    {
      if (!(zMapFeatureFormatStrand(strand_str, &strand)))
	{
	  strand = ZMAPSTRAND_NONE ;
	}

      if(search_data->window && search_data->window->revcomped_features)
	{
	  /* switch the strand to fix rt bug # 77224 */
	  switch(strand)
	    {
	    case ZMAPSTRAND_FORWARD:
	      strand = ZMAPSTRAND_REVERSE;
	      break;
	    case ZMAPSTRAND_REVERSE:
	      strand = ZMAPSTRAND_FORWARD;
	      break;
	    default:
	      break;
	    }
	}
    }

  if (search_data->frame_entry && (frame_str = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->frame_entry))))
    {
      if (!(zMapFeatureFormatFrame(frame_str, &frame)))
	{
	  frame = ZMAPFRAME_NONE ;
	}
    }

  /* Convert to relative coords.... */
  start = search_data->search_start - search_data->block->block_to_sequence.block.x1 ;
  end = search_data->search_end - search_data->block->block_to_sequence.block.x1 ;
  dna = search_data->block->sequence.sequence ;
  dna_len = strlen(dna) ;


  /* Validate the query string, note that gtk_entry returns "" for no text, _not_ NULL. */
  query_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->dna_entry)) ;
  query_buf =  query_txt =  g_strdup(query_txt);
  query_txt = zMapStringFlatten(query_txt);	/* take out the null/ newline characters */

  if(*query_txt == '>')		/* we probably pasted a FASTA sequence */
  {

	  if (search_data->sequence_type == ZMAPSEQUENCE_DNA)
	  {
		/* caution: reverse engineered fix, search for 'bp' */
		char * where = (char *) g_strstr_len(query_txt,-1,"bp");
		if(where)
			query_txt = where + 2;
	  }
	  else
	  {
  		/* caution: reverse engineered fix, search for 'AA' */
		char * where = (char *) g_strstr_len(query_txt,-1,"AA");
		if(where)
			query_txt = where + 2;
	  }
  }


  gtk_entry_set_text(GTK_ENTRY(search_data->dna_entry), query_txt);

  if (strlen(query_txt) == 0)
    {
      err_text = g_strdup_printf("no query %s supplied.", (search_data->sequence_type == ZMAPSEQUENCE_DNA
							   ? "dna" : "peptide")) ;
    }
  else
    {
      /* canonicalise search string. */
      if (search_data->sequence_type == ZMAPSEQUENCE_DNA)
	zMapDNACanonical(query_txt) ;
      else
	zMapPeptideCanonical(query_txt) ;

      if ((search_data->sequence_type == ZMAPSEQUENCE_DNA && !zMapDNAValidate(query_txt))
	  || (search_data->sequence_type == ZMAPSEQUENCE_PEPTIDE && !zMapPeptideValidate(query_txt)))
	err_text = g_strdup_printf("query %s contains invalid %s.",
				   (search_data->sequence_type == ZMAPSEQUENCE_DNA ? "dna" : "peptide"),
				   (search_data->sequence_type == ZMAPSEQUENCE_DNA ? "bases" : "amino-acids")) ;
    }

  if (!err_text)
    {
      if ((start < 0 || start > dna_len) || (end < 0 || end > dna_len)
	  || (search_data->max_errors < 0 || search_data->max_errors > dna_len)
	  || (search_data->max_Ns < 0 || search_data->max_Ns > dna_len))
	err_text = g_strdup_printf("start/end/max errors/max Ns\n must all be within range %d -> %d",
				   search_data->block->block_to_sequence.block.x1,
			       search_data->block->block_to_sequence.block.x2) ;
    }


  if (err_text)
    {
      zMapMessage("Please correct and retry: %s", err_text) ;
    }
  else
    {
      ZMapFeatureTypeStyle orig_style ;
      GList *match_list ;

      orig_style = zMapFindStyle(search_data->window->context_map->styles,
				 zMapStyleCreateID(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME)) ;
      zMapAssert(orig_style) ;

      if (search_data->sequence_type == ZMAPSEQUENCE_DNA
	  && (match_list = zMapDNAFindAllMatches(dna, query_txt, strand, start, end - start + 1,
						 search_data->max_errors, search_data->max_Ns, TRUE)))
	{
	  ZMapFeatureSet new_feature_set = NULL ;
	  ZMapFeatureTypeStyle new_style = NULL ;
	  ZMapDNAMatch match_data ;
	  char *match_seq, *match_details ;


          if (window_dna_debug_G)
            g_list_foreach(match_list, printCoords, dna) ;

	  match_data = (ZMapDNAMatch)(match_list->data) ;
	  match_seq = match_data->match ;

	  match_details = g_strdup_printf("Matches for \"%s\", start = %d, end = %d, max errors = %d, max N's %d",
					  g_quark_to_string(search_data->block->original_id),
					  search_data->search_start, search_data->search_end,
					  search_data->max_errors, search_data->max_Ns) ;

	  /* Need to convert coords back to block coords here.... */
	  g_list_foreach(match_list, remapCoords, search_data) ;

	  if (!(search_data->keep_previous_hits))
	    remove_current_matches_from_display(search_data);

	  dnaMatchesToFeatures(search_data->window, match_list, orig_style,
			       &new_feature_set, &new_style) ;

	  setColoursInStyle(search_data, new_style) ;

	  zmapWindowDrawSeparatorFeatures(search_data->window,
					  search_data->block,
					  new_feature_set,
					  new_style) ;

	  zmapWindowDNAListCreate(search_data->window, match_list,
				  (char *)g_quark_to_string(search_data->block->original_id),
				  match_seq,
				  match_details,
				  search_data->block, new_feature_set) ;

	  g_free(match_details) ;
	}
      else if (search_data->sequence_type == ZMAPSEQUENCE_PEPTIDE
	       && (match_list = zMapPeptideMatchFindAll(dna, query_txt,
							search_data->window->revcomped_features,
							strand, frame, start, end - start + 1,
							search_data->max_errors, search_data->max_Ns, TRUE)))
	{
	  ZMapFeatureSet new_feature_set = NULL;
	  ZMapFeatureTypeStyle new_style = NULL;
	  ZMapDNAMatch match_data ;
	  char *match_seq, *match_details ;

          if(window_dna_debug_G)
            g_list_foreach(match_list, printCoords, dna) ;

	  match_data = (ZMapDNAMatch)(match_list->data) ;
	  match_seq = match_data->match ;

	  match_details = g_strdup_printf("Reference: \"%s\", Match: \"%s\"\n"
					  "Start = %d, End = %d, Max Errors = %d, Max N's %d",
					  g_quark_to_string(search_data->block->original_id), match_seq,
					  search_data->search_start, search_data->search_end,
					  search_data->max_errors, search_data->max_Ns) ;


	  /* Need to convert coords back to block coords here.... */
	  g_list_foreach(match_list, remapCoords, search_data) ;

	  if (!(search_data->keep_previous_hits))
	    remove_current_matches_from_display(search_data);

	  dnaMatchesToFeatures(search_data->window, match_list,  orig_style, &new_feature_set, &new_style) ;

	  setColoursInStyle(search_data, new_style) ;

	  zmapWindowDrawSeparatorFeatures(search_data->window,
					  search_data->block,
					  new_feature_set,
					  new_style);

	  zmapWindowDNAListCreate(search_data->window, match_list,
				  (char *)g_quark_to_string(search_data->block->original_id),
				  match_seq,
				  match_details,
				  search_data->block, new_feature_set) ;

	  g_free(match_details) ;
	}
      else
	{
	  zMapMessage("Sorry, no matches in sequence \"%s\" for query \"%s\"",
		      g_quark_to_string(search_data->block->original_id), query_txt) ;
	}
    }

  if (err_text)
    g_free(err_text) ;

  if(query_buf)
	g_free(query_buf) ;

  return ;
}


/* "clear" button callback, resets the DNA entry text to an empty string. */
static void clearCB(GtkWidget *widget, gpointer cb_data)
{
  DNASearchData search_data = (DNASearchData)cb_data;

  remove_current_matches_from_display(search_data);

  gtk_entry_set_text(GTK_ENTRY(search_data->dna_entry), "") ;

  gtk_widget_grab_focus(search_data->dna_entry);

  return ;
}


/* Slightly obscure GTK interface...this function is called when the user selects a colour
 * from the colour chooser dialog displayed when they click on the colour button,
 * i.e. it is _not_ a callback for the button itself. */
static void colourSetCB(GtkColorButton *widget, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;
  GdkColor colour = {0} ;
//  char *colour_string = NULL ;
  char **target_string ;

  if (GTK_WIDGET(widget) == search_data->forward_widget)
    target_string = &(search_data->forward_colour_str) ;
  else
    target_string = &(search_data->reverse_colour_str) ;

  if (*target_string)
    g_free(*target_string) ;

  gtk_color_button_get_color(widget, &colour) ;

  *target_string = gdk_color_to_string(&colour) ;

  return ;
}



static void keepHitsCB(GtkToggleButton *toggle_button, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;

  search_data->keep_previous_hits = gtk_toggle_button_get_active(toggle_button) ;

  return ;
}





/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  char *title = "DNA Search Window" ;
  char *help_text =
    "The ZMap DNA Search Window allows you to search for DNA. You can cut/paste DNA into the\n"
    "DNA text field or type it in. You can specify the maximum number of mismatches\n"
    "and the maximum number of ambiguous/unknown bases acceptable in the match.\n" ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}


/* Requests destroy of search window which will end up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  DNASearchData search_data = (DNASearchData)cb_data ;

  gtk_widget_destroy(search_data->toplevel) ;

  return ;
}


static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  DNASearchData search_data = (DNASearchData)cb_data ;

  remove_current_matches_from_display(search_data);

  g_ptr_array_remove(search_data->window->dna_windows, (gpointer)search_data->toplevel);

  return ;
}


static void startSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;

  search_data->screen_search_start = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}

static void endSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;

  search_data->screen_search_end = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}


static void errorSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;

  search_data->max_errors = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}

static void nSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData search_data = (DNASearchData)user_data ;

  search_data->max_Ns = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}



static void remapCoords(gpointer data, gpointer user_data)
{
  ZMapDNAMatch match_data = (ZMapDNAMatch)data ;
  DNASearchData search_data = (DNASearchData)user_data ;
  ZMapFeatureBlock block = (ZMapFeatureBlock)search_data->block ;


  zMapBlock2FeatureCoords(block, &(match_data->ref_start), &(match_data->ref_end)) ;

  zmapWindowCoordPairToDisplay(search_data->window, match_data->ref_start, match_data->ref_end,
			       &(match_data->screen_start), &(match_data->screen_end)) ;

  if (search_data->window && search_data->window->revcomped_features)
    {
      /* switch the strand to fix rt bug # 77224 */
      switch(match_data->strand)
	{
	case ZMAPSTRAND_FORWARD:
	  match_data->strand = ZMAPSTRAND_REVERSE;
	  break;
	case ZMAPSTRAND_REVERSE:
	default:
	  match_data->strand = ZMAPSTRAND_FORWARD;
	  break;
	}
    }

  return ;
}


static void printCoords(gpointer data, gpointer user_data)
{
  ZMapDNAMatch match_data = (ZMapDNAMatch)data ;
  char *dna = (char *)user_data ;
  GString *match_str ;

  match_str = g_string_new("") ;

  match_str = g_string_append_len(match_str,
				  (dna + match_data->ref_start),
				  (match_data->ref_end - match_data->ref_start + 1)) ;

  printf("Ref start,end  =  %d,%d  Match start,end = %d,%d    %s\n",
	 match_data->ref_start, match_data->ref_end,
	 match_data->start, match_data->end,
	 match_str->str) ;


  g_string_free(match_str, TRUE) ;

  return ;
}



static void matches_to_features(gpointer list_data, gpointer user_data)
{
  ZMapDNAMatch current_match = (ZMapDNAMatch)list_data;
  zmapWindowFeatureSetStyle fstyle = (zmapWindowFeatureSetStyle)user_data;
  ZMapFeatureSet feature_set;
  ZMapFeatureTypeStyle style;
  ZMapFeature current_feature;
  char *sequence = NULL, *ontology = "";
  double score = 100.0;
  int start, end;
  gboolean has_score = TRUE;

  start = current_match->ref_start;
  end   = current_match->ref_end;

  feature_set = fstyle->feature_set;
  style       = fstyle->feature_style;

  if(!feature_set->style)
  {
	  feature_set->style = style;
  }

  current_feature = zMapFeatureCreateFromStandardData(current_match->match,
						      sequence,
						      ontology,
						      ZMAPSTYLE_MODE_BASIC,
						      &fstyle->feature_set->style,
						      start, end,
						      has_score, score,
						      current_match->strand) ;

  zMapFeatureSetAddFeature(feature_set, current_feature);

  return ;
}

static void remove_current_matches_from_display(DNASearchData search_data)
{
  /* If there's already a context we need to remove the current markers... */

  /* I've used the container and feature set destroy calls rather than the
   * Method used by the code in zmapView/zmapViewRemoteReceive which removes
   * features.. Why because it didn't work
   */

  /* Scrap that. Now it does, but I've left the other code around... */
  if (search_data->window->strand_separator_context)
    {
      ZMapFeatureAlignment align;
      ZMapFeatureBlock block;
      ZMapFeatureSet feature_set;
      FooCanvasItem *container = NULL;
	GList *sets = NULL, *free_this;
	ZMapFeatureContext diff_context = NULL, erase_context;
	gboolean is_master = FALSE;

      /* Get the alignment from the current context by looking up using block's parent */
      align = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)search_data->block,
							      ZMAPFEATURE_STRUCT_ALIGN) ;
      align = zMapFeatureContextGetAlignmentByID(search_data->window->strand_separator_context,
						 align->unique_id);
      /* Get the block matching the search_data->block */
      block = zMapFeatureAlignmentGetBlockByID(align, search_data->block->unique_id);

	free_this = sets = zMapFeatureBlockGetMatchingSets(block,g_ascii_strdown(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME,-1));


	/* remove multiple featuresets matching the name, we have a diff set for each search */
	is_master = (search_data->window->strand_separator_context->master_align == align);

	block = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block);

	align = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)align);

	erase_context = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)search_data->window->strand_separator_context);


	zMapFeatureContextAddAlignment(erase_context, align, is_master);
	zMapFeatureAlignmentAddBlock(align, block);


	for( ; sets; sets = sets->next)
	{
		feature_set = (ZMapFeatureSet) sets->data;
		/* its container, to hide it later */
		if(!container)
			container = zmapWindowFToIFindSetItem(search_data->window,search_data->window->context_to_item,
						feature_set,
						ZMAPSTRAND_NONE,
						ZMAPFRAME_NONE);

		feature_set = zMapFeatureSetCopy(feature_set);

		zMapFeatureBlockAddFeatureSet(block, feature_set);
	}

	if(container)
	{
		if(!zMapFeatureContextErase(&(search_data->window->strand_separator_context),
					erase_context,
					&diff_context))
		{
			zMapLogCritical("%s", "Failed to complete Context Erase!");
		}
		else
		{
			/* remove the FtoiHash only */
			zMapWindowUnDisplaySearchFeatureSets(search_data->window, NULL, diff_context);

			/* this handles destroy of the (virtual) CanvasFeatureset */
			zmapWindowContainerGroupDestroy((ZMapWindowContainerGroup)(container));

			zmapWindowContainerRequestReposition(search_data->window->feature_root_group);

			zMapFeatureContextDestroy(diff_context, TRUE);
		}
	}

	zMapFeatureContextDestroy(erase_context, TRUE);

	g_list_free(free_this);
    }


  return ;
}


static void setColoursInStyle(DNASearchData search_data, ZMapFeatureTypeStyle style)
{
  char *forward_colour_spec, *reverse_colour_spec ;

  forward_colour_spec = zMapStyleMakeColourString(search_data->forward_colour_str, "black", NULL,
						  search_data->forward_colour_str, "black", NULL) ;

  reverse_colour_spec = zMapStyleMakeColourString(search_data->reverse_colour_str, "black", NULL,
						  search_data->reverse_colour_str, "black", NULL) ;

  g_object_set(G_OBJECT(style),
		 ZMAPSTYLE_PROPERTY_COLOURS, forward_colour_spec,
		 ZMAPSTYLE_PROPERTY_REV_COLOURS, reverse_colour_spec,
		 NULL);

  return ;
}


