/*  File: zmapAppRemoteControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Application level functions to set up ZMap remote control
 *              and communicate with a peer program.
 *              
 *              This code provides the interface between ZMapRemoteControl
 *              and the _whole_ of the rest of ZMap. It doesn't "know"
 *              about the inidividual commands it just ensures that
 *              requests and replies get moved between the remote control
 *              code and the various components of zmap (e.g. Control,
 *              View, Window) and back.
 *
 * Exported functions: See zmapApp_P.h
 * HISTORY:
 * Last edited: Jun  3 15:54 2013 (edgrif)
 * Created: Mon Nov  1 10:32:56 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <glib.h>
#include <gdk/gdk.h>


#include <ZMap/zmapRemoteCommand.h>
#include <ZMap/zmapAppRemote.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <zmapApp_P.h>



/* Data required for each request. Used in the global queue of requests. */
typedef struct _RequestQueueDataStruct
{
  int request_num;
  ZMapAppRemote remote;
  char *command;
  char *request;
  ZMapRemoteAppProcessReplyFunc process_reply_func;
  gpointer process_reply_func_data;
  int num_retries;
  GtkWidget *widget;
} RequestQueueData;





static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlReturnReplyFunc remote_reply_func, void *remote_reply_data,
			     char *request, void *user_data) ;
static void replySentCB(void *user_data) ;
static void requestSentCB(void *user_data) ;
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data) ;

static void handleZMapRepliesCB(char *command, gboolean abort, RemoteCommandRCType result, char *reason,
				ZMapXMLUtilsEventStack reply, gpointer app_reply_data) ;
static void handleZMapRequestsCB(gpointer caller_data,
				 gpointer sub_system_ptr,
				 char *command, ZMapXMLUtilsEventStack request_body,
				 ZMapRemoteAppProcessReplyFunc reply_func, gpointer reply_func_data) ;

static void errorHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlRCType error_type, char *err_msg,
			   void *user_data) ;
static gboolean timeoutHandlerCB(ZMapRemoteControl remote_control, void *user_data) ;

static void requestBlockingTestAndBlock(void) ;
static void requestBlockingSetActive(void) ;
static void requestBlockingSetInActive(void) ;
static gboolean requestBlockingIsActive(void) ;

static gboolean sendRequestCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data) ;
static void performNextRequest(GQueue *request_queue);
static gboolean sendOrQueueRequest(ZMapAppRemote remote, const char *command, const char *request, ZMapRemoteAppProcessReplyFunc process_reply_func, gpointer process_reply_func_data, int num_retries, GtkWidget *widget) ;

static void setDebugLevel(void) ;



/* 
 *                Globals. 
 */

/* Maintains a lock while a remote request is active. */
static gboolean is_active_G = FALSE ;
static gboolean is_active_debug_G = TRUE ;

/* This maintain a queue of requests so that we can execute them in 
 * the order that they were requested */
static GQueue *request_queue_G = NULL;

/* Set debug level in remote control. */
static ZMapRemoteControlDebugLevelType remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_OFF ;



/* 
 *                       External interface.
 */


/* Returns the function that sub-systems in zmap call when they have a request to be sent to a peer. */
ZMapRemoteAppMakeRequestFunc zmapAppRemoteControlGetRequestCB(void)
{
  ZMapRemoteAppMakeRequestFunc req_func ;

  req_func = handleZMapRequestsCB ;

  return req_func ;
}



