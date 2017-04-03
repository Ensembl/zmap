/*  File: zmapThreads.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
 * Description: Interface to low level thread creation, destruction,
 *              setting/getting data from threads.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREADSLIB_H
#define ZMAP_THREADSLIB_H

#include <config.h>
#include <glib.h>

#include <ZMap/zmapEnum.hpp>
#include <ZMap/zmapThreadSlave.hpp>





// Thread debugging messages

enum class ZMapThreadType {MASTER, SLAVE} ;

/* We should have a function to access this global.... */
/* debugging messages, TRUE for on... */
extern bool zmap_thread_debug_G ;


#define ZMAPTHREAD_DEBUG_PREFIX " %s "

#define ZMAPTHREAD_DEBUG_MSG(CALLER_THREAD_TYPE, CALLER_THREAD, CALLEE_THREAD_TYPE, CALLEE_THREAD, FORMAT_STR, ...) \
  G_STMT_START                                                          \
  {                                                                     \
    if (zmap_thread_debug_G)                                            \
      {                                                                 \
        char *prefix_str ;                                              \
                                                                        \
        prefix_str = zmapThreadGetDebugPrefix(CALLER_THREAD_TYPE, CALLER_THREAD, \
                                              CALLEE_THREAD_TYPE, CALLEE_THREAD) ; \
                                                                        \
        zMapDebugPrint(zmap_thread_debug_G, ZMAPTHREAD_DEBUG_PREFIX FORMAT_STR, \
                       prefix_str, __VA_ARGS__) ;                       \
                                                                        \
        zMapLogMessage(ZMAPTHREAD_DEBUG_PREFIX FORMAT_STR, prefix_str, __VA_ARGS__) ; \
                                                                        \
        g_free((void *)prefix_str) ;                                    \
      }                                                                 \
  } G_STMT_END



/* Requests to a slave thread. */
#define ZMAP_THREAD_REQUEST_LIST(_)                                             \
_(ZMAPTHREAD_REQUEST_INVALID,    , "invalid",    "invalid request. ",       "") \
_(ZMAPTHREAD_REQUEST_WAIT,       , "wait",       "Wait for next request. ", "")	\
_(ZMAPTHREAD_REQUEST_TIMED_OUT,  , "timed_out",  "You have timed out. ",    "")	\
_(ZMAPTHREAD_REQUEST_EXECUTE,    , "execute",    "Execute request. ",       "") \
_(ZMAPTHREAD_REQUEST_TERMINATE,  , "terminate",  "Terminate and return.",   "")

ZMAP_DEFINE_ENUM(ZMapThreadRequest, ZMAP_THREAD_REQUEST_LIST) ;


/* Replies from a slave thread. */
#define ZMAP_THREAD_REPLY_LIST(_)             \
_(ZMAPTHREAD_REPLY_INVALID,    , "invalid",          "Invalid reply. ", "") \
_(ZMAPTHREAD_REPLY_WAIT,       , "waiting",          "Thread waiting. ", "") \
_(ZMAPTHREAD_REPLY_GOTDATA,    , "got_data",         "Thread returning data. ", "") \
_(ZMAPTHREAD_REPLY_REQERROR,   , "request_error",    "Thread received bad request. ",    "") \
_(ZMAPTHREAD_REPLY_DIED,       , "have_died",        "Thread has died unexpectedly. ",   "") \
_(ZMAPTHREAD_REPLY_CANCELLED,  , "thread_cancelled", "Thread has been cancelled. ",      "") \
_(ZMAPTHREAD_REPLY_QUIT,       , "quit",             "Thread has terminated normally. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadReply, ZMAP_THREAD_REPLY_LIST) ;




/* The thread, one per connection, an opaque private type. */
typedef struct _ZMapThreadStruct *ZMapThread ;


typedef void *(*ZMapThreadCreateFunc)(void *func_data) ;



//temp....
char *zmapThreadGetDebugPrefix(ZMapThreadType caller_thread_type, ZMapThread caller_thread,
                               ZMapThreadType callee_thread_type, ZMapThread callee_thread) ;






// bool "new_interface" is TEMP NEW/OLD INTERFACE WHILE I SET UP THE NEW THREADS STUFF....
// true means use new interface, false the old/existing interface.
ZMapThread zMapThreadCreate(bool new_interface,
                            ZMapSlaveRequestHandlerFunc req_handler_func,
                            ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                            ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;
bool zMapThreadStart(ZMapThread thread, ZMapThreadCreateFunc create_func) ;

bool zMapThreadRequest(ZMapThread thread, void *request) ;
bool zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state) ;
bool zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state) ;
bool zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state,
				    void **data, char **err_msg) ;
char *zMapThreadGetThreadID(ZMapThread thread) ;
char *zMapThreadGetRequestString(ZMapThreadRequest signalled_state) ;
char *zMapThreadGetReplyString(ZMapThreadReply signalled_state) ;
bool zMapThreadExists(ZMapThread thread);
bool zMapIsThreadFinished(ZMapThread thread) ;

bool zMapThreadStop(ZMapThread thread) ;
void zMapThreadKill(ZMapThread thread) ;
bool zMapThreadDestroy(ZMapThread thread) ;


ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadRequest2ExactStr, ZMapThreadRequest) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReply2ExactStr, ZMapThreadReply) ;


#endif /* !ZMAP_THREADSLIB_H */
