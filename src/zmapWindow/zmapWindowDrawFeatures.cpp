/*  File: zmapWindowDrawFeatures.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmap.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>

#include <zmapWindow_P.hpp>
#include <zmapWindowContainerUtils.hpp>

#include <zmapWindowContainerFeatureSet_I.hpp>                /* WHY'S THIS HERE ???? */



#include <zmapWindowCanvasFeatureset_I.hpp>                // for debug only


/* these will go when scale is in separate window. */
#define SCALEBAR_OFFSET   0.0
#define SCALEBAR_WIDTH   50.0




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
  //  GHashTable *styles ;
  int feature_count;
  ZMapWindowFeatureStackStruct feature_stack;         /* the context of the feature */
  ZMapWindowContainerFeatures curr_forward_col ;
  ZMapWindowContainerFeatures curr_reverse_col ;
  ZMapFrame frame ;
  gboolean drawable_frame;        /* filter */
} CreateFeatureSetDataStruct, *CreateFeatureSetData ;


typedef struct RemoveFeatureDataStructName
{
  ZMapWindow window ;
  ZMapStrand strand ;
  ZMapFrame frame ;
  ZMapFeatureSet feature_set ;
} RemoveFeatureDataStruct, *RemoveFeatureData ;


static ZMapFeatureContextExecuteStatus windowDrawContextCB(GQuark   key_id,
                                                           gpointer data,
                                                           gpointer user_data,
                                                           char   **error_out) ;
static FooCanvasGroup *produce_column(ZMapCanvasData  canvas_data,
                                      ZMapWindowContainerFeatures container,
                                      GQuark          feature_set_id,
                                      ZMapStrand      column_strand,
                                      ZMapFrame       column_frame,
                                      ZMapFeatureSet  feature_set);
static FooCanvasGroup *find_or_create_column(ZMapCanvasData  canvas_data,
                                             ZMapWindowContainerFeatures strand_container,
                                             GQuark          feature_set_id,
                                             ZMapStrand      column_strand,
                                             ZMapFrame       column_frame,
                                             ZMapFeatureSet feature_set,
                                             gboolean        create_if_not_exist) ;

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


static void ProcessFeature(gpointer key, gpointer data, gpointer user_data) ;
static void ProcessListFeature(gpointer data, gpointer user_data) ;

static void purge_hide_frame_specific_columns(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                                              ZMapContainerLevelType level, gpointer user_data) ;
static gboolean feature_set_matches_frame_drawing_mode(ZMapWindow     window,
                                                       ZMapFeatureTypeStyle style,
                                                       int *frame_start_out,
                                                       int *frame_end_out) ;

static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean setColumnTooltip(FooCanvasItem *item, GdkEvent *event, gpointer data) ;




static gboolean containerDestroyCB(FooCanvasItem *item_in_hash, gpointer data) ;

static void setColours(ZMapWindow window) ;

static void hideEmpty(ZMapWindow window, const char *where);
static void hideEmptyCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                        ZMapContainerLevelType level, gpointer user_data);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printFeatureSet(GQuark key_id, gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





static void removeAllFeatures(ZMapWindow window, ZMapWindowContainerFeatureSet container_set) ;
/*static void removeFeatureCB(gpointer key, gpointer value, gpointer user_data) ;*/


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
  ZMapCanvasDataStruct canvas_data = {NULL} ;                    /* Rest of struct gets set to zero. */
  ZMapWindowContainerGroup root_group ;
  FooCanvasItem *tmp_item = NULL ;
  gboolean debug_containers = FALSE, root_created = FALSE ;
  double x, y ;
  int seq_start,seq_end ;

  zMapPrintTimer(NULL, "About to create canvas features") ;

  if (!window || !full_context || !diff_context)
    return ;

  zmapWindowBusy(window, TRUE) ;


  /* Set up colours. */
  if (!window->done_colours)
    {
      setColours(window) ;
      window->done_colours = TRUE ;
    }


  /* Must be reset each time because context will change as features get merged in. */
  window->feature_context = full_context ;


  // we want to display just the data in our blocks, not the surrounding sequence
  zMapFeatureContextGetMasterAlignSpan(full_context,&seq_start,&seq_end);
  /* NOTE sequence is a pointer to the view's sequence
   * and should have been updated the view eg if revcomped
   */
   if(!window->sequence->end)
     {
       window->sequence->start = seq_start;
       window->sequence->end = seq_end;
     }

  window->seqLength = zmapWindowExt(window->sequence->start, window->sequence->end) ;

  zmapWindowScaleCanvasSetRevComped(window->ruler, window->flags[ZMAPFLAG_REVCOMPED_FEATURES]) ;
/*
 * MH17: after a revcomp we end up with 1-based coords if we have no official parent span
 * which is very confusing but valid. To display the ruler properly we need to use those coordinates
 * and not the original sequence coordinates.
 * the code just above sets sequence->start,end, but only on the first display, which is not revcomped
 */
//  zmapWindowScaleCanvasSetSpan(window->ruler, seq_start, seq_end) ;

  zmapWindowZoomControlInitialise(window);                    /* Sets min/max/zf */


  /* we use diff coords from the sequence if RevComped */
  window->min_coord = seq_start;
  window->max_coord = seq_end ;
  window->display_origin = seq_start ;

  zmapWindowSeq2CanExt(&(window->min_coord), &(window->max_coord)) ;

#if MH17_REVCOMP_DEBUG
  zMapLogWarning("drawFeatures window: %d-%d (%d,%d), %d (%f,%f), canvas = %p",
  window->sequence->start,window->sequence->end,
  seq_start,seq_end,
  window->flags[ZMAPFLAG_REVCOMPED_FEATURES],
  window->min_coord,window->max_coord,
  window->canvas);
#endif

  h_adj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

  if(!window->scroll_initialised)
    {
      /* MH17: draw features does not change the zoom factor, so we only set if first time round
       * re-using scroll initialised flag but that should be safe
       */
      zmapWindowSetPixelxy(window, 1.0, zMapWindowGetZoomFactor(window)) ;

      zmapWindowScaleCanvasSetPixelsPerUnit(window->ruler, 1.0, zMapWindowGetZoomFactor(window)) ;
    }

#if 0
  {
    double border_copy = 0.0;
    zmapWindowGetBorderSize(window, &border_copy);
    zmapWindowScaleCanvasSetLineHeight(window->ruler, border_copy * window->canvas->pixels_per_unit_y);
  }
#endif

  canvas_data.window = window;
  canvas_data.canvas = window->canvas;


  if((tmp_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                            0, 0, 0, ZMAPSTRAND_NONE, ZMAPFRAME_NONE, 0)))
    {
      root_group   = (ZMapWindowContainerGroup)tmp_item;
      root_created = FALSE;
    }
  else
    {
      double sx1,sy1,sx2,sy2;

      /*
        MH17 -> RT162629
        on v-split when zoomed this caused the LH pane to have a full range scroll region
        meaning that the two windows were not exact copies & they become unlocked
        we set the scroll region here as this is where we have the min/max coords

        erm... see also CAUSED_RT_57193 in zmapWindow.c
      */

      if(window->scroll_initialised)
        {
          zmapWindowGetScrollableArea(window, &sx1, &sy1, &sx2, &sy2);
        }
      else
        {
          sx1 = 0.0;
          sy1 = window->min_coord;
          sx2 = ZMAP_CANVAS_INIT_SIZE;
          sy2 = window->max_coord;
          zmapWindowSetScrollableArea(window, &sx1, &sy1, &sx2, &sy2,"zmapWindowDrawFeatures");
        }


      /*! \todo #warning seq offset should be in block and/or align? shouldn-t this should go into drawContextCB?? */

      /* Add a background to the root window, must be as long as entire sequence... */
      root_group = zmapWindowContainerGroupCreate((FooCanvasGroup *) foo_canvas_root(window->canvas),
                                                  ZMAPCONTAINER_LEVEL_ROOT,
                                                  window->config.align_spacing,
                                                  &(window->colour_root),
                                                  NULL);

      g_signal_connect(G_OBJECT(root_group), "destroy", G_CALLBACK(containerDestroyCB), window) ;

      root_created = TRUE;

      g_object_set_data(G_OBJECT(root_group), ITEM_FEATURE_STATS,
                        zmapWindowStatsCreate((ZMapFeatureAny)full_context)) ;

      g_object_set_data(G_OBJECT(root_group), ZMAP_WINDOW_POINTER, window) ;
    }

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(root_group) ;


  /* Always reset this as context changes with new features.*/
  zmapWindowContainerAttachFeatureAny(root_group, (ZMapFeatureAny)full_context);

  zmapWindowDrawSetGroupBackground(window, root_group, seq_start, seq_end,
                                   1.0, ZMAP_CANVAS_LAYER_ROOT_BACKGROUND, &(window->colour_root), NULL);

  zmapWindowFToIAddRoot(window->context_to_item, (FooCanvasGroup *)root_group);

  window->feature_root_group = root_group ;

  /* Set root group to start where sequence starts... */
  x = canvas_data.curr_x_offset ;
  y = full_context->master_align->sequence_span.x1 ;
  foo_canvas_item_w2i(FOO_CANVAS_ITEM(foo_canvas_root(window->canvas)), &x, &y) ;

  foo_canvas_item_set(FOO_CANVAS_ITEM(root_group),
                      "x", x,
                      "y", y,
                      NULL) ;



  /*
   *     Draw all the features, so much in so few lines...sigh...
   */

  zMapWindowDrawContext(&canvas_data, full_context, diff_context, masked);


  /* Now we've drawn all the features we can position them all. */
  zmapWindowColOrderColumns(window);


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

  hideEmpty(window, "draw features");

  zmapWindowBusy(window, FALSE) ;

  return ;
}



void zmapGetFeatureStack(ZMapWindowFeatureStack feature_stack,ZMapFeatureSet feature_set, ZMapFeature feature, ZMapFrame frame)
{
  /* scan for the call to get_featureset_column_index() for 2 more fields
     feature_stack->set_index =
     feature_stack->maps_to =
  */
  if (!(feature_set || feature))
    return ;

  feature_stack->id = 0;              /* set once per col for graph features in the item factory */

  feature_stack->strand = ZMAPSTRAND_NONE;
  feature_stack->frame = ZMAPFRAME_NONE;

  feature_stack->feature = feature;   /* may be NULL in which case featureset must not be */

  feature_stack->filter = FALSE;

  if(feature && *feature->style)        /* chicken */
    {
      //      if(zMapStyleIsStrandSpecific(*feature->style))        /* set per feature */
      //                feature_stack->strand = zmapWindowFeatureStrand(NULL,feature);

      //      if(zMapStyleIsFrameSpecific(*feature->style) && IS_3FRAME(display_3_frame))
      //                feature_stack->frame = zmapWindowFeatureFrame(feature);
    }
  feature_stack->frame = frame;


  if(!feature_set)
    feature_set = (ZMapFeatureSet)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                            ZMAPFEATURE_STRUCT_FEATURESET) ;

  feature_stack->feature_set       = feature_set;
  feature_stack->block     = (ZMapFeatureBlock)zMapFeatureGetParentGroup(
                                                                         (ZMapFeatureAny) feature_set,
                                                                         ZMAPFEATURE_STRUCT_BLOCK) ;
  feature_stack->align     = (ZMapFeatureAlignment)zMapFeatureGetParentGroup(
                                                                             (ZMapFeatureAny) feature_stack->block,
                                                                             ZMAPFEATURE_STRUCT_ALIGN) ;
  feature_stack->context   = (ZMapFeatureContext)zMapFeatureGetParentGroup(
                                                                           (ZMapFeatureAny) feature_stack->align,
                                                                           ZMAPFEATURE_STRUCT_CONTEXT) ;

  return ;
}



