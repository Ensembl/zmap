/*  File: zmapAppRemote.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: None
 * HISTORY:
 * Last edited: Mar  7 17:12 2007 (rds)
 * Created: Thu May  5 18:19:30 2005 (rds)
 * CVS info:   $Id: zmapAppremote.c,v 1.24 2007-03-08 11:47:46 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapApp_P.h>
#include <ZMap/zmapXML.h>
#include <ZMap/zmapCmdLineArgs.h>
#include <ZMap/zmapConfigDir.h>

typedef enum {
  ZMAP_APP_REMOTE_ALL = 1
} appObjType;

typedef enum {
  ZMAP_APP_REMOTE_UNKNOWN,
  ZMAP_APP_REMOTE_OPEN_ZMAP,
  ZMAP_APP_REMOTE_CLOSE_ZMAP
} appValidActions;

typedef enum {KILLING_ALL_ZMAPS = 1} ZMapAppContextState;

/* This should be somewhere else ... 
   or we should be making other objects */
typedef struct 
{
  appValidActions action;
  GQuark sequence;
  GQuark start;
  GQuark end;
  GQuark source;
} AppRemoteAllStruct, *AppRemoteAll;

typedef struct
{
  ZMapAppContext app_context;
  int code;
  gboolean handled;
  GString *message;
}ResponseContextStruct, *ResponseContext;

typedef struct
{
  GTimer *timer;
  ZMapAppContext app_context;
  ZMapAppContextState state_flag;
} TimerContextStruct, *TimerContext;

static gboolean start(void *userData, 
                      ZMapXMLElement element, 
                      ZMapXMLParser parser);
static gboolean end(void *userData, 
                    ZMapXMLElement element, 
                    ZMapXMLParser parser);

static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode);
static gboolean createZMap(ZMapAppContext app, AppRemoteAll obj, ResponseContext response);
static void destroyNotifyData(gpointer destroy_data);

/* closing from remote */
static gboolean closingTimedOut(GTimer *timer);
static gboolean setupRemoteCloseHandler(ZMapAppContext app_context, AppRemoteAll request, ResponseContext response);
static gboolean remoteCloseHandler(gpointer data);
static gboolean confirmCloseRequestResponseReceived(ZMapAppContext app_context);
static void remoteCloseDestroyNotify(gpointer data);


void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  zMapXRemoteObj xremote;
  ZMapCmdLineArgsType value ;
  Window id;
  zMapAssert(GTK_WIDGET_REALIZED(widget));

  id = (Window)GDK_DRAWABLE_XID(widget->window);
  
  if((xremote = zMapXRemoteNew()) != NULL)
    {
      zMapXRemoteNotifyData notifyData;

      notifyData           = g_new0(zMapXRemoteNotifyDataStruct, 1);
      notifyData->xremote  = xremote;
      notifyData->callback = ZMAPXREMOTE_CALLBACK(appexecuteCommand);
      notifyData->data     = app_context_data; 
      
      zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, ZMAP_DEFAULT_REQUEST_ATOM_NAME, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
      zMapConfigDirWriteWindowIdFile(id, "main");

      /* Makes sure we actually get the events!!!! Use add-events as set_events needs to be done BEFORE realize */
      gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK) ;
      /* probably need g_signal_connect_data here so we can destroy when disconnected */
      app_context->propertyNotifyEventId =
        g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                              G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                              (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                              );

      app_context->propertyNotifyData = notifyData; /* So we can free it */
    }

  if (zMapCmdLineArgsValue(ZMAPARG_WINDOW_ID, &value))
    {
      unsigned long clientId = 0;
      char *win_id = NULL;
      win_id   = value.s ;
      clientId = strtoul(win_id, (char **)NULL, 16);
      if(clientId)
        {
          zMapXRemoteObj client = NULL;
          if((app_context->xremoteClient = client = zMapXRemoteNew()) != NULL)
            {
              char *req = NULL;
              int ret_code = 0;
              req = g_strdup_printf("<zmap action=\"register_client\"><client xwid=\"0x%lx\" request_atom=\"%s\" response_atom=\"%s\" /></zmap>",
                                    id,
                                    ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                                    ZMAP_DEFAULT_RESPONSE_ATOM_NAME
                                    );
              zMapXRemoteInitClient(client, clientId);
              zMapXRemoteSetRequestAtomName(client, ZMAP_CLIENT_REQUEST_ATOM_NAME);
              zMapXRemoteSetResponseAtomName(client, ZMAP_CLIENT_RESPONSE_ATOM_NAME);
              if((ret_code = zMapXRemoteSendRemoteCommand(client, req, NULL)) != 0)
                {
                  zMapLogWarning("Could not communicate with client '0x%lx'. code %d", clientId, ret_code);
                  app_context->xremoteClient = NULL;
                  zMapXRemoteDestroy(client);
                }
              if(req)
                g_free(req);
            }
        }
      else
        {
          zMapLogWarning("%s", "Failed to get window id.");
        }
    }
  else
    zMapLogWarning("%s", "--win_id option not specified.");
  
  return;
}

