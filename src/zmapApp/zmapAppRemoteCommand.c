/*  File: zmapAppRemoteCommand.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: functions to process remote control commands for
 *              the "App" level of ZMap.
 *
 * Exported functions: See zmapApp_P.h
 * HISTORY:
 * Last edited: Apr 11 10:51 2012 (edgrif)
 * Created: Mon Jan 16 14:04:43 2012 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapRemoteCommand.h>
#include <zmapApp_P.h>



/* COPY STUFF FROM OLD REMOTE... */

/* Requests that come to us from an external program. */
typedef enum
  {
    ZMAPAPP_REMOTE_INVALID,
    ZMAPAPP_REMOTE_OPEN_ZMAP,				    /* Open a new zmap window. */
    ZMAPAPP_REMOTE_CLOSE_ZMAP,				    /* Close a window. */
    ZMAPAPP_REMOTE_UNKNOWN
  } ZMapAppValidXRemoteAction;



/* This should be somewhere else ...
   or we should be making other objects */
typedef struct
{
  ZMapAppContext app_context;
  ZMapAppValidXRemoteAction action;

  char *command_name ;
  RemoteCommandRCType command_rc ;
  char *err_msg ;

  GString *message;


  /* only for now....not sure why this is in app_context->info->code */
  int code ;


  GQuark sequence ;
  GQuark start ;
  GQuark end ;
  GQuark add_to_view ;

  /* Is this used ?????? */
  GQuark source ;

  GQuark view_id ;

} RequestDataStruct, *RequestData ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef struct
{
  int code;

  GString *message;
}ResponseContextStruct, *ResponseContext;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static void localProcessRemoteRequest(gpointer local_data,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void localProcessReplyFunc(char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
				  gpointer reply_handler_func_data) ;



static void processRequest(ZMapAppContext app_context,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void parseExecuteRequest(ZMapAppContext app_context,
				char *command_name, ZMapAppRemoteViewID view_id, char *request,
				RemoteCommandRCType *command_rc_out, char **reason_out,
				ZMapXMLUtilsEventStack *reply_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static ZMapXMLUtilsEventStack createPeerElement(char *app_id, char *app_unique_id) ;

static gboolean shutdownHandler(gpointer data) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static char *application_execute_command(char *command_text, gpointer app_context,
					 int *statusCode, ZMapXRemoteObj owner);
static void send_finalised(ZMapXRemoteObj client);
static gboolean finalExit(gpointer data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);

static void createZMap(ZMapAppContext app,
		       GQuark sequence_id, int start, int end, GQuark add_to_view,
		       RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;
static void closeZMap(ZMapAppContext app,
		      GQuark view_id,
		      RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/*
 *        Globals
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapXMLObjTagFunctionsStruct start_handlers_G[] =
  {
    { "zmap", start },
    { "request", req_start },
    { NULL,   NULL  }
  };
static ZMapXMLObjTagFunctionsStruct end_handlers_G[] =
  {
    { "zmap",    end  },
    { "request", req_end  },
    { NULL,   NULL }
  };
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







/* 
 *                 External functions.
 */


/* Handles all incoming requests to zmap from a peer. */
void zmapAppProcessAnyRequest(ZMapAppContext app_context,
			      char *request, ZMapRemoteAppReturnReplyFunc replyHandlerFunc)
{
  ZMapAppRemote remote = app_context->remote_control ;
  RemoteValidateRCType valid_rc ;
  gboolean result ;
  char *command_name = NULL ;
  char *id_str = NULL ;
  ZMapAppRemoteViewIDStruct view_id = {NULL}, *view_id_ptr ;
  char *err_msg = NULL ;
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_OK ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  /* Set state...for new request (should allocate/deallocate...one step at a time.... */
  remote->curr_request = request ;


  /* Validate the request envelope. */
  if ((valid_rc = zMapRemoteCommandValidateEnvelope(remote->remote_controller, request, &err_msg))
       != REMOTE_VALIDATE_RC_OK)
    {
      gboolean abort ;

      if (valid_rc == REMOTE_VALIDATE_RC_ENVELOPE_XML || valid_rc == REMOTE_VALIDATE_RC_ENVELOPE_CONTENT)
	abort = TRUE ;
      else
	abort = FALSE ;

      command_rc = REMOTE_COMMAND_RC_BAD_XML ;
      reason = g_strdup_printf("Bad xml: %s", err_msg) ;
      reply = NULL ;

      (replyHandlerFunc)(command_name, abort, command_rc, reason, reply, app_context) ;

      zMapLogWarning("Bad request envelope: %s", err_msg) ;

      g_free(reason) ;
      g_free(err_msg) ;
    }
  else if (!(command_name = zMapRemoteCommandRequestGetCommand(remote->curr_request)))
    {
      /* Still record current request, needed for debugging and replying. */
      remote->curr_request = request ;

      command_rc = REMOTE_COMMAND_RC_BAD_XML ;
      reason = "Command is a required attribute for the \"request\" attribute." ;
      reply = NULL ;

      (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
    }
  else
    {
      /* Fill in current request and from that the current command and view_id to which 
       * the command should be applied. ViewID id defaults to first one allocated if none specified. */
      remote->curr_request = request ;

      command_name = zMapRemoteCommandRequestGetCommand(remote->curr_request) ;

      /* Check to see if caller provided a view_id id, if not then set the default
       * view_id if there is one, otherwise pass an empty one (empty one means we don't.
       * have a zmap displayed yet. */
      view_id_ptr = &view_id ;

      if (zMapRemoteCommandGetAttribute(remote->curr_request,
					ZACP_REQUEST, ZACP_VIEWID, &(id_str),
					&err_msg))
	{
	  if (zMapAppRemoteViewParseIDStr(id_str, &view_id_ptr))
	    {
	      remote->curr_view_id = view_id ;		    /* struct copy. */
	    }
	  else
	    {
	      command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	      reason = "Bad view_id !" ;
	      reply = NULL ;

	      (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;

	      result = FALSE ;
	    }
	}
      else if (zMapAppRemoteViewIsValidID(&(remote->default_view_id)))
	{
	  remote->curr_view_id = view_id = remote->default_view_id ; /* struct copies. */
	}



      /* If the command is known at this level then call the command processing code for
       * this (App) level. If the command is not known then pass it down to the next level
       * and so on, if the command is not known at any level then we send back an error message. */
      if (result)
	{
	  if ((strcmp(command_name, ZACP_PING) == 0
	       || strcmp(command_name, ZACP_SHUTDOWN) == 0))
	    {
	      localProcessRemoteRequest(app_context,
					command_name, &view_id, request, replyHandlerFunc, app_context) ;
	    }
	  else if (!(result = zMapManagerProcessRemoteRequest(app_context->zmap_manager,
							      command_name, &view_id, request,
							      replyHandlerFunc, app_context)))
	    {
	      RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_CMD_UNKNOWN ;
	      char *reason = "Command not known !" ;
	      ZMapXMLUtilsEventStack reply = NULL ;

	      (replyHandlerFunc)(command_name, FALSE, command_rc, reason, reply, app_context) ;
	    }
	  else
	    {
	      /* We have no default view_id so set one up from returned view_id. */
	      if (!(zMapAppRemoteViewIsValidID(&(remote->default_view_id))))
		remote->default_view_id = view_id ;
	    }
	}

      /* Reset current stuff. */
      remote->curr_view_id.zmap = remote->curr_view_id.view = remote->curr_view_id.window = NULL ;
    }

  return ;
}




/* Handles all incoming replies to zmap from a peer, there is no return result from
 * here because only the reply function that is called from here knows how to handle
 * any errors. */
void zmapAppProcessAnyReply(ZMapAppContext app_context, char *reply)
{
  ZMapAppRemote remote = app_context->remote_control ;
  char *command = NULL ;
  RemoteCommandRCType command_rc ;
  char *reason = NULL ;
  char *reply_body = NULL ;
  char *error_out = NULL ;

  /* Revisit this .....we are creating a problem for ourselves in that the process_reply_func
   * needs to be be called even if the reply is badly formed so that it can reset state etc.... */

  /* Need to parse out reply components..... */
  if (!(zMapRemoteCommandReplyGetAttributes(reply,
					    &command,
					    &command_rc, &reason,
					    &reply_body,
					    &error_out)))
    {
      /* Log the bad format reply. */
      zMapLogCritical("Could not parse peer programs reply because \"%s\", their reply was: \"%s\"",
		      error_out, reply) ;

      /* MAKE SURE IT'S OK TO DO THIS..... */
      g_free(error_out) ;
    }


  /* Return the reply to the relevant zmap sub-system, note if the reply could not be parsed then
   * command will be NULL, all called functions must cope with this. */
  (remote->process_reply_func)(command, command_rc, reason, reply_body, remote->process_reply_func_data) ;


  return ;
}



/* Connect to peer. */
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  ZMapAppRemote remote = app_context->remote_control ;
  ZMapRemoteAppMakeRequestFunc request_func ;
  ZMapXMLUtilsEventStack request_body ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  request_body = createPeerElement(remote->app_id, remote->app_unique_id) ;

  (request_func)(ZACP_HANDSHAKE, request_body, app_context, localProcessReplyFunc, app_context) ;

  return result ;
}


/* Ping peer. */
gboolean zmapAppRemoteControlPing(ZMapAppContext app_context)
{
  gboolean result = TRUE ;
  ZMapRemoteAppMakeRequestFunc request_func ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  (request_func)(ZACP_PING, NULL, app_context, localProcessReplyFunc, app_context) ;

  return result ;
}


/* Disconnect from peer. */
gboolean zmapAppRemoteControlDisconnect(ZMapAppContext app_context, gboolean app_exit)
{
  gboolean result = TRUE ;
  ZMapRemoteAppMakeRequestFunc request_func ;
  ZMapXMLUtilsEventStack goodbye_type = NULL ;

  zmapAppPingStop(app_context) ;

  request_func = zmapAppRemoteControlGetRequestCB() ;

  if (app_exit)
    {
      /* Add element to say we are going to exit. */
      goodbye_type = zMapRemoteCommandCreateElement(ZACP_GOODBYE, ZACP_GOODBYE_TYPE, ZACP_EXIT) ;
    }

  (request_func)(ZACP_GOODBYE, goodbye_type, app_context, localProcessReplyFunc, app_context) ;

  return result ;
}


/* Gets called by remotecontrol right at the end of the transaction to process a request we sent. */
void zmapAppRemoteControlOurRequestEndedCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;


  return ;
}


/* Gets called by remotecontrol right at the end of the transaction for cases where we need
 * to process a request we received _after_ the reply has reached the peer, e.g. shutdown...
 * 
 * Slightly tricky coding here. We can't just shutdown here as we were called from RemoteControl
 * and this would cause errors in that code. Instead we set a timeout so that we return to
 * RemoteControl which returns the gtk mainloop and then our timeout routine gets called which
 * then does the shutdown. (we don't use a "sendevent" because stupid gtk will do a direct
 * function call to our sendevent handler instead of putting the event on the event stack
 * which would leave us with the same problem..big sigh...).
 * 
 */
void zmapAppRemoteControlTheirRequestEndedCB(void *user_data)
{
  ZMapAppContext app_context = (ZMapAppContext)user_data ;

  if (app_context->remote_control->deferred_shutdown)
    {
      int exit_timeout = 500 ;				    /* time in milliseconds. */

      g_timeout_add(exit_timeout, shutdownHandler, (gpointer)app_context) ;
    }


  return ;
}




/* 
 *                       Internal routines.
 */



/* Creates:
 * 
 *       <peer app_id="XXXX" unique_id="YYYY"/>
 * 
 * e.g.
 *       <peer app_id="ZMap" unique_id="ZMap-13385-1328020970"/>
 * 
 *  */
static ZMapXMLUtilsEventStack createPeerElement(char *app_id, char *app_unique_id)
{
  static ZMapXMLUtilsEventStackStruct
    peer[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, "peer",      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "app_id",    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     "unique_id", ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   "peer",      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  /* Fill in peer element attributes. */
  peer[1].value.q = g_quark_from_string(app_id) ;
  peer[2].value.q = g_quark_from_string(app_unique_id) ;

  return &peer[0] ;
}


/* A GSourceFunc, called to ensure that we have finished remotecontrol stuff and
 * then this routine is called from the gtk mainloop to start the shutdown. */
static gboolean shutdownHandler(gpointer data)
{
  ZMapAppContext app_context = (ZMapAppContext)data ;

  /* kill remotecontrol first so we don't recurse trying to report our exit back to peer. */
  zmapAppRemoteControlDestroy(app_context) ;

  /* now exit. */
  zmapAppExit(app_context) ;

  return FALSE ;
}


/* This is where we process a command and return the result via the callback which allows
 * for asynch returning of values where needed. */
static void localProcessRemoteRequest(gpointer local_data,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  ZMapAppContext app_context = (ZMapAppContext)local_data ;
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  processRequest(app_context, command_name, view_id, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}



/* This is where we process a reply received from a peer. */
static void localProcessReplyFunc(char *command,
				  RemoteCommandRCType command_rc,
				  char *reason,
				  char *reply,
				  gpointer reply_handler_func_data)
{
  ZMapAppContext app_context = (ZMapAppContext)reply_handler_func_data ;


  if (strcmp(command, ZACP_HANDSHAKE) == 0)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapAppPingStart(app_context) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  else if (strcmp(command, ZACP_GOODBYE) == 0)
    {
      /* should log any good bye problems here...... */

      /* Peer has replied so now we need to exit. */
      (app_context->remote_control->exit_routine)(app_context) ;
    }


  return ;
}


/* It's a command we know so get on and process it, result is returned in command_rc et al.
 * 
 * Note that some commands (ping, shutdown) have no arguments do not need parsing.
 *  */
static void processRequest(ZMapAppContext app_context,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{

  if (strcmp(command_name, ZACP_SHUTDOWN) == 0)
    {
      char *shutdown_type = NULL ;
      char *err_msg = NULL ;

      /* If our peer asked us to abort then we exit immediately right here..... */
      if (zMapRemoteCommandGetAttribute(request,
					ZACP_SHUTDOWN_TAG, ZACP_SHUTDOWN_TYPE, &shutdown_type,
					&err_msg)
	  && strcmp(shutdown_type, ZACP_ABORT) == 0)
	{
	  gtk_exit(EXIT_FAILURE) ;
	}
      else
	{
	  /* Although we return a result here, the shutdown needs to be deferred until our reply gets
	   * through to the peer.  We initiate the shutdown from the remote callback which is called
	   * right at the end of the transaction and get that to work off a timeout routine. */
	  *command_rc_out = REMOTE_COMMAND_RC_OK ;
	  *reason_out = NULL ;

	  *reply_out = zMapRemoteCommandMessage2Element("zmap shutting down now !") ;

	  app_context->remote_control->deferred_shutdown = TRUE ;
	}
    }
  else if (strcmp(command_name, ZACP_PING) == 0)
    {
      *command_rc_out = REMOTE_COMMAND_RC_OK ;
      *reason_out = NULL ;

      *reply_out = zMapRemoteCommandMessage2Element("ping ok !") ;
    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* MOVED TO ZMAPMANAGER..... */

  else
    {
      parseExecuteRequest(app_context, command_name, view_id, request,
			  command_rc_out, reason_out, reply_out) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE



/* Commands by this routine need the xml to be parsed for arguments etc. */
static void parseExecuteRequest(ZMapAppContext app_context, char *command_name, char *resource_id, char *request,
				RemoteCommandRCType *command_rc_out, char **reason_out,
				ZMapXMLUtilsEventStack *reply_out)
{
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


  /* set up return code etc. for parsing code. */
  request_data.command_name = command_name ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers_G, end_handlers_G) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))

    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = g_strdup(zMapXMLParserLastErrorMsg(parser)) ;
    }
  else
    {
      if (strcmp(command_name, ZACP_NEWVIEW) == 0)
	{
	  /* This call directly sets command_rc etc. */
          createZMap(app_context,
		     request_data.sequence, request_data.start, request_data.end, 0,
		     command_rc_out, reason_out, reply_out) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* WHAT IS THIS ABOUT....SIGH.... */
          if (app_context->info)
            request_data.code = app_context->info->code ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else if (strcmp(command_name, ZACP_ADD_TO_VIEW) == 0)
	{
	  /* This call directly sets command_rc etc. */
          createZMap(app_context,
		     request_data.sequence, request_data.start, request_data.end, resource_id,
		     command_rc_out, reason_out, reply_out) ;
	}
      else if (strcmp(command_name, ZACP_CLOSEVIEW) == 0)
	{
	  closeZMap(app_context, resource_id,
		    command_rc_out, reason_out, reply_out) ;
	}
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser) ;

  return ;
}

/* Make a new zmap window containing a view of the given sequence from start -> end. */
static void createZMap(ZMapAppContext app,
		       GQuark sequence_id, int start, int end, GQuark add_to_view,
		       RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out)
{
  char *sequence = NULL ;
  ZMapFeatureSequenceMap seq_map ;
  char *err_msg = NULL ;
  ZMap zmap = NULL ;
  ZMapView view = NULL ;
  static ZMapXMLUtilsEventStackStruct
    view_reply[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VIEWID,    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;


  /* MH17: this is a bodge FTM, we need a dataset XRemote field as well */
  /* default sequence may be NULL */
  seq_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  if (sequence_id)
    {
      seq_map->sequence = sequence = g_strdup(g_quark_to_string(sequence_id)) ;

      seq_map->start = start ;
      seq_map->end = end ;
    }
  else if (app->default_sequence->sequence)
    {
      seq_map->sequence = app->default_sequence->sequence ;
      seq_map->start = app->default_sequence->start ;
      seq_map->end = app->default_sequence->end ;
    }

  if (app->default_sequence)
    seq_map->dataset = app->default_sequence->dataset ;



  /* If request is to add this view to an existing one then look up the zmap to 
   * which this view should be added, if lookup fails must make a new window. */
  if (add_to_view)
    {
      ZMapView tmp_view ;

      if (!((tmp_view = (ZMapView)zMapUtilsStr2Ptr((char *)g_quark_to_string(add_to_view)))
	    && (zmap = (ZMap)g_hash_table_lookup(app->remote_control->id2zmap, tmp_view))))
	{
	  zMapLogCritical("Could not find view \"%s\" in view->zmap hash,"
			  " new view will be created in separate window.", (char *)g_quark_to_string(add_to_view)) ;
	}
    }


  if (!zmapAppCreateZMap(app, seq_map, &zmap, &view, &err_msg))
    {
      *command_rc_out = REMOTE_COMMAND_RC_FAILED ;
      *reason_out = err_msg ;
    }
  else
    {
      char *ptr_str ;

      /* Store the view/zmap in our hash table which maps view to which zmap they are
       * displayed in. */
      g_hash_table_insert(app->remote_control->view2zmap, view, zmap) ;

      /* Make a string version of the view pointer to use as an ID. */
      ptr_str = (char *)zMapUtilsPtr2Str((void *)view) ;
      view_reply[1].value.q = g_quark_from_string(ptr_str) ;
      *reply_out = &view_reply[0] ;
      g_free(ptr_str) ;

      *command_rc_out = REMOTE_COMMAND_RC_OK ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I WANT TO GET RID OF THIS.....CHECK WHERE THIS INFO STUFF IS REFERENCED.... */

  /* that screwy rabbit */
  g_string_append_printf(request_data->message, "%s", app->info->message) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Clean up. */
  if (sequence)

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    g_free(sequence) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



static void closeZMap(ZMapAppContext app_context,
		      GQuark view_id,
		      RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out)
{
  ZMapView view ;
  ZMap zmap = NULL ;


  if (!((view = zMapUtilsStr2Ptr((char *)g_quark_to_string(view_id)))
	&& (zmap = g_hash_table_lookup(app_context->remote_control->view2zmap, view))))

    {
      *command_rc_out = REMOTE_COMMAND_RC_FAILED ;
      *reason_out = g_strdup_printf("Could not find view %s so cannot close it.", g_quark_to_string(view_id)) ;
    }
  else
    {
      gboolean result = FALSE ;

      /* Remove the view->zmap mapping. */
      if (!g_hash_table_remove(app_context->remote_control->view2zmap, view))
	zMapLogCritical("Could not remove view \"%s\" from hash table mapping view -> zmap.", view_id) ;


      /* Delete the named view, if it's the last view in the zmap then destroy the zmap
       * too. */
      if (zMapNumViews(zmap) > 1)
	result = zMapManagerDestroyView(app_context->zmap_manager, zmap, view) ;
      else
	result = zMapManagerKill(app_context->zmap_manager, zmap) ;

      if (result)
	{
	  *reply_out = makeMessageElement("View deleted.") ;
	  *command_rc_out = REMOTE_COMMAND_RC_OK ;
	}
      else
	{
	  *command_rc_out = REMOTE_COMMAND_RC_FAILED ;
	  *reason_out = g_strdup_printf("Close of view %s failed.", g_quark_to_string(view_id)) ;
	}
    }

  return ;
}



/* Quite dumb routine to save repeating this all over the place. The caller
 * is completely responsible for memory managing message-text, it must
 * be around long enough for the xml string to be created, after that it
 * can be free'd. */
static ZMapXMLUtilsEventStack makeMessageElement(char *message_text)
{
  static ZMapXMLUtilsEventStackStruct
    message[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_MESSAGE,   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_CHAR_DATA_EVENT,     NULL,    ZMAPXML_EVENT_DATA_STRING, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_MESSAGE,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  message[1].value.s = message_text ;

  return &(message[0]) ;
}




/* all the action has been moved from <zmap> to <request> */
static gboolean start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;

  return handled ;
}

static gboolean end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;

  return handled;
}


static gboolean req_start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;

  return handled ;
}

static gboolean req_end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;
  RequestData request_data = (RequestData)user_data ;
  ZMapXMLElement child ;
  ZMapXMLAttribute attr ;
  char *err_msg = NULL ;



  if (strcmp(request_data->command_name, ZACP_NEWVIEW) == 0
      || strcmp(request_data->command_name, ZACP_ADD_TO_VIEW) == 0)
    {
      if (strcmp(request_data->command_name, ZACP_ADD_TO_VIEW) == 0)
	{
	  if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(element, ZACP_VIEWID)) != NULL))
	    request_data->add_to_view = zMapXMLAttributeGetValue(attr) ;
	  else
	    err_msg = g_strdup_printf("%s is a required tag.", ZACP_VIEWID) ;
	}

      if (!err_msg && ((child = zMapXMLElementGetChildByName(element, ZACP_SEQUENCE_TAG)) == NULL))
	err_msg = g_strdup_printf("%s is a required element.", ZACP_SEQUENCE_TAG) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_NAME)) != NULL))
	request_data->sequence = zMapXMLAttributeGetValue(attr) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_NAME) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_START)) != NULL))
	request_data->start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_START) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_END)) != NULL))
	request_data->end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_END) ;
    }
  else if (strcmp(request_data->command_name, ZACP_CLOSEVIEW) == 0)
    {
      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(element, ZACP_VIEWID)) != NULL))
	request_data->view_id = zMapXMLAttributeGetValue(attr) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_VIEWID) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (!err_msg && ((child = zMapXMLElementGetChildByName(element, ZACP_VIEW)) == NULL))
	err_msg = g_strdup_printf("%s is a required element.", ZACP_VIEW) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }


  if (err_msg)
    {
      request_data->command_rc = REMOTE_COMMAND_RC_BAD_XML ;

      zMapXMLParserRaiseParsingError(parser, err_msg) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_free(err_msg) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      handled = FALSE ;
    }

  return handled ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */









