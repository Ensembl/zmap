/*  File: zmapWindowState.c
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
 * Last edited: Sep 21 12:17 2007 (rds)
 * Created: Mon Jun 11 09:49:16 2007 (rds)
 * CVS info:   $Id: zmapWindowState.c,v 1.1 2007-09-21 15:20:08 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowState.h>

typedef struct
{
  GQuark align_id, block_id, set_id, feature_id;
  ZMapFrame frame;
  ZMapStrand strand;
  double x1, x2, y1, y2;
} ZMapWindowMarkSerialStruct;

typedef struct _ZMapWindowStateStruct
{
  ZMapMagic   *magic;

  /* state of stuff ... */
  double       zoom_factor;
  ZMapWindowMarkSerialStruct mark;

  /* flags to say which states are set/unset */
  unsigned int zoom_set   : 1 ;
  unsigned int mark_set   : 1 ;

} ZMapWindowStateStruct;

static void state_mark_restore(ZMapWindow window, ZMapWindowMark mark, ZMapWindowMarkSerialStruct *serialized);


ZMAP_DEFINE_NEW_MAGIC(window_state_magic_G);


ZMapWindowState zmapWindowStateCreate(void)
{
  ZMapWindowState state = NULL;

  if(!(state = g_new0(ZMapWindowStateStruct, 1)))
    zMapAssertNotReached();
  else
    state->magic = &window_state_magic_G;

  return state;
}

void zmapWindowStateRestore(ZMapWindowState state, ZMapWindow window)
{
#ifdef NEEDS_ZMAP_WINDOW_ZOOM_TO_FACTOR
  if(state->zoom_set)
    zmapWindowZoomToFactor(window, state->zoom_factor);
#endif /* NEEDS_ZMAP_WINDOW_ZOOM_TO_FACTOR */

  if(state->mark_set)
    {
      state_mark_restore(window, window->mark, &(state->mark));
    }

  return ;
}

ZMapWindowState zmapWindowStateCopy(ZMapWindowState state)
{
  ZMapWindowState new_state = NULL;

  ZMAP_ASSERT_MAGICAL(state->magic, window_state_magic_G);

  new_state = zmapWindowStateCreate();
  
  memcpy(new_state, state, sizeof(ZMapWindowStateStruct));

  return new_state;
}

ZMapWindowState zmapWindowStateDestroy(ZMapWindowState state)
{
  zMapAssert(state);
  ZMAP_ASSERT_MAGICAL(state->magic, window_state_magic_G);

  /* Anything that needs freeing */


  /* ZERO AND FREE */
  ZMAP_PTR_ZERO_FREE(state, ZMapWindowStateStruct);

  return state;
}


gboolean zmapWindowStateSaveZoom(ZMapWindowState state, double zoom_factor)
{
  ZMAP_ASSERT_MAGICAL(state->magic, window_state_magic_G);

  state->zoom_set    = 1;
  state->zoom_factor = zoom_factor;

  return state->zoom_set;
}

gboolean zmapWindowStateSaveMark(ZMapWindowState state, ZMapWindowMark mark)
{
  ZMAP_ASSERT_MAGICAL(state->magic, window_state_magic_G);

  if((state->mark_set = zmapWindowMarkIsSet(mark)))
    {
      FooCanvasItem *mark_item;
      FooCanvasGroup *mark_item_cont;
      if((mark_item = zmapWindowMarkGetItem(mark)))
	{
	  ZMapFeature feature = g_object_get_data(G_OBJECT(mark_item), ITEM_FEATURE_DATA);

	  if((mark_item_cont = zmapWindowContainerGetFromItem(mark_item)))
	    {
	      ZMapWindowItemFeatureSetData set_data;

	      if((set_data = zmapWindowContainerGetData(mark_item_cont, ITEM_FEATURE_SET_DATA)))
		{
		  state->mark.strand     = set_data->strand;
		  state->mark.frame      = set_data->frame;
		  state->mark.align_id   = feature->parent->parent->parent->unique_id;
		  state->mark.block_id   = feature->parent->parent->unique_id;
		  state->mark.set_id     = feature->parent->unique_id;
		  state->mark.feature_id = feature->unique_id;
		}
	      else
		state->mark_set = FALSE;
	    }
	}
      else
	{
	  zmapWindowMarkGetWorldRange(mark, 
				      &(state->mark.x1), &(state->mark.y1),
				      &(state->mark.x2), &(state->mark.y2));
	}
    }

  return state->mark_set;
}


/* INTERNAL */


static void state_mark_restore(ZMapWindow window, ZMapWindowMark mark, ZMapWindowMarkSerialStruct *serialized)
{
  if(serialized->align_id != 0 && serialized->feature_id != 0)
    {
      FooCanvasItem *mark_item;
      if((mark_item = zmapWindowFToIFindItemFull(window->context_to_item,
						 serialized->align_id,
						 serialized->block_id,
						 serialized->set_id,
						 serialized->strand,
						 serialized->frame,
						 serialized->feature_id)))
	{
	  zmapWindowMarkSetItem(mark, mark_item);
	}
    }
  else
    {
      zmapWindowMarkSetWorldRange(mark, serialized->x1, serialized->y1, serialized->x2, serialized->y2);
    }

  return ;
}

















#ifdef RDS_NO

ZMapWindowStateQueue zmapWindowStateQueueCreate(void)
{
  GQueue queue = NULL;

  if(!(queue = g_queue_new()))
    zMapAssertNotReached();

  return queue;
}

void zmapWindowStateQueueForward(ZMapWindowStateQueue   queue, 
                                 ZMapWindowStateStruct *state)
{
  ZMapWindowState stored_state = NULL;

  /* As we can't make users pass in a valid state :( */
  if(state->magic == NULL)
    state->magic = &window_state_magic_G;

  stored_state = zmapWindowStateCopy(state);
    
  g_queue_push_tail(queue, stored_state);

  return ;
}

ZMapWindowState zmapWindowStateQueueBackward(ZMapWindowStateQueue queue)
{
  ZMapWindowState previous = NULL;

  previous = g_queue_pop_tail(queue);

  ZMAP_ASSERT_MAGICAL(previous->magic, window_state_magic_G);

  return previous;
}


ZMapWindowStateQueue zmapWindowStateQueueDestroy(ZMapWindowStateQueue queue)
{
  ZMapWindowState state = NULL;

  while((state = g_queue_pop_head(queue)))
    {
      state = zmapWindowStateDestroy(state);
    }

  g_queue_free(queue);

  queue = NULL;

  return queue;
}
#endif
