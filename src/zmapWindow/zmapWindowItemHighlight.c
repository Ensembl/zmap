/*  File: zmapWindowItemHighlight.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2006
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
 * Last edited: Oct 13 11:29 2006 (rds)
 * Created: Thu Oct 12 14:40:57 2006 (rds)
 * CVS info:   $Id: zmapWindowItemHighlight.c,v 1.1 2006-10-18 15:28:41 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindowItemHighlight.h>

typedef enum
  {
    /* The input we get from GdkEvents ... */
    ZMAP_HL_ORIGIN_X,
    ZMAP_HL_ORIGIN_Y,
    ZMAP_HL_CURRENT_X,
    ZMAP_HL_CURRENT_Y,
    /* Translation of the input to item coords ... */
    ZMAP_HL_FROM_X1,
    ZMAP_HL_FROM_X2,
    ZMAP_HL_FROM_Y1,
    ZMAP_HL_FROM_Y2,
    ZMAP_HL_TO_X1,
    ZMAP_HL_TO_X2,
    ZMAP_HL_TO_Y1,
    ZMAP_HL_TO_Y2,
    /* The total number of points to allocate */
    ZMAP_HL_TOTAL
  }ZMapHLCoord;

typedef enum
  {
    ZMAP_HL_EMPTY = ZMAP_HL_TOTAL,
    ZMAP_HL_SINGLE,
    ZMAP_HL_MULTI,
    ZMAP_HL_GROUP
  }ZMapHLType;

typedef struct _ZMapWindowTextItemHLStruct
{
  ZMapHLType target_type;

  union{
    FooCanvasItem   *text_item;
    GList           *multi_text;
    FooCanvasGroup  *features_container;
  }target;                      /* either way we'll work with a list of text items */

  FooCanvasItem *item_from;
  FooCanvasItem *item_to;

  GList *list_from;

  FooCanvasGroup  *parent_container;
  FooCanvasPoints *points;

  gboolean active;
  gboolean need_update;

  char *text_string;

} ZMapWindowTextItemHLStruct;


static void foo_canvas_item_get_world_bounds(FooCanvasItem *item, 
                                             double *x1, double *y1,
                                             double *x2, double *y2);

static gboolean checkReceivingItem(FooCanvasItem *source, double x, double y);
static gboolean initialiseFromItem(ZMapWindowTextItemHL object, 
                                   FooCanvasItem *source, 
                                   double x, double y);
static void buttonRelease(ZMapWindowTextItemHL object);

static gboolean updateToItemPlusDrawHighlight(ZMapWindowTextItemHL object, 
                                              FooCanvasItem *source, 
                                              double x, double y);
static void invoke_highlight_item(ZMapWindowTextItemHL object, ZMapGListDirection direction);
static ZMapGListDirection invoke_picking_current_item(ZMapWindowTextItemHL object);
static void pick_current_item(gpointer list_data, gpointer user_data);
static void highlight_item(gpointer list_data, gpointer user_data);


ZMapWindowTextItemHL zmapWindowTextItemHLCreate(FooCanvasGroup *parent_container)
{
  ZMapWindowTextItemHL object = NULL;

  if((object = g_new0(ZMapWindowTextItemHLStruct, 1)))
    {
      int points_count = (int)(ZMAP_HL_TOTAL / 2);

      object->points      = foo_canvas_points_new(points_count);
      object->target_type = ZMAP_HL_EMPTY;

      zMapAssert(FOO_IS_CANVAS_GROUP(parent_container));
      object->parent_container = parent_container;
      /* ... */
    }

  return object;
}

void zmapWindowTextItemHLAddItem(ZMapWindowTextItemHL object, 
                                 FooCanvasItem *text_item)
{
  zMapAssert(FOO_IS_CANVAS_TEXT(text_item));

  switch(object->target_type)
    {
    case ZMAP_HL_EMPTY:
      {
        /* ... first text item */
        object->target.text_item = text_item;
        object->target_type = ZMAP_HL_SINGLE;
      }
      break;
    case ZMAP_HL_SINGLE:
      {
        FooCanvasItem *previous = object->target.text_item;
        object->target.multi_text = NULL;
        object->target.multi_text = 
          g_list_append(object->target.multi_text, previous);        
      } /* Falls through!! */
    case ZMAP_HL_MULTI:
      {
        object->target.multi_text = 
          g_list_append(object->target.multi_text, text_item);
        object->target_type = ZMAP_HL_MULTI;
      }
      break;
    case ZMAP_HL_GROUP:
    default:
      zMapAssertNotReached();
      break;
    }

  return ;
}

