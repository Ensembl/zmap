/*  File: zmapWindowFeatureList.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Oct  6 16:15 2005 (rds)
 * Created: Tue Sep 27 13:06:09 2005 (rds)
 * CVS info:   $Id: zmapWindowFeatureList.c,v 1.4 2005-10-07 10:56:16 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindow_P.h>
#include <ZMap/zmapUtils.h>

enum { USE_TREE_STORE = 0 };

/* data for the g_datalist_foreach() function addFeatureToStore */
typedef struct _strandWindowModelStruct
{
  ZMapStrand filterStrand;
  ZMapWindow   zmapWindow;
  GtkTreeModel     *model;
} strandWindowModelStruct, *strandWindowModel;


/* ======================================================= */
/* INTERNAL prototypes. */
static gint sortByFunc (GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      userdata);
static char *featureLookUpEnum(int id, int enumType);

static void addFeatureItemToStore(GtkTreeModel *treeModel, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item,
                                  GtkTreeIter *parent);

/* ======================================================= */
/* Functions */

GtkTreeModel *zmapWindowFeatureListCreateStore(gboolean use_tree_store)
{
  GtkTreeSortable *sortable = NULL;
  GtkTreeModel *treeModel   = NULL;
  int colNo = 0;
  /* Not sure on the sanity of this.  It's going to make the order
   * look a bit odd maybe. Forward is logically before Reverse and 
   * 1 before 2, but for feature type this isn't going to be the 
   * case. Oh well it's really easy to change*/
  int columnSortIDs[ZMAP_WINDOW_LIST_COL_NUMBER] = { 
    ZMAP_WINDOW_LIST_COL_NAME,        ZMAP_WINDOW_LIST_COL_SORT_TYPE,
    ZMAP_WINDOW_LIST_COL_SORT_STRAND, ZMAP_WINDOW_LIST_COL_START,
    ZMAP_WINDOW_LIST_COL_END,         ZMAP_WINDOW_LIST_COL_SORT_PHASE,
    ZMAP_WINDOW_LIST_COL_SCORE,       ZMAP_WINDOW_LIST_COL_FEATURE_TYPE,
    -1, -1, -1, -1
  };

  if(use_tree_store)
    {
      GtkTreeStore *store = NULL;
      store = gtk_tree_store_new(ZMAP_WINDOW_LIST_COL_NUMBER,
                                 G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                 G_TYPE_INT,    G_TYPE_INT,    G_TYPE_STRING,
                                 G_TYPE_FLOAT,  G_TYPE_STRING, G_TYPE_POINTER,
                                 G_TYPE_INT,    G_TYPE_INT,    G_TYPE_INT
                                 );
      treeModel = GTK_TREE_MODEL(store);
    }
  else
    {
      GtkListStore *list = NULL;
      list = gtk_list_store_new(ZMAP_WINDOW_LIST_COL_NUMBER,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                G_TYPE_INT,    G_TYPE_INT,    G_TYPE_STRING,
                                G_TYPE_FLOAT,  G_TYPE_STRING, G_TYPE_POINTER,
                                G_TYPE_INT,    G_TYPE_INT,    G_TYPE_INT
                                );
      treeModel = GTK_TREE_MODEL(list);
    }
  sortable  = GTK_TREE_SORTABLE(treeModel);

  for(colNo = 0; colNo < ZMAP_WINDOW_LIST_COL_NUMBER; colNo++)
    {  
      gtk_tree_sortable_set_sort_func(sortable, 
                                      colNo, 
                                      sortByFunc, 
                                      GINT_TO_POINTER(columnSortIDs[colNo]), 
                                      NULL);
    }

  return treeModel;
}


