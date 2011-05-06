/*  File: zmapServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and,
 *          Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *       Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Connects to DAS v2 server to get data.
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 * HISTORY:
 * Last edited: Nov 11 15:02 2010 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: dasServer.c,v 1.51 2011-05-06 14:52:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>					    /* only for testing... */

#include <zmapServerPrototype.h>
#include <ZMap/zmapUtils.h>
#include <dasServer_P.h>

typedef struct
{
  ZMapServerResponseType result ;
  DasServer server ;
  GHashTable *styles ;
} GetFeaturesStruct, *GetFeatures ;

typedef struct
{
  ZMapFeatureBlock block;
  DasServer server;
  GHashTable *styles ;
} BlockServerStruct, *BlockServer;

typedef struct
{
  GQuark capability;
  double level;
} hostAbilityStruct, *hostAbility;

typedef struct
{
  ZMapServerResponseType result ;
  DasServer server;
  GHashTable *output;
}DasServerTypesStruct, *DasServerTypes;

typedef struct
{
  char *url;
  ZMapDAS1QueryType request_type;
  gpointer filter;
  gpointer filter_data;
}requestParseDetailStruct, *requestParseDetail;

typedef struct
{
  GQuark id_to_match;
  ZMapDAS1Segment segment;
  gboolean matched;
} listFindResultStruct, *listFindResult;


/* required for server */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server,ZMapServerReqOpen req_open) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType getFeatureSets(void *server,
					     GList **feature_sets_out,
					     GList *sources,
					     GList **required_styles,
					     GHashTable **featureset_2_stylelist_inout,
					     GHashTable **featureset_2_column_inout,
					     GHashTable **source_2_sourcedata_inout) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context);
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out);
static ZMapServerResponseType closeConnection(void *server) ;
static ZMapServerResponseType destroyConnection(void *server) ;

/* Internal */
static gboolean segmentsFindCurrent(gpointer data, gpointer user_data);
static gboolean setSequenceMapping(DasServer server, ZMapFeatureContext feature_context);

#ifdef RDS_DONT_INCLUDE
static gboolean searchAssembly(DasServer server, ZMapDAS1DSN dsn);
#endif

static char *makeCurrentSegmentString(DasServer das, ZMapFeatureContext fc);

static void initialiseXMLParser(DasServer server);

static void getFeatures4Aligns(gpointer key, gpointer data, gpointer userData);
static void getFeatures4Blocks(gpointer key, gpointer data, gpointer userData) ;

static gboolean fetchFeatures(DasServer server, GHashTable *styles, ZMapFeatureBlock block);


/* curl required */
static size_t WriteHeaderCallback(void *ptr, size_t size, size_t nmemb, void *data) ;
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) ;


static void dsnFilter        (ZMapDAS1DSN dsn,                gpointer user_data);
#ifdef RDS_DONT_INCLUDE
static void dnaFilter        (ZMapDAS1DNA dna,                gpointer user_data);
#endif
static void entryFilter      (ZMapDAS1EntryPoint entry_point, gpointer user_data);
static void typesFilter      (ZMapDAS1Type type,              gpointer user_data);
static void featureFilter    (ZMapDAS1Feature feature,        gpointer user_data);
static void stylesheetFilter (ZMapDAS1Stylesheet style,       gpointer user_data);

static void applyGlyphToEachType(gpointer style_id, gpointer data, gpointer user_data) ;

static gboolean getRequestedDSN(DasServer das, ZMapDAS1DSN *dsn_out);

/* helpers */
static char *dsnRequestURL(DasServer server);
static char *dsnFullURL(DasServer server,
                        char *query);
#ifdef RDS_DONT_INCLUDE
static gboolean serverHasCapabilityLevel(DasServer server,
                                  char *capability,
                                  double minimum);
#endif
static gint compareExonCoords(gconstpointer a, gconstpointer b, gpointer feature_data);
static void fixFeatureCache(gpointer key, gpointer data, gpointer user_data);
static void fixUpFeaturesAndClearCache(DasServer server, ZMapFeatureBlock block);
static gboolean mergeWithGroupFeature(ZMapFeature grp_feature,
                                      ZMapDAS1Feature das_sub_feature,
                                      ZMapStyleMode feature_type,

                                      GQuark group_id);


