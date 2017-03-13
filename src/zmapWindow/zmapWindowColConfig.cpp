/*  File: zmapWindowColConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Functions to implement column configuration.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <string>
#include <list>
#include <map>
#include <algorithm>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <zmapWindow_P.hpp>
#include <zmapWindowContainers.hpp>
#include <gbtools/gbtools.hpp>

using namespace std ;


/* Labels for column state, used in code and the help page. */
#define SHOW_LABEL     "Show"
#define SHOWHIDE_LABEL "Auto"
#define HIDE_LABEL     "Hide"
#define BUMPED_TEXT    "bumped"
#define UNBUMPED_TEXT  "unbumped"


#define XPAD 4
#define YPAD 4

#define NOTEBOOK_PAGE_DATA "notebook_page_data"

typedef struct _ColConfigureStruct *ColConfigure;

#define CONFIGURE_DATA "col_data"

typedef enum
  {
    NAME_COLUMN,       /* shows the column (i.e. track) name */
    SHOW_FWD_COLUMN,   /* toggle button to show this track on the forward strand */
    AUTO_FWD_COLUMN,   /* toggle button to set visibility to auto on the forward strand */
    HIDE_FWD_COLUMN,   /* toggle button to hide this track on the forward strand */
    SHOW_REV_COLUMN,   /* toggle button to show this track on the reversestrand */
    AUTO_REV_COLUMN,   /* toggle button to set visibility to auto on the reverse strand */
    HIDE_REV_COLUMN,   /* toggle button to hide this track on the reverse strand */
    BUMPED_COLUMN,
    GROUP_COLUMN,
    STYLE_COLUMN,
    SOURCE_COLUMN,

    N_COLUMNS, /* must appear after main list of columns */
    NO_COLUMN
  } DialogColumns ;




typedef struct _ColConfigureStruct
{
  ZMapWindow window ;

  ZMapWindowColConfigureMode mode;

  ZMapWindowColConfigureMode curr_mode ;

  GtkWidget *notebook;
  GtkWidget *loaded_page;
  GtkWidget *deferred_page;

  GtkWidget *vbox;

  /* If true, flag indicates that changes will only be applied when the Apply button is pressed;
   * if false, there will be no Apply button and changes will take effect immediately. */
  unsigned int has_apply_button : 1 ;

} ColConfigureStruct;


typedef struct _NotebookPageStruct *NotebookPage ;

typedef enum
  {
    NOTEBOOK_INVALID_FUNC,
    NOTEBOOK_POPULATE_FUNC,
    NOTEBOOK_REORDER_FUNC,
    NOTEBOOK_APPLY_FUNC,
    NOTEBOOK_UPDATE_FUNC,
    NOTEBOOK_CLEAR_FUNC,
  } NotebookFuncType ;


/* For updating button state. */
typedef struct
{
  ZMapWindowColConfigureMode mode ;
  FooCanvasGroup *column_group ;
  GQuark column_id;
  gboolean button_found ;
} ChangeButtonStateDataStruct, *ChangeButtonStateData ;





typedef void (*NotebookPageConstruct)(NotebookPage notebook_page, GtkWidget *page);

typedef void (*NotebookPageColFunc)(NotebookPage notebook_page, FooCanvasGroup *column_group);

typedef void (*NotebookPageUpdateFunc)(NotebookPage notebook_page, ChangeButtonStateData user_data) ;

typedef void (*NotebookPageFunc)(NotebookPage notebook_page);

typedef struct _NotebookPageStruct
{
  ColConfigure          configure_data;

  GtkWidget            *page_container;
  char                 *page_name;

  NotebookPageConstruct  page_constructor; /* A+ */
  NotebookPageColFunc    page_populate;    /* B+ */
  NotebookPageFunc       page_apply;  /* C  */
  NotebookPageFunc       page_reorder;  /* C  */
  NotebookPageUpdateFunc page_update;      /* D */
  NotebookPageFunc       page_clear;  /* B- */
  NotebookPageFunc       page_destroy;  /* A- */

  gpointer              page_data;
} NotebookPageStruct;


typedef struct _LoadedPageDataStruct *LoadedPageData;

typedef void (*LoadedPageResetFunc)(LoadedPageData page_data);


typedef struct _LoadedPageDataStruct
{
  ZMapWindow window ;
  GtkWidget *cols_panel ;    // the container widget for the columns panel
  GtkWidget *page_container ; // the container for cols_panel
  GtkTreeView *tree_view ;
  GtkTreeModel *tree_model ;
  GtkTreeModel *tree_filtered ;
  GtkEntry *search_entry ;
  GtkEntry *filter_entry ;
  GtkLabel *selection_feedback ;
  GtkComboBox *search_field_combo ;

  ColConfigure configure_data ;

  /* Remember data we've created so we can free it */
  GList *cb_data_list ;

  unsigned int apply_now        : 1 ;
  unsigned int reposition       : 1 ;

  gboolean clicked_button ; /* set to true when the user clicked a radio button; false if they
                             * clicked another column */

  GQuark last_style_id ;   /* the last style id that was chosen using the Choose Style option */
  GQuark last_group_id ;

  DialogColumns search_field ; /* which field to apply the search/filter to */
} LoadedPageDataStruct;


typedef struct
{
  LoadedPageData loaded_page_data ;
  DialogColumns tree_col_id ;
  ZMapStrand strand ;
} ShowHideDataStruct, *ShowHideData;



typedef struct _DeferredPageDataStruct *DeferredPageData;

typedef void (*DeferredPageResetFunc)(DeferredPageData page_data);

typedef struct _DeferredPageDataStruct
{
  ZMapFeatureBlock block;

  GList *load_in_mark;
  GList *load_all;
  GList *load_none;

} DeferredPageDataStruct;


typedef struct
{
  DeferredPageData deferred_page_data;
  FooCanvasGroup  *column_group;
  GQuark           column_quark;
  GtkWidget       *column_button;
} DeferredButtonStruct, *DeferredButton;


/* Utility struct to pass data to a callback to set the state for a particular row in the tree
 * model */
typedef struct _SetTreeRowStateDataStruct
{
  LoadedPageData loaded_page_data ;
  ZMapStrand strand ;
  ZMapStyleColumnDisplayState state ;
} SetTreeRowStateDataStruct, *SetTreeRowStateData ;


/* Arrgh, pain this is needed.  Also, will fall over when multiple alignments... */
typedef struct
{
  GList *forward, *reverse;
  gboolean loaded_or_deferred;/* loaded = FALSE, deferred = TRUE */
  ZMapWindowContainerBlock block_group;
  int mark1, mark2;
}ForwardReverseColumnListsStruct, *ForwardReverseColumnLists;


/* required for the sizing of the widget... */
typedef struct
{
  unsigned int debug   : 1 ;

  GtkWidget *toplevel;

  GtkRequisition widget_requisition;
  GtkRequisition user_requisition;
} SizingDataStruct, *SizingData;


typedef struct GetFeaturesetsDataStructType
{
  LoadedPageData page_data ;
  GList *feature_sets ;
} GetFeaturesetsDataStruct, *GetFeaturesetsData ;


/* Utility for looking up an id in a hash of GQuark to list of GQuark */
typedef struct HashListFindIDStructType
{
  GQuark result ;
  GQuark search_id ;
} HashListFindIDStruct, *HashListFindID ;


static GtkWidget *make_menu_bar(ColConfigure configure_data);
static void loaded_cols_panel(LoadedPageData loaded_page_data, FooCanvasGroup *column_group) ;
static char *label_text_from_column(FooCanvasGroup *column_group);
static GtkWidget *create_label(GtkWidget *parent, const char *text);
static GtkWidget *create_revert_apply_button(ColConfigure configure_data);

static GtkWidget *make_scrollable_vbox(GtkWidget *vbox);
static ColConfigure configure_create(ZMapWindow window, ZMapWindowColConfigureMode configure_mode);

static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;
static void saveCB(gpointer cb_data, guint callback_action, GtkWidget *widget) ;
static void saveColumnsConfig(ColConfigure configure_data, const char *filename) ;

static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void helpDisplayCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void loaded_column_visibility_toggled_cb(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data) ;

static GtkWidget *configure_make_toplevel(ColConfigure configure_data);
static void configure_add_pages(ColConfigure configure_data);
static void configure_simple(ColConfigure configure_data,
                             FooCanvasGroup *column_group, ZMapWindowColConfigureMode curr_mode) ;
static void configure_populate_containers(ColConfigure    configure_data,
                                          FooCanvasGroup *column_group);
static void configure_clear_containers(ColConfigure configure_data);

static void cancel_button_cb(GtkWidget *apply_button, gpointer user_data) ;
static void revert_button_cb(GtkWidget *apply_button, gpointer user_data);
static void apply_button_cb(GtkWidget *apply_button, gpointer user_data);
static void ok_button_cb(GtkWidget *apply_button, gpointer user_data);
static void reorder_button_cb(GtkWidget *apply_button, gpointer user_data) ;


static void notebook_foreach_page(GtkWidget *notebook_widget, gboolean current_page_only,
                                  NotebookFuncType func_type, gpointer foreach_data);

//static void apply_state_cb(GtkWidget *widget, gpointer user_data);

static FooCanvasGroup *configure_get_point_block_container(ColConfigure configure_data,
                                                           FooCanvasGroup *column_group);

static void set_tree_store_value_from_state(ZMapStyleColumnDisplayState col_state,
                                            GtkListStore *store,
                                            GtkTreeIter *iter,
                                            const gboolean forward) ;

static gboolean zmapAddSizingSignalHandlers(GtkWidget *widget, gboolean debug,
                                            int width, int height);

static void loaded_page_apply_visibility_tree_row(LoadedPageData loaded_page_data, ZMapStrand strand, GtkTreeModel *model, GtkTreeIter *iter) ;

static GList* tree_model_get_column_featuresets(LoadedPageData page_data, GtkTreeModel *model, GtkTreeIter *iter) ;
static GQuark tree_model_get_column_id(GtkTreeModel *model, GtkTreeIter *iter, const gboolean unique) ;
static char* tree_model_get_style_ids(GtkTreeModel *model, GtkTreeIter *iter) ;
static GQuark tree_model_get_group_id(GtkTreeModel *model, GtkTreeIter *iter) ;
static bool tree_model_get_bumped(GtkTreeModel *model, GtkTreeIter *iter) ;

static GQuark hashListFindID(GHashTable *column_groups, const GQuark search_id) ;

static ZMapStyleColumnDisplayState get_state_from_tree_store_value(GtkTreeModel *model, 
                                                                   GtkTreeIter *iter, 
                                                                   ZMapStrand strand) ;

static GList* tree_view_get_selected_featuresets(LoadedPageData page_data) ;

static GList* getFeaturesetStructsFromIDs(GList *featureset_ids, ZMapFeatureContext context) ;

static DialogColumns comboGetValue(GtkComboBox *combo) ;



/*
 *          Globals
 */

static GtkItemFactoryEntry menu_items_G[] =
  {
    { (gchar *)"/_File",           NULL,          NULL,             0, (gchar *)"<Branch>",         NULL},
    { (gchar *)"/File/Close",      (gchar *)"<control>W", (GtkItemFactoryCallback)requestDestroyCB, 0, NULL,            NULL},
    { (gchar *)"/File/Save",       (gchar *)"<control>S", (GtkItemFactoryCallback)saveCB, 0, NULL, NULL},
    { (gchar *)"/_Help",           NULL,          NULL,             0, (gchar *)"<LastBranch>", NULL},
    { (gchar *)"/Help/General",    NULL,          (GtkItemFactoryCallback)helpCB,           0, NULL, NULL},
    { (gchar *)"/Help/Display",    NULL,          (GtkItemFactoryCallback)helpDisplayCB,           0, NULL, NULL}
  };


/* Map of search field enum to a descriptive string for use in the drop-down box. */
static map<DialogColumns, string> searchfields_G = {
  {NO_COLUMN, "All"},    // if no column is specified, we'll search/filter all relevant fields
  {NAME_COLUMN, "Name"},
  {GROUP_COLUMN, "Group"},
  {STYLE_COLUMN, "Style"},
  {SOURCE_COLUMN, "Source"}
  } ;


/*
 *          External Public functions.
 */


/* Shows a list of displayable columns for forward/reverse strands with radio buttons
 * for turning them on/off.
 */
void zMapWindowColumnList(ZMapWindow window)
{
  zmapWindowColumnConfigure(window, NULL, ZMAPWINDOWCOLUMN_CONFIGURE_ALL) ;

  return ;
}




/*
 *          External Package functions.
 */


/* Note that column_group is not looked at for configuring all columns. */
void zmapWindowColumnConfigure(ZMapWindow                 window,
                               FooCanvasGroup            *column_group,
                               ZMapWindowColConfigureMode configure_mode)
{
  ColConfigure configure_data = NULL;
  int page_no = 0;

  if (!window)
    return ;

  if(window->col_config_window)
    {
      configure_data = (ColConfigure)g_object_get_data(G_OBJECT(window->col_config_window),
                                                       CONFIGURE_DATA);
    }

  switch (configure_mode)
    {
    case ZMAPWINDOWCOLUMN_HIDE:
    case ZMAPWINDOWCOLUMN_SHOW:
      {
        ColConfigureStruct configure_struct = {NULL};

        if(!configure_data)
          {
            configure_data         = &configure_struct;
            configure_data->window = window;
            configure_data->mode   = configure_mode;
          }

        configure_data->curr_mode = configure_mode ;

        configure_simple(configure_data, column_group, configure_mode) ;
      }
      break ;
    case ZMAPWINDOWCOLUMN_CONFIGURE:
    case ZMAPWINDOWCOLUMN_CONFIGURE_ALL:
      {
        gboolean created = FALSE;

        if(configure_data == NULL)
          {
            configure_data = configure_create(window, configure_mode);
            created = TRUE;
          }
        else
          {
            configure_clear_containers(configure_data);
            page_no = gtk_notebook_get_current_page(GTK_NOTEBOOK(configure_data->notebook));
          }

        configure_populate_containers(configure_data, column_group);

        if(created && configure_data->has_apply_button)
          {
            GtkWidget *apply_button;

            apply_button = create_revert_apply_button(configure_data);

            gtk_window_set_default(GTK_WINDOW(window->col_config_window), apply_button) ;
          }

        break ;
      }
    default:
      break ;
    }

  if(window->col_config_window)
    {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(configure_data->notebook), page_no);

      gtk_widget_show_all(window->col_config_window);

      if (GTK_IS_WINDOW(window->col_config_window))
        gtk_window_present(GTK_WINDOW(window->col_config_window)) ;
    }

  return ;
}


/* Destroy the column configuration window if there is one. */
void zmapWindowColumnConfigureDestroy(ZMapWindow window)
{
  if (window->col_config_window)
    {
      gtk_widget_destroy(window->col_config_window) ;
      window->col_config_window =  NULL ;
    }

  return ;
}






/*
 *                 Internal functions
 */


static void configure_simple(ColConfigure configure_data,
                             FooCanvasGroup *column_group, ZMapWindowColConfigureMode curr_mode)
{
  /* If there's a config window displayed then toggle correct button in window to effect
   * change, otherwise operate directly on column. */
  if (configure_data->window->col_config_window && configure_data->notebook)
    {
      ChangeButtonStateDataStruct cb_data ;

      cb_data.mode = curr_mode ;
      cb_data.column_group = column_group ;
      cb_data.button_found = FALSE ;

      notebook_foreach_page(configure_data->notebook, FALSE,
                            NOTEBOOK_UPDATE_FUNC, &cb_data) ;
    }
  else
    {
      ZMapStyleColumnDisplayState col_state ;

      if (configure_data->mode == ZMAPWINDOWCOLUMN_HIDE)
        col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      else
        col_state = ZMAPSTYLE_COLDISPLAY_SHOW ;

      /* TRUE makes the function do a zmapWindowFullReposition() if needed */
      zmapWindowColumnSetState(configure_data->window, column_group, col_state, TRUE) ;
    }

  return ;
}

/* create the widget and all the containers where we draw the buttons etc... */
static ColConfigure configure_create(ZMapWindow window, ZMapWindowColConfigureMode configure_mode)
{
  GtkWidget *toplevel, *vbox, *menubar;
  ColConfigure configure_data ;

  configure_data = g_new0(ColConfigureStruct, 1) ;

  configure_data->window = window ;
  configure_data->mode   = configure_mode;

  /* set up the top level window */
  configure_data->window->col_config_window =
    toplevel = configure_make_toplevel(configure_data);

  /* vbox for everything below the toplevel */
  configure_data->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  /* menu... */
  menubar = make_menu_bar(configure_data) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

  /* make notebook and pack it. */
  configure_data->notebook = gtk_notebook_new();
  gtk_box_pack_start(GTK_BOX(vbox), configure_data->notebook, TRUE, TRUE, 0) ;

  /* add loaded and deferred pages */
  configure_add_pages(configure_data);

  return configure_data;
}

