/*  File: zmapWindowDraw.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: General drawing functions for zmap window, e.g.
 *              repositioning columns after one has been bumped
 *              or removed etc.
 *
 * Exported functions: See zmapWindow_P.h
 * HISTORY:
 * Last edited: Apr 27 11:22 2009 (edgrif)
 * Created: Thu Sep  8 10:34:49 2005 (edgrif)
 * CVS info:   $Id: zmapWindowDraw.c,v 1.110 2009-04-28 14:32:17 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainer.h>



/* For straight forward bumping. */
typedef struct
{
  ZMapWindow window ;
  ZMapWindowItemFeatureSetData set_data ;

  GHashTable *pos_hash ;
  GList *pos_list ;

  double offset ;
  double incr ;

  int start, end ;

  gboolean bump_all ;

  ZMapFeatureTypeStyle bumped_style ;
} BumpColStruct, *BumpCol ;


typedef struct
{
  double y1, y2 ;
  double offset ;
  double incr ;
} BumpColRangeStruct, *BumpColRange ;



/* THERE IS A BUG IN THE CODE THAT USES THIS...IT MOVES FEATURES OUTSIDE OF THE BACKGROUND SO
 * IT ALL LOOKS NAFF, PROBABLY WE GET AWAY WITH IT BECAUSE COLS ARE SO WIDELY SPACED OTHERWISE
 * THEY MIGHT OVERLAP. I THINK THE MOVING CODE MAY BE AT FAULT. */
/* For complex bump users seem to want columns to overlap a bit, if this is set to 1.0 there is
 * no overlap, if you set it to 0.5 they will overlap by a half and so on. */
#define COMPLEX_BUMP_COMPRESS 1.0

typedef struct
{
  ZMapWindow window ;
  GList **gaps_added_items ;
} AddGapsDataStruct, *AddGapsData ;

typedef gboolean (*OverLapListFunc)(GList *curr_features, GList *new_features) ;

typedef GList* (*GListTraverseFunc)(GList *list) ;

typedef struct
{
  ZMapWindow window ;
  ZMapFeatureTypeStyle bumped_style ;
  gboolean bump_all ;
  GHashTable *name_hash ;
  GList *bumpcol_list ;
  double curr_offset ;
  double incr ;
  OverLapListFunc overlap_func ;
  gboolean protein ;
} ComplexBumpStruct, *ComplexBump ;


typedef struct
{
  ZMapWindow window ;
  double incr ;
  double offset ;
  GList *feature_list ;
} ComplexColStruct, *ComplexCol ;


typedef struct
{
  GList **name_list ;
  OverLapListFunc overlap_func ;
} FeatureDataStruct, *FeatureData ;

typedef struct
{
  gboolean overlap ;
  int start, end ;
  int feature_diff ;
  ZMapFeature feature ;
  double width ;
} RangeDataStruct, *RangeData ;



typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;


typedef struct execOnChildrenStruct_
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;


/* For 3 frame display/normal display. */
typedef struct
{
  FooCanvasGroup *forward_group, *reverse_group ;
  ZMapFeatureBlock block ;
  ZMapWindow window ;
  GData *styles ;

  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

  int frame3_pos ;
  ZMapFrame frame ;

  ZMapFeatureTypeStyle style;
} RedrawDataStruct, *RedrawData ;



typedef enum {COLINEAR_INVALID, COLINEAR_NOT, COLINEAR_IMPERFECT, COLINEAR_PERFECT} ColinearityType ;




typedef struct
{
  ZMapWindowItemFeatureBlockData block_data ;
  int start, end ;
  gboolean in_view ;
  double left, right ;
  GList *columns_hidden ;
  ZMapWindowCompressMode compress_mode ;
} VisCoordsStruct, *VisCoords ;


typedef struct
{
  ZMapWindow window;

  /* Records which alignment, block, set, type we are processing. */
  ZMapFeatureContext full_context ;
  GData *styles ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  FooCanvasGroup *curr_root_group ;
  FooCanvasGroup *curr_align_group ;
  FooCanvasGroup *curr_block_group ;
  FooCanvasGroup *curr_forward_group ;
  FooCanvasGroup *curr_reverse_group ;

  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

}SeparatorCanvasDataStruct, *SeparatorCanvasData;


static void preZoomCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                      ZMapContainerLevelType level, gpointer user_data) ;
static void resetWindowWidthCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                               ZMapContainerLevelType level, gpointer user_data);
static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window) ;


static gint horizPosCompare(gconstpointer a, gconstpointer b) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printChild(gpointer data, gpointer user_data) ;
static void printQuarks(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void hideColsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
		       ZMapContainerLevelType level, gpointer user_data) ;
static void featureInViewCB(void *data, void *user_data) ;
static void showColsCB(void *data, void *user_data) ;
static void rebumpColsCB(void *data, void *user_data) ;

static ZMapFeatureContextExecuteStatus draw_separator_features(GQuark key_id,
							       gpointer feature_data,
							       gpointer user_data,
							       char **error_out);
static void drawSeparatorFeatures(SeparatorCanvasData canvas_data, ZMapFeatureContext context);


/*! @addtogroup zmapwindow
 * @{
 *  */


/*!
 * Toggles the display of the Reading Frame columns, these columns show frame senstive
 * features in separate sets of columns according to their reading frame.
 *
 * @param window     The zmap window to display the reading frame in.
 * @return  <nothing>
 *  */
