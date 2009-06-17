/*  File: zmapWindowTextItem_I.h
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
 * Last edited: Jun 17 10:27 2009 (rds)
 * Created: Fri Jan 16 13:56:52 2009 (rds)
 * CVS info:   $Id: zmapWindowTextItem_I.h,v 1.1 2009-06-17 09:46:16 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_WINDOW_TEXT_ITEM_I_H
#define ZMAP_WINDOW_TEXT_ITEM_I_H

#include <glib-object.h>
#include <libfoocanvas/libfoocanvas.h>
#include <zmapWindowTextItem.h>

/* enum for the return item coords for 
 * zmapWindowItemTextIndexGetBounds()  
 * TL means Top Left, BR means Bottom Right
 */
enum
  {
    ITEMTEXT_CHAR_BOUND_TL_X  = 0,
    ITEMTEXT_CHAR_BOUND_TL_Y  = 1,
    ITEMTEXT_CHAR_BOUND_BR_X  = 2,
    ITEMTEXT_CHAR_BOUND_BR_Y  = 3,
    ITEMTEXT_CHAR_BOUND_COUNT = 4
  };

enum
  {
    TEXT_ITEM_SELECTED_SIGNAL,
    TEXT_ITEM_LAST_SIGNAL
  };

typedef enum
  {
    TEXT_ITEM_SELECT_NONE   = 0,
    TEXT_ITEM_SELECT_EVENT  = 1 << 0,
    TEXT_ITEM_SELECT_ENDURE = 1 << 1,
    TEXT_ITEM_SELECT_SIGNAL = 1 << 2,
  } SelectState;

/* This struct needs rationalising. */
typedef struct
{
  int              origin_index;
  double           origin_x, origin_y;
  double           event_x, event_y;
  int start_index, end_index;

  SelectState      selected_state;

  gboolean         result;
  FooCanvasPoints *points;
  double           index_bounds[ITEMTEXT_CHAR_BOUND_COUNT];
}DNAItemEventStruct, *DNAItemEvent;


typedef struct _zmapWindowTextItemClassStruct
{
  FooCanvasTextClass __parent__;

  ZMapWindowTextItemSelectionCB selected_signal;

  guint signals[TEXT_ITEM_LAST_SIGNAL];

} zmapWindowTextItemClassStruct;

typedef struct _zmapWindowTextItemStruct
{
  FooCanvasText __parent__;

  FooCanvasItem *highlight;

  DNAItemEventStruct        item_event;

  /* a fixed size array buffer_size long */
  char                     *buffer;
  gint                      buffer_size;

  double                    requested_width;
  double                    requested_height;
  /* unsure why these two are here, remove?? */
  double                    zx,zy;

  /* a cache of the drawing extents/scroll region etc... */
  ZMapTextItemDrawDataStruct    update_cache;
  ZMapTextItemDrawDataStruct    draw_cache;
  /* the extents that should be used instead of the  */
  PangoRectangle            char_extents;
  ZMapTextItemAllocateCB   allocate_func;
  ZMapTextItemFetchTextCB  fetch_text_func;
  gpointer                  callback_data; /* user data for the allocate_func & fetch_text_func */
  unsigned int              flags        : 3;
  unsigned int              allocated    : 1;
  unsigned int              text_fetched : 1;

} zmapWindowTextItemStruct;



#endif /* ZMAP_WINDOW_TEXT_ITEM_I_H */
