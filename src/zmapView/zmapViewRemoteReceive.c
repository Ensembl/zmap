/*  File: zmapViewRemoteRequests.c
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
 * Last edited: Feb  4 14:06 2009 (edgrif)
 * Created: Tue Jul 10 21:02:42 2007 (rds)
 * CVS info:   $Id: zmapViewRemoteReceive.c,v 1.24 2009-02-04 16:16:30 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmapGFF.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapView_P.h>

#define VIEW_POST_EXECUTE_DATA "xremote_post_execute_data"

typedef enum
  {
    ZMAPVIEW_REMOTE_INVALID,
    /* Add below here... */

    ZMAPVIEW_REMOTE_FIND_FEATURE,
    ZMAPVIEW_REMOTE_CREATE_FEATURE,
    ZMAPVIEW_REMOTE_DELETE_FEATURE,
    ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE,
    ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE,
    ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE,
    ZMAPVIEW_REMOTE_REGISTER_CLIENT,
    ZMAPVIEW_REMOTE_LIST_WINDOWS,
    ZMAPVIEW_REMOTE_NEW_WINDOW,

    ZMAPVIEW_REMOTE_DUMP_CONTEXT,

    /* ...but above here */
    ZMAPVIEW_REMOTE_UNKNOWN
  }ZMapViewValidXRemoteActions;

typedef struct
{
  ZMapView           view;

  ZMapFeatureContext orig_context;
  ZMapFeatureContext edit_context;

  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
  ZMapFeatureSet       feature_set;
  ZMapFeature          feature;

  GList             *feature_list;
  GData             *styles;

  char *filename;
  char *format;
}RequestDataStruct, *RequestData;

typedef struct
{
  int      code;
  gboolean handled;
  GString *messages;
}ResponseDataStruct, *ResponseData;

typedef struct
{
  ZMapViewValidXRemoteActions action;
  ZMapFeatureContext edit_context;
}PostExecuteDataStruct, *PostExecuteData;

static char *view_execute_command(char *command_text, gpointer user_data, int *statusCode);
static char *view_post_execute(char *command_text, gpointer user_data, int *statusCode);
static void delete_failed_make_message(gpointer list_data, gpointer user_data);
static void drawNewFeatures(ZMapView view, RequestData input_data, ResponseData output_data);
static void getChildWindowXID(ZMapView view, RequestData input_data, ResponseData output_data);
static void viewDumpContextToFile(ZMapView view, RequestData input_data, ResponseData output_data);
static gboolean sanityCheckContext(ZMapView view, RequestData input_data, ResponseData output_data);
static void draw_failed_make_message(gpointer list_data, gpointer user_data);
static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data);
static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key, gpointer data, gpointer user_data, char **error_out);
static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key, gpointer data, gpointer user_data, char **error_out);
static ZMapFeatureContextExecuteStatus sanity_check_context(GQuark key, 
                                                            gpointer data, 
                                                            gpointer user_data,
                                                            char **error_out);
static void createClient(ZMapView view, ZMapXRemoteParseCommandData input_data, ResponseData output_data);
static void eraseFeatures(ZMapView view, RequestData input_data, ResponseData output_data);
static void populate_data_from_view(ZMapView view, RequestData xml_data);
static gboolean setupStyles(ZMapFeatureContext context,
			    ZMapFeatureSet set, ZMapFeature feature, 
                            GData *styles, GQuark style_id);

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_export_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);

static gboolean xml_return_true_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);



static ZMapXMLObjTagFunctionsStruct view_starts_G[] = {
  { "zmap",       xml_zmap_start_cb                  },
  { "client",     zMapXRemoteXMLGenericClientStartCB },
  { "export",     xml_export_start_cb                },
  { "featureset", xml_featureset_start_cb            },
  { "feature",    xml_feature_start_cb               },
  {NULL, NULL}
};
static ZMapXMLObjTagFunctionsStruct view_ends_G[] = {
  { "zmap",       xml_return_true_cb    },
  { "feature",    xml_return_true_cb    },
  { "subfeature", xml_subfeature_end_cb },
  {NULL, NULL}
};

static char *actions_G[ZMAPVIEW_REMOTE_UNKNOWN + 1] = {
  NULL, "find_feature", "create_feature", "delete_feature",
  "single_select", "multiple_select", "unselect",
  "register_client", "list_windows", "new_window",
  "export_context",
  NULL
};

