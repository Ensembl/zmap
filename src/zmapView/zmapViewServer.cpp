/*  File: zmapViewServer.cpp
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



// TEMP WHILE I FIX THIS UP....some work required to move this out....
//
#include <zmapView_P.hpp>




#include <zmapViewServer.hpp>




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


typedef struct FindStylesStructType
{
  ZMapStyleTree *all_styles_tree ;
  GHashTable *all_styles_hash ;
  gboolean found_style ;
  GString *missing_styles ;
} FindStylesStruct, *FindStyles ;


typedef struct DrawableDataStructType
{
  char *config_file ;
  gboolean found_style ;
  GString *missing_styles ;
} DrawableDataStruct, *DrawableData ;




static gboolean dispatchContextRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any) ;
static gboolean processDataRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any) ;
static void freeDataRequest(ZMapServerReqAny req_any) ;

static GList *get_required_styles_list(GHashTable *srchash,GList *fsets) ;
static void mergeHashTableCB(gpointer key, gpointer value, gpointer user) ;
static gboolean haveRequiredStyles(ZMapStyleTree &all_styles, GList *required_styles, char **missing_styles_out) ;
static gboolean makeStylesDrawable(char *config_file, ZMapStyleTree &styles, char **missing_styles_out) ;
static void findStyleCB(gpointer data, gpointer user_data) ;
static void drawableCB(ZMapFeatureTypeStyle style, gpointer user_data) ;


/*
 *                  External interface functions.
 */

void zMapServerSetScheme(ZMapViewSessionServer server, ZMapURLScheme scheme)
{
  server->scheme = scheme ;

  return ;
}



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




/* Create/copy/delete load_features structs. */
LoadFeaturesData zMapServerCreateLoadFeatures(GList *feature_sets)
{
  LoadFeaturesData loaded_features = NULL ;

  loaded_features = g_new0(LoadFeaturesDataStruct, 1) ;

  if (feature_sets)
    loaded_features->feature_sets = g_list_copy(feature_sets) ;

  return loaded_features ;
}

LoadFeaturesData zMapServerCopyLoadFeatures(LoadFeaturesData loaded_features_in)
{
  LoadFeaturesData loaded_features = NULL ;

  loaded_features = g_new0(LoadFeaturesDataStruct, 1) ;

  *loaded_features = *loaded_features_in ;

  if (loaded_features_in->feature_sets)
    loaded_features->feature_sets = g_list_copy(loaded_features_in->feature_sets) ;

  return loaded_features ;
}

