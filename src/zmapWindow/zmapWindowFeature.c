/*  File: zmapWindowFeature.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 * Description: Functions that manipulate displayed features, they
 *              encapsulate handling of the feature context, the
 *              displayed canvas items and the feature->item hash.
 *
 * Exported functions: See zmapWindow_P.h
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <math.h>

#include <ZMap/zmap.h>
#include <ZMap/zmapFASTA.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapPeptide.h>
#include <ZMap/zmapGLibUtils.h>
#include <zmapWindow_P.h>
#include <zmapWindowContainerUtils.h>
//#include <zmapWindowItemFactory.h>
#include <zmapWindowCanvasItem.h>
#include <libpfetch/libpfetch.h>
#include <zmapWindowContainerFeatureSet_I.h>


#include <ZMap/zmapThreads.h> // for ForkLock functions



#define PFETCH_READ_SIZE 80        /* about a line */
#define PFETCH_FAILED_PREFIX "PFetch failed:"
#define PFETCH_TITLE_FORMAT "ZMap - pfetch \"%s\""

typedef struct PFetchDataStructType
{
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  char *title;
  gulong widget_destroy_handler_id;
  PFetchHandle pfetch;
  gboolean got_response;
} PFetchDataStruct, *PFetchData ;


/* Used in handling callbacks from xremote requests. */
typedef struct RemoteDataStructType
{
  ZMapWindow window ;

  char *command ;

  ZMapFeatureAny feature_any ;

  FooCanvasItem *real_item ;

} RemoteDataStruct,  *RemoteData ;




static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data) ;
static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature) ;

static gboolean canvasItemDestroyCB(FooCanvasItem *item, gpointer data) ;

static void handle_dialog_close(GtkWidget *dialog, gpointer user_data);

static gboolean factoryTopItemCreated(FooCanvasItem *top_item,
                                      ZMapWindowFeatureStack feature_stack,
                                      gpointer handler_data);

static PFetchStatus pfetch_reader_func(PFetchHandle *handle, char *text, guint *actual_read,
                                       GError *error, gpointer user_data) ;
static PFetchStatus pfetch_closed_func(gpointer user_data) ;

static void callXRemote(ZMapWindow window, ZMapFeatureAny feature_any,
                        char *action, FooCanvasItem *real_item) ;
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





/* Does a feature select using the same mechanism as when the user clicks on a feature.
 * The slight caveat is that for compound objects like transcripts the object
 * information is displayed whereas when the user clicks on a transcript the info.
 * for the exon/intron they clicked on is also displayed. */
gboolean zMapWindowFeatureSelect(ZMapWindow window, ZMapFeature feature)
{
  gboolean result = FALSE ;
  FooCanvasItem *feature_item ;

  if ((feature_item = zmapWindowFToIFindFeatureItem(window, window->context_to_item,
                                                    feature->strand, ZMAPFRAME_NONE, feature)))
    {

      zmapWindowUpdateInfoPanel(window, feature, NULL, feature_item, NULL, 0, 0,  0, 0,
                                NULL, TRUE, FALSE, FALSE, FALSE) ;
      result = TRUE ;
    }

  return result ;
}


