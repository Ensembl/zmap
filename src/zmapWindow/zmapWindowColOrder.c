/*  File: zmapWindowColOrder.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Dec 13 14:43 2006 (rds)
 * Created: Tue Dec  5 14:48:45 2006 (rds)
 * CVS info:   $Id: zmapWindowColOrder.c,v 1.4 2006-12-13 15:13:29 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

typedef struct 
{
  ZMapWindow window;            /* The window */
  GList *item_list2order;       /* Which list we'll order */
  int current_list_number;      /* zero based. */
  int group_length;             /* item_list2order length */
  ZMapStrand strand;            /* which strand group we're sorting... 
                                 * direction of window->feature_set_names */

  GQuark feature_set_name;      /* columnWithNameAndFrame depends on this. */
  ZMapFrame frame;              /* columnWithNameAndFrame depends on this. */

  GList *names_list;
  gboolean quick_sort;
  int three_frame_position;
} OrderColumnsDataStruct, *OrderColumnsData;

static gint columnWithName(gconstpointer list_data, gconstpointer user_data);
static gint columnWithFrame(gconstpointer list_data, gconstpointer user_data);
static gint columnWithNameAndFrame(gconstpointer list_data, gconstpointer user_data);
static gint matchFrameSensitiveColCB(gconstpointer list_data, gconstpointer user_data);
static void sortByFSNList(gpointer list_data, gpointer user_data);
static void orderColumnsForFrame(OrderColumnsData order_data, GList *list, ZMapFrame frame);
static void orderColumnsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                           ZMapContainerLevelType level, gpointer user_data);
static gint qsortColumnsCB(gconstpointer colA, gconstpointer colB, gpointer user_data);


static gboolean order_debug_G = FALSE;
static gboolean frame_debug_G = FALSE;


/* void zmapWindowColOrderColumns(ZMapWindow window)
 *
 * Ordering the columns in line with the contents of
 * window->feature_set_names. The foocanvas 
 */
void zmapWindowColOrderColumns(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  OrderColumnsDataStruct order_data = {NULL} ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							   0,0,0,
							   ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
							   0)) ;
  zMapAssert(super_root) ;

  order_data.window = window;
  order_data.strand = ZMAPSTRAND_FORWARD; /* makes things simpler */

  order_data.quick_sort = TRUE;

  if(order_debug_G)
    printf("%s: starting column ordering\n", __PRETTY_FUNCTION__);

  zmapWindowContainerExecute(FOO_CANVAS_GROUP(super_root),
                             ZMAPCONTAINER_LEVEL_STRAND,
                             orderColumnsCB, &order_data);

  /* If we've reversed the feature_set_names list, and left it like that, re-reverse. */
  if(order_data.strand == ZMAPSTRAND_REVERSE)
    window->feature_set_names = g_list_reverse(window->feature_set_names);

  if(order_debug_G)
    printf("%s: columns should now be in order\n", __PRETTY_FUNCTION__);

  return ;
}


static gint columnWithName(gconstpointer list_data, gconstpointer user_data)
{
  FooCanvasGroup *parent = FOO_CANVAS_GROUP(list_data);
  ZMapFeatureAny feature_any;
  GQuark name_data = GPOINTER_TO_UINT(user_data);
  gint match = -1;

  if((feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(parent), ITEM_FEATURE_DATA))))
    {
      if(name_data == feature_any->unique_id)
        match = 0;
    }

  return match;
}

static gint columnWithFrame(gconstpointer list_data, gconstpointer user_data)
{
  FooCanvasGroup *parent = FOO_CANVAS_GROUP(list_data);
  ZMapFrame frame_data = GPOINTER_TO_UINT(user_data);
  ZMapWindowItemFeatureSetData set_data;
  gint match = -1;

  if((set_data = g_object_get_data(G_OBJECT(parent), ITEM_FEATURE_SET_DATA)))
    {
      if(set_data->frame == frame_data)
        match = 0;
    }

  return match;
}

