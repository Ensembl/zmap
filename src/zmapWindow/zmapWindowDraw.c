/*  File: zmapWindowDraw.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: General drawing functions for zmap window, e.g.
 *              repositioning columns after one has been bumped
 *              or removed etc.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <math.h>
#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLogical.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerBlock.h>
#include <zmapWindowContainerFeatureSet_I.h>




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


#if 0
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
#endif


typedef struct
{
  ZMapWindowContainerBlock block_group;
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
  GHashTable *styles ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  FooCanvasGroup *curr_root_group ;
  ZMapWindowContainerFeatures curr_align_group ;
  ZMapWindowContainerFeatures curr_block_group ;
  ZMapWindowContainerFeatures curr_forward_group ;
  ZMapWindowContainerFeatures curr_reverse_group ;

  FooCanvasGroup *curr_forward_col ;
  FooCanvasGroup *curr_reverse_col ;

}SeparatorCanvasDataStruct, *SeparatorCanvasData;


static void toggleColumnInMultipleBlocks(ZMapWindow window, char *name,
					 GQuark align_id, GQuark block_id,
					 gboolean force_to, gboolean force) ;

static void preZoomCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                      ZMapContainerLevelType level, gpointer user_data) ;
static gboolean resetWindowWidthCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				   ZMapContainerLevelType level, gpointer user_data);


static gint horizPosCompare(gconstpointer a, gconstpointer b) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printChild(gpointer data, gpointer user_data) ;
static void printQuarks(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void hideColsCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
		       ZMapContainerLevelType level, gpointer user_data) ;
static void featureInViewCB(void *data, void *user_data) ;
static void showColsCB(void *data, void *user_data) ;
static void rebumpColsCB(void *data, void *user_data) ;
static ZMapFeatureContextExecuteStatus draw_separator_features(GQuark key_id,
							       gpointer feature_data,
							       gpointer user_data,
							       char **error_out);
static void drawSeparatorFeatures(SeparatorCanvasData canvas_data, ZMapFeatureContext context);

static void set3FrameState(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ; ;
static void myWindowSet3FrameMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ;



/* Turn on/off 3 frame cols. */
void zMapWindow3FrameToggle(ZMapWindow window)
{
  ZMapWindow3FrameMode mode = ZMAP_WINDOW_3FRAME_COLS;

  if(IS_3FRAME(window->display_3_frame))
      mode =ZMAP_WINDOW_3FRAME_INVALID;

  myWindowSet3FrameMode(window, mode ) ;

  return ;
}



/* Turn on/off 3 frame cols. */
void zMapWindow3FrameSetMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode)
{
  myWindowSet3FrameMode(window, frame_mode) ;

  return ;
}





/*!
 * Sets the display of the Reading Frame columns, these columns show frame senstive
 * features in separate sets of columns according to their reading frame.
 *
 *  */
static void myWindowSet3FrameMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode)
{
  gpointer three_frame_id = NULL;
  gpointer three_frame_Id = NULL;   // capitalised!

  zmapWindowBusy(window, TRUE) ;


  zMapStartTimer("3Frame" , IS_3FRAME(window->display_3_frame) ? "off" : "on");

  // yuk...
  three_frame_id = GUINT_TO_POINTER(zMapStyleCreateID(ZMAP_FIXED_STYLE_3FRAME));
  three_frame_Id = GUINT_TO_POINTER(g_quark_from_string(ZMAP_FIXED_STYLE_3FRAME));

  if (g_list_find(window->feature_set_names, three_frame_id)
      || g_list_find(window->feature_set_names, three_frame_Id))
    {
#ifndef RDS_REMOVED_STATS

      /* OK...SOMETHING'S NOT WORKING HERE...I DON'T SEE THE DYNAMIC RELOAD STUFF.... */

      /* Remove all col. configuration windows as columns will be destroyed/recreated and the column
       * list will be out of date. */
      zmapWindowColumnConfigureDestroy(window) ;
      /* The column configure widget can reload itself now. */
#endif

      zmapWindowFocusReset(window->focus) ;


      /* THIS NEEDS CHANGING SO WE DON'T REDRAW ALL THE COLUMNS WHEN WE ARE JUST ADDING
       * THE TRANSLATION COLUMN OR WHATEVER.....AND THE FOCUS REMOVE WILL NEED REDOING. */
      // remove always columns or 3-frame depending on mode
      zmapWindowDrawRemove3FrameFeatures(window);
      zMapStopTimer("3FrameRemove","");




      /* Set up the frame state. */
      set3FrameState(window, frame_mode) ;

      // draw always columns or 3-frame depending on mode
      zmapWindowDraw3FrameFeatures(window);
      zMapStopTimer("3FrameDraw","");

     /* Now we've drawn all the features we can position them all. */
     zmapWindowColOrderColumns(window);

     /* FullReposition Sets the correct scroll region. */
     zmapWindowFullReposition(window);

    }
  else
    {
      zMapWarning("%s", "No '" ZMAP_FIXED_STYLE_3FRAME "' column in config file.");
    }

  zMapStopTimer("3Frame" , IS_3FRAME(window->display_3_frame) ? "off" : "on");

  zmapWindowBusy(window, FALSE) ;

  return ;
}



