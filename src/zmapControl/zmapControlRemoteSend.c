/*  File: zmapControlRemoteSend.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See zmapControl_P.h
 *              
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>



/* AS FAR AS I CAN TELL THIS CODE IS NOT USED ANYWHERE...HOW BIZARRE........ */


#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <zmapControl_P.h>



typedef struct
{
  GString *full_text;
  ZMap zmap;

  ZMapXMLParser parser;
  ZMapXMLObjTagFunctions start_handlers ;
  ZMapXMLObjTagFunctions end_handlers ;

  ZMapXMLTagHandler tag_handler ;
} SendClientCommandStruct, *SendClientCommand;


static void send_client_command(gpointer client_data, gpointer message_data);
static int  xml_event_to_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data);

/* xml event callbacks */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser);


static ZMapXMLUtilsEventStackStruct wrap_start_G[] =
  {
    {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {0}},
    {0}
  } ;

static ZMapXMLUtilsEventStackStruct wrap_end_G[] =
  {
    {ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  } ;


static ZMapXMLObjTagFunctionsStruct response_starts_G[] = {
  {"zmap",     xml_zmap_start_cb     },
  {"response", xml_response_start_cb },
  { NULL, NULL}
};

static ZMapXMLObjTagFunctionsStruct response_ends_G[] = {
  {"zmap",  xml_zmap_end_cb  },
  {"error", xml_error_end_cb },
  { NULL, NULL}
};
static gboolean alert_client_debug_G = FALSE;



/* 
 *                  External interface.
 */

/* NONE OF THIS APPEARS TO BE CALLED ANYWHERE....SIGH..... */

gboolean zmapControlRemoteAlertClient(ZMap zmap,
                                      char *action, GArray *xml_events,
                                      ZMapXMLObjTagFunctions start_handlers,
                                      ZMapXMLObjTagFunctions end_handlers,
                                      gpointer *handler_data)
{
  gboolean yield = FALSE;
  GList *clients = NULL;
  
  if(zmap->xremote_client)
    {
      clients = g_list_append(clients, zmap->xremote_client);
      
      yield = zmapControlRemoteAlertClients(zmap, clients, action, xml_events,
                                            start_handlers, end_handlers, handler_data);
      
      g_list_free(clients);
    }

  return yield;
}

gboolean zmapControlRemoteAlertClients(ZMap zmap, GList *clients,
                                       char *action, GArray *xml_events,
                                       ZMapXMLObjTagFunctions start_handlers,
                                       ZMapXMLObjTagFunctions end_handlers,
                                       gpointer *handler_data)
{
  ZMapXMLWriter xml_creator = NULL;
  SendClientCommandStruct message_data = {NULL} ;
  ZMapXMLUtilsEventStack wrap_ptr;
  gboolean yield = FALSE;

  if(clients)
    {
      ZMapXMLTagHandlerWrapperStruct full_xml_data = {NULL};
      ZMapXMLTagHandlerStruct control_data = {FALSE, NULL};

      full_xml_data.tag_handler = &control_data;
      full_xml_data.user_data   = handler_data;

      message_data.full_text = g_string_sized_new(500);
      message_data.zmap      = zmap;
      message_data.parser    = zMapXMLParserCreate(&full_xml_data, FALSE, FALSE);
      message_data.start_handlers = start_handlers ;
      message_data.end_handlers   = end_handlers ;
      message_data.tag_handler    = &control_data;

      if(!xml_events)
        xml_events = g_array_sized_new(FALSE, FALSE, sizeof(ZMapXMLWriterEventStruct), 5);

      wrap_ptr = &wrap_start_G[1];
      wrap_ptr->value.s = g_strdup(action) ;

      xml_events = zMapXMLUtilsAddStackToEventsArrayStart(xml_events, &wrap_start_G[0]);
      xml_events = zMapXMLUtilsAddStackToEventsArrayEnd(xml_events, &wrap_end_G[0]);
      
      xml_creator = zMapXMLWriterCreate(xml_event_to_buffer, message_data.full_text);

      if((zMapXMLWriterProcessEvents(xml_creator, xml_events, FALSE)) == ZMAPXMLWRITER_OK)
        g_list_foreach(clients, send_client_command, &message_data);
      else
        zMapLogWarning("%s", "Error processing events");

      zMapXMLWriterDestroy(xml_creator);
      zMapXMLParserDestroy(message_data.parser);

      yield = control_data.handled;

      if(control_data.error_message)
        g_free(control_data.error_message);
    }

  return yield;
}



/* 
 *                  Internal interface.
 */



static void send_client_command(gpointer client_data, gpointer user_data)
{
  ZMapXRemoteObj      client = (ZMapXRemoteObj)client_data;
  SendClientCommand message_data = (SendClientCommand)user_data;
  char *response = NULL, *command = NULL;
  int result;

  command = message_data->full_text->str ;

  if(alert_client_debug_G)
    {
      zMapLogWarning("xremote sending cmd with length %d", message_data->full_text->len);
      zMapLogWarning("xremote cmd = %s", command);
    }

  if ((result = zMapXRemoteSendRemoteCommand(client, command, &response)) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
    {
      char *xml_only = NULL;
      int code  = 0;
      gboolean parses_ok = FALSE, error_response = FALSE;;
      ZMapXMLObjTagFunctions start, end ;

      start = &response_starts_G[0] ;
      end = &response_ends_G[0] ;

      if (message_data->start_handlers)
	start = message_data->start_handlers ;

      if (message_data->end_handlers)
	end = message_data->end_handlers ;

      zMapXMLParserReset(message_data->parser);

      zMapXMLParserSetMarkupObjectTagHandlers(message_data->parser, start, end) ;
      
      zMapXRemoteResponseSplit(client, response, &code, &xml_only);

      if((zMapXRemoteResponseIsError(client, response)))
        error_response = TRUE;

      /* You can do dummy tests of xml by setting xml_only to point to a string of xml
       * that you have defined. */

      if ((parses_ok = zMapXMLParserParseBuffer(message_data->parser, 
						xml_only, 
						strlen(xml_only))) != TRUE)
        {
          if (alert_client_debug_G)
            zMapLogWarning("Parsing error : %s", zMapXMLParserLastErrorMsg(message_data->parser));

	  message_data->tag_handler->handled = FALSE ;
        }
      else if (error_response == TRUE)
	{
	  zMapWarning("Failed to get successful response from external program.\n"
		      "Code: %d\nResponse: %s", 
		      code, 
		      (message_data->tag_handler->error_message ? message_data->tag_handler->error_message : xml_only));

	  message_data->tag_handler->handled = FALSE ;
	}
    }
  else if(alert_client_debug_G)
    zMapLogWarning("Failed sending xremote command. Code = %d", result);

  return ;
}


static int xml_event_to_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data)
{
  GString *buffer = (GString *)user_data;

  g_string_append_len(buffer, xml, len);

  return len;
}


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, 
                                  ZMapXMLParser parser)
{
  if(alert_client_debug_G)
    zMapLogWarning("%s", "In zmap Start Handler");

  return TRUE;
}


static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  if(alert_client_debug_G)
    zMapLogWarning("In zmap %s Handler.", "End");

  return TRUE;
}



static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapXMLTagHandler message_data = wrapper->tag_handler;
  ZMapXMLAttribute handled_attribute = NULL;
  gboolean is_handled = FALSE;

  if((handled_attribute = zMapXMLElementGetAttributeByName(element, "handled")) &&
     (message_data->handled == FALSE))
    {
      is_handled = zMapXMLAttributeValueToBool(handled_attribute);
      message_data->handled = is_handled;
    }

  return TRUE;
}

static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, 
                                 ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapXMLTagHandler message_data = wrapper->tag_handler;
  ZMapXMLElement mess_element = NULL;

  if((mess_element = zMapXMLElementGetChildByName(element, "message")) &&
     !message_data->error_message &&
     mess_element->contents->str)
    {
      message_data->error_message = g_strdup(mess_element->contents->str);
    }
  
  return TRUE;
}

