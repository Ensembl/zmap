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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 17 18:49 2003 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.1 2003-11-18 10:32:22 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <ZMap_P.h>



static void killGUI(ZMap zmap) ;
static void killZMap(ZMap zmap) ;
static gint idleCB(gpointer cb_data) ;
static void checkConnection(ZMap zmap) ;
static void startConnectionChecking(ZMap zmap) ;
static void stopConnectionChecking(ZMap zmap) ;



/* Trivial at the moment..... */
ZMap zMapCreate(void *app_data, ZMapCallbackFunc destroy_func)
{
  ZMap zmap = NULL ;

  zmap = g_new(ZMapStruct, sizeof(ZMapStruct)) ;

  zmap->app_data = app_data ;
  zmap->destroy_zmap_cb = destroy_func ;

  zmap->state = ZMAP_INIT ;

  return zmap ;
}



/* Create a new zmap with its window and a connection to a database via a thread,
 * at this point the ZMap is blank and waiting to be told to load some data. */
gboolean zMapConnect(ZMap zmap, char *machine, int port, char *sequence)
{
  gboolean result = TRUE ;

  if (result)
    {
      if (!(zmap->connection = zMapConnCreate(machine, port, sequence)))
	result = FALSE ;
    }

  /* We should pass in the address of our "idle" function to the window create routine...or
   * perhaps we can just set up the idle function here.....better ?? */

  if (result)
    {
      /* The destroy_func bit is probably not right...needs to be another window func... */
      if ((zmap->window = zMapWindowCreate(machine, port, sequence,
					   zmap->destroy_zmap_cb, zmap->app_data)))
	{
	  zmap->state = ZMAP_CONNECTED ;

	}
      else
	{
	  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
	   * will actually die sometime later and be properly cleaned up by code in
	   * zMapManagerCheckConnections() */
	  zMapConnKill(zmap->connection) ;

	  result = FALSE ;
	}
    }

  /* Now set up our idle routine to check the connection. */
  if (result)
    {
      startConnectionChecking(zmap) ;
    }


  return result ;
}


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



/* Called to kill a zmap window and get the associated thread killed, this latter will be
 * asynchronous. */
gboolean zMapDestroy(ZMap zmap)
{
  stopConnectionChecking(zmap) ;

  killGUI(zmap) ;

  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
   * will actually die sometime later and be properly cleaned up by code in
   * zMapManagerCheckConnections() */
  zMapConnKill(zmap->connection) ;

  return TRUE ;
}



/* Start and stop our idle function (gets run when the GUI is doing nothing). This routine
 * checks the status of the connection to the server. */
static void startConnectionChecking(ZMap zmap)
{
  zmap->idle_handle = gtk_idle_add(idleCB, (gpointer)zmap) ;

  return ;
}

static void stopConnectionChecking(ZMap zmap)
{
  gtk_idle_remove(zmap->idle_handle) ;

  return ;
}



/* This is really the guts of the code to check what a connection thread is up
 * to. Every time the GUI thread has stopped doing things this routine gets called
 * so then we check our connection for action..... */
static gint idleCB(gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  checkConnection(zmap) ;

  return 1 ;						    /* > 0 tells gtk to keep calling idleCB */
}




/* This function checks the status of the connection and checks for any reply and
 * then acts on it.
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





/*
 *  ------------------- Internal functions -------------------
 */

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


