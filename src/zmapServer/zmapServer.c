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
 * Last edited: Dec 13 15:15 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.17 2004-12-13 15:16:22 edgrif Exp $
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
				    char *version_str,
				    char *userid, char *passwd)
{
  gboolean result = TRUE ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;

  /* we must have as a minumum the host name and a protocol, everything else is optional. */
  zMapAssert(host && *host && protocol && *protocol) ;

  server = g_new0(ZMapServerStruct, 1) ;		    /* n.b. crashes on failure. */
  *server_out = server ;

  /* set function table. */
  server->funcs = serverfuncs ;

  if (result)
    {
      if ((server->funcs->create)(&(server->server_conn), host, port, version_str,
				  userid, passwd, timeout))
	{
	  server->host = g_strdup(host) ;
	  server->protocol = g_strdup(protocol) ;
	  server->last_response = ZMAP_SERVERRESPONSE_OK ;
	  server->last_error_msg = NULL ;

	  result = TRUE ;
	}
      else
	{
	  server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							  (server->funcs->errmsg)(server->server_conn)) ;
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
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}


ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapServerSetContext context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      if (!context->sequence || !*(context->sequence)
	  || (context->end != 0 && context->start > context->end))
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, server->protocol, server->host, "Cannot set server context:%s%s",
			 ((!context->sequence || !*(context->sequence))
			  ? " no sequence name" : ""),
			 ((context->end != 0 && context->start > context->end)
			  ? "bad coords, start > end" : "")) ;
	}
      else
	{
	  result = server->last_response = (server->funcs->set_context)(server->server_conn, context) ;

	  if (result != ZMAP_SERVERRESPONSE_OK)
	    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							    (server->funcs->errmsg)(server->server_conn)) ;
	}
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



      /* This switch is pointless at the moment but I expect to have do some preprocessing on some
       * requests at some time. */
      switch (req_any->request)
	{
	case ZMAP_PROTOCOLREQUEST_FEATURES:
	case ZMAP_PROTOCOLREQUEST_SEQUENCE:
	case ZMAP_PROTOCOLREQUEST_FEATURE_SEQUENCE:
	  {
	    ZMapProtocoltGetFeatures get_features = (ZMapProtocoltGetFeatures)req_any ;


	    result = server->last_response
	      = (server->funcs->request)(server->server_conn, get_features) ;

	    break ;
	  }
	default:
	  {	  
	    ZMAPSERVER_LOG(Fatal, server->protocol, server->host,
			   "Unknown request type: %d", req_any->request) ; /* Exit appl. */
	    break ;
	  }
	}




      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}




ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) 
{
  gboolean result = FALSE ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      if (!(result = (server->funcs->close)(server->server_conn)))
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
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
