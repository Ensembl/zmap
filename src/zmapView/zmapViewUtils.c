/*  File: zmapViewUtils.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility functions for a ZMapView.
 *
 * Exported functions: See ZMap/ZMapView.h for public functions and
 *              zmapView_P.h for private functions.
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <zmapView_P.h>


typedef struct
{
  GList *window_list;
} ContextDestroyListStruct, *ContextDestroyList ;


/* Used in stepListFindRequest() to search for a step or a connection. */
typedef struct
{
  ZMapServerReqType request_type ;
  ZMapViewConnectionStep step ;
  ZMapViewConnectionRequest request ;
} StepListFindStruct, *StepListFind ;


/* Used in zmapViewStepListStepRequestDeleteAll(). */
typedef struct
{
  ZMapViewConnectionStepList step_list ;
  ZMapViewConnection connection ;
} StepListDeleteStruct, *StepListDelete ;



typedef struct
{
  GHashTable *featureset_2_column ;
  GList *set_list ;
} GetSetDataStruct, *GetSetData ;



static void cwh_destroy_key(gpointer cwh_data) ;
static void cwh_destroy_value(gpointer cwh_data) ;
static ZMapViewConnectionStep stepListFindStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type) ;
static void stepDestroy(gpointer data, gpointer user_data) ;
static void formatSession(gpointer data, gpointer user_data) ;




/*
 *                  External interface functions.
 */


/* This wierd macro creates a function that will return string literals for each num in the ZMAP_XXXX_LIST's. */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapViewThreadStatus2ExactStr, ZMapViewThreadStatus, ZMAP_VIEW_THREAD_STATE_LIST) ;



gpointer zMapViewFindView(ZMapView view_in, gpointer view_id)
{
  gpointer view = NULL ;

  if (view_in->window_list)
    {
      GList *list_view_window ;

      /* Try to find the view_id in the current zmaps. */
      list_view_window = g_list_first(view_in->window_list) ;
      do
	{
	  ZMapViewWindow next_view_window = (ZMapViewWindow)(list_view_window->data) ;

	  if (next_view_window->window == view_id)
	    {
	      view = next_view_window->parent_view ;

	      break ;
	    }
	}
      while ((list_view_window = g_list_next(list_view_window))) ;
    }

  return view ;
}





void zMapViewGetVisible(ZMapViewWindow view_window, double *top, double *bottom)
{
  zMapReturnIfFail((view_window && top && bottom)) ;

  zMapWindowGetVisible(view_window->window, top, bottom) ;

  return ;
}



ZMapFeatureSource zMapViewGetFeatureSetSource(ZMapView view, GQuark f_id)
{
  ZMapFeatureSource src;

  src = g_hash_table_lookup(view->context_map.source_2_sourcedata,GUINT_TO_POINTER(f_id)) ;

  return src ;
}

void zMapViewSetFeatureSetSource(ZMapView view, GQuark f_id, ZMapFeatureSource src)
{
  g_hash_table_replace(view->context_map.source_2_sourcedata,GUINT_TO_POINTER(f_id), src) ;

  return ;
}



/* Return, as text, details of all data connections for this view. Interface is not completely
 * parameterised in that this function "knows" how to format the reply for its caller.
 */
gboolean zMapViewSessionGetAsText(ZMapViewWindow view_window, GString *session_data_inout)
{
  gboolean result = FALSE ;
  ZMapView view ;

  view = view_window->parent_view ;

  if (view->connection_list)
    {
      g_string_append_printf(session_data_inout, "\tSequence: %s\n\n", view->view_sequence->sequence) ;

      g_list_foreach(view->connection_list, formatSession, session_data_inout) ;

      result = TRUE ;
    }

  return result ;
}




/*
 *            ZMapView external package-level functions.
 */