/* each column has a glist of featuresets, find where we appear in this list */
/* in case of snafu return zero rather than bombing out */
int get_featureset_column_index(ZMapFeatureContextMap map,GQuark featureset_id)
{
  int index = 0;
  ZMapFeatureColumn column;
  ZMapFeatureSetDesc desc_set = NULL ;
  ZMapFeatureSource src = NULL ;
  GList *l;

  desc_set = (ZMapFeatureSetDesc) g_hash_table_lookup(map->featureset_2_column,GUINT_TO_POINTER(featureset_id));
  if(!desc_set)
    return 0;
  src = (ZMapFeatureSource) g_hash_table_lookup(map->source_2_sourcedata,GUINT_TO_POINTER(featureset_id));
  if(!src)
    return 0;
  column = (ZMapFeatureColumn) g_hash_table_lookup(map->columns,GUINT_TO_POINTER(desc_set->column_id));
  if(!column)
    return 0;

  for(l = column->featuresets_unique_ids;l;l = l->next, index++)
    {
      if(GPOINTER_TO_UINT(l->data) == featureset_id)
        return(index);

      if(GPOINTER_TO_UINT(l->data) == src->maps_to)
        return(index);
    }

  return 0;
}


/* Called for each feature set, it then calls a routine to draw each of its features.  */
/* The feature set will be filtered on supplied frame by ProcessFeature.
 * ProcessFeature splits the feature sets features into the separate strands.
 */
int zmapWindowDrawFeatureSet(ZMapWindow window,
                             ZMapFeatureSet feature_set,
                             FooCanvasGroup *forward_col_wcp,
                             FooCanvasGroup *reverse_col_wcp,
                             ZMapFrame frame,
                             gboolean redraw_needed)
{
  ZMapWindowContainerGroup forward_container = NULL;
  ZMapWindowContainerGroup reverse_container = NULL;
  CreateFeatureSetDataStruct featureset_data = {NULL} ;
  ZMapFeatureSet view_feature_set = NULL;
  gboolean bump_required = TRUE;
  ZMapFeatureSource f_src ;

  /* We shouldn't be called if there is no forward _AND_ no reverse col..... */
  if (!(forward_col_wcp || reverse_col_wcp))
    return  0 ;
#define MODULE_STATS        0
#if MODULE_STATS
  double time = zMapElapsedSeconds;
#endif

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
          if(!container->maskable)            /* cannot have been unset -> this is the first maskable set */
            container->masked = FALSE;     /* default to visible on first feature set */
          container->maskable = TRUE;
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
          if(!container->maskable)            /* cannot have been unset */
            container->masked = FALSE;     /* default to visible on first feature set */
          container->maskable = TRUE;
        }

      if (!view_feature_set)
        {
          view_feature_set = zmapWindowContainerGetFeatureSet(reverse_container);

          if (!view_feature_set)
            zMapLogWarning("Container with no Feature Set attached [%s]", "Reverse Strand");
        }
    }

  featureset_data.frame         = frame ;
  featureset_data.feature_count = 0;

  zmapGetFeatureStack(&featureset_data.feature_stack,feature_set,NULL,frame);

  featureset_data.feature_stack.filter = TRUE;


  f_src = (ZMapFeatureSource)g_hash_table_lookup(window->context_map->source_2_sourcedata, GUINT_TO_POINTER(feature_set->unique_id));

  if(zMapStyleDensityStagger(feature_set->style))
    {
      featureset_data.feature_stack.set_index =
        get_featureset_column_index(window->context_map,feature_set->unique_id);
    }
  if(f_src)
    featureset_data.feature_stack.maps_to = f_src->maps_to;

  featureset_data.feature_stack.set_column[ZMAPSTRAND_NONE]    = (ZMapWindowContainerFeatureSet) forward_container;
  featureset_data.feature_stack.set_column[ZMAPSTRAND_FORWARD] = (ZMapWindowContainerFeatureSet) forward_container;
  featureset_data.feature_stack.set_column[ZMAPSTRAND_REVERSE] = (ZMapWindowContainerFeatureSet) reverse_container;

  featureset_data.feature_stack.set_features[ZMAPSTRAND_NONE]    = featureset_data.curr_forward_col;
  featureset_data.feature_stack.set_features[ZMAPSTRAND_FORWARD] = featureset_data.curr_forward_col;
  featureset_data.feature_stack.set_features[ZMAPSTRAND_REVERSE] = featureset_data.curr_reverse_col;

  /* Now draw all the features in the column. */
  g_hash_table_foreach(feature_set->features, ProcessFeature, &featureset_data) ;
  {
    char *str = g_strdup_printf("Processed %d features",featureset_data.feature_count);
    zMapStopTimer("DrawFeatureSet",str);
    g_free(str);
#if MODULE_STATS
    time = zMapElapsedSeconds - time;
    printf("Draw featureset %s: %d features in %.3f seconds\n", g_quark_to_string(feature_set->unique_id),featureset_data.feature_count,time);
#endif
  }


#if MH17_CODE_DOES_NOTHING
  if (featureset_data.feature_count > 0)
    {
      ZMapWindowContainerGroup column_container_parent;

      if ((column_container_parent = forward_container) || (column_container_parent = reverse_container))
        {
          ZMapWindowContainerGroup block_container;

          block_container = zmapWindowContainerUtilsGetParentLevel(column_container_parent,
                                                                   ZMAPCONTAINER_LEVEL_BLOCK);

        }
    }
#endif

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
      /* forward col and strand separator */

      ZMapStyleColumnDisplayState display = ZMAPSTYLE_COLDISPLAY_INVALID ;

      /* We should be bumping columns here if required... */
      if (bump_required && view_feature_set)
        {
          ZMapStyleBumpMode bump_mode ;

          //            zMapStartTimer("DrawFeatureSet","Bump fwd");

          if ((bump_mode =
               zmapWindowContainerFeatureSetGetBumpMode((ZMapWindowContainerFeatureSet)forward_container))
              > ZMAPBUMP_UNBUMP)
            {
              zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(forward_col_wcp), bump_mode, ZMAPWINDOW_COMPRESS_ALL) ;
            }

          //            zMapStopTimer("DrawFeatureSet","Bump fwd");
        }

      /* try 3 frame stuff here...more complicated */
      if (zmapWindowContainerFeatureSetIsFrameSpecific(ZMAP_CONTAINER_FEATURESET(forward_col_wcp), NULL))
        {
          if (IS_3FRAME(window->display_3_frame))
            {
              if (zmapWindowColumnIs3frameDisplayed(window, forward_col_wcp))
                display = ZMAPSTYLE_COLDISPLAY_SHOW ;
              else
                display = ZMAPSTYLE_COLDISPLAY_HIDE ;
            }
        }

      /* Some columns are hidden initially, could be mag. level, 3 frame only display or
       * set explicitly in the style for the column. */

      zmapWindowColumnSetState(window, forward_col_wcp, display, redraw_needed) ;

    }

  if (reverse_col_wcp)
    {
      /* We should be bumping columns here if required... */
      if (bump_required && view_feature_set)
        {
          ZMapStyleBumpMode bump_mode ;

          if ((bump_mode =
               zmapWindowContainerFeatureSetGetBumpMode( (ZMapWindowContainerFeatureSet) reverse_container)) > ZMAPBUMP_UNBUMP)
            {
              zmapWindowColumnBumpRange(FOO_CANVAS_ITEM(reverse_col_wcp), bump_mode, ZMAPWINDOW_COMPRESS_ALL) ;
            }
        }
      /* Some columns are hidden initially, could be mag. level, 3 frame only display or
       * set explicitly in the style for the column. */
      zmapWindowColumnSetState(window, reverse_col_wcp, ZMAPSTYLE_COLDISPLAY_INVALID, FALSE) ;
    }

  return featureset_data.feature_count;
}





