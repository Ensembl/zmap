/*  File: zmapWindowContainerUtils.c
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
 * Description: Utility functions for "container" zmap canvas items,
 *              these include feature set, alignment, block etc.
 *
 * Exported functions: See zmapWindowContainerUtils.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <ZMap/zmapUtilsFoo.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>

/* There's a lot of sharing here....is it wise or necessary ? */
#include <zmapWindowContainerGroup_I.h>
//#include <zmapWindowContainerChildren_I.h>

/* It doesn't feel good that this is here.... */
#include <zmapWindowContainerFeatureSet_I.h>


typedef struct ContainerRecursionDataStruct_
{
  ZMapContainerLevelType     stop ;

  ZMapContainerUtilsExecFunc container_enter_cb   ;
  gpointer                   container_enter_data ;

  ZMapContainerUtilsExecFunc container_leave_cb   ;
  gpointer                   container_leave_data ;

} ContainerRecursionDataStruct, *ContainerRecursionData;


typedef struct
{
  GList *forward, *reverse;
}ForwardReverseColumnListsStruct, *ForwardReverseColumnLists;


static void eachContainer(gpointer data, gpointer user_data);

static void set_column_lists_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				ZMapContainerLevelType level, gpointer user_data);


static FooCanvasItem *getNextFeatureItem(FooCanvasGroup *group,
					 FooCanvasItem *orig_item,
					 ZMapContainerItemDirection direction, gboolean wrap,
					 zmapWindowContainerItemTestCallback item_test_func_cb,
					 gpointer user_data) ;





/* 
 *                            External routines
 */



/* Return the feature set item which has zoom and much else....
 * 
 * There is no checking here as it's a trial function...I need to add lots
 * if I keep this function.
 * 
 *  */
ZMapWindowFeaturesetItem ZMapWindowContainerGetFeatureSetItem(ZMapWindowContainerGroup container)
{
  ZMapWindowFeaturesetItem container_feature_list = NULL ;
  FooCanvasGroup *group = (FooCanvasGroup *)container ;
  GList *l ;

  for (l = group->item_list ; l ; l = l->next)
    {
      ZMapWindowFeaturesetItem temp ;

      temp = (ZMapWindowFeaturesetItem)(l->data) ;

      if (zMapWindowCanvasIsFeatureSet(temp))
	{
	  container_feature_list = temp ;

	  break ;
	}
    }

  return container_feature_list ;
}




/*!
 * \brief Checks whether the container is valid
 *
 * \param any_group  Any FooCanvas group is accepted here.
 *
 * \return boolean describing validity TRUE = valid, FALSE = invalid
 */

gboolean zmapWindowContainerUtilsIsValid(FooCanvasGroup *any_group)
{
  gboolean valid = FALSE;

  valid = ZMAP_IS_CONTAINER_GROUP(any_group);

  return valid;
}

void zmapWindowContainerUtilsPrint(FooCanvasGroup *any_group)
{

  if (ZMAP_IS_CONTAINER_GROUP(any_group))
    {
      ZMapWindowContainerGroup this_container;

      this_container = ZMAP_CONTAINER_GROUP(any_group);

      switch(this_container->level)
	{
	case ZMAPCONTAINER_LEVEL_ROOT:       printf("context: ");    break;
	case ZMAPCONTAINER_LEVEL_ALIGN:      printf("align: ");      break;
	case ZMAPCONTAINER_LEVEL_BLOCK:      printf("block: ");      break;
	case ZMAPCONTAINER_LEVEL_FEATURESET: printf("featureset: "); break;
	default:
	  break;
	}

      printf("\n");
    }
  else
    {
      printf("Just a regular foocanvas group\n");
    }

  return ;
}

/* gross tree access. any item -> container group */

ZMapWindowContainerGroup zmapWindowContainerChildGetParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;

  if (ZMAP_IS_CONTAINER_GROUP(item))
    {
      container_group = ZMAP_CONTAINER_GROUP(item);
    }
  return container_group;
}

