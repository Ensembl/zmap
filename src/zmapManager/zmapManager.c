/*  File: zmapManager.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * and was written by
 *              
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *       Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *  Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Handles a list of zmaps which it can delete and add
 *              to.
 *
 * Exported functions: See zmapManager.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapXML.h>
#include <zmapManager_P.h>


#include <zmapApp/zmapApp_P.h>


typedef struct
{
  ZMapManager manager ;

  char *command_name ;
  RemoteCommandRCType command_rc ;
  char *err_msg ;

  GString *message;


  /* only for now....not sure why this is in app_context->info->code */
  int code ;

  GQuark sequence ;
  GQuark start ;
  GQuark end ;
  GQuark config_file ;
  GQuark add_to_view ;

  /* Is this used ?????? */
  GQuark source ;

  GQuark view_id ;

} RequestDataStruct, *RequestData ;




static void addedCB(ZMap zmap, void *cb_data) ;
static void destroyedCB(ZMap zmap, void *cb_data) ;
static void quitReqCB(ZMap zmap, void *cb_data) ;

static void localProcessRemoteRequest(ZMapManager manager,
				      char *command_name, char *request,
				      ZMap zmap, gpointer view_id,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
static void processRequest(ZMapManager app_context,
			   char *command_name, char *request,
			   ZMap zmap, gpointer view_id,
			   RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;

static ZMapXMLUtilsEventStack makeMessageElement(char *message_text) ;
static gboolean start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_start(void *userData, ZMapXMLElement element, ZMapXMLParser parser);
static gboolean req_end(void *userData, ZMapXMLElement element, ZMapXMLParser parser);

static void createZMap(ZMapManager manager, ZMap zmap, gpointer view_id,
		       GQuark sequence_id, int start, int end, GQuark config_file,
		       RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;
static ZMapManagerAddResult addNewView(ZMapManager manager, ZMap zmap_in, ZMapFeatureSequenceMap sequence_map,
				       ZMapView *view_out) ;
static void closeZMap(ZMapManager app, ZMap zmap, gpointer view_id,
		      RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out) ;



/*                  Globals                           */


/* Holds callbacks the level above us has asked to be called back on. */
static ZMapManagerCallbacks manager_cbs_G = NULL ;


/* Holds callbacks we set in the level below us (ZMap window) to be called back on. */
ZMapCallbacksStruct zmap_cbs_G = {addedCB, destroyedCB, quitReqCB, NULL} ;


static ZMapXMLObjTagFunctionsStruct start_handlers_G[] =
  {
    { "zmap", start },
    { "request", req_start },
    { NULL,   NULL  }
  };
static ZMapXMLObjTagFunctionsStruct end_handlers_G[] =
  {
    { "zmap",    end  },
    { "request", req_end  },
    { NULL,   NULL }
  };





/* 
 *                   External routines
 */


/* This routine must be called just once before any other zmaps routine, it is a fatal error
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 *
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of zmaps terminate routine. */
void zMapManagerInit(ZMapManagerCallbacks callbacks)
{
  zMapAssert(!manager_cbs_G) ;

  zMapAssert(callbacks
	     && callbacks->zmap_added_func && callbacks->zmap_deleted_func && callbacks->zmap_set_info_func
	     && callbacks->quit_req_func) ;

  manager_cbs_G = g_new0(ZMapManagerCallbacksStruct, 1) ;

  manager_cbs_G->zmap_added_func = callbacks->zmap_added_func ; /* called when a zmap is added. */
  manager_cbs_G->zmap_deleted_func = callbacks->zmap_deleted_func ; /* called when a zmap closes */
  manager_cbs_G->zmap_set_info_func = callbacks->zmap_set_info_func ;
							    /* called when zmap does something that gui needs to know about (remote calls) */
  manager_cbs_G->quit_req_func = callbacks->quit_req_func ; /* called when layer below requests that zmap app exits. */

  /* Called when a zmap component wants to make a remote request. */
  manager_cbs_G->remote_request_func = callbacks->remote_request_func ;
  manager_cbs_G->remote_request_func_data = callbacks->remote_request_func_data ;

  /* Init zmap remote control callbacks.... */
  zmap_cbs_G.remote_request_func = callbacks->remote_request_func ;
  zmap_cbs_G.remote_request_func_data = callbacks->remote_request_func_data ;

  zMapInit(&zmap_cbs_G) ;

  return ;
}


ZMapManager zMapManagerCreate(void *gui_data)
{
  ZMapManager manager ;

  manager = g_new0(ZMapManagerStruct, 1) ;

  manager->zmap_list = NULL ;

  manager->gui_data = gui_data ;

  if (manager_cbs_G->remote_request_func)
    manager->remote_control = TRUE ;

  return manager ;
}


/* Add a new zmap window with associated thread and all the gubbins.
 * Return indicates what happened on trying to add zmap.
 * 

OOPS...THIS ALL DOESN'T WORK LIKE THAT NOW....

 * If zmap_out is NULL view is displayed in a new window, if an
 * existing  zmap is given then view is added to that zmap.
 * 
 * Policy is:
 *
 * If no connection can be made for the view then the zmap is destroyed,
 * if any connection succeeds then the zmap is added but errors are reported.
 * 
 * sequence_map is assumed to be filled in correctly.
 * 
 * 
 * load_view is a hack that can go once the new view stuff is here...
 * 
 *  */
ZMapManagerAddResult zMapManagerAdd(ZMapManager zmaps, ZMapFeatureSequenceMap sequence_map,
				    ZMap *zmap_inout, ZMapView *view_out, gboolean load_view)
{
  ZMapManagerAddResult result = ZMAPMANAGER_ADD_FAIL ;
  ZMap zmap = *zmap_inout ;
  ZMapView view = NULL ;


  if (!zmap && !(zmap = zMapCreate((void *)(zmaps->gui_data), sequence_map)))
    zMapLogWarning("%s", "Cannot create zmap.") ;

  if (zmap)
    {
      if (!load_view)
	{
	  result = ZMAPMANAGER_ADD_OK ;
	}
      else
	{
	  ZMapViewWindow view_window ;

	  /* Try to load the sequence. */
	  if ((view_window = zMapAddView(zmap, sequence_map)) && zMapConnectView(zmap, zMapViewGetView(view_window)))
	    {
	      result = ZMAPMANAGER_ADD_OK ;
	    }
	  else
	    {
	      /* Remove zmap we just created. */
	      zMapDestroy(zmap, NULL) ;
	      result = ZMAPMANAGER_ADD_FAIL ;
	    }
	}
    }

  if (result == ZMAPMANAGER_ADD_OK)
    {
      zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;
      *zmap_inout = zmap ;

      /* Only fill if there was a sequence and we created the view. */
      if (view)
	*view_out = view ;

      (*(manager_cbs_G->zmap_set_info_func))(zmaps->gui_data, zmap) ;
    }

  return result ;
}



/* Given a remote control type view id find the zmap for it. */
ZMap zMapManagerFindZMap(ZMapManager manager, gpointer view_id)
{
  ZMap zmap = NULL ;

  if (manager->zmap_list)
    {
      GList *list_zmap ;

      /* Try to find the view_id in the current zmaps. */
      list_zmap = g_list_first(manager->zmap_list) ;
      do
	{
	  ZMap next_zmap = (ZMap)(list_zmap->data) ;

	  if ((zMapControlFindView(next_zmap, view_id)))
	    {
	      zmap = next_zmap ;			    /* Found view so return this zmap. */

	      break ;
	    }
	}
      while ((list_zmap = g_list_next(list_zmap))) ;
    }

  return zmap ;
}

/* Given a pointer that might be a view or window, find the
 * view pointer for it. */
gpointer zMapManagerFindView(ZMapManager manager, gpointer view_id)
{
  gpointer view = NULL ;

  if (manager->zmap_list)
    {
      GList *list_zmap ;

      /* Try to find the view_id in the current zmaps. */
      list_zmap = g_list_first(manager->zmap_list) ;
      do
	{
	  ZMap next_zmap = (ZMap)(list_zmap->data) ;
	  gpointer tmp_view ;

	  if ((tmp_view = zMapControlFindView(next_zmap, view_id)))
	    {
	      view = tmp_view ;

	      break ;
	    }
	}
      while ((list_zmap = g_list_next(list_zmap))) ;
    }

  return view ;
}




/* Delete a view from within a zmap. */
void zMapManagerDestroyView(ZMapManager zmaps, ZMap zmap, ZMapView view)
{
  zMapControlCloseView(zmap, view) ;

  return ;
}



guint zMapManagerCount(ZMapManager zmaps)
{
  guint zmaps_count = 0;
  zmaps_count = g_list_length(zmaps->zmap_list);
  return zmaps_count;
}



/* Reset an existing ZMap, this call will:
 *
 *    - leave the ZMap window displayed and hold onto user information such as machine/port
 *      sequence etc so user does not have to add the data again.
 *    - Free all ZMap window data that was specific to the view being loaded.
 *    - Kill the existing server thread and start a new one from scratch.
 *
 * After this call the ZMap will be ready for the user to try again with the existing
 * machine/port/sequence or to enter data for a new sequence etc.
 *
 *  */
gboolean zMapManagerReset(ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapReset(zmap) ;

  return result ;
}


gboolean zMapManagerRaise(ZMap zmap)
{
  gboolean result = TRUE ;

  result = zMapRaise(zmap) ;

  return result ;
}

gboolean zMapManagerProcessRemoteRequest(ZMapManager manager,
					 char *command_name, char *request,
					 ZMap zmap, gpointer view_id,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  gboolean result = FALSE ;


  if (strcmp(command_name, ZACP_NEWVIEW) == 0
      || strcmp(command_name, ZACP_ADD_TO_VIEW) == 0
      || strcmp(command_name, ZACP_CLOSEVIEW) == 0)
    {
      localProcessRemoteRequest(manager,
				command_name, request,
				zmap, view_id,
				app_reply_func, app_reply_data) ;

      result = TRUE ;
    }
  else if (manager->zmap_list)
    {
      GList *next_zmap ;
      gboolean zmap_found = FALSE ;

      /* Look for the right zmap. */
      next_zmap = g_list_first(manager->zmap_list) ;
      do
	{
	  ZMap zmap = (ZMap)(next_zmap->data) ;

	  if (zmap == view_id)
	    {
	      zmap_found = TRUE ;
	      result = zMapControlProcessRemoteRequest(zmap,
						       command_name, request, view_id,
						       app_reply_func, app_reply_data) ;
	      break ;
	    }
	}
      while ((next_zmap = g_list_next(next_zmap))) ;

      /* If we were passed a zmap but did not find it this is an error... */
      if (!zmap_found)
	{
	  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_BAD_ARGS ;
	  char *reason ;
	  ZMapXMLUtilsEventStack reply = NULL ;

	  reason = g_strdup_printf("view id %p not found.", view_id) ;

	  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;

	  result = TRUE ;				    /* Signals we have handled the request. */
	}
    }

  return result ;
}


/* Note the zmap will get removed from managers list once it signals via destroyedCB()
 * that it has died. */
void zMapManagerKill(ZMapManager zmaps, ZMap zmap)
{
  /* TEMP...WE WILL NEED THE VIEW LIST BACK I THINK....OR MAYBE NOT....THEY CAN ONLY CLOSE
   ONE AT A TIME....VIA THE INTERFACE.... */
  zMapDestroy(zmap, NULL) ;

  return ;
}


gboolean zMapManagerKillAllZMaps(ZMapManager zmaps)
{
  if (zmaps->zmap_list)
    {
      GList *next_zmap ;

      next_zmap = g_list_first(zmaps->zmap_list) ;
      do
	{
	  zMapManagerKill(zmaps, (ZMap)(next_zmap->data)) ;
	}
      while ((next_zmap = g_list_next(next_zmap))) ;
    }

  return TRUE;
}



/* Frees zmapmanager, the list of zmaps is expected to already have been
 * free'd. This needs to be done separately because the zmaps are threaded and
 * may take some time to die. */
gboolean zMapManagerDestroy(ZMapManager zmaps)
{
  gboolean result = TRUE ;

  zMapAssert(!(zmaps->zmap_list)) ;

  g_free(zmaps) ;

  return result ;
}




/*
 *                       Internal routines
 */



/* Gets called when a ZMap window is created. */
static void addedCB(ZMap zmap, void *cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  ZMapManager zmaps = app_context->zmap_manager ;

  (*(manager_cbs_G->zmap_added_func))(zmaps->gui_data, zmap) ;

  return ;
}


/* Gets called when a single ZMap window closes from "under our feet" as a result of user interaction,
 * we then make sure we clean up. */
static void destroyedCB(ZMap zmap, void *cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  ZMapManager zmaps = app_context->zmap_manager ;

  zmaps->zmap_list = g_list_remove(zmaps->zmap_list, zmap) ;

  (*(manager_cbs_G->zmap_deleted_func))(zmaps->gui_data, zmap) ;

  return ;
}



/* Gets called when a ZMap requests that the application "quits" as a result of user interaction,
 * we signal the layer above us to start the clean up prior to quitting. */
static void quitReqCB(ZMap zmap, void *cb_data)
{
  ZMapAppContext app_context = (ZMapAppContext)cb_data ;
  ZMapManager zmaps = app_context->zmap_manager ;

  (*(manager_cbs_G->quit_req_func))(zmaps->gui_data, zmap) ;

  return ;
}



/* This is where we process a command. */
static void localProcessRemoteRequest(ZMapManager manager,
				      char *command_name, char *request,
				      ZMap zmap, gpointer view_id,
				      ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data)
{
  RemoteCommandRCType command_rc = REMOTE_COMMAND_RC_FAILED ;
  char *reason = NULL ;
  ZMapXMLUtilsEventStack reply = NULL ;

  processRequest(manager, command_name, request, zmap, view_id, &command_rc, &reason, &reply) ;

  (app_reply_func)(command_name, FALSE, command_rc, reason, reply, app_reply_data) ;
      
  return ;
}



/* Commands by this routine need the xml to be parsed for arguments etc. */
static void processRequest(ZMapManager manager,
			   char *command_name, char *request,
			   ZMap zmap, gpointer view_id,
			   RemoteCommandRCType *command_rc_out, char **reason_out,
			   ZMapXMLUtilsEventStack *reply_out)
{
  ZMapXMLParser parser ;
  gboolean cmd_debug = FALSE ;
  gboolean parse_ok = FALSE ;
  RequestDataStruct request_data = {0} ;


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
	  /* Note, view_id will be null here. */
          createZMap(manager, zmap, view_id,
		     request_data.sequence, request_data.start, request_data.end, request_data.config_file,
		     command_rc_out, reason_out, reply_out) ;
	}
      else if (strcmp(command_name, ZACP_ADD_TO_VIEW) == 0)
	{
	  createZMap(manager, zmap, view_id,
		     request_data.sequence, request_data.start, request_data.end, request_data.config_file,
		     command_rc_out, reason_out, reply_out) ;
	}
      else if (strcmp(command_name, ZACP_CLOSEVIEW) == 0)
	{
	  closeZMap(manager, zmap, view_id,
		    command_rc_out, reason_out, reply_out) ;
	}
    }

  /* Free the parser!!! */
  zMapXMLParserDestroy(parser) ;

  return ;
}



/* Make a new zmap window containing a view of the given sequence from start -> end. */
static void createZMap(ZMapManager manager, ZMap zmap, gpointer view_id,
		       GQuark sequence_id, int start, int end, GQuark config_file,
		       RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out)
{
  char *sequence = NULL ;
  ZMapFeatureSequenceMap sequence_map ;
  static ZMapXMLUtilsEventStackStruct
    view_reply[] =
    {
      {ZMAPXML_START_ELEMENT_EVENT, ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_ATTRIBUTE_EVENT,     ZACP_VIEWID,    ZMAPXML_EVENT_DATA_QUARK, {0}},
      {ZMAPXML_END_ELEMENT_EVENT,   ZACP_VIEW,      ZMAPXML_EVENT_DATA_NONE,  {0}},
      {ZMAPXML_NULL_EVENT}
    } ;


  /* MH17: this is a bodge FTM, we need a dataset XRemote field as well */
  /* default sequence may be NULL */
  sequence_map = g_new0(ZMapFeatureSequenceMapStruct, 1) ;

  sequence_map->config_file = g_strdup(g_quark_to_string(config_file)) ;
  sequence_map->sequence = sequence = g_strdup(g_quark_to_string(sequence_id)) ;
  sequence_map->start = start ;
  sequence_map->end = end ;


  if (sequence_map->sequence && (sequence_map->start < 1 || sequence_map->end < sequence_map->start))
    {
      *reason_out = zMapXMLUtilsEscapeStrPrintf("Bad start/end coords: %d, %d",
						sequence_map->start, sequence_map->end) ;

      zMapWarning("%s", *reason_out) ;
    }
  else
    {
      ZMapManagerAddResult add_result ;
      ZMapView new_view = NULL ;

      add_result = addNewView(manager, zmap, sequence_map, &new_view) ;

      if (add_result == ZMAPMANAGER_ADD_DISASTER)
	{
	  *reason_out = zMapXMLUtilsEscapeStrPrintf("%s",
						    "Failed to create ZMap and then failed to clean up properly,"
						    " save your work and exit now !") ;

	  zMapWarning("%s", *reason_out) ;
	}
      else if (add_result == ZMAPMANAGER_ADD_FAIL)
	{
	  *reason_out = zMapXMLUtilsEscapeStrPrintf("%s", "Failed to create ZMap") ;

	  zMapWarning("%s", *reason_out) ;
	}
      else
	{
	  char *id_str ;

	  /* Make a string version of the view pointer to use as an ID. */
	  zMapAppRemoteViewCreateIDStr(new_view, &id_str) ;

	  view_reply[1].value.q = g_quark_from_string(id_str) ;
	  *reply_out = &view_reply[0] ;

	  g_free(id_str) ;

	  *command_rc_out = REMOTE_COMMAND_RC_OK ;
	}
    }

  return ;
}



static void closeZMap(ZMapManager manager, ZMap zmap, gpointer view_id,
		      RemoteCommandRCType *command_rc_out, char **reason_out, ZMapXMLUtilsEventStack *reply_out)
{
  ZMapView view = view_id ;

  /* Delete the named view, if it's the last view in the zmap then destroy the zmap
   * too. */
  if (zMapNumViews(zmap) > 1)
    zMapManagerDestroyView(manager, zmap, view) ;
  else
    zMapManagerKill(manager, zmap) ;

  *reply_out = makeMessageElement("View deleted.") ;
  *command_rc_out = REMOTE_COMMAND_RC_OK ;

  return ;
}


/* Add a new zmap window with associated thread and all the gubbins.
 * Return indicates what happened on trying to add zmap.
 * 
 * If zmap_inout is NULL view is displayed in a new window, if an
 * existing zmap is given then view is added to that zmap.
 * 
 * Policy is:
 *
 * If no connection can be made for the view then the zmap is destroyed,
 * if any connection succeeds then the zmap is added but errors are reported,
 * if sequence is NULL a blank zmap is created.
 *  */
static ZMapManagerAddResult addNewView(ZMapManager zmaps,
				       ZMap zmap_in,
				       ZMapFeatureSequenceMap sequence_map,
				       ZMapView *view_out)
{
  ZMapManagerAddResult result = ZMAPMANAGER_ADD_FAIL ;
  ZMap zmap ;
  ZMapView view = NULL ;
  ZMapWindow window = NULL ;


  zmap = zmap_in ;

  if (!zmap)
    {
      /* Not adding to existing zmap so make a new one. */
      if (!(zmap = zMapCreate((void *)(zmaps->gui_data), sequence_map)))
	{
	  result = ZMAPMANAGER_ADD_FAIL ;
	  
	  zMapLogWarning("%s", "Cannot create zmap.") ;
	}
    }

  if (zmap)
    {

      /* NOT SURE THIS BIT WORKS PROPERLY NOW.... */
      if (!(zmaps->remote_control))
	{
	  /* No sequence so just add blank zmap. */
	  result = ZMAPMANAGER_ADD_OK ;
	}
      else
	{
	  ZMapViewWindow view_window ;

	  /* Try to load the sequence. */
	  if ((view_window = zMapAddView(zmap, sequence_map))
	      && (view = zMapViewGetView(view_window))
	      && zMapConnectView(zmap, view))
	    {
	      window = zMapViewGetWindow(view_window) ;

	      result = ZMAPMANAGER_ADD_OK ;
	    }
	  else
	    {
	      /* Remove zmap we just created, if this fails then return disaster.... */
	      zMapDestroy(zmap, NULL) ;
	      result = ZMAPMANAGER_ADD_FAIL ;
	    }
	}
    }

  if (result == ZMAPMANAGER_ADD_OK)
    {
      /* Only add zmap to list if we made a new one. */
      if (!zmap_in)
	zmaps->zmap_list = g_list_append(zmaps->zmap_list, zmap) ;

      (*(manager_cbs_G->zmap_set_info_func))(zmaps->gui_data, zmap) ;

      *view_out = view ;
    }

  return result ;
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




/* all the action has been moved from <zmap> to <request> */
static gboolean start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;

  return handled ;
}

static gboolean end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;

  return handled;
}


