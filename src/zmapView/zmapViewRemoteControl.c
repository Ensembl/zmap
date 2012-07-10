/*  Last edited: Apr 11 10:51 2012 (edgrif) */
/*  File: zmapViewRemoteControl.c
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
 * Description: 
 *
 * Exported functions: See ZMap/zmapView.h
 * 
 * Created: Mon Mar 26 13:35:27 2012 (edgrif)
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapXML.h>

/* We are forced to include this to get SOURCE_GROUP_DELAYED, given that this symbol is only
 * used in zmapView that's where it should be defined.....I'll move it some other time. */
#include <ZMap/zmapConfigStanzaStructs.h>

#include <zmapView_P.h>



/* defunct list...... */
typedef enum
  {
    ZMAPVIEW_REMOTE_INVALID,
    /* Add below here... */

    ZMAPVIEW_REMOTE_FIND_FEATURE,
    ZMAPVIEW_REMOTE_CREATE_FEATURE,
    ZMAPVIEW_REMOTE_DELETE_FEATURE,
    ZMAPVIEW_REMOTE_GET_FEATURE_NAMES,
    ZMAPVIEW_REMOTE_LOAD_FEATURES,
    ZMAPVIEW_REMOTE_DUMP_CONTEXT,


    /* should be in window... */
    ZMAPVIEW_REMOTE_HIGHLIGHT_FEATURE,
    ZMAPVIEW_REMOTE_HIGHLIGHT2_FEATURE,
    ZMAPVIEW_REMOTE_UNHIGHLIGHT_FEATURE,




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* already in window */
    ZMAPVIEW_REMOTE_ZOOM_TO,
    ZMAPVIEW_REMOTE_GET_MARK,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* Not needed.... */
    ZMAPVIEW_REMOTE_REGISTER_CLIENT,
    ZMAPVIEW_REMOTE_LIST_WINDOWS,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* not implemented.... */
    ZMAPVIEW_REMOTE_NEW_WINDOW,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




    /* ...but above here */
    ZMAPVIEW_REMOTE_UNKNOWN
  } ZMapViewValidXRemoteActions ;







/* Type specifying whether feature must, must not exist or is irrelevant. */
typedef enum
  {
    FEATURE_INVALID,
    FEATURE_MUST,
    FEATURE_MUST_NOT,
    FEATURE_DONT_CARE
  } FeatureExistsType ;


/* Command descriptor table, records whether command edits the feature context and so on. */
typedef struct CommandDescriptorStructName
{
  char *command ;					    /* Command name/id. */
  GQuark command_id ;

  gboolean is_edit ;					    /* Does this command edit the feature context ? */
  FeatureExistsType exists ;				    /* Does the target of the command need
							       to exist/not exist etc. ? e.g. columns may be
							       dynamically loaded so may not exist yet. */

  gboolean needs_feature_desc ;				    /* Does command need a feature
							       description ? */

} CommandDescriptorStruct, *CommandDescriptor ;


/* Control block for all the remote actions. */
typedef struct RequestDataStructName
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  int code ;
  gboolean handled ;
  GString *messages ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Command. */
  char *command_name ;
  GQuark command_id ;
  CommandDescriptor cmd_desc ;

  /* Command result. */
  RemoteCommandRCType command_rc ;
  GString *err_msg ;


  /* View, window to apply command to. */
  ZMapAppRemoteViewID view_id ;

  ZMapView           view;

  /* some operations are "read only" and hence can use the orig_context, others require edits and
   * must create an edit_context to do these. */

  /* Note that only orig_context stays the same throughout processing of a request, other elements
   * will change if more than one align/block or whatever is specified in the request. */
  ZMapFeatureContext orig_context ;
  ZMapFeatureAlignment orig_align;
  ZMapFeatureBlock orig_block;
  ZMapFeatureSet orig_feature_set;
  ZMapFeature orig_feature;

  /* edit_context is constructed to carry out context chnages. */
  ZMapFeatureContext edit_context ;
  ZMapFeatureAlignment align;
  ZMapFeatureBlock block;
  ZMapFeatureSet feature_set;
  ZMapFeature feature;

  gboolean found_feature ;

  GQuark source_id ;
  GQuark style_id ;
  ZMapFeatureTypeStyle style ;

  GList *feature_list;

  GList *locations ;

  char *filename;
  char *format;

  gboolean use_mark ;
  GList *feature_sets ;

  gboolean zoomed ;

  int start, end ;

} RequestDataStruct, *RequestData ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
typedef struct
{
  ZMapViewValidXRemoteActions action;
  ZMapFeatureContext edit_context;
} PostExecuteDataStruct, *PostExecuteData;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







static void localProcessRemoteRequest(ZMapView view,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void processRequest(ZMapView view,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out) ;
static CommandDescriptor cmdGetDesc(GQuark command_id) ;

static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_export_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_align_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_block_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_featureset_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_featureset_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_feature_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);
static gboolean xml_return_true_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser);

static void copyAddFeature(gpointer key, gpointer value, gpointer user_data) ;
static ZMapFeature createLocusFeature(ZMapFeatureContext context, ZMapFeature feature,
				      ZMapFeatureSet locus_feature_set, ZMapFeatureTypeStyle locus_style,
				      GQuark new_locus_id) ;
static gboolean executeRequest(ZMapXMLParser parser, RequestData request_data) ;
static void viewDumpContextToFile(ZMapView view, RequestData request_data) ;
static void eraseFeatures(ZMapView view, RequestData request_data) ;
static gboolean findFeature(ZMapView view, RequestData request_data) ;
static gboolean mergeNewFeatures(ZMapView view, RequestData request_data) ;
static void draw_failed_make_message(gpointer list_data, gpointer user_data) ;
static void delete_failed_make_message(gpointer list_data, gpointer user_data) ;
static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data) ;
static ZMapFeatureContextExecuteStatus delete_from_list(GQuark key,
                                                        gpointer data,
                                                        gpointer user_data,
                                                        char **error_out) ;
static ZMapFeatureContextExecuteStatus mark_matching_invalid(GQuark key,
                                                             gpointer data,
                                                             gpointer user_data,
                                                             char **error_out) ;
