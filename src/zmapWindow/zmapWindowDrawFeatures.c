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
 * Last edited: Jul 19 09:46 2005 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.78 2005-07-19 09:35:33 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapUtils.h>
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

/* default spacings...will be tailorable one day ? */
#define ALIGN_SPACING    30.0
#define STRAND_SPACING   20.0
#define COLUMN_SPACING    5.0


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
  ZMapFeatureTypeStyle curr_style ;
  GQuark curr_forward_col_id ;
  GQuark curr_reverse_col_id ;

  /* Records current positional information. */
  double curr_x_offset ;
  double curr_y_offset ;
  double curr_forward_offset ;				    /* I think we do need these... */
  double curr_reverse_offset ;


  /* Records current canvas item groups. */
  FooCanvasGroup *curr_align_group ;
  FooCanvasGroup *curr_block_group ;
  FooCanvasGroup *curr_forward_group ;
  FooCanvasGroup *curr_reverse_group ;
  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

  GdkColor mblock_for ;
  GdkColor mblock_rev ;
  GdkColor qblock_for ;
  GdkColor qblock_rev ;

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
				    ZMapFeatureTypeStyle style, ZMapStrand strand,
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
static void blixemAllTypesMenuCB(int menu_item_id, gpointer callbackk_data);
static void blixemOneTypeMenuCB(int menu_item_id, gpointer callbackk_data);
static void editorMenuCB(int menu_item_id, gpointer callback_data);



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
/* THIS WILL HAVE TO CHANGE TO DRAWING ALIGNMENTS AND BLOCKS WITHIN THE ALIGNMENTS.... */
/* Draw features on the canvas, I think that this routine should _not_ get called unless 
 * there is actually data to draw.......
 * 
 * full_context contains all the features.
 * diff_context contains just the features from full_context that are newly added and therefore
 *              have not yet been drawn.
 * 
 * So NOTE that if no features have yet been drawn then  full_context == diff_context
 * 
 *  */
