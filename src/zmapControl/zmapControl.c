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
 * Last edited: Jan 23 11:24 2004 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.2 2004-01-23 13:28:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <ZMap_P.h>


/* ZMap debugging output. */
gboolean zmap_debug_G = TRUE ; 


static gint zmapIdleCB(gpointer cb_data) ;
static void zmapWindowCB(void *cb_data, int reason) ;
static void checkConnection(ZMap zmap) ;
static void startConnectionChecking(ZMap zmap) ;
static void stopConnectionChecking(ZMap zmap) ;
static void killGUI(ZMap zmap) ;
static void killZMap(ZMap zmap) ;
static gboolean getServer(char *server_string, char **machine, char **port, char **protocol) ;



/*
 *  ------------------- External functions -------------------
 *   (includes callbacks)
 */


/* Trivial at the moment..... */
ZMap zMapCreate(void *app_data, ZMapCallbackFunc destroy_cb, ZMapConfig config)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER... */
  static int zmap_num = 0 ;

  zmap = g_new(ZMapStruct, sizeof(ZMapStruct)) ;
  zmap_num++ ;

  zmap->zmap_id = g_strdup_printf("%d", zmap_num) ;

  zmap->state = ZMAP_INIT ;

  zmap->sequence = NULL ;

  zmap->config = config ;

  zmap->connection_list = NULL ;

  zmap->app_data = app_data ;
  zmap->destroy_zmap_cb = destroy_cb ;

  return zmap ;
}



/* Create a new zmap with its window and a connection to a database via a thread,
 * at this point the ZMap is blank and waiting to be told to load some data. */
gboolean zMapConnect(ZMap zmap, char *sequence)
{
  gboolean result = TRUE ;
  char **server_list = NULL ;

  zmap->sequence = sequence ;

  /* We need to retrieve the connect data here from the config stuff.... */
  if (result)
    {
      if (!(zMapConfigGetServers(zmap->config, &server_list)))
	result = FALSE ;
    }


  /* Set up a connection to the named server. */
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
	      && (connection = zMapConnCreate(machine, port, protocol, sequence)))
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

  /* Create the zmap window itself. */
  if (result)
    {
      if (!(zmap->window = zMapWindowCreate(sequence, zmap->zmap_id, zmapWindowCB, zmap)))
	result = FALSE ;
    }

  /* Now set up our idle routine to check the connection. */
  if (result)
    {
      startConnectionChecking(zmap) ;
    }

  /* If everything is ok then we are connected to the server, otherwise signal failure
   * and clean up. */
  if (result)
    {
      zmap->state = ZMAP_CONNECTED ;
    }
  else
    {
      if (zmap->connection_list)
	{

	  /* OK HERE WE NEED TO CLEAN UP PROPERLY.....NEED A FUNCTION I THINK.... */

	  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
	   * will actually die sometime later and be properly cleaned up by code in
	   * zMapManagerCheckConnections() */
	  zMapConnKill(zmap->connection) ;
	}

      if (zmap->window)
	{
	  killGUI(zmap) ;
	}
    }

  return result ;
}


/* Signal that we want data to stick into the ZMap */
gboolean zMapLoad(ZMap zmap, char *sequence)
{
  gboolean result = TRUE ;

  zMapConnLoadData(zmap->connection) ;

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

  /* We need a new windows call to reset the window and blank it. */

  /* We need to destroy the existing thread connection and start a new one...watch out,
   * remember the destroy will be asynchronous..... */

  return TRUE ;
}


/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id ;

  id = zmap->zmap_id ;

  return id ;
}


char *zMapGetSequence(ZMap zmap)
{
  char *sequence ;

  sequence = zmap->sequence ;

  return sequence ;
}

char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static char *zmapStates[] = {"ZMAP_INIT", "ZMAP_CONNECTED", "ZMAP_LOADING",
			       "ZMAP_DISPLAYED", "ZMAP_QUITTING"} ;
  char *state ;

  state = zmapStates[zmap->state] ;

  return state ;
}



/* Called to kill a zmap window and get the associated thread killed, this latter will be
 * asynchronous. */
