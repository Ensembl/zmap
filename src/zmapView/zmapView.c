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
 * Last edited: Dec 13 14:19 2004 (edgrif)
 * Created: Thu May 13 15:28:26 2004 (edgrif)
 * CVS info:   $Id: zmapView.c,v 1.37 2004-12-13 15:20:11 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapConn.h>
#include <ZMap/zmapProtocol.h>
#include <ZMap/zmapWindow.h>
#include <zmapView_P.h>



static ZMapView createZMapView(char *sequence, int start, int end, void *app_data) ;
static void destroyZMapView(ZMapView zmap) ;

static gint zmapIdleCB(gpointer cb_data) ;

/* I'm not sure if we need this anymore, I think we will just do it with multiple callbacks... */
static void zmapWindowCB(void *cb_data, int reason) ;

static void scrollCB(ZMapWindow window, void *caller_data) ;
static void clickCB(ZMapWindow window, void *caller_data, ZMapFeature feature) ;
static void setZoomStatusCB(ZMapWindow window, void *caller_data);
static void destroyCB(ZMapWindow window, void *caller_data) ;

static void setZoomStatus(gpointer data, gpointer user_data);

static void startStateConnectionChecking(ZMapView zmap_view) ;
static void stopStateConnectionChecking(ZMapView zmap_view) ;
static gboolean checkStateConnections(ZMapView zmap_view) ;

static void loadDataConnections(ZMapView zmap_view) ;

static void killZMapView(ZMapView zmap_view) ;
static void killGUI(ZMapView zmap_view) ;
static void killConnections(ZMapView zmap_view) ;

static ZMapViewConnection createConnection(ZMapView zmap_view,
					   char *machine, int port, char *protocol,
					   int timeout, char *version,
					   char *sequence, int start, int end, gboolean load_features) ;
static void destroyConnection(ZMapViewConnection *view_conn) ;

static void resetWindows(ZMapView zmap_view) ;
static void displayDataWindows(ZMapView zmap_view,
			       ZMapFeatureContext all_features, ZMapFeatureContext new_features) ;
static void killWindows(ZMapView zmap_view) ;


static void freeContext(ZMapFeatureContext feature_context) ;

static void getFeatures(ZMapView zmap_view, ZMapProtocolAny req_any) ;

/* debugging... */
static void methodPrintFunc(GQuark key_id, gpointer data, gpointer user_data) ;


/* Hack to read files in users $HOME/.ZMap */
static GData *getTypesFromFile(void) ;





/* These callback routines are static because they are set just once for the lifetime of the
 * process. */

/* Callbacks we make back to the level above us. */
static ZMapViewCallbacks view_cbs_G = NULL ;

/* Callbacks back to us from the level below, i.e. zMapWindow. */
ZMapWindowCallbacksStruct window_cbs_G = {scrollCB, clickCB, setZoomStatusCB, destroyCB} ;



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

  zMapAssert(callbacks && callbacks->load_data && callbacks->click && callbacks->destroy) ;

  view_cbs_G = g_new0(ZMapViewCallbacksStruct, 1) ;

  view_cbs_G->load_data = callbacks->load_data ;
  view_cbs_G->click = callbacks->click ;
  view_cbs_G->destroy = callbacks->destroy ;


  /* Init windows.... */
  zMapWindowInit(&window_cbs_G) ;


  return ;
}



void zmapViewFeatureDump(ZMapViewWindow view_window, char *file, int format)
{
  zMapFeatureDump(view_window->parent_view->features, file, format);

  return;
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
  if (zMapUtilsConfigDebug(ZMAPTHR_CONFIG_DEBUG_STR, &debug))
    zmap_thr_debug_G = debug ;

  zmap_view = createZMapView(sequence, start, end, app_data) ;

  zmap_view->state = ZMAPVIEW_INIT ;

  return zmap_view ;
}


/* Adds a window to a view, the view may not have a window yet.
 * Returns the window on success, NULL on failure. */