gboolean zmapWindowRemoveIfEmptyCol(FooCanvasGroup **col_group)
{
  ZMapWindowContainerGroup container;
  gboolean removed = FALSE ;

  if (ZMAP_IS_CONTAINER_GROUP(*col_group))
    {
      container = ZMAP_CONTAINER_GROUP(*col_group);

      if ((!zmapWindowContainerHasFeatures(container)) &&
          (!zmapWindowContainerFeatureSetShowWhenEmpty(ZMAP_CONTAINER_FEATURESET(container))))
        {
          container = zmapWindowContainerGroupDestroy(container) ;

          *col_group = (FooCanvasGroup *)container;

          removed = TRUE ;
        }
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
  /* offset root group so that block starts at 0,0 (well sort of) */
  canvas_data.curr_x_offset = -(window->config.align_spacing + window->config.block_spacing);
  canvas_data.full_context  = full_context;
  canvas_data.frame_mode_change = TRUE ;       // refer to comment in feature_set_matches_frame_drawing_mode()

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(window->feature_root_group) ;


  zMapFeatureContextExecuteComplete((ZMapFeatureAny)full_context,
                                    ZMAPFEATURE_STRUCT_FEATURESET,
                                    windowDrawContextCB,
                                    NULL, &canvas_data);

  return ;
}


/* columns get created if they are to be drawn but may contain no features
   in which case depending on the style they should be hidden
   featuresets are created from GFF if a feature is defined so will not exist if empty
   but to record failed requests when operating the load columns dialog
   we need to have the featureset in the feature context to record what has been loaded
   so these get created within justMergeContext() if they do not exist.
   So we have to hide empty columns after displaying all the featuresets
*/
static void hideEmpty(ZMapWindow window, const char *where)
{
  zmapWindowContainerUtilsExecute(window->feature_root_group,
                                  ZMAPCONTAINER_LEVEL_FEATURESET,
                                  hideEmptyCB,
                                  window);

  zmapWindowFullReposition(window->feature_root_group,TRUE, where);

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


// CHECK WHERE THIS IS CALLED FROM.....   
void zmapWindowDrawSplices(ZMapWindow window, GList *highlight_features, int seq_start, int seq_end)
{
  FooCanvasGroup *focus_column ;
  FooCanvasItem *focus_item  ;

  if ((focus_column = zmapWindowFocusGetHotColumn(window->focus))
      && (focus_item = zmapWindowFocusGetHotItem(window->focus))
      && (highlight_features
          || (highlight_features = zmapWindowFocusGetFeatureList(window->focus))))
    {
      ZMapWindowContainerFeatureSet focus_container ;
      gboolean result ;

      focus_container = (ZMapWindowContainerFeatureSet)focus_column ;

      if ((result = zMapWindowContainerFeatureSetFilterFeatures(ZMAP_CANVAS_MATCH_PARTIAL,
                                                                0,
                                                                ZMAP_CANVAS_FILTER_PARTS,
                                                                ZMAP_CANVAS_FILTER_PARTS,
                                                                ZMAP_CANVAS_ACTION_HIGHLIGHT_SPLICE,
                                                                ZMAP_CANVAS_TARGET_ALL,
                                                                focus_container,
                                                                NULL,
                                                                NULL,
                                                                seq_start, seq_end)))
        {
          zmapWindowFullReposition(window->feature_root_group, TRUE, "col filter") ;

          window->splice_highlight_on = TRUE ;
        }
     }

  return ;
}







/*
 *                          internal functions
 */


static void hideEmptyCB(ZMapWindowContainerGroup container, FooCanvasPoints *points,
                        ZMapContainerLevelType level, gpointer user_data)
{
  switch(level)
    {
    case ZMAPCONTAINER_LEVEL_FEATURESET:
      {
          if (!zmapWindowContainerHasFeatures(container) &&
              !zmapWindowContainerFeatureSetShowWhenEmpty(ZMAP_CONTAINER_FEATURESET(container)))
              {
            zmapWindowColumnHide((FooCanvasGroup *)container) ;
              }
      }
      break ;
    default:
      break ;
    }

  return ;
}



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

  for (l = canvas_data->masked ; l ; l = l->next)
    {
      if (g_list_find(cfs->featuresets, l->data))
        {
          // this does the lot!
          // mh17: cannot believe my luck -> or is it skill??
          zMapWindowContainerFeatureSetShowHideMaskedFeatures(cfs, !cfs->masked, TRUE) ;

          break;
        }
    }
}

void zMapWindowDrawContext(ZMapCanvasData     canvas_data,
                           ZMapFeatureContext full_context,
                           ZMapFeatureContext diff_context,
                           GList *masked)
{

  canvas_data->curr_x_offset = -(canvas_data->window->config.align_spacing
                                 + canvas_data->window->config.block_spacing);

  /* Full Context to attach to the items.
   * Diff Context feature any (aligns,blocks,sets) are destroyed! */
  canvas_data->full_context  = full_context;

  canvas_data->masked = masked;

  canvas_data->feature_count = 0;


  /* we iterate through the window's canvas data to mask existing features */
  zmapWindowContainerUtilsExecute(canvas_data->window->feature_root_group,
                                  ZMAPCONTAINER_LEVEL_FEATURESET,
                                  container_mask_cb, canvas_data);


  zMapLogTime(TIMER_DRAW_CONTEXT,TIMER_START,0,"");

  /* We iterate through the diff context to draw new data */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)diff_context,
                                    ZMAPFEATURE_STRUCT_FEATURESET,
                                    windowDrawContextCB,
                                    NULL,
                                    canvas_data);

  zMapLogTime(TIMER_DRAW_CONTEXT,TIMER_STOP,canvas_data->feature_count,"");

  //  hideEmpty(canvas_data->window);        /* done by caller */

  return ;
}


/* eg after changing the style, we remove the column from the canvas and draw it again to handle stranding changes */
void zmapWindowRedrawFeatureSet(ZMapWindow window, ZMapFeatureSet featureset)
{
  ZMapCanvasDataStruct canvas_data = {NULL};
  ZMapFeatureContext full_context;

  full_context = window->feature_context;

  canvas_data.window        = window;
  canvas_data.canvas        = window->canvas;
  canvas_data.curr_x_offset = -(window->config.align_spacing + window->config.block_spacing);
  canvas_data.full_context  = full_context;
  canvas_data.frame_mode_change = FALSE;       // refer to comment in feature_set_matches_frame_drawing_mode()

  canvas_data.curr_root_group = zmapWindowContainerGetFeatures(window->feature_root_group) ;

#if 0
  // pointers are all wrong after destroy

  ZMapFeatureAlignment align;
  ZMapFeatureBlock block;
  ZMapFeatureContext diff_context = NULL;
  gboolean is_master = FALSE;

  block = (ZMapFeatureBlock) featureset->parent;
  align = (ZMapFeatureAlignment) block->parent;

  is_master = (full_context->master_align == align);

  block = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block);
  align = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)align);
  diff_context = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)full_context);


  zMapFeatureContextAddAlignment(diff_context, align, is_master);
  zMapFeatureAlignmentAddBlock(align, block);

  //  must not copy else the data we display will be freed
  //  featureset = zMapFeatureSetCopy(featureset);
  zMapFeatureBlockAddFeatureSet(block, featureset);

  /* have to do this to run the create column stuff */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny)diff_context,
                                    ZMAPFEATURE_STRUCT_FEATURESET,
                                    windowDrawContextCB,
                                    NULL, &canvas_data);

  zMapFeatureBlockRemoveFeatureSet(block, featureset);

  zMapFeatureContextDestroy(diff_context, TRUE);

#else

  canvas_data.this_featureset_only = featureset;

  /* have to do this to run the create column stuff */
  zMapFeatureContextExecuteComplete((ZMapFeatureAny) full_context,
                                    ZMAPFEATURE_STRUCT_FEATURESET,
                                    windowDrawContextCB,
                                    NULL, &canvas_data);

#endif
  hideEmpty(window, "redraw featureset");
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
#ifdef MH17_NEVER_INCLUDE_THIS_CODE
              zMapLogMessage("3F1: hiding %s", g_quark_to_string(container_set->unique_id)) ;
#endif
              // printf("3F1: hiding %s\n", g_quark_to_string(container_set->unique_id)) ;
              //              if (window->display_3_frame)

#if HIDE_3F_COLUMN
              zmapWindowColumnHide((FooCanvasGroup *)container) ;
#else
              removeAllFeatures(window, container_set) ;
              zmapWindowContainerFeatureSetRemoveAllItems(container_set) ;
              gtk_object_destroy((GtkObject *) container_set);
#endif
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
#ifdef MH17_NEVER_INCLUDE_THIS_CODE
                  zMapLogMessage("3F3: hiding %s", g_quark_to_string(container_set->unique_id)) ;
#endif

#if HIDE_3F_COLUMN
                  // printf("3F3: hiding %s\n", g_quark_to_string(container_set->unique_id)) ;
                  zmapWindowColumnHide((FooCanvasGroup *)container) ;
#else
                  removeAllFeatures(window, container_set) ;
                  zmapWindowContainerFeatureSetRemoveAllItems(container_set) ;
                  gtk_object_destroy((GtkObject *) container_set);
#endif
                }
            }
        }
    }

  return ;
}



/* MH17: remove all featuresets in the column, and all the features
 * let's use the list of featuresets and the hash functions to do the work.
 */
static void removeAllFeatures(ZMapWindow window, ZMapWindowContainerFeatureSet container_set)
{
  GList *l;

  for(l = container_set->featuresets;l;l = l->next)
    {
      /* NOTE this call removes the features and not the set */

      zmapWindowFToIRemoveSet(window->context_to_item,
                              container_set->align_id,
                              container_set->block_id,
                              GPOINTER_TO_UINT(l->data),
                              container_set->strand,
                              container_set->frame,TRUE) ;
    }
}






static FooCanvasGroup *produce_column(ZMapCanvasData  canvas_data,
                                      ZMapWindowContainerFeatures container,
                                      GQuark          feature_set_id,
                                      ZMapStrand      column_strand,
                                      ZMapFrame       column_frame,
                                      ZMapFeatureSet feature_set)
{
  FooCanvasGroup *new_column = NULL;

  if (!canvas_data || !container)
    return new_column ;

  new_column = find_or_create_column(canvas_data, container, feature_set_id,
                                     column_strand, column_frame, feature_set, TRUE) ;


#if ED_G_NEVER_INCLUDE_THIS_CODE

  /* We need the feature_set to be attached to the column in find_or_create_column().... */
  /* SO I THINK THIS ROUTINE CAN NOW BE REMOVED...IT DOESN'T ADD ANYTHING......... */

  if (new_column)
    {
      zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup) new_column, (ZMapFeatureAny) feature_set);

#if MH17_PRINT_CREATE_COL
      printf("produced column  %s\n",
             g_quark_to_string(zmapWindowContainerFeatureSetGetColumnId((ZMapWindowContainerFeatureSet) new_column)));
#endif
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return new_column ;
}



static gboolean add_featureset_style_to_column(ZMapFeatureColumn column, ZMapFeatureSet feature_set)
{
  gboolean result = FALSE,
    item_present = FALSE ;

  if (!column || !column->style || !feature_set || !feature_set->style)
    return result ;

  /*
   * Check whether the item is present first.
   */
  GList *l = NULL ;
  for (l=column->style_table; l!=NULL ; l=l->next)
    {
      if (l->data == feature_set->style)
        {
          item_present = TRUE ;
          break ;
        }
    }

  /*
   * If not then add it, and reconcile possible differences.
   */
  if (!item_present)
    {
      column->style_table = g_list_append(column->style_table, (gpointer) feature_set->style);

      /*
       * Deal with one possible case of bump_mode only.
       */
      if (column->style->default_bump_mode == ZMAPBUMP_OVERLAP &&
          feature_set->style->default_bump_mode == ZMAPBUMP_ALL)
        column->style->default_bump_mode = ZMAPBUMP_ALL ;

      result = TRUE ;

    }
  else
    {
      result = TRUE ;
    }

  return result ;
}


