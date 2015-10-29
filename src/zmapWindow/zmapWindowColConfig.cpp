/*  File: zmapWindowColConfig.c
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
 * Description: Functions to implement column configuration.
 *
 * Exported functions: See ZMap/zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapWindow_P.hpp>
#include <zmapWindowContainers.hpp>
#include <gbtools/gbtools.hpp>


/* Labels for column state, used in code and the help page. */
#define SHOW_LABEL     "Show"
#define SHOWHIDE_LABEL "Auto"
#define HIDE_LABEL     "Hide"
#define UP_LABEL       "Up"
#define DOWN_LABEL     "Down"

#define XPAD 4
#define YPAD 0

#define NOTEBOOK_PAGE_DATA "notebook_page_data"

typedef struct _ColConfigureStruct *ColConfigure;

#define CONFIGURE_DATA "col_data"

typedef enum
  {
    NAME_COLUMN, 
    SHOW_FWD_COLUMN, 
    AUTO_FWD_COLUMN, 
    HIDE_FWD_COLUMN, 
    SHOW_REV_COLUMN, 
    AUTO_REV_COLUMN, 
    HIDE_REV_COLUMN,

    N_COLUMNS
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

  unsigned int has_apply_button : 1 ;

  gboolean columns_changed ;

} ColConfigureStruct;


typedef struct _NotebookPageStruct *NotebookPage ;

typedef enum
  {
    NOTEBOOK_INVALID_FUNC,
    NOTEBOOK_POPULATE_FUNC,
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
  GtkTreeModel *tree_model ;

  ColConfigure configure_data ;

  /* Lists of all ShowHideButton pointers */
  GList *show_list_fwd;
  GList *default_list_fwd;
  GList *hide_list_fwd;
  GList *show_list_rev;
  GList *default_list_rev;
  GList *hide_list_rev;

  /* Fwd and rev strand lists of UpDownButton pointers. */
  GList *up_list;
  GList *down_list;

  /* This list maintains the current order of the columns on the dialog */
  GList *columns_list ;

  unsigned int apply_now        : 1 ;
  unsigned int reposition       : 1 ;

} LoadedPageDataStruct;


typedef struct
{
  LoadedPageData              loaded_page_data;
  ZMapStyleColumnDisplayState show_hide_state;
  FooCanvasGroup             *show_hide_column;
  GtkWidget                  *show_hide_button;
} ShowHideButtonStruct, *ShowHideButton;


typedef struct
{
  LoadedPageData loaded_page_data ;
  DialogColumns tree_col_id ;
  ZMapStrand strand ;
} ShowHideDataStruct, *ShowHideData;


typedef struct
{
  GQuark column_id ;         // the id of the column this button will move
  int direction ;            // -1 to move up, 1 to move down

  LoadedPageData page_data ;
  GList *columns_list_iter ; // the iter into columns_list for this column
} UpDownButtonStruct, *UpDownButton;



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




static GtkWidget *make_menu_bar(ColConfigure configure_data);
static void loaded_cols_panel(LoadedPageData loaded_page_data) ;
static char *label_text_from_column(FooCanvasGroup *column_group);
static GtkWidget *create_label(GtkWidget *parent, const char *text);
static GtkWidget *create_revert_apply_button(ColConfigure configure_data);

static GtkWidget *make_scrollable_vbox(GtkWidget *vbox);
static ColConfigure configure_create(ZMapWindow window, ZMapWindowColConfigureMode configure_mode);

static void requestDestroyCB(gpointer data, guint callback_action, GtkWidget *widget) ;
static void destroyCB(GtkWidget *widget, gpointer cb_data) ;

static void helpCB(gpointer data, guint callback_action, GtkWidget *w) ;
static void loaded_show_button_cb(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data) ;

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

static void select_all_buttons(GtkWidget *button, gpointer user_data);

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
static void activate_matching_column(gpointer list_data, gpointer user_data) ;

static void loaded_page_apply_tree_row(LoadedPageData loaded_page_data, ZMapStrand strand, 
                                       GtkTreeIter *iter, ZMapStyleColumnDisplayState show_hide_state) ;

static GQuark tree_model_get_column_id(GtkTreeModel *model, GtkTreeIter *iter) ;

static ZMapStyleColumnDisplayState get_state_from_tree_store_value(GtkTreeModel *model, 
                                                                   GtkTreeIter *iter, 
                                                                   const gboolean forward) ;