static void loadFeatures(ZMapView view, RequestData request_data) ;
static void getFeatureNames(ZMapView view, RequestData request_data) ;
static void findUniqueCB(gpointer data, gpointer user_data) ;
static void makeUniqueListCB(gpointer key, gpointer value, gpointer user_data) ;







/*
 *               Globals
 */

/* Descriptor table of command attributes, e.g. 'edit' */
static CommandDescriptorStruct command_table_G[] =
  {
    {ZACP_FIND_FEATURE,      0, FALSE, FEATURE_DONT_CARE, TRUE},
    {ZACP_CREATE_FEATURE,    0, TRUE,  FEATURE_MUST_NOT,  TRUE},
    {ZACP_DELETE_FEATURE,    0, TRUE,  FEATURE_MUST,      TRUE},
    {ZACP_GET_FEATURE_NAMES, 0, FALSE, FEATURE_DONT_CARE, FALSE},
    {ZACP_LOAD_FEATURES,     0, TRUE,  FEATURE_DONT_CARE, FALSE},
    {ZACP_DUMP_FEATURES,     0, FALSE, FEATURE_DONT_CARE, FALSE},

    {NULL, 0, FALSE, FALSE}				    /* Terminator record. */
  } ;


/* xml parser callbacks */
static ZMapXMLObjTagFunctionsStruct view_starts_G[] = 
  {
    { "zmap",       xml_zmap_start_cb                  },
    { "request",    xml_request_start_cb               },
    { "align",      xml_align_start_cb                 },
    { "block",      xml_block_start_cb                 },
    { "featureset", xml_featureset_start_cb            },
    { "feature",    xml_feature_start_cb               },
    {NULL, NULL}
  };

static ZMapXMLObjTagFunctionsStruct view_ends_G[] =
  {
    { "zmap",       xml_return_true_cb    },
    { "request",    xml_request_end_cb    },
    { "align",      xml_return_true_cb    },
    { "block",      xml_return_true_cb    },
    { "featureset", xml_featureset_end_cb },
    { "feature",    xml_feature_end_cb    },
    { "subfeature", xml_subfeature_end_cb },
    {NULL, NULL}
  } ;




/* 
 *                     External interface
 */




/* See if view can process the command, if not then try all the windows to see if one can process the request. */
gboolean zMapViewProcessRemoteRequest(ZMapView view,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;

  if (strcmp(command_name, ZACP_CREATE_FEATURE) == 0
      || strcmp(command_name, ZACP_DELETE_FEATURE) == 0
      || strcmp(command_name, ZACP_FIND_FEATURE) == 0
      || strcmp(command_name, ZACP_LOAD_FEATURES) == 0)
    {
      localProcessRemoteRequest(view, command_name, view_id, request,
				app_reply_func, app_reply_data) ;

      result = TRUE ;					    /* Signal we have handled the request. */
    }
  else
    {
      GList* list_item ;
      gboolean window_found = FALSE ;

      list_item = g_list_first(view->window_list) ;
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  if (view_window->window == view_id->window)
	    {
	      window_found = TRUE ;
	      result = zMapWindowProcessRemoteRequest(view_window->window, command_name, view_id, request,
						      app_reply_func, app_reply_data) ;
	      break ;
	    }
	}
      while ((list_item = g_list_next(list_item))) ;

      /* If we were passed a window but did not find it this is an error... */
      if (!window_found)
	{
	  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  char *reason ;
	  ZMapXMLUtilsEventStack reply = NULL ;

	  reason = g_strdup_printf("window id %p not found.", view_id->window) ;

	  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

	  result = TRUE ;				    /* Signal we have handled the request. */
	}
    }

  return result ;
}



/* Process all peer requests that apply to view. */
static void localProcessRemoteRequest(ZMapView view,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func,
				      gpointer app_reply_data)
{
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  processRequest(view, command_name, view_id, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}


/* Commands by this routine need the xml to be parsed for arguments etc. */
static void processRequest(ZMapView view,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


  /* set up return code etc. for parsing code. */
  request_data.command_name = command_name ;
  request_data.command_id = g_quark_from_string(command_name) ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  request_data.view = view ;
  request_data.view_id = view_id ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, &view_starts_G[0], &view_ends_G[0]) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))

    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = g_strdup(zMapXMLParserLastErrorMsg(parser)) ;
    }
  else
    {
      request_data.err_msg = g_string_new("") ;

      if (!executeRequest(parser, &request_data))
	{
	  *reason_out = request_data.err_msg->str ;
	}
      *command_rc_out = request_data.command_rc ;

      g_string_free(request_data.err_msg, FALSE) ;
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser) ;

  return ;
}




/* XML tag handlers used to decode messages like these:
 *
 * <zmap action="create_feature">
 *   <featureset name="Coding">
 *     <feature name="RP6-206I17.6-001" start="3114" end="99885" strand="-">
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
 *    <featureset name="polyA_site">
 *      <feature name="U.2969591-3331416" start="87607" end="87608" strand="+"score="0.000"></feature>
 *    </featureset>
 *  </zmap>
 *
 */


static gboolean xml_zmap_start_cb(gpointer user_data, ZMapXMLElement zmap_element, ZMapXMLParser parser)
{
  /* actions all moved into the response tag, zmap tag will probably take on other meanings. */


  return TRUE ;
}



static gboolean xml_request_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  RequestData request_data = (RequestData)user_data ;
  ZMapXMLAttribute attr = NULL;
  char *err_msg = NULL ;

  request_data->cmd_desc = cmdGetDesc(request_data->command_id) ;

  zMapAssert(request_data->cmd_desc) ;


  request_data->orig_context = zMapViewGetFeatures(request_data->view) ;


  /* For actions that change the feature context create an "edit" context. */
  if (request_data->cmd_desc->is_edit)
    {
      request_data->edit_context
	= (ZMapFeatureContext)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_context)) ;
    }


  if (request_data->command_id == g_quark_from_string(ZACP_LOAD_FEATURES))
    {
      if ((attr = zMapXMLElementGetAttributeByName(set_element, "load")))
	{
	  GQuark load_id ;

	  load_id = zMapXMLAttributeGetValue(attr) ;

	  if (zMapLogQuarkIsStr(load_id, "mark"))
	    {
	      request_data->use_mark = TRUE ;
	    }
	  else if (zMapLogQuarkIsStr(load_id, "full"))
	    {
	      request_data->use_mark = FALSE ;
	    }
	  else
	    {
	      err_msg = g_strdup_printf("Value \"%s\" for \"load\" attr is unknown.", g_quark_to_string(load_id)) ;

	      result = FALSE ;
	    }
	}
    }

  if (!result)
    {
      request_data->command_rc = REMOTE_COMMAND_RC_BAD_XML ;

      zMapXMLParserRaiseParsingError(parser, err_msg) ;

      g_free(err_msg) ;
    }

  return result ;
}


