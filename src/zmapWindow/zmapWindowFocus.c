/*  File: zmapWindowFocus.c
 *  Author: Ed Griffiths edgrif@sanger.ac.uk
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
 * Description: Code to implement "focus" items on the canvas.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Apr 30 11:00 2010 (edgrif)
 * Created: Tue Jan 16 09:46:23 2007 (rds)
 * CVS info:   $Id: zmapWindowFocus.c,v 1.20 2010-06-14 15:40:16 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerFeatureSet_I.h>

/* Used for window focus items and column.
 *
 * Note that if only a column has been selected then focus_item_set may be NULL, BUT
 * if an item has been selected then focus_column will be that items parent.
 *
 *  */
typedef struct _ZMapWindowFocusStruct
{
  /* the selected/focused items. Interesting operations on these should be possible... */
  GList *focus_item_set ;

  int hot_item_orig_index ;				    /* Record where hot_item was in its list. */

  ZMapWindowContainerFeatureSet focus_column ;

  GList *overlay_managers;
} ZMapWindowFocusStruct ;


typedef struct
{
  FooCanvasItem *result;
  ZMapFrame frame;
}MatchFrameStruct, *MatchFrame;



static gint find_item(gconstpointer list_data, gconstpointer user_data);
static ZMapWindowFocusItemArea ensure_unique(ZMapWindowFocus focus,
                                             FooCanvasItem *item);
static void addFocusItemCB(gpointer data, gpointer user_data) ;
static void freeFocusItems(ZMapWindowFocus focus) ;
static void setFocusColumn(ZMapWindowFocus focus, FooCanvasGroup *column) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void match_frame(gpointer list_data, gpointer user_data);
static FooCanvasItem *get_item_with_matching_frame(FooCanvasItem *any_item, FooCanvasItem *feature_item);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void mask_in_overlay_swap(gpointer list_data, gpointer user_data);
static void mask_in_overlay(gpointer list_data, gpointer user_data);
static void set_default_highlight_colour(gpointer list_data, gpointer user_data);

static void invoke_overlay_unmask_all(gpointer overlay_data, gpointer unused_data);
static void FocusUnmaskOverlay(ZMapWindowFocus focus);

static gboolean overlay_manager_list_debug_G = FALSE;

/*
 *              Set of routines to handle focus items.
 *
 * Holds a list of items that are highlighted/focussed, one of these is the "hot" item
 * which is the last one selected by the user.
 *
 * The first item in the list is always the "hot" item.
 *
 * The "add" functions just append item(s) to the list, they do not remove any items from it.
 *
 * The "hot" item must be set with a separate call.
 *
 * To replace the list you need to free it first then add new item(s).
 *
 * Note that these routines do not handle highlighting of items, that must be done with
 * a separate call.
 *
 */



ZMapWindowFocus zmapWindowFocusCreate(void)
{
  ZMapWindowFocus focus ;

  focus = g_new0(ZMapWindowFocusStruct, 1) ;

  return focus ;
}

/* N.B. unless there are no items, then item is added to end of list,
 * it is _not_ the new hot item so we do not reset the focus column for instance. */
void zmapWindowFocusAddItem(ZMapWindowFocus focus, FooCanvasItem *item)
{

  if (!focus->focus_item_set)
    zmapWindowFocusSetHotItem(focus, item, TRUE) ;
  else
    addFocusItemCB((gpointer)item, (gpointer)focus) ;

  return ;
}

/* Same remark applies to this routine as to zmapWindowFocusAddItem() */
void zmapWindowFocusAddItems(ZMapWindowFocus focus, GList *item_list)
{

  /* If there is no focus item, make the first item in the list the hot focus item and
   * move the list on one. */
  if (!focus->focus_item_set)
    {
      zmapWindowFocusSetHotItem(focus, FOO_CANVAS_ITEM(item_list->data), TRUE) ;
      item_list = g_list_next(item_list) ;
    }

  if (item_list)
    g_list_foreach(item_list, addFocusItemCB, focus) ;

  return ;
}


/* Is the given item in the hot focus column ? */
/* SHOULD BE RENAMED TO BE _ANY_ ITEM AS IT CAN INCLUDE BACKGROUND ITEMS.... */
gboolean zmapWindowFocusIsItemInHotColumn(ZMapWindowFocus focus, FooCanvasItem *item)
{
  gboolean result = FALSE ;

  if (focus->focus_column)
    {
      ZMapWindowContainerGroup column_container;

      column_container = (ZMapWindowContainerGroup)focus->focus_column;

      if (column_container == zmapWindowContainerCanvasItemGetContainer(item))
	result = TRUE ;
    }

  return result ;
}



