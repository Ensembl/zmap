/*  File: zmapRemoteControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the "Session" layer (OSI 7 layer model)
 *              of the Remote Control interface: the code sends
 *              a request and then waits for the reply. The code
 *              knows nothing about the content of the messages,
 *              it simply ensures that each request receives a
 *              reply or an error is reported (e.g. timeout).
 *
 * Exported functions: See ZMap/zmapRemoteControl.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>                                         /* Needed for event status type for zeromq. */
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gprintf.h>
#include <zmq.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapCmdLineArgs.hpp>
#include <ZMap/zmapRemoteCommand.hpp>
#include <zmapRemoteControl_P.hpp>



/* Text for debug/log messages.... */
#define PEER_PREFIX   "PEER"
#define CLIENT_PREFIX "CLIENT"
#define SERVER_PREFIX "SERVER"

#define ENTER_TXT "Enter >>>>"
#define EXIT_TXT  "Exit  <<<<"



/* Have no idea what's sensible but a guess is to think that we need socket checking to be faster
 * than queue checking since the queue checker can't have anything to check unless there is something
 * from the socket. */
enum
  {
    SOCKET_WATCH_INTERVAL = 10,                             /* Interval in ms between checking zmq socket. */

    QUEUE_WATCH_INTERVAL = 20                               /* Interval in ms between checking request/repy queues. */
  } ;



/*
 * Use these macros for _ALL_ logging calls/error handling in this code
 * to ensure that messages go to the right place.
 */
#define REMOTELOGMSG(REMOTE_CONTROL, FORMAT_STR, ...)   \
  do                                                    \
    {                                                   \
      errorMsg((REMOTE_CONTROL),                        \
	       __FILE__,                                \
	       __PRETTY_FUNCTION__,                     \
	       (FORMAT_STR), __VA_ARGS__) ;             \
    } while (0)


#define REPORTMSG(REMOTE_CONTROL, MSG, PREFIX)                          \
  do                                                                    \
    {                                                                   \
      REMOTELOGMSG((REMOTE_CONTROL), "%s:\n%s\n%s", (PREFIX), (MSG)->header, (MSG)->body) ; \
    } while (0)


/* Switchable version of remotelogmsg for debugging sessions. */
#define DEBUGLOGMSG(REMOTE_CONTROL, DEBUG_LEVEL, FORMAT_STR, ...)       \
  do									\
    {									\
      if ((DEBUG_LEVEL) <= remote_debug_G)                              \
        {								\
	  REMOTELOGMSG((REMOTE_CONTROL),                                \
		       (FORMAT_STR), __VA_ARGS__) ;			\
	}								\
    } while (0)


/* Call the error handler with standard params. */
#define CALL_ERR_HANDLER(REMOTE_CONTROL, ERROR_RC, FORMAT_STR, ...)	\
  do									\
    {									\
      errorHandler((REMOTE_CONTROL),                                    \
		   __FILE__,						\
		   __PRETTY_FUNCTION__,					\
		   (ERROR_RC),						\
		   (FORMAT_STR), __VA_ARGS__) ;				\
    } while (0)

/* Used every time there is a serious errror, must call app err handler. */
#define LOG_AND_CALL_ERR_HANDLER(REMOTE_CONTROL, ERROR_RC, FORMAT_STR, ...) \
  do									\
    {									\
      REMOTELOGMSG((REMOTE_CONTROL),                                    \
                   (FORMAT_STR), __VA_ARGS__) ;                         \
      CALL_ERR_HANDLER((REMOTE_CONTROL), (ERROR_RC),                    \
                       (FORMAT_STR), __VA_ARGS__) ;			\
    } while (0)





static RemoteReceive createReceive(GQuark app_id,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer process_request_func_data,
				   ZMapRemoteControlReplySentFunc reply_sent_func,
				   gpointer reply_sent_func_data) ;
static void destroyReceive(ZMapRemoteControl remote_control) ;
static gboolean createSend(ZMapRemoteControl remote_control, const char *socket_string,
                           ZMapRemoteControlRequestSentFunc req_sent_func, gpointer req_sent_func_data,
                           ZMapRemoteControlReplyHandlerFunc process_reply_func, gpointer process_reply_func_data,
                           char **err_msg_out) ;
static gboolean recreateSend(ZMapRemoteControl remote_control, char **err_msg_out) ;
static void destroySend(ZMapRemoteControl remote_control) ;

static gboolean waitForRequestCB(gpointer user_data) ;
static gboolean waitForReplyCB(gpointer user_data) ;
static void receiveReplyFromAppCB(void *remote_data, gboolean abort, char *reply) ;

static gboolean setToFailed(ZMapRemoteControl remote_control) ;
static gboolean setToInactive(ZMapRemoteControl remote_control) ;

static char *headerCreate(const char *request_type, char *request_id, int retry_num, char *request_time) ;
static char *headerSetRetry(char *curr_header, int retry_num) ;

static RemoteZeroMQMessage zeroMQMessageCreate(char *header, char *body) ;
static void zeroMQMessageDestroy(RemoteZeroMQMessage zeromq_msg) ;

static gboolean queueMonitorCB(gpointer user_data) ;
static GQueue *queueCreate(void) ;
static void queueAdd(GQueue *gqueue, RemoteZeroMQMessage req_rep) ;
static void queueAddFront(GQueue *gqueue, RemoteZeroMQMessage req_rep) ;
static void queueAddInRequestPriority(GQueue *gqueue, RemoteZeroMQMessage req_rep, void *user_data) ;
static gint higherPriorityCB(gconstpointer a, gconstpointer b, gpointer user_data_unused) ;
static RemoteZeroMQMessage queueRemove(GQueue *gqueue) ;
static RemoteZeroMQMessage queuePeek(GQueue *gqueue) ;
static gboolean queueIsEmpty(GQueue *gqueue) ;
static void queueEmpty(GQueue *gqueue) ;
#ifdef NOT_USED
static void queueDestroy(GQueue *gqueue) ;
#endif
static void freeQueueElementCB(gpointer data, gpointer user_data_unused) ;

static gboolean sendRequest(ZMapRemoteControl remote_control, RemoteZeroMQMessage request) ;
static gboolean passReplyToApp(ZMapRemoteControl remote_control, RemoteZeroMQMessage reply) ;

static gboolean passReceiveToApp(ZMapRemoteControl remote_control, RemoteZeroMQMessage request) ;
static gboolean sendReply(ZMapRemoteControl remote_control, RemoteZeroMQMessage reply) ;

static ReqReply reqReplyCreate(RemoteZeroMQMessage request, RemoteControlRequestType request_type, char *end_point) ;
static void reqReplyAddReply(ReqReply req_reply, RemoteZeroMQMessage reply) ;
static gboolean reqReplyMatch(ZMapRemoteControl remote_control, ReqReply req_reply, char *reply, char **err_msg_out) ;
static RemoteZeroMQMessage reqReplyStealRequest(ReqReply req_rep) ;
static RemoteZeroMQMessage reqReplyStealReply(ReqReply req_rep) ;
gboolean reqReplyIsEarlier(ReqReply req_reply_1, ReqReply req_reply_2) ;
static gboolean reqReplyRequestIsSame(ZMapRemoteControl remote_control,
                                      char *request_1, char *request_2, char **err_msg_out) ;
static void reqReplyDestroy(ReqReply req_reply) ;

int reqIsEarlier(char *request_1_time, char *request_2_time) ;

static gboolean zeroMQSocketRequestorCreate(void *zmq_context, const char *end_point,
                                            void **socket_out, char **err_msg_out) ;
static gboolean zeroMQSocketReplierCreate(void *zmq_context,
                                          void **socket_out, char **end_point_out, char **err_msg_out) ;
static char *zeroMQSocketGetAddress(void *zmq_socket, char **err_msg_out) ;
static TimerData zeroMQSocketSetWatch(GSourceFunc wait_func_cb, ZMapRemoteControl remote_control) ;
static gboolean zeroMQSocketUnSetWatch(TimerData timer_data) ;
static gboolean zeroMQSocketSendMessage(void *zmq_socket, char *header, char *body, char **err_msg_out) ;
static gboolean zeroMQSocketSendFrame(void *zmq_socket, void *msg, int msg_len, gboolean send_more,
                                      char **err_msg_out) ;
static SocketFetchRC zeroMQSocketFetchFrame(void *socket, char **msg_out, char **err_msg_out) ;
static SocketFetchRC zeroMQSocketFetchMessage(void *socket, char **header_out, char **body_out, const char **err_msg_out) ;
static gboolean zeroMQSocketClose(void *zmq_socket, char *endpoint, gboolean disconnect, char **err_msg_out) ;

static void timeoutStartTimer(ZMapRemoteControl remote_control) ;
static TimeoutType timeoutHasTimedOut(ZMapRemoteControl remote_control) ;
static void timeoutResetTimeouts(ZMapRemoteControl remote_control) ;
static void timeoutGetCurrTimeout(ZMapRemoteControl remote_control, int *timeout_num_out, double *timeout_s_out) ;

static void errorHandler(ZMapRemoteControl remote_control,
			 const char *file_name, const char *func_name,
			 ZMapRemoteControlRCType error_rc, const char *format_str, ...) ;
static gboolean stderrOutputCB(gpointer user_data, char *err_msg) ;
static void errorMsg(ZMapRemoteControl remote_control,
                     const char *file_name, const char *func_name, const char *format_str, ...) ;
static void logMsg(ZMapRemoteControl remote_control,
		   const char *file_name, const char *func_name, const char *format_str, va_list args) ;




/*
 *        Globals.
 */

/* Used for validation of ZMapRemoteControlStruct. */
ZMAP_MAGIC_NEW(remote_control_magic_G, ZMapRemoteControlStruct) ;

/* Debugging stuff... */
static ZMapRemoteControlDebugLevelType remote_debug_G = ZMAP_REMOTECONTROL_DEBUG_OFF ;




/*
 *                     External routines
 */



/* Creates a remote control object.
 *
 * If args are wrong returns NULL otherwise the interface is ready to be used.
 *
 */
