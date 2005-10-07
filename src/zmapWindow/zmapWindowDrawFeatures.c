/*  File: zmapWindowDrawFeatures.c
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * and was written by
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk and
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Oct  5 21:49 2005 (rds)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.89 2005-10-07 10:55:19 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>



/* Used to pass data back to column menu callbacks. */
typedef struct
{
  GdkEventButton *button_event ;
  ZMapWindow window ;
  FooCanvasItem *item ;
  ZMapFeatureSet feature_set ;
} ColumnMenuCBDataStruct, *ColumnMenuCBData ;


/* Used to pass data to canvas item menu callbacks. */
typedef struct
{
  ZMapWindow window ;
  FooCanvasItem *item ;
} ItemMenuCBDataStruct, *ItemMenuCBData ;


/* these will go when scale is in separate window. */
#define SCALEBAR_OFFSET   0.0
#define SCALEBAR_WIDTH   50.0



/* parameters passed between the various functions drawing the features on the canvas, it's
 * simplest to cache the current stuff as we go otherwise the code becomes convoluted by having
 * to look up stuff all the time. */
typedef struct _ZMapCanvasDataStruct
{
  ZMapWindow window ;
  FooCanvas *canvas ;

  /* Records which alignment, block, set, type we are processing. */
  ZMapFeatureContext full_context ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  GQuark curr_set_name ;

  GQuark curr_forward_col_id ;
  GQuark curr_reverse_col_id ;

  /* Records current positional information. */
  double curr_x_offset ;
  double curr_y_offset ;
  double curr_forward_offset ;				    /* I think we do need these... */
  double curr_reverse_offset ;


  /* Records current canvas item groups, these are the direct parent groups of the display
   * types they contain, e.g. curr_root_group is the parent of the align */
  FooCanvasGroup *curr_root_group ;
  FooCanvasGroup *curr_align_group ;
  FooCanvasGroup *curr_block_group ;
  FooCanvasGroup *curr_forward_group ;
  FooCanvasGroup *curr_reverse_group ;
  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

  GdkColor colour_root ;
  GdkColor colour_alignment ;
  GdkColor colour_block ;
  GdkColor colour_mblock_for ;
  GdkColor colour_mblock_rev ;
  GdkColor colour_qblock_for ;
  GdkColor colour_qblock_rev ;
  GdkColor colour_mforward_col ;
  GdkColor colour_mreverse_col ;
  GdkColor colour_qforward_col ;
  GdkColor colour_qreverse_col ;

  double bump_for, bump_rev ;

} ZMapCanvasDataStruct, *ZMapCanvasData ;




static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;


static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext diff_context) ;
static void drawAlignments(GQuark key_id, gpointer data, gpointer user_data) ;
static void drawBlocks(gpointer data, gpointer user_data) ;
static void createSetColumn(gpointer data, gpointer user_data) ;
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data) ;
static void positionColumn(gpointer data, gpointer user_data) ;
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    GQuark feature_set_id,
				    ZMapStrand strand,
				    double start, double top, double bot, GdkColor *colour) ;
static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			   FooCanvasItem *item, ZMapFeatureSet feature_set) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;
static FooCanvasItem *getBoundingBoxChild(FooCanvasGroup *parent_group) ;

static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline, GdkColor *background,
					ZMapWindow window) ;
static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, GdkColor *background,
					    GdkColor *block_background,
					    ZMapWindow window) ;


static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window,
			 FooCanvasItem *item) ;
static void itemMenuCB(int menu_item_id, gpointer callback_data) ;
static void blixemMenuCB(int menu_item_id, gpointer callback_data) ;

static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item) ;

static void setColours(ZMapCanvasData canvas_data) ;


/* Drawing coordinates: PLEASE READ THIS BEFORE YOU START MESSING ABOUT WITH ANYTHING...
 * 
 * It seems that item coordinates are _not_ specified in absolute world coordinates but
 * in fact in the coordinates of their parent group. This has a lot of knock on effects as
 * we get our feature coordinates in absolute world coords and wish to draw them as simply
 * as possible.
 * 
 * Currently we have these coordinate systems operating:
 * 
 * 
 * min_coord     min_coord   min_coord      min_coord
 *  ^             ^             ^              ^
 *                                                       scr_start
 *                                                          ^
 * 
 * root          col           col            all         scroll
 * group        group(s)     bounding       features      region
 *                             box
 * 
 *                                                          v
 *                                                       scr_end
 *  v             v             v              v
 * max_coord     max_coord   max_coord      max_coord
 * 
 * 
 * where min_coord = seq_start and max_coord = seq_end + 1 (to cover the last base)
 * 
 */


/************************ external functions ************************************************/


/* REMEMBER WHEN YOU READ THIS CODE THAT THIS ROUTINE MAY BE CALLED TO UPDATE THE FEATURES
 * IN A CANVAS WITH NEW FEATURES FROM A SEPARATE SERVER. */

/* Draw features on the canvas, I think that this routine should _not_ get called unless 
 * there is actually data to draw.......
 * 
 * full_context contains all the features.
 * diff_context contains just the features from full_context that are newly added and therefore
 *              have not yet been drawn.
 * 
 * So NOTE that if no features have _yet_ been drawn then  full_context == diff_context
 * 
 *  */
