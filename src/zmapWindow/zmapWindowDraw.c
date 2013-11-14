/*  File: zmapWindowDraw.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: General drawing functions for zmap window, e.g.
 *              repositioning columns after one has been bumped
 *              or removed etc.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <math.h>
#include <glib.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsLogical.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowContainerBlock.h>
#include <zmapWindowCanvasSequence.h>
#include <zmapWindowContainerFeatureSet.h>


typedef struct
{
  ZMapWindow window ;
  double zoom ;

} ZoomDataStruct, *ZoomData ;


typedef struct execOnChildrenStructType
{
  ZMapContainerLevelType stop ;

  GFunc                  down_func_cb ;
  gpointer               down_func_data ;

  GFunc                  up_func_cb ;
  gpointer               up_func_data ;

} execOnChildrenStruct, *execOnChildren ;




typedef struct PositionColumnStructType
{
  double block_cur_x;					    /* x pos of current column */
  double block_spacing_x;				    /* as set in current block */
  double x1,x2;
  double left;						    /* left most coordinate of a column that has moved */
  double last_left;					    /* previous column */
  double last_right;

} PositionColumnStruct, *PositionColumn;


typedef struct VisCoordsStructType
{
  ZMapWindowContainerBlock block_group;
  int start, end ;
  gboolean in_view ;
  double left, right ;
  GList *columns_hidden ;
  ZMapWindowCompressMode compress_mode ;
} VisCoordsStruct, *VisCoords ;



static void toggleColumnInMultipleBlocks(ZMapWindow window, char *name,
					 GQuark align_id, GQuark block_id,
					 gboolean force_to, gboolean force) ;
static void preZoomCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                      ZMapContainerLevelType level, gpointer user_data) ;
static void positionColumnCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
			     ZMapContainerLevelType level, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printChild(gpointer data, gpointer user_data) ;
static void printQuarks(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void hideColsCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
		       ZMapContainerLevelType level, gpointer user_data) ;
static void featureInViewCB(void *data, void *user_data) ;
static void showColsCB(void *data, void *user_data) ;
static void rebumpColsCB(void *data, void *user_data) ;

static void set3FrameState(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ; ;
static void myWindowSet3FrameMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode) ;

static gboolean zMapWindowResetWindowWidth(ZMapWindow window, FooCanvasItem *item, double x1, double x2);





/* 
 *                       External Routines
 */


/* Turn on/off 3 frame cols. */
void zMapWindow3FrameToggle(ZMapWindow window)
{
  ZMapWindow3FrameMode mode = ZMAP_WINDOW_3FRAME_TRANS ;

  if (IS_3FRAME(window->display_3_frame))
    mode = ZMAP_WINDOW_3FRAME_INVALID ;

  myWindowSet3FrameMode(window, mode) ;

  return ;
}



/* Turn on/off 3 frame cols. */
void zMapWindow3FrameSetMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode)
{
  myWindowSet3FrameMode(window, frame_mode) ;

  return ;
}


void zMapWindowToggleDNAProteinColumns(ZMapWindow window,
                                       GQuark align_id,   GQuark block_id,
                                       gboolean dna,      gboolean protein, gboolean trans,
                                       gboolean force_to, gboolean force)
{
  zmapWindowBusy(window, TRUE) ;

  if (dna)
    toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_DNA_NAME,
				 align_id, block_id, force_to, force);

  if (protein)
    toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_3FT_NAME,
				 align_id, block_id, force_to, force);

  if (trans)
    toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME,
				 align_id, block_id, force_to, force) ;

  zmapWindowColOrderColumns(window);

  zmapWindowFullReposition(window->feature_root_group,TRUE, "toggle columns") ;

  zmapWindowBusy(window, FALSE) ;

  return ;
}


void zMapWindowToggleScratchColumn(ZMapWindow window,
                                   GQuark align_id,   GQuark block_id,
                                   gboolean force_to, gboolean force)
{
  zmapWindowBusy(window, TRUE) ;

  toggleColumnInMultipleBlocks(window, ZMAP_FIXED_STYLE_SCRATCH_NAME,
                               align_id, block_id, force_to, force);

  zmapWindowFullReposition(window->feature_root_group, TRUE, "toggle columns") ;

  zmapWindowBusy(window, FALSE) ;

  return ;
}