void zmapWindowDrawFeatures(ZMapWindow window,
			    ZMapFeatureContext full_context, ZMapFeatureContext diff_context)
{
  GtkAdjustment *h_adj;
  ZMapCanvasDataStruct canvas_data = {NULL} ;		    /* Rest of struct gets set to zero. */
  double x1, y1, x2, y2 ;

  zMapAssert(window && full_context && diff_context) ;

  /* Set up colours. */
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &canvas_data.mblock_for) ;
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &canvas_data.mblock_rev) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &canvas_data.qblock_for) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &canvas_data.qblock_rev) ;

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

  foo_canvas_get_scroll_region(window->canvas, NULL, &y1, NULL, &y2);
  zmapWindowDrawScaleBar(window, y1, y2);

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x1, &y1, &x2, &y2) ;
  window->alignment_start = x2 + COLUMN_SPACING ;

  canvas_data.curr_x_offset = x2 + COLUMN_SPACING ;

  /* 
   *     Draw all the features, so much in so few lines...sigh...
   */
  canvas_data.full_context = full_context ;
  drawZMap(&canvas_data, diff_context) ;

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
  double position ;
  gboolean status ;
  FooCanvasItem *background_item ;


  canvas_data->curr_alignment = alignment ;

  /* Always reset the aligns to start at y = 0. */
  canvas_data->curr_y_offset = 0.0 ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)),
		     "align group offset", canvas_data->curr_x_offset, canvas_data->curr_y_offset) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  canvas_data->curr_align_group = FOO_CANVAS_GROUP(foo_canvas_item_new(foo_canvas_root(window->canvas),
								       foo_canvas_group_get_type(),
								       "x", canvas_data->curr_x_offset,
								       "y", canvas_data->curr_y_offset,
								       NULL)) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_align_group), "item_feature_type",
		    GINT_TO_POINTER(ITEM_ALIGN)) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_align_group), "item_feature_data", alignment) ;


  /* add dummy colouring group.... */
  {
    double top, bottom ;
    GdkColor colour ;

    top = window->min_coord ;
    bottom = window->max_coord ;
    zmapWindowExt2Zero(&top, &bottom) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    zmapWindowPrintI2W(FOO_CANVAS_ITEM(canvas_data->curr_align_group),
		       "align fake background offset", 0.0, top) ;

    gdk_color_parse("light green", &colour) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    background_item = zMapDrawBox(FOO_CANVAS_ITEM(canvas_data->curr_align_group),
				  0.0,
				  top,
				  10, 
				  bottom,
				  &(window->canvas_background), &(window->canvas_background)) ;

    zmapWindowLongItemCheck(window, background_item, top, bottom) ;
  }

  status = zmapWindowFToIAddAlign(window->context_to_item, key_id, canvas_data->curr_align_group) ;

  /* Do all the blocks within the alignment. */
  g_list_foreach(alignment->blocks, drawBlocks, canvas_data) ;


  /* We find out how wide the alignment ended up being after we drew all the blocks/columns
   * and then we draw the next alignment to the right of this one. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x1, &y1, &x2, &y2) ;

  canvas_data->curr_x_offset = x2 + ALIGN_SPACING ;	    /* Must come before we reset x2 below. */

  zmapWindowExt2Zero(&x1, &x2) ;
  foo_canvas_item_set(background_item,
		      "x2", x2,
		      NULL) ;

  foo_canvas_item_lower_to_bottom(background_item) ;

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
  FooCanvasItem *background_item = NULL ;
  GdkColor *bg_colour ;
  double top, bottom ;

  canvas_data->curr_block = block ;

  /* Always set y offset to be top of current block. */
  canvas_data->curr_y_offset = block->block_to_sequence.t1 ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(canvas_data->curr_align_group),
		     "block offset", 0.0, canvas_data->curr_y_offset) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Add a group for the block and groups for the forward and reverse columns. */
  canvas_data->curr_block_group = FOO_CANVAS_GROUP(foo_canvas_item_new(canvas_data->curr_align_group,
								       foo_canvas_group_get_type(),
								       "x", 0.0,
								       "y", canvas_data->curr_y_offset,
								       NULL)) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_block_group), "item_feature_type",
		    GINT_TO_POINTER(ITEM_BLOCK)) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_block_group), "item_feature_data", block) ;


  /* Add a background colouring for the align block.
   * NOTE that the bottom of the box is the bottom of the block _not_ the span
   * of all features, the user will want to see the full extent of the alignment block. */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    bg_colour = &(canvas_data->mblock_for) ;
  else
    bg_colour = &(canvas_data->qblock_for) ;


  top = block->block_to_sequence.t1 ;
  bottom = block->block_to_sequence.t2 ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowExtent2Zero(&top, &bottom) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  zmapWindowSeq2CanExtZero(&top, &bottom) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(canvas_data->curr_block_group),
		     "block background offset", 0.0, 0.0) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  background_item = zMapDrawBox(FOO_CANVAS_ITEM(canvas_data->curr_block_group),
				0.0,
				top,
				10,			    /* place holder, proper width set below. */
				bottom,
				&(canvas_data->window->canvas_border),
				bg_colour) ;

  foo_canvas_item_lower_to_bottom(background_item) ;	    /* Put box in the background. */

  zmapWindowLongItemCheck(canvas_data->window, background_item, top, bottom) ;


  /* Add this block to our hash for going from the feature context to its on screen item. */
  status = zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
				  block->parent_alignment->unique_id, block->unique_id,
				  canvas_data->curr_block_group) ;

  canvas_data->curr_forward_group = FOO_CANVAS_GROUP(foo_canvas_item_new(canvas_data->curr_block_group,
									 foo_canvas_group_get_type(),
									 "x", 0.0, /* do we need to set position ? */
									 "y", 0.0,
									 NULL)) ;


  canvas_data->curr_reverse_group = FOO_CANVAS_GROUP(foo_canvas_item_new(canvas_data->curr_block_group,
									 foo_canvas_group_get_type(),
									 "x", 0.0, /* do we need to set position ? */
									 "y", 0.0,
									 NULL)) ;


  /* Start each set of cols in a block at 0 */
  canvas_data->curr_forward_offset = canvas_data->curr_reverse_offset = 0.0 ;


  /* Add _all_ the types cols for the block whether or not they have any features. */
  g_list_foreach(canvas_data->full_context->types, createSetColumn, canvas_data) ;


  /* Now draw all features within each column. */
  g_datalist_foreach(&(block->feature_sets), ProcessFeatureSet, canvas_data) ;



  /* THIS SHOULD BE WRITTEN AS A GENERAL FUNCTION TO POSITION STUFF, IN FACT WE WILL NEED
   *  A FUNCTION THAT REPOSITIONS EVERYTHING....... */
  /* Now we must position all the columns as some columns will have been bumped. */
  canvas_data->curr_forward_offset = canvas_data->curr_reverse_offset = 0.0 ;
  g_list_foreach(canvas_data->full_context->types, positionColumn, canvas_data) ;



  /* Here we need to position the forward/reverse column groups by taking their size and
   * resetting their positions....should we have a reverse group if there are no reverse cols ? */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_reverse_group), &x1, &y1, &x2, &y2) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(canvas_data->curr_block_group),
		     "column forward group", x2 + STRAND_SPACING, 0.0) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  foo_canvas_item_set(FOO_CANVAS_ITEM(canvas_data->curr_forward_group),
		      "x", x2 + STRAND_SPACING,
		      NULL) ;

  /* Expand it to cover the entire width of the block. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_block_group), &x1, &y1, &x2, &y2) ;
  zmapWindowExt2Zero(&x1, &x2) ;
  foo_canvas_item_set(background_item,
		      "x2", x2,
		      NULL) ;

  return ;
}



/* Makes a column for each style in the list, the columns are populated with features by the 
 * ProcessFeatureSet() routine, at this stage the columns do not have any features and some
 * columns may end up not having any features at all. */
