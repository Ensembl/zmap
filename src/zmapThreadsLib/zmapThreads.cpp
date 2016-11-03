/*  File: zmapThreads.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Code to implement communcation between a control
 *              thread and a slave thread. This code knows nothing
 *              about what it is passing, it just handles the passing
 *              and returning of data.
 *              zMapThreads routines create, issue requests to, and
 *              destroy slave threads.
 *              On creation slave threads are given a routine that
 *              they will call whenever they receive a request. This
 *              routine handles the request and returns the result
 *              to the slave thread code which forwards it to the
 *              master thread.
 *
 * Exported functions: See ZMap/zmapThread.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include <ZMap/zmapUtils.hpp>
#include <zmapThreads_P.hpp>



static ZMapThread createThread() ;
static void destroyThread(ZMapThread thread) ;
static GString *addThreadString(GString *str, ZMapThreadType thread_type, ZMapThread thread) ;


//
//                   Globals
//

/* Turn on/off all debugging messages for threads. */
bool zmap_thread_debug_G = false ;



//
//                   External interface
//



/* Creates a new thread. On creation the thread will wait indefinitely until given a request,
 * on receiving the request the slave thread will forward it to the handler_func
 * supplied by the caller when the thread was created.
 *
 * handler_func   A function that the slave thread can call to handle all requests.
 * returns a thread object, all subsequent requests must be sent to this thread object.
 *  */
ZMapThread zMapThreadCreate(bool new_interface,
                            ZMapSlaveRequestHandlerFunc req_handler_func,
                            ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                            ZMapSlaveDestroyHandlerFunc destroy_handler_func)
{
  ZMapThread thread = NULL ;
  bool status = true ;

  thread = createThread() ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Creating thread...") ;

  if (status)
    {
      if ((status = zmapCondVarCreate(&thread->request)))
        {
          thread->request.state = ZMAPTHREAD_REQUEST_WAIT ;
          thread->request.request = NULL ;
        }
    }

  if (status)
    {
      if ((status = zmapVarCreate(&thread->reply)))
        {
          thread->reply.state = ZMAPTHREAD_REPLY_WAIT ;
          thread->reply.reply = NULL ;
          thread->reply.error_msg = NULL ;
        }
    }


  if (status)
    {
      thread->new_interface = new_interface ;
      thread->req_handler_func = req_handler_func ;
      thread->terminate_handler_func = terminate_handler_func ;
      thread->destroy_handler_func = destroy_handler_func ;

      thread->state = ThreadState::INIT ;
    }
  else
    {
      // Something failed so clean up.

      if (thread->request.state == ZMAPTHREAD_REQUEST_WAIT)
        zmapCondVarDestroy(&thread->request) ;

      
      if (thread->reply.state == ZMAPTHREAD_REPLY_WAIT)
        zmapVarDestroy(&thread->reply) ;

      destroyThread(thread) ;

      thread = NULL ;
    }

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Created thread...") ;

  return thread ;
}

// if false returned then thread could not be started and cannot be used again
// so app should destroy thread struct. 
bool zMapThreadStart(ZMapThread thread, ZMapThreadCreateFunc create_func)
{
  bool result = false ;
  pthread_attr_t thread_attr ;
  int status = 0 ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Starting thread...") ;

  zMapReturnValIfFail((thread->state == ThreadState::INIT), false) ;

  /* Set the new threads attributes so it will run "detached", we do not want anything from them.
   * when they die, we want them to go away and release their resources. */
  if (status == 0)
    {
      if ((status = pthread_attr_init(&thread_attr)) != 0)
        {
          zMapLogCritical("Failed to create thread attibutes: %s", g_strerror(status)) ;
        }
    }

  if (status == 0)
    {
      if ((status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0)
        {
          zMapLogCritical("Failed to set thread detached attibute: %s", g_strerror(status)) ;
        }
    }


  /* Create the new thread. */
  if (status == 0)
    {
      pthread_t thread_id ;

      if ((status = pthread_create(&thread_id, &thread_attr, create_func, (void *)thread)) != 0)
        {
          // either out of memory or more likely exceeded system imposed user limit for thread creation.
	  zMapLogCritical("Failed to create thread: %s", g_strerror(status)) ;
        }
      else
        {
          thread->thread_id = thread_id ;
        }
    }

  if (status == 0)
    {
      thread->state = ThreadState::CONNECTED ;

      result = true ;
    }
  else
    {
      thread->state = ThreadState::FINISHED ;

      result = false ;
    }


  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Started thread...") ;

  return result ;
}



bool zMapThreadRequest(ZMapThread thread, void *request)
{
  bool result = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Sending request...") ;

  if (thread->state == ThreadState::CONNECTED)
    {
      result = zmapCondVarSignal(&thread->request, ZMAPTHREAD_REQUEST_EXECUTE, request) ;
    }

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Sent request...") ;

  return result ;
}



// Get a reply from thread, note can still get reply even if thread is finished.
bool zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state)
{
  bool got_value = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Getting reply...") ;

  if (thread->state == ThreadState::CONNECTED || thread->state == ThreadState::FINISHED)
    got_value = zmapVarGetValue(&(thread->reply), state) ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Got reply...") ;

  return got_value ;
}


