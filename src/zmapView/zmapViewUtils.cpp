/*  File: zmapViewUtils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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

#include <ZMap/zmap.hpp>

#include <glib.h>

#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <zmapView_P.hpp>


/* preferences strings. */
#define VIEW_CHAPTER "Features"
#define VIEW_PAGE "Display"
#define VIEW_FILTERED "Highlight filtered columns"


typedef struct
{
  GList *window_list;
} ContextDestroyListStruct, *ContextDestroyList ;





typedef struct
{
  GHashTable *featureset_2_column ;
  GList *set_list ;
} GetSetDataStruct, *GetSetData ;



typedef struct ForAllDataStructType
{
  ZMapViewForAllCallbackFunc user_func_cb ;
  void *user_func_data ;
} ForAllDataStruct, *ForAllData ;







static void cwh_destroy_key(gpointer cwh_data) ;
static void cwh_destroy_value(gpointer cwh_data) ;

static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent, ZMapView view) ;
static void readChapter(ZMapGuiNotebookChapter chapter, ZMapView view) ;
static void applyCB(ZMapGuiNotebookAny any_section, void *user_data) ;
static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused) ;

static void forAllCB(void *data, void *user_data) ;

static void setCursorCB(ZMapWindow window, void *user_data) ;
static void toggleDisplayCoordinatesCB(ZMapWindow, void* user_data) ;

static void formatSession(gpointer data, gpointer user_data) ;

static void invoke_merge_in_names(gpointer list_data, gpointer user_data) ;


/*
 *                  External interface functions.
 */


/* This wierd macro creates a function that will return string literals for each num in the ZMAP_XXXX_LIST's. */
ZMAP_ENUM_AS_EXACT_STRING_FUNC(zMapViewThreadStatus2ExactStr, ZMapViewThreadStatus, ZMAP_VIEW_THREAD_STATE_LIST) ;



//  Some context functions....
//


// Simply cover function for internal version.
ZMapFeatureContext zMapViewCreateContext(ZMapView view, GList *feature_set_names, ZMapFeatureSet feature_set)
{
  ZMapFeatureContext context = NULL ;

  context = zmapViewCreateContext(view, feature_set_names, feature_set) ;

  return context ;
}



// Do a merge and draw of a new context into an existing one in view.
ZMapFeatureContextMergeCode zMapViewContextMerge(ZMapView view, ZMapFeatureContext new_context)
{
  ZMapFeatureContextMergeCode result = ZMAPFEATURE_CONTEXT_ERROR ;
  ZMapFeatureContextMergeStats merge_stats = NULL ;
  bool acedb_source = false ;
  GList *masked = NULL ;


  result = zmapJustMergeContext(view,
                                &new_context, &merge_stats,
                                &masked, acedb_source, TRUE) ;

  // do something with merge stats ?
  g_free(merge_stats) ;


  if (result == ZMAPFEATURE_CONTEXT_OK)
    {
      // why no return code ??
      zmapJustDrawContext(view, new_context, masked, NULL, NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      result = TRUE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return result ;
}



// Some utilities for views, visibility etc.
// 


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


void zMapViewSetFlag(ZMapView view, ZMapFlag flag, const gboolean value)
{
  zMapReturnIfFail(view && view->view_sequence && flag >= 0 && flag < ZMAPFLAG_NUM_FLAGS) ;

  view->view_sequence->setFlag(flag, value) ;
  
  /* Perform any updates required for the particular flag that was set */
  switch (flag)
    {
    case ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS:
      zMapViewUpdateColumnBackground(view);
      break ;

    case ZMAPFLAG_ENABLE_ANNOTATION:
      zMapViewToggleScratchColumn(view, value, TRUE) ;
      break ;

    default:
      break ;
    } ;
}

gboolean zMapViewGetFlag(ZMapView view, ZMapFlag flag)
{
  gboolean result = FALSE ;
  zMapReturnValIfFail(view && view->view_sequence && flag >= 0 && flag < ZMAPFLAG_NUM_FLAGS, result) ;

  result = view->view_sequence->getFlag(flag) ;

  return result ;
}



/*!
 * \briefReturns a ZMapGuiNotebookChapter containing all general zmap preferences.
 */
ZMapGuiNotebookChapter zMapViewGetPrefsChapter(ZMapView view, ZMapGuiNotebook note_book_parent)
{
  ZMapGuiNotebookChapter chapter = NULL ;

  chapter = makeChapter(note_book_parent, view) ;

  return chapter ;
}


/* Execute some user function on all child "view windows". Note that the user callback gets called
 * with a ZMapViewWindow not a ZMapWindow. */
void zMapViewForAllZMapWindows(ZMapView view, ZMapViewForAllCallbackFunc user_func_cb, void *user_func_data)
{
  ForAllDataStruct all_data = {user_func_cb, user_func_data} ;

  g_list_foreach(view->window_list, forAllCB, &all_data) ;

  return ;
}


void zMapViewToggleDisplayCoordinates(ZMapView view)
{
  void *user_data = view->features ;
  zMapViewForAllZMapWindows(view, toggleDisplayCoordinatesCB, user_data ) ;
  zMapWindowNavigatorReset(view->navigator_window) ;
  zMapWindowNavigatorDrawFeatures(view->navigator_window, view->features, view->context_map.styles);

}


/* Set the given cursor on all child windows. */
void zMapViewSetCursor(ZMapView view, GdkCursor *cursor)
{

  /* zmapView has no widgets of it's own and so is expected to set cursors
   * on it's all children. */
  zMapViewForAllZMapWindows(view, setCursorCB, cursor) ;


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

	  view_window = (ZMapViewWindow)(list_item->data) ;

	  zMapWindowBusyFull(view_window->window, busy, file, function) ;
	}
      while ((list_item = g_list_next(list_item))) ;
    }


  return ;
}