void zMapServerDestroyLoadFeatures(LoadFeaturesData loaded_features)
{
  if (loaded_features->feature_sets)
    g_list_free(loaded_features->feature_sets) ;

  if (loaded_features->err_msg)
    g_free(loaded_features->err_msg) ;

  g_free(loaded_features) ;

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
ZMapViewConnection zMapServerCreateViewConnection(ZMapView zmap_view,
                                               ZMapViewConnection view_con,
                                               ZMapFeatureContext context,
                                               char *server_url, char *format,
                                               int timeout, char *version,
                                               gboolean req_styles,
                                               char *styles_file,
                                               GList *req_featuresets,
                                               GList *req_biotypes,
                                               gboolean dna_requested,
                                               gint features_start, gint features_end,
                                               gboolean terminate)
{

  ZMapThread thread ;
  ConnectionData connect_data = NULL ;
  gboolean existing = FALSE;
  int url_parse_error ;
  ZMapURL urlObj;

  /* Parse the url and create connection. */
  if (!(urlObj = url_parse(server_url, &url_parse_error)))
    {
      zMapLogWarning("GUI: url %s did not parse. Parse error < %s >",
                     server_url, url_error(url_parse_error)) ;
      return(NULL) ;
    }

  if (view_con)
    {
      // use existing connection if not busy
      if (view_con->step_list)
        view_con = NULL ;
      else
        existing = TRUE ;

      //if (existing) printf("using existing connection %s\n",view_con->url);
    }

  if (!view_con)
    {
      /* Create the thread to service the connection requests, we give it a function that it will call
       * to decode the requests we send it and a terminate function. */
      if ((thread = zMapThreadCreate(zMapServerRequestHandler,
                                     zMapServerTerminateHandler, zMapServerDestroyHandler)))
        {
          /* Create the connection struct. */
          view_con = g_new0(ZMapViewConnectionStruct, 1) ;
          view_con->parent_view = zmap_view ;
          view_con->thread = thread ;
          view_con->url = g_strdup(server_url) ;
          //printf("create thread for %s\n",view_con->url);
          view_con->thread_status = THREAD_STATUS_PENDING;

          view_con->server = g_new0(ZMapViewSessionServerStruct, 1) ;

          zMapServerSetScheme(view_con->server, SCHEME_INVALID) ;
        }
      else
        {
          /* reporting an error here woudl be good
           * but the thread interafce does not apper to return its error code
           * and was written to exit zmap in case of failure
           * we need to pop up a messgae if (!view->thread_fail_silent)
           * and also reply to otterlace if active
           */
          return(NULL);
        }
    }

  if (view_con)
    {
      ZMapServerReqAny req_any;
      StepListActionOnFailureType on_fail = REQUEST_ONFAIL_CANCEL_THREAD;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      // take out as now not needed and besides didn't work
      if (terminate)
        on_fail = REQUEST_ONFAIL_CONTINUE;  /* to get pipe server external script status */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      view_con->curr_request = ZMAPTHREAD_REQUEST_EXECUTE ;

      /* Set up the connection data for this view connection. */
      view_con->request_data = connect_data = g_new0(ConnectionDataStruct, 1) ;

      connect_data->curr_context = context ;

      /* ie server->delayed -> called after startup */
      if (terminate)
        connect_data->dynamic_loading = TRUE ;

      connect_data->column_2_styles = zMap_g_hashlist_create() ;
      // better?      connect_data->column_2_styles = zmap_view->context_map.column_2_styles;

      connect_data->featureset_2_column = zmap_view->context_map.featureset_2_column;
      connect_data->source_2_sourcedata = zmap_view->context_map.source_2_sourcedata;

      // we need to save this to tell otterlace when we've finished
      // it also gets given to threads: when can we free it?
      connect_data->feature_sets = req_featuresets;
      //zMapLogWarning("request %d %s",g_list_length(req_featuresets),g_quark_to_string(GPOINTER_TO_UINT(req_featuresets->data)));


      /* well actually no it's not come kind of iso violation.... */
      /* the bad news is that these two little numbers have to tunnel through three distinct data
         structures and layers of s/w to get to the pipe scripts.  Originally the request
         coordinates were buried in blocks in the context supplied incidentally when requesting
         features after extracting other data from the server.  Obviously done to handle multiple
         blocks but it's another iso 7 violation  */

      connect_data->start = features_start;
      connect_data->end = features_end;

      /* likewise this has to get copied through a series of data structs */
      connect_data->sequence_map = zmap_view->view_sequence;

      connect_data->display_after = ZMAP_SERVERREQ_FEATURES ;

      /* This struct will needs to persist after the viewcon is gone so we can
       * return information to the layer above us about feature loading. */
      connect_data->loaded_features = zMapServerCreateLoadFeatures(NULL) ;


      view_con->step_list = zmapViewConnectionStepListCreate(dispatchContextRequests,
                                                             processDataRequests,
                                                             freeDataRequest);

      /* Record info. for this session. */
      zmapViewSessionAddServer(view_con->server, urlObj, format) ;


      /* Set up this connection in the step list. */
      if (!existing)
        {
          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_CREATE,
                                            zmap_view->view_sequence->config_file,
                                            urlObj, format, timeout, version) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_CREATE, req_any, on_fail) ;
          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_OPEN) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_OPEN, req_any, on_fail) ;
          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_GETSERVERINFO, req_any, on_fail) ;
        }

      //      if (req_featuresets)
      // need to request all if none specified
      {
        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURESETS, req_featuresets, req_biotypes, NULL) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_FEATURESETS, req_any, on_fail) ;

        if(req_styles || styles_file)
          {
            req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_STYLES, req_styles, styles_file && *styles_file ? styles_file : NULL) ;
            zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_STYLES, req_any, on_fail) ;
          }
        else
          {
            connect_data->curr_styles = &zmap_view->context_map.styles ;
          }

        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_NEWCONTEXT, context) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_NEWCONTEXT, req_any, on_fail) ;

        req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURES) ;
        zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_FEATURES, req_any, on_fail) ;
      }

      if (dna_requested)
        {
          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_SEQUENCE) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_SEQUENCE, req_any, on_fail) ;
          connect_data->display_after = ZMAP_SERVERREQ_SEQUENCE ;
          /* despite appearing before features in the GFF this gets requested afterwards */
        }

      if (terminate)
        {
          /* MH17 NOTE
           * These calls are here in the order they should be executed in for clarity
           * but the order chosen is defined in zmapViewConnectionStepListCreate()
           * this code could be reordered without any effect
           * the step list is operated as an array indexed by request type
           */
          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSTATUS) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_GETSTATUS, req_any, on_fail) ;
          connect_data->display_after = ZMAP_SERVERREQ_GETSTATUS;


          req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_TERMINATE) ;
          zmapViewStepListAddServerReq(view_con->step_list, view_con, ZMAP_SERVERREQ_TERMINATE, req_any, on_fail ) ;
        }

      if (!existing)
        zmap_view->connection_list = g_list_append(zmap_view->connection_list, view_con) ;

      /* Start the connection to the source. */
      zmapViewStepListIter(view_con) ;
    }
  else
    {
      zMapLogWarning("GUI: url %s looks ok (host: %s\tport: %d)"
                     "but could not connect to server.",
                     urlObj->url,
                     urlObj->host,
                     urlObj->port) ;
    }

  return view_con ;
}


