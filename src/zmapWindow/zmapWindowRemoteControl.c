/*  Last edited: Apr 11 10:51 2012 (edgrif) */
/*  File: zmapWindowRemoteControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2012: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Contains code to make and to respond to remotecontrol
 *              requests that are specific to individual windows.
 *
 * Exported functions: See ZMap/zmapWindow.h
 * 
 * Created: Mon Mar 26 13:23:04 2012 (edgrif)
 *-------------------------------------------------------------------
 */

#include <string.h>

#include <ZMap/zmap.h>

#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapRemoteProtocol.h>
#include <ZMap/zmapXML.h>
#include <zmapWindow_P.h>



/* This struct should most certainly be a union of mutually exclusive fields, it's good
 * documentation as well as good practice... */
typedef struct
{
  ZMapWindow window ;

  char *command_name ;
  RemoteCommandRCType command_rc ;

  ZMapFeatureContext orig_context ;
  ZMapFeatureAlignment orig_align ;
  ZMapFeatureBlock orig_block ;
  ZMapFeatureSet orig_feature_set ;
  ZMapFeature feature ;

  GQuark source_id ;

  GHashTable *styles ;
  ZMapFeatureTypeStyle style ;
  GQuark style_id ;

  GList *locations ;

  gboolean use_mark ;
  GList *feature_sets ;

  gboolean zoomed ;
} RequestDataStruct, *RequestData ;



static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_align_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_block_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_return_true_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);

static void zoomWindowToFeature(ZMapWindow window, RequestData input_data,
				RemoteCommandRCType *command_rc_out, char **reason_out,
				ZMapXMLUtilsEventStack *reply_out) ;
static void getWindowMark(ZMapWindow window, RemoteCommandRCType *command_rc_out, char **reason_out,
			  ZMapXMLUtilsEventStack *reply_out) ;

static ZMapXMLUtilsEventStack makeMessageElement(char *message_text) ;





static ZMapXMLObjTagFunctionsStruct window_starts_G[] = 
  {
    { "request",    xml_request_start_cb               },
    { "align",      xml_align_start_cb                 },
    { "block",      xml_block_start_cb                 },
    { "featureset", xml_featureset_start_cb            },
    { "feature",    xml_feature_start_cb               },
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct window_ends_G[] =
  {
    { "request",    xml_return_true_cb    },
    { "align",      xml_return_true_cb    },
    { "block",      xml_return_true_cb    },
    { "featureset", xml_return_true_cb    },
    { "feature",    xml_return_true_cb    },
    {NULL, NULL}
  } ;





static void localProcessRemoteRequest(ZMapWindow window,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void localProcessReplyFunc(char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
				  gpointer reply_handler_func_data) ;

static void processRequest(ZMapWindow window,
			   char *command_name, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;





/* WE NEED TO ADD THE CODE TO HERE FROM VIEW TO DO THESE COMMANDS, THIS IS THE LEVEL AT WHICH
   THEY SHOULD HAPPEN...*/


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	case ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE:
	case ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE:
	case ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE:
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* 
 *                External routines.
 */


/* Process the command if we recognise it, otherwise return FALSE. */
gboolean zMapWindowProcessRemoteRequest(ZMapWindow window,
					char *command_name, ZMapAppRemoteViewID view_id, char *request,
					ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;

  if (strcmp(command_name, ZACP_ZOOM_TO) == 0
      || strcmp(command_name, ZACP_GET_MARK) == 0)
    {
      localProcessRemoteRequest(window, command_name, view_id, request,
				app_reply_func, app_reply_data) ;

      result = TRUE ;
    }

  return result ;
}



/* 
 *                Internal routines.
 */


/* This is where we process a command. */
static void localProcessRemoteRequest(ZMapWindow window,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  /* We don't really need window here if we have view_id....sort this out... */
  processRequest(window, command_name, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}


/* This is where we process a reply received from a peer. */
static void localProcessReplyFunc(char *command,
				  RemoteCommandRCType command_rc,
				  char *reason,
				  char *reply,
				  gpointer reply_handler_func_data)
{
  ZMapWindow window = (ZMapWindow)reply_handler_func_data ;

  if (strcmp(command, ZACP_HANDSHAKE) == 0)
    {
      /* do something.... */
    }
  else if (strcmp(command, ZACP_GOODBYE) == 0)
    {
      /* etc etc...... */
    }

  return ;
}


/* Commands by this routine need the xml to be parsed for arguments etc. */
static void processRequest(ZMapWindow window,
			   char *command_name, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


  /* set up return code etc. for parsing code. */
  request_data.window = window ;
  request_data.command_name = command_name ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, window_starts_G, window_ends_G) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))
    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = g_strdup(zMapXMLParserLastErrorMsg(parser)) ;
    }
  else
    {
      if (strcmp(command_name, ZACP_ZOOM_TO) == 0)
	{
          zoomWindowToFeature(window, &request_data,
			      command_rc_out, reason_out, reply_out) ;
	}
      else if (strcmp(command_name, ZACP_GET_MARK) == 0)
	{
	  getWindowMark(window,	command_rc_out, reason_out, reply_out) ;
	}
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser) ;

  return ;
}


