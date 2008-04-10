/*  File: zmapWindowItemFeatureSet.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 * Last edited: Apr 10 15:16 2008 (rds)
 * Created: Mon Jul 30 13:09:33 2007 (rds)
 * CVS info:   $Id: zmapWindowItemFeatureSet.c,v 1.2 2008-04-10 14:19:01 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>

typedef struct
{
  GList      *list;
  ZMapFeature feature;
}ListFeatureStruct, *ListFeature;

typedef struct
{
  GQueue     *queue;
  ZMapFeature feature;
}QueueFeatureStruct, *QueueFeature;

static void queueRemoveFromList(gpointer queue_data, gpointer user_data);
static void listRemoveFromList(gpointer list_data, gpointer user_data);
static void removeList(gpointer data, gpointer user_data_unused) ;
static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new);


ZMapWindowItemFeatureSetData zmapWindowItemFeatureSetCreate(ZMapWindow window,
                                                            ZMapFeatureTypeStyle style,
                                                            ZMapStrand strand,
                                                            ZMapFrame frame)
{
  ZMapWindowItemFeatureSetData item_feature_set = NULL;

  if((item_feature_set = g_new0(ZMapWindowItemFeatureSetDataStruct, 1)))
    {
      item_feature_set->strand      = strand;
      item_feature_set->frame       = frame;
      item_feature_set->style       = zMapFeatureStyleCopy(style) ;
      item_feature_set->style_table = zmapWindowStyleTableCreate() ;
      item_feature_set->window      = window;
      item_feature_set->user_hidden_stack = g_queue_new();
    }

  return item_feature_set;
}

void zmapWindowItemFeatureSetDestroy(ZMapWindowItemFeatureSetData item_feature_set)
{
  item_feature_set->window = NULL; /* not ours ... */

  if(item_feature_set->style)
    zMapFeatureTypeDestroy(item_feature_set->style);

  if(item_feature_set->style_table)
    zmapWindowStyleTableDestroy(item_feature_set->style_table);

  /* Get rid of stack of user hidden items. */
  if(!g_queue_is_empty(item_feature_set->user_hidden_stack))
    g_queue_foreach(item_feature_set->user_hidden_stack, removeList, NULL) ;

  if(item_feature_set->user_hidden_stack)
    g_queue_free(item_feature_set->user_hidden_stack);

  g_free(item_feature_set);

  return ;
}

/* As we keep a list of the item we need to delete them at times.  This is actually _not_ 
 * used ATM (Apr 2008) as the reason it was written turned out to have a better solution
 * RT# 63281.  Anyway the code is here if needed.
 */
void zmapWindowItemFeatureSetFeatureRemove(ZMapWindowItemFeatureSetData item_feature_set,
					   ZMapFeature feature)
{
  QueueFeatureStruct queue_feature;

  queue_feature.queue   = item_feature_set->user_hidden_stack;
  queue_feature.feature = feature;
  /* user_hidden_stack is a copy of the list in focus. We need to
   * remove items when they get destroyed */
  g_queue_foreach(queue_feature.queue, queueRemoveFromList, &queue_feature);

  return ;
}


/* INTERNAL */

static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}

static void queueRemoveFromList(gpointer queue_data, gpointer user_data)
{
  GList *item_list = (GList *)queue_data;
  QueueFeature queue_feature = (QueueFeature)user_data;
  ListFeatureStruct list_feature;

  list_feature.list    = item_list;
  list_feature.feature = (ZMapFeature)queue_feature->feature;

  g_list_foreach(item_list, listRemoveFromList, &list_feature);

  if(list_feature.list != item_list)
    zmap_g_queue_replace(queue_feature->queue, item_list, list_feature.list);

  return;
}



static void listRemoveFromList(gpointer list_data, gpointer user_data)
{
  ListFeature list_feature = (ListFeature)user_data;
  ZMapFeature item_feature;
  FooCanvasItem *item;

  zMapAssert(FOO_IS_CANVAS_ITEM(list_data));

  item         = FOO_CANVAS_ITEM(list_data);
  item_feature = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA);
  zMapAssert(item_feature);
  
  if(item_feature == list_feature->feature)
    list_feature->list = g_list_remove(list_feature->list, item);

  return ;
}

static void zmap_g_queue_replace(GQueue *queue, gpointer old, gpointer new)
{
  int length, index;

  if((length = g_queue_get_length(queue)))
    {
      if((index = g_queue_index(queue, old)) != -1)
	{
	  gpointer popped = g_queue_pop_nth(queue, index);
	  zMapAssert(popped == old);
	  g_queue_push_nth(queue, new, index);
	}
    }
  else
    g_queue_push_head(queue, new);

  return ;
}