void zMapWindowToggle3Frame(ZMapWindow window)
{
  gpointer three_frame_id = NULL;

  zMapWindowBusy(window, TRUE) ;

  three_frame_id = GUINT_TO_POINTER(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME));

  if(g_list_find(window->feature_set_names, three_frame_id))
    {
#ifdef RDS_REMOVED
      /* Remove all col. configuration windows as columns will be destroyed/recreated and the column
       * list will be out of date. */
      zmapWindowColumnConfigureDestroy(window) ;
      /* The column configure widget can reload itself now. */
#endif
      zmapWindowDrawRemove3FrameFeatures(window);
      
      window->display_3_frame = !window->display_3_frame;
      
      zmapWindowDraw3FrameFeatures(window);
      
      zmapWindowColOrderColumns(window);
      
      zmapWindowFullReposition(window) ;
    }
  else
    zMapWarning("%s", "No '" ZMAP_FIXED_STYLE_3FRAME "' column in config file.");

  zMapWindowBusy(window, FALSE) ;

  return ;
}



/*! @} end of zmapwindow docs. */




/* Sorts the children of a group by horizontal position, sorts all children that
 * are groups and leaves children that are items untouched, this is because item children
 * are background items that do not need to be ordered.
 */
void zmapWindowCanvasGroupChildSort(FooCanvasGroup *group_inout)
{

  zMapAssert(FOO_IS_CANVAS_GROUP(group_inout)) ;

  group_inout->item_list = g_list_sort(group_inout->item_list, horizPosCompare) ;
  group_inout->item_list_end = g_list_last(group_inout->item_list);

  /* Now reset the last item as well !!!! */
  group_inout->item_list_end = g_list_last(group_inout->item_list) ;

  return ;
}




/*
 *                         some column functions.... 
 */


/* This function sets the current visibility of a column according to the column state,
 * this may be easy (off or on) or may depend on current mag state/mark, frame state or compress
 * state. If !new_col_state then the current column state is used to set the show/hide state,
 * need this for setting up initial state of columns.
 * 
 *  */
void zmapWindowColumnSetState(ZMapWindow window, FooCanvasGroup *column_group,
			      ZMapStyleColumnDisplayState new_col_state, gboolean redraw_if_needed)
{
  ZMapWindowItemFeatureSetData set_data ;
  ZMapStyleColumnDisplayState curr_col_state ;

  set_data = g_object_get_data(G_OBJECT(column_group), ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  curr_col_state = zmapWindowItemFeatureSetGetDisplay(set_data) ;

  /* Do we need a redraw....not every time..... */
  if (!new_col_state || new_col_state != curr_col_state)
    {
      gboolean redraw = FALSE ;

      if (!new_col_state)
	new_col_state = curr_col_state ;

      switch(new_col_state)
	{
	case ZMAPSTYLE_COLDISPLAY_HIDE:
	  {
	    /* Always hide column. */
	    zmapWindowContainerSetVisibility(column_group, FALSE) ;

	    redraw = TRUE ;
	    break ;
	  }
	case ZMAPSTYLE_COLDISPLAY_SHOW_HIDE:
	  {
	    gboolean mag_visible, frame_visible ;

	    mag_visible   = zmapWindowColumnIsMagVisible(window, column_group) ;
	    
	    frame_visible = zmapWindowColumnIs3frameVisible(window, column_group) ;


	    if(mag_visible && frame_visible)
	      {
		zmapWindowColumnShow(column_group);
		redraw = TRUE;
	      }
	    else if(!mag_visible || !frame_visible)
	      {
		zmapWindowColumnHide(column_group);
		redraw = TRUE;
	      }

#ifdef RDS_REMOVED
	    /* Check mag, mark, compress etc. etc....probably need some funcs in compress/mark/mag
	     * packages to return whether a column should be hidden.... */
	    if ((curr_col_state == ZMAPSTYLE_COLDISPLAY_HIDE || curr_col_state == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE)
		&& mag_visible && frame_visible)
	      {
		zmapWindowColumnShow(column_group) ;
		redraw = TRUE ;
	      }
	    else if ((curr_col_state == ZMAPSTYLE_COLDISPLAY_SHOW || curr_col_state == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE)
		     && (!mag_visible || !frame_visible))
	      {
		zmapWindowColumnHide(column_group) ;
		redraw = TRUE ;
	      }
#endif
	    break ;
	  }
	default: /* ZMAPSTYLE_COLDISPLAY_SHOW */
	  {
	    /* Always show column. */
	    zmapWindowContainerSetVisibility(column_group, TRUE) ;

	    redraw = TRUE ;
	    break ;
	  }
	}

      zmapWindowItemFeatureSetDisplay(set_data, new_col_state) ;

      /* Only do redraw if it was requested _and_ state change needs it. */
      if (redraw_if_needed && redraw)
	zmapWindowFullReposition(window) ;
    }

  
  return ;
}



/* Set the hidden status of a column group, currently this depends on the mag setting in the
 * style for the column but we could allow the user to switch this on and off as well. */
void zmapWindowColumnSetMagState(ZMapWindow window, FooCanvasGroup *col_group)
{
  ZMapWindowItemFeatureSetData set_data ;

  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group)) ;

  set_data = zmapWindowContainerGetData(col_group, ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  /* Only check the mag factor if the column is visible. (wrong) */

  /* 
   * This test needs to be thought about.  We can't test if a column
   * is visible as it'll never get shown on zoom in. We don't want to 
   * mess with the columns that have been hidden by the user though 
   * (as happens now). I'm not sure we have a record of this.
   */

  if (zmapWindowItemFeatureSetGetDisplay(set_data) == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE &&
      zmapWindowColumnIs3frameVisible(window, col_group))
    {
      gboolean visible_at_this_mag = FALSE;

      visible_at_this_mag = zmapWindowColumnIsMagVisible(window, col_group);

      if(visible_at_this_mag)
	{
	  zmapWindowColumnShow(col_group) ;
	}
      else
	{
	  zmapWindowColumnHide(col_group) ; 
	}
    }

  return ;
}


