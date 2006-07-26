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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk and
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Jul 26 10:08 2006 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.143 2006-07-26 09:09:53 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>


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

  GQuark curr_forward_col_id ;
  GQuark curr_reverse_col_id ;


  /* THESE ARE REALLY NEEDED TO POSITION THE ALIGNMENTS FOR WHICH WE CURRENTLY HAVE NO
   * ORDERING/PLACEMENT MECHANISM.... */
  /* Records current positional information. */
  double curr_x_offset ;
  double curr_y_offset ;


  /* Records current canvas item groups, these are the direct parent groups of the display
   * types they contain, e.g. curr_root_group is the parent of the align */
  FooCanvasGroup *curr_root_group ;
  FooCanvasGroup *curr_align_group ;
  FooCanvasGroup *curr_block_group ;
  FooCanvasGroup *curr_forward_group ;
  FooCanvasGroup *curr_reverse_group ;

  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

} ZMapCanvasDataStruct, *ZMapCanvasData ;



/* Used to pass data to removeEmptyColumn function. */
typedef struct
{
  ZMapCanvasData canvas_data ;
  ZMapStrand strand ;
} RemoveEmptyColumnStruct, *RemoveEmptyColumn ;



typedef struct
{
  double offset ;				    /* I think we do need these... */
} PositionColumnStruct, *PositionColumn ;




static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext diff_context) ;
static void drawAlignments(GQuark key_id, gpointer data, gpointer user_data) ;
static void drawBlocks(gpointer data, gpointer user_data) ;
static void createSetColumn(gpointer data, gpointer user_data) ;
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    ZMapFeatureTypeStyle style,
				    ZMapStrand strand,
				    double width,
				    double top, double bot, GdkColor *colour) ;
static void ProcessFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data) ;
static void removeEmptyColumns(ZMapCanvasData canvas_data) ;
static void removeEmptyColumnCB(gpointer data, gpointer user_data) ;

static void positionColumns(ZMapCanvasData canvas_data) ;
static void positionColumnCB(gpointer data, gpointer user_data) ;

static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static void columnGroupDestroyCB(GtkObject *object, gpointer user_data) ;
static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item,
			   ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style) ;