static GtkWidget *configure_make_toplevel(ColConfigure configure_data)
{
  GtkWidget *toplevel = NULL;

  if (!configure_data->window->col_config_window)
    {
      char *seq_name ;

      /* Get sequence name for the title */
      seq_name = (char *)g_quark_to_string(configure_data->window->feature_context->sequence_name);

      toplevel = zMapGUIToplevelNew("Column configuration", seq_name) ;

      /* Calculate a sensible initial size - say 3/4 of screen height */
      int width = 0, height = 0 ;
      if (gbtools::GUIGetTrueMonitorSize(toplevel, &width, &height))
        {
          gtk_window_set_default_size(GTK_WINDOW(toplevel), width * 0.5, height * 0.75) ;
        }

      /* Add destroy func - destroyCB */
      g_signal_connect(GTK_OBJECT(toplevel), "destroy",
                       GTK_SIGNAL_FUNC(destroyCB), (gpointer)configure_data) ;

      /* Save a record of configure_data, so we only save the widget */
      g_object_set_data(G_OBJECT(toplevel), CONFIGURE_DATA, configure_data) ;

      gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
    }

  return toplevel;
}


static void loaded_page_construct(NotebookPage notebook_page, GtkWidget *page)
{
  notebook_page->page_container = page;

  return ;
}

static void loaded_page_populate (NotebookPage notebook_page, FooCanvasGroup *column_group)
{
  LoadedPageData loaded_page_data;
  ColConfigure configure_data;

  configure_data = notebook_page->configure_data;

  loaded_page_data = (LoadedPageData)(notebook_page->page_data);
  loaded_page_data->window = configure_data->window ; 
  loaded_page_data->page_container = notebook_page->page_container ;
  loaded_page_data->configure_data = configure_data ;
  loaded_page_data->search_field = NO_COLUMN ;

  loaded_cols_panel(loaded_page_data, column_group) ;

  if (!loaded_page_data->configure_data->has_apply_button)
    {
      loaded_page_data->apply_now  = TRUE;
      loaded_page_data->reposition = TRUE;
    }

  return ;
}


/* Update all of the columns based on the values in the tree store */
static void loaded_page_apply_visibility(LoadedPageData loaded_page_data)
{
  GtkTreeIter iter ;
  GtkTreeModel *model = loaded_page_data->tree_model ;

  if (gtk_tree_model_get_iter_first(model, &iter))
    {
      do
        {
          loaded_page_apply_visibility_tree_row(loaded_page_data, ZMAPSTRAND_FORWARD, model, &iter) ;
          loaded_page_apply_visibility_tree_row(loaded_page_data, ZMAPSTRAND_REVERSE, model, &iter) ;
        } while (gtk_tree_model_iter_next(model, &iter)) ;
    }
}


static gboolean setColumnOrder(const GQuark column_id, 
                               std::map<GQuark, ZMapFeatureColumn> &columns, 
                               int *i)
{
  gboolean columns_changed = FALSE ;
  ZMapFeatureColumn column = NULL ;
          
  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = columns.find(column_id) ;

  if (col_iter != columns.end())
    column = col_iter->second ;
                  
  if (column && column->order != *i)
    {
      column->order = *i ;
      *i += 1 ;

      columns_changed = TRUE ;
    }

  return columns_changed ;
}

/* Reorder the columns based on their order in the tree */
static void loaded_page_apply_reorder_columns(LoadedPageData page_data)
{
  zMapReturnIfFail(page_data && 
                   page_data->configure_data &&
                   page_data->tree_model &&
                   page_data->window &&
                   page_data->window->context_map &&
                   page_data->window->context_map->columns) ;

  /* We must only reorder columns if the tree contains all of them! */
  if (page_data->apply_now &&
      page_data->configure_data->mode == ZMAPWINDOWCOLUMN_CONFIGURE_ALL)
    {
      gboolean columns_changed = FALSE ;

      /* Loop through all rows in the tree and set the column order index */
      GtkTreeModel *model = page_data->tree_model ;
      std::map<GQuark, ZMapFeatureColumn> &columns = *page_data->window->context_map->columns ;
      GtkTreeIter iter;
      int i = 0 ;

      /* Add default columns first */
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_STRAND_SEPARATOR), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_HAND_BUILT_NAME), columns, &i) ;
      columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_ORF_NAME), columns, &i) ;
      //columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME), columns, &i) ;
      //columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_GFF_NAME), columns, &i) ;
      //columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_SCALE_NAME), columns, &i) ;
      //columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME), columns, &i) ;
      //columns_changed |= setColumnOrder(zMapStyleCreateID(ZMAP_FIXED_STYLE_ASSEMBLY_PATH_NAME), columns, &i) ;

      if (gtk_tree_model_get_iter_first(model, &iter))
        {
          do
            {
              GQuark column_id = tree_model_get_column_id(model, &iter, TRUE) ;
              columns_changed |= setColumnOrder(column_id, columns, &i) ;
            } while (gtk_tree_model_iter_next(model, &iter)) ;
        }

      /* Need to set the feature_set_names list to the correct order. Not sure if we should include
       * all featuresets or just reorder the list that is there. For now, include all. */
      if (page_data->window->feature_set_names)
        g_list_free(page_data->window->feature_set_names) ;

      page_data->window->feature_set_names = page_data->window->context_map->getOrderedColumnsGListIDs() ;

      /* Redraw */
      zmapWindowBusy(page_data->window, FALSE) ;
      zmapWindowColOrderColumns(page_data->window);

      if(page_data->reposition)
        zmapWindowFullReposition(page_data->window->feature_root_group, TRUE, "loaded_page_apply_reorder_columns") ;

      zmapWindowBusy(page_data->window, FALSE) ;
     
      zMapWindowSetFlag(page_data->configure_data->window, ZMAPFLAG_SAVE_COLUMNS, TRUE) ;
    }
}


/* Get the list of styles as a ;-separated string for the given column */
static char* columnGetStylesList(LoadedPageData page_data, ZMapFeatureContextMap context_map, const GQuark column_id)
{
  char *style_names = NULL ;
  zMapReturnValIfFail(context_map && page_data && page_data->window && context_map->columns, style_names) ;

  //  // finding all styles for column doesn't seem to work ;for now look in the featuresets
//  FooCanvasGroup *column_group = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_FORWARD, column_id) ;
//
//  if (!column_group)
//    column_group = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_REVERSE, column_id) ;
//
//  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)(column_group);
//
//  /* gb10: we should use all featuresets in the column but it causes a crash so leaving it at
//   * just this one featureset for now */
//  if (container)
//    {
//      ZMapFeatureSet feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container) ;
//
//      if (feature_set && feature_set->style)
//        style_names = g_strdup(g_quark_to_string(feature_set->style->original_id)) ;
//    }
  
  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;
  ZMapFeatureColumn column = NULL ;

  if (col_iter != context_map->columns->end())
    column = col_iter->second ;

  if (column)
    {
      GList *styles_list = (GList*)g_hash_table_lookup(context_map->column_2_styles, GINT_TO_POINTER(column_id)) ;
      style_names = zMap_g_list_quark_to_string(styles_list, ";") ;
    }

  return style_names ;
}


/* Get the list of sources as a ;-separated string for the given column */
static string columnGetSourcesList(LoadedPageData page_data, ZMapFeatureContextMap context_map, const GQuark column_id)
{
  string style_names ;
  zMapReturnValIfFail(context_map && 
                      page_data && 
                      page_data->window && 
                      page_data->window->feature_context &&
                      context_map->columns, 
                      style_names) ;

  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = context_map->columns->find(column_id) ;
  ZMapFeatureColumn column = NULL ;

  if (col_iter != context_map->columns->end())
    column = col_iter->second ;

  if (column)
    {
      ZMapFeatureContext context = page_data->window->feature_context ;

      for (GList *item = column->featuresets_unique_ids; item; item = item->next)
        {
          GQuark fset_id = (GQuark)GPOINTER_TO_INT(item->data) ;
          ZMapFeatureSet feature_set = zmapFeatureContextGetFeaturesetFromId(context, fset_id) ;

          if (feature_set && feature_set->source && !feature_set->source->toplevelName().empty())
            {
              if (!style_names.empty())
                style_names += "; " ;

              style_names += feature_set->source->toplevelName() ;
            }
        }
    }

  return style_names ;
}


/* Assign new styles to columns based on the values in the tree rows. */
static void loaded_page_apply_styles(LoadedPageData page_data)
{
  zMapReturnIfFail(page_data && 
                   page_data->tree_model &&
                   page_data->window &&
                   page_data->window->context_map &&
                   page_data->window->context_map->columns) ;

  if (page_data->apply_now)
    {
      GtkTreeModel *model = page_data->tree_model ;

      /* Loop through all rows in the tree and set the style for each column */
      GtkTreeIter iter;
  
      if (gtk_tree_model_get_iter_first(model, &iter))
        {
          do
            {
              GQuark column_id = tree_model_get_column_id(model, &iter, TRUE) ;
              char *new_style_names = tree_model_get_style_ids(model, &iter) ;
              char *old_style_names = columnGetStylesList(page_data, page_data->window->context_map, column_id) ;


              /* Only apply changes if the style has changed. The new style should be a single
               * style that we will assign to all featuresets in the column. It should therefore 
               * not contain a separator character. */
              if (new_style_names != old_style_names &&
                  (!new_style_names || !old_style_names || strcmp(new_style_names, old_style_names) != 0))
                {
                  if (new_style_names && strchr(new_style_names, ';'))
                    {
                      zMapLogWarning("Could not set new style for column '%s': expected a single style but multiple styles given: %s",
                                     g_quark_to_string(column_id),
                                     new_style_names) ;
                    }
                  else
                    {
                      /* Remove the old style(s) */
                      if (old_style_names)
                        {
                          GList *old_style_ids = zMapConfigString2QuarkIDGList(old_style_names) ;
                          
                          for (GList *item = old_style_ids; item; item = g_list_next(item))
                            {
                              GQuark old_style_id = GPOINTER_TO_INT(item->data) ;

                              zmapWindowColumnRemoveStyle(old_style_id, 
                                                          column_id, 
                                                          page_data->window->context_map, 
                                                          page_data->window) ;
                            }
                        }

                      /* Add the new style */
                      GQuark new_style_id = zMapStyleCreateID(new_style_names) ;

                      zmapWindowColumnAddStyle(new_style_id, 
                                               column_id, 
                                               page_data->window->context_map, 
                                               page_data->window) ;

                      /* Set the style in all of the featuresets */
                      GList *featuresets = tree_model_get_column_featuresets(page_data, model, &iter) ;
                      
                      for (GList *item = featuresets; item; item = item->next)
                        {
                          ZMapFeatureSet feature_set = (ZMapFeatureSet)(item->data) ;
                          
                          if (feature_set && feature_set->style && feature_set->style->unique_id != new_style_id)
                            {
                              zmapWindowFeaturesetSetStyle(new_style_id,
                                                           feature_set,
                                                           page_data->window->context_map,
                                                           page_data->window,
                                                           FALSE, FALSE, FALSE);
                            }
                        }
                    }
                }
            } while (gtk_tree_model_iter_next(model, &iter)) ;
        }
    }
}


/* For a hash table of IDs to GLists of IDs, remove the given value from the GList for the given
 * key. If this would leave the list empty then also remove the list from the hash table. */
static void hashListRemoveID(GHashTable *ghash, const GQuark key, const GQuark value)
{
  /* Get the list of values */
  GList *values = (GList*)(g_hash_table_lookup(ghash, GINT_TO_POINTER(key))) ;

  /* Remove the given value from the list of values */
  values = g_list_remove(values, GINT_TO_POINTER(value)) ;

  if (values && g_list_length(values) > 0)
    {
      /* There are still values in the list, so update the list in the hash table */
      g_hash_table_replace(ghash, GINT_TO_POINTER(key), values) ;
    }
  else
    {
      /* The list is now empty so remove it from the hash table */
      g_hash_table_remove(ghash, GINT_TO_POINTER(key)) ;

      if (values)
        g_list_free(values) ;
    }
}


/* For a hash table of IDs to GLists of IDs, insert the given value into the GList for the given
 * key. If the list does not exist then create it. */
static void hashListInsertID(GHashTable *ghash, const GQuark key, const GQuark value)
{
  GList *list = (GList*)(g_hash_table_lookup(ghash, GINT_TO_POINTER(key))) ;

  /* If the list is null then we create a new one.*/
  gboolean new_list = (list == NULL) ;
  list = g_list_append(list, GINT_TO_POINTER(value)) ;

  if (new_list)
    g_hash_table_insert(ghash, GINT_TO_POINTER(key), list) ;
  else
    g_hash_table_replace(ghash, GINT_TO_POINTER(key), list) ;
}


/* Assign groups based on their values in the tree */
static void loaded_page_apply_groups(LoadedPageData page_data)
{
  zMapReturnIfFail(page_data && 
                   page_data->tree_model &&
                   page_data->window &&
                   page_data->window->context_map &&
                   page_data->window->context_map->column_groups) ;

  if (page_data->apply_now)
    {
      GtkTreeModel *model = page_data->tree_model ;
      GHashTable *column_groups = page_data->window->context_map->column_groups ;

      /* Loop through all rows in the tree */
      GtkTreeIter iter;
  
      if (gtk_tree_model_get_iter_first(model, &iter))
        {
          do
            {
              GQuark column_id = tree_model_get_column_id(model, &iter, FALSE) ;
              GQuark new_group_id = tree_model_get_group_id(model, &iter) ;
              GQuark old_group_id = hashListFindID(column_groups, column_id) ;

              /* Check if the group has changed */
              if (new_group_id != old_group_id)
                {
                  if (old_group_id)
                    hashListRemoveID(column_groups, old_group_id, column_id) ;

                  if (new_group_id)
                    hashListInsertID(column_groups, new_group_id, column_id) ;
                  
                  zMapWindowSetFlag(page_data->window, ZMAPFLAG_SAVE_COLUMN_GROUPS, TRUE) ;
                }
            } while (gtk_tree_model_iter_next(model, &iter)) ;
        }
    }
}


/* Assign bump status based on the value in the tree */
static void loaded_page_apply_bumped(LoadedPageData page_data)
{
  zMapReturnIfFail(page_data && 
                   page_data->tree_model &&
                   page_data->window &&
                   page_data->window->context_map &&
                   page_data->window->context_map->column_groups) ;

  if (page_data->apply_now)
    {
      GtkTreeModel *model = page_data->tree_model ;

      /* Loop through all rows in the tree */
      GtkTreeIter iter;
  
      if (gtk_tree_model_get_iter_first(model, &iter))
        {
          do
            {
              // Get the column id for this row
              GQuark column_id = tree_model_get_column_id(model, &iter, TRUE) ;

              // Get the canvas items for the columns (forward and rev strand)
              FooCanvasGroup *column_group_fwd = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_FORWARD, column_id) ;
              FooCanvasGroup *column_group_rev = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_REVERSE, column_id) ;

              ZMapWindowContainerFeatureSet container_fwd = (ZMapWindowContainerFeatureSet)column_group_fwd ;
              ZMapWindowContainerFeatureSet container_rev = (ZMapWindowContainerFeatureSet)column_group_rev ;

              // Get the compress mode depending on whether the mark is set
              ZMapWindowCompressMode compress_mode ;

              if (zmapWindowMarkIsSet(page_data->window->mark))
                compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
              else
                compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

              // Get the required bump status from the tree row. Note that this is just bumped or
              // unbumped - if bumped, we use the default bump mode for the column.
              bool bumped = tree_model_get_bumped(model, &iter) ;
              
              ZMapStyleBumpMode bump_mode_fwd = ZMAPBUMP_UNBUMP ;
              ZMapStyleBumpMode bump_mode_rev = ZMAPBUMP_UNBUMP ;

              if (bumped)
                {
                  if (container_fwd)
                    bump_mode_fwd = zmapWindowContainerFeatureSetGetDefaultBumpMode(container_fwd);

                  if (container_rev)
                    bump_mode_rev = zmapWindowContainerFeatureSetGetDefaultBumpMode(container_rev);
                }

              // Do the bump
              if (container_fwd)
                zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(column_group_fwd), bump_mode_fwd, compress_mode) ;

              if (container_rev)
                zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(column_group_rev), bump_mode_rev, compress_mode) ;

              // Redraw
              zmapWindowFullReposition(page_data->window->feature_root_group,TRUE, "apply bump mode to columns") ;
            } while (gtk_tree_model_iter_next(model, &iter)) ;
        }
    }
}


