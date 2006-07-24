/*  File: zmapView.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See ZMap/zmapView.h
 * HISTORY:
 * Last edited: Jul 24 21:55 2006 (rds)
 * Created: Thu May 13 15:28:26 2004 (edgrif)
 * CVS info:   $Id: zmapView.c,v 1.83 2006-07-24 22:01:38 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapWindow.h>
#include <zmapView_P.h>


static ZMapView createZMapView(char *sequence, int start, int end, void *app_data) ;
static void destroyZMapView(ZMapView zmap) ;

static gint zmapIdleCB(gpointer cb_data) ;
static void enterCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void leaveCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void scrollCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void focusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void selectCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data);
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data);
static void destroyCB(ZMapWindow window, void *caller_data, void *window_data) ;

static void viewSplitToPatternCB(ZMapWindow window, void *caller_data, void *window_data);
static void viewDoubleSelectCB(ZMapWindow window, void *caller_data, void *window_data);

static void setZoomStatus(gpointer data, gpointer user_data);
static void splitMagic(gpointer data, gpointer user_data);

static void startStateConnectionChecking(ZMapView zmap_view) ;
static void stopStateConnectionChecking(ZMapView zmap_view) ;
static gboolean checkStateConnections(ZMapView zmap_view) ;

static void loadDataConnections(ZMapView zmap_view) ;

static void killZMapView(ZMapView zmap_view) ;
static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;


/* These are candidates to moved into zmapServer in fact, would be a more logical place for them. */
static ZMapViewConnection createConnection(ZMapView zmap_view,
					   zMapURL url, char *format,
					   int timeout, char *version,
					   char *styles_file, char *feature_sets,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end) ;
static void destroyConnection(ZMapViewConnection *view_conn) ;

static void resetWindows(ZMapView zmap_view) ;
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features, ZMapFeatureContext new_features) ;
static void killAllWindows(ZMapView zmap_view) ;

static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window) ;
static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window) ;

static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req) ;

static GList *string2StyleQuarks(char *feature_sets) ;
static gboolean nextIsQuoted(char **text) ;

static ZMapFeatureContext createContext(char *sequence, int start, int end,
					GList *types, GList *feature_set_names) ;
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget) ;

/* this surely needs to end up somewhere else in the end... */
ZMapAlignBlock zMapAlignBlockCreate(char *ref_seq, int ref_start, int ref_end, int ref_strand,
				    char *non_seq, int non_start, int non_end, int non_strand) ;
static void addAlignments(ZMapFeatureContext context) ;







/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;


/* Callbacks back we set in the level below us, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G = {enterCB, leaveCB,
					  scrollCB, focusCB, selectCB, viewDoubleSelectCB, 
                                          viewSplitToPatternCB,
					  setZoomStatusCB, viewVisibilityChangeCB, destroyCB} ;



/*
 *  ------------------- External functions -------------------
 */


/* This routine must be called just once before any other views routine,
 * the caller must supply all of the callback routines.
 * 
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of views terminate routine. */
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
  view_cbs_G->double_select = callbacks->double_select ;
  view_cbs_G->split_to_pattern = callbacks->split_to_pattern;
  view_cbs_G->visibility_change = callbacks->visibility_change ;
  view_cbs_G->state_change = callbacks->state_change ;
  view_cbs_G->destroy = callbacks->destroy ;



  /* Init windows.... */
  zMapWindowInit(&window_cbs_G) ;


  return ;
}



/* Create a "view", this is the holder for a single feature context. The view may use
 * several threads to get this context and may display it in several windows.
 * A view _always_ has at least one window, this window may be blank but as long as
 * there is a view, there is a window. This makes the coding somewhat simpler and is
 * intuitively sensible. */