/* Set up the remote controller object. */
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context, char *peer_name, char *peer_clipboard,
				    int peer_retries, int peer_timeout_ms)
{
  gboolean result = FALSE ;
  ZMapAppRemote remote ;
  char *app_id ;
  ZMapRemoteControl remote_control ;
  static gboolean debug_set = FALSE ;  

  app_id = zMapGetAppName() ;
  if ((remote_control = zMapRemoteControlCreate(app_id,
						errorHandlerCB, app_context,
						timeoutHandlerCB, app_context,
						NULL, NULL)))
    {
      remote = g_new0(ZMapAppRemoteStruct, 1) ;
      remote->app_id = g_strdup(app_id) ;
      remote->app_unique_id = zMapMakeUniqueID(remote->app_id) ;

      remote->peer_name = peer_name ;
      remote->peer_clipboard = peer_clipboard ;

      remote->remote_controller = remote_control ;

      if (peer_retries > 0)
	remote->window_retries_max = remote->window_retries_left = peer_retries ;
      else
	remote->window_retries_max = remote->window_retries_left = ZMAP_WINDOW_RETRIES ;

      if (peer_timeout_ms > 0)
	zMapRemoteControlSetTimeout(remote->remote_controller, peer_timeout_ms) ;
      else
	zMapRemoteControlSetTimeout(remote->remote_controller, ZMAP_WINDOW_TIMEOUT_MS) ;
	


      /* Only set debug from cmdline the first time around otherwise it overrides what
       * is set dynamically....maybe it should be per remote object ? */
      if (!debug_set)
	{
	  setDebugLevel() ;

	  debug_set = TRUE ;
	}

      zMapRemoteControlSetDebug(remote->remote_controller, remote_debug_G) ;

      app_context->remote_control = remote ;

      app_context->remote_ok = result = TRUE ;
    }
  else
    {
      zMapCritical("%s", "Initialisation of remote control failed.") ;

      result = FALSE ;
    }

  return result ;
}


/* We assume that all is good here, initialise the remote control interface and
 * then set ourselves up to wait for requests. */
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  ZMapAppRemote remote = app_context->remote_control ;

  if (result)
    {
      GdkWindow *gdk_window ;
      Window x_window ;

      if ((gdk_window = gtk_widget_get_window(app_context->app_widg))
	  && (x_window = GDK_WINDOW_XID(gdk_window)))
	{
	  app_context->remote_control->app_window = x_window ;
	  app_context->remote_control->app_window_str
	    = g_strdup_printf(ZMAP_XWINDOW_FORMAT_STR, app_context->remote_control->app_window) ;

	  result = TRUE ;
	}
      else
	{
	  zMapLogCritical("%s",
			  "Could not retrieve X Window of app_window, remote control initialisation has failed.") ;

	  result = FALSE ;
	}
    }

  if (result)
    result = zMapRemoteControlReceiveInit(remote->remote_controller,
					  remote->app_unique_id,
					  requestHandlerCB, app_context,
					  replySentCB, app_context) ;

  if (result)
    result = zMapRemoteControlSendInit(remote->remote_controller,
				       remote->peer_name, remote->peer_clipboard,
				       requestSentCB, app_context,
				       replyHandlerCB, app_context) ;

  if (result)
    result = zMapRemoteControlReceiveWaitForRequest(remote->remote_controller) ;

  return result ;
}



void zmapAppRemoteControlSetExitRoutine(ZMapAppContext app_context, ZMapAppCBFunc exit_routine)
{
  app_context->remote_control->exit_routine = exit_routine ;

  return ;
}


/* Destroy all resources for the remote controller object. */
void zmapAppRemoteControlDestroy(ZMapAppContext app_context)
{
  ZMapAppRemote remote ;

  remote = app_context->remote_control ;

  g_free(remote->app_id) ;
  g_free(remote->app_unique_id) ;

  g_free(remote->peer_name) ;
  g_free(remote->peer_clipboard) ;

  /* Reset the interface and then destroy it. */
  zMapRemoteControlReset(remote->remote_controller) ;

  zMapRemoteControlDestroy(remote->remote_controller) ;

  g_free(app_context->remote_control) ;
  app_context->remote_control = NULL ;

  return ;
}








/* 
 *                Internal functions
 */


/*
 * Functions called to service requests from a peer.
 */

/* 
 * Called by ZMapRemoteControl: receives all requests sent to zmap by a peer.
 */
static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlReturnReplyFunc return_reply_func,
			     void *return_reply_func_data,
			     char *request, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;

  /* Test to see if we are processing a remote command....and then set that we are active. */
  zMapDebugPrint(is_active_debug_G, "%s", "About to test for blocking.") ;
  requestBlockingTestAndBlock() ;
  zMapDebugPrint(is_active_debug_G, "%s", "Setting our own block.") ;
  requestBlockingSetActive() ;

  /* Cache these, must be called from handleZMapRepliesCB() */
  remote->return_reply_func = return_reply_func ;
  remote->return_reply_func_data = return_reply_func_data ;

  /* Start of a series of messages with host so set window retries */
  remote->window_retries_left = ZMAP_WINDOW_RETRIES ;

  /* Call the command processing code.... */
  zmapAppProcessAnyRequest(app_context, request, handleZMapRepliesCB) ;

  return ;
}

