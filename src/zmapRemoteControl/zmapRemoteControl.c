/*  File: zmapRemoteControl.c
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

#include <ZMap/zmap.h>

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <gtk/gtk.h>

#include <ZMap/zmapUtils.h>
#include <zmapRemoteControl_P.h>



/* Text for debug/log messages.... */
#define PEER_PREFIX   "PEER"
#define CLIENT_PREFIX "CLIENT"
#define SERVER_PREFIX "SERVER"

#define ENTER_TXT "Enter >>>>"
#define EXIT_TXT  "Exit  <<<<"

#define WRONG_STATE_STR "Wrong State !! Expected state to be: \"%s\". "
#define OUT_OF_BAND_STR "Received out-of-band \"%s\" while waiting for \"%s\" !!"



/* 
 * Use these macros for _ALL_ logging calls/error handling in this code
 * to ensure that messages go to the right place.
 */
#define REMOTELOGMSG(REMOTE_CONTROL, FORMAT_STR, ...)			\
  do									\
    {									\
      errorMsg((REMOTE_CONTROL),					\
	       __FILE__,						\
	       __PRETTY_FUNCTION__,					\
	       (FORMAT_STR), __VA_ARGS__) ;				\
    } while (0)

/* Switchable version of remotelogmsg for debugging sessions. */
#define DEBUGLOGMSG(REMOTE_CONTROL, FORMAT_STR, ...)			\
  do									\
    {									\
      if (remote_debug_G)						\
        {								\
	  REMOTELOGMSG((REMOTE_CONTROL),				\
		       (FORMAT_STR), __VA_ARGS__) ;			\
	}								\
    } while (0)


/* Messages for common errors in code. */
#define BADSTATELOGMSG(REMOTE_CONTROL, STATE, FORMAT_STR, ...)		\
  do									\
    {									\
      REMOTELOGMSG((REMOTE_CONTROL),					\
		   WRONG_STATE_STR FORMAT_STR,				\
		   remoteState2ExactStr((STATE)),			\
		   __VA_ARGS__) ;					\
    } while (0)


#define CALL_ERR_HANDLER(REMOTE_CONTROL, ERROR_RC, FORMAT_STR, ...)	\
  do									\
    {									\
      errorHandler((REMOTE_CONTROL),					\
		   __FILE__,						\
		   __PRETTY_FUNCTION__,					\
		   (ERROR_RC),						\
		   (FORMAT_STR), __VA_ARGS__) ;				\
    } while (0)

#define CALL_BADSTATE_HANDLER(REMOTE_CONTROL, STATE, FORMAT_STR, ...)	\
  do									\
    {									\
      errorHandler((REMOTE_CONTROL),					\
		   __FILE__,						\
		   __PRETTY_FUNCTION__,					\
		   ZMAP_REMOTECONTROL_RC_BAD_STATE,			\
		   WRONG_STATE_STR FORMAT_STR,				\
		   remoteState2ExactStr((STATE)),			\
		   __VA_ARGS__) ;					\
    } while (0)

#define CALL_OUTOFBAND_HANDLER(REMOTE_CONTROL, BAD_SIGNAL, EXPECTED_SIGNAL) \
  do									\
    {									\
      errorHandler((REMOTE_CONTROL),					\
		   __FILE__,						\
		   __PRETTY_FUNCTION__,					\
		   ZMAP_REMOTECONTROL_RC_OUT_OF_BAND,			\
		   OUT_OF_BAND_STR,					\
		   (BAD_SIGNAL),					\
		   (EXPECTED_SIGNAL)) ;					\
    } while (0)


/* some useful timeout values... */
enum
  {
    NULL_TIMEOUT = 0,					    /* Turns timeouts off. */
    DEFAULT_TIMEOUT = 500,				    /* Standard timeout, needs testing. */
    DEBUG_TIMEOUT = 3600000				    /* Debug timeout of an hour.... */
  } ;



static void sendClipboardSendRequestCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data) ;
static void sendClipboardClearCB(GtkClipboard *clipboard, gpointer user_data) ;
static void sendReplyWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					guint info, gpointer user_data) ;
static void sendReplyWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data) ;
static void sendGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data) ;
static void sendRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					      guint info, gpointer user_data) ;
static void sendRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;

static void receiveWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				      guint info, gpointer user_data) ;
static void receiveWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void receiveRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
						 guint info, gpointer user_data) ;
static void receiveRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void receiveGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer data) ;
static void receiveAppCB(void *remote_data, gboolean abort, char *reply) ;
static void receiveClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data) ;
static void receiveClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data) ;

static void endOfReceiveClipboardClearCB(GtkClipboard *clipboard, gpointer user_data) ;
static void endReceiveClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				     guint info, gpointer user_data) ;

static RemoteIdle createIdle(void) ;
static RemoteReceive createReceive(GQuark app_id, char *app_str,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer process_request_func_data,
				   ZMapRemoteControlReplySentFunc reply_sent_func,
				   gpointer reply_sent_func_data) ;
static RemoteSend createSend(char *app_name, char *send_unique_atom_string,
			     ZMapRemoteControlRequestSentFunc req_sent_func, gpointer req_sent_func_data,
			     ZMapRemoteControlReplyHandlerFunc process_reply_func, gpointer process_reply_func_data) ;
static void resetIdle(RemoteIdle idle_request) ;
static void resetReceive(RemoteReceive receive_request) ;
static void resetSend(RemoteSend send_request) ;
static void destroyIdle(RemoteIdle idle_request) ;
static void destroyReceive(RemoteReceive client_request) ;
static void destroySend(RemoteSend server_request) ;

static gboolean clipboardTakeOwnership(GtkClipboard *clipboard,
				       GtkClipboardGetFunc get_func, GtkClipboardClearFunc clear_func,
				       gpointer user_data) ;
static void resetRemoteToIdle(ZMapRemoteControl remote_control) ;
static gboolean isClipboardDataValid(GtkSelectionData *selection_data, char *curr_selection, char **err_msg_out) ;
static char *getClipboardData(GtkSelectionData *selection_data, char **err_msg_out) ;
gboolean isInReceiveState(RemoteControlState state) ;
gboolean isInSendState(RemoteControlState state) ;

static void addTimeout(ZMapRemoteControl remote_control) ;
static gboolean haveTimeout(ZMapRemoteControl remote_control) ;
static void removeTimeout(ZMapRemoteControl remote_control) ;
static gboolean timeOutCB(gpointer user_data) ;

