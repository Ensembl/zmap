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
 * Last edited: Mar 22 11:26 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: acedbServer.c,v 1.2 2004-03-22 13:32:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <AceConn.h>
#include <ZMap/zmapServerPrototype.h>
#include <acedbServer_P.h>


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout) ;
static gboolean openConnection(void *server) ;
static gboolean request(void *server, char *request, void **reply, int *reply_len) ;
char *lastErrorMsg(void *server) ;
static gboolean closeConnection(void *server) ;
static gboolean destroyConnection(void *server) ;


/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an acedb server. The functions are only visible through
 * this struct. */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs)
{
  acedb_funcs->global_init = globalInit ;
  acedb_funcs->create = createConnection ;
  acedb_funcs->open = openConnection ;
  acedb_funcs->request = request ;
  acedb_funcs->errmsg = lastErrorMsg ;
  acedb_funcs->close = closeConnection;
  acedb_funcs->destroy = destroyConnection ;

  return ;
}



/* 
 * ---------------------  Internal routines.  ---------------------
 */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;



  return result ;
}



static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout)
{
  gboolean result = FALSE ;
  AcedbServer server ;

  server = (AcedbServer)g_new(AcedbServerStruct, 1) ;
  server->last_err_status = ACECONN_OK ;

  if ((server->last_err_status =
       AceConnCreate(&(server->connection), host, port, userid, passwd, 20)) == ACECONN_OK)
    {
      *server_out = (void *)server ;

      result = TRUE ;
    }

  return result ;
}


static gboolean openConnection(void *server_in)
{
  gboolean result = FALSE ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status = AceConnConnect(server->connection)) == ACECONN_OK)
    result = TRUE ;

  return result ;
}


static gboolean request(void *server_in, char *request, void **reply, int *reply_len)
{
  gboolean result = FALSE ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status =
       AceConnRequest(server->connection, request, reply, reply_len)) == ACECONN_OK)
    result = TRUE ;

  return result ;
}


/* ummm, this doesn't seem to work for aceconn...where does the status come from...???? */
char *lastErrorMsg(void *server_in)
{
  AcedbServer server = (AcedbServer)server_in ;

  AceConnGetErrorMsg(server->connection, server->last_err_status) ;

  return NULL ;
}


static gboolean closeConnection(void *server_in)
{
  gboolean result = TRUE ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status = AceConnConnectionOpen(server->connection)) == ACECONN_OK)
    {
      if ((server->last_err_status = AceConnDisconnect(server->connection)) != ACECONN_OK)
	result = FALSE ;
    }

  return result ;
}

static gboolean destroyConnection(void *server_in)
{
  gboolean result = TRUE ;
  AcedbServer server = (AcedbServer)server_in ;

  AceConnDestroy(server->connection) ;			    /* Does not fail. */

  return result ;
}


