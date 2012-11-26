/*  File: zmapWindowDNAList.c
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
 * Description: Shows a list of dna locations that can be selected
 *              causing the zmapwindow to scroll to that location.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapBase.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapSequence.h>
#include <ZMap/zmapDNA.h>
#include <zmapWindow_P.h>
#include <zmapWindowDNAList_I.h>


typedef struct _ZMapWindowListStruct
{
  ZMapWindow        window ;

  char             *title ;
  char *match_details ;

  GtkWidget        *view  ;
  GtkWidget        *toplevel ;
  GtkWidget        *tree_widget;

  GList            *dna_match_list ;
  ZMapWindowDNAList dna_list;

  ZMapFeatureBlock  block;
  ZMapFeatureSet match_feature_set ;

} DNAWindowListDataStruct, *DNAWindowListData ;



static GtkWidget *createToplevel(char *title) ;
static void drawListWindow(DNAWindowListData windowList, GtkWidget *tree_view);
static GtkWidget *makeMenuBar(DNAWindowListData wlist);

static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget);
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget);
static void destroyCB(GtkWidget *window, gpointer user_data);
static gboolean selectionFuncCB(GtkTreeSelection *selection,
                                GtkTreeModel     *model,
                                GtkTreePath      *path,
                                gboolean          path_currently_selected,
                                gpointer          user_data);
static void freeDNAMatchCB(gpointer data, gpointer user_data_unused) ;



/* 
 *                       Globals
 */


/* menu GLOBAL! */
static GtkItemFactoryEntry menu_items_G[] = {
 /* File */
 { "/_File",              NULL,         NULL,       0,          "<Branch>", NULL},
 { "/File/Close",         "<control>W", requestDestroyCB, 0,          NULL,       NULL},
 /* Help */
 { "/_Help",             NULL, NULL,       0,            "<LastBranch>", NULL},
 { "/Help/Overview", NULL, helpMenuCB, 0,  NULL,           NULL}
};




/* 
 *                     External routines
 */


/* Displays a list of selectable DNA locations
 *
 * All the features the chosen column are displayed in ascending start-coordinate
 * sequence.  When the user selects one, the main display is scrolled to that feature
 * and the selected item highlighted.
 *
 */
void zmapWindowDNAListCreate(ZMapWindow zmap_window, GList *dna_list,
			     char *ref_seq_name, char *match_sequence, char *match_details,
			     ZMapFeatureBlock block, ZMapFeatureSet match_feature_set)
{
  DNAWindowListData window_list ;
  GString *title_str ;
  enum {MAX_LEN = 20} ;

  window_list = g_new0(DNAWindowListDataStruct, 1) ;

  window_list->window = zmap_window ;
  window_list->dna_match_list = dna_list ;
  window_list->dna_list = zMapWindowDNAListCreate();
  window_list->block = block;
  window_list->match_feature_set = match_feature_set ;

  title_str = g_string_new(ref_seq_name) ;
  title_str = g_string_append(title_str, ": \"") ;
  if (strlen(match_sequence) <= MAX_LEN)
    {
      g_string_append_printf(title_str, "%s\"", match_sequence) ;
    }
  else
    {
      title_str = g_string_append_len(title_str, match_sequence, MAX_LEN) ;
      title_str = g_string_append(title_str, "...\"") ;
    }
  window_list->title = g_string_free(title_str, FALSE) ;

  window_list->match_details = match_details ;

  g_object_get(G_OBJECT(window_list->dna_list),
             "tree-view", &(window_list->tree_widget),
             NULL);

  g_object_set(G_OBJECT(window_list->dna_list),
             "selection-mode", GTK_SELECTION_SINGLE,
             "selection-func", selectionFuncCB,
             "selection-data", window_list,
             NULL);

  zMapWindowDNAListAddMatches(window_list->dna_list, dna_list);

  drawListWindow(window_list, window_list->tree_widget) ;

  g_free(window_list->title) ;

  return ;
}




/*
 *                 Internal routines
 */


