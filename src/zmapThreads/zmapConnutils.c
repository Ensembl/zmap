/*  File: zmapthrutils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar  1 14:26 2004 (edgrif)
 * Created: Thu Jul 24 14:37:35 2003 (edgrif)
 * CVS info:   $Id: zmapConnutils.c,v 1.3 2004-03-03 12:06:27 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <errno.h>
#include <zmapConn_P.h>


static void releaseCondvarMutex(void *thread_data) ;
static int getAbsTime(const TIMESPEC *relative_timeout, TIMESPEC *abs_timeout) ;



void zmapCondVarCreate(ZMapRequest thread_state)
{
  int status ;

  if ((status = pthread_mutex_init(&(thread_state->mutex), NULL)) != 0)
    {
      ZMAPSYSERR(status, "mutex init") ;
    }

  if ((status = pthread_cond_init(&(thread_state->cond), NULL)) != 0)
    {
      ZMAPSYSERR(status, "cond init") ;
    }
  
  thread_state->state = ZMAP_REQUEST_INIT ;
  
  return ;
}

void zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state)
{
  int status ;
  
  pthread_cleanup_push(releaseCondvarMutex, (void *)thread_state) ;
  
  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarSignal mutex lock") ;
    }

  thread_state->state = new_state ;

  if ((status = pthread_cond_signal(&(thread_state->cond))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarSignal cond signal") ;
    }

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarSignal mutex unlock") ;
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
      ZMAPSYSERR(status, "zmapCondVarWait mutex lock") ;
    }

  while (thread_state->state == waiting_state)
    {
      if ((status = pthread_cond_wait(&(thread_state->cond), &(thread_state->mutex))) != 0)
	{
	  ZMAPSYSERR(status, "zmapCondVarWait cond wait") ;
	}
    }

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarWait mutex unlock") ;
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
				       TIMESPEC *relative_timeout, gboolean reset_to_waiting)
{
  ZMapThreadRequest signalled_state = ZMAP_REQUEST_INIT ;
  int status ;
  TIMESPEC abs_timeout ;


  pthread_cleanup_push(releaseCondvarMutex, (void *)condvar) ;


  if ((status = pthread_mutex_lock(&(condvar->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarWait mutex lock") ;
    }
  
  /* Get the relative timeout converted to absolute for the call. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((status = pthread_get_expiration_np(relative_timeout, &abs_timeout)) != 0)
    ZMAPSYSERR(status, "zmapCondVarWaitTimed invalid time") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if ((status = getAbsTime(relative_timeout, &abs_timeout)) != 0)
    ZMAPSYSERR(status, "zmapCondVarWaitTimed invalid time") ;

  while (condvar->state == waiting_state)
    {
      if ((status = pthread_cond_timedwait(&(condvar->cond), &(condvar->mutex),
					   &abs_timeout)) != 0)
	{
	  if (status == ETIMEDOUT)			    /* Timed out so return. */
	    {
	      condvar->state = ZMAP_REQUEST_TIMED_OUT ;
	      break ;
	    }
	  else
	    ZMAPSYSERR(status, "zmapCondVarWait cond wait") ;
	}
    }

  signalled_state = condvar->state ;			    /* return signalled end state. */

  /* optionally reset current state to wait state. */
  if (reset_to_waiting)
    condvar->state = waiting_state ;

  if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapCondVarWait mutex unlock") ;
    }

  pthread_cleanup_pop(0) ;				    /* 0 => only call cleanup if cancelled. */

  return signalled_state ;
}


void zmapCondVarDestroy(ZMapRequest thread_state)
{
  int status ;
  
  if ((status = pthread_cond_destroy(&(thread_state->cond))) != 0)
    {
      ZMAPSYSERR(status, "cond destroy") ;
    }
  

  if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "mutex destroy") ;
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
      ZMAPSYSERR(status, "mutex init") ;
    }

  thread_state->state = ZMAP_REPLY_INIT ;
  
  return ;
}



void zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValue mutex lock") ;
    }

  thread_state->state = new_state ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValue mutex unlock") ;
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
	ZMAPSYSERR(status, "zmapVarGetValue mutex lock") ;
    }
  else
    {
      *state_out = thread_state->state ;
      unlocked = TRUE ;

      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  ZMAPSYSERR(status, "zmapVarGetValue mutex unlock") ;
	}
    }

  return unlocked ;
}


void zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValueWithData mutex lock") ;
    }

  thread_state->state = new_state ;
  thread_state->data = data ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValueWithData mutex unlock") ;
    }

  return ;
}


void zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, char *err_msg)
{
  int status ;

  if ((status = pthread_mutex_lock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValueWithError mutex lock") ;
    }

  thread_state->state = new_state ;
  thread_state->error_msg = err_msg ;

  if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "zmapVarSetValueWithError mutex unlock") ;
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
	ZMAPSYSERR(status, "zmapVarGetValue mutex lock") ;
    }
  else
    {
      *state_out = thread_state->state ;
      if (thread_state->data)
	*data_out = thread_state->data ;
      if (thread_state->error_msg)
	*err_msg_out = thread_state->error_msg ;

      unlocked = TRUE ;

      if ((status = pthread_mutex_unlock(&(thread_state->mutex))) != 0)
	{
	  ZMAPSYSERR(status, "zmapVarGetValue mutex unlock") ;
	}
    }

  return unlocked ;
}


void zmapVarDestroy(ZMapReply thread_state)
{
  int status ;
  
  if ((status = pthread_mutex_destroy(&(thread_state->mutex))) != 0)
    {
      ZMAPSYSERR(status, "mutex destroy") ;
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

  ZMAP_THR_DEBUG(("releaseCondvarMutex cleanup handler\n")) ;

  if ((status = pthread_mutex_unlock(&(condvar->mutex))) != 0)
    {
      ZMAPSYSERR(status, "releaseCondvarMutex cleanup handler - mutex unlock") ;
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

  
  clock_tick_nano = 1000000000 / CLK_TCK ;		    /* CLK_TCK can be a function. */

  if ((relative_timeout->tv_sec > 0 && relative_timeout->tv_nsec >= 0)
      || (relative_timeout->tv_sec == 0 && relative_timeout->tv_nsec > clock_tick_nano))
    {
      *abs_timeout = *relative_timeout ;		    /* struct copy */
      abs_timeout->tv_sec += time(NULL) ;

      status = 0 ;
    }


  return status ;
}
