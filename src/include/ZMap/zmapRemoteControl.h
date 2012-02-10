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
 * Last edited: Dec  1 16:01 2011 (edgrif)
 * Created: Fri Sep 24 14:51:35 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL_H
#define ZMAP_REMOTE_CONTROL_H

#include <gtk/gtk.h>

#include <ZMap/zmapRemoteProtocol.h>



/* Opaque type representing a single peer-peer pair. There must be one of these for each pair
 * of peers that wish to communicate. */
typedef struct ZMapRemoteControlStructName *ZMapRemoteControl ;




/* 
 *                         Callback definitions.
 */

/* Apps function called when a RemoteControl has sent a request and received the reply
 * and the whole transaction is finished. */
typedef void (*ZMapRemoteControlReportRequestSentEndFunc)(void *user_data) ;

/* Apps function called when a RemoteControl has received a request and sent the reply
 * and the whole transaction is finished. */
typedef void (*ZMapRemoteControlReportRequestReceivedEndFunc)(void *user_data) ;

/* Apps function for handling errors from remote_control, e.g. timeouts. */
typedef gboolean (*ZMapRemoteControlErrorHandlerFunc)(ZMapRemoteControl remote_control, void *user_data) ;

/* Apps function that remote_control should call to report errors. */
typedef gboolean (*ZMapRemoteControlErrorReportFunc)(void *user_data, char *err_msg) ;




/* RemoteControl callback, called by app after processing a request to return the reply
 * to remote_control to be sent to the peer. */
typedef void (*ZMapRemoteControlCallWithReplyFunc)(void *remote_data, char *reply) ;

/* RemoteControl callback, called by app to signal that it has finished processing the reply
 * from a peer. */
typedef void (*ZMapRemoteControlCallReplyDoneFunc)(void *remote_data) ;



/* App callback, called by remote_control to pass App the request it has received from a peer.
 * Once app has processed the request it should call remote_request_done_func to return the
 * reply to remotecontrol. */
typedef void (*ZMapRemoteControlRequestHandlerFunc)(ZMapRemoteControl remote_control,
						    ZMapRemoteControlCallWithReplyFunc remote_request_done_func,
						    void *remote_request_done_data,
						    char *request, void *user_data) ;

/* App callback, called by remote_control to pass App the reply to a request App made to its peer.
 * Once App has processed the reply it should call remote_reply_done_func to signal remotecontrol
 * that it's processed the reply. */
typedef void (*ZMapRemoteControlReplyHandlerFunc)(ZMapRemoteControl remote_control,
						  ZMapRemoteControlCallReplyDoneFunc remote_reply_done_func,
						  void *remote_reply_done_data,
						  char *reply, void *user_data) ;




/* 
 *            The RemoteControl interface.
 */
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlReportRequestSentEndFunc req_sent_func,
					  gpointer req_sent_func_data,
					  ZMapRemoteControlReportRequestReceivedEndFunc req_received_func,
					  gpointer req_received_func_data,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_data,
					  ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;

gboolean zMapRemoteControlReceiveInit(ZMapRemoteControl remote_control,
				      char *app_str,
				      ZMapRemoteControlRequestHandlerFunc process_request_func,
				      gpointer process_request_func_data) ;
gboolean zMapRemoteControlReceiveWaitForRequest(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlSendInit(ZMapRemoteControl remote_control,
				   char *send_app_name, char *peer_unique_str,
				   ZMapRemoteControlReplyHandlerFunc process_reply_func,
				   gpointer process_reply_funcdata) ;
gboolean zMapRemoteControlSendRequest(ZMapRemoteControl remote_control, char *peer_xml_request) ;

gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on) ;
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control) ;
void zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL_H */