/* 
 * ALL THIS IS FROM THE OLD STUFF AND NEEDS REDOING FOR THE NEW INTERFACE....
 * 
 * 
 * 
 * 
 * 
 */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This should just be a filter command passing to the correct
   function defined by the action="value" of the request */
static char *application_execute_command(char *command_text,
					 gpointer app_context_data, int *statusCode, ZMapXRemoteObj owner)
{
  ZMapXMLParser parser;
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  char *xml_reply      = NULL;
  gboolean cmd_debug   = FALSE;
  gboolean parse_ok    = FALSE;
  RequestDataStruct request_data = {0};

  if (zMapXRemoteIsPingCommand(command_text, statusCode, &xml_reply) != 0)
    {
      goto HAVE_RESPONSE;
    }

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers_G, end_handlers_G);

  if ((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      ResponseContextStruct response_data = {0} ;

      response_data.code    = ZMAPXREMOTE_INTERNAL; /* unknown command if this isn't changed */
      response_data.message = g_string_sized_new(256);

      switch(request_data.action)
        {
        case ZMAPAPP_REMOTE_OPEN_ZMAP:

          createZMap(app_context_data, &request_data, &response_data);
          if (app_context->info)
            response_data.code = app_context->info->code;
          break;
        case ZMAPAPP_REMOTE_CLOSE_ZMAP:
	  {
	    guint handler_id ;

	    /* Send a response to the external program that we got the CLOSE. */
	    g_string_append_printf(response_data.message, "zmap is closing, wait for finalised request.") ;
	    response_data.handled = TRUE ;
	    response_data.code = ZMAPXREMOTE_OK;

            /* Remove the notify handler. */
            if(app_context->property_notify_event_id)
              g_signal_handler_disconnect(app_context->app_widg, app_context->property_notify_event_id);
            app_context->property_notify_event_id = 0 ;

	    /* Attach an idle handler from which we send the final quit, we must do this because
	     * otherwise we end up exitting before the external program sees the final quit. */
	    handler_id = g_idle_add(finalExit, app_context) ;
	  }
          break;
        case ZMAPAPP_REMOTE_UNKNOWN:
        case ZMAPAPP_REMOTE_INVALID:
        default:
          response_data.code = ZMAPXREMOTE_UNKNOWNCMD;
          g_string_append_printf(response_data.message, "Unknown command");
          break;
        }

      *statusCode = response_data.code;
      xml_reply   = g_string_free(response_data.message, FALSE);
    }
  else
    {
      *statusCode = ZMAPXREMOTE_BADREQUEST;
      xml_reply   = g_strdup(zMapXMLParserLastErrorMsg(parser));
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser);

 HAVE_RESPONSE:
  if(!zMapXRemoteValidateStatusCode(statusCode) && xml_reply != NULL)
    {
      zMapLogWarning("%s", xml_reply);
      g_free(xml_reply);
      xml_reply = g_strdup("Broken code. Check zmap.log file");
    }
  if(xml_reply == NULL){ xml_reply = g_strdup("Broken code."); }

  return xml_reply;
}