/* Apply column order changes on the loaded-columns page */
static void loaded_page_reorder(NotebookPage notebook_page)
{
  LoadedPageData loaded_page_data;
  gboolean save_apply_now, save_reposition;

  loaded_page_data = (LoadedPageData)(notebook_page->page_data);

  /* Reset the flags so that we apply changes now but don't reposition yet. Save the original
   * flag values first */
  save_apply_now  = loaded_page_data->apply_now;
  save_reposition = loaded_page_data->reposition;

  loaded_page_data->apply_now  = TRUE;
  loaded_page_data->reposition = TRUE;


  /* Do the reorder */
  loaded_page_apply_reorder_columns(loaded_page_data) ;


  zmapWindowBusy(loaded_page_data->window, FALSE) ;
  zmapWindowColOrderColumns(loaded_page_data->window);
  zmapWindowFullReposition(loaded_page_data->window->feature_root_group, TRUE, "loaded_page_apply") ;
  zmapWindowBusy(loaded_page_data->window, FALSE) ;

  /* Restore the original flag values */
  loaded_page_data->apply_now  = save_apply_now;
  loaded_page_data->reposition = save_reposition;

  /* Maintain the current order but clear any sorting */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(loaded_page_data->tree_model), 
                                       GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                       GTK_SORT_ASCENDING) ;

}


/* Apply changes on the loaded-columns page */
static void loaded_page_apply(NotebookPage notebook_page)
{
  LoadedPageData loaded_page_data;
  gboolean save_apply_now, save_reposition;

  loaded_page_data = (LoadedPageData)(notebook_page->page_data);

  /* Reset the flags so that we apply changes now but don't reposition yet. Save the original
   * flag values first */
  save_apply_now  = loaded_page_data->apply_now;
  save_reposition = loaded_page_data->reposition;

  loaded_page_data->apply_now  = TRUE;
  loaded_page_data->reposition = FALSE;

  /* Apply the new visibility states, styles and groups */
  loaded_page_apply_visibility(loaded_page_data) ;
  loaded_page_apply_styles(loaded_page_data) ;
  loaded_page_apply_groups(loaded_page_data) ;
  loaded_page_apply_bumped(loaded_page_data) ;

  zmapWindowBusy(loaded_page_data->window, FALSE) ;
  zmapWindowColOrderColumns(loaded_page_data->window);
  zmapWindowFullReposition(loaded_page_data->window->feature_root_group, TRUE, "loaded_page_apply") ;
  zmapWindowBusy(loaded_page_data->window, FALSE) ;

  /* Restore the original flag values */
  loaded_page_data->apply_now  = save_apply_now;
  loaded_page_data->reposition = save_reposition;

  /* Maintain the current order but clear any sorting */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(loaded_page_data->tree_model), 
                                       GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                       GTK_SORT_ASCENDING) ;

  return ;
}

static void container_clear(GtkWidget *widget)
{
  if(GTK_IS_CONTAINER(widget))
    {
      GtkContainer *container;
      GList *children;

      container = GTK_CONTAINER(widget);

      children = gtk_container_get_children(container);

      g_list_foreach(children, (GFunc)gtk_widget_destroy, NULL) ;
    }

  return ;
}


static void loaded_page_clear_data(LoadedPageData page_data)
{
  g_list_foreach(page_data->cb_data_list, (GFunc)g_free, NULL) ;
  g_list_free(page_data->cb_data_list);
  page_data->cb_data_list = NULL;

  return ;
}


static void loaded_page_clear  (NotebookPage notebook_page)
{
  container_clear(notebook_page->page_container);

  LoadedPageData page_data = (LoadedPageData)(notebook_page->page_data) ;

  /* Clear the callback data pointers */
  loaded_page_clear_data(page_data) ;

  return ;
}

static void loaded_page_destroy(NotebookPage notebook_page)
{
  return ;
}


/* Return the appropriate display state for the given tree view column */
static ZMapStyleColumnDisplayState get_state_from_tree_store_column(DialogColumns tree_col_id)
{
  ZMapStyleColumnDisplayState result;

  switch (tree_col_id)
    {
    case SHOW_FWD_COLUMN: //fall through
    case SHOW_REV_COLUMN:
      result = ZMAPSTYLE_COLDISPLAY_SHOW ;
      break ;

    case AUTO_FWD_COLUMN: //fall through
    case AUTO_REV_COLUMN:
      result = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE ;
      break ;

    case HIDE_FWD_COLUMN: //fall through
    case HIDE_REV_COLUMN:
      result = ZMAPSTYLE_COLDISPLAY_HIDE ;
      break ;

    default:
      result = ZMAPSTYLE_COLDISPLAY_SHOW ;
      zMapLogWarning("Unexpected tree column number %d", tree_col_id) ;
      break ;
    }

  return result ;
}


/* Finds which radio button in the given tree row is active and returns the appropriate state for
 * that column */
static ZMapStyleColumnDisplayState get_state_from_tree_store_value(GtkTreeModel *model, 
                                                                   GtkTreeIter *iter, 
                                                                   ZMapStrand strand)
{
  ZMapStyleColumnDisplayState result ;

  const gboolean forward = (strand == ZMAPSTRAND_FORWARD) ;

  DialogColumns show_col = forward ? SHOW_FWD_COLUMN : SHOW_REV_COLUMN ;
  DialogColumns auto_col = forward ? AUTO_FWD_COLUMN : AUTO_REV_COLUMN ;
  DialogColumns hide_col = forward ? HIDE_FWD_COLUMN : HIDE_REV_COLUMN ;
  gboolean show_val = FALSE ;
  gboolean auto_val = FALSE ;
  gboolean hide_val = FALSE ;

  gtk_tree_model_get(model, iter,
                     show_col, &show_val,
                     auto_col, &auto_val,
                     hide_col, &hide_val,
                     -1) ;

  DialogColumns active_col = show_col ;

  if (show_val)
    active_col = show_col ;
  else if (auto_val)
    active_col = auto_col ;
  else if (hide_val)
    active_col = hide_col ;
  else
    zMapLogWarning("%s", "Expected one of show/auto/hide to be true but none is") ;

  result = get_state_from_tree_store_column(active_col) ;

  return result ;
}


static void set_tree_store_value_from_state(ZMapStyleColumnDisplayState col_state,
                                            GtkListStore *store,
                                            GtkTreeIter *iter,
                                            const gboolean forward)
{
  DialogColumns show_col = forward ? SHOW_FWD_COLUMN : SHOW_REV_COLUMN ;
  DialogColumns auto_col = forward ? AUTO_FWD_COLUMN : AUTO_REV_COLUMN ;
  DialogColumns hide_col = forward ? HIDE_FWD_COLUMN : HIDE_REV_COLUMN ;
  gboolean show_val = FALSE ;
  gboolean auto_val = FALSE ;
  gboolean hide_val = FALSE ;

  switch(col_state)
    {
    case ZMAPSTYLE_COLDISPLAY_HIDE:
      hide_val = TRUE ;
      break ;
    case ZMAPSTYLE_COLDISPLAY_SHOW_HIDE:
      auto_val = TRUE ;
      break ;
    case ZMAPSTYLE_COLDISPLAY_SHOW:
    default:
      show_val = TRUE ;
      break ;
    }

  gtk_list_store_set(store, iter, 
                     show_col, show_val,
                     auto_col, auto_val, 
                     hide_col, hide_val,
                     -1);

  return ;
}


/* Update the values in the tree store based on the current real state of the given column */
static void loaded_page_update_matching_tree_rows(LoadedPageData loaded_page_data,
                                                  ZMapStrand strand,
                                                  FooCanvasGroup *search_column_group,
                                                  ZMapStyleColumnDisplayState required_state)
{
  zMapReturnIfFail(loaded_page_data &&
                   loaded_page_data->tree_model &&
                   search_column_group) ;


  const gboolean forward = (strand == ZMAPSTRAND_FORWARD) ;

  /* Loop through all rows in the tree until we find the given column group */
  GtkTreeIter iter ;
  GtkTreeModel *model = loaded_page_data->tree_model ;

  if (gtk_tree_model_get_iter_first(model, &iter))
    {
      do
        {
          GQuark column_id = tree_model_get_column_id(model, &iter, TRUE) ;
          FooCanvasGroup *column_group = zmapWindowGetColumnByID(loaded_page_data->window, strand, column_id) ;

          if (column_group == search_column_group)
            {
              /* Update the tree store */
              set_tree_store_value_from_state(required_state, GTK_LIST_STORE(model), &iter, forward) ;

              /* Apply the changes */
              loaded_page_apply_visibility_tree_row(loaded_page_data, strand, model, &iter) ;
              
              break ; // should only be one row with this column id
            }

        } while (gtk_tree_model_iter_next(model, &iter)) ;
    }
}


/* This callback is called to update an exsiting dialog when the user has updated the
 * column display status via another method, e.g. right-click and hide a column. It may only be
 * applicable to a particular column so the relevant column group is passed in the cb_data. */
static void loaded_page_update(NotebookPage notebook_page, ChangeButtonStateData cb_data)
{
  ZMapStrand strand;
  ZMapWindowColConfigureMode mode ;
  LoadedPageData loaded_page_data;


  mode = cb_data->mode ;

  loaded_page_data = (LoadedPageData)(notebook_page->page_data) ;


  /* - Get the correct strand container for labels */
  strand = zmapWindowContainerFeatureSetGetStrand(ZMAP_CONTAINER_FEATURESET(cb_data->column_group)) ;
  if (strand != ZMAPSTRAND_FORWARD && strand != ZMAPSTRAND_REVERSE)
    return ;

  ZMapStyleColumnDisplayState state;

  switch(mode)
    {
    case ZMAPWINDOWCOLUMN_HIDE:
      state = ZMAPSTYLE_COLDISPLAY_HIDE ;
      break;

    case ZMAPWINDOWCOLUMN_SHOW:
      state = ZMAPSTYLE_COLDISPLAY_SHOW ;
      break;

    default:
      state = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE ;
      break;
    }

  gboolean save_reposition, save_apply_now;

  save_apply_now  = loaded_page_data->apply_now;
  save_reposition = loaded_page_data->reposition;

  /* We're only aiming to find one button to toggle so reposition = TRUE is ok. */
  loaded_page_data->apply_now  = TRUE;
  loaded_page_data->reposition = TRUE;

  /* Update the relevant row in the tree store with the required state */
  loaded_page_update_matching_tree_rows(loaded_page_data, strand, cb_data->column_group, state) ;

  //g_list_foreach(button_list_fwd, activate_matching_column, cb_data) ;
  //g_list_foreach(button_list_rev, activate_matching_column, cb_data) ;

  loaded_page_data->apply_now  = save_apply_now;
  loaded_page_data->reposition = save_reposition;

  return ;
}


static NotebookPage loaded_page_create(ColConfigure configure_data, char **page_name_out)
{
  NotebookPage page_data = NULL;

  if((page_data = g_new0(NotebookPageStruct, 1)))
    {
      page_data->configure_data = configure_data;
      page_data->page_name      = g_strdup("Current Columns");

      if(page_name_out)
        *page_name_out = page_data->page_name;

      page_data->page_constructor = loaded_page_construct;
      page_data->page_populate    = loaded_page_populate;
      page_data->page_apply       = loaded_page_apply;
      page_data->page_reorder     = loaded_page_reorder;
      page_data->page_update      = loaded_page_update;
      page_data->page_clear       = loaded_page_clear;
      page_data->page_destroy     = loaded_page_destroy;

      page_data->page_data = g_new0(LoadedPageDataStruct, 1);
    }

  return page_data;
}


static void deferred_page_construct(NotebookPage notebook_page, GtkWidget *page)
{
  notebook_page->page_container = page;

  return ;
}

static void deferred_radio_buttons(GtkWidget *parent, GQuark column_id,  gboolean loaded_in_mark,
                                   DeferredButton *all_out, DeferredButton *mark_out, DeferredButton *none_out)
{
  GtkWidget *radio_load_all, *radio_load_mark, *radio_deferred;
  GtkRadioButton *radio_group_button;
  DeferredButton all, mark, none;

  radio_load_all = gtk_radio_button_new_with_label(NULL, "All Data") ;

  radio_group_button = GTK_RADIO_BUTTON(radio_load_all);

  gtk_box_pack_start(GTK_BOX(parent), radio_load_all, TRUE, TRUE, 0) ;

  radio_load_mark = gtk_radio_button_new_with_label_from_widget(radio_group_button,
                                                                "Marked Area") ;

  gtk_box_pack_start(GTK_BOX(parent), radio_load_mark, TRUE, TRUE, 0) ;

  radio_deferred = gtk_radio_button_new_with_label_from_widget(radio_group_button,
                                                               "Do Not Load/No Data") ;
  gtk_box_pack_start(GTK_BOX(parent), radio_deferred, TRUE, TRUE, 0) ;

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_deferred), TRUE) ;

  if(loaded_in_mark)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_load_mark), TRUE) ;

  all = g_new0(DeferredButtonStruct, 1);
  all->column_button = radio_load_all;
  all->column_quark  = column_id;
  mark = g_new0(DeferredButtonStruct, 1);
  mark->column_button = radio_load_mark;
  mark->column_quark  = column_id;
  none = g_new0(DeferredButtonStruct, 1);
  none->column_button = radio_deferred;
  none->column_quark  = column_id;

  if(all_out)
    *all_out = all;
  if(mark_out)
    *mark_out = mark;
  if(none_out)
    *none_out = none;

  return ;
}

static void each_button_activate(gpointer list_data, gpointer user_data)
{
  DeferredButton button_data = (DeferredButton)list_data;
  GtkWidget *widget;

  widget = button_data->column_button;

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
    }

  return ;
}

static gboolean deferred_activate_all(GtkWidget *button, gpointer user_data)
{
  GList *deferred_button_list = (GList *)user_data;

  g_list_foreach(deferred_button_list, each_button_activate, NULL);

  return TRUE;
}

static gint find_name_cb(gconstpointer list_data, gconstpointer user_data)
{
  const char *list_column_name;
  const char *query_column_name;
  gint match = -1;

  list_column_name  = (const char *)list_data;
  query_column_name = (const char *)user_data;

  if (!list_column_name && !query_column_name)
    match = 0 ;
  else if (!list_column_name)
    match = -1 ;
  else if (!query_column_name)
    match = 1 ;
  else
    match = g_ascii_strcasecmp(list_column_name, query_column_name);

  return match;
}


/* column loaded in a range? (set by the mark) */
/* also called for full range by using seq start and end coords */
static gboolean column_is_loaded_in_range(ZMapFeatureContextMap map,
                                          ZMapFeatureBlock block, GQuark column_id, int start, int end)
{
#define MH17_DEBUG      0
  GList *fsets = NULL;

  if (map)
    fsets = map->getColumnFeatureSets(column_id, TRUE);

#if MH17_DEBUG
  zMapLogWarning("is col loaded %s %d -> %d? ",g_quark_to_string(column_id),start,end);
#endif
  while(fsets)
    {
      gboolean loaded;

      loaded = zMapFeatureSetIsLoadedInRange(block,GPOINTER_TO_UINT(fsets->data), start, end);
#if MH17_DEBUG
      zMapLogWarning("%s loaded: %s,",g_quark_to_string(GPOINTER_TO_UINT(fsets->data)),loaded? "yes":"no");
#endif

      if(!loaded)
        return(FALSE);


      /* NOTE fsets is an allocated list
       * if this is changed to a static one held in the column struct
       * then we should not free it
       */
      /* fsets = g_list_delete_link(fsets,fsets); */
      /* NOTE: now is a cached list */
      fsets = fsets->next;  /* if zMapFeatureGetColumnFeatureSets doesn't allocate the list */
    }
  return TRUE;
}




