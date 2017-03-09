/*  File: zmapControl.c
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
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: This is the ZMap interface code, it controls both
 *              the window code and the threaded server code.
 * Exported functions: See ZMap.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <stdio.h>
#include <gtk/gtk.h>

#include <ZMap/zmapView.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsGUI.hpp>
#include <ZMap/zmapUrlUtils.hpp>
#include <zmapControl_P.hpp>



typedef struct ForAllDataStructType
{
  ZMapControlForAllCallbackFunc user_func_cb ;
  void *user_func_data ;
} ForAllDataStruct, *ForAllData ;


static ZMap createZMap(void *app_data, ZMapFeatureSequenceMap seq_map) ;
static void destroyZMap(ZMap zmap) ;
static void killFinal(ZMap *zmap, ZMapView view) ;
static void killViews(ZMap zmap, GList **destroyed_views_inout) ;
static gboolean findViewInZMap(ZMap zmap, ZMapView view) ;
static void updateControl(ZMap zmap, ZMapView view) ;
static void dataLoadCB(ZMapView view, void *app_data, void *view_data) ;
static void enterCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void leaveCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlFocusCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlSelectCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlSplitToPatternCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlVisibilityChangeCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void viewStateChangeCB(ZMapView view, void *app_data, void *view_data) ;
static void viewKilledCB(ZMapView view, void *app_data, void *view_data) ;
static void infoPanelLabelsHashCB(gpointer labels_data);
static void removeView(ZMap zmap, ZMapView view, unsigned long xwid) ;

static void forAllCB(void *data, void *user_data) ;

static void setCursorCB(ZMapView view, void *user_data) ;

/* These variables holding callback routine information are static because they are
 * set just once for the lifetime of the process. */

/* Holds callbacks the level above us has asked to be called back on. */
static ZMapCallbacks zmap_cbs_G = NULL ;

/* Holds callbacks we set in the level below us to be called back on. */
ZMapViewCallbacksStruct view_cbs_G =
  {
    enterCB,
    leaveCB,
    dataLoadCB,
    controlFocusCB,
    controlSelectCB,
    controlSplitToPatternCB,
    controlVisibilityChangeCB,
    viewStateChangeCB,
    viewKilledCB,
    NULL,						    /* Filled in by caller for xremote stuff. */
    NULL
  } ;




/*
 *                         External functions
 */



/* This routine must be called just once before any other zmaps routine, it is a fatal error
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 *
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of zmaps terminate routine. */
void zMapInit(ZMapCallbacks callbacks)
{
  zMapReturnIfFail(!zmap_cbs_G) ;

  zMapReturnIfFail(callbacks
                   && callbacks->view_add && callbacks->view_destroy
                   &&callbacks->destroy && callbacks->quit_req) ; 

  zmap_cbs_G = g_new0(ZMapCallbacksStruct, 1) ;

  zmap_cbs_G->view_add = callbacks->view_add ;
  zmap_cbs_G->view_destroy = callbacks->view_destroy ;
  zmap_cbs_G->destroy = callbacks->destroy ;
  zmap_cbs_G->quit_req = callbacks->quit_req ;

  zmap_cbs_G->remote_request_func = callbacks->remote_request_func ;
  zmap_cbs_G->remote_request_func_data = callbacks->remote_request_func_data ;


  /* Init view.... */
  view_cbs_G.remote_request_func = callbacks->remote_request_func ;
  view_cbs_G.remote_request_func_data = callbacks->remote_request_func_data ;

  zMapViewInit(&view_cbs_G) ;

  return ;
}


/* Create a new zmap which is blank with no views. Returns NULL on failure.
 * Note how I casually assume that none of this can fail. */
ZMap zMapCreate(void *app_data, ZMapFeatureSequenceMap seq_map, GdkCursor *normal_cursor)
{
  ZMap zmap = NULL ;

  /* No callbacks, then no zmap creation. */
  /* if (!zmap_cbs_G) 
    return zmap ; */
  zMapReturnValIfFail(zmap_cbs_G, zmap) ; 

  zmap = createZMap(app_data, seq_map) ;

  /* Make the main/toplevel window for the ZMap. */
  zmapControlWindowCreate(zmap, normal_cursor) ;

  zmap->state = ZMAP_INIT ;

  if (zmap_cbs_G->remote_request_func)
    zmap->remote_control = TRUE ;

  zmapControlWindowSetGUIState(zmap) ;

  zmapControlWindowMaximize(zmap->toplevel, zmap) ;

  return zmap ;
}


