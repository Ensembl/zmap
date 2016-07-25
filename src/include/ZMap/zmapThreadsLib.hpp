/*  File: zmapThreads.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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
#include <ZMap/zmapThreads.hpp>



/* We should have a function to access this global.... */
/* debugging messages, TRUE for on... */
extern gboolean zmap_thread_debug_G ;


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



// bool "new_interface" is TEMP NEW/OLD INTERFACE WHILE I SET UP THE NEW THREADS STUFF....
// true means use new interface, false the old/existing interface.
ZMapThread zMapThreadCreate(bool new_interface,
                            ZMapThreadCreateFunc create_func,
                            ZMapSlaveRequestHandlerFunc req_handler_func,
                            ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                            ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;
void zMapThreadRequest(ZMapThread thread, void *request) ;
gboolean zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state) ;
void zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state) ;
gboolean zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state,
				    void **data, char **err_msg) ;
char *zMapThreadGetThreadID(ZMapThread thread) ;
char *zMapThreadGetRequestString(ZMapThreadRequest signalled_state) ;
char *zMapThreadGetReplyString(ZMapThreadReply signalled_state) ;
void zMapThreadKill(ZMapThread thread) ;
gboolean zMapThreadExists(ZMapThread thread);
void zMapThreadDestroy(ZMapThread thread) ;

ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadRequest2ExactStr, ZMapThreadRequest) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReply2ExactStr, ZMapThreadReply) ;






// THESE SHOULD NOT BE HERE.... MOVE TO ZMAPUTILS WHERE THEY BELONG...THEY ARE NOTHING TO DO WITH
// THIS THREADING INTERFACE.......
void zMapThreadForkLock(void) ;
void zMapThreadForkUnlock(void) ;



#endif /* !ZMAP_THREADSLIB_H */
