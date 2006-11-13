/*  File: zmapWindowDNAChoose.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Shows a dialog window allowing user to choose a section
 *              of DNA to export. Initially the DNA section is that
 *              of the feature the user selected and they can add
 *              a +/- extra section to that.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Nov 13 09:48 2006 (edgrif)
 * Created: Fri Nov 10 09:50:48 2006 (edgrif)
 * CVS info:   $Id: zmapWindowDNAChoose.c,v 1.1 2006-11-13 09:56:56 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapDNA.h>
#include <zmapWindow_P.h>


typedef struct
{
  ZMapWindow window ;

  ZMapFeatureBlock block ;

  GtkWidget *toplevel ;
  GtkWidget *dna_entry ;

  char *entry_text ;

  int dna_start ;
  int dna_end ;
  int dna_flanking ;
  gboolean revcomp ;

  char *dna ;
} DNASearchDataStruct, *DNASearchData ;



static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void startSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static void endSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static void flankingSpinCB(GtkSpinButton *spinbutton, gpointer user_data) ;
static GtkWidget *makeMenuBar(DNASearchData dna_data) ;
static GtkWidget *makeSpinPanel(DNASearchData dna_data,
				char *title,
				char *spin_label, int min, int max, int init, GtkSignalFunc func,
				char *spin_label2, int min2, int max2, int init2, GtkSignalFunc func2,
				char *spin_label3, int min3, int max3, int init3, GtkSignalFunc func3) ;
static void getDNA(DNASearchData dna_data) ;




static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Close",      "<control>W",  requestDestroyCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/Overview",   NULL,          helpCB,      0, NULL,            NULL}
};




/* Returns the DNA chosen by the user. */
char *zmapWindowDNAChoose(ZMapWindow window, FooCanvasItem *feature_item, ZMapWindowDialogType dialog_type)
{
  char *dna = NULL ;
  GtkWidget *toplevel, *vbox, *menubar, *topbox, *hbox, *frame, *entry, *start_end ;
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT ;
  DNASearchData dna_data ;
  ZMapFeature feature ;
  ZMapFeatureBlock block ;
  gint result ;
  gint block_start, block_end ;
  char *button_text ;

  /* Need to check that there is any dna...n.b. we need the item that was clicked for us to check
   * the dna..... */
  feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA) ;
  zMapAssert(feature) ;
  block = (ZMapFeatureBlock)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
						      ZMAPFEATURE_STRUCT_BLOCK) ;

  if (block->sequence.type == ZMAPSEQUENCE_NONE)
    {
      zMapWarning("Cannot search DNA in this block \"%s\", "
		  "either it has no DNA or the DNA was not fetched from the server.",
		  g_quark_to_string(block->original_id)) ;

      return dna ;
    }

  block_start = block->block_to_sequence.q1 ;
  block_end = block->block_to_sequence.q2 ;

  dna_data = g_new0(DNASearchDataStruct, 1) ;

  dna_data->window = window ;
  dna_data->block = block ;
  dna_data->dna_start = feature->x1 ;
  dna_data->dna_end = feature->x2 ;
  dna_data->dna_flanking = 0 ;
  if (feature->strand == ZMAPSTRAND_REVERSE)
    dna_data->revcomp = TRUE ;
  else
    dna_data->revcomp = FALSE ;
    

  /* set up the top level window */
  button_text = zmapWindowGetDialogText(dialog_type) ;
  flags |= GTK_DIALOG_MODAL ;
  dna_data->toplevel = toplevel = gtk_dialog_new_with_buttons("Choose DNA Extent", NULL, flags,
							      button_text, GTK_RESPONSE_ACCEPT,
							      "Close", GTK_RESPONSE_NONE,
							      NULL) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(destroyCB), (gpointer)dna_data) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, -1) ;


  /* Add ptrs so parent knows about us */
  g_ptr_array_add(window->dna_windows, (gpointer)toplevel) ;


  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(GTK_BOX(GTK_DIALOG(toplevel)->vbox)), vbox) ;

  menubar = makeMenuBar(dna_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);


  /* Make the box for dna text. */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  frame = gtk_frame_new( "Selected Feature: " );
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0) ;

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;


  dna_data->dna_entry = entry = gtk_entry_new() ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE) ;
  gtk_entry_set_text(GTK_ENTRY(entry), g_quark_to_string(feature->original_id)) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(topbox), entry, FALSE, FALSE, 0) ;

  
  /* Make the start/end boxes. */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
  start_end = makeSpinPanel(dna_data,
			    "Set Start/End coords for DNA export: ",
			    "Start: ", block_start, block_end,
			    feature->x1, GTK_SIGNAL_FUNC(startSpinCB),
			    "End: ", block_start, block_end,
			    feature->x2, GTK_SIGNAL_FUNC(endSpinCB),
			    "Flanking bases: ", 0, (block_end - 1),
			    dna_data->dna_flanking, GTK_SIGNAL_FUNC(flankingSpinCB)) ;
  gtk_box_pack_start(GTK_BOX(hbox), start_end, TRUE, TRUE, 0) ;


  gtk_widget_show_all(toplevel) ;


  /* block waiting for user to answer dialog. */
  result = gtk_dialog_run(GTK_DIALOG(toplevel)) ;


  /* Now get the dna and return it, note we must get entry text before destroying widgets. */
  if (result == GTK_RESPONSE_ACCEPT)
    {
      dna_data->entry_text = (char *)gtk_entry_get_text(GTK_ENTRY(dna_data->dna_entry)) ;
      getDNA(dna_data) ;
      dna = dna_data->dna ;
    }

  gtk_widget_destroy(toplevel) ;

  g_ptr_array_remove(window->dna_windows, (gpointer)toplevel) ;


  return dna ;
}




