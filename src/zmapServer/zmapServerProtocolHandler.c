/*  File: zmapServerProtocolHandler.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See ZMap/zmapServerProtocol.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

/* this file is a collection of code from zmapslave that needs to handle server connections...
 * note that the thread stuff does percolate into this file as we need mutex locks.... */

#include <strings.h>

/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapServerProtocol.h>
#include <zmapServer_P.h>
#include <ZMap/zmapConfigIni.h>



/*
Note that these are not even in the same order!

ZMAPTHREAD_RETURNCODE_OK, ZMAPTHREAD_RETURNCODE_TIMEDOUT,
            ZMAPTHREAD_RETURNCODE_REQFAIL, ZMAPTHREAD_RETURNCODE_BADREQ,
            ZMAPTHREAD_RETURNCODE_SERVERDIED, ZMAPTHREAD_RETURNCODE_QUIT

 _(ZMAP_SERVERRESPONSE_OK, , "ok", "", "")                              \
    _(ZMAP_SERVERRESPONSE_BADREQ, , "error in request args", "", "")          \
    _(ZMAP_SERVERRESPONSE_UNSUPPORTED, , "unsupported request", "", "")       \
    _(ZMAP_SERVERRESPONSE_REQFAIL, , "request failed", "", "")                \
    _(ZMAP_SERVERRESPONSE_TIMEDOUT, , "timed out", "", "")              \
    _(ZMAP_SERVERRESPONSE_SERVERDIED, , "server died", "", "")

ZMAPTHREAD_REPLY_INIT, ZMAPTHREAD_REPLY_WAIT, ZMAPTHREAD_REPLY_GOTDATA, ZMAPTHREAD_REPLY_REQERROR,
            ZMAPTHREAD_REPLY_DIED, ZMAPTHREAD_REPLY_CANCELLED, ZMAPTHREAD_REPLY_QUIT } ZMapThreadReply ;

*/




/* Some protocols have global init/cleanup functions that must only be called once, this type/list
 * allows us to do this. */
typedef struct
{
  int protocol;
  gboolean init_called ;
  void *global_init_data ;
  gboolean cleanup_called ;
} ZMapProtocolInitStruct, *ZMapProtocolInit ;

typedef struct
{
  pthread_mutex_t mutex ;				    /* Protects access to init/cleanup list. */
  GList *protocol_list ;				    /* init/cleanup list. */
} ZMapProtocolInitListStruct, *ZMapProtocolInitList ;



typedef struct
{
  gboolean found_style ;
  GString *missing_styles ;
} DrawableStruct, *Drawable ;


static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, ZMapURL url,
				   void **global_init_data) ;
static int findProtocol(gconstpointer list_protocol, gconstpointer protocol) ;
static ZMapThreadReturnCode getSequence(ZMapServer server, ZMapServerReqGetSequence request, char **err_msg_out) ;
static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static ZMapThreadReturnCode destroyServer(ZMapServer *server) ;
static ZMapThreadReturnCode getStyles(ZMapServer server, ZMapServerReqStyles styles, char **err_msg_out) ;
static ZMapThreadReturnCode getStatus(ZMapServer server, ZMapServerReqGetStatus request, char **err_msg_out);
static ZMapThreadReturnCode getConnectState(ZMapServer server,
					    ZMapServerReqGetConnectState request, char **err_msg_out);
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code) ;


/* Set up the list, note the special pthread macro that makes sure mutex is set up before
 * any threads can use it. */
static ZMapProtocolInitListStruct protocol_init_G = {PTHREAD_MUTEX_INITIALIZER, NULL} ;

gboolean zmap_server_feature2style_debug_G = FALSE ;
gboolean zmap_server_styles_debug_G = FALSE ;




/*
 *                           External Interface
 */


