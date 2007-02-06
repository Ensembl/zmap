/*  File: zmapWindowDrawFeatures.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *
 * Description: Draws genomic features in the data display window.
 *              
 * Exported functions: 
 * HISTORY:
 * Last edited: Feb  6 17:23 2007 (rds)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.179 2007-02-06 17:38:04 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapConfig.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>
#include <zmapWindowItemFactory.h>

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


  GHashTable *feature_hash ;

} ZMapCanvasDataStruct, *ZMapCanvasData ;



/* Used to pass data to removeEmptyColumn function. */
typedef struct
{
  ZMapWindow window ;
  ZMapStrand strand ;
} RemoveEmptyColumnStruct, *RemoveEmptyColumn ;



/* Data for creating a feature set with features.... */
typedef struct
{
  ZMapWindow window ;
  GHashTable *feature_hash ;
  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;
  ZMapFrame frame ;
} CreateFeatureSetDataStruct, *CreateFeatureSetData ;


static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext context);

static FooCanvasGroup *createColumn(FooCanvasGroup      *parent_group,
				    ZMapWindow           window,
                                    ZMapFeatureSet       feature_set,
				    ZMapFeatureTypeStyle style,
				    ZMapStrand           strand, 
                                    ZMapFrame            frame,
				    double width, double top, double bot);
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data) ;

static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean containerDestroyCB(FooCanvasItem *item_in_hash, gpointer data) ;

static void removeEmptyColumnCB(gpointer data, gpointer user_data) ;

static void makeColumnMenu(GdkEventButton *button_event, ZMapWindow window, FooCanvasItem *item,
			   ZMapFeatureSet feature_set, ZMapFeatureTypeStyle style) ;
static ZMapGUIMenuItem makeMenuColumnOps(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;

static void setColours(ZMapWindow window) ;

static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;

static void removeList(gpointer data, gpointer user_data_unused) ;


extern GTimer *view_timer_G ;



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
  double x, y;
  double ix1, ix2, iy1, iy2;    /* initial root_group coords */
  double fx1, fx2, fy1, fy2;    /* final root_group coords   */



  zMapPrintTimer(NULL, "About to create canvas features") ;


  zMapAssert(window && full_context && diff_context) ;

  if(!window->item_factory)
    {
      window->item_factory = zmapWindowFToIFactoryOpen(window->context_to_item, window->long_items);
      zmapWindowFeatureFactoryInit(window);
    }

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


  /* Set the origin used for displaying coords....
   * The "+ 2" is because we have to go from sequence_start == 1 to sequence_start == -1
   * which is a shift of 2. */
  if (window->display_forward_coords)
    {
      if (window->revcomped_features)
	window->origin = window->seq_end + 2 ;
      else
	window->origin = 1 ;

      zmapWindowRulerCanvasSetOrigin(window->ruler, window->origin) ;
    }

  zmapWindowZoomControlInitialise(window);		    /* Sets min/max/zf */


  /* HOPE THIS IS THE RIGHT PLACE TO SET ZOOM FOR LONG_ITEMS... */
  zmapWindowLongItemSetMaxZoom(window->long_items, zMapWindowGetZoomMax(window)) ;


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
                                            0, 0, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
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
                                             &(window->canvas_border),
					     window->long_items) ;

      g_signal_connect(G_OBJECT(root_group), "destroy", G_CALLBACK(containerDestroyCB), window) ;

      root_created = TRUE;
    }

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(root_group) ;

  zmapWindowContainerSetData(root_group, ITEM_FEATURE_DATA, full_context) ; /* Always reset this
                                                                               as context changes
                                                                               with new features.*/

  zmapWindowFToIAddRoot(window->context_to_item, root_group);
  window->feature_root_group = root_group ;

#ifdef RDS_DONT_INCLUDE
  start = window->seq_start ;
  end   = window->seq_end ;
  zmapWindowSeq2CanExtZero(&start, &end) ;
  zmapWindowLongItemCheck(window->long_items, zmapWindowContainerGetBackground(root_group),
			  start, end) ;
#endif /* RDS_DONT_INCLUDE */

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

  drawZMap(&canvas_data, diff_context);
  
  zmapWindowColOrderColumns(window);

  zmapWindowNewReposition(window);


  /* There may be a focus item if this routine is called as a result of splitting a window
   * or adding more features, make sure we scroll to the same point as we were
   * at in the previously. */
  if ((fresh_focus_item = zmapWindowFocusGetHotItem(window->focus)))
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


  zMapPrintTimer(NULL, "Finished creating canvas features") ;

  /* cursor should have been set on by anyone calling us. */
  zMapWindowBusy(window, FALSE) ;

  return ;
}


/* Makes a column for each style in the list, the columns are populated with features by the 
 * ProcessFeatureSet() routine, at this stage the columns do not have any features and some
 * columns may end up not having any features at all. */