static void createSetColumn(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  ZMapFeatureBlock block = canvas_data->curr_block ;
  GQuark type_quark ;
  double x1, y1, x2, y2 ;
  FooCanvasItem *group ;
  double top, bottom ;
  gboolean status ;
  GdkColor *forward_colour, *reverse_colour ;

  /* Set colours... */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      forward_colour = &(canvas_data->mblock_for) ;
      reverse_colour = &(canvas_data->mblock_rev) ;
    }
  else
    {
      forward_colour = &(canvas_data->qblock_for) ;
      reverse_colour = &(canvas_data->qblock_rev) ;
    }


  /* Each column is known by its type/style name. */
  type_quark = style->unique_id ;

  /* Cache the style. */
  canvas_data->curr_style = style ;

  /* We need the background column object to span the entire bottom of the alignment block. */
  top = canvas_data->curr_block->block_to_sequence.t1 ;
  bottom = canvas_data->curr_block->block_to_sequence.t2 ;
  zmapWindowSeq2CanExtZero(&top, &bottom) ;


  canvas_data->curr_forward_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_forward_group),
					       window,
					       style, ZMAPSTRAND_FORWARD,
					       canvas_data->curr_forward_offset,
					       top, bottom,
					       forward_colour) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_forward_col), "item_feature_type",
		    GINT_TO_POINTER(ITEM_SET)) ;
  g_object_set_data(G_OBJECT(canvas_data->curr_forward_col), "item_feature_strand",
		    GINT_TO_POINTER(ZMAPSTRAND_FORWARD)) ;
  /* We can't set the "item_feature_data" as we don't have the feature set at this point.
   * This probably points to some muckiness in the code, problem is caused by us deciding
   * to display all columns whether they have features or not and so some columns may not
   * have feature sets. */



  /* We need a special hash here for a forward group.... */
  canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(type_quark, ZMAPSTRAND_FORWARD) ;

  status = zmapWindowFToIAddSet(window->context_to_item,
				canvas_data->curr_alignment->unique_id,
				canvas_data->curr_block->unique_id,
				type_quark,
				ZMAPSTRAND_FORWARD,
				canvas_data->curr_forward_col) ;
  zMapAssert(status) ;


  /* We find out how big the forward block is as this encloses all current columns,
   * then we draw the next column group to the right of this one. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_forward_group), &x1, &y1, &x2, &y2) ;
  canvas_data->curr_forward_offset = x2 + COLUMN_SPACING ;

  if (canvas_data->curr_style->show_rev_strand)
    {
      canvas_data->curr_reverse_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_reverse_group),
						   window,
						   style, ZMAPSTRAND_REVERSE,
						   canvas_data->curr_reverse_offset,
						   top, bottom,
						   reverse_colour) ;
      g_object_set_data(G_OBJECT(canvas_data->curr_reverse_col), "item_feature_type",
			GINT_TO_POINTER(ITEM_SET)) ;
      g_object_set_data(G_OBJECT(canvas_data->curr_forward_col), "item_feature_strand",
			GINT_TO_POINTER(ZMAPSTRAND_REVERSE)) ;

      /* We can't set the "item_feature_data" as we don't have the feature set at this point.
       * This probably points to some muckiness in the code, problem is caused by us deciding
       * to display all columns whether they have features or not and so some columns may not
       * have feature sets. */


      /* We need a special hash here for a reverse group.... */
      canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(type_quark, ZMAPSTRAND_REVERSE) ;
      status = zmapWindowFToIAddSet(window->context_to_item,
				    canvas_data->curr_alignment->unique_id,
				    canvas_data->curr_block->unique_id,
				    type_quark,
				    ZMAPSTRAND_REVERSE,
				    canvas_data->curr_reverse_col) ;
      zMapAssert(status) ;

      /* We find out how big the reverse group is as this encloses all current reverse columns,
       * then we draw the next column group to the right of this one. */
      foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(canvas_data->curr_reverse_group),
				 &x1, &y1, &x2, &y2) ;
      canvas_data->curr_reverse_offset = x2 + COLUMN_SPACING ;
    }

  return ;
}



