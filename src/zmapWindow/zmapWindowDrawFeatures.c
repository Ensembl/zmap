/*  File: zmapWindowDrawFeatures.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Draws genomic features in the data display window.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: Jul 29 11:28 2010 (edgrif)
 * Created: Thu Jul 29 10:45:00 2004 (rnc)
 * CVS info:   $Id: zmapWindowDrawFeatures.c,v 1.293 2010-09-10 18:22:47 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>

#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
#include <zmapWindowItemFactory.h>
#include <zmapWindowContainerFeatureSet_I.h>

#include <ZMap/zmapGFF.h>     // for GFFSet struct for column mapping

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
  GHashTable *styles ;
  ZMapFeatureAlignment curr_alignment ;
  ZMapFeatureBlock curr_block ;
  ZMapFeatureSet curr_set ;

  /* THESE ARE REALLY NEEDED TO POSITION THE ALIGNMENTS FOR WHICH WE CURRENTLY HAVE NO
   * ORDERING/PLACEMENT MECHANISM.... */
  /* Records current positional information. */
  double curr_x_offset ;
//  double curr_y_offset ;


  /* Records current canvas item groups, these are the direct parent groups of the display
   * types they contain, e.g. curr_root_group is the parent of the align */
  ZMapWindowContainerFeatures curr_root_group ;
  ZMapWindowContainerFeatures curr_align_group ;
  ZMapWindowContainerFeatures curr_block_group ;
  ZMapWindowContainerFeatures curr_forward_group ;
  ZMapWindowContainerFeatures curr_reverse_group ;

  ZMapWindowContainerFeatures curr_forward_col ;
  ZMapWindowContainerFeatures curr_reverse_col ;


  GHashTable *feature_hash ;

  GList *masked;

  gboolean    frame_mode_change ;
  ZMapFrame   current_frame;

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
  GHashTable *styles ;
/*  GHashTable *feature_hash ;   not used */
  GList *feature_list;
  int feature_count;
  ZMapWindowContainerFeatures curr_forward_col ;
  ZMapWindowContainerFeatures curr_reverse_col ;
  ZMapFrame frame ;
  gboolean drawable_frame;	/* filter */
} CreateFeatureSetDataStruct, *CreateFeatureSetData ;


typedef struct RemoveFeatureDataStructName
{
  ZMapWindow window ;
  ZMapStrand strand ;
  ZMapFrame frame ;
  ZMapFeatureSet feature_set ;
} RemoveFeatureDataStruct, *RemoveFeatureData ;


static void windowDrawContext(ZMapCanvasData     canvas_data,
			       GHashTable             *styles,
			       ZMapFeatureContext full_context,
			       ZMapFeatureContext diff_context,
                         GList *masked);
static ZMapFeatureContextExecuteStatus windowDrawContextCB(GQuark   key_id,
							   gpointer data,
							   gpointer user_data,
							   char   **error_out) ;
static FooCanvasGroup *produce_column(ZMapCanvasData  canvas_data,
				      ZMapWindowContainerFeatures container,
				      GQuark          feature_set_id,
				      ZMapStrand      column_strand,
				      ZMapFrame       column_frame);


static FooCanvasGroup *createColumnFull(ZMapWindowContainerFeatures parent_group,
					ZMapWindow           window,
					ZMapFeatureAlignment align,
					ZMapFeatureBlock     block,
					ZMapFeatureSet       feature_set,
                              GQuark               display_id,
					GQuark               feature_set_unique_id,
					ZMapStrand           strand,
					ZMapFrame            frame,
					gboolean             is_separator_col,
					double width, double top, double bot);

static FooCanvasGroup *createColumn(ZMapWindowContainerFeatures parent_group,
				    ZMapWindow           window,
                                    ZMapFeatureSet       feature_set,
				    ZMapStrand           strand,
                                    ZMapFrame            frame,
				    gboolean             is_separator_col,
				    double width, double top, double bot);
static void ProcessFeature(gpointer key, gpointer data, gpointer user_data) ;
static void ProcessListFeature(gpointer data, gpointer user_data) ;

static void purge_hide_frame_specific_columns(ZMapWindowContainerGroup container, FooCanvasPoints *points,
					      ZMapContainerLevelType level, gpointer user_data) ;
static gboolean feature_set_matches_frame_drawing_mode(ZMapWindow     window,
						       ZMapCanvasData canvas_data,
						       ZMapFeatureSet feature_set,
						       int *frame_start_out,
						       int *frame_end_out) ;

static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean strandBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean containerDestroyCB(FooCanvasItem *item_in_hash, gpointer data) ;

static void removeEmptyColumnCB(gpointer data, gpointer user_data) ;

static ZMapGUIMenuItem makeMenuColumnOps(int *start_index_inout,
					    ZMapGUIMenuItemCallbackFunc callback_func,
					    gpointer callback_data) ;
static void columnMenuCB(int menu_item_id, gpointer callback_data) ;

static void setColours(ZMapWindow window) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static FooCanvasGroup *separatorGetFeatureSetColumn(ZMapWindowContainerStrand separator_parent,
						    ZMapFeatureSet  feature_set);

static gboolean contextGetMasterAlignSpan(ZMapFeatureContext context,int *start,int *end);



static void removeAllFeatures(ZMapWindow window, ZMapWindowContainerFeatureSet container_set) ;
static void removeFeatureCB(gpointer key, gpointer value, gpointer user_data) ;


/* Globals. */

extern GTimer *view_timer_G ;
static gboolean window_draw_context_debug_G = FALSE;



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
 * masked is a list of featureset id's that have been masked, old and new
 *
 * So NOTE that if no features have _yet_ been drawn then  full_context == diff_context
 *
 *  */