static void errorHandler(ZMapRemoteControl remote_control,
			 const char *file_name, const char *func_name,
			 ZMapRemoteControlRCType error_rc, char *format_str, ...) ;
static void errorMsg(ZMapRemoteControl remote_control,
		     const char *file_name, const char *func_name, char *format_str, ...) ;
static void logMsg(ZMapRemoteControl remote_control,
		   const char *file_name, const char *func_name, char *format_str, va_list args) ;
static gboolean stderrOutputCB(gpointer user_data, char *err_msg) ;



/* 
 *        Globals.
 */

/* Used for validation of ZMapRemoteControlStruct. */
ZMAP_MAGIC_NEW(remote_control_magic_G, ZMapRemoteControlStruct) ;



/* Descriptor of data type we support for the properties....note that the id
 * is local to the application, a kind of shortcut way of checking the clipboard format. */
#define TARGET_ID 111111
#define TARGET_NUM 1
static GtkTargetEntry clipboard_target_G[TARGET_NUM] = {{ZACP_DATA_TYPE, 0, TARGET_ID}} ;



/* Debugging stuff... */
static gboolean remote_debug_G = TRUE ;




/* 
 *                     External routines
 */


/* Creates a remote control object.
 * 
 * NOTE that you must only call this function after you have displayed some windows (I'm not sure
 * of the precise restrictions as they aren't documented in gtk). A good way to do this is to have
 * a "map-event" handler attached to a window that you always display in your application and make
 * this call from there.
 * 
 * If args are wrong call will return NULL otherwise the interface is ready to be used.
 * 
 * To make the requests the caller must know the peer's unique id and have called remote_control
 * object to set it up and make then make the request.
 * 
 */
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_func_data,
					  ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data)
{
  ZMapRemoteControl remote_control = NULL ;

  if (app_id && *app_id && error_func && error_func_data)
    {
      remote_control = g_new0(ZMapRemoteControlStruct, 1) ;
      remote_control->magic = remote_control_magic_G ;

      remote_control->version = g_quark_from_string(ZACP_VERSION) ;

      remote_control->state = REMOTE_STATE_IDLE ;
      remote_control->idle = createIdle() ;
      remote_control->curr_remote = (RemoteAny)(remote_control->idle) ;


      /* Set app_id and default error reporting and then we can output error messages. */
      remote_control->app_id = g_quark_from_string(app_id) ;

      /* Data type id as an atom for use in set/get selection. */
      remote_control->target_atom = gdk_atom_intern(ZACP_DATA_TYPE, FALSE) ;

      /* Init the unique request id. */
      remote_control->request_id_num = 0 ;
      remote_control->request_id = g_string_new("") ;

      /* Set a default timeout of 0.5 seconds, probably reasonable. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      remote_control->timeout_ms = DEFAULT_TIMEOUT ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      remote_control->timeout_ms = NULL_TIMEOUT ;

      remote_control->app_error_func = error_func ;
      remote_control->app_error_func_data = error_func_data ;

      if (err_report_func)
	{
	  remote_control->app_err_report_func = err_report_func ;
	  remote_control->app_err_report_data = err_report_data ;
	}
      else
	{
	  remote_control->app_err_report_func = stderrOutputCB ;
	  remote_control->app_err_report_data = NULL ;
	}

      /* can't be earlier as we need app_id and err_report_XX stuff. */
      DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

      REMOTELOGMSG(remote_control, "%s", "RemoteControl object created.") ;

      DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;
    }

  return remote_control ;
}



/* Initialise our own clipboard to receive commands. We can't receive commands
 * until this is done.
 *  */
gboolean zMapRemoteControlReceiveInit(ZMapRemoteControl remote_control, char *app_unique_str,
				      ZMapRemoteControlRequestHandlerFunc process_request_func,
				      gpointer process_request_func_data,
				      ZMapRemoteControlReplySentFunc reply_sent_func,
				      gpointer reply_sent_func_data)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->receive)
   {
     REMOTELOGMSG(remote_control,
		  "%s",
		  "Receive interface already initialised.") ;

     result = FALSE ;
   }
  else if (isInReceiveState(remote_control->state) || remote_control->state != REMOTE_STATE_IDLE)
    {
      REMOTELOGMSG(remote_control,
		  "%s", "Must be in idle state or non-client state to initialise \"receive\".") ;

      result = FALSE ;
    }
  else if (!app_unique_str || !(*app_unique_str))
    {
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Bad Args: no clipboard string, receive interface initialisation failed.") ;
    }
  else
    {
      remote_control->receive = createReceive(remote_control->app_id, app_unique_str,
					      process_request_func, process_request_func_data,
					      reply_sent_func, reply_sent_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Receive interface initialised (receive clipboard is \"%s\").",
		   remote_control->receive->our_atom_string) ;
      
      result = TRUE ;
    }    

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;				   

  return result ;
}


/* Set up the send interface, note that the peer atom string must be unique for this connection,
 * the obvious thing is to use an xwindow id as part of the name as this will be unique. The
 * peer though can choose how it makes the sting unique but it must be _globally_ unique in
 * context of an X Windows server. */
