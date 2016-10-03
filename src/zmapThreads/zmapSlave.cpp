/*  File: zmapSlave.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: This code is called when a new thread is created by the
 *              zmapThread code, it loops waiting for commands from
 *              a master thread.
 *
 * Exported functions: See zmapConn_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdio.h>
#include <string.h>

/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>

#include <ZMap/zmapUtils.hpp>

/* With some additional calls in the zmapConn code I could get rid of the need for
 * the private header here......check this out... */
#include <zmapThreads_P.hpp>
#include <zmapSlave_P.hpp>


enum {ZMAPTHREAD_SLAVE_REQ_BUFSIZE = 512} ;


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
  zmapThreadCB thread_cb = NULL ;
  ZMapThread thread = (ZMapThread)thread_args ;
  ZMapRequest thread_state = &(thread->request) ;
  TIMESPEC timeout ;
  ZMapThreadRequest signalled_state ;
  ZMapThreadReturnCode slave_response =  ZMAPTHREAD_RETURNCODE_OK;
  int call_clean = 1 ;


  ZMAPTHREAD_DEBUG(thread, "%s", "main thread routine starting....") ;

  /* We set up the thread struct and then immediately set up our pthread clean up routine
   * to catch errors, if we are cancelled before this then the clean up routine will not
   * be called. */
  thread_cb = g_new0(zmapThreadCBstruct, 1) ;
  thread_cb->thread = thread ;
  thread_cb->thread_died = FALSE ;
  thread_cb->initial_error = NULL ;
  thread_cb->slave_data = NULL ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  thread_cb->server_request = ZMAPTHREAD_SERVERREQ_INVALID ;
  thread_cb->server_reply = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  pthread_cleanup_push(cleanUpThread, (void *)thread_cb) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* somehow doing this screws up the whole gui-slave communication bit...not sure how....
   * the slaves just die, almost all the time.... */

  /* Signal that we are ready and waiting... */
  zMapConnSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Next two calls added to fix MACOSX pthread issues */
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL) ;
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL) ;


  while (slave_response != ZMAPTHREAD_RETURNCODE_QUIT)
    {
      void *request ;

      ZMAPTHREAD_DEBUG(thread, "%s", "about to do timed wait.") ;

      /* this will crap over performance...asking the time all the time !! */
      timeout.tv_sec = 5 ;				    /* n.b. interface seems to absolute time. */
      timeout.tv_nsec = 0 ;
      request = NULL ;
      signalled_state = zmapCondVarWaitTimed(thread_state, ZMAPTHREAD_REQUEST_WAIT, &timeout, TRUE,
					     &request) ;

      ZMAPTHREAD_DEBUG(thread, "finished condvar wait, state = %s", zMapThreadRequest2ExactStr(signalled_state)) ;

      /* pthread_testcancel fix for MACOSX */
      pthread_testcancel();

      if (signalled_state == ZMAPTHREAD_REQUEST_TIMED_OUT)
        {
          continue ;
        }
      else if (signalled_state == ZMAPTHREAD_REQUEST_EXECUTE)
        {
          void *reply ;
          char *slave_error = NULL ;

          ZMAPTHREAD_DEBUG(thread, "%s", "calling server to service request....") ;

          zMapPrintTimer(NULL, "In thread, calling handler function") ;

          /* Call the registered slave handler function. */
          slave_response = (*(thread->handler_func))(&(thread_cb->slave_data), request, &reply, &slave_error) ;
          zMapPrintTimer(NULL, "In thread, returned from handler function") ;

          ZMAPTHREAD_DEBUG(thread, "returned from server, response was %s....",
                           zMapThreadReturnCode2ExactStr(slave_response)) ;


          /* The handling below is not now correct, if a call fails we need to kill the thread
           * we can't cope with dangling threads....we will need to set the thread_died flag */
          switch (slave_response)
            {
            case ZMAPTHREAD_RETURNCODE_OK:
              {
                ZMAPTHREAD_DEBUG(thread, "%s: %s", zMapThreadReturnCode2ExactStr(slave_response), "got all data....") ;

                /* Signal that we got some data. */
                zmapVarSetValueWithData(&(thread->reply), ZMAPTHREAD_REPLY_GOTDATA, request) ;
                request = NULL ;			    /* Reset, we don't free this data. */
                break ;
              }
            case ZMAPTHREAD_RETURNCODE_SOURCEEMPTY:
            case ZMAPTHREAD_RETURNCODE_REQFAIL:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG(thread, "%s", "request failed....") ;

                /* Create an informative error message for the log */
                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("%s", error_msg) ;

                /* Create a simpler message (without the return code etc) to show to the user */
                g_free(error_msg) ;
                error_msg = g_strdup_printf("%s", slave_error) ;

                /* Signal that we failed. */
                zmapVarSetValueWithErrorAndData(&(thread->reply), ZMAPTHREAD_REPLY_REQERROR, error_msg, request) ;

                request = NULL ;

                g_free(error_msg) ;
                error_msg = NULL ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_TIMEDOUT:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG(thread, "%s", "request failed....") ;

                /* Create an informative error message for the log */
                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("%s", error_msg) ;

                /* Create a simpler message (without the return code etc) to show to the user */
                g_free(error_msg) ;
                error_msg = g_strdup_printf("%s", slave_error) ;

                /* Signal that we failed. */
                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_REQERROR, error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_BADREQ:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG(thread, "%s", "bad request....") ;

                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("Bad Request: %s", error_msg) ;

                thread_cb->thread_died = TRUE ;
                thread_cb->initial_error = g_strdup(error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;

                goto clean_up ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_SERVERDIED:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG(thread, "%s", "server died....") ;

                /* Create an informative error message for the log */
                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("%s", error_msg) ;

                /* a misnomer, it's the server that the thread talks to */
                if (!thread_cb->thread_died)
                  zMapLogWarning("Server died: %s", error_msg) ;

                thread_cb->thread_died = TRUE ;
                thread_cb->initial_error = g_strdup(error_msg) ;
                
                /* Create a simpler message (without the return code etc) to show to the user */
                g_free(error_msg) ;
                error_msg = g_strdup_printf("%s", slave_error) ;

                /* must continue on to getStatus if it's in the step list
                 * zmapServer functions will not run if status is DIED
                 */
                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_DIED, error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;

                break;
              }
            case ZMAPTHREAD_RETURNCODE_QUIT:
              {
                char *error_msg = NULL ;

                /* this message goes to the otterlace features loaded message
                   and no error gets mangled into a string that says (Server Pipe: - null)
                   there's no obvious way to get the real exit status here
                   due to the structure of the code and data
                   its unfeasably difficult to detect a sucessful server here and we can only report
                   "(no error: ( no error ( no error)))"
                */
                error_msg = g_strdup_printf("%s %s - %s (%s)", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), "server terminated", slave_error) ;

                zMapLogWarning("%s", error_msg) ;

                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_QUIT, error_msg) ;

                call_clean = 0 ;

                g_free(error_msg) ;
                error_msg = NULL ;

                break;
              }

            default:
              {
                zMapLogCritical("Data server code has returned an unhandled/bad slave response: %d", slave_response) ;

                break ;
              }

            }
        }
      /* pthread_testcancel fix for MACOSX */
      pthread_testcancel();

    }



  /* Note that once we reach here the thread will exit, the pthread_cleanup_pop(1) ensures
   * we call our cleanup routine before we exit.
   * Most times we will not get here because we will be pthread_cancel'd and go straight into
   * our clean_up routine. */

 clean_up:

  ZMAPTHREAD_DEBUG(thread, "%s", "main thread routine exitting....") ;

  /* something about 64 bit pthread needs this at the end. */
  /* cleanup_push and pop are basically fancy open and close braces so
   * there must be some code between the "clean_up:" label and this pop or it doesn't compile! */
  pthread_cleanup_pop(call_clean) ;     /* 1 => always call clean up routine */

  return thread_args ;
}


