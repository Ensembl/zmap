/*  File: zmapControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: This is the ZMap interface code, it controls both
 *              the window code and the threaded server code.
 * Exported functions: See ZMap.h
 * HISTORY:
 * Last edited: Mar 12 14:37 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.4 2004-03-12 15:53:23 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <ZMap/zmapUtils.h>
#include <ZMap_P.h>

#include <../zmapThreads/zmapConn_P.h>

/* ZMap debugging output. */
gboolean zmap_debug_G = TRUE ; 


static gint zmapIdleCB(gpointer cb_data) ;
static void zmapWindowCB(void *cb_data, int reason) ;

static ZMap createZMap(char *sequence, void *app_data, ZMapCallbackFunc destroy_cb, ZMapConfig config) ;
static void destroyZMap(ZMap zmap) ;

static void startConnectionChecking(ZMap zmap) ;
static void stopConnectionChecking(ZMap zmap) ;
static gboolean checkConnections(ZMap zmap) ;

static void loadDataConnections(ZMap zmap, char *sequence) ;

static void killZMap(ZMap zmap) ;
static void killGUI(ZMap zmap) ;
static void killConnections(ZMap zmap) ;
static void destroyConnection(ZMapConnection *connection) ;

static gboolean getServer(char *server_string, char **machine, char **port, char **protocol) ;



/*
 *  ------------------- External functions -------------------
 *   (includes callbacks)
 */


/* Create a new/blank zmap with its window, has no thread connections to databases.
 * Returns NULL on failure. */
ZMap zMapCreate(char *sequence, void *app_data, ZMapCallbackFunc destroy_cb, ZMapConfig config)
{
  ZMap zmap = NULL ;

  zmap = createZMap(sequence, app_data, destroy_cb, config) ;

  /* Create the zmap window itself. */
  if (!(zmap->window = zMapWindowCreate(zmap->sequence, zmap->zmap_id, zmapWindowCB, zmap)))
    {
      destroyZMap(zmap) ;
      zmap = NULL ;
    }
  else
    zmap->state = ZMAP_INIT ;

  return zmap ;
}



/* Connect a blank ZMap to its databases via threads,
 * at this point the ZMap is blank and waiting to be told to load some data. */
gboolean zMapConnect(ZMap zmap)
{
  gboolean result = TRUE ;
  char **server_list = NULL ;

  if (zmap->state != ZMAP_INIT)
    {
      /* Probably we should indicate to caller what the problem was here.... */
      result = FALSE ;
    }
  else
    {
      /* We need to retrieve the connect data here from the config stuff.... */
      if (result)
	{
	  if (!(zMapConfigGetServers(zmap->config, &server_list)))
	    result = FALSE ;
	}

      /* Set up connections to the named servers. */
      if (result)
	{
	  char **next = server_list ;
	  int connections = 0 ;

	  /* Error handling is tricky here, if we manage to connect to some servers and not
	   * others, what do we do ?  THINK ABOUT THIS..... */
	  while (result && *next != NULL)
	    {
	      char *machine, *port, *protocol ;
	      ZMapConnection connection ;

	      if (getServer(*next, &machine, &port, &protocol)
		  && (connection = zMapConnCreate(machine, port, protocol, zmap->sequence)))
		{
		  zmap->connection_list = g_list_append(zmap->connection_list, connection) ;
		  connections++ ;
		}
	      else
		{
		  gchar *err_msg ;

		  err_msg = g_strdup_printf("Could not connect to %s protocol server "
					    "on %s, port %s", protocol, machine, port) ;

		  zmapGUIShowMsg(err_msg) ;

		  g_free(err_msg) ;
		}

	      next++ ;
	    }

	  if (!connections)
	    result = FALSE ;
	}

      /* If everything is ok then set up idle routine to check the connections,
       * otherwise signal failure and clean up. */
      if (result)
	{
	  zmap->state = ZMAP_RUNNING ;
	  startConnectionChecking(zmap) ;
	}
      else
	{
	  zmap->state = ZMAP_DYING ;
	  killZMap(zmap) ;
	}
    }

  return result ;
}