void zMapWindowToggleDNAProteinColumns(ZMapWindow window,
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein,
                                       gboolean force_to, gboolean force)
{

  zmapWindowBusy(window, TRUE) ;

  if (dna)
    toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_DNA_NAME,
				 align_id, block_id, force_to, force);

  if (protein)
    toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_3FT_NAME,
				 align_id, block_id, force_to, force);

  zmapWindowFullReposition(window) ;

  zmapWindowBusy(window, FALSE) ;

  return ;
}



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


/* return state column should be in
 * so that we can set the initial state to what we want to avoid foocanvasing it twice
 */
gboolean zmapWindowGetColumnVisibility(ZMapWindow window,FooCanvasGroup *column_group)
{
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet) column_group;
  ZMapFeatureTypeStyle style = zMapWindowContainerFeatureSetGetStyle(container);
  ZMapStyleColumnDisplayState curr_col_state ;
  ZMapStyle3FrameMode frame_mode ;
  gboolean frame_sensitive ;
  gboolean visible = FALSE;
  gboolean mag_visible, frame_visible, frame_displayed = TRUE ;

  zMapAssert(style);

  if(!style->displayable)
      return(FALSE);

  curr_col_state = zmapWindowContainerFeatureSetGetDisplay(container) ;

  switch(curr_col_state)
  {
  case ZMAPSTYLE_COLDISPLAY_HIDE:
      visible = FALSE;
      break;

  case ZMAPSTYLE_COLDISPLAY_SHOW_HIDE:

      frame_sensitive = zmapWindowContainerFeatureSetIsFrameSpecific(container, &frame_mode) ;

      mag_visible = zmapWindowColumnIsMagVisible(window, column_group);

      frame_visible = zmapWindowColumnIs3frameVisible(window, column_group) ;

      if (IS_3FRAME(window->display_3_frame) && frame_sensitive)
            frame_displayed = zmapWindowColumnIs3frameDisplayed(window, column_group) ;

          /* Check mag, mark, compress etc. etc....probably need some funcs in compress/mark/mag
           * packages to return whether a column should be hidden.... */
      if (mag_visible && frame_visible && (!frame_sensitive || frame_displayed))
            visible = TRUE;
      break;

  case ZMAPSTYLE_COLDISPLAY_SHOW:
      visible = TRUE;
      break;

  default:
      /* set to INVALID (ie 0) on column creation */
      visible = TRUE;
      break;
  }

  return(visible);
}

/* This function sets the current visibility of a column according to the column state,
 * this may be easy (off or on) or may depend on current mag state/mark, frame state or compress
 * state. If !new_col_state then the current column state is used to set the show/hide state,
 * need this for setting up initial state of columns.
 *
 * new_col_state = NULL if new (empty) column
 *  */