/* Where is all starts from. Everything else should be static */
void zmapViewSetupXRemote(ZMapView view, GtkWidget *widget)
{
  zMapXRemoteInitialiseWidgetFull(widget, PACKAGE_NAME, 
				  ZMAP_DEFAULT_REQUEST_ATOM_NAME, 
				  ZMAP_DEFAULT_RESPONSE_ATOM_NAME, 
				  view_execute_command, 
				  view_post_execute, 
				  view);
  return ;
}

char *zMapViewRemoteReceiveAccepts(ZMapView view)
{
  char *xml = NULL;

  xml = zMapXRemoteClientAcceptsActionsXML(zMapXRemoteWidgetGetXID(view->xremote_widget), 
                                           &actions_G[ZMAPVIEW_REMOTE_INVALID + 1], 
                                           ZMAPVIEW_REMOTE_UNKNOWN - 1);

  return xml;
}

static char *view_post_execute(char *command_text, gpointer user_data, int *statusCode)
{
  ZMapView view = (ZMapView)user_data;
  PostExecuteData post_data;
  gboolean status;

  if(!view->xremote_widget)
    return NULL;		/* N.B. Early return. */

  /* N.B. We _steal_ the g_object data here! */
  if((post_data = g_object_steal_data(G_OBJECT(view->xremote_widget), VIEW_POST_EXECUTE_DATA)))
    {
      switch(post_data->action)
	{
	case ZMAPVIEW_REMOTE_CREATE_FEATURE:
	  {
	    status = zmapViewDrawDiffContext(view, &(post_data->edit_context));
	    
	    if(!status)
	      post_data->edit_context = NULL; /* So the view->features context doesn't get destroyed */
	    
	    if(post_data->edit_context)
	      zMapFeatureContextDestroy(post_data->edit_context, TRUE);
	  }
	  break;
	default:
	  zMapAssertNotReached();
	  break;
	}
      g_free(post_data);
    }

  return NULL;
}

/* The ZMapXRemoteCallback */
static char *view_execute_command(char *command_text, gpointer user_data, int *statusCode)
{
  ZMapXMLParser parser;
  ZMapXRemoteParseCommandDataStruct input = { NULL };
  RequestDataStruct input_data = {0};
  ZMapView view = (ZMapView)user_data;
  char *response = NULL;

  if(zMapXRemoteIsPingCommand(command_text, statusCode, &response) != 0)
    {
      goto HAVE_RESPONSE;
    }

  input_data.view = view;
  input.user_data = &input_data;

  parser = zMapXMLParserCreate(&input, FALSE, FALSE);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &view_starts_G[0], &view_ends_G[0]);

  if((zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))))
    {
      ResponseDataStruct output_data = {0};

      output_data.code = 0;
      output_data.messages = g_string_sized_new(512);

      switch(input.common.action)
        {
        case ZMAPVIEW_REMOTE_FIND_FEATURE:
          break;
        case ZMAPVIEW_REMOTE_DELETE_FEATURE:
          eraseFeatures(view, &input_data, &output_data);
          break;
        case ZMAPVIEW_REMOTE_CREATE_FEATURE:
          if(sanityCheckContext(view, &input_data, &output_data))
	    {
	      drawNewFeatures(view, &input_data, &output_data);

	      /* slice the input_data into the post_data to make the view_post_execute happy. */
	      if(view->xremote_widget && input_data.edit_context)
		{
		  PostExecuteData post_data = g_new0(PostExecuteDataStruct, 1);

		  post_data->action       = input.common.action;
		  post_data->edit_context = input_data.edit_context;

		  g_object_set_data(G_OBJECT(view->xremote_widget), 
				    VIEW_POST_EXECUTE_DATA, 
				    post_data);
		}

	      input_data.edit_context = NULL;
	    }
          break;
        case ZMAPVIEW_REMOTE_REGISTER_CLIENT:
          createClient(view, &input, &output_data);
          break;
        case ZMAPVIEW_REMOTE_LIST_WINDOWS:
          getChildWindowXID(view, &input_data, &output_data);
          break;
	case ZMAPVIEW_REMOTE_DUMP_CONTEXT:
	  viewDumpContextToFile(view, &input_data, &output_data);
	  break;
        case ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE:
        case ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE:
        case ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE:
        case ZMAPVIEW_REMOTE_NEW_WINDOW:
          //newWindowForView(view, &input, &output_data);
          //break;
        case ZMAPVIEW_REMOTE_INVALID:
        case ZMAPVIEW_REMOTE_UNKNOWN:
        default:
          g_string_append_printf(output_data.messages, "%s", "Unknown command");
          output_data.code = ZMAPXREMOTE_UNKNOWNCMD;
          break;
        }
      
      *statusCode = output_data.code;
      response    = g_string_free(output_data.messages, FALSE);
    }
  else
    {
      *statusCode = ZMAPXREMOTE_BADREQUEST;
      response    = g_strdup(zMapXMLParserLastErrorMsg(parser));
    }

  if(input_data.edit_context)
    zMapFeatureContextDestroy(input_data.edit_context, TRUE);

  zMapXMLParserDestroy(parser);

 HAVE_RESPONSE:
  if(!zMapXRemoteValidateStatusCode(statusCode) && response != NULL)
    {
      zMapLogWarning("%s", response);
      g_free(response);
      response = g_strdup("Broken code. Check zmap.log file"); 
    }
  if(response == NULL){ response = g_strdup("Broken code."); }
    
  return response;
}