static GtkItemFactoryEntry menu_items_G[] =
  {
    { (gchar *)"/_File",           NULL,          NULL,             0, (gchar *)"<Branch>",      NULL},
    { (gchar *)"/File/Close",      (gchar *)"<control>W", (GtkItemFactoryCallback)requestDestroyCB, 0, NULL,            NULL},
    { (gchar *)"/_Help",           NULL,          NULL,             0, (gchar *)"<LastBranch>",  NULL},
    { (gchar *)"/Help/General",    NULL,          (GtkItemFactoryCallback)helpCB,           0, NULL,            NULL}
  };



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

      /* Calculate a sensible initial height - say 3/4 of screen height */
      int height = 0 ;
      if (gbtools::GUIGetTrueMonitorSize(toplevel, NULL, &height))
        {
          gtk_window_set_default_size(GTK_WINDOW(toplevel), -1, height * 0.75) ;
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

  loaded_cols_panel(loaded_page_data) ;

  if (loaded_page_data->show_list_fwd || loaded_page_data->show_list_rev)
    {
      configure_data->has_apply_button = TRUE;
    }
  else
    {
      loaded_page_data->apply_now  = TRUE;
      loaded_page_data->reposition = TRUE;
    }

  return ;
}


/* This emits a "toggled" signal for all radio buttons in the given list that are currently active */
static void each_button_toggle_if_active(gpointer list_data, gpointer user_data)
{
  ShowHideButton button_data  = (ShowHideButton)list_data;

  GtkWidget *widget;

  widget = button_data->show_hide_button;

  if (GTK_IS_TOGGLE_BUTTON(widget))
    {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
        g_signal_emit_by_name(G_OBJECT(widget), "toggled");
    }


  return ;
}


/* Apply changes on the loaded-columns page */
static void loaded_page_apply(NotebookPage notebook_page)
{
  LoadedPageData loaded_page_data;
  ColConfigure configure_data;
  gboolean save_apply_now, save_reposition;
  ZMapWindow window ;

  configure_data = notebook_page->configure_data;
  loaded_page_data = (LoadedPageData)(notebook_page->page_data);
  window = loaded_page_data->window ;

  /* Reset the flags so that we apply changes now but don't reposition yet. Save the original
   * flag values first */
  save_apply_now  = loaded_page_data->apply_now;
  save_reposition = loaded_page_data->reposition;

  loaded_page_data->apply_now  = TRUE;
  loaded_page_data->reposition = FALSE;

  /* Update all of the columns based on the values in the tree store */
  GtkTreeIter iter ;
  GtkTreeModel *model = loaded_page_data->tree_model ;
  if (gtk_tree_model_get_iter_first(model, &iter))
    {
      do
        {
          ZMapStyleColumnDisplayState state_fwd = get_state_from_tree_store_value(model, &iter, TRUE) ;
          ZMapStyleColumnDisplayState state_rev = get_state_from_tree_store_value(model, &iter, FALSE) ;

          loaded_page_apply_tree_row(loaded_page_data, ZMAPSTRAND_FORWARD, &iter, state_fwd) ;
          loaded_page_apply_tree_row(loaded_page_data, ZMAPSTRAND_REVERSE, &iter, state_rev) ;
        } while (gtk_tree_model_iter_next(model, &iter)) ;
    }

  /* Apply the new column order */
  int i = 0 ;
  for (GList *item = loaded_page_data->columns_list ; item ; item = item->next, ++i)
    {
      ZMapFeatureColumn column = (ZMapFeatureColumn)(item->data) ;
      column->order = i ;
    }

  /* Need to set the feature_set_names list to the correct order. Not sure if we should include
   * all featuresets or just reorder the list that is there. For now, include all. */
  if (window->feature_set_names)
    g_list_free(window->feature_set_names) ;

  window->feature_set_names = zMapFeatureGetOrderedColumnsListIDs(window->context_map) ;

  zmapWindowBusy(loaded_page_data->window, FALSE) ;
  zmapWindowColOrderColumns(loaded_page_data->window);
  zmapWindowFullReposition(loaded_page_data->window->feature_root_group, TRUE, "move_button_cb") ;
  zmapWindowBusy(loaded_page_data->window, FALSE) ;

  /* Restore the original flag values */
  loaded_page_data->apply_now  = save_apply_now;
  loaded_page_data->reposition = save_reposition;

  /* Now reposition everything */
  zmapWindowFullReposition(configure_data->window->feature_root_group,TRUE, "show hide") ;

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
  g_list_foreach(page_data->show_list_fwd, (GFunc)g_free, NULL) ;
  g_list_free(page_data->show_list_fwd);
  page_data->show_list_fwd = NULL;

  g_list_foreach(page_data->default_list_fwd, (GFunc)g_free, NULL) ;
  g_list_free(page_data->default_list_fwd);
  page_data->default_list_fwd = NULL;

  g_list_foreach(page_data->hide_list_fwd, (GFunc)g_free, NULL);
  g_list_free(page_data->hide_list_fwd);
  page_data->hide_list_fwd = NULL;

  g_list_foreach(page_data->show_list_rev, (GFunc)g_free, NULL) ;
  g_list_free(page_data->show_list_rev);
  page_data->show_list_rev = NULL;

  g_list_foreach(page_data->default_list_rev, (GFunc)g_free, NULL) ;
  g_list_free(page_data->default_list_rev);
  page_data->default_list_rev = NULL;

  g_list_foreach(page_data->hide_list_rev, (GFunc)g_free, NULL);
  g_list_free(page_data->hide_list_rev);
  page_data->hide_list_rev = NULL;

  g_list_foreach(page_data->up_list, (GFunc)g_free, NULL);
  g_list_free(page_data->up_list);
  page_data->up_list = NULL;

  g_list_foreach(page_data->down_list, (GFunc)g_free, NULL);
  g_list_free(page_data->down_list);
  page_data->down_list = NULL;

  return ;
}


