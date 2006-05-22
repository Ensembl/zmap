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
 * Last edited: May 22 10:25 2006 (rds)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: dasServer.c,v 1.19 2006-05-22 09:30:12 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>					    /* only for testing... */

#include <zmapServerPrototype.h>
#include <dasServer_P.h>

typedef struct
{
  ZMapServerResponseType result ;
  DasServer server ;
} GetFeaturesStruct, *GetFeatures ;

typedef struct
{
  GQuark capability;
  double level;
} hostAbilityStruct, *hostAbility;


/* required for server */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 zMapURL url, char *format, 
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType getTypes(void *server_in, GList *requested_types, GList **types);
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context);
static ZMapFeatureContext copyContext(void *server_conn) ;
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server) ;
static gboolean destroyConnection(void *server) ;

/* Internal */
static gboolean setSequenceMapping(DasServer server, ZMapFeatureContext feature_context);
static gboolean searchAssembly(DasServer server, dasOneDSN dsn);
static char *makeCurrentSegmentString(DasServer das, ZMapFeatureContext fc);

static void initialiseXMLParser(DasServer server);

static void getFeatures4Aligns(GQuark key,
                               gpointer data,
                               gpointer userData);
static void getFeatures4Blocks(gpointer data, gpointer userData);
static gboolean fetchFeatures(DasServer server, ZMapFeatureBlock block);

/* curl required */
static size_t WriteHeaderCallback(void *ptr, size_t size, size_t nmemb, void *data) ;
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) ;

/* helpers */
static char *dsnFullURL(DasServer server, 
                        char *query);
static gboolean serverHasCapabilityLevel(DasServer server, 
                                  char *capability, 
                                  double minimum);

/*  */
/* To do all requests */
static gboolean requestAndParseOverHTTP(DasServer server, 
                                        char *url, 
                                        dasDataType type);

/* Although most of these "external" routines are static they are properly part of our
 * external interface and hence are in the external section of this file. */


/* Compulsory routine, the caller must call this to get a list of our interface
 * functions, without this they can't do anything. The functions are only
 * visible through this struct. */
void dasGetServerFuncs(ZMapServerFuncs das_funcs)
{
  das_funcs->global_init  = globalInit ;
  das_funcs->create       = createConnection ;
  das_funcs->open         = openConnection ;
  das_funcs->get_types    = getTypes ;
  das_funcs->set_context  = setContext ;
  das_funcs->copy_context = copyContext ;
  das_funcs->get_features = getFeatures ;
  das_funcs->get_sequence = getSequence ;
  das_funcs->errmsg       = lastErrorMsg ;
  das_funcs->close        = closeConnection;
  das_funcs->destroy      = destroyConnection ;

  return ;
}

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
  /* We ALSO need to do a curl_global_cleanup! */
  
  return result ;
}

