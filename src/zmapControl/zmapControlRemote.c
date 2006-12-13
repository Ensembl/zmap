/*  File: zmapControlRemote.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Implements code to respond to commands sent to a
 *              ZMap via the xremote program. The xremote program
 *              is distributed as part of acedb but is not dependent
 *              on it (although it currently makes use of acedb utils.
 *              code in the w1 directory).
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Dec 13 16:33 2006 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.35 2006-12-13 16:34:56 rds Exp $
 *-------------------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <gtk/gtk.h>
#include <ZMap/zmapConfigDir.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapControl_P.h>

/* For switch statements */
typedef enum {
  ZMAP_CONTROL_ACTION_ZOOM_IN = 1,
  ZMAP_CONTROL_ACTION_ZOOM_OUT,
  ZMAP_CONTROL_ACTION_ZOOM_TO,
  ZMAP_CONTROL_ACTION_CREATE_FEATURE,
  ZMAP_CONTROL_ACTION_ALTER_FEATURE,
  ZMAP_CONTROL_ACTION_DELETE_FEATURE,
  ZMAP_CONTROL_ACTION_FIND_FEATURE,  
  ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE,
  ZMAP_CONTROL_ACTION_REGISTER_CLIENT,

  ZMAP_CONTROL_ACTION_NEW_VIEW,
  ZMAP_CONTROL_ACTION_INSERT_VIEW_DATA,

  ZMAP_CONTROL_ACTION_ADD_DATASOURCE,

  ZMAP_CONTROL_ACTION_UNKNOWN
} zmapControlAction;

typedef struct
{
  char *xml_action_string;
  zmapControlAction action;
  GQuark string_as_quark;
} ActionValidatorStruct, *ActionValidator;

typedef struct {
  unsigned long xid;
  GQuark request;
  GQuark response;
} controlClientObjStruct, *controlClientObj;

typedef struct
{
  GQuark sequence;
  gint   start, end;
  char *config;
}ViewConnectDataStruct, *ViewConnectData;

typedef struct {
  zmapControlAction action;
  ZMapWindow window;

  controlClientObj  client;
  ZMapWindowFToIQuery set;   /* The current featureset */
  /* queries.... */
  GList *query_list;

  GList *styles;                /* These should be ZMapFeatureTypeStyle */
  GList *locations;             /* This is just a list of ZMapSpan Structs */

  ViewConnectDataStruct new_view;
} XMLDataStruct, *XMLData;

typedef struct
{
  ZMap zmap;
  int  code;
  gboolean persist;
  GString *messages;
} ResponseCodeZMapStruct, *ResponseCodeZMap;

typedef struct
{
  GString *full_text;
  ZMap zmap;
  ZMapXMLParser parser;
  gboolean handled;
  char *error_message;
} AlertClientMessageStruct, *AlertClientMessage;

/* ZMAPXREMOTE_CALLBACK and destroy internals for zmapControlRemoteInstaller */
static char *controlExecuteCommand(char *command_text, ZMap zmap, int *statusCode);
static void destroyNotifyData(gpointer destroy_data);


static gboolean createClient(ZMap zmap, controlClientObj data);
static gboolean insertView(ZMap zmap, ViewConnectData new_view);
static ZMapXMLParser setupControlRemoteXMLParser(void *data);

static void drawNewFeature(gpointer data, gpointer userdata);
static void alterFeature(gpointer data, gpointer userdata);
static void deleteFeature(gpointer data, gpointer userdata);
/* xml handlers */
static gboolean zmapStrtHndlr(gpointer userdata, 
                              ZMapXMLElement zmap_element,
                              ZMapXMLParser parser);
static gboolean featuresetStrtHndlr(gpointer userdata, 
                                    ZMapXMLElement set_element,
                                    ZMapXMLParser parser);
static gboolean featureStrtHndlr(gpointer userdata, 
                                 ZMapXMLElement feature_element,
                                 ZMapXMLParser parser);
static gboolean featureEndHndlr(void *userData, 
                                ZMapXMLElement sub_element, 
                                ZMapXMLParser parser);
static gboolean segmentEndHndlr(void *userData, 
                                ZMapXMLElement sub_element, 
                                ZMapXMLParser parser);
static gboolean clientStrtHndlr(gpointer userdata, 
                                ZMapXMLElement client_element,
                                ZMapXMLParser parser);
static gboolean locationEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);
static gboolean styleEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);
static gboolean zmapEndHndlr(gpointer userdata, 
                             ZMapXMLElement zmap_element,
                             ZMapXMLParser parser);

static void alertClientToMessage(gpointer client_data, gpointer message_data);
static int xmlToMessageBuffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data);
static gboolean responseZmapStartHndlr(void *userData, 
                                       ZMapXMLElement element, 
                                       ZMapXMLParser parser);