static FooCanvasGroup *find_or_create_column(ZMapCanvasData  canvas_data,
                                             ZMapWindowContainerFeatures strand_container,
                                             GQuark          feature_set_id,
                                             ZMapStrand      column_strand,
                                             ZMapFrame       column_frame,
                                             ZMapFeatureSet feature_set,
                                             gboolean        create_if_not_exist)
{
  FooCanvasGroup *column = NULL ;
  FooCanvasGroup *tmp_column = NULL ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;
  ZMapWindow window ;
  double top, bottom ;
  FooCanvasItem *existing_column_item ;
  gboolean valid_strand = FALSE ;
  gboolean valid_frame  = FALSE ;
  GQuark column_id = feature_set_id;
  GQuark display_id = feature_set_id;
  ZMapFeatureColumn f_col = NULL;
  gboolean add_to_hash = FALSE;


  /* This surely should be fixed by NOT calling this function if either of these are NULL !! */
  if (!canvas_data || !strand_container)
    return column ;

  window = canvas_data->window ;
  alignment = canvas_data->curr_alignment ;
  block = canvas_data->curr_block ;

#define MH17_PRINT_CREATE_COL        0

  /* MH17: map feature sets from context to canvas columns
   * refer to zmapGFF2parser.c/makeNewFeature()
   */
  if(!window->context_map->featureset_2_column)
    window->context_map->featureset_2_column = g_hash_table_new(NULL,NULL);

  {
    ZMapFeatureSetDesc set_data ;

    set_data = (ZMapFeatureSetDesc)g_hash_table_lookup(window->context_map->featureset_2_column, GUINT_TO_POINTER(feature_set_id));
    {
      column_id = set_data->column_id;      /* the display column as a key */
      display_id = set_data->column_ID;
      f_col = (ZMapFeatureColumn)g_hash_table_lookup(window->context_map->columns,GUINT_TO_POINTER(column_id));
    }
    /* else we use the original feature_set_id
       which should never happen as the view creates a 1-1 mapping regardless */
  }
  /* but then we can't as we need the column style to work out whether to create it */


  if(!f_col)
    {
      zMapLogWarning("No column defined for featureset \"%s\"\n", g_quark_to_string(feature_set_id));
      return NULL;
    }

  if ((existing_column_item = zmapWindowFToIFindItemColumn(window,window->context_to_item,
                                                           alignment->unique_id,
                                                           block->unique_id,
                                                           feature_set_id,
                                                           column_strand,
                                                           column_frame)))
    {
      tmp_column = FOO_CANVAS_GROUP(existing_column_item) ;
    }
  else if ((existing_column_item = zmapWindowContainerFeatureSetFindCanvasColumn(window->feature_root_group,
                                                                                 alignment->unique_id,
                                                                                 block->unique_id,
                                                                                 column_id,
                                                                                 column_strand,
                                                                                 column_frame)))
    {
      /* if the column exists but this feature_set is not in it (col has another featureset)
       * then we need to add this feature_set to the hash. */

      tmp_column = FOO_CANVAS_GROUP(existing_column_item) ;

      add_to_hash = TRUE;
    }
  else if (create_if_not_exist)
    {
      /* The column doesn't exist so create it and we need to add feature_set to hash. */

      top    = block->block_to_sequence.block.x1 ;
      bottom = block->block_to_sequence.block.x2 ;
      zmapWindowSeq2CanExtZero(&top, &bottom) ;

      if (column_strand == ZMAPSTRAND_FORWARD)
        valid_strand = TRUE;
      else if (column_strand == ZMAPSTRAND_REVERSE)
        valid_strand = (zMapStyleIsStrandSpecific(f_col->style) && zMapStyleIsShowReverseStrand(f_col->style));

      if (column_frame == ZMAPFRAME_NONE)
        valid_frame = TRUE;
      else if (column_strand == ZMAPSTRAND_FORWARD || window->show_3_frame_reverse)
        valid_frame = zMapStyleIsFrameSpecific(f_col->style);

      if (valid_frame && valid_strand)
        {
          /* need to create the column */
          if ((tmp_column = createColumnFull(strand_container,
                                             window, alignment, block,
                                             feature_set,
                                             display_id, column_id,
                                             column_strand, column_frame, FALSE,
                                             0.0, top, bottom)))
            {
              add_to_hash = TRUE;
            }
          else
            {
              zMapLogWarning("Column '%s', frame '%d', strand '%d', not created\n",
                             g_quark_to_string(feature_set_id), column_frame, column_strand);
            }
        }
    }

  if (tmp_column)
    {
      /* Always attach the new feature set to the column. */
      /* creating a new column passes through... */
      zmapWindowContainerAttachFeatureAny((ZMapWindowContainerGroup)tmp_column, (ZMapFeatureAny)feature_set) ;
      column = tmp_column ;

      if (!add_featureset_style_to_column(f_col, feature_set))
        {
          zMapLogWarning("Could not add featureset style to column %d\n",
                             g_quark_to_string(column_id));
        }
    }

  if (add_to_hash)
    {
      ZMapWindowContainerFeatureSet container_set ;

#if FEATURESET_AS_COLUMN
      gboolean status;

      status = zmapWindowFToIAddSet(window->context_to_item,
                                    alignment->unique_id,
                                    block->unique_id,
                                    feature_set_id,
                                    column_strand, column_frame,
                                    FOO_CANVAS_GROUP(tmp_column)) ;
#endif

      container_set = (ZMapWindowContainerFeatureSet) tmp_column;

      /* SHOULD BE IN zmapWindowContainerFeatureset.c..... */
      if (!g_list_find(container_set->featuresets, GUINT_TO_POINTER(feature_set_id)))
        {
          container_set->featuresets = g_list_prepend(container_set->featuresets,
                                                      GUINT_TO_POINTER(feature_set_id)) ;
        }
    }

  return column ;
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
                                                                                     ZMAPSTRAND_FORWARD, frame, feature_set)))
    {
      *fwd_col_out = set_column;
      found_one    = TRUE;
#if MH17_PRINT_CREATE_COL
      printf("picked fwd column  %s\n",
             g_quark_to_string(zmapWindowContainerFeatureSetGetColumnId((ZMapWindowContainerFeatureSet) set_column)));
#endif

    }

  /* reverse */
  if (rev_col_out && canvas_data->curr_reverse_group && (set_column = produce_column(canvas_data,
                                                                                     canvas_data->curr_reverse_group,
                                                                                     canvas_data->curr_set->unique_id,
                                                                                     ZMAPSTRAND_REVERSE, frame, feature_set)))
    {
      *rev_col_out = set_column;
      found_one    = TRUE;
#if MH17_PRINT_CREATE_COL
      printf("picked rev column  %s\n",
             g_quark_to_string(zmapWindowContainerFeatureSetGetColumnId((ZMapWindowContainerFeatureSet) set_column)));
#endif
    }

  return found_one;
}

