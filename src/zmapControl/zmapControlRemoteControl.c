/*  File: zmapControlRemote.c
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
 *    Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 * Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions to support the remote control interface
 *              to zmap.
 *              Requests are made to code in zMapApp by a registered
 *              callback function. This code also receives requests
 *              from zMapApp and handles commands it knows and forwards
 *              ones it doesn't to ZMapWindow.
 *
 * Exported functions: See ZMap/zmapControl.h
 * HISTORY:
 * Last edited: Apr 11 10:51 2012 (edgrif)
 * Created: Fri Feb 17 09:16:41 2012 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapView.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsXRemote.h>
#include <ZMap/zmapRemoteCommand.h>
#include <zmapControl_P.h>



/*                     NEW STUFF..................               */

/* Used to build xml giving the list of deleted views. */
typedef struct GetViewNamesStructName
{
  gboolean result ;

  void *curr_zmap ;
  void *curr_view ;

  ZMapXMLUtilsEventStack view_elements ;
  ZMapXMLUtilsEventStack curr_element ;
} GetViewNamesStruct, *GetViewNames ;





static void localProcessRemoteRequest(ZMap zmap,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;

static void processRequest(ZMap zmap,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out) ;


static void handlePeerReply(char *command,
			    RemoteCommandRCType command_rc, char *reason, char *reply,
			    gpointer reply_handler_func_data) ;

static void doViews(gpointer data, gpointer user_data) ;
static void doWindows(gpointer data, gpointer user_data) ;

static ZMapXMLUtilsEventStack makeViewElement(ZMap zmap, ZMapView view, ZMapWindow window) ;




/*                 OLD STUFF...............                          */


/* IT'S LOOKING LIKE THERE ARE NO COMMANDS AT THE CONTROL LEVEL AT THE MOMENT.... */


enum
  {
    ZMAPCONTROL_REMOTE_INVALID,
    /* Add below here... */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* These appear to be totally unknown.....sigh.... */
    ZMAPCONTROL_REMOTE_ZOOM_IN,
    ZMAPCONTROL_REMOTE_ZOOM_OUT,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    /* REGISTER_CLIENT IS NOT NEEDED AND THE OTHER TWO ARE HANDLED AT THE APP LEVEL. */
    ZMAPCONTROL_REMOTE_REGISTER_CLIENT,
    ZMAPCONTROL_REMOTE_NEW_VIEW,
    ZMAPCONTROL_REMOTE_CLOSE_VIEW,

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

  unsigned long xwid ;

  ViewConnectDataStruct view_params;
}RequestDataStruct, *RequestData;

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




/* 
 *                  Globals.
 */

static gboolean control_execute_debug_G = FALSE;








/* 
 *                 External routines.
 */



/* Receives peer program requests from layer above and either handles them or forwards them
 * to ZMapView. */
gboolean zMapControlProcessRemoteRequest(ZMap zmap,
					 char *command_name, ZMapAppRemoteViewID view_id, char *request,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;

  if (strcmp(command_name, ZACP_NEWVIEW) == 0
      || strcmp(command_name, ZACP_CLOSEVIEW) == 0)
    {
      localProcessRemoteRequest(zmap, command_name, view_id, request,
				app_reply_func, app_reply_data) ;

      result = TRUE ;
    }
  else if (zmap->view_list)
    {
      GList *next_view ;
      gboolean view_found = FALSE ;

      /* Try all the views. */
      next_view = g_list_first(zmap->view_list) ;
      do
	{
	  ZMapView view = (ZMapView)(next_view->data) ;

	  if (view == view_id->view)
	    {
	      view_found = TRUE ;
	      result = zMapViewProcessRemoteRequest(view, command_name, view_id, request,
						    app_reply_func, app_reply_data) ;

	      break ;
	    }
	}
      while ((next_view = g_list_next(next_view))) ;

      /* If we were passed a view but did not find it this is an error... */
      if (!view_found)
	{
	  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  char *reason ;
	  ZMapXMLUtilsEventStack reply = NULL ;

	  reason = g_strdup_printf("view id %p not found.", view_id->view) ;

	  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

	  result = TRUE ;				    /* Signals we have handled the request. */
	}
    }

  return result ;
}




/* 
 *                            Package routines
 */


/* User interaction has created a new view (by splitting an existing one). */
void zmapControlSendViewCreated(ZMap zmap, ZMapView view, ZMapWindow window)
{
  ZMapXMLUtilsEventStack view_element ;

  view_element = makeViewElement(zmap, view, window) ;

  (*(zmap->zmap_cbs_G->remote_request_func))(ZACP_VIEW_CREATED, view_element,
					     zmap->app_data,
					     handlePeerReply, zmap) ;

  return ;
}


/* User interaction has resulted in view(s)/window(s) being deleted, send message
 * to peer giving ids of views/windows that have gone. */
void zmapControlSendViewDeleted(ZMap zmap, ZMapViewWindowTree destroyed_zmaps)
{
  GetViewNamesStruct get_view_names = {TRUE, NULL} ;
  static ZMapXMLUtilsEventStackStruct			    /* Should dynamically allocate... */
    view_reply[20] =
    {
      {ZMAPXML_NULL_EVENT}
    } ;

  /* collect all the names.... */
  get_view_names.curr_zmap = destroyed_zmaps->parent ;
  get_view_names.curr_element = get_view_names.view_elements = &(view_reply[0]) ;
  g_list_foreach(destroyed_zmaps->children, doViews, &get_view_names) ;

  (*(zmap->zmap_cbs_G->remote_request_func))(ZACP_VIEW_DELETED, &(view_reply[0]),
					     zmap->app_data,
					     handlePeerReply, zmap) ;

  return ;
}






/* 
 *                            Internal routines
 */


/* Handles peer programs reply to requests made from this layer. */
static void handlePeerReply(char *command,
			    RemoteCommandRCType command_rc, char *reason, char *reply,
			    gpointer reply_handler_func_data)
{
  ZMap zmap = (ZMap)reply_handler_func_data ;




  return ;
}


/* THIS FUNCTION COULD CALL control_execute_command...... */
static void localProcessRemoteRequest(ZMap zmap,
				      char *command_name, ZMapAppRemoteViewID view_id, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func,
				      gpointer app_reply_data)
{
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_OK ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  processRequest(zmap, command_name, view_id, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}


/* Commands by this routine need the xml to be parsed for arguments etc. */
static void processRequest(ZMap zmap,
			   char *command_name, ZMapAppRemoteViewID view_id, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* Sample code from Manager....code here needs to be similar.... */


  /* set up return code etc. for parsing code. */
  request_data.command_name = command_name ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers_G, end_handlers_G) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))

    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = g_strdup(zMapXMLParserLastErrorMsg(parser)) ;
    }
  else
    {
      if (strcmp(command_name, ZACP_NEWVIEW) == 0)
	{
	  /* Should be creating a new one so pass in NULL view_id, we have to reset it
	   * because it may have been set to the default. */
	  zMapAppRemoteViewResetID(view_id) ;

          createZMap(manager, view_id,
		     request_data.sequence, request_data.start, request_data.end,
		     command_rc_out, reason_out, reply_out) ;
	}
      else if (strcmp(command_name, ZACP_ADD_TO_VIEW) == 0)
	{
	  if (!zMapAppRemoteViewIsValidID(view_id))
	    {
	      *command_rc_out = REMOTE_COMMAND_RC_BAD_ARGS ;
	      *reason_out = g_strdup("New view is to be added to existing view but no existing view_id specified.") ;
	    }
	  else
	    {
	      /* This call directly sets command_rc etc. */
	      createZMap(manager, view_id,
			 request_data.sequence, request_data.start, request_data.end,
			 command_rc_out, reason_out, reply_out) ;
	    }
	}
      else if (strcmp(command_name, ZACP_CLOSEVIEW) == 0)
	{
	  closeZMap(manager, view_id,
		    command_rc_out, reason_out, reply_out) ;
	}
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



/* GFunc() to record zmap ptr and step through view pointer children. */
static void doViews(gpointer data, gpointer user_data)
{
  ZMapViewWindowTree view = (ZMapViewWindowTree)data ;
  GetViewNames get_view_names = (GetViewNames)user_data ;

  get_view_names->curr_view = view->parent ;
  g_list_foreach(view->children, doWindows, get_view_names) ;

  return ;
}


/* GFunc() to record zmap ptr and step through view pointer children.
 * 
 * We use the local element array to initialise the final view element array
 * via struct copies.
 *  */
static void doWindows(gpointer data, gpointer user_data)
{
  ZMapViewWindowTree window = (ZMapViewWindowTree)data ;
  GetViewNames get_view_names = (GetViewNames)user_data ;

  if (get_view_names->result)
    {
      ZMapXMLUtilsEventStack view_element ;

      if (!(view_element = makeViewElement(get_view_names->curr_zmap, get_view_names->curr_view, window->parent)))
	{
	  get_view_names->result = FALSE ;
	}
      else
	{
	  ZMapXMLUtilsEventStack curr_element ;

	  curr_element = get_view_names->curr_element ;

	  *curr_element = *view_element ;
	  curr_element++ ;
	  view_element++ ;

	  *curr_element = *view_element ;
	  curr_element++ ;
	  view_element++ ;

	  *curr_element = *view_element ;
	  curr_element++ ;
	  view_element++ ;

	  curr_element->event_type = ZMAPXML_NULL_EVENT ;   /* 'null' terminate... */

	  get_view_names->curr_element = curr_element ;
	}
    }


  return ;
}



/* Returns a pointer to a static struct containing the stack to make a view id. If you
 * need to call this multiple times or alter the stack then you need to take a copy
 * of the returned struct as it will be overwritten by subsequent calls. */
static ZMapXMLUtilsEventStack makeViewElement(ZMap zmap, ZMapView view, ZMapWindow window)
{
  ZMapXMLUtilsEventStack element = NULL ;
  static ZMapXMLUtilsEventStackStruct
    view_element[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VIEWID,    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;
  ZMapAppRemoteViewIDStruct remote_view = {NULL} ;
  GQuark view_id ;

  remote_view.zmap = zmap ;
  remote_view.view = view ;
  remote_view.window = window ;

  if (zMapAppRemoteViewCreateID(&remote_view, &view_id))
    {
      element = &(view_element[0]) ;      

      view_element[1].value.q = view_id ;
    }

  return element ;
}










/* 
 * 
 * 
 *             OLD CODE......................
 * 
 * 
 */



void zmapControlRemoteInstaller(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMap zmap = (ZMap)user_data ;

  zMapXRemoteInitialiseWidget(widget, PACKAGE_NAME,
                              ZMAP_DEFAULT_REQUEST_ATOM_NAME,
                              ZMAP_DEFAULT_RESPONSE_ATOM_NAME,
                              control_execute_command, zmap) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}

char *zMapControlRemoteReceiveAccepts(ZMap zmap)
{
  char *xml = NULL;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  xml = zMapXRemoteClientAcceptsActionsXML(zMapXRemoteWidgetGetXID(zmap->toplevel),
                                           &actions_G[ZMAPCONTROL_REMOTE_INVALID + 1],
                                           ZMAPCONTROL_REMOTE_UNKNOWN - 1);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return xml;
}




/* ========================= */
/* ONLY INTERNALS BELOW HERE */
/* ========================= */











#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

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
          xml_reply   = g_string_free(output_data.messages, FALSE);
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
  ViewConnectData view_params = &(input_data->view_params);
  ZMapViewWindow view_window ;
  ZMapView view ;
  char *sequence;
  ZMapFeatureSequenceMap seq_map = g_new0(ZMapFeatureSequenceMapStruct,1);


  if ((sequence = (char *)g_quark_to_string(view_params->sequence)) && view_params->config)
    {
      /* If this happens there has been a major configuration error */
      if (!zmap->default_sequence || !zmap->default_sequence->dataset)
	{
	  output_data->code = ZMAPXREMOTE_INTERNAL;
	  g_string_append_printf(output_data->messages,
                                 "No sequence specified in ZMap config - cannot create view");
	  return;
	}

      seq_map->dataset = zmap->default_sequence->dataset;   /* provide a default FTM */
      seq_map->sequence = sequence;
      seq_map->start = view_params->start;
      seq_map->end = view_params->end;

      if ((view_window = zMapAddView(zmap, seq_map)) && (view = zMapViewGetView(view_window)))
        {
          zMapViewReadConfigBuffer(view, view_params->config);

          if (!(zmapConnectViewConfig(zmap, view, view_params->config)))
            {
	      zmapControlRemoveView(zmap, view, NULL) ;

              output_data->code = ZMAPXREMOTE_UNKNOWNCMD;
              g_string_append_printf(output_data->messages,
                                     "view connection failed.");
            }
          else
            {
              char *xml = NULL ;

              output_data->code = ZMAPXREMOTE_OK ;

              xml = zMapViewRemoteReceiveAccepts(view) ;
              g_string_append(output_data->messages, xml) ;
              g_free(xml) ;
            }
        }
      else
        {
          output_data->code = ZMAPXREMOTE_INTERNAL ;
          g_string_append_printf(output_data->messages, "failed to create view") ;
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


/* Handlers */

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

      for(i = ZMAPCONTROL_REMOTE_INVALID + 1; i < ZMAPCONTROL_REMOTE_UNKNOWN; i++)
        {
          if(action == g_quark_from_string(actions_G[i]))
            parsing_data->common.action = i;
        }

      /* unless((action > INVALID) and (action < UNKNOWN)) */
      if(!(parsing_data->common.action > ZMAPCONTROL_REMOTE_INVALID &&
           parsing_data->common.action < ZMAPCONTROL_REMOTE_UNKNOWN))
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




#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
