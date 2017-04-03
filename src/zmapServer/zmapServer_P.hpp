/*  File: zmapServer_P.h
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
 * Description: Private header for generalised server layer code that
 *              interfaces to the acedb/file etc. sources.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_P_H
#define ZMAP_SERVER_P_H

#include <zmapServer.hpp>
#include <zmapServerPrototype.hpp>
#include <ZMap/zmapServerProtocol.hpp>



// The state of the data source connection, requests can only be made to "opened" connections.
//
#define ZMAP_SERVER_CONNECT_STATE_LIST(_)                         \
  _(ZMAP_SERVERCONNECT_STATE_INVALID, , "invalid", "", "")				\
    _(ZMAP_SERVERCONNECT_STATE_CREATED, , "created", "created", "Server connection created but not opened.") \
    _(ZMAP_SERVERCONNECT_STATE_OPENED, , "opened", "opened", "Server connection opened and ready.") \
    _(ZMAP_SERVERCONNECT_STATE_CLOSED, , "closed", "closed", "Server connection closed.")

ZMAP_DEFINE_ENUM(ZMapServerConnectStateType, ZMAP_SERVER_CONNECT_STATE_LIST) ;


/* A connection to a data source. */
typedef struct _ZMapServerStruct
{
  ZMapServerConnectStateType state ;

  char *config_file ;

  ZMapURL url ;                 /* Replace the host & protocol ... */

  ZMapServerFuncs funcs ;       /* implementation specific functions
                                   to make the server do the right
                                   thing for it's protocol */
  void *server_conn ;         /* opaque type used for server calls. */

  ZMapServerResponseType last_response ; /* For errors returned by connection. */

  char *last_error_msg ;

} ZMapServerStruct ;


/* A context for sequence operations. */
typedef struct _ZMapServerContextStruct
{
  ZMapServer server ;
  void *server_conn_context ; /* opaque type used for server calls. */

} ZMapServerContextStruct ;


/* Hard coded for now...sigh....would like to be more dynamic really.... */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs) ;
void fileGetServerFuncs(ZMapServerFuncs file_funcs) ;
void pipeGetServerFuncs(ZMapServerFuncs pipe_funcs) ;
void ensemblGetServerFuncs(ZMapServerFuncs ensembl_funcs) ;

ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapServerConnectState2ExactStr, ZMapServerConnectStateType) ;



#endif /* !ZMAP_SERVER_P_H */