static void createClient(ZMapView view, ZMapXRemoteParseCommandData input_data, ResponseData output_data)
{
  ZMapXRemoteObj client;
  ClientParameters client_params = &(input_data->common.client_params);
  char *format_response = "<client created=\"%d\" exists=\"%d\" />";
  int created, exists;

  if(!(view->xremote_client) && (client = zMapXRemoteNew()) != NULL)
    {
      zMapXRemoteInitClient(client, client_params->xid);
      zMapXRemoteSetRequestAtomName(client, (char *)g_quark_to_string(client_params->request));
      zMapXRemoteSetResponseAtomName(client, (char *)g_quark_to_string(client_params->response));

      view->xremote_client = client;
      created = 1; exists = 0;
    }
  else if(view->xremote_client)
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

static void getChildWindowXID(ZMapView view, RequestData input_data, ResponseData output_data)
{

  if(view->state != ZMAPVIEW_LOADED)
    {
      output_data->code = ZMAPXREMOTE_PRECOND;
      g_string_append_printf(output_data->messages, "%s",
                             "view isn't loaded yet");
    }
  else
    {
      GList *list_item;

      list_item = g_list_first(view->window_list);

      do
        {
          ZMapViewWindow view_window;
          ZMapWindow window;
          char *client_xml = NULL;

          view_window = (ZMapViewWindow)list_item->data;
          window      = zMapViewGetWindow(view_window);
          client_xml  = zMapWindowRemoteReceiveAccepts(window);

          g_string_append_printf(output_data->messages,
                                 "%s", client_xml);
          if(client_xml)
            g_free(client_xml);
        }
      while((list_item = g_list_next(list_item))) ;

      output_data->code = ZMAPXREMOTE_OK;
    }

  return ;
}

static void viewDumpContextToFile(ZMapView view, RequestData input_data, ResponseData output_data)
{
  GIOChannel *file = NULL;
  GError *error = NULL;
  char *filepath = NULL;

  filepath = input_data->filename;

  if(!(file = g_io_channel_new_file(filepath, "w", &error)))
    {
      output_data->code = ZMAPXREMOTE_UNAVAILABLE;
      output_data->handled = FALSE;
      if(error)
	g_string_append(output_data->messages, error->message);
    }
  else
    {
      char *format = NULL;
      gboolean result = FALSE;

      format = input_data->format;

      if(format && g_ascii_strcasecmp(format, "gff") == 0)
	result = zMapGFFDump((ZMapFeatureAny)view->features, view->orig_styles, file, &error);
#ifdef STYLES_PRINT_TO_FILE
      else if(format && g_ascii_strcasecmp(format, "test-style") == 0)
	zMapFeatureTypePrintAll(view->features->styles, __FILE__);
#endif /* STYLES_PRINT_TO_FILE */
      else
	result = zMapFeatureContextDump(view->features, view->orig_styles, file, &error);

      if(!result)
	{
	  output_data->code    = ZMAPXREMOTE_INTERNAL;
	  output_data->handled = FALSE;
	  if(error)
	    g_string_append(output_data->messages, error->message);
	}
      else
	{
	  output_data->code    = ZMAPXREMOTE_OK;
	  output_data->handled = TRUE;
	}
    }

  return ;
}

static gboolean sanityCheckContext(ZMapView view, RequestData input_data, ResponseData output_data)
{
  gboolean features_are_sane = TRUE;
  ZMapXRemoteStatus save_code;

  save_code = output_data->code;
  output_data->code = 0;

  zMapFeatureContextExecute((ZMapFeatureAny)(input_data->edit_context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            sanity_check_context,
                            output_data);

  if(output_data->code != 0)
    features_are_sane = FALSE;
  else
    output_data->code = save_code;

  return features_are_sane;
}

static void eraseFeatures(ZMapView view, RequestData input_data, ResponseData output_data)
{
  zmapViewEraseFromContext(view, input_data->edit_context);

  zMapFeatureContextExecute((ZMapFeatureAny)(input_data->edit_context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            mark_matching_invalid,
                            &(input_data->feature_list));

  output_data->code = 0;

  if(g_list_length(input_data->feature_list))
    g_list_foreach(input_data->feature_list, delete_failed_make_message, output_data);

  /* if delete_failed_make_message didn't change the code then all is ok */
  if(output_data->code == 0)
    output_data->code = ZMAPXREMOTE_OK;

  output_data->handled = TRUE;

  return ;
}

static void drawNewFeatures(ZMapView view, RequestData input_data, ResponseData output_data)
{
  zMapFeatureAnyForceModesToStyles((ZMapFeatureAny)(input_data->edit_context), NULL) ;
  
  input_data->edit_context = zmapViewMergeInContext(view, input_data->edit_context) ;
  
  zMapFeatureContextExecute((ZMapFeatureAny)(input_data->edit_context), 
                            ZMAPFEATURE_STRUCT_FEATURE,
                            delete_from_list,
                            &(input_data->feature_list));

  zMapFeatureContextExecute((ZMapFeatureAny)(view->features), 
                            ZMAPFEATURE_STRUCT_FEATURE,
                            mark_matching_invalid,
                            &(input_data->feature_list));

  output_data->code = 0;

  if(g_list_length(input_data->feature_list))
    g_list_foreach(input_data->feature_list, draw_failed_make_message, output_data);

  if(output_data->code == 0)
    output_data->code = ZMAPXREMOTE_OK;

  output_data->handled = TRUE;

  return ;
}


static void draw_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  ResponseData response_data = (ResponseData)user_data;

  response_data->code = ZMAPXREMOTE_CONFLICT; /* possibly the wrong code */

  if(feature_any->struct_type == ZMAPFEATURE_STRUCT_INVALID)
    {
      g_string_append_printf(response_data->messages,
			     "Failed to draw feature '%s' [%s]. Feature already exists.\n",
			     (char *)g_quark_to_string(feature_any->original_id),
			     (char *)g_quark_to_string(feature_any->unique_id));
    }
  else
    {
      g_string_append_printf(response_data->messages,
			     "Failed to draw feature '%s' [%s]. Unknown reason.\n",
			     (char *)g_quark_to_string(feature_any->original_id),
			     (char *)g_quark_to_string(feature_any->unique_id));      
    }

  return ;
}

static void delete_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  ResponseData response_data = (ResponseData)user_data;

  if(feature_any->struct_type == ZMAPFEATURE_STRUCT_INVALID)
    {
      response_data->code = ZMAPXREMOTE_CONFLICT; /* possibly the wrong code */
      g_string_append_printf(response_data->messages,
                             "Failed to delete feature '%s' [%s].\n",
                             (char *)g_quark_to_string(feature_any->original_id),
                             (char *)g_quark_to_string(feature_any->unique_id));
    }

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

static ZMapFeatureContextExecuteStatus sanity_check_context(GQuark key, 
                                                            gpointer data, 
                                                            gpointer user_data,
                                                            char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  ResponseData output_data = (ResponseData)user_data;
  char *reason = NULL;

  if(!zMapFeatureAnyIsSane(any, &reason))
    {
      output_data->code = ZMAPXREMOTE_BADREQUEST;
      output_data->messages = g_string_append(output_data->messages, reason);
    }

  if(reason)
    g_free(reason);

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}


static void populate_data_from_view(ZMapView view, RequestData xml_data)
{
  xml_data->orig_context = zMapViewGetFeatures(view) ;
  
  /* Copy basics of original context. */
  xml_data->edit_context = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)(xml_data->orig_context)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  xml_data->edit_context->styles = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  xml_data->styles  = xml_data->orig_context->styles ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  xml_data->styles  = view->orig_styles ;
  

  
  return ;
}




/* ====================================================== */
/*                 XML_TAG_HANDLERS                       */
/* ====================================================== */

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
 *
 * <zmap action="shutdown" />
 *
 * <zmap action="new">
 *   <segment sequence="U.2969591-3331416" start="1" end="" />
 * </zmap>
 *
 *  <zmap action="delete_feature">
 *    <featureset>
 *      <feature name="U.2969591-3331416" start="87607" end="87608" strand="+" style="polyA_site" score="0.000"></feature>
 *    </featureset>
 *  </zmap>
 *
 */

static gboolean xml_zmap_start_cb(gpointer user_data, 
                                  ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  ZMapXMLAttribute attr = NULL;
  GQuark action = 0;

  if((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      int i;
      action = zMapXMLAttributeGetValue(attr);

      xml_data->common.action = ZMAPVIEW_REMOTE_INVALID;

      for(i = ZMAPVIEW_REMOTE_INVALID + 1; i < ZMAPVIEW_REMOTE_UNKNOWN; i++)
        {
          if(action == g_quark_from_string(actions_G[i]))
            xml_data->common.action = i;
        }

      /* unless((action > INVALID) and (action < UNKNOWN)) */
      if(!(xml_data->common.action > ZMAPVIEW_REMOTE_INVALID &&
           xml_data->common.action < ZMAPVIEW_REMOTE_UNKNOWN))
        {
          zMapLogWarning("action='%s' is unknown", g_quark_to_string(action));
          xml_data->common.action = ZMAPVIEW_REMOTE_UNKNOWN;
        }
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");
      xml_data->common.action = ZMAPVIEW_REMOTE_INVALID;
    }

  switch(xml_data->common.action)
    {
    case ZMAPVIEW_REMOTE_FIND_FEATURE:
    case ZMAPVIEW_REMOTE_CREATE_FEATURE:
    case ZMAPVIEW_REMOTE_DELETE_FEATURE:
    case ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE:
    case ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE:
    case ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE:
      {
        RequestData request_data = (RequestData)(xml_data->user_data);
        populate_data_from_view(request_data->view, request_data);
      }
      break;
    case ZMAPVIEW_REMOTE_LIST_WINDOWS:
    case ZMAPVIEW_REMOTE_REGISTER_CLIENT:
    case ZMAPVIEW_REMOTE_NEW_WINDOW:
    case ZMAPVIEW_REMOTE_DUMP_CONTEXT:
      break;
    default:
      xml_data->common.action = ZMAPVIEW_REMOTE_INVALID;
      zMapLogWarning("%s", "action set to ZMAPVIEW_REMOTE_INVALID");
      break;
    }


  return TRUE;
}

static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement set_element,
                                        ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
  ZMapFeatureSet feature_set;
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  GQuark align_id, block_id, set_id;
  char *align_name, *block_seq, *set_name;

  if(xml_data->common.action == ZMAPVIEW_REMOTE_INVALID)
    return FALSE;

  /* Isn't this fun... */
  if((attr = zMapXMLElementGetAttributeByName(set_element, "align")))
    {
      align_id   = zMapXMLAttributeGetValue(attr);
      align_name = (char *)g_quark_to_string(align_id);

      /* look for align in context. try master and non-master ids */
      if(!(align = zMapFeatureContextGetAlignmentByID(request_data->edit_context, zMapFeatureAlignmentCreateID(align_name, TRUE))) &&
         !(align = zMapFeatureContextGetAlignmentByID(request_data->edit_context, zMapFeatureAlignmentCreateID(align_name, FALSE))))
        {
          /* not there...create... */
          align = zMapFeatureAlignmentCreate(align_name, FALSE);
        }

      request_data->align = align;
    }
  else
    {
      request_data->align = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_context->master_align)) ;
    }
  zMapFeatureContextAddAlignment(request_data->edit_context, request_data->align, FALSE);



  if((attr = zMapXMLElementGetAttributeByName(set_element, "block")))
    {
      int ref_start, ref_end, non_start, non_end;
      ZMapStrand ref_strand, non_strand;

      block_id  = zMapXMLAttributeGetValue(attr);

      if(zMapFeatureBlockDecodeID(block_id, &ref_start, &ref_end, &ref_strand,
                                  &non_start, &non_end, &non_strand))
        {
          if(!(block = zMapFeatureAlignmentGetBlockByID(request_data->align, block_id)))
            {
              block_seq = (char *)g_quark_to_string(request_data->align->original_id);
              block = zMapFeatureBlockCreate(block_seq,
                                             ref_start, ref_end, ref_strand,
                                             non_start, non_end, non_strand);
            }
        }
      else
        {
          /* Get the first one! */
          block = zMap_g_hash_table_nth(request_data->align->blocks, 0) ;
        }

      request_data->block = block;
    }
  else
    {
      /* Get the first one! */
      request_data->block = (ZMapFeatureBlock)zMapFeatureAnyCopy(zMap_g_hash_table_nth(request_data->orig_context->master_align->blocks, 0)) ;
    }
  zMapFeatureAlignmentAddBlock(request_data->align, request_data->block);



  if((attr = zMapXMLElementGetAttributeByName(set_element, "set")))
    {
      set_id   = zMapXMLAttributeGetValue(attr);
      set_name = (char *)g_quark_to_string(set_id);
      set_id   = zMapFeatureSetCreateID(set_name);

      if(!(feature_set = zMapFeatureBlockGetSetByID(request_data->block, set_id)))
        {
          feature_set = zMapFeatureSetCreate(set_name, NULL);
          zMapFeatureBlockAddFeatureSet(request_data->block, feature_set);
        }
      request_data->feature_set = feature_set;


      request_data->edit_context->feature_set_names = g_list_append(request_data->edit_context->feature_set_names,
							   GINT_TO_POINTER(set_id)) ;

    }
  else
    {
      /* the feature handler will rely on the style to find the feature set... */
      request_data->feature_set = NULL;
    }

  return FALSE;
}

