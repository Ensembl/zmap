/*  File: zmapSlave.c
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapConn_P.h
 * HISTORY:
 * Last edited: Sep 29 13:07 2004 (edgrif)
 * Created: Thu Jul 24 14:37:26 2003 (edgrif)
 * CVS info:   $Id: zmapSlave.c,v 1.15 2004-09-29 12:37:24 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <strings.h>
/* YOU SHOULD BE AWARE THAT ON SOME PLATFORMS (E.G. ALPHAS) THERE SEEMS TO BE SOME INTERACTION
 * BETWEEN pthread.h AND gtk.h SUCH THAT IF pthread.h COMES FIRST THEN gtk.h INCLUDES GET MESSED
 * UP AND THIS FILE WILL NOT COMPILE.... */
#include <pthread.h>
#include <ZMap/zmapUtils.h>

/* With some additional calls in the zmapConn code I could get rid of the need for
 * the private header here...... */
#include <zmapConn_P.h>

#include <zmapSlave_P.h>



enum {ZMAP_SLAVE_REQ_BUFSIZE = 512} ;


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




static void cleanUpThread(void *thread_args) ;
static void protocolGlobalInitFunc(ZMapProtocolInitList protocols, char *protocol,
				   void **global_init_data) ;
static int findProtocol(gconstpointer list_protocol, gconstpointer protocol) ;



/* Set up the list, note the special pthread macro that makes sure mutex is set up before
 * any threads can use it. */
static ZMapProtocolInitListStruct protocol_init_G = {PTHREAD_MUTEX_INITIALIZER, NULL} ;





/* This is the routine that is called by the pthread_create() function, in effect
 * this is an endless loop processing requests signalled to the thread via a
 * condition variable. If this thread kills itself we will exit this routine
 * normally, if the thread is cancelled however we may exit from any thread cancellable
 * system call currently in progress or any of our cancellation points so its
 * important that the clean up routine registered with pthread_cleanup_push()
 * really does clean up properly. */
void *zmapNewThread(void *thread_args)
{
  zmapThreadCB thread_cb ;
  void *global_init_data ;
  ZMapConnection connection = (ZMapConnection)thread_args ;
  ZMapRequest thread_state = &(connection->request) ;
  int status ;
  TIMESPEC timeout ;
  ZMapThreadRequest signalled_state ;


  ZMAP_THR_DEBUG(("%x: main thread routine starting....\n", connection->thread_id)) ;


  thread_cb = g_new0(zmapThreadCBstruct, 1) ;
  thread_cb->connection = connection ;
  thread_cb->thread_died = FALSE ;
  thread_cb->initial_error = NULL ;
  thread_cb->server = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  thread_cb->server_request = ZMAP_SERVERREQ_INVALID ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  thread_cb->server_reply = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Only do the minimum before setting this up as this is the call that makes sure our
   * cleanup routine is called. */
  pthread_cleanup_push(cleanUpThread, (void *)thread_cb) ;


  /* Check if we need to call the global init function of the protocol, this is a
   * function that should only be called once. */
  protocolGlobalInitFunc(&protocol_init_G, connection->protocol, &global_init_data) ;


  /* Create the connection block for this specific server connection. */
  if (!zMapServerCreateConnection(&(thread_cb->server), global_init_data,
				  connection->machine, connection->port,
				  connection->protocol, connection->timeout,
				  "any", "any"))
    {
      thread_cb->thread_died = TRUE ;
      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNCREATE,
						 zMapServerLastErrorMsg(thread_cb->server)) ;
      goto clean_up ;
    }

  if (zMapServerOpenConnection(thread_cb->server) != ZMAP_SERVERRESPONSE_OK)
    {
      thread_cb->thread_died = TRUE ;
      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNOPEN,
						 zMapServerLastErrorMsg(thread_cb->server)) ;
      goto clean_up ;
    }


  /* Now we have the added step of creating a sequence context from the sequence start/end data. */
  if (zMapServerSetContext(thread_cb->server, connection->sequence, connection->start, connection->end)
      != ZMAP_SERVERRESPONSE_OK)
    {
      thread_cb->thread_died = TRUE ;
      thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNCONTEXT,
						 zMapServerLastErrorMsg(thread_cb->server)) ;
      goto clean_up ;
    }


  if (connection->load_features)
    {
      ZMapServerResponseType server_response ;
      ZMapProtocoltGetFeatures req_features ;

      req_features = g_new0(ZMapProtocolGetFeaturesStruct, 1) ;
      req_features->request = ZMAP_PROTOCOLREQUEST_SEQUENCE ;

      if (zMapServerRequest(thread_cb->server, (ZMapProtocolAny)req_features) == ZMAP_SERVERRESPONSE_OK)
	{
	  /* Signal that we got some data. */
	  zmapVarSetValueWithData(&(connection->reply), ZMAP_REPLY_GOTDATA, req_features) ;
	}
      else
	{
	  g_free(req_features) ;

	  thread_cb->thread_died = TRUE ;
	  thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNREQUEST,
						     zMapServerLastErrorMsg(thread_cb->server)) ;
	  goto clean_up ;
	}
    }
  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* somehow doing this screws up the whole gui-slave communication bit...not sure how....
   * the slaves just die, almost all the time.... */

  /* Signal that we are ready and waiting... */
  zMapConnSetReply(connection, ZMAP_REPLY_WAIT) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  while (1)
    {
      void *request ;

      ZMAP_THR_DEBUG(("%x: about to do timed wait\n", connection->thread_id)) ;

      /* this will crap over performance...asking the time all the time !! */
      timeout.tv_sec = 5 ;				    /* n.b. interface seems to absolute time. */
      timeout.tv_nsec = 0 ;
      request = NULL ;
      signalled_state = zmapCondVarWaitTimed(thread_state, ZMAP_REQUEST_WAIT, &timeout, TRUE,
					     &request) ;

      /* The whole request bit needs sorting out...into types and data... */

      ZMAP_THR_DEBUG(("%x: finished condvar wait, state = %s\n", connection->thread_id,
		      zMapVarGetRequestString(signalled_state))) ;

      if (signalled_state == ZMAP_REQUEST_TIMED_OUT)
	{
	  continue ;
	}
      else if (signalled_state == ZMAP_REQUEST_GETDATA)
	{
	  static int failure = 0 ;
	  char *server_command ;
	  int reply_len = 0 ;
	  ZMapServerResponseType server_response ;

	  /* Must have a request at this stage.... */
	  zMapAssert(request) ;

	  /* this needs changing according to request.... */
	  ZMAP_THR_DEBUG(("%x: servicing request....\n", connection->thread_id)) ;

	  server_response = zMapServerRequest(thread_cb->server, (ZMapProtocolAny)request) ;

	  switch (server_response)
	    {
	    case ZMAP_SERVERRESPONSE_OK:
	      {
		ZMAP_THR_DEBUG(("%x: got all data....\n", connection->thread_id)) ;

		/* Signal that we got some data. */
		zmapVarSetValueWithData(&(connection->reply), ZMAP_REPLY_GOTDATA, request) ;
		
		request = NULL ;			    /* Reset, we don't free this data. */
		break ;
	      }
	    case ZMAP_SERVERRESPONSE_TIMEDOUT:
	    case ZMAP_SERVERRESPONSE_REQFAIL:
	    case ZMAP_SERVERRESPONSE_BADREQ:
	      {
		char *error_msg ;

		ZMAP_THR_DEBUG(("%x: request failed....\n", connection->thread_id)) ;

		error_msg = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNREQUEST,
					    zMapServerLastErrorMsg(thread_cb->server)) ;

		/* Signal that we failed. */
		zmapVarSetValueWithError(&(connection->reply), ZMAP_REPLY_REQERROR, error_msg) ;

		break ;
	      }
	    case ZMAP_SERVERRESPONSE_SERVERDIED:
	      {
		char *error_msg ;

		ZMAP_THR_DEBUG(("%x: server died....\n", connection->thread_id)) ;

		error_msg = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNREQUEST,
					    zMapServerLastErrorMsg(thread_cb->server)) ;

		/* Signal that we failed. */
		zmapVarSetValueWithError(&(connection->reply), ZMAP_REPLY_DIED, error_msg) ;

		thread_cb->thread_died = TRUE ;

		thread_cb->initial_error = g_strdup_printf("%s - %s", ZMAPSLAVE_CONNREQUEST,
							   zMapServerLastErrorMsg(thread_cb->server)) ;

		goto clean_up ;
	      }
	    }
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      else
	{
	  /* unknown state......... */
	  thread_cb->thread_died = TRUE ;
	  thread_cb->initial_error = g_strdup_printf("Thread received unknown state from GUI: %s",
						     zMapVarGetRequestString(signalled_state)) ;
	  goto clean_up ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }



  /* Note that once we reach here the thread will exit, the pthread_cleanup_pop(1) ensures
   * we call our cleanup routine before we exit.
   * Most times we will not get here because we will be pthread_cancel'd and go straight into
   * our clean_up routine. */
 clean_up:

  pthread_cleanup_pop(1) ;				    /* 1 => always call clean up routine */


  ZMAP_THR_DEBUG(("%x: main thread routine exitting....\n", connection->thread_id)) ;


  return thread_args ;
}


