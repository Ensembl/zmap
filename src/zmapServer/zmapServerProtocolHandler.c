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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See ZMap/zmapServerProtocol.h
 * HISTORY:
 * Last edited: Nov  5 11:23 2008 (rds)
 * Created: Thu Jan 27 13:17:43 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocolHandler.c,v 1.31 2008-11-05 12:25:19 rds Exp $
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
static ZMapThreadReturnCode openServerAndLoad(ZMapServerReqOpenLoad request, ZMapServer *server_out,
					      char **err_msg_out, void *global_init_data) ;
static ZMapThreadReturnCode getSequence(ZMapServer server, ZMapServerReqGetSequence request, char **err_msg_out) ;
static ZMapThreadReturnCode getServerInfo(ZMapServer server, ZMapServerReqGetServerInfo request, char **err_msg_out) ;
static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;
static gboolean haveRequiredStyles(GData *all_styles, GList *required_styles, char **missing_styles_out) ;
static void findStyleCB(gpointer data, gpointer user_data) ;
static gboolean getStylesFromFile(char *styles_list, char *styles_file, GData **styles_out) ;

static gboolean makeStylesDrawable(GData *styles, char **missing_styles_out) ;
static void drawableCB(GQuark key_id, gpointer data, gpointer user_data) ;


/* Set up the list, note the special pthread macro that makes sure mutex is set up before
 * any threads can use it. */
static ZMapProtocolInitListStruct protocol_init_G = {PTHREAD_MUTEX_INITIALIZER, NULL} ;



