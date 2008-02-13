/*  File: zmapViewUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Feb  8 15:36 2008 (edgrif)
 * Created: Mon Sep 20 10:29:15 2004 (edgrif)
 * CVS info:   $Id: zmapViewUtils.c,v 1.8 2008-02-13 16:52:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapView_P.h>

typedef struct
{
  GList *window_list;
}ContextDestroyListStruct, *ContextDestroyList;

static void cwh_destroy_key(gpointer cwh_data);
static void cwh_destroy_value(gpointer cwh_data);


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



void zmapViewSessionAddServer(ZMapViewSession session_data, zMapURL url, char *format)
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
    case SCHEME_FILE:
      {
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
 *                       Internal routines.
 */






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