ZMapServerReqAny zMapServerRequestCreate(ZMapServerReqType request_type, ...)
{
  ZMapServerReqAny req_any = NULL ;
  va_list args ;
  int size = 0 ;


  /* Allocate struct. */
  switch (request_type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
	size = sizeof(ZMapServerReqCreateStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_OPEN:
      {
	size = sizeof(ZMapServerReqOpenStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
	size = sizeof(ZMapServerReqGetServerInfoStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
	size = sizeof(ZMapServerReqFeatureSetsStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	size = sizeof(ZMapServerReqStylesStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
	size = sizeof(ZMapServerReqNewContextStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
	size = sizeof(ZMapServerReqGetFeaturesStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      {
	size = sizeof(ZMapServerReqGetFeaturesStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
	size = sizeof(ZMapServerReqGetSequenceStruct) ;

	break ;
      }
    case ZMAP_SERVERREQ_GETSTATUS:
      {
      size = sizeof(ZMapServerReqGetStatusStruct) ;

      break ;
      }
    case ZMAP_SERVERREQ_GETCONNECT_STATE:
      {
      size = sizeof(ZMapServerReqGetConnectStateStruct) ;

      break ;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
      size = sizeof(ZMapServerReqTerminateStruct) ;

      break ;
      }
    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", request_type) ;
	break ;
      }
    }

  req_any = g_malloc0(size) ;


  /* Fill in the struct. */
  va_start(args, request_type) ;

  req_any->type = request_type ;

  switch (request_type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
	ZMapServerReqCreate create = (ZMapServerReqCreate)req_any ;

	create->config_file = g_strdup(va_arg(args, char *)) ;
	create->url = va_arg(args, ZMapURL) ;
	create->format  = g_strdup(va_arg(args, char *)) ;
	create->timeout = va_arg(args, int) ;
	create->version = g_strdup(va_arg(args, char *)) ;

	break ;
      }
    case ZMAP_SERVERREQ_OPEN:
      {
      ZMapServerReqOpen open = (ZMapServerReqOpen)req_any ;

      open->sequence_server = va_arg(args,gboolean);
      break;
      }

    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
	break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
	ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;

	feature_sets->feature_sets_inout = va_arg(args, GList *) ;
	feature_sets->sources = va_arg(args, GList *) ;

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

	get_styles->req_styles = va_arg(args, gboolean) ;
	get_styles->styles_file_in = g_strdup(va_arg(args, char *)) ;

	break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
	ZMapServerReqNewContext new_context = (ZMapServerReqNewContext)req_any ;

	new_context->context = va_arg(args, ZMapFeatureContext) ;

	break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_SEQUENCE:
    case ZMAP_SERVERREQ_GETSTATUS:
    case ZMAP_SERVERREQ_GETCONNECT_STATE:
    case ZMAP_SERVERREQ_TERMINATE:
      {
	break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
	ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;

	get_sequence->sequences = va_arg(args, GList *) ;
	get_sequence->caller_data = va_arg(args, void *) ;

	break ;
      }

    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", request_type) ;
	break ;
      }
    }

  va_end(args) ;

  return req_any ;
}


void zMapServerRequestDestroy(ZMapServerReqAny request)
{
  switch (request->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
	ZMapServerReqCreate create = (ZMapServerReqCreate)request ;

	g_free(create->config_file) ;
	g_free(create->format) ;
	g_free(create->version) ;

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	ZMapServerReqStyles styles = (ZMapServerReqStyles)request ;

	g_free(styles->styles_file_in) ;

	break ;
      }
    case ZMAP_SERVERREQ_OPEN:
    case ZMAP_SERVERREQ_GETSERVERINFO:
    case ZMAP_SERVERREQ_FEATURESETS:
    case ZMAP_SERVERREQ_NEWCONTEXT:
    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_SEQUENCE:
    case ZMAP_SERVERREQ_GETSEQUENCE:
    case ZMAP_SERVERREQ_GETSTATUS:
    case ZMAP_SERVERREQ_GETCONNECT_STATE:
    case ZMAP_SERVERREQ_TERMINATE:
      {
	break ;
      }
    default:
      {
	zMapLogFatalLogicErr("switch(), unknown value: %d", request->type) ;
	break ;
      }
    }

  g_free(request) ;

  return ;
}




/* This routine sits in the slave thread and is called by the threading interface to
 * service a request from the master thread which includes decoding it, calling the appropriate server
 * routines and returning the answer to the master thread. */
ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
                                              void *request_in, void **reply_out,
                                              char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqAny request = (ZMapServerReqAny)request_in ;
  ZMapServer server = NULL ;
  void *global_init_data ;

  /* Slightly opaque here, if *slave_data is NULL this implies that we are not set up yet
   * so the request should be to start a connection and we should have been passed in
   * a load of connection stuff.... */
  zMapAssert(request_in && reply_out && err_msg_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zMapAssert(!*slave_data && (request->type == ZMAP_SERVERREQ_CREATE || request->type == ZMAP_SERVERREQ_OPENLOAD)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* slave_data is NULL first time we are called. */
  if (*slave_data)
    server = (ZMapServer)*slave_data ;

#if MH_NEVER_INCLUDE_THIS_CODE
  if(*slave_data) zMapLogMessage("req %s/%s %d",server->url->protocol,server->url->query,request->type);
#endif

  switch (request->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
        ZMapServerReqCreate create = (ZMapServerReqCreate)request_in ;

	/* Check if we need to call the global init function of the protocol, this is a
	 * function that should only be called once, only need to do this when setting up a server. */
	protocolGlobalInitFunc(&protocol_init_G, create->url, &global_init_data) ;

	if ((request->response
	     = zMapServerCreateConnection(&server, global_init_data, create->config_file,
					  create->url, create->format, create->timeout, create->version))
	    != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;

	    thread_rc = thread_RC(request->response);
	  }
	else
	  {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    create->server = server ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  }

	break ;
      }
    case ZMAP_SERVERREQ_OPEN:
      {
	ZMapServerReqOpen open = (ZMapServerReqOpen)request_in ;

	if ((request->response = zMapServerOpenConnection(server,open)) != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;

	    thread_rc = thread_RC(request->response);
	  }

	break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)request_in ;
	ZMapServerReqGetServerInfoStruct info = {ZMAP_SERVERREQ_INVALID} ;

	if ((request->response
	     = zMapServerGetServerInfo(server, &info)) != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	    thread_rc = thread_RC(request->response);
	  }
	else
	  {
	    get_info->database_name_out = info.database_name_out  ;
	    get_info->database_title_out = info.database_title_out  ;
	    get_info->database_path_out = info.database_path_out  ;
	    get_info->request_as_columns = info.request_as_columns  ;
	  }

	break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)request_in ;

	request->response = zMapServerFeatureSetNames(server,
						      &(feature_sets->feature_sets_inout),
						      feature_sets->sources,
						      &(feature_sets->required_styles_out),
						      &(feature_sets->featureset_2_stylelist_out),
						      &(feature_sets->featureset_2_column_inout),
						      &(feature_sets->source_2_sourcedata_inout)) ;


	if (request->response == ZMAP_SERVERRESPONSE_OK)
	  {
	    if(zmap_server_feature2style_debug_G)       // OK status: only do this for active databases not files
	      {
		/* Print out the featureset -> styles lists. */
		zMap_g_hashlist_print(feature_sets->featureset_2_stylelist_out);
	      }
	  }
        else if(request->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	    thread_rc = thread_RC(request->response);
	  }
	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles styles = (ZMapServerReqStyles)request_in ;

	/* Sets thread_rc and err_msg_out properly. */
	thread_rc = getStyles(server, styles, err_msg_out) ;

	break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        ZMapServerReqNewContext context = (ZMapServerReqNewContext)request_in ;

	/* Create a sequence context from the sequence and start/end data. */
	if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
	    && ((request->response = zMapServerSetContext(server, context->context))
		!= ZMAP_SERVERRESPONSE_OK))
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	    thread_rc = thread_RC(request->response);
	  }
	else
	  {
#if MH17_BLOCK_COORDS_SET_FROM_SEQ_COORDS_IN_REQUEST_BY_SERVER_OR_GFF_HEADER
	    /* The start/end for the master alignment may have been specified as start = 1 and end = 0
	     * so we may need to fill in the start/end for the master align block. */
	    GHashTable *blocks = context->context->master_align->blocks ;
	    ZMapFeatureBlock block ;

	    zMapAssert(g_hash_table_size(blocks) == 1) ;

	    block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(blocks, 0)) ;
	    if(!block->block_to_sequence.t2)
	      {
		// mh17: this happens before getFeatures? which is where the data gets set
		// adding the if has no effect of course
		block->block_to_sequence.q1 = block->block_to_sequence.t1
		  = context->context->sequence_to_parent.c1 ;
		block->block_to_sequence.q2 = block->block_to_sequence.t2
		  = context->context->sequence_to_parent.c2 ;
	      }
	    block->block_to_sequence.q_strand = block->block_to_sequence.t_strand = ZMAPSTRAND_FORWARD ;
#endif
	  }

	break ;
      }

    case ZMAP_SERVERREQ_FEATURES:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request_in ;

	if ((request->response = zMapServerGetFeatures(server, features->styles, features->context))
	    != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;

	    thread_rc = thread_RC(request->response) ;
	  }

	break ;
      }

    case ZMAP_SERVERREQ_SEQUENCE:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request_in ;

	/* If DNA is one of the requested cols and there is an error report it, but not if its
	 * just unsupported. */
	if (zMap_g_list_find_quark(features->context->req_feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
	  {
	    request->response = zMapServerGetContextSequences(server, features->styles, features->context) ;

	    if (request->response != ZMAP_SERVERRESPONSE_OK && request->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
	      {
		*err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
		thread_rc = thread_RC(request->response);
	      }
	  }

	break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
        ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)request_in ;

	*err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;

	thread_rc = getSequence(server, get_sequence, err_msg_out) ;
	break ;
      }

    case ZMAP_SERVERREQ_GETSTATUS:
      {
        ZMapServerReqGetStatus get_status = (ZMapServerReqGetStatus)request_in ;

	thread_rc = getStatus(server, get_status, err_msg_out) ;
	break ;
      }

    case ZMAP_SERVERREQ_GETCONNECT_STATE:
      {
        ZMapServerReqGetConnectState get_connect_state = (ZMapServerReqGetConnectState)request_in ;

	thread_rc = getConnectState(server, get_connect_state, err_msg_out) ;
	break ;
      }

    case ZMAP_SERVERREQ_TERMINATE:
      {
        request->response = zMapServerCloseConnection(server);
        *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ; /* get error msg here because next call destroys the struct */

        if (request->response == ZMAP_SERVERRESPONSE_OK)
          {
            request->response = zMapServerFreeConnection(server) ;
            server = NULL;
          }
        
        if (request->response == ZMAP_SERVERRESPONSE_OK)
	  thread_rc = ZMAPTHREAD_RETURNCODE_QUIT ;
        else
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;     // server will likely evaporate and be detected

        break ;
      }

    default:
      zMapCheck(1, "Coding error, unknown request type number: %d", request->type) ;
      break ;
    }

  /* Return server. */
  *slave_data = (void *)server ;

#if  MH_NEVER_INCLUDE_THIS_CODE
  /* ERRR....so was this ever fixed ???????? EG */

  // mysteriously falls over on terminate (request = 11)
  if(request->type != 11)
    {
      char *emsg = "";
      
      if (err_msg_out && *err_msg_out)
	emsg = *err_msg_out;

      if(*slave_data)
	zMapLogMessage("req %s/%s req %d/%d returns %d (%s)", server->url->protocol,server->url->query,request->type,request->response,thread_rc,emsg);
    }
#endif

  return thread_rc ;
}