/*  */
/* To do all requests */
static gboolean requestAndParseOverHTTP(DasServer server,
                                        requestParseDetail detail);

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
  das_funcs->get_info = getInfo ;
  das_funcs->feature_set_names = getFeatureSets ;
  das_funcs->get_styles = getStyles ;
  das_funcs->have_modes = haveModes ;
  das_funcs->get_sequence = getSequences ;
  das_funcs->set_context  = setContext ;
  das_funcs->get_features = getFeatures ;
  das_funcs->get_context_sequences = getContextSequence ;
  das_funcs->errmsg       = lastErrorMsg ;
  das_funcs->get_status   = getStatus ;
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
				 ZMapURL url, char *format,
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

  server->zmap_start = 1;
  server->zmap_end = 0;

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

  server->feature_cache = g_hash_table_new(NULL, NULL) ;

  if(result)
    *server_out = (void *)server ;

  return result ;
}

/* Need to check that the server has the dsn requested */
// mh17: added sequence_server flag for compatability with pipeServer, it's not used here
static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  DasServer server = (DasServer)server_in ;
  ZMapDAS1DSN dsn = NULL;
  requestParseDetailStruct detail = {0};

  server->zmap_start = req_open->zmap_start;
  server->zmap_end = req_open->zmap_end;

  if(server->protocol == g_quark_from_string("file"))
    return result;
  else
    {
      /* Only need to do this once */
      if(server->dsn_list != NULL)
        return result;

      detail.url          = dsnRequestURL(server);
      detail.filter       = dsnFilter;
      detail.filter_data  = server;
      detail.request_type = ZMAP_DAS1_DSN;
      if(!requestAndParseOverHTTP(server, &detail))
        result = ZMAP_SERVERRESPONSE_REQFAIL;
    }

  if((result == ZMAP_SERVERRESPONSE_OK) && (getRequestedDSN(server, &dsn) != TRUE))
    result = ZMAP_SERVERRESPONSE_REQFAIL;

  if((result == ZMAP_SERVERRESPONSE_OK) && dsn->entry_point == NULL)
    {
      if(detail.url)
        g_free(detail.url);

      detail.url          = dsnFullURL(server, "entry_points");
      detail.request_type = ZMAP_DAS1_ENTRY_POINTS;
      detail.filter       = entryFilter;

      if((requestAndParseOverHTTP(server, &detail) == TRUE))
        result = ZMAP_SERVERRESPONSE_OK;
      else
        result = ZMAP_SERVERRESPONSE_REQFAIL;
    }

  if(detail.url)
    g_free(detail.url);

  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  DasServer server = (DasServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

/* MH17: treat as pipe server not ACEDB??
  no idea really but this is a reminder to find out
  DAS code is not currently used. DAS comes in via a pipe
*/
  info->request_as_columns = FALSE;

  /* Needs implementing............ */

  return result ;
}



static ZMapServerResponseType getStyles(void *server_in, GHashTable **types)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK;
  DasServer server = (DasServer)server_in;
  DasServerTypesStruct server_types = { 0 };
  requestParseDetailStruct detail = {0};

  server_types.server = server;
  server_types.output = NULL;

  detail.url          = dsnFullURL(server, "types");
  detail.filter       = typesFilter;
  detail.filter_data  = &server_types;
  detail.request_type = ZMAP_DAS1_TYPES;

  if (!requestAndParseOverHTTP(server, &detail))
    result = ZMAP_SERVERRESPONSE_REQFAIL;
  else if (types)
    {
      if(detail.url)
        g_free(detail.url);

      detail.url          = dsnFullURL(server, "stylesheet");
      detail.request_type = ZMAP_DAS1_STYLESHEET;
      detail.filter       = stylesheetFilter;

      if(!requestAndParseOverHTTP(server, &detail))
        result = ZMAP_SERVERRESPONSE_REQFAIL;
      else
	{
	  *types = server_types.output ;
	}
    }

  if(detail.url)
    g_free(detail.url);

  return result;
}


/* DAS styles do not include a mode (e.g. transcript etc) so this function
 * always returns FALSE.
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  DasServer server = (DasServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  *have_mode = FALSE ;

  return result ;
}


static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out)
{
      *exit_code = 0;
      *stderr_out = NULL;
      return ZMAP_SERVERRESPONSE_OK;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}



/* Roy, this function is supposed to get a list of the _names_ of all the feature
 * sets, not the feature sets themselves.
 *
 * I haven't filled it in as there was no original code to do this.
 *  */
static ZMapServerResponseType getFeatureSets(void *server,
					     GList **feature_sets_out,
					     GList *sources,
					     GList **required_styles,
					     GHashTable **featureset_2_stylelist_inout,
					     GHashTable **featureset_2_column_inout,
					     GHashTable **source_2_sourcedata_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK;


  return result ;
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


static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK;
  DasServer server = (DasServer)server_in;
  GetFeaturesStruct get_features;

  get_features.result = result;
  get_features.server = server;
  get_features.styles = styles ;

  g_hash_table_foreach(feature_context->alignments, getFeatures4Aligns, (gpointer)&get_features);

  return get_features.result;
}