static void zoomWindowToFeature(ZMapWindow window, RequestData request_data,
				RemoteCommandRCType *command_rc_out, char **reason_out,
				ZMapXMLUtilsEventStack *reply_out)
{
  ZMapFeature feature = request_data->feature ;

  if (!(zMapWindowZoomToFeature(window, feature)))
    {
      *command_rc_out = REMOTE_COMMAND_RC_FAILED ;

      *reason_out = g_strdup_printf("Zoom feature %s failed",
				    (char *)g_quark_to_string(feature->original_id));
    }
  else
    {
      if (!zMapWindowFeatureSelect(window, feature))
	{
	  *command_rc_out = REMOTE_COMMAND_RC_FAILED ;

	  *reason_out = g_strdup_printf("Select feature %s failed",
					(char *)g_quark_to_string(feature->original_id)) ;
	}
      else
	{
	  char *message ;

	  message = g_strdup_printf("Zoom to feature %s ok",
				    (char *)g_quark_to_string(feature->original_id)) ; 
	  *reply_out = makeMessageElement(message) ;
	  *command_rc_out = REMOTE_COMMAND_RC_OK ;
	}
    }

  return ;
}


static void getWindowMark(ZMapWindow window, RemoteCommandRCType *command_rc_out, char **reason_out,
			  ZMapXMLUtilsEventStack *reply_out)
{
  int start = 0, end = 0 ;
  static ZMapXMLUtilsEventStackStruct mark_reply[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_MARK,  ZMAPXML_EVENT_DATA_NONE, {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_START, ZMAPXML_EVENT_DATA_INTEGER,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_END,   ZMAPXML_EVENT_DATA_INTEGER,  {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_MARK,  ZMAPXML_EVENT_DATA_NONE, {0}},
      {ZMAPXML_NULL_EVENT}
    } ;


  if (!zMapWindowGetMark(window, &start, &end))
    {
      *command_rc_out = REMOTE_COMMAND_RC_FAILED ;

      *reason_out = g_strdup_printf("No mark.") ;
    }
  else
    {
      mark_reply[1].value.i = start ;
      mark_reply[2].value.i = end ;
      *reply_out = &mark_reply[0] ;

      *command_rc_out = REMOTE_COMMAND_RC_OK ;
    }

  return ;
}




/* Quite dumb routine to save repeating this all over the place. The caller
 * is completely responsible for memory managing message-text, it must
 * be around long enough for the xml string to be created, after that it
 * can be free'd. */
static ZMapXMLUtilsEventStack makeMessageElement(char *message_text)
{
  static ZMapXMLUtilsEventStackStruct
    message[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_MESSAGE,   ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_CHAR_DATA_EVENT,     NULL,    ZMAPXML_EVENT_DATA_STRING, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_MESSAGE,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;

  message[1].value.s = message_text ;

  return &(message[0]) ;
}





/* 
 * The xml handler functions, called as each element is parsed.
 * 
 */



static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  RequestData request_data = (RequestData)user_data ;

  if (strcmp(request_data->command_name, ZACP_ZOOM_TO) == 0)
    {
      request_data->orig_context = request_data->window->feature_context ;

      /* We default to the master alignment and the first block within that alignment, the xml
       * does not have to contain either making it easier for the peer. */
      request_data->orig_align = request_data->orig_context->master_align ;
      request_data->orig_block = zMap_g_hash_table_nth(request_data->orig_context->master_align->blocks, 0) ;

      result = TRUE ;
    }

  return result ;
}


static gboolean xml_align_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data ;
  GQuark align_id ;
  char *align_name = NULL ;

  /* Look for an align name...doesn't have to be one. */
  if ((attr = zMapXMLElementGetAttributeByName(set_element, "name")))
    {
      if ((align_id = zMapXMLAttributeGetValue(attr)))
	align_name = (char *)g_quark_to_string(align_id) ;
    }

  if (align_name)
    {
      gboolean master_align ;

      request_data->orig_align = NULL ;

      if ((request_data->orig_align
	   = zMapFeatureContextGetAlignmentByID(request_data->orig_context,
						zMapFeatureAlignmentCreateID(align_name, TRUE))))
	master_align = TRUE ;
      else if ((request_data->orig_align
		= zMapFeatureContextGetAlignmentByID(request_data->orig_context,
						     zMapFeatureAlignmentCreateID(align_name, FALSE))))
	master_align = FALSE ;

      if ((request_data->orig_align))
	{
	  result = TRUE ;
	}
      else
	{
	  /* If we can't find the align it's a serious error and we can't carry on. */
	  char *err_msg ;

	  err_msg = g_strdup_printf("Unknown Align \"%s\":  not found in original_context", align_name) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}
    }
  else
    {
      /* default to master align (actually this should be set already. */
      request_data->orig_align = request_data->orig_context->master_align ;
      
      result = TRUE ;
    }



  return result ;
}