gboolean zMapRaise(ZMap zmap)
{
  zMapReturnValIfFail(zmap && (zmap->state != ZMAP_DYING), FALSE) ;

  /* Presents a window to the user. This may mean raising the window
   * in the stacking order, deiconifying it, moving it to the current
   * desktop, and/or giving it the keyboard focus, possibly dependent
   * on the user's platform, window manager, and preferences.
   */
  gtk_window_present(GTK_WINDOW(zmap->toplevel));

  return TRUE ;
}


/* Set cursor on this zmap but also on all child widgets. */
void zMapSetCursor(ZMap zmap, GdkCursor *cursor)
{

  zMapGUISetCursor(GTK_WIDGET(zmap->toplevel), cursor) ;

  zMapControlForAllZMapViews(zmap, setCursorCB, cursor) ;

  return ;
}




gboolean zMapSetSessionColour(ZMap zmap, GdkColor *session_colour)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((zmap && session_colour && zmap->menubar), FALSE) ; 

  zmap->session_colour = *session_colour ;
  zmap->session_colour_set = TRUE ;

  gtk_widget_modify_bg(zmap->menubar, GTK_STATE_NORMAL, &(zmap->session_colour)) ;

  return result ;
}



void zMapControlForAllZMapViews(ZMap zmap, ZMapControlForAllCallbackFunc user_func_cb, void *user_func_data)
{
  ForAllDataStruct all_data = {user_func_cb, user_func_data} ;

  g_list_foreach(zmap->view_list, forAllCB, &all_data) ;

  return ;
}



/* Noddy function to return number of current views. */
int zMapNumViews(ZMap zmap)
{
  int num_views = 0 ;

  zMapReturnValIfFail(zmap, num_views) ; 

  num_views = g_list_length(zmap->view_list) ;

  return num_views ;
}



/* Might rename this to be more meaningful maybe.... */
ZMapViewWindow zMapAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, GError **error)
{
  ZMapViewWindow view_window = NULL ;

  zMapReturnValIfFail((zmap && sequence_map->sequence && *sequence_map->sequence
                       && (sequence_map->start > 0 && sequence_map->end > sequence_map->start)) , view_window ) ;  

  g_return_val_if_fail((zmap->state != ZMAP_DYING), NULL) ;

  if ((view_window = zmapControlAddView(zmap, sequence_map)))
    {
      zmapControlWindowSetGUIState(zmap) ;
    }

  return view_window ;
}


/* Inserts a view into an existing zmap and returns that view, on error returns NULL
 * and an error message in err_msg which the caller should free with g_free.
 * Note this call only creates the window and tells the view to connect, the connection
 * may fail some time later resulting in the view being removed. */
ZMapView zMapControlInsertView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, char **err_msg)
{
  ZMapView view = NULL ;
  ZMapViewWindow view_window ;
  GError *tmp_error = NULL ;

  zMapReturnValIfFail(zmap, view) ;

  if ((view_window = zmapControlAddView(zmap, sequence_map)))
    {
      view = zMapViewGetView(view_window) ;

      if (!zMapViewConnect(sequence_map, view, NULL, &tmp_error))
	{
          *err_msg = g_strdup_printf("Display of sequence \"%s\" failed: %s",
                                     sequence_map->sequence, (tmp_error ? tmp_error->message : "<no error message>")) ;

	  zMapViewDestroy(view) ;

	  view = NULL ;
	}
    }

  if (tmp_error)
    {
      g_error_free(tmp_error) ;
      tmp_error = NULL ;
    }

  return view ;
}


gboolean zmapConnectViewConfig(ZMap zmap, ZMapView view, char *config)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((zmap && view && findViewInZMap(zmap, view)), result );
  zMapReturnValIfFail((zmap->state != ZMAP_DYING), FALSE) ;

  result = zMapViewConnect(zmap->default_sequence, view, config, NULL) ;

  zmapControlWindowSetGUIState(zmap) ;

  return result ;
}


gboolean zMapConnectView(ZMap zmap, ZMapView view, GError **error)
{
  gboolean result = FALSE ;

  zMapReturnValIfFail((zmap && view && findViewInZMap(zmap, view)), result ) ;  
  zMapReturnValIfFail((zmap->state != ZMAP_DYING), FALSE) ;

  if ((result = zMapViewConnect(zmap->default_sequence, view, NULL, error)))
    zmapControlWindowSetGUIState(zmap) ;

  return result ;
}


/* We need a load or connect call here..... */
gboolean zMapLoadView(ZMap zmap, ZMapView view)
{
  printf("not implemented\n") ;

  return FALSE ;
}

gboolean zMapStopView(ZMap zmap, ZMapView view)
{
  printf("not implemented\n") ;

  return FALSE ;
}


gboolean zMapControlCloseView(ZMap zmap, ZMapView view)
{
  /* NEED RETURN CODE HERE............!!!!!!!!!!!! */

  zmapControlCloseFull(zmap, view) ;

  return TRUE ;
}


