/*  File: zmapWindowFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Functions that manipulate displayed features, they
 *              encapsulate handling of the feature context, the
 *              displayed canvas items and the feature->item hash.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>

#include <ZMap/zmap.hpp>


#include <gbtools/gbtools.hpp>
#include <ZMap/zmapFASTA.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapPeptide.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <zmapWindow_P.hpp>
#include <zmapWindowContainerUtils.hpp>
#include <zmapWindowCanvasItem.hpp>
#include <zmapWindowContainerFeatureSet_I.hpp>


#include <ZMap/zmapThreadsLib.hpp> // for ForkLock functions


using namespace gbtools ;



#define PFETCH_READ_SIZE 80        /* about a line */
#define PFETCH_FAILED_PREFIX "DB fetch failed:"
#define PFETCH_TITLE_FORMAT "DB fetch %s\"%s\""
#define PFETCH_FULL_RECORD_ARG "-F "


static gboolean g_dragging = FALSE ;                        /* Have clicked button 1 but not yet released */
static gboolean g_dnd_in_progress = FALSE ;                 /* Drag and drop is in progress */
static gboolean g_second_press = FALSE ;                    /* Used for double clicks... */
static guint32  g_last_but_release = 0 ;                     /* Used for double clicks... */



typedef struct PFetchDataStructType
{
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  char *title;
  gulong widget_destroy_handler_id;

  Pfetch *pfetch ;

  gboolean got_response;
} PFetchDataStruct, *PFetchData ;


/* Used in handling callbacks from xremote requests. */
typedef struct RemoteDataStructType
{
  ZMapWindow window ;

  const char *command ;

  ZMapFeatureAny feature_any ;

  FooCanvasItem *real_item ;

} RemoteDataStruct,  *RemoteData ;



static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature) ;

static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data) ;



static gboolean factoryTopItemCreated(FooCanvasItem *top_item, GCallback featureset_callback_func,
                                      ZMapWindowFeatureStack feature_stack, gpointer handler_data);

static bool pfetch_reader_func(char *text, guint *actual_read, char **error, gpointer user_data) ;
static void pfetch_closed_func(gpointer user_data) ;
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data);

static void handleXRemoteReply(gboolean reply_ok, char *reply_error,
			       char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
			       gpointer reply_handler_func_data) ;

static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data) ;

static void revcompTransChildCoordsCB(gpointer data, gpointer user_data) ;



/*
 *                        Globals
 */

/* debugging. */
static gboolean mouse_debug_G = FALSE ;




/*
 *                External interface routines.
 */



/* Does a feature select using the same mechanism as when the user clicks on a feature.
 * The slight caveat is that for compound objects like transcripts the object
 * information is displayed whereas when the user clicks on a transcript the info.
 * for the exon/intron they clicked on is also displayed. */
gboolean zMapWindowFeatureSelect(ZMapWindow window, ZMapFeature feature)
{
  gboolean result = FALSE ;
  FooCanvasItem *feature_item ;

  zMapReturnValIfFail(window && feature, result) ;

  if ((feature_item = zmapWindowFToIFindFeatureItem(window, window->context_to_item,
                                                    feature->strand, ZMAPFRAME_NONE, feature)))
    {

      zmapWindowUpdateInfoPanel(window, feature, NULL, feature_item, NULL, 0, 0,  0, 0,
                                NULL, TRUE, FALSE, FALSE, FALSE) ;
      result = TRUE ;
    }

  return result ;
}


