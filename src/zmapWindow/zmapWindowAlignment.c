/*  File: zmapWindowAlignment.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Deals with construction of foocanvas groups to hold
 *              alignments, blocks within those alignments and columns
 *              within those blocks.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Apr 18 16:25 2005 (edgrif)
 * Created: Thu Feb 24 11:19:23 2005 (edgrif)
 * CVS info:   $Id: zmapWindowAlignment.c,v 1.6 2005-04-21 13:45:47 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>



/* Used to pass data back to column menu callbacks. */
typedef struct
{
  ZMapWindowColumn column ;
} ColumnMenuCBDataStruct, *ColumnMenuCBData ;





/* These should be user defineable...... */
#define COLUMN_GAP 10.0
#define STRAND_GAP COLUMN_GAP / 2


static ZMapWindowColumn findColumn(GPtrArray *col_array, GQuark type_name, gboolean forward_strand) ;
static ZMapWindowColumn createColumnGroup(ZMapWindowAlignmentBlock block,
					  GQuark type_name, ZMapFeatureTypeStyle type) ;
static FooCanvasItem *createColumn(FooCanvasGroup *parent_group, ZMapWindowColumn column,
				   double start, double top, double bot, double width,
				   GdkColor *colour) ;
static gboolean canvasBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static void makeColumnMenu(GdkEventButton *button_event, ZMapWindowColumn column) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;




/* An alignment is all the features for a particular sequence alignment, there may be
 * several alignments within one window. Alignments may be discontinuous, here each
 * section of the alignment is held within and alignment "block",
 * see zmapWindowAlignmentAddBlock() */
ZMapWindowAlignment zmapWindowAlignmentCreate(char *align_name, ZMapWindow window,
					      FooCanvasGroup *parent_group, double position)
{
  ZMapWindowAlignment alignment ;

  alignment = g_new0(ZMapWindowAlignmentStruct, 1) ;

  alignment->alignment_id = g_quark_from_string(align_name) ;

  alignment->window = window ;

  alignment->alignment_group = foo_canvas_item_new(parent_group,
						   foo_canvas_group_get_type(),
						   "x", position,
						   "y", 0.0,
						   NULL) ;

  alignment->col_gap = COLUMN_GAP ;			    /* should be user settable... */

  return alignment ;
}




ZMapWindowAlignmentBlock zmapWindowAlignmentAddBlock(ZMapWindowAlignment alignment,
						     char *block_name, double position)
{
  ZMapWindowAlignmentBlock block = NULL ;


  block = g_new0(ZMapWindowAlignmentBlockStruct, 1) ;

  block->block_id = g_quark_from_string(block_name) ;

  block->parent = alignment ;

  block->block_group = foo_canvas_item_new(FOO_CANVAS_GROUP(alignment->alignment_group),
					   foo_canvas_group_get_type(),
					   "x", 0.0,
					   "y", position,
					   NULL) ;

  block->columns = g_ptr_array_new() ;

  return block ;
}


ZMapWindowColumn zmapWindowAlignmentAddColumn(ZMapWindowAlignmentBlock block, GQuark type_name,
					      ZMapFeatureTypeStyle type)
{
  ZMapWindowColumn column ;

  /* If the column doesn't already exist we need to find a new one. */
  if (!(column = findColumn(block->columns, type_name, TRUE)))
    {
      column = createColumnGroup(block, type_name, type) ;

      g_ptr_array_add(block->columns, column) ;
    }


  return column ;
}


/* Return the appropriate column group for the forward or reverse strand. */
FooCanvasItem *zmapWindowAlignmentGetColumn(ZMapWindowColumn column_group, ZMapStrand strand)
{
  FooCanvasItem *column ;

  if (strand == ZMAPSTRAND_FORWARD || strand == ZMAPSTRAND_NONE)
    column = column_group->forward_group ;
  else
    column = column_group->reverse_group ;

  return column ;
}


