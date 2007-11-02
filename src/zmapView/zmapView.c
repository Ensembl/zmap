/*  File: zmapView.c
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
 * Description: 
 * Exported functions: See ZMap/zmapView.h
 * HISTORY:
 * Last edited: Nov  2 09:16 2007 (edgrif)
 * Created: Thu May 13 15:28:26 2004 (edgrif)
 * CVS info:   $Id: zmapView.c,v 1.125 2007-11-02 09:36:13 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <sys/types.h>
#include <signal.h>             /* kill() */
#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapServerProtocol.h>
#include <zmapView_P.h>


static ZMapView createZMapView(GtkWidget *xremote_widget, char *view_name,
			       GList *sequences, void *app_data) ;
static void destroyZMapView(ZMapView *zmap) ;

static gint zmapIdleCB(gpointer cb_data) ;
static void enterCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void leaveCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void scrollCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void focusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void commandCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void loaded_dataCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data);

static void setZoomStatus(gpointer data, gpointer user_data);
static void splitMagic(gpointer data, gpointer user_data);

static void startStateConnectionChecking(ZMapView zmap_view) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void stopStateConnectionChecking(ZMapView zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean checkStateConnections(ZMapView zmap_view) ;
static void loadDataConnections(ZMapView zmap_view) ;

static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;


static ZMapViewConnection createConnection(ZMapView zmap_view,
					   zMapURL url, char *format,
					   int timeout, char *version,
					   char *styles_file,
					   char *feature_sets, char *navigator_set_names,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end) ;
static void destroyConnection(ZMapViewConnection *view_conn) ;

static void resetWindows(ZMapView zmap_view) ;
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features, 
                               ZMapFeatureContext new_features,
                               gboolean undisplay) ;
static void killAllWindows(ZMapView zmap_view) ;

static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window) ;
static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window) ;

static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GList *string2StyleQuarks(char *feature_sets) ;
static gboolean nextIsQuoted(char **text) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static gboolean justMergeContext(ZMapView view, ZMapFeatureContext *context_inout);
static void justDrawContext(ZMapView view, ZMapFeatureContext diff_context);

static ZMapFeatureContext createContext(char *sequence, int start, int end,
					GData *types, GList *feature_set_names) ;
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget) ;

static void addAlignments(ZMapFeatureContext context) ;

static gboolean mergeAndDrawContext(ZMapView view, ZMapFeatureContext context_inout);
static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout);

static gboolean getSequenceServers(ZMapView zmap_view, char *config_str) ;
static void destroySeq2ServerCB(gpointer data, gpointer user_data_unused) ;
static ZMapViewSequence2Server createSeq2Server(char *sequence, char *server) ;
static void destroySeq2Server(ZMapViewSequence2Server seq_2_server) ;
static gboolean checkSequenceToServerMatch(GList *seq_2_server, ZMapViewSequence2Server target_seq_server) ;
static gint findSequence(gconstpointer a, gconstpointer b) ;

static void threadDebugMsg(ZMapThread thread, char *format_str, char *msg) ;

static void killAllSpawned(ZMapView zmap_view);

static ZMapConfig getConfigFromBufferOrFile(char *config_str);




/* These callback routines are global because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;


/* Callbacks back we set in the level below us, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G = 
{
  enterCB, leaveCB,
  scrollCB,
  focusCB, 
  viewSelectCB, 
  viewSplitToPatternCB,
  setZoomStatusCB,
  viewVisibilityChangeCB,
  commandCB,
  loaded_dataCB
} ;



/*
 *  ------------------- External functions -------------------
 */



/*! @defgroup zmapview   zMapView: feature context display/processing
 * @{
 * 
 * \brief  Feature Context View Handling.
 * 
 * zMapView routines receive requests to load, display and process
 * feature contexts. Each ZMapView corresponds to a single feature context.
 * 
 *
 *  */



/*!
 * This routine must be called just once before any other views routine,
 * the caller must supply all of the callback routines.
 * 
 * @param callbacks   Caller registers callback functions that view will call
 *                    from the appropriate actions.
 * @return <nothing>
 *  */
void zMapViewInit(ZMapViewCallbacks callbacks)
{
  zMapAssert(!view_cbs_G) ;

  zMapAssert(callbacks
	     && callbacks->enter && callbacks->leave
	     && callbacks->load_data && callbacks->focus && callbacks->select
	     && callbacks->visibility_change && callbacks->state_change && callbacks->destroy) ;

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


  /* Init windows.... */
  zMapWindowInit(&window_cbs_G) ;

  return ;
}



/*!
 * Create a "view", this is the holder for a single feature context. The view may use
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
ZMapViewWindow zMapViewCreate(GtkWidget *xremote_widget, GtkWidget *view_container,
			      char *sequence, int start, int end,
			      void *app_data)
{
  ZMapViewWindow view_window = NULL ;
  ZMapView zmap_view = NULL ;
  gboolean debug ;
  ZMapViewSequenceMap sequence_fetch ;
  GList *sequences_list = NULL ;
  char *view_name ;

  /* No callbacks, then no view creation. */
  zMapAssert(view_cbs_G && GTK_IS_WIDGET(view_container) && sequence && start > 0 && (end == 0 || end >= start)) ;

  /* Set up debugging for threads, we do it here so that user can change setting in config file
   * and next time they create a view the debugging will go on/off. */
  if (zMapUtilsConfigDebug(ZMAPTHREAD_CONFIG_DEBUG_STR, &debug))
    zmap_thread_debug_G = debug ;

  /* Set up sequence to be fetched, in this case server defaults to whatever is set in config. file. */
  sequence_fetch = g_new0(ZMapViewSequenceMapStruct, 1) ;
  sequence_fetch->sequence = g_strdup(sequence) ;
  sequence_fetch->start = start ;
  sequence_fetch->end = end ;
  sequences_list = g_list_append(sequences_list, sequence_fetch) ;

  view_name = sequence ;

  zmap_view = createZMapView(xremote_widget, view_name, sequences_list, app_data) ; /* N.B. this step can't fail. */

  zmap_view->state = ZMAPVIEW_INIT ;

  view_window = addWindow(zmap_view, view_container) ;

  zmap_view->cwh_hash = zmapViewCWHHashCreate();

  return view_window ;
}



void zMapViewSetupNavigator(ZMapViewWindow view_window, GtkWidget *canvas_widget)
{
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmap_view->navigator_window = zMapWindowNavigatorCreate(canvas_widget);
      zMapWindowNavigatorSetCurrentWindow(zmap_view->navigator_window, view_window->window);
    }

  return ;
}



/* Connect a View to its databases via threads, at this point the View is blank and waiting
 * to be called to load some data. */