static gboolean createConnection(void **server_out,
				 zMapURL url, char *format, 
                                 char *version_str, int timeout)
{
  gboolean result = TRUE;
  gboolean fail_on_error = TRUE; /* Makes curl fail on none 200s */
  gboolean debug         = FALSE; /* Makes parser print debug */
  DasServer server ;

  server = (DasServer)g_new0(DasServerStruct, 1) ;

  server->protocol = g_quark_from_string(url->protocol);
  server->host     = g_quark_from_string(url->host) ;
  server->port     = url->port ;

  /* Make sure we don't store quarks of empty strings. */
  if(url->user && *(url->user))
    server->user   = g_quark_from_string(url->user) ;
  if(url->passwd && *(url->passwd))
    server->passwd = g_quark_from_string(url->passwd) ;

  if(url->path && *(url->path))
    server->path = g_quark_from_string(url->path);

  server->chunks = 0 ;
  server->debug  = (getenv("DAS_DEBUG") ? (debug = TRUE) : (debug = FALSE));

  initialiseXMLParser(server);
  
  server->last_errmsg = NULL ;
  server->curl_error  = CURLE_OK ;
  server->curl_errmsg = (char *)g_malloc0(CURL_ERROR_SIZE * 2) ;	/* Big margin for safety. */
  server->data        = NULL ;

  /* init the curl session */
  if (!(server->curl_handle = curl_easy_init()))
    {
      server->last_errmsg = "Cannot init curl" ;
      server->curl_error  = CURLE_FAILED_INIT; /* ASSUME THIS! */
      result = FALSE ;
    }

  /* Set buffer for curl to return error messages in. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_ERRORBUFFER, server->curl_errmsg)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }
 
  /* Set callback function, called by curl everytime it has a buffer full of data. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }
  /* Set data we want passed to callback function. */
  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_WRITEDATA, (void *)server)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }

  /* Optionally set a port, this will usually be the default http port. Might remove this.*/
  if (result && url->port &&
      ((server->curl_error = curl_easy_setopt(server->curl_handle, CURLOPT_PORT, url->port)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }

  if(result && 
     ((server->curl_error = curl_easy_setopt(server->curl_handle, CURLOPT_FAILONERROR, fail_on_error)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }
  if(result && 
     ((server->curl_error = curl_easy_setopt(server->curl_handle, CURLOPT_VERBOSE, debug)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }

  if (result &&
      ((server->curl_error =
	curl_easy_setopt(server->curl_handle, CURLOPT_HEADERFUNCTION, WriteHeaderCallback)) != CURLE_OK))
    {
      server->last_errmsg = g_strdup_printf("Line %d", __LINE__);
      result = FALSE ;
    }
  if(result &&
     ((server->curl_error =
       curl_easy_setopt(server->curl_handle, CURLOPT_WRITEHEADER, (void *)server)) != CURLE_OK))
    {
      result = FALSE;
    }

  if(result)
    *server_out = (void *)server ;

  return result ;
}

/* Need to check that the server has the dsn requested */
static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer server = (DasServer)server_in ;

  if(server->protocol == g_quark_from_string("file"))
    return result;
  else
    {
      char *url = NULL, *preDASPath = "";
      
      /* Only need to do this once */
      if(server->dsn_list != NULL)
        return result;

      /* Need to work out what preDASPath is
       * Not all servers are http://das.host.tld/das/dasSourceName
       * Some are set up as http://das.host.tld/web/dataAccess/das/dasSourceName
       * The dsn request in the latter case should be a url like
       * http://das.host.tld/web/dataAccess/das/dsn which obviously includes
       * the pre "das" part "web/dataAccess/".  This pre das part MUST
       * include the trailing "/" 
       * For now I'm assuming they're all the former!
       */
      
      /* Can we do a prompt for passwd here? */
      if(server->user && server->passwd)
        url = g_strdup_printf("%s://%s:%s@%s:%d/%sdas/dsn", 
                              g_quark_to_string(server->protocol),
                              g_quark_to_string(server->user),
                              g_quark_to_string(server->passwd),
                              g_quark_to_string(server->host),
                              server->port,
                              preDASPath
                              );
      else
        url = g_strdup_printf("%s://%s:%d/%sdas/dsn",
                              g_quark_to_string(server->protocol),
                              g_quark_to_string(server->host),
                              server->port,
                              preDASPath
                              );
      
      if(!requestAndParseOverHTTP(server, url, ZMAP_DASONE_DSN))
        result = ZMAP_SERVERRESPONSE_REQFAIL;
    }

  if(server->curl_error != CURLE_OK)
    result = ZMAP_SERVERRESPONSE_REQFAIL;

  if((result == ZMAP_SERVERRESPONSE_OK) && (checkDSNExists(server, NULL) != TRUE))
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL;
      server->last_errmsg = "DSN does not exist";
    }

  return result ;
}

static ZMapServerResponseType getTypes(void *server_in, GList *requested_types, GList **types)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK;
  DasServer server = (DasServer)server_in;
  char *url = NULL;

  url = dsnFullURL(server, "types");

  requestAndParseOverHTTP(server, url, ZMAP_DASONE_TYPES);

  return result;
}


static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer das = (DasServer)server ;
  gboolean status ;

  das->req_context = feature_context ;

  if(!(status = setSequenceMapping(server, feature_context)))
    result = ZMAP_SERVERRESPONSE_REQFAIL;
  else
    das->cur_context = feature_context;

  return result ;
}


/* dummy routine...... */
static ZMapFeatureContext copyContext(void *server_in)
{
  ZMapFeatureContext context = NULL ;
  DasServer server = (DasServer)server_in ;

  server->last_errmsg = "copying context";

  return context ;
}

static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK;
  DasServer server = (DasServer)server_in;
  GetFeaturesStruct get_features;

  get_features.result = result;
  get_features.server = server;

  g_datalist_foreach(&(feature_context->alignments), 
                     getFeatures4Aligns, 
                     (gpointer)&get_features);

  return get_features.result;
}