static ZMapGUIMenuItem makeMenuColumnOps(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;

static void setColours(ZMapWindow window) ;







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
 * sequence
 *  start           0             0            >= 0    
 *    ^             ^             ^              ^
 *                                                       scr_start
 *                                                          ^
 * 
 *   root          col           col            all         scroll
 *   group        group(s)     bounding       features      region
 *                             box
 * 
 *                                                          v
 *                                                       scr_end
 *    v             v             v              v
 * sequence      max_coord     max_coord     <= max_coord
 *  end + 1
 * 
 * where  max_coord = ((seq_end - seq_start) + 1) + 1 (to cover the last base)
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
  FooCanvasGroup *root_group ;
  FooCanvasItem *fresh_focus_item = NULL, *tmp_item = NULL;
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT;
  gboolean debug_containers = FALSE, root_created = FALSE;
  double x, y, start, end ;
  double ix1, ix2, iy1, iy2;    /* initial root_group coords */
  double fx1, fx2, fy1, fy2;    /* final root_group coords   */

  zMapAssert(window && full_context && diff_context) ;

  /* Set up colours. */
  if (!window->done_colours)
    {
      setColours(window) ;
      window->done_colours = TRUE ;
    }


  /* Must be reset each time because context will change as features get merged in. */
  window->feature_context = full_context ;

  window->seq_start = full_context->sequence_to_parent.c1 ;
  window->seq_end   = full_context->sequence_to_parent.c2 ;
  window->seqLength = zmapWindowExt(window->seq_start, window->seq_end) ;

  zmapWindowZoomControlInitialise(window);		    /* Sets min/max/zf */

  window->min_coord = window->seq_start ;
  window->max_coord = window->seq_end ;
  zmapWindowSeq2CanExt(&(window->min_coord), &(window->max_coord)) ;

  h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  foo_canvas_set_pixels_per_unit_xy(window->canvas, 1.0, zMapWindowGetZoomFactor(window)) ;
  zmapWindowRulerCanvasSetPixelsPerUnit(window->ruler, 1.0, zMapWindowGetZoomFactor(window)) ;

  {
    double border_copy = 0.0;
    zmapWindowGetBorderSize(window, &border_copy);
    zmapWindowRulerCanvasSetLineHeight(window->ruler, border_copy * window->canvas->pixels_per_unit_y);
  }

  canvas_data.window = window;
  canvas_data.canvas = window->canvas;

  /* Get the current scroll region */
  ix1 = ix2 = iy1 = iy2 = 0.0;
  zmapWindowScrollRegionTool(window, &ix1, &iy1, &ix2, &iy2);

  if((tmp_item = zmapWindowFToIFindItemFull(window->context_to_item, 
                                            0, 0, 0, ZMAPSTRAND_NONE, 0)))
    {
      root_group = FOO_CANVAS_GROUP(tmp_item);
      root_created = FALSE;
    }
  else
    {
      /* Add a background to the root window, must be as long as entire sequence... */
      root_group = zmapWindowContainerCreate(foo_canvas_root(window->canvas),
                                             ZMAPCONTAINER_LEVEL_ROOT,
                                             window->config.align_spacing,
                                             &(window->colour_root), 
                                             &(window->canvas_border)) ;
      root_created = TRUE;
    }

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(root_group) ;
  g_object_set_data(G_OBJECT(root_group), ITEM_FEATURE_DATA, full_context) ;
  zmapWindowFToIAddRoot(window->context_to_item, root_group);
  window->feature_root_group = root_group ;

  start = window->seq_start ;
  end   = window->seq_end ;
  zmapWindowSeq2CanExtZero(&start, &end) ;
  zmapWindowLongItemCheck(window, zmapWindowContainerGetBackground(root_group),
			  start, end) ;

  /* Set root group to start where sequence starts... */
  x = canvas_data.curr_x_offset ;
  y = full_context->sequence_to_parent.c1 ;
  foo_canvas_item_w2i(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x, &y) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(root_group),
		      "y", y,
		      NULL) ;
  /* 
   *     Draw all the features, so much in so few lines...sigh...
   */
  canvas_data.curr_x_offset = 0.0;
  canvas_data.full_context = full_context ;
  drawZMap(&canvas_data, diff_context) ;

  /* Set the background object size now we have finished... */
  zmapWindowContainerSetBackgroundSize(root_group, 0.0) ;

  /* There may be a focus item if this routine is called as a result of splitting a window
   * or adding more features, make sure we scroll to the same point as we were
   * at in the previously. */
  if ((fresh_focus_item = zmapWindowItemGetHotFocusItem(window)))
    {
      zMapWindowScrollToItem(window, fresh_focus_item) ;
    }

  /* We've probably added width to the root group during drawZMap.  If
   * there was already a root group before this function we need to
   * honour the scroll region height (canvas height limit). Grow the
   * scroll region to the width of the root group */
  fx1 = fx2 = fy1 = fy2 = 0.0;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(root_group), &fx1, &fy1, &fx2, &fy2);
  /* We should be using the x coords of the root group and the y
   * coords of the previous scroll region Or the new root group
   * if the scroll region wasn't set by us!
   */
  zoom_status = zMapWindowGetZoomStatus(window);
  if(zoom_status == ZMAP_ZOOM_MID || 
     zoom_status == ZMAP_ZOOM_MAX)
    {
      fy1 = iy1;
      fy2 = iy2;
    }
  
  zmapWindowScrollRegionTool(window, &fx1, &fy1, &fx2, &fy2);

  if(debug_containers)
    zmapWindowContainerPrint(root_group) ;

  return ;
}

