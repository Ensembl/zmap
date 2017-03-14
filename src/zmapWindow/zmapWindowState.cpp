/*  File: zmapWindowState.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: Implements saving state for zmapwindow prior to
 *              operations after which we must restore ourselves.
 *
 * Exported functions: See zmapWindowState.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapWindow_P.hpp>
#include <zmapWindowState.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>

typedef struct
{
  gboolean strand_specific ;
  GQuark align_id, block_id, column_id, fset_id, feature_id ;
  ZMapFrame frame ;
  ZMapStrand strand ;
} SerializedItemStruct ;

typedef struct
{
  SerializedItemStruct item;
  double x1, x2, y1, y2;
  gboolean rev_comp_state;
} ZMapWindowMarkSerialStruct;

typedef struct
{
  SerializedItemStruct item;
  gboolean rev_comp_state;
} ZMapWindowFocusSerialStruct;


/* Problem with these comments is that it doesn't say why we need to be careful.... */
typedef struct
{
  double scroll_x1, scroll_x2;
  double scroll_y1, scroll_y2;

  GtkAdjustment v_adjuster;        /* Be very careful with this! */
  GtkAdjustment h_adjuster;        /* Be very careful with this! */

  int scroll_offset_x, scroll_offset_y;
  gboolean rev_comp_state;
} ZMapWindowPositionStruct, ZMapWindowPosition;

typedef struct
{
  SerializedItemStruct column;
  ZMapStyleBumpMode bump_mode;
  gboolean strand_specific;
} StyleBumpModeStruct;

typedef struct
{
  GArray                *style_bump; /* array of StyleBumpModeStruct */
  ZMapWindowCompressMode compress;
  gboolean               rev_comp_state;
} ZMapWindowBumpStateStruct;

/* _NOTHING_ in this struct (ZMapWindowStateStruct) should refer
 * to dynamic memory elsewhere. This means ...
 * - No FooCanvasItems
 * - No ZMapWindows
 * - No ZMapViews
 * Everything should be serialised to enable reverse complements,
 * window splitting, zooming and any other action where state needs
 * to be kept, but where features might be destroyed, altered or
 * logically be the wrong object to restore state to.
 *************************************/
typedef struct _ZMapWindowStateStruct
{
  ZMapMagic   magic;

  /* state of stuff ... */
  double       zoom_factor;
  ZMapWindowMarkSerialStruct mark;
  ZMapWindowPositionStruct position;
  ZMapWindowFocusSerialStruct focus;
  ZMapWindowBumpStateStruct bump;

  gboolean rev_comp_state;

  /* flags to say which states are set/unset */
  unsigned int zoom_set   : 1 ;
  unsigned int mark_set   : 1 ;
  unsigned int bump_state_set : 1;
  unsigned int focus_items_set : 1;
  unsigned int position_set : 1 ;
  unsigned int in_state_restore : 1 ;
} ZMapWindowStateStruct;

/* Could or should use zmapWindow.c lockedDisplayCB */
typedef struct _QLockedDisplay
{
  ZMapWindow window;
  double x1, y1, x2, y2;
}QLockedDisplayStruct, *QLockedDisplay;


static void state_mark_restore(ZMapWindow window, ZMapWindowMark mark, ZMapWindowMarkSerialStruct *serialized);
static void state_position_restore(ZMapWindow window, ZMapWindowPositionStruct *position);
static void state_focus_items_restore(ZMapWindow window, ZMapWindowFocusSerialStruct *serialized);
static void state_bumped_columns_restore(ZMapWindow window, ZMapWindowBumpStateStruct *serialized);
static void print_position(ZMapWindowPositionStruct *position, const char *from);
static gboolean serialize_item(FooCanvasItem *item, SerializedItemStruct *serialize);
/* update stuff is so that queue doesn't get cleared under the feet of a restore... */
/* Main reason though is to not save whilst doing restore. endless history, no thanks */
static gboolean queue_doing_update(ZMapWindowStateQueue zqueue);
static void mark_queue_updating(ZMapWindowStateQueue zqueue, gboolean update_flag);