/* N.B. Test the return value as not all columns can be created (meta
   columns, those without styles). */
gboolean zmapWindowCreateSetColumns(ZMapWindow window, 
                                    FooCanvasGroup *forward_strand_group, 
                                    FooCanvasGroup *reverse_strand_group,
                                    ZMapFeatureBlock block, 
                                    ZMapFeatureSet feature_set,
                                    ZMapFrame frame,
                                    FooCanvasGroup **forward_col_out, 
                                    FooCanvasGroup **reverse_col_out)
{
  ZMapFeatureContext context = window->feature_context ;
  ZMapFeatureTypeStyle style ;
  double top, bottom ;
  gboolean created = TRUE;

  /* Look up the overall column style, its possible the style for the column may not have
   * have been found either because styles may be on a separate server or more often because
   * the style for a feature set isn't known until the features have been read. Then if we don't
   * find any features feature set won't have a style...kind of a catch 22. */
  style = zMapFindStyle(context->styles, feature_set->unique_id) ;

  if (!style)
    {
      char *name = (char *)g_quark_to_string(feature_set->unique_id) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Temporary...probably user will not want to see this in the end but for now its good for debugging. */
      zMapShowMsg(ZMAP_MSG_WARNING, "feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		  name, name) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      zMapLogCritical("feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		      name, name) ;
      created = FALSE;
    }
  else if (style->opts.hidden_always)
    {
      /* some styles should not be shown, e.g. they may be "meta" styles like "3 Frame". */
      created = FALSE;
    }

  if(!(forward_col_out && reverse_col_out))
    created = FALSE;

  if(created)
    {
      FooCanvasItem *set_group_item;
      /* We need the background column object to span the entire bottom of the alignment block. */
      top = block->block_to_sequence.t1 ;
      bottom = block->block_to_sequence.t2 ;
      zmapWindowSeq2CanExtZero(&top, &bottom) ;
      
      if (forward_strand_group)
        {
          if((set_group_item = zmapWindowFToIFindItemFull(window->context_to_item,
                                                          block->parent->unique_id,
                                                          block->unique_id,
                                                          feature_set->unique_id, 
                                                          ZMAPSTRAND_FORWARD,
                                                          frame, 0)))
            {
              zMapAssert(set_group_item && FOO_IS_CANVAS_GROUP(set_group_item));
              *forward_col_out = FOO_CANVAS_GROUP(set_group_item);
            }
          else
            *forward_col_out = createColumn(FOO_CANVAS_GROUP(forward_strand_group),
                                            window,
                                            feature_set,
                                            style,
                                            ZMAPSTRAND_FORWARD, 
                                            frame,
                                            style->width,
                                            top, bottom) ;
        }
      
      if (reverse_strand_group)
        {
          if((set_group_item = zmapWindowFToIFindItemFull(window->context_to_item,
                                                          block->parent->unique_id,
                                                          block->unique_id,
                                                          feature_set->unique_id, 
                                                          ZMAPSTRAND_REVERSE,
                                                          frame, 0)))
            {
              zMapAssert(set_group_item && FOO_IS_CANVAS_GROUP(set_group_item));
              *reverse_col_out = FOO_CANVAS_GROUP(set_group_item);
            }
          else
            *reverse_col_out = createColumn(FOO_CANVAS_GROUP(reverse_strand_group),
                                            window,
                                            feature_set,
                                            style,
                                            ZMAPSTRAND_REVERSE, 
                                            frame,
                                            style->width,
                                            top, bottom) ;
        }
    }

  return created;
}


/* Called for each feature set, it then calls a routine to draw each of its features.  */
/* The feature set will be filtered on supplied frame by ProcessFeature.  
 * ProcessFeature splits the feature sets features into the separate strands.
 */
void zmapWindowDrawFeatureSet(ZMapWindow window, 
                              ZMapFeatureSet feature_set,
                              FooCanvasGroup *forward_col_wcp, 
                              FooCanvasGroup *reverse_col_wcp,
                              ZMapFrame frame)
{
  CreateFeatureSetDataStruct featureset_data = {NULL} ;
  gboolean bump_required = TRUE;
  featureset_data.window = window ;

  if (forward_col_wcp)
    featureset_data.curr_forward_col = zmapWindowContainerGetFeatures(forward_col_wcp) ;

  if (reverse_col_wcp)
    featureset_data.curr_reverse_col = zmapWindowContainerGetFeatures(reverse_col_wcp) ;
  
  featureset_data.frame = frame ;

  /* Now draw all the features in the column. */
  g_datalist_foreach(&(feature_set->features), ProcessFeature, &featureset_data) ;

  /* We should be bumping columns here if required... */
  if(bump_required)
    {
      ZMapStyleOverlapMode overlap_mode ;
      
      if ((overlap_mode = zMapStyleGetOverlapMode(feature_set->style)) != ZMAPOVERLAP_COMPLETE)
        {
          if (forward_col_wcp)
            zmapWindowColumnBump(FOO_CANVAS_ITEM(forward_col_wcp), overlap_mode) ;
          
          if (reverse_col_wcp)
            zmapWindowColumnBump(FOO_CANVAS_ITEM(reverse_col_wcp), overlap_mode) ;
        }
    }	    

  return ;
}


