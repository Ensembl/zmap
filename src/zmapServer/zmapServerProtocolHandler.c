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
 * Last edited: Apr 18 08:10 2007 (edgrif)
 * Created: Thu Jan 27 13:17:43 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocolHandler.c,v 1.19 2007-04-18 09:29:36 edgrif Exp $
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




static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, zMapURL url,
				   void **global_init_data) ;
static int findProtocol(gconstpointer list_protocol, gconstpointer protocol) ;


static ZMapThreadReturnCode openServerAndLoad(ZMapServerReqOpenLoad request, ZMapServer *server_out,
					      char **err_msg_out, void *global_init_data) ;

static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;

static gboolean addModesToStyles(ZMapFeatureContext context) ;
static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id, 
						 gpointer data, 
						 gpointer user_data,
						 char **error_out) ;
static void addFeatureModeCB(GQuark key_id, gpointer data, gpointer user_data) ;

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
  zMapAssert(!server && (request->type == ZMAP_SERVERREQ_OPEN
                         || request->type == ZMAP_SERVERREQ_OPENLOAD)) ;


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
    case ZMAP_SERVERREQ_TERMINATE:
      thread_rc = terminateServer(&server, err_msg_out) ;
      break ;
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
static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, zMapURL url,
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
  ZMapServerReqStyles styles = &request->styles ;
  ZMapServerReqFeatureSets feature_sets = &request->feature_sets ;
  ZMapServerReqNewContext context = &request->context ;
  ZMapServerReqGetFeatures features = &request->features ;


  /* Create the thread block for this specific server thread. */
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


  /* We may have been supplied with the styles by the caller, if not then retrieve all
   * available styles from the server. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (!(styles->styles))
	{
	  if (zMapServerGetStyles(server, &(styles->styles))
	      != ZMAP_SERVERRESPONSE_OK)
	    {
	      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	    }
	  else
	    {
	      /* Some styles are predefined and do not have to be in the server,
	       * do a merge of styles from the server with these predefined ones. */
	      context->context->styles = zMapStyleGetAllPredefined() ;

	      context->context->styles = zMapStyleMergeStyles(context->context->styles, styles->styles) ;

	      zMapStyleDestroyStyles(styles->styles) ;
	      styles->styles = NULL ;

	      /* Now we have all the styles do the inheritance for them all. */
	      if (!zMapStyleInheritAllStyles(&(context->context->styles)))
		zMapLogWarning("%s", "There were errors in inheriting styles.") ;
	    }
	}
    }


  /* Find out if the styles will need to have their mode set from the features. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (zMapServerStylesHaveMode(server, &(styles->server_styles_have_mode))
	  != ZMAP_SERVERRESPONSE_OK)
	{
	  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
    }


  /* If there is no feature set list then get the list of all feature sets from the server. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      if (!(feature_sets->feature_sets))
	{
	  if (zMapServerGetFeatureSets(server, &(feature_sets->feature_sets))
	      != ZMAP_SERVERRESPONSE_OK)
		{
		  *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
		  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
		}
	      else
		{
		  /* Got the types so record them in the server context. */
		  context->context->feature_set_names = feature_sets->feature_sets ;
		}
	}
    }




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* WE SHOULD BE DOING THIS OTHERWISE HOW CAN BE SURE WE CAN DO THE DISPLAY ? BUT ON THE
   * OTHER HAND WHAT IF STYLES FROM ANOTHER SERVER ???? */

  /* I've removed a whole load of acedb-centric stuff here but I've left this as a reminder that
     we may wish to check feature_sets/styles more carefully at some stage. */

  if (!zMapSetListEqualStyles(&(context->context->feature_set_names),
			      &(context->context->styles)))
    {
      *err_msg_out = g_strdup("The styles available from the server do not match"
			      " the list of required styles.") ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



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

      GData *blocks = context->context->master_align->blocks ;
      ZMapFeatureBlock block ;

      zMapAssert(zMap_g_datalist_length(&blocks) == 1) ;

      block = (ZMapFeatureBlock)(zMap_g_datalist_first(&blocks));
      block->block_to_sequence.q1 = block->block_to_sequence.t1
	= context->context->sequence_to_parent.c1 ;
      block->block_to_sequence.q2 = block->block_to_sequence.t2
	= context->context->sequence_to_parent.c2 ;
      
      block->block_to_sequence.q_strand = block->block_to_sequence.t_strand = ZMAPSTRAND_FORWARD ;

    }

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

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && (features->type == ZMAP_SERVERREQ_SEQUENCE
	  || features->type == ZMAP_SERVERREQ_FEATURE_SEQUENCE)
      && zMapServerGetSequence(server, context->context) != ZMAP_SERVERRESPONSE_OK)
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
      if (!addModesToStyles(context->context))
	{
	  *err_msg_out = g_strdup_printf("Inferring Style modes from Features failed.") ;
	  thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
	}
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


