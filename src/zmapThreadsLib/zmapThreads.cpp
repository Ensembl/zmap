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
 *              zMapThreads routines create, issue requests to, and destroy slave threads.
 *              On creation slave threads are given a routine that they will call whenever
 *              they receive a request. This routine handles the request and returns the
 *              result to the slave thread code which forwards it to the master thread.
 *
 * Exported functions: See ZMap/zmapThread.h
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



//
//                   Globals
//

/* Turn on/off all debugging messages for threads. */
gboolean zmap_thread_debug_G = FALSE ;


/* For locking/unlocking of fork calls. */
static pthread_mutex_t thread_fork_mutex_G = PTHREAD_MUTEX_INITIALIZER ;




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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  pthread_t thread_id ;
  pthread_attr_t thread_attr ;
  int status = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  thread = createThread() ;

  thread->new_interface = new_interface ;
  thread->req_handler_func = req_handler_func ;
  thread->terminate_handler_func = terminate_handler_func ;
  thread->destroy_handler_func = destroy_handler_func ;

  /* ok to just set state here because we have not started the thread yet.... */
  zmapCondVarCreate(&(thread->request)) ;
  thread->request.state = ZMAPTHREAD_REQUEST_WAIT ;
  thread->request.request = NULL ;

  zmapVarCreate(&(thread->reply)) ;

  thread->reply.state = ZMAPTHREAD_REPLY_WAIT ;
  thread->reply.reply = NULL ;
  thread->reply.error_msg = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Set the new threads attributes so it will run "detached", we do not want anything from them.
   * when they die, we want them to go away and release their resources. */
  if (status == 0
      && (status = pthread_attr_init(&thread_attr)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Create thread attibutes") ;
    }

  if (status == 0
      && (status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Set thread detached attibute") ;
    }


  /* Create the new thread. */
  if (status == 0
      && (status = pthread_create(&thread_id, &thread_attr, create_func, (void *)thread)) != 0)
    {
      //      zMapLogFatalSysErr(status, "%s", "Thread creation") ;
      zMapLogWarning("Failed to create thread: %s",g_strerror(status));	/* likely out of memory */
    }

  if (status == 0)
    {
      thread->thread_id = thread_id ;
    }
  else
    {
      /* Ok to just destroy thread here as the thread was not successfully created so
       * there can be no complications with interactions with condvars in connect struct. */
      destroyThread(thread) ;
      thread = NULL ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return thread ;
}

// if returns false then thread could not be started so app should destroy thread
// struct. 
bool zMapThreadStart(ZMapThread thread, ZMapThreadCreateFunc create_func)
{
  bool result = FALSE ;
  pthread_t thread_id ;
  pthread_attr_t thread_attr ;
  int status = 0 ;

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
      if ((status = pthread_create(&thread_id, &thread_attr, create_func, (void *)thread)) != 0)
        {
	  zMapLogCritical("Failed to create thread: %s", g_strerror(status));	/* likely out of memory */
        }
    }


  if (status == 0)
    {
      thread->thread_id = thread_id ;

      result = TRUE ;
    }

  return result ;
}



void zMapThreadRequest(ZMapThread thread, void *request)
{
  if (thread->thread_id)
    zmapCondVarSignal(&thread->request, ZMAPTHREAD_REQUEST_EXECUTE, request) ;

  return ;
}


gboolean zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state)
{
  gboolean got_value = FALSE ;

  got_value = zmapVarGetValue(&(thread->reply), state) ;

  return got_value ;
}



void zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state)
{
  zmapVarSetValue(&(thread->reply), state) ;

  return ;
}


gboolean zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state,
                                    void **data, char **err_msg)
{
  gboolean got_value = FALSE ;

  got_value = zmapVarGetValueWithData(&(thread->reply), state, data, err_msg) ;

  return got_value ;
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

  if (thread->thread_id)
    thread_id = g_strdup_printf(format, thread->thread_id) ;

  return thread_id ;
}



gboolean zMapThreadExists(ZMapThread thread)
{
  gboolean exists = FALSE ;

  if (thread->thread_id)
    {
      if (pthread_kill(thread->thread_id, 0) == 0)
        exists = TRUE ;
    }

  return exists ;
}


bool zMapThreadStop(ZMapThread thread)
{
  bool result = false ;

  if (thread->thread_id)
    {
      // WHAT WE NEED HERE IS CODE TO SEND A TERMINATE TO THE SLAVE THREAD.

    }

  return result ;
}




