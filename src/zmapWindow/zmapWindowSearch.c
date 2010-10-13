/*  File: zmapWindowSearch.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements a search window which allows a user to
 *              specify align, block, set and feature patterns to
 *              find sets of features.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Jul 14 14:03 2010 (edgrif)
 * Created: Fri Aug 12 16:53:21 2005 (edgrif)
 * CVS info:   $Id: zmapWindowSearch.c,v 1.47 2010-10-13 15:44:25 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerFeatureSet_I.h>

typedef struct
{
  ZMapWindow window ;
  FooCanvasItem *feature_item ;
  ZMapFeatureAny feature_any ;
  GHashTable *styles ;

  GHashTable *context_to_item;
  ZMapWindowListGetFToIHash get_hash_func;
  gpointer                  get_hash_data;

  /* Context field widgets */
  GtkWidget *toplevel ;
  GtkWidget *align_entry ;
  GtkWidget *block_entry ;
  GtkWidget *set_entry ;
  GtkWidget *feature_entry ;

  /* Filter field widgets */
  GtkWidget *strand_entry ;
  GtkWidget *frame_entry ;
  GtkWidget *start_entry ;
  GtkWidget *end_entry ;
  GtkWidget *locus_but ;
  GtkWidget *style_entry ;


  /* Context field data... */
  char *align_txt ;
  GQuark align_id ;
  GQuark align_original_id ;

  char *block_txt ;
  GQuark block_id ;
  GQuark block_original_id ;

  char *set_txt ;       /* this is the column not the featureset(s) */
  GQuark set_id ;
  GQuark set_original_id ;

  GList *featuresets;   /* defaults to all in the column */

  char *feature_txt ;
  GQuark feature_id ;
  GQuark feature_original_id ;

  /* Filter data. */
  char *strand_txt ;					    /* No need for ids for strand. */
  char *frame_txt ;					    /* No need for ids for frame. */
  char *start ;						    /* Coords range to limit search. */
  char *end ;
  gboolean locus ;
  char *style_text ;
  ZMapFeatureTypeStyle style ;


} SearchDataStruct, *SearchData ;


typedef struct
{
  GList *align_list;
  GList *block_list;
  GList *set_list;
} AllComboListsStruct, *AllComboLists;


typedef struct
{
  gint start ;
  gint end ;

  gboolean locus ;

  ZMapFeatureTypeStyle style ;
} SearchPredCBDataStruct, *SearchPredCBData ;




static GtkWidget *makeMenuBar(SearchData search_data) ;
static GtkWidget *makeFieldsPanel(SearchData search_data) ;
static GtkWidget *makeFiltersPanel(SearchData search_data) ;

static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void searchCB(GtkWidget *widget, gpointer cb_data) ;
static void locusCB(GtkToggleButton *toggle_button, gpointer cb_data) ;

static void setFieldDefaults(SearchData search_data) ;
static void setFilterDefaults(SearchData search_data) ;

static void addToComboBoxText(gpointer list_data, gpointer combo_data);
static void addToComboBoxQuark(gpointer list_data, gpointer combo_data);
static GtkWidget *createPopulateComboBox(GList *list, gboolean quarks);
static void fetchAllComboLists(ZMapFeatureAny feature_any,
                               GList **align_list_out,
                               GList **block_list_out,
                               GList **set_list_out);
static ZMapFeatureContextExecuteStatus fillAllComboList(GQuark key, gpointer data,
                                                        gpointer user_data, char **err_out);

static GList *getStyleQuarks(GHashTable *styles) ;
static void getQuark(gpointer key, gpointer data, gpointer user_data) ;

gboolean searchPredCB(FooCanvasItem *canvas_item, gpointer user_data) ;

GQuark makeCanonID(char *orig_text) ;

/* If TRUE will cause the clipboard to be set to the selected text. */
static gboolean select_region_G = FALSE;

static GtkItemFactoryEntry menu_items_G[] = {
 { "/_File",           NULL,          NULL,          0, "<Branch>",      NULL},
 { "/File/Close",       "<control>W",  requestDestroyCB,    0, NULL,            NULL},
 { "/_Help",           NULL,          NULL,          0, "<LastBranch>",  NULL},
 { "/Help/General",    NULL,          helpCB,      0, NULL,            NULL}
};


static GHashTable *access_window_context_to_item(gpointer user_data)
{
  return ((ZMapWindow)user_data)->context_to_item;
}