/* go through all the feature sets in a context and find out what type of features are in the
 * set and set the style mode from that...a bit hacky really...think about this.... */
static gboolean addModesToStyles(ZMapFeatureContext context)
{
  gboolean result = TRUE ;


  zMapFeatureContextExecute((ZMapFeatureAny)context,
			    ZMAPFEATURE_STRUCT_FEATURESET,
			    addModeCB,
			    &result) ;

  return result ;
}


static ZMapFeatureContextExecuteStatus addModeCB(GQuark key_id, 
						 gpointer data, 
						 gpointer user_data,
						 char **error_out)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data ;
  gboolean *error_ptr = (gboolean *)user_data ;
  ZMapFeatureStructType feature_type ;
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;

  feature_type = feature_any->struct_type ;

  switch(feature_type)
    {
    case ZMAPFEATURE_STRUCT_CONTEXT:
    case ZMAPFEATURE_STRUCT_ALIGN:
    case ZMAPFEATURE_STRUCT_BLOCK:
      {
	break ;
      }
    case ZMAPFEATURE_STRUCT_FEATURESET:
      {
	ZMapFeatureSet feature_set ;

        feature_set = (ZMapFeatureSet)feature_any ;

	g_datalist_foreach(&(feature_set->features), addFeatureModeCB, feature_set) ;

	break;
      }
    case ZMAPFEATURE_STRUCT_FEATURE:
    case ZMAPFEATURE_STRUCT_INVALID:
    default:
      {
	status = ZMAP_CONTEXT_EXEC_STATUS_ERROR;
	zMapAssertNotReached();
	break;
      }
    }

  return status ;
}



/* A GDataForeachFunc() to add a mode to the styles for all features in a set, note that
 * this is not efficient as we go through all features but we would need more information
 * stored in the feature set to avoid this. */
static void addFeatureModeCB(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)data ;
  ZMapFeatureSet feature_set = (ZMapFeatureSet)user_data ;

  if (!zMapStyleHasMode(feature->style))
    {
      ZMapStyleMode mode ;

      switch (feature->type)
	{
	case ZMAPFEATURE_BASIC:
	  mode = ZMAPSTYLE_MODE_BASIC ;

	  if (g_ascii_strcasecmp(g_quark_to_string(zMapStyleGetID(feature->style)), "GF_splice") == 0)
	    {
	      mode = ZMAPSTYLE_MODE_GLYPH ;
	      zMapStyleSetGlyphMode(feature->style, ZMAPSTYLE_GLYPH_SPLICE) ;

	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME0, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "red", NULL, NULL) ;
	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME1, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "blue", NULL, NULL) ;
	      zMapStyleSetColours(feature->style, ZMAPSTYLE_COLOURTARGET_FRAME2, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  "green", NULL, NULL) ;
	    }
	  break ;
	case ZMAPFEATURE_ALIGNMENT:
	  mode = ZMAPSTYLE_MODE_ALIGNMENT ;
	  break ;
	case ZMAPFEATURE_TRANSCRIPT:
	  mode = ZMAPSTYLE_MODE_TRANSCRIPT ;
	  break ;
	case ZMAPFEATURE_RAW_SEQUENCE:
	  mode = ZMAPSTYLE_MODE_TEXT ;
	  break ;
	case ZMAPFEATURE_PEP_SEQUENCE:
	  mode = ZMAPSTYLE_MODE_TEXT ;
	  break ;
	  /* What about glyph and graph..... */

	default:
	  zMapAssertNotReached() ;
	  break ;
	}

      /* Tricky....we can have features within a single feature set that have _different_
       * styles, if this is the case we must be sure to set the mode in feature_set style
       * (where in fact its kind of useless as this is a style for the whole column) _and_
       * we must set it in the features own style. */
      zMapStyleSetMode(feature_set->style, mode) ;

      if (feature_set->style != feature->style)
	zMapStyleSetMode(feature->style, mode) ;
    }


  return ;
}