ZMapViewWindow zMapViewCreate(GtkWidget *parent_widget,
			      char *sequence, int start, int end,
			      void *app_data)
{
  ZMapViewWindow view_window = NULL ;
  ZMapView zmap_view = NULL ;
  gboolean debug ;

  /* No callbacks, then no view creation. */
  zMapAssert(view_cbs_G) ;

  /* Set up debugging for threads, we do it here so that user can change setting in config file
   * and next time they create a view the debugging will go on/off. */
  if (zMapUtilsConfigDebug(ZMAPTHREAD_CONFIG_DEBUG_STR, &debug))
    zmap_thread_debug_G = debug ;

  zmap_view = createZMapView(sequence, start, end, app_data) ;

  zmap_view->state = ZMAPVIEW_INIT ;

  view_window = addWindow(zmap_view, parent_widget) ;


  return view_window ;
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

      zmapViewBusy(zmap_view, TRUE) ;

      /* There is some redundancy of state here as the below code actually does a connect
       * and load in one call but we will almost certainly need the extra states later... */
      zmap_view->state = ZMAPVIEW_CONNECTING ;

      config_file = zMapConfigDirGetFile() ;
      if ((config_str != NULL && (config = zMapConfigCreateFromBuffer(config_str))) || 
          (config_str == NULL && (config = zMapConfigCreateFromFile(config_file))))
	{
	  ZMapConfigStanza server_stanza ;
	  ZMapConfigStanzaElementStruct server_elements[] = {{ZMAPSTANZA_SOURCE_URL,     ZMAPCONFIG_STRING, {NULL}},
							     {ZMAPSTANZA_SOURCE_TIMEOUT, ZMAPCONFIG_INT,    {NULL}},
							     {ZMAPSTANZA_SOURCE_VERSION, ZMAPCONFIG_STRING, {NULL}},
							     {"sequence", ZMAPCONFIG_BOOL, {NULL}},
							     {"writeback", ZMAPCONFIG_BOOL, {NULL}},
							     {"stylesfile", ZMAPCONFIG_STRING, {NULL}},
							     {"format", ZMAPCONFIG_STRING, {NULL}},
							     {ZMAPSTANZA_SOURCE_FEATURESETS, ZMAPCONFIG_STRING, {NULL}},
							     {NULL, -1, {NULL}}} ;

	  /* Set defaults for any element that is not a string. */
	  zMapConfigGetStructInt(server_elements, ZMAPSTANZA_SOURCE_TIMEOUT) = 120 ; /* seconds. */
	  zMapConfigGetStructBool(server_elements, "sequence") = FALSE ;
	  zMapConfigGetStructBool(server_elements, "writeback") = FALSE ;

	  server_stanza = zMapConfigMakeStanza(ZMAPSTANZA_SOURCE_CONFIG, server_elements) ;

	  if (!zMapConfigFindStanzas(config, server_stanza, &server_list))
	    result = FALSE ;

	  zMapConfigDestroyStanza(server_stanza) ;
	  server_stanza = NULL ;
	}


      /* Set up connections to the named servers. */
      if (config && server_list)
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
	      char *version, *styles_file, *format, *url, *featuresets ;
	      int timeout, url_parse_error ;
              zMapURL urlObj;
	      gboolean sequence_server, writeback_server ;
	      ZMapViewConnection view_con ;

	      url     = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_URL) ;
	      format  = zMapConfigGetElementString(next_server, "format") ;
	      timeout = zMapConfigGetElementInt(next_server, ZMAPSTANZA_SOURCE_TIMEOUT) ;
	      version = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_VERSION) ;
	      styles_file = zMapConfigGetElementString(next_server, "stylesfile") ;
	      featuresets = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_FEATURESETS) ;

              /* url is absolutely required. Go on to next stanza if there isn't one.
               * Done before anything else so as not to set seq/write servers to invalid locations  */
              if(!url)
		{
		  zMapLogWarning("GUI: %s", "computer says no url specified") ;
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
              else if (urlObj
		       && (view_con = createConnection(zmap_view, urlObj,
						       format,
						       timeout, version, styles_file, featuresets,
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
	  destroyWindow(zmap_view, view_window) ;
	}
    }

  return ;
}




/* Force a redraw of all the windows in a view, may be reuqired if it looks like
 * drawing has got out of whack due to an overloaded network etc etc. */
void zMapViewRedraw(ZMapViewWindow view_window)
{
  ZMapView view ;
  GList* list_item ;

  view = zMapViewGetView(view_window) ;
  zMapAssert(view) ;

  list_item = g_list_first(view->window_list) ;
  do
    {
      ZMapViewWindow view_window ;

      view_window = list_item->data ;

      zMapWindowRedraw(view_window->window) ;
    }
  while ((list_item = g_list_next(list_item))) ;

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
      if (zmap_view->revcomp_strand == ZMAPSTRAND_FORWARD)
	zmap_view->revcomp_strand = ZMAPSTRAND_REVERSE ;
      else
	zmap_view->revcomp_strand = ZMAPSTRAND_FORWARD ;

      list_item = g_list_first(zmap_view->window_list) ;
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowFeatureRedraw(view_window->window, zmap_view->features, TRUE) ;
	}
      while ((list_item = g_list_next(list_item))) ;

      /* Not sure if we need to do this or not.... */
      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;
    }

  return TRUE ;
}

/* Return which strand we are showing viz-a-viz reverse complementing. */
ZMapStrand zMapViewGetRevCompStatus(ZMapView zmap_view)
{

  return zmap_view->revcomp_strand ;

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
 */
gboolean zMapViewDestroy(ZMapView zmap_view)
{

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

	  (*(view_cbs_G->destroy))(zmap_view, zmap_view->app_data, NULL) ;
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

  return TRUE ;
}




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

  return ;
}