ZMapWindowContainerGroup zmapWindowContainerGetNextParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;

  if ((container_group = zmapWindowContainerChildGetParent(item)))
    {
      ZMapWindowContainerGroup tmp_group;
      FooCanvasItem *parent = NULL;
      FooCanvasItem *tmp_item;

      tmp_group = container_group;
      tmp_item  = FOO_CANVAS_ITEM(container_group);

      container_group = NULL;

      if(tmp_group->level == ZMAPCONTAINER_LEVEL_ROOT)
	{
	  parent = FOO_CANVAS_ITEM(tmp_group);
	}
      else if(tmp_item->parent)
	{
	  parent = tmp_item->parent;
	}

      if(parent && ZMAP_IS_CONTAINER_GROUP(parent))
	container_group = ZMAP_CONTAINER_GROUP(parent);
    }

  return container_group;
}


ZMapWindowContainerGroup zmapWindowContainerUtilsGetParentLevel(ZMapWindowContainerGroup container_group,
								ZMapContainerLevelType   level)
{
  ZMapContainerLevelType curr_level;

  while((curr_level = container_group->level) != level)
    {
      FooCanvasItem *item = (FooCanvasItem *)(container_group);

      if(!(container_group = zmapWindowContainerGetNextParent(item)))
	break;
    }

  return container_group;
}

ZMapWindowContainerGroup zmapWindowContainerUtilsItemGetParentLevel(FooCanvasItem *item, ZMapContainerLevelType level)
{
  ZMapWindowContainerGroup container_group = NULL;

  if((container_group = zmapWindowContainerCanvasItemGetContainer(item)))
    {
      container_group = zmapWindowContainerUtilsGetParentLevel(container_group, level);
    }

  return container_group;
}

ZMapContainerLevelType zmapWindowContainerUtilsGetLevel(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  ZMapContainerLevelType level = ZMAPCONTAINER_LEVEL_INVALID;

  if((container_group = ZMAP_CONTAINER_GROUP(item)))
    level = container_group->level;

  return level;
}

ZMapWindowContainerGroup zmapWindowContainerCanvasItemGetContainer(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  FooCanvasItem *container_item = NULL;
  FooCanvasItem *parent;

  g_return_val_if_fail(item != NULL, NULL);
  g_return_val_if_fail(FOO_IS_CANVAS_ITEM(item), NULL);

  if (ZMAP_IS_CONTAINER_GROUP(item))
    {
      container_item = item;
    }
  else if((parent = item->parent))
    {
      do
	{
	  if(ZMAP_IS_CONTAINER_GROUP(parent))
	    container_item = parent;
	}
      while((container_item == NULL) && parent && (parent = parent->parent));
    }

  if(ZMAP_IS_CONTAINER_GROUP(container_item))
    {
      container_group = ZMAP_CONTAINER_GROUP(container_item);
    }
  else
    {
      zMapLogWarning("Failed to find container in path root -> ... -> item (%p).", item);
    }

  return container_group;
}







/* Get the index of an item in the feature list. */
GList *zmapWindowContainerFindItemInList(ZMapWindowContainerGroup container_parent, FooCanvasItem *item)
{
  GList *list_item = NULL ;
  FooCanvasGroup *container_features ;

  container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(container_parent)) ;

  list_item = zMap_foo_canvas_find_list_item(container_features, item) ;

  return list_item ;
}


/* Get the index of an item in the feature list, positions start at 0 so returns -1 if item cannot
 * be found. */
int zmapWindowContainerGetItemPosition(ZMapWindowContainerGroup container_parent, FooCanvasItem *item)
{
  int index = -1 ;
  FooCanvasGroup *container_features ;

  container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(container_parent)) ;

  index = zMap_foo_canvas_find_item(container_features, item) ;

  return index ;
}

/* Moves item to position in children of container_parent, positions start at 0.
 * Returns FALSE if item cannot be found in container_parent. */
gboolean zmapWindowContainerSetItemPosition(ZMapWindowContainerGroup container_parent,
					    FooCanvasItem *item, int position)
{
  gboolean result = FALSE ;
  int index ;
  FooCanvasGroup *container_features ;

  container_features = FOO_CANVAS_GROUP(zmapWindowContainerGetFeatures(container_parent)) ;

  if ((index = zMap_foo_canvas_find_item(container_features, item)) == -1)
    {
      if (position > index)
	foo_canvas_item_raise(item, (position - index)) ;
      else if (position < index)
	foo_canvas_item_lower(item, (index - position)) ;

      result = TRUE ;
    }

  return result ;
}