void zmapWindowDrawFeatures(ZMapWindow window, ZMapFeatureContext full_context,
      ZMapFeatureContext diff_context,GList *masked)
{
  GtkAdjustment *h_adj ;
  ZMapCanvasDataStruct canvas_data = {NULL} ;		    /* Rest of struct gets set to zero. */
  ZMapWindowContainerGroup root_group ;
  FooCanvasItem *tmp_item = NULL ;
  gboolean debug_containers = FALSE, root_created = FALSE ;
  double x, y ;
  int seq_start,seq_end ;

  zMapPrintTimer(NULL, "About to create canvas features") ;

  zMapAssert(window && full_context && diff_context) ;

  zMapWindowBusy(window, TRUE) ;

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

#ifdef SHOW_PARENT_SPAN
  //original code, window based from 1
  window->seq_start = full_context->sequence_to_parent.c1 ;
  window->seq_end   = full_context->sequence_to_parent.c2 ;
#else
  // we want to display just the data in our block, not the surrounding sequence
  contextGetMasterAlignSpan(full_context,&seq_start,&seq_end);
  window->seq_start = seq_start;
  window->seq_end = seq_end;
#endif

  window->seqLength = zmapWindowExt(window->seq_start, window->seq_end) ;


  /* Set the origin used for displaying coords....
   * which must refer to the parent span to allow reverse strand coords to be calculated.
   * The "+ 2" is because we have to go from sequence_start == 1 to sequence_start == -1
   * which is a shift of 2. */
  if (window->display_forward_coords)
    {
      if (window->revcomped_features)
	window->origin = full_context->sequence_to_parent.c2 + 2 ;
      else
	window->origin = 1;

      zmapWindowRulerCanvasSetOrigin(window->ruler, window->origin) ;
    }
#ifdef MH17_REVCOMP_DEBUG
  printf("drawFeatures window: %f-%f, %d, %d\n",window->seq_start,window->seq_end,window->origin,window->revcomped_features);
#endif
  zmapWindowZoomControlInitialise(window);		    /* Sets min/max/zf */


  /* HOPE THIS IS THE RIGHT PLACE TO SET ZOOM FOR LONG_ITEMS... */
  zmapWindowLongItemSetMaxZoom(window->long_items, zMapWindowGetZoomMax(window)) ;


  window->min_coord = window->seq_start;
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


  if((tmp_item = zmapWindowFToIFindItemFull(window->context_to_item,
                                            0, 0, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
    {
      zMapAssert(ZMAP_IS_CONTAINER_GROUP(tmp_item));
      root_group   = (ZMapWindowContainerGroup)tmp_item;
      root_created = FALSE;
    }
  else
    {
      double sx1,sy1,sx2,sy2;

/*
      MH17 -> RT162629
      on v-split when zoomed this causes the LH pane to have a full range scroll region
      meaning that the two widnows were not exact copies & they become unlocked
      we set the scroll region here as this is where we have the min/max coords

      this is flaky if we ever use y1 coords < 1
      Ideally the window should have a flag to say if it's been set
      We are using foo defaults as this flag which is dreadful hack
*/
      zmapWindowGetScrollRegion(window, &sx1, &sy1, &sx2, &sy2);
      if(sy1 < 1.0)     // foo canvas defaults to 0.0
      {
            sx1 = 0.0;
            sy1 = window->min_coord;
            sx2 = ZMAP_CANVAS_INIT_SIZE;
            sy2 = window->max_coord;
      }
      zmapWindowSetScrollRegion(window, &sx1, &sy1, &sx2, &sy2);


      /* Add a background to the root window, must be as long as entire sequence... */
      root_group = zmapWindowContainerGroupCreateFromFoo(foo_canvas_root(window->canvas),
							 ZMAPCONTAINER_LEVEL_ROOT,
							 window->config.align_spacing,
							 &(window->colour_root),
							 &(window->canvas_border));

      zmapWindowContainerGroupBackgroundSize(root_group, window->max_coord - window->min_coord);

      g_signal_connect(G_OBJECT(root_group), "destroy", G_CALLBACK(containerDestroyCB), window) ;

      root_created = TRUE;

      g_object_set_data(G_OBJECT(root_group), ITEM_FEATURE_STATS,
			zmapWindowStatsCreate((ZMapFeatureAny)full_context)) ;

      g_object_set_data(G_OBJECT(root_group), ZMAP_WINDOW_POINTER, window) ;
    }

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(root_group) ;

  /* Always reset this as context changes with new features.*/
  zmapWindowContainerAttachFeatureAny(root_group, (ZMapFeatureAny)full_context);

  zmapWindowFToIAddRoot(window->context_to_item, (FooCanvasGroup *)root_group);

  window->feature_root_group = root_group ;

  if(root_created)
    {
      zmapWindowDrawManageWindowWidth(window);
    }

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

  windowDrawContext(&canvas_data, window->display_styles,
		    full_context, diff_context, masked);


  /* Now we've drawn all the features we can position them all. */
  zmapWindowColOrderColumns(window);

#ifdef CONTAINER_REQUEST_REPOSITION_DOES_THE_SAME_THING
  /* FullReposition Sets the correct scroll region. */
  zmapWindowFullReposition(window);
#endif

#ifdef THIS_IS_DONE_WITH_THE_STATE_CODE_NOW
#ifdef FIX_RT_66294
  /* There may be a focus item if this routine is called as a result of splitting a window
   * or adding more features, make sure we scroll to the same point as we were
   * at in the previously. */
  if ((fresh_focus_item = zmapWindowFocusGetHotItem(window->focus)))
    {
      zMapWindowScrollToItem(window, fresh_focus_item) ;
    }
#endif /* FIX_RT_66294 */
#endif /* THIS_IS_DONE_WITH_THE_STATE_CODE_NOW */

  /* Update dependent windows...there is more work to do here.... */

  if (window->col_config_window)
    {
      zmapWindowColumnConfigure(window, NULL, ZMAPWINDOWCOLUMN_CONFIGURE_ALL) ;
    }

  if(debug_containers)
    {
      /* This code no longer works */
      /* zmapWindowContainerPrint(root_group) ; */
    }

  zMapPrintTimer(NULL, "Finished creating canvas features") ;

  zmapWindowContainerRequestReposition(root_group);

  zMapWindowBusy(window, FALSE) ;

  return ;
}


/* Makes a column for each style in the list, the columns are populated with features by the
 * ProcessFeatureSet() routine, at this stage the columns do not have any features and some
 * columns may end up not having any features at all.
 *
 * N.B. Test the return value as not all columns can be created (e.g. meta columns, those without
 * styles). This is actually vital now as styles will be tested to see if they have been
 * set correctly for display and no display will be done unless they are. The problem
 * will be logged but the caller should log this as well as its a serious data error.
 *
 *  */
gboolean zmapWindowCreateSetColumns(ZMapWindow window,
                                    ZMapWindowContainerFeatures forward_strand_group,
                                    ZMapWindowContainerFeatures reverse_strand_group,
                                    ZMapFeatureBlock block,
                                    ZMapFeatureSet feature_set,
				    GHashTable *styles,
                                    ZMapFrame frame,
                                    FooCanvasGroup **forward_col_out,
                                    FooCanvasGroup **reverse_col_out,
				    FooCanvasGroup **separator_col_out)
{
  ZMapFeatureTypeStyle style ;
  double top, bottom ;
  gboolean created = TRUE;
  char *name  ;
  GError *style_error = NULL ;

  zMapAssert(window && (forward_strand_group || reverse_strand_group)
	     && block && feature_set && (forward_col_out || reverse_col_out)) ;





  /* Look up the overall column style, its possible the style for the column may not have
   * have been found either because styles may be on a separate server or more often because
   * the style for a feature set isn't known until the features have been read. Then if we don't
   * find any features feature set won't have a style...kind of a catch 22. */
  style = zMapFindStyle(styles, feature_set->unique_id) ;

  name = (char *)g_quark_to_string(feature_set->unique_id) ;


  if (!style)
    {
      /* Temporary...probably user will not want to see this in the end but for now its good for debugging. */
      zMapShowMsg(ZMAP_MSG_WARNING, "feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		  name, name) ;

      zMapLogCritical("feature set \"%s\" not displayed because its style (\"%s\") could not be found.",
		      name, name) ;

      created = FALSE;
    }
  else if (!zMapStyleIsDisplayable(style))
    {
      /* some styles should not be shown, e.g. they may be "meta" styles like "3 Frame". */
      created = FALSE;
    }
  else if (!zMapStyleIsDrawable(style, &style_error))
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Temporary...while I test new styles... */
      zMapShowMsg(ZMAP_MSG_WARNING, "feature set \"%s\" not displayed because its style (\"%s\") is not valid: %s",
		  name, name, style_error->message) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      zMapLogCritical("feature set \"%s\" not displayed because its style (\"%s\") is not valid: %s",
		      name, name, style_error->message) ;

      created = FALSE ;
      g_clear_error(&style_error) ;
    }

  if(separator_col_out)
    *separator_col_out = NULL;

  if (created)
    {
      FooCanvasGroup *set_group_item;
      /* We need the background column object to span the entire bottom of the alignment block. */
      top = block->block_to_sequence.t1 ;
      bottom = block->block_to_sequence.t2 ;
      zmapWindowSeq2CanExtZero(&top, &bottom) ;

      /* This code is _HERE_ _BEFORE_ the forward or reverse strand
       * code so that _if_ this is a separator column it will get drawn
       * and added to the hash correctly.  This is not good in one
       * respect, but saves a lot of other coding...
       * ********************************************************** */
      if(zMapStyleDisplayInSeparator(style) && separator_col_out)
	{
	  ZMapWindowContainerGroup tmp_container;
	  ZMapWindowContainerGroup block_container;
	  ZMapWindowContainerStrand separator;

	  /* Yes we piggy back here, but as the function requires a F or R group so what. */
	  if(forward_strand_group)
	    {
	      tmp_container   = zmapWindowContainerChildGetParent((FooCanvasItem *)forward_strand_group);
	      block_container = zmapWindowContainerUtilsGetParentLevel(tmp_container, ZMAPCONTAINER_LEVEL_BLOCK);
	    }
	  else
	    {
	      tmp_container   = zmapWindowContainerChildGetParent((FooCanvasItem *)reverse_strand_group);
	      block_container = zmapWindowContainerUtilsGetParentLevel(tmp_container, ZMAPCONTAINER_LEVEL_BLOCK);
	    }

	  if((separator = zmapWindowContainerBlockGetContainerSeparator((ZMapWindowContainerBlock)block_container)))
	    {
	      /* No need to create if we've already got one. */
	      /* N.B. separatorGetFeatureSetColumn returns a FooCanvasGroup * */
	      if(!(*separator_col_out = separatorGetFeatureSetColumn(separator,
								     feature_set)))
		{
		  ZMapWindowContainerFeatures separator_features;
		  separator_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)separator);
		  *separator_col_out = createColumn(separator_features, window,
						    feature_set,
						    ZMAPSTRAND_NONE,
						    frame,
						    FALSE,
						    zMapStyleGetWidth(style),
						    top, bottom);
		}
	    }
	}

      if (forward_strand_group)
        {
	  ZMapCanvasDataStruct canvas_data = {NULL};

	  canvas_data.window         = window;
	  canvas_data.curr_alignment = (ZMapFeatureAlignment)(block->parent) ;
	  canvas_data.curr_block     = block;
	  canvas_data.styles         = styles;

	  set_group_item = produce_column(&canvas_data, forward_strand_group,
					  feature_set->unique_id,
					  ZMAPSTRAND_FORWARD, frame);
	  if(set_group_item)
	    {
              zMapAssert(set_group_item && FOO_IS_CANVAS_GROUP(set_group_item));
              *forward_col_out = FOO_CANVAS_GROUP(set_group_item);

	      zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)set_group_item, feature_set);
            }
	  else
	    *forward_col_out = NULL;
        }

      if (reverse_strand_group)
        {
	  ZMapCanvasDataStruct canvas_data = {NULL};

	  canvas_data.window         = window;
	  canvas_data.curr_alignment = (ZMapFeatureAlignment)block->parent;
	  canvas_data.curr_block     = block;
	  canvas_data.styles         = styles;

	  set_group_item = NULL;

	  if(zMapStyleIsStrandSpecific(style))
	    set_group_item = produce_column(&canvas_data, reverse_strand_group,
					    feature_set->unique_id,
					    ZMAPSTRAND_REVERSE, frame);

	  if(set_group_item)
            {
              zMapAssert(set_group_item && FOO_IS_CANVAS_GROUP(set_group_item));
              *reverse_col_out = FOO_CANVAS_GROUP(set_group_item);

	      zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)set_group_item, feature_set);
            }
	  else
	    *reverse_col_out = NULL;
        }
    }

  return created;
}

/* Called for each feature set, it then calls a routine to draw each of its features.  */
/* The feature set will be filtered on supplied frame by ProcessFeature.
 * ProcessFeature splits the feature sets features into the separate strands.
 */