/* Called when some sequence window feature (e.g. column, actual feature etc.)
 * has been selected. */
static void selectCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
  ZMapWindowSelect select_item = (ZMapWindowSelect)window_data ;
  ZMapViewSelectStruct vselect = {{0}} ;

  if (select_item->item)
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

	  if ((item = zMapWindowFindFeatureItemByItem(view_window->window, select_item->item)))
	    zMapWindowHighlightObject(view_window->window, item) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }


  vselect.feature_desc = select_item->feature_desc ;
  vselect.secondary_text = select_item->secondary_text ;

  /* Pass back a ZMapViewWindow as it has both the View and the window to our caller. */
  (*(view_cbs_G->select))(view_window, view_window->parent_view->app_data, (void *)&vselect) ;

  return ;
}


/* DOES THIS EVER ACTUALLY HAPPEN........... */
/* Called when an underlying window is destroyed. */
static void destroyCB(ZMapWindow window, void *caller_data, void *window_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  printf("In View, in window destroyed callback\n") ;

  return ;
}
static void viewDoubleSelectCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data;
  ZMapWindowDoubleSelect window_select = (ZMapWindowDoubleSelect)window_data;
  ZMapViewDoubleSelectStruct double_select = {NULL};
  
  double_select.xml_events = window_select->xml_events;

  (*(view_cbs_G->double_select))(view_window, view_window->parent_view->app_data, &double_select);

  window_select->handled   = double_select.handled;
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

static ZMapView createZMapView(char *sequence, int start, int end, void *app_data)
{
  ZMapView zmap_view = NULL ;

  zmap_view = g_new0(ZMapViewStruct, 1) ;

  zmap_view->state = ZMAPVIEW_INIT ;
  zmap_view->busy = FALSE ;


  /* Set the region we want to display. */
  zmap_view->sequence = g_strdup(sequence) ;
  zmap_view->start = start ;
  zmap_view->end = end ;

  zmap_view->window_list = zmap_view->connection_list = NULL ;

  zmap_view->app_data = app_data ;

  zmap_view->revcomp_strand = ZMAPSTRAND_FORWARD ;

  return zmap_view ;
}


/* Adds the first window to a view.
 * Returns the window on success, NULL on failure. */
static ZMapViewWindow addWindow(ZMapView zmap_view, GtkWidget *parent_widget)
{
  ZMapViewWindow view_window = NULL ;
  ZMapWindow window ;

  view_window = createWindow(zmap_view, NULL) ;

  if (!(window = zMapWindowCreate(parent_widget, zmap_view->sequence, view_window)))
    {
      /* should glog and/or gerror at this stage....really need g_errors.... */
      view_window = NULL ;
    }
  else
    {
      view_window->window = window ;

      /* add to list of windows.... */
      zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;
	  
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
    }

  return view_window ;
}



/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. The lists of windows and thread connections are dealt with somewhat
 * asynchronously by killAllWindows() & checkConnections() */
