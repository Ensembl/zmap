/*  File: zmapAppRemote.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Last edited: Sep 20 12:27 2005 (rds)
 * Created: Thu May  5 18:19:30 2005 (rds)
 * CVS info:   $Id: zmapAppremote.c,v 1.12 2005-09-20 17:21:38 rds Exp $
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

/* This should be somewhere else ... 
   or we should be making other objects */
typedef struct 
{
  appValidActions action;
  GQuark sequence;
  GQuark start;
  GQuark end;
  GQuark source;
} appRemoteAllStruct, *appRemoteAll;

static zmapXMLFactory openFactory(void);
static gboolean start(void *userData, 
                      zmapXMLElement element, 
                      zmapXMLParser parser);
static gboolean end(void *userData, 
                    zmapXMLElement element, 
                    zmapXMLParser parser);

static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode);
static gboolean createZMap(ZMapAppContext app, appRemoteAll obj);
static void destroyNotifyData(gpointer destroy_data);

void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  zMapXRemoteObj xremote;
  ZMapCmdLineArgsType value ;
  Window id;
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
      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                            (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                            );

      app_context->propertyNotifyData = notifyData; /* So we can free it */
    }

  if (zMapCmdLineArgsValue(ZMAPARG_WINDOW_ID, &value))
    {
      unsigned long l_id = 0;
      char *win_id       = NULL;
      win_id = value.s ;
      l_id   = strtoul(win_id, (char **)NULL, 16);
      if(l_id)
        {
          zMapXRemoteObj client = NULL;
          if((client = zMapXRemoteNew()) != NULL)
            {
              char *req = NULL;
              req = g_strdup_printf("<zmap action=\"register_client\"><client xwid=\"0x%lx\" request_atom=\"%s\" response_atom=\"%s\" /></zmap>",
                                    id,
                                    ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                                    ZMAP_DEFAULT_RESPONSE_ATOM_NAME
                                    );
              zMapXRemoteInitClient(client, l_id);
              zMapXRemoteSetRequestAtomName(client, ZMAP_CLIENT_REQUEST_ATOM_NAME);
              zMapXRemoteSetResponseAtomName(client, ZMAP_CLIENT_RESPONSE_ATOM_NAME);
              zMapXRemoteSendRemoteCommand(client, req);
              if(req)
                g_free(req);
            }
        }

    }

  
  return;
}
/* This should just be a filter command passing to the correct
   function defined by the action="value" of the request */
static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode){
  ZMapAppContext app   = (ZMapAppContext)app_context;
  char *xml_reply      = NULL;
  int code             = ZMAPXREMOTE_INTERNAL; /* unknown command if this isn't changed */
  zmapXMLParser parser = NULL;
  gboolean cmd_debug   = FALSE;
  gboolean parse_ok    = FALSE;
  GList *list          = NULL;
  zmapXMLFactory factory = NULL;

  g_clear_error(&(app->info));

  factory = openFactory();
  parser  = zMapXMLParser_create(factory, FALSE, cmd_debug);
  zMapXMLParser_setMarkupObjectHandler(parser, start, end);
  parse_ok = zMapXMLParser_parseBuffer(parser, command_text, strlen(command_text));

  if(parse_ok && 
     zMapXMLFactoryLite_decodeNameQuark(factory, g_quark_from_string("zmap"), &list) > 0 
     && !(list == NULL))
    {
      appRemoteAll appOpen = (appRemoteAll)(list->data);

      switch(appOpen->action){
      case ZMAP_APP_REMOTE_OPEN_ZMAP:
        if(createZMap(app, appOpen))
          code = ZMAPXREMOTE_OK;
        else
          code = ZMAPXREMOTE_BADREQUEST;
        break;
      case ZMAP_APP_REMOTE_CLOSE_ZMAP:
        printf("Doesn't do this yet\n");
        break;
      default:
        break;
      }
    }
  else if(!parse_ok)
    {
      code = ZMAPXREMOTE_BADREQUEST;
      app->info = 
        g_error_new(g_quark_from_string(__FILE__),
                    code,
                    "%s",
                    zMapXMLParser_lastErrorMsg(parser)
                    );      
    }

  /* Free the parser!!! */
  /* zMapXMLParser_free(parser); */

  /* Now check what the status is */
  switch (code)
    {
    case ZMAPXREMOTE_INTERNAL:
      {
        code = ZMAPXREMOTE_UNKNOWNCMD;    
        app->info || (app->info = 
                      g_error_new(g_quark_from_string(__FILE__),
                                  code,
                                  "<!-- request was %s -->%s",
                                  command_text,
                                  "unknown command"
                                  ));
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        app->info || ( app->info = 
                       g_error_new(g_quark_from_string(__FILE__),
                                   code,
                                   "<!-- request was %s -->%s",
                                   command_text,
                                   "bad request"
                                   ));
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        app->info = 
          g_error_new(g_quark_from_string(__FILE__),
                      code,
                      "%s",
                      "forbidden request"
                      );
        break;
      }
    default:
      {
        /* If the info isn't set then someone forgot to set it */
        if(!app->info)
          {
            code = ZMAPXREMOTE_INTERNAL;
            app->info = 
              g_error_new(g_quark_from_string(__FILE__),
                          code,
                          "<!-- request was %s -->%s",
                          command_text,
                          "CODE error on the part of the zmap programmers."
                          );
          }
        break;
      }
    }
  

  /* We should have a info object by now, pass it on. */
  xml_reply   = g_strdup(app->info->message);
  *statusCode = code;

  return xml_reply;
}