static ZMapFeatureContextExecuteStatus windowDrawContextCB(GQuark   key_id,
                                                           gpointer data,
                                                           gpointer user_data,
                                                           char   **error_out)
{
  ZMapCanvasData canvas_data = (ZMapCanvasData)user_data;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapWindow          window = canvas_data->window;
  ZMapFeatureAlignment feature_align = NULL ;
  ZMapFeatureBlock     feature_block = NULL ;
  ZMapFeatureSet         feature_set = NULL ;
  ZMapFeatureLevelType feature_type = ZMAPFEATURE_STRUCT_INVALID ;
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
        ZMapWindowContainerGroup align_parent = NULL;
        FooCanvasItem  *align_hash_item = NULL;
        double x, y;
        int start,end;

        feature_align = (ZMapFeatureAlignment)feature_any;
#if MH17_REVCOMP_DEBUG
        zMapLogWarning("drawFeatures align","");
#endif
        /* record the full_context current align, not the diff align which will get destroyed! */
        canvas_data->curr_alignment = zMapFeatureContextGetAlignmentByID(canvas_data->full_context,
                                                                         feature_any->unique_id) ;

        x = canvas_data->curr_x_offset ;
        canvas_data->curr_x_offset += window->config.align_spacing;
        zMapFeatureContextGetMasterAlignSpan(canvas_data->full_context,&start,&end);   // else we get everything offset by the start coord
        y = start;

        my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_root_group), &x, &y) ;

        if ((align_hash_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                                          feature_align->unique_id,
                                                          0, 0, ZMAPSTRAND_NONE,
                                                          ZMAPFRAME_NONE, 0)))
          {
            if (ZMAP_IS_CONTAINER_GROUP(align_hash_item))
              {
                align_parent = (ZMapWindowContainerGroup)(align_hash_item);
              }
          }
        else
          {
            align_parent = zmapWindowContainerGroupCreate((FooCanvasGroup *) canvas_data->curr_root_group,
                                                          ZMAPCONTAINER_LEVEL_ALIGN,
                                                          window->config.block_spacing,
                                                          &(window->colour_alignment),
                                                          NULL);
            g_signal_connect(G_OBJECT(align_parent),
                             "destroy",
                             G_CALLBACK(containerDestroyCB),
                             window) ;


            g_object_set_data(G_OBJECT(align_parent), ITEM_FEATURE_STATS,
                              zmapWindowStatsCreate((ZMapFeatureAny)(canvas_data->curr_alignment))) ;

            g_object_set_data(G_OBJECT(align_parent), ZMAP_WINDOW_POINTER, window) ;

            zmapWindowContainerAlignmentAugment((ZMapWindowContainerAlignment)align_parent,
                                                canvas_data->curr_alignment);

            zmapWindowDrawSetGroupBackground(window, align_parent,
                                             start, end, 1.0,
                                             ZMAP_CANVAS_LAYER_ALIGN_BACKGROUND, &(window->colour_alignment), NULL);

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

        if (align_parent && status == ZMAP_CONTEXT_EXEC_STATUS_OK)
          canvas_data->curr_align_group = zmapWindowContainerGetFeatures(align_parent) ;

        break;
      }

    case ZMAPFEATURE_STRUCT_BLOCK:
      {
        ZMapWindowContainerGroup block_group = NULL;
        //    ZMapWindowContainerGroup forward_group, reverse_group ;
        //        ZMapWindowContainerGroup strand_separator;
        ZMapWindowContainerBlock container_block = NULL;
        FooCanvasItem *block_hash_item = NULL;
        //    GdkColor *for_bg_colour, *rev_bg_colour ;
        double x, y;
        gboolean block_created = FALSE;
        double start, end, height ;

        zMapStartTimer("DrawBlock","");

        feature_block = (ZMapFeatureBlock)feature_any;

#if MH17_REVCOMP_DEBUG
        printf("drawFeatures block %d-%d\n", feature_block->block_to_sequence.block.x1,feature_block->block_to_sequence.block.x2);
#endif

        /* Work out height of block making sure it spans all bases in the block. */
        start = (double) feature_block->block_to_sequence.block.x1 ;
        end = (double) feature_block->block_to_sequence.block.x2 ;
        zmapWindowSeq2CanExt(&start, &end) ;
        height = zmapWindowExt(start, end) ;


        /* record the full_context current block, not the diff block which will get destroyed! */
        canvas_data->curr_block = zMapFeatureAlignmentGetBlockByID(canvas_data->curr_alignment,
                                                                   feature_block->unique_id);

        /* Always set y offset to be top of current block. */
        //        canvas_data->curr_y_offset = feature_block->block_to_sequence.x1 ;

        if ((block_hash_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                                          canvas_data->curr_alignment->unique_id,
                                                          feature_block->unique_id, 0, ZMAPSTRAND_NONE,
                                                          ZMAPFRAME_NONE, 0)))
          {
            if (ZMAP_IS_CONTAINER_GROUP(block_hash_item))
              {
                block_group = (ZMapWindowContainerGroup)(block_hash_item);
#if MH17_REVCOMP_DEBUG
            zMapLogWarning(" old","");
#endif
              }
          }
        else
          {
            block_group = zmapWindowContainerGroupCreate((FooCanvasGroup *) canvas_data->curr_align_group,
                                                         ZMAPCONTAINER_LEVEL_BLOCK,
                                                         window->config.column_spacing,
                                                         &(window->colour_block),
                                                         NULL);

            g_signal_connect(G_OBJECT(block_group),
                             "destroy",
                             G_CALLBACK(containerDestroyCB),
                             window) ;
            block_created = TRUE;
#if MH17_REVCOMP_DEBUG
            zMapLogWarning(" new","");
#endif

            zmapWindowContainerBlockAugment((ZMapWindowContainerBlock)block_group,
                                            canvas_data->curr_block) ;

            g_object_set_data(G_OBJECT(block_group), ITEM_FEATURE_STATS,
                              zmapWindowStatsCreate((ZMapFeatureAny)(canvas_data->curr_block))) ;

            g_object_set_data(G_OBJECT(block_group), ZMAP_WINDOW_POINTER, window) ;

            x = canvas_data->curr_x_offset ;
            canvas_data->curr_x_offset += window->config.block_spacing;
            y = feature_block->block_to_sequence.block.x1 ;

            my_foo_canvas_item_w2i(FOO_CANVAS_ITEM(canvas_data->curr_align_group), &x, &y) ;

            foo_canvas_item_set(FOO_CANVAS_ITEM(block_group),
                                "x",x,
                                "y", y,
                                NULL) ;

            zmapWindowDrawSetGroupBackground(window, block_group,
                                             start, end, 1.0,
                                             ZMAP_CANVAS_LAYER_BLOCK_BACKGROUND, &(window->colour_block), NULL);

            /* Add this block to our hash for going from the feature context to its on screen item. */
            if (!(zmapWindowFToIAddBlock(canvas_data->window->context_to_item,
                                         canvas_data->curr_alignment->unique_id,
                                         key_id,
                                         (FooCanvasGroup *)block_group)))
              {
                status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
                *error_out = g_strdup_printf("Failed to add block '%s' to hash!",
                                             g_quark_to_string(key_id));
              }
          }

        if (block_group)
          {
            container_block = (ZMapWindowContainerBlock)block_group;

            if (status == ZMAP_CONTEXT_EXEC_STATUS_OK)
              {
                canvas_data->curr_block_group = zmapWindowContainerGetFeatures(block_group) ;
              }
          }

        zMapStopTimer("DrawBlock","");

        break;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        FooCanvasGroup *tmp_forward = NULL, *tmp_reverse = NULL ;
        int frame_start, frame_end;
        ZMapFeatureTypeStyle style;

        const char *name, *unique_name ;

        name = g_quark_to_string(feature_any->original_id) ;
        unique_name = g_quark_to_string(feature_any->unique_id) ;


        /* record the full_context current block, not the diff block which will get destroyed! */
        canvas_data->curr_set = zMapFeatureBlockGetSetByID(canvas_data->curr_block, feature_any->unique_id);

        if (!canvas_data->curr_set)
          {
            char *x = g_strdup_printf("cannot find set %s, available are: ",
                                      g_quark_to_string(feature_any->unique_id));
            {
              GList *l;
              char *x;

              zMap_g_hash_table_get_keys(&l,canvas_data->curr_block->feature_sets);
              for(;l;l = l->next)
                {
                  char *y = (char *) g_quark_to_string(GPOINTER_TO_UINT(l->data));
                  x = g_strconcat(x," ",y,NULL);
                }
            }

            zMapLogWarning(x,"");
          }
        //printf("DrawFeatureSet %s, context = %p\n",g_quark_to_string(feature_any->unique_id),canvas_data->full_context);

        /* Don't attach this feature_set to anything. It is
         * potentially part of a _diff_ context in which it might be a
         * copy of the view context's feature set.  It should also get
         * destroyed with the diff context, so be warned. */
        feature_set = (ZMapFeatureSet)feature_any;

        style = zMapWindowGetSetColumnStyle(window,feature_set->unique_id);
        if(!style)
          {
            /* MH17: there is something very odd going on here, both these functions are identical */
            /* they are both very short and appear next to each other in WindowUtils.c */
            /* Ashley no: there is one very small difference */

            /* for special columns eg locus we may not have a mapping */
            style = zMapWindowGetColumnStyle(window,feature_set->unique_id);
          }

        if(!style)
          {
            /* for autoconfigured columns we have to patch up a few data structs
             * that are needed by various bits of code scattered all over the place
             * that are assumed to have been set up before requesting the data
             * and they are assumed to have been copied to some other place at some time
             * in between startup, requesting data, getting data and displaying it
             *
             * what's below is in repsonse to whatever errors and assertions happened
             * it's called 'design by experiment'
             */
            style = feature_set->style;        /* eg for an auto configured featureset with a default style */
            /* also set up column2styles */
            if(style)
              {
                ZMapFeatureColumn f_col;
                ZMapFeatureSetDesc f2c;

                /* createColumnFull() needs a style table, although the error is buried in zmapWindowUtils.c */
                zMap_g_hashlist_insert(window->context_map->column_2_styles,
                                       feature_set->unique_id,     // the column
                                       GUINT_TO_POINTER(style->unique_id)) ;  // the style


                /* find_or_create_column() needs f_col->style */
                f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(window->context_map->featureset_2_column,GUINT_TO_POINTER(feature_set->unique_id));
                f_col = (ZMapFeatureColumn)g_hash_table_lookup(window->context_map->columns,GUINT_TO_POINTER(f2c->column_id));
                if(f_col)
                  {
                    if(!f_col->style)
                      f_col->style = style;
                    if(!f_col->style_table)
                      f_col->style_table = g_list_append(f_col->style_table, (gpointer) style);
                  }

              }
          }

        if(!style)
          {
            zMapLogCritical("no column style for featureset \"%s\"",g_quark_to_string(feature_set->unique_id));
            break;
          }
        canvas_data->style = style;

        if(!feature_set->style)                /* eg from a featureset with no features.... how can it exist?  */
          {
            /* mh17: it's very odd. this has suddenly started crashing using data that used to work */
            /* maybe we could look up the style in window->context_map->source_to_sourcedata
               and then go looking for the style but this should have been done already ?? */
            feature_set->style = style;
          }

        /* Default is to draw one column... */
        frame_start = ZMAPFRAME_NONE;
        frame_end   = ZMAPFRAME_NONE;

        /* translation should just be one col..... */
        if ((!(canvas_data->frame_mode_change)
             || feature_set_matches_frame_drawing_mode(window, style, &frame_start, &frame_end)))
          {

            if(!(canvas_data->this_featureset_only)  || (canvas_data->this_featureset_only == feature_set))
              {
                int i, got_columns = 0;
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

                        zMapStartTimer("DrawFeatureSet",g_quark_to_string(feature_set->unique_id));

                        canvas_data->feature_count += zmapWindowDrawFeatureSet(window,
                                                                               //window->context_map->styles,
                                                                               feature_set,
                                                                               tmp_forward,
                                                                               tmp_reverse,
                                                                               canvas_data->current_frame, FALSE);
                        // causes screen refresh thrash: canvas_data->frame_mode_change) ;

                        zMapStopTimer("DrawFeatureSet", g_quark_to_string(feature_set->unique_id));
                      }
                    i++;
                  }
                while( i <= frame_end);
                //                    while(got_columns && i <= frame_end);
              }
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
        zMapWarnIfReached();
        break;
      }
    }
#if MH17_REVCOMP_DEBUG
  if(feature_type != ZMAPFEATURE_STRUCT_FEATURE)
    zMapLogWarning(" -> status %s",status == ZMAP_CONTEXT_EXEC_STATUS_OK ? "OK" : "error");
#endif

  return status;
}



/* function to obtain and step through a list of styles "attached" to
 * a column, filtering for frame specificity, before returning the
 * frame start and end, to be used in a for() loop when splitting a
 * feature set into separate, or not so, framed columns */
