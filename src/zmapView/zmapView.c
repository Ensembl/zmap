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
 * Last edited: Apr 20 19:30 2005 (edgrif)
 * Created: Thu May 13 15:28:26 2004 (edgrif)
 * CVS info:   $Id: zmapView.c,v 1.54 2005-04-21 13:51:13 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUrl.h>
#include <zmapView_P.h>


static ZMapView createZMapView(char *sequence, int start, int end, void *app_data) ;
static void destroyZMapView(ZMapView zmap) ;

static gint zmapIdleCB(gpointer cb_data) ;
static void enterCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void leaveCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void scrollCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void focusCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void selectCB(ZMapWindow window, void *caller_data, void *window_data) ;
static void visibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data);
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data);
static void destroyCB(ZMapWindow window, void *caller_data, void *window_data) ;

static void setZoomStatus(gpointer data, gpointer user_data);

static void startStateConnectionChecking(ZMapView zmap_view) ;
static void stopStateConnectionChecking(ZMapView zmap_view) ;
static gboolean checkStateConnections(ZMapView zmap_view) ;

static void loadDataConnections(ZMapView zmap_view) ;

static void killZMapView(ZMapView zmap_view) ;
static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;


/* These are candidates to moved into zmapServer in fact, would be a more logical place for them. */
static ZMapViewConnection createConnection(ZMapView zmap_view,
					   struct url *url, char *format,
					   int timeout, char *version, char *types_file,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end) ;
static void destroyConnection(ZMapViewConnection *view_conn) ;



static void resetWindows(ZMapView zmap_view) ;
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features, ZMapFeatureContext new_features) ;
static void killAllWindows(ZMapView zmap_view) ;

static ZMapViewWindow createWindow(ZMapView zmap_view, ZMapWindow window) ;
static void destroyWindow(ZMapView zmap_view, ZMapViewWindow view_window) ;

static void freeContext(ZMapFeatureContext feature_context) ;

static void getFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req) ;




/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;

/* Callbacks back we set in the level below us, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G = {enterCB, leaveCB,
					  scrollCB, focusCB, selectCB,
					  setZoomStatusCB, visibilityChangeCB, destroyCB} ;



/*
 *  ------------------- External functions -------------------
 *                     (includes callbacks)
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
	     && callbacks->visibility_change && callbacks->destroy) ;

  view_cbs_G = g_new0(ZMapViewCallbacksStruct, 1) ;

  view_cbs_G->enter = callbacks->enter ;
  view_cbs_G->leave = callbacks->leave ;
  view_cbs_G->load_data = callbacks->load_data ;
  view_cbs_G->focus = callbacks->focus ;
  view_cbs_G->select = callbacks->select ;
  view_cbs_G->visibility_change = callbacks->visibility_change ;
  view_cbs_G->destroy = callbacks->destroy ;


  /* Init windows.... */
  zMapWindowInit(&window_cbs_G) ;


  return ;
}



/* Create a new/blank zmap has no windows, has no thread connections to databases.
 * Returns NULL on failure. */
ZMapView zMapViewCreate(char *sequence,	int start, int end, void *app_data)
{
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

  return zmap_view ;
}


/* NEED TO REVISIT THIS, IT MAY ACTUALLY BE BETTER TO AMALGAMATE THIS ROUTINE WITH
 * ZMAPVIEWCREATE BUT THIS NEEDS SOME THOUGHT AS IT WILL INVOLVE CHANGES IN ZMAPCONTROL. */
/* Adds the first window to a view.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewMakeWindow(ZMapView zmap_view, GtkWidget *parent_widget)
{
  ZMapViewWindow view_window = NULL ;
  ZMapWindow window ;


  /* the view should _not already have a window. */
  zMapAssert(zmap_view && parent_widget
	     && (zmap_view->state == ZMAPVIEW_INIT) && !(zmap_view->window_list)) ;

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
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

	  zmap_view->state = ZMAPVIEW_NOT_CONNECTED ;

	  /* Start polling function that checks state of this view and its connections, note
	   * that at this stage there is no data to display. */
	  startStateConnectionChecking(zmap_view) ;

	  zmapViewBusy(zmap_view, FALSE) ;
	}
    }

  return view_window ;
}