static gboolean req_start(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled  = FALSE;

  return handled ;
}

static gboolean req_end(void *user_data, ZMapXMLElement element, ZMapXMLParser parser)
{
  gboolean handled = TRUE ;
  RequestData request_data = (RequestData)user_data ;
  ZMapXMLElement child ;
  ZMapXMLAttribute attr ;
  char *err_msg = NULL ;



  if (strcmp(request_data->command_name, ZACP_NEWVIEW) == 0
      || strcmp(request_data->command_name, ZACP_ADD_TO_VIEW) == 0)
    {
      if (strcmp(request_data->command_name, ZACP_ADD_TO_VIEW) == 0)
	{
	  if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(element, ZACP_VIEWID)) != NULL))
	    request_data->add_to_view = zMapXMLAttributeGetValue(attr) ;
	  else
	    err_msg = g_strdup_printf("%s is a required tag.", ZACP_VIEWID) ;
	}

      if (!err_msg && ((child = zMapXMLElementGetChildByName(element, ZACP_SEQUENCE_TAG)) == NULL))
	err_msg = g_strdup_printf("%s is a required element.", ZACP_SEQUENCE_TAG) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_NAME)) != NULL))
	request_data->sequence = zMapXMLAttributeGetValue(attr) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_NAME) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_START)) != NULL))
	request_data->start = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_START) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_END)) != NULL))
	request_data->end = strtol(g_quark_to_string(zMapXMLAttributeGetValue(attr)), (char **)NULL, 10) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_SEQUENCE_END) ;

      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(child, ZACP_SEQUENCE_CONFIG)) != NULL))
	request_data->config_file = zMapXMLAttributeGetValue(attr) ;
    }
  else if (strcmp(request_data->command_name, ZACP_CLOSEVIEW) == 0)
    {
      if (!err_msg && ((attr = zMapXMLElementGetAttributeByName(element, ZACP_VIEWID)) != NULL))
	request_data->view_id = zMapXMLAttributeGetValue(attr) ;
      else
	err_msg = g_strdup_printf("%s is a required tag.", ZACP_VIEWID) ;
    }


  if (err_msg)
    {
      request_data->command_rc = REMOTE_COMMAND_RC_BAD_XML ;

      zMapXMLParserRaiseParsingError(parser, err_msg) ;

      handled = FALSE ;
    }

  return handled ;
}



