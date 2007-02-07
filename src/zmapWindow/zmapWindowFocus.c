/*  File: zmapWindowFocus.c
 *  Author: Ed Griffiths edgrif@sanger.ac.uk
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
 * Last edited: Feb  7 12:52 2007 (rds)
 * Created: Tue Jan 16 09:46:23 2007 (rds)
 * CVS info:   $Id: zmapWindowFocus.c,v 1.2 2007-02-07 13:04:04 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>

#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>

/* Used for window focus items and column.
 * 
 * Note that if only a column has been selected then focus_item_set may be NULL, BUT
 * if an item has been selected then focus_column will that items parent.
 * 
 *  */
typedef struct _ZMapWindowFocusStruct
{
  /* the selected/focused items. Interesting operations on these should be possible... */
  GList *focus_item_set ;       /* list of FocusSetItems */

  FooCanvasGroup *focus_column ;

} ZMapWindowFocusStruct ;


static gint find_item(gconstpointer list_data, gconstpointer user_data);
static ZMapWindowFocusItemArea ensure_unique(ZMapWindowFocus focus, 
                                             FooCanvasItem *item);
static void addFocusItemCB(gpointer data, gpointer user_data) ;
static void freeFocusItems(ZMapWindowFocus focus) ;


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
    zmapWindowFocusSetHotItem(focus, item) ;
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
      zmapWindowFocusSetHotItem(focus, FOO_CANVAS_ITEM(item_list->data)) ;
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
      if (focus->focus_column == zmapWindowItemGetParentContainer(item))
	result = TRUE ;
    }

  return result ;
}



void zmapWindowFocusSetHotItem(ZMapWindowFocus focus, FooCanvasItem *item)
{
  ZMapWindowFocusItemArea item_area;

  item_area = ensure_unique(focus, item);

  /* Stick the item on the front. */
  focus->focus_item_set = g_list_prepend(focus->focus_item_set, item_area) ;

  /* Set the focus items column as the focus column. */
  focus->focus_column = zmapWindowContainerGetParentContainerFromItem(item) ;

  return ;
}


/* this one is different, a new column can be set _without_ setting a new item, so we
 * need to get rid of the old items. */
void zmapWindowFocusSetHotColumn(ZMapWindowFocus focus, FooCanvasGroup *column)
{
  freeFocusItems(focus) ;

  focus->focus_column = column ;
  
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

  column = focus->focus_column ;

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
  GList *remove;

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set)
    {
      if((remove = g_list_find_custom(focus->focus_item_set, item, find_item)))
        {
          focus->focus_item_set = g_list_remove(focus->focus_item_set, 
                                                remove->data);
          //zmapWindowFocusItemAreaDestroy((ZMapWindowFocusItemArea)(remove->data));
        }
    }

  return ;
}


void zmapWindowFocusReset(ZMapWindowFocus focus)
{
  freeFocusItems(focus) ;

  focus->focus_column = NULL ;

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
#ifdef RDS_NOT_SURE
  if(item_area->area_highlights)
    g_list_foreach(item_area->area_highlights, destroy_set, NULL);
#endif
  g_free(item_area);

  return ;
}


/* INTERNAL */

static ZMapWindowFocusItemArea ensure_unique(ZMapWindowFocus focus, FooCanvasItem *item)
{
  ZMapWindowFocusItemArea item_area;
  GList *remove, *areas = NULL;

  /* always try to remove, if item is not in list then list is unchanged. */
  if (focus->focus_item_set)
    {
      if((remove = g_list_find_custom(focus->focus_item_set, item, find_item)))
        {
          item_area = remove->data;
          focus->focus_item_set = g_list_remove(focus->focus_item_set, 
                                                remove->data);
          areas = item_area->area_highlights;
          item_area->area_highlights = NULL;
          zmapWindowFocusItemAreaDestroy(item_area);
        }
    }

  item_area = zmapWindowFocusItemAreaCreate(item);
  zMapAssert(item_area);

  if(areas != NULL)
    item_area->area_highlights = areas;

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
 * 
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