/* checks to see if a column is 3 frame visible. */
gboolean zmapWindowColumnIs3frameVisible(ZMapWindow window, FooCanvasGroup *col_group)
{
  ZMapWindowItemFeatureSetData set_data ;
  ZMapStyle3FrameMode frame_mode;
  gboolean frame_specific, visible;
  int forward[ZMAPFRAME_2 + 1], reverse[ZMAPFRAME_2 + 1], none[ZMAPFRAME_2 + 1];
  int *frame_sens[ZMAPSTRAND_REVERSE + 1];

  zMapAssert(window);
  zMapAssert(FOO_IS_CANVAS_GROUP(col_group)) ;

  set_data = zmapWindowContainerGetData(col_group, ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  frame_specific = zmapWindowItemFeatureSetIsFrameSpecific(set_data, &frame_mode);

  if(frame_specific)
    {
      gboolean display, show_on_reverse;

      display         = window->display_3_frame;
      show_on_reverse = window->show_3_frame_reverse;

      /* 
	 | WINDOW->...      | REVERSE STRAND | FORWARD STRAND |
	 | DISPLAY|SHOW REV |NONE| 0 | 1 | 2 |NONE| 0 | 1 | 2 |
	 ------------------------------------------------------
	 | FALSE  | FALSE   | 1  | 0 | 0 | 0 | 1  | 0 | 0 | 0 |
	 | TRUE   | FALSE   | 1  | 0 | 0 | 0 | 0  | 1 | 1 | 1 |
	 | TRUE   | TRUE    | 0  | 1 | 1 | 1 | 0  | 1 | 1 | 1 |
	 | FALSE  | TRUE    | 1  | 0 | 0 | 0 | 1  | 0 | 0 | 0 |
      */
      
      frame_sens[ZMAPSTRAND_NONE]    = &none[0];
      frame_sens[ZMAPSTRAND_FORWARD] = &forward[0];
      frame_sens[ZMAPSTRAND_REVERSE] = &reverse[0];
      
      frame_sens[ZMAPSTRAND_NONE][ZMAPFRAME_NONE] = TRUE;
      frame_sens[ZMAPSTRAND_NONE][ZMAPFRAME_0]    = FALSE;
      frame_sens[ZMAPSTRAND_NONE][ZMAPFRAME_1]    = FALSE;
      frame_sens[ZMAPSTRAND_NONE][ZMAPFRAME_2]    = FALSE;
      
      frame_sens[ZMAPSTRAND_FORWARD][ZMAPFRAME_NONE] = (!display);
      frame_sens[ZMAPSTRAND_FORWARD][ZMAPFRAME_0]    = display;
      frame_sens[ZMAPSTRAND_FORWARD][ZMAPFRAME_1]    = display;
      frame_sens[ZMAPSTRAND_FORWARD][ZMAPFRAME_2]    = display;
      
      frame_sens[ZMAPSTRAND_REVERSE][ZMAPFRAME_NONE] = ((!display) || (display && !show_on_reverse));
      frame_sens[ZMAPSTRAND_REVERSE][ZMAPFRAME_0]    = (display && show_on_reverse);
      frame_sens[ZMAPSTRAND_REVERSE][ZMAPFRAME_1]    = (display && show_on_reverse);
      frame_sens[ZMAPSTRAND_REVERSE][ZMAPFRAME_2]    = (display && show_on_reverse);

      /* frame mode too... */
      switch(frame_mode)
	{
	case ZMAPSTYLE_3_FRAME_ALWAYS:
	case ZMAPSTYLE_3_FRAME_ONLY_3:
	case ZMAPSTYLE_3_FRAME_ONLY_1:
	  visible = frame_sens[set_data->strand][set_data->frame];
	  if(frame_mode == ZMAPSTYLE_3_FRAME_ONLY_1)
	    {
	      if(set_data->frame == ZMAPFRAME_NONE)
		visible = !visible;
	      else
		visible = FALSE;
	    }
	  else if(frame_mode == ZMAPSTYLE_3_FRAME_ONLY_3 &&
		  set_data->frame == ZMAPFRAME_NONE)
	    {
	      visible = FALSE;
	    }
	  break;
	case ZMAPSTYLE_3_FRAME_INVALID:
	case ZMAPSTYLE_3_FRAME_NEVER:
	  visible = FALSE;
	  break;
	default:
	  zMapLogWarning("%s", "Frame mode out of range");
	  visible = FALSE;
	  break;
	}
    }
  else if(set_data->frame == ZMAPFRAME_NONE)
    {
      visible = TRUE;
    }
  else
    {
      visible = FALSE;
    }

  return visible ;
}



/* Checks to see if column would be visible according to current mag state.
 */
gboolean zmapWindowColumnIsMagVisible(ZMapWindow window, FooCanvasGroup *col_group)
{
  gboolean visible = TRUE, mag_sensitive = FALSE ;
  ZMapWindowItemFeatureSetData set_data ;
  double min_mag, max_mag ;
  double curr_bases ;

  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group)) ;

  if((visible = zmapWindowContainerHasFeatures(col_group)))
    {
      set_data = zmapWindowContainerGetData(col_group, ITEM_FEATURE_SET_DATA) ;
      
      zMapAssert(set_data) ;
      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      curr_zoom = zMapWindowGetZoomMagnification(window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      curr_bases = zMapWindowGetZoomMagAsBases(window) ;
      
      if ((mag_sensitive = zmapWindowItemFeatureSetGetMagValues(set_data, &min_mag, &max_mag)))
	{
	  if ((min_mag && curr_bases > min_mag)
	      || 
	      (max_mag && curr_bases < max_mag))
	    {
	      visible = FALSE ;
	    }
	}
      
    }

  return visible ;
}