/* Signal threads that we want data to stick into the ZMap */
gboolean zMapLoad(ZMap zmap, char *sequence)
{
  gboolean result = TRUE ;

  if (zmap->state == ZMAP_RESETTING || zmap->state == ZMAP_DYING)
    result = FALSE ;
  else
    {
      if (zmap->state == ZMAP_INIT)
	result = zMapConnect(zmap) ;

      if (result)
	{
	  if (sequence && *sequence)
	    {
	      if (zmap->sequence)
		g_free(zmap->sequence) ;
	      zmap->sequence = g_strdup(sequence) ;
	    }

	  loadDataConnections(zmap, zmap->sequence) ;
	}
    }

  return result ;
}


/* Reset an existing ZMap, this call will:
 * 
 *    - leave the ZMap window displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - Free all ZMap window data that was specific to the view being loaded.
 *    - Kill the existing server thread and start a new one from scratch.
 * 
 * After this call the ZMap will be ready for the user to try again with the existing
 * machine/port/sequence or to enter data for a new sequence etc.
 * 
 *  */
gboolean zMapReset(ZMap zmap)
{
  gboolean result = TRUE ;

  if (zmap->state != ZMAP_RUNNING)
    result = FALSE ;
  else
    {
      zmap->state = ZMAP_RESETTING ;

      /* We need a new windows call to reset the window and blank it. */
      zMapWindowReset(zmap->window) ;

      /* We need to destroy the existing thread connection and wait until user loads a new
	 sequence. */
      killConnections(zmap) ;
    }


  return TRUE ;
}



/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id = NULL ;

  if (zmap->state != ZMAP_DYING)
    id = zmap->zmap_id ;

  return id ;
}


char *zMapGetSequence(ZMap zmap)
{
  char *sequence = NULL ;

  if (zmap->state != ZMAP_DYING)
    sequence = zmap->sequence ;

  return sequence ;
}

char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static char *zmapStates[] = {"ZMAP_INIT", "ZMAP_RUNNING", "ZMAP_RESETTING",
			       "ZMAP_DYING", "ZMAP_DEAD"} ;
  char *state ;

  state = zmapStates[zmap->state] ;

  return state ;
}


/* Called to kill a zmap window and get the associated threads killed, note that
 * this call just signals everything to die, its the checkConnections() routine
 * that really clears up.
 */
gboolean zMapDestroy(ZMap zmap)
{
  if (zmap->state != ZMAP_DYING)
    {
      killGUI(zmap) ;

      /* If we are in init state or resetting then the connections have already being killed. */
      if (zmap->state != ZMAP_INIT && zmap->state != ZMAP_RESETTING)
	killConnections(zmap) ;

      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;
    }

  return TRUE ;
}



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing....  */




/* This is really the guts of the code to check what a connection thread is up
 * to. Every time the GUI thread has stopped doing things this routine gets called
 * so then we check our connection for action..... */
static gint zmapIdleCB(gpointer cb_data)
{
  gint call_again = 0 ;
  ZMap zmap = (ZMap)cb_data ;

  /* Returning a value > 0 tells gtk to call zmapIdleCB again, so if checkConnections() returns
   * TRUE we ask to be called again. */
  if (checkConnections(zmap))
    call_again = 1 ;
  else
    call_again = 0 ;

  return call_again ;
}


/* I'M NOT COMPLETELY SURE WE NEED THIS "REASON" STUFF....BUT PERHAPS IT IS A GOOD THING. */
/* Gets called when zmap Window code needs to signal us that the user has done something
 * such as click "quit". */
static void zmapWindowCB(void *cb_data, int reason)
{
  ZMap zmap = (ZMap)cb_data ;
  ZmapWindowCmd window_cmd = (ZmapWindowCmd)reason ;
  char *debug ;

  if (window_cmd == ZMAP_WINDOW_QUIT)
    {
      debug = "ZMAP_WINDOW_QUIT" ;

      zMapDestroy(zmap) ;

      (*(zmap->destroy_zmap_cb))(zmap, zmap->app_data) ;
    }
  else if (window_cmd == ZMAP_WINDOW_LOAD)
    {
      debug = "ZMAP_WINDOW_LOAD" ;

      zMapLoad(zmap, "") ;
    }
  else if (window_cmd == ZMAP_WINDOW_STOP)
    {
      debug = "ZMAP_WINDOW_STOP" ;

      zMapReset(zmap) ;
    }


  ZMAP_DEBUG("GUI: received %s from zmap window\n", debug) ;

  return ;
}




/*
 *  ------------------- Internal functions -------------------
 */