void zmapViewBusyFull(ZMapView zmap_view, gboolean busy, const char *file, const char *function)
{
  GList* list_item ;

  zmap_view->busy = busy ;

  if ((list_item = g_list_first(zmap_view->window_list)))
    {
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowBusyFull(view_window->window, busy, file, function) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }


  return ;
}


/* Not sure where this is used...check this out.... in checkStateConnections() zmapView.c*/
gboolean zmapAnyConnBusy(GList *connection_list)
{
  gboolean result = FALSE ;
  GList* list_item ;
  ZMapViewConnection view_con ;

  if (connection_list)
    {
      list_item = g_list_first(connection_list) ;

      do
	{
	  view_con = list_item->data ;

	  if (view_con->curr_request == ZMAPTHREAD_REQUEST_EXECUTE)
	    {
	      result = TRUE ;
	      break ;
	    }
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return result ;
}




/*
 *
 * A series of functions for handling the stepping through of sending requests to slave threads.
 *
 */


/* Make the step list. */
ZMapViewConnectionStepList zmapViewStepListCreate(StepListDispatchCB dispatch_func,
						  StepListProcessDataCB process_func,
						  StepListFreeDataCB free_func)
{
  ZMapViewConnectionStepList step_list ;

  step_list = g_new0(ZMapViewConnectionStepListStruct, 1) ;

  step_list->dispatch_func = dispatch_func ;
  step_list->process_func = process_func ;
  step_list->free_func = free_func ;

  return step_list ;
}


/* Add a step to the step list, if it's the first step to be added then the current step will be
 * set to this first step. */
void zmapViewStepListAddStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type,
			     StepListActionOnFailureType on_fail)
{
  ZMapViewConnectionStep step ;
  gboolean first = FALSE ;

  if (!(step_list->steps))
    first = TRUE ;

  step = g_new0(ZMapViewConnectionStepStruct, 1) ;
  step->state = STEPLIST_INVALID ;        // might not be requested
  step->on_fail = on_fail ;
  step->request = request_type ;
  step_list->steps = g_list_append(step_list->steps, step) ;

  if (first)
    step_list->current = step_list->steps ;		    /* Make current the first step. */

  return ;
}


/* Add a connection to the specified step in the step list. */
ZMapViewConnectionRequest zmapViewStepListAddServerReq(ZMapViewConnectionStepList step_list,
						       ZMapViewConnection view_con,
						       ZMapServerReqType request_type,
						       gpointer request_data,
						       StepListActionOnFailureType on_fail)
{
  ZMapViewConnectionRequest request = NULL ;
  ZMapViewConnectionStep step ;

  if ((step = stepListFindStep(step_list, request_type)))
    {
      request = g_new0(ZMapViewConnectionRequestStruct, 1) ;
      step->state = request->state = STEPLIST_PENDING ;           // some duplication here?
      request->request_data = request_data ;
      step->connection_req = request ;

      if(on_fail != 0)
	step->on_fail = on_fail ;
      //printf("add request %d to %s\n",request_type,view_con->url);
    }

  return request ;
}



// create a complete step list. each step will be processed is a request is added
// steps default to STEPLIST_INVALID
ZMapViewConnectionStepList zmapViewConnectionStepListCreate(StepListDispatchCB dispatch_func,
							    StepListProcessDataCB process_func,
							    StepListFreeDataCB free_func)
{
  ZMapViewConnectionStepList step_list;

  step_list = zmapViewStepListCreate(dispatch_func,
				     process_func,
				     free_func) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_CREATE,    REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_OPEN,      REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_GETSERVERINFO, REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_FEATURESETS, REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_STYLES,    REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_NEWCONTEXT,REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_FEATURES,  REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_SEQUENCE,  REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_GETSTATUS,  REQUEST_ONFAIL_CANCEL_THREAD) ;
  zmapViewStepListAddStep(step_list, ZMAP_SERVERREQ_TERMINATE, REQUEST_ONFAIL_CANCEL_THREAD) ;

  return(step_list);

}


