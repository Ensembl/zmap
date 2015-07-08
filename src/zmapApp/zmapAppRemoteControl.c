/*  File: zmapAppRemoteControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010-2015: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Application level functions to set up ZMap remote control
 *              and communicate with a peer program.
 *              
 *              This code provides the interface between ZMapRemoteControl
 *              and the _whole_ of the rest of ZMap. It doesn't "know"
 *              about the individual commands it just ensures that
 *              requests and replies get moved between the remote control
 *              code and ZMap and back.
 *
 * Exported functions: See zmapApp_P.h
 *
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib.h>
#include <gdk/gdk.h>



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapRemoteCommand.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <ZMap/zmapAppRemote.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <ZMap/zmapCmdLineArgs.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

/* NEEDED ??? */
#include <zmapApp_P.h>

#include <zmapAppRemoteInternal.h>



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Timeout list for remote control for sends. */
#define ZMAP_WINDOW_REMOTECONTROL_TIMEOUT_LIST "333,1000,3000,9000"

/* Initially we don't know the remote programs name so default to this... */
#define ZMAP_APP_REMOTE_PEER "Remote Peer"


static void receivedPeerRequestCB(ZMapRemoteControl remote_control,
                                  ZMapRemoteControlReturnReplyFunc remote_reply_func, void *remote_reply_data,
                                  char *request, void *user_data) ;
static void sendZmapReplyCB(const char *command, gboolean abort, RemoteCommandRCType result, const char *reason,
                            ZMapXMLUtilsEventStack reply, gpointer app_reply_data) ;
static void zmapReplySentCB(void *user_data) ;

static void sendZMapRequestCB(gpointer caller_data,
                              gpointer sub_system_ptr,
                              const char *command, ZMapXMLUtilsEventStack request_body,
                              ZMapRemoteAppProcessReplyFunc reply_func, gpointer reply_func_data,
                              ZMapRemoteAppErrorHandlerFunc error_handler_func, gpointer error_handler_func_data) ;
static void zmapRequestSentCB(void *user_data) ;
static void receivedPeerReplyCB(ZMapRemoteControl remote_control, char *reply, void *user_data) ;

static void errorHandlerCB(ZMapRemoteControl remote_control,
                           ZMapRemoteControlRCType error_type, char *err_msg,
                           void *user_data) ;

static void setDebugLevel(void) ;
static void setModal(ZMapAppContext app_context, gboolean modal) ;
static void setCursorCB(ZMap zmap, void *user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* 
 *                Globals. 
 */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Set debug level in remote control. */
static ZMapRemoteControlDebugLevelType remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_OFF ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/* 
 *                       External interface.
 */


gboolean zmapAppRemoteControlIsAvailable(void)
{
  gboolean result = FALSE ;

#ifdef USE_ZEROMQ
  result = TRUE ;
#endif

  return result ;
}




/* Returns the function that sub-systems in zmap call when they have a request to be sent to a peer. */
ZMapRemoteAppMakeRequestFunc zmapAppRemoteControlGetRequestCB(void)
{
  ZMapRemoteAppMakeRequestFunc req_func = NULL ;

#ifdef USE_ZEROMQ
  req_func = zmapAppRemoteInternalGetRequestCB() ;
#endif

  return req_func ;
}



/* Set up the remote controller object. */
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context, char *peer_socket, const char *peer_timeout_list_in)
{
  gboolean result = FALSE ;

#ifdef USE_ZEROMQ
  result = zmapAppRemoteInternalCreate(app_context, peer_socket, peer_timeout_list_in) ;
#endif

  return result ;
}


/* Intialise the remotecontrol send/receive interfaces. */
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context)
{
  gboolean result = FALSE ;

#ifdef USE_ZEROMQ
  result = zmapAppRemoteInternalInit(app_context) ;
#endif

  return result ;
}


/* should return a boolean...... */
void zmapAppRemoteControlSetExitRoutine(ZMapAppContext app_context, ZMapAppRemoteExitFunc exit_routine)
{
#ifdef USE_ZEROMQ

  /* make this a function call...... */

  app_context->remote_control->exit_routine = exit_routine ;
#endif

  return ;
}


