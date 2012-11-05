/*  File: zmapControlRemoteReceive.c
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface functions for xremote API to zmap control window.
 *
 * Exported functions: See zmapControl_P.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapView.h>
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
    ZMAPCONTROL_REMOTE_REGISTER_CLIENT,
    ZMAPCONTROL_REMOTE_NEW_VIEW,
    ZMAPCONTROL_REMOTE_CLOSE_VIEW,

    /* ...but above here */
    ZMAPCONTROL_REMOTE_UNKNOWN
  }ZMapControlValidXRemoteActions;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef struct
{
  GQuark sequence ;
  int start ;
  int end ;
  char *config_file ;
}ViewConnectDataStruct, *ViewConnectData;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


typedef struct
{
  ZMap zmap;

  ZMapFeatureContext edit_context;
  GList *locations;

  unsigned long xwid ;

  ZMapFeatureSequenceMapStruct seq_map ;
}RequestDataStruct, *RequestData ;

typedef struct
{
  int  code;
  gboolean handled;
  GString *messages;
} ResponseDataStruct, *ResponseData;


typedef struct
{
  unsigned long xwid ;

  ZMapView view ;
} FindViewDataStruct, *FindViewData ;



/* ZMAPXREMOTE_CALLBACK */
static char *control_execute_command(char *command_text, gpointer user_data,
				     ZMapXRemoteStatus *statusCode, ZMapXRemoteObj owner);
static void insertView(ZMap zmap, RequestData input_data, ResponseData output_data);
static void closeView(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data) ;
static void createClient(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data);
static void findView(gpointer data, gpointer user_data) ;

static gboolean xml_zmap_start_cb(gpointer user_data,
                                  ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser);
static gboolean xml_request_start_cb(gpointer user_data,
				     ZMapXMLElement zmap_element,
				     ZMapXMLParser parser);
static gboolean xml_segment_start_cb(gpointer user_data,
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
  { "request",    xml_request_start_cb    },

#ifdef NOT_YET
  { "featureset", xml_featureset_start_cb },
  { "feature",    xml_feature_start_cb    },
#endif
  { "segment",    xml_segment_start_cb    },

  { "client",     zMapXRemoteXMLGenericClientStartCB },
  {NULL, NULL}
};

static ZMapXMLObjTagFunctionsStruct control_ends_G[] = {
  { "zmap",       xml_return_true_cb    },
  { "request",    xml_return_true_cb    },
  { "feature",    xml_return_true_cb    },

#ifdef NOT_YET
  { "subfeature", xml_subfeature_end_cb },
#endif

  { "location",   xml_location_end_cb   },
  { "style",      xml_style_end_cb      },
  {NULL, NULL}
};

static char *actions_G[ZMAPCONTROL_REMOTE_UNKNOWN + 1] =
  {
    NULL,
    "zoom_in", "zoom_out",
    "register_client",
    "new_view", "close_view",
    NULL
  };




void zmapControlRemoteInstaller(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  ZMap zmap = (ZMap)user_data ;

  zMapXRemoteInitialiseWidget(widget, PACKAGE_NAME,
                              ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                              ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                              control_execute_command, zmap) ;
  return ;
}

char *zMapControlRemoteReceiveAccepts(ZMap zmap)
{
  char *xml = NULL;

  xml = zMapXRemoteClientAcceptsActionsXML(zMapXRemoteWidgetGetXID(zmap->toplevel),
                                           &actions_G[ZMAPCONTROL_REMOTE_INVALID + 1],
                                           ZMAPCONTROL_REMOTE_UNKNOWN - 1);

  return xml;
}


/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */

/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *control_execute_command(char *command_text, gpointer user_data,
				     ZMapXRemoteStatus *statusCode, ZMapXRemoteObj owner)
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


  zMapDebugPrint(xremote_debug_GG, "ZMap Control Remote Handler: %s",  command_text) ; 


  input_data.zmap = zmap;
  input.user_data = &input_data;

  zmap->xremote_server = owner;     /* so we can do a delayed reply */

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
	  input_data.seq_map.dataset = zmap->default_sequence->dataset;   /* provide a default FTM */

          insertView(zmap, &input_data, &output_data);
          break;
        case ZMAPCONTROL_REMOTE_CLOSE_VIEW:
          closeView(zmap, &input, &output_data);
          break;
        case ZMAPCONTROL_REMOTE_INVALID:
        case ZMAPCONTROL_REMOTE_UNKNOWN:
        default:
          g_string_append_printf(output_data.messages, "Unknown command");
          output_data.code = ZMAPXREMOTE_UNKNOWNCMD;
          break;
        }

      *statusCode = output_data.code;
      if(input.common.action != ZMAPCONTROL_REMOTE_NEW_VIEW || output_data.code != ZMAPXREMOTE_OK)
        {
	  /* new view has to delay before responding */
          xml_reply = g_string_free(output_data.messages, FALSE);
        }
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
/*  if(xml_reply == NULL){ xml_reply = g_strdup("Broken code."); }
 not for new view: null resposnse to squelch output
*/

  return xml_reply;
}