/* This function is called if a thread terminates.  see doc/Design_notes/slaveThread.shtml */
ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapAssert(slave_data) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = terminateServer(&server, err_msg_out) ;

  return thread_rc ;
}


/* This function is called if a thread terminates.  see doc/Design_notes/slaveThread.shtml */
ZMapThreadReturnCode zMapServerDestroyHandler(void **slave_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapAssert(slave_data) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = destroyServer(&server) ;

  return thread_rc ;
}



/*
 *                     Internal routines
 */



/* Static/global list of protocols and whether their global init/cleanup functions have been
 * called. These are functions that must only be called once. */
static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, ZMapURL url,
                                   void **global_init_data_out)
{
  int status = 0;
  GList *curr_ptr ;
  ZMapProtocolInit init ;
  int protocol = (int)(url->scheme);

  if ((status = pthread_mutex_lock(&(protocols->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "protocolGlobalInitFunc() mutex lock") ;
    }

  /* If we don't find the protocol in the list then add it, initialised to FALSE. */
  if (!(curr_ptr = g_list_find_custom(protocols->protocol_list, &protocol, findProtocol)))
    {
      init = (ZMapProtocolInit)g_new0(ZMapProtocolInitStruct, 1) ;
      init->protocol = protocol ;
      init->init_called = init->cleanup_called = FALSE ;
      init->global_init_data = NULL ;

      protocols->protocol_list = g_list_prepend(protocols->protocol_list, init) ;
    }
  else
    init = curr_ptr->data ;

  /* Call the init routine if its not been called yet and either way return the global_init_data. */
  if (!init->init_called)
    {
      if (!zMapServerGlobalInit(url, &(init->global_init_data)))
 	zMapLogFatal("Initialisation call for %d protocol failed.", protocol) ;

      init->init_called = TRUE ;
    }

  *global_init_data_out = init->global_init_data ;

  if ((status = pthread_mutex_unlock(&(protocols->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "protocolGlobalInitFunc() mutex unlock") ;
    }

  return ;
}



/* Enum -> String functions, these functions convert the enums _directly_ to strings.
 *
 * The functions all have the form
 *  const char *zMapXXXX2ExactStr(ZMapXXXXX type)
 *  {
 *    code...
 *  }
 *
 *  */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapServerReqType2ExactStr, ZMapServerReqType, ZMAP_SERVER_REQ_LIST) ;



/* Compare a protocol in the list with the supplied one. Just a straight string
 * comparison currently. */
static int findProtocol(gconstpointer list_data, gconstpointer custom_data)
{
  int result   = 0;
  int protocol = *((int *)(custom_data));

  /* Set result dependent on comparison */
  if (((ZMapProtocolInit)list_data)->protocol == protocol)
    {
      result = 0;
    }
  else if (((ZMapProtocolInit)list_data)->protocol > protocol)
    {
      result = -1;
    }
  else
    {
      result = 1;
    }

  return result ;
}


/* Get an exit code from the server. */
/* NOTE we always return OK, not SERVERDIED as the thread return code */
/* unlike the other requests */
static ZMapThreadReturnCode getStatus(ZMapServer server, ZMapServerReqGetStatus request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if ((request->response = zMapServerGetStatus(server, &request->exit_code, &request->stderr_out)) != ZMAP_SERVERRESPONSE_OK)
      {
        *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          thread_rc = thread_RC(request->response);
      }
      else
      {
        /* nothing to do... */
      }
    }

  return thread_rc ;
}


/* Get current connection state of server. */
static ZMapThreadReturnCode getConnectState(ZMapServer server,
					    ZMapServerReqGetConnectState request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if ((request->response = zMapServerGetConnectState(server, &(request->connect_state)))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	  thread_rc = thread_RC(request->response) ;
	}
      else
	{
	  /* nothing to do... */
	}
    }

  return thread_rc ;
}


/* Get a specific sequences from the server. */
static ZMapThreadReturnCode getSequence(ZMapServer server, ZMapServerReqGetSequence request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if ((request->response = zMapServerGetSequence(server, request->sequences))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          thread_rc = thread_RC(request->response);
	}
      else
	{
	  /* nothing to do... */
	}
    }

  return thread_rc ;
}