static GtkWidget *deferred_cols_panel(NotebookPage notebook_page, GList *columns_list, GQuark column_name)
{
  DeferredPageData deferred_page_data;
  GtkWidget *frame = NULL, *vbox, *hbox;
  GtkWidget *label_box, *column_box, *scroll_vbox;
  ZMapWindow window;
  gboolean mark_set = TRUE;
  GList *column;
  int list_length = 0, mark1, mark2;

  deferred_page_data = (DeferredPageData)(notebook_page->page_data);

  window = notebook_page->configure_data->window;


  if((mark_set = zmapWindowMarkIsSet(window->mark)))
    {
      zmapWindowMarkGetSequenceRange(window->mark,
                                     &mark1, &mark2);
    }


  if(column_name)
    frame = gtk_frame_new(g_quark_to_string(column_name));
  else
    frame = gtk_frame_new("Available Columns");
  gtk_container_set_border_width(GTK_CONTAINER(frame),
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame), vbox);

  scroll_vbox = make_scrollable_vbox(vbox);

  /* A box for the label and column boxes */
  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(scroll_vbox), hbox) ;

  /* A box for the labels */
  label_box = gtk_vbox_new(FALSE, 0) ;


  /* A box for the columns */
  column_box = gtk_vbox_new(FALSE, 0) ;


  if((column = g_list_first(columns_list)))
    {
      GList *make_unique = NULL;


      do
        {
          GtkWidget *button_box;
          //  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(column->data);
          char *column_name;
          DeferredButton all, mark, none;
          gboolean loaded_in_mark = FALSE;
          gboolean force_mark = FALSE;
          GQuark col_id;

          column_name = (char *) g_quark_to_string(GPOINTER_TO_UINT(column->data));     //label_text_from_column(column_group);

          if(column_name && !g_list_find_custom(make_unique, column_name, find_name_cb))
            {
              create_label(label_box, column_name);

              button_box = gtk_hbox_new(FALSE, 0);

              gtk_box_pack_start(GTK_BOX(column_box), button_box, TRUE, TRUE, 0);


              if (mark_set)
                {
                  DeferredPageData page_data;
                  ZMapFeatureColumn col = NULL ;

                  page_data   = (DeferredPageData)notebook_page->page_data;

                  GQuark unique_id = zMapFeatureSetCreateID(column_name) ;

                  std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = window->context_map->columns->find(unique_id) ;

                  if (col_iter != window->context_map->columns->end())
                    col = col_iter->second ;

                  if (col)
                    loaded_in_mark = column_is_loaded_in_range(window->context_map,
                                                               page_data->block, col->unique_id, mark1, mark2) ;
                  else
                    zMapWarning("Column lookup failed for '%s'\n", column_name ? column_name : "") ;
                }

              col_id = g_quark_from_string(column_name) ;

              deferred_radio_buttons(button_box, col_id, loaded_in_mark,
                                     &all, &mark, &none);

              all->deferred_page_data = mark->deferred_page_data = none->deferred_page_data = deferred_page_data;

              gtk_widget_set_sensitive(all->column_button, !force_mark);
              gtk_widget_set_sensitive(mark->column_button, mark_set && !loaded_in_mark);

              deferred_page_data->load_all     = g_list_append(deferred_page_data->load_all, all);
              deferred_page_data->load_in_mark = g_list_append(deferred_page_data->load_in_mark, mark);
              deferred_page_data->load_none    = g_list_append(deferred_page_data->load_none, none);

              /* If no mark set then we need to make mark button insensitive */

              list_length++;
              make_unique = g_list_append(make_unique, column_name);
            }
        }
      while((column = g_list_next(column)));

      g_list_free(make_unique);
    }

  if(list_length > 0)
    {
      gtk_container_add(GTK_CONTAINER(hbox), label_box) ;
      gtk_container_add(GTK_CONTAINER(hbox), column_box) ;
      notebook_page->configure_data->has_apply_button = TRUE;
    }

  if(list_length > 1)
    {
      GtkWidget *button, *frame, *button_box;

      frame = gtk_frame_new(NULL) ;

      gtk_container_set_border_width(GTK_CONTAINER(frame),
                                     ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

      gtk_box_pack_end(GTK_BOX(vbox), frame, FALSE, FALSE, 0);

      button_box = gtk_hbutton_box_new();

      gtk_container_add(GTK_CONTAINER(frame), button_box) ;

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Select All Columns Fully");

      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(deferred_activate_all), deferred_page_data->load_all);

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Select All Columns Marked");

      gtk_widget_set_sensitive(button, mark_set);

      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(deferred_activate_all), deferred_page_data->load_in_mark);

      /* button for selecting all full loads */
      button = gtk_button_new_with_label("Select None");

      gtk_box_pack_start(GTK_BOX(button_box), button, TRUE, TRUE, 0);

      g_signal_connect(G_OBJECT(button), "clicked",
                       G_CALLBACK(deferred_activate_all), deferred_page_data->load_none);
    }
  else if(list_length == 0)
    {
      GtkWidget *label;

      label = gtk_label_new("No extra data available");

      gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.0);

      gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
    }

  return frame;
}

static void set_block(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                      ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_BLOCK:
      {
        ZMapWindowContainerGroup *block = (ZMapWindowContainerGroup *)user_data;

        if(block && !(*block))
          *block = container;
      }
      break;
    default:
      break;
    }

  return ;
}

static FooCanvasGroup *configure_get_point_block_container(ColConfigure configure_data,
                                                           FooCanvasGroup *column_group)
{
  FooCanvasGroup *block = NULL;

  if(!column_group)
    {
      ZMapWindow window;
      gboolean mark_set = FALSE;

      window = configure_data->window;

      /* Either look in the mark, or get the first block */
      if(window->mark)
        mark_set = zmapWindowMarkIsSet(window->mark);

      if(mark_set)
        {
#ifdef REMOVE_WORLD2SEQ
          double x1, y1, x2, y2;
          int wy1,wy2;

          zmapWindowMarkGetWorldRange(window->mark, &x1, &y1, &x2, &y2);

          // iffed out  zmapWindowWorld2SeqCoords(window, x1, y1, x2, y2, &block, &wy1, &wy2);
#endif
          block = (FooCanvasGroup *)zmapWindowMarkGetCurrentBlockContainer(window->mark);
        }
      else
        {
          ZMapWindowContainerGroup first_block_container = NULL;
          /* Just get the first one! */
          zmapWindowContainerUtilsExecute(window->feature_root_group,
                                          ZMAPCONTAINER_LEVEL_BLOCK,
                                          set_block, &first_block_container);

          block = (FooCanvasGroup *)first_block_container;
        }
    }
  else
    {
      /* parent block (canvas container) should have block attached... */
      block = (FooCanvasGroup *)zmapWindowContainerUtilsGetParentLevel((ZMapWindowContainerGroup)(column_group),
                                                                       ZMAPCONTAINER_LEVEL_BLOCK);
    }

  return block;
}


/*
 * unloaded columns do not exist, so unlike the loaded columns page that allows show/hide
 * we can't find them from the canvas
 * we need to get the columns list (or featuresets) from ZMap config and
 * select those that aren't fully loaded yet
 * which means a) not including the whole sequenceand b) not including all featuresets
 * NOTE the user selects columns but we request featuresets
 * NOTE we make requests relative to the current block, just in case there is more than one
 *
 * NOTE unlike loaded columns page we store the column names not the canvas containers
 */




static gint col_sort_by_name(gconstpointer a, gconstpointer b)
{
  int result = 0 ;

  /* we sort by unique but display original; the result should be the same */
  if (!a && !b)
    result = 0 ;
  else if (!a)
    result = -1 ;
  else if (!b)
    result = 1 ;
  else
    result = (g_ascii_strcasecmp(g_quark_to_string(GPOINTER_TO_UINT(a)),g_quark_to_string(GPOINTER_TO_UINT(b))));

  return result ;
}

/* get all deferered columns or just the one specified */
static GList *configure_get_deferred_column_lists(ColConfigure configure_data,
                                                  ZMapFeatureBlock block, GQuark column_name)
{
  GList *columns = NULL;
  ZMapWindow window;
  ZMapFeatureColumn column;

  window         = configure_data->window;


  /* get unloaded columns in a list
   * from window->context_map->columns hash table
   * sort into alpha order (not display order)
   * if we are doing 'configure this column' then we only include it
   * if not fully loaded and also include no others
   */
  for (std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = window->context_map->columns->begin();
       col_iter != window->context_map->columns->end();
       ++col_iter)
    {
      column = col_iter->second ;

      if((!column_name || column_name == column->unique_id )
         && (column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME))
         && (column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME))
         && !column_is_loaded_in_range(window->context_map,block,
                                       column->unique_id, window->sequence->start, window->sequence->end)
         && !window->context_map->isSeqColumn(column->unique_id)
         )
        {
          columns = g_list_prepend(columns,GUINT_TO_POINTER(column->column_id));
        }
    }

  columns = g_list_sort(columns,col_sort_by_name);

  return columns;
}


static void deferred_page_populate(NotebookPage notebook_page, FooCanvasGroup *column_group)
{
  GList *all_deferred_columns = NULL;
  GtkWidget *hbox, *frame;
  GQuark column_name = 0;

  DeferredPageData page_data;
  FooCanvasGroup *point_block;

  point_block = configure_get_point_block_container(notebook_page->configure_data, column_group);

  page_data   = (DeferredPageData)notebook_page->page_data;

  page_data->block = zmapWindowItemGetFeatureBlock((FooCanvasItem *)point_block);


  if(column_group)
    column_name = g_quark_from_string(label_text_from_column(column_group));

  all_deferred_columns = configure_get_deferred_column_lists(notebook_page->configure_data,
                                                             page_data->block,column_name);

  hbox = notebook_page->page_container;

  if((frame = deferred_cols_panel(notebook_page, all_deferred_columns,column_name)))
    {
      gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    }

  return ;
}

static void add_name_to_list(gpointer list_data, gpointer user_data)
{
  DeferredButton button_data = (DeferredButton)list_data;
  GList **list_out = (GList **)user_data;
  GtkWidget *widget;

  widget = button_data->column_button;

  if (GTK_IS_RADIO_BUTTON(widget))
    {
      /* some buttons are set insensitive because they do not make sense, e.g. there is no
       * point in fetching data for a marked region if all the data for the column has
       * already been fetched. */
      if (gtk_widget_is_sensitive(widget) && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
        {
          *list_out = g_list_append(*list_out,GUINT_TO_POINTER(button_data->column_quark));

          zMapUtilsDebugPrintf(stdout, "%s\n", g_quark_to_string(button_data->column_quark)) ;
        }
    }

  return ;
}

static void deferred_page_apply(NotebookPage notebook_page)
{
  DeferredPageData deferred_data;
  ColConfigure configure_data;
  FooCanvasGroup *block_group;
  ZMapFeatureBlock block = NULL;
  GList *mark_list = NULL, *all_list = NULL;

  /* Get the data we need */
  configure_data = notebook_page->configure_data;
  deferred_data  = (DeferredPageData)(notebook_page->page_data);

  if((block_group = configure_get_point_block_container(configure_data, NULL)))
    {
      block = zmapWindowItemGetFeatureBlock((FooCanvasItem *)block_group);

      /* Go through the mark only ones... */
      g_list_foreach(deferred_data->load_in_mark, add_name_to_list, &mark_list);

      /* Go through the load all ones... */
      g_list_foreach(deferred_data->load_all, add_name_to_list, &all_list);

      if (mark_list)
        zmapWindowFetchData(configure_data->window, block, mark_list, TRUE, FALSE) ;
      if (all_list)
        zmapWindowFetchData(configure_data->window, block, all_list, FALSE, FALSE) ;
    }

  return ;
}

static void deferred_page_clear(NotebookPage notebook_page)
{
  DeferredPageData page_data;
  container_clear(notebook_page->page_container);

  page_data = (DeferredPageData)(notebook_page->page_data);
  g_list_foreach(page_data->load_in_mark, (GFunc)g_free, NULL);
  g_list_free(page_data->load_in_mark);
  page_data->load_in_mark = NULL;

  g_list_foreach(page_data->load_all, (GFunc)g_free, NULL);
  g_list_free(page_data->load_all);
  page_data->load_all = NULL;

  g_list_foreach(page_data->load_none, (GFunc)g_free, NULL);
  g_list_free(page_data->load_none);
  page_data->load_none = NULL;

  return ;
}
static void deferred_page_destroy(NotebookPage notebook_page)
{
  return ;
}




static NotebookPage deferred_page_create(ColConfigure configure_data, char **page_name_out)
{
  NotebookPage page_data = NULL;

  if((page_data = g_new0(NotebookPageStruct, 1)))
    {
      page_data->configure_data = configure_data;
      page_data->page_name      = g_strdup("Available Columns");

      if(page_name_out)
        *page_name_out = page_data->page_name;

      page_data->page_constructor = deferred_page_construct;
      page_data->page_populate    = deferred_page_populate;
      page_data->page_apply       = deferred_page_apply;
      page_data->page_reorder     = NULL ;
      page_data->page_clear       = deferred_page_clear;
      page_data->page_destroy     = deferred_page_destroy;

      page_data->page_data = g_new0(DeferredPageDataStruct, 1);
    }

  return page_data;
}





static void configure_add_pages(ColConfigure configure_data)
{
  NotebookPage page_data;
  GtkWidget *label, *new_page;
  char *page_name = NULL;

  if((page_data = loaded_page_create(configure_data, &page_name)))
    {
      label = gtk_label_new(page_name);

      configure_data->loaded_page = new_page = gtk_hbox_new(FALSE, 0);

      g_object_set_data(G_OBJECT(new_page), NOTEBOOK_PAGE_DATA, page_data);

      gtk_notebook_append_page(GTK_NOTEBOOK(configure_data->notebook),
                               new_page, label);
      /* get block */

      if(page_data->page_constructor)
        (page_data->page_constructor)(page_data, new_page);
    }

  if((page_data = deferred_page_create(configure_data, &page_name)))
    {
      label = gtk_label_new(page_name);

      configure_data->deferred_page = new_page = gtk_hbox_new(FALSE, 0);

      g_object_set_data(G_OBJECT(new_page), NOTEBOOK_PAGE_DATA, page_data);

      gtk_notebook_append_page(GTK_NOTEBOOK(configure_data->notebook),
                               new_page, label);

      if(page_data->page_constructor)
        (page_data->page_constructor)(page_data, new_page);
    }

  return ;
}

static void configure_populate_containers(ColConfigure configure_data, FooCanvasGroup *column_group)
{

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_POPULATE_FUNC, column_group);

  return ;
}

static void configure_clear_containers(ColConfigure configure_data)
{
  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_CLEAR_FUNC, NULL);

  return;
}
/* Simple make menu bar function */
static GtkWidget *make_menu_bar(ColConfigure configure_data)
{
  GtkAccelGroup *accel_group;
  GtkItemFactory *item_factory;
  GtkWidget *menubar ;
  gint nmenu_items = sizeof (menu_items_G) / sizeof (menu_items_G[0]);

  accel_group  = gtk_accel_group_new ();

  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group );

  gtk_item_factory_create_items(item_factory, nmenu_items, menu_items_G,
                                (gpointer)configure_data) ;

  gtk_window_add_accel_group(GTK_WINDOW(configure_data->window->col_config_window), accel_group) ;

  menubar = gtk_item_factory_get_widget(item_factory, "<main>");

  return menubar ;
}


static GtkWidget *make_scrollable_vbox(GtkWidget *child)
{
  GtkWidget *scroll_vbox, *scrolled, *viewport;

  /* A scrolled window to go in the vbox ... */
  scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(child), scrolled);

  /* ... which needs a box to put the viewport in ... */
  scroll_vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_set_border_width(GTK_CONTAINER(scroll_vbox),
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  /* ... which we need to put the rows of radio buttons in for when
   * there's a large number of canvas columns. */
  viewport = gtk_viewport_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(viewport), scroll_vbox);
  gtk_viewport_set_shadow_type(GTK_VIEWPORT(viewport), GTK_SHADOW_NONE);

  gtk_container_add(GTK_CONTAINER(scrolled), viewport);

  /* Make sure we get the correctly sized widget... */
  zmapAddSizingSignalHandlers(viewport, FALSE, -1, -1);

  return scroll_vbox;
}


/* Utility to create a tree view column with the required properties and add it to the tree view */
static void createTreeViewColumn(GtkTreeView *tree_view,
                                 const char *col_name, 
                                 DialogColumns tree_col_id,
                                 GtkCellRenderer *renderer,
                                 const char *property,
                                 const gboolean sortable)
{
  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(col_name, renderer, property, tree_col_id, NULL);
  
  gtk_tree_view_column_set_resizable(column, TRUE) ;
  gtk_tree_view_column_set_reorderable(column, TRUE) ;
  gtk_tree_view_column_set_min_width(column, 30) ;
  gtk_tree_view_column_set_max_width(column, 800) ;

  if (sortable)
    gtk_tree_view_column_set_sort_column_id(column, tree_col_id) ;
 
  gtk_tree_view_append_column(tree_view, column);
}


