/*  File: zmapRemoteControl_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010-2015: Genome Research Ltd.
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

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapRemoteControl.h>


/* Wild card address, used so the bind will "choose" a port for us. */
#define TCP_WILD_CARD_ADDRESS "tcp://127.0.0.1:*"

/* Must be long enough to hold a zmq style address (as in TCP_WILD_CARD_ADDRESS). */
#define ZMQ_ADDR_LEN 256


/* Timeouts are now a list of values. Change as needed for debugging/production etc. */
enum {TIMEOUT_LIST_INIT_SIZE = 20} ;                        /* Intial size of timeout array. */

#define TIMEOUT_DEBUG_LIST "1000000"
#define TIMEOUT_PRODUCTION_LIST "333,1000,3000,9000"

#define TIMEOUT_DEFAULT_LIST TIMEOUT_PRODUCTION_LIST


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


/* Remote Control state, most states area obvious but here are some notes:
 * 
 * REMOTE_STATE_INVALID - the object should never be in this state.
 * 
 * REMOTE_STATE_INACTIVE - neither the receive or send interfaces are initialised
 *                         so messages cannot be handled. This state implies
 *                         that initialisation has not taken place yet or that
 *                         the remote control object has been reset by the app.
 * 
 * REMOTE_STATE_FAILED - Implies there has been some drastic error, the object
 *                       can no longer be used and the app should call the destroy
 *                       method.
 * 
 * REMOTE_STATE_DYING - Because of the asynchronous nature of the comms it's possible
 * 
 *  note that REMOTE_STATE_FAILED means that
 *  */
#define REMOTE_STATE_LIST(_)						\
  _(REMOTE_STATE_INVALID,            ,					\
    "invalid", "Invalid state !"                             , "")      \
    _(REMOTE_STATE_INACTIVE,               ,				\
      "inactive", "Receive/Send interfaces not intialised."     , "")   \
    _(REMOTE_STATE_IDLE,               ,				\
      "idle", "No requests active."     , "")                           \
    _(REMOTE_STATE_OUTGOING_REQUEST_TO_BE_SENT,     ,                   \
      "outgoing-request-to-be-sent"    , "Have request to send to our peer."     , "") \
    _(REMOTE_STATE_OUTGOING_REQUEST_WAITING_FOR_THEIR_REPLY,     ,      \
      "outgoing-request-waiting-for-reply"    , "Sent request to peer, waiting for their reply."     , "") \
    _(REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED,     ,               \
      "incoming-request-to-be-received"    , "Have received request from peer."     , "") \
    _(REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY,     ,        \
      "incoming-request-waiting-for-reply"    , "Received request from peer, waiting for our reply."     , "") \
    _(REMOTE_STATE_FAILED,              ,                               \
      "failed", "Communication with peer has failed and cannot be recovered.", "") \
    _(REMOTE_STATE_DYING,              ,                                \
      "dying", "Dying, no requests can be accepted.", "")

ZMAP_DEFINE_ENUM(RemoteControlState, REMOTE_STATE_LIST) ;


/* Result of trying to get data from a zmq socket. */
#define SOCKET_FETCH_RC_LIST(_)                                            \
_(SOCKET_FETCH_OK,      , "ok",      "Socket fetch succeeded",     "") \
_(SOCKET_FETCH_NOTHING, , "nothing", "Nothing to be fetched", "") \
_(SOCKET_FETCH_FAILED,  , "failed",  "Serious error on socket",      "")

ZMAP_DEFINE_ENUM(SocketFetchRC, SOCKET_FETCH_RC_LIST) ;


/* Type for a request, must be either incoming or outgoing. */
#define REQUEST_TYPE_LIST(_)						\
_(REQUEST_TYPE_INVALID,            ,					\
  "invalid", "Invalid type !"                             , "") \
_(REQUEST_TYPE_INCOMING,               ,				\
  "incoming", "Incoming request from peer."     , "") \
_(REQUEST_TYPE_OUTGOING,              ,\
  "outgoing", "Outgoing request to peer.."                        , "")

ZMAP_DEFINE_ENUM(RemoteControlRequestType, REQUEST_TYPE_LIST) ;



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


/* Data required for each request/reply in a queue. Used to represent a request/reply
 * that has currently been dequeued and is being processed prior to requeing or completion. */
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

} ReqReplyStruct, *ReqReply ;




/* 
 *                    Structs for Send/Receive interfaces
 */

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




/* 
 *     The main remote control struct which contains the send and receive interfaces.
 */

typedef struct ZMapRemoteControlStructType
{
  /* We keep a magic pointer because these structs are handed to us by external callers
   * so we need to be able to check if they are valid. */
  ZMapMagic magic ;


  /* Our current state/interface. */

  RemoteControlState state ;

  RemoteReceive receive ;				    /* Active when acting as server. */
  RemoteSend send ;					    /* Active when acting as client. */

  void *zmq_context ;                                       /* zeroMQ context. */


  /* Used in constructing/validating etc. of requests/replies */
  GQuark app_id ;					    /* Applications "name", good for debug messages. */

  GQuark version ;					    /* Current protocol version. */

  /* Used to provide a unique id for each request, N.B. kept here so we can construct requests
   * even if "send" has not been initialised. The id is always > 0 */
  int request_id ;



  /* Queues of incoming requests/outgoing replies and outgoing requests/incoming replies,
   * these are queues of RemoteZeroMQMessage's, i.e. the queues hold messages to be received
   * or sent. */
  GQueue *incoming_requests ;
  GQueue *outgoing_replies ;

  GQueue *outgoing_requests ;
  GQueue *incoming_replies ;

  guint queue_monitor_cb_id ;                               /* queue monitor glib callback id. */



  /* The current Request/reply taken from the above queues. */
  ReqReply curr_req ;                                       /* Request/Reply being processed. */

  ReqReply prev_incoming_req ;                              /* Hold on to this in case our reply
                                                               was too slow and peer timed us out. */

  ReqReply stalled_req ;                                    /* Request/Reply stalled as a result of a collision. */


  /* Timeouts: timeout list and timer. */
  GArray *timeout_list ;
  int timeout_list_pos ;
  GTimer *timeout_timer ;


  /* where to send error messages, can be overridden by app, err_report_data enables app to
   * supply any necessary data for message output. */
  ZMapRemoteControlErrorReportFunc err_report_func ;
  gpointer err_report_data ;


  /* App callback function to be called if there is an error, timeout etc. */
  ZMapRemoteControlErrorHandlerFunc app_error_func ;
  gpointer app_error_func_data ;


} ZMapRemoteControlStruct ;



/* Function decs generated by macros, see zmapEnum.h for func. prototypes. */
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteType2ExactStr, RemoteType) ;
ZMAP_ENUM_AS_EXACT_STRING_DEC(remoteState2ExactStr, RemoteControlState) ;

unsigned int zmapRemoteControlGetNewReqID(void) ;
char *zmapRemoteControlGetCurrCommand(ZMapRemoteControl remote_control) ;


#endif /* ZMAP_REMOTE_CONTROL__P_H */