void zmapWindowDrawFeatures(ZMapWindow window,
			    ZMapFeatureContext full_context, ZMapFeatureContext diff_context)
{
  GtkAdjustment *h_adj;
  ZMapCanvasDataStruct canvas_data = {NULL} ;		    /* Rest of struct gets set to zero. */
  double x1, y1, x2, y2 ;
  FooCanvasGroup *root_group ;
  FooCanvasItem *background_item ;
  gboolean debug = TRUE ;

  zMapAssert(window && full_context && diff_context) ;


  /* Set up colours. */
  setColours(&canvas_data) ;


  /* Must be reset each time because context will change as features get merged in. */
  window->feature_context = full_context ;

  window->seq_start = full_context->sequence_to_parent.c1 ;
  window->seq_end   = full_context->sequence_to_parent.c2 ;
  window->seqLength = zmapWindowExt(window->seq_start, window->seq_end) ;

  zmapWindowZoomControlInitialise(window); /* Sets min/max/zf */
  window->min_coord = window->seq_start ;
  window->max_coord = window->seq_end ;
  zmapWindowSeq2CanExt(&(window->min_coord), &(window->max_coord)) ;

  h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, zMapWindowGetZoomFactor(window)) ;

  canvas_data.window = window;
  canvas_data.canvas = window->canvas;


  /* Draw the scale bar. */
  foo_canvas_get_scroll_region(window->canvas, NULL, &y1, NULL, &y2);
  zmapWindowDrawScaleBar(window, y1, y2);


  /* Make sure we start drawing alignments to the right of the scale bar. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;
  window->alignment_start = x2 + COLUMN_SPACING ;


  /* Add a background to the root window... */
  root_group = zmapWindowContainerCreate(foo_canvas_root(window->canvas),
					 &(canvas_data.colour_root), &(window->canvas_border)) ;
  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(root_group) ;
  zmapWindowLongItemCheck(window, zmapWindowContainerGetBackground(root_group), y1, y2) ;

  /* There is a hole in the ftoi stuff in a way in that we don't have the context right at
   * the top......... */


  /* 
   *     Draw all the features, so much in so few lines...sigh...
   */
  canvas_data.curr_x_offset = window->alignment_start ;
  canvas_data.full_context = full_context ;
  drawZMap(&canvas_data, diff_context) ;


  /* Set the background object size now we have finished... */
  zmapWindowContainerSetBackgroundSize(root_group, 0.0) ;


  /* There may be a focus item if this routine is called as a result of splitting a window
   * or adding more features, make sure we scroll to the same point as we were
   * at in the previously. */
  if (window->focus_item)
    {
      zMapWindowScrollToItem(window, window->focus_item) ;
    }
  else
    {
      double scroll_x1, scroll_y1, scroll_x2, scroll_y2 ;

      foo_canvas_get_scroll_region(window->canvas, &scroll_x1, &scroll_y1, &scroll_x2, &scroll_y2) ;
      if (scroll_x1 == 0 && scroll_y1 == 0 && scroll_x2 == 0 && scroll_y2 == 0)
	foo_canvas_set_scroll_region(window->canvas,
				     0.0, window->min_coord,
				     h_adj->page_size, window->max_coord) ;
      /* Not sure what this set_scroll_region achieves, page_size is
         likely to be too small at some point (more and more
         columns) */
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Now hide/show any columns that have specific show/hide zoom factors set. */
  if (window->columns)
    zmapHideUnhideColumns(window) ;

  /* we need a hide columns call here..... */

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zmapWindowLongItemCrop(window) ;


  /* Expand the scroll region to include everything, note the hard-coded zero start, this is
   * because the actual visible window may not change when the scrolled region changes if its
   * still visible so we can end up with the visible window starting where the alignment box.
   * starts...should go away when scale moves into separate window. */  
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;

  /* Expand the scroll region to include everything again as we need to include the scale bar. */  
  foo_canvas_get_scroll_region(window->canvas, NULL, &y1, NULL, &y2);

  if(y1 && y2)
    zmapWindow_set_scroll_region(window, y1, y2);
  else
    zmapWindow_set_scroll_region(window, window->min_coord, window->max_coord);


  foo_canvas_item_request_redraw(FOO_CANVAS_ITEM(root_group));
  zmapWindowContainerPrint(root_group) ;



  return ;
}




/************************ internal functions **************************************/


/* NOTE THAT WHEN WE COME TO MERGE DATA FROM SEVERAL SOURCES WE ARE GOING TO HAVE TO
 * CHECK AT EACH STAGE WHETHER A PARTICULAR ALIGN, BLOCK OR SET ALREADY EXISTS... */


static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext diff_context)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (!canvas_data->window->alignments)
    canvas_data->window->alignments = diff_context->alignments ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  g_datalist_foreach(&(diff_context->alignments), drawAlignments, canvas_data) ;


  /* Reorder list of alignments based on their position. */
  zmapWindowCanvasGroupChildSort(foo_canvas_root(canvas_data->window->canvas)) ;


  return ;
}


/* Draw all the alignments in a context, one of these is special in that it is the master
 * sequence that all the other alignments are aligned to. Commonly zmap will only have
 * the master alignment for many users as they will just view the features on a single
 * sequence. */
static void drawAlignments(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  ZMapCanvasData canvas_data = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  double x1, y1, x2, y2 ;
  double top, bottom ;
  double position ;
  gboolean status ;
  FooCanvasGroup *align_parent ;
  FooCanvasItem *background_item ;


  canvas_data->curr_alignment = alignment ;

  /* Always reset the aligns to start at y = 0. */
  canvas_data->curr_y_offset = 0.0 ;


  /* Not needed anymore ??? */
  top = window->min_coord ;
  bottom = window->max_coord ;
  zmapWindowExt2Zero(&top, &bottom) ;

  align_parent = zmapWindowContainerCreate(canvas_data->curr_root_group,
					   &(canvas_data->colour_alignment),
					   &(window->canvas_border)) ;
  canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(align_parent),
		      "x", canvas_data->curr_x_offset,
		      "y", canvas_data->curr_y_offset,
		      NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_object_set_data(G_OBJECT(align_parent), "item_feature_type",
		    GINT_TO_POINTER(ITEM_ALIGN)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  g_object_set_data(G_OBJECT(align_parent), "item_feature_data", alignment) ;

  status = zmapWindowFToIAddAlign(window->context_to_item, key_id, align_parent) ;


  /* Do all the blocks within the alignment. */
  g_list_foreach(alignment->blocks, drawBlocks, canvas_data) ;


  /* We should have a standard routine to do this.... */

  /* We find out how wide the alignment ended up being after we drew all the blocks/columns
   * and then we draw the next alignment to the right of this one. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(align_parent), &x1, &y1, &x2, &y2) ;

  canvas_data->curr_x_offset = x2 + ALIGN_SPACING ;	    /* Must come before we reset x2 below. */


  /* Set the background object size for the align now we have finished drawing... */
  zmapWindowContainerSetBackgroundSize(align_parent, 0.0) ;


  /* N.B. no need to sort blocks as they are vertically aligned. */

  return ;
}


/* Draw all the blocks within an alignment, these will vertically one below another and positioned
 * according to their alignment on the master sequence. If we are drawing the master sequence,
 * the code assumes this is represented by a single block that spans the entire master alignment. */
static void drawBlocks(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data ;
  ZMapCanvasData canvas_data = (ZMapCanvasData)user_data ;
  gboolean status ;
  double x1, y1, x2, y2 ;
  FooCanvasItem *colgroup_background ;
  GdkColor *for_bg_colour, *rev_bg_colour ;
  double top, bottom ;
  FooCanvasGroup *block_parent, *forward_group, *reverse_group ;


  /* not needed now...????? */
  top = block->block_to_sequence.t1 ;
  bottom = block->block_to_sequence.t2 ;
  zmapWindowSeq2CanExtZero(&top, &bottom) ;


  canvas_data->curr_block = block ;

  /* Always set y offset to be top of current block. */
  canvas_data->curr_y_offset = block->block_to_sequence.t1 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(canvas_data->curr_align_group),
		     "block offset", 0.0, canvas_data->curr_y_offset) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Add a group for the block and groups for the forward and reverse columns. */
  block_parent = zmapWindowContainerCreate(canvas_data->curr_align_group,
					   &(canvas_data->colour_block),
					   &(canvas_data->window->canvas_border)) ;
  canvas_data->curr_block_group = zmapWindowContainerGetFeatures(block_parent) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(block_parent),
			  top, bottom) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(block_parent),
		      "x", 0.0,
		      "y", canvas_data->curr_y_offset,
		      NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_object_set_data(G_OBJECT(block_parent), "item_feature_type",
		    GINT_TO_POINTER(ITEM_BLOCK)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  g_object_set_data(G_OBJECT(block_parent), "item_feature_data", block) ;

  /* Add this block to our hash for going from the feature context to its on screen item. */
  status = zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
				  block->parent->unique_id, block->unique_id,
				  block_parent) ;



  /* Add a background colouring for the align block.
   * NOTE that the bottom of the box is the bottom of the block _not_ the span
   * of all features, the user will want to see the full extent of the alignment block. */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      for_bg_colour = &(canvas_data->colour_mblock_for) ;
      rev_bg_colour = &(canvas_data->colour_mblock_rev) ;
    }
  else
    {
      for_bg_colour = &(canvas_data->colour_qblock_for) ;
      rev_bg_colour = &(canvas_data->colour_qblock_rev) ;
    }

  forward_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
					    for_bg_colour, &(canvas_data->window->canvas_border)) ;
  canvas_data->curr_forward_group = zmapWindowContainerGetFeatures(forward_group) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(forward_group),
			  top, bottom) ;

  reverse_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
					    rev_bg_colour, &(canvas_data->window->canvas_border)) ;
  canvas_data->curr_reverse_group = zmapWindowContainerGetFeatures(reverse_group) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(reverse_group),
			  top, bottom) ;

  /* Start each set of cols in a block at 0 */
  canvas_data->curr_forward_offset = canvas_data->curr_reverse_offset = 0.0 ;


  /* Add _all_ the types cols for the block whether or not they have any features. */
  g_list_foreach(canvas_data->full_context->feature_set_names, createSetColumn, canvas_data) ;


  /* Now draw all features within each column. */
  g_datalist_foreach(&(block->feature_sets), ProcessFeatureSet, canvas_data) ;


  /* At this point we could hide all cols with no features, this would be reasonable where
   * the user had not given a feature set list..... */


  /* THIS SHOULD BE WRITTEN AS A GENERAL FUNCTION TO POSITION STUFF, IN FACT WE WILL NEED
   *  A FUNCTION THAT REPOSITIONS EVERYTHING....... */
  /* Now we must position all the columns as some columns will have been bumped. */
  canvas_data->curr_forward_offset = canvas_data->curr_reverse_offset = 0.0 ;
  g_list_foreach(canvas_data->full_context->feature_set_names, positionColumn, canvas_data) ;


  zmapWindowContainerSetBackgroundSize(forward_group, 0.0) ;
  zmapWindowContainerSetBackgroundSize(reverse_group, 0.0) ;

  /* Here we need to position the forward/reverse column groups by taking their size and
   * resetting their positions....should we have a reverse group if there are no reverse cols ? */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(reverse_group), &x1, &y1, &x2, &y2) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(forward_group),
		      "x", x2 + STRAND_SPACING,
		      NULL) ;

  zmapWindowContainerSetBackgroundSize(block_parent, 0.0) ;


  /* Sort lists of children in block and column groups by their position.... */
  zmapWindowCanvasGroupChildSort(canvas_data->curr_block_group) ;

  zmapWindowCanvasGroupChildSort(canvas_data->curr_forward_group) ;

  zmapWindowCanvasGroupChildSort(canvas_data->curr_reverse_group) ;

  return ;
}