static void loaded_page_clear  (NotebookPage notebook_page)
{
  container_clear(notebook_page->page_container);

  LoadedPageData page_data = (LoadedPageData)(notebook_page->page_data) ;

  /* Clear the callback data pointers */
  loaded_page_clear_data(page_data) ;

  /* Clear any other data */
  g_list_free(page_data->columns_list) ;
  page_data->columns_list = NULL ;

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
                                                                   const gboolean forward)
{
  ZMapStyleColumnDisplayState result ;

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


/* Create enough radio buttons for the user to toggle between all the states of column visibility */
static void loaded_radio_buttons(GtkListStore *store, 
                                 GtkTreeIter *iter,
                                 const gboolean fwd,
                                 FooCanvasGroup *column_group,
                                 ShowHideButton *show_out,
                                 ShowHideButton *default_out,
                                 ShowHideButton *hide_out)
{
  GtkWidget *radio_show,
    *radio_maybe,
    *radio_hide;
  GtkRadioButton *radio_group_button;
  ShowHideButton show_data = NULL, default_data = NULL, hide_data = NULL;

  /* Create the "show" radio button, which will create the group too. */
  radio_show = gtk_radio_button_new(NULL) ;
  gtk_widget_set_tooltip_text(radio_show, SHOW_LABEL) ;

  /* Get the group so we can add the other buttons to the same group */
  radio_group_button = GTK_RADIO_BUTTON(radio_show);

  /* Create the "default" radio button */
  radio_maybe = gtk_radio_button_new_from_widget(radio_group_button) ;
  gtk_widget_set_tooltip_text(radio_maybe, SHOWHIDE_LABEL) ;

  /* Create the "hide" radio button */
  radio_hide = gtk_radio_button_new_from_widget(radio_group_button) ;
  gtk_widget_set_tooltip_text(radio_hide, HIDE_LABEL) ;

  /* Now create the data to attach to the buttons and connect the callbacks */

  /* Show */
  show_data = g_new0(ShowHideButtonStruct, 1);
  show_data->show_hide_button = radio_show;
  show_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_SHOW;
  show_data->show_hide_column = column_group;


  /* Default */
  default_data = g_new0(ShowHideButtonStruct, 1);
  default_data->show_hide_button = radio_maybe;
  default_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_SHOW_HIDE;
  default_data->show_hide_column = column_group;

  /* Hide */
  hide_data = g_new0(ShowHideButtonStruct, 1);
  hide_data->show_hide_button = radio_hide;
  hide_data->show_hide_state  = ZMAPSTYLE_COLDISPLAY_HIDE;
  hide_data->show_hide_column = column_group;

  /* Return the data... We need to add it to the list */
  if(show_out)
    *show_out = show_data;

  if(default_out)
    *default_out = default_data;

  if(hide_out)
    *hide_out = hide_data;

  return ;
}

static void loaded_page_update(NotebookPage notebook_page, ChangeButtonStateData cb_data)
{
  ColConfigure configure_data;
  ZMapStrand strand;
  GList *button_list_fwd = NULL ;
  GList *button_list_rev = NULL ;
  ZMapWindowColConfigureMode mode ;
  LoadedPageData loaded_page_data;


  /* First get the configure_data  */
  configure_data = notebook_page->configure_data;

  mode = cb_data->mode ;

  loaded_page_data = (LoadedPageData)(notebook_page->page_data) ;


  /* - Get the correct strand container for labels */
  strand = zmapWindowContainerFeatureSetGetStrand(ZMAP_CONTAINER_FEATURESET(cb_data->column_group)) ;
  if (strand != ZMAPSTRAND_FORWARD && strand != ZMAPSTRAND_REVERSE)
    return ;

  switch(mode)
    {
    case ZMAPWINDOWCOLUMN_HIDE:
      button_list_fwd = loaded_page_data->hide_list_fwd ;
      button_list_rev = loaded_page_data->hide_list_rev ;
      break;

    case ZMAPWINDOWCOLUMN_SHOW:
      button_list_fwd = loaded_page_data->show_list_fwd ;
      button_list_rev = loaded_page_data->show_list_rev ;
      break;

    default:
      button_list_fwd = loaded_page_data->default_list_fwd;
      button_list_rev = loaded_page_data->default_list_rev;
      break;
    }

  if (button_list_fwd || button_list_rev)
    {
      gboolean save_reposition, save_apply_now;

      save_apply_now  = loaded_page_data->apply_now;
      save_reposition = loaded_page_data->reposition;

      /* We're only aiming to find one button to toggle so reposition = TRUE is ok. */
      loaded_page_data->apply_now  = TRUE;
      loaded_page_data->reposition = TRUE;

      g_list_foreach(button_list_fwd, activate_matching_column, cb_data) ;
      g_list_foreach(button_list_rev, activate_matching_column, cb_data) ;

      loaded_page_data->apply_now  = save_apply_now;
      loaded_page_data->reposition = save_reposition;
    }


  return ;
}


static void activate_matching_column(gpointer list_data, gpointer user_data)
{
  ShowHideButton button_data = (ShowHideButton)list_data;
  ChangeButtonStateData cb_data = (ChangeButtonStateData)user_data ;

  if (!(cb_data->button_found) && button_data->show_hide_column == cb_data->column_group)
    {
      GtkWidget *widget;

      widget = button_data->show_hide_button ;

      if (GTK_IS_TOGGLE_BUTTON(widget))
        {
          if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
            {
              gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE) ;

              cb_data->button_found = TRUE ;
            }
        }
    }

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
gboolean column_is_loaded_in_range(ZMapFeatureContextMap map,
                                   ZMapFeatureBlock block, GQuark column_id, int start, int end)
{
#define MH17_DEBUG      0
  GList *fsets;

  fsets = zMapFeatureGetColumnFeatureSets(map, column_id, TRUE);

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
          GtkWidget *label, *button_box;
          //  FooCanvasGroup *column_group = FOO_CANVAS_GROUP(column->data);
          char *column_name;
          DeferredButton all, mark, none;
          gboolean loaded_in_mark = FALSE;
          gboolean force_mark = FALSE;
          GQuark col_id;

          column_name = (char *) g_quark_to_string(GPOINTER_TO_UINT(column->data));     //label_text_from_column(column_group);

          if(column_name && !g_list_find_custom(make_unique, column_name, find_name_cb))
            {
              label = create_label(label_box, column_name);

              button_box = gtk_hbox_new(FALSE, 0);

              gtk_box_pack_start(GTK_BOX(column_box), button_box, TRUE, TRUE, 0);


              if (mark_set)
                {
                  DeferredPageData page_data;
                  ZMapFeatureColumn col;

                  page_data   = (DeferredPageData)notebook_page->page_data;

                  col = (ZMapFeatureColumn)g_hash_table_lookup(window->context_map->columns,
                                                               GUINT_TO_POINTER(zMapFeatureSetCreateID(column_name)));

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




gint col_sort_by_name(gconstpointer a, gconstpointer b)
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
  GList *disp_cols = NULL;
  ZMapFeatureColumn column;

  window         = configure_data->window;


  /* get unloaded columns in a list
   * from window->context_map->columns hash table
   * sort into alpha order (not display order)
   * if we are doing 'configure this column' then we only include it
   * if not fully loaded and also include no others
   */
  zMap_g_hash_table_get_data(&disp_cols,window->context_map->columns);

  while(disp_cols)
    {
      column = (ZMapFeatureColumn) disp_cols->data;

      if((!column_name || column_name == column->unique_id )
         && (column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME))
         && (column->column_id != g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME))
         && !column_is_loaded_in_range(window->context_map,block,
                                       column->unique_id, window->sequence->start, window->sequence->end)
         && !zMapFeatureIsSeqColumn(window->context_map,column->unique_id)
         )
        {
          columns = g_list_prepend(columns,GUINT_TO_POINTER(column->column_id));
        }

      disp_cols = g_list_delete_link(disp_cols,disp_cols);
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

static void create_select_all_button(GtkTable *table, const int row, const int col,
                                     GList *show_hide_button_list, const char *desc)
{
  GtkWidget *button;

  button = gtk_button_new();
  gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_BUTTON)) ;

  char *text = g_strdup_printf("Set all columns to '%s'", desc) ;
  gtk_widget_set_tooltip_text(button, text) ;
  g_free(text) ;

  gtk_table_attach(table, button, col, col + 1, row, row + 1, GTK_SHRINK, GTK_SHRINK, XPAD, YPAD) ;
  g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(select_all_buttons), show_hide_button_list);

  return ;
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


/* Callback when user presses Up/Down buttons (to reorder columns) */
static void move_button_cb(GtkWidget *button, gpointer user_data)
{
  UpDownButton data = (UpDownButton)user_data ;
  zMapReturnIfFail(data
                   && data->direction
                   && data->page_data
                   && data->column_id
                   && data->columns_list_iter
                   && data->page_data->columns_list) ;

  LoadedPageData page_data = data->page_data ;

  /* The columns_list_iter is the position in the columns_list for this column */
  GList *this_item = data->columns_list_iter ;
  ZMapFeatureColumn this_column = (ZMapFeatureColumn)(this_item->data) ;

  /* We want to swap it with the prev/next item, depending on the direction */
  GList *swap_item = data->direction < 0 ? this_item->prev : this_item->next ;

  /* If there's no item to swap with we're already at the start/end, so nothing to do */
  if (swap_item)
    {
      /* Flag that there are changes */
      page_data->configure_data->columns_changed = TRUE ;

      /* Swap the items in the columns list */
      page_data->columns_list = g_list_remove_link(page_data->columns_list, this_item) ;

      if (data->direction < 0)
        page_data->columns_list = g_list_insert_before(page_data->columns_list, swap_item, this_column) ;
      else if (swap_item->next)
        page_data->columns_list = g_list_insert_before(page_data->columns_list, swap_item->next, this_column) ;
      else
        page_data->columns_list = g_list_append(page_data->columns_list, this_column) ;

      /* Recreate the columns panel on the dialog. This re-creates the data lists based on the new
       * columns list, so we need to clear the existing data first. This is pretty dodgy because
       * it will free the user_data pointers that gets passed to this callback (so our own
       * user_data will be invalidated after this). Should really tidy this up but it would
       * require quite a bit of reorganisation and I'm not sure it's worth it. We destroy the
       * widgets first so it's unlikely the callback will be called with the invalidated data pointers. */

      gtk_widget_destroy(page_data->cols_panel) ; // destroy all the current widgets
      page_data->cols_panel = NULL ;
      loaded_page_clear_data(page_data) ;         // free all the callback data
      loaded_cols_panel(page_data) ;              // recreate all the widgets and data
    }
}

static UpDownButton create_up_down_button_data(GQuark column_id, 
                                               const int direction,
                                               LoadedPageData page_data,
                                               GList *columns_list_iter)
{
  UpDownButton data = g_new0(UpDownButtonStruct, 1);

  data->column_id = column_id ;
  data->direction = direction ;
  data->page_data = page_data ;
  data->columns_list_iter = columns_list_iter ;

  return data ;
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

  GtkCellRenderer *renderer = gtk_cell_renderer_toggle_new();
  gtk_cell_renderer_toggle_set_radio(GTK_CELL_RENDERER_TOGGLE(renderer), TRUE) ;
  g_signal_connect (renderer, "toggled", G_CALLBACK (loaded_show_button_cb), show_hide_data);

  GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(col_name, renderer, "active", tree_col_id, NULL);

  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
}


static void loaded_cols_panel(LoadedPageData page_data)
{
  GtkWidget *cols_panel = NULL ;

  /* Get list of column structs in current display order */
  if (!page_data->columns_list)
    page_data->columns_list = zMapFeatureGetOrderedColumnsList(page_data->window->context_map) ;

  /* Create the overall container */
  cols_panel = gtk_vbox_new(FALSE, 0) ;

  /* Create a scrolled window for our tree */
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(cols_panel), scrolled, TRUE, TRUE, 0) ;

  /* Create a tree view listing all the columns */
  GtkListStore *store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, 
                                           G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, 
                                           G_TYPE_BOOLEAN, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN) ;

  /* Loop through all columns in display order (columns are shown in mirror order on the rev
   * strand but we always use forward-strand order) */
  for (GList *col_iter = page_data->columns_list; col_iter; col_iter = col_iter->next)
    {
      /* Create a new row in the list store */
      GtkTreeIter iter ;
      gtk_list_store_append(store, &iter);

      ZMapFeatureColumn column = (ZMapFeatureColumn)(col_iter->data) ;

      FooCanvasGroup *column_group_fwd = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_FORWARD, column->unique_id) ;
      FooCanvasGroup *column_group_rev = zmapWindowGetColumnByID(page_data->window, ZMAPSTRAND_REVERSE, column->unique_id) ;

      ShowHideButton show_data_fwd, default_data_fwd, hide_data_fwd;
      ShowHideButton show_data_rev, default_data_rev, hide_data_rev;

      const char *label_text = g_quark_to_string(column->column_id);

      if((column_group_fwd || column_group_rev))
        {
          gtk_list_store_set(store, &iter, NAME_COLUMN, label_text, -1);

          /* Show two sets of radio buttons for each column to change column display state for
           * each strand. */
          if (column_group_fwd)
            {
              loaded_radio_buttons(store, &iter, TRUE, column_group_fwd,
                                   &show_data_fwd, &default_data_fwd, &hide_data_fwd);

              default_data_fwd->loaded_page_data = page_data ;
              show_data_fwd->loaded_page_data    = page_data ;
              hide_data_fwd->loaded_page_data    = page_data ;

              page_data->default_list_fwd = g_list_append(page_data->default_list_fwd, default_data_fwd);
              page_data->show_list_fwd    = g_list_append(page_data->show_list_fwd, show_data_fwd);
              page_data->hide_list_fwd    = g_list_append(page_data->hide_list_fwd, hide_data_fwd);

              ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)column_group_rev ;
              if (container)
                {
                  ZMapStyleColumnDisplayState col_state = zmapWindowContainerFeatureSetGetDisplay(container) ;
                  set_tree_store_value_from_state(col_state, store, &iter, TRUE) ;
                }
            }

          if (column_group_rev)
            {
              loaded_radio_buttons(store, &iter, FALSE, column_group_rev,
                                   &show_data_rev, &default_data_rev, &hide_data_rev);

              default_data_rev->loaded_page_data = page_data ;
              show_data_rev->loaded_page_data    = page_data ;
              hide_data_rev->loaded_page_data    = page_data ;

              page_data->default_list_rev = g_list_append(page_data->default_list_rev, default_data_rev);
              page_data->show_list_rev    = g_list_append(page_data->show_list_rev, show_data_rev);
              page_data->hide_list_rev    = g_list_append(page_data->hide_list_rev, hide_data_rev);

              ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)column_group_rev ;
              if (container)
                {
                  ZMapStyleColumnDisplayState col_state = zmapWindowContainerFeatureSetGetDisplay(container) ;
                  set_tree_store_value_from_state(col_state, store, &iter, FALSE) ;
                }
            }
        }
    }

  /* Create the tree view widget */
  GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
  gtk_tree_view_set_search_column (GTK_TREE_VIEW (tree), NAME_COLUMN);

  /* Create the text column */
  GtkTreeViewColumn *column = NULL ;

  GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Column", text_renderer, "text", NAME_COLUMN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

  /* Create the radio button columns */
  GtkTreeModel *model = GTK_TREE_MODEL(store) ;
  GtkTreeView *tree_view = GTK_TREE_VIEW(tree) ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, SHOW_FWD_COLUMN, "Show") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, AUTO_FWD_COLUMN, "Auto") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_FORWARD, HIDE_FWD_COLUMN, "Hide") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, SHOW_REV_COLUMN, "Show") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, AUTO_REV_COLUMN, "Auto") ;
  loaded_cols_panel_create_column(page_data, model, tree_view, ZMAPSTRAND_REVERSE, HIDE_REV_COLUMN, "Hide") ;
  
  /* pack the panel into the page container. */
  gtk_container_add(GTK_CONTAINER(scrolled), tree) ;
  page_data->cols_panel = cols_panel ;
  page_data->tree_model = GTK_TREE_MODEL(store) ;
  gtk_container_add(GTK_CONTAINER(page_data->page_container), cols_panel) ;
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
  GtkWidget *button_box, *apply_button, *cancel_button, *revert_button, *frame, *parent;

  parent = configure_data->vbox;

  button_box = gtk_hbutton_box_new();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (button_box), GTK_BUTTONBOX_END);
  gtk_box_set_spacing (GTK_BOX(button_box),
                       ZMAP_WINDOW_GTK_BUTTON_BOX_SPACING);
  gtk_container_set_border_width (GTK_CONTAINER (button_box),
                                  ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);

  revert_button = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED) ;
  gtk_box_pack_start(GTK_BOX(button_box), revert_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(revert_button), "clicked",
                     GTK_SIGNAL_FUNC(revert_button_cb), (gpointer)configure_data) ;

  cancel_button = gtk_button_new_from_stock(GTK_STOCK_CLOSE) ;
  gtk_box_pack_start(GTK_BOX(button_box), cancel_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked",
                     GTK_SIGNAL_FUNC(cancel_button_cb), (gpointer)configure_data) ;

  apply_button = gtk_button_new_from_stock(GTK_STOCK_APPLY) ;
  gtk_box_pack_end(GTK_BOX(button_box), apply_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(apply_button), "clicked",
     GTK_SIGNAL_FUNC(apply_button_cb), (gpointer)configure_data) ;

  /* set close button as default. */
  GTK_WIDGET_SET_FLAGS(apply_button, GTK_CAN_DEFAULT) ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_set_border_width(GTK_CONTAINER(frame),
                                 ZMAP_WINDOW_GTK_CONTAINER_BORDER_WIDTH);
  gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0) ;

  gtk_container_add(GTK_CONTAINER(frame), button_box);

  return apply_button;
}