static gboolean xml_block_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data ;
  GQuark block_id ;
  char *block_name = NULL ;

  if ((attr = zMapXMLElementGetAttributeByName(set_element, "name")))
    {
      if ((block_id = zMapXMLAttributeGetValue(attr)))
	block_name = (char *)g_quark_to_string(block_id) ;
    }

  if (block_name)
    {
      int ref_start, ref_end, non_start, non_end;
      ZMapStrand ref_strand, non_strand;

      if (!zMapFeatureBlockDecodeID(block_id, &ref_start, &ref_end, &ref_strand,
				    &non_start, &non_end, &non_strand))
	{
	  /* Bad format block name. */
	  char *err_msg ;

	  err_msg = g_strdup_printf("Bad Format Block name: \"%s\"", block_name) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}
      else if (!(request_data->orig_block
		 = zMapFeatureAlignmentGetBlockByID(request_data->orig_align, block_id)))
	{
	  /* If we can't find the block it's a serious error and we can't carry on. */
	  char *err_msg ;

	  err_msg = g_strdup_printf("Unknown Block \"%s\":  not found in original_context", block_name) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}
      else
	{
	  result = TRUE ;
	}
    }
  else
    {
      /* Get the first one! */
      request_data->orig_block = zMap_g_hash_table_nth(request_data->orig_context->master_align->blocks, 0) ;

      result = TRUE ;
    }

  return result ;
}




static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement set_element,
                                        ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  RequestData request_data = (RequestData)user_data ;
  ZMapXMLAttribute attr = NULL ;
  GQuark featureset_id ;
  char *featureset_name ;
  GQuark unique_set_id ;
  char *unique_set_name ;

  if ((attr = zMapXMLElementGetAttributeByName(set_element, "name")))
    {
      /* Record original name and unique names for feature set, all look-ups are via the latter. */
      featureset_id = zMapXMLAttributeGetValue(attr) ;
      featureset_name = (char *)g_quark_to_string(featureset_id) ;

      request_data->source_id = unique_set_id = zMapFeatureSetCreateID(featureset_name) ;
      unique_set_name = (char *)g_quark_to_string(unique_set_id);

      /* Look for the feature set in the current context, it's an error if it's supposed to exist. */
      request_data->orig_feature_set = zMapFeatureBlockGetSetByID(request_data->orig_block, unique_set_id) ;
      if (!(request_data->orig_feature_set))
	{
	  /* If we can't find the featureset but it's meant to exist then it's a serious error
	   * and we can't carry on. */
	  char *err_msg ;

	  err_msg = g_strdup_printf("Unknown Featureset \"%s\":  not found in original_block", featureset_name) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}
      else
	{
	  result = TRUE ;
	}
    }
  else
    {
      /* If no featureset was specified then it's a serious error and we can't carry on. */
      zMapXMLParserRaiseParsingError(parser, "\"name\" is a required attribute for featureset.") ;

      request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
      result = FALSE ;
    }

  /* We must be able to look up the column and other data for this featureset otherwise we
   * won't know which column/style etc to use. */
  if (result)
    {
      ZMapFeatureSource source_data ;

      if (!(source_data = g_hash_table_lookup(request_data->window->context_map->source_2_sourcedata,
					      GINT_TO_POINTER(unique_set_id))))
	{
	  char *err_msg ;

	  err_msg = g_strdup_printf("Source %s not found in window->context_map.source_2_sourcedata",
				    g_quark_to_string(unique_set_id)) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}
      else
	{
	  request_data->style_id = source_data->style_id ;
	}

      if (result)
	{
	  // check style for all command except load_features
	  // as load features processes a complete step list that inludes requesting the style
	  // then if the style is not there then we'll drop the features

	  if (!(request_data->style = zMapFindStyle(request_data->window->context_map->styles,
						    request_data->style_id)))
	    {
	      char *err_msg ;

	      err_msg = g_strdup_printf("Style %s not found in window->context_map.styles",
					g_quark_to_string(request_data->style_id)) ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	      result = FALSE ;
	    }
	}
    }

  return result ;
}



