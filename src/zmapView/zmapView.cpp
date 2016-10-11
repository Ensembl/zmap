/*  File: zmapView.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Handles the getting of the feature context from sources
 *              and their subsequent processing and viewing.
 *              zMapView routines receive requests to load, display
 *              and process feature contexts. Each ZMapView corresponds
 *              to a single feature context.
 *
 * Exported functions: See ZMap/zmapView.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string>
#include <list>
#include <map>

#include <string.h>
#include <sys/types.h>
#include <signal.h>                                            /* kill() */
#include <glib.h>

#include <gbtools/gbtools.hpp>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsDebug.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapCmdLineArgs.hpp>
#include <ZMap/zmapConfigDir.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapCmdLineArgs.hpp>
#include <ZMap/zmapUrlUtils.hpp>
#include <ZMap/zmapOldSourceServer.hpp>

#include <zmapView_P.hpp>


using namespace std ;

using namespace ZMapThreadSource ;


#define SOURCE_FAILURE_WARNING_FORMAT "Error loading source(s). First error was:\n\n%s\n\nFurther source failures will not be reported. See the log file for details of any other failures."

/* Define thread debug messages, used in checkStateConnections() mostly. */
#define THREAD_DEBUG_MSG_PREFIX " Reply from slave thread %s, "

#define THREAD_DEBUG_MSG(CHILD_THREAD, FORMAT_STR, ...)        \
  G_STMT_START                                                                \
  {                                                                        \
    if (thread_debug_G)                                                 \
      {                                                                 \
        char *thread_str ;                                              \
                                                                        \
        thread_str = zMapThreadGetThreadID((CHILD_THREAD)) ;                \
                                                                        \
        zMapDebugPrint(thread_debug_G, THREAD_DEBUG_MSG_PREFIX FORMAT_STR, thread_str, __VA_ARGS__) ; \
                                                                        \
        zMapLogMessage(THREAD_DEBUG_MSG_PREFIX FORMAT_STR, thread_str, __VA_ARGS__) ; \
                                                                        \
        g_free(thread_str) ;                                                \
      }                                                                 \
  } G_STMT_END

/* Define thread debug messages with extra information, used in checkStateConnections() mostly. */
#define THREAD_DEBUG_MSG_FULL(CHILD_THREAD, VIEW_CON, REQUEST_TYPE, REPLY, FORMAT_STR, ...) \
  G_STMT_START                                                                \
  {                                                                        \
    if (thread_debug_G)                                                 \
      {                                                                 \
        GString *msg_str ;                                              \
                                                                        \
        msg_str = g_string_new("") ;                                    \
                                                                        \
        g_string_append_printf(msg_str, "status = \"%s\", request = \"%s\", reply = \"%s\", msg = \"" FORMAT_STR "\"", \
                               zMapViewThreadStatus2ExactStr((VIEW_CON)->thread_status), \
                               zMapServerReqType2ExactStr((REQUEST_TYPE)), \
                               zMapThreadReply2ExactStr((REPLY)),       \
                               __VA_ARGS__) ;                           \
                                                                        \
        g_string_append_printf(msg_str, ", url = \"%s\"", zMapServerGetUrl((VIEW_CON))) ; \
                                                                        \
        THREAD_DEBUG_MSG((CHILD_THREAD), "%s", msg_str->str) ;          \
                                                                        \
        g_string_free(msg_str, TRUE) ;                                  \
      }                                                                 \
  } G_STMT_END













static void getIniData(ZMapView view, char *config_str, GList *sources) ;

static ZMapView createZMapView(char *view_name, GList *sequences, void *app_data) ;
static void destroyZMapView(ZMapView *zmap) ;
static void displayDataWindows(ZMapView zmap_view,
                               ZMapFeatureContext all_features, ZMapFeatureContext new_features,
                               LoadFeaturesData loaded_features,
                               gboolean undisplay, GList *masked,
                               ZMapFeature highlight_feature, gboolean splice_highlight,
                               gboolean allow_clean) ;
static gint zmapIdleCB(gpointer cb_data) ;
static void enterCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void leaveCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void scrollCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewFocusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void commandCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void loadedDataCB(ZMapWindow window, void *caller_data, gpointer loaded_data, void *window_data) ;
static gboolean mergeNewFeatureCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data);
static void setZoomStatus(gpointer data, gpointer user_data);
static void splitMagic(gpointer data, gpointer user_data);
static void doBlixemCmd(ZMapView view, ZMapWindowCallbackCommandAlign align_cmd) ;

static void startStateConnectionChecking(ZMapView zmap_view) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void stopStateConnectionChecking(ZMapView zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean checkStateConnections(ZMapView zmap_view) ;


static gboolean processGetSeqRequests(void *user_data, ZMapServerReqAny req_any) ;

static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;

static void resetWindows(ZMapView zmap_view) ;

static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window) ;
static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window) ;

static GtkResponseType checkForUnsavedAnnotation(ZMapView zmap_view) ;
static GtkResponseType checkForUnsavedFeatures(ZMapView zmap_view) ;
//static GtkResponseType checkForUnsavedConfig(ZMapView zmap_view, ZMapViewExportType export_type) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GList *string2StyleQuarks(char *feature_sets) ;
static gboolean nextIsQuoted(char **text) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget) ;

#if 0
static void addAlignments(ZMapFeatureContext context) ;
#endif

static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout);

#ifdef NOT_REQUIRED_ATM
static gboolean getSequenceServers(ZMapView zmap_view, char *config_str) ;
static void destroySeq2ServerCB(gpointer data, gpointer user_data_unused) ;
static ZMapViewSequence2Server createSeq2Server(char *sequence, char *server) ;
static void destroySeq2Server(ZMapViewSequence2Server seq_2_server) ;
static gboolean checkSequenceToServerMatch(GList *seq_2_server, ZMapViewSequence2Server target_seq_server) ;
static gint findSequence(gconstpointer a, gconstpointer b) ;
#endif /* NOT_REQUIRED_ATM */

static void killAllSpawned(ZMapView zmap_view);

static gboolean checkContinue(ZMapView zmap_view) ;


static void addPredefined(ZMapStyleTree &styles, GHashTable **column_2_styles_inout) ;
static void styleCB(gpointer key_id, gpointer data, gpointer user_data) ;

static void sendViewLoaded(ZMapView zmap_view, LoadFeaturesData lfd) ;

static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data) ;
static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key,
                                                        gpointer data,
                                                        gpointer user_data,
                                                        char **error_out) ;
static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **error_out) ;


#define DEBUG_CONTEXT_MAP        0

#if DEBUG_CONTEXT_MAP
void print_source_2_sourcedata(char * str,GHashTable *data) ;
void print_fset2col(char * str,GHashTable *data) ;
void print_col2fset(char * str,GHashTable *data) ;
#endif

static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
                                  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                  gpointer reply_handler_func_data) ;

static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void getWindowList(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void viewWindowsMergeColumns(ZMapView zmap_view) ;
static void viewSetUpStyles(ZMapView zmap_view, char *stylesfile) ;
static void viewSetUpPredefinedColumns(ZMapView zmap_view, ZMapFeatureSequenceMap sequence_map) ;

static ZMapFeatureContextExecuteStatus add_default_styles(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                          char **error_out) ;

static gboolean zMapViewSortExons(ZMapFeatureContext diff_context) ;





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapViewConnection zmapViewRequestServer(ZMapView view, ZMapViewConnection view_conn,
                                                ZMapFeatureBlock block_orig, GList *req_featuresets, GList *req_biotypes,
                                                ZMapConfigSource server,
                                                const char *req_sequence, int req_start, int req__end,
                                                gboolean dna_requested, gboolean terminate, gboolean show_warning) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/*
 *                 Globals
 */


/* These callback routines are global because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;

/* Callbacks back we set in the level below us, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G =
{
  enterCB, leaveCB,
  scrollCB,
  viewFocusCB,
  viewSelectCB,
  viewSplitToPatternCB,
  setZoomStatusCB,
  viewVisibilityChangeCB,
  commandCB,
  loadedDataCB,
  mergeNewFeatureCB,
  NULL,
  NULL
} ;


/* Turn on/off debugging output for thread handling especially in checkStateConnections().
 * Note, this var is used explicitly in the THREAD_DEBUG_MSG() macro. */
static gboolean thread_debug_G = FALSE ;


/*
 *                           External routines
 */


/* This routine must be called just once before any other views routine,
 * the caller must supply all of the callback routines.
 *
 * callbacks   Caller registers callback functions that view will call
 *                    from the appropriate actions.
 *  */
void zMapViewInit(ZMapViewCallbacks callbacks)
{

  zMapReturnIfFail((callbacks
                    && callbacks->enter
                    && callbacks->leave
                    && callbacks->load_data
                    && callbacks->focus
                    && callbacks->select
                    && callbacks->visibility_change
                    && callbacks->state_change
                    && callbacks->destroy)) ;

  /* Now we can get on. */
  view_cbs_G = g_new0(ZMapViewCallbacksStruct, 1) ;

  view_cbs_G->enter = callbacks->enter ;
  view_cbs_G->leave = callbacks->leave ;
  view_cbs_G->load_data = callbacks->load_data ;
  view_cbs_G->focus = callbacks->focus ;
  view_cbs_G->select = callbacks->select ;
  view_cbs_G->split_to_pattern = callbacks->split_to_pattern;
  view_cbs_G->visibility_change = callbacks->visibility_change ;
  view_cbs_G->state_change = callbacks->state_change ;
  view_cbs_G->destroy = callbacks->destroy ;
  view_cbs_G->remote_request_func = callbacks->remote_request_func ;
  view_cbs_G->remote_request_func_data = callbacks->remote_request_func_data ;

  /* Init windows.... */
  window_cbs_G.remote_request_func = callbacks->remote_request_func ;
  window_cbs_G.remote_request_func_data = callbacks->remote_request_func_data ;
  zMapWindowInit(&window_cbs_G) ;


  return ;
}


/* Return the global callbacks registered with view by caller. */
ZMapViewCallbacks zmapViewGetCallbacks(void)
{
  ZMapViewCallbacks call_backs ;

  call_backs = view_cbs_G ;

  return call_backs ;
}



/* Create a "view", this is the holder for a single feature context. The view may use
 * several threads to get this context and may display it in several windows.
 * A view _always_ has at least one window, this window may be blank but as long as
 * there is a view, there is a window. This makes the coding somewhat simpler and is
 * intuitively sensible.
 *
 * @param xremote_widget   Widget that xremote commands for this view will be delivered to.
 * @param view_container   Parent widget of the view window(s)
 * @param sequence         Name of virtual sequence of context to be created.
 * @param start            Start coord of virtual sequence.
 * @param end              End coord of virtual sequence.
 * @param app_data         data that will be passed to the callers callback routines.
 * @return a new ZMapViewWindow (= view + a window)
 *  */
ZMapViewWindow zMapViewCreate(GtkWidget *view_container, ZMapFeatureSequenceMap sequence_map, void *app_data)
{
  ZMapViewWindow view_window = NULL ;
  ZMapView zmap_view = NULL ;
  ZMapFeatureSequenceMap sequence_fetch ;
  GList *sequences_list = NULL ;
  char *view_name ;
  int curr_scr_num, num_screens ;


  zMapReturnValIfFail((GTK_IS_WIDGET(view_container)), view_window);
  zMapReturnValIfFail((sequence_map->sequence
                       && (sequence_map->start > 0 && sequence_map->start <= sequence_map->end)), view_window) ;

  /* Set up debugging for threads and servers, we do it here so that user can change setting
   * in config file and next time they create a view the debugging will go on/off */
  zMapUtilsConfigDebug(sequence_map->config_file) ;

  zMapInitTimer() ;                                         /* operates as a reset if already
                                                               defined. */


  /* I DON'T UNDERSTAND WHY THERE IS A LIST OF SEQUENCES HERE.... */

  /* Set up sequence to be fetched, in this case server defaults to whatever is set in config. file. */
  sequence_fetch = sequence_map->copy() ;
  sequences_list = g_list_append(sequences_list, sequence_fetch) ;

  view_name = sequence_map->sequence ;


  /* UM....THIS IS RUBBISH...WE GET PASSED THIS ALWAYS....CHECK THAT THIS IS TRUE !! */
  if(!sequence_map->start)
    {
      /* this should use coords extracted from ACEDB/smap or provided by otterlace
       * but unfortunately the info is not available
       * so we use this unsafe mechanism in the interim
       */
      char *p = sequence_fetch->sequence;

      while(*p && *p != '_')
        p++;

      if(*p)
        {
          p++;
          sequence_fetch->start = atol(p);

          while(*p && *p != '_')
            p++;
          if(*p)
            {
              p++;
              sequence_fetch->start = atol(p);
            }
        }
    }


  zmap_view = createZMapView(view_name, sequences_list, app_data) ; /* N.B. this step can't fail. */

  /* If we have multiple screens then work out where we should show blixem. */
  if (zMapGUIGetScreenInfo(view_container, &curr_scr_num, &num_screens) && num_screens > 1)
    {
      ZMapCmdLineArgsType value ;

      zmap_view->multi_screen = TRUE ;

      /* Can be overridden by user from commandline. */
      if ((zMapCmdLineArgsValue(ZMAPARG_SINGLE_SCREEN, &value)) && (value.b))
        zmap_view->multi_screen = FALSE ;

      if (zmap_view->multi_screen)
        {
          /* Crude but it will do for now. */
          if (curr_scr_num == 0)
            zmap_view->blixem_screen = 1 ;
          else if (curr_scr_num > 0)
            zmap_view->blixem_screen = 0 ;
        }
      else
        {
          zmap_view->blixem_screen = curr_scr_num ;
        }
    }



  if (view_cbs_G->remote_request_func)
    zmap_view->remote_control = TRUE ;

  zmap_view->state = ZMAPVIEW_INIT ;

  view_window = addWindow(zmap_view, view_container) ;

  zmap_view->cwh_hash = zmapViewCWHHashCreate();

  zmap_view->context_map.column_2_styles = zMap_g_hashlist_create() ;

  return view_window ;
}



void zMapViewSetupNavigator(ZMapViewWindow view_window, GtkWidget *canvas_widget)
{
  ZMapView zmap_view ;

  zMapReturnIfFail((view_window)) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
//printf("view setupnavigator\n");
      zmap_view->navigator_window = zMapWindowNavigatorCreate(canvas_widget);
      zMapWindowNavigatorSetCurrentWindow(zmap_view->navigator_window, view_window->window);
    }

  return ;
}


/* Connect a View to its databases via threads, at this point the View is blank and waiting
 * to be called to load some data.
 *
 * WHAT IS config_str ?????????? I THINK IT'S A BIT CONTEXT FILE SPECIFYING A SEQUENCE.
 *
 * I'm adding an arg to specify a different config file.
 *
 *
 *  */
gboolean zMapViewConnect(ZMapFeatureSequenceMap sequence_map, ZMapView zmap_view, char *config_str, GError **error)
{
  gboolean result = TRUE ;
  char *stylesfile = NULL;
  GError *tmp_error = NULL ;

  if (zmap_view->state > ZMAPVIEW_MAPPED)        // && zmap_view->state != ZMAPVIEW_LOADED)

    {
      zMapLogCritical("GUI: %s", "cannot connect because not in initial state.") ;
      g_set_error(&tmp_error, ZMAP_VIEW_ERROR, ZMAPVIEW_ERROR_STATE, "Cannot connect because not in initial state") ;

      result = FALSE ;
    }
  else
    {
      GList *settings_list = NULL ;

      zMapPrintTimer(NULL, "Open connection") ;

      /* There is some redundancy of state here as the below code actually does a connect
       * and load in one call but we will almost certainly need the extra states later... */
      zmap_view->state = ZMAPVIEW_CONNECTING ;

      if(!zmap_view->view_sequence->config_file)
        {
          zmap_view->view_sequence->config_file = zMapConfigDirGetFile();
        }

      /* set the default stylesfile */
      stylesfile = zmap_view->view_sequence->stylesfile;

      zmap_view->view_sequence->addSourcesFromConfig(config_str, &stylesfile) ;
      settings_list = zmap_view->view_sequence->getSources() ;

      viewSetUpStyles(zmap_view, stylesfile) ;

      /* read in a few ZMap stanzas giving column groups etc. */
      getIniData(zmap_view, config_str, settings_list) ;

      if (!zmap_view->features)
        {
          viewSetUpPredefinedColumns(zmap_view, sequence_map) ;
        }

      /* Merge the list of columns into each window's list of feature_set_names. */
      viewWindowsMergeColumns(zmap_view) ;

      /* Set up connections to named servers */
      result = zmapViewSetUpServerConnections(zmap_view, settings_list, &tmp_error) ;

      if (!result)
        {
          zmap_view->state = ZMAPVIEW_LOADED ;    /* no initial servers just pretend they've loaded */
        }


      /* Start polling function that checks state of this view and its connections,
       * this will wait until the connections reply, process their replies and then
       * execute any further steps. */
      startStateConnectionChecking(zmap_view) ;
    }

  //  if(stylesfile)
  //    g_free(stylesfile);

  if (tmp_error)
    g_propagate_error(error, tmp_error) ;

  return result ;
}



/*!
 * \brief Returns true if filtered columns should be highlighted
 */
gboolean zMapViewGetHighlightFilteredColumns(ZMapView view)
{
  return zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS) ;
}


char *zMapViewGetDataset(ZMapView zmap_view)
{
  char *dataset = NULL ;

  if (zmap_view && zmap_view->view_sequence)
  {
    dataset = zmap_view->view_sequence->dataset ;
  }

  return dataset ;
}



/* Copies an existing window in a view.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
                                  ZMapWindow copy_window, ZMapWindowLockType window_locking)
{
  ZMapViewWindow view_window = NULL ;

  zMapReturnValIfFail((zmap_view->state != ZMAPVIEW_DYING), view_window) ;
  zMapReturnValIfFail((zmap_view && parent_widget && zmap_view->window_list), view_window) ;

  /* the view _must_ already have a window _and_ data. */
  view_window = createWindow(zmap_view, NULL) ;

  if (!(view_window->window = zMapWindowCopy(parent_widget, zmap_view->view_sequence,
                                             view_window, copy_window,
                                             zmap_view->features,
                                             window_locking)))
    {
      /* should glog and/or gerror at this stage....really need g_errors.... */
      /* should free view_window.... */

      view_window = NULL ;
    }
  else
    {
      /* add to list of windows.... */
      zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;

    }

  return view_window ;
}


/* Returns number of windows for current view. */
int zMapViewNumWindows(ZMapViewWindow view_window)
{
  int num_windows = 0 ;
  ZMapView zmap_view ;

  zMapReturnValIfFail((view_window), num_windows) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->window_list)
    {
      num_windows = g_list_length(zmap_view->window_list) ;
    }

  return num_windows ;
}


/* Removes a window from a view, the last window cannot be removed, the view must
 * always have at least one window. The function does nothing if there is only one
 * window. If the window is removed NULL is returned, otherwise the original
 * pointer is returned.
 *
 * Should be called like this:
 *
 * view_window = zMapViewRemoveWindow(view_window) ;
 *  */
ZMapViewWindow zMapViewRemoveWindow(ZMapViewWindow view_window_in)
{
  ZMapViewWindow view_window = view_window_in ;
  ZMapView zmap_view ;

  zMapReturnValIfFail((view_window), view_window_in) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      if (g_list_length(zmap_view->window_list) > 1)
        {
          destroyWindow(zmap_view, view_window) ;
        }
    }

  return view_window ;
}



/*!
 * Get the views "xremote" widget, returns NULL if view is dying.
 *
 * @param                The ZMap View
 * @return               NULL or views xremote widget.
 *  */
GtkWidget *zMapViewGetXremote(ZMapView view)
{
  GtkWidget *xremote_widget = NULL ;

  if (view->state != ZMAPVIEW_DYING)
    xremote_widget = view->xremote_widget ;

  return xremote_widget ;
}




