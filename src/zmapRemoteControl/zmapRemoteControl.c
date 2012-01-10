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
#include <zmapRemoteControl_.h>



/* Text for debug/log messages.... */

#define PEER_PREFIX "PEER"
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


static void selfWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
			       guint info, gpointer user_data) ;
static void selfWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void selfRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				  guint info, gpointer user_data) ;
static void selfRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;
static void selfGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer data) ;
static gboolean selfProcessRequest(void *remote_data, void *reply, int reply_len) ;
static void selfClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				    guint info, gpointer user_data) ;
static void selfClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data) ;


static void peerClipboardSendRequestCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data) ;
static void peerClipboardClearCB(GtkClipboard *clipboard, gpointer user_data) ;
static void peerReplyWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					guint info, gpointer user_data) ;
static void peerReplyWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data) ;
static void peerGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data) ;
static void peerRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				  guint info, gpointer user_data) ;
static void peerRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data_or_owner) ;

static gboolean timeOutCB(gpointer user_data) ;

static void debugMsg(ZMapRemoteControl remote_control, const char *func_name, char *format_str, ...) ;
static gboolean stderrOutputCB(gpointer user_data, char *err_msg) ;

static RemoteIdle createIdle(void) ;
static RemoteSelf createSelf(char *app_str,
			     ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_func_data) ;
static RemotePeer createPeer(char *peer_unique_atom_string,
			     ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_func_data) ;

static void resetRemoteToIdle(ZMapRemoteControl remote_control) ;
static void resetIdle(RemoteIdle idle_request) ;
static void resetSelf(RemoteSelf self_request) ;
static void resetPeer(RemotePeer peer_request) ;

static void destroyIdle(RemoteIdle idle_request) ;
static void destroySelf(RemoteSelf client_request) ;
static void destroyPeer(RemotePeer server_request) ;