/* The meat of the step list stuff, check step list to see where we are and dispatch next
 * step if we are ready for it.
 *
 *  */
void zmapViewStepListIter(ZMapViewConnection view_con)
{
  ZMapViewConnectionStepList step_list = view_con->step_list;
  gboolean result = FALSE;

  if (step_list->current)
    {
      ZMapViewConnectionStep curr_step ;

      curr_step = (ZMapViewConnectionStep)(step_list->current->data) ;

      switch (curr_step->state)
	{
	case STEPLIST_PENDING:
	  {
	    /* All requests are in STEPLIST_PENDING state and after dispatching will go into STEPLIST_DISPATCHED. */

            /* Call users dispatch func. */
	    if ((step_list->dispatch_func))
	      result = step_list->dispatch_func(view_con, curr_step->connection_req->request_data) ;

	    if (result || !(step_list->dispatch_func))
	      {
		zMapThreadRequest(view_con->thread, curr_step->connection_req->request_data) ;

		curr_step->connection_req->state = STEPLIST_DISPATCHED ;
		curr_step->state = STEPLIST_DISPATCHED ;
	      }
	    else
	      {
		curr_step->connection_req->has_failed = TRUE ;
	      }

	    break ;
	  }
	case STEPLIST_DISPATCHED:
	  {
	    if (curr_step->connection_req->state != STEPLIST_DISPATCHED)
	      curr_step->state = STEPLIST_FINISHED ;

	    break ;
	  }
	case STEPLIST_INVALID:  // if a step is not used then skip over it
	case STEPLIST_FINISHED:
	  {
	    /* Move to next step, do not dispatch, that is our callers decision.
	     * If this is the last step then current will be set to NULL. */
	    step_list->current = step_list->current->next ;

	    break ;
	  }
	default:
	  {
	    zMapLogFatalLogicErr("switch(), unknown value: %d", curr_step->state) ;

	    break ;
	  }
	}
    }

  return ;
}



static void stepFindReq(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  StepListFind step_find = (StepListFind)user_data ;

  if (!(step_find->request_type) || step->request == step_find->request_type)
    {
      step_find->step = step ;
      step_find->request = step->connection_req;
    }

  return ;
}

/* Find a connections request in a given request step. */
ZMapViewConnectionRequest zmapViewStepListFindRequest(ZMapViewConnectionStepList step_list,
						      ZMapServerReqType request_type, ZMapViewConnection connection)
{
  ZMapViewConnectionRequest request = NULL ;

  if (step_list)
    {
      StepListFindStruct step_find = {0} ;
      
      step_find.request_type = request_type ;
      step_find.request = NULL ;
      
      g_list_foreach(step_list->steps, stepFindReq, &step_find) ;
      
      request = step_find.request ;
    }
  else
    {
      zMapLogWarning("%s", "Program error: step_list is NULL");
    }

  return request ;
}


/* Process data from request, gets called from main connection loop of the View which itself is
 * called whenever a connection thread returns data.
 *  */
void zmapViewStepListStepProcessRequest(ZMapViewConnection view_con, ZMapViewConnectionRequest request)
{
  gboolean result = TRUE ;
  ZMapViewConnectionStepList step_list = view_con->step_list;

  /* Call users request processing func, if this signals the connection has failed then
   * remove it from the step list. */
  if ((step_list->process_func))
    result = step_list->process_func(view_con, request->request_data) ;

  if (result)
    {
      request->state = STEPLIST_FINISHED ;
    }

  return ;
}


/*                Step list funcs.                             */

/* Free a step. */
static void stepDestroy(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  ZMapViewConnectionStepList step_list = (ZMapViewConnectionStepList)user_data ;

  if(step->connection_req)
    {
      /* Call users free func. */
      if ((step_list->free_func))
	step_list->free_func(step->connection_req->request_data) ;

      g_free(step->connection_req) ;
    }

  step->connection_req = NULL ;

  g_free(step) ;

  return ;
}