gboolean zMapViewConnect(ZMapView zmap_view, char *config_str)
{
  gboolean result = TRUE ;
  ZMapConfigStanzaSet server_list = NULL ;


  if (zmap_view->state != ZMAPVIEW_INIT)
    {
      /* Probably we should indicate to caller what the problem was here....
       * e.g. if we are resetting then say we are resetting etc.....again we need g_error here. */
      result = FALSE ;
    }
  else
    {
      ZMapConfig config ;
      char *config_file = NULL ;

      zMapStartTimer(ZMAP_GLOBAL_TIMER) ;
      zMapPrintTimer(NULL, "Open connection") ;

      zmapViewBusy(zmap_view, TRUE) ;

      /* There is some redundancy of state here as the below code actually does a connect
       * and load in one call but we will almost certainly need the extra states later... */
      zmap_view->state = ZMAPVIEW_CONNECTING ;

      config_file = zMapConfigDirGetFile() ;
      if ((config = getConfigFromBufferOrFile(config_str)))
	{
	  ZMapConfigStanza server_stanza ;
	  ZMapConfigStanzaElementStruct server_elements[] = {{ZMAPSTANZA_SOURCE_URL,     ZMAPCONFIG_STRING, {NULL}},
							     {ZMAPSTANZA_SOURCE_TIMEOUT, ZMAPCONFIG_INT,    {NULL}},
							     {ZMAPSTANZA_SOURCE_VERSION, ZMAPCONFIG_STRING, {NULL}},
							     {ZMAPSTANZA_SOURCE_STYLE,  ZMAPCONFIG_BOOL, {NULL}},
							     {"sequence", ZMAPCONFIG_BOOL, {NULL}},
							     {"writeback", ZMAPCONFIG_BOOL, {NULL}},
							     {"stylesfile", ZMAPCONFIG_STRING, {NULL}},
							     {"format", ZMAPCONFIG_STRING, {NULL}},
							     {ZMAPSTANZA_SOURCE_FEATURESETS, ZMAPCONFIG_STRING, {NULL}},
                                                             {"navigator_sets", ZMAPCONFIG_STRING, {NULL}},
							     {NULL, -1, {NULL}}} ;

	  /* Set defaults for any element that is not a string. */
	  zMapConfigGetStructInt(server_elements, ZMAPSTANZA_SOURCE_TIMEOUT) = 120 ; /* seconds. */
	  zMapConfigGetStructBool(server_elements, ZMAPSTANZA_SOURCE_STYLE) = FALSE ;
	  zMapConfigGetStructBool(server_elements, "sequence") = FALSE ;
	  zMapConfigGetStructBool(server_elements, "writeback") = FALSE ;

	  server_stanza = zMapConfigMakeStanza(ZMAPSTANZA_SOURCE_CONFIG, server_elements) ;

	  if (!zMapConfigFindStanzas(config, server_stanza, &server_list))
	    result = FALSE ;

	  zMapConfigDestroyStanza(server_stanza) ;
	  server_stanza = NULL ;
	}


      /* Set up connections to the named servers. */
      if (result && config && server_list)
	{
	  int connections = 0 ;
	  ZMapConfigStanza next_server ;

	  /* Current error handling policy is to connect to servers that we can and
	   * report errors for those where we fail but to carry on and set up the ZMap
	   * as long as at least one connection succeeds. */
	  next_server = NULL ;
	  while (result
		 && ((next_server = zMapConfigGetNextStanza(server_list, next_server)) != NULL))
	    {
	      char *version, *styles_file, *format, *url, *featuresets, *navigatorsets ;
	      int timeout, url_parse_error ;
              zMapURL urlObj;
	      gboolean sequence_server, writeback_server, acedb_styles ;
	      ZMapViewConnection view_con ;
	      ZMapViewSequence2ServerStruct tmp_seq = {NULL} ;

	      url     = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_URL) ;
	      format  = zMapConfigGetElementString(next_server, "format") ;
	      timeout = zMapConfigGetElementInt(next_server, ZMAPSTANZA_SOURCE_TIMEOUT) ;
	      version = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_VERSION) ;
	      styles_file = zMapConfigGetElementString(next_server, "stylesfile") ;
	      featuresets = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_FEATURESETS) ;
              navigatorsets = zMapConfigGetElementString(next_server, "navigator_sets");
	      acedb_styles = zMapConfigGetElementBool(next_server, ZMAPSTANZA_SOURCE_STYLE) ;

	      tmp_seq.sequence = zmap_view->sequence ;
	      tmp_seq.server = url ;

              if (!url)
		{
		  /* url is absolutely required. Go on to next stanza if there isn't one.
		   * Done before anything else so as not to set seq/write servers to invalid locations  */

		  zMapLogWarning("GUI: %s", "computer says no url specified") ;
		  continue ;
		}
	      else if (!checkSequenceToServerMatch(zmap_view->sequence_2_server, &tmp_seq))
		{
		  /* If certain sequences must only be fetched from certain servers then make sure
		   * we only make those connections. */

		  continue ;
		}


	      /* We only record the first sequence and writeback servers found, this means you
	       * can only have one each of these which seems sensible. */
	      if (!zmap_view->sequence_server)
		sequence_server = zMapConfigGetElementBool(next_server, "sequence") ;
	      if (!zmap_view->writeback_server)
		writeback_server = zMapConfigGetElementBool(next_server, "writeback") ;

              /* Parse the url, only here if there is a url to parse */
              urlObj = url_parse(url, &url_parse_error);
              if (!urlObj)
                {
                  zMapLogWarning("GUI: url %s did not parse. Parse error < %s >\n",
                                 url,
                                 url_error(url_parse_error)) ;
                }
              else if ((view_con = createConnection(zmap_view, urlObj,
						    format,
						    timeout, version,
						    styles_file,
						    featuresets, navigatorsets,
						    sequence_server, writeback_server,
						    zmap_view->sequence,
						    zmap_view->start, zmap_view->end)))
		{
		  /* Update now we have successfully created a connection. */
		  zmap_view->connection_list = g_list_append(zmap_view->connection_list, view_con) ;
		  connections++ ;

		  if (sequence_server)
		    zmap_view->sequence_server = view_con ;
		  if (writeback_server)
		    zmap_view->writeback_server = view_con ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

		  /* FEATURE AUGMENT NEEDS REWRITING TO USE LISTS, NOT DATA_LISTS */
		  if (view_con->types
		      && !zMapFeatureTypeSetAugment(&(zmap_view->types), &(view_con->types)))
		    zMapLogCritical("Could not merge types for server %s into existing types.", 
                                    urlObj->host) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		}
	      else
		{
                  zMapLogWarning("GUI: url %s looks ok, host: %s\nport: %d....\n",
                                 urlObj->url, 
                                 urlObj->host, 
                                 urlObj->port) ; 
		  zMapLogWarning("Could not connect to server on %s, port %d", 
                                 urlObj->host, 
                                 urlObj->port) ;
		}
	    }

	  /* Ought to return a gerror here........ */
	  if (!connections)
	    result = FALSE ;

	}
      else
        result = FALSE;

      /* clean up. */
      if (server_list)
	zMapConfigDeleteStanzaSet(server_list) ;
      if (config)
	zMapConfigDestroy(config) ;



      /* If at least one connection succeeded then we are up and running, if not then the zmap
       * returns to the init state. */
      if (result)
	{
	  zmap_view->state = ZMAPVIEW_LOADING ;

	  /* Start polling function that checks state of this view and its connections, note
	   * that at this stage there is no data to display. */
	  startStateConnectionChecking(zmap_view) ;
	}
      else
	{
	  zmap_view->state = ZMAPVIEW_INIT ;
	}

    }

  return result ;
}


/* Is this ever called..... */
/* Signal threads that we want data to stick into the ZMap */
gboolean zMapViewLoad(ZMapView zmap_view)
{
  gboolean result = FALSE ;

  /* Perhaps we should return a "wrong state" warning here.... */
  if (zmap_view->state == ZMAPVIEW_CONNECTED)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      loadDataConnections(zmap_view) ;

      result = TRUE ;
    }

  return result ;
}


/* Copies an existing window in a view.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				  ZMapWindow copy_window, ZMapWindowLockType window_locking)
{
  ZMapViewWindow view_window = NULL ;


  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      /* the view _must_ already have a window _and_ data. */
      zMapAssert(zmap_view && parent_widget && zmap_view->window_list
		 && zmap_view->state == ZMAPVIEW_LOADED) ;

      view_window = createWindow(zmap_view, NULL) ;

      zmapViewBusy(zmap_view, TRUE) ;

      if (!(view_window->window = zMapWindowCopy(parent_widget, zmap_view->sequence,
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

      zmapViewBusy(zmap_view, FALSE) ;
    }

  return view_window ;
}


/* Returns number of windows for current view. */
int zMapViewNumWindows(ZMapViewWindow view_window)
{
  int num_windows = 0 ;
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->window_list)
    {
      num_windows = g_list_length(zmap_view->window_list) ;
    }

  return num_windows ;
}