gboolean zMapWindowGetDNAStatus(ZMapWindow window)
{
  gboolean drawable = FALSE;

  zMapReturnValIfFail(window && window->context_map, drawable) ;


  /* We just need one of the blocks to have DNA.
   * This enables us to turn on this button as we
   * can't have half sensitivity.  Any block which
   * doesn't have DNA creates a warning for the user
   * to complain about.
   */

  /* check for style too. */
  /* sometimes we don't have a feature_context ... ODD! */
  if(window->feature_context
     && window->context_map->styles.find_style(zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
    {
      drawable = zMapFeatureContextGetDNAStatus(window->feature_context);
    }

  return drawable;
}


/* Remove an existing feature from the displayed feature context.
 *
 * NOTE IF YOU EVER CHANGE THIS FUNCTION OR CALL IT TO REMOVE A WHOLE FEATURESET
 * refer to the comment above zmapWindowCanvasfeatureset.c/zMapWindowFeaturesetItemRemoveFeature()
 * and write a new function to delete the whole set (i did that: zMapWindowFeaturesetItemRemoveSet)
 * NOTE: destroying the CanvasFeaturset will also do the job
 *
 * Returns FALSE if the feature does not exist. */
gboolean zMapWindowFeatureRemove(ZMapWindow zmap_window, FooCanvasItem *feature_item, ZMapFeature feature, gboolean destroy_feature)
{
  ZMapWindowContainerFeatureSet container_set;
  ZMapWindowContainerGroup column_group ;
  gboolean result = FALSE ;
  ZMapFeatureSet feature_set ;

  if (!feature || !zMapFeatureIsValid((ZMapFeatureAny)feature))
    return result ;

  feature_set = (ZMapFeatureSet)(feature->parent) ;

  column_group = zmapWindowContainerCanvasItemGetContainer(feature_item) ;

  if (ZMAP_IS_CONTAINER_FEATURESET(column_group))
    {
      container_set = (ZMapWindowContainerFeatureSet)column_group ;

      /* Need to delete the feature from the feature set and from the hash and destroy the
       * canvas item....NOTE this is very order dependent. */

      /* I'm still not sure this is all correct.
       * canvasItemDestroyCB has a FToIRemove!
       */

      /* Firstly remove from the FToI hash... */
      if (zmapWindowFToIRemoveFeature(zmap_window->context_to_item,
                                      container_set->strand,
                                      container_set->frame, feature))
        {
          /* check the feature is in featureset. */
          if (zMapFeatureSetFindFeature(feature_set, feature))
            {
              if (ZMAP_IS_WINDOW_FEATURESET_ITEM(feature_item))
                {
                  zMapWindowFeaturesetItemRemoveFeature(feature_item,feature);
                }
              else
                {
                  /* destroy the canvas item...this will invoke canvasItemDestroyCB() */
                  gtk_object_destroy(GTK_OBJECT(feature_item)) ;
                }

              /* I think we shouldn't need to do this probably....on the other hand showing
               * empty cols is configurable.... */
              if (!zmapWindowContainerHasFeatures(ZMAP_CONTAINER_GROUP(container_set)) &&
                  !zmapWindowContainerFeatureSetShowWhenEmpty(container_set))
                {
                  zmapWindowContainerSetVisibility(FOO_CANVAS_GROUP(container_set), FALSE) ;
                }

              /* destroy the feature... deletes record in the featureset. */
              if (destroy_feature)
                zMapFeatureDestroy(feature);

              result = TRUE ;
            }
        }
    }

  return result ;
}





/*
 *                 Package routines.
 */

void zmapWindowPfetchEntry(ZMapWindow window, char *sequence_name)
{
  PFetchData pfetch_data ;
  PFetchUserPrefsStruct prefs = {NULL};
  GString *pfetch_args_str ;

  zMapReturnIfFail(window && sequence_name) ;

  if (!(zmapWindowGetPFetchUserPrefs(window->sequence->config_file, &prefs)) || (prefs.location == NULL))
    {
      zMapWarning("%s", "Failed to obtain preferences for pfetch.\n"
                  "ZMap's config file needs at least pfetch "
                  "entry in the ZMap stanza.");
    }
  else
    {
      // Allocate our callback struct, this is freed in the close dialog callback.
      pfetch_data = g_new0(PFetchDataStruct, 1) ;


      if (prefs.mode && strncmp(prefs.mode, "pipe", 4) == 0)
        {
          pfetch_data->pfetch = new PfetchPipe(prefs.location,
                                               pfetch_reader_func, pfetch_reader_func, pfetch_closed_func,
                                               pfetch_data) ;
        }
      else
        {
          pfetch_data->pfetch = new PfetchHttp(prefs.location, prefs.port,
                                               prefs.cookie_jar, prefs.ipresolve, prefs.cainfo, prefs.proxy,
                                               pfetch_reader_func, pfetch_reader_func, pfetch_closed_func,
                                               pfetch_data) ;
        }

      pfetch_data->pfetch->setEntryType(prefs.full_record) ;

      pfetch_data->pfetch->setDebug(prefs.verbose) ;

      if (prefs.location)
        g_free(prefs.location);
      if (prefs.cookie_jar)
        g_free(prefs.cookie_jar);

      // Show the DB fetch dialog with a message of "DB fetching..."
      const char *pfetch_args = prefs.full_record ? PFETCH_FULL_RECORD_ARG : "";

      pfetch_data->title = g_strdup_printf(PFETCH_TITLE_FORMAT, pfetch_args, sequence_name) ;

      pfetch_data->dialog = zMapGUIShowTextFull(pfetch_data->title, "DB fetching...\n",
                                                FALSE, NULL, &(pfetch_data->text_buffer));

      pfetch_data->widget_destroy_handler_id =
        g_signal_connect(G_OBJECT(pfetch_data->dialog), "destroy",
                         G_CALLBACK(handle_dialog_close), pfetch_data);


      pfetch_args_str = g_string_new("") ;

      g_string_append_printf(pfetch_args_str, "%s", sequence_name) ;


      // Try to fetch the sequence.

      /* It would seem that PFetchHandleFetch() calls g_spawn_async_with_pipes() which is not
       * thread safe so lock round it...should locks be in pfetch code ?? */
      {
        bool result ;
        int err_num = 0 ;
        char *err_msg = NULL ;

        if (!(result = UtilsGlobalThreadLock(&err_num)))
          {
            zMapLogCriticalSysErr(err_num, "%s", "Error trying to lock for pfetch") ;
          }

        if (result)
          {
            if (!(pfetch_data->pfetch->fetch(pfetch_args_str->str, &err_msg)))
              {
                zMapWarning("Error fetching sequence '%s':%s", sequence_name, err_msg) ;
                
                free((void *)err_msg) ;
              }
          }

        if (result)
          {
            if (!(result = UtilsGlobalThreadUnlock(&err_num)))
              {
                zMapLogCriticalSysErr(err_num, "%s", "Error trying to lock for pfetch") ;
              }
          }
      }

      g_string_free(pfetch_args_str, TRUE) ;
    }

  return ;
}


void zmapWindowFeatureItemEventButRelease(GdkEvent *event)
{
  GdkEventButton *but_event = (GdkEventButton *)event ;

  g_dragging = FALSE ;
  g_dnd_in_progress = FALSE ;
  g_second_press = FALSE ;
  g_last_but_release = but_event->time ;
}


/* Handle events on items, note that events for text items are passed through without processing
 * so the text item code can do highlighting etc.
 *
 * This is a package routine because features are no longer foocanvas items and so cannot have
 * events attached directly to them. Instead if the column code finds that the mouse click
 * happened on a feature it calls this function to handle the event.
 *
 *  */
gboolean zmapWindowFeatureItemEventHandler(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;                            /* By default we _don't_ handle events. */
  ZMapWindow window = (ZMapWindowStruct*)data ;
  static ZMapFeature feature = NULL ;
  static GtkTargetEntry targetentries[] =                   /* Set up targets for drag and drop */
    {
      { (gchar *)"STRING",        0, TARGET_STRING },
      { (gchar *)"text/plain",    0, TARGET_STRING },
      { (gchar *)"text/uri-list", 0, TARGET_URL },
    };
  static GtkTargetList *targetlist = NULL;


  if (!targetlist)
    targetlist= gtk_target_list_new(targetentries, 3);


  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS  || event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;
      FooCanvasItem *highlight_item = NULL ;

      zMapDebugPrint(mouse_debug_G, "Start: %s %d",
                     (event->type == GDK_BUTTON_PRESS ? "button_press"
                      : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
                     but_event->button) ;

      if (!ZMAP_IS_CANVAS_ITEM(item))
        {
          zMapDebugPrint(mouse_debug_G, "Leave (Not canvas item): %s %d - return FALSE",
                         (event->type == GDK_BUTTON_PRESS ? "button_press"
                          : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
                         but_event->button) ;

          return FALSE ;
        }


      /* freeze composite feature to current coords
       * seems a bit more semantic to do this in zMapWindowCanvasItemGetInterval()
       * but that's called by handleButton which doesn't do double click
       */
      if (but_event->button == 1 || but_event->button == 3)
        zMapWindowCanvasItemSetFeature((ZMapWindowCanvasItem) item, but_event->x, but_event->y);

      /* Get the feature attached to the item, checking that its type is valid */
      feature = zMapWindowCanvasItemGetFeature(item) ;

      if (but_event->type == GDK_BUTTON_PRESS)
        {
          if (but_event->button == 1)
            {
              g_dragging = TRUE ;
              g_dnd_in_progress = FALSE ;
            }
          else if (but_event->button == 3)
            {
              /* Pop up an item menu. */
              zmapMakeItemMenu(but_event, window, item) ;
            }

          event_handled = TRUE ;
        }
      else if (but_event->type == GDK_2BUTTON_PRESS)
        {
          g_second_press = TRUE ;

          event_handled = TRUE ;
        }
      else                                                    /* button release */
        {
          /*                            button release                             */
          g_dragging = FALSE ;
          g_dnd_in_progress = FALSE ;

          /* Gdk defines double clicks as occuring within 250 milliseconds of each other
           * but unfortunately if on the first click we do a lot of processing,
           * STUPID Gdk no longer delivers the GDK_2BUTTON_PRESS so we have to do this
           * hack looking for the difference in time. This can happen if user clicks on
           * a very large feature causing us to paste a lot of text to the selection
           * buffer. */
          guint but_threshold = 500 ;                            /* Separation of clicks in milliseconds. */

          if (g_second_press || but_event->time - g_last_but_release < but_threshold)
            {
              const gchar *style_id = g_quark_to_string(zMapStyleGetID(*feature->style)) ;

              /* Second click of a double click means show feature details. */
              if (but_event->button == 1)
                {
                  highlight_item = item;

                  /* If external client then call them to do editing. */
                  if (window->xremote_client)
                    {
                      /* For the scratch column, the feature doesn't exist in the peer.
                       * Pop up a dialog to give the user the option to . */
                      if (feature && feature->style && strcmp(style_id, ZMAP_FIXED_STYLE_SCRATCH_NAME) == 0)
                        {
                          zmapWindowFeatureShow(window, item, TRUE) ;
                        }
                      else
                        {
                          zmapWindowFeatureCallXRemote(window, (ZMapFeatureAny)feature, ZACP_EDIT_FEATURE, highlight_item) ;
                        }
                    }
                  else
                    {
                      /* No xremote peer connected. If the user double clicked the annotation
                       * column then open the Create Feature dialog to create the feature
                       * locally. For other columns we may want to do an 'edit' operation - copy the feature to
                       * the Annotation column ready for editing. This requires a little thought
                       * though (we probably only want to do this if the Annotation column is
                       * empty? Or always copy and merge it in... probably with a control key to
                       * determine whether the entire feature or just the subfeature should be copied). */
                      if (feature && feature->style && strcmp(style_id, ZMAP_FIXED_STYLE_SCRATCH_NAME) == 0)
                        {
                          zmapWindowFeatureShow(window, item, TRUE) ;
                        }
                    }
                }

              g_second_press = FALSE ;
            }
          else
            {


              event_handled = handleButton(but_event, window, item, feature) ;
            }

          g_last_but_release = but_event->time ;

          event_handled = TRUE ;
        }

      zMapDebugPrint(mouse_debug_G, "Leave: %s %d - return %s",
                     (event->type == GDK_BUTTON_PRESS ? "button_press"
                      : event->type == GDK_2BUTTON_PRESS ? "button_2press" : "button_release"),
                     but_event->button,
                     event_handled ? "TRUE" : "FALSE") ;

      if (mouse_debug_G)
        fflush(stdout) ;
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      /* We initiate drag-and-drop if the user has started dragging a feature that is already
       * selected. "dragging" indicates that a left-click has happened and not been released yet, and
       * "dnd_in_progress" means that a drag-and-drop has already been initiated (so there's
       * nothing left to do). */
      if (!g_dnd_in_progress && g_dragging && item && window)
        {
          if (item != NULL && zmapWindowFocusGetHotItem(window->focus) == item)
            {
              gtk_drag_begin(GTK_WIDGET(window->canvas), targetlist,
                             (GdkDragAction)(GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK),
                             1, (GdkEvent*)event);

              g_dnd_in_progress = TRUE ;
            }
        }
    }


  return event_handled ;
}



ZMapFrame zmapWindowFeatureFrame(ZMapFeature feature)
{
  ZMapFrame frame ;

  /* do we need to consider the reverse strand.... or just return ZMAPFRAME_NONE */

  frame = zMapFeatureFrame(feature) ;

  return frame ;
}

/* Encapulates the rules about which strand a feature will be drawn on.
 *
 * For ZMap this amounts to:
 *                            strand specific     not strand specific
 *       features strand
 *
 *       ZMAPSTRAND_NONE            +                     +
 *       ZMAPSTRAND_FORWARD         +                     +
 *       ZMAPSTRAND_REVERSE         -                     +
 *
 * Some features (e.g. repeats) are derived from alignments that match to the
 * forward or reverse strand and hence the feature can be said to have strand
 * in the alignment sense but for the actual feature (the repeat) the strand
 * is irrelevant.
 *
 * Points to a data problem really, if features are not strand specific then
 * their strand should be ZMAPSTRAND_NONE !
 *
 *  */
ZMapStrand zmapWindowFeatureStrand(ZMapWindow window, ZMapFeature feature)
{
  ZMapFeatureTypeStyle style = NULL;
  ZMapStrand strand = ZMAPSTRAND_FORWARD ;

  //  style = zMapFindStyle(window->context_map.styles, feature->style_id) ;
  style = *feature->style;     // safe failure...

  zMapReturnValIfFail(feature && style, strand) ;

  if ((!(zMapStyleIsStrandSpecific(style))) ||
      ((feature->strand == ZMAPSTRAND_FORWARD) ||
       (feature->strand == ZMAPSTRAND_NONE)))
    strand = ZMAPSTRAND_FORWARD ;
  else
    strand = ZMAPSTRAND_REVERSE ;

  if(zMapStyleDisplayInSeparator(style))
    strand = ZMAPSTRAND_NONE;

  return strand ;
}




/* Called to draw each individual feature. */
FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow window,
                                     ZMapFeatureTypeStyle style,
                                     ZMapWindowContainerFeatureSet set_group,
                                     ZMapWindowContainerFeatures set_features,
                                     FooCanvasItem *foo_featureset,
                                     GCallback featureset_callback_func,
                                     ZMapWindowFeatureStack feature_stack)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet) set_group;
  gboolean masked;
  ZMapFeature feature = NULL ;

  zMapReturnValIfFail(window && feature_stack, new_feature) ;

  feature = feature_stack->feature;

  if ((masked = (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT &&  feature->feature.homol.flags.masked)))
    {
      if (container->masked && !window->highlights_set.masked)
        {
          return NULL ;
        }
    }

  new_feature = zmapWindowFeatureFactoryRunSingle(window->context_to_item,
                                                  set_group, set_features,
                                                  foo_featureset,
                                                  feature_stack) ;

  /* This is where the event handler is attached for a column...crazy....completely crazy... */
  //#warning could make this take a window not a gpointer, would be more readable
  //#warning ideally only call this first time canvasfeatureset is created
  //  if(!zMapWindowCanvasItemIsConnected((ZMapWindowCanvasItem) new_feature))
  if (new_feature != foo_featureset)
    {
      /* THIS IS ALL HACKED UP AS A TEMPORARY SOLUTION TO THE PROBLEM OF FACTORYRUNSINGLE
       * BEING THE PLACE WHERE A FEATURESET IS MADE...DEEP SIGH....
       *
       * It should always be true that if we come in here then there is a
       * featureset_callback_func.
       * This function does get called from featureexpand but there should always be a featureset
       * already defined. */

      if (!featureset_callback_func)
        zMapLogCritical("%s", "new featureset should be made but there is no callback function.") ;
      else
        factoryTopItemCreated(new_feature, featureset_callback_func, feature_stack, (gpointer)window) ;
    }

  if (masked && container->masked && new_feature)
    foo_canvas_item_hide(new_feature) ;

  return new_feature ;
}