/* to resize ourselves and reposition stuff to the right of a CanvasFeatureset we have to resize the root */
/* There's a lot of container code that labouriously trundles up the tree, but each canvas item knows the root. so let's use that */
/* hmmmm... you need to call special invocations that set properties that then set flags... yet another run-around. */

/* after removing a fair bit of code you should now be able to call FullReposition from canvas->root */
/* this is called from canvasLocus & Sequence and ColBump */
void zMapWindowRequestReposition(FooCanvasItem *foo)
{
  ZMapWindowContainerGroup container;

  /* container and item code is separate despite all of them having parent pointers */
  container = zmapWindowContainerCanvasItemGetContainer(foo);

  container = zmapWindowContainerUtilsGetParentLevel(container, ZMAPCONTAINER_LEVEL_ROOT);

  zmapWindowFullReposition((ZMapWindowContainerGroup) container, FALSE, "request reposition");

  return ;
}




/* 
 *                          Package routines
 */


/*                some column functions....                              */

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

  if (!style || !style->displayable)
    return visible ;

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

  zMapLogTime(TIMER_SETVIS,TIMER_START,0,"");

  container = (ZMapWindowContainerFeatureSet)column_group;

  curr_col_state = zmapWindowContainerFeatureSetGetDisplay(container) ;
  //printf("set state %s\n",g_quark_to_string(container->unique_id));

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

      zmapWindowContainerFeatureSetSetDisplay(container, new_col_state, window) ;

      new_visible = zmapWindowGetColumnVisibility(window,column_group);
      /* state we want rather than what's current */

      //printf("set visible = %d -> %d\n",cur_visible, new_visible);

      if(new_visible)
	{
	  if(!cur_visible)
            {
	      redraw = TRUE;
	      zmapWindowContainerSetVisibility(column_group, TRUE) ;
            }
	}
      else
	{
	  if(cur_visible)
            {
	      redraw = TRUE;
	      zmapWindowContainerSetVisibility(column_group, FALSE) ;
            }
	}

      /* Only do redraw if it was requested _and_ state change needs it. */
      if (redraw_if_needed && redraw)
	{
	  zmapWindowFullReposition(window->feature_root_group,TRUE, "set state") ;
	}
    }
  zMapLogTime(TIMER_SETVIS,TIMER_STOP,0,"");
}




/* Set the hidden status of a column group, currently this depends on the mag setting in the
 * style for the column but we could allow the user to switch this on and off as well. */