/* These next two calls should be followed in the user's code
 * by a call to zmapWindowNewReposition()...
 * Doing 
 *  g_list_foreach(columns_to_hide, call_column_hide, NULL);
 *  zmapWindowNewReposition(window);
 * Is a lot quicker than putting the Reposition call in these.
 */
void zmapWindowColumnHide(FooCanvasGroup *column_group)
{
  zMapAssert(column_group && FOO_IS_CANVAS_GROUP(column_group)) ;

  zmapWindowContainerSetVisibility(column_group, FALSE) ;

  return ;
}

void zmapWindowColumnShow(FooCanvasGroup *column_group)
{
  zMapAssert(column_group && FOO_IS_CANVAS_GROUP(column_group)) ;

  zmapWindowContainerSetVisibility(column_group, TRUE) ;

  return ;
}


/* Function toggles compression on and off. */
void zmapWindowColumnsCompress(FooCanvasItem *column_item, ZMapWindow window, ZMapWindowCompressMode compress_mode)
{
  ZMapWindowItemFeatureType feature_type ;
  ZMapContainerLevelType container_type ;
  FooCanvasGroup *column_group =  NULL, *block_group = NULL ;
  ZMapWindowItemFeatureBlockData block_data ;
  GList *compressed, *bumped;
  VisCoordsStruct coords ;
  double wx1, wy1, wx2, wy2 ;

  /* Decide if the column_item is a column group or a feature within that group. */
  if ((feature_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(column_item), ITEM_FEATURE_TYPE)))
      != ITEM_FEATURE_INVALID)
    {
      column_group = zmapWindowContainerGetParentContainerFromItem(column_item) ;
    }
  else if ((container_type = zmapWindowContainerGetLevel(FOO_CANVAS_GROUP(column_item)))
	   == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      column_group = FOO_CANVAS_GROUP(column_item) ;
    }
  else
    zMapAssertNotReached() ;

  block_group = zmapWindowContainerGetSuperGroup(column_group) ;
  container_type = zmapWindowContainerGetLevel(FOO_CANVAS_GROUP(block_group)) ;
  block_group = zmapWindowContainerGetSuperGroup(block_group) ;
  container_type = zmapWindowContainerGetLevel(FOO_CANVAS_GROUP(block_group)) ;
  
  block_data = g_object_get_data(G_OBJECT(block_group), ITEM_FEATURE_BLOCK_DATA) ;
  zMapAssert(block_data) ;

  compressed = zmapWindowItemFeatureBlockRemoveCompressedColumns(block_data);
  bumped     = zmapWindowItemFeatureBlockRemoveBumpedColumns(block_data);

  if(compressed || bumped)
    {
      if(compressed)
	{
	  g_list_foreach(compressed, showColsCB, NULL) ;
	  g_list_free(compressed) ;
	}

      if(bumped)
	{
	  g_list_foreach(bumped, rebumpColsCB, GUINT_TO_POINTER(compress_mode));
	  g_list_free(bumped);
	}

      zmapWindowFullReposition(window) ;
    }
  else
    {
      coords.block_data = block_data ;
      coords.compress_mode = compress_mode ;

      /* If there is no mark or user asked for visible area only then do that. */
      if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE)
	{
	  zmapWindowItemGetVisibleCanvas(window, 
					 &wx1, &wy1,
					 &wx2, &wy2) ;
        
	  /* should really clamp to seq. start/end..... */
	  coords.start = (int)wy1 ;
	  coords.end = (int)wy2 ;
	}
      else if (compress_mode == ZMAPWINDOW_COMPRESS_MARK)
	{
	  /* we know mark is set so no need to check result of range check. But should check
	   * that col to be bumped and mark are in same block ! */
	  zmapWindowMarkGetSequenceRange(window->mark, &coords.start, &coords.end) ;
	}
      else
	{
	  coords.start = window->min_coord ;
	  coords.end = window->max_coord ;
	}

      /* Note that curretly we need to start at the top of the tree or redraw does
       * not work properly. */
      zmapWindowreDrawContainerExecute(window, hideColsCB, &coords);
    }

  return ;
}


void zmapWindowContainerMoveEvent(FooCanvasGroup *super_root, ZMapWindow window)
{
  ContainerType type = CONTAINER_INVALID ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT);
  /* pre callback was set to zmapWindowContainerRegionChanged */
  zmapWindowContainerExecuteFull(FOO_CANVAS_GROUP(super_root),
                                 ZMAPCONTAINER_LEVEL_FEATURESET,
                                 NULL, NULL,
                                 NULL, NULL, TRUE) ;
  return ;
}