/* This is not the way to do help, we should really used html and have a set of help files. */
static void helpCB(gpointer data, guint callback_action, GtkWidget *w)
{
  const char *title = "Column Configuration Window" ;
  const char *help_text =
    "The ZMap Column Configuration Window allows you to change the way columns are displayed.\n"
    "\n"
    "The window displays columns separately for the forward and reverse strands. You can set\n"
    "the display state of each column to one of:\n"
    "\n"
    "\"" SHOW_LABEL "\"  - always show the column\n"
    "\n"
    "\"" SHOWHIDE_LABEL "\"  - show the column depending on ZMap settings (e.g. zoom, compress etc.)\n"
    "\n"
    "\"" HIDE_LABEL "\"  - always hide the column\n"
    "\n"
    "By default column display is controlled by settings in the Style for that column,\n"
    "including the min and max zoom levels at which the column should be shown, how the\n"
    "the column is bumped, the window mark and compress options. Column display can be\n"
    "overridden however to always show or always hide columns.\n"
    "\n"
    "To redraw the display, either click the \"Apply\" button, or if there is no button the\n"
    "display will be redrawn immediately.  To temporarily turn on the immediate redraw hold\n"
    "Control while selecting the radio button."
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


/* For the given row and strand, apply the updates to the appropriate column */
static void loaded_page_apply_tree_row(LoadedPageData loaded_page_data, ZMapStrand strand, 
                                       GtkTreeIter *iter, ZMapStyleColumnDisplayState show_hide_state)
{
  zMapReturnIfFail(loaded_page_data && 
                   loaded_page_data->configure_data &&
                   loaded_page_data->configure_data->window) ;

  ColConfigure configure_data = loaded_page_data->configure_data ;
  GQuark column_id = tree_model_get_column_id(loaded_page_data->tree_model, iter) ;
  FooCanvasGroup *column_group = zmapWindowGetColumnByID(loaded_page_data->window, strand, column_id) ;

  /* We only do this if there is no apply button.  */
  if(column_group && loaded_page_data->apply_now)
    {
      ZMapWindow window = configure_data->window;

      if (IS_3FRAME(window->display_3_frame))
        {
          ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet)(column_group);;
          ZMapFeatureSet feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container);

          if(feature_set)
            {
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
          else
            {

            }
        }
      else
        {
          zmapWindowColumnSetState(window,
                                   column_group,
                                   show_hide_state,
                                   loaded_page_data->reposition) ;
        }
    }
}