/* this one function is the remains of zmapWindowItemFactory.c */
FooCanvasItem *zmapWindowFeatureFactoryRunSingle(GHashTable *ftoi_hash,
                                                 ZMapWindowContainerFeatureSet parent_container,
                                                 ZMapWindowContainerFeatures features_container,
                                                 FooCanvasItem *foo_featureset,
                                                 ZMapWindowFeatureStack feature_stack)
{
  FooCanvasItem *feature_item = NULL ;
  ZMapFeature feature = feature_stack->feature ;
  ZMapWindowFeaturesetItem canvas_item = (ZMapWindowFeaturesetItem)foo_featureset ;
  ZMapFeatureBlock block = feature_stack->block ;

  /* NOTE parent is the features group in the container featureset
   * fset is the parent of that (the column) which has the column id
   * foo_featureset is NULL or an existing CanvasFeatureset which is the
   * (normally) single foo canvas item in the column
   */
  if(!canvas_item)
    {
      /* for frame spcecific data process_feature() in zmapWindowDrawFeatures.c extracts
       * all of one type at a time
       * so frame and strand are stable NOTE only frame, strand will wobble
       * we save the id here to optimise the code
       */
      GQuark col_id ;
      FooCanvasItem * foo = FOO_CANVAS_ITEM(parent_container);
      GQuark fset_id = feature_stack->feature_set->unique_id;
      char strand = '+';
      char frame = '0';
      char *x;
      ZMapWindow window ;
      GtkAdjustment *v_adjust ;

      col_id = zmapWindowContainerFeaturesetGetColumnUniqueId(parent_container) ;

      window = zMapWindowContainerFeatureSetGetWindow(parent_container) ;


      /* as we process both strands together strand is per feature not per set */
      if(zMapStyleIsStrandSpecific(*feature->style))
        {
          if(feature->strand == ZMAPSTRAND_REVERSE)
            strand = '-';
          feature_stack->strand = feature->strand;
        }

      if(feature_stack->frame != ZMAPFRAME_NONE)
        {
          feature_stack->frame = zmapWindowFeatureFrame(feature);
          frame += feature_stack->frame;
        }

      /* see comment by zMapWindowGraphDensityItemGetDensityItem() */
      if(feature_stack->maps_to)
        {
          /* a virtual featureset for combing several source into one display item */
          fset_id = feature_stack->maps_to;
          x = g_strdup_printf("%p_%s_%s_%c%c", foo->canvas, g_quark_to_string(col_id), g_quark_to_string(fset_id),strand,frame);
        }
      else
        {
          /* a display column for combing one or several sources into one display item */
          x = g_strdup_printf("%p_%s_%c%c", foo->canvas, g_quark_to_string(col_id), strand,frame);
        }

      feature_stack->id = g_quark_from_string(x);


      /* adds once per canvas+column+style, then returns that repeatedly */
      v_adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(window->scrolled_window)) ;

      canvas_item = zMapWindowCanvasItemFeaturesetGetFeaturesetItem((FooCanvasGroup *)features_container,
                                                                    feature_stack->id,
                                                                    v_adjust,
                                                                    block->block_to_sequence.block.x1,
                                                                    block->block_to_sequence.block.x2,
                                                                    *feature->style,
                                                                    feature_stack->strand,
                                                                    feature_stack->frame,
                                                                    feature_stack->set_index, 0);

#if !FEATURESET_AS_COLUMN
      zmapWindowFToIAddSet(ftoi_hash,
                           feature_stack->align->unique_id, feature_stack->block->unique_id,
                           feature_stack->feature_set->unique_id, feature_stack->strand, feature_stack->frame, (FooCanvasItem *) canvas_item) ;
#endif
      g_free(x);
    }

  feature_item = (FooCanvasItem *) canvas_item;

  if(feature_item)        // will always work
    {
      ZMapFrame frame;
      ZMapStrand strand;

      /* NOTE the item hash used canvas _item->feature to set up a pointer to the feature
       * so I changed FToIAddfeature to take the feature explicitly
       * setting the feature here every time also fixes the problem but by fluke
       */
      zMapWindowCanvasItemSetFeaturePointer((ZMapWindowCanvasItem)canvas_item, feature);

      if(feature_stack->filter && (feature->flags.collapsed || feature->flags.squashed || feature->flags.joined))
        {
          /* collapsed items are not displayed as they contain no new information
           * but they cam be searched for in the FToI hash
           * so return the item that they got collapsed into
           * if selected from the search they get assigned to the canvas item
           * and the population copied in.
           *
           * NOTE calling code will need to set the feature in the hash as the composite feature
           */
          /*! \todo #warning need to set composite feature in lookup code */

          return (FooCanvasItem *) feature_item;
        }

      zMapWindowCanvasFeaturesetAddFeature((ZMapWindowFeaturesetItem) canvas_item, feature, feature->x1, feature->x2);

      feature_item = (FooCanvasItem *)canvas_item;

      frame  = zmapWindowContainerFeatureSetGetFrame(parent_container);
      strand = zmapWindowContainerFeatureSetGetStrand(parent_container);

      if(ftoi_hash)
        {
          if(!feature_stack->col_hash[strand])
            {
              feature_stack->col_hash[strand] = zmapWindowFToIGetSetHash(ftoi_hash,
                                                                         feature_stack->align->unique_id,
                                                                         feature_stack->block->unique_id,
                                                                         feature_stack->feature_set->unique_id, strand, frame);
            }

          zmapWindowFToIAddSetFeature(feature_stack->col_hash[strand],
                                      feature->unique_id, feature_item, feature);

        }
    }

  return feature_item;
}

