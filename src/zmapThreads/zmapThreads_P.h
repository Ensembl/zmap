/*  File: zmapThreads_P.h
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

#include <ZMap/zmapThreads.h>



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



/* Standard message macro for thread debug messages.
 * Note: the FORMAT parameter MUST be a string literal which has the printf style
 * format syntax describing the rest of the arguments to the macro.
 *  */
#define ZMAPTHREAD_DEBUG(THREAD, FORMAT_STR, ...)		    \
  zMapDebugPrint(zmap_thread_debug_G, "%s: " FORMAT_STR, zMapThreadGetThreadID((THREAD)), __VA_ARGS__)



/* Requests are via a cond var system. */
typedef struct
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
  gchar *error_msg ;					    /* Error message for when thread fails. */
} ZMapReplyStruct, *ZMapReply ;



/* The ZMapThread, this structure represents a slave thread. */
typedef struct _ZMapThreadStruct
{
  pthread_t thread_id ;

  /* These control the communication between the GUI thread and the thread threads,
   * they are mutex locked and the request code makes use of the condition variable to
   * communicate with slave threads. */
  ZMapRequestStruct request ;				    /* GUIs request. */
  ZMapReplyStruct reply ;				    /* Slaves reply. */

  /* User registered routine which the thread calls to handle requests and replies. */
  ZMapThreadRequestHandlerFunc handler_func ;

  /* User registered routine to terminate thread if it needs to exit abnormally. */
  ZMapThreadTerminateHandler terminate_func ;

  /* User registered routine to destroy/clean up thread if it needs to exit abnormally. */
  ZMapThreadDestroyHandler destroy_func ;

} ZMapThreadStruct ;


/* Thread calls. */
void *zmapNewThread(void *thread_args) ;


/* Request routines. */
void zmapCondVarCreate(ZMapRequest thread_state) ;
void zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state, void *request) ;
void zmapCondVarWait(ZMapRequest thread_state, ZMapThreadRequest waiting_state) ;
ZMapThreadRequest zmapCondVarWaitTimed(ZMapRequest condvar, ZMapThreadRequest waiting_state,
				       TIMESPEC *timeout, gboolean reset_to_waiting, void **data) ;
void zmapCondVarDestroy(ZMapRequest thread_state) ;


/* Reply routines. */
void zmapVarCreate(ZMapReply thread_state) ;
void zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state) ;
gboolean zmapVarGetValue(ZMapReply thread_state, ZMapThreadReply *state_out) ;
void zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data) ;
void zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, char *err_msg) ;
void zmapVarSetValueWithErrorAndData(ZMapReply thread_state, ZMapThreadReply new_state,
				     char *err_msg, void *data) ;
gboolean zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
				 void **data_out, char **err_msg_out) ;
void zmapVarDestroy(ZMapReply thread_state) ;


#endif /* !ZMAP_THREAD_PRIV_H */
