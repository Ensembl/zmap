/*  File: zmapAppRemote.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: None
 * HISTORY:
 * Last edited: Jun 15 09:54 2011 (edgrif)
 * Created: Thu May  5 18:19:30 2005 (rds)
 * CVS info:   $Id: zmapAppremote.c,v 1.44 2011-05-06 14:52:20 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <zmapApp_P.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtilsXRemote.h>

typedef enum {
  ZMAP_APP_REMOTE_ALL = 1
} appObjType;

/* Requests that come to us from an external program. */
typedef enum
  {
    ZMAPAPP_REMOTE_INVALID,
    ZMAPAPP_REMOTE_OPEN_ZMAP,				    /* Open a new zmap window. */
    ZMAPAPP_REMOTE_CLOSE_ZMAP,				    /* Close a window. */
    ZMAPAPP_REMOTE_UNKNOWN
  } ZMapAppValidXRemoteAction;

typedef enum {RUNNING_ZMAPS, KILLING_ALL_ZMAPS = 1} ZMapAppContextState;

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


static char *application_execute_command(char *command_text, gpointer app_context, int *statusCode,ZMapXRemoteObj owner);
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

/* Installs the handlers to monitor/handle requests to/from an external program. */
void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  ZMapCmdLineArgsType value ;

  /* should we be doing this if no win_id specified (see next call) ????? CHECK THIS OUT.... */
  zMapXRemoteInitialiseWidget(widget, PACKAGE_NAME,
                              ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                              ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                              application_execute_command, app_context_data) ;


  /* Set ourselves up to send requests to an external program if we are given their window id. */
  if (!zMapCmdLineArgsValue(ZMAPARG_WINDOW_ID, &value))
    {
      zMapLogMessage("%s", "--win_id option not specified.") ;
    }
  else
    {
      unsigned long clientId = 0;
      char *win_id = NULL;
      win_id   = value.s ;
      clientId = strtoul(win_id, (char **)NULL, 16);

      if (clientId)
        {
          ZMapXRemoteObj client = NULL;

          if ((app_context->xremote_client = client = zMapXRemoteNew(GDK_DISPLAY())) != NULL)
            {
              Window id = (Window)GDK_DRAWABLE_XID(widget->window);
              char *req = NULL, *resp = NULL;
              int ret_code = 0;

              zMapXRemoteInitClient(client, clientId);

              zMapXRemoteSetRequestAtomName(client, ZMAP_CLIENT_REQUEST_ATOM_NAME);

              zMapXRemoteSetResponseAtomName(client, ZMAP_CLIENT_RESPONSE_ATOM_NAME);

              req = g_strdup_printf("<zmap>\n"
				    "  <request action=\"register_client\">\n"
                                    "    <client xwid=\"0x%lx\" request_atom=\"%s\" response_atom=\"%s\" >\n"
                                    "      <action>%s</action>\n"
                                    "      <action>%s</action>\n"
                                    "    </client>\n"
				    "  </request>\n"
                                    "</zmap>",
                                    id,
                                    ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                                    ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                                    actions_G[ZMAPAPP_REMOTE_OPEN_ZMAP],
                                    actions_G[ZMAPAPP_REMOTE_CLOSE_ZMAP]);

              if ((ret_code = zMapXRemoteSendRemoteCommand(client, req, &resp)) != 0)
                {
                  zMapLogWarning("Could not communicate with client '0x%lx'. code %d", clientId, ret_code) ;

                  if (resp)
                    {
                      zMapLogWarning("Full response was %s", resp);
                      g_free(resp);
                    }
                  else
		    {
		      zMapLogWarning("%s", "No response was received!");
		    }

                  app_context->xremote_client = NULL ;
                  zMapXRemoteDestroy(client) ;
                }

	      g_free(req);
            }
        }
      else
        {
          zMapLogWarning("%s", "Failed to get window id.");
        }
    }

    return ;
}

void zmapAppRemoteSendFinalised(ZMapAppContext app_context)
{
  if(app_context->xremote_client && app_context->sent_finalised == FALSE)
    {
      send_finalised(app_context->xremote_client);
      app_context->sent_finalised = TRUE;
    }

  return ;
}

/* This should just be a filter command passing to the correct
   function defined by the action="value" of the request */
static char *application_execute_command(char *command_text, gpointer app_context_data, int *statusCode, ZMapXRemoteObj owner)
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