void zmapWindowColumnSetState(ZMapWindow window, FooCanvasGroup *column_group,
			      ZMapStyleColumnDisplayState new_col_state, gboolean redraw_if_needed)
{
  ZMapWindowContainerFeatureSet container;
  ZMapStyleColumnDisplayState curr_col_state ;
  gboolean cur_visible,new_visible;

  zMapStartTimer("SetState","");

  container = (ZMapWindowContainerFeatureSet)column_group;

  curr_col_state = zmapWindowContainerFeatureSetGetDisplay(container) ;

  /* Do we need a redraw....not every time..... */
  if (!new_col_state || new_col_state != curr_col_state || redraw_if_needed)
    {
      gboolean redraw = FALSE ;

      if (!new_col_state)
      {
          new_col_state = curr_col_state ;
          cur_visible = FALSE;
        }
      else
      {
            ZMapWindowContainerGroup group = (ZMapWindowContainerGroup) column_group;

            cur_visible = group->flags.visible; /* actual current state */
      }

      zmapWindowContainerFeatureSetSetDisplay(container, new_col_state) ;

      new_visible = zmapWindowGetColumnVisibility(window,column_group);
            /* state we want rather than what's current */

      if(new_visible)
      {
            if(!cur_visible)
            {
                  zMapStartTimer("SetState","SetVis show");
                  redraw = TRUE;
                  zmapWindowContainerSetVisibility(column_group, TRUE) ;
            }
      }
      else
      {
            if(cur_visible)
            {
                  zMapStartTimer("SetState","SetVis show");
                  redraw = TRUE;
                  zmapWindowContainerSetVisibility(column_group, FALSE) ;
            }
      }
      if(redraw)
            zMapStopTimer("SetState","SetVis");

      /* Only do redraw if it was requested _and_ state change needs it. */
      if (redraw_if_needed && redraw)
            zmapWindowFullReposition(window) ;
   }
}




/* Set the hidden status of a column group, currently this depends on the mag setting in the
 * style for the column but we could allow the user to switch this on and off as well. */
void zmapWindowColumnSetMagState(ZMapWindow window, FooCanvasGroup *col_group)
{
  ZMapWindowContainerFeatureSet container;
  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group)) ;

  container = (ZMapWindowContainerFeatureSet)col_group;
  /* Only check the mag factor if the column is visible. (wrong) */

  /*
   * This test needs to be thought about.  We can't test if a column
   * is visible as it'll never get shown on zoom in. We don't want to
   * mess with the columns that have been hidden by the user though
   * (as happens now). I'm not sure we have a record of this.
   */

  if (zmapWindowContainerFeatureSetGetDisplay(container) == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE
      && zmapWindowColumnIs3frameVisible(window, col_group))
    {
      gboolean visible_at_this_mag = FALSE;

      visible_at_this_mag = zmapWindowColumnIsMagVisible(window, col_group);

      if(visible_at_this_mag)
	{
#ifdef DEBUG_COLUMN_MAG
	  printf("Column '%s' being shown\n", g_quark_to_string(container->unique_id));
#endif /* DEBUG_COLUMN_MAG */

	  zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP( container ), TRUE) ;
	}
      else
	{
#ifdef DEBUG_COLUMN_MAG
	  printf("Column '%s' being hidden\n", g_quark_to_string(container->unique_id));
#endif /* DEBUG_COLUMN_MAG */

	  zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP( container ), FALSE) ;
	}
    }

  return ;
}


/* checks to see if a column is 3 frame visible. */
/* NOTE (mh17) this function name is a misnomer, it means 'ShouldBe3FrameVisible'
 * if you want to know what the visibility state really is try group->flags.visible
 */