static gboolean xml_request_end_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* WHY DO WE EXECUTE STUFF HERE....SHOULD BE WHEN WE RETURN FROM PARSING !!!!! */

  RequestData request_data = (RequestData)user_data ;

  result = executeRequest(parser, request_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}




/* this all seems a bit screwy...do export in a better xml way...... */
static gboolean xml_export_start_cb(gpointer user_data, ZMapXMLElement export_element, ZMapXMLParser parser)
{
  RequestData request_data = (RequestData)user_data ;
  ZMapXMLAttribute attr = NULL ;
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



static gboolean xml_align_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data;
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

	  result = FALSE ;
	}
    }
  else
    {
      /* default to master align. */
      request_data->orig_align = request_data->orig_context->master_align ;

      result = TRUE ;
    }

  if (result && request_data->cmd_desc->is_edit)
    {
      request_data->align = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_align)) ;

      zMapFeatureContextAddAlignment(request_data->edit_context, request_data->align, FALSE) ;
    }

  return result ;
}



static gboolean xml_block_start_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = FALSE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data;
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

  if (result && request_data->cmd_desc->is_edit)
    {
      request_data->block = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_block)) ;

      zMapFeatureAlignmentAddBlock(request_data->align, request_data->block) ;

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
  ZMapFeatureSet feature_set ;
  GQuark featureset_id ;
  char *featureset_name ;
  GQuark unique_set_id ;
  char *unique_set_name ;

  request_data->source_id = 0 ;			    /* reset needed for remove features. */


  /* We allow the client to leave out the align and block tags so if either is missing
   * use the defaults. */
  if (!(request_data->orig_align))
    {
      /* default to master align. */
      request_data->orig_align = request_data->orig_context->master_align ;

      if (request_data->cmd_desc->is_edit)
	{
	  request_data->align = (ZMapFeatureAlignment)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_align)) ;

	  zMapFeatureContextAddAlignment(request_data->edit_context, request_data->align, FALSE) ;
	}
    }

  if (!(request_data->orig_block))
    {
      /* Get the first one! */
      request_data->orig_block = zMap_g_hash_table_nth(request_data->orig_context->master_align->blocks, 0) ;

      if (request_data->cmd_desc->is_edit)
	{
	  request_data->block = (ZMapFeatureBlock)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_block)) ;

	  zMapFeatureAlignmentAddBlock(request_data->align, request_data->block) ;
	}
    }


  /* Now process the featureset. */
  if ((attr = zMapXMLElementGetAttributeByName(set_element, "name")))
    {
      /* Record original name and unique names for feature set, all look-ups are via the latter. */
      featureset_id = zMapXMLAttributeGetValue(attr) ;
      featureset_name = (char *)g_quark_to_string(featureset_id) ;

      request_data->source_id = unique_set_id = zMapFeatureSetCreateID(featureset_name) ;
      unique_set_name = (char *)g_quark_to_string(unique_set_id);

      /* Look for the feature set in the current context, it's an error if it's supposed to exist. */
      request_data->orig_feature_set = zMapFeatureBlockGetSetByID(request_data->orig_block, unique_set_id) ;

      if (!(request_data->orig_feature_set) && request_data->cmd_desc->exists == FEATURE_MUST)
	{
	  /* If we can't find the featureset but it's meant to exist then it's a serious error
	   * and we can't carry on. */
	  char *err_msg ;

	  err_msg = g_strdup_printf("Unknown Featureset \"%s\":  not found in original_block", featureset_name) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  result = FALSE ;
	}
      else
	{
	  result = TRUE ;
	}


      /* Processing for featuresets is different, if a featureset is marked to be dynamically
       * loaded it will not have been created yet so we shouldn't check for existence. */
      if (result && request_data->cmd_desc->is_edit)
	{
	  /* Make sure this feature set is a child of the block........ */
	  if (!(feature_set = zMapFeatureBlockGetSetByID(request_data->block, unique_set_id)))
	    {
	      feature_set = zMapFeatureSetCreate(featureset_name, NULL) ;
	      zMapFeatureBlockAddFeatureSet(request_data->block, feature_set) ;
	    }

	  request_data->feature_set = feature_set ;

	  result = TRUE ;
	}
    }
  else
    {
      /* No name, then get the first one! No one in their right mind should leave this to chance.... */
      request_data->orig_feature_set = zMap_g_hash_table_nth(request_data->orig_block->feature_sets, 0) ;

      if (request_data->cmd_desc->is_edit)
	request_data->feature_set
	  = (ZMapFeatureSet)zMapFeatureAnyCopy((ZMapFeatureAny)(request_data->orig_feature_set)) ;

      result = TRUE ;
    }

  /* We must be able to look up the column and other data for this featureset otherwise we
   * won't know which column/style etc to use. */
  if (result)
    {
      ZMapFeatureSource source_data ;

      if (!(source_data = g_hash_table_lookup(request_data->view->context_map.source_2_sourcedata,
					      GINT_TO_POINTER(unique_set_id))))
	{
	  char *err_msg ;

	  err_msg = g_strdup_printf("Source %s not found in view->context_map.source_2_sourcedata",
				    g_quark_to_string(unique_set_id)) ;
	  zMapXMLParserRaiseParsingError(parser, err_msg) ;
	  g_free(err_msg) ;

	  result = FALSE ;
	}
      else
	{
	  request_data->style_id = source_data->style_id ;
	}

      if (result && (request_data->command_id != g_quark_from_string(ZACP_LOAD_FEATURES)))
	{
	  // check style for all command except load_features
	  // as load features processes a complete step list that inludes requesting the style
	  // then if the style is not there then we'll drop the features

	  if (!(request_data->style = zMapFindStyle(request_data->view->context_map.styles, request_data->style_id)))
	    {
	      char *err_msg ;

	      err_msg = g_strdup_printf("Style %s not found in view->context_map.styles",
					g_quark_to_string(request_data->style_id)) ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      result = FALSE ;
	    }
	}
    }

  if (result)
    {
      if (request_data->cmd_desc->is_edit)
	request_data->edit_context->req_feature_set_names
	  = g_list_append(request_data->edit_context->req_feature_set_names, GINT_TO_POINTER(featureset_id)) ;

      if (request_data->command_id == g_quark_from_string(ZACP_LOAD_FEATURES))
	{
	  request_data->feature_sets = g_list_append(request_data->feature_sets, GINT_TO_POINTER(featureset_id)) ;
	}
      else if (request_data->command_id == g_quark_from_string(ZACP_GET_FEATURE_NAMES))
	{
	  /* NOTE SURE WHY THIS IS HERE.....should be in the feature handler ?? */

	  if (result && (attr = zMapXMLElementGetAttributeByName(set_element, "start")))
	    {
	      request_data->start = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
					     (char **)NULL, 10);
	    }
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "start is a required attribute for feature.");
	      result = FALSE ;
	    }

	  if (result && (attr = zMapXMLElementGetAttributeByName(set_element, "end")))
	    {
	      request_data->end = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
					 (char **)NULL, 10);
	    }
	  else
	    {
	      zMapXMLParserRaiseParsingError(parser, "end is a required attribute for feature.");
	      result = FALSE ;
	    }
	}
    }

  return result ;
}