/*!
 * It's easy to forget, or not have access to set the window
 * scrollregion using resetWindowWidthCB, so here's a function
 * to do it.
 *
 * It does what zmapWindowFullReposition does, but in the same
 * cycle of recursion as the user's. Most calls to 
 * zmapWindowContainerExecuteFull which had TRUE for the last
 * parameter were missing doing this...
 *
 * N.B. All containers are traversed, so if speed is an issue...
 *
 * @param window     ZMapWindow to reset width for
 * @param enter_cb   Callback to get called at each container.
 * @param enter_data Data passed to callback as last parameter.
 * @return void
 */
void zmapWindowreDrawContainerExecute(ZMapWindow             window,
				      ZMapContainerExecFunc  enter_cb,
				      gpointer               enter_data)
{
  zMapPrintTimer(NULL, "About to do some work - including a reposition") ;

  window->interrupt_expose = TRUE ;

  zmapWindowContainerExecuteFull(FOO_CANVAS_GROUP(window->feature_root_group),
				 ZMAPCONTAINER_LEVEL_FEATURESET,
				 enter_cb,
				 enter_data,
				 resetWindowWidthCB, window,
				 TRUE) ;

  window->interrupt_expose = FALSE ;

  zMapPrintTimer(NULL, "Finished the work - including a reposition") ;

  return ;
}


/* Makes sure all the things that need to be redrawn when the canvas needs redrawing. */
void zmapWindowFullReposition(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;

  zMapPrintTimer(NULL, "About to resposition") ;



  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							   0,0,0,
							   ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
							   0)) ;
  zMapAssert(super_root) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;


  window->interrupt_expose = TRUE ;

  /* This could probably call the col order stuff as pre recurse function... */
  zmapWindowContainerExecuteFull(FOO_CANVAS_GROUP(super_root),
                                 ZMAPCONTAINER_LEVEL_FEATURESET,
                                 NULL,
                                 NULL,
                                 resetWindowWidthCB,
                                 window, TRUE) ;

  window->interrupt_expose = FALSE ;

  zmapWindowReFocusHighlights(window);

  zMapPrintTimer(NULL, "Finished resposition") ;

  return ;
}


/* Reset scrolled region width so that user can scroll across whole of canvas. */
void zmapWindowResetWidth(ZMapWindow window)
{
  FooCanvasGroup *root ;
  double x1, x2, y1, y2 ;
  double root_x1, root_x2, root_y1, root_y2 ;
  double scr_reg_width, root_width ;


  foo_canvas_get_scroll_region(window->canvas, &x1, &y1, &x2, &y2);

  scr_reg_width = x2 - x1 + 1.0 ;

  root = foo_canvas_root(window->canvas) ;

  foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(root), &root_x1, &root_y1, &root_x2, &root_y2) ;

  root_width = root_x2 - root_x1 + 1 ;

  if (root_width != scr_reg_width)
    {
      double excess ;

      excess = root_width - scr_reg_width ;

      x2 = x2 + excess;

      foo_canvas_set_scroll_region(window->canvas, x1, y1, x2, y2) ;
    }


  return ;
}



/* Makes sure all the things that need to be redrawn for zooming get redrawn. */
void zmapWindowDrawZoom(ZMapWindow window)
{
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;
  ZoomDataStruct zoom_data = {NULL} ;

  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window->context_to_item,
							   0,0,0,
							   ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
							   0)) ;
  zMapAssert(super_root) ;


  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;

  zoom_data.window = window ;
  zoom_data.zoom = zMapWindowGetZoomFactor(window) ;

  zmapWindowContainerExecuteFull(FOO_CANVAS_GROUP(super_root),
                                 ZMAPCONTAINER_LEVEL_FEATURESET,
                                 preZoomCB,
                                 &zoom_data,
                                 resetWindowWidthCB,
                                 window, TRUE) ;

  return ;
}


/* Some features have different widths according to their score, this helps annotators
 * guage whether something like an alignment is worth taking notice of.
 * 
 * Currently we only implement the algorithm given by acedb's Score_by_width but it maybe
 * that we will need to do others to support their display.
 *
 * I've done this because for James Havana DBs all methods that mention score have:
 *
 * Score_by_width	
 * Score_bounds	 70.000000 130.000000
 *
 * (the bounds are different for different data sources...)
 * 
 * The interface is slightly tricky here in that we use the style->width to calculate
 * width, not (x2 - x1), hence x2 is only used to return result.
 * 
 */
void zmapWindowGetPosFromScore(ZMapFeatureTypeStyle style, 
			       double score,
			       double *curr_x1_inout, double *curr_x2_out)
{
  double dx = 0.0 ;
  double numerator, denominator ;
  double max_score, min_score ;

  zMapAssert(style && curr_x1_inout && curr_x2_out) ;

  min_score = zMapStyleGetMinScore(style) ;
  max_score = zMapStyleGetMaxScore(style) ;

  /* We only do stuff if there are both min and max scores to work with.... */
  if (max_score && min_score)
    {
      double start = *curr_x1_inout ;
      double width, half_width, mid_way ;

      width = zMapStyleGetWidth(style) ;

      half_width = width / 2 ;
      mid_way = start + half_width ;

      numerator = score - min_score ;
      denominator = max_score - min_score ;

      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.25 ;
	  else if (numerator > 0)
	    dx = 1 ;
	}
      else
	{
	  dx = 0.25 + (0.75 * (numerator / denominator)) ;
	}

      if (dx < 0.25)
	dx = 0.25 ;
      else if (dx > 1)
	dx = 1 ;
      
      *curr_x1_inout = mid_way - (half_width * dx) ;
      *curr_x2_out = mid_way + (half_width * dx) ;
    }

  return ;
}