static void lockedDisplaySetScrollRegionCB(gpointer key, gpointer value, gpointer user_data);
static void set_scroll_region(ZMapWindow window,
                              double x1, double y1,
                              double x2, double y2);

ZMAP_MAGIC_NEW(window_state_magic_G, ZMapWindowStateStruct) ;

static gboolean window_state_debug_G = FALSE;
#ifdef RDS_DONT_INCLUDE
static gboolean state_restore_copies_G = TRUE;
#endif




/*
 *                      External package routines.
 */



ZMapWindowState zmapWindowStateCreate(void)
{
  ZMapWindowState state = NULL;

  state = g_new0(ZMapWindowStateStruct, 1) ;

  state->magic = window_state_magic_G ;

  return state ;
}

void zmapWindowStateRestore(ZMapWindowState state, ZMapWindow window)
{
  ZMapWindowStateStruct state_copy = {NULL};

  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic))
    return ;

  state_copy = *state;                /* n.b. struct copy */
  state = &state_copy;

  /* So the real requirement of marking the queue as updating is to
   * not save whilst restoring... */
  mark_queue_updating(window->history, TRUE);

    {
      if(state->zoom_set)
        {
          double current, target, factor;
          current = zMapWindowGetZoomFactor(window);
          target  = state->zoom_factor;
          factor  = target / current;
          zMapWindowZoom(window, factor);
        }
      if(state->position_set)
        {
          state_position_restore(window, &(state->position));
        }
    }

  if (state->mark_set)
    {
      state_mark_restore(window, window->mark, &(state->mark)) ;
    }
  else
    {
      zmapWindowMarkReset(window->mark) ;
    }


  if(state->focus_items_set)
    {
      state_focus_items_restore(window, &(state->focus));
    }

  if(state->bump_state_set)
    {
      state_bumped_columns_restore(window, &(state->bump));
    }

  mark_queue_updating(window->history, FALSE);

  zMapWindowRedraw(window);        /* include area off canvas */

  return ;
}

ZMapWindowState zmapWindowStateCopy(ZMapWindowState state)
{
  ZMapWindowState new_state = NULL;

  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic))
    return new_state ;

  new_state = zmapWindowStateCreate();

  memcpy(new_state, state, sizeof(ZMapWindowStateStruct));

  return new_state;
}

ZMapWindowState zmapWindowStateDestroy(ZMapWindowState state)
{
  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic))
    return state ;

  /* Reset magic ptr to invalidate the block. */
  state->magic = NULL ;

  /* ZERO AND FREE */
  ZMAP_PTR_ZERO_FREE(state, ZMapWindowStateStruct);

  return state ;
}

gboolean zmapWindowStateSavePosition(ZMapWindowState state, ZMapWindow window)
{
  GtkAdjustment *v_adjuster, *h_adjuster;
  FooCanvas *canvas;
  GtkScrolledWindow *scrolled_window;

  if(window->canvas && FOO_IS_CANVAS(window->canvas))
    {
      canvas          = FOO_CANVAS(window->canvas);
      scrolled_window = GTK_SCROLLED_WINDOW(window->scrolled_window);

      if((v_adjuster  = gtk_scrolled_window_get_vadjustment(scrolled_window)))
        {
          state->position.v_adjuster = *v_adjuster; /* n.b. struct copy! */
        }

      if((h_adjuster  = gtk_scrolled_window_get_hadjustment(scrolled_window)))
        {
          state->position.h_adjuster = *h_adjuster; /* n.b. struct copy! */
        }

      foo_canvas_get_scroll_offsets(canvas, &(state->position.scroll_offset_x), &(state->position.scroll_offset_y));

      foo_canvas_get_scroll_region(canvas,
                                   &(state->position.scroll_x1),
                                   &(state->position.scroll_y1),
                                   &(state->position.scroll_x2),
                                   &(state->position.scroll_y2));

      state->rev_comp_state = state->position.rev_comp_state = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES);

      if(window_state_debug_G)
        print_position(&(state->position), "save_position");

      state->position_set = TRUE;
    }

  return state->position_set;
}