/*
 *                 Internal functions
 */


static GtkWidget *makeMenuBar(DNASearchData dna_data)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)dna_data) ;

  gtk_window_add_accel_group(GTK_WINDOW(dna_data->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}



static GtkWidget *makeSpinPanel(DNASearchData dna_data,
				char *title,
				char *spin_label, int min, int max, int init, GtkSignalFunc func,
				char *spin_label2, int min2, int max2, int init2, GtkSignalFunc func2,
				char *spin_label3, int min3, int max3, int init3, GtkSignalFunc func3)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *label, *start_spinbox, *end_spinbox, *flanking_spinbox ;


  frame = gtk_frame_new(title);
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  label = gtk_label_new(spin_label) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  start_spinbox =  gtk_spin_button_new_with_range(min, max, 1.0) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(start_spinbox), init) ;
  gtk_signal_connect(GTK_OBJECT(start_spinbox), "value-changed",
		     GTK_SIGNAL_FUNC(func), (gpointer)dna_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), start_spinbox, FALSE, FALSE, 0) ;

  label = gtk_label_new(spin_label2) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  end_spinbox =  gtk_spin_button_new_with_range(min2, max2, 1.0) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(end_spinbox), init2) ;
  gtk_signal_connect(GTK_OBJECT(end_spinbox), "value-changed",
		     GTK_SIGNAL_FUNC(func2), (gpointer)dna_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), end_spinbox, FALSE, FALSE, 0) ;

  label = gtk_label_new(spin_label3) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0) ;

  flanking_spinbox = gtk_spin_button_new_with_range(min3, max3, 1.0) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(flanking_spinbox), init3) ;
  gtk_signal_connect(GTK_OBJECT(flanking_spinbox), "value-changed",
		     GTK_SIGNAL_FUNC(func3), (gpointer)dna_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), flanking_spinbox, FALSE, FALSE, 0) ;


  return frame ;
}



static void getDNA(DNASearchData dna_data)
{
  char *feature_txt = NULL ;
  char *err_text = NULL ;
  char *dna ;
  int start, end, dna_len ;


  /* Convert to relative coords.... */
  start = dna_data->dna_start - dna_data->block->block_to_sequence.q1 + 1 ;
  end = dna_data->dna_end - dna_data->block->block_to_sequence.q1 + 1 ;
  dna = dna_data->block->sequence.sequence ;
  dna_len = strlen(dna) ;

  /* Note that gtk_entry returns "" for no text, _not_ NULL. */
  feature_txt = dna_data->entry_text ;
  feature_txt = g_strdup(feature_txt) ;
  feature_txt = g_strstrip(feature_txt) ;

  if (strlen(feature_txt) == 0)
    err_text = g_strdup("no feature name supplied but is required for FASTA file.") ;
  else if ((start < 0 || start > dna_len)
	   || (end < 0 || end > dna_len)
	   || (start - dna_data->dna_flanking < 0 || end + dna_data->dna_flanking > dna_len))
    err_text = g_strdup_printf("start/end +/- flanking must be within range %d -> %d",
			       dna_data->block->block_to_sequence.q1,
			       dna_data->block->block_to_sequence.q2) ;

  if (err_text)
    {
      zMapMessage("Please correct and retry: %s", err_text) ;
    }
  else
    {
      start = start - dna_data->dna_flanking ;
      end = end + dna_data->dna_flanking ;
      dna_data->dna = zMapFeatureGetDNA(dna_data->block, start, end, dna_data->revcomp) ;
    }

  if (err_text)
    g_free(err_text) ;
  g_free(feature_txt) ;

  return ;
}



/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  char *title = "DNA Export Window" ;
  char *help_text =
    "The ZMap DNA Export Window allows you to export a section of DNA from a block.\n"
    "The fields are filled in with the name and start/end coords of the feature you\n"
    "selected but you can type in your own values if you wish to change any of these fields.\n"
    "You can specify an flanking sequence length." ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}


/* Requests destroy of search window which will end up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  DNASearchData dna_data = (DNASearchData)cb_data ;

  gtk_widget_destroy(dna_data->toplevel) ;

  return ;
}


static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  DNASearchData dna_data = (DNASearchData)cb_data ;

  g_ptr_array_remove(dna_data->window->dna_windows, (gpointer)dna_data->toplevel);

  return ;
}


static void startSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData dna_data = (DNASearchData)user_data ;

  dna_data->dna_start = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}

static void endSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData dna_data = (DNASearchData)user_data ;

  dna_data->dna_end = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}


static void flankingSpinCB(GtkSpinButton *spin_button, gpointer user_data)
{
  DNASearchData dna_data = (DNASearchData)user_data ;

  dna_data->dna_flanking = gtk_spin_button_get_value_as_int(spin_button) ;

  return ;
}

