/*  File: zmapWindowRemoteReceive.c
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
 * Description:
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */


/* THIS FILE IS BECOMING REDUNDANT....AND BEING REPLACED BY zmapWindowRemoteControl.c */



#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <ZMap/zmapGLibUtils.h> /* zMap_g_hash_table_nth */
#include <zmapWindow_P.h>


/* should be a type.... */
enum
  {
    ZMAPWINDOW_REMOTE_INVALID,
    /* Add below here... */

    ZMAPWINDOW_REMOTE_REGISTER_CLIENT,


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* MOVING TO VIEW.... */

    ZMAPWINDOW_REMOTE_ZOOM_TO,
    ZMAPWINDOW_REMOTE_GET_MARK,
    ZMAPWINDOW_REMOTE_LOAD_FEATURES,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    /* ...but above here */
    ZMAPWINDOW_REMOTE_UNKNOWN
  } ZMapViewValidXRemoteActions ;


static char *actions_G[ZMAPWINDOW_REMOTE_UNKNOWN + 1] =
  {
    NULL,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* moving to view */

    "zoom_to", "get_mark", "load_features",
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    "register_client",
    NULL
  };



char *zMapWindowRemoteReceiveAccepts(ZMapWindow window)
{
  char *xml = NULL;

  xml = zMapXRemoteClientAcceptsActionsXML(zMapXRemoteWidgetGetXID(window->toplevel),
                                           &actions_G[ZMAPWINDOW_REMOTE_INVALID + 1],
                                           ZMAPWINDOW_REMOTE_UNKNOWN - 1);

  return xml;
}






#if MH17_UNUSED_CODE_FOR_WINDOW_XML

/*
      all the window code was moved to the view long ago
*/


/* ----------------- old code -------------------------*/


/* This struct should most certainly be a union of mutually exclusive fields, it's good
 * documentation as well as good practice... */
typedef struct
{
  ZMapWindow window;

  ZMapFeatureContext orig_context;
  ZMapFeatureContext edit_context;

  int action ;

  ZMapFeatureAlignment align;
  ZMapFeatureBlock     block;
  ZMapFeatureSet feature_set;
  ZMapFeature        feature;

  GHashTable *styles;

  GList *locations, *feature_list;

  gboolean use_mark ;
  GList *feature_sets ;

  gboolean zoomed ;
}RequestDataStruct, *RequestData;

typedef struct
{
  int  code;
  gboolean handled;
  GString *messages;
} ResponseDataStruct, *ResponseData;