/* Gets called when:
 *  1) There is an error and the thread has to exit.
 *  2) The thread gets cancelled, can happen in any of the POSIX cancellable
 *     system calls (e.g. read, select etc.) or from anywhere we declare as a cancellation point.
 *
 * This means some care is needed in handling resources to be released, we may need to set flags
 * to remember which resources have been allocated.
 * In particular we DO NOT free the thread_cb->connection, this is done by the GUI thread
 * which created it.
 */
static void cleanUpThread(void *thread_args)
{
  zmapThreadCB thread_cb = (zmapThreadCB)thread_args ;
  ZMapConnection connection = thread_cb->connection ;
  ZMapThreadReply reply ;
  gchar *error_msg = NULL ;

  ZMAP_THR_DEBUG(("%x: thread clean-up routine starting....\n", connection->thread_id)) ;

  if (thread_cb->thread_died)
    {
      reply = ZMAP_REPLY_DIED ;
      if (thread_cb->initial_error)
	error_msg = g_strdup(thread_cb->initial_error) ;
    }
  else
    reply = ZMAP_REPLY_CANCELLED ;

  if (thread_cb->server)
    {
      if (zMapServerCloseConnection(thread_cb->server)
	  && zMapServerFreeConnection(thread_cb->server))
	{
	  thread_cb->server = NULL ;
	}
      else
	ZMAP_THR_DEBUG(("%x: Unable to close connection to server cleanly\n",
			connection->thread_id)) ;

      /* this should be an error message now as well as a debug message..... */
    }


  g_free(thread_cb) ;

  printf("%x: error msg before condvar set: %s\n", connection->thread_id, error_msg) ;
  fflush(stdout) ;

  if (!error_msg)
    zmapVarSetValue(&(connection->reply), reply) ;
  else
    zmapVarSetValueWithError(&(connection->reply), reply, error_msg) ;

  printf("%x: error msg after condvar set: %s\n", connection->thread_id, error_msg) ;
  fflush(stdout) ;




  ZMAP_THR_DEBUG(("%x: thread clean-up routine exitting because %s....\n",
		  connection->thread_id,
		  zMapVarGetReplyString(reply))) ;

  return ;
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
  
 