/* Makes a column for each style in the list, the columns are populated with features by the 
 * ProcessFeatureSet() routine, at this stage the columns do not have any features and some
 * columns may end up not having any features at all. */
static void createSetColumn(gpointer data, gpointer user_data)
{
  GQuark feature_set_id = GPOINTER_TO_UINT(data) ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  ZMapFeatureBlock block = canvas_data->curr_block ;
  GQuark type_quark ;
  double x1, y1, x2, y2 ;
  FooCanvasItem *group ;
  double top, bottom ;
  gboolean status ;
  FooCanvasGroup *forward_col, *reverse_col ;
  GdkColor *for_bg_colour, *rev_bg_colour ;



  /* Add a background colouring for the column. */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      for_bg_colour = &(canvas_data->colour_mforward_col) ;
      rev_bg_colour = &(canvas_data->colour_mreverse_col) ;
    }
  else
    {
      for_bg_colour = &(canvas_data->colour_qforward_col) ;
      rev_bg_colour = &(canvas_data->colour_qreverse_col) ;
    }


  /* We need the background column object to span the entire bottom of the alignment block. */
  top = canvas_data->curr_block->block_to_sequence.t1 ;
  bottom = canvas_data->curr_block->block_to_sequence.t2 ;
  zmapWindowSeq2CanExtZero(&top, &bottom) ;


  forward_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_forward_group),
			     window,
			     feature_set_id,
			     ZMAPSTRAND_FORWARD,
			     canvas_data->curr_forward_offset,
			     top, bottom,
			     for_bg_colour) ;


  canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(feature_set_id, ZMAPSTRAND_FORWARD) ;

  status = zmapWindowFToIAddSet(window->context_to_item,
				canvas_data->curr_alignment->unique_id,
				canvas_data->curr_block->unique_id,
				feature_set_id,
				ZMAPSTRAND_FORWARD,
				forward_col) ;
  zMapAssert(status) ;


  canvas_data->curr_forward_col = zmapWindowContainerGetFeatures(forward_col) ;


  /* We find out how big the forward block is as this encloses all current columns,
   * then we draw the next column group to the right of this one. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_forward_col), &x1, &y1, &x2, &y2) ;
  canvas_data->curr_forward_offset = x2 + COLUMN_SPACING ;


  /* We are going to make all cols and then eventually hide some.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (canvas_data->curr_style->show_rev_strand)
    {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      reverse_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_reverse_group),
				 window,
				 feature_set_id,
				 ZMAPSTRAND_REVERSE,
				 canvas_data->curr_reverse_offset,
				 top, bottom,
				 rev_bg_colour) ;


      /* We can't set the "item_feature_data" as we don't have the feature set at this point.
       * This probably points to some muckiness in the code, problem is caused by us deciding
       * to display all columns whether they have features or not and so some columns may not
       * have feature sets. */

      canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(feature_set_id, ZMAPSTRAND_REVERSE) ;
      status = zmapWindowFToIAddSet(window->context_to_item,
				    canvas_data->curr_alignment->unique_id,
				    canvas_data->curr_block->unique_id,
				    feature_set_id,
				    ZMAPSTRAND_REVERSE,
				    reverse_col) ;
      zMapAssert(status) ;

      canvas_data->curr_reverse_col = zmapWindowContainerGetFeatures(reverse_col) ;


      /* We find out how big the reverse group is as this encloses all current reverse columns,
       * then we draw the next column group to the right of this one. */
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_reverse_col),
				 &x1, &y1, &x2, &y2) ;
      canvas_data->curr_reverse_offset = x2 + COLUMN_SPACING ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