static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
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

  if(server->das)
    zMapDAS1ParserDestroy(server->das);

  if(server->parser)
    zMapXMLParserDestroy(server->parser);
  server->parser = NULL ;

  return result ;
}


/* Actually get rid of the control block, the final clean up. */
static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
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

static gint segmentsFindCurrent(gpointer data, gpointer user_data)
{
  ZMapDAS1Segment segment  = (ZMapDAS1Segment)data;
  listFindResult full_data = (listFindResult)user_data;
  gint found = 1;

  if((segment->id == full_data->id_to_match))
    {
      full_data->segment = segment;
      full_data->matched = TRUE;
      found = 0;
    }

  return found;
}

static gboolean setSequenceMapping(DasServer server, ZMapFeatureContext feature_context)
{
  ZMapMapBlockStruct seq2p = {{0, 0}, {0, 0},FALSE};
  ZMapSpanStruct pspan = {0, 0};
  ZMapDAS1DSN dsn = NULL;
  gboolean result = FALSE;     /* A Flag to continue going */
  GList *segments = NULL;
  listFindResultStruct full_data = {0};

  /* First we need to know whether we can get the sequence.
   * We get the entry points
   */
  if((result = getRequestedDSN(server, &dsn)) == TRUE)
    {
      if(!(zMapDAS1DSNSegments(dsn, &segments)))
        {
          /* Why hasn't the dsn got any entry points/segments */
          /* We could possibly try to fetch them again from here */
          result = FALSE;       /* but for now we'll stop. */
        }
      else
        {
          full_data.id_to_match = feature_context->sequence_name;
          g_list_find_custom(segments, &full_data, (GCompareFunc)segmentsFindCurrent);
          if(full_data.matched)
            server->current_segment = full_data.segment;
        }
    }

  /* Now we have entry points we can move on and get the rest of the
   * information we need */
  if(result)
    {
      if(server->current_segment != NULL)
        {
          pspan.x1 = server->current_segment->start;
          pspan.x2 = server->current_segment->stop;

          if(pspan.x1 && pspan.x2)
            {
#warning need to check this code
              /* Just adjust for the user's request. */
#if 0
              if(feature_context->sequence_to_parent.c1 >= pspan.x1)
                seq2p.p1 = seq2p.c1 = feature_context->sequence_to_parent.c1;
              else
                seq2p.p1 = seq2p.c1 = pspan.x1;

              if((feature_context->sequence_to_parent.c2 > feature_context->sequence_to_parent.c1)
                 && feature_context->sequence_to_parent.c2 <= pspan.x2)
                seq2p.p2 = seq2p.c2 = feature_context->sequence_to_parent.c2;
              else
                seq2p.p2 = seq2p.c2 = pspan.x2;
#else
            int seq_start,seq_end;
            zMapFeatureContextGetMasterAlignSpan(feature_context,&seq_start,&seq_end);

            if(seq_start >= pspan.x1)
                seq2p.parent.x1 = seq2p.block.x1 = seq_start;
            else
                seq2p.parent.x1 = seq2p.block.x1 = pspan.x1;

            if((seq_end > seq_start)
                 && seq_end <= pspan.x2)
                seq2p.parent.x2 = seq2p.block.x2 = seq_end;
            else
                seq2p.parent.x2 = seq2p.block.x2 = pspan.x2;
#endif
            }
        }
#ifdef RDS_DONT_INCLUDE
      /* This is quite a big query */
      else if (searchAssembly(server, dsn))
        {
          if(!server->current_segment)
            {
              server->last_errmsg = g_strdup_printf("Segment '%s' not available",
                                                    g_quark_to_string(feature_context->sequence_name));
              result = FALSE;
            }
          else if(0) /* now we need to do some complex sh1 te work it all out */
            {
              GHashTable *datalist = NULL;
              dasOneFeature feature = NULL;
              printf("FIX ME NOW!!\n");

              result = FALSE;   /* unless we find the sequence and we
                                   get it's location correctly */

              if((feature = (dasOneFeature)g_hash_table_lookup(datalist,
							       GINT_TO_POINTER(feature_context->sequence_name)))
		 != NULL)
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
#endif
      else
        result = FALSE;
    }

  if(result)
    {
      /* Copy the structs we made to the feature context */
//    feature_context->length                = seq2p.c2 - seq2p.c1 + 1;

#warning need to check this code
#if !OLD_VERSION
      feature_context->master_align->sequence_span.x1 = seq2p.block.x1;
      feature_context->master_align->sequence_span.x2 = seq2p.block.x2;
#else
      feature_context->sequence_to_parent.c1 = seq2p.c1;
      feature_context->sequence_to_parent.c2 = seq2p.c2;
      feature_context->sequence_to_parent.p1 = seq2p.p1;
      feature_context->sequence_to_parent.p2 = seq2p.p2;
#endif
      feature_context->parent_span.x1        = pspan.x1;
      feature_context->parent_span.x2        = pspan.x2;

      feature_context->parent_name           = server->current_segment->id;
#ifdef RDS_DONT_INCLUDE
      printf("|-----------------------------------------|\n");
      printf("|Parent is " ZMAP_DAS_FORMAT_SEGMENT "\n",
             (char *)g_quark_to_string( feature_context->parent_name ),
             feature_context->sequence_to_parent.p1,
             feature_context->sequence_to_parent.p2);
      printf("|child is " ZMAP_DAS_FORMAT_SEGMENT "\n",
             (char *)g_quark_to_string( feature_context->parent_name ),
             feature_context->sequence_to_parent.c1,
             feature_context->sequence_to_parent.c2);
#endif
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

static gboolean requestAndParseOverHTTP(DasServer server,
                                        requestParseDetail detail)
{
  gboolean response = FALSE, result = TRUE;
  ZMapDAS1QueryType type;
  char *url = NULL;

  zMapAssert(detail);

  if((url = detail->url) && !(*url))
    {
      result = FALSE;
      server->last_errmsg = g_strdup_printf("dasServer cannot continue with null url");
    }

  if(result)
    {
      server->request_type = type = detail->request_type;
      switch(server->request_type)
        {
        case ZMAP_DAS1_DSN:
          zMapDAS1ParserDSNPrepareXMLParser(server->das,
                                            detail->filter,
                                            detail->filter_data);
          break;
        case ZMAP_DAS1_DNA:
          zMapDAS1ParserDNAPrepareXMLParser(server->das,
                                            detail->filter,
                                            detail->filter_data);
          break;
        case ZMAP_DAS1_ENTRY_POINTS:
          zMapDAS1ParserEntryPointsPrepareXMLParser(server->das,
                                                    detail->filter,
                                                    detail->filter_data);
          break;
        case ZMAP_DAS1_TYPES:
          zMapDAS1ParserTypesPrepareXMLParser(server->das,
                                              detail->filter,
                                              detail->filter_data);
          break;
        case ZMAP_DAS1_FEATURES:
          zMapDAS1ParserFeaturesPrepareXMLParser(server->das,
                                                 detail->filter,
                                                 detail->filter_data);
          break ;
        case ZMAP_DAS1_STYLESHEET:
          zMapDAS1ParserStylesheetPrepareXMLParser(server->das,
                                                   detail->filter,
                                                   detail->filter_data);
          break ;
        case ZMAP_DAS1_LINK:
        case ZMAP_DAS1_SEQUENCE:
        default:
          server->last_errmsg  = g_strdup_printf("Unknown Das type %d",  server->request_type);
          server->request_type = ZMAP_DAS_UNKNOWN;
          result = FALSE;
          break ;
        }
    }

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
      server->chunks = 0;
      if(((server->curl_error =
           curl_easy_perform(server->curl_handle)) == CURLE_OK))
        {
          printf("Downloaded %d chunks from url %s\n", server->chunks, url);
          response = TRUE ;
        }
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

  /* Reset this now */
  server->request_type = ZMAP_DAS_UNKNOWN;

  return response;
}


#ifdef RDS_DONT_INCLUDE

/* We might get asked for a sequence which is quite a valid reference
 * sequence (clone/contig) that isn't listed in the entry_points query.
 * In order to get round this we must request all of the features of
 * category component from all of the segments listed in the entry_points
 * results which have subparts="yes".  These segments _MUST_ already be
 * a property of the dsn entry point.

 * The data is set up in the handler routine (see das1XMLhandlers.c)
 */

static gboolean searchAssembly(DasServer server, ZMapDAS1DSN dsn)
{
  char *asmURL = NULL;
  char    *tmp = NULL;
  gboolean res = FALSE;

  tmp  = zMapDAS1DSNRefSegmentsQueryString(dsn, ";");

  if(tmp && *(tmp))   /* Check we have a string */
    {
      char *tmpURL = NULL;
      tmpURL = dsnFullURL(server, "features?");
      asmURL = g_strdup_printf("%s%scategory=component;categorize=yes;feature_id=%s",
                               tmpURL, tmp,
                               g_quark_to_string(server->req_context->sequence_name));
      g_free(tmpURL);
    }

#ifdef RDS_DONT_INCLUDE
  if(!(res = requestAndParseOverHTTP(server, asmURL, ZMAP_DASONE_INTERNALFEATURES)))
    server->current_segment = NULL; /* Just in case something odd is going on */
#endif
  if(asmURL)
    g_free(asmURL);

  return res;
}

#endif


static void getFeatures4Aligns(gpointer key, gpointer data, gpointer userData)
{
  ZMapFeatureAlignment align = (ZMapFeatureAlignment)data;
  GetFeatures getFeaturesPtr = (GetFeatures)userData;

  if(getFeaturesPtr->result == ZMAP_SERVERRESPONSE_OK)

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    g_datalist_foreach(&(align->blocks), getFeatures4Blocks, (gpointer)getFeaturesPtr);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    g_hash_table_foreach(align->blocks, getFeatures4Blocks, (gpointer)getFeaturesPtr);


  return ;
}


static void getFeatures4Blocks(gpointer key, gpointer data, gpointer userData)
{
  ZMapFeatureBlock block = (ZMapFeatureBlock)data;
  GetFeatures getFeaturesPtr = (GetFeatures)userData;

  if (getFeaturesPtr->result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!fetchFeatures(getFeaturesPtr->server, getFeaturesPtr->styles, block))
        {
          getFeaturesPtr->result = ZMAP_SERVERRESPONSE_REQFAIL;
        }
    }

  return ;
}

static gboolean fetchFeatures(DasServer server, GHashTable *styles, ZMapFeatureBlock block)
{
  char *segStr = NULL, *query = NULL, *url = NULL;
  requestParseDetailStruct detail = {0};
  BlockServerStruct block_server  = {0};
  gboolean bad = FALSE;

  if((segStr = makeCurrentSegmentString(server, (ZMapFeatureContext)(block->parent->parent))) == NULL)
    {
      server->last_errmsg = "Failed to get current segment for url creation";
      bad = TRUE;
    }

  if(!bad && ((query = g_strdup_printf("features?%s", segStr)) == NULL))
    bad = TRUE;

  if(!bad && ((url = dsnFullURL(server, query)) == NULL))
    bad = TRUE;

  if(!bad)
    {
      block_server.server = server;
      block_server.block  = block;
      block_server.styles = styles ;

      detail.url          = url;
      detail.request_type = ZMAP_DAS1_FEATURES;
      detail.filter       = featureFilter;
      detail.filter_data  = &block_server;

      if(!(requestAndParseOverHTTP(server, &detail)))
        bad = TRUE;
      else
        fixUpFeaturesAndClearCache(server, block);
    }

  if(url)
    g_free(url);
  if(segStr)
    g_free(segStr);
  if(query)
    g_free(query);

  return !bad;
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
                              (char *)g_quark_to_string(das->current_segment->id),
                              fc->master_align->sequence_span.x1,
                              fc->master_align->sequence_span.x2);
    }

  return query;
}

static char *dsnRequestURL(DasServer server)
{
  char *url = NULL, *preDASPath = "";
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
  return url;
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

#ifdef RDS_DONT_INCLUDE
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
#endif

static void initialiseXMLParser(DasServer server)
{

  server->parser = zMapXMLParserCreate((void *)server, FALSE, server->debug);
  server->das    = zMapDAS1ParserCreate(server->parser);

  return ;
}

static gboolean getRequestedDSN(DasServer das, ZMapDAS1DSN *dsn_out)
{
  ZMapDAS1DSN dsn = NULL;
  gboolean exists = FALSE;
  char *pathstr = NULL, *dsn_str = NULL;
  GList *dsn_list = das->dsn_list;
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
  dsn_list = g_list_first(dsn_list);

  while(dsn_list)
    {
      dsn = (ZMapDAS1DSN)(dsn_list->data);
      if(want == dsn->source_id)
        {
          if(dsn_out != NULL)
            *dsn_out = dsn;
          exists = TRUE;
          break;
        }
      dsn_list = dsn_list->next;
    }

  if(!exists)
    das->last_errmsg = g_strdup_printf("DSN '%s' does not exist.", dsn_str);

  return exists;

}

static void dsnFilter        (ZMapDAS1DSN dsn,                gpointer user_data)
{
  DasServer server    = (DasServer)user_data;
  ZMapDAS1DSN new_dsn = NULL;
  gboolean  interesting_dsn = TRUE;

#ifdef RDS_EXAMPLE_OF_FILTERING
  if(dsn->source_id == g_quark_from_string("filtering invalid dsns"))
    interesting_dsn = FALSE;
#endif

  /* might as well cache these */
  if(interesting_dsn &&
     (new_dsn = g_new0(ZMapDAS1DSNStruct, 1)))
    {
      new_dsn->source_id = dsn->source_id;
      new_dsn->version   = dsn->version;
      new_dsn->name      = dsn->name;
      new_dsn->map       = dsn->map;
      new_dsn->desc      = dsn->desc;

      server->dsn_list   = g_list_prepend(server->dsn_list, new_dsn);
    }

  return ;
}

#ifdef RDS_DONT_INCLUDE
static void dnaFilter        (ZMapDAS1DNA dna,                gpointer user_data)
{
  DasServer server = (DasServer)user_data;
  printf("dnaFilter %p\n", (void *)server);
  return ;
}
#endif

static void entryFilter      (ZMapDAS1EntryPoint entry_point, gpointer user_data)
{
  DasServer server = (DasServer)user_data;
  ZMapDAS1DSN  dsn = NULL;
  ZMapDAS1Segment new_seg = NULL, seg = NULL;
  ZMapDAS1EntryPoint new_ep = NULL;

  /* This makes it slow! maybe we can speed up the getRequestedDSN function! */
  if(getRequestedDSN(server, &dsn))
    {
      if(entry_point->href || entry_point->version || !dsn->entry_point)
        {
          new_ep = g_new0(ZMapDAS1EntryPointStruct, 1);
          new_ep->href     = entry_point->href;
          new_ep->version  = entry_point->version;
          dsn->entry_point = new_ep;
        }

      if (entry_point->segments)
        {
          seg = (ZMapDAS1Segment)entry_point->segments->data;
          new_seg = g_new0(ZMapDAS1SegmentStruct, 1);
          new_seg->id          = seg->id;
          new_seg->type        = seg->type;
          new_seg->orientation = seg->orientation;
          new_seg->start       = seg->start;
          new_seg->stop        = seg->stop;
          dsn->entry_point->segments =
            g_list_prepend(dsn->entry_point->segments, new_seg);
        }
    }

  return ;
}
/* This isn't filtering, more just blindly creating... */
static void typesFilter      (ZMapDAS1Type type,              gpointer user_data)
{
  DasServerTypes server_types = (DasServerTypes)user_data;
  ZMapFeatureTypeStyle style  = NULL;
  GQuark tmp_quark;
  char *name, *desc, *outline, *fg, *bg;
  double width = 5.0;           /* pass sanity check in featuretypecreate */
  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;

  tmp_quark = type->type_id;
  name = (char *)g_quark_to_string(tmp_quark);
  desc = g_strdup_printf("DAS type '%s'", name);

  outline = fg = bg = NULL;

  /* Mode is currently hard-coded, don't know if das will address this. */
  style = zMapStyleCreate(name, desc) ;

  if (mode != ZMAPSTYLE_MODE_INVALID)
    zMapStyleSetMode(style, mode) ;

  zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, bg, fg, outline) ;

  zMapStyleSetWidth(style, width) ;

  g_hash_table_insert(server_types->output, GUINT_TO_POINTER(zMapStyleGetID(style)), (gpointer) style) ;

  return ;
}


static void featureFilter(ZMapDAS1Feature feature, gpointer user_data)
{
  BlockServer     block_server = (BlockServer)user_data;
  DasServer             server = NULL;
  ZMapFeatureTypeStyle   style = NULL;
  ZMapFeatureBlock       block = NULL;
  ZMapFeatureSet   feature_set = NULL;
  ZMapFeature      new_feature = NULL;
  ZMapStrand    feature_strand = ZMAPSTRAND_NONE;
  ZMapStyleMode feature_type = ZMAPSTYLE_MODE_INVALID;

  /* gdouble        feature_score = 0.0; */
  gboolean has_score = TRUE;
  GHashTable  *all_styles = NULL;
  char *feature_name = NULL,
    *short_ft_name   = NULL,
    *type_name       = NULL,
    *method_name     = NULL;
  GQuark feature_id, style_id, group_id;

  block  = block_server->block;
  server = block_server->server;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  all_styles = ((ZMapFeatureContext)(block->parent->parent))->styles;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  all_styles = block_server->styles ;

  type_name  = (char *)g_quark_to_string(feature->type_id);
  style_id   = zMapStyleCreateID(type_name);
  if(!(style = zMapFindStyle(all_styles, style_id)))
    {
      printf("Unable to find style %s\n", g_quark_to_string(feature->type_id));
    }
  method_name = (char *)g_quark_to_string(feature->method_id);

  /* make ZMapStrand, ZMapPhase, ZMapFeatureType and check for score */
  zMapFeatureFormatStrand((char *)g_quark_to_string(feature->orientation), &feature_strand);
  if(!(zMapFeatureFormatType(FALSE, FALSE, method_name, &feature_type)))
    {
      zMapLogWarning("Ignoring feature with type %s", method_name);
      return ;
    }

  /* Here we're using the first group id! If there are multiple then we need to work that out */
  if(feature->groups && (group_id = ((ZMapDAS1Group)(feature->groups->data))->group_id))
    {
      /* QUERY COORDS! */
      short_ft_name = (char *)g_quark_to_string(group_id);
      feature_name  = zMapFeatureCreateName(feature_type, short_ft_name,
                                            feature_strand, feature->start, feature->end, 0, 0);
      feature_id = g_quark_from_string(feature_name);


      if(!(new_feature = g_hash_table_lookup(server->feature_cache, GINT_TO_POINTER(group_id))))
        {
          new_feature = zMapFeatureCreateEmpty();

	  g_hash_table_insert(server->feature_cache, GINT_TO_POINTER(group_id), (gpointer)new_feature) ;

          zMapFeatureAddStandardData(new_feature, feature_name,
                                     short_ft_name, NULL, method_name,
                                     feature_type, style,
                                     feature->start, feature->end,
                                     has_score, feature->score,
                                     feature_strand) ;
        }
      else
        mergeWithGroupFeature(new_feature, feature, feature_type, group_id);
    }
  else
    {
      /* No group information for this feature */
      if(!(new_feature = g_hash_table_lookup(feature_set->features, GINT_TO_POINTER(group_id))))
        {
          new_feature = zMapFeatureCreateEmpty();

          if(zMapFeatureAddStandardData(new_feature, feature_name,
                                        short_ft_name, NULL, method_name,
                                        feature_type, style,
                                        feature->start, feature->end,
                                        has_score, feature->score,
                                        feature_strand))
            zMapFeatureSetAddFeature(feature_set, new_feature);
        }
    }

  return ;
}

static void applyGlyphToEachType(gpointer style_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data;
  ZMapDAS1Glyph glyph = (ZMapDAS1Glyph)user_data;
  char *outline = "black", *fg = NULL, *bg = NULL;

  if(glyph->fg)
    fg = (char *)g_quark_to_string(glyph->fg);
  if(glyph->bg)
    bg = (char *)g_quark_to_string(glyph->bg);

  zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL, bg, fg, outline);

  return ;
}