static gboolean feature_set_matches_frame_drawing_mode(ZMapWindow window,
                                                       ZMapFeatureTypeStyle style,
                                                       int *frame_start_out,
                                                       int *frame_end_out)
{
  gboolean matched = FALSE ;
  int frame_start, frame_end;
  gboolean frame_specific = FALSE;
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID;

  /* Default is not to draw more than one column... */
  frame_start = ZMAPFRAME_NONE;
  frame_end   = ZMAPFRAME_NONE;

  frame_specific = zMapStyleIsFrameSpecific(style);
  zMapStyleGetStrandAttrs(style,NULL,NULL,&frame_mode);

  if (frame_specific)
    {

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

  if (frame_start_out)
    *frame_start_out = frame_start ;
  if (frame_end_out)
    *frame_end_out = frame_end ;

  return matched ;
}




GQuark zMapWindowGetFeaturesetContainerID(ZMapWindow window, GQuark featureset_id)
{
  GQuark container_id = featureset_id ;
  ZMapFeatureSetDesc gffset ;


  gffset = (ZMapFeatureSetDesc)g_hash_table_lookup(window->context_map->featureset_2_column,
                                                   GUINT_TO_POINTER(featureset_id)) ;
  if (gffset)
    container_id = gffset->column_id ;

  return container_id ;
}


/* add or update an empty CanvasFeatureset that covers the group extent
 * Formerly all columns had foo backgrounds so they could be clicked on.
 * We could use existing CanvasFeaturesets to draw backgrounds but there's two problems:
 * - we may have an empty column for no CanvasFeatureset (esp strand separator)
 * - we may have overlapping CanvasFeaturesets (wiggle plots) that would hide each other
 * so boringly we have to add a CanvasFeatureset to cover a column (group) background
 * the good news is that these are relatively easy to resize to a containing group
 * NOT TRUE: instead of blindly adding backgrounds as before we only do so if they are a diff colour from the container's container
 * there's some race condition w/ fresh backgrounds as update has not been doem so a redraw seems to fail
 * ways round this:
 * - always draw backgrouns even if same colour (works but we get flicker on LHS)
 * always draw but hide if same colour as parent
 */
FooCanvasItem *zmapWindowDrawSetGroupBackground(ZMapWindow window, ZMapWindowContainerGroup container,
                                                int start, int end, double width,
                                                gint layer, GdkColor *fill_col, GdkColor *border_col)
{
  static ZMapFeatureTypeStyle style = NULL;
  GList *l;
  GQuark id;
  const char *x,*y,*name = "?";
  ZMapWindowFeaturesetItem cfs = NULL;
  FooCanvasItem *foo = (FooCanvasItem *) container;
  FooCanvasGroup *group = (FooCanvasGroup *) container;
  GdkColor *parent_fill = NULL, *parent_border = NULL;
  ZMapWindowContainerGroup parent = (ZMapWindowContainerGroup) (container->__parent__.item.parent);
  gboolean show = FALSE;

  if(!style)
    {
      /* hard coded, it's not related to features at all */
      style = zMapStyleCreate("background", "internal style used to draw backgrounds");

      g_object_set(G_OBJECT(style),
                   ZMAPSTYLE_PROPERTY_MODE, ZMAPSTYLE_MODE_PLAIN,
                   ZMAPSTYLE_PROPERTY_COLOURS, "normal fill white; normal border black",
                   NULL);
    }


  if(parent && ZMAP_IS_CONTAINER_GROUP(parent))
    {
      parent_fill = parent->background_fill;
      parent_border = parent->background_border;
    }

  /*
   * add an empty CanvasFeatureset to paint the background
   * can-t use an existing one as:
   * a) there might not be one (strand sep)
   * b) there may be several overlapping (wiggle plots)
   *
   * for columns container is a ContainerFeatureset that has typically one CanvasFeatureset in it
   * but may have several (eg heatmap or other group columns)
   */


  /* get the id of our focus background CanvasFeatureset */
  //        x = g_strdup_printf("%p_%s_%c%c_bgnd", foo->canvas, g_quark_to_string(container->feature_any->unique_id), strand, frame);

  /* this is a bit tedious: strand does not have a feature_any so will go bang
   * instead we just use the address of the containing group which is unique
   * despite being a bit cryptic
   * i wanted to leave strand and frame as args so the we can restore this when strand groups are removed
   * but GroupCreate does not have this info
   * previous backgrounds were anonymous so no big deal
   */
  y = "?";

  if(container->feature_any)
    y = (char *)g_quark_to_string(container->feature_any->unique_id);

  if(ZMAP_IS_CONTAINER_FEATURESET(group))
    name
      = (char *)g_quark_to_string(zmapWindowContainerFeaturesetGetColumnUniqueId((ZMapWindowContainerFeatureSet)group));
  else
    name = g_strdup_printf("%d-%s",container->level, y);

  x = g_strdup_printf("%p_%p_%s_l%x", foo->canvas, group, name,layer);

  //printf("zmapWindowDrawSetGroupBackground %s\n",x);
  id = g_quark_from_string(x);


  for(l = group->item_list; l ; l = l->next)
    {
      ZMapWindowFeaturesetItem temp = (ZMapWindowFeaturesetItem) l->data;

      /* should not be any naked foo items, but groups are not CanvasFeaturesets */
      /* we don't always have a background */
      /* and we expect it to be at the front of the list */

      if(ZMAP_IS_WINDOW_FEATURESET_ITEM(temp))
        {
          if(zMapWindowCanvasFeaturesetGetId(temp) == id)
            {
              cfs = temp;
              break;
            }
        }
    }

#if 0
  NO.... remove background if it's null, don't auto fill_col w/group colour
    set group colour explicitly in call to this
    need for locator dragger
                                  and no doubt lasoo later on

                                  /* clear highlight done by removing colours in which case we restore the default ones if they exist */
                                  if(!fill_col)
                                    fill_col = container->background_fill;
  if(!border_col)
    border_col = container->background_border;
#endif

#if 0
  if(container->feature_any)
    {
      printf("add back %s %d", g_quark_to_string(container->feature_any->unique_id), container->level);
      if(fill_col) printf(" fill_col = %x %x %x", fill_col->red, fill_col->green, fill_col->blue);
      if(parent_fill) printf(" parent fill_col = %x %x %x", parent_fill->red, parent_fill->green, parent_fill->blue);
      if(border_col) printf(" border_col = %x %x %x", border_col->red, border_col->green, border_col->blue);
      if(parent_border) printf(" parent border_col = %x %x %x", parent_border->red, parent_border->green, parent_border->blue);
      printf("\n");
    }
#endif

#if OPTIMISE
  /* there's some very odd stuff going on, we get expose and call gdk_draw_rectangle but it does not paint
   *  but only if the foo canvas item is new, so it should be related to mapped or realize flags
   *  but as we do get exposed and do draw this cannot prevent backgrounds being visible yet they are not
   * (this refers to column backgrounds)
   */
  /* don't paint white on white */
  /* boringly we have to compare RGB and these have not been set in GdkColor.pixel */
  /* if you happen to know a good function to do that then fix this.... */
  if(fill_col && parent_fill && fill_col->red == parent_fill->red && fill_col->green == parent_fill->green && fill_col->blue == parent_fill->blue)
    fill_col = NULL;
  //        if(border_col && parent_border && border_col->red == parent_border->red && border_col->green == parent_border->green && border_col->blue == parent_border->blue)
  //                border_col = NULL;
#endif

  if(fill_col || border_col)
    {
      double x1, x2;

      //printf("try background %s\n", container->feature_any ? g_quark_to_string(container->feature_any->unique_id) : "none");

      /* add a background CanvasFeatureset if it's not there */
      if (!cfs)
        {
          GtkAdjustment *v_adjust ;

          v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

          /* foo canvas update will resize this */
          cfs = (ZMapWindowFeaturesetItem)zMapWindowCanvasItemFeaturesetGetFeaturesetItem(group, id,
                                                                                          v_adjust,
                                                                                          start, end, style,
                                                                                          ZMAPSTRAND_NONE,
                                                                                          ZMAPFRAME_NONE, 0, layer) ;
        }

      if(cfs)
        {
          foo = (FooCanvasItem *) container;

          if(layer & ZMAP_CANVAS_LAYER_STRETCH_X)
            {
              /* get the width of the container, should be set by group update regardless */
              foo_canvas_c2w(foo->canvas, foo->x1, 0, &x1, NULL);
              foo_canvas_c2w(foo->canvas, foo->x2, 0, &x2, NULL);
              width = x2 - x1;
            }
          //printf("set background %s %x %x %x = %f %f %f\n", container->feature_any ? g_quark_to_string(container->feature_any->unique_id) : "none", fill_col->red,fill_col->green,fill_col->blue, foo->x1,foo->x2, width);

          zMapWindowCanvasFeaturesetSetWidth(cfs, width);

          /* set background does a request redraw */
          zMapWindowCanvasFeaturesetSetBackground((FooCanvasItem *)cfs, fill_col, border_col) ;

          show = TRUE;
        }

    }
#if DELETE_IF_NULL
  else if(cfs)
    {
      //printf("del background %s\n", container->feature_any ? g_quark_to_string(container->feature_any->unique_id) : "none");
      //                gtk_object_destroy(GTK_OBJECT(cfs));

      zMapWindowCanvasFeaturesetSetBackground((FooCanvasItem *) cfs, NULL, NULL );

    }
#endif

  if(cfs)
    {
      if(fill_col && parent_fill && fill_col->red == parent_fill->red && fill_col->green == parent_fill->green && fill_col->blue == parent_fill->blue)
        show = FALSE;

      if(show)
        foo_canvas_item_show((FooCanvasItem *) cfs);
      else
        foo_canvas_item_hide((FooCanvasItem *) cfs);
    }

  g_free((char *)x) ;

  return((FooCanvasItem *) cfs);
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
                                        GQuark               column_id,
                                        ZMapStrand           strand,
                                        ZMapFrame            frame,
                                        gboolean             is_separator_col,
                                        double width, double top, double bot)
{
  FooCanvasGroup *group = NULL ;
  ZMapFeatureTypeStyle column_style = NULL;
  GList *style_list = NULL;
  GdkColor *colour = NULL ;
  gboolean proceed = FALSE ;
  ZMapFeatureColumn column = NULL ;

  /* We _must_ have an align and a block... */
  /* ...and either a featureset _or_ a featureset id. */
  if (!align || !block || !column_id)
    return group ;

  if(!original_id)
    {
      original_id = column_id;
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
          zMapWarnIfReached() ;
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

  column_style = zMapWindowGetColumnStyle(window, column_id);
  column = zMapWindowGetColumn(window->context_map,column_id);

  if(column && column->style_id)
    {
      /* for strand separator this is hard coded in zmapView/getIniData(); */
      ZMapFeatureTypeStyle s;

      s = (ZMapFeatureTypeStyle)g_hash_table_lookup(window->context_map->styles,GUINT_TO_POINTER(column->style_id));
      if(s)
        {
          GdkColor *fill_col = NULL;

          zMapStyleGetColours(s, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, &fill_col, NULL, NULL);
          colour = fill_col;
        }
      else
        {
          colour = parent_group->background_fill;
        }
    }

  if(column)
    style_list = column->style_table;

  if(!style_list)
    {
      proceed = FALSE;
      zMapLogMessage("Styles list for Column '%s' not found.",
                     g_quark_to_string(column_id)) ;
    }
  else
    {
      GList *glist;
      proceed = TRUE;

      if((glist = g_list_first(style_list)))
        {
          do
            {
              ZMapFeatureTypeStyle featureset_style;

              featureset_style = ZMAP_FEATURE_STYLE(glist->data);

              if(!zMapStyleIsDisplayable(featureset_style))
                {
                  zMapLogWarning("featureset style %s for %s is not displayable",
                                 g_quark_to_string(featureset_style->unique_id),
                                 g_quark_to_string(column_id));
                  proceed = FALSE; /* not displayable, so bomb out the rest of the code. */
                }
            }
          while(proceed && (glist = g_list_next(glist)));
        }
    }

  /* Can we still proceed? */
  if(proceed)
    {
      int start = block->block_to_sequence.block.x1;
      int end = block->block_to_sequence.block.x2;
      ZMapWindowContainerGroup container;


      /* Only now can we create a group. */
      container = zmapWindowContainerGroupCreate((FooCanvasGroup *) parent_group, ZMAPCONTAINER_LEVEL_FEATURESET,
                                                 window->config.feature_spacing,
                                                 colour, NULL);

      /* reverse the column ordering on the reverse strand */
      if (strand == ZMAPSTRAND_REVERSE)
        foo_canvas_item_lower_to_bottom(FOO_CANVAS_ITEM(container));


      /* THIS WOULD ALL GO IF WE DIDN'T ADD EMPTY COLS...... */
      /* We can't set the ITEM_FEATURE_DATA as we don't have the feature set at this point.
       * This probably points to some muckiness in the code, problem is caused by us deciding
       * to display all columns whether they have features or not and so some columns may not
       * have feature sets. */

      /* Attach data to the column including what strand the column is on and what frame it
       * represents, and also its style and a table of styles, used to cache column feature styles
       * where there is more than one feature type in a column. */
#if ED_G_NEVER_INCLUDE_THIS_CODE
      if(zMapStyleGetMode(feature_set->style) == ZMAPSTYLE_MODE_SEQUENCE)
        printf("Adding column: \"%s\" %s %s %s\n",
               g_quark_to_string(original_id), g_quark_to_string(column_id), zMapFeatureStrand2Str(strand), zMapFeatureFrame2Str(frame)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* needs to accept style_list */

      zmapWindowContainerFeatureSetAugment((ZMapWindowContainerFeatureSet)container, window,
                                           align->unique_id,
                                           block->unique_id,
                                           column_id, original_id,
                                           column_style, strand, frame);

      /* This will create the stats if feature_set != NULL */
      zmapWindowContainerAttachFeatureAny(container, (ZMapFeatureAny)feature_set);


      if(feature_set->style && zMapStyleDisplayInSeparator(feature_set->style))
        {
          FooCanvasItem *feature_set ;

          /* fixed width, colour already set to yellow, but need start and end as there are no features */
          feature_set = zmapWindowDrawSetGroupBackground(window, container,
                                                         start, end, column_style->width,
                                                         ZMAP_CANVAS_LAYER_SEPARATOR_BACKGROUND, colour, NULL) ;

          window->separator_feature_set = ZMAP_WINDOW_FEATURESET_ITEM(feature_set) ;
        }
      else
        {
          /* white background that will not be added as block has a white background */
          zmapWindowDrawSetGroupBackground(window, container,
                                           start, end, 1.0,
                                           ZMAP_CANVAS_LAYER_COL_BACKGROUND, colour, NULL) ;
        }

      g_signal_connect(G_OBJECT(container), "destroy", G_CALLBACK(containerDestroyCB), (gpointer)window) ;

      g_object_set_data(G_OBJECT(container), ZMAP_WINDOW_POINTER, window) ;

      group = (FooCanvasGroup *)container;

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

  zMapReturnIfFail(feature && feature->mode != ZMAPSTYLE_MODE_INVALID && feature->style) ;

  CreateFeatureSetData featureset_data = (CreateFeatureSetData) user_data ;
  ZMapWindow window = featureset_data->window ;
#if CALCULATE_COLUMNS
  FooCanavsGroup * column_group;
#else
  ZMapWindowContainerFeatureSet column_group ;
#endif
  ZMapWindowContainerFeatures features;
  ZMapStrand display_strand ;
  FooCanvasItem *feature_item = NULL;
  ZMapFeatureTypeStyle style = NULL ;

#if MH17_REVCOMP_DEBUG > 1
  if(featureset_data->frame != ZMAPFRAME_NONE)
    printf("ProcessFeature %s %d-%d\n",g_quark_to_string(feature->original_id), feature->x1,feature->x2);
#endif


  if (feature->style)
    style = *feature->style;

  /* if fails: no display. fixed for pipe via GFF2parser, ACE seems to call it???
   * features paint so it musk be ok! *
   * but if a user adds an object and we make a fetaure OTF then no style is attached
   * the above is an optimisation
   */
  if (!style)
    {
      /* should only be a consideration for OTF data w/ no styles?? */
      /* xremote code should set this up anyway */
      /* NOTE there's a knot here
       * to allow column styles to be changed we have features point to the styles in the containing context featureset
       * if we get a feature via XRemote the we don't have that set up as it doesnly go through GFFParser
       * so we have to find the feature's parent anf set the style there
       */
      /* NOTE: 3FT and DNA get supplied by acedbServer and pipeServer and are given temporary styles that are freed
       * which means thet that data is not usable.
       */
      ZMapFeatureSet feature_set = (ZMapFeatureSet)feature->parent ;

      if (!feature_set->style)
        {
          char *err_msg ;

          err_msg = g_strdup_printf("no style for feature set \"%s\", feature \"%s\", (%s)",
                                    g_quark_to_string(feature_set->original_id),
                                    g_quark_to_string(feature->original_id),
                                    g_quark_to_string(feature->unique_id));


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* Actually really annoying for the user so let's not do it... */
          zMapWarning("%s", err_msg) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          zMapLogWarning("%s", err_msg) ;

          zMapLogStack();

          g_free(err_msg) ;
        }

      feature->style = &feature_set->style;
    }

#if MH17_REVCOMP_DEBUG > 1
  if(!style) zMapLogWarning("no style 1 ","");
#endif

  /* Do some filtering on frame and strand */

  /* Find out which strand group the feature should be displayed in. */
  display_strand = zmapWindowFeatureStrand(window, feature) ;


#if CALCULATE_COLUMNS

  /* Caller may not want a forward or reverse strand and this is indicated by NULL value for
   * curr_forward_col or curr_reverse_col */
  if ((display_strand == ZMAPSTRAND_FORWARD && !(featureset_data->curr_forward_col))
      || (display_strand == ZMAPSTRAND_REVERSE && !(featureset_data->curr_reverse_col)))
    {
#if MH17_REVCOMP_DEBUG > 1
      zMapLogWarning("wrong strand 1","");
#endif
      return ;
    }
#else
  features = featureset_data->feature_stack.set_features[display_strand];
  if(!features)
    return;
#endif

  /* If we are doing frame specific display then don't display the feature if its the wrong
   * frame or its on the reverse strand and we aren't displaying reverse strand frames. */
  //if(!strncmp(g_quark_to_string(feature->unique_id),"3 frame",7))
  //        printf("process feature %s: %d/%d (%d)\n",g_quark_to_string(feature->unique_id), featureset_data->frame, zmapWindowFeatureFrame(feature), feature->x1);

  if (featureset_data->frame != ZMAPFRAME_NONE
      && (featureset_data->frame != zmapWindowFeatureFrame(feature)
          || (!(window->show_3_frame_reverse) && display_strand == ZMAPSTRAND_REVERSE)))
    {
#if MH17_REVCOMP_DEBUG > 1
      zMapLogWarning("wrong strand 2","");
#endif
      return ;
    }


#if CALCULATE_COLUMNS
  /* Get the correct column to draw into... */
  if (display_strand == ZMAPSTRAND_FORWARD)
    {
      column_group = zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(featureset_data->curr_forward_col)) ;
    }
  else
    {
      column_group = zmapWindowContainerChildGetParent(FOO_CANVAS_ITEM(featureset_data->curr_reverse_col)) ;
    }
#else
  column_group = featureset_data->feature_stack.set_column[display_strand];
  feature_item = featureset_data->feature_stack.col_featureset[display_strand];
#endif



#if MH17_NOT_NEEDED_FEATURESET_HAS_OWN_COPY_POINTED_AT_BY_FEATURE
  if(style)
    style = zmapWindowContainerFeatureSetStyleFromStyle((ZMapWindowContainerFeatureSet)column_group, style) ;
  else
    g_warning("need a style '%s' for feature '%s'",
              g_quark_to_string(feature->style_id),
              g_quark_to_string(feature->original_id));

#endif

#if MH17_REVCOMP_DEBUG > 1
  if(!style) zMapLogWarning("no style 2","");

#endif

  featureset_data->feature_stack.feature = feature;

  if(style)
    {
      feature_item = zmapWindowFeatureDraw(window,
                                           style, column_group, features,
                                           feature_item, (GCallback)columnBoundingBoxEventCB,
                                           &featureset_data->feature_stack) ;

#if !CALCULATE_COLUMNS
      featureset_data->feature_stack.col_featureset[display_strand] = feature_item;
#endif
    }
  else
    g_warning("definitely need a style for feature '%s'",
              g_quark_to_string(feature->original_id));

  if(feature_item)
    featureset_data->feature_count++;

  return ;
}







/*
 *                           Event handlers
 */


/* Handles events on a column, currently this is only mouse press/release events for
 * highlighting and column menus. */
static gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;


  if (event->type == GDK_ENTER_NOTIFY || event->type == GDK_LEAVE_NOTIFY)
    {
      /* damn....we need to get the parent here to get the column name.... */
      ZMapWindowContainerGroup container_parent;

      container_parent = zmapWindowContainerCanvasItemGetContainer(item);

      event_handled = setColumnTooltip(FOO_CANVAS_ITEM(container_parent), event, data) ;
    }
  else if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS  || event->type == GDK_BUTTON_RELEASE
           || event->type == GDK_MOTION_NOTIFY)
    {
      if (zMapWindowCanvasFeaturesetHasPointFeature(item))
        {
          /* Click was on a feature within a column so forward the event to the feature handling code. */
          event_handled = zmapWindowFeatureItemEventHandler(item, event, data) ;
        }
      else if (event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE)
        {
          /* Click was on a column background, i.e. not on a feature. */
          ZMapWindow window = (ZMapWindow)data ;
          GdkEventButton *but_event = (GdkEventButton *)event ;
          ZMapFeatureSet feature_set = NULL ;
          ZMapWindowContainerFeatureSet container_set;
          ZMapWindowContainerGroup container_parent;

          container_parent = zmapWindowContainerCanvasItemGetContainer(item);

          /* These should go in container some time.... */
          container_set = (ZMapWindowContainerFeatureSet)container_parent ;
          feature_set = zmapWindowContainerFeatureSetRecoverFeatureSet(container_set) ;


          /* Only buttons 1 and 3 are handled. */
          if (event->type == GDK_BUTTON_PRESS && but_event->button == 3)
            {
              /* Do the column menu. */
              if (feature_set)
                {
                  zmapMakeColumnMenu(but_event, window, (FooCanvasItem *) container_set, container_set, NULL) ;

                  event_handled = TRUE ;
                }
            }
          else if (event->type == GDK_BUTTON_RELEASE && but_event->button == 1)
            {
              /* Highlight a column. */
              ZMapWindowSelectStruct select = {(ZMapWindowSelectType)0} ;
              char *clipboard_text = NULL;

              GdkModifierType shift_mask = GDK_SHIFT_MASK,
                control_mask = GDK_CONTROL_MASK,
                alt_mask = GDK_MOD1_MASK,
                meta_mask = GDK_META_MASK,
                unwanted_masks = (GdkModifierType)(GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK
                                                   | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK
                                                   | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK),
                locks_mask ;

              ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED, ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                            ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS} ;


              /* In order to make the modifier only checks work we need to OR in the unwanted masks that might be on.
               * This includes the shift lock and num lock. Depending on the setup of X these might be mapped
               * to other things which is why MODs 2-5 are included This in theory should include the new (since 2.10)
               * GDK_SUPER_MASK, GDK_HYPER_MASK and GDK_META_MASK */
              if ((locks_mask = (GdkModifierType)(but_event->state & unwanted_masks)))
                {
                  shift_mask = (GdkModifierType)(shift_mask | locks_mask) ;
                  control_mask = (GdkModifierType)(control_mask | locks_mask) ;
                  alt_mask = (GdkModifierType)(alt_mask | locks_mask) ;
                  meta_mask = (GdkModifierType)(meta_mask | locks_mask) ;
                }


              /* shift adds to the selection so we don't unhighlight if nothing's there */
              if (!zMapGUITestModifiers(but_event, shift_mask))
                {
                  char *column_name ;
                  GQuark column_id ;

                  /* Swop focus from previous item(s)/columns to this column. */
                  zmapWindowUnHighlightFocusItems(window) ;

                  /* Try unhighlighting dna/translations... NOTE item not needed, used to be container group (item) */
                  zmapWindowItemUnHighlightDNA(window, item) ;
                  zmapWindowItemUnHighlightTranslations(window, item) ;
                  zmapWindowItemUnHighlightShowTranslations(window, item) ;


                  /*! \todo #warning COLUMN_HIGHLIGHT_NEEDS_TO_WORK_WITH_MULTIPLE_WINDOWS */
                  zmapWindowFocusSetHotColumn(window->focus, (FooCanvasGroup *)container_parent, NULL);

                  select.feature_desc.struct_type = ZMAPFEATURE_STRUCT_FEATURESET ;


                  column_id = zmapWindowContainerFeaturesetGetColumnId(container_set) ;
                  column_name = (char *)g_quark_to_string(column_id) ;
                  if (zmapWindowContainerFeatureSetGetBumpMode(container_set) == ZMAPBUMP_UNBUMP)
                    {
                      select.feature_desc.feature_set = g_strdup(column_name) ;
                    }
                  else
                    {
                      GQuark feature_set_id ;
                      char *full_name ;

                      if ((feature_set_id = zMapWindowCanvasFeaturesetGetSetIDAtPos((ZMapWindowFeaturesetItem)item,
                                                                                    but_event->x)))
                        full_name = g_strdup_printf("%s / %s",
                                                    column_name,
                                                    g_quark_to_string(feature_set_id)) ;
                      else
                        full_name = g_strdup(column_name) ;

                      select.feature_desc.feature_set = full_name ;
                    }


                  if (zMapGUITestModifiers(but_event, alt_mask)
                      || zMapGUITestModifiers(but_event, meta_mask))
                    {
                      display_style.coord_frame = ZMAPWINDOW_COORD_NATURAL ;
                      display_style.paste_style = ZMAPWINDOW_PASTE_FORMAT_BROWSER ;
                      display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;
                    }

                  if ((clipboard_text = zmapWindowMakeColumnSelectionText(window, but_event->x, but_event->y,
                                                                          &display_style, container_set)))
                    select.feature_desc.feature_set_description = clipboard_text ;


                  select.type = ZMAPWINDOW_SELECT_SINGLE;

                  /* user clicked on column background - get first canvas item and
                   * if is a CanvasFeatureset consider enabling filtering by score */
                  {
                    select.filter.enable = FALSE;

                    if(ZMAP_IS_WINDOW_FEATURESET_ITEM(item))
                      {
                        ZMapWindowFeaturesetItem fi;
                        ZMapFeatureTypeStyle style;

                        fi = (ZMapWindowFeaturesetItem) item;
                        style = zMapWindowContainerFeatureSetGetStyle(container_set);

                        if (style)
                          {
                            if ( zMapStyleIsFilter(style) )
                              {
                                select.filter.min_val = zMapStyleGetMinScore(style);
                                select.filter.max_val = zMapStyleGetMaxScore(style);
                                select.filter.value = zMapWindowCanvasFeaturesetGetFilterValue((FooCanvasItem *)fi) ;
                                select.filter.n_filtered =
                                  zMapWindowCanvasFeaturesetGetFilterCount((FooCanvasItem *)fi) ;
                                select.filter.column =  item;                /* needed for re-bumping */
                                select.filter.featureset = fi;
                                select.filter.enable = TRUE;
                                select.filter.window = window;

                                select.filter.func = zMapWindowCanvasFeaturesetFilter;
                              }
                          }
                      }
                  }

                  (*(window->caller_cbs->select))(window, window->app_data, (void *)&select) ;

                  /* select data memory should be cleared up here..... */
                  g_free(select.feature_desc.feature_set) ;


                  if(clipboard_text)
                    {
                      zMapGUISetClipboard(window->toplevel, GDK_SELECTION_PRIMARY, clipboard_text) ;

                      g_free(clipboard_text) ;
                    }

                }
              event_handled = TRUE ;
            }
        }
    }

  return event_handled ;
}



