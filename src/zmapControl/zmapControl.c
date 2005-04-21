/*  File: zmapControl.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: This is the ZMap interface code, it controls both
 *              the window code and the threaded server code.
 * Exported functions: See ZMap.h
 * HISTORY:
 * Last edited: Apr 18 14:42 2005 (edgrif)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.49 2005-04-21 13:44:15 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>


static ZMap createZMap(void *app_data) ;
static void destroyZMap(ZMap zmap) ;
static void killZMap(ZMap zmap) ;
static void killFinal(ZMap zmap) ;
static void killViews(ZMap zmap) ;
static gboolean findViewInZMap(ZMap zmap, ZMapView view) ;
static ZMapView addView(ZMap zmap, char *sequence, int start, int end) ;
static void updateControl(ZMap zmap, ZMapView view) ;

static void dataLoadCB(ZMapView view, void *app_data, void *view_data) ;
static void enterCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void leaveCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void focusCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void selectCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void visibilityChangeCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void viewKilledCB(ZMapView view, void *app_data, void *view_data) ;



/* These variables holding callback routine information are static because they are
 * set just once for the lifetime of the process. */

/* Holds callbacks the level above us has asked to be called back on. */
static ZMapCallbacks zmap_cbs_G = NULL ;

/* Holds callbacks we set in the level below us to be called back on. */
ZMapViewCallbacksStruct view_cbs_G = {enterCB, leaveCB,
				      dataLoadCB, focusCB, selectCB,
				      visibilityChangeCB, viewKilledCB} ;




/*
 *  ------------------- External functions -------------------
 *   (includes callbacks)
 */



/* This routine must be called just once before any other zmaps routine, it is a fatal error
 * if the caller calls this routine more than once. The caller must supply all of the callback
 * routines.
 * 
 * Note that since this routine is called once per application we do not bother freeing it
 * via some kind of zmaps terminate routine. */
void zMapInit(ZMapCallbacks callbacks)
{
  zMapAssert(!zmap_cbs_G) ;

  zMapAssert(callbacks && callbacks->destroy) ;

  zmap_cbs_G = g_new0(ZMapCallbacksStruct, 1) ;

  zmap_cbs_G->destroy = callbacks->destroy ;

  /* Init view.... */
  zMapViewInit(&view_cbs_G) ;

  return ;
}



/* Create a new zmap which is blank with no views. Returns NULL on failure.
 * Note how I casually assume that none of this can fail. */
ZMap zMapCreate(void *app_data)
{
  ZMap zmap = NULL ;

  /* No callbacks, then no zmap creation. */
  zMapAssert(zmap_cbs_G) ;

  zmap = createZMap(app_data) ;

  /* Make the main/toplevel window for the ZMap. */
  zmapControlWindowCreate(zmap) ;

  zmap->state = ZMAP_INIT ;

  return zmap ;
}


/* Might rename this to be more meaningful maybe.... */
ZMapView zMapAddView(ZMap zmap, char *sequence, int start, int end)
{
  ZMapView view ;

  zMapAssert(zmap && sequence && *sequence
	     && (start > 0 && (end == 0 || end > start))) ;

  view = addView(zmap, sequence, start, end) ;

  return view ;
}


gboolean zMapConnectView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  result = zMapViewConnect(view) ;
  
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



gboolean zMapDeleteView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  zmap->view_list = g_list_remove(zmap->view_list, view) ;

  zMapViewDestroy(view) ;
  
  return result ;
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

  if (zmap->state == ZMAP_VIEWS)
    {
      killViews(zmap) ;

      zmap->state = ZMAP_RESETTING ;

      result = TRUE ;
    }

  return result ;
}



/*
 *    A set of accessor functions.
 */

char *zMapGetZMapID(ZMap zmap)
{
  char *id = NULL ;

  if (zmap->state != ZMAP_DYING)
    id = zmap->zmap_id ;

  return id ;
}


char *zMapGetZMapStatus(ZMap zmap)
{
  /* Array must be kept in synch with ZmapState enum in ZMap_P.h */
  static char *zmapStates[] = {"ZMAP_INIT", "ZMAP_VIEWS", "ZMAP_RESETTING",
			       "ZMAP_DYING"} ;
  char *state ;

  zMapAssert(zmap->state >= ZMAP_INIT && zmap->state <= ZMAP_DYING) ;

  state = zmapStates[zmap->state] ;

  return state ;
}


/* Called to kill a zmap window and get all associated windows/threads destroyed.
 */
gboolean zMapDestroy(ZMap zmap)
{
  killZMap(zmap) ;

  return TRUE ;
}



