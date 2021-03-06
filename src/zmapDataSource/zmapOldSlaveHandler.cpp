/*  File: zmapServerProtocolHandler.c
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
 * Description:
 * Exported functions: See ZMap/zmapServerProtocol.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapServerProtocol.hpp>

#include <../zmapServer/zmapServer.hpp>



static ZMapThreadReturnCode threadRequestHandler(void **slave_data, void *request_in, char **err_msg_out) ;
static ZMapThreadReturnCode threadTerminateHandler(void **slave_data, char **err_msg_out) ;
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data) ;

static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static ZMapThreadReturnCode destroyServer(ZMapServer *server) ;
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code) ;




/*
 *                           External Interface
 */
bool zMapOldGetHandlers(ZMapSlaveRequestHandlerFunc *handler_func_out,
                        ZMapSlaveTerminateHandlerFunc *terminate_func_out,
                        ZMapSlaveDestroyHandlerFunc *destroy_func_out)
{
  bool result = false ;

  
  *handler_func_out = threadRequestHandler ;
  *terminate_func_out = threadTerminateHandler ;
  *destroy_func_out = threadDestroyHandler ;


  return result ;
}




/*
 *                     Internal routines
 */



/* This routine sits in the slave thread and is called by the threading interface to
 * service a request from the master thread which includes decoding it, calling the appropriate server
 * routines and returning the answer to the master thread. */
static ZMapThreadReturnCode threadRequestHandler(void **slave_data, void *request_in, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqAny request = (ZMapServerReqAny)request_in ;
  ZMapServerResponseType reply ;
  ZMapServer server = NULL ;

  /* slave_data is NULL first time we are called. */
  if (*slave_data)
    server = (ZMapServer)*slave_data ;

  reply = zMapServerRequest(&server, request, err_msg_out) ;

  // Slightly hacky here unfortunately, but there's no clean 1:1 relationship in the
  // reply:threadrc relationship.
  if (request->type == ZMAP_SERVERREQ_TERMINATE)
    {
      if (reply == ZMAP_SERVERRESPONSE_OK)
        thread_rc = ZMAPTHREAD_RETURNCODE_QUIT ;
      else
        thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else
    {
      thread_rc = thread_RC(reply) ;
    }

  /* Return server (only really necessary on create call..... */
  *slave_data = (void *)server ;


  return thread_rc ;
}





/* This function is called if a thread terminates.  see slaveThread.html */
static ZMapThreadReturnCode threadTerminateHandler(void **slave_data, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapReturnValIfFail((slave_data && err_msg_out), ZMAPTHREAD_RETURNCODE_BADREQ) ;

  server = (ZMapServer)*slave_data ;

  {
    // debug info.....
    ZMapServerReqGetServerInfo req_info ;

    req_info = (ZMapServerReqGetServerInfo)zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;

    if (zMapServerGetServerInfo(server, req_info) == ZMAP_SERVERRESPONSE_OK)
      {
        zMapLogWarning("Terminate function called for: %s%s%s%s%s%s%s",
                       (req_info->data_format_out ? req_info->data_format_out : ""),
                       (req_info->database_name_out ? ", " : ""),
                       (req_info->database_name_out ? req_info->database_name_out : ""),
                       (req_info->database_title_out ? ", " : ""),
                       (req_info->database_title_out ? req_info->database_title_out : ""),
                       (req_info->database_path_out  ?  ", " : ""),
                       (req_info->database_path_out  ? req_info->database_path_out : "")) ;
      }

    zMapServerRequestDestroy((ZMapServerReqAny)req_info) ;
  }

  thread_rc = terminateServer(&server, err_msg_out) ;
    
  *slave_data = server ;

  return thread_rc ;
}


/* This function is called if a thread terminates.  see slaveThread.html */
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapReturnValIfFail((slave_data), ZMAPTHREAD_RETURNCODE_BADREQ) ;

  server = (ZMapServer)*slave_data ;

  {
    // debug info.....
    ZMapServerReqGetServerInfo req_info ;

    req_info = (ZMapServerReqGetServerInfo)zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;

    if (zMapServerGetServerInfo(server, req_info) == ZMAP_SERVERRESPONSE_OK)
      {
        zMapLogWarning("Terminate function called for: %s%s%s%s%s%s%s",
                       (req_info->data_format_out ? req_info->data_format_out : ""),
                       (req_info->database_name_out ? ", " : ""),
                       (req_info->database_name_out ? req_info->database_name_out : ""),
                       (req_info->database_title_out ? ", " : ""),
                       (req_info->database_title_out ? req_info->database_title_out : ""),
                       (req_info->database_path_out  ?  ", " : ""),
                       (req_info->database_path_out  ? req_info->database_path_out : "")) ;
      }

    zMapServerRequestDestroy((ZMapServerReqAny)req_info) ;
  }

  thread_rc = destroyServer(&server) ;

  return thread_rc ;
}




static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqTerminateStruct term_req = {ZMAP_SERVERREQ_TERMINATE, ZMAP_SERVERRESPONSE_INVALID} ;
  ZMapServerResponseType reply ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapServerConnectStateType connect_state = ZMAP_SERVERCONNECT_STATE_INVALID ;

  if ((zMapServerGetConnectState(*server, &connect_state) == ZMAP_SERVERRESPONSE_OK)
      && connect_state == ZMAP_SERVERCONNECT_STATE_CONNECTED
      && (zMapServerCloseConnection(*server) == ZMAP_SERVERRESPONSE_OK))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    if ((reply = zMapServerRequest(server, (ZMapServerReqAny)&term_req, err_msg_out)) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}



static ZMapThreadReturnCode destroyServer(ZMapServer *server)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqTerminateStruct term_req = {ZMAP_SERVERREQ_TERMINATE, ZMAP_SERVERRESPONSE_INVALID} ;
  ZMapServerResponseType reply ;
  char *err_msg = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (zMapServerFreeConnection(*server) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    if ((reply = zMapServerRequest(server, (ZMapServerReqAny)&term_req, &err_msg)) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      // Log err message
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  return thread_rc ;
}


/* THIS NEEDS CLEANING UP AND WILL BE BY THE THREADS REWRITE... */
/* Return a thread return code for each type of server response. */
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code)
{
  ZMapThreadReturnCode thread_rc ;

  switch(code)
    {
    case ZMAP_SERVERRESPONSE_OK:
      thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
      break ;

    case ZMAP_SERVERRESPONSE_BADREQ:
      thread_rc = ZMAPTHREAD_RETURNCODE_BADREQ ;
      break ;

    /*
     * Modified version; next step is to remove NODATA
     */
    case ZMAP_SERVERRESPONSE_REQFAIL:
    case ZMAP_SERVERRESPONSE_UNSUPPORTED:
    case ZMAP_SERVERRESPONSE_SOURCEERROR:
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
      break ;

    /*
     * Valid gff header, but no features or fasta data.
     */
    case ZMAP_SERVERRESPONSE_SOURCEEMPTY:
      thread_rc = ZMAPTHREAD_RETURNCODE_SOURCEEMPTY ;
      break ;


    case ZMAP_SERVERRESPONSE_TIMEDOUT:
    case ZMAP_SERVERRESPONSE_SERVERDIED:
      thread_rc = ZMAPTHREAD_RETURNCODE_SERVERDIED ;
      break ;

    default:
      thread_rc = ZMAPTHREAD_RETURNCODE_INVALID ;
      break ;
    }

  return thread_rc ;
}


