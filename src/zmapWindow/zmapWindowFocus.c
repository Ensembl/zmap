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
 * Last edited: Jul 29 10:58 2010 (edgrif)
 * Created: Tue Jan 16 09:46:23 2007 (rds)
 * CVS info:   $Id: zmapWindowFocus.c,v 1.25 2010-10-26 15:46:23 mh17 Exp $
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
 * MH17: subverted to handle lists of evidence features from several columns
 * which are to be highlit on a semi-permanent basis
 * Normal focus operations require a global focus column and these are preserved as was
 *
 * evidence features (or other lists of features) may come from many columns.
 */
typedef struct _ZMapWindowFocusStruct
{
  ZMapWindow window;     // retrograde pointer, we need it for highlighting

  /* the selected/focused items. Interesting operations on these should be possible... */
  GList *focus_item_set ;


  ZMapWindowContainerFeatureSet focus_column ;
  FooCanvasItem *hot_item;           // current hot focus item or NULL
  int hot_item_orig_index ;          /* Record where hot_item was in its list. */
  /* mh17: this is of limited use as it works only for single items; not used
   * and not comapatble with raising focus item to the top and loweiring previous to the bottom
   * (see zMapWindowCanvasItemSetIntervalColours())
   */


  GList *overlay_managers;
} ZMapWindowFocusStruct ;


typedef struct _ZMapWindowFocusItemStruct
{
      FooCanvasItem *item;

      ZMapWindowContainerFeatureSet item_column ;   // column if on display: interacts w/ focus_column

      // next one not used yet
      ZMapFeatureSet featureset;                    // featureset it lives in (esp if not displayed)

      int flags;                                    // a bitmap of focus types and misc data
      int display_state;
            // selected flags if any
            // all states need to be kept as more than one may be visible
            // as some use fill and some use border

// if needed add this for data structs for any group
//      void *data[N_FOCUS_GROUPS];

// from the previous ZMapWindowFocusItemAreaStruct
//  gboolean        highlighted;    // this was used but never unset
//  gpointer        associated;     // this had set values preserved but never set

} ZMapWindowFocusItemStruct, *ZMapWindowFocusItem;



typedef struct
{
  FooCanvasItem *result;
  ZMapFrame frame;
}MatchFrameStruct, *MatchFrame;


ZMapWindowFocusItem zmapWindowFocusItemCreate(FooCanvasItem *item);
void zmapWindowFocusItemDestroy(ZMapWindowFocusItem list_item);



static ZMapWindowFocusItem add_unique(ZMapWindowFocus focus,FooCanvasItem *item,
      ZMapWindowFocusType type);

static void freeFocusItems(ZMapWindowFocus focus, ZMapWindowFocusType type);
static void setFocusColumn(ZMapWindowFocus focus, FooCanvasGroup *column) ;



static void highlightCB(gpointer data, gpointer user_data) ;
static void highlightItem(ZMapWindow window, ZMapWindowFocusItem item) ;
static void rehighlightFocusCB(gpointer list_data, gpointer user_data) ;


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
 *    extended by mh17 to handle evidence features highlighting too.
 *
 * Holds a list of items that are highlighted/focussed, one of these is the "hot" item
 * which is the last one selected by the user.
 *
 * The hot item is in the list but is alos pointed at directly.
 *
 * The "add" functions just append item(s) to the list, they do not remove any items from it.
 *
 * The "hot" item must be set with a separate call.
 *
 * To replace the list you need to free it first then add new item(s).
 *
 * Items appear in the list once only and have flags to indicate which group(s)
 *
 * Note that these routines do not handle highlighting of items, that must be done with
 * a separate call.
 *
 * NOTE removal of previous focus items is done by
 * zmapWindowItem,c/zMapWindowUnHighlightFocusItems()
 * which is called via zmapWindowHighlightObject()
 * and not the SetHotItem function in this file which had code to do this that was never run
 */