static void stylesheetFilter (ZMapDAS1Stylesheet style, gpointer user_data)
{
  DasServerTypes server_types = (DasServerTypes)user_data;
  ZMapDAS1Glyph glyph = NULL;   /* What we're interested in */
  ZMapDAS1StyleCategory category = NULL; /* we need it's id */
  ZMapDAS1StyleType type = NULL; /* We need it's id */
  GQuark default_id = g_quark_from_string("default");

  /* Get them out of the stylesheet */
  category = (ZMapDAS1StyleCategory)(style->categories->data);
  type     = (ZMapDAS1StyleType)(category->types->data);
  glyph    = (ZMapDAS1Glyph)(type->glyphs->data);

  if(category->id == default_id) /* apply to all categories */
    {
      if(type->id == default_id) /* apply to all types in all categories*/
	g_hash_table_foreach(server_types->output, applyGlyphToEachType, (gpointer)glyph);
      else                      /* apply to just the specific type in all categories */
        printf("[dasServer] Applying type '%s' to like types in all categories\n",
               (char *)g_quark_to_string(type->id));
    }
  else if(category->id)
    {
      printf("[dasServer] Looking up category '%s'\n",
             (char *)g_quark_to_string(category->id));
      if(type->id == default_id)
        printf("[dasServer] Apply to all type in category '%s'\n",
             (char *)g_quark_to_string(category->id));
      else
        printf("[dasServer] Applying to only type '%s' in category '%s'\n",
               (char *)g_quark_to_string(type->id),
               (char *)g_quark_to_string(category->id));
    }
  else
    zMapAssertNotReached();


  return ;
}