/* Called by any component of ZMap (e.g. control, view etc) which has processed a command
 * and needs to return the reply for sending to the peer program.
 * 
 * This function calls back to ZMapRemoteControl to return the result of a command.
 * 
 * If abort is TRUE then command_rc, reason and reply are _not_ set and this 
 * function passes the abort to remotecontrol which terminates the transaction.
 * 
 *  */
static void handleZMapRepliesCB(char *command,
				gboolean abort,
				RemoteCommandRCType command_rc, char *reason,
				ZMapXMLUtilsEventStack reply,
				gpointer app_reply_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_reply_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  GArray *xml_stack ;
  char *err_msg = NULL ;
  char *full_reply = NULL ;

  if (abort)
    {
      zMapLogWarning("Aborting request \"%s\", see log file.",
		     (command ? command : "no command name available")) ;
    }
  else
    {
      /* Make a zmap protocol reply from the return code etc..... */
      if (command)
	xml_stack = zMapRemoteCommandCreateReplyFromRequest(remote->remote_controller,
							    remote->curr_peer_request,
							    command_rc, reason, reply, &err_msg) ;
      else
	xml_stack = zMapRemoteCommandCreateReplyEnvelopeFromRequest(remote->remote_controller,
								    remote->curr_peer_request,
								    command_rc, reason, reply, &err_msg) ;

      /* If we can't make sense of what's passed to us something is wrong in our code so abort
       * the request. */
      if (!xml_stack || !(full_reply = zMapXMLUtilsStack2XML(xml_stack, &err_msg, FALSE)))
	{
	  zMapLogWarning("%s", err_msg) ;

	  abort = TRUE ;
	}
    }

  /* Next phase of messages with host so reset window retries */
  remote->window_retries_left = ZMAP_WINDOW_RETRIES ;

  /* Must ALWAYS call back to ZMapRemoteControl to return reply to peer. */
  (remote->return_reply_func)(remote->return_reply_func_data, abort, full_reply) ;


  return ;
}

/* Called by remote control when peer has signalled that it has received reply,
 * i.e. the transaction has ended. */
static void replySentCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  
  zmapAppRemoteControlTheirRequestEndedCB(user_data) ;


  zMapDebugPrint(is_active_debug_G, "%s", "Received confirmation that they have our reply so unsetting our block.") ;
  requestBlockingSetInActive() ;


  /* Need to return to waiting here..... */
  if (!zMapRemoteControlReceiveWaitForRequest(app_context->remote_control->remote_controller))
    zMapLogCritical("%s", "Cannot set remote controller to wait for requests.") ;

  return ;
}




/*
 * Functions called to send requests to a peer.
 */

/* Called by any component of ZMap (e.g. control, view etc) which needs a request
 * to be sent to the peer program. This is the entry point for zmap sub-systems
 * that wish to send a request.
 * 
 *  */