/* Look for any description in the feature set. */
char *zmapWindowFeatureSetDescription(ZMapFeatureSet feature_set)
{
  char *description = NULL ;

  zMapReturnValIfFail(feature_set, description) ;

  description = g_strdup_printf("%s  :  %s%s%s", (char *)g_quark_to_string(feature_set->original_id),
                                feature_set->description ? "\"" : "",
                                feature_set->description ? feature_set->description : "<no description available>",
                                feature_set->description ? "\"" : "") ;

  return description ;
}


/* Get various aspects of features source... */
void zmapWindowFeatureGetSourceTxt(ZMapFeature feature, char **source_name_out, char **source_description_out)
{
  zMapReturnIfFail(feature) ;

  // gb10: The 
  if (feature->parent && ((ZMapFeatureSet)(feature->parent))->source)
    *source_name_out = g_strdup(((ZMapFeatureSet)(feature->parent))->source->toplevelName().c_str());
  else if (feature->source_id)
    *source_name_out = g_strdup(g_quark_to_string(feature->source_id)) ;

  if (feature->source_text)
    *source_description_out = g_strdup(g_quark_to_string(feature->source_text)) ;

  return ;
}


char *zmapWindowFeatureDescription(ZMapFeature feature)
{
  char *description = NULL ;

  zMapReturnValIfFail(feature && zMapFeatureIsValid((ZMapFeatureAny)feature), description) ;

  if (feature->description)
    {
      description = g_strdup(feature->description) ;
    }

  return description ;
}