void zMapWindowToggleDNAProteinColumns(ZMapWindow window, 
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein,
                                       gboolean force_to, gboolean force)
{

  if(dna)
    zmapWindowToggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_DNA_NAME, align_id, block_id, force_to, force);
  if(protein)
    zmapWindowToggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_3FT_NAME, align_id, block_id, force_to, force);

  zmapWindowNewReposition(window);

  return ;
}
void zmapWindowToggleColumnInMultipleBlocks(ZMapWindow window, char *name,
                                            GQuark align_id, GQuark block_id, 
                                            gboolean force_to, gboolean force)
{
  GList *blocks = NULL;
  const char *wildcard = "*";
  GQuark feature_set_unique  = 0;

  feature_set_unique = zMapStyleCreateID(name);

  if(align_id == 0)
    align_id = g_quark_from_string(wildcard);
  if(block_id == 0)
    block_id = g_quark_from_string(wildcard);

  /* check we have the style... */
  if(zMapFindStyle(window->feature_context->styles, feature_set_unique))
    blocks = zmapWindowFToIFindItemSetFull(window->context_to_item, 
                                           align_id, block_id, 0, NULL, 0, NULL, NULL) ;
  else
    {
      zMapWarning("Column with name '%s' does not exist."
                  " Check there is a style of the same"
                  " name in your style source.", name);
    }


  /* Foreach of the blocks, toggle the display of the DNA */
  while(blocks)                 /* I cant bear to create ANOTHER struct! */
    {
      FooCanvasItem            *item = NULL;
      ZMapFeatureAny     feature_any = NULL;
      ZMapFeatureBlock feature_block = NULL;

      feature_any   = (ZMapFeatureAny)g_object_get_data(G_OBJECT(blocks->data), ITEM_FEATURE_DATA);
      feature_block = (ZMapFeatureBlock)feature_any;

      if((item = zmapWindowFToIFindItemFull(window->context_to_item,
                                            feature_block->parent->unique_id,
                                            feature_block->unique_id,
                                            feature_set_unique,
                                            ZMAPSTRAND_FORWARD,
                                            0)) != NULL)
        {
          if (force && force_to)
            zmapWindowColumnShow(FOO_CANVAS_GROUP(item)) ;
          else if (force && !force_to)
            zmapWindowColumnHide(FOO_CANVAS_GROUP(item)) ;
          else if (zmapWindowItemIsShown(FOO_CANVAS_ITEM(item)))
            zmapWindowColumnHide(FOO_CANVAS_GROUP(item)) ;
          else
            zmapWindowColumnShow(FOO_CANVAS_GROUP(item)) ;
        }

      blocks = blocks->next;
    }

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
  double x, y ;
  gboolean status ;
  FooCanvasGroup *align_parent ;


  canvas_data->curr_alignment = alignment ;

  /* THIS MUST GO.... */
  /* Always reset the aligns to start at y = 0. */
  canvas_data->curr_y_offset = 0.0 ;


  x = canvas_data->curr_x_offset ;
  y = canvas_data->full_context->sequence_to_parent.c1 ;
  foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

  x = canvas_data->curr_x_offset ;
  y = canvas_data->full_context->sequence_to_parent.c1 ;
  my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

  align_parent = zmapWindowContainerCreate(canvas_data->curr_root_group,
					   ZMAPCONTAINER_LEVEL_ALIGN,
					   window->config.block_spacing,
					   &(window->colour_alignment),
					   &(window->canvas_border)) ;
  canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(align_parent),
		      "x", x,
		      "y", y,
		      NULL) ;

  g_object_set_data(G_OBJECT(align_parent), ITEM_FEATURE_DATA, alignment) ;

  status = zmapWindowFToIAddAlign(window->context_to_item, key_id, align_parent) ;


  /* Do all the blocks within the alignment. */
  g_list_foreach(alignment->blocks, drawBlocks, canvas_data) ;


  /* We should have a standard routine to do this.... */

  /* We find out how wide the alignment ended up being after we drew all the blocks/columns
   * and then we draw the next alignment to the right of this one. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(align_parent), &x1, &y1, &x2, &y2) ;

  canvas_data->curr_x_offset = x2 + window->config.align_spacing ;
							    /* Must come before we reset x2 below. */


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
  ZMapWindow window = canvas_data->window ;
  gboolean status ;
  double x1, y1, x2, y2 ;
  GdkColor *for_bg_colour, *rev_bg_colour ;
  double top, bottom ;
  FooCanvasGroup *block_parent, *forward_group, *reverse_group ;
  double x, y ;

  /* not needed now...????? I think these can go now.... */
  top    = block->block_to_sequence.t1 ;
  bottom = block->block_to_sequence.t2 ;
  zmapWindowSeq2CanExtZero(&top, &bottom) ;


  canvas_data->curr_block = block ;

  /* Always set y offset to be top of current block. */
  canvas_data->curr_y_offset = block->block_to_sequence.t1 ;


  /* Add a group for the block and groups for the forward and reverse columns. */
  x = 0.0 ;
  y = block->block_to_sequence.t1 ;
  foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x, &y) ;


  x = 0.0 ;
  y = block->block_to_sequence.t1 ;
  my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x, &y) ;


  block_parent = zmapWindowContainerCreate(canvas_data->curr_align_group,
					   ZMAPCONTAINER_LEVEL_BLOCK,
					   window->config.strand_spacing,
					   &(window->colour_block),
					   &(canvas_data->window->canvas_border)) ;
  canvas_data->curr_block_group = zmapWindowContainerGetFeatures(block_parent) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(block_parent),
			  top, bottom) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(block_parent),
		      "y", y,
		      NULL) ;

  g_object_set_data(G_OBJECT(block_parent), ITEM_FEATURE_DATA, block) ;


  /* Add this block to our hash for going from the feature context to its on screen item. */
  status = zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
				  block->parent->unique_id, block->unique_id,
				  block_parent) ;



  /* Add a group each to hold forward and reverse strand columns. */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      for_bg_colour = &(window->colour_mblock_for) ;
      rev_bg_colour = &(window->colour_mblock_rev) ;
    }
  else
    {
      for_bg_colour = &(window->colour_qblock_for) ;
      rev_bg_colour = &(window->colour_qblock_rev) ;
    }

  forward_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
					    ZMAPCONTAINER_LEVEL_STRAND,
					    window->config.column_spacing,
					    for_bg_colour, &(canvas_data->window->canvas_border)) ;
  zmapWindowContainerSetStrand(forward_group, ZMAPSTRAND_FORWARD);
  canvas_data->curr_forward_group = zmapWindowContainerGetFeatures(forward_group) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(forward_group),
			  top, bottom) ;
  
  reverse_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
					    ZMAPCONTAINER_LEVEL_STRAND,
					    window->config.column_spacing,
					    rev_bg_colour, &(canvas_data->window->canvas_border)) ;
  zmapWindowContainerSetStrand(reverse_group, ZMAPSTRAND_REVERSE);
  canvas_data->curr_reverse_group = zmapWindowContainerGetFeatures(reverse_group) ;
  zmapWindowLongItemCheck(canvas_data->window, zmapWindowContainerGetBackground(reverse_group),
			  top, bottom) ;

  /* Add _all_ the types cols for the block, at this stage we don't know if they have features
   * and also the user may want them displayed even if empty.... */
  g_list_foreach(canvas_data->full_context->feature_set_names, createSetColumn, canvas_data) ;


  /* Now draw all features within each column, note that this operates on the feature context
   * so is called only for feature sets that contain features. */
  g_datalist_foreach(&(block->feature_sets), ProcessFeatureSet, canvas_data) ;
  

  /* Optionally remove all empty columns. */
  if (!(canvas_data->window->keep_empty_cols))
    {
      removeEmptyColumns(canvas_data) ;
    }


  /* THIS SHOULD BE WRITTEN AS A GENERAL FUNCTION TO POSITION STUFF, IN FACT WE WILL NEED
   *  A FUNCTION THAT REPOSITIONS EVERYTHING....... */
  /* Now we must position all the columns as some columns will have been bumped. */
  positionColumns(canvas_data) ;

  /* Now we've positioned all the columns we can set the backgrounds for the forward and
   * reverse strand groups and also set their positions within the block. */
  zmapWindowContainerSetBackgroundSizePlusBorder(forward_group, 0.0, window->config.column_spacing) ;
  zmapWindowContainerSetBackgroundSizePlusBorder(reverse_group, 0.0, window->config.column_spacing) ;

  /* Here we need to position the forward/reverse column groups by taking their size and
   * resetting their positions....should we have a reverse group if there are no reverse cols ? */

  x1 = 0.0 ;
  my_foo_canvas_item_goto(FOO_CANVAS_ITEM(reverse_group), &x1, NULL) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(reverse_group), &x1, &y1, &x2, &y2) ;

  x2 = x2 + window->config.strand_spacing ;
  my_foo_canvas_item_goto(FOO_CANVAS_ITEM(forward_group), &x2, NULL) ;


  /* Set the size of background for the block now everything is positioned. */
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
  /*  ZMapFeatureBlock block = canvas_data->curr_block ; */
  /* GQuark type_quark ; */
  /*  double x1, y1, x2, y2 ; */
  /* FooCanvasItem *group ;*/
  double top, bottom ;
  gboolean status ;
  FooCanvasGroup *forward_col, *reverse_col ;
  GdkColor *for_bg_colour, *rev_bg_colour ;
  ZMapFeatureContext context = window->feature_context ;
  ZMapFeatureTypeStyle style ;


  /* Look up the overall column style, its possible the style for the column may not have
   * have been found either because styles may be on a separate server or more often because
   * the style for a feature set isn't known until the features have been read. Then if we don't
   * find any features feature set won't have a style...kind of a catch 22. */
  if (!(style = zMapFindStyle(context->styles, feature_set_id)))
    {
      char *name = (char *)g_quark_to_string(feature_set_id) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Temporary...probably user will not want to see this in the end but for now its good for debugging. */
      zMapShowMsg(ZMAP_MSG_WARNING, "feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		  name, name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      zMapLogCritical("feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		      name, name) ;

      return ;
    }



  /* Add a background colouring for the column. */
  if (canvas_data->curr_alignment == canvas_data->full_context->master_align)
    {
      for_bg_colour = &(window->colour_mforward_col) ;
      rev_bg_colour = &(window->colour_mreverse_col) ;
    }
  else
    {
      for_bg_colour = &(window->colour_qforward_col) ;
      rev_bg_colour = &(window->colour_qreverse_col) ;
    }



  /* if this is not here the remove column routine crashes...feels uncomfortable, I
   * haven't tracked down why yet..... */
  if(!(g_datalist_id_get_data(&(canvas_data->curr_block->feature_sets), feature_set_id)))
    {
      ZMapFeatureSet fs ;

      fs = zMapFeatureSetIDCreate(feature_set_id, feature_set_id, style, NULL) ;
      zMapFeatureBlockAddFeatureSet(canvas_data->curr_block, fs) ;
    }



  /* We need the background column object to span the entire bottom of the alignment block. */
  top = canvas_data->curr_block->block_to_sequence.t1 ;
  bottom = canvas_data->curr_block->block_to_sequence.t2 ;
  zmapWindowSeq2CanExtZero(&top, &bottom) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    double x = canvas_data->curr_block->block_to_sequence.t1,
      y = canvas_data->curr_block->block_to_sequence.t2 ;

    foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;
    my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

  }
