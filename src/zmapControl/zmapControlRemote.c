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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Implements code to respond to commands sent to a
 *              ZMap via the xremote program. The xremote program
 *              is distributed as part of acedb but is not dependent
 *              on it (although it currently makes use of acedb utils.
 *              code in the w1 directory).
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Jul  2 11:36 2007 (rds)
 * Created: Wed Nov  3 17:38:36 2004 (edgrif)
 * CVS info:   $Id: zmapControlRemote.c,v 1.52 2007-07-03 15:11:49 rds Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapFeature.h>
#include <zmapControlRemote_P.h>

typedef struct
{
  GString *full_text;
  ZMap zmap;

  ZMapXMLParser parser;
  ZMapXMLObjTagFunctions start_handlers ;
  ZMapXMLObjTagFunctions end_handlers ;

  ZMapXMLTagHandler tag_handler ;
} AlertClientMessageStruct, *AlertClientMessage;


/* ZMAPXREMOTE_CALLBACK and destroy internals for zmapControlRemoteInstaller */
static char *controlExecuteCommand(char *command_text, ZMap zmap, int *statusCode);
static void  destroyNotifyData(gpointer destroy_data); /* free attached pointer */

static gboolean createClient(ZMap zmap, controlClientObj data);
static gboolean insertView(ZMap zmap, ViewConnectData new_view);
static gboolean controlZoomTo(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data);

static void drawNewFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data);
static void eraseFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data);
static void highlightFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data);
static void delete_failed_make_message(gpointer list_data, gpointer user_data);
static void draw_failed_make_message(gpointer list_data, gpointer user_data);
static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data);

static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key, 
                                                        gpointer data, 
                                                        gpointer user_data, 
                                                        char **error_out);
static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key, 
                                                             gpointer data, 
                                                             gpointer user_data, 
                                                             char **error_out);

static void alertClientToMessage(gpointer client_data, gpointer message_data);
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

static gboolean control_execute_debug_G = FALSE;
static gboolean alert_client_debug_G    = FALSE;

void zMapAddClient(ZMap zmap, void *client_data)
{
  zMapXRemoteObj client = (zMapXRemoteObj)client_data;

  if(client)
    zmap->xremote_clients = g_list_append(zmap->xremote_clients, client);

  return ;
}

void zmapControlRemoteInstaller(GtkWidget *widget, gpointer zmap_data)
{
  ZMap zmap = (ZMap)zmap_data;
  zMapXRemoteObj xremote;

  externalPerl = FALSE;

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
#ifdef USELESS_WRITE_WINDOW_ID_FILE
      zMapConfigDirWriteWindowIdFile(id, zmap->zmap_id);
#endif
      g_signal_connect_data(G_OBJECT(widget), "property_notify_event",
                            G_CALLBACK(zMapXRemotePropertyNotifyEvent), (gpointer)notifyData,
                            (GClosureNotify) destroyNotifyData, G_CONNECT_AFTER
                            );
      
      zmap->propertyNotifyData = notifyData;
    }

  return;
}