static void drawListWindow(DNAWindowListData window_list, GtkWidget *tree_view)
{
  GtkWidget *window, *vbox, *label, *sub_frame, *scrolled_window ;

  /* Create window top level */
  window_list->toplevel = window = createToplevel(window_list->title);

  /* Add ptrs so parent knows about us, and we know parent */
  g_ptr_array_add(window_list->window->dnalist_windows, (gpointer)window);

  /* And a destroy function */
  g_signal_connect(GTK_OBJECT(window), "destroy",
                   GTK_SIGNAL_FUNC(destroyCB), window_list);

  /* Start drawing things in it. */
  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(window), vbox);

  gtk_box_pack_start(GTK_BOX(vbox), makeMenuBar(window_list), FALSE, FALSE, 0);


  sub_frame = gtk_frame_new(NULL);

  gtk_frame_set_shadow_type(GTK_FRAME(sub_frame), GTK_SHADOW_ETCHED_IN);

  gtk_box_pack_start(GTK_BOX(vbox), sub_frame, FALSE, FALSE, 0);

  label =  gtk_label_new(window_list->match_details) ;

  gtk_container_add(GTK_CONTAINER(sub_frame), GTK_WIDGET(label));


  sub_frame = gtk_frame_new(NULL);

  gtk_frame_set_shadow_type(GTK_FRAME(sub_frame), GTK_SHADOW_ETCHED_IN);

  gtk_box_pack_start(GTK_BOX(vbox), sub_frame, TRUE, TRUE, 0);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                         GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtk_container_add(GTK_CONTAINER(sub_frame), GTK_WIDGET(scrolled_window));

  gtk_container_add(GTK_CONTAINER(scrolled_window), window_list->tree_widget) ;


  /* Now show everything. */
  gtk_widget_show_all(window) ;

  return ;
}


static GtkWidget *createToplevel(char *title)
{
  GtkWidget *window;

  window = zMapGUIToplevelNew(NULL, title) ;

  gtk_window_set_default_size(GTK_WINDOW(window), -1, 600);
  gtk_container_border_width(GTK_CONTAINER(window), 5) ;

  return window;
}


/* make the menu from the global defined above ! */
GtkWidget *makeMenuBar(DNAWindowListData wlist)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G, (gpointer)wlist) ;

  gtk_window_add_accel_group(GTK_WINDOW(wlist->toplevel), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}



/* The list created by the calling code can be either of dna or peptide hits.
 * When the user selects a hit this function scrolls to the dna or peptide
 * in the dna and peptide columns _if_ they are visible, highlights the
 * matching sequence and scrolls to it.
 * 
 * The code is slightly complex because it may be passed either a dna or a
 * peptide match and has to highlight in either dna or peptide columns.
 */
static gboolean selectionFuncCB(GtkTreeSelection *selection,
                                GtkTreeModel     *model,
                                GtkTreePath      *path,
                                gboolean          path_currently_selected,
                                gpointer          user_data)
{
  DNAWindowListData window_list = (DNAWindowListData)user_data ;
  gint rows_selected = 0 ;
  GtkTreeIter iter ;

  if (((rows_selected = gtk_tree_selection_count_selected_rows(selection)) < 1)
      && gtk_tree_model_get_iter(model, &iter, path))
    {
      int start = 0, end = 0 ;
      ZMapFrame frame ;
      ZMapStrand strand ;
      ZMapSequenceType seq_type ;
      int start_index, end_index, seq_type_index, strand_index, frame_index ;
      ZMapGUITreeView zmap_tree_view ;

      zmap_tree_view = ZMAP_GUITREEVIEW(window_list->dna_list) ;

      /* Get the column indices */
      start_index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WINDOWDNALIST_START_COLUMN_NAME) ;
      end_index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WINDOWDNALIST_END_COLUMN_NAME) ;
      strand_index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WINDOWDNALIST_STRAND_ENUM_COLUMN_NAME) ;
      frame_index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WINDOWDNALIST_FRAME_ENUM_COLUMN_NAME) ;
      seq_type_index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WINDOWDNALIST_SEQTYPE_COLUMN_NAME) ;

      /* Get the column data */
      gtk_tree_model_get(model, &iter,
			 start_index,    &start,
			 end_index,      &end,
			 seq_type_index, &seq_type,
			 strand_index,   &strand,
			 frame_index,    &frame,
                         -1) ;

      if (!path_currently_selected)
        {
	  GtkTreeView *tree_view = NULL;
	  ZMapWindow window = window_list->window;
	  ZMapFeatureBlock block = NULL ;
	  FooCanvasItem *item ;
	  ZMapFrame tmp_frame ;
	  ZMapStrand tmp_strand ;
	  gboolean done_centring = FALSE ;

	  block = window_list->block ;
	  zMapAssert(block) ;
	
	  /* Scroll to treeview entry. */
	  tree_view = gtk_tree_selection_get_tree_view(selection) ;
	  gtk_tree_view_scroll_to_cell(tree_view, path, NULL, FALSE, 0.0, 0.0) ;

	  /* Scroll to marker and highlight sequence (if its displayed). */
	  tmp_strand = ZMAPSTRAND_NONE ;
	  tmp_frame = ZMAPFRAME_NONE ;

	  if ((item = zmapWindowFToIFindSetItem(window, window->context_to_item,
						window_list->match_feature_set,
						tmp_strand, tmp_frame)))
	    {
	      int dna_start, dna_end ;

	      dna_start = start ;
	      dna_end = end ;

	      /* Need to convert sequence coords to block for this call. */
	      zMapFeature2BlockCoords(block, &dna_start, &dna_end) ;

	      zmapWindowItemCentreOnItemSubPart(window, item, FALSE, 0.0, dna_start, dna_end) ;

	      zmapWindowHighlightSequenceRegion(window, block, seq_type, frame, start, end, FALSE) ;
	    }
        }
    }

  return TRUE ;
}


