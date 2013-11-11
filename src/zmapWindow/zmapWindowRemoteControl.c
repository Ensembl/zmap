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

#include <ZMap/zmap.h>

#include <string.h>

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


static ZMapXMLObjTagFunctionsStruct mark_starts_G[] = 
  {
    { "request",    xml_request_start_cb},
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct mark_ends_G[] =
  {
    { "request",    xml_return_true_cb},
    {NULL, NULL}
  } ;





static void localProcessRemoteRequest(ZMapWindow window,
				      char *command_name, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
#ifdef UNUSED
static void localProcessReplyFunc(char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
				  gpointer reply_handler_func_data) ;
#endif 

static void processRequest(ZMapWindow window,
			   char *command_name, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;



/* 
 *                Globals. 
 */




/* 
 *                External routines.
 */


/* Entry point to handle requests from a peer program, process the command if we recognise it,
 * otherwise return FALSE to indicate we don't know that command. */
gboolean zMapWindowProcessRemoteRequest(ZMapWindow window,
					char *command_name, char *request,
					ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;

  if (strcmp(command_name, ZACP_ZOOM_TO) == 0
      || strcmp(command_name, ZACP_GET_MARK) == 0)
    {
      localProcessRemoteRequest(window, command_name, request,
				app_reply_func, app_reply_data) ;

      result = TRUE ;
    }

  return result ;
}





/* 
 *                            Package routines
 */



/* MOVED FROM ZMAPWINDOW.C......BEING FIXED THIS UP TO WORK WITH NEW REMOTE CONTROL.....
 * NEED TO BROUGHT IN LINE WITH OTHER REMOTE CONTROL HANDLING SECTIONS OF CODE... */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapWindowUpdateXRemoteData(ZMapWindow window, ZMapFeatureAny feature_any,
				 char *action, FooCanvasItem *real_item)

{
  zmapWindowUpdateXRemoteDataFull(window, feature_any,
				  action, real_item, NULL, NULL, NULL) ;

  return ;
}


/* It is probably just about worth having this here as a unified place to handle requests but
 * that may need revisiting.... */
void zmapWindowUpdateXRemoteDataFull(ZMapWindow window, ZMapFeatureAny feature_any,
				     char *action, FooCanvasItem *real_item,
				     ZMapXMLObjTagFunctions start_handlers,
				     ZMapXMLObjTagFunctions end_handlers,
				     gpointer handler_data)
{
  ZMapWindowCallbacks window_cbs_G = zmapWindowGetCBs() ;
  ZMapXMLUtilsEventStack xml_elements ;
  ZMapWindowSelectStruct select = {0} ;
  ZMapFeatureSetStruct feature_set = {0} ;
  ZMapFeatureSet multi_set ;
  ZMapFeature feature_copy ;
  int chr_bp ;

  /* We should only ever be called with a feature, not a set or anything else. */
  zMapAssert(feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE) ;


  /* OK...IN HERE IS THE PLACE FOR THE HACK FOR COORDS....NEED TO COPY FEATURE
   * AND INSERT NEW CHROMOSOME COORDS...IF WE CAN DO THIS FOR THIS THEN WE
   * CAN HANDLE VIEW FEATURE STUFF IN SAME WAY...... */
  feature_copy = (ZMapFeature)zMapFeatureAnyCopy(feature_any) ;
  feature_copy->parent = feature_any->parent ;	    /* Copy does not do parents so we fill in. */


  /* REVCOMP COORD HACK......THIS HACK IS BECAUSE OUR COORD SYSTEM IS MUCKED UP FOR
   * REVCOMP'D FEATURES..... */
  /* Convert coords */
  if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
    {
      /* remap coords to forward strand range and also swop
       * them as they get reversed in revcomping.... */
      chr_bp = feature_copy->x1 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x1 = chr_bp ;


      chr_bp = feature_copy->x2 ;
      chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
      feature_copy->x2 = chr_bp ;

      zMapUtilsSwop(int, feature_copy->x1, feature_copy->x2) ;

      if (feature_copy->strand == ZMAPSTRAND_FORWARD)
	feature_copy->strand = ZMAPSTRAND_REVERSE ;
      else
	feature_copy->strand = ZMAPSTRAND_FORWARD ;
	      

      if (ZMAPFEATURE_IS_TRANSCRIPT(feature_copy))
	{
	  if (!zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_EXON,
						 revcompTransChildCoordsCB, window)
	      || !zMapFeatureTranscriptChildForeach(feature_copy, ZMAPFEATURE_SUBPART_INTRON,
						    revcompTransChildCoordsCB, window))
	    zMapLogCritical("RemoteControl error revcomping coords for transcript %s",
			    zMapFeatureName((ZMapFeatureAny)(feature_copy))) ;

	  zMapFeatureTranscriptSortExons(feature_copy) ;
	}
    }

  /* Streuth...why doesn't this use a 'creator' function...... */
#ifdef FEATURES_NEED_MAGIC
  feature_set.magic       = feature_copy->magic ;
#endif
  feature_set.struct_type = ZMAPFEATURE_STRUCT_FEATURESET;
  feature_set.parent      = feature_copy->parent->parent;
  feature_set.unique_id   = feature_copy->parent->unique_id;
  feature_set.original_id = feature_copy->parent->original_id;

  feature_set.features = g_hash_table_new(NULL, NULL) ;
  g_hash_table_insert(feature_set.features, GINT_TO_POINTER(feature_copy->unique_id), feature_copy) ;

  multi_set = &feature_set ;


  /* I don't get this at all... */
  select.type = ZMAPWINDOW_SELECT_DOUBLE;

  /* Set up xml/xremote request. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  select.xml_handler.zmap_action = g_strdup(action);

  select.xml_handler.xml_events = zMapFeatureAnyAsXMLEvents((ZMapFeatureAny)(multi_set), ZMAPFEATURE_XML_XREMOTE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* WILL ALL NEED TO BE SET UP FOR FEATURESHOW CODE..... */

  select.xml_handler.start_handlers = start_handlers ;
  select.xml_handler.end_handlers = end_handlers ;
  select.xml_handler.handler_data = handler_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  xml_elements = zMapFeatureAnyAsXMLEvents(feature_copy) ;

  /* free feature_copy, remove reference to parent otherwise we are removed from there. */
  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      feature_copy->parent = NULL ;
      zMapFeatureDestroy(feature_copy) ;
    }


  if (feature_set.unique_id)
    {
      g_hash_table_destroy(feature_set.features) ;
      feature_set.features = NULL ;
    }


  /* HERE VIEW IS CALLED WHICH IS FINE.....BUT.....WE NOW NEED ANOTHER CALL WHERE
     WINDOW CALLS REMOTE TO MAKE THE REQUEST.....BECAUSE VIEW WILL NO LONGER BE
     DOING THIS...*/
  (*(window->caller_cbs->select))(window, window->app_data, &select) ;



  /* Send request to peer program. */
  (*(window_cbs_G->remote_request_func))(window_cbs_G->remote_request_func_data,
					 window,
					 action, xml_elements,
					 localProcessReplyFunc, window) ;




  /* ALL THIS NEEDS TO GO TO..... */
/* this result bit needs changing too.......... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  result = select.xml_handler.handled ;

  result = select.remote_result ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ALL NEEDS FIXING UP.... */

  /* Free xml/xremote stuff. */
  if (select.xml_handler.zmap_action)
    g_free(select.xml_handler.zmap_action);
  if (select.xml_handler.xml_events)
    g_array_free(select.xml_handler.xml_events, TRUE);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/* 
 *                Internal routines.
 */


/* Process requests from remote programs and calls app_reply_func to return the result. */
static void localProcessRemoteRequest(ZMapWindow window,
				      char *command_name, char *request,
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



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Handles replies from remote program to commands sent from this layer. */
static void localProcessReplyFunc(char *command,
				  RemoteCommandRCType command_rc,
				  char *reason,
				  char *reply,
				  gpointer reply_handler_func_data)
{
  ZMapWindow window = (ZMapWindow)reply_handler_func_data ;

  /* Currently we don't have a reply handler for all things, we should but
   * we don't .....this will change... */
  if (window->xremote_reply_handler)
    (window->xremote_reply_handler)(window, window->xremote_reply_data,
				    command, command_rc, reason, reply) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





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
  ZMapXMLObjTagFunctions starts, ends ;



  /* set up return code etc. for parsing code. */
  request_data.window = window ;
  request_data.command_name = command_name ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  /* Set start/end handlers, some commands are much simpler than others. */
  if (strcmp(command_name, ZACP_ZOOM_TO) == 0)
    {
      starts = window_starts_G ;
      ends = window_ends_G ;
    }
  else if (strcmp(command_name, ZACP_GET_MARK) == 0)
    {
      starts = mark_starts_G ;
      ends = mark_ends_G ;
    }
  zMapXMLParserSetMarkupObjectTagHandlers(parser, starts, ends) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))
    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = zMapXMLUtilsEscapeStr(zMapXMLParserLastErrorMsg(parser)) ;
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

      *reason_out = zMapXMLUtilsEscapeStrPrintf("Zoom feature %s failed",
						(char *)g_quark_to_string(feature->original_id));
    }
  else
    {
      if (!zMapWindowFeatureSelect(window, feature))
	{
	  *command_rc_out = REMOTE_COMMAND_RC_FAILED ;

	  *reason_out = zMapXMLUtilsEscapeStrPrintf("Select feature %s failed",
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

      *reason_out = zMapXMLUtilsEscapeStr("No mark.") ;
    }
  else
    {
      /* Need to check for revcomp..... */
      if (window->flags[ZMAPFLAG_REVCOMPED_FEATURES])
	{
	  ZMapFeatureAlignment align ;
	  ZMapFeatureBlock block ;
	  int seq_start, seq_end ;


	  /* default to master align. */
	  align = window->feature_context->master_align ;
	  block = zMap_g_hash_table_nth(align->blocks, 0) ;


	  seq_start = start ;
	  seq_end = end ;

	  zMapFeatureReverseComplementCoords(block, &seq_start, &seq_end) ;

	  /* NOTE, always return start < end so swop coords */
	  start = seq_end ;
	  end = seq_start ;
	}

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
      if (request_data->window->feature_context)
	{
	  request_data->orig_context = request_data->window->feature_context ;

	  /* We default to the master alignment and the first block within that alignment, the xml
	   * does not have to contain either making it easier for the peer. */
	  request_data->orig_align = request_data->orig_context->master_align ;
	  request_data->orig_block = zMap_g_hash_table_nth(request_data->orig_context->master_align->blocks, 0) ;

	  result = TRUE ;
	}
      else
	{
	  /* If we can't find the feature context it probably means that zmap hasn't had time to 
	   * fetch any features before the peer has sent a command needing features. */
	  zMapXMLParserRaiseParsingError(parser, "No features in context yet !") ;

	  request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;
	  result = FALSE ;
	}
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




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* HACK....function to remap coords to forward strand range and also swop
 * them as they get reversed in revcomping.... */
static void revcompTransChildCoordsCB(gpointer data, gpointer user_data)
{
  ZMapSpan child = (ZMapSpan)data ;
  ZMapWindow window = (ZMapWindow)user_data ;
  int chr_bp ;

  chr_bp = child->x1 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x1 = chr_bp ;

 
  chr_bp = child->x2 ;
  chr_bp = zmapWindowWorldToSequenceForward(window, chr_bp) ;
  child->x2 = chr_bp ;

  zMapUtilsSwop(int, child->x1, child->x2) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