/* Implements a tooltip window that shows the column name as the pointer enters a column.
 *
 * With the new version of tooltips as of gtk 2.12 you don't need to allocate/deallocate
 * the tooltip. Tooltips have become "per-widget" and some settings like "time before
 * tooltip is shown" have to be made using a whole new "settings" model.
 *  */
static gboolean setColumnTooltip(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;

  if (event->type == GDK_ENTER_NOTIFY || event->type == GDK_LEAVE_NOTIFY)
    {
      FooCanvas *foo_canvas = item->canvas ;

      if (event->type == GDK_ENTER_NOTIFY)
        {
          ZMapWindowContainerFeatureSet feature_set_container = (ZMapWindowContainerFeatureSet)item ;
          GQuark col_id ;
          char *col_name ;

          col_id = zmapWindowContainerFeaturesetGetColumnId(feature_set_container) ;
          col_name = (char *)g_quark_to_string(col_id) ;

          gtk_widget_set_tooltip_text((GtkWidget *)foo_canvas, col_name) ;
        }
      else
        {
          gtk_widget_set_tooltip_text((GtkWidget *)foo_canvas, NULL) ; /* NULL makes tooltip disappear. */
        }
    }

  return event_handled ;
}








/*
 *                       Menu functions.
 */



/* Read colour information from the configuration file, note that we read _only_ the first
 * colour stanza found in the file, subsequent ones are not read. */