/* Copies an existing window in a view.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				  ZMapWindow copy_window, ZMapWindowLockType window_locking)
{
  ZMapViewWindow view_window = NULL ;

  /* the view _must_ already have a window. */
  zMapAssert(zmap_view && parent_widget
	     && (zmap_view->state != ZMAPVIEW_INIT) && zmap_view->window_list) ;


  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      view_window = createWindow(zmap_view, NULL) ;

      zmapViewBusy(zmap_view, TRUE) ;

      if (!(view_window->window = zMapWindowCopy(parent_widget, zmap_view->sequence,
						 view_window, copy_window,
						 zmap_view->features, zmap_view->types,
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





/* Removes a view from a window, the window may be the last/only window in the view. */
void zMapViewRemoveWindow(ZMapViewWindow view_window)
{
  ZMapView zmap_view = view_window->parent_view ;


  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      /* We should check the window is in the list of windows for that view and abort if
       * its not........ */

      destroyWindow(zmap_view, view_window) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* would like to set state here but can't, we should probably just terminate when there
       * no windows left, we should make that a condition really of the zmap.... */

      if (!zmap_view->window_list)
	zmap_view->state = undefined ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }



  return ;
}



/* Connect a View to its databases via threads, at this point the View is blank and waiting
 * to be called to load some data. */
gboolean zMapViewConnect(ZMapView zmap_view)
{
  gboolean result = TRUE ;
  ZMapConfigStanzaSet server_list = NULL ;

  if (zmap_view->state != ZMAPVIEW_NOT_CONNECTED)
    {
      /* Probably we should indicate to caller what the problem was here....
       * e.g. if we are resetting then say we are resetting etc.....again we need g_error here. */
      result = FALSE ;
    }
  else
    {
      ZMapConfig config ;
      ZMapCmdLineArgsType value ;
      char *config_file = NULL ;

      zmapViewBusy(zmap_view, TRUE) ;

      config_file = zMapConfigDirGetFile() ;
      if ((config = zMapConfigCreateFromFile(config_file)))
	{
	  ZMapConfigStanza server_stanza ;
	  /* NOTE: If you change this resource array be sure to check that the subsequent
	   * initialisation is still correct. */
	  ZMapConfigStanzaElementStruct server_elements[] = {{"url", ZMAPCONFIG_STRING, {NULL}},
							     {"timeout", ZMAPCONFIG_INT, {NULL}},
							     {"version", ZMAPCONFIG_STRING, {NULL}},
							     {"sequence", ZMAPCONFIG_BOOL, {NULL}},
							     {"writeback", ZMAPCONFIG_BOOL, {NULL}},
							     {"stylesfile", ZMAPCONFIG_STRING, {NULL}},
							     {"format", ZMAPCONFIG_STRING, {NULL}},
							     {NULL, -1, {NULL}}} ;

	  /* Set defaults for any element that is not a string. */
	  zMapConfigGetStructInt(server_elements, "timeout") = 120 ; /* seconds. */
	  zMapConfigGetStructBool(server_elements, "sequence") = FALSE ;
	  zMapConfigGetStructBool(server_elements, "writeback") = FALSE ;

	  server_stanza = zMapConfigMakeStanza("source", server_elements) ;

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
	      char *version, *styles_file, *format, *url ;
	      int timeout, url_parse_error ;
              struct url *url_struct;
	      gboolean sequence_server, writeback_server ;
	      ZMapViewConnection view_con ;

	      url = zMapConfigGetElementString(next_server, "url") ;
	      format = zMapConfigGetElementString(next_server, "format") ;
	      timeout = zMapConfigGetElementInt(next_server, "timeout") ;
	      version = zMapConfigGetElementString(next_server, "version") ;
	      styles_file = zMapConfigGetElementString(next_server, "stylesfile") ;

              /* url is absolutely required. Go on to next stanza if there isn't one.
               * Done before anything else so as not to set seq/write servers to invalid locations  */
              if(!url){
                zMapLogWarning("GUI: %s", "computer says no url specified");
                continue;
              }

	      /* We only record the first sequence and writeback servers found, this means you
	       * can only have one each of these which seems sensible. */
	      if (!zmap_view->sequence_server)
		sequence_server = zMapConfigGetElementBool(next_server, "sequence") ;
	      if (!zmap_view->writeback_server)
		writeback_server = zMapConfigGetElementBool(next_server, "writeback") ;

              /* Parse the url, only here if there is a url to parse */
              url_struct = url_parse(url, &url_parse_error);
              if (!url_struct)
                {
                  zMapLogWarning("GUI: url %s did not parse. Parse error < %s >\n",
                                 url,
                                 url_error(url_parse_error)) ;
                }
              else if (url_struct && (view_con = createConnection(zmap_view, url_struct,
						    format,
						    timeout, version, styles_file,
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

		  if (!zMapFeatureTypeSetAugment(&(zmap_view->types), &(view_con->types)))
		    zMapLogCritical("Could not merge types for server %s into existing types.", 
                                    url_struct->host) ;
		}
	      else
		{
                  zMapLogWarning("GUI: url %s looks ok, host: %s\nport: %d....\n",
                                 url_struct->url, 
                                 url_struct->host, 
                                 url_struct->port) ; 
		  zMapLogWarning("Could not connect to server on %s, port %d", 
                                 url_struct->host, 
                                 url_struct->port) ;
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
	  zmap_view->state = ZMAPVIEW_RUNNING ;
	}
      else
	{
	  zmap_view->state = ZMAPVIEW_NOT_CONNECTED ;
	}

    }

  return result ;
}


/* Signal threads that we want data to stick into the ZMap */
gboolean zMapViewLoad(ZMapView zmap_view)
{
  gboolean result = FALSE ;

  /* Perhaps we should return a "wrong state" warning here.... */
  if (zmap_view->state == ZMAPVIEW_RUNNING)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      loadDataConnections(zmap_view) ;

      result = TRUE ;
    }

  return result ;
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
  gboolean result = TRUE ;

  if (zmap_view->state != ZMAPVIEW_RUNNING)
    result = FALSE ;
  else
    {
      zmapViewBusy(zmap_view, TRUE) ;

      zmap_view->state = ZMAPVIEW_RESETTING ;

      /* Reset all the windows to blank. */
      resetWindows(zmap_view) ;

      /* We need to destroy the existing thread connection and wait until user loads a new
	 sequence. */
      killConnections(zmap_view) ;
    }

  return TRUE ;
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

  if (zmap_view->state != ZMAPVIEW_DYING)
    features = zmap_view->features ;

  return features ;
}

/* N.B. we don't exclude ZMAPVIEW_DYING because caller may want to know that ! */
ZMapViewState zMapViewGetStatus(ZMapView zmap_view)
{
  return zmap_view->state ;
}

char *zMapViewGetStatusStr(ZMapViewState state)
{
  /* Array must be kept in synch with ZmapState enum in ZMap.h */
  static char *zmapStates[] = {"ZMAPVIEW_INIT", "ZMAPVIEW_NOT_CONNECTED", "ZMAPVIEW_RUNNING",
			       "ZMAPVIEW_RESETTING", "ZMAPVIEW_DYING"} ;
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
  zMapFeatureDump(view_window->parent_view->features, file) ;

  return;
}


/* Called to kill a zmap window and get the associated threads killed, note that
 * this call just signals everything to die, its the checkConnections() routine
 * that really clears up and when everything has died signals the caller via the
 * callback routine that they supplied when the view was created.
 */
gboolean zMapViewDestroy(ZMapView zmap_view)
{

  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      zmapViewBusy(zmap_view, TRUE) ;

      /* All states except init have GUI components which need to be destroyed. */
      if (zmap_view->state != ZMAPVIEW_INIT)
	killGUI(zmap_view) ;

      /* If we are in init state or resetting then the connections have already being killed. */
      if (zmap_view->state != ZMAPVIEW_INIT && zmap_view->state != ZMAPVIEW_NOT_CONNECTED
	  && zmap_view->state != ZMAPVIEW_RESETTING)
	killConnections(zmap_view) ;

      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap_view->state = ZMAPVIEW_DYING ;
    }

  return TRUE ;
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
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->enter))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


static void leaveCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not currently used because we are doing "click to focus" at the moment. */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->leave))(view_window, view_window->parent_view->app_data, NULL) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