gboolean zmapControlRemoteAlertClients(ZMap zmap,
				       char *action, GArray *xml_events,
				       ZMapXMLObjTagFunctions start_handlers,
				       ZMapXMLObjTagFunctions end_handlers,
				       gpointer *handler_data)
{
  GList *clients = NULL;
  ZMapXMLWriter xml_creator = NULL;
  AlertClientMessageStruct message_data = {NULL} ;
  ZMapXMLUtilsEventStack wrap_ptr;
  gboolean yield = FALSE;
  static ZMapXMLUtilsEventStackStruct wrap_start[] = {
    {ZMAPXML_START_ELEMENT_EVENT, "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {ZMAPXML_ATTRIBUTE_EVENT,     "action", ZMAPXML_EVENT_DATA_QUARK, {NULL}},
    {0}
  }, wrap_end[] = {
    {ZMAPXML_END_ELEMENT_EVENT,   "zmap",   ZMAPXML_EVENT_DATA_NONE,  {0}},
    {0}
  };

  /* Making this a GList, I can't afford time to make the
   * zmap->clients a glist, but this will make it easier when I do */
  if(zmap->xremote_clients)
    clients = zmap->xremote_clients;

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

      wrap_ptr = &wrap_start[1];
      wrap_ptr->value.s = action;

      xml_events = zMapXMLUtilsAddStackToEventsArrayStart(&wrap_start[0], xml_events);
      xml_events = zMapXMLUtilsAddStackToEventsArray(&wrap_end[0], xml_events);
      
      xml_creator = zMapXMLWriterCreate(xml_event_to_buffer, message_data.full_text);

      if((zMapXMLWriterProcessEvents(xml_creator, xml_events)) == ZMAPXMLWRITER_OK)
        g_list_foreach(clients, alertClientToMessage, &message_data);
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

/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */

/* Handle commands sent from xremote. */
/* Return is string in the style of ZMAP_XREMOTE_REPLY_FORMAT (see ZMap/zmapXRemote.h) */
/* Building the reply string is a bit arcane in that the xremote reply strings are really format
 * strings...perhaps not ideal...., but best in the cicrumstance I guess */
static char *controlExecuteCommand(char *command_text, ZMap zmap, int *statusCode)
{
  char *xml_reply = NULL ;
  int code        = ZMAPXREMOTE_INTERNAL;
  ResponseCodeZMapStruct foreach_data = {NULL} ;
  XMLDataStruct objdata = {NULL} ;
  ZMapXMLParser parser = NULL;
  gboolean parse_ok    = FALSE;
  ZMapView view;

  g_clear_error(&(zmap->info)); /* Clear the info */

  if(zMapXRemoteIsPingCommand(command_text, statusCode, &xml_reply) != 0)
    return xml_reply;         /* to short circuit some of this... */  

  /* Create and setup the parser */
  parser = zmapControlRemoteXMLInitialise(&objdata);

  objdata.window = zMapViewGetWindow(zmap->focus_viewwindow);

  view = zMapViewGetView(zmap->focus_viewwindow);

  objdata.orig_context = zMapViewGetFeatures(view) ;

  /* Copy basics of original context. */
  objdata.context = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)(objdata.orig_context)) ;
  objdata.context->styles = NULL ;

  objdata.styles  = objdata.orig_context->styles ;

  /* Do the parsing and check all ok */
  if((parse_ok = zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))) == TRUE)
    {
      foreach_data.code     = ZMAPXREMOTE_OK;
      foreach_data.zmap     = zmap;
      foreach_data.handled  = FALSE;
      foreach_data.messages = g_string_sized_new(512);

      /* Check which action  */
      switch(objdata.action)
        {
          /* PRECOND falls through to default below, so info gets set there. */
        case ZMAP_CONTROL_ACTION_ZOOM_IN:
          if((foreach_data.handled = zmapControlWindowDoTheZoom(zmap, 2.0)) == TRUE)
            {
              foreach_data.code = ZMAPXREMOTE_OK;
              g_string_append_printf(foreach_data.messages, "zoomed in x 2");
            }
          else
            foreach_data.code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_ZOOM_OUT:
          if((foreach_data.handled = zmapControlWindowDoTheZoom(zmap, 0.5)) == TRUE)
            {
              foreach_data.code = ZMAPXREMOTE_OK;
              g_string_append_printf(foreach_data.messages, "zoomed out x 2");
            }
          else
            foreach_data.code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_ZOOM_TO:
          if((foreach_data.handled = controlZoomTo(zmap, &objdata, &foreach_data)) == TRUE)
            foreach_data.code = ZMAPXREMOTE_OK;
          else
            foreach_data.code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_FIND_FEATURE:
          foreach_data.code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_CREATE_FEATURE:
          drawNewFeatures(zmap, &objdata, &foreach_data);
          break;
        case ZMAP_CONTROL_ACTION_ALTER_FEATURE:
          foreach_data.code = ZMAPXREMOTE_PRECOND;
          break;
        case ZMAP_CONTROL_ACTION_DELETE_FEATURE:
          eraseFeatures(zmap, &objdata, &foreach_data);
          break;
        case ZMAP_CONTROL_ACTION_HIGHLIGHT_FEATURE:
        case ZMAP_CONTROL_ACTION_HIGHLIGHT2_FEATURE:
          highlightFeatures(zmap, &objdata, &foreach_data);
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_UNHIGHLIGHT_FEATURE:
          code = ZMAPXREMOTE_OK;
          break;
        case ZMAP_CONTROL_ACTION_REGISTER_CLIENT:
          if(objdata.client != NULL)
            {
              if((foreach_data.handled = createClient(zmap, objdata.client)) == TRUE)
                foreach_data.code = ZMAPXREMOTE_OK;
              else
                foreach_data.code = ZMAPXREMOTE_BADREQUEST;
            }
          else
            foreach_data.code = ZMAPXREMOTE_BADREQUEST;
          break;
        case ZMAP_CONTROL_ACTION_NEW_VIEW:
          foreach_data.code = ZMAPXREMOTE_BADREQUEST;
          if(objdata.new_view.sequence != 0)
            {
              foreach_data.code = ZMAPXREMOTE_OK;
              if(!(foreach_data.handled = insertView(zmap, &(objdata.new_view))))
                foreach_data.code = ZMAPXREMOTE_PRECOND;
            }
          else
            zmapControlInfoSet(zmap, code, "%s",
                               "Missing sequence parameter.");
          break;
        case ZMAP_CONTROL_ACTION_INSERT_VIEW_DATA:
          break;
        default:
          foreach_data.handled = FALSE;
          foreach_data.code    = ZMAPXREMOTE_BADREQUEST;
          break;
        }

      if(foreach_data.messages->str)
        zmapControlInfoSet(zmap, foreach_data.code, 
                           "%s", foreach_data.messages->str);
      else if(foreach_data.handled)
        zmapControlInfoSet(zmap, 412, "%s", 
                           "Request handled, but no message set. Probably a data error.");
      else
        { zMapLogWarning("%s", "Request not handled!"); }

      code = foreach_data.code;
      g_string_free(foreach_data.messages, TRUE);

      memset(&foreach_data, 0, sizeof(ResponseCodeZMapStruct));
    }
  else if(!parse_ok)
    {
      code = ZMAPXREMOTE_BADREQUEST;
      zmapControlInfoSet(zmap, code, "%s",
                         zMapXMLParserLastErrorMsg(parser));
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

  if(control_execute_debug_G)
    zMapLogWarning("Destroying created/copied context (%p)", objdata.context);


  if(objdata.context)
    zMapFeatureContextDestroy(objdata.context, TRUE);


  zMapXMLParserDestroy(parser);
  
  return xml_reply;
}