/* Destroy all resources for the remote controller object. */
void zmapAppRemoteControlDestroy(ZMapAppContext app_context)
{
#ifdef USE_ZEROMQ
  zmapAppRemoteInternalDestroy(app_context) ;
#endif

  return ;
}


/* 
 *                Internal functions
 */



/*
 * Functions called to service requests from a peer.
 */

/* Called by ZMapRemoteControl: receives all requests sent to zmap by a peer. */
static void receivedPeerRequestCB(ZMapRemoteControl remote_control,
                                  ZMapRemoteControlReturnReplyFunc return_reply_func,
                                  void *return_reply_func_data,
                                  char *request, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;

  /* Update last interaction time, used to detect when zmap/peer connection is inactive for a
   * long time and warn the user. */
  app_context->remote_control->last_active_time_s = zMapUtilsGetRawTime() ;

  /* Cache these, must be called from handleZMapRepliesCB() */
  remote->return_reply_func = return_reply_func ;
  remote->return_reply_func_data = return_reply_func_data ;

  /* Call the command processing code.... */
  zmapAppProcessAnyRequest(app_context, request, sendZmapReplyCB) ;

  /* try modal..... */
  zMapLogWarning("Setting modal on for request %s", request) ;
  setModal(app_context, TRUE) ;

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
static void sendZmapReplyCB(char *command,
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



/* Called by remote control when the reply has been sent to the peer,
 * i.e. the transaction has ended. Note that we don't actually know that
 * the peer receives the reply, this was a design decision with the
 * switch to zeromq.
 * 
 * This function allows us to take any actions necessary after
 * a reply has been sent.
 * 
 *  */
static void zmapReplySentCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  /* try modal..... */
  zMapLogWarning("%s", "Setting modal off") ;
  setModal(app_context, FALSE) ;

  /* Call function to execute any actions that must occur after
   * the reply is sent to peer. */
  zmapAppRemoteControlReplySent(app_context) ;

  return ;
}




/*
 * Functions called to send requests to a peer.
 */

/* Called by any component of ZMap (e.g. control, view etc) which needs a request
 * to be sent to the peer program. This is the entry point for zmap sub-systems
 * that wish to send a request to the peer.
 * 
 *  */
static void sendZMapRequestCB(gpointer caller_data,
                              gpointer sub_system_ptr,
                              char *command, ZMapXMLUtilsEventStack request_body,
                              ZMapRemoteAppProcessReplyFunc process_reply_func, gpointer process_reply_func_data,
                              ZMapRemoteAppErrorHandlerFunc error_handler_func,
                              gpointer error_handler_func_data)
{
  ZMapAppContext app_context = (ZMapAppContext)caller_data ;
  ZMapAppRemote remote = app_context->remote_control ;

  /* Update last interaction time. */
  app_context->remote_control->last_active_time_s = zMapUtilsGetRawTime() ;

  /* Can this even happen, how can we be called if there is no remote ??? */
  /* If we have a remote control peer then build the request.....surely this should be in assert.... */
  if (remote)
    {
      gboolean result = TRUE ;
      GArray *request_stack = NULL ;
      char *request ;
      char *view = NULL ;    /* to be filled in later..... */
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
          char *err_msg = NULL ;

          if (result && !(request_stack = zMapRemoteCommandCreateRequest(remote->remote_controller,
                                                                         command, -1,
                                                                         ZACP_VIEWID, view,
                                                                         (char*)NULL)))
            {
              err_msg = g_strdup_printf("Could not create request for command \"%s\"", command) ;

              result = FALSE ;

              /* for debugging... */
              request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE) ;
            }

      
          if (request_body)
            {
              if (result && !(request_stack = zMapRemoteCommandAddBody(request_stack, "request", request_body)))
                {
                  err_msg = g_strdup_printf("Could not add request body for command \"%s\"", command) ;

                  result = FALSE ;
                }
            }
        
          if (result && !(request = zMapXMLUtilsStack2XML(request_stack, &err_msg, FALSE)))
            {
              err_msg = g_strdup_printf("Could not create raw xml from request for command \"%s\"", command) ;

              result = FALSE ;
            }

          if (result)
            {
              remote->process_reply_func = process_reply_func ;
              remote->process_reply_func_data = process_reply_func_data ;

              remote->error_handler_func = error_handler_func ;
              remote->error_handler_func_data = error_handler_func_data ;

              if (!(result = zMapRemoteControlSendRequest(remote->remote_controller, request)))
                {
                  err_msg = g_strdup_printf("Could not send request to peer: \"%s\"", request) ;

                  result = FALSE ;
                }
              else
                {
                  zMapLogWarning("Just sent request to peer: \"%s\"", request) ;
                }
            }

          /* Serious failure, record that remote is no longer working properly. */
          if (!result)
            {
              app_context->remote_ok = FALSE ;
        
              zMapLogCritical("%s", err_msg) ;
        
              g_free(err_msg) ;
            }
        }

    }

  return ;
}