static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data ;
  ZMapStrand strand = ZMAPSTRAND_NONE;
  char *feature_name ;
  GQuark feature_name_id, feature_unique_id ;
  int start = 0, end = 0 ;
  ZMapFeature feature ;
  ZMapStyleMode mode ;

  result = TRUE ;

  zMap_g_hash_table_print(request_data->orig_feature_set->features, "gquark") ;
  fflush(stdout) ;

  /* Must have following attributes for all feature-based operations. */
  if (strcmp(request_data->command_name, ZACP_ZOOM_TO) == 0)
    {
      if (result && (attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
	{
	  feature_name_id = zMapXMLAttributeGetValue(attr) ;
	  feature_name = (char *)g_quark_to_string(feature_name_id) ;
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "\"name\" is a required attribute for feature.") ;

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}


      if (result && (attr = zMapXMLElementGetAttributeByName(feature_element, "start")))
	{
	  start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
			 (char **)NULL, 10);
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "\"start\" is a required attribute for feature.");

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}

      if (result && (attr = zMapXMLElementGetAttributeByName(feature_element, "end")))
	{
	  end = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
		       (char **)NULL, 10);
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "\"end\" is a required attribute for feature.");

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}

      if (result && (attr = zMapXMLElementGetAttributeByName(feature_element, "strand")))
	{
	  zMapFeatureFormatStrand((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
				  &(strand));
	}
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "\"strand\" is a required attribute for feature.");

	  request_data->command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  result = FALSE ;
	}


      /* Check if feature exists, for some commands it must do, for others it must not. */
      if (result)
	{
	  /* Need the style to get hold of the feature mode, better would be for
	   * xml to contain SO term ?? Maybe not....not sure. */
	  mode = zMapStyleGetMode(request_data->style) ;

	  feature_unique_id = zMapFeatureCreateID(mode, feature_name, strand, start, end, 0, 0) ;


	  feature = zMapFeatureSetGetFeatureByID(request_data->orig_feature_set, feature_unique_id) ;

	  if (!feature)
	    {
	      /* If we _don't_ find the feature then it's a serious error for these commands. */
	      char *err_msg ;

	      err_msg = g_strdup_printf("Feature \"%s\" with id \"%s\" could not be found in featureset \"%s\",",
					feature_name, g_quark_to_string(feature_unique_id),
					zMapFeatureName((ZMapFeatureAny)(request_data->orig_feature_set))) ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;
	      result = FALSE ;
	    }
	}

      /* Now do something... */
      if (result)
	{
	  ZMapFeature feature ;
	  ZMapStyleMode mode ;

	  /* Need the style to get hold of the feature mode, better would be for
	   * xml to contain SO term ?? Maybe not....not sure. */
	  mode = zMapStyleGetMode(request_data->style) ;

	  /* should be removed.... */
	  feature_unique_id = zMapFeatureCreateID(mode,
						  feature_name,
						  strand,
						  start, end, 0, 0) ;
      
	  /* We don't need a list here...simplify this too.... */
	  if ((feature = zMapFeatureSetGetFeatureByID(request_data->orig_feature_set, feature_unique_id)))
	    {
	      request_data->feature = feature ;
	    }
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "Cannot find feature in zmap.") ;

	      request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;
	      result = FALSE ;
	    }
	}
    }


  return result ;
}



static gboolean xml_return_true_cb(gpointer user_data,
                                   ZMapXMLElement zmap_element,
                                   ZMapXMLParser parser)
{
  return TRUE;
}








