/*  File: zmapServerProtocolHandler.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb  4 08:58 2005 (edgrif)
 * Created: Thu Jan 27 13:17:43 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocolHandler.c,v 1.3 2005-02-10 16:38:45 edgrif Exp $
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

#include <ZMap/zmapThreads.h>
#include <ZMap/zmapServerProtocol.h>
#include <zmapServer_P.h>



/* Some protocols have global init/cleanup functions that must only be called once, this type/list
 * allows us to do this. */
typedef struct
{
  char *protocol ;
  gboolean init_called ;
  void *global_init_data ;
  gboolean cleanup_called ;
} ZMapProtocolInitStruct, *ZMapProtocolInit ;

typedef struct
{
  pthread_mutex_t mutex ;				    /* Protects access to init/cleanup list. */
  GList *protocol_list ;				    /* init/cleanup list. */
} ZMapProtocolInitListStruct, *ZMapProtocolInitList ;




static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, char *protocol,
				   void **global_init_data) ;
static int findProtocol(gconstpointer list_protocol, gconstpointer protocol) ;


static ZMapThreadReturnCode openServerAndLoad(ZMapServerReqOpenLoad request, ZMapServer *server_out,
					      char **err_msg_out, void *global_init_data) ;

static ZMapThreadReturnCode terminateServer(ZMapServer *server, char **err_msg_out) ;



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


  zMapAssert(request_in && reply_out && err_msg_out) ;	    /* slave_data is NULL first time we
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

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* sort this out later...... */

    case  ZMAP_SERVERREQ_OPEN:
      {
	ZMapServerReqOpen open = (ZMapServerReqOpen)request_in ;
	
	/* Check if we need to call the global init function of the protocol, this is a
	 * function that should only be called once, only need to do this when setting up a server. */
	protocolGlobalInitFunc(&protocol_init_G, open->protocol, &global_init_data) ;

	thread_rc = openServer(open, &server, err_msg_out, global_init_data) ;
	break ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    case ZMAP_SERVERREQ_OPENLOAD:
      {
	ZMapServerReqOpenLoad open_load = (ZMapServerReqOpenLoad)request_in ;
	
	/* Check if we need to call the global init function of the protocol, this is a
	 * function that should only be called once, only need to do this when setting up a server. */
	protocolGlobalInitFunc(&protocol_init_G, open_load->open.protocol, &global_init_data) ;

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
static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, char *protocol,
				   void **global_init_data_out)
{
  int status ;
  GList *curr_ptr ;
  ZMapProtocolInit init ;

  if ((status = pthread_mutex_lock(&(protocols->mutex))) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "protocolGlobalInitFunc() mutex lock") ;
    }

  /* If we don't find the protocol in the list then add it, initialised to FALSE. */
  if (!(curr_ptr = g_list_find_custom(protocols->protocol_list, protocol, findProtocol)))
    {
      init = (ZMapProtocolInit)g_new0(ZMapProtocolInitStruct, 1) ;
      init->protocol = g_strdup(protocol) ;
      init->init_called = init->cleanup_called = FALSE ;
      init->global_init_data = NULL ;

      protocols->protocol_list = g_list_prepend(protocols->protocol_list, init) ;
    }
  else
    init = curr_ptr->data ;

  /* Call the init routine if its not been called yet and either way return the global_init_data. */
  if (!init->init_called)
    {
      if (!zMapServerGlobalInit(protocol, &(init->global_init_data)))
 	zMapLogFatal("Initialisation call for %s protocol failed.", protocol) ;

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
  int result ;
  char *list_protocol = ((ZMapProtocolInit)list_data)->protocol, *protocol = (char *)custom_data ;

  result = strcasecmp(list_protocol, protocol) ;

  return result ;
}
  
 
static ZMapThreadReturnCode openServerAndLoad(ZMapServerReqOpenLoad request, ZMapServer *server_out,
					      char **err_msg_out, void *global_init_data)
{
  ZMapThreadReturnCode thread_rc = ZMAPTHREAD_RETURNCODE_OK ;
  ZMapServer server ;
  ZMapServerReqOpen open = &request->open ;
  ZMapServerReqNewContext context = &request->context ;
  ZMapServerReqGetFeatures features = &request->features ;
  ZMapFeatureContext feature_context ;

  /* Create the thread block for this specific server thread. */
  if (!zMapServerCreateConnection(&server, global_init_data,
				  open->machine, open->port,
				  open->protocol, open->format, open->timeout, open->version,
				  "any", "any"))
    {
      *err_msg_out = zMapServerLastErrorMsg(server) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && zMapServerOpenConnection(server) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  /* Create a sequence context from the sequence and start/end data. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && zMapServerSetContext(server, context->sequence,
			      context->start, context->end, context->types)
      != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;
    }


  /* Get a copy of the context to use in fetching the features and/or sequence. */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      feature_context = zMapServerCopyContext(server) ;
    }

 

  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK
      && (features->type == ZMAP_SERVERREQ_FEATURES
	  || features->type == ZMAP_SERVERREQ_FEATURE_SEQUENCE)
      && zMapServerGetFeatures(server, feature_context) != ZMAP_SERVERRESPONSE_OK)
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
      && zMapServerGetSequence(server, feature_context) != ZMAP_SERVERRESPONSE_OK)
    {
      *err_msg_out = g_strdup_printf(zMapServerLastErrorMsg(server)) ;
      thread_rc = ZMAPTHREAD_RETURNCODE_REQFAIL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* We need to free feature_context here, its subparts should have been freed by routines
       * we call....... */
      g_free(feature_context) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* error handling...if there is a server we should get rid of it....and sever connection if
   * required.... */
  if (thread_rc == ZMAPTHREAD_RETURNCODE_OK)
    {
      *server_out = server ;
      features->feature_context_out = feature_context ;
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