int focus_group_mask[] = { 1,2,4,8,16,32,64,128,258 };
// indexed by ZMapWindowFocusType enum
// item flags can be stored in higher bits - define here
#define WINDOW_FOCUS_GROUP_ALL 0xff

#if N_FOCUS_GROUPS > 8
#error array too short: FIX THIS
// (right now there are only 3)
#endif


gboolean zmapWindowFocusHasType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *l;
  ZMapWindowFocusItem fl;

  if(type == WINDOW_FOCUS_GROUP_ALL)
    return(focus->focus_item_set != NULL);

  for(l = focus->focus_item_set;l;l = l->next)
    {
      fl  = (ZMapWindowFocusItem) l->data;

      if(fl->flags & focus_group_mask[type])
	return(TRUE);
    }

  return(FALSE);
}


ZMapWindowFocus zmapWindowFocusCreate(ZMapWindow window)
{
  ZMapWindowFocus focus ;

  focus = g_new0(ZMapWindowFocusStruct, 1) ;
  focus->window = window;

  return focus ;
}

/* N.B. unless there are no items, then item is added to end of list,
 * it is _not_ the new hot item so we do not reset the focus column for instance. */
void zmapWindowFocusAddItemType(ZMapWindowFocus focus, FooCanvasItem *item, ZMapWindowFocusType type)
{
  if(item)  // in case we add GetHotItem() which could return NULL
    {
      add_unique(focus,item,type);

      if (!focus->hot_item && type == WINDOW_FOCUS_GROUP_FOCUS)
        zmapWindowFocusSetHotItem(focus, item) ;
     }

  return ;
}

/* Same remark applies to this routine as to zmapWindowFocusAddItem() */
void zmapWindowFocusAddItemsType(ZMapWindowFocus focus, GList *list, FooCanvasItem *hot, ZMapWindowFocusType type)
{
  FooCanvasItem *first = NULL;

  if (list)
    first = FOO_CANVAS_ITEM (list->data);

  for (; list ; list = list->next)
    {
      add_unique(focus,FOO_CANVAS_ITEM(list->data),type);
    }

  if (!focus->hot_item && first && type == WINDOW_FOCUS_GROUP_FOCUS)
    zmapWindowFocusSetHotItem(focus, hot) ;


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



// set the hot item, which is already in the focus set
// NB: this is only called from this file
// previously this function removed existing focus but only if was never set
void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item)
{
  FooCanvasGroup *column ;

  focus->hot_item = item;

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
  freeFocusItems(focus,WINDOW_FOCUS_GROUP_FOCUS) ;

  focus->hot_item = NULL;
  setFocusColumn(focus, column) ;

  return ;
}


/* highlight/unhiglight cols. */
void zmapWindowFocusHighlightHotColumn(ZMapWindowFocus focus)
{
  FooCanvasGroup *hot_column;

  if ((hot_column = zmapWindowFocusGetHotColumn(focus)))
  {
    zmapWindowContainerGroupSetBackgroundColour(ZMAP_CONTAINER_GROUP(hot_column),
             &(focus->window->colour_column_highlight)) ;
  }

  return ;
}


void zmapWindowFocusUnHighlightHotColumn(ZMapWindowFocus focus)
{
  FooCanvasGroup *hot_column;

  if ((hot_column = zmapWindowFocusGetHotColumn(focus)))
      zmapWindowContainerGroupResetBackgroundColour(ZMAP_CONTAINER_GROUP(hot_column)) ;

  return ;
}




FooCanvasItem *zmapWindowFocusGetHotItem(ZMapWindowFocus focus)
{
  FooCanvasItem *item;

  item = focus->hot_item;

  return item ;
}

