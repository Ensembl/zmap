/*  File: zmapAppRemoteCommand.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: functions to process remote control commands for
 *              the "App" level of ZMap.
 *
 * Exported functions: See zmapApp_P.h
 * 
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <errno.h>
#include <glib.h>

#include <ZMap/zmapRemoteCommand.hpp>
#include <zmapAppRemote_P.hpp>




/* COPY STUFF FROM OLD REMOTE... */

/* Requests that come to us from an external program. */
typedef enum
  {
    ZMAPAPP_REMOTE_INVALID,
    ZMAPAPP_REMOTE_OPEN_ZMAP,    /* Open a new zmap window. */
    ZMAPAPP_REMOTE_CLOSE_ZMAP,    /* Close a window. */
    ZMAPAPP_REMOTE_UNKNOWN
  } ZMapAppValidXRemoteAction;



/* This should be somewhere else ...
   or we should be making other objects */
typedef struct
{
  ZMapAppContext app_context;
  ZMapAppValidXRemoteAction action;

  char *command_name ;
  RemoteCommandRCType command_rc ;
  char *err_msg ;

  GString *message;


  /* only for now....not sure why this is in app_context->info->code */
  int code ;


  GQuark sequence ;
  GQuark start ;
  GQuark end ;
  GQuark add_to_view ;

  /* Is this used ?????? */
  GQuark source ;

  GQuark view_id ;

} RequestDataStruct, *RequestData ;




static void localProcessRemoteRequest(gpointer sub_system,
                                      char *command_name, char *request,
                                      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
                                  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                  gpointer reply_handler_func_data) ;
static void processRequest(ZMapAppContext app_context,
                           char *command_name, char *request,
                           RemoteCommandRCType *command_rc_out, char **reason_out,
                           ZMapXMLUtilsEventStack *reply_out) ;

static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data) ;

static ZMapXMLUtilsEventStack createPeerElement(char *socket_id) ;
static gboolean deferredRequestHandler(gpointer data) ;


/*
 *        Globals
 */




/* 
 *                 External functions.
 */