/* Free the list of steps. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zmapViewStepListDestroy(ZMapViewConnection view_con)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
void zmapViewStepListDestroy(ZMapViewConnectionStepList step_list)
{
  g_list_foreach(step_list->steps, stepDestroy, step_list) ;
  g_list_free(step_list->steps) ;
  step_list->steps = NULL ;
  g_free(step_list) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  view_con->step_list = NULL;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* THIS IS COMPLETELY AND UTTERLY THE WRONG PLACE TO DO THIS....AGGGGGHHHHHHH...... */

  if(view_con->request_data)
    {
      g_free(view_con->request_data) ;
      view_con->request_data = NULL ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



/* Test to see if there are any more steps to perform, does not look to see if there are requests. */
gboolean zmapViewStepListIsNext(ZMapViewConnectionStepList step_list)
{
  gboolean more = FALSE ;

  if (step_list->current)
    more = TRUE ;

  return more ;
}





/*!
 * Context Window Hash mini-package (CWH)
 *
 * As the FooCanvas isn't an MVC canvas we need to draw a
 * FeatureContext on multiple canvases. This happens in an
 * asynchronous manner which means we can't just destroy the context
 * immediately, if it needs to be freed at all.  To overcome this the
 * Window level calls back the View level after it's finished with the
 * Context (finished drawing it).  The View must then handle
 * destroying the Context, when it's not the view->features context,
 * when all the windows have returned.  To acheive this we have this
 * hash of lists of windows keyed on FeatureContext pointers.
 *
 * zmapViewCWHHashCreate - Creates the hash
 * zmapViewCWHSetList - Sets the list of windows (one time only, no appending!)
 * zmapViewCWHIsLastWindow - tests if a window is the last one (for context)
 * zmapViewCWHRemoveContextWindow - Removes the window and if zmapViewCWHIsLastWindow
 * returns true destroys the FeatureContext.
 * zmapViewCWHDestroy - Destroys the hash
 *
 * zmapViewCWHHashCreate
 * @return               GHashTable * to the new CWH hash.
 *************************************************/
GHashTable *zmapViewCWHHashCreate(void)
{
  GHashTable *hash = NULL;

  hash = g_hash_table_new_full(NULL, NULL, cwh_destroy_key, cwh_destroy_value);

  return hash;
}

/*!
 * zmapViewCWHSetList sets the list of windows, but note this is a one
 * time only set giving no possibility of appending.
 *
 * @param               CWH HashTable *
 * @param               Feature Context
 * @param               GList * of ZMapWindow
 * @return              void
 *************************************************/
void zmapViewCWHSetList(GHashTable *hash, ZMapFeatureContext context, GList *list)
{
  ContextDestroyList destroy_list ;

  destroy_list = g_new0(ContextDestroyListStruct, 1) ;
  destroy_list->window_list = list ;

  g_hash_table_insert(hash, context, destroy_list) ;

  return ;
}

/*!
 * zmapViewCWHIsLastWindow checks whether the window is the last
 * window in the list for the given feature context.
 *
 * @param                CWH HashTable *
 * @param                Feature Context
 * @param                ZMapWindow to look up
 * @return               Boolean as to whether Window is last.
 *************************************************/
gboolean zmapViewCWHIsLastWindow(GHashTable *hash, ZMapFeatureContext context, ZMapWindow window)
{
  ContextDestroyList destroy_list = NULL;
  gboolean last = FALSE;
  int before = 0, after = 0;

  if ((destroy_list = g_hash_table_lookup(hash, context)))
    {
      if (!(destroy_list->window_list))
        {
          zMapLogCritical("%s", "destroy_list->window_list is NULL, cannot check for last window !") ;
        }
      else
        {
          before = g_list_length(destroy_list->window_list);

          /* The _only_ reason we need the struct! */
          destroy_list->window_list = g_list_remove(destroy_list->window_list, window);

          after  = g_list_length(destroy_list->window_list);

          if (before > after)
            {
              if(after == 0)
                last = TRUE;
            }
          else
            {
              zMapLogCritical("%s", "Failed to find window in CWH list!");
            }
        }
    }


  return last;
}

/*!
 * zmapViewCWHRemoveContextWindowRemoves the window and if
 * zmapViewCWHIsLastWindow returns true destroys the FeatureContext.
 *
 * @param                 CWH HashTeble *
 * @param                 FeatureContext in/out as potentially destroyed
 * @param                 ZMapWindow to remove from list
 * @param                 Boolean output set to notify if context is
 *                        the only one in the view i.e. == view->features
 * @return                Boolean to notify whether context was destroyed.
 */
gboolean zmapViewCWHRemoveContextWindow(GHashTable *hash, ZMapFeatureContext *context,
                                        ZMapWindow window, gboolean *is_only_context)
{
  ContextDestroyList destroy_list = NULL;
  gboolean removed = FALSE, is_last = FALSE, absent_context = FALSE;
  int length;

  if((destroy_list = g_hash_table_lookup(hash, *context)) &&
     (is_last = zmapViewCWHIsLastWindow(hash, *context, window)))
    {
      if((length = g_list_length(destroy_list->window_list)) == 0)
        {
          if((removed  = g_hash_table_remove(hash, *context)))
            {
              *context = NULL;
            }
          else
            zMapLogCritical("%s", "Failed to remove Context from hash, despite evidence to contrary.");
        }
      else
        zMapLogCritical("Hash still has a list with length %d", length);
    }
  else if(!destroy_list)
    absent_context = TRUE;

  if(is_only_context)
    *is_only_context = absent_context;

  return removed;
}

/*!
 * zmapViewCWHDestroy - destroys the hash.
 *
 * @param              GHashTable **
 * @return             void
 *************************************************/
void zmapViewCWHDestroy(GHashTable **hash)
{
  GHashTable *cwh_hash = *hash;

  g_hash_table_destroy(cwh_hash);

  *hash = NULL;

  return ;
}




/* 
 * Server Session stuff: holds information about data server connections.
 * 
 * NOTE: the list of connection session data is dynamic, as a server
 * connection is terminated the session data is free'd too so the final
 * list may be very short or not even have any connections at all.
 */

/* Record initial details for a session, these are known when the session is created. */
void zmapViewSessionAddServer(ZMapViewSessionServer server_data, ZMapURL url, char *format)
{
  server_data->scheme = url->scheme ;
  server_data->url = g_strdup(url->url) ;
  server_data->protocol = g_strdup(url->protocol) ;

  if (format)
    server_data->format = g_strdup(format) ;

  switch(url->scheme)
    {
    case SCHEME_ACEDB:
      {
	server_data->scheme_data.acedb.host = g_strdup(url->host) ;
	server_data->scheme_data.acedb.port = url->port ;
	server_data->scheme_data.acedb.database = g_strdup(url->path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	server_data->scheme_data.pipe.path = g_strdup(url->path) ;
	server_data->scheme_data.pipe.query = g_strdup(url->query) ;
	break ;
      }

    case SCHEME_FILE:
      {
	// mgh: file:// is now handled by pipe://, but as this is a view struct it is unchanged
	// consider also DAS, which is still known as a file://
	server_data->scheme_data.file.path = g_strdup(url->path) ;
	break ;
      }
    default:
      {
	/* other schemes not currently supported so mark as invalid. */
	server_data->scheme = SCHEME_INVALID ;
	break ;
      }
    }

  return ;
}

/* Record dynamic details for a session which can only be found out by later interrogation
 * of the server. */
void zmapViewSessionAddServerInfo(ZMapViewSessionServer session_data, ZMapServerReqGetServerInfo session)
{
  if (session->data_format_out)
    session_data->format = g_strdup(session->data_format_out) ;


  switch(session_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	session_data->scheme_data.acedb.database = g_strdup(session->database_path_out) ;
	break ;
      }
    case SCHEME_FILE:
      {

	break ;
      }
    case SCHEME_PIPE:
      {

	break ;
      }
    default:
      {

	break ;
      }
    }

  return ;
}


/* Free all the session data, NOTE: we did not allocate the original struct
 * so we do not free it. */
void zmapViewSessionFreeServer(ZMapViewSessionServer server_data)
{
  g_free(server_data->url) ;
  g_free(server_data->protocol) ;
  g_free(server_data->format) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	g_free(server_data->scheme_data.acedb.host) ;
	g_free(server_data->scheme_data.acedb.database) ;
	break ;
      }
    case SCHEME_FILE:
      {
	g_free(server_data->scheme_data.file.path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	g_free(server_data->scheme_data.pipe.path) ;
	g_free(server_data->scheme_data.pipe.query) ;
	break ;
      }
    default:
      {
	/* no action currently. */
	break ;
      }
    }

  return ;
}





