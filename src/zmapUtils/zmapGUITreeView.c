/*  File: zmapGUITreeView.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h> /* memcpy() */

#include <ZMap/zmapBase.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapGUITreeView_I.h>

enum
  {
    ZMAP_GUITV_NOPROP,        /* zero is invalid property id */

    ZMAP_GUITV_ROW_COUNTER_COLUMN, /* must be first in variadic args... if present ...*/
    ZMAP_GUITV_DATA_PTR_COLUMN, /* must be first in variadic args... if present ...*/
    ZMAP_GUITV_COLUMN_COUNT, /* otherwise this must be first in variadic args... */

    /* specified as blocks */
    ZMAP_GUITV_COLUMN_NAME,
    ZMAP_GUITV_COLUMN_TYPE,
    ZMAP_GUITV_COLUMN_DATA_FUNC,
    ZMAP_GUITV_COLUMN_FLAGS,

    /* specified as GList * */
    ZMAP_GUITV_COLUMN_NAMES,
    ZMAP_GUITV_COLUMN_NAME_QUARKS,
    ZMAP_GUITV_COLUMN_TYPES,
    ZMAP_GUITV_COLUMN_DATA_FUNCS,
    ZMAP_GUITV_COLUMN_FLAGS_LIST,

    /* selecting */
    ZMAP_GUITV_SELECT_MODE,
    ZMAP_GUITV_SELECT_FUNC,
    ZMAP_GUITV_SELECT_FUNC_DATA,
    ZMAP_GUITV_SELECT_FUNC_DESTROY,

    /* sorting columns... */
    ZMAP_GUITV_SORTABLE,
    ZMAP_GUITV_SORT_COLUMN_NAME,
    ZMAP_GUITV_SORT_COLUMN_INDEX,
    ZMAP_GUITV_SORT_ORDER,
    ZMAP_GUITV_SORT_FUNC,
    ZMAP_GUITV_SORT_FUNC_DATA,
    ZMAP_GUITV_SORT_FUNC_DESTROY,

    /* Access _only_ properties */
    ZMAP_GUITV_WIDGET,
    ZMAP_GUITV_MODEL,
    ZMAP_GUITV_COUNTER_COLUMN_INDEX,
    ZMAP_GUITV_DATA_PTR_COLUMN_INDEX,
  };


static void zmap_guitreeview_class_init(ZMapGUITreeViewClass zmap_tv_class);
static void zmap_guitreeview_init(ZMapGUITreeView zmap_tv);
static void zmap_guitreeview_set_property(GObject *gobject,
                                guint param_id,
                                const GValue *value,
                                GParamSpec *pspec);
static void zmap_guitreeview_get_property(GObject *gobject,
                                guint param_id,
                                GValue *value,
                                GParamSpec *pspec);
static void zmap_guitreeview_dispose(GObject *object);
static void zmap_guitreeview_finalize(GObject *object);

static void zmap_guitreeview_simple_add(ZMapGUITreeView zmap_tv,
                              gpointer user_data);
static void zmap_guitreeview_simple_add_values(ZMapGUITreeView zmap_tv,
                                     GList *values_list);
static void zmap_guitreeview_add_list_of_tuples(ZMapGUITreeView zmap_tv,
                                    GList *tuples_list);

static void each_tuple_swap_invoke_add(gpointer list_data, gpointer user_data);


static void set_column_names (gpointer list_data, gpointer user_data);
static void set_column_quarks(gpointer list_data, gpointer user_data);
static void set_column_types (gpointer list_data, gpointer user_data);
static void set_column_funcs (gpointer list_data, gpointer user_data);
static void set_column_flags (gpointer list_data, gpointer user_data);

static void tuple_no_to_model     (GValue *value, gpointer user_data);
static void tuple_pointer_to_model(GValue *value, gpointer user_data);
static GtkTreeView  *createView (ZMapGUITreeView zmap_tv);
static GtkTreeModel *createModel(ZMapGUITreeView zmap_tv);

static int column_index_from_name(ZMapGUITreeView zmap_tv, char *name);

static void progressive_set_selection(ZMapGUITreeView zmap_tv);

static void update_tuple_data(ZMapGUITreeView zmap_tv,
                        GtkListStore   *store,
                        GtkTreeIter    *iter,
                        gboolean        update_counter,
                        gpointer        user_data);
static void update_tuple_data_list(ZMapGUITreeView zmap_tv,
                           GtkListStore   *store,
                           GtkTreeIter    *iter,
                           gboolean        update_counter,
                           GList          *values_list);

/* Sizing functions for the GtkWidget *GtkTreeView.
 * Hopefully They make it do the right thing. */
static void  tree_view_size_request(GtkWidget      *widget,
                            GtkRequisition *requisition,
                            gpointer        user_data);
static gboolean tree_view_map(GtkWidget *widget, GdkEvent  *event, gpointer user_data) ;
static gboolean tree_view_unmap(GtkWidget *widget, gpointer user_data) ;
static void tree_view_size_allocation(GtkWidget     *widget,
                              GtkAllocation *allocation,
                              gpointer       user_data);


GType zMapGUITreeViewGetType (void)
{
  static GType type = 0;

  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (zmapGUITreeViewClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) zmap_guitreeview_class_init,
      (GClassFinalizeFunc) NULL,
      NULL /* class_data */,
      sizeof (zmapGUITreeView),
      0 /* n_preallocs */,
      (GInstanceInitFunc) zmap_guitreeview_init,
      NULL
    };

    type = g_type_register_static (G_TYPE_OBJECT, "ZMapGUITreeView", &info, (GTypeFlags)0);
  }

  return type;
}


ZMapGUITreeView zMapGUITreeViewCreate(void)
{
  ZMapGUITreeView zmap_tv = NULL;

  zmap_tv = ((ZMapGUITreeView)g_object_new(zMapGUITreeViewGetType(), NULL));

  return zmap_tv;
}

GtkTreeView *zMapGUITreeViewGetView(ZMapGUITreeView zmap_tv)
{
  GtkTreeView *gtk_tree_view = NULL;

  if(!(gtk_tree_view = zmap_tv->tree_view))
    {
      gtk_tree_view = zmap_tv->tree_view = createView(zmap_tv);

      /* Some clever person wrote this to depend on zmap_tv->tree_view ;) */
      progressive_set_selection(zmap_tv);
    }

  return gtk_tree_view;
}

GtkTreeModel *zMapGUITreeViewGetModel(ZMapGUITreeView zmap_tv)
{
  GtkTreeModel *gtk_tree_model = NULL;

  if(!(gtk_tree_model = zmap_tv->tree_model))
    {
      gtk_tree_model = zmap_tv->tree_model = createModel(zmap_tv);
    }

  return gtk_tree_model;
}