void zmapWindowDrawSeparatorFeatures(ZMapWindow           window, 
				     ZMapFeatureBlock     block,
				     ZMapFeatureSet       feature_set,
				     ZMapFeatureTypeStyle style)
{
  ZMapFeatureContext context_cp, context, diff;
  ZMapFeatureAlignment align_cp, align;
  ZMapFeatureBlock     block_cp;

  /* We need the block to know which one to draw into... */

  if (zMapStyleDisplayInSeparator(style))
    {
      SeparatorCanvasDataStruct canvas_data = {NULL};
      /* this is a good start. */
      /* need to copy the block, */
      block_cp = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block);
      /* ... the alignment ... */
      align    = (ZMapFeatureAlignment)zMapFeatureGetParentGroup((ZMapFeatureAny)block, 
								 ZMAPFEATURE_STRUCT_ALIGN) ;
      align_cp = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)align);
      /* ... and the context ... */
      context  = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)align, 
							       ZMAPFEATURE_STRUCT_CONTEXT) ;
      context_cp = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)context);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* This is pretty important! */
      //context_cp->styles = context->styles;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Now make a tree */
      zMapFeatureContextAddAlignment(context_cp, align_cp, context->master_align == align);
      zMapFeatureAlignmentAddBlock(align_cp, block_cp);
      zMapFeatureBlockAddFeatureSet(block_cp, feature_set);

      /* Now we have a context to merge. */
      zMapFeatureContextMerge(&(window->strand_separator_context),
			      &context_cp, &diff);

      canvas_data.window = window;
      canvas_data.full_context = window->strand_separator_context;
      canvas_data.styles = window->read_only_styles ;

      drawSeparatorFeatures(&canvas_data, diff);

      zmapWindowFullReposition(window);
    }
  else if(style)
    zMapLogWarning("Trying to draw feature set with non-separator "
		   "style '%s' in strand separator.",
		   zMapStyleGetName(style));
  else
    zMapLogWarning("Feature set '%s' has no style!", 
		   g_quark_to_string(feature_set->original_id));

  return;
}



/* 
 *                Internal routines.
 */




/* MAY NEED TO INGNORE BACKGROUND BOXES....WHICH WILL BE ITEMS.... */

/* A GCompareFunc() called from g_list_sort(), compares groups on their horizontal position. */
static gint horizPosCompare(gconstpointer a, gconstpointer b)
{
  gint result = 0 ;
  FooCanvasGroup *group_a = (FooCanvasGroup *)a, *group_b = (FooCanvasGroup *)b ;

  if (!FOO_IS_CANVAS_GROUP(group_a) || !FOO_IS_CANVAS_GROUP(group_b))
    {
      result = 0 ;
    }
  else
    {
      if (group_a->xpos < group_b->xpos)
	result = -1 ;
      else if (group_a->xpos > group_b->xpos)
	result = 1 ;
      else
	result = 0 ;
    }

  return result ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printItem(FooCanvasItem *item, int indent, char *prefix)
{
  int i ;

  for (i = 0 ; i < indent ; i++)
    {
      printf("  ") ;
    }
  printf("%s", prefix) ;

  zmapWindowPrintItemCoords(item) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* We need the window in here.... */
/* GFunc to call on potentially all container groups.
 */
static void preZoomCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                      ZMapContainerLevelType level, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZoomData zoom_data = (ZoomData)user_data ;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	FooCanvasGroup *container_underlay;
	container_underlay = zmapWindowContainerGetUnderlays(container);
	zmapWindowContainerPurge(container_underlay);
      }
      columnZoomChanged(container, zoom_data->zoom, zoom_data->window) ;
      break ;
    default:
      break ;
    }

  return ;
}

static void set_hlocked_scroll_region(gpointer key, gpointer value, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)key;
  FooCanvasPoints *box = (FooCanvasPoints *)user_data;

  zmapWindowSetScrollRegion(window, 
			    &box->coords[0], &box->coords[1], 
			    &box->coords[2], &box->coords[3]) ;

  return ;
}

