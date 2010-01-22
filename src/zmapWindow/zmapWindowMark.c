/*  File: zmapWindowMark.c
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
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Jan 22 10:58 2010 (edgrif)
 * Created: Tue Jan 16 09:51:19 2007 (rds)
 * CVS info:   $Id: zmapWindowMark.c,v 1.21 2010-01-22 13:55:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>	/* ZMapWindow */
#include <zmapWindowMark_P.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerBlock.h>

#define mark_bitmap_width 16
#define mark_bitmap_height 4


typedef struct _ZMapWindowMarkStruct
{
  ZMapMagic       magic ;

  ZMapWindowContainerBlock block_container ; /*! The block the mark is set on.  This might need to become a list of   */
  ZMapWindow               window ; /*! mark needs access to the ZMapWindow without having to pass it in.  */
  FooCanvasItem           *mark_src_item ; /*! This is the item that is the src of the mark.  Can be NULL. */
  
  /*! world coords of the mark.  */
  double          world_x1 ;
  double          world_y1 ;
  double          world_x2 ;
  double          world_y2 ;

  /* These are block sequence coordinates */
  int             seq_start ; /*! start of the mark in seq coords (of block)  */
  int             seq_end   ; /*! end of the mark in seq coords (of block)  */

  GdkColor        colour ;	/*! colour used for the mark  */
  GdkBitmap      *stipple ;	/*! stipple used for the mark  */

  gboolean        mark_set ;	/*! internal flag for whether mark is set.  */
  double          margin ;
} ZMapWindowMarkStruct ;


static void markItem(ZMapWindowMark mark, FooCanvasItem *item, gboolean set_mark) ;
static void markRange(ZMapWindowMark mark, double y1, double y2) ;
static void mark_block_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points, 
			  ZMapContainerLevelType level, gpointer user_data);


/* Some static global declarations... Oh joy. */

/*! mark_magic_G is a simple pointer to check validity of a ZMapWindowMark  */
ZMAP_MAGIC_NEW(mark_magic_G, ZMapWindowMarkStruct) ;

/*! mark_bitmap_bits_G is the definition of the default stipple. */
static char mark_bitmap_bits_G[] =
  {
    0x11, 0x11,
    0x22, 0x22,
    0x44, 0x44,
    0x88, 0x88
  } ;




/* 
 *    Set of functions for handling the marked feature or region
 *    ----------------------------------------------------------
 * 
 * Essentially there are two ways to mark, either by marking a feature or
 * by marking a region. The two are mutually exclusive.
 * 
 * If a feature is marked then it will be used in a number of window operations
 * such as zooming and column bumping.
 * 
 * Details:
 *
 * - Either mark using a feature item, or using a _world_ coordinate range
 * - Using an item leads to 'synchronous' setting of the mark with respect 
 *   to the ZMapWindowContainerBlock that is stored in the mark.
 * - Using a region leads to 'asynchronous' setting of the mark with respect
 *   to the ZMapWindowContainerBlock theat is stored.
 * - The items that display the mark are handled in the ZMapWindowContainerBlock
 *   code.  This is the root cause of the sync/async nature.
 * - The mark items are stippled using the supplied stipple.
 * - The mark items must be resized by some means for long items reasons.
 * - The long items cropping/resizing is handled in the ZMapWindowContainerBlock
 *   code by the update hooks the ZMapWindowContainerGroup interface provides.
 * - Initial selection of the correct block (setting by world coord) is again 
 *   done in the container code by way of the update hooks.  Selection works
 *   on a world coord basis.  When setting by item the item's parent's parent
 *   is used.
 * - The mark is saved across revcomp, vsplit, hsplit, by the ZMapWindowState
 *   code and it handles serializing the mark and calling the mark functions.
 * - There are a couple of issues to watch out for when multiple blocks get displayed
 *  - The block_container will need to handle multiple blocks, as will the interface.
 *  - Selecting the correct block might need thought.  I think it's ok as it is, 
 *    but check out the container block code.
 */






