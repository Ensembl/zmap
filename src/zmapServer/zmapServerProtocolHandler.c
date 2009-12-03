/*  File: zmapServerProtocolHandler.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 * 	Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *
 * Description: 
 * Exported functions: See ZMap/zmapServerProtocol.h
 * HISTORY:
 * Last edited: Nov 26 08:26 2009 (edgrif)
 * Created: Thu Jan 27 13:17:43 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocolHandler.c,v 1.48 2009-12-03 15:03:08 mh17 Exp $
 *-------------------------------------------------------------------
 */


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
  GData *all_styles ;
  gboolean found_style ;
  GString *missing_styles ;
} FindStylesStruct, *FindStyles ;

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
static gboolean haveRequiredStyles(GData *all_styles, GList *required_styles, char **missing_styles_out) ;
static void findStyleCB(gpointer data, gpointer user_data) ;
static gboolean getStylesFromFile(char *styles_list, char *styles_file, GData **styles_out) ;
ZMapThreadReturnCode getStyles(ZMapServer server, ZMapServerReqStyles styles, char **err_msg_out) ;


/* Set up the list, note the special pthread macro that makes sure mutex is set up before
 * any threads can use it. */
static ZMapProtocolInitListStruct protocol_init_G = {PTHREAD_MUTEX_INITIALIZER, NULL} ;




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

	create->url = va_arg(args, ZMapURL) ;
	create->format  = g_strdup(va_arg(args, char *)) ;
	create->timeout = va_arg(args, int) ;
	create->version = g_strdup(va_arg(args, char *)) ;

	break ;
      }
    case ZMAP_SERVERREQ_OPEN:
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

	get_styles->styles_list_in = g_strdup(va_arg(args, char *)) ;
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
      {
	break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
	ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)req_any ;

	get_sequence->orig_feature = va_arg(args, ZMapFeature) ;
	get_sequence->sequences = va_arg(args, GList *) ;
	get_sequence->flags = va_arg(args, int) ;

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


void zMapServerCreateRequestDestroy(ZMapServerReqAny request)
{
  switch (request->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
	ZMapServerReqCreate create = (ZMapServerReqCreate)request ;

	g_free(create->format) ;
	g_free(create->version) ;

	break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
	ZMapServerReqStyles styles = (ZMapServerReqStyles)request ;

	g_free(styles->styles_list_in) ;
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

  switch (request->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
        ZMapServerReqCreate create = (ZMapServerReqCreate)request_in ;

	/* Check if we need to call the global init function of the protocol, this is a
	 * function that should only be called once, only need to do this when setting up a server. */
	protocolGlobalInitFunc(&protocol_init_G, create->url, &global_init_data) ;

	if ((request->response
	     = zMapServerCreateConnection(&server, global_init_data,
					  create->url, create->format, create->timeout, create->version))
	    != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;

	    thread_rc = ZMAPTHREAD_RETURNCODE_SERVERDIED ;
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
	if ((request->response = zMapServerOpenConnection(server)) != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;

	    thread_rc = ZMAPTHREAD_RETURNCODE_SERVERDIED ;
	  }

	break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)request_in ;
	ZMapServerInfoStruct info = {NULL} ;

	if ((request->response
	     = zMapServerGetServerInfo(server, &info)) != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	    thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	  }
	else
	  {
	    get_info->database_name_out = info.database_name ;
	    get_info->database_title_out = info.database_title ;
	    get_info->database_path_out = info.database_path ;
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
						      &(feature_sets->source_2_featureset_out)) ;


	if (request->response != ZMAP_SERVERRESPONSE_OK && request->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	    thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
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
	    && (request->response = zMapServerSetContext(server, context->context)
		!= ZMAP_SERVERRESPONSE_OK))
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	    thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	  }
	else
	  {
	    /* The start/end for the master alignment may have been specified as start = 1 and end = 0
	     * so we may need to fill in the start/end for the master align block. */
	    GHashTable *blocks = context->context->master_align->blocks ;
	    ZMapFeatureBlock block ;

	    zMapAssert(g_hash_table_size(blocks) == 1) ;

	    block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(blocks, 0)) ;
	    block->block_to_sequence.q1 = block->block_to_sequence.t1
	      = context->context->sequence_to_parent.c1 ;
	    block->block_to_sequence.q2 = block->block_to_sequence.t2
	      = context->context->sequence_to_parent.c2 ;
      
	    block->block_to_sequence.q_strand = block->block_to_sequence.t_strand = ZMAPSTRAND_FORWARD ;

	  }

	break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request_in ;

	if ((request->response = zMapServerGetFeatures(server, features->styles, features->context))
	    != ZMAP_SERVERRESPONSE_OK)
	  {
	    *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	    thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	  }

	break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request_in ;

	/* If DNA is one of the requested cols and there is an error report it, but not if its
	 * just unsupported. */
	if (zMap_g_list_find_quark(features->context->feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
	  {
	    request->response = zMapServerGetContextSequences(server, features->styles, features->context) ;

	    if (request->response != ZMAP_SERVERRESPONSE_OK && request->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
	      {
		*err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
		thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	      }
	  }

	break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
        ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)request_in ;

	thread_rc = getSequence(server, get_sequence, err_msg_out) ;
	break ;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
	thread_rc = terminateServer(&server, err_msg_out) ;
	break ;
      }
    default:
      zMapCheck(1, "Coding error, unknown request type number: %d", request->type) ;
      break ;
    }

  /* Return server. */
  *slave_data = (void *)server ;

  return thread_rc ;
}