void zmapWindowDrawFeatureSet(ZMapWindow window,
			      GHashTable *styles,
                              ZMapFeatureSet feature_set,
                              FooCanvasGroup *forward_col_wcp,
                              FooCanvasGroup *reverse_col_wcp,
                              ZMapFrame frame,
			      gboolean frame_mode_change)
{
  ZMapWindowContainerGroup forward_container = NULL;
  ZMapWindowContainerGroup reverse_container = NULL;
  CreateFeatureSetDataStruct featureset_data = {NULL} ;
  ZMapFeatureSet view_feature_set = NULL;
  gboolean bump_required = TRUE;

  /* We shouldn't be called if there is no forward _AND_ no reverse col..... */
  zMapAssert(forward_col_wcp || reverse_col_wcp) ;

  featureset_data.window = window ;

  if (forward_col_wcp)
    {
      ZMapWindowContainerFeatureSet container;

      forward_container = (ZMapWindowContainerGroup)forward_col_wcp;
      container = (ZMapWindowContainerFeatureSet) forward_container;

      if (zmapWindowColumnIs3frameVisible(window, forward_col_wcp))
      	featureset_data.curr_forward_col = zmapWindowContainerGetFeatures(forward_container) ;

      if(feature_set->masker_sorted_features)
        {
            if(!container->maskable)      /* cannot have been unset */
                  container->masked = FALSE;
            container->maskable = TRUE;
        }

      if(!g_list_find(container->featuresets,GUINT_TO_POINTER(feature_set->unique_id)))
        {
            container->featuresets = g_list_prepend(container->featuresets,
                              GUINT_TO_POINTER(feature_set->unique_id));
        }

      view_feature_set = zmapWindowContainerGetFeatureSet(forward_container);

      if (!view_feature_set)
	zMapLogWarning("Container with no Feature Set attached [%s]", "Forward Strand");
    }

  if (reverse_col_wcp)
    {
      ZMapWindowContainerFeatureSet container;

      reverse_container = (ZMapWindowContainerGroup)reverse_col_wcp;
      container = (ZMapWindowContainerFeatureSet) reverse_container;

      if (zmapWindowColumnIs3frameVisible(window, reverse_col_wcp))
	      featureset_data.curr_reverse_col = zmapWindowContainerGetFeatures(reverse_container) ;

      if(feature_set->masker_sorted_features)
        {
            if(!container->maskable)      /* cannot have been unset */
                  container->masked = FALSE;
            container->maskable = TRUE;
        }

      if(!g_list_find(container->featuresets,GUINT_TO_POINTER(feature_set->unique_id)))
        {
            container->featuresets = g_list_prepend(container->featuresets,
                              GUINT_TO_POINTER(feature_set->unique_id));
        }


      if (!view_feature_set)
	{
	  view_feature_set = zmapWindowContainerGetFeatureSet(reverse_container);

	  if (!view_feature_set)
	    zMapLogWarning("Container with no Feature Set attached [%s]", "Reverse Strand");
	}
    }

  featureset_data.frame         = frame ;
  featureset_data.styles        = styles ;
  featureset_data.feature_count = 0;


  /* Now draw all the features in the column. */
  zMapStartTimer("DrawFeatureSet","ProcessFeature");

  if(zMapWindowContainerSummarise(window,feature_set->style))
  {
#if MH17_DONT_INCLUDE
      printf("summarise %s zoom: %f,%f\n", g_quark_to_string(feature_set->unique_id),
            zMapStyleGetSummarise(feature_set->style),zMapWindowGetZoomFactor(window));
#endif
      featureset_data.feature_list = zMapWindowContainerSummariseSortFeatureSet(feature_set);

      g_list_foreach(featureset_data.feature_list, ProcessListFeature, &featureset_data) ;

      g_list_free(featureset_data.feature_list);
      featureset_data.feature_list = NULL;

      zMapWindowContainerSummariseClear(window,feature_set);
  }
  else
  {
      g_hash_table_foreach(feature_set->features, ProcessFeature, &featureset_data) ;
  }

  zMapStopTimer("DrawFeatureSet","ProcessFeature");


  if (featureset_data.feature_count > 0)
    {
      ZMapWindowContainerGroup column_container_parent;

      if ((column_container_parent = forward_container) || (column_container_parent = reverse_container))
	{
	  ZMapWindowContainerGroup block_container;

	  block_container = zmapWindowContainerUtilsGetParentLevel(column_container_parent,
								   ZMAPCONTAINER_LEVEL_BLOCK);

	  zmapWindowContainerBlockFlagRegionForColumn((ZMapWindowContainerBlock)block_container,
						      (ZMapFeatureBlock)feature_set->parent,
						      (ZMapWindowContainerFeatureSet)column_container_parent);
	}
    }


  /* We should be bumping columns here if required... */
  if (bump_required && view_feature_set)
    {
      ZMapStyleBumpMode bump_mode ;

      zMapStartTimer("DrawFeatureSet","Bump");

      /* Use the style from the feature set attached to the
       * column... Better than using what is potentially a diff
       * context... */

      /* Changed zmapWindowColumnBump to zmapWindowColumnBumpRange to fix RT#66832 */
      /* The problem was objects outside of the mark were being hidden as ColumnBump
       * respects the mark if there is one.  This is probably _not_ the right thing
       * to do here. However, this might need some re-evaluation... */
      /* The main reason I think we will fall down here is when we have deferred
       * loading.  If we are loading a set of alignments within the marked area,
       * then if the default display is to bump these and there are other aligns
       * already loaded in this column a COMPRESS_ALL will bump the whole column
       * _not_ just the newly loaded ones... */


      if (forward_col_wcp)
	{
	  ZMapStyleBumpMode curr_bump_mode = ZMAPSTYLE_COLDISPLAY_INVALID ;
	  gboolean redraw_needed = FALSE ;

	  if ((bump_mode = zmapWindowContainerFeatureSetGetBumpMode((ZMapWindowContainerFeatureSet)forward_container))
	      != ZMAPBUMP_UNBUMP)
	    zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(forward_col_wcp), bump_mode, ZMAPWINDOW_COMPRESS_ALL) ;

	  /* try 3 frame stuff here...more complicated */
	  if (zmapWindowContainerFeatureSetIsFrameSpecific(ZMAP_CONTAINER_FEATURESET(forward_col_wcp), NULL))
	    {
	      if (IS_3FRAME(window->display_3_frame))
		{
		  if (zmapWindowColumnIs3frameDisplayed(window, forward_col_wcp))
		    curr_bump_mode = ZMAPSTYLE_COLDISPLAY_SHOW ;
		  else
		    curr_bump_mode = ZMAPSTYLE_COLDISPLAY_HIDE ;
		}

	      if (frame_mode_change)
		redraw_needed = TRUE ;
	    }

	  /* Some columns are hidden initially, could be mag. level, 3 frame only display or
	   * set explicitly in the style for the column. */
	  zMapStartTimer("DrawFeatureSet","SetState");

	  zmapWindowColumnSetState(window, forward_col_wcp, curr_bump_mode, redraw_needed) ;

	  zMapStopTimer("DrawFeatureSet","SetState");
    	}

      if (reverse_col_wcp)
	{
	  if ((bump_mode = zmapWindowContainerFeatureSetGetBumpMode((ZMapWindowContainerFeatureSet)reverse_container)) != ZMAPBUMP_UNBUMP)
	    zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(reverse_col_wcp), bump_mode, ZMAPWINDOW_COMPRESS_ALL) ;

	  /* Some columns are hidden initially, could be mag. level, 3 frame only display or
	   * set explicitly in the style for the column. */
	  zmapWindowColumnSetState(window, reverse_col_wcp, ZMAPSTYLE_COLDISPLAY_INVALID, FALSE) ;
	}

      zMapStopTimer("DrawFeatureSet","Bump");
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
  ZMapWindowContainerGroup container;
  gboolean removed = FALSE ;

  container = (ZMapWindowContainerGroup)(*col_group);

  if ((!zmapWindowContainerHasFeatures(container)) &&
      (!zmapWindowContainerFeatureSetShowWhenEmpty((ZMapWindowContainerFeatureSet)container)))
    {
      container = zmapWindowContainerGroupDestroy(container) ;

      *col_group = (FooCanvasGroup *)container;

      removed = TRUE ;
    }

  return removed ;
}




void zmapWindowDraw3FrameFeatures(ZMapWindow window)
{
      ZMapCanvasDataStruct canvas_data = {NULL};
      ZMapFeatureContext full_context;

      full_context = window->feature_context;

      canvas_data.window        = window;
      canvas_data.canvas        = window->canvas;
      canvas_data.curr_x_offset = 0.0;
      canvas_data.styles        = window->display_styles;
      canvas_data.full_context  = full_context;
      canvas_data.frame_mode_change = TRUE ;       // refer to comment in feature_set_matches_frame_drawing_mode()

      canvas_data.curr_root_group = zmapWindowContainerGetFeatures(window->feature_root_group) ;


      zMapFeatureContextExecuteComplete((ZMapFeatureAny)full_context,
				    ZMAPFEATURE_STRUCT_FEATURE,
				    windowDrawContextCB,
				    NULL, &canvas_data);

  return ;
}



void zmapWindowDrawRemove3FrameFeatures(ZMapWindow window)
{
  zmapWindowContainerUtilsExecute(window->feature_root_group,
				  ZMAPCONTAINER_LEVEL_FEATURESET,
				  purge_hide_frame_specific_columns,
				  window) ;

  return ;
}





/*
 *                          internal functions
 */


/* mask features that are displayed */
/* iterate through a column's item list if it contains any featureset in the 'just-masked' list */
void container_mask_cb(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                        ZMapContainerLevelType level, gpointer user_data)
{
      ZMapCanvasData     canvas_data = (ZMapCanvasData) user_data;
      ZMapWindowContainerFeatureSet cfs = (ZMapWindowContainerFeatureSet) container;
      GList *l;

      if(level != ZMAPCONTAINER_LEVEL_FEATURESET)
            return;

      for(l = canvas_data->masked;l;l = l->next)
      {
            if(g_list_find(cfs->featuresets,l->data))
            {
                  // this does the lot!
                  // mh17: cannot believe my luck -> or is it skill??
                  zMapWindowContainerFeatureSetShowHideMaskedFeatures(cfs, TRUE);
                  break;
            }
      }
}

static void windowDrawContext(ZMapCanvasData     canvas_data,
			      GHashTable             *styles,
			      ZMapFeatureContext full_context,
			      ZMapFeatureContext diff_context,
                        GList *masked)
{

  canvas_data->curr_x_offset = 0.0;

  canvas_data->styles        = styles;

  /* Full Context to attach to the items.
   * Diff Context feature any (aligns,blocks,sets) are destroyed! */
  canvas_data->full_context  = full_context;

  canvas_data->masked = masked;

  /* we iterate through the window's canvas data to mask existing features */
  zmapWindowContainerUtilsExecute(canvas_data->window->feature_root_group,
                             ZMAPCONTAINER_LEVEL_FEATURESET,
                             container_mask_cb, canvas_data);


  /* We iterate through the diff context to draw new data */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)diff_context,
                                    ZMAPFEATURE_STRUCT_FEATURE,
                                    windowDrawContextCB,
                                    NULL,
                                    canvas_data);


      /* unbumped features might be wider */
  zmapWindowFullReposition(canvas_data->window) ;

  return ;
}


static void purge_hide_frame_specific_columns(ZMapWindowContainerGroup container, FooCanvasPoints *points,
					      ZMapContainerLevelType level, gpointer user_data)
{
  ZMapWindow window = (ZMapWindow)user_data;

  if (level == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID ;
      ZMapWindowContainerFeatureSet container_set;

      container_set = (ZMapWindowContainerFeatureSet)container;

      if (zmapWindowContainerFeatureSetIsFrameSpecific(container_set, &frame_mode))
	{
	  if (frame_mode == ZMAPSTYLE_3_FRAME_ONLY_1)
	    {
#ifdef MH17_NEVER_INCLUDE_THIS_CODE
            ZMapStrand column_strand;

            column_strand = zmapWindowContainerFeatureSetGetStrand(container_set) ;

            zMapLogMessage("3F1: column %s [%s]",
                       g_quark_to_string(container_set->unique_id),
                       zMapFeatureStrand2Str(column_strand)) ;
#endif
#if 1 //def MH17_NEVER_INCLUDE_THIS_CODE
              zMapLogMessage("3F1: hiding %s", g_quark_to_string(container_set->unique_id)) ;
#endif
//	      if (window->display_3_frame)
		    zmapWindowColumnHide((FooCanvasGroup *)container) ;
	    }
	  else
	    {
	      ZMapStrand column_strand;

	      column_strand = zmapWindowContainerFeatureSetGetStrand(container_set) ;
#ifdef MH17_NEVER_INCLUDE_THIS_CODE
	      zMapLogMessage("column %s [%s]",
			     g_quark_to_string(container_set->unique_id),
			     zMapFeatureStrand2Str(column_strand)) ;
#endif
	      if ((column_strand != ZMAPSTRAND_REVERSE)
		  || (column_strand == ZMAPSTRAND_REVERSE && window->show_3_frame_reverse))
		{
#if 1 //def MH17_NEVER_INCLUDE_THIS_CODE
		  zMapLogMessage("3F3: hiding %s", g_quark_to_string(container_set->unique_id)) ;
#endif
		  zmapWindowColumnHide((FooCanvasGroup *)container) ;

		  /* remove all items from hash first !! */
		  removeAllFeatures(window, container_set) ;

		  zmapWindowContainerFeatureSetRemoveAllItems(container_set) ;
		}
	    }
	}
    }


  return ;
}


/* AGH, THIS ALL NEEDS TIDYING UP, ROYS NEW ITEMS HAVE CREATED A WHOLE LOAD OF ENCAPSULATION PROBLEMS.... */

/* This function may exist already....not sure..... */
static void removeAllFeatures(ZMapWindow window, ZMapWindowContainerFeatureSet container_set)
{
  RemoveFeatureDataStruct remove_data ;

  remove_data.window = window ;
  remove_data.strand = zmapWindowContainerFeatureSetGetStrand(container_set) ;
  remove_data.frame = zmapWindowContainerFeatureSetGetFrame(container_set) ;
  remove_data.feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set) ;

  g_hash_table_foreach(remove_data.feature_set->features, removeFeatureCB, &remove_data) ;

  return ;
}