/* Create an individual column group, this will have the feature items added to it. */
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    GQuark feature_set_name,
				    ZMapStrand strand,
				    double start, double top, double bot,
				    GdkColor *colour)
{
  FooCanvasGroup *group, *child_group ;
  FooCanvasItem *bounding_box ;
  double left, right ;
  double x1, x2, y1, y2 ;


  group = zmapWindowContainerCreate(parent_group, colour, &(window->canvas_border)) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(group),
		      "x", start,
		      "y", 0.0,
		      NULL) ;

  child_group = zmapWindowContainerGetFeatures(group) ;

  /* Make sure group covers whole span in y direction. */
  zmapWindowContainerSetBackgroundSize(group, zmapWindowExt(top, bot)) ;

  zmapWindowLongItemCheck(window, zmapWindowContainerGetBackground(group), top, bot) ;

  bounding_box = zmapWindowContainerGetBackground(group) ;

  /* Give columns a default width, this is not the way really, we should look for the style and
   * get the width somehow.... */
  foo_canvas_item_get_bounds(bounding_box, &x1, &y1, &x2, &y2) ;
  if (x2 == 0)
    foo_canvas_item_set(bounding_box,
			"x2", 5.0,
			NULL) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* NB I HAVE CHANGED THIS TO BE ON THE GROUP NOW....SO HAD BETTER IMPLEMENT IT.... */

  /* We should just do this on the group, not this bounding box but one step at a time....
   * and when we do we should swop to using ITEM_SET for the type but this will require
   * changes to the code that finds the feature item from its parent but button clicks etc. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* We can't set the "item_feature_data" as we don't have the feature set at this point.
   * This probably points to some muckiness in the code, problem is caused by us deciding
   * to display all columns whether they have features or not and so some columns may not
   * have feature sets. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_object_set_data(G_OBJECT(group), "item_feature_type",
		    GINT_TO_POINTER(ITEM_SET)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  g_object_set_data(G_OBJECT(group), "item_feature_strand",
		    GINT_TO_POINTER(strand)) ;
  g_object_set_data(G_OBJECT(group), "item_feature_style",
		    GINT_TO_POINTER(feature_set_name)) ;

  g_signal_connect(G_OBJECT(group), "event",
		   G_CALLBACK(columnBoundingBoxEventCB), (gpointer)window) ;

  return group ;
}



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  GQuark type_quark ;
  double x1, y1, x2, y2 ;
  GList *types ;
  ZMapFeatureBlock block = (ZMapFeatureBlock)feature_set->parent ;
  FooCanvasItem *group ;
  double top, bottom ;
  gboolean status ;
  FooCanvasGroup *forward_col, *reverse_col ;
  FooCanvasItem *bounding_box ;


  canvas_data->curr_set = feature_set ;


  /* Each column is known by its type/style name. */
  type_quark = feature_set->unique_id ;
  zMapAssert(type_quark == key_id) ;			    /* sanity check. */


  /* Get hold of the current column and set up canvas_data to do the right thing,
   * remember that there must always be a forward column but that there might
   * not be a reverse column ! */
  forward_col = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
							   feature_set, ZMAPSTRAND_FORWARD)) ;
  zMapAssert(forward_col) ;

  /* Now we have the feature set, make sure it is set for the column. */

  g_object_set_data(G_OBJECT(forward_col), "item_feature_data", feature_set) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_object_set_data(G_OBJECT(zmapWindowContainerGetBackground(forward_col)),
		    "item_feature_data", feature_set) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  canvas_data->curr_forward_col = zmapWindowContainerGetFeatures(forward_col) ;

  canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
							     ZMAPSTRAND_FORWARD) ;


  if ((reverse_col = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
								feature_set, ZMAPSTRAND_REVERSE))))
    {
      /* Now we have the feature set, make sure it is set for the column. */


      g_object_set_data(G_OBJECT(reverse_col), "item_feature_data", feature_set) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_object_set_data(G_OBJECT(zmapWindowContainerGetBackground(reverse_col)),
			"item_feature_data", feature_set) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      canvas_data->curr_reverse_col = zmapWindowContainerGetFeatures(reverse_col) ;

      canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
								 ZMAPSTRAND_REVERSE) ;
    }


  /* testing.... */
  canvas_data->bump_for = canvas_data->bump_rev = 0.0 ;


  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvas_data) ;



  /* TRY RESIZING BACKGROUND NOW..... */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(forward_col), &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(forward_col)),
			     &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetBackground(forward_col)),
			     &x1, &y1, &x2, &y2) ;

  zmapWindowMaximiseRectangle(zmapWindowContainerGetBackground(forward_col)) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(forward_col), &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(forward_col)),
			     &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetBackground(forward_col)),
			     &x1, &y1, &x2, &y2) ;


  
  if (reverse_col)
    zmapWindowMaximiseRectangle(zmapWindowContainerGetBackground(reverse_col)) ;



  return ;
}