/* Gets called when:
 *  1) There is an error and the thread has to exit.
 *  2) The thread gets cancelled, can happen in any of the POSIX cancellable
 *     system calls (e.g. read, select etc.) or from anywhere we declare as a cancellation point.
 *
 * This means some care is needed in handling resources to be released, we may need to set flags
 * to remember which resources have been allocated.
 * In particular we DO NOT free the thread_cb->thread, this is done by the GUI thread
 * which created it.
 */
static void cleanUpThread(void *thread_args)
{
  zmapThreadCB thread_cb = (zmapThreadCB)thread_args ;
  ZMapThread thread = thread_cb->thread ;
  ZMapThreadReply reply ;
  gchar *error_msg = NULL ;

  ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: starting....") ;

  if (thread_cb->thread_died)
    {
      ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: thread has died...") ;

      reply = ZMAPTHREAD_REPLY_DIED ;

      if (thread_cb->initial_error)
        error_msg = g_strdup(thread_cb->initial_error) ;
    }
  else
    {
      ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: thread has been cancelled...") ;

      reply = ZMAPTHREAD_REPLY_CANCELLED ;

      /* If thread was cancelled we need to ensure it is terminated correctly. */
      if (thread_cb->slave_data)
        {
          ZMapThreadReturnCode slave_response ;

          /* Call the registered slave handler function. */
          if ((slave_response = (*(thread->terminate_func))(&(thread_cb->slave_data), &error_msg)) != ZMAPTHREAD_RETURNCODE_OK)
            {
              ZMAPTHREAD_DEBUG(thread, "%s", "Unable to close connection to server cleanly") ;
            }
        }
    }

  /* Now make sure thread is destroyed correctly. */
  if (thread_cb->slave_data)
    {
      ZMapThreadReturnCode slave_response ;

      ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: calling slave terminate function...") ;

      /* Call the registered slave handler function. */
      if ((slave_response = (*(thread->destroy_func))(&(thread_cb->slave_data)))!= ZMAPTHREAD_RETURNCODE_OK)
        {
          ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: Unable to destroy connection") ;
        }
    }

  g_free(thread_cb) ;

  /* And report what happened..... */
  if (!error_msg)
    {
      ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: no errors") ;

      zmapVarSetValue(&(thread->reply), reply) ;
    }
  else
    {
      ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine: error: %s") ;

      zmapVarSetValueWithError(&(thread->reply), reply, error_msg) ;
    }


  ZMAPTHREAD_DEBUG(thread, "%s", "thread clean-up routine exitting because....") ;

  return ;
}