/* Get the peptide as a fasta string.  All contained in a function so
 * there's no falling over phase values and stop codons... */
char *zmapWindowFeatureTranscriptFASTA(ZMapFeature feature, ZMapSequenceType sequence_type,
                                       gboolean spliced, gboolean cds_only)
{
  char *fasta = NULL ;
  ZMapFeatureContext context ;

  zMapReturnValIfFail(feature, NULL) ;


  if((feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
     && ((context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                                  ZMAPFEATURE_STRUCT_CONTEXT))))
    {
      char *dna, *seq_name = NULL, *gene_name = NULL;

      seq_name  = (char *)g_quark_to_string(context->original_id);
      gene_name = (char *)g_quark_to_string(feature->original_id);

      if ((dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds_only)))
        {
          int dna_len ;

          dna_len = strlen(dna) ;

          if (sequence_type == ZMAPSEQUENCE_DNA)
            {
              fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA,
                                      seq_name, "Nucleotide",
                                      gene_name, dna_len,
                                      dna) ;
            }
          else
            {
              ZMapPeptide peptide ;
              int pep_length, start_incr = 0 ;

              /* Adjust for when its known that the start exon is incomplete.... */
              if (feature->feature.transcript.flags.start_not_found)
                start_incr = feature->feature.transcript.start_not_found - 1 ; /* values are 1 <= start_not_found <= 3 */

              peptide = zMapPeptideCreate(seq_name, gene_name, (dna + start_incr), NULL, TRUE) ;

              /* Note that we do not include the "Stop" in the peptide length, is this the norm ? */
              pep_length = zMapPeptideLength(peptide) ;
              if (zMapPeptideHasStopCodon(peptide))
                pep_length-- ;

              fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA,
                                      seq_name, "Protein",
                                      gene_name, pep_length,
                                      zMapPeptideSequence(peptide));
            }

          g_free(dna) ;
        }
    }

  return fasta ;
}




/*
 *                       Internal functions.
 */


/* Callback for destroy of feature items... */
static gboolean canvasItemDestroyCB(FooCanvasItem *feature_item, gpointer data)
{
  gboolean event_handled = FALSE ;                            /* Make sure any other callbacks also get run. */
  ZMapWindow window = (ZMapWindowStruct*)data ;

  if (window->focus)
    zmapWindowFocusRemoveOnlyFocusItem(window->focus, feature_item) ;

  return event_handled ;
}




static void featureCopySelectedItem(ZMapFeature feature_in, ZMapFeature feature_out, FooCanvasItem *selected)
{
  if (!feature_in || !feature_out)
    return ;

  if (feature_in && feature_out)
    memcpy(feature_out, feature_in, sizeof(ZMapFeatureStruct));

  return ;
}