/* Called for each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  ZMapFeatureTypeStyle style = feature->style ;
  ZMapCanvasData canvas_data  = (ZMapCanvasDataStruct*)user_data ;
  ZMapWindow window = canvas_data->window ;
  int i ;
  FooCanvasGroup *column_group ;
  GQuark column_id ;
  FooCanvasItem *top_feature_item = NULL ;
  double feature_top, feature_bottom, feature_offset ;
  GdkColor *background ;
  double x = 0.0, y = 0.0 ;				    /* for testing... */
  double start_x, end_x ;


  /* Users will often not want to see what is on the reverse strand. */
  if (feature->strand == ZMAPSTRAND_REVERSE && style->show_rev_strand == FALSE)
    return ;

  /* Set colours... */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	background = &(canvas_data->colour_mblock_rev) ;
      else
	background = &(canvas_data->colour_mblock_for) ;
    }
  else
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	background = &(canvas_data->colour_qblock_rev) ;
      else
	background = &(canvas_data->colour_qblock_for) ;
    }


  /* Retrieve the parent col. group/id. */
  if (feature->strand == ZMAPSTRAND_FORWARD || feature->strand == ZMAPSTRAND_NONE)
    {
      column_group = canvas_data->curr_forward_col ;
      column_id = canvas_data->curr_forward_col_id ;
    }
  else
    {
      column_group = canvas_data->curr_reverse_col ;
      column_id = canvas_data->curr_reverse_col_id ;
    }


  /* Start/end of feature within alignment block.
   * Feature position on screen is determined the relative offset of the features coordinates within
   * its align block added to the alignment block _query_ coords. You can't just use the
   * features own coordinates as these will its coordinates in its original sequence. */
  feature_offset = canvas_data->curr_block->block_to_sequence.q1 ;


  /* NOTE THAT ONCE WE SUPPORT HOMOLOGIES MORE FULLY WE WILL WANT SEPARATE BOXES
   * FOR BLOCKS WITHIN A SINGLE ALIGNMENT, this will mean a change to "item_feature_type"
   * in line with how its done for transcripts. */
  if (feature->type == ZMAPFEATURE_HOMOL && style->bump)
    {
      double bump ;

      if (feature->strand == ZMAPSTRAND_FORWARD)
	bump = canvas_data->bump_for ;
      else
	bump = canvas_data->bump_rev ;
	    

      start_x = bump ;
      end_x = bump + style->width ;

      if (feature->strand == ZMAPSTRAND_FORWARD)
	canvas_data->bump_for += 2.0 ;
      else
	canvas_data->bump_rev += 2.0 ;

    }
  else
    {
      start_x = 0.0 ;
      end_x = style->width ;
    }


  switch (feature->type)
    {
    case ZMAPFEATURE_BASIC: case ZMAPFEATURE_HOMOL:
    case ZMAPFEATURE_VARIATION:
    case ZMAPFEATURE_BOUNDARY: case ZMAPFEATURE_SEQUENCE:
      {
	top_feature_item = drawSimpleFeature(column_group, feature,
					     feature_offset,
					     start_x, feature->x1, end_x, feature->x2,
					     &style->outline,
					     &style->background,
					     window) ;
	break ;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	top_feature_item = drawTranscriptFeature(column_group, feature,
						 feature_offset,
						 start_x, feature->x1, end_x, feature->x2,
						 &style->outline,
						 &style->background,
						 background,
						 window) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	zmapWindowPrintGroup(FOO_CANVAS_GROUP(feature_group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	break ;
      }
    default:
      zMapLogFatal("Feature %s is of unknown type: %d\n", 
		   (char *)g_quark_to_string(feature->original_id), feature->type) ;
      break;
    }


  /* If we created an object then set up a hash that allows us to go unambiguously from
   * (feature, style) -> feature_item. */
  if (top_feature_item)
    {
      gboolean status ;

      status = zmapWindowFToIAddFeature(window->context_to_item,
					canvas_data->curr_alignment->unique_id,
					canvas_data->curr_block->unique_id, 
					column_id,
					feature->unique_id, top_feature_item) ;
      zMapAssert(status) ;
    }


  return ;
}




/* some drawing functions, may want these to be externally visible later... */

static FooCanvasItem *drawSimpleFeature(FooCanvasGroup *parent, ZMapFeature feature,
					double feature_offset,
					double x1, double y1, double x2, double y2,
					GdkColor *outline, GdkColor *background,
					ZMapWindow window)
{
  FooCanvasItem *feature_item ;

  zmapWindowSeq2CanOffset(&y1, &y2, feature_offset) ;

  feature_item = zMapDrawBox(FOO_CANVAS_ITEM(parent),
			     x1, y1, x2, y2,
			     outline, background) ;
  g_object_set_data(G_OBJECT(feature_item), "item_feature_type",
		    GINT_TO_POINTER(ITEM_FEATURE_SIMPLE)) ;
  g_object_set_data(G_OBJECT(feature_item), "item_feature_data", feature) ;

  g_signal_connect(GTK_OBJECT(feature_item), "event",
		   GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
  g_signal_connect(GTK_OBJECT(feature_item), "destroy",
		   GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;

  zmapWindowLongItemCheck(window, feature_item, y1, y2) ;

  return feature_item ;
}


	/* Note that for transcripts the boxes and lines are contained in a canvas group
	 * and that therefore their coords are relative to the start of the group which
	 * is the start of the transcript, i.e. feature->x1. */

static FooCanvasItem *drawTranscriptFeature(FooCanvasGroup *parent, ZMapFeature feature,
					    double feature_offset,
					    double x1, double feature_top,
					    double x2, double feature_bottom,
					    GdkColor *outline, GdkColor *background,
					    GdkColor *block_background,
					    ZMapWindow window)
{
  FooCanvasItem *feature_item ;
  FooCanvasPoints *points ;
  FooCanvasItem *feature_group ;
  int i ;
  double offset ;


  zmapWindowSeq2CanOffset(&feature_top, &feature_bottom, feature_offset) ;


  /* allocate a points array for drawing the intron lines, we need three points to draw the
   * two lines that make the familiar  /\  shape between exons. */
  points = foo_canvas_points_new(ZMAP_WINDOW_INTRON_POINTS) ;

  /* this group is the parent of the entire transcript. */
  feature_group = foo_canvas_item_new(FOO_CANVAS_GROUP(parent),
				      foo_canvas_group_get_type(),
				      "y", feature_top,
				      NULL) ;
      
  feature_item = FOO_CANVAS_ITEM(feature_group) ;

  /* we probably need to set this I think, it means we will go to the group when we
   * navigate or whatever.... */
  g_object_set_data(G_OBJECT(feature_item), "item_feature_type",
		    GINT_TO_POINTER(ITEM_FEATURE_PARENT)) ;
  g_object_set_data(G_OBJECT(feature_item), "item_feature_data", feature) ;



  /* Calculate total offset for for subparts of the feature. */
  offset = feature_top + feature_offset ;

  /* first we draw the introns, then the exons.  Introns will have an invisible
   * box around them to allow the user to click there and get a reaction.
   * It's a bit hacky but the bounding box has the same coords stored on it as
   * the intron...this needs tidying up because its error prone, better if they
   * had a surrounding group.....e.g. what happens when we free ?? */
  if (feature->feature.transcript.introns)
    {
      float line_width  = 1.5 ;

      for (i = 0 ; i < feature->feature.transcript.introns->len ; i++)  
	{
	  ZMapSpan intron_span ;
	  FooCanvasItem *intron_box, *intron_line ;
	  double left, right, top, middle, bottom ;
	  ZMapWindowItemFeature box_data, intron_data ;

	  intron_span = &g_array_index(feature->feature.transcript.introns, ZMapSpanStruct, i) ;

	  box_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  box_data->start = intron_span->x1 ;
	  box_data->end = intron_span->x2 ;

	  /* Need to remember that group coords start at zero, need to encapsulate this
	   * in some kind of macro/function that uses the group coords etc. to set
	   * positions. */
	  left = x2 / 2 ;
	  right = x2 ;

	  top = intron_span->x1 ;
	  bottom = intron_span->x2 ;
	  zmapWindowSeq2CanOffset(&top, &bottom, offset) ;


	  middle = top + ((bottom - top + 1) / 2) ;

	  intron_box = zMapDrawBox(feature_group,
				   x1, top,
				   right, bottom,
				   block_background,
				   block_background) ;
	  zmapWindowLongItemCheck(window, intron_box, top, bottom) ;

	  g_object_set_data(G_OBJECT(intron_box), "item_feature_type",
			    GINT_TO_POINTER(ITEM_FEATURE_BOUNDING_BOX)) ;
	  g_object_set_data(G_OBJECT(intron_box), "item_feature_data", feature) ;
	  g_object_set_data(G_OBJECT(intron_box), "item_subfeature_data",
			    box_data) ;

	  g_signal_connect(GTK_OBJECT(intron_box), "event",
			   GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
	  g_signal_connect(GTK_OBJECT(intron_box), "destroy",
			   GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;



	  intron_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
	  intron_data->start = intron_span->x1 ;
	  intron_data->end = intron_span->x2 ;

	  /* fill out the points */
	  points->coords[0] = left ;
	  points->coords[1] = top ;
	  points->coords[2] = right ;
	  points->coords[3] = middle ;
	  points->coords[4] = left ;
	  points->coords[5] = bottom ;

	  intron_line = zMapDrawPolyLine(FOO_CANVAS_GROUP(feature_group),
					 points,
					 background,
					 line_width) ;
	  zmapWindowLongItemCheck(window, intron_line, top, bottom) ;

	  g_object_set_data(G_OBJECT(intron_line), "item_feature_type",
			    GINT_TO_POINTER(ITEM_FEATURE_CHILD)) ;
	  g_object_set_data(G_OBJECT(intron_line), "item_feature_data", feature) ;
	  g_object_set_data(G_OBJECT(intron_line), "item_subfeature_data",
			    intron_data) ;

	  g_signal_connect(GTK_OBJECT(intron_line), "event",
			   GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
	  g_signal_connect(GTK_OBJECT(intron_line), "destroy",
			   GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;


	  /* Now we can get at either twinned item from the other item. */
	  box_data->twin_item = intron_line ;
	  intron_data->twin_item = intron_box ;
	}
    }


  /* We assume there are exons....I think we should check this actually.... */
  for (i = 0; i < feature->feature.transcript.exons->len; i++)
    {
      ZMapSpan exon_span ;
      FooCanvasItem *exon_box ;
      ZMapWindowItemFeature feature_data ;
      double top, bottom ;


      exon_span = &g_array_index(feature->feature.transcript.exons, ZMapSpanStruct, i);

      top = exon_span->x1 ;
      bottom = exon_span->x2 ;
      zmapWindowSeq2CanOffset(&top, &bottom, offset) ;


      feature_data = g_new0(ZMapWindowItemFeatureStruct, 1) ;
      feature_data->start = exon_span->x1 ;
      feature_data->end = exon_span->x2 ;

      exon_box = zMapDrawBox(feature_group, 
			     x1,
			     top,
			     x2,
			     bottom,
			     outline, 
			     background) ;
      zmapWindowLongItemCheck(window, exon_box, top, bottom) ;

      g_object_set_data(G_OBJECT(exon_box), "item_feature_type",
			GINT_TO_POINTER(ITEM_FEATURE_CHILD)) ;
      g_object_set_data(G_OBJECT(exon_box), "item_feature_data", feature) ;
      g_object_set_data(G_OBJECT(exon_box), "item_subfeature_data",
			feature_data) ;

      g_signal_connect(GTK_OBJECT(exon_box), "event",
		       GTK_SIGNAL_FUNC(canvasItemEventCB), window) ;
      g_signal_connect(GTK_OBJECT(exon_box), "destroy",
		       GTK_SIGNAL_FUNC(canvasItemDestroyCB), window) ;

    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintGroup(FOO_CANVAS_GROUP(feature_group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* tidy up. */
  foo_canvas_points_free(points) ;


  return feature_item ;
}



/* THIS IS WHERE WE SHOULD HIDE COLUMNS IF NECESSARY....EITHER BECAUSE OF MAG LIMIT OR BECAUSE
 * USER DOESN'T WANT IT SHOWN.... */
/* Positions columns within the forward/reverse strand groups. We have to do this retrospectively
 * because the width of some columns is determined at the time they are drawn and hence we don't
 * know their size until they've been drawn. */
static void positionColumn(gpointer data, gpointer user_data)
{
  GQuark feature_set_id = GPOINTER_TO_UINT(data) ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  double x1, y1, x2, y2 ;
  FooCanvasGroup *forward_col, *reverse_col ;
  GQuark id ;
  FooCanvasItem *bounding_box ;

  /* Get the forward column group. */
  forward_col = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							    canvas_data->curr_alignment->unique_id,
							    canvas_data->curr_block->unique_id,
							    feature_set_id,
							    ZMAPSTRAND_FORWARD,
							    0)) ;
  zMapAssert(forward_col) ;

  /* Set its x position. */
  foo_canvas_item_set(FOO_CANVAS_ITEM(forward_col),
		      "x", canvas_data->curr_forward_offset,
		      NULL) ;

  /* Calculate the offset of the next column from this ones width. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(forward_col), &x1, &y1, &x2, &y2) ;
  canvas_data->curr_forward_offset
    = canvas_data->curr_forward_offset + zmapWindowExt(x1, x2) + COLUMN_SPACING ;


  /* Make the background box of the column big enough to cover the whole column (which may have
   * been bumped. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  bounding_box = getBoundingBoxChild(forward_col) ;
  zMapAssert(bounding_box) ;
  zmapWindowExt2Zero(&x1, &x2) ;
  foo_canvas_item_set(bounding_box,
		      "x2", x2,
		      NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Repeat the same for the reverse strand column if there is one displayed. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (style->show_rev_strand)
    {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      reverse_col = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
								canvas_data->curr_alignment->unique_id,
								canvas_data->curr_block->unique_id,
								feature_set_id,
								ZMAPSTRAND_REVERSE,
								0)) ;
      zMapAssert(reverse_col) ;

      foo_canvas_item_set(FOO_CANVAS_ITEM(reverse_col),
			  "x", canvas_data->curr_reverse_offset,
			  NULL) ;

      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(reverse_col), &x1, &y1, &x2, &y2) ;
      canvas_data->curr_reverse_offset =
	canvas_data->curr_reverse_offset + zmapWindowExt(x1, x2) + COLUMN_SPACING ;

      /* we need to expand the background box to cover the whole column which may have been bumped. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      bounding_box = getBoundingBoxChild(reverse_col) ;
      zMapAssert(bounding_box) ;
      zmapWindowExt2Zero(&x1, &x2) ;
      foo_canvas_item_set(bounding_box,
			  "x2", x2,
			  NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* We should be doing this if required.... */

  /* Decide whether or not this column should be visible */
  /* thisType->min_mag is in bases per line, but window->zoom_factor is pixels per base */
  min_mag = (canvas_data->thisType->min_mag ? 
	     canvas_data->window->max_zoom/canvas_data->thisType->min_mag : 0.0);

  if (canvas_data->window->zoom_factor < min_mag)
    foo_canvas_item_hide(FOO_CANVAS_ITEM(group)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}




/* Callback for destroy of items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;			    /* Make sure any other callbacks also
							       get run. */
  ZMapWindow  window = (ZMapWindowStruct*)data ;
  ZMapWindowItemFeatureType item_feature_type ;

  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_type")) ;

  switch (item_feature_type)
    {
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
      {
	/* Nothing to do... */

	break ;
      }
    case ITEM_FEATURE_BOUNDING_BOX:
    case ITEM_FEATURE_CHILD:
      {
	ZMapWindowItemFeature item_data ;

	item_data = g_object_get_data(G_OBJECT(item), "item_subfeature_data") ;
	g_free(item_data) ;
	break ;
      }
    default:
      {
	zMapLogFatal("Coding error, unrecognised ZMapWindowItemFeatureType: %d", item_feature_type) ;
	break ;
      }
    }

  return event_handled ;
}



/* Callback for any events that happen on individual canvas items. */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow  window = (ZMapWindowStruct*)data ;
  ZMapFeature feature ;
  ZMapFeatureTypeStyle type ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
	ZMapWindowItemFeatureType item_feature_type ;
	FooCanvasItem *real_item ;

	item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							      "item_feature_type")) ;
	zMapAssert(item_feature_type == ITEM_FEATURE_SIMPLE
		   || item_feature_type == ITEM_FEATURE_CHILD
		   || item_feature_type == ITEM_FEATURE_BOUNDING_BOX) ;

	/* If its a bounding box then we don't want the that to influence highlighting.. */
	if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
	  real_item = zMapWindowFindFeatureItemByItem(window, item) ;
	else
	  real_item = item ;

	/* Retrieve the feature item info from the canvas item. */
	feature = (ZMapFeature)g_object_get_data(G_OBJECT(item), "item_feature_data");  
	zMapAssert(feature) ;

	/* Button 1 and 3 are handled, 2 is left for a general handler which could be
	 * the root handler. */
	if (but_event->button == 1 || but_event->button == 3)
	  {
	    /* Highlight the object the user clicked on and pass information about
	     * about it back to layer above. */
	    ZMapWindowSelectStruct select = {NULL} ;
	    double x = 0.0, y = 0.0 ;
	    
	    /* Pass information about the object clicked on back to the application. */
	    zMapWindowUpdateInfoPanel(window, feature, item);      

	    if (but_event->button == 3)
	      {
		/* Pop up an item menu. */
		makeItemMenu(but_event, window, real_item) ;
	      }

	    event_handled = TRUE ;
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


static void makeItemMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item)
{
  char *menu_title = "Item menu" ;
  ZMapWindowMenuItemStruct menu[] =
    {
      {"Show Feature List",  1, itemMenuCB, NULL},
      {"Edit Details"     ,  2, itemMenuCB, NULL},
      {"Column Bump Simple", 3, itemMenuCB, NULL},
      {"Column Bump Position"  , 5, itemMenuCB, NULL},
      {"Column UnBump"    ,  4, itemMenuCB, NULL},
      {"Search Window"    , 6, itemMenuCB, NULL},
      {NULL               , 0, NULL, NULL},
      {NULL               , 0, NULL, NULL},
      {NULL               , 0, NULL, NULL},
      {NULL               , 0, NULL, NULL},
      {NULL               , 0, NULL, NULL},
      {NULL               , 0, NULL, NULL}
    } ;
  int free_menu_slot = 6 ;
  ZMapWindowMenuItem menu_item ;
  ItemMenuCBData menu_data ;
  ZMapFeature feature ;


  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(item), "item_feature_data") ;
  zMapAssert(feature) ;
  
  if (feature->type == ZMAPFEATURE_HOMOL)
    {
      if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	{
	  menu[free_menu_slot].name = "Show multiple protein alignment in Blixem";
	  menu[free_menu_slot + 1].name = "Show multiple protein alignment for just this type of homology";
	}
      else
	{
	  menu[free_menu_slot].name = "Show multiple dna alignment";      
	  menu[free_menu_slot + 1].name = "Show multiple dna alignment for just this type of homology";      
	}

      /* We use id to distinguish between when user selects showing just one type of homology
       * and all homologies. */
      menu[free_menu_slot].callback_func = menu[free_menu_slot + 1].callback_func = blixemMenuCB ;
      menu[free_menu_slot].id = 1 ;
      menu[free_menu_slot + 1].id = 2 ;
    }

  menu_data = g_new0(ItemMenuCBDataStruct, 1) ;
  menu_data->window = window ;
  menu_data->item = item ;

  menu_item = menu ;
  while (menu_item->name != NULL)
    {
      menu_item->callback_data = menu_data ;
      menu_item++ ;
    }

  zMapWindowMakeMenu(menu_title, menu, button_event) ;

  return ;
}


/* call blixem either for a single type of homology or for all homologies. */
static void blixemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  gboolean single_homol_type = FALSE ;
  gboolean status ;

  switch (menu_item_id)
    {
    case 1:
      single_homol_type = FALSE ;
      break ;
    case 2:
      single_homol_type = TRUE ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }

  status = zmapWindowCallBlixem(menu_data->window, menu_data->item, single_homol_type) ;
  
  g_free(menu_data) ;

  return ;
}




static void itemMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  ZMapFeature feature ;

  /* Retrieve the feature item info from the canvas item. */
  feature = g_object_get_data(G_OBJECT(menu_data->item), "item_feature_data") ;
  zMapAssert(feature) ;

  switch (menu_item_id)
    {
    case 1:
      {
        ZMapFeature feature = NULL;
        GList *list = NULL;
        feature = g_object_get_data(G_OBJECT(menu_data->item), "item_feature_data");
        list    = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
                                                feature->parent->parent->parent->unique_id,
                                                feature->parent->parent->unique_id,
                                                feature->parent->unique_id,
                                                feature->strand,
                                                g_quark_from_string("*"));
        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->parent->original_id), 
                                   menu_data->item) ;
      }
      break ;
    case 2:
      {
        ZMapFeature feature = NULL;
        GList *list = NULL;
        feature = g_object_get_data(G_OBJECT(menu_data->item), "item_feature_data");
        list    = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
                                                feature->parent->parent->parent->unique_id,
                                                feature->parent->parent->unique_id,
                                                feature->parent->unique_id,
                                                feature->strand,
                                                feature->unique_id);
        zmapWindowEditorCreate(menu_data->window, list->data) ;
        
      }
      break ;
    case 3:
    case 4:
    case 5:
      {
	ZMapWindowBumpType bump_type ;

	if (menu_item_id == 3)
	  bump_type = ZMAP_WINDOW_BUMP_SIMPLE ;
	else if (menu_item_id == 4)
	  bump_type = ZMAP_WINDOW_BUMP_NONE ;
	else
	  bump_type = ZMAP_WINDOW_BUMP_POSITION ;
	zmapWindowColumnBump(getItemsColGroup(menu_data->item), bump_type) ;
	break ;
      }
    case 6:
      zmapWindowCreateSearchWindow(menu_data->window, (ZMapFeatureAny)feature) ;
      break ;

    default:
      zMapAssert("Coding error, unrecognised menu item number.") ; /* exits... */
      break ;
    }

  g_free(menu_data) ;

  return ;
}