static void destroyNotifyData(gpointer destroy_data)
{
  ZMap zmap;
  zMapXRemoteNotifyData destroy_me = (zMapXRemoteNotifyData)destroy_data;

  zmap = (ZMap)(destroy_me->data);
  zmap->propertyNotifyData = NULL; /* Set this to null, as we're emptying the mem */
  g_free(destroy_me);           /* Is this all we need to destroy?? */

  return ;
}


static gboolean createClient(ZMap zmap, controlClientObj data)
{
  gboolean result;
  zMapXRemoteObj client;
  char *format_response = "<client created=\"%d\" exists=\"%d\" />";

  if((client = zMapXRemoteNew()) != NULL)
    {
      zMapXRemoteInitClient(client, data->xid);
      zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(data->request));
      zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(data->response));
      result = TRUE;
      zMapAddClient(zmap, client);
      zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                         format_response, 1 , 1);
    }
  else
    {
      zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                         format_response, 0, 0);
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
          zMapViewReadConfigBuffer(view, new_view->config);
          if(!(zmapConnectViewConfig(zmap, view, new_view->config)))
            zMapDeleteView(zmap, view); /* Need 2 do more here. */
          else
            {
              zmapControlInfoSet(zmap, ZMAPXREMOTE_OK,
                                 "new_view=%s", sequence);
              inserted = TRUE;
            }
        }
    }

  if(!inserted)
    zmapControlInfoSet(zmap, ZMAPXREMOTE_PRECOND, "%s", "Failed to add view");

  return inserted;
}


static void delete_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  ResponseCodeZMap response_data = (ResponseCodeZMap)user_data;


  if(feature_any->struct_type == ZMAPFEATURE_STRUCT_INVALID)
    {
      response_data->code = 404;
      g_string_append_printf(response_data->messages,
                             "Failed to delete feature '%s' [%s].\n",
                             (char *)g_quark_to_string(feature_any->original_id),
                             (char *)g_quark_to_string(feature_any->unique_id));
    }

  return ;
}

static void draw_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  ResponseCodeZMap response_data = (ResponseCodeZMap)user_data;

  response_data->code = 404;

  g_string_append_printf(response_data->messages,
                         "Failed to draw feature '%s' [%s].\n",
                         (char *)g_quark_to_string(feature_any->original_id),
                         (char *)g_quark_to_string(feature_any->unique_id));

  return ;
}

static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data)
{
  gint match = -1;
  ZMapFeatureAny 
    a = (ZMapFeatureAny)list_data, 
    b = (ZMapFeatureAny)user_data;

  match = !(a->unique_id == b->unique_id);
    
  return match;
}