/* Returns the nth item in the container (0-based), there are two special values for
 * nth_item for the first and last items:  ZMAPCONTAINER_ITEM_FIRST, ZMAPCONTAINER_ITEM_LAST */
FooCanvasItem *zmapWindowContainerGetNthFeatureItem(ZMapWindowContainerGroup container, int nth_item)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *features ;

  features = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

  if (nth_item == 0 || nth_item == ZMAPCONTAINER_ITEM_FIRST)
    item = FOO_CANVAS_ITEM(features->item_list->data) ;
  else if (nth_item == ZMAPCONTAINER_ITEM_LAST)
    item = FOO_CANVAS_ITEM(features->item_list_end->data) ;
  else
    item = FOO_CANVAS_ITEM((g_list_nth(features->item_list, nth_item))->data) ;

  return item ;
}




/*!
 * \brief Iterate through feature items
 *
 * Given any item that is a direct child of a column group (e.g. not a subfeature), returns
 * the previous or next item that optionally satisfies item_test_func_cb(). The function skips
 * over items that fail these tests.
 *
 * direction controls which way the item list is processed and if wrap is TRUE then processing
 * wraps around on reaching the end of the list.
 *
 * If no item can be found then the original will be returned, note that if item_test_func_cb()
 * was specified and the original item does not satisfy item_test_func_cb() then NULL is returned.
 *
 * */
FooCanvasItem *zmapWindowContainerGetNextFeatureItem(FooCanvasItem *orig_item,
						     ZMapContainerItemDirection direction, gboolean wrap,
						     zmapWindowContainerItemTestCallback item_test_func_cb,
						     gpointer user_data)
{
  FooCanvasItem *item = NULL ;
  ZMapWindowContainerGroup container_group;
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *group ;

  if (ZMAP_IS_CONTAINER_GROUP(orig_item))
    {
      /* If it's a container, get it's parents so we can step through containers. */
      container_group = zmapWindowContainerGetNextParent(orig_item) ;

      container_features = zmapWindowContainerGetFeatures(container_group);
    }
  else
    {
      /* If its a feature then return the column so we can step through features. */
      container_group = zmapWindowContainerCanvasItemGetContainer(orig_item);

      container_features = zmapWindowContainerGetFeatures(container_group);
    }

  group = FOO_CANVAS_GROUP(container_features) ;

  item = getNextFeatureItem(group, orig_item, direction, wrap,
			    item_test_func_cb, user_data) ;

  return item ;
}


static FooCanvasItem *getNextFeatureItem(FooCanvasGroup *group,
					 FooCanvasItem *orig_item,
					 ZMapContainerItemDirection direction, gboolean wrap,
					 zmapWindowContainerItemTestCallback item_test_func_cb,
					 gpointer user_data)
{
  FooCanvasItem *item = NULL ;
  FooCanvasGroup *features ;
  GList *feature_ptr ;
  FooCanvasItem *found_item = NULL ;

  features = group ;

  feature_ptr = zMap_foo_canvas_find_list_item(features, orig_item) ;

  while (feature_ptr && !item && found_item != orig_item)
    {
      if (direction == ZMAPCONTAINER_ITEM_NEXT)
	{
	  feature_ptr = g_list_next(feature_ptr) ;

	  if (!feature_ptr && wrap)
	    {
	      feature_ptr = g_list_first(features->item_list);

	      /* check that the group really knows where the start of
	       * the list is */

	      if(feature_ptr != features->item_list)
		features->item_list = feature_ptr;
	    }
	}
      else
	{
	  feature_ptr = g_list_previous(feature_ptr) ;

	  if (!feature_ptr && wrap)
	    {
	      feature_ptr = g_list_last(features->item_list) ;

	      /* check that the group really knows where the end of
	       * the list is */

	      if(feature_ptr != features->item_list_end)
		features->item_list_end = feature_ptr;
	    }
	}

      /* If wrap is FALSE then feature_ptr can be NULL */
      if (feature_ptr)
	{
	  found_item = (FooCanvasItem *)(feature_ptr->data) ;



	  if (!item_test_func_cb || item_test_func_cb(found_item, user_data))
	    item = found_item ;
	}
    }

  return item ;
}