void zMapDeleteView(ZMap zmap, ZMapView view, GList **destroyed_views_inout)
{
  gboolean view_found = FALSE ;

  if (zmap && view && (view_found = findViewInZMap(zmap, view)))
    {
      if (zmap->state != ZMAP_DYING)
	{
	  zmapControlRemoveView(zmap, view, destroyed_views_inout) ;
	}
    }
  else
    {
      zMapLogCritical("Bad Parameters: %s%s%s",
		      (zmap ? "" : "NULL ZMap"),
		      (view ? "" : "NULL View"),
		      (view_found ? "" : "View not found in zmap")) ;
    }

  return ;
}



/* Reset an existing ZMap, this call will:
 *
 *    - Completely reset a ZMap window to blank with no sequences displayed and no
 *      threads attached or anything.
 *    - Frees all ZMap window data
 *    - Kills all existings server threads etc.
 *
 * After this call the ZMap will be ready for the user to specify a new sequence to be
 * loaded.
 *
 *  */
gboolean zMapReset(ZMap zmap)
{
  gboolean result = FALSE ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* not implemented or needed even ?? */

  zMapReturnValIfFail((zmap && zmap->state != ZMAP_DYING), result ) ;

  if (zmap->state == ZMAP_VIEWS)
    {
      killViews(zmap) ;

      zmap->state = ZMAP_RESETTING ;

      result = TRUE ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}



/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id = NULL ;

  zMapReturnValIfFail((zmap && zmap->state != ZMAP_DYING), id ) ;

  id = zmap->zmap_id ;

  return id ;
}


char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static const char *zmapStates[] = {"ZMAP_INIT", "ZMAP_VIEWS", "ZMAP_RESETTING",
			       "ZMAP_DYING"} ;
  char *state = NULL ;

  zMapReturnValIfFail((zmap && zmap->state >= ZMAP_INIT && zmap->state <= ZMAP_DYING), state  ) ;

  state = (char *)zmapStates[zmap->state] ;

  return state ;
}


/* Check if there are any unsaved changes and if so ask the user whether to save or not.
 * Returns true if we should continue or false if we should cancel. */
gboolean zMapCheckIfUnsaved(ZMap zmap)
{
  gboolean result = TRUE ;
  GList *list_item ;

  list_item = zmap->view_list ;
  do
    {
      ZMapView view ;

      view = (ZMapView)(list_item->data) ;

      result = zMapViewCheckIfUnsaved(view);
    }
  while (result && (list_item = g_list_next(list_item))) ;

  return result ;
}


/* Called to kill a zmap window and get all associated windows/threads destroyed.
 */
void zMapDestroy(ZMap zmap, GList **destroyed_views_inout)
{
  GList *destroyed_views = NULL ;

  if (destroyed_views_inout)
    destroyed_views = *destroyed_views_inout ;

  zmapControlDoKill(zmap, &destroyed_views) ;

  if (destroyed_views_inout)
    *destroyed_views_inout = destroyed_views ;

  return ;
}


/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing this would just send a kill to all the views but not wait for the
 * the reply but is complicated by toplevel window potentially being removed before the views
 * windows... */




/*
 *                Package routines.
 */

ZMapCallbacks zmapControlGetCallbacks(void)
{
  return zmap_cbs_G ;
}




/* Interactive Version:
 * 
 * This function encapsulates logic about how to handle closing the last view or the last
 * window in a view, we give the user the choice about whether to do this. */