static gboolean xml_featureset_end_cb(gpointer user_data, ZMapXMLElement set_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  RequestData request_data = (RequestData)user_data;

  /* Only do stuff if a feature set was found and has features. */
  if (request_data->feature_set && request_data->feature_set->features)
    {
      /* If deleting and no feature was specified then delete them all. NOTE that if
       * a source name was given then only features from _that_ source are deleted, not
       * all features in the column. */
      if (request_data->command_id == g_quark_from_string(ZACP_DELETE_FEATURE)
	  && !(request_data->feature_list))
	{
	  g_hash_table_foreach(request_data->orig_feature_set->features,
			       copyAddFeature, request_data) ;
	}
    }

  return result ;
}



static gboolean xml_feature_start_cb(gpointer user_data, ZMapXMLElement feature_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data;
  ZMapFeatureAny feature_any;
  ZMapStrand strand = ZMAPSTRAND_NONE;
  ZMapPhase start_phase = ZMAPPHASE_NONE ;
  gboolean has_score = FALSE, start_not_found = FALSE, end_not_found = FALSE;
  char *feature_name ;
  GQuark feature_name_id, feature_unique_id ;
  int start = 0, end = 0 ;
  double score = 0.0 ;
  ZMapFeature feature ;
  ZMapStyleMode mode ;


  /* Must have following attributes for all feature level operations. */
  if (result && (attr = zMapXMLElementGetAttributeByName(feature_element, "name")))
    {
      feature_name_id = zMapXMLAttributeGetValue(attr) ;
      feature_name = (char *)g_quark_to_string(feature_name_id) ;
    }
  else
    {
      zMapXMLParserRaiseParsingError(parser, "\"name\" is a required attribute for feature.") ;
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
      result = FALSE ;
    }

  /* Need the style to get hold of the feature mode, better would be for
   * xml to contain SO term ?? Maybe not....not sure. */
  if (result && ((mode = zMapStyleGetMode(request_data->style)) == ZMAPSTYLE_MODE_INVALID))
    {
      zMapXMLParserRaiseParsingError(parser, "\"style\" must have a valid feature mode.");
      result = FALSE ;
    }


  /* Check if feature exists, for some commands it must do, for others it must not. */
  if (result)
    {
      feature_unique_id = zMapFeatureCreateID(mode, feature_name, strand, start, end, 0, 0) ;

      feature = zMapFeatureSetGetFeatureByID(request_data->orig_feature_set, feature_unique_id) ;

      if (request_data->cmd_desc->exists == FEATURE_MUST)
	{
	  if (!feature)
	    {
	      /* If we _don't_ find the feature then it's a serious error for these commands. */
	      char *err_msg ;

	      err_msg = g_strdup_printf("Feature \"%s\" with id \"%s\" could not be found in featureset \"%s\",",
					feature_name, g_quark_to_string(feature_unique_id),
					zMapFeatureName((ZMapFeatureAny)(request_data->orig_feature_set))) ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      result = FALSE ;
	    }
	}
      else if (request_data->cmd_desc->exists == FEATURE_DONT_CARE)
	{
	  if (request_data->command_id == g_quark_from_string(ZACP_FIND_FEATURE))
	    {
	      if (feature)
		request_data->found_feature = TRUE ;
	      else
		request_data->found_feature = FALSE ;
	    }
	}
      else if (request_data->cmd_desc->exists == FEATURE_MUST_NOT)
	{
	  if (feature)
	    {
	      /* If we _do_ find the feature then it's a serious error. */
	      char *err_msg ;

	      err_msg = g_strdup_printf("Feature \"%s\" with id \"%s\" already exists in featureset \"%s\",",
					feature_name, g_quark_to_string(feature_unique_id),
					zMapFeatureName((ZMapFeatureAny)(request_data->orig_feature_set))) ;
	      zMapXMLParserRaiseParsingError(parser, err_msg) ;
	      g_free(err_msg) ;

	      result = FALSE ;
	    }
	}
    }


  /* Now do something... */
  if (result)
    {
      /* Having both options seems redundant.... */
      if (request_data->cmd_desc->is_edit && request_data->cmd_desc->needs_feature_desc)
	{
	  if ((attr = zMapXMLElementGetAttributeByName(feature_element, "score")))
	    {
	      score = zMapXMLAttributeValueToDouble(attr) ;
	      has_score = TRUE ;
	    }

	  if ((attr = zMapXMLElementGetAttributeByName(feature_element, "start_not_found")))
	    {
	      /* I don't understand what this ghastliness is....how can phase be -ve ?? */

	      start_phase = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
				   (char **)NULL, 10);
	      start_not_found = TRUE;

	      switch (start_phase)
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

	  if ((attr = zMapXMLElementGetAttributeByName(feature_element, "end_not_found")))
	    {
	      end_not_found = zMapXMLAttributeValueToBool(attr);
	    }

	  if (result
	      && (request_data->feature = zMapFeatureCreateFromStandardData(feature_name, NULL, "",
									    mode,
									    request_data->style,
									    start, end, has_score,
									    score, strand)))
	    {
	      request_data->feature->style_id = request_data->style_id ;

	      zMapFeatureSetAddFeature(request_data->feature_set, request_data->feature);

	      /* We need to get all the source info. in here somehow.... */
	      zMapFeatureAddText(request_data->feature, request_data->source_id, NULL, NULL) ;

	      if (start_not_found || end_not_found)
		{
		  zMapFeatureAddTranscriptStartEnd(request_data->feature, start_not_found,
						   start_phase, end_not_found);
		}


	      if ((attr = zMapXMLElementGetAttributeByName(feature_element, "locus")))
		{
		  ZMapFeatureSet locus_feature_set = NULL;
		  ZMapFeature locus_feature = NULL;
		  GQuark new_locus_id  = zMapXMLAttributeGetValue(attr);
		  GQuark locus_set_id  = zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME);
		  char *new_locus_name = (char *)g_quark_to_string(new_locus_id);
		  ZMapFeatureTypeStyle locus_style ;

		  if (!(locus_style
			= zMapFindStyle(request_data->view->context_map.styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_LOCUS_NAME))))
		    {
		      zMapLogWarning("Feature %s created but Locus %s could not be added"
				     " because Locus style could not be found.",
				     feature_name, new_locus_name) ;
		    }
		  else
		    {
		      /* Find locus feature set first ... */
		      if (!(locus_feature_set = zMapFeatureBlockGetSetByID(request_data->block, locus_set_id)))
			{
			  /* Can't find one, create one and add it. */
			  locus_feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_LOCUS_NAME, NULL);
			  zMapFeatureBlockAddFeatureSet(request_data->block, locus_feature_set);
			}

		      locus_feature = createLocusFeature(request_data->orig_context, request_data->feature,
							 locus_feature_set, locus_style, new_locus_id) ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

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

		      if ((old_feature) && (old_feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
			  && (old_feature->feature.transcript.locus_id != 0)
			  && (old_feature->feature.transcript.locus_id != new_locus_id))
			{
			  /* ^^^ check the old one was a transcript and had a locus that doesn't match this one */
			  ZMapFeature tmp_locus_feature;
			  ZMapFeatureAny old_locus_feature;
			  char *old_locus_name = (char *)g_quark_to_string(old_feature->feature.transcript.locus_id);

			  /* I DON'T REALLY UNDERSTAND THIS BIT CURRENTLY...WHY DO WE ALWAYS LOG THIS... */
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
										NULL, "",
										ZMAPSTYLE_MODE_BASIC, locus_style,
										start, end, FALSE, 0.0,
										ZMAPSTRAND_NONE) ;

			  zMapFeatureSetAddFeature(locus_feature_set, tmp_locus_feature);

			  if ((old_locus_feature
			       = zMapFeatureContextFindFeatureFromFeature(request_data->orig_context,
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
										NULL, "",
										ZMAPSTYLE_MODE_BASIC, locus_style,
										start, end, FALSE, 0.0,
										ZMAPSTRAND_NONE) ;
			    }

			  zMapFeatureDestroy(tmp_locus_feature);
			}
		      else
			{
			  /* make the locus feature itself. */
			  locus_feature = zMapFeatureCreateFromStandardData(new_locus_name,
									    NULL, "",
									    ZMAPSTYLE_MODE_BASIC, locus_style,
									    start, end, FALSE, 0.0,
									    ZMAPSTRAND_NONE) ;

			}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




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

	      /* THIS DOESN'T DO A DEEP ENOUGH COPY, WE FAIL LATER FOR SOME ACTIONS
	       * BECAUSE STUFF LIKE THE HOMOL DATA IS NOT COPIED SO WE CAN'T FIND
	       * THE FEATURE IN THE HASH. */
	      feature_any = zMapFeatureAnyCopy((ZMapFeatureAny)request_data->feature) ;

	      request_data->feature_list = g_list_prepend(request_data->feature_list, feature_any);
	    }

	}
    }

  return result ;
}