static gboolean createZMap(ZMapAppContext app, appRemoteAll obj)
{
  gboolean executed = FALSE;
  char *sequence    = g_strdup(g_quark_to_string(obj->sequence));

  zmapAppCreateZMap(app, sequence, obj->start, obj->end) ;
  executed = TRUE;
  
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
  /* user_data->data points to parent
   * hold onto this parent.
   * cleanup user_data = NULL;
   * free(user_data)
   */
  return ;
}



static gboolean start(void *userData, 
                      zmapXMLElement element, 
                      zmapXMLParser parser)
{
  zmapXMLFactory factory = (zmapXMLFactory)userData;
  gboolean handled  = FALSE;
  appObjType type;

  type  = (appObjType)zMapXMLFactoryLite_decodeElement(factory, element, NULL);

  switch(type){
  case ZMAP_APP_REMOTE_ALL:
    {
      zmapXMLAttribute attr = NULL;
      appRemoteAll objAll   = g_new0(appRemoteAllStruct, 1);
      if((attr = zMapXMLElement_getAttributeByName(element, "action")) != NULL)
        {
          GQuark action = zMapXMLAttribute_getValue(attr);
          if(action == g_quark_from_string("new"))
            objAll->action = ZMAP_APP_REMOTE_OPEN_ZMAP;
          if(action == g_quark_from_string("close"))
            objAll->action = ZMAP_APP_REMOTE_CLOSE_ZMAP;
        }
      /* Add obj to list */
      zMapXMLFactoryLite_addOutput(factory, element, objAll);
    }
    break;
  default:
    break;
  }
  return handled;
}
static gboolean end(void *userData, 
                    zmapXMLElement element, 
                    zmapXMLParser parser)
{
  zmapXMLFactory factory = (zmapXMLFactory)userData;
  GList *list      = NULL;
  gboolean handled = FALSE;
  appObjType type  = (appObjType)zMapXMLFactoryLite_decodeElement(factory, 
                                                                 element, 
                                                                 &list);
  switch(type){
  case ZMAP_APP_REMOTE_ALL:
    {
      zmapXMLElement child = NULL;
      appRemoteAll appOpen = (appRemoteAll)(list->data);
      if((child = zMapXMLElement_getChildByName(element, "segment")) != NULL)
        {
          zmapXMLAttribute attr = NULL;
          if((attr = zMapXMLElement_getAttributeByName(child, "sequence")) != NULL)
            appOpen->sequence = zMapXMLAttribute_getValue(attr);
          if((attr = zMapXMLElement_getAttributeByName(child, "start")) != NULL)
            appOpen->start = strtol(g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
          else
            appOpen->start = 1;
          if((attr = zMapXMLElement_getAttributeByName(child, "end")) != NULL)
            appOpen->end = strtol(g_quark_to_string(zMapXMLAttribute_getValue(attr)), (char **)NULL, 10);
          else
            appOpen->end = 0;
        }
      if((child = zMapXMLElement_getChildByName(element, "segment")) != NULL)
        appOpen->source = g_quark_from_string( child->contents->str );
    }
    handled = TRUE;
    break;
  default:
    break;
  }
  return handled;
}

static zmapXMLFactory openFactory(void)
{
  zmapXMLFactory factory = NULL;
  zmapXMLFactoryLiteItem zmap = NULL;

  factory = zMapXMLFactory_create(TRUE);
  zmap    = zMapXMLFactoryLiteItem_create(ZMAP_APP_REMOTE_ALL);
  
  zMapXMLFactoryLite_addItem(factory, zmap, "zmap");

  return factory;
}
