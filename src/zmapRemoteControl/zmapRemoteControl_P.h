/*  File: zmapRemoteControl_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010-2014: Genome Research Ltd.
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
 * Last edited: Jun  3 10:38 2013 (edgrif)
 * Created: Fri Sep 24 14:44:59 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_CONTROL__P_H
#define ZMAP_REMOTE_CONTROL__P_H


#include <zmq.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>                                  /* for XXX_LIST macros. */
#include <ZMap/zmapRemoteControl.h>


/* Timeouts are now a list of values. Change as needed for debugging/production etc. */
enum {TIMEOUT_LIST_INIT_SIZE = 20} ;                        /* Intial size of timeout array. */
#define TIMEOUT_DEBUG_LIST "1000000"
#define TIMEOUT_PRODUCTION_LIST "100,400,800,1600"
#define TIMEOUT_DEFAULT_LIST TIMEOUT_DEBUG_LIST


#define TIMEOUT_TYPE_LIST(_)                                            \
_(TIMEOUT_NONE,      , "none",      "Have not timed out."         , "") \
_(TIMEOUT_NOT_FINAL, , "not-final", "Not the last/final timeout." , "") \
_(TIMEOUT_FINAL,     , "final",     "Final timeout reached."      , "")

ZMAP_DEFINE_ENUM(TimeoutType, TIMEOUT_TYPE_LIST) ;




/* Is the remote object idle, a client or a server ? */
#define REMOTE_TYPE_LIST(_)                                                  \
_(REMOTE_TYPE_INVALID,  , "invalid"  , "Remote is invalid !"           , "") \
_(REMOTE_TYPE_IDLE,     , "idle"     , "Remote is idle."               , "") \
_(REMOTE_TYPE_CLIENT,   , "client"   , "Remote is acting as client."   , "") \
_(REMOTE_TYPE_SERVER,   , "server"   , "Remote is acting as server."   , "")

ZMAP_DEFINE_ENUM(RemoteType, REMOTE_TYPE_LIST) ;


/* unused stuff....

_(REMOTE_STATE_SERVER_WAIT_REQ_SEND,  ,\
  "server-waiting-for-request-send" , "Server - client has signalled they have a request, waiting for client to send it."  , "") \
_(REMOTE_STATE_SERVER_WAIT_REQ_ACK,  ,\
  "server-waiting-for-req-ack" , "Server - told client we have request, waiting for client to acknowledge."  , "") \
_(REMOTE_STATE_SERVER_WAIT_GET,     ,\
  "server-waiting-for-reply-get"    , "Server - signalled client we have reply, waiting for client to ask for it."    , "") \
_(REMOTE_STATE_SERVER_WAIT_REPLY_ACK,     ,\
  "server-waiting-for-reply-ack"    , "Server - sent reply, waiting for client to acknowledge they have it."    , "") \

*/


/* Remote Control state. */

/* THIS IS THE OLD STATE...WE NEED NEW STUFF NOW....

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
  "server-waiting" , "Server - waiting for client to send a new request."  , "") \
_(REMOTE_STATE_SERVER_PROCESS_REQ,     ,\
  "server-processing"    , "Server - received request, forwarded it to app for processing."    , "")

  .....END OF OLD STUFF */

/* New stuff.... */
#define REMOTE_STATE_LIST(_)						\
_(REMOTE_STATE_INVALID,            ,					\
  "invalid", "Invalid state !"                             , "") \
_(REMOTE_STATE_IDLE,               ,				\
  "idle", "Idle, no requests active."     , "") \
_(REMOTE_STATE_DYING,              ,\
  "dying"             , "Dying, no requests can be accepted."                        , "") \
_(REMOTE_STATE_OUTGOING_REQUEST_TO_BE_SENT,     ,\
  "outgoing-request-to-be-sent"    , "Have request to send to our peer."     , "") \
_(REMOTE_STATE_OUTGOING_REQUEST_WAITING_FOR_THEIR_REPLY,     ,\
  "outgoing-request-waiting-for-reply"    , "Sent request to peer, waiting for their reply."     , "") \
_(REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED,     ,\
  "incoming-request-to-be-received"    , "Have received request from peer."     , "") \
_(REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY,     ,\
  "incoming-request-waiting-for-reply"    , "Received request from peer, waiting for our reply."     , "")

ZMAP_DEFINE_ENUM(RemoteControlState, REMOTE_STATE_LIST) ;




#define SOCKET_FETCH_RC_LIST(_)                                            \
_(SOCKET_FETCH_OK,      , "ok",      "Socket fetch succeeded",     "") \
_(SOCKET_FETCH_NOTHING, , "nothing", "Nothing to be fetched", "") \
_(SOCKET_FETCH_FAILED,  , "failed",  "Serious error on socket",      "")

ZMAP_DEFINE_ENUM(SocketFetchRC, SOCKET_FETCH_RC_LIST) ;





/* Wild card address, used so the bind will "choose" a port for us. */
#define TCP_WILD_CARD_ADDRESS "tcp://127.0.0.1:*"

/* Must be long enough to hold a zmq style address (as in TCP_WILD_CARD_ADDRESS). */
#define ZMQ_ADDR_LEN 256


/* Data needed by timer functions monitoring request/reply zeromq sockets. */
typedef struct TimerDataStructType
{
  ZMapRemoteControl remote_control ;                        /* Needed for debug messages etc. */

  GSourceFunc timer_func_cb ;                               /* timer callback func. */
  guint timer_id ;                                          /* timer callback id. */
  gboolean remove_timer ;                                   /* If true then timer callback removes itself. */

} TimerDataStruct, *TimerData ;



/* This is the message struct that gets queued and dequeued.... */
typedef struct RemoteZeroMQMessageStructType
{
  char *header ;
  char *body ;
} RemoteZeroMQMessageStruct, *RemoteZeroMQMessage ;





