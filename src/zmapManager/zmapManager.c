/*  File: zmapManager.c
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov  7 17:43 2003 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapManager.c,v 1.1 2003-11-10 17:04:27 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapManager_P.h>

static void killGUI(ZMapManager zmaps, ZMapWinConn zmap) ;
static void killZMap(ZMapManager zmaps, ZMapWinConn zmap) ;
static void zmapWindowCB(void *cb_data, int reason) ;



ZMapManager zMapManagerInit(zmapAppCallbackFunc delete_func,
			    void *app_data)
{
  ZMapManager manager ;

  manager = g_new(ZMapManagerStruct, sizeof(ZMapManagerStruct)) ;

  manager->zmap_list = NULL ;

  manager->delete_zmap_guifunc = delete_func ;
  manager->app_data = app_data ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins. */
ZMapWinConn zMapManagerAdd(ZMapManager zmaps, char *machine, int port, char *sequence)
{
  ZMapWinConn zmapconn ;
  ZMapManagerCB zmap_cb ;

  zmapconn = g_new(ZMapWinConnStruct, sizeof(ZMapWinConnStruct)) ;

  zmapconn->connection = zMapConnCreate(machine, port, sequence) ;

  zmap_cb = g_new(ZMapManagerCBStruct, sizeof(ZMapManagerCBStruct)) ;
  zmap_cb->zmap_list = zmaps ;
  zmap_cb->zmap = zmapconn ;

  zmapconn->window = zMapWindowCreate(machine, port, sequence, zmapWindowCB, (void *)zmap_cb) ;

  zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmapconn) ;

  return zmapconn ;
}


void zMapManagerLoadData(ZMapWinConn zmapconn)
{
  zMapConnLoadData(zmapconn->connection) ;

  return ;
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
void zMapManagerReset(ZMapWinConn zmap)
{

  /* We need a new windows call to reset the window and blank it. */

  /* We need to destroy the existing thread connection and start a new one...watch out,
   * remember the destroy will be asynchronous..... */

  return ;
}



/* Called to kill a zmap window and get the associated thread killed, this latter will be
 * asynchronous. */
void zMapManagerKill(ZMapManager zmaps, ZMapWinConn zmap)
{
  killGUI(zmaps, zmap) ;

  /* NOTE, we do a _kill_ here, not a destroy. This just signals the thread to die, it
   * will actually die sometime later and be properly cleaned up by code in
   * zMapManagerCheckConnections() */
  zMapConnKill(zmap->connection) ;

  return ;
}


/* This function passes through the list of connections once and checks for any replies
 * from the any of the connections and acts on those replies.
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *  */
void zMapManagerCheckConnections(ZMapManager zmaps)
{
  ZMapThreadReply reply ;

  if (zmaps->zmap_list)
    {
      GList*  next ;

      next = g_list_first(zmaps->zmap_list) ;
      do
	{
	  ZMapWinConn zmap ;
	  gboolean got_value ;
	  void *data = NULL ;
	  char *err_msg = NULL ;

	  zmap = (ZMapWinConn)next->data ;


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
		  
		  next = g_list_next(next) ;		    /* Move next pointer _before_ we
							       remove the current zmap otherwise
							       what happens if this is the last
							       one...... */

		  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

		  killZMap(zmaps, zmap) ;
		}
	      else if (reply == ZMAP_REPLY_CANCELLED)
		{
		  /* This means the thread was cancelled so we should clean up..... */
		  ZMAP_DEBUG(("GUI: thread %x has been cancelled so cleaning up....\n",
			      zMapConnGetThreadid(zmap->connection))) ;

		  next = g_list_next(next) ;		    /* Move next pointer _before_ we
							       remove the current zmap otherwise
							       what happens if this is the last
							       one...... */

		  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

		  killZMap(zmaps, zmap) ;
		}
	    }

	} while ((next = g_list_next(next))) ;
    }


  return ;
}





/*
 *  ------------------- Internal functions -------------------
 */


/* Calls the control window callback to remove any reference to the zmap and then destroys
 * the actual zmap itself.
 * 
 * The window pointer must be null'd because this prevents us trying to doubly destroy
 * the window when a thread is killed but only signals its own destruction some time
 * later.
 *  */
static void killGUI(ZMapManager zmaps, ZMapWinConn zmap)
{
  (*(zmaps->delete_zmap_guifunc))(zmaps->app_data, zmap) ;

  zMapWindowDestroy(zmap->window) ;
  zmap->window = NULL ;

  return ;
}



/* destroys the window if this has not happened yet and then destroys the slave thread
 * control block.
 */
static void killZMap(ZMapManager zmaps, ZMapWinConn zmap)
{
  if (zmap->window != NULL)
    {
      killGUI(zmaps, zmap) ;
    }

  zMapConnDestroy(zmap->connection) ;
		  
  return ;
}


/* Gets called when zmap Window needs Manager to do something. */
static void zmapWindowCB(void *cb_data, int reason)
{
  ZMapManagerCB zmap_cb = (ZMapManagerCB)cb_data ;
  ZmapWindowCmd window_cmd = (ZmapWindowCmd)reason ;
  char *debug ;


  if (window_cmd == ZMAP_WINDOW_QUIT)
    {
      debug = "ZMAP_WINDOW_QUIT" ;

      zMapManagerKill(zmap_cb->zmap_list, zmap_cb->zmap) ;

      g_free(zmap_cb) ;					    /* Only free on destroy. */
    }
  else if (window_cmd == ZMAP_WINDOW_LOAD)
    {
      debug = "ZMAP_WINDOW_LOAD" ;

      zMapManagerLoadData(zmap_cb->zmap) ;
    }
  else if (window_cmd == ZMAP_WINDOW_STOP)
    {
      debug = "ZMAP_WINDOW_STOP" ;
    }


  ZMAP_DEBUG(("GUI: received %s from thread %x\n", debug,
	      zMapConnGetThreadid(zmap_cb->zmap->connection))) ;

  return ;
}


