/*  File: zmapViewUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Utility functions for a ZMapView.
 *              
 * Exported functions: See ZMap/ZMapView.h for public functions and
 *              zmapView_P.h for private functions.
 * HISTORY:
 * Last edited: Oct  1 15:42 2009 (edgrif)
 * Created: Mon Sep 20 10:29:15 2004 (edgrif)
 * CVS info:   $Id: zmapViewUtils.c,v 1.17 2010-03-04 15:11:38 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapView_P.h>


typedef struct
{
  GList *window_list;
} ContextDestroyListStruct, *ContextDestroyList ;


/* Used in stepListFindRequest() to search for a step or a connection. */
typedef struct
{
  ZMapServerReqType request_type ;
  ZMapViewConnection connection ;
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
  GHashTable *source_2_featureset ;
  GList *set_list ;
} GetSetDataStruct, *GetSetData ;



static void cwh_destroy_key(gpointer cwh_data) ;
static void cwh_destroy_value(gpointer cwh_data) ;

static void stepDispatch(gpointer data, gpointer user_data) ;
static void stepFinished(gpointer data, gpointer user_data) ;
static ZMapViewConnectionStep stepListFindStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type) ;
static void stepFind(gpointer data, gpointer user_data) ;
static void reqFind(gpointer data, gpointer user_data) ;
static void connectionRemove(gpointer data, gpointer user_data) ;
static void stepDestroy(gpointer data, gpointer user_data) ;
static void requestDestroy(gpointer data, gpointer user_data) ;
static void isConnection(gpointer data, gpointer user_data) ;
static void removeFailedConnections(ZMapViewConnectionStepList step_list, ZMapViewConnectionStep step) ;
static GList *findFailedRequests(ZMapViewConnectionStep step) ;
static void findFailed(gpointer data, gpointer user_data) ;
static void removeFailed(gpointer data, gpointer user_data) ;

static void getSetCB(void *data, void *user_data) ;


/* 
 *                  ZMapView interface functions.
 */


void zMapViewGetVisible(ZMapViewWindow view_window, double *top, double *bottom)
{
  zMapAssert(view_window && top && bottom) ;

  zMapWindowGetVisible(view_window->window, top, bottom) ;

  return ;
}