static void removeFeatureCB(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)value ;
  RemoveFeatureData remove_data = (RemoveFeatureData)user_data ;

  zmapWindowFToIRemoveFeature(remove_data->window->context_to_item,
			      remove_data->strand, remove_data->frame, feature) ;

  return ;
}





#if MH17_THEY_GET_CREATED_ON_DEMAND
static void set_name_create_set_columns(gpointer list_data, gpointer user_data)
{
  GQuark feature_set_id = GPOINTER_TO_INT(list_data);
  ZMapCanvasData canvas_data = (ZMapCanvasData)user_data;
  ZMapFrame frame;
  int frame_i;
  GQuark feature_set_unique_id = zMapFeatureSetCreateID((char *) g_quark_to_string(feature_set_id));

  for (frame_i = ZMAPFRAME_NONE; frame_i <= ZMAPFRAME_2; frame_i++)
    {
      frame = (ZMapFrame)frame_i;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("making \"%s\" %s %s column \n",
	     g_quark_to_string(feature_set_id),
	     zMapFeatureFrame2Str(frame), zMapFeatureStrand2Str(ZMAPSTRAND_FORWARD)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      if (canvas_data->curr_forward_group)
	produce_column(canvas_data,
		       canvas_data->curr_forward_group,
		       feature_set_unique_id, ZMAPSTRAND_FORWARD,
		       frame) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("making \"%s\" %s %s column \n",
	     g_quark_to_string(feature_set_id),
	     zMapFeatureFrame2Str(frame), zMapFeatureStrand2Str(ZMAPSTRAND_REVERSE)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      if (canvas_data->curr_reverse_group)
	produce_column(canvas_data,
		       canvas_data->curr_reverse_group,
		       feature_set_unique_id, ZMAPSTRAND_REVERSE,
		       frame);
    }

  return ;
}
#endif


static FooCanvasGroup *find_or_create_column(ZMapCanvasData  canvas_data,
					     ZMapWindowContainerFeatures strand_container,
					     GQuark          feature_set_id,
					     ZMapStrand      column_strand,
					     ZMapFrame       column_frame,
					     gboolean        create_if_not_exist)
{
  FooCanvasGroup *column = NULL ;
  FooCanvasGroup *tmp_column = NULL ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;
  ZMapWindow window ;
  double top, bottom ;
  FooCanvasItem *existing_column_item ;
  ZMapStyle3FrameMode frame_mode ;
  gboolean valid_strand = FALSE ;
  gboolean valid_frame  = FALSE ;
  GQuark display_id = feature_set_id;
  GQuark orig_set_id = feature_set_id;

  zMapAssert(canvas_data && strand_container) ;

  window    = canvas_data->window;
  alignment = canvas_data->curr_alignment;
  block     = canvas_data->curr_block;

      /* MH17: map feature sets from context to canvas columns
       * refer to zmapGFF2parser.c/makeNewFeature()
       */
  if (window->featureset_2_column)  /* will always be but let's play safe */
    {
      ZMapGFFSet set_data ;

      if ((set_data = g_hash_table_lookup(window->featureset_2_column,
                                 GINT_TO_POINTER(feature_set_id))))
      {
            feature_set_id = set_data->feature_set_id;      /* the display column as a key */
            display_id = set_data->feature_set_ID;
      }
      /* else we use the original feature_set_id
         which should never happen as the view creates a 1-1 mapping regardless */
    }



  if ((existing_column_item = zmapWindowFToIFindItemFull(window->context_to_item,
							 alignment->unique_id,
							 block->unique_id,
							 feature_set_id,
							 column_strand,
							 column_frame, 0)))
    {

      tmp_column = FOO_CANVAS_GROUP(existing_column_item) ;

    }
  else if (create_if_not_exist)
    {
      top    = block->block_to_sequence.t1 ;
      bottom = block->block_to_sequence.t2 ;
      zmapWindowSeq2CanExtZero(&top, &bottom) ;

#if MH17_PRINT_CREATE_COL
/* now only a sensible number created. */
      printf("create column %s -> %s S-%d F-%d\n",
            g_quark_to_string(orig_set_id),
            g_quark_to_string(feature_set_id),
            column_strand,column_frame);
#endif
      /* need to create the column */
      if (!(tmp_column = createColumnFull(strand_container,
					  window, alignment, block,
					  NULL,
                                display_id, feature_set_id,
					  column_strand, column_frame, FALSE,
					  0.0, top, bottom)))
	{
	  zMapLogMessage("Column '%s', frame '%d', strand '%d', not created.",
			 g_quark_to_string(feature_set_id), column_frame, column_strand);
	}
    }


  if (tmp_column)
    {
      if (column_strand == ZMAPSTRAND_FORWARD)
      	valid_strand = TRUE;
      else if (column_strand == ZMAPSTRAND_REVERSE)
      	valid_strand = zmapWindowContainerFeatureSetIsStrandShown(
                 (ZMapWindowContainerFeatureSet)tmp_column);

      if (column_frame == ZMAPFRAME_NONE)
	      valid_frame = TRUE;
      else if (column_strand == ZMAPSTRAND_FORWARD || window->show_3_frame_reverse)
	      valid_frame = zmapWindowContainerFeatureSetIsFrameSpecific(
                 (ZMapWindowContainerFeatureSet)tmp_column, &frame_mode) ;

      if (valid_frame && valid_strand)
      	column = tmp_column ;
    }
#if MH17_DEBUG
printf("create %s: %p %p, v=%d %d\n",
            g_quark_to_string(feature_set_id),
            column,tmp_column,valid_strand,valid_frame);
#endif
  return column ;
}

#if MH17_THEY_GET_CREATED_ON_DEMAND
static FooCanvasGroup *find_column(ZMapCanvasData  canvas_data,
				   ZMapWindowContainerFeatures container,
				   GQuark          feature_set_id,
				   ZMapStrand      column_strand,
				   ZMapFrame       column_frame)
{
  FooCanvasGroup *existing_column = NULL;

  zMapAssert(canvas_data && container) ;

  existing_column = find_or_create_column(canvas_data, container, feature_set_id,
					  column_strand, column_frame, FALSE) ;

  return existing_column;
}
#endif

static FooCanvasGroup *produce_column(ZMapCanvasData  canvas_data,
				      ZMapWindowContainerFeatures container,
				      GQuark          feature_set_id,
				      ZMapStrand      column_strand,
				      ZMapFrame       column_frame)
{
  FooCanvasGroup *new_column = NULL;

  zMapAssert(canvas_data && container) ;

  new_column = find_or_create_column(canvas_data, container, feature_set_id,
				     column_strand, column_frame, TRUE) ;

  return new_column;
}

static gboolean pick_forward_reverse_columns(ZMapWindow       window,
					     ZMapCanvasData   canvas_data,
					     ZMapFeatureSet   feature_set,
					     ZMapFrame        frame,
					     FooCanvasGroup **fwd_col_out,
					     FooCanvasGroup **rev_col_out)
{
  FooCanvasGroup *set_column;
  gboolean found_one = FALSE;

  /* forward */
  if (fwd_col_out && canvas_data->curr_forward_group && (set_column = produce_column(canvas_data,
										  canvas_data->curr_forward_group,
										  canvas_data->curr_set->unique_id,
										  ZMAPSTRAND_FORWARD, frame)))
    {
      zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)set_column, feature_set);

      *fwd_col_out = set_column;
      found_one    = TRUE;
    }

  /* reverse */
  if (rev_col_out && canvas_data->curr_reverse_group && (set_column = produce_column(canvas_data,
										  canvas_data->curr_reverse_group,
										  canvas_data->curr_set->unique_id,
										  ZMAPSTRAND_REVERSE, frame)))
    {
      zmapWindowContainerFeatureSetAttachFeatureSet((ZMapWindowContainerFeatureSet)set_column, feature_set);

      *rev_col_out = set_column;
      found_one    = TRUE;
    }

  return found_one;
}

static gboolean contextGetMasterAlignSpan(ZMapFeatureContext context,int *start,int *end)
{
      gboolean res = TRUE;
      GHashTable *blocks = context->master_align->blocks ;
      ZMapFeatureBlock block ;

      // refer to RT 158542
      zMapAssert(g_hash_table_size(blocks) == 1) ;      // (currently we only ever have one)

      block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(blocks, 0)) ;
      if(start) *start = block->block_to_sequence.t1;
      if(end)   *end   = block->block_to_sequence.t2;

      return(res);
}


static ZMapFeatureContextExecuteStatus windowDrawContextCB(GQuark   key_id,
							   gpointer data,
							   gpointer user_data,
							   char   **error_out)
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

  if(window_draw_context_debug_G)
    zMapLogWarning("windowDrawContext: drawing %s", g_quark_to_string(feature_any->unique_id));

  /* Note in the below code for each feature type we either find that it already exists or
   * we create a new one. */
  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
      {
	if(window_draw_context_debug_G)
	  {
	    if(feature_any == (ZMapFeatureAny)canvas_data->full_context)
	      {
		/* As it says really.  Had a bit of trouble with calling functions
		 * passing in the wrong contexts... Results were as expected feature sets
		 * attached to columns left as dangling pointers and ditto for styles. */
		zMapLogWarning("Identical context pointers passed in. %s",
			       "This should only happen once per view.");
	      }
	  }
	break;
      }

    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapWindowContainerGroup align_parent;
        FooCanvasItem  *align_hash_item;
        double x, y;
        int start,end;
        feature_align = (ZMapFeatureAlignment)feature_any;
#ifdef MH17_REVCOMP_DEBUG
        printf("drawFeatures align");
#endif
        /* record the full_context current align, not the diff align which will get destroyed! */
        canvas_data->curr_alignment = zMapFeatureContextGetAlignmentByID(canvas_data->full_context,
                                                                         feature_any->unique_id) ;

        /* THIS MUST GO....because we will have aligns that .sigh.do not start at 0 one day.... */
        /* Always reset the aligns to start at y = 0. */
        //canvas_data->curr_y_offset = 0.0 ;

        x = canvas_data->curr_x_offset ;
//      y = canvas_data->full_context->sequence_to_parent.c1 ;
        contextGetMasterAlignSpan(canvas_data->full_context,&start,&end);   // else we get everything offset by the start coord
        y = start;
