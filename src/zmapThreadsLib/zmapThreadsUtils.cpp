/*  File: zmapThreadsUtils.c
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
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility functions for the slave thread interface.
 *
 * Exported functions: See ZMap/zmapThreads.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdio.h>
#include <errno.h>
#include <time.h>

#include <ZMap/zmapUtils.hpp>
#include <zmapThreads_P.hpp>


static void releaseCondvarMutex(void *thread_data) ;
static int getAbsTime(const TIMESPEC *relative_timeout, TIMESPEC *abs_timeout) ;



bool zmapCondVarCreate(ZMapRequest thread_state)
{
  bool result = FALSE ;
  int status = 0 ;

  if (status == 0
      && ((status = pthread_mutex_init(&(thread_state->mutex), NULL)) != 0))
    {
      zMapLogCriticalSysErr(status, "%s", "mutex init") ;
    }

  if (status == 0
      && ((status = pthread_cond_init(&(thread_state->cond), NULL)) != 0))
    {
      zMapLogCriticalSysErr(status, "%s", "cond init") ;
    }

  if (status == 0)
    {
      thread_state->state = ZMAPTHREAD_REQUEST_INVALID ;
      thread_state->request = NULL ;

      result = true ;
    }

  return result ;
}

// Note that if we manage to lock but then something else fails that the condvar is left
// locked and then can't be used again.
// This is probably the right thing to do as either it's all working or there is something
// really wrong !
bool zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state, void *request)
{
  bool result = FALSE ;
  int status = 0 ;

  pthread_cleanup_push(releaseCondvarMutex, (void *)thread_state) ;

  // Lock....
  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapCondVarSignal mutex lock") ;
        }
    }

  // Set the data in the condvar.
  if (status == 0)
    {
      thread_state->state = new_state ;

      /* For some requests there will be no data. */
      if (request)
        thread_state->request = request ;

      if ((status = pthread_cond_signal(&(thread_state->cond))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapCondVarSignal cond signal") ;
        }
    }

  // Unlock...
  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapCondVarSignal mutex unlock") ;
        }
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */

  // Did we manage to set the cond var ?
  if (status == 0)
    result = true ;


  return result ;
}


/* timed wait
 * 
 * NOTE that you can optionally get the condvar state reset to the waiting_state, this
 * can be useful if you are calling this routine from a loop in which you wait until
 * something has changed from the waiting state...i.e. somehow you need to return to the
 * waiting state before looping again. */
ZMapThreadRequest zmapCondVarWaitTimed(ZMapRequest condvar, ZMapThreadRequest waiting_state,
				       TIMESPEC *relative_timeout, gboolean reset_to_waiting,
				       void **data_out)
{
  ZMapThreadRequest signalled_state = ZMAPTHREAD_REQUEST_INVALID ;
  int status = 0 ;
  TIMESPEC abs_timeout ;

  pthread_cleanup_push(releaseCondvarMutex, (void *)condvar) ;

  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(condvar->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapCondVarWait mutex lock") ;
        }
    }


  /* Get the relative timeout converted to absolute for the call. */
  if (status == 0)
    {
      if ((status = getAbsTime(relative_timeout, &abs_timeout)) != 0)
        zMapLogCriticalSysErr(status, "%s", "zmapCondVarWaitTimed invalid time") ;
    }

  if (status == 0)
    {
      while (condvar->state == waiting_state)
        {
          if ((status = pthread_cond_timedwait(&(condvar->cond), &(condvar->mutex),
                                               &abs_timeout)) != 0)
            {
              if (status == ETIMEDOUT)			    /* Timed out so return. */
                {
                  condvar->state = ZMAPTHREAD_REQUEST_TIMED_OUT ;
                  break ;
                }
              else
                {
                  zMapLogCriticalSysErr(status, "%s", "zmapCondVarWait cond wait") ;
                }
            }
        }
    }

  
  if (status == 0)
    {
      signalled_state = condvar->state ;			    /* return signalled end state. */

      /* optionally reset current state to wait state. */
      if (reset_to_waiting)
        condvar->state = waiting_state ;

      /* Return data if there is some, seems to make sense only to do this if we _haven't_ timed out.
       * Note how we reset condvar->request so we detect if new data comes in next time. */
      if (condvar->state != ZMAPTHREAD_REQUEST_TIMED_OUT)
        {
          if (condvar->request)
            {
              *data_out = condvar->request ;
              condvar->request = NULL ;
            }
        }
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapCondVarWait mutex unlock") ;
        }
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */

  return signalled_state ;
}


bool zmapCondVarDestroy(ZMapRequest thread_state)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_cond_destroy(&(thread_state->cond))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "cond destroy") ;
        }
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "mutex destroy") ;
        }
    }

  if (status == 0)
    result = true ;

  return result ;
}



/* this set of routines manipulates the variable in the thread state struct but do not
 * involve the Condition Variable. */

bool zmapVarCreate(ZMapReply thread_state)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_init(&(thread_state->mutex), NULL)) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "mutex init") ;
        }
    }

  if (status == 0)
    {
      thread_state->state = ZMAPTHREAD_REPLY_INVALID ;

      result = true ;
    }

  return result ;
}



bool zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValue mutex lock") ;
        }
    }

  if (status == 0)
    {
      thread_state->state = new_state ;
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValue mutex unlock") ;
        }
    }

  if (status)
    result = true ;

  return result ;
}


/* Returns TRUE if it could read the value (i.e. the mutex was unlocked)
 * and returns the value in state_out,
 * returns FALSE otherwise. */