static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element,
                                     ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapFeatureAny feature_any;
  ZMapStrand strand = ZMAPSTRAND_NONE;
  GQuark feature_name_q = 0, style_q = 0, style_id;
  gboolean has_score = FALSE, start_not_found = FALSE, end_not_found = FALSE;
  char *feature_name, *style_name;
  int start = 0, end = 0, start_phase = ZMAPPHASE_NONE;
  double score = 0.0;

  switch(xml_data->common.action)
    {
    case ZMAPVIEW_REMOTE_CREATE_FEATURE:
    case ZMAPVIEW_REMOTE_DELETE_FEATURE:
    case ZMAPVIEW_REMOTE_FIND_FEATURE:
    case ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE:
    case ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE:
    case ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE:
      {
        zMapXMLParserCheckIfTrueErrorReturn(request_data->block == NULL,
                                            parser, 
                                            "feature tag not contained within featureset tag");
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
          {
            feature_name_q = zMapXMLAttributeGetValue(attr);
          }
        else
          zMapXMLParserRaiseParsingError(parser, "name is a required attribute for feature.");
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "style")))
          {
            style_q = zMapXMLAttributeGetValue(attr);
          }
        else
          zMapXMLParserRaiseParsingError(parser, "style is a required attribute for feature.");
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "start")))
          {
            start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                           (char **)NULL, 10);
          }
        else
          zMapXMLParserRaiseParsingError(parser, "start is a required attribute for feature.");
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "end")))
          {
            end = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                         (char **)NULL, 10);
          }
        else
          zMapXMLParserRaiseParsingError(parser, "end is a required attribute for feature.");
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "strand")))
          {
            zMapFeatureFormatStrand((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                                    &(strand));
          }
        
        if((attr = zMapXMLElementGetAttributeByName(feature_element, "score")))
          {
            score = zMapXMLAttributeValueToDouble(attr);
            has_score = TRUE;
          }

        if((attr = zMapXMLElementGetAttributeByName(feature_element, "start_not_found")))
          {
            start_phase = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)), 
                                 (char **)NULL, 10);
            start_not_found = TRUE;
            switch(start_phase)
              {
              case 1:
              case -1:
                start_phase = ZMAPPHASE_0;
                break;
              case 2:
              case -2:
                start_phase = ZMAPPHASE_1;
                break;
              case 3:
              case -3:
                start_phase = ZMAPPHASE_2;
                break;
              default:
                start_not_found = FALSE;
                start_phase = ZMAPPHASE_NONE;
                break;
              }
          }

        if((attr = zMapXMLElementGetAttributeByName(feature_element, "end_not_found")))
          {
            end_not_found = zMapXMLAttributeValueToBool(attr);
          }

        if((attr = zMapXMLElementGetAttributeByName(feature_element, "suid")))
          {
            /* Nothing done here yet. */
            zMapXMLAttributeGetValue(attr);
          }
        
        if(!zMapXMLParserLastErrorMsg(parser))
          {
            style_name = (char *)g_quark_to_string(style_q);
            style_id   = zMapStyleCreateID(style_name);
            feature_name = (char *)g_quark_to_string(feature_name_q);
            
            if(!(request_data->feature_set = zMapFeatureBlockGetSetByID(request_data->block, style_id)))
              {
                request_data->feature_set = zMapFeatureSetCreate(style_name , NULL);
                zMapFeatureBlockAddFeatureSet(request_data->block, request_data->feature_set);
              }
            
            if((request_data->feature = zMapFeatureCreateFromStandardData(feature_name, NULL, "", 
									  ZMAPSTYLE_MODE_BASIC, NULL,
									  start, end, has_score,
									  score, strand, ZMAPPHASE_NONE)))
              {
		request_data->feature->style_id = style_id;

		zMapFeatureSetAddFeature(request_data->feature_set, request_data->feature);

		if(start_not_found || end_not_found)
		  {
		    request_data->feature->type = ZMAPSTYLE_MODE_TRANSCRIPT;
		    zMapFeatureAddTranscriptStartEnd(request_data->feature, start_not_found,
						     start_phase, end_not_found);
		  }
		
		if((attr = zMapXMLElementGetAttributeByName(feature_element, "locus")))
		  {
		    ZMapFeatureSet locus_feature_set = NULL;
		    ZMapFeature old_feature;
		    ZMapFeature locus_feature = NULL;
		    GQuark new_locus_id  = zMapXMLAttributeGetValue(attr);
		    GQuark locus_set_id  = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME);
		    char *new_locus_name = (char *)g_quark_to_string(new_locus_id);
		    
		    /* Find locus feature set first ... */
		    if(!(locus_feature_set = zMapFeatureBlockGetSetByID(request_data->block, locus_set_id)))
		      {
			/* Can't find one, create one and add it. */
			locus_feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_LOCUS_NAME, NULL);
			zMapFeatureBlockAddFeatureSet(request_data->block, locus_feature_set);
		      }
		    
		    /* For some reason lace passes over odd xml here...
		       <zmap action="delete_feature">
		         <featureset>
		           <feature style="Known_CDS" name="RP23-383F20.1-004" locus="Oxsm" ... />
		         </featureset>
		       </zmap>
		       <zmap action="create_feature">
		         <featureset>
		           <feature style="Known_CDS" name="RP23-383F20.1-004" locus="Oxsm" ... />
		         ...
		       i.e. deleting the locus name it's creating!
		    */

		    old_feature = 
		      (ZMapFeature)zMapFeatureContextFindFeatureFromFeature(request_data->orig_context,
									    (ZMapFeatureAny)request_data->feature);
		    
		    if((old_feature) &&
		       (old_feature->type == ZMAPSTYLE_MODE_TRANSCRIPT) &&
		       (old_feature->locus_id != 0) &&
		       (old_feature->locus_id != new_locus_id))
		      {
			/* ^^^ check the old one was a transcript and had a locus that doesn't match this one */
			ZMapFeature tmp_locus_feature;
			ZMapFeatureAny old_locus_feature;
			char *old_locus_name = (char *)g_quark_to_string(old_feature->locus_id);
			
			/* If we're here, assumptions have been made
			 * 1 - We are in an action=delete_feature request
			 * 2 - We are modifying an existing feature.
			 * 3 - Lace has passed a locus="name" which does not = existing feature locus name.
			 * 4 - Locus start and end are based on feature start end.
			 *     If they are the extent of the locus as they should be... 
			 *     The unique_id will be different and therefore the next 
			 *     zMapFeatureContextFindFeatureFromFeature will fail.
			 *     This means the locus won't be deleted as it should be.
			 */
			zMapLogMessage("loci '%s' & '%s' don't match will delete '%s'",
				       old_locus_name, new_locus_name, old_locus_name);
			
			tmp_locus_feature = zMapFeatureCreateFromStandardData(old_locus_name,
									      NULL, "", ZMAPSTYLE_MODE_BASIC, NULL,
									      start, end, FALSE, 0.0, 
									      ZMAPSTRAND_NONE,
									      ZMAPPHASE_NONE);
			zMapFeatureSetAddFeature(locus_feature_set, tmp_locus_feature);
			
			if((old_locus_feature = zMapFeatureContextFindFeatureFromFeature(request_data->orig_context,
											 (ZMapFeatureAny)tmp_locus_feature)))
			  {
			    zMapLogMessage("Found old locus '%s', deleting.", old_locus_name);
			    locus_feature = (ZMapFeature)zMapFeatureAnyCopy(old_locus_feature);
			  }
			else
			  {
			    zMapLogMessage("Failed to find old locus '%s' during delete.", old_locus_name);
			    /* make the locus feature itself. */
			    locus_feature = zMapFeatureCreateFromStandardData(old_locus_name,
									      NULL, "", ZMAPSTYLE_MODE_BASIC, NULL,
									      start, end, FALSE, 0.0, 
									      ZMAPSTRAND_NONE,
									      ZMAPPHASE_NONE);
			  }
			
			zMapFeatureDestroy(tmp_locus_feature);
		      }
		    else
		      {
			/* make the locus feature itself. */
			locus_feature = zMapFeatureCreateFromStandardData(new_locus_name,
									  NULL, "", ZMAPSTYLE_MODE_BASIC, NULL,
									  start, end, FALSE, 0.0, 
									  ZMAPSTRAND_NONE,
									  ZMAPPHASE_NONE);
			
		      }			
		    
		    /* The feature set and feature need to have their styles set... */
		    
		    /* managed to get the styles set up. Add the feature to 
		     * feature set and finish up the locus. */
		    zMapFeatureSetAddFeature(locus_feature_set, 
					     locus_feature);
		    zMapFeatureAddLocus(locus_feature, new_locus_id);
		    
		    /* We'll still add the locus to the transcript
		     * feature so at least this information is
		     * preserved whatever went on with the styles */
		    zMapFeatureAddLocus(request_data->feature, new_locus_id);
		  }
		
	      }
	    

	    /* This must go and instead use a feature create call... */
            feature_any = g_new0(ZMapFeatureAnyStruct, 1) ;
	    feature_any->unique_id   = request_data->feature->unique_id;
	    feature_any->original_id = feature_name_q;
	    feature_any->struct_type = ZMAPFEATURE_STRUCT_FEATURE;
	    request_data->feature_list   = g_list_prepend(request_data->feature_list, feature_any);
          }
      }
      break;
    case ZMAPVIEW_REMOTE_INVALID:
      /* Not really a parsing error... */
      break;
    default:
      zMapXMLParserRaiseParsingError(parser, "Unexpected element for action");
      break;
    }

  return FALSE;
}