//      canvas_data->curr_y_offset = start ;

        my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

        if ((align_hash_item = zmapWindowFToIFindItemFull(window->context_to_item,
							  feature_align->unique_id,
							  0, 0, ZMAPSTRAND_NONE,
							  ZMAPFRAME_NONE, 0)))
          {
            zMapAssert(ZMAP_IS_CONTAINER_GROUP(align_hash_item));
            align_parent = (ZMapWindowContainerGroup)(align_hash_item);
          }
        else
          {
            align_parent = zmapWindowContainerGroupCreate(canvas_data->curr_root_group,
							  ZMAPCONTAINER_LEVEL_ALIGN,
							  window->config.block_spacing,
							  &(window->colour_alignment),
							  &(window->canvas_border));
            g_signal_connect(G_OBJECT(align_parent),
                             "destroy",
                             G_CALLBACK(containerDestroyCB),
                             window) ;

	    g_object_set_data(G_OBJECT(align_parent), ITEM_FEATURE_STATS,
			      zmapWindowStatsCreate((ZMapFeatureAny)(canvas_data->curr_alignment))) ;

	    g_object_set_data(G_OBJECT(align_parent), ZMAP_WINDOW_POINTER, window) ;

	    zmapWindowContainerAlignmentAugment((ZMapWindowContainerAlignment)align_parent,
						canvas_data->curr_alignment);

	    foo_canvas_item_set(FOO_CANVAS_ITEM(align_parent),
				"x", x,
				"y", y,
				NULL) ;

	    if (!(zmapWindowFToIAddAlign(window->context_to_item, key_id, (FooCanvasGroup *)align_parent)))
	      {
		status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
		*error_out = g_strdup_printf("Failed to add alignment '%s' to hash!",
					     g_quark_to_string(key_id));
	      }
	  }

	if (status == ZMAP_CONTEXT_EXEC_STATUS_OK)
	  canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent) ;

	break;
      }

    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	ZMapWindowContainerGroup block_parent;
        ZMapWindowContainerGroup forward_group, reverse_group ;
	ZMapWindowContainerGroup strand_separator;
	ZMapWindowContainerBlock container_block;
        FooCanvasItem *block_hash_item;
        GdkColor *for_bg_colour, *rev_bg_colour ;
        double x, y;
        gboolean block_created = FALSE;

        zMapStartTimer("DrawBlock","");

        feature_block = (ZMapFeatureBlock)feature_any;
#ifdef MH17_REVCOMP_DEBUG
printf("\ndrawFeatures block %d-%d",feature_block->block_to_sequence.t1,feature_block->block_to_sequence.t2);
#endif

        /* record the full_context current block, not the diff block which will get destroyed! */
        canvas_data->curr_block = zMapFeatureAlignmentGetBlockByID(canvas_data->curr_alignment,
                                                                   feature_block->unique_id);

        /* Always set y offset to be top of current block. */
//        canvas_data->curr_y_offset = feature_block->block_to_sequence.t1 ;

        if ((block_hash_item = zmapWindowFToIFindItemFull(window->context_to_item,
                                                         canvas_data->curr_alignment->unique_id,
                                                         feature_block->unique_id, 0, ZMAPSTRAND_NONE,
                                                         ZMAPFRAME_NONE, 0)))
          {
            zMapAssert(ZMAP_IS_CONTAINER_GROUP(block_hash_item));
            block_parent = (ZMapWindowContainerGroup)(block_hash_item);
#ifdef MH17_REVCOMP_DEBUG
            printf(" old");
#endif
          }
        else
          {
            block_parent = zmapWindowContainerGroupCreate(canvas_data->curr_align_group,
							  ZMAPCONTAINER_LEVEL_BLOCK,
							  window->config.strand_spacing,
							  &(window->colour_block),
							  &(canvas_data->window->canvas_border));
            g_signal_connect(G_OBJECT(block_parent),
                             "destroy",
                             G_CALLBACK(containerDestroyCB),
                             window) ;
            block_created = TRUE;
#ifdef MH17_REVCOMP_DEBUG
            printf(" new");
#endif

	    zmapWindowContainerBlockAugment((ZMapWindowContainerBlock)block_parent,
					    canvas_data->curr_block) ;

	    g_object_set_data(G_OBJECT(block_parent), ITEM_FEATURE_STATS,
			      zmapWindowStatsCreate((ZMapFeatureAny)(canvas_data->curr_block))) ;

	    g_object_set_data(G_OBJECT(block_parent), ZMAP_WINDOW_POINTER, window) ;

	    x = 0.0 ;
	    y = feature_block->block_to_sequence.t1 ;
	    my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x, &y) ;

	    foo_canvas_item_set(FOO_CANVAS_ITEM(block_parent),
				"y", y,
				NULL) ;


	    /* Add this block to our hash for going from the feature context to its on screen item. */
	    if (!(zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
					 canvas_data->curr_alignment->unique_id,
					 key_id,
					 (FooCanvasGroup *)block_parent)))
	      {
		status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
		*error_out = g_strdup_printf("Failed to add block '%s' to hash!",
					     g_quark_to_string(key_id));
	      }
	  }

	container_block = (ZMapWindowContainerBlock)block_parent;

	if(feature_block->features_start != 0 &&
	   feature_block->features_end   != 0)
	  {
	    zmapWindowContainerBlockFlagRegion(container_block, feature_block);
	  }


	if (status == ZMAP_CONTEXT_EXEC_STATUS_OK)
	  {
	    canvas_data->curr_block_group = zmapWindowContainerGetFeatures(block_parent) ;

	    if (block_created)
	      {
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
	      }

            /* Create the reverse group first.  It's then first in the list and
             * so gets called first in container execute. e.g. reposition code */
            if ((block_created) ||
		(reverse_group = (ZMapWindowContainerGroup)zmapWindowContainerBlockGetContainerStrand(container_block, ZMAPSTRAND_REVERSE)) == NULL)
              {
                reverse_group = zmapWindowContainerGroupCreate(canvas_data->curr_block_group,
							       ZMAPCONTAINER_LEVEL_STRAND,
							       window->config.column_spacing,
							       rev_bg_colour,
							       &(canvas_data->window->canvas_border));

                zmapWindowContainerStrandAugment((ZMapWindowContainerStrand)reverse_group, ZMAPSTRAND_REVERSE);

		zmapWindowContainerGroupBackgroundSize(reverse_group,
						       (double)(feature_block->block_to_sequence.t2 - feature_block->block_to_sequence.t1));

                g_signal_connect(G_OBJECT(zmapWindowContainerGetBackground(reverse_group)),
                                 "event", G_CALLBACK(strandBoundingBoxEventCB),
                                 (gpointer)window);

		g_object_set_data(G_OBJECT(reverse_group), ZMAP_WINDOW_POINTER, window) ;
              }

            canvas_data->curr_reverse_group = zmapWindowContainerGetFeatures(reverse_group) ;

	    /* Create the strand separator... */
	    if ((block_created == TRUE) ||

		(strand_separator = (ZMapWindowContainerGroup)zmapWindowContainerBlockGetContainerSeparator(container_block)) == NULL)

	      {
		strand_separator = zmapWindowContainerGroupCreate(canvas_data->curr_block_group,
								  ZMAPCONTAINER_LEVEL_STRAND,
								  window->config.column_spacing,
								  &(window->colour_separator),
								  &(canvas_data->window->canvas_border));

		zmapWindowContainerStrandSetAsSeparator((ZMapWindowContainerStrand)strand_separator);

		zmapWindowContainerGroupBackgroundSize(strand_separator,
						       (double) (feature_block->block_to_sequence.t2 - feature_block->block_to_sequence.t1));

		g_object_set_data(G_OBJECT(strand_separator), ZMAP_WINDOW_POINTER, window) ;
	      }

            if ((block_created == TRUE) ||
		(forward_group = (ZMapWindowContainerGroup)zmapWindowContainerBlockGetContainerStrand(container_block, ZMAPSTRAND_FORWARD)) == NULL)
              {
                forward_group = zmapWindowContainerGroupCreate(canvas_data->curr_block_group,
							       ZMAPCONTAINER_LEVEL_STRAND,
							       window->config.column_spacing,
							       for_bg_colour,
							       &(canvas_data->window->canvas_border));

                zmapWindowContainerStrandAugment((ZMapWindowContainerStrand)forward_group, ZMAPSTRAND_FORWARD);
		    zmapWindowContainerGroupBackgroundSize(forward_group,
						       (double)(feature_block->block_to_sequence.t2 - feature_block->block_to_sequence.t1));

                g_signal_connect(G_OBJECT(zmapWindowContainerGetBackground(forward_group)),
                                 "event", G_CALLBACK(strandBoundingBoxEventCB),
                                 (gpointer)window);

		g_object_set_data(G_OBJECT(forward_group), ZMAP_WINDOW_POINTER, window) ;
              }

            canvas_data->curr_forward_group = zmapWindowContainerGetFeatures(forward_group) ;

#if MH17_THEY_GET_CREATED_ON_DEMAND
          zMapStartTimer("CreateColumns","");

	    /* We create the columns here now. */
	    /* Why? So that we always have the column, even though it's empty... */
	    g_list_foreach(window->feature_set_names,
			   set_name_create_set_columns,
			   canvas_data);

          zMapStopTimer("CreateColumns","");
#endif

          }

        zMapStopTimer("DrawBlock","");

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        FooCanvasGroup *tmp_forward = NULL, *tmp_reverse = NULL ;
	int frame_start, frame_end;

        /* record the full_context current block, not the diff block which will get destroyed! */
        canvas_data->curr_set = zMapFeatureBlockGetSetByID(canvas_data->curr_block, feature_any->unique_id);

	/* Don't attach this feature_set to anything. It is
	 * potentially part of a _diff_ context in which it might be a
	 * copy of the view context's feature set.  It should also get
	 * destroyed with the diff context, so be warned. */
        feature_set = (ZMapFeatureSet)feature_any;

	/* Default is not to draw more than one column... */
	frame_start = ZMAPFRAME_NONE;
	frame_end   = ZMAPFRAME_NONE;

	if (!(canvas_data->frame_mode_change)
	    || feature_set_matches_frame_drawing_mode(window, canvas_data, canvas_data->curr_set,
						      &frame_start, &frame_end))
	  {
	    int i, got_columns = 0;

	    /* re-written for(i = frame_start; i <= frame_end; i++) */
	    i = frame_start;
	    do
	      {
		canvas_data->current_frame = (ZMapFrame)i;

		if ((got_columns = pick_forward_reverse_columns(window, canvas_data,
								/* This is the one to be attached to the column. */
								canvas_data->curr_set,
								canvas_data->current_frame,
								&tmp_forward, &tmp_reverse)))
		  {
		    if(tmp_forward)
		      zMapWindowContainerFeatureSetMarkUnsorted(ZMAP_CONTAINER_FEATURESET(tmp_forward));
		    if(tmp_reverse)
		      zMapWindowContainerFeatureSetMarkUnsorted(ZMAP_CONTAINER_FEATURESET(tmp_reverse));

		    zMapStartTimer("DrawFeatureSet",g_quark_to_string(feature_set->unique_id));

		    zmapWindowDrawFeatureSet(window,
					     canvas_data->styles,
					     feature_set,
					     tmp_forward,
					     tmp_reverse,
					     canvas_data->current_frame, canvas_data->frame_mode_change) ;

		    zMapStopTimer("DrawFeatureSet",g_quark_to_string(feature_set->unique_id));
		  }
		i++;
	      }
	    while(got_columns && i <= frame_end);
	  }

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
      {

	break;
      }
     case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
	status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
	zMapAssertNotReached();
	break;
      }
    }