static gboolean responseResponseStartHndlr(void *userData, 
                                           ZMapXMLElement element, 
                                           ZMapXMLParser parser);
static gboolean responseErrorEndHndlr(void *userData, 
                                      ZMapXMLElement element, 
                                      ZMapXMLParser parser);
static gboolean responseZmapEndHndlr(void *userData, 
                                     ZMapXMLElement element, 
                                     ZMapXMLParser parser);

void zMapSetClient(ZMap zmap, void *client_data)
{
  zMapXRemoteObj client = (zMapXRemoteObj)client_data;

  if(client)
    zmap->client = client;

  return ;
}

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap_data)
{
  ZMap zmap = (ZMap)zmap_data;
  zMapXRemoteObj xremote;

  if((xremote = zMapXRemoteNew()) != NULL)
    {
      Window id;
      zMapXRemoteNotifyData notifyData;

      id = (Window)zMapGetXID(zmap);

      notifyData           = g_new0(zMapXRemoteNotifyDataStruct, 1);
      notifyData->xremote  = xremote;
      notifyData->callback = ZMAPXREMOTE_CALLBACK(controlExecuteCommand);
      notifyData->data     = zmap_data; 

      /* Moving this (add_events) BEFORE the call to InitServer stops
       * some Xlib BadWindow errors (turn on debugging in zmapXRemote 2 c
       * them).  This doesn't feel right, but I couldn't bear the
       * thought of having to store a handler id for an expose_event
       * in the zmap struct just so I could use it over realize in
       * much the same way as the zmapWindow code does.  This
       * definitely points to some problem with GTK2.  The Widget
       * reports it's realized GTK_WIDGET_REALIZED(widget) has a
       * window id, but then the XChangeProperty() fails in the
       * zmapXRemote code.  Hmmm.  If this continues to be a problem
       * then I'll change it to use expose.  Only appeared on Linux. */
      /* Makes sure we actually get the events!!!! Use add_events as set_events needs to be done BEFORE realize */
      gtk_widget_add_events(widget, GDK_PROPERTY_CHANGE_MASK) ;
      
      zMapXRemoteInitServer(xremote, id, PACKAGE_NAME, ZMAP_DEFAULT_REQUEST_ATOM_NAME, ZMAP_DEFAULT_RESPONSE_ATOM_NAME);
      zMapConfigDirWriteWindowIdFile(id, zmap->zmap_id);

      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                            (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                            );
      
      zmap->propertyNotifyData = notifyData;
      
    }

  return;
}

gboolean zmapControlRemoteAlertClients(ZMap zmap, GArray *xml_events, char *action)
{
  GList *clients = NULL;
  ZMapXMLWriter xml_creator = NULL;
  AlertClientMessageStruct message_data = {0};
  ZMapXMLWriterEventStruct zmap_event = {0};
  
  /* Making this a GList, I can't afford time to make the
   * zmap->clients a glist, but this will make it easier when I do */
  if(zmap->client)
    clients = g_list_append(clients, zmap->client);

  if(clients)
    {
      message_data.full_text = g_string_sized_new(500);
      message_data.zmap      = zmap;
      message_data.parser    = zMapXMLParserCreate(&message_data, FALSE, FALSE);

      if(!xml_events)
        xml_events = g_array_sized_new(FALSE, FALSE, sizeof(ZMapXMLWriterEventStruct), 5);

      zmap_event.type = ZMAPXML_ATTRIBUTE_EVENT;
      zmap_event.data.comp.name        = g_quark_from_string("action");
      zmap_event.data.comp.value.quark = g_quark_from_string(action);
      g_array_prepend_val(xml_events, zmap_event);
      
      zmap_event.type = ZMAPXML_START_ELEMENT_EVENT;
      zmap_event.data.simple = g_quark_from_string("zmap");
      g_array_prepend_val(xml_events, zmap_event);
      
      zmap_event.type = ZMAPXML_END_ELEMENT_EVENT;
      g_array_append_val(xml_events, zmap_event);
      
      xml_creator = zMapXMLWriterCreate(xmlToMessageBuffer, &message_data);
      if((zMapXMLWriterProcessEvents(xml_creator, xml_events)) == ZMAPXMLWRITER_OK)
        g_list_foreach(clients, alertClientToMessage, &message_data);
      else
        zMapLogWarning("%s", "Error processing events");

      zMapXMLWriterDestroy(xml_creator);
      zMapXMLParserDestroy(message_data.parser);

      g_list_free(clients);
    }

  return message_data.handled;
}

/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */

