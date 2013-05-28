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
 * Created: Mon Nov  1 10:32:56 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>

#include <ZMap/zmapRemoteCommand.h>
#include <ZMap/zmapAppRemote.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <zmapApp_P.h>




static void errorHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlRCType error_type, char *err_msg,
			   void *user_data) ;

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


/* TRY PUTTING AT APP LEVEL.... */
static void requestblockIfActive(void) ;
static void requestSetActive(void) ;
static void requestSetInActive(void) ;
static gboolean requestIsActive(void) ;

static void setDebugLevel(void) ;

/* 
 *                Globals. 
 */

/* Maintains a lock while a remote request is active. */
static gboolean is_active_G = FALSE ;
static gboolean is_active_debug_G = TRUE ;

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
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context, char *peer_name, char *peer_clipboard)
{
  gboolean result = FALSE ;
  ZMapAppRemote remote ;
  char *app_id ;
  ZMapRemoteControl remote_control ;
  static gboolean debug_set = FALSE ;  

  app_id = zMapGetAppName() ;
  if ((remote_control = zMapRemoteControlCreate(app_id,
						errorHandlerCB, app_context,
						NULL, NULL)))
    {
      remote = g_new0(ZMapAppRemoteStruct, 1) ;
      remote->app_id = g_strdup(app_id) ;
      remote->app_unique_id = zMapMakeUniqueID(remote->app_id) ;

      remote->peer_name = peer_name ;
      remote->peer_clipboard = peer_clipboard ;

      remote->remote_controller = remote_control ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* set no timeout for now... */
      zMapRemoteControlSetTimeout(remote->remote_controller, 0) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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


/* We need to keep a "lock" flag because for some actions, e.g. double click,
 * we can end up trying to send two requests, in this case a "single_select"
 * followed by an "edit" too quickly, i.e. we haven't finished the "single_select"
 * before we try to send the "edit".
 * 
 * So we test this flag and if we are processing we block (but process events)
 * until the previous action is complete.
 * 
 *  */
static void requestblockIfActive(void)
{


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* turn off for now.... */
  return ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  zMapDebugPrint(is_active_debug_G, "Entering blocking code: %s",
		 (is_active_G ? "Request Active" : "No Request Active")) ;

  while (is_active_G)
    {
      gtk_main_iteration() ;
    }

  zMapDebugPrint(is_active_debug_G, "Leaving blocking code: %s",
		 (is_active_G ? "Request Active" : "No Request Active")) ;

  return ;
}

static void requestSetActive(void)
{
  is_active_G = TRUE ;

  zMapDebugPrint(is_active_debug_G, "%s", "Setting Block: Request Active") ;

  return ;
}


static void requestSetInActive(void)
{
  is_active_G = FALSE ;

  zMapDebugPrint(is_active_debug_G, "%s", "Setting Block: Request InActive") ;

  return ;
}


static gboolean requestIsActive(void)
{
  return is_active_G ;
}




/*             Functions called directly by ZMapRemoteControl.                */


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

  /* Cache these, must be called from handleZMapRepliesCB() */
  remote->return_reply_func = return_reply_func ;
  remote->return_reply_func_data = return_reply_func_data ;

  /* Call the command processing code.... */
  zmapAppProcessAnyRequest(app_context, request, handleZMapRepliesCB) ;

  return ;
}


/* Called by remote control when peer has signalled that it has received reply,
 * i.e. the transaction has ended. */
static void replySentCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  
  zmapAppRemoteControlTheirRequestEndedCB(user_data) ;


  /* Need to return to waiting here..... */
  if (!zMapRemoteControlReceiveWaitForRequest(app_context->remote_control->remote_controller))
    zMapLogCritical("%s", "Cannot set remote controller to wait for requests.") ;

  return ;
}



/* Called by remote control when peer has signalled that it has received request. */
static void requestSentCB(void *user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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


  /* TRY THIS HERE.... */
  /* Now we know that the request/reply is over unset our "request active" flag. */
  requestSetInActive() ;



  /* I think this is the place to go back to waiting again.... */
  if (!zMapRemoteControlReceiveWaitForRequest(remote->remote_controller))
    zMapLogCritical("%s", "Cannot set remote object to waiting for new request.") ;


  return ;
}




/* 
 *         Called by ZMapRemoteControl: Receives all errors (e.g. timeouts).
 */
static void errorHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlRCType error_type, char *err_msg,
			   void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;


  /* Unblock if we are blocked. */
  if (requestIsActive())
    requestSetInActive() ;

  /* Probably we need to call the level that made the original command request ??
   * Do we have an error handler for each level...need to do this....
   *  */
  

  /* and go back to waiting for a request...... */
  if (!zMapRemoteControlReceiveWaitForRequest(app_context->remote_control->remote_controller))
    {
      zMapLogCritical("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
      zMapWarning("%s", "Call to wait for peer requests failed, cannot communicate with peer.") ;
    }



  return ;
}





/*
 *        Functions called by ZMap sub-components, e.g. Control, View, Window
 */


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


  /* Must ALWAYS call back to ZMapRemoteControl to return reply to peer. */
  (remote->return_reply_func)(remote->return_reply_func_data, abort, full_reply) ;


  return ;
}



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

      /* TRY ALL THIS HERE.... */
      /* Test to see if we are processing a remote command....and then set that we are active. */
      requestblockIfActive() ;
      requestSetActive() ;


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
	      /* cache the command and request, need them later. */
	      if (remote->curr_zmap_command)
		g_free(remote->curr_zmap_command) ;
	      remote->curr_zmap_command = g_strdup(command) ;
	      if (remote->curr_zmap_request)
		g_free(remote->curr_zmap_request) ;
	      remote->curr_zmap_request = g_strdup(request) ;

	      /* Cache app function to be called when we receive a reply from the peer. */
	      remote->process_reply_func = process_reply_func ;
	      remote->process_reply_func_data = process_reply_func_data ;


	      if (!(result = zMapRemoteControlSendRequest(remote->remote_controller, remote->curr_zmap_request)))
		{
		  err_msg = g_strdup_printf("Could not send request to peer: \"%s\"", request) ;
		}
	    }

	  /* Serious failure, record that remote is no longer working properly. */
	  if (err_msg)
	    {
	      app_context->remote_ok = FALSE ;

	      zMapLogCritical("%s", err_msg) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      /* TOO MANY WARNINGS.... */

	      zMapCritical("%s", err_msg) ;
	      zMapWarning("%s", err_msg) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	      g_free(err_msg) ;
	    }
	}

    }

  return ;
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