gboolean zmapWindowStateSaveZoom(ZMapWindowState state, double zoom_factor)
{
  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic))
    return FALSE ;

  state->zoom_set    = 1;
  state->zoom_factor = zoom_factor;

  return state->zoom_set;
}

gboolean zmapWindowStateSaveMark(ZMapWindowState state, ZMapWindow window)
{
  ZMapWindowMark mark = window->mark;
  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic))
    return FALSE ;

  if ((state->mark_set = zmapWindowMarkIsSet(mark)))
    {
      /* Always get the world range, so that if feature can not be found on restore, we can still remark. */
      zmapWindowMarkGetWorldRange(mark,
                                  &(state->mark.x1), &(state->mark.y1),
                                  &(state->mark.x2), &(state->mark.y2));

      state->rev_comp_state = state->mark.rev_comp_state = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES);
    }

  return state->mark_set;
}


/* mh17: a misnomer, it only does one item */
gboolean zmapWindowStateSaveFocusItems(ZMapWindowState state, ZMapWindow window)
{
  gboolean result = FALSE ;
  FooCanvasItem *focus_item ;

  if (!state || !ZMAP_MAGIC_IS_VALID(window_state_magic_G, state->magic) || !window)
    return result ;

  if ((window->focus) && (focus_item = zmapWindowFocusGetHotItem(window->focus)))
    {
      state->focus_items_set = serialize_item(focus_item, &(state->focus.item));
      state->rev_comp_state = state->focus.rev_comp_state = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES);

      result = state->focus_items_set ;
    }

  return result ;
}



static void get_bumped_columns(ZMapWindowContainerGroup container,
                               FooCanvasPoints         *unused_points,
                               ZMapContainerLevelType   level,
                               gpointer                 user_data)
{
  ZMapWindowBumpStateStruct *bump = (ZMapWindowBumpStateStruct *)user_data;

  if(level == ZMAPCONTAINER_LEVEL_FEATURESET)
    {
      StyleBumpModeStruct bump_data = {{0}, (ZMapStyleBumpMode)0, FALSE};
      ZMapWindowContainerFeatureSet container_set;
      ZMapFeatureAny feature_any;
      ZMapFeatureSet fset;

      container_set = (ZMapWindowContainerFeatureSet)container;

      if((feature_any = (ZMapFeatureAny)zmapWindowContainerFeatureSetRecoverFeatureSet(container_set)))
        {
          bump_data.column.align_id   = feature_any->parent->parent->unique_id;
          bump_data.column.block_id   = feature_any->parent->unique_id;
          fset = (ZMapFeatureSet) feature_any;
          bump_data.column.fset_id    = fset->unique_id;
              bump_data.column.column_id  = container_set->unique_id;
          bump_data.column.feature_id = 0;  /* (we are saving the column not the feature) container_set->unique_id; */
          bump_data.column.strand     = container_set->strand;
          bump_data.strand_specific   = zmapWindowContainerFeatureSetIsStrandShown(container_set);
          bump_data.bump_mode         = zmapWindowContainerFeatureSetGetBumpMode(container_set);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          printf("bump_save %s/%s = %d\n", g_quark_to_string(bump_data.column.column_id),g_quark_to_string(bump_data.column.fset_id),bump_data.bump_mode);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          bump->style_bump = g_array_append_val(bump->style_bump, bump_data);
        }
    }

  return ;
}