ZMapRemoteControl zMapRemoteControlCreate(const char *app_id,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_func_data,
					  ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data)
{
  ZMapRemoteControl remote_control = NULL ;


  if (app_id && *app_id && error_func && error_func_data)
    {
      remote_control = g_new0(ZMapRemoteControlStruct, 1) ;
      remote_control->magic = remote_control_magic_G ;

      remote_control->version = g_quark_from_string(ZACP_VERSION) ;

      remote_control->state = REMOTE_STATE_INACTIVE ;

      /* Set up our zeroMQ context for talking to our peer. */
      remote_control->zmq_context = zmq_ctx_new() ;

      remote_control->incoming_requests = queueCreate() ;
      remote_control->outgoing_replies = queueCreate() ;
      remote_control->outgoing_requests = queueCreate() ;
      remote_control->incoming_replies = queueCreate() ;

      /* Set app_id and default error reporting and then we can output error messages. */
      remote_control->app_id = g_quark_from_string(app_id) ;

      /* Init the unique request id. */
      remote_control->request_id = 0 ;

      /* Set up default list of timeout values. */
      zMapRemoteControlSetTimeoutList(remote_control, TIMEOUT_DEFAULT_LIST) ;

      /* Set up apps error handler function we call for serious errors. */
      remote_control->app_error_func = error_func ;
      remote_control->app_error_func_data = error_func_data ;

      /* By default we write errors to stdout but the app can specify somewhere else. */
      if (err_report_func)
	{
	  remote_control->err_report_func = err_report_func ;
	  remote_control->err_report_data = err_report_data ;
	}
      else
	{
	  remote_control->err_report_func = stderrOutputCB ;
	  remote_control->err_report_data = NULL ;
	}

      /* can't be earlier as we need app_id and err_report_XX stuff. */
      REMOTELOGMSG(remote_control, "%s", "RemoteControl object created.") ;
    }

  return remote_control ;
}



/* Initialise the interface to receive commands. We can't receive anything
 * until this is done.
 *
 * Returns TRUE if the interface is successfully initialised and returns the
 * zeromq socket descriptor app_socket_out, caller should g_free() this string when
 * no longer required.
 *  */
gboolean zMapRemoteControlReceiveInit(ZMapRemoteControl remote_control,
                                      char **app_socket_out,
				      ZMapRemoteControlRequestHandlerFunc process_request_func,
				      gpointer process_request_func_data,
				      ZMapRemoteControlReplySentFunc reply_sent_func,
				      gpointer reply_sent_func_data)
{
  gboolean result = FALSE ;
  void *receive_socket ;
  char *socket_address ;
  char *err_msg ;
  TimerData timer_data ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  if (remote_control->receive)
    {
      REMOTELOGMSG(remote_control,
                   "%s",
                   "Receive interface already initialised.") ;

      result = FALSE ;
    }
  else if (!zeroMQSocketReplierCreate(remote_control->zmq_context, &receive_socket, &socket_address, &err_msg))
    {
      REMOTELOGMSG(remote_control, "%s", err_msg) ;
    }
  else if (!(timer_data = zeroMQSocketSetWatch(waitForRequestCB, remote_control)))
    {
      REMOTELOGMSG(remote_control,
                   "Cannot set a watch on zeroMQ socket \"%s\" so cannot wait for requests.", socket_address) ;
    }
  else
    {
      /* Lazy set up of timer routine to monitor our incoming/outgoing queues,
       * not needed until send or receive is set up. */
      if (!(remote_control->queue_monitor_cb_id))
        remote_control->queue_monitor_cb_id = g_timeout_add(QUEUE_WATCH_INTERVAL, queueMonitorCB, remote_control) ;


      remote_control->receive = createReceive(remote_control->app_id,
                                              process_request_func, process_request_func_data,
                                              reply_sent_func, reply_sent_func_data) ;

      remote_control->receive->zmq_socket = receive_socket ;
      remote_control->receive->zmq_end_point = socket_address ;
      remote_control->receive->timer_data = timer_data ;

      remote_control->state = REMOTE_STATE_IDLE ;

      if (app_socket_out)
        *app_socket_out = g_strdup(remote_control->receive->zmq_end_point) ;

      REMOTELOGMSG(remote_control,
                   "Receive interface initialised on \"%s\".",
                   remote_control->receive->zmq_end_point) ;

      result = TRUE ;
    }

  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE, "%s", EXIT_TXT) ;

  return result ;
}


/* Set up the send interface, note that the peer atom string must be unique for this connection,
 * the obvious thing is to use an xwindow id as part of the name as this will be unique. The
 * peer though can choose how it makes the sting unique but it must be _globally_ unique in
 * context of an X Windows server. */
gboolean zMapRemoteControlSendInit(ZMapRemoteControl remote_control,
				   const char *socket_string,
				   ZMapRemoteControlRequestSentFunc req_sent_func,
				   gpointer req_sent_func_data,
				   ZMapRemoteControlReplyHandlerFunc process_reply_func,
				   gpointer process_reply_func_data)
{
  gboolean result = FALSE ;
  char *err_msg = NULL ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  if (remote_control->send)
    {
      REMOTELOGMSG(remote_control,
                   "%s",
                   "Send interface already initialised.") ;

      result = FALSE ;
    }
  else if (!socket_string || !(*socket_string))
    {
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Bad Args: no socket name, send interface initialisation failed.") ;
    }
  else if (createSend(remote_control, socket_string,
                      req_sent_func, req_sent_func_data,
                      process_reply_func, process_reply_func_data,
                      &err_msg))
    {
      /* Lazy set up of timer routine to monitor our incoming/outgoing queues,
       * not needed until send or receive is set up. */
      if (!(remote_control->queue_monitor_cb_id))
        remote_control->queue_monitor_cb_id = g_timeout_add(QUEUE_WATCH_INTERVAL, queueMonitorCB, remote_control) ;

      remote_control->state = REMOTE_STATE_IDLE ;

      REMOTELOGMSG(remote_control,
                   "Send interface initialised on \"%s\".",
                   remote_control->send->zmq_end_point) ;

      result = TRUE ;
    }
  else
    {
      REMOTELOGMSG(remote_control, "%s", err_msg) ;

      g_free(err_msg) ;
    }


  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE, "%s", EXIT_TXT) ;

  return result ;
}



/* Make a request to a peer. */
gboolean zMapRemoteControlSendRequest(ZMapRemoteControl remote_control, char *request)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  if (!(remote_control->send))
    {
      /* send not initialised so nothing we can do. */
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Send interface not initialised so cannot send request.") ;
    }
  else
    {
      /* Queue the request.... */
      char *request_id = NULL, *request_time = NULL ;
      char *header ;

      if (!(request_id = zMapRemoteCommandRequestGetEnvelopeAttr(request, REMOTE_ENVELOPE_ATTR_REQUEST_ID))
          || !(request_time = zMapRemoteCommandRequestGetEnvelopeAttr(request, REMOTE_ENVELOPE_ATTR_TIMESTAMP)))
        {
          REMOTELOGMSG(remote_control,
                       "Request had bad/no %s%s%s",
                       (!request_id ? ZACP_REQUEST_ID : ""),
                       (!request_id ? " and " : ""),
                       (!request_time ? ZACP_REQUEST_TIME : "")) ;
        }
      else
        {
          RemoteZeroMQMessage outgoing_request ;

          header = headerCreate(ZACP_REQUEST, request_id, 1, request_time) ;

          outgoing_request = zeroMQMessageCreate(header, request) ;

          queueAddInRequestPriority(remote_control->outgoing_requests, outgoing_request, NULL) ;

          DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "Received request from zmap, added to outgoing request queue: %s", outgoing_request) ;

          result = TRUE ;
        }
    }

  return result ;
}


/* Attempts to completely reset the remotecontrol object, after this call the send/receive
 * interfaces need to be reinit'd. Returns TRUE if the reset was possible, FALSE otherwise.
 * Fails if the remote control object is dying. */
gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control)
{
  gboolean result = TRUE ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  if (!(result = setToInactive(remote_control)))            /* N.B. sets state to inactive. */
    {
      REMOTELOGMSG(remote_control, "%s", "Could not reset remote control cleanly, object has failed.") ;

      remote_control->state = REMOTE_STATE_FAILED ;
    }

  return result ;
}



/* If remote control has failed then no messages can be sent or received.
 * The app should destroy the remote control object and recreate to
 * re-establish communications with the peer. */
gboolean zMapRemoteControlHasFailed(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  if (remote_control->state == REMOTE_STATE_FAILED || remote_control->state == REMOTE_STATE_DYING)
    result = TRUE ;

  return result ;
}



/* Set debug on/off. */
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, ZMapRemoteControlDebugLevelType debug_level)
{
  gboolean result = TRUE ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  remote_debug_G = debug_level ;

  return result ;
}


/* Takes a comma-separated list of millisecond timeout values, e.g. "5,500,10000" and stores
 * them to be used in controlling timeout behaviour in waiting for peer to respond. */
gboolean zMapRemoteControlSetTimeoutList(ZMapRemoteControl remote_control, const char *peer_timeout_list)
{
  gboolean result = FALSE ;
  gchar **timeouts ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((peer_timeout_list || *peer_timeout_list), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  if ((timeouts = g_strsplit(peer_timeout_list, ",", -1)))
    {
      char **next_timeout = NULL ;
      GArray *new_timeouts ;

      new_timeouts = g_array_sized_new(TRUE, TRUE, sizeof(double), TIMEOUT_LIST_INIT_SIZE) ;

      for (next_timeout = timeouts ; *next_timeout ; next_timeout++)
        {
          double tmp = 0 ;

          if (zMapStr2Double(*next_timeout, &tmp))
            {
              /* Convert to seconds in fractional double form for comparision with GTimer. */
              tmp /= 1000.0 ;
              g_array_append_val(new_timeouts, tmp) ;

              result = TRUE ;
            }
        }

      g_strfreev(timeouts) ;

      if (result)
        {
          if (remote_control->timeout_list)                 /* NULL first time through. */
            g_array_free(remote_control->timeout_list, TRUE) ;

          remote_control->timeout_list = new_timeouts ;
          remote_control->timeout_list_pos = 0 ;
        }
    }

  return result ;
}



/* Specify a function to handle error messages, by default errors are sent to stderr
 * but caller can specify their own routine. Sometimes it's good to allow caller
 * to switch routines (e.g. from graphic to terminal or whatever).
 */
gboolean zMapRemoteControlSetErrorCB(ZMapRemoteControl remote_control,
                                     ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((err_report_func), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  remote_control->err_report_func = err_report_func ;
  remote_control->err_report_data = err_report_data ;

  result = TRUE ;

  return result ;
}


/* Return to default error message handler. */
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), FALSE) ;
  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  remote_control->err_report_func = stderrOutputCB ;
  remote_control->err_report_data = NULL ;

  return result ;
}


/* Free all resources and destroy the remote control block.
 *
 *  */