static void destroyNotifyData(gpointer destroy_data)
{
  ZMap zmap;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData)destroy_data;

  zmap = destroy_me->data;
  zmap->propertyNotifyData = NULL; /* Set this to null, as we're emptying the mem */
  g_free(destroy_me);           /* Is this all we need to destroy?? */

  return ;
}

/* 
 *
 * <zmap action="create_feature">
 *   <featureset>
 *     <feature name="RP6-206I17.6-001" start="3114" end="99885" strand="-" style="Coding">
 *       <subfeature ontology="exon"   start="3114"  end="3505" />
 *       <subfeature ontology="intron" start="3505"  end="9437" />
 *       <subfeature ontology="exon"   start="9437"  end="9545" />
 *       <subfeature ontology="intron" start="9545"  end="18173" />
 *       <subfeature ontology="exon"   start="18173" end="19671" />
 *       <subfeature ontology="intron" start="19671" end="99676" />
 *       <subfeature ontology="exon"   start="99676" end="99885" />
 *       <subfeature ontology="cds"    start="1"     end="2210" />
 *     </feature>
 *   </featureset>
 * </zmap>

 */


static ZMapXMLParser setupControlRemoteXMLParser(void *data)
{
  ZMapXMLParser parser = NULL;
  gboolean cmd_debug   = TRUE;
  ZMapXMLObjTagFunctionsStruct starts[] = {
    { "zmap",       zmapStrtHndlr       },
    { "featureset", featuresetStrtHndlr },
    { "feature",    featureStrtHndlr    },
    { "client",     clientStrtHndlr     },
    { NULL, NULL }
  };
  ZMapXMLObjTagFunctionsStruct ends[] = {
    { "zmap",       zmapEndHndlr     },
    { "segment",    segmentEndHndlr  },
    { "subfeature", featureEndHndlr  },
    { "location",   locationEndHndlr },
    { "style",      styleEndHndlr    },
    { NULL, NULL }
  };

  zMapAssert(data);

  parser = zMapXMLParserCreate(data, FALSE, cmd_debug);
  /*  zMapXMLParser_setMarkupObjectHandler(parser, start, end); */
  zMapXMLParserSetMarkupObjectTagHandlers(parser, &starts[0], &ends[0]);

  return parser;
}

static gboolean feature_input_valid(ZMapFeature feature)
{
  gboolean valid = TRUE;
  ZMapStrand strand;

  strand = feature->strand;

  if(!(strand == ZMAPSTRAND_REVERSE || 
       strand == ZMAPSTRAND_FORWARD ||
       strand == ZMAPSTRAND_NONE))
    valid = FALSE ;

  if(feature->x1 > feature->x2)
    {
      if(strand == ZMAPSTRAND_REVERSE)
        {
          Coord tmp   = feature->x1;
          feature->x1 = feature->x2;
          feature->x2 = tmp;
        }
      else
        valid = FALSE;
    }


  return valid;
}

static void alterFeature(gpointer data, gpointer userdata)
{
  return ;
}

static void deleteFeature(gpointer data, gpointer userdata)
{ 
  ResponseCodeZMap    response_data = (ResponseCodeZMap)userdata;
  FooCanvasItem *feature_item = NULL;
  ZMapWindowFToIQuery   query = (ZMapWindowFToIQuery)data;
  ZMapWindow           window = NULL;
  ZMap                   zmap = response_data->zmap;
  gboolean removed = TRUE, 
    error_recorded = FALSE;

  zMapAssert(response_data->messages);

  if(!response_data->persist)
    return ;

  window  = zMapViewGetWindow(zmap->focus_viewwindow);

  if(response_data->messages->len)
    g_string_append_printf(response_data->messages, "\n");

  if((query == NULL) || !(feature_input_valid(&(query->feature_in))))
    {
      response_data->code = 412;
      g_string_append_printf(response_data->messages, 
                             "XML Data Error. Not enough or incorrect information.");
      error_recorded = TRUE;
    }
  else
    {
      gboolean found = FALSE;

      query->query_type = ZMAP_FTOI_QUERY_FEATURE_ITEM;

      if((found = zMapWindowFToIFetchByQuery(window, query)) &&
         (query->return_type == ZMAP_FTOI_RETURN_ITEM))
        {
          feature_item = query->ans.item_answer;
          removed = zMapWindowFeatureRemove(window, feature_item);
        }
      else
        {
          response_data->code = 404;
          g_string_append_printf(response_data->messages,
                                 "Failed to find feature '%s'.",
                                 (char *)g_quark_to_string(query->feature_in.original_id));
          error_recorded = TRUE;
        }
    }
  
  if(!removed && !error_recorded)
    {
      response_data->code = 404;
      g_string_append_printf(response_data->messages,
                             "Failed to remove feature '%s'.",
                             (char *)g_quark_to_string(query->feature_in.original_id));
      error_recorded = TRUE;
    }

  response_data->persist = !error_recorded;

  return ; 
}

