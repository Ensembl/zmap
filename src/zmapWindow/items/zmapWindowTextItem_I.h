/*  File: zmapWindowTextItem_I.h
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
 * Description: Internal data structures of zmap text item for foocanvas
 *              to implement dna/peptide sequence display.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_WINDOW_TEXT_ITEM_I_H
#define ZMAP_WINDOW_TEXT_ITEM_I_H

#include <glib-object.h>
#include <libzmapfoocanvas/libfoocanvas.h>
#include <zmapWindowTextItem.h>



/* enum for the return item coords for zmapWindowItemTextIndexGetBounds()
 * TL means Top Left, BR means Bottom Right */
typedef enum
  {
    ITEMTEXT_CHAR_BOUND_TL_X  = 0,
    ITEMTEXT_CHAR_BOUND_TL_Y  = 1,
    ITEMTEXT_CHAR_BOUND_BR_X  = 2,
    ITEMTEXT_CHAR_BOUND_BR_Y  = 3,
    ITEMTEXT_CHAR_BOUND_COUNT = 4
  } CharBoundType ;

enum
  {
    TEXT_ITEM_SELECTED_SIGNAL,
    TEXT_ITEM_LAST_SIGNAL
  } SignalType ;

typedef enum
  {
    TEXT_ITEM_SELECT_NONE   = 0,
    TEXT_ITEM_SELECT_EVENT  = 1 << 0,			    /* Turned on/off for button down/up during selection. */
    TEXT_ITEM_SELECT_ENDURE = 1 << 1,
    TEXT_ITEM_SELECT_SIGNAL = 1 << 2,
    TEXT_ITEM_SELECT_CANVAS_CHANGED = 1 << 3,
  } SelectState;


/* Used for highlighting etc etc. */
typedef struct ItemEventStructType
{
  int              origin_index;
  double           origin_x, origin_y;
  double           event_x, event_y;

  /* I think these are meant to be the start/end within the text.... */
  int start_index, end_index ;

  SelectState      selected_state;

  gboolean         result;

  FooCanvasPoints *points;
  double           index_bounds[ITEMTEXT_CHAR_BOUND_COUNT];

} ItemEventStruct, *ItemEvent ;


typedef struct
{
  FooCanvasItem  *highlight;
  ItemEventStruct user_select;
} HighlightItemEventStruct, *HighlightItemEvent;


typedef void (* zmap_window_sequence_feature_select_method)(ZMapWindowTextItem text_item,
							    int index1, int index2,
							    gboolean deselect,  gboolean signal) ;

typedef void (* zmap_window_sequence_feature_deselect_method)(ZMapWindowTextItem text_item, gboolean signal) ;

typedef struct _zmapWindowTextItemClassStruct
{
  FooCanvasTextClass __parent__;

  zmap_window_sequence_feature_select_method   select;
  zmap_window_sequence_feature_deselect_method deselect;

  ZMapWindowTextItemSelectionCB selected_signal;

  guint signals[TEXT_ITEM_LAST_SIGNAL];

} zmapWindowTextItemClassStruct;



typedef struct _zmapWindowTextItemStruct
{
  FooCanvasText __parent__;

  FooCanvasItem *highlight;

  ItemEventStruct        item_event;

  GList                 *selections;

  GdkColor               select_colour;

  /* We actually need to know the ratio of chars to sequence coords otherwise we cannot correctly
   * calculate offsets for sequences that do not have one base/coord (e.g. peptides) for when the
   * scrolled region is less than the whole sequence. */
  int refseq_start, refseq_end ;
  int text_length ;
  int bases2coords ;

  /* a fixed size array buffer_size long */
  char                     *buffer;
  gint                      buffer_size;

  double                    requested_width;
  double                    requested_height;

  /* Need to keep a running record of the last ypos for a selected item so we can calculate
   * it's offset correctly when the scrolled region changes. */
  double last_selected_ypos ;

  /* a cache of the drawing extents/scroll region etc... */
  ZMapTextItemDrawDataStruct    update_cache;
  ZMapTextItemDrawDataStruct    draw_cache;

  /* the extents that should be used instead of the  */
  PangoRectangle            char_extents;
  ZMapTextItemAllocateCB   allocate_func;
  ZMapTextItemFetchTextCB  fetch_text_func;
  gpointer                  callback_data; /* user data for the allocate_func & fetch_text_func */

  unsigned int              flags        : 3 ;

  /* State for text and for properties where we cannot detect whether they are set from their values. */
  struct
  {
    unsigned int select_colour : 1 ;

    unsigned int being_destroyed : 1 ;			    /* not sure if this is needed, may be in g_object. */
  } state ;


} zmapWindowTextItemStruct ;



#endif /* ZMAP_WINDOW_TEXT_ITEM_I_H */