static gboolean xml_feature_end_cb(gpointer user_data, ZMapXMLElement sub_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  RequestData request_data = (RequestData)user_data;

  if (request_data->command_id == g_quark_from_string(ZACP_CREATE_FEATURE))
    {
      /* I don't really like this error handling, it seems wierd.... */
      if (!(request_data->feature))
	{
	  zMapXMLParserCheckIfTrueErrorReturn(request_data->feature == NULL,
					      parser,
					      "a feature end tag without a created feature.") ;

	  result = FALSE ;
	}
      else
	{
	  /* It's probably here that we need to revcomp the feature if the
	   * view is revcomp'd.... */
	  if (request_data->view->revcomped_features)
	    {
	      zMapFeatureReverseComplement(request_data->orig_context, request_data->feature) ;
	    }
	}
    }

  return result ;
}



static gboolean xml_subfeature_end_cb(gpointer user_data, ZMapXMLElement sub_element, ZMapXMLParser parser)
{
  gboolean result = TRUE ;
  ZMapXMLAttribute attr = NULL;
  RequestData request_data = (RequestData)user_data;
  ZMapFeature feature = NULL;

  zMapXMLParserCheckIfTrueErrorReturn(request_data->feature == NULL,
                                      parser,
                                      "a feature end tag without a created feature.");
  feature = request_data->feature;

  if ((attr = zMapXMLElementGetAttributeByName(sub_element, "ontology")))
    {
      GQuark ontology ;
      ZMapSpanStruct span = {0} ;
      ZMapSpan exon_ptr = NULL, intron_ptr = NULL ;
      gboolean valid = TRUE ;

      ontology = zMapXMLAttributeGetValue(attr) ;

      feature->type = ZMAPSTYLE_MODE_TRANSCRIPT;

      if (valid && (attr = zMapXMLElementGetAttributeByName(sub_element, "start")))
        {
          span.x1 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                           (char **)NULL, 10);
        }
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "start is a required attribute for subfeature.");

	  valid = FALSE ;
	}

      if (valid && (attr = zMapXMLElementGetAttributeByName(sub_element, "end")))
        {
          span.x2 = strtol((char *)g_quark_to_string(zMapXMLAttributeGetValue(attr)),
                           (char **)NULL, 10) ;
        }
      else
	{
	  zMapXMLParserRaiseParsingError(parser, "end is a required attribute for subfeature.");

	  valid = FALSE ;
	}

      if (valid)
	{
	  if (ontology == g_quark_from_string("cds"))
	    {
	      zMapFeatureAddTranscriptCDS(feature, TRUE, span.x1, span.x2) ;
	    }
	  else
	    {
	      if (ontology == g_quark_from_string("exon"))
		exon_ptr = &span ;
	      else if (ontology == g_quark_from_string("intron"))
		intron_ptr = &span ;

	      zMapFeatureAddTranscriptExonIntron(feature, exon_ptr, intron_ptr) ;
	    }
	}
    }

  return result ;                  /* tell caller to clean us up. */
}