static void drawNewFeature(gpointer data, gpointer userdata)
{
  ResponseCodeZMap response_data = (ResponseCodeZMap)userdata;
  FooCanvasItem *feature_item;
  FooCanvasGroup *block_group;
  FooCanvasGroup   *col_group;
  ZMapWindow           window;
  ZMapFeatureTypeStyle  style;
  ZMapFeature         feature;
  ZMapWindowFToIQuery   query = (ZMapWindowFToIQuery)data;
  ZMap                   zmap = response_data->zmap;
  char *feature_set_name, *style_name;
  GList *style_list;
  gboolean created = TRUE, 
    error_recorded = FALSE;

  zMapAssert(response_data->messages);

  if(!response_data->persist)
    return ;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  if(response_data->messages->len)
    g_string_append_printf(response_data->messages, "\n");

  if((query == NULL) || !(feature_input_valid(&(query->feature_in))))
    {
      response_data->code = 412;
      g_string_append_printf(response_data->messages, 
                             "XML Data Error. Not enough or incorrect information.");
      error_recorded = TRUE;
    }
  else
    {
      /* Get Block group! */
      query->query_type = ZMAP_FTOI_QUERY_BLOCK_ITEM;

      if(zMapWindowFToIFetchByQuery(window, query) &&
         (query->return_type == ZMAP_FTOI_RETURN_ITEM))
        {
          block_group = FOO_CANVAS_GROUP( query->ans.item_answer );

          /* Add the feature set */
          feature_set_name = (char *)g_quark_to_string(query->set_original_id);
          style_name       = (char *)g_quark_to_string(query->style_original_id);
          zMapWindowFeatureSetAdd(window, block_group, feature_set_name);

          /* Check we've got the correct strand, possibly a new search... */
          query->query_type = ZMAP_FTOI_QUERY_SET_ITEM;

          if(zMapWindowFToIFetchByQuery(window, query) &&
             (query->return_type == ZMAP_FTOI_RETURN_ITEM))
            {
              col_group = FOO_CANVAS_GROUP( query->ans.item_answer );
              feature   = zMapFeatureCopy(&(query->feature_in));

              style_list = zMapViewGetStyles(zmap->focus_viewwindow);

              if(!(style = zMapFindStyle(style_list, zMapStyleCreateID(style_name))))
                style = zMapFindStyle(style_list, zMapStyleCreateID(feature_set_name));

              feature->style = style;

              if(feature->style == NULL)
                {
                  response_data->code = 404;
                  g_string_append_printf(response_data->messages,
                                         "Failed to find style having name of either '%s' or '%s'.",
                                         (char *)g_quark_to_string(query->style_original_id),
                                         (char *)g_quark_to_string(query->set_original_id));
                  error_recorded = TRUE;
                  created = FALSE;
                }
              else if((feature_item = zMapWindowFeatureAdd(window, col_group, feature)) == NULL)
                created = FALSE;
              else
                {
                  /* Need to clean up feature here... */
                }
            }
          else
            {
              response_data->code = 404;
              g_string_append_printf(response_data->messages,
                                     "Failed to find column with name '%s'.",
                                     (char *)g_quark_to_string(query->set_original_id));
              error_recorded = TRUE;
            }
        }
      else
        {
          response_data->code = 404;
          g_string_append_printf(response_data->messages,
                                 "Failed to find column for feature '%s'.",
                                 (char *)g_quark_to_string(query->feature_in.original_id));
          error_recorded = TRUE;
        }
    }
  
  if(!created && !error_recorded)
    {
      response_data->code = 404;
      g_string_append_printf(response_data->messages,
                             "Failed to draw feature '%s'.",
                             (char *)g_quark_to_string(query->feature_in.original_id));
      error_recorded = TRUE;
    }

  response_data->persist = !error_recorded;

  return ;
}

static void invoke_list_foreach(GList *list, GFunc foreach_func, ZMap user_data)
{
  ResponseCodeZMapStruct foreach_data = {};

  foreach_data.code     = ZMAPXREMOTE_OK;
  foreach_data.zmap     = user_data;
  foreach_data.persist  = TRUE;
  foreach_data.messages = g_string_sized_new(200);
  g_list_foreach(list, foreach_func, &foreach_data);

  if(foreach_data.messages->str)
    zmapControlInfoSet(foreach_data.zmap, 
                       foreach_data.code, 
                       "%s", foreach_data.messages->str);
  else
    zmapControlInfoSet(foreach_data.zmap, 
                       412, 
                       "%s", "Nothing happened. Probably a data error.");

  g_string_free(foreach_data.messages, TRUE);

  return ;
}