void zmapWindowColumnSetMagState(ZMapWindow window, FooCanvasGroup *col_group)
{
  ZMapWindowContainerFeatureSet container;
  if (!window || !FOO_IS_CANVAS_GROUP(col_group)) 
    return ;

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
  gboolean frame_specific, visible = FALSE ;
  int forward[ZMAPFRAME_2 + 1], reverse[ZMAPFRAME_2 + 1], none[ZMAPFRAME_2 + 1];
  int *frame_sens[ZMAPSTRAND_REVERSE + 1];

  if (!window || !ZMAP_IS_CONTAINER_FEATURESET(col_group))
    return visible ;

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

  if (!window || !ZMAP_IS_CONTAINER_FEATURESET(col_group)) 
    return displayed;

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
      if(zmapWindowContainerFeatureSetColumnDisplayName(container) == g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME))
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
  
  if (ZMAP_IS_CONTAINER_FEATURESET(col_group))
    {
      ZMapWindowContainerGroup container = ZMAP_CONTAINER_GROUP(col_group);
      ZMapWindowContainerFeatureSet featureset = ZMAP_CONTAINER_FEATURESET(col_group);

      if(window && FOO_IS_CANVAS_GROUP(col_group))
        {
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
  if (!column_group || !FOO_IS_CANVAS_GROUP(column_group)) 
    return ;

  zmapWindowContainerSetVisibility(column_group, FALSE) ;

  return ;
}

void zmapWindowColumnShow(FooCanvasGroup *column_group)
{
  if (!column_group || !FOO_IS_CANVAS_GROUP(column_group)) 
    return ;

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
  bumped = zmapWindowContainerBlockRemoveBumpedColumns((ZMapWindowContainerBlock)block_container);

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

      zmapWindowFullReposition(window->feature_root_group,TRUE, "compress 1") ;
    }
  else
    {
      coords.block_group   = (ZMapWindowContainerBlock)block_container;
      coords.compress_mode = compress_mode ;

      /* If there is no mark or user asked for visible area only then do that. */
      if (compress_mode == ZMAPWINDOW_COMPRESS_VISIBLE)
	{
	  zmapWindowItemGetVisibleWorld(window, &wx1, &wy1, &wx2, &wy2) ;

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

      zmapWindowContainerUtilsExecute(window->feature_root_group,
				      ZMAPCONTAINER_LEVEL_FEATURESET,
				      hideColsCB, &coords);

      zmapWindowFullReposition(window->feature_root_group,TRUE, "compress 2");

    }

  return ;
}


/* Makes sure all the things that need to be redrawn when the canvas needs redrawing. */
void zmapWindowFullReposition(ZMapWindowContainerGroup root, gboolean redraw, char *where)
{
  PositionColumn poscol = g_new0(PositionColumnStruct,1);
  ZMapWindow window = g_object_get_data(G_OBJECT(root), ZMAP_WINDOW_POINTER);

  /* scan canvas and move columns sideways */
  //printf("full repos %s %p %d\n", where, window, redraw);

  zmapWindowContainerUtilsExecuteFull(root,
				      ZMAPCONTAINER_LEVEL_FEATURESET,
				      positionColumnCB, poscol,
				      NULL, NULL) ;

  if(window)	/* if we get called from the navigator there is no window */
    {
      zMapWindowResetWindowWidth(window,(FooCanvasItem *) root, poscol->x1, poscol->x2);

      if(redraw)
	{
	  /*
	   * don-t refresh the whole screen for a column added on the right
	   * this would be bad form and besides give a 10:1 performance hit or worse for 3FT toggle
	   * and may cause horrid flickering on column load (there may be another cause of this... *.
	   */

	  double x1,x2,y1,y2;
	  int cx1,cx2,cy1,cy2;

	  zmapWindowGetScrollableArea(window, &x1, &y1, &x2, &y2);
	  foo_canvas_w2c(((FooCanvasItem *) root)->canvas, poscol->left, y1, &cx1, &cy1);
	  foo_canvas_w2c(((FooCanvasItem *) root)->canvas, x2, y2, &cx2, &cy2);

	  foo_canvas_request_redraw(((FooCanvasItem *) root)->canvas, cx1, cy1, cx2, cy2);
	  //printf("full repos request redraw %d %d - %d %d\n",cx1, cy1, cx2, cy2);
	}
    }

  g_free(poscol);
  return ;
}


/* Makes sure all the things that need to be redrawn for zooming get redrawn. */
void zmapWindowDrawZoom(ZMapWindow window)
{
  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_FEATURESET,
				  preZoomCB,
				  window) ;

  zmapWindowFullReposition(window->feature_root_group,TRUE, "draw zoom") ;

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


  if(!style)
    {
      zMapLogWarning("Feature set '%s' has no style!", g_quark_to_string(feature_set->original_id));
    }
  else
    {
      ZMapCanvasDataStruct canvas_data = {NULL};
      FooCanvasItem *foo;

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

      /* Now make a tree */
      zMapFeatureContextAddAlignment(context_cp, align_cp, context->master_align == align);
      zMapFeatureAlignmentAddBlock(align_cp, block_cp);
      zMapFeatureBlockAddFeatureSet(block_cp, feature_set);

      /* Now we have a context to merge. */
      zMapFeatureContextMerge(&(window->strand_separator_context),
			      &context_cp, &diff,NULL);

      canvas_data.window = window;
      canvas_data.canvas = window->canvas;

      /* let's just use the code that we debugged for the normal columns */
      canvas_data.curr_root_group = zmapWindowContainerGetFeatures(window->feature_root_group) ;
      zMapWindowDrawContext(&canvas_data, window->strand_separator_context, diff, NULL);

      foo = zmapWindowFToIFindSetItem(window,window->context_to_item,
				      feature_set,
				      ZMAPSTRAND_FORWARD, ZMAPFRAME_NONE);

      foo_canvas_item_request_redraw(foo->parent);	/* will have been updated so coords work */
    }

  return;
}







/* 
 *                   Internal routines
 */


/* Sets the display of the Reading Frame columns, these columns show frame senstive
 * features in separate sets of columns according to their reading frame.
 *
 *  */