gboolean zMapGUITreeViewPrepare(ZMapGUITreeView zmap_tv)
{
  GtkTreeView *tree_view;
  GtkTreeModel *tree_model;
  gboolean prepared = FALSE;

  if((tree_view  = zMapGUITreeViewGetView(zmap_tv)) &&
     (tree_model = zMapGUITreeViewGetModel(zmap_tv)))
    {
      GtkTreeModel *set_model;

      set_model = gtk_tree_view_get_model(tree_view);

      if(set_model == tree_model)
      prepared = TRUE;

      if(prepared)
      {
        GtkTreeSortable *sort_model = GTK_TREE_SORTABLE(set_model);
        GtkSortType sort_order;
        int index;

        /* Detach model from view */
        g_object_ref(G_OBJECT(tree_model));

        gtk_tree_view_set_model(tree_view, NULL);

        gtk_tree_sortable_get_sort_column_id(sort_model, &index, &sort_order);

        if(index != GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID &&
           index != GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID  &&
           index != zmap_tv->sort_index)
          zmap_tv->sort_index = index;

        if(sort_order != zmap_tv->sort_order)
          zmap_tv->sort_order = sort_order;

        gtk_tree_sortable_set_sort_column_id(sort_model,
                                     GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
                                     GTK_SORT_ASCENDING);
        {
          zMapAssert(gtk_tree_view_get_model(tree_view) == NULL);
        }
      }

      /* If we have a model, but it has never been attached prepared
       * == FALSE still. This is logically wrong for the name of the
       * function and its prototype, so we set it to TRUE here. This
       * is done after the previous bit so as not to unref. */

      if(set_model == NULL)
      prepared = TRUE;
    }

  return prepared;
}

gboolean zMapGUITreeViewAttach(ZMapGUITreeView zmap_tv)
{
  GtkTreeView *tree_view;
  GtkTreeModel *tree_model;
  gboolean attached = FALSE;

  if((tree_view  = zMapGUITreeViewGetView(zmap_tv)) &&
     (tree_model = zMapGUITreeViewGetModel(zmap_tv)))
    {
      GtkTreeSortable *sort_model;
      GtkTreeModel *set_model;
      GtkSortType sort_order;
      int index = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
      /* index will most likely end up being
       * GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID */
      sort_model = GTK_TREE_SORTABLE(tree_model);

      gtk_tree_sortable_get_sort_column_id(sort_model, &index, &sort_order);

      set_model  = gtk_tree_view_get_model(tree_view);

      if((zmap_tv->sort_index != GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID) &&
       (zmap_tv->sort_index != index))
      {
        if(zmap_tv->sort_func)
          gtk_tree_sortable_set_sort_func(sort_model,
                                  zmap_tv->sort_index,
                                  zmap_tv->sort_func,
                                  zmap_tv->sort_func_data,
                                  zmap_tv->sort_destroy);
        else if(gtk_tree_sortable_has_default_sort_func(sort_model))
          gtk_tree_sortable_set_sort_column_id(sort_model,
                                     zmap_tv->sort_index,
                                     zmap_tv->sort_order);
        else if(!GTK_IS_TREE_MODEL_SORT(set_model))
          {
            GtkTreeViewColumn *sort_column;
            GtkSortType sort_order;

            gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tree_model),
                                       zmap_tv->sort_index,
                                       zmap_tv->sort_order);
            sort_column = gtk_tree_view_get_column(zmap_tv->tree_view,
                                         zmap_tv->sort_index);
            sort_order  = (zmap_tv->sort_order == GTK_SORT_ASCENDING ?
                       GTK_SORT_DESCENDING : GTK_SORT_ASCENDING);
            g_object_set(G_OBJECT(sort_column),
                     "sort-indicator", TRUE,
                     "sort-order",     sort_order,
                     NULL);
          }
      }

      if(set_model != tree_model)
      gtk_tree_view_set_model(tree_view, tree_model);

      if(set_model == NULL || set_model != tree_model)
      {
        g_object_unref(G_OBJECT(tree_model));
      }

      attached = TRUE;
    }

  return attached;
}

void zMapGUITreeViewAddTuple(ZMapGUITreeView zmap_tv, gpointer user_data)
{
  if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_simple)
    (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_simple)(zmap_tv, user_data);

  return ;
}

void zMapGUITreeViewAddTupleFromColumnData(ZMapGUITreeView zmap_tv,
                                 GList *values_list)
{
  if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_value_list)
    (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_value_list)(zmap_tv, values_list);

  return ;
}

void zMapGUITreeViewAddTuples(ZMapGUITreeView zmap_tv, GList *list_of_user_data)
{
  if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuples)
    (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuples)(zmap_tv, list_of_user_data);

  return ;
}

int zMapGUITreeViewGetColumnIndexByName(ZMapGUITreeView zmap_tv, char *column_name)
{
  int index = -1;

  if(zmap_tv->column_count > 0)
    {
      GQuark *quarks = zmap_tv->column_names;
      GQuark query   = g_quark_from_string(column_name);
      int i;

      for(i = 0; quarks && i < zmap_tv->column_count; i++, quarks++)
      {
        GQuark curr_name = *quarks;

        if(curr_name == query)
          {
            index = i;
            break;
          }
      }
    }

  return index;
}

void zMapGUITreeViewUpdateTuple(ZMapGUITreeView zmap_tv, GtkTreeIter *iter, gpointer user_data)
{
  GtkListStore *store;

  store = GTK_LIST_STORE(zmap_tv->tree_model);

  update_tuple_data(zmap_tv, store, iter, FALSE, user_data);

  return ;
}

ZMapGUITreeView zMapGUITreeViewDestroy(ZMapGUITreeView zmap_tv)
{
  g_object_unref(G_OBJECT(zmap_tv));

  zmap_tv = NULL;

  return zmap_tv;
}