/* Force a redraw of all the windows in a view, may be required if it looks like
 * drawing has got out of whack due to an overloaded network etc etc. */
void zMapViewRedraw(ZMapViewWindow view_window)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapReturnIfFail(view) ;

  if (view->state == ZMAPVIEW_LOADED)
    {
      list_item = g_list_first(view->window_list) ;
      do
        {
          ZMapViewWindow view_window ;

          view_window = (ZMapViewWindow)(list_item->data) ;

          zMapWindowRedraw(view_window->window) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}


/* Reverse complement a view, this call will:
 *
 *    - leave the View window(s) displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - reverse complement the sequence context.
 *    - display the reversed context.
 *
 *  */
gboolean zMapViewReverseComplement(ZMapView zmap_view)
{
  gboolean result = FALSE ;


//  if (zmap_view->state == ZMAPVIEW_LOADED)
// data is processed only when idle so this should be safe
  if(zmap_view->features)
    {
      GList* list_item ;

      zmapViewBusy(zmap_view, TRUE) ;

      zMapLogTime(TIMER_REVCOMP,TIMER_CLEAR,0,"Revcomp");
      zMapLogTime(TIMER_EXPOSE,TIMER_CLEAR,0,"Revcomp");
      zMapLogTime(TIMER_UPDATE,TIMER_CLEAR,0,"Revcomp");
      zMapLogTime(TIMER_DRAW,TIMER_CLEAR,0,"Revcomp");
      zMapLogTime(TIMER_DRAW_CONTEXT,TIMER_CLEAR,0,"Revcomp");
      zMapLogTime(TIMER_SETVIS,TIMER_CLEAR,0,"Revcomp");

      zmapViewResetWindows(zmap_view, TRUE);

        zMapWindowNavigatorReset(zmap_view->navigator_window);

      zMapLogTime(TIMER_REVCOMP,TIMER_START,0,"Context");

      /* Call the feature code that will do the revcomp. */
      zMapFeatureContextReverseComplement(zmap_view->features) ;

      zMapLogTime(TIMER_REVCOMP,TIMER_STOP,0,"Context");

      /* Set our record of reverse complementing. */
      const gboolean value = zMapViewGetRevCompStatus(zmap_view) ;
      zMapViewSetFlag(zmap_view, ZMAPFLAG_REVCOMPED_FEATURES, !value) ;

      zMapWindowNavigatorSetStrand(zmap_view->navigator_window, zMapViewGetRevCompStatus(zmap_view));
      zMapWindowNavigatorDrawFeatures(zmap_view->navigator_window, zmap_view->features, zmap_view->context_map.styles);

      if((list_item = g_list_first(zmap_view->window_list)))
        {
          do
            {
              ZMapViewWindow view_window ;

              view_window = (ZMapViewWindow)(list_item->data) ;

              zMapWindowFeatureRedraw(view_window->window, zmap_view->features,TRUE) ;
            }
          while ((list_item = g_list_next(list_item))) ;
        }

      /* Not sure if we need to do this or not.... */
      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;

      zMapLogTime(TIMER_REVCOMP,TIMER_ELAPSED,0,"");
      zmapViewBusy(zmap_view, FALSE);

      result = TRUE ;
    }

    if (zmap_view->feature_cache)
      {
        g_hash_table_destroy(zmap_view->feature_cache) ;
        zmap_view->feature_cache = g_hash_table_new(NULL, NULL) ;
      }

  return result ;
}

/* Return which strand we are showing viz-a-viz reverse complementing. */
gboolean zMapViewGetRevCompStatus(ZMapView zmap_view)
{
  return zMapViewGetFlag(zmap_view, ZMAPFLAG_REVCOMPED_FEATURES) ;
}



/* Reset an existing view, this call will:
 *
 *    - leave the View window(s) displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - Free all ZMap window data that was specific to the view being loaded.
 *    - Kill the existing server thread(s).
 *
 * After this call the ZMap will be ready for the user to try again with the existing
 * machine/port/sequence or to enter data for a new sequence etc.
 *
 *  */
gboolean zMapViewReset(ZMapView zmap_view)
{
  gboolean result = FALSE ;

  if (zmap_view->state == ZMAPVIEW_CONNECTING || zmap_view->state == ZMAPVIEW_CONNECTED
      || zmap_view->state == ZMAPVIEW_LOADING || zmap_view->state == ZMAPVIEW_LOADED)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      zmap_view->state = ZMAPVIEW_RESETTING ;

      /* Reset all the windows to blank. */
      resetWindows(zmap_view) ;

      /* We need to destroy the existing thread connection and wait until user loads a new
         sequence. */
      killConnections(zmap_view) ;

      result = TRUE ;
    }

  return result ;
}


/* User passes in a view and we return the first window from our window list,
 * returns NULL if view has no windows. */
ZMapViewWindow zMapViewGetDefaultViewWindow(ZMapView view)
{
  ZMapViewWindow view_window = NULL ;

  if (view->window_list)
    {
      view_window = (ZMapViewWindow)(view->window_list->data) ;
    }

  return view_window ;
}



/*
 * If view_window is NULL all windows are zoomed.
 *
 *
 *  */
void zMapViewZoom(ZMapView zmap_view, ZMapViewWindow view_window, double zoom)
{
  if (zmap_view->state == ZMAPVIEW_LOADED)
    {
      if (view_window)
        zMapWindowZoom(zMapViewGetWindow(view_window), zoom) ;
      else
        {
          GList* list_item ;

          list_item = g_list_first(zmap_view->window_list) ;

          do
            {
              ZMapViewWindow view_window ;

              view_window = (ZMapViewWindow)(list_item->data) ;

              zMapWindowZoom(view_window->window, zoom) ;
            }
          while ((list_item = g_list_next(list_item))) ;
        }
    }

  return ;
}

/*
 *    A set of accessor functions.
 */

char *zMapViewGetSequence(ZMapView zmap_view)
{
  char *sequence = NULL ;

  if (zmap_view->state != ZMAPVIEW_DYING)
  {
    sequence = zMapViewGetSequenceName(zmap_view->view_sequence);
  }

  return sequence ;
}

char *zMapViewGetSequenceName(ZMapFeatureSequenceMap sequence_map)
{
  char *sequence = NULL ;

  if(!g_strstr_len(sequence_map->sequence,-1,"_"))    /* sequencename_start-end format? */
    {
      sequence = g_strdup_printf("%s_%d-%d", sequence_map->sequence, sequence_map->start, sequence_map->end);
    }
    else
    {
      sequence = g_strdup(sequence_map->sequence);
    }

  return sequence ;
}

ZMapFeatureSequenceMap zMapViewGetSequenceMap(ZMapView zmap_view)
{
  if (zmap_view->state != ZMAPVIEW_DYING)
  {
    return zmap_view->view_sequence;
  }

  return NULL ;
}


void zMapViewGetSourceNameTitle(ZMapView zmap_view, char **name, char **title)
{

  if (zmap_view->view_db_name)
    *name = zmap_view->view_db_name ;

  if (zmap_view->view_db_title)
    *title = zmap_view->view_db_title ;

  return ;
}


ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view)
{
  ZMapFeatureContext features = NULL ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    features = zmap_view->features ;

  return features ;
}

ZMapStyleTree *zMapViewGetStyles(ZMapViewWindow view_window)
{
  ZMapStyleTree *styles = NULL ;
  ZMapView view = zMapViewGetView(view_window);

  if (view->state != ZMAPVIEW_DYING)
    styles = &view->context_map.styles ;

  return styles;
}

gboolean zMapViewGetFeaturesSpan(ZMapView zmap_view, int *start, int *end)
{
  gboolean result = FALSE ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    {
      if(zmap_view->view_sequence->end)
        {
          *start = zmap_view->view_sequence->start;
          *end = zmap_view->view_sequence->end;
        }
      else if(zmap_view->features)
        {
          *start = zmap_view->features->master_align->sequence_span.x1 ;
          *end = zmap_view->features->master_align->sequence_span.x2 ;
        }

#if MH17_DEBUG
zMapLogWarning("view span: seq %d-%d,par %d-%d, align %d->%d",
      zmap_view->view_sequence->start,zmap_view->view_sequence->end,
      zmap_view->features->parent_span.x1,zmap_view->features->parent_span.x2,
      zmap_view->features->master_align->sequence_span.x1,zmap_view->features->master_align->sequence_span.x2);
#endif
      result = TRUE ;
   }

  return result ;
}


/* N.B. we don't exclude ZMAPVIEW_DYING because caller may want to know that ! */
ZMapViewState zMapViewGetStatus(ZMapView zmap_view)
{
  return zmap_view->state ;
}

/* auto define of function to return view state as a string, see zmapEnum.h. */
ZMAP_ENUM_TO_SHORT_TEXT_FUNC(zMapView2Str, ZMapViewState, VIEW_STATE_LIST) ;



/* Get status of view and of feature set loading in view.
 *
 * A summary string is returned but if state is
 *
 * ZMAPVIEW_LOADED || state == ZMAPVIEW_UPDATING || state == ZMAPVIEW_LOADED
 *
 * then lists of sources loading or that have failed to load will be returned
 * if loading_sources_out and/or failed_sources_out are non-NULL.
 *
 *  */
char *zMapViewGetLoadStatusStr(ZMapView view,
                               char **loading_sources_out, char **empty_sources_out, char **failed_sources_out)
{
  char *load_state_str = NULL ;
  ZMapViewState state = view->state ;
  char *state_str ;

  state_str = (char *)zMapView2Str(state) ;

  if (state == ZMAPVIEW_LOADING || state == ZMAPVIEW_UPDATING || state == ZMAPVIEW_LOADED)
    {
      if (state == ZMAPVIEW_LOADING || state == ZMAPVIEW_UPDATING)
        load_state_str = g_strdup_printf("%s (%d to go) (%d empty) (%d failed)", state_str,
                                         g_list_length(view->sources_loading),
                                         g_list_length(view->sources_empty),
                                         g_list_length(view->sources_failed)) ;
      else
        load_state_str = g_strdup_printf("%s (%d empty) (%d failed)", state_str,
                                         g_list_length(view->sources_empty),
                                         g_list_length(view->sources_failed)) ;

      if (loading_sources_out)
        *loading_sources_out = zMap_g_list_quark_to_string(view->sources_loading, NULL) ;

      if (empty_sources_out)
        *empty_sources_out = zMap_g_list_quark_to_string(view->sources_empty, NULL) ;

      if (failed_sources_out)
        *failed_sources_out = zMap_g_list_quark_to_string(view->sources_failed, NULL) ;
    }
  else
    {
      load_state_str = g_strdup(state_str) ;
    }


  return load_state_str ;
}


ZMapFeatureContextMap zMapViewGetContextMap(ZMapView view)
{
  ZMapFeatureContextMap context_map = NULL ;

  zMapReturnValIfFail((view), context_map) ;

  context_map = &view->context_map ;

  return context_map ;
}

ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window)
{
  ZMapWindow window = NULL ;

  zMapReturnValIfFail((view_window), window) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    window = view_window->window ;

  return window ;
}

ZMapFeatureContext zMapViewGetContext(ZMapViewWindow view_window)
{
  ZMapFeatureContext context = NULL ;

  zMapReturnValIfFail((view_window), context) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    context = view_window->parent_view->features ;

  return context ;
}

ZMapWindowNavigator zMapViewGetNavigator(ZMapView view)
{
  ZMapWindowNavigator navigator = NULL ;

  zMapReturnValIfFail((view), navigator) ;

  if (view->state != ZMAPVIEW_DYING)
    navigator = view->navigator_window ;

  return navigator ;
}


GList *zMapViewGetWindowList(ZMapViewWindow view_window)
{
  GList *window_list = NULL ;

  zMapReturnValIfFail((view_window), window_list) ;

  window_list = view_window->parent_view->window_list ;

  return window_list ;
}


void zMapViewSetWindowList(ZMapViewWindow view_window, GList *glist)
{
  zMapReturnIfFail((view_window && glist)) ;

  view_window->parent_view->window_list = glist;

  return;
}


ZMapView zMapViewGetView(ZMapViewWindow view_window)
{
  ZMapView view = NULL ;

  if (view_window && view_window->parent_view && view_window->parent_view->state != ZMAPVIEW_DYING)
    view = view_window->parent_view ;

  return view ;
}

void zMapViewReadConfigBuffer(ZMapView zmap_view, char *buffer)
{
  /* designed to add extra config bits to an already created view... */
  /* This is probably a bit of a hack, why can't we make the zMapViewConnect do it?  */
#ifdef NOT_REQUIRED_ATM
  getSequenceServers(zmap_view, buffer);
#endif /* NOT_REQUIRED_ATM */
  return ;
}


/* Check if there have been any changes of various types that have not been
 * saved and if so ask the user what they want to do. Returns true to continue 
 * or false to cancel. */
gboolean zMapViewCheckIfUnsaved(ZMapView zmap_view)
{
  gboolean result = TRUE ;
  GtkResponseType response = GTK_RESPONSE_NONE ;

  if (response != GTK_RESPONSE_CANCEL)
    response = checkForUnsavedFeatures(zmap_view) ;

  if (response != GTK_RESPONSE_CANCEL)
    response = checkForUnsavedAnnotation(zmap_view) ;

//  if (response != GTK_RESPONSE_CANCEL)
//    response = checkForUnsavedConfig(zmap_view, ZMAPVIEW_EXPORT_CONFIG) ;
//
//  if (response != GTK_RESPONSE_CANCEL)
//    response = checkForUnsavedConfig(zmap_view, ZMAPVIEW_EXPORT_STYLES) ;

  if (response == GTK_RESPONSE_CANCEL)
    result = FALSE ;

  return result ;
}


/* Called to kill a view and get the associated threads killed, note that
 * this call just signals everything to die, its the checkConnections() routine
 * that really clears up and when everything has died signals the caller via the
 * callback routine that they supplied when the view was created.
 *
 */
void zMapViewDestroy(ZMapView zmap_view)
{

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      /* All states have GUI components which need to be destroyed. */
      killGUI(zmap_view) ;

      if (zmap_view->state <= ZMAPVIEW_MAPPED)
        {
          /* For init we simply need to signal to our parent layer that we have died,
           * we will then be cleaned up immediately. */

          zmap_view->state = ZMAPVIEW_DYING ;
        }
      else
        {
          /* for other states there are threads to kill so they must be cleaned
           * up asynchronously. */

          if (zmap_view->state != ZMAPVIEW_RESETTING)
            {
              /* If we are resetting then the connections have already being killed. */
              killConnections(zmap_view) ;
            }

          /* Must set this as this will prevent any further interaction with the ZMap as
           * a result of both the ZMap window and the threads dying asynchronously.  */
          zmap_view->state = ZMAPVIEW_DYING ;
        }
    }

  return ;
}




void zMapViewShowLoadStatus(ZMapView view)
{
  if (view->state < ZMAPVIEW_LOADING)
    view->state = ZMAPVIEW_LOADING ;

  if (view->state > ZMAPVIEW_LOADING)
    view->state = ZMAPVIEW_UPDATING ;

  zmapViewBusy(view, TRUE) ;     // gets unset when all step lists finish

  (*(view_cbs_G->state_change))(view, view->app_data, NULL) ;

  return ;
}



/*
 *                      Package external routines
 */




void zmapViewFeatureDump(ZMapViewWindow view_window, char *file)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapFeatureDump(view_window->parent_view->features, file) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  printf("reimplement.......\n") ;

  return;
}




// I'LL BET A LOT OF MONEY THIS DOESN'T REALLY WORK....   
/*!
 * Erases the supplied context from the view's context and instructs
 * the window to delete the old features.
 *
 * @param                The ZMap View
 * @param                The Context to erase.  Those features which
 *                       match will be removed from this context and
 *                       the view's own context. They will also be
 *                       removed from the display windows. Those that
 *                       don't match will be left in this context.
 * @return               void
 *************************************************** */
void zmapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout)
{
  if (replace_me->state != ZMAPVIEW_DYING)
    {
      /* should replace_me be a view or a view_window???? */
      eraseAndUndrawContext(replace_me, context_inout);
    }

  return;
}

/*!
 * Merges the supplied context with the view's context.
 *
 * @param                The ZMap View
 * @param                The Context to merge in.  This will be emptied
 *                       but needs destroying...
 * @return               The diff context.  This needs destroying.
 *************************************************** */
ZMapFeatureContext zmapViewMergeInContext(ZMapView view, ZMapFeatureContext context,
                                          ZMapFeatureContextMergeStats *merge_stats_out)
{

  /* called from zmapViewRemoteReceive.c, no styles are available. */
  /* we do not expect masked features to be affected and do not process these */

  if (view->state != ZMAPVIEW_DYING)
    {
      ZMapFeatureContextMergeCode result = ZMAPFEATURE_CONTEXT_ERROR ;

      result = zmapJustMergeContext(view, &context, merge_stats_out, NULL, TRUE, FALSE) ;

      if (result == ZMAPFEATURE_CONTEXT_OK)
        zMapLogMessage("%s", "Context merge successful.") ;
      else if (result == ZMAPFEATURE_CONTEXT_NONE)
        zMapLogWarning("%s", "Context merge failed because no new features found in new context.") ;
      else
        zMapLogCritical("%s", "Context merge failed, serious error.") ;
    }

  return context ;
}

/*!
 * Instructs the view to draw the diff context. The drawing will
 * happen sometime in the future, so we NULL the diff_context!
 *
 * @param               The ZMap View
 * @param               The Context to draw...
 *
 * @return              Boolean to notify whether the context was
 *                      free'd and now == NULL, FALSE only if
 *                      diff_context is the same context as view->features
 *************************************************** */
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context, ZMapFeature highlight_feature)
{
  gboolean context_freed = TRUE;

  /* called from zmapViewRemoteReceive.c, no styles are available. */

  if (view->state != ZMAPVIEW_DYING)
    {
      if (view->features == *diff_context)
        context_freed = FALSE ;

      zmapJustDrawContext(view, *diff_context, NULL, highlight_feature, NULL) ;
    }
  else
    {
      context_freed = TRUE;
      zMapFeatureContextDestroy(*diff_context, context_freed) ;
    }

  if (context_freed)
    *diff_context = NULL ;

  return context_freed ;
}


ZMapFeatureContext zmapViewCopyContextAll(ZMapFeatureContext context,
                                          ZMapFeature feature,
                                          ZMapFeatureSet feature_set,
                                          GList **feature_list,
                                          ZMapFeature *feature_copy_out)
{
  ZMapFeatureContext context_copy = NULL ;

  g_return_val_if_fail(context && context->master_align && feature && feature_set, NULL) ;

  context_copy = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)context) ;
  context_copy->req_feature_set_names = g_list_append(context_copy->req_feature_set_names, GINT_TO_POINTER(feature_set->unique_id)) ;

  ZMapFeatureAlignment align_copy = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)context->master_align) ;
  zMapFeatureContextAddAlignment(context_copy, align_copy, FALSE) ;

  ZMapFeatureBlock block = (ZMapFeatureBlock)zMap_g_hash_table_nth(context->master_align->blocks, 0) ;
  g_return_val_if_fail(block, NULL) ;

  ZMapFeatureBlock block_copy = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)block) ;
  zMapFeatureAlignmentAddBlock(align_copy, block_copy) ;

  ZMapFeatureSet feature_set_copy = (ZMapFeatureSet)zMapFeatureAnyCopy((ZMapFeatureAny)feature_set) ;
  zMapFeatureBlockAddFeatureSet(block_copy, feature_set_copy) ;

  ZMapFeature feature_copy = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)feature) ;
  zMapFeatureSetAddFeature(feature_set_copy, feature_copy) ;

  if (feature_list)
    *feature_list = g_list_append(*feature_list, feature_copy) ;

  if (feature_copy_out)
    *feature_copy_out = feature_copy ;

  return context_copy ;
}