void zMapRemoteControlDestroy(ZMapRemoteControl remote_control)
{
  char *err_msg ;

  zMapReturnIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G))) ;
  zMapReturnIfFail(remote_control->state != REMOTE_STATE_DYING) ;

  if (remote_control->state != REMOTE_STATE_INACTIVE
      && remote_control->state != REMOTE_STATE_IDLE && remote_control->state != REMOTE_STATE_FAILED)
    {
      REMOTELOGMSG(remote_control,
		   "Remote Control is being destroyed while in %s state, transactions will be lost !",
		   remoteState2ExactStr(remote_control->state)) ;
    }

  /* Set the whole interface to inactive (frees queues etc.). */
  if (remote_control->state != REMOTE_STATE_FAILED || remote_control->state != REMOTE_STATE_INACTIVE)
    setToInactive(remote_control) ;

  /* Get rid of the zeromq context. */
#if (ZMQ_VERSION_MAJOR > 3)
  if (zmq_ctx_term(remote_control->zmq_context) == 0)
#else
  if (zmq_ctx_destroy(remote_control->zmq_context) == 0)
#endif
    err_msg = g_strdup("zeroMQ context destroyed cleanly") ;
  else
    err_msg = g_strdup_printf("zeroMQ context destroy failed: \"%s\".", g_strerror(errno)) ;

  REMOTELOGMSG(remote_control, "%s.", err_msg) ;
  g_free(err_msg) ;

  remote_control->zmq_context = NULL ;


  remote_control->state = REMOTE_STATE_DYING ;

  REMOTELOGMSG(remote_control, "%s", "Destroyed request/reply queues.") ;

  if (remote_control->timeout_list)
    {
      g_array_free (remote_control->timeout_list, TRUE) ;
      remote_control->timeout_list = NULL ;
    }


  /* Before free() reset magic to invalidate this memory block. */
  ZMAP_MAGIC_RESET(remote_control->magic) ;

  g_free(remote_control) ;

  return ;
}





/*
 *                    Package routines.
 */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Not great....need access to a unique id, this should be a "friend" function
 * in C++ terms....not for general consumption. */
unsigned int zmapRemoteControlGetNewReqID(ZMapRemoteControl remote_control)
{
  unsigned int req_id = 0 ;

  zMapReturnValIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G)), 0) ;

  req_id = ++(remote_control->request_id) ;

  return req_id ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Autogenerate function to return string version of enum. */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapRemoteControlRCType2ExactStr, ZMapRemoteControlRCType,
                               ZMAP_REMOTECONTROL_RC_LIST) ;




/*
 *                    Internal routines.
 */


/* Auto generated Enum -> String functions, converts the enums _directly_ to strings.
 *
 * Function signature is of the form:
 *
 *  const char *remoteState2ExactStr(RemoteControlState state) ;
 *
 *  */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(remoteType2ExactStr, RemoteType, REMOTE_TYPE_LIST) ;
ZMAP_ENUM_AS_EXACT_STRING_FUNC(remoteState2ExactStr, RemoteControlState, REMOTE_STATE_LIST) ;



/*
 * Callbacks to handle receiving requests/replies from our peer. These are g_timer
 * callbacks and can be removed by returning FALSE.
 */


/* A GSourceFunc() called every SOCKET_WATCH_INTERVAL ms to check our
 * request receive socket to see if there have been any requests from our peer.
 * If there is a request it is queued for processing.
 */
static gboolean waitForRequestCB(gpointer user_data)
{
  gboolean call_again = TRUE ;                              /* Make sure we are called again. */
  TimerData timer_data = (TimerData)user_data ;
  ZMapRemoteControl remote_control = timer_data->remote_control ;
  RemoteReceive receive = remote_control->receive ;
  SocketFetchRC result = SOCKET_FETCH_FAILED ;
  char *header = NULL ;
  char *request = NULL ;
  const char *err_msg = NULL ;

  if ((result = zeroMQSocketFetchMessage(receive->zmq_socket, &header, &request, &err_msg)) == SOCKET_FETCH_FAILED)
    {
      /* Bad error, disable remote control. */
      REMOTELOGMSG(remote_control, "Error, could not fetch header and request: \"%s\".", err_msg) ;

      call_again = FALSE ;

      setToFailed(remote_control) ;
    }
  else if (result == SOCKET_FETCH_NOTHING)
    {
      /* nothing to do. */
      ;
    }
  else
    {
      RemoteZeroMQMessage incoming_request ;

      incoming_request = zeroMQMessageCreate(header, request) ;

      queueAdd(remote_control->incoming_requests, incoming_request) ;

      DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                  "Received request from peer, adding to incoming request queue: %s", incoming_request) ;
    }

  return call_again ;
}


/* A GSourceFunc() called every SOCKET_WATCH_INTERVAL ms to check our
 * reply receive socket to see if there has been a reply from our peer to our request.
 * If there is a reply it is queued for processing.
 */
static gboolean waitForReplyCB(gpointer user_data)
{
  gboolean call_again = TRUE ;                              /* Make sure we are called again. */
  TimerData timer_data = (TimerData)user_data ;
  ZMapRemoteControl remote_control = timer_data->remote_control ;
  RemoteSend send = remote_control->send ;
  gboolean result ;
  char *header = NULL ;
  char *reply = NULL ;
  const char *err_msg = NULL ;


  if ((result = zeroMQSocketFetchMessage(send->zmq_socket, &header, &reply, &err_msg)) == SOCKET_FETCH_FAILED)
    {
      REMOTELOGMSG(remote_control, "Error, could not fetch reply header and body: \"%s\".", err_msg) ;

      call_again = FALSE ;

      setToFailed(remote_control) ;
    }
  else if (result == SOCKET_FETCH_NOTHING)
    {
      /* nothing to do. */
      ;
    }
  else
    {
      RemoteZeroMQMessage incoming_reply ;

      incoming_reply = zeroMQMessageCreate(header, reply) ;

      queueAdd(remote_control->incoming_replies, incoming_reply) ;

      DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                  "Received reply from peer, adding to incoming reply queue: %s", incoming_reply) ;
    }

  return call_again ;
}


/* THERE'S A SLIGHTLY UNEASY ASSYMETRY HERE.... WE DON'T HAVE A FUNCTION LIKE THIS FOR
 * MAKING REQUESTS TO THE PEER....IT'S CAUSED BY NOT HAVING A CALLBACKS TO SAY THE
 * REQUEST ARRIVED AND THE REPLY GOT SENT, WE JUST RELY ON ZEROMQ HAVING DONE IT. */
/* Called by app to give us the reply to the original request which we then send to the peer. */
static void receiveReplyFromAppCB(void *remote_data, gboolean abort, char *reply)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)remote_data ;

  zMapReturnIfFail((ZMAP_MAGIC_IS_VALID(remote_control->magic, remote_control_magic_G))) ;

  if (abort)
    {
      CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_APP_ABORT,
                       "%s",
                       "App has aborted transaction, see log file for reason.") ;
    }
  else
    {
      /* Queue the reply.... */
      char *reply_id = NULL, *reply_time = NULL ;
      char *header ;

      if (!(reply_id = zMapRemoteCommandRequestGetEnvelopeAttr(reply, REMOTE_ENVELOPE_ATTR_REQUEST_ID))
          || !(reply_time = zMapRemoteCommandRequestGetEnvelopeAttr(reply, REMOTE_ENVELOPE_ATTR_TIMESTAMP)))
        {
          REMOTELOGMSG(remote_control,
                       "Reply had bad/no %s%s%s",
                       (!reply_id ? ZACP_REQUEST_ID : ""),
                       (!reply_id ? " and " : ""),
                       (!reply_time ? ZACP_REQUEST_TIME : "")) ;
        }
      else
        {
          RemoteZeroMQMessage outgoing_reply ;

          header = headerCreate(ZACP_REPLY, reply_id, 1, reply_time) ;

          outgoing_reply = zeroMQMessageCreate(header, reply) ;

          DEBUGLOGMSG(remote_control,  ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "Received zmaps reply, adding to outgoing reply queue: %s",outgoing_reply) ;

          queueAdd(remote_control->outgoing_replies, outgoing_reply) ;
        }
    }

  return ;
}


/* If communication with the peer fails in an unrecoverable way this function is called
 * to set state and clear up, sets state to FAILED so remote control object cannot be
 * used.
 *
 * Returns TRUE if clear up done cleanly, FALSE otherwise. */
static gboolean setToFailed(ZMapRemoteControl remote_control)
{
  gboolean result = TRUE ;

  result = setToInactive(remote_control) ;

  remote_control->state = REMOTE_STATE_FAILED ;

  return result ;
}


/* Set remote control object to inactive by destroying the send/receive
 * interfaces (if they exist).
 *
 * Returns TRUE if object could be set inactively cleanly, FALSE otherwise. */
static gboolean setToInactive(ZMapRemoteControl remote_control)
{
  gboolean result = TRUE ;
  const char *msg ;

  /* Stop monitoring for new requests/replies in the queues. */
  if ((remote_control->queue_monitor_cb_id) && !g_source_remove(remote_control->queue_monitor_cb_id))
    {
      REMOTELOGMSG(remote_control, "%s", "Could not clear up queue monitor cleanly.") ;
      result = FALSE ;
    }
  remote_control->queue_monitor_cb_id = 0 ;

  /* Empty the queues. */
  queueEmpty(remote_control->incoming_requests) ;
  queueEmpty(remote_control->outgoing_replies) ;
  queueEmpty(remote_control->outgoing_requests) ;
  queueEmpty(remote_control->incoming_replies) ;

  remote_control->incoming_requests = remote_control->outgoing_replies = remote_control->outgoing_requests
    = remote_control->incoming_replies = NULL ;

  /* Remove request/reply interfaces (includes destroying zmq sockets). */
  if (remote_control->receive)
    {
      destroyReceive(remote_control) ;
      remote_control->receive = NULL ;
    }

  if (remote_control->send)
    {
      destroySend(remote_control) ;
      remote_control->send = NULL ;
    }

  if (remote_control->curr_req)
    {
      reqReplyDestroy(remote_control->curr_req) ;
      remote_control->curr_req = NULL ;
    }

  if (remote_control->prev_incoming_req)
    {
      reqReplyDestroy(remote_control->prev_incoming_req) ;
      remote_control->prev_incoming_req = NULL ;
    }

  if (remote_control->stalled_req)
    {
      reqReplyDestroy(remote_control->stalled_req) ;
      remote_control->stalled_req = NULL ;
    }

  timeoutResetTimeouts(remote_control) ;

  remote_control->state = REMOTE_STATE_INACTIVE ;

  if (result)
    msg = "RemoteControl interfaces destroyed cleanly." ;
  else
    msg = "Could not destroy RemoteControl interfaces cleanly." ;

  REMOTELOGMSG(remote_control, "%s", msg) ;

  return result ;
}




/* THIS FUNCTION HAS GOT TOO LONG, IT'S HARD TO SEE WHAT'S GOING ON, THE SWITCH CLAUSES
 * NEED TO BE TURNED INTO FUNCTIONS SO THE OVERALL STRUCTURE OF THE SWITCH CAN BE SEEN.... */
