/*  File: zmapAppRemoteControl.c
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Application level functions to set up ZMap remote control
 *              to communicate with a peer program.
 *
 * Exported functions: See zmapApp_P.h
 * HISTORY:
 * Last edited: Dec  1 15:44 2011 (edgrif)
 * Created: Mon Nov  1 10:32:56 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapRemoteControl.h>
#include <zmapApp_P.h>



static gboolean requestHandlerCB(ZMapRemoteControl remote_control,
				 ZMapRemoteControlCallWithReplyFunc remote_reply_func, void *remote_reply_data,
				 void *request, void *user_data) ;
static gboolean replyHandlerCB(ZMapRemoteControl remote_control, void *reply, void *user_data) ;
static gboolean timeoutHandlerFunc(ZMapRemoteControl remote_control, void *user_data) ;

static char *getHelloMessage(char *unique_id) ;



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
  GQuark sequence;
  GQuark start;
  GQuark end;
  GQuark source;
} RequestDataStruct, *RequestData;

typedef struct
{
  int code;
  gboolean handled;
  GString *message;
}ResponseContextStruct, *ResponseContext;





static char *application_execute_command(char *command_text, gpointer app_context,
					 int *statusCode, ZMapXRemoteObj owner);
static gboolean start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);

static void createZMap(ZMapAppContext app, RequestData request_data, ResponseContext response);
static void send_finalised(ZMapXRemoteObj client);
static gboolean finalExit(gpointer data) ;

static ZMapXMLObjTagFunctionsStruct start_handlers_G[] = {
  { "zmap", start },
  { "request", req_start },
  { NULL,   NULL  }
};
static ZMapXMLObjTagFunctionsStruct end_handlers_G[] = {
  { "zmap",    end  },
  { "request", req_end  },
  { NULL,   NULL }
};

static char *actions_G[ZMAPAPP_REMOTE_UNKNOWN + 1] = {
  NULL, "new_zmap", "shutdown", NULL
};











/* Set up the remote controller object. */
gboolean zmapAppRemoteControlCreate(ZMapAppContext app_context)
{
  gboolean result = FALSE ;

  app_context->app_id = g_strdup(zMapGetAppName()) ;
  app_context->app_unique_id = zMapMakeUniqueID(app_context->app_id) ;

  if ((app_context->remote_controller = zMapRemoteControlCreate(app_context->app_id,
								NULL, NULL)))
    {
      /* set no timeout for now... */
      zMapRemoteControlSetTimeout(app_context->remote_controller, 0) ;

      result = TRUE ;
    }
  else
    {
      zMapCritical("%s", "Initialisation of remote control failed.") ;

      result = FALSE ;
    }

  return result ;
}


/* We assume that all is good here, i.e. we have a remote id etc. etc. */
gboolean zmapAppRemoteControlInit(ZMapAppContext app_context)
{
  gboolean result = FALSE ;

  if ((result = zMapRemoteControlSelfInit(app_context->remote_controller,
					  app_context->app_unique_id,
					  requestHandlerCB, app_context,
					  timeoutHandlerFunc, app_context
					  )))
    {
      result = zMapRemoteControlSelfWaitForRequest(app_context->remote_controller) ;
    }


  if (result)
    {
      result = zMapRemoteControlPeerInit(app_context->remote_controller,
					 app_context->peer_unique_id,
					 replyHandlerCB, app_context,
					 timeoutHandlerFunc, app_context) ;
    }

  return result ;
}


/* We need to contact the peer and get their id to establish two way communication. Note
 * this will happen asychronously.... */
gboolean zmapAppRemoteControlConnect(ZMapAppContext app_context)
{
  gboolean result = FALSE ;
  char *hello_msg ;

  /* NOTE THAT WE NEED A CALL HERE TO CREATE THE XML MESSAGE TO GO TO THE PEER.... */
  hello_msg = getHelloMessage(app_context->app_unique_id) ;

  if (!(result = zMapRemoteControlPeerRequest(app_context->remote_controller, hello_msg)))
    zMapLogCritical("%s", "Cannot initialise zmap remote control interface.") ;

  return result ;
}





/* 
 *                Internal functions
 */