/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *controlExecuteCommand(char *command_text, ZMap zmap, int *statusCode)
{
  char *xml_reply = NULL ;
  int code        = ZMAPXREMOTE_INTERNAL;
  XMLDataStruct objdata = {0, NULL};
  ZMapXMLParser parser = NULL;
  gboolean parse_ok    = FALSE;

  g_clear_error(&(zmap->info)); /* Clear the info */
  
  /* Create and setup the parser */
  parser = setupControlRemoteXMLParser(&objdata);
  objdata.window = zMapViewGetWindow(zmap->focus_viewwindow);
  /* Do the parsing and check all ok */
  if((parse_ok = zMapXMLParserParseBuffer(parser, 
                                          command_text, 
                                          strlen(command_text))) == TRUE)
    {
      /* Check which action  */
      switch(objdata.action)
        {
          /* PRECOND falls through to default below, so info gets set there. */
        case ZMAP_CONTROL_ACTION_ZOOM_IN:
          if(zmapControlWindowDoTheZoom(zmap, 2.0) == TRUE)
            code = ZMAPXREMOTE_OK;
          else
            code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_ZOOM_OUT:
          if(zmapControlWindowDoTheZoom(zmap, 0.5) == TRUE)
            code = ZMAPXREMOTE_OK;
          else
            code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_ZOOM_TO:
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_FIND_FEATURE:
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_CREATE_FEATURE:
          invoke_list_foreach(objdata.query_list,
                              drawNewFeature, zmap);
          break;
        case ZMAP_CONTROL_ACTION_ALTER_FEATURE:
          invoke_list_foreach(objdata.query_list,
                              alterFeature, zmap);
          break;
        case ZMAP_CONTROL_ACTION_DELETE_FEATURE:
          invoke_list_foreach(objdata.query_list,
                              deleteFeature, zmap);
          break;
        case ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE:
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE:
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_REGISTER_CLIENT:
          if(objdata.client != NULL)
            {
              if(createClient(zmap, objdata.client) == TRUE)
                code = ZMAPXREMOTE_OK;
              else
                code = ZMAPXREMOTE_BADREQUEST;
            }
          else
            code = ZMAPXREMOTE_BADREQUEST;
          break;
        case ZMAP_CONTROL_ACTION_NEW_VIEW:
          code = ZMAPXREMOTE_BADREQUEST;
          if(objdata.new_view.sequence != 0)
            {
              code = ZMAPXREMOTE_OK;
              if(!(insertView(zmap, &(objdata.new_view))))
                code = ZMAPXREMOTE_PRECOND;
            }
          zmapControlInfoSet(zmap, code,
                             "%s",
                             "a quick message about the new view");
          break;
        case ZMAP_CONTROL_ACTION_INSERT_VIEW_DATA:
          break;
        default:
          code = ZMAPXREMOTE_INTERNAL;
          break;
        }
    }
  else if(!parse_ok)
    {
      code = ZMAPXREMOTE_BADREQUEST;
      zmapControlInfoSet(zmap, code,
                         "%s",
                         zMapXMLParserLastErrorMsg(parser)
                         );
    }

  /* Now check what the status is */
  /* zmap->info is tested for truth first in case a function called
     above set the info, we don't want to kill this information as it
     might be more useful than these defaults! */
  switch (code)
    {
    case ZMAPXREMOTE_INTERNAL:
      {
        code = ZMAPXREMOTE_UNKNOWNCMD;    
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "unknown command");
        break;
      }
    case ZMAPXREMOTE_BADREQUEST:
      {
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "bad request");
        break;
      }
    case ZMAPXREMOTE_FORBIDDEN:
      {
        zmapControlInfoSet(zmap, code,
                           "<!-- request was %s -->%s",
                           command_text,
                           "forbidden request");
        break;
      }
    default:
      {
        /* If the info isn't set then someone forgot to set it */
        zmapControlInfoSet(zmap, ZMAPXREMOTE_INTERNAL,
                          "<!-- request was %s -->%s",
                          command_text,
                          "CODE error on the part of the zmap programmers.");
        break;
      }
    }

  xml_reply   = g_strdup(zmap->info->message);
  *statusCode = zmap->info->code; /* use the code from the info (GError) */

  zMapXMLParserDestroy(parser);
  
  return xml_reply;
}

