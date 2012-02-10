/*  Last edited: Dec 16 11:12 2011 (edgrif) */
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
#include <gdk/gdkx.h>					    /* For X Windows stuff. */
#include <gtk/gtk.h>

#include <ZMap/zmapUtils.h>
#include <zmapRemoteControl_P.h>



/* Text for debug/log messages.... */

#define PEER_PREFIX   "PEER"
#define CLIENT_PREFIX "CLIENT"
#define SERVER_PREFIX "SERVER"

#define ENTER_TXT "Enter >>>>"
#define EXIT_TXT  "Exit  <<<<"



/* 
 * Use these macros for _ALL_ logging calls in this code to ensure that they go to the right place.
 */
#define REMOTELOGMSG(REMOTE_CONTROL, FORMAT_STR, ...)			\
  do									\
    {									\
      debugMsg((REMOTE_CONTROL),					\
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





static gboolean clipboardTakeOwnership(GtkClipboard *clipboard,
				       GtkClipboardGetFunc get_func, GtkClipboardClearFunc clear_func,
				       gpointer user_data) ;


static void receiveWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
			       guint info, gpointer user_data) ;
static void receiveWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void receiveRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				  guint info, gpointer user_data) ;
static void receiveRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void receiveGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer data) ;
static void receiveAppCB(void *remote_data, char *reply) ;
static void receiveClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				    guint info, gpointer user_data) ;
static void receiveClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data) ;


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
static void sendAppCB(void *remote_data) ;

static gboolean timeOutCB(gpointer user_data) ;

static void debugMsg(ZMapRemoteControl remote_control, const char *func_name, char *format_str, ...) ;
static gboolean stderrOutputCB(gpointer user_data, char *err_msg) ;

static RemoteIdle createIdle(void) ;
static RemoteReceive createReceive(GQuark app_id, char *app_str,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer process_request_func_data) ;
static RemoteSend createSend(char *app_name, char *send_unique_atom_string,
			     ZMapRemoteControlReplyHandlerFunc process_reply_func, gpointer process_reply_func_data) ;
static void resetClipboard(ZMapRemoteControl remote_control) ;
static void resetRemoteToIdle(ZMapRemoteControl remote_control) ;
static void resetIdle(RemoteIdle idle_request) ;
static void resetReceive(RemoteReceive receive_request) ;
static void resetSend(RemoteSend send_request) ;
static gboolean resetSendToRequestWait(ZMapRemoteControl remote_control) ;
static void destroyIdle(RemoteIdle idle_request) ;
static void destroyReceive(RemoteReceive client_request) ;
static void destroySend(RemoteSend server_request) ;