/* Called by remote control when request has been sent to peer, note that we do not know
 * the peer has actually received the request, this was a design decision when we switched
 * to using zeromq. */
static void zmapRequestSentCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  /* try modal..... */
  zMapLogWarning("%s", "Setting modal on to send zmap request") ;

  setModal(app_context, TRUE) ;

  return ;
}

/* 
 * Called by ZMapRemoteControl: Receives all replies sent to zmap by a peer.
 */
static void receivedPeerReplyCB(ZMapRemoteControl remote_control, char *reply, void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  gboolean result ;
  char *error_out = NULL ;

  zMapLogWarning("%s", "Setting modal off") ;
  setModal(app_context, FALSE) ;

  /* Call the command processing code, there's no return code here because only the function
   * ultimately handling the problem knows how to handle the error. */
  zmapAppProcessAnyReply(app_context, reply) ;

  return ;
}


/* Called by ZMapRemoteControl, receives all errors including timeouts. */
static void errorHandlerCB(ZMapRemoteControl remote_control,
                           ZMapRemoteControlRCType error_type, char *err_msg,
                           void *user_data)
{
  /* not used currently. */
  ZMapAppContext app_context = (ZMapAppContext)user_data ;
  ZMapAppRemote remote = app_context->remote_control ;
  GtkWindow *window ;
  gboolean curr_modal ;

  /* If we are dying the app_widg window will already have gone and calling back to error
   * handler funcs will be too late. */
  if (app_context->state != ZMAPAPP_DYING)
    {
      /* Get current modality, if it's on we must turn it off otherwise user cannot
       * interact with zmap. */
      window = GTK_WINDOW(app_context->app_widg) ;
      if ((curr_modal = gtk_window_get_modal(window)))
        setModal(app_context, FALSE) ;
  
      if (remote->error_handler_func)
        (remote->error_handler_func)(error_type, err_msg, remote->error_handler_func_data) ;
      else
        zMapLogWarning("%s", err_msg) ;
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



static void setModal(ZMapAppContext app_context, gboolean modal)
{
  GdkCursor *curr_cursor ;
  GtkWindow *window ;
  gboolean curr_modal ;

  /* Get current modality */
  window = GTK_WINDOW(app_context->app_widg) ;
  curr_modal = gtk_window_get_modal(window) ;


  /* Set modality. */
  if (curr_modal == modal)
    {
      char *msg ;

      msg = g_strdup_printf("Trying to set Modal %s but already %s",
                            (modal ? "ON" : "OFF"), (modal ? "ON" : "OFF")) ;

      zMapLogCritical("%s", msg) ;
    }

  gtk_window_set_modal(window, modal) ;

  /* Set/reset the cursor on the app window and all ZMap windows below. */
  curr_cursor = (modal ? app_context->remote_busy_cursor : app_context->normal_cursor) ;

  zMapGUISetCursor(app_context->app_widg, curr_cursor) ;

  zMapManagerForAllZMaps(app_context->zmap_manager, setCursorCB, curr_cursor) ;


  zMapLogWarning("Modal - %s", (modal ? "ON" : "OFF")) ;

  return ;
}


static void setCursorCB(ZMap zmap, void *user_data)
{
  GdkCursor *cursor = (GdkCursor *)user_data ;
 
  zMapSetCursor(zmap, cursor) ;

  return ;
}
