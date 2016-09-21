/*  File: zmapServerProtocolHandler.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>

#include <string.h>

/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <zmapServer_P.hpp>
#include <ZMap/zmapConfigIni.hpp>



/* Some protocols have global init/cleanup functions that must only be called once, this type/list
 * allows us to do this. */
typedef struct ZMapProtocolInitStructType
{
  int protocol;
  gboolean init_called ;
  void *global_init_data ;
  gboolean cleanup_called ;
} ZMapProtocolInitStruct, *ZMapProtocolInit ;

typedef struct
{
  pthread_mutex_t mutex ;                                    /* Protects access to init/cleanup list. */
  GList *protocol_list ;                                    /* init/cleanup list. */
} ZMapProtocolInitListStruct, *ZMapProtocolInitList ;



typedef struct DrawableStructType
{
  gboolean found_style ;
  GString *missing_styles ;
} DrawableStruct, *Drawable ;


static gboolean protocolGlobalInitFunc(ZMapProtocolInitList protocols, ZMapURL url,
                                       void **global_init_data) ;
static int findProtocol(gconstpointer list_protocol, gconstpointer protocol) ;


/* Set up the list, note the special pthread macro that makes sure mutex is set up before
 * any threads can use it. */
static ZMapProtocolInitListStruct protocol_init_G = {PTHREAD_MUTEX_INITIALIZER, NULL} ;

gboolean zmap_server_feature2style_debug_G = FALSE ;
gboolean zmap_server_styles_debug_G = FALSE ;




/*
 *                           External Interface
 */