static gboolean xml_return_true_cb(gpointer user_data,
                                   ZMapXMLElement zmap_element,
                                   ZMapXMLParser parser)
{
  return TRUE;
}





static ZMapFeature createLocusFeature(ZMapFeatureContext context, ZMapFeature feature,
				      ZMapFeatureSet locus_feature_set, ZMapFeatureTypeStyle locus_style,
				      GQuark new_locus_id)
{
  ZMapFeature locus_feature = NULL;
  ZMapFeature old_feature;
  char *new_locus_name = (char *)g_quark_to_string(new_locus_id);
  int start, end ;

  /* Not sure I really get this comment.... */

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

  start = feature->x1 ;
  end = feature->x2 ;

  old_feature = (ZMapFeature)zMapFeatureContextFindFeatureFromFeature(context, (ZMapFeatureAny)feature) ;

  if ((old_feature) && (old_feature->type == ZMAPSTYLE_MODE_TRANSCRIPT)
      && (old_feature->feature.transcript.locus_id != 0)
      && (old_feature->feature.transcript.locus_id != new_locus_id))
    {
      /* ^^^ check the old one was a transcript and had a locus that doesn't match this one */
      ZMapFeature tmp_locus_feature;
      ZMapFeatureAny old_locus_feature;
      char *old_locus_name = (char *)g_quark_to_string(old_feature->feature.transcript.locus_id);

      /* I DON'T REALLY UNDERSTAND THIS BIT CURRENTLY...WHY DO WE ALWAYS LOG THIS... */
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
							    NULL, "",
							    ZMAPSTYLE_MODE_BASIC, locus_style,
							    start, end, FALSE, 0.0,
							    ZMAPSTRAND_NONE) ;

      zMapFeatureSetAddFeature(locus_feature_set, tmp_locus_feature);

      if ((old_locus_feature
	   = zMapFeatureContextFindFeatureFromFeature(context,
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
							    NULL, "",
							    ZMAPSTYLE_MODE_BASIC, locus_style,
							    start, end, FALSE, 0.0,
							    ZMAPSTRAND_NONE) ;
	}

      zMapFeatureDestroy(tmp_locus_feature);
    }
  else
    {
      /* make the locus feature itself. */
      locus_feature = zMapFeatureCreateFromStandardData(new_locus_name,
							NULL, "",
							ZMAPSTYLE_MODE_BASIC, locus_style,
							start, end, FALSE, 0.0,
							ZMAPSTRAND_NONE) ;

    }

  return locus_feature ;
}


/* A GHFunc() to copy a feature and add it to the supplied list. */
static void copyAddFeature(gpointer key, gpointer value, gpointer user_data)
{
  ZMapFeature feature = (ZMapFeature)value ;
  RequestData request_data = (RequestData)user_data ;
  ZMapFeatureAny feature_any ;

  /* Only add to this list if there is no source name or the feature and source
   * names match, caller will have given a source name. */
  if (!(request_data->source_id) || feature->source_id == request_data->source_id)
    {
      /* THIS DOESN'T DO A DEEP ENOUGH COPY, WE FAIL LATER FOR SOME ACTIONS
       * BECAUSE STUFF LIKE THE HOMOL DATA IS NOT COPIED SO WE CAN'T FIND
       * THE FEATURE IN THE HASH. */
      if ((feature_any = zMapFeatureAnyCopy((ZMapFeatureAny)feature)))
	{
	  ZMapFeatureAny feature_copy ;

	  zMapFeatureSetAddFeature(request_data->feature_set, (ZMapFeature)feature_any) ;

	  feature_copy = zMapFeatureAnyCopy(feature_any) ;
	  request_data->feature_list = g_list_append(request_data->feature_list, feature_copy) ;
	}
    }

  return ;
}


/* Function assumes that command_name will be found in table which is
 * reasonable as code only gets this far if it knows about the command. */
static CommandDescriptor cmdGetDesc(GQuark command_id)
{
  CommandDescriptor cmd_desc = NULL ;
  CommandDescriptor curr_desc ;

  curr_desc = &(command_table_G[0]) ;

  /* First time through init the quark values for quick comparisons... */
  if (!(curr_desc->command_id))
    {
      while ((curr_desc->command))
	{
	  curr_desc->command_id = g_quark_from_string(curr_desc->command) ;

	  curr_desc++ ;
	}

      curr_desc = &(command_table_G[0]) ;		    /* reset to start. */
    }

  /* Find this commands description. */
  while ((curr_desc->command))
    {
      if (command_id == curr_desc->command_id)
	{
	  cmd_desc = curr_desc ;

	  break ;
	}

      curr_desc++ ;
    }

  return cmd_desc ;
}