void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item, gboolean remove_hot_item)
{
  FooCanvasItem *curr_focus_item ;
  ZMapWindowFocusItemArea item_area ;
  FooCanvasGroup *column ;

  if (remove_hot_item)
    {
      /* We should be returning items to their original position here.... */
      if ((curr_focus_item = zmapWindowFocusGetHotItem(focus)))
	{
	  zmapWindowFocusRemoveFocusItem(focus, curr_focus_item) ;
	}
    }

  item_area = ensure_unique(focus, item);

  /* Stick the item on the front. */
  focus->focus_item_set = g_list_prepend(focus->focus_item_set, item_area) ;

  /* Set the focus items column as the focus column. */
  column = (FooCanvasGroup *) zmapWindowContainerCanvasItemGetContainer(item) ;

  setFocusColumn(focus, column) ;			    /* N.B. May sort features. */

  /* Record where the item is in the stack of column items _after_ setFocusColumn. */
  focus->hot_item_orig_index = zmapWindowContainerGetItemPosition(ZMAP_CONTAINER_GROUP(column), item) ;

  /* Now raise the item to the top of its group to make sure it is visible. */
  zmapWindowRaiseItem(item) ;

  return ;
}


/* this one is different, a new column can be set _without_ setting a new item, so we
 * need to get rid of the old items. */
void zmapWindowFocusSetHotColumn(ZMapWindowFocus focus, FooCanvasGroup *column)
{
  freeFocusItems(focus) ;

  setFocusColumn(focus, column) ;

  return ;
}


FooCanvasItem *zmapWindowFocusGetHotItem(ZMapWindowFocus focus)
{
  FooCanvasItem *item = NULL ;
  ZMapWindowFocusItemArea item_area;
  GList *first ;

  if (focus->focus_item_set && (first = g_list_first(focus->focus_item_set)))
    {
      item_area = (ZMapWindowFocusItemArea)(first->data) ;
      item = item_area->focus_item;
    }

  return item ;
}

GList *zmapWindowFocusGetFocusItems(ZMapWindowFocus focus)
{
  GList *areas = NULL, *items = NULL;
  ZMapWindowFocusItemArea area = NULL;

  if (focus->focus_item_set)
    {
      areas = g_list_first(focus->focus_item_set) ;
      while(areas)
        {
          area = (ZMapWindowFocusItemArea)(areas->data);
          items = g_list_append(items, area->focus_item);
          areas = areas->next;
        }
    }

  return items ;
}


FooCanvasGroup *zmapWindowFocusGetHotColumn(ZMapWindowFocus focus)
{
  FooCanvasGroup *column = NULL ;

  column =(FooCanvasGroup *)( focus->focus_column );

  return column ;
}

/*!
 * zmapWindowFocusForEachFocusItem
 *
 * Call given user function for all highlighted items.
 * The data passed to the function will be a ZMapWindowFocusItemArea
 *
 * void callback(gpointer item_area_data, gpointer user_data);
 *
 */
void zmapWindowFocusForEachFocusItem(ZMapWindowFocus focus, GFunc callback, gpointer user_data)
{
  if (focus->focus_item_set)
    g_list_foreach(focus->focus_item_set, callback, user_data) ;

  return ;
}



/* Remove single item, a side effect is that if this is the hot item then we simply
 * make the next item in the list a hot item. */
void zmapWindowFocusRemoveFocusItem(ZMapWindowFocus focus, FooCanvasItem *item)
{
  ZMapWindowFocusItemArea gonner;
  GList *remove;

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set)
    {
      FooCanvasGroup  *container_parent ;

      int curr_index, position ;


      if ((remove = g_list_find_custom(focus->focus_item_set, item, find_item)))
        {
          gonner = (ZMapWindowFocusItemArea)(remove->data) ;

          focus->focus_item_set = g_list_remove(focus->focus_item_set, gonner) ;

          FocusUnmaskOverlay(focus) ;

          zmapWindowFocusItemAreaDestroy(gonner) ;


	  /* Try to put it back in its original position, note if this function is called during destroy
	   * the features list may already have gone so we cannot put it back.... */
	  container_parent = FOO_CANVAS_GROUP(zmapWindowContainerCanvasItemGetContainer(item)) ;

	  if ((curr_index = zmapWindowContainerGetItemPosition(ZMAP_CONTAINER_GROUP(container_parent), item)) != -1)
	    {
	      position = curr_index - focus->hot_item_orig_index ;

	      if ((zmapWindowContainerSetItemPosition(ZMAP_CONTAINER_GROUP(container_parent),
						      item, focus->hot_item_orig_index)) == -1)
		zMapLogWarning("%s", "zmapWindowContainerSetItemPosition() could not find item in container_parent.") ;

	      focus->hot_item_orig_index = 0 ;
	    }

        }
    }

  return ;
}