/* Removes a column which has no features (the default action). */
void zmapWindowRemoveEmptyColumns(ZMapWindow window,
				  FooCanvasGroup *forward_group, FooCanvasGroup *reverse_group)
{
  RemoveEmptyColumnStruct remove_data ;

  zMapAssert(forward_group || reverse_group) ;

  remove_data.window = window ;

  if (forward_group)
    {
      remove_data.strand = ZMAPSTRAND_FORWARD ;
      g_list_foreach(forward_group->item_list, removeEmptyColumnCB, &remove_data) ;
    }

  if (reverse_group)
    {
      remove_data.strand = ZMAPSTRAND_REVERSE ;
      g_list_foreach(reverse_group->item_list, removeEmptyColumnCB, &remove_data) ;
    }

  return ;
}

gboolean zmapWindowRemoveIfEmptyCol(FooCanvasGroup **col_group)
{
  gboolean removed = FALSE ;

  if (!zmapWindowContainerHasFeatures(*col_group))
    {
      zmapWindowContainerDestroy(*col_group) ;
      *col_group = NULL;
      removed = TRUE ;
    }

  return removed ;
}




void zMapWindowToggleDNAProteinColumns(ZMapWindow window, 
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein,
                                       gboolean force_to, gboolean force)
{

  zMapWindowBusy(window, TRUE) ;

  if (dna)
    zmapWindowToggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_DNA_NAME,
					   align_id, block_id, force_to, force);

  if (protein)
    zmapWindowToggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_3FT_NAME,
					   align_id, block_id, force_to, force);

  zmapWindowNewReposition(window) ;

  zMapWindowBusy(window, FALSE) ;

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
  if (zMapFindStyle(window->feature_context->styles, feature_set_unique))
    blocks = zmapWindowFToIFindItemSetFull(window->context_to_item, 
                                           align_id, block_id, 0,
					   NULL, NULL, 0, NULL, NULL) ;
  else
    {
      zMapWarning("Column with name '%s' does not exist."
                  " Check there is a style of the same"
                  " name in your style source.", name);
    }


  /* Foreach of the blocks, toggle the display of the DNA */
  while (blocks)                 /* I cant bear to create ANOTHER struct! */
    {
      FooCanvasItem            *item = NULL;
      ZMapFeatureAny     feature_any = NULL;
      ZMapFeatureBlock feature_block = NULL;

      feature_any   = (ZMapFeatureAny)g_object_get_data(G_OBJECT(blocks->data), ITEM_FEATURE_DATA);
      feature_block = (ZMapFeatureBlock)feature_any;

      if ((item = zmapWindowFToIFindItemFull(window->context_to_item,
					     feature_block->parent->unique_id,
					     feature_block->unique_id,
					     feature_set_unique,
					     ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE,
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






/*
 *                          internal functions
 */



static gboolean strandBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE;

  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *but_event = (GdkEventButton *)event ;
        if(but_event->button == 1)
          {
            
          }
      }
      break;
    default:
      break;
    }

  return event_handled;
}





/* 
 * Create a Single Column.  
 * This column is a Container, has ITEM_FEATURE_SET_DATA and ITEM_FEATURE_DATA set,
 * is created for one of the 6 possibilities of STRAND and FRAME.  
 * ITEM_FEATURE_SET_DATA will reflect this.
 * The Container is added to the FToI Hash.
 */
static FooCanvasGroup *createColumn(FooCanvasGroup      *parent_group,
				    ZMapWindow           window,
                                    ZMapFeatureSet       feature_set,
				    ZMapFeatureTypeStyle style,
				    ZMapStrand           strand, 
                                    ZMapFrame            frame,
				    double width, double top, double bot)
{
  FooCanvasGroup *group = NULL ;
  GdkColor *colour ;
  FooCanvasItem *bounding_box ;
  ZMapFeatureAlignment align ;
  ZMapFeatureBlock     block;
  gboolean status ;
  ZMapWindowItemFeatureSetData set_data ;

  block = (ZMapFeatureBlock)(feature_set->parent);
  zMapAssert(block);
  align = (ZMapFeatureAlignment)(block->parent) ;
  zMapAssert(align);

  /* Add a background colouring for the column. */
  if (frame != ZMAPFRAME_NONE)
    {
      /* NOTE that if frame is not ZMAPFRAME_NONE it means we are drawing in 3 frame mode, a bit
       * indirect but otherwise we need to pass more state in about what we are drawing. */
      switch (frame)
	{
	case ZMAPFRAME_0:
	  colour = &(window->colour_frame_0) ;
	  break ;
	case ZMAPFRAME_1:
	  colour = &(window->colour_frame_1) ;
	  break ;
	case ZMAPFRAME_2:
	  colour = &(window->colour_frame_2) ;
	  break ;
	default:
	  zMapAssertNotReached() ;
	}
    }
  else
    {
      if (align == window->feature_context->master_align)
	{
	  if (strand == ZMAPSTRAND_FORWARD)
	    colour = &(window->colour_mforward_col) ;
	  else
	    colour = &(window->colour_mreverse_col) ;
	}
      else
	{
	  if (strand == ZMAPSTRAND_FORWARD)
	    colour = &(window->colour_qforward_col) ;
	  else
	    colour = &(window->colour_qreverse_col) ;
	}
    }


  group = zmapWindowContainerCreate(parent_group, ZMAPCONTAINER_LEVEL_FEATURESET,
				    window->config.feature_spacing,
				    colour, &(window->canvas_border),
				    window->long_items) ;

  /* reverse the column ordering on the reverse strand */
  if(strand == ZMAPSTRAND_REVERSE)
    foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(group));

  /* By default we do not redraw our children which are the individual features, the canvas
   * should do this for us. */
  zmapWindowContainerSetChildRedrawRequired(group, FALSE) ;

  /* Make sure group covers whole span in y direction. */
  zmapWindowContainerSetBackgroundSize(group, bot) ;

  /* THIS WOULD ALL GO IF WE DIDN'T ADD EMPTY COLS...... */
  /* We can't set the ITEM_FEATURE_DATA as we don't have the feature set at this point.
   * This probably points to some muckiness in the code, problem is caused by us deciding
   * to display all columns whether they have features or not and so some columns may not
   * have feature sets. */

  /* Attach data to the column including what strand the column is on and what frame it
   * represents, and also its style and a table of styles, used to cache column feature styles
   * where there is more than one feature type in a column. */
  set_data = g_new0(ZMapWindowItemFeatureSetDataStruct, 1) ;
  set_data->window      = window ;
  set_data->strand      = strand ;
  set_data->frame       = frame ;
  set_data->style       = zMapFeatureStyleCopy(style) ;
  set_data->style_table = zmapWindowStyleTableCreate() ;
  set_data->user_hidden_stack = g_queue_new() ;

  zmapWindowContainerSetData(group, ITEM_FEATURE_SET_DATA, set_data);

  zmapWindowContainerSetData(group, ITEM_FEATURE_DATA, feature_set);

  g_signal_connect(G_OBJECT(group), "destroy", G_CALLBACK(containerDestroyCB), (gpointer)window) ;

  bounding_box = zmapWindowContainerGetBackground(group) ;
  /* Not sure why the bounding_box isn't the subject here, but it breaks a load of stuff. */
  g_signal_connect(G_OBJECT(group), "event", G_CALLBACK(columnBoundingBoxEventCB), (gpointer)window) ;


  /* Some columns are hidden initially, perhaps because of magnification level or explicitly in
   * the style for the column. */
  if (zMapStyleGetHidden(style))
    zmapWindowColumnHide(group) ;
  else
    zmapWindowColumnSetMagState(window, group) ;


  status = zmapWindowFToIAddSet(window->context_to_item,
				align->unique_id,
				block->unique_id,
				feature_set->unique_id,
				strand, frame,
				group) ;
  zMapAssert(status) ;


  return group ;
}