/* Features */
gboolean zmapWindowContainerAttachFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny feature_any)
{
  gboolean status = FALSE;

  if(feature_any && ZMAP_IS_CONTAINER_GROUP(container))
    {
      switch(feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	  status = ZMAP_IS_CONTAINER_CONTEXT(container);
	  break;
	case ZMAPFEATURE_STRUCT_ALIGN:
	  status = ZMAP_IS_CONTAINER_ALIGNMENT(container);
	  break;
	case ZMAPFEATURE_STRUCT_BLOCK:
	  status = ZMAP_IS_CONTAINER_BLOCK(container);
	  break;
	case ZMAPFEATURE_STRUCT_FEATURESET:
#if 1 /* MH17_NO_RECOVER */
/* I'd like to remove this but that will mean having to remove stats code
 * in 3 files iffed by RDS_REMOVED_STATS
 *
 * this is the only place this function gets called
 *
 * some months later ... when trying tp ding the block feature from a CanvasBlock ...
 * all the canvas items/groups have a feature_any (except strand) and not attaching one might cause errors
 * but as several featuresets can go in a column (featureset in canvas speak) then we don't get a 1-1 mapping
 */
	  if((status = ZMAP_IS_CONTAINER_FEATURESET(container)))
	    {
	      status = zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)container,
								     (ZMapFeatureSet)feature_any);
	    }
#else
      status = ZMAP_IS_CONTAINER_FEATURESET(container);
#endif
	  break;
	case ZMAPFEATURE_STRUCT_FEATURE:
	case ZMAPFEATURE_STRUCT_INVALID:
	default:
	  status = FALSE;
	  break;
	}

      if(status)
	{
	  container->feature_any = feature_any;
#ifdef RDS_DONT_INCLUDE
	  g_object_set_data(G_OBJECT(container), ITEM_FEATURE_DATA, feature_any);
#endif
	}
    }

  return status;
}