void zmapWindowFocusReset(ZMapWindowFocus focus)
{
  freeFocusItems(focus) ;

  focus->focus_column = NULL ;

  zmapWindowFocusClearOverlayManagers(focus);

  return ;
}

/* We pass in the default from the window->colour_item_highlight in case there's no other default */
/* Actually I've now put one in the zmapFeatureTypes.c file, so this is probably useless and confusing */

/* Foreach of the overlay managers that a given focus object has, run
 * mask_in_overlay for the item, which is presumably the highlighted
 * item! */
void zmapWindowFocusMaskOverlay(ZMapWindowFocus focus, FooCanvasItem *item, GdkColor *highlight_colour)
{
  return ;

  if(highlight_colour)
    g_list_foreach(focus->overlay_managers, set_default_highlight_colour, highlight_colour);

  g_list_foreach(focus->overlay_managers, mask_in_overlay, item);

  return ;
}

void zmapWindowFocusAddOverlayManager(ZMapWindowFocus focus, ZMapWindowOverlay overlay)
{
  if(overlay_manager_list_debug_G)
    zMapLogWarning("adding overlay_manager %p to focus %p", overlay, focus);

  focus->overlay_managers = g_list_append(focus->overlay_managers, overlay);

  return ;
}

void zmapWindowFocusRemoveOverlayManager(ZMapWindowFocus focus, ZMapWindowOverlay overlay)
{
  if(overlay_manager_list_debug_G)
    zMapLogWarning("removing overlay_manager %p from focus %p", overlay, focus);

  focus->overlay_managers = g_list_remove(focus->overlay_managers, overlay);

  return ;
}

void zmapWindowFocusClearOverlayManagers(ZMapWindowFocus focus)
{
  if(overlay_manager_list_debug_G)
    zMapLogWarning("Removing all overlay_managers from focus %p", focus);

  if(focus->overlay_managers)
    g_list_free(focus->overlay_managers);

  focus->overlay_managers = NULL;

  return ;
}

void zmapWindowFocusDestroy(ZMapWindowFocus focus)
{
  zMapAssert(focus) ;

  freeFocusItems(focus) ;

  focus->focus_column = NULL ;

  g_free(focus) ;

  return ;
}

ZMapWindowFocusItemArea zmapWindowFocusItemAreaCreate(FooCanvasItem *item)
{
  ZMapWindowFocusItemArea item_area;

  if(!(item_area = g_new0(ZMapWindowFocusItemAreaStruct, 1)))
    {
      zMapAssertNotReached();
    }
  else
    item_area->focus_item = item;

  return item_area;
}

void zmapWindowFocusItemAreaDestroy(ZMapWindowFocusItemArea item_area)
{
  item_area->focus_item = NULL;

  if(item_area->associated)
    zmapWindowOverlayDestroy(item_area->associated);

  g_free(item_area);

  return ;
}


/* INTERNAL */

static ZMapWindowFocusItemArea ensure_unique(ZMapWindowFocus focus, FooCanvasItem *item)
{
  ZMapWindowFocusItemArea item_area;
  gpointer associated = NULL;
  GList *remove;

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set)
    {
      if((remove = g_list_find_custom(focus->focus_item_set, item, find_item)))
        {
          item_area = remove->data;
          focus->focus_item_set = g_list_remove(focus->focus_item_set,
                                                remove->data);
          associated = item_area->associated;
          item_area->associated = NULL;
          zmapWindowFocusItemAreaDestroy(item_area);
        }
    }

  item_area = zmapWindowFocusItemAreaCreate(item);
  zMapAssert(item_area);

  if(associated != NULL)
    item_area->associated = associated;

  return item_area;
}

static gint find_item(gconstpointer list_data, gconstpointer user_data)
{
  ZMapWindowFocusItemArea area = (ZMapWindowFocusItemArea)list_data;
  gint result = -1;

  if(user_data == (gconstpointer)(area->focus_item))
    result = 0;

  return result;
}

/* A GFunc() to add all items in a list to the windows focus item list. We don't want duplicates
 * so we try to remove an item first and then append it. This is a potential performance
 * problem if there is a _huge_ list of focus items....
 * NB: append requires processing the whole list
 *  */
static void addFocusItemCB(gpointer data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)data ;
  ZMapWindowFocus focus = (ZMapWindowFocus)user_data ;
  ZMapWindowFocusItemArea item_area;

  item_area = ensure_unique(focus, item);

  focus->focus_item_set = g_list_append(focus->focus_item_set, item_area) ;

  return ;
}