void zmapWindowCreateSearchWindow(ZMapWindow                window,
				  ZMapWindowListGetFToIHash get_hash_func,
				  gpointer                  get_hash_data,
				  FooCanvasItem            *feature_item)
{
  ZMapFeatureAny feature_any ;
  GtkWidget *toplevel, *vbox, *menubar, *hbox, *frame,
    *search_button, *fields, *filters, *buttonBox ;
  SearchData search_data ;

  feature_any = zmapWindowItemGetFeatureAny(feature_item);
  zMapAssert(feature_any) ;

  search_data = g_new0(SearchDataStruct, 1) ;

  if(!get_hash_func)
    {
      get_hash_func = access_window_context_to_item;
      get_hash_data = window;
    }

  search_data->window          = window ;
  search_data->get_hash_func   = get_hash_func;
  search_data->get_hash_data   = get_hash_data;
  search_data->context_to_item = (get_hash_func)(get_hash_data);
  search_data->feature_item    = feature_item ;
  search_data->feature_any     = feature_any ;

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
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *combo ;
  GList *alignList, *blockList, *columnList;


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
  gtk_container_set_focus_chain (GTK_CONTAINER(frame), NULL);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);

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
  gtk_container_set_focus_chain (GTK_CONTAINER(entrybox), NULL);
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;

  fetchAllComboLists(search_data->feature_any, &alignList, &blockList, &columnList);

#if MH17_COLUMN
#else
      /* we have a list of featuresets and need to make this a list of columns, removing any duplicates */
  {
      ZMapFeatureSetDesc f2c;
      GList *l = columnList;

      for(columnList = NULL;l ;l = l->next)
      {
            f2c = g_hash_table_lookup(search_data->window->context_map->featureset_2_column, GUINT_TO_POINTER(l->data));
            if(f2c)
            {
                  if(!g_list_find(columnList,GUINT_TO_POINTER(f2c->column_ID)))
                        columnList = g_list_append(columnList,GUINT_TO_POINTER(f2c->column_ID));
                        /* this gets canonicalised on search by manage_quark_from_entry() */
            }
      }
  }

  /* also create a list of featuresets in each column
   * for featuresets submenu to replace style filter */
  /* TDB */

#endif

  combo = createPopulateComboBox(alignList, TRUE) ;
  search_data->align_entry = entry = GTK_BIN(combo)->child;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->align_txt) ;
  gtk_entry_set_editable(GTK_ENTRY(entry), TRUE);
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0) ;
  gtk_editable_set_position(GTK_EDITABLE(entry), 0);
  gtk_box_pack_start(GTK_BOX(entrybox), combo, FALSE, TRUE, 0) ;

  combo = createPopulateComboBox(blockList, TRUE);
  search_data->block_entry = entry = GTK_BIN(combo)->child ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->block_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);

  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, 0) ;

  gtk_box_pack_start(GTK_BOX(entrybox), combo, FALSE, FALSE, 0) ;

  combo = createPopulateComboBox(columnList, TRUE);
  search_data->set_entry = entry = GTK_BIN(combo)->child ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->set_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), combo, FALSE, FALSE, 0) ;

  search_data->feature_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->feature_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;


  return frame ;
}


