/*  Last edited: Jul  8 14:05 2011 (edgrif) */
/*  File: zmapThreadsUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility functions for the slave thread interface.
 *
 * Exported functions: See ZMap/zmapThreads.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <ZMap/zmapUtils.h>
#include <zmapThreads_P.h>


static void releaseCondvarMutex(void *thread_data) ;
static int getAbsTime(const TIMESPEC *relative_timeout, TIMESPEC *abs_timeout) ;



void zmapCondVarCreate(ZMapRequest thread_state)
{
  int status ;

  if ((status = pthread_mutex_init(&(thread_state->mutex), NULL)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "mutex init") ;
    }

  if ((status = pthread_cond_init(&(thread_state->cond), NULL)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "cond init") ;
    }

  thread_state->state = ZMAPTHREAD_REQUEST_INVALID ;

  thread_state->request = NULL ;

  return ;
}

void zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state, void *request)
{
  int status ;

  pthread_cleanup_push(releaseCondvarMutex, (void *)thread_state) ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarSignal mutex lock") ;
    }

  thread_state->state = new_state ;

  /* For some requests there will be no data. */
  if (request)
    thread_state->request = request ;

  if ((status = pthread_cond_signal(&(thread_state->cond))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarSignal cond signal") ;
    }

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarSignal mutex unlock") ;
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */

  return ;
}


/* Blocking wait.... */
void zmapCondVarWait(ZMapRequest thread_state, ZMapThreadRequest waiting_state)
{
  int status ;

  pthread_cleanup_push(releaseCondvarMutex, (void *)thread_state) ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarWait mutex lock") ;
    }

  while (thread_state->state == waiting_state)
    {
      if ((status = pthread_cond_wait(&(thread_state->cond), &(thread_state->mutex))) != 0)
	{
	  zMapLogFatalSysErr(status, "%s", "zmapCondVarWait cond wait") ;
	}
    }

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarWait mutex unlock") ;
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */

  return ;
}


/* timed wait, returns FALSE if cond var was not signalled (i.e. it timed out),
 * TRUE otherwise....
 * NOTE that you can optionally get the condvar state reset to the waiting_state, this
 * can be useful if you are calling this routine from a loop in which you wait until
 * something has changed from the waiting state...i.e. somehow you need to return to the
 * waiting state before looping again. */
ZMapThreadRequest zmapCondVarWaitTimed(ZMapRequest condvar, ZMapThreadRequest waiting_state,
				       TIMESPEC *relative_timeout, gboolean reset_to_waiting,
				       void **data_out)
{
  ZMapThreadRequest signalled_state = ZMAPTHREAD_REQUEST_INVALID ;
  int status ;
  TIMESPEC abs_timeout ;

  pthread_cleanup_push(releaseCondvarMutex, (void *)condvar) ;

  if ((status = pthread_mutex_lock(&(condvar->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarWait mutex lock") ;
    }

  /* Get the relative timeout converted to absolute for the call. */
  if ((status = getAbsTime(relative_timeout, &abs_timeout)) != 0)
    zMapLogFatalSysErr(status, "%s", "zmapCondVarWaitTimed invalid time") ;

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
	    zMapLogFatalSysErr(status, "%s", "zmapCondVarWait cond wait") ;
	}
    }

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

  if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapCondVarWait mutex unlock") ;
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */


  return signalled_state ;
}


void zmapCondVarDestroy(ZMapRequest thread_state)
{
  int status ;

  if ((status = pthread_cond_destroy(&(thread_state->cond))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "cond destroy") ;
//	zMapLogWarning("%s","cond_destroy");
    }


  if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "mutex destroy") ;
//	zMapLogWarning("%s","mutex_destroy");
    }

  return ;
}



/* this set of routines manipulates the variable in the thread state struct but do not
 * involve the Condition Variable. */

void zmapVarCreate(ZMapReply thread_state)
{
  int status ;

  if ((status = pthread_mutex_init(&(thread_state->mutex), NULL)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "mutex init") ;
    }


  thread_state->state = ZMAPTHREAD_REPLY_INVALID ;

  return ;
}



void zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValue mutex lock") ;
    }

  thread_state->state = new_state ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValue mutex unlock") ;
    }

  return ;
}


/* Returns TRUE if it could read the value (i.e. the mutex was unlocked)
 * and returns the value in state_out,
 * returns FALSE otherwise. */
gboolean zmapVarGetValue(ZMapReply thread_state, ZMapThreadReply *state_out)
{
  gboolean unlocked = TRUE ;
  int status ;

  if ((status = pthread_mutex_trylock(&(thread_state->mutex))) != 0)
    {
      if (status == EBUSY)
	unlocked = FALSE ;
      else
	zMapLogFatalSysErr(status, "%s", "zmapVarGetValue mutex lock") ;
    }
  else
    {
      *state_out = thread_state->state ;
      unlocked = TRUE ;

      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  zMapLogFatalSysErr(status, "%s", "zmapVarGetValue mutex unlock") ;
	}
    }

  return unlocked ;
}


void zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithData mutex lock") ;
    }

  thread_state->state = new_state ;
  thread_state->reply = data ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithData mutex unlock") ;
    }

  return ;
}


void zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, char *err_msg)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithError mutex lock") ;
    }

  thread_state->state = new_state ;

  if (err_msg)
    thread_state->error_msg = err_msg ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithError mutex unlock") ;
    }

  return ;
}


/* Sometimes for Errors we need to return some of the data as it includeds request
 * details and so on. */
void zmapVarSetValueWithErrorAndData(ZMapReply thread_state, ZMapThreadReply new_state,
				     char *err_msg, void *data)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithError mutex lock") ;
    }

  thread_state->state = new_state ;

  if (err_msg)
    thread_state->error_msg = err_msg ;
  thread_state->reply = data ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "zmapVarSetValueWithError mutex unlock") ;
    }

  return ;
}


/* Returns TRUE if it could read the value (i.e. the mutex was unlocked)
 * and returns the value in state_out and also if there is any data it
 * is returned in data_out and if there is an err_msg it is returned in err_msg_out,
 * returns FALSE if it could not read the value. */
gboolean zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
				 void **data_out, char **err_msg_out)
{
  gboolean unlocked = TRUE ;
  int status ;

  if ((status = pthread_mutex_trylock(&(thread_state->mutex))) != 0)
    {
      if (status == EBUSY)
	unlocked = FALSE ;
      else
	zMapLogFatalSysErr(status, "%s", "zmapVarGetValue mutex lock") ;
    }
  else
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

      unlocked = TRUE ;

      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  zMapLogFatalSysErr(status, "%s", "zmapVarGetValue mutex unlock") ;
	}
    }

  return unlocked ;
}


void zmapVarDestroy(ZMapReply thread_state)
{
  int status ;

  if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "mutex destroy") ;
    }

  return ;
}





/*
 * -----------------------  Internal Routines  -----------------------
 */

/* Called when a thread gets cancelled while waiting on a mutex to ensure that the mutex
 * gets released. */
static void releaseCondvarMutex(void *thread_data)
{
  ZMapRequest condvar = (ZMapRequest)thread_data ;
  int status ;

  ZMAPTHREAD_DEBUG(("releaseCondvarMutex cleanup handler\n")) ;

  if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "releaseCondvarMutex cleanup handler - mutex unlock") ;
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
