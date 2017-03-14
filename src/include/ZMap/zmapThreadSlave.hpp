/*  File: zmapThreadSlave.hpp
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
 * Description: Slave source interface, called by slave side of
 *              threading interface to accesss data slave source which
 *              turn accesses the data source sever.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREADSLAVE_H
#define ZMAP_THREADSLAVE_H

#include <config.h>

#include <ZMap/zmapEnum.hpp>


// get rid of quit, serverdied, sourceempty, timed out returncode....

// Return codes from the slave source request handler function called by the slave
// thread code to service a request.

#define ZMAP_THREAD_RETURNCODE_LIST(_)             \
_(ZMAPTHREAD_RETURNCODE_INVALID,      , "invalid",        "Invalid return code. ", "") \
_(ZMAPTHREAD_RETURNCODE_OK,           , "ok",             "OK. ", "") \
_(ZMAPTHREAD_RETURNCODE_REQFAIL,      , "request_failed", "Request failed. ", "") \
_(ZMAPTHREAD_RETURNCODE_BADREQ,       , "bad_request",    "Invalid request. ", "") \
_(ZMAPTHREAD_RETURNCODE_SOURCEEMPTY,  , "source_empty",   "Source is empty ", "") \
_(ZMAPTHREAD_RETURNCODE_TIMEDOUT,     , "timed_out",      "Timed out. ", "") \
_(ZMAPTHREAD_RETURNCODE_SERVERDIED,   , "server_died",    "Server has died. ", "") \
_(ZMAPTHREAD_RETURNCODE_QUIT,         , "server_quit",    "Server has quit. ", "")

ZMAP_DEFINE_ENUM(ZMapThreadReturnCode, ZMAP_THREAD_RETURNCODE_LIST) ;





typedef ZMapThreadReturnCode (*ZMapSlaveRequestHandlerFunc)(void **slave_data, void *request_in, char **err_msg_out) ;
typedef ZMapThreadReturnCode (*ZMapSlaveTerminateHandlerFunc)(void **slave_data, char **err_msg_out) ;
typedef ZMapThreadReturnCode (*ZMapSlaveDestroyHandlerFunc)(void **slave_data) ;



ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapThreadReturnCode2ExactStr, ZMapThreadReturnCode) ;





#endif /* !ZMAP_THREADSLAVE_H */