GList *zmapWindowFocusGetFocusItemsType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *lists = NULL, *items = NULL;
  ZMapWindowFocusItem list = NULL;
  int mask = focus_group_mask[type];

  if (focus->focus_item_set)
    {
      lists = g_list_first(focus->focus_item_set) ;
      while(lists)
        {
          list = (ZMapWindowFocusItem)(lists->data);
          if((list->flags & mask))
            items = g_list_append(items, list->item);
          lists = lists->next;
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
 * The data passed to the function will be a ZMapWindowFocusItem
 *
 * void callback(gpointer list_item_data, gpointer user_data);
 *
 */
void zmapWindowFocusForEachFocusItemType(ZMapWindowFocus focus, ZMapWindowFocusType type,
            GFunc callback, gpointer user_data)
{
  GList *l;
  ZMapWindowFocusItem list_item;

  for(l = focus->focus_item_set;l;l = l->next)
    {
      list_item = (ZMapWindowFocusItem) l->data;
      if(type == WINDOW_FOCUS_GROUP_ALL || (list_item->flags & focus_group_mask[type]))
            callback((gpointer) list_item,user_data);
    }

  return ;
}


// NB: never called
void zmapWindowFocusRehighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window)
{
  zmapWindowFocusForEachFocusItemType(focus,WINDOW_FOCUS_GROUP_FOCUS, rehighlightFocusCB, window) ;

  return ;
}


void zmapWindowFocusHighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window)
{
    zmapWindowFocusForEachFocusItemType(focus, WINDOW_FOCUS_GROUP_FOCUS, highlightCB, window) ;
}


void zmapWindowFocusUnhighlightFocusItems(ZMapWindowFocus focus, ZMapWindow window)
{
      // this unhighlights on free
    freeFocusItems(focus,WINDOW_FOCUS_GROUP_FOCUS);
}




/* Remove single item, a side effect is that if this is the hot item
 * then we find the first one in the list
 * so no point in replacing the item in its original position
 * esp w/ shift select being possible, previous code only handled one item
 *
 * NOTE need to replace the item back where it was to handle feature tabbing
 * this means we need to handle many items moving around not just one focus item
 */

/* MH17: this may be called from a destroy object callback in which case the canvas is not in a safe state
 * the assumption in this code if that the focus is removed from an object while it still exists
 * but alignments appear to call this whenthioer parent has dissappeared causing a crash
 * wghich is a bug in the canvas item code not here
 * The fix is to add another option 'destroy' which means 'don't highlight it because it's not there'
 */

void zmapWindowFocusRemoveFocusItemType(ZMapWindowFocus focus,
					FooCanvasItem *item, ZMapWindowFocusType type, gboolean unhighlight)
{
  ZMapWindowFocusItem gonner,data;
  GList *remove,*l;

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set && item)
    {
      for (remove = focus->focus_item_set;remove; remove = remove->next)
	{
	  gonner = (ZMapWindowFocusItem) remove->data;
	  if (gonner->item == item)
            {

	      if(type == WINDOW_FOCUS_GROUP_ALL)
		gonner->flags &= ~WINDOW_FOCUS_GROUP_ALL;        // zap the lot
	      else
		gonner->flags &= ~focus_group_mask[type];        // remove from this group

	      if (unhighlight)
		highlightItem(focus->window,gonner);

	      FocusUnmaskOverlay(focus) ;   // (not sure what this is for)

	      if(!(gonner->flags & WINDOW_FOCUS_GROUP_ALL))   // no groups: remove from list
		{
		  zmapWindowFocusItemDestroy(gonner);
		  focus->focus_item_set = g_list_delete_link(focus->focus_item_set,remove);
		}
	      break;

            }
	}

      if(item == focus->hot_item && type == WINDOW_FOCUS_GROUP_FOCUS)
	{
	  focus->hot_item = NULL;
	  for(l = focus->focus_item_set;l;l = l->next)
            {
	      data = (ZMapWindowFocusItem) l->data;
	      if((data->flags & focus_group_mask[WINDOW_FOCUS_GROUP_FOCUS]))
		{
		  focus->hot_item = data->item;
		  break;
		}
            }
	}
    }

  return ;
}


void zmapWindowFocusResetType(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  freeFocusItems(focus,type) ;

  return ;
}


