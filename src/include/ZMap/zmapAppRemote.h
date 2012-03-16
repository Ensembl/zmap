/*  Last edited: Feb 14 20:24 2012 (edgrif) */
/*  File: zmapAppRemote.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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

#include <ZMap/zmapRemoteCommand.h>
#include <ZMap/zmapXML.h>




/* To make sure that commands are delivered to the right component of zmap
 * (ZMap, ZMapView or ZMapWindow) a struct containing pointers to these
 * components is passed to the request handling functions. If the component
 * does not exist then this is an error. */
typedef struct ZMapAppRemoteViewIDStructName
{
  void *zmap ;
  void *view ;
  void *window ;
} ZMapAppRemoteViewIDStruct, *ZMapAppRemoteViewID ;



/* DOES THIS NEED TO BE EXPOSED HERE.....CHECK AGAIN... */
/* A nested tree struct of list of zmaps with each zmap containing a list of zmapviews
 * and each zmapview containing a list of zmapwindows. */
typedef struct ZMapViewWindowTreeStructName
{
  void *parent ;
  GList *children ;
} ZMapViewWindowTreeStruct, *ZMapViewWindowTree ;





/* All sub-systems that accept or generate xremote requests/replies must use the
 * functions defined here to make and reply to requests. The code that
 * services this interface is in zmapApp/zmapAppRemoteCommand.c which
 * provides the interface between the zmap sub-systems and ZMapRemoteControl.
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
typedef void (*ZMapRemoteAppReturnReplyFunc)(char *command,
					     RemoteCommandRCType command_rc,
					     char *reason,
					     ZMapXMLUtilsEventStack reply,
					     gpointer app_reply_data) ;

/* All sub-systems must provide a function with this prototype to respond to requests.
 * When they have processed the request their reply _MUST_ be returned by calling
 * the app_reply_func and returning the command_rc, reason or reply and app_reply_data
 * as per the ZMapRemoteAppReturnReplyFunc prototype. */
typedef void (*ZMapRemoteAppProcessRequestFunc)(gpointer local_data,
						char *command_name,
						ZMapAppRemoteViewID view_id,
						char *command,
						ZMapRemoteAppReturnReplyFunc app_reply_func,
						gpointer app_reply_data) ;



/* 
 *  Functions that sub-systems must call/provide to _MAKE_ requests to a peer.
 */

/* All sub-systems must provide a function with this prototype to process replies to
 * their requests. */
typedef void (*ZMapRemoteAppProcessReplyFunc)(char *command,
					      RemoteCommandRCType command_rc,
					      char *reason,
					      char *reply,
					      gpointer reply_handler_func_data) ;


/* Function that sub-system must call when it has a request it wants sent to the peer.
 * When a reply is received from the peer the sub-systems reply_func will be called back
 * with the command_rc, reason or reply and the reply_func_data as per the
 * ZMapRemoteAppProcessReplyFunc prototype. */
typedef void (*ZMapRemoteAppMakeRequestFunc)(char *command, ZMapXMLUtilsEventStack request_body,
					     gpointer app_request_data,
					     ZMapRemoteAppProcessReplyFunc reply_handler_func,
					     gpointer reply_handler_func_data) ;





/* Defines IDs that represent a "view" of the data, there is one of these for every
 * single window that displays data. */
gboolean zMapAppRemoteViewCreateIDStr(ZMapAppRemoteViewID remote_view_id, char **view_id_str_out) ;
gboolean zMapAppRemoteViewCreateID(ZMapAppRemoteViewID remote_view_id, GQuark *view_id_out) ;
gboolean zMapAppRemoteViewParseIDStr(char *view_id_str, ZMapAppRemoteViewID *remote_view_id_inout) ;
gboolean zMapAppRemoteViewParseID(GQuark view_id, ZMapAppRemoteViewID *remote_view_id_inout) ;
gboolean zMapAppRemoteViewIsValidID(ZMapAppRemoteViewID remote_view_id) ;
gboolean zMapAppRemoteViewResetID(ZMapAppRemoteViewID remote_view_id) ;

#endif /* ZMAP_APP_REMOTE_H */