static gboolean createClient(ZMap zmap, controlClientObj data)
{
  gboolean result;
  zMapXRemoteObj client;
  char *format_response = "<client created=\"%d\" exists=\"%d\" />";

  if(!zmap->client){
    if((client = zMapXRemoteNew()) != NULL)
      {
        zMapXRemoteInitClient(client, data->xid);
        zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(data->request));
        zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(data->response));
        result = TRUE;
        zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                           format_response, 1 , 1);
        zmap->client = client;
      }
    else
      {
        zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                           format_response, 0, 0);
        result = FALSE;
      }
  }
  else{
    zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                       format_response, 0, 1);
    result = FALSE;        
  }

  return result ;
}

static gboolean insertView(ZMap zmap, ViewConnectData new_view)
{
  ZMapView view = NULL;
  char *sequence;
  gboolean inserted = FALSE;
  
  if((sequence = (char *)g_quark_to_string(new_view->sequence)) &&
     new_view->config)
    {
      if((view = zMapAddView(zmap, sequence, 
                             new_view->start, 
                             new_view->end)))
        {
          if(!(zmapConnectViewConfig(zmap, view, new_view->config)))
            zMapDeleteView(zmap, view); /* Need 2 do more here. */
          else
            inserted = TRUE;
        }
    }

  return inserted;
}


static gboolean zmapStrtHndlr(gpointer userdata, 
                              ZMapXMLElement zmap_element,
                              ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  XMLData   obj  = (XMLData)userdata;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      GQuark action = zMapXMLAttributeGetValue(attr);
      if(action == g_quark_from_string("zoom_in"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_IN;
      else if(action == g_quark_from_string("zoom_out"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_OUT;
      else if(action == g_quark_from_string("zoom_to"))
        obj->action = ZMAP_CONTROL_ACTION_ZOOM_TO;  
      else if(action == g_quark_from_string("find_feature"))
        obj->action = ZMAP_CONTROL_ACTION_FIND_FEATURE;
      else if(action == g_quark_from_string("create_feature"))
        obj->action = ZMAP_CONTROL_ACTION_CREATE_FEATURE;
      /********************************************************
       * delete and create will have to do
       * else if(action == g_quark_from_string("alter_feature"))
       * obj->action = ZMAP_CONTROL_ACTION_ALTER_FEATURE;
       *******************************************************/
      else if(action == g_quark_from_string("delete_feature"))
        obj->action = ZMAP_CONTROL_ACTION_DELETE_FEATURE;
      else if(action == g_quark_from_string("highlight_feature"))
        obj->action = ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE;
      else if(action == g_quark_from_string("unhighlight_feature"))
        obj->action = ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE;
      else if(action == g_quark_from_string("create_client"))
        obj->action = ZMAP_CONTROL_ACTION_REGISTER_CLIENT;
      else if(action == g_quark_from_string("new_view"))
        obj->action = ZMAP_CONTROL_ACTION_NEW_VIEW;
    }
  else
    zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");

  return FALSE;
}

static gboolean featuresetStrtHndlr(gpointer userdata, 
                                    ZMapXMLElement set_element,
                                    ZMapXMLParser parser)
{
  ZMapWindowFToIQuery query = NULL;
  ZMapXMLAttribute    attr  = NULL;
  XMLData input = (XMLData)userdata;

  if(input && !(query = input->set))
    query = input->set = zMapWindowFToINewQuery();

  /* Isn't this fun... */
  if((attr = zMapXMLElementGetAttributeByName(set_element, "align")))
    {
      query->align_original_id = zMapXMLAttributeGetValue(attr);
    }
  if((attr = zMapXMLElementGetAttributeByName(set_element, "block")))
    {
      query->block_original_id = zMapXMLAttributeGetValue(attr);
    }
  if((attr = zMapXMLElementGetAttributeByName(set_element, "set")))
    {
      query->set_original_id = zMapXMLAttributeGetValue(attr);
    }

  return FALSE;
}

static gboolean featureStrtHndlr(gpointer userdata, 
                                 ZMapXMLElement feature_element,
                                 ZMapXMLParser parser)
{
  XMLData    data = (XMLData)userdata;
  ZMapXMLAttribute attr  = NULL;
  ZMapWindowFToIQuery new_query = NULL, query;

  zMapXMLParserCheckIfTrueErrorReturn(data->set == NULL,
                                      parser, 
                                      "feature tag not contained within featureset tag");
  
  query     = data->set;        /* Get current one */  
  /* Make a new one for this feature. */
  new_query = zMapWindowFToINewQuery(); 
  /* copy stuff accross that gets set in featuresetStrtHndlr */
  /* so it inherits data from parent */
  new_query->align_original_id = query->align_original_id;
  new_query->block_original_id = query->block_original_id;
  new_query->style_original_id = query->style_original_id;

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
    {
      new_query->feature_in.original_id = zMapXMLAttributeGetValue(attr);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "name is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "style")))
    {
      new_query->style_original_id = zMapXMLAttributeGetValue(attr);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "style is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "start")))
    {
      new_query->feature_in.x1 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                      (char **)NULL, 10);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "start is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "end")))
    {
      new_query->feature_in.x2 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                      (char **)NULL, 10);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "end is a required attribute for feature.");

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "strand")))
    {
      zMapFeatureFormatStrand((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                              &(new_query->feature_in.strand));
    }

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "score")))
    {
      new_query->feature_in.score = zMapXMLAttributeValueToDouble(attr);
    }

  if((attr = zMapXMLElementGetAttributeByName(feature_element, "suid")))
    {
      /* Nothing done here yet. */
      new_query->session_unique_id = zMapXMLAttributeGetValue(attr);
    }

  data->query_list = g_list_append(data->query_list, new_query);

  return FALSE;
}