/* Create an individual column group, this will have the feature items added to it. */
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    ZMapFeatureTypeStyle style, ZMapStrand strand,
				    double start, double top, double bot,
				    GdkColor *colour)
{
  FooCanvasGroup *group ;
  FooCanvasItem *boundingBox ;
  double left, right ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(parent_group),
		     "column group", start, top) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  group = FOO_CANVAS_GROUP(foo_canvas_item_new(parent_group,
					       foo_canvas_group_get_type(),
					       "x", start,
					       "y", 0.0,
					       NULL)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapWindowPrintI2W(FOO_CANVAS_ITEM(parent_group), "column background", 0, top) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* should use draw box.... */
  /* To trap events anywhere on a column we need an item that covers the whole column (note
   * that you can't do this by setting an event handler on the column group, the group handler
   * will only be called if someone has clicked on an item in the group. The item is drawn
   * first and in white to match the canvas background to make an 'invisible' box. */

  left = 1 ;
  right = style->width ;
  zmapWindowExt2Zero(&left, &right) ;

  boundingBox = foo_canvas_item_new(group,
				    foo_canvas_rect_get_type(),
				    "x1", left,
				    "y1", top,
				    "x2", right,
				    "y2", bot,
				    "fill_color_gdk", colour,
				    NULL) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* We should just do this on the group, not this bounding box but one step at a time....
   * and when we do we should swop to using ITEM_SET for the type but this will require
   * changes to the code that finds the feature item from its parent but button clicks etc. */

  g_object_set_data(G_OBJECT(boundingBox), "item_feature_type",
		    GINT_TO_POINTER(ITEM_SET)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  g_object_set_data(G_OBJECT(boundingBox), "item_feature_type",
		    GINT_TO_POINTER(ITEM_FEATURE_BOUNDING_BOX)) ;

  g_object_set_data(G_OBJECT(boundingBox), "item_feature_strand",
		    GINT_TO_POINTER(strand)) ;
  g_object_set_data(G_OBJECT(boundingBox), "item_feature_style",
		    (gpointer)style) ;


  g_signal_connect(GTK_OBJECT(boundingBox), "event",
		   GTK_SIGNAL_FUNC(columnBoundingBoxEventCB), (gpointer)window) ;


  zmapWindowLongItemCheck(window, boundingBox, top, bot) ;

  return group ;
}



/* THIS IS WHERE WE SHOULD HIDE COLUMNS IF NECESSARY.... */
/* Positions columns within the forward/reverse strand groups. We have to do this retrospectively
 * because the width of some columns is determined at the time they are drawn and hence we don't
 * know their size until they've been drawn. */
static void positionColumn(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  double x1, y1, x2, y2 ;
  FooCanvasGroup *forward_col, *reverse_col ;
  GQuark id ;
  FooCanvasItem *bounding_box ;

  /* Get the forward column group. */

  id = zmapWindowFToIMakeSetID(style->unique_id, ZMAPSTRAND_FORWARD) ;

  forward_col = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							    canvas_data->curr_alignment->unique_id,
							    canvas_data->curr_block->unique_id,
							    style->unique_id,
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
  bounding_box = getBoundingBoxChild(forward_col) ;
  zMapAssert(bounding_box) ;
  zmapWindowExt2Zero(&x1, &x2) ;
  foo_canvas_item_set(bounding_box,
		      "x2", x2,
		      NULL) ;

  /* Repeat the same for the reverse strand column if there is one displayed. */
  if (style->show_rev_strand)
    {

      id = zmapWindowFToIMakeSetID(style->unique_id, ZMAPSTRAND_REVERSE) ;

      reverse_col = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
								canvas_data->curr_alignment->unique_id,
								canvas_data->curr_block->unique_id,
								style->unique_id,
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
      bounding_box = getBoundingBoxChild(reverse_col) ;
      zMapAssert(bounding_box) ;
      zmapWindowExt2Zero(&x1, &x2) ;
      foo_canvas_item_set(bounding_box,
			  "x2", x2,
			  NULL) ;
    }


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



/* Called for each feature set, it then calls a routine to draw each of its features.  */
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;
  ZMapCanvasData canvas_data  = (ZMapCanvasData)user_data ;
  ZMapWindow window = canvas_data->window ;
  GQuark type_quark ;
  double x1, y1, x2, y2 ;
  GList *types ;
  ZMapFeatureBlock block = feature_set->parent_block ;
  FooCanvasItem *group ;
  double top, bottom ;
  gboolean status ;
  FooCanvasItem *bounding_box ;


  canvas_data->curr_set = feature_set ;


  /* Each column is known by its type/style name. */
  type_quark = feature_set->unique_id ;
  zMapAssert(type_quark == key_id) ;			    /* sanity check. */


  /* Get hold of the style for this feature set. */
  types = canvas_data->curr_alignment->parent_context->types ;


  canvas_data->curr_style = zMapFindStyle(types, type_quark) ;
  zMapAssert(canvas_data->curr_style) ;


  /* Get hold of the current column and set up canvas_data to do the right thing,
   * remember that there must always be a forward column but that there might
   * not be a reverse column ! */
  canvas_data->curr_forward_col
    = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
						 feature_set, ZMAPSTRAND_FORWARD)) ;
  zMapAssert(canvas_data->curr_forward_col) ;

  /* Now we have the feature set, make sure it is set for the column. */
  g_object_set_data(G_OBJECT(canvas_data->curr_forward_col), "item_feature_data", feature_set) ;


  bounding_box = getBoundingBoxChild(canvas_data->curr_forward_col) ;
  zMapAssert(bounding_box) ;

  g_object_set_data(G_OBJECT(bounding_box), "item_feature_data", feature_set) ;



  canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
							     ZMAPSTRAND_FORWARD) ;

  if ((canvas_data->curr_reverse_col
       = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
						    feature_set, ZMAPSTRAND_REVERSE))))
    {
      /* Now we have the feature set, make sure it is set for the column. */
      g_object_set_data(G_OBJECT(canvas_data->curr_reverse_col), "item_feature_data", feature_set) ;

      bounding_box = getBoundingBoxChild(canvas_data->curr_reverse_col) ;
      zMapAssert(bounding_box) ;
      g_object_set_data(G_OBJECT(bounding_box), "item_feature_data", feature_set) ;

      canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
								 ZMAPSTRAND_REVERSE) ;
    }


  /* testing.... */
  canvas_data->bump_for = canvas_data->bump_rev = 0.0 ;


  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvas_data) ;


  return ;
}