void zmapViewBusy(ZMapView zmap_view, gboolean busy)
{
  GList* list_item ;

  zmap_view->busy = busy ;

  if ((list_item = g_list_first(zmap_view->window_list)))
    {
      do
	{
	  ZMapViewWindow view_window ;

	  view_window = list_item->data ;

	  zMapWindowBusy(view_window->window, busy) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}


ZMapViewSession zMapViewSessionGetData(ZMapViewWindow zmap_view_window)
{
  ZMapViewSession session_data = NULL ;
  ZMapView zmap_view ;

  zMapAssert(zmap_view_window) ;

  zmap_view = zmap_view_window->parent_view ;

  if (zmap_view->state != ZMAPVIEW_DYING && zmap_view->session_data)
    session_data = zmap_view->session_data ;


  return session_data ;
}





/* 
 *            ZMapView internal package functions.
 */


/* Not sure where this is used...check this out.... */
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
  step->state = STEPLIST_PENDING ;
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
						       gpointer request_data)
{
  ZMapViewConnectionRequest request = NULL ;
  ZMapViewConnectionStep step ;

  if ((step = stepListFindStep(step_list, request_type)))
    {
      request = g_new0(ZMapViewConnectionRequestStruct, 1) ;
      request->state = STEPLIST_PENDING ;
      request->step = step ;
      request->connection = view_con ;
      request->request_data = request_data ;
      
      step->connections = g_list_append(step->connections, request) ;
    }


  return request ;
}


/* The meat of the step list stuff, check step list to see where we are and dispatch next
 * step if we are ready for it.
 * 
 *  */
void zmapViewStepListIter(ZMapViewConnectionStepList step_list)
{
  if (step_list->current)
    {
      ZMapViewConnectionStep curr_step ;

      curr_step = (ZMapViewConnectionStep)(step_list->current->data) ;

      switch (curr_step->state)
	{
	case STEPLIST_PENDING:
	  {
	    /* Dispatch all requests in this step. */
	    g_list_foreach(curr_step->connections, stepDispatch, step_list) ;

	    curr_step->state = STEPLIST_DISPATCHED ;

	    /* Check to see if any connections failed and remove them from the steplist if they
	     * did, this might result in there being no connections in the step list.  */
	    removeFailedConnections(step_list, curr_step) ;
	    
	    break ;
	  }
	case STEPLIST_DISPATCHED:
	  {
	    gboolean finished = TRUE ;

	    /* Check all requests, if all finished then change state to finished. */
	    g_list_foreach(curr_step->connections, stepFinished, &finished) ;

	    if (finished)
	      curr_step->state = STEPLIST_FINISHED ;

	    break ;
	  }
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


/* Find a connections request in a given request step. */
ZMapViewConnectionRequest zmapViewStepListFindRequest(ZMapViewConnectionStepList step_list,
						      ZMapServerReqType request_type, ZMapViewConnection connection)
{
  ZMapViewConnectionRequest request = NULL ;
  StepListFindStruct step_find = {0} ;

  step_find.request_type = request_type ;
  step_find.connection = connection ;
  step_find.request = NULL ;

  g_list_foreach(step_list->steps, stepFind, &step_find) ;

  request = step_find.request ;

  return request ;
}


/* Process data from request, gets called from main connection loop of the View which itself is
 * called whenever a connection thread returns data.
 *  */
void zmapViewStepListStepProcessRequest(ZMapViewConnectionStepList step_list, ZMapViewConnectionRequest request)
{
  gboolean result = TRUE ;

  /* Call users request processing func, if this signals the connection has failed then
   * remove it from the step list. */
  if ((step_list->process_func))
    result = step_list->process_func(request->connection, request->request_data) ;

  if (result)
    {
      request->state = STEPLIST_FINISHED ;
    }
  else
    {
      zmapViewStepListStepConnectionDeleteAll(step_list, request->connection) ;
    }

  return ;
}



/* A Connection failed so remove it from the step list. */
void zmapViewStepListStepConnectionDeleteAll(ZMapViewConnectionStepList step_list, ZMapViewConnection connection)
{
  StepListDeleteStruct delete_data = {NULL} ;

  delete_data.step_list = step_list ;
  delete_data.connection = connection ;

  g_list_foreach(step_list->steps, connectionRemove, &delete_data) ;

  return ;
}






/* Test to see if there are any requests in the steps, use this after
 * doing a 'delete all' to check that there is still something to run. */
gboolean zmapViewStepListAreConnections(ZMapViewConnectionStepList step_list)
{
  gboolean connections = FALSE ;

  g_list_foreach(step_list->steps, isConnection, &connections) ;

  return connections ;
}


/* Test to see if there are any more steps to perform, does not look to see if there are requests. */
gboolean zmapViewStepListIsNext(ZMapViewConnectionStepList step_list)
{
  gboolean more = FALSE ;

  if (step_list->current)
    more = TRUE ;

  return more ;
}



/* Free the list of steps. */
void zmapViewStepListDestroy(ZMapViewConnectionStepList step_list)
{
  g_list_foreach(step_list->steps, stepDestroy, step_list) ;
  g_list_free(step_list->steps) ;
  step_list->steps = NULL ;

  g_free(step_list) ;

  return ;
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
  ContextDestroyList destroy_list = NULL; /* Just to fix up the GList hassle. */

  if((destroy_list = g_new0(ContextDestroyListStruct, 1)))
    {
      destroy_list->window_list = list;
      g_hash_table_insert(hash, context, destroy_list);
    }
  else
    zMapAssertNotReached();

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

  if((destroy_list = g_hash_table_lookup(hash, context)))
    {
      zMapAssert( destroy_list->window_list );
      
      before = g_list_length(destroy_list->window_list);

      /* The _only_ reason we need the struct! */
      destroy_list->window_list = g_list_remove(destroy_list->window_list, window);

      after  = g_list_length(destroy_list->window_list);

      if(before > after)
        {
          if(after == 0)
            last = TRUE;
        }
      else
        zMapLogCritical("%s", "Failed to find window in CWH list!");
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



void zmapViewSessionAddServer(ZMapViewSession session_data, ZMapURL url, char *format)
{
  ZMapViewSessionServer server_data ;

  server_data = g_new0(ZMapViewSessionServerStruct, 1) ;

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

  session_data->servers = g_list_append(session_data->servers, server_data) ;

  return ;
}

void zmapViewSessionAddServerInfo(ZMapViewSession session_data, char *database_path)
{
  ZMapViewSessionServer server_data ;

  /* In practice we need to find the correct DB entry.... */
  server_data = (ZMapViewSessionServer)(session_data->servers->data) ;

  switch(server_data->scheme)
    {
    case SCHEME_ACEDB:
      {
	server_data->scheme_data.acedb.database = g_strdup(database_path) ;
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


/* A GFunc to free a session server struct... */
void zmapViewSessionFreeServer(gpointer data, gpointer user_data_unused)
{
  ZMapViewSessionServer server_data = (ZMapViewSessionServer)data ;

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

  g_free(server_data) ;

  return ;
}


/* Set of noddy functions to do stuff to with the source_2_featureset mappings. */

/* Given the name of a source, return its featureset. */
GQuark zmapViewSrc2FSetGetID(GHashTable *source_2_featureset, char *source_name)
{
  GQuark set_id = 0 ;
  GQuark source_id ;
  ZMapGFFSet set_data ;

  source_id = zMapFeatureSetCreateID(source_name) ;

  if ((set_data = g_hash_table_lookup(source_2_featureset, GINT_TO_POINTER(set_id))))
    {
      set_id = set_data->feature_set_id ;
    }

  return set_id ;
}


/* Given a list of source names (as quarks) return a list of featureset names as quarks. */
GList *zmapViewSrc2FSetGetList(GHashTable *source_2_featureset, GList *source_list)
{
  GList *set_list = NULL ;
  GetSetDataStruct cb_data ;

  cb_data.source_2_featureset = source_2_featureset ;
  cb_data.set_list = NULL ;

  g_list_foreach(source_list, getSetCB, &cb_data) ;

  set_list = cb_data.set_list ;

  return set_list ;
}





/* 
 *               Internal routines.
 */



/*                Step list funcs.                             */

/* Free a step. */
static void stepDestroy(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  ZMapViewConnectionStepList step_list = (ZMapViewConnectionStepList)user_data ;

  g_list_foreach(step->connections, requestDestroy, step_list) ;
  g_list_free(step->connections) ;
  step->connections = NULL ;

  g_free(step) ;

  return ;
}


/* Trawls through whole  */
static void removeFailedConnections(ZMapViewConnectionStepList step_list, ZMapViewConnectionStep step)
{
  GList *failed_requests ;

  if ((failed_requests = findFailedRequests(step)))
    {
      g_list_foreach(failed_requests, removeFailed, step_list) ;
    }

  return ;
}


static GList *findFailedRequests(ZMapViewConnectionStep step)
{
  GList *failed_reqs = NULL ;

  g_list_foreach(step->connections, findFailed, &failed_reqs) ;

  return failed_reqs ;
}


static void findFailed(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  GList **failed_list = (GList **)user_data ;

  if (request->has_failed)
    *failed_list = g_list_append(*failed_list, request) ;

  return ;
}


static void removeFailed(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  ZMapViewConnectionStepList step_list = (ZMapViewConnectionStepList)user_data ;

  zmapViewStepListStepConnectionDeleteAll(step_list, request->connection) ;

  return ;
}


static void connectionRemove(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  StepListDelete delete_data = (StepListDelete)user_data ;
  StepListFindStruct step_find = {0} ;

  step_find.connection = delete_data->connection ;

  stepFind(data, &step_find) ;

  if (step_find.request)
    {
      step->connections = g_list_remove(step->connections, step_find.request) ;
      requestDestroy(step_find.request, delete_data->step_list) ;
    }

  return ;
}



/* All requests are in STEPLIST_PENDING state and after dispatching will go into STEPLIST_DISPATCHED. */
static void stepDispatch(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  ZMapViewConnectionStepList step_list = (ZMapViewConnectionStepList)user_data ;
  gboolean result = TRUE ;

  /* Call users dispatch func. */
  if ((step_list->dispatch_func))
    result = step_list->dispatch_func(request->connection, request->request_data) ;

  if (result)
    {
      zMapThreadRequest(request->connection->thread, request->request_data) ;

      request->state = STEPLIST_DISPATCHED ;
    }
  else
    {
      request->has_failed = TRUE ;
    }

  return ;
}


/* Returns whether a request has finished. */
static void stepFinished(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  gboolean *finished = (gboolean *)user_data ;

  if (request->state == STEPLIST_DISPATCHED)
    *finished = FALSE ;

  return ;
}


/* Find a connections request in a given request step. */
static ZMapViewConnectionStep stepListFindStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type)
{
  ZMapViewConnectionStep step = NULL ;
  StepListFindStruct step_find = {0} ;

  step_find.request_type = request_type ;
  step_find.step = NULL ;

  g_list_foreach(step_list->steps, stepFind, &step_find) ;

  step = step_find.step ;

  return step ;
}

static void stepFind(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  StepListFind step_find = (StepListFind)user_data ;

  if (!(step_find->request_type) || step->request == step_find->request_type)
    {
      /* If there is a connection then go on and find it otherwise just return the step. */
      if (step_find->connection)
	g_list_foreach(step->connections, reqFind, user_data) ;
      else
	step_find->step = step ;
    }

  return ;
}

static void reqFind(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  StepListFind step_find = (StepListFind)user_data ;

  if (request->connection == step_find->connection)
    step_find->request = request ;

  return ;
}


static void isConnection(gpointer data, gpointer user_data)
{
  ZMapViewConnectionStep step = (ZMapViewConnectionStep)data ;
  gboolean *connections = (gboolean *)user_data ;

  if (step->connections)
    *connections = TRUE ;

  return ;
}



/* Free a request within a step.
 * We do not free the view connections, they are taken care of elsewhere
 * in the view code. */
static void requestDestroy(gpointer data, gpointer user_data)
{
  ZMapViewConnectionRequest request = (ZMapViewConnectionRequest)data ;
  ZMapViewConnectionStepList step_list = (ZMapViewConnectionStepList)user_data ;

  /* Call users free func. */
  if ((step_list->free_func))
    step_list->free_func(request->connection, request->request_data) ;

  g_free(request) ;

  return ;
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




static void getSetCB(void *data, void *user_data)
{
  GQuark source_id = GPOINTER_TO_INT(data) ;
  GetSetData set_data_cb = (GetSetData)user_data ;
  GList *set_list = set_data_cb->set_list ;
  ZMapGFFSet set_data ;
 

  if ((set_data = g_hash_table_lookup(set_data_cb->source_2_featureset, GINT_TO_POINTER(source_id))))
    {
      GQuark set_id ;

      set_id = set_data->feature_set_id ;
      set_list = g_list_append(set_list, GINT_TO_POINTER(set_id)) ;

      set_data_cb->set_list = set_list ;
    }

  return ;
}