bool zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state, void **data, char **err_msg)
{
  bool got_value = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Getting reply with data...") ;

  if (thread->state == ThreadState::CONNECTED || thread->state == ThreadState::FINISHED)
    got_value = zmapVarGetValueWithData(&(thread->reply), state, data, err_msg) ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Got reply with data...") ;

  return got_value ;
}


// should only do with connected state ?
bool zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state)
{
  bool result = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "Setting reply...") ;

  if (thread->state == ThreadState::CONNECTED || thread->state == ThreadState::FINISHED)
    result = zmapVarSetValue(&(thread->reply), state) ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "Set reply...") ;

  return result ;
}


/* Surely we can use the autoconf stuff for this.....sigh.....actually this may just be
 * impossible as pthread_t can be anything from an int to a pointer to a struct... */

/* User must free returned string, note that we need this routine because pthread_t is defined
 * in very different ways on different systems...... */
char *zMapThreadGetThreadID(ZMapThread thread)
{
  char *thread_id = NULL ;
#ifdef LINUX
  const char *format = "%ul" ;
#elif defined DARWIN
  const char *format = "%p" ;
#else
  const  char *format = "%d" ;
#endif

  if (thread->state == ThreadState::CONNECTED)
    thread_id = g_strdup_printf(format, thread->thread_id) ;

  return thread_id ;
}


// If we think thread is connected and the pthread_kill() call tells us the thread exists then it
// exists !!
bool zMapThreadExists(ZMapThread thread)
{
  bool exists = FALSE ;

  if (thread->state == ThreadState::CONNECTED)
    {
      if (pthread_kill(thread->thread_id, 0) == 0)
        exists = TRUE ;
    }

  return exists ;
}


bool zMapIsThreadFinished(ZMapThread thread)
{
  bool finished = false ;

  if (thread->state == ThreadState::FINISHED)
    finished = true ;

  return finished ;
}


// Call from master to stop a thread, need to loop calling zMapIsThreadFinished() to see if
// thread has finished.
bool zMapThreadStop(ZMapThread thread)
{
  bool result = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Stopping thread...") ;

  if (thread->state == ThreadState::CONNECTED)
    {
      // Marks thread so no requests can be made once we are here.
      thread->state = ThreadState::FINISHING ;

      // On receiving this the slave thread should exit but should use zMapThreadFinish() to
      // indicate that it is doing so.
      result = zmapCondVarSignal(&thread->request, ZMAPTHREAD_REQUEST_TERMINATE, NULL) ;
    }

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Stopped thread...") ;

  return result ;
}


// The final solution if all else fails, this function forces the thread to stop and clears up
// without checking if anything worked. This may result in some small amount of dangling memory
// (condvars or whatever) but that's better than having the thread hanging around.
//
// Kill the thread by cancelling it, as this will happen asynchronously we cannot release the threads
// resources in this call.
//
// You still need to call zMapThreadDestroy() after this.
//
void zMapThreadKill(ZMapThread thread)
{
  int status ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Killing thread...") ;

  /* Unconditionally cancel the thread if it still exists. */
  if ((thread->thread_id) && (pthread_kill(thread->thread_id, 0) == 0))
    {
      ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread,
                           "Issuing pthread_cancel on this thread (%s)", zMapThreadGetThreadID(thread)) ;

      if ((status = pthread_cancel(thread->thread_id)) != 0)
        {
          zMapLogCriticalSysErr(status, "pthread_cancel() of thread %s failed", zMapThreadGetThreadID(thread)) ;
        }

      thread->thread_id = 0 ;
    }

  thread->state = ThreadState::FINISHED ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Killed thread...") ;

  return ;
}