static void disconnectTimerHandler(guint *func_id) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean validateRequest(char *request, char **err_msg_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean isValidClipboardType(guint info, ZMapRemoteControl remote_control) ;

static void logBadState(ZMapRemoteControl remote_control,
			RemoteControlState expected_state, char *format, ...) ;
static void logOutOfBandReq(ZMapRemoteControl remote_control,
			    char *format, ...) ;

gboolean isInSelfState(RemoteControlState state) ;
gboolean isInPeerState(RemoteControlState state) ;


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
					  ZMapRemoteControlErrorHandlerFunc error_func, gpointer error_func_data,
					  ZMapRemoteControlErrorReportFunc err_report_func, gpointer err_report_data)
{
  ZMapRemoteControl remote_control = NULL ;

  if (app_id && *app_id)
    {
      remote_control = g_new0(ZMapRemoteControlStruct, 1) ;
      remote_control->magic = remote_control_magic_G ;

      remote_control->state = REMOTE_STATE_IDLE ;
      remote_control->idle = createIdle() ;
      remote_control->curr_remote = (RemoteAny)(remote_control->idle) ;


      /* Set app_id and default error reporting and then we can output error messages. */
      remote_control->app_id = g_strdup(app_id) ;

      /* Data type id as an atom for use in set/get selection. */
      remote_control->target_atom = gdk_atom_intern(ZACP_DATA_TYPE, FALSE) ;


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
gboolean zMapRemoteControlSelfInit(ZMapRemoteControl remote_control, char *app_unique_str,
				   ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_func_data)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->self)
   {
     REMOTELOGMSG(remote_control,
		  "%s",
		  "Self interface already initialised.") ;

     result = FALSE ;
   }
  else if (isInSelfState(remote_control->state) || remote_control->state != REMOTE_STATE_IDLE)
    {
      REMOTELOGMSG(remote_control,
		  "%s", "Must be in idle state or non-client state to initialise \"self\".") ;

      result = FALSE ;
    }
  else if (!app_unique_str || !(*app_unique_str))
    {
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Bad Args: no clipboard string, self interface initialisation failed.") ;
    }
  else
    {
      remote_control->self = createSelf(app_unique_str,
					request_func, request_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Self interface initialised (self clipboard is \"%s\").",
		   remote_control->self->our_atom_string) ;
      
      result = TRUE ;
    }    

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;				   

  return result ;
}


/* Until this call is issued self will not respond to any requests. */
gboolean zMapRemoteControlSelfWaitForRequest(ZMapRemoteControl remote_control)
{
  gboolean result = FALSE ;
  RemoteSelf self ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  self = remote_control->self ;

  if (remote_control->state != REMOTE_STATE_IDLE)
    {
      logBadState(remote_control, REMOTE_STATE_IDLE,
		  "Cannot wait on self clipboard \"%s\".",
		  self->our_atom_string) ;

      result = FALSE ;
    }
  else if (!(remote_control->self))
    {
      REMOTELOGMSG(remote_control,
		   "Self interface is not initialised so cannot wait on our clipboard \"%s\".",
		   self->our_atom_string) ;

      result = FALSE ;
    }
  else
    {
      if ((result = clipboardTakeOwnership(self->our_clipboard,
					   selfWaitClipboardGetCB, selfWaitClipboardClearCB,
					   remote_control)))
	{
	  REMOTELOGMSG(remote_control,
		       "Waiting (have ownership of) on our clipboard \"%s\").",
		       self->our_atom_string) ;


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_NEW_REQ ;
	  remote_control->curr_remote = (RemoteAny)(remote_control->self) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Cannot become owner of own clipboard \"%s\" so cannot wait for requests.",
		       self->our_atom_string) ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}


/* Set up the peer interface, note that the peer atom string must be unique for this connection,
 * the obvious thing is to use an xwindow id as part of the name as this will be unique. The
 * peer though can choose how it makes the sting unique but it must be _globally_ unique in
 * context of an X Windows server. */
gboolean zMapRemoteControlPeerInit(ZMapRemoteControl remote_control, char *peer_unique_string,
				   ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_func_data)
{
  gboolean result = FALSE ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->peer)
   {
     REMOTELOGMSG(remote_control,
		  "%s",
		  "Peer interface already initialised.") ;

     result = FALSE ;
   }
  else if (!isInPeerState(remote_control->state) && remote_control->state != REMOTE_STATE_IDLE)
    {
      REMOTELOGMSG(remote_control,
		  "%s", "Must be in idle state or in server state to initialise \"peer\".") ;

      result = FALSE ;
    }
  else if (!peer_unique_string || !(*peer_unique_string))
    {
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Bad Args: no peer clipboard string, peer interface initialisation failed.") ;
    }
  else
    {
      remote_control->peer = createPeer(peer_unique_string,
					reply_func, reply_func_data) ;

      REMOTELOGMSG(remote_control,
		   "Peer interface initialised (peer clipboard is \"%s\").",
		   remote_control->peer->their_atom_string) ;

      result = TRUE ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;				   

  return result ;
}



/* Make a request to a peer. */
gboolean zMapRemoteControlPeerRequest(ZMapRemoteControl remote_control, char *request)
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
  else if (!(remote_control->peer))
    {
      /* we have not been given the peer id so cannot make requests to it. */
      REMOTELOGMSG(remote_control,
		   "%s",
		   "Peer interface not initialised so cannot send request.") ;

      result = FALSE ;
    }
  else
    {
      RemotePeer peer ;

      /* If waiting for new request from some client then reset to idle before making request. */
      if (remote_control->state != REMOTE_STATE_IDLE)
	{
	  resetRemoteToIdle(remote_control) ;
	}

      /* Try sending a request to our peer. */
      peer = remote_control->peer ;
      peer->our_request = g_strdup(request) ;

      /* Swop to client state to make request. */
      remote_control->state = REMOTE_STATE_CLIENT_WAIT_GET ;
      remote_control->curr_remote = (RemoteAny)(remote_control->peer) ;


      REMOTELOGMSG(remote_control, "Taking ownership of peer clipboard \"%s\" to make request.",
		   peer->their_atom_string, request) ;

      if ((result = gtk_clipboard_set_with_data(peer->their_clipboard,
						clipboard_target_G,
						TARGET_NUM,
						peerClipboardSendRequestCB,
						peerClipboardClearCB,
						remote_control)))
	{
	  REMOTELOGMSG(remote_control,
		       "Have ownership of peer clipboard \"%s\", waiting for peer to ask for our request.",
		       peer->their_atom_string) ;

	  if (peer->timeout_ms)
	    remote_control->timer_source_id = g_timeout_add(peer->timeout_ms,
							    timeOutCB, remote_control) ;
	}
      else
	{
    	  REMOTELOGMSG(remote_control,
		       "Cannot set request on peer clipboard \"%s\".",
		       peer->their_atom_string) ;


	  /* Need to reset state here.... */
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
  /* allow separate timeouts for self and peer ???? need to be able to set them separately... */

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
  if (remote_control->self)
    destroySelf(remote_control->self) ;
  if (remote_control->peer)
    destroyPeer(remote_control->peer) ;


  REMOTELOGMSG(remote_control, "%s", "Destroyed RemoteControl object.") ;

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;		    /* Latest we can make this call as we 
							       need remote_control. */
  g_free(remote_control->app_id) ;

  ZMAP_MAGIC_RESET(remote_control->magic) ;		    /* Before free() reset magic to invalidate memory. */

  g_free(remote_control) ;

  return ;
}





/* 
 *                    Package routines.
 */





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
static void selfWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  if (remote_control->state == REMOTE_STATE_RESETTING_TO_IDLE)
    {
      DEBUGLOGMSG(remote_control, "%s", "Resetting our state to idle.") ;

      remote_control->state = REMOTE_STATE_IDLE ;
    }
  else if (remote_control->state == REMOTE_STATE_SERVER_WAIT_NEW_REQ)
    {
      gtk_clipboard_request_contents(clipboard,
				     remote_control->target_atom,
				     selfGetRequestClipboardCB, remote_control) ;

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
static void selfWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
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
		   remote_control->self->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}





/* GtkClipboardReceivedFunc() function called when client sends us it's request. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void selfGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSelf self_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  self_request = remote_control->self ;


  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_SEND,
		  "%s"
		  "Expected to be waiting for request.") ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Received request from peer on self clipboard \"%s\".",
		   self_request->our_atom_string) ;


      /* IS THIS THE PLACE TO DO THIS...??? */
      /* Retake ownership so they know we have got request. */
      if (!clipboardTakeOwnership(self_request->our_clipboard,
				  selfRequestReceivedClipboardGetCB, selfRequestReceivedClipboardClearCB,
				  remote_control))
	{
	  REMOTELOGMSG(remote_control,
		       "Tried to retake ownership of self clipboard \"%s\" to signal peer that we have"
		       " request but this failed, aborting request.",
		       self_request->our_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Have retaken ownership of self clipboard \"%s\" to signal peer that we have their request.",
		       self_request->our_atom_string) ;

	  /* All ok so record the request, we'll process it when we get a clear from the peer. */
	  if (selection_data->length < 0)
	    {
	      REMOTELOGMSG(remote_control,
			   "Could not retrieve peer data from clipboard \"%s\".",
			   self_request->our_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Retrieved peers request from clipboard \"%s\", request is:\n \"%s\".",
			   self_request->our_atom_string, selection_data->data) ;

	      self_request->their_request = g_strdup((char *)(selection_data->data)) ;


	      remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_ACK ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void selfGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSelf self_request ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  self_request = remote_control->self ;


  if (remote_control->state != REMOTE_STATE_SERVER_WAIT_REQ_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_SERVER_WAIT_REQ_SEND,
		  "%s"
		  "Expected to be waiting for request.") ;
    }
  else
    {

      REMOTELOGMSG(remote_control,
		   "Received request from peer on self clipboard \"%s\".",
		   self_request->our_atom_string) ;

      /* Check request is ok. */
      if (selection_data->length < 0)
	{
	  REMOTELOGMSG(remote_control,
		       "Could not retrieve peer data from clipboard \"%s\".",
		       self_request->our_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Retrieved peers request from clipboard \"%s\", request is:\n \"%s\".",
		       self_request->our_atom_string, selection_data->data) ;

	  self_request->their_request = g_strdup((char *)(selection_data->data)) ;


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_REQ_ACK ;


	  /* Retake ownership so they know we have got request. */
	  if (!clipboardTakeOwnership(self_request->our_clipboard,
				      selfRequestReceivedClipboardGetCB, selfRequestReceivedClipboardClearCB,
				      remote_control))
	    {
	      REMOTELOGMSG(remote_control,
			   "Tried to retake ownership of self clipboard \"%s\" to signal peer that we have"
			   " request but this failed, aborting request.",
			   self_request->our_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Have retaken ownership of self clipboard \"%s\" to signal peer that we have their request.",
			   self_request->our_atom_string) ;

	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}










/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is waiting
 * for the peer to retake ownership of the our clipboard as a signal that we can begin
 * processing the request. */
static void selfRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
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
		   remote_control->self->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* A GtkClipboardClearFunc() function called when the clipboard is cleared by our peer, now we can
 * go ahead and service the request. */
static void selfRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSelf self_request ;

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
      self_request = remote_control->self ;

      REMOTELOGMSG(remote_control, "Calling apps callback func to process request:\n \"%s\"",
		   self_request->their_request) ;

      /* set state here as not sure when app will call us back ?? */
      remote_control->state = REMOTE_STATE_SERVER_PROCESS_REQ ;

      /* Call the app callback. */
      (self_request->request_func)(remote_control, selfProcessRequest, remote_control,
				   self_request->their_request, self_request->request_func_data) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* A ZMapRemoteControlCallWithReplyFunc() which is called by the application when it has 
 * finished processing the request. */
static gboolean selfProcessRequest(void *remote_data, void *reply, int reply_len)
{
  gboolean result = FALSE ;
  ZMapRemoteControl remote_control = (ZMapRemoteControl)remote_data ;
  RemoteSelf self ;

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
      self = remote_control->self ;

      REMOTELOGMSG(remote_control, "Received reply from app: \"%s\" (length %d).",
		   (char *)reply, reply_len) ;

      self->our_reply = reply ;

      if ((result = gtk_clipboard_set_with_data(self->our_clipboard,
						clipboard_target_G,
						TARGET_NUM,
						selfClipboardGetReplyCB,
						selfClipboardClearWaitAfterReplyCB,
						remote_control)))
	{
	  REMOTELOGMSG(remote_control,
		       "Have ownership of peer clipboard \"%s\", waiting for peer to ask for our reply.",
		       self->our_atom_string) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  remote_control->state = CLIENT_STATE_SENDING ;


	  if (peer->timeout_ms)
	    remote_control->timer_source_id = g_timeout_add(peer->timeout_ms,
							    timeOutCB, remote_control) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  remote_control->state = REMOTE_STATE_SERVER_WAIT_GET ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Cannot set request on peer clipboard \"%s\".",
		       self->our_atom_string) ;
	}
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return result ;
}



/* GtkClipboardGetFunc() called when peer wants our reply. */
static void selfClipboardGetReplyCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					 guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSelf self ;
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
      self = remote_control->self ;

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
		       self->our_atom_string,
		       target_name, self->our_reply) ;

	  /* Set the data in the selection data param. */
	  gtk_selection_data_set(selection_data,
				 remote_control->target_atom,
				 8,
				 (guchar *)(self->our_reply),
				 (int)(strlen(self->our_reply) + 1)) ;

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
static void selfClipboardClearWaitAfterReplyCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemoteSelf self ;

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
      self = remote_control->self ;

      REMOTELOGMSG(remote_control,
		   "Lost ownership of self clipboard \"%s\", about to retake it to signal we know they have our reply.",
		   self->our_atom_string) ;

      /* Back we go to the beginning..... */
      if (clipboardTakeOwnership(self->our_clipboard,
				 selfWaitClipboardGetCB, selfWaitClipboardClearCB,
				 remote_control))
	{
	  REMOTELOGMSG(remote_control,
		       "Self interface initialised (now have ownership of our clipboard \"%s\").",
		       self->our_atom_string) ;

	  remote_control->state = REMOTE_STATE_SERVER_WAIT_NEW_REQ ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Self init failed, cannot become owner of own clipboard \"%s\".",
		       self->our_atom_string) ;
	}
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
static void peerClipboardSendRequestCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
				       guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;
  GdkAtom target_atom ;
  char *target_name ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_GET)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
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
		       peer->their_atom_string,
		       target_name, peer->our_request) ;

	  /* Set the data in the selection data param. */
	  gtk_selection_data_set(selection_data,
				 remote_control->target_atom,
				 8,
				 (guchar *)(peer->our_request),
				 (int)(strlen(peer->our_request) + 1)) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REQ_ACK ;
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* Called when peer has received our request and retakes ownership of it's clipboard
 * to signal it has the request. */
static void peerClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REQ_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Peer has retaken ownership their clipboard \"%s\" to signal they have received our request.",
		   peer->their_atom_string) ;


      /* Retake ownership of the peers clipboard to signal that we know they got the request and we
       * are now waiting for their reply. */
      if (!clipboardTakeOwnership(peer->their_clipboard,
				  peerReplyWaitClipboardGetCB, peerReplyWaitClipboardClearCB,
				  remote_control))
	{
	  REMOTELOGMSG(remote_control,
		       "Failed to retake ownership of their clipboard \"%s\", aborting request.",
		       peer->their_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Have retaken ownership of their clipboard \"%s\" to signal they can process the request.",
		       peer->their_atom_string) ;

	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY ;
	}

    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}