#endif


  /* this forward/reverse stuff needs factorising..... */
  forward_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_forward_group),
			     window,
			     style,
			     ZMAPSTRAND_FORWARD,
			     style->width,
			     top, bottom,
			     for_bg_colour) ;


  status = zmapWindowFToIAddSet(window->context_to_item,
				canvas_data->curr_alignment->unique_id,
				canvas_data->curr_block->unique_id,
				feature_set_id,
				ZMAPSTRAND_FORWARD,
				forward_col) ;
  zMapAssert(status) ;


  canvas_data->curr_forward_col = zmapWindowContainerGetFeatures(forward_col) ;
  canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(feature_set_id, ZMAPSTRAND_FORWARD) ;



  reverse_col = createColumn(FOO_CANVAS_GROUP(canvas_data->curr_reverse_group),
			     window,
			     style,
			     ZMAPSTRAND_REVERSE,
			     style->width,
			     top, bottom,
			     rev_bg_colour) ;

  status = zmapWindowFToIAddSet(window->context_to_item,
				canvas_data->curr_alignment->unique_id,
				canvas_data->curr_block->unique_id,
				feature_set_id,
				ZMAPSTRAND_REVERSE,
				reverse_col) ;
  zMapAssert(status) ;

  canvas_data->curr_reverse_col = zmapWindowContainerGetFeatures(reverse_col) ;
  canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(feature_set_id, ZMAPSTRAND_REVERSE) ;


  return ;
}


