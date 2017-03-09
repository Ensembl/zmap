/*  File: zmapSlave.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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

#include <iostream>

#include <stdio.h>
#include <string.h>
#include <pthread.h>



#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapThreadSlave.hpp>


/* With some additional calls in the zmapConn code I could get rid of the need for
 * the private header here......check this out... */
#include <zmapThreadsLib/zmapThreads_P.hpp>


#include <zmapSlave_P.hpp>


using namespace std ;



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
  ZMapRequest thread_request = &(thread->request) ;
  TIMESPEC timeout ;
  ZMapThreadRequest signalled_state ;
  ZMapThreadReturnCode slave_response =  ZMAPTHREAD_RETURNCODE_OK;
  bool exit_on_fail ;
  int call_clean = 1 ;
  int old_state = 0, old_type = 0, errno ;


  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL,
                       "%s", "slave thread routine starting....") ;


  /* Next two calls added to fix MACOSX pthread issues with cancellation. */
  if ((errno = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state)) != 0)
    zMapLogCriticalSysErr(errno, "%s", "pthread_setcancelstate() failed") ;

  if ((errno = pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_type)) != 0)
    zMapLogCriticalSysErr(errno, "%s", "pthread_setcanceltype() failed") ;



  /* We set up the thread struct and then immediately set up our pthread clean up routine
   * to catch errors, if we are cancelled before this then the clean up routine will not
   * be called. */
  thread_cb = g_new0(zmapThreadCBstruct, 1) ;
  thread_cb->thread = thread ;

  // If we get cancelled we end up directly in the cleanup routine with thread_cancelled == true
  thread_cb->thread_cancelled = true ;


  // Records that the server request failed for serveral reasons.
  thread_cb->server_died = FALSE ;

  thread_cb->initial_error = NULL ;

  thread_cb->slave_data = NULL ;


  // Set up thread clean up routine, will receive thread_cb as its arg.
  // From this point on if we are cancelled we'll end up in cleanUpThread().
  pthread_cleanup_push(cleanUpThread, (void *)thread_cb) ;



  // Set up either new or old slave handlers and exit policy.
  if (thread->new_interface)
    {
      exit_on_fail = true ;
    }
  else
    {
      exit_on_fail = false ;
    }



  // Ok...loop waiting for requests from the master thread.
  //
  while (slave_response != ZMAPTHREAD_RETURNCODE_QUIT)
    {
      void *request ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      // TRY ONE HERE....
      /* pthread_testcancel fix for MACOSX */
      pthread_testcancel() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, "%s", "about to do timed wait.") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      // Set up a timed wait for a command to arrive from the master thread.
      timeout.tv_sec = 5 ;
      timeout.tv_nsec = 0 ;
      request = NULL ;
      signalled_state = zmapCondVarWaitTimed(thread_request, ZMAPTHREAD_REQUEST_WAIT, &timeout, TRUE,
					     &request) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, "finished condvar wait, state = %s", zMapThreadRequest2ExactStr(signalled_state)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* pthread_testcancel fix for MACOSX */
      pthread_testcancel();


      if (signalled_state == ZMAPTHREAD_REQUEST_TIMED_OUT)
        {
          // We just timed out so go back to waiting.

          continue ;
        }
      else if (signalled_state == ZMAPTHREAD_REQUEST_TERMINATE)
        {
          // We've been told to terminate no matter what the state of our data source....

          ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL,
                               "Been told to terminate by master slave, state = %s", zMapThreadRequest2ExactStr(signalled_state)) ;

          // CHECK THIS STATE....
          zmapVarSetValue(&(thread->reply), ZMAPTHREAD_REPLY_QUIT) ;

          // Faked this up to ensure we quit the loop.....
          slave_response = ZMAPTHREAD_RETURNCODE_QUIT ;

          // We've been asked to quit so just quit, don't try to clear up the data source.
          call_clean = 0 ;
        }
      else if (signalled_state == ZMAPTHREAD_REQUEST_EXECUTE)
        {
          char *slave_error = NULL ;

          bool found_error = FALSE ;


          // Call the data source with the new request, this is a synchronous call.
          //

          ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "calling server to service request....") ;
          zMapPrintTimer(NULL, "In thread, calling handler function") ;

          /* Call the registered slave handler function. */
          slave_response = (*(thread->req_handler_func))(&(thread_cb->slave_data), request, &slave_error) ;

          zMapPrintTimer(NULL, "In thread, returned from handler function") ;
          ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "returned from server, response was %s....",
                           zMapThreadReturnCode2ExactStr(slave_response)) ;


          // Handle the response.
          //

          switch (slave_response)
            {
            case ZMAPTHREAD_RETURNCODE_OK:
              {
                ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s: %s", zMapThreadReturnCode2ExactStr(slave_response), "got all data....") ;

                /* Signal that we got some data. */
                zmapVarSetValueWithData(&(thread->reply), ZMAPTHREAD_REPLY_GOTDATA, request) ;
                request = NULL ;			    /* Reset, we don't free this data. */

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_SOURCEEMPTY:
            case ZMAPTHREAD_RETURNCODE_REQFAIL:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "request failed....") ;

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

                if (exit_on_fail)
                  found_error = true ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_TIMEDOUT:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "request failed....") ;

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

                if (exit_on_fail)
                  found_error = true ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_BADREQ:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "bad request....") ;

                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("Bad Request: %s", error_msg) ;

                thread_cb->initial_error = g_strdup(error_msg) ;

                /* Signal that we failed. */
                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_REQERROR, error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;

                if (exit_on_fail)
                  found_error = true ;

                break ;
              }
            case ZMAPTHREAD_RETURNCODE_SERVERDIED:
              {
                char *error_msg = NULL ;

                ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "server died....") ;

                thread_cb->server_died = TRUE ;

                /* Create an informative error message for the log */
                error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                            zMapThreadReturnCode2ExactStr(slave_response), slave_error) ;

                zMapLogWarning("%s", error_msg) ;

                thread_cb->initial_error = g_strdup(error_msg) ;
                
                /* Create a simpler message (without the return code etc) to show to the user */
                g_free(error_msg) ;
                error_msg = g_strdup_printf("%s", slave_error) ;

                // THIS SHOULD BE A GOT_DATA.....
                /* must continue on to getStatus if it's in the step list
                 * zmapServer functions will not run if status is DIED
                 */
                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_DIED, error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;

                if (exit_on_fail)
                  found_error = true ;

                // Server died so no point in calling clean up routine.
                call_clean = 0 ;

                break;
              }
            case ZMAPTHREAD_RETURNCODE_QUIT:
              {
                // THIS ALL SEEMS TO BE SCREWED UP...QUITTING SHOULD IMPLY A NORMAL EXIT BUT
                // SOMEHOW THIS ALL SEEMS TO HAVE BECOME A MESS....Ed

                char *error_msg = NULL ;

                /* this message goes to the otterlace features loaded message
                   and no error gets mangled into a string that says (Server Pipe: - null)
                   there's no obvious way to get the real exit status here
                   due to the structure of the code and data
                   its unfeasably difficult to detect a sucessful server here and we can only report
                   "(no error: ( no error ( no error)))"
                */
                if (slave_error)
                  error_msg = g_strdup_printf("%s %s - %s \"%s\"", ZMAPTHREAD_SLAVEREQUEST,
                                              zMapThreadReturnCode2ExactStr(slave_response), "server terminated with error:", slave_error) ;
                else
                  error_msg = g_strdup_printf("%s %s - %s", ZMAPTHREAD_SLAVEREQUEST,
                                              zMapThreadReturnCode2ExactStr(slave_response), "server terminated cleanly") ;

                zMapLogWarning("%s", error_msg) ;

                zmapVarSetValueWithError(&(thread->reply), ZMAPTHREAD_REPLY_QUIT, error_msg) ;

                g_free(error_msg) ;
                error_msg = NULL ;


                // Clean quit from slave so no need to call clean up routine.
                call_clean = 0 ;

                break;
              }

            default:
              {
                zMapLogCritical("Data server code has returned an unhandled/bad slave response: %d", slave_response) ;

                break ;
              }

            }


          // for the new slave handling we quit the loop if there was a problem.          
          if (found_error)
            break ;
        }



      /* pthread_testcancel fix for MACOSX */
      pthread_testcancel();

    }


  /* Note that once we reach here the thread will exit, pthread_cleanup_pop() will call
   * our cleanup routine if call_clean == 1 before we exit.
   * Note if thread is cancelled we will go straight into our clean_up routine. */

  // if we got here then the exit is normal so set state for clean up routine.
  thread_cb->thread_cancelled = false ;


  /* something about 64 bit pthread needs pthread_cleanup_pop() at the end. */
  /* cleanup_push and pop are basically fancy open and close braces so
   * there must be some code between the "clean_up:" label and this pop or it doesn't compile! */

  // Call the clean up routine if call_clean == 1
  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL,
                       "slave thread about to exit: will %scall clean up routine....",
                       (call_clean ? "" : "not ")) ;



  pthread_cleanup_pop(call_clean) ;


  // Tidy up......
  g_free(thread_cb) ;

  // Mark thread as finished.
  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "Marking thread as finished and exiting....") ;


  // Signal that the thread is finished.
  zmapThreadFinish(thread) ;


  return thread_args ;
}


