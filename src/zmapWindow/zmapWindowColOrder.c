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
 * Last edited: Aug 13 10:09 2008 (edgrif)
 * Created: Tue Dec  5 14:48:45 2006 (rds)
 * CVS info:   $Id: zmapWindowColOrder.c,v 1.9 2008-09-24 15:04:28 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

typedef struct 
{
  ZMapWindow window;            /* The window */

  ZMapStrand strand;            /* which strand group we're sorting... 
                                 * direction of window->feature_set_names */
  GList *names_list;
  int three_frame_position;
} OrderColumnsDataStruct, *OrderColumnsData;

static void orderPositionColumns(ZMapWindow window, gboolean redraw_too);
static void orderColumnsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                           ZMapContainerLevelType level, gpointer user_data);
static gint qsortColumnsCB(gconstpointer colA, gconstpointer colB, gpointer user_data);
static gboolean isFrameSensitive(gconstpointer col_data);
static int columnFSNListPosition(gconstpointer col_data, GList *feature_set_names);
static ZMapFrame columnFrame(gconstpointer col_data);


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
  FooCanvasGroup *super_root ;
  OrderColumnsDataStruct order_data = {NULL} ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							   0,0,0,
							   ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
							   0)) ;
  zMapAssert(super_root) ;

  order_data.window = window;
  order_data.strand = ZMAPSTRAND_FORWARD; /* makes things simpler */

  if(order_debug_G)
    printf("%s: starting column ordering\n", __PRETTY_FUNCTION__);

  zmapWindowContainerExecuteFull(FOO_CANVAS_GROUP(super_root),
                                 ZMAPCONTAINER_LEVEL_STRAND,
                                 orderColumnsCB, &order_data,
                                 NULL, NULL, redraw_too);

  /* If we've reversed the feature_set_names list, and left it like that, re-reverse. */
  if(order_data.strand == ZMAPSTRAND_REVERSE)
    window->feature_set_names = g_list_reverse(window->feature_set_names);

  if(order_debug_G)
    printf("%s: columns should now be in order\n", __PRETTY_FUNCTION__);

  return ;

}

static void orderColumnsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                           ZMapContainerLevelType level, gpointer user_data)
{
  OrderColumnsData order_data = (OrderColumnsData)user_data;
  FooCanvasGroup *strand_group;
  ZMapStrand strand;
  ZMapWindow window = order_data->window;
  GList *frame_list = NULL;

  if(level == ZMAPCONTAINER_LEVEL_STRAND)
    {
      /* Get Features */
      strand_group = zmapWindowContainerGetFeatures(data);
      zMapAssert(strand_group);

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
      else if(zmapWindowContainerIsStrandSeparator(data))
	return ;		/* Watch early return! */
      else
        zMapAssertNotReached(); /* What! */


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

      /* update foo_canvas list cache. joy. */
      strand_group->item_list_end = g_list_last(strand_group->item_list);
    }
  
  return ;
}


static gboolean isFrameSensitive(gconstpointer col_data)
{
  gboolean frame_sensitive = FALSE ;
  FooCanvasGroup *col_group = FOO_CANVAS_GROUP(col_data) ;
  ZMapFeatureAny feature_any ;
  ZMapWindowItemFeatureSetData set_data ;
  ZMapFeatureTypeStyle style ;


  if((set_data = g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_SET_DATA)) &&
     (feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(col_group), ITEM_FEATURE_DATA))))
    {
      style = set_data->style;
      
      if (set_data->frame != ZMAPFRAME_NONE)
        {
	  frame_sensitive = zMapStyleIsFrameSpecific(style) ;
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

  if(order_debug_G)
    printf("returning order %d\n", order);

  return order;
}
