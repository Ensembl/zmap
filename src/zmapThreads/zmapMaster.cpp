/*  File: zmapMaster.cpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Functions to handle replies from slave threads.
 *
 * Exported functions: See ZMap/zmapThreads.hpp
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <string.h>
#include <pthread.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <glib.h>
#include <gtk/gtk.h>

#include <ZMap/zmapThreads.hpp>
#include <ZMap/zmapUtilsDebug.hpp>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapDataSource.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
using namespace ZMapDataSource ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Formatting thread debug messages. */
#define THREAD_DEBUG_MSG_PREFIX " Reply from NEW_CODE slave thread %s, "

#define THREAD_DEBUG_MSG(CHILD_THREAD, FORMAT_STR, ...)        \
  G_STMT_START                                                                \
  {                                                                        \
    if (thread_debug_G)                                                 \
      {                                                                 \
        char *thread_str ;                                              \
                                                                        \
        thread_str = zMapThreadGetThreadID((CHILD_THREAD)) ;                \
                                                                        \
        zMapDebugPrint(thread_debug_G, THREAD_DEBUG_MSG_PREFIX FORMAT_STR, thread_str, __VA_ARGS__) ; \
                                                                        \
        zMapLogMessage(THREAD_DEBUG_MSG_PREFIX FORMAT_STR, thread_str, __VA_ARGS__) ; \
                                                                        \
        g_free(thread_str) ;                                                \
      }                                                                 \
  } G_STMT_END

/* Define thread debug messages with extra information, used in checkStateConnections() mostly. */
#define THREAD_DEBUG_MSG_FULL(CHILD_THREAD, VIEW_CON, REQUEST_TYPE, REPLY, FORMAT_STR, ...) \
  G_STMT_START                                                                \
  {                                                                        \
    if (thread_debug_G)                                                 \
      {                                                                 \
        GString *msg_str ;                                              \
                                                                        \
        msg_str = g_string_new("") ;                                    \
                                                                        \
        g_string_append_printf(msg_str, "status = \"%s\", request = \"%s\", reply = \"%s\", msg = \"" FORMAT_STR "\"", \
                               zMapViewThreadStatus2ExactStr((VIEW_CON)->thread_status), \
                               zMapServerReqType2ExactStr((REQUEST_TYPE)), \
                               zMapThreadReply2ExactStr((REPLY)),       \
                               __VA_ARGS__) ;                           \
                                                                        \
        g_string_append_printf(msg_str, ", url = \"%s\"", zMapServerGetUrl((VIEW_CON))) ; \
                                                                        \
        THREAD_DEBUG_MSG((CHILD_THREAD), "%s", msg_str->str) ;          \
                                                                        \
        g_string_free(msg_str, TRUE) ;                                  \
      }                                                                 \
  } G_STMT_END



typedef unsigned int ZMapPollSlaveID ;


typedef struct ZMapThreadPollSlaveCallbackDataStructType
{
  ZMapPollSlaveID poll_id ;
  ZMapThread thread ;
  ZMapThreadPollSlaveUserReplyFunc user_reply_func ;
  void *user_reply_func_data ;

} ZMapThreadPollSlaveCallbackDataStruct ;



static gint sourceCheckCB(gpointer cb_data) ;
static bool sourceCheck(gpointer cb_data) ;



//
// Globals
//

// milliseconds delay between checks for reply from source thread.
static const guint32 source_check_interval_G = 100 ;

static bool thread_debug_G = false ;





//
//                           External Interface
//

// NEED TO ALLOW A SEPARATE TYPE OF TIMER TO BE USED....PERHAPS THROUGH A GET/SET INTERFACE....

// Start the GTK timer function that will check for a reply from the source thread.
//
// (Note, we don't use a gtk idle function because it gets repeatedly called ALL 
// the time that zmap is not doing anything which is unnecessary and makes it look
// like zmap is looping using 100% CPU.)
//
ZMapThreadPollSlaveCallbackData zMapThreadPollSlaveStart(ZMapThread thread,
                                                         ZMapThreadPollSlaveUserReplyFunc user_reply_func,
                                                         void *user_reply_func_data)
{
  ZMapThreadPollSlaveCallbackData cb_data = NULL ;

  cb_data = g_new0(ZMapThreadPollSlaveCallbackDataStruct, 1) ;
  cb_data->thread = thread ;
  cb_data->user_reply_func = user_reply_func ;
  cb_data->user_reply_func_data = user_reply_func_data ;

  // WARNING: gtk_timeout_add is deprecated and should not be used in newly-written code. Use g_timeout_add() instead.
  cb_data->poll_id = gtk_timeout_add(source_check_interval_G, sourceCheckCB, cb_data) ;

  return cb_data ;
}