gboolean zmapWindowColumnIs3frameVisible(ZMapWindow window, FooCanvasGroup *col_group)
{
  ZMapWindowContainerFeatureSet container;
  ZMapStyle3FrameMode frame_mode;
  ZMapFrame set_frame;
  ZMapStrand set_strand;
  gboolean frame_specific, visible;
  int forward[ZMAPFRAME_2 + 1], reverse[ZMAPFRAME_2 + 1], none[ZMAPFRAME_2 + 1];
  int *frame_sens[ZMAPSTRAND_REVERSE + 1];

  zMapAssert(window);
  zMapAssert(ZMAP_IS_CONTAINER_FEATURESET(col_group));

  container = (ZMapWindowContainerFeatureSet)col_group;

  frame_specific = zmapWindowContainerFeatureSetIsFrameSpecific(container, &frame_mode);

  set_strand = zmapWindowContainerFeatureSetGetStrand(container);
  set_frame  = zmapWindowContainerFeatureSetGetFrame(container);


  if (frame_specific)
    {
      gboolean display, show_on_reverse;

      display = IS_3FRAME(window->display_3_frame) ;
      show_on_reverse = window->show_3_frame_reverse ;

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
	case ZMAPSTYLE_3_FRAME_AS_WELL:
	case ZMAPSTYLE_3_FRAME_ONLY_3:
	case ZMAPSTYLE_3_FRAME_ONLY_1:
//	  set_strand = zmapWindowContainerFeatureSetGetStrand(container);

	  visible = frame_sens[set_strand][set_frame];
	  if(frame_mode == ZMAPSTYLE_3_FRAME_ONLY_1)
	    {
	      if(set_frame == ZMAPFRAME_NONE)
		visible = !visible;
	      else
		visible = FALSE;
	    }
	  else if(frame_mode == ZMAPSTYLE_3_FRAME_ONLY_3 &&
		  set_frame == ZMAPFRAME_NONE)
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
  else if(set_frame == ZMAPFRAME_NONE)
    {
      visible = TRUE;
    }
  else
    {
      visible = FALSE;
    }

  return visible ;
}



/* checks to see if a column has been set by user to be displayed in 3 frame mode. */
gboolean zmapWindowColumnIs3frameDisplayed(ZMapWindow window, FooCanvasGroup *col_group)
{
  gboolean displayed = FALSE ;
  ZMapWindowContainerFeatureSet container;
  ZMapStyle3FrameMode frame_mode;
  gboolean frame_specific ;
/*  ZMapFeatureSet feature_set ;*/

  zMapAssert(window) ;
  zMapAssert(ZMAP_IS_CONTAINER_FEATURESET(col_group)) ;

//  if(!IS_3FRAME(window->display_3_frame))
//    return displayed;

  container = (ZMapWindowContainerFeatureSet)col_group ;

  frame_specific = zmapWindowContainerFeatureSetIsFrameSpecific(container, &frame_mode) ;

  if (frame_specific)
    {
#if MH17_NOT_USED
      ZMapFrame set_frame;
      ZMapStrand set_strand;

      set_strand = zmapWindowContainerFeatureSetGetStrand(container) ;
      set_frame  = zmapWindowContainerFeatureSetGetFrame(container) ;
#endif


/* MH17: acedb only gives us lower cased names
   previously capitalised name came from req_featuresets ???
      if(container->unique_id == zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME))
   but we can patch up from the [ZMap] columns list
*/
      if(container->original_id == g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME))
	{
	  if (IS_3FRAME_TRANS(window->display_3_frame))
	    displayed = TRUE ;
	}
      else if (IS_3FRAME_COLS(window->display_3_frame))
	{
	  displayed = TRUE ;
	}
    }

  return displayed ;
}


/* Checks to see if column would be visible according to current mag state.
 */
