/*  File: zmapWindowSearch.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Implements a search window which allows a user to
 *              specify align, block, set and feature patterns to
 *              find sets of features.
 *              
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: May 22 14:14 2006 (edgrif)
 * Created: Fri Aug 12 16:53:21 2005 (edgrif)
 * CVS info:   $Id: zmapWindowSearch.c,v 1.10 2006-05-22 13:27:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

typedef struct
{
  ZMapWindow window ;
  ZMapFeatureAny feature_any ;

  GtkWidget *toplevel ;
  GtkWidget *align_entry ;
  GtkWidget *block_entry ;
  GtkWidget *strand_entry ;
  GtkWidget *set_entry ;
  GtkWidget *feature_entry ;

  char *align_txt ;
  GQuark align_id ;
  GQuark align_original_id ;

  char *block_txt ;
  GQuark block_id ;
  GQuark block_original_id ;

  char *set_txt ;
  GQuark set_id ;
  GQuark set_original_id ;

  char *feature_txt ;
  GQuark feature_id ;
  GQuark feature_original_id ;

  /* should make this into set of filters, e.g. strand, position etc..... */
  char *strand_txt ;					    /* No need for ids for strand. */

} SearchDataStruct, *SearchData ;



static GtkWidget *makeMenuBar(SearchData search_data) ;
static GtkWidget *makeFieldsPanel(SearchData search_data) ;
static GtkWidget *makeFiltersPanel(SearchData search_data) ;

static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void searchCB(GtkWidget *widget, gpointer cb_data) ;

static void displayResult(GList *search_result) ;
static void printListDataCB(gpointer data, gpointer user_data) ;
static void setFieldDefaults(SearchData search_data) ;
static void setFilterDefaults(SearchData search_data) ;




static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Close",       "<control>W",  requestDestroyCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/One",        NULL,          helpCB,      0, NULL,            NULL}
};






void zmapWindowCreateSearchWindow(ZMapWindow window, ZMapFeatureAny feature_any)
{
  GtkWidget *toplevel, *vbox, *menubar, *hbox, *frame,
    *search_button, *fields, *filters, *buttonBox ;
  SearchData search_data ;

  search_data = g_new0(SearchDataStruct, 1) ;

  search_data->window = window ;
  search_data->feature_any = feature_any ;

  /* set up the top level window */
  search_data->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(destroyCB), (gpointer)search_data) ;

  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "Feature Search") ;
  gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, -1) ;

  /* Add ptrs so parent knows about us */
  g_ptr_array_add(window->search_windows, (gpointer)toplevel) ;


  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = makeMenuBar(search_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  hbox = gtk_hbox_new(FALSE, 0) ;

  /* Make the main search boxes for Align, Block etc. */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  fields = makeFieldsPanel(search_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), fields, TRUE, TRUE, 0) ;

  /* Make the filter boxes. */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  filters = makeFiltersPanel(search_data) ;
  gtk_box_pack_start(GTK_BOX(hbox), filters, TRUE, TRUE, 0) ;

  buttonBox = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (buttonBox), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(buttonBox), 
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (buttonBox), 
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  search_button = gtk_button_new_with_label("Search") ;
  gtk_container_add(GTK_CONTAINER(buttonBox), search_button) ;
  gtk_signal_connect(GTK_OBJECT(search_button), "clicked",
		     GTK_SIGNAL_FUNC(searchCB), (gpointer)search_data) ;
  /* set search button as default. */
  GTK_WIDGET_SET_FLAGS(search_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(toplevel), search_button) ;

  frame = gtk_frame_new("") ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), 
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0) ;

  gtk_container_add(GTK_CONTAINER(frame), buttonBox);

  gtk_widget_show_all(toplevel) ;

  return ;
}


/*
 *                 Internal functions
 */


GtkWidget *makeMenuBar(SearchData search_data)
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


static GtkWidget *makeFieldsPanel(SearchData search_data)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;


  /* Need to set up the default values for the text using feature_any, we should
   * also keep a record of the original default values so we can restore them...
   * Can we also easily detect which values have been editted...then we could ignore
   * them and use the defaults... */
  setFieldDefaults(search_data) ;

  frame = gtk_frame_new( "Specify Search: " );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);


  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Align :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Block :" ) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;

  label = gtk_label_new( "Column :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Feature :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  search_data->align_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->align_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  search_data->block_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->block_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->set_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->set_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->feature_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->feature_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;


  return frame ;
}


static GtkWidget *makeFiltersPanel(SearchData search_data)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label;

  setFilterDefaults(search_data) ;


  frame = gtk_frame_new( "Specify Filters: " );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);


  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;


  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Strand :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  search_data->strand_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->strand_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  return frame ;
}




/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  char *title = "Feature Search Window" ;
  char *help_text =
    "The ZMap Search Window allows you to search for features using simple filtering.\n"
    "When initially show the search window displays the names it has assigned to the\n"
    "displayed alignment, block, set and the feature you clicked on. You can either\n"
    "replace or augment these names with the \"*\" wild card to find sets of features,\n"
    "e.g. if the feature name is \"B0250\" then changing it to \"B025*\" will find all\n"
    "the features for the given align/block/set whose name begins with \"B025\". You can\n"
    "add wild cards to any of the fields except the strand filter. The strand filter should be\n"
    "set to one of  +, -, . or *, where * means both strands and . means both strands will be\n"
    "shown if the feature is not strand sensitive, otherwise only the forward strand is shown." ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}



