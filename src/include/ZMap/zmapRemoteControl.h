/*  File: zmapRemoteControl.h
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *      Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
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

#include <ZMap/zmapEnum.h>				    /* for the XXX_LIST macros. */
#include <ZMap/zmapRemoteProtocol.h>



/* Opaque type representing a single peer-peer pair. There must be one of these for each pair
 * of peers that wish to communicate. */
typedef struct ZMapRemoteControlStructName *ZMapRemoteControl ;



/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */



/* Error code values for Remote Control errors. */
#define ZMAP_REMOTECONTROL_RC_LIST(_)					                                \
_(ZMAP_REMOTECONTROL_RC_OK,            , "ok",             "No error.",                             "") \
_(ZMAP_REMOTECONTROL_RC_TIMED_OUT,     , "timed_out",      "Timed out, peer not replying in time.", "") \
_(ZMAP_REMOTECONTROL_RC_APP_ABORT,     , "app_abort",      "Application has aborted transaction.",  "") \
_(ZMAP_REMOTECONTROL_RC_OUT_OF_BAND,   , "out_of_band",    "Peer is out of synch.",                 "") \
_(ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD, , "bad_clipboard",  "Clipboard error.",                      "") \
_(ZMAP_REMOTECONTROL_RC_BAD_STATE,     , "bad_state",      "Internal error, bad state detected.",   "")

ZMAP_DEFINE_ENUM(ZMapRemoteControlRCType, ZMAP_REMOTECONTROL_RC_LIST) ;


/* Debugging levels. */
typedef enum {ZMAP_REMOTECONTROL_DEBUG_OFF,
	      ZMAP_REMOTECONTROL_DEBUG_NORMAL, ZMAP_REMOTECONTROL_DEBUG_VERBOSE} ZMapRemoteControlDebugLevelType ;


/* 
 *                         Callback definitions.
 */


/* Error Handling: */

/* Apps callback for handling errors from remote_control, e.g. timeouts. */
typedef void (*ZMapRemoteControlErrorHandlerFunc)(ZMapRemoteControl remote_control,
						  ZMapRemoteControlRCType error_type, char *err_msg,
						  void *user_data) ;

/* Apps callback that remote_control should call to report errors. */
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
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_data,
					  ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;

gboolean zMapRemoteControlReceiveInit(ZMapRemoteControl remote_control,
				      char *app_str,
				      ZMapRemoteControlRequestHandlerFunc request_handler_func,
				      gpointer request_handler_func_data,
				      ZMapRemoteControlReplySentFunc reply_sent_func,
				      gpointer reply_sent_func_data) ;
gboolean zMapRemoteControlSendInit(ZMapRemoteControl remote_control,
				   char *send_app_name, char *peer_unique_str,
				   ZMapRemoteControlRequestSentFunc req_sent_func,
				   gpointer req_sent_func_data,
				   ZMapRemoteControlReplyHandlerFunc reply_handler_func,
				   gpointer reply_handler_func_data) ;

gboolean zMapRemoteControlReceiveWaitForRequest(ZMapRemoteControl remote_control) ;
gboolean zMapRemoteControlSendRequest(ZMapRemoteControl remote_control, char *peer_xml_request) ;

void zMapRemoteControlReset(ZMapRemoteControl remote_control) ;
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, ZMapRemoteControlDebugLevelType debug_level) ;
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapRemoteControlRCType2ExactStr, ZMapRemoteControlRCType) ;

#endif /* ZMAP_REMOTE_CONTROL_H */