/* A version of zmapWindowResetWidth which uses the points from the recursion to set the width */
static void resetWindowWidthCB(FooCanvasGroup *data, FooCanvasPoints *points, 
                               ZMapContainerLevelType level, gpointer user_data)
{
  ZMapWindow window = NULL;
  double x1, x2, y1, y2 ;       /* scroll region positions */
  double scr_reg_width, root_width ;

  if(level == ZMAPCONTAINER_LEVEL_ROOT)
    {
      window = (ZMapWindow)user_data ;

      zmapWindowGetScrollRegion(window, &x1, &y1, &x2, &y2);

      scr_reg_width = x2 - x1 + 1.0 ;

      root_width = points->coords[2] - points->coords[0] + 1.0 ;

      if (((root_width != scr_reg_width) && 
	   (window->curr_locking != ZMAP_WINLOCK_HORIZONTAL)))
        {
          double excess ;
          
          excess = root_width - scr_reg_width ;
          /* the spacing should be a border width from somewhere. */
          x2 = x2 + excess + window->config.strand_spacing;

	  /* Annoyingly the initial size of the canvas is an issue here on first draw */
	  if(y2 == 100.0)
	    y2 = window->max_coord;

          zmapWindowSetScrollRegion(window, &x1, &y1, &x2, &y2) ;
        }
      else if(((window->curr_locking == ZMAP_WINLOCK_HORIZONTAL) &&
	       (root_width > scr_reg_width)))
	{
	  double excess ;
          FooCanvasPoints *box;

          excess = root_width - scr_reg_width ;
          /* the spacing should be a border width from somewhere. */
          x2 = x2 + excess + window->config.strand_spacing;

	  /* Annoyingly the initial size of the canvas is an issue here on first draw */
	  if(y2 == 100.0)
	    y2 = window->max_coord;

	  box = foo_canvas_points_new(2);
	  box->coords[0] = x1;
	  box->coords[1] = y1;
	  box->coords[2] = x2;
	  box->coords[3] = y2;

          zmapWindowSetScrollRegion(window, &x1, &y1, &x2, &y2) ;

	  g_hash_table_foreach(window->sibling_locked_windows,
			       set_hlocked_scroll_region, box);

	  foo_canvas_points_free(box);
	}
    }
  
  return ;
}


static void columnZoomChanged(FooCanvasGroup *container, double new_zoom, ZMapWindow window)
{
  ZMapWindowItemFeatureSetData set_data ;

  set_data = zmapWindowContainerGetData(container, ITEM_FEATURE_SET_DATA) ;
  zMapAssert(set_data) ;

  if (zmapWindowItemFeatureSetGetDisplay(set_data) == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE)
    zmapWindowColumnSetMagState(window, container) ;

  return ;
}




/* 
 *             3 Frame Display functions.
 * 
 * 3 frame display is complex in that some columns should only be shown in "3 frame"
 * mode, whereas others are shown as a single column normally and then as the three
 * "frame" columns in "3 frame" mode. This involves deleting columns, moving them,
 * recreating them etc.
 * 
 */


/*
 *      Functions to remove frame sensitive columns from the canvas.
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* 
 *             debug functions, please leave here for future use.
 */


static void printChild(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  ZMapFeatureAny any_feature ;

  any_feature = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_DATA) ;

  printf("%s ", g_quark_to_string(any_feature->unique_id)) ;

  return ;
}



static void printPtr(gpointer data, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;

  printf("%p ", container) ;

  return ;
}


static void printQuarks(gpointer data, gpointer user_data)
{


  printf("quark str: %s\n", g_quark_to_string(GPOINTER_TO_INT(data))) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* GFunc to call on potentially all container groups.
 */
static void hideColsCB(FooCanvasGroup *data, FooCanvasPoints *points, 
		       ZMapContainerLevelType level, gpointer user_data)
{
  FooCanvasGroup *container = (FooCanvasGroup *)data ;
  VisCoords coord_data = (VisCoords)user_data ;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	FooCanvasGroup *features ;

	coord_data->in_view = FALSE ;
	coord_data->left = 999999999 ;			    /* Must be bigger than any feature position. */
	coord_data->right = 0 ;

	features = zmapWindowContainerGetFeatures(container) ;

	g_list_foreach(features->item_list, featureInViewCB, coord_data) ;

	if (zmapWindowItemIsShown(FOO_CANVAS_ITEM(container)))
	  {
	    ZMapFeatureSet feature_set ;
	    ZMapWindowItemFeatureSetData set_data ;

	    set_data = g_object_get_data(G_OBJECT(container), ITEM_FEATURE_SET_DATA) ;
	    zMapAssert(set_data) ;

	    feature_set = zmapWindowItemFeatureSetRecoverFeatureSet(set_data);
	    zMapAssert(feature_set);
	    
	    if (!(coord_data->in_view)
		&& (coord_data->compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE
		    || zmapWindowItemFeatureSetGetDisplay(set_data) != ZMAPSTYLE_COLDISPLAY_SHOW))
	      {
		/* No items overlap with given area so hide the column completely. */

		zmapWindowColumnHide(container) ;

		zmapWindowItemFeatureBlockAddCompressedColumn(coord_data->block_data, container);
	      }
	    else
	      {
		/* There are some items showing but column may need to be rebumped if there only a few. */
		double x1, y1, x2, y2, width_col, width_features ;
		gboolean bump_col = FALSE ;

		foo_canvas_item_get_bounds(FOO_CANVAS_ITEM(container), &x1, &y1, &x2, &y2) ;

		/* Compare width of column and total width of visible features, if difference is
		 * greater than 10% then rebump. This stops us rebumping columns that are just
		 * slightly different in size or have already been bumped to a mark for instance. */
		width_col = (x2 - x1) ;
		width_features = (coord_data->right - coord_data->left) ;

		if (((fabs(width_col - width_features)) / width_features) > 0.1)
		  bump_col = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		printf("col %s %s: %f, %f (%f)\titems: %f, %f (%f)\n",
		       g_quark_to_string(feature_set->original_id),
		       (bump_col ? "Bumped" : "Not Bumped"),
		       x1, x2, width_col,
		       coord_data->left, coord_data->right, width_features) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		if (bump_col)
		  {
		    zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(container),
					      ZMAPBUMP_INVALID, coord_data->compress_mode) ;

		    zmapWindowItemFeatureBlockAddBumpedColumn(coord_data->block_data, container);
		  }
	      }
	  }

	coord_data->in_view = FALSE ;

	break ;
      }
    default:
      break ;
    }

  return ;
}


