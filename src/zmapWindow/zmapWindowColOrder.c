/*  File: zmapWindowColOrder.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
 *  Re-factored by malcolm hinsley
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






#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainers.h>


typedef struct
{
  ZMapWindow window;            /* The window */

  ZMapStrand strand;            /* which strand group we're sorting...
                                 * direction of window->feature_set_names */
//  GList *names_list;
  GHashTable *names_hash;      // featureset unique id -> position
  int three_frame_position;
  int n_names;
} OrderColumnsDataStruct, *OrderColumnsData;

static void orderPositionColumns(ZMapWindow window, gboolean redraw_too);
static void orderColumnsCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data);
static gint qsortColumnsCB(gconstpointer colA, gconstpointer colB, gpointer user_data);
static gboolean isFrameSensitive(gconstpointer col_data);
//static int columnFSNListPosition(gconstpointer col_data, GList *feature_set_names);



static gboolean order_debug_G = FALSE;


/* void zmapWindowColOrderColumns(ZMapWindow window)
 *
 * Ordering the columns in line with the contents of
 * window->feature_set_names. The foocanvas
 */
void zmapWindowColOrderColumns(ZMapWindow window)
{
  return orderPositionColumns(window, FALSE);
}

void zmapWindowColOrderPositionColumns(ZMapWindow window)
{
  return orderPositionColumns(window, TRUE);
}

/* INTERNALS */

static void orderPositionColumns(ZMapWindow window, gboolean redraw_too)
{
  OrderColumnsDataStruct order_data = {NULL} ;
  GList *names;
  guint i;
  GQuark threeframe;

  order_data.window = window;
  order_data.strand = ZMAPSTRAND_FORWARD; /* makes things simpler */

  if(order_debug_G)
    printf("%s: starting column ordering\n", __PRETTY_FUNCTION__);

      // make a hash table of positions for easy lookup
  order_data.names_hash = g_hash_table_new(NULL,NULL);
  threeframe = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME);

  for(names = window->feature_set_names,i = 1;names;names = names->next,i++)  // start at 1 to let us catch 0 == not found
  {
      GQuark id = zMapFeatureSetCreateID((char *) g_quark_to_string(GPOINTER_TO_UINT(names->data)));

      g_hash_table_insert(order_data.names_hash,GUINT_TO_POINTER(id), GUINT_TO_POINTER(i));
      if(id == threeframe)
            order_data.three_frame_position = i;

      if(order_debug_G)
            printf("WFSN %s = %d\n",g_quark_to_string(GPOINTER_TO_UINT(names->data)),i);
  }
  // empty columns to the end as we can't find them
  g_hash_table_insert(order_data.names_hash,GUINT_TO_POINTER(0), GUINT_TO_POINTER(i));
  order_data.n_names = i;


  if(!order_data.three_frame_position)
      order_data.three_frame_position = i;           // off the end


  zmapWindowContainerUtilsExecuteFull(window->feature_root_group,
				      ZMAPCONTAINER_LEVEL_STRAND,
				      orderColumnsCB, &order_data,
				      NULL, NULL, redraw_too);

  /* If we've reversed the feature_set_names list, and left it like that, re-reverse. */
  if(order_data.strand == ZMAPSTRAND_REVERSE)
    window->feature_set_names = g_list_reverse(window->feature_set_names);

  g_hash_table_destroy(order_data.names_hash);

  if(order_debug_G)
    printf("%s: columns should now be in order\n", __PRETTY_FUNCTION__);

  return ;

}

static void orderColumnsCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                           ZMapContainerLevelType level, gpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *strand_features_group;
  ZMapStrand strand;
  ZMapWindow window = order_data->window;

  if(level == ZMAPCONTAINER_LEVEL_STRAND)
    {
      /* Get Features */
      container_features = zmapWindowContainerGetFeatures(container);
      zMapAssert(container_features);
      /* Cast to FooCanvasGroup */
      strand_features_group = (FooCanvasGroup *)container_features;

      strand = zmapWindowContainerGetStrand(container);

      if(strand == ZMAPSTRAND_REVERSE)
        {
          if(order_data->strand == ZMAPSTRAND_FORWARD)
            window->feature_set_names =
              g_list_reverse(window->feature_set_names);
          order_data->strand = ZMAPSTRAND_REVERSE;
        }
      else if(strand == ZMAPSTRAND_FORWARD)
        {
          if(order_data->strand == ZMAPSTRAND_REVERSE)
            window->feature_set_names =
              g_list_reverse(window->feature_set_names);
          order_data->strand = ZMAPSTRAND_FORWARD;
        }
      else if(zmapWindowContainerIsStrandSeparator(container))
	return ;		/* Watch early return! */
      else
        zMapAssertNotReached(); /* What! */  //mh17: strand can only bw fwd, bwd, or sep */


      strand_features_group->item_list =
            g_list_sort_with_data(strand_features_group->item_list,
                                  qsortColumnsCB, order_data);

      /* update foo_canvas list cache. joy. */
      strand_features_group->item_list_end = g_list_last(strand_features_group->item_list);


    }

  return ;
}