/* Removes a window from a view, the last window cannot be removed, the view must
 * always have at least one window. The function does nothing if there is only one
 * window. */
void zMapViewRemoveWindow(ZMapViewWindow view_window)
{
  ZMapView zmap_view ;

  zMapAssert(view_window) ;

  zmap_view = view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      if (g_list_length(zmap_view->window_list) > 1)
	{
	  /* We should check the window is in the list of windows for that view and abort if
	   * its not........ */
	  zMapWindowBusy(view_window->window, TRUE) ;

	  destroyWindow(zmap_view, view_window) ;

	  /* no need to reset cursor here...the window is gone... */
	}
    }

  return ;
}

/* This is a big hack, but a worthwhile one for the moment.
 * Only zmapControlRemote.c uses it. See there for why..... */
ZMapFeatureContext zMapViewGetContextAsEmptyCopy(ZMapView do_not_use)
{
  ZMapFeatureContext context = NULL ;
  ZMapView view = do_not_use;

  if (view->state != ZMAPVIEW_DYING)
    {
      if(view->features)
	{
	  context = zMapFeatureContextCreateEmptyCopy(view->features);
	  if(context && view->revcomped_features)
	    zMapFeatureReverseComplement(context);
	}
    }

  return context;
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
    /* should replace_me be a view or a view_window???? */
    eraseAndUndrawContext(replace_me, context_inout);
    
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
ZMapFeatureContext zmapViewMergeInContext(ZMapView view, ZMapFeatureContext context)
{
  if (view->state != ZMAPVIEW_DYING)
    justMergeContext(view, &context);

  return context;
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
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context)
{
  gboolean context_freed = TRUE;

  if (view->state != ZMAPVIEW_DYING)
    {
      if(view->features == *diff_context)
        context_freed = FALSE;

      justDrawContext(view, *diff_context);
    }
  else
    {
      context_freed = TRUE;
      zMapFeatureContextDestroy(*diff_context, context_freed);
    }

  if(context_freed)
    *diff_context = NULL;

  return context_freed;
}

/* Force a redraw of all the windows in a view, may be reuqired if it looks like
 * drawing has got out of whack due to an overloaded network etc etc. */
void zMapViewRedraw(ZMapViewWindow view_window)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapAssert(view) ;

  if (view->state == ZMAPVIEW_LOADED)
    {
      list_item = g_list_first(view->window_list) ;
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowRedraw(view_window->window) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}


/* Show stats for this view. */
void zMapViewStats(ZMapViewWindow view_window)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapAssert(view) ;

  if (view->state == ZMAPVIEW_LOADED)
    {
      ZMapViewWindow view_window ;

      list_item = g_list_first(view->window_list) ;
      view_window = list_item->data ;

      zMapWindowStats(view_window->window) ;
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
  gboolean result = TRUE ;

  if (zmap_view->state != ZMAPVIEW_LOADED)
    result = FALSE ;
  else
    {
      GList* list_item ;

      zmapViewBusy(zmap_view, TRUE) ;

      /* Call the feature code that will do the revcomp. */
      zMapFeatureReverseComplement(zmap_view->features) ;

      /* Set our record of reverse complementing. */
      zmap_view->revcomped_features = !(zmap_view->revcomped_features) ;

      zMapWindowNavigatorSetStrand(zmap_view->navigator_window, zmap_view->revcomped_features);
      zMapWindowNavigatorReset(zmap_view->navigator_window);
      zMapWindowNavigatorDrawFeatures(zmap_view->navigator_window, zmap_view->features);

      list_item = g_list_first(zmap_view->window_list) ;
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowFeatureRedraw(view_window->window,
				  zmap_view->features, TRUE) ;
	}
      while ((list_item = g_list_next(list_item))) ;

      /* Not sure if we need to do this or not.... */
      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;
    }

  return TRUE ;
}

/* Return which strand we are showing viz-a-viz reverse complementing. */
gboolean zMapViewGetRevCompStatus(ZMapView zmap_view)
{
  return zmap_view->revcomped_features ;
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

      zmap_view->connections_loaded = 0 ;

      result = TRUE ;
    }

  return result ;
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

	      view_window = list_item->data ;

	      zMapWindowZoom(view_window->window, zoom) ;
	    }
	  while ((list_item = g_list_next(list_item))) ;
	}
    }

  return ;
}

void zMapViewHighlightFeatures(ZMapView view, ZMapViewWindow view_window, ZMapFeatureContext context, gboolean multiple)
{
  GList *list;

  if (view->state == ZMAPVIEW_LOADED)
    {
      if (view_window)
	{
	  zMapLogWarning("%s", "What were you thinking");
	}
      else
	{
	  list = g_list_first(view->window_list);
	  do
	    {
	      view_window = list->data;
	      zMapWindowHighlightObjects(view_window->window, context, multiple);
	    }
	  while((list = g_list_next(list)));
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
    sequence = zmap_view->sequence ;

  return sequence ;
}

ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view)
{
  ZMapFeatureContext features = NULL ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    features = zmap_view->features ;

  return features ;
}

GData *zMapViewGetStyles(ZMapViewWindow view_window)
{
  GData *styles = NULL ;
  ZMapView view = zMapViewGetView(view_window);
  ZMapFeatureContext context;
  
  if(view->state != ZMAPVIEW_DYING && (context = zMapViewGetFeatures(view)))
    styles = context->styles;

  return styles;
}

gboolean zMapViewGetFeaturesSpan(ZMapView zmap_view, int *start, int *end)
{
  gboolean result = FALSE ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->features)
    {
      *start = zmap_view->features->sequence_to_parent.c1 ;
      *end = zmap_view->features->sequence_to_parent.c2 ;

      result = TRUE ;
    }

  return result ;
}


/* N.B. we don't exclude ZMAPVIEW_DYING because caller may want to know that ! */
ZMapViewState zMapViewGetStatus(ZMapView zmap_view)
{
  return zmap_view->state ;
}

char *zMapViewGetStatusStr(ZMapViewState state)
{
  /* Array must be kept in synch with ZmapState enum in zmapView.h */
  static char *zmapStates[] = {"",
			       "Connecting", "Connected",
			       "Data loading", "Data loaded",
			       "Resetting", "Dying"} ;
  char *state_str ;

  zMapAssert(state >= ZMAPVIEW_INIT && state <= ZMAPVIEW_DYING) ;

  state_str = zmapStates[state] ;

  return state_str ;
}


ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window)
{
  ZMapWindow window = NULL ;

  zMapAssert(view_window) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    window = view_window->window ;

  return window ;
}

ZMapWindowNavigator zMapViewGetNavigator(ZMapView view)
{
  ZMapWindowNavigator navigator = NULL ;

  zMapAssert(view) ;

  if (view->state != ZMAPVIEW_DYING)
    navigator = view->navigator_window ;

  return navigator ;
}


GList *zMapViewGetWindowList(ZMapViewWindow view_window)
{
  zMapAssert(view_window);

  return view_window->parent_view->window_list;
}


void zMapViewSetWindowList(ZMapViewWindow view_window, GList *list)
{
  zMapAssert(view_window);
  zMapAssert(list);

  view_window->parent_view->window_list = list;

  return;
}


ZMapView zMapViewGetView(ZMapViewWindow view_window)
{
  ZMapView view = NULL ;

  zMapAssert(view_window) ;

  if (view_window->parent_view->state != ZMAPVIEW_DYING)
    view = view_window->parent_view ;

  return view ;
}

