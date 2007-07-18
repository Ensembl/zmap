/*  File: zmapControlRemoteReceive.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2007: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 16 11:27 2007 (rds)
 * Created: Thu Jul 12 14:54:30 2007 (rds)
 * CVS info:   $Id: zmapControlRemoteReceive.c,v 1.1 2007-07-18 13:30:24 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <zmapControl_P.h>

enum
  {
    ZMAPCONTROL_REMOTE_INVALID,
    /* Add below here... */

    ZMAPCONTROL_REMOTE_ZOOM_IN,
    ZMAPCONTROL_REMOTE_ZOOM_OUT,
    ZMAPCONTROL_REMOTE_ZOOM_TO,
    ZMAPCONTROL_REMOTE_REGISTER_CLIENT,
    ZMAPCONTROL_REMOTE_NEW_VIEW,

    /* ...but above here */
    ZMAPCONTROL_REMOTE_UNKNOWN
  }ZMapControlValidXRemoteActions;

typedef struct
{
  GQuark sequence;
  gint   start, end;
  char *config;
}ViewConnectDataStruct, *ViewConnectData;

typedef struct
{
  ZMap zmap;

  ZMapFeatureContext edit_context;
  GList *locations;

  ViewConnectDataStruct view_params;
}RequestDataStruct, *RequestData;

typedef struct
{
  int  code;
  gboolean handled;
  GString *messages;
} ResponseDataStruct, *ResponseData;


/* ZMAPXREMOTE_CALLBACK */
static char *control_execute_command(char *command_text, gpointer user_data, int *statusCode);
static void insertView(ZMap zmap, RequestData input_data, ResponseData output_data);
static void createClient(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data);

static gboolean xml_zmap_start_cb(gpointer user_data, 
                                  ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser);
static gboolean xml_segment_end_cb(gpointer user_data, 
                                   ZMapXMLElement segment, 
                                   ZMapXMLParser parser);
static gboolean xml_location_end_cb(gpointer user_data, 
                                    ZMapXMLElement segment, 
                                    ZMapXMLParser parser);
static gboolean xml_style_end_cb(gpointer user_data, 
                                   ZMapXMLElement segment, 
                                   ZMapXMLParser parser);
static gboolean xml_return_true_cb(gpointer user_data, 
                                   ZMapXMLElement zmap_element, 
                                   ZMapXMLParser parser);

static gboolean control_execute_debug_G = FALSE;

static ZMapXMLObjTagFunctionsStruct control_starts_G[] = {
  { "zmap",       xml_zmap_start_cb       },
#ifdef NOT_YET
  { "featureset", xml_featureset_start_cb },
  { "feature",    xml_feature_start_cb    },
#endif
  { "client",     zMapXRemoteXMLGenericClientStartCB },
  {NULL, NULL}
};
static ZMapXMLObjTagFunctionsStruct control_ends_G[] = {
  { "zmap",       xml_return_true_cb    },
  { "feature",    xml_return_true_cb    },
  { "segment",    xml_segment_end_cb    },
#ifdef NOT_YET
  { "subfeature", xml_subfeature_end_cb },
#endif
  { "location",   xml_location_end_cb   },
  { "style",      xml_style_end_cb      },
  {NULL, NULL}
};


void zmapControlRemoteInstaller(ZMap zmap, GtkWidget *widget)
{
  zMapXRemoteInitialiseWidget(widget, PACKAGE_NAME, 
                              ZMAP_DEFAULT_REQUEST_ATOM_NAME, 
                              ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                              control_execute_command, zmap);
  return ;
}



/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */

/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *control_execute_command(char *command_text, gpointer user_data, int *statusCode)
{
  ZMapXMLParser parser;
  ZMap zmap = (ZMap)user_data;
  char *xml_reply = NULL;
  ZMapXRemoteParseCommandDataStruct input = { NULL };
  RequestDataStruct input_data = {0};

  if(zMapXRemoteIsPingCommand(command_text, statusCode, &xml_reply) != 0)
    {
      goto HAVE_RESPONSE;
    }

  input_data.zmap = zmap;
  input.user_data = &input_data;

  parser = zMapXMLParserCreate(&input, FALSE, FALSE);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &control_starts_G[0], &control_ends_G[0]);

  if((zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))) == TRUE)
    {
      ResponseDataStruct output_data = {0};
      
      output_data.code = 0;
      output_data.messages = g_string_sized_new(512);
      
      switch(input.common.action)
        {
        case ZMAPCONTROL_REMOTE_REGISTER_CLIENT:
          createClient(zmap, &input, &output_data);
          break;
        case ZMAPCONTROL_REMOTE_NEW_VIEW:
          insertView(zmap, &input_data, &output_data);
          break;
        case ZMAPCONTROL_REMOTE_INVALID:
        case ZMAPCONTROL_REMOTE_UNKNOWN:
        default:
          g_string_append_printf(output_data.messages, "Unknown command");
          output_data.code = ZMAPXREMOTE_UNKNOWNCMD;
          break;
        }

      *statusCode = output_data.code;
      xml_reply   = g_string_free(output_data.messages, FALSE);
    }
  else
    {
      *statusCode = ZMAPXREMOTE_BADREQUEST;
      xml_reply   = g_strdup(zMapXMLParserLastErrorMsg(parser));
    }

  if(control_execute_debug_G)
    printf("Destroying context\n");

  if(input_data.edit_context)
    zMapFeatureContextDestroy(input_data.edit_context, TRUE);

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