/* Handles all incoming requests to zmap from a peer. */
void zmapAppProcessAnyRequest(ZMapAppContext app_context,
                              char *request, ZMapRemoteAppReturnReplyFunc replyHandlerFunc)
{
  ZMapAppRemote remote = app_context->remote_control ;
  RemoteValidateRCType valid_rc ;
  gboolean result = TRUE ;
  char *command_name = NULL ;
  ZMap zmap = NULL ;
  gpointer view_id = NULL ;
  gpointer view_ptr = NULL ;
  char *err_msg = NULL ;
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_OK ;
  ZMapXMLUtilsEventStack reply = NULL ;



  /* Hold on to original request, need it later. */
  if (remote->curr_peer_request)
    {
      g_free(remote->curr_peer_request) ;
      remote->curr_peer_request = NULL ;
    }
  remote->curr_peer_request = g_strdup(request) ;


  /* Validate the request envelope. */ 
  if ((valid_rc = zMapRemoteCommandValidateEnvelope(remote->remote_controller, request, &err_msg))
      != REMOTE_VALIDATE_RC_OK)
    {
      gboolean abort ;
      char *reason = NULL ;

      if (valid_rc == REMOTE_VALIDATE_RC_ENVELOPE_XML || valid_rc == REMOTE_VALIDATE_RC_ENVELOPE_CONTENT)
        abort = TRUE ;
      else
        abort = FALSE ;

      command_rc = REMOTE_COMMAND_RC_BAD_XML ;
      reason = g_strdup_printf("Bad xml: %s", err_msg) ;
      reply = NULL ;

      (replyHandlerFunc)(command_name, abort, command_rc, reason, reply, app_context) ;

      zMapLogWarning("Bad request envelope: %s", err_msg) ;

      g_free(reason) ;
      g_free(err_msg) ;

      result = FALSE ;
    }

  /* Make sure there's a command specified. */
  if (result)
    {
      if (!(command_name = zMapRemoteCommandRequestGetCommand(remote->curr_peer_request)))
        {
          const char *reason = NULL ;

          command_rc = REMOTE_COMMAND_RC_BAD_XML ;
          reason = "Command is a required attribute for the \"request\" attribute." ;
          reply = NULL ;
        
          (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
        
          result = FALSE ;
        }
      else
        {
          if (remote->curr_peer_command)
            g_free(remote->curr_peer_command) ;
        
          remote->curr_peer_command = g_strdup(command_name) ;
        }
    }


  /* check for a view id and parse it out if there is one. */
  if (result)
    {
      char *id_str = NULL ;

      if (zMapRemoteCommandGetAttribute(remote->curr_peer_request,
                                        ZACP_REQUEST, ZACP_VIEWID, &(id_str),
                                        &err_msg))
        {
          if (zMapAppRemoteViewParseIDStr(id_str, (void **)&view_id))
            {
              remote->curr_view_id = (ZMapView)view_id ;
            }
          else
            {
              const char *reason = NULL ;

              command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
              reason = "Bad view_id !" ;
              reply = NULL ;
        
              (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
        
              result = FALSE ;
            }
        }
    }


  /* If a view_id was specified then check it actually exists. */
  if (result)
    {
      if (view_id)
        {
          if (!(zmap = zMapManagerFindZMap(app_context->zmap_manager, view_id, &view_ptr)))
            {
              RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
              char *reason ;
              ZMapXMLUtilsEventStack reply = NULL ;
        
              reason = g_strdup_printf("view id %p not found.", view_id) ;
        
              (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
        
              g_free(reason) ;
              result = FALSE ;
            }
        }
    }

  /* If the command is known at this level then call the command processing code for
   * this (App) level. If the command is not known then pass it down to the next level
   * and so on, if the command is not known at any level then we send back an error message. */
  if (result)
    {
      if ((strcmp(command_name, ZACP_PING) == 0
           || strcmp(command_name, ZACP_SHUTDOWN) == 0))
        {
          localProcessRemoteRequest(app_context,
                                    command_name, request,
                                    replyHandlerFunc, app_context) ;
        }
      else if (!(result = zMapManagerProcessRemoteRequest(app_context->zmap_manager,
                                                          command_name, request,
                                                          zmap, view_ptr,
                                                          replyHandlerFunc, app_context)))
        {
          RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_CMD_UNKNOWN ;
          const char *reason = "Command not known !" ;
          ZMapXMLUtilsEventStack reply = NULL ;
        
          (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
        }
    }

  /* Reset current stuff. */
  remote->curr_view_id = NULL ;

  return ;
}




/* Handles all incoming replies to zmap from a peer, there is no return result from
 * here because only the reply function that is called from here knows how to handle
 * any errors. */
void zmapAppProcessAnyReply(ZMapAppContext app_context, char *reply)
{
  ZMapAppRemote remote = app_context->remote_control ;
  gboolean reply_ok ;
  char *command = NULL ;
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  char *reply_body = NULL ;
  char *error = NULL ;

  /* Revisit this .....we are creating a problem for ourselves in that the process_reply_func
   * needs to be be called even if the reply is badly formed so that it can reset state etc.... */

  /* Need to parse out reply components..... */
  if (!(reply_ok = zMapRemoteCommandReplyGetAttributes(reply,
                                                       &command,
                                                       &command_rc, &reason,
                                                       &reply_body,
                                                       &error)))
    {
      /* Log the problem with the reply. */
      zMapLogCritical("Could not parse peer programs reply because \"%s\", their reply was: \"%s\"",
                      error, reply) ;
    }


  /* Return the reply to the relevant zmap sub-system, note if the reply could not be parsed then
   * command will be NULL, all called functions must cope with this. */
  (remote->process_reply_func)(reply_ok, error, command, command_rc, reason, reply, remote->process_reply_func_data) ;

  if (error)
    g_free(error) ;


  return ;
}



/* Connect to peer. */
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  ZMapAppRemote remote = app_context->remote_control ;
  ZMapRemoteAppMakeRequestFunc request_func ;
  ZMapXMLUtilsEventStack request_body ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  request_body = createPeerElement(remote->app_socket) ;

  (request_func)(app_context,
                 NULL,
                 ZACP_HANDSHAKE, request_body,
                 localProcessReplyFunc, app_context,
                 remoteReplyErrHandler, app_context) ;

  return result ;
}


/* Ping peer. */
gboolean zmapAppRemoteControlPing(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  ZMapRemoteAppMakeRequestFunc request_func ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  (request_func)(app_context,
                 NULL,
                 ZACP_PING, NULL,
                 localProcessReplyFunc, app_context,
                 remoteReplyErrHandler, app_context) ;

  return result ;
}


/* Disconnect from peer. */
gboolean zmapAppRemoteControlDisconnect(ZMapAppContext app_context, gboolean app_exit)
{
  gboolean result = TRUE ;
  ZMapRemoteAppMakeRequestFunc request_func ;
  ZMapXMLUtilsEventStack goodbye_type = NULL ;

  zmapAppPingStop(app_context) ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  if (app_exit)
    {
      /* Add element to say we are going to exit. */
      goodbye_type = zMapRemoteCommandCreateElement(ZACP_GOODBYE, ZACP_GOODBYE_TYPE, ZACP_EXIT) ;
    }

  /* Sets app_context->remote_ok to FALSE if disconnect fails. */
  (request_func)(app_context,
                 NULL,
                 ZACP_GOODBYE, goodbye_type,
                 localProcessReplyFunc, app_context,
                 remoteReplyErrHandler, app_context) ;
  result = app_context->remote_ok ;

  return result ;
}


/* Gets called by remotecontrol right at the end of the transaction to process a request we sent. */
void zmapAppRemoteControlOurRequestEndedCB(void *user_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


/* Gets called by remotecontrol right at the end of the transaction for cases where we need
 * to process a request we received _after_ the reply has reached the peer, e.g. shutdown...
 * 
 * Slightly tricky coding here. We can't just shutdown here as we were called from RemoteControl
 * and this would cause errors in that code. Instead we set a timeout so that we return to
 * RemoteControl which returns the gtk mainloop and then our timeout routine gets called which
 * then does the shutdown. (we don't use a "sendevent" because stupid gtk will do a direct
 * function call to our sendevent handler instead of putting the event on the event stack
 * which would leave us with the same problem..big sigh...).
 * 
 */
void zmapAppRemoteControlReplySent(ZMapAppContext app_context)
{
  if (app_context->remote_control->deferred_action)
    {
      int exit_timeout = 500 ;    /* time in milliseconds. */

      g_timeout_add(exit_timeout, deferredRequestHandler, (gpointer)app_context) ;
    }

  return ;
}




/* 
 *                       Internal routines.
 */



/* Creates:
 * 
 *       <peer socket_id="XXXX"/>
 * 
 * e.g.
 *       <peer socket_id="tcp://127.0.0.1:9999"/>
 * 
 *  */
static ZMapXMLUtilsEventStack createPeerElement(char *socket_id)
{
  static ZMapXMLUtilsEventStackStruct
    peer[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_PEER,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_SOCKET_ID, ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_PEER,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  /* Fill in peer element attributes. */
  peer[1].value.q = g_quark_from_string(socket_id) ;

  return &peer[0] ;
}


/* A GSourceFunc, called after we have finished any processing to do with
 * current request, e.g. to service a shutdown. This function is a timeout
 * routine called from the gtk mainloop. */
static gboolean deferredRequestHandler(gpointer data)
{
  ZMapAppContext app_context = (ZMapAppContext)data ;
  ZMapAppRemote remote = app_context->remote_control ;

  if (strcmp(remote->curr_peer_command, ZACP_SHUTDOWN) == 0)
    {
      zmapAppExit(app_context) ;
    }

  return FALSE ;
}


/* This is where we process a command and return the result via the callback which allows
 * for asynch returning of values where needed. */
static void localProcessRemoteRequest(gpointer local_data,
                                      char *command_name, char *request,
                                      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  ZMapAppContext app_context = (ZMapAppContext)local_data ;
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  processRequest(app_context, command_name, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}



/* This is where we process a reply received from a peer. */
static void localProcessReplyFunc(gboolean reply_ok, char *reply_error,
                                  char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
                                  gpointer reply_handler_func_data)
{
  ZMapAppContext app_context = (ZMapAppContext)reply_handler_func_data ;
  char *command_err_msg = NULL ;

  if (!reply_ok)
    {
      zMapLogCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }
  else
    {
      if (command_rc != REMOTE_COMMAND_RC_OK)
        {
          command_err_msg = g_strdup_printf("Peer reported error in reply for command \"%s\":"
                                            " %s, \"%s\"",
                                            command, zMapRemoteCommandRC2Str(command_rc), reason) ;
        }

      if (strcmp(command, ZACP_HANDSHAKE) == 0)
        {
          gboolean result = FALSE ;
          char *full_err_str = NULL ;
        
          if (command_rc != REMOTE_COMMAND_RC_OK)
            {
              full_err_str = g_strdup(command_err_msg) ;
        
              result = FALSE ;
            }
          else
            {
              result = TRUE ;
            }

          if (!result)
            {
              zMapLogCritical("Cannot continue because %s failed: %s", ZACP_HANDSHAKE, full_err_str) ;
        
              zMapCritical("Cannot continue because %s failed: %s", ZACP_HANDSHAKE, full_err_str) ;
        
              g_free(full_err_str) ;
        
              (app_context->remote_control->exit_routine)(app_context) ;
            }
        }
      else if (strcmp(command, ZACP_GOODBYE) == 0)
        {
          if (command_rc != REMOTE_COMMAND_RC_OK)
            zMapLogCritical("%s", command_err_msg) ;
        
          zmapAppExit(app_context) ;
        }

      if (command_err_msg)
        g_free(command_err_msg) ;
    }

  return ;
}


/* It's a command we know so get on and process it, result is returned in command_rc et al.
 * 
 * Note that some commands (ping, shutdown) have no arguments do not need parsing.
 *  */
static void processRequest(ZMapAppContext app_context,
                           char *command_name, char *request,
                           RemoteCommandRCType *command_rc_out, char **reason_out,
                           ZMapXMLUtilsEventStack *reply_out)
{

  if (strcmp(command_name, ZACP_SHUTDOWN) == 0)
    {
      char *shutdown_type = NULL ;
      char *err_msg = NULL ;

      /* If our peer asked us to abort then we exit immediately right here..... */
      if (zMapRemoteCommandGetAttribute(request,
                                        ZACP_SHUTDOWN_TAG, ZACP_SHUTDOWN_TYPE, &shutdown_type,
                                        &err_msg)
          && strcmp(shutdown_type, ZACP_ABORT) == 0)
        {
          gtk_exit(EXIT_FAILURE) ;
        }
      else
        {
          /* Although we return a result here, the shutdown needs to be deferred until our reply gets
           * through to the peer.  We initiate the shutdown from the remote callback which is called
           * right at the end of the transaction and get that to work off a timeout routine. */
          *command_rc_out = REMOTE_COMMAND_RC_OK ;
          *reason_out = NULL ;
        
          *reply_out = zMapRemoteCommandMessage2Element("zmap shutting down now !") ;
        
          app_context->remote_control->deferred_action = TRUE ;
        }
    }
  else if (strcmp(command_name, ZACP_PING) == 0)
    {
      *command_rc_out = REMOTE_COMMAND_RC_OK ;
      *reason_out = NULL ;

      *reply_out = zMapRemoteCommandMessage2Element("ping ok !") ;
    }

  return ;
}





static void remoteReplyErrHandler(ZMapRemoteControlRCType error_type, char *err_msg, void *user_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Unused currently. */

  ZMapAppContext app_context = (ZMapAppContext)user_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






  return ;
}