static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  

  return result ;
}

/* N.B. last error may be NULL. */
static char *lastErrorMsg(void *server_in)
{
  DasServer server = (DasServer)server_in ;

#ifdef SOME_THOUGHTS
  g_strdup_printf(__FILE__ " %s [[ libcurl (code, message) %d, %s ]]",
                  server->last_errmsg,
                  server->curl_error,
                  server->curl_errmsg);
#endif

  return server->last_errmsg ;
}


/* Close any connections that curl may have and clean up the xml parser. */
static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer server = (DasServer)server_in ;

  curl_easy_cleanup(server->curl_handle);
  server->curl_handle = NULL ;

  if(server->parser)
    zMapXMLParserDestroy(server->parser);
  server->parser = NULL ;

  return result ;
}


/* Actually get rid of the control block, the final clean up. */
static gboolean destroyConnection(void *server_in)
{
  gboolean result = TRUE ;
  DasServer server = (DasServer)server_in ;

  if(server->last_errmsg)
    g_free(server->last_errmsg);
  if(server->curl_errmsg)
    g_free(server->curl_errmsg);

  g_free(server) ;

  return result ;
}








/* 
 * ---------------------  Internal routines.  ---------------------
 */

static gboolean setSequenceMapping(DasServer server, ZMapFeatureContext feature_context)
{
  ZMapMapBlockStruct seq2p = {0, 0, 0, 0};
  ZMapSpanStruct pspan = {0, 0};
  char *urlep = NULL;
  dasOneEntryPoint ep;
  dasOneDSN dsn = NULL;
  gboolean result = FALSE;     /* A Flag to continue going */

  /* First we need to know whether we can get the sequence.
   * We get the entry points
   */
  if((result = checkDSNExists(server, &dsn)) == TRUE)
    {
      GList *list = NULL;
      urlep = dsnFullURL(server, "entry_points");
      ep    = dasOneEntryPoint_create1(g_quark_from_string(urlep), g_quark_from_string("1.0"));
      dasOneDSN_entryPoint(dsn, ep);
      requestAndParseOverHTTP(server, urlep, ZMAP_DASONE_ENTRY_POINTS);
      printf("FIX ME NOW!\n");

      while(list)
        {
          dasOneSegment current_segment = (dasOneSegment)list->data;
          dasOneEntryPoint_addSegment(ep, current_segment);
          if(dasOneSegment_id1(current_segment) == feature_context->sequence_name)
            server->current_segment = current_segment;
          list = list->next;
        }
      g_free(urlep);
    }
  
  /* Now we have entry points we can move on and get the rest of the information we need */

  if(result)
    {
      if(server->current_segment != NULL)
        {
          /* Ok look like we're a top level segment, this is easy. it only returns true!*/
          if(dasOneSegment_getBounds(server->current_segment, &(pspan.x1), &(pspan.x2)))
            {
              /* Just adjust for the user's request. */
              if(feature_context->sequence_to_parent.c1 >= pspan.x1)
                seq2p.p1 = seq2p.c1 = feature_context->sequence_to_parent.c1;
              else
                seq2p.p1 = seq2p.c1 = pspan.x1;

              if((feature_context->sequence_to_parent.c2 > feature_context->sequence_to_parent.c1) 
                 && feature_context->sequence_to_parent.c2 <= pspan.x2)
                seq2p.p2 = seq2p.c2 = feature_context->sequence_to_parent.c2;
              else
                seq2p.p2 = seq2p.c2 = pspan.x2;
            }
        }
      /* This is quite a big query */
      else if (searchAssembly(server, dsn))
        {
          if(!server->current_segment)
            {
              server->last_errmsg = g_strdup_printf("Segment '%s' not available", 
                                                    g_quark_to_string(feature_context->sequence_name));
              result = FALSE;
            }
          else /* now we need to do some complex sh1 te work it all out */
            {
              GData *datalist = NULL;
              dasOneFeature feature = NULL;
              printf("FIX ME NOW!!\n");
              //datalist = zMapXMLFactory_dataFetchOutput(server->factory, 
              //                                          g_quark_from_string("feature"));
              result = FALSE;   /* unless we find the sequence and we
                                   get it's location correctly */
              if((feature = (dasOneFeature)g_datalist_id_get_data(&(datalist), feature_context->sequence_name)) != NULL)
                {
                  /* We found the matching sequence */
                  if(dasOneFeature_getLocation(feature, &(seq2p.p1), &(seq2p.p2)) &&
                     dasOneFeature_getTargetBounds(feature, &(seq2p.c1), &(seq2p.c2)) &&
                     dasOneSegment_getBounds(server->current_segment, &(pspan.x1), &(pspan.x2)))
                    {
                      /* And we have filled in the structs.  */
                      result = TRUE;
                      /* Just adjust for the user's request. */
                      if(feature_context->sequence_to_parent.c1 >= seq2p.c1)
                        {
                          seq2p.p1 += feature_context->sequence_to_parent.c1 - seq2p.c1;
                          seq2p.c1 = feature_context->sequence_to_parent.c1;
                        }
                      if((feature_context->sequence_to_parent.c2 > feature_context->sequence_to_parent.c1) 
                         && (feature_context->sequence_to_parent.c2 <= seq2p.c2))
                        {
                          seq2p.p2 -= seq2p.c2 - feature_context->sequence_to_parent.c2;
                          seq2p.c2 = feature_context->sequence_to_parent.c2;
                        }
                    }
                }
            }
          
        }
      else
        result = FALSE;
    }

  if(result)
    {
      /* Copy the structs we made to the feature context */
      feature_context->length = seq2p.c2 - seq2p.c1 + 1;
      feature_context->sequence_to_parent.c1 = seq2p.c1;
      feature_context->sequence_to_parent.c2 = seq2p.c2;
      feature_context->sequence_to_parent.p1 = seq2p.p1;
      feature_context->sequence_to_parent.p2 = seq2p.p2;
      feature_context->parent_span.x1 = pspan.x1;
      feature_context->parent_span.x2 = pspan.x2;
      feature_context->parent_name = dasOneSegment_id1(server->current_segment);
      printf("|-----------------------------------------|\n");
      printf("|Parent is " ZMAP_DAS_FORMAT_SEGMENT "\n",
             (char *)g_quark_to_string( feature_context->parent_name ),
             feature_context->sequence_to_parent.p1,
             feature_context->sequence_to_parent.p2);
      printf("|child is " ZMAP_DAS_FORMAT_SEGMENT "\n", 
             (char *)g_quark_to_string( feature_context->parent_name ),
             feature_context->sequence_to_parent.c1,
             feature_context->sequence_to_parent.c2);
    }

  return result;
}



