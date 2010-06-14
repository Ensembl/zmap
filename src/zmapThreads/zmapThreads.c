/*  File: zmapThreads.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Code to implement communcation between a control
 *              thread and a slave thread. This code knows nothing
 *              about what it is passing, it just handles the passing
 *              and returning of data.
 *
 * Exported functions: See ZMap/zmapThread.h
 * HISTORY:
 * Last edited: Mar 20 12:09 2009 (edgrif)
 * Created: Thu Jan 27 11:25:37 2005 (edgrif)
 * CVS info:   $Id: zmapThreads.c,v 1.12 2010-06-14 10:39:05 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ZMap/zmapUtils.h>
#include <zmapThreads_P.h>


/* Turn on/off all debugging messages for threads. */
gboolean zmap_thread_debug_G = FALSE ;


static ZMapThread createThread(ZMapThreadRequestHandlerFunc handler_func,
			       ZMapThreadTerminateHandler terminate_func, ZMapThreadDestroyHandler destroy_func) ;
static void destroyThread(ZMapThread thread) ;


/*! @defgroup zmapthreads   zMapThreads: creating, controlling and destroying slave threads
 * @{
 *
 * \brief  Slave Threads
 *
 * zMapThreads routines create, issue requests to, and destroy slave threads.
 * On creation slave threads are given a routine that they will call whenever
 * they receive a request. This routine handles the request and returns the
 * result to the slave thread code which forwards it to the master thread.
 *
 *
 *  */



/*!
 * Creates a new thread. On creation the thread will wait indefinitely until given a request,
 * on receiving the request the slave thread will forward it to the handler_func
 * supplied by the caller when the thread was created.
 *
 * @param handler_func   A function that the slave thread can call to handle all requests.
 * @return  A thread object, all subsequent requests must be sent to this thread object.
 *  */
ZMapThread zMapThreadCreate(ZMapThreadRequestHandlerFunc handler_func,
			    ZMapThreadTerminateHandler terminate_func, ZMapThreadDestroyHandler destroy_func)
{
  ZMapThread thread ;
  pthread_t thread_id ;
  pthread_attr_t thread_attr ;
  int status = 0 ;

  zMapAssert(handler_func) ;

  thread = createThread(handler_func, terminate_func, destroy_func) ;

  /* ok to just set state here because we have not started the thread yet.... */
  zmapCondVarCreate(&(thread->request)) ;
  thread->request.state = ZMAPTHREAD_REQUEST_WAIT ;
  thread->request.request = NULL ;

  zmapVarCreate(&(thread->reply)) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  thread->reply.state = ZMAPTHREAD_REPLY_INIT ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  thread->reply.state = ZMAPTHREAD_REPLY_WAIT ;
  thread->reply.reply = NULL ;
  thread->reply.error_msg = NULL ;

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
      && (status = pthread_create(&thread_id, &thread_attr, zmapNewThread, (void *)thread)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Thread creation") ;
    }

  if (status == 0)
    thread->thread_id = thread_id ;
  else
    {
      /* Ok to just destroy thread here as the thread was not successfully created so
       * there can be no complications with interactions with condvars in connect struct. */
      destroyThread(thread) ;
      thread = NULL ;
    }

  return thread ;
}



void zMapThreadRequest(ZMapThread thread, void *request)
{
  zmapCondVarSignal(&thread->request, ZMAPTHREAD_REQUEST_EXECUTE, request) ;

  return ;
}


gboolean zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state)
{
  gboolean got_value ;

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
  gboolean got_value ;

  got_value = zmapVarGetValueWithData(&(thread->reply), state, data, err_msg) ;

  return got_value ;
}


/* User must free returned string, note that we need this routine because pthread_t is defined
 * in very different ways on different systems...... */
char *zMapThreadGetThreadID(ZMapThread thread)
{
  char *thread_id = NULL ;
#ifdef LINUX
  char *format = "%ul" ;
#elif defined DARWIN
  char *format = "%p" ;
#else
  char *format = "%d" ;
#endif

  thread_id = g_strdup_printf(format, thread->thread_id) ;

  return thread_id ;
}



/* Must be kept in step with declaration of ZMapThreadRequest enums in zmapThread_P.h */
char *zMapThreadGetRequestString(ZMapThreadRequest signalled_state)
{
  char *str_states[] = {"ZMAPTHREAD_REQUEST_INIT", "ZMAPTHREAD_REQUEST_WAIT", "ZMAPTHREAD_REQUEST_TIMED_OUT",
			"ZMAPTHREAD_REQUEST_GETDATA"} ;

  return str_states[signalled_state] ;
}

/* Must be kept in step with declaration of ZMapThreadReply enums in zmapThread_P.h */
char *zMapThreadGetReplyString(ZMapThreadReply signalled_state)
{
  char *str_states[] = {"ZMAPTHREAD_REPLY_INIT", "ZMAPTHREAD_REPLY_WAIT",
			"ZMAPTHREAD_REPLY_GOTDATA",  "ZMAPTHREAD_REPLY_REQERROR",
			"ZMAPTHREAD_REPLY_DIED", "ZMAPTHREAD_REPLY_CANCELLED"} ;

  return str_states[signalled_state] ;
}




/* Kill the thread by cancelling it, as this will asynchronously we cannot release the threads
 * resources in this call. */
void zMapThreadKill(ZMapThread thread)
{
  int status ;

  ZMAPTHREAD_DEBUG(("GUI: killing and destroying thread for thread %s\n", zMapThreadGetThreadID(thread))) ;

  /* we could signal an exit here by setting a condvar of EXIT...but that might lead to
   * deadlocks, think about this bit.. */

  /* Signal the thread to cancel it */
  if ((status = pthread_cancel(thread->thread_id)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Thread cancel") ;
    }

  return ;
}

gboolean zMapThreadExists(ZMapThread thread)
{
//      if(pthread_kill(thread->thread_id,0) != ESRCH)
      if(!pthread_kill(thread->thread_id,0))
            return(TRUE);
      return(FALSE);
}

/* Release the threads resources, don't do this until the slave thread has gone. */
void zMapThreadDestroy(ZMapThread thread)
{
  ZMAPTHREAD_DEBUG(("GUI: destroying thread for thread %s\n", zMapThreadGetThreadID(thread))) ;

  zmapVarDestroy(&thread->reply) ;
  zmapCondVarDestroy(&(thread->request)) ;

  g_free(thread) ;

  return ;
}


/*! @} end of zmapthreads docs. */



/*
 * ---------------------  Internal routines  ------------------------------
 */



static ZMapThread createThread(ZMapThreadRequestHandlerFunc handler_func,
			       ZMapThreadTerminateHandler terminate_func, ZMapThreadDestroyHandler destroy_func)
{
  ZMapThread thread ;

  thread = g_new0(ZMapThreadStruct, 1) ;

  thread->handler_func = handler_func ;
  thread->terminate_func = terminate_func ;
  thread->destroy_func = destroy_func ;

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