static void freeFocusItems(ZMapWindowFocus focus)
{
  if (focus->focus_item_set)
    {
      g_list_free(focus->focus_item_set) ;

      focus->focus_item_set = NULL ;
    }

  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* g_list_foreach to get a feature item with a matching frame. */
static void match_frame(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *feature_item = FOO_CANVAS_ITEM(list_data);
  MatchFrame data = (MatchFrame)user_data;
  ZMapFeature feature;

  if((feature = zmapWindowItemGetFeature(feature_item)))
    {
      ZMapFrame feature_item_frame = zmapWindowFeatureFrame(feature);

      if(!data->result && data->frame == feature_item_frame)
        data->result = feature_item;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* search for a feature item in the container parent
 * of any_item that has the same frame as feature_item. */
static FooCanvasItem *get_item_with_matching_frame(FooCanvasItem *any_item,
                                                   FooCanvasItem *feature_item)
{
  FooCanvasItem *same_frame_item = NULL;
  FooCanvasGroup  *container_parent, *container_features = NULL;
  ZMapWindowItemFeatureType item_feature_type;
  ContainerType container_type;

  /* possibly bad to have these 2 here, but wanted to not crash, but instead return NULL. */
  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(any_item), ITEM_FEATURE_TYPE));
  container_type    = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(any_item), CONTAINER_TYPE_KEY));

  if((item_feature_type != ITEM_FEATURE_INVALID) &&
     (container_parent = zmapWindowContainerGetParentContainerFromItem(any_item)))
    {
      container_features = zmapWindowContainerGetFeatures(container_parent);
    }
  else if((container_type != CONTAINER_INVALID) &&
          (container_parent = zmapWindowContainerGetParent(any_item)))
    {
      container_features = zmapWindowContainerGetFeatures(container_parent);
    }
  else
    {
      container_features = NULL;
    }

  if(container_features)
    {
      ZMapFeature feature;

      if((feature = zmapWindowItemGetFeature(feature_item)))
        {
          MatchFrameStruct match = {NULL};

          match.frame = zmapWindowFeatureFrame(feature);

          g_list_foreach(FOO_CANVAS_GROUP(container_features)->item_list, match_frame, &match);

          same_frame_item = match.result;
        }
    }

  return same_frame_item;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* swap round the parameters and call mask_in_overlay */
static void mask_in_overlay_swap(gpointer list_data, gpointer user_data)
{
  mask_in_overlay(user_data, list_data);

  return ;
}

/* Do _all_ that is required to overlay/highlight some text.  Note
 * this is a g_list_foreach function that recursively calls itself,
 * via mask_in_overlay_swap. */
static void mask_in_overlay(gpointer list_data, gpointer user_data)
{
  ZMapWindowOverlay overlay = (ZMapWindowOverlay)list_data;

  if(FOO_IS_CANVAS_GROUP(user_data))
    {
      FooCanvasGroup *group = FOO_CANVAS_GROUP(user_data);
      g_list_foreach(group->item_list, mask_in_overlay_swap, list_data);
    }
  else if(FOO_IS_CANVAS_ITEM(user_data))
    {
      FooCanvasItem *item = FOO_CANVAS_ITEM(user_data);

      zmapWindowOverlaySetSubject(overlay, item);
      zmapWindowOverlayMask(overlay);
    }

  return ;
}

static void invoke_overlay_unmask_all(gpointer overlay_data, gpointer unused_data)
{
  ZMapWindowOverlay overlay = (ZMapWindowOverlay)overlay_data;

  zmapWindowOverlayUnmaskAll(overlay);

  return ;
}

static void FocusUnmaskOverlay(ZMapWindowFocus focus)
{
  g_list_foreach(focus->overlay_managers, invoke_overlay_unmask_all, NULL);

  return ;
}


/* This is a pain in the backside! */
static void set_default_highlight_colour(gpointer list_data, gpointer user_data)
{
  ZMapWindowOverlay overlay = (ZMapWindowOverlay)list_data;
  GdkColor *colour = (GdkColor *)user_data;

  zmapWindowOverlaySetGdkColorFromGdkColor(overlay, colour);

  return ;
}


static void setFocusColumn(ZMapWindowFocus focus, FooCanvasGroup *column)
{
  ZMapWindowContainerFeatureSet container;

  if(ZMAP_IS_CONTAINER_FEATURESET(column))
    {
      focus->focus_column =container = (ZMapWindowContainerFeatureSet)column ;


      if (!container->sorted)
	{
	  //zmapWindowContainerFeatureSetSortFeatures(focus->focus_column, ZMAPCONTAINER_VERTICAL) ;
	  zmapWindowContainerFeatureSetSortFeatures(container, 0) ;

	  container->sorted = TRUE ;
	}
    }

  return ;
}