void zMapViewReadConfigBuffer(ZMapView zmap_view, char *buffer)
{
  /* designed to add extra config bits to an already created view... */
  /* This is probably a bit of a hack, why can't we make the zMapViewConnect do it?  */

  getSequenceServers(zmap_view, buffer);

  return ;
}

void zmapViewFeatureDump(ZMapViewWindow view_window, char *file)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapFeatureDump(view_window->parent_view->features, file) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  printf("reimplement.......\n") ;

  return;
}


/* Called to kill a view and get the associated threads killed, note that
 * this call just signals everything to die, its the checkConnections() routine
 * that really clears up and when everything has died signals the caller via the
 * callback routine that they supplied when the view was created.
 *
 * NOTE: if the function returns FALSE it means the view has signalled its threads
 * and is waiting for them to die, the caller should thus wait until view signals
 * via the killedcallback that the view has really died before doing final clear up.
 * 
 * If the function returns TRUE it means that the view has been killed immediately
 * because it had no threads so the caller can clear up immediately.
 */
gboolean zMapViewDestroy(ZMapView zmap_view)
{
  gboolean killed_immediately = TRUE ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      /* All states have GUI components which need to be destroyed. */
      killGUI(zmap_view) ;

      if (zmap_view->state == ZMAPVIEW_INIT)
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

	      killed_immediately = FALSE ;
	    }

	  /* Must set this as this will prevent any further interaction with the ZMap as
	   * a result of both the ZMap window and the threads dying asynchronously.  */
	  zmap_view->state = ZMAPVIEW_DYING ;
	}
    }

  return killed_immediately ;
}


/*! @} end of zmapview docs. */



/* Functions internal to zmapView package. */



char *zmapViewGetStatusAsStr(ZMapViewState state)
{
  /* Array must be kept in synch with ZmapState enum in ZMap.h */
  static char *zmapStates[] = {"ZMAPVIEW_INIT",
			       "ZMAPVIEW_NOT_CONNECTED", "ZMAPVIEW_NO_WINDOW",
			       "ZMAPVIEW_CONNECTING", "ZMAPVIEW_CONNECTED",
			       "ZMAPVIEW_LOADING", "ZMAPVIEW_LOADED",
			       "ZMAPVIEW_RESETTING", "ZMAPVIEW_DYING"} ;
  char *state_str ;

  zMapAssert(state >= ZMAPVIEW_INIT && state <= ZMAPVIEW_DYING) ;

  state_str = zmapStates[state] ;

  return state_str ;
}






/*
 *  ------------------- Internal functions -------------------
 */



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


  return ;
}


/* Called when a sequence window has been focussed, usually by user actions. */
static void focusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->focus))(view_window, view_window->parent_view->app_data, NULL) ;

  {
    double x1, x2, y1, y2;
    x1 = x2 = y1 = y2 = 0.0;
    /* zero the input so that nothing changes */
    zMapWindowNavigatorFocus(view_window->parent_view->navigator_window, TRUE,
                             &x1, &y1, &x2, &y2);
  }

  return ;
}


/* Called when some sequence window feature (e.g. column, actual feature etc.)
 * has been selected. */
static void viewSelectCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowSelect window_select = (ZMapWindowSelect)window_data ;
  ZMapViewSelectStruct view_select = {0} ;

  /* Check we've got a window_select! */
  if(!window_select)
    return ;                    /* !!! RETURN !!! */

  if((view_select.type = window_select->type) == ZMAPWINDOW_SELECT_SINGLE)
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

	      view_window = list_item->data ;

	      if ((item = zMapWindowFindFeatureItemByItem(view_window->window, window_select->highlight_item)))
		zMapWindowHighlightObject(view_window->window, item,
					  window_select->replace_highlight_item,
					  window_select->highlight_same_names) ;
	    }
	  while ((list_item = g_list_next(list_item))) ;
	}


      view_select.feature_desc   = window_select->feature_desc ;
      view_select.secondary_text = window_select->secondary_text ;

    }


  view_select.xml_handler = window_select->xml_handler ;    /* n.b. struct copy. */
  
  if(window_select->xml_handler.zmap_action)
    {
      view_select.xml_handler.handled =
        window_select->xml_handler.handled = zmapViewRemoteSendCommand(view_window->parent_view,
                                                                       window_select->xml_handler.zmap_action,
                                                                       window_select->xml_handler.xml_events,
                                                                       window_select->xml_handler.start_handlers,
                                                                       window_select->xml_handler.end_handlers,
                                                                       window_select->xml_handler.handler_data);        
    }


  /* Pass back a ZMapViewWindow as it has both the View and the window to our caller. */
  (*(view_cbs_G->select))(view_window, view_window->parent_view->app_data, &view_select) ;

  
  window_select->xml_handler.handled = view_select.xml_handler.handled;
  
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

static ZMapView createZMapView(GtkWidget *xremote_widget, char *view_name, GList *sequences, void *app_data)
{
  ZMapView zmap_view = NULL ;
  GList *first ;
  ZMapViewSequenceMap master_seq ;

  first = g_list_first(sequences) ;
  master_seq = (ZMapViewSequenceMap)(first->data) ;

  zmap_view = g_new0(ZMapViewStruct, 1) ;

  zmap_view->state = ZMAPVIEW_INIT ;
  zmap_view->busy = FALSE ;

  zmap_view->xremote_widget = xremote_widget ;

  zmapViewSetupXRemote(zmap_view, xremote_widget);

  zmap_view->view_name = g_strdup(view_name) ;

  /* TEMP CODE...UNTIL I GET THE MULTIPLE SEQUENCES IN ONE VIEW SORTED OUT..... */
  /* TOTAL HACK UP MESS.... */
  zmap_view->sequence = g_strdup(master_seq->sequence) ;
  zmap_view->start = master_seq->start ;
  zmap_view->end = master_seq->end ;


  /* TOTAL LASH UP FOR NOW..... */
  if (!(zmap_view->sequence_2_server))
    {
      getSequenceServers(zmap_view, NULL) ;
    }


  /* Set the regions we want to display. */
  zmap_view->sequence_mapping = sequences ;

  zmap_view->window_list = zmap_view->connection_list = NULL ;

  zmap_view->app_data = app_data ;

  zmap_view->revcomped_features = FALSE ;

  return zmap_view ;
}