void zmapViewMergeNewFeature(ZMapView view, ZMapFeature feature, ZMapFeatureSet feature_set)
{
  zMapReturnIfFail(view && view->features && feature && feature_set);

  ZMapFeatureContext context = view->features ;

  GList *feature_list = NULL ;
  ZMapFeature feature_copy = NULL ;
  ZMapFeatureContext context_copy = zmapViewCopyContextAll(context, feature, feature_set, &feature_list, &feature_copy) ;

  if (context_copy && feature_list)
    zmapViewMergeNewFeatures(view, &context_copy, NULL, &feature_list) ;

  if (context_copy && feature_copy)
    zmapViewDrawDiffContext(view, &context_copy, feature_copy) ;

  if (context_copy)
    zMapFeatureContextDestroy(context_copy, TRUE) ;
}

gboolean zmapViewMergeNewFeatures(ZMapView view,
                                  ZMapFeatureContext *context, ZMapFeatureContextMergeStats *merge_stats_out,
                                  GList **feature_list)
{
  gboolean result = FALSE ;

  *context = zmapViewMergeInContext(view, *context, merge_stats_out) ;

  if (*context)
    {
      result = TRUE ;

      zMapFeatureContextExecute((ZMapFeatureAny)(*context),
                                ZMAPFEATURE_STRUCT_FEATURE,
                                delete_from_list,
                                feature_list);

      zMapFeatureContextExecute((ZMapFeatureAny)(view->features),
                                ZMAPFEATURE_STRUCT_FEATURE,
                                mark_matching_invalid,
                                feature_list);
    }

  return result ;
}

void zmapViewEraseFeatures(ZMapView view, ZMapFeatureContext context, GList **feature_list)
{
  /* If any of the features are used in the scratch column they must be removed because they'll
   * become invalid */
  zmapViewScratchRemoveFeatures(view, *feature_list) ;

  zmapViewEraseFromContext(view, context);

  zMapFeatureContextExecute((ZMapFeatureAny)(context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            mark_matching_invalid,
                            feature_list);

  return ;
}






/* Reset the state for all windows in this view */
void zmapViewResetWindows(ZMapView zmap_view, gboolean revcomp)
{
  GList* list_item ;

  /* First, loop through and save the state for ALL windows.
   * We must do this BEFORE WE RESET ANY WINDOWS because
   * windows can share the same vadjustment (when scrolling
   * is locked) and we don't want to reset the scroll position
   * until we've saved it for all windows. */
  if((list_item = g_list_first(zmap_view->window_list)))
    {
      do
        {
          ZMapViewWindow view_window ;

          view_window = (ZMapViewWindow)(list_item->data) ;

          zMapWindowFeatureSaveState(view_window->window, revcomp) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }

  /* Now reset the windows */
  if((list_item = g_list_first(zmap_view->window_list)))
    {
      do
        {
          ZMapViewWindow view_window ;

          view_window = (ZMapViewWindow)(list_item->data) ;

          zMapWindowFeatureReset(view_window->window, revcomp) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }
}



ZMapFeatureContextMergeCode zmapJustMergeContext(ZMapView view, ZMapFeatureContext *context_inout,
                                             ZMapFeatureContextMergeStats *merge_stats_out,
                                             GList **masked,
                                             gboolean request_as_columns, gboolean revcomp_if_needed)
{
  ZMapFeatureContextMergeCode result = ZMAPFEATURE_CONTEXT_ERROR ;
  ZMapFeatureContext new_features, diff_context = NULL ;
  GList *featureset_names = NULL;
  GList *l;


  new_features = *context_inout ;

  //  printf("just Merge new = %s\n",zMapFeatureContextGetDNAStatus(new_features) ? "yes" : "non");

  zMapStartTimer("Merge Context","") ;

  if(!view->features)
    {
      /* we need a context with a master_align with a block, all with valid sequence coordinates */
      /* this is all back to front, we only know what we requested when the answer comes back */
      /* and we need to have asked the same qeusrtion 3 times */
      ZMapFeatureBlock block = (ZMapFeatureBlock)zMap_g_hash_table_nth(new_features->master_align->blocks, 0) ;

      view->features = zMapFeatureContextCopyWithParents((ZMapFeatureAny) block) ;
    }


  if(!view->view_sequence->end)
    {
      view->view_sequence->start = new_features->master_align->sequence_span.x1;
      view->view_sequence->end   = new_features->master_align->sequence_span.x2;
    }


  /* When coming from xremote we don't need to do this. */
  if (revcomp_if_needed && zMapViewGetRevCompStatus(view))
    {
      zMapFeatureContextReverseComplement(new_features);
    }


  /* we need a list of requested featureset names, which is different from those returned
   * these names are user compatable (not normalised)
   */
  //printf("justMerge req=%d, %d featuresets\n", request_as_columns,g_list_length(new_features->req_feature_set_names));

  if (request_as_columns)                                   /* ie came from ACEDB */
    {
      ZMapFeatureColumn column = NULL ;

      l = new_features->req_feature_set_names;  /* column names as quarks, not normalised */
      while(l)
        {
          GQuark set_id = GPOINTER_TO_UINT(l->data);
          char *set_name = (char *) g_quark_to_string(set_id);
          set_id = zMapFeatureSetCreateID(set_name);

          std::map<GQuark, ZMapFeatureColumn>::iterator col_iter = view->context_map.columns->find(set_id) ;

          if (col_iter != view->context_map.columns->end())
            column = col_iter->second ;

          if (!column)
            {
              ZMapFeatureSetDesc set_desc ;

              /* AGGGHHHHHHHHH, SOMETIMES WE DON'T FIND IT HERE AS WELL.... */

              /* If merge request came directly from otterlace then need to look up feature set
               * in column/featureset hash. */
              if ((set_desc = (ZMapFeatureSetDesc)g_hash_table_lookup(view->context_map.featureset_2_column, GUINT_TO_POINTER(set_id))))
                {
                  col_iter = view->context_map.columns->find(set_desc->column_id) ;

                  if (col_iter != view->context_map.columns->end())
                    column = col_iter->second ;
                }
            }


          /* the column featureset lists have been assembled by this point,
             could not do this at the request stage */

          if(column)
            {                                    /* more effcient this way round? */
              featureset_names = g_list_concat(column->featuresets_names,featureset_names);
            }
          else  /* should not happen */
            {
              /* this is a not configured error */
              zMapLogWarning("merge: no column configured for %s in ACEDB",set_name);
            }

          l = l->next;
          /* as this list is to be replaced it would be logical to delete the links
           * but it is also referred to by connection_data
           */

        }
      new_features->req_feature_set_names = featureset_names;

    }
  else
    {
      /* not sure if these lists need copying, but a small memory leak is better than a crash due to a double free */
      if(!new_features->req_feature_set_names)
        new_features->req_feature_set_names = g_list_copy(new_features->src_feature_set_names);

      featureset_names = new_features->req_feature_set_names;

      /* need to add column_2_style and column style table; tediously we need the featureset structs to do this */
      zMapFeatureContextExecute((ZMapFeatureAny) new_features,
                                ZMAPFEATURE_STRUCT_FEATURESET,
                                add_default_styles,
                                (gpointer) view);
    }

  if (0)
    {
      char *x = g_strdup_printf("zmapJustMergeContext req=%d, %d featuresets after processing\n",
                                request_as_columns, g_list_length(new_features->req_feature_set_names));
      GList *l;
      for(l = new_features->req_feature_set_names;l;l = l->next)
        {
          char *y = (char *) g_quark_to_string(GPOINTER_TO_UINT(l->data));
          x = g_strconcat(x," ",y,NULL);
        }
      //      zMapLogWarning(x,"");
      printf("%s\n",x);

    }


  /* Merge the new features !! */
  result = zMapFeatureContextMerge(&(view->features), &new_features, &diff_context,
                                   merge_stats_out, featureset_names) ;


  //  printf("just Merge view = %s\n",zMapFeatureContextGetDNAStatus(view->features) ? "yes" : "non");
  //  printf("just Merge diff = %s\n",zMapFeatureContextGetDNAStatus(diff_context) ? "yes" : "non");


  {
    gboolean debug = FALSE ;
    GError *err = NULL;

    if (debug)
      {
        zMapFeatureDumpToFileName(diff_context,"features.txt","(justMerge) diff context:\n", NULL, &err) ;
        zMapFeatureDumpToFileName(view->features,"features.txt","(justMerge) view->Features:\n", NULL, &err) ;
      }

  }


  if (result == ZMAPFEATURE_CONTEXT_OK)
    {
      /*      zMapLogMessage("%s", "Context merge succeeded.") ;*/
      if(0)
        {
          char *x = g_strdup_printf("justMerge req=%d, diff has %d featuresets after merging: ", request_as_columns,g_list_length(diff_context->src_feature_set_names));
          GList *l;
          for(l = diff_context->src_feature_set_names;l;l = l->next)
            {
              char *y = (char *) g_quark_to_string(GPOINTER_TO_UINT(l->data));
              x = g_strconcat(x,",",y,NULL);
            }
          zMapLogWarning(x,"");
          printf("%s\n",x);
        }

      /* ensure transcripts have exons in fwd strand order
       * needed for CanvasTranscript... ACEDB did this but pipe scripts return exons in transcript (strand) order
       */
      zMapViewSortExons(diff_context);


      /* collpase short reads if configured
       * NOTE this is simpler than EST masking as we simply don't display the collapsed features
       * if it is thought necessary to change this then follow the example of EST masking code
       * you will need to head into window code via just DrawContext()
       */
      zMapViewCollapseFeatureSets(view,diff_context);

      // mask ESTs with mRNAs if configured
      l = zMapViewMaskFeatureSets(view, diff_context->src_feature_set_names);

      if(masked)
        *masked = l;
    }

  zMapStopTimer("Merge Context","") ;

  /* Return the diff_context which is the just the new features (NULL if merge fails). */
  *context_inout = diff_context ;

  return result ;
}



void zmapJustDrawContext(ZMapView view, ZMapFeatureContext diff_context,
                         GList *masked, ZMapFeature highlight_feature,
                         ZMapConnectionData connect_data)
{
  LoadFeaturesData loaded_features = NULL ;

  /* THIS CAN GO ONCE IT'S WORKING.... */
  /* we need to tell the user what features were loaded but this struct needs to
   * persist as the information may be passed _after_ the connection has gone. */
  if (connect_data && connect_data->loaded_features)
    {
      loaded_features = connect_data->loaded_features ;
      zMapLogMessage("copied pointer of ConnectData LoadFeaturesDataStruct"
                     " to pass to displayDataWindows(): %p -> %p",
                     connect_data->loaded_features, loaded_features) ;
    }

  /* Signal the ZMap that there is work to be done. */
  displayDataWindows(view, view->features, diff_context, 
                     loaded_features, FALSE, masked, NULL, FALSE, TRUE) ;

  /* Not sure about the timing of the next bit. */

  /* We have to redraw the whole navigator here.  This is a bit of
   * a pain, but it's due to the scaling we do to make the rest of
   * the navigator work.  If the length of the sequence changes the
   * all the previously drawn features need to move.  It also
   * negates the need to keep state as to the length of the sequence,
   * the number of times the scale bar has been drawn, etc... */
  zMapWindowNavigatorReset(view->navigator_window); /* So reset */
  zMapWindowNavigatorSetStrand(view->navigator_window, zMapViewGetRevCompStatus(view));
  /* and draw with _all_ the view's features. */
  zMapWindowNavigatorDrawFeatures(view->navigator_window, view->features, view->context_map.styles);

  /* signal our caller that we have data. */
  (*(view_cbs_G->load_data))(view, view->app_data, NULL) ;

  return ;
}





/*
 *                      Internal routines
 */


static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout)
{
  ZMapFeatureContext diff_context = NULL;

  if(!zMapFeatureContextErase(&(view->features), context_inout, &diff_context))
    {
      zMapLogCritical("%s", "Cannot erase feature data from...") ;
    }
  else
    {
      displayDataWindows(view, view->features, diff_context, NULL, TRUE, NULL, NULL, FALSE, TRUE) ;

      zMapFeatureContextDestroy(diff_context, TRUE) ;
    }

  return ;
}



/* Merge the list of columns into each window's list of feature_set_names. */
static void viewWindowsMergeColumns(ZMapView zmap_view)
{
  if (zmap_view && zmap_view->columns_set)
    {
      /* MH17:
       * due to an oversight I lost this ordering when converting columns to a hash table from a list
       * the columns struct contains the order, and bump status too
       *
       * due to constraints w/ old config we need to give the window a list of column name quarks in order
       */
      GList *columns = zmap_view->context_map.getOrderedColumnsGListIDs() ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_list_foreach(zmap_view->window_list, invoke_merge_in_names, columns);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      zmapViewMergeColNames(zmap_view, columns) ;

      g_list_free(columns);
    }

}


static void viewSetUpStyles(ZMapView zmap_view, char *stylesfile)
{
  //
  // read styles from file
  // the idea is that we can create a new view with new styles without restarting ZMap
  // but as that involves re-requesting data there's little gain.
  // Maybe you could have views of two sequences and you want to change a style in one ?
  //
  // each server can have it's own styles file, but was always use the same for each
  // and ACE can provide it's own styles. w/otterlace we use that same styles file
  // w/ XACE they want thier original styles????
  // so we have an (optional) global file and cache this data in the view
  // servers would traditionally read the file each time, and merge it into the view data
  // which is then passsed back to the servers. No need to do this 40x
  //
  // if we define a global stylesfile and still want styles from ACE then we
  // set 'req_styles=true' in the server config
  //

  /* There are a number of predefined methods that we require so add these in as well
   * and the mapping for "feature set" -> style for these.
   */
  addPredefined(zmap_view->context_map.styles, &(zmap_view->context_map.column_2_styles)) ;

  if (stylesfile)
    {
      GHashTable * styles = NULL;

      if (zMapConfigIniGetStylesFromFile(zmap_view->view_sequence->config_file, NULL, stylesfile, &styles, NULL))
        {
          zMapStyleMergeStyles(zmap_view->context_map.styles, styles, ZMAPSTYLE_MERGE_MERGE) ;
        }
      else
        {
          zMapLogWarning("Could not read styles file \"%s\"", stylesfile) ;
        }
    }
}


static void viewSetUpPredefinedColumns(ZMapView zmap_view, ZMapFeatureSequenceMap sequence_map)
{
  /* add a strand separator featureset, we need it for the yellow stripe in the middle of the screen */
  /* it will cause a column of the same name to be created */
  /* real separator featuresets have diff names and will be added to the column */
  ZMapFeatureSet feature_set;
  GQuark style_id ;
  ZMapFeatureTypeStyle style ;
  ZMapSpan loaded;
  ZMapFeatureSequenceMap sequence = zmap_view->view_sequence;
  ZMapFeatureContext context;
  ZMapFeatureContextMergeStats merge_stats ;
  GList *dummy = NULL;
  ZMapFeatureBlock block ;
  ZMapFeatureContextMergeCode merge ;

  style_id = zMapStyleCreateID(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) ;

  if ((feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_STRAND_SEPARATOR, NULL)))
    {
      style = zmap_view->context_map.styles.find_style(style_id) ;

      zMapFeatureSetStyle(feature_set,style);

      loaded = g_new0(ZMapSpanStruct,1);        /* prevent silly log messages */
      loaded->x1 = sequence->start;
      loaded->x2 = sequence->end;
      feature_set->loaded = g_list_append(NULL,loaded);

    }

  context = zmapViewCreateContext(zmap_view, g_list_append(NULL, GUINT_TO_POINTER(style_id)),
                                  feature_set);        /* initialise to strand separator */

  block = (ZMapFeatureBlock)(feature_set->parent) ;
  zmapViewScratchInit(zmap_view, sequence_map, context, block) ;


  /* now merge and draw it */
  if ((merge = zmapJustMergeContext(zmap_view,
                                &context, &merge_stats,
                                &dummy, FALSE, TRUE))
      == ZMAPFEATURE_CONTEXT_OK)
    {
      zmapJustDrawContext(zmap_view, context, dummy, NULL, NULL) ;
    }
  else if (merge == ZMAPFEATURE_CONTEXT_NONE)
    {
      zMapLogWarning("%s", "Context merge failed because no new features found in new context.") ;
    }
  else
    {
      zMapLogCritical("%s", "Context merge failed, serious error.") ;
    }
}



/* Check if there have been changes to the Annotation column that have not been
 * saved and if so ask the user if they really want to quit. Returns OK to
 * continue quitting or Cancel to stop. Returns OK to quit, or Cancel. */
static GtkResponseType checkForUnsavedAnnotation(ZMapView zmap_view)
{
  GtkResponseType response = GTK_RESPONSE_OK ;

  if (zmap_view && zMapViewGetFlag(zmap_view, ZMAPFLAG_SAVE_SCRATCH))
    {
      GtkWindow *parent = NULL ;

      /* The responses for the 2 button arguments are ok or cancel. We could also offer
       * the option to Save here but that's complicated because we then also need to save
       * the new feature that is created to file - it's probably more intuitive for
       * the user to just cancel and then do the save manually */
      char *msg =
        g_strdup_printf("There are unsaved edits in the Annotation column for sequence '%s' - do you really want to quit?.",
                        zmap_view->view_sequence ? zmap_view->view_sequence->sequence : "<null>") ;

      gboolean result = zMapGUIMsgGetBoolFull(parent,
                                              ZMAP_MSG_WARNING,
                                              msg,
                                              GTK_STOCK_QUIT,
                                              GTK_STOCK_CANCEL) ;

      g_free(msg) ;

      if (!result)
        response = GTK_RESPONSE_CANCEL ;
    }

  return response ;
}


/* Check if there are new features that have not been saved and if so ask the
 * user if they really want to quit. Returns OK to quit, Cancel, or Close if we saved. */
static GtkResponseType checkForUnsavedFeatures(ZMapView zmap_view)
{
  GtkResponseType response = GTK_RESPONSE_OK ;

  if (zmap_view && zMapViewGetFlag(zmap_view, ZMAPFLAG_SAVE_FEATURES))
    {
      GtkWindow *parent = NULL ;

      /* The responses for the 3 button arguments are ok, cancel, close. */
      char *msg =
        g_strdup_printf("There are unsaved features for sequence '%s' - do you really want to quit?",
                        zmap_view->view_sequence ? zmap_view->view_sequence->sequence : "<null>") ;

      response = zMapGUIMsgGetSaveFull(parent,
                                       ZMAP_MSG_WARNING,
                                       msg,
                                       GTK_STOCK_QUIT,
                                       GTK_STOCK_CANCEL,
                                       GTK_STOCK_SAVE) ;

      g_free(msg) ;

      if (response == GTK_RESPONSE_CLOSE)
        {
          /* Loop through each window and save changes */
          GList *window_item = zmap_view->window_list ;
          GError *error = NULL ;

          for ( ; window_item ; window_item = window_item->next)
            {
              ZMapViewWindow view_window = (ZMapViewWindow)(window_item->data) ;
              ZMapWindow window = zMapViewGetWindow(view_window) ;

              /* If we've already set a save file then use that. (Don't let it default
               * to the input file though because the user hasn't explicitly asked
               * for a Save option rather than Save As...) */
              const char *file = zMapViewGetSaveFile(zmap_view, ZMAPVIEW_EXPORT_FEATURES, FALSE) ;
              char *filename = g_strdup(file) ;
              gboolean saved = zMapWindowExportFeatures(window, TRUE, FALSE, NULL, &filename, &error) ;

              if (!saved)
                {
                  /* There was a problem saving so don't continue quitting zmap */
                  response = GTK_RESPONSE_CANCEL ;

                  /* If error is null then the user cancelled the save so don't report
                   * an error */
                  if (error)
                    {
                      zMapWarning("Save failed: %s", error->message) ;
                      g_error_free(error) ;
                      error = NULL ;
                    }
                }
            }
        }
    }

  return response ;
}


/* Check if there are unsaved changes to config/styles and if so ask the
 * user if they really want to quit. Returns OK to quit, Cancel, or Close if we saved. */
//static GtkResponseType checkForUnsavedConfig(ZMapView zmap_view, ZMapViewExportType export_type)
//{
//  GtkResponseType response = GTK_RESPONSE_OK ;
//  GError *error = NULL ;
//
//  if (zmap_view && zMapViewGetFlag(zmap_view, ZMAPFLAG_SAVE_CONFIG))
//    {
//      GtkWindow *parent = NULL ;
//
//      /* The responses for the 3 button arguments are ok, cancel, close. */
//      char *msg = NULL ;
//
//      if (export_type == ZMAPVIEW_EXPORT_STYLES)
//        msg = g_strdup_printf("There are unsaved styles - do you really want to quit?") ;
//      else
//        msg = g_strdup_printf("There are unsaved changes to the configuration - do you really want to quit?") ;
//
//      response = zMapGUIMsgGetSaveFull(parent,
//                                       ZMAP_MSG_WARNING,
//                                       msg,
//                                       GTK_STOCK_QUIT,
//                                       GTK_STOCK_CANCEL,
//                                       GTK_STOCK_SAVE) ;
//
//      g_free(msg) ;
//
//      if (response == GTK_RESPONSE_CLOSE)
//        {
//          /* If we've already set a save file then use that. (Don't let it default
//           * to the input file though because the user hasn't explicitly asked
//           * for a Save option rather than Save As...) */
//          const char *file = zMapViewGetSaveFile(zmap_view, export_type, FALSE) ;
//          char *filename = g_strdup(file) ;
//          gboolean saved = zMapViewExportConfig(zmap_view, export_type, &filename, &error) ;
//
//          if (!saved)
//            {
//              /* There was a problem saving so don't continue quitting zmap */
//              response = GTK_RESPONSE_CANCEL ;
//
//              /* If error is null then the user cancelled the save so don't report
//               * an error */
//              if (error)
//                {
//                  zMapWarning("Save failed: %s", error->message) ;
//                  g_error_free(error) ;
//                  error = NULL ;
//                }
//            }
//        }
//    }
//
//  return response ;
//}




static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data)
{
  gint match = -1;
  ZMapFeatureAny a = (ZMapFeatureAny)list_data, b = (ZMapFeatureAny)user_data ;

  match = !(a->unique_id == b->unique_id);

  return match;
}


