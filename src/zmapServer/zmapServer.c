/*  File: zmapServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See ZMap/zmapServer.h
 * HISTORY:
 * Last edited: Sep 29 13:09 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.13 2004-09-29 12:37:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapProtocol.h>
#include <zmapServer_P.h>


/* We need matching serverInit and serverCleanup functions that are only called once
 * for each server type, libcurl needs this to avoid memory leaks but maybe this is not
 * such an issue for us as its once per program run...... */


/* This routine must be called before any other server routine and must only be called once,
 * it is the callers responsibility to make sure this is true....
 * NOTE the result is always TRUE at the moment because if we fail on any of these we crash... */
gboolean zMapServerGlobalInit(char *protocol, void **server_global_data_out)
{
  gboolean result = TRUE ;
  ZMapServerFuncs serverfuncs ;

  serverfuncs = g_new0(ZMapServerFuncsStruct, 1) ;	    /* n.b. crashes on failure. */

  /* Set up the server according to the protocol, this is all a bit hard coded but it
   * will do for now.... */
  /* Probably I should do this with a table of protocol and function stuff...perhaps
   * even using dynamically constructed function names....  */
  if (strcasecmp(protocol, "acedb") == 0)
    {
      acedbGetServerFuncs(serverfuncs) ;
    }
  else if (strcasecmp(protocol, "das") == 0)
    {
      dasGetServerFuncs(serverfuncs) ;
    }
  else if (strcasecmp(protocol, "file") == 0)
    {
      fileGetServerFuncs(serverfuncs) ;
    }
  else
    {
      /* Fatal coding error, we exit here..... */
      zMapLogFatal("Unsupported server protocol: %s", protocol) ;
    }

  /* All functions MUST be specified. */
  zMapAssert(serverfuncs->global_init
	     && serverfuncs->create && serverfuncs->open
	     && serverfuncs->set_context && serverfuncs->request && serverfuncs->errmsg
	     && serverfuncs->close && serverfuncs->destroy) ;
  
  /* Call the global init function. */
  if (result)
    {
      result = (serverfuncs->global_init)() ;
    }

  if (result)
    *server_global_data_out = (void *)serverfuncs ;

  return result ;
}



gboolean zMapServerCreateConnection(ZMapServer *server_out, void *global_data,
				    char *host, int port, char *protocol, int timeout,
				    char *userid, char *passwd)
{
  gboolean result = TRUE ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;


  server = g_new0(ZMapServerStruct, 1) ;		    /* n.b. crashes on failure. */
  *server_out = server ;

  /* set function table. */
  server->funcs = serverfuncs ;

  if (result)
    {
      if ((server->funcs->create)(&(server->server_conn), host, port, userid, passwd, timeout))
	{
	  server->host = g_strdup(host) ;
	  server->port = port ;
	  server->protocol = g_strdup(protocol) ;
	  server->last_response = ZMAP_SERVERRESPONSE_OK ;
	  server->last_error_msg = NULL ;

	  result = TRUE ;
	}
      else
	{
	  server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;
	  result = FALSE ;
	}
    }

  return result ;
}


ZMapServerResponseType zMapServerOpenConnection(ZMapServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->open)(server->server_conn) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;

  return result ;
}



ZMapServerResponseType zMapServerSetContext(ZMapServer server, char *sequence, int start, int end)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      result = server->last_response = (server->funcs->set_context)(server->server_conn,
								    sequence, start, end) ;
      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;
    }

  return result ;
}



ZMapServerResponseType zMapServerRequest(ZMapServer server, ZMapProtocolAny request)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      /* OK, THIS IS WHERE WE NEED TO DECODE THE REQUESTS...... */
      ZMapProtocolAny req_any = (ZMapProtocolAny)request ;

      switch (req_any->request)
	{
	case ZMAP_PROTOCOLREQUEST_SEQUENCE:
	  {
	    ZMapProtocoltGetFeatures get_features = (ZMapProtocoltGetFeatures)req_any ;


	    result = server->last_response
	      = (server->funcs->request)(server->server_conn, &(get_features->feature_context_out)) ;

	    break ;
	  }
	default:
	  {	  
	    zMapLogFatal("Unknown request type: %d", req_any->request) ; /* Exit appl. */
	    break ;
	  }
	}




      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;
    }

  return result ;
}




ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) 
{
  gboolean result = FALSE ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      if (!(result = (server->funcs->close)(server->server_conn)))
	server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;
    }

  return result ;
}

gboolean zMapServerFreeConnection(ZMapServer server)
{
  gboolean result = TRUE ;

  (server->funcs->destroy)(server->server_conn) ;
  g_free(server->host) ;
  g_free(server->protocol) ;
  g_free(server) ;

  return result ;
}


char *zMapServerLastErrorMsg(ZMapServer server)
{
  return server->last_error_msg ;
}
