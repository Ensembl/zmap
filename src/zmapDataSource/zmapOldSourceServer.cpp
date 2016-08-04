/*  File: zmapDataSourceServer.cpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: Work in Progress, contains server related stuff that
 *              has been hacked out of zmapView.c
 *
 * Exported functions: See zmapViewServer.hpp
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapThreads.hpp>

#include <zmapOldSourceServer_P.hpp>


using namespace ZMapThreadSource ;



/*
 * Session data, each connection has various bits of information recorded about it
 * depending on what type of connection it is (http, acedb, pipe etc).
 */

typedef struct ZMapViewSessionServerStructType
{
  ZMapURLScheme scheme ;
  char *url ;
  char *protocol ;
  char *format ;

  union							    /* Keyed by scheme field above. */
  {
    struct
    {
      char *host ;
      int port ;
      char *database ;
    } acedb ;

    struct
    {
      char *path ;
    } file ;

    struct
    {
      char *path ;
      char *query ;
    } pipe ;

    struct
    {
      char *host ;
      int port ;
    } ensembl ;
  } scheme_data ;

} ZMapViewSessionServerStruct ;


static void freeServer(ZMapViewSessionServer server_data) ;



/*
 *                  External interface functions.
 */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapServerSetScheme(ZMapViewSessionServer server, ZMapURLScheme scheme)
{
  server->scheme = scheme ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Produce information for each session as formatted text.
 *
 * NOTE: some information is only available once the server connection
 * is established and the server can be queried for it. This is not formalised
 * in a struct but could be if found necessary.
 *
 *  */
void zMapServerFormatSession(ZMapViewSessionServer server_data, GString *session_text)
{
  const char *unavailable_txt = "<< not available yet >>" ;


  g_string_append(session_text, "Server\n") ;

  g_string_append_printf(session_text, "\tURL: %s\n\n", server_data->url) ;
  g_string_append_printf(session_text, "\tProtocol: %s\n\n", server_data->protocol) ;
  g_string_append_printf(session_text, "\tFormat: %s\n\n",
			 (server_data->format ? server_data->format : unavailable_txt)) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	g_string_append_printf(session_text, "\tServer: %s\n\n", server_data->scheme_data.acedb.host) ;
	g_string_append_printf(session_text, "\tPort: %d\n\n", server_data->scheme_data.acedb.port) ;
	g_string_append_printf(session_text, "\tDatabase: %s\n\n",
			       (server_data->scheme_data.acedb.database
				? server_data->scheme_data.acedb.database : unavailable_txt)) ;
	break ;
      }
    case SCHEME_FILE:
      {
	g_string_append_printf(session_text, "\tFile: %s\n\n", server_data->scheme_data.file.path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	g_string_append_printf(session_text, "\tScript: %s\n\n", server_data->scheme_data.pipe.path) ;
	g_string_append_printf(session_text, "\tQuery: %s\n\n", server_data->scheme_data.pipe.query) ;
	break ;
      }
    case SCHEME_ENSEMBL:
      {
        g_string_append_printf(session_text, "\tServer: %s\n\n", server_data->scheme_data.ensembl.host) ;
        g_string_append_printf(session_text, "\tPort: %d\n\n", server_data->scheme_data.ensembl.port) ;
        break ;
      }
    default:
      {
	g_string_append(session_text, "\tUnsupported server type !") ;
	break ;
      }
    }

  return ;
}




/*#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
aggghhhhhhhhhhhhhhhhhhH MALCOLM.......YOUVE CHANGED IT ALL BUT NOT CHANGED
THE COMMENTS ETC ETC....HORRIBLE, HORRIBLE, HORRIBLE......

  THIS FUNCTION NOW NEEDS TO BE SPLIT INTO TWO FUNCTIONS, ONE TO DO THE CURRENT
  FUNCTIONS PURPOSE: used the passed in view or create a new one if needed

  AND THE ORIGINAL "create" function.....
#endif*/

/* Allocate a connection and send over the request to get the sequence displayed. */
/* NB: this is called from zmapViewLoadFeatures() and commandCB (for DNA only) */
ZMapNewDataSource zMapServerCreateViewConnection(ZMapNewDataSource view_con,
                                                 void *connect_data,
                                                 char *server_url)
{
  if (!view_con)
    {
      /* Create the thread to service the connection requests, we give it a function that it will call
       * to decode the requests we send it and a terminate function. */
      ZMapSlaveRequestHandlerFunc req_handler_func = NULL ;
      ZMapSlaveTerminateHandlerFunc terminate_handler_func = NULL ;
      ZMapSlaveDestroyHandlerFunc destroy_handler_func = NULL ;
      ThreadSource *new_thread = NULL ;

      // Create and start a new thread to fetch the data.
      zMapOldGetHandlers(&req_handler_func, &terminate_handler_func, &destroy_handler_func) ;

      new_thread = new ThreadSource(false, req_handler_func, terminate_handler_func, destroy_handler_func) ;

      if (!(new_thread->SlaveStartPoll()))
        {
          delete new_thread ;
        }
      else
        {
          // Create a connection representing the new thread/server connection.
          view_con = g_new0(ZMapNewDataSourceStruct, 1) ;

          view_con->thread = new_thread ;

          view_con->thread_status = THREAD_STATUS_PENDING;

          view_con->server = g_new0(ZMapViewSessionServerStruct, 1) ;

          view_con->server->scheme = SCHEME_INVALID ;

          // Hack for now.....to replace url....
          view_con->server->url = g_strdup(server_url) ;
        }
    }




  // ERROR HANDLING ALL OVER THE PLACE....
  if (!view_con)
    {
      zMapLogWarning("GUI: url %s looks ok (%s)"
                     "but could not connect to server.", server_url) ;
    }


  // What happened to previous connect data....duh.......
  if (view_con)
    {
      view_con->request_data = connect_data ;
    }


  return view_con ;
}