/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing this would just send a kill to all the views but not wait for the
 * the reply but is complicated by toplevel window potentially being removed before the views
 * windows... */




/*
 * These functions are internal to zmapControl and get called as a result of user interaction
 * with the gui.
 * 
 */



/* Called when the user kills the toplevel window of the ZMap either by clicking the "quit"
 * button or by using the window manager frame menu to kill the window. */
void zmapControlTopLevelKillCB(ZMap zmap)
{
  /* this is not strictly correct, there may be complications if we are resetting, sort this
   * out... */

  if (zmap->state != ZMAP_DYING)
    killZMap(zmap) ;

  return ;
}



void zmapControlLoadCB(ZMap zmap)
{

  /* We can only load something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      gboolean status = TRUE ;
      ZMapView curr_view ;

      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error 
       * if no view is selected....perhaps there should always be a selected view. */

      zMapAssert(zmap->focus_viewwindow) ;

      curr_view = zMapViewGetView(zmap->focus_viewwindow) ;

      if (zMapViewGetStatus(curr_view) == ZMAPVIEW_INIT)
	status = zMapViewConnect(curr_view) ;

      if (status && zMapViewGetStatus(curr_view) == ZMAPVIEW_RUNNING)
	zMapViewLoad(curr_view) ;
    }

  return ;
}


void zmapControlResetCB(ZMap zmap)
{

  /* We can only reset something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      ZMapView curr_view ;

      curr_view = zMapViewGetView(zmap->focus_viewwindow) ;

      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error 
       * if no view is selected....perhaps there should always be a selected view. */
      if (zMapViewGetStatus(curr_view) == ZMAPVIEW_RUNNING)
	{
	  zMapViewReset(curr_view) ;
	}
    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* this is not right, we may not need the resetting state at top level at all in fact..... */
  zmap->state = ZMAP_RESETTING ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}


/* Put a new view into a zmap. */
void zmapControlNewViewCB(ZMap zmap, char *new_sequence)
{
  ZMapView view ;

  /* these should be passed in ...... */
  int start = 1, end = 0 ;

  if ((view = addView(zmap, new_sequence, start, end)))
    zMapViewConnect(view) ;				    /* return code ???? */

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */


/* Note that we rely on the struct being set to binary zeros to act as initialisation for most
 * fields. */
static ZMap createZMap(void *app_data)
{
  ZMap zmap = NULL ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new0(ZMapStruct, 1) ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("ZMap.%d", zmap_num) ;

  zmap->app_data = app_data ;

  /* Use default hashing functions, but THINK ABOUT THIS, MAY NEED TO ATTACH DESTROY FUNCTIONS. */
  zmap->viewwindow_2_parent = g_hash_table_new(NULL, NULL) ;

  return zmap ;
}


/* This is the strict opposite of createZMap(), should ONLY be called once all of the Views
 * and other resources have gone. */
static void destroyZMap(ZMap zmap)
{
  g_free(zmap->zmap_id) ;

  g_hash_table_destroy(zmap->viewwindow_2_parent) ;

  g_free(zmap) ;

  return ;
}




/* This routine gets called when, either via a direct call or a callback (user action) the ZMap
 * needs to be destroyed. It does not do the whole destroy but instead signals all the Views
 * to die, when they die they call our view_detroyed_cb and this will eventually destroy the rest
 * of the ZMap (top level window etc.) when all the views have gone. */
static void killZMap(ZMap zmap)
{
  /* no action, if dying already.... */
  if (zmap->state != ZMAP_DYING)
    {
      /* Call the application callback first so that there will be no more interaction with us */
      (*(zmap_cbs_G->destroy))(zmap, zmap->app_data) ;

      /* If there are no views we can just go ahead and kill everything, otherwise we just
       * signal all the views to die. */
      if (zmap->state == ZMAP_INIT || zmap->state == ZMAP_RESETTING)
	{
	  killFinal(zmap) ;
	}
      else if (zmap->state == ZMAP_VIEWS)
	{
	  killViews(zmap) ;
	}


      /* set our state to DYING....so we don't respond to anything anymore.... */
      /* Must set this as this will prevent any further interaction with the ZMap as
       * a result of both the ZMap window and the threads dying asynchronously.  */
      zmap->state = ZMAP_DYING ;
    }

  return ;
}



static ZMapView addView(ZMap zmap, char *sequence, int start, int end)
{
  ZMapView view = NULL ;

  /* this is a bit clutzy, it is irrelevant for the first window.... */
  GtkOrientation orientation = GTK_ORIENTATION_VERTICAL ;


  if ((view = zMapViewCreate(sequence, start, end, (void *)zmap)))
    {
      /* add to list of views.... */
      zmap->view_list = g_list_append(zmap->view_list, view) ;

      zmapControlSplitInsertWindow(zmap, view, orientation) ;

      zmap->state = ZMAP_VIEWS ;
    }

  return view ;
}


/* Called when a view has loaded data. */
static void dataLoadCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* Update title etc. */
  updateControl(zmap, view) ;

  return ;
}