/* Get the column name for the given row in the given tree. Return it as a unique id */
static GQuark tree_model_get_column_id(GtkTreeModel *model, GtkTreeIter *iter)
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


/* Called when user selects one of the display state radio buttons.
 *
 * NOTE: selecting a button will deselect the other buttons, this routine
 * gets called for each of those deselections as well. */
static void loaded_show_button_cb(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data)
{
  ShowHideData button_data = (ShowHideData)user_data ;
  zMapReturnIfFail(path_string && 
                   button_data && 
                   button_data->loaded_page_data->tree_model && 
                   button_data->loaded_page_data && 
                   button_data->loaded_page_data->configure_data) ;

  gboolean ok = TRUE ;
  GtkTreeModel *model = button_data->loaded_page_data->tree_model ;
  LoadedPageData loaded_page_data = button_data->loaded_page_data;
  ZMapStyleColumnDisplayState show_hide_state ;

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
      show_hide_state = get_state_from_tree_store_column(button_data->tree_col_id) ;
    }

  if (ok)
    {
      /* Toggle the cell values (the selected cell should be set to active and the others in the
       * group set to inactive) */
      gboolean forward = (button_data->strand == ZMAPSTRAND_FORWARD) ;
      set_tree_store_value_from_state(show_hide_state, GTK_LIST_STORE(model), &iter, forward) ;
    }

  if (ok)
    {
      /* Set the correct state. */
      loaded_page_apply_tree_row(loaded_page_data, button_data->strand, &iter, show_hide_state) ;
    }

  return ;
}