/* Return, as text, details of all data connections for this view. Interface is not completely
 * parameterised in that this function "knows" how to format the reply for its caller.
 */
gboolean zMapViewSessionGetAsText(ZMapViewWindow view_window, GString *session_str_inout)
{
  gboolean result = FALSE ;
  ZMapView view ;

  view = view_window->parent_view ;

  if (view->connection_list)
    {
      g_string_append_printf(session_str_inout, "\tSequence: %s\n\n", view->view_sequence->sequence) ;

      g_list_foreach(view->connection_list, formatSession, session_str_inout) ;

      result = TRUE ;
    }

  return result ;
}




/*
 *            ZMapView external package-level functions.
 */


void zmapViewMergeColNames(ZMapView view, GList *names)
{
  g_list_foreach(view->window_list, invoke_merge_in_names, names) ;

  return ;
}





/* Not sure where this is used...check this out.... in checkStateConnections() zmapView.c*/
gboolean zmapAnyConnBusy(GList *connection_list)
{
  gboolean result = FALSE ;
  GList* list_item ;
  ZMapNewDataSource view_con ;

  if (connection_list)
    {
      list_item = g_list_first(connection_list) ;

      do
	{
	  view_con = (ZMapNewDataSource)(list_item->data) ;

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
  GHashTable *ghash = NULL;

  ghash = g_hash_table_new_full(NULL, NULL, cwh_destroy_key, cwh_destroy_value);

  return ghash;
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
void zmapViewCWHSetList(GHashTable *ghash, ZMapFeatureContext context, GList *glist)
{
  ContextDestroyList destroy_list ;

  destroy_list = g_new0(ContextDestroyListStruct, 1) ;
  destroy_list->window_list = glist ;

  g_hash_table_insert(ghash, context, destroy_list) ;

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
gboolean zmapViewCWHIsLastWindow(GHashTable *ghash, ZMapFeatureContext context, ZMapWindow window)
{
  ContextDestroyList destroy_list = NULL;
  gboolean last = FALSE;
  int before = 0, after = 0;

  if ((destroy_list = (ContextDestroyList)(g_hash_table_lookup(ghash, context))))
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
gboolean zmapViewCWHRemoveContextWindow(GHashTable *ghash, ZMapFeatureContext *context,
                                        ZMapWindow window, gboolean *is_only_context)
{
  ContextDestroyList destroy_list = NULL;
  gboolean removed = FALSE, is_last = FALSE, absent_context = FALSE;
  int length;

  if((destroy_list = (ContextDestroyList)g_hash_table_lookup(ghash, *context)) &&
     (is_last = zmapViewCWHIsLastWindow(ghash, *context, window)))
    {
      if((length = g_list_length(destroy_list->window_list)) == 0)
        {
          if((removed  = g_hash_table_remove(ghash, *context)))
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
void zmapViewCWHDestroy(GHashTable **ghash)
{
  GHashTable *cwh_hash = *ghash;

  g_hash_table_destroy(cwh_hash);

  *ghash = NULL;

  return ;
}





/*
 *               Internal routines.
 */




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





/*!
 * \brief Does the work to create a chapter in the preferences notebook for general zmap settings
 */
static ZMapGuiNotebookChapter makeChapter(ZMapGuiNotebook note_book_parent, ZMapView view)
{
  ZMapGuiNotebookChapter chapter = NULL ;
  ZMapGuiNotebookCBStruct user_CBs = {cancelCB, NULL, applyCB, view, NULL, NULL, NULL, NULL} ;
  ZMapGuiNotebookPage page ;
  ZMapGuiNotebookSubsection subsection ;
  ZMapGuiNotebookParagraph paragraph ;
  ZMapGuiNotebookTagValue tagvalue ;


  chapter = zMapGUINotebookCreateChapter(note_book_parent, VIEW_CHAPTER, &user_CBs) ;


  page = zMapGUINotebookCreatePage(chapter, VIEW_PAGE) ;

  subsection = zMapGUINotebookCreateSubsection(page, NULL) ;

  paragraph = zMapGUINotebookCreateParagraph(subsection, NULL,
					     ZMAPGUI_NOTEBOOK_PARAGRAPH_TAGVALUE_TABLE,
					     NULL, NULL) ;

  tagvalue = zMapGUINotebookCreateTagValue(paragraph, VIEW_FILTERED,
                                           NULL,
					   ZMAPGUI_NOTEBOOK_TAGVALUE_CHECKBOX,
					   "bool", zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS)) ;

  return chapter ;
}


static void readChapter(ZMapGuiNotebookChapter chapter, ZMapView view)
{
  ZMapGuiNotebookPage page ;
  gboolean bool_value = FALSE ;

  if ((page = zMapGUINotebookFindPage(chapter, VIEW_PAGE)))
    {
      if (zMapGUINotebookGetTagValue(page, VIEW_FILTERED, "bool", &bool_value))
	{
	  if (zMapViewGetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS) != bool_value)
	    {
              zMapViewSetFlag(view, ZMAPFLAG_HIGHLIGHT_FILTERED_COLUMNS, bool_value) ;
              zMapViewUpdateColumnBackground(view);
	    }
	}
    }

  return ;
}


static void applyCB(ZMapGuiNotebookAny any_section, void *user_data)
{
  ZMapView view = (ZMapView)user_data;

  readChapter((ZMapGuiNotebookChapter)any_section, view) ;

  return ;
}


static void cancelCB(ZMapGuiNotebookAny any_section, void *user_data_unused)
{
  return ;
}



static void forAllCB(void *data, void *user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)data ;
  ForAllData all_data = (ForAllData)user_data ;

  (all_data->user_func_cb)(view_window->window, all_data->user_func_data) ;

  return ;
}

static void setCursorCB(ZMapWindow window, void *user_data)
{
  GdkCursor *cursor = (GdkCursor *)user_data ;

  zMapWindowSetCursor(window, cursor) ;

  return ;
}



static void toggleDisplayCoordinatesCB(ZMapWindow window, void* user_data)
{
  if (window)
    {
      zMapWindowToggleDisplayCoordinates(window) ;
    }
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
  ZMapNewDataSource view_con = (ZMapNewDataSource)data ;
  ZMapViewSessionServer server_data = view_con->server ;
  GString *session_text = (GString *)user_data ;

  // Get session info as a string....
  zMapServerFormatSession(server_data, session_text) ;

  return ;
}




/* This is called on each view_window in a view to merge the given list of featureset names into
 * the window's list of feature_set_names */
static void invoke_merge_in_names(gpointer list_data, gpointer user_data)
{
  ZMapViewWindow view_window = (ZMapViewWindow)list_data;
  GList *feature_set_names = (GList *)user_data;

  /* This relies on hokey code... with a number of issues...
   * 1) the window function only concats the lists.
   * 2) this view code might well want to do the merge?
   * 3) how do we order all these columns?
   */
   /* mh17: column ordering is as in ZMap config
    * either by 'columns' command or by server and then featureset order
    * concat is ok as featuresets are not duplicated, but beware repeat requests to the same server
    * NOTE (mar 2011) with req from mark we expect duplicate requests, merge must merge not concatenate
    */
  zMapWindowMergeInFeatureSetNames(view_window->window, feature_set_names);

  return ;
}