/* Create an individual column group, this will have the feature items added to it. */
static FooCanvasGroup *createColumn(FooCanvasGroup *parent_group,
				    ZMapWindow window,
				    ZMapFeatureTypeStyle style,
				    ZMapStrand strand,
				    double width,
				    double top, double bot,
				    GdkColor *colour)
{
  FooCanvasGroup *group ;
  FooCanvasItem *bounding_box ;
  double x1, x2, y1, y2 ;
  GHashTable *style_table ;


  group = zmapWindowContainerCreate(parent_group, ZMAPCONTAINER_LEVEL_FEATURESET,
				    window->config.feature_spacing,
				    colour, &(window->canvas_border)) ;

  /* By default we do not redraw our children which are the individual features, the canvas
   * should do this for us. */
  zmapWindowContainerSetChildRedrawRequired(group, FALSE) ;

  /* Make sure group covers whole span in y direction. */
  zmapWindowContainerSetBackgroundSize(group, bot) ;

  bounding_box = zmapWindowContainerGetBackground(group) ;
  zmapWindowLongItemCheck(window, bounding_box, top, bot) ;


  /* Ugh...what is this code.....??? should be done by a container call.... */
  foo_canvas_item_get_bounds(bounding_box, &x1, &y1, &x2, &y2) ;
  if (x2 == 0)
    foo_canvas_item_set(bounding_box,
			"x2", width,
			NULL) ;


  /* We can't set the ITEM_FEATURE_DATA as we don't have the feature set at this point.
   * This probably points to some muckiness in the code, problem is caused by us deciding
   * to display all columns whether they have features or not and so some columns may not
   * have feature sets. */
  g_object_set_data(G_OBJECT(group), ITEM_FEATURE_STRAND, GINT_TO_POINTER(strand)) ;

  /* Copy the columns style and attach to the column. */
  style = zMapFeatureStyleCopy(style) ;
  g_object_set_data(G_OBJECT(group), ITEM_FEATURE_STYLE, GINT_TO_POINTER(style)) ;

  /* Attach a table of styles, used to cache feature styles. */
  style_table = zmapWindowStyleTableCreate() ;
  g_object_set_data(G_OBJECT(group), ITEM_FEATURE_STYLETABLE, style_table) ;

  /* We now need this to free the style table cache.... */
  g_signal_connect(G_OBJECT(group), "destroy",
		   G_CALLBACK(columnGroupDestroyCB), (gpointer)window) ;


  g_signal_connect(G_OBJECT(group), "event",
		   G_CALLBACK(columnBoundingBoxEventCB), (gpointer)window) ;




  /* Some columns are hidden initially, perhaps because of magnification level or explicitly in
   * the style for the column. */
  if (zMapStyleGetHideInitial(style))
    zmapWindowColumnHide(group) ;
  else
    zmapWindowColumnSetMagState(window, group) ;


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
  FooCanvasGroup *forward_col, *reverse_col ;


  canvas_data->curr_set = feature_set ;

  /* Each column is known by its type/style name. */
  type_quark = feature_set->unique_id ;
  zMapAssert(type_quark == key_id) ;			    /* sanity check. */


  forward_col = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
							   feature_set, ZMAPSTRAND_FORWARD)) ;

  reverse_col = FOO_CANVAS_GROUP(zmapWindowFToIFindSetItem(window->context_to_item,
							   feature_set, ZMAPSTRAND_REVERSE)) ;

  /* If neither column can be found it means that there is a mismatch between the requested
   * feature_sets and the styles. This happens when dealing with gff sources read from
   * files for instance because the features must be retrieved using the style names but then
   * filtered by feature_set name, so if extra styles are provided this will result in the
   * mismatch.
   *  */
  if (!forward_col && !reverse_col)
    {
      /* There is a memory leak here in that we do not delete the feature set from the context. */
      char *name = (char *)g_quark_to_string(feature_set->original_id) ;

      /* Temporary...probably user will not want to see this in the end but for now its good for debugging. */
      zMapShowMsg(ZMAP_MSG_WARNING, 
		  "Feature set \"%s\" not displayed because although it was retrieved from the server,"
		  " it is not in the list of feature sets to be displayed."
		  " Please check the list of styles and feature sets in the ZMap configuration file.",
		  name) ;

      zMapLogCritical("Feature set \"%s\" not displayed because although it was retrieved from the server,"
		      " it is not in the list of feature sets to be displayed."
		      " Please check the list of styles and feature sets in the ZMap configuration file.",
		      name) ;

      return ;
    }



  /* Get hold of the current column and set up canvas_data to do the right thing,
   * remember that there must always be a forward column but that there might
   * not be a reverse column ! */
  if (forward_col)
    {
      /* Now we have the feature set, make sure it is set for the column. */
      g_object_set_data(G_OBJECT(forward_col), ITEM_FEATURE_DATA, feature_set) ;


      canvas_data->curr_forward_col = zmapWindowContainerGetFeatures(forward_col) ;
      
      canvas_data->curr_forward_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
								 ZMAPSTRAND_FORWARD) ;
    }

  if (reverse_col)
    {
      /* Now we have the feature set, make sure it is set for the column. */
      g_object_set_data(G_OBJECT(reverse_col), ITEM_FEATURE_DATA, feature_set) ;

      canvas_data->curr_reverse_col = zmapWindowContainerGetFeatures(reverse_col) ;

      canvas_data->curr_reverse_col_id = zmapWindowFToIMakeSetID(feature_set->unique_id,
								 ZMAPSTRAND_REVERSE) ;
    }


  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, canvas_data) ;


  /* TRY RESIZING BACKGROUND NOW.....get rid of debug info.... */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(forward_col), &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(forward_col)),
			     &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetBackground(forward_col)),
			     &x1, &y1, &x2, &y2) ;

  if (forward_col)
    zmapWindowContainerMaximiseBackground(forward_col) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(forward_col), &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetFeatures(forward_col)),
			     &x1, &y1, &x2, &y2) ;
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(zmapWindowContainerGetBackground(forward_col)),
			     &x1, &y1, &x2, &y2) ;
  

  if (reverse_col)
    zmapWindowContainerMaximiseBackground(reverse_col) ;

  return ;
}