/* Adds a window to a view. */
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget)
{
  ZMapViewWindow view_window = NULL ;
  ZMapWindow window ;

  view_window = createWindow(zmap_view, NULL) ;

  /* There are no steps where this can fail at the moment. */
  window = zMapWindowCreate(parent_widget, zmap_view->sequence, view_window, NULL) ;
  zMapAssert(window) ;

  view_window->window = window ;

  /* add to list of windows.... */
  zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;
	  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* This seems redundant now....old code ???? */

  /* There is a bit of a hole in the "busy" state here in that we do the windowcreate
   * but have not signalled that we are busy, I suppose we could do the busy stuff twice,
   * once before the windowCreate and once here to make sure we set cursors everywhere.. */
  zmapViewBusy(zmap_view, TRUE) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Start polling function that checks state of this view and its connections, note
   * that at this stage there is no data to display. */
  startStateConnectionChecking(zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zmapViewBusy(zmap_view, FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return view_window ;
}

static void killAllSpawned(ZMapView zmap_view)
{
  GPid pid;
  GList *processes = zmap_view->spawned_processes;

  while(processes)
    {
      pid = GPOINTER_TO_INT(processes->data);
      g_spawn_close_pid(pid);
      kill(pid, 9);
      processes = processes->next;
    }

  if(zmap_view->spawned_processes)
    g_list_free(zmap_view->spawned_processes);
  zmap_view->spawned_processes = NULL ;

  return ;
}

/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. The lists of windows and thread connections are dealt with somewhat
 * asynchronously by killAllWindows() & checkConnections() */
static void destroyZMapView(ZMapView *zmap_view_out)
{
  ZMapView zmap_view = *zmap_view_out ;

  g_free(zmap_view->sequence) ;

  if (zmap_view->sequence_2_server)
    {
      g_list_foreach(zmap_view->sequence_2_server, destroySeq2ServerCB, NULL) ;
      g_list_free(zmap_view->sequence_2_server) ;
      zmap_view->sequence_2_server = NULL ;
    }

  if(zmap_view->cwh_hash)
    zmapViewCWHDestroy(&(zmap_view->cwh_hash));

  killAllSpawned(zmap_view);

  g_free(zmap_view) ;

  *zmap_view_out = NULL ;

  return ;
}



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
/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopStateConnectionChecking(ZMapView zmap_view)
{
  gtk_timeout_remove(zmap_view->idle_handle) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





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
 *  */
static gboolean checkStateConnections(ZMapView zmap_view)
{
  gboolean call_again = TRUE ;				    /* Normally we want to called continuously. */
  GList* list_item ;
  gboolean state_change = TRUE ;			    /* Records whehter state of view has changed. */
  gboolean threads_have_died = FALSE ;			    /* Records if any threads have died. */


  /* should assert the zmapview_state here to save checking later..... */

  if (zmap_view->connection_list)
    {
      list_item = g_list_first(zmap_view->connection_list) ;
      /* should assert this as never null.... */

      do
	{
	  ZMapViewConnection view_con ;
	  ZMapThread thread ;
	  ZMapThreadReply reply ;
	  void *data = NULL ;
	  char *err_msg = NULL ;
      
	  view_con = list_item->data ;
	  thread = view_con->thread ;

	  /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
	   * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
	   * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
	   * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

	  data = NULL ;
	  err_msg = NULL ;
	  if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
	    {
	      threadDebugMsg(thread, "GUI: thread %s, cannot access reply from thread - %s\n", err_msg) ;

	      /* should abort or dump here....or at least kill this connection. */

	    }
	  else
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      threadDebugMsg(thread, "GUI: thread %s, thread reply = %s\n", zMapVarGetReplyString(reply)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      
	      if (reply == ZMAPTHREAD_REPLY_WAIT)
		{
		  state_change = FALSE ;
		}
	      else if (reply == ZMAPTHREAD_REPLY_GOTDATA)
		{
		  view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;

		  /* Really we should only be loading stuff if we are LOADING.... */
		  if (zmap_view->state == ZMAPVIEW_LOADING || zmap_view->state == ZMAPVIEW_LOADED)
		    {
		      ZMapServerReqAny req_any = (ZMapServerReqAny)data ;

		      threadDebugMsg(thread, "GUI: thread %s, got data\n", NULL) ;

		      /* Reset the reply from the slave. */
		      zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
		  

		      /* Process the different types of data coming back. */
		      switch (req_any->type)
			{
			case ZMAP_SERVERREQ_OPENLOAD:
			  {
			    ZMapServerReqGetFeatures features
			      = (ZMapServerReqGetFeatures)&(((ZMapServerReqOpenLoad)req_any)->features) ;

			    getFeatures(zmap_view, features) ;

			    zmap_view->connections_loaded++ ;

			    /* This will need to be more sophisticated, we will need to time
			     * connections out. */
			    if (zmap_view->connections_loaded
				== g_list_length(zmap_view->connection_list))
			      zmap_view->state = ZMAPVIEW_LOADED ;

			    break ;
			  }

			case ZMAP_SERVERREQ_GETSEQUENCE:
			  {
			    ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;
			    GPid blixem_pid ;
			    gboolean status ;

			    /* Got the sequences so launch blixem. */
			    if ((status = zmapViewCallBlixem(zmap_view,
							     get_sequence->orig_feature, get_sequence->sequences,
							     &blixem_pid)))
			     zmap_view->spawned_processes = g_list_append(zmap_view->spawned_processes,
									  GINT_TO_POINTER(blixem_pid)) ;

			    break ;
			  }

			case ZMAP_SERVERREQ_FEATURES:
			case ZMAP_SERVERREQ_FEATURE_SEQUENCE:
			case ZMAP_SERVERREQ_SEQUENCE:
			  {
			    getFeatures(zmap_view, (ZMapServerReqGetFeatures)req_any) ;

			    break ;
			  }

			default:
			  {	  
			    zMapLogFatal("Unknown request type: %d", req_any->type) ; /* Exit appl. */
			    break ;
			  }
			}

		      /* Free the request block. */
		      g_free(data) ;
		    }
		  else
		    threadDebugMsg(thread, "GUI: thread %s, got data but ZMap state is - %s\n",
				   zmapViewGetStatusAsStr(zMapViewGetStatus(zmap_view))) ;

		}
	      else if (reply == ZMAPTHREAD_REPLY_REQERROR)
		{
		  if (err_msg)
		    zMapWarning("%s", err_msg) ;

		  view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;

		  /* Reset the reply from the slave. */
		  zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;

		  /* This means the request failed for some reason. */
		  threadDebugMsg(thread, "GUI: thread %s, request to server failed....\n", NULL) ;

		  g_free(err_msg) ;
		}
	      else if (reply == ZMAPTHREAD_REPLY_DIED)
		{
		  /* This means the thread has failed for some reason and we should clean up. */

		  if (err_msg)
		    zMapWarning("%s", err_msg) ;

		  threads_have_died = TRUE ;

		  threadDebugMsg(thread, "GUI: thread %s has died so cleaning up....\n", NULL) ;
		  
		  /* We are going to remove an item from the list so better move on from
		   * this item. */
		  list_item = g_list_next(list_item) ;
		  zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;

		  destroyConnection(&view_con) ;
		}
	      else if (reply == ZMAPTHREAD_REPLY_CANCELLED)
		{
		  /* This happens when we have signalled the threads to die and they are
		   * replying to say that they have now died. */

		  /* This means the thread was cancelled so we should clean up..... */
		  threadDebugMsg(thread, "GUI: thread %s has been cancelled so cleaning up....\n", NULL) ;

		  /* We are going to remove an item from the list so better move on from
		   * this item. */
		  list_item = g_list_next(list_item) ;
		  zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;

		  destroyConnection(&view_con) ;
		}
	    }
	}
      while ((list_item = g_list_next(list_item))) ;
    }


  /* Fiddly logic here as this could be combined with the following code that handles if we don't
   * have any connections any more...but not so easy as some of the code below kills the zmap so.
   * easier to do this here. */
  if (zmap_view->busy && !zmapAnyConnBusy(zmap_view->connection_list))
    zmapViewBusy(zmap_view, FALSE) ;


  /* At this point if there are no threads left we need to examine our state and take action
   * depending on whether we are dying or threads have died or whatever.... */
  if (zmap_view->connection_list)
    {
      /* Signal layer above us if view has changed state. */
      if (state_change)
	(*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;
    }
  else
    {
      /* Change to a switch here.... */

      if (zmap_view->state == ZMAPVIEW_INIT)
	{
	  /* Nothing to do here I think.... */

	}
      else if (zmap_view->state == ZMAPVIEW_CONNECTING || zmap_view->state == ZMAPVIEW_CONNECTED
	       || zmap_view->state == ZMAPVIEW_LOADING || zmap_view->state == ZMAPVIEW_LOADED)
	{

	  /* We should kill the ZMap here with an error message....or allow user to reload... */

	  if (threads_have_died)
	    {
	      /* Threads have died because of their own errors....but the ZMap is not dying so
	       * reset state to init and we should return an error here....(but how ?), we 
	       * should not be outputting messages....I think..... */
	      zMapWarning("%s", "Cannot show ZMap because server connections have all died") ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      killGUI(zmap_view) ;
	      destroyZMapView(zmap_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	      /* need to reset here..... */

	      call_again = FALSE ;
	    }
	  /* I don't think the "else" should be possible......so perhaps we should fail if that
	   * happens..... */

	  zmap_view->state = ZMAPVIEW_INIT ;
	}
      else if (zmap_view->state == ZMAPVIEW_RESETTING)
	{
	  /* zmap is ok but user has reset it and all threads have died so we need to stop
	   * checking.... */
	  zmap_view->state = ZMAPVIEW_INIT ;

	  /* Signal layer above us because the view has reset. */
	  (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;

	  call_again = FALSE ;
	}
      else if (zmap_view->state == ZMAPVIEW_DYING)
	{
	  /* Tricky coding here: we need to destroy the view _before_ we notify the layer above us
	   * that the zmap_view has gone but we need to pass information from the view back, so we
	   * keep a temporary record of certain parts of the view. */
	  ZMapView zmap_view_ref = zmap_view ;
	  void *app_data = zmap_view->app_data ;

	  /* view was waiting for threads to die, now they have we can free everything. */
	  destroyZMapView(&zmap_view) ;

	  /* Signal layer above us that view has died. */
	  (*(view_cbs_G->destroy))(zmap_view_ref, app_data, NULL) ;

	  call_again = FALSE ;
	}
    }


  return call_again ;
}


static void loadDataConnections(ZMapView zmap_view)
{

  if (zmap_view->connection_list)
    {
      GList* list_item ;

      list_item = g_list_first(zmap_view->connection_list) ;

      do
	{
	  ZMapViewConnection view_con ;
	  ZMapThread thread ;

	  ZMapServerReqGetFeatures req_features ;

	  view_con = list_item->data ;
	  thread = view_con->thread ;

	  /* Construct the request to get the features, if its the sequence server then
	   * get the sequence as well. */
	  req_features = g_new0(ZMapServerReqGetFeaturesStruct, 1) ;
	  if (view_con->sequence_server)
	    req_features->type = ZMAP_SERVERREQ_FEATURE_SEQUENCE ;
	  else
	    req_features->type = ZMAP_SERVERREQ_FEATURES ;

	  zMapThreadRequest(thread, req_features) ;

	} while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}



/* Calls the control window callback to remove any reference to the zmap and then destroys
 * the actual zmap itself.
 *  */
static void killGUI(ZMapView zmap_view)
{
  killAllWindows(zmap_view) ;

  return ;
}


static void killConnections(ZMapView zmap_view)
{
  GList* list_item ;

  zMapAssert(zmap_view->connection_list) ;

  list_item = g_list_first(zmap_view->connection_list) ;
  do
    {
      ZMapViewConnection view_con ;
      ZMapThread thread ;

      view_con = list_item->data ;
      thread = view_con->thread ;

      /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
       * will actually die sometime later. */
      zMapThreadKill(thread) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}





static void invoke_merge_in_names(gpointer list_data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)list_data;
  GList *feature_set_names = (GList *)user_data;

  /* This relies on hokey code... with a number of issues...
   * 1) the window function only concats the lists.
   * 2) this view code might well want to do the merge?
   * 3) how do we order all these columns?
   */
  zMapWindowMergeInFeatureSetNames(view_window->window, feature_set_names);

  return ;
}


/* Allocate a connection and send over the request to get the sequence displayed. */
static ZMapViewConnection createConnection(ZMapView zmap_view,
					   zMapURL url, char *format,
					   int timeout, char *version,
					   char *styles_file,
					   char *featuresets_names, char *navigator_set_names,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end)
{
  ZMapViewConnection view_con = NULL ;
  GData *types = NULL ;
  GList *req_featuresets = NULL, *tmp_navigator_sets = NULL ;
  ZMapThread thread ;
  gboolean status = TRUE ;
  gboolean dna_requested = FALSE ;


  /* User can specify styles in a file. If they don't we get them all and filter them
   * according to the feature set name list if specified. */
  if (styles_file)
    {
      char *filepath ;

      if (!(filepath = zMapConfigDirFindFile(styles_file))
	  || !(types = zMapFeatureTypeGetFromFile(filepath)))
	{
	  zMapLogWarning("Could not read types from \"stylesfile\" %s", filepath) ;
	  status = FALSE ;
	}
      
      if (filepath)
	g_free(filepath) ;
    }


  /* User can specify feature set names that should be displayed in an ordered list. Order of
   * list determines order of columns. */
  if (featuresets_names)
    {
      /* If user only wants some featuresets displayed then build a list of their quark names. */
      req_featuresets = zMapFeatureString2QuarkList(featuresets_names) ;


      /* Check whether dna was requested, see comments below about setting up sequence req. */
      if ((zMap_g_list_find_quark(req_featuresets, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
	dna_requested = TRUE ;

      g_list_foreach(zmap_view->window_list, invoke_merge_in_names, req_featuresets);

    }

  if (navigator_set_names)
    {
      tmp_navigator_sets = zmap_view->navigator_set_names = zMapFeatureString2QuarkList(navigator_set_names);

      if(zmap_view->navigator_window)
        zMapWindowNavigatorMergeInFeatureSetNames(zmap_view->navigator_window, tmp_navigator_sets);
    }

  if (req_featuresets && tmp_navigator_sets)
    {
      /* We should do a proper merge here! */
      req_featuresets = g_list_concat(req_featuresets, tmp_navigator_sets);
    }

  /* Create the thread to service the connection requests, we give it a function that it will call
   * to decode the requests we send it and a terminate function. */
  if (status && (thread = zMapThreadCreate(zMapServerRequestHandler, zMapServerTerminateHandler)))
    {
      ZMapServerReqOpenLoad open_load = NULL ;

      /* Build the intial request. */
      open_load = g_new0(ZMapServerReqOpenLoadStruct, 1) ;
      open_load->type = ZMAP_SERVERREQ_OPENLOAD ;

      open_load->open.url     = url;
      open_load->open.format  = g_strdup(format) ;
      open_load->open.timeout = timeout ;
      open_load->open.version = g_strdup(version) ;

      open_load->context.context = createContext(sequence, start, end, types, req_featuresets) ;

      open_load->styles.styles = types ;

      open_load->feature_sets.feature_sets = req_featuresets ;

      /* NOTE, some slightly tricky logic here: if this is a sequence server AND dna has been
       * requested then ask for the sequence, otherwise don't. Only one server can be a sequence server. */
      if (sequence_server && dna_requested)
	open_load->features.type = ZMAP_SERVERREQ_FEATURE_SEQUENCE ;
      else
	open_load->features.type = ZMAP_SERVERREQ_FEATURES ;

      /* Send the request to the thread to open a connection and get the date. */
      zMapThreadRequest(thread, (void *)open_load) ;

      view_con = g_new0(ZMapViewConnectionStruct, 1) ;

      view_con->sequence_server = sequence_server ;
      view_con->writeback_server = writeback_server ;

      view_con->types = types ;

      view_con->parent_view = zmap_view ;

      view_con->curr_request = ZMAPTHREAD_REQUEST_EXECUTE ;

      view_con->thread = thread ;
    }

  return view_con ;
}



static void destroyConnection(ZMapViewConnection *view_conn_ptr)
{
  ZMapViewConnection view_conn = *view_conn_ptr ;

  zMapThreadDestroy(view_conn->thread) ;

  /* Need to destroy the types array here....... */

  g_free(view_conn) ;

  *view_conn_ptr = NULL ;

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

      view_window = list_item->data ;

      zMapWindowReset(view_window->window) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}



/* Signal all windows there is data to draw. */
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features, 
                               ZMapFeatureContext new_features,
                               gboolean undisplay)
{
  GList *list_item, *window_list  = NULL;
  gboolean clean_required = FALSE;

  list_item = g_list_first(zmap_view->window_list) ;

  /* when the new features aren't the stored features i.e. not the first draw */
  /* un-drawing the features doesn't work the same way as drawing */
  if(all_features != new_features && !undisplay)
    {
      clean_required = TRUE;
    }

  do
    {
      ZMapViewWindow view_window ;

      view_window = list_item->data ;
      if(!undisplay)
        zMapWindowDisplayData(view_window->window, NULL, all_features, new_features) ;
      else
        zMapWindowUnDisplayData(view_window->window, all_features, new_features);

      if(clean_required)
        window_list = g_list_append(window_list, view_window->window);
    }
  while ((list_item = g_list_next(list_item))) ;

  if(clean_required)
    zmapViewCWHSetList(zmap_view->cwh_hash, new_features, window_list);

  return ;
}


static void loaded_dataCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapFeatureContext context = (ZMapFeatureContext)window_data;
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
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

  return ;
}


/* Kill all the windows... */
static void killAllWindows(ZMapView zmap_view)
{

  while (zmap_view->window_list)
    {
      ZMapViewWindow view_window ;

      view_window = zmap_view->window_list->data ;

      destroyWindow(zmap_view, view_window) ;
    }


  return ;
}


static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window)
{
  ZMapViewWindow view_window ;
  
  view_window = g_new0(ZMapViewWindowStruct, 1) ;
  view_window->parent_view = zmap_view ;		    /* back pointer. */
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



/* Error handling is rubbish here...stuff needs to be free whether there is an error or not.
 * 
 * new_features should be freed (but not the data...ahhhh actually the merge should
 * free any replicated data...yes, that is what should happen. Then when it comes to
 * the diff we should not free the data but should free all our structs...
 * 
 * We should free the context_inout context here....actually better
 * would to have a "free" flag............ 
 *  */
static gboolean mergeAndDrawContext(ZMapView view, ZMapFeatureContext context_in)
{
  ZMapFeatureContext diff_context = NULL ;
  gboolean identical_contexts = FALSE, free_diff_context = FALSE;

  zMapAssert(context_in);

  if(justMergeContext(view, &context_in))
    {
      if(diff_context == context_in)
        identical_contexts = TRUE;

      diff_context = context_in;

      free_diff_context = !(identical_contexts);

      justDrawContext(view, diff_context);
    }
  else
    zMapLogCritical("%s", "Unable to draw diff context after mangled merge!");

  return identical_contexts;
}

static gboolean justMergeContext(ZMapView view, ZMapFeatureContext *context_inout)
{
  ZMapFeatureContext new_features, diff_context = NULL ;
  gboolean merged = FALSE;

  new_features = *context_inout ;

  zMapPrintTimer(NULL, "Merge Context starting...") ;

  if (view->revcomped_features)
    {
      zMapPrintTimer(NULL, "Merge Context has to rev comp first, starting") ;
        
      zMapFeatureReverseComplement(new_features);

      zMapPrintTimer(NULL, "Merge Context has to rev comp first, finished") ;
    }

  if (!(merged = zMapFeatureContextMerge(&(view->features), &new_features, &diff_context)))
    {
      zMapLogCritical("%s", "Cannot merge feature data from....") ;
    }
  else
    {
      zMapLogWarning("%s", "Helpful message to say merge went well...");
    }

  zMapPrintTimer(NULL, "Merge Context Finished.") ;

  /* Return the diff_context which is the just the new features (NULL if merge fails). */
  *context_inout = diff_context ;

  return merged;
}

static void justDrawContext(ZMapView view, ZMapFeatureContext diff_context)
{
   /* Signal the ZMap that there is work to be done. */
  displayDataWindows(view, view->features, diff_context, FALSE) ;
 
  /* Not sure about the timing of the next bit. */
 
  /* We have to redraw the whole navigator here.  This is a bit of
   * a pain, but it's due to the scaling we do to make the rest of
   * the navigator work.  If the length of the sequence changes the 
   * all the previously drawn features need to move.  It also 
   * negates the need to keep state as to the length of the sequence,
   * the number of times the scale bar has been drawn, etc... */
  zMapWindowNavigatorReset(view->navigator_window); /* So reset */
  zMapWindowNavigatorSetStrand(view->navigator_window, view->revcomped_features);
  /* and draw with _all_ the view's features. */
  zMapWindowNavigatorDrawFeatures(view->navigator_window, view->features);
  
  /* signal our caller that we have data. */
  (*(view_cbs_G->load_data))(view, view->app_data, NULL) ;
  
  return ;
}

static void eraseAndUndrawContext(ZMapView view, ZMapFeatureContext context_inout)
{                              
  ZMapFeatureContext diff_context = NULL;

  if(!zMapFeatureContextErase(&(view->features), context_inout, &diff_context))
    zMapLogCritical("%s", "Cannot erase feature data from...");
  else
    {
      displayDataWindows(view, view->features, diff_context, TRUE);
      
      zMapFeatureContextDestroy(diff_context, TRUE);
    }

  return ;
}

static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req)
{
  ZMapFeatureContext new_features = NULL;

  zMapPrintTimer(NULL, "Got Features from Thread") ;


  /* Merge new data with existing data (if any). */
  if ((new_features = feature_req->feature_context_out))
    {
      /* Truth means view->features == new_features i.e. first time round */
      if(!mergeAndDrawContext(zmap_view, new_features))
        {
          new_features = feature_req->feature_context_out = NULL;
        }
    }


  return ;
}



/* Layer below wants us to execute a command. */
static void commandCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowCallbackCommandAny cmd_any = (ZMapWindowCallbackCommandAny)window_data ;

  switch(cmd_any->cmd)
    {
    case ZMAPWINDOW_CMD_SHOWALIGN:
      {
	ZMapWindowCallbackCommandAlign align_cmd = (ZMapWindowCallbackCommandAlign)cmd_any ;
	gboolean status ;
	ZMapView view = view_window->parent_view ;
	GList *local_sequences = NULL ;

	if ((status = zmapViewBlixemLocalSequences(view, align_cmd->feature, &local_sequences)))
	  {
	    if (!view->sequence_server)
	      zMapWarning("%s", "No sequence server was specified so cannot fetch raw sequences for blixem.") ;
	    else
	      {
		ZMapViewConnection view_con ;
		ZMapThread thread ;
		ZMapServerReqGetSequence req_sequences ;

		view_con = view->sequence_server ;
		zMapAssert(view_con->sequence_server) ;

		thread = view_con->thread ;

		/* Construct the request to get the sequences. */
		req_sequences = g_new0(ZMapServerReqGetSequenceStruct, 1) ;
		req_sequences->type = ZMAP_SERVERREQ_GETSEQUENCE ;
		req_sequences->orig_feature = align_cmd->feature ;
		req_sequences->sequences = local_sequences ;

		zMapThreadRequest(thread, (void *)req_sequences) ;
	      }
	  }
	else
	  {
	    GPid blixem_pid ;

	    if ((status = zmapViewCallBlixem(view, align_cmd->feature, NULL, &blixem_pid)))
	      view->spawned_processes = g_list_append(view->spawned_processes, GINT_TO_POINTER(blixem_pid)) ;
	  }

	break ;
      }
    case ZMAPWINDOW_CMD_REVERSECOMPLEMENT:
      {
	gboolean status ;
	ZMapView view = view_window->parent_view ;

	/* NOTE, there is no need to signal the layer above that things are changing,
	 * the layer above does the complement and handles all that. */
	if (!(status = zMapViewReverseComplement(view)))
	  {
	    zMapLogCritical("%s", "View Reverse Complement failed.") ;

	    zMapWarning("%s", "View Reverse Complement failed.") ;
	  }

	break ;
      }
    default:
      {
	zMapAssertNotReached() ;
	break ;
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

  /* view_window->window can be NULL (when window copying) so we use the passed in window... */
  /* Yes it's a bit messy, but it's stopping it crashing. */
  zMapWindowNavigatorSetCurrentWindow(view_window->parent_view->navigator_window, window);

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
static ZMapFeatureContext createContext(char *sequence, int start, int end,
					GData *types, GList *feature_set_names)
{
  ZMapFeatureContext context = NULL ;
  gboolean master = TRUE ;
  ZMapFeatureAlignment alignment ;
  ZMapFeatureBlock block ;

  context = zMapFeatureContextCreate(sequence, start, end, types, feature_set_names) ;

  /* Add the master alignment and block. */
  alignment = zMapFeatureAlignmentCreate(sequence, master) ; /* TRUE => master alignment. */

  zMapFeatureContextAddAlignment(context, alignment, master) ;

  block = zMapFeatureBlockCreate(sequence,
				 start, end, ZMAPSTRAND_FORWARD,
				 start, end, ZMAPSTRAND_FORWARD) ;

  zMapFeatureAlignmentAddBlock(alignment, block) ;


  /* Add other alignments if any were specified in a config file. */
  addAlignments(context) ;


  return context ;
}


/* Add other alignments if any specified in a config file. */
static void addAlignments(ZMapFeatureContext context)
{
  ZMapConfigStanzaSet blocks_list = NULL ;
  ZMapConfig config ;
  char *config_file ;
  char *stanza_name = "align" ;
  gboolean result = FALSE ;
  char *ref_seq ;
  int start, end ;
  ZMapFeatureAlignment alignment = NULL ;


  ref_seq = (char *)g_quark_to_string(context->sequence_name) ;
  start = context->sequence_to_parent.c1 ;
  end = context->sequence_to_parent.c2 ;


  config_file = zMapConfigDirGetFile() ;
  if ((config = zMapConfigCreateFromFile(config_file)))
    {
      ZMapConfigStanza block_stanza ;
      ZMapConfigStanzaElementStruct block_elements[]
	= {{"reference_seq"        , ZMAPCONFIG_STRING , {NULL}},
	   {"reference_start"      , ZMAPCONFIG_INT    , {NULL}},
	   {"reference_end"        , ZMAPCONFIG_INT    , {NULL}},
	   {"reference_strand"     , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_seq"    , ZMAPCONFIG_STRING , {NULL}},
	   {"non_reference_start"  , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_end"    , ZMAPCONFIG_INT    , {NULL}},
	   {"non_reference_strand" , ZMAPCONFIG_INT    , {NULL}},
	   {NULL, -1, {NULL}}} ;

      /* init non string elements. */
      zMapConfigGetStructInt(block_elements, "reference_start")      = 0 ;
      zMapConfigGetStructInt(block_elements, "reference_end")        = 0 ;
      zMapConfigGetStructInt(block_elements, "reference_strand")     = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_start")  = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_end")    = 0 ;
      zMapConfigGetStructInt(block_elements, "non_reference_strand") = 0 ;

      block_stanza = zMapConfigMakeStanza(stanza_name, block_elements) ;

      result = zMapConfigFindStanzas(config, block_stanza, &blocks_list) ;
    }

  if (result)
    {
      ZMapConfigStanza next_block = NULL;

      while ((next_block = zMapConfigGetNextStanza(blocks_list, next_block)) != NULL)
        {
          ZMapFeatureBlock data_block = NULL ;
	  char *reference_seq, *non_reference_seq ;
	  int reference_start, reference_end, non_reference_start, non_reference_end ;
	  ZMapStrand ref_strand = ZMAPSTRAND_REVERSE, non_strand = ZMAPSTRAND_REVERSE ;
	  int diff ;

	  reference_seq = zMapConfigGetElementString(next_block, "reference_seq") ;
	  reference_start = zMapConfigGetElementInt(next_block, "reference_start") ;
	  reference_end = zMapConfigGetElementInt(next_block, "reference_end") ;
	  non_reference_seq = zMapConfigGetElementString(next_block, "non_reference_seq") ;
	  non_reference_start = zMapConfigGetElementInt(next_block, "non_reference_start") ;
	  non_reference_end = zMapConfigGetElementInt(next_block, "non_reference_end") ;
	  if (zMapConfigGetElementInt(next_block, "reference_strand"))
	    ref_strand = ZMAPSTRAND_FORWARD ;
	  if (zMapConfigGetElementInt(next_block, "non_reference_strand"))
	    non_strand = ZMAPSTRAND_FORWARD ;


	  /* We only add aligns/blocks that are for the relevant reference sequence and within
	   * the start/end for that sequence. */
	  if (g_ascii_strcasecmp(ref_seq, reference_seq) == 0
	      && !(reference_start > end || reference_end < start))
	    {

	      if (!alignment)
		{
		  /* Add the other alignment, note that we do this dumbly at the moment assuming
		   * that there is only one other alignment */
		  alignment = zMapFeatureAlignmentCreate(non_reference_seq, FALSE) ;
	  
		  zMapFeatureContextAddAlignment(context, alignment, FALSE) ;
		}


	      /* Add the block for this set of data. */

	      /* clamp coords...SHOULD WE DO THIS, PERHAPS WE JUST REJECT THINGS THAT DON'T FIT ? */
	      if ((diff = start - reference_start) > 0)
		{
		  reference_start += diff ;
		  non_reference_start += diff ;
		}
	      if ((diff = reference_end - end) > 0)
		{
		  reference_end -= diff ;
		  non_reference_end -= diff ;
		}

	      data_block = zMapFeatureBlockCreate(non_reference_seq,
						  reference_start, reference_end, ref_strand,
						  non_reference_start, non_reference_end, non_strand) ;

	      zMapFeatureAlignmentAddBlock(alignment, data_block) ;
	    }
        }
    }

  return ;
}

/* Read list of sequence to server mappings (i.e. which sequences must be fetched from which
 * servers) from the zmap config file. */
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

		  search_str = server_seq_str ;
		  while ((sequence = strtok(search_str, " ")))
		    {
		      search_str = NULL ;
		      server = strtok(NULL, " ") ;

		      seq_2_server = createSeq2Server(sequence, server) ;

		      zmap_view->sequence_2_server = g_list_append(zmap_view->sequence_2_server, seq_2_server) ;
		    }
		}
	    }

	  zMapConfigDeleteStanzaSet(zmap_list) ;		    /* Not needed anymore. */
	}
      
      zMapConfigDestroyStanza(zmap_stanza) ;
      
      zMapConfigDestroy(config) ;

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


/* Hacky...sorry.... */
static void threadDebugMsg(ZMapThread thread, char *format_str, char *msg)
{
  char *thread_id ;
  char *full_msg ;

  thread_id = zMapThreadGetThreadID(thread) ;
  full_msg = g_strdup_printf(format_str, thread_id, msg ? msg : "") ;

  zMapDebug("%s", full_msg) ;

  g_free(full_msg) ;
  g_free(thread_id) ;

  return ;
}

static ZMapConfig getConfigFromBufferOrFile(char *config_str)
{
  ZMapConfig config = NULL;
  char *config_file = NULL;

  if((config_file = zMapConfigDirGetFile()))
    {
      if(config_str && !*config_str)
        config_str = NULL;

      if((config_str != NULL && (config = zMapConfigCreateFromBuffer(config_str)))
         ||
         (config_str == NULL && (config = zMapConfigCreateFromFile(config_file))))
        {
          config_file = NULL;
        }
    }

  return config;
}
