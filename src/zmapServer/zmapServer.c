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
 * Last edited: Sep  5 16:20 2003 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: zmapServer.c,v 1.1 2003-11-13 15:01:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <AceConn.h>
#include <zmapServer_P.h>



gboolean zMapServerCreateConnection(ZMapServer *server_out, char *host, int port,
				    char *userid, char *passwd)
{
  gboolean result = FALSE ;
  ZMapServer server ;
  AceConnection connection = NULL ;
  AceConnStatus status ;

  /* slightly odd, always return a struct to provide error information. */
  server = g_new(ZMapServerStruct, sizeof(ZMapServerStruct)) ; /* n.b. crashes on failure. */
  *server_out = server ;

  if ((status = AceConnCreate(&connection, host, port, userid, passwd, 20)) == ACECONN_OK)
    {
      /* Don't know if we really need to keep hold of these... */
      server->host = g_strdup(host) ;
      server->port = port ;

      server->connection = connection ;

      server->last_error_msg = NULL ;

      result = TRUE ;
    }
  else
    server->last_error_msg = AceConnGetErrorMsg(connection, status) ;

  return result ;
}


gboolean zMapServerOpenConnection(ZMapServer server)
{
  gboolean result = FALSE ;
  AceConnStatus status ;

  if ((status = AceConnConnect(server->connection)) == ACECONN_OK)
    result = TRUE ;
  else
    server->last_error_msg = AceConnGetErrorMsg(server->connection, status) ;

  return result ;
}


gboolean zMapServerRequest(ZMapServer server, char *request, char **reply)
{
  gboolean result = FALSE ;
  int reply_len = 0 ;
  AceConnStatus status ;

  if ((status = AceConnRequest(server->connection, request, (void **)reply, &reply_len)) == ACECONN_OK)
    result = TRUE ;
  else
    server->last_error_msg = AceConnGetErrorMsg(server->connection, status) ;

  return result ;
}

gboolean zMapServerCloseConnection(ZMapServer server)
{
  gboolean result = TRUE ;
  AceConnStatus status ;

  if ((status = AceConnConnectionOpen(server->connection)) == ACECONN_OK)
    {
      if ((status = AceConnDisconnect(server->connection)) != ACECONN_OK)
	result = FALSE ;
      else
	server->last_error_msg = AceConnGetErrorMsg(server->connection, status) ;
    }
  else
    server->last_error_msg = AceConnGetErrorMsg(server->connection, status) ;

  return result ;
}

gboolean zMapServerFreeConnection(ZMapServer server)
{
  gboolean result = TRUE ;

  AceConnDestroy(server->connection) ;			    /* Does not fail. */
  g_free(server->host) ;
  g_free(server) ;

  return result ;
}


char *zMapServerLastErrorMsg(ZMapServer server)
{
  return server->last_error_msg ;
}