/* To merge a das feature (exon) which is part of a group (gene/transcript)
 * into the ZMapFeature (gene/transcript).
 * N.B. To save time sorting... these features need fixing later see
 *
 */
static gboolean mergeWithGroupFeature(ZMapFeature grp_feature,
                                      ZMapDAS1Feature das_sub_feature,
				      ZMapStyleMode feature_type,
                                      GQuark group_id)
{
  gboolean bad = FALSE;

  /* A few sanity checks.
   * feature types are equal and not basic
   * group ids match */
  if(!bad && grp_feature->type != feature_type)
    bad = TRUE;
  if(!bad && feature_type == ZMAPSTYLE_MODE_BASIC)
    bad = TRUE;
  if(!bad && group_id && grp_feature->original_id != group_id)
    bad = TRUE;

  /* if type looks like a gene we're ok and will believe it's a gene */
  if(!bad && feature_type == ZMAPSTYLE_MODE_TRANSCRIPT)
    {
      ZMapSpanStruct   exon = {0};

      if(!(grp_feature->feature.transcript.exons))
        {
          exon.x1 = grp_feature->x1;
          exon.x2 = grp_feature->x2;
          if(!(zMapFeatureAddTranscriptExonIntron(grp_feature, &exon, NULL)))
            bad = TRUE;
        }

      if(!bad)                  /* paranoia */
        {
          exon.x1 = das_sub_feature->start;
          exon.x2 = das_sub_feature->end;
          if(!(zMapFeatureAddTranscriptExonIntron(grp_feature, &exon, NULL)))
            bad = TRUE;
        }
    }

  if(!bad && feature_type == ZMAPSTYLE_MODE_ALIGNMENT)
    {
      zMapLogWarning("%s", "DAS server not managing alignments correctly.\n");
    }

  return bad;
}