static void handleZMapRequestsCB(gpointer caller_data,
				 gpointer sub_system_ptr,
				 char *command, ZMapXMLUtilsEventStack request_body,
				 ZMapRemoteAppProcessReplyFunc process_reply_func, gpointer process_reply_func_data)
{
  ZMapAppContext app_context = (ZMapAppContext)caller_data ;
  ZMapAppRemote remote = app_context->remote_control ;


  /* If we have a remote control peer then build the request.....surely this should be in assert.... */
  if (remote)
    {
      gboolean result = TRUE ;
      GArray *request_stack ;
      char *request ;
      char *err_msg = NULL ;
      char *view = NULL ;					    /* to be filled in later..... */
      gpointer view_id = NULL ;


      /* If request came from a subsystem (zmap, view, window) find the corresponding view_id
       * so we can return it to the caller. */
      if (sub_system_ptr)
	{
	  if (!(view_id = zMapManagerFindView(app_context->zmap_manager, sub_system_ptr)))
	    {
	      zMapLogCritical("Cannot find sub_system_ptr %p in zmaps/views/windows, remote call has failed.",
			      sub_system_ptr) ;
	      result = FALSE ;
	    }
	  else if (!zMapAppRemoteViewCreateIDStr(view_id, &view))
	    {
	      zMapLogCritical("Cannot convert sub_system_ptr %p to view_id, remote call has failed.",
			      sub_system_ptr) ;
	      result = FALSE ;
	    }
	}

      if (result)
	{
	  if (!err_msg && !(request_stack = zMapRemoteCommandCreateRequest(remote->remote_controller,
									   command, view, -1)))
	    {
	      err_msg = g_strdup_printf("Could not create request for command \"%s\"", command) ;
	    }

	  /* for debugging... */
	  request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE) ;
      
	  if (request_body)
	    {
	      if (!err_msg && !(request_stack = zMapRemoteCommandAddBody(request_stack, "request", request_body)))
		err_msg = g_strdup_printf("Could not add request body for command \"%s\"", command) ;
	    }

	  if (!err_msg && !(request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE)))
	    {
	      err_msg = g_strdup_printf("Could not create raw xml from request for command \"%s\"", command) ;
	    }

	  if (!err_msg)
	    {
	      if (!(result = sendOrQueueRequest(remote, command, request, process_reply_func, process_reply_func_data, ZMAP_WINDOW_RETRIES, app_context->app_widg)))
		{
		  err_msg = g_strdup_printf("Could not send request to peer: \"%s\"", request) ;


		  zMapDebugPrint(is_active_debug_G, "%s", "Send request failed so unsetting our block.") ;
		  requestBlockingSetInActive() ;


                  /* and go back to waiting for a request...... */
                  if (!zMapRemoteControlReceiveWaitForRequest(app_context->remote_control->remote_controller))
                    {
                      zMapLogCritical("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
                      zMapWarning("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
                    }
		}
	    }

	  /* Serious failure, record that remote is no longer working properly. */
	  if (err_msg)
	    {
	      app_context->remote_ok = FALSE ;

	      zMapLogCritical("%s", err_msg) ;

	      g_free(err_msg) ;
	    }
	}

    }

  return ;
}

/* Called by remote control when peer has signalled that it has received request. */
static void requestSentCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  /* Next phase of messages with host will start after this so reset window retries */
  app_context->remote_control->window_retries_left = ZMAP_WINDOW_RETRIES ;

  return ;
}

/* 
 * Called by ZMapRemoteControl: Receives all replies sent to zmap by a peer.
 */
