/*  File: zmapthrslave.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *          Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *         Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 21 17:01 2004 (edgrif)
 * Created: Thu Jul 24 14:37:26 2003 (edgrif)
 * CVS info:   $Id: zmapSlave.test.c,v 1.6 2010-06-14 15:40:14 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






/* With some additional calls in the zmapConn code I could get rid of the need for
 * the private header here...... */

#include <zmapConn_P.h>


typedef struct
{
  ZMapConnection connection ;
  gboolean thread_died ;
} zmapThreadCBstruct, *zmapThreadCB ;



static void delay(void) ;				    /* testing only */

static void cleanUpThread(void *thread_args) ;


/* This is the routine that is called by the pthread_create() function, in effect
 * this is an endless loop processing requests signalled to the thread via a
 * condition variable. If this thread kills itself we will exit this routine
 * normally, if the thread is cancelled however we may exit from anywhere so its
 * important that the clean up thread registered with pthread_cleanup_push()
 * really does clean up properly. */
void *zmapNewThread(void *thread_args)
{
  zmapThreadCB thread_cb ;
  ZMapConnection connection = (ZMapConnection)thread_args ;
  ZMapRequest thread_state = &(connection->request) ;
  int status ;
  timespec_t timeout ;
  ZMapThreadRequest signalled_state ;


  thread_cb = g_new0(zmapThreadCBstruct, 1) ;
  thread_cb->connection = connection ;
  thread_cb->thread_died = FALSE ;
  

  pthread_cleanup_push(cleanUpThread, (void *)thread_cb) ;


  while (1)
    {
      ZMAP_THR_DEBUG(("%x: about to do timed wait\n", connection->thread_id)) ;

      /* this will crap over performance...asking the time all the time !! */
      timeout.tv_sec = 5 ;				    /* n.b. interface seems to absolute time. */
      timeout.tv_nsec = 0 ;
      signalled_state = zmapCondVarWaitTimed(thread_state, ZMAP_REQUEST_WAIT, &timeout, TRUE) ;

      ZMAP_THR_DEBUG(("%x: finished condvar wait, state = %s\n", connection->thread_id,
		      zmapVarGetRequestString(signalled_state))) ;

      if (signalled_state == ZMAP_REQUEST_TIMED_OUT)
	{
	  continue ;
	}
      else if (signalled_state == ZMAP_REQUEST_GETDATA)
	{
	  static int failure = 0 ;
	  char *data = NULL ;


	  /* simulation code only...in the final app we would call the socket here... */
	  ZMAP_THR_DEBUG(("%x: getting data....\n", connection->thread_id)) ;

	  delay() ;
	  pthread_testcancel() ;			    /* allow cancel point.... */
	  delay() ;
	  pthread_testcancel() ;			    /* allow cancel point.... */
	  delay() ;
	  pthread_testcancel() ;			    /* allow cancel point.... */
	  delay() ;
	  pthread_testcancel() ;			    /* allow cancel point.... */

	  ZMAP_THR_DEBUG(("%x: got all data....\n", connection->thread_id)) ;


	  /* This snippet is for testing only, just fakes the thread failing. */
	  failure++ ;
	  if (!(failure % 5))
	    {
	      thread_cb->thread_died = TRUE ;

	      /* NOTE IMPLICIT TERMINATION OF THREAD BY JUMPING OUT OF THIS LOOP
	       * WHICH LEADS TO EXITTING FROM THIS ROUTINE. FOR OTHER ERRORS WE WIL
	       * HAVE TO HAVE A MORE FORMAL MECHANISM.... */
	      break ;
	    }


	  data = g_strdup_printf("some really wild thread data for thread %x", connection->thread_id) ;

	  /* Signal that we got some data. */
	  zmapVarSetValueWithData(&(connection->reply), ZMAP_REPLY_GOTDATA, (void *)data) ;


	}
      else if (signalled_state == ZMAP_REQUEST_EXIT)
	{
	  /* THIS DOESN'T REALLY HAPPEN AT THE MOMENT.....THINK ABOUT THIS....
	   * We actually just do a thread cancel because it gives better control
	   * to the GUI and perhaps that's the best way..... */

	  ZMAP_THR_DEBUG(("%x: gotta exit now....\n", connection->thread_id)) ;
	  break ;
	}
    }


  /* Note that once we reach here the thread will exit, the pthread_cleanup_pop(1) ensures
   * we call our cleanup routine before we exit.
   * Most times we will not get here because we will be pthread_cancel'd and go straight into
   * our clean_up routine. */

  pthread_cleanup_pop(1) ;				    /* 1 => always call clean up routine */


  ZMAP_THR_DEBUG(("%x: thread exitting from main thread routine....\n", connection->thread_id)) ;


  return thread_args ;
}


/* Gets called when:
 *  1) There is an error and the thread has to exit.
 *  2) The thread gets cancelled, can happen in any of the POSIX cancellable
 *     system calls (e.g. read, select etc.) or from anywhere we declare as a cancellation point.
 *
 * This means some care is needed in handling resources to be released, we may need to set flags
 * to remember which resources have been allocated.
 */
static void cleanUpThread(void *thread_args)
{
  zmapThreadCB thread_cb = (zmapThreadCB)thread_args ;
  ZMapConnection connection = thread_cb->connection ;
  ZMapThreadReply reply ;

  ZMAP_THR_DEBUG(("%x: in overall thread clean up routine....\n", connection->thread_id)) ;

  if (thread_cb->thread_died)
    reply = ZMAP_REPLY_DIED ;
  else
    reply = ZMAP_REPLY_CANCELLED ;

  zmapVarSetValue(&(connection->reply), reply) ;

  ZMAP_THR_DEBUG(("%x: Leaving overall thread clean up routine because %s....\n",
		  connection->thread_id,
		  zmapVarGetReplyString(reply))) ;

  return ;
}


/* Testing only.... */
static void delay(void)
{
  enum {ITERATIONS = INT_MAX/32} ;
  int i, j ;

  for (i = 0, j = 0 ; i < ITERATIONS ; i++)
    {
      j++ ;
    }

  return ;
}


