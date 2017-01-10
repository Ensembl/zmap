/*  File: zmapThreads_P.h
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Internals for the thread control code.
 *              
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_PRIV_H
#define ZMAP_THREAD_PRIV_H

#include <config.h>
#include <pthread.h>
#include <glib.h>

#include <ZMap/zmapThreadsLib.hpp>



enum class ThreadState {INVALID, INIT, CONNECTED, FINISHING, FINISHED} ;



/* NOTE THIS WILL GO AWAY ONCE ITS SORTED OUT IN AUTOCONF.... */
/* Seems that Linux defines the time structure used in some pthread calls as "timespec",
 * whereas alphas call it "timespec_t"....deep sigh.... */
#if defined LINUX
#define TIMESPEC struct timespec
#elif defined DARWIN
#define TIMESPEC struct timespec
#else
#define TIMESPEC timespec_t
#endif




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define ZMAPTHREAD_DEBUG_PREFIX "%s"

#define ZMAPTHREAD_DEBUG(CHILD_THREAD, FORMAT_STR, ...)                 \
  G_STMT_START                                                          \
  {                                                                     \
    if (zmap_thread_debug_G)                                            \
      {                                                                 \
        char *thread_str, *thread_msg_str ;                             \
                                                                        \
        if ((thread_str = zMapThreadGetThreadID((CHILD_THREAD))))       \
          {                                                             \
            thread_msg_str = g_strdup_printf(" slave thread %s, ", thread_str) ; \
            g_free(thread_str) ;                                        \
          }                                                             \
        else                                                            \
          {                                                             \
            thread_msg_str = g_strdup(" no slave thread, ") ;           \
          }                                                             \
                                                                        \
        zMapDebugPrint(zmap_thread_debug_G, ZMAPTHREAD_DEBUG_PREFIX FORMAT_STR, thread_msg_str, __VA_ARGS__) ; \
                                                                        \
        zMapLogMessage(ZMAPTHREAD_DEBUG_PREFIX FORMAT_STR, thread_msg_str, __VA_ARGS__) ;     \
                                                                        \
        g_free(thread_msg_str) ;                                        \
      }                                                                 \
  } G_STMT_END
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Requests are via a cond var system. */
typedef struct ZMapRequestStructType
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  pthread_cond_t cond ;					    /* Slave waits on this. */
  ZMapThreadRequest state ;				    /* Thread request to slave. */
  void *request ;					    /* Request from caller. */
} ZMapRequestStruct, *ZMapRequest ;


/* Replies via a simpler mutex. */
typedef struct
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  ZMapThreadReply state ;				    /* Thread reply from slave. */
  void *reply ;						    /* Reply from callee. */
  char *error_msg ;                                         /* Error message for when thread fails. */
} ZMapReplyStruct, *ZMapReply ;



/* The ZMapThread, this structure represents a slave thread. */
typedef struct _ZMapThreadStruct
{
  ThreadState state ;

  pthread_t thread_id ;

  // TEMP WHILE I SORT OUT SLAVE INTERFACE....
  bool new_interface ;

  /* These control the communication between the GUI thread and the thread threads,
   * they are mutex locked and the request code makes use of the condition variable to
   * communicate with slave threads. */
  ZMapRequestStruct request ;				    /* GUIs request. */
  ZMapReplyStruct reply ;				    /* Slaves reply. */

  // Handler funcs used in slave part of thread interface.
  ZMapSlaveRequestHandlerFunc req_handler_func ;
  ZMapSlaveTerminateHandlerFunc terminate_handler_func ;
  ZMapSlaveDestroyHandlerFunc destroy_handler_func ;

} ZMapThreadStruct ;



// This thread routine should only be called from the slave thread to show that it has terminated.

void zmapThreadFinish(ZMapThread thread) ;




/* Request routines. */
bool zmapCondVarCreate(ZMapRequest thread_state) ;
bool zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state, void *request) ;
ZMapThreadRequest zmapCondVarWaitTimed(ZMapRequest condvar, ZMapThreadRequest waiting_state,
				       TIMESPEC *timeout, gboolean reset_to_waiting, void **data) ;
bool zmapCondVarDestroy(ZMapRequest thread_state) ;


/* Reply routines. */
bool zmapVarCreate(ZMapReply thread_state) ;
bool zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state) ;
bool zmapVarGetValue(ZMapReply thread_state, ZMapThreadReply *state_out) ;
bool zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data) ;
bool zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, const char *err_msg) ;
bool zmapVarSetValueWithErrorAndData(ZMapReply thread_state, ZMapThreadReply new_state,
				     const char *err_msg, void *data) ;
bool zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
                             void **data_out, char **err_msg_out) ;
bool zmapVarDestroy(ZMapReply thread_state) ;


#endif /* !ZMAP_THREAD_PRIV_H */