void zmapWindowFocusReset(ZMapWindowFocus focus)
{
  zmapWindowFocusResetType(focus,WINDOW_FOCUS_GROUP_FOCUS);

  zmapWindowFocusUnHighlightHotColumn(focus);

  focus->focus_column = NULL ;
  focus->hot_item = NULL;

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

  freeFocusItems(focus,WINDOW_FOCUS_GROUP_ALL) ;

  focus->focus_column = NULL ;

  g_free(focus) ;

  return ;
}

ZMapWindowFocusItem zmapWindowFocusItemCreate(FooCanvasItem *item)
{
  ZMapWindowFocusItem list_item;

  if(!(list_item = g_new0(ZMapWindowFocusItemStruct, 1)))
    {
      zMapAssertNotReached();
    }
  else
    list_item->item = item;

  return list_item;
}

void zmapWindowFocusItemDestroy(ZMapWindowFocusItem list_item)
{
  list_item->item = NULL;

//  if(list_item->associated)
//    zmapWindowOverlayDestroy(list_item->associated);

  g_free(list_item);

  return ;
}






/*
 *                           Internal routines.
 */



/* GFunc() to hide the given item and record it in the user hidden list. */
static void hideFocusItemsCB(gpointer data, gpointer user_data)
{
  ZMapWindowFocusItem list_item = (ZMapWindowFocusItem)data;
  FooCanvasItem *item = (FooCanvasItem *)list_item->item ;
  GList **list_ptr = (GList **)user_data ;
  GList *user_hidden_items = *list_ptr ;

  zMapAssert(FOO_IS_CANVAS_ITEM(item));

  foo_canvas_item_hide(item) ;

  user_hidden_items = g_list_append(user_hidden_items, item) ;

  *list_ptr = user_hidden_items ;

  return ;
}


void zmapWindowFocusHideFocusItems(ZMapWindowFocus focus, GList **hidden_items)
{
      zmapWindowFocusForEachFocusItemType(focus,WINDOW_FOCUS_GROUP_FOCUS, hideFocusItemsCB, hidden_items) ;
}


static void rehighlightFocusCB(gpointer list_data, gpointer user_data)
{
  ZMapWindowFocusItem data = (ZMapWindowFocusItem)list_data;
  FooCanvasItem *item = (FooCanvasItem *)data->item ;
  ZMapWindow window = (ZMapWindow)user_data ;
  GdkColor *highlight = NULL;

  if(window->highlights_set.item)
    highlight = &(window->colour_item_highlight);

  zmapWindowFocusMaskOverlay(window->focus, item, highlight);

  return ;
}

/* Do the right thing with groups and items
 * also does unhighlight and is called on free
 */
static void highlightItem(ZMapWindow window, ZMapWindowFocusItem item)
{
  GdkColor *fill = NULL, *border = NULL;
  int n_focus;
  GList *l;

  if((item->flags & WINDOW_FOCUS_GROUP_ALL) == item->display_state)
    return;

  if((item->flags & WINDOW_FOCUS_GROUP_ALL))
    {
      if((item->flags & focus_group_mask[WINDOW_FOCUS_GROUP_FOCUS]))
      {
            if(window->highlights_set.item)
                  fill = &(window->colour_item_highlight);

      }
      else if((item->flags & focus_group_mask[WINDOW_FOCUS_GROUP_EVIDENCE]))
      {
         if(window->highlights_set.evidence)
           {
             fill = &(window->colour_evidence_fill);
             border = &(window->colour_evidence_border);
           }
      }

      zMapWindowCanvasItemSetIntervalColours(item->item, ZMAPSTYLE_COLOURTYPE_SELECTED, fill, border);
      foo_canvas_item_raise_to_top(FOO_CANVAS_ITEM(item->item)) ;

    }
  else
    {
      zMapWindowCanvasItemSetIntervalColours(item->item, ZMAPSTYLE_COLOURTYPE_NORMAL, NULL,NULL);
      /* foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(item->item)) ;*/

      /* this is a pain: to keep ordering stable we have to put the focus item back where it was
       * so we have to comapre it wiht items not in the focus list
       */
      /* find out how many focus item there are in this item's column */
      for(n_focus = 0,l = window->focus->focus_item_set;l;l = l->next)
        {
            ZMapWindowFocusItem focus_item = (ZMapWindowFocusItem) l->data;

            if(item->item_column == focus_item->item_column)      /* count includes self */
                  n_focus++;
        }
      /* move the item back to where it should be */
      zmapWindowContainerFeatureSetItemLowerToMiddle(item->item_column, item->item, n_focus,0);
    }

   item->display_state = item->flags & WINDOW_FOCUS_GROUP_ALL;

  return ;
}