/*

CURLOPT_WRITEFUNCTION

Function pointer that should match the following prototype: size_t
function( void *ptr, size_t size, size_t nmemb, void *stream); This
function gets called by libcurl as soon as there is data received that
needs to be saved. The size of the data pointed to by ptr is size
multiplied with nmemb, it will not be zero terminated. Return the
number of bytes actually taken care of. If that amount differs from
the amount passed to your function, it'll signal an error to the
library and it will abort the transfer and return CURLE_WRITE_ERROR.

This function may be called with zero bytes data if the transfered
file is empty.

Set the stream argument with the CURLOPT_WRITEDATA option.

NOTE: you will be passed as much data as possible in all invokes, but
you cannot possibly make any assumptions. It may be one byte, it may
be thousands. The maximum amount of data that can be passed to the
write callback is defined in the curl.h header file:
CURL_MAX_WRITE_SIZE.

*/

/* Gets called by libcurl code every time libcurl has gathered some data from the http
 * server, this routine then sends the data to the expat xml parser. */
static size_t WriteMemoryCallback(void *xml, size_t size, size_t nmemb, void *app_data) 
{
  int realsize     = size * nmemb;
  DasServer server = (DasServer)app_data ;

  server->chunks++ ;

  if(zMapXMLParserParseBuffer(server->parser, xml, realsize) != TRUE)
    realsize = 0;               /* Signal an error to curl we should
                                   be able to figure it out that this
                                   is what went wrong */

  return realsize ;
}