/* Destroy the list window
 *
 * Destroy the list window and its corresponding entry in the
 * array of such windows held in the ZMapWindow structure.
 */
static void destroyCB(GtkWidget *widget, gpointer user_data)
{
  DNAWindowListData window_list = (DNAWindowListData)user_data;

  if(window_list != NULL)
    {
      ZMapWindow zmap_window = NULL;

      zmap_window = window_list->window;

      if(zmap_window != NULL)
        g_ptr_array_remove(zmap_window->dnalist_windows, (gpointer)window_list->toplevel) ;

      /* Free all the dna stuff... */
      g_list_foreach(window_list->dna_match_list, freeDNAMatchCB, NULL) ;
      g_list_free(window_list->dna_match_list) ;

      zMapGUITreeViewDestroy(ZMAP_GUITREEVIEW(window_list->dna_list));
    }

  return;
}



/* Request destroy of list window, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  DNAWindowListData window_list = (DNAWindowListData)data;

  gtk_widget_destroy(GTK_WIDGET(window_list->toplevel));

  return ;
}




/*
 * Show limited help.
 */
static void helpMenuCB(gpointer data, guint cb_action, GtkWidget *widget)
{
  char *title = "DNA List Window" ;
  char *help_text =
    "The ZMap DNA List Window shows all the matches from your DNA search.\n"
    "You can click on a match and the corresponding ZMap window will scroll\n"
    "to the matchs location." ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}



/* Free allocated DNA match structs. */
static void freeDNAMatchCB(gpointer data, gpointer user_data_unused)
{
  ZMapDNAMatch match = (ZMapDNAMatch)data ;

  if (match->match)
    g_free(match->match) ;

  g_free(match) ;

  return ;
}


/* ZMapWindowDNAList extends ZMapGUITreeView */

#define ZMAP_WDL_SSTART_COLUMN_NAME "Start"
#define ZMAP_WDL_SEND_COLUMN_NAME   "End"

enum
  {
    PROP_0,
    SCREEN_START_COLUMN_INDEX,
    SCREEN_END_COLUMN_INDEX,
  };

static void zmap_windowdnalist_class_init(ZMapWindowDNAListClass zmap_tv_class);
static void zmap_windowdnalist_init      (ZMapWindowDNAList zmap_tv);
static void zmap_windowdnalist_set_property(GObject *gobject,
                                  guint param_id,
                                  const GValue *value,
                                  GParamSpec *pspec);
static void zmap_windowdnalist_get_property(GObject *gobject,
                                  guint param_id,
                                  GValue *value,
                                  GParamSpec *pspec);
static void zmap_windowdnalist_dispose (GObject *object);
static void zmap_windowdnalist_finalize(GObject *object);

static void setup_dna_tree(ZMapGUITreeView zmap_tree_view);
static void dna_get_titles_types_funcs(GList **titles_out,
                               GList **types_out,
                               GList **funcs_out,
                               GList **flags_out);

static void dna_match_match_to_value (GValue *value, gpointer user_data);
static void dna_match_start_to_value (GValue *value, gpointer user_data);
static void dna_match_end_to_value   (GValue *value, gpointer user_data);
static void dna_match_strand_to_value(GValue *value, gpointer user_data);
static void dna_match_frame_to_value (GValue *value, gpointer user_data);
static void dna_match_screen_start_to_value(GValue *value, gpointer user_data);
static void dna_match_screen_end_to_value  (GValue *value, gpointer user_data);
static void dna_match_seq_type_to_value    (GValue *value, gpointer user_data);
static void dna_match_strand_enum_to_value (GValue *value, gpointer user_data);
static void dna_match_frame_enum_to_value  (GValue *value, gpointer user_data);

