/*  File: zmapWindowMark.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the "mark" in zmap, this is an "active" zone
 *              in zmap marked by overlaid boundaries either side. The
 *              user can limit certain operations to the marked zone
 *              which makes them much faster.
 *
 * Exported functions: See zmapWindow.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>

#include <ZMap/zmapUtils.h>
#include <zmapWindow_P.h>	/* ZMapWindow */
#include <zmapWindowMark.h>
#include <zmapWindowCanvasItem.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerBlock.h>

/* We draw the mark top/bottom overlays just outside of the sequence coords of the mark
 * so that the overlays do not go over the feature. */
#define MARK_MARGIN 1

#define mark_bitmap_width 16
#define mark_bitmap_height 4



/* Used to find containing block for a given world position. */
typedef struct GetBlockStructType
{
  ZMapWindow window ;
#if TEST_BLOCK
  double world_x1, world_y1, world_x2, world_y2 ;
#endif
  ZMapWindowContainerBlock block ;
} GetBlockStruct, *GetBlock ;


typedef struct _ZMapWindowMarkStruct
{
  ZMapMagic magic ;

  ZMapWindow window ;					    /* Mark needs access to the ZMapWindow
							       without having to pass it in. */

  ZMapWindowContainerBlock block_container ;		    /* The group the mark is set
							       on. ColConfig assumes this is a block */

  gboolean mark_set ;					    /* internal flag for whether mark is set.  */

  double margin ;					    /* Sets a small margin so that the
							       mark is drawn just outside a
							       feature and not over the ends of it. */

  /* Bases _wholly_ contained in mark, this is the definitive range of sequence coords that are
   * within the marked range. If set from an item then they are -/+ margin sequence coords
   * either side of the feature to ensure mark does not clip feature. */
  int seq_start ;					    /* start of the mark in seq coords (of block)  */
  int seq_end   ;					    /* end of the mark in seq coords (of block)  */

  int			prev_start, prev_end;		    /* where it used to be */

  /* World coords of the mark, these are used to draw the mark, they should be for graphical
   * use only, e.g. is an canvas item within the mark ? etc. They are ALWAYS derived
   * directly from the sequence start/end coords */
  double          world_x1 ;
  double          world_y1 ;
  double          world_x2 ;
  double          world_y2 ;


  /* The stippled areas above and below the mark. */
  GdkColor colour ;					    /* colour */
  GdkBitmap *stipple ;					    /* stipple pattern */
  FooCanvasItem *mark_top, *mark_bot ;


} ZMapWindowMarkStruct ;



static void markRange(ZMapWindowMark mark) ;
static ZMapWindowContainerBlock getBlock(ZMapWindow window, double world_y1, double world_y2) ;
static void get_block_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
			 ZMapContainerLevelType level, gpointer user_data) ;
static FooCanvasItem *set_mark_item(ZMapWindowMark mark, gboolean top) ;




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
 *
 * - The mark is set to be outside the item coords selected or outside the
 *   range of sequence coords selected via world coords.
 *
 * - Using an item leads to 'synchronous' setting of the mark with respect
 *   to the ZMapWindowContainerBlock that is stored in the mark.
 *
 * - Using a region leads to 'asynchronous' setting of the mark with respect
 *   to the ZMapWindowContainerBlock theat is stored.
 *
 * - The items that display the mark are handled in the ZMapWindowContainerBlock
 *   code.  This is the root cause of the sync/async nature.
 *
 * - The mark items are stippled using the supplied stipple.
 *
 * - The mark items must be resized by some means for long items reasons.
 *
 * - The long items cropping/resizing is handled in the ZMapWindowContainerBlock
 *   code by the update hooks the ZMapWindowContainerGroup interface provides.
 *
 * - Initial selection of the correct block (setting by world coord) is again
 *   done in the container code by way of the update hooks.  Selection works
 *   on a world coord basis.  When setting by item the item's parent's parent
 *   is used.
 *
 * - The mark is saved across revcomp, vsplit, hsplit, by the ZMapWindowState
 *   code and it handles serializing the mark and calling the mark functions.
 *
 * - There are a couple of issues to watch out for when multiple blocks get displayed
 *    - The block_container will need to handle multiple blocks, as will the interface.
 *    - Selecting the correct block might need thought.  I think it's ok as it is,
 *      but check out the container block code.
 */






