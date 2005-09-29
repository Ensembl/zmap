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
 * Last edited: Sep 29 13:23 2005 (rds)
 * Created: Tue Sep 27 13:06:09 2005 (rds)
 * CVS info:   $Id: zmapWindowFeatureList.c,v 1.2 2005-09-29 13:21:23 rds Exp $
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
static void addDataToStore(GQuark key_id, 
                           gpointer data, 
                           gpointer user_data);
static gint sortByFunc (GtkTreeModel *model,
                        GtkTreeIter  *a,
                        GtkTreeIter  *b,
                        gpointer      userdata);
static char *featureLookUpEnum(int id, int enumType);

static void addFeatureItemToStore(GtkTreeModel *treeModel, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item);

/* ======================================================= */
/* Functions */

GtkTreeModel *zmapWindowFeatureListCreateStore(void)
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

  if(USE_TREE_STORE)
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
                                           zmapWindowFeatureListCallbacks callbacks,
                                           gpointer user_data)
{
  GtkWidget    *treeView;
  GtkTreeSelection  *selection;
  GtkCellRenderer   *renderer;
  int colNo = 0;
  char *column_titles[ZMAP_WINDOW_LIST_COL_NUMBER] = {"Name", "Type", "Strand", "Start", "End", "Phase", "Score", "Method", "FEATURE_ITEM", "","",""};

  /* Create the view from the model */
  treeView = gtk_tree_view_new_with_model(treeModel);

  /* A renderer for the columns. */
  renderer = gtk_cell_renderer_text_new ();
  /* Add it to all of them, not sure we need to add it to all, just the visible ones... */
  for(colNo = 0; colNo < ZMAP_WINDOW_LIST_COL_NUMBER; colNo++)
    {
      GtkTreeViewColumn *column = NULL;
      column = gtk_tree_view_column_new_with_attributes(column_titles[colNo],
                                                        renderer,
                                                        "text", colNo,
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
  gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
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

void zmapWindowFeatureListPopulateStoreDataList(GtkTreeModel *treeModel,
                                                ZMapWindow window,
                                                ZMapStrand strand,
                                                GData *featureSet)
{
  strandWindowModelStruct winListModel = {ZMAPSTRAND_NONE, NULL, NULL};
  winListModel.filterStrand = strand;
  winListModel.zmapWindow   = window;
  winListModel.model        = treeModel;
  /* This needs setting here so we can draw it later. */
  g_datalist_foreach(&featureSet, addDataToStore, &winListModel) ;
  return ;
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

      addFeatureItemToStore(treeModel, feature, item);

      list = list->next;
    }
  return ;
}


/* ======================================================= */
/* INTERNALS */

/** \Brief Load an item into the list.
 *
 * Loads the list, ensuring separation of forward and reverse strand features.
 * The x coordinate is in canvasData, but is only guaranteed while no other
 * column has its own list, so we put it into the list for this column now.
 */
static void addDataToStore(GQuark key_id, 
                           gpointer data, 
                           gpointer user_data)
{
  ZMapFeature feature   = (ZMapFeature)data ;
  strandWindowModel swm = (strandWindowModel)user_data;

#define STRAND_DOESNT_ADD_ANYTHING
#ifdef STRAND_DOESNT_ADD_ANYTHING
  /* Only do our current strand. As ZMAPSTRAND_FORWARD = 1 &&
   * ZMAPSTRAND_REVERSE = 2 it's conceiveable we could show both and
   * bit and here */
  if (swm->filterStrand & feature->strand 
      || (feature->strand == 0 && swm->filterStrand != ZMAPSTRAND_REVERSE)) 
    /* Annoying this is here as ZMAP_STRAD_NONE = 0 and the window
     * sets the strand of the column feature set to be forward! Doh!
     * and this still isn't right! */
    {
#endif /* STRAND_DOESNT_ADD_ANYTHING */
      FooCanvasItem *item ;

      item = zmapWindowFToIFindFeatureItem(swm->zmapWindow->context_to_item, feature) ;
      //zMapAssert(item) ;
      if(item)
        addFeatureItemToStore(swm->model, feature, item);
#ifdef STRAND_DOESNT_ADD_ANYTHING
    }
#endif /* STRAND_DOESNT_ADD_ANYTHING */
  return ;
}

static void addFeatureItemToStore(GtkTreeModel *treeModel, 
                                  ZMapFeature feature, 
                                  FooCanvasItem *item)
{
  GtkTreeIter iter1;

  zMapAssert(feature && item);

  if(USE_TREE_STORE)
    {
      GtkTreeStore *store = NULL;
      store = GTK_TREE_STORE(treeModel);
      gtk_tree_store_append(store, &iter1, NULL);
      gtk_tree_store_set(store, &iter1, 
                         ZMAP_WINDOW_LIST_COL_NAME,   (char *)g_quark_to_string(feature->original_id),
                         ZMAP_WINDOW_LIST_COL_STRAND, featureLookUpEnum(feature->strand, STRAND_ENUM),
                         ZMAP_WINDOW_LIST_COL_START,  feature->x1,
                         ZMAP_WINDOW_LIST_COL_END,    feature->x2,
                         ZMAP_WINDOW_LIST_COL_TYPE,   featureLookUpEnum(feature->type, TYPE_ENUM),
                         ZMAP_WINDOW_LIST_COL_PHASE,  featureLookUpEnum(feature->phase, PHASE_ENUM),
                         ZMAP_WINDOW_LIST_COL_SCORE,  feature->score,
                         ZMAP_WINDOW_LIST_COL_FEATURE_TYPE, zMapStyleGetName(zMapFeatureGetStyle(feature)),
                         ZMAP_WINDOW_LIST_COL_FEATURE_ITEM, item,
                         -1) ;
    }
  else
    {
      GtkListStore *store = NULL;
      store = GTK_LIST_STORE(treeModel);
      gtk_list_store_append(store, &iter1);
      gtk_list_store_set(store, &iter1, 
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