gboolean zMapRemoteControlSendInit(ZMapRemoteControl remote_control,
				   char *send_app_name, char *send_unique_string,
				   ZMapRemoteControlRequestSentFunc req_sent_func,
				   gpointer req_sent_func_data,
				   ZMapRemoteControlReplyHandlerFunc process_reply_func,
				   gpointer process_reply_func_data)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->send)
   {
     REMOTELOGMSG(remote_control,
		  "%s",
		  "Send interface already initialised.") ;

     result = FALSE ;
   }
  else if (!isInSendState(remote_control->state) && remote_control->state != REMOTE_STATE_IDLE)
    {
      REMOTELOGMSG(remote_control,
		  "%s", "Must be in idle state or in server state to initialise \"send\".") ;

      result = FALSE ;
    }
  else if (!send_unique_string || !(*send_unique_string))
    {
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Bad Args: no send clipboard string, send interface initialisation failed.") ;
    }
  else
    {
      remote_control->send = createSend(send_app_name, send_unique_string,
					req_sent_func,
					req_sent_func_data,
					process_reply_func, process_reply_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Send interface initialised (send clipboard is \"%s\").",
		   remote_control->send->their_atom_string) ;

      result = TRUE ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;				   

  return result ;
}



/* Until this call is issued remote control will not respond to any requests. */
gboolean zMapRemoteControlReceiveWaitForRequest(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;
  RemoteReceive receive ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  receive = remote_control->receive ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      BADSTATELOGMSG(remote_control, REMOTE_STATE_IDLE,
		     "Cannot wait on receive clipboard \"%s\".",
		     receive->our_atom_string) ;


      result = FALSE ;
    }
  else if (!(remote_control->receive))
    {
      REMOTELOGMSG(remote_control,
		   "Receive interface is not initialised so cannot wait on our clipboard \"%s\".",
		   receive->our_atom_string) ;

      result = FALSE ;
    }
  else
    {
      if ((result = clipboardTakeOwnership(receive->our_clipboard,
					   receiveWaitClipboardGetCB, receiveWaitClipboardClearCB,
					   remote_control)))
	{
	  REMOTELOGMSG(remote_control,
		       "Waiting (have ownership of) on our clipboard \"%s\").",
		       receive->our_atom_string) ;


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_NEW_REQ ;
	  remote_control->curr_remote = (RemoteAny)(remote_control->receive) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Cannot become owner of own clipboard \"%s\" so cannot wait for requests.",
		       receive->our_atom_string) ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}


/* Make a request to a peer. */
gboolean zMapRemoteControlSendRequest(ZMapRemoteControl remote_control, char *request)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE && remote_control->state != REMOTE_STATE_SERVER_WAIT_NEW_REQ)
    {
      /* Can only switch to making requests from "idle" or "waiting for request" states. */
      REMOTELOGMSG(remote_control,
		   "Current state is %s but must be in %s or %s states to initiate making a requests,"
		   " command was:\"%s\".",
		   remoteState2ExactStr(remote_control->state),
		   remoteState2ExactStr(REMOTE_STATE_IDLE),
		   remoteState2ExactStr(REMOTE_STATE_SERVER_WAIT_NEW_REQ),
		   request) ;

      result = FALSE ;
    }
  else if (!(remote_control->send))
    {
      /* we have not been given the send id so cannot make requests to it. */
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Send interface not initialised so cannot send request.") ;

      result = FALSE ;
    }
  else
    {
      RemoteSend send ;

      /* If we are waiting for requests then reset to idle before making our request. */
      if (remote_control->state == REMOTE_STATE_SERVER_WAIT_NEW_REQ)
	resetRemoteToIdle(remote_control) ;

      /* Try sending a request to our send. */
      send = remote_control->send ;
      send->our_request = g_strdup(request) ;

      /* Swop to client interface to make request. */
      remote_control->curr_remote = (RemoteAny)(remote_control->send) ;

      REMOTELOGMSG(remote_control, "Taking ownership of peer clipboard \"%s\" to make request.",
		   send->their_atom_string, request) ;

      if ((result = gtk_clipboard_set_with_data(send->their_clipboard,
						clipboard_target_G,
						TARGET_NUM,
						sendClipboardSendRequestCB,
						sendClipboardClearCB,
						remote_control)))
	{
	  REMOTELOGMSG(remote_control,
		       "Have ownership of peer clipboard \"%s\", waiting for peer to ask for our request.",
		       send->their_atom_string) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_GET ;

	  /* Now we are waiting so set timeout. */
	  addTimeout(remote_control) ;

	  result = TRUE ;
	}
      else
	{
    	  REMOTELOGMSG(remote_control,
		       "Cannot set request on peer clipboard \"%s\".",
		       send->their_atom_string) ;

	  resetRemoteToIdle(remote_control) ;

	  result = FALSE ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}



/* Resets the remote interface to REMOTE_STATE_IDLE. */
void zMapRemoteControlReset(ZMapRemoteControl remote_control)
{
  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  resetRemoteToIdle(remote_control) ;

  return ;
 }


/* Set debug on/off. */
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on)
{
  gboolean result = TRUE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  remote_debug_G = debug_on ;

  return result ;
 }



/* Set timeout, if timeout_secs <= 0 then no timeouts are set,
 * good for testing, not for real life ! */
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs)
{
  gboolean result = TRUE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  if (timeout_secs < 0)
    remote_control->timeout_ms = 0 ;
  else
    remote_control->timeout_ms = timeout_secs * 1000 ;

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

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;
  zMapAssert(err_report_func) ;

  remote_control->app_err_report_func = err_report_func ;
  remote_control->app_err_report_data = err_report_data ;

  return result ;
 }


/* Return to default error message handler. */
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  remote_control->app_err_report_func = stderrOutputCB ;
  remote_control->app_err_report_data = NULL ;

  return result ;
 }



/* Free all resources and destroy the remote control block, returns
 * TRUE if the object was idle and could be destroyed and FALSE
 * otherwise.
 * 
 * It's unusual to return a value to indicate failure of a destroy
 * function but if the application calls this from the "wrong" callback
 * it's possible because of the inherent asynchronicity of X Windows
 * for one of the callbacks in this file to be called with an invalid
 * RemoteControl pointer. This function requires the remote control
 * object to be in the idle state.
 * 
 *  */
gboolean zMapRemoteControlDestroy(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state == REMOTE_STATE_IDLE)
    {
      if (remote_control->idle)
	destroyIdle(remote_control->idle) ;
      if (remote_control->receive)
	destroyReceive(remote_control->receive) ;
      if (remote_control->send)
	destroySend(remote_control->send) ;

      REMOTELOGMSG(remote_control, "%s", "Destroyed RemoteControl object.") ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Object cannot be destroyed, must be in %s state but is in %s state.",
		   remoteState2ExactStr(REMOTE_STATE_IDLE), remoteState2ExactStr(remote_control->state)) ;
    }

  /* Make sure this happens whether destroy succeeds or not.... */
  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;		    /* Latest we can make this call as we 
							       need remote_control. */

  if (remote_control->state == REMOTE_STATE_IDLE)
    {
      ZMAP_MAGIC_RESET(remote_control->magic) ;		    /* Before free() reset magic to
							       invalidate this memory block. */

      g_free(remote_control) ;

      result = TRUE ;
    }

  return result ;
}



/* 
 *                    Package routines.
 */

/* Not great....need access to a unique id, this should be a "friend" function
 * in C++ terms....not for general consumption. */
char *zmapRemoteControlMakeReqID(ZMapRemoteControl remote_control)
{
  char *id = NULL ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;
  
  (remote_control->request_id_num)++ ;
  g_string_printf(remote_control->request_id, "%d", remote_control->request_id_num) ;
  id = remote_control->request_id->str ;

  return id ;
}




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