static void disconnectTimerHandler(guint *func_id) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean validateRequest(char *request, char **err_msg_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean isValidClipboardType(guint info, ZMapRemoteControl remote_control) ;

static void logBadState(ZMapRemoteControl remote_control,
			RemoteControlState expected_state, char *format, ...) ;
static void logOutOfBandReq(ZMapRemoteControl remote_control,
			    char *format, ...) ;

gboolean isInReceiveState(RemoteControlState state) ;
gboolean isInSendState(RemoteControlState state) ;


/* 
 *        Globals.
 */

/* Used for validation of ZMapRemoteControlStruct. */
ZMAP_MAGIC_NEW(remote_control_magic_G, ZMapRemoteControlStruct) ;


/* Debugging stuff... */
static gboolean remote_debug_G = TRUE ;


/* We should be exporting these in a protocol header..... */
/* Data types we support for the properties....only text ! */
#define TARGET_ID 111111
#define TARGET_NUM 1
static GtkTargetEntry clipboard_target_G[TARGET_NUM] = {{ZACP_DATA_TYPE, 0, TARGET_ID}} ;



/* 
 *                     External interface
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
 * At the end of this function the caller will have a remote control object ready to receive
 * requests.
 * 
 * To make the requests the caller must know the peer's unique id and have called remote_control
 * object to set it up and make then make the request.
 * 
 */
ZMapRemoteControl zMapRemoteControlCreate(char *app_id,
					  ZMapRemoteControlReportRequestSentEndFunc req_sent_func,
					  gpointer req_sent_func_data,
					  ZMapRemoteControlReportRequestReceivedEndFunc req_received_func,
					  gpointer req_received_func_data,
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_func_data,
					  ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data)
{
  ZMapRemoteControl remote_control = NULL ;

  if (app_id && *app_id)
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


      if (req_sent_func)
	{
	  remote_control->req_sent_func = req_sent_func ;
	  remote_control->req_sent_func_data = req_sent_func_data ;
	}

      if (req_received_func)
	{
	  remote_control->req_received_func = req_received_func ;
	  remote_control->req_received_func_data = req_received_func_data ;
	}

      if (error_func)
	{
	  remote_control->error_func = error_func ;
	  remote_control->error_func_data = error_func_data ;
	}


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
				      gpointer process_request_func_data)
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
					      process_request_func, process_request_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Receive interface initialised (receive clipboard is \"%s\").",
		   remote_control->receive->our_atom_string) ;
      
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
      logBadState(remote_control, REMOTE_STATE_IDLE,
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
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      result = resetSendToRequestWait(remote_control) ;
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
					process_reply_func, process_reply_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Send interface initialised (send clipboard is \"%s\").",
		   remote_control->send->their_atom_string) ;

      result = TRUE ;
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
	  logBadState(remote_control, REMOTE_STATE_IDLE,
		      "Must be in idle state to make requests, command was:\"%s\".",
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

      /* If waiting for new request from some client then reset to idle before making request. */
      if (remote_control->state != REMOTE_STATE_IDLE)
	{
	  resetClipboard(remote_control) ;

	  resetRemoteToIdle(remote_control) ;
	}

      /* Try sending a request to our send. */
      send = remote_control->send ;
      send->our_request = g_strdup(request) ;

      /* Swop to client state to make request. */
      remote_control->state = REMOTE_STATE_CLIENT_WAIT_GET ;
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

	  if (send->timeout_ms)
	    remote_control->timer_source_id = g_timeout_add(send->timeout_ms,
							    timeOutCB, remote_control) ;

	  result = TRUE ;
	}
      else
	{
    	  REMOTELOGMSG(remote_control,
		       "Cannot set request on peer clipboard \"%s\".",
		       send->their_atom_string) ;


	  /* Need to reset state here.... */

	  result = FALSE ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}



/* Will reset the remote interface to REMOTE_STATE_INACTIVE,
 * 
 * Consider following options:
 * 
 * hard_reset == TRUE this will 
 * be done regardless of the current state and without trying to issue a warning, otherwise
 * if  */
gboolean zMapRemoteControlReset(ZMapRemoteControl remote_control)
{
  gboolean result = TRUE ;

  resetClipboard(remote_control) ;

  resetRemoteToIdle(remote_control) ;

  return result ;
 }




/* Set debug on/off. */
gboolean zMapRemoteControlSetDebug(ZMapRemoteControl remote_control, gboolean debug_on)
{
  gboolean result = TRUE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  remote_debug_G = debug_on ;

  return result ;
 }



/* Set timeout, if timeout_secs == 0 then no timeout. */
gboolean zMapRemoteControlSetTimeout(ZMapRemoteControl remote_control, int timeout_secs)
{
  gboolean result = TRUE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* allow separate timeouts for receive and peer ???? need to be able to set them separately... */

  if (timeout_secs <= 0)
    remote_control->timeout_ms = 0 ;
  else
    remote_control->timeout_ms = timeout_secs * 1000 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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

  remote_control->err_report_func = err_report_func ;
  remote_control->err_report_data = err_report_data ;

  return result ;
 }


/* Return to default error message handler. */
gboolean zMapRemoteControlUnSetErrorCB(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  remote_control->err_report_func = stderrOutputCB ;
  remote_control->err_report_data = NULL ;

  return result ;
 }



/* Free all resources and destroy the remote control block. */
void zMapRemoteControlDestroy(ZMapRemoteControl remote_control)
{
  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->idle)
    destroyIdle(remote_control->idle) ;
  if (remote_control->receive)
    destroyReceive(remote_control->receive) ;
  if (remote_control->send)
    destroySend(remote_control->send) ;


  REMOTELOGMSG(remote_control, "%s", "Destroyed RemoteControl object.") ;

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;		    /* Latest we can make this call as we 
							       need remote_control. */
  ZMAP_MAGIC_RESET(remote_control->magic) ;		    /* Before free() reset magic to invalidate memory. */

  g_free(remote_control) ;

  return ;
}





/* 
 *                    Package routines.
 */

/* Not great....need access to a unique id, this should be a "friend" function
 * in C++ terms....not for general consumption. */
char *zmapRemoteControlMakeReqID(ZMapRemoteControl remote_control)
{
  char *id = NULL ;
  
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





/*                Callbacks for receiving requests.             */


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

  if (remote_control->state == REMOTE_STATE_RESETTING_TO_IDLE)
    {
      DEBUGLOGMSG(remote_control, "%s", "Resetting our state to idle so no action taken.") ;
    }
  else if (remote_control->state == REMOTE_STATE_SERVER_WAIT_NEW_REQ)
    {
      gtk_clipboard_request_contents(clipboard,
				     remote_control->target_atom,
				     receiveGetRequestClipboardCB, remote_control) ;

      remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_SEND ;
    }
  else
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_NEW_REQ,
		  "%s"
		  "Received signal from client that they have new request to make.") ;
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is in
 * idle state, this should not happen in normal operation. */
static void receiveWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" while waiting for \"clear\" for %s, ignoring \"get\".",
		   remote_control->receive->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


static void receiveGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  receive_request = remote_control->receive ;


  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_SEND,
		  "%s"
		  "Expected to be waiting for request.") ;
    }
  else
    {

      REMOTELOGMSG(remote_control,
		   "Received request from peer on receive clipboard \"%s\".",
		   receive_request->our_atom_string) ;

      /* Check request is ok. */
      if (selection_data->length < 0)
	{
	  REMOTELOGMSG(remote_control,
		       "Could not retrieve peer data from clipboard \"%s\".",
		       receive_request->our_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Retrieved peers request from clipboard \"%s\", request is:\n \"%s\".",
		       receive_request->our_atom_string, selection_data->data) ;

	  receive_request->their_request = g_strdup((char *)(selection_data->data)) ;


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_ACK ;


	  /* Retake ownership so they know we have got request. */
	  if (!clipboardTakeOwnership(receive_request->our_clipboard,
				      receiveRequestReceivedClipboardGetCB, receiveRequestReceivedClipboardClearCB,
				      remote_control))
	    {
	      REMOTELOGMSG(remote_control,
			   "Tried to retake ownership of receive clipboard \"%s\" to signal peer that we have"
			   " request but this failed, aborting request.",
			   receive_request->our_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Have retaken ownership of receive clipboard \"%s\" to signal peer that we have their request.",
			   receive_request->our_atom_string) ;

	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is waiting
 * for the peer to retake ownership of the our clipboard as a signal that we can begin
 * processing the request. */
static void receiveRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  
  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;


  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      /* We should probably try to put more info about the request here. */

      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" while waiting for \"clear\" for %s, ignoring \"get\".",
		   remote_control->receive->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* A GtkClipboardClearFunc() function called when the clipboard is cleared by our peer, now we can
 * go ahead and service the request. */
static void receiveRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_ACK,
		  "%"
		  "Expected to be waiting for request.") ;
    }
  else
    {
      receive_request = remote_control->receive ;

      REMOTELOGMSG(remote_control, "Calling apps callback func to process request:\n \"%s\"",
		   receive_request->their_request) ;

      /* set state here as not sure when app will call us back ?? */
      remote_control->state = REMOTE_STATE_SERVER_PROCESS_REQ ;

      /* Call the app callback....um the second remote_control pointer looks wrong here..... */
      (receive_request->process_request_func)(remote_control, receiveAppCB, remote_control,
					      receive_request->their_request,
					      receive_request->process_request_func_data) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* A ZMapRemoteControlCallWithReplyFunc() which is called by the application when it has 
 * finished processing the request. */
static void receiveAppCB(void *remote_data, char *reply)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)remote_data ;
  RemoteReceive receive ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


  if (remote_control->state != REMOTE_STATE_SERVER_PROCESS_REQ)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_PROCESS_REQ,
		  "%s"
		  "Expected to be waiting for request.") ;
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  remote_control->state = CLIENT_STATE_SENDING ;


	  if (send->timeout_ms)
	    remote_control->timer_source_id = g_timeout_add(send->timeout_ms,
							    timeOutCB, remote_control) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_GET ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Cannot set request on peer clipboard \"%s\".",
		       receive->our_atom_string) ;
	}
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* GtkClipboardGetFunc() called when peer wants our reply. */
static void receiveClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					 guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive ;
  GdkAtom target_atom ;
  char *target_name ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_GET)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_GET,
		  "%s"
		  "Expected to be waiting for request.") ;
    }
  else
    {
      receive = remote_control->receive ;

      target_atom = gtk_selection_data_get_target(selection_data) ;
      target_name = gdk_atom_name(target_atom) ;

      if (info != TARGET_ID)
	{
	  REMOTELOGMSG(remote_control, "Unsupported Data Type requested: %s.", target_name) ;
	}
      else
	{
	  /* All ok so set our request on the peers clipboard. */
	  REMOTELOGMSG(remote_control, "Setting our reply on clipboard \"%s\" using property \"%s\","
		       "reply is:\n \"%s\".",
		       receive->our_atom_string,
		       target_name, receive->our_reply) ;

	  /* Set the data in the selection data param. */
	  gtk_selection_data_set(selection_data,
				 remote_control->target_atom,
				 8,
				 (guchar *)(receive->our_reply),
				 (int)(strlen(receive->our_reply) + 1)) ;

	  remote_control->state = REMOTE_STATE_SERVER_WAIT_REPLY_ACK ;
	}
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* GtkClipboardClearFunc() function called when a client signals it wants to make a request
 * by taking ownership of our clipboard, we get the request by doing a get for
 * the contents of the clipboard.
 * 
 * (N.B. MAY BE CALLED IF CLIPBOARD IS CLEARED...HOW WOULD WE KNOW THIS ??
 *  SORT THIS OUT !!!)
 * 
 * 
 * 
 *  */
static void receiveClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteReceive receive ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REPLY_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_REPLY_ACK,
		  "%s"
		  "Expected to be waiting for request.") ;
    }
  else
    {
      receive = remote_control->receive ;

      REMOTELOGMSG(remote_control,
		   "Lost ownership of receive clipboard \"%s\", about to retake it to signal we know they have our reply.",
		   receive->our_atom_string) ;

      /* Back we go to the beginning..... */
      if (clipboardTakeOwnership(receive->our_clipboard,
				 receiveWaitClipboardGetCB, receiveWaitClipboardClearCB,
				 remote_control))
	{
	  REMOTELOGMSG(remote_control,
		       "Receive interface initialised (now have ownership of our clipboard \"%s\").",
		       receive->our_atom_string) ;

	  remote_control->state = REMOTE_STATE_SERVER_WAIT_NEW_REQ ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Receive init failed, cannot become owner of own clipboard \"%s\".",
		       receive->our_atom_string) ;
	}


      /* Optionally call back to app to say we have definitively finished with peers request/reply. */
      if (remote_control->req_received_func)
	(remote_control->req_received_func)(remote_control->req_received_func_data) ;

    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/* 
 *             Callbacks for making requests.
 * 
 * They form a chain linking from one to the other if all goes well. They are all callbacks
 * called by gtk clipboard/selection handling code, no direct event handling/processing
 * is required.
 * 
 */