#ifdef MH17_REVCOMP_DEBUG
  if(feature_type != ZMAPFEATURE_STRUCT_FEATURE)
      printf(" -> status %s\n",status == ZMAP_CONTEXT_EXEC_STATUS_OK ? "OK" : "error");
#endif

  return status;
}



/* function to obtain and step through a list of styles "attached" to
 * a column, filtering for frame specificity, before returning the
 * frame start and end, to be used in a for() loop when splitting a
 * feature set into separate, or not so, framed columns */
static gboolean feature_set_matches_frame_drawing_mode(ZMapWindow     window,
						       ZMapCanvasData canvas_data,
						       ZMapFeatureSet feature_set,
						       int *frame_start_out,
						       int *frame_end_out)
{
  gboolean matched = FALSE ;
  GList *list, *style_list = NULL;
  int frame_start, frame_end;
  gboolean frame_specific = FALSE;
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;


  if (!(style_list = zmapWindowFeatureSetStyles(window, canvas_data->styles, feature_set->unique_id)))
    {
      zMapLogCritical("No style list for '%s'!", g_quark_to_string(feature_set->unique_id)) ;
    }
  else
    {
      /* Default is not to draw more than one column... */
      frame_start = ZMAPFRAME_NONE;
      frame_end   = ZMAPFRAME_NONE;

      if ((list = g_list_first(style_list)))
	{
	  do
	    {
	      ZMapFeatureTypeStyle frame_style;

	      frame_style = ZMAP_FEATURE_STYLE(list->data);

	      if (!frame_specific)
		frame_specific = zMapStyleIsFrameSpecific(frame_style);

	      if (!frame_mode)
		zMapStyleGetStrandAttrs(frame_style,NULL,NULL,&frame_mode);
	    }
	  while (!frame_specific && !frame_mode && (list = g_list_next(list)));
	}

      if (frame_specific)
	{
	  /* Get the style from the column. */
	  if (style_list && frame_specific)
	    {
	      GValue value = {0};

	      g_value_init(&value, G_TYPE_UINT);

	      zmapWindowStyleListGetSetting(style_list, ZMAPSTYLE_PROPERTY_FRAME_MODE, &value);

	      frame_mode = g_value_get_uint(&value);
	    }

	  if (IS_3FRAME(window->display_3_frame))
	    {
	      /* We are going to draw in 3 frame so set up the correct frames to be drawn. */

	      switch(frame_mode)
		{
		case ZMAPSTYLE_3_FRAME_ONLY_1:
		  frame_start = ZMAPFRAME_NONE;
		  frame_end   = ZMAPFRAME_NONE;
		  break;
		case ZMAPSTYLE_3_FRAME_ONLY_3:
		case ZMAPSTYLE_3_FRAME_AS_WELL:
		  /* we're only redrawing the 3 frame columns here...*/
		  frame_start = ZMAPFRAME_0;
		  frame_end   = ZMAPFRAME_2;
		  break;
		default:
		  break;
		}

	      matched = TRUE ;
	    }
	  else
	    {
	      /* We are returning from 3 frame to normal display so only redraw cols that should
	       * be shown normally. */

	      switch(frame_mode)
		{
		case ZMAPSTYLE_3_FRAME_ONLY_1:
		case ZMAPSTYLE_3_FRAME_ONLY_3:
		  matched = FALSE ;
		  break;
		default:
		  matched = TRUE ;
		  break;
		}
	    }
	}

      g_list_free(style_list);

      if (frame_start_out)
	*frame_start_out = frame_start ;
      if (frame_end_out)
	*frame_end_out = frame_end ;
    }
#if MH17_DEBUG
printf("%s: matched = %d, %d-%d\n", g_quark_to_string(feature_set->unique_id),matched,frame_start,frame_end) ;
#endif
  return matched ;
}



/* Cover/Utility function for createColumnFull() to save caller work. */
// NOTE: only ever called for separator features
static FooCanvasGroup *createColumn(ZMapWindowContainerFeatures parent_group,
				    ZMapWindow           window,
				    ZMapFeatureSet       feature_set,
				    ZMapStrand           strand,
				    ZMapFrame            frame,
				    gboolean             is_separator_col,
				    double width, double top, double bot)
{
  FooCanvasGroup *group = NULL;
  ZMapFeatureAlignment align ;
  ZMapFeatureBlock     block;

  if((block = (ZMapFeatureBlock)(feature_set->parent)) &&
     (align = (ZMapFeatureAlignment)(block->parent)))
    {
      group = createColumnFull(parent_group, window,
			       align, block, NULL,
                         feature_set->original_id,
			       feature_set->unique_id,
			       strand, frame, is_separator_col,
			       width, top, bot);
    }
  else if(feature_set)
    {
      if(!block)
	zMapLogCritical("Featureset '%s' has no block parent.", g_quark_to_string(feature_set->original_id));
      if(!align)
	zMapLogCritical("Featureset '%s' has no align parent.", g_quark_to_string(feature_set->original_id));
    }
  else
    zMapLogCritical("%s", "No featureset!");

  return group;
}


GQuark zMapWindowGetFeaturesetContainerID(ZMapWindow window,GQuark featureset_id)
{
  ZMapGFFSet gffset;
  GQuark container_id = featureset_id;

  gffset = (ZMapGFFSet) g_hash_table_lookup(window->featureset_2_column,GUINT_TO_POINTER(featureset_id));
  if(gffset)
      container_id = gffset->feature_set_id;    /* this is the column id, due to historical naming. */

  return container_id;
}

/*
 * Create a Single Column.
 * This column is a Container, is created for one of the 6 possibilities of STRAND and FRAME.
 * The container has the required struct members to know which strand/frame it is since
 * they are now ZMapWindowContainerGroup objects (FooCanvasGroup subclass).
 * The Container is added to the FToI Hash.
 */
static FooCanvasGroup *createColumnFull(ZMapWindowContainerFeatures parent_group,
					ZMapWindow           window,
					ZMapFeatureAlignment align,
					ZMapFeatureBlock     block,
					ZMapFeatureSet       feature_set,
                              GQuark               original_id,
					GQuark               feature_set_unique_id,
					ZMapStrand           strand,
					ZMapFrame            frame,
					gboolean             is_separator_col,
					double width, double top, double bot)
{
  FooCanvasGroup *group = NULL ;
  GList *style_list = NULL;
  GdkColor *colour ;
  gboolean status ;
  gboolean proceed;
  GHashTable *styles;

  /* We _must_ have an align and a block... */
  zMapAssert(align != NULL);
  zMapAssert(block != NULL);
  /* ...and either a featureset _or_ a featureset id. */
//  zMapAssert((feature_set != NULL) || (feature_set_unique_id != 0));
  zMapAssert((feature_set_unique_id != 0));



  if(!original_id)
    {
      original_id = feature_set_unique_id;
    }

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
	  else if(strand == ZMAPSTRAND_REVERSE)
	    colour = &(window->colour_mreverse_col) ;
	  else
	    colour = NULL;
	}
      else
	{
	  if (strand == ZMAPSTRAND_FORWARD)
	    colour = &(window->colour_qforward_col) ;
	  else if(strand == ZMAPSTRAND_REVERSE)
	    colour = &(window->colour_qreverse_col) ;
	  else
	    colour = NULL;
	}
    }

  styles = window->display_styles;
  if(!g_hash_table_size(styles))
      styles = window->read_only_styles;
  /* Get the list of styles for this column and check that _all_ the styles are displayable. */
  if (!(style_list = zmapWindowFeatureSetStyles(window, styles, feature_set_unique_id)))
    {
      proceed = FALSE;
      zMapLogMessage("Styles list for Column '%s' not found.",
		     g_quark_to_string(feature_set_unique_id)) ;
    }
  else
    {
      GList *list;
      proceed = TRUE;
      if((list = g_list_first(style_list)))
	{
	  do
	    {
	      ZMapFeatureTypeStyle style;
	      style = ZMAP_FEATURE_STYLE(list->data);
	      if(!zMapStyleIsDisplayable(style))
            {
                  printf("style %s for %s is not displayable\n", g_quark_to_string(style->unique_id),g_quark_to_string(feature_set_unique_id));
		      proceed = FALSE; /* not displayable, so bomb out the rest of the code. */
            }
#if MH17_DEBUG
if(feature_set_unique_id == g_quark_from_string("trembl"))
{
printf("trembl style %s = %d\n",g_quark_to_string(style->unique_id),style->frame_mode);
}
#endif
	    }
	  while(proceed && (list = g_list_next(list)));
	}
    }

  /* Can we still proceed? */
  if(proceed)
    {
      ZMapWindowContainerGroup container;
      /* Only now can we create a group. */
      container = zmapWindowContainerGroupCreate(parent_group, ZMAPCONTAINER_LEVEL_FEATURESET,
						 window->config.feature_spacing,
						 colour, &(window->canvas_border));

      /* reverse the column ordering on the reverse strand */
      if (strand == ZMAPSTRAND_REVERSE)
	foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(container));

      /* By default we do not redraw our children which are the individual features, the canvas
       * should do this for us. */
      zmapWindowContainerGroupChildRedrawRequired(container, FALSE) ;

      /* Make sure group covers whole span in y direction. */
      zmapWindowContainerGroupBackgroundSize(container, bot-top) ;

      /* THIS WOULD ALL GO IF WE DIDN'T ADD EMPTY COLS...... */
      /* We can't set the ITEM_FEATURE_DATA as we don't have the feature set at this point.
       * This probably points to some muckiness in the code, problem is caused by us deciding
       * to display all columns whether they have features or not and so some columns may not
       * have feature sets. */

      /* Attach data to the column including what strand the column is on and what frame it
       * represents, and also its style and a table of styles, used to cache column feature styles
       * where there is more than one feature type in a column. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("Adding column: \"%s\" %s %s\n",
	     g_quark_to_string(original_id), zMapFeatureStrand2Str(strand), zMapFeatureFrame2Str(frame)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      /* needs to accept style_list */
      zmapWindowContainerFeatureSetAugment((ZMapWindowContainerFeatureSet)container, window,
					   align->unique_id,
					   block->unique_id,
					   feature_set_unique_id, original_id,
					   style_list, strand, frame);

      /* This will create the stats if feature_set != NULL */
      zmapWindowContainerAttachFeatureAny(container, (ZMapFeatureAny)feature_set);

      g_signal_connect(G_OBJECT(container), "destroy", G_CALLBACK(containerDestroyCB), (gpointer)window) ;

      g_signal_connect(G_OBJECT(container), "event", G_CALLBACK(columnBoundingBoxEventCB), (gpointer)window) ;

      g_object_set_data(G_OBJECT(container), ZMAP_WINDOW_POINTER, window) ;

      group = (FooCanvasGroup *)container;

      if(!is_separator_col)
	{
	  status = zmapWindowFToIAddSet(window->context_to_item,
					align->unique_id,
					block->unique_id,
					feature_set_unique_id,
					strand, frame,
					group) ;
	  zMapAssert(status) ;
	}

      g_list_free(style_list);
      style_list = NULL;
    }

  return group ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Debug. */
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureSet feature_set = (ZMapFeatureSet)data ;

  printf("%s ", g_quark_to_string(feature_set->unique_id)) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Called to draw each individual feature. */