static void destroyZMapView(ZMapView zmap_view)
{
  g_free(zmap_view->sequence) ;

  g_free(zmap_view) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  ZMAP_DEBUG("GUI: checking connection for thread %lu\n",
		     zMapConnGetThreadid(connection)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
	   * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
	   * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
	   * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

	  data = NULL ;
	  err_msg = NULL ;
	  if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
	    {
	      zMapDebug("GUI: thread %lu, cannot access reply from thread - %s\n",
			zMapThreadGetThreadid(thread), err_msg) ;

	      /* should abort or dump here....or at least kill this connection. */

	    }
	  else
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      zMapDebug("GUI: thread %lu, thread reply = %s\n",
			zMapThreadGetThreadid(thread),
			zMapVarGetReplyString(reply)) ;
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

		      zMapDebug("GUI: thread %lu, got data\n",
				zMapThreadGetThreadid(thread)) ;

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
		    zMapDebug("GUI: thread %lu, got data but ZMap state is - %s\n",
			      zMapThreadGetThreadid(thread),
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
		  zMapDebug("GUI: thread %lu, request to server failed....\n",
			    zMapThreadGetThreadid(thread)) ;

		  g_free(err_msg) ;
		}
	      else if (reply == ZMAPTHREAD_REPLY_DIED)
		{
		  if (err_msg)
		    zMapWarning("%s", err_msg) ;

		  threads_have_died = TRUE ;

		  /* This means the thread has failed for some reason and we should clean up. */
		  zMapDebug("GUI: thread %lu has died so cleaning up....\n",
			    zMapThreadGetThreadid(thread)) ;
		  
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
		  zMapDebug("GUI: thread %lu has been cancelled so cleaning up....\n",
			    zMapThreadGetThreadid(thread)) ;

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
	  /* Signal layer above us that view has died. */
	  (*(view_cbs_G->destroy))(zmap_view, zmap_view->app_data, NULL) ;

	  /* zmap was waiting for threads to die, now they have we can free everything and stop. */
	  destroyZMapView(zmap_view) ;

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


/* SOME ORDER ISSUES NEED TO BE ATTENDED TO HERE...WHEN SHOULD THE IDLE ROUTINE BE REMOVED ? */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* destroys the window if this has not happened yet and then destroys the slave threads
 * if there are any.
 */
static void killZMapView(ZMapView zmap_view)
{
  /* precise order is not straight forward...bound to be some race conditions here but...
   * 
   * - should inactivate GUI as this should be cheap and prevents further interactions, needs
   *   to inactivate ZMap _and_ controlling GUI.
   * - then should kill the threads so we can then wait until they have all died _before_
   *   freeing up data etc. etc.
   * - then we should stop checking the threads once they have died.
   * - then GUI should disappear so user cannot interact with it anymore.
   * Then we should kill the threads */


  if (zmap_view->window_list != NULL)
    {
      killGUI(zmap_view) ;
    }

  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
   * will actually die sometime later...check clean up sequence..... */
  if (zmap_view->connection_list != NULL)
    {
      killConnections(zmap_view) ;
    }
		  
  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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


/* Allocate a connection and send over the request to get the sequence displayed. */
static ZMapViewConnection createConnection(ZMapView zmap_view,
					   zMapURL url, char *format,
					   int timeout, char *version,
					   char *styles_file, char *featuresets_names,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end)
{
  ZMapViewConnection view_con = NULL ;
  GList *types = NULL ;
  GList *req_featuresets = NULL ;
  ZMapThread thread ;
  gboolean status = TRUE ;



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
      req_featuresets = string2StyleQuarks(featuresets_names) ;
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

      if (sequence_server)
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
			       ZMapFeatureContext all_features, ZMapFeatureContext new_features)
{
  GList* list_item ;

  list_item = g_list_first(zmap_view->window_list) ;

  do
    {
      ZMapViewWindow view_window ;

      view_window = list_item->data ;

      zMapWindowDisplayData(view_window->window, all_features, new_features) ;
    }
  while ((list_item = g_list_next(list_item))) ;

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
 *  */
static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req)
{
  ZMapFeatureContext new_features = NULL, diff_context = NULL ;

  /* Merge new data with existing data (if any). */
  new_features = feature_req->feature_context_out ;

  if (!zMapFeatureContextMerge(&(zmap_view->features), new_features, &diff_context))
    zMapLogCritical("%s", "Cannot merge feature data from....") ;
  else
    {
      /* We should free the new_features context here....actually better
       * would to have a "free" flag on the above merge call. */

      /* Signal the ZMap that there is work to be done. */
      displayDataWindows(zmap_view, zmap_view->features, diff_context) ;

      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;

    }

  return ;
}



static void viewVisibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  /* signal our caller that something has changed. */
  (*(view_cbs_G->visibility_change))(view_window, view_window->parent_view->app_data, window_data) ;

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



/* Take a string containing space separated featureset names (e.g. "coding fgenes codon")
 * and convert it to a list of proper featureset id quarks. */
static GList *string2StyleQuarks(char *featuresets)
{
  GList *featureset_quark_list = NULL ;
  char *list_pos = NULL ;
  char *next_featureset = NULL ;
  char *target ;


  target = featuresets ;
  do
    {
      GQuark featureset_id ;

      if (next_featureset)
	{
	  target = NULL ;
	  featureset_id = zMapStyleCreateID(next_featureset) ;
	  featureset_quark_list = g_list_append(featureset_quark_list, GUINT_TO_POINTER(featureset_id)) ;
	}
      else
	list_pos = target ;
    }
  while (((list_pos && nextIsQuoted(&list_pos))
	  && (next_featureset = strtok_r(target, "\"", &list_pos)))
	 || (next_featureset = strtok_r(target, " \t", &list_pos))) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMap_g_quark_list_print(featureset_quark_list) ;	    /* debug.... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return featureset_quark_list ;
}


/* Look through string to see if next non-space char is a quote mark, if it is return TRUE
 * and set *text to point to the quote mark, otherwise return FALSE and leave *text unchanged. */
static gboolean nextIsQuoted(char **text)
{
  gboolean quoted = FALSE ;
  char *text_ptr = *text ;

  while (*text_ptr)
    {
      if (!(g_ascii_isspace(*text_ptr)))
	{
	  if (*text_ptr == '"')
	    {
	      quoted = TRUE ;
	      *text = text_ptr ;
	    }
	  break ;
	}

      text_ptr++ ;
    }

  return quoted ;
}






/* Trial code to get alignments from a file and create a context...... */
static ZMapFeatureContext createContext(char *sequence, int start, int end,
					GList* types, GList *feature_set_names)
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

