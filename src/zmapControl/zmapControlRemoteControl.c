/*  File: zmapControlRemoteControl.c
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
#include <ZMap/zmapRemoteCommand.h>
#include <zmapControl_P.h>



enum
  {
    ZMAPCONTROL_REMOTE_INVALID,
    /* Add below here... */

    /* No commands currently. */


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



/* Used to build xml giving the list of deleted views. */
typedef struct GetViewNamesStructName
{
  gboolean result ;

  GList *windows ;

  ZMapXMLUtilsEventStack view_elements ;
  ZMapXMLUtilsEventStack curr_element ;
} GetViewNamesStruct, *GetViewNames ;





#ifdef UNUSED
static void localProcessRemoteRequest(ZMap zmap,
				      char *command_name, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void processRequest(ZMap zmap,
			   char *command_name, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out) ;
#endif

static void handlePeerReply(gboolean reply_ok, char *reply_error,
			    char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
			    gpointer reply_handler_func_data) ;

static void doViews(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void doWindows(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static ZMapXMLUtilsEventStack makeViewElement(ZMapView view) ;




/* ZMAPXREMOTE_CALLBACK */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
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
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* 
 *                  Globals.
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean control_execute_debug_G = FALSE;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */









/* 
 *                 External routines.
 */



/* Receives peer program requests from layer above and either handles them or forwards them
 * to ZMapView. */
gboolean zMapControlProcessRemoteRequest(ZMap zmap,
					 char *command_name, char *request, gpointer view_id,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* CURRENTLY THERE ARE NO CONTROL LEVEL COMMANDS, ADD THEM LIKE THIS WHEN
   * WE NEED THEM...JUST REMOVE THE UNDEF'S AND JOIN UP THE  else if (zmap->view_list) */

  if (strcmp(command_name, ZACP_NEWVIEW) == 0
      || strcmp(command_name, ZACP_CLOSEVIEW) == 0)
    {
      localProcessRemoteRequest(zmap, command_name, request,
				app_reply_func, app_reply_data) ;

      result = TRUE ;
    }
  else
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (zmap->view_list)
    {
      char *id_str = NULL ;
      char *err_msg = NULL ;
      ZMapViewWindow view_window = NULL ;
      ZMapView view = NULL ;
      ZMapView request_view = NULL ;
      gboolean parse_view_id = TRUE ;
      gboolean view_found = TRUE ;


      /* Get the view from the request, if no view specified then use focus_viewwindow. */
      if (zMapRemoteCommandGetAttribute(request,
					ZACP_REQUEST, ZACP_VIEWID, &(id_str),
					&err_msg))
	
	{
	  ZMapView request_view = NULL ;

	  /* Parse the view and check that it exists in our list of views. */
	  if ((parse_view_id = zMapAppRemoteViewParseIDStr(id_str, (void **)&request_view)))
	    {
	      GList *list_ptr ;

	      /* Find the view in our list of views. */
	      view_found = FALSE ;
	      list_ptr = g_list_first(zmap->view_list) ;
	      do
		{
		  ZMapView next_view = (ZMapView)(list_ptr->data) ;

		  if (next_view == request_view)
		    {
		      view = request_view ;
		      view_found = TRUE ;

		      break ;
		    }
		}
	      while ((list_ptr = g_list_next(list_ptr))) ;
	    }
	}
      else
	{
	  /* No view specified in request so use the focus view or if there isn't one yet
	   * then use the first view in the list....a bit hacky... */
	  if (zmap->focus_viewwindow)
	    {
	      view = zMapViewGetView(zmap->focus_viewwindow) ;

	      view_window = zmap->focus_viewwindow ;
	    }
	  else
	    {
	      /* This is a bit hacky, if there is no focus view/window we default to the first one.
	       * .....do some logging. */
	      view = (ZMapView)(zmap->view_list->data) ; /* default to first view. */

	      zMapLogWarning("%s",
			     "Received remote view command with no view specified,"
			     " there is no focus view so defaulted to first view in list.") ;
	    }
	}



      /* Find the view_window for the request or the default view .... */
      if (view && !view_window)
	{
	  if ((view == zMapViewGetView(zmap->focus_viewwindow)))
	    {
	      /* view is the focus view. */
	      view_window = zmap->focus_viewwindow ;
	    }
	  else
	    {
	      /* If the view is not the focus_viewwindow we get the first view_window
	       * of the view, feels hacky and needs revising..... */
	      view_window = zMapViewGetDefaultViewWindow(view) ;
	    }
	}




      /* If we have a view then we can carry on and call the view request processor. */
      if (view_window)
	{

	  /* Found a view, send the command on to it. */
	  result = zMapViewProcessRemoteRequest(view_window, command_name, request,
						app_reply_func, app_reply_data) ;
	}
      else
	{
	  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  char *reason ;
	  ZMapXMLUtilsEventStack reply = NULL ;

	  /* Something went wrong. */
	  if (!parse_view_id)
	    {
	      reason = "Badly formed view_id !" ;
	      result = FALSE ;
	    }
	  else						    /* !view_found */
	    {
	      reason = g_strdup_printf("view_id %p not found.", request_view) ;
	      result = TRUE ;				    /* Signals we have handled the request. */
	    }

	  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;
	}

    }

  return result ;
}




/* 
 *                            Package routines
 */


/* User interaction has created a new view/window (by splitting an existing one). */
void zmapControlSendViewCreated(ZMap zmap, ZMapView view, ZMapWindow window)
{
  ZMapXMLUtilsEventStack view_element ;

  view_element = makeViewElement(view) ;

  /* Not sure if this should be view or what....probably should pass view.... */
  (*(zmap->zmap_cbs_G->remote_request_func))(zmap->app_data,
					     view,
					     ZACP_VIEW_CREATED, view_element,
					     handlePeerReply, zmap) ;

  return ;
}


/* User interaction has resulted in a zmap toplevel window being deleted,
 * so we need to report which views were destroyed. */
void zmapControlSendViewDeleted(ZMap zmap, GList *destroyed_views)
{
  GetViewNamesStruct get_view_names = {TRUE, NULL} ;
  static ZMapXMLUtilsEventStackStruct			    /* Should dynamically allocate... */
    view_reply[20] =
    {
      {ZMAPXML_NULL_EVENT}
    } ;


  /* collect all the views that have been destroyed. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  get_view_names.curr_zmap = destroyed_zmaps->parent ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  get_view_names.curr_element = get_view_names.view_elements = &(view_reply[0]) ;
  g_list_foreach(destroyed_views, doViews, &get_view_names) ;


  (*(zmap->zmap_cbs_G->remote_request_func))(zmap->app_data,
					     NULL,
					     ZACP_VIEW_DELETED, &(view_reply[0]),
					     handlePeerReply, zmap) ;

  return ;
}






/* 
 *                            Internal routines
 */


/* Handles peer programs reply to requests made from this layer. */
static void handlePeerReply(gboolean reply_ok, char *reply_error,
			    char *command, RemoteCommandRCType command_rc, char *reason, char *reply,
			    gpointer reply_handler_func_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMap zmap = (ZMap)reply_handler_func_data ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (!reply_ok)
    {
      zMapLogCritical("Bad reply from peer: \"%s\"", reply_error) ;
    }

  return ;
}


#ifdef UNUSED
/* CURRENTLY UNUSED AS WE DO NOT HAVE CONTROL LEVEL COMMANDS, I'M LEAVING THE CODE
 * CODE HERE BECAUSE IT'S VERY LIKELY WE WILL ADD COMMANDS. */
static void localProcessRemoteRequest(ZMap zmap,
				      char *command_name, char *request,
				      ZMapRemoteAppReturnReplyFunc app_reply_func,
				      gpointer app_reply_data)
{
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_OK ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;


  processRequest(zmap, command_name, request, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

  return ;
}
#endif


#ifdef UNUSED
/* CURRENTLY UNUSED AS WE DO NOT HAVE CONTROL LEVEL COMMANDS, I'M LEAVING THE CODE
 * CODE HERE BECAUSE IT'S VERY LIKELY WE WILL ADD COMMANDS. */
static void processRequest(ZMap zmap,
			   char *command_name, char *request,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{

  /* DISABLED CODE....DUH.......MOVED TO MANAGER ???? */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


  /* Sample code from Manager....code here needs to be similar.... */


  /* set up return code etc. for parsing code. */
  request_data.command_name = command_name ;
  request_data.command_rc = REMOTE_COMMAND_RC_OK ;

  parser = zMapXMLParserCreate(&request_data, FALSE, cmd_debug) ;

  zMapXMLParserSetMarkupObjectTagHandlers(parser, start_handlers_G, end_handlers_G) ;

  if (!(parse_ok = zMapXMLParserParseBuffer(parser, request, strlen(request))))

    {
      *command_rc_out = request_data.command_rc ;
      *reason_out = zMapXMLUtilsEscapeStr(zMapXMLParserLastErrorMsg(parser)) ;
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
	      *reason_out = zMapXMLUtilsEscapeStr("New view is to be added to existing view"
						  "but no existing view_id specified.") ;
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
#endif


/* GFunc() to make an xml view element each time it is called.
 * 
 * We use the local element array to initialise the final view element array
 * via struct copies.
 *  */
static void doViews(gpointer data, gpointer user_data)
{
  ZMapView view = (ZMapView)data ;
  GetViewNames get_view_names = (GetViewNames)user_data ;

  if (get_view_names->result)
    {
      ZMapXMLUtilsEventStack view_element ;

      if (!(view_element = makeViewElement(view)))
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
static ZMapXMLUtilsEventStack makeViewElement(ZMapView view)
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
  GQuark view_id ;

  if (zMapAppRemoteViewCreateID(view, &view_id))
    {
      element = &(view_element[0]) ;      

      view_element[1].value.q = view_id ;
    }

  return element ;
}