/*!
 * \brief Get the marked region of a window.
 *
 * \param window  The window to get the mark from
 * \param start   The address to store the start of the mark
 * \param end     The address to store the end of the mark
 *
 * \return boolean corresponding to whether mark is set TRUE = set, FALSE = unset
 */
gboolean zMapWindowGetMark(ZMapWindow window, int *start, int *end)
{
  gboolean result = FALSE ;

  if (window->mark && window->mark->mark_set)
    {
      double wx1, wx2, wy1, wy2 ;

      zmapWindowMarkGetWorldRange(window->mark, &wx1, &wy1, &wx2, &wy2) ;

      *start = (int)(wy1) ;
      *end = (int)(wy2) ;

      if (window->display_forward_coords)
	{
	  *start = zmapWindowCoordToDisplay(window, *start) ;
	  *end   = zmapWindowCoordToDisplay(window, *end) ;
	}

      result = TRUE ;
    }

  return result ;
}

/*!
 * \brief Create a ZMapWindowMark
 *
 * \param window  The window to create the mark for.
 *
 * \return The newly allocated ZMapWindowMark, which should be zmapWindowMarkDestroy()d
 *         when finished with.
 */

ZMapWindowMark zmapWindowMarkCreate(ZMapWindow window)
{
  ZMapWindowMark mark ;

  zMapAssert(window) ;

  mark = g_new0(ZMapWindowMarkStruct, 1) ;

  mark->magic = mark_magic_G ;

  mark->mark_set = FALSE ;

  mark->window = window ;

  mark->mark_src_item = NULL ;

  mark->seq_start = mark->seq_end = 0 ;

  mark->margin = 0;

  zmapWindowMarkSetColour(mark, ZMAP_WINDOW_ITEM_MARK) ;

  mark->stipple = gdk_bitmap_create_from_data(NULL, &mark_bitmap_bits_G[0], mark_bitmap_width, mark_bitmap_height) ;

  return mark ;
}

/*!
 * \brief Access to the state of a mark
 * 
 * This function returns whether the mark is set.
 *
 * \param mark  The mark to interrogate
 *
 * \return boolean TRUE = set, FALSE = unset
 */
gboolean zmapWindowMarkIsSet(ZMapWindowMark mark)
{
  gboolean result;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  /* marking is 'asynchronous' during redraws for marks with no mark_src_item. */
  /* mark->block_container is only populated for non-SetItem marks when the blocks are drawn. */
  /* including mark->block_container in the isset calculation therefore leads to lost marks
   * when save state does a mark is set before the redraw has completed.
   */
  /* This is not a problem for MarkSetItem marks as the block_container is known before hand
   * and is therefore saved and available any time afterwards to save state. */
  result = (mark->mark_set && mark->block_container);
  result = (mark->mark_set);

  if(mark->mark_set)
    {
      if(mark->mark_src_item)
	result = (mark->mark_set && mark->block_container);
      else
	result = (mark->mark_set);
    }

  return result;
}

/*!
 * \brief Reset the mark i.e. Unmark
 * 
 * \param mark  The mark to reset
 *
 * \return nothing.
 */
void zmapWindowMarkReset(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  if (mark->mark_set)
    {
      mark->mark_set = FALSE ;

      if(mark->block_container)
	zmapWindowContainerBlockUnmark(mark->block_container);

      mark->block_container = NULL;

      if (mark->mark_src_item)
	{
	  /* undo highlighting */
	  markItem(mark, mark->mark_src_item, FALSE) ;

	  mark->mark_src_item = NULL ;
	}

      /* reset all the coords */
      mark->world_x1  = mark->world_x2 = 0.0;
      mark->world_y1  = mark->world_y2 = 0.0;
      mark->seq_start = mark->seq_end  = 0 ;
    }

  return ;
}