/* Called to draw each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  ZMapCanvasData canvas_data  = (ZMapCanvasDataStruct*)user_data ;
  ZMapWindow window = canvas_data->window ;
  FooCanvasGroup *column_group ;
  ZMapStrand strand ;

  if ((strand = zmapWindowFeatureStrand(feature)) == ZMAPSTRAND_FORWARD)
    {
      column_group = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(canvas_data->curr_forward_col)) ;
    }
  else
    {
      column_group = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(canvas_data->curr_reverse_col)) ;
    }

  zmapWindowFeatureDraw(window, column_group, feature) ;

  return ;
}



/* Removes a column which has no features (the default action). */
static void removeEmptyColumns(ZMapCanvasData canvas_data)
{
  RemoveEmptyColumnStruct remove_data ;


  remove_data.canvas_data = canvas_data ;

  remove_data.strand = ZMAPSTRAND_FORWARD ;
  g_list_foreach(canvas_data->curr_forward_group->item_list, removeEmptyColumnCB, &remove_data) ;

  remove_data.strand = ZMAPSTRAND_REVERSE ;
  g_list_foreach(canvas_data->curr_reverse_group->item_list, removeEmptyColumnCB, &remove_data) ;

  return ;
}


/* Note how we must look at the items drawn, _not_ whether there are any features in the feature
 * set because the features may not be drawn (e.g. because they are on the opposite strand. */
static void removeEmptyColumnCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;

  /* There is some confusion here as we need to get the featureset id for the ftoi removal,
   * not the style id..... */
  if (!zmapWindowContainerHasFeatures(container))
    {
      RemoveEmptyColumn remove_data = (RemoveEmptyColumn)user_data ;
      ZMapCanvasData canvas_data = remove_data->canvas_data ;
      ZMapFeatureTypeStyle style ;


      /* Remove this item from the hash of features -> items */
      style = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_STYLE) ;
#ifdef RDS_DONT_INCLUDE
      status = zmapWindowFToIRemoveSet(canvas_data->window->context_to_item,
				       canvas_data->curr_alignment->unique_id,
				       canvas_data->curr_block->unique_id,
				       style->unique_id,
				       remove_data->strand) ;
      zMapAssert(status) ;