static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)data ;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
	ZMapFeatureSet feature_set = NULL ;
	GQuark feature_set_id = 0 ;

	/* If a column is empty it will not have a feature set but it will have a style from which we
	 * can display the column id. */
	feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(item), "item_feature_data") ;
	feature_set_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item), "item_feature_style")) ;
	zMapAssert(feature_set || feature_set_id) ;

	/* Button 1 and 3 are handled, 2 is passed on to a general handler which could be
	 * the root handler. */
	switch (but_event->button)
	  {
	  case 1:
	    {
	      ZMapWindowSelectStruct select = {NULL} ;

	      if (feature_set)
		select.text = (char *)g_quark_to_string(feature_set->original_id) ;
	      else
		select.text = (char *)g_quark_to_string(feature_set_id) ;

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
	      if (feature_set)
		{
		  makeColumnMenu(but_event, window, item, feature_set) ;

		  event_handled = TRUE ;
		}
	      break ;
	    }
	  }
	break ;
      } 
    default:
      {
	/* By default we _don't_ handle events. */
	event_handled = FALSE ;

	break ;
      }
    }

  return event_handled ;
}


static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			   FooCanvasItem *item, ZMapFeatureSet feature_set)
{
  char *menu_title = "Column menu" ;
  ZMapWindowMenuItemStruct menu[] =
    {
      {"Show Feature List", 1, columnMenuCB, NULL},
      {"Column Bump Simple"     , 2, columnMenuCB, NULL},
      {"Column Bump Position"   , 4, columnMenuCB, NULL},
      {"Column UnBump"   , 3, columnMenuCB, NULL},
      {"Search Window"   , 5, columnMenuCB, NULL},
      {NULL,                0, NULL, NULL}
    } ;
  ZMapWindowMenuItem menu_item ;
  ColumnMenuCBData cbdata ;

  cbdata = g_new0(ColumnMenuCBDataStruct, 1) ;
  cbdata->button_event = button_event ;
  cbdata->window = window ;
  cbdata->item = item ;
  cbdata->feature_set = feature_set ;

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
        ZMapFeatureAny feature = NULL;
        GList *list = NULL;
        feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), "item_feature_data");