/* Create a column in the given tree for the given toggle button column */
static void loaded_cols_panel_create_column(LoadedPageData page_data,
                                            GtkTreeModel *model,
                                            GtkTreeView *tree,
                                            ZMapStrand strand, 
                                            DialogColumns tree_col_id, 
                                            const char *col_name)
{
  ShowHideData show_hide_data = g_new0(ShowHideDataStruct, 1) ;
  show_hide_data->loaded_page_data = page_data ;
  show_hide_data->tree_col_id = tree_col_id ;
  show_hide_data->strand = strand ;

  page_data->cb_data_list = g_list_append(page_data->cb_data_list, show_hide_data) ;

  GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE) ;
  g_signal_connect (renderer, "toggled", G_CALLBACK (loaded_column_visibility_toggled_cb), show_hide_data);

  createTreeViewColumn(GTK_TREE_VIEW(tree), col_name, tree_col_id, renderer, "active", FALSE) ;
}

/* Called after the tree selection has changed. Updates info about the selected tracks in the
 * selection_feedback label. */
static void tree_selection_changed_cb(GtkTreeSelection *selection, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->selection_feedback) ;

  int num_selected = gtk_tree_selection_count_selected_rows(selection) ;

  stringstream ss ;

  if (num_selected < 1)
    ss << "No tracks selected" ;
  else if (num_selected == 1)
    ss << num_selected << " track selected" ;
  else
    ss << num_selected << " tracks selected" ;

  gtk_label_set_text(page_data->selection_feedback, ss.str().c_str()) ;
}


/* Callback called when the user clicks a row, before the selection is changed. Returns true
 * if it's ok to change the selection, false otherwise. This is used to disable changing the
 * selection when also clicking on a toggle button */
static gboolean tree_select_function_cb(GtkTreeSelection *selection, 
                                        GtkTreeModel *model,
                                        GtkTreePath *path,
                                        gboolean path_currently_selected,
                                        gpointer user_data)
{
  gboolean result = TRUE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnValIfFail(page_data, result) ;

  const int num_selected = gtk_tree_selection_count_selected_rows(selection) ;

  /* Disable changing the selection if the user clicked on a toggle button and
   * we have a multiple selection */
  if (page_data->clicked_button && num_selected > 1)
    {
      result = FALSE ;
    }

  return result ;
}


/* Called when the user selects a style on the tree right-click menu */
//static void tree_view_popup_menu_selected_cb(GtkWidget *menuitem, gpointer userdata)
//{
//}


static gboolean tree_view_show_popup_menu(GtkWidget *tree_view, LoadedPageData page_data, GdkEvent *event)
{
  gboolean result = FALSE ;

//  GtkWidget *menu = gtk_menu_new();
//
//  GtkWidget *menuitem = gtk_menu_item_new_with_label("Do something");
//
//  g_signal_connect(menuitem, "activate", G_CALLBACK(tree_view_popup_menu_selected_cb), page_data);
//
//  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
//
//  gtk_widget_show_all(menu);
//
//  /* Note: event can be NULL here when called from view_onPopupMenu;
//   *  gdk_event_get_time() accepts a NULL argument */
//  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gdk_event_get_time(event));

  return result ;
}


static gboolean tree_view_button_press_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gboolean result = FALSE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnValIfFail(page_data, result) ;

  if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
      /* single click with the right mouse button - show context menu */
      result = tree_view_show_popup_menu(widget, page_data, (GdkEvent*)event) ;
    }

  return result ;
}


static gboolean tree_view_button_release_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  gboolean result = FALSE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnValIfFail(page_data, result) ;

  page_data->clicked_button = FALSE ;

  return result ;
}


static gboolean tree_view_popup_menu_cb(GtkWidget *tree_view, gpointer user_data)
{
  gboolean result = FALSE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnValIfFail(page_data, result) ;

  result = tree_view_show_popup_menu(tree_view, page_data, NULL) ;

  return result ;
}


/* Case insensitive version of strstr */
static char* my_strcasestr(const char *haystack, const char *needle)
{
  char *result = 0 ;

  if (haystack && needle && *haystack && *needle)
    {
      char *h = g_ascii_strdown(haystack, -1) ;
      char *n = g_ascii_strdown(needle, -1) ;
      
      result = strstr(h, n) ;

      g_free(h) ;
      g_free(n) ;
    }

  return result ;
}


/* Utility to see if the value in the given column matches the given text */
static gboolean columnMatchesText(GtkTreeModel *model,
                                  GtkTreeIter *iter,
                                  DialogColumns col_id, 
                                  const char *text)
{
  gboolean result = FALSE ;

  // Get the string value for this column from the selected row in the drop-down box
  char *value = NULL ;
  gtk_tree_model_get(model, iter, col_id, &value, -1) ;

  if (text &&
      value && 
      *text != 0 &&
      *value != 0 && 
      my_strcasestr(value, text) != NULL)
    {
      result = TRUE ;
    }

  g_free(value) ;

  return result ;
}


/* Callback used to determine if a search term matches a row in the tree. Returns false if it
 * matches, true otherwise. */
static gboolean tree_view_search_equal_func_cb(GtkTreeModel *model,
                                               gint column,
                                               const gchar *key,
                                               GtkTreeIter *iter,
                                               gpointer user_data)
{
  gboolean result = TRUE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  gboolean found = FALSE ;

  /* Get the column to search */
  DialogColumns col_id = comboGetValue(page_data->search_field_combo) ;

  if (col_id != NO_COLUMN)
    {
      // Check the specified column
      found = columnMatchesText(model, iter, col_id, key) ;
    }
  else
    {
      // Check all relevant columns
      for (auto &field_iter : searchfields_G)
        {
          if (field_iter.first != NO_COLUMN)
            found |= columnMatchesText(model, iter, field_iter.first, key) ;
        }
    }

  // The return value is false if it matches, true otherwise
  result = !found ;

  return result ;
}



/* Create the tree view widget */
static GtkWidget* loaded_cols_panel_create_tree_view(LoadedPageData page_data, 
                                                     GtkTreeModel *model)
{
  GtkWidget *tree = NULL ;
  zMapReturnValIfFail(page_data && model, tree) ;

  /* Create the tree view widget */
  tree = gtk_tree_view_new_with_model(model);
  g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(tree_view_button_press_cb), page_data) ;
  g_signal_connect(G_OBJECT(tree), "button-release-event", G_CALLBACK(tree_view_button_release_cb), page_data) ;
  g_signal_connect(G_OBJECT(tree), "popup-menu", G_CALLBACK(tree_view_popup_menu_cb), page_data) ;


  GtkTreeView *tree_view = GTK_TREE_VIEW(tree) ;

  /* Make the tree reorderable so users can drag-and-drop rows to reorder columns */
  gtk_tree_view_set_reorderable(tree_view, TRUE);

  /* Set up searching */
  if (page_data->search_entry)
    {
      gtk_tree_view_set_enable_search(tree_view, FALSE);
      gtk_tree_view_set_search_column(tree_view, NAME_COLUMN);
      gtk_tree_view_set_search_entry(tree_view, page_data->search_entry) ;
      gtk_tree_view_set_search_equal_func(tree_view, tree_view_search_equal_func_cb, page_data, NULL) ;
    }

  GtkTreeSelection *tree_selection = gtk_tree_view_get_selection(tree_view) ;
  gtk_tree_selection_set_mode(tree_selection, GTK_SELECTION_MULTIPLE);
  gtk_tree_selection_set_select_function(tree_selection, tree_select_function_cb, page_data, NULL) ;
  g_signal_connect(G_OBJECT(tree_selection), "changed", G_CALLBACK(tree_selection_changed_cb), page_data) ;

  /* Create the name column */
  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
  createTreeViewColumn(tree_view, "Column", NAME_COLUMN, text_renderer, "text", TRUE) ;

  /* Create the radio button columns */
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, SHOW_FWD_COLUMN, "FS") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, AUTO_FWD_COLUMN, "FA") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, HIDE_FWD_COLUMN, "FH") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, SHOW_REV_COLUMN, "RS") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, AUTO_REV_COLUMN, "RA") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, HIDE_REV_COLUMN, "RH") ;

  /* Create the bumped column */
  createTreeViewColumn(tree_view, "Bumped", BUMPED_COLUMN, text_renderer, "text", TRUE) ;

  /* Create the group column */
  createTreeViewColumn(tree_view, "Group", GROUP_COLUMN, text_renderer, "text", TRUE) ;

  /* Create the style column */
  createTreeViewColumn(tree_view, "Styles", STYLE_COLUMN, text_renderer, "text", TRUE) ;

  /* Create the source column */
  createTreeViewColumn(tree_view, "Source", SOURCE_COLUMN, text_renderer, "text", TRUE) ;


  page_data->tree_view = GTK_TREE_VIEW(tree) ;

  return tree ;
}


/* Called on each entry in a hash table of glists and sets the result if this glist contains the
 * given search id */
static void hashListFindIDCB(gpointer key, gpointer data, gpointer user_data)
{
  GQuark group_id = (GQuark)GPOINTER_TO_INT(key) ;
  GList *group_featuresets = (GList*)data ;
  HashListFindID find_data = (HashListFindID)user_data ;

  if (group_id && group_featuresets)
    {
      GQuark unique_id = zMapStyleCreateID(g_quark_to_string(find_data->search_id)) ;

      for (GList *item = group_featuresets; item; item = item->next)
        {
          GQuark group_featureset_id = GPOINTER_TO_INT(item->data) ;
          GQuark group_unique_id = zMapStyleCreateID(g_quark_to_string(group_featureset_id)) ;

          if (group_featureset_id == find_data->search_id ||
              group_unique_id == unique_id)
            {
              find_data->result = group_id ;
              break ;
            }
        }
    }
}


/* Return the key in the hash table of the entry that contains the given id in a list. Returns 0
 * if not found. Takes a hash table of GQuark to GList of GQuarks and looks for the search_id in
 * the GList. */
static GQuark hashListFindID(GHashTable *ghash, const GQuark search_id)
{
  HashListFindIDStruct data = {0, search_id} ;

  g_hash_table_foreach(ghash, hashListFindIDCB, &data) ;

  return data.result ;
}


static string getBumpedText(ZMapWindowContainerFeatureSet container_fwd,
                            ZMapWindowContainerFeatureSet container_rev)
{
  /* Set whether bumped or unbumped. We don't currently bother listing forward and rev
   * strand: just show bumped or unbumped if both are the same or blank otherwise. */
  string bumped_text ;

  if (container_fwd && container_rev)
    {
      ZMapStyleBumpMode bumped_fwd = zmapWindowContainerFeatureSetGetBumpMode(container_fwd) ;
      ZMapStyleBumpMode bumped_rev = zmapWindowContainerFeatureSetGetBumpMode(container_rev) ;
      
      if (bumped_fwd == ZMAPBUMP_UNBUMP && bumped_rev == ZMAPBUMP_UNBUMP)
        bumped_text = UNBUMPED_TEXT ;
      else if (bumped_fwd != ZMAPBUMP_UNBUMP && bumped_rev != ZMAPBUMP_UNBUMP)
        bumped_text = BUMPED_TEXT ;
    }
  else if (container_fwd || container_rev)
    {
      ZMapStyleBumpMode bumped ;

      if (container_fwd)
        bumped = zmapWindowContainerFeatureSetGetBumpMode(container_fwd) ;
      else
        bumped = zmapWindowContainerFeatureSetGetBumpMode(container_rev) ;

      if (bumped == ZMAPBUMP_UNBUMP)
        bumped_text = UNBUMPED_TEXT ;
      else
        bumped_text = BUMPED_TEXT ;
    }

  return bumped_text ;
}


static void loaded_cols_panel_create_tree_row(LoadedPageData page_data, 
                                              GtkListStore *store,
                                              ZMapFeatureColumn column,
                                              FooCanvasGroup *column_group_fwd,
                                              FooCanvasGroup *column_group_rev)
{
  ZMapWindowContainerFeatureSet container_fwd = (ZMapWindowContainerFeatureSet)column_group_fwd ;
  ZMapWindowContainerFeatureSet container_rev = (ZMapWindowContainerFeatureSet)column_group_rev ;

  zMapReturnIfFail(page_data && page_data->configure_data && column && store) ;

  /* Only add a row if this column exists. Ignore default columns */
  if ((container_fwd || container_rev) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_SCRATCH_NAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_HAND_BUILT_NAME) &&
      column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_ORF_NAME))
      //column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_LOCUS_NAME) &&
      //column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_GFF_NAME) &&
      //column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_SCALE_NAME) &&
      //column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME) &&
      //column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_ASSEMBLY_PATH_NAME) &&
    {
      /* Create a new row in the list store */
      GtkTreeIter iter ;
      gtk_list_store_append(store, &iter);

      /* Set the column name */
      const char *label_text = g_quark_to_string(column->column_id);
      gtk_list_store_set(store, &iter, NAME_COLUMN, label_text, -1);

      /* Set the style names */
      char *style_names = columnGetStylesList(page_data, page_data->window->context_map, column->unique_id) ;
      
      if (style_names)
        {
          gtk_list_store_set(store, &iter, STYLE_COLUMN, style_names, -1);
          g_free(style_names) ;
        }

      /* Set the source name(s) */
      string source_names = columnGetSourcesList(page_data, page_data->window->context_map, column->unique_id) ;
      gtk_list_store_set(store, &iter, SOURCE_COLUMN, source_names.c_str(), -1);

      /* Set the group name */
      GQuark group_id = hashListFindID(page_data->window->context_map->column_groups, column->column_id) ;
 
      if (!group_id)
        group_id = hashListFindID(page_data->window->context_map->column_groups, column->unique_id) ;
          
      if (group_id)
        gtk_list_store_set(store, &iter, GROUP_COLUMN, g_quark_to_string(group_id), -1);

      string bumped_text = getBumpedText(container_fwd, container_rev) ;
      gtk_list_store_set(store, &iter, BUMPED_COLUMN, bumped_text.c_str(), -1);

      /* Show two sets of radio buttons for each column to change column display state for
       * each strand. gb10: not sure why but historically we only added an apply button if we set
       * non-default values for either forward or rev strand. I've kept this behaviour for now.  */
      if (container_fwd)
        {
          ZMapStyleColumnDisplayState col_state = zmapWindowContainerFeatureSetGetDisplay(container_fwd) ;
          set_tree_store_value_from_state(col_state, store, &iter, TRUE) ;
          page_data->configure_data->has_apply_button = TRUE;
        }
      else
        {
          /* Auto by default */
          set_tree_store_value_from_state(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE, store, &iter, TRUE) ;
        }

      if (container_rev)
        {
          ZMapStyleColumnDisplayState col_state = zmapWindowContainerFeatureSetGetDisplay(container_rev) ;
          set_tree_store_value_from_state(col_state, store, &iter, FALSE) ;
          page_data->configure_data->has_apply_button = TRUE;
        }
      else
        {
          /* Auto by default */
          set_tree_store_value_from_state(ZMAPSTYLE_COLDISPLAY_SHOW_HIDE, store, &iter, FALSE) ;
        }
    }
}


/* Callback used to determine if a given row in the filtered tree model is visible */
static gboolean tree_model_filter_visible_cb(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
  gboolean result = FALSE ;

  LoadedPageData page_data = (LoadedPageData)user_data ;
  GtkEntry *search_entry = page_data->filter_entry ;
  zMapReturnValIfFail(search_entry, result) ;

  const char *text = gtk_entry_get_text(search_entry) ;

  if (!text || *text == 0)
    {
      /* If text isn't specified, show everything */
      result = TRUE ;
    }
  else
    {
      /* Get the column to filter */
      DialogColumns col_id = comboGetValue(page_data->search_field_combo) ;
      
      if (col_id != NO_COLUMN)
        {
          // Check the specified column
          result = columnMatchesText(model, iter, col_id, text) ;
        }
      else
        {
          // Check all relevant columns
          for (auto &field_iter : searchfields_G)
            {
              if (field_iter.first != NO_COLUMN)
                result |= columnMatchesText(model, iter, field_iter.first, text) ;
            }
        }
    }

  return result ;
}