static gboolean requestHandlerCB(ZMapRemoteControl remote_control, ZMapRemoteControlCallWithReplyFunc remote_reply_func, void *remote_reply_data,
				 void *request, void *user_data)
{
  gboolean result = TRUE ;
  char *test_data = "Test reply from ZMap." ;

  /* this is where we need to process the zmap commands perhaps passing on the remote_reply_func
   * and data to be called from a callback for an asynch. request.
   * 
   * For now we just call straight back for testing.... */
  result = (remote_reply_func)(remote_reply_data, test_data, strlen(test_data) + 1) ;


  return result ;
}


static gboolean replyHandlerCB(ZMapRemoteControl remote_control, void *request, void *user_data)
{
  gboolean result = TRUE ;




  return result ;
}




static gboolean timeoutHandlerFunc(ZMapRemoteControl remote_control, void *user_data)
{
  gboolean result = FALSE ;




  return result ;
}



static char *getHelloMessage(char *unique_id)
{
  char *hello_message = NULL ;


  /* We need an xml writer that will add version numbers and request numbers etc. etc... */
  /* Will need more than this..... */
  hello_message = g_strdup_printf("<zmap version=\"1.0\">\n"
				  "  <request id=\"1\", action=\"register_client\">\n"
				  "    <client remote_id=\"%s\">\n"
				  "    </client>\n"
				  "  </request>\n"
				  "</zmap>",
				  unique_id) ;

  return hello_message ;
}





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

  if(zMapXRemoteIsPingCommand(command_text, statusCode, &xml_reply) != 0)
    {
      goto HAVE_RESPONSE;
    }

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers_G, end_handlers_G);

  if ((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      ResponseContextStruct response_data = {0};

      response_data.code    = ZMAPXREMOTE_INTERNAL; /* unknown command if this isn't changed */
      response_data.message = g_string_sized_new(256);

      switch(request_data.action)
        {
        case ZMAPAPP_REMOTE_OPEN_ZMAP:
          createZMap(app_context_data, &request_data, &response_data);
          if(app_context->info)
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
  char *sequence = g_strdup(g_quark_to_string(request_data->sequence));
  ZMapFeatureSequenceMap seq_map = g_new0(ZMapFeatureSequenceMapStruct,1);

  /* MH17: this is a bodge FTM, we need a dataset XRemote field as well */
  // default sequence may be NULL
  if(app->default_sequence)
        seq_map->dataset = app->default_sequence->dataset;
  seq_map->sequence = sequence;
  seq_map->start = request_data->start;
  seq_map->end = request_data->end;

  zmapAppCreateZMap(app, seq_map) ;

  response_data->handled = TRUE;

  /* that screwy rabbit */
  g_string_append_printf(response_data->message, "%s", app->info->message);

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


/* all the action has been moved from <zmap> to <request> */
static gboolean start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  RequestData request_data = (RequestData)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return handled ;
}

static gboolean end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  RequestData request_data = (RequestData)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return handled;
}


static gboolean req_start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;
  RequestData request_data = (RequestData)user_data;
  ZMapXMLAttribute attr = NULL;

  if ((attr = zMapXMLElementGetAttributeByName(element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);

      if (action == g_quark_from_string(actions_G[ZMAPAPP_REMOTE_OPEN_ZMAP]))
	{
	  request_data->action = ZMAPAPP_REMOTE_OPEN_ZMAP ;
	  handled = TRUE ;
	}
      else if (action == g_quark_from_string(actions_G[ZMAPAPP_REMOTE_CLOSE_ZMAP]))
	{
	  request_data->action = ZMAPAPP_REMOTE_CLOSE_ZMAP ;
	  handled = TRUE ;
	}
    }
  else
    zMapXMLParserRaiseParsingError(parser, "Attribute 'action' is required for element 'request'.");

  return handled ;
}

static gboolean req_end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;
  RequestData request_data = (RequestData)user_data;
  ZMapXMLElement child ;
  ZMapXMLAttribute attr;

  if ((child = zMapXMLElementGetChildByName(element, "segment")) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(child, "sequence")) != NULL)
        request_data->sequence = zMapXMLAttributeGetValue(attr);

      if((attr = zMapXMLElementGetAttributeByName(child, "start")) != NULL)
        request_data->start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      else
        request_data->start = 1;

      if((attr = zMapXMLElementGetAttributeByName(child, "end")) != NULL)
        request_data->end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      else
        request_data->end = 0;
    }

  return handled;
}



