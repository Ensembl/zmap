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
 * Last edited: Jul 26 18:10 2005 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.24 2005-07-27 12:23:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapUtils.h>
#include <zmapServer_P.h>


/* We need matching serverInit and serverCleanup functions that are only called once
 * for each server type, libcurl needs this to avoid memory leaks but maybe this is not
 * such an issue for us as its once per program run...... */


/* This routine must be called before any other server routine and must only be called once,
 * it is the callers responsibility to make sure this is true....
 * NOTE the result is always TRUE at the moment because if we fail on any of these we crash... */
gboolean zMapServerGlobalInit(struct url *url, void **server_global_data_out)
{
  gboolean result = TRUE ;
  ZMapServerFuncs serverfuncs ;
  enum url_scheme protocol = url->scheme ;

  serverfuncs = g_new0(ZMapServerFuncsStruct, 1) ;	    /* n.b. crashes on failure. */

  /* Set up the server according to the protocol, this is all a bit hard coded but it
   * will do for now.... */
  /* Probably I should do this with a table of protocol and function stuff...perhaps
   * even using dynamically constructed function names....  */

  if (protocol == SCHEME_ACEDB)
    {
      acedbGetServerFuncs(serverfuncs) ;
    }
  else if (protocol == SCHEME_HTTP) /* Force http to BE das at the moment, but later I think we should have FORMAT too */
    {                               /* Not that Format gets passed in here though!!! we'd need to pass the url struct */
      //      if(strcasecmp(format, 'das') == 0)
      // {
          dasGetServerFuncs(serverfuncs) ;
          //  }
    }
  else if (protocol == SCHEME_FILE)
    {
      fileGetServerFuncs(serverfuncs) ;
    }
  else
    {
      /* Fatal coding error, we exit here..... Nothing more can happen
         without setting up serverfuncs! */
      /* Getting here means somethings been added to ZMap/zmapUrl.h
         and not to the above protocol decision above. */
      zMapLogFatal("Unsupported server protocol: %s", protocol) ;
    }

  /* All functions MUST be specified. */
  zMapAssert(serverfuncs->global_init
	     && serverfuncs->create && serverfuncs->open
	     && serverfuncs->set_context
	     && serverfuncs->get_features && serverfuncs->get_sequence
	     && serverfuncs->errmsg
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
				    struct url *url, char *format,
				    int timeout, char *version_str)
{
  gboolean result = TRUE ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;
  char *host = url->host;
  int port = url->port;
  enum url_scheme protocol = url->scheme ;

  char *userid = url->user;
  char *passwd = url->passwd;


  zMapAssert(server_out && global_data && url) ;

  /* For some protocols we finesse the host as a filename or whatever... */
  if (protocol == SCHEME_FILE)
    host = url->path ;

  server = g_new0(ZMapServerStruct, 1) ;		    /* n.b. crashes on failure. */
  *server_out = server ;

  /* set function table. */
  server->funcs = serverfuncs ;

  if (result)
    {
      if ((server->funcs->create)(&(server->server_conn), host, port, format, version_str,
				  userid, passwd, timeout))
	{
	  server->host = g_strdup(host) ;
	  server->protocol = protocol ;
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

ZMapServerResponseType zMapServerGetTypes(ZMapServer server, GList *requested_types, GList **types_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->get_types)(server->server_conn,
							      requested_types, types_out) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}



ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  zMapAssert(feature_context) ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      result = server->last_response
	= (server->funcs->set_context)(server->server_conn, feature_context) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}


ZMapFeatureContext zMapServerCopyContext(ZMapServer server)
{
  ZMapFeatureContext feature_context ;

  feature_context = (server->funcs->copy_context)(server->server_conn) ;

  return feature_context ;
}


ZMapServerResponseType zMapServerGetFeatures(ZMapServer server, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {

      result = server->last_response
	= (server->funcs->get_features)(server->server_conn, feature_context) ;


      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->protocol, server->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}


ZMapServerResponseType zMapServerGetSequence(ZMapServer server, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      result = server->last_response
	= (server->funcs->get_sequence)(server->server_conn, feature_context) ;

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
  g_free(server) ;

  return result ;
}


char *zMapServerLastErrorMsg(ZMapServer server)
{
  return server->last_error_msg ;
}
