/*  File: zmapWindowContainerUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * HISTORY:
 * Last edited: May 24 15:27 2010 (edgrif)
 * Created: Tue Apr 28 16:10:46 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerUtils.c,v 1.21 2010-10-13 09:00:38 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtilsFoo.h>
#include <zmapWindowCanvas.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>

/* There's a lot of sharing here....is it wise or necessary ? */
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerChildren_I.h>

/* It doesn't feel good that these are here.... */
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerStrand_I.h> /* access to ZMapWindowContainerStrand->strand */




typedef struct ContainerRecursionDataStruct_
{
  ZMapContainerLevelType     stop ;

  ZMapContainerUtilsExecFunc container_enter_cb   ;
  gpointer                   container_enter_data ;

  ZMapContainerUtilsExecFunc container_leave_cb   ;
  gpointer                   container_leave_data ;

  gboolean                   redraw_during_recursion;

} ContainerRecursionDataStruct, *ContainerRecursionData;


typedef struct
{
  GList *forward, *reverse;
}ForwardReverseColumnListsStruct, *ForwardReverseColumnLists;


static FooCanvasItem *container_get_child(ZMapWindowContainerGroup container, guint position);

static void eachContainer(gpointer data, gpointer user_data);

static void set_column_lists_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				ZMapContainerLevelType level, gpointer user_data);


static FooCanvasItem *getNextFeatureItem(FooCanvasGroup *group,
					 FooCanvasItem *orig_item,
					 ZMapContainerItemDirection direction, gboolean wrap,
					 zmapWindowContainerItemTestCallback item_test_func_cb,
					 gpointer user_data) ;






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
	case ZMAPCONTAINER_LEVEL_STRAND:     printf("strand: ");     break;
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
  else if (ZMAP_IS_CONTAINER_FEATURES(item)   ||
	  ZMAP_IS_CONTAINER_BACKGROUND(item) ||
	  ZMAP_IS_CONTAINER_OVERLAY(item)    ||
	  ZMAP_IS_CONTAINER_UNDERLAY(item))
    {
      if ((item = item->parent) && ZMAP_IS_CONTAINER_GROUP(item))
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
      else if(tmp_item->parent && tmp_item->parent->parent)
	{
	  parent = tmp_item->parent->parent;
	}

      if(parent && ZMAP_IS_CONTAINER_GROUP(parent))
	container_group = ZMAP_CONTAINER_GROUP(parent);
    }

  return container_group;
}

ZMapWindowContainerFeatures zmapWindowContainerUtilsItemGetFeatures(FooCanvasItem         *item,
								    ZMapContainerLevelType level)
{
  ZMapWindowContainerGroup container;
  ZMapWindowContainerFeatures features = NULL;

  if((container = zmapWindowContainerCanvasItemGetContainer(item)))
    {
      features = zmapWindowContainerGetFeatures(container);
    }

  return features;
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

ZMapWindowContainerStrand zmapWindowContainerBlockGetContainerStrand(ZMapWindowContainerBlock container_block,
								     ZMapStrand               strand)
{
  ZMapWindowContainerStrand container_strand = NULL;
  GList *item_list;
  int max = 3, llength;

  if(ZMAP_IS_CONTAINER_BLOCK(container_block))
    {
      ZMapWindowContainerFeatures features;

      features  = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)container_block);

      item_list = ((FooCanvasGroup *)features)->item_list;
      llength   = g_list_length(item_list);

      zMapAssert(llength <= max && llength > 0);

      do
	{
	  container_strand = ZMAP_CONTAINER_STRAND(item_list->data);

	  if(container_strand->strand == strand)
	    break;
	  else
	    container_strand = NULL;
	}
      while((item_list = item_list->next));
    }

  return container_strand ;
}

ZMapWindowContainerStrand zmapWindowContainerBlockGetContainerSeparator(ZMapWindowContainerBlock container_block)
{
  ZMapWindowContainerStrand container_strand = NULL;

  container_strand = zmapWindowContainerBlockGetContainerStrand(container_block,
								(ZMapStrand)CONTAINER_STRAND_SEPARATOR);

  return container_strand;
}


/* Child access. container group -> container <CHILD> */

/* Note this returns the _canvasgroup_ containing the features, not the feature list. */
ZMapWindowContainerFeatures zmapWindowContainerGetFeatures(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerFeatures features = NULL;
  FooCanvasItem *item;

  if ((item = container_get_child(container, _CONTAINER_FEATURES_POSITION)))
    {
      if (ZMAP_IS_CONTAINER_FEATURES(item))
        features = ZMAP_CONTAINER_FEATURES(item);

#ifdef MH17_NEVER_INCLUDE_THIS_CODE // item is not a ZCF so treat it as one ?
      else
	{
	  int i;
	  features = ZMAP_CONTAINER_FEATURES(item);
	  for(i = 0; i < 4; i++)
	    {
	      item = container_get_child(container, i);
	      printf("item @ position %d is type %s\n", i, G_OBJECT_TYPE_NAME(item));
	    }
	}
#endif //MH17_NEVER_INCLUDE_THIS_CODE
    }

  return features;
}