/* A GSourceFunc() called back by glib every QUEUE_WATCH_INTERVAL ms to monitor and process
 * incoming and outgoing requests.
 *
 * This function won't be called until either/both of the receive or send interfaces have
 * been initialised.
 *
 * This is the gatekeeper for ordering and processing of requests, we can only be handling
 * one request at a time and collisions and timeouts are handled here.
 * 
 * 
 * You should note the following about the general processing of state and messages:
 * 
 * If the result of an action is that we can process the next state/step immediately
 * then we leave "done" as FALSE so that we loop and process the new state.
 *
 * If the result of the action is that we will need to wait (e.g. for a reply from the peer)
 * then we set "done" to TRUE and exit this routine to be called back again in
 * QUEUE_WATCH_INTERVAL ms by which time the reply or whatever may have happened.
 * 
 *  */
static gboolean queueMonitorCB(gpointer user_data)
{
  gboolean result = TRUE ;                                  /* Usually want to be called back. */
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  gboolean done = FALSE ;

  zMapReturnValIfFail((remote_control->state != REMOTE_STATE_FAILED && remote_control->state != REMOTE_STATE_DYING),
                      FALSE) ;

  while (!done)
    {
      switch (remote_control->state)
        {
        case REMOTE_STATE_INACTIVE:
          {
            LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_STATE,
                             "%s",
                             "Function should not be called while in inactive state, check log for errors.") ;

            break ;
          }

        case REMOTE_STATE_IDLE:
          {
            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "----> In %s state...........", remoteState2ExactStr(remote_control->state)) ;

            /* We only need to do something if there is a previously stalled request
             * or an incoming or outgoing request. */
            if (remote_control->stalled_req)
              {
                remote_control->curr_req = remote_control->stalled_req ;
                remote_control->stalled_req = NULL ;

                if (remote_control->curr_req->request_type == REQUEST_TYPE_INCOMING)
                  remote_control->state = REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY ;
                else
                  remote_control->state = REMOTE_STATE_OUTGOING_REQUEST_WAITING_FOR_THEIR_REPLY ;
              }
            else if (!(queueIsEmpty(remote_control->incoming_requests)
                       && queueIsEmpty(remote_control->outgoing_requests)))
              {
                /* Note deliberate decision: we take the incoming request first. This is separate
                 * from collision handling as we have not yet sent our request so if they have
                 * sent a request we handle that first before sending our own. */
                if (!queueIsEmpty(remote_control->incoming_requests))
                  {
                    remote_control->state = REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED ;
                  }
                else
                  {
                    remote_control->state = REMOTE_STATE_OUTGOING_REQUEST_TO_BE_SENT ;
                  }
              }
            else
              {
                done = TRUE ;
              }

            break ;
          }

        case REMOTE_STATE_OUTGOING_REQUEST_TO_BE_SENT:
          {
            RemoteSend send = remote_control->send ;
            RemoteZeroMQMessage curr_req_raw ;

            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "----> In %s state...........", remoteState2ExactStr(remote_control->state)) ;

            curr_req_raw = queueRemove(remote_control->outgoing_requests) ;

            /* Now call request dispatcher */
            if ((result = sendRequest(remote_control, curr_req_raw)))
              {
                remote_control->curr_req = reqReplyCreate(curr_req_raw,
                                                          REQUEST_TYPE_OUTGOING,
                                                          send->zmq_end_point) ;

                timeoutStartTimer(remote_control) ;

                remote_control->state = REMOTE_STATE_OUTGOING_REQUEST_WAITING_FOR_THEIR_REPLY ;


                DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                            "Sent ZMap's request: %s", curr_req_raw) ;
              }
            else
              {
                LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_SOCKET,
                                         "%s", "Could not send outgoing request.") ;

                zeroMQMessageDestroy(curr_req_raw) ;

                remote_control->state = REMOTE_STATE_IDLE ;
              }

            done = TRUE ;

            break ;
          }

        case REMOTE_STATE_OUTGOING_REQUEST_WAITING_FOR_THEIR_REPLY:
          {
            /* Have sent a request to our peer, waiting for a reply. */

            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "----> In %s state...........", remoteState2ExactStr(remote_control->state)) ;


            if (!queueIsEmpty(remote_control->incoming_replies))
              {
                /* ok...we got a reply so process it, note that strictly we should check
                 * to see if we timed out but why do that now we've got the reply.... */
                RemoteZeroMQMessage reply ;
                char *err_msg = NULL ;

                reply = queueRemove(remote_control->incoming_replies) ;

                DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                            "Received peer's reply", reply) ;

                if (reqReplyMatch(remote_control, remote_control->curr_req, reply->body, &err_msg))
                  {
                    DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                                "Matched peer's reply, removing from input queue: %s", reply) ;


                    /* Pass the reply back to the application. */
                    result = passReplyToApp(remote_control, reply) ;

                    reqReplyDestroy(remote_control->curr_req) ;
                    remote_control->curr_req = NULL ;

                    timeoutResetTimeouts(remote_control) ;
                  }
                else
                  {
                    DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                                "Could not match peer's reply, not removed from input queue: %s", reply) ;


                    LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_SOCKET,
                                     "Reply could not be matched to request because: \"%s\"", err_msg) ;

                    zeroMQMessageDestroy(reply) ;

                    g_free(err_msg) ;
                  }

                remote_control->state = REMOTE_STATE_IDLE ;
              }
            else
              {
                if (!queueIsEmpty(remote_control->incoming_requests))
                  {
                    /* We have sent a request but before we see the reply the peer has also sent
                     * a request, i.e. the peer has launched request at same time as us....if their request
                     * is earlier then we service theirs before going back to waiting for ours. */
                    RemoteZeroMQMessage peer_request ;
                    RemoteZeroMQMessage live_req, stalled_req ; /* For log message. */

                    /* Get the peer request. */
                    peer_request = queuePeek(remote_control->incoming_requests) ;

                    live_req = remote_control->curr_req->request ;
                    stalled_req = peer_request ;

                    if (reqIsEarlier(peer_request->body, remote_control->curr_req->request->body) < 0
                        || strcmp(remote_control->receive->zmq_end_point, remote_control->send->zmq_end_point) < 0)
                      {
                        remote_control->stalled_req = remote_control->curr_req ;
                        remote_control->curr_req = NULL ;

                        live_req = peer_request ;
                        stalled_req = remote_control->stalled_req->request ;

                        remote_control->state = REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED ;
                      }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                    else
                      {
                        done = TRUE ;
                      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


                    REMOTELOGMSG(remote_control,
                                 "Request collision, order of servicing requests will be:"
                                 "\n%s\n\"%s\""
                                 "\nfollowed by:"
                                 "\n%s\n\"%s\"",
                                 live_req->header, live_req->body,
                                 stalled_req->header, stalled_req->body) ;
                  }

                /* ok....we are servicing our request first either because we won the collision
                 * or because there was no incoming request, so check timeout. */
                if (remote_control->state != REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED)
                  {
                    /* Incoming reply queue is empty, check we haven't timed out. */
                    int timeout_num = 0 ;
                    double timeout_s = 0.0 ;
                    TimeoutType timeout_type ;
                    RemoteZeroMQMessage curr_req_raw ;

                    /* Debugging... */
                    timeoutGetCurrTimeout(remote_control, &timeout_num, &timeout_s) ;


                    if ((timeout_type = timeoutHasTimedOut(remote_control)) == TIMEOUT_NONE)
                      {
                        done = TRUE ;
                      }
                    else
                      {
                        char *err_msg = NULL ;

                        /* Keep timed out request so we can resend it or refer to it if we need to. */
                        curr_req_raw = reqReplyStealRequest(remote_control->curr_req) ;

                        if (timeout_type != TIMEOUT_FINAL)
                          {
                            /* Need to redo header.... */
                            curr_req_raw->header
                              = headerSetRetry(curr_req_raw->header,
                                               (remote_control->timeout_list_pos + 1)) ;
                          }

                        reqReplyDestroy(remote_control->curr_req) ;
                        remote_control->curr_req = NULL ;

                        /* Recreate our send socket to clean it all up.  */
                        if (!recreateSend(remote_control, &err_msg))
                          {
                            /* Disaster, we can't reset the socket so set to fail. */
                            LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_SOCKET,
                                                     "Could not recreate send socket: \"%s\" !",
                                                     err_msg) ;

                            setToFailed(remote_control) ;

                            done = TRUE ;
                          }
                        else
                          {
                            if (timeout_type == TIMEOUT_FINAL)
                              {
                                /* Timed out so chuck our request away. */
                                LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_TIMED_OUT,
                                                         "Request final timeout after %gs, request discarded: \"%s\"",
                                                         timeout_s,
                                                         curr_req_raw->body) ;

                                zeroMQMessageDestroy( curr_req_raw) ;

                                timeoutResetTimeouts(remote_control) ;

                                remote_control->state = REMOTE_STATE_IDLE ;
                              }
                            else
                              {
                                REMOTELOGMSG(remote_control,
                                             "Request %d%s timeout after %gs,"
                                             " resending request: \n%s\n\"%s\".",
                                             timeout_num,
                                             (timeout_num == 1 ? "st" :
                                              (timeout_num == 2 ? "nd" :
                                               (timeout_num == 3 ? "rd" : "th"))),
                                             timeout_s,
                                             curr_req_raw->header,
                                             curr_req_raw->body) ;

                                /* must readd to our queue... */
                                queueAddFront(remote_control->outgoing_requests, curr_req_raw) ;


                                remote_control->state = REMOTE_STATE_OUTGOING_REQUEST_TO_BE_SENT ;
                              }
                          }
                      }
                  }
              }

            break ;
          }

        case REMOTE_STATE_INCOMING_REQUEST_TO_BE_RECEIVED:
          {
            RemoteReceive receive  = remote_control->receive ;
            RemoteZeroMQMessage request ;
            char *err_msg = NULL ;

            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "----> In %s state...........", remoteState2ExactStr(remote_control->state)) ;

            request = queueRemove(remote_control->incoming_requests) ;

            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "Received peer's request: %s", request) ;


            if (remote_control->prev_incoming_req
                && reqReplyRequestIsSame(remote_control,
                                         remote_control->prev_incoming_req->request->body, request->body, &err_msg))
              {
                /* There's a new incoming request but it's the same as the previous one, which
                 * means that we must have timed out with our reply to the peer so we just send
                 * the same reply back and throw away the incoming event. */
                RemoteZeroMQMessage raw_reply ;

                raw_reply = reqReplyStealReply(remote_control->prev_incoming_req) ;

                REPORTMSG(remote_control, raw_reply, "Repeat request, resending original reply") ;

                queueAddFront(remote_control->outgoing_replies, raw_reply) ;

                remote_control->curr_req = remote_control->prev_incoming_req ;
                remote_control->prev_incoming_req = NULL ;

                zeroMQMessageDestroy(request) ;             /* Get rid of the incoming request. */

                remote_control->state = REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY ;
              }
            else
              {
                if ((result = passReceiveToApp(remote_control, request)))
                  {
                    remote_control->curr_req = reqReplyCreate(request, REQUEST_TYPE_INCOMING,
                                                              receive->zmq_end_point) ;

                    timeoutStartTimer(remote_control) ;

                    remote_control->state = REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY ;
                  }
                else
                  {
                    LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_SOCKET,
                                             "%s", "Could not receive incoming request.") ;

                    remote_control->state = REMOTE_STATE_IDLE ;

                    done = TRUE ;
                  }
              }

            break ;
          }

        case REMOTE_STATE_INCOMING_REQUEST_WAITING_FOR_OUR_REPLY:
          {
            DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                        "----> In %s state...........", remoteState2ExactStr(remote_control->state)) ;

            if (queueIsEmpty(remote_control->outgoing_replies))
              {
                /* We could add timeout stuff here but we shouldn't need to as the reply is coming
                 * from our application. */
                ;
              }
            else
              {
                RemoteZeroMQMessage reply ;
                char *err_msg = NULL ;

                reply = queueRemove(remote_control->outgoing_replies) ;

                if (!reqReplyMatch(remote_control, remote_control->curr_req, reply->body, &err_msg))
                  {
                    LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_OUT_OF_BAND,
                                             "Reply could not be matched to request because: \"%s\"", err_msg) ;

                    g_free(err_msg) ;

                    zeroMQMessageDestroy(reply) ;

                    remote_control->state = REMOTE_STATE_IDLE ;
                  }
                else if (!(result = sendReply(remote_control, reply)))
                  {
                    LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_SOCKET,
                                             "%s", "Reply could not be sent to peer, discarding reply") ;

                    zeroMQMessageDestroy(reply) ;

                    remote_control->state = REMOTE_STATE_IDLE ;
                  }
                else
                  {
                    reqReplyAddReply(remote_control->curr_req, reply) ;

                    DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                                "Sent ZMap's reply: %s", reply) ;

                    remote_control->prev_incoming_req = remote_control->curr_req ;
                    remote_control->curr_req = NULL ;

                    remote_control->state = REMOTE_STATE_IDLE ;
                  }
              }

            done = TRUE ;

            break ;
          }

        default:
          {
            /* Bad state, set interface to failed. */
            zMapWarnIfReached() ;

            LOG_AND_CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_STATE,
                                     "Remote control disabled because invalid state encountered: %d",
                                     remote_control->state) ;

            setToFailed(remote_control) ;

            break ;
          }
        }
    }

  return result ;
}