void zmapControlClose(ZMap zmap)
{
  int num_views, num_windows ;
  ZMapViewWindow view_window ;
  ZMapView view ;
  GList *destroyed_views = NULL ;
  gboolean view_destroyed = FALSE ;

  num_views = zmapControlNumViews(zmap) ;

  /* We shouldn't get called if there are no views. */
  /* if (!num_views || !zmap->focus_viewwindow) 
    return ; */
  zMapReturnIfFail(num_views && zmap && zmap->focus_viewwindow ) ; 


  /* focus_viewwindow gets reset so hang on to view_window pointer and view.*/
  view_window = zmap->focus_viewwindow ;
  view = zMapViewGetView(view_window) ;


  /* If there is just one view or just one window left in a view then we warn the user. */
  num_windows = zMapViewNumWindows(view_window) ;
  if (num_views == 1 && num_windows == 1)
    {
      if (zMapGUIMsgGetBool(GTK_WINDOW(zmap->toplevel), ZMAP_MSG_WARNING,
			    "Closing this window will close this zmap window, "
			    "do you really want to do this ?"))
	{
	  zmapControlDoKill(zmap, &destroyed_views) ;

	  view_destroyed = TRUE ;
	}
    }
  else
    {
      char *msg ;

      char *name = zMapViewGetSequence(view);
      msg = g_strdup_printf("Closing this window will remove view \"%s\", "
			    "do you really want to do this ?", name) ;

      if (num_windows > 1 || zMapGUIMsgGetBool(GTK_WINDOW(zmap->toplevel), ZMAP_MSG_WARNING, msg))
	{
	  zmapControlRemoveWindow(zmap, view_window, &destroyed_views) ;

	  if (num_windows == 1)
	    view_destroyed = TRUE ;
	}

      g_free(name);
      g_free(msg) ;
    }

  /* If stuff was destroyed then report it....... */
  if (view_destroyed && destroyed_views)
    {
      ZMapCallbacks zmap_cbs_G ;

      /* Call the application callback so that they know a view has gone from within this zmap. */
      zmap_cbs_G = zmapControlGetCallbacks() ;

      (*(zmap_cbs_G->view_destroy))(zmap, view, zmap->app_data) ;


      /* CURRENTLY WE ARE NOT REPORTING WHEN THE USER UNSPLITS THE WINDOW, IT DOESN'T CREATE
       * A NEW VIEW SO LOGICALLY WE CAN'T SAY MUCH ABOUT IT, THIS MAY CHANGE THOUGH.
       * HENCE CURRENT REDUNDANCY WITH view_destroyed FLAG, WE MAY NEED TO REINSTATE REPORTING. */
      if (zmap->remote_control)
        zmapControlSendViewDeleted(zmap, destroyed_views) ;
    }

  return ;
}


/* Function call version of the above. */
void zmapControlCloseFull(ZMap zmap, ZMapView view)
{
  int num_views, num_windows ;
  ZMapViewWindow view_window ;
  GList *destroyed_views = NULL ;


  num_views = zmapControlNumViews(zmap) ;

  /* We shouldn't get called if there are no views. */
  zMapReturnIfFail(num_views) ; 

  /* focus_viewwindow gets reset so hang on to view_window pointer and view.*/
  view_window = zmapControlFindViewWindow(zmap, view) ;


  /* If there is just one view or just one window left in a view then we warn the user. */
  num_windows = zMapViewNumWindows(view_window) ;
  if (num_views == 1 && num_windows == 1)
    {
      zmapControlDoKill(zmap, &destroyed_views) ;
    }
  else
    {
      zmapControlRemoveWindow(zmap, view_window, &destroyed_views) ;
    }


  /* shouldn't we reporting a view was destroyed here ???? */
  if (destroyed_views && zmap->remote_control)
    {
      zmapControlSendViewDeleted(zmap, destroyed_views) ;
    }


  return ;
}



void zmapControlWindowSetGUIState(ZMap zmap)
{

  zmapControlWindowSetButtonState(zmap,&zmap->filter) ;

  /* We also need to set the navigator state..... */

  zmapControlWindowSetStatus(zmap) ;

  return ;
}


/* This function sets the button status and other bits of the GUI for a particular view/window. */
void zmapControlSetGUIVisChange(ZMap zmap, ZMapWindowVisibilityChange vis_change)
{
  gboolean automatic_open = FALSE;
  int pane_width ;

  /* There is replication here so need to deal with that.... */
  zmapControlWindowSetButtonState(zmap,&zmap->filter) ;

  zmapControlWindowSetZoomButtons(zmap, vis_change->zoom_status) ;

  zmapControlWindowSetStatus(zmap) ;

  /* If the user has zoomed in so far that we cannot show the whole sequence in one window
   * then open the pane that shows the window navigator scroll bar. */
  pane_width = zmapNavigatorSetWindowPos(zmap->navigator,
					 vis_change->scrollable_top, vis_change->scrollable_bot) ;
  if(automatic_open)
    gtk_paned_set_position(GTK_PANED(zmap->hpane), pane_width) ;


  return ;
}







/* Called when the user kills the toplevel window of the ZMap either by clicking the "quit"
 * button or by using the window manager frame menu to kill the window.
 *
 * Really this function just signals the zmap to be killed. */
void zmapControlSignalKill(ZMap zmap)
{

  zMapReturnIfFail(zmap) ; 

  gtk_widget_destroy(zmap->toplevel) ;

  return ;
}


/* This routine gets called when, either via a direct call or a callback (user action) the ZMap
 * needs to be destroyed. It does not do the whole destroy but instead signals all the Views
 * to die, when they die they call our view_detroyed_cb and this will eventually destroy the rest
 * of the ZMap when all the views have gone. At this point we will be able to signal to the
 * layer above us that we have died. */
