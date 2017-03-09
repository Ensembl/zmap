/*  File: zmapMaster.cpp
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
 * Description: Functions to handle replies from slave threads.
 *
 * Exported functions: See ZMap/zmapThreadSource.hpp
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <iostream>

#include <glib.h>
#include <gtk/gtk.h>
#include <string>

#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapUtilsDebug.hpp>

#include <zmapSlave_P.hpp>


using namespace std ;



bool zmap_threadsource_debug_G = true ;


namespace ZMapThreadSource
{

  static gint sourceCheckCB(gpointer cb_data) ;



  //
  // Globals
  //

  // milliseconds delay between checks for reply from source thread.
  static const guint32 source_check_interval_G = 100 ;


  //-----------------------------------------------------------------------------
  //
  // Interface calls for old code, do not use in new code.
  //
  ThreadSource::ThreadSource(bool new_interface,
                             ZMapSlaveRequestHandlerFunc req_handler_func,
                             ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                             ZMapSlaveDestroyHandlerFunc destroy_handler_func)
    : failed_{false}, state_{ThreadSourceStateType::INVALID}, thread_{NULL}, poll_id_{0},
    user_reply_func_{NULL}, user_reply_func_data_{NULL}
  {
    // Add checking of args to this function, throw if not provided....


    thread_ = zMapThreadCreate(new_interface,
                               req_handler_func, terminate_handler_func, destroy_handler_func) ;

    state_ = ThreadSourceStateType::INIT ;

    return ;
  }


  // Hack for old code.....which does it's own monitoring of thread value returns (in the huge
  // checkStateConnections() function in zmapView.cpp...which I'm not going to touch.
  //
  bool ThreadSource::ThreadStart()
  {
    bool result = false ;

    result = zMapThreadStart(thread_, zmapNewThread) ;

    return result ;
  }
  //-----------------------------------------------------------------------------





  //
  //                           External Interface
  //


  ThreadSource::ThreadSource(ZMapSlaveRequestHandlerFunc req_handler_func,
                             ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                             ZMapSlaveDestroyHandlerFunc destroy_handler_func)
    : failed_{false}, state_{ThreadSourceStateType::INVALID}, thread_{NULL}, poll_id_{0},
    user_reply_func_{NULL}, user_reply_func_data_{NULL}
  {
    // Add checking of args to this function, throw if not provided....

    thread_ = zMapThreadCreate(TRUE, req_handler_func, terminate_handler_func, destroy_handler_func) ;

    state_ = ThreadSourceStateType::INIT ;

    return ;
  }


  // NEED TO ALLOW A SEPARATE TYPE OF TIMER TO BE USED....PERHAPS THROUGH A GET/SET INTERFACE....

  // Start the GTK timer function that will check for a reply from the source thread.
  //
  // (Note, we don't use a gtk idle function because it gets repeatedly called ALL 
  // the time that zmap is not doing anything which is unnecessary and makes it look
  // like zmap is looping using 100% CPU.)
  //
  bool ThreadSource::ThreadStart(ZMapThreadPollSlaveUserReplyFunc user_reply_func,
                                 void *user_reply_func_data)
  {
    bool result = false ;

    if (state_ == ThreadSourceStateType::INIT)
      {
        user_reply_func_ = user_reply_func ;
        user_reply_func_data_ = user_reply_func_data ;

        // If we can't start the thread we set an error state.
        if (zMapThreadStart(thread_, zmapNewThread))
          {
            // WARNING: gtk_timeout_add is deprecated and should not be used in newly-written
            // code. Use g_timeout_add() instead.
            poll_id_ = gtk_timeout_add(source_check_interval_G, sourceCheckCB, this) ;

            state_ = ThreadSourceStateType::POLLING ;

            result = true ;
          }
        else
          {
            state_ = ThreadSourceStateType::STOPPED ;
          }
      }

    return result ;
  }


  bool ThreadSource::SendRequest(void *request)
  {
    bool result = false ;

    if (state_ == ThreadSourceStateType::POLLING)
      {
        result = zMapThreadRequest(thread_, request) ;
      }

    return result ;
  }


  // Request slave thread to stop.
  //
  bool ThreadSource::ThreadStop()
  {
    bool result = false ;

    if (state_ == ThreadSourceStateType::POLLING)
      {
        StopPolling() ;

        state_ = ThreadSourceStateType::STOPPING ;

        result = true ;
      }

    return result ;
  }


  ZMapThread ThreadSource::GetThread()
  {
    ZMapThread thread = NULL ;

    if (state_ == ThreadSourceStateType::INIT || state_ == ThreadSourceStateType::POLLING)
      {
        thread = thread_ ;
      }

    return thread ;
  }

  bool ThreadSource::HasFailed()
  {
    return failed_ ;
  }

  // Stop the thread and destroy it.
  //
  ThreadSource::~ThreadSource()
  {
    cout << "in theadsource destructor" << endl ;

    // Stop any interaction in our callback in case it gets run again. 
    state_ = ThreadSourceStateType::STOPPED ;

    // I'm a bit worried that our callback may be called again....by gtk, depends how the
    // gtk_remove_timeout() function works.....
    StopPolling() ;

    zMapThreadDestroy(thread_) ;
    thread_ = NULL ;                                          // Just to be sure.

    return ;
  }



  //
  //                     Internal routines
  //

  // Stop polling unless already stopped.
  void ThreadSource::StopPolling()
  {
    // If we are called properly there should always be a poll_id_
    if (poll_id_)
      {
        gtk_timeout_remove(poll_id_) ;

        poll_id_ = 0 ;
      }
   

    return ;
  }




  //        Functions to check and control the connection to a source thread.
  //

  // A gtk_timeout callback function, not part of the ThreadSource class, called every   
  // source_check_interval_G milliseconds to see if the source thread has replied.
  static gint sourceCheckCB(gpointer cb_data)
  {
    gint call_again = 0 ;
    ThreadSource *thread = (ThreadSource *)cb_data ;

    /* Returning a value > 0 tells gtk to call sourceCheckCB again, so if sourceCheck() returns
     * TRUE we ask to be called again. */
    if (ThreadSource::sourceCheck(*thread))
      call_again = 1 ;

    return call_again ;
  }



  /* This function checks the status of the connection and checks for any reply and
   * then acts on it, it gets called from the ZMap idle function.
   * If all threads are ok and zmap has not been killed then routine returns TRUE
   * meaning it wants to be called again, otherwise FALSE in which case gtk automatically
   * removes the gtk_timeout, no need for us to do it.
   *
   * NOTE that you cannot use a condvar here, if the connection thread signals us using a
   * condvar we will probably miss it, that just doesn't work, we have to pole for changes
   * and this is possible because this routine is called from the idle function of the GUI.
   *
   *  */
  bool ThreadSource::sourceCheck(ThreadSource &thread_source)
  {
    gboolean call_again = TRUE ;                                    /* Normally we want to be called continuously. */
    ZMapThreadReply reply = ZMAPTHREAD_REPLY_INVALID ;
    void *data = NULL ;
    char *err_msg = NULL ;

    if (thread_source.state_ == ThreadSourceStateType::STOPPING
        || thread_source.state_ == ThreadSourceStateType::POLLING)
      {
        bool status ;


        if (!(status = zMapThreadGetReplyWithData(thread_source.thread_, &reply, &data, &err_msg)))
          {
            // something bad has happened if we can't get a reply so kill the thread.
            // Note: we would like to throw an exception here but there is no sensible catcher in the
            // app as this function is called asynch well after the original code sending a request
            // has finished.

            ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread_source.thread_,
                                 "cannot access reply from child, error was  \"%s\"", err_msg) ;

            zMapLogCritical("Thread %p will be killed as cannot access reply from child, error was  \"%s\"",
                            thread_source.thread_, err_msg) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            // This call will cancel the thread 
            zMapThreadKill(thread_source.thread_) ;

            // signal that we are finished, 
            thread_source.state_ = ThreadSourceStateType::STOPPED ;
            thread_source.poll_id_ = 0 ;                        // 
                                                                // gtk_timeout removed automatically
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            // Need our caller to be able to find out if we have failed.....
            thread_source.failed_ = true ;
          }

        if (!status || reply != ZMAPTHREAD_REPLY_WAIT)
          {
            if (reply != ZMAPTHREAD_REPLY_QUIT)
              {
                zMapLogWarning("Thread %p failed with \"%s\" so will be killed.",
                               thread_source.thread_, zMapThreadReply2ExactStr(reply)) ;
              }

            // Call the user callback and tell them what happened.
            (thread_source.user_reply_func_)(thread_source.user_reply_func_data_) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            thread_source.poll_id_ = 0 ;                    // 
            // gtk_timeout removed automatically
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            call_again = FALSE ;                        // gtk_timeout removed automatically
          }
      }


    return call_again ;
  }



} // End of ZMapThread namespace