static void myWindowSet3FrameMode(ZMapWindow window, ZMapWindow3FrameMode frame_mode)
{
  gpointer three_frame_id = NULL;
  gpointer three_frame_Id = NULL;   // capitalised!

  zmapWindowBusy(window, TRUE) ;


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

      /* Set up the frame state. */
      set3FrameState(window, frame_mode) ;

      // draw always columns or 3-frame depending on mode
      zmapWindowDraw3FrameFeatures(window);

      /* Now we've drawn all the features we can position them all. */
      zmapWindowColOrderColumns(window);

      /* FullReposition Sets the correct scroll region. */
      zmapWindowFullReposition(window->feature_root_group, TRUE, "set 3 frame");
    }
  else
    {
      zMapWarning("%s", "No '" ZMAP_FIXED_STYLE_3FRAME "' column in config file.");
    }


  zmapWindowBusy(window, FALSE) ;

  return ;
}

static void positionColumnCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
			     ZMapContainerLevelType level, gpointer user_data)
{
  PositionColumn pc = (PositionColumn) user_data;
  FooCanvasGroup *group = (FooCanvasGroup *) container;
  GList *l ;
  FooCanvasItem *foo ;
  ZMapWindowFeaturesetItem cfs = NULL;
  ZMapWindowContainerBlock block;
  ZMapWindowContainerFeatureSet featureset;
  double col_width, width;
  gboolean debug = FALSE ;


  /* columns start at 0, container backgrounds can go -ve */
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_ROOT:
      pc->left = pc->x1 = pc->block_cur_x = 0 ;
      /* fall through */

    case ZMAPCONTAINER_LEVEL_ALIGN:
      pc->x1 -= container->child_spacing ;
      break ;

    case ZMAPCONTAINER_LEVEL_BLOCK:
      block = (ZMapWindowContainerBlock) container ;
      pc->block_spacing_x = container->child_spacing ;
      pc->x1 -= container->child_spacing ;
      pc->last_left = pc->last_right = 0.0 ;
      break ;

    case ZMAPCONTAINER_LEVEL_FEATURESET:
      /* each column is a ContainerFeatureSet is a foo canvas group and contains 0 or more Canvasfeaturesets */

      zMapDebugPrint(debug, "About to do pos col \"%s\"\n", g_quark_to_string((container->feature_any->unique_id))) ;

      foo = (FooCanvasItem *) container;

      if (!(foo->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	break;

      featureset = (ZMapWindowContainerFeatureSet) container;
      col_width = 0.0;
      cfs = NULL;

      pc->block_cur_x += pc->block_spacing_x;	/* spacer before column */

      for (l = group->item_list; l ; l = l->next)
	{
	  foo = (FooCanvasItem *) l->data;

	  if(!(foo->object.flags & FOO_CANVAS_ITEM_VISIBLE))
	    {
	      continue;
	    }

	  if (ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))
	    {
	      gint layer;

	      cfs = (ZMapWindowFeaturesetItem) foo;
	      layer = zMapWindowCanvasFeaturesetGetLayer(cfs);

	      if( !(layer & ZMAP_CANVAS_LAYER_DECORATION) || !(layer & ZMAP_CANVAS_LAYER_STRETCH_X))
		{
		  width = zMapWindowCanvasFeaturesetGetWidth(cfs);
		  width += zMapWindowCanvasFeaturesetGetOffset(cfs);

		  if(width > col_width)
		    col_width = width;
		}
	    }
	}

      if (!pc->left && pc->last_right > group->xpos)	/* previous column got bigger, must refresh that one */
	pc->left = pc->last_left ;

      pc->last_left = pc->block_cur_x ;

      if (group->xpos != pc->block_cur_x)	/* if new col xpos will be 0 */
	{
	  if(!pc->left)
	    pc->left = pc->last_left;	//pc->block_cur_x;

	  zMapDebugPrint(debug, "pos col %s %f %f %f\n", g_quark_to_string((container->feature_any->unique_id)),
			 pc->block_cur_x, pc->block_spacing_x, width) ;
	}

      g_object_set(G_OBJECT(group), "x",pc->block_cur_x, NULL);	/* this sets deep update flags */

      pc->block_cur_x += col_width;
      pc->last_right = pc->block_cur_x;
      zMapDebugPrint(debug, "pos col %s %f %f %f %f %f\n", g_quark_to_string((container->feature_any->unique_id)),
		     pc->block_cur_x, pc->block_spacing_x, width, pc->last_left, pc->last_right) ;

      break;

    default:
      break;
    }

  pc->x2 = pc->block_cur_x + pc->block_spacing_x - pc->x1; 	/* x1 is -ve, block x1 is 0 */

  return ;
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



/* GFunc called for all container groups (but some don't have functions for
 * prezoom etc) to prepare for zooming.
 */
static void preZoomCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                      ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
	ZMapWindow window = (ZMapWindow)user_data;
	ZMapWindowFeaturesetItem cfs = NULL ;
	FooCanvasItem *item = (FooCanvasItem *)container ;

	if (zmapWindowContainerFeatureSetGetDisplay((ZMapWindowContainerFeatureSet)container)
	    == ZMAPSTYLE_COLDISPLAY_SHOW_HIDE)
	  zmapWindowColumnSetMagState(window, (FooCanvasGroup *)container) ;

	/* Get the feature set item and set the canvas zoom and call any col. functions
	 * needed to prepare for zooming (e.g. to set text width etc. */
	if ((cfs = ZMapWindowContainerGetFeatureSetItem(container)))
	  {
	    zMapWindowCanvasFeaturesetSetZoomY(cfs, item->canvas->pixels_per_unit_y) ;

	    zMapWindowCanvasFeaturesetPreZoom(cfs) ;
	  }

	break ;
      }

    default:
      break ;
    }

  return ;
}