/* We have ownership of our peers clipboard and this function is called when they ask us for our
 * request. */
static void sendClipboardSendRequestCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;
  GdkAtom target_atom ;
  char *target_name ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_GET)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  send->our_request) ;
    }
  else
    {
      target_atom = gtk_selection_data_get_target(selection_data) ;
      target_name = gdk_atom_name(target_atom) ;

      if (info != TARGET_ID)
	{
	  REMOTELOGMSG(remote_control, "Unsupported Data Type requested: %s.", target_name) ;
	}
      else
	{
	  /* All ok so set our request on the peers clipboard. */

	  REMOTELOGMSG(remote_control, "Setting our request on clipboard \"%s\" using property \"%s\","
		       "request is:\n \"%s\".",
		       send->their_atom_string,
		       target_name, send->our_request) ;

	  /* Set the data in the selection data param. */
	  gtk_selection_data_set(selection_data,
				 remote_control->target_atom,
				 8,
				 (guchar *)(send->our_request),
				 (int)(strlen(send->our_request) + 1)) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REQ_ACK ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* Called when peer has received our request and retakes ownership of it's clipboard
 * to signal it has the request. */
static void sendClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REQ_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
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
	  REMOTELOGMSG(remote_control,
		       "Failed to retake ownership of their clipboard \"%s\", aborting request.",
		       send->their_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Have retaken ownership of their clipboard \"%s\" to signal they can process the request.",
		       send->their_atom_string) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY ;
	}

    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/* GtkClipboardGetFunc() called if someone does a 'get' on the peers clipboard while we
 * are waiting for a clear, shouldn't normally happen. */
static void sendReplyWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


  send = remote_control->send ;


  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      /* We should probably try to put more info about the request here. */

      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" for %s, ignoring request and resetting state to %s.",
		   send->their_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* GtkClipboardClearFunc() function called when a peer signals it has finished request
 * and wants to return its reply.
 * 
 *  */
static void sendReplyWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
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
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* GtkClipboardReceivedFunc() function called when peer sends us it's reply. */
static void sendGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  send->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Received reply from peer on receive clipboard \"%s\".",
		   send->their_atom_string) ;


      /* All ok so record the reply, we'll process it when we get a clear from the peer. */
      if (selection_data->length < 0)
	{
	  REMOTELOGMSG(remote_control,
		       "Could not retrieve peer data from clipboard \"%s\".",
		       send->their_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Retrieved peers reply from clipboard \"%s\", reply is:\n \"%s\".",
		       send->their_atom_string, selection_data->data) ;

	  send->their_reply = g_strdup((char *)(selection_data->data)) ;


	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY_ACK ;

	  /* STATE NEEDS SORTING OUT ON ERROR.... */
	  /* Retake ownership so they know we have got reply. */
	  if (!clipboardTakeOwnership(send->their_clipboard,
				      sendRequestReceivedClipboardGetCB, sendRequestReceivedClipboardClearCB,
				      remote_control))
	    {
	      REMOTELOGMSG(remote_control,
			   "Tried to retake ownership of receive clipboard \"%s\" to signal peer that we have"
			   " request but this failed, aborting request.",
			   send->their_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Have retaken ownership of receive clipboard \"%s\" to signal peer that we have their reply.",
			   send->their_atom_string) ;

	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}






/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is waiting
 * for the receive to retake ownership of their clipboard, this shouldn't happen in normal
 * processing. */
static void sendRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;


  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      /* We should probably try to put more info about the request here. */

      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" for %s, ignoring request and resetting state to %s.",
		   remote_control->receive->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* A GtkClipboardClearFunc() function called when the clipboard is cleared by peer to signal they
 * know we have the reply. */
static void sendRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSend send ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  send = remote_control->send ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  send->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Lost ownership of peer clipboard \"%s\", they now know we have the reply.",
		   send->their_atom_string) ;

      REMOTELOGMSG(remote_control, "About to call apps callback to process the reply:\n \"%s\"", send->their_reply) ;

      /* Call the app callback. */
      (send->process_reply_func)(remote_control, sendAppCB, remote_control, send->their_reply,
				 send->process_reply_func_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* THIS NEEDS SORTING OUT IN ALL ROUTINES....FOR NOW WE JUST RETURN TO LISTENING... */

      /* Oh gosh...slightly tricky here...app may decide to set our state depending on
       * whether is suceeded or not....for now we check to see if our state has been
       * changed by app and if not we return to waiting for a request. */
      if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY_ACK)
	{
	  REMOTELOGMSG(remote_control, "%s", "App has set our state so we do not do anything.") ;
	}
      else
	{
	  REMOTELOGMSG(remote_control, "%s", "App has not set our state so reset to idle.") ;

	  resetRemoteToIdle(remote_control) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* ok....this reset stuff is not in the right place...it should be in the callback that the
	 app must call..... */


      /* Reset first then set to wait...clears old request stuff...or should do... */
      resetRemoteToIdle(remote_control) ;

      if (!resetSendToRequestWait(remote_control))
	{
	  REMOTELOGMSG(remote_control, "%s", "Could not reset to wait for next request, resetting to idle.") ;

	  resetRemoteToIdle(remote_control) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* Called by the application when it has finished processing the reply it received from the peer. */
static void sendAppCB(void *remote_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)remote_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  /* Reset first then set to wait...clears old request stuff...or should do... */
  resetRemoteToIdle(remote_control) ;

  if (!(resetSendToRequestWait(remote_control)))
    {
      REMOTELOGMSG(remote_control, "%s", "Could not reset to wait for next request, resetting to idle.") ;

      resetRemoteToIdle(remote_control) ;
    }


  /* Optionally call back to app to say we have definitively finished with our request/reply. */
  if (remote_control->req_sent_func)
    (remote_control->req_sent_func)(remote_control->req_sent_func_data) ;

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}





/* Useful ????? */
static gboolean isValidClipboardType(guint info, ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;

  if (info == TARGET_ID)
    result = TRUE ;
  else
    REMOTELOGMSG(remote_control,
		 "Invalid clipboard datatype key, expected %d, got %d.",
		 TARGET_ID, info) ;

  return result ;
}


/* A GSourceFunc() for time outs. */
static gboolean timeOutCB(gpointer user_data)
{
  gboolean result = TRUE ;
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* THIS NEEDS TO BE SET ACCORDING TO WHETHER WE ARE PEER OR SELF..... */

  remote_control->has_timed_out = TRUE ;


  (remote_control->timeout_func)(remote_control->timeout_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  result = FALSE ;					    /* Returning FALSE removes timer. */

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}




/* Create idle record. */
static RemoteIdle createIdle(void)
{
  RemoteIdle idle ;

  idle = g_new0(RemoteIdleStruct, 1) ;
  idle->remote_type = REMOTE_TYPE_IDLE ;

  return idle ;
}


/* Create receive record. */
static RemoteReceive createReceive(GQuark app_id, char *app_unique_str,
				   ZMapRemoteControlRequestHandlerFunc process_request_func,
				   gpointer  process_request_func_data)
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

  return receive_request ;
}


/* Create either a client or peer request. */
static RemoteSend createSend(char *send_app_name, char *send_unique_atom_string,
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

  return send_request ;
}

static void resetClipboard(ZMapRemoteControl remote_control)
{
  /* Set state, this will tell our clearCB that we are resetting and that it's not a request. */
  remote_control->state = REMOTE_STATE_RESETTING_TO_IDLE ;

  /* Lose ownership of our clipboard so we don't respond to requests. */
  gtk_clipboard_clear(remote_control->receive->our_clipboard) ;


  remote_control->state = REMOTE_STATE_IDLE ;

  return ;
}


static void resetRemoteToIdle(ZMapRemoteControl remote_control)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Set state that we are resetting as the clipboard clear is essentially asynchronous. */
  remote_control->state = REMOTE_STATE_RESETTING_TO_IDLE ;

  /* Lose ownership of our clipboard so we don't respond to requests. */
  gtk_clipboard_clear(remote_control->receive->our_clipboard) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  resetReceive(remote_control->receive) ;

  resetSend(remote_control->send) ;

  remote_control->curr_remote = (RemoteAny)(remote_control->idle) ;

  remote_control->state = REMOTE_STATE_IDLE ;

  return ;
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


/* Reset either a client or peer request. */
static void resetSend(RemoteSend send_request)
{
  if (send_request->our_request)
    {
      g_free(send_request->our_request) ;
      send_request->our_request = NULL ;
    }

  return ;
}


/* Reset send to waiting for a request. */
static gboolean resetSendToRequestWait(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;
  RemoteReceive receive ;

  receive = remote_control->receive ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      logBadState(remote_control, REMOTE_STATE_IDLE,
		  "Cannot wait on receive clipboard \"%s\".",
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


  return result ;
}




/* Create idle record. */
static void destroyIdle(RemoteIdle idle_request)
{
  g_free(idle_request) ;

  return ;
}


/* Destroy receive record. */
static void destroyReceive(RemoteReceive receive_request)
{
  /* No receive or server specific code currently. */

  g_free(receive_request->their_request) ;

  g_free(receive_request) ;

  return ;
}


/* Destroy either a client or send request. */
static void destroySend(RemoteSend send_request)
{
  /* No client or peer specific code currently. */

  g_free(send_request->our_request) ;

  g_free(send_request) ;

  return ;
}







/* Called using REMOTELOGMSG(), by default this calls stderrOutputCB() unless caller
 * sets a different output function.

 remotecontrol(receiveClipboardClearWaitAfterReplyCB) Acting as SERVER - <msg text> 

 */
static void debugMsg(ZMapRemoteControl remote_control, const char *func_name, char *format_str, ...)
{				
  GString *msg ;
  va_list args ;
  char *send_type ;

  msg = g_string_sized_new(500) ;				

  if (remote_control->send && remote_control->send->our_request)
    send_type = CLIENT_PREFIX ;
  else if (remote_control->receive && remote_control->receive->their_request)
    send_type = SERVER_PREFIX ;
  else
    send_type = PEER_PREFIX ;
						
  g_string_append_printf(msg, "%s: %s()\tActing as: %s  State: %s\tMessage: ",			
			 g_quark_to_string(remote_control->app_id),
			 func_name,
			 remoteType2ExactStr(remote_control->curr_remote->remote_type),
			 remoteState2ExactStr(remote_control->state)) ;

  va_start(args, format_str) ;

  g_string_append_vprintf(msg, format_str, args) ;

  va_end(args) ;
									
  (remote_control->err_report_func)(remote_control->err_report_data, msg->str) ;			
									
  g_string_free(msg, TRUE) ;					

  return ;
}



/* Sends err_msg to stderr and flushes stderr to make the message appear immediately. */
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





static void disconnectTimerHandler(guint *func_id)
{
  if (*func_id)
    {
      gtk_timeout_remove(*func_id) ;
      *func_id = 0 ;
    }

  return ;
}


static void logBadState(ZMapRemoteControl remote_control,
			RemoteControlState expected_state, char *format, ...)
{
  char *reason_string = NULL ;

  if (format)
    {
      va_list args ;

      va_start(args, format) ;
      reason_string = g_strdup_vprintf(format, args) ;
      va_end(args) ;
    }

  REMOTELOGMSG(remote_control,
	       "Wrong State !! Expected state to be: \"%s\".%s%s",
	       remoteState2ExactStr(expected_state),
	       (reason_string ? " " : ""),
	       (reason_string ? reason_string : "")) ;

  if (reason_string)
    {
      g_free(reason_string) ;
    }

  return ;
}



static void logOutOfBandReq(ZMapRemoteControl remote_control,
			    char *format, ...)
{
  char *reason_string ;
  va_list args ;

  va_start(args, format) ;
  reason_string = g_strdup_vprintf(format, args) ;
  va_end(args) ;

  REMOTELOGMSG(remote_control,
	       "Out of Band Request: %s",
	       reason_string) ;

  g_free(reason_string) ;

  return ;
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








