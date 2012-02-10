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
 * Last edited: Feb  8 21:31 2012 (edgrif)
 * Created: Mon Nov  1 10:32:56 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>

#include <ZMap/zmapRemoteCommand.h>
#include <ZMap/zmapAppRemote.h>
#include <zmapApp_P.h>



static void ourRequestEndedCB(void *user_data) ;
static void theirRequestEndedCB(void *user_data) ;
static gboolean errorHandlerCB(ZMapRemoteControl remote_control, void *user_data) ;

static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlCallWithReplyFunc remote_reply_func, void *remote_reply_data,
			     char *request, void *user_data) ;
static void replyHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlCallReplyDoneFunc remote_reply_func, void *remote_reply_data,
			   char *reply, void *user_data) ;

static void handleZMapRepliesCB(char *command, RemoteCommandRCType result, char *reason,
				ZMapXMLUtilsEventStack reply, gpointer app_reply_data) ;
static void handleZMapRequestsCB(char *command, ZMapXMLUtilsEventStack request_body, gpointer app_request_data,
				 ZMapRemoteAppProcessReplyFunc reply_func, gpointer reply_func_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static GArray *createRequestEnvelope(ZMapAppRemote remote, char *unique_id, char *command) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */









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
  

  app_id = zMapGetAppName() ;
  if ((remote_control = zMapRemoteControlCreate(app_id,
						ourRequestEndedCB, app_context,
						theirRequestEndedCB, app_context,
						errorHandlerCB, app_context,
						NULL, NULL)))
    {
      remote = g_new0(ZMapAppRemoteStruct, 1) ;
      remote->app_id = g_strdup(app_id) ;
      remote->app_unique_id = zMapMakeUniqueID(remote->app_id) ;

      remote->peer_name = peer_name ;
      remote->peer_clipboard = peer_clipboard ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      remote->request_id_num = 0 ;
      remote->request_id = g_string_new("") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      remote->remote_controller = remote_control ;

      /* set no timeout for now... */
      zMapRemoteControlSetTimeout(remote->remote_controller, 0) ;

      remote->view2zmap = g_hash_table_new(NULL, NULL) ;

      app_context->remote_control = remote ;

      result = TRUE ;
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
					  requestHandlerCB, app_context) ;

  if (result)
    result = zMapRemoteControlSendInit(remote->remote_controller,
				       remote->peer_name, remote->peer_clipboard,
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

  zMapRemoteControlDestroy(remote->remote_controller) ;

  g_hash_table_destroy(remote->view2zmap) ;

  g_free(app_context->remote_control) ;
  app_context->remote_control = NULL ;

  return ;
}








/* 
 *                Internal functions
 */



/*             Functions called directly by ZMapRemoteControl.                */


/* 
 * Called by ZMapRemoteControl: receives all requests sent to zmap by a peer.
 */
static void requestHandlerCB(ZMapRemoteControl remote_control,
			     ZMapRemoteControlCallWithReplyFunc remotecontrol_request_done_func,
			     void *remotecontrol_request_done_data,
			     char *request, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  gboolean result ;
  char *error_out = NULL ;

  /* should we be validating at this level ?....surely remotecontrol code should have done that ? */
  if (!(result = zMapRemoteCommandValidateRequest(remote->remote_controller, request, &error_out)))
    {
      /* error message etc...... */
      zMapLogWarning("Bad remote message from peer: %s", error_out) ;

      g_free(error_out) ;
    }
  else
    {
      /* Cache these, must be called from handleZMapRepliesCB() */
      remote->remotecontrol_request_done_func = remotecontrol_request_done_func ;
      remote->remotecontrol_request_done_data = remotecontrol_request_done_data ;

      /* Call the command processing code....if it succeeds we don't need to do anything, we will
       * be called back. */
      if (!(result = zmapAppProcessAnyRequest(app_context, request, handleZMapRepliesCB)))
	{
	  /* report error.... */

	}
    }


  return ;
}



/* 
 * Called by ZMapRemoteControl: Receives all replies sent to zmap by a peer.
 */
static void replyHandlerCB(ZMapRemoteControl remote_control,
			   ZMapRemoteControlCallReplyDoneFunc remote_reply_done_func,
			   void *remote_reply_done_data,
			   char *reply, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  gboolean result ;
  char *error_out = NULL ;

  if (!(result = zMapRemoteCommandValidateReply(remote->remote_controller, remote->curr_request, reply, &error_out)))
    {
       /* error message etc...... */
      zMapLogWarning("Bad remote message from peer: %s", error_out) ;

      g_free(error_out) ;
    }
  else
    {
      remote->reply_done_func = remote_reply_done_func ;
      remote->reply_done_data = remote_reply_done_data ;

      /* Call the command processing code....if it succeeds we don't need to do anything, we will
       * be called back. */
      if (!(result = zmapAppProcessAnyReply(app_context, reply)))
	{
	  /* report error.... */
	}

      /* We could just call back to remotecontrol from here....alternative is pass functions
       * right down to reply handler....and have them call back to appcommand level and then
       * app command level calls back here...needed if asynch....  */
      (remote_reply_done_func)(remote_reply_done_data) ;
    }



  return ;
}


/* Called by RemoteControl when a request we sent has been replied to by the peer
 * and the transaction is ended. */
static void ourRequestEndedCB(void *user_data)
{



  return ;
}



/* Called by RemoteControl when our peer sent a request and our reply has been sent
 * and the transaction is ended. */
static void theirRequestEndedCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  
  zmapAppRemoteControlTheirRequestEndedCB(user_data) ;


  return ;
}