gboolean zmapWindowContainerGetFeatureAny(ZMapWindowContainerGroup container, ZMapFeatureAny *feature_any_out)
{
  gboolean status = FALSE;

  if(ZMAP_IS_CONTAINER_GROUP(container) && feature_any_out)
    {
      if(ZMAP_IS_CONTAINER_FEATURESET(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_BLOCK(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_ALIGNMENT(container))
	status = TRUE;
      else if(ZMAP_IS_CONTAINER_CONTEXT(container))
	status = TRUE;
      else
	{
	  status = FALSE;
	  zMapLogWarning("%s", "request for feature from unexpected container");
	}

      if(status)
	{
	  *feature_any_out = container->feature_any;
	}
    }

  return status;
}

ZMapFeatureSet zmapWindowContainerGetFeatureSet(ZMapWindowContainerGroup feature_group)
{
  ZMapFeatureSet feature_set = NULL;
  ZMapFeatureAny feature_any;

  if(zmapWindowContainerGetFeatureAny(feature_group, &feature_any) &&
     (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET))
    {
      feature_set = (ZMapFeatureSet)feature_any;
    }

  return feature_set;
}

ZMapFeatureBlock zmapWindowContainerGetFeatureBlock(ZMapWindowContainerGroup feature_group)
{
  ZMapFeatureBlock feature_block = NULL;
  ZMapFeatureAny feature_any;

  if(zmapWindowContainerGetFeatureAny(feature_group, &feature_any) &&
     (feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK))
    {
      feature_block = (ZMapFeatureBlock)feature_any;
    }

  return feature_block;
}


/* this used to be the features list as distinct from background item or underlay/overlay lists
 * so to preserve the meaning we return TRUE if there are real features
 * ie we ignore background
 * there is a loop here but we expect only on background feature
 * and there for we will not scan loads
 */
gboolean zmapWindowContainerHasFeatures(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerFeatures features;
  GList *l;
  gint layer;

  if((features = zmapWindowContainerGetFeatures(container)))
    {
	for (l = ((FooCanvasGroup *)features)->item_list ; l ; l = l->next)
	{
		if(FOO_IS_CANVAS_GROUP(l->data))
			continue;
		if(!ZMAP_IS_WINDOW_FEATURESET_ITEM(l->data))	/* a real foo canvas item? should not happen */
			return(TRUE);

		layer = zMapWindowCanvasFeaturesetGetLayer((ZMapWindowFeaturesetItem) l->data);
		if(!(layer & ZMAP_CANVAS_LAYER_DECORATION))
			return TRUE;
	}
    }

  return FALSE;
}



gboolean zmapWindowContainerUtilsGetColumnLists(ZMapWindowContainerGroup block_or_strand_group,
						GList **forward_columns_out,
						GList **reverse_columns_out)
{
  ZMapWindowContainerGroup block_group;
  gboolean found_block = FALSE;

  if((block_group = zmapWindowContainerUtilsGetParentLevel(block_or_strand_group,
							   ZMAPCONTAINER_LEVEL_BLOCK)))
    {
      ForwardReverseColumnListsStruct for_rev_lists = {NULL};

      zmapWindowContainerUtilsExecute(block_group,
				      ZMAPCONTAINER_LEVEL_FEATURESET,
				      set_column_lists_cb, &for_rev_lists);

      if(forward_columns_out)
	*forward_columns_out = for_rev_lists.forward;
      if(reverse_columns_out)
	*reverse_columns_out = for_rev_lists.reverse;

      found_block = TRUE;
    }

  return found_block;
}



void zmapWindowContainerUtilsRemoveAllItems(FooCanvasGroup *group)
{
  GList *list,*l;

  if((list = g_list_first(group->item_list)))
    {
      do
	{
	  GtkObject *gtk_item_object;

	  gtk_item_object = GTK_OBJECT(list->data);

        l = list;
	  list = list->next;
//        g_free(l);      /* mh17: oddly this was not done */

	  gtk_object_destroy(gtk_item_object);
	}
      while((list));
    }
  group->item_list = NULL; /* nor this */
  group->item_list_end = NULL;
  return ;
}


/* Recursion... */

/* unified way to descend and do things to ALL and every or just some */
void zmapWindowContainerUtilsExecuteFull(ZMapWindowContainerGroup   container_group,
					 ZMapContainerLevelType     stop_at_type,
					 ZMapContainerUtilsExecFunc container_enter_cb,
					 gpointer                   container_enter_data,
					 ZMapContainerUtilsExecFunc container_leave_cb,
					 gpointer                   container_leave_data)
{
  ContainerRecursionDataStruct data  = {0,NULL};
//  ZMapWindowCanvas zmap_canvas;
  FooCanvasItem *parent;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT &&
	     stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  parent = (FooCanvasItem *)container_group;

  data.stop                    = stop_at_type;

  data.container_enter_cb   = container_enter_cb;
  data.container_enter_data = container_enter_data;
  data.container_leave_cb   = container_leave_cb;
  data.container_leave_data = container_leave_data;



  eachContainer((gpointer)container_group, &data) ;

  return ;
}

/* unified way to descend and do things to ALL and every or just some */
void zmapWindowContainerUtilsExecute(ZMapWindowContainerGroup   parent,
				     ZMapContainerLevelType     stop_at_type,
				     ZMapContainerUtilsExecFunc container_enter_cb,
				     gpointer                   container_enter_data)
{
  zmapWindowContainerUtilsExecuteFull(parent, stop_at_type,
				      container_enter_cb,
				      container_enter_data,
				      NULL, NULL);
  return ;
}



/* look up a group in the the item_list
 * rather tediously the canvas is structured differently than the feature context and FToIHash
 * - it has a strand layer under block
 * - 3-frame columns all have the same id, whereas in the FToI hash the names are mangled
 */
static ZMapWindowContainerGroup getChildById(ZMapWindowContainerGroup group,GQuark id, ZMapStrand strand, ZMapFrame frame)
{
      ZMapWindowContainerGroup g;
      GList *l;
      FooCanvasGroup *children = (FooCanvasGroup *) zmapWindowContainerGetFeatures(group) ;
            /* this gets the group one of the four item positions */

      for(l = children->item_list;l;l = l->next)
      {
		if(!ZMAP_IS_CONTAINER_GROUP(l->data))	/* now we can have background CanvasFeaturesets in top level groups, just one item list */
			continue;

            g = (ZMapWindowContainerGroup) l->data;

		if(g->level == ZMAPCONTAINER_LEVEL_FEATURESET)
            {
                  ZMapWindowContainerFeatureSet set = ZMAP_CONTAINER_FEATURESET(g);;

                  if(set->unique_id == id && set->frame == frame && set->strand == strand)
                        return g;
            }
            else
            {
                  zMapAssert(g->feature_any);
                  if(g->feature_any->unique_id == id)
                        return g;
            }
      }
      return NULL;
}


FooCanvasItem *zMapFindCanvasColumn(ZMapWindowContainerGroup group,
      GQuark align, GQuark block, GQuark set, ZMapStrand strand, ZMapFrame frame)
{
  if(group)
      group = getChildById(group,align, ZMAPSTRAND_NONE, ZMAPFRAME_NONE);
  if(group)
      group = getChildById(group,block,ZMAPSTRAND_NONE, ZMAPFRAME_NONE);
  if(group)
      group = getChildById(group,set, strand, frame);

  return FOO_CANVAS_ITEM(group);
}


/* Internal */


/* Called for every container while descending.... */
static void eachContainer(gpointer data, gpointer user_data)
{
  ZMapWindowContainerGroup container = (ZMapWindowContainerGroup)data ;
  ContainerRecursionData   all_data  = (ContainerRecursionData)user_data ;
  ZMapContainerLevelType   level     = ZMAPCONTAINER_LEVEL_INVALID;
  FooCanvasGroup *children ;
  FooCanvasPoints this_points_data = {NULL}, parent_points_data = {NULL};
  FooCanvasPoints *this_points = NULL, *parent_points = NULL;
  double *bound = NULL, spacing = 0.0, bound_data = 0.0, coords[4] = {0.0, 0.0, 0.0, 0.0};

  if(!ZMAP_IS_CONTAINER_GROUP(container))	/* now we can have background CanvasFeaturesets in top level groups, just one item list */
	  return;

  children = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

  /* We need to get what we need in case someone destroys it under us. */
  level   = container->level;
  spacing = container->this_spacing;

  zMapAssert(level >  ZMAPCONTAINER_LEVEL_INVALID &&
             level <= ZMAPCONTAINER_LEVEL_FEATURESET);

  this_points   = &this_points_data;
  parent_points = &parent_points_data;
  bound         = &bound_data;

  this_points->coords   = &coords[0];
  parent_points->coords = &coords[0];

  /* Execute pre-recursion function. */
  if(all_data->container_enter_cb)
    (all_data->container_enter_cb)(container, this_points, level, all_data->container_enter_data);

  /* Recurse ? */
  if (children)
    {
      if (all_data->stop != level)
	{
	  g_list_foreach(children->item_list, eachContainer, user_data) ;
	}
    }


  /* Execute post-recursion function. */
  if(all_data->container_leave_cb)
    (all_data->container_leave_cb)(container, this_points, level, all_data->container_leave_data);

  return ;
}


static void set_column_lists_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ForwardReverseColumnLists lists_data;
	ZMapWindowContainerFeatureSet container_set;

	lists_data = (ForwardReverseColumnLists)user_data;

	container_set = ZMAP_CONTAINER_FEATURESET(container);

	if(container_set->strand == ZMAPSTRAND_FORWARD)
	  lists_data->forward = g_list_append(lists_data->forward, container);
	else if(container_set->strand == ZMAPSTRAND_REVERSE)
	  lists_data->reverse = g_list_append(lists_data->reverse, container);
      }
      break;
    default:
      break;
    }

  return ;
}