/* Final destroy of a connection, by the time we get here the thread has
 * already been destroyed. */
void zMapServerDestroyViewConnection(ZMapView view, ZMapViewConnection view_conn)
{
  if (view->sequence_server == view_conn)
    view->sequence_server = NULL ;

  g_free(view_conn->url) ;

  if (view_conn->request_data)
    {
      /* There is more to do here but proceed carefully, sometimes parts of the code are still
       * referring to this because of asynchronous returns from zmapWindow and other code. */
      ConnectionData connect_data = (ConnectionData)(view_conn->request_data) ;

      if (connect_data->loaded_features)
        {
          zMapServerDestroyLoadFeatures(connect_data->loaded_features) ;

          connect_data->loaded_features = NULL ;
        }

      if (connect_data->error)
        {
          g_error_free(connect_data->error) ;
          connect_data->error = NULL ;
        }

      g_free(connect_data) ;
      view_conn->request_data = NULL ;
    }


  if (view_conn->step_list)
    {
      zmapViewStepListDestroy(view_conn->step_list) ;
      view_conn->step_list = NULL ;
    }

  /* Need to destroy the types array here.......errrrr...so why not do it ???? */

  zmapViewSessionFreeServer(view_conn->server) ;

  g_free(view_conn->server) ;

  g_free(view_conn) ;

  return ;
}















/*
 *            ZMapView external package-level functions.
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


/* Free all the session data, NOTE: we did not allocate the original struct
 * so we do not free it. */
void zmapViewSessionFreeServer(ZMapViewSessionServer server_data)
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





/*
 *               Internal routines.
 */