/* Kill the thread by cancelling it, as this will happen asynchronously we cannot release the threads
 * resources in this call. */
void zMapThreadKill(ZMapThread thread)
{
  int status ;

  ZMAPTHREAD_DEBUG(thread, "Issuing pthread_cancel on this thread (%s)", zMapThreadGetThreadID(thread)) ;

  /* we could signal an exit here by setting a condvar of EXIT...but that might lead to
   * deadlocks, think about this bit.. */

  /* Signal the thread to cancel it */
  if (thread->thread_id)
    {
      if ((status = pthread_cancel(thread->thread_id)) != 0)
        {
          zMapLogCritical("Cancel of thread %p failed: %s",
                          thread->thread_id, g_strsignal(status)) ;
        }

      thread->thread_id = 0 ;
    }

  return ;
}


/* Kill the thread if necessary and release its resources. */
void zMapThreadDestroy(ZMapThread thread)
{
  ZMAPTHREAD_DEBUG(thread, "Destroying control block/condvar for this thread (%s)", zMapThreadGetThreadID(thread)) ;

  zmapVarDestroy(&thread->reply) ;
  zmapCondVarDestroy(&(thread->request)) ;

  destroyThread(thread) ;

  return ;
}



/* These wierd macros create functions that will return string literals for each enum. */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadRequest2ExactStr, ZMapThreadRequest, ZMAP_THREAD_REQUEST_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadReply2ExactStr, ZMapThreadReply, ZMAP_THREAD_REPLY_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapThreadReturnCode2ExactStr, ZMapThreadReturnCode, ZMAP_THREAD_RETURNCODE_LIST) ;




/* Two functions to lock/unlock round the spawning of a sub-process.
 * 
 * 
 *  */
// 30 threads generate 'can't fork (no memory)' errors from linux
// tried using pthread_atfork() but htis has no effect
//    NB: without the child_at_fork() function this hangs, which implies something odd
// So fork() is thread safe but g_spawn_async_with_pipes() is not
// Must call zMapThreadForkLock() and zMapThreadForkUnlock() around this to serialise the calls
// as pthread_cancel() operates asynchronously then we don't have to worry about unlock on exit
// however, we can just call pthread_mutex_unlock() regardless

// NB libpfetch also call g_spwan_async()
// ZMap has a copy of that code but does not call it
// zMapViewCallBlixem()  also calls g_spawn_...
// but these operate on user command so if they fail can be retried


// prevent concurrent fork() calls from slave threads -> and also the main one?
//          pthread_atfork(prepare_atfork,parent_atfork,child_atfork);
//void prepare_atfork(void).....DON'T KNOW WHY THIS COMMENT IS HERE.....
//
// NB: don't ever nest calls to this function
void zMapThreadForkLock(void)
{
  int locked ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* shouldn't do this really....use the static initialisers..... */

  static gboolean init = FALSE ;


  if (!init)
    {
      init = TRUE ;
      pthread_mutex_init(&thread_fork_mutex_G,NULL) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  locked = pthread_mutex_lock(&thread_fork_mutex_G) ;

  switch (locked)
    {
    case EINVAL:
      zMapLogFatal("%s", "pthread_mutex_lock() called on a mutex which is not correctly initialised.") ;
      break ;						    /* Not zmaplogfatal terminates the process in fact. */
    case EDEADLK:
      zMapLogFatal("%s", "pthread_mutex_lock() called on a mutex which is already locked by this thread.") ;
      break ;
    default:
      break ;
    }

  return ;
}


//void parent_atfork(void).....DON'T KNOW WHY THIS COMMENT IS HERE.....
//
void zMapThreadForkUnlock(void)
{
  int unlocked ;

  unlocked = pthread_mutex_unlock(&thread_fork_mutex_G) ;

  switch (unlocked)
    {
    case EINVAL:
      zMapLogFatal("%s", "pthread_mutex_unlock() called on a mutex which is not correctly initialised.") ;
      break ;						    /* Not zmaplogfatal terminates the process in fact. */
    case EPERM:
      zMapLogCritical("%s", "pthread_mutex_unlock() called on a mutex which this thread does not own.") ;
      break ;
    default:
      break ;
    }

  return ;
}




/*
 *                         Internal routines
 */




  static ZMapThread createThread()
{
  ZMapThread thread ;

  static gboolean init = FALSE ;

  // This should not be in here...it's not part of this package at all...should be in zmaputils somewhere.
  if (!init)
    {
      pthread_mutex_init(&thread_fork_mutex_G,NULL) ;
      init = TRUE ;
    }

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