static void scrollCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

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


  if (select_item->item)
    {
      GList* list_item ;

      /* Highlight the feature in all windows. */
      list_item = g_list_first(view_window->parent_view->window_list) ;

      do
	{
	  ZMapViewWindow view_window ;
	  FooCanvasItem *item ;

	  view_window = list_item->data ;

	  item = zMapWindowFindFeatureItemByItem(view_window->window, select_item->item) ;
	    
	  zMapWindowHighlightObject(view_window->window, item) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }


  /* Pass back a ZMapViewWindow as it has both the View and the window to our caller. */
  (*(view_cbs_G->select))(view_window, view_window->parent_view->app_data, (void *)select_item->text) ;

  return ;
}


/* DOES THIS EVER ACTUALLY HAPPEN........... */
/* Called when an underlying window is destroyed. */
static void destroyCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  printf("In View, in window destroyed callback\n") ;

  return ;
}




/*
 *  ------------------- Internal functions -------------------
 */


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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmap_view->parent_widget = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  zmap_view->window_list = zmap_view->connection_list = NULL ;

  zmap_view->app_data = app_data ;


  /* Hack to read methods from a file in $HOME/.ZMap for now.....really we should be getting
   * this from a server and updating it for information from different servers. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmap_view->types = getTypesFromFile() ;
  zMapAssert(zmap_view->types) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return zmap_view ;
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


/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopStateConnectionChecking(ZMapView zmap_view)
{
  gtk_timeout_remove(zmap_view->idle_handle) ;

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
 *  */