void zmapWindowAlignmentHideUnhideColumns(ZMapWindowAlignmentBlock block)
{
  ZMapWindowAlignment alignment = block->parent ;
  int i ;
  ZMapWindowColumn column ;
  double min_mag ;

  for (i = 0 ; i < block->columns->len ; i++)
    {
      column = g_ptr_array_index(block->columns, i) ;

      /* type->min_mag is in bases per line, but window->zoom_factor is pixels per base */
      min_mag = (column->type->min_mag ? alignment->window->max_zoom/column->type->min_mag : 0.0) ;

      if (alignment->window->zoom_factor > min_mag)
	{
	  if (column->forward)
	    foo_canvas_item_show(column->forward_group);
	  else if (column->type->showUpStrand)
	    foo_canvas_item_show(column->reverse_group);
	}
      else
	foo_canvas_item_hide(column->column_group) ;
    }

  return ;
}



/* Returns NULL always so you can have this function "automatically" reset your
 * pointer to the alignment when you destroy it. */
ZMapWindowAlignment zmapWindowAlignmentDestroy(ZMapWindowAlignment alignment)
{



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* NEEDS REWRITING FOR BLOCKS....... */

  if (alignment->columns)
    {
      /* Should we first call a foreach routine to free off the canvas obj. etc. ?? */
      
      g_ptr_array_free(alignment->columns, TRUE) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  g_free(alignment) ;

  return NULL ;
}



/* Find the column with the given name and strand, returns NULL if not found. */
static ZMapWindowColumn findColumn(GPtrArray *col_array, GQuark type_name, gboolean forward_strand)
{
  ZMapWindowColumn column = NULL ;
  int i ;
  
  for (i = 0 ; i < col_array->len ; i++)
    {
      ZMapWindowColumn next_column = g_ptr_array_index(col_array, i) ;

      if (next_column->type_name == type_name && next_column->forward == forward_strand)
	{
	  column = next_column ;
	  break ;
	}
    }

  return column ;
}



/* create Column */
static ZMapWindowColumn createColumnGroup(ZMapWindowAlignmentBlock block,
					  GQuark type_name, ZMapFeatureTypeStyle type)
{
  ZMapWindowColumn column ;
  ZMapWindowAlignment alignment = block->parent ;
  FooCanvasItem *group, *boundingBox ;
  double min_mag ;
  GdkColor column_colour ;
  double x1, y1, x2, y2 ;
  double position ;

  /* We find out how big the block is as this encloses all current columns, then we draw the next
   * column group to the right of the last current column. */
  foo_canvas_item_get_bounds(block->block_group, &x1, &y1, &x2, &y2) ;
  position = x2 + block->parent->col_gap ;

  /* Make sure this gets freed...... */
  column = g_new0(ZMapWindowColumnStruct, 1) ;
  column->parent_block = block ;
  column->type = type ;
  column->type_name = type_name ;
  column->strand_gap = STRAND_GAP ;			    /* Should be user defineable */

  column->column_group = foo_canvas_item_new(FOO_CANVAS_GROUP(block->block_group),
					     foo_canvas_group_get_type(),
					     "x", position,
					     "y", 0.0,
					     NULL) ;

  gdk_color_parse("white", &column_colour) ;

  column->forward_group = createColumn(FOO_CANVAS_GROUP(column->column_group),
				       column,
				       0.0,
				       alignment->window->seq_start, alignment->window->seq_end,
				       type->width, &column_colour) ;

  if (type->showUpStrand)
    {
      foo_canvas_item_get_bounds(column->forward_group, &x1, &y1, &x2, &y2) ;
      column->reverse_group = createColumn(FOO_CANVAS_GROUP(column->column_group),
					   column,
					   x2 + column->strand_gap,
					   alignment->window->seq_start, alignment->window->seq_end,
					   type->width,
					   &column_colour) ;
    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* can be done later.... */

  /* Decide whether or not this column should be visible */
  /* thisType->min_mag is in bases per line, but window->zoom_factor is pixels per base */
  min_mag = (canvas_data->thisType->min_mag ? 
	                 canvas_data->window->max_zoom/canvas_data->thisType->min_mag : 0.0);

  
  if (canvas_data->window->zoom_factor < min_mag)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return column ;
}


/* Create an individual column group, this will have the feature items added to it. */
static FooCanvasItem *createColumn(FooCanvasGroup *parent_group, ZMapWindowColumn column,
				   double start, double top, double bot, double width,
				   GdkColor *colour)
{
  FooCanvasItem *group ;
  FooCanvasItem *boundingBox ;

  group = foo_canvas_item_new(parent_group,
			      foo_canvas_group_get_type(),
			      "x", start,
			      "y", 0.0,
			      NULL) ;

  /* To trap events anywhere on a column we need an item that covers the whole column (note
   * that you can't do this by setting an event handler on the column group, the group handler
   * will only be called if someone has clicked on an item in the group. The item is drawn
   * first and in white to match the canvas background to make an 'invisible' box. */
  boundingBox = foo_canvas_item_new(FOO_CANVAS_GROUP(group),
				    foo_canvas_rect_get_type(),
				    "x1", 0.0,
				    "y1", top,
				    "x2", width,
				    "y2", bot,
				    "fill_color_gdk", colour,
				    NULL) ;

  g_signal_connect(GTK_OBJECT(boundingBox), "event",
		   GTK_SIGNAL_FUNC(canvasBoundingBoxEventCB), (gpointer)column) ;

  return group ;
}



static gboolean canvasBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindowColumn column = (ZMapWindowColumn)data ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	printf("in COLUMN group event handler - CLICK\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	/* Button 1 and 3 are handled, 2 is passed on to a general handler which could be
	 * the root handler. */
	switch (but_event->button)
	  {
	  case 1:
	    {
	      ZMapWindow window = column->parent_block->parent->window ;
	      ZMapWindowSelectStruct select = {NULL} ;

	      select.text = (char *)g_quark_to_string(column->type_name) ;

	      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

	      event_handled = TRUE ;
	      break ;
	    }
	  /* There are > 3 button mouse,  e.g. scroll wheels, which we don't want to handle. */
	  default:					    
	  case 2:
	    {
	      event_handled = FALSE ;
	      break ;
	    }
	  case 3:
	    {
	      makeColumnMenu(but_event, column) ;

	      event_handled = TRUE ;
	      break ;
	    }
	  }
	break ;
      } 
    default:
      {
	/* By default we _don't_handle events. */
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
}



static void makeColumnMenu(GdkEventButton *button_event, ZMapWindowColumn column)
{
  char *menu_title = "Column menu" ;
  ZMapWindowMenuItemStruct menu[] =
    {
      {"Show Feature List", 1, columnMenuCB},
      {"Dummy",             2, columnMenuCB},
      {NULL,                0, NULL}
    } ;
  ZMapWindowMenuItem menu_item ;
  ColumnMenuCBData cbdata ;

  cbdata = g_new0(ColumnMenuCBDataStruct, 1) ;
  cbdata->column = column ;

  menu_item = menu ;
  while (menu_item->name != NULL)
    {
      menu_item->callback_data = cbdata ;
      menu_item++ ;
    }

  zMapWindowMakeMenu(menu_title, menu, button_event) ;

  return ;
}



static void columnMenuCB(int menu_item_id, gpointer callback_data)
{
  ColumnMenuCBData menu_data = (ColumnMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      {
	printf("sorry, you didn't click on an actual feature so hard luck...\n") ;

	/* Actually we should still do this.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* display a list of the features in this column so the user
	 * can select one and have the display scroll to it. */
	zMapWindowCreateListWindow(menu_data->window, menu_data->feature_item);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	break ;
      }
    case 2:
      {
	printf("pillock\n") ;
	break ;
      }
    default:
      {
	zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
	break ;
      }
    }

  g_free(menu_data) ;

  return ;
}