#endif
      /* Now remove from Long Items (if it was there in the first place... */
      zmapWindowLongItemRemove(&(canvas_data->window->long_items),
			       zmapWindowContainerGetBackground(container)) ;


      /* Now destroy the items themselves. */
      zmapWindowContainerDestroy(container) ;
    }

  return ;
}




/* this should all be container code....this is no good having it all spread everywhere.... */


/* Positions columns within the forward/reverse strand groups. We have to do this retrospectively
 * because the width of some columns is determined at the time they are drawn and hence we don't
 * know their size until they've been drawn.
 */
static void positionColumns(ZMapCanvasData canvas_data)
{
  PositionColumnStruct pos_data ;

  pos_data.offset = 0.0 ;
  g_list_foreach(canvas_data->curr_forward_group->item_list, positionColumnCB, &pos_data) ;

  /* We want the reverse strand columns as a mirror image of the forward strand so we position
   * them backwards. */
  pos_data.offset = 0.0 ;
  zMap_g_list_foreach_directional(canvas_data->curr_reverse_group->item_list_end, 
                                  positionColumnCB, &pos_data, ZMAP_GLIST_REVERSE) ;

  return ;
}

/* Note how we must look at the items drawn, _not_ whether there are any features in the feature
 * set because the features may not be drawn (e.g. because they are on the opposite strand. */
static void positionColumnCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  FooCanvasGroup *parent ;
  PositionColumn pos_data  = (PositionColumn)user_data ;
  double x1, y1, x2, y2 ;
  ZMapFeatureTypeStyle style ;
  double spacing ;

  parent = zmapWindowContainerGetSuperGroup(container) ;

  spacing = zmapWindowContainerGetSpacing(parent) ;


  style = zmapWindowContainerGetStyle(container) ;
  zMapAssert(style) ;

  /* Bump columns that need to be bumped. */
  if (style->overlap_mode != ZMAPOVERLAP_COMPLETE)
    zmapWindowColumnBump(FOO_CANVAS_ITEM(container), style->overlap_mode) ;


  /* Set its x position. */
  my_foo_canvas_item_goto(FOO_CANVAS_ITEM(container), &(pos_data->offset), NULL) ;

  /* Calculate the offset of the next column from this ones width. */
  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container), &x1, &y1, &x2, &y2) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pos_data->offset = pos_data->offset + zmapWindowExt(x1, x2) + COLUMN_SPACING ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  pos_data->offset = pos_data->offset + zmapWindowExt(x1, x2) + spacing ;

  return ;
}





/* 
 *                           Event handlers
 */

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
	ZMapFeatureTypeStyle style ;

	/* If a column is empty it will not have a feature set but it will have a style from which we
	 * can display the column id. */
	feature_set = (ZMapFeatureSet)g_object_get_data(G_OBJECT(item), ITEM_FEATURE_DATA) ;
	style = g_object_get_data(G_OBJECT(item), ITEM_FEATURE_STYLE) ;
	zMapAssert(feature_set || style) ;

	zmapHackAHighlightColumn(window, item) ;
        
	/* Button 1 and 3 are handled, 2 is passed on to a general handler which could be
	 * the root handler. */
	switch (but_event->button)
	  {
	  case 1:
	    {
	      ZMapWindowSelectStruct select = {NULL} ;
	      GQuark feature_set_id ;

	      if (feature_set)
		feature_set_id = feature_set->original_id ;
	      else
		feature_set_id = style->original_id ;

	      select.feature_desc.feature_set = (char *)g_quark_to_string(feature_set_id) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      /* probably we should get this displayed somehow but currently it doesn't. */

	      select.feature_desc.feature_description
		= zmapWindowFeatureSetDescription(style->original_id, style) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


              select.secondary_text = zmapWindowFeatureSetDescription(feature_set_id, style) ;

	      (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

	      g_free(select.secondary_text) ;

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
		  makeColumnMenu(but_event, window, item, feature_set, style) ;

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




/* 
 *                       Menu functions.
 */



/* Build the background menu for a column. */
static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			   FooCanvasItem *item,
			   ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style)
{
  static ZMapGUIMenuItemStruct separator[] =
    {
      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  char *menu_title ;
  GList *menu_sets = NULL ;
  ItemMenuCBData cbdata ;

  menu_title = zMapFeatureName((ZMapFeatureAny)feature_set) ;

  cbdata = g_new0(ItemMenuCBDataStruct, 1) ;
  cbdata->item_cb = FALSE ;
  cbdata->window = window ;
  cbdata->item = item ;
  cbdata->feature_set = feature_set ;

  /* Make up the menu. */
  menu_sets = g_list_append(menu_sets,
			    zmapWindowMakeMenuBump(NULL, NULL, cbdata,
						   zMapStyleGetOverlapMode(style))) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDumpOps(NULL, NULL, cbdata)) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, makeMenuColumnOps(NULL, NULL, cbdata)) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}




/* this is in the general menu and needs to be handled separately perhaps as the index is a global
 * one shared amongst all general menu functions... */
static ZMapGUIMenuItem makeMenuColumnOps(int *start_index_inout,
					 ZMapGUIMenuItemCallbackFunc callback_func,
					 gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "Show Feature List",      1, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Feature Search Window",  2, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                     0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}




static void columnMenuCB(int menu_item_id, gpointer callback_data)
{
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  switch (menu_item_id)
    {
    case 1:
      {
        ZMapFeatureAny feature ;
	ZMapStrand strand ;
        GList *list ;
	
        feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

        strand = zmapWindowContainerGetStrand(FOO_CANVAS_GROUP(menu_data->item));

	list = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     feature->unique_id,
					     zMapFeatureStrand2Str(strand),
					     g_quark_from_string("*"), NULL, NULL) ;
	
        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->original_id), 
                                   NULL) ;

	break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->window, menu_data->item) ;
      break ;
    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}