/* Create a ZMapWindowMark, note that this simply creates the data struct,
 * if does not set a mark zone in the the window.
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

  mark->seq_start = mark->seq_end = 0 ;

  mark->prev_start = mark->prev_end  = -1 ;

  mark->margin = MARK_MARGIN ;

  zmapWindowMarkSetColour(mark, ZMAP_WINDOW_ITEM_MARK) ;

  mark->stipple = gdk_bitmap_create_from_data(NULL, &mark_bitmap_bits_G[0], mark_bitmap_width, mark_bitmap_height) ;

  return mark ;
}

gboolean zMapWindowMarkIsSet(ZMapWindow window)
{
  /* scope and header isssues... */

  return zmapWindowMarkIsSet(window->mark) ;
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
  //  result = (mark->mark_set && mark->block_container);

  result = (mark->mark_set);

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

      if(mark->mark_top)
	gtk_object_destroy(GTK_OBJECT(mark->mark_top));
      mark->mark_top = NULL;
      if(mark->mark_bot)
	gtk_object_destroy(GTK_OBJECT(mark->mark_bot));
      mark->mark_bot = NULL;

      /* reset all the coords */
      mark->world_x1  = mark->world_x2 = 0.0;
      mark->world_y1  = mark->world_y2 = 0.0;
      mark->seq_start = mark->seq_end  = 0 ;
      mark->prev_start = mark->prev_end  = -1 ;
    }

  return ;
}