static GtkWidget *makeFiltersPanel(SearchData search_data)
{
  GtkWidget *frame ;
  GtkWidget *topbox, *hbox, *entrybox, *labelbox, *entry, *label, *combo ;
  ZMapFeatureContext context ;
  GHashTable *styles ;
  GList *style_quarks ;

  context = (ZMapFeatureContext)zMapFeatureGetParentGroup(search_data->feature_any,
							  ZMAPFEATURE_STRUCT_CONTEXT) ;

  search_data->styles = styles = search_data->window->context_map->styles ;
  style_quarks = getStyleQuarks(styles) ;


  setFilterDefaults(search_data) ;

  frame = gtk_frame_new( "Specify Filters: " );
  gtk_frame_set_label_align( GTK_FRAME( frame ), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);


  topbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_border_width(GTK_CONTAINER(topbox), 5) ;
  gtk_container_add (GTK_CONTAINER (frame), topbox) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(topbox), NULL);


  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 0);
  gtk_container_set_focus_chain (GTK_CONTAINER(hbox), NULL);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, TRUE, FALSE, 0) ;

  labelbox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), labelbox, FALSE, FALSE, 0) ;

  label = gtk_label_new( "Strand :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Frame :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Start :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "End :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Locus :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;

  label = gtk_label_new( "Style :" ) ;
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_RIGHT) ;
  gtk_box_pack_start(GTK_BOX(labelbox), label, FALSE, TRUE, 0) ;


  entrybox = gtk_vbox_new(TRUE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), entrybox, TRUE, TRUE, 0) ;
  gtk_container_set_focus_chain (GTK_CONTAINER(entrybox), NULL);

  search_data->strand_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->strand_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->frame_entry = entry = gtk_entry_new() ;
  gtk_entry_set_text(GTK_ENTRY(entry), search_data->frame_txt) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;


  /* Frame is only possible if we are in 3 frame mode. */
  if (!(IS_3FRAME(search_data->window->display_3_frame)))
    gtk_widget_set_sensitive(search_data->frame_entry, FALSE) ;


  search_data->start_entry = entry = gtk_entry_new() ;
  if (search_data->start)
    gtk_entry_set_text(GTK_ENTRY(entry), search_data->start) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;

  search_data->end_entry = entry = gtk_entry_new() ;
  if (search_data->end)
    gtk_entry_set_text(GTK_ENTRY(entry), search_data->end) ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), entry, FALSE, FALSE, 0) ;


  search_data->locus_but = gtk_check_button_new() ;
  g_signal_connect(GTK_OBJECT(search_data->locus_but), "toggled",
		   GTK_SIGNAL_FUNC(locusCB), (gpointer)search_data) ;
  gtk_box_pack_start(GTK_BOX(entrybox), search_data->locus_but, FALSE, FALSE, 0) ;


  combo = createPopulateComboBox(style_quarks, TRUE);
  search_data->style_entry = entry = GTK_BIN(combo)->child ;
  gtk_entry_set_text(GTK_ENTRY(entry), "") ;
  gtk_entry_set_activates_default (GTK_ENTRY(entry), TRUE);
  if(select_region_G)
    gtk_editable_select_region(GTK_EDITABLE(entry), 0, -1) ;
  gtk_box_pack_start(GTK_BOX(entrybox), combo, FALSE, FALSE, 0) ;



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
    "the features for the given align/block/set whose name begins with \"B025\".\n\n"
    "You can add wild cards to any of the fields except the strand and frame filters.\n"
    "The strand filter should be set to one of  +, -, . or *, where * means both strands\n"
    "and . means both strands will be shown if the feature is not strand sensitive,\n"
    "otherwise only the forward strand is shown.\n\n"
    "The frame filter should be set to one of  ., 0, 1, 2 or *, where * means all 3 frames\n"
    "and . the features are not frame sensitive so frame will be ignored.\n" ;

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

/*! Get the quark representation of the entry widget's text...
 *
 * The GtkEntry returns "" for no text, but we translate that to 0
 *
 */
static GQuark entry_get_text_quark(GtkEntry *entry, char *wildcard)
{
  GQuark entry_quark = 0;
  char *entry_text = NULL;

  entry_text = (char *)gtk_entry_get_text(entry);

  if(strlen(entry_text) == 0 || strcmp(entry_text, "0") == 0)
    {
      entry_quark = 0;
    }
  else if(strcmp(entry_text, wildcard) == 0)
    {
      entry_quark = g_quark_from_string(wildcard);
    }
  else if((entry_text = g_strdup(entry_text)))
    {
      entry_text = g_strstrip(entry_text);

      entry_quark = g_quark_from_string(entry_text);

      /* The Quark table has a reference to this now... Free it. */
      g_free(entry_text);
    }

  return entry_quark;
}

/* return either usable, wildcard or canonicalized(user_set) */
static GQuark manage_quark_from_entry(GQuark user_set, GQuark original,
				      GQuark usable,   GQuark wildcard)
{
  GQuark managed_quark = 0;

  managed_quark = user_set;

  if(managed_quark == original)
    managed_quark = usable;
  else if(managed_quark != wildcard)
    {
      char *canonicalize = (char *)g_quark_to_string(user_set);
      managed_quark = makeCanonID(canonicalize);
    }

  return managed_quark;
}


/* some of this function needs to go into the FTOI package so that we have standard code
 * to go from strings to the ids needed to search.... */