gboolean zmapWindowStateSaveBumpedColumns(ZMapWindowState state, ZMapWindow window)
{
  /* the restore was commented out so this function is not needed right now
   * both need to handle columns not featuresets
   * if bump applies to featuresets not columns (bump on feature)
   * then they need to be cleverer.
   */
  if(!state->bump_state_set)
    {
      ZMapWindowCompressMode compress_mode;

      state->bump.style_bump = g_array_new(FALSE, TRUE,
                                           sizeof(StyleBumpModeStruct));
      zmapWindowContainerUtilsExecute(window->feature_root_group,
                                      ZMAPCONTAINER_LEVEL_FEATURESET,
                                      get_bumped_columns,
                                      &(state->bump));

      if (zmapWindowMarkIsSet(window->mark))
        compress_mode = ZMAPWINDOW_COMPRESS_MARK ;
      else
        compress_mode = ZMAPWINDOW_COMPRESS_ALL ;

      state->bump.compress  = compress_mode;
      state->rev_comp_state = state->bump.rev_comp_state = zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES);

      state->bump_state_set = TRUE;
    }

  return state->bump_state_set;
}

gboolean zmapWindowStateGetScrollRegion(ZMapWindowState state,
                                        double *x1, double *y1,
                                        double *x2, double *y2)
{
  gboolean found_position = FALSE;
  if(state->position_set)
    {
      if(x1)
        *x1 = state->position.scroll_x1;
      if(x2)
        *x2 = state->position.scroll_x2;

      if(y1)
        *y1 = state->position.scroll_y1;
      if(y2)
        *y2 = state->position.scroll_y2;

      found_position = TRUE;
    }
  return found_position;
}

/* INTERNAL */

void zmapWindowStateRevCompRegion(ZMapWindow window, double *a, double *b)
{
  /*
   * rev comp'd coords are based on sequence end (the new origin)
   * these are displayed as -ve numbers via some calcultions in zmapWindowUtils.c
   */

  /* RevComp applies to an align not a window or featurecontext
   * but for the moment we only handle one align and one block
   * So we can get the block query sequence coord from here as
   * they are the same as the sequence_to_parent child coordinates
   * ref to RT 158542
   */

  ZMapSpan sspan;

  /* we have to use the parent span as the alignment's sequence span gets fflipped */
  sspan = &window->feature_context->parent_span;


  if (a && b)
    {
      double tp;                  /* temp */
      *a = sspan->x2 + 1 - *a;
      *b = sspan->x2 + 1 - *b;
      tp = *a;
      *a = *b;
      *b = tp;
    }

  return ;
}

static void state_mark_restore(ZMapWindow window, ZMapWindowMark mark, ZMapWindowMarkSerialStruct *serialized)
{
  ZMapWindowMarkSerialStruct restore = {} ;

  restore = *serialized ;                                    /* n.b. struct copy */

  if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) != restore.rev_comp_state)
    {
      int seq_start, seq_end ;

      seq_start = (int)(restore.y1) ;
      seq_end = (int)(restore.y2) ;

      zMapFeatureReverseComplementCoords(window->feature_context, &seq_start, &seq_end) ;

      restore.y1 = (double)seq_start ;
      restore.y2 = (double)seq_end ;
    }


  /* There's a problem with positions here: we can be called before everything
   * has been drawn (revcomp, window split) and therefore the block can be smaller
   * or in a different position than the original x1, x2 position of the mark
   * causing the zmapWindowMarkSetWorldRange() call to fail because it can't
   * find the block....
   *
   * We hack this by setting x1 to zero, this will fail if have multiple
   * blocks horizontally but then so will a lot of things. */
  restore.x1 = 0.0 ;

  zmapWindowMarkSetWorldRange(mark,
                              restore.x1, restore.y1,
                              restore.x2, restore.y2) ;

  return ;
}

