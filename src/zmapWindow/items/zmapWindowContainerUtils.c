/*  File: zmapWindowContainerUtils.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Jun  3 21:47 2009 (rds)
 * Created: Tue Apr 28 16:10:46 2009 (rds)
 * CVS info:   $Id: zmapWindowContainerUtils.c,v 1.2 2009-06-03 22:29:08 rds Exp $
 *-------------------------------------------------------------------
 */

#include <zmapWindowCanvas.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerGroup_I.h>
#include <zmapWindowContainerChildren_I.h>
#include <zmapWindowContainerFeatureSet_I.h>
#include <zmapWindowContainerStrand_I.h> /* access to ZMapWindowContainerStrand->strand */
#include <zmapWindowContainerBlock.h>
#include <zmapWindowContainerAlignment.h>
#include <zmapWindowContainerContext.h>

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


FooCanvasGroup *zmapWindowContainerCreate(FooCanvasGroup *parent,
					  ZMapContainerLevelType level,
					  double child_spacing,
					  GdkColor *background_fill_colour,
					  GdkColor *background_border_colour,
					  ZMapWindowLongItems long_items)
{
  ZMapWindowContainerGroup container_group;
  FooCanvasGroup *new_container = NULL;
  
  if((container_group = zmapWindowContainerGroupCreate(parent, level, child_spacing, 
						       background_fill_colour, 
						       background_border_colour)))
    {
      new_container = (FooCanvasGroup *)container_group;
    }

  return new_container;
}


gboolean zmapWindowContainerUtilsIsValid(FooCanvasGroup *any_group)
{
  gboolean valid = FALSE;

  valid = ZMAP_IS_CONTAINER_GROUP(any_group);

  return valid;
}

/* gross tree access. any item -> container group */

ZMapWindowContainerGroup zmapWindowContainerChildGetParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  
  if(ZMAP_IS_CONTAINER_GROUP(item))
    container_group = ZMAP_CONTAINER_GROUP(item);
  else if(ZMAP_IS_CONTAINER_FEATURES(item)   || 
	  ZMAP_IS_CONTAINER_BACKGROUND(item) ||
	  ZMAP_IS_CONTAINER_OVERLAY(item)    ||
	  ZMAP_IS_CONTAINER_UNDERLAY(item))
    {
      if((item = item->parent) && ZMAP_IS_CONTAINER_GROUP(item))
	container_group = ZMAP_CONTAINER_GROUP(item);
    }
    
  return container_group;
}

ZMapWindowContainerGroup zmapWindowContainerGetNextParent(FooCanvasItem *item)
{
  ZMapWindowContainerGroup container_group = NULL;
  
  if((container_group = zmapWindowContainerChildGetParent(item)))
    {
      ZMapWindowContainerGroup tmp_group;
      FooCanvasItem *parent = NULL;
      FooCanvasItem *tmp_item;

      tmp_group = container_group;
      tmp_item  = FOO_CANVAS_ITEM(container_group);

      container_group = NULL;

      if(tmp_group->level == ZMAPCONTAINER_LEVEL_ROOT)
	parent = FOO_CANVAS_ITEM(tmp_group);
      else if(tmp_item->parent && 
	      tmp_item->parent->parent)
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

  if(ZMAP_IS_CONTAINER_GROUP(item))
    container_item = item;
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
    container_group = ZMAP_CONTAINER_GROUP(container_item);
  else
    zMapLogWarning("Failed to find container in path root -> ... -> item (%p).", item);

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

ZMapWindowContainerFeatures zmapWindowContainerGetFeatures(ZMapWindowContainerGroup container)
{
  ZMapWindowContainerFeatures features = NULL;
  FooCanvasItem *item;

  if((item = container_get_child(container, _CONTAINER_FEATURES_POSITION)))
    {
      features = ZMAP_CONTAINER_FEATURES(item);
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

  if(ZMAP_IS_CONTAINER_STRAND(container))
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
	  if((status = ZMAP_IS_CONTAINER_FEATURESET(container)))
	    {
	      status = zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)container,
								     (ZMapFeatureSet)feature_any);
	    }
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
	  g_object_set_data(G_OBJECT(container), ITEM_FEATURE_DATA, feature_any);
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
  GList *list;
  
  if((list = g_list_first(group->item_list)))
    {
      do
	{
	  GtkObject *gtk_item_object;
	  
	  gtk_item_object = GTK_OBJECT(list->data);
	  
	  list = list->next;
	  
	  gtk_object_destroy(gtk_item_object);
	}
      while((list));
    }
  
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

  if(redraw_during_recursion)
    {
      zmap_canvas = ZMAP_CANVAS(parent->canvas);
      zMapWindowCanvasBusy(zmap_canvas);
    }

  eachContainer((gpointer)container_group, &data) ;

  if(redraw_during_recursion)
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
      ZMapWindowContainerBackground container_background;

      container_background = zmapWindowContainerGetBackground(container);

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