/* Handle button single click to highlight and double click to show feature details. */
static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature)
{
  gboolean event_handled = FALSE ;
  GdkModifierType shift_mask = GDK_SHIFT_MASK,
    control_mask = GDK_CONTROL_MASK,
    alt_mask = GDK_MOD1_MASK,
    meta_mask = GDK_META_MASK,
    unwanted_masks = (GdkModifierType)(GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK
                                       | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK
                                       | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK),
    locks_mask ;

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

  /* Button 1 and 3 are handled, 2 is left for a general handler which could be the root handler. */
  if (but_event->button == 1 || but_event->button == 3)
    {
      FooCanvasItem *sub_item = NULL, *highlight_item = NULL ;
      gboolean replace_highlight = TRUE, highlight_same_names = TRUE ;
      ZMapFeatureSubPart sub_feature = NULL ;
      ZMapWindowCanvasItem canvas_item ;
      ZMapFeatureStruct feature_copy = {};
      ZMapFeatureAny xremote_feature = (ZMapFeatureAny)feature ;
      gboolean highlight_sub_part = FALSE;
      ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED, ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                    ZMAPWINDOW_PASTE_TYPE_SELECTED} ;

      canvas_item = ZMAP_CANVAS_ITEM(item);
      highlight_item = item;

      sub_item = zMapWindowCanvasItemGetInterval(canvas_item, but_event->x, but_event->y, &sub_feature);

      if (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT || zMapStyleIsUnique(*feature->style))
        highlight_same_names = FALSE ;


      if (zMapGUITestModifiers(but_event, control_mask))
        {
          /* Only highlight the single item user clicked on. */
          highlight_same_names = FALSE ;
          highlight_sub_part = TRUE;

          /* Annotators say they don't want subparts sub selections + multiple
           * selections for alignments. */
          if (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT)
            {
              highlight_item = sub_item ;


              /* WHY ARE WE MAKING A COPY.....WHY ??????????????? */
              /* monkey around to get feature_copy to be the right correct data */
              featureCopySelectedItem(feature, &feature_copy, highlight_item);
              xremote_feature = (ZMapFeatureAny) &feature_copy;
            }
        }

      if (zMapGUITestModifiers(but_event, shift_mask))
        {
          /* multiple selections */
          if (zmapWindowFocusIsItemInHotColumn(window->focus, item))      //      && window->multi_select)
            {
              replace_highlight = FALSE ;
            }
        }

      if (zMapGUITestModifiers(but_event, alt_mask) || zMapGUITestModifiers(but_event, meta_mask))
        {
          display_style.coord_frame = ZMAPWINDOW_COORD_NATURAL ;
          display_style.paste_style = ZMAPWINDOW_PASTE_FORMAT_BROWSER ;
          display_style.paste_feature = ZMAPWINDOW_PASTE_TYPE_EXTENT ;
        }


      {
        /* mh17 Foo sequence features have a diff interface, but we wish to avoid that,
         * see sequenceSelectionCB() above */
        /* using a CanvasFeatureset we get here, first off just pass a single coord through so it does not crash */
        /* InfoPanel has two sets of coords, but they appear the same in totalview */
        /* possibly we can hide region selection in the GetInterval call above:
         * we can certainly use the X coordinate ?? */

        int start = feature->x1, end = feature->x2;

        if (sub_feature)
          {
            start = sub_feature->start;
            end = sub_feature->end;
          }

        /* Pass information about the object clicked on back to the application. */
        zmapWindowUpdateInfoPanel(window, feature, NULL, item, sub_feature, start, end, start, end,
                                  NULL, replace_highlight, highlight_same_names, highlight_sub_part, &display_style) ;

        /* if we have an active dialog update it: they have to click on a feature not the column */
        ZMapFeatureSet feature_set = (ZMapFeatureSet)(feature->parent) ; 
        if (feature_set)
          zmapWindowStyleDialogSetStyle(window, feature_set->style, feature_set, FALSE) ;

      }

      if (zMapGUITestModifiers(but_event, shift_mask))
        {
          /* multiple selections */
          if (zmapWindowFocusIsItemInHotColumn(window->focus, item))      //      && window->multi_select)
            {
              replace_highlight = FALSE ;

              if (window->xremote_client)
                zmapWindowFeatureCallXRemote(window, xremote_feature, ZACP_SELECT_MULTI_FEATURE, highlight_item) ;
            }
          else
            {
              if (window->xremote_client)
                zmapWindowFeatureCallXRemote(window, xremote_feature, ZACP_SELECT_FEATURE, highlight_item) ;

              window->multi_select = TRUE ;
            }
        }
      else
        {
          /* single select */
          if (window->xremote_client)
            zmapWindowFeatureCallXRemote(window, xremote_feature, ZACP_SELECT_FEATURE, highlight_item) ;

          window->multi_select = FALSE ;
        }

      if (sub_feature)
        g_free(sub_feature) ;
    }


  return event_handled ;
}





/*
 * if a feature is compressed then replace it with the underlying data
 *
 * NOTE: ZMapWindowCanvasFeatureset only
 * can't return a feature as there will be many
 *
 * NOTE: un-bumping and re-bumping should show the same picture _this must be stable_
 * and this can be done by detecting the same bump mode and using the previous bump offset coordinates
 * however if we selected an altername bump mode we would have to compress the expanded features
 * so we do that anyway: it's reasonable to expect a fresh bump to put all features in the same state
 * this can be changed if bump to diff mode clears up expanded features
 * NOTE: we also have to take care of some now quite complex HIDE_REASON flag status
 * see zmapWindowCanvasFeatureset.c/zmapWindowFeaturesetItemShowHide()
 */

void zmapWindowFeatureExpand(ZMapWindow window, FooCanvasItem *foo,
                             ZMapFeature feature, ZMapWindowContainerFeatureSet container_set)
{
  GList *l;
  ZMapWindowFeatureStackStruct feature_stack = { NULL };
  ZMapWindowCanvasItem item = (ZMapWindowCanvasItem) foo;
  ZMapStyleBumpMode curr_bump_mode ;
  ZMapWindowContainerFeatures features = zmapWindowContainerGetFeatures((ZMapWindowContainerGroup) container_set);
  FooCanvasItem *foo_featureset = NULL;

  //printf("\n\nexpand %s\n",g_quark_to_string(feature->original_id));
  if(feature->population < 2)        /* should not happen */
    return;

  /* hide the compressed feature */
  zMapWindowCanvasItemShowHide(item, FALSE);

  /* draw all its children  */
  zmapGetFeatureStack(&feature_stack, NULL, feature, container_set->frame);


  for (l = feature->children; l ; l = l->next)
    {
      /* (mh17) NOTE we have to be careful that these features end up in the same
       * (singleton) CanvasFeatureset else they overlap on bump */
      feature_stack.feature = (ZMapFeature)(l->data) ;

      /* I think we can pass in NULL for the callback here..... */
      item = (ZMapWindowCanvasItem)zmapWindowFeatureDraw(window, *feature->style,
                                                         container_set, features,
                                                         foo_featureset, NULL,
                                                         &feature_stack) ;

      foo_featureset = (FooCanvasItem *)item ;

      //printf(" show %s\n", g_quark_to_string(feature_stack.feature->original_id));

#if MH17_DO_HIDE
      // ref to same #if in zmapWindowCanvasAlignment.c
      zMapWindowCanvasItemShowHide(item, TRUE);
#endif
    }


  /* bump all features to the right of the original right of the composite */

  curr_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container_set) ;
  if(curr_bump_mode != ZMAPBUMP_UNBUMP)
    {
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container_set), ZMAPBUMP_UNBUMP) ;
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container_set), curr_bump_mode ) ;

      /* redraw this col and ones to the right */
      zmapWindowFullReposition(window->feature_root_group,TRUE, "expand") ;
    }
}