static void state_position_restore(ZMapWindow window, ZMapWindowPositionStruct *position)
{
  ZMapWindowPositionStruct new_position = {};

  if (window->canvas && FOO_IS_CANVAS(window->canvas))
    {
      /* I'm copying this, so as not to alter the state->position version. */
      /* Accidentally or otherwise doing a state restore more than once should be ok. */
      new_position = *position;        /* n.b. struct copy! */

      if(window_state_debug_G)
        print_position(position, "state_position_restore input");

      if(zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) != position->rev_comp_state)
        {
          double tmp;

          if(window_state_debug_G)
            print_position(position, "state_position_restore rev-comp status switched! reversing position...");

          zmapWindowStateRevCompRegion(window, &(new_position.scroll_y1), &(new_position.scroll_y2));


#if MH17_REVCOMP_DEBUG
          zMapLogWarning("restore scroll: %f,%f -> %f,%f", position->scroll_y1,position->scroll_y2,new_position.scroll_y1,new_position.scroll_y2);
#endif

#ifdef RDS_DONT_INCLUDE
          new_position.scroll_y1 = seq_length - position->scroll_y1;
          new_position.scroll_y2 = seq_length - position->scroll_y2;

          tmp                    = new_position.scroll_y1;
          new_position.scroll_y1 = new_position.scroll_y2;
          new_position.scroll_y2 = tmp;
#endif
          tmp                           = new_position.v_adjuster.value + new_position.v_adjuster.page_size;
          new_position.v_adjuster.value = new_position.v_adjuster.upper - tmp;
          new_position.scroll_offset_y  = new_position.v_adjuster.upper - tmp;
        }

      set_scroll_region(window,
                        new_position.scroll_x1, new_position.scroll_y1,
                        new_position.scroll_x2, new_position.scroll_y2);

#if 0
      printf("pos restore: %f %f %f %f\n",gtk_adjustment_get_value(v_adjuster), gtk_adjustment_get_value(h_adjuster),
             new_position.v_adjuster.value, new_position.h_adjuster.value);
      // old is 0, new y is 4000+: prevents expose events to the canvas

      //      gtk_adjustment_set_value(v_adjuster, new_position.v_adjuster.value);
      //      gtk_adjustment_set_value(h_adjuster, new_position.h_adjuster.value);
#else
      foo_canvas_scroll_to(FOO_CANVAS(window->canvas), new_position.scroll_offset_x, new_position.scroll_offset_y);
#endif

      if(window_state_debug_G)
        print_position(&new_position, "state_position_restore input");
    }

  return ;
}

static void state_focus_items_restore(ZMapWindow window, ZMapWindowFocusSerialStruct *serialized)
{
  ZMapWindowFocusSerialStruct restore = {} ;
  restore = *serialized ;                                    /* n.b. struct copy */

  if (serialized->item.align_id != 0 && serialized->item.feature_id != 0)
    {
      FooCanvasItem *focus_item = NULL;
      GList *possible_focus_items;

      /* Forming the search correctly when a reverse complement has happened is not trivial
       * because of strand issues. */
      if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) != serialized->rev_comp_state)
        {
          if (restore.item.strand_specific)
            restore.item.strand = ZMAPFEATURE_SWOP_STRAND(restore.item.strand) ;
        }

      if ((focus_item = zmapWindowFToIFindItemFull(window,window->context_to_item,
                                                   restore.item.align_id,
                                                   restore.item.block_id,
                                                   restore.item.fset_id,
                                                   restore.item.strand,
                                                   restore.item.frame,
                                                   restore.item.feature_id))
          && zmapWindowItemIsShown(focus_item))
        {

          zmapWindowFocusAddItem(window->focus, focus_item, zMapWindowCanvasItemGetFeature(focus_item));

          zmapWindowHighlightFocusItems(window);
        }
      else if ((possible_focus_items = zmapWindowFToIFindItemSetFull(window,window->context_to_item,
                                                                     restore.item.align_id,
                                                                     restore.item.block_id,
                                                                     restore.item.column_id,
                                                                     0,
                                                                     "*",        /* reverse complement... */
                                                                     "*",        /* laziness */
                                                                     restore.item.feature_id,
                                                                     NULL, NULL)))
        {
          //          zmapWindowFocusAddItem(window->focus, possible_focus_items->data,NULL);
          zmapWindowFocusAddItems(window->focus, possible_focus_items, NULL, NULL) ;

          zmapWindowHighlightFocusItems(window);

          /* Need single hot item here for focus below.... */
        }
      else
        {
          /* this can happen if something is strand specific and is not shown on the reverse
             strand ?? or maybe not...actually we should be checking if something is visible !!!! */

          /* Blank the info panel if we can't find the feature. */
          zmapWindowUpdateInfoPanel(window, NULL, NULL, NULL, NULL, 0, 0, 0, 0, NULL, TRUE, FALSE, FALSE, FALSE) ;


          zMapLogWarning("%s", "Failed to find serialized focus item.");
        }


      if (focus_item)
        {
          ZMapFeature feature ;
          FooCanvasItem *sub_item = focus_item;
          gboolean replace_highlight = TRUE, highlight_same_names = FALSE ;

          feature = zMapWindowCanvasItemGetFeature(focus_item) ;

          /* Pass information about the object clicked on back to the application. */
          zmapWindowUpdateInfoPanel(window, feature, NULL, sub_item, NULL, 0, 0, 0, 0,
                                    NULL, replace_highlight, highlight_same_names, FALSE, FALSE) ;
        }
    }

  return ;
}