/* Requests destroy of search window which will end up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  SearchData search_data = (SearchData)cb_data ;

  gtk_widget_destroy(search_data->toplevel) ;

  return ;
}



static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;

  g_ptr_array_remove(search_data->window->search_windows, (gpointer)search_data->toplevel);

  return ;
}






/* some of this function needs to go into the FTOI package so that we have standard code
 * to go from strings to the ids needed to search.... */
static void searchCB(GtkWidget *widget, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;
  char *align_txt, *block_txt, *strand_txt, *set_txt, *feature_txt  ;
  GQuark align_id, block_id, set_id, feature_id ;
  char *strand_spec ;
  GList *search_result ;


  align_txt = block_txt = strand_txt = set_txt = feature_txt = NULL ;
  align_id = block_id = set_id = feature_id = 0 ;


  /* Currently we check to see if the text matches the original id and if it does we use the
   * original unique id, otherwise we generate a new id... */


  /* Note that gtk_entry returns "" for no text, _not_ NULL. */

  /* WE NEED TO DO THIS...THERE IS BOUND TO BE WHITE SPACE.... */
  /* we can use g_strstrip() to get rid of leading/trailing space as in inplace operation
   * on the string...but we should probably copy the strings from the widget first.... */


  /* STOP ALL THIS DUPLICATION OF CODE... */
  align_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->align_entry)) ;
  if (align_txt && strlen(align_txt) == 0)
    {
      align_id = search_data->align_id ;
    }
  else if (strcmp(align_txt, "0") == 0)
    align_id = 0 ;
  else
    {
      /* We could do a g_quark_try_string() to see if the id exists which would be some help... */

      align_id = g_quark_from_string(align_txt) ;
      if (align_id == search_data->align_original_id)
	align_id = search_data->align_id ;
    }

  if (align_id)
    {
      block_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->block_entry)) ;
      if (block_txt && strlen(block_txt) == 0)
	{
	  block_id = search_data->block_id ;
	}
      else if (strcmp(block_txt, "0") == 0)
	block_id = 0 ;
      else
	{
	  /* We could do a g_quark_try_string() to see if the id exists which would be some help... */
	  block_id = g_quark_from_string(block_txt) ;
	  if (block_id == search_data->block_original_id)
	    block_id = search_data->block_id ;
	}
    }


  if (block_id)
    {
      set_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->set_entry)) ;
      if (set_txt && strlen(set_txt) == 0)
	{
	  set_id = search_data->set_id ;
	}
      else if (strcmp(set_txt, "0") == 0)
	set_id = 0 ;
     else
	{
	  /* We could do a g_quark_try_string() to see if the id exists which would be some help... */
	  set_id = g_quark_from_string(set_txt) ;
	  if (set_id == search_data->set_original_id)
	    set_id = search_data->set_id ;
	}
    }

  if (set_id)
    {
      feature_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->feature_entry)) ;
      if (feature_txt && strlen(feature_txt) == 0)
	{
	  feature_id = search_data->feature_id ;
	}
      else if (strcmp(feature_txt, "0") == 0)
	feature_id = 0 ;
      else
	{
	  feature_id = g_quark_from_string(feature_txt) ;
	  if (feature_id == search_data->feature_original_id)
	    feature_id = search_data->feature_id ;
	}
    }



  /* For strand, "." is ZMAPSTRAND_NONE which means search forward strand columns,
   * "*" means search forward and reverse columns. */
  strand_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->strand_entry)) ;
  if (strand_txt && strlen(strand_txt) == 0)
    strand_spec = "." ;
  else if (strstr(strand_txt, "+"))
    strand_spec = "+" ;
  else if (strstr(strand_txt, "-"))
    strand_spec = "-" ;
  else
    strand_spec = "*" ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Left for debugging.... */
  printf("Search parameters -    align: %s   block: %s  strand: %s  set: %s  feature: %s\n",
	 align_txt, block_txt, strand_spec, set_txt, feature_txt) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  if ((search_result = zmapWindowFToIFindItemSetFull(search_data->window->context_to_item,
						     align_id, block_id, set_id,
						     strand_spec, feature_id)))
    {

      /* What the code is trying to trap here is the sort of item returned...
       * the search function will simply return a list of items which could be blocks,
       * aligns, columns or features, in practice we probably only want to handle
       * features for now....for now we will just warn the user that we don't
       * support items returned other than features.... */
      GList *list_item ;
      FooCanvasItem *item ;
      ZMapFeatureAny any_feature ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debugging.... */
      displayResult(search_result) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* Get hold of the first canvas item in the returned list.... */
      list_item = g_list_first(search_result) ;
      item = (FooCanvasItem *)list_item->data ;
      any_feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
      zMapAssert(any_feature) ;

      if (any_feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
	{
	  zmapWindowListWindowCreate(search_data->window, search_result, 
				     g_strdup_printf("Results '%s'", feature_txt), 
				     NULL) ;
	}
      else
	{
	  zMapMessage("Sorry, list windows of %ss within a feature context not currently supported.", 
		      zMapFeatureStructType2Str(any_feature->struct_type)) ;
	}

      g_list_free(search_result) ;

    }
  else
    {
      char *msg = g_strdup_printf("No Results for:\n"
		  "\n     align  =  \"%s\""
		  "\n     block  =  \"%s\""
		  "\n       set  =  \"%s\""
		  "\n    strand  =  \"%s\""
		  "\n   feature  =  \"%s\"",
		  align_txt, block_txt, strand_spec, set_txt, feature_txt) ;

      zMapGUIShowMsgFull(NULL, msg, ZMAP_MSG_INFORMATION, GTK_JUSTIFY_LEFT) ;
							    /* Format msg for clarity. */
      g_free(msg) ;
    }

  return ;
}


