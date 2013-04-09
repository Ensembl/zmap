/*  File: zmapViewRemoteSend.c
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
 * Description: Handles sending xml messages to any connected client.
 *
 * Exported functions: See zmapView_P.h
 *              
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <zmapView_P.h>



static ZMapXRemoteSendCommandError send_client_command(ZMapXRemoteObj client, ZMapXMLParser parser, 
						       GString *command_string, ZMapXMLTagHandler tag_handler) ;
static int  xml_event_to_buffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data) ;



/* xml event callbacks */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean xml_error_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, ZMapXMLParser parser);


static ZMapXMLUtilsEventStackStruct wrap_start_G[] =
  {
    {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  } ;
static ZMapXMLUtilsEventStackStruct wrap_end_G[] =
  {
    {ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  };

static ZMapXMLUtilsEventStackStruct wrap_request_G[] =
  {
    {ZMAPXML_START_ELEMENT_EVENT, "request",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {NULL}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "xwid", ZMAPXML_EVENT_DATA_QUARK, {NULL}},
    {0}
  } ;

static ZMapXMLUtilsEventStackStruct wrap_request_end_G[] =
  {
    {ZMAPXML_END_ELEMENT_EVENT,   "request",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  };

static ZMapXMLObjTagFunctionsStruct response_starts_G[] = {
  {"zmap", xml_zmap_start_cb     },
  {"request", xml_request_start_cb     },
  {"response", xml_response_start_cb },
  { NULL, NULL}
};

static ZMapXMLObjTagFunctionsStruct response_ends_G[] = {
  {"zmap",  xml_zmap_end_cb  },
  {"error", xml_error_end_cb },
  { NULL, NULL}
};

static gboolean view_remote_send_debug_G = FALSE;




ZMapXRemoteSendCommandError zmapViewRemoteSendCommand(ZMapView view,
						      char *action, GArray *xml_events,
						      ZMapXMLObjTagFunctions start_handlers,
						      ZMapXMLObjTagFunctions end_handlers,
						      gpointer *handler_data)
{
  ZMapXRemoteSendCommandError result = ZMAPXREMOTE_SENDCOMMAND_UNAVAILABLE ;
  ZMapXRemoteObj xremote ;

#ifdef ZMAP_VIEW_REMOTE_SEND_XML_TEST
  if(ZMAP_VIEW_REMOTE_SEND_XML_TEST)
#else
  if((xremote = view->xremote_client))
#endif /* ZMAP_VIEW_REMOTE_SEND_XML_TEST */
    {
      ZMapXMLTagHandlerWrapperStruct wrapper_data = {NULL};
      ZMapXMLTagHandlerStruct common_data = {FALSE, NULL};
      ZMapXMLUtilsEventStack wrap_ptr;
      ZMapXMLWriter xml_creator;
      ZMapXMLParser parser;

      GString *full_text = g_string_sized_new(512);

      wrapper_data.tag_handler = &common_data;
      wrapper_data.user_data   = handler_data;

      parser = zMapXMLParserCreate(&wrapper_data, FALSE, FALSE);
      
      if (!xml_events)
        xml_events = g_array_sized_new(FALSE, FALSE, sizeof(ZMapXMLWriterEventStruct), 5);


      wrap_ptr = &wrap_request_G[1] ;
      wrap_ptr->value.s = action ;
      wrap_ptr = &wrap_request_G[2] ;
      wrap_ptr->value.s = view->xwid_txt ;

      xml_events = zMapXMLUtilsAddStackToEventsArrayStart(&wrap_request_G[0], xml_events);

      xml_events = zMapXMLUtilsAddStackToEventsArrayStart(&wrap_start_G[0], xml_events);

      xml_events = zMapXMLUtilsAddStackToEventsArray(&wrap_request_end_G[0], xml_events);

      xml_events = zMapXMLUtilsAddStackToEventsArray(&wrap_end_G[0], xml_events);

      xml_creator = zMapXMLWriterCreate(xml_event_to_buffer, full_text);

      if (!start_handlers)
        start_handlers = &response_starts_G[0];

      if (!end_handlers)
        end_handlers = &response_ends_G[0];

      zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers, end_handlers);

      if ((zMapXMLWriterProcessEvents(xml_creator, xml_events)) == ZMAPXMLWRITER_OK)
        {
          result = send_client_command(xremote, parser, full_text, &common_data);
        }
      else
	{
	  zMapLogWarning("%s", "Error processing events");
	}

      zMapXMLWriterDestroy(xml_creator);
      zMapXMLParserDestroy(parser);

      if (!(common_data.handled))
	result = ZMAPXREMOTE_SENDCOMMAND_CLIENT_ERROR ;

    }
  
  return result ;
}


/* 
 *                 Internal routines.
 */


static ZMapXRemoteSendCommandError send_client_command(ZMapXRemoteObj client, ZMapXMLParser parser, 
						       GString *command_string, ZMapXMLTagHandler tag_handler)
{
  ZMapXRemoteSendCommandError result ;
  char *command  = command_string->str;
  char *response = NULL;

  if (view_remote_send_debug_G)
    {
      zMapLogWarning("xremote sending cmd with length %d", command_string->len);
      zMapLogWarning("xremote cmd = %s", command);
    }

#ifdef ZMAP_VIEW_REMOTE_SEND_XML_TEST
  if(ZMAP_VIEW_REMOTE_SEND_XML_TEST)
    {
      response = "200:<zmap />";
#else
  if ((result = zMapXRemoteSendRemoteCommand(client, command, &response)) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
    {
#endif /* ZMAP_VIEW_REMOTE_SEND_XML_TEST */

      char *xml_only = NULL ;
      int code  = 0 ;
      gboolean parses_ok = FALSE, error_response = FALSE ;
      
      zMapXRemoteResponseSplit(client, response, &code, &xml_only) ;

      if ((zMapXRemoteResponseIsError(client, response)))
        error_response = TRUE ;

      /* You can do dummy tests of xml by setting xml_only to point to a string of xml
       * that you have defined. */
#ifdef ZMAP_VIEW_REMOTE_SEND_XML_TEST
      xml_only = ZMAP_VIEW_REMOTE_SEND_XML_TEST;
#endif /* ZMAP_VIEW_REMOTE_SEND_XML_TEST */

      if ((parses_ok = zMapXMLParserParseBuffer(parser, 
						xml_only, 
						strlen(xml_only))) != TRUE)
        {
          if (view_remote_send_debug_G)
            zMapLogWarning("Parsing error : %s", zMapXMLParserLastErrorMsg(parser));

	  tag_handler->handled = FALSE ;

	  result = ZMAPXREMOTE_SENDCOMMAND_XML_ERROR ;
        }
      else if (error_response == TRUE)
	{
	  zMapWarning("Failed to get successful response from external program.\n"
		      "Code: %d\nResponse: %s", 
		      code, 
		      (tag_handler->error_message ? tag_handler->error_message : xml_only));

	  tag_handler->handled = FALSE ;

	  result = ZMAPXREMOTE_SENDCOMMAND_CLIENT_ERROR ;
	}
    }
  else if (view_remote_send_debug_G)
    {
      zMapLogWarning("Failed sending xremote command. Code = %d", result) ;
    }

  return result ;
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
  if (view_remote_send_debug_G)
    zMapLogWarning("%s", "In zmap Start Handler");

  return TRUE;
}


static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement element, 
				     ZMapXMLParser parser)
{
  if (view_remote_send_debug_G)
    zMapLogWarning("%s", "In Request Start Handler");

  return TRUE;
}


static gboolean xml_response_start_cb(gpointer user_data, ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLTagHandlerWrapper wrapper = (ZMapXMLTagHandlerWrapper)user_data;
  ZMapXMLTagHandler message_data = wrapper->tag_handler;
  ZMapXMLAttribute handled_attribute = NULL;
  gboolean is_handled = FALSE;

  if ((handled_attribute = zMapXMLElementGetAttributeByName(element, "handled"))
      && (message_data->handled == FALSE))
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

  if ((mess_element = zMapXMLElementGetChildByName(element, "message"))
      && !message_data->error_message
      && mess_element->contents->str)
    {
      message_data->error_message = g_strdup(mess_element->contents->str);
    }
  
  return TRUE;
}



static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  if (view_remote_send_debug_G)
    zMapLogWarning("In zmap %s Handler.", "End");

  return TRUE;
}