gboolean zmapWindowColumnIsMagVisible(ZMapWindow window, FooCanvasGroup *col_group)
{
  gboolean visible = TRUE ;
  ZMapWindowContainerGroup container = (ZMapWindowContainerGroup)col_group;
  ZMapWindowContainerFeatureSet featureset = (ZMapWindowContainerFeatureSet)col_group;

  zMapAssert(window && FOO_IS_CANVAS_GROUP(col_group)) ;


  if ((visible = (zmapWindowContainerHasFeatures(container) || zmapWindowContainerFeatureSetShowWhenEmpty(featureset))))
    {
      double min_mag = 0.0, max_mag = 0.0 ;
      double curr_bases ;

      curr_bases = zMapWindowGetZoomMagAsBases(window) ;

      if (zmapWindowContainerFeatureSetGetMagValues(featureset, &min_mag, &max_mag))
	{
	  if ((min_mag && curr_bases < min_mag)
	      || (max_mag && curr_bases > max_mag))
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
  ZMapWindowContainerGroup column_container, block_container;
  FooCanvasGroup *column_group =  NULL;
  GList *compressed, *bumped;
  VisCoordsStruct coords ;
  double wx1, wy1, wx2, wy2 ;

  column_container = zmapWindowContainerCanvasItemGetContainer(column_item);
  if(column_container)
    column_group = (FooCanvasGroup *)column_container;
  else
    zMapAssertNotReached() ;

  block_container = zmapWindowContainerUtilsGetParentLevel(column_container, ZMAPCONTAINER_LEVEL_BLOCK) ;

  compressed = zmapWindowContainerBlockRemoveCompressedColumns((ZMapWindowContainerBlock)block_container);
  bumped     = zmapWindowContainerBlockRemoveBumpedColumns((ZMapWindowContainerBlock)block_container);

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
      coords.block_group   = (ZMapWindowContainerBlock)block_container;
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

void zmapWindowreDrawContainerExecute(ZMapWindow                 window,
				      ZMapContainerUtilsExecFunc enter_cb,
				      gpointer                   enter_data)
{
  zMapPrintTimer(NULL, "About to do some work - including a reposition") ;
  window->interrupt_expose = TRUE ;

  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_FEATURESET,
				  enter_cb,
				  enter_data);

  zmapWindowContainerRequestReposition(window->feature_root_group);

  window->interrupt_expose = FALSE ;
  zMapPrintTimer(NULL, "Finished the work - including a reposition") ;

  return ;
}

void zmapWindowDrawManageWindowWidth(ZMapWindow window)
{
  if(window->feature_root_group)
    zmapWindowContainerGroupAddUpdateHook(window->feature_root_group, resetWindowWidthCB, window);

  return;
}


/* Makes sure all the things that need to be redrawn when the canvas needs redrawing. */
void zmapWindowFullReposition(ZMapWindow window)
{
  /* Is this enough or do we need to foo_canvas_update_now() */
  zmapWindowContainerRequestReposition(window->feature_root_group);

#ifdef REWRITE_THIS
  FooCanvasGroup *super_root ;
  ContainerType type = CONTAINER_INVALID ;

  zMapPrintTimer(NULL, "About to resposition") ;



  super_root = FOO_CANVAS_GROUP(zmapWindowFToIFindItemFull(window,window->context_to_item,
                                             0,0,0,
							   ZMAPSTRAND_NONE, ZMAPFRAME_NONE,
							   0)) ;
  zMapAssert(super_root) ;

  type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(super_root), CONTAINER_TYPE_KEY)) ;
  zMapAssert(type = CONTAINER_ROOT) ;



  /* This could probably call the col order stuff as pre recurse function... */
  zmapWindowContainerUtilsExecuteFull(FOO_CANVAS_GROUP(super_root),
				      ZMAPCONTAINER_LEVEL_FEATURESET,
				      NULL,
				      NULL,
				      resetWindowWidthCB,
				      window, TRUE) ;


  zmapWindowReFocusHighlights(window);

  zMapPrintTimer(NULL, "Finished resposition") ;
#endif /* REWRITE_THIS */
  return ;
}


/* Makes sure all the things that need to be redrawn for zooming get redrawn. */
void zmapWindowDrawZoom(ZMapWindow window)
{
  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_FEATURESET,
				  preZoomCB,
				  window);

  zmapWindowContainerRequestReposition(window->feature_root_group);

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
			      &context_cp, &diff,NULL);

      canvas_data.window = window;
      canvas_data.full_context = window->strand_separator_context;
      canvas_data.styles = window->context_map->styles ;

      drawSeparatorFeatures(&canvas_data, window->strand_separator_context) ;

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




/* Translate window 3 frame requests into 3 frame state.
 * ... could do this more simply by using the flag defines explicitly
 */
static void set3FrameState(ZMapWindow window, ZMapWindow3FrameMode frame_mode)
{
  switch (frame_mode)
    {
    case ZMAP_WINDOW_3FRAME_INVALID:
      {
      ZMAP_FLAG_OFF(window->display_3_frame, DISPLAY_3FRAME_TRANS) ;    /* comment out to remember selection */
      ZMAP_FLAG_OFF(window->display_3_frame, DISPLAY_3FRAME_COLS) ;

      ZMAP_FLAG_OFF(window->display_3_frame, DISPLAY_3FRAME_ON) ;
	break ;
      }
    case ZMAP_WINDOW_3FRAME_TRANS:
      {
      ZMAP_FLAG_OFF(window->display_3_frame, DISPLAY_3FRAME_COLS) ;
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_TRANS) ;
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_ON) ;
	break ;
      }
    case ZMAP_WINDOW_3FRAME_COLS:
      {
      ZMAP_FLAG_OFF(window->display_3_frame, DISPLAY_3FRAME_TRANS) ;
	ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_COLS) ;
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_ON) ;
	break ;
      }
    case ZMAP_WINDOW_3FRAME_ALL:
      {
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_TRANS) ;
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_COLS) ;
      ZMAP_FLAG_ON(window->display_3_frame, DISPLAY_3FRAME_ON) ;
	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
      }
    }

  return ;
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
static void preZoomCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                      ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ZMapWindowContainerUnderlay container_underlay;
	ZMapWindow window = (ZMapWindow)user_data;

	container_underlay = zmapWindowContainerGetUnderlay(container);
	//zmapWindowContainerPurge(container_underlay);

	if (zmapWindowContainerFeatureSetGetDisplay((ZMapWindowContainerFeatureSet)container) == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE)
	  zmapWindowColumnSetMagState(window, (FooCanvasGroup *)container) ;

      }
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
			    &box->coords[2], &box->coords[3],"set_hlocked_scroll_region") ;

  return ;
}