GtkWidget *zmapWindowFeatureListCreateView(GtkTreeModel *treeModel,
                                           GtkCellRenderer *renderer,
                                           zmapWindowFeatureListCallbacks callbacks,
                                           gpointer user_data)
{
  GtkWidget    *treeView;
  GtkTreeSelection  *selection;
  int colNo = 0;
  char *column_titles[ZMAP_WINDOW_LIST_COL_NUMBER] = {"Name", "Type", "Strand", 
                                                      "Start", "End", "Phase",
                                                      "Score", "Method", "FEATURE_ITEM",
                                                      "","",""};

  /* Create the view from the model */
  treeView = gtk_tree_view_new_with_model(treeModel);

  /* A renderer for the columns. */
  if(renderer == NULL)
    renderer = gtk_cell_renderer_text_new ();

  /* Add it to all of them, not sure we need to add it to all, just the visible ones... */
  for(colNo = 0; colNo < ZMAP_WINDOW_LIST_COL_NUMBER; colNo++)
    {
      GtkTreeViewColumn *column = NULL;
      column = gtk_tree_view_column_new_with_attributes(column_titles[colNo],
                                                        renderer,
                                                        "text", colNo,
                                                        NULL);
      g_object_set_data_full(G_OBJECT(column),
                             ZMAP_WINDOW_FEATURE_LIST_COL_NUMBER_KEY, 
                             GINT_TO_POINTER(colNo),
                             NULL);
      /* The order of the next two calls IS IMPORTANT.  With the
       * signal_connect FIRST the callback is called BEFORE the
       * sorting happens. A user can then manipulate how the sort is
       * done, but the other way round the sorting is done FIRST and
       * the callback is useless to effect the sort without needlessly
       * sorting again!! This gives me a headache.
       */
      if(callbacks->columnClickedCB)
        g_signal_connect(G_OBJECT(column), "clicked",
                         G_CALLBACK(callbacks->columnClickedCB), 
                         user_data);

      /* set the pointer and data rows to be invisible */
      if(colNo >= ZMAP_WINDOW_LIST_COL_FEATURE_ITEM)
        gtk_tree_view_column_set_visible(column, FALSE);
      else
        gtk_tree_view_column_set_sort_column_id(column, colNo);

      /* Add the column to the view now it's all set up */
      gtk_tree_view_append_column (GTK_TREE_VIEW (treeView), column);
    }

  //  gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(treeView), TRUE);

  /* Setup the selection handler */
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeView));
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);
  if(callbacks->selectionFuncCB)
    gtk_tree_selection_set_select_function(selection, 
                                           callbacks->selectionFuncCB, 
                                           user_data, NULL);

  /* Allow users to edit from this list... */
  if(callbacks->rowActivatedCB)
    g_signal_connect(G_OBJECT(treeView), "row-activated", 
                     G_CALLBACK(callbacks->rowActivatedCB),
                     user_data);

  return treeView;
}

void zmapWindowFeatureListPopulateStoreList(GtkTreeModel *treeModel,
                                            GList *list)
{
  while(list)
    {
      FooCanvasItem *item = NULL;
      ZMapFeature feature = NULL;

      item    = (FooCanvasItem *)list->data;
      /* do we need some kind of type check on feature here? */
      feature = g_object_get_data(G_OBJECT(item), "item_feature_data");

      addFeatureItemToStore(treeModel, feature, item, NULL);

      list = list->next;
    }
  return ;
}
static void selection_counter(GtkTreeModel *model,
                              GtkTreePath  *path,
                              GtkTreeIter  *iter,
                              gpointer      userdata)
{
  gint *p_count = (gint*) userdata;
  g_assert (p_count != NULL);
  (*p_count)++;
  return ;
}

gint zmapWindowFeatureListCountSelected(GtkTreeSelection *selection)
{
  gint count = 0;
  gtk_tree_selection_selected_foreach(selection, 
                                      selection_counter, 
                                      &count);
  return count;
}
/* Returns column number or -1 if not found or on error */

gint zmapWindowFeatureListGetColNumberFromTVC(GtkTreeViewColumn *col)
{
  GList *cols;
  gint   num = -1;

  g_return_val_if_fail ( col != NULL, num );
  g_return_val_if_fail ( col->tree_view != NULL, num );

  /* Returns -1 if tree view is reorderable. Use g_object_{s,g}et_data functions */
  if(gtk_tree_view_get_reorderable(GTK_TREE_VIEW(col->tree_view)))
     return num;

  cols = gtk_tree_view_get_columns(GTK_TREE_VIEW(col->tree_view));

  num = g_list_index(cols, (gpointer) col);

  g_list_free(cols);

  return num;
}