static void ProcessFeature(gpointer key, gpointer data, gpointer user_data)
{
      ProcessListFeature(data,user_data);
}

static void ProcessListFeature(gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature) data ;
  CreateFeatureSetData featureset_data = (CreateFeatureSetData) user_data ;
  ZMapWindow window = featureset_data->window ;
  ZMapWindowContainerGroup column_group ;
  ZMapStrand display_strand ;
  FooCanvasItem *feature_item ;
  ZMapFeatureTypeStyle style ;

#ifdef MH17_REVCOMP_DEBUG
  printf("ProcessFeature %d-%d ",feature->x1,feature->x2);
#endif

  style = feature->style;
      /* if fails: no display. fixed for pipe via GFF2parser, ACE seems to call it???
       * features paint so it musk be ok! *
       * but if a user adds an object and we make a fetaure OTF then no style is attached
       * the above is an optimisation
       */
  if(!style)
  {
      /* should only be a consideration for OTF data w/ no styles?? */
      style = zMapFindStyle(featureset_data->styles, feature->style_id) ;
      feature->style = style;
  }

#ifdef MH17_REVCOMP_DEBUG
  if(!style) printf("no style 1 ");
#endif

  /* Do some filtering on frame and strand */

  /* Find out which strand group the feature should be displayed in. */
  display_strand = zmapWindowFeatureStrand(window, feature) ;

  /* Caller may not want a forward or reverse strand and this is indicated by NULL value for
   * curr_forward_col or curr_reverse_col */
  if ((display_strand == ZMAPSTRAND_FORWARD && !(featureset_data->curr_forward_col))
      || (display_strand == ZMAPSTRAND_REVERSE && !(featureset_data->curr_reverse_col)))
  {
#ifdef MH17_REVCOMP_DEBUG
  printf("wrong strand 1\n");
#endif
    return ;
  }

  /* If we are doing frame specific display then don't display the feature if its the wrong
   * frame or its on the reverse strand and we aren't displaying reverse strand frames. */
  if (featureset_data->frame != ZMAPFRAME_NONE
      && (featureset_data->frame != zmapWindowFeatureFrame(feature)
	  || (!(window->show_3_frame_reverse) && display_strand == ZMAPSTRAND_REVERSE)))
  {
#ifdef MH17_REVCOMP_DEBUG
  printf("wrong strand 2\n");
#endif
    return ;
  }

  /* Get the correct column to draw into... */
  if (display_strand == ZMAPSTRAND_FORWARD)
    {
      column_group = zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(featureset_data->curr_forward_col)) ;
    }
  else
    {
      column_group = zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(featureset_data->curr_reverse_col)) ;
    }

  featureset_data->feature_count++;



#if MH17_NOT_NEEDED_FEATURESET_HAS_OWN_COPY_POINTED_AT_BY_FEATURE
  if(style)
    style = zmapWindowContainerFeatureSetStyleFromStyle((ZMapWindowContainerFeatureSet)column_group, style) ;
  else
    g_warning("need a style '%s' for feature '%s'",
	      g_quark_to_string(feature->style_id),
	      g_quark_to_string(feature->original_id));

#ifdef MH17_REVCOMP_DEBUG
  if(!style) printf("no style 2");
  printf("\n");
#endif

#endif

  if(style)
    feature_item = zmapWindowFeatureDraw(window, style, (FooCanvasGroup *)column_group, feature) ;
  else
    g_warning("definitely need a style '%s' for feature '%s'",
	      g_quark_to_string(feature->style_id),
	      g_quark_to_string(feature->original_id));
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


/* Handles events on a column, currently this is only mouse press/release events for
 * highlighting and column menus. */
static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;

  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE)
    {
      ZMapWindow window = (ZMapWindow)data ;
      GdkEventButton *but_event = (GdkEventButton *)event ;
      ZMapFeatureSet feature_set = NULL ;
      ZMapWindowContainerFeatureSet container_set;
      ZMapWindowContainerGroup container_parent;

      container_parent = zmapWindowContainerChildGetParent(item);

      /* These should go in container some time.... */
      container_set = (ZMapWindowContainerFeatureSet)container_parent;
      feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set);
      zMapAssert(feature_set || container_set) ;


      /* Only buttons 1 and 3 are handled. */
      if (event->type == GDK_BUTTON_PRESS && but_event->button == 3)
	{
	  /* Do the column menu. */
	  if (feature_set)
	    {
	      zmapMakeColumnMenu(but_event, window, item, container_set, NULL) ;

	      event_handled = TRUE ;
	    }
	}
      else if (event->type == GDK_BUTTON_RELEASE && but_event->button == 1)
	{
	  /* Highlight a column. */
	  ZMapWindowSelectStruct select = {0} ;
	  GQuark feature_set_id ;
	  char *clipboard_text = NULL;

#warning COLUMN_HIGHLIGHT_NEEDS_TO_WORK_WITH_MULTIPLE_WINDOWS

	  /* Swop focus from previous item(s)/columns to this column. */
	  zMapWindowUnHighlightFocusItems(window) ;

	  /* Try unhighlighting dna/translations... */
	  zmapWindowItemUnHighlightDNA(window, item) ;

	  {
	    ZMapFrame frame ;

	    for (frame = ZMAPFRAME_0 ; frame < ZMAPFRAME_2 + 1 ; frame++)
	      {
		zmapWindowItemUnHighlightTranslation(window, item, frame) ;
	      }
	  }

	  zmapWindowFocusSetHotColumn(window->focus, (FooCanvasGroup *)container_parent);

        select.feature_desc.struct_type = ZMAPFEATURE_STRUCT_FEATURESET ;

        feature_set_id = zmapWindowContainerFeatureSetColumnDisplayName(container_set);
        select.feature_desc.feature_set = (char *) g_quark_to_string(feature_set_id);

        {
            GQuark q;
            ZMapGFFSet gff;

            q = zmapWindowContainerFeatureSetGetColumnId(container_set);
            gff = g_hash_table_lookup(window->columns,GUINT_TO_POINTER(q));
            if(gff && gff->feature_set_text)
                  clipboard_text = select.feature_desc.feature_set_description = g_strdup(gff->feature_set_text);
        }

	  select.type = ZMAPWINDOW_SELECT_SINGLE;

	  (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

        if(clipboard_text)
          {
	      zMapWindowUtilsSetClipboard(window, clipboard_text);

	      g_free(clipboard_text) ;
          }

	  event_handled = TRUE ;
	}
    }

  return event_handled ;
}




/*
 *                       Menu functions.
 */



/* Build the background menu for a column. */
void zmapMakeColumnMenu(GdkEventButton *button_event, ZMapWindow window,
			FooCanvasItem *item,
                  ZMapWindowContainerFeatureSet container_set,
                  ZMapFeatureTypeStyle style_unused)
{
  static ZMapGUIMenuItemStruct separator[] =
    {
      {ZMAPGUI_MENU_SEPARATOR, NULL, 0, NULL, NULL},
      {ZMAPGUI_MENU_NONE, NULL, 0, NULL, NULL}
    } ;
  char *menu_title ;
  GList *menu_sets = NULL ;
  ItemMenuCBData cbdata ;
  ZMapFeature feature;
  ZMapFeatureSet feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set);
  menu_title = zMapFeatureName((ZMapFeatureAny)feature_set) ;

  cbdata = g_new0(ItemMenuCBDataStruct, 1) ;
  cbdata->item_cb = FALSE ;
  cbdata->window = window ;
  cbdata->item = item ;
  cbdata->feature_set = feature_set ;
  cbdata->container_set = container_set;

  /* Make up the menu. */
  if (zMapUtilsUserIsDeveloper())
    {
      menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDeveloperOps(NULL, NULL, cbdata)) ;

      menu_sets = g_list_append(menu_sets, separator) ;
    }

  menu_sets = g_list_append(menu_sets,
			    zmapWindowMakeMenuBump(NULL, NULL, cbdata,
						   zmapWindowContainerFeatureSetGetBumpMode((ZMapWindowContainerFeatureSet)item))) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDumpOps(NULL, NULL, cbdata)) ;

  menu_sets = g_list_append(menu_sets, separator) ;

  menu_sets = g_list_append(menu_sets, makeMenuColumnOps(NULL, NULL, cbdata)) ;

  if((feature = zMap_g_hash_table_nth(feature_set->features, 0)))
    {
      if(feature->type == ZMAPSTYLE_MODE_ALIGNMENT)
	{
	  menu_sets = g_list_append(menu_sets, separator) ;

	  if (feature->feature.homol.type == ZMAPHOMOL_X_HOMOL)
	    menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuProteinHomol(NULL, NULL, cbdata)) ;
	  else
	    menu_sets = g_list_append(menu_sets, zmapWindowMakeMenuDNAHomol(NULL, NULL, cbdata)) ;
	}
    }

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
      {ZMAPGUI_MENU_NORMAL, "List All Column Features",     1, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Feature Search Window", 2, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "DNA Search Window",     5, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Peptide Search Window", 6, columnMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "Toggle Mark",           7, columnMenuCB, NULL, "M"},
      {ZMAPGUI_MENU_NONE, NULL,                      0, NULL,         NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void columnMenuCB(int menu_item_id, gpointer callback_data)
{
  ZMapWindowContainerFeatureSet container_set;
  ItemMenuCBData menu_data = (ItemMenuCBData)callback_data ;

  container_set = (ZMapWindowContainerFeatureSet)(menu_data->item) ;

  switch (menu_item_id)
    {
    case 1:
      {
      ZMapFeatureSet feature_set ;
	ZMapWindowFToISetSearchData search_data;
	gboolean zoom_to_item = TRUE;

      feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set);

#ifndef REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION
	zoom_to_item = FALSE;
#endif /* REQUEST_TO_STOP_ZOOMING_IN_ON_SELECTION */

	search_data = zmapWindowFToISetSearchCreate(zmapWindowFToIFindItemSetFull, NULL,
						    feature_set->parent->parent->unique_id,
						    feature_set->parent->unique_id,
						    feature_set->column_id,
						    g_quark_from_string("*"),
						    zMapFeatureStrand2Str(zmapWindowContainerFeatureSetGetStrand(container_set)),
						    zMapFeatureFrame2Str(zmapWindowContainerFeatureSetGetFrame(container_set)));

	zmapWindowListWindow(menu_data->window,
			     NULL, (char *)g_quark_to_string(feature_set->original_id),
			     NULL, NULL,
			     (ZMapWindowListSearchHashFunc)zmapWindowFToISetSearchPerform, search_data,
			     (GDestroyNotify)zmapWindowFToISetSearchDestroy, zoom_to_item);
	break ;
      }
    case 2:
      zmapWindowCreateSearchWindow(menu_data->window, NULL, NULL, menu_data->item) ;
      break ;

    case 5:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_DNA) ;

      break ;

    case 6:
      zmapWindowCreateSequenceSearchWindow(menu_data->window, menu_data->item, ZMAPSEQUENCE_PEPTIDE) ;

      break ;

    case 7:
      zmapWindowToggleMark(menu_data->window, 0);
      break;

    default:
      zMapAssertNotReached() ;
      break ;
    }

  g_free(menu_data) ;

  return ;
}