/* A version of zmapWindowResetWidth which uses the points from the recursion to set the width */
static gboolean resetWindowWidthCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
				   ZMapContainerLevelType level, gpointer user_data)
{
  ZMapWindow window = NULL;
  double x1, x2, y1, y2 ;       /* scroll region positions */
  double scr_reg_width, root_width ;
  gboolean result = TRUE;

  window = (ZMapWindow)user_data ;

  zMapAssert(level == ZMAPCONTAINER_LEVEL_ROOT);

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
      if(y2 == ZMAP_CANVAS_INIT_SIZE)
	y2 = window->max_coord;

      zmapWindowSetScrollRegion(window, &x1, &y1, &x2, &y2,"resetWindowWidthCB 1") ;
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

      zmapWindowSetScrollRegion(window, &x1, &y1, &x2, &y2,"resetWindowWidthCB 2") ;

      /* We need to make the horizontal split & locked windows have
       * the maximum width so that _all_ the features are
       * accessible. Test: bump a column, split window, bump column
       * another few columns and check the windows are scroolable
       * to the full extent of columns. */
      g_hash_table_foreach(window->sibling_locked_windows,
			   set_hlocked_scroll_region, box);

      foo_canvas_points_free(box);
    }

  return result;
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


static void toggleColumnInMultipleBlocks(ZMapWindow window, char *name,
					 GQuark align_id, GQuark block_id,
					 gboolean force_to, gboolean force)
{
  GList *blocks = NULL;
  const char *wildcard = "*";
  GQuark featureset_unique  = 0;
  ZMapStyleColumnDisplayState show_hide_state ;

  featureset_unique = zMapStyleCreateID(name);

  if (align_id == 0)
    align_id = g_quark_from_string(wildcard);
  if (block_id == 0)
    block_id = g_quark_from_string(wildcard);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#warning "Test doesn't find 3 frame column, work out why after annotation test."

  /* check we have the style... */
  if (!(zmapWindowFToIFindItemFull(window,window->context_to_item,
                           align_id, block_id,
				   featureset_unique,   /* this is column really but the name is the same */
				   ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE, 0)))
    {
      zMapWarning("Column with name \"%s\" does not exist."
                  " Check that you specified \"%s\" in your ZMap config file.",
		  name, name) ;
    }
  else
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    {
      FooCanvasItem *frame_column ;

      blocks = zmapWindowFToIFindItemSetFull(window,window->context_to_item,
					     align_id, block_id, 0, 0,
					     NULL, NULL, 0, NULL, NULL) ;

      /* Foreach of the blocks, toggle the display of the DNA */
      while (blocks)                 /* I cant bear to create ANOTHER struct! */
	{
	  ZMapFeatureBlock feature_block = NULL ;
	  ID2Canvas id2c;

	  int first, last, i ;

	  id2c = (ID2Canvas) blocks->data;
	  feature_block = (ZMapFeatureBlock) id2c->feature_any;	//zmapWindowItemGetFeatureBlock(blocks->data) ;

	  if (g_ascii_strcasecmp(name, ZMAP_FIXED_STYLE_3FT_NAME) == 0)
	    {
	      if (IS_3FRAME(window->display_3_frame))
		{
		  first = ZMAPFRAME_0 ;
		  last = ZMAPFRAME_2 ;
		}
	      else
		{
		  first = last = ZMAPFRAME_NONE ;
		}
	    }
	  else
	    {
	      first = last = ZMAPFRAME_NONE ;
	    }

	  for (i = first ; i <= last ; i++)
	    {
	      ZMapFrame frame = (ZMapFrame)i ;

	      frame_column = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                          feature_block->parent->unique_id,
							feature_block->unique_id,
							featureset_unique,
							ZMAPSTRAND_FORWARD, frame, 0) ;

	      if (frame_column && ZMAP_IS_CONTAINER_FEATURESET(frame_column)
		  && zmapWindowContainerHasFeatures((ZMapWindowContainerGroup)(frame_column)))
		{
		  if (force && force_to)
		    show_hide_state = ZMAPSTYLE_COLDISPLAY_SHOW ;
		  else if (force && !force_to)
		    show_hide_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
		  else if (zmapWindowItemIsShown(FOO_CANVAS_ITEM(frame_column)))
		    show_hide_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
		  else
		    show_hide_state = ZMAPSTYLE_COLDISPLAY_SHOW ;

		  zmapWindowColumnSetState(window,
					   FOO_CANVAS_GROUP(frame_column),
					   show_hide_state,
					   FALSE) ;
		}
	    }

	  blocks = blocks->next ;
	}
    }

  return ;
}



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

  any_feature = zmapWindowItemGetFeatureAny(container);

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
static void hideColsCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
		       ZMapContainerLevelType level, gpointer user_data)
{
  VisCoords coord_data = (VisCoords)user_data ;

  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	FooCanvasGroup *features ;

	coord_data->in_view = FALSE ;
	coord_data->left = 999999999 ;			    /* Must be bigger than any feature position. */
	coord_data->right = 0 ;

	features = (FooCanvasGroup *)zmapWindowContainerGetFeatures(container) ;

	g_list_foreach(features->item_list, featureInViewCB, coord_data) ;

	if (zmapWindowItemIsShown(FOO_CANVAS_ITEM(container)))
	  {
	    if (!(coord_data->in_view)
		&& (coord_data->compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE
		    || zmapWindowContainerFeatureSetGetDisplay((ZMapWindowContainerFeatureSet)container) != ZMAPSTYLE_COLDISPLAY_SHOW))
	      {
		/* No items overlap with given area so hide the column completely. */
		zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP( container ), FALSE) ;

		zmapWindowContainerBlockAddCompressedColumn((ZMapWindowContainerBlock)(coord_data->block_group), (FooCanvasGroup *)container);
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

		    zmapWindowContainerBlockAddBumpedColumn((ZMapWindowContainerBlock)(coord_data->block_group), (FooCanvasGroup *)container);
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
  if ((feature = zmapWindowItemGetFeature(feature_item)))
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
  ZMapWindowContainerFeatureSet container;

  if(ZMAP_IS_CONTAINER_FEATURESET(col_group))
    {
      container = ZMAP_CONTAINER_FEATURESET(col_group);
      /* This is called from the Compress Columns code, which _is_ a user
       * action. */
      zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP( container ), TRUE) ;
    }

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
	FooCanvasItem  *align_hash_item;

	canvas_data->curr_alignment =
	  zMapFeatureContextGetAlignmentByID(canvas_data->full_context,
					     feature_any->unique_id);
	if((align_hash_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                           feature_any->unique_id,
							 0, 0, ZMAPSTRAND_NONE,
							 ZMAPFRAME_NONE, 0)))
	  {
	    ZMapWindowContainerGroup align_parent;

	    zMapAssert(ZMAP_IS_CONTAINER_GROUP(align_hash_item));

	    align_parent = (ZMapWindowContainerGroup)align_hash_item;

	    canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent);
	  }
	else
	  zMapAssertNotReached();
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	FooCanvasItem  *block_hash_item;
	canvas_data->curr_block =
	  zMapFeatureAlignmentGetBlockByID(canvas_data->curr_alignment,
					   feature_any->unique_id);

	if((block_hash_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                           canvas_data->curr_alignment->unique_id,
							 feature_any->unique_id, 0,
							 ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
	  {
	    ZMapWindowContainerGroup block_parent;
	    ZMapWindowContainerStrand forward_strand;

	    zMapAssert(ZMAP_IS_CONTAINER_GROUP(block_hash_item));

	    block_parent = (ZMapWindowContainerGroup)(block_hash_item);

	    canvas_data->curr_block_group =
	      zmapWindowContainerGetFeatures(block_parent);

	    forward_strand = zmapWindowContainerBlockGetContainerStrand((ZMapWindowContainerBlock)block_parent,
									ZMAPSTRAND_FORWARD);
	    canvas_data->curr_forward_group = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)forward_strand);
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
				     NULL, separator, ZMAPFRAME_NONE, FALSE) ;

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