/* ======================================================= */
/* INTERNALS */

static void addFeatureItemToStore(GtkTreeModel *treeModel, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item,
                                  GtkTreeIter *parent)
{
  GtkTreeIter append;

  zMapAssert(feature && item);

  if(GTK_IS_TREE_STORE(treeModel))
    {
      GtkTreeStore *store = NULL;
      gboolean descend    = TRUE, appended = FALSE;
      ZMapWindowItemFeatureType type;

      store = GTK_TREE_STORE(treeModel);
      type  = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type"));

      switch(type)
        { 
        case ITEM_FEATURE_SIMPLE: 
        case ITEM_FEATURE_PARENT: 
          gtk_tree_store_append(store, &append, parent);
          appended = TRUE;
          gtk_tree_store_set(store, &append, 
                             ZMAP_WINDOW_LIST_COL_NAME,   (char *)g_quark_to_string(feature->original_id),
                             ZMAP_WINDOW_LIST_COL_START,  feature->x1,
                             ZMAP_WINDOW_LIST_COL_END,    feature->x2,
                             ZMAP_WINDOW_LIST_COL_SCORE,  feature->score,
                             -1);
          if(descend && FOO_IS_CANVAS_GROUP(item))
            {
              GList *children     = NULL;
              FooCanvasGroup *grp = FOO_CANVAS_GROUP(item);
              children = grp->item_list;
              while (children)
                {
                  FooCanvasItem *childItem = (FooCanvasItem *)children->data;
                  addFeatureItemToStore(treeModel, feature, childItem, &append);
                  children = children->next;
                }
            }
          break;
        case ITEM_FEATURE_CHILD: 
          {
            ZMapWindowItemFeature subfeature;
            subfeature = (ZMapWindowItemFeature)g_object_get_data(G_OBJECT(item),
                                                                  "item_subfeature_data") ;
            gtk_tree_store_append(store, &append, parent);
            appended = TRUE;
            gtk_tree_store_set(store, &append,
                               ZMAP_WINDOW_LIST_COL_START, subfeature->start,
                               ZMAP_WINDOW_LIST_COL_END,   subfeature->end,
                               -1);
          }
          break;
        case ITEM_FEATURE_BOUNDING_BOX: 
        default: 
          break;
        }
      
      if(appended)
        gtk_tree_store_set(store, &append, 
                           ZMAP_WINDOW_LIST_COL_STRAND, featureLookUpEnum(feature->strand, STRAND_ENUM),
                           ZMAP_WINDOW_LIST_COL_TYPE,   featureLookUpEnum(feature->type, TYPE_ENUM),
                           ZMAP_WINDOW_LIST_COL_PHASE,  featureLookUpEnum(feature->phase, PHASE_ENUM),
                           ZMAP_WINDOW_LIST_COL_FEATURE_TYPE, zMapStyleGetName(zMapFeatureGetStyle(feature)),
                           ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, item,
                           -1) ;


    }
  else if(GTK_IS_LIST_STORE(treeModel))
    {
      GtkListStore *store = NULL;
      store = GTK_LIST_STORE(treeModel);
      gtk_list_store_append(store, &append);
      gtk_list_store_set(store, &append, 
                         ZMAP_WINDOW_LIST_COL_NAME,   (char *)g_quark_to_string(feature->original_id),
                         ZMAP_WINDOW_LIST_COL_STRAND, featureLookUpEnum(feature->strand, STRAND_ENUM),
                         ZMAP_WINDOW_LIST_COL_START,  feature->x1,
                         ZMAP_WINDOW_LIST_COL_END,    feature->x2,
                         ZMAP_WINDOW_LIST_COL_TYPE,   featureLookUpEnum(feature->type, TYPE_ENUM),
                         ZMAP_WINDOW_LIST_COL_PHASE,  featureLookUpEnum(feature->phase, PHASE_ENUM),
                         ZMAP_WINDOW_LIST_COL_SCORE,  feature->score,
                         ZMAP_WINDOW_LIST_COL_FEATURE_TYPE, zMapStyleGetName(zMapFeatureGetStyle(feature)),
                         ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, item,
                         ZMAP_WINDOW_LIST_COL_SORT_PHASE,   feature->phase,
                         ZMAP_WINDOW_LIST_COL_SORT_TYPE,    feature->type,
                         ZMAP_WINDOW_LIST_COL_SORT_STRAND,  feature->strand,
                         -1) ;
    }
  return ;
}
/** \Brief To sort the column  
 */