ZMapViewWindow zMapViewAddWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				 ZMapWindow zMapWindow)
{
  ZMapViewWindow view_window ;
  double zoom_factor;


  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      view_window = g_new0(ZMapViewWindowStruct, 1) ;
      view_window->parent_view = zmap_view ;		    /* back pointer. */

      if (zMapWindow)
	view_window->window = zMapWindowCopy(parent_widget, zmap_view->sequence, 
					     view_window, zMapWindow);
      else
	view_window->window = zMapWindowCreate(parent_widget, zmap_view->sequence, 
					       view_window);
      
      if (view_window->window)
	{
	  /* set the zoom_factor of the new window the same as the previous one */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* this is wrong, it will change all the time, question is...what is the parent, it can
	   * change.....this is why we need an intermediate widget that does not change.... */
	  zmap_view->parent_widget = parent_widget ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  /* add to list of windows.... */
	  zmap_view->window_list = g_list_append(zmap_view->window_list, view_window) ;

	  /* There is a bit of a hole in the "busy" state here in that we do the windowcreate
	   * but have not signalled that we are busy, I suppose we could do the busy stuff twice,
	   * once before the windowCreate and once here to make sure we set cursors everywhere.. */
	  zmapViewBusy(zmap_view, TRUE) ;

	  /* We may be the first window to added, or there may already be others. */
	  if (zmap_view->state == ZMAPVIEW_INIT)
	    {
	      zmap_view->state = ZMAPVIEW_NOT_CONNECTED ;

	      /* Start polling function that checks state of this view and its connections, note
	       * that at this stage there is no data to display. */
	      startStateConnectionChecking(zmap_view) ;
	    }
	  else if (zmap_view->state == ZMAPVIEW_RUNNING)
	    {
	      /* If we are running then we should display the current data in the new window we
	       * have just created. */
	      zMapWindowDisplayData(view_window->window, zmap_view->features, NULL, 
				    zmap_view->types, zmap_view) ;
	    }


	  zmapViewBusy(zmap_view, FALSE) ;
	}
      else
	{
	  /* should glog and/or gerror at this stage....really need g_errors.... */
	  g_free(view_window) ;
	  view_window = NULL ;
	}
    }

  return view_window ;
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

      /* this should be user settable.... */
      gboolean load_features = TRUE ;

      zmapViewBusy(zmap_view, TRUE) ;

      /* We need to retrieve the connect data here from the config stuff.... */
      if (result && (config = zMapConfigCreate()))
	{
	  ZMapConfigStanza server_stanza ;
	  /* If you change this resource array be sure to check that the subesequent
	   * initialisation is still correct. */
	  ZMapConfigStanzaElementStruct server_elements[] = {{"host", ZMAPCONFIG_STRING, {NULL}},
							     {"port", ZMAPCONFIG_INT, {NULL}},
							     {"protocol", ZMAPCONFIG_STRING, {NULL}},
							     {"timeout", ZMAPCONFIG_INT, {NULL}},
							     {"version", ZMAPCONFIG_STRING, {"4.9.28"}},
							     {NULL, -1, {NULL}}} ;

	  /* Set defaults for any element that is not a string. */
	  server_elements[1].data.i = -1 ;
	  server_elements[3].data.i = 120 ;		    /* timeout in seconds. */

	  server_stanza = zMapConfigMakeStanza("server", server_elements) ;

	  if (!zMapConfigFindStanzas(config, server_stanza, &server_list))
	    result = FALSE ;

	  zMapConfigDestroyStanza(server_stanza) ;
	}

      /* Set up connections to the named servers. */
      if (result)
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
	      char *machine, *protocol ;
	      int port, timeout ;
	      char *version ;
	      ZMapViewConnection view_con ;

	      /* There MUST be a host and a protocol, port is not needed for some protocols,
	       * e.g. http, because it should be allowed to default. */
	      machine = zMapConfigGetElementString(next_server, "host") ;
	      port = zMapConfigGetElementInt(next_server, "port") ;
	      protocol = zMapConfigGetElementString(next_server, "protocol") ;
	      timeout = zMapConfigGetElementInt(next_server, "timeout") ;
	      version = zMapConfigGetElementString(next_server, "version") ;

	      if (!machine || !protocol)
		{
		  zMapLogWarning("%s",
				 "Found \"server\" stanza without valid \"host\" or \"protocol\", "
				 "stanza was ignored.") ;
		}
	      else if ((view_con = createConnection(zmap_view, machine, port, protocol,
						    timeout, version,
						    zmap_view->sequence,
						    zmap_view->start, zmap_view->end,
						    load_features)))
		{
		  zmap_view->connection_list = g_list_append(zmap_view->connection_list, view_con) ;
		  connections++ ;
		}
	      else
		{
		  zMapLogWarning("Could not connect to %s protocol server "
				 "on %s, port %d", protocol, machine, port) ;
		}
	    }

	  /* Ought to return a gerror here........ */
	  if (!connections)
	    result = FALSE ;
	}

      /* clean up. */
      if (server_list)
	zMapConfigDeleteStanzaSet(server_list) ;

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

      /* If we are loading features then we need to visually tell the user we are doing this. */
      if (!load_features)
	zmapViewBusy(zmap_view, FALSE) ;
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