/*
 * Functions to send/receive requests/replies via the zeromq sockets.
 */


/* Send a request to our peer.
 *
 * Somewhat mucky, because of the way my xml code works I need to extract
 * the request id and time from the request...not too bad....
 *
 *  */
static gboolean sendRequest(ZMapRemoteControl remote_control, RemoteZeroMQMessage request)
{
  gboolean result = FALSE ;
  RemoteSend send = remote_control->send ;
  char *err_msg = NULL ;

  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
              "About to send request: %s", request->body) ;

  if ((result = zeroMQSocketSendMessage(send->zmq_socket, request->header, request->body, &err_msg)))
    {
      DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                  "Request sent: %s", request) ;

      /* Optionally call app to signal that peer that request has been sent. */
      if (send->req_sent_func)
        {
          DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "%s", "About to call Apps requestSentCB.") ;

          (send->req_sent_func)(send->req_sent_func_data) ;

          DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "%s", "Back from Apps requestSentCB.") ;
        }

      result = TRUE ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
                   "Cannot send message with zeromq send socket: \"%s\"",
                   err_msg) ;

      g_free(err_msg) ;
    }


  return result ;
}



/* Pass our peers reply back to the application. */
static gboolean passReplyToApp(ZMapRemoteControl remote_control, RemoteZeroMQMessage reply)
{
  gboolean result = TRUE ;
  RemoteSend send = remote_control->send ;

  if (send->process_reply_func)
    {
      DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                  "About to call apps ReplyHandlerFunc callback"
                  " to process the reply:\n \"%s\"",
                  reply->body) ;

      /* Call the app callback, this MUST be last because app may make a new call
       * to this code causing us to re-enter. */
      (send->process_reply_func)(remote_control, reply->body, send->process_reply_func_data) ;

      DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                  "\"%s\"", "Back from apps ReplyHandlerFunc callback.") ;
    }

  return result ;
}




/* Receive a request from our peer, bit of a funny function, needs renaming now.... */
static gboolean passReceiveToApp(ZMapRemoteControl remote_control, RemoteZeroMQMessage request)
{
  gboolean result = TRUE ;
  RemoteReceive receive = remote_control->receive ;

  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
              "Calling apps callback func to process request:\n \"%s\"",
              request->body) ;

  /* Call the app callback to process the request, the app will call
   * receiveReplyFromAppCB() with its response, this may happen
   * synchronously or asynchronously. */
  (receive->process_request_func)(remote_control, receiveReplyFromAppCB, remote_control,
                                  request->body,
                                  receive->process_request_func_data) ;

  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
              "Returned from apps callback func to process request:\n \"%s\"",
              request->body) ;

  return result ;
}


/* Send a reply to our peer. */
static gboolean sendReply(ZMapRemoteControl remote_control, RemoteZeroMQMessage reply)
{
  gboolean result = FALSE ;
  RemoteReceive receive = remote_control->receive ;
  char *err_msg = NULL ;

  /* Send the header.... */
  DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
              "About to send reply: %s", reply->body) ;

  if (!(result = zeroMQSocketSendMessage(receive->zmq_socket, reply->header, reply->body, &err_msg)))
    {
      REMOTELOGMSG(remote_control,
                   "Cannot send message with zeromq send socket: \"%s\"",
                   err_msg) ;

      g_free(err_msg) ;
    }
  else
    {
      REPORTMSG(remote_control, reply, "Reply sent") ;

      /* Call back to app to signal that peer has been sent reply. */
      if (receive->reply_sent_func)
        {
          DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "%s", "About to call Apps replySentCB.") ;

          (receive->reply_sent_func)(receive->reply_sent_func_data) ;

          DEBUGLOGMSG(remote_control, ZMAP_REMOTECONTROL_DEBUG_VERBOSE,
                      "%s", "Back from Apps replySentCB.") ;
        }

      result = TRUE ;
    }

  return result ;
}




/*
 * Set of functions to handle request/reply objects, there is one of these created for
 * each of our own requests and for each peer request currently being handled.
 */


static ReqReply reqReplyCreate(RemoteZeroMQMessage request, RemoteControlRequestType request_type, char *end_point)
{
  ReqReply req_reply = NULL ;

  req_reply = g_new0(ReqReplyStruct, 1) ;

  req_reply->request_type = request_type ;

  req_reply->end_point = end_point ;

  req_reply->request = request ;

  req_reply->request_id = zMapRemoteCommandRequestGetEnvelopeAttr(request->body, REMOTE_ENVELOPE_ATTR_REQUEST_ID) ;

  req_reply->command = zMapRemoteCommandRequestGetCommand(request->body) ;

  return req_reply ;
}


static void reqReplyAddReply(ReqReply req_reply, RemoteZeroMQMessage reply)
{
  req_reply->reply = reply ;

  return ;
}


/* Are the two requests the same, especially same id. */
static gboolean reqReplyRequestIsSame(ZMapRemoteControl remote_control,
                                      char *request_1, char *request_2, char **err_msg_out)
{
  gboolean result = FALSE ;

  result = zMapRemoteCommandRequestsIdentical(remote_control, request_1, request_2, err_msg_out) ;

  return result ;
}


/* Does the reply match the request, e.g. request id, command etc. */
static gboolean reqReplyMatch(ZMapRemoteControl remote_control, ReqReply req_reply, char *reply, char **err_msg_out)
{
  gboolean result = FALSE ;

  result = zMapRemoteCommandValidateReply(remote_control, req_reply->request->body, reply, err_msg_out) ;

  return result ;
}



static RemoteZeroMQMessage reqReplyStealRequest(ReqReply req_rep)
{
  RemoteZeroMQMessage request = NULL ;

  request = req_rep->request ;
  req_rep->request = NULL ;                                 /* Prevent it being free'd on reqrep destroy. */

  return request ;
}


static RemoteZeroMQMessage reqReplyStealReply(ReqReply req_rep)
{
  RemoteZeroMQMessage reply = NULL ;

  reply = req_rep->reply ;
  req_rep->reply = NULL ;                                   /* Prevent it being free'd on reqrep destroy. */

  return reply ;
}





/* I'M NOT SURE THIS IS USED NOW....CHECK THIS...SEE FOLLOWING ROUTINE....DOH... */
/* Returns TRUE if request_1 is earlier than request_2, FALSE otherwise. */
gboolean reqReplyIsEarlier(ReqReply req_reply_1, ReqReply req_reply_2)
{
  gboolean result = FALSE ;
  char *request_1_time, *request_2_time ;
  long int request_1_s, request_1_ms, request_2_s, request_2_ms ;


  if (((request_1_time = zMapRemoteCommandRequestGetEnvelopeAttr(req_reply_1->request->body,
                                                                 REMOTE_ENVELOPE_ATTR_TIMESTAMP))
       && zMapTimeGetTime(request_1_time, ZMAPTIME_SEC_MICROSEC, NULL, &request_1_s, &request_1_ms))
      && ((request_2_time = zMapRemoteCommandRequestGetEnvelopeAttr(req_reply_2->request->body,
                                                                    REMOTE_ENVELOPE_ATTR_TIMESTAMP))
          && zMapTimeGetTime(request_2_time, ZMAPTIME_SEC_MICROSEC, NULL, &request_2_s, &request_2_ms)))
    {
      if (request_1_s == request_2_s && request_1_ms == request_2_ms)
        {
          if (strcmp(req_reply_1->end_point, req_reply_2->end_point) < 0)
            result = TRUE ;
        }
      else if (request_1_s < request_2_s
               || (request_1_s == request_2_s && request_1_ms < request_2_ms))
        {
          result = TRUE ;
        }
      else
        {
          result = FALSE ;
        }
    }

  return result ;
}


static void reqReplyDestroy(ReqReply req_reply)
{
  /* Should free the zeromessagestruct....need to do this... */

  /* Does end_point need freeing ?...CHECK  THIS ... */

  if (req_reply->request)
    zeroMQMessageDestroy(req_reply->request) ;

  if (req_reply->reply)
    zeroMQMessageDestroy(req_reply->reply) ;

  g_free(req_reply) ;

  return ;
}