/* 
 *         Called by ZMapRemoteControl: Receives all errors (e.g. timeouts).
 */
static gboolean errorHandlerCB(ZMapRemoteControl remote_control, void *user_data)
{
  gboolean result = FALSE ;




  return result ;
}



/*        Functions called by ZMap sub-components, e.g. Control, View, Window   */



/* Called by any component of ZMap (e.g. control, view etc) which has processed a command
 * and needs to return the reply for sending to the peer program.
 * 
 * This function calls back to ZMapRemoteControl to return the result of a command.
 *  */
static void handleZMapRepliesCB(char *command, RemoteCommandRCType command_rc, char *reason,
				ZMapXMLUtilsEventStack reply,
				gpointer app_reply_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_reply_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  GArray *xml_stack ;
  char *err_msg = NULL ;
  char *full_reply = NULL ;

  /* Make a zmap protocol reply from the return code etc..... */
  if (!(xml_stack = zMapRemoteCommandCreateReplyFromRequest(remote->remote_controller,
							    remote->curr_request,
							    command_rc, reason, reply, &err_msg))
      || !(full_reply = zMapRemoteCommandStack2XML(xml_stack, &err_msg)))
    {
      zMapLogWarning("%s", err_msg) ;

      /* There's a problem here....if the app messes up the request syntax somehow this
       * part will fail and then we lose communication....need to tie down who looks
       * after the request that we make the reply from...be better to keep it internally
       * if possible..... */

      full_reply = "disaster...bad xml...." ;
    }

  /* Must ALWAYS call back to ZMapRemoteControl to return reply to peer. */
  (remote->remotecontrol_request_done_func)(remote->remotecontrol_request_done_data, full_reply) ;

  return ;
}



/* Called by any component of ZMap (e.g. control, view etc) which needs a request
 * to be sent to the peer program. This is the entry point for zmap sub-systems
 * that wish to send a request.
 * 
 *  */
static void handleZMapRequestsCB(char *command, ZMapXMLUtilsEventStack request_body, gpointer app_request_data,
				 ZMapRemoteAppProcessReplyFunc process_reply_func, gpointer process_reply_func_data)
{
  gboolean result = FALSE ;
  ZMapAppContext app_context = (ZMapAppContext)app_request_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  GArray *request_stack ;
  char *request ;
  char *err_msg = NULL ;
  char *view = NULL ;					    /* to be filled in later..... */


  /* Build the request. */
  request_stack = zMapRemoteCommandCreateRequest(remote->remote_controller, command, view, -1) ;

  if (request_body)
    request_stack = zMapRemoteCommandAddBody(request_stack, "request", request_body) ;

  request = zMapRemoteCommandStack2XML(request_stack, &err_msg) ;


  /* cache the command and request, need them later. */
  remote->curr_command = command ;
  remote->curr_request = request ;

  /* Cache app function to be called when we receive a reply from the peer. */
  remote->process_reply_func = process_reply_func ;
  remote->process_reply_func_data = process_reply_func_data ;


  if (!(result = zMapRemoteControlSendRequest(remote->remote_controller, remote->curr_request)))
    {
      zMapLogCritical("Could not send request to peer program: %s.", request) ;

      zMapCritical("Could not send request to peer program: %s.", request) ;
    }

  return ;
}