/* This routine gets called when someone clicks in one of the zmap windows....it
 * handles general focus handling of windows etc.
 */ 
static void focusCB(ZMapViewWindow view_window, void *app_data, void *view_data_unused)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapView view = zMapViewGetView(view_window) ;

  /* Make this view window the focus view window. */
  zmapControlSetWindowFocus(zmap, view_window) ;

  /* If view has features then change the window title. */
  updateControl(zmap, view) ;

  return ;
}


/* This routine gets called when someone clicks in one of the zmap window items, i.e.
 * a feature, a column etc. It gets passed text which this routine then displays.
 */
static void selectCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  char *text = (char *)view_data ;
  ZMapView view = zMapViewGetView(view_window) ;

  /* If there is text then display it in info. box of zmap. (There may be no text if
   * user clicked on blank background.) */
  if (text)
    gtk_entry_set_text(GTK_ENTRY(zmap->info_panel), text) ;

  return ;
}


static void visibilityChangeCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapWindowVisibilityChange vis_change = (ZMapWindowVisibilityChange)view_data ;

  zmapControlWindowSetZoomButtons(zmap, vis_change->zoom_status) ;

  zMapNavigatorSetWindowPos(zmap->navigator,
			    vis_change->scrollable_top, vis_change->scrollable_bot) ;

  return ;
}

static void enterCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapControlSetWindowFocus(zmap, view_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}

static void leaveCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  zmapControlUnSetWindowFocus(zmap, view_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



/* Gets called when a ZMapView dies, this is asynchronous because the view has to kill threads
 * and wait for them to die and also because the ZMapView my die of its own accord.
 * BUT NOTE that when this routine is called by the last ZMapView within the fmap to say that
 * it has died then we either reset the ZMap to its INIT state or if its dying we kill it. */
static void viewKilledCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* N.B. the pane needs to go away here....ugh...not well tested code...we must call closePane() */
  /* NOTE THAT THIS MUST END UP SETTING A NEW FOCUSPANE OR SETTING IT TO NULL IF THERE ARE NO
   * MORE VIEWS.... */

  /* A View has died so we should clean up its stuff.... */
  zmap->view_list = g_list_remove(zmap->view_list, view) ;

  /* If the last view has gone AND we are dying then we can kill the rest of the ZMap
   * and clean up, otherwise we just set the state to "reset". */
  if (!zmap->view_list)
    {
      if (zmap->state == ZMAP_DYING)
	{
	  killFinal(zmap) ;
	}
      else
	zmap->state = ZMAP_INIT ;
    }

  return ;
}


/* This MUST only be called once all the views have gone. */
static void killFinal(ZMap zmap)
{
  zMapAssert(zmap->state != ZMAP_VIEWS) ;

  /* Free the top window */
  if (zmap->toplevel)
    {
      zmapControlWindowDestroy(zmap) ;
      zmap->toplevel = NULL ;
    }

  destroyZMap(zmap) ;

  return ;
}





static void killViews(ZMap zmap)
{
  GList* list_item ;

  zMapAssert(zmap->view_list) ;

  list_item = zmap->view_list ;
  do
    {
      ZMapView view ;

      view = list_item->data ;

      zMapViewDestroy(view) ;
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
  if ((view = zMapViewGetView(zmap->focus_viewwindow)))
    {
      ZMapFeatureContext features ;
      double top, bottom ;
      char *title, *seq_name ;

      features = zMapViewGetFeatures(view) ;

      zMapViewGetVisible(zmap->focus_viewwindow, &top, &bottom) ;

      zMapNavigatorSetView(zmap->navigator, features, top, bottom) ;
							    /* n.b. features may be NULL for
							       blank views. */


      /* Update title bar of zmap window. */
      seq_name = zMapViewGetSequence(view) ;
      title = g_strdup_printf("%s - %s%s", zmap->zmap_id,
			      seq_name ? seq_name : "<no sequence>",
			      features ? "" : " <no sequence loaded>") ;
      gtk_window_set_title(GTK_WINDOW(zmap->toplevel), title) ;
      g_free(title) ;


      /* Set up zoom buttons. */
      {
	ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow) ;
	ZMapWindowZoomStatus zoom_status = zMapWindowGetZoomStatus(window) ;

	zmapControlWindowSetZoomButtons(zmap, zoom_status) ;
      }
    }


  return ;
}