/* Read colour information from the configuration file, note that we read _only_ the first
 * window stanza found in the file, subsequent ones are not read. */
static void setColours(ZMapWindow window)
{
  ZMapConfig config ;
  ZMapConfigStanzaSet colour_list = NULL ;
  ZMapConfigStanza colour_stanza ;
  char *window_stanza_name = ZMAP_WINDOW_CONFIG ;
  ZMapConfigStanzaElementStruct colour_elements[]
    = {{"colour_root", ZMAPCONFIG_STRING, {ZMAP_WINDOW_BACKGROUND_COLOUR}},
       {"colour_alignment", ZMAPCONFIG_STRING, {ZMAP_WINDOW_BACKGROUND_COLOUR}},
       {"colour_block", ZMAPCONFIG_STRING, {ZMAP_WINDOW_STRAND_DIVIDE_COLOUR}},
       {"colour_m_forward", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_F_BG}},
       {"colour_m_reverse", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_R_BG}},
       {"colour_q_forward", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_F_BG}},
       {"colour_q_reverse", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_R_BG}},
       {"colour_m_forwardcol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_F_BG}},
       {"colour_m_reversecol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_MBLOCK_R_BG}},
       {"colour_q_forwardcol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_F_BG}},
       {"colour_q_reversecol", ZMAPCONFIG_STRING, {ZMAP_WINDOW_QBLOCK_R_BG}},
       {"colour_column_highlight", ZMAPCONFIG_STRING, {ZMAP_WINDOW_COLUMN_HIGHLIGHT}},
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
			  &(window->colour_root)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_alignment"),
			  &window->colour_alignment) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_block"),
			  &window->colour_block) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_forward"),
			  &window->colour_mblock_for) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_reverse"),
			  &window->colour_mblock_rev) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_forward"),
			  &window->colour_qblock_for) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_reverse"),
			  &window->colour_qblock_rev) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_forwardcol"),
			  &(window->colour_mforward_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_m_reversecol"),
			  &(window->colour_mreverse_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_forwardcol"),
			  &(window->colour_qforward_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_q_reversecol"),
			  &(window->colour_qreverse_col)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_column_highlight"),
			  &(window->colour_column_highlight)) ;
	  
	  zMapConfigDeleteStanzaSet(colour_list) ;		    /* Not needed anymore. */
	}
      else
	{
	  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(window->colour_root)) ;
	  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &window->colour_alignment) ;
	  gdk_color_parse(ZMAP_WINDOW_STRAND_DIVIDE_COLOUR, &window->colour_block) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &window->colour_mblock_for) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &window->colour_mblock_rev) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &window->colour_qblock_for) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &window->colour_qblock_rev) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &(window->colour_mforward_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &(window->colour_mreverse_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &(window->colour_qforward_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &(window->colour_qreverse_col)) ;
	  gdk_color_parse(ZMAP_WINDOW_COLUMN_HIGHLIGHT, &(window->colour_column_highlight)) ;
	}


      zMapConfigDestroyStanza(colour_stanza) ;
      
      zMapConfigDestroy(config) ;
    }

  return ;
}


/* Called to free resources we added to the column group canvas item. */
static void columnGroupDestroyCB(GtkObject *object, gpointer user_data)
{
  ZMapFeatureTypeStyle style ;
  GHashTable *style_table ;

  /* Get rid of this columns own style + its style table. */
  style = g_object_get_data(G_OBJECT(object), ITEM_FEATURE_STYLE) ;
  zMapAssert(style) ;
  zMapFeatureTypeDestroy(style) ;

  style_table = g_object_get_data(G_OBJECT(object), ITEM_FEATURE_STYLETABLE) ;
  zMapAssert(style_table) ;
  zmapWindowStyleTableDestroy(style_table) ;


  return ;
}




/****************** end of file ************************************/
