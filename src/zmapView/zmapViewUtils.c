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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Utility functions for a ZMapView.
 * StepLists changed to be the child of a connection by Malcolm Hinsley (mh17@sanger.ac.uk) 11 Mar 2010
 *
 * Exported functions: See ZMap/ZMapView.h for public functions and
 *              zmapView_P.h for private functions.
 * HISTORY:
 * Last edited: Mar 11 13:27 2010 (edgrif)
 * Created: Mon Sep 20 10:29:15 2004 (edgrif)
 * CVS info:   $Id: zmapViewUtils.c,v 1.24 2010-10-13 09:00:38 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <glib.h>
#include <ZMap/zmapUtils.h>
//#include <ZMap/zmapGFF.h>
#include <zmapView_P.h>


typedef struct
{
  GList *window_list;
} ContextDestroyListStruct, *ContextDestroyList ;


/* Used in stepListFindRequest() to search for a step or a connection. */
typedef struct
{
  ZMapServerReqType request_type ;
//  ZMapViewConnection connection ;
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



/*
 *                  ZMapView interface functions.
 */


void zMapViewGetVisible(ZMapViewWindow view_window, double *top, double *bottom)
{
  zMapAssert(view_window && top && bottom) ;

  zMapWindowGetVisible(view_window->window, top, bottom) ;

  return ;
}



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
						       gpointer request_data)
{
  ZMapViewConnectionRequest request = NULL ;
  ZMapViewConnectionStep step ;

  if ((step = stepListFindStep(step_list, request_type)))
    {
      request = g_new0(ZMapViewConnectionRequestStruct, 1) ;
      step->state = request->state = STEPLIST_PENDING ;           // some duplication here?
      request->request_data = request_data ;
      step->connection_req = request ;
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

          if (result)
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
  StepListFindStruct step_find = {0} ;

  step_find.request_type = request_type ;
  step_find.request = NULL ;

  g_list_foreach(step_list->steps, stepFindReq, &step_find) ;

  request = step_find.request ;

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
void zmapViewStepListDestroy(ZMapViewConnection view_con)
{
  ZMapViewConnectionStepList step_list = view_con->step_list;

  if(step_list)
  {
    g_list_foreach(step_list->steps, stepDestroy, step_list) ;
    g_list_free(step_list->steps) ;
    step_list->steps = NULL ;

    g_free(step_list) ;
  }
  view_con->step_list = NULL;

  if(view_con->request_data)
  {
    g_free(view_con->request_data) ;
    view_con->request_data = NULL ;
  }

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



