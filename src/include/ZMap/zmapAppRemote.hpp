/*  File: zmapAppRemote.h
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
 * Description: Provides an interface between the zmap application
 *              components and the ZMapRemoteControl interface.
 *              
 *              All peer requests/replies and all zmap component
 *              request/relies are routed through this interface.
 * 
 *              Since these functions are zmap specific they are in
 *              a separate header from the other remote stuff. The
 *              associated code is in zmapApp.
 *
 * Created: Thu Dec 15 17:39:32 2011 (edgrif)
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_APP_REMOTE_H
#define ZMAP_APP_REMOTE_H

#include <glib.h>

#include <ZMap/zmapRemoteCommand.hpp>
#include <ZMap/zmapXML.hpp>



/* All ZMap sub-systems (Mananger, Control etc) that accept or generate
 * xremote requests/replies must use the functions defined here to make
 * and reply to requests. The code that services this interface is in
 * zmapApp/zmapAppRemoteCommand.c which provides the interface between
 * the zmap sub-systems and ZMapRemoteControl.
 * 
 * The functions that respond to requests received by zmapAppRemoteCommand
 * are defined in the public header for the sub-system and then called
 * by name by the sub-system's parent.
 * 
 * In contrast the zmapAppRemoteCommand function that must be called by the sub-system
 * to make a request will be passed from parent to child sub-system using
 * the existing sub-system callback mechanism to avoid introducing circular 
 * compile dependencies.
 */



/* Function that sub-system can provide to be called on error.
 * When remotecontrol detects an error in transmitting/receiving messages it calls
 * the sub-system with the type of error and an error message for display or logging.
 * user_data enables the sub-system to pass through any data it needs to handle
 * the error. */
typedef void (*ZMapRemoteAppErrorHandlerFunc)(ZMapRemoteControlRCType error_type,
                                              char *err_msg,
                                              void *user_data) ;


/* 
 *  Functions that sub-systems must provide/call to _RESPOND_ to requests from a peer.
 */

/* Function that sub-system must call to return a reply for a request it has processed.
 * The sub-system must return the return code to indicate whether the request
 * succeeded and if it succeeded a reply string or if it failed a "reason" string
 * explaining the failure.
 * 
 * Note that function returns nothing because the sub-system cannot sensibly respond
 * to any error from this callback, it just returns the result which is then
 * returned to the peer program. */
typedef void (*ZMapRemoteAppReturnReplyFunc)(const char *command,
					     gboolean abort,
					     RemoteCommandRCType command_rc,
					     const char *reason,
					     ZMapXMLUtilsEventStack reply,
					     gpointer app_reply_data) ;

/* All sub-systems must provide a function with this prototype to respond to requests.
 * When they have processed the request their reply _MUST_ be returned by calling
 * the app_reply_func and returning the command_rc, reason or reply and app_reply_data
 * as per the ZMapRemoteAppReturnReplyFunc prototype.
 * 
 * Note that the local_data pointer will be a ZMapView, ZMapWindow etc depending
 * on the sub-system. */
typedef void (*ZMapRemoteAppProcessRequestFunc)(gpointer sub_system,
						char *command_name, char *command,
						ZMapRemoteAppReturnReplyFunc app_reply_func,
						gpointer app_reply_data) ;



/* 
 *  Functions that sub-systems must call/provide to _MAKE_ requests to a peer.
 */

/* All sub-systems must provide a function with this prototype to process replies to
 * their requests.
 * 
 * If the reply could be parsed then reply_ok == TRUE and command, command_rc,
 * reason and reply are set appropriately otherwise reply_ok == FALSE and reply_error
 * gives an error string the function can display to the user if required.
 *  */
typedef void (*ZMapRemoteAppProcessReplyFunc)(gboolean reply_ok, char *reply_error,
					      char *command,
					      RemoteCommandRCType command_rc,
					      char *reason,
					      char *reply,
					      gpointer reply_handler_func_data) ;


/* Function that sub-system must call when it has a request it wants sent to the peer.
 * When a reply is received from the peer the sub-systems reply_func will be called back
 * with the command_rc, reason or reply and the reply_func_data as per the
 * ZMapRemoteAppProcessReplyFunc prototype. */
typedef void (*ZMapRemoteAppMakeRequestFunc)(gpointer caller_data,
					     gpointer sub_system_ptr,
					     const char *command, ZMapXMLUtilsEventStack request_body,
					     ZMapRemoteAppProcessReplyFunc reply_handler_func,
					     gpointer reply_handler_func_data,
                                             ZMapRemoteAppErrorHandlerFunc error_handler_func,
                                             gpointer error_handler_func_data) ;


typedef void (*ZMapAppRemoteExitFunc)(gpointer cb_data) ;


/* Defines IDs that represent a "view" of the data, there is one of these for every
 * view, i.e. set of sequence data/features. */
gboolean zMapAppRemoteViewCreateIDStr(gpointer remote_view_id, char **view_id_str_out) ;
gboolean zMapAppRemoteViewCreateID(gpointer remote_view_id, GQuark *view_id_out) ;
gboolean zMapAppRemoteViewParseIDStr(char *view_id_str, gpointer *remote_view_id_inout) ;
gboolean zMapAppRemoteViewParseID(GQuark view_id, gpointer *remote_view_id_inout) ;


#endif /* ZMAP_APP_REMOTE_H */