static gboolean featureEndHndlr(void *userData, 
                                ZMapXMLElement sub_element, 
                                ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  XMLData   data = (XMLData)userData;
  ZMapFeature   feature = NULL;
  gboolean          bad = FALSE;
  ZMapWindowFToIQuery query;

  zMapXMLParserCheckIfTrueErrorReturn(data->query_list == NULL, 
                                      parser, 
                                      "a feature end tag without a created feature.");

  query   = (ZMapWindowFToIQuery)((g_list_last(data->query_list))->data);
  feature = &(query->feature_in);

  if(!bad && (attr = zMapXMLElementGetAttributeByName(sub_element, "ontology")))
    {
      GQuark ontology = zMapXMLAttributeGetValue(attr);
      ZMapSpanStruct span = {0,0};
      ZMapSpan exon_ptr = NULL, intron_ptr = NULL;
      
      feature->type = ZMAPFEATURE_TRANSCRIPT;

      if((attr = zMapXMLElementGetAttributeByName(sub_element, "start")))
        {
          span.x1 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }
      else
        zMapXMLParserRaiseParsingError(parser, "start is a required attribute for subfeature.");

      if((attr = zMapXMLElementGetAttributeByName(sub_element, "end")))
        {
          span.x2 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
        }
      else
        zMapXMLParserRaiseParsingError(parser, "end is a required attribute for subfeature.");

      /* Don't like this lower case stuff :( */
      if(ontology == g_quark_from_string("exon"))
        exon_ptr   = &span;
      if(ontology == g_quark_from_string("intron"))
        intron_ptr = &span;
      if(ontology == g_quark_from_string("cds"))
        zMapFeatureAddTranscriptData(feature, TRUE, span.x1, span.x2, FALSE, 0, FALSE, NULL, NULL);

      if(exon_ptr != NULL || intron_ptr != NULL) /* Do we need this check isn't it done internally? */
        zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr);

    }

  return TRUE;                  /* tell caller to clean us up. */
}
static gboolean segmentEndHndlr(void *user_data, 
                                ZMapXMLElement segment, 
                                ZMapXMLParser parser)
{
  XMLData xml_data = (XMLData)user_data;
  ZMapXMLAttribute attr = NULL;

  if((attr = zMapXMLElementGetAttributeByName(segment, "sequence")) != NULL)
    xml_data->new_view.sequence = zMapXMLAttributeGetValue(attr);
  if((attr = zMapXMLElementGetAttributeByName(segment, "start")) != NULL)
    xml_data->new_view.start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    xml_data->new_view.start = 1;
  if((attr = zMapXMLElementGetAttributeByName(segment, "end")) != NULL)
    xml_data->new_view.end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10);
  else
    xml_data->new_view.end = 0;

  /* Need to put contents into a source stanza buffer... */
  xml_data->new_view.config = zMapXMLElementStealContent(segment);
  
  return TRUE;
}

static gboolean clientStrtHndlr(gpointer userdata, 
                                ZMapXMLElement client_element,
                                ZMapXMLParser parser)
{
  controlClientObj clientObj = (controlClientObj)g_new0(controlClientObjStruct, 1);
  ZMapXMLAttribute xid_attr, req_attr, res_attr;
  if((xid_attr = zMapXMLElementGetAttributeByName(client_element, "id")) != NULL)
    {
      char *xid;
      xid = (char *)g_quark_to_string(zMapXMLAttributeGetValue(xid_attr));
      clientObj->xid = strtoul(xid, (char **)NULL, 16);
    }
  else
    zMapXMLParserRaiseParsingError(parser, "id is a required attribute for client.");

  if((req_attr  = zMapXMLElementGetAttributeByName(client_element, "request")) != NULL)
    clientObj->request = zMapXMLAttributeGetValue(req_attr);
  if((res_attr  = zMapXMLElementGetAttributeByName(client_element, "response")) != NULL)
    clientObj->response = zMapXMLAttributeGetValue(res_attr);

  ((XMLData)userdata)->client = clientObj;

  return FALSE;
}