static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key,
                                                        gpointer data,
                                                        gpointer user_data,
                                                        char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  GList **glist = (GList **)user_data, *match;

  if (any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if ((match = g_list_find_custom(*glist, any, matching_unique_id)))
        {
          *glist = g_list_remove(*glist, match->data);
        }
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}


static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  GList **glist = (GList **)user_data, *match;

  if (any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if ((match = g_list_find_custom(*glist, any, matching_unique_id)))
        {
          any = (ZMapFeatureAny)(match->data);
          any->struct_type = ZMAPFEATURE_STRUCT_INVALID;
        }
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}



/* THE LOGIC IN THIS ROUTINE IS NOT CLEAR AND NOT WELL EXPLAINED...... */

/* read in rather a lot of stanzas and add the data to a few hash tables and lists
 *
 * This sets up:
 * view->context->map formerly as:
 *    view->columns
 *    view->featureset_2_column
 *    view->source_2_sourcedata
 *    view->context_map.column_2_styles
 *
 * This 'replaces' ACEDB data but if absent we should still use the ACE stuff, it all gets merged
 */
static void getIniData(ZMapView view, char *config_str, GList *req_sources)
{
  ZMapConfigIniContext context ;
  GHashTable *fset_col;
  GHashTable *fset_styles;
  GHashTable *gff_src;
  GHashTable *col_styles;
  GHashTable *gff_desc;
  GHashTable *gff_related;
  GHashTable *source_2_sourcedata;
  ZMapFeatureSource gff_source;
  ZMapFeatureSetDesc gffset;
  ZMapFeatureColumn column;
  GList *iter;
  char *str;
  gpointer key, value;
  GList *sources;

#if 0
  ZMapCmdLineArgsType arg;

  if(zMapCmdLineArgsValue(ZMAPARG_SERIAL,&arg))
    view->serial_load = arg.b;

  //view->serial_load = TRUE;
  zMapLogWarning("serial load = %d",view->serial_load);
#endif


  /*
   * This gets very fiddly
   * - lots of the names may have whitespace, which we normalise
   * - most of the rest of the code lowercases the names so we have to
   *   yet we still need to display the text as capitlaised if that's what people put in
   * See zMapConfigIniGetQQHash()...
   *
   */

  zMapLogTime(TIMER_LOAD,TIMER_CLEAR,0,"View init");

  if ((context = zMapConfigIniContextProvide(view->view_sequence->config_file, ZMAPCONFIG_FILE_NONE)))
    {
      if(config_str)
        zMapConfigIniContextIncludeBuffer(context, config_str);

      /* view global thread fail popup switch */
      zMapConfigIniContextGetBoolean(context,
                                     ZMAPSTANZA_APP_CONFIG,
                                     ZMAPSTANZA_APP_CONFIG,
                                     ZMAPSTANZA_APP_REPORT_THREAD, &view->thread_fail_silent);

      if (zMapConfigIniContextGetString(context,
                                        ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_NAVIGATOR_SETS,&str))
        {
          view->navigator_set_names = zMapConfigString2QuarkGList(str,FALSE);

          if (view->navigator_window)
            zMapWindowNavigatorMergeInFeatureSetNames(view->navigator_window, view->navigator_set_names);
        }

      /*-------------------------------------
       * the dataset
       *-------------------------------------
       */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                        ZMAPSTANZA_APP_DATASET, &str))
        {
          view->view_sequence->dataset = str;
        }

      /*-------------------------------------
       * the display columns in L -> R order
       *-------------------------------------
       */
      view->context_map.columns = zMapConfigIniGetColumns(context);

      if (view->context_map.columns->size() > 0)
        view->columns_set = TRUE;

      /*-------------------------------------
       * column groups
       *-------------------------------------
       */
      view->context_map.column_groups = zMapConfigIniGetColumnGroups(context);

      {
        /*-------------------------------------------------------------------------------
         * featureset to column mapping, with column descriptions added to GFFSet struct
         *-------------------------------------------------------------------------------
         */

        /* default 1-1 mapping : add for pipe/file and Acedb servers, otherwise get from config
         * file. */
        fset_col = g_hash_table_new(NULL,NULL);

        for(sources = req_sources; sources ; sources = sources->next)
          {
            GList *featuresets;
            ZMapConfigSource src;

            src = (ZMapConfigSource)sources->data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* THIS CODE CAUSES A PROBLEM IN THE FEATURESET/STYLE MAPPING IF ACEDB IS INCLUDED,
             * LEAVE THIS HERE UNTIL FURTHER TESTING DONE WITH WORM STUFF.... */
            if (g_ascii_strncasecmp(src->url,"pipe", 4) != 0
                && g_ascii_strncasecmp(src->url,"file", 4) != 0
                && g_ascii_strncasecmp(src->url,"acedb", 5) != 0)
              continue;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            if (g_ascii_strncasecmp(src->url,"pipe", 4) != 0
                && g_ascii_strncasecmp(src->url,"ensembl", 7) != 0
                && g_ascii_strncasecmp(src->url,"file", 4) != 0)
              continue;

            featuresets = zMapConfigString2QuarkGList(src->featuresets,FALSE) ;

            // MH17: need to add server name as default featureset
            //  -> it doesn't have one due to GLib config file rubbish
            //            if(!featuresets)
            //                    featuresets = g_list_add(featuresets,src->name);
            while(featuresets)
              {
                GQuark fset,col;

                gffset = g_new0(ZMapFeatureSetDescStruct,1);

                col = GPOINTER_TO_UINT(featuresets->data);
                fset = zMapFeatureSetCreateID((char *) g_quark_to_string(col));

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                printf("featureset: %s (%s)\n", g_quark_to_string(col), g_quark_to_string(fset)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                gffset->column_id = fset;
                gffset->column_ID = col;
                gffset->feature_src_ID = col;
                gffset->feature_set_text = g_strdup(g_quark_to_string(col));

                /* replace in case we get one twice */
                g_hash_table_replace(fset_col,GUINT_TO_POINTER(fset),gffset);

                featuresets = g_list_delete_link(featuresets,featuresets);
              }
          }

        
        if (view->context_map.columns)
          fset_col = zMapConfigIniGetFeatureset2Column(context, fset_col, *view->context_map.columns) ;

        if (g_hash_table_size(fset_col))
          view->context_map.featureset_2_column = fset_col ;
        else
          g_hash_table_destroy(fset_col) ;

        fset_col = NULL ;


        /*-----------------------------------------------------------------------
         * source_2_sourcedata:featureset -> (sourceid, styleid, description)
         *-----------------------------------------------------------------------
         */

        source_2_sourcedata = g_hash_table_new(NULL,NULL);

        // source id may not be a style but it's name still gets normalised
        gff_src   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_GFF_SOURCE_CONFIG,QQ_QUARK);
        fset_styles = zMapConfigIniGetQQHash(context,ZMAPSTANZA_FEATURESET_STYLE_CONFIG,QQ_STYLE);
        gff_desc  = zMapConfigIniGetQQHash(context,ZMAPSTANZA_GFF_DESCRIPTION_CONFIG,QQ_QUARK);
        /* column related to featureset: get unique ids */
        gff_related   = zMapConfigIniGetQQHash(context,ZMAPSTANZA_GFF_RELATED_CONFIG,QQ_STYLE);

        gff_source = NULL;


        /* YES BUT WHAT IF THERE IS NO featureset_2_column ??????????? duh..... */
        // it's an input and output for servers, must provide for pipes
        // see featureset_2_column as above
        if (view->context_map.featureset_2_column)
          {
            zMap_g_hash_table_iter_init(&iter, view->context_map.featureset_2_column) ;

            while(zMap_g_hash_table_iter_next(&iter, &key, &value))
              {
                GQuark q ;
                char *set_name ;

                set_name = (char *)g_quark_to_string(GPOINTER_TO_UINT(key)) ;

                // start with a 1-1 default mapping
                gffset = (ZMapFeatureSetDesc)value ;

                gff_source = g_new0(ZMapFeatureSourceStruct,1) ;

                gff_source->source_id = gffset->feature_src_ID;   // upper case wanted


                /* THIS WAS COMMENTED OUT MEANING SOME COLUMNS ENDED UP WITHOUT A STYLE
                 * WHEN USED WITH AN ACEDB DATABASE.....I'VE PUT IT BACK IN BUT AN NOT
                 * SURE THIS IS CORRECT, THE LOGIC HAS BECOME ARCANE....
                 */
                // hard coded in zmapGFF-parser.c
                gff_source->style_id = zMapStyleCreateID(set_name) ;


                gff_source->source_text = gff_source->source_id;

                // then overlay this with the config file

                // get the FeatureSource name
                // this is displayed at status bar top right
                if(gff_src)
                  {
                    q = GPOINTER_TO_UINT(g_hash_table_lookup(gff_src,key));
                    if(q)
                      gff_source->source_id = q;
                  }

                // get style defined by featureset name
                if(fset_styles)
                  {
                    //                if(q)                /* default to source name */
                    //                  gff_source->style_id = q;
                    /* but change to explicit config if it's there */
                    q = GPOINTER_TO_UINT(g_hash_table_lookup(fset_styles,key));
                    if(q)
                      gff_source->style_id = q;
                  }

                // get description defined by featureset name
                if(gff_desc)
                  {
                    q = GPOINTER_TO_UINT(g_hash_table_lookup(gff_desc,key));
                    if(q)
                      gff_source->source_text = q;
                  }

                if(gff_related)
                  {
                    q = GPOINTER_TO_UINT(g_hash_table_lookup(gff_related,key));
                    if(q)
                      gff_source->related_column = q;
                  }


                /* THIS COMMENT MAKES NO SENSE....WHICH FIELD IS SET...AND AREN'T YOU OVERWRITING
                 * DATA ALREADY THERE ?? NOT SURE........ */
                /* source_2_source data defaults are hard coded in GFF2parser
                   but if we set one field then we set them all */
                g_hash_table_replace(source_2_sourcedata,
                                     GUINT_TO_POINTER(zMapFeatureSetCreateID(set_name)), gff_source) ;
              }
          }

        GList *seq_data_featuresets = NULL ;

        if(zMapConfigIniContextGetString(context,
                                         ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_CONFIG,
                                         ZMAPSTANZA_APP_SEQ_DATA,&str))
          {
            seq_data_featuresets = zMapConfigString2QuarkIDGList(str);
          }

        /* add a flag for each seq_data featureset */
        for(iter = seq_data_featuresets; iter; iter = iter->next)
          {
            gff_source = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,iter->data);
            //zMapLogWarning("view is_seq: %s -> %p",g_quark_to_string(GPOINTER_TO_UINT(iter->data)),gff_source);
            if(gff_source)
              gff_source->is_seq = TRUE;
          }

        g_list_free(seq_data_featuresets) ;

        view->context_map.source_2_sourcedata = source_2_sourcedata;

        view->context_map.virtual_featuresets
          = zMapConfigIniGetFeatureset2Featureset(context, source_2_sourcedata, view->context_map.featureset_2_column);

        if(gff_src)
          g_hash_table_destroy(gff_src);
        if(fset_styles)
          g_hash_table_destroy(fset_styles);
        if(gff_desc)
          g_hash_table_destroy(gff_desc);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        print_source_2_sourcedata("view ini",view->context_map.source_2_sourcedata);
        print_fset2col("view ini",view->context_map.featureset_2_column);
        print_col2fset("view ini",view->context_map.columns);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



        /* ok....there is some problem here when a style is not explicitly
         * given for a column and the column is just derived from a featureset,
         * we don't end up defaulting to a style with the same name as the featureset.
         * ....I'm guessing
         * there must be somewhere where we don't set the style if one of these
         * already exists ???? ugh.....needs working through... */

        /*---------------------------------------------
         * context_map.column_2_styles: hash of Glist of quarks
         *---------------------------------------------
         * contains all the styles needed by a column
         * NB: style for each featureset is also held in source_2_sourcedata above
         * column specifc style (not featureset style included in column)
         * is optional and we only include it if configured
         */

        // NB we have to use zMap_g_hashlist_thing() as this is used all over
        // view->context_map.column_2_styles is pre-allocated


        // get list of featuresets for each column
        // add column style if it exists to glist
        // add corresponding featureset styles to glist
        // add glist to view->featuresets_2_styles

        // or rather, given that our data is upside down:
        // add each featureset's style to the hashlist
        // then add the column styles if configured
        // these are keyed by the column quark


        // add featureset styles
        if (view->context_map.featureset_2_column)
          {
            zMap_g_hash_table_iter_init (&iter, view->context_map.featureset_2_column);
            while (zMap_g_hash_table_iter_next (&iter, &key, &value))
              {
                GQuark style_id,fset_id;

                gffset = (ZMapFeatureSetDesc) value;

                // key is featureset quark, value is column GFFSet struct

                gff_source = (ZMapFeatureSource)g_hash_table_lookup(source_2_sourcedata,key);
                if(gff_source)
                  {
                    style_id = gff_source->style_id;
                    if(!style_id)
                      style_id = GPOINTER_TO_UINT(key);
                    //                fset_id = zMapFeatureSetCreateID((char *)g_quark_to_string(gffset->feature_set_id));
                    fset_id = gffset->column_id;

                    zMap_g_hashlist_insert(view->context_map.column_2_styles,
                                           fset_id,     // the column
                                           GUINT_TO_POINTER(style_id)) ;  // the style
                    //printf("getIniData featureset adds %s to %s\n",g_quark_to_string(style_id),g_quark_to_string(fset_id));
                  }
              }
          }


#if 0
        /* AH OK...LOOKS LIKE MALCOLM HAD THE PROBLEM DESCRIBED ABOVE ABOUT DEFAULTING
         * STYLES BUT HAS COMMENTED THIS OUT......NOT SURE WHY.... */

        /* add 1-1 mappingg from colum to style where featureset->column is not defined
           and it's not sourced from acedb */
        /* there's a race condtion... */
        for(sources = req_sources; sources ; sources = sources->next)
          {
            GList *featuresets;
            ZMapConfigSource src;
            src = (ZMapConfigSource) sources->data;


            if (!g_ascii_strncasecmp(src->url,"pipe", 4) || !g_ascii_strncasecmp(src->url,"file", 4) != 0)
              {
              }

          }
#endif


        // add col specific styles
        col_styles  = zMapConfigIniGetQQHash(context,ZMAPSTANZA_COLUMN_STYLE_CONFIG,QQ_STYLE);

        if (col_styles)
          {
            for (std::map<GQuark, ZMapFeatureColumn>::iterator map_iter = view->context_map.columns->begin();
                 map_iter != view->context_map.columns->end();
                 ++map_iter)
              {
                column = map_iter->second ;

                GQuark style_id = GPOINTER_TO_UINT(g_hash_table_lookup(col_styles,GUINT_TO_POINTER(column->unique_id)));
                // set the column style if there
                if(style_id)
                  {
                    column->style_id = style_id;
                    zMap_g_hashlist_insert(view->context_map.column_2_styles,
                                           column->unique_id,
                                           GUINT_TO_POINTER(style_id)) ;
                  }
              }
            g_hash_table_destroy(col_styles);
          }


        {

          GQuark col_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_STRAND_SEPARATOR) ;
          GQuark set_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME) ;
          ZMapFeatureSetDesc f2c ;

          /* hard coded column style for strand separator */
          std::map<GQuark, ZMapFeatureColumn>::iterator map_iter = view->context_map.columns->find(col_id) ;

          if (map_iter != view->context_map.columns->end())
            {
              column = map_iter->second ;

              column->style_id = col_id ;

              zMap_g_hashlist_insert(view->context_map.column_2_styles,
                                     column->unique_id,
                                     GUINT_TO_POINTER(col_id)) ;
              zMap_g_hashlist_insert(view->context_map.column_2_styles,
                                     column->unique_id,
                                     GUINT_TO_POINTER(set_id)) ;
            }

          /* hard coded default column for search hit markers */
          if (view->context_map.featureset_2_column)
            {
              if (!(f2c = (ZMapFeatureSetDesc)g_hash_table_lookup(view->context_map.featureset_2_column, GUINT_TO_POINTER(set_id))))
                {
                  /* NOTE this is the strand separator column which used to be the third strand
                   * it used to have a Search Hit Markers column in it sometimes
                   * but now we load these featuresets into the strand sep. directly
                   */
                  f2c = g_new0(ZMapFeatureSetDescStruct,1);
                  f2c->column_id = col_id;
                  f2c->column_ID = g_quark_from_string(ZMAP_FIXED_STYLE_STRAND_SEPARATOR);
                  f2c->feature_src_ID = g_quark_from_string(ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME);
                  f2c->feature_set_text = ZMAP_FIXED_STYLE_SEARCH_MARKERS_NAME;

                  g_hash_table_insert(view->context_map.featureset_2_column,GUINT_TO_POINTER(set_id), f2c);
                }
            }
        }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nini fset2style\n");
        zMap_g_hashlist_print(view->context_map.column_2_styles);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      }

      zMapConfigIniContextDestroy(context);
    }

  return ;
}



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing, it would actually kill/delete stuff without waiting for threads
 * to die....or we could allow a "force" flag to zmapViewKill/Destroy  */


/* This is really the guts of the code to check what a connection thread is up
 * to. Every time the GUI thread has stopped doing things GTK calls this routine
 * which then checks our connections for responses from the threads...... */
static gint zmapIdleCB(gpointer cb_data)
{
  gint call_again = 0 ;
  ZMapView zmap_view = (ZMapView)cb_data ;


  /* Returning a value > 0 tells gtk to call zmapIdleCB again, so if checkConnections() returns
   * TRUE we ask to be called again. */
  if (checkStateConnections(zmap_view))
    call_again = 1 ;
  else
    call_again = 0 ;

  return call_again ;
}



static void enterCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->enter))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


static void leaveCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->leave))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