static void state_bumped_columns_restore(ZMapWindow window, ZMapWindowBumpStateStruct *serialized)
{
  if(serialized->style_bump)
    {
      GArray *bumped_columns;
      int bumped_col_count, i, changed = 0;
      gboolean swap_strand = FALSE;

      bumped_columns   = serialized->style_bump;
      bumped_col_count = bumped_columns->len;

      swap_strand = (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES) != serialized->rev_comp_state);

      for(i = 0; i < bumped_col_count; i++)
        {
          FooCanvasItem *container;
          StyleBumpModeStruct *column_state;


          column_state = &(g_array_index(bumped_columns, StyleBumpModeStruct, i));

          if(swap_strand && column_state->strand_specific)
            {
              if(column_state->column.strand == ZMAPSTRAND_FORWARD)
                column_state->column.strand = ZMAPSTRAND_REVERSE;
              else if(column_state->column.strand == ZMAPSTRAND_REVERSE)
                column_state->column.strand = ZMAPSTRAND_FORWARD;
            }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          printf("bump_restore state fset %s = %d\n",g_quark_to_string(column_state->column.fset_id),column_state->bump_mode);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          if((container = zmapWindowFToIFindItemColumn(window,window->context_to_item,
                                                       column_state->column.align_id,
                                                       column_state->column.block_id,
                                                       column_state->column.fset_id,
                                                       column_state->column.strand,
                                                       column_state->column.frame)))
            {
              ZMapWindowContainerFeatureSet container_set;

              container_set = (ZMapWindowContainerFeatureSet)(container);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              printf("bump restore col %s = %d\n", g_quark_to_string(container_set->original_id),zmapWindowContainerFeatureSetGetBumpMode(container_set));
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


              /* Only bump if it's different from current.
               * This _implicit_ test is the only way to currently check this as
               * there's no initial_bump_mode in the style...
               */
              /* Also if the bump is not different from the current the compress mode
               * will almost certainly mean there will be odd results...
               */
              /*! \todo #warning WRONG_NEED_INITIAL_BUMP_MODE */
              if(zmapWindowContainerFeatureSetGetBumpMode(container_set) != column_state->bump_mode)
                {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  printf("bump restore (mark) %d %d",serialized->compress,zmapWindowMarkIsSet(window->mark));
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                  if((serialized->compress != ZMAPWINDOW_COMPRESS_MARK ||
                      (zmapWindowMarkIsSet(window->mark))))
                    {
                      zmapWindowColumnBumpRange(container,
                                                column_state->bump_mode,
                                                serialized->compress);
                      changed++;
                    }
                }
            }
        }

      if(changed)
        zmapWindowFullReposition(window->feature_root_group,TRUE, "bump state");
    }

  return ;
}

