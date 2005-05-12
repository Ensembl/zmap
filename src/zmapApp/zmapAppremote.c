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
 * Last edited: May 12 16:37 2005 (rds)
 * Created: Thu May  5 18:19:30 2005 (rds)
 * CVS info:   $Id: zmapAppremote.c,v 1.2 2005-05-12 15:45:35 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapApp_P.h>
#include <ZMap/zmapXRemote.h>


static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode);
static char *createZMap(ZMapAppContext app, char *command_text);
static void destroyNotifyData(gpointer destroy_data);

void zmapAppRemoteInstaller(GtkWidget *widget, gpointer app_context_data)
{
  ZMapAppContext app_context = (ZMapAppContext)app_context_data;
  zMapXRemoteObj xremote;
  
  if((xremote = zMapXRemoteNew()) != NULL)
    {
      Window id;
      zMapXRemoteNotifyData notifyData;
      
      id = (Window)GDK_DRAWABLE_XID(widget->window);

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
  
  return;
}

static char *appexecuteCommand(char *command_text, gpointer app_context, int *statusCode){
  char *xml_reply     = NULL;

  if(g_str_has_prefix(command_text, "newZmap"))
    {
      *statusCode = ZMAPXREMOTE_OK;
      xml_reply   = createZMap((ZMapAppContext) app_context, command_text);
    }
  else
    {
      *statusCode = ZMAPXREMOTE_UNKNOWNCMD;
      xml_reply   = "<message>unknown command</message>";
    }
  return xml_reply;
}


static char *createZMap(ZMapAppContext app, char *command_text)
{
  char *result = NULL ;
  gboolean parse_error ;
  char *next ;
  char *keyword, *value ;
  gchar *type = NULL ;
  gint start = 0, end = 0;
  char *sequence;

  next = strtok(command_text, " ") ;			    /* Skip newZmap. */

  parse_error = FALSE ;
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
	  result = "<message>failed to parse command.</message>";
	}

      g_free(keyword) ;
      g_free(value) ;
    }

  if (!result)
    zmapAppCreateZMap(app, sequence, start, end) ;

  /* Clean up. */
  if (sequence)
    g_free(sequence) ;

  return result ;
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