gboolean zMapDestroy(ZMap zmap)
{
  stopConnectionChecking(zmap) ;

  killGUI(zmap) ;

  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
   * will actually die sometime later...check clean up sequence..... */
  zMapConnKill(zmap->connection) ;

  /* CHECK THAT THIS ALL WORKS...IT SHOULD DO..... */
  g_free(zmap->zmap_id) ;

  g_free(zmap) ;

  return TRUE ;
}



/* This is really the guts of the code to check what a connection thread is up
 * to. Every time the GUI thread has stopped doing things this routine gets called
 * so then we check our connection for action..... */
static gint zmapIdleCB(gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  checkConnection(zmap) ;

  return 1 ;						    /* > 0 tells gtk to keep calling zmapIdleCB */
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
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMAP_DEBUG(("GUI: received %s from thread %x\n", debug,
	      zMapConnGetThreadid(zmap->connection))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}





/*
 *  ------------------- Internal functions -------------------
 */




/* Start and stop the ZMap GTK idle function (gets run when the GUI is doing nothing).
 * */
static void startConnectionChecking(ZMap zmap)
{
  zmap->idle_handle = gtk_idle_add(zmapIdleCB, (gpointer)zmap) ;

  return ;
}

static void stopConnectionChecking(ZMap zmap)
{
  gtk_idle_remove(zmap->idle_handle) ;

  return ;
}




/* This function checks the status of the connection and checks for any reply and
 * then acts on it, it gets called from the ZMap idle function.
 *
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *  */
static void checkConnection(ZMap zmap)
{
  ZMapThreadReply reply ;

  if (zmap->connection)
    {
      gboolean got_value ;
      void *data = NULL ;
      char *err_msg = NULL ;
      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMAP_DEBUG(("GUI: checking connection for thread %x\n",
		  zMapConnGetThreadid(zmap->connection))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      data = NULL ;
      if (!(got_value = zMapConnGetReplyWithData(zmap->connection, &reply, &data, &err_msg)))
	{
	  printf("GUI: thread state locked, cannot access it....\n") ;
	}
      else
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  ZMAP_DEBUG(("GUI: got state for thread %x, state = %s\n",
		      zMapConnGetThreadid(zmap->connection),
		      zmapVarGetStringState(reply))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  if (reply == ZMAP_REPLY_WAIT)
	    {
	      ;					    /* nothing to do. */
	    }
	  else if (reply == ZMAP_REPLY_GOTDATA)
	    {
	      ZMAP_DEBUG(("GUI: got data for thread %x\n",
			  zMapConnGetThreadid(zmap->connection))) ;

	      zMapConnSetReply(zmap->connection, ZMAP_REPLY_WAIT) ;
		  
	      /* Signal the ZMap that there is work to be done. */
	      zMapWindowSignalData(zmap->window, data) ;
	    }
	  else if (reply == ZMAP_REPLY_DIED)
	    {
	      /* This means the thread has failed for some reason and we should clean up. */
	      ZMAP_DEBUG(("GUI: thread %x has died so cleaning up....\n",
			  zMapConnGetThreadid(zmap->connection))) ;

	      if (err_msg && *err_msg)
		zmapGUIShowMsg(err_msg) ;
		  
	      killZMap(zmap) ;
	    }
	  else if (reply == ZMAP_REPLY_CANCELLED)
	    {
	      /* This means the thread was cancelled so we should clean up..... */
	      ZMAP_DEBUG(("GUI: thread %x has been cancelled so cleaning up....\n",
			  zMapConnGetThreadid(zmap->connection))) ;

	      killZMap(zmap) ;
	    }
	}
    }

  return ;
}



/* SOME ORDER ISSUES NEED TO BE ATTENDED TO HERE...WHEN SHOULD THE IDLE ROUTINE BE REMOVED ? */

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



/* destroys the window if this has not happened yet and then destroys the slave thread
 * control block.
 */
static void killZMap(ZMap zmap)
{
  stopConnectionChecking(zmap) ;

  if (zmap->window != NULL)
    {
      killGUI(zmap) ;
    }

  zMapConnDestroy(zmap->connection) ;
		  
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
      *port = (++tokens)[0] ;
      *protocol = (++tokens)[0] ;

      /* NOTE WELL the minimalistic error handling here.... */
      if (machine && port && protocol)
	result = TRUE ;
    }

  return result ;
}