ZMapWindowContainerBackground zmapWindowContainerGetBackground(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerBackground background = NULL;
  FooCanvasItem *item;

  if((item = container_get_child(container, _CONTAINER_BACKGROUND_POSITION)))
    {
      background = ZMAP_CONTAINER_BACKGROUND(item);
    }

  return background;
}

ZMapWindowContainerOverlay zmapWindowContainerGetOverlay(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerOverlay overlay = NULL;
  FooCanvasItem *item;

  if((item = container_get_child(container, _CONTAINER_OVERLAY_POSITION)))
    {
      overlay = ZMAP_CONTAINER_OVERLAY(item);
    }

  return overlay;
}

ZMapWindowContainerUnderlay zmapWindowContainerGetUnderlay(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerUnderlay underlay = NULL;
  FooCanvasItem *item;

  if((item = container_get_child(container, _CONTAINER_UNDERLAY_POSITION)))
    {
      underlay = ZMAP_CONTAINER_UNDERLAY(item);
    }

  return underlay;
}

/* Strand code */

ZMapStrand zmapWindowContainerGetStrand(ZMapWindowContainerGroup container)
{
  ZMapStrand strand = ZMAPSTRAND_NONE;

  container = zmapWindowContainerUtilsGetParentLevel(container,ZMAPCONTAINER_LEVEL_STRAND);
  if(container && ZMAP_IS_CONTAINER_STRAND(container))
    {
      strand = (ZMAP_CONTAINER_STRAND(container))->strand;
    }

  return strand;
}