static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key, 
                                                        gpointer data, 
                                                        gpointer user_data, 
                                                        char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  GList **list = (GList **)user_data, *match;

  if(any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if((match = g_list_find_custom(*list, any, matching_unique_id)))
        {
          *list = g_list_remove(*list, match->data);
        }
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}

static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key, 
                                                             gpointer data, 
                                                             gpointer user_data,
                                                             char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  GList **list = (GList **)user_data, *match;

  if(any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if((match = g_list_find_custom(*list, any, matching_unique_id)))
        {
          any = (ZMapFeatureAny)(match->data);
          any->struct_type = ZMAPFEATURE_STRUCT_INVALID;
        }
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}

static void eraseFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data)
{
  ZMapView view;

  view = zMapViewGetView(zmap->focus_viewwindow);

  zMapViewEraseFromContext(view, xml_data->context);

  zMapFeatureContextExecute((ZMapFeatureAny)(xml_data->context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            mark_matching_invalid,
                            &(xml_data->feature_list));

  if(g_list_length(xml_data->feature_list))
    g_list_foreach(xml_data->feature_list, delete_failed_make_message, rc_data);

  rc_data->handled = TRUE;

  return ;
}

static void drawNewFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data)
{
  ZMapView view;

  view = zMapViewGetView(zmap->focus_viewwindow);

  zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(xml_data->context)) ;

  xml_data->context = zMapViewMergeInContext(view, xml_data->context) ;

  zMapFeatureContextExecute((ZMapFeatureAny)(xml_data->context), ZMAPFEATURE_STRUCT_FEATURE,
                            delete_from_list,
                            &(xml_data->feature_list));

  if(g_list_length(xml_data->feature_list))
    g_list_foreach(xml_data->feature_list, draw_failed_make_message, rc_data);

  rc_data->handled = TRUE;

  return ;
}

static void highlightFeatures(ZMap zmap, XMLData xml_data, ResponseCodeZMap rc_data)
{
  ZMapView view;

  view = zMapViewGetView(zmap->focus_viewwindow);

  zMapViewHighlightFeatures(view, NULL, xml_data->context, 
                            (xml_data->action == ZMAP_CONTROL_ACTION_HIGHLIGHT2_FEATURE ? TRUE : FALSE));

  g_string_append_printf(rc_data->messages, "Highlighted objects");

  rc_data->handled = TRUE;

  return ;
}

static ZMapFeatureContextExecuteStatus zoomToFeatureCB(GQuark key, 
                                                       gpointer data, 
                                                       gpointer user_data,
                                                       char **error_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  ZMapWindow window = (ZMapWindow)user_data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      zMapWindowZoomToFeature(window, (ZMapFeature)feature_any);      
      break;
    default:
      break;
    }

  return status;
}


static gboolean controlZoomTo(ZMap zmap, XMLData xml_data, ResponseCodeZMap foreach_data)
{
  ZMapWindow window;
  gboolean status = TRUE;
  GList *list;
  ZMapSpan span;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  if((list = g_list_first(xml_data->feature_list)))
    {
      zMapFeatureContextExecute((ZMapFeatureAny)(xml_data->context), ZMAPFEATURE_STRUCT_FEATURE,
                                zoomToFeatureCB, window);
    }
  else if((list = g_list_first(xml_data->locations)))
    {
      span = (ZMapSpan)(list->data);
      zMapWindowZoomToWorldPosition(window, FALSE, 
                                    0.0, span->x1,
                                    100.0, span->x2);
    }
  else
    status = FALSE;

  return status;
}



static void alertClientToMessage(gpointer client_data, gpointer user_data) /*  */
{
  zMapXRemoteObj      client = (zMapXRemoteObj)client_data;
  AlertClientMessage message_data = (AlertClientMessage)user_data;
  char *response = NULL, *command = NULL;
  int result;

  command = message_data->full_text->str;

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
      ZMapXMLObjTagFunctionsStruct starts[] = {
        {"zmap",     xml_zmap_start_cb     },
        {"response", xml_response_start_cb },
        { NULL, NULL}
      };
      ZMapXMLObjTagFunctionsStruct ends[] = {
        {"zmap",  xml_zmap_end_cb  },
        {"error", xml_error_end_cb },
        { NULL, NULL}
      };
      ZMapXMLObjTagFunctions start, end ;

      start = &starts[0] ;
      end = &ends[0] ;

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

static gboolean xml_zmap_end_cb(gpointer user_data, ZMapXMLElement element, 
                                ZMapXMLParser parser)
{
  if(alert_client_debug_G)
    zMapLogWarning("In zmap %s Handler.", "End");
  return TRUE;
}