void zmapWindowToggleMark(ZMapWindow window, gboolean whole_feature)
{
  FooCanvasItem *focus_item ;

  if (zmapWindowMarkIsSet(window->mark))
    {
      zMapWindowStateRecord(window);
      /* Unmark an item. */
      zmapWindowMarkReset(window->mark) ;
    }
  else
    {
      zMapWindowStateRecord(window);
      /* If there's a focus item we mark that, otherwise we check if the user set
       * a rubber band area and use that, otherwise we mark to the screen area. */
      if ((focus_item = zmapWindowFocusGetHotItem(window->focus)))
	{
	  ZMapFeature feature ;

	  feature = zMapWindowCanvasItemGetFeature(focus_item);
	  zMapAssert(zMapFeatureIsValid((ZMapFeatureAny)feature)) ;

	  /* If user presses 'M' we mark "whole feature", e.g. whole transcript,
	   * all HSP's, otherwise we mark just the highlighted ones. */
	  if (whole_feature)
	    {
	      if (feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
		{
		  GList *list = NULL;
		  ZMapStrand set_strand ;
		  ZMapFrame set_frame ;
		  gboolean result ;
		  double rootx1, rooty1, rootx2, rooty2 ;

		  result = zmapWindowItemGetStrandFrame(focus_item, &set_strand, &set_frame) ;
		  zMapAssert(result) ;

		  list = zmapWindowFToIFindSameNameItems(window,window->context_to_item,
							 zMapFeatureStrand2Str(set_strand),
							 zMapFeatureFrame2Str(set_frame),
							 feature) ;

		  zmapWindowGetMaxBoundsItems(window, list, &rootx1, &rooty1, &rootx2, &rooty2) ;

		  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;

		  g_list_free(list) ;
		}
	      else
		{
		  zmapWindowMarkSetItem(window->mark, focus_item) ;
		}
	    }
	  else
	    {
	      GList *focus_items ;
	      double rootx1, rooty1, rootx2, rooty2 ;

	      focus_items = zmapWindowFocusGetFocusItems(window->focus) ;

	      if(g_list_length(focus_items) == 1)
		{
		  ID2Canvas id2c = (ID2Canvas) focus_items->data;
		  zmapWindowMarkSetItem(window->mark, id2c->item);
		}
	      else
		{
		  zmapWindowGetMaxBoundsItems(window, focus_items, &rootx1, &rooty1, &rootx2, &rooty2) ;

		  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;
		}

	      g_list_free(focus_items) ;
	    }
	}
      else if (window->rubberband)
	{
	  double rootx1, rootx2, rooty1, rooty2 ;

	  my_foo_canvas_item_get_world_bounds(window->rubberband, &rootx1, &rooty1, &rootx2, &rooty2);

	  zmapWindowClampedAtStartEnd(window, &rooty1, &rooty2);
	  /* We ignore any failure, perhaps we should warn the user ? If we colour
	   * the region it will be ok though.... */
	  zmapWindowMarkSetWorldRange(window->mark, rootx1, rooty1, rootx2, rooty2) ;
	}
      else
	{
	  /* If there is no feature selected and no rubberband set then mark to the
	   * visible screen. */
	  double x1, x2, y1, y2;
	  double margin ;

	  zmapWindowItemGetVisibleWorld(window, &x1, &y1, &x2, &y2) ;

	  zmapWindowClampedAtStartEnd(window, &y1, &y2) ;

	  /* Make the mark visible to the user by making its extent slightly smaller
	   * than the window. */
	  margin = 15.0 * (1 / window->canvas->pixels_per_unit_y) ;
	  y1 += margin ;
	  y2 -= margin ;
	  /* We only ever want the mark as wide as the scroll region */
	  zmapWindowGetScrollableArea(window, NULL, NULL, &x2, NULL);

	  zmapWindowMarkSetWorldRange(window->mark, x1, y1, x2, y2) ;
	}
    }

  return ;
}


/* add overlay stuff to show where it is */
/* this goes on to the marks block_container */
void zmapWindowMarkShowMark(ZMapWindowMark mark)
{
  if(!mark->block_container)
    return;

  if(mark->seq_start != mark->prev_start)		/* only display if changed */
    mark->mark_top = set_mark_item(mark, TRUE);

  if(mark->seq_end != mark->prev_end)		/* only display if changed */
    mark->mark_bot = set_mark_item(mark, FALSE);

  mark->prev_start = mark->seq_start;
  mark->prev_end = mark->seq_end;

  /* dissapointingly it's quite difficult to request a redraw of an item until it's been updated,
   * something that involves the containing groups
   * we need to calc the future canvas coords and request a foo canvas redraw
   * instead redraw the whole block
   */
  foo_canvas_item_request_redraw((FooCanvasItem *) mark->block_container);

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
 * \return the GdkColor.
 */
GdkColor *zmapWindowMarkGetColour(ZMapWindowMark mark)
{
  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  return &(mark->colour) ;
}




/* Mark an item, the marking must be done explicitly, it is unaffected by highlighting.
 * Note that the item must be a feature item.
 */
gboolean zmapWindowMarkSetItem(ZMapWindowMark mark, FooCanvasItem *item)
{
  gboolean result = FALSE ;
  ZMapFeature feature ;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic) && FOO_IS_CANVAS_ITEM(item)) ;

  if (!(feature = zMapWindowCanvasItemGetFeature(item)))
    {
      zMapLogCritical("Unable to retrieve feature from canvas item %p for setting mark.", item) ;
      result = FALSE ;
    }
  else
    {
      ZMapFeatureBlock block ;
      double draw_start, draw_end ;

      zmapWindowMarkReset(mark) ;

      /* Get hold of the block this item sits in. */
      mark->block_container =
	(ZMapWindowContainerBlock)zmapWindowContainerUtilsItemGetParentLevel(item,
									     ZMAPCONTAINER_LEVEL_BLOCK) ;

      zmapWindowContainerGetFeatureAny(ZMAP_CONTAINER_GROUP(mark->block_container), (ZMapFeatureAny *)&block) ;


      /* Set the seq_start and seq_end from the feature coords, note the mark
       * is set just outside the feature, also note that the feature can extend
       * outside the block (though it's drawn coords are clipped to the block)
       * so we need to clip to the block. */
      mark->seq_start = feature->x1 - mark->margin ;
      mark->seq_end = feature->x2 + mark->margin ;

      draw_start = (double)(mark->seq_start) ;
      draw_end = (double)(mark->seq_end) ;
      zmapWindowSeq2CanExt(&draw_start, &draw_end) ;
      mark->seq_start =  (int)draw_start ;
      mark->seq_end = (int)draw_end ;


      if (mark->seq_start < block->block_to_sequence.block.x1)
        mark->seq_start = block->block_to_sequence.block.x1 ;
      if (mark->seq_end > block->block_to_sequence.block.x2)
        mark->seq_end = block->block_to_sequence.block.x2 ;

      /* Now put the overlays above/below the marked region. */
      markRange(mark) ;

      mark->mark_set = result = TRUE ;
    }

  return result ;
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
  zmapWindowGetScrollableArea(mark->window, &scroll_x1, NULL, &scroll_x2, NULL);

  if (world_x2 > scroll_x2)
    {
      world_x2 = scroll_x2;

      /* Together with swapping below, appears to fix RT # 68249, thanks to log from RT # 107182 */
      world_x1 = ((scroll_x1) + ((scroll_x2 - scroll_x1) / 2)) ;
    }

  if (world_y1 >= world_y2)
    world_y1 = world_y2 - 1.0 ;

  if (world_x1 > world_x2)
    ZMAP_SWAP_TYPE(double, world_x1, world_x2) ;

#if MARK_BLOCK
  /* At this point we always want to get the block container. */
  mark->block_container = getBlock(mark->window, world_x1, world_y1, world_x2, world_y2) ;


  /* we need to convert these coords to sequence coords inside block and then
   * back convert to world coords.... */
#endif

  mark->world_x1 = world_x1 ;
  mark->world_y1 = world_y1 ;
  mark->world_x2 = world_x2 ;
  mark->world_y2 = world_y2 ;

  //  mark->seq_start = 0;  /* if set is from set mark from feature, if we move the mark we need to recalc this */
  //  mark->seq_end = 0;

  /* Now sort out the seq coords which should be inside the world coords. */
  mark->seq_start = ceil(world_y1 - mark->margin) ;
  mark->seq_end = floor(world_y2 + mark->margin) ;


  /* Now put the overlays above/below the marked region. */
  markRange(mark) ;


  mark->mark_set = result = TRUE ;

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

      result = TRUE ;
    }

  return result ;
}