void zmapWindowTextItemHLUseContainer(ZMapWindowTextItemHL object, 
                                      FooCanvasGroup *features_container)
{
  zMapAssert(FOO_IS_CANVAS_GROUP(features_container));

  switch(object->target_type)
    {
    case ZMAP_HL_EMPTY:
    case ZMAP_HL_GROUP:
      {
        object->target.features_container = features_container;
        object->target_type = ZMAP_HL_GROUP;
      }
      break;
    case ZMAP_HL_SINGLE:
    case ZMAP_HL_MULTI:
    default:
      zMapAssertNotReached();
      break;
    }

  return ;
}

void zmapWindowTextItemHLEvent(ZMapWindowTextItemHL object,
                               FooCanvasItem *receiving_item,
                               GdkEvent *event, gboolean first_event)
{
  GdkEventButton *button = NULL;
  GdkEventMotion *motion = NULL;
  double x, y;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      button = (GdkEventButton *)event;
      if(button->button == 1)   /* Only left mouse click */
        {
          x = button->x;
          y = button->y;
          if(checkReceivingItem(receiving_item, x, y) && first_event)
            {
              gboolean initialised = FALSE;
              initialised = object->active = 
                initialiseFromItem(object, receiving_item, x, y);
              zMapAssert(initialised);
            }
        }
      break;
    case GDK_BUTTON_RELEASE:
      if(object->active)
        {
          button = (GdkEventButton *)event;
          x = button->x;
          y = button->y;
          updateToItemPlusDrawHighlight(object, receiving_item, x, y);
          buttonRelease(object);
        }
      zMapAssert((!(object->active)));
      break;
    case GDK_MOTION_NOTIFY:
      if(object->active)
        {
          motion = (GdkEventMotion *)event;
          x = motion->x;
          y = motion->y;
          updateToItemPlusDrawHighlight(object, receiving_item, x, y);
        }
      break;
    default:
      break;
    }

  return ;
}
void zmapWindowTextItemHLSetFullText(ZMapWindowTextItemHL object, char *text)
{
  
  return ;
}

gboolean zmapWindowTextItemHLSelectedText(ZMapWindowTextItemHL object, char **selected_text)
{
  gboolean free_text = TRUE;
  char *selection    = NULL;

  if(0)                         /* all_selected */
    {
      selection = object->text_string;
      free_text = FALSE;
    }
  else
    {
      selection = g_strdup_printf("%s", object->text_string);
    }

  if(!selection)
    free_text = FALSE;

  if(selected_text)
    *selected_text = selection;

  return free_text;
}

void zmapWindowTextItemHLDestroy(ZMapWindowTextItemHL object)
{
  /* Never free the text! */
  /* Un set everything ... */
  
  return ;
}



/* INTERNAL */

static void foo_canvas_item_get_world_bounds(FooCanvasItem *item, 
                                             double *x1, double *y1,
                                             double *x2, double *y2)
{
  double wx1, wx2, wy1, wy2;
  
  foo_canvas_item_get_bounds(item, &wx1, &wy1, &wx2, &wy2);
  foo_canvas_item_i2w(item, &wx1, &wy1);
  foo_canvas_item_i2w(item, &wx2, &wy2);

  if(x1)
    *x1 = wx1;
  if(x2)
    *x2 = wx2;
  if(y1)
    *y1 = wy1;
  if(y2)
    *y2 = wy2;

  return ;
}

static gboolean checkReceivingItem(FooCanvasItem *source, double x, double y)
{
  gboolean item_is_hit = FALSE;
  double wx1, wx2, wy1, wy2;

  /* Get current bounds in world coords */
  foo_canvas_item_get_world_bounds(source, &wx1, &wy1, &wx2, &wy2);
  
  if((x > wx1 && x < wx2) &&
     (y > wy1 && y < wy2))
    {
      item_is_hit = TRUE;
    }

  return item_is_hit;
}

static gboolean initialiseFromItem(ZMapWindowTextItemHL object, 
                                   FooCanvasItem *source, double x, double y)
{
  gboolean initialised = FALSE;

  if(source && FOO_IS_CANVAS_TEXT(source))
    {
      object->list_from = NULL;
      object->item_from = source;
      object->points->coords[ZMAP_HL_ORIGIN_X] = x;
      object->points->coords[ZMAP_HL_ORIGIN_Y] = y;      
      foo_canvas_item_get_bounds(source, 
                                 &(object->points->coords[ZMAP_HL_FROM_X1]),
                                 &(object->points->coords[ZMAP_HL_FROM_Y1]),
                                 &(object->points->coords[ZMAP_HL_FROM_X2]),
                                 &(object->points->coords[ZMAP_HL_FROM_Y2]));

      switch(object->target_type)
        {
        case ZMAP_HL_MULTI:
          zMapAssert(object->target.multi_text);

          object->list_from = g_list_find(object->target.multi_text, object->item_from);

          zMapAssert(object->list_from);
          zMapAssert(object->list_from->data == object->item_from); /* paranoia */
          break;
        case ZMAP_HL_GROUP:
          {
            GList *item_list = NULL;

            zMapAssert((FOO_IS_CANVAS_GROUP(object->target.features_container)) &&
                       (item_list = FOO_CANVAS_GROUP(object->target.features_container)->item_list));

            object->list_from = g_list_find(item_list, object->item_from);

            zMapAssert(object->list_from);
            zMapAssert(object->list_from->data == object->item_from); /* paranoia */
          }
          break;
        case ZMAP_HL_SINGLE:
          /* nothing to do ... */
          break;
        case ZMAP_HL_EMPTY:
        default:
          zMapAssertNotReached();
          break;
        }

      object->active = initialised = TRUE;

    }

  return initialised;
}