/* Create the tree store containing info about the columns */
static GtkTreeModel* loaded_cols_panel_create_tree_model(LoadedPageData page_data, 
                                                         FooCanvasGroup *required_column_group)
{
  GtkTreeModel *model = NULL ;
  zMapReturnValIfFail(page_data && page_data->window && page_data->window->context_map, model) ;

  /* Get list of column structs in current display order */
  list<ZMapFeatureColumn> columns_list = page_data->window->context_map->getOrderedColumnsList() ;

  /* Create a tree store containing one row per column */
  GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, 
                                           G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, 
                                           G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
                                           G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING) ;

  /* Loop through all columns in display order (columns are shown in mirror order on the rev
   * strand but we always use forward-strand order) */
  for (auto col_iter = columns_list.begin(); col_iter != columns_list.end(); ++col_iter)
    {
      ZMapFeatureColumn column = *col_iter ;

      FooCanvasGroup *column_group_fwd = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_FORWARD, column->unique_id) ;
      FooCanvasGroup *column_group_rev = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_REVERSE, column->unique_id) ;

      /* If looking for a specific column group, skip if this isn't it */
      if (required_column_group && required_column_group != column_group_fwd && required_column_group != column_group_rev)
        continue ;

      loaded_cols_panel_create_tree_row(page_data, store, column, column_group_fwd, column_group_rev) ;
    }

  /* Return the standard model - this will be used by default so that it can be reordered */
  model = GTK_TREE_MODEL(store) ;
  page_data->tree_model = model ;

  /* Create a filtered tree model which will filter by the text in the filter_entry (if not empty) */
  if (page_data->filter_entry)
    {
      GtkTreeModel *filtered = gtk_tree_model_filter_new(model, NULL) ;
     
      gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filtered), 
                                             tree_model_filter_visible_cb, 
                                             page_data,
                                             NULL);
      page_data->tree_filtered = filtered ;
    }

  return model ;
}


/* Callback called when the user hits the enter key in the filter text entry box */
static void filter_entry_activate_cb(GtkEntry *entry, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model) ;

  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(page_data->tree_filtered) ;

  /* Refilter the filtered model */
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;

  /* If the entry is empty then use the unfiltered tree model so that it can be
   * reordered. Otherwise use the filtered model (which will disable reordering). */
  const char *text = gtk_entry_get_text(entry) ;

  if (!text || *text == 0 || !filter)
    gtk_tree_view_set_model(page_data->tree_view, page_data->tree_model) ;
  else
    gtk_tree_view_set_model(page_data->tree_view, page_data->tree_filtered) ;
}


/* Utility to get a GtkTreeIter for the given path from the tree model. This uses the filtered
 * model if appropriate, or the base model otherwise. Sets the relevant model in model_out. */
static void treeGetIter(GtkTreePath *path, 
                        GtkTreeModel *model_in, 
                        GtkTreeModel **model_out, 
                        GtkTreeIter *iter_out)
{
  GtkTreeModel *model = model_in ;

  if (GTK_IS_TREE_MODEL_FILTER(model_in))
    {
      /* Its the filtered model, so we must find the child model and iter */
      model = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model)) ;
      GtkTreeIter iter_in ;
      gtk_tree_model_get_iter(model_in, &iter_in, path) ;
      gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(model_in), iter_out, &iter_in) ;
    }
  else
    {
      gtk_tree_model_get_iter(model_in, iter_out, path) ;
    }

  if (model_out)
    *model_out = model ;
}


/* Called for each row in the current selection to set the style to the last-selected style in
 * the data */
static void set_column_style(GtkTreeModel *model_in, GtkTreePath *path, LoadedPageData page_data)
{
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  /* Update the group column in the tree model */
  GtkTreeIter iter ;
  gtk_tree_model_get_iter(model_in, &iter, path) ;

  gtk_list_store_set(GTK_LIST_STORE(model_in), &iter, 
                     STYLE_COLUMN, g_quark_to_string(page_data->last_style_id),
                     -1) ;
}


/* Called when the user hits the select-all button. */
static void select_all_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(page_data->tree_view) ;

  if (selection)
    {
      gtk_tree_selection_select_all(selection) ;
    }
}

/* Called when the user hits the select-none button. */
static void select_none_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(page_data->tree_view) ;

  if (selection)
    {
      gtk_tree_selection_unselect_all(selection) ;
    }
}

/* Called when the user hits the invert-selection button. */
static void select_invert_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(page_data->tree_view) ;

  // Loop through all rows in the tree model and toggle the selection status
  GtkTreeIter iter ;

  for (GtkTreePath *path = gtk_tree_path_new_first(); 
       gtk_tree_model_get_iter(page_data->tree_model, &iter, path);
       gtk_tree_path_next(path))
    {
      if (gtk_tree_selection_path_is_selected(selection, path))
        gtk_tree_selection_unselect_path(selection, path) ;
      else
        gtk_tree_selection_select_path(selection, path) ;
    }
}


/* Get the selected rows from the given model as row references (these can be edited without
 * safely without messing up the iterators e.g. if the changes cause the row order to change).
 * Optionally also return the tree model. */
static list<GtkTreeRowReference*> getSelectedRows(GtkTreeView *tree_view,
                                                  GtkTreeModel **model_out = NULL)
{
  list<GtkTreeRowReference*> row_refs ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view) ;

  if (selection)
    {
      GtkTreeModel *model = NULL ;
      GList *rows = gtk_tree_selection_get_selected_rows(selection, &model) ;
      GtkTreeModel *model_base = model ;

      // get the base (unfiltered) model
      if (GTK_IS_TREE_MODEL_FILTER(model))
        model_base = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(model)) ;

      if (model_out)
        *model_out = model_base ;

      // Loop through the selection and convert the paths to row references. 
      for (GList *item = rows; item; item = item->next)
        {
          GtkTreePath *path = (GtkTreePath*)(item->data) ;

          // convert to path in base model
          GtkTreeIter iter_base ;
          treeGetIter(path, model, NULL, &iter_base) ;
          GtkTreePath *path_base = gtk_tree_model_get_path(model_base, &iter_base) ;

          row_refs.push_back(gtk_tree_row_reference_new(model_base, path_base)) ;

          gtk_tree_path_free(path_base) ;
        }

      // Clean up
      g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);
    }

  return row_refs ;
}


/* Called when the user hits the set-group button. Prompts for a style and assigns it to the
 * currently-selected rows. */
static void choose_style_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  GtkTreeModel *model = NULL ;
  list<GtkTreeRowReference*> selected_rows = getSelectedRows(page_data->tree_view, &model) ;

  GList *feature_sets = tree_view_get_selected_featuresets(page_data) ;

  if (!selected_rows.empty())
    {
      /* Open a dialog for the user to choose a style */
      GQuark style_id = zMapWindowChooseStyleDialog(page_data->window, feature_sets) ;

      if (style_id)
        {
          page_data->last_style_id = style_id ;

          // Loop through each row ref and update the group
          for (GtkTreeRowReference *row : selected_rows)
            {
              GtkTreePath *path = gtk_tree_row_reference_get_path(row) ;

              set_column_style(model, path, page_data) ;

              gtk_tree_row_reference_free(row) ;
              gtk_tree_path_free(path) ;
            }

          /* Apply the changes (if the apply-now flag is set) */
          loaded_page_apply_styles(page_data) ;
        }
    }
  else
    {
      zMapWarning("%s", "Please select one or more columns") ;
    }
}


/* Called for each row in the current selection to set the group to the last-selected group in
 * the data */
static void set_column_group(LoadedPageData page_data, 
                             GtkTreeModel *model_in, 
                             GtkTreePath *path)
{
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;

  /* Update the group column in the tree model */
  GtkTreeIter iter ;
  GtkTreeModel *model = model_in ;

  treeGetIter(path, model_in, &model, &iter) ;

  if (page_data->last_group_id)
    {
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
                         GROUP_COLUMN, g_quark_to_string(page_data->last_group_id),
                         -1) ;
    }
  else
    {
      gtk_list_store_set(GTK_LIST_STORE(model), &iter, 
                         GROUP_COLUMN, NULL,
                         -1) ;
    }
}


/* Set the visibility of the selected tracks to the given state */
static void selected_tracks_set_visibility(LoadedPageData page_data,
                                           ZMapStyleColumnDisplayState col_state)
{
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  GtkTreeModel *model = NULL ;
  list<GtkTreeRowReference*> selected_rows = getSelectedRows(page_data->tree_view, &model) ;

  if (!selected_rows.empty())
    {
      // Loop through each row ref and update the group
      for (GtkTreeRowReference *row : selected_rows)
        {
          GtkTreePath *path = gtk_tree_row_reference_get_path(row) ;
          GtkTreeIter iter ;
          gtk_tree_model_get_iter(model, &iter, path) ;

          /* Set the visibility of both strands */
          set_tree_store_value_from_state(col_state, GTK_LIST_STORE(model), &iter, TRUE) ;
          set_tree_store_value_from_state(col_state, GTK_LIST_STORE(model), &iter, FALSE) ;

          gtk_tree_row_reference_free(row) ;
          gtk_tree_path_free(path) ;
        }
    }
}

/* Set the visibility of the selected tracks to the given state */
static void selected_tracks_set_bumped(LoadedPageData page_data,
                                       const bool bumped)
{
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  GtkTreeModel *model = NULL ;
  list<GtkTreeRowReference*> selected_rows = getSelectedRows(page_data->tree_view, &model) ;

  if (!selected_rows.empty())
    {
      // Loop through each row ref and update the group
      for (GtkTreeRowReference *row : selected_rows)
        {
          GtkTreePath *path = gtk_tree_row_reference_get_path(row) ;
          GtkTreeIter iter ;
          gtk_tree_model_get_iter(model, &iter, path) ;

          string bumped_text = (bumped ? BUMPED_TEXT : UNBUMPED_TEXT) ;
          gtk_list_store_set(GTK_LIST_STORE(model), &iter, BUMPED_COLUMN, bumped_text.c_str(), -1);

          gtk_tree_row_reference_free(row) ;
          gtk_tree_path_free(path) ;
        }
    }
}

/* Called when the user hits the button to show sleected tracks. */
static void show_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  selected_tracks_set_visibility(page_data, ZMAPSTYLE_COLDISPLAY_SHOW) ;
}

/* Called when the user hits the button to Auto-show selected tracks. */
static void auto_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  selected_tracks_set_visibility(page_data, ZMAPSTYLE_COLDISPLAY_SHOW_HIDE) ;
}

/* Called when the user hits the button to hide selected tracks. */
static void hide_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  selected_tracks_set_visibility(page_data, ZMAPSTYLE_COLDISPLAY_HIDE) ;
}

/* Called when the user hits the button to bump selected tracks. */
static void bump_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  selected_tracks_set_bumped(page_data, true) ;
}

/* Called when the user hits the button to unbump sleected tracks. */
static void unbump_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_view) ;

  selected_tracks_set_bumped(page_data, false) ;
}


/* Called when the user hits the set-group button. Prompts for a group name and applies it to the
 * currently-selected rows. */
static void set_group_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model && page_data->window) ;
  
  GtkTreeModel *model = NULL ;
  list<GtkTreeRowReference*> selected_rows = getSelectedRows(page_data->tree_view, &model) ;

  if (!selected_rows.empty())
    {
      const char *msg = "Please enter a group name (lowercase only).\n\nClear the text to remove the group." ;
      char *text_out = NULL ;

      GtkResponseType response = zMapGUIMsgGetText(NULL, ZMAP_MSG_INFORMATION, msg, FALSE, &text_out) ;

      if (response == GTK_RESPONSE_OK)
        {
          if (text_out && *text_out)
            page_data->last_group_id = zMapStyleCreateID(text_out) ;
          else
            page_data->last_group_id = 0 ;

          // Loop through each row ref and update the group
          for (GtkTreeRowReference *row : selected_rows)
            {
              GtkTreePath *path = gtk_tree_row_reference_get_path(row) ;
              set_column_group(page_data, model, path) ;
              gtk_tree_row_reference_free(row) ;
              gtk_tree_path_free(path) ;
            }

          // Apply the changes (if the apply-now flag is set)
          loaded_page_apply_groups(page_data) ;
         }
    }
  else
    {
      zMapWarning("%s", "Please select one or more columns") ;
    }
}


/* Clear the text in the search/filter boxes */
static void clearSearchFilter(LoadedPageData page_data)
{
  zMapReturnIfFail(page_data) ;

  if (page_data->search_entry)
    gtk_entry_set_text(page_data->search_entry, "") ;

  if (page_data->filter_entry)
    gtk_entry_set_text(page_data->filter_entry, "") ;

  /* Refilter the filtered model */
  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(page_data->tree_filtered) ;
  if (filter)
    gtk_tree_model_filter_refilter(filter) ;

  /* Use the unfiltered tree model so that it can be reordered */
  gtk_tree_view_set_model(page_data->tree_view, page_data->tree_model) ;

  /* Clear the sort column so that the tree can be reordered again manually. Note that this does
   * not revert the sort order - use the Revert button for that. */
  gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(page_data->tree_model), 
                                       GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                       GTK_SORT_ASCENDING) ;
}


/* Callback to clear the current search/filter */
static void clear_button_cb(GtkButton *button, gpointer user_data)
{
  LoadedPageData page_data = (LoadedPageData)user_data ;
  zMapReturnIfFail(page_data && page_data->tree_model) ;

  clearSearchFilter(page_data) ;
}


/* Returns the selected value in the combo box */
static DialogColumns comboGetValue(GtkComboBox *combo)
{
  DialogColumns result = NO_COLUMN ;

  GtkTreeIter iter;
  
  if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combo), &iter))
    {
      GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo));
      char *value = NULL ;

      // Get the string value
      gtk_tree_model_get(model, &iter, 0, &value, -1);

      if (value)
        {
          // Find this string value in the map of search fields
          auto search_result = find_if(searchfields_G.begin(), searchfields_G.end(),
                                       [&value](const pair<DialogColumns, string> &search_pair) 
                                       { 
                                         return search_pair.second == value ; 
                                       }) ;
          
          if (search_result != searchfields_G.end())
            result = search_result->first ;

          g_free(value) ;
        }
    }

  return result ;
}


/* Called when the user changes the value in the search/filter drop-down box */
static void search_field_changed_cb(GtkComboBox *combo, gpointer data)
{
  LoadedPageData page_data = (LoadedPageData)data ;
  zMapReturnIfFail(page_data) ;

  // Just update the search field in the data
  page_data->search_field = comboGetValue(combo) ;

  // Reset the search/filter boxes because any results are now invalid
  clearSearchFilter(page_data) ;
}


static void createComboItem(GtkComboBox *combo, 
                            GtkListStore *store,
                            const char *text,
                            const bool active)
{
  GtkTreeIter iter;
  gtk_list_store_append(store, &iter);
  gtk_list_store_set(store, &iter, 0, text, -1);

  if (active)
    gtk_combo_box_set_active_iter(combo, &iter);
}


static GtkComboBox *createSearchFieldCombo(GCallback cb_func, gpointer cb_data)
{
  GtkComboBox *combo = NULL ;

  LoadedPageData page_data = (LoadedPageData)cb_data ;
  zMapReturnValIfFail(page_data, combo) ;

  /* Create a combo box to choose the type of source to create */
  GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
  combo = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(store)));
  g_object_unref(store) ;

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
  gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 0, NULL);

  gtk_widget_set_tooltip_text(GTK_WIDGET(combo), "Select which field to search/filter") ;

  /* Add the rows to the combo box. Make "All" (NO_COLUMN) active by default. */
  for (auto &iter : searchfields_G)
    {
      createComboItem(combo, store, iter.second.c_str(), iter.first == NO_COLUMN) ;
    }

  g_signal_connect(G_OBJECT(combo), "changed", cb_func, cb_data);

  page_data->search_field_combo = combo ;

  return combo ;
}