static void insertView(ZMap zmap, RequestData input_data, ResponseData output_data)
{
  ZMapView view ;
  ZMapFeatureSequenceMap seq_map_in = &(input_data->seq_map) ;

  if (seq_map_in->sequence && seq_map_in->start >= 1 && seq_map_in->end >= seq_map_in->start)
    {
      ZMapFeatureSequenceMap seq_map ;
      char *err_msg = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Is this needed now.....oh....probably need dataset= in the xml..... */

#warning we need to get dataset (= species) from otterlace with added XML
      //      zMapAssert(zmap->default_sequence);
      if (!zmap->default_sequence || !zmap->default_sequence->dataset)
	{
	  /* there has been a major configuration error */

	  output_data->code = ZMAPXREMOTE_INTERNAL;
	  g_string_append_printf(output_data->messages,
                                 "No sequence specified in ZMap config - cannot create view");
	  return;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      seq_map = g_new0(ZMapFeatureSequenceMapStruct,1) ;
      *seq_map = *seq_map_in ;

      /* call to add view needed here with err_msg return etc..... */
      if ((view = zmapControlInsertView(zmap, seq_map, &err_msg)))
	{
	  char *xml = NULL;

	  output_data->code = ZMAPXREMOTE_OK;
	  xml = zMapViewRemoteReceiveAccepts(view);
	  g_string_append(output_data->messages, xml);
	  g_free(xml);
	}
      else
        {
          output_data->code = ZMAPXREMOTE_INTERNAL;
          g_string_append(output_data->messages, err_msg) ;
	  g_free(err_msg) ;
        }
    }

  return ;
}


/* Received a command to close a view. */
static void closeView(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data)
{
  ClientParameters client_params = &(input_data->common.client_params);
  FindViewDataStruct view_data ;

  view_data.xwid = client_params->xid ;
  view_data.view = NULL ;

  /* Find the view to close... */
  g_list_foreach(zmap->view_list, findView, &view_data) ;

  if (!(view_data.view))
    {
      output_data->code = ZMAPXREMOTE_INTERNAL ;
      g_string_append_printf(output_data->messages,
			     "could not find view with xwid=\"0x%lx\"", view_data.xwid) ;
    }
  else
    {
      char *xml = NULL;

      zmapControlRemoveView(zmap, view_data.view) ;

      /* Is this correct ??? check with Roy..... */
      output_data->code = ZMAPXREMOTE_OK;
      xml = zMapViewRemoteReceiveAccepts(view_data.view);
      g_string_append(output_data->messages, xml);
      g_free(xml);
    }

  return ;
}

static void findView(gpointer data, gpointer user_data)
{
  ZMapView view = (ZMapView)data ;
  FindViewData view_data = (FindViewData)user_data ;

  if (!(view_data->view) && zMapXRemoteWidgetGetXID(zMapViewGetXremote(view)) == view_data->xwid)
    view_data->view = view ;

  return ;
}



static void createClient(ZMap zmap, ZMapXRemoteParseCommandData input_data, ResponseData output_data)
{
  ZMapXRemoteObj client;
  ClientParameters client_params = &(input_data->common.client_params);
  char *format_response = "<client xwid=\"0x%lx\" created=\"%d\" exists=\"%d\" />";
  int created, exists;

  if (!(zmap->xremote_client) && (client = zMapXRemoteNew(GDK_DISPLAY())) != NULL)
    {
      zMapXRemoteInitClient(client, client_params->xid) ;
      zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(client_params->request)) ;
      zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(client_params->response)) ;

      zmap->xremote_client = client ;
      created = exists = 1 ;
    }
  else if (zmap->xremote_client)
    {
      created = 0 ;
      exists  = 1 ;
    }
  else
    {
      created = exists = 0 ;
    }

  g_string_append_printf(output_data->messages, format_response,
			 zMapXRemoteWidgetGetXID(zmap->toplevel), created, exists) ;
  output_data->code = ZMAPXREMOTE_OK ;

  return ;
}



/*
 *                 XML Handlers
 */


/* all the action happens in <request> now. */
static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapXRemoteParseCommandData parsing_data = (ZMapXRemoteParseCommandData)user_data;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return FALSE;
}


static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element,
				     ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapXRemoteParseCommandData parsing_data = (ZMapXRemoteParseCommandData)user_data;
  GQuark action = 0;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      int i;
      action = zMapXMLAttributeGetValue(attr);

      parsing_data->common.action = ZMAPCONTROL_REMOTE_INVALID;

      for (i = ZMAPCONTROL_REMOTE_INVALID + 1; i < ZMAPCONTROL_REMOTE_UNKNOWN; i++)
        {
          if (action == g_quark_from_string(actions_G[i]))
	    {
	      parsing_data->common.action = i;
	      break ;
	    }
        }

      /* unless((action > INVALID) and (action < UNKNOWN)) */
      if(!(parsing_data->common.action > ZMAPCONTROL_REMOTE_INVALID
	   && parsing_data->common.action < ZMAPCONTROL_REMOTE_UNKNOWN))
        {
          zMapLogWarning("action='%s' is unknown", g_quark_to_string(action));
          parsing_data->common.action = ZMAPCONTROL_REMOTE_UNKNOWN;
        }
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "action is a required attribute for request.");
      parsing_data->common.action = ZMAPCONTROL_REMOTE_INVALID;
    }

  return FALSE;
}

static gboolean xml_segment_start_cb(gpointer user_data, ZMapXMLElement segment,
                                   ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(segment, "sequence")) != NULL)
    request_data->seq_map.sequence = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)) ;

  if((attr = zMapXMLElementGetAttributeByName(segment, "start")) != NULL)
    request_data->seq_map.start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    request_data->seq_map.start = 1;

  if((attr = zMapXMLElementGetAttributeByName(segment, "end")) != NULL)
    request_data->seq_map.end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    request_data->seq_map.end = 0;

  if((attr = zMapXMLElementGetAttributeByName(segment, "config-file")) != NULL)
    request_data->seq_map.config_file = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)) ;

  return TRUE ;
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