static ZMap createZMap(char *sequence, void *app_data, ZMapCallbackFunc destroy_cb, ZMapConfig config)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new(ZMapStruct, sizeof(ZMapStruct)) ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("%d", zmap_num) ;

  zmap->sequence = g_strdup(sequence) ;

  zmap->config = config ;

  zmap->connection_list = NULL ;

  zmap->app_data = app_data ;
  zmap->destroy_zmap_cb = destroy_cb ;

  return zmap ;
}


/* Should only do this after the window and all threads have gone as this is our only handle
 * to these resources. */
static void destroyZMap(ZMap zmap)
{
  g_free(zmap->zmap_id) ;

  g_free(zmap->sequence) ;

  g_free(zmap) ;

  return ;
}


/* Start and stop the ZMap GTK idle function (gets run when the GUI is doing nothing).
 */
static void startConnectionChecking(ZMap zmap)
{
  zmap->idle_handle = gtk_idle_add(zmapIdleCB, (gpointer)zmap) ;

  return ;
}


/* I think that probably I won't need this most of the time, it could be used to remove the idle
 * function in a kind of unilateral way as a last resort, otherwise the idle function needs
 * to cancel itself.... */
static void stopConnectionChecking(ZMap zmap)
{
  gtk_idle_remove(zmap->idle_handle) ;

  return ;
}