static GtkWidget* loaded_cols_panel_create_buttons(LoadedPageData page_data)
{
  zMapReturnValIfFail(page_data && page_data->configure_data, NULL) ;

  GtkWidget *vbox = gtk_vbox_new(FALSE, XPAD) ;

  /* Add search/filter boxes (only if viewing multiple columns) */
  if (page_data->configure_data->mode == ZMAPWINDOWCOLUMN_CONFIGURE_ALL)
    {
      GtkWidget *hbox = gtk_hbox_new(FALSE, XPAD) ;
      gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;

      gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Search:"), FALSE, FALSE, 0) ;
      page_data->search_entry = GTK_ENTRY(gtk_entry_new()) ;
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(page_data->search_entry), FALSE, FALSE, 0) ;
      gtk_widget_set_tooltip_text(GTK_WIDGET(page_data->search_entry), "Search for column names containing this text. Press the up/down arrows to progress to next/previous match.") ;

      gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Filter:"), FALSE, FALSE, 0) ;
      page_data->filter_entry = GTK_ENTRY(gtk_entry_new()) ;
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(page_data->filter_entry), FALSE, FALSE, 0) ;
      g_signal_connect(G_OBJECT(page_data->filter_entry), "activate", G_CALLBACK(filter_entry_activate_cb), page_data) ;
      gtk_widget_set_tooltip_text(GTK_WIDGET(page_data->filter_entry), "Show only column names containing this text. Press enter to apply the filter.") ;

      /* Add a drop-down box to select which fields to search/filter by */
      GtkWidget *combo = GTK_WIDGET(createSearchFieldCombo(G_CALLBACK(search_field_changed_cb), page_data)) ;
      gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 0) ;

      /* Add a button to clear the search/filter boxes and sort order */
      GtkWidget *button = gtk_button_new_with_mnemonic("C_lear") ;
      gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
      g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(clear_button_cb), page_data) ;
      gtk_widget_set_tooltip_text(button, "Clear the search/filter boxes and the sort order, so that rows can be reordered manually") ;
    }

  GtkWidget *hbox = gtk_hbox_new(FALSE, XPAD) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Select:"), FALSE, FALSE, 0) ;

  /* Add a button to select all the visible rows */
  GtkWidget *button = gtk_button_new_with_mnemonic("Al_l") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_all_button_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Select all visible tracks") ;

  /* Add a button to unselect all visible rows */
  button = gtk_button_new_with_mnemonic("_None") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_none_button_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Deselect all visible tracks") ;

  /* Add a button to invert the current selection */
  button = gtk_button_new_with_mnemonic("_Invert") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_invert_button_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Invert the current selection") ;

  /* Add a text box to show info about the selection */
  page_data->selection_feedback = GTK_LABEL(gtk_label_new("")) ;
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(page_data->selection_feedback), FALSE, FALSE, 0) ;


  hbox = gtk_hbox_new(FALSE, XPAD) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Action:"), FALSE, FALSE, 0) ;


  /* Add a button to Show all selected columns */
  button = gtk_button_new_with_mnemonic("Sho_w") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(show_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Set the visibility of all selected tracks to Show") ;

  /* Add a button to Auto all selected columns */
  button = gtk_button_new_with_mnemonic("_Auto") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(auto_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Set visibility of all selected tracks to Auto") ;

  /* Add a button to Hide all selected columns */
  button = gtk_button_new_with_mnemonic("_Hide") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(hide_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Set visibility of all selected tracks to Hide") ;

  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(" "), FALSE, FALSE, 0) ; //separator

  /* Add buttons to bump/unbump all selected columns */
  button = gtk_button_new_with_mnemonic("_Bump") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(bump_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Bump (expand) all selected tracks") ;

  button = gtk_button_new_with_mnemonic("_Unbump") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(unbump_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Unbump (collapse) all selected tracks") ;

  gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(" "), FALSE, FALSE, 0) ; //separator

  /* Add a button to set a group for the selected columns */
  button = gtk_button_new_with_mnemonic("Change _Group") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(set_group_button_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Assign or clear the group for the selected tracks") ;

  /* Add a button to choose a style for the selected columns */
  button = gtk_button_new_with_mnemonic("Change _Style") ;
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(choose_style_button_cb), page_data) ;
  gtk_widget_set_tooltip_text(button, "Edit the style and/or assign a new style for the selected tracks") ;

  return vbox ;
}


static void loaded_cols_panel(LoadedPageData page_data, FooCanvasGroup *column_group)
{
  zMapReturnIfFail(page_data && page_data->configure_data) ;

  GtkWidget *cols_panel = NULL ;
  ZMapWindowColConfigureMode configure_mode = page_data->configure_data->mode ;

  /* If configured only to show one column, set the column group we should show */
  FooCanvasGroup *required_column_group = NULL ;

  if (configure_mode == ZMAPWINDOWCOLUMN_CONFIGURE && column_group)
    required_column_group = column_group ;

  /* Create the overall container */
  cols_panel = gtk_vbox_new(FALSE, YPAD) ;
  gtk_container_add(GTK_CONTAINER(page_data->page_container), cols_panel) ;

  /* Add a short key to the abbreviations */
  GtkWidget *label = gtk_label_new("FS/FA/FH => Forward strand Show/Auto/Hide   RS/RA/RH => Reverse strand Show/Auto/Hide") ;
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.0) ;
  gtk_box_pack_start(GTK_BOX(cols_panel), label, FALSE, FALSE, YPAD) ;

  /* Create search/filter buttons */
  GtkWidget *buttons_container = loaded_cols_panel_create_buttons(page_data) ;
  gtk_box_pack_start(GTK_BOX(cols_panel), buttons_container, FALSE, FALSE, YPAD) ;

  /* Create a scrolled window for our tree */
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(cols_panel), scrolled, TRUE, TRUE, YPAD) ;

  /* Create the tree view that will list the columns */
  GtkTreeModel *model = loaded_cols_panel_create_tree_model(page_data, required_column_group) ;
  GtkWidget *tree = loaded_cols_panel_create_tree_view(page_data, model) ;
  gtk_container_add(GTK_CONTAINER(scrolled), tree) ;
  
  /* Cache pointers to the widgets. */
  page_data->cols_panel = cols_panel ;

  /* Show the columns panel */
  gtk_widget_show_all(cols_panel) ;
}

static char *label_text_from_column(FooCanvasGroup *column_group)
{
  GQuark display_id;
  char *label_text;

  /* Get hold of the style. */
  display_id = zmapWindowContainerFeaturesetGetColumnId((ZMapWindowContainerFeatureSet)column_group) ;

  label_text = (char *)(g_quark_to_string(display_id));

  return label_text;
}

/* Quick function to create an aligned label with the name of the style.
 * Label has a destroy callback for the button_data!
 */
static GtkWidget *create_label(GtkWidget *parent, const char *text)
{
  GtkWidget *label;

  /* show the column name. */
  label = gtk_label_new(text);

  /* x, y alignments between 0.0 (left, top) and 1.0 (right, bottom) [0.5 == centered] */
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);

  if (parent)
    gtk_box_pack_start(GTK_BOX(parent), label, TRUE, TRUE, 0) ;

  return label;
}


static GtkWidget *create_revert_apply_button(ColConfigure configure_data)
{
  GtkWidget *button_box, *ok_button, *reorder_button, *apply_button,
    *cancel_button, *revert_button, *frame, *parent;

  parent = configure_data->vbox;

  button_box = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(button_box), ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (button_box), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  revert_button = gtk_button_new_from_stock(GTK_STOCK_REFRESH) ;
  gtk_box_pack_end(GTK_BOX(button_box), revert_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(revert_button), "clicked", GTK_SIGNAL_FUNC(revert_button_cb), configure_data) ;
  gtk_widget_set_tooltip_text(revert_button, "Revert unsaved changes and/or re-load data (required to view new columns that have been loaded in the background)") ;

  /* Add a button to apply the new column order. We want them to explicitly choose to reorder
   * rather than doing this when they hit the Apply button because they might have reordered just
   * to be able to view columns better. */
  reorder_button = gtk_button_new_with_mnemonic("Apply _Order") ;
  gtk_box_pack_end(GTK_BOX(button_box), reorder_button, FALSE, FALSE, 0) ;
  g_signal_connect(G_OBJECT(reorder_button), "clicked", G_CALLBACK(reorder_button_cb), configure_data) ;
  gtk_widget_set_tooltip_text(reorder_button, "Applies the new track order based on the order of the rows in the list. (Drag and drop rows to reorder the list, or click column headers to sort.)") ;

  apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY) ;
  gtk_box_pack_end(GTK_BOX(button_box), apply_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(apply_button), "clicked", GTK_SIGNAL_FUNC(apply_button_cb), configure_data) ;
  gtk_widget_set_tooltip_text(apply_button, "Save and apply changes (can't be undone)") ;

  cancel_button = gtk_button_new_from_stock(GTK_STOCK_CANCEL) ;
  gtk_box_pack_end(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked", GTK_SIGNAL_FUNC(cancel_button_cb), configure_data) ;
  gtk_widget_set_tooltip_text(cancel_button, "Close and lose unsaved changes") ;

  ok_button = gtk_button_new_from_stock(GTK_STOCK_OK) ;
  gtk_box_pack_end(GTK_BOX(button_box), ok_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(ok_button), "clicked", GTK_SIGNAL_FUNC(ok_button_cb), configure_data) ;
  gtk_widget_set_tooltip_text(ok_button, "Save and apply changes (can't be undone)") ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame), ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0) ;

  gtk_container_add(GTK_CONTAINER(frame), button_box);


  /* set apply button as default. */
  GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT) ;

  return apply_button;
}

/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  const char *title = "Column Configuration Window" ;
  const char *help_text =
    "The ZMap Column Configuration Window allows you to change various Column-specific settings.\n"
    "\n"
    "Select one or more rows in the list of Columns. Then you can change the visibility by\n"
    "selecting S/A/H (Show/Auto/Hide - see the Help->Display menu for more information) or you can\n"
    "assign a new style for the Columns by pressing the Choose Style button.\n"
    "\n"
    "You can navigate up/down the list with the up/down arrows on your keyboard. Hold down shift\n"
    "or control to select mutliple rows.\n"
    "\n"
    "Enter a search term in the Search box to jump to rows containing that text in the Column name.\n"
    "Press the down/up arrows while the focus is in the Search box to jump to the next/previous match.\n"
    "Enter a search term in the Filter box to filter the rows to Column names containing that text.\n"
    "Press Enter to perform the filter. Press the Clear button to clear the Search/Filter boxes.\n"
    "\n"
    "Drag and drop rows to reorder the Columns manually. Click on the column headers to sort the columns\n"
    "automatically. Click Clear to disable automatic sorting so that you can drag-and-drop rows manually\n"
    "again (note that this will not revert the order, it just enables manual ordering based on the new order).\n"
    "Note that you must click the APPLY COLUMN ORDER button to apply the new order (the main Apply button WILL\n"
    "NOT reorder columns).\n"
    "\n"
    "Click Apply when finished to save and apply the changes you have made (other than column order - see above).\n"
    "If there is no Apply button, changes will be applied immediately. If you want to reset the dialog to the\n"
    "last-saved values, click Revert. If you want to exit without saving the latest changes, click Close.\n"
    "\n" ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}

/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpDisplayCB(gpointer data, guint callback_action, GtkWidget *w)
{
  const char *title = "Column Configuration - Display States" ;
  const char *help_text =
    "The Show/Auto/Hide buttons allow you to change which columns are visible in ZMap.\n"
    "\n"
    "The window displays separate options for the forward and reverse strands. You can set\n"
    "the display state for each strand to one of:\n"
    "\n"
    "\"" SHOW_LABEL "\"  - always show the column\n"
    "\n"
    "\"" SHOWHIDE_LABEL "\"  - show the column depending on ZMap settings (e.g. zoom, compress etc.)\n"
    "\n"
    "\"" HIDE_LABEL "\"  - always hide the column\n"
    "\n"
    "These options are abbreviated to S/A/H respectively in the headers and are preceeded by F/R\n"
    "for the Forward/Reverse strand, i.e. 'RH' means 'Reverse strand - Hide'.\n"
    "\n"
    "By default column display is controlled by settings in the Style for that column,\n"
    "including the min and max zoom levels at which the column should be shown, how the\n"
    "the column is bumped, the window mark and compress options. Column display can be\n"
    "overridden however to always show or always hide columns.\n"
    "\n" ;

  zMapGUIShowText(title, help_text, FALSE) ;

  return ;
}



/* requests window destroy, ends up with gtk calling destroyCB(). */
static void requestDestroyCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  gtk_widget_destroy(configure_data->window->col_config_window) ;

  return ;
}


static void destroyCB(GtkWidget *widget, gpointer cb_data)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;

  configure_data->window->col_config_window = NULL ;

  g_free(configure_data) ;

  return ;
}


/* Save the changed settings to the config file */
static void saveCB(gpointer cb_data, guint callback_action, GtkWidget *widget)
{
  ColConfigure configure_data = (ColConfigure)cb_data ;
  zMapReturnIfFail(configure_data && configure_data->window) ;

  static char *filename = NULL ;

  saveColumnsConfig(configure_data, filename) ;

  return ;
}


/* For the given row and strand, apply the updates to the appropriate column */
static void loaded_page_apply_visibility_tree_row(LoadedPageData loaded_page_data, ZMapStrand strand, 
                                                  GtkTreeModel *model, GtkTreeIter *iter)
{
  zMapReturnIfFail(loaded_page_data && 
                   loaded_page_data->configure_data &&
                   loaded_page_data->configure_data->window) ;

  ColConfigure configure_data = loaded_page_data->configure_data ;
  
  /* Get the column that's in this tree row */
  GQuark column_id = tree_model_get_column_id(model, iter, TRUE) ;
  FooCanvasGroup *column_group = zmapWindowGetColumnByID(loaded_page_data->window, strand, column_id) ;

  /* Get the required state of this tree row according to the toggle value in the tree model */
  ZMapStyleColumnDisplayState show_hide_state = get_state_from_tree_store_value(model, iter, strand) ;

  /* We only do this if there is no apply button.  */
  if(column_group && loaded_page_data->apply_now)
    {
      ZMapWindow window = configure_data->window;

      zmapWindowColumnSetState(window,
                               column_group,
                               show_hide_state,
                               loaded_page_data->reposition) ;

      if (IS_3FRAME(window->display_3_frame))
        {
          ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)(column_group);;

          GList *feature_sets = NULL ;
          GList *featureset_ids = NULL ;

          if (container)
            featureset_ids = zmapWindowContainerFeatureSetGetFeatureSets(container) ;
      
          if (featureset_ids)
            feature_sets = getFeaturesetStructsFromIDs(featureset_ids, loaded_page_data->window->feature_context) ;

          for (GList *item = feature_sets; item; item = item->next)
            {
              ZMapFeatureSet feature_set = (ZMapFeatureSet)(item->data) ;

              /* MH17: we need to find the 3frame columns from a given column id
               * Although we are only looking at one featureset out of possibly many
               * each one will have the same featureset attatched
               * so this will work. Phew!
               *
               * but only if the featureset is in the hash for that frame and strand combo.
               * Hmmm... this was always the case, but all the features were in one context featureset
               *
               */

              for(int i = ZMAPFRAME_NONE ; i <= ZMAPFRAME_2; i++)
                {
                  FooCanvasItem *frame_column;
                  ZMapFrame frame = (ZMapFrame)i;

                  frame_column = zmapWindowFToIFindSetItem(window, window->context_to_item,feature_set,
                                                           zmapWindowContainerFeatureSetGetStrand(container), frame);

                  if(frame_column &&
                     ZMAP_IS_CONTAINER_GROUP(frame_column) &&
                     (zmapWindowContainerHasFeatures(ZMAP_CONTAINER_GROUP(frame_column)) ||
                      zmapWindowContainerFeatureSetShowWhenEmpty(ZMAP_CONTAINER_FEATURESET(frame_column))))
                    {
                      zmapWindowColumnSetState(window,
                                               FOO_CANVAS_GROUP(frame_column),
                                               show_hide_state,
                                               FALSE) ;
                    }
                }

              if(loaded_page_data->reposition)
                zmapWindowFullReposition(window->feature_root_group,TRUE,"show button");
            }
          
          if (feature_sets)
            g_list_free(feature_sets) ;
        }
    }
}



/* Get the column name for the given row in the given tree. If 'unique' is true returns the
 * unique id, otherwise the original id */
static GQuark tree_model_get_column_id(GtkTreeModel *model, GtkTreeIter *iter, const gboolean unique)
{
  GQuark column_id = 0 ;
  char *column_name = NULL ;

  gtk_tree_model_get (model, iter,
                      NAME_COLUMN, &column_name,
                      -1);

  if (column_name)
    {
      column_id = zMapStyleCreateID(column_name) ;
      g_free(column_name) ;
    }

  return column_id ;
}


/* Get the style name(s) for the given row in the given tree. Returns a list of semi-colon
 * separated values */
static char* tree_model_get_style_ids(GtkTreeModel *model, GtkTreeIter *iter)
{
  char *style_names = NULL ;

  gtk_tree_model_get (model, iter,
                      STYLE_COLUMN, &style_names,
                      -1);

  return style_names ;
}


/* Get the group id for the given row in the given tree */
static GQuark tree_model_get_group_id(GtkTreeModel *model, GtkTreeIter *iter)
{
  GQuark group_id = 0 ;
  char *group_name = NULL ;

  gtk_tree_model_get (model, iter,
                      GROUP_COLUMN, &group_name,
                      -1);

  if (group_name)
    {
      group_id = zMapStyleCreateID(group_name) ;
      g_free(group_name) ;
    }

  return group_id ;
}


