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


/* RemoteControl callback, allows app to do asynch processing and then return the reply to remote_control. */
typedef gboolean (*ZMapRemoteControlCallWithReplyFunc)(void *remote_data, void *reply, int reply_len) ;

/* App function called by remote_control to pass App the request it has receieved from a peer. */
typedef gboolean (*ZMapRemoteControlRequestHandlerFunc)(ZMapRemoteControl remote_control,
							ZMapRemoteControlCallWithReplyFunc remote_reply_func,
							void *remote_reply_data,
							void *request, void *user_data) ;

/* We may need to have an mechanism for async reply processing here too..... */
typedef gboolean (*ZMapRemoteControlReplyHandlerFunc)(ZMapRemoteControl remote_control,
						      void *reply, void *user_data) ;

/* Apps function for handling errors from remote_control, e.g. timeouts. */
typedef gboolean (*ZMapRemoteControlErrorHandlerFunc)(ZMapRemoteControl remote_control, void *user_data) ;

/* Apps function that remote_control should call to report errors. */
typedef gboolean (*ZMapRemoteControlErrorReportFunc)(void *user_data, char *err_msg) ;



/* 
 *            The RemoteControl interface.
 */
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_data,
					  ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;

gboolean zMapRemoteControlSelfInit(ZMapRemoteControl remote_control,
				   char *app_str,
				   ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_data) ;
gboolean zMapRemoteControlPeerInit(ZMapRemoteControl remote_control,
				   char *peer_unique_str,
				   ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_data) ;

gboolean zMapRemoteControlSelfWaitForRequest(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlPeerRequest(ZMapRemoteControl remote_control, char *peer_xml_request) ;

gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control) ;
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on) ;
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control) ;

void zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL_H */