#warning FIX THIS STRAND INFO WHEN FILTERS ARE WRITTEN
        list    = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
                                                feature->parent->parent->unique_id,
                                                feature->parent->unique_id,
                                                feature->unique_id,
                                                ZMAPSTRAND_FORWARD,
                                                g_quark_from_string("*"));
        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->original_id), 
                                   NULL);
      }
      break ;
    case 2:
      zmapWindowColumnBump(FOO_CANVAS_GROUP(menu_data->item), ZMAP_WINDOW_BUMP_SIMPLE) ;
      break ;
    case 3:
      zmapWindowColumnBump(FOO_CANVAS_GROUP(menu_data->item), ZMAP_WINDOW_BUMP_NONE) ;
      break ;
    case 4:
      zmapWindowColumnBump(FOO_CANVAS_GROUP(menu_data->item), ZMAP_WINDOW_BUMP_POSITION) ;
      break ;
    case 5:
      zmapWindowCreateSearchWindow(menu_data->window, (ZMapFeatureAny)(menu_data->feature_set)) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}



static FooCanvasItem *getBoundingBoxChild(FooCanvasGroup *parent_group)
{
  FooCanvasItem *bounding_box_item = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;


  if (FOO_IS_CANVAS_GROUP(parent_group))
    {
      GList *list_item ;

      list_item = g_list_first(parent_group->item_list) ;

      while (list_item)
	{
	  FooCanvasItem *item ;
	  ZMapWindowItemFeatureType item_feature_type ;

	  item = FOO_CANVAS_ITEM(list_item->data) ;
	  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
								"item_feature_type")) ;

	  /* If its the bounding box then we've finished. */
	  if (item_feature_type == ITEM_FEATURE_BOUNDING_BOX)
	    {
	      bounding_box_item = item ;
	      break ;
	    }
	  else
	    list_item = g_list_next(list_item) ;
	}
    }

  return bounding_box_item ;
}