/* Called for each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
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
  if (feature->strand == ZMAPSTRAND_REVERSE && canvas_data->curr_style->show_rev_strand == FALSE)
    return ;

  /* Set colours... */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	background = &(canvas_data->mblock_rev) ;
      else
	background = &(canvas_data->mblock_for) ;
    }
  else
    {
      if (feature->strand == ZMAPSTRAND_REVERSE)
	background = &(canvas_data->qblock_rev) ;
      else
	background = &(canvas_data->qblock_for) ;
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
  if (feature->type == ZMAPFEATURE_HOMOL && canvas_data->curr_style->bump)
    {
      double bump ;

      if (feature->strand == ZMAPSTRAND_FORWARD)
	bump = canvas_data->bump_for ;
      else
	bump = canvas_data->bump_rev ;
	    

      start_x = bump ;
      end_x = bump + canvas_data->curr_style->width ;

      if (feature->strand == ZMAPSTRAND_FORWARD)
	canvas_data->bump_for += 2.0 ;
      else
	canvas_data->bump_rev += 2.0 ;

    }
  else
    {
      start_x = 0.0 ;
      end_x = canvas_data->curr_style->width ;
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
					     &canvas_data->curr_style->outline,
					     &canvas_data->curr_style->background,
					     window) ;
	break ;
      }
    case ZMAPFEATURE_TRANSCRIPT:
      {
	top_feature_item = drawTranscriptFeature(column_group, feature,
						 feature_offset,
						 start_x, feature->x1, end_x, feature->x2,
						 &canvas_data->curr_style->outline,
						 &canvas_data->curr_style->background,
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
				      "x", 0.0,
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
    case ITEM_ALIGN:
    case ITEM_BLOCK:
    case ITEM_SET:
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	printf("ITEM event handler - CLICK\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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
	    select.text = g_strdup_printf("%s   %s   %d   %d   %s   %s", 
					  (char *)g_quark_to_string(feature->original_id),
					  zmapFeatureLookUpEnum(feature->strand, STRAND_ENUM),
					  feature->x1,
					  feature->x2,
					  zmapFeatureLookUpEnum(feature->type, TYPE_ENUM),
					  zMapStyleGetName(zMapFeatureGetStyle(feature))) ;
	    
	    select.item = real_item ;
	    
	    (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;
	    
	    g_free(select.text) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    /* I'm not sure what to do here, actually the callback above ends up highlighting
	     * so there is no need to here....perhaps we should leave it to the caller to
	     * do any highlighting ??? */
	    zMapWindowHighlightObject(window, real_item) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    if (but_event->button == 3)
	      {
		/* Pop up an item menu. */
		makeItemMenu(but_event, window, real_item) ;
	      }

	    event_handled = TRUE ;
	  }
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
      {"Show Feature List", 1, itemMenuCB, NULL},
      {"Edit Details"     , 2, editorMenuCB, NULL},
      {NULL               , 3, NULL, NULL},
      {NULL               , 4, NULL, NULL},
      {NULL               , 0, NULL, NULL}
    } ;
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
	  menu[2].name = "Show multiple protein alignment in Blixem";
	  menu[3].name = "Show multiple protein alignment for just this type of homology";
	}
      else
	{
	  menu[2].name = "Show multiple dna alignment";      
	  menu[3].name = "Show multiple dna alignment for just this type of homology";      
	}
      menu[2].callback_func = blixemAllTypesMenuCB ;
      menu[3].callback_func = blixemOneTypeMenuCB ;
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



static void editorMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  
  zmapWindowEditor(menu_data->window, menu_data->item) ;
  
  g_free(menu_data) ;

  return ;
}