static ZMapGUITreeViewClass parent_class_G = NULL;

/* Public functions */
GType zMapWindowDNAListGetType (void)
{
  static GType type = 0;

  if (type == 0)
    {
      static const GTypeInfo info =
      {
        sizeof (zmapWindowDNAListClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) zmap_windowdnalist_class_init,
        (GClassFinalizeFunc) NULL,
        NULL /* class_data */,
        sizeof (zmapWindowDNAList),
        0 /* n_preallocs */,
        (GInstanceInitFunc) zmap_windowdnalist_init,
        NULL
      };

      type = g_type_register_static (zMapGUITreeViewGetType(), "ZMapWindowDNAList", &info, (GTypeFlags)0);
    }

  return type;
}

ZMapWindowDNAList zMapWindowDNAListCreate(void)
{
  ZMapWindowDNAList dna_list = NULL;

  dna_list = (ZMapWindowDNAList)g_object_new(zMapWindowDNAListGetType(), NULL);

  return dna_list;
}

void zMapWindowDNAListAddMatch(ZMapWindowDNAList dna_list,
			       ZMapDNAMatch match)
{
  zMapGUITreeViewAddTuple(ZMAP_GUITREEVIEW(dna_list), match) ;
  return ;
}

void zMapWindowDNAListAddMatches(ZMapWindowDNAList dna_list,
				 GList *list_of_matches)
{
  zMapGUITreeViewAddTuples(ZMAP_GUITREEVIEW(dna_list), list_of_matches) ;
  return ;
}






/* Object Implementation */

static void zmap_windowdnalist_class_init(ZMapWindowDNAListClass zmap_tv_class)
{
  GObjectClass *gobject_class;
  ZMapGUITreeViewClass parent_class;

  gobject_class  = (GObjectClass *)zmap_tv_class;
  parent_class   = ZMAP_GUITREEVIEW_CLASS(zmap_tv_class);
  parent_class_G = g_type_class_peek_parent(zmap_tv_class);

  gobject_class->set_property = zmap_windowdnalist_set_property;
  gobject_class->get_property = zmap_windowdnalist_get_property;

  g_object_class_install_property(gobject_class,
                          SCREEN_START_COLUMN_INDEX,
                          g_param_spec_uint("screen-start-index", "screen-start-index",
                                        "Index for the screen start column.",
                                        0, 128, 0,
                                        ZMAP_PARAM_STATIC_RO));

  g_object_class_install_property(gobject_class,
                          SCREEN_END_COLUMN_INDEX,
                          g_param_spec_uint("screen-end-index", "screen-end-index",
                                        "Index for the screen end column.",
                                        0, 128, 0,
                                        ZMAP_PARAM_STATIC_RO));

  /* No need to override this... */
  /* parent_class->add_tuple_simple = dna_match_add_simple; */

  /* Or parent_class->add_tuples. Parent versions are ok. */

  /* add_tuple_value_list _not_ implemented! Doesn't make sense. */
  parent_class->add_tuple_value_list = NULL;

  gobject_class->dispose  = zmap_windowdnalist_dispose;
  gobject_class->finalize = zmap_windowdnalist_finalize;

  return ;
}

static void zmap_windowdnalist_init      (ZMapWindowDNAList dna_list)
{
  setup_dna_tree(ZMAP_GUITREEVIEW(dna_list));

  return ;
}

static void zmap_windowdnalist_set_property(GObject *gobject,
                                  guint param_id,
                                  const GValue *value,
                                  GParamSpec *pspec)
{
  switch(param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }
  return ;
}