// handle highlighting for all groups
// maintain current state to avoid repeat highlighting
// which should not be a problem for features, but possibly so for overlay mamagers??
static void highlightCB(gpointer list_data, gpointer user_data)
{
  ZMapWindowFocusItem data = (ZMapWindowFocusItem)list_data;
  FooCanvasItem *item = (FooCanvasItem *)data->item ;
  ZMapWindow window = (ZMapWindow)user_data ;

  GdkColor *highlight = NULL;

  highlightItem(window, data) ;

  if(window->highlights_set.item)
    highlight = &(window->colour_item_highlight);

  zmapWindowFocusMaskOverlay(window->focus, item, highlight);

  return ;
}


// add a struct to the list, no duplicates
// return the list and a pointer to the struct
static ZMapWindowFocusItem add_unique(ZMapWindowFocus focus,
            FooCanvasItem *item,
            ZMapWindowFocusType type)
{
  GList *gl;
  ZMapWindowFocusItem list_item = NULL;

  for(gl = focus->focus_item_set;gl;gl = gl->next)
    {
      list_item = (ZMapWindowFocusItem) gl->data;
      if(list_item->item == item)
	break;
    }
  if(!gl)     // didn't find it
    {
      list_item = g_new0(ZMapWindowFocusItemStruct,1);
      focus->focus_item_set = g_list_prepend(focus->focus_item_set,list_item);
    }

  list_item->flags |= focus_group_mask[type];
  list_item->item = item;

  /* tricky -> a container is a foo canvas group
   * that has a fixed size list of foo canvas groups
   * that contain lists of features
   * that all have the same parent
   */
  list_item->item_column = (ZMapWindowContainerFeatureSet) FOO_CANVAS_ITEM(item)->parent;
  // also need to fill in featureset

  if (!list_item->item_column->sorted)
    {
      /* we need this for uh-highlight into stable ordering
       * for focus items it happens in setHotColumn
       * but if we select features eg via XRemote maybe it doesn't
       */
      zmapWindowContainerFeatureSetSortFeatures(list_item->item_column, 0) ;
      list_item->item_column->sorted = TRUE ;
    }


  highlightItem(focus->window,list_item);
  return list_item;
}





static void freeFocusItems(ZMapWindowFocus focus, ZMapWindowFocusType type)
{
  GList *l,*del;
  ZMapWindowFocusItem li;

  for(l = focus->focus_item_set; l;)
    {
      li = (ZMapWindowFocusItem) l->data;
      del = l;
      l = l->next;

      if(type == WINDOW_FOCUS_GROUP_ALL)
            li->flags &= ~WINDOW_FOCUS_GROUP_ALL;
      else
            li->flags &= ~focus_group_mask[type];

      highlightItem(focus->window,li);

      if(!(li->flags & WINDOW_FOCUS_GROUP_ALL))
      {
            zmapWindowFocusItemDestroy(del->data);
            focus->focus_item_set = g_list_delete_link(focus->focus_item_set,del) ;
      }
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
      if(focus->focus_column)
            zmapWindowFocusUnHighlightHotColumn(focus);

      focus->focus_column = container = (ZMapWindowContainerFeatureSet)column ;

      zmapWindowFocusHighlightHotColumn(focus);

      if (!container->sorted)
	{
	  zmapWindowContainerFeatureSetSortFeatures(container, 0) ;

	  container->sorted = TRUE ;
	}
    }

  return ;
}