/* This is _not_ a generalised dispatch function, it handles a sequence of requests that
 * will end up fetching a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean dispatchContextRequests(ZMapViewConnection connection, ZMapServerReqAny req_any)
{
  gboolean result = TRUE ;
  ConnectionData connect_data = (ConnectionData)(connection->request_data) ;

//zMapLogWarning("%s: dispatch %d",connection->url, req_any->type);
  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      break;
    case ZMAP_SERVERREQ_OPEN:
      {
      ZMapServerReqOpen open = (ZMapServerReqOpen) req_any;

      open->sequence_map = connect_data->sequence_map;
      open->zmap_start = connect_data->start;
      open->zmap_end = connect_data->end;
      }
      break;
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {

        break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;

        feature_sets->featureset_2_stylelist_out = connect_data->column_2_styles ;

        /* MH17: if this is an output parameter why do we set it on dispatch?
         * beacuse it's a preallocated hash table
         */

        /* next 2 are outputs from ACE and inputs to pipeServers */
        feature_sets->featureset_2_column_inout = connect_data->featureset_2_column;
        feature_sets->source_2_sourcedata_inout = connect_data->source_2_sourcedata;

        break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

        /* required styles comes from featuresets call. */
        get_styles->required_styles_in = connect_data->required_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        ZMapServerReqNewContext new_context = (ZMapServerReqNewContext)req_any ;

        new_context->context = connect_data->curr_context ;

        break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        get_features->context = connect_data->curr_context ;
        get_features->styles = connect_data->curr_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        get_features->context = connect_data->curr_context ;
        get_features->styles = connect_data->curr_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_GETSTATUS:
      {
            break;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
      //ZMapServerReqTerminate terminate = (ZMapServerReqTerminate) req_any ;source_2_sourcedata_inout

      break ;
      }
    default:
      {
        zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;

        break ;
      }
    }

  result = TRUE ;

  return result ;
}



/* This is _not_ a generalised processing function, it handles a sequence of replies from
 * a thread that build up a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean processDataRequests(ZMapViewConnection view_con, ZMapServerReqAny req_any)
{
  gboolean result = TRUE ;
  ConnectionData connect_data = (ConnectionData)(view_con->request_data) ;
  ZMapView zmap_view = view_con->parent_view ;
  GList *fset;

  /* Process the different types of data coming back. */
  //printf("%s: response to %d was %d\n",view_con->url,req_any->type,req_any->response);
  //zMapLogWarning("%s: response to %d was %d",view_con->url,req_any->type,req_any->response);

  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      break;

    case ZMAP_SERVERREQ_OPEN:
      {
        break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)req_any ;

        connect_data->session = *get_info ;                    /* struct copy. */

        /* Hacky....what if there are several source names etc...we need a flag to say "master" source... */
        if (!(zmap_view->view_db_name))
          zmap_view->view_db_name = connect_data->session.database_name_out ;

        if (!(zmap_view->view_db_title))
          zmap_view->view_db_title = connect_data->session.database_title_out ;

        break ;
      }

    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;


        if (req_any->response == ZMAP_SERVERRESPONSE_OK)
          {
            /* Got the feature sets so record them in the server context. */
            connect_data->curr_context->req_feature_set_names = feature_sets->feature_sets_inout ;
          }
        else if (req_any->response == ZMAP_SERVERRESPONSE_UNSUPPORTED && feature_sets->feature_sets_inout)
          {
            /* If server doesn't support checking feature sets then just use those supplied. */
            connect_data->curr_context->req_feature_set_names = feature_sets->feature_sets_inout ;

            feature_sets->required_styles_out =
              get_required_styles_list(zmap_view->context_map.source_2_sourcedata,
                                       feature_sets->feature_sets_inout);
          }

        /* NOTE (mh17)
           tasked with handling a GFF file from the command line and no config..
           it became clear that this could not be done simply without reading each file twice
           and the plan moved to generating a config file with a perl script
           the idea being to read the GFF headers before running ZMap.
           But servers need a list of featuresets
           a) so that the request code could work out which servers to use
           b) so that we could precalculate featureset to column mapping and source to source data mapping
           (otherwise all data will be ignored and ZMap will abort from several places)
           which gives the crazy situation of having the read the entire file
           to extract all the featureset names so that we can read the entire file.
           NB files could be remote which is not ideal, and we expect some files to be large

           So i adapted the code to have featureset free servers
           (that can only be requested on startup - can't look one up by featureset if it's not defined)
           and hoped that i'd be able to patch this data in.
           There is a server protocol step to get featureset names
           but that would require processing subsequent steps to find out this information and
           the GFFparser rejects features from sources that have not been preconfigured.
           ie it's tied up in a knot

           Several parts of the display code have been patched to make up featureset to columns etc OTF
           which is in direct confrontation with the design of most of the server and display code,
           which explicitly assumes that this is predefined

           Well it sort of runs but really the server code needs a rewrite.
        */