/* This function checks the status of the connection and checks for any reply and
 * then acts on it, it gets called from the ZMap idle function.
 * If all threads are ok and zmap has not been killed then routine returns TRUE
 * meaning it wants to be called again, otherwise FALSE.
 *
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *  */
static gboolean checkConnections(ZMap zmap)
{
  gboolean call_again = TRUE ;				    /* Normally we want to called continuously. */
  GList* list_item ;

  list_item = g_list_first(zmap->connection_list) ;
  /* should assert this as never null.... */

  do
    {
      ZMapConnection connection ;
      ZMapThreadReply reply ;
      void *data = NULL ;
      char *err_msg = NULL ;
      
      connection = list_item->data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMAP_DEBUG("GUI: checking connection for thread %x\n",
		  zMapConnGetThreadid(connection)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      data = NULL ;
      err_msg = NULL ;
      if (!(zMapConnGetReplyWithData(connection, &reply, &data, &err_msg)))
	{
	  ZMAP_DEBUG("GUI: thread %x, cannot access reply from thread - %s\n",
		     zMapConnGetThreadid(connection), err_msg) ;
	}
      else
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  ZMAP_DEBUG("GUI: thread %x, thread reply = %s\n",
		     zMapConnGetThreadid(connection),
		     zMapVarGetReplyString(reply)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      
	  if (reply == ZMAP_REPLY_WAIT)
	    {
	      ;					    /* nothing to do. */
	    }
	  else if (reply == ZMAP_REPLY_GOTDATA)
	    {
	      if (zmap->state == ZMAP_RUNNING)
		{
		  ZMAP_DEBUG("GUI: thread %x, got data\n",
			     zMapConnGetThreadid(connection)) ;

		  /* Is this right....????? check my logic here....  */
		  zMapConnSetReply(connection, ZMAP_REPLY_WAIT) ;
		  
		  /* Signal the ZMap that there is work to be done. */
		  zMapWindowDisplayData(zmap->window, data) ;
		}
	      else
		ZMAP_DEBUG("GUI: thread %x, got data but ZMap state is - %s\n",
			   zMapConnGetThreadid(connection), zMapGetZMapStatus(zmap)) ;

	    }
	  else if (reply == ZMAP_REPLY_DIED)
	    {
	      if (err_msg)
		zmapGUIShowMsg(err_msg) ;


	      /* This means the thread has failed for some reason and we should clean up. */
	      ZMAP_DEBUG("GUI: thread %x has died so cleaning up....\n",
			 zMapConnGetThreadid(connection)) ;
		  
	      /* We are going to remove an item from the list so better move on from
	       * this item. */
	      list_item = g_list_next(list_item) ;
	      zmap->connection_list = g_list_remove(zmap->connection_list, connection) ;

	      destroyConnection(&connection) ;
	    }
	  else if (reply == ZMAP_REPLY_CANCELLED)
	    {
	      /* I'm not sure we need to do anything here as now this loop is "inside" a
	       * zmap, we should already chopping the zmap threads outside of this routine,
	       * so I think logically this cannot happen...???? */

	      /* This means the thread was cancelled so we should clean up..... */
	      ZMAP_DEBUG("GUI: thread %x has been cancelled so cleaning up....\n",
			 zMapConnGetThreadid(connection)) ;

	      /* We are going to remove an item from the list so better move on from
	       * this item. */
	      list_item = g_list_next(list_item) ;
	      zmap->connection_list = g_list_remove(zmap->connection_list, connection) ;

	      destroyConnection(&connection) ;
	    }
	}
    }
  while ((list_item = g_list_next(list_item))) ;


  /* At this point if there are no threads left we need to examine our state and take action
   * depending on whether we are dying or threads have died or whatever.... */
  if (!zmap->connection_list)
    {
      if (zmap->state == ZMAP_DYING)
	{
	  /* zmap was waiting for threads to die, now they have we can free everything and stop. */
	  destroyZMap(zmap) ;

	  call_again = FALSE ;
	}
      else if (zmap->state == ZMAP_RESETTING)
	{
	  /* zmap is ok but user has reset it and all threads have died so we need to stop
	   * checking.... */
	  zmap->state = ZMAP_INIT ;

	  call_again = FALSE ;
	}
      else
	{
	  /* Threads have died because of their own errors....so what should we do ?
	   * for now we kill the window and then the rest of ZMap..... */
	  zmapGUIShowMsg("Cannot show ZMap because server connections have all died") ;

	  killGUI(zmap) ;
	  destroyZMap(zmap) ;

	  call_again = FALSE ;
	}
    }

  return call_again ;
}


static void loadDataConnections(ZMap zmap, char *sequence)
{

  if (zmap->connection_list)
    {
      GList* list_item ;

      list_item = g_list_first(zmap->connection_list) ;

      do
	{
	  ZMapConnection connection ;

 	  connection = list_item->data ;

	  /* ERROR HANDLING..... */
	  zMapConnLoadData(connection, sequence) ;

	} while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}


/* SOME ORDER ISSUES NEED TO BE ATTENDED TO HERE...WHEN SHOULD THE IDLE ROUTINE BE REMOVED ? */

/* destroys the window if this has not happened yet and then destroys the slave threads
 * if there are any.
 */
static void killZMap(ZMap zmap)
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


  if (zmap->window != NULL)
    {
      killGUI(zmap) ;
    }

  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
   * will actually die sometime later...check clean up sequence..... */
  if (zmap->connection_list != NULL)
    {
      killConnections(zmap) ;
    }
		  
  return ;
}


/* Calls the control window callback to remove any reference to the zmap and then destroys
 * the actual zmap itself.
 * 
 * The window pointer must be null'd because this prevents us trying to doubly destroy
 * the window when a thread is killed but only signals its own destruction some time
 * later.
 *  */
static void killGUI(ZMap zmap)
{

  /* is this the correct order...better if the window disappears first probably ?? */

  (*(zmap->destroy_zmap_cb))(zmap, zmap->app_data) ;

  zMapWindowDestroy(zmap->window) ;
  zmap->window = NULL ;

  return ;
}


static void killConnections(ZMap zmap)
{
  GList* list_item ;

  list_item = g_list_first(zmap->connection_list) ;

  do
    {
      ZMapConnection connection ;

      connection = list_item->data ;

      /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
       * will actually die sometime later. */
      zMapConnKill(connection) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}


static void destroyConnection(ZMapConnection *connection)
{
  zMapConnDestroy(*connection) ;

  *connection = NULL ;

  return ;
}



/* In some ways this code should really be in zmapConfig...maybe....
 * We are expecting server_string to be of the form:
 * 
 *         "machine_id port protocol"
 * 
 * i.e. three words only. */
static gboolean getServer(char *server_string, char **machine, char **port, char **protocol)
{
  gboolean result = FALSE ;
  static gchar **tokens = NULL ;			    /* Static because caller needs strings
							       for a short while. */

  if (tokens)
    {
      g_strfreev(tokens) ;
      tokens = NULL ;
    }

  if ((tokens = g_strsplit(server_string, " ", 3)))
    {
      *machine = tokens[0] ;
      *port = tokens[1] ;
      *protocol = tokens[2] ;

      /* NOTE WELL the minimalistic error handling here.... */
      if (machine && port && protocol)
	result = TRUE ;
    }

  return result ;
}