static size_t WriteHeaderCallback(void *hdr_in, size_t size, size_t nmemb, void *data)
{
  int realsize     = size * nmemb;
  char *wantKey    = "X-DAS-Capabilities:";
  DasServer server = (DasServer)data ;
  char *hdr = (char *)hdr_in ;
  
  if(server && server->hostAbilities == NULL)
    {
      if((g_str_has_prefix((char *)hdr, wantKey) == TRUE))
        {
          int i, valStart = strlen(wantKey);
          char **caps = NULL;
          hdr+=valStart;
          caps = g_strsplit(hdr, ";", realsize - valStart);
          for(i = 0; caps[i] && *(caps[i]); i++)
            {
              char *cap = caps[i];
              char **keyval;
              double level;
              hostAbility ability = NULL;
              ability = g_new(hostAbilityStruct, 1);
              g_strstrip(cap);

              keyval = g_strsplit(cap, "/", strlen(cap));
              level  = strtod(keyval[1], (char **)NULL);

              ability->capability = g_quark_from_string(g_ascii_strdown(keyval[0], strlen(keyval[0])));
              ability->level      = level;
              server->hostAbilities = g_list_prepend(server->hostAbilities, ability);
            }
        }
    }
  return realsize;
}

static gboolean requestAndParseOverHTTP(DasServer server, char *url, dasDataType type)
{
  gboolean response = FALSE, result = TRUE;

  server->type = type;

  /* Set up the correct parsing handlers */
  switch(server->type)
    {
    case ZMAP_DASONE_DSN:
    case ZMAP_DASONE_DNA:
    case ZMAP_DASONE_SEQUENCE:
    case ZMAP_DASONE_ENTRY_POINTS:
    case ZMAP_DASONE_TYPES:
    case ZMAP_DASONE_INTERNALFEATURES:
    case ZMAP_DASONE_FEATURES:
    case ZMAP_DASONE_LINK:
    case ZMAP_DASONE_STYLESHEET:
      break ;
    default:
      server->last_errmsg = g_strdup_printf("Unknown Das type %d",  server->type);
      server->type        = ZMAP_DAS_UNKNOWN;
      result = FALSE;
      break ;
    }

  if(result && !(url && *url))
    {
      result = FALSE;
      server->last_errmsg = "dasServer cannot continue with null url";
    }

  /* result will be TRUE unless we've been passed an unknown
     dasDataType in which case the next two conditionals will be
     skipped */

  /* If we can't parse the url in curl we'll get an error and no more
     should happen and we set result to FALSE*/
  if (result && ((server->curl_error = 
                  curl_easy_setopt(server->curl_handle, CURLOPT_URL, url)) != CURLE_OK))
    {
      result = FALSE ;
      server->last_errmsg = g_strdup_printf("dasServer requesting '%s' over HTTP Error: %d, %s",
                                            url,
                                            server->curl_error,
                                            server->curl_errmsg
                                            );
    }

  /* OK, this call actually contacts the http server and gets the data.
   * ONLY if everything succeeds will we get a response of TRUE.
   */
  if (result)
    {
      if(((server->curl_error = 
           curl_easy_perform(server->curl_handle)) == CURLE_OK))
        response = TRUE ;
      else
        {
          result = FALSE;
          /* Have to check the parser in case it hit a wall! */
          /* checks that error wasn't write error (falls through immediately)
           * but if it was then checks it actually was WRITE_ERROR (pointless??) 
           * and that the parser hasn't got an error meesage. In which case it falls through
           */
          if((server->curl_error != CURLE_WRITE_ERROR) ||
             ((server->curl_error == CURLE_WRITE_ERROR) 
              && ((server->last_errmsg = zMapXMLParserLastErrorMsg(server->parser)) == NULL)))
            server->last_errmsg = g_strdup_printf("dasServer url: '%s', HTTP Error: %d, %s",
                                                  url,
                                                  server->curl_error,
                                                  server->curl_errmsg
                                                  );
          /* I haven't checked, but I think we need to finish off the parser if we get here.
           * need to consider/preserve/copy the parser->last_errmsg though.
           */
        }
    }

  /* SHOULDN'T WE BE RESETTING THE PARSER HERE???? */
  /* finish off the parser... */
  if(result && (zMapXMLParserParseBuffer(server->parser, NULL, 0) != TRUE))
    {
      response = FALSE;
      server->last_errmsg = zMapXMLParserLastErrorMsg( server->parser );
    }
  /* ALL THE DATA CREATION GETS DONE IN THE HANDLERS! */

  return response;
}