static void scrollCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  printf("In View, in window scroll callback\n") ;

//
  return ;
}


/* Called when a sequence window has been focussed, usually by user actions. */
static void viewFocusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->focus))(view_window, view_window->parent_view->app_data, NULL) ;

  {
  /* done by focus callback above */
//    zMapWindowNavigatorFocus(view_window->parent_view->navigator_window, TRUE);
  }

  return ;
}


/* Called when some sequence window feature (e.g. column, actual feature etc.)
 * has been selected, takes care of highlighting of objects....sounds like this
 * should be done in ZMapControl to me.... */
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowSelect window_select = (ZMapWindowSelect)window_data ;
  ZMapViewSelectStruct view_select = {(ZMapWindowSelectType)0} ;

  /* I DON'T UNDERSTAND HOW WE CAN BE CALLED IF THERE IS NO SELECT...SOUNDS DUBIOUS... */

  /* Check we've got a window_select! */
  if (window_select)
    {
      if ((view_select.type = window_select->type) == ZMAPWINDOW_SELECT_SINGLE)
        {
          if (window_select->highlight_item)
            {
              GList* list_item ;

              /* Highlight the feature in all windows, BUT note that we may not find it in some
               * windows, e.g. if a window has been reverse complemented the item may be hidden. */
              list_item = g_list_first(view_window->parent_view->window_list) ;

              do
                {
                  ZMapViewWindow view_window ;
                  FooCanvasItem *item ;
                  GList *l;

                  view_window = (ZMapViewWindow)(list_item->data) ;

                  if ((item = zMapWindowFindFeatureItemByItem(view_window->window, window_select->highlight_item)))
                    {
                      zMapWindowHighlightObject(view_window->window, item,
                                                window_select->replace_highlight_item,
                                                window_select->highlight_same_names,
                                                window_select->sub_part) ;
                    }

                  for(l = window_select->feature_list;l; l = l->next)
                    {
                      ZMapFeature feature = (ZMapFeature) l->data;

                      /* NOTE we restrict multi select to one column in line with previous policy (in the calling code)
                       * NOTE: can have several featuresets in one column
                       * feature_list inlcudes the first and second and subsequent features found,
                       * the first is also given explicitly in the item
                       */
                      if(!l->prev)                /* already dome the first one */
                        continue;

                      zMapWindowHighlightFeature(view_window->window, feature, window_select->highlight_same_names, FALSE);
                    }
                }
              while ((list_item = g_list_next(list_item))) ;
            }

          view_select.feature_desc = window_select->feature_desc ;
          view_select.secondary_text = window_select->secondary_text ;

          view_select.filter = window_select->filter ;
        }


      /* Pass back a ZMapViewWindow as it has both the View and the window to our caller. */
      (*(view_cbs_G->select))(view_window, view_window->parent_view->app_data, &view_select) ;
    }

  return ;
}



static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
  ZMapWindowSplitting split  = (ZMapWindowSplitting)window_data;
  ZMapViewSplittingStruct view_split = {0};

  view_split.split_patterns      = split->split_patterns;
  view_split.touched_window_list = NULL;

  view_split.touched_window_list = g_list_append(view_split.touched_window_list, view_window);

  (*(view_cbs_G->split_to_pattern))(view_window, view_window->parent_view->app_data, &view_split);

  /* foreach window find feature and Do something according to pattern */
  split->window_index = 0;
  g_list_foreach(view_split.touched_window_list, splitMagic, window_data);

  /* clean up the list */
  g_list_free(view_split.touched_window_list);

  return ;
}

static void splitMagic(gpointer data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data;
  ZMapWindowSplitting  split = (ZMapWindowSplitting)user_data;
  ZMapSplitPattern   pattern = NULL;

  if(view_window->window == split->original_window)
    {
      printf("Ignoring original window for now."
             "  I may well revisit this, but I"
             " think we should leave it alone ATM.\n");
      return ;                    /* really return from this */
    }

  if((pattern = &(g_array_index(split->split_patterns, ZMapSplitPatternStruct, split->window_index))))
    {
      printf("Trying pattern %d\n", split->window_index);
      /* Do it here! */
    }

  (split->window_index)++;

  return ;
}


/* This function checks the status of the connection and checks for any reply and
 * then acts on it, it gets called from the ZMap idle function.
 * If all threads are ok and zmap has not been killed then routine returns TRUE
 * meaning it wants to be called again, otherwise FALSE.
 *
 * The function monitors the View state so that when the last connection has disappeared
 * and the View is dying then the View is cleaned up and the caller gets called to say
 * the View is now dead.
 *
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *
 *
 * There are now too many state variables here, it's all confusing, this routine needs
 * a big tidy up.
 *
 *
 *  */
static gboolean checkStateConnections(ZMapView zmap_view)
{
  gboolean call_again = TRUE ;                                    /* Normally we want to be called continuously. */
  gboolean state_change = TRUE ;                            /* Has view state changed ?. */
  gboolean reqs_finished = FALSE ;                            /* at least one thread just finished */
  int has_step_list = 0 ;                                    /* any requests still active? */


  /* We should fix this so the function is not called unless there are connections... */
  if (zmap_view->connection_list)
    {
      GList *list_item ;

      list_item = g_list_first(zmap_view->connection_list) ;


      /* GOSH THE STATE HAS BECOME EVER MORE COMPLEX HERE...NEEDS A GOOD CLEAN UP.
       *
       * MALCOLM'S ATTEMPT TO CLEAN IT UP WITH A NEW THREAD STATE HAS JUST MADE IT
       * EVEN MORE COMPLEX....
       *  */


      do
        {
          ZMapNewDataSource view_con ;
          ZMapThread thread ;
          ZMapThreadReply reply = ZMAPTHREAD_REPLY_INVALID ;
          void *data = NULL ;
          char *err_msg = NULL ;
          gint err_code = -1 ;
          gboolean thread_has_died = FALSE ;
          gboolean all_steps_finished = FALSE ;
          gboolean this_step_finished = FALSE ;
          ZMapServerReqType request_type = ZMAP_SERVERREQ_INVALID ;
          gboolean is_continue = FALSE ;
          ZMapConnectionData connect_data = NULL ;
          ZMapViewConnectionStepList step_list = NULL ;
          gboolean is_empty = FALSE ;

          view_con = (ZMapNewDataSource)(list_item->data) ;
          thread = view_con->thread->GetThread() ;

          data = NULL ;
          err_msg = NULL ;


          connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_con) ;


          // WHEN WOULD WE NOT HAVE CONNECT DATA ???? SEEMS A BIT ODD.....

          /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
           * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
           * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
           * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

          // need to copy this info in case of thread death which clears it up
          if (connect_data)
            {
              /* Does this need to be separate....?? probably not.... */

              /* this seems to leak memory but it's very hard to trace the logic of its usage. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              /* This leaks memory, I've removed the copy and tested but the logic is hard to follow
                 * and it's not at all clear when the list goes away....if there's a subsequent
                 * problem then it needs sorting out properly or hacking by doing a g_list_free()
                 * on connect_data->loaded_features->feature_set and reallocating..... 2/12/2014 EG
                 *  */
              connect_data->loaded_features->feature_sets = g_list_copy(connect_data->feature_sets) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
              connect_data->loaded_features->feature_sets = connect_data->feature_sets ;

              connect_data->loaded_features->xwid = zmap_view->xwid ;

              step_list = connect_data->step_list ;
            }

          if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
            {
              /* We assume that something bad has happened to the connection and remove it
               * if we can't read the reply. */

              THREAD_DEBUG_MSG(thread, "cannot access reply from child thread - %s", err_msg) ;

              /* Warn the user ! Note: I'm trying overriding show_warning for the create step as
                 not showing a warning can mask a configuration error that the user should be
                 warned about. */
              if ((request_type == ZMAP_SERVERREQ_CREATE || view_con->show_warning)
                  && !zMapViewGetDisablePopups(zmap_view))
                {
                  zMapWarning("Source \"%s\" is being removed, error was: %s\n",
                              zMapServerGetUrl(view_con), (err_msg ? err_msg : "<no error message>")) ;
                  zMapViewSetDisablePopups(zmap_view, true) ;
                }

              zMapLogCritical("Source \"%s\", cannot access reply from server thread,"
                              " error was: %s", zMapServerGetUrl(view_con), (err_msg ? err_msg : "<no error message>")) ;

              thread_has_died = TRUE ;
            }
          else
            {
              ZMapServerReqAny req_any = NULL ;

              /* Recover the request from the thread data....is there always data ? check this... */
              if (data)
                {
                  req_any = (ZMapServerReqAny)data ;
                  request_type = req_any->type ;

                  if (req_any->response == ZMAP_SERVERRESPONSE_SOURCEEMPTY)
                    is_empty = TRUE ;
                  else
                    is_empty = FALSE ;
                }


              switch (reply)
                {
                case ZMAPTHREAD_REPLY_WAIT:
                  {
                    state_change = FALSE ;

                    break ;
                  }
                case ZMAPTHREAD_REPLY_GOTDATA:
                case ZMAPTHREAD_REPLY_REQERROR:
                  {
                    ZMapViewConnectionRequest request ;
                    ZMapViewConnectionStep step = NULL ;
                    gboolean kill_connection = FALSE ;


                    /* this is not good and shows this function needs rewriting....
                     * if we get this it means the thread has already failed and been
                     * asked by us to die, it dies but something goes wrong in the dying
                     * so we get an error in clearing up...need to check why...so
                     * we should not go on and process this request in the normal way.
                     *  */
                    if (reply == ZMAPTHREAD_REPLY_REQERROR && view_con->thread_status == THREAD_STATUS_FAILED)
                      {
                        if (!(step_list))
                          THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                "%s", "thread asked to quit but failed in clearing up.") ;
                        else
                          THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                "%s", "LOGIC ERROR....CHECK WHAT'S HAPPENING !") ;

                        /* I'm loathe to do this but all this badly needs a rewrite... */
                        continue ;
                      }

                    /* WHY IS THIS DONE ?? */
                    view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;


                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply, "%s", "thread replied") ;


                    /* Recover the stepRequest from the view connection and process the data from
                     * the request. */
                    if (!req_any || !view_con || !(request = zmapViewStepListFindRequest(step_list, req_any->type, view_con)))
                      {
                        zMapLogCritical("Request of type %s for connection %s not found in view %s step list !",
                                        (req_any ? zMapServerReqType2ExactStr(req_any->type) : ""),
                                        (view_con ? zMapServerGetUrl(view_con) : ""),
                                        (zmap_view ? zmap_view->view_name : "")) ;

                        kill_connection = TRUE ;
                      }
                    else
                      {
                        step = zmapViewStepListGetCurrentStep(step_list) ;

                        if (reply == ZMAPTHREAD_REPLY_REQERROR)
                          {
                            /* This means the request failed for some reason. */
                            if (err_msg  && zMapStepOnFailAction(step) != REQUEST_ONFAIL_CONTINUE)
                              {
                                THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                      "Source has returned error \"%s\"", err_msg) ;
                              }

                            this_step_finished = TRUE ;
                          }
                        else
                          {
                            if (zmap_view->state != ZMAPVIEW_LOADING && zmap_view->state != ZMAPVIEW_UPDATING)
                              {
                                THREAD_DEBUG_MSG(thread, "got data but ZMap state is - %s",
                                                 zMapView2Str(zMapViewGetStatus(zmap_view))) ;
                              }

                            zmapViewStepListStepProcessRequest(step_list, (void *)view_con, request) ;

                            if (zMapConnectionIsFinished(request))
                              {
                                this_step_finished = TRUE ;
                                request_type = req_any->type ;
                                view_con->thread_status = THREAD_STATUS_OK ;

                                if (req_any->type == ZMAP_SERVERREQ_FEATURES)
                                  zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                                         connect_data->feature_sets) ;

                              }
                            else
                              {
                                reply = ZMAPTHREAD_REPLY_REQERROR;    // ie there was an error


                                /* TRY THIS HERE.... */
                                if (err_msg)
                                  {
                                    g_free(err_msg) ;
                                    err_msg = NULL ;
                                  }
                                
                                if (connect_data && connect_data->error)
                                  {
                                    err_msg = g_strdup(connect_data->error->message) ;
                                    err_code = connect_data->error->code ;
                                  }

                                this_step_finished = TRUE ;
                              }
                           }
                      }

                    if (reply == ZMAPTHREAD_REPLY_REQERROR)
                      {
                        if (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_THREAD
                            || zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_STEPLIST)
                          {
                            if (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_THREAD)
                              kill_connection = TRUE ;

                            /* Remove request from all steps.... */
                            zmapViewStepListDestroy(step_list) ;
                            connect_data->step_list = step_list = NULL ;

                            zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                                   connect_data->feature_sets) ;
                          }
                        else
                          {
                            zMapStepSetState(step, STEPLIST_FINISHED) ;
                          }



                        view_con->thread_status = THREAD_STATUS_FAILED;        /* so that we report an error */
                      }

                    if (kill_connection)
                      {
                        if (is_empty)
                          zmap_view->sources_empty
                            = zMap_g_list_insert_list_after(zmap_view->sources_empty,
                                                            connect_data->loaded_features->feature_sets,
                                                            g_list_length(zmap_view->sources_empty),
                                                            TRUE) ;
                        else
                          zmap_view->sources_failed
                            = zMap_g_list_insert_list_after(zmap_view->sources_failed,
                                                            connect_data->loaded_features->feature_sets,
                                                            g_list_length(zmap_view->sources_failed),
                                                            TRUE) ;



                        /* Do not reset reply from slave, we need to wait for slave to reply
                         * to the cancel. */

                        /* Warn the user ! Note: I'm trying overriding show_warning for the create step as
                           not showing a warning can mask a configuration error that the user should be
                           warned about. */
                        if (request_type == ZMAP_SERVERREQ_CREATE || view_con->show_warning)
                          {
                            if (!zMapViewGetDisablePopups(zmap_view))
                              {
                                if (err_code != ZMAPVIEW_ERROR_CONTEXT_EMPTY)
                                  zMapWarning(SOURCE_FAILURE_WARNING_FORMAT, (err_msg ? err_msg : "<no error message>")) ;
                                zMapViewSetDisablePopups(zmap_view, true) ;
                              }

                            zMapLogWarning("Source failed with error: '%s'. Source: %s",
                                           (err_msg ? err_msg : "<no error message>"), zMapServerGetUrl(view_con)) ;
                          }

                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "Thread being cancelled because of error \"%s\"",
                                              err_msg) ;

                        /* Signal thread to die. */
                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "%s", "signalling child thread to die....") ;
                        zMapThreadKill(thread) ;
                        thread_has_died = TRUE ; 
                      }
                    else
                      {
                        /* Reset the reply from the slave. */
                        zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
                      }

                    break ;
                  }
                case ZMAPTHREAD_REPLY_DIED:
                  {
                    ZMapViewConnectionStep step ;

                    step = zmapViewStepListGetCurrentStep(step_list) ;

                    /* THIS LOGIC CAN'T BE RIGHT....IF WE COME IN HERE IT'S BECAUSE THE THREAD
                     * HAS ACTUALLY DIED AND NOT JUST QUIT....CHECK STATUS SET IN SLAVES... */
                    if (zMapStepOnFailAction(step) != REQUEST_ONFAIL_CONTINUE)
                      {
                        /* Thread has failed for some reason and we should clean up. */
                        if (err_msg && view_con->show_warning && 
                            !zMapViewGetDisablePopups(zmap_view))
                          {
                            zMapWarning(SOURCE_FAILURE_WARNING_FORMAT, (err_msg ? err_msg : "<no error>")) ;
                            zMapViewSetDisablePopups(zmap_view, true) ;
                          }

                        thread_has_died = TRUE ;

                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "cleaning up because child thread has died with: \"%s\"", err_msg) ;
                      }
                    else
                      {
                        /* mark the current step as finished or else we won't move on
                         * this was buried in zmapViewStepListStepProcessRequest()
                         */
                        zMapStepSetState(step, STEPLIST_FINISHED) ;

                        /* Reset the reply from the slave. */
                        zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
                      }

                    break ;
                  }
                case ZMAPTHREAD_REPLY_CANCELLED:
                  {
                    /* This happens when we have signalled the threads to die and they are
                     * replying to say that they have now died. */
                    thread_has_died = TRUE ;

                    /* This means the thread was cancelled so we should clean up..... */
                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                          "%s", "child thread has been cancelled so cleaning up....") ;

                    break ;
                  }
                case ZMAPTHREAD_REPLY_QUIT:
                  {
                    thread_has_died = TRUE;

                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                          "%s", "child thread has quit so cleaning up....") ;

                    break;
                  }

                default:
                  {
                    zMapLogFatalLogicErr("switch(), unknown value: %d", reply) ;

                    break ;
                  }
                }

            }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* There is a problem with the zMapThreadExists() call on the Mac,
           * it says the thread has died when it hasn't. */

          if (!thread_has_died && !zMapThreadExists(thread))
            {
              thread_has_died = TRUE;
              // message to differ from REPLY_DIED above
              // it really is sudden death, thread is just not there
              THREAD_DEBUG_MSG(thread, "%s", "child thread has died suddenly so cleaning up....") ;
            }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



          /* CHECK HOW STEPS_FINISHED IS SET...SOMETHING IS WRONG.....GETTING CALLED EVERYTIME A
             SOURCE
             * FINISHES INSTEAD WHEN ALL SOURCES ARE FINISHED.... */



          /* If the thread has died then remove it's connection. */
          // do this before counting up the number of step lists
          if (thread_has_died)
            {
              ZMapViewConnectionStep step;

              /* TRY THIS HERE.... */
              /* Add any new sources that have failed. */
              if (connect_data->loaded_features->feature_sets && reply == ZMAPTHREAD_REPLY_DIED)
                {
                  if (is_empty)
                    zmap_view->sources_empty
                      = zMap_g_list_insert_list_after(zmap_view->sources_empty,
                                                      connect_data->loaded_features->feature_sets,
                                                      g_list_length(zmap_view->sources_empty),
                                                      TRUE) ;
                  else
                    zmap_view->sources_failed
                      = zMap_g_list_insert_list_after(zmap_view->sources_failed,
                                                      connect_data->loaded_features->feature_sets,
                                                      g_list_length(zmap_view->sources_failed),
                                                      TRUE) ;
                }


              is_continue = FALSE ;


              if (step_list && (step = zmapViewStepListGetCurrentStep(step_list)))
                {
                  is_continue = (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CONTINUE) ;
                }

              /* We are going to remove an item from the list so better move on from
               * this item. */
              zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;
              list_item = zmap_view->connection_list ;


              /* GOSH....I DON'T UNDERSTAND THIS..... */
              if (step_list)
                reqs_finished = TRUE;


              if (reply == ZMAPTHREAD_REPLY_QUIT && view_con->thread_status != THREAD_STATUS_FAILED)
                {

                  // segfaults because step is null but not sure how I caused that....
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  if (zMapStepGetRequest(step) == ZMAP_SERVERREQ_TERMINATE)  /* normal OK status in response */
                    {
                      view_con->thread_status = THREAD_STATUS_OK ;
                    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                  view_con->thread_status = THREAD_STATUS_OK ;
                }
              else
                {
                  view_con->thread_status = THREAD_STATUS_FAILED ;
                }

               all_steps_finished = TRUE ;                /* destroy connection kills the step list */
            }

          /* Check for more connection steps and dispatch them or clear up if finished. */
          if ((step_list))
            {
              /* If there were errors then all connections may have been removed from
               * step list or if we have finished then destroy step_list. */
              if (zmapViewStepListIsNext(step_list))
                {
                  zmapViewStepListIter(step_list, view_con->thread->GetThread(), view_con) ;

                  has_step_list++;

                  all_steps_finished = FALSE ;
                }
              else
                {
                  zmapViewStepListDestroy(step_list) ;
                  connect_data->step_list = step_list = NULL ;

                  reqs_finished = TRUE;

                  if (view_con->thread_status != THREAD_STATUS_FAILED)
                    view_con->thread_status = THREAD_STATUS_OK ;

                  all_steps_finished = TRUE ;
                }
            }


          /* ...shouldn't be doing this if we are dying....I think ? we should
           * have disconnected this handler....that's why we need the stopConnectionState call
           * add it in.... */

          if (this_step_finished || all_steps_finished)
            {
              if (connect_data)      // ie was valid at the start of the loop
                {
                  if (connect_data->exit_code)
                    view_con->thread_status = THREAD_STATUS_FAILED ;

                  if (reply == ZMAPTHREAD_REPLY_DIED)
                    connect_data->exit_code = 1 ;

                  if (view_con->thread_status == THREAD_STATUS_FAILED)
                    {
                      //char *request_type_str = (char *)zMapServerReqType2ExactStr(request_type) ;

                      if (!err_msg)
                        {
                          /* NOTE on TERMINATE OK/REPLY_QUIT we get thread_has_died and NULL the error message */
                          /* but if we set thread_status for FAILED on successful exit then we get this, so let's not do that: */
                          err_msg = g_strdup("Thread failed but there is no error message to say why !") ;

                          zMapLogWarning("%s", err_msg) ;
                        }

                      if (view_con->show_warning && is_continue)
                        {
                          /* we get here at the end of a step list, prev errors not reported till now */
                          if (!zMapViewGetDisablePopups(zmap_view))
                            {
                              zMapWarning(SOURCE_FAILURE_WARNING_FORMAT, (err_msg ? err_msg : "<no error>")) ;
                              zMapViewSetDisablePopups(zmap_view, true) ;
                            }
                        }

                      //zMapLogWarning("Thread %p failed, request = %s, empty sources now %d, failed sources now %d",
                      //               thread,
                      //               request_type_str,
                      //               g_list_length(zmap_view->sources_empty),
                      //               g_list_length(zmap_view->sources_failed)) ;
                    }

                  /* Record if features were loaded or if there was an error, if the latter
                   * then report that to our remote peer if there is one. */
                  if (request_type == ZMAP_SERVERREQ_FEATURES || reply == ZMAPTHREAD_REPLY_REQERROR)
                    {
                      connect_data->loaded_features->status = (view_con->thread_status == THREAD_STATUS_OK
                                                               ? TRUE : FALSE) ;

                      if (connect_data->loaded_features->err_msg)
                        g_free(connect_data->loaded_features->err_msg) ;

                      connect_data->loaded_features->err_msg = g_strdup(err_msg) ;
                      connect_data->loaded_features->start = connect_data->start;
                      connect_data->loaded_features->end = connect_data->end;
                      connect_data->loaded_features->exit_code = connect_data->exit_code ;
                      connect_data->loaded_features->stderr_out = connect_data->stderr_out ;

                      if (reply == ZMAPTHREAD_REPLY_REQERROR && zmap_view->remote_control)
                        {
                          /* Note that load_features is cleaned up by sendViewLoaded() */
                          LoadFeaturesData loaded_features ;
                          if (is_empty)
                            connect_data->loaded_features->status = TRUE ;

                          loaded_features = zmapViewCopyLoadFeatures(connect_data->loaded_features) ;

                          zMapLogWarning("View loaded from %s !!", "checkStateConnections()") ;

                          sendViewLoaded(zmap_view, loaded_features) ;
                        }
                    }


                }
            }

          if (err_msg)
            {
              g_free(err_msg) ;
              err_msg = NULL ;
            }

          if (thread_has_died)
            {

              // Need to get rid of other stuff too......
              /* There is more to do here but proceed carefully, sometimes parts of the code are still
               * referring to this because of asynchronous returns from zmapWindow and other code. */
              ZMapConnectionData connect_data ;

              if ((connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_con)))
                {
                  if (connect_data->loaded_features)
                    {
                      zmapViewDestroyLoadFeatures(connect_data->loaded_features) ;

                      connect_data->loaded_features = NULL ;
                    }

                  if (connect_data->error)
                    {
                      g_error_free(connect_data->error) ;
                      connect_data->error = NULL ;
                    }

                  if (connect_data->step_list)
                    {
                      zmapViewStepListDestroy(connect_data->step_list) ;
                      connect_data->step_list = NULL ;
                    }

                  g_free(connect_data) ;
                  view_con->request_data = NULL ;


                  /* Now there is no current view_con.....this is the WRONG PLACE FOR THIS.....SIGH.... */
                  if (zmap_view->sequence_server == view_con)
                    zmap_view->sequence_server = NULL ;

                  zMapServerDestroyViewConnection(view_con) ;
                  view_con = NULL ;
                }
            }

          if (thread_has_died || all_steps_finished)
            {
              zmapViewBusy(zmap_view, FALSE) ;
            }

        } while (list_item && (list_item = g_list_next(list_item))) ;
    }



  /* ok...there's some problem here, the loaded counter gets decremented in
   * processDataRequests() and it can reach zero but we don't enter this section
   * of code so we don't record the state as "loaded"....agh.....
   *
   * One problem might be that has_step_list is only ever incremented ??
   *
   * Looks like reqs_finished == TRUE but there is still a step_list.
   *
   *  */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Use this to trap this problem..... */
  if ((has_step_list || !reqs_finished) && !(zmap_view->sources_loading))
    printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Once a view is loaded there won't be any more column changes until the user elects
   * to load a new column. */
  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      if (zmap_view->state != ZMAPVIEW_LOADED)
        {
          if (!has_step_list && reqs_finished)
            {
              /*
               * rather than count up the number loaded we say 'LOADED' if there's no LOADING active
               * This accounts for failures as well as completed loads
               */
              zmap_view->state = ZMAPVIEW_LOADED ;
              g_list_free(zmap_view->sources_loading) ;
              zmap_view->sources_loading = NULL ;

              state_change = TRUE ;
            }
          else if (!(zmap_view->sources_loading))
            {
              /* We shouldn't need to do this if reqs_finished and has_step_list were consistent. */

              zmap_view->state = ZMAPVIEW_LOADED ;
              state_change = TRUE ;
            }
        }
    }



  /* If we have connections left then inform the layer about us that our state has changed. */
  if ((zmap_view->connection_list) && state_change)
    {
      (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;
    }


  /* At this point if we have connections then we carry on looping looking for
   * replies from the views. If there are no threads left then we need to examine
   * our state and take action depending on whether we are dying or threads
   * have died or whatever.... */
  if (!zmap_view->connection_list)
    {
      /* Decide if we need to be called again or if everythings dead. */
      call_again = checkContinue(zmap_view) ;
    }


  return call_again ;
}