/* this needs to be a general function... */
static FooCanvasGroup *getItemsColGroup(FooCanvasItem *item)
{
  FooCanvasGroup *group = NULL ;
  ZMapWindowItemFeatureType item_feature_type ;


  item_feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(item),
							"item_feature_type")) ;

  switch (item_feature_type)
    {
    case ITEM_FEATURE_SIMPLE:
    case ITEM_FEATURE_PARENT:
      group = zmapWindowContainerGetParent(item->parent) ;
      break ;
    case ITEM_FEATURE_CHILD:
    case ITEM_FEATURE_BOUNDING_BOX:
      group = zmapWindowContainerGetParent(item->parent->parent) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  return group ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void setColours(ZMapCanvasData canvas_data)
{

  if (debug)
    {

      /* OK, ALL THIS NEEDS TO BE IN THE RESOURCE FILE.... */

      gdk_color_parse("red", &(canvas_data.colour_root)) ;
      gdk_color_parse("orange", &canvas_data.colour_alignment) ;
      gdk_color_parse("yellow", &canvas_data.colour_block) ;
      gdk_color_parse("brown", &canvas_data.colour_mblock_for) ;
      gdk_color_parse("wheat", &canvas_data.colour_mblock_rev) ;
      gdk_color_parse("blue", &canvas_data.colour_qblock_for) ;
      gdk_color_parse("light blue", &canvas_data.colour_qblock_rev) ;
      gdk_color_parse("light green", &(canvas_data.colour_forward_col)) ;
      gdk_color_parse("green", &(canvas_data.colour_reverse_col)) ;
    }
  else
    {
      gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &canvas_data.colour_mblock_for) ;
      gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &canvas_data.colour_mblock_rev) ;
      gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &canvas_data.colour_qblock_for) ;
      gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &canvas_data.colour_qblock_rev) ;
    }


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Read colour information from the configuration file, note that we read _only_ the first
 * window stanza found in the file, subsequent ones are not read. */
static void setColours(ZMapCanvasData canvas_data)
{
  ZMapConfig config ;
  ZMapConfigStanzaSet colour_list = NULL ;
  ZMapConfigStanza colour_stanza ;
  char *window_stanza_name = ZMAP_WINDOW_CONFIG ;
  ZMapConfigStanzaElementStruct colour_elements[] = {{"colour_root", ZMAPCONFIG_STRING, {ZMAP_WINDOW_BACKGROUND_COLOUR}},
						     {"colour_alignment", ZMAPCONFIG_STRING, {ZMAP_WINDOW_BACKGROUND_COLOUR}},
						     {"colour_block", ZMAPCONFIG_STRING, {ZMAP_WINDOW_BACKGROUND_COLOUR}},
						     {"colour_m_forward", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_F_BG}},
						     {"colour_m_reverse", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_R_BG}},
						     {"colour_q_forward", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_F_BG}},
						     {"colour_q_reverse", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_R_BG}},
						     {"colour_m_forwardcol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_F_BG}},
						     {"colour_m_reversecol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_R_BG}},
						     {"colour_q_forwardcol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_F_BG}},
						     {"colour_q_reversecol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_R_BG}},
						     {NULL, -1, {NULL}}} ;


  /* IN A HURRY...BADLY WRITTEN, COULD BE MORE COMPACT.... */

  if ((config = zMapConfigCreate()))
    {
      colour_stanza = zMapConfigMakeStanza(window_stanza_name, colour_elements) ;

      if (zMapConfigFindStanzas(config, colour_stanza, &colour_list))
	{
	  ZMapConfigStanza next_colour ;
	  
	  /* Get the first window stanza found, we will ignore any others. */
	  next_colour = zMapConfigGetNextStanza(colour_list, NULL) ;

	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_root"),
			  &(canvas_data->colour_root)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_alignment"),
			  &canvas_data->colour_alignment) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_block"),
			  &canvas_data->colour_block) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_forward"),
			  &canvas_data->colour_mblock_for) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_reverse"),
			  &canvas_data->colour_mblock_rev) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_forward"),
			  &canvas_data->colour_qblock_for) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_reverse"),
			  &canvas_data->colour_qblock_rev) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_forwardcol"),
			  &(canvas_data->colour_mforward_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_reversecol"),
			  &(canvas_data->colour_mreverse_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_forwardcol"),
			  &(canvas_data->colour_qforward_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_reversecol"),
			  &(canvas_data->colour_qreverse_col)) ;
	  
	  zMapConfigDeleteStanzaSet(colour_list) ;		    /* Not needed anymore. */
	}
      else
	{
	  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(canvas_data->colour_root)) ;
	  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &canvas_data->colour_alignment) ;
	  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &canvas_data->colour_block) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &canvas_data->colour_mblock_for) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &canvas_data->colour_mblock_rev) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &canvas_data->colour_qblock_for) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &canvas_data->colour_qblock_rev) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &(canvas_data->colour_mforward_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &(canvas_data->colour_mreverse_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &(canvas_data->colour_qforward_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &(canvas_data->colour_qreverse_col)) ;
	}


      zMapConfigDestroyStanza(colour_stanza) ;
      
      zMapConfigDestroy(config) ;
    }

  return ;
}





/****************** end of file ************************************/