/* Get the bumped status for the given row in the given tree (returns true if bumped, false if
 * not) */
static bool tree_model_get_bumped(GtkTreeModel *model, GtkTreeIter *iter)
{
  bool bumped = false ;
  char *bumped_text = NULL ;

  gtk_tree_model_get (model, iter,
                      BUMPED_COLUMN, &bumped_text,
                      -1);

  if (bumped_text && bumped_text[0] != '\0')
    {
      bumped = (strcmp(bumped_text, BUMPED_TEXT) == 0) ;
    }

  return bumped ;
}


/* Convert a list of featureset ids to a list of ZMapFeatureSet structs. Returns a newly
 * allocated GList. */
static GList* getFeaturesetStructsFromIDs(GList *featureset_ids, ZMapFeatureContext context)
{
  GList *result = NULL ;
  zMapReturnValIfFail(context, result) ;

  /* Convert the list of ids into list of featureset structs */
  for (GList *item = featureset_ids; item; item = item->next)
    {
      GQuark featureset_id = GPOINTER_TO_INT(item->data) ;
      ZMapFeatureSet feature_set = zmapFeatureContextGetFeaturesetFromId(context, featureset_id) ;

      if (feature_set)
        result = g_list_append(result, feature_set) ;
      else
        zMapLogWarning("Failed to find featureset struct for '%s'", g_quark_to_string(featureset_id)) ;
    }

  return result ;
}


/* Get the featureset structs for the given row in the given tree. Returns null if not found */
static GList* tree_model_get_column_featuresets(LoadedPageData page_data, GtkTreeModel *model, GtkTreeIter *iter)
{
  GList *result = NULL ;
  zMapReturnValIfFail(page_data && page_data->window, result) ;

  GQuark column_id = tree_model_get_column_id(model, iter, TRUE) ;

  FooCanvasGroup *column_group = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_FORWARD, column_id) ;

  if (!column_group)
    column_group = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_REVERSE, column_id) ;

  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)(column_group);

  if (container)
    {
      GList *featureset_ids = zmapWindowContainerFeatureSetGetFeatureSets(container);
      
      if (featureset_ids)
        result = getFeaturesetStructsFromIDs(featureset_ids, page_data->window->feature_context) ;
    }

  return result ;
}


/* Called for each row in the current selection to set the list of selected featuresets */
static void get_selected_featuresets_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
  GetFeaturesetsData data = (GetFeaturesetsData)user_data ;
  
  GList *featuresets = tree_model_get_column_featuresets(data->page_data, model, iter) ;

  for (GList *item = featuresets; item; item = item->next)
    {
      data->feature_sets = g_list_append(data->feature_sets, item->data) ;
    }
}


/* Get the list of selected featuresets in the tree view. Result is a list of ZMapFeatureSet
 * structs or NULL if there are none. */
static GList* tree_view_get_selected_featuresets(LoadedPageData page_data)
{
  zMapReturnValIfFail(page_data && 
                      page_data->tree_view &&
                      page_data->window &&
                      page_data->window->feature_context, 
                      NULL) ;

  GtkTreeSelection *selection = gtk_tree_view_get_selection(page_data->tree_view) ;
  
  GetFeaturesetsDataStruct data = {page_data, NULL} ;

  gtk_tree_selection_selected_foreach(selection, &get_selected_featuresets_cb, &data) ;

  return data.feature_sets ;
}


/* Set the visibility in the given row and apply it, based on the given state (in the user data) */
static void set_and_apply_state(GtkTreeModel *model_in, 
                                GtkTreePath *path, 
                                SetTreeRowStateData set_state_data)
{
  GtkTreeIter iter ;
  GtkTreeModel *model = model_in ;

  treeGetIter(path, model_in, &model, &iter) ;

  /* Set the state of the cells */
  gboolean forward = (set_state_data->strand == ZMAPSTRAND_FORWARD) ;
  set_tree_store_value_from_state(set_state_data->state, GTK_LIST_STORE(model), &iter, forward) ;

  /* Apply the new states. */
  loaded_page_apply_visibility_tree_row(set_state_data->loaded_page_data, set_state_data->strand, model, &iter) ;
}


/* Called when user selects one of the display state radio buttons. */
static void loaded_column_visibility_toggled_cb(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data)
{
  ShowHideData button_data = (ShowHideData)user_data ;
  zMapReturnIfFail(path_string && 
                   button_data && 
                   button_data->loaded_page_data->tree_model && 
                   button_data->loaded_page_data && 
                   button_data->loaded_page_data->configure_data) ;

  gboolean ok = TRUE ;
  GtkTreeModel *model = gtk_tree_view_get_model(button_data->loaded_page_data->tree_view) ;
  LoadedPageData loaded_page_data = button_data->loaded_page_data;
  ZMapStyleColumnDisplayState show_hide_state ;

  /* Set the flag to indicate that the user clicked on a toggle button column */
  loaded_page_data->clicked_button = TRUE ;

  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);

  if (!path)
    {
      ok = FALSE ;
      zMapLogWarning("Error getting tree path '%s'", path_string) ;
    }

  if (ok)
    {
      ok = gtk_tree_model_get_iter (model, &iter, path);
      gtk_tree_path_free (path);
    }

  if (ok)
    {
      /* Only toggle the button if the clicked row is in the selection */
      GtkTreeSelection *selection = gtk_tree_view_get_selection(loaded_page_data->tree_view) ;

      if (!gtk_tree_selection_iter_is_selected(selection, &iter))
        ok = FALSE ;
    }

  if (ok)
    {
      show_hide_state = get_state_from_tree_store_column(button_data->tree_col_id) ;
    }

  if (ok)
    {
      /* Do this for all tree rows in the current selection. */
      GtkTreeSelection *selection = gtk_tree_view_get_selection(loaded_page_data->tree_view) ;
      SetTreeRowStateDataStruct set_state_data = {loaded_page_data, button_data->strand, show_hide_state} ;

      GtkTreeModel *model = NULL ;
      GList *rows = gtk_tree_selection_get_selected_rows(selection, &model) ;
          
      for (GList *item = rows; item; item = item->next)
        {
          GtkTreePath *path = (GtkTreePath*)(item->data) ;
          set_and_apply_state(model, path, &set_state_data) ;
        }
    }

  return ;
}


static void notebook_foreach_page(GtkWidget       *notebook_widget,
                                  gboolean         current_page_only,
                                  NotebookFuncType func_type,
                                  gpointer         foreach_data)
{
  GtkNotebook *notebook = GTK_NOTEBOOK(notebook_widget);
  int pages = 0, i, current_page;

  pages = gtk_notebook_get_n_pages(notebook);

  if(current_page_only)
    current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook_widget));

  for(i = 0; i < pages; i++)
    {
      NotebookPage page_data;
      GtkWidget *page;

      page = gtk_notebook_get_nth_page(notebook, i);

      if(current_page_only && current_page != i)
        continue;

      if((page_data = (NotebookPage)g_object_get_data(G_OBJECT(page), NOTEBOOK_PAGE_DATA)))
        {
          switch(func_type)
            {
            case NOTEBOOK_APPLY_FUNC:
              {
                NotebookPageFunc apply_func;

                if((apply_func = page_data->page_apply))
                  {
                    (apply_func)(page_data);
                  }
              }
              break;
            case NOTEBOOK_REORDER_FUNC:
              {
                NotebookPageFunc reorder_func;

                if((reorder_func = page_data->page_reorder))
                  {
                    (reorder_func)(page_data);
                  }
              }
              break;
            case NOTEBOOK_POPULATE_FUNC:
              {
                NotebookPageColFunc pop_func;

                if((pop_func = page_data->page_populate))
                  {
                    (pop_func)(page_data, FOO_CANVAS_GROUP(foreach_data));
                  }
              }
              break;
            case NOTEBOOK_UPDATE_FUNC:
              {
                NotebookPageUpdateFunc pop_func;

                if((pop_func = page_data->page_update))
                  {
                    (pop_func)(page_data, (ChangeButtonStateData)foreach_data) ;
                  }
              }
              break;
            case NOTEBOOK_CLEAR_FUNC:
              {
                NotebookPageFunc clear_func;

                if((clear_func = page_data->page_clear))
                  {
                    (clear_func)(page_data);
                  }
              }
              break;
            default:
              break;
            }
        }
    }

  return ;
}


static void cancel_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  gtk_widget_destroy(configure_data->window->col_config_window) ;

  return ;
}

static void revert_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  zmapWindowColumnConfigure(configure_data->window, NULL, configure_data->mode);

  return ;
}

static void reorder_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_REORDER_FUNC, NULL);

  return ;
}

static void apply_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_APPLY_FUNC, NULL);

  return ;
}

static void ok_button_cb(GtkWidget *ok_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_APPLY_FUNC, NULL);

  gtk_widget_destroy(configure_data->window->col_config_window) ;

  return ;
}




/* *********************************************************
 * * Code to do the resizing of the widget, according to   *
 * * available space, and to add scrollbar when required   *
 * *********************************************************
 */

/* Entry point is zmapAddSizingSignalHandlers() */


/* Called by widget_size_request() */

static void handle_scrollbars(GtkWidget      *widget,
                              GtkRequisition *requisition_inout)
{
  GtkWidget *parent;

  if((parent = gtk_widget_get_parent(widget)) &&
     (GTK_IS_SCROLLED_WINDOW(parent)))
    {
      GtkWidget *scrollbar;

      if((scrollbar = gtk_scrolled_window_get_vscrollbar(GTK_SCROLLED_WINDOW(parent))))
        {
          GtkRequisition v_requisition;
          int scrollbar_spacing = 0;

          gtk_widget_size_request(scrollbar, &v_requisition);

          gtk_widget_style_get(parent,
                               "scrollbar-spacing", &scrollbar_spacing,
                               NULL);

          v_requisition.width      += scrollbar_spacing;
          requisition_inout->width += v_requisition.width;
        }

      if((scrollbar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(parent))))
        {
          GtkRequisition h_requisition;
          int scrollbar_spacing = 0;

          gtk_widget_size_request(scrollbar, &h_requisition);

          gtk_widget_style_get(parent,
                               "scrollbar-spacing", &scrollbar_spacing,
                               NULL);

          h_requisition.height      += scrollbar_spacing;
          requisition_inout->height += h_requisition.height;
        }
    }

  return ;
}

/* Called by widget_size_request() */

static void restrict_screen_size_to_toplevel(GtkWidget      *widget,
                                             GtkRequisition *screen_size)
{
  GtkWidget *toplevel = zMapGUIFindTopLevel( widget );
  int widget_dx, widget_dy;

  widget_dx = toplevel->allocation.width  - widget->allocation.width;
  widget_dy = toplevel->allocation.height - widget->allocation.height;

  screen_size->width  -= widget_dx;
  screen_size->height -= widget_dy;

  return ;
}

/* Called by widget_size_request() */

static void get_screen_size(GtkWidget      *widget,
                            double          screen_percentage,
                            GtkRequisition *requisition_inout)
{
  GdkScreen *screen;
  gboolean has_screen;

  if((has_screen = gtk_widget_has_screen(widget)) &&
     (screen     = gtk_widget_get_screen(widget)) &&
     (requisition_inout))
    {
      int screen_width, screen_height;

      screen_width   = gdk_screen_get_width(screen);
      screen_height  = gdk_screen_get_height(screen);

      screen_width  *= screen_percentage;
      screen_height *= screen_percentage;

      requisition_inout->width  = screen_width;
      requisition_inout->height = screen_height;

#ifdef DEBUG_SCREEN_SIZE
      requisition_inout->width  = 600;
      requisition_inout->height = 400;
#endif
    }

  return ;
}


static void widget_size_request(GtkWidget      *widget,
                                GtkRequisition *requisition_inout,
                                gpointer        user_data)
{
  SizingData sizing_data = (SizingData)user_data;
  GtkRequisition screen_requisition = {-1, -1};

  gtk_widget_size_request(widget, requisition_inout);

  handle_scrollbars(widget, requisition_inout);

  if(!sizing_data->toplevel)
    sizing_data->toplevel = zMapGUIFindTopLevel(widget);

  get_screen_size(sizing_data->toplevel, 0.85, &screen_requisition);

  restrict_screen_size_to_toplevel(widget, &screen_requisition);

  requisition_inout->width  = MIN(requisition_inout->width,  screen_requisition.width);
  requisition_inout->height = MIN(requisition_inout->height, screen_requisition.height);

  requisition_inout->width  = MAX(requisition_inout->width,  -1);
  requisition_inout->height = MAX(requisition_inout->height, -1);

  return ;
}

/*
 * The following 4 functions are to modify the sizing behaviour of the
 * GtkTreeView Widget.  By default it uses a very small size and
 * although it's possible to resize it, this is not useful for the
 * users who would definitely prefer _not_ to have to do this for
 * _every_ window that is opened.
 */
static void  zmap_widget_size_request(GtkWidget      *widget,
                                      GtkRequisition *requisition,
                                      gpointer        user_data)
{

  widget_size_request(widget, requisition, user_data);

  return ;
}

static gboolean zmap_widget_map(GtkWidget      *widget,
                                gpointer        user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  sizing_data->widget_requisition = widget->requisition;

  widget_size_request(widget, &(sizing_data->widget_requisition), user_data);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    zMapLogWarning("map: %d x %d", sizing_data->widget_requisition.width, sizing_data->widget_requisition.height);
#endif /* DEBUGGING_ONLY */

  gtk_widget_set_size_request(widget,
                              sizing_data->widget_requisition.width,
                              sizing_data->widget_requisition.height);

  return FALSE;
}

static gboolean zmap_widget_unmap(GtkWidget *widget,
                                  gpointer   user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  gtk_widget_set_size_request(widget,
                              sizing_data->user_requisition.width,
                              sizing_data->user_requisition.height);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    zMapLogWarning("unmap: %d x %d", widget->requisition.width, widget->requisition.height);
#endif /* DEBUGGING_ONLY */

  return FALSE;
}

static void zmap_widget_size_allocation(GtkWidget     *widget,
                                        GtkAllocation *allocation,
                                        gpointer       user_data)
{
  SizingData sizing_data = (SizingData)user_data;

  gtk_widget_set_size_request(widget,
                              sizing_data->user_requisition.width,
                              sizing_data->user_requisition.height);

#ifdef DEBUGGING_ONLY
  if(sizing_data->debug)
    zMapLogWarning("alloc: %d x %d", allocation->width, allocation->height);
#endif /* DEBUGGING_ONLY */

  return ;
}

static gboolean zmapAddSizingSignalHandlers(GtkWidget *widget, gboolean debug,
                                            int width, int height)
{
  SizingData sizing_data = g_new0(SizingDataStruct, 1);

  sizing_data->debug  = debug;

  sizing_data->user_requisition.width  = width;
  sizing_data->user_requisition.height = height;

  g_signal_connect(G_OBJECT(widget), "size-request",
                   G_CALLBACK(zmap_widget_size_request), sizing_data);

  g_signal_connect(G_OBJECT(widget), "map",
                   G_CALLBACK(zmap_widget_map), sizing_data);

  g_signal_connect(G_OBJECT(widget), "size-allocate",
                   G_CALLBACK(zmap_widget_size_allocation), sizing_data);

  g_signal_connect(G_OBJECT(widget), "unmap",
                   G_CALLBACK(zmap_widget_unmap), sizing_data);


  return TRUE;
}

/* Apply changes and save the columns configuration to the user prefs */
static void saveColumnsConfig(ColConfigure configure_data, const char *filename)
{
  zMapReturnIfFail(configure_data && configure_data->window) ;

  ZMapWindow window = configure_data->window ; 

  ZMapConfigIniFileType file_type = ZMAPCONFIG_FILE_PREFS ;

  ZMapConfigIniContext context = zMapConfigIniContextProvide(filename, file_type) ;
  
  if (context)
    {
      if (filename)
        zMapConfigIniContextSetFile(context, file_type, filename) ;

      zMapConfigIniContextCreateKeyFile(context, file_type) ;

      zMapFeatureUpdateContext(window->context_map,
                               window->sequence,
                               (ZMapFeatureAny)window->feature_context,
                               context, 
                               file_type) ;

      zMapConfigIniContextSetUnsavedChanges(context, file_type, TRUE) ;
      zMapConfigIniContextSave(context, file_type) ;

      zMapConfigIniContextDestroy(context) ;
      context = NULL ;
    }
  else
    {
      zMapCritical("%s", "Program error: failed to create context") ;
    }
}