/* check whether a feature is between start and end coords at all. */
static void featureInViewCB(void *data, void *user_data)
{
  FooCanvasItem *feature_item = (FooCanvasItem *)data ;
  VisCoords coord_data = (VisCoords)user_data ;
  ZMapFeature feature ;

  /* bumped cols have items that are _not_ features. */
  if ((feature = g_object_get_data(G_OBJECT(feature_item), ITEM_FEATURE_DATA)))
    {
      if (!(feature->x1 > coord_data->end || feature->x2 < coord_data->start))
	{
	  double x1, y1, x2, y2 ;

	  coord_data->in_view = TRUE ;

	  foo_canvas_item_get_bounds(feature_item, &x1, &y1, &x2, &y2) ;

	  if (x1 < coord_data->left)
	    coord_data->left = x1 ;
	  if (x2 > coord_data->right)
	    coord_data->right = x2 ;
	}
    }

  return ;
}


/* Reshow hidden cols. */
static void showColsCB(void *data, void *user_data_unused)
{
  FooCanvasGroup *col_group = (FooCanvasGroup *)data ;

  /* This is called from the Compress Columns code, which _is_ a user
   * action. */
  zmapWindowColumnShow(col_group) ; 

  return ;
}

/* Reshow bumped cols. */
static void rebumpColsCB(void *data, void *user_data)
{
  FooCanvasGroup *col_group = (FooCanvasGroup *)data ;

  /* This is called from the Compress Columns code, which _is_ a user
   * action. */

  zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(col_group), ZMAPBUMP_INVALID, GPOINTER_TO_UINT(user_data)) ;

  return ;
}


static ZMapFeatureContextExecuteStatus draw_separator_features(GQuark key_id,
							       gpointer feature_data,
							       gpointer user_data,
							       char **error_out)
{
  ZMapFeatureAny feature_any             = (ZMapFeatureAny)feature_data;
  SeparatorCanvasData canvas_data        = (SeparatorCanvasData)user_data;
  ZMapWindow window                      = canvas_data->window;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      break;
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
	FooCanvasGroup *align_parent;
	FooCanvasItem  *align_hash_item;

	canvas_data->curr_alignment = 
	  zMapFeatureContextGetAlignmentByID(canvas_data->full_context,
					     feature_any->unique_id);
	if((align_hash_item = zmapWindowFToIFindItemFull(window->context_to_item,
							 feature_any->unique_id,
							 0, 0, ZMAPSTRAND_NONE,
							 ZMAPFRAME_NONE, 0)))
	  {
	    zMapAssert(FOO_IS_CANVAS_GROUP(align_hash_item));
	    align_parent = FOO_CANVAS_GROUP(align_hash_item);
	    canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent);
	  }
	else
	  zMapAssertNotReached();
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	FooCanvasGroup *block_parent;
	FooCanvasItem  *block_hash_item;
	canvas_data->curr_block = 
	  zMapFeatureAlignmentGetBlockByID(canvas_data->curr_alignment,
					   feature_any->unique_id);

	if((block_hash_item = zmapWindowFToIFindItemFull(window->context_to_item,
							 canvas_data->curr_alignment->unique_id,
							 feature_any->unique_id, 0,
							 ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
	  {
	    zMapAssert(FOO_IS_CANVAS_GROUP(block_hash_item));
	    block_parent = FOO_CANVAS_GROUP(block_hash_item);

	    canvas_data->curr_block_group = 
	      zmapWindowContainerGetFeatures(block_parent);
	      
	    canvas_data->curr_forward_group =
	      zmapWindowContainerGetStrandGroup(block_parent, ZMAPSTRAND_FORWARD);
	    canvas_data->curr_forward_group =
	      zmapWindowContainerGetFeatures(canvas_data->curr_forward_group);
	  }
	else
	  zMapAssertNotReached();
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	FooCanvasGroup *tmp_forward, *separator;
	
	canvas_data->curr_set = zMapFeatureBlockGetSetByID(canvas_data->curr_block,
							   feature_any->unique_id);
	if (zmapWindowCreateSetColumns(window,
				       canvas_data->curr_forward_group,
				       canvas_data->curr_reverse_group,
				       canvas_data->curr_block,
				       canvas_data->curr_set,
				       canvas_data->styles,
				       ZMAPFRAME_NONE,
				       &tmp_forward, NULL, &separator))
	  {
	    zmapWindowColumnSetState(window, separator, ZMAPSTYLE_COLDISPLAY_SHOW, FALSE);

	    zmapWindowDrawFeatureSet(window,  canvas_data->styles, (ZMapFeatureSet)feature_any,
				     NULL, separator, ZMAPFRAME_NONE);

	    if (tmp_forward)
	      zmapWindowRemoveIfEmptyCol(&tmp_forward);
	  }
      }
      break;
    case ZMAPFEATURE_STRUCT_FEATURE:
      break;
    default:
      zMapAssertNotReached();
      break;
    }

  return status;
}

static void drawSeparatorFeatures(SeparatorCanvasData canvas_data, ZMapFeatureContext context)
{
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)context,
				    ZMAPFEATURE_STRUCT_FEATURE,
				    draw_separator_features,
				    NULL, canvas_data);
  return;
}