// Stop the GTK timer function checking for a reply from the source thread.
//
void zMapThreadPollSlaveStop(ZMapThreadPollSlaveCallbackData cb_data)
{
  gtk_timeout_remove(cb_data->poll_id) ;

  g_free(cb_data) ;

  return ;
}








//
//                     Internal routines
//




//
//        Functions to check and control the connection to a source thread.
//


// Called every source_check_interval_G milliseconds to see if the source thread has replied.
//
static gint sourceCheckCB(gpointer cb_data)
{
  gint call_again = 0 ;

  /* Returning a value > 0 tells gtk to call sourceCheckCB again, so if sourceCheck() returns
   * TRUE we ask to be called again. */
  if (sourceCheck(cb_data))
    call_again = 1 ;

  return call_again ;
}


/* This function checks the status of the connection and checks for any reply and
 * then acts on it, it gets called from the ZMap idle function.
 * If all threads are ok and zmap has not been killed then routine returns TRUE
 * meaning it wants to be called again, otherwise FALSE.
 *
 * NOTE that you cannot use a condvar here, if the connection thread signals us using a
 * condvar we will probably miss it, that just doesn't work, we have to pole for changes
 * and this is possible because this routine is called from the idle function of the GUI.
 *
 *  */


static bool sourceCheck(gpointer cb_data)
{
  gboolean call_again = TRUE ;                                    /* Normally we want to be called continuously. */
  ZMapThreadPollSlaveCallbackData poll_data = (ZMapThreadPollSlaveCallbackData)cb_data ;
  ZMapThread thread = poll_data->thread ;
  ZMapThreadReply reply = ZMAPTHREAD_REPLY_DIED ;
  void *data = NULL ;
  char *err_msg = NULL ;


  if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
    {
      /* We assume that something bad has happened to the connection and remove it
       * if we can't read the reply. */

      THREAD_DEBUG_MSG(thread, "cannot access reply from child thread - %s", err_msg) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapLogCritical("Source \"%s\", cannot access reply from server thread,"
                      " error was: %s", zMapServerGetUrl(view_con), (err_msg ? err_msg : "<no error message>")) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    }
  else
    {
      if (reply != ZMAPTHREAD_REPLY_WAIT)
        {
          // If we get a "quit" reply the thread will have exited.
          if (reply != ZMAPTHREAD_REPLY_QUIT)
            {
              zMapThreadKill(thread) ;
            }


          (poll_data->user_reply_func)(poll_data->user_reply_func_data) ;

          // signal that we are finished.
          call_again = FALSE ;
        }
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gboolean state_change = TRUE ;                            /* Has view state changed ?. */
  gboolean reqs_finished = FALSE ;                            /* at least one thread just finished */
  int has_step_list = 0 ;                                    /* any requests still active? */


  /* We should fix this so the function is not called unless there are connections... */
  if (zmap_view->connection_list)
    {
      GList *list_item ;

      list_item = g_list_first(zmap_view->connection_list) ;


      /* GOSH THE STATE HAS BECOME EVER MORE COMPLEX HERE...NEEDS A GOOD CLEAN UP.
       *
       * MALCOLM'S ATTEMPT TO CLEAN IT UP WITH A NEW THREAD STATE HAS JUST MADE IT
       * EVEN MORE COMPLEX....
       *  */


      do
        {
          ZMapNewDataSource view_con ;
          ZMapThread thread ;
          ZMapThreadReply reply = ZMAPTHREAD_REPLY_DIED ;
          void *data = NULL ;
          char *err_msg = NULL ;
          gint err_code = -1 ;
          gboolean thread_has_died = FALSE ;
          gboolean all_steps_finished = FALSE ;
          gboolean this_step_finished = FALSE ;
          ZMapServerReqType request_type = ZMAP_SERVERREQ_INVALID ;
          gboolean is_continue = FALSE ;
          ZMapConnectionData connect_data = NULL ;
          ZMapViewConnectionStepList step_list = NULL ;
          gboolean is_empty = FALSE ;

          view_con = (ZMapNewDataSource)(list_item->data) ;
          thread = view_con->thread ;

          data = NULL ;
          err_msg = NULL ;


          connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_con) ;


          // WHEN WOULD WE NOT HAVE CONNECT DATA ???? SEEMS A BIT ODD.....

          /* NOTE HOW THE FACT THAT WE KNOW NOTHING ABOUT WHERE THIS DATA CAME FROM
           * MEANS THAT WE SHOULD BE PASSING A HEADER WITH THE DATA SO WE CAN SAY WHERE
           * THE INFORMATION CAME FROM AND WHAT SORT OF REQUEST IT WAS.....ACTUALLY WE
           * GET A LOT OF INFO FROM THE CONNECTION ITSELF, E.G. SERVER NAME ETC. */

          // need to copy this info in case of thread death which clears it up
          if (connect_data)
            {
              /* Does this need to be separate....?? probably not.... */

              /* this seems to leak memory but it's very hard to trace the logic of its usage. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              /* This leaks memory, I've removed the copy and tested but the logic is hard to follow
                 * and it's not at all clear when the list goes away....if there's a subsequent
                 * problem then it needs sorting out properly or hacking by doing a g_list_free()
                 * on connect_data->loaded_features->feature_set and reallocating..... 2/12/2014 EG
                 *  */
              connect_data->loaded_features->feature_sets = g_list_copy(connect_data->feature_sets) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
              connect_data->loaded_features->feature_sets = connect_data->feature_sets ;

              connect_data->loaded_features->xwid = zmap_view->xwid ;

              step_list = connect_data->step_list ;
            }

          if (!(zMapThreadGetReplyWithData(thread, &reply, &data, &err_msg)))
            {
              /* We assume that something bad has happened to the connection and remove it
               * if we can't read the reply. */

              THREAD_DEBUG_MSG(thread, "cannot access reply from child thread - %s", err_msg) ;

              /* Warn the user ! */
              if (view_con->show_warning)
                zMapWarning("Source is being removed: Error was: %s\n\nSource: %s",
                            (err_msg ? err_msg : "<no error message>"), zMapServerGetUrl(view_con)) ;

              zMapLogCritical("Source \"%s\", cannot access reply from server thread,"
                              " error was: %s", zMapServerGetUrl(view_con), (err_msg ? err_msg : "<no error message>")) ;

              thread_has_died = TRUE ;
            }
          else
            {
              ZMapServerReqAny req_any = NULL ;


              /* Recover the request from the thread data....is there always data ? check this... */
              if (data)
                {
                  req_any = (ZMapServerReqAny)data ;
                  request_type = req_any->type ;

                  if (req_any->response == ZMAP_SERVERRESPONSE_SOURCEEMPTY)
                    is_empty = TRUE ;
                  else
                    is_empty = FALSE ;
                }


              switch (reply)
                {
                case ZMAPTHREAD_REPLY_WAIT:
                  {
                    state_change = FALSE ;

                    break ;
                  }
                case ZMAPTHREAD_REPLY_GOTDATA:
                case ZMAPTHREAD_REPLY_REQERROR:
                  {
                    ZMapViewConnectionRequest request ;
                    ZMapViewConnectionStep step = NULL ;
                    gboolean kill_connection = FALSE ;


                    /* this is not good and shows this function needs rewriting....
                     * if we get this it means the thread has already failed and been
                     * asked by us to die, it dies but something goes wrong in the dying
                     * so we get an error in clearing up...need to check why...so
                     * we should not go on and process this request in the normal way.
                     *  */
                    if (reply == ZMAPTHREAD_REPLY_REQERROR && view_con->thread_status == THREAD_STATUS_FAILED)
                      {
                        if (!(step_list))
                          THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                "%s", "thread asked to quit but failed in clearing up.") ;
                        else
                          THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                "%s", "LOGIC ERROR....CHECK WHAT'S HAPPENING !") ;

                        /* I'm loathe to do this but all this badly needs a rewrite... */
                        continue ;
                      }

                    /* WHY IS THIS DONE ?? */
                    view_con->curr_request = ZMAPTHREAD_REQUEST_WAIT ;


                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply, "%s", "thread replied") ;


                    /* Recover the stepRequest from the view connection and process the data from
                     * the request. */
                    if (!req_any || !view_con || !(request = zmapViewStepListFindRequest(step_list, req_any->type, view_con)))
                      {
                        zMapLogCritical("Request of type %s for connection %s not found in view %s step list !",
                                        (req_any ? zMapServerReqType2ExactStr(req_any->type) : ""),
                                        (view_con ? zMapServerGetUrl(view_con) : ""),
                                        (zmap_view ? zmap_view->view_name : "")) ;

                        kill_connection = TRUE ;
                      }
                    else
                      {
                        step = zmapViewStepListGetCurrentStep(step_list) ;

                        if (reply == ZMAPTHREAD_REPLY_REQERROR)
                          {
                            /* This means the request failed for some reason. */
                            if (err_msg  && zMapStepOnFailAction(step) != REQUEST_ONFAIL_CONTINUE)
                              {
                                THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                                      "Source has returned error \"%s\"", err_msg) ;
                              }

                            this_step_finished = TRUE ;
                          }
                        else
                          {
                            if (zmap_view->state != ZMAPVIEW_LOADING && zmap_view->state != ZMAPVIEW_UPDATING)
                              {
                                THREAD_DEBUG_MSG(thread, "got data but ZMap state is - %s",
                                                 zMapView2Str(zMapViewGetStatus(zmap_view))) ;
                              }

                            zmapViewStepListStepProcessRequest(step_list, (void *)view_con, request) ;

                            if (zMapConnectionIsFinished(request))
                              {
                                this_step_finished = TRUE ;
                                request_type = req_any->type ;
                                view_con->thread_status = THREAD_STATUS_OK ;

                                if (req_any->type == ZMAP_SERVERREQ_FEATURES)
                                  zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                                         connect_data->feature_sets) ;

                              }
                            else
                              {
                                reply = ZMAPTHREAD_REPLY_REQERROR;    // ie there was an error


                                /* TRY THIS HERE.... */
                                if (err_msg)
                                  {
                                    g_free(err_msg) ;
                                    err_msg = NULL ;
                                  }
                                
                                if (connect_data && connect_data->error)
                                  {
                                    err_msg = g_strdup(connect_data->error->message) ;
                                    err_code = connect_data->error->code ;
                                  }

                                this_step_finished = TRUE ;
                              }
                           }
                      }

                    if (reply == ZMAPTHREAD_REPLY_REQERROR)
                      {
                        if (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_THREAD
                            || zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_STEPLIST)
                          {
                            if (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CANCEL_THREAD)
                              kill_connection = TRUE ;

                            /* Remove request from all steps.... */
                            zmapViewStepListDestroy(step_list) ;
                            connect_data->step_list = step_list = NULL ;

                            zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                                   connect_data->feature_sets) ;
                          }
                        else
                          {
                            zMapStepSetState(step, STEPLIST_FINISHED) ;
                          }



                        view_con->thread_status = THREAD_STATUS_FAILED;        /* so that we report an error */
                      }

                    if (kill_connection)
                      {
                        if (is_empty)
                          zmap_view->sources_empty
                            = zMap_g_list_insert_list_after(zmap_view->sources_empty,
                                                            connect_data->loaded_features->feature_sets,
                                                            g_list_length(zmap_view->sources_empty),
                                                            TRUE) ;
                        else
                          zmap_view->sources_failed
                            = zMap_g_list_insert_list_after(zmap_view->sources_failed,
                                                            connect_data->loaded_features->feature_sets,
                                                            g_list_length(zmap_view->sources_failed),
                                                            TRUE) ;



                        /* Do not reset reply from slave, we need to wait for slave to reply
                         * to the cancel. */
                        /* Warn the user ! */
                        if (view_con->show_warning)
                          {
                            /* Don't bother the user if it's just that there were no features in
                             * the source */
                            if (err_code != ZMAPVIEW_ERROR_CONTEXT_EMPTY)
                              zMapWarning("Error loading source.\n\n %s", (err_msg ? err_msg : "<no error message>")) ;
                            
                            zMapLogWarning("Source is being cancelled: Error was: '%s'. Source: %s",
                                           (err_msg ? err_msg : "<no error message>"), zMapServerGetUrl(view_con)) ;
                          }

                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "Thread being cancelled because of error \"%s\"",
                                              err_msg) ;

                        /* Signal thread to die. */
                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "%s", "signalling child thread to die....") ;
                        zMapThreadKill(thread) ;
                      }
                    else
                      {
                        /* Reset the reply from the slave. */
                        zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
                      }

                    break ;
                  }
                case ZMAPTHREAD_REPLY_DIED:
                  {
                    ZMapViewConnectionStep step ;

                    step = zmapViewStepListGetCurrentStep(step_list) ;

                    /* THIS LOGIC CAN'T BE RIGHT....IF WE COME IN HERE IT'S BECAUSE THE THREAD
                     * HAS ACTUALLY DIED AND NOT JUST QUIT....CHECK STATUS SET IN SLAVES... */
                    if (zMapStepOnFailAction(step) != REQUEST_ONFAIL_CONTINUE)
                      {
                        /* Thread has failed for some reason and we should clean up. */
                        if (err_msg && view_con->show_warning)
                          zMapWarning("%s", err_msg) ;

                        thread_has_died = TRUE ;

                        THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                              "cleaning up because child thread has died with: \"%s\"", err_msg) ;
                      }
                    else
                      {
                        /* mark the current step as finished or else we won't move on
                         * this was buried in zmapViewStepListStepProcessRequest()
                         */
                        zMapStepSetState(step, STEPLIST_FINISHED) ;

                        /* Reset the reply from the slave. */
                        zMapThreadSetReply(thread, ZMAPTHREAD_REPLY_WAIT) ;
                      }

                    break ;
                  }
                case ZMAPTHREAD_REPLY_CANCELLED:
                  {
                    /* This happens when we have signalled the threads to die and they are
                     * replying to say that they have now died. */
                    thread_has_died = TRUE ;

                    /* This means the thread was cancelled so we should clean up..... */
                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                          "%s", "child thread has been cancelled so cleaning up....") ;

                    break ;
                  }
                case ZMAPTHREAD_REPLY_QUIT:
                  {
                    thread_has_died = TRUE;

                    THREAD_DEBUG_MSG_FULL(thread, view_con, request_type, reply,
                                          "%s", "child thread has quit so cleaning up....") ;

                    break;
                  }

                default:
                  {
                    zMapLogFatalLogicErr("switch(), unknown value: %d", reply) ;

                    break ;
                  }
                }

            }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          /* There is a problem with the zMapThreadExists() call on the Mac,
           * it says the thread has died when it hasn't. */

          if (!thread_has_died && !zMapThreadExists(thread))
            {
              thread_has_died = TRUE;
              // message to differ from REPLY_DIED above
              // it really is sudden death, thread is just not there
              THREAD_DEBUG_MSG(thread, "%s", "child thread has died suddenly so cleaning up....") ;
            }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



          /* CHECK HOW STEPS_FINISHED IS SET...SOMETHING IS WRONG.....GETTING CALLED EVERYTIME A
             SOURCE
             * FINISHES INSTEAD WHEN ALL SOURCES ARE FINISHED.... */



          /* If the thread has died then remove it's connection. */
          // do this before counting up the number of step lists
          if (thread_has_died)
            {
              ZMapViewConnectionStep step;

              /* TRY THIS HERE.... */
              /* Add any new sources that have failed. */
              if (connect_data->loaded_features->feature_sets && reply == ZMAPTHREAD_REPLY_DIED)
                {
                  if (is_empty)
                    zmap_view->sources_empty
                      = zMap_g_list_insert_list_after(zmap_view->sources_empty,
                                                      connect_data->loaded_features->feature_sets,
                                                      g_list_length(zmap_view->sources_empty),
                                                      TRUE) ;
                  else
                    zmap_view->sources_failed
                      = zMap_g_list_insert_list_after(zmap_view->sources_failed,
                                                      connect_data->loaded_features->feature_sets,
                                                      g_list_length(zmap_view->sources_failed),
                                                      TRUE) ;
                }


              is_continue = FALSE ;


              if (step_list && (step = zmapViewStepListGetCurrentStep(step_list)))
                {
                  is_continue = (zMapStepOnFailAction(step) == REQUEST_ONFAIL_CONTINUE) ;
                }

              /* We are going to remove an item from the list so better move on from
               * this item. */
              zmap_view->connection_list = g_list_remove(zmap_view->connection_list, view_con) ;
              list_item = zmap_view->connection_list ;


              /* GOSH....I DON'T UNDERSTAND THIS..... */
              if (step_list)
                reqs_finished = TRUE;


              if (reply == ZMAPTHREAD_REPLY_QUIT && view_con->thread_status != THREAD_STATUS_FAILED)
                {
                  if (zMapStepGetRequest(step) == ZMAP_SERVERREQ_TERMINATE)  /* normal OK status in response */
                    {
                      view_con->thread_status = THREAD_STATUS_OK ;
                    }
                }
              else
                {
                  view_con->thread_status = THREAD_STATUS_FAILED ;
                }

               all_steps_finished = TRUE ;                /* destroy connection kills the step list */
            }

          /* Check for more connection steps and dispatch them or clear up if finished. */
          if ((step_list))
            {
              /* If there were errors then all connections may have been removed from
               * step list or if we have finished then destroy step_list. */
              if (zmapViewStepListIsNext(step_list))
                {

                  zmapViewStepListIter(step_list, view_con->thread, view_con) ;
                  has_step_list++;

                  all_steps_finished = FALSE ;
                }
              else
                {
                  zmapViewStepListDestroy(step_list) ;
                  connect_data->step_list = step_list = NULL ;

                  reqs_finished = TRUE;

                  if (view_con->thread_status != THREAD_STATUS_FAILED)
                    view_con->thread_status = THREAD_STATUS_OK ;

                  all_steps_finished = TRUE ;
                }
            }


          /* ...shouldn't be doing this if we are dying....I think ? we should
           * have disconnected this handler....that's why we need the stopConnectionState call
           * add it in.... */

          if (this_step_finished || all_steps_finished)
            {
              if (connect_data)      // ie was valid at the start of the loop
                {
                  if (connect_data->exit_code)
                    view_con->thread_status = THREAD_STATUS_FAILED ;

                  if (reply == ZMAPTHREAD_REPLY_DIED)
                    connect_data->exit_code = 1 ;

                  if (view_con->thread_status == THREAD_STATUS_FAILED)
                    {
                      char *request_type_str = (char *)zMapServerReqType2ExactStr(request_type) ;

                      if (!err_msg)
                        {
                          /* NOTE on TERMINATE OK/REPLY_QUIT we get thread_has_died and NULL the error message */
                          /* but if we set thread_status for FAILED on successful exit then we get this, so let's not do that: */
                          err_msg = g_strdup("Thread failed but there is no error message to say why !") ;

                          zMapLogWarning("%s", err_msg) ;
                        }

                      if (view_con->show_warning && is_continue)
                        {
                          /* we get here at the end of a step list, prev errors not reported till now */
                          zMapWarning("Data request failed: %s\n%s%s", err_msg,
                                      ((connect_data->stderr_out && *connect_data->stderr_out)
                                       ? "Server reports:\n" : ""),
                                      connect_data->stderr_out) ;
                        }

                      zMapLogWarning("Thread %p failed, request = %s, empty sources now %d, failed sources now %d",
                                     thread,
                                     request_type_str,
                                     g_list_length(zmap_view->sources_empty),
                                     g_list_length(zmap_view->sources_failed)) ;
                    }

                  /* Record if features were loaded or if there was an error, if the latter
                   * then report that to our remote peer if there is one. */
                  if (request_type == ZMAP_SERVERREQ_FEATURES || reply == ZMAPTHREAD_REPLY_REQERROR)
                    {
                      connect_data->loaded_features->status = (view_con->thread_status == THREAD_STATUS_OK
                                                               ? TRUE : FALSE) ;

                      if (connect_data->loaded_features->err_msg)
                        g_free(connect_data->loaded_features->err_msg) ;

                      connect_data->loaded_features->err_msg = g_strdup(err_msg) ;
                      connect_data->loaded_features->start = connect_data->start;
                      connect_data->loaded_features->end = connect_data->end;
                      connect_data->loaded_features->exit_code = connect_data->exit_code ;
                      connect_data->loaded_features->stderr_out = connect_data->stderr_out ;

                      if (reply == ZMAPTHREAD_REPLY_REQERROR && zmap_view->remote_control)
                        {
                          /* Note that load_features is cleaned up by sendViewLoaded() */
                          LoadFeaturesData loaded_features ;
                          if (is_empty)
                            connect_data->loaded_features->status = TRUE ;

                          loaded_features = zmapViewCopyLoadFeatures(connect_data->loaded_features) ;

                          zMapLogWarning("View loaded from %s !!", "checkStateConnections()") ;

                          sendViewLoaded(zmap_view, loaded_features) ;
                        }
                    }


                }
            }

          if (err_msg)
            {
              g_free(err_msg) ;
              err_msg = NULL ;
            }

          if (thread_has_died)
            {

              // Need to get rid of other stuff too......
              /* There is more to do here but proceed carefully, sometimes parts of the code are still
               * referring to this because of asynchronous returns from zmapWindow and other code. */
              ZMapConnectionData connect_data ;

              if ((connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_con)))
                {
                  if (connect_data->loaded_features)
                    {
                      zmapViewDestroyLoadFeatures(connect_data->loaded_features) ;

                      connect_data->loaded_features = NULL ;
                    }

                  if (connect_data->error)
                    {
                      g_error_free(connect_data->error) ;
                      connect_data->error = NULL ;
                    }

                  if (connect_data->step_list)
                    {
                      zmapViewStepListDestroy(connect_data->step_list) ;
                      connect_data->step_list = NULL ;
                    }

                  g_free(connect_data) ;
                  view_con->request_data = NULL ;


                  /* Now there is no current view_con.....this is the WRONG PLACE FOR THIS.....SIGH.... */
                  if (zmap_view->sequence_server == view_con)
                    zmap_view->sequence_server = NULL ;

                  zMapServerDestroyViewConnection(view_con) ;
                  view_con = NULL ;
                }
            }

          if (thread_has_died || all_steps_finished)
            {
              zmapViewBusy(zmap_view, FALSE) ;
            }

        } while (list_item && (list_item = g_list_next(list_item))) ;
    }



  /* ok...there's some problem here, the loaded counter gets decremented in
   * processDataRequests() and it can reach zero but we don't enter this section
   * of code so we don't record the state as "loaded"....agh.....
   *
   * One problem might be that has_step_list is only ever incremented ??
   *
   * Looks like reqs_finished == TRUE but there is still a step_list.
   *
   *  */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Use this to trap this problem..... */
  if ((has_step_list || !reqs_finished) && !(zmap_view->sources_loading))
    printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Once a view is loaded there won't be any more column changes until the user elects
   * to load a new column. */
  if (zmap_view->state != ZMAPVIEW_DYING)
    {
      if (zmap_view->state != ZMAPVIEW_LOADED)
        {
          if (!has_step_list && reqs_finished)
            {
              /*
               * rather than count up the number loaded we say 'LOADED' if there's no LOADING active
               * This accounts for failures as well as completed loads
               */
              zmap_view->state = ZMAPVIEW_LOADED ;
              g_list_free(zmap_view->sources_loading) ;
              zmap_view->sources_loading = NULL ;

              state_change = TRUE ;
            }
          else if (!(zmap_view->sources_loading))
            {
              /* We shouldn't need to do this if reqs_finished and has_step_list were consistent. */

              zmap_view->state = ZMAPVIEW_LOADED ;
              state_change = TRUE ;
            }
        }
    }



  /* If we have connections left then inform the layer about us that our state has changed. */
  if ((zmap_view->connection_list) && state_change)
    {
      (*(view_cbs_G->state_change))(zmap_view, zmap_view->app_data, NULL) ;
    }


  /* At this point if we have connections then we carry on looping looking for
   * replies from the views. If there are no threads left then we need to examine
   * our state and take action depending on whether we are dying or threads
   * have died or whatever.... */
  if (!zmap_view->connection_list)
    {
      /* Decide if we need to be called again or if everythings dead. */
      call_again = checkContinue(zmap_view) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return call_again ;
}