void zmapWindowFeatureContract(ZMapWindow window, FooCanvasItem *foo,
                               ZMapFeature feature, ZMapWindowContainerFeatureSet container_set)
{
  /* find the daddy feature and un-hide it */
  ZMapFeature daddy = feature->composite;
  ZMapWindowCanvasItem item = (ZMapWindowCanvasItem) foo;
  GList *l;
  ZMapStyleBumpMode curr_bump_mode ;

  if(!daddy || !ZMAP_IS_WINDOW_FEATURESET_ITEM(foo))
    {
      zMapLogWarning("compress called for invalid type of foo canvas item", "");
      return;
    }
  //printf("\n\ncontract %s\n",g_quark_to_string(daddy->original_id));

  zMapWindowCanvasItemSetFeaturePointer (item, daddy);
  zMapWindowCanvasItemShowHide(item, TRUE);

  /* find all the child features and delete them. */

  for(l = daddy->children; l; l = l->next)
    {
      zMapWindowFeatureRemove(window, foo, (ZMapFeature) l->data, FALSE);
    }


  /* de-bump all features to the right of the original right of the last one */
  /* redraw this col and ones to the right */
  curr_bump_mode = zmapWindowContainerFeatureSetGetBumpMode(container_set) ;
  if(curr_bump_mode != ZMAPBUMP_UNBUMP)
    {
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container_set), ZMAPBUMP_UNBUMP) ;
      zmapWindowColumnBump(FOO_CANVAS_ITEM(container_set), curr_bump_mode ) ;

      /* redraw this col and ones to the right */
      zmapWindowFullReposition(window->feature_root_group,TRUE, "contract") ;
    }

  return ;
}


// Callback functions for DB fetching sequence.
//

// Called when there is something to read from pfetch.
static bool pfetch_reader_func(char *text, guint *actual_read, char **error, gpointer user_data)
{
  bool status = true ;
  PFetchData pfetch_data = (PFetchData)user_data;


  if (actual_read && *actual_read > 0 && pfetch_data)
    {
      GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(pfetch_data->text_buffer);

      /* clear the buffer the first time... */
      if(pfetch_data->got_response == FALSE)
        gtk_text_buffer_set_text(text_buffer, "", 0);

      gtk_text_buffer_insert_at_cursor(text_buffer, text, *actual_read);

      pfetch_data->got_response = TRUE;
    }

  return status ;
}

// Called when connection (pipe) to pfetch is closed.
static void pfetch_closed_func(gpointer user_data)
{
#ifdef DEBUG_ONLY
  printf("pfetch closed\n");
#endif        /* DEBUG_ONLY */

  return ;
}


/* Called when pfetch dialog showing sequence is closed by user. */
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data ;

  delete pfetch_data->pfetch ;

  g_free(pfetch_data) ;

  return ;
}




static gboolean factoryTopItemCreated(FooCanvasItem *top_item, GCallback featureset_callback_func,
                                      ZMapWindowFeatureStack feature_stack,
                                      gpointer handler_data)
{
  gboolean columnBoundingBoxEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;

  g_signal_connect(GTK_OBJECT(top_item), "destroy",
                   GTK_SIGNAL_FUNC(canvasItemDestroyCB), handler_data) ;


  /* the problem with doing this kind of thing is that we also need the canvasItemEventCB to be
   *  attached as there are general things we need to do as well...
   *
   * REVISIT THE WHOLE EVENT DELIVERY ORDER STUFF.....
   *
   *  */
  switch(feature_stack->feature->mode)
    {
    case ZMAPSTYLE_MODE_ASSEMBLY_PATH:
    case ZMAPSTYLE_MODE_TRANSCRIPT:
    case ZMAPSTYLE_MODE_ALIGNMENT:
    case ZMAPSTYLE_MODE_BASIC:
    case ZMAPSTYLE_MODE_TEXT:
    case ZMAPSTYLE_MODE_GLYPH:
    case ZMAPSTYLE_MODE_GRAPH:
    case ZMAPSTYLE_MODE_SEQUENCE:

      g_signal_connect(G_OBJECT(top_item), "event", featureset_callback_func, handler_data) ;

      break ;

      /* Gosh...is this a good idea....not sure... */
    default:
      break ;
    }

  /* this is completely pointless as is this whole routine..... */
  zMapWindowCanvasItemSetConnected((ZMapWindowCanvasItem)top_item, TRUE) ;

  return TRUE ;
}





/* THIS FUNCTION NEEDS CLEANING UP, HACKED FROM OLD XREMOTE CODE..... */
/* Called by select and edit code to call xremote with "select", "edit" etc commands
 * for peer. The peers reply is handled in handleXRemoteReply() */