static gint columnWithNameAndFrame(gconstpointer list_data, gconstpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  gint match = -1;

  if(!columnWithName(list_data, GUINT_TO_POINTER(order_data->feature_set_name)))
    {
      if(order_data->frame == ZMAPFRAME_NONE)
        match = 0;
      else if(!columnWithFrame(list_data, GUINT_TO_POINTER(order_data->frame)))
        match = 0;
    }
  
  return match;
}

static gint matchFrameSensitiveColCB(gconstpointer list_data, gconstpointer user_data)
{
  FooCanvasGroup *col_group   = FOO_CANVAS_GROUP(list_data);
  ZMapWindowItemFeatureSetData set_data;
  ZMapFeatureAny feature_any;
  ZMapFeatureTypeStyle style;
  gint match = -1;

  if((set_data = g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_SET_DATA)) &&
     (feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_DATA))))
    {
      style = set_data->style;
      /* style = zmapWindowStyleTableFind(set_data->style_table, feature_any->unique_id); */

      /* Using the set_data->style seems to work well here.  There are
       * frame specific columns ONLY when 3 Frame display is on.  This 
       * avoids looking at window->display_3_frame which is actaully 
       * UNSET when drawing the 3 frame columns... */

      if(style->opts.frame_specific)
        {
          match = 0;
          if(frame_debug_G)
            printf("%s: %s is frame sensitive\n", __PRETTY_FUNCTION__,
                   g_quark_to_string(feature_any->unique_id));
        }
    }

  return match;
}

static void sortByFSNList(gpointer list_data, gpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  GList *column;
  int position, target, positions_to_move;

  order_data->feature_set_name = GPOINTER_TO_UINT(list_data);

  if((column = g_list_find_custom(order_data->item_list2order,
                                  order_data, columnWithNameAndFrame)))
    {
      /* find position based on the difference between the 2 
       * list lengths. Although there is a g_list_position
       * function, it means passing in the original list and
       * possibly longer list traversal... Not that is really 
       * an issue. */
      position = g_list_length(column);
      target   = order_data->current_list_number;

      if(order_debug_G)
        printf("%s: %s\n\tlist_length = %d\n\tgroup_length = %d\n\tposition = %d\n\ttarget = %d\n",
               __PRETTY_FUNCTION__,
               g_quark_to_string(order_data->feature_set_name),
               position,
               order_data->group_length,
               order_data->group_length - position,
               target);

      position = order_data->group_length - position;

      if(position != target)
        {
          positions_to_move = target - position;

          if(order_debug_G)
            printf("%s: moving %d\n", __PRETTY_FUNCTION__, positions_to_move);

          if(positions_to_move > 0)
            zMap_g_list_raise(column, positions_to_move);
          else if(positions_to_move < 0)
            zMap_g_list_lower(column, positions_to_move * -1);
          
        }

      order_data->current_list_number++;
    }

  return ;
}

static void orderColumnsForFrame(OrderColumnsData order_data, GList *list, ZMapFrame frame)
{
  /* simple function so that all this get remembered... */
  order_data->current_list_number = 
    order_data->feature_set_name  = 0;
  order_data->item_list2order     = list;

  order_data->group_length = g_list_length(list);
  order_data->frame        = ZMAPFRAME_NONE;

  g_list_foreach(order_data->window->feature_set_names, sortByFSNList, order_data);

  return ;
}