/* Debug. */
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;

  printf("%s ", g_quark_to_string(feature_set->unique_id)) ;

  return ;
}



/* Called to draw each individual feature. */
static void ProcessFeature(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ; 
  CreateFeatureSetData featureset_data = (CreateFeatureSetData)user_data ;
  ZMapWindow window = featureset_data->window ;
  FooCanvasGroup *column_group ;
  ZMapStrand strand ;
  FooCanvasItem *feature_item ;


  strand = zmapWindowFeatureStrand(feature) ;

  /* Do some filtering on frame and strand */

  /* If we are doing frame specific display then don't display the feature if its the wrong
   * frame or its on the reverse strand and we aren't displaying reverse strand frames. */
  if (featureset_data->frame != ZMAPFRAME_NONE
      && (featureset_data->frame != zmapWindowFeatureFrame(feature)
	  || (!(window->show_3_frame_reverse) && strand == ZMAPSTRAND_REVERSE)))
    return ;


  /* Caller may not want a forward or reverse strand and this is indicated by NULL value for
   * curr_forward_col or curr_reverse_col */
  if ((strand == ZMAPSTRAND_FORWARD && !(featureset_data->curr_forward_col))
      || (strand == ZMAPSTRAND_REVERSE && !(featureset_data->curr_reverse_col)))
    return ;


  /* Get the correct column to draw into... */
  if (strand == ZMAPSTRAND_FORWARD)
    {
      column_group = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(featureset_data->curr_forward_col)) ;
    }
  else
    {
      column_group = zmapWindowContainerGetParent(FOO_CANVAS_ITEM(featureset_data->curr_reverse_col)) ;
    }


  feature_item = zmapWindowFeatureDraw(window, column_group, feature) ;


