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
 * Description: Connects to DAS v2 server to get data.
 *              
 * Exported functions: See ZMap/zmapServerPrototype.h
 * HISTORY:
 * Last edited: Feb  3 14:37 2005 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: dasServer.c,v 1.10 2005-02-03 14:59:30 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>					    /* only for testing... */

#include <zmapServerPrototype.h>
#include <dasServer_P.h>


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *host, int port, char *format, char *version_str,
				 char *userid, char *passwd, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType setContext(void *server,  char *sequence,
					 int start, int end, GData *types) ;
static ZMapFeatureContext copyContext(void *server_conn) ;
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server) ;
static gboolean destroyConnection(void *server) ;


static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) ;



/* Although most of these "external" routines are static they are properly part of our
 * external interface and hence are in the external section of this file. */


/* Compulsory routine, the caller must call this to get a list of our interface
 * functions, without this they can't do anything. The functions are only
 * visible through this struct. */
void dasGetServerFuncs(ZMapServerFuncs das_funcs)
{
  das_funcs->global_init = globalInit ;
  das_funcs->create = createConnection ;
  das_funcs->open = openConnection ;
  das_funcs->set_context = setContext ;
  das_funcs->copy_context = copyContext ;
  das_funcs->get_features = getFeatures ;
  das_funcs->get_sequence = getSequence ;
  das_funcs->errmsg = lastErrorMsg ;
  das_funcs->close = closeConnection;
  das_funcs->destroy = destroyConnection ;

  return ;
}


/* ok we need to sort out the urls into two parts that will equate with our model of
 * "host" and "request",
 * i.e.                   http://dev.acedb.org/das = host
 *                             the rest of the url = request  */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  /* This should only be called once and there is a corresponding clean up call, need to
   * sort this out......as a general mechanism...maybe the function pointer stuff needs
   * to be done once up front allowing calls to init and clean at an early stage....
   * note that this also returns a curl code which can be checked.... */
  if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK)
    result = FALSE ;


  return result ;
}




/* Need to sort what "host" should be, we can probably ignore "port" as http servers
 * are usually on a well known port, perhaps we should only set the port if we get passed
 * a port of zero........ */
static gboolean createConnection(void **server_out,
				 char *host, int port, char *format, char *version_str,
				 char *userid, char *passwd, int timeout)
{
  gboolean result = TRUE ;
  DasServer server ;

  server = (DasServer)g_new0(DasServerStruct, 1) ;

  server->host = g_strdup(host) ;
  server->port = port ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  server->userid = g_strdup(userid) ;
  server->passwd = g_strdup(passwd) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  server->chunks = 0 ;

  /* hack for now, need to parse in a proper struct here that can be interpreted and filled in
   * properly..... */
  server->parser = saxCreateParser(&(server->data)) ;

  server->last_errmsg = NULL ;
  server->curl_error = CURLE_OK ;
  server->curl_errmsg = (char *)g_malloc0(CURL_ERROR_SIZE * 2) ;	/* Big margin for safety. */
  server->data = NULL ;

  /* init the curl session */
  if (!(server->curl_handle = curl_easy_init()))
    {
      server->last_errmsg = "Cannot init curl" ;
      result = FALSE ;
    }

  /* Set buffer for curl to return error messages in. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_ERRORBUFFER, server->curl_error)) != CURLE_OK))
    result = FALSE ;
 
  /* Set callback function, called by curl everytime it has a buffer full of data. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)) != CURLE_OK))
    result = FALSE ;
 
  /* Set data we want passed to callback function. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_WRITEDATA, (void *)server)) != CURLE_OK))
    result = FALSE ;

  /* Optionally set a port, this will usually be the default http port. */
  if (result && port &&
      ((server->curl_error = curl_easy_setopt(server->curl_handle, CURLOPT_PORT, port)) != CURLE_OK))
    result = FALSE ;

  if (result)
    *server_out = (void *)server ;

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer server = (DasServer)server_in ;


  /* CURRENTLY THIS FUNCTION DOES NOTHING BECAUSE WE DON'T HOLD ON TO A CONNECTION TO THE
   * HTTP SERVER....NEED TO CHECK THIS OUT.... */


  return result ;
}


static ZMapServerResponseType setContext(void *server_in,  char *sequence,
					 int start, int end, GData *types)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  /* This operation needs implementing....... */

  return result ;
}


/* dummy routine...... */
static ZMapFeatureContext copyContext(void *server_in)
{
  ZMapFeatureContext context = NULL ;
  DasServer server = (DasServer)server_in ;

  return context ;
}



static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}


static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This function is defunct now, its code should be used to implement the getFeatures() and
 * getSequence() functions. */

/* OH, OK, THE HTTP BIT SHOULD BE THE REQUEST IN HERE PROBABLY.... */
static gboolean request(void *server_in, ZMapProtocolAny request)
{
  gboolean result = TRUE ;
  DasServer server = (DasServer)server_in ;
  ZMapProtocolGetFeatures get_seqfeatures = (ZMapProtocolGetFeatures)request ;

  /* specify URL to get */
  /* FAKED FOR NOW......... */
  {
    char *url = "http://dev.acedb.org/das/wormbase/1/type" ;

    curl_easy_setopt(server->curl_handle, CURLOPT_URL, url) ;
  }


  /* WE COULD CREATE A FRESH PARSER AT WITH EACH CALL AT THIS POINT, WOULD BE SIMPLER
   * PROBABLY..... */


  /* Hacked for now, normally we will use the request passed in..... */
  if (result &&
      ((server->curl_error = 
	curl_easy_setopt(server->curl_handle, CURLOPT_URL, server->host)) != CURLE_OK))
    result = FALSE ;

  /* OK, this call actually contacts the http server and gets the data. */
  if (result &&
      ((server->curl_error = curl_easy_perform(server->curl_handle)) != CURLE_OK))
    result = FALSE ;

  /* Dummy data.... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (result)
    {
      *reply = (void *)"das dummy data...." ;
      *reply_len = strlen("das dummy data....") + 1 ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* DUMMY FOR NOW......... */
  get_seqfeatures->feature_context_out = NULL ;


  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* N.B. last error may be NULL. */
static char *lastErrorMsg(void *server_in)
{
  DasServer server = (DasServer)server_in ;

  return server->last_errmsg ;
}


/* Close any connections that curl may have and clean up the xml parser. */
static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer server = (DasServer)server_in ;

  curl_easy_cleanup(server->curl_handle);
  server->curl_handle = NULL ;

  saxDestroyParser(server->parser) ;
  server->parser = NULL ;

  return result ;
}


/* Actually get rid of the control block, the final clean up. */
static gboolean destroyConnection(void *server_in)
{
  gboolean result = TRUE ;
  DasServer server = (DasServer)server_in ;


  g_free(server->host) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_free(server->userid) ;
  g_free(server->passwd) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  g_free(server) ;

  return result ;
}



/* 
 * ---------------------  Internal routines.  ---------------------
 */



/* Gets called by libcurl code every time libcurl has gathered some data from the http
 * server, this routine then sends the data to the expat xml parser. */
static size_t WriteMemoryCallback(void *xml, size_t size, size_t nmemb, void *app_data) 
{
  int realsize = size * nmemb;
  DasServer server = (DasServer)app_data ;

  server->chunks++ ;

  saxParseData(server->parser, xml, realsize) ;

  return realsize ;
}