/*
 *               Internal routines.
 */


/* Find a connections request in a given request step. */
static ZMapViewConnectionStep stepListFindStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type)
{
  ZMapViewConnectionStep step = NULL ;
  StepListFindStruct step_find = {0} ;

  step_find.request_type = request_type ;
  step_find.step = NULL ;

  g_list_foreach(step_list->steps, stepFindReq, &step_find) ;

  step = step_find.step ;

  return step ;
}






/*                  hash funcs.                           */


/* g_hash_table key destroy func */
static void cwh_destroy_key(gpointer cwh_data)
{
  ZMapFeatureContext context = (ZMapFeatureContext)cwh_data;

  zMapFeatureContextDestroy(context, TRUE);

  return ;
}


/* g_hash_table value destroy func */
static void cwh_destroy_value(gpointer cwh_data)
{
  ContextDestroyList destroy_list = (ContextDestroyList)cwh_data;

  g_list_free(destroy_list->window_list);
  g_free(destroy_list);

  return ;
}




/* Produce information for each session as formatted text.
 * 
 * NOTE: some information is only available once the server connection
 * is established and the server can be queried for it. This is not formalised
 * in a struct but could be if found necessary.
 * 
 *  */
static void formatSession(gpointer data, gpointer user_data)
{
  ZMapViewConnection view_con = (ZMapViewConnection)data ;
  ZMapViewSessionServer server_data = &(view_con->session) ;
  GString *session_text = (GString *)user_data ;
  char *unavailable_txt = "<< not available yet >>" ;


  g_string_append(session_text, "Server\n") ;

  g_string_append_printf(session_text, "\tURL: %s\n\n", server_data->url) ;
  g_string_append_printf(session_text, "\tProtocol: %s\n\n", server_data->protocol) ;
  g_string_append_printf(session_text, "\tFormat: %s\n\n",
			 (server_data->format ? server_data->format : unavailable_txt)) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	g_string_append_printf(session_text, "\tServer: %s\n\n", server_data->scheme_data.acedb.host) ;
	g_string_append_printf(session_text, "\tPort: %d\n\n", server_data->scheme_data.acedb.port) ;
	g_string_append_printf(session_text, "\tDatabase: %s\n\n",
			       (server_data->scheme_data.acedb.database
				? server_data->scheme_data.acedb.database : unavailable_txt)) ;
	break ;
      }
    case SCHEME_FILE:
      {
	g_string_append_printf(session_text, "\tFile: %s\n\n", server_data->scheme_data.file.path) ;
	break ;
      }
    case SCHEME_PIPE:
      {
	g_string_append_printf(session_text, "\tScript: %s\n\n", server_data->scheme_data.pipe.path) ;
	g_string_append_printf(session_text, "\tQuery: %s\n\n", server_data->scheme_data.pipe.query) ;
	break ;
      }
    default:
      {
	g_string_append(session_text, "\tUnsupported server type !") ;
	break ;
      }
    }

  return ;
}