#if 0
        // defaulted in GFFparser
        /* Not all servers will provide a mapping between feature sets and styles so we add
         * one now which is a straight mapping from feature set name to a style of the same name. */

        if (!(g_hash_table_size(feature_sets->featureset_2_stylelist_out)))
          {
            GList *sets ;

            sets = feature_sets->feature_sets_inout ;
            do
              {
                GQuark feature_set_id, feature_set_name_id;

                /* We _must_ canonicalise here. */
                feature_set_name_id = GPOINTER_TO_UINT(sets->data) ;

                feature_set_id = zMapFeatureSetCreateID((char *)g_quark_to_string(feature_set_name_id)) ;

                zMap_g_hashlist_insert(feature_sets->featureset_2_stylelist_out,
                                       feature_set_id,
                                       GUINT_TO_POINTER(feature_set_id)) ;
                //("processData f2s adds %s to %s\n",g_quark_to_string(feature_set_id),g_quark_to_string(feature_set_id));
              }
            while((sets = g_list_next(sets))) ;
          }
#endif


        /* not all servers provide a source to source data mapping
         * ZMap config can include this info but only if someone provides it
         * make sure there is an entry for each featureset from this server
         */
        for(fset = feature_sets->feature_sets_inout;fset;fset = fset->next)
          {
            ZMapFeatureSource src;
            GQuark fid = zMapStyleCreateID((char *) g_quark_to_string( GPOINTER_TO_UINT(fset->data)));

            if (!(src = (ZMapFeatureSource)g_hash_table_lookup(feature_sets->source_2_sourcedata_inout,GUINT_TO_POINTER(fid))))
              {
                GQuark src_unique_id ;

                // if entry is missing
                // allocate a new struct and add to the table
                src = g_new0(ZMapFeatureSourceStruct,1);

                src->source_id = GPOINTER_TO_UINT(fset->data);        /* may have upper case */
                src->source_text = src->source_id;
                src_unique_id = fid;
                //                src->style_id = fid;

                g_hash_table_insert(feature_sets->source_2_sourcedata_inout, GUINT_TO_POINTER(fid), src) ;
              }
            else
              {
                if(!src->source_id)
                  src->source_id = GPOINTER_TO_UINT(fset->data);
                if(!src->source_text)
                  src->source_text = src->source_id;
                //                if(!src->style_id)
                //                  src->style_id = fid;        defaults to this in zmapGFF-Parser.c
              }
          }

        /* I don't know if we need these, can get from context. */
        connect_data->feature_sets = feature_sets->feature_sets_inout ;
        connect_data->required_styles = feature_sets->required_styles_out ;


