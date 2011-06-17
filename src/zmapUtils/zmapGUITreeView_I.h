/*  File: zmapGUITreeView_I.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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

#ifndef __ZMAP_GUITREEVIEW_I_H__
#define __ZMAP_GUITREEVIEW_I_H__

#include <ZMap/zmapGUITreeView.h>


#define DEFAULT_COLUMN_FLAGS (ZMAP_GUITREEVIEW_COLUMN_VISIBLE | ZMAP_GUITREEVIEW_COLUMN_CLICKABLE)
/*
 * Type checking and casting macros
 */

/* In public header... */
/* Make sure this stays in step with the ZMapGUITreeViewColumnFlags enum in the public header */
typedef union
{
  unsigned int bitflags;
  struct
  {
    unsigned int visible   : 1;
    unsigned int editable  : 1;
    unsigned int clickable : 1;
  }named;
} ColumnFlagsStruct;

/*
 * Main object structure
 */

typedef struct _zmapGUITreeViewStruct
{
  GObject __parent__;

  unsigned int tuple_counter : 1; /* First column is a counter column
                           * giving each row an id in the
                           * model */
  unsigned int sortable : 1;
  unsigned int init_layout_called : 1;
  unsigned int add_data_ptr : 1;
  unsigned int resized : 1;   /* Flag to record/control resizing the tree_view */
  unsigned int mapped : 1;    /* Flag to control resizing */
  unsigned int sort_index;
  unsigned int column_count;
  unsigned int curr_column;
  unsigned int curr_tuple;    /* For filling the first column... */

  GQuark   *column_names;
  GType    *column_types;
  GValue   *column_values;
  int      *column_numbers;
  ColumnFlagsStruct *column_flags;

  ZMapGUITreeViewCellFunc *column_funcs;

  GtkTreeView         *tree_view;
  GtkTreeModel        *tree_model;
  GtkSelectionMode     select_mode;
  GtkTreeSelectionFunc select_func;
  gpointer             select_func_data;
  GDestroyNotify       select_destroy;
  GtkRequisition       requisition;

  GtkSortType            sort_order;
  GtkTreeIterCompareFunc sort_func;
  gpointer               sort_func_data;
  GDestroyNotify         sort_destroy;
} zmapGUITreeViewStruct;


/*
 * Class definition
 */

typedef struct _zmapGUITreeViewClassStruct {
  GObjectClass __parent__;

  /* init_layout isn't completely implemented currently. expect odd behaviour. */
  void (* init_layout)(ZMapGUITreeView zmap_tv);

  void (* add_tuple_simple)(ZMapGUITreeView zmap_tv, gpointer user_data);

  void (* add_tuple_value_list)(ZMapGUITreeView zmap_tv, GList *list_of_user_data);

  void (* add_tuples)(ZMapGUITreeView zmap_tv, GList *list_of_user_data);
} zmapGUITreeViewClassStruct;


/*
 * Public methods
 */

/* In public header... */


#endif /* __ZMAP_GUITREEVIEW_I_H__ */