/* Get the start/end coords of the marked region of a window, the coords
 * are returned in the original coord range the reference sequence, not
 * the displayed range which may have been remapped to start at "1".
 * 
 * Returns TRUE and the start/end if there is a mark set, FALSE otherwise.
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

      result = TRUE ;
    }

  return result ;
}


/* GROSS HACK UNTIL I FIX THE MARK STUFF...what should happen is that we use
 * zmapWindowMarkGetSequenceRange() but both calls are bugged... */
gboolean zMapWindowMarkGetSequenceSpan(ZMapWindow window, int *start, int *end)
{
  gboolean result = FALSE ;
  ZMapWindowMark mark = window->mark ;

  zMapAssert(mark && ZMAP_MAGIC_IS_VALID(mark_magic_G, mark->magic)) ;

  if ((result = zmapWindowMarkGetSequenceRange(mark, start, end)))
    {
      /* TERRIBLE HACK...COORDS SHOULD BE DONE RIGHT.... */

      if (*start < window->sequence->start)
	*start = window->sequence->start ;
      if (*end > window->sequence->end)
	*end = window->sequence->end ;
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

      if (!mark->seq_start)    // ie it has never been set
	{
	  // fix for RT161721 bumped col diappears in hsplit/vsplit
	  // start and end are 0 in mark so no features displayed
	  mark->seq_start = (int) mark->world_y1 - 1 ;
	  mark->seq_end   = (int) mark->world_y2 + 1 ;
	}

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


/* Oh gosh, this all seems to point to some mixed code not properly encapsulated..... */
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

  if (mark->mark_set)
    {
      if(ZMAP_IS_CONTAINER_BLOCK(container) && !mark->block_container)
	{
	  block_container       = (ZMapWindowContainerBlock)container;
	  mark->block_container = block_container;
#warning Need to implement block relative coordinates for this to work
	  /* MH17: features are at absolute coordinates and we cannot have multiple blocks
	   * we need to use world coordinates for the mark to be able to request external data
	   * sequence_start and _end are block relative
	   */
	  /*
	    mark->seq_start       = (int)sequence_start;
	    mark->seq_end         = (int)sequence_end;
	  */

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


/* Debug stuff...print out mark state. */
void zmapWindowMarkPrint(ZMapWindow window, char *title)
{
  ZMapWindowMark mark = window->mark ;

  printf("%s -\n"
	 "Block:\t%p\n"
	 "World:\tx1=%g, y1=%g, x2=%g, y2=%g\n"
	 "Sequence:\tstart=%d, end=%d\n",
	 (title ? title : "Current mark"),
	 mark->block_container,
	 mark->world_x1, mark->world_y1, mark->world_x2, mark->world_y2,
	 mark->seq_start, mark->seq_end) ;

  fflush(stdout) ;

  return ;
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

static FooCanvasItem *set_mark_item(ZMapWindowMark mark, gboolean top)
{
  FooCanvasItem *foo = top ? mark->mark_top : mark->mark_bot;
  int y1,y2;
  static GQuark id_top, id_bot;
  GQuark id;
  static ZMapFeatureTypeStyle style = NULL;
  ZMapFeatureBlock block;
  char *tmp_name ;

  zmapWindowContainerGetFeatureAny(ZMAP_CONTAINER_GROUP(mark->block_container), (ZMapFeatureAny *)&block) ;

  tmp_name = g_strdup_printf("mark_top_%p", mark->window->canvas) ;
  id_top =  g_quark_from_string(tmp_name) ;
  g_free(tmp_name) ;

  tmp_name = g_strdup_printf("mark_bot_%p", mark->window->canvas) ;
  id_bot = g_quark_from_string(tmp_name) ;
  g_free(tmp_name) ;

  if(!style)
    {
      style = zMapStyleCreate("mark", "mark");

      g_object_set(G_OBJECT(style),
		   ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_PLAIN,
		   ZMAPSTYLE_PROPERTY_COLOURS, "normal draw cyan; normal border cyan",
		   NULL);
      zMapStyleSetDisplayable(style, TRUE);	/* (always drawable) */
    }

  if(top)
    {
      y1 = block->block_to_sequence.block.x1;
      y2 = mark->seq_start - 1;
      if(y2 < y1)
	y2 = y1;
      id = id_top;
    }
  else
    {
      y2 = block->block_to_sequence.block.x2;
      y1 = mark->seq_end + 1;
      if(y1 > y2)
	y1 = y2;
      id = id_bot;
    }

  if(!foo)
    {
      foo = (FooCanvasItem *) zMapWindowCanvasItemFeaturesetGetFeaturesetItem((FooCanvasGroup *) mark->block_container, id, y1, y2, style, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0, ZMAP_CANVAS_LAYER_BLOCK_MARK);
    }

  zMapWindowCanvasFeaturesetSetSequence((ZMapWindowFeaturesetItem) foo, y1, y2);
  zMapWindowCanvasFeaturesetSetBackground(foo, &mark->colour, NULL );
  zMapWindowCanvasFeaturesetSetStipple((ZMapWindowFeaturesetItem) foo, mark->stipple);

  foo_canvas_item_request_update(foo);
  foo_canvas_item_show(foo);

  return foo;
}


/* The range markers are implemented as overlays in the block container. This makes sense because
 * a mark is relevant only to its parent block. */
static void markRange(ZMapWindowMark mark)
{
  double block_y1, block_y2, tmp_y1, tmp_y2 ;
  ZMapFeatureBlock block = NULL ;

  tmp_y1 = mark->seq_start ;
  tmp_y2 = mark->seq_end ;

  if(!mark->block_container)
    mark->block_container = getBlock(mark->window, mark->world_y1, mark->world_y2) ;

  if(mark->block_container)
    {
      /* mh17: block_container can be null in whiich can block will be invalid */
      zmapWindowContainerGetFeatureAny(ZMAP_CONTAINER_GROUP(mark->block_container), (ZMapFeatureAny *)&block) ;
    }
  if(block)
    {
      block_y1 = block->block_to_sequence.block.x1 ;
      block_y2 = block->block_to_sequence.block.x2 ;


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
    }
  mark->world_y1 = tmp_y1;
  mark->world_y2 = tmp_y2;
  mark->mark_set = TRUE ;


  zmapWindowMarkShowMark(mark);

  return ;
}





/* This potentially should be converted to a generalised function that finds items in a
 * certain coord range, better than Roys bug-prone method of hoping he finds a canvas
 * item of a certain type which probably fails because the point is not over a feature item. */
static ZMapWindowContainerBlock getBlock(ZMapWindow window, double world_y1, double world_y2)
{
  ZMapWindowContainerBlock block = NULL ;
  GetBlockStruct get_block_data = {NULL} ;

#if TEST_BLOCK
  get_block_data.window = window ;
  get_block_data.world_y1 = world_y1 ;
  get_block_data.world_y2 = world_y2 ;
#endif

  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_BLOCK,
				  get_block_cb,
				  &get_block_data) ;

  block = get_block_data.block ;

  return block ;
}


static void get_block_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
			 ZMapContainerLevelType level, gpointer user_data)
{
  if (level == ZMAPCONTAINER_LEVEL_BLOCK)
    {
      ZMapWindowContainerBlock block = (ZMapWindowContainerBlock)container ;


      GetBlock get_block_data = (GetBlock)user_data ;
#if 0
      ZMapFeatureBlock b;

      b = (ZMapFeatureBlock) container->feature_any;


      /* this is a bit pedantic, we only have one block */

      if(get_block_data->world_x1 < block->block_to_sequence.block.x2 && get_block_data->world_x2 > block->block_to_sequence.block.x1)
#endif

	get_block_data->block = block ;
    }

  return ;
}




