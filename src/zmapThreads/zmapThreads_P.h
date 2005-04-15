/*  File: zmapThreads_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Description: Internals for the thread control code.
 *              
 * HISTORY:
 * Last edited: Apr 15 17:43 2005 (edgrif)
 * Created: Thu Jan 27 11:18:44 2005 (edgrif)
 * CVS info:   $Id: zmapThreads_P.h,v 1.4 2005-04-15 18:08:52 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_PRIV_H
#define ZMAP_THREAD_PRIV_H

#include <config.h>
#include <pthread.h>
#include <glib.h>
#include <ZMap/zmapSys.h>
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

#define ZMAPTHREAD_DEBUG(ALL_ARGS_WITH_BRACKETS) \
do { \
     if (zmap_thread_debug_G) \
       printf ALL_ARGS_WITH_BRACKETS ; \
   } while (0)


/* Requests are via a cond var system. */
typedef struct
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  pthread_cond_t cond ;					    /* Slave waits on this. */
  ZMapThreadRequest state ;				    /* Thread request to slave. */
  void *request ;						    /* Actual request from caller. */
} ZMapRequestStruct, *ZMapRequest ;


/* Replies via a simpler mutex. */
typedef struct
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  ZMapThreadReply state ;				    /* Thread reply from slave. */
  void *reply ;					    /* Actual reply from callee. */
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

  /* User registered routine to which the thread calls to handle requests and replies. */
  ZMapThreadRequestHandlerFunc handler_func ;

  /* User registered routine to be called when a thread has to exit abnormally. */
  ZMapThreadTerminateHandler terminate_func ;

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
gboolean zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
				 void **data_out, char **err_msg_out) ;
void zmapVarDestroy(ZMapReply thread_state) ;


#endif /* !ZMAP_THREAD_PRIV_H */