void *zMapServerConnectionGetUserData(ZMapNewDataSource view_conn)
{
  return view_conn->request_data ;
}


const char *zMapServerGetUrl(ZMapNewDataSource view_conn)
{
  const char *url = view_conn->server->url ;

  return url ;
}


/* Final destroy of a connection, by the time we get here the thread has
 * already been destroyed. */
void zMapServerDestroyViewConnection(ZMapNewDataSource view_conn)
{
  /* Need to destroy the types array here.......errrrr...so why not do it ???? */

  freeServer(view_conn->server) ;

  g_free(view_conn->server) ;

  g_free(view_conn) ;

  return ;
}















/*
 *            Package functions
 */


/*
 * Server Session stuff: holds information about data server connections.
 *
 * NOTE: the list of connection session data is dynamic, as a server
 * connection is terminated the session data is free'd too so the final
 * list may be very short or not even have any connections at all.
 */

/* Record initial details for a session, these are known when the session is created. */
void zmapViewSessionAddServer(ZMapViewSessionServer server_data, ZMapURL url, char *format)
{
  server_data->scheme = url->scheme ;
  server_data->url = g_strdup(url->url) ;
  server_data->protocol = g_strdup(url->protocol) ;

  if (format)
    server_data->format = g_strdup(format) ;

  switch(url->scheme)
    {
    case SCHEME_ACEDB:
      {
	server_data->scheme_data.acedb.host = g_strdup(url->host) ;
	server_data->scheme_data.acedb.port = url->port ;
	server_data->scheme_data.acedb.database = g_strdup(url->path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	server_data->scheme_data.pipe.path = g_strdup(url->path) ;
	server_data->scheme_data.pipe.query = g_strdup(url->query) ;
	break ;
      }

    case SCHEME_FILE:
      {
	// mgh: file:// is now handled by pipe://, but as this is a view struct it is unchanged
	// consider also DAS, which is still known as a file://
	server_data->scheme_data.file.path = g_strdup(url->path) ;
	break ;
      }
    case SCHEME_ENSEMBL:
      {
        server_data->scheme_data.ensembl.host = g_strdup(url->host) ;
        server_data->scheme_data.ensembl.port = url->port ;
        break ;
      }
    default:
      {
	/* other schemes not currently supported so mark as invalid. */
	server_data->scheme = SCHEME_INVALID ;
	break ;
      }
    }

  return ;
}

/* Record dynamic details for a session which can only be found out by later interrogation
 * of the server. */
void zmapViewSessionAddServerInfo(ZMapViewSessionServer session_data, ZMapServerReqGetServerInfo session)
{
  if (session->data_format_out)
    session_data->format = g_strdup(session->data_format_out) ;


  switch(session_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	session_data->scheme_data.acedb.database = g_strdup(session->database_path_out) ;
	break ;
      }
    case SCHEME_FILE:
      {

	break ;
      }
    case SCHEME_PIPE:
      {

	break ;
      }
    case SCHEME_ENSEMBL:
      {

        break ;
      }
    default:
      {

	break ;
      }
    }

  return ;
}





/*
 *               Internal routines.
 */




/* Free all the session data, NOTE: we did not allocate the original struct
 * so we do not free it. */
static void freeServer(ZMapViewSessionServer server_data)
{
  g_free(server_data->url) ;
  g_free(server_data->protocol) ;
  g_free(server_data->format) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	g_free(server_data->scheme_data.acedb.host) ;
	g_free(server_data->scheme_data.acedb.database) ;
	break ;
      }
    case SCHEME_FILE:
      {
	g_free(server_data->scheme_data.file.path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	g_free(server_data->scheme_data.pipe.path) ;
	g_free(server_data->scheme_data.pipe.query) ;
	break ;
      }
    case SCHEME_ENSEMBL:
      {
        g_free(server_data->scheme_data.ensembl.host) ;
        break ;
      }
    default:
      {
	/* no action currently. */
	break ;
      }
    }

  return ;
}