#ifdef RDS_DONT_INCLUDE
  if (feature->type == ZMAPFEATURE_ALIGNMENT)
    {
      GList *feature_list ;

      /* Try and find this feature in the hash of features for this column, if we find it
       * we need to remove it from the hash table and then re-add it. */
      if ((feature_list = g_hash_table_lookup(featureset_data->feature_hash,
					      GINT_TO_POINTER(feature->original_id))))
	{
	  if (!g_hash_table_steal(featureset_data->feature_hash,
				  GINT_TO_POINTER(feature->original_id)))
	    zMapCrash("Logic error in hash table handling for constructing lists of alignments"
		      "with same parent (\"%s\")" , g_quark_to_string(feature->original_id)) ;
	}

      /* Add this feature item into the list. */
      feature_list = g_list_append(feature_list, feature_item) ;

      /* Insert (or reinsert) the new list into the hash. */
      g_hash_table_insert(featureset_data->feature_hash, 
			  GINT_TO_POINTER(feature->original_id), feature_list) ;
    }
#endif

  return ;
}



/* Note how we must look at the items drawn, _not_ whether there are any features in the feature
 * set because the features may not be drawn (e.g. because they are on the opposite strand. */
static void removeEmptyColumnCB(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;

  zmapWindowRemoveIfEmptyCol(&container) ;

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
	ZMapWindowItemFeatureSetData set_data ;
        FooCanvasGroup *container_parent;

        container_parent = zmapWindowContainerGetParent(item);

	/* THIS WILL ALSO GO WHEN WE HAVE NO EMPTY COLS..... */
	/* If a column is empty it will not have a feature set but it will have a style from which we
	 * can display the column id. */
	feature_set = (ZMapFeatureSet)zmapWindowContainerGetData(container_parent, 
                                                                 ITEM_FEATURE_DATA);

	/* These should go in container some time.... */
	set_data = (ZMapWindowItemFeatureSetData)zmapWindowContainerGetData(container_parent, 
                                                                            ITEM_FEATURE_SET_DATA) ;
	zMapAssert(set_data) ;

	zMapAssert(feature_set || set_data->style) ;

	/* Swop focus from previous item(s)/columns to this column. */
	zMapWindowUnHighlightFocusItems(window) ;

	zmapWindowFocusSetHotColumn(window->focus, container_parent);
	zmapHighlightColumn(window, container_parent) ;
        
	/* Button 1 and 3 are handled, 2 is passed on to a general handler which could be
	 * the root handler. */
	switch (but_event->button)
	  {
	  case 1:
	    {
	      ZMapWindowSelectStruct select = {0} ;
	      GQuark feature_set_id ;

	      if (feature_set)
		feature_set_id = feature_set->original_id ;
	      else
		feature_set_id = set_data->style->original_id ;

	      select.feature_desc.feature_set = (char *)g_quark_to_string(feature_set_id) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      /* probably we should get this displayed somehow but currently it doesn't. */

	      select.feature_desc.feature_description
		= zmapWindowFeatureSetDescription(style->original_id, style) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


              select.secondary_text = zmapWindowFeatureSetDescription(feature_set_id, set_data->style) ;
              select.type = ZMAPWINDOW_SELECT_SINGLE;

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
		  makeColumnMenu(but_event, window, item, feature_set, set_data->style) ;

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
      {ZMAPGUI_MENU_NORMAL, "DNA Search Window",  5, columnMenuCB, NULL},
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
	ZMapWindowItemFeatureSetData set_data ;
        GList *list ;
	
        feature = (ZMapFeatureAny)g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_DATA) ;

	set_data = g_object_get_data(G_OBJECT(menu_data->item), ITEM_FEATURE_SET_DATA) ;
	zMapAssert(set_data) ;

	list = zmapWindowFToIFindItemSetFull(menu_data->window->context_to_item, 
					     feature->parent->parent->unique_id,
					     feature->parent->unique_id,
					     feature->unique_id,
					     zMapFeatureStrand2Str(set_data->strand),
					     zMapFeatureFrame2Str(set_data->frame),
					     g_quark_from_string("*"), NULL, NULL) ;
	
        zmapWindowListWindowCreate(menu_data->window, list, 
                                   (char *)g_quark_to_string(feature->original_id), 
                                   NULL, TRUE) ;

	break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->window, menu_data->item) ;
      break ;

    case 5:
      zmapWindowCreateDNAWindow(menu_data->window, menu_data->item) ;

      break ;

    default:
      zMapAssert("Coding error, unrecognised menu item number.") ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}