static ZMapView createZMapView(char *view_name, GList *sequences, void *app_data)
{
  ZMapView zmap_view = NULL ;
  GList *first ;
  ZMapFeatureSequenceMap master_seq ;

  first = g_list_first(sequences) ;
  master_seq = (ZMapFeatureSequenceMap)(first->data) ;

  zmap_view = g_new0(ZMapViewStruct, 1) ;

  zmap_view->state = ZMAPVIEW_INIT ;
  zmap_view->busy = FALSE ;
  zmap_view->disable_popups = false ;


  zmap_view->view_name = g_strdup(view_name) ;

  /* TEMP CODE...UNTIL I GET THE MULTIPLE SEQUENCES IN ONE VIEW SORTED OUT..... */
  /* TOTAL HACK UP MESS.... */

  // mh17:  we expect the sequence name to be like 'chr4-04_210623-364887'
  // and use start to calculate chromosome coordinates for features
  // see zmapWindow.c/myWindowCreate() for where this happens

  zmap_view->view_sequence = master_seq;  //g_memdup(master_seq,sizeof(ZMapFeatureSequenceMapStruct));

#ifdef NOT_REQUIRED_ATM
  /* TOTAL LASH UP FOR NOW..... */
  if (!(zmap_view->sequence_2_server))
    {
      getSequenceServers(zmap_view, NULL) ;
    }
#endif

  /* Set the regions we want to display. */
  zmap_view->sequence_mapping = sequences ;

  zmap_view->window_list = zmap_view->connection_list = NULL ;

  zmap_view->app_data = app_data ;

  zmap_view->kill_blixems = TRUE ;

  /* If a remote request func is registered then we're an xremote client */
  if (view_cbs_G->remote_request_func)
    zmap_view->xremote_client = TRUE ;

  return zmap_view ;
}


/* Adds a window to a view. */
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget)
{
  ZMapViewWindow view_window = NULL ;
  ZMapWindow window ;

  view_window = createWindow(zmap_view, NULL) ;

  /* There are no steps where this can fail at the moment. */
  window = zMapWindowCreate(parent_widget,
                            zmap_view->view_sequence,
                            view_window,
                            NULL,
                            zmap_view->int_values) ;

  view_window->window = window ;

  /* add to list of windows.... */
  zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;


  return view_window ;
}

static void killAllSpawned(ZMapView zmap_view)
{
  GPid pid;
  GList *processes = zmap_view->spawned_processes;

  if (zmap_view->kill_blixems)
    {
      while (processes)
        {
          pid = GPOINTER_TO_INT(processes->data);
          g_spawn_close_pid(pid);
          kill(pid, 9);
          processes = processes->next;
        }
    }

  if (zmap_view->spawned_processes)
    {
      g_list_free(zmap_view->spawned_processes);
      zmap_view->spawned_processes = NULL ;
    }

  return ;
}

/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. The lists of windows and thread connections are dealt with somewhat
 * asynchronously by killGUI() & checkConnections() */
static void destroyZMapView(ZMapView *zmap_view_out)
{
  ZMapView zmap_view = *zmap_view_out ;

  if(zmap_view->view_sequence)
  {
//      if(zmap_view->view_sequence->sequence)
//           g_free(zmap_view->view_sequence->sequence);
//      g_free(zmap_view->view_sequence) ;
      zmap_view->view_sequence = NULL;
  }

#ifdef NOT_REQUIRED_ATM
  if (zmap_view->sequence_2_server)
    {
      g_list_foreach(zmap_view->sequence_2_server, destroySeq2ServerCB, NULL) ;
      g_list_free(zmap_view->sequence_2_server) ;
      zmap_view->sequence_2_server = NULL ;
    }
#endif /* NOT_REQUIRED_ATM */

  if (zmap_view->cwh_hash)
    zmapViewCWHDestroy(&(zmap_view->cwh_hash));

  if (zmap_view->context_map.column_2_styles)
    zMap_g_hashlist_destroy(zmap_view->context_map.column_2_styles) ;

  killAllSpawned(zmap_view);

  g_free(zmap_view) ;

  *zmap_view_out = NULL ;

  return ;
}




/*
 *       Connection control functions, interface to the data fetching threads.
 */


/* Start the ZMapView GTK idle function (gets run when the GUI is doing nothing).
 */
static void startStateConnectionChecking(ZMapView zmap_view)
{

#ifdef UTILISE_ALL_CPU_ON_DESKPRO203
  zmap_view->idle_handle = gtk_idle_add(zmapIdleCB, (gpointer)zmap_view) ;
#endif /* UTILISE_ALL_CPU_ON_DESKPRO203 */



  zmap_view->idle_handle = gtk_timeout_add(100, zmapIdleCB, (gpointer)zmap_view) ;
  // WARNING: gtk_timeout_add is deprecated and should not be used in newly-written code. Use g_timeout_add() instead.

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* WE do need to do this in fact......we should do it when all connections have died...
 * ADD CODE TO call this.... */


/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopStateConnectionChecking(ZMapView zmap_view)
{
  gtk_timeout_remove(zmap_view->idle_handle) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





void printStyle(GQuark style_id, gpointer data, gpointer user_data)
{
  char *x = (char *) user_data;
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle) data;

  zMapLogWarning("%s: style %s = %s (%d)",x,g_quark_to_string(style_id), g_quark_to_string(style->unique_id), style->default_bump_mode);
}




/*
 *      Callbacks for getting local sequences for passing to blixem.
 */
static gboolean processGetSeqRequests(void *user_data, ZMapServerReqAny req_any)
{
  gboolean result = FALSE ;
  ZMapView zmap_view = (ZMapView)user_data ;


  if (req_any->type == ZMAP_SERVERREQ_GETSEQUENCE)
    {
      ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;
      ZMapWindowCallbackCommandAlign align = (ZMapWindowCallbackCommandAlign)(get_sequence->caller_data) ;
      GPid blixem_pid ;
      gboolean status ;

      /* Got the sequences so launch blixem. */
      if ((status = zmapViewCallBlixem(zmap_view, align->block,
                                       align->homol_type,
                                       align->offset, align->cursor_position,
                                       align->window_start, align->window_end,
                                       align->mark_start, align->mark_end, align->features_from_mark,
                                       align->homol_set,
                                       align->isSeq,
                                       align->features, align->feature_set, NULL, get_sequence->sequences,
                                       &blixem_pid, &(zmap_view->kill_blixems))))
        {
          zmap_view->spawned_processes = g_list_append(zmap_view->spawned_processes,
                                                       GINT_TO_POINTER(blixem_pid)) ;
        }

      result = TRUE ;
    }
  else
    {
      zMapLogFatalLogicErr("wrong request type: %d", req_any->type) ;

      result = FALSE ;
    }


  return result ;
}



/* Kill all the windows... */
static void killGUI(ZMapView view)
{
  while (view->window_list)
    {
      ZMapViewWindow view_window ;

      view_window = (ZMapViewWindow)(view->window_list->data) ;

      destroyWindow(view, view_window) ;
    }

  return ;
}


static void killConnections(ZMapView zmap_view)
{
  GList* list_item ;

  if ((zmap_view->connection_list))
    {
      list_item = g_list_first(zmap_view->connection_list) ;
      do
        {
          ZMapNewDataSource view_con ;
          ZMapThread thread ;

          view_con = (ZMapNewDataSource)(list_item->data) ;
          thread = view_con->thread->GetThread() ;

          /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
           * will actually die sometime later. */
          if (zMapThreadExists(thread))
            zMapThreadKill(thread) ;
        }
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}




/* set all the windows attached to this view so that they contain nothing. */
static void resetWindows(ZMapView zmap_view)
{
  GList* list_item ;

  list_item = g_list_first(zmap_view->window_list) ;

  do
    {
      ZMapViewWindow view_window ;

      view_window = (ZMapViewWindow)(list_item->data) ;

      zMapWindowReset(view_window->window) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}



/* Signal all windows there is data to draw. */
void displayDataWindows(ZMapView zmap_view,
                        ZMapFeatureContext all_features, ZMapFeatureContext new_features,
                        LoadFeaturesData loaded_features_in,
                        gboolean undisplay, GList *masked,
                        ZMapFeature highlight_feature, gboolean splice_highlight,
                        gboolean allow_clean)
{
  GList *list_item, *window_list  = NULL;
  gboolean clean_required = FALSE;

  list_item = g_list_first(zmap_view->window_list) ;

  /* when the new features aren't the stored features i.e. not the first draw */
  /* un-drawing the features doesn't work the same way as drawing */
  if (all_features != new_features && !undisplay && allow_clean)
    {
      clean_required = TRUE;
    }

  do
    {
      ZMapViewWindow view_window ;

      view_window = (ZMapViewWindow)(list_item->data) ;

      if (!undisplay)
        {
          LoadFeaturesData loaded_features = NULL ;

          /* we need to tell the user what features were loaded but this struct needs to
           * persist as the information may be passed to loadedDataCB() _after_ we have
           * destroyed the connection. */
          if (loaded_features_in)
            {
              loaded_features = zmapViewCopyLoadFeatures(loaded_features_in) ;
            }

          zMapWindowDisplayData(view_window->window, NULL,
                                all_features, new_features,
                                &zmap_view->context_map,
                                masked,
                                highlight_feature, splice_highlight,
                                loaded_features) ;
        }
      else
        {
          zMapWindowUnDisplayData(view_window->window, all_features, new_features) ;
        }

      if (clean_required)
        window_list = g_list_append(window_list, view_window->window);
    }
  while ((list_item = g_list_next(list_item))) ;

  if (clean_required)
    zmapViewCWHSetList(zmap_view->cwh_hash, new_features, window_list);

  return ;
}


static void loadedDataCB(ZMapWindow window, void *caller_data, gpointer loaded_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
  LoadFeaturesData loaded_features = (LoadFeaturesData)loaded_data ;
  ZMapFeatureContext context = (ZMapFeatureContext)window_data;
  ZMapView view;
  gboolean removed, debug = FALSE, unique_context;

  view = zMapViewGetView(view_window);

  if(debug)
    zMapLogWarning("%s", "Attempting to Destroy Diff Context");

  removed = zmapViewCWHRemoveContextWindow(view->cwh_hash, &context, window, &unique_context);

  if(debug)
    {
      if(removed)
        zMapLogWarning("%s", "Context was destroyed");
      else if(unique_context)
        zMapLogWarning("%s", "Context is the _only_ context");
      else
        zMapLogWarning("%s", "Another window still needs the context memory");
    }

  if(removed || unique_context)
    (*(view_cbs_G->load_data))(view, view->app_data, NULL) ;


  if (view->remote_control && loaded_features)
    {
      zMapLogMessage("Received LoadFeaturesDataStruct to pass to sendViewLoaded() at: %p, ", loaded_features) ;

      if (view->remote_control)
        {
          zMapLogWarning("VIEW LOADED FROM %s !!", "loadedDataCB()") ;

          sendViewLoaded(view, loaded_features) ;
        }

      /* We were passed a copy of this by displayDataWindows() and now we can delete it. */
      zmapViewDestroyLoadFeatures(loaded_features) ;
    }

  return ;
}


static gboolean mergeNewFeatureCB(ZMapWindow window, void *caller_data, void *window_data)
{
  gboolean result = TRUE ;

  GtkResponseType response = GTK_RESPONSE_CANCEL ;
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowMergeNewFeature merge = (ZMapWindowMergeNewFeature)window_data ;
  ZMapView view = zMapViewGetView(view_window) ;
  GQuark annotation_column_id = zMapFeatureSetCreateID(ZMAP_FIXED_STYLE_SCRATCH_NAME);

  if (merge->feature_set->unique_id != annotation_column_id)
    {
      /* If the feature already exists in the feature set ask if we should overwrite it. If
       * so, we'll delete the old feature before creating the new one. We want to check if a
       * feature exists in this featureset with the same name and strand regardless of start/end
       * coords, because the user may have edited the coords and want to overwrite them in the
       * original feature. */
      GList *existing_features = zMapFeatureSetGetNamedFeaturesForStrand(merge->feature_set, merge->feature->original_id, merge->feature->strand) ;

      //ZMapFeatureAny existing_feature = zMapFeatureParentGetFeatureByID((ZMapFeatureAny)merge->feature_set, merge->feature->unique_id) ;

      if (existing_features)
        {
          if (g_list_length(existing_features) == 1)
            {
              ZMapFeature existing_feature = (ZMapFeature)(existing_features->data) ;

              char *msg = g_strdup_printf("Feature '%s' already exists in strand %s of featureset %s: overwrite?",
                                          g_quark_to_string(merge->feature->original_id),
                                          zMapFeatureStrand2Str(merge->feature->strand),
                                          g_quark_to_string(merge->feature_set->original_id)) ;

              response = zMapGUIMsgGetYesNoCancel(NULL, ZMAP_MSG_WARNING, msg) ;
              g_free(msg) ;

              /* The response is OK for Yes, Close for No or Cancel for Cancel */
              if (response == GTK_RESPONSE_OK)
                {
                  /* OK means Yes, overwrite the existing feature, so delete it first */
                  GList *feature_list = NULL ;
                  ZMapFeatureContext context_copy = zmapViewCopyContextAll(view->features, (ZMapFeature)existing_feature, merge->feature_set, &feature_list, NULL) ;

                  if (context_copy && feature_list)
                    zmapViewEraseFeatures(view, context_copy, &feature_list) ;

                  if (context_copy)
                    zMapFeatureContextDestroy(context_copy, TRUE) ;
                }
              else if (response == GTK_RESPONSE_CANCEL)
                {
                  /* Cancel: don't save at all */
                  result = FALSE ;
                }
            }
          else if (g_list_length(existing_features) > 1)
            {
              /*! \todo More than one feature that we could overwrite: pop up dialog listing them and
               * asking which (if any) the user wants to overwrite. Not sure it's worth doing
               * this so just create a new feature for now. */
            }
        }
    }

  if (result)
    {
      /* Now create the new feature */
      zmapViewMergeNewFeature(view, merge->feature, merge->feature_set) ;

      /* Save the new feature to the annotation column (so the temp feature is updated with the
       * same info as the new feature we've created). This takes ownership of merge->feature. */
      zmapViewScratchSave(view, merge->feature) ;

    }

  return result ;
}


/* Sends a message to our peer that all features are now loaded. */
static void sendViewLoaded(ZMapView zmap_view, LoadFeaturesData loaded_features)
{
  static ZMapXMLUtilsEventStackStruct
    viewloaded[] = {{ZMAPXML_START_ELEMENT_EVENT, "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "names",      ZMAPXML_EVENT_DATA_STRING,   {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "start",      ZMAPXML_EVENT_DATA_INTEGER,   {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "end",      ZMAPXML_EVENT_DATA_INTEGER,   {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   "featureset", ZMAPXML_EVENT_DATA_NONE,    {0}},

                    {ZMAPXML_START_ELEMENT_EVENT, "status", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "value",      ZMAPXML_EVENT_DATA_INTEGER,   {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "features_loaded",      ZMAPXML_EVENT_DATA_INTEGER,   {0}},

                    {ZMAPXML_START_ELEMENT_EVENT, ZACP_MESSAGE,   ZMAPXML_EVENT_DATA_NONE,  {0}},
                    {ZMAPXML_CHAR_DATA_EVENT,     NULL,    ZMAPXML_EVENT_DATA_STRING, {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   ZACP_MESSAGE,      ZMAPXML_EVENT_DATA_NONE,  {0}},

                    {ZMAPXML_END_ELEMENT_EVENT,   "status", ZMAPXML_EVENT_DATA_NONE,    {0}},

                    {ZMAPXML_START_ELEMENT_EVENT, "exit_code", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "value",      ZMAPXML_EVENT_DATA_INTEGER,   {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   "exit_code", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_START_ELEMENT_EVENT, "stderr", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_ATTRIBUTE_EVENT,     "value",      ZMAPXML_EVENT_DATA_STRING,   {0}},
                    {ZMAPXML_END_ELEMENT_EVENT,   "stderr", ZMAPXML_EVENT_DATA_NONE,    {0}},
                    {ZMAPXML_NULL_EVENT}} ;

  if (zmap_view->remote_control)
    {
      if (!loaded_features || !(loaded_features->feature_sets))
        {
          zMapLogCritical("%s", "Data Load notification received but no datasets specified.") ;
        }
      else
        {
          GList *features;
          char *featurelist = NULL;
          char *f ;
          char *emsg = NULL ;
          char *ok_mess = NULL ;
          int i ;

          for (features = loaded_features->feature_sets ; features ; features = features->next)
            {
              char *prev ;

              f = (char *) g_quark_to_string(GPOINTER_TO_UINT(features->data)) ;
              prev = featurelist ;

              if (!prev)
                featurelist = g_strdup(f) ;
              else
                featurelist = g_strjoin(";", prev, f, NULL) ;

              g_free(prev) ;
            }


          /* THIS PART NEEDS FIXING UP TO RETURN THE TRUE ERROR MESSAGE AS PER rt 369227 */
          if (loaded_features->status)          /* see comment in zmapSlave.c/ RETURNCODE_QUIT, we are tied up in knots */
            {
              ok_mess = g_strdup_printf("%d features loaded", loaded_features->merge_stats.features_added) ;
              emsg = html_quote_string(ok_mess) ;                /* see comment about really free() below */
              g_free(ok_mess) ;

              {
                static long total = 0;

                total += loaded_features->merge_stats.features_added ;

                zMapLogTime(TIMER_LOAD,TIMER_ELAPSED,total,""); /* how long is startup... */
              }
            }
          else
            {
              emsg = html_quote_string(loaded_features->err_msg ? loaded_features->err_msg  : "");
            }



          if (loaded_features->stderr_out)
            {
              gchar *old = loaded_features->stderr_out;
              loaded_features->stderr_out =  html_quote_string(old);
              g_free(old);
            }

          i = 1 ;
          viewloaded[i].value.s = featurelist ;

          i++ ;
          viewloaded[i].value.i = loaded_features->start ;

          i++ ;
          viewloaded[i].value.i = loaded_features->end ;

          i += 3 ;
          viewloaded[i].value.i = (int)loaded_features->status ;

          i++ ;
          viewloaded[i].value.i = loaded_features->merge_stats.features_added ;

          i += 2 ;
          viewloaded[i].value.s = emsg ;

          i += 4 ;
          viewloaded[i].value.i = loaded_features->exit_code ;

          i += 3 ;
          viewloaded[i].value.s = loaded_features->stderr_out ? loaded_features->stderr_out : "" ;


          /* Send request to peer program. */
          /* NOTE WELL....gives a pointer to a static struct in this function, not ideal but will
           * have to do for now. */
          (*(view_cbs_G->remote_request_func))(view_cbs_G->remote_request_func_data,
                                               zmap_view,
                                               ZACP_FEATURES_LOADED, &viewloaded[0],
                                               localProcessReplyFunc, zmap_view,
                                               remoteReplyErrHandler, zmap_view) ;

          free(emsg);                                           /* yes really free() not g_free()-> see zmapUrlUtils.c */


          zMapLogWarning("VIEW LOADED FEATURE LIST: %s !!", featurelist) ;

          g_free(featurelist);
        }
    }

  return ;
}




static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window)
{
  ZMapViewWindow view_window ;

  view_window = g_new0(ZMapViewWindowStruct, 1) ;
  view_window->parent_view = zmap_view ;                    /* back pointer. */
  view_window->window = window ;

  return view_window ;
}


static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window)
{
  zmap_view->window_list = g_list_remove(zmap_view->window_list, view_window) ;

  zMapWindowDestroy(view_window->window) ;

  g_free(view_window) ;

  return ;
}





static gboolean zMapViewSortExons(ZMapFeatureContext diff_context)
{
  zMapFeatureContextExecute((ZMapFeatureAny) diff_context,
                            ZMAPFEATURE_STRUCT_FEATURESET,
                            zMapFeatureContextTranscriptSortExons,
                            NULL);

  return(TRUE);
}




static ZMapFeatureContextExecuteStatus add_default_styles(GQuark key,
                                                         gpointer data,
                                                         gpointer user_data,
                                                         char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapView view = (ZMapView) user_data;
  ZMapFeatureTypeStyle style;
  ZMapFeatureColumn f_col;

  zMapReturnValIfFail((feature_any && zMapFeatureIsValid(feature_any)), ZMAP_CONTEXT_EXEC_STATUS_ERROR) ;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_ALIGN:
      {
        ZMapFeatureAlignment feature_align ;
        feature_align = (ZMapFeatureAlignment)feature_any;
      }
      break;
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
      }
      break;

    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
                ZMapFeatureSet feature_set = NULL;
                feature_set = (ZMapFeatureSet)feature_any;

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
                        if(!g_hash_table_lookup(view->context_map.column_2_styles,GUINT_TO_POINTER(feature_set->unique_id)))
                        {
                                /* createColumnFull() needs a style table, although the error is buried in zmapWindowUtils.c */
                                zMap_g_hashlist_insert(view->context_map.column_2_styles,
                                        feature_set->unique_id,     // the column
                                        GUINT_TO_POINTER(style->unique_id)) ;  // the style
                        }

#if 0
/* cretaed by zMapFeatureGetSetColumn() below */
                        if (!(set_data = g_hash_table_lookup(view->context_map->featureset_2_column,GUINT_TO_POINTER(feature_set_id))))
                        {
                                // to handle autoconfigured servers we have to make this up
                                set_data = g_new0(ZMapFeatureSetDescStruct,1);
                                set_data->column_id = feature_set_id;
                                set_data->column_ID = feature_set_id;
                                g_hash_table_insert(view->context_map->featureset_2_column,GUINT_TO_POINTER(feature_set_id), (gpointer) set_data);
                        }

#endif
                        /* find_or_create_column() needs f_col->style */
                        f_col = view->context_map.getSetColumn(feature_set->unique_id);
                        if(f_col)
                        {
                                if(!f_col->style)
                                        f_col->style = style;
                                if(!f_col->style_table)
                                        f_col->style_table = g_list_append(f_col->style_table, (gpointer) style);
                        }

                        /* source_2_sourcedata has been set up by GFF parser, which needed it. */
                }
        }
        break;

    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
        zMapWarnIfReached() ;
      break;
      }
    }

  return status;
}