static void createZMap(ZMapAppContext app, RequestData request_data, ResponseContext response_data)
{
  char *sequence ;
  ZMapFeatureSequenceMap seq_map ;

  sequence = g_strdup(g_quark_to_string(request_data->sequence)) ;

  /* MH17: this is a bodge FTM, we need a dataset XRemote field as well */
  /* default sequence may be NULL */
  seq_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;
  if(app->default_sequence)
        seq_map->dataset = app->default_sequence->dataset ;
  seq_map->sequence = sequence ;
  seq_map->start = request_data->start ;
  seq_map->end = request_data->end ;

  zmapAppCreateZMap(app, seq_map) ;

  response_data->handled = TRUE ;

  /* that screwy rabbit */
  g_string_append_printf(response_data->message, "%s", app->info->message) ;

  /* Clean up. */
  if (sequence)
    g_free(sequence) ;

  return ;
}

static void send_finalised(ZMapXRemoteObj client)
{
  char *request = "<zmap>\n"
                  "  <request action=\"finalised\" />\n"
                  "</zmap>" ;
  char *response = NULL;

  g_return_if_fail(client != NULL);

  /* Send the final quit, after this we can exit. */
  if (zMapXRemoteSendRemoteCommand(client, request, &response) != ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
    {
      response = response ? response : zMapXRemoteGetResponse(client);
      zMapLogWarning("Final Quit to client program failed: \"%s\"", response) ;
    }

  return ;
}

/* A GSourceFunc() called from an idle handler to do the final clear up. */
static gboolean finalExit(gpointer data)
{
  ZMapAppContext app_context = (ZMapAppContext)data ;

  /* Signal zmap we want to exit now. */
  zmapAppExit(app_context) ;

  return FALSE ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




