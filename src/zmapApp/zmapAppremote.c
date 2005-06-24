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
 * Last edited: Jun 24 08:45 2005 (edgrif)
 * Created: Thu May  5 18:19:30 2005 (rds)
 * CVS info:   $Id: zmapAppremote.c,v 1.5 2005-06-24 13:17:26 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapApp_P.h>
#include <ZMap/zmapCmdLineArgs.h>


static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode);
static gboolean createZMap(ZMapAppContext app, char *command_text);
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
              req = g_strdup_printf("register_client id = 0x%lx ; request = %s ; response = %s;",
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
   function defined by the primary word of the request */
static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode){
  ZMapAppContext app  = (ZMapAppContext)app_context;
  char *xml_reply     = NULL;
  int code            = ZMAPXREMOTE_INTERNAL; /* unknown command if this isn't changed */

  g_clear_error(&(app->info));

  /* check command for primary word */
  if(g_str_has_prefix(command_text, "newZmap"))
    {
      if(createZMap(app, command_text))
        code = ZMAPXREMOTE_OK;
      else
        code = ZMAPXREMOTE_BADREQUEST;
    }
  else if(g_str_has_prefix(command_text, "closeZmap"))
    {
      code = ZMAPXREMOTE_FORBIDDEN;
      //      closeZMap(app, command_text);
    }

  /* Now check what the status is */
  switch (code)
    {
    case ZMAPXREMOTE_INTERNAL:
      {
        code = ZMAPXREMOTE_UNKNOWNCMD;    
        app->info = 
          g_error_new(g_quark_from_string(__FILE__),
                      code,
                      "<request>%s</request><message>%s</message>",
                      command_text,
                      "unknown command"
                      );
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        app->info = 
          g_error_new(g_quark_from_string(__FILE__),
                      code,
                      "<request>%s</request><message>%s</message>",
                      command_text,
                      "bad request"
                      );
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        app->info = 
          g_error_new(g_quark_from_string(__FILE__),
                      code,
                      "<request>%s</request><message>%s</message>",
                      command_text,
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
            app->info || (app->info = 
              g_error_new(g_quark_from_string(__FILE__),
                          code,
                          "<request>%s</request><message>%s</message>",
                          command_text,
                          "CODE error on the part of the zmap programmers."
                          ));
          }
        break;
      }
    }
  

  /* We should have a info object by now, pass it on. */
  xml_reply   = g_strdup(app->info->message);
  *statusCode = code;

  return xml_reply;
}


static gboolean createZMap(ZMapAppContext app, char *command_text)
{
  gboolean parse_error, executed ;
  char *next ;
  char *keyword, *value ;
  gchar *type = NULL ;
  gint start = 0, end = 0;
  char *sequence;

  next = strtok(command_text, " ") ;			    /* Skip newZmap. */

  executed = parse_error = FALSE ;
  while (!parse_error && (next = strtok(NULL, " ")))
    {
      keyword = g_strdup(next) ;			    /* Get keyword. */

      next = strtok(NULL, " ") ;			    /* skip "=" */

      next = strtok(NULL, " ") ;			    /* Get value. */
      value = g_strdup(next) ;

      next = strtok(NULL, " ") ;			    /* skip ";" */


      if (g_ascii_strcasecmp(keyword, "seq") == 0)
        {
          sequence = g_strdup(value) ;
        }
      else if (g_ascii_strcasecmp(keyword, "start") == 0)
	{
          start = strtol(value, (char **)NULL, 10) ;
	}
      else if (g_ascii_strcasecmp(keyword, "end") == 0)
        {
          end = strtol(value, (char **)NULL, 10) ;
        }
      else
	{
	  parse_error = TRUE ;
	}

      g_free(keyword) ;
      g_free(value) ;
    }

  if (!parse_error)
    {
      zmapAppCreateZMap(app, sequence, start, end) ;
      executed = TRUE;
    }

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





