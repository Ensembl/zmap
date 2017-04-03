/*  File: zmapRemoteControl.h
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
 * Description: External interface to remote control package.
 *              These functions handle the sending/receiving of messages
 *              and check that requests/responses have matching ids,
 *              are the right protocol version.
 *
 * HISTORY:
 * Last edited: Apr 11 09:39 2012 (edgrif)
 * Created: Fri Sep 24 14:51:35 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL_H
#define ZMAP_REMOTE_CONTROL_H

#include <gtk/gtk.h>

#include <ZMap/zmapEnum.hpp>				    /* for the XXX_LIST macros. */
#include <ZMap/zmapRemoteProtocol.hpp>



/* Opaque type representing a single peer-peer pair. There must be one of these for each pair
 * of peers that wish to communicate. */
typedef struct ZMapRemoteControlStructType *ZMapRemoteControl ;



/* Error code values for Remote Control errors. */
#define ZMAP_REMOTECONTROL_RC_LIST(_)					                                \
_(ZMAP_REMOTECONTROL_RC_OK,            , "ok",             "No error.",                             "") \
_(ZMAP_REMOTECONTROL_RC_TIMED_OUT,     , "timed_out",      "Timed out, peer not replying in time.", "") \
_(ZMAP_REMOTECONTROL_RC_APP_ABORT,     , "app_abort",      "Application has aborted transaction.",  "") \
_(ZMAP_REMOTECONTROL_RC_OUT_OF_BAND,   , "out_of_band",    "Peer is out of synch.",                 "") \
_(ZMAP_REMOTECONTROL_RC_BAD_SOCKET,    , "bad_socket",     "Socket error.",                         "") \
_(ZMAP_REMOTECONTROL_RC_BAD_STATE,     , "bad_state",      "Internal error, bad state detected.",   "")

ZMAP_DEFINE_ENUM(ZMapRemoteControlRCType, ZMAP_REMOTECONTROL_RC_LIST) ;


/* Debugging levels, must be in ascending order of verboseness. */
typedef enum {ZMAP_REMOTECONTROL_DEBUG_OFF,
	      ZMAP_REMOTECONTROL_DEBUG_NORMAL, ZMAP_REMOTECONTROL_DEBUG_VERBOSE} ZMapRemoteControlDebugLevelType ;


/* 
 *                         Callback definitions.
 */


/* Error Handling: */

/* Apps callback that remote_control should call to allow app to handle errors, e.g. peer not responding. */
typedef void (*ZMapRemoteControlErrorHandlerFunc)(ZMapRemoteControl remote_control,
						  ZMapRemoteControlRCType error_type, char *err_msg,
						  void *user_data) ;

/* Apps callback that remote_control can call to output/log errors. */
typedef gboolean (*ZMapRemoteControlErrorReportFunc)(void *user_data, char *err_msg) ;



/* Receiving requests: */

/* RemoteControl callback that app _must_ call after processing a request,
 * whether there is an error or not, to return the reply/error to remote_control
 * to be sent to back to the requesting peer. If abort is TRUE then reply must be NULL
 * and remotecontrol will abort the transaction. */
typedef void (*ZMapRemoteControlReturnReplyFunc)(void *return_reply_func_data,
						 gboolean abort, char *reply) ;


/* App callback, called by remote_control to pass App the request it has received from a peer.
 * Once app has processed the request it should call remote_request_done_func to return the
 * reply to remotecontrol. */
typedef void (*ZMapRemoteControlRequestHandlerFunc)(ZMapRemoteControl remote_control,
						    ZMapRemoteControlReturnReplyFunc return_reply_func,
						    void *return_reply_func_data,
						    char *request, void *user_data) ;

/* Optional app function that RemoteControl calls when peer has signalled
 * that it has received app's request. Enables app that has serviced the
 * request to set state/take action once it knows its reply has been received. */
typedef void (*ZMapRemoteControlReplySentFunc)(void *user_data) ;




/* Sending requests: */

/* Optional app function that RemoteControl will call when peer has signalled
 * that it has received app's request. Enables app that made the request to
 * set state/take any action it needs once its request is delivered. */
typedef void (*ZMapRemoteControlRequestSentFunc)(void *user_data) ;

/* App callback, called by remote_control to pass App the reply to a request app made to its
 * peer. */
typedef void (*ZMapRemoteControlReplyHandlerFunc)(ZMapRemoteControl remote_control,
						  char *reply, void *user_data) ;




/* 
 *            The RemoteControl interface.
 */
ZMapRemoteControl zMapRemoteControlCreate(const char *app_id,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_data,
					  ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data) ;

gboolean zMapRemoteControlReceiveInit(ZMapRemoteControl remote_control,
                                      char **app_socket_out,
				      ZMapRemoteControlRequestHandlerFunc request_handler_func,
				      gpointer request_handler_func_data,
				      ZMapRemoteControlReplySentFunc reply_sent_func,
				      gpointer reply_sent_func_data) ;
gboolean zMapRemoteControlSendInit(ZMapRemoteControl remote_control,
                                   const char *peer_socket,
				   ZMapRemoteControlRequestSentFunc req_sent_func,
				   gpointer req_sent_func_data,
				   ZMapRemoteControlReplyHandlerFunc reply_handler_func,
				   gpointer reply_handler_func_data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapRemoteControlReceiveWaitForRequest(ZMapRemoteControl remote_control) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

gboolean zMapRemoteControlSendRequest(ZMapRemoteControl remote_control, char *peer_xml_request) ;


/* Not too sure about this...this is actually quite complex...better to destroy and recreate. */
gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlHasFailed(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, ZMapRemoteControlDebugLevelType debug_level) ;
gboolean zMapRemoteControlSetTimeoutList(ZMapRemoteControl remote_control, const char *peer_timeout_list) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control) ;

void zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapRemoteControlRCType2ExactStr, ZMapRemoteControlRCType) ;

#endif /* ZMAP_REMOTE_CONTROL_H */