static void orderColumnsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                           ZMapContainerLevelType level, gpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  FooCanvasGroup *strand_group;
  int interface_check;
  ZMapStrand strand;
  ZMapWindow window = order_data->window;
  GList *frame_list = NULL, 
    *sensitive_list = NULL, 
    *sensitive_tmp  = NULL;

  if(level == ZMAPCONTAINER_LEVEL_STRAND)
    {
      /* Get Features */
      strand_group = zmapWindowContainerGetFeatures(data);
      zMapAssert(strand_group);

      /* check that g_list_length doesn't do g_list_first! */
      interface_check = g_list_length(strand_group->item_list_end);
      zMapAssert(interface_check == 1);

      strand = zmapWindowContainerGetStrand(data);

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
      else
        zMapAssertNotReached(); /* What! */

      if(order_data->quick_sort)
        {
          order_data->names_list = window->feature_set_names;
          if((frame_list = g_list_find(window->feature_set_names, 
                                       GUINT_TO_POINTER(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME)))))
            {
              order_data->three_frame_position = g_list_position(order_data->names_list,
                                                                 frame_list);
              strand_group->item_list = 
                g_list_sort_with_data(strand_group->item_list,
                                      qsortColumnsCB, order_data);
            }
        }
      else
        {
          if((frame_list = g_list_find(window->feature_set_names, 
                                       GUINT_TO_POINTER(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME)))))
            {
              /* grep out the group->item_list frame sensitive columns */
              /* This only produces a list when 3 frame is on 
               * (see comment in matchFrameSensitiveColCB). */
              sensitive_list = zMap_g_list_grep(&(strand_group->item_list),
                                                order_data,
                                                matchFrameSensitiveColCB);
              sensitive_tmp = sensitive_list; /* hold on to a copy of ptr for later */
            }
          
          if(order_debug_G)
            printf("%s: \n", __PRETTY_FUNCTION__);
          
          /* order the non frame sensitive ones */
          orderColumnsForFrame(order_data, strand_group->item_list, ZMAPFRAME_NONE);
          /* reset this. */
          strand_group->item_list = g_list_first(strand_group->item_list);
          
          /* Are we displaying any 3 frame columns */
          if(sensitive_list)
            {
              GList *tmp_list;
              int position = 0;
              
              /* If so, order the frame sensitive ones */
              
              /* This all works based on the g_list_find_custom function
               * returning the first and only the first column with a name
               * it finds. The orderColumnsForFrame then moves the column 
               * only as far as the first column in the list.
               */
              
              /* start with Frame 0. */
              orderColumnsForFrame(order_data, sensitive_list, ZMAPFRAME_0);
              
              /* list will have changed, find first... this is a pain ... */
              sensitive_list = tmp_list = g_list_first(sensitive_list);
              
              /* alter the start of the list to be the first column with frame 1 */
              if((sensitive_list = g_list_find_custom(sensitive_list, 
                                                      GUINT_TO_POINTER(ZMAPFRAME_1),
                                                      columnWithFrame)))
                {
                  /* order this list of columns */
                  orderColumnsForFrame(order_data, sensitive_list, ZMAPFRAME_1);
                }
              else
                sensitive_list = tmp_list; /* reset list to not be NULL in case of no frame 1 cols */
              
              /* list will have changed, find first... this is a pain ... (still) */
              sensitive_list = tmp_list = g_list_first(sensitive_list);
              
              /* alter the start of the list to be the first column with frame 2 */
              if((sensitive_list = g_list_find_custom(sensitive_list, 
                                                      GUINT_TO_POINTER(ZMAPFRAME_2),
                                                      columnWithFrame)))
                {
                  /* order this list of columns */
                  orderColumnsForFrame(order_data, sensitive_list, ZMAPFRAME_2);
                }
              else
                sensitive_list = tmp_list; /* reset list, although kinda redundant. */
              
              /* again reset the list everything has changed... */
              sensitive_list = g_list_first(sensitive_tmp);
              
              /* find where to insert the sensitive list. */
              /* This is a little tedious and possibly the weakest point of this.
               * frame_list is a GList member from the window->feature_set_names
               * We step backwards through this until we get to the beginning 
               * trying to find a column in the strand_group (not frame sensitive)
               * which where we can insert the sensitive list.
               */
              while(frame_list)
                {
                  if((tmp_list = g_list_find_custom(strand_group->item_list,
                                                    frame_list->data,
                                                    columnWithName)))
                    {
                      position = g_list_position(strand_group->item_list, tmp_list);
                      if(frame_debug_G)
                        printf("%s: inserting sensitive list after %s @ position %d\n",
                               __PRETTY_FUNCTION__,
                               g_quark_to_string(GPOINTER_TO_UINT(frame_list->data)),
                               position);
                      break;
                    }
                  frame_list = frame_list->prev;
                }
              
              /* insert the frame sensitive ones into the non frame sensitive list */
              strand_group->item_list = zMap_g_list_insert_list_after(strand_group->item_list, 
                                                                      sensitive_list, position);
            }
        }
      /* update foo_canvas list cache. joy. */
      strand_group->item_list_end = g_list_last(strand_group->item_list);
    }
  
  return ;
}