/* Gets called when:
 *
 *  1) There is an error and the thread has to exit and asks for cleanup via pthread_cleanup_pop(1).
 *
 *  2) The thread gets cancelled, can happen in any of the POSIX cancellable
 *     system calls (e.g. read, select etc.) or from anywhere we declare as a cancellation point.
 *
 * What this means is that if we are cancelled then there is no really safe way to clear up so we
 * don't even try. We would have to keep state and so would all of the code in the various sources
 * (acedb, file etc) so that changes could be undone. It's not workable.
 *
 */
static void cleanUpThread(void *thread_args)
{
  zmapThreadCB thread_cb = (zmapThreadCB)thread_args ;
  ZMapThread thread = thread_cb->thread ;
  ZMapThreadReply reply ;
  gchar *error_msg = NULL ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine: starting....") ;

  if (thread->new_interface && thread_cb->thread_cancelled)
    {
      ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s",
                           "thread clean-up routine: thread has been cancelled so NO cleanup has been done...") ;
    }
  else
    {
      if (thread_cb->server_died)
        {
          char *err_msg ;

          err_msg = g_strdup_printf("%s%s",
                                    "thread clean-up routine: server has died...",
                                    (thread_cb->initial_error
                                     ? thread_cb->initial_error : "")) ;

          ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", err_msg) ;
          zMapLogCritical("%s", err_msg) ;

          g_free(err_msg) ;

          reply = ZMAPTHREAD_REPLY_DIED ;

          if (thread_cb->initial_error)
            error_msg = g_strdup(thread_cb->initial_error) ;
        }
      else
        {
          reply = ZMAPTHREAD_REPLY_QUIT ;

          /* If thread was cancelled we need to ensure it is terminated correctly. */
          if (thread_cb->slave_data)
            {
              ZMapThreadReturnCode slave_response ;

              /* Call the registered slave handler function. */
              if ((slave_response = (*(thread->terminate_handler_func))(&(thread_cb->slave_data), &error_msg))
                  != ZMAPTHREAD_RETURNCODE_OK)
                {
                  char *err_msg ;

                  err_msg = g_strdup_printf("%s%s",
                                            "Unable to close connection to server cleanly...",
                                            error_msg) ;

                  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", err_msg) ;

                  zMapLogCritical("%s", err_msg) ;

                  g_free(err_msg) ;
                }
            }
        }

      /* Now make sure slave is destroyed correctly. */
      if (thread_cb->slave_data)
        {
          ZMapThreadReturnCode slave_response ;

          ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine: calling slave terminate function...") ;

          /* Call the registered slave handler function. */
          if ((slave_response = (*(thread->destroy_handler_func))(&(thread_cb->slave_data)))
              != ZMAPTHREAD_RETURNCODE_OK)
            {
              ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine: Unable to destroy connection") ;

              zMapLogCritical("%s", "thread clean-up routine: Unable to destroy connection") ;
            }
        }




      // Old part needs this otherwise clearing up of threads is not reported properly.
      if (!(thread_cb->thread->new_interface))
        {
          /* And report what happened..... */
          if (!error_msg)
            {
              ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine: no errors") ;

              zmapVarSetValue(&(thread->reply), reply) ;
            }
          else
            {
              ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine: error: %s") ;

              zmapVarSetValueWithError(&(thread->reply), reply, error_msg) ;
            }
        }
    }


  // Some duplicated clean up if we are cancelled and come straight in here.
  if (thread_cb->thread_cancelled)
    {
      // Signal that the thread is finished.
      zmapThreadFinish(thread) ;
    }



  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "thread clean-up routine exiting....") ;

  return ;
}