gboolean zmapWindowContainerIsStrandSeparator(ZMapWindowContainerGroup container)
{
  gboolean result = FALSE;

  if(ZMAP_IS_CONTAINER_STRAND(container))
    {
      ZMapWindowContainerStrand strand;

      strand = ZMAP_CONTAINER_STRAND(container);

      if(strand->strand == CONTAINER_STRAND_SEPARATOR)
	result = TRUE;
    }

  return result;
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
/* I'd like to remove this but that will mean having to re3move stats code in 3 files iffed bu RDS_REMOVED_STATS */
/* this is the only place this function gets called */
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
      else if(ZMAP_IS_CONTAINER_STRAND(container))
	{
	  status = FALSE;		/* strands don't have feature context equivalent levels */
	  zMapLogWarning("%s", "request for feature from a container strand");
	}
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

gboolean zmapWindowContainerHasFeatures(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerFeatures features;
  gboolean has_features = FALSE;

  if((features = zmapWindowContainerGetFeatures(container)))
    {
      if(((FooCanvasGroup *)features)->item_list)
	has_features = TRUE;
    }

  return has_features;
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


void zmapWindowContainerSetOverlayResizing(ZMapWindowContainerGroup container_group,
					   gboolean maximise_width, gboolean maximise_height)
{
  ZMapWindowContainerOverlay overlay;

  if((overlay = zmapWindowContainerGetOverlay(container_group)))
    {
      overlay->flags.max_width  = maximise_width ;
      overlay->flags.max_height = maximise_height ;
    }

  return ;
}

void zmapWindowContainerSetUnderlayResizing(ZMapWindowContainerGroup container_group,
					    gboolean maximise_width, gboolean maximise_height)
{
  ZMapWindowContainerUnderlay underlay;

  if((underlay = zmapWindowContainerGetUnderlay(container_group)))
    {
      underlay->flags.max_width  = maximise_width ;
      underlay->flags.max_height = maximise_height ;
    }

  return ;
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
//  group->item_list = NULL; /* nor this */
  return ;
}


/* Recursion... */

/* unified way to descend and do things to ALL and every or just some */
void zmapWindowContainerUtilsExecuteFull(ZMapWindowContainerGroup   container_group,
					 ZMapContainerLevelType     stop_at_type,
					 ZMapContainerUtilsExecFunc container_enter_cb,
					 gpointer                   container_enter_data,
					 ZMapContainerUtilsExecFunc container_leave_cb,
					 gpointer                   container_leave_data,
					 gboolean                   redraw_during_recursion)
{
  ContainerRecursionDataStruct data  = {0,NULL};
  ZMapWindowCanvas zmap_canvas;
  FooCanvasItem *parent;

  zMapAssert(stop_at_type >= ZMAPCONTAINER_LEVEL_ROOT &&
	     stop_at_type <= ZMAPCONTAINER_LEVEL_FEATURESET) ;

  parent = (FooCanvasItem *)container_group;

  data.stop                    = stop_at_type;
  data.redraw_during_recursion = redraw_during_recursion;

  data.container_enter_cb   = container_enter_cb;
  data.container_enter_data = container_enter_data;
  data.container_leave_cb   = container_leave_cb;
  data.container_leave_data = container_leave_data;

  if(redraw_during_recursion && ZMAP_IS_CANVAS(parent->canvas))
    {
      zmap_canvas = ZMAP_CANVAS(parent->canvas);
      zMapWindowCanvasBusy(zmap_canvas);
    }

  eachContainer((gpointer)container_group, &data) ;

  if(redraw_during_recursion && ZMAP_IS_CANVAS(parent->canvas))
    zMapWindowCanvasUnBusy(zmap_canvas);

  /* RDS_CANT_CROP_LONG_ITEMS_HERE
   * This seems like a good idea, but is flawed in a number of ways, here's 2:
   * 1) The scroll region can move after this call... needs to be done again.
   * 2) The scroll region results in unclamped coords meaning extra unneccessary
   *    calls to the guts in LongItemCrop.
   */

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
				      NULL, NULL, FALSE);
  return ;
}



/* look up a group in the the item_list
 * rather tediously the canvas is structured differently than the feature context and FToIHash
 * - it has a strand layer under block
 * - 3-frame columns all have the same id, whereas in the FToI hash the names are mangled
 */
static ZMapWindowContainerGroup getChildById(ZMapWindowContainerGroup group,GQuark id, ZMapFrame frame)
{
      ZMapWindowContainerGroup g;
      GList *l;
      FooCanvasGroup *children = (FooCanvasGroup *) zmapWindowContainerGetFeatures(group) ;
            /* this gets the group one of the four item positions */

      for(l = children->item_list;l;l = l->next)
      {
            g = (ZMapWindowContainerGroup) l->data;

            if(g->level == ZMAPCONTAINER_LEVEL_STRAND)
            {
                  /* has no feature_any */
                  ZMapWindowContainerStrand s = (ZMapWindowContainerStrand) g;

                  if(s->strand == id)
                        return(g);
            }
            else if(g->level == ZMAPCONTAINER_LEVEL_FEATURESET)
            {
                  ZMapWindowContainerFeatureSet set = ZMAP_CONTAINER_FEATURESET(g);;

                  if(set->unique_id == id && set->frame == frame)
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
      group = getChildById(group,align,ZMAPFRAME_NONE);
  if(group)
      group = getChildById(group,block,ZMAPFRAME_NONE);
  if(group)
      group = getChildById(group,strand,ZMAPFRAME_NONE);
  if(group)
      group = getChildById(group,set,frame);

  return FOO_CANVAS_ITEM(group);
}


/* Internal */

static FooCanvasItem *container_get_child(ZMapWindowContainerGroup container, guint position)
{
  FooCanvasItem *child = NULL;

  if(ZMAP_IS_CONTAINER_GROUP(container))
    {
      FooCanvasGroup *group;
      GList *list;

      group = FOO_CANVAS_GROUP(container);

      if((list = g_list_nth(group->item_list, position)))
	{
	  child = FOO_CANVAS_ITEM(list->data);
	}
    }

  return child;
}



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

  /* If we're redrawing then we need to do extra work... */
  if (all_data->redraw_during_recursion)
    {
//      ZMapWindowContainerBackground container_background;

//      container_background = zmapWindowContainerGetBackground(container);

      switch(level)
        {
        case ZMAPCONTAINER_LEVEL_ROOT:
	  zmapWindowContainerRequestReposition(container);
          break;
        case ZMAPCONTAINER_LEVEL_ALIGN:
	  zmapWindowContainerRequestReposition(container);
          break;
        case ZMAPCONTAINER_LEVEL_BLOCK:
	  zmapWindowContainerRequestReposition(container);
          break;
        case ZMAPCONTAINER_LEVEL_STRAND:
	  zmapWindowContainerRequestReposition(container);
          break;
        case ZMAPCONTAINER_LEVEL_FEATURESET:
          {
            /* If this featureset requires a redraw... */
            if (children && container->flags.column_redraw)
              {
		//redrawColumn(container, this_points);
                container->height = 0.0; /* reset, although, maybe not sensible. see maximise_container_background */
              }

          }
          break;
        case ZMAPCONTAINER_LEVEL_INVALID:
        default:
          break;
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