#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nadding stylelists:\n");
        zMap_g_hashlist_print(feature_sets->featureset_2_stylelist_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nview styles lists before merge:\n");
        zMap_g_hashlist_print(zmap_view->context_map.column_2_styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        /* Merge the featureset to style hashses. */
        zMap_g_hashlist_merge(zmap_view->context_map.column_2_styles, feature_sets->featureset_2_stylelist_out) ;

#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nview styles lists after merge:\n");
        zMap_g_hashlist_print(zmap_view->context_map.column_2_styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


        /* If the hashes aren't equal, we had to do a merge.  Need to free the server
         * created hash that will otherwise be left dangling... */
        if (zmap_view->context_map.column_2_styles != feature_sets->featureset_2_stylelist_out)
          {
            zMap_g_hashlist_destroy(feature_sets->featureset_2_stylelist_out);
            feature_sets->featureset_2_stylelist_out = NULL;
          }

        /* merge these in, the base mapping is defined in zMapViewIniGetData() */
        // NB these are not supplied by pipeServers and we assume a 1-1 mapping
        // (see above) of source to display featureset and source to style.
        // See also zmapViewRemoteReceive.c/xml_featureset_start_cb()

        if (!(zmap_view->context_map.featureset_2_column))
          {
            zmap_view->context_map.featureset_2_column = feature_sets->featureset_2_column_inout ;
          }
        else if (feature_sets->featureset_2_column_inout &&
                 zmap_view->context_map.featureset_2_column != feature_sets->featureset_2_column_inout)
          {
            //print_fset2col("merge view",zmap_view->context_map.featureset_2_column);
            //print_fset2col("merge inout",feature_sets->featureset_2_column_inout);
            g_hash_table_foreach(feature_sets->featureset_2_column_inout,
                                 mergeHashTableCB,zmap_view->context_map.featureset_2_column);
          }

        /* we get lower case column names from ACE
         * - patch in upppercased ones from other config if we have it
         * (otterlace should provide config for this)
         */
        {
          GList *iter;
          gpointer key,value;

          zMap_g_hash_table_iter_init(&iter,zmap_view->context_map.featureset_2_column);
          while(zMap_g_hash_table_iter_next(&iter,&key,&value))
            {
              ZMapFeatureSetDesc fset;
              ZMapFeatureSource fsrc;
              ZMapFeatureColumn column;

              fset = (ZMapFeatureSetDesc) value;

              /* construct a reverse col 2 featureset mapping */
              std::map<GQuark, ZMapFeatureColumn>::iterator map_iter = zmap_view->context_map.columns->find(fset->column_id) ;
              
              if(map_iter != zmap_view->context_map.columns->end())
                {
                  column = map_iter->second ;

                  fset->column_ID = column->column_id;      /* upper cased display name */

                  /* construct reverse mapping from column to featureset */
                  /* but don't add featuresets that get virtualised */
                  if(!g_list_find(column->featuresets_unique_ids,key))
                    {
                      fsrc = (ZMapFeatureSource)g_hash_table_lookup(zmap_view->context_map.source_2_sourcedata,key);
                      if(fsrc && !fsrc->maps_to)
                        {
                          /* NOTE this is an ordered list */
                          column->featuresets_unique_ids = g_list_append(column->featuresets_unique_ids,key);
                          /*! \todo #warning this code gets run for all featuresets for every server which is silly */
                        }
                    }
                }

              /* we get hundreds of these from ACE that are not in the config */
              /* and there's no capitalised names */
              /* we could hack in a capitaliser function but it would never be perfect */
              if(!fset->feature_src_ID)
                fset->feature_src_ID = GPOINTER_TO_UINT(key);
            }
        }

        if (!(zmap_view->context_map.source_2_sourcedata))
          {
            zmap_view->context_map.source_2_sourcedata = feature_sets->source_2_sourcedata_inout ;
          }
        else if(feature_sets->source_2_sourcedata_inout &&
                zmap_view->context_map.source_2_sourcedata !=  feature_sets->source_2_sourcedata_inout)
          {
            g_hash_table_foreach(feature_sets->source_2_sourcedata_inout,
                                 mergeHashTableCB,zmap_view->context_map.source_2_sourcedata);
          }


        //print_source_2_sourcedata("got featuresets",zmap_view->context_map.source_2_sourcedata);
        //print_fset2col("got featuresets",zmap_view->context_map.featureset_2_column);
        //print_col2fset("got columns",zmap_view->context_map.columns);

        // MH17: need to think about freeing _inout tables if in != out

        break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

        /* Merge the retrieved styles into the views canonical style list. */
        if(get_styles->styles_out)
          {
            char *missing_styles = NULL ;

#if 0
            pointless doing this if we have defaults
              code moved from zmapServerProtocolHandler.c
              besides we should test the view data which may contain global config

              char *missing_styles = NULL;

            /* Make sure that all the styles that are required for the feature sets were found.
             * (This check should be controlled from analysing the number of feature servers or
             * flags set for servers.....) */

            if (!haveRequiredStyles(get_styles->styles_out, get_styles->required_styles_in, &missing_styles))
              {
                *err_msg_out = g_strdup_printf("The following required Styles could not be found on the server: %s",
                                               missing_styles) ;
              }
            if(missing_styles)
              {
                g_free(missing_styles);           /* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */
              }
#endif

            zMapStyleMergeStyles(zmap_view->context_map.styles, get_styles->styles_out, ZMAPSTYLE_MERGE_PRESERVE) ;

            /* need to patch in sub style pointers after merge/ copy */
            zmap_view->context_map.styles.foreach(zMapStyleSetSubStyle, &zmap_view->context_map.styles) ;

            /* test here, where we have global and predefined styles too */

            if (!haveRequiredStyles(zmap_view->context_map.styles, get_styles->required_styles_in, &missing_styles))
              {
                zMapLogWarning("The following required Styles could not be found on the server: %s",missing_styles) ;
              }
            if(missing_styles)
              {
                g_free(missing_styles);           /* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */
              }
          }

        /* Store the curr styles for use in creating the context and drawing features. */
        //        connect_data->curr_styles = get_styles->styles_out ;
        /* as the styles in the window get replaced we need to have all of them not the new ones */
        connect_data->curr_styles = &zmap_view->context_map.styles ;

        break ;
      }

    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        break ;
      }

    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_GETSTATUS:
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        char *missing_styles = NULL ;
        /* features and getstatus combined as they can both display data */
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        if (req_any->response != ZMAP_SERVERRESPONSE_OK)
          result = FALSE ;

        if (result && req_any->type == ZMAP_SERVERREQ_FEATURES)
          {

            /* WHAT !!!!! THIS DOESN'T SOUND RIGHT AT ALL.... */
#if MH17_ADDED_IN_GFF_PARSER
            if (!(connect_data->server_styles_have_mode)
                && !zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(connect_data->curr_context),
                                                   zmap_view->context_map.styles))
              {
                     zMapLogWarning("Source %s, inferring Style modes from Features failed.",
                               view_con->url) ;

                result = FALSE ;
              }
#endif

            /* I'm not sure if this couldn't come much earlier actually....something
             * to investigate.... */
            if (!makeStylesDrawable(zmap_view->view_sequence->config_file,
                                    zmap_view->context_map.styles, &missing_styles))
              {
                zMapLogWarning("Failed to make following styles drawable: %s", missing_styles) ;

                result = FALSE ;
              }

            /* Why is this needed....to cache for the getstatus call ???? */
            connect_data->get_features = get_features ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* not ready for these yet.... */

            connect_data->exit_code = get_features->exit_code ;
            connect_data->stderr_out = get_features->stderr_out ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          }
        else if (result && req_any->type == ZMAP_SERVERREQ_GETSTATUS)
          {
            /* store the exit code and STDERR */
            ZMapServerReqGetStatus get_status = (ZMapServerReqGetStatus)req_any ;

            connect_data->exit_code = get_status->exit_code;
          }


        /* ok...once we are here we can display stuff.... */
        if (result && req_any->type == connect_data->display_after)
          {
            zmapViewSessionAddServerInfo(view_con->server, &(connect_data->session)) ;

            if (connect_data->get_features)  /* may be nul if server died */
              {

                /* Gosh...what a dereference.... */
                zMapStopTimer("LoadFeatureSet",
                              g_quark_to_string(GPOINTER_TO_UINT(connect_data->get_features->context->req_feature_set_names->data)));

                /* can't copy this list after getFeatures as it gets wiped */
                if (!connect_data->feature_sets)        /* (is autoconfigured server/ featuresets not specified) */
                  connect_data->feature_sets = g_list_copy(connect_data->get_features->context->src_feature_set_names) ;


                // NEEDS MOVING BACK INTO zmapView.c.....
                //
                /* Does the merge and issues the request to do the drawing. */
                result = zmapViewGetFeatures(zmap_view, connect_data->get_features, connect_data) ;



              }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* Is this the right place to decrement....dunno.... */
            /* we record succcessful requests, if some fail they will get zapped in checkstateconnections() */
            zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                   connect_data->feature_sets) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          }

        break ;
      }
      //    case ZMAP_SERVERREQ_GETSEQUENCE:
      // never appears?? - see commandCB() and processGetSeqRequests() in this file


    case ZMAP_SERVERREQ_TERMINATE:
      {
        break ;
      }

    default:
      {
        zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;
        result = FALSE ;

        break ;
      }
    }

  return result ;
}