static void zmap_guitreeview_class_init(ZMapGUITreeViewClass zmap_tv_class)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)zmap_tv_class;

  gobject_class->set_property = zmap_guitreeview_set_property;
  gobject_class->get_property = zmap_guitreeview_get_property;

  /* Specifying the model/layout */
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_ROW_COUNTER_COLUMN,
                          g_param_spec_boolean("row-counter-column", "row-counter-column",
                                           "First Column is a row counter column",
                                           FALSE, ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_DATA_PTR_COLUMN,
                          g_param_spec_boolean("data-ptr-column", "data-ptr-column",
                                           "Specify there should be a column holding the pointer.",
                                           FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_COUNT,
                          g_param_spec_uint("column-count", "columns",
                                        "Number of columns in view",
                                        1, 128, 1, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_NAME,
                          g_param_spec_string("column-name", "column-name",
                                          "The name of the column",
                                          "default-column", ZMAP_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_TYPE,
                          g_param_spec_gtype("column-type", "column-type",
                                         "The GType for the column",
                                          G_TYPE_NONE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_DATA_FUNC,
                          g_param_spec_pointer("column-func", "column-func",
                                           "A getter column function",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_FLAGS,
                          g_param_spec_uint("column-flags", "column-flags",
                                        "Setting for current column flags",
                                        ZMAP_GUITREEVIEW_COLUMN_NOTHING,
                                        ZMAP_GUITREEVIEW_COLUMN_INVALID - 1,
                                        ZMAP_GUITREEVIEW_COLUMN_NOTHING,
                                        ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_NAMES,
                          g_param_spec_pointer("column-names", "column-names",
                                           "A GList * of the column names",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_NAME_QUARKS,
                          g_param_spec_pointer("column-names-q", "column-names-q",
                                           "A GList * of GQuarks of the column names",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_TYPES,
                          g_param_spec_pointer("column-types", "column-types",
                                           "A GList * of the column types",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_DATA_FUNCS,
                          g_param_spec_pointer("column-funcs", "column-funcs",
                                           "A GList * of the column functions",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COLUMN_FLAGS_LIST,
                          g_param_spec_pointer("column-flags-list", "column-flags-list",
                                           "A GList * of the column flags",
                                           ZMAP_PARAM_STATIC_RW));

  /* Selection functionality... */
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SELECT_MODE,
                          g_param_spec_enum("selection-mode", "selection-mode",
                                        "A GtkSelectionMode for selection",
                                        GTK_TYPE_SELECTION_MODE,
                                        GTK_SELECTION_NONE,
                                        ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SELECT_FUNC,
                          g_param_spec_pointer("selection-func", "selection-func",
                                           "A GtkTreeSelectionFunc to handle selection",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SELECT_FUNC_DATA,
                          g_param_spec_pointer("selection-data", "selection-data",
                                           "A pointer to pass to the GtkTreeSelectionFunc",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SELECT_FUNC_DESTROY,
                          g_param_spec_pointer("selection-destroy", "selection-destroy",
                                           "A GDestroyNotify called on removal of selection-func",
                                           ZMAP_PARAM_STATIC_RW));

  /* Sort functionality... */
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORTABLE,
                          g_param_spec_boolean("sortable", "sortable",
                                           "Whether the tree view/model is sortable",
                                           FALSE, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_COLUMN_NAME,
                          g_param_spec_string("sort-column-name", "sort-column-name",
                                          "Name of column to sort",
                                          "#", ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_COLUMN_INDEX,
                          g_param_spec_uint("sort-column-index", "sort-column-index",
                                        "Zero based index of column to sort",
                                        0, 128, 0, ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_ORDER,
                          g_param_spec_enum("sort-order", "sort-order",
                                        "Order to sort with",
                                        GTK_TYPE_SORT_TYPE,
                                        GTK_SORT_ASCENDING,
                                        ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_FUNC,
                          g_param_spec_pointer("sort-func", "sort-func",
                                           "A GtkTreeIterCompareFunc to handle sorting",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_FUNC_DATA,
                          g_param_spec_pointer("sort-data", "sort-data",
                                           "A pointer to pass to the GtkTreeIterCompareFunc",
                                           ZMAP_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_SORT_FUNC_DESTROY,
                          g_param_spec_pointer("sort-destroy", "sort-destroy",
                                           "A GDestroyNotify called on removal of sort-func",
                                           ZMAP_PARAM_STATIC_RW));

  /* Access to the widget/model */
  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_WIDGET,
                          g_param_spec_object("tree-view", "tree-view",
                                          "GtkWidget *tree_view",
                                          GTK_TYPE_WIDGET,
                                          ZMAP_PARAM_STATIC_RO));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_MODEL,
                          g_param_spec_object("tree-model", "tree-model",
                                          "GtkTreeModel *tree_model",
                                          GTK_TYPE_TREE_MODEL,
                                          ZMAP_PARAM_STATIC_RO));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_COUNTER_COLUMN_INDEX,
                          g_param_spec_int("counter-index", "counter-index",
                                        "The index for the counter column",
                                        -1, 1, -1,
                                        ZMAP_PARAM_STATIC_RO));

  g_object_class_install_property(gobject_class,
                          ZMAP_GUITV_DATA_PTR_COLUMN_INDEX,
                          g_param_spec_int("data-ptr-index", "data-ptr-index",
                                       "The index for the data ptr column.",
                                       -1, 1, -1,
                                       ZMAP_PARAM_STATIC_RO));


  zmap_tv_class->init_layout          = NULL;
  zmap_tv_class->add_tuple_simple     = zmap_guitreeview_simple_add;
  zmap_tv_class->add_tuple_value_list = zmap_guitreeview_simple_add_values;
  zmap_tv_class->add_tuples           = zmap_guitreeview_add_list_of_tuples;

  gobject_class->dispose  = zmap_guitreeview_dispose;
  gobject_class->finalize = zmap_guitreeview_finalize;

  return ;
}

static void zmap_guitreeview_init(ZMapGUITreeView zmap_tv)
{

  if(ZMAP_IS_GUITREEVIEW(zmap_tv))
    {
      zmap_tv->column_count = 0;
      zmap_tv->curr_column  = 0;
      zmap_tv->sort_index = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
    }

  return ;
}

static void zmap_guitreeview_set_property(GObject *gobject,
                                guint param_id,
                                const GValue *value,
                                GParamSpec *pspec)
{
  ZMapGUITreeView zmap_tv;

  g_return_if_fail(ZMAP_IS_GUITREEVIEW(gobject));

  zmap_tv = ZMAP_GUITREEVIEW(gobject);

  if((param_id > ZMAP_GUITV_COLUMN_COUNT) &&
     (zmap_tv->column_count == 0))
    {
      g_warning("%s", "Column count should be set first.");
      g_return_if_fail(zmap_tv->column_count != 0);
    }

  switch(param_id)
    {
    case ZMAP_GUITV_ROW_COUNTER_COLUMN:
      zmap_tv->tuple_counter = g_value_get_boolean(value);
      break;
    case ZMAP_GUITV_DATA_PTR_COLUMN:
      zmap_tv->add_data_ptr = g_value_get_boolean(value);
      break;
    case ZMAP_GUITV_COLUMN_COUNT:   /* must be first in variadic args... */
      {
      int requested_count  = g_value_get_uint(value);
      zmap_tv->curr_column = 0;

      if(zmap_tv->column_count == 0)
        {
          zmap_tv->column_count = requested_count;

          if(zmap_tv->tuple_counter)
            zmap_tv->column_count+=1;

          if(zmap_tv->add_data_ptr)
            zmap_tv->column_count+=1;

          /* alloc arrays */

          zmap_tv->column_names   = g_new0(GQuark, zmap_tv->column_count);

          zmap_tv->column_types   = g_new0(GType, zmap_tv->column_count);

          zmap_tv->column_funcs   = g_new0(ZMapGUITreeViewCellFunc, zmap_tv->column_count);

          zmap_tv->column_values  = g_new0(GValue, zmap_tv->column_count);

          zmap_tv->column_numbers = g_new0(int, zmap_tv->column_count);

          zmap_tv->column_flags   = g_new0(ColumnFlagsStruct, zmap_tv->column_count);

          {
            int i;
            for(i = 0; i < zmap_tv->column_count; i++)
            {
              zmap_tv->column_numbers[i] = i;
              /* Default columns to visible */
              zmap_tv->column_flags[i].bitflags = DEFAULT_COLUMN_FLAGS;
            }
          }

          if(zmap_tv->tuple_counter)
            {
            zmap_tv->curr_column = 0;
            set_column_names(ZMAP_GUITREEVIEW_COUNTER_COLUMN_NAME, zmap_tv);
            zmap_tv->curr_column = 0;
            set_column_types(GINT_TO_POINTER(G_TYPE_INT), zmap_tv);
            zmap_tv->curr_column = 0;
            set_column_funcs(tuple_no_to_model, zmap_tv);
            zmap_tv->curr_column = 0;
            set_column_flags(GINT_TO_POINTER(DEFAULT_COLUMN_FLAGS), zmap_tv);
            }

          if(zmap_tv->add_data_ptr)
            {
            zmap_tv->curr_column = 1;
            set_column_names(ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME, zmap_tv);
            zmap_tv->curr_column = 1;
            set_column_types(GINT_TO_POINTER(G_TYPE_POINTER), zmap_tv);
            zmap_tv->curr_column = 1;
            set_column_funcs(tuple_pointer_to_model, zmap_tv);
            zmap_tv->curr_column = 1;
            set_column_flags(GINT_TO_POINTER(ZMAP_GUITREEVIEW_COLUMN_NOTHING), zmap_tv);
            }
        }
      else
        {
          /* re allocate, how are we going to specify additional columns... */
          /* NOT IMPLEMENTED... */
        }
      }
      break;
    /* specified as blocks */
    case ZMAP_GUITV_COLUMN_NAME:
      {
      GQuark column_id;
      column_id = g_quark_from_string(g_value_get_string(value));

      if(column_id != 0)
        {
#ifdef G_OBJECT_SET_MEMO
          /* The idea is for callers to this style of g_object_set */
          g_object_set(gobject,
                   "column-name", "first_column",
                   "column_type", G_TYPE_INT,
                   "column_func", first_col_get,
                   "column-name", "second_column",
                   "column_func", second_col_get,
                   "column_type", G_TYPE_INT,
                   "column-name", "third_column",
                   "column_func", third_col_get,
                   "column_type", G_TYPE_STRING,
                   NULL);
#endif /* G_OBJECT_SET_MEMO */
          if(zmap_tv->curr_column < zmap_tv->column_count)
            {
            zmap_tv->column_names[zmap_tv->curr_column] = column_id;

            /* Only here do we increment the current column */
            zmap_tv->curr_column++;
            }
        }
      }
      break;
    case ZMAP_GUITV_COLUMN_TYPE:
      {
      GType column_type;
      column_type = g_value_get_gtype(value);
      if(column_type != 0)
        {
          if(zmap_tv->curr_column < zmap_tv->column_count &&
             zmap_tv->curr_column > 0)
            zmap_tv->column_types[zmap_tv->curr_column - 1] = column_type;
        }
      }
      break;
    case ZMAP_GUITV_COLUMN_DATA_FUNC:
      {
      gpointer column_func;
      column_func = g_value_get_pointer(value);
      if(column_func)
        {
          if(zmap_tv->curr_column < zmap_tv->column_count &&
             zmap_tv->curr_column > 0)
            zmap_tv->column_funcs[zmap_tv->curr_column - 1] = column_func;
        }
      }
      break;
    case ZMAP_GUITV_COLUMN_FLAGS:
      {
      unsigned int column_flags;
      column_flags = g_value_get_uint(value);
      if(column_flags)
        {
          int save_curr_column = zmap_tv->curr_column;
          if(zmap_tv->curr_column < zmap_tv->column_count &&
             zmap_tv->curr_column > 0)
            set_column_flags(GINT_TO_POINTER(column_flags), zmap_tv);
          zmap_tv->curr_column = save_curr_column;
        }
      }
      break;

    /* specified as GList * */
    case ZMAP_GUITV_COLUMN_NAMES:
      zmap_tv->curr_column = 0;

      if(zmap_tv->tuple_counter)
      zmap_tv->curr_column++;

      if(zmap_tv->add_data_ptr)
      zmap_tv->curr_column++;

      g_list_foreach(g_value_get_pointer(value), set_column_names, zmap_tv);

      if((zmap_tv->curr_column == 0) ||
       (zmap_tv->curr_column != zmap_tv->column_count) ||
       (zmap_tv->curr_column == 1 && zmap_tv->tuple_counter))
      {
        g_warning("Expected %d column names, but got %d",
                zmap_tv->column_count,
                zmap_tv->curr_column);
      }
      break;
    case ZMAP_GUITV_COLUMN_NAME_QUARKS:
      zmap_tv->curr_column = 0;

      if(zmap_tv->tuple_counter)
      zmap_tv->curr_column++;

      g_list_foreach(g_value_get_pointer(value), set_column_quarks, zmap_tv);

      if((zmap_tv->curr_column == 0) ||
       (zmap_tv->curr_column != zmap_tv->column_count) ||
       (zmap_tv->curr_column == 1 && zmap_tv->tuple_counter))
      {
        g_warning("Expected %d column names, but got %d",
                zmap_tv->column_count,
                zmap_tv->curr_column);
      }
      break;
    case ZMAP_GUITV_COLUMN_TYPES:
      zmap_tv->curr_column = 0;

      if(zmap_tv->tuple_counter)
      zmap_tv->curr_column++;

      if(zmap_tv->add_data_ptr)
      zmap_tv->curr_column++;

      g_list_foreach(g_value_get_pointer(value), set_column_types, zmap_tv);

      if((zmap_tv->curr_column == 0) ||
       (zmap_tv->curr_column != zmap_tv->column_count) ||
       (zmap_tv->curr_column == 1 && zmap_tv->tuple_counter))
      {
        g_warning("Expected %d column types, but got %d",
                zmap_tv->column_count,
                zmap_tv->curr_column);
      }
      break;
    case ZMAP_GUITV_COLUMN_DATA_FUNCS:
      zmap_tv->curr_column = 0;

      if(zmap_tv->tuple_counter)
      zmap_tv->curr_column++;

      if(zmap_tv->add_data_ptr)
      zmap_tv->curr_column++;

      g_list_foreach(g_value_get_pointer(value), set_column_funcs, zmap_tv);

      if((zmap_tv->curr_column == 0) ||
       (zmap_tv->curr_column != zmap_tv->column_count) ||
       (zmap_tv->curr_column == 1 && zmap_tv->tuple_counter))
      {
        g_warning("Expected %d column funcs, but got %d",
                zmap_tv->column_count,
                zmap_tv->curr_column);
      }
      break;
    case ZMAP_GUITV_COLUMN_FLAGS_LIST:
      zmap_tv->curr_column = 0;

      if(zmap_tv->tuple_counter)
      zmap_tv->curr_column++;

      if(zmap_tv->add_data_ptr)
      zmap_tv->curr_column++;

      g_list_foreach(g_value_get_pointer(value), set_column_flags, zmap_tv);

      if((zmap_tv->curr_column == 0) ||
       (zmap_tv->curr_column != zmap_tv->column_count) ||
       (zmap_tv->curr_column == 1 && zmap_tv->tuple_counter))
      {
        g_warning("Expected %d column flag settings, but got %d",
                zmap_tv->column_count,
                zmap_tv->curr_column);
      }
      break;

      /* selecting columns */
    case ZMAP_GUITV_SELECT_MODE:
      {
      GtkTreeSelection *selection;

      zmap_tv->select_mode = g_value_get_enum(value);
      if(zmap_tv->tree_view &&
         (selection = gtk_tree_view_get_selection(zmap_tv->tree_view)))
        {
          gtk_tree_selection_set_mode(selection, zmap_tv->select_mode);
        }
      }
      break;
    case ZMAP_GUITV_SELECT_FUNC:
      zmap_tv->select_func = g_value_get_pointer(value);

      progressive_set_selection(zmap_tv);
      break;
    case ZMAP_GUITV_SELECT_FUNC_DATA:
      zmap_tv->select_func_data = g_value_get_pointer(value);

      progressive_set_selection(zmap_tv);
      break;
    case ZMAP_GUITV_SELECT_FUNC_DESTROY:
      zmap_tv->select_destroy = g_value_get_pointer(value);

      progressive_set_selection(zmap_tv);
      break;
      /* sorting columns... */
    case ZMAP_GUITV_SORTABLE:
      zmap_tv->sortable = g_value_get_boolean(value);
      break;
    case ZMAP_GUITV_SORT_COLUMN_NAME:
      {
      char *sort_column;
      int index;
      /* Should we be copying this string? */
      sort_column = (char *)g_value_get_string(value);

      index = column_index_from_name(zmap_tv, sort_column);
      zmap_tv->sort_index = index;
      }
      break;
    case ZMAP_GUITV_SORT_COLUMN_INDEX:
      {
      int index;
      index = g_value_get_uint(value);
      if(index < zmap_tv->column_count)
        zmap_tv->sort_index = index;
      }
      break;
    case ZMAP_GUITV_SORT_ORDER:
      zmap_tv->sort_order = g_value_get_enum(value);
      break;
    case ZMAP_GUITV_SORT_FUNC:
      zmap_tv->sort_func = g_value_get_pointer(value);
      break;
    case ZMAP_GUITV_SORT_FUNC_DATA:
      zmap_tv->sort_func_data = g_value_get_pointer(value);
      break;
    case ZMAP_GUITV_SORT_FUNC_DESTROY:
      zmap_tv->sort_destroy = g_value_get_pointer(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
}

  return ;
}

static void zmap_guitreeview_get_property(GObject *gobject,
                                guint param_id,
                                GValue *value,
                                GParamSpec *pspec)
{
  ZMapGUITreeView zmap_tv;

  g_return_if_fail(ZMAP_IS_GUITREEVIEW(gobject));

  zmap_tv = ZMAP_GUITREEVIEW(gobject);

  switch(param_id)
    {
    case ZMAP_GUITV_SELECT_FUNC:
      g_value_set_pointer(value, zmap_tv->select_func);
      break;
    case ZMAP_GUITV_WIDGET:
      {
      GtkWidget *widget = NULL;
      GtkTreeView *view = NULL;

      if((view = zMapGUITreeViewGetView(zmap_tv)))
        widget = GTK_WIDGET(view);

      if(widget != NULL)
        g_value_set_object(value, widget);
      }
      break;
    case ZMAP_GUITV_MODEL:
      {
      GtkTreeModel *tree_model = NULL;

      tree_model = zMapGUITreeViewGetModel(zmap_tv);

      if(tree_model != NULL)
        g_value_set_object(value, tree_model);
      }
      break;
    case ZMAP_GUITV_DATA_PTR_COLUMN_INDEX:
      {
      int index;
      index = zMapGUITreeViewGetColumnIndexByName(zmap_tv, ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME);
      g_value_set_int(value, index);
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void zmap_guitreeview_dispose(GObject *object)
{
  ZMapGUITreeView zmap_tv;

  g_return_if_fail(ZMAP_IS_GUITREEVIEW(object));

  zmap_tv = ZMAP_GUITREEVIEW(object);

#ifdef DOING_THIS_RESULTS_IN_GLIB_CRITICAL_MESS
  /* The tree_view has a ref on the model. We unref the model first... */
  if(zmap_tv->tree_model)
    g_object_unref(zmap_tv->tree_model);
#endif

  zmap_tv->tree_model = NULL;

  if(zmap_tv->tree_view)
    g_object_unref(zmap_tv->tree_view);

  zmap_tv->tree_view = NULL;

  return ;
}

static void zmap_guitreeview_finalize(GObject *object)
{
  gboolean warn_on_finalize = FALSE;

  if(warn_on_finalize)
    zMapLogWarning("%s", "finalize...");

  return ;
}

static void zmap_guitreeview_simple_add(ZMapGUITreeView zmap_tv,
                              gpointer user_data)
{
  GtkListStore *store;
  GtkTreeIter iter;

  store = GTK_LIST_STORE(zmap_tv->tree_model);

  gtk_list_store_append(store, &iter);

  update_tuple_data(zmap_tv, store, &iter, TRUE, user_data);

  return ;
}

static void zmap_guitreeview_simple_add_values(ZMapGUITreeView zmap_tv,
                                     GList *values_list)
{
  GList *tmp;

  /* step through the list and set the values */
  if((tmp = values_list))
    {
      GtkTreeIter iter;
      GtkListStore *store;

      store = GTK_LIST_STORE(zmap_tv->tree_model);

      gtk_list_store_append(store, &iter);

      update_tuple_data_list(zmap_tv, store, &iter, TRUE, tmp);
    }

  return ;
}

static void zmap_guitreeview_add_list_of_tuples(ZMapGUITreeView zmap_tv,
                                    GList *tuples_list)
{
  zMapGUITreeViewPrepare(zmap_tv);

  g_list_foreach(tuples_list, each_tuple_swap_invoke_add, zmap_tv);

  zMapGUITreeViewAttach(zmap_tv);

  return ;
}

/* useful function to call add_tuple_simple from a list of tuples using g_list_foreach */
static void each_tuple_swap_invoke_add(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_simple)
    (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->add_tuple_simple)(zmap_tv, list_data);

  return ;
}


static void set_column_names(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(zmap_tv->curr_column < zmap_tv->column_count)
    {
      GQuark column_id;

      column_id = g_quark_from_string((char *)list_data);

      zmap_tv->column_names[zmap_tv->curr_column] = column_id;

      (zmap_tv->curr_column)++;
    }

  return ;
}

static void set_column_quarks(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(zmap_tv->curr_column < zmap_tv->column_count)
    {
      GQuark column_id;

      column_id = GPOINTER_TO_INT(list_data);

      zmap_tv->column_names[zmap_tv->curr_column] = column_id;

      (zmap_tv->curr_column)++;
    }

  return ;
}

static void set_column_types(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(zmap_tv->curr_column < zmap_tv->column_count)
    {
      GValue *value;
      GType type;

      type = GPOINTER_TO_INT(list_data);

      zmap_tv->column_types[zmap_tv->curr_column] = type;

      value = &(zmap_tv->column_values[zmap_tv->curr_column]);

      g_value_init(value, type);

      (zmap_tv->curr_column)++;
    }

  return ;
}

static void set_column_funcs(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(zmap_tv->curr_column < zmap_tv->column_count)
    {
      zmap_tv->column_funcs[zmap_tv->curr_column] = list_data;

      (zmap_tv->curr_column)++;
    }

  return ;
}

static void set_column_flags(gpointer list_data, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  if(zmap_tv->curr_column < zmap_tv->column_count)
    {
      ColumnFlagsStruct *flags;
      unsigned int user_flags = GPOINTER_TO_UINT(list_data);
      flags = &(zmap_tv->column_flags[zmap_tv->curr_column]);

      flags->bitflags        = user_flags;

      flags->named.visible   = (user_flags & ZMAP_GUITREEVIEW_COLUMN_VISIBLE   ? 1 : 0);
      flags->named.editable  = (user_flags & ZMAP_GUITREEVIEW_COLUMN_EDITABLE  ? 1 : 0);
      flags->named.clickable = (user_flags & ZMAP_GUITREEVIEW_COLUMN_CLICKABLE ? 1 : 0);


      (zmap_tv->curr_column)++;
    }

  return ;
}

static void tuple_no_to_model(GValue *value, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  g_value_set_int(value, ++(zmap_tv->curr_tuple));

  return ;
}

static void tuple_pointer_to_model(GValue *value, gpointer user_data)
{

  g_value_set_pointer(value, user_data);

  return ;
}

static void column_clicked_cb(GtkTreeViewColumn *column_clicked,
                        gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);
  GtkSortType sort_order;
  char *title;
  gboolean sort_indicator;

  g_object_get(G_OBJECT(column_clicked),
             "title",          &title,
             "sort-order",     &sort_order,
             "sort-indicator", &sort_indicator,
             NULL);

#ifdef DEBUGGING
  printf("Column '%s' clicked\n", title);
  printf("Column has '%s' indicator and a sort order of %s\n",
       (sort_indicator ? "an" : "no"),
       (sort_order == GTK_SORT_ASCENDING ? "ascending" : "descending"));
#endif

  if(zmap_tv->sortable)
    {
      GList *all_columns;

      if((all_columns = gtk_tree_view_get_columns(zmap_tv->tree_view)))
      {
        int index = -1, itr = 0;
        do
          {
            GtkTreeViewColumn *column;

            column = GTK_TREE_VIEW_COLUMN(all_columns->data);

            if(column == column_clicked)
              index = itr;
            else
            {
              g_object_set(G_OBJECT(column),
                         "sort-indicator", FALSE,
                         NULL);
            }
            itr++;
          }
        while((all_columns = g_list_next(all_columns)));

        if(index != -1)
          {
            GtkTreeModel *tree_model;

            tree_model = gtk_tree_view_get_model(zmap_tv->tree_view);

            if(tree_model != zmap_tv->tree_model)
            g_warning("%s", "Tree Models don't match");

            zmap_tv->sort_index = index;
            zmap_tv->sort_order = sort_order;

            gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(tree_model),
                                       zmap_tv->sort_index,
                                       zmap_tv->sort_order);

            /* There appears to be a small problem with the column indicator...
             * When ascending is set it shows a V arrow and descending a ^ arrow.
             * This might be me misunderstanding something about state of the
             * column and the view/model, but...
             * Anyway this ordering of calls seems to work and produce expected
             * behaviour.
             */
            zmap_tv->sort_order = (sort_order == GTK_SORT_ASCENDING ?
                             GTK_SORT_DESCENDING :
                             GTK_SORT_ASCENDING);

            g_object_set(G_OBJECT(column_clicked),
                     "sort-order",     zmap_tv->sort_order,
                     "sort-indicator", TRUE,
                     NULL);
          }
      }

    }

  return ;
}

static void view_add_column(GtkTreeView       *tree_view,
                      GQuark            *column_name_ptr,
                      GType             *column_type_ptr,
                      ColumnFlagsStruct *column_flags_ptr,
                      gpointer           clicked_cb_data)
{
  GtkTreeViewColumn *new_column;
  GtkCellRenderer *renderer;
  char *column_title;
  float aligned = 1.0;
  int column_index = -1;

  renderer = gtk_cell_renderer_text_new();

  if(column_type_ptr && *column_type_ptr && *column_type_ptr == G_TYPE_STRING)
    aligned = 0.0;

  g_object_set(G_OBJECT(renderer),
             "editable", column_flags_ptr->named.editable,
             "xalign",   aligned,
             NULL);

  column_title = (char *)(g_quark_to_string(*column_name_ptr));

  column_index =
    gtk_tree_view_insert_column_with_attributes(tree_view,
                                    column_index,
                                    column_title,
                                    renderer,
                                    NULL);
  new_column = gtk_tree_view_get_column(tree_view, column_index - 1);

  g_object_set(G_OBJECT(new_column),
             "clickable", column_flags_ptr->named.clickable,
             "visible",   column_flags_ptr->named.visible,
             NULL);

  if(column_flags_ptr->named.visible)
    gtk_tree_view_column_set_attributes(new_column,
                              renderer,
                              /* Get the text from the model */
                              "text",  column_index - 1,
                              NULL);

  if(column_flags_ptr->named.clickable)
    g_signal_connect(G_OBJECT(new_column), "clicked",
                 G_CALLBACK(column_clicked_cb), clicked_cb_data);

  return ;
}

static GtkTreeView *createView(ZMapGUITreeView zmap_tv)
{
  GtkTreeView *tree_view = NULL;
  GQuark *column_name_ptr;
  GType  *column_type_ptr;
  ColumnFlagsStruct *column_flags_ptr;

  if(zmap_tv->column_count > 0)
    {
      GtkTreeSelection *selection;
      int index;

      column_type_ptr  = zmap_tv->column_types;
      column_name_ptr  = zmap_tv->column_names;
      column_flags_ptr = zmap_tv->column_flags;

      tree_view = GTK_TREE_VIEW(gtk_tree_view_new());

      /* Create the behaviour you would expect from a tree view widget.
       * See more info around functions... */
      g_signal_connect(G_OBJECT(tree_view), "size-request",
                   G_CALLBACK(tree_view_size_request), zmap_tv);
      g_signal_connect(G_OBJECT(tree_view), "map",
                   G_CALLBACK(tree_view_map), zmap_tv);

      g_signal_connect(G_OBJECT(tree_view), "size-allocate",
                   G_CALLBACK(tree_view_size_allocation), zmap_tv);
      g_signal_connect(G_OBJECT(tree_view), "unmap",
                   G_CALLBACK(tree_view_unmap), zmap_tv);

      /* This should be a property */
      gtk_tree_view_set_headers_clickable(tree_view, TRUE);

      for(index = 0; index < zmap_tv->column_count; index++, column_name_ptr++, column_type_ptr++, column_flags_ptr++)
      {
        view_add_column(tree_view,
                    column_name_ptr,
                    column_type_ptr,
                    column_flags_ptr,
                    zmap_tv);
      }

      selection = gtk_tree_view_get_selection(tree_view);

      gtk_tree_selection_set_mode(selection, zmap_tv->select_mode);
    }
  else if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->init_layout &&
        !zmap_tv->init_layout_called)
    {
      (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->init_layout)(zmap_tv);
      zmap_tv->init_layout_called = TRUE;
      tree_view = createView(zmap_tv);
    }

  return tree_view;
}

static void row_deletion_cb(GtkTreeModel *tree_model,
                      GtkTreePath  *path,
                      gpointer      user_data)
{
#ifdef COMPILER_COMPLAINS
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);
#endif

  printf("row '%s' deleted\n", gtk_tree_path_to_string(path));

  return ;
}
/* Separate function in case we start making Different TreeModels other than ListStores */
static GtkTreeModel *createModel(ZMapGUITreeView zmap_tv)
{
  GtkTreeModel *model = NULL;
  GtkListStore *store;
  GType *types;

  if(zmap_tv->column_count > 0)
    {
      types = zmap_tv->column_types;

      store = gtk_list_store_newv(zmap_tv->column_count, types) ;

      model = GTK_TREE_MODEL(store);
    }
  else if(ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->init_layout &&
        !zmap_tv->init_layout_called)
    {
      (* ZMAP_GUITREEVIEW_GET_CLASS(zmap_tv)->init_layout)(zmap_tv);
      zmap_tv->init_layout_called = TRUE;
      model = createModel(zmap_tv);
    }

  if(model)
    {
      g_signal_connect(G_OBJECT(model), "row-deleted",
                   G_CALLBACK(row_deletion_cb), zmap_tv);
    }

  return model ;
}

static int column_index_from_name(ZMapGUITreeView zmap_tv, char *name)
{
  GQuark name_id, *names_ptr;
  int index, i;

  names_ptr = zmap_tv->column_names;
  name_id   = g_quark_from_string(name);

  for(i = 0; i < zmap_tv->column_count; i++, names_ptr++)
    {
      if(*names_ptr == name_id)
      index = i;
    }

  return index;
}

static void progressive_set_selection(ZMapGUITreeView zmap_tv)
{
  GtkTreeSelection *selection;

  if(zmap_tv->tree_view &&
     (selection = gtk_tree_view_get_selection(zmap_tv->tree_view)))
    {
      GtkTreeSelectionFunc select_func = zmap_tv->select_func;
      gpointer select_data             = zmap_tv->select_func_data;
      GDestroyNotify select_destroy    = zmap_tv->select_destroy;

      /* We don't want to set the selection function if we don't have a func */

      /* We also don't want to cause a destroy notify unwittingly, but users
       * might also expect that changing the func/data does do that...
       * Currently there is no checking in this area...
       */
      if(select_func)
      gtk_tree_selection_set_select_function(selection,
                                     select_func,
                                     select_data,
                                     select_destroy);
    }

  return ;
}

static void update_tuple_data(ZMapGUITreeView zmap_tv,
                        GtkListStore   *store,
                        GtkTreeIter    *iter,
                        gboolean        update_counter,
                        gpointer        user_data)
{
  int index = 0;

  if(zmap_tv->tuple_counter)
    {
      GValue *column_value;

      column_value = &(zmap_tv->column_values[index]);

      if(update_counter)
      {
        tuple_no_to_model(column_value, zmap_tv);

        gtk_list_store_set_value(store, iter, index, column_value);
      }

      index++;
    }

  if(zmap_tv->add_data_ptr)
    {
      GValue *column_value;

      column_value = &(zmap_tv->column_values[index]);

      tuple_pointer_to_model(column_value, user_data);

      gtk_list_store_set_value(store, iter, index, column_value);

      index++;
    }


  /* step through the functions and set the values */
  for(; index < zmap_tv->column_count; index++)
    {
      GValue *value;
      ZMapGUITreeViewCellFunc func;

      value = &(zmap_tv->column_values[index]);
      func  = zmap_tv->column_funcs[index];

      if(!func)
      g_warning("Failed to set value for column index %d.", index);
      else
      {
        g_value_reset(value);

        (func)(value, user_data);

        gtk_list_store_set_value(store, iter, index, value);
      }
    }

  return ;
}

static void update_tuple_data_list(ZMapGUITreeView zmap_tv,
                           GtkListStore   *store,
                           GtkTreeIter    *iter,
                           gboolean        update_counter,
                           GList          *values_list)
{
  GList *tmp;

  /* step through the list and set the values */
  if((tmp = values_list))
    {
      int index = 0;

      if(zmap_tv->tuple_counter)
      {
        GValue *column_value;

        column_value = &(zmap_tv->column_values[index]);

        if(update_counter)
          {
            tuple_no_to_model(column_value, zmap_tv);

            gtk_list_store_set_value(store, iter, index, column_value);
          }

        index++;
      }

      if(zmap_tv->add_data_ptr)
      {
        GValue *column_value;

        column_value = &(zmap_tv->column_values[index]);

        tuple_pointer_to_model(column_value, NULL);

        gtk_list_store_set_value(store, iter, index, column_value);

        index++;
      }

      do
      {
        GType column_type;
        GValue *column_value;

        column_type  = zmap_tv->column_types[index];
        column_value = &(zmap_tv->column_values[index]);

        g_value_reset(column_value);

        if(column_type == G_VALUE_TYPE(column_value))
          {
            if(column_type == G_TYPE_INT)
            g_value_set_int(column_value, GPOINTER_TO_INT(tmp->data));
            else if(column_type == G_TYPE_STRING)
            g_value_set_string(column_value, (char *)(tmp->data));
            else if(column_type == G_TYPE_BOOLEAN)
            g_value_set_boolean(column_value, GPOINTER_TO_INT(tmp->data));
            else if(column_type == G_TYPE_FLOAT)
            {
              int fint = GPOINTER_TO_INT(tmp->data);
              float ffloat;

              memcpy(&ffloat, &fint, 4); /* Let's hope float is 4  bytes */

              g_value_set_float(column_value, ffloat);
            }
            else if(column_type == G_TYPE_POINTER)
            {
              GtkTreeViewColumn *hide_column = NULL;
              hide_column = gtk_tree_view_get_column(zmap_tv->tree_view, index);
              g_object_set(G_OBJECT(hide_column),
                         "visible", FALSE,
                         NULL);
              g_value_set_pointer(column_value, tmp->data);
            }
            else
            zMapAssertNotReached(); /* v. unexpected! */
          }
        else
          g_warning("%s", "Bad column type");

        /* Send the values off to the store... */
        gtk_list_store_set_value(store, iter, index, column_value);
        /* Keep the index in step. */
        index++;
      }
      while((tmp = g_list_next(tmp)) && index < zmap_tv->column_count);
    }

  return ;
}


/*
 * The following 4 functions are to modify the sizing behaviour of the
 * GtkTreeView Widget.  By default it uses a very small size and
 * although it's possible to resize it, this is not useful for the
 * users who would definitely prefer _not_ to have to do this for
 * _every_ window that is opened.
 */
static void  tree_view_size_request(GtkWidget      *widget,
                            GtkRequisition *requisition,
                            gpointer        user_data)
{
  GdkScreen *screen;
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);
  gboolean has_screen;

  if((has_screen = gtk_widget_has_screen(widget)) &&
     (screen     = gtk_widget_get_screen(widget)) &&
     (zmap_tv->resized == 0) &&
     (zmap_tv->mapped  == 1))
    {
      GtkWidget *parent, *toplevel;
      int screen_width, screen_height;
      int size_req_width, size_req_height;
      double screen_percentage = 0.85;
      GtkRequisition new_requisition = {-1, -1};

      screen_width  = gdk_screen_get_width(screen);
      screen_height = gdk_screen_get_height(screen);

      screen_width  *= screen_percentage;
      screen_height *= screen_percentage;

      if((toplevel = zMapGUIFindTopLevel(widget)))
      {
        GtkAllocation toplevel_alloc;
        GtkRequisition expansion_requistion;

        /* Have to respect our toplevel size too... */
        toplevel_alloc = toplevel->allocation;

        /* We can't just expand to the full size of the requistion
         * as it will likely force some of the widget off the
         * screen. */

        expansion_requistion.width  = screen_width  - toplevel_alloc.width;
        expansion_requistion.height = screen_height - toplevel_alloc.height;

        /* This code is not tested for when there are 2 or more of
         * these widgets in a single toplevel where they are all
         * mapped at the same time. i.e. have the same direct parent
         * (e.g.a vbox) */
        screen_width  = zmap_tv->requisition.width  + expansion_requistion.width;
        screen_height = zmap_tv->requisition.height + expansion_requistion.height;
      }

      if(screen_width  < -1)
      screen_width  = -1;
      if(screen_height < -1)
      screen_height = -1;

      gtk_widget_get_size_request(widget, &size_req_width, &size_req_height);

      if(size_req_width == -1)
      new_requisition.width  = MIN(screen_width,  requisition->width);
      if(size_req_height == -1)
      new_requisition.height = MIN(screen_height, requisition->height);


      /* Get our parent and check if we have scrollbars eating our
       * tree view widget real estate */
      parent = gtk_widget_get_parent(widget);

      if(GTK_IS_SCROLLED_WINDOW(parent))
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

            v_requisition.width  += scrollbar_spacing;
            new_requisition.width = MIN((new_requisition.width + v_requisition.width), screen_width);
          }

        if((scrollbar = gtk_scrolled_window_get_hscrollbar(GTK_SCROLLED_WINDOW(parent))))
          {
            GtkRequisition h_requisition;
            int scrollbar_spacing = 0;

            gtk_widget_size_request(scrollbar, &h_requisition);

            gtk_widget_style_get(parent,
                           "scrollbar-spacing", &scrollbar_spacing,
                           NULL);

            h_requisition.height  += scrollbar_spacing;
            new_requisition.height = MIN((new_requisition.height + h_requisition.height), screen_height);
          }
      }

      /* Actually set the size */
      gtk_widget_set_size_request(widget,
                          new_requisition.width,
                          new_requisition.height);
      /* Note we've been resized */
      zmap_tv->resized = 1;
    }

  return ;
}

static gboolean tree_view_map(GtkWidget *widget, GdkEvent  *event, gpointer user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  /* Set the flags we need for resizing and allocation recording... */
  zmap_tv->mapped  = 1;
  zmap_tv->resized = 0;

  /* Ensure we get a size-request */
  gtk_widget_set_size_request(widget, -1, -1);

  return FALSE;
}

static gboolean tree_view_unmap(GtkWidget *widget,
                        gpointer   user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  /* Set the flags we need for resizing and allocation recording... */
  zmap_tv->mapped  = 0;
  zmap_tv->resized = 0;

  /* Set this size so the widget can be resized back to it's original
   * dimensions. See GUINotebook behaviour... */

  /* Need to call this so the map call does actually cause a size-request */
  gtk_widget_set_size_request(widget, 0, 0);

  return FALSE;
}

static void tree_view_size_allocation(GtkWidget     *widget,
                              GtkAllocation *allocation,
                              gpointer       user_data)
{
  ZMapGUITreeView zmap_tv = ZMAP_GUITREEVIEW(user_data);

  /* If we've not resized, just record allocation width & height */
  if((!zmap_tv->resized))
    {
      /* We record this so that we don't limit the minimum size to
       * something smaller than default. */

      zmap_tv->requisition.width  = allocation->width;
      zmap_tv->requisition.height = allocation->height;
    }
  else
    gtk_widget_set_size_request(widget, -1, -1);
  /* Else the user is resizing, we just default size request. */

  return ;
}