static char *window_execute_command(char *command_text, gpointer user_data, int *statusCode) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void zoomWindowToFeature(ZMapWindow window, RequestData input_data, ResponseData output_data) ;
static void reportWindowMark(ZMapWindow window, RequestData input_data, ResponseData output_data) ;
static void loadFeatures(ZMapWindow window, RequestData input_data, ResponseData output_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static char *actions_G[ZMAPWINDOW_REMOTE_UNKNOWN + 1] =
  {
    NULL,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* moving to view */

    "zoom_to", "get_mark", "load_features",
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    "register_client",
    NULL
  };

static gboolean xml_zmap_start_cb(gpointer user_data,
                                  ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser);
static gboolean xml_request_start_cb(gpointer user_data,
				     ZMapXMLElement zmap_element,
				     ZMapXMLParser parser);
#if NOT_USED
static gboolean xml_featureset_start_cb(gpointer user_data,
                                        ZMapXMLElement zmap_element,
                                        ZMapXMLParser parser);
static gboolean xml_feature_start_cb(gpointer user_data,
                                     ZMapXMLElement zmap_element,
                                     ZMapXMLParser parser);
#endif
static gboolean xml_return_true_cb(gpointer user_data,
                                   ZMapXMLElement zmap_element,
                                   ZMapXMLParser parser);


static ZMapXMLObjTagFunctionsStruct window_starts_G[] =
  {
    { "zmap",       xml_zmap_start_cb                  },
    { "request",    xml_request_start_cb               },
    { "client",     zMapXRemoteXMLGenericClientStartCB },

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    { "featureset", xml_featureset_start_cb            },
    { "feature",    xml_feature_start_cb               },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct window_ends_G[] =
  {
    { "zmap",       xml_return_true_cb    },
    { "request",    xml_return_true_cb    },

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    { "feature",    xml_return_true_cb    },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    {NULL, NULL}
  };



void zMapWindowSetupXRemote(ZMapWindow window, GtkWidget *widget)
{
  zMapXRemoteInitialiseWidget(widget, PACKAGE_NAME,
                              ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                              ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                              window_execute_command, window);
  return ;
}


static char *window_execute_command(char *command_text, gpointer user_data, int *statusCode)
{
  ZMapXMLParser parser;
  ZMapWindow window = (ZMapWindow)user_data;
  ZMapXRemoteParseCommandDataStruct input = { NULL };
  RequestDataStruct input_data = {0};
  char *response = NULL;

  input_data.window = window;
  input.user_data   = &input_data;

  parser = zMapXMLParserCreate(&input, FALSE, FALSE);

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &window_starts_G[0], &window_ends_G[0]);

  if((zMapXMLParserParseBuffer(parser, command_text, strlen(command_text))) == TRUE)
    {
      ResponseDataStruct output_data = {0};

      output_data.code     = 0;
      output_data.messages = g_string_sized_new(512);

      switch(input.common.action)
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
        case ZMAPWINDOW_REMOTE_ZOOM_TO:
          zoomWindowToFeature(window, &input_data, &output_data);
          break;
	case ZMAPWINDOW_REMOTE_GET_MARK:
	  reportWindowMark(window, &input_data, &output_data);
	  break;
        case ZMAPWINDOW_REMOTE_LOAD_FEATURES:
	  loadFeatures(window, &input_data, &output_data) ;
          break ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        case ZMAPWINDOW_REMOTE_INVALID:
        case ZMAPWINDOW_REMOTE_UNKNOWN:
        default:
          g_string_append(output_data.messages, "Unknown command");
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

  if(!zMapXRemoteValidateStatusCode(statusCode) && response != NULL)
    {
      zMapLogWarning("%s", response);
      g_free(response);
      response = g_strdup("Broken code. Check zmap.log file");
    }
  if(response == NULL){ response = g_strdup("Broken code."); }

  return response;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static ZMapFeatureContextExecuteStatus zoomToFeatureCB(GQuark key,
                                                       gpointer data,
                                                       gpointer user_data,
                                                       char **error_out)
{
  ZMapFeatureContextExecuteStatus status = ZMAP_CONTEXT_EXEC_STATUS_OK;
  ZMapFeatureAny feature_any = (ZMapFeatureAny)data;
  RequestData input_data = (RequestData)user_data;

  switch(feature_any->struct_type)
    {
    case ZMAPFEATURE_STRUCT_FEATURE:
      if(!input_data->zoomed)
        {
          zMapWindowZoomToFeature(input_data->window, (ZMapFeature)feature_any);
          input_data->zoomed = TRUE;
        }
      break;
    default:
      break;
    }

  return status;
}


#if ED_G_NEVER_INCLUDE_THIS_CODE
static void zoomWindowToFeature(ZMapWindow window, RequestData input_data, ResponseData output_data)
{
  GList *list;
  ZMapSpan span;

  output_data->code = ZMAPXREMOTE_OK;

  if ((list = g_list_first(input_data->feature_list)))
    {
      ZMapFeature feature = (ZMapFeature)(list->data);
      input_data->zoomed  = FALSE;

      if (window->revcomped_features)
	zMapFeatureContextReverseComplement(input_data->edit_context, window->context_map->styles) ;

      zMapFeatureContextExecute((ZMapFeatureAny)(input_data->edit_context), ZMAPFEATURE_STRUCT_FEATURE,
                                zoomToFeatureCB, input_data) ;
      if (input_data->zoomed)
        {
          g_string_append_printf(output_data->messages,
                                 "Zoom to feature %s executed",
                                 (char *)g_quark_to_string(feature->original_id));
        }
      else
        {
          output_data->code = ZMAPXREMOTE_CONFLICT;
          g_string_append_printf(output_data->messages,
                                 "Zoom to feature %s failed",
                                 (char *)g_quark_to_string(feature->original_id));
        }
    }
  else if ((list = g_list_first(input_data->locations)))
    {
      span = (ZMapSpan)(list->data);
      zMapWindowZoomToWorldPosition(window, FALSE,
                                    0.0, span->x1,
                                    100.0, span->x2);
      g_string_append_printf(output_data->messages,
                             "Zoom to location %d-%d executed",
                             span->x1, span->x2);
    }
  else
    {
      g_string_append_printf(output_data->messages,
                             "No data for %s action",
                             actions_G[ZMAPWINDOW_REMOTE_ZOOM_TO]);
      output_data->code = ZMAPXREMOTE_BADREQUEST;
    }

  return ;
}
#endif

static void reportWindowMark(ZMapWindow window, RequestData input_data, ResponseData output_data)
{
  ZMapWindowMark mark;

  output_data->code = ZMAPXREMOTE_OK;

  if ((mark = window->mark) && zmapWindowMarkIsSet(mark))
    {
      char *sequence_name;
      double wx1, wx2, wy1, wy2;
      int start, end;

      zmapWindowMarkGetWorldRange(mark, &wx1, &wy1, &wx2, &wy2);

      /* we're only interested in the y coords here... */
      start = (int)(wy1);
      end   = (int)(wy2);

      sequence_name = window->sequence;	/* hmm, this doesn't feel right. */

      g_string_append_printf(output_data->messages,
			     "<segment sequence=\"%s\" start=\"%d\" end=\"%d\" />",
			     sequence_name, start, end);
    }
  else
    g_string_append_printf(output_data->messages,
			   "<segment sequence=\"%s\" />",
			   window->sequence);


  return ;
}


/* Load features on request from a client. */
static void loadFeatures(ZMapWindow window, RequestData input_data, ResponseData output_data)
{
  gboolean use_mark = FALSE ;

  /* If mark then get mark, otherwise get big start/end. */
  if (input_data->use_mark)
    {
      if ((window->mark) && zmapWindowMarkIsSet(window->mark))
	{
	  use_mark = TRUE ;
	}
      else
	{
	  g_string_append(output_data->messages, "Load features to marked region failed: no mark set.") ;
	  output_data->code = ZMAPXREMOTE_BADREQUEST;
	}
    }

  zmapWindowFetchData(window, input_data->block, input_data->feature_sets, use_mark) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void populate_request_data(RequestData input_data)
{
  input_data->orig_context = input_data->window->feature_context;

  /* Copy basics of original context. */
  input_data->edit_context = (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)(input_data->orig_context)) ;
  //input_data->edit_context->styles = NULL ;

  input_data->styles = input_data->window->context_map->styles ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean setupStyles(ZMapFeatureSet set, ZMapFeature feature,
                            GHashTable *styles, GQuark style_id)
{
  gboolean got_style = TRUE;

  return got_style;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static gboolean xml_zmap_start_cb(gpointer user_data,
                                  ZMapXMLElement zmap_element,
                                  ZMapXMLParser parser)
{
  gboolean result = TRUE ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  ZMapXMLAttribute attr = NULL;
  GQuark action = 0;

  if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      int i;
      action = zMapXMLAttributeGetValue(attr);

      xml_data->common.action = ZMAPWINDOW_REMOTE_INVALID;

      for (i = ZMAPWINDOW_REMOTE_INVALID + 1; i < ZMAPWINDOW_REMOTE_UNKNOWN; i++)
        {
          if (action == g_quark_from_string(actions_G[i]))
            xml_data->common.action = i ;
        }

      /* unless((action > INVALID) and (action < UNKNOWN)) */
      if (!(xml_data->common.action > ZMAPWINDOW_REMOTE_INVALID
	    && xml_data->common.action < ZMAPWINDOW_REMOTE_UNKNOWN))
        {
          zMapLogWarning("action='%s' is unknown", g_quark_to_string(action));
          xml_data->common.action = ZMAPWINDOW_REMOTE_UNKNOWN;

	  result = FALSE ;
        }
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");
      xml_data->common.action = ZMAPWINDOW_REMOTE_INVALID;

      result = FALSE ;
    }

  if (result)
    {
      RequestData request_data = (RequestData)(xml_data->user_data);

      switch (xml_data->common.action)
	{
	case ZMAPWINDOW_REMOTE_ZOOM_TO:
	  {
	    populate_request_data(request_data);

	    break;
	  }
	case ZMAPWINDOW_REMOTE_LOAD_FEATURES:
	  {
	    if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "load")))
	      {
		GQuark load_id ;

		load_id = zMapXMLAttributeGetValue(attr) ;

		if (zMapLogQuarkIsStr(load_id, "mark"))
		  request_data->use_mark = TRUE ;
		else if (zMapLogQuarkIsStr(load_id, "full"))
		  request_data->use_mark = FALSE ;
		else
		  {
		    zMapLogWarning("Value \"%s\" for \"load\" attr is unknown.", g_quark_to_string(load_id));
		    xml_data->common.action = ZMAPWINDOW_REMOTE_UNKNOWN;

		    result = FALSE ;
		  }
	      }

	    break;
	  }

	case ZMAPWINDOW_REMOTE_REGISTER_CLIENT:

	  printf("got register_client\n") ;

	case ZMAPWINDOW_REMOTE_GET_MARK:
	default:
	  break;
	}

      if (result)
	request_data->action = xml_data->common.action ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}

static gboolean xml_request_start_cb(gpointer user_data,
				     ZMapXMLElement zmap_element,
				     ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  ZMapXMLAttribute attr = NULL;
  GQuark action = 0;

  if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "action")) != NULL)
    {
      int i;
      action = zMapXMLAttributeGetValue(attr);

      xml_data->common.action = ZMAPWINDOW_REMOTE_INVALID;

      for (i = ZMAPWINDOW_REMOTE_INVALID + 1; i < ZMAPWINDOW_REMOTE_UNKNOWN; i++)
        {
          if (action == g_quark_from_string(actions_G[i]))
            xml_data->common.action = i ;
        }

      /* unless((action > INVALID) and (action < UNKNOWN)) */
      if (!(xml_data->common.action > ZMAPWINDOW_REMOTE_INVALID
	    && xml_data->common.action < ZMAPWINDOW_REMOTE_UNKNOWN))
        {
          zMapLogWarning("action='%s' is unknown", g_quark_to_string(action));
          xml_data->common.action = ZMAPWINDOW_REMOTE_UNKNOWN;

	  result = FALSE ;
        }
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "action is a required attribute for zmap.");
      xml_data->common.action = ZMAPWINDOW_REMOTE_INVALID;

      result = FALSE ;
    }

  if (result)
    {
      RequestData request_data = (RequestData)(xml_data->user_data);

      switch (xml_data->common.action)
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	case ZMAPWINDOW_REMOTE_ZOOM_TO:
	  {
	    populate_request_data(request_data);

	    break;
	  }
	case ZMAPWINDOW_REMOTE_LOAD_FEATURES:
	  {
	    if ((attr = zMapXMLElementGetAttributeByName(zmap_element, "load")))
	      {
		GQuark load_id ;

		load_id = zMapXMLAttributeGetValue(attr) ;

		if (zMapLogQuarkIsStr(load_id, "mark"))
		  request_data->use_mark = TRUE ;
		else if (zMapLogQuarkIsStr(load_id, "full"))
		  request_data->use_mark = FALSE ;
		else
		  {
		    zMapLogWarning("Value \"%s\" for \"load\" attr is unknown.", g_quark_to_string(load_id));
		    xml_data->common.action = ZMAPWINDOW_REMOTE_UNKNOWN;

		    result = FALSE ;
		  }
	      }

	    break;
	  }

	case ZMAPWINDOW_REMOTE_GET_MARK:
	  break ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	case ZMAPWINDOW_REMOTE_REGISTER_CLIENT:
	  printf("got register_client\n") ;
	  break ;

	default:
	  break;
	}

      if (result)
	request_data->action = xml_data->common.action ;
    }



  return result ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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

  /* Isn't this fun... */
  if ((attr = zMapXMLElementGetAttributeByName(set_element, "align")))
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



  if ((attr = zMapXMLElementGetAttributeByName(set_element, "block")))
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


  if ((attr = zMapXMLElementGetAttributeByName(set_element, "set")))
    {
      set_id   = zMapXMLAttributeGetValue(attr);
      set_name = (char *)g_quark_to_string(set_id);
      set_id   = zMapFeatureSetCreateID(set_name);

      if (!(feature_set = zMapFeatureBlockGetSetByID(request_data->block, set_id)))
        {
          feature_set = zMapFeatureSetCreate(set_name, NULL);
          zMapFeatureBlockAddFeatureSet(request_data->block, feature_set);
        }
      request_data->feature_set = feature_set;


      request_data->edit_context->feature_set_names = g_list_append(request_data->edit_context->feature_set_names,
								    GINT_TO_POINTER(set_id)) ;

      switch (request_data->action)
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	case ZMAPWINDOW_REMOTE_LOAD_FEATURES:
	  {
	    request_data->feature_sets = g_list_append(request_data->feature_sets, GINT_TO_POINTER(set_id)) ;

	    break;
	  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	default:
	  break ;
	}

    }
  else
    {
      /* the feature handler will rely on the style to find the feature set... */
      request_data->feature_set = NULL;
    }


  return FALSE;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element,
                                     ZMapXMLParser parser)
{
  ZMapXMLAttribute attr = NULL;
  ZMapXRemoteParseCommandData xml_data = (ZMapXRemoteParseCommandData)user_data;
  RequestData request_data = (RequestData)(xml_data->user_data);
  ZMapFeatureAny feature_any;
  ZMapStrand strand = ZMAPSTRAND_NONE;
  GQuark feature_name_q = 0, style_q = 0, style_id;
  gboolean has_score = FALSE;
  char *feature_name, *style_name;
  int start = 0, end = 0;
  double score = 0.0;

  switch(xml_data->common.action)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    case ZMAPWINDOW_REMOTE_ZOOM_TO:
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
                if (setupStyles(request_data->feature_set, request_data->feature,
				request_data->styles, style_id))
		  {

#ifdef RDS_DONT_INCLUDE
		    ZMapFeatureTypeStyle orig_set_style = request_data->feature_set->style ;

		    request_data->feature_set->style = zMapFeatureStyleCopy(request_data->feature_set->style) ;
		    zMapStyleSetAdd(&(request_data->edit_context->styles), request_data->feature_set->style) ;
		    request_data->edit_context->feature_set_names
		      = g_list_append(request_data->edit_context->feature_set_names,
				      GINT_TO_POINTER(zMapStyleGetUniqueID(request_data->feature_set->style))) ;

		    if (orig_set_style == request_data->feature->style)
		      {
			request_data->feature->style = request_data->feature_set->style ;
		      }
		    else
		      {
			request_data->feature->style = zMapFeatureStyleCopy(request_data->feature->style) ;
			zMapStyleSetAdd(&(request_data->edit_context->styles), request_data->feature->style) ;
			request_data->edit_context->feature_set_names
			  = g_list_append(request_data->edit_context->feature_set_names,
					  GINT_TO_POINTER(zMapStyleGetUniqueID(request_data->feature->style))) ;
		      }
#endif
		    request_data->feature->style_id = style_id;
		    zMapFeatureSetAddFeature(request_data->feature_set, request_data->feature);
		  }
                else
                  {
                    char *error;
                    error = g_strdup_printf("Valid style is required. Nothing known of '%s'.", style_name);
                    zMapFeatureDestroy(request_data->feature);
                    zMapXMLParserRaiseParsingError(parser, error);
                    g_free(error);
                  }
              }


	    /* This must go and instead use a feature create call... */
            feature_any = g_new0(ZMapFeatureAnyStruct, 1) ;
	    feature_any->unique_id   = request_data->feature->unique_id;
	    feature_any->original_id = feature_name_q;
	    feature_any->struct_type = ZMAPFEATURE_STRUCT_FEATURE;
	    request_data->feature_list = g_list_prepend(request_data->feature_list, feature_any);
          }
      }
      break;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




    default:
      zMapXMLParserRaiseParsingError(parser, "Unexpected element for action");
      break;
    }

  return FALSE;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static gboolean xml_return_true_cb(gpointer user_data,
                                   ZMapXMLElement zmap_element,
                                   ZMapXMLParser parser)
{
  return TRUE;
}

#endif