/* Read colour information from the configuration file, note that we read _only_ the first
 * colour stanza found in the file, subsequent ones are not read. */
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
       {"colour_item_mark", ZMAPCONFIG_STRING, {ZMAP_WINDOW_ITEM_MARK}},
       {"colour_item_highlight", ZMAPCONFIG_STRING, {NULL}},
       {"colour_frame_0", ZMAPCONFIG_STRING, {ZMAP_WINDOW_FRAME_0}},
       {"colour_frame_1", ZMAPCONFIG_STRING, {ZMAP_WINDOW_FRAME_1}},
       {"colour_frame_2", ZMAPCONFIG_STRING, {ZMAP_WINDOW_FRAME_2}},
       {NULL, -1, {NULL}}} ;


  /* IN A HURRY...BADLY WRITTEN, COULD BE MORE COMPACT.... */

  if ((config = zMapConfigCreate()))
    {
      colour_stanza = zMapConfigMakeStanza(window_stanza_name, colour_elements) ;

      if (zMapConfigFindStanzas(config, colour_stanza, &colour_list))
	{
	  ZMapConfigStanza next_colour ;
	  char *colour ;
	  
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

	  if ((colour = zMapConfigGetElementString(next_colour, "colour_item_mark")))
	    {
	      zmapWindowMarkSetColour(window->mark, colour) ;
	    }

	  if ((colour = zMapConfigGetElementString(next_colour, "colour_item_highlight")))
	    {
	      gdk_color_parse(colour, &(window->colour_item_highlight)) ;
	      window->use_rev_video = FALSE ;
	    }

	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_frame_0"),
			  &(window->colour_frame_0)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_frame_1"),
			  &(window->colour_frame_1)) ;
	  gdk_color_parse(zMapConfigGetElementString(next_colour, "colour_frame_2"),
			  &(window->colour_frame_2)) ;
	  
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
	  gdk_color_parse(ZMAP_WINDOW_FRAME_0, &(window->colour_frame_0)) ;
	  gdk_color_parse(ZMAP_WINDOW_FRAME_1, &(window->colour_frame_1)) ;
	  gdk_color_parse(ZMAP_WINDOW_FRAME_2, &(window->colour_frame_2)) ;
	}


      zMapConfigDestroyStanza(colour_stanza) ;
      
      zMapConfigDestroy(config) ;
    }

  return ;
}

/* Should this be a FooCanvasItem* of a gpointer GObject ??? The documentation is lacking here! 
 * well it compiles.... The wonders of the G_CALLBACK macro. */