void zmapWindowFeatureCallXRemote(ZMapWindow window, ZMapFeatureAny feature_any,
                                  const char *command, FooCanvasItem *real_item)
{
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapXMLUtilsEventStack xml_elements ;
  ZMapWindowSelectStruct select = {(ZMapWindowSelectType)0} ;
  ZMapFeatureSetStruct feature_set = {0} ;
  ZMapFeature feature_copy ;
  int chr_bp ;
  RemoteData remote_data ;


  /* We should only ever be called with a feature, not a set or anything else. */
  g_return_if_fail(feature_any && feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;

  /* OK...IN HERE IS THE PLACE FOR THE HACK FOR COORDS....NEED TO COPY FEATURE
   * AND INSERT NEW CHROMOSOME COORDS...IF WE CAN DO THIS FOR THIS THEN WE
   * CAN HANDLE VIEW FEATURE STUFF IN SAME WAY...... */
  feature_copy = (ZMapFeature)zMapFeatureAnyCopy(feature_any) ;
  feature_copy->parent = feature_any->parent ;            /* Copy does not do parents so we fill in. */


  /* REVCOMP COORD HACK......THIS HACK IS BECAUSE OUR COORD SYSTEM IS MUCKED UP FOR
   * REVCOMP'D FEATURES..... */
  /* Convert coords */
  if (zMapWindowGetFlag(window, ZMAPFLAG_REVCOMPED_FEATURES))
    {
      /* remap coords to forward strand range and also swop
       * them as they get reversed in revcomping.... */
      chr_bp = feature_copy->x1 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x1 = chr_bp ;


      chr_bp = feature_copy->x2 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x2 = chr_bp ;

      zMapUtilsSwop(int, feature_copy->x1, feature_copy->x2) ;

      if (feature_copy->strand == ZMAPSTRAND_FORWARD)
        feature_copy->strand = ZMAPSTRAND_REVERSE ;
      else
        feature_copy->strand = ZMAPSTRAND_FORWARD ;


      if (ZMAPFEATURE_IS_TRANSCRIPT(feature_copy))
        {
          if (!zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_EXON,
                                                 revcompTransChildCoordsCB, window)
              || !zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_INTRON,
                                                    revcompTransChildCoordsCB, window))
            zMapLogCritical("RemoteControl error revcomping coords for transcript %s",
                            zMapFeatureName((ZMapFeatureAny)(feature_copy))) ;

          zMapFeatureTranscriptSortExons(feature_copy) ;
        }
    }

  /* Streuth...why doesn't this use a 'creator' function...... */
#ifdef FEATURES_NEED_MAGIC
  feature_set.magic       = feature_copy->magic ;
#endif
  feature_set.struct_type = ZMAPFEATURE_STRUCT_FEATURESET;
  if (feature_copy->parent)
    {
      feature_set.parent      = feature_copy->parent->parent;
      feature_set.unique_id   = feature_copy->parent->unique_id;
      feature_set.original_id = feature_copy->parent->original_id;
    }

  feature_set.features = g_hash_table_new(NULL, NULL) ;
  g_hash_table_insert(feature_set.features, GINT_TO_POINTER(feature_copy->unique_id), feature_copy) ;


  /* I don't get this at all... */
  select.type = ZMAPWINDOW_SELECT_DOUBLE;


  /* Set up xml/xremote request. */
  xml_elements = zMapFeatureAnyAsXMLEvents(feature_copy) ;

  /* free feature_copy, remove reference to parent otherwise we are removed from there. */
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      feature_copy->parent = NULL ;
      zMapFeatureDestroy(feature_copy) ;
    }


  if (feature_set.unique_id)
    {
      g_hash_table_destroy(feature_set.features) ;
      feature_set.features = NULL ;
    }


  /* HERE VIEW IS CALLED WHICH IS FINE.....BUT.....WE NOW NEED ANOTHER CALL WHERE
     WINDOW CALLS REMOTE TO MAKE THE REQUEST.....BECAUSE VIEW WILL NO LONGER BE
     DOING THIS...*/
  (*(window->caller_cbs->select))(window, window->app_data, &select) ;


  /* Send request to peer program. */
  remote_data = g_new0(RemoteDataStruct, 1) ;                    /* free'd in handleXRemoteReply() */
  remote_data->window = window ;
  remote_data->feature_any = feature_any ;
  remote_data->command = command ;
  remote_data->real_item = real_item ;

  (*(window_cbs_G->remote_request_func))(window_cbs_G->remote_request_func_data,
					 window,
					 command, xml_elements,
					 handleXRemoteReply, remote_data,
                                         remoteReplyErrHandler, remote_data) ;

  return ;
}




/* Handles replies received by zmap from the peer remote program to commands sent from this file. */
static void handleXRemoteReply(gboolean reply_ok, char *reply_error,
                               char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                               gpointer reply_handler_func_data)
{
  RemoteData remote_data = (RemoteData)reply_handler_func_data ;
  if (! remote_data) {
      zMapLogCritical("%s", "handleXRemoteReply: reply_handler_func_data should not be NULL");
      return;
  }

  if (!reply_ok)
    {
      zMapLogCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }
  else
    {
      if (g_ascii_strcasecmp(command, ZACP_EDIT_FEATURE) == 0)
        {
          /* If the remote edit command fails then we do our best to show the feature
           * locally, this is just display, not editing. */
          if (command_rc == REMOTE_COMMAND_RC_OK)
            {
              /* Don't do anything. */
            }
          else if (command_rc == REMOTE_COMMAND_RC_FAILED)
            {
              zmapWindowFeatureShow(remote_data->window, remote_data->real_item, FALSE) ;
            }
        }
      else if (g_ascii_strcasecmp(command, ZACP_CREATE_FEATURE) == 0)
        {
          if (command_rc == REMOTE_COMMAND_RC_OK)
            {
              /* Reset the unsaved-changes flag. Really we should wait until the feature has
               * actually been created but the peer may change any of the feature details before
               * creating and sending it back to us so we have no way of knowing when the correct
               * feature has been created. */
              zMapWindowSetFlag(remote_data->window, ZMAPFLAG_SAVE_SCRATCH, FALSE) ;
            }
        }
    }

  g_free(remote_data) ;                                            /* Allocated in zmapWindowFeatureCallXRemote() */
  remote_data = NULL;

  return ;
}



/* HACK....function to remap coords to forward strand range and also swop
 * them as they get reversed in revcomping.... */
static void revcompTransChildCoordsCB(gpointer data, gpointer user_data)
{
  ZMapSpan child = (ZMapSpan)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  int chr_bp ;

  chr_bp = child->x1 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x1 = chr_bp ;


  chr_bp = child->x2 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x2 = chr_bp ;

  zMapUtilsSwop(int, child->x1, child->x2) ;

  return ;
}


static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Unused currently..... */

  RemoteData remote_data = (RemoteData)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






  return ;
}