/* This routine sits in the slave thread and is called by the threading interface to
 * service a request from the master thread which includes decoding it, calling the appropriate server
 * routines and returning the answer to the master thread. */
ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
                                              void *request_in, void **reply_out,
                                              char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServerReqAny request = (ZMapServerReqAny)request_in ;
  ZMapServer server ;
  void *global_init_data ;


  zMapAssert(request_in && reply_out && err_msg_out) ;     /* slave_data is NULL first time we
                                                              are called. */

  if (slave_data)
    server = (ZMapServer)*slave_data ;
  else
    server = NULL ;

  /* Slightly opaque here, if server is NULL this implies that we are not set up yet
   * so the request should be to start a connection and we should have been passed in
   * a load of connection stuff.... */
  zMapAssert((!server && (request->type == ZMAP_SERVERREQ_OPEN
			  || request->type == ZMAP_SERVERREQ_OPENLOAD))
	     || (server && (request->type == ZMAP_SERVERREQ_GETSEQUENCE))) ;


  switch(request->type)
    {
    case ZMAP_SERVERREQ_OPENLOAD:
      {
        ZMapServerReqOpenLoad open_load = (ZMapServerReqOpenLoad)request_in ;
        
        /* Check if we need to call the global init function of the protocol, this is a
         * function that should only be called once, only need to do this when setting up a server. */
        protocolGlobalInitFunc(&protocol_init_G, open_load->open.url, &global_init_data) ;

        thread_rc = openServerAndLoad(open_load, &server, err_msg_out, global_init_data) ;

        break ;
      }
    case ZMAP_SERVERREQ_GETSEQUENCE:
      {
        ZMapServerReqGetSequence get_sequence = (ZMapServerReqGetSequence)request_in ;

	thread_rc = getSequence(server, get_sequence, err_msg_out) ;
	break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)request_in ;

	thread_rc = getServerInfo(server, get_info, err_msg_out) ;
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

/* Compare a protocol in the list with the supplied one. Just a straight string
 * comparison currently. */
static int findProtocol(gconstpointer list_data, gconstpointer custom_data)
{
  int result   = 0;
  int protocol = *((int *)(custom_data));

  /* Set result dependent on comparison */
  if(((ZMapProtocolInit)list_data)->protocol == protocol)
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
  
 
static ZMapThreadReturnCode openServerAndLoad(ZMapServerReqOpenLoad request, ZMapServer *server_out,
					      char **err_msg_out, void *global_init_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;
  ZMapServerReqOpen open = &request->open ;
  ZMapServerReqGetServerInfo get_info = &request->get_info ;
  ZMapServerReqStyles styles = &request->styles ;
  ZMapServerReqFeatureSets feature_sets = &request->feature_sets ;
  ZMapServerReqNewContext context = &request->context ;
  ZMapServerReqGetFeatures features = &request->features ;
  GList *required_styles = NULL ;
  char *missing_styles = NULL ;

  /* Create the thread block for this server connection. */
  if (!zMapServerCreateConnection(&server, global_init_data,
				  open->url,
                                  open->format, open->timeout, 
                                  open->version))
    {
      *err_msg_out = zMapServerLastErrorMsg(server) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  /* Open the connection. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && zMapServerOpenConnection(server) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  /* Get server information (version etc). */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && zMapServerGetServerInfo(server, &(get_info->database_path_out)) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  /* Check the feature set name list, or if the list is NULL get all feature set names.
   * I think we should not allow NULL for the feature set names, caller should have to say.... */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      /* We could return a set of required styles here....to use below in checking the styles we need. */

      if (zMapServerFeatureSetNames(server, &(feature_sets->feature_sets), &required_styles)
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
      else
	{
	  /* Got the feature sets so record them in the server context. */
	  context->context->feature_set_names = feature_sets->feature_sets ;
	}
    }


  /* Get the styles, they may come from a file or from the source itself. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      /*  I THINK WE NEED TO GET THE PREDEFINED HERE FIRST AND THEN MERGE.... */

      /* If there's a styles file get the styles from that, otherwise get them from the source.
       * At the moment we don't merge styles from files and sources, perhaps we should... */
      if (styles->styles_list && styles->styles_file)
	{
	  if (!getStylesFromFile(styles->styles_list, styles->styles_file, &(styles->styles)))
	    {
	      *err_msg_out = g_strdup_printf("Could not read types from styles file \"%s\"", styles->styles_file) ;
	      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	    }
	}
      else if (zMapServerGetStyles(server, &(styles->styles)) != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else
	{
	  *err_msg_out = g_strdup("No styles available.") ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMapFeatureTypePrintAll(styles->styles, "Before merge") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* Some styles are predefined and do not have to be in the server,
	   * do a merge of styles from the server with these predefined ones. */
	  context->context->styles = zMapStyleGetAllPredefined() ;

	  context->context->styles = zMapStyleMergeStyles(context->context->styles, styles->styles) ;

	  zMapStyleDestroyStyles(&(styles->styles)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMapFeatureTypePrintAll(context->context->styles, "Before inherit") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* Now we have all the styles do the inheritance for them all. */
	  if (!zMapStyleInheritAllStyles(&(context->context->styles)))
	    zMapLogWarning("%s", "There were errors in inheriting styles.") ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMapFeatureTypePrintAll(context->context->styles, "After inherit") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	}
    }



  /* Make sure that all the styles that are required for the feature sets were found.
   * (This check should be controlled from analysing the number of feature servers or
   * flags set for servers.....) */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && !haveRequiredStyles(context->context->styles, required_styles, &missing_styles))
    {
      *err_msg_out = g_strdup_printf("The following required Styles could not be found on the server: %s",
				     missing_styles) ;
      g_free(missing_styles) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
  else if(missing_styles)
    g_free(missing_styles);	/* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */

  /* Find out if the styles will need to have their mode set from the features.
   * I'm feeling like this is a bit hacky because it's really an acedb issue. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && !(styles->styles_file))
    {
      if (zMapServerStylesHaveMode(server, &(styles->server_styles_have_mode))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
    }



  /* Create a sequence context from the sequence and start/end data. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && zMapServerSetContext(server, context->context)
      != ZMAP_SERVERRESPONSE_OK)
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


  /* Get the features if requested. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && (features->type == ZMAP_SERVERREQ_FEATURES
	  || features->type == ZMAP_SERVERREQ_FEATURE_SEQUENCE)
      && zMapServerGetFeatures(server, context->context) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* We need to free feature_context here, its subparts should have been freed by routines
       * we call....... */
      g_free(feature_context) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* Get the actual dna sequence for all aligns/blocks if it was requested. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && (features->type == ZMAP_SERVERREQ_SEQUENCE
	  || features->type == ZMAP_SERVERREQ_FEATURE_SEQUENCE)
      && (zMap_g_list_find_quark(context->context->feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME)))
      && zMapServerGetContextSequences(server, context->context) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* We need to free feature_context here, its subparts should have been freed by routines
       * we call....... */
      g_free(feature_context) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* If the style modes need to be set from features then do that now. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK && !(styles->server_styles_have_mode))
    {
      if (!zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(context->context)))
	{
	  *err_msg_out = g_strdup_printf("Inferring Style modes from Features failed.") ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapFeatureTypePrintAll(context->context->styles, "After zMapFeatureAnyAddModesToStyles") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }


  /* Make styles drawable.....if they are set to displayable..... */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (!makeStylesDrawable(context->context->styles, &missing_styles))
	{
	  *err_msg_out = g_strdup_printf("Failed to make following styles drawable: %s", missing_styles) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zMapFeatureTypePrintAll(context->context->styles, "After makeStylesDrawable") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }




  /* error handling...if there is a server we should get rid of it....and sever connection if
   * required.... */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      *server_out = server ;

      features->feature_context_out = context->context ;
    }

  return thread_rc ;
}


/* Get a specific sequences from the server. */
static ZMapThreadReturnCode getSequence(ZMapServer server, ZMapServerReqGetSequence request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (zMapServerGetSequence(server, request->sequences)
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




/* Get a specific sequences from the server. */
static ZMapThreadReturnCode getServerInfo(ZMapServer server, ZMapServerReqGetServerInfo request, char **err_msg_out)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (zMapServerGetServerInfo(server, &(request->database_path_out))
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

  if (zMapServerCloseConnection(*server)
      && zMapServerFreeConnection(*server))
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (zMapFileAccess(styles_file, "r") && !(styles = zMapFeatureTypeGetFromFile(styles_list, styles_file)))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if ((styles = zMapFeatureTypeGetFromFile(styles_list, styles_file)))
    {
      *styles_out = styles ;

      result = TRUE ;
    }

  return result ;
}



static gboolean makeStylesDrawable(GData *styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  DrawableStruct drawable_data = {FALSE} ;

  g_datalist_foreach(&styles, drawableCB, &drawable_data) ;

  if (drawable_data.missing_styles)
    *missing_styles_out = g_string_free(drawable_data.missing_styles, FALSE) ;

  result = drawable_data.found_style ;

  return result ;
}



/* A GDataForeachFunc() to make the given style drawable. */
static void drawableCB(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;
  Drawable drawable_data = (Drawable)user_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("%s\n", zMapStyleGetName(style)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Should we do the drawable bit here ??? I think so probably..... */
  /* Should check for "no_display" once we support that....and only do this if it's not set... */
  if (zMapStyleIsDisplayable(style))
    {
      if (zMapStyleMakeDrawable(style))
	{
	  drawable_data->found_style = TRUE ;
	}
      else
	{
	  if (!(drawable_data->missing_styles))
	    drawable_data->missing_styles = g_string_sized_new(1000) ;

	  g_string_append_printf(drawable_data->missing_styles, "%s ", g_quark_to_string(key_id)) ;
	}
    }

  return ;
}