/* Get rid of this.....??? */
/* NOT USEFUL....JUST PUT THE CLIPBOARD CALLS INSTEAD.... */
/* Take ownership of a clipboard ready to receive commands. */
static gboolean clipboardTakeOwnership(GtkClipboard *clipboard,
				       GtkClipboardGetFunc get_func, GtkClipboardClearFunc clear_func,
				       gpointer user_data)
{
  gboolean result = FALSE ;

  result = gtk_clipboard_set_with_data(clipboard,
				       clipboard_target_G,
				       TARGET_NUM,
				       get_func,
				       clear_func,
				       user_data) ;

  return result ;
}







/* 
 *             Callbacks for making requests.
 * 
 * They form a chain linking from one to the other if all goes well. They are all callbacks
 * called by gtk clipboard/selection handling code, no direct event handling/processing
 * is required.
 * 
 */

/* Send Request Step 1)
 * We have ownership of the peers clipboard and this function is called when they do a "get"
 * request to ask us for our request which we now set on the clipboard.
 */

/* GtkClipboardGetFunc() called if there is a 'get' request for a clipboards contents. */
static void sendClipboardSendRequestCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;
  GdkAtom target_atom ;
  char *target_name ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      send = remote_control->send ;

      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_GET)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
				"Already servicing a request: \"%s\".",
				send->our_request) ;
	}
      else
	{
	  target_atom = gtk_selection_data_get_target(selection_data) ;
	  target_name = gdk_atom_name(target_atom) ;

	  if (info != TARGET_ID)
	    {
	      CALL_ERR_HANDLER(remote_control,
			       ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "Unsupported Data Type requested: %s.", target_name) ;
	    }
	  else
	    {
	      /* All ok so set our request on the peers clipboard. */

	      REMOTELOGMSG(remote_control, "Setting our request on clipboard \"%s\" using property \"%s\","
			   "request is:\n \"%s\".",
			   send->their_atom_string,
			   target_name, send->our_request) ;

	      /* Set the data in the selection data param, N.B. the terminating NULL is not sent. */
	      gtk_selection_data_set(selection_data,
				     remote_control->target_atom,
				     8,
				     (guchar *)(send->our_request),
				     (int)(strlen(send->our_request))) ;

	      remote_control->state = REMOTE_STATE_CLIENT_WAIT_REQ_ACK ;

	      /* Now we are waiting so set timeout. */
	      addTimeout(remote_control) ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* Send Request Step 2)
 * Peer has received our request and retakes ownership of it's clipboard
 * to signal it has the request.
 * The get callback should not be called at this stage.
 */

/* GtkClipboardClearFunc() function called when a peer takes ownership of the clipboard. */
static void sendClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      send = remote_control->send ;

      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REQ_ACK)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_CLIENT_WAIT_REQ_ACK,
				"Already servicing a request: \"%s\".",
				send->our_request) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Peer has retaken ownership their clipboard \"%s\" to signal they have received our request.",
		       send->their_atom_string) ;


	  /* Retake ownership of the peers clipboard to signal that we know they got the request and we
	   * are now waiting for their reply. */
	  if (!clipboardTakeOwnership(send->their_clipboard,
				      sendReplyWaitClipboardGetCB, sendReplyWaitClipboardClearCB,
				      remote_control))
	    {
	      CALL_ERR_HANDLER(remote_control,
			       ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "Failed to retake ownership of their clipboard \"%s\", aborting request.",
			       send->their_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Have retaken ownership of their clipboard \"%s\" to signal they can process the request.",
			   send->their_atom_string) ;

	      remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY ;

	      /* Optionally call app to signal that peer has received their request. */
	      if (send->req_sent_func)
		(send->req_sent_func)(send->req_sent_func_data) ;

	      /* Now we are waiting so set timeout. */
	      addTimeout(remote_control) ;
	    }

	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* Send Request Step 3)
 * Peer wants to return its reply to our request so takes ownership of the clipboard
 * to signal this. We then request the clipboard contents.
 * Our get callback should not be called at this stage.
 * 
 */

/* GtkClipboardClearFunc() function called when a peer takes ownership of the clipboard. */
static void sendReplyWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      send = remote_control->send ;

      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_CLIENT_WAIT_REPLY,
				"Already servicing a request: \"%s\".",
				send->our_request) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Lost ownership of receive clipboard \"%s\", asking for peers reply to our request.",
		       send->their_atom_string) ;

	  gtk_clipboard_request_contents(clipboard,
					 remote_control->target_atom,
					 sendGetRequestClipboardCB, remote_control) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_SEND ;

	  /* Now we are waiting so set timeout. */
	  addTimeout(remote_control) ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}

/* GtkClipboardGetFunc() called if someone does a 'get' on the clipboard. */
static void sendReplyWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      CALL_OUTOFBAND_HANDLER(remote_control, "get", "clear") ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* Send Request Step 4)
 * Peer returns its reply to us, we then retake ownership of its clipboard to signal
 * that we have the reply.
 * 
 */

/* GtkClipboardReceivedFunc() function called when peer sends us it's reply. */
static void sendGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      send = remote_control->send ;

      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_SEND)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_CLIENT_WAIT_SEND,
				"Already servicing a request: \"%s\".",
				send->our_request) ;
	}
      else
	{
	  char *selection_str ;
	  char *err_msg = NULL ;

	  REMOTELOGMSG(remote_control,
		       "Received reply from peer on receive clipboard \"%s\".",
		       send->their_atom_string) ;


	  /* Check and record the reply if all ok, we'll process it when we get a clear from the peer. */
	  if (!isClipboardDataValid(selection_data, send->their_atom_string, &err_msg))
	    {
	      CALL_ERR_HANDLER(remote_control,
			       ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "%s", err_msg) ;

	      g_free(err_msg) ;
	    }
	  else if (!(selection_str = getClipboardData(selection_data, &err_msg)))
	    {
	      CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "%s", err_msg) ;

	      g_free(err_msg) ;
	    }	  
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Retrieved peers reply from clipboard \"%s\", reply is:\n \"%s\".",
			   send->their_atom_string, selection_str) ;

	      send->their_reply = selection_str ;

	      /* Retake ownership so they know we have got reply. */
	      if (!clipboardTakeOwnership(send->their_clipboard,
					  sendRequestReceivedClipboardGetCB, sendRequestReceivedClipboardClearCB,
					  remote_control))
		{
		  CALL_ERR_HANDLER(remote_control,
				   ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
				   "Tried to retake ownership of receive clipboard \"%s\" to signal peer that we have"
				   " request but this failed, aborting request.",
				   send->their_atom_string) ;
		}
	      else
		{
		  REMOTELOGMSG(remote_control,
			       "Have retaken ownership of receive clipboard \"%s\" to signal peer that we have their reply.",
			       send->their_atom_string) ;

		  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY_ACK ;

		  /* Now we are waiting so set timeout. */
		  addTimeout(remote_control) ;
		}
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/* Send Request Step 5)
 * Peer takes ownership of its clipboard to signal that they know we have the reply.
 * that we have the reply. We pass the reply back to the application and reset our state
 * to waiting for a request.
 * Our get callback should not be called at this stage.
 */