void zmapControlDoKill(ZMap zmap, GList **destroyed_views_out)
{
  zMapReturnIfFail(zmap) ; 

  if (zmap->state != ZMAP_DYING)
    {
      /* set our state to DYING....so we don't respond to anything anymore....
       * Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;

      /* There may be no views if we are killed early on before connecting in which case
       * we can just kill the zmap.If there are no views we can just go ahead and kill everything, otherwise we just
       * signal all the views to die. */
      if (!(zmap->view_list))
	{
	  killFinal(&zmap, NULL) ;
	}
      else
	{
	  GList *destroyed_views = NULL ;

	  killViews(zmap, &destroyed_views) ;

	  if (destroyed_views_out)
	    *destroyed_views_out = destroyed_views ;
	}
    }

  return ;
}


void zmapControlLoadCB(ZMap zmap)
{
  GError *tmp_error = NULL ;
  zMapReturnIfFail(zmap && zmap->focus_viewwindow) ; 

  /* We can only load something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      gboolean status = TRUE ;
      ZMapView curr_view ;

      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error
       * if no view is selected....perhaps there should always be a selected view. */

      curr_view = zMapViewGetView(zmap->focus_viewwindow) ;

      if (zMapViewGetStatus(curr_view) != ZMAPVIEW_MAPPED)    /* ZMAPVIEW_INIT */
	{
	  zMapWarning("%s", "ZMap not ready to load, please retry when ready.") ;
	}
      else
	{
          if (!(status = zMapViewConnect(zmap->default_sequence, curr_view, NULL, &tmp_error)))
            {
              zMapCritical("%s", "ZMap could not configure server connections, please check connection data: %s",
                           (tmp_error ? tmp_error->message : "<no error message>")) ;
            }
	}
    }

  if (tmp_error)
    {
      g_error_free(tmp_error) ;
      tmp_error = NULL ;
    }

  return ;
}


void zmapControlResetCB(ZMap zmap)
{
  zMapReturnIfFail(zmap) ; 

  /* We can only reset something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      ZMapView curr_view ;

      curr_view = zMapViewGetView(zmap->focus_viewwindow) ;

      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error
       * if no view is selected....perhaps there should always be a selected view. */
      zMapViewReset(curr_view) ;

      zmapControlWindowSetGUIState(zmap) ;
    }

  return ;
}



ZMapViewWindow zmapControlAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map)
{
  ZMapViewWindow view_window = NULL ;

  zMapReturnValIfFail(zmap, view_window) ; 

  if ((view_window = zmapControlNewWindow(zmap, sequence_map)))
    {
      ZMapView view ;

      view = zMapViewGetView(view_window) ;

      /* add to list of views.... */
      zmap->view_list = g_list_append(zmap->view_list, view) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debug... */
      zmapControlPrintView(zmap, view, "Added", TRUE) ;
      zmapControlPrintAllViews(zmap, TRUE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      zmap->state = ZMAP_VIEWS ;
    }

  return view_window ;
}



void zmapControlRemoveView(ZMap zmap, ZMapView view, GList **destroyed_views_inout)
{
  zMapViewDestroy(view) ;

  if (destroyed_views_inout)
    *destroyed_views_inout = g_list_append(*destroyed_views_inout, view) ;

  return ;
}



/*
 *                      Internal functions
 */


/* Note that we rely on the struct being set to binary zeros to act as initialisation for most
 * fields. */
static ZMap createZMap(void *app_data, ZMapFeatureSequenceMap seq_map)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new0(ZMapStruct, 1) ;

  zmap->zmap_cbs_G = zmap_cbs_G ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("%d", zmap_num) ;

  zmap->app_data = app_data ;
  zmap->default_sequence = seq_map;

  /* Use default hashing functions, but THINK ABOUT THIS, MAY NEED TO ATTACH DESTROY FUNCTIONS. */
  zmap->viewwindow_2_parent = g_hash_table_new(NULL, NULL) ;

  zmap->view2infopanel = g_hash_table_new_full(NULL, NULL, NULL, infoPanelLabelsHashCB);

  return zmap ;
}




/* This is the strict opposite of createZMap(), should ONLY be called once all of the Views
 * and other resources have gone. */
static void destroyZMap(ZMap zmap)
{
  zMapReturnIfFail(zmap) ; 

  if (zmap->sequence_dialog)
    gtk_widget_destroy(zmap->sequence_dialog) ;

  if (zmap->import_file_dialog)
    gtk_widget_destroy(zmap->import_file_dialog) ;


  g_free(zmap->zmap_id) ;

  g_hash_table_destroy(zmap->viewwindow_2_parent) ;

  g_hash_table_destroy(zmap->view2infopanel);

  g_free(zmap) ;

  return ;
}





/* Called when a view has loaded data. */
static void dataLoadCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* Update title etc., need to update per column loaded for exciting feedback to the user */
  updateControl(zmap, view) ;

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}