static gboolean isFrameSensitive(gconstpointer col_data)
{
  ZMapWindowContainerFeatureSet container = ZMAP_CONTAINER_FEATURESET(col_data);
  gboolean frame_sensitive = FALSE ;
  ZMapFrame frame;

  frame = zmapWindowContainerFeatureSetGetFrame(container);

  if(frame != ZMAPFRAME_NONE)
    {
      frame_sensitive = zmapWindowContainerFeatureSetIsFrameSpecific(container, NULL);
    }

  return frame_sensitive;
}

#if 0
static int columnFSNListPosition(gconstpointer col_data, GList *feature_set_names)
{
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data);
  ZMapFeatureAny feature_any;
  GList *list;
  int position = 0;

  if((feature_any = zmapWindowItemGetFeatureAny(col_group)))
    {
      list = g_list_find(feature_set_names, GUINT_TO_POINTER(feature_any->unique_id));
      position = g_list_position(feature_set_names, list);
    }

  return position;
}
#endif

static GQuark columnFSNid(gconstpointer col_data)
{
  GQuark id = 0;

#if MH17_COL_ID_WAS_FEATURE_ID
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data);
  ZMapFeatureAny feature_any;

  if((feature_any = zmapWindowItemGetFeatureAny(col_group)))
      id = feature_any->unique_id;
#else
  ZMapWindowContainerFeatureSet container  = (ZMapWindowContainerFeatureSet) col_data;

  id = zmapWindowContainerFeatureSetGetColumnId(container);
#endif
  return id;
}


/*
 * -1 if a is before b
 * +1 if a is after  b
 *  0 if equal
 */
static gint qsortColumnsCB(gconstpointer colA, gconstpointer colB, gpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  ZMapFrame frame_a, frame_b;
  int pos_a, pos_b;
  gboolean sens_a, sens_b;
  gint order = 0;
  GQuark idA,idB;

  if(colA == colB)
    return order;

  idA = columnFSNid(colA);
  idB = columnFSNid(colB);

  sens_a = isFrameSensitive(colA);
  sens_b = isFrameSensitive(colB);

  pos_a = GPOINTER_TO_UINT(g_hash_table_lookup(order_data->names_hash,GUINT_TO_POINTER(idA)));
  pos_b = GPOINTER_TO_UINT(g_hash_table_lookup(order_data->names_hash,GUINT_TO_POINTER(idB)));

  if(!pos_a && !pos_b)  // both not in the list: put at the end and order by featureset id
    {
      pos_a = idA;
      pos_b = idB;
    }
    // else put unknown columns at the end
  else if(!pos_a)
      pos_a = order_data->n_names + 1;
  else if(!pos_b)
      pos_b = order_data->n_names + 1;

  if(order_debug_G)
    printf("compare %s/%d=%d %s/%d=%d", g_quark_to_string(idA),(int) sens_a,pos_a, g_quark_to_string(idB),(int) sens_b, pos_b);


  if(sens_a && sens_b)
    {
      /* Both are frame sensitive */
      frame_a = zmapWindowContainerFeatureSetGetFrame((ZMapWindowContainerFeatureSet) colA);
      frame_b = zmapWindowContainerFeatureSetGetFrame((ZMapWindowContainerFeatureSet) colB);
    }

  if(sens_a && sens_b)
    {
      /* If frames are equal rely on position */
      if(frame_a == frame_b)
        {
          if(pos_a > pos_b)
            order = 1;
          else if(pos_a < pos_b)
            order = -1;
          else
            order = 0;
        }
      else
        {
          /* else use frame */
          if(frame_a > frame_b)
            order = 1;
          else if(frame_a < frame_b)
            order = -1;
          else
            order = 0;
        }
    }
  else
   {
      if(sens_a)
      {
        /* only a is frame sensitive. Does b come before or after the 3 Frame section */
        pos_a = order_data->three_frame_position;
      }
      else if(sens_b)
      {
        /* only b is frame sensitive. Does a come before or after the 3 Frame section */
        pos_b = order_data->three_frame_position;
      }

      if(pos_a > pos_b)
        order = 1;
      else if(pos_a < pos_b)
        order = -1;
      else
        order = 0;
    }

  if(order_debug_G)
    printf("order=  %d\n", order);

  return order;
}
