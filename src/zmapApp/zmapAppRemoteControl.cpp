/*  File: zmapAppRemoteControl.c
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


#include <config.h>
#include <ZMap/zmapAppRemote.hpp>

/* NEEDED ??? */
#include <zmapApp_P.hpp>

#include <zmapAppRemoteInternal.hpp>



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

