/*  File: containers.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#include <containers.h>

typedef enum
  {
    DEMO_CONTAINER_INVALID,
    DEMO_CONTAINER_COLUMN_SET,
    DEMO_CONTAINER_COLUMN
  }demoContainerType;

typedef struct _demoContainerStruct
{
  demoContainerType        type;
  GTimer                  *timer;
  gpointer                 canvas_item;

  GList                   *children;
  demoContainerCallbackSet callback_func;
  gpointer                 callback_data;
}demoContainerStruct;

demoContainer demoContainerCreate(demoContainer parent, demoContainerCallbackSet callbacks, gpointer user_data)
{
  demoContainer container = NULL;
  gpointer parent_item = NULL;

  if((container = g_new0(demoContainerStruct, 1)))
    {
      double leftmost = 0.0;
      if(!parent)
        container->type = DEMO_CONTAINER_COLUMN_SET;
      else
        {
         
          parent_item     = parent->canvas_item;
          if(parent->callback_func->get_bounds)
            (parent->callback_func->get_bounds)(parent_item, NULL, NULL, &leftmost, NULL, parent->callback_data);
          container->type = DEMO_CONTAINER_COLUMN;
        }

      container->callback_func = callbacks;
      container->callback_data = user_data;

      leftmost += COLUMN_SPACE;

      if(container->callback_func->create_group)
        container->canvas_item = (container->callback_func->create_group)(parent_item, leftmost, user_data);

      container->timer = g_timer_new();
      
    }

  return container;
}

void demoContainerExercise(demoContainer container)
{
  demoContainerCallbackFunc create_item;
  demoContainer new_child;
  gpointer user_data, canvas_item;
  gulong micro;
  int i;

  if(container->type == DEMO_CONTAINER_COLUMN_SET)
    {
      if((new_child = demoContainerCreate(container, container->callback_func, 
                                          container->callback_data)))
        {
          container->children = g_list_append(container->children, new_child);
          demoContainerExercise(new_child);
        }
    }
  else
    {
      create_item = container->callback_func->create_item;
      user_data   = container->callback_data;
      canvas_item = container->canvas_item;
      /* start timer */
      printf("%s: Starting...\n", __PRETTY_FUNCTION__);
      g_timer_start(container->timer);
      
      
      /* do work... */
      for(i = 0; i < ITEMS_PER_COLUMN; i++)
        {
          (create_item)(canvas_item, user_data);
        }

      /* print timer... */
      printf("%s: Finished. Total time %f\n", 
             __PRETTY_FUNCTION__, 
             g_timer_elapsed(container->timer, &micro));
    }

  return ;
}

void demoContainerPositionChildren(demoContainer container)
{
  return ;
}

void demoContainerHideChildren(demoContainer container)
{
  return ;
}