static gboolean confirmCloseRequestResponseReceived(ZMapAppContext app_context)
{
  gboolean received = TRUE;
  char *response = NULL;

  received = FALSE;
  
  if(app_context->xremoteClient)
    {
      char *request = "<zmap action=\"finalised\" />";
      zMapXRemoteSendRemoteCommand(app_context->xremoteClient, request, &response);
      received = TRUE;
      /* I'm not sure on the library call to get the response, did I write one?
       * Not super important though... */
    } 
  else
    received = TRUE;            /* This is the best we can do here. */
  
  return received;
}

static gboolean closingTimedOut(GTimer *timer)
{
  gdouble limit = 10.0, clock;    /* 10 secs */
  gulong microsecs;
  gboolean timed_out = FALSE;

  if((clock = g_timer_elapsed(timer, &microsecs)) &&
     clock > limit)
    timed_out = TRUE;

  return timed_out;
}

/* A GSourceFunc, should return FALSE when it is time to remove */
static gboolean remoteCloseHandler(gpointer data)
{
  ZMapAppContext app_context = NULL;
  TimerContext tc = (TimerContext)data;
  gboolean remove = FALSE;

  app_context = tc->app_context;

  if(tc->state_flag != KILLING_ALL_ZMAPS)
    {
      tc->state_flag = KILLING_ALL_ZMAPS;
      zMapManagerKillAllZMaps(app_context->zmap_manager);
    }
  else if( (remove = ( zMapManagerCount(app_context->zmap_manager) == 0 )) )
    remove = TRUE;
  else if(closingTimedOut(tc->timer))
    remove = TRUE;

  return !remove;
}

static void remoteCloseDestroyNotify(gpointer data)
{
  TimerContext tc = (TimerContext)data;
  ZMapAppContext app_context = NULL;

  app_context = tc->app_context;

  if(closingTimedOut(tc->timer) ||
     ((zMapManagerCount(app_context->zmap_manager) == 0) &&
      confirmCloseRequestResponseReceived(app_context)))
    {
      /* first clean up the TimerContext */
      g_timer_destroy(tc->timer);
      tc->app_context = NULL;
      g_free(tc);

      zmapAppExit(app_context) ;
    }

  return ;
}

static gboolean setupRemoteCloseHandler(ZMapAppContext  app_context,
                                        AppRemoteAll    request_data,
                                        ResponseContext response_data)
{
  TimerContext timer_data;
  guint timeoutId = 0, interval = 1000;
  gboolean handled = FALSE;

  /* possibly a good idea to check whether we have a client, although
   * it goes on latter before sending the request, but also to stop 
   * random close requests... i.e. zmap must have been started as
   * zmap --win_id 0xNNNNNNN
   */
  if(app_context->xremoteClient != NULL &&
     (timer_data = g_new0(TimerContextStruct, 1)))
    {
      timer_data->app_context = app_context;
      timer_data->timer       = g_timer_new();

      if((timeoutId = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE, 
                                         interval,
                                         remoteCloseHandler, 
                                         (gpointer)timer_data,
                                         remoteCloseDestroyNotify)))
        {
          if(app_context->propertyNotifyEventId)
            g_signal_handler_disconnect(app_context->app_widg, 
                                        app_context->propertyNotifyEventId);
          app_context->propertyNotifyEventId = timer_data->state_flag = 0;
          g_string_append_printf(response_data->message, "zmap is closing, wait for finalised request.");
          response_data->handled = handled = TRUE;
        }
    }

  return handled;
}

/* This should just be a filter command passing to the correct
   function defined by the action="value" of the request */
