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
 * Last edited: Dec  1 15:58 2011 (edgrif)
 * Created: Fri Sep 24 14:44:59 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL__P_H
#define ZMAP_REMOTE_CONTROL__P_H

#include <ZMap/zmapEnum.h>
#include <ZMap/zmapRemoteControl.h>



/* Using the XXX_LIST macros to declare enums means we get lots of enum functions for free (see zmapEnum.h). */


/* Is the remote object idle, a client or a server ? */
#define REMOTE_TYPE_LIST(_)                                                                                 \
_(REMOTE_TYPE_INVALID,            , "invalid"           , "Remote is invalid !"                             , "") \
_(REMOTE_TYPE_IDLE,               , "idle"              , "Remote is idle."                             , "") \
_(REMOTE_TYPE_CLIENT,               , "client"              , "Remote is acting as client."    , "") \
_(REMOTE_TYPE_SERVER,               , "server"              , "Remote is acting as server."         , "")

ZMAP_DEFINE_ENUM(RemoteType, REMOTE_TYPE_LIST) ;


/* Remote Control state. */
#define REMOTE_STATE_LIST(_)						\
_(REMOTE_STATE_INVALID,            ,					\
  "invalid"           , "Invalid state !"                             , "") \
_(REMOTE_STATE_IDLE,               ,				\
  "idle"              , "Peer - not waiting for a request/not making a request."     , "") \
_(REMOTE_STATE_CLIENT_WAIT_GET,     ,\
  "client-waiting-for-get"    , "Peer/client - contacted server, waiting for server ask for request."     , "") \
_(REMOTE_STATE_CLIENT_WAIT_REQ_ACK,	   ,\
  "client-waiting-for-req-ack"    , "Peer/client - sent request waiting for server to acknowledge.", "") \
_(REMOTE_STATE_CLIENT_WAIT_REPLY,  ,\
  "client-waiting-for-reply" , "Peer/client - waiting for server to signal it has a reply to send." , "") \
_(REMOTE_STATE_CLIENT_WAIT_SEND,  ,\
  "client-waiting-for-send" , "Peer/client - asked for reply, waiting for server to send it." , "") \
_(REMOTE_STATE_CLIENT_WAIT_REPLY_ACK,  ,\
  "client-waiting-for-reply-ack" , "Peer/client - waiting for server to acknowledge end of transaction." , "") \
_(REMOTE_STATE_SERVER_WAIT_SIGNAL_NEW,  ,\
  "server-waiting" , "Peer/server - waiting for client to signal they have a new request."  , "") \
_(REMOTE_STATE_SERVER_WAIT_REQ,  ,\
  "server-waiting-for-request" , "Peer/server - waiting for client to send request."  , "") \
_(REMOTE_STATE_SERVER_WAIT_REQ_ACK,  ,\
  "server-waiting-for-req-ack" , "Peer/server - told client we have request, waiting for acknowledgement."  , "") \
_(REMOTE_STATE_SERVER_REQ_PROCESS,     ,\
  "server-processing"    , "Peer/server - Contacted client to say we have reply, now processing reply."    , "") \
_(REMOTE_STATE_SERVER_WAIT_GET,     ,\
  "server-waiting-for-get"    , "Peer/server - Sent reply, waiting for client to acknowledge they have it."    , "") \
_(REMOTE_STATE_SERVER_WAIT_REPLY_ACK,     ,\
  "server-waiting-for-reply-ack"    , "Peer/server - Sent reply, waiting for client to acknowledge they have it."    , "") \
_(REMOTE_STATE_RESETTING,          ,\
  "resetting"         , "Peer - Resetting to idle state after error."      , "") \
_(REMOTE_STATE_DYING,              ,\
  "dying"             , "Peer - Dying after error."                        , "")


ZMAP_DEFINE_ENUM(RemoteControlState, REMOTE_STATE_LIST) ;




/* Usual common struct, all the below structs must have the same members as these first. */
typedef struct RemoteAnyStructName
{
  RemoteType remote_type ;
} RemoteAnyStruct, *RemoteAny ;


/* Remote is idle, i.e. not acting as a server or a client. */
typedef struct RemoteIdleStructName
{
  RemoteType remote_type ;
} RemoteIdleStruct, *RemoteIdle ;



/* Control block for when acting as a server, i.e. we have received a request from the peer. */
typedef struct RemoteSelfStructName
{
  RemoteType remote_type ;

  /* Our atom for receiving peer requests and returning a response, stays the same for
   * the life of this control struct. */
  GdkAtom our_atom ;
  char *our_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *our_clipboard ;

  guint32 timeout_ms ;					    /* timeout in milliseconds. */

  /* Our record of the two way traffic. */
  char *their_request ;
  char *our_reply ;


  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlRequestHandlerFunc request_func ;
  gpointer request_func_data ;

  ZMapRemoteControlTimeoutHandlerFunc timeout_func ;
  gpointer timeout_func_data ;

} RemoteSelfStruct, *RemoteSelf ;



/* Control block for when acting as a client, i.e. we are making a request. */
typedef struct RemotePeerStructName
{
  RemoteType remote_type ;

  /* Their 'request' atom for receiving our requests and giving us responses, stays the same for
   * the life of this control struct. */
  GdkAtom their_atom ;
  char *their_atom_string ;				    /* Cached because it's a pain to get the string. */
  GtkClipboard *their_clipboard ;

  guint32 timeout_ms ;					    /* timeout in milliseconds. */

  /* Our record of the two way traffic. */
  char *our_request ;
  char *their_reply ;

  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlReplyHandlerFunc reply_func ;
  gpointer reply_func_data ;

  ZMapRemoteControlTimeoutHandlerFunc timeout_func ;
  gpointer timeout_func_data ;

} RemotePeerStruct, *RemotePeer ;




/* The main remote control struct which contains the self and peer interfaces. */
typedef struct ZMapRemoteControlStructName
{
  /* We keep a magic pointer because these structs are handed to us by external callers
   * so we need to be able to check if they are valid. */
  ZMapMagic magic ;

  /* Our current state. */
  RemoteControlState state ;
  RemoteAny curr_remote ;				    /* points to idle, self or peer. */

  /* The idle, self (== when are acting as server) and peer (== when we are acting as client) interfaces. */
  RemoteIdle idle ;
  RemoteSelf self ;
  RemotePeer peer ;


  /* Applications "name", good for debug messages. */
  char *app_id ;

  /* atom representation of target type for data. */
  GdkAtom target_atom ;



  /* where to send error messages, can be overridden by user. */
  ZMapRemoteControlErrorReportFunc err_func ;
  gpointer err_data ;

  guint timer_source_id ;


} ZMapRemoteControlStruct ;



/* Function decs generated by macros, see zmapEnum.h for func. prototypes. */
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteType2ExactStr, RemoteType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteState2ExactStr, RemoteControlState) ;



#endif /* ZMAP_REMOTE_CONTROL__P_H */
