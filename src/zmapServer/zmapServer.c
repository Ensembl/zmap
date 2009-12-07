/*  File: zmapServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: 2009-11-26 12:57:05 (mgh)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.41 2009-12-07 12:53:42 mh17 Exp $
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
gboolean zMapServerGlobalInit(ZMapURL url, void **server_global_data_out)
{
  gboolean result = TRUE ;
  ZMapServerFuncs serverfuncs ;

  serverfuncs = g_new0(ZMapServerFuncsStruct, 1) ;	    /* n.b. crashes on failure. */

  /* Set up the server according to the protocol, this is all a bit hard coded but it
   * will do for now.... */
  /* Probably I should do this with a table of protocol and function stuff...perhaps
   * even using dynamically constructed function names....  */

  switch(url->scheme){
  case SCHEME_ACEDB:
    acedbGetServerFuncs(serverfuncs) ;
    break;
  case SCHEME_HTTP:
    /*  case SCHEME_HTTPS: */
    /* Force http[s] to BE das at the moment, but later I think we should have FORMAT too */
    /* Not that Format gets passed in here though!!! we'd need to pass the url struct */
    /* if(strcasecmp(format, 'das') == 0) */
    dasGetServerFuncs(serverfuncs);
    break;
  case SCHEME_FILE:     // DAS only: file gets handled by pipe
    if(url->params)
    {
      dasGetServerFuncs(serverfuncs);
      break;
    }
    // fall through for real files
  case SCHEME_PIPE:
    pipeGetServerFuncs(serverfuncs);
    break;
  default:
    /* Fatal coding error, we exit here..... Nothing more can happen
       without setting up serverfuncs! */
    /* Getting here means somethings been added to ZMap/zmapUrl.h
       and not to the above protocol decision above. */
    zMapLogFatal("Unsupported server protocol: %s", url->protocol) ;
    break;
  }

  /* All functions MUST be specified. */
  zMapAssert(serverfuncs->global_init
	     && serverfuncs->create
	     && serverfuncs->open
	     && serverfuncs->get_info
	     && serverfuncs->feature_set_names
	     && serverfuncs->get_styles
	     && serverfuncs->have_modes
	     && serverfuncs->get_sequence
	     && serverfuncs->set_context
	     && serverfuncs->get_features
	     && serverfuncs->get_context_sequences
	     && serverfuncs->errmsg
	     && serverfuncs->close
	     && serverfuncs->destroy) ;
  
  /* Call the global init function. */
  if (result)
    {
      result = (serverfuncs->global_init)() ;
    }

  if (result)
    *server_global_data_out = (void *)serverfuncs ;

  return result ;
}



ZMapServerResponseType zMapServerCreateConnection(ZMapServer *server_out, void *global_data,
						  ZMapURL url, char *format,
						  int timeout, char *version_str)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;
  int parse_error;
  zMapAssert(server_out && global_data && url) ;

  server      = g_new0(ZMapServerStruct, 1) ; /* n.b. crashes on failure. */
  *server_out = server ;
  /* set function table. */
  server->funcs = serverfuncs ;

  /* COPY/REPARSE the url into server... with paranoia as it managed to parse 1st time! */
  if ((server->url = url_parse(url->url, &parse_error)) == NULL)
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(url->protocol, 
                                                      url->host, "%s",
                                                      url_error(parse_error)) ;
    }

  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      if ((server->funcs->create)(&(server->server_conn), url, format, 
                                  version_str, timeout))
	{
	  server->last_response  = ZMAP_SERVERRESPONSE_OK ;
	  server->last_error_msg = NULL ;
	  result = ZMAP_SERVERRESPONSE_OK ;
	}
      else
	{
	  server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                          server->url->host, "%s",
							  (server->funcs->errmsg)(server->server_conn)) ;
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
    }

  return result ;
}


ZMapServerResponseType zMapServerOpenConnection(ZMapServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->open)(server->server_conn) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}



ZMapServerResponseType zMapServerFeatureSetNames(ZMapServer server,
						 GList **feature_sets_inout,
						 GList *sources,
						 GList **required_styles_out,
						 GHashTable **featureset_2_stylelist_out,
						 GHashTable **source_2_featureset_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  zMapAssert(server && *feature_sets_inout && !*required_styles_out) ;

  result = server->last_response
    = (server->funcs->feature_set_names)(server->server_conn,
					 feature_sets_inout,
					 sources,
					 required_styles_out,
					 featureset_2_stylelist_out,
					 source_2_featureset_out) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}


ZMapServerResponseType zMapServerGetStyles(ZMapServer server, GData **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->get_styles)(server->server_conn, styles_out) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}


ZMapServerResponseType zMapServerStylesHaveMode(ZMapServer server, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->have_modes)(server->server_conn, have_mode) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}



ZMapServerResponseType zMapServerGetSequence(ZMapServer server, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  result = server->last_response = (server->funcs->get_sequence)(server->server_conn, sequences_inout) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
						    (server->funcs->errmsg)(server->server_conn)) ;

  return result ;
}


ZMapServerResponseType zMapServerGetServerInfo(ZMapServer server, ZMapServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;


  result = server->last_response = (server->funcs->get_info)(server->server_conn, info) ;

  if (result != ZMAP_SERVERRESPONSE_OK)
    server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                    server->url->host, "%s",
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
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                        server->url->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}


ZMapServerResponseType zMapServerGetFeatures(ZMapServer server, GData *styles, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {

      result = server->last_response
	= (server->funcs->get_features)(server->server_conn, styles, feature_context) ;


      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                        server->url->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}


ZMapServerResponseType zMapServerGetContextSequences(ZMapServer server, GData *styles,
						     ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      result = server->last_response
	= (server->funcs->get_context_sequences)(server->server_conn, styles, feature_context) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                        server->url->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}



ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) 
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  if (server->last_response != ZMAP_SERVERRESPONSE_SERVERDIED)
    {
      result = server->last_response
	= (server->funcs->close)(server->server_conn) ;

      if (result != ZMAP_SERVERRESPONSE_OK)
	server->last_error_msg = ZMAPSERVER_MAKEMESSAGE(server->url->protocol, 
                                                        server->url->host, "%s",
							(server->funcs->errmsg)(server->server_conn)) ;
    }

  return result ;
}

ZMapServerResponseType zMapServerFreeConnection(ZMapServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  /* This function is a bit different, as we free the server struct the only thing we can do
   * is return the status from the destroy call. */
  result = (server->funcs->destroy)(server->server_conn) ;
  /* Free ZMapURL!!!! url_free(server->url)*/
  g_free(server) ;

  return result ;
}


char *zMapServerLastErrorMsg(ZMapServer server)
{
  return server->last_error_msg ;
}