static gint sortByFunc (GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      userdata)
{
  gint answer = 0;
  gint col    = GPOINTER_TO_INT(userdata);
  /* remember to g_free() any strings! */
  switch(col)
    {
    case ZMAP_WINDOW_LIST_COL_SCORE:
      {
        float endA, endB;
        gtk_tree_model_get(model, a, col, &endA, -1);
        gtk_tree_model_get(model, b, col, &endB, -1);
        answer = ( (endA == endB) ? 0 : ((endA < endB) ? -1 : 1 ) );
      }
      break;
    case ZMAP_WINDOW_LIST_COL_END:
    case ZMAP_WINDOW_LIST_COL_START:
    case ZMAP_WINDOW_LIST_COL_SORT_PHASE:
    case ZMAP_WINDOW_LIST_COL_SORT_TYPE:
    case ZMAP_WINDOW_LIST_COL_SORT_STRAND:
      {
        int rowA, rowB;
        
        gtk_tree_model_get(model, a, col, &rowA, -1);
        gtk_tree_model_get(model, b, col, &rowB, -1);
        answer = ( (rowA == rowB) ? 0 : ((rowA < rowB) ? -1 : 1 ) );
      }
      break;
    case ZMAP_WINDOW_LIST_COL_FEATURE_TYPE:
    case ZMAP_WINDOW_LIST_COL_NAME:
      /* These need writing! */
      {
        gchar *row1, *row2;

        gtk_tree_model_get(model, a, col, &row1, -1);
        gtk_tree_model_get(model, b, col, &row2, -1);

        if (row1 == NULL || row2 == NULL)
        {
          if (!(row1 == NULL && row2 == NULL))
            answer = (row1 == NULL) ? -1 : 1;
        }
        else
        {
          answer = g_utf8_collate(row1,row2);
        }
        if(row1)
          g_free(row1);
        if(row2)
          g_free(row2);
      }
      break;
    case ZMAP_WINDOW_LIST_COL_TYPE:
    case ZMAP_WINDOW_LIST_COL_STRAND:
    case ZMAP_WINDOW_LIST_COL_PHASE:
      answer = 0;
      zMapAssert(answer);
      break;
    default:
      g_print("Column isn't sortable.\n");
      break;
    }
  return answer;
}

static char *featureLookUpEnum(int id, int enumType)
{
  /* These arrays must correspond 1:1 with the enums declared in zmapFeature.h */
  static char *types[]   = {"Basic", "Homology", "Exon", "Intron", "Transcript",
			    "Variation", "Boundary", "Sequence"} ;
  static char *strands[] = {".", "Forward", "Reverse" } ;
  static char *phases[]  = {"NONE", "0", "1", "2" } ;
  static char *homolTypes[] = {"dna", "protein", "translated"} ;
  char *enum_str = NULL ;

  zMapAssert(enumType == TYPE_ENUM || enumType == STRAND_ENUM 
	     || enumType == PHASE_ENUM || enumType == HOMOLTYPE_ENUM) ;

  switch (enumType)
    {
    case TYPE_ENUM:
      enum_str = types[id];
      break;
      
    case STRAND_ENUM:
      enum_str = strands[id];
      break;
      
    case PHASE_ENUM:
      enum_str = phases[id];
      break;

    case HOMOLTYPE_ENUM:
      enum_str = homolTypes[id];
      break;
    default:
      break;
    }
  
  return enum_str ;
}