// Variadic request creator, args depend on request type. Nothing is checked, in some weird way
// you just have to guess what to supply...better document this....!!
// 
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

  req_any = (ZMapServerReqAny)g_malloc0(size) ;


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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        create->format  = g_strdup(va_arg(args, char *)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        create->timeout = va_arg(args, int) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
        feature_sets->biotypes_inout = va_arg(args, GList *) ;
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        g_free(create->format) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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


// General request/reply routine to interface to zmapServer interface.
// Calls the appropriate server routine for the request and returns its reply.
// 
ZMapServerResponseType zMapServerRequest(ZMapServer *server_inout, ZMapServerReqAny request, char **err_msg_out)
{
  ZMapServerResponseType reply = ZMAP_SERVERRESPONSE_OK ;
  ZMapServer server = *server_inout ;                       // NULL on create connection call.
  void *global_init_data ;

  switch (request->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      {
        ZMapServerReqCreate create = (ZMapServerReqCreate)request ;

        /* Check if we need to call the global init function of the protocol, this is a
         * function that should only be called once, only need to do this when setting up a server. */
        if (!protocolGlobalInitFunc(&protocol_init_G, create->url, &global_init_data))
          {
            *err_msg_out = g_strdup_printf("Global Init failed for %s.", create->url->protocol) ;

            request->response = ZMAP_SERVERRESPONSE_REQFAIL ;
          }
        else
          {
            if ((request->response = zMapServerCreateConnection(&server, global_init_data, create->config_file,
                                                                create->url,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                                                                create->format, 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                                                                create->timeout,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                                                                create->version))

                != ZMAP_SERVERRESPONSE_OK)
              {
                *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
              }

            // Server always created here even if there is an error so we must always return it.
            *server_inout = server ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_OPEN:
      {
        ZMapServerReqOpen open = (ZMapServerReqOpen)request ;

        if ((request->response = zMapServerOpenConnection(server,open)) != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)request ;
        ZMapServerReqGetServerInfoStruct info = {ZMAP_SERVERREQ_INVALID} ;

        if ((request->response
             = zMapServerGetServerInfo(server, &info)) != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }
        else
          {
            get_info->data_format_out = info.data_format_out ;
            get_info->database_name_out = info.database_name_out  ;
            get_info->database_title_out = info.database_title_out  ;
            get_info->database_path_out = info.database_path_out  ;
            get_info->request_as_columns = info.request_as_columns  ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)request ;

        request->response = zMapServerFeatureSetNames(server,
                                                      &(feature_sets->feature_sets_inout),
                                                      &(feature_sets->biotypes_inout),
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
          }
        break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles styles = (ZMapServerReqStyles)request ;
        GHashTable *tmp_styles = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        // SORT THIS OUT.....

        /* Sets thread_rc and err_msg_out properly. */
        thread_rc = getStyles(server, styles, err_msg_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        /* If there's a styles file get the styles from that, otherwise get them from the source.
         * At the moment we don't merge styles from files and sources, perhaps we should...
         *
         * mgh: function modified to return all styles in file if style list not specified
         * pipe and file servers should not need to do this as zmapView will read the file anyway
         */

        if (styles->req_styles)
          {
            /* get from server */

            if ((styles->response = zMapServerGetStyles(server, &(styles->styles_out))) != ZMAP_SERVERRESPONSE_OK)
              {
                *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
              }
          }
        else if (styles->styles_file_in)
          {
            /* server specific file not global: return from thread */

            if (!zMapConfigIniGetStylesFromFile(server->config_file,
                                                styles->styles_list_in, styles->styles_file_in, &(styles->styles_out),
                                                NULL))
              {
                styles->response = ZMAP_SERVERRESPONSE_REQFAIL ;
                *err_msg_out = g_strdup_printf("Could not read types from styles file \"%s\"",
                                               styles->styles_file_in) ;
              }

            // Wonder how we know this !!
            styles->server_styles_have_mode = TRUE;
          }

        if (styles->response == ZMAP_SERVERRESPONSE_OK)
          {
            tmp_styles = styles->styles_out;

            /* Now we have all the styles do the inheritance for them all. */
            if (tmp_styles)
              {
                if(!zMapStyleInheritAllStyles(tmp_styles))
                  zMapLogWarning("%s", "There were errors in inheriting styles.") ;

                /* this is not effective as a subsequent style copy will not copy this internal data */
                zMapStyleSetSubStyles(tmp_styles);
              }
          }


        if (styles->response == ZMAP_SERVERRESPONSE_OK)
          {
            /* Find out if the styles will need to have their mode set from the features.
             * I'm feeling like this is a bit hacky because it's really an acedb issue. */
            /* mh17: these days ACE can be set to use a styles file */

            if (!(styles->styles_file_in))
              {
                if (zMapServerStylesHaveMode(server, &(styles->server_styles_have_mode))
                    != ZMAP_SERVERRESPONSE_OK)
                  {
                    styles->response = ZMAP_SERVERRESPONSE_REQFAIL ;
                    *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
                  }
              }
          }


        if (styles->response == ZMAP_SERVERRESPONSE_OK)
          {
            /* return the styles in the styles struct... */
            styles->styles_out = tmp_styles ;
          }


        break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        ZMapServerReqNewContext context = (ZMapServerReqNewContext)request ;

        /* Create a sequence context from the sequence and start/end data. */
        if ((request->response = zMapServerSetContext(server, context->context))
            != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request ;

        if (features->styles && (request->response = zMapServerGetFeatures(server, *features->styles, features->context))
            != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        ZMapServerReqGetFeatures features = (ZMapServerReqGetFeatures)request ;

        /* If DNA is one of the requested cols and there is an error report it, but not if its
         * just unsupported. */
        GList *req_sets = features->context->req_feature_set_names ;
        GQuark dna_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;
        GQuark threeft_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;
        GQuark showtrans_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

        /* We need to load the DNA if we are showing DNA, 3FT or ShowTranslation */
        if (zMap_g_list_find_quark(req_sets, dna_quark) ||
            zMap_g_list_find_quark(req_sets, threeft_quark) ||
            zMap_g_list_find_quark(req_sets, showtrans_quark))
          {
            if (features->styles)
              request->response = zMapServerGetContextSequences(server, *features->styles, features->context) ;
            else
              request->response = ZMAP_SERVERRESPONSE_REQFAIL ;

            if (request->response != ZMAP_SERVERRESPONSE_OK && request->response != ZMAP_SERVERRESPONSE_UNSUPPORTED)
              {
                *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
              }
          }

        break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
        ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)request ;

        if ((request->response = zMapServerGetSequence(server, get_sequence->sequences)) != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_GETSTATUS:
      {
        ZMapServerReqGetStatus get_status = (ZMapServerReqGetStatus)request ;

        if ((request->response = zMapServerGetStatus(server, &(get_status->exit_code))) != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_GETCONNECT_STATE:
      {
        ZMapServerReqGetConnectState get_connect_state = (ZMapServerReqGetConnectState)request ;

        if ((request->response = zMapServerGetConnectState(server, &(get_connect_state->connect_state)))
            != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ;
          }

        break ;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
        // this function should log and free the server whatever happens....not just leave it....

        if ((request->response = zMapServerCloseConnection(server)) != ZMAP_SERVERRESPONSE_OK)
          {
            *err_msg_out = g_strdup(zMapServerLastErrorMsg(server)) ; /* get error msg here because next call destroys the struct */
          }
        else
          {
            if ((request->response = zMapServerFreeConnection(server)) != ZMAP_SERVERRESPONSE_OK)
              {
                *err_msg_out = g_strdup("FreeConnectin of server failed.") ;
              }

            // Can no longer be sure of server struct state so don't reference it any more.
            server = NULL ;
            *server_inout = NULL ;
          }

        break ;
      }
    default:
      {
        zMapWarnIfReached() ;

        request->response = ZMAP_SERVERRESPONSE_BADREQ ;
        *err_msg_out = g_strdup_printf("Unknown request number: %d", request->type) ;

        break ;
      }
    }


  reply = request->response ;


  return reply ;
}



/*
 *                     Internal routines
 */



/* Static/global list of protocols and whether their global init/cleanup functions have been
 * called. These are functions that must only be called once. */
static gboolean protocolGlobalInitFunc(ZMapProtocolInitList protocols, ZMapURL url, void **global_init_data_out)
{
  gboolean result = FALSE ;
  int status = 0;
  GList *curr_ptr ;
  ZMapProtocolInit init = NULL ;
  int protocol = (int)(url->scheme);

  if ((status = pthread_mutex_lock(&(protocols->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "protocolGlobalInitFunc() mutex lock") ;
    }

  /* If we don't find the protocol in the list then add it, initialised to FALSE. */
  if ((curr_ptr = g_list_find_custom(protocols->protocol_list, &protocol, findProtocol)))
    {
      init = (ZMapProtocolInit)(curr_ptr->data) ;

      result = TRUE ;
    }
  else
    {
      void *global_init_data = NULL ;

      if (!zMapServerGlobalInit(url, &global_init_data))
        {
          zMapLogCritical("Initialisation call for %d protocol failed.", protocol) ;
        }
      else
        {
          init = (ZMapProtocolInit)g_new0(ZMapProtocolInitStruct, 1) ;
          init->protocol = protocol ;
          init->init_called = TRUE ;
          init->cleanup_called = FALSE ;
          init->global_init_data = NULL ;
          init->global_init_data = global_init_data ;

          protocols->protocol_list = g_list_prepend(protocols->protocol_list, init) ;

          result = TRUE ;
        }
    }

  if (result)
    *global_init_data_out = init->global_init_data ;

  if ((status = pthread_mutex_unlock(&(protocols->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "protocolGlobalInitFunc() mutex unlock") ;
    }

  return result ;
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
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapServerResponseType2ExactStr, ZMapServerResponseType, ZMAP_SERVER_RESPONSE_LIST) ;



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



