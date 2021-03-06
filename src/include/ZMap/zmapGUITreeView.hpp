/*  File: zmapGUITreeView.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_GUITREEVIEW_H__
#define __ZMAP_GUITREEVIEW_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>


#define ZMAP_GUITREEVIEW_COUNTER_COLUMN_NAME "#"
#define ZMAP_GUITREEVIEW_DATA_PTR_COLUMN_NAME "-DATA-PTR-"

/*
 * Type checking and casting macros
 */
#define ZMAP_TYPE_GUITREEVIEW         (zMapGUITreeViewGetType())
#define ZMAP_GUITREEVIEW(obj)         G_TYPE_CHECK_INSTANCE_CAST((obj), zMapGUITreeViewGetType(), zmapGUITreeView)
#define ZMAP_GUITREEVIEW_CONST(obj) G_TYPE_CHECK_INSTANCE_CAST((obj), zMapGUITreeViewGetType(), zmapGUITreeView const)
#define ZMAP_GUITREEVIEW_CLASS(klass)     G_TYPE_CHECK_CLASS_CAST((klass),  zMapGUITreeViewGetType(), zmapGUITreeViewClass)
#define ZMAP_IS_GUITREEVIEW(obj)    G_TYPE_CHECK_INSTANCE_TYPE((obj), zMapGUITreeViewGetType())
#define ZMAP_GUITREEVIEW_GET_CLASS(obj)   G_TYPE_INSTANCE_GET_CLASS((obj),  zMapGUITreeViewGetType(), zmapGUITreeViewClass)

typedef enum
  {
    ZMAP_GUITREEVIEW_COLUMN_NOTHING   = 0,
    ZMAP_GUITREEVIEW_COLUMN_VISIBLE   = 1 << 0,
    ZMAP_GUITREEVIEW_COLUMN_EDITABLE  = 1 << 1,
    ZMAP_GUITREEVIEW_COLUMN_CLICKABLE = 1 << 2,

    ZMAP_GUITREEVIEW_COLUMN_INVALID   = 1 << 3,
  } ZMapGUITreeViewColumnFlags;

typedef void (* ZMapGUITreeViewCellFunc)(GValue *value, gpointer user_data);

typedef void (* ZMapGUITreeViewCellFreeFunc)(gpointer user_data);

/*
 * Main object structure
 */
typedef struct _zmapGUITreeViewStruct *ZMapGUITreeView;

typedef struct _zmapGUITreeViewStruct  zmapGUITreeView;


/*
 * Class definition
 */
typedef struct _zmapGUITreeViewClassStruct *ZMapGUITreeViewClass;

typedef struct _zmapGUITreeViewClassStruct zmapGUITreeViewClass;

/*
 * Public methods
 */
GType zMapGUITreeViewGetType (void);
/*!
 * \brief create a new ZMapGUITreeView Object
 */
ZMapGUITreeView zMapGUITreeViewCreate(void);

/*!
 * \brief obtain the GtkTreeView the ZMapGUITreeView owns.
 *        This will create the view if it doesn't exist.
 */
GtkTreeView *zMapGUITreeViewGetView(ZMapGUITreeView zmap_tv);

/*!
 * \brief obtain the GtkTreeModel the ZMapGUITreeView owns.
 *        This will create the model if it doesn't exist.
 */
GtkTreeModel *zMapGUITreeViewGetModel(ZMapGUITreeView zmap_tv);

/*!
 * \brief Prepare the Model, creating it, and the view and ensures the
 * model and the view are not attached.  This means the model is ready
 * for multiple inserts.
 */
gboolean zMapGUITreeViewPrepare(ZMapGUITreeView zmap_tv);

/*!
 * \brief Attach the view and the model and set the sort column...
 */
gboolean zMapGUITreeViewAttach(ZMapGUITreeView zmap_tv);

/*!
 * \brief add a row to the model.
 */
void zMapGUITreeViewAddTuple(ZMapGUITreeView zmap_tv, gpointer user_data);

/*!
 * \brief add a row to the model.
 */
void zMapGUITreeViewAddTupleFromColumnData(ZMapGUITreeView zmap_tv,
                                 GList *values_list);
/*!
 * \brief add rows to the model
 */
void zMapGUITreeViewAddTuples(ZMapGUITreeView zmap_tv, GList *tuples_list);
/*!
 * \brief get the index for a column by name
 */
int zMapGUITreeViewGetColumnIndexByName(ZMapGUITreeView zmap_tv, const char *column_name);

/*!
 * \brief update the row with the iterator from the data.
 */
void zMapGUITreeViewUpdateTuple(ZMapGUITreeView zmap_tv, GtkTreeIter *iter, gpointer user_data);

/*!
 * \brief free up everything...
 */
ZMapGUITreeView zMapGUITreeViewDestroy(ZMapGUITreeView zmap_tv);

#endif /* __ZMAP_GUITREEVIEW_H__ */