/* Returns:
 *           -1   if request_1 is earlier than request_2
 *            0   if they are the same time
 *            1   if request_1 is later than request_2
 *
 */
int reqIsEarlier(char *request_1, char *request_2)
{
  int result = 0 ;
  char *request_1_time, *request_2_time ;
  long int request_1_s, request_1_ms, request_2_s, request_2_ms ;


  if (((request_1_time = zMapRemoteCommandRequestGetEnvelopeAttr(request_1, REMOTE_ENVELOPE_ATTR_TIMESTAMP))
       && zMapTimeGetTime(request_1_time, ZMAPTIME_SEC_MICROSEC, NULL, &request_1_s, &request_1_ms))
      && ((request_2_time = zMapRemoteCommandRequestGetEnvelopeAttr(request_2,
                                                                    REMOTE_ENVELOPE_ATTR_TIMESTAMP))
          && zMapTimeGetTime(request_2_time, ZMAPTIME_SEC_MICROSEC, NULL, &request_2_s, &request_2_ms)))
    {
      if (request_1_s == request_2_s && request_1_ms == request_2_ms)
        {
          result = 0 ;
        }
      else if (request_1_s < request_2_s
               || (request_1_s == request_2_s && request_1_ms < request_2_ms))
        {
          result = -1 ;
        }
      else
        {
          result = 1 ;
        }
    }

  return result ;
}



/*
 * Functions to manipulate our queues...mostly straight forward "cover" functions
 * for the g_queue_xxx() interface.
 */

static GQueue *queueCreate(void)
{
  GQueue *gqueue ;

  gqueue = g_queue_new() ;

  return gqueue ;
}

/* Usual "add to end of queue" operation.  */
static void queueAdd(GQueue *gqueue, RemoteZeroMQMessage req_rep)
{
  g_queue_push_tail(gqueue, req_rep) ;

  return ;
}

/* Put at front of gqueue. */
static void queueAddFront(GQueue *gqueue, RemoteZeroMQMessage req_rep)
{
  g_queue_push_head(gqueue, req_rep) ;

  return ;
}

/* Here high priority commands are shifted to the front of the queue, where two commands
 * have the same priority they are sorted by the time of the request (earliest first). */
static void queueAddInRequestPriority(GQueue *gqueue, RemoteZeroMQMessage req_rep, void *user_data)
{
  g_queue_insert_sorted(gqueue,
                        req_rep,
                        higherPriorityCB,
                        user_data) ;

  return ;
}

/* A  GCompareDataFunc() to compare two requests and insert the higher priority first. */
static gint higherPriorityCB(gconstpointer a, gconstpointer b, gpointer user_data_unused)
{
  gint priority = 0 ;
  RemoteZeroMQMessage req_rep1 = (RemoteZeroMQMessage)a ;
  RemoteZeroMQMessage req_rep2 = (RemoteZeroMQMessage)b ;
  int priority_1, priority_2 ;

  priority_1 = zMapRemoteCommandGetPriority(req_rep1->body) ;
  priority_2 = zMapRemoteCommandGetPriority(req_rep2->body) ;

  if (priority_1 > priority_2)
    priority = -1 ;
  else if (priority_1 < priority_2)
    priority = 1 ;
  else
    priority = reqIsEarlier(req_rep1->body, req_rep2->body) ;

  return priority ;
}

static RemoteZeroMQMessage queueRemove(GQueue *gqueue)
{
  RemoteZeroMQMessage req_resp = NULL ;

  req_resp = (RemoteZeroMQMessage)g_queue_pop_head(gqueue) ;

  return req_resp ;
}

static RemoteZeroMQMessage queuePeek(GQueue *gqueue)
{
  RemoteZeroMQMessage req_resp = NULL ;

  req_resp = (RemoteZeroMQMessage)g_queue_peek_head(gqueue) ;

  return req_resp ;
}

static gboolean queueIsEmpty(GQueue *gqueue)
{
  gboolean is_empty ;

  is_empty = g_queue_is_empty(gqueue) ;

  return is_empty ;
}

static void queueEmpty(GQueue *gqueue)
{
  if (!queueIsEmpty(gqueue))
    g_queue_foreach(gqueue, freeQueueElementCB, NULL) ;

  return ;
}

static void freeQueueElementCB(gpointer data, gpointer user_data_unused)
{
  RemoteZeroMQMessage zeromq_message = (RemoteZeroMQMessage)data ;

  g_free(zeromq_message->header) ;
  g_free(zeromq_message->body) ;

  g_free(data) ;

  return ;
}


#ifdef NOT_USED
/* gb10: ifdef'd out function to fix compiler warning. Not sure if it's still needed. */
static void queueDestroy(GQueue *gqueue)
{
  queueEmpty(gqueue) ;

  g_queue_free(gqueue) ;

  return ;
}
#endif


/*
 * Functions to handle zeromq sockets.
 */