static void each_radio_button_activate(gpointer list_data, gpointer user_data)
{
  ShowHideButton show_hide_button = (ShowHideButton)list_data;
  GtkWidget *widget;

  widget = show_hide_button->show_hide_button;

  if(GTK_IS_RADIO_BUTTON(widget))
    {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE) ;
    }
  else
    zMapLogWarning("widget '%s' is not a radio button", G_OBJECT_TYPE_NAME(G_OBJECT(widget)));

  return ;
}

static void select_all_buttons(GtkWidget *button, gpointer user_data)
{
  GList *show_hide_button_list = (GList *)user_data;
  gboolean needs_reposition;
  //  ColConfigure configure_data;
  LoadedPageData loaded_page_data;

  if(show_hide_button_list)
    {
      ShowHideButton first;

      first = (ShowHideButton)(show_hide_button_list->data);

      loaded_page_data = first->loaded_page_data;

      needs_reposition = loaded_page_data->reposition;

      loaded_page_data->reposition = FALSE;

      g_list_foreach(show_hide_button_list, each_radio_button_activate, NULL);

      // unitiialised
      //      if((loaded_page_data->reposition = needs_reposition))
      //zmapWindowFullReposition(configure_data->window->feature_root_group,TRUE);
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

  configure_data->columns_changed = FALSE ;

  return ;
}

static void apply_button_cb(GtkWidget *apply_button, gpointer user_data)
{
  ColConfigure configure_data = (ColConfigure)user_data;

  notebook_foreach_page(configure_data->notebook, FALSE, NOTEBOOK_APPLY_FUNC, NULL);

  if (configure_data->columns_changed)
    configure_data->window->flags[ZMAPFLAG_COLUMNS_NEED_SAVING] = TRUE ;

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
    g_warning("map: %d x %d", sizing_data->widget_requisition.width, sizing_data->widget_requisition.height);
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
    g_warning("unmap: %d x %d", widget->requisition.width, widget->requisition.height);
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
    g_warning("alloc: %d x %d", allocation->width, allocation->height);
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