/* This routine gets called when someone clicks in one of the zmap windows....it
 * handles general focus handling of windows etc.
 */
static void controlFocusCB(ZMapViewWindow view_window, void *app_data, void *view_data_unused)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapView view = zMapViewGetView(view_window) ;
  ZMapWindowNavigator navigator = NULL;
  GList *list_item = NULL;

  /* Make this view window the focus view window. */
  zmapControlSetWindowFocus(zmap, view_window) ;

  list_item = zmap->view_list;


  /* Step through each of the navigators and get their size and
   * maximise the current view_window->view navigator */

  do
    {
      ZMapView view_item = (ZMapView)(list_item->data);
      navigator = zMapViewGetNavigator(view_item);
      /* badly named function... */
      zMapWindowNavigatorFocus(navigator, FALSE);
    }
  while((list_item = g_list_next(list_item)));

  /* The view_window->view's navigator */
  navigator = zMapViewGetNavigator(view);

  if (navigator)
    {
      ZMapWindow window    = zMapViewGetWindow(view_window);
      /* Make sure that this is the one we see */
      zMapWindowNavigatorFocus(navigator, TRUE);
      zMapWindowNavigatorSetCurrentWindow(navigator, window);
    }

  /* If view has features then change the window title. */
  updateControl(zmap, view) ;

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}


/* This routine gets called when someone clicks in one of the zmap window items, i.e.
 * a feature, a column etc. It gets passed text which this routine then displays.
 */
static void controlSelectCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapViewSelect vselect = (ZMapViewSelect)view_data ;

  if (vselect->type == ZMAPWINDOW_SELECT_SINGLE)
    {
      ZMapInfoPanelLabels labels;
      labels = (ZMapInfoPanelLabels)g_hash_table_lookup(zmap->view2infopanel, zMapViewGetView(view_window));
      /* Display the feature details in the info. panel. */
      if (vselect)
	{
	  zmapControlInfoPanelSetText(zmap, labels, &(vselect->feature_desc)) ;
	  zmapControlWindowSetButtonState(zmap,&(vselect->filter) );	/* for filter button */
	}
      else
	{
	  zmapControlInfoPanelSetText(zmap, labels, NULL) ;
	  zmapControlWindowSetButtonState(zmap,&zmap->filter);	/* for filter button */
	}
    }

  return ;
}

static void controlSplitToPatternCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data;
  ZMapView zmap_view = NULL;
  ZMapWindow zmap_window = NULL;
  ZMapViewSplitting split = (ZMapViewSplitting)view_data;
  ZMapViewWindow new_view_window = NULL;
  GtkWidget *window_container = NULL;
  ZMapSplitPattern pattern = NULL;
  char *title = NULL;
  unsigned int i;

  for(i = 0; i < split->split_patterns->len; i++)
    {
      ZMapViewWindow tmp_vw = NULL;

      pattern = &(g_array_index(split->split_patterns, ZMapSplitPatternStruct, i));

      switch(pattern->subject)
        {
        case ZMAPSPLIT_NEW:
          tmp_vw = (ZMapViewWindow)((g_list_last(split->touched_window_list))->data);
          break;
        case ZMAPSPLIT_LAST:
          {
            GList *tmp_list = split->touched_window_list;
            int i = 2;          /* Go through loop twice */

            /* First time sets to the last list member, second time if there is another, the one before that. */
            for(tmp_list = g_list_last(tmp_list); i > 0 && tmp_list; i--)
              {
                tmp_vw   = view_window;
                tmp_list = tmp_list->prev;
              }
          }
          break;
        case ZMAPSPLIT_ORIGINAL:
          tmp_vw = view_window;
          break;
        default:
          zMapWarnIfReached();
        }

      if (tmp_vw)
        {
          zmap_view   = zMapViewGetView(tmp_vw);
          zmap_window = zMapViewGetWindow(tmp_vw);

          title = zMapViewGetSequence(zmap_view);

          /* hmmm.... */
          window_container = (GtkWidget*)g_hash_table_lookup(zmap->viewwindow_2_parent, tmp_vw);

          if((new_view_window = zmapControlNewWidgetAndWindowForView(zmap, zmap_view,
                                                                     zmap_window,
                                                                     window_container,
                                                                     pattern->orientation,
                                                                     ZMAPCONTROL_SPLIT_LAST,
                                                                     title)))
            {
              split->touched_window_list = g_list_append(split->touched_window_list, new_view_window);
            }
        }
    }

  zmapControlWindowSetGUIState(zmap) ;

  /* leave this to the last minute */
  gtk_widget_show_all(zmap->toplevel) ;

  return ;
}

/* Called when a view needs to tell us it has changed. Note that we only need to change
 * anything if the view in question is the focus view. */