/* Laborious but important to have good error reporting.... */
static gboolean zeroMQSocketRequestorCreate(void *zmq_context, const char *end_point, void **socket_out, char **err_msg_out)
{
  gboolean result = TRUE ;
  void *socket = NULL ;
  int rc ;

  if (result)
    {
      if (!(socket = zmq_socket(zmq_context, ZMQ_REQ)))
        {
          *err_msg_out = g_strdup_printf("zeroMQ socket create failed: \"%s\".", g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  if (result)
    {
      int zmq_linger_zero = 0 ;
      size_t int_size = sizeof(int) ;

      if ((rc = zmq_setsockopt(socket, ZMQ_LINGER, &zmq_linger_zero, int_size)))
        {

          *err_msg_out = g_strdup_printf("zeroMQ socket create failed: \"%s\".", g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  if (result)
    {
      if ((rc = zmq_connect(socket, end_point)) != 0)
        {
          *err_msg_out = g_strdup_printf("zeroMQ socket create failed: \"%s\".", g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  /* Return the result or clear up. */
  if (result)
    {
      *socket_out = socket ;
    }
  else
    {
      if (socket)
        {
          if ((rc = zmq_close(socket)) != 0)
            {
              char *tmp ;

              tmp = g_strdup_printf("Clear up of socket failed: \"%s\", clear up caused by: %s",
                                    g_strerror(errno), *err_msg_out) ;

              g_free(*err_msg_out) ;

              *err_msg_out = tmp ;

              result = FALSE ;
            }
        }
    }

  return result ;
}


/* Creates a listening socket which will accept requests.
 *
 * The bind is done using a wildcard for a localhost port (i.e. tcp://127.0.0.1:*) which
 * results in the bind choosing a free port for us which we then return to the caller
 * as they will need to return it to our peer.
 *
 * Returns TRUE and the created socket in socket_out and it's port in end_point_out,
 * otherwise returns FALSE and an error message in err_msg_out.
 */
static gboolean zeroMQSocketReplierCreate(void *zmq_context,
                                          void **socket_out, char **end_point_out, char **err_msg_out)
{
  gboolean result = TRUE ;
  void *socket = NULL ;
  char *end_point = NULL ;
  int rc ;

  if (result)
    {
      if (!(socket = zmq_socket(zmq_context, ZMQ_REP)))
        {
          *err_msg_out = g_strdup_printf("zeroMQ reply socket create failed: \"%s\".", g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  if (result)
    {
      if ((rc = zmq_bind(socket, TCP_WILD_CARD_ADDRESS)) != 0)
        {
          *err_msg_out = g_strdup_printf("Could not bind to zeroMQ reply socket: %s.", g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  if (result)
    {
      if (!(end_point = zeroMQSocketGetAddress(socket, err_msg_out)))
        {
          result = FALSE ;
        }
    }

  if (result)
    {
      *socket_out = socket ;
      *end_point_out = end_point ;
    }


  return result ;
}


/* Returns the zmq address of a socket as a string, the string should be g_free'd
 * by the caller when no longer required. */
static char *zeroMQSocketGetAddress(void *zmq_socket, char **err_msg_out)
{
  char *socket_address = NULL ;
  gboolean done ;
  size_t buf_len = ZMQ_ADDR_LEN ;

  done = FALSE ;
  do
    {
      char *buffer ;
      size_t str_len ;

      buffer = (char *)g_malloc0(buf_len) ;                         /* Allows for terminating NULL. */
      str_len = buf_len - 1 ;

      if (zmq_getsockopt(zmq_socket, ZMQ_LAST_ENDPOINT, buffer, &str_len) != 0)
        {
          *err_msg_out = g_strdup_printf("Cannot get zeroMQ end point from socket: %s.",
                                         g_strerror(errno)) ;

          done = TRUE ;
        }
      else
        {
          if (str_len == (buf_len - 1))
            {
              /* Can't be sure if string was  */
              buf_len *= 2 ;

              g_free(buffer) ;
            }
          else
            {
              socket_address = buffer ;

              done = TRUE ;
            }
        }

    }  while (!done) ;


  return socket_address ;
}



static TimerData zeroMQSocketSetWatch(GSourceFunc timer_func_cb, ZMapRemoteControl remote_control)
{
  TimerData timer_data = NULL ;

  timer_data = g_new0(TimerDataStruct, 1) ;
  timer_data->remote_control = remote_control ;
  timer_data->timer_func_cb = timer_func_cb ;
  timer_data->timer_id = g_timeout_add(SOCKET_WATCH_INTERVAL, timer_data->timer_func_cb, timer_data) ;

  return timer_data ;
}


/* Remove the timer routine that is called to check a zmq socket.
 *
 *  */
static gboolean zeroMQSocketUnSetWatch(TimerData timer_data)
{
  gboolean result = FALSE ;

  result = g_source_remove(timer_data->timer_id) ;

  g_free(timer_data) ;

  return result ;
}




static gboolean zeroMQSocketSendMessage(void *zmq_socket, char *header, char *body, char **err_msg_out)
{
  gboolean result = FALSE ;
  int msg_len ;
  char *err_msg = NULL ;

  /* Send the header.... */
  msg_len = strlen(header) ;

  if (!(result = zeroMQSocketSendFrame(zmq_socket, header, msg_len, TRUE, &err_msg)))
    {
      *err_msg_out = err_msg ;
    }
  else
    {
      /* Send the body.... */
      msg_len = strlen(body) ;

      if (!(result = zeroMQSocketSendFrame(zmq_socket, body, msg_len, FALSE, &err_msg)))
        {
          *err_msg_out = err_msg ;
        }
      else
        {
          result = TRUE ;
        }
    }


  return result ;
}


/* Send a message via a zermq socket, returns TRUE if the message could be sent,
 * returns FALSE and an error message if not. */
static gboolean zeroMQSocketSendFrame(void *zmq_socket, void *msg, int msg_len, gboolean send_more,
                                      char **err_msg_out)
{
  gboolean result = FALSE ;
  zmq_msg_t zmq_msg ;
  uint32_t status ;
  size_t sizeof_status = sizeof(status) ;

  /* Can we write to the socket.... */
  if (zmq_getsockopt(zmq_socket, ZMQ_EVENTS, &status, &sizeof_status) != 0)
    {
      *err_msg_out = g_strdup_printf("Cannot access socket to look for requests: %s.",
                                     g_strerror(errno)) ;

      result = FALSE ;
    }
  else if (!(status & ZMQ_POLLOUT))
    {
      *err_msg_out = g_strdup_printf("%s", "Socket in wrong state for sending messages.") ;

      result = FALSE ;
    }
  else
    {
      /* Init the zeromq message for size of message to be sent. */
      if (zmq_msg_init_size(&zmq_msg, msg_len) == -1)
        {
          *err_msg_out = g_strdup_printf("Could not init message size to %d bytes: %s.",
                                         msg_len,
                                         g_strerror(errno)) ;

          result = FALSE ;
        }
      else
        {
          int rc, flags = 0 ;

          if (send_more)
            flags = ZMQ_SNDMORE ;

          /* Copy message into zeromq mesg. */
          memcpy(zmq_msg_data(&zmq_msg), msg, msg_len) ;

          /* Send the message to the socket */
          if ((rc = zmq_msg_send(&zmq_msg, zmq_socket, flags)) == -1)
            {
              *err_msg_out = g_strdup_printf("Cannot send message with zeromq send socket: %s.",
                                             g_strerror(errno)) ;

              result = FALSE ;
            }
          else
            {
              result = TRUE ;
            }
        }
    }

  return result ;
}


/* Peer to peer messages are sent in two frames in the zmq version of the communications, we
 * completely assume that if we get the header then the body should be there. That seems to be
 * part of zmq's guarantee to us, if this fails we do not try to recover but record errors. */
static SocketFetchRC zeroMQSocketFetchMessage(void *socket, char **header_out, char **body_out, const char **err_msg_out)
{
  SocketFetchRC result = SOCKET_FETCH_FAILED ;
  char *header = NULL ;
  char *body = NULL ;
  char *err_msg = NULL ;
  int more = 0 ;
  size_t more_size = sizeof(int) ;

  if ((result = zeroMQSocketFetchFrame(socket, &header, &err_msg)) == SOCKET_FETCH_FAILED)
    {
      *err_msg_out = err_msg ;
    }
  else if (result == SOCKET_FETCH_NOTHING)
    {
      /* Nothing to read so just return. */
      ;
    }
  else if (zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size) != 0)
    {
      *err_msg_out = "Serious error, read first frame of message (header) but there is no more to read from socket." ;
    }
  else
    {
      if ((result = zeroMQSocketFetchFrame(socket, &body, &err_msg)) == SOCKET_FETCH_FAILED)
        {
          /* See comments in function header for why this is a serious error.... */
          char *tmp_msg ;

          tmp_msg = g_strdup_printf("Serious error, read first frame of message (header)"
                                    " but could not read second frame (body): \"%s\".",
                                    err_msg) ;
          g_free(err_msg) ;

          *err_msg_out = tmp_msg ;
        }
      else if (result == SOCKET_FETCH_NOTHING)
        {
          /* See comments in function header for why this is a serious error.... */
          char *tmp_msg ;

          tmp_msg = g_strdup("Serious error, read first frame of message (header)"
                             " but there was nothing to read for the  second frame (body).") ;

          *err_msg_out = tmp_msg ;
        }
      else
        {
          *header_out= header ;
          *body_out = body ;
        }
    }

  return result ;
}





/* Complicated....may have to rework...we shouldn't have EAGAIN if I can get the socket
 * test to work properly....
 *
 * Returns SOCKET_FETCH_OK and a message in msg_out if there was a message,
 * returns SOCKET_FETCH_NOTHING and no message if there was nothing to fetch from the socket,
 * returns SOCKET_FETCH_FAILED and an error message in err_msg_out if there was a serious
 * error on the socket, e.g. it could not be read.
 *  */
static SocketFetchRC zeroMQSocketFetchFrame(void *zmq_socket, char **msg_out, char **err_msg_out)
{
  SocketFetchRC result = SOCKET_FETCH_FAILED ;
  uint32_t status ;
  size_t sizeof_status = sizeof(status) ;

  /* Check to see if we can read a message without blocking. */
  if (zmq_getsockopt(zmq_socket, ZMQ_EVENTS, &status, &sizeof_status) != 0)
    {
      *err_msg_out = g_strdup_printf("Cannot access socket to look for requests: %s.",
                                     g_strerror(errno)) ;
    }
  else if (!(status & ZMQ_POLLIN))
    {
      result = SOCKET_FETCH_NOTHING ;
    }
  else
    {
      zmq_msg_t zmq_msg ;

      if (zmq_msg_init(&zmq_msg) == -1)
        {
          *err_msg_out = g_strdup_printf("Could not init zeromq message: %s.", g_strerror(errno)) ;

          result = SOCKET_FETCH_FAILED ;
        }
      else
        {
          int rc ;                                              /* also gives number of bytes returned. */

          /* If there's a message then read it. */
          if ((rc = zmq_msg_recv(&zmq_msg, zmq_socket, ZMQ_DONTWAIT)) == -1)
            {
              /* If erro is EAGAIN just return and poll again, otherwise it's an error. */
              if (errno == EAGAIN)
                {
                  result = SOCKET_FETCH_NOTHING ;
                }
              else
                {
                  *err_msg_out = g_strdup_printf("Cannot access socket to look for requests: %s.", g_strerror(errno)) ;

                  result = SOCKET_FETCH_FAILED ;
                }
            }
          else
            {
              if (rc == 0)
                {
                  /* Not sure if this can happen outside of our control....however we should trap it. */
                  *err_msg_out = g_strdup_printf("%s", "Socket could be read by there was no message !") ;

                  result = SOCKET_FETCH_FAILED ;
                }
              else
                {
                  /* ok....now let's get the message.... */
                  void *msg_data ;

                  msg_data = zmq_msg_data(&zmq_msg) ;

                  *msg_out = g_strndup((const gchar *)msg_data, rc) ;      /* Turns message into a C string. */

                  result = SOCKET_FETCH_OK ;
                }
            }
        }
    }

  return result ;
}


/* The disconnect may fail if we haven't yet connected to anything but we go ahead
 * do the close whatever happens. */
static gboolean zeroMQSocketClose(void *zmq_socket, char *endpoint, gboolean disconnect, char **err_msg_out)
{
  gboolean result = TRUE ;
  int rc ;

  if (disconnect)
    {
      if ((rc = zmq_disconnect(zmq_socket, endpoint)) != 0)
        {
          *err_msg_out = g_strdup_printf("Could not disconnect socket from end point \"%s\": %s.",
                                         endpoint, g_strerror(errno)) ;

          result = FALSE ;
        }
    }

  if ((rc = zmq_close(zmq_socket)) != 0)
    {
      *err_msg_out = g_strdup_printf("Could not close zeroMQ socket: %s.", g_strerror(errno)) ;

      result = FALSE ;
    }

  return result ;
}





/* The send/receive code works asynchronously and if there are errors, the application needs
 * to be called so that it can set its state. */
static void errorHandler(ZMapRemoteControl remote_control,
                         const char *file_name, const char *func_name,
                         ZMapRemoteControlRCType error_rc, const char *format_str, ...)
{
  va_list args1, args2 ;
  int bytes_printed ;
  char *err_msg = NULL ;
  char *full_err_msg = NULL ;


  /* Log the error, must be done first so current state etc. is recorded in log message. */
  va_start(args1, format_str) ;
  G_VA_COPY(args2, args1) ;

  /* Should have a common message...need rc for failure too.... */
  logMsg(remote_control, file_name, func_name, format_str, args1) ;

  va_end(args1) ;

  if (!(bytes_printed = g_vasprintf(&err_msg, format_str, args2)))
    REMOTELOGMSG(remote_control, "%s",
                 "No error message parameters supplied so error message could be passed back to the application.") ;

  va_end(args2) ;


  full_err_msg = g_strdup_printf("Error code: %s, error: %s", zMapRemoteControlRCType2ExactStr(error_rc),
                                 err_msg) ;

  REMOTELOGMSG(remote_control, "%s", "About to call apps ZMapRemoteControlErrorHandlerFunc().") ;

  /* we need to call the apps error handler func here..... */
  (remote_control->app_error_func)(remote_control,
                                   error_rc, full_err_msg,
                                   remote_control->app_error_func_data) ;

  REMOTELOGMSG(remote_control, "%s", "Back from apps ZMapRemoteControlErrorHandlerFunc().") ;

  g_free(err_msg) ;
  g_free(full_err_msg) ;


  return ;
}


/* Default logging message output function, sends err_msg to stderr and flushes
 * stderr to make the message appear immediately. */
static gboolean stderrOutputCB(gpointer user_data_unused, char *err_msg)
{
  gboolean result = TRUE ;

  if ((strstr(err_msg, ENTER_TXT)))
    fprintf(stderr, "%s", "\n") ;

  fprintf(stderr, "%s\n", err_msg) ;

  if ((strstr(err_msg, EXIT_TXT)))
    fprintf(stderr, "%s", "\n") ;

  fflush(stderr) ;

  return result ;
}

/* Only to be called using REMOTELOGMSG(), do not call directly. Outputs a log message
 * in a standard format.
 *  */
static void errorMsg(ZMapRemoteControl remote_control,
                     const char *file_name, const char *func_name, const char *format_str, ...)
{
  va_list args ;

  va_start(args, format_str) ;

  logMsg(remote_control, file_name, func_name, format_str, args) ;

  va_end(args) ;

  return ;
}


/* Called by errorHandler() and errorMsg(), do not call directly.
 *
 * Produces messages in the format:
 *
 *   "remotecontrol: receiveClipboardClearWaitAfterReplyCB() Acting as: REMOTE_TYPE_SERVER  \
 *                                 State: REMOTE_STATE_IDLE  Message: <msg text>"
 *
 * By default this calls stderrOutputCB() unless caller sets a different output function.
 *
 */
static void logMsg(ZMapRemoteControl remote_control,
                   const char *file_name, const char *func_name, const char *format_str, va_list args)
{
  GString *msg ;
  char *name ;
  gboolean add_time = TRUE ;
  char *time_str = NULL ;

  /* compiler gives full path so chop down to just filename. */
  name = g_path_get_basename(file_name) ;

  if (add_time)
    time_str = zMapGetTimeString(ZMAPTIME_LOG, NULL) ;

  msg = g_string_sized_new(500) ;

  g_string_append_printf(msg, "%s%s%s: %s %s()\tState: %s\nMessage: ",
			 (add_time ? time_str : ""),
			 (add_time ? "\t" : ""),
			 g_quark_to_string(remote_control->app_id),
			 name,
			 func_name,
			 remoteState2ExactStr(remote_control->state)) ;

  g_string_append_vprintf(msg, format_str, args) ;

  if (time_str)
    g_free(time_str) ;

  (remote_control->err_report_func)(remote_control->err_report_data, msg->str) ;

  g_string_free(msg, TRUE) ;

  g_free(name) ;

  return ;
}




/*
 * Functions to create/destroy the receive/send interfaces.
 */


/* Create receive interface struct. */
static RemoteReceive createReceive(GQuark app_id,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer process_request_func_data,
				   ZMapRemoteControlReplySentFunc reply_sent_func,
				   gpointer reply_sent_func_data)
{
  RemoteReceive receive_request ;

  receive_request = g_new0(RemoteReceiveStruct, 1) ;
  receive_request->remote_type = REMOTE_TYPE_SERVER ;

  receive_request->our_app_name_id = app_id ;

  receive_request->process_request_func = process_request_func ;
  receive_request->process_request_func_data = process_request_func_data ;

  receive_request->reply_sent_func = reply_sent_func ;
  receive_request->reply_sent_func_data = reply_sent_func_data ;

  return receive_request ;
}

/* Destroy receive record. */
static void destroyReceive(ZMapRemoteControl remote_control)
{
  RemoteReceive receive_request = remote_control->receive ;
  char *err_msg = NULL ;

  /* Get rid of the zeromq socket stuff. */
  if (receive_request->zmq_socket)
    {
      if (!zeroMQSocketClose(receive_request->zmq_socket, receive_request->zmq_end_point, FALSE, &err_msg))
        {
          REMOTELOGMSG(remote_control, "%s", err_msg) ;

          g_free(err_msg) ;
        }

      receive_request->zmq_socket = NULL ;
    }

  if (receive_request->zmq_end_point)
    g_free(receive_request->zmq_end_point) ;

  if (receive_request->timer_data)
    {
      if (!zeroMQSocketUnSetWatch(receive_request->timer_data))
        {
          REMOTELOGMSG(remote_control, "%s", "Could not clear up socket time cleanly.") ;
        }
    }

  g_free(receive_request) ;

  return ;
}




/* Create send interface struct. */
static gboolean createSend(ZMapRemoteControl remote_control, const char *socket_string,
                           ZMapRemoteControlRequestSentFunc req_sent_func, gpointer req_sent_func_data,
                           ZMapRemoteControlReplyHandlerFunc process_reply_func, gpointer process_reply_func_data,
                           char **err_msg_out)
{
  gboolean result = FALSE ;
  void *send_socket = NULL ;
  char *err_msg = NULL ;
  TimerData timer_data ;

  if (!zeroMQSocketRequestorCreate(remote_control->zmq_context, socket_string,
                                   &send_socket, &err_msg))
    {
      *err_msg_out = g_strdup_printf("Could not set up zmq send socket \"%s\": %s.",
                                     socket_string,
                                     err_msg) ;

      g_free(err_msg) ;
    }
  else if (!(timer_data = zeroMQSocketSetWatch(waitForReplyCB, remote_control)))
    {
      *err_msg_out = g_strdup_printf("Could not set up waiting for reply on socket \"%s\".", socket_string) ;
    }
  else
    {
      remote_control->send = g_new0(RemoteSendStruct, 1) ;
      remote_control->send->remote_type = REMOTE_TYPE_CLIENT ;

      remote_control->send->process_reply_func = process_reply_func ;
      remote_control->send->process_reply_func_data = process_reply_func_data ;

      remote_control->send->req_sent_func = req_sent_func ;
      remote_control->send->req_sent_func_data = req_sent_func_data ;

      remote_control->send->zmq_socket = send_socket ;
      remote_control->send->zmq_end_point = g_strdup(socket_string) ;
      remote_control->send->timer_data = timer_data ;

      result = TRUE ;
    }

  return result ;
}


/* Reset send struct. */
static gboolean recreateSend(ZMapRemoteControl remote_control, char **err_msg_out)
{
  gboolean result = FALSE ;
  char *socket_string  ;
  ZMapRemoteControlRequestSentFunc req_sent_func = remote_control->send->req_sent_func ;
  gpointer req_sent_func_data = remote_control->send->req_sent_func_data;
  ZMapRemoteControlReplyHandlerFunc process_reply_func = remote_control->send->process_reply_func ;
  gpointer process_reply_func_data = remote_control->send->process_reply_func_data ;

  socket_string = g_strdup(remote_control->send->zmq_end_point) ;

  destroySend(remote_control) ;
  remote_control->send = NULL ;

  result = createSend(remote_control, socket_string,
                      req_sent_func, req_sent_func_data,
                      process_reply_func, process_reply_func_data,
                      err_msg_out) ;

  return result ;
}


/* Destroy send, if there is an error returns FALSE and an error message
 * in err_msg_out, BUT note that the send is destroyed anyway. */
static void destroySend(ZMapRemoteControl remote_control)
{
  RemoteSend send = remote_control->send ;
  char *err_msg = NULL ;

  /* Get rid of the zeromq socket stuff. */
  if (send->zmq_socket)
    {
      if (!zeroMQSocketClose(send->zmq_socket, send->zmq_end_point, TRUE, &err_msg))
        {
          REMOTELOGMSG(remote_control, "%s", err_msg) ;

          g_free(err_msg) ;
        }

      send->zmq_socket = NULL ;
    }

  if (send->zmq_end_point)
    g_free(send->zmq_end_point) ;

  if (send->timer_data)
    {
      if (!zeroMQSocketUnSetWatch(send->timer_data))
        {
          REMOTELOGMSG(remote_control, "%s", "Could not clear up socket time cleanly.") ;
        }
    }

  g_free(send) ;

  return ;
}


/*
 * Timeout functions.
 */

static void timeoutStartTimer(ZMapRemoteControl remote_control)
{
  if (!(remote_control->timeout_timer))
    remote_control->timeout_timer = g_timer_new() ;
  else
    g_timer_start(remote_control->timeout_timer) ;

  return ;
}

/* Returns whether we've timed out or not.
 *
 * If a timeout has occurred then the type of that timeout is returned.
 *
 * If the timeout is not the last one in the list then TIMEOUT_NOT_FINAL
 * is returned and the timer is moved on to the next timeout period in the list.
 *
 * If the timeout is the final timeout then TIMEOUT_FINAL is returned,
 * further calls continue to return this value until the timer
 * is explicitly reset and restarted.
 *
 * Otherwise TIMEOUT_NONE is returned.
 *  */
static TimeoutType timeoutHasTimedOut(ZMapRemoteControl remote_control)
{
  TimeoutType timeout_type = TIMEOUT_NONE ;
  double elapsed_time ;
  double timeout_time ;

  elapsed_time = g_timer_elapsed(remote_control->timeout_timer, NULL) ;
  timeout_time = g_array_index(remote_control->timeout_list, double, remote_control->timeout_list_pos) ;

  if (elapsed_time > timeout_time)
    {
      if ((remote_control->timeout_list_pos + 1) == (int)remote_control->timeout_list->len)
        {
          /* final timeout. */
          timeout_type = TIMEOUT_FINAL ;
        }
      else
        {
          /* Intermediate timeout, reset for next timeout. */
          timeout_type = TIMEOUT_NOT_FINAL ;

          remote_control->timeout_list_pos++ ;

          g_timer_start(remote_control->timeout_timer) ;
        }
    }

  return timeout_type ;
}

static void timeoutResetTimeouts(ZMapRemoteControl remote_control)
{
  remote_control->timeout_list_pos = 0 ;

  return ;
}

/* Returns how many timeouts have happened so far and how many seconds the current timeout is set to. */
static void timeoutGetCurrTimeout(ZMapRemoteControl remote_control, int *timeout_num_out, double *timeout_s_out)
{
  *timeout_num_out = (remote_control->timeout_list_pos + 1) ;

  *timeout_s_out = g_array_index(remote_control->timeout_list, double, remote_control->timeout_list_pos) ;

  return ;
}


/* Creates the header for zeromq messages as a string in the form:
 *
 * "<REQUEST | REPLY> <ID>/<RETRY_NUM> <TIME>"
 *
 * e.g.
 *
 * "REQUEST 1/5  1404129247,412653"
 *
 * Time is seconds,microseconds.
 *  */
static char *headerCreate(const char *request_type, char *request_id, int retry_num, char *request_time)
{
  char *header ;
  const char *header_type ;


  if (strcmp(request_type, ZACP_REQUEST) == 0)
    header_type = "REQUEST" ;
  else
    header_type = "REPLY" ;

  header = g_strdup_printf("%s %s/%d %s",
                           header_type, request_id, retry_num, request_time) ;

  return header ;
}


/* Takes an existing header and in effect overwrites the retry number
 * in that header with the new retry number.
 *
 * Note that curr_header is free'd by this function so you should use func like this:
 *
 * my_header = headerSetRetry(my_header, 2) ;
 *  */
static char *headerSetRetry(char *curr_header, int new_retry_num)
{
  char *new_header = NULL ;
  char **split_string, **split_string_orig ;
  char *header_type, *request_id, *request_time ;

  split_string_orig = split_string = g_strsplit_set(curr_header, " /", 0) ;
  header_type = *split_string ;
  split_string++ ;
  request_id = *split_string ;
  split_string++ ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *orig_retry_num ;
  orig_retry_num = *split_string ;                          /* not used...debugging only. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  split_string++ ;
  request_time  = *split_string ;
  split_string++ ;

  new_header =  g_strdup_printf("%s %s/%d %s",
                                header_type, request_id, new_retry_num, request_time) ;

  g_strfreev(split_string_orig) ;

  g_free(curr_header) ;

  return new_header ;
}



static RemoteZeroMQMessage zeroMQMessageCreate(char *header, char *body)
{
  RemoteZeroMQMessage zeromq_msg ;

  zeromq_msg = g_new0(RemoteZeroMQMessageStruct, 1) ;
  zeromq_msg->header = header ;
  zeromq_msg->body = body ;

  return zeromq_msg ;
}


static void zeroMQMessageDestroy(RemoteZeroMQMessage zeromq_msg)
{
  if (zeromq_msg)
    {
      g_free(zeromq_msg->header) ;
      g_free(zeromq_msg->body) ;
      g_free(zeromq_msg) ;
    }

  return ;
}