static gboolean zMapWindowResetWindowWidth(ZMapWindow window, FooCanvasItem *item, double x1, double x2)
{
  double y1, y2 ;       /* scroll region positions */
  gboolean result = TRUE;

  zmapWindowGetScrollableArea(window, NULL, &y1, NULL, &y2);

  /* Annoyingly the initial size of the canvas is an issue here on first draw */
  if(y2 == ZMAP_CANVAS_INIT_SIZE)
    {
      y1 = window->min_coord;		/* rev comp changes both of these */
      y2 = window->max_coord;
    }
  zmapWindowSetScrollableArea(window, &x1, &y1, &x2, &y2,"resetWindowWidth") ;

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
  GQuark featureset_unique ;
  const char *wildcard = "*" ;


  featureset_unique = zMapStyleCreateID(name) ;

  if (align_id == 0)
    align_id = g_quark_from_string(wildcard) ;
  if (block_id == 0)
    block_id = g_quark_from_string(wildcard) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /*! \todo #warning "Test doesn't find 3 frame column, work out why after annotation test." */

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
      GList *blocks ;

      blocks = zmapWindowFToIFindItemSetFull(window,window->context_to_item,
					     align_id, block_id, 0, 0,
					     NULL, NULL, 0, NULL, NULL) ;

      /* Foreach of the blocks, toggle the display of the DNA */
      while (blocks)                 /* I cant bear to create ANOTHER struct! */
	{
	  ID2Canvas id2c;
	  ZMapFeatureBlock feature_block ;
	  int first, last, i ;

	  id2c = (ID2Canvas) blocks->data;
	  feature_block = (ZMapFeatureBlock)(id2c->feature_any) ;

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
	      FooCanvasItem *frame_column ;
	      ZMapFrame frame = (ZMapFrame)i ;

	      frame_column = zmapWindowFToIFindItemColumn(window, window->context_to_item,
							  feature_block->parent->unique_id,
					  		  feature_block->unique_id,
							  featureset_unique,
                                                          // DNA is reverse but ST is fwd when revcomped
                                                          // this is very tedious
                                                          //							window->flags[ZMAPFLAG_REVCOMPED_FEATURES] ? ZMAPSTRAND_REVERSE: ZMAPSTRAND_FORWARD,
							  ZMAPSTRAND_FORWARD,
							  frame) ;


	      if (frame_column && ZMAP_IS_CONTAINER_FEATURESET(frame_column) &&
                  (zmapWindowContainerHasFeatures(ZMAP_CONTAINER_GROUP(frame_column)) ||
                   zmapWindowContainerFeatureSetShowWhenEmpty(ZMAP_CONTAINER_FEATURESET(frame_column))))
		{
                  ZMapStyleColumnDisplayState show_hide_state ;
                  
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