/* Needs RENAMING */
gboolean checkDSNExists(DasServer das, dasOneDSN *dsn)
{
  gboolean exists = FALSE;
  GList *list = NULL;
  char *pathstr = NULL, *dsn_str = NULL;
  list = das->dsn_list;
  GQuark want;

  pathstr  = (char *)g_quark_to_string(das->path);
  dsn_str  = g_strrstr(pathstr, "das/");
  dsn_str += 4;
#ifdef USE_STRDUPDELIM_LOGIC_HERE_RDS
  char *params_str = NULL;
  if((params_str = g_strrstr(pathstr, ";")) != NULL)
    {
      dsn_str[params_str - dsn_str] = '\0';
    }
#endif
  want     = g_quark_from_string(dsn_str);
  while(list)
    {
      dasOneDSN cur_dsn = list->data;
      if(want == dasOneDSN_id1(cur_dsn))
        {
          exists = TRUE;
          if(dsn != NULL)
            *dsn = cur_dsn;
          break;
        }
      list = list->next;
    }
  
  return exists;
}

/* We might get asked for a sequence which is quite a valid reference
 * sequence (clone/contig) that isn't listed in the entry_points query.
 * In order to get round this we must request all of the features of 
 * category component from all of the segments listed in the entry_points
 * results which have subparts="yes".  These segments _MUST_ already be 
 * a property of the dsn entry point.

 * The data is set up in the handler routine (see das1XMLhandlers.c)
 */

static gboolean searchAssembly(DasServer server, dasOneDSN dsn)
{
  char *asmURL = NULL;
  GList *list  = NULL;
  GString *tmp = NULL;
  gboolean res = FALSE;

  list = dasOneEntryPoint_getSegmentsList( dasOneDSN_entryPoint(dsn, NULL) );
  tmp  = g_string_sized_new(400); /* segment=ID:1,1000000 (length = 20) * 20 = 400 */
  while(list)
    {
      int start, stop;
      dasOneRefProperties ref;
      dasOneSegment s = (dasOneSegment)list->data;
      dasOneSegment_getBounds(s, &start, &stop);
      ref = dasOneSegment_refProperties(s, NULL, NULL, FALSE);
      /* ONLY append those which have subparts */
      if(ref & (DASONE_REF_ISREFERENCE | DASONE_REF_HASSUBPARTS))
        g_string_append_printf(tmp, ZMAP_DAS_FORMAT_SEGMENT, 
                               g_quark_to_string(dasOneSegment_id1(s)), 
                               start, stop);
      list = list->next;
    }

  if(tmp->str && *(tmp->str))   /* Check we have a string */
    {
      char *tmpURL = NULL;
      tmpURL = dsnFullURL(server, "features?");
      asmURL = g_strdup_printf("%s%scategory=component;categorize=yes;feature_id=%s", 
                               tmpURL, tmp->str, 
                               g_quark_to_string(server->req_context->sequence_name));
      g_free(tmpURL);
    }
  /* We've copied the data, so we can free it */
  g_string_free(tmp, TRUE);
  
  if(!(res = requestAndParseOverHTTP(server, asmURL, ZMAP_DASONE_INTERNALFEATURES)))
    server->current_segment = NULL; /* Just in case something odd is going on */
    
  if(asmURL)
    g_free(asmURL);

  return res;
}

static void getFeatures4Aligns(GQuark key,
                               gpointer data,
                               gpointer userData)
{
  ZMapFeatureAlignment align = (ZMapFeatureAlignment)data;
  GetFeatures getFeaturesPtr = (GetFeatures)userData;

  if(getFeaturesPtr->result == ZMAP_SERVERRESPONSE_OK)
    g_list_foreach(align->blocks, getFeatures4Blocks, (gpointer)getFeaturesPtr);

  return ;
}
static void getFeatures4Blocks(gpointer data, gpointer userData)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data;
  GetFeatures getFeaturesPtr = (GetFeatures)userData;

  if(getFeaturesPtr->result == ZMAP_SERVERRESPONSE_OK)
    {
      if(!fetchFeatures(getFeaturesPtr->server, block))
        {
          getFeaturesPtr->result = ZMAP_SERVERRESPONSE_REQFAIL;
        }
    }

  return ;
}

