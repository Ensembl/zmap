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
 * Last edited: Jul 15 14:26 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: acedbServer.c,v 1.4 2004-07-15 15:12:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <AceConn.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapServerPrototype.h>
#include <ZMap/zmapGFF.h>
#include <acedbServer_P.h>


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout) ;
static gboolean openConnection(void *server) ;
static gboolean request(void *server, ZMapServerRequestType request,
			char *sequence, ZMapFeatureContext *feature_context) ;
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

  /* Always return a server struct as it contains error message stuff. */
  server = (AcedbServer)g_new0(AcedbServerStruct, 1) ;
  server->last_err_status = ACECONN_OK ;
  *server_out = (void *)server ;

  if ((server->last_err_status =
       AceConnCreate(&(server->connection), host, port, userid, passwd, 20)) == ACECONN_OK)
    result = TRUE ;

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


static gboolean request(void *server_in, ZMapServerRequestType request,
			char *sequence, ZMapFeatureContext *feature_context_out)
{
  gboolean result = FALSE ;
  AcedbServer server = (AcedbServer)server_in ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;


  /* Lash up for now, we will later need a  "ZMap request" -> "acedb request" translator. */
  acedb_request =  g_strdup_printf("gif seqget %s ; seqfeatures", sequence) ;


  if ((server->last_err_status =
       AceConnRequest(server->connection, acedb_request, &reply, &reply_len)) == ACECONN_OK)
    {
      char *next_line ;
      ZMapReadLine line_reader ;
      gboolean inplace = TRUE ;
      ZMapGFFParser parser ;
      ZMapFeatureContext feature_context = NULL ;

      /* Here we can convert the GFF that comes back, in the end we should be doing a number of
       * calls to AceConnRequest() as the server slices...but that will need a change to my
       * AceConn package.....
       * We make the big assumption that what comes back is a C string for now, this is true
       * for most acedb requests, only images/postscript are not and we aren't asking for them. */
      
      line_reader = zMapReadLineCreate((char *)reply, inplace) ;

      parser = zMapGFFCreateParser(FALSE) ;

      /* We probably won't have to deal with part lines here acedb should only return whole lines
       * ....but should check for sure...bomb out for now.... */
      do
	{
	  if (!zMapReadLineNext(line_reader, &next_line) && *next_line)
	    {
	      zMapLogFatal("Request from server contained incomplete line: %s", next_line) ;
	    }
	  else
	    {
	      if (!zMapGFFParseLine(parser, next_line))
		{
		  GError *error = zMapGFFGetError(parser) ;

		  if (!error)
		    {
		      zMapLogCritical("zMapGFFParseLine() failed with no GError for line %d: %s\n",
				      zMapGFFGetLineNumber(parser), next_line) ;
		    }
		  else
		    zMapLogWarning("%s\n", (zMapGFFGetError(parser))->message) ;
		}
	    }
	}
      while (*next_line) ;

      if ((feature_context = zmapGFFGetFeatures(parser)))
	{
	  *feature_context_out = feature_context ;
	  result = TRUE ;
	}

      zMapReadLineDestroy(line_reader, TRUE) ;
      zMapGFFSetFreeOnDestroy(parser, FALSE) ;
      zMapGFFDestroyParser(parser) ;
    }

  g_free(acedb_request) ;

  return result ;
}



char *lastErrorMsg(void *server_in)
{
  char *err_msg ;
  AcedbServer server = (AcedbServer)server_in ;

  zMapAssert(server_in) ;

  err_msg = AceConnGetErrorMsg(server->connection, server->last_err_status) ;

  return err_msg ;
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