static void searchCB(GtkWidget *widget, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;
  char *align_txt, *block_txt, *strand_txt, *frame_txt, *set_txt, *feature_txt  ;
  char *start_txt, *end_text ;
  gboolean locus ;
  GQuark align_id, block_id, set_id, feature_id ;
  char *strand_spec, *frame_spec ;
  ZMapWindowFToIPredFuncCB callback = NULL ;
  SearchPredCBDataStruct search_pred = {0} ;
  SearchPredCBData search_pred_ptr = NULL ;
  char *wild_card_str = "*" ;
  GQuark wild_card_id ;
  char *style_text ;


  wild_card_id = g_quark_from_string(wild_card_str) ;

  align_txt = block_txt = strand_txt = frame_txt = set_txt = feature_txt = NULL ;
  align_id = block_id = set_id = feature_id = 0 ;


  /* Currently we check to see if the text matches the original id and if it does we use the
   * original unique id, otherwise we generate a new id... */


  /* Note that gtk_entry returns "" for no text, _not_ NULL. */

  /* Note also that the pointer gtk_entry returns _must_ _not_ be modified.
   * Please g_strdup any strings you wish to do that with... */


  /* Needed in more than one place. */
  start_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->start_entry)) ;
  end_text = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->end_entry)) ;

  if((align_id = entry_get_text_quark(GTK_ENTRY(search_data->align_entry), wild_card_str)) != 0)
    {
      /* fix up align_id */
      align_id = manage_quark_from_entry(align_id, search_data->align_original_id,
					 search_data->align_id, wild_card_id);

      /* fix up block_id */
      if((block_id = entry_get_text_quark(GTK_ENTRY(search_data->block_entry), wild_card_str)) != 0)
	{
	  block_id = manage_quark_from_entry(block_id, search_data->block_original_id,
					     search_data->block_id, wild_card_id);

	  /* fix up set_id NB: this is the column id */
	  if((set_id = entry_get_text_quark(GTK_ENTRY(search_data->set_entry), wild_card_str)) != 0)
	    {
	      set_id = manage_quark_from_entry(set_id, search_data->set_original_id,
					       search_data->set_id, wild_card_id);

	      /* fix up feature_id */
	      if((feature_id = entry_get_text_quark(GTK_ENTRY(search_data->feature_entry), wild_card_str)) != 0)
		{
		  if(feature_id != wild_card_id)
		    {
		      int feature_len;
		      char *tmp_txt = NULL;

		      feature_txt = (char *)g_quark_to_string(feature_id);
		      feature_len = strlen(feature_txt);

		      if(strcmp(feature_txt + feature_len - 1, wild_card_str) == 0)
			tmp_txt = g_strdup(feature_txt);
		      else
			tmp_txt = g_strdup_printf("%s*", feature_txt);

		      feature_id = makeCanonID(tmp_txt);

		      g_free(tmp_txt);
		    }
		}
	    }
	}
    }

  /* For strand, "." is ZMAPSTRAND_NONE which means search forward strand columns,
   * "*" means search forward and reverse columns. */
  strand_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->strand_entry)) ;
  if (strand_txt && (strlen(strand_txt) == 0 || strstr(strand_txt, ".")))
    strand_spec = "." ;
  else if (strstr(strand_txt, "+"))
    strand_spec = "+" ;
  else if (strstr(strand_txt, "-"))
    strand_spec = "-" ;
  else
    strand_spec = "*" ;


  /* For frame, "." is ZMAPFRAME_NONE which means there are no frame specific columns,
   * "*" means search all three frame columns on a strand. */
  frame_txt = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->frame_entry)) ;
  if (frame_txt && (strlen(frame_txt) == 0 || strstr(frame_txt, ".")))
    frame_spec = "." ;
  else if (strstr(frame_txt, "1"))
    frame_spec = "1" ;
  else if (strstr(frame_txt, "2"))
    frame_spec = "2" ;
  else if (strstr(frame_txt, "3"))
    frame_spec = "3" ;
  else
    frame_spec = "*" ;


  /* Get predicate stuff, start/end/locus currently. */
  locus = search_data->locus ;
  style_text = (char *)gtk_entry_get_text(GTK_ENTRY(search_data->style_entry)) ;

  if ((start_txt && *start_txt && end_text && *end_text) || locus || (style_text && *style_text))
    {
      callback          = searchPredCB ;
      search_pred_ptr   = &search_pred ;

      search_pred.start = atoi(start_txt) ;

      search_pred.end   = atoi(end_text) ;

      search_pred.locus = locus ;

      if (style_text && *style_text)
	{
	  ZMapFeatureTypeStyle style ;

	  if ((style = zMapFindStyle(search_data->styles, zMapStyleCreateID(style_text))))
	    search_pred.style = style ;
	}
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Left for debugging.... */
  printf("Search parameters -    align: %s   block: %s  strand: %s  frame: %s  set: %s  feature: %s\n",
	 align_txt, block_txt, strand_spec, frame_spec, set_txt, feature_txt) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#define USING_SET_SEARCH_DATA_METHOD
#ifndef USING_SET_SEARCH_DATA_METHOD
  if ((search_result = zmapWindowFToIFindItemSetFull(search_data->window,search_data->context_to_item,
						     align_id, block_id, set_id,
						     strand_spec, frame_spec,
						     feature_id,
						     callback, search_pred_ptr)))
    {

      /* What the code is trying to trap here is the sort of item returned...
       * the search function will simply return a list of items which could be blocks,
       * aligns, columns or features, in practice we probably only want to handle
       * features for now....for now we will just warn the user that we don't
       * support items returned other than features.... */
      GList *list_item ;
      FooCanvasItem *item ;
      ZMapFeatureAny any_feature ;
      gboolean zoom_to_item = TRUE;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debugging.... */
      displayResult(search_result) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#ifndef REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION
      zoom_to_item = FALSE;
#endif /* REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION */

      /* Get hold of the first canvas item in the returned list.... */
      list_item   = g_list_first(search_result) ;
      item        = (FooCanvasItem *)list_item->data ;
      any_feature = zmapWindowItemGetFeatureAny(item);
      zMapAssert(any_feature) ;

      if (any_feature->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
	{
	  char *title = NULL;
        if(!feature_text)
            feature_text = g_quark_to_string(set_id);
	  title = g_strdup_printf("Results '%s'", feature_txt);
	  zmapWindowListWindow(search_data->window,
			       NULL, title,
			       search_data->get_hash_func,
			       search_data->get_hash_data,
			       NULL, NULL, NULL,
			       zoom_to_item);
	  if(title)
	    g_free(title);
	}
      else
	{
	  zMapMessage("Sorry, list windows of %ss within a feature context not currently supported.",
		      zMapFeatureStructType2Str(any_feature->struct_type)) ;
	}

      g_list_free(search_result) ;

    }
#else	/* !USING_SET_SEARCH_DATA_METHOD */
  if(1)
    {
      ZMapWindowFToISetSearchData search_set_data = NULL;
      SearchPredCBData search_pred_data = NULL ;
      gboolean zoom_to_item = TRUE;
      char *title = NULL;

      title = g_strdup_printf("Results '%s'", feature_txt);

      if(callback && search_pred_ptr)
	{
	  search_pred_data        = g_new0(SearchPredCBDataStruct, 1);
	  search_pred_data->start = search_pred_ptr->start;
	  search_pred_data->end   = search_pred_ptr->end;
	  search_pred_data->locus = search_pred_ptr->locus;
	  search_pred_data->style = search_pred_ptr->style;
	}

      search_set_data = zmapWindowFToISetSearchCreateFull(zmapWindowFToIFindItemSetFull, NULL,
							  align_id, block_id, set_id, feature_id,
							  strand_spec, frame_spec,
							  callback, search_pred_data, g_free);

#ifndef REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION
      zoom_to_item = FALSE;
#endif /* REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION */

      zmapWindowListWindow(search_data->window, NULL, title,
			   search_data->get_hash_func,
			   search_data->get_hash_data,
			   (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform,
			   search_set_data,
			   (GDestroyNotify)zmapWindowFToISetSearchDestroy,
			   zoom_to_item);

    }
#endif /* USING_SET_SEARCH_DATA_METHOD */
  else
    {
      char *msg = g_strdup_printf("No Results for:\n"
				  "\n     align  =  \"%s\""
				  "\n     block  =  \"%s\""
				  "\n       set  =  \"%s\""
				  "\n    strand  =  \"%s\""
				  "\n    frame  =  \"%s\""
				  "\n   feature  =  \"%s\"",
				  align_txt, block_txt, set_txt, strand_spec, frame_spec, feature_txt) ;

      zMapGUIShowMsgFull(NULL, msg, ZMAP_MSG_INFORMATION, GTK_JUSTIFY_LEFT, 0, TRUE) ;
							    /* Format msg for clarity. */
      g_free(msg) ;
    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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

  feature = zmapWindowItemGetFeature(item);
  zMapAssert(feature) ;

  printf("%s\n", g_quark_to_string(feature->unique_id)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void setFieldDefaults(SearchData search_data)
{
  ZMapFeatureAny feature_any = search_data->feature_any ;
  char *wild_card_str = "*" ;
  GQuark wild_card_id ;

  wild_card_id = g_quark_from_string(wild_card_str) ;

  search_data->align_txt = search_data->block_txt = search_data->strand_txt = search_data->frame_txt
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

#if MH17_COLUMN
	    search_data->set_txt = (char *)g_quark_to_string(set->original_id) ;
	    search_data->set_id = set->unique_id ;
	    search_data->set_original_id = set->original_id ;
#else
            /* need to get the column that the featureset is in
             * for when we search the hash
             */
          ZMapFeatureSetDesc f2c;

          f2c = g_hash_table_lookup(search_data->window->context_map->featureset_2_column, GUINT_TO_POINTER(set->unique_id));
          if(f2c)
          {
            search_data->set_txt = (char *) g_quark_to_string(f2c->column_ID) ;
            search_data->set_id = f2c->column_id;
            search_data->set_original_id = f2c->column_ID;
          }
#endif
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
        case ZMAPFEATURE_STRUCT_CONTEXT:
          break;
        case ZMAPFEATURE_STRUCT_INVALID:
        default:
          zMapAssertNotReached();
          break;
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

  search_data->strand_txt = search_data->frame_txt = wild_card_str ;


  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE
      || feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET)
    {
      FooCanvasGroup *featureset_group ;
      ZMapWindowContainerFeatureSet container;

      if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
	{
	  featureset_group = (FooCanvasGroup *)zmapWindowContainerCanvasItemGetContainer(search_data->feature_item) ;
	}
      else
	featureset_group = FOO_CANVAS_GROUP(search_data->feature_item) ;

      container = (ZMapWindowContainerFeatureSet)(featureset_group);

      if (container->strand == ZMAPSTRAND_FORWARD)
	search_data->strand_txt = "+" ;
      else if (container->strand == ZMAPSTRAND_REVERSE)
	search_data->strand_txt = "-" ;
      else
	search_data->strand_txt = "." ;

      if (container->frame == ZMAPFRAME_NONE)
	search_data->frame_txt = "." ;
      else if (container->frame == ZMAPFRAME_0)
	search_data->frame_txt = "1" ;
      else if (container->frame == ZMAPFRAME_1)
	search_data->frame_txt = "2" ;
      else if (container->frame == ZMAPFRAME_2)
	search_data->frame_txt = "3" ;
    }



  /* The loop is unecessary at the moment but may need it later on ..... */
  while (feature_any->parent)
    {
      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_FEATURE:
	  {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    /* I'm commenting this out for now, its just kind of annoying in fact.... */

	    ZMapFeature feature = (ZMapFeature)feature_any ;
	    char *coord = NULL ;

	    if (zMapInt2Str(feature->x1, &coord))
	      search_data->start = coord ;		    /* str needs to be freed... */

	    if (zMapInt2Str(feature->x2, &coord))
	      search_data->end = coord ;		    /* str needs to be freed... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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
        case ZMAPFEATURE_STRUCT_CONTEXT:
          break;
        case ZMAPFEATURE_STRUCT_INVALID:
        default:
          zMapAssertNotReached();
          break;
	}

      feature_any = feature_any->parent ;
    }

  return ;
}

static GtkWidget *createPopulateComboBox(GList *list, gboolean quarks)
{
  GtkWidget *combo = NULL;

  combo = gtk_combo_box_entry_new_text();

  if(quarks)
    g_list_foreach(list, addToComboBoxQuark, (gpointer)combo);
  else
    g_list_foreach(list, addToComboBoxText, (gpointer)combo);

  return combo;
}

static ZMapFeatureContextExecuteStatus fillAllComboList(GQuark key, gpointer data,
                                                        gpointer user_data, char **err_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  AllComboLists all_data = (AllComboLists)user_data;
  ZMapFeatureStructType feature_type = ZMAPFEATURE_STRUCT_INVALID;
  ZMapFeatureAlignment align = NULL;
  ZMapFeatureBlock     block = NULL;
  ZMapFeatureSet         set = NULL;

  feature_type = feature_any->struct_type;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      align = (ZMapFeatureAlignment)feature_any;
      all_data->align_list = g_list_prepend(all_data->align_list, GUINT_TO_POINTER(key));
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      block = (ZMapFeatureBlock)feature_any;
      all_data->block_list = g_list_prepend(all_data->block_list, GUINT_TO_POINTER(key));
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      set   = (ZMapFeatureSet)feature_any;
      all_data->set_list   = g_list_prepend(all_data->set_list,   GUINT_TO_POINTER(key));
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapAssertNotReached();
      break;
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}

static void fetchAllComboLists(ZMapFeatureAny feature_any,
                               GList **align_list_out,
                               GList **block_list_out,
                               GList **set_list_out)
{
  AllComboListsStruct all_data = { NULL };

  zMapFeatureContextExecute(feature_any, ZMAPFEATURE_STRUCT_FEATURESET, fillAllComboList, &all_data);

  if(align_list_out)
    *align_list_out = all_data.align_list;
  if(block_list_out)
    *block_list_out = all_data.block_list;
  if(set_list_out)
    *set_list_out = all_data.set_list;
  return ;
}

static void addToComboBoxText(gpointer list_data, gpointer combo_data)
{
  GtkWidget *combo = (GtkWidget *)combo_data;
  char *text = (char *)list_data;

  gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);

  return ;
}

static void addToComboBoxQuark(gpointer list_data, gpointer combo_data)
{
  GtkWidget *combo = (GtkWidget *)combo_data;
  GQuark textId = GPOINTER_TO_UINT(list_data);
  const char *text = NULL;

  if(textId)
    {
      text = g_quark_to_string(textId);
      gtk_combo_box_append_text(GTK_COMBO_BOX(combo), text);
    }

  return ;
}



gboolean searchPredCB(FooCanvasItem *canvas_item, gpointer user_data)
{
  gboolean result = FALSE ;
  SearchPredCBData search_pred = (SearchPredCBData)user_data ;
  ZMapFeatureAny feature_any ;

  feature_any = zmapWindowItemGetFeatureAny(canvas_item);
  zMapAssert(feature_any && zMapFeatureIsValid(feature_any)) ;


  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
    case ZMAPFEATURE_STRUCT_FEATURESET:
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {
	ZMapFeature feature = (ZMapFeature)feature_any ;


	/* for features we apply various filters according to what has been set. */
	if (search_pred->start || search_pred->end)
	  {
	    if (feature->x1 > search_pred->end || feature->x2 < search_pred->start)
	      result = FALSE ;
	    else
	      result = TRUE ;
	  }

	if (search_pred->locus)
	  {
	    if (feature->locus_id)
	      result = TRUE ;
	    else
	      result = FALSE ;
	  }

	if (search_pred->style)
	  {
	    if (feature->style_id == zMapStyleGetID(search_pred->style))
	      result = TRUE ;
	    else
	      result = FALSE ;
	  }

	break;
      }
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      zMapAssertNotReached();
      break;

    }



  return result ;
}


/* copies orig_text, canonicalises it and returns the quark for the text. */
GQuark makeCanonID(char *orig_text)
{
  GQuark canon_id= 0 ;
  char *canon_text ;

  canon_text = g_strdup(orig_text) ;

  canon_text = zMapFeatureCanonName(canon_text) ;

  canon_id = g_quark_from_string(canon_text) ;

  return canon_id ;
}



static void locusCB(GtkToggleButton *toggle_button, gpointer cb_data)
{
  SearchData search_data = (SearchData)cb_data ;

  search_data->locus = gtk_toggle_button_get_active(toggle_button) ;

  return ;
}



static GList *getStyleQuarks(GHashTable *styles)
{
  GList *style_quarks = NULL ;

  g_hash_table_foreach(styles, getQuark, &style_quarks) ;

  return style_quarks ;
}



static void getQuark(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  GList *style_quarks = *((GList **)user_data) ;

  style_quarks = g_list_append(style_quarks, GINT_TO_POINTER(zMapStyleGetID(style))) ;

  *((GList **)user_data) = style_quarks ;

  return ;
}

/*************************** end of file *********************************/