#define REQUEST_TYPE_LIST(_)						\
_(REQUEST_TYPE_INVALID,            ,					\
  "invalid", "Invalid type !"                             , "") \
_(REQUEST_TYPE_INCOMING,               ,				\
  "incoming", "Incoming request from peer."     , "") \
_(REQUEST_TYPE_OUTGOING,              ,\
  "outgoing", "Outgoing request to peer.."                        , "")


ZMAP_DEFINE_ENUM(RemoteControlRequestType, REQUEST_TYPE_LIST) ;



/* Data required for each request/reply in a queue. Used to represent request/replies
 * in the queues of requests/replies. */
typedef struct ReqReplyStructType
{
  RemoteControlRequestType request_type ;

  char *end_point ;

  RemoteZeroMQMessage request ;

  /* derived from "request" */
  char *request_id ;
  char *command ;

  int req_time_s ;
  int req_time_ms ;

  RemoteZeroMQMessage reply ;


  int num_retries;

} ReqReplyStruct, *ReqReply ;





/* Structs for Send/Receive interfaces */

/* Usual common struct, all the below structs must have the same members as these first. */
typedef struct RemoteAnyStructType
{
  RemoteType remote_type ;

  GQuark any_app_name_id ;

  /* zeromq handle for requests/replies */
  char *end_point ;

  /* Our record of the two way traffic. */
  char *any_request ;
  char *any_reply ;

} RemoteAnyStruct, *RemoteAny ;


/* Control block for when acting as a server, i.e. we have received a request from the peer. */
typedef struct RemoteReceiveStructName
{
  RemoteType remote_type ;

  GQuark our_app_name_id ;

  char *zmq_end_point ;                                     /* zeromq style address, e.g. "tcp://localhost:5555" */

  /* zeromq handle and associated stuff to receive requests. */
  void *zmq_socket ;                                        /* zeromq "socket". */

  /* We use timers to monitor for zeromq socket activity, here's the data needed to control this. */
  TimerData timer_data ;

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

  GQuark peer_app_name_id ;

  char *zmq_end_point ;                                     /* zeromq style address, e.g. "tcp://localhost:5555" */

  /* zeromq handle to send requests. */
  void *zmq_socket ;                                        /* zeromq "socket". */

  /* We use timers to monitor for zeromq socket activity, here's the data needed to control this. */
  TimerData timer_data ;

  /* Call this app function to tell app that peer has received its request. */
  ZMapRemoteControlRequestSentFunc req_sent_func ;
  gpointer req_sent_func_data ;

  /* Callback functions specified by caller to receive requests and subsequent replies/errors. */
  ZMapRemoteControlReplyHandlerFunc process_reply_func ;
  gpointer process_reply_func_data ;

} RemoteSendStruct, *RemoteSend ;




/* The main remote control struct which contains the self and peer interfaces. */
typedef struct ZMapRemoteControlStructType
{
  /* We keep a magic pointer because these structs are handed to us by external callers
   * so we need to be able to check if they are valid. */
  ZMapMagic magic ;

  GQuark app_id ;					    /* Applications "name", good for debug messages. */

  GQuark version ;					    /* Current protocol version. */

  /* Used to provide a unique id for each request, N.B. kept here so we can construct requests
   * even if "send" has not been initialised. The id is always > 0 */
  int request_id ;

  /* Request/replies we are currently dealing with or NULL. */
  RemoteZeroMQMessage curr_req_raw ;                        /* Just the header and xml request string. */

  ReqReply curr_req ;                                       /* Request/Reply being processed. */
  ReqReply stalled_req ;                                    /* Request/Reply stalled as a result of a collision. */
  ReqReply timedout_req ;                                   /* Last timedout request. */

  /* Our current state/interface. */
  RemoteControlState state ;

  RemoteReceive receive ;				    /* Active when acting as server. */
  RemoteSend send ;					    /* Active when acting as client. */


  /* zeroMQ context. */
  void *zmq_context ;

  /* I'M NOT COMPLETELY SURE OF THIS....MIGHT NOT BE THE BEST WAY TO ORGANISE THINGS...
   * NOT SURE.... */
  /* Queues of incoming requests/outgoing replies and outgoing requests/incoming replies. */
  GQueue *incoming_requests ;
  GQueue *outgoing_replies ;

  GQueue *outgoing_requests ;
  GQueue *incoming_replies ;

  guint queue_monitor_cb_id ;                                        /* queue monitor glib callback id. */
  gboolean stop_monitoring ;                                /* If TRUE monitor callback will remove itself. */


  /* Timeouts, timer_source_id is cached so we can cancel the timeouts. */
  GArray *timeout_list ;
  int timeout_list_pos ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  guint timer_source_id ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  GTimer *timeout_timer ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  guint32 timeout_ms ;					    /* timeout in milliseconds. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* App function to call when there is an error, e.g. peer not responding. */
  ZMapRemoteControlErrorHandlerFunc app_error_func ;
  gpointer app_error_func_data ;

  /* App function to call when there is a timeout. */
  ZMapRemoteControlTimeoutHandlerFunc app_timeout_func ;
  gpointer app_timeout_func_data ;

  /* where to send error messages, can be overridden by app. */
  ZMapRemoteControlErrorReportFunc app_err_report_func ;
  gpointer app_err_report_data ;


} ZMapRemoteControlStruct ;



/* Function decs generated by macros, see zmapEnum.h for func. prototypes. */
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteType2ExactStr, RemoteType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteState2ExactStr, RemoteControlState) ;

unsigned int zmapRemoteControlGetNewReqID(ZMapRemoteControl remote_control) ;
char *zmapRemoteControlGetCurrCommand(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL__P_H */