// Destroy the thread if the created thread is gone.
// Note this function takes no notice of any request/reply values, it just goes ahead and destroys
// the thread.
bool zMapThreadDestroy(ZMapThread thread)
{
  bool result = false ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Destroying thread...") ;

  if (thread->state == ThreadState::FINISHED)
    {
      bool var_destroy, cond_destroy ;

      ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "Destroying control block/condvar for this thread (%s)",
                       zMapThreadGetThreadID(thread)) ;

      var_destroy = zmapVarDestroy(&thread->reply) ;
      cond_destroy = zmapCondVarDestroy(&(thread->request)) ;

      destroyThread(thread) ;

      if (var_destroy && cond_destroy)
        result = true ;
    }

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::MASTER, NULL, ZMapThreadType::SLAVE, thread, "%s", "Destroyed thread...") ;

  return result ;
}



/* These wierd macros create functions that will return string literals for each enum. */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadRequest2ExactStr, ZMapThreadRequest, ZMAP_THREAD_REQUEST_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadReply2ExactStr, ZMapThreadReply, ZMAP_THREAD_REPLY_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadReturnCode2ExactStr, ZMapThreadReturnCode, ZMAP_THREAD_RETURNCODE_LIST) ;




/*
 *                         Package routines
 */


// Only call from slave thread just before exitting to show we have quit, could be because we were
// asked to quit or because we are quitting because we have finished or there was an error.
void zmapThreadFinish(ZMapThread thread)
{
  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "Finishing thread...") ;

  thread->thread_id = 0 ;

  thread->state = ThreadState::FINISHED ;

  ZMAPTHREAD_DEBUG_MSG(ZMapThreadType::SLAVE, thread, ZMapThreadType::MASTER, NULL, "%s", "Finished thread...") ;

  return ;
}


// Produce a prefix for thread debugging strings giving caller and callee thread details:
//
//    "<Master | Slave <thread_id>> calling <Master | Slave <thread_id>>"
//
char *zmapThreadGetDebugPrefix(ZMapThreadType caller_thread_type, ZMapThread caller_thread,
                               ZMapThreadType callee_thread_type, ZMapThread callee_thread)
{
  char *prefix = NULL ;
  GString *prefix_str ;

  prefix_str = g_string_sized_new(500) ;

  prefix_str = addThreadString(prefix_str, caller_thread_type, caller_thread) ;

  prefix_str = g_string_append(prefix_str, " to ") ;

  prefix_str = addThreadString(prefix_str, callee_thread_type, callee_thread) ;

  prefix = g_string_free(prefix_str, FALSE) ;

  return prefix ;
}



/*
 *                         Internal routines
 */




static ZMapThread createThread()
{
  ZMapThread thread ;

  thread = g_new0(ZMapThreadStruct, 1) ;

  return thread ;
}


/* some care needed in using this....what about the condvars, when can they be freed ?
 * CHECK THIS ALL WORKS.... */
static void destroyThread(ZMapThread thread)
{
  /* Setting this to zero prevents subtle bugs where calling code continues
   * to try to reuse a defunct control block. */

  /* Should crash if this returns NULL, need my macros from acedb code.... */
  memset((void *)thread, 0, sizeof(ZMapThreadStruct)) ;

  g_free(thread) ;

  return ;
}



// If thread_type == ZMapThreadType::MASTER adds "Master" to str other adds "Slave <thread_id>"
//
// Note if thread is master then thread arg is NULL.
//
static GString *addThreadString(GString *str, ZMapThreadType thread_type, ZMapThread thread)
{
  GString *result = str ;
  pthread_t p_thread ;

  if ((thread_type) == ZMapThreadType::MASTER)
    {        
      str = g_string_append(str, "Master ") ;

      p_thread = pthread_self() ;
    }
  else
    {
      str = g_string_append(str, "Slave ") ;

      p_thread = thread->thread_id ;
    }

  if (p_thread)
    g_string_append_printf(str, "%lu", (unsigned long)p_thread) ;
  else
    str = g_string_append(str, "<NULL>") ;
      
  return result ;
}