/* Read colour information from the configuration file, note that we read _only_ the first
 * colour stanza found in the file, subsequent ones are not read. */
static void setColours(ZMapWindow window)
{
  ZMapConfigIniContext context = NULL;

  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(window->colour_root)) ;
  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &window->colour_alignment) ;
  gdk_color_parse(ZMAP_WINDOW_STRAND_DIVIDE_COLOUR, &window->colour_block) ;
  gdk_color_parse(ZMAP_WINDOW_STRAND_DIVIDE_COLOUR, &window->colour_separator) ;
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &window->colour_mblock_for) ;
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &window->colour_mblock_rev) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &window->colour_qblock_for) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &window->colour_qblock_rev) ;
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_F_BG, &(window->colour_mforward_col)) ;
  gdk_color_parse(ZMAP_WINDOW_MBLOCK_R_BG, &(window->colour_mreverse_col)) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_F_BG, &(window->colour_qforward_col)) ;
  gdk_color_parse(ZMAP_WINDOW_QBLOCK_R_BG, &(window->colour_qreverse_col)) ;
  gdk_color_parse(ZMAP_WINDOW_COLUMN_HIGHLIGHT, &(window->colour_column_highlight)) ;
  window->highlights_set.column = TRUE ;
//  gdk_color_parse(ZMAP_WINDOW_ITEM_HIGHLIGHT, &(window->colour_item_highlight)) ;
//  window->highlights_set.item = TRUE ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_0, &(window->colour_frame_0)) ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_1, &(window->colour_frame_1)) ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_2, &(window->colour_frame_2)) ;

  gdk_color_parse(ZMAP_WINDOW_ITEM_EVIDENCE_BORDER, &(window->colour_evidence_border)) ;
  gdk_color_parse(ZMAP_WINDOW_ITEM_EVIDENCE_FILL, &(window->colour_evidence_fill)) ;
  window->highlights_set.evidence = TRUE ;

  if ((context = zMapConfigIniContextProvide()))
    {
      char *colour = NULL;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_ROOT, &colour))
	gdk_color_parse(colour, &window->colour_root);

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_ALIGNMENT, &colour))
	gdk_color_parse(colour, &window->colour_alignment) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_BLOCK, &colour))
	gdk_color_parse(colour, &window->colour_block) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_SEPARATOR, &colour))
	gdk_color_parse(colour, &window->colour_separator) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_M_FORWARD, &colour))
	gdk_color_parse(colour, &window->colour_mblock_for) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_M_REVERSE, &colour))
	gdk_color_parse(colour, &window->colour_mblock_rev) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_Q_FORWARD, &colour))
	gdk_color_parse(colour, &window->colour_qblock_for) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_Q_REVERSE, &colour))
	gdk_color_parse(colour, &window->colour_qblock_rev) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_M_FORWARDCOL, &colour))
	gdk_color_parse(colour, &window->colour_mforward_col) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_M_REVERSECOL, &colour))
	gdk_color_parse(colour, &window->colour_mreverse_col) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_Q_FORWARDCOL, &colour))
	gdk_color_parse(colour, &window->colour_qforward_col) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_Q_REVERSECOL, &colour))
	gdk_color_parse(colour, &window->colour_qreverse_col) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_COL_HIGH, &colour))
	{
	  gdk_color_parse(colour, &window->colour_column_highlight) ;
	  window->highlights_set.column = TRUE ;
	}

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_ITEM_MARK, &colour))
	zmapWindowMarkSetColour(window->mark, colour);

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_ITEM_HIGH, &colour))
	{
	  gdk_color_parse(colour, &(window->colour_item_highlight)) ;
	  window->highlights_set.item = TRUE ;
	}

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                               ZMAPSTANZA_WINDOW_MASKED_FEATURE_BORDER, &colour))
      {
        gdk_color_parse(colour, &(window->colour_masked_feature_border)) ;
        window->highlights_set.masked = TRUE ;
      }
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                               ZMAPSTANZA_WINDOW_MASKED_FEATURE_FILL, &colour))
      {
        gdk_color_parse(colour, &(window->colour_masked_feature_fill)) ;
        window->highlights_set.masked = TRUE ;
      }

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_FRAME_0, &colour))
	gdk_color_parse(colour, &window->colour_frame_0) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_FRAME_1, &colour))
	gdk_color_parse(colour, &window->colour_frame_1) ;

      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
				       ZMAPSTANZA_WINDOW_FRAME_2, &colour))
	gdk_color_parse(colour, &window->colour_frame_2) ;


      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                               ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_BORDER, &colour))
      {
        gdk_color_parse(colour, &window->colour_evidence_border) ;
        window->highlights_set.evidence = TRUE ;
      }
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_WINDOW_CONFIG, ZMAPSTANZA_WINDOW_CONFIG,
                               ZMAPSTANZA_WINDOW_ITEM_EVIDENCE_FILL, &colour))
      {
        gdk_color_parse(colour, &window->colour_evidence_fill) ;
        window->highlights_set.evidence = TRUE ;
      }

      zMapConfigIniContextDestroy(context);
    }

  return ;
}

/* Should this be a FooCanvasItem* of a gpointer GObject ??? The documentation is lacking here!
 * well it compiles.... The wonders of the G_CALLBACK macro. */
static gboolean containerDestroyCB(FooCanvasItem *item, gpointer user_data)
{
  FooCanvasGroup *group = FOO_CANVAS_GROUP(item) ;
  ZMapWindow     window = (ZMapWindow)user_data ;
  ZMapWindowContainerFeatureSet container_set;
  ZMapWindowContainerGroup container;
  GHashTable *context_to_item ;
  ZMapFeatureAny feature_any;
  gboolean status = FALSE ;
  gboolean result = FALSE ;				    /* Always return FALSE ?? check this.... */
  gboolean log_all = FALSE ;

  if(log_all)
    zMapLogMessage("gobject type = '%s'", G_OBJECT_TYPE_NAME(item));

  /* Some items may not have features attached...e.g. empty cols....I should revisit the empty
   * cols bit, it keeps causing trouble all over the place.....column creation would be so much
   * simpler without it.... */
  if ((ZMAP_IS_CONTAINER_GROUP(item) == TRUE) &&
      (container   = ZMAP_CONTAINER_GROUP(item)))
    {
      context_to_item = window->context_to_item;

      feature_any = container->feature_any;

      switch (container->level)
	{
	case ZMAPCONTAINER_LEVEL_ROOT:
	  {
	    zMapAssert(feature_any->struct_type == ZMAPFEATURE_STRUCT_CONTEXT);

	    status = zmapWindowFToIRemoveRoot(context_to_item) ;

	    break ;
	  }
	case ZMAPCONTAINER_LEVEL_ALIGN:
	  {
	    zMapAssert(feature_any->struct_type == ZMAPFEATURE_STRUCT_ALIGN);

	    status = zmapWindowFToIRemoveAlign(context_to_item, feature_any->unique_id) ;

	    break ;
	  }
	case ZMAPCONTAINER_LEVEL_BLOCK:
	  {
	    zMapAssert(feature_any->struct_type == ZMAPFEATURE_STRUCT_BLOCK);
	    zMapAssert(feature_any->parent);

	    status = zmapWindowFToIRemoveBlock(context_to_item,
					       feature_any->parent->unique_id,
					       feature_any->unique_id) ;

	  }
	  break ;
	case ZMAPCONTAINER_LEVEL_FEATURESET:
	  {
	    container_set = (ZMapWindowContainerFeatureSet)container;

	    if(feature_any)
	      {
		zMapAssert(feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURESET);
		zMapAssert(feature_any->parent);
		zMapAssert(feature_any->parent->parent);
//		zMapAssert(feature_any->unique_id == container_set->unique_id); // not always true any more
	      }

	    status = zmapWindowFToIRemoveSet(context_to_item,
					     container_set->align_id,
					     container_set->block_id,
					     container_set->unique_id,
					     container_set->strand,
					     container_set->frame) ;

	    /* If the focus column goes then so should the focus items as they should be in step. */
	    if (window->focus && zmapWindowFocusGetHotColumn(window->focus) == group)
	      zmapWindowFocusReset(window->focus) ;

	  }
	  break ;
	default:
	  {
	    zMapAssertNotReached() ;
	  }
	}
#ifdef RDS_REMOVED
      /* It's not possible to check status as the parent containers
       * get destroyed first, which call FToIRemove, leading to the
       * removal of all the child hashes */
      if(!status)
	zMapLogCritical("containerDestroyCB (%p): %s remove failed", group, G_OBJECT_TYPE_NAME(item));
#endif /* RDS_REMOVED */
    }
  else
    {
      zMapLogCritical("containerDestroyCB (%p): no Feature Data", group);
    }

#ifndef RDS_REMOVED_STATS
  {
    ZMapWindowStats stats ;
    if ((stats = (ZMapWindowStats)(g_object_get_data(G_OBJECT(group), ITEM_FEATURE_STATS))))
      zmapWindowStatsDestroy(stats) ;
  }
#endif /* RDS_REMOVED_STATS */

  return result ;					    /* ????? */
}




/* return zero if match */
static gint separator_find_col_func(gconstpointer list_data, gconstpointer user_data)
{
  ZMapWindowContainerGroup column_parent = (ZMapWindowContainerGroup)list_data;
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data;
  ZMapFeatureSet column_set;
  gint match = -1;

  if((column_set = zmapWindowContainerGetFeatureSet(column_parent)))
    {
      if(column_set->unique_id == feature_set->unique_id)
	match = 0;
    }

  return match;
}

static FooCanvasGroup *separatorGetFeatureSetColumn(ZMapWindowContainerStrand separator_container,
						    ZMapFeatureSet  feature_set)
{
  ZMapWindowContainerFeatures container_features;
  FooCanvasGroup *column = NULL;
  FooCanvasGroup *features;
  GList *column_list = NULL;

  if((container_features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup)separator_container)))
    {
      features = (FooCanvasGroup *)container_features;
      column_list = g_list_find_custom(features->item_list, feature_set, separator_find_col_func);

      if(column_list)
	column = FOO_CANVAS_GROUP(column_list->data);
    }

  return column;
}


/****************** end of file ************************************/