/* This function is called if a thread terminates in some abnormal way (e.g. is cancelled),
 * it enables the server to clean up.  */
ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapAssert(slave_data) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = terminateServer(&server, err_msg_out) ;

  return thread_rc ;
}


/* This function is called if a thread terminates in some abnormal way (e.g. is cancelled),
 * it enables the server to clean up.  */
ZMapThreadReturnCode zMapServerDestroyHandler(void **slave_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;

  zMapAssert(slave_data) ;

  server = (ZMapServer)*slave_data ;

  thread_rc = destroyServer(&server) ;

  return thread_rc ;
}



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
  

 
/* Get a specific sequences from the server. */
static ZMapThreadReturnCode getSequence(ZMapServer server, ZMapServerReqGetSequence request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if ((request->response = zMapServerGetSequence(server, request->sequences))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
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

  if (zMapServerCloseConnection(*server) == ZMAP_SERVERRESPONSE_OK)
    {
      *server = NULL ;
    }
  else
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(*server)) ;
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



static gboolean haveRequiredStyles(GData *all_styles, GList *required_styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  FindStylesStruct find_data = {NULL} ; ;

  find_data.all_styles = all_styles ;

  g_list_foreach(required_styles, findStyleCB, &find_data) ;

  if (find_data.missing_styles)
    *missing_styles_out = g_string_free(find_data.missing_styles, FALSE) ;

  result = find_data.found_style ;

  return result ;
}


    /* GFunc()    */
static void findStyleCB(gpointer data, gpointer user_data)
{
  GQuark style_id = GPOINTER_TO_INT(data) ;
  FindStyles find_data = (FindStyles)user_data ;

  style_id = zMapStyleCreateID((char *)g_quark_to_string(style_id)) ;

  if ((zMapFindStyle(find_data->all_styles, style_id)))
    find_data->found_style = TRUE ;
  else
    {
      if (!(find_data->missing_styles))
	find_data->missing_styles = g_string_sized_new(1000) ;

      g_string_append_printf(find_data->missing_styles, "%s ", g_quark_to_string(style_id)) ;
    }

  return ;
}





static gboolean getStylesFromFile(char *styles_list, char *styles_file, GData **styles_out)
{
  gboolean result = FALSE ;
  GData *styles = NULL ;

  if ((styles = zMapFeatureTypeGetFromFile(styles_list, styles_file)))
    {
      *styles_out = styles ;

      result = TRUE ;
    }

  return result ;
}



ZMapThreadReturnCode getStyles(ZMapServer server, ZMapServerReqStyles styles, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  GData *tmp_styles = NULL ;
  char *missing_styles = NULL ;


  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      /* If there's a styles file get the styles from that, otherwise get them from the source.
       * At the moment we don't merge styles from files and sources, perhaps we should... */
      if (styles->styles_list_in && styles->styles_file_in)
	{
	  if (!getStylesFromFile(styles->styles_list_in, styles->styles_file_in, &(styles->styles_out)))
	    {
	      *err_msg_out = g_strdup_printf("Could not read types from styles file \"%s\"", styles->styles_file_in) ;
	      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	    }
	}
      else if ((styles->response = zMapServerGetStyles(server, &(styles->styles_out)) != ZMAP_SERVERRESPONSE_OK))
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}

      if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
	{
	  ZMapIOOut dest ;
	  char *string ;
	  gboolean styles_debug = FALSE;


	  /* PRINTOUT is changed to this now so need to write a short debug routine for all
	   * the style calls below...n.b. should print to a glib io buffer.... */
	  if(styles_debug)
	    {
	      dest = zMapOutCreateStr(NULL, 0) ;

	      zMapStyleSetPrintAll(dest, styles->styles_out, "Before merge", styles_debug) ;

	      string = zMapOutGetStr(dest) ;
	      
	      printf("%s\n", string) ;
	      
	      zMapOutDestroy(dest) ;
	    }


	  tmp_styles = styles->styles_out ;		    /* dummy code for now.... */

	  /* Some styles are predefined and do not have to be in the server,
	   * do a merge of styles from the server with these predefined ones. */
	  tmp_styles = zMapStyleGetAllPredefined() ;

	  tmp_styles = zMapStyleMergeStyles(tmp_styles, styles->styles_out, ZMAPSTYLE_MERGE_MERGE) ;

	  zMapStyleDestroyStyles(&(styles->styles_out)) ;


	  if(styles_debug)
	    {
	      dest = zMapOutCreateStr(NULL, 0) ;
	      
	      zMapStyleSetPrintAll(dest, tmp_styles, "Before inherit", styles_debug) ;

	      string = zMapOutGetStr(dest) ;

	      printf("%s\n", string) ;
	      
	      zMapOutDestroy(dest) ;
	    }

	  /* Now we have all the styles do the inheritance for them all. */
	  if (!zMapStyleInheritAllStyles(&(tmp_styles)))
	    zMapLogWarning("%s", "There were errors in inheriting styles.") ;


	  if(styles_debug)
	    {
	      dest = zMapOutCreateStr(NULL, 0) ;
	      
	      zMapStyleSetPrintAll(dest, tmp_styles, "After inherit", styles_debug) ;
	      
	      string = zMapOutGetStr(dest) ;
	      
	      printf("%s\n", string) ;
	      
	      zMapOutDestroy(dest) ;
	    }
	}
    }



  /* Make sure that all the styles that are required for the feature sets were found.
   * (This check should be controlled from analysing the number of feature servers or
   * flags set for servers.....) */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && !haveRequiredStyles(tmp_styles, styles->required_styles_in, &missing_styles))
    {
      *err_msg_out = g_strdup_printf("The following required Styles could not be found on the server: %s",
				     missing_styles) ;
      g_free(missing_styles) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else if(missing_styles)
    {
      g_free(missing_styles);	/* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */
    }

  /* Find out if the styles will need to have their mode set from the features.
   * I'm feeling like this is a bit hacky because it's really an acedb issue. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && !(styles->styles_file_in))
    {
      if (zMapServerStylesHaveMode(server, &(styles->server_styles_have_mode))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
    }

  /* return the styles in the styles struct... */
  styles->styles_out = tmp_styles ;


  return thread_rc ;
}




