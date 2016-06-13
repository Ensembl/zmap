/*  File: zmapServerProtocolHandler.c
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See ZMap/zmapServerProtocol.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>


#include <ZMap/zmapServerProtocol.hpp>
#include <zmapServer_P.hpp>
#include <ZMap/zmapConfigIni.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapThreads.hpp>
#include <ZMap/zmapSlaveHandler.hpp>
#include <zmapServer.hpp>
#include <zmapServerRequestHandler.hpp>


static ZMapThreadReturnCode threadRequestHandler(void **slave_data, void *request_in, char **err_msg_out) ;
static ZMapThreadReturnCode threadTerminateHandler(void **slave_data, char **err_msg_out) ;
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data) ;

static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static ZMapThreadReturnCode destroyServer(ZMapServer *server) ;
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code) ;




/*
 *                           External Interface
 */
bool zMapSlaveHandlerInitOld(ZMapSlaveRequestHandlerFunc *handler_func_out,
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

  thread_rc = terminateServer(&server, err_msg_out) ;

  return thread_rc ;
}


/* This function is called if a thread terminates.  see slaveThread.html */
static ZMapThreadReturnCode threadDestroyHandler(void **slave_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapReturnValIfFail((slave_data), ZMAPTHREAD_RETURNCODE_BADREQ) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = destroyServer(&server) ;

  return thread_rc ;
}




static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerConnectStateType connect_state = ZMAP_SERVERCONNECT_STATE_INVALID ;

  if ((zMapServerGetConnectState(*server, &connect_state) == ZMAP_SERVERRESPONSE_OK)
      && connect_state == ZMAP_SERVERCONNECT_STATE_CONNECTED
      && (zMapServerCloseConnection(*server) == ZMAP_SERVERRESPONSE_OK))
    {
      *server = NULL ;
    }
  else if (connect_state == ZMAP_SERVERCONNECT_STATE_UNCONNECTED)
    {
      *server = NULL ;
    }
  else
    {
      *err_msg_out = g_strdup(zMapServerLastErrorMsg(*server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}



static ZMapThreadReturnCode destroyServer(ZMapServer *server)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (zMapServerFreeConnection(*server) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
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


