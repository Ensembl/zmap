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
 * Last edited: Jul 30 12:09 2007 (rds)
 * Created: Mon Sep 20 10:29:15 2004 (edgrif)
 * CVS info:   $Id: zmapViewUtils.c,v 1.7 2007-07-30 11:23:18 rds Exp $
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