static gboolean locationEndHndlr(gpointer userdata, 
                                 ZMapXMLElement zmap_element,
                                 ZMapXMLParser parser)
{
  /* we only allow one of these objects???? */
  return TRUE;
}
static gboolean styleEndHndlr(gpointer userdata, 
                              ZMapXMLElement element,
                              ZMapXMLParser parser)
{
#ifdef RDS_FIX_THIS
  ZMapFeatureTypeStyle style = NULL;
  XMLData               data = (XMLData)userdata;
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


static gboolean zmapEndHndlr(void *userData, 
                             ZMapXMLElement element, 
                             ZMapXMLParser parser)
{
  return TRUE;
}

static void alertClientToMessage(gpointer client_data, gpointer message_data) /*  */
{
  zMapXRemoteObj      client = (zMapXRemoteObj)client_data;
  AlertClientMessage message = (AlertClientMessage)message_data;
  char *response = NULL, *command = NULL;
  int result;

  command = message->full_text->str;

  zMapLogWarning("xremote sending cmd with length %d", message->full_text->len);
  zMapLogWarning("xremote cmd = %s", command);

  if((result = zMapXRemoteSendRemoteCommand(client, command, &response)) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
    {
      char *xml_only = NULL;
      int code  = 0;
      gboolean parses_ok = FALSE, error_response = FALSE;;
      ZMapXMLObjTagFunctionsStruct starts[] = {
        {"zmap",     responseZmapStartHndlr     },
        {"response", responseResponseStartHndlr },
        { NULL, NULL}
      };
      ZMapXMLObjTagFunctionsStruct ends[] = {
        {"zmap",  responseZmapEndHndlr  },
        {"error", responseErrorEndHndlr },
        { NULL, NULL}
      };
      zMapXMLParserReset(message->parser);
      zMapXMLParserSetMarkupObjectTagHandlers(message->parser, &starts[0], &ends[0]);
      
      zMapXRemoteResponseSplit(client, response, &code, &xml_only);
      if((zMapXRemoteResponseIsError(client, response)))
        error_response = TRUE;

      if((parses_ok = zMapXMLParserParseBuffer(message->parser, 
                                               xml_only, 
                                               strlen(xml_only))) != TRUE)
        {
          zMapLogWarning("Parsing error : %s", zMapXMLParserLastErrorMsg(message->parser));
        }
      else if(error_response == TRUE)
        zMapWarning("Failed to get successful response from external program.\n"
                    "Code: %d\nResponse: %s", 
                    code, 
                    (message->error_message ? message->error_message : xml_only));
    }
  else
    zMapLogWarning("Failed sending xremote command. Code = %d", result);

  return ;
}

static int xmlToMessageBuffer(ZMapXMLWriter writer, char *xml, int len, gpointer user_data)
{
  AlertClientMessage message = (AlertClientMessage)user_data;

  g_string_append_len(message->full_text, xml, len);

  return len;
}

static gboolean responseZmapStartHndlr(void *userData, 
                                       ZMapXMLElement element, 
                                       ZMapXMLParser parser)
{
  zMapLogWarning("%s", "In zmap Start Handler");
  return TRUE;
}

static gboolean responseResponseStartHndlr(void *userData, 
                                       ZMapXMLElement element, 
                                       ZMapXMLParser parser)
{
  AlertClientMessage message = (AlertClientMessage)userData;
  ZMapXMLAttribute handled_attribute = NULL;
  gboolean is_handled = FALSE;

  if((handled_attribute = zMapXMLElementGetAttributeByName(element, "handled")) &&
     (message->handled == FALSE))
    {
      is_handled = zMapXMLAttributeValueToBool(handled_attribute);
      message->handled = is_handled;
    }

  return TRUE;
}

static gboolean responseErrorEndHndlr(void *userData, 
                                      ZMapXMLElement element, 
                                      ZMapXMLParser parser)
{
  AlertClientMessage message = (AlertClientMessage)userData;
  ZMapXMLElement mess_element = NULL;

  if((mess_element = zMapXMLElementGetChildByName(element, "message")) &&
     !message->error_message &&
     mess_element->contents->str)
    {
      message->error_message = g_strdup(mess_element->contents->str);
    }
  
  return TRUE;
}

static gboolean responseZmapEndHndlr(void *userData, 
                                     ZMapXMLElement element, 
                                     ZMapXMLParser parser)
{
  zMapLogWarning("In zmap %s Handler.", "End");
  return TRUE;
}