static void print_position(ZMapWindowPositionStruct *position, const char *from)
{
  printf("%s: position is\n", from);

  printf("\tscroll-region:\t%f, %f, %f, %f\n",
         position->scroll_x1, position->scroll_y1,
         position->scroll_x2, position->scroll_y2);

  printf("\tv-adjuster: value = %f, page-size = %f, lower = %f, upper = %f\n",
         position->v_adjuster.value, position->v_adjuster.page_size,
         position->v_adjuster.lower, position->v_adjuster.upper);

  printf("\tscroll-offsets: x = %d, y = %d\n",
         position->scroll_offset_x, position->scroll_offset_y);

  return ;
}


static gboolean serialize_item(FooCanvasItem *item, SerializedItemStruct *serialize)
{
  gboolean serialized = FALSE ;
  ZMapWindowContainerGroup container_group ;
  ZMapFeature feature ;

  if ((container_group = zmapWindowContainerCanvasItemGetContainer(item))
      && (feature = zMapWindowCanvasItemGetFeature(item)))
    {
      if (zMapFeatureIsValid((ZMapFeatureAny)feature))
        {
          ZMapWindowContainerFeatureSet container_set;

          container_set = (ZMapWindowContainerFeatureSet)container_group ;

          /* we need to record strand stuff here.......otherwise we can't set strand correctly later.....*/
          if (feature->style)
            serialize->strand_specific = zMapStyleIsStrandSpecific(*feature->style) ;

          serialize->strand     = container_set->strand;
          serialize->frame      = container_set->frame;
          serialize->column_id  = container_set->unique_id;

          serialize->feature_id = feature->unique_id;

          if (feature->parent)
            {
              serialize->fset_id    = feature->parent->unique_id;

              if (feature->parent->parent)
                {
                  serialize->block_id   = feature->parent->parent->unique_id;

                  if (feature->parent->parent->parent)
                    serialize->align_id   = feature->parent->parent->parent->unique_id;
                }
            }

          serialized = TRUE;
        }
      else
        {
          zMapLogWarning("%s", "serialize_item failed: invalid feature") ;
        }
    }

  return serialized;
}


/* Queue code */

ZMapWindowStateQueue zmapWindowStateQueueCreate(void)
{
  ZMapWindowStateQueue zqueue = NULL;

  if(!(zqueue = g_queue_new()))
    zMapWarnIfReached();

  if(zqueue)
    {
      ZMapWindowState head_state;
      if((head_state = zmapWindowStateCreate()))
        {
          head_state->in_state_restore = FALSE;
          g_queue_push_head(zqueue, head_state);
        }
    }

  return zqueue;
}

int zmapWindowStateQueueLength(ZMapWindowStateQueue zqueue)
{
  int size = 0;

  if((size = g_queue_get_length(zqueue)) > 0)
    size--;
  else if(size == 0)
    zMapLogWarning("%s", "Queue has zero size. This is unexpected!");

  return size;
}

gboolean zMapWindowHasHistory(ZMapWindow window)
{
  if(zmapWindowStateQueueLength(window->history))
    return TRUE;
  return FALSE;
}

gboolean zmapWindowStateGetPrevious(ZMapWindow window, ZMapWindowState *state_out, gboolean pop)
{
  ZMapWindowState state = NULL;
  gboolean got_state = FALSE;

  if(zMapWindowHasHistory(window))
    {
      if(pop)
        state = (ZMapWindowState)g_queue_pop_tail(window->history);
      else
        state = (ZMapWindowState)g_queue_peek_tail(window->history);
    }

  if(state_out && state)
    {
      got_state  = TRUE;
      *state_out = state;
    }

  return got_state;
}