static char *appexecuteCommand(char *command_text, gpointer app_context_data, int *statusCode)
{
  ZMapXMLParser parser;
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  char *xml_reply      = NULL;
  gboolean cmd_debug   = FALSE;
  gboolean parse_ok    = FALSE;
  AppRemoteAllStruct       request_data = {0};
  ResponseContextStruct   response_data = {0};
  ZMapXMLObjTagFunctionsStruct startH[] = {
    {"zmap", start},
    {NULL, NULL}
  };
  ZMapXMLObjTagFunctionsStruct endH[] = {
    {"zmap", end},
    {NULL, NULL}
  };

  parser  = zMapXMLParserCreate(&request_data, FALSE, cmd_debug);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, startH, endH);

  response_data.code = ZMAPXREMOTE_INTERNAL; /* unknown command if this isn't changed */
  response_data.message = g_string_sized_new(256);

  if((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      switch(request_data.action)
        {
        case ZMAP_APP_REMOTE_OPEN_ZMAP:
          if((response_data.handled = createZMap(app_context_data, &request_data, &response_data)))
            response_data.code = ZMAPXREMOTE_OK;
          else
            response_data.code = ZMAPXREMOTE_BADREQUEST;
          break;
        case ZMAP_APP_REMOTE_CLOSE_ZMAP:
          if((response_data.handled = setupRemoteCloseHandler(app_context, &request_data, &response_data)))
            response_data.code = ZMAPXREMOTE_OK;
          else
            {
              response_data.code = ZMAPXREMOTE_FORBIDDEN;
              g_string_append_printf(response_data.message,
                                     "closing zmap is not allowed via xremote");
            }
          break;
        default:
          break;
        }

      if(!(response_data.message->str) && response_data.handled)
        g_string_append_printf(response_data.message, 
                               "Request handled, but no message set. Probably a data error.");
      else if(!response_data.handled)
        zMapLogWarning("%s", "Request not handled!");
    }
  else
    {
      response_data.code = ZMAPXREMOTE_BADREQUEST;
      g_string_append_printf(response_data.message, "%s",
                             zMapXMLParserLastErrorMsg(parser));
    }

  /* Now check what the status is */
  switch (response_data.code)
    {
    case ZMAPXREMOTE_INTERNAL:
      {
        response_data.code = ZMAPXREMOTE_UNKNOWNCMD;    
        if(!response_data.message->str)
          g_string_append_printf(response_data.message,
                                 "<!-- request was %s -->%s",
                                 command_text, "unknown command");
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        if(!response_data.message->str)
          g_string_append_printf(response_data.message,
                                 "<!-- request was %s -->%s",
                                 command_text, "bad request");
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        if(!response_data.message->str)
          g_string_append_printf(response_data.message,
                                 "%s", "forbidden request");
        break;
      }
    default:
      {
        if(!response_data.message->str)
          {
            g_string_append_printf(response_data.message,
                                   "<!-- request was %s -->%s",
                                   command_text,
                                   "CODE error on the part of the zmap programmers.");
          }
        break;
      }
    }
  
  /* We should have a info object by now, pass it on. */
  xml_reply   = response_data.message->str;
  *statusCode = response_data.code;

  g_string_free(response_data.message, FALSE);

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser);

  return xml_reply;
}


static gboolean createZMap(ZMapAppContext app, AppRemoteAll obj, ResponseContext response_data)
{
  gboolean executed = FALSE;
  char *sequence    = g_strdup(g_quark_to_string(obj->sequence));

  zmapAppCreateZMap(app, sequence, obj->start, obj->end) ;

  response_data->handled = executed = TRUE;
  /* that screwy rabbit */
  g_string_append_printf(response_data->message, app->info->message);
  
  /* Clean up. */
  if (sequence)
    g_free(sequence) ;

  return executed;
}



static void destroyNotifyData(gpointer destroy_data)
{
  ZMapAppContext app;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData) destroy_data;

  app = destroy_me->data;

  app->propertyNotifyData = NULL;

  g_free(destroy_me);

  return ;
}



static gboolean start(void *user_data, 
                      ZMapXMLElement element, 
                      ZMapXMLParser parser)
{
  AppRemoteAll all_data = (AppRemoteAll)user_data;
  gboolean handled  = FALSE;
  ZMapXMLAttribute attr = NULL;
  
  if((attr = zMapXMLElementGetAttributeByName(element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      if(action == g_quark_from_string("new"))
        all_data->action = ZMAP_APP_REMOTE_OPEN_ZMAP;
      if(action == g_quark_from_string("close") || 
         action == g_quark_from_string("shutdown"))
        all_data->action = ZMAP_APP_REMOTE_CLOSE_ZMAP;
    }
  else
    zMapXMLParserRaiseParsingError(parser, "Attribute 'action' is required for element 'zmap'.");

  return handled;
}

static gboolean end(void *user_data, 
                    ZMapXMLElement element, 
                    ZMapXMLParser parser)
{
  AppRemoteAll all_data = (AppRemoteAll)user_data;
  gboolean handled      = TRUE;
  ZMapXMLElement child ;
  ZMapXMLAttribute attr;
  
  if((child = zMapXMLElementGetChildByName(element, "segment")) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(child, "sequence")) != NULL)
        all_data->sequence = zMapXMLAttributeGetValue(attr);

      if((attr = zMapXMLElementGetAttributeByName(child, "start")) != NULL)
        all_data->start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      else
        all_data->start = 1;

      if((attr = zMapXMLElementGetAttributeByName(child, "end")) != NULL)
        all_data->end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
      else
        all_data->end = 0;
    }

  return handled;
}
