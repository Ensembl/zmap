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
 * Exported functions: See zmapServer.h
 * HISTORY:
 * Last edited: Mar 22 10:42 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.2 2004-03-22 13:22:25 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <strings.h>
#include <ZMap/zmapUtils.h>
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

  serverfuncs = g_new(ZMapServerFuncsStruct, sizeof(ZMapServerFuncsStruct)) ; /* n.b. crashes on failure. */

  /* Set up the server according to the protocol, this is all a bit hard coded but it
   * will do for now.... */
  /* Probably I should do this with a table of protocol and function stuff...perhaps
   * even using dynamically constructed function names....  */
  if (strcasecmp(protocol, "acedb") == 0)
    {
      acedbGetServerFuncs(serverfuncs) ;		    /* Must not fail..check with assert ? */
    }
  else if (strcasecmp(protocol, "das") == 0)
    {
      dasGetServerFuncs(serverfuncs) ;			    /* Must not fail..check with assert ? */
    }
  else
    {
      /* Fatal error..... */
      /* THIS "Internal Error" SHOULD BE IN A MACRO SO WE GET CONSISTENT MESSAGES..... */
      ZMAPFATALERR("Internal Error: unsupported server protocol: %s", protocol) ;

    }


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
				    char *host, int port, char *protocol,
				    char *userid, char *passwd)
{
  gboolean result = TRUE ;
  ZMapServer server ;
  ZMapServerFuncs serverfuncs = (ZMapServerFuncs)global_data ;


  server = g_new(ZMapServerStruct, sizeof(ZMapServerStruct)) ; /* n.b. crashes on failure. */
  *server_out = server ;

  /* set function table. */
  server->funcs = serverfuncs ;

  if (result)
    {
      if ((server->funcs->create)(&(server->server_conn), host, port, userid, passwd, 20))
	{
	  server->host = g_strdup(host) ;
	  server->port = port ;
	  server->protocol = g_strdup(protocol) ;
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


gboolean zMapServerOpenConnection(ZMapServer server)
{
  gboolean result = FALSE ;

  if ((server->funcs->open)(server->server_conn))
    result = TRUE ;
  else
    server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;

  return result ;
}


/* NEED TO SORT OUT WHETHER WE RETURN C STRINGS OR NOT.... */ 
gboolean zMapServerRequest(ZMapServer server, char *request, char **reply, int *reply_len)
{
  gboolean result = FALSE ;

  if ((server->funcs->request)(server->server_conn, request, (void **)reply, reply_len))
    result = TRUE ;
  else
    server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;

  return result ;
}

gboolean zMapServerCloseConnection(ZMapServer server)
{
  gboolean result = FALSE ;

  if ((server->funcs->close)(server->server_conn))
    {
      result = TRUE ;
    }
  else
    server->last_error_msg = (server->funcs->errmsg)(server->server_conn) ;

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