/* Layer below wants us to execute a command. */
static void commandCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapView view = view_window->parent_view ;
  ZMapWindowCallbackCommandAny cmd_any = (ZMapWindowCallbackCommandAny)window_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *err_msg = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Rewrite to use a better interface.... */



  /* TEMP CODE...... */
  if (cmd_any->cmd == ZMAPWINDOW_CMD_COLFILTER)
    {
      gboolean status ;

      status = zmapViewPassCommandToAllWindows(view, window_data) ;
    }
  else
  switch (cmd_any->cmd)
    {
    case ZMAPWINDOW_CMD_SHOWALIGN:
      {
        ZMapWindowCallbackCommandAlign align_cmd = (ZMapWindowCallbackCommandAlign)cmd_any ;

        doBlixemCmd(view, align_cmd) ;

        break ;
      }
    case ZMAPWINDOW_CMD_GETFEATURES:
      {
        ZMapWindowCallbackGetFeatures get_data = (ZMapWindowCallbackGetFeatures)cmd_any ;
        int req_start = get_data->start;
        int req_end = get_data->end;

        if (zMapViewGetRevCompStatus(view))
          {
            int tmp;

            /* rev comp the request to get the right features, we request as fwd strand */

            zmapFeatureRevCompCoord(&req_start, view->features->parent_span.x1,view->features->parent_span.x2);
            zmapFeatureRevCompCoord(&req_end, view->features->parent_span.x1,view->features->parent_span.x2);

            tmp = req_start;
            req_start = req_end;
            req_end = tmp;
          }

        zmapViewLoadFeatures(view, get_data->block, get_data->feature_set_ids, NULL, NULL,
                             NULL, req_start, req_end, view->thread_fail_silent,
                             SOURCE_GROUP_DELAYED, TRUE, FALSE) ;        /* don't terminate, need to keep alive for blixem */

        break ;
      }


    case ZMAPWINDOW_CMD_REVERSECOMPLEMENT:
      {
        gboolean status ;

        /* NOTE, there is no need to signal the layer above that things are changing,
         * the layer above does the complement and handles all that. */
        if (!(status = zMapViewReverseComplement(view)))
          {
            zMapLogCritical("%s", "View Reverse Complement failed.") ;

            zMapWarning("%s", "View Reverse Complement failed.") ;
          }

        break ;
      }

    case ZMAPWINDOW_CMD_COPYTOSCRATCH:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchCopyFeatures(view, scratch_cmd->features, scratch_cmd->seq_start, scratch_cmd->seq_end, scratch_cmd->subpart, scratch_cmd->use_subfeature);
        break;
      }

    case ZMAPWINDOW_CMD_SETCDSSTART:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchSetCDS(view, scratch_cmd->features, scratch_cmd->seq_start, scratch_cmd->seq_end,
                              scratch_cmd->subpart, scratch_cmd->use_subfeature,
                              TRUE, FALSE);
        break;
      }
    case ZMAPWINDOW_CMD_SETCDSEND:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchSetCDS(view, scratch_cmd->features, scratch_cmd->seq_start, scratch_cmd->seq_end, 
                              scratch_cmd->subpart, scratch_cmd->use_subfeature,
                              FALSE, TRUE);
        break;
      }
    case ZMAPWINDOW_CMD_SETCDSRANGE:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchSetCDS(view, scratch_cmd->features, scratch_cmd->seq_start, scratch_cmd->seq_end,
                              scratch_cmd->subpart, scratch_cmd->use_subfeature,
                              TRUE, TRUE);
        break;
      }

    case ZMAPWINDOW_CMD_DELETEFROMSCRATCH:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchDeleteFeatures(view, scratch_cmd->features, scratch_cmd->seq_start, scratch_cmd->seq_end, scratch_cmd->subpart, scratch_cmd->use_subfeature);
        break;
      }

    case ZMAPWINDOW_CMD_CLEARSCRATCH:
      {
        zmapViewScratchClear(view);
        break;
      }

    case ZMAPWINDOW_CMD_UNDOSCRATCH:
      {
        zmapViewScratchUndo(view);
        break;
      }

    case ZMAPWINDOW_CMD_REDOSCRATCH:
      {
        zmapViewScratchRedo(view);
        break;
      }

    case ZMAPWINDOW_CMD_GETEVIDENCE:
      {
        ZMapWindowCallbackCommandScratch scratch_cmd = (ZMapWindowCallbackCommandScratch)cmd_any ;
        zmapViewScratchFeatureGetEvidence(view, scratch_cmd->evidence_cb, scratch_cmd->evidence_cb_data);
        break;
      }

    default:
      {
        zMapWarnIfReached() ;
        break ;
      }
    }


  return ;
}


/* Call blixem functions, note that if we fetch local sequences this is asynchronous and
 * results in a callback to processGetSeqRequests() which then does the zmapViewCallBlixem() call. */
static void doBlixemCmd(ZMapView view, ZMapWindowCallbackCommandAlign align_cmd)
{
  gboolean status ;
  GList *local_sequences = NULL ;


  if (align_cmd->homol_set == ZMAPWINDOW_ALIGNCMD_NONE
      || !(status = zmapViewBlixemLocalSequences(view, align_cmd->block, align_cmd->homol_type,
                                                 align_cmd->offset, align_cmd->cursor_position,
                                                 align_cmd->feature_set, &local_sequences)))
    {
      GPid blixem_pid ;

      if ((status = zmapViewCallBlixem(view, align_cmd->block,
                                       align_cmd->homol_type,
                                       align_cmd->offset,
                                       align_cmd->cursor_position,
                                       align_cmd->window_start, align_cmd->window_end,
                                       align_cmd->mark_start, align_cmd->mark_end, align_cmd->features_from_mark,
                                       align_cmd->homol_set,
                                         align_cmd->isSeq,
                                       align_cmd->features, align_cmd->feature_set, align_cmd->source, NULL,
                                       &blixem_pid, &(view->kill_blixems))))
        view->spawned_processes = g_list_append(view->spawned_processes, GINT_TO_POINTER(blixem_pid)) ;
    }
  else
    {
      if (!view->sequence_server)
        {
          zMapWarning("%s", "No sequence server was specified so cannot fetch raw sequences for blixem.") ;
        }
      else
        {
          ZMapNewDataSource view_con ;
          ZMapViewConnectionRequest request ;
          ZMapServerReqAny req_any ;
          ZMapConnectionData connect_data ;

          view_con = view->sequence_server ;

          if (!view_con) // || !view_con->sequence_server)
            {
              zMapWarning("%s", "Sequence server incorrectly specified in config file"
                          " so cannot fetch local sequences for blixem.") ;
            }
          else if ((connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_con)))
            {
              if (connect_data->step_list)
                {
                  zMapWarning("%s", "Sequence server is currently active"
                              " so cannot fetch local sequences for blixem.") ;
                }
              else
                {
                  zmapViewBusy(view, TRUE) ;

                  /* Create the step list that will be used to fetch the sequences. */
                  connect_data->step_list = zmapViewStepListCreate(NULL, processGetSeqRequests, NULL) ;
                  zmapViewStepListAddStep(connect_data->step_list, ZMAP_SERVERREQ_GETSEQUENCE,
                                          REQUEST_ONFAIL_CANCEL_STEPLIST) ;

                  /* Add the request to the step list. */
                  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSEQUENCE,
                                                    local_sequences, align_cmd) ;

                  request = zmapViewStepListAddServerReq(connect_data->step_list,
                                                         ZMAP_SERVERREQ_GETSEQUENCE, req_any, REQUEST_ONFAIL_CANCEL_STEPLIST) ;


                  /* Start the step list. */
                  zmapViewStepListIter(connect_data->step_list, view_con->thread->GetThread(), view) ;
                }
            }
        }
    }


  return ;
}


static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowVisibilityChange vis = (ZMapWindowVisibilityChange)window_data;

  /* signal our caller that something has changed. */
  (*(view_cbs_G->visibility_change))(view_window, view_window->parent_view->app_data, window_data) ;

  zMapWindowNavigatorDrawLocator(view_window->parent_view->navigator_window, vis->scrollable_top, vis->scrollable_bot);

  return;
}

/* When a window is split, we set the zoom status of all windows */
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  g_list_foreach(view_window->parent_view->window_list, setZoomStatus, NULL) ;

  return;
}

static void setZoomStatus(gpointer data, gpointer user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  return;
}



/* Trial code to get alignments from a file and create a context...... */
ZMapFeatureContext zmapViewCreateContext(ZMapView view, GList *feature_set_names, ZMapFeatureSet feature_set)
{
  ZMapFeatureContext context = NULL ;
  gboolean master = TRUE ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;

  ZMapFeatureSequenceMap sequence = view->view_sequence;

  context = zMapFeatureContextCreate(sequence->sequence, sequence->start, sequence->end, feature_set_names) ;

  /* Add the master alignment and block. */
  alignment = zMapFeatureAlignmentCreate(sequence->sequence, master) ; /* TRUE => master alignment. */

  zMapFeatureContextAddAlignment(context, alignment, master) ;

  block = zMapFeatureBlockCreate(sequence->sequence,
                                 sequence->start, sequence->end, ZMAPSTRAND_FORWARD,
                                 sequence->start, sequence->end, ZMAPSTRAND_FORWARD) ;

  zMapFeatureAlignmentAddBlock(alignment, block) ;

  if(feature_set)                /* this will be the strand separator dummy featureset */
        zMapFeatureBlockAddFeatureSet(block, feature_set);

  if (feature_set)
    zMapFeatureBlockAddFeatureSet(block, feature_set) ;

  return context ;
}








#ifdef NOT_REQUIRED_ATM

/*
 * Read list of sequence to server mappings (i.e. which sequences must be fetched from which
 * servers) from the zmap config file.
 */
static gboolean getSequenceServers(ZMapView zmap_view, char *config_str)
{
  gboolean result = FALSE ;
  ZMapConfig config ;
  ZMapConfigStanzaSet zmap_list = NULL ;
  ZMapConfigStanza zmap_stanza ;
  char *zmap_stanza_name = ZMAPSTANZA_APP_CONFIG ;
  ZMapConfigStanzaElementStruct zmap_elements[] = {{ZMAPSTANZA_APP_SEQUENCE_SERVERS, ZMAPCONFIG_STRING, {NULL}},
                                                   {NULL, -1, {NULL}}} ;


  if ((config = getConfigFromBufferOrFile(config_str)))
    {
      zmap_stanza = zMapConfigMakeStanza(zmap_stanza_name, zmap_elements) ;

      if (zMapConfigFindStanzas(config, zmap_stanza, &zmap_list))
        {
          ZMapConfigStanza next_zmap = NULL ;

          /* We only read the first of these stanzas.... */
          if ((next_zmap = zMapConfigGetNextStanza(zmap_list, next_zmap)) != NULL)
            {
              char *server_seq_str ;

              if ((server_seq_str
                   = zMapConfigGetElementString(next_zmap, ZMAPSTANZA_APP_SEQUENCE_SERVERS)))
                {
                  char *sequence, *server ;
                  ZMapViewSequence2Server seq_2_server ;
                  char *search_str ;
                  char *curr_pos = NULL ;

                  search_str = server_seq_str ;

                  while ((sequence = strtok_r(search_str, " ", &curr_pos)))
                    {
                      search_str = NULL ;
                      server = strtok_r(NULL, " ", &curr_pos) ;

                      seq_2_server = createSeq2Server(sequence, server) ;

                      zmap_view->sequence_2_server = g_list_append(zmap_view->sequence_2_server, seq_2_server) ;
                    }

                }
            }

          zMapConfigDeleteStanzaSet(zmap_list) ;                    /* Not needed anymore. */
        }

      zMapConfigDestroyStanza(zmap_stanza) ;

      zMapConfigIniDestroy(config) ;

      result = TRUE ;
    }

  return result ;
}