static gboolean xml_export_start_cb(gpointer user_data, ZMapXMLElement export_element, ZMapXMLParser parser)
{
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapXMLAttribute attr = NULL;
  /* <export filename="" format="" /> */

  if((attr = zMapXMLElementGetAttributeByName(export_element, "filename")))
    request_data->filename = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
  else
    zMapXMLParserRaiseParsingError(parser, "filename is a required attribute for export.");

  if((attr = zMapXMLElementGetAttributeByName(export_element, "format")))
    request_data->format = (char *)g_quark_to_string(zMapXMLAttributeGetValue(attr));
  else
    request_data->format = "gff";

  return FALSE;
}


static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement sub_element, 
                                      ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapFeature feature = NULL;

  if(xml_data->common.action == ZMAPVIEW_REMOTE_INVALID)
    return FALSE;

  zMapXMLParserCheckIfTrueErrorReturn(request_data->feature == NULL, 
                                      parser, 
                                      "a feature end tag without a created feature.");
  feature = request_data->feature;

  if((attr = zMapXMLElementGetAttributeByName(sub_element, "ontology")))
    {
      GQuark ontology = zMapXMLAttributeGetValue(attr);
      ZMapSpanStruct span = {0,0};
      ZMapSpan exon_ptr = NULL, intron_ptr = NULL;
      
      feature->type = ZMAPSTYLE_MODE_TRANSCRIPT;

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
        zMapFeatureAddTranscriptData(feature, TRUE, span.x1, span.x2, NULL, NULL);

      if(exon_ptr != NULL || intron_ptr != NULL) /* Do we need this check isn't it done internally? */
        zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr);

    }

  return TRUE;                  /* tell caller to clean us up. */
}

static gboolean xml_return_true_cb(gpointer user_data, 
                                   ZMapXMLElement zmap_element, 
                                   ZMapXMLParser parser)
{
  return TRUE;
}