/* 
 * Could all be done with quick sort in g_list_sort_with_data
 * Not sure which is faster...
 *
 */

static gboolean isFrameSensitive(gconstpointer col_data)
{
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data);
  ZMapFeatureAny feature_any;
  ZMapWindowItemFeatureSetData set_data;
  ZMapFeatureTypeStyle style ;
  gboolean frame_sensitive = FALSE;

  if((set_data = g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_SET_DATA)) &&
     (feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_DATA))))
    {
      style = set_data->style;
      
      if(set_data->frame != ZMAPFRAME_NONE && style->opts.frame_specific)
        {
          frame_sensitive = TRUE;
        }
      if(order_debug_G)
        printf("  column %s %s frame sensitive\n", 
               g_quark_to_string(feature_any->original_id), 
               (frame_sensitive ? "is" : "is not"));
    }
  
  return frame_sensitive;
}

static int columnFSNListPosition(gconstpointer col_data, GList *feature_set_names)
{
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data);
  ZMapFeatureAny feature_any;
  GList *list;
  int position = 0;

  if((feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_DATA))))
    {
      list = g_list_find(feature_set_names, GUINT_TO_POINTER(feature_any->unique_id));
      position = g_list_position(feature_set_names, list);
    }

  return position;
}

static ZMapFrame columnFrame(gconstpointer col_data)
{
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data);
  ZMapWindowItemFeatureSetData set_data;
  ZMapFrame frame = ZMAPFRAME_NONE;

  if((set_data = g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_SET_DATA)))
    {
      frame = set_data->frame;
    }

  return frame;
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

  if(colA == colB)
    return order;
  
  if(order_debug_G)
    printf("enter colA and colB test\n");

  sens_a = isFrameSensitive(colA);
  sens_b = isFrameSensitive(colB);

  if(sens_a && sens_b)
    {
      /* Both are frame sensitive */
      frame_a = columnFrame(colA);
      frame_b = columnFrame(colB);

      if(order_debug_G)
        printf("    columns are both sensitive frame_a = %d, frame_b = %d\n", frame_a, frame_b);

      /* If frames are equal rely on position */
      if(frame_a == frame_b)
        {
          pos_a = columnFSNListPosition(colA, order_data->names_list);
          pos_b = columnFSNListPosition(colB, order_data->names_list);
          if(pos_a > pos_b)
            order = 1;
          else if(pos_b < pos_b)
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
  else if(sens_a)
    {
      /* only a is frame sensitive. Does b come before or after the 3 Frame section */
      pos_b = columnFSNListPosition(colB, order_data->names_list);
      pos_a = order_data->three_frame_position;
  
      if(pos_a > pos_b)
        order = 1;
      else if(pos_a < pos_b)
        order = -1;
      else
        order = 0;
    }
  else if(sens_b)
    {
      /* only b is frame sensitive. Does a come before or after the 3 Frame section */
      pos_a = columnFSNListPosition(colA, order_data->names_list);
      pos_b = order_data->three_frame_position;

      if(pos_a > pos_b)
        order = 1;
      else if(pos_a < pos_b)
        order = -1;
      else
        order = 0;      
    }
  else
    {
      pos_a = columnFSNListPosition(colA, order_data->names_list);
      pos_b = columnFSNListPosition(colB, order_data->names_list);

      if(pos_a > pos_b)
        order = 1;
      else if(pos_a < pos_b)
        order = -1;
      else
        order = 0;
    }

  printf("returning order %d\n", order);

  return order;
}



