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
 * Last edited: Dec 16 10:13 2011 (edgrif)
 * Created: Fri Sep 24 14:44:59 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL__P_H
#define ZMAP_REMOTE_CONTROL__P_H

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapRemoteControl.h>



/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */

/* Is the remote object idle, a client or a server ? */
#define REMOTE_TYPE_LIST(_)                                                  \
_(REMOTE_TYPE_INVALID,  , "invalid"  , "Remote is invalid !"           , "") \
_(REMOTE_TYPE_IDLE,     , "idle"     , "Remote is idle."               , "") \
_(REMOTE_TYPE_CLIENT,   , "client"   , "Remote is acting as client."   , "") \
_(REMOTE_TYPE_SERVER,   , "server"   , "Remote is acting as server."   , "")

ZMAP_DEFINE_ENUM(RemoteType, REMOTE_TYPE_LIST) ;


/* Remote Control state. */
#define REMOTE_STATE_LIST(_)						\
_(REMOTE_STATE_INVALID,            ,					\
  "invalid", "Invalid state !"                             , "") \
_(REMOTE_STATE_IDLE,               ,				\
  "idle", "Peer - not in client or server state."     , "") \
_(REMOTE_STATE_DYING,              ,\
  "dying"             , "Peer - Dying."                        , "") \
_(REMOTE_STATE_CLIENT_WAIT_GET,     ,\
  "client-waiting-for-req-get"    , "Client - signalled server we have a request, waiting for server to ask for it."     , "") \
_(REMOTE_STATE_CLIENT_WAIT_REQ_ACK,	   ,\
  "client-waiting-for-req-ack"    , "Client - sent request to server, waiting for server to signal they have it.", "") \
_(REMOTE_STATE_CLIENT_WAIT_REPLY,  ,\
  "client-waiting-for-reply" , "Client - Server signalled they have request, waiting for server to signal it has a reply to send." , "") \
_(REMOTE_STATE_CLIENT_WAIT_SEND,  ,\
  "client-waiting-for-reply-send" , "Client - Server signalled they have a reply, signalled server to send it." , "") \
_(REMOTE_STATE_CLIENT_WAIT_REPLY_ACK,  ,\
  "client-waiting-for-reply-ack" , "Client - told server we have reply, waiting for server to acknowledge." , "") \
_(REMOTE_STATE_SERVER_WAIT_NEW_REQ,  ,\
  "server-waiting" , "Server - waiting for client to signal they have a new request."  , "") \
_(REMOTE_STATE_SERVER_WAIT_REQ_SEND,  ,\
  "server-waiting-for-request-send" , "Server - client has signalled they have a request, waiting for client to send it."  , "") \
_(REMOTE_STATE_SERVER_WAIT_REQ_ACK,  ,\
  "server-waiting-for-req-ack" , "Server - told client we have request, waiting for client to acknowledge."  , "") \
_(REMOTE_STATE_SERVER_PROCESS_REQ,     ,\
  "server-processing"    , "Server - client acknowledged we have received request, now processing it."    , "") \
_(REMOTE_STATE_SERVER_WAIT_GET,     ,\
  "server-waiting-for-reply-get"    , "Server - signalled client we have reply, waiting for client to ask for it."    , "") \
_(REMOTE_STATE_SERVER_WAIT_REPLY_ACK,     ,\
  "server-waiting-for-reply-ack"    , "Server - sent reply, waiting for client to acknowledge they have it."    , "") \

ZMAP_DEFINE_ENUM(RemoteControlState, REMOTE_STATE_LIST) ;




/* Usual common struct, all the below structs must have the same members as these first. */
typedef struct RemoteAnyStructName
{
  RemoteType remote_type ;

  GQuark any_app_name ;

  GQuark any_unique_str ;
  GdkAtom any_atom ;
  char *any_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *any_clipboard ;

  /* Our record of the two way traffic. */
  char *any_request ;
  char *any_reply ;

} RemoteAnyStruct, *RemoteAny ;