static void controlVisibilityChangeCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapWindowVisibilityChange vis_change = (ZMapWindowVisibilityChange)view_data ;

  if (view_window == zmap->focus_viewwindow)
    zmapControlSetGUIVisChange(zmap, vis_change) ;

  return ;
}



static void enterCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMap zmap = (ZMap)app_data ;

  zmapControlSetWindowFocus(zmap, view_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}

static void leaveCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMap zmap = (ZMap)app_data ;

  zmapControlUnSetWindowFocus(zmap, view_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}



/* Gets called when a ZMapView resets, this is asynchronous because the view has to kill threads
 * and wait for them to die.
 *  */
static void viewStateChangeCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  if (zmap->state != ZMAP_DYING)
    zmapControlWindowSetGUIState(zmap) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* AS FAR AS I CAN TELL THIS IS NEVER CALLED BECAUSE fubar IS NEVER PASSED IN BY OUR CALLER.... */

#if 1
  if(zmap->remote_control)
    {
      ZMapViewCallbackFubar fubar = (ZMapViewCallbackFubar)view_data ;

      if(fubar && fubar->state == ZMAPVIEW_MAPPED)
	{
        gchar *response = NULL;
        gchar *xml_text, *xml_stub;

        xml_stub = zMapViewRemoteReceiveAccepts(view);

          /* Do some processing of the answer to make it fit the protocol */
        xml_text = zMapXRemoteProcessForReply(zmap->xremote_server, ZMAPXREMOTE_OK, xml_stub);
        response = g_strdup_printf(ZMAP_XREMOTE_REPLY_FORMAT, ZMAPXREMOTE_OK, xml_text) ;

        /* REMOTELOGMSG(Warning, "[XREMOTE respond to new_view] %s", response); */
        /*zMapLogWarning( "[XREMOTE respond to new_view] %s", response); */


          /* actually do the replying */
        zMapXRemoteSetReply(zmap->xremote_server, response);

          /* clean up */
        g_free(response) ;
        g_free(xml_text) ;
        g_free(xml_stub) ;
      }

/*
      ZMapXRemoteSendCommandError result;

        request = g_strdup_printf(
            "<zmap> <request action=\"view_ready\">"
            " <client xwid=\"0x%lx\" />"
              "</request></zmap>",
            fubar->xwid) ;

        if ((result = zMapXRemoteSendRemoteCommand(zmap->xremote_client, request, &response)) != ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
          {
            response = response ? response : zMapXRemoteGetResponse(zmap->xremote_client);
            zMapLogWarning("Notify of view mapped failed: %d,\"%s\"", result, response) ;
          }

        g_free(request);
      }
*/
    }
#endif
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return ;
}


/* Gets called from Zmap View layer when a ZMapView dies which can happen for two
 * reasons:
 *
 * 1) We requested the view to die but this is asynchronous because the view has to
 *    kill threads and wait for them to die before it can signal us back that it has
 *    really died.
 *
 * 2) The View may have died because it has detected an error and is now signalling us
 *    to say it has died.
 *
 * NOTE we have made a policy decision to kill the whole zmap when the last view
 * goes away.
 *
 *  */
static void viewKilledCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapViewCallbackDestroyData destroy_data = (ZMapViewCallbackDestroyData)view_data ;

  removeView(zmap, view, destroy_data->xwid) ;

  if (zmap->view_list)
    {
      /* Call the application callback so that they know a view has gone from within this zmap. */
      (*(zmap_cbs_G->view_destroy))(zmap, view, zmap->app_data) ;
    }
  else
    {
      if (zmap->state != ZMAP_DYING)
	{
	  zMapLogCritical("ZMap \"%s\": the last view has died but zmap is not in ZMAP_DYING state.,"
			  " this means views were not cancelled but died for other reasons,"
			  " see previous log entries.", zmap->zmap_id) ;
	  zmap->state = ZMAP_DYING ;
	}

      killFinal(&zmap, view) ;
    }

  return ;
}


/* This MUST only be called once all the views have gone. */
static void killFinal(ZMap *zmap_out, ZMapView view)
{
  ZMap zmap = *zmap_out ;
  void *app_data = zmap->app_data ;			    /* Hang on to app data. */

  if (zmap->state != ZMAP_DYING) 
    return ;

  /* Free the top window */
  if (zmap->toplevel)
    {
      zmapControlWindowDestroy(zmap) ;
      zmap->toplevel = NULL ;
    }

  destroyZMap(zmap) ;					    /* destroys zmap block. */

  /* Call the application callback so that they know we have finally died. */
  (*(zmap_cbs_G->destroy))(zmap, view, app_data) ;

  *zmap_out = NULL ;

  return ;
}