bool zmapVarGetValue(ZMapReply thread_state, ZMapThreadReply *state_out)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_trylock(&(thread_state->mutex))) != 0)
        {
          if (status != EBUSY)
            zMapLogCriticalSysErr(status, "%s", "zmapVarGetValue mutex lock") ;
        }
    }

  if (status == 0)
    {
      *state_out = thread_state->state ;
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  zMapLogCriticalSysErr(status, "%s", "zmapVarGetValue mutex unlock") ;
	}
    }

  if (status == 0)
    {
      result = true ;
    }

  return result ;
}


bool zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithData mutex lock") ;
        }
    }

  if (status == 0)
    {
      thread_state->state = new_state ;
      thread_state->reply = data ;
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithData mutex unlock") ;
        }
    }

  if (status == 0)
    {
      result = true ;
    }

  return result ;
}


bool zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, const char *err_msg)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithError mutex lock") ;
        }
    }

  if (status == 0)
    {
      thread_state->state = new_state ;

      if (err_msg)
        {
          if (thread_state->error_msg)
            g_free(thread_state->error_msg) ;

          thread_state->error_msg = g_strdup(err_msg) ;
        }
    }


  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithError mutex unlock") ;
        }
    }

  if (status == 0)
    {
      result = true ;
    }

  return result ;
}


/* Sometimes for Errors we need to return some of the data as it includeds request
 * details and so on. */
bool zmapVarSetValueWithErrorAndData(ZMapReply thread_state, ZMapThreadReply new_state,
				     const char *err_msg, void *data)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithError mutex lock") ;
        }
    }

  if (status == 0)
    {
      thread_state->state = new_state ;

      if (err_msg)
        {
          if (thread_state->error_msg)
            g_free(thread_state->error_msg) ;

          thread_state->error_msg = g_strdup(err_msg) ;
        }

      thread_state->reply = data ;
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "zmapVarSetValueWithError mutex unlock") ;
        }
    }

  if (status == 0)
    {
      result = true ;
    }

  return result ;
}


/* Returns TRUE if it could read the value (i.e. the mutex was unlocked)
 * and returns the value in state_out and also if there is any data it
 * is returned in data_out and if there is an err_msg it is returned in err_msg_out,
 * returns FALSE if it could not read the value. */
bool zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
                             void **data_out, char **err_msg_out)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_trylock(&(thread_state->mutex))) != 0)
        {
          if (status != EBUSY)
            zMapLogCriticalSysErr(status, "%s", "zmapVarGetValue mutex lock") ;
        }
    }

  if (status == 0)
    {
      *state_out = thread_state->state ;

      if (thread_state->reply)
	{
	  *data_out = thread_state->reply ;
	  thread_state->reply = NULL ;
	}

      if (thread_state->error_msg)
	{
	  *err_msg_out = thread_state->error_msg ;
	  thread_state->error_msg = NULL ;
	}
    }

  if (status == 0)
    {
      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  zMapLogCriticalSysErr(status, "%s", "zmapVarGetValue mutex unlock") ;
	}
    }

  if (status == 0)
    {
      result = TRUE ;
    }

  return result ;
}


bool zmapVarDestroy(ZMapReply thread_state)
{
  bool result = false ;
  int status = 0 ;

  if (status == 0)
    {
      if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
        {
          zMapLogCriticalSysErr(status, "%s", "mutex destroy") ;
        }
    }

  if (status == 0)
    {
      result = TRUE ;
    }

  return result ;
}





/*
 *                           Internal Routines
 */

/* Called when a thread gets cancelled while waiting on a mutex to ensure that the mutex
 * gets released. */
static void releaseCondvarMutex(void *thread_data)
{
  ZMapRequest condvar = (ZMapRequest)thread_data ;
  int status ;

  zMapDebugPrint(zmap_thread_debug_G, "%s", "thread cancelled while waiting on mutex, in cleanup handler") ;

  if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
    {
      zMapLogCriticalSysErr(status, "%s", "releaseCondvarMutex cleanup handler - mutex unlock") ;
    }

  return ;
}


/* This function is a cheat really. You can only portably get the time in seconds
 * as far as I can see so specifying small relative timeouts will not work....
 * to this end I have inserted code to check that the relative timeout is not
 * less than a single clock tick, hardly perfect but better than nothing.
 *
 * On the alpha you can do this:
 *
 *       pthread_get_expiration_np(relative_timeout, &abs_timeout)
 *
 * to get a timeout in seconds and nanoseconds but this call is unavailable on
 * Linux at least....
 *
 *  */
static int getAbsTime(const TIMESPEC *relative_timeout, TIMESPEC *abs_timeout)
{
  int status = 1 ;					    /* Fail by default. */
  static int clock_tick_nano ;

  /* We used to use CLK_TCK but this seems not to be available on the latest POSIX systems
   * so I've switched to CLOCKS_PER_SEC which is supported and if literature is correct
   * is the same thing. */
  clock_tick_nano = 1000000000 / CLOCKS_PER_SEC ;


  if ((relative_timeout->tv_sec > 0 && relative_timeout->tv_nsec >= 0)
      || (relative_timeout->tv_sec == 0 && relative_timeout->tv_nsec > clock_tick_nano))
    {
      *abs_timeout = *relative_timeout ;		    /* struct copy */
      abs_timeout->tv_sec += time(NULL) ;

      status = 0 ;
    }

  return status ;
}