static void scrollCB(ZMapWindow window, void *caller_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  printf("In View, in window scroll callback\n") ;


  return ;
}


static void clickCB(ZMapWindow window, void *caller_data, ZMapFeature feature)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;


  /* Is there any focus stuff we want to do here ??? */

  /* Pass back a ZMapViewWindow as it has both the View and the window. */
  (*(view_cbs_G->click))(view_window, view_window->parent_view->app_data, feature) ;

  return ;
}


/* DOES THIS EVER ACTUALLY HAPPEN........... */
/* Called when an underlying window is destroyed. */
static void destroyCB(ZMapWindow window, void *caller_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)caller_data ;

  printf("In View, in window destroyed callback\n") ;

  return ;
}



/* This routine needs some work, with the reorganisation of the code it is no longer correct,
 * it depends on what the zmapWindow can return to us. */
/* Gets called when zmap Window code needs to signal us that the user has done something
 * such as click "quit" BUT THIS MAY NO LONGER BE POSSIBLE FOR A VIEW WINDOW...UNLESS WE HAVE.
 * A MENU THAT ALLOWS THINGS TO BE KILLED. */
static void zmapWindowCB(void *cb_data, int reason)
{
  ZMapView zmap_view = (ZMapView)cb_data ;
  ZmapWindowCmd window_cmd = (ZmapWindowCmd)reason ;
  char *debug ;

  if (window_cmd == ZMAP_WINDOW_QUIT)
    {
      debug = "ZMAP_WINDOW_QUIT" ;

      zMapViewDestroy(zmap_view) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Must come later now that destruction is deferred ??? */
      (*(view_cbs_G->destroy_cb))(zmap_view, zmap_view->app_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  else if (window_cmd == ZMAP_WINDOW_LOAD)
    {
      debug = "ZMAP_WINDOW_LOAD" ;

      zMapViewLoad(zmap_view) ;
    }
  else if (window_cmd == ZMAP_WINDOW_STOP)
    {
      debug = "ZMAP_WINDOW_STOP" ;

      zMapViewReset(zmap_view) ;
    }


  zMapDebug("GUI: received %s from zmap window\n", debug) ;

  return ;
}



/* I KNOW NOTHING ABOUT ANY OF THIS IS ABOUT.....OR EVEN IF IT IS IN THE RIGHT PLACE.... */

int zMapViewGetRegionLength(ZMapView view)
{
  return view->zMapRegion->length;
}

Coord zMapViewGetRegionArea(ZMapView view, int num)
{
  if (num == 1)
    return view->zMapRegion->area1;
  else
    return view->zMapRegion->area2;
}

void zMapViewSetRegionArea(ZMapView view, Coord area, int num)
{
  if (num == 1)
    view->zMapRegion->area1 = area;
  else
    view->zMapRegion->area2 = area;

return;
}

gboolean zMapViewGetRegionReverse(ZMapView view)
{
  return view->zMapRegion->rootIsReverse;
}


int zMapViewGetRegionSize (ZMapView view)
{
  return view->zMapRegion->area2 - view->zMapRegion->area1;
}


ZMapRegion *zMapViewGetZMapRegion(ZMapView view)
{
  return view->zMapRegion;
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
  zmap_view->types = getTypesFromFile() ;
  zMapAssert(zmap_view->types) ;


  return zmap_view ;
}


/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. The lists of windows and thread connections are dealt with somewhat
 * asynchronously by killWindows() & checkConnections() */
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
  zmap_view->idle_handle = gtk_idle_add(zmapIdleCB, (gpointer)zmap_view) ;

  return ;
}


/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopStateConnectionChecking(ZMapView zmap_view)
{
  gtk_idle_remove(zmap_view->idle_handle) ;

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
	  ZMapConnection connection ;
	  ZMapThreadReply reply ;
	  void *data = NULL ;
	  char *err_msg = NULL ;
      
	  view_con = list_item->data ;
	  connection = view_con->connection ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  ZMAP_DEBUG("GUI: checking connection for thread %x\n",
		     zMapConnGetThreadid(connection)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
	   * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
	   * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
	   * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

	  data = NULL ;
	  err_msg = NULL ;
	  if (!(zMapConnGetReplyWithData(connection, &reply, &data, &err_msg)))
	    {
	      zMapDebug("GUI: thread %x, cannot access reply from thread - %s\n",
			zMapConnGetThreadid(connection), err_msg) ;

	      /* should abort or dump here....or at least kill this connection. */

	    }
	  else
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      zMapDebug("GUI: thread %x, thread reply = %s\n",
			zMapConnGetThreadid(connection),
			zMapVarGetReplyString(reply)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      
	      if (reply == ZMAP_REPLY_WAIT)
		{
		  ;					    /* nothing to do. */
		}
	      else if (reply == ZMAP_REPLY_GOTDATA)
		{
		  view_con->curr_request = ZMAP_REQUEST_WAIT ;

		  if (zmap_view->state == ZMAPVIEW_RUNNING)
		    {
		      ZMapProtocolAny req_any = (ZMapProtocolAny)data ;

		      zMapDebug("GUI: thread %x, got data\n",
				zMapConnGetThreadid(connection)) ;

		      /* Reset the reply from the slave. */
		      zMapConnSetReply(connection, ZMAP_REPLY_WAIT) ;
		  

		      /* Process the different types of data coming back. */
		      switch (req_any->request)
			{
			case ZMAP_PROTOCOLREQUEST_FEATURES:
			case ZMAP_PROTOCOLREQUEST_FEATURE_SEQUENCE:
			case ZMAP_PROTOCOLREQUEST_SEQUENCE:
			  {
			    getFeatures(zmap_view, req_any) ;

			    break ;
			  }
			default:
			  {	  
			    zMapLogFatal("Unknown request type: %d", req_any->request) ; /* Exit appl. */
			    break ;
			  }
			}

		      /* Free the request block. */
		      g_free(data) ;
		    }
		  else
		    zMapDebug("GUI: thread %x, got data but ZMap state is - %s\n",
			      zMapConnGetThreadid(connection), zMapViewGetStatus(zmap_view)) ;

		}
	      else if (reply == ZMAP_REPLY_REQERROR)
		{
		  if (err_msg)
		    zMapWarning("%s", err_msg) ;

		  view_con->curr_request = ZMAP_REQUEST_WAIT ;

		  /* Reset the reply from the slave. */
		  zMapConnSetReply(connection, ZMAP_REPLY_WAIT) ;

		  /* This means the request failed for some reason. */
		  zMapDebug("GUI: thread %x, request to server failed....\n",
			    zMapConnGetThreadid(connection)) ;

		  g_free(err_msg) ;
		}
	      else if (reply == ZMAP_REPLY_DIED)
		{
		  if (err_msg)
		    zMapWarning("%s", err_msg) ;

		  threads_have_died = TRUE ;

		  /* This means the thread has failed for some reason and we should clean up. */
		  zMapDebug("GUI: thread %x has died so cleaning up....\n",
			    zMapConnGetThreadid(connection)) ;
		  
		  /* We are going to remove an item from the list so better move on from
		   * this item. */
		  list_item = g_list_next(list_item) ;
		  zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;

		  destroyConnection(&view_con) ;
		}
	      else if (reply == ZMAP_REPLY_CANCELLED)
		{
		  /* I'm not sure we need to do anything here as now this loop is "inside" a
		   * zmap, we should already chopping the zmap threads outside of this routine,
		   * so I think logically this cannot happen...???? */

		  /* This means the thread was cancelled so we should clean up..... */
		  zMapDebug("GUI: thread %x has been cancelled so cleaning up....\n",
			    zMapConnGetThreadid(connection)) ;

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
	  (*(view_cbs_G->destroy))(zmap_view, zmap_view->app_data) ;

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
	  ZMapConnection connection ;
	  ZMapProtocoltGetFeatures req_features ;

	  view_con = list_item->data ;
	  connection = view_con->connection ;

	  /* Need to construct a request to do the load of data... */
	  req_features = g_new0(ZMapProtocolGetFeaturesStruct, 1) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  req_features->request = ZMAP_PROTOCOLREQUEST_FEATURES ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  req_features->request = ZMAP_PROTOCOLREQUEST_FEATURE_SEQUENCE ;


	  zMapConnRequest(connection, req_features) ;

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
  killWindows(zmap_view) ;

  return ;
}


static void killConnections(ZMapView zmap_view)
{
  GList* list_item ;

  list_item = g_list_first(zmap_view->connection_list) ;

  do
    {
      ZMapViewConnection view_con ;
      ZMapConnection connection ;

      view_con = list_item->data ;
      connection = view_con->connection ;

      /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
       * will actually die sometime later. */
      zMapConnKill(connection) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}


/* Allocate and destroy ZMapViews record of a connection. */
static ZMapViewConnection createConnection(ZMapView zmap_view,
					   char *machine, int port, char *protocol,
					   int timeout, char *version,
					   char *sequence, int start, int end, gboolean load_features)
{
  ZMapViewConnection view_con = NULL ;
  ZMapConnection connection ;

  if ((connection = zMapConnCreate(machine, port, protocol, timeout, version,
				   sequence, start, end, zmap_view->types, load_features)))
    {
      view_con = g_new0(ZMapViewConnectionStruct, 1) ;
      view_con->parent_view = zmap_view ;

      /* If we are asking the thread to get the features when it starts up then we must record
       * that we have made a request. */
      if (load_features)
	view_con->curr_request = ZMAP_REQUEST_GETDATA ;
      else
	view_con->curr_request = ZMAP_REQUEST_WAIT ;

      view_con->connection = connection ;
    }

  return view_con ;
}

static void destroyConnection(ZMapViewConnection *view_conn_ptr)
{
  ZMapViewConnection view_conn = *view_conn_ptr ;

  zMapConnDestroy(view_conn->connection) ;

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

      zMapWindowDisplayData(view_window->window,
			    all_features, new_features, zmap_view->types, NULL) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}


/* Kill all the windows... */
static void killWindows(ZMapView zmap_view)
{

  while (zmap_view->window_list)
    {
      ZMapViewWindow view_window ;

      view_window = zmap_view->window_list->data ;

      zMapWindowDestroy(view_window->window) ;

      zmap_view->window_list = g_list_remove(zmap_view->window_list, view_window) ;
    }


  return ;
}




/* Temporary....needs to go in some feature handling package.... */
static void freeContext(ZMapFeatureContext feature_context)
{
  zMapAssert(feature_context) ;

  if (feature_context->sequence)
    g_free(feature_context->sequence) ;

  if (feature_context->parent)
    g_free(feature_context->parent) ;

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
static void getFeatures(ZMapView zmap_view, ZMapProtocolAny req_any)
{
  ZMapProtocoltGetFeatures feature_req = (ZMapProtocoltGetFeatures)req_any ;
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
      (*(view_cbs_G->load_data))(zmap_view, zmap_view->app_data) ;

    }

  return ;
}











/* This is a temporary routine to read type/method/source (call it what you will)
 * information from a file in the users $HOME/.ZMap directory. */
static GData *getTypesFromFile(void)
{
  GData *types = NULL ;
  gboolean result = FALSE ;
  ZMapConfigStanzaSet types_list = NULL ;
  ZMapConfig config ;
  char *types_file_name = "ZMapTypes" ;


  if ((config = zMapConfigCreateFromFile(NULL, types_file_name)))
    {
      ZMapConfigStanza types_stanza ;

      /* Set up default values for variables in the stanza.  Elements here must match
       * those being loaded below or you might segfault.
       * IF YOU ADD ANY ELEMENTS TO THIS ARRAY THEN MAKE SURE YOU UPDATE THE INIT STATEMENTS
       * FOLLOWING THIS ARRAY SO THEY POINT AT THE RIGHT ELEMENTS....!!!!!!!! */
      ZMapConfigStanzaElementStruct types_elements[] = {{"name"        , ZMAPCONFIG_STRING, {NULL}},
							{"outline"     , ZMAPCONFIG_STRING, {"black"}},
							{"foreground"  , ZMAPCONFIG_STRING, {"white"}},
							{"background"  , ZMAPCONFIG_STRING, {"black"}},
							{"width"       , ZMAPCONFIG_FLOAT , {NULL}},
							{"showUpStrand", ZMAPCONFIG_BOOL  , {NULL}},
							{"minmag"      , ZMAPCONFIG_INT   , {NULL}},
							{NULL, -1, {NULL}}} ;


      types_elements[4].data.f = DEFAULT_WIDTH ;	    /* Must init separately as compiler
							       cannot statically init different
							       union types....sigh.... */

      types_stanza = zMapConfigMakeStanza("Type", types_elements) ;

      if (!zMapConfigFindStanzas(config, types_stanza, &types_list))
	result = FALSE ;
      else
	result = TRUE ;
    }

  /* Set up connections to the named types. */
  if (result)
    {
      int num_types = 0 ;
      ZMapConfigStanza next_types ;

      g_datalist_init(&types) ;

      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      next_types = NULL ;
      while (result
	     && ((next_types = zMapConfigGetNextStanza(types_list, next_types)) != NULL))
	{
	  char *name;
	  GString *name_mixed, *name_lower;

	  /* Name must be set so if its not found then don't make a struct.... */
	  if ((name = zMapConfigGetElementString(next_types, "name")))
	    {
	      ZMapFeatureTypeStyle new_type = g_new0(ZMapFeatureTypeStyleStruct, 1) ;

	      /* NB Elements here must match those pre-initialised above or you might segfault. */

	      gdk_color_parse(zMapConfigGetElementString(next_types, "outline"   ), &new_type->outline   ) ;
	      gdk_color_parse(zMapConfigGetElementString(next_types, "foreground"), &new_type->foreground) ;
	      gdk_color_parse(zMapConfigGetElementString(next_types, "background"), &new_type->background) ;
	      new_type->width = zMapConfigGetElementFloat(next_types, "width") ;
	      new_type->showUpStrand = zMapConfigGetElementBool(next_types, "showUpStrand");
	      new_type->min_mag = zMapConfigGetElementInt(next_types, "minmag");

	      /* lowercase the name (aka type) */
	      name_mixed = g_string_new(name);
	      name_lower = g_string_ascii_down(name_mixed);
		
	      g_datalist_set_data(&types, name_lower->str, new_type) ;
	      num_types++ ;
	    }
	  else
	    {
	      zMapLogWarning("config file \"%s\" has a \"Type\" stanza which has no \"name\" element, "
			     "the stanza has been ignored.", types_file_name) ;
	    }
	}

      /* Found no valid types.... */
      if (!num_types)
	result = FALSE ;
    }

  /* clean up. */
  if (types_list)
    zMapConfigDeleteStanzaSet(types_list) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug.... */
  printAllTypes(types, "getTypesFromFile()") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return types ;
}


void printAllTypes(GData *method_set, char *user_string)
{
  printf("\nMethods at %s\n", user_string) ;

  g_datalist_foreach(&method_set, methodPrintFunc, NULL) ;

  return ;
}

static void methodPrintFunc(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  char *style_name = (char *)g_quark_to_string(key_id) ;
  
  printf("\t%s: \t%f \t%d\n", style_name, style->width, style->showUpStrand) ;

  return ;
}

/* When a window is split, we set the zoom status of all windows
 */
static void setZoomStatusCB(ZMapWindow window, void *caller_data)
{
  ZMapView view = (ZMapView)caller_data;

  g_list_foreach(view->window_list, setZoomStatus, NULL) ;

  return;
}

static void setZoomStatus(gpointer data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data;

  zMapWindowSetMinZoom(view_window->window);
  zMapWindowSetZoomStatus(view_window->window);
  if (zMapWindowGetFocusQuark(view_window->window))
    zMapWindowScrollToItem(view_window->window, 
			   zMapWindowGetTypeName(view_window->window), 
			   zMapWindowGetFocusQuark(view_window->window));
  
  return;
}