/*!
 * \brief Set the colour for the mark
 * 
 * \param mark   The mark
 * \param colour The colour as a string for gdk_color_parse
 *
 * \return nothing.
 */
void zmapWindowMarkSetColour(ZMapWindowMark mark, char *colour)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  gdk_color_parse(colour, &(mark->colour)) ;

  return ;
}

/*!
 * \brief Get the colour the mark is using.
 * 
 * \param mark  The mark
 *
 * \return rhe GdkColor.
 */
GdkColor *zmapWindowMarkGetColour(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  return &(mark->colour) ;
}

/*!
 * \brief Set the stipple for the mark.
 * 
 * \param mark    The mark
 * \param stipple The stipple
 *
 * \return nothing.
 */
void zmapWindowMarkSetStipple(ZMapWindowMark mark, GdkBitmap *stipple)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  if(mark->stipple)
    {
      g_object_unref(G_OBJECT(mark->stipple)) ;
    }

  mark->stipple = stipple;

  return ;
}

/*!
 * \brief Get the stipple from the mark
 * 
 * \param mark  The mark
 *
 * \return GdkBitmap.
 */

GdkBitmap *zmapWindowMarkGetStipple(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  return mark->stipple ;
}

/* Mark an item, the marking must be done explicitly, it is unaffected by highlighting. 
 * Note that the item must be a feature item.
 * 
 * The coords are all -1 or +1 to make sure we don't clip the given item with the
 * marking.
 *  */
void zmapWindowMarkSetItem(ZMapWindowMark mark, FooCanvasItem *item)
{
  ZMapFeature feature ;
  double x1, y1, x2, y2 ;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic) && FOO_IS_CANVAS_ITEM(item)) ;

  zmapWindowMarkReset(mark) ;

  mark->mark_src_item = item ;

  /* We need to get the world coords because the item passed in is likely to be a child of a child
   * of a.... */
  my_foo_canvas_item_get_world_bounds(mark->mark_src_item, &x1, &y1, &x2, &y2) ;

  feature = zMapWindowCanvasItemGetFeature(mark->mark_src_item) ;
  zMapAssert(feature) ;

  mark->seq_start = feature->x1 - 1 ;
  mark->seq_end   = feature->x2 + 1 ;

  mark->block_container = 
    (ZMapWindowContainerBlock)zmapWindowContainerUtilsItemGetParentLevel(mark->mark_src_item, 
									 ZMAPCONTAINER_LEVEL_BLOCK) ;

  markItem(mark, mark->mark_src_item, TRUE) ;

  markRange(mark, y1 - mark->margin, y2 + mark->margin) ;


  return ;
}


/*!
 * \brief Get the item.
 * 
 * \param mark  The mark
 *
 * \return FooCanvasItem that is the source of the mark.
 */
FooCanvasItem *zmapWindowMarkGetItem(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  return mark->mark_src_item ;
}

/*!
 * \brief Set the world range of the mark
 * 
 * \param mark  The mark
 * \param x1    The start x coord
 * \param y1    The start y coord
 * \param x2    The end x coord
 * \param y2    The end y coord
 *
 * \return boolean.
 */