/* This is where after all the parsing we finally execute the requests. */
static gboolean executeRequest(ZMapXMLParser parser, RequestData request_data)
{
  gboolean result = TRUE ;
  ZMapView view = request_data->view ;

  if (request_data->command_id == g_quark_from_string(ZACP_GET_FEATURE_NAMES))
    {
      getFeatureNames(view, request_data) ;
    }
  else if (request_data->command_id == g_quark_from_string(ZACP_LOAD_FEATURES))
    {
      loadFeatures(view, request_data) ;
    }
  else if (request_data->command_id == g_quark_from_string(ZACP_FIND_FEATURE))
    {
      findFeature(view, request_data) ;
    }
  else if (request_data->command_id == g_quark_from_string(ZACP_DELETE_FEATURE))
    {
      eraseFeatures(view, request_data) ;
    }
  else if (request_data->command_id == g_quark_from_string(ZACP_CREATE_FEATURE))
    {
      /* mergeNewFeatures sets the request error code/msg if it fails. */
      if (mergeNewFeatures(view, request_data))
	zmapViewDrawDiffContext(view, &(request_data->edit_context)) ;
      else
	result = FALSE ;
    }
  else if (request_data->command_id == g_quark_from_string(ZACP_DUMP_FEATURES))
    {
      viewDumpContextToFile(view, request_data) ;
    }

  if (request_data->edit_context)
    {
      zMapFeatureContextDestroy(request_data->edit_context, TRUE) ;
      request_data->edit_context = NULL ;
    }

  return result ;
}