gboolean zmapWindowStateQueueStore(ZMapWindow window, ZMapWindowState state_in, gboolean clear_current)
{
  ZMapWindowStateQueue zqueue = window->history;
  gboolean stored = FALSE;

  if((stored = !queue_doing_update(zqueue)) == TRUE)
    {
      if(clear_current)
        zmapWindowStateQueueClear(zqueue);

      g_queue_push_tail(zqueue, state_in);
    }
#ifdef RDS_DONT_INCLUDE
  else if(state_restore_copies_G == TRUE &&
          stored == FALSE &&
          clear_current == TRUE)
    {
      zmapWindowStateQueueClear(zqueue);

      g_queue_push_tail(zqueue, state_in);

      stored = TRUE;
    }
#endif

  return stored;
}

void zmapWindowStateQueueRemove(ZMapWindowStateQueue zqueue,
                                ZMapWindowState      state_del)
{
  g_queue_remove_all(zqueue, state_del);

  return ;
}

void zmapWindowStateQueueClear(ZMapWindowStateQueue zqueue)
{
  ZMapWindowState state = NULL;

  while(zmapWindowStateQueueLength(zqueue) &&
        (state = (ZMapWindowState)g_queue_pop_tail(zqueue)))
    {
      state = zmapWindowStateDestroy(state);
    }

  return ;
}

gboolean zmapWindowStateQueueIsRestoring(ZMapWindowStateQueue zqueue)
{
  gboolean doing_update = FALSE;

  doing_update = queue_doing_update(zqueue);

  return doing_update;
}

ZMapWindowStateQueue zmapWindowStateQueueDestroy(ZMapWindowStateQueue zqueue)
{
  ZMapWindowState head_state;

  zmapWindowStateQueueClear(zqueue);

  if((head_state = (ZMapWindowState)g_queue_pop_head(zqueue)))
    zmapWindowStateDestroy(head_state);

  g_queue_free(zqueue);

  zqueue = NULL;

  return zqueue;
}

/* Queue internals */

static gboolean queue_doing_update(ZMapWindowStateQueue zqueue)
{
  ZMapWindowState head_state;
  gboolean doing_update = FALSE;

  if((head_state = (ZMapWindowState)g_queue_peek_head(zqueue)))
    {
      doing_update = head_state->in_state_restore;
    }

  return doing_update;
}

static void mark_queue_updating(ZMapWindowStateQueue zqueue, gboolean update_flag)
{
  ZMapWindowState head_state;

  if((head_state = (ZMapWindowState)g_queue_peek_head(zqueue)))
    {
      head_state->in_state_restore = update_flag;
      if(queue_doing_update(zqueue) != update_flag)
        zMapWarnIfReached();
    }

  return ;
}

static void lockedDisplaySetScrollRegionCB(gpointer key, gpointer value, gpointer user_data)
{
  QLockedDisplay locked = (QLockedDisplay)user_data;
  double x1, x2, y1, y2;

  x1 = locked->x1;
  x2 = locked->x2;
  y1 = locked->y1;
  y2 = locked->y2;

  zmapWindowSetScrollableArea(locked->window, &x1, &y1, &x2, &y2,"lockedDisplaySetScrollRegionCB");

  return ;
}

static void set_scroll_region(ZMapWindow window, double x1, double y1, double x2, double y2)
{
  if(window->locked_display)
    {
      QLockedDisplayStruct locked = {NULL};

      locked.window = window;

      locked.x1 = x1;
      locked.x2 = x2;
      locked.y1 = y1;
      locked.y2 = y2;

      g_hash_table_foreach(window->sibling_locked_windows, lockedDisplaySetScrollRegionCB, &locked);
    }
  else
    {
      zmapWindowSetScrollableArea(window, &x1, &y1, &x2, &y2,"set_scroll_region");
    }

  return ;
}
