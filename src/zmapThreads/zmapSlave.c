/*  File: zmapthrslave.c
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
 * Last edited: Mar 11 15:56 2004 (edgrif)
 * Created: Thu Jul 24 14:37:26 2003 (edgrif)
 * CVS info:   $Id: zmapSlave.c,v 1.3 2004-03-12 15:59:27 edgrif Exp $
 *-------------------------------------------------------------------
 */

/* With some additional calls in the zmapConn code I could get rid of the need for
 * the private header here...... */

#include <zmapConn_P.h>
#include <ZMap/zmapServer.h>
#include <zmapSlave_P.h>



enum {ZMAP_SLAVE_REQ_BUFSIZE = 512} ;



static void cleanUpThread(void *thread_args) ;





/* This is the routine that is called by the pthread_create() function, in effect
 * this is an endless loop processing requests signalled to the thread via a
 * condition variable. If this thread kills itself we will exit this routine
 * normally, if the thread is cancelled however we may exit from any thread cancellable
 * system call currently in progress or any of our cancellation points so its
 * important that the clean up routine registered with pthread_cleanup_push()
 * really does clean up properly. */
void *zmapNewThread(void *thread_args)
{
  zmapThreadCB thread_cb ;
  ZMapConnection connection = (ZMapConnection)thread_args ;
  ZMapRequest thread_state = &(connection->request) ;
  int status ;
  TIMESPEC timeout ;
  ZMapThreadRequest signalled_state ;


  ZMAP_THR_DEBUG(("%x: main thread routine starting....\n", connection->thread_id)) ;


  thread_cb = g_new(zmapThreadCBstruct, sizeof(zmapThreadCBstruct)) ;
  thread_cb->connection = connection ;
  thread_cb->thread_died = FALSE ;
  thread_cb->initial_error = NULL ;
  thread_cb->server = NULL ;
  thread_cb->server_request = g_string_sized_new(ZMAP_SLAVE_REQ_BUFSIZE) ;
  thread_cb->server_reply = NULL ;


  /* Only do the minimum before setting this up as this is the call that makes sure our
   * cleanup routine is called. */
  pthread_cleanup_push(cleanUpThread, (void *)thread_cb) ;

  if (!zMapServerCreateConnection(&(thread_cb->server), connection->machine, connection->port,
				  "any", "any"))
    {
      thread_cb->thread_died = TRUE ;
      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNCREATE,
						 zMapServerLastErrorMsg(thread_cb->server)) ;
      goto clean_up ;
    }

  if (!zMapServerOpenConnection(thread_cb->server))
    {
      thread_cb->thread_died = TRUE ;
      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNOPEN,
						 zMapServerLastErrorMsg(thread_cb->server)) ;
      goto clean_up ;
    }

  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* somehow doing this screws up the whole gui-slave communication bit...not sure how....
   * the slaves just die, almost all the time.... */

  /* Signal that we are ready and waiting... */
  zMapConnSetReply(connection, ZMAP_REPLY_WAIT) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  while (1)
    {
      gchar *sequence ;

      ZMAP_THR_DEBUG(("%x: about to do timed wait\n", connection->thread_id)) ;

      /* this will crap over performance...asking the time all the time !! */
      timeout.tv_sec = 5 ;				    /* n.b. interface seems to absolute time. */
      timeout.tv_nsec = 0 ;
      sequence = NULL ;
      signalled_state = zmapCondVarWaitTimed(thread_state, ZMAP_REQUEST_WAIT, &timeout, TRUE,
					     &sequence) ;

      ZMAP_THR_DEBUG(("%x: finished condvar wait, state = %s\n", connection->thread_id,
		      zMapVarGetRequestString(signalled_state))) ;

      if (signalled_state == ZMAP_REQUEST_TIMED_OUT)
	{
	  continue ;
	}
      else if (signalled_state == ZMAP_REQUEST_GETDATA)
	{
	  static int failure = 0 ;
	  char *server_command ;

	  /* Is it an error to not have a sequence ????? */
	  if (!sequence)
	    sequence = "" ;

	  ZMAP_THR_DEBUG(("%x: getting sequence %s....\n", connection->thread_id, sequence)) ;

	  g_string_sprintf(thread_cb->server_request,
			   "gif seqget %s ; seqfeatures", sequence) ;

	  if (!zMapServerRequest(thread_cb->server, thread_cb->server_request->str,
				 &(thread_cb->server_reply)))
	    {
	      thread_cb->thread_died = TRUE ;

	      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNREQUEST,
							 zMapServerLastErrorMsg(thread_cb->server)) ;

	      /* NOTE IMPLICIT TERMINATION OF THREAD BY JUMPING OUT OF THIS LOOP
	       * WHICH LEADS TO EXITTING FROM THIS ROUTINE. FOR OTHER ERRORS WE WIL
	       * HAVE TO HAVE A MORE FORMAL MECHANISM.... */
	      break ;
	    }

	  ZMAP_THR_DEBUG(("%x: got all data....\n", connection->thread_id)) ;


	  /* Signal that we got some data. */
	  zmapVarSetValueWithData(&(connection->reply), ZMAP_REPLY_GOTDATA,
				  (void *)(thread_cb->server_reply)) ;

	  thread_cb->server_reply = NULL ;		    /* Reset, we don't free this string. */
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else
	{
	  /* unknown state......... */
	  thread_cb->thread_died = TRUE ;
	  thread_cb->initial_error = g_strdup_printf("Thread received unknown state from GUI: %s",
						     zMapVarGetRequestString(signalled_state)) ;
	  goto clean_up ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* Note that once we reach here the thread will exit, the pthread_cleanup_pop(1) ensures
   * we call our cleanup routine before we exit.
   * Most times we will not get here because we will be pthread_cancel'd and go straight into
   * our clean_up routine. */
 clean_up:

  pthread_cleanup_pop(1) ;				    /* 1 => always call clean up routine */


  ZMAP_THR_DEBUG(("%x: main thread routine exitting....\n", connection->thread_id)) ;


  return thread_args ;
}


/* Gets called when:
 *  1) There is an error and the thread has to exit.
 *  2) The thread gets cancelled, can happen in any of the POSIX cancellable
 *     system calls (e.g. read, select etc.) or from anywhere we declare as a cancellation point.
 *
 * This means some care is needed in handling resources to be released, we may need to set flags
 * to remember which resources have been allocated.
 * In particular we DO NOT free the thread_cb->connection, this is done by the GUI thread
 * which created it.
 */
static void cleanUpThread(void *thread_args)
{
  zmapThreadCB thread_cb = (zmapThreadCB)thread_args ;
  ZMapConnection connection = thread_cb->connection ;
  ZMapThreadReply reply ;
  gchar *error_msg = NULL ;

  ZMAP_THR_DEBUG(("%x: thread clean-up routine starting....\n", connection->thread_id)) ;

  if (thread_cb->thread_died)
    {
      reply = ZMAP_REPLY_DIED ;
      if (thread_cb->initial_error)
	error_msg = g_strdup(thread_cb->initial_error) ;
    }
  else
    reply = ZMAP_REPLY_CANCELLED ;

  if (thread_cb->server)
    {
      if (zMapServerCloseConnection(thread_cb->server)
	  && zMapServerFreeConnection(thread_cb->server))
	{
	  thread_cb->server = NULL ;
	}
      else
	ZMAP_THR_DEBUG(("%x: Unable to close connection to server cleanly\n",
			connection->thread_id)) ;

      /* this should be an error message now as well as a debug message..... */
    }


  if (thread_cb->server_request)
    g_string_free(thread_cb->server_request, TRUE) ;
  if (thread_cb->server_reply)
    g_free(thread_cb->server_reply) ;


  g_free(thread_cb) ;


  printf("%x: error msg before condvar set: %s\n", connection->thread_id, error_msg) ;
  fflush(stdout) ;

  if (!error_msg)
    zmapVarSetValue(&(connection->reply), reply) ;
  else
    zmapVarSetValueWithError(&(connection->reply), reply, error_msg) ;

  printf("%x: error msg after condvar set: %s\n", connection->thread_id, error_msg) ;
  fflush(stdout) ;




  ZMAP_THR_DEBUG(("%x: thread clean-up routine exitting because %s....\n",
		  connection->thread_id,
		  zMapVarGetReplyString(reply))) ;

  return ;
}