static gboolean containerDestroyCB(FooCanvasItem *item, gpointer user_data)
{
  gboolean result = FALSE ;				    /* Always return FALSE ?? check this.... */
  FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;
  ZMapWindow window = (ZMapWindow)user_data ;
  GHashTable *context_to_item ;
  ZMapFeatureAny feature_any  = NULL;
  ZMapFeatureSet feature_set = NULL;
  ZMapFeatureBlock feature_block = NULL;
  ZMapFeatureAlignment feature_align = NULL;
  gboolean status = FALSE ;

  /* Some items may not have features attached...e.g. empty cols....I should revisit the empty
     cols bit, it keeps causing trouble all over the place.....column creation would be so much
     * simpler without it.... */
  if ((feature_any = (ZMapFeatureAny)(g_object_get_data(G_OBJECT(group), ITEM_FEATURE_DATA))))
    {
      context_to_item = window->context_to_item;

      switch (feature_any->struct_type)
	{
	case ZMAPFEATURE_STRUCT_CONTEXT:
	  {
	    status = zmapWindowFToIRemoveRoot(context_to_item) ;

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_ALIGN:
	  {
	    feature_align = (ZMapFeatureAlignment)feature_any;

	    zmapWindowFToIRemoveAlign(context_to_item, feature_align->unique_id) ;

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_BLOCK:
	  {
	    feature_block = (ZMapFeatureBlock)feature_any ;

	    status = zmapWindowFToIRemoveBlock(context_to_item,
					       feature_block->parent->unique_id,
					       feature_block->unique_id) ;

	    break ;
	  }
	case ZMAPFEATURE_STRUCT_FEATURESET:
	  {
	    ZMapWindowItemFeatureSetData set_data ;

	    feature_set = (ZMapFeatureSet)feature_any ;


	    /* If the focus column goes then so should the focus items as they should be in step. */
	    if (zmapWindowFocusGetHotColumn(window->focus) == group)
	      zmapWindowFocusReset(window->focus) ;


	    /* These should go in container some time.... */
	    set_data = g_object_get_data(G_OBJECT(group), ITEM_FEATURE_SET_DATA) ;
	    zMapAssert(set_data) ;

	    status = zmapWindowFToIRemoveSet(context_to_item,
					     feature_set->parent->parent->unique_id,
					     feature_set->parent->unique_id,
					     feature_set->unique_id,
					     set_data->strand, set_data->frame) ;


	    /* Get rid of this columns own style + its style table. */
	    zMapFeatureTypeDestroy(set_data->style) ;
	    zmapWindowStyleTableDestroy(set_data->style_table) ;

	    /* Get rid of stack or user hidden items. */
	    if (!g_queue_is_empty(set_data->user_hidden_stack))
	      g_queue_foreach(set_data->user_hidden_stack, removeList, NULL) ;
	    g_queue_free(set_data->user_hidden_stack) ;

	    g_free(set_data) ;

	    break ;
	  }
	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}

#ifdef RDS_DEBUG_ITEM_IN_HASH_DESTROY
      if(!status)
	printf("containerDestroyCB (%p): remove failed\n", group);
#endif /* RDS_DEBUG_ITEM_IN_HASH_DESTROY */
    }
  else
    {
      printf("containerDestroyCB (%p): no Feature Data\n", group);
    }

  return result ;					    /* ????? */
}



static ZMapFeatureContextExecuteStatus windowDrawContext(GQuark key_id, 
                                                         gpointer data, 
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapCanvasData canvas_data = (ZMapCanvasData)user_data;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapWindow          window = canvas_data->window;
  ZMapFeatureAlignment feature_align;
  ZMapFeatureBlock     feature_block;
  ZMapFeatureSet         feature_set;
  ZMapFeatureStructType feature_type;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  feature_type = feature_any->struct_type;

  if(0)
    printf("drawing %s\n", g_quark_to_string(feature_any->unique_id));

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        FooCanvasGroup *align_parent;
        FooCanvasItem  *align_hash_item;
        double x, y;
        feature_align = (ZMapFeatureAlignment)feature_any;

        /* record the full_context current align, not the diff align which will get destroyed! */
        canvas_data->curr_alignment = zMapFeatureContextGetAlignmentByID(canvas_data->full_context, 
                                                                         feature_any->unique_id) ;
        /* THIS MUST GO.... */
        /* Always reset the aligns to start at y = 0. */
        canvas_data->curr_y_offset = 0.0 ;

        x = canvas_data->curr_x_offset ;
        y = canvas_data->full_context->sequence_to_parent.c1 ;
        my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

        if((align_hash_item = zmapWindowFToIFindItemFull(window->context_to_item, 
                                                         feature_align->unique_id, 
                                                         0, 0, ZMAPSTRAND_NONE, 
                                                         ZMAPFRAME_NONE, 0)))
          {
            zMapAssert(FOO_IS_CANVAS_GROUP(align_hash_item));
            align_parent = FOO_CANVAS_GROUP(align_hash_item);
          }
        else
          {
            align_parent = zmapWindowContainerCreate(canvas_data->curr_root_group,
                                                     ZMAPCONTAINER_LEVEL_ALIGN,
                                                     window->config.block_spacing,
                                                     &(window->colour_alignment),
                                                     &(window->canvas_border),
                                                     window->long_items) ;
            g_signal_connect(G_OBJECT(align_parent), 
                             "destroy", 
                             G_CALLBACK(containerDestroyCB), 
                             window) ;
          }

        zmapWindowContainerSetData(align_parent, ITEM_FEATURE_DATA, canvas_data->curr_alignment) ;

        canvas_data->curr_align_group = 
          zmapWindowContainerGetFeatures(align_parent) ;

        foo_canvas_item_set(FOO_CANVAS_ITEM(align_parent),
                            "x", x,
                            "y", y,
                            NULL) ;

        if(!(zmapWindowFToIAddAlign(window->context_to_item, key_id, align_parent)))
          {
            status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
            *error_out = g_strdup_printf("Failed to add alignment '%s' to hash!", 
                                         g_quark_to_string(key_id));
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        FooCanvasGroup *block_parent, *forward_group, *reverse_group ;
        FooCanvasItem *block_hash_item;
        GdkColor *for_bg_colour, *rev_bg_colour ;
        double x, y;
        gboolean block_created = FALSE;

        feature_block = (ZMapFeatureBlock)feature_any;

        /* record the full_context current block, not the diff block which will get destroyed! */
        canvas_data->curr_block = zMapFeatureAlignmentGetBlockByID(canvas_data->curr_alignment, 
                                                                   feature_block->unique_id);

        /* Always set y offset to be top of current block. */
        canvas_data->curr_y_offset = feature_block->block_to_sequence.t1 ;

        if((block_hash_item = zmapWindowFToIFindItemFull(window->context_to_item,
                                                         canvas_data->curr_alignment->unique_id,
                                                         feature_block->unique_id, 0, ZMAPSTRAND_NONE,
                                                         ZMAPFRAME_NONE, 0)))
          {
            zMapAssert(FOO_IS_CANVAS_GROUP(block_hash_item));
            block_parent = FOO_CANVAS_GROUP(block_hash_item);
          }
        else
          {
            block_parent = zmapWindowContainerCreate(canvas_data->curr_align_group,
                                                     ZMAPCONTAINER_LEVEL_BLOCK,
                                                     window->config.strand_spacing,
                                                     &(window->colour_block),
                                                     &(canvas_data->window->canvas_border),
                                                     window->long_items) ;
            g_signal_connect(G_OBJECT(block_parent), 
                             "destroy", 
                             G_CALLBACK(containerDestroyCB), 
                             window) ;
            block_created = TRUE;
          }

        zmapWindowContainerSetData(block_parent, ITEM_FEATURE_DATA, canvas_data->curr_block) ;

        canvas_data->curr_block_group = 
          zmapWindowContainerGetFeatures(block_parent) ;
        
        x = 0.0 ;
        y = feature_block->block_to_sequence.t1 ;
        my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x, &y) ;

        foo_canvas_item_set(FOO_CANVAS_ITEM(block_parent),
                            "y", y,
                            NULL) ;
        

        /* Add this block to our hash for going from the feature context to its on screen item. */
        if(!(zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
                                    canvas_data->curr_alignment->unique_id, 
                                    key_id,
                                    block_parent)))
          {
            status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
            *error_out = g_strdup_printf("Failed to add block '%s' to hash!", 
                                         g_quark_to_string(key_id));
          }
        else
          {
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
            
            /* Create the reverse group first.  It's then first in the list and
             * so gets called first in container execute. e.g. reposition code */
            if(block_created ||
               !(reverse_group = zmapWindowContainerGetStrandGroup(block_parent, ZMAPSTRAND_REVERSE)))
              {
                reverse_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
                                                          ZMAPCONTAINER_LEVEL_STRAND,
                                                          window->config.column_spacing,
                                                          rev_bg_colour, 
                                                          &(canvas_data->window->canvas_border),
                                                          window->long_items) ;
                
                zmapWindowContainerSetStrand(reverse_group, ZMAPSTRAND_REVERSE);
                
                g_signal_connect(G_OBJECT(zmapWindowContainerGetBackground(reverse_group)),
                                 "event", G_CALLBACK(strandBoundingBoxEventCB), 
                                 (gpointer)window);
              }

            canvas_data->curr_reverse_group = 
              zmapWindowContainerGetFeatures(reverse_group) ;

            if(block_created ||
               !(forward_group = zmapWindowContainerGetStrandGroup(block_parent, ZMAPSTRAND_FORWARD)))
              {
                forward_group = zmapWindowContainerCreate(canvas_data->curr_block_group,
                                                          ZMAPCONTAINER_LEVEL_STRAND,
                                                          window->config.column_spacing,
                                                          for_bg_colour, 
                                                          &(canvas_data->window->canvas_border),
                                                          window->long_items) ;
                
                zmapWindowContainerSetStrand(forward_group, ZMAPSTRAND_FORWARD);
                
                g_signal_connect(G_OBJECT(zmapWindowContainerGetBackground(forward_group)),
                                 "event", G_CALLBACK(strandBoundingBoxEventCB), 
                                 (gpointer)window);
              }

            canvas_data->curr_forward_group = 
              zmapWindowContainerGetFeatures(forward_group) ;

          }      
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        FooCanvasGroup *tmp_forward, *tmp_reverse;

        canvas_data->curr_set = zMapFeatureBlockGetSetByID(canvas_data->curr_block, feature_any->unique_id);

        feature_set = (ZMapFeatureSet)feature_any;

        if(zmapWindowCreateSetColumns(window,
                                      canvas_data->curr_forward_group,
                                      canvas_data->curr_reverse_group,
                                      canvas_data->curr_block,
                                      canvas_data->curr_set,
                                      ZMAPFRAME_NONE,
                                      &tmp_forward, &tmp_reverse))
          {
            zmapWindowDrawFeatureSet(window, 
                                     feature_set,
                                     tmp_forward,
                                     tmp_reverse,
                                     ZMAPFRAME_NONE);
            if(tmp_forward)
              zmapWindowRemoveIfEmptyCol(&tmp_forward);
            if(tmp_reverse)
              zmapWindowRemoveIfEmptyCol(&tmp_reverse);
          }
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      {

      }
      break;
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
      zMapAssertNotReached();
      break;
    }

  return status;
}

static void drawZMap(ZMapCanvasData canvas_data, ZMapFeatureContext context)
{

  zMapFeatureContextExecuteComplete((ZMapFeatureAny)context,
                                    ZMAPFEATURE_STRUCT_FEATURE,
                                    windowDrawContext,
                                    NULL,
                                    canvas_data);

  return ;
}





static void removeList(gpointer data, gpointer user_data_unused)
{
  GList *user_hidden_items = (GList *)data ;

  g_list_free(user_hidden_items) ;

  return ;
}


/****************** end of file ************************************/