static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerConnectStateType connect_state = ZMAP_SERVERCONNECT_STATE_INVALID ;

  if ((zMapServerGetConnectState(*server, &connect_state) == ZMAP_SERVERRESPONSE_OK)
      && connect_state == ZMAP_SERVERCONNECT_STATE_CONNECTED
      && (zMapServerCloseConnection(*server) == ZMAP_SERVERRESPONSE_OK))
    {
      *server = NULL ;
    }
  else if (connect_state == ZMAP_SERVERCONNECT_STATE_UNCONNECTED)
    {
      *server = NULL ;
    }
  else
    {
      *err_msg_out = g_strdup(zMapServerLastErrorMsg(*server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}



static ZMapThreadReturnCode destroyServer(ZMapServer *server)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (zMapServerFreeConnection(*server) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  return thread_rc ;
}







static ZMapThreadReturnCode getStyles(ZMapServer server, ZMapServerReqStyles styles, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  GHashTable *tmp_styles = NULL ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      /* If there's a styles file get the styles from that, otherwise get them from the source.
       * At the moment we don't merge styles from files and sources, perhaps we should...
       *
       * mgh: function modified to return all styles in file if style list not specified
       * pipe and file servers should not need to do this as zmapView will read the file anyway
       */

	if(styles->req_styles)		/* get from server */
      {
        styles->response = zMapServerGetStyles(server, &(styles->styles_out));
            // unsupported eg for PIPE or FILE - zmapView will have loaded these anyway
        if(styles->response != ZMAP_SERVERRESPONSE_OK && styles->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
	  {
	    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	    thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
        }
	}
	else if (styles->styles_file_in)   /* server specific file not global: return from thread */
	{
	  if (!zMapConfigIniGetStylesFromFile(server->config_file, styles->styles_list_in, styles->styles_file_in, &(styles->styles_out), NULL))
	    {
	      *err_msg_out = g_strdup_printf("Could not read types from styles file \"%s\"", styles->styles_file_in) ;
	      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	    }
        styles->server_styles_have_mode = TRUE;
	}

      if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
	{
	  /* removed haveRequiredStyles() call to zmapView,c */

#if 0
	  /* Some styles are predefined and do not have to be in the server,
	   * do a merge of styles from the server with these predefined ones. */
// these are set in zMapViewConnect()
//	  tmp_styles = zMapStyleGetAllPredefined() ;

//	  tmp_styles = zMapStyleMergeStyles(tmp_styles, styles->styles_out, ZMAPSTYLE_MERGE_MERGE) ;

//	  zMapStyleDestroyStyles(styles->styles_out) ;
#else
	  tmp_styles = styles->styles_out;
#endif


	  /* Now we have all the styles do the inheritance for them all. */
	  if (tmp_styles)
	  {
		if(!zMapStyleInheritAllStyles(tmp_styles))
			zMapLogWarning("%s", "There were errors in inheriting styles.") ;

		zMapStyleSetSubStyles(tmp_styles); /* this is not effective as a subsequent style copy will not copy this internal data */
	  }

	}
    }


  if(styles->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
    {

      /* Find out if the styles will need to have their mode set from the features.
       * I'm feeling like this is a bit hacky because it's really an acedb issue. */
      /* mh17: these days ACE can be set to use a styles file */
      if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
	  && !(styles->styles_file_in))
	{
	  if (zMapServerStylesHaveMode(server, &(styles->server_styles_have_mode))
	      != ZMAP_SERVERRESPONSE_OK)
	    {
	      *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
	      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	    }
	}
    }


  /* return the styles in the styles struct... */
  styles->styles_out = tmp_styles ;

  return thread_rc ;
}



/* Return a thread return code for each type of server response. */
static ZMapThreadReturnCode thread_RC(ZMapServerResponseType code)
{
  ZMapThreadReturnCode thread_rc ;

  switch(code)
    {
    case ZMAP_SERVERRESPONSE_OK:
      thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
      break ;

    case ZMAP_SERVERRESPONSE_BADREQ:
      thread_rc = ZMAPTHREAD_RETURNCODE_BADREQ ;
      break ;

    case ZMAP_SERVERRESPONSE_REQFAIL:
    case ZMAP_SERVERRESPONSE_UNSUPPORTED:
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
      break ;

    case ZMAP_SERVERRESPONSE_TIMEDOUT:
    case ZMAP_SERVERRESPONSE_SERVERDIED:
      thread_rc = ZMAPTHREAD_RETURNCODE_SERVERDIED ;
      break ;

    default:
      zMapAssertNotReached() ;
      break ;
    }

  return thread_rc ;
}