static void viewDumpContextToFile(ZMapView view, RequestData request_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GIOChannel *file = NULL;
  GError *error = NULL;
  char *filepath = NULL;

  filepath = request_data->filename;

  if (!(file = g_io_channel_new_file(filepath, "w", &error)))
    {
      request_data->code = ZMAPXREMOTE_UNAVAILABLE;
      request_data->handled = FALSE;
      if (error)
	g_string_append(request_data->err_msg, error->message);
    }
  else
    {
      char *format = NULL;
      gboolean result = FALSE;

      format = request_data->format;

      if(format && g_ascii_strcasecmp(format, "gff") == 0)
	result = zMapGFFDump((ZMapFeatureAny)view->features, view->context_map.styles, file, &error);

#ifdef STYLES_PRINT_TO_FILE
      else if(format && g_ascii_strcasecmp(format, "test-style") == 0)
	zMapFeatureTypePrintAll(view->features->styles, __FILE__);
#endif /* STYLES_PRINT_TO_FILE */

      else
	result = zMapFeatureContextDump(view->features, view->context_map.styles, file, &error);

      if(!result)
	{
	  request_data->code    = ZMAPXREMOTE_INTERNAL;
	  request_data->handled = FALSE;
	  if(error)
	    g_string_append(request_data->err_msg, error->message);
	}
      else
	{
	  request_data->code    = ZMAPXREMOTE_OK;
	  request_data->handled = TRUE;
	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean sanityCheckContext(ZMapView view, RequestData request_data)
{
  gboolean features_are_sane = TRUE ;
  ZMapXRemoteStatus save_code ;

  save_code = request_data->code ;
  request_data->code = 0 ;

  zMapFeatureContextExecute((ZMapFeatureAny)(request_data->edit_context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            sanity_check_context,
                            request_data) ;

  if (request_data->code != 0)
    features_are_sane = FALSE ;
  else
    request_data->code = save_code ;

  return features_are_sane ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Remove a list of features from the context. */
static void eraseFeatures(ZMapView view, RequestData request_data)
{
  zmapViewEraseFromContext(view, request_data->edit_context);

  zMapFeatureContextExecute((ZMapFeatureAny)(request_data->edit_context),
                            ZMAPFEATURE_STRUCT_FEATURE,
                            mark_matching_invalid,
                            &(request_data->feature_list));

  /* If anything failed then delete_failed_make_message() will set command_rc and err_msg. */
  request_data->command_rc = REMOTE_COMMAND_RC_OK ;
  if (g_list_length(request_data->feature_list))
    g_list_foreach(request_data->feature_list, delete_failed_make_message, request_data);

  return ;
}


static gboolean findFeature(ZMapView view, RequestData request_data)
{
  gboolean result = FALSE ;

  if (request_data->found_feature)
    {
      request_data->command_rc = REMOTE_COMMAND_RC_OK ;

      result = TRUE ;
    }
  else
    {
      request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;

      g_string_append(request_data->err_msg, "Cannot find feature") ;
    }

  return result ;
}


static gboolean mergeNewFeatures(ZMapView view, RequestData request_data)
{
  gboolean result = FALSE ;

  if (!(request_data->edit_context = zmapViewMergeInContext(view, request_data->edit_context)))
    {
      request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;

      g_string_append(request_data->err_msg, "Merge of new feature into View feature context failed") ;
    }
  else
    {
      zMapFeatureContextExecute((ZMapFeatureAny)(request_data->edit_context),
				ZMAPFEATURE_STRUCT_FEATURE,
				delete_from_list,
				&(request_data->feature_list));

      zMapFeatureContextExecute((ZMapFeatureAny)(view->features),
				ZMAPFEATURE_STRUCT_FEATURE,
				mark_matching_invalid,
				&(request_data->feature_list));

      request_data->command_rc = REMOTE_COMMAND_RC_OK ;

      if (g_list_length(request_data->feature_list))
	g_list_foreach(request_data->feature_list, draw_failed_make_message, request_data);

      result = TRUE ;
    }

  return result ;
}


/* We get called for any features that failed to get drawn but we may not know why
 * as the error will have been reported by the merge, we can at least tell user
 * to look at log. */
static void draw_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  RequestData request_data = (RequestData)user_data;

  request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_INVALID)
    {
      g_string_append_printf(request_data->err_msg,
			     "Failed to draw feature '%s' [%s]. Feature already exists.\n",
			     (char *)g_quark_to_string(feature_any->original_id),
			     (char *)g_quark_to_string(feature_any->unique_id));
    }
  else
    {
      

      g_string_append_printf(request_data->err_msg,
			     "Failed to draw feature '%s' [%s]. Unknown reason, check zmap log file.\n",
			     (char *)g_quark_to_string(feature_any->original_id),
			     (char *)g_quark_to_string(feature_any->unique_id));
    }

  return ;
}

static void delete_failed_make_message(gpointer list_data, gpointer user_data)
{
  ZMapFeatureAny feature_any = (ZMapFeatureAny)list_data;
  RequestData request_data = (RequestData)user_data;

  if (feature_any->struct_type == ZMAPFEATURE_STRUCT_INVALID)
    {
      request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;

      g_string_append_printf(request_data->err_msg,
                             "Failed to delete feature '%s' [%s].\n",
                             (char *)g_quark_to_string(feature_any->original_id),
                             (char *)g_quark_to_string(feature_any->unique_id));
    }

  return ;
}

static gint matching_unique_id(gconstpointer list_data, gconstpointer user_data)
{
  gint match = -1;
  ZMapFeatureAny a = (ZMapFeatureAny)list_data, b = (ZMapFeatureAny)user_data ;

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

  if (any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if ((match = g_list_find_custom(*list, any, matching_unique_id)))
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

  if (any->struct_type == ZMAPFEATURE_STRUCT_FEATURE)
    {
      if ((match = g_list_find_custom(*list, any, matching_unique_id)))
        {
          any = (ZMapFeatureAny)(match->data);
          any->struct_type = ZMAPFEATURE_STRUCT_INVALID;
        }
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* CURRENTLY DISABLED BECAUSE IT DOESN'T GO NEARLY FAR ENOUGH.... */
static ZMapFeatureContextExecuteStatus sanity_check_context(GQuark key,
                                                            gpointer data,
                                                            gpointer user_data,
                                                            char **error_out)
{
  ZMapFeatureAny any = (ZMapFeatureAny)data;
  RequestData request_data = (RequestData)user_data;
  char *reason = NULL;

  if (!zMapFeatureAnyIsSane(any, &reason))
    {
      request_data->command_rc = ZMAPXREMOTE_BADREQUEST;
      request_data->err_msg = g_string_append(request_data->err_msg, reason) ;

      if (reason)
	g_free(reason);
    }

  return ZMAP_CONTEXT_EXEC_STATUS_OK ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Load features on request from a client. */
static void loadFeatures(ZMapView view, RequestData request_data)
{
  gboolean use_mark = FALSE ;
  int start = 0, end = 0 ;

  request_data->command_rc = REMOTE_COMMAND_RC_OK ;

  /* If mark then get mark, otherwise get big start/end. */
  if (request_data->use_mark)
    {
      ZMapWindow window ;

      window = request_data->view_id->window ;

      if ((zMapWindowMarkGetSequenceSpan(window, &start, &end)))
	{
	  use_mark = TRUE ;
	}
      else
	{
	  g_string_append(request_data->err_msg, "Load features to marked region failed: no mark set.") ;
	  request_data->command_rc = REMOTE_COMMAND_RC_FAILED ;
	}
    }
  else
    {
      start = request_data->block->block_to_sequence.parent.x1 ;
      end = request_data->block->block_to_sequence.parent.x2 ;
    }

  if (request_data->command_rc == REMOTE_COMMAND_RC_OK)
    zmapViewLoadFeatures(view, request_data->block, request_data->feature_sets, start, end,
			 SOURCE_GROUP_DELAYED, TRUE, TRUE) ;

  return ;
}



/* Get the names of all features within a given range. */
static void getFeatureNames(ZMapView view, RequestData request_data)
{
  ZMapViewWindow view_window ;
  ZMapWindow window ;

  request_data->command_rc = ZMAPXREMOTE_OK ;

  /* Hack, just grab first window...work out what to do about this.... */
  view_window = (ZMapViewWindow)(request_data->view->window_list->data) ;
  window = view_window->window ;


  /* IS THIS RIGHT ??? AREN'T WE USING PARENT COORDS...??? */
  if (request_data->start < request_data->block->block_to_sequence.block.x2
      || request_data->end > request_data->block->block_to_sequence.block.x2)
    {
      g_string_append_printf(request_data->err_msg,
			     "Requested coords (%d, %d) are outside of block coords (%d, %d).",
			     request_data->start, request_data->end,
			     request_data->block->block_to_sequence.block.x1,
			     request_data->block->block_to_sequence.block.x2) ;

      request_data->command_rc = ZMAPXREMOTE_BADREQUEST;
    }
  else
    {
      GList *feature_list ;

      if (!(feature_list = zMapFeatureSetGetRangeFeatures(request_data->feature_set,
							  request_data->start, request_data->end)))
	{
	  g_string_append_printf(request_data->err_msg,
				 "No features found for feature set \"%s\" in range (%d, %d).",
				 zMapFeatureSetGetName(request_data->feature_set),
				 request_data->start, request_data->end) ;

	  request_data->command_rc = ZMAPXREMOTE_NOCONTENT ;
	}
      else
	{
	  GHashTable *uniq_features = g_hash_table_new(NULL,NULL) ;

	  g_list_foreach(feature_list, findUniqueCB, &uniq_features) ;

	  g_hash_table_foreach(request_data->feature_set->features, makeUniqueListCB, request_data->err_msg) ;
	}
    }

  return ;
}


/* A GFunc() to add feature names to a hash list. */
static void findUniqueCB(gpointer data, gpointer user_data)
{
  GHashTable **uniq_features_ptr = (GHashTable **)user_data ;
  GHashTable *uniq_features = *uniq_features_ptr ;
  ZMapFeature feature = (ZMapFeature)user_data ;

  g_hash_table_insert(uniq_features, GINT_TO_POINTER(feature->original_id), NULL) ;

  *uniq_features_ptr = uniq_features ;

  return ;
}

/* A GHFunc() to add a feature to a list if it within a given range */
static void makeUniqueListCB(gpointer key, gpointer value, gpointer user_data)
{
  GQuark feature_id = GPOINTER_TO_INT(key) ;
  GString *list_str = (GString *)user_data ;

  g_string_append_printf(list_str, " %s ;", g_quark_to_string(feature_id)) ;

  return ;
}