static void buttonRelease(ZMapWindowTextItemHL object)
{
  object->active = FALSE;
  /* ... */
  return ;
}

static gboolean updateToItemPlusDrawHighlight(ZMapWindowTextItemHL object, 
                                              FooCanvasItem *source, double x, double y)
{
  gboolean updated = TRUE;
  ZMapGListDirection direction = ZMAP_GLIST_FORWARD;

  object->item_to  = NULL;

  object->points->coords[ZMAP_HL_CURRENT_X] = x;
  object->points->coords[ZMAP_HL_CURRENT_Y] = y;

  switch(object->target_type)
    {
    case ZMAP_HL_SINGLE:
      object->item_to = source;
      break;
    case ZMAP_HL_MULTI:
    case ZMAP_HL_GROUP:
      direction = invoke_picking_current_item(object);
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  zMapAssert(object->item_to);

  foo_canvas_item_get_bounds(object->item_to, 
                             &(object->points->coords[ZMAP_HL_TO_X1]),
                             &(object->points->coords[ZMAP_HL_TO_Y1]),
                             &(object->points->coords[ZMAP_HL_TO_X2]),
                             &(object->points->coords[ZMAP_HL_TO_Y2]));

  invoke_highlight_item(object, direction);

  return updated;
}

static void invoke_highlight_item(ZMapWindowTextItemHL object, ZMapGListDirection direction)
{
  GList *item_list = NULL, stack_list = {NULL};

  switch(object->target_type)
    {
    case ZMAP_HL_SINGLE:
      stack_list.data = object->target.text_item;
      item_list = &(stack_list);
      break;
    case ZMAP_HL_MULTI:
    case ZMAP_HL_GROUP:
      item_list = object->list_from;
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  object->need_update = TRUE;

  zMap_g_list_foreach_directional(item_list, highlight_item, (gpointer)object, direction);

  return ;
}

static ZMapGListDirection invoke_picking_current_item(ZMapWindowTextItemHL object)
{
  FooCanvasItem *from_item = NULL;
  double x, y, x1, y1, x2, y2;
  ZMapGListDirection direction = ZMAP_GLIST_FORWARD;

  x = object->points->coords[ZMAP_HL_CURRENT_X];
  y = object->points->coords[ZMAP_HL_CURRENT_Y];

  if(object->list_from)
    {
      from_item = (FooCanvasItem *)(object->list_from->data);
      foo_canvas_item_get_world_bounds(from_item, &x1, &y1, &x2, &y2);
      if(y > y1 && y < y2)
        {
          object->need_update = TRUE;
          if(y <= y1)
            direction = ZMAP_GLIST_REVERSE;
          else if(y >= y2)
            direction = ZMAP_GLIST_FORWARD;
        }
      else
        {
          object->need_update = FALSE;
          object->item_to = from_item;
        }
    }

  if(object->need_update)
    {
      zMap_g_list_foreach_directional(object->list_from, pick_current_item,
                                      (gpointer)object, direction);
    }

  return direction;
}

/* g_list_foreach function */
static void pick_current_item(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)list_data;
  ZMapWindowTextItemHL object = (ZMapWindowTextItemHL)user_data;
  double x, y;

  if(object->need_update && FOO_IS_CANVAS_TEXT(item))
    {
      x = object->points->coords[ZMAP_HL_CURRENT_X];
      y = object->points->coords[ZMAP_HL_CURRENT_Y];
      if(checkReceivingItem(item, x, y))
        {
          object->item_to = item;
          object->need_update = FALSE;
        }
    }

  return ;
}

/* g_list_foreach function */
static void highlight_item(gpointer list_data, gpointer user_data)
{
  FooCanvasItem *item = (FooCanvasItem *)list_data;
  FooCanvasItem *high = NULL;
  ZMapWindowTextItemHL object = (ZMapWindowTextItemHL)user_data;
  
  if(object->need_update)
    {
      /* get highlight ... item dependent ... */
      high = NULL;              /* ... */

      zMapAssert(high);

      if(item == object->item_from)
        {
          /* update the x coords to the ... and ... */
        }
      else if(item == object->item_to)
        {
          object->need_update = FALSE;
          /* update the x coords to the ... and ... */
        }
      else
        {
          /* update the x coords of the higlight to the min and max */
        }

    }
  
  return ;
}


