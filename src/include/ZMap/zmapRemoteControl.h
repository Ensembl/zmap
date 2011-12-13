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


/* RemoteControl callback. */
typedef gboolean (*ZMapRemoteControlCallWithReplyFunc)(void *remote_data, void *reply, int reply_len) ;

/* App Callbacks, will be called by remotecontrol when a request, a reply or an error occurs respectively. */
typedef gboolean (*ZMapRemoteControlRequestHandlerFunc)(ZMapRemoteControl remote_control,
							ZMapRemoteControlCallWithReplyFunc remote_reply_func,
							void *remote_reply_data,
							void *request, void *user_data) ;
typedef gboolean (*ZMapRemoteControlReplyHandlerFunc)(ZMapRemoteControl remote_control, void *reply, void *user_data) ;
typedef gboolean (*ZMapRemoteControlTimeoutHandlerFunc)(ZMapRemoteControl remote_control, void *user_data) ;


/* do we need an error handler callback here....perhaps one of the errors should be a timeout ??
   and not have a specific timeout handler callback....better.... */


/* App-defined function to output remote control errors. */
typedef gboolean (*ZMapRemoteControlErrorReportFunc)(void *user_data, char *err_msg) ;


/* 
 *            The RemoteControl interface.
 */
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;

gboolean zMapRemoteControlSelfInit(ZMapRemoteControl remote_control,
				   char *app_str,
				   ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_data,
				   ZMapRemoteControlTimeoutHandlerFunc timeout_func, gpointer timeout_data) ;

/* I think we need to do this....split out from above call the stuff to set up the clipboard. */
gboolean zMapRemoteControlSelfWaitForRequest(ZMapRemoteControl remote_control) ;

gboolean zMapRemoteControlPeerInit(ZMapRemoteControl remote_control,
				   char *peer_unique_str,
				   ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_data,
				   ZMapRemoteControlTimeoutHandlerFunc timeout_func, gpointer timeout_data) ;
gboolean zMapRemoteControlPeerRequest(ZMapRemoteControl remote_control, char *peer_xml_request) ;

gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control) ;
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on) ;
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs) ;
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
				     ZMapRemoteControlErrorReportFunc err_func, gpointer err_data) ;
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control) ;
void zMapRemoteControlDestroy(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL_H */