static void fixFeatureCache(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeature        feature = (ZMapFeature)data;
  ZMapFeatureBlock     block = (ZMapFeatureBlock)user_data;
  ZMapFeatureSet feature_set = NULL;
  GQuark style_id = 0;
  GArray *exons;
  char *type_name = NULL;
  int exon_count  = 0, i = 0;

  if(feature->type == ZMAPSTYLE_MODE_TRANSCRIPT &&
     (exons = feature->feature.transcript.exons))
    {
      ZMapSpanStruct
        *first_exon = NULL,
        *last_exon  = NULL,
        intron      = {0};
      /* sort exons in transcripts */
      exon_count = exons->len;

      g_array_sort_with_data(exons, compareExonCoords, (gpointer)feature);

      /* fixes up feature->x1 & feature->x2 too! */
      first_exon = &g_array_index(exons, ZMapSpanStruct, 0);
      last_exon  = &g_array_index(exons, ZMapSpanStruct, exon_count - 1);

      feature->x1 = first_exon->x1;
      feature->x2 = last_exon->x2;

      /* create introns in transcripts */
      for(i = 0; i < exon_count - 1; i++)
        {
          first_exon = &g_array_index(exons, ZMapSpanStruct, i);
          last_exon  = &g_array_index(exons, ZMapSpanStruct, i+1);
          intron.x1  = first_exon->x2 + 1;
          intron.x2  = last_exon->x1  - 1;
          /* These will naturally be sorted as the exons were. */
          zMapFeatureAddTranscriptExonIntron(feature, NULL, &intron);
        }

      /* fix up feature->unique_id to reflect fixed x1 & x2 */
      feature->unique_id = zMapFeatureCreateID(feature->type,
                                               (char *)g_quark_to_string(feature->original_id),
                                               feature->strand,
                                               feature->x1,
                                               feature->x2,
                                               0, 0);
    }

  /* add feature to the correct set! Not just this block! To do this: */
  /* find the feature_set.  If it doesn't exist create it.
   * I haven't got time to work out why */
  style_id  = feature->style_id;
  type_name = (char *)g_quark_to_string(feature->style_id);

  if((feature_set = g_hash_table_lookup(block->feature_sets, GINT_TO_POINTER(style_id))) == NULL)
    {
      feature_set  = zMapFeatureSetCreate(type_name, NULL);
      zMapFeatureBlockAddFeatureSet(block, feature_set);
    }
  zMapFeatureSetAddFeature(feature_set, feature);

  return ;
}

static void fixUpFeaturesAndClearCache(DasServer server, ZMapFeatureBlock block)
{
  g_hash_table_foreach(server->feature_cache, fixFeatureCache, (gpointer)block);

  g_hash_table_destroy(server->feature_cache) ;

  return ;
}

static gint compareExonCoords(gconstpointer a, gconstpointer b, gpointer feature_data)
{
  ZMapSpan
    exon_a = (ZMapSpan)a,
    exon_b = (ZMapSpan)b;
  gint gt_lt_eq = 0;

  if(exon_a->x1 < exon_b->x1)
    {
      gt_lt_eq = -1;
    }
  else if(exon_a->x1 > exon_b->x1)
    {
      gt_lt_eq =  1;
    }
  else if(exon_a->x1 == exon_b->x1)
    gt_lt_eq = 0;


  /* negative value if a < b; zero if a = b; positive value if a > b. */
  return gt_lt_eq;
}