static void killViews(ZMap zmap, GList **destroyed_views_inout)
{
  GList *list_item ;

  list_item = zmap->view_list ;
  do
    {
      ZMapView view ;

      view = (ZMapView)list_item->data ;

      zmapControlRemoveView(zmap, view, destroyed_views_inout) ;
    }
  while ((list_item = g_list_next(list_item))) ;

  return ;
}


/* Find a given view within a given zmap. */
static gboolean findViewInZMap(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;
  GList* list_ptr ;

  if ((list_ptr = g_list_find(zmap->view_list, view)))
    result = TRUE ;

  return result ;
}


static void updateControl(ZMap zmap, ZMapView view)
{

  /* We only do this if the view is the current one. */
  if ((zmap->focus_viewwindow) && (view = zMapViewGetView(zmap->focus_viewwindow)))
    {
      ZMapFeatureContext features ;
      double top, bottom ;
      char *title, *db_name = NULL, *db_title = NULL, *seq_name, *dataset = NULL ;

      features = zMapViewGetFeatures(view) ;

      zMapViewGetVisible(zmap->focus_viewwindow, &top, &bottom) ;

      zmapNavigatorSetView(zmap->navigator, features, top, bottom) ;
							    /* n.b. features may be NULL for
							       blank views. */


      /* Update title bar of zmap window. */
      zMapViewGetSourceNameTitle(view, &db_name, &db_title) ;
      seq_name = zMapViewGetSequence(view) ;
      dataset = zMapViewGetDataset(view) ;

      title = g_strdup_printf("%s%s%s%s%.3s%s%s%s",
			      db_name ? db_name : "",
			      db_title ? " - ": "",
			      db_title ? db_title : "",
                              db_name || db_title ? "-" : "",
                              dataset ? dataset : "",
                              dataset ? " " : "",
			      seq_name ? seq_name : "<no sequence>",
			      features ? "" : " <no sequence loaded>") ;
      zMapGUISetToplevelTitle(zmap->toplevel, zMapGetZMapID(zmap), title) ;

      g_free(title) ;
      g_free(seq_name);


      /* Set up zoom buttons. */
      {
	ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow) ;
	ZMapWindowZoomStatus zoom_status = zMapWindowGetZoomStatus(window) ;

	zmapControlWindowSetZoomButtons(zmap, zoom_status) ;
      }
    }

  return ;
}

void zmapControlInfoOverwrite(void *data, int code, char *format, ...)
{
  ZMap zmap = (ZMap)data ;
  char *callInfo = NULL;
  va_list args;

  g_clear_error(&(zmap->info));

  va_start(args, format);
  callInfo = g_strdup_vprintf(format, args);
  va_end(args);

  zmap->info = g_error_new(g_quark_from_string(__FILE__), code,
                           "%s", callInfo) ;
  g_free(callInfo);

  return ;
}

void zmapControlInfoSet(void *data, int code, char *format, ...)
{
  ZMap zmap = (ZMap)data ;
  char *callInfo = NULL;
  va_list args;

  va_start(args, format);
  callInfo = g_strdup_vprintf(format, args);
  va_end(args);

  if(!zmap->info)
    zmapControlInfoOverwrite(zmap, code, callInfo);

  g_free(callInfo);

  return ;
}



static void infoPanelLabelsHashCB(gpointer labels_data)
{
  ZMapInfoPanelLabels labels = (ZMapInfoPanelLabels)labels_data;

  /* Widget may already have been destroyed if whole zmap window has been destroyed. */
  if (labels->hbox)
    gtk_widget_destroy(labels->hbox) ;

  g_free(labels);

  return ;
}


/* Remove references to a view from the zmap and if there is a remote client
 * then signal it to say view is gone. */
static void removeView(ZMap zmap, ZMapView view, unsigned long xwid)
{

  if (!findViewInZMap(zmap, view))
    {
      zMapLogCritical("Could not find view %p in zmap %s", view, zmap->zmap_id) ;
    }
  else
    {
      g_hash_table_remove(zmap->view2infopanel, view);

      zmap->view_list = g_list_remove(zmap->view_list, view) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* debug. */
      zmapControlPrintView(zmap, view, "Removed", FALSE) ;
      zmapControlPrintAllViews(zmap, FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return ;
}




static void forAllCB(void *data, void *user_data)
{
  ZMapView view = (ZMapView)data ;
  ForAllData all_data = (ForAllData)user_data ;

  (all_data->user_func_cb)(view, all_data->user_func_data) ;

  return ;
}




static void setCursorCB(ZMapView view, void *user_data)
{
  GdkCursor *cursor = (GdkCursor *)user_data ;
 
  zMapViewSetCursor(view, cursor) ;

  return ;
}