static gboolean fetchFeatures(DasServer server, ZMapFeatureBlock block)
{
  gboolean cont = TRUE;
  char *segStr = NULL, *query = NULL, *url = NULL;
  
  if((segStr = makeCurrentSegmentString(server, (ZMapFeatureContext)(block->parent->parent))) == NULL)
    {
      server->last_errmsg = "Failed to get current segment";
      cont = FALSE;
    }

  if(cont && ((query = g_strdup_printf("features?%s", segStr)) == NULL))
    cont = FALSE;

  if(cont && ((url = dsnFullURL(server, query)) == NULL))
    cont = FALSE;

  if(cont)
    {
      g_free(segStr);           /* free as we go */
      g_free(query);
      if(!(requestAndParseOverHTTP(server, url, ZMAP_DASONE_FEATURES)))
        cont = FALSE;
      g_free(url); 
    }

  return TRUE;
}


/* This needs to be a little bit cleverer than it currently is.  ATM
 * it only calculates the string based on the assumption (wrongly)
 * that the server->current_segment will be a listed entry_point and
 * hence we'll fetch the wrong/too much of the segment.

 * <entry_points>
 *   <segment id="chr3" />
 * </entry_points>

 * <segment id="chr3">
 *   <feature id="AL10102.3.1.123123">
 *   <start>2001</start>
 *   <stop>9000</stop>
 *   <target start="1" stop="9000" />
 * </segment>

 *   |-------------------------chr3--------------------------------|
 *   1                                                           10000
 *          |---------------AL10102.3.1.123123-----------------|

 * Hence we need to map the AL10102.3.1.123123 to chr3 coords.
 */

static char *makeCurrentSegmentString(DasServer das, ZMapFeatureContext fc)
{
  char *query  = NULL;

  if(das->current_segment)
    {
      query = g_strdup_printf(ZMAP_DAS_FORMAT_SEGMENT,
                              (char *)g_quark_to_string(dasOneSegment_id1(das->current_segment)),
                              fc->sequence_to_parent.p1,
                              fc->sequence_to_parent.p2);
    }

  return query;
}

/* Must free me when done with me!!! */
static char *dsnFullURL(DasServer server, char *query)
{
  char *url = NULL;
  if(server->user && server->passwd)
    url = g_strdup_printf(ZMAP_URL_FORMAT_UN_PWD, 
                          g_quark_to_string(server->protocol),
                          g_quark_to_string(server->user),
                          g_quark_to_string(server->passwd),
                          g_quark_to_string(server->host),
                          server->port,
                          g_quark_to_string(server->path),
                          query
                          );
  else
    url = g_strdup_printf(ZMAP_URL_FORMAT,
                          g_quark_to_string(server->protocol),
                          g_quark_to_string(server->host),
                          server->port,
                          g_quark_to_string(server->path),
                          query
                          );
  return url;
}

static gboolean serverHasCapabilityLevel(DasServer server, char *capability, double minimum)
{
  GList *list = NULL;
  GQuark want = 0;
  gboolean hasLevel = FALSE;
  
  list = server->hostAbilities;
  want = g_quark_from_string(g_ascii_strdown(capability, -1));
  while(list)
    {
      hostAbility ability = (hostAbility)list->data;
      if(ability->capability == want)
        {
          if(ability->level >= minimum)
            hasLevel = TRUE;
        }
    }
  return hasLevel;
}

static void initialiseXMLParser(DasServer server)
{  
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "dsn",        dsnStart },
    { "segment",    segStart },
    { "client",     NULL },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "dsn",        dsnEnd  },
    { "segment",    segEnd  },
    { "dasdsn",     cleanUpDoc },
    { NULL, NULL }
  };

  server->parser = zMapXMLParserCreate((void *)server, FALSE, server->debug);
  /*  zMapXMLParser_setMarkupObjectHandler(parser, start, end); */
  zMapXMLParserSetMarkupObjectTagHandlers(server->parser, &starts[0], &ends[0]);

  return ;
}
