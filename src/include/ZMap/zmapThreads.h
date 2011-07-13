/*  Last edited: Jul  8 14:02 2011 (edgrif) */
/*  File: zmapThreads.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * Description: Interface to sub threads of the ZMap GUI thread.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_H
#define ZMAP_THREAD_H
#include <config.h>
#if defined DARWIN
#include <pthread.h>
#endif

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* WARNING, IN THEORY WE SHOULD INCLUDE THIS AS WE REFERENCE A PTHREAD TYPE BELOW,
 * _BUT_ IF WE DO THEN WE RUN INTO TROUBLE ON THE ALPHAS AS FOR UNKNOWN AND PROBABLY
 * BIZARRE REASONS IT FAILS TO COMPILE COMPLAINING ABOUT "leave" IN THE Window/View
 * HEADERS TO BE ILLEGAL...THEY INCLUDE THIS HEADER...SIGH... WE COULD JUST HACK
 * OUR OWN VERSION OF THE pthread_t type to get round this.... */

#include <pthread.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#include <ZMap/zmapEnum.h>


/* The calls need changing to handle a more general
 * mechanism of requests etc.....this interface should not need to know the requests it is
 * servicing.....all the server stuff needs to be removed from the conncreate and these
 * routines should be renamed to  zMapThreadNNNN */


/* We should have a function to access this global.... */
/* debugging messages, TRUE for on... */
extern gboolean zmap_thread_debug_G ;


/* Requests to a slave thread. */
#define ZMAP_THREAD_REQUEST_LIST(_)             \
_(ZMAPTHREAD_REQUEST_INVALID,   , "invalid",    "invalid request. ", "")   \
_(ZMAPTHREAD_REQUEST_WAIT,      , "wait",       "Wait for next request. ", "")	\
_(ZMAPTHREAD_REQUEST_TIMED_OUT, , "timed_out",  "You have timed out. ", "")	\
_(ZMAPTHREAD_REQUEST_EXECUTE,   , "execute",    "Execute request. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadRequest, ZMAP_THREAD_REQUEST_LIST) ;


/* Replies from a slave thread. */
#define ZMAP_THREAD_REPLY_LIST(_)             \
_(ZMAPTHREAD_REPLY_INVALID,   , "invalid",          "Invalid reply. ", "") \
_(ZMAPTHREAD_REPLY_WAIT,      , "waiting",          "Thread waiting. ", "") \
_(ZMAPTHREAD_REPLY_GOTDATA,   , "got_data",         "Thread returning data. ", "") \
_(ZMAPTHREAD_REPLY_REQERROR,  , "request_error",    "Thread received bad request. ", "") \
_(ZMAPTHREAD_REPLY_DIED,      , "have_died",        "Thread has died unexpectedly. ", "") \
_(ZMAPTHREAD_REPLY_CANCELLED, , "thread_cancelled", "Thread has been cancelled. ", "") \
_(ZMAPTHREAD_REPLY_QUIT,    ,   "quit",             "Thread has terminated normally. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadReply, ZMAP_THREAD_REPLY_LIST) ;


/* Return codes from the handler function called by the slave thread to service a request. */
#define ZMAP_THREAD_RETURNCODE_LIST(_)             \
_(ZMAPTHREAD_RETURNCODE_INVALID,    , "invalid",        "Invalid return code. ", "") \
_(ZMAPTHREAD_RETURNCODE_OK,         , "ok",             "OK. ", "") \
_(ZMAPTHREAD_RETURNCODE_TIMEDOUT,   , "timed_out",      "Timed out. ", "") \
_(ZMAPTHREAD_RETURNCODE_REQFAIL,    , "request_failed", "Request failed. ", "") \
_(ZMAPTHREAD_RETURNCODE_BADREQ,     , "bad_request",    "Invalid request. ", "") \
_(ZMAPTHREAD_RETURNCODE_SERVERDIED, , "server_died",    "Server has died. ", "") \
_(ZMAPTHREAD_RETURNCODE_QUIT,       , "server_quit",    "Server has quit. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadReturnCode, ZMAP_THREAD_RETURNCODE_LIST) ;







/* One per connection to a thread, an opaque private type. */
typedef struct _ZMapThreadStruct *ZMapThread ;


/* Function to handle requests received by the thread.
 * slave_data will be passed into the handler function each time it is called to provide a
 *            mechanism for the slave to get at its own state data.
 * request is the request from the controlling thread
 * replay  is the reply from the slave code. */
typedef ZMapThreadReturnCode (*ZMapThreadRequestHandlerFunc)(void **slave_data,
							     void *request, void **reply,
							     char **err_msg_out) ;

/* Function that the thread code will call when the thread gets terminated abnormally by an
 * error or thread cancel, this gives the slave code the chance to close down properly. */
typedef ZMapThreadReturnCode (*ZMapThreadTerminateHandler)(void **slave_data, char **err_msg_out) ;

/* Function that the thread code will call when the thread gets terminated abnormally by an
 * error or thread cancel, this gives the slave code the chance to clean up, the slave code
 * should set slave_data to null on returning. */
typedef ZMapThreadReturnCode (*ZMapThreadDestroyHandler)(void **slave_data) ;


void zMapThreadForkLock(void);
void zMapThreadForkUnlock(void);


ZMapThread zMapThreadCreate(ZMapThreadRequestHandlerFunc handler_func,
			    ZMapThreadTerminateHandler terminate_func, ZMapThreadDestroyHandler destroy_func) ;
void zMapThreadRequest(ZMapThread thread, void *request) ;
gboolean zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state) ;
void zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state) ;
gboolean zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state,
				    void **data, char **err_msg) ;
char *zMapThreadGetThreadID(ZMapThread thread) ;


char *zMapThreadGetRequestString(ZMapThreadRequest signalled_state) ;
char *zMapThreadGetReplyString(ZMapThreadReply signalled_state) ;

ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadRequest2ExactStr, ZMapThreadRequest) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReply2ExactStr, ZMapThreadReply) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReturnCode2ExactStr, ZMapThreadReturnCode) ;




void zMapThreadKill(ZMapThread thread) ;
gboolean zMapThreadExists(ZMapThread thread);
void zMapThreadDestroy(ZMapThread thread) ;

#endif /* !ZMAP_THREAD_H */