/* GtkClipboardGetFunc() called if someone does a 'get' on the peers clipboard while we
 * are waiting for a clear, shouldn't normally happen. */
static void peerReplyWaitClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;


  peer = remote_control->peer ;


  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      /* We should probably try to put more info about the request here. */

      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" for %s, ignoring request and resetting state to %s.",
		   peer->their_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* GtkClipboardClearFunc() function called when a peer signals it has finished request
 * and wants to return result.
 * 
 *  */
static void peerReplyWaitClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Lost ownership of self clipboard \"%s\", asking for peers reply to our request.",
		   peer->their_atom_string) ;

      gtk_clipboard_request_contents(clipboard,
				     remote_control->target_atom,
				     peerGetRequestClipboardCB, remote_control) ;

      remote_control->state = REMOTE_STATE_CLIENT_WAIT_SEND ;
    }


  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}



/* GtkClipboardReceivedFunc() function called when peer sends us it's reply. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void peerGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Received reply from peer on self clipboard \"%s\".",
		   peer->their_atom_string) ;


      /* IS THIS THE PLACE TO DO THIS...??? */
      /* Retake ownership so they know we have got reply. */
      if (!clipboardTakeOwnership(peer->their_clipboard,
				  peerRequestReceivedClipboardGetCB, peerRequestReceivedClipboardClearCB,
				  remote_control))
	{
	  REMOTELOGMSG(remote_control,
		       "Tried to retake ownership of self clipboard \"%s\" to signal peer that we have"
		       " request but this failed, aborting request.",
		       peer->their_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Have retaken ownership of self clipboard \"%s\" to signal peer that we have their reply.",
		       peer->their_atom_string) ;

	  /* All ok so record the reply, we'll process it when we get a clear from the peer. */
	  if (selection_data->length < 0)
	    {
	      REMOTELOGMSG(remote_control,
			   "Could not retrieve peer data from clipboard \"%s\".",
			   peer->their_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Retrieved peers reply from clipboard \"%s\", reply is:\n \"%s\".",
			   peer->their_atom_string, selection_data->data) ;

	      peer->their_reply = g_strdup((char *)(selection_data->data)) ;


	      remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY_ACK ;
	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void peerGetRequestClipboardCB(GtkClipboard *clipboard, GtkSelectionData *selection_data, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_SEND)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
    }
  else
    {
      REMOTELOGMSG(remote_control,
		   "Received reply from peer on self clipboard \"%s\".",
		   peer->their_atom_string) ;


      /* All ok so record the reply, we'll process it when we get a clear from the peer. */
      if (selection_data->length < 0)
	{
	  REMOTELOGMSG(remote_control,
		       "Could not retrieve peer data from clipboard \"%s\".",
		       peer->their_atom_string) ;
	}
      else
	{
	  REMOTELOGMSG(remote_control,
		       "Retrieved peers reply from clipboard \"%s\", reply is:\n \"%s\".",
		       peer->their_atom_string, selection_data->data) ;

	  peer->their_reply = g_strdup((char *)(selection_data->data)) ;


	  remote_control->state = REMOTE_STATE_CLIENT_WAIT_REPLY_ACK ;

	  /* STATE NEEDS SORTING OUT ON ERROR.... */
	  /* Retake ownership so they know we have got reply. */
	  if (!clipboardTakeOwnership(peer->their_clipboard,
				      peerRequestReceivedClipboardGetCB, peerRequestReceivedClipboardClearCB,
				      remote_control))
	    {
	      REMOTELOGMSG(remote_control,
			   "Tried to retake ownership of self clipboard \"%s\" to signal peer that we have"
			   " request but this failed, aborting request.",
			   peer->their_atom_string) ;
	    }
	  else
	    {
	      REMOTELOGMSG(remote_control,
			   "Have retaken ownership of self clipboard \"%s\" to signal peer that we have their reply.",
			   peer->their_atom_string) ;

	    }
	}
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}






/* GtkClipboardGetFunc() called if there is a 'get' request while remote control is waiting
 * for the self to retake ownership of their clipboard, this shouldn't happen in normal
 * processing. */
static void peerRequestReceivedClipboardGetCB(GtkClipboard *clipboard, GtkSelectionData *selection_data,
					      guint info, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;


  if (!isValidClipboardType(info, remote_control))
    {
      /* Log that this is "out of band"...i.e. we are not expecting to get here in the normal
       * protocol. */
      /* We should probably try to put more info about the request here. */

      REMOTELOGMSG(remote_control,
		   "Received out-of-band \"get\" for %s, ignoring request and resetting state to %s.",
		   remote_control->self->our_atom_string, remoteState2ExactStr(remote_control->state)) ;
    }

  DEBUGLOGMSG(remote_control, "%s", EXIT_TXT) ;

  return ;
}


/* A GtkClipboardClearFunc() function called when the clipboard is cleared by peer to signal they
 * know we have the reply. */
static void peerRequestReceivedClipboardClearCB(GtkClipboard *clipboard, gpointer user_data)
{
  ZMapRemoteControl remote_control = (ZMapRemoteControl)user_data ;
  RemotePeer peer ;

  ZMAP_MAGIC_ASSERT(remote_control_magic_G, remote_control->magic) ;

  DEBUGLOGMSG(remote_control, "%s", ENTER_TXT) ;

  peer = remote_control->peer ;

  if (remote_control->state != REMOTE_STATE_CLIENT_WAIT_REPLY_ACK)
    {
      logBadState(remote_control, REMOTE_STATE_CLIENT_WAIT_GET,
		  "Already servicing a request: \"%s\".",
		  peer->our_request) ;
    }
  else
    {
      gboolean app_result ;

      REMOTELOGMSG(remote_control,
		   "Lost ownership of peer clipboard \"%s\", they now know we have the reply.",
		   peer->their_atom_string) ;

      REMOTELOGMSG(remote_control, "About to call apps callback to process the reply:\n \"%s\"", peer->their_reply) ;

      /* Call the app callback. */
      app_result = (peer->reply_func)(remote_control, peer->their_reply, peer->reply_func_data) ;

      REMOTELOGMSG(remote_control, "Apps reply callback %s.", (app_result ? "succeeded" : "failed")) ;

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
    }

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


/* Create self record. */
static RemoteSelf createSelf(char *app_str,
			     ZMapRemoteControlRequestHandlerFunc request_func, gpointer request_func_data)
{
  RemoteSelf self_request ;
  char *tmp ;

  self_request = g_new0(RemoteSelfStruct, 1) ;
  self_request->remote_type = REMOTE_TYPE_SERVER ;

  /* Set up the request atom, this atom will hold the request/replies from a client process. */
  tmp = g_strdup(app_str) ;

  self_request->our_atom = gdk_atom_intern(tmp, FALSE) ;

  g_free(tmp) ;

  self_request->our_atom_string = gdk_atom_name(self_request->our_atom) ;

  self_request->our_clipboard = gtk_clipboard_get(self_request->our_atom) ;

  self_request->request_func = request_func ;
  self_request->request_func_data = request_func_data ;

  return self_request ;
}


/* Create either a client or peer request. */
static RemotePeer createPeer(char *peer_unique_atom_string,
			     ZMapRemoteControlReplyHandlerFunc reply_func, gpointer reply_func_data)
{
  RemotePeer peer_request ;

  peer_request = g_new0(RemotePeerStruct, 1) ;
  peer_request->remote_type = REMOTE_TYPE_CLIENT ;

  peer_request->their_atom_string = g_strdup(peer_unique_atom_string) ;

  peer_request->their_atom = gdk_atom_intern(peer_request->their_atom_string, FALSE) ;

  peer_request->their_clipboard = gtk_clipboard_get(peer_request->their_atom) ;

  peer_request->reply_func = reply_func ;
  peer_request->reply_func_data = reply_func_data ;

  return peer_request ;
}


static void resetRemoteToIdle(ZMapRemoteControl remote_control)
{
  /* Set state that we are resetting as the clipboard clear is essentially asynchronous. */
  remote_control->state = REMOTE_STATE_RESETTING_TO_IDLE ;

  /* Lose ownership of our clipboard so we don't respond to requests. */
  gtk_clipboard_clear(remote_control->self->our_clipboard) ;

  resetSelf(remote_control->self) ;

  resetPeer(remote_control->peer) ;

  remote_control->curr_remote = (RemoteAny)(remote_control->idle) ;

  return ;
}



/* Reset idle record. */
static void resetIdle(RemoteIdle idle_request)
{

  return ;
}


/* Reset self record. */
static void resetSelf(RemoteSelf self_request)
{
  if (self_request->their_request)
    g_free(self_request->their_request) ;

  return ;
}


/* Reset either a client or peer request. */
static void resetPeer(RemotePeer peer_request)
{
  if (peer_request->our_request)
    g_free(peer_request->our_request) ;

  return ;
}


/* Create idle record. */
static void destroyIdle(RemoteIdle idle_request)
{
  g_free(idle_request) ;

  return ;
}


/* Destroy self record. */
static void destroySelf(RemoteSelf self_request)
{
  /* No self or server specific code currently. */

  g_free(self_request->their_request) ;

  g_free(self_request) ;

  return ;
}


/* Destroy either a client or peer request. */
static void destroyPeer(RemotePeer peer_request)
{
  /* No client or peer specific code currently. */

  g_free(peer_request->our_request) ;

  g_free(peer_request) ;

  return ;
}







/* Called using REMOTELOGMSG(), by default this calls stderrOutputCB() unless caller
 * sets a different output function.

 remotecontrol(selfClipboardClearWaitAfterReplyCB) Acting as SERVER - <msg text> 

 */
static void debugMsg(ZMapRemoteControl remote_control, const char *func_name, char *format_str, ...)
{				
  GString *msg ;
  va_list args ;
  char *peer_type ;

  msg = g_string_sized_new(500) ;				

  if (remote_control->peer && remote_control->peer->our_request)
    peer_type = CLIENT_PREFIX ;
  else if (remote_control->self && remote_control->self->their_request)
    peer_type = SERVER_PREFIX ;
  else
    peer_type = PEER_PREFIX ;
						
  g_string_append_printf(msg, "%s: %s()\tActing as: %s  State: %s\tMessage: ",			
			 remote_control->app_id,
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

gboolean isInSelfState(RemoteControlState state)
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



gboolean isInPeerState(RemoteControlState state)
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