static void freeDataRequest(ZMapServerReqAny req_any)
{
  zMapServerRequestDestroy(req_any) ;

  return ;
}


// get 1-1 mapping of featureset names to style id except when configured differently
static GList *get_required_styles_list(GHashTable *srchash,GList *fsets)
{
  GList *iter;
  GList *styles = NULL;
  ZMapFeatureSource src;
  gpointer key,value;

  zMap_g_hash_table_iter_init(&iter,srchash);
  while(zMap_g_hash_table_iter_next(&iter,&key,&value))
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(srchash, key);
      if(src)
        value = GUINT_TO_POINTER(src->style_id);
      styles = g_list_prepend(styles,value);
    }

  return(styles);
}



static void mergeHashTableCB(gpointer key, gpointer value, gpointer user)
{
  GHashTable *old = (GHashTable *) user;

  if(!g_hash_table_lookup(old,key))
    g_hash_table_insert(old,key,value);
}


// returns whether we have any of the needed styles and lists the ones we don't
static gboolean haveRequiredStyles(ZMapStyleTree &all_styles, GList *required_styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  FindStylesStruct find_data = {NULL} ;

  if(!required_styles)  // MH17: semantics -> don't need styles therefore have those that are required
    return(TRUE);

  find_data.all_styles_tree = &all_styles ;

  g_list_foreach(required_styles, findStyleCB, &find_data) ;

  if (find_data.missing_styles)
    *missing_styles_out = g_string_free(find_data.missing_styles, FALSE) ;

  result = find_data.found_style ;

  return result ;
}