static void zmap_windowdnalist_get_property(GObject *gobject,
                                  guint param_id,
                                  GValue *value,
                                  GParamSpec *pspec)
{
  ZMapGUITreeView zmap_tree_view;

  switch(param_id)
    {
    case SCREEN_START_COLUMN_INDEX:
      {
      unsigned int index;
      zmap_tree_view = ZMAP_GUITREEVIEW(gobject);
      index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WDL_SSTART_COLUMN_NAME);
      g_value_set_uint(value, index);
      }
      break;
    case SCREEN_END_COLUMN_INDEX:
      {
      unsigned int index;
      zmap_tree_view = ZMAP_GUITREEVIEW(gobject);
      index = zMapGUITreeViewGetColumnIndexByName(zmap_tree_view, ZMAP_WDL_SEND_COLUMN_NAME);
      g_value_set_uint(value, index);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_windowdnalist_dispose (GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  if(gobject_class->dispose)
    (*gobject_class->dispose)(object);

  return ;
}

static void zmap_windowdnalist_finalize(GObject *object)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(parent_class_G);

  if(gobject_class->finalize)
    (*gobject_class->finalize)(object);

  return ;
}

static void setup_dna_tree(ZMapGUITreeView zmap_tree_view)
{
  GList *column_titles = NULL;
  GList *column_types  = NULL;
  GList *column_funcs  = NULL;
  GList *column_flags  = NULL;

  dna_get_titles_types_funcs(&column_titles,
                       &column_types,
                       &column_funcs,
                       &column_flags);

  g_object_set(G_OBJECT(zmap_tree_view),
             "row-counter-column",  TRUE,
             "data-ptr-column",     TRUE,
             "column_count",        g_list_length(column_titles),
             "column_names",        column_titles,
             "column_types",        column_types,
             "column_funcs",        column_funcs,
             "column_flags_list",   column_flags,
             "sortable",            TRUE,
             NULL);


  zMapGUITreeViewPrepare(zmap_tree_view);

  if(column_titles)
    g_list_free(column_titles);
  if(column_types)
    g_list_free(column_types);
  if(column_funcs)
    g_list_free(column_funcs);
  if(column_flags)
    g_list_free(column_flags);

}

static void dna_get_titles_types_funcs(GList **titles_out,
                               GList **types_out,
                               GList **funcs_out,
                               GList **flags_out)
{
  GList *titles, *types, *funcs, *flags;
  unsigned int flags_set = (ZMAP_GUITREEVIEW_COLUMN_VISIBLE |
                      ZMAP_GUITREEVIEW_COLUMN_CLICKABLE);
  titles = types = funcs = flags = NULL;

  /* N.B. Order here dictates order of columns */

  /* Match */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_MATCH_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, dna_match_match_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(flags_set));

  /* Screen start */
  titles = g_list_append(titles, ZMAP_WDL_SSTART_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_screen_start_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(flags_set));

  /* Screen End */
  titles = g_list_append(titles, ZMAP_WDL_SEND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_screen_end_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(flags_set));

  /* Strand */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_STRAND_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, dna_match_strand_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(flags_set));

  /* Frame */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_FRAME_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_STRING));
  funcs  = g_list_append(funcs, dna_match_frame_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(flags_set));

  /* Not visible... real start and end according to zmap */
  /* -start- */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_START_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_start_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* -end- */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_END_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_end_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* Match Sequence Type */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_SEQTYPE_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_seq_type_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* Match Strand as an int/enum */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_STRAND_ENUM_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_strand_enum_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));

  /* Match Frame as an int/enum */
  titles = g_list_append(titles, ZMAP_WINDOWDNALIST_FRAME_ENUM_COLUMN_NAME);
  types  = g_list_append(types, GINT_TO_POINTER(G_TYPE_INT));
  funcs  = g_list_append(funcs, dna_match_frame_enum_to_value);
  flags  = g_list_append(flags, GUINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING));


  if(titles_out)
    *titles_out = titles;
  if(types_out)
    *types_out  = types;
  if(funcs_out)
    *funcs_out  = funcs;
  if(flags_out)
    *flags_out  = flags;

  return ;
}

static void dna_match_match_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_string(value, match->match);

  return ;
}

static void dna_match_start_to_value(GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->ref_start);

  return ;
}

static void dna_match_end_to_value(GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->ref_end);

  return ;
}

static void dna_match_strand_to_value(GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_string(value, zMapFeatureStrand2Str(match->strand));

  return ;
}

static void dna_match_frame_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_string(value, zMapFeatureFrame2Str(match->frame));

  return ;
}

static void dna_match_screen_start_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->screen_start);

  return ;
}

static void dna_match_screen_end_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->screen_end);

  return ;
}

static void dna_match_seq_type_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->match_type);

  return ;
}

static void dna_match_strand_enum_to_value (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->strand);

  return ;
}

static void dna_match_frame_enum_to_value  (GValue *value, gpointer user_data)
{
  ZMapDNAMatch match = (ZMapDNAMatch)user_data;

  g_value_set_int(value, match->frame);

  return ;
}




/*************************** end of file *********************************/
