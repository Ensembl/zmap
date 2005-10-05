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
 * Description: 
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Sep 30 12:06 2005 (edgrif)
 * Created: Fri Aug 12 16:53:21 2005 (edgrif)
 * CVS info:   $Id: zmapWindowSearch.c,v 1.2 2005-10-05 10:54:09 edgrif Exp $
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



static void quitMenuCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void noHelpCB(gpointer data, guint callback_action, GtkWidget *w) ;

static void quitCB(GtkWidget *widget, gpointer cb_data) ;
static void searchCB(GtkWidget *widget, gpointer cb_data) ;
static void displayResult(GList *search_result) ;
static void printListDataCB(gpointer data, gpointer user_data) ;
static void setFieldDefaults(SearchData search_data) ;
static void setFilterDefaults(SearchData search_data) ;




static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Quit",       "<control>Q",  quitMenuCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/One",        NULL,          noHelpCB,      0, NULL,            NULL}
};






void zmapWindowCreateSearchWindow(ZMapWindow window, ZMapFeatureAny feature_any)
{
  FooCanvasItem *parent_item = NULL ;
  GtkWidget *toplevel, *vbox, *menubar, *hbox, *frame,
    *search_button, *scrolledWindow, *fields, *filters ;
  SearchData search_data ;

  search_data = g_new0(SearchDataStruct, 1) ;

  search_data->window = window ;
  search_data->feature_any = feature_any ;

  /* set up the top level window */
  search_data->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  g_signal_connect(GTK_OBJECT(toplevel), "destroy",
		   GTK_SIGNAL_FUNC(quitCB), (gpointer)search_data) ;

  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), "Feature Search") ;
  gtk_window_set_default_size(GTK_WINDOW(toplevel), 500, -1) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = makeMenuBar(search_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0) ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), hbox) ;

  search_button = gtk_button_new_with_label("Search") ;
  gtk_box_pack_start(GTK_BOX(hbox), search_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(search_button), "clicked",
		     GTK_SIGNAL_FUNC(searchCB), (gpointer)search_data) ;

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
  ZMapFeatureAny feature_any = search_data->feature_any ;
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label ;
  char *align_txt = "*", *block_txt = "*", *set_txt = "*", *feature_txt = "*" ;


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
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, TRUE, 0) ;

  search_data->block_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->block_txt) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->set_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->set_txt) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->feature_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->feature_txt) ;
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* set create button as default. */
  GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(app_context->app_widg), create_button) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return frame ;
}


static GtkWidget *makeFiltersPanel(SearchData search_data)
{
  ZMapFeatureAny feature_any = search_data->feature_any ;
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *create_button ;
  char *strand_txt = "+" ;

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
  gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* set create button as default. */
  GTK_WIDGET_SET_FLAGS(create_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(app_context->app_widg), create_button) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return frame ;
}




static void quitMenuCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{

  /* To avoid lots of destroys we call our other quit routine. */
  quitCB(widget, cb_data) ;

  return ;
}


static void noHelpCB(gpointer data, guint callback_action, GtkWidget *w)
{

  printf("sorry, no help\n") ;

  return ;
}


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;

  gtk_widget_destroy(search_data->toplevel) ;

  return ;
}



/* some of this function needs to go into the FTOI package so that we have standard code
 * to go from strings to the ids needed to search.... */
static void searchCB(GtkWidget *widget, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;
  char *align_txt, *block_txt, *strand_txt, *set_txt, *feature_txt  ;
  GQuark align_id, block_id, set_id, feature_id ;
  ZMapStrand strand ;
  GList *search_result ;


  align_txt = block_txt = strand_txt = set_txt = feature_txt = NULL ;
  align_id = block_id = set_id = feature_id = 0 ;


  /* Currently we check to see if the text matches the original id and if it does we use the
   * original unique id, otherwise we generate a new id... */


  /* Note that gtk_entry returns "" for no text, _not_ NULL. */

  /* WE NEED TO DO THIS...THERE IS BOUND TO BE WHITE SPACE.... */
  /* we can use g_strstrip() to get rid of leading/trailing space as in inplace operation
   * on the string...but we should probably copy the strings from the widget first.... */

  align_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->align_entry)) ;
  if (align_txt && strlen(align_txt) == 0)
    {
      align_id = search_data->align_id ;
    }
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
      /* For Strand "*" means no strand or both strands. */
      strand_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->strand_entry)) ;
      if (strand_txt && strlen(strand_txt) == 0)
	strand = ZMAPSTRAND_NONE ;
      else if (strstr(strand_txt, "+"))
	strand = ZMAPSTRAND_FORWARD ;
      else if (strstr(strand_txt, "-"))
	strand = ZMAPSTRAND_REVERSE ;
      else
	strand = ZMAPSTRAND_NONE ;

      set_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->set_entry)) ;
      if (set_txt && strlen(set_txt) == 0)
	{
	  set_id = search_data->set_id ;
	}
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
      else
	{
	  feature_id = g_quark_from_string(feature_txt) ;
	  if (feature_id == search_data->feature_original_id)
	    feature_id = search_data->feature_id ;
	}
    }


  printf("Search parameters -    align: %s   block: %s  strand: %s  set: %s  feature: %s\n",
	 align_txt, block_txt, strand_txt, set_txt, feature_txt) ;


  if ((search_result = zmapWindowFToIFindItemSetFull(search_data->window->context_to_item,
						     align_id, block_id, set_id,
						     strand, feature_id)))
    {
      /* Here we want a more generalised list window... */

      displayResult(search_result) ;
    }

  return ;
}


static void displayResult(GList *search_result)
{

  g_list_foreach(search_result, printListDataCB, NULL) ;

  g_list_free(search_result) ;

  return ;
}

static void printListDataCB(gpointer data, gpointer user_data_unused)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapFeature feature ;

  feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), "item_feature_data");  
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
      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_FEATURE:
	  {
	    ZMapFeature feature = (ZMapFeature)feature_any ;

	    search_data->feature_txt = (char *)g_quark_to_string(feature->original_id) ;
	    search_data->feature_id = feature->unique_id ;
	    search_data->feature_original_id = feature->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  {
	    ZMapFeatureSet set = (ZMapFeatureSet)feature_any ;

	    search_data->set_txt = (char *)g_quark_to_string(set->original_id) ;
	    search_data->set_id = set->unique_id ;
	    search_data->set_original_id = set->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  {
	    ZMapFeatureBlock block = (ZMapFeatureBlock)feature_any ;

	    search_data->block_txt = (char *)g_quark_to_string(block->original_id) ;
	    search_data->block_id = block->unique_id ;
	    search_data->block_original_id = block->original_id ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
	    ZMapFeatureAlignment align = (ZMapFeatureAlignment)feature_any ;

	    search_data->align_txt = (char *)g_quark_to_string(align->original_id) ;
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
	      search_data->strand_txt = "*" ;
	    break ;
	  }
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  {
	    ZMapFeatureSet set = (ZMapFeatureSet)feature_any ;

	    /* We should be able to tell strand from this but we can't at the moment.... */

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  {
	    ZMapFeatureBlock block = (ZMapFeatureBlock)feature_any ;

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
	    ZMapFeatureAlignment align = (ZMapFeatureAlignment)feature_any ;

	    break ;
	  }
	}

      feature_any = feature_any->parent ;
    }

  return ;
}








/*************************** end of file *********************************/