/* debugging aid, prints results to stdout. */
static void displayResult(GList *search_result)
{
  g_list_foreach(search_result, printListDataCB, NULL) ;

  return ;
}

static void printListDataCB(gpointer data, gpointer user_data_unused)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapFeature feature ;

  feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);  
  zMapAssert(feature) ;

  printf("%s\n", g_quark_to_string(feature->unique_id)) ;

  return ;
}


static void setFieldDefaults(SearchData search_data)
{
  ZMapFeatureAny feature_any = search_data->feature_any ;
  char *wild_card_str = "*" ;
  GQuark wild_card_id ;

  wild_card_id = g_quark_from_string(wild_card_str) ;

  search_data->align_txt = search_data->block_txt = search_data->strand_txt
    = search_data->set_txt = search_data->feature_txt = wild_card_str ;

  search_data->align_id = search_data->block_id
    = search_data->set_id = search_data->feature_id = wild_card_id ;

  search_data->align_original_id = search_data->block_original_id
    = search_data->set_original_id = search_data->feature_original_id = 0 ;

  /* Loop up the feature context hierachy setting default values as we go. Where we start
   * in the hierachy depends on what level feature we get passed. */
  while (feature_any->parent)
    {

      /* OK, I'm changing from showing the original_id to the unique_id here because
       * wild card searches cannot work with original_id's as they are not in the hash... */

      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_FEATURE:
	  {
	    ZMapFeature feature = (ZMapFeature)feature_any ;

	    search_data->feature_txt = (char *)g_quark_to_string(feature->unique_id) ;
	    search_data->feature_id = feature->unique_id ;
	    search_data->feature_original_id = feature->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  {
	    ZMapFeatureSet set = (ZMapFeatureSet)feature_any ;

	    search_data->set_txt = (char *)g_quark_to_string(set->unique_id) ;
	    search_data->set_id = set->unique_id ;
	    search_data->set_original_id = set->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  {
	    ZMapFeatureBlock block = (ZMapFeatureBlock)feature_any ;

	    search_data->block_txt = (char *)g_quark_to_string(block->unique_id) ;
	    search_data->block_id = block->unique_id ;
	    search_data->block_original_id = block->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
	    ZMapFeatureAlignment align = (ZMapFeatureAlignment)feature_any ;

	    search_data->align_txt = (char *)g_quark_to_string(align->unique_id) ;
	    search_data->align_id = align->unique_id ;
	    search_data->align_original_id = align->original_id ;
	    break ;
	  }
	}

      feature_any = feature_any->parent ;
    }

  return ;
}




static void setFilterDefaults(SearchData search_data)
{
  ZMapFeatureAny feature_any = search_data->feature_any ;
  char *wild_card_str = "*" ;
  GQuark wild_card_id ;

  wild_card_id = g_quark_from_string(wild_card_str) ;

  search_data->strand_txt = wild_card_str ;

  /* The loop is unecessary at the moment but may need it later on ..... */
  while (feature_any->parent)
    {
      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_FEATURE:
	  {
	    ZMapFeature feature = (ZMapFeature)feature_any ;

	    if (feature->strand == ZMAPSTRAND_FORWARD)
	      search_data->strand_txt = "+" ;
	    if (feature->strand == ZMAPSTRAND_REVERSE)
	      search_data->strand_txt = "-" ;
	    else
	      search_data->strand_txt = "." ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  {
#ifdef RDS_DONT_INCLUDE
	    ZMapFeatureSet set = (ZMapFeatureSet)feature_any ;
#endif /* RDS_DONT_INCLUDE */
	    /* We should be able to tell strand from this but we can't at the moment.... */

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  {
#ifdef RDS_DONT_INCLUDE
	    ZMapFeatureBlock block = (ZMapFeatureBlock)feature_any ;
#endif /* RDS_DONT_INCLUDE */

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
#ifdef RDS_DONT_INCLUDE
	    ZMapFeatureAlignment align = (ZMapFeatureAlignment)feature_any ;
#endif /* RDS_DONT_INCLUDE */

	    break ;
	  }
	}

      feature_any = feature_any->parent ;
    }

  return ;
}








/*************************** end of file *********************************/