static void insertView(ZMap zmap, RequestData input_data, ResponseData output_data)
{
  ViewConnectData view_params = &(input_data->view_params);
  ZMapView view;
  char *sequence;

  if((sequence = (char *)g_quark_to_string(view_params->sequence)) &&
     view_params->config)
    {
      if((view = zMapAddView(zmap, sequence, 
                             view_params->start, 
                             view_params->end)))
        {
          zMapViewReadConfigBuffer(view, view_params->config);

          if(!(zmapConnectViewConfig(zmap, view, view_params->config)))
            {
              zMapDeleteView(zmap, view); /* Need 2 do more here. */
              output_data->code = ZMAPXREMOTE_UNKNOWNCMD;
              g_string_append_printf(output_data->messages,
                                     "view connection failed.");
            }
          else
            {
              output_data->code = ZMAPXREMOTE_OK;
              g_string_append_printf(output_data->messages, 
                                     "<windowid>0x%lx</windowid>", 
                                     zMapViewGetXID(view));
            }
        }
      else
        {
          output_data->code = ZMAPXREMOTE_INTERNAL;
          g_string_append_printf(output_data->messages,
                                 "failed to create view");
        }
        
    }

  return ;
}

static void createClient(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data)
{
  ZMapXRemoteObj client;
  ClientParameters client_params = &(input_data->common.client_params);
  char *format_response = "<client created=\"%d\" exists=\"%d\" />";
  int created, exists;

  if(!(zmap->xremote_client) && (client = zMapXRemoteNew()) != NULL)
    {
      zMapXRemoteInitClient(client, client_params->xid);
      zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(client_params->request));
      zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(client_params->response));

      zmap->xremote_client = client;
      created = exists = 1;
    }
  else if(zmap->xremote_client)
    {
      created = 0;
      exists  = 1;
    }
  else
    {
      created = exists = 0;
    }

  g_string_append_printf(output_data->messages, format_response, created, exists);
  output_data->code = ZMAPXREMOTE_OK;

  return;
}


/* Handlers */

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapXRemoteParseCommandData parsing_data = (ZMapXRemoteParseCommandData)user_data;
  GQuark action = 0;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      action = zMapXMLAttributeGetValue(attr);

      if(action == g_quark_from_string("zoom_in"))
        parsing_data->common.action = ZMAPCONTROL_REMOTE_ZOOM_IN;
      else if(action == g_quark_from_string("zoom_out"))
        parsing_data->common.action = ZMAPCONTROL_REMOTE_ZOOM_OUT;
      else if(action == g_quark_from_string("zoom_to"))
        parsing_data->common.action = ZMAPCONTROL_REMOTE_ZOOM_TO;  
      else if(action == g_quark_from_string("register_client"))
        parsing_data->common.action = ZMAPCONTROL_REMOTE_REGISTER_CLIENT;
      else if(action == g_quark_from_string("new_view"))
        parsing_data->common.action = ZMAPCONTROL_REMOTE_NEW_VIEW;
      else
        {
          zMapLogWarning("action='%s' is unknown", g_quark_to_string(action));
          parsing_data->common.action = ZMAPCONTROL_REMOTE_UNKNOWN;
        }
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");
      parsing_data->common.action = ZMAPCONTROL_REMOTE_INVALID;
    }

  return FALSE;
}

static gboolean xml_segment_end_cb(gpointer user_data, ZMapXMLElement segment, 
                                   ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(segment, "sequence")) != NULL)
    request_data->view_params.sequence = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(segment, "start")) != NULL)
    request_data->view_params.start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    request_data->view_params.start = 1;
  if((attr = zMapXMLElementGetAttributeByName(segment, "end")) != NULL)
    request_data->view_params.end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    request_data->view_params.end = 0;

  /* Need to put contents into a source stanza buffer... */
  request_data->view_params.config = zMapXMLElementStealContent(segment);
  
  return TRUE;
}

static gboolean xml_location_end_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                    ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapSpan location;
  ZMapXMLAttribute attr;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "start")))
    {
      if((location = g_new0(ZMapSpanStruct, 1)))
        {
          location->x1 = zMapXMLAttributeValueToInt(attr);
          if((attr = zMapXMLElementGetAttributeByName(zmap_element, "end")))
            location->x2 = zMapXMLAttributeValueToInt(attr);
          else
            {
              location->x2 = location->x1;
              zMapXMLParserRaiseParsingError(parser, "end is required as well as start");
            }
          request_data->locations = g_list_append(request_data->locations, location);
        }
    }

  return TRUE;
}
static gboolean xml_style_end_cb(gpointer user_data, ZMapXMLElement element,
                                 ZMapXMLParser parser)
{
#ifdef RDS_FIX_THIS
  ZMapFeatureTypeStyle style = NULL;
  XMLData               data = (XMLData)user_data;
#endif
  ZMapXMLAttribute      attr = NULL;
  GQuark id = 0;

  if((attr = zMapXMLElementGetAttributeByName(element, "id")))
    id = zMapXMLAttributeGetValue(attr);

#ifdef PARSINGCOLOURS
  if(id && (style = zMapFindStyle(zMapViewGetStyles(zmap->focus_viewwindow), id)) != NULL)
    {
      if((attr = zMapXMLElementGetAttributeByName(element, "foreground")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->foreground));
      if((attr = zMapXMLElementGetAttributeByName(element, "background")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->background));
      if((attr = zMapXMLElementGetAttributeByName(element, "outline")))
        gdk_color_parse(g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                        &(style->outline));
      if((attr = zMapXMLElementGetAttributeByName(element, "description")))
        style->description = (char *)g_quark_to_string( zMapXMLAttributeGetValue(attr) );
      
    }
#endif

  return TRUE;
}


static gboolean xml_return_true_cb(gpointer user_data, 
                                   ZMapXMLElement zmap_element, 
                                   ZMapXMLParser parser)
{
  return TRUE;
}