/* A GtkClipboardClearFunc() function called when the clipboard is cleared. */
static void sendRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      send = remote_control->send ;

      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY_ACK)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_CLIENT_WAIT_REPLY_ACK,
				"Already servicing a request: \"%s\".",
				send->our_request) ;
	}
      else
	{
	  char *their_reply ;

	  REMOTELOGMSG(remote_control,
		       "Lost ownership of peer clipboard \"%s\", they now know we have the reply.",
		       send->their_atom_string) ;

	  REMOTELOGMSG(remote_control, "About to call apps callback to process the reply:\n \"%s\"", send->their_reply) ;

	  /* Hang on to reply otherwise it will be free'd by reset call. */
	  their_reply = send->their_reply ;
	  send->their_reply = NULL ;


	  /* Reset to idle, app then needs to decide whether to wait again. */
	  resetRemoteToIdle(remote_control) ;


	  /* Call the app callback. */
	  (send->process_reply_func)(remote_control, their_reply, send->process_reply_func_data) ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* GtkClipboardGetFunc() called if there is a 'get' request on the clipboard. */
static void sendRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      CALL_OUTOFBAND_HANDLER(remote_control, "get", "clear") ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}

/* 
 *             End of Callbacks for making requests.
 */



/*                Callbacks for receiving requests.
 * 
 * They form a chain linking from one to the other if all goes well. They are all callbacks
 * called either by gtk clipboard/selection handling code or by the application.
 * Note that no direct event handling/processing is required.
 * 
 */


/*
 * Receive Request Step 1)
 * 
 * Peer receives a clear event on its clipboard and then requests the contents
 * of its clipboard which will be the request.
 * Peer's "get request" handler should NOT be called at this stage.
 * 
 */

/* GtkClipboardClearFunc() function called under two circumstances:
 * 
 * 1) We are resetting ourselves perhaps in preparation to making our
 *    own request in which case our state will be REMOTE_STATE_RESET_TO_IDLE
 * 
 * 2) A client signals it wants to make a request by taking ownership
 *    of our clipboard, we get the actual request by doing a get for
 *    the contents of the clipboard. Our state should be REMOTE_STATE_SERVER_WAIT_NEW_REQ
 * 
 *  */
static void receiveWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (remote_control->state == REMOTE_STATE_SERVER_WAIT_NEW_REQ)
	{
	  gtk_clipboard_request_contents(clipboard,
					 remote_control->target_atom,
					 receiveGetRequestClipboardCB, remote_control) ;

	  remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_SEND ;

	  /* Now we are waiting so set timeout. */
	  addTimeout(remote_control) ;
	}
      else
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_WAIT_NEW_REQ,
				"%s",
				"Supposed to be waiting for next request from peer.") ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is in
 * idle state.
 * This is OUT OF BAND if this gets called in normal operation. */
static void receiveWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      CALL_OUTOFBAND_HANDLER(remote_control, "get", "clear") ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/*
 * Receive Request Step 2)
 * 
 * We receive contents of clipboard from our peer which should be a valid request.
 * 
 */

/* A GtkClipboardReceivedFunc() fuction, called with clipboard contents. */
static void receiveGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      receive_request = remote_control->receive ;

      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_SEND)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_SEND,
				"%s",
				"Expected to be waiting for request.") ;
	}
      else
	{
	  char *err_msg = NULL ;
	  char *selection_str ;

	  REMOTELOGMSG(remote_control,
		       "Received request from peer on receive clipboard \"%s\".",
		       receive_request->our_atom_string) ;

	  if (!isClipboardDataValid(selection_data, receive_request->our_atom_string, &err_msg))
	    {
	      CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "%s", err_msg) ;

	      g_free(err_msg) ;
	    }
	  else if (!(selection_str = getClipboardData(selection_data, &err_msg)))
	    {
	      CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "%s", err_msg) ;

	      g_free(err_msg) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Retrieved peers request from clipboard \"%s\", request is:\n \"%s\".",
			   receive_request->our_atom_string, selection_str) ;

	      receive_request->their_request = selection_str ;

	      /* Retake ownership so they know we have got request. */
	      if (!clipboardTakeOwnership(receive_request->our_clipboard,
					  receiveRequestReceivedClipboardGetCB, receiveRequestReceivedClipboardClearCB,
					  remote_control))
		{
		  CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
				   "Tried to retake ownership of receive clipboard \"%s\" to signal peer that we have"
				   " request but this failed, aborting request.",
				   receive_request->our_atom_string) ;
		}
	      else
		{
		  remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_ACK ;

		  REMOTELOGMSG(remote_control,
			       "Have retaken ownership of receive clipboard \"%s\" to signal peer that we have their request.",
			       receive_request->our_atom_string) ;

		  addTimeout(remote_control) ;
		}
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/*
 * Receive Request Step 3)
 * 
 * Server peer receives a clear event on its clipboard which signals that the
 * client peer knows we have the request and is now waiting for the reply.
 * We can now pass the request to the application for processing.
 * Server peer's "get request" handler should NOT be called at this stage.
 * 
 */

/* A GtkClipboardClearFunc() function called when the clipboard is cleared by our peer. */
static void receiveRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_ACK)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_ACK,
				"%s",
				"Expected to be waiting for request.") ;
	}
      else
	{
	  receive_request = remote_control->receive ;

	  REMOTELOGMSG(remote_control, "Calling apps callback func to process request:\n \"%s\"",
		       receive_request->their_request) ;

	  /* set state here as not sure when app will call us back. */
	  remote_control->state = REMOTE_STATE_SERVER_PROCESS_REQ ;

	  /* Call the app callback....um the second remote_control pointer looks wrong here..... */
	  (receive_request->process_request_func)(remote_control, receiveAppCB, remote_control,
						  receive_request->their_request,
						  receive_request->process_request_func_data) ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}