/* Remote is idle, i.e. not acting as a server or a client. */
typedef struct RemoteIdleStructName
{
  RemoteType remote_type ;

  GQuark any_app_name ;

  /* Their 'request' atom for receiving our requests and giving us responses, stays the same for
   * the life of this control struct. */
  GQuark any_unique_str ;
  GdkAtom any_atom ;
  char *any_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *any_clipboard ;

  /* Our record of the two way traffic. */
  char *any_request ;
  char *any_reply ;

} RemoteIdleStruct, *RemoteIdle ;



/* Control block for when acting as a server, i.e. we have received a request from the peer. */
typedef struct RemoteReceiveStructName
{
  RemoteType remote_type ;

  GQuark our_app_name ;

  /* Our atom for receiving peer requests and returning a response, stays the same for
   * the life of this control struct. */
  GQuark our_unique_str ;
  GdkAtom our_atom ;
  char *our_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *our_clipboard ;

  /* Our record of the two way traffic. */
  char *their_request ;
  char *our_reply ;


  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlRequestHandlerFunc process_request_func ;
  gpointer process_request_func_data ;

  /* Call this app callback to tell app that the peer has received its reply. */
  ZMapRemoteControlReplySentFunc reply_sent_func ;
  gpointer reply_sent_func_data ;

} RemoteReceiveStruct, *RemoteReceive ;



/* Control block for when acting as a client, i.e. we are making a request. */
typedef struct RemoteSendStructName
{
  RemoteType remote_type ;

  GQuark their_app_name ;

  /* Their 'request' atom for receiving our requests and giving us responses, stays the same for
   * the life of this control struct. */
  GQuark their_unique_str ;
  GdkAtom their_atom ;
  char *their_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *their_clipboard ;

  /* Our record of the two way traffic. */
  char *our_request ;
  char *their_reply ;

  /* Call this app function to tell app that peer has received its request. */
  ZMapRemoteControlRequestSentFunc req_sent_func ;
  gpointer req_sent_func_data ;

  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlReplyHandlerFunc process_reply_func ;
  gpointer process_reply_func_data ;

} RemoteSendStruct, *RemoteSend ;




/* The main remote control struct which contains the self and peer interfaces. */
typedef struct ZMapRemoteControlStructName
{
  /* We keep a magic pointer because these structs are handed to us by external callers
   * so we need to be able to check if they are valid. */
  ZMapMagic magic ;

  GQuark app_id ;					    /* Applications "name", good for debug messages. */

  GQuark version ;					    /* Current protocol version. */

  GdkAtom target_atom ;					    /* atom representation of target type for data. */

  /* Our current state/interface. */
  RemoteControlState state ;

  RemoteAny curr_remote ;				    /* Points to idle, self or peer. */
  RemoteIdle idle ;					    /* When not doing anything. */
  RemoteReceive receive ;				    /* Active when acting as server. */
  RemoteSend send ;					    /* Active when acting as client. */

  /* Used to provide a unique id for each request, N.B. kept here so we can construct requests
   * even if "send" has not been intialised. */
  int request_id_num ;
  GString *request_id ;


  /* Timeouts, timer_source_id is cached so we can cancel the timeouts. */
  guint timer_source_id ;
  guint32 timeout_ms ;					    /* timeout in milliseconds. */


  /* App function to call when there is an error, e.g. timeout. */
  ZMapRemoteControlErrorHandlerFunc app_error_func ;
  gpointer app_error_func_data ;

  /* where to send error messages, can be overridden by app. */
  ZMapRemoteControlErrorReportFunc app_err_report_func ;
  gpointer app_err_report_data ;


} ZMapRemoteControlStruct ;



/* Function decs generated by macros, see zmapEnum.h for func. prototypes. */
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteType2ExactStr, RemoteType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteState2ExactStr, RemoteControlState) ;

char *zmapRemoteControlMakeReqID(ZMapRemoteControl remote_control) ;
char *zmapRemoteControlGetCurrCommand(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL__P_H */