/* call blixem for all types of homology */
static void blixemAllTypesMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;
  
  zmapWindowCallBlixem(menu_data->window, menu_data->item, FALSE) ;
  
  g_free(menu_data) ;

  return ;
}



/* call blixem for a single type of homology */
static void blixemOneTypeMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  zmapWindowCallBlixem(menu_data->window, menu_data->item, TRUE) ;
  
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
	/* display a list of the features in this column so the user
	 * can select one and have the display scroll to it. */
	zMapWindowCreateListWindow(menu_data->window, menu_data->item) ;
	break ;
      }
    case 2:
      {
	printf("Dummy!\n");
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




static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;
  ZMapWindow window = (ZMapWindow)data ;
  ZMapFeatureSet feature_set = NULL ;
  ZMapFeatureTypeStyle style = NULL ;
  GQuark column_id = 0 ;


  /* If a column is empty it will not have a feature set but it will have a style from which we
   * can display the column id. */
  feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(item), "item_feature_data") ;
  style = (ZMapFeatureTypeStyle)g_object_get_data(G_OBJECT(item), "item_feature_style") ;
  zMapAssert(feature_set || style) ;			    /* MUST have one or the other. */

  if (!feature_set)
    column_id = style->original_id ;


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
	      ZMapWindowSelectStruct select = {NULL} ;

	      if (feature_set)
		select.text = (char *)g_quark_to_string(feature_set->original_id) ;
	      else
		select.text = (char *)g_quark_to_string(column_id) ;

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
	/* By default we _don't_handle events. */
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
      {"Show Feature List", 1, columnMenuCB},
      {"Dummy",             2, columnMenuCB},
      {NULL,                0, NULL}
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
	FooCanvasItem *item ;

	/* display a list of the features in this column so the user
	 * can select one and have the display scroll to it. */
	zMapWindowCreateListWindow(menu_data->window, menu_data->item) ;

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







/****************** end of file ************************************/