static void replyHandlerCB(ZMapRemoteControl remote_control, char *reply, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  gboolean result ;
  char *error_out = NULL ;




  /* Again.....is this a good idea....the app callback needs to be called whatever happens
   * to reset state...fine to log a bad message but we still need to call the app func. */

  if (!(result = zMapRemoteCommandValidateReply(remote->remote_controller,
						remote->curr_zmap_request, reply, &error_out)))
    {
       /* error message etc...... */
      zMapLogWarning("Bad remote message from peer: %s", error_out) ;

      g_free(error_out) ;
    }
  else
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      remote->reply_done_func = remote_reply_done_func ;
      remote->reply_done_data = remote_reply_done_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      /* Call the command processing code, there's no return code here because only the function
       * ultimately handling the problem knows how to handle the error. */
      zmapAppProcessAnyReply(app_context, reply) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I THINK NOW THAT WE SHOULD NOT BE DOING THIS..... */

      /* We could just call back to remotecontrol from here....alternative is pass functions
       * right down to reply handler....and have them call back to appcommand level and then
       * app command level calls back here...needed if asynch....  */
      (remote_reply_done_func)(remote_reply_done_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* gb10: this was done at the start of this function in case another call needed to be
   * made from this function. That doesn't seem to ever happen and, anyway, if another call 
   * needs to be made as part of the reply, it should probably be put in the gtk main loop 
   * and performed after we've exited this function. */
  zMapDebugPrint(is_active_debug_G, "%s", "got reply from peer so unsetting our block.") ;
  requestBlockingSetInActive() ;


  /* I think this is the place to go back to waiting again.... */
  if (!zMapRemoteControlReceiveWaitForRequest(remote->remote_controller))
    zMapLogCritical("%s", "Cannot set remote object to waiting for new request.") ;


  /* Perform the next request in the queue, if there are any */
  performNextRequest(request_queue_G);

  return ;
}




/* 
 *         Called by ZMapRemoteControl: Receives all errors (n.b. only gets timeouts
 *         if timeoutHandlerCB returns FALSE).
 */
static void errorHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlRCType error_type, char *err_msg,
			   void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;


  /* Unblock if we are blocked. */
  if (requestBlockingIsActive())
    {
      zMapDebugPrint(is_active_debug_G, "%s", "error in command/reply processing so unsetting our block.") ;
      requestBlockingSetInActive() ;
    }

  /* Probably we need to call the level that made the original command request ??
   * Do we have an error handler for each level...need to do this....
   *  */
  

  /* and go back to waiting for a request...... */
  if (!zMapRemoteControlReceiveWaitForRequest(app_context->remote_control->remote_controller))
    {
      zMapLogCritical("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
      zMapWarning("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
    }

  /* Perform the next request in the queue, if there are any */
  performNextRequest(request_queue_G);

  return ;
}


/* Called by ZMapRemoteControl: Receives all timeouts, returns TRUE if 
 * timeout should be continued, FALSE otherwise.
 * 
 * If FALSE is returned the errorHandlerCB will be called by ZMapRemoteControl.
 */
static gboolean timeoutHandlerCB(ZMapRemoteControl remote_control, void *user_data)
{
  gboolean result = FALSE ;
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote app_remote_control = app_context->remote_control ;
  Window x_window ;


  /* If there is a peer window then we continue to wait by resetting the timeout,
   * otherwise it's an error and our error handler will be called, we only do this
   * up to a max of ZMAP_WINDOW_RETRIES for each phase of a transaction. */
  if ((x_window = app_remote_control->peer_window))
    {
      if (!(app_remote_control->window_retries_left))
	{
	  char *full_err_msg ;

	  full_err_msg = g_strdup_printf("Peer has still not responded after %d timeouts, transaction has timed out\n",
					 app_remote_control->window_retries_max) ;

	  zMapLogCritical("%s", full_err_msg) ;

	  zMapCritical("%s", full_err_msg) ;

	  g_free(full_err_msg) ;
	}
      else
	{
	  GdkWindow *gdk_window ; 
	  Display *x_display ;
	  char *err_msg = NULL ;      

	  gdk_window = gtk_widget_get_window(app_context->app_widg) ;
	  x_display = GDK_WINDOW_XDISPLAY(gdk_window) ;


	  (app_remote_control->window_retries_left)-- ;

	  /* This is our way of testing to see if the peer application is alive, if the window id
	   * they gave us is still good then we assume they are still alive. */
	  if (!(result = zMapGUIXWindowExists(x_display, x_window, &err_msg)))
	    {
	      char *full_err_msg ;

	      full_err_msg = g_strdup_printf("Peer window ("ZMAP_XWINDOW_FORMAT_STR") has gone,"
					     " stopping remote control because: \"%s\".", x_window, err_msg) ;

	      zMapLogCritical("%s", full_err_msg) ;

	      zMapCritical("%s", full_err_msg) ;

	      g_free(full_err_msg) ;
	    }

	  g_free(err_msg) ;
	}
    }

  return result ;
}


/* We need to keep a "lock" flag because for some actions, e.g. double click,
 * we can end up trying to send two requests, in this case a "single_select"
 * followed by an "edit" too quickly, i.e. we haven't finished the "single_select"
 * before we try to send the "edit".
 * 
 * So we test this flag and if we are processing we block (but process events)
 * until the previous action is complete.
 * 
 *  */
static void requestBlockingTestAndBlock(void)
{


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* turn off for now.... */
  return ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  zMapDebugPrint(is_active_debug_G, "Entering blocking code: %s",
		 (is_active_G ? "Request Active" : "No Request Active")) ;

  zMapDebugPrint(is_active_debug_G, "%s",
		 (is_active_G ? "Request Active, now blocking." : "No Request Active, no blocking")) ;

  while (is_active_G)
    {
      gtk_main_iteration() ;
    }

  zMapDebugPrint(is_active_debug_G, "Leaving blocking code: %s",
		 (is_active_G ? "Request Active" : "No Request Active")) ;

  return ;
}

static void requestBlockingSetActive(void)
{
  is_active_G = TRUE ;

  zMapDebugPrint(is_active_debug_G, "%s", "Setting Block: Request Active") ;

  return ;
}


static void requestBlockingSetInActive(void)
{
  is_active_G = FALSE ;

  zMapDebugPrint(is_active_debug_G, "%s", "Unsetting Block: Request InActive") ;

  return ;
}


static gboolean requestBlockingIsActive(void)
{
  return is_active_G ;
}


/* This callback is called from the gtk main loop to send a request */
static gboolean sendRequestCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  GQueue *request_queue = (GQueue*)cb_data;

  if (g_queue_is_empty(request_queue))
    {
      zMapDebugPrint(is_active_debug_G, "%s", "Request queue is empty.");
      return TRUE;
    }

  RequestQueueData *request_data = (RequestQueueData*)g_queue_peek_head(request_queue);
  zMapDebugPrint(is_active_debug_G, "Attempting to send request %d", request_data->request_num) ;

  /* Test to see if we are processing a remote command.... and then set that we are active. */
  zMapDebugPrint(is_active_debug_G, "%s", "About to test for blocking.") ;
  requestBlockingTestAndBlock() ;
  zMapDebugPrint(is_active_debug_G, "%s", "Setting our own block.") ;
  requestBlockingSetActive() ;

  if (g_queue_is_empty(request_queue))
    {
      zMapDebugPrint(is_active_debug_G, "%s", "Request queue is empty.");
      return TRUE;
    }

  request_data = (RequestQueueData*)g_queue_peek_head(request_queue);
  ZMapAppRemote remote = request_data->remote ;
  zMapDebugPrint(is_active_debug_G, "Executing request %d", request_data->request_num);
  
  /* cache the command and request, need them later. */
  if (remote->curr_zmap_command)
    g_free(remote->curr_zmap_command) ;
  remote->curr_zmap_command = request_data->command ;
  if (remote->curr_zmap_request)
    g_free(remote->curr_zmap_request) ;
  remote->curr_zmap_request = request_data->request ;
  
  /* Cache app function to be called when we receive a reply from the peer. */
  remote->process_reply_func = request_data->process_reply_func ;
  remote->process_reply_func_data = request_data->process_reply_func_data ;
  
  /* Start of a series of messages with host so set window retries */
  remote->window_retries_left = request_data->num_retries ;

  if (!request_data->request || !zMapRemoteControlSendRequest(remote->remote_controller, request_data->request))
    {
      zMapDebugPrint(is_active_debug_G, "%s", "Send request failed so unsetting our block.") ;
      requestBlockingSetInActive() ;
      
      /* Go back to waiting for a request...... */
      zMapRemoteControlReceiveWaitForRequest(remote->remote_controller) ;

      /* Perform the next request in the queue, if there are any */
      performNextRequest(request_queue_G);
    }

  /* Now remove the current command from the queue and perform the next, if any. */
  zMapDebugPrint(is_active_debug_G, "Removing request %d from queue", request_data->request_num) ;
  g_queue_pop_head(request_queue);
  g_free(request_data);

  return TRUE;  
}


/* Perform the next request in the request queue. This puts the request on the 
 * gtk main loop rather than performing it immediately */
static void performNextRequest(GQueue *request_queue)
{
  /*! \todo gb10: We have a known race condition here because we don't
   * handle the case where both peers try to make a request simultaneously,
   * which is likely to happen if both have requests queued because
   * they'll both be unblocked at the same time, and therefore try to make
   * their next requests at the same time. Processing any outstanding
   * gtk events here seems to alleviate the problem (at least with xace). */
  while (gtk_events_pending())
    gtk_main_iteration() ;

  /* If no more requests in the queue, there's nothing to do */
  if (g_queue_get_length(request_queue) < 1)
    {
      zMapDebugPrint(is_active_debug_G, "%s", "No more requests in queue") ;
      return ;
    }

  /* Get the next request in the queue */
  RequestQueueData *request_data = (RequestQueueData*)g_queue_peek_head(request_queue);
  zMapDebugPrint(is_active_debug_G, "Performing request %d", request_data->request_num);

  static gboolean first_time = TRUE;

  if (first_time)
    {
      /* Set up the callback to sendRequestCB. Because we're passing the global
       * request queue as the data we only need to set this up once. (If we passed
       * individual request data, we'd need to disconnect and reconnect
       * the callback each time.) */
      gtk_signal_connect(GTK_OBJECT(request_data->widget), "client_event", GTK_SIGNAL_FUNC(sendRequestCB), request_queue_G) ;
      first_time = FALSE;
    }

  /* Create a 'send' event to send the command specified in the
   * given remote_data. We can't do a direct xremote call because we 
   * must let gtk finish any current communications before sending a new
   * xremote command. The event will trigger sendRequestCB. */
  
  GdkEventClient event ;
  event.type         = GDK_CLIENT_EVENT ;
  event.window       = request_data->widget->window ;                     /* no window generates this event. */
  event.send_event   = TRUE ;                               /* we sent this event. */
  event.message_type = gdk_atom_intern("zmap_atom", FALSE);             /* This is our id for events. */
  event.data_format  = 8 ;		                      /* Not sure about data format here... */
  
  gdk_event_put((GdkEvent *)&event);
}


/* Entry point for caller to send a request. The request is either put into
 * a queue (if we're already processing a transaction) or into the gtk main loop.
 * 
 * Note that as soon as one request is sent to the peer, we put the next request from the
 * queue into the gtk main loop. This will probably be processed by gtk before the
 * previous transaction has completed, but that's fine because sendRequestCB will block.
 * The aim of the queueing system is to ensure that we only have one sendRequestCB
 * in the gtk main loop at any one time. (It actually works even if we issue all
 * all commands straight to the gtk loop, because sendRequestCB uses the next request
 * in the queue so it should always process them in the correct order, but I think
 * that makes things more confusing and probably more prone to error.)
 */
static gboolean sendOrQueueRequest(ZMapAppRemote remote,
                                   const char *command,
                                   const char *request,
                                   ZMapRemoteAppProcessReplyFunc process_reply_func, 
                                   gpointer process_reply_func_data,
                                   int num_retries,
                                   GtkWidget *widget)
{
  /* First time round, create the request queue */
  if (!request_queue_G)
    request_queue_G = g_queue_new();

  GQueue *request_queue = request_queue_G;

  /* request_num is a count that's used to provide a unique id, as well as the order
   * number, for each request we add to the queue. Note that it doesn't need to be 
   * the same as the request_id that is used in the xml, but in practice 
   * it should probably always be the same unless something has gone wrong. */
  static int request_num = 1;
  
  RequestQueueData *request_data = g_new0(RequestQueueData, 1);
  request_data->request_num = request_num;
  request_data->remote = remote;
  request_data->command = g_strdup(command);
  request_data->request = g_strdup(request);
  request_data->process_reply_func = process_reply_func;
  request_data->process_reply_func_data = process_reply_func_data;
  request_data->num_retries = num_retries;
  request_data->widget = widget;

  ++request_num;
  
  /* Add the request to the queue (even if we're going to perform it
   * straight away, because it's easiest to pass the global queue as
   * data to sendRequestCB) */
  g_queue_push_tail(request_queue, request_data);

  zMapDebugPrint(is_active_debug_G, "Added request %d to queue (length=%d): %s", request_data->request_num, g_queue_get_length(request_queue), request);

  /* If this is the only request in the queue, execute it now */
  if (g_queue_get_length(request_queue) == 1)
    performNextRequest(request_queue);

  return TRUE;
}


/* Check command line for remote_debug level. */
static void setDebugLevel(void)
{
  ZMapCmdLineArgsType cmdline_arg = {FALSE} ;
	  
  if (zMapCmdLineArgsValue(ZMAPARG_REMOTE_DEBUG, &cmdline_arg))
    {
      char *debug_level_str ;
	      
      debug_level_str = cmdline_arg.s ;

      if (strcasecmp(debug_level_str, "off") == 0)
	remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_OFF ;
      else if (strcasecmp(debug_level_str, "normal") == 0)
	remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_NORMAL ;
      else if (strcasecmp(debug_level_str, "verbose") == 0)
	remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_VERBOSE ;
    }

  return ;
}