/* A GtkClipboardGetFunc() called if there is a 'get' request on our clipboard. */
static void receiveRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
						 guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  
  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      CALL_OUTOFBAND_HANDLER(remote_control,"get", "clear") ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/*
 * Receive Request Step 4)
 * 
 * A ZMapRemoteControlCallWithReplyFunc() which is called by the application when it has 
 * finished processing the request and is returning its reply. This may happen synchronously
 * or asynchronously.
 * 
 * If abort is TRUE this signals an unrecoverable error (perhaps the request
 * was in the wrong format completely) and the transaction must be aborted.
 */
static void receiveAppCB(void *remote_data, gboolean abort, char *reply)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)remote_data ;
  RemoteReceive receive ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


  /* STICK IN ABORT STUFF HERE....NEED TO CALL ERROR HANDLER.... */

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (remote_control->state != REMOTE_STATE_SERVER_PROCESS_REQ)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_PROCESS_REQ,
				"%s",
				"Expected to be waiting for request.") ;
	}
      else if (abort)
	{
	  CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_APP_ABORT,
			   "%s",
			   "App has aborted transaction, see log file. \"%s\".") ;
	}
      else
	{
	  receive = remote_control->receive ;

	  REMOTELOGMSG(remote_control, "App processing finished, reply is: \"%s\".", reply) ;

	  receive->our_reply = reply ;

	  if (gtk_clipboard_set_with_data(receive->our_clipboard,
					  clipboard_target_G,
					  TARGET_NUM,
					  receiveClipboardGetReplyCB,
					  receiveClipboardClearWaitAfterReplyCB,
					  remote_control))
	    {
	      REMOTELOGMSG(remote_control,
			   "Have ownership of peer clipboard \"%s\", waiting for peer to ask for our reply.",
			   receive->our_atom_string) ;

	      remote_control->state = REMOTE_STATE_SERVER_WAIT_GET ;

	      addTimeout(remote_control) ;
	    }
	  else
	    {
	      CALL_ERR_HANDLER(remote_control, ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "Cannot set request on peer clipboard \"%s\".",
			       receive->our_atom_string) ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/*
 * Receive Request Step 5)
 * 
 * The client peer is asking us for our reply so we return it on the clipboard.
 * Our clear callback should NOT be called at this stage.
 * 
 */

/* GtkClipboardGetFunc() called when peer wants clipboard contents. */
static void receiveClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive ;
  GdkAtom target_atom ;
  char *target_name ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      if (remote_control->state != REMOTE_STATE_SERVER_WAIT_GET)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_WAIT_GET,
				"%s",
				"Expected to be waiting for request.") ;
	}
      else
	{
	  receive = remote_control->receive ;

	  target_atom = gtk_selection_data_get_target(selection_data) ;
	  target_name = gdk_atom_name(target_atom) ;

	  if (info != TARGET_ID)
	    {
	      CALL_ERR_HANDLER(remote_control,
			       ZMAP_REMOTECONTROL_RC_BAD_CLIPBOARD,
			       "Unsupported Data Type requested: %s.", target_name) ;
	    }
	  else
	    {
	      /* All ok so set our request on the peers clipboard. */
	      REMOTELOGMSG(remote_control, "Setting our reply on clipboard \"%s\" using property \"%s\","
			   "reply is:\n \"%s\".",
			   receive->our_atom_string,
			   target_name, receive->our_reply) ;

	      /* Set the data in the selection data param, N.B. terminating NULL is not sent. */
	      gtk_selection_data_set(selection_data,
				     remote_control->target_atom,
				     8,
				     (guchar *)(receive->our_reply),
				     (int)(strlen(receive->our_reply))) ;

	      remote_control->state = REMOTE_STATE_SERVER_WAIT_REPLY_ACK ;

	      addTimeout(remote_control) ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/*
 * Receive Request Step 6)
 * 
 * The client peer has taken ownership of our clipboard to signal it has the reply,
 * we now retake ownership of our clipboard to acknowledge this and terminate
 * the transaction.
 * 
 */

/* GtkClipboardClearFunc() function called when a peer takes ownership of our clipboard. */
static void receiveClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      if (haveTimeout(remote_control))
	removeTimeout(remote_control) ;

      if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REPLY_ACK)
	{
	  CALL_BADSTATE_HANDLER(remote_control, REMOTE_STATE_SERVER_WAIT_REPLY_ACK,
				"%s",
				"Expected to be waiting for acknowledgement from peer that is has our reply.") ;
	}
      else
	{
	  receive = remote_control->receive ;

	  REMOTELOGMSG(remote_control,
		       "Lost ownership of receive clipboard \"%s\", now we know they have our reply.",
		       receive->our_atom_string) ;

	  REMOTELOGMSG(remote_control,
		       "Clearing our clipboard to signal peer we know they got reply on \"%s\").",
		       receive->our_atom_string) ;

	  gtk_clipboard_clear(receive->our_clipboard) ;


	  REMOTELOGMSG(remote_control,
		       "Resetting to idle as request finished on \"%s\").",
		       receive->our_atom_string) ;

	  /* Reset to idle. */
	  resetRemoteToIdle(remote_control) ;

	  /* Optionally call back to app to signal that peer has received reply, enables app
	   * to go back to waiting for a reply or make its own request. */
	  if (receive->reply_sent_func)
	    {
	      REMOTELOGMSG(remote_control,
			   "%s", "About to call Apps replySentCB.") ;
		  
	      (receive->reply_sent_func)(receive->reply_sent_func_data) ;
		  
	      REMOTELOGMSG(remote_control,
			   "%s", "Back from Apps replySentCB.") ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}
/* 
 *               End of callbacks for receiving requests.
 */ 



/* 
 *               Utility functions.
 */


/* Reset the state of the Remote Control object to idle, freeing all resources. */
static void resetRemoteToIdle(ZMapRemoteControl remote_control)
{
  RemoteControlState curr_state ;

  /* Record current state, needed to decide if we need to clear our clipboard to prevent
   * further requests being received. */
  curr_state = remote_control->state ;

  /* Need to set this state so we when we clear the clipboard we don't try and process
   * the event in our clear clipboard handler routine thinking it's a genuine request. */
  remote_control->state = REMOTE_STATE_IDLE ;


  /* UM....DON'T LIKE THIS REALLY....CHECK WHY WE DO THIS.... */
  if (curr_state == REMOTE_STATE_SERVER_WAIT_NEW_REQ)
    gtk_clipboard_clear(remote_control->receive->our_clipboard) ;


  if (haveTimeout(remote_control))
    removeTimeout(remote_control) ;


  /* Reset the various states: idle, receiving or sending and set current state to idle. */
  resetIdle(remote_control->idle) ;

  if (remote_control->receive)
    resetReceive(remote_control->receive) ;

  if (remote_control->send)
    resetSend(remote_control->send) ;

  remote_control->curr_remote = (RemoteAny)(remote_control->idle) ;

  return ;
}


/* Create idle struct. */
static RemoteIdle createIdle(void)
{
  RemoteIdle idle ;

  idle = g_new0(RemoteIdleStruct, 1) ;
  idle->remote_type = REMOTE_TYPE_IDLE ;

  return idle ;
}


/* Create receive interface struct. */
static RemoteReceive createReceive(GQuark app_id, char *app_unique_str,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer process_request_func_data,
				   ZMapRemoteControlReplySentFunc reply_sent_func,
				   gpointer reply_sent_func_data)
{
  RemoteReceive receive_request ;

  receive_request = g_new0(RemoteReceiveStruct, 1) ;
  receive_request->remote_type = REMOTE_TYPE_SERVER ;

  receive_request->our_app_name = app_id ;

  /* Set up the request atom, this atom will hold the request/replies from a client process. */
  receive_request->our_unique_str = g_quark_from_string(app_unique_str) ;

  receive_request->our_atom = gdk_atom_intern(app_unique_str, FALSE) ;

  receive_request->our_atom_string = gdk_atom_name(receive_request->our_atom) ;

  receive_request->our_clipboard = gtk_clipboard_get(receive_request->our_atom) ;

  receive_request->process_request_func = process_request_func ;
  receive_request->process_request_func_data = process_request_func_data ;

  receive_request->reply_sent_func = reply_sent_func ;
  receive_request->reply_sent_func_data = reply_sent_func_data ;

  return receive_request ;
}


/* Create send interface struct. */
static RemoteSend createSend(char *send_app_name, char *send_unique_atom_string,
			     ZMapRemoteControlRequestSentFunc req_sent_func, gpointer req_sent_func_data,
			     ZMapRemoteControlReplyHandlerFunc process_reply_func, gpointer process_reply_func_data)
{
  RemoteSend send_request ;

  send_request = g_new0(RemoteSendStruct, 1) ;
  send_request->remote_type = REMOTE_TYPE_CLIENT ;

  send_request->their_app_name = g_quark_from_string(send_app_name) ;

  send_request->their_unique_str = g_quark_from_string(send_unique_atom_string) ;

  send_request->their_atom = gdk_atom_intern(send_unique_atom_string, FALSE) ;

  send_request->their_atom_string = gdk_atom_name(send_request->their_atom) ;

  send_request->their_clipboard = gtk_clipboard_get(send_request->their_atom) ;

  send_request->process_reply_func = process_reply_func ;
  send_request->process_reply_func_data = process_reply_func_data ;

  send_request->req_sent_func = req_sent_func ;
  send_request->req_sent_func_data = req_sent_func_data ;

  return send_request ;
}


/* Reset idle record. */
static void resetIdle(RemoteIdle idle_request)
{

  return ;
}


/* Reset receive record. */
static void resetReceive(RemoteReceive receive_request)
{
  if (receive_request->their_request)
    {
      g_free(receive_request->their_request) ;

      receive_request->their_request = NULL ;
    }

  return ;
}


/* Reset send struct. */
static void resetSend(RemoteSend send_request)
{
  if (send_request->our_request)
    {
      g_free(send_request->our_request) ;

      send_request->our_request = NULL ;
    }

  return ;
}

/* Destroy idle record. */
static void destroyIdle(RemoteIdle idle_request)
{
  g_free(idle_request) ;

  return ;
}

/* Destroy receive record. */
static void destroyReceive(RemoteReceive receive_request)
{
  if (receive_request->their_request)
    g_free(receive_request->their_request) ;

  g_free(receive_request) ;

  return ;
}


/* Destroy send record. */
static void destroySend(RemoteSend send_request)
{
  if (send_request->our_request)
    g_free(send_request->our_request) ;

  g_free(send_request) ;

  return ;
}




/* The send/receive code works asynchronously and if there are errors, state needs to be
 * reset so that the send or receive is aborted. */
static void errorHandler(ZMapRemoteControl remote_control,
			 const char *file_name, const char *func_name,
			 ZMapRemoteControlRCType error_rc, char *format_str, ...)
{
  va_list args ;

  /* pass error_rc to log handler ???? should do really.... */

  /* Log the error, must be done first so current state etc. is recorded in log message. */
  va_start(args, format_str) ;

  logMsg(remote_control, file_name, func_name, format_str, args) ;

  va_end(args) ;


  /* Reset remote control to idle before calling user error handler, user may want to go
   * back to waiting. */
  REMOTELOGMSG(remote_control, "Resetting to %s state after error.", remoteState2ExactStr(REMOTE_TYPE_IDLE)) ;

  resetRemoteToIdle(remote_control) ;


  REMOTELOGMSG(remote_control, "%s", "About to call apps ZMapRemoteControlErrorHandlerFunc().") ;

  /* bother we need the error string from above to pass to the app..... */
  /* we need to call the apps error handler func here..... */
  (remote_control->app_error_func)(remote_control,
				   error_rc, "dummy",
				   remote_control->app_error_func_data) ;

  REMOTELOGMSG(remote_control, "%s", "Back from apps ZMapRemoteControlErrorHandlerFunc().") ;


  return ;
}


/* Only to be called using REMOTELOGMSG(), do not call directly. Outputs a log message
 * in a standard format.
 *  */
static void errorMsg(ZMapRemoteControl remote_control,
		     const char *file_name, const char *func_name, char *format_str, ...)
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
		   const char *file_name, const char *func_name, char *format_str, va_list args)
{				
  GString *msg ;
  char *name ;
  gboolean add_filename = FALSE ;			    /* no point currently. */

  name = g_path_get_basename(file_name) ;

  msg = g_string_sized_new(500) ;				

  g_string_append_printf(msg, "%s:%s%s%s()\tActing as: %s  State: %s\tMessage: ",			
			 g_quark_to_string(remote_control->app_id),
			 (add_filename ? name : ""),
			 (add_filename ? ":" : ""),
			 func_name,
			 remoteType2ExactStr(remote_control->curr_remote->remote_type),
			 remoteState2ExactStr(remote_control->state)) ;

  g_string_append_vprintf(msg, format_str, args) ;

  (remote_control->app_err_report_func)(remote_control->app_err_report_data, msg->str) ;			
									
  g_string_free(msg, TRUE) ;					

  g_free(name) ;

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



/* 
 * The timeout functions, we need a timeout any time we are waiting for the peer to respond
 * otherwise we may deadlock.
 */

static void addTimeout(ZMapRemoteControl remote_control)
{
  zMapAssert(!(remote_control->timer_source_id)) ;

  if ((remote_control->timeout_ms))
    remote_control->timer_source_id = g_timeout_add(remote_control->timeout_ms, timeOutCB, remote_control) ;

  return ;
}

static gboolean haveTimeout(ZMapRemoteControl remote_control)
{
  gboolean have_timeout = FALSE ;

  if (remote_control->timer_source_id)
    have_timeout = TRUE ;

  return have_timeout ;
}

static void removeTimeout(ZMapRemoteControl remote_control)
{
  zMapAssert(remote_control->timer_source_id) ;

  if ((remote_control->timeout_ms))
    gtk_timeout_remove(remote_control->timer_source_id) ;

  remote_control->timer_source_id = 0 ;

  return ;
}

/* A GSourceFunc() for time outs.
 * 
 * Note that this function returns FALSE which automatically removes us from gtk so no need
 * to call removeTimeout().
 *  */
static gboolean timeOutCB(gpointer user_data)
{
  gboolean result = FALSE ;				    /* Returning FALSE removes timer. */
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  CALL_ERR_HANDLER(remote_control,
		   ZMAP_REMOTECONTROL_RC_TIMED_OUT,
		   "Timed out waiting for peer to reply while in state \"%s\".",
		   remoteState2ExactStr(remote_control->state)) ;

  /* This callback is removed automatically by gtk so reset the source_id. */
  remote_control->timer_source_id = 0 ;

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}




gboolean isInReceiveState(RemoteControlState state)
{
  gboolean result = FALSE ;

  switch (state)
    {
    case REMOTE_STATE_CLIENT_WAIT_GET:
    case REMOTE_STATE_CLIENT_WAIT_REQ_ACK:
    case REMOTE_STATE_CLIENT_WAIT_REPLY:
    case REMOTE_STATE_CLIENT_WAIT_SEND:
    case REMOTE_STATE_CLIENT_WAIT_REPLY_ACK:
      result = TRUE ;
      break ;
    default:
      result = FALSE ;
      break ;
    }

  return result ;
}


gboolean isInSendState(RemoteControlState state)
{
  gboolean result = FALSE ;

  switch (state)
    {
    case REMOTE_STATE_SERVER_WAIT_NEW_REQ:
    case REMOTE_STATE_SERVER_WAIT_REQ_SEND:
    case REMOTE_STATE_SERVER_WAIT_REQ_ACK:
    case REMOTE_STATE_SERVER_PROCESS_REQ:
    case REMOTE_STATE_SERVER_WAIT_GET:
    case REMOTE_STATE_SERVER_WAIT_REPLY_ACK:
      result = TRUE ;
      break ;
    default:
      result = FALSE ;
      break ;
    }

  return result ;
}


/* Checks to see that the data specified in selection_data is a format that we support
 * and that there is data.
 * Returns TRUE if data format is correct otherwise returns FALSE and sets err_msg_out
 * to explain problem. */
static gboolean isClipboardDataValid(GtkSelectionData *selection_data, char *curr_selection, char **err_msg_out)
{
  gboolean result = TRUE ;

  if (result)
    {
      /* Check the format descriptors. */
      char *selection, *data_type, *target ;

      selection = gdk_atom_name(gtk_selection_data_get_selection(selection_data)) ;
      data_type = gdk_atom_name(gtk_selection_data_get_data_type(selection_data)) ;
      target = gdk_atom_name(gtk_selection_data_get_target(selection_data)) ;

      if (strcmp(selection, curr_selection) !=0
	  || strcmp(data_type, ZACP_DATA_TYPE) != 0
	  || strcmp(target, ZACP_DATA_TYPE) != 0)
	{
	  *err_msg_out = g_strdup_printf("Wrong selection/target on clipboard,"
					 " Clipboard details: selection = \"%s\", data_type = \"%s\", target = \"%s\""
					 " Expected details: selection = \"%s\", data_type = \"%s\", target = \"%s\"",
					 selection, data_type, target,
					 curr_selection, ZACP_DATA_TYPE , ZACP_DATA_TYPE) ;

	  result = FALSE ;
	}

      g_free(selection) ;
      g_free(data_type) ;
      g_free(target) ;
    }

  if (result)
    {
      /* Check the memory format for the data. */
      int format ;

      format = gtk_selection_data_get_format(selection_data) ;

      if (format != ZACP_DATA_FORMAT)
	{
	  *err_msg_out = g_strdup_printf("Wrong data format on clipboard,"
					 "expected %d bits/byte but got %d.",
					 ZACP_DATA_FORMAT, format) ;

	  result = FALSE ;
	}
    }

  if (result)
    {
      /* Check there is some data ! */
      int length ;

      length = gtk_selection_data_get_length(selection_data) ;

      if (length < 0)
	{
	  *err_msg_out = g_strdup_printf("Could not retrieve peer data from clipboard, length was \"%d\".",
					 length) ;

	  result = FALSE ;
	}
    }				 

  return result ;
}


/* 
 * All requests are strings but they do _NOT_ have a null terminator as they must
 * be language independent. This function checks to see that the string is non-NULL
 * and then returns a valid C string made from the clipboard contents, the string
 * should be g_free()'d when finished with.
 * 
 * Note, function expects format etc. of selection to already have been checked using
 * isClipboardDataValid().
 * 
 * Returns NULL and an error message in err_msg_out if there was a problem.
 */
static char *getClipboardData(GtkSelectionData *selection_data, char **err_msg_out)
{
  char *clipboard_str = NULL ;
  int length ;
  char *data_ptr ;

  length = gtk_selection_data_get_length(selection_data) ;
  data_ptr = (char *)gtk_selection_data_get_data(selection_data) ;

  if (!(*data_ptr))
    {
      *err_msg_out = g_strdup_printf("First char in clipboard data is NULL, data length was \"%d\".", length) ;

      clipboard_str = NULL ;
    }
  else
    {
      clipboard_str = g_strndup(data_ptr, length) ;
    }

  return clipboard_str ;
}