static void destroySeq2ServerCB(gpointer data, gpointer user_data_unused)
{
  ZMapViewSequence2Server seq_2_server = (ZMapViewSequence2Server)data ;

  destroySeq2Server(seq_2_server) ;

  return ;
}

static ZMapViewSequence2Server createSeq2Server(char *sequence, char *server)
{
  ZMapViewSequence2Server seq_2_server = NULL ;

  seq_2_server = g_new0(ZMapViewSequence2ServerStruct, 1) ;
  seq_2_server->sequence = g_strdup(sequence) ;
  seq_2_server->server = g_strdup(server) ;

  return seq_2_server ;
}


static void destroySeq2Server(ZMapViewSequence2Server seq_2_server)
{
  g_free(seq_2_server->sequence) ;
  g_free(seq_2_server->server) ;
  g_free(seq_2_server) ;

  return ;
}

/* Check to see if a sequence/server pair match any in the list mapping sequences to servers held
 * in the view. NOTE that if the sequence is not in the list at all then this function returns
 * TRUE as it is assumed by default that sequences can be fetched from any servers.
 * */
static gboolean checkSequenceToServerMatch(GList *seq_2_server, ZMapViewSequence2Server target_seq_server)
{
  gboolean result = FALSE ;
  GList *server_item ;

  /* If the sequence is in the list then check the server names. */
  if ((server_item = g_list_find_custom(seq_2_server, target_seq_server, findSequence)))
    {
      ZMapViewSequence2Server curr_seq_2_server = (ZMapViewSequence2Server)server_item->data ;

      if ((g_ascii_strcasecmp(curr_seq_2_server->server, target_seq_server->server) == 0))
        result = TRUE ;
    }
  else
    {
      /* Return TRUE if sequence not in list. */
      result = TRUE ;
    }

  return result ;
}


/* A GCompareFunc to check whether servers match in the sequence to server mapping. */
static gint findSequence(gconstpointer a, gconstpointer b)
{
  gint result = -1 ;
  ZMapViewSequence2Server seq_2_server = (ZMapViewSequence2Server)a ;
  ZMapViewSequence2Server target_fetch = (ZMapViewSequence2Server)b ;

  if ((g_ascii_strcasecmp(seq_2_server->sequence, target_fetch->sequence) == 0))
    result = 0 ;

  return result ;
}
#endif /* NOT_REQUIRED_ATM */







/* check whether there are live connections or not, return TRUE if there are, FALSE otherwise. */
static gboolean checkContinue(ZMapView zmap_view)
{
  gboolean connections = FALSE ;

  switch (zmap_view->state)
    {
    case ZMAPVIEW_INIT:       /* shouldn't be here! */
    case ZMAPVIEW_MAPPED:
    case ZMAPVIEW_LOADED:           /*delayed servers get cleared up, we do not need connections to run */

      {
        /* Nothing to do here I think.... */
        connections = TRUE ;
        break ;
      }
    case ZMAPVIEW_CONNECTING:
    case ZMAPVIEW_CONNECTED:
    case ZMAPVIEW_LOADING:
//    case ZMAPVIEW_LOADED:
      {
        /* Kill the view as all connections have died. */
        /* Threads have died because of their own errors....but the ZMap is not dying so
         * reset state to init and we should return an error here....(but how ?), we
         * should not be outputting messages....I think..... */
        /* need to reset here..... */

        /* Tricky coding here: we need to destroy the view _before_ we notify the layer above us
         * that the zmap_view has gone but we need to pass information from the view back, so we
         * keep a temporary record of certain parts of the view. */
        ZMapView zmap_view_ref = zmap_view ;
        void *app_data = zmap_view->app_data ;
        ZMapViewCallbackDestroyData destroy_data ;

        zMapWarning("%s", "Cannot show ZMap because server connections have all died or been cancelled.\n") ;
        zMapLogWarning("%s", "Cannot show ZMap because server connections have all died or been cancelled.") ;

        zmap_view->state = ZMAPVIEW_DYING ;


        /* OK...WE MAY NEED TO SIGNAL THAT VIEWS HAVE DIED HERE..... */
        /* Do we need to return what's been killed here ???? */
        killGUI(zmap_view) ;

        destroy_data = g_new(ZMapViewCallbackDestroyDataStruct, 1) ; /* Caller must free. */
        destroy_data->xwid = zmap_view->xwid ;

        destroyZMapView(&zmap_view) ;

        /* Signal layer above us that view has died. */
        (*(view_cbs_G->destroy))(zmap_view_ref, app_data, destroy_data) ;

        connections = FALSE ;

        break ;
      }
    case ZMAPVIEW_UPDATING:
      {
      /* zmap is ok but a  new column didn't come through */
      zmap_view->state = ZMAPVIEW_LOADED;

      /* Signal layer above us because the view has stopped loading. */
      (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;
      //printf("check continue thread died while updating\n");
      //zMapLogWarning("check continue thread died while updating",NULL);
      connections = TRUE ;

      break;
      }
    case ZMAPVIEW_RESETTING:
      {
        /* zmap is ok but user has reset it and all threads have died so we need to stop
         * checking.... */
        zmap_view->state = ZMAPVIEW_MAPPED;       /* ZMAPVIEW_INIT */

        /* Signal layer above us because the view has reset. */
        (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;

        connections = FALSE ;

        break ;
      }
    case ZMAPVIEW_DYING:
      {
        /* Tricky coding here: we need to destroy the view _before_ we notify the layer above us
         * that the zmap_view has gone but we need to pass information from the view back, so we
         * keep a temporary record of certain parts of the view. */
        ZMapView zmap_view_ref = zmap_view ;
        void *app_data = zmap_view->app_data ;
        ZMapViewCallbackDestroyData destroy_data ;

        destroy_data = g_new(ZMapViewCallbackDestroyDataStruct, 1) ; /* Caller must free. */
        destroy_data->xwid = zmap_view->xwid ;

        /* view was waiting for threads to die, now they have we can free everything. */
        destroyZMapView(&zmap_view) ;

        /* Signal layer above us that view has died. */
        (*(view_cbs_G->destroy))(zmap_view_ref, app_data, destroy_data) ;

        connections = FALSE ;

        break ;
      }
    default:
      {
        zMapLogFatalLogicErr("switch(), unknown value: %d", zmap_view->state) ;

        break ;
      }
    }

//if(!connections) printf("checkContinue returns FALSE\n");


  return connections ;
}








static void addPredefined(ZMapStyleTree &styles_tree, GHashTable **column_2_styles_inout)
{
  GHashTable *styles_hash ;
  GHashTable *f2s = *column_2_styles_inout ;

  styles_hash = zmapConfigIniGetDefaultStyles();                // zMapStyleGetAllPredefined() ;

  g_hash_table_foreach(styles_hash, styleCB, f2s) ;

  styles_tree.merge(styles_hash, ZMAPSTYLE_MERGE_PRESERVE) ;

  *column_2_styles_inout = f2s ;

  return ;
}

/* GHashListForeachFunc() to set up a feature_set/style hash. */
static void styleCB(gpointer key, gpointer data, gpointer user_data)
{
  GQuark key_id = GPOINTER_TO_UINT(key);
  GHashTable *ghash = (GHashTable *)user_data ;
  GQuark feature_set_id, feature_set_name_id;

  /* We _must_ canonicalise here. */
  feature_set_name_id = key_id ;

  feature_set_id = zMapStyleCreateID((char *)g_quark_to_string(feature_set_name_id)) ;

  zMap_g_hashlist_insert(ghash,
                         feature_set_id,
                         GUINT_TO_POINTER(feature_set_id)) ;

  return ;
}



#if DEBUG_CONTEXT_MAP

void print_source_2_sourcedata(char * str,GHashTable *data)
{
  GList *iter;
  ZMapFeatureSource gff_source;
  gpointer key, value;

  printf("\nsource_2_sourcedata %s:\n",str);

  zMap_g_hash_table_iter_init (&iter, data);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      gff_source = (ZMapFeatureSource) value;
      printf("%s = %s %s\n",g_quark_to_string(GPOINTER_TO_UINT(key)),
             g_quark_to_string(gff_source->source_id),
             g_quark_to_string(gff_source->style_id));
    }
}

void print_fset2col(char * str,GHashTable *data)
{
  GList *iter;
  ZMapFeatureSetDesc gff_set;
  gpointer key, value;

  printf("\nfset2col %s:\n",str);

  zMap_g_hash_table_iter_init (&iter, data);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      gff_set = (ZMapFeatureSetDesc) value;
      if(!g_strstr_len(g_quark_to_string(GPOINTER_TO_UINT(key)),-1,":"))
        zMapLogWarning("%s = %s %s %s \"%s\"",g_quark_to_string(GPOINTER_TO_UINT(key)),
               g_quark_to_string(gff_set->column_id),
               g_quark_to_string(gff_set->column_ID),
               g_quark_to_string(gff_set->feature_src_ID),
               gff_set->feature_set_text ? gff_set->feature_set_text : "");
    }
}

void print_col2fset(char * str,GHashTable *data)
{
  GList *iter;
  ZMapFeatureColumn column;
  gpointer key, value;

  printf("\ncol2fset %s:\n",str);

  zMap_g_hash_table_iter_init (&iter, data);
  while (zMap_g_hash_table_iter_next (&iter, &key, &value))
    {
      column = (ZMapFeatureColumn) value;
      if(column->featuresets_unique_ids)
              zMap_g_list_quark_print(column->featuresets_unique_ids, (char *) g_quark_to_string(column->unique_id), FALSE);
      else
              printf("%s: no featuresets\n",(char *) g_quark_to_string(column->unique_id));
    }
}

#endif


/* Receives peers reply to our "features_loaded" message. */
static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
                                  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                  gpointer reply_handler_func_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapView view = (ZMapView)reply_handler_func_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (!reply_ok)
    {
      zMapLogCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }
  else
    {
      if (command_rc != REMOTE_COMMAND_RC_OK)
        {
          zMapLogCritical("%s command to peer program returned %s: \"%s\".",
                          ZACP_FEATURES_LOADED, zMapRemoteCommandRC2Str(command_rc), reason) ;
        }
    }

  return ;
}


/*!
 * \brief Callback to update the column background for a given item
 *
 * Only does anything if the item is a featureset
 */
static ZMapFeatureContextExecuteStatus updateColumnBackgroundCB(GQuark key,
                                                                gpointer data,
                                                                gpointer user_data,
                                                                char **err_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK ;
  ZMapView view = (ZMapView)user_data ;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
        ZMapFeatureSet feature_set = (ZMapFeatureSet)feature_any;
        gboolean highlight_filtered_columns = zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS) ;
        GList *list_item = g_list_first(view->window_list);

        for ( ; list_item; list_item = g_list_next(list_item))
          {
            ZMapViewWindow view_window = (ZMapViewWindow)(list_item->data) ;
            zMapWindowUpdateColumnBackground(view_window->window, feature_set, highlight_filtered_columns);
          }

        break;
      }

    default:
      {
        /* nothing to do for most of it */
        break;
      }
    }

  return status ;
}


/*!
 * \brief Redraw the background for all columns in all windows in this view
 *
 * This can be used to set/clear highlighting after changing filtering options
 */
void zMapViewUpdateColumnBackground(ZMapView view)
{
  zMapFeatureContextExecute((ZMapFeatureAny)view->features,
                            ZMAPFEATURE_STRUCT_FEATURE,
                            updateColumnBackgroundCB,
                            view);

  return ;
}



static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* UNUSED AT THE MOMENT */

  ZMapView zmap_view = (ZMapView)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






  return ;
}

void getFilename(gpointer key, gpointer value, gpointer data)
{
  GQuark filename_quark = GPOINTER_TO_INT(key) ;
  GQuark *result = (GQuark*)data ;
  *result = filename_quark ;
}



/* Get the filename of the default file to use for the Save operation. Returns null if
 * it hasn't been set. */
const char* zMapViewGetSaveFile(ZMapView view, const ZMapViewExportType export_type, const gboolean use_input_file)
{
  const char *result = NULL ;
  zMapReturnValIfFail(view && view->view_sequence, result) ;

  result = g_quark_to_string(view->save_file[export_type]) ;

  /* If it hasn't been set yet, check if there was an input file and use that (but only if
   * the flag allows, and if there was one and only one input file) */
  if (!result && use_input_file)
    {
      switch (export_type)
        {
        case ZMAPVIEW_EXPORT_FEATURES:
          {
            /* The cached parsers give all of the input files */
            GHashTable *cached_parsers = view->view_sequence->cached_parsers ;

            if (g_hash_table_size(cached_parsers) == 1)
              {
                g_hash_table_foreach(cached_parsers, getFilename, &view->save_file[export_type]) ;
                result = g_quark_to_string(view->save_file[export_type]) ;
              }
            
            break ;
          }
      
        case ZMAPVIEW_EXPORT_CONFIG:
          {
            result = view->view_sequence->config_file ;
            break ;
          }

        case ZMAPVIEW_EXPORT_STYLES:
          {
            result = view->view_sequence->stylesfile ;
            break ;
          }

        default:
          {
            zMapWarnIfReached() ;
            break ;
          }
        }
    }

  return result ;
}


/* Set the default filename to use for the Save operation. */
void zMapViewSetSaveFile(ZMapView view, const ZMapViewExportType export_type, const char *filename)
{
  zMapReturnIfFail(view) ;
  view->save_file[export_type] = g_quark_from_string(filename) ;
  zMapViewSetFlag(view, ZMAPFLAG_SAVE_FEATURES, FALSE) ;
}


/* Export config or styles (as given by export_type) to the given file. If filename is null,
 * prompt the user for a filename. Returns true if successful, false if there was an error. */
gboolean zMapViewExportConfig(ZMapView view, 
                              const ZMapViewExportType export_type,
                              ZMapConfigIniContextUpdatePrefsFunc update_func,
                              char **filepath_inout, 
                              GError **error)
{
  gboolean result = FALSE ;
  gboolean ok = TRUE ;
  char *output_file = NULL ;
  char *input_file = NULL ;
  ZMapConfigIniFileType file_type ;
  ZMapConfigIniContext context = NULL ;

  if (export_type == ZMAPVIEW_EXPORT_CONFIG)
    {
      /* For config, we'll set up the export context with the same values as the input
       * context. This is because there are currently only a few config options that are editable
       * so we'll basically export the original config with just a few changes. */
      input_file = view->view_sequence->config_file ;
      file_type = ZMAPCONFIG_FILE_USER ;
    }
  else if (export_type == ZMAPVIEW_EXPORT_STYLES)
    {
      /* For styles, don't use the input file to set up the context because we can easily loop
       * through all styles and export their current values. If we were to use the input file we
       * would have to make sure duplicate style names have the same case, because loading
       * styles is case sensitive. */
      input_file = NULL; //view->view_sequence->stylesfile ;
      file_type = ZMAPCONFIG_FILE_STYLES ;
    }
  else
    {
      ok = FALSE ;
    }

  if (ok && filepath_inout && *filepath_inout)
    {
      output_file = g_strdup(*filepath_inout) ;
    }

  if (ok && !output_file)
    {
      output_file = zmapGUIFileChooser(NULL, "Configuration Export filename ?", NULL, "ini") ;

      if (!output_file)
        ok = FALSE ;
    }

  if (ok)
    {
      /* Read the context from the original input config file (if there was one - otherwise this
       * will return an empty context. Note that it will also include any system config files too.) */
      context = zMapConfigIniContextProvide(input_file, file_type) ;

      if (!context)
        ok = FALSE ;
    }

  if (ok)
    {
      /* Set the output file name in the context so we write over the correct file (otherwise 
       * it will write over the input file). This is either the 'user' file for the
       * config, or the 'extra' file for the styles. */
      zMapConfigIniContextSetFile(context, file_type, output_file) ;

      /* Create the key file, if it doesn't exist already */
      zMapConfigIniContextCreateKeyFile(context, file_type) ;

      /* Update the context with the new preferences or styles, if anything has changed */
      update_func(context, file_type, &view->context_map.styles) ;

      /* Update the context with any configuration values that have changed */
      if (export_type == ZMAPVIEW_EXPORT_CONFIG)
        zMapFeatureUpdateContext(&view->context_map, 
                                 view->view_sequence,
                                 (ZMapFeatureAny)view->features,
                                 context, 
                                 file_type) ;

      /* Do the save to file (force changes=true so we export even if nothing's changed) */
      zMapConfigIniContextSetUnsavedChanges(context, file_type, TRUE) ;
      result = zMapConfigIniContextSave(context, file_type) ;

      /* Destroy the context */
      zMapConfigIniContextDestroy(context) ;
      context = NULL ;
    }

  if (output_file && filepath_inout && !*filepath_inout)
    {
      /* We've created the filepath in this function so set the output value from it */
      *filepath_inout = output_file ;
    }
  else if (output_file)
    {
      /* Return value wasn't requested so free this */
      g_free(output_file) ;
    }

  return result ;
}


/* Returns true if the given style should be included when exporting column-style values */
static gboolean exportColumnStyle(const char *key_str, const char *value_str)
{
  gboolean result = TRUE ;

  /* Only export if value is different to key (otherwise the style name is the
   * same as the column name, and this stanza doesn't add anything). Also don't
   * export if it's a default style. */
  if (value_str != key_str &&
      strcmp(value_str, "invalid") &&
      strcmp(value_str, "basic") &&
      strcmp(value_str, "alignment") &&
      strcmp(value_str, "transcript") &&
      strcmp(value_str, "sequence") &&
      strcmp(value_str, "assembly-path") &&
      strcmp(value_str, "text") &&
      strcmp(value_str, "graph") &&
      strcmp(value_str, "glyph") &&
      strcmp(value_str, "plain") &&
      strcmp(value_str, "meta") &&
      strcmp(value_str, "search hit marker"))
    {
      result = FALSE ;
    }

  return result ;
}


/*  Update the given context with column-style relationships */
void updateContextColumnStyles(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, 
                               const char *stanza, GHashTable *ghash)
{
  zMapReturnIfFail(context && context->config) ;

  GKeyFile *gkf = zMapConfigIniGetKeyFile(context, file_type) ;

  if (gkf)
    {
      /* Loop through all entries in the hash table */
      GList *iter = NULL ;
      gpointer key = NULL,value = NULL;

      zMap_g_hash_table_iter_init(&iter, ghash) ;

      while(zMap_g_hash_table_iter_next(&iter, &key, &value))
        {
          const char *key_str = g_quark_to_string(GPOINTER_TO_INT(key)) ;

          if (key_str)
            {
              GString *values_str = NULL ;

              for (GList *item = (GList*)value ; item ; item = item->next) 
                {
                  const char *value_str = g_quark_to_string(GPOINTER_TO_INT(item->data)) ;

                  if (exportColumnStyle(key_str, value_str))
                    {
                      if (!values_str)
                        values_str = g_string_new(value_str) ;
                      else
                        g_string_append_printf(values_str, " ; %s", value_str) ;
                    }
                }

              if (values_str)
                {
                  g_key_file_set_string(gkf, stanza, key_str, values_str->str) ;
                  g_string_free(values_str, TRUE) ;
                }
            }
        }
    }
}


bool zMapViewGetDisablePopups(ZMapView zmap_view)
{
  return zmap_view->disable_popups ;
}

void zMapViewSetDisablePopups(ZMapView zmap_view, const bool value)
{
  zmap_view->disable_popups = value ;
}