static gboolean makeStylesDrawable(char *config_file, ZMapStyleTree &styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  DrawableDataStruct drawable_data = {NULL, FALSE, NULL} ;

  drawable_data.config_file = config_file ;

  styles.foreach(drawableCB, &drawable_data) ;

  if (drawable_data.missing_styles)
    *missing_styles_out = g_string_free(drawable_data.missing_styles, FALSE) ;

  result = drawable_data.found_style ;

  return result ;
}


/* GFunc()    */
static void findStyleCB(gpointer data, gpointer user_data)
{
  GQuark style_id = GPOINTER_TO_INT(data) ;
  FindStyles find_data = (FindStyles)user_data ;

  style_id = zMapStyleCreateID((char *)g_quark_to_string(style_id)) ;

  if (find_data->all_styles_hash && zMapFindStyle(find_data->all_styles_hash, style_id))
    find_data->found_style = TRUE ;
  else if (find_data->all_styles_tree && find_data->all_styles_tree->find(style_id))
    find_data->found_style = TRUE;
  else
    {
      if (!(find_data->missing_styles))
        find_data->missing_styles = g_string_sized_new(1000) ;

      g_string_append_printf(find_data->missing_styles, "%s ", g_quark_to_string(style_id)) ;
    }

  return ;
}




/* A callback to make the given style drawable. */
static void drawableCB(ZMapFeatureTypeStyle style, gpointer user_data)
{
  DrawableData drawable_data = (DrawableData)user_data ;

  if (style && zMapStyleIsDisplayable(style))
    {
      if (zMapStyleMakeDrawable(drawable_data->config_file, style))
        {
          drawable_data->found_style = TRUE ;
        }
      else
        {
          if (!(drawable_data->missing_styles))
            drawable_data->missing_styles = g_string_sized_new(1000) ;

          g_string_append_printf(drawable_data->missing_styles, "%s ", g_quark_to_string(style->unique_id)) ;
        }
    }

  return ;
}


