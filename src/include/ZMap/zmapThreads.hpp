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
 * Description: Interface to running data source threads.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_H
#define ZMAP_THREAD_H

#include <config.h>

#include <glib.h>

#include <ZMap/zmapEnum.hpp>



/* The thread, one per connection, an opaque private type. */
typedef struct _ZMapThreadStruct *ZMapThread ;



/* Return codes from the handler function called by the slave thread to service a request. */
#define ZMAP_THREAD_RETURNCODE_LIST(_)             \
_(ZMAPTHREAD_RETURNCODE_INVALID,      , "invalid",        "Invalid return code. ", "") \
_(ZMAPTHREAD_RETURNCODE_OK,           , "ok",             "OK. ", "") \
_(ZMAPTHREAD_RETURNCODE_TIMEDOUT,     , "timed_out",      "Timed out. ", "") \
_(ZMAPTHREAD_RETURNCODE_REQFAIL,      , "request_failed", "Request failed. ", "") \
_(ZMAPTHREAD_RETURNCODE_SOURCEEMPTY,  , "source_empty",   "Source is empty ", "") \
_(ZMAPTHREAD_RETURNCODE_BADREQ,       , "bad_request",    "Invalid request. ", "") \
_(ZMAPTHREAD_RETURNCODE_SERVERDIED,   , "server_died",    "Server has died. ", "") \
_(ZMAPTHREAD_RETURNCODE_QUIT,         , "server_quit",    "Server has quit. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadReturnCode, ZMAP_THREAD_RETURNCODE_LIST) ;



typedef void *(*ZMapThreadCreateFunc)(void *func_data) ;
typedef ZMapThreadReturnCode (*ZMapSlaveRequestHandlerFunc)(void **slave_data, void *request_in, char **err_msg_out) ;
typedef ZMapThreadReturnCode (*ZMapSlaveTerminateHandlerFunc)(void **slave_data, char **err_msg_out) ;
typedef ZMapThreadReturnCode (*ZMapSlaveDestroyHandlerFunc)(void **slave_data) ;


// Opaque pointer to thread handler data needed in thread polling functions.
//
typedef struct ZMapThreadPollSlaveCallbackDataStructType *ZMapThreadPollSlaveCallbackData ;


// User function to call when slave has replied.
typedef void (*ZMapThreadPollSlaveUserReplyFunc)(void *func_data) ;



ZMapThread zMapThreadStart(bool new_interface,
                           ZMapSlaveRequestHandlerFunc req_handler_func,
                           ZMapSlaveTerminateHandlerFunc terminate_handler_func,
                           ZMapSlaveDestroyHandlerFunc destroy_handler_func) ;
ZMapThreadPollSlaveCallbackData zMapThreadPollSlaveStart(ZMapThread thread,
                                                         ZMapThreadPollSlaveUserReplyFunc user_reply_func,
                                                         void *user_reply_func_data) ;
void zMapThreadPollSlaveStop(ZMapThreadPollSlaveCallbackData poll_cb_data) ;



ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReturnCode2ExactStr, ZMapThreadReturnCode) ;



#endif /* !ZMAP_THREAD_H */
