/*  File: zmapRemoteControl_.h
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Private header for remote control package.
 *
 * HISTORY:
 * Last edited: Jun 28 13:03 2011 (edgrif)
 * Created: Fri Sep 24 14:44:59 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL__P_H
#define ZMAP_REMOTE_CONTROL__P_H

#include <ZMap/zmapEnum.h>
#include <ZMap/zmapRemoteControl.h>



#define DATA_RECEIVING_ATOM "REMOTECONTROL_DATA_ATOM"



/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */

/* Is the request functioning as a client or server ? */
#define REMOTE_TYPE_LIST(_)                                                                                 \
_(REMOTE_TYPE_INVALID,            , "invalid"           , "invalid type."                             , "") \
_(REMOTE_TYPE_CLIENT,             , "client"            , "Request client."                           , "") \
_(REMOTE_TYPE_SERVER,             , "server"            , "Request server."                           , "")

ZMAP_DEFINE_ENUM(RemoteType, REMOTE_TYPE_LIST) ;


/* Using this macro to declare the enum means we get lots of enum functions for free (see zmapEnum.h). */
#define CLIENT_STATE_LIST(_)                                                                                 \
_(CLIENT_STATE_INVALID,            , "invalid"           , "invalid mode "                             , "") \
_(CLIENT_STATE_INACTIVE,           , "inactive"          , "Waiting to make or receive a request."     , "") \
_(CLIENT_STATE_SENDING,            , "client-sending"    , "Peer is client and sending a request."     , "") \
_(CLIENT_STATE_WAITING,	           , "client-waiting"    , "Peer is client and waiting for a response.", "") \
_(CLIENT_STATE_RETRIEVING,         , "client-retrieving" , "Peer is client and retrieving a response." , "")

ZMAP_DEFINE_ENUM(ClientControlState, CLIENT_STATE_LIST) ;


/* Using this macro to declare the enum means we get lots of enum functions for free (see zmapEnum.h). */
#define SERVER_STATE_LIST(_)                                                                                 \
_(SERVER_STATE_INVALID,            , "invalid"           , "invalid mode "                             , "") \
_(SERVER_STATE_INACTIVE,           , "inactive"          , "Waiting to make or receive a request."     , "") \
_(SERVER_STATE_RETRIEVING,         , "server-retrieving" , "Peer is server and retrieving a request."  , "") \
_(SERVER_STATE_PROCESSING,         , "server-processing" , "Peer is server and processing a request."  , "") \
_(SERVER_STATE_SENDING,	           , "server-sending"    , "Peer is server and sending a response."    , "")

ZMAP_DEFINE_ENUM(ServerControlState, SERVER_STATE_LIST) ;


/* Describes either client or server requests. */
typedef struct AnyRequestStructName
{
  RemoteType request_type ;

  ZMapRemoteControl remote_control ;			    /* Parent remote control object. */

  /* The _current_ peers atom for receiving requests and responses, will be rewritten for
   * each new request/response, may change or not depending if it's a new client. */
  GdkAtom peer_atom ;

  GdkNativeWindow peer_window ;

  gulong curr_req_time ;				    /* Time curr request initiated. */

  gboolean has_timed_out ;				    /* Have we timed out ? */

  char *request ;

} AnyRequestStruct, *AnyRequest ;


/* Data for client requests. */
typedef struct ClientRequestStructName
{
  /* Section common to all request structs (see AnyRequest). */

  RemoteType request_type ;

  ZMapRemoteControl remote_control ;			    /* Parent remote control object. */

  /* The current peers atom for receiving requests and responses, will be rewritten for
   * each new request/response, may change or not depending if it's a new client. */
  GdkAtom peer_atom ;

  GdkNativeWindow peer_window ;

  gulong curr_req_time ;				    /* Time curr request initiated. */

  gboolean has_timed_out ;				    /* Have we timed out ? */

  char *request ;


  /* Client specific section. */

  ClientControlState state ;				    /* Current state of this peer. */

} ClientRequestStruct, *ClientRequest ;


/* Data for individual requests. */
typedef struct ServerRequestStructName
{
  /* Section common to all request structs (see AnyRequest). */

  RemoteType request_type ;				    /* Must be first to match AnyRequest. */

  ZMapRemoteControl remote_control ;			    /* Parent remote control object. */

  /* The _current_ peers atom for receiving requests and responses, will be rewritten for
   * each new request/response, may change or not depending if it's a new client. */
  GdkAtom peer_atom ;

  GdkNativeWindow peer_window ;

  gulong curr_req_time ;				    /* Time curr request initiated. */

  gboolean has_timed_out ;				    /* Have we timed out ? */

  char *request ;


  /* Server specific section. */

  ServerControlState state ;				    /* Current state of this peer. */

} ServerRequestStruct, *ServerRequest ;





/* The main remote control struct. */
typedef struct ZMapRemoteControlStructName
{
  /* We keep a magic pointer because these structs are handed to us by external callers
   * so we need to be able to check if they are valid. */
  ZMapMagic magic ;

  /* Our callers "name", good for debug messages. */
  char *app_id ;

  /* This is the widget which we attach all properties to. */
  GtkWidget *our_widget ;
  GdkNativeWindow our_window ;				    /* Cache this as it's a pain to derive
							       from widget. */


  gboolean show_all_events ;				    /* If TRUE then print to stderr all
							       events for our main window. */

  /* Atoms for communication. */

  /* Our atom for receiving requests and responses from other peers, stays the same for
   * the life of this control struct. */
  GdkAtom our_atom ;
  char *our_atom_string ;				    /* Cached because it's a pain to get the string. */

  GdkAtom data_format_atom ;				    /* Still needed ???? */
  gint data_format_bits ;

  GtkClipboard *our_clipboard ;



  /* The current request, NULL when there isn't one. Can be client or server. */
  AnyRequest curr_request ;


  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlRequestHandlerFunc request_func ;
  gpointer request_data ;

  ZMapRemoteControlReplyHandlerFunc reply_func ;
  gpointer reply_data ;

  guint32 timeout_ms ;					    /* timeout in milliseconds. */
  ZMapRemoteControlTimeoutHandlerFunc timeout_func ;
  gpointer timeout_data ;

  ZMapRemoteControlErrorFunc err_func ;
  gpointer err_data ;


  /* internal callback function instance ids, ids are used to remove callbacks. */

  gulong test_notify_id ;


  /* general callbacks */
  gulong all_events_id ;
  guint timer_source_id ;


  /* Server callbacks */
  gulong select_clear_id ;
  gulong select_notify_id ;
  gulong server_message_id ;


  /* client callbacks */
  gulong select_request_id ;
  gulong client_message_id ;
  gulong client_select_notify_id ;


} ZMapRemoteControlStruct ;



ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteType2ExactStr, RemoteType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(clientState2ExactStr, ClientControlState) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(serverState2ExactStr, ServerControlState) ;









#endif /* ZMAP_REMOTE_CONTROL__P_H */