static void setColours(ZMapWindow window)
{
  ZMapConfigIniContext context = NULL;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* THIS ALL NEEDS RATIONALISING BUT WE NEED TO SET OTHER COLOURS SUCH AS
   * LASSO COLOUR, DNA TEXT COLOUR AND SO ON SO THEY SHOW UP AGAINST THE BACKGROUND. */

  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &(window->colour_root)) ;
  gdk_color_parse(ZMAP_WINDOW_BACKGROUND_COLOUR, &window->colour_alignment) ;
  gdk_color_parse(ZMAP_WINDOW_BLOCK_BACKGROUND_COLOUR, &window->colour_block) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  window->colour_root = window->colour_alignment = window->colour_block = window->canvas_background ;
                                                            /* struct copies */


  gdk_color_parse(ZMAP_WINDOW_STRAND_DIVIDE_COLOUR, &window->colour_separator) ;

  /* Get rid of these ?? */
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
  
  gdk_color_parse(ZMAP_WINDOW_RUBBER_BAND, &(window->colour_rubber_band)) ;
  
  //  gdk_color_parse(ZMAP_WINDOW_ITEM_HIGHLIGHT, &(window->colour_item_highlight)) ;
  //  window->highlights_set.item = TRUE ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_0, &(window->colour_frame_0)) ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_1, &(window->colour_frame_1)) ;
  gdk_color_parse(ZMAP_WINDOW_FRAME_2, &(window->colour_frame_2)) ;

  gdk_color_parse(ZMAP_WINDOW_ITEM_EVIDENCE_BORDER, &(window->colour_evidence_border)) ;
  gdk_color_parse(ZMAP_WINDOW_ITEM_EVIDENCE_FILL, &(window->colour_evidence_fill)) ;
  window->highlights_set.evidence = TRUE ;

  if ((context = zMapConfigIniContextProvide(window->sequence->config_file, ZMAPCONFIG_FILE_NONE)))
    {
      char *colour = NULL;
      gboolean truth = FALSE;                /* i always wanted to say that :-) */
      /* actually on a philosophical note 'gboolean' subverts the design of the language
       * and is part of a bug creating meme the reaches the depths of despair when people
       * write things like if(x == TRUE) when in fact there are 4G-2 other equally true values
       * if you had a library that had 1 as TRUE and another as 0xffffffff then you have a problem
       */

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
                                       ZMAPSTANZA_WINDOW_RUBBER_BAND, &colour))
        gdk_color_parse(colour, &window->colour_rubber_band) ;

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
                                       ZMAPSTANZA_WINDOW_FILTERED_COLUMN, &colour))
        {
          gdk_color_parse(colour, &(window->colour_filtered_column)) ;
          window->highlights_set.filtered = TRUE ;
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

      /* why bury this here?
       * we can't set config info glabally due to the design
       * so every time we R click on a feature we'd have to read a very long config file
       * so I'm setting a flag for the lifetime of the window
       * can-t edit styles if we can see no features and this must get called first.
       *
       * it would be more logical to cal this func from myWindowCreate, but previous this was tangled up
       * with FooCanvas map and realise even though we're only reading a configration file.
       */

      window->edit_styles = TRUE;
      if(zMapConfigIniContextGetBoolean(context,ZMAPSTANZA_APP_CONFIG,ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_EDIT_STYLES, &truth))
        {
          window->edit_styles = truth ;
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
  gboolean result = FALSE ;                                    /* Always return FALSE ?? check this.... */
  gboolean log_all = FALSE ;

  if(log_all)
    zMapLogMessage("gobject type = '%s'", G_OBJECT_TYPE_NAME(item));

  /* Some items may not have features attached...e.g. empty cols....I should revisit the empty
   * cols bit, it keeps causing trouble all over the place.....column creation would be so much
   * simpler without it.... */
  if ((ZMAP_IS_CONTAINER_GROUP(item) == TRUE) && (container   = ZMAP_CONTAINER_GROUP(item)))
    {
      context_to_item = window->context_to_item;

      feature_any = container->feature_any;

      switch (container->level)
        {
        case ZMAPCONTAINER_LEVEL_ROOT:
          {
            status = zmapWindowFToIRemoveRoot(context_to_item) ;

            break ;
          }
        case ZMAPCONTAINER_LEVEL_ALIGN:
          {
            status = zmapWindowFToIRemoveAlign(context_to_item, feature_any->unique_id) ;

            break ;
          }
        case ZMAPCONTAINER_LEVEL_BLOCK:
          {
            status = zmapWindowFToIRemoveBlock(context_to_item,
                                               feature_any->parent->unique_id,
                                               feature_any->unique_id) ;

          }
          break ;
        case ZMAPCONTAINER_LEVEL_FEATURESET:
          {
            GList *l;
            container_set = (ZMapWindowContainerFeatureSet)container;

            /* need to remove all the featuresets held in the column
             * fortunately we have a handy list of these
             */
            for(l = container_set->featuresets;l;l = l->next)
              {
                status = zmapWindowFToIRemoveSet(context_to_item,
                                                 container_set->align_id,
                                                 container_set->block_id,
                                                 GPOINTER_TO_UINT(l->data),     /*container_set->unique_id, */
                                                 container_set->strand,
                                                 container_set->frame,FALSE) ;
                /* not sure where the features got deleted from but this used to work */
              }
            g_list_free(container_set->featuresets);
            container_set->featuresets = NULL;

            /* If the focus column goes then so should the focus items as they should be in step. */
            if (window->focus && zmapWindowFocusGetHotColumn(window->focus) == group)
              zmapWindowFocusReset(window->focus) ;

          }
          break ;
        default:
          {
            zMapWarnIfReached() ;
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

  return result ;                                            /* ????? */
}





/****************** end of file ************************************/