static gboolean checkStateConnections(ZMapView zmap_view)
{
  gboolean call_again = TRUE ;				    /* Normally we want to called continuously. */
  GList* list_item ;
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
		  ;					    /* nothing to do. */
		}
	      else if (reply == ZMAPTHREAD_REPLY_GOTDATA)
		{
		  view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;

		  if (zmap_view->state == ZMAPVIEW_RUNNING)
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
			      zMapViewGetStatusStr(zMapViewGetStatus(zmap_view))) ;

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
		  /* I'm not sure we need to do anything here as now this loop is "inside" a
		   * zmap, we should already chopping the zmap threads outside of this routine,
		   * so I think logically this cannot happen...???? */

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
  if (!zmap_view->connection_list)
    {
      if (zmap_view->state == ZMAPVIEW_INIT || zmap_view->state == ZMAPVIEW_NOT_CONNECTED)
	{
	  /* Nothing to do here I think.... */

	}
      else if (zmap_view->state == ZMAPVIEW_RUNNING)
	{
	  if (threads_have_died)
	    {
	      /* Threads have died because of their own errors....but the ZMap is not dying so
	       * reset state to init and we should return an error here....(but how ?), we 
	       * should not be outputting messages....I think..... */
	      zMapWarning("%s", "Cannot show ZMap because server connections have all died") ;

	      /* THIS IMPLIES WE NEED A "RESET" BUTTON WHICH IS NOT IMPLEMENTED AT THE MOMENT...
	       * PUT SOME THOUGHT INTO THIS.... */
	      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      killGUI(zmap_view) ;
	      destroyZMapView(zmap_view) ;
	      call_again = FALSE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	      /* need to reset here..... */

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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      call_again = FALSE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else if (zmap_view->state == ZMAPVIEW_DYING)
	{

	  /* this is probably the place to callback to zmapcontrol.... */
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
					   struct url *url, char *format,
					   int timeout, char *version, char *styles_file,
					   gboolean sequence_server, gboolean writeback_server,
					   char *sequence, int start, int end)
{
  ZMapViewConnection view_con = NULL ;
  GData *types ;
  ZMapThread thread ;
  gboolean status = FALSE ;

  /* Create the thread to service the connection requests, we give it a function that it will call
   * to decode the requests we send it and a terminate function. */
  if (styles_file)
    {
      char *directory ;
      char *filepath ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if ((directory = zMapGetControlFileDir((char *)zMapGetControlDirName()))
	  && (filepath = zMapGetFile(directory, styles_file))
	  && !(types = zMapFeatureTypeGetFromFile(filepath)))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	if (!(filepath = zMapConfigDirFindFile(styles_file))
	    || !(types = zMapFeatureTypeGetFromFile(filepath)))
	{
	  zMapLogWarning("Could not read types from \"stylesfile\" %s", filepath) ;
	  status = FALSE ;
	}
      else
	status = TRUE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (directory)
	g_free(directory) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      if (filepath)
	g_free(filepath) ;
    }

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

      open_load->context.sequence = g_strdup(sequence) ;
      open_load->context.start = start ;
      open_load->context.end = end ;
      open_load->context.types = types ;

      if (sequence_server)
	open_load->features.type = ZMAP_SERVERREQ_FEATURE_SEQUENCE ;
      else
	open_load->features.type = ZMAP_SERVERREQ_FEATURES ;


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




/* set all the windows attached to this view to blank. */
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

      zMapWindowDisplayData(view_window->window, all_features, new_features, zmap_view->types) ;
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



/* Temporary....needs to go in some feature handling package.... */
static void freeContext(ZMapFeatureContext feature_context)
{
  zMapAssert(feature_context) ;

  if (feature_context->feature_sets)
    g_datalist_clear(&(feature_context->feature_sets)) ;

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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      freeContext(new_features) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Signal the ZMap that there is work to be done. */
      displayDataWindows(zmap_view, zmap_view->features, diff_context) ;

      /* signal our caller that we have data. */
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data, NULL) ;

    }

  return ;
}



static void visibilityChangeCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  /* signal our caller that something has changed. */
  (*(view_cbs_G->visibility_change))(view_window, view_window->parent_view->app_data, window_data) ;

  return;
}



/* I REALLY DON'T KNOW WHAT TO SAY ABOUT THIS BIT OF CODE, WHAT ON EARTH IS IT TRYING
 * TO ACHIEVE...WHAT ARE ALL THESE ACCESSOR FUNCTIONS DOING (zMapWindowGetTypeName ETC)
 * THIS SEEMS COMPLETELY OUT OF WHACK........... */

/* When a window is split, we set the zoom status of all windows
 */
static void setZoomStatusCB(ZMapWindow window, void *caller_data, void *window_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  g_list_foreach(view_window->parent_view->window_list, setZoomStatus, NULL) ;

  return;
}

static void setZoomStatus(gpointer data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data ;

  zMapWindowSetMinZoom(view_window->window) ;
  zMapWindowSetZoomStatus(view_window->window) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Would be better to have a function that just scrolls to the current focus item...?? */

  if (zMapWindowGetFocusQuark(view_window->window))
    zMapWindowScrollToItem(view_window->window, 
			   zMapWindowGetTypeName(view_window->window), 
			   zMapWindowGetFocusQuark(view_window->window)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  
  return;
}