gboolean zmapWindowMarkSetWorldRange(ZMapWindowMark mark,
				     double world_x1, double world_y1, 
				     double world_x2, double world_y2)
{
  double scroll_x1, scroll_x2;
  gboolean result ;

  zMapAssert(mark);
  zMapAssert(ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  zmapWindowMarkReset(mark) ;
  
  /* clamp x to scroll region. Fix RT # 55131 */
  zmapWindowGetScrollRegion(mark->window, &scroll_x1, NULL, &scroll_x2, NULL);

  if(world_x2 > scroll_x2)
    {
      world_x2 = scroll_x2;
      /* Together with swapping below, appears to fix RT # 68249, thanks to log from RT # 107182 */
      world_x1 = ((scroll_x1) + ((scroll_x2 - scroll_x1) / 2));
    }

  if(world_y1 >= world_y2)
    world_y1 = world_y2 - 1.0;

  if(world_x1 > world_x2)
    ZMAP_SWAP_TYPE(double, world_x1, world_x2);


  mark->world_x1 = world_x1 ;
  mark->world_y1 = world_y1 ;
  mark->world_x2 = world_x2 ;
  mark->world_y2 = world_y2 ;
  
  mark->mark_set = result = TRUE ;

  markRange(mark, world_y1, world_y2) ;

  return result ;
}

/*!
 * \brief Get the world range of the mark
 * 
 * \param mark  The mark
 * \param x1    Address to store the start x coord
 * \param y1    Address to store the start y coord
 * \param x2    Address to store the end x coord
 * \param y2    Address to store the end y coord
 *
 * \return boolean.
 */
gboolean zmapWindowMarkGetWorldRange(ZMapWindowMark mark,
				     double *world_x1, double *world_y1, double *world_x2, double *world_y2)
{
  gboolean result = FALSE ;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  if (mark->mark_set)
    {
      *world_x1 = mark->world_x1 ;
      *world_y1 = mark->world_y1 ;
      *world_x2 = mark->world_x2 ;
      *world_y2 = mark->world_y2 ;


#warning THIS_MUST_BE_CHECKED_FOR_CORRECTNESS
      /* I feel this has got to be wrong somehow. I bet this is a
       * misguided Ext2Zero, i2w or such like call */
      if(mark->mark_src_item)
	{
	  /* See the markRange() call in zmapWindowMarkSetItem() */
	  (*world_y2) -= mark->margin;
	  (*world_y1) += mark->margin;
	}

      result = TRUE ;
    }

  return result ;
}

/*!
 * \brief Get the sequence range of the mark
 * 
 * \param mark  The mark
 * \param start The start coord
 * \param end   The end coord
 *
 * \return boolean.
 */
gboolean zmapWindowMarkGetSequenceRange(ZMapWindowMark mark, int *start, int *end)
{
  gboolean result = FALSE ;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  if (mark->mark_set)
    {
      *start = mark->seq_start ;
      *end   = mark->seq_end ;

      result = TRUE ;
    }

  return result ;
}

/*!
 * \brief Get the current block container that is marked
 * 
 * \param mark  The mark
 *
 * \return container block or NULL.
 */
ZMapWindowContainerGroup zmapWindowMarkGetCurrentBlockContainer(ZMapWindowMark mark)
{
  ZMapWindowContainerGroup block = NULL;

  if(mark->mark_set)
    {
      block = (ZMapWindowContainerGroup)mark->block_container;
    }

  return block;
}

/*!
 * \brief Not for general use.  
 * 
 * It does not perform any marking, nor control any marking.  It is only called from 
 * the zmapWindowContainerBlock code when it has matched a mark area to its area.
 *
 */
gboolean zmapWindowMarkSetBlockContainer(ZMapWindowMark mark, ZMapWindowContainerGroup container,
					 double sequence_start, double sequence_end,
					 double x1, double y1, double x2, double y2)
{
  ZMapWindowContainerBlock block_container;
  gboolean result = FALSE;

  if(mark->mark_set)
    {
      if(ZMAP_IS_CONTAINER_BLOCK(container) && !mark->block_container)
	{
	  block_container       = (ZMapWindowContainerBlock)container;
	  mark->block_container = block_container;
	  mark->seq_start       = (int)sequence_start;
	  mark->seq_end         = (int)sequence_end;

	  result = TRUE;
	}

      /* This seems like a strange thing to do, but if we have set
       * an item the x coords might not be set. */
      if(mark->world_x1 != x1 && mark->world_x1 == 0.0)
	mark->world_x1 = x1;
      
      if(mark->world_x2 != x2 && mark->world_x2 == 0.0)
	mark->world_x2 = x2;
      
      /* I also want to check we have the correct y coords. */
      if(mark->world_y1 != y1)
	zMapLogWarning("mark start coord (%f) does not match parameter (%f)", mark->world_y1, y1);
      
      if(mark->world_y2 != y2)
	zMapLogWarning("mark end coord (%f) does not match parameter (%f)", mark->world_y2, y2);
    }

  return result;
}


/*!
 * \brief Free the memory used by a ZMapWindowMark
 *
 * The should be changed to return a NULL ZMapWindowMark pointer.
 * 
 * \param mark  The mark to free.
 *
 * \return nothing
 */
void zmapWindowMarkDestroy(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  zmapWindowMarkReset(mark) ;

  g_object_unref(G_OBJECT(mark->stipple)) ;		    /* This seems to be the only way to
							       get rid of a drawable. */

  g_free(mark) ;

  return ;
}




/*
 *               Internal functions
 */


/* Mark/unmark an item with a highlight colour. */
/* 
 * The bounding_box is a bit of a hack to support marking objects,
 * a better way would be to use masks as overlays but I don't have time just
 * now. ALSO....if you put highlighting over the top of the feature you won't
 * be able to click on it any more....agh....something to solve.....
 * 
 * This is solved by the ZMapWindowCanvasItems
 */
static void markItem(ZMapWindowMark mark, FooCanvasItem *item, gboolean set_mark)
{
  if(set_mark)
    {
      GdkColor  *mark_colour;
      GdkBitmap *mark_stipple;
      
      mark_colour  = zmapWindowMarkGetColour(mark);
      mark_stipple = mark->stipple;
      
      zMapWindowCanvasItemMark((ZMapWindowCanvasItem)item, mark_colour, mark_stipple);
    }
  else
    zMapWindowCanvasItemUnmark((ZMapWindowCanvasItem)item);

  return ;
}

/* The range markers are implemented as overlays in the block container. This makes sense because
 * a mark is relevant only to its parent block. */
static void markRange(ZMapWindowMark mark, double y1, double y2)
{
  double block_y1, block_y2, tmp_y1, tmp_y2 ;

  tmp_y1 = y1 ;
  tmp_y2 = y2 ;

  /* This seems wierd but it only really happens when we get marks very close to the end of blocks
   * and it's because items can be slightly bigger than blocks because of the thickness of their
   * lines and foocanvas does not pick this up. So by reversing coords we can calculate these
   * marks zones correctly at the very ends of the blocks. */
  if (tmp_y1 < block_y1)
    {
      double tmp ;

      tmp = tmp_y1 ;
      tmp_y1 = block_y1 ;
      block_y1 = tmp ;
    }
  
  if (block_y2 < tmp_y2)
    {
      double tmp ;

      tmp = tmp_y2 ;
      tmp_y2 = block_y2 ;
      block_y2 = tmp ;
    }

  mark->world_y1 = y1;
  mark->world_y2 = y2;
  mark->mark_set = TRUE ;

  if(mark->block_container)
    {
      /* Hey, we've got the block.  We'll just mark that block rather
       * than go to each one and potentially mark one. */
      zmapWindowContainerBlockMark(mark->block_container, mark);  
    }
  else
    {
      /* Potentially we could mark the wrong block, or multiple blocks.
       * marking multiple blocks that have intersections in y axis is
       * probably not bad and might be desired... Actually thinking
       * about the code this is probably what would happen.
       */
      zmapWindowContainerUtilsExecute(mark->window->feature_root_group,
				      ZMAPCONTAINER_LEVEL_BLOCK,
				      mark_block_cb,
				      mark);
    }

  return ;
}


static void mark_block_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points, 
			  ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_BLOCK:
      {
	ZMapWindowContainerBlock block = (ZMapWindowContainerBlock)container;
	ZMapWindowMark mark = (ZMapWindowMark)user_data;

	zmapWindowContainerBlockMark(block, mark);
      }
      break;
    default:
      break;
    }
}