void zmapWindowPfetchEntry(ZMapWindow window, char *sequence_name)
{
  PFetchData pfetch_data = g_new0(PFetchDataStruct, 1);
  PFetchHandle    pfetch = NULL;
  PFetchUserPrefsStruct prefs = {NULL};
  gboolean  debug_pfetch = FALSE;

  if((zmapWindowGetPFetchUserPrefs(window->sequence->config_file, &prefs)) && (prefs.location != NULL))
    {
      GType pfetch_type = PFETCH_TYPE_HTTP_HANDLE;

      if(prefs.mode && strncmp(prefs.mode, "pipe", 4) == 0)
        pfetch_type = PFETCH_TYPE_PIPE_HANDLE;

      pfetch_data->pfetch = pfetch = PFetchHandleNew(pfetch_type);

      if((pfetch_data->title = g_strdup_printf(PFETCH_TITLE_FORMAT, sequence_name)))
        {
          pfetch_data->dialog = zMapGUIShowTextFull(pfetch_data->title, "pfetching...\n",
                                                    FALSE, NULL, &(pfetch_data->text_buffer));

          pfetch_data->widget_destroy_handler_id =
            g_signal_connect(G_OBJECT(pfetch_data->dialog), "destroy",
                             G_CALLBACK(handle_dialog_close), pfetch_data);
        }

      PFetchHandleSettings(pfetch,
                           "debug",       debug_pfetch,
                           "full",        prefs.full_record,
                           "pfetch",      prefs.location,
                           "isoform-seq", TRUE,
                           NULL);

      if(PFETCH_IS_HTTP_HANDLE(pfetch))
        PFetchHandleSettings(pfetch,
                             "port",       prefs.port,
                             "cookie-jar", prefs.cookie_jar,
                             NULL);

      g_free(prefs.location);
      g_free(prefs.cookie_jar);

      g_signal_connect(G_OBJECT(pfetch), "reader", G_CALLBACK(pfetch_reader_func), pfetch_data);

      g_signal_connect(G_OBJECT(pfetch), "error",  G_CALLBACK(pfetch_reader_func), pfetch_data);

      g_signal_connect(G_OBJECT(pfetch), "closed", G_CALLBACK(pfetch_closed_func), pfetch_data);

      /* It would seem that PFetchHandleFetch() calls g_spawn_async_with_pipes() which is not
       * thread safe so lock round it...should locks be in pfetch code ?? */
      zMapThreadForkLock();   // see zmapThreads.c

      if(PFetchHandleFetch(pfetch, sequence_name) == PFETCH_STATUS_FAILED)
              zMapWarning("Error fetching sequence '%s'", sequence_name);

      zMapThreadForkUnlock();
    }
  else
    {
      zMapWarning("%s", "Failed to obtain preferences for pfetch.\n"
                  "ZMap's config file needs at least pfetch "
                  "entry in the ZMap stanza.");
    }

  return ;
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

  g_return_val_if_fail(style != NULL, strand);

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
FooCanvasItem *zmapWindowFeatureDraw(ZMapWindow      window,
                                     ZMapFeatureTypeStyle style,
                                     ZMapWindowContainerFeatureSet set_group,
                                     ZMapWindowContainerFeatures set_features,
                                     FooCanvasItem *foo_featureset,
                                     ZMapWindowFeatureStack     feature_stack)
{
  FooCanvasItem *new_feature = NULL ;
  ZMapWindowContainerFeatureSet container = (ZMapWindowContainerFeatureSet) set_group;
  gboolean masked;
  ZMapFeature feature = feature_stack->feature;

#if MH17_REVCOMP_DEBUG
  zMapLogWarning("FeatureDraw %d-%d",feature->x1,feature->x2);
#endif

#if DONE_BY_CALLER
  /* Users will often not want to see what is on the reverse strand, style specifies what should
   * be shown. */
  if ((zMapStyleIsStrandSpecific(style)) &&
      ((feature->strand == ZMAPSTRAND_REVERSE) && (!zMapStyleIsShowReverseStrand(style))))
    {
      zMapWarnIfReached();
      return NULL ;
    }
  if ((zMapStyleIsStrandSpecific(style)) && window->flags[ZMAPFLAG_REVCOMPED_FEATURES] &&
      ((feature->strand == ZMAPSTRAND_FORWARD) && (zMapStyleIsHideForwardStrand(style))))
    {
      zMapWarnIfReached();
      return NULL ;
    }
#endif

#if MH17_REVCOMP_DEBUG
  zMapLogWarning("right strand %d",feature->strand);
#endif


  masked = (zMapStyleGetMode(style) == ZMAPSTYLE_MODE_ALIGNMENT &&  feature->feature.homol.flags.masked);
  if(masked)
    {
      if(container->masked && !window->highlights_set.masked)
        {
#if MH17_REVCOMP_DEBUG
          zMapLogWarning("masked","");
#endif


          return NULL;
        }
    }

  new_feature = zmapWindowFToIFactoryRunSingle(window->context_to_item,
#if RUN_SET
                                               NULL,
#endif
                                               set_group, set_features,
                                               foo_featureset,
                                               feature_stack);

  //#warning could make this take a window not a gpointer, would be more readable
  //#warning ideally only call this first time canvasfeatureset is created
  //  if(!zMapWindowCanvasItemIsConnected((ZMapWindowCanvasItem) new_feature))
  if(new_feature != foo_featureset)
    factoryTopItemCreated (new_feature, feature_stack, (gpointer) window);

  if(masked && container->masked && new_feature)
    foo_canvas_item_hide(new_feature);

  return new_feature;
}


/* Look for any description in the feature set. */
char *zmapWindowFeatureSetDescription(ZMapFeatureSet feature_set)
{
  char *description = NULL ;

  description = g_strdup_printf("%s  :  %s%s%s", (char *)g_quark_to_string(feature_set->original_id),
                                feature_set->description ? "\"" : "",
                                feature_set->description ? feature_set->description : "<no description available>",
                                feature_set->description ? "\"" : "") ;

  return description ;
}


/* Get various aspects of features source... */
void zmapWindowFeatureGetSourceTxt(ZMapFeature feature, char **source_name_out, char **source_description_out)
{
  if (feature->source_id)
    *source_name_out = g_strdup(g_quark_to_string(feature->source_id)) ;

  if (feature->source_text)
    *source_description_out = g_strdup(g_quark_to_string(feature->source_text)) ;

  return ;
}


char *zmapWindowFeatureDescription(ZMapFeature feature)
{
  char *description = NULL ;

  if (!zMapFeatureIsValid((ZMapFeatureAny)feature))
    return description ;


  if (feature->description)
    {
      description = g_strdup(feature->description) ;
    }

  return description ;
}


gboolean zMapWindowGetDNAStatus(ZMapWindow window)
{
  gboolean drawable = FALSE;

  /* We just need one of the blocks to have DNA.
   * This enables us to turn on this button as we
   * can't have half sensitivity.  Any block which
   * doesn't have DNA creates a warning for the user
   * to complain about.
   */

  /* check for style too. */
  /* sometimes we don't have a feature_context ... ODD! */
  if(window->feature_context
     && zMapFindStyle(window->context_map->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
    {
      drawable = zMapFeatureContextGetDNAStatus(window->feature_context);
    }

  return drawable;
}


/* Get the peptide as a fasta string.  All contained in a function so
 * there's no falling over phase values and stop codons... */
char *zmapWindowFeatureTranscriptFASTA(ZMapFeature feature, gboolean spliced, gboolean cds_only)
{
  char *peptide_fasta = NULL ;
  ZMapFeatureContext context ;


  if((feature->mode == ZMAPSTYLE_MODE_TRANSCRIPT)
     && ((context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature,
                                                                  ZMAPFEATURE_STRUCT_CONTEXT))))
    {
      ZMapPeptide peptide;
      char *dna, *seq_name = NULL, *gene_name = NULL;
      int pep_length, start_incr = 0;

      seq_name  = (char *)g_quark_to_string(context->original_id);
      gene_name = (char *)g_quark_to_string(feature->original_id);

      if ((dna = zMapFeatureGetTranscriptDNA(feature, spliced, cds_only)))
        {
          /* Adjust for when its known that the start exon is incomplete.... */
          if (feature->feature.transcript.flags.start_not_found)
            start_incr = feature->feature.transcript.start_not_found - 1 ; /* values are 1 <= start_not_found <= 3 */

          peptide = zMapPeptideCreate(seq_name, gene_name, (dna + start_incr), NULL, TRUE) ;

          /* Note that we do not include the "Stop" in the peptide length, is this the norm ? */
          pep_length = zMapPeptideLength(peptide) ;
          if (zMapPeptideHasStopCodon(peptide))
            pep_length-- ;

          peptide_fasta = zMapFASTAString(ZMAPFASTA_SEQTYPE_AA,
                                          seq_name, "Protein",
                                          gene_name, pep_length,
                                          zMapPeptideSequence(peptide));

          g_free(dna) ;
        }
    }

  return peptide_fasta;
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



/* Handle events on items, note that events for text items are passed through without processing
 * so the text item code can do highlighting etc. */
static gboolean canvasItemEventCB(FooCanvasItem *item, GdkEvent *event, gpointer data)
{
  gboolean event_handled = FALSE ;                            /* By default we _don't_ handle events. */
  ZMapWindow window = (ZMapWindowStruct*)data ;
  static ZMapFeature feature = NULL ;
  static guint32 last_but_release = 0 ;                            /* Used for double clicks... */
  static gboolean second_press = FALSE ;                    /* Used for double clicks... */
  static gboolean dragging = FALSE ;                        /* Have clicked button 1 but not yet released */
  static gboolean dnd_in_progress = FALSE ;                 /* Drag and drop is in progress */

  /* Set up targets for drag and drop */
  static GtkTargetEntry targetentries[] =
    {
      { "STRING",        0, TARGET_STRING },
      { "text/plain",    0, TARGET_STRING },
      { "text/uri-list", 0, TARGET_URL },
    };

  static GtkTargetList *targetlist = NULL;

  if (!targetlist)
    targetlist= gtk_target_list_new(targetentries, 3);


  if (event->type == GDK_BUTTON_PRESS || event->type == GDK_2BUTTON_PRESS  || event->type == GDK_BUTTON_RELEASE)
    {
      GdkEventButton *but_event = (GdkEventButton *)event ;
      FooCanvasItem *highlight_item = NULL ;


      /* this is absolutely hateful coding.....the canvascolumn event func. should call this
         function, not the other way round....... I should do that later......! */

      if (!zMapWindowCanvasFeaturesetHasPointFeature(item))
        {
          /* click on column not feature */
          event_handled = zmapWindowColumnBoundingBoxEventCB(item, event, data) ;
        }
      else
        {
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
                  dragging = TRUE ;
                  dnd_in_progress = FALSE ;
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
              second_press = TRUE ;

              event_handled = TRUE ;
            }
          else                                                    /* button release */
            {
              /*                            button release                             */
              dragging = FALSE ;
              dnd_in_progress = FALSE ;

              /* Gdk defines double clicks as occuring within 250 milliseconds of each other
               * but unfortunately if on the first click we do a lot of processing,
               * STUPID Gdk no longer delivers the GDK_2BUTTON_PRESS so we have to do this
               * hack looking for the difference in time. This can happen if user clicks on
               * a very large feature causing us to paste a lot of text to the selection
               * buffer. */
              guint but_threshold = 500 ;                            /* Separation of clicks in milliseconds. */

              if (second_press || but_event->time - last_but_release < but_threshold)
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
                           * Ask the peer to create it. */
                          /*! \todo We may wish to change this so that, rather than creating
                           * the feature immediately, it opens an intermediary dialog where the
                           * user can set some attributes locally in zmap instead. Then from
                           * that dialog, or another option in zmap, they could have the option
                           * to save the feature to the peer. */
                          if (feature && feature->style && strcmp(style_id, ZMAP_FIXED_STYLE_SCRATCH_NAME) == 0)
                            {
                              callXRemote(window, (ZMapFeatureAny)feature, ZACP_CREATE_FEATURE, highlight_item) ;
                            }
                          else
                            {
                              callXRemote(window, (ZMapFeatureAny)feature, ZACP_EDIT_FEATURE, highlight_item) ;
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

                  second_press = FALSE ;
                }
              else
                {


                  event_handled = handleButton(but_event, window, item, feature) ;
                }

              last_but_release = but_event->time ;

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
    }
  else if (event->type == GDK_MOTION_NOTIFY)
    {
      /* We initiate drag-and-drop if the user has started dragging a feature that is already
       * selected. "dragging" indicates that a left-click has happened and not been released yet, and
       * "dnd_in_progress" means that a drag-and-drop has already been initiated (so there's
       * nothing left to do). */
      if (!dnd_in_progress && dragging && item && window)
        {
          if (zmapWindowFocusIsItemFocused(window->focus, item))
            {
              gtk_drag_begin(GTK_WIDGET(window->canvas), targetlist, 
                             GDK_ACTION_COPY|GDK_ACTION_MOVE|GDK_ACTION_LINK,
                             1, (GdkEvent*)event);
              
              dnd_in_progress = TRUE ;
            }
        }
    }


  return event_handled ;
}


/* Handle button single click to highlight and double click to show feature details. */
static gboolean handleButton(GdkEventButton *but_event, ZMapWindow window, FooCanvasItem *item, ZMapFeature feature)
{
  gboolean event_handled = FALSE ;
  GdkModifierType shift_mask = GDK_SHIFT_MASK,
    control_mask = GDK_CONTROL_MASK,
    alt_mask = GDK_MOD1_MASK,
    meta_mask = GDK_META_MASK,
    unwanted_masks = (GDK_LOCK_MASK | GDK_MOD2_MASK | GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK
                      | GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK
                      | GDK_BUTTON4_MASK | GDK_BUTTON5_MASK),
    locks_mask ;

  /* In order to make the modifier only checks work we need to OR in the unwanted masks that might be on.
   * This includes the shift lock and num lock. Depending on the setup of X these might be mapped
   * to other things which is why MODs 2-5 are included This in theory should include the new (since 2.10)
   * GDK_SUPER_MASK, GDK_HYPER_MASK and GDK_META_MASK */
  if ((locks_mask = (but_event->state & unwanted_masks)))
    {
      shift_mask |= locks_mask ;
      control_mask |= locks_mask ;
      alt_mask |= locks_mask ;
      meta_mask |= locks_mask ;
    }

  /* Button 1 and 3 are handled, 2 is left for a general handler which could be the root handler. */
  if (but_event->button == 1 || but_event->button == 3)
    {
      FooCanvasItem *sub_item = NULL, *highlight_item = NULL ;
      gboolean replace_highlight = TRUE, highlight_same_names = TRUE ;
      ZMapFeatureSubPartSpan sub_feature = NULL ;
      ZMapWindowCanvasItem canvas_item ;
      ZMapFeatureStruct feature_copy = {};
      ZMapFeatureAny my_feature = (ZMapFeatureAny) feature ;
      gboolean control = FALSE;
      ZMapWindowDisplayStyleStruct display_style = {ZMAPWINDOW_COORD_ONE_BASED, ZMAPWINDOW_PASTE_FORMAT_OTTERLACE,
                                                    ZMAPWINDOW_PASTE_TYPE_ALLSUBPARTS} ;

      canvas_item = ZMAP_CANVAS_ITEM(item);
      highlight_item = item;

      sub_item = zMapWindowCanvasItemGetInterval(canvas_item, but_event->x, but_event->y, &sub_feature);

      if (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT || zMapStyleIsUnique(*feature->style))
        highlight_same_names = FALSE ;


      if (zMapGUITestModifiers(but_event, control_mask))
        {
          /* Only highlight the single item user clicked on. */
          highlight_same_names = FALSE ;
          control = TRUE;

          /* Annotators say they don't want subparts sub selections + multiple
           * selections for alignments. */
          if (feature->mode != ZMAPSTYLE_MODE_ALIGNMENT)
            {
              highlight_item = sub_item ;

              /* monkey around to get feature_copy to be the right correct data */
              featureCopySelectedItem(feature, &feature_copy, highlight_item);
              my_feature = (ZMapFeatureAny) &feature_copy;
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
        /* mh17 Foo sequence features have a diff interface, but we wish to avoid that, see sequenceSelectionCB() above */
        /* using a CanvasFeatureset we get here, first off just pass a single coord through so it does not crash */
        /* InfoPanel has two sets of coords, but they appear the same in totalview */
        /* possibly we can hide region selection in the GetInterval call above: we can certainly use the X coordinate ?? */

        int start = feature->x1, end = feature->x2;

        if (sub_feature)
          {
            start = sub_feature->start;
            end = sub_feature->end;
          }

        /* Pass information about the object clicked on back to the application. */
        zmapWindowUpdateInfoPanel(window, feature, NULL, item, sub_feature, start, end, start, end,
                                  NULL, replace_highlight, highlight_same_names, control, &display_style) ;

        /* if we have an active dialog update it: they have to click on a feature not the column */
        zmapWindowSetStyleFeatureset(window, item, feature);

      }

      if (zMapGUITestModifiers(but_event, shift_mask))
        {
          /* multiple selections */
          if (zmapWindowFocusIsItemInHotColumn(window->focus, item))      //      && window->multi_select)
            {
              replace_highlight = FALSE ;

              if (window->xremote_client)
                callXRemote(window, my_feature, ZACP_SELECT_MULTI_FEATURE, highlight_item) ;
            }
          else
            {
              if (window->xremote_client)
                callXRemote(window, my_feature, ZACP_SELECT_FEATURE, highlight_item) ;

              window->multi_select = TRUE ;
            }
        }
      else
        {
          /* single select */
          if (window->xremote_client)
            callXRemote(window, my_feature, ZACP_SELECT_FEATURE, highlight_item) ;

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


  for(l = feature->children; l; l = l->next)
    {
      /* (mh17) NOTE we have to be careful that these features end up in the same (singleton) CanvasFeatureset else they overlap on bump */
      feature_stack.feature = (ZMapFeature) l->data;
      item = (ZMapWindowCanvasItem) zmapWindowFeatureDraw(window, *feature->style,  container_set,features, foo_featureset, &feature_stack);
      foo_featureset = (FooCanvasItem *) item;
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




static PFetchStatus pfetch_reader_func(PFetchHandle *handle, char *text, guint *actual_read,
                                       GError *error, gpointer user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  PFetchStatus status    = PFETCH_STATUS_OK;

  if(actual_read && *actual_read > 0 && pfetch_data)
    {
      GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(pfetch_data->text_buffer);

      /* clear the buffer the first time... */
      if(pfetch_data->got_response == FALSE)
        gtk_text_buffer_set_text(text_buffer, "", 0);

      gtk_text_buffer_insert_at_cursor(text_buffer, text, *actual_read);

      pfetch_data->got_response = TRUE;
    }

  return status;
}

static PFetchStatus pfetch_closed_func(gpointer user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;
#ifdef DEBUG_ONLY
  printf("pfetch closed\n");
#endif        /* DEBUG_ONLY */
  return status;
}

/* GtkObject destroy signal handler */
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data)
{
  PFetchData pfetch_data   = (PFetchData)user_data;
  pfetch_data->text_buffer = NULL;
  pfetch_data->widget_destroy_handler_id = 0; /* can we get this more than once? */

  if(pfetch_data->pfetch)
    pfetch_data->pfetch = PFetchHandleDestroy(pfetch_data->pfetch);

  return ;
}


static gboolean factoryTopItemCreated(FooCanvasItem *top_item,
                                      ZMapWindowFeatureStack feature_stack,
                                      gpointer handler_data)
{
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

      //      gtk_widget_set_events(GTK_WIDGET(top_item),GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
      // not a widget it's a canvas item
      // see if this speeds up up! (no difference)
      g_signal_connect(G_OBJECT(top_item), "event", G_CALLBACK(canvasItemEventCB), handler_data);

      break;

    default:
      break;
    }

  /* this is completely pointless as is this whole routine..... */
  zMapWindowCanvasItemSetConnected((ZMapWindowCanvasItem) top_item, TRUE);


  return TRUE ;
}


/* THIS FUNCTION NEEDS CLEANING UP, HACKED FROM OLD XREMOTE CODE..... */
/* Called by select and edit code to call xremote with "select", "edit" etc commands
 * for peer. The peers reply is handled in handleXRemoteReply() */
static void callXRemote(ZMapWindow window, ZMapFeatureAny feature_any,
                        char *command, FooCanvasItem *real_item)
{
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapXMLUtilsEventStack xml_elements ;
  ZMapWindowSelectStruct select = {0} ;
  ZMapFeatureSetStruct feature_set = {0} ;
  ZMapFeatureSet multi_set ;
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
  if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
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
  feature_set.parent      = feature_copy->parent->parent;
  feature_set.unique_id   = feature_copy->parent->unique_id;
  feature_set.original_id = feature_copy->parent->original_id;

  feature_set.features = g_hash_table_new(NULL, NULL) ;
  g_hash_table_insert(feature_set.features, GINT_TO_POINTER(feature_copy->unique_id), feature_copy) ;

  multi_set = &feature_set ;


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
    }

  g_free(remote_data) ;                                            /* Allocated in callXRemote() */

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
  RemoteData remote_data = (RemoteData)user_data ;





  return ;
}


