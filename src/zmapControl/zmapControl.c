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
 * Last edited: Jul 21 17:47 2006 (rds)
 * Created: Thu Jul 24 16:06:44 2003 (edgrif)
 * CVS info:   $Id: zmapControl.c,v 1.66 2006-07-22 09:32:20 rds Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <gtk/gtk.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>




static ZMap createZMap(void *app_data) ;
static void destroyZMap(ZMap zmap) ;
static void killZMap(ZMap zmap) ;
static void killFinal(ZMap zmap) ;
static void killViews(ZMap zmap) ;
static gboolean findViewInZMap(ZMap zmap, ZMapView view) ;
static void updateControl(ZMap zmap, ZMapView view) ;

static void dataLoadCB(ZMapView view, void *app_data, void *view_data) ;
static void enterCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void leaveCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void focusCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void selectCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlDoubleSelectCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlSplitToPatternCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void controlVisibilityChangeCB(ZMapViewWindow view_window, void *app_data, void *view_data) ;
static void viewStateChangeCB(ZMapView view, void *app_data, void *view_data) ;
static void viewKilledCB(ZMapView view, void *app_data, void *view_data) ;



/* These variables holding callback routine information are static because they are
 * set just once for the lifetime of the process. */

/* Holds callbacks the level above us has asked to be called back on. */
static ZMapCallbacks zmap_cbs_G = NULL ;

/* Holds callbacks we set in the level below us to be called back on. */
ZMapViewCallbacksStruct view_cbs_G = {enterCB, leaveCB,
				      dataLoadCB, focusCB, selectCB, 
                                      controlDoubleSelectCB, controlSplitToPatternCB,
				      controlVisibilityChangeCB, viewStateChangeCB, viewKilledCB} ;




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

  zMapAssert(callbacks && callbacks->destroy && callbacks->exit) ;

  zmap_cbs_G = g_new0(ZMapCallbacksStruct, 1) ;

  zmap_cbs_G->destroy = callbacks->destroy ;
  zmap_cbs_G->exit = callbacks->exit ;

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

  zmapControlWindowSetGUIState(zmap) ;

  return zmap ;
}


gboolean zMapRaise(ZMap zmap)
{
  /* Presents a window to the user. This may mean raising the window
   * in the stacking order, deiconifying it, moving it to the current
   * desktop, and/or giving it the keyboard focus, possibly dependent
   * on the user's platform, window manager, and preferences. 
   */
  gtk_window_present(GTK_WINDOW(zmap->toplevel));

  return TRUE;
}



/* Might rename this to be more meaningful maybe.... */
ZMapView zMapAddView(ZMap zmap, char *sequence, int start, int end)
{
  ZMapView view ;

  zMapAssert(zmap && sequence && *sequence
	     && (start > 0 && (end == 0 || end > start))) ;

  view = zmapControlAddView(zmap, sequence, start, end) ;

  zmapControlWindowSetGUIState(zmap) ;

  return view ;
}


gboolean zMapConnectView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  result = zMapViewConnect(view, NULL) ;

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



gboolean zMapDeleteView(ZMap zmap, ZMapView view)
{
  gboolean result = FALSE ;

  zMapAssert(zmap && view && findViewInZMap(zmap, view)) ;

  zmap->view_list = g_list_remove(zmap->view_list, view) ;

  zMapViewDestroy(view) ;

  if (!zmap->view_list)
    zmap->state = ZMAP_INIT ;

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



/* should be using this other places too I feel zmapControlRemote.c for example... */
/* Gets the toplevel widget x-window window id */
unsigned long zMapGetXID(ZMap zmap)
{
  GtkWidget *top = NULL;
  top = zmap->toplevel;
  unsigned long id = 0;

  if(GTK_WIDGET_REALIZED(top) == TRUE)
    id = (Window)GDK_DRAWABLE_XID(top->window);

  return id;
}


/* We could provide an "executive" kill call which just left the threads dangling, a kind
 * of "kill -9" style thing this would just send a kill to all the views but not wait for the
 * the reply but is complicated by toplevel window potentially being removed before the views
 * windows... */




/*
 * These functions are internal to zmapControl.
 * 
 */


/* This function encapsulates logic about how to handle closing the last view or the last
 * window in a view, we give the user the choice about whether to do this. */
void zmapControlClose(ZMap zmap)
{
  int num_views, num_windows ;
  ZMapViewWindow view_window ;
  ZMapView view ;

  num_views = zmapControlNumViews(zmap) ;

  /* We shouldn't get called if there are no views. */
  zMapAssert(num_views && zmap->focus_viewwindow) ;


  /* focus_viewwindow gets reset so hang on to view_window pointer and view.*/
  view_window = zmap->focus_viewwindow ;
  view = zMapViewGetView(view_window) ;


  /* If there is just one view or just one window left in a view then we warn the user. */
  num_windows = zMapViewNumWindows(view_window) ;
  if (num_views == 1 && num_windows == 1)
    {
      if (zMapGUIShowChoice(GTK_WINDOW(zmap->toplevel), ZMAP_MSG_WARNING,
			    "Closing this window will close this zmap window, "
			    "do you really want to do this ?"))
	killZMap(zmap) ;
    }
  else
    {
      char *msg ;

      msg = g_strdup_printf("Closing this window will remove view \"%s\", "
			    "do you really want to do this ?", zMapViewGetSequence(view)) ;

      if (num_windows > 1
	  || zMapGUIShowChoice(GTK_WINDOW(zmap->toplevel), ZMAP_MSG_WARNING, msg))
	zmapControlRemoveWindow(zmap) ;

      g_free(msg) ;
    }


  return ;
}



void zmapControlWindowSetGUIState(ZMap zmap)
{

  zmapControlWindowSetButtonState(zmap) ;

  /* We also need to set the navigator state..... */

  zmapControlWindowSetStatus(zmap) ;

  return ;
}


/* This function sets the button status and other bits of the GUI for a particular view/window. */
void zmapControlSetGUIVisChange(ZMap zmap, ZMapWindowVisibilityChange vis_change)
{
  int pane_width ;

  /* There is replication here so need to deal with that.... */
  zmapControlWindowSetButtonState(zmap) ;

  zmapControlWindowSetZoomButtons(zmap, vis_change->zoom_status) ;

  zmapControlWindowSetStatus(zmap) ;

  /* If the user has zoomed in so far that we cannot show the whole sequence in one window
   * then open the pane that shows the window navigator scroll bar. */
  pane_width = zMapNavigatorSetWindowPos(zmap->navigator,
					 vis_change->scrollable_top, vis_change->scrollable_bot) ;
  gtk_paned_set_position(GTK_PANED(zmap->hpane), pane_width) ;


  return ;
}







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
	status = zMapViewConnect(curr_view, NULL) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (status && zMapViewGetStatus(curr_view) == ZMAPVIEW_CONNECTED)
	zMapViewLoad(curr_view) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return ;
}


void zmapControlResetCB(ZMap zmap)
{

  /* We can only reset something if there is at least one view. */
  if (zmap->state == ZMAP_VIEWS)
    {
      ZMapView curr_view ;
      ZMapViewState view_state ;

      curr_view = zMapViewGetView(zmap->focus_viewwindow) ;
      view_state = zMapViewGetStatus(curr_view) ;

      /* for now we are just doing the current view but this will need to change to allow a kind
       * of global load of all views if there is no current selected view, or perhaps be an error 
       * if no view is selected....perhaps there should always be a selected view. */
      zMapViewReset(curr_view) ;

      zmapControlWindowSetGUIState(zmap) ;
    }

  return ;
}




ZMapView zmapControlAddView(ZMap zmap, char *sequence, int start, int end)
{
  ZMapView view = NULL ;

  if ((view = zmapControlNewWindow(zmap, sequence, start, end)))
    {
      /* add to list of views.... */
      zmap->view_list = g_list_append(zmap->view_list, view) ;

      zmap->state = ZMAP_VIEWS ;
    }

  return view ;
}







/*
 *  ------------------- Internal functions -------------------
 */


/* Note that we rely on the struct being set to binary zeros to act as initialisation for most
 * fields. */
static ZMap createZMap(void *app_data)
{
  ZMap zmap = NULL ;
  GdkScreen *screen ;

  /* GROSS HACK FOR NOW, NEED SOMETHING BETTER LATER, JUST A TACKY ID...... */
  static int zmap_num = 0 ;

  zmap = g_new0(ZMapStruct, 1) ;

  zmap->zmap_cbs_G = zmap_cbs_G ;

  zmap_num++ ;
  zmap->zmap_id = g_strdup_printf("ZMap.%d", zmap_num) ;

  zmap->app_data = app_data ;

  /* Use default hashing functions, but THINK ABOUT THIS, MAY NEED TO ATTACH DESTROY FUNCTIONS. */
  zmap->viewwindow_2_parent = g_hash_table_new(NULL, NULL) ;


  /* Set up some screen stuff, we make zmap 2/3rd height of the screen by default.
   * If someone displays a really short piece of dna this will make the window
   * too big so really we should readjust the window size to fit the sequence
   * but this will be rare. */
  screen = gdk_screen_get_default() ;
  zmap->window_height = (int)((float)(gdk_screen_get_height(screen)) * 0.67) ;

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



/* Called when a view has loaded data. */
static void dataLoadCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* Update title etc. */
  updateControl(zmap, view) ;

  zmapControlWindowSetGUIState(zmap) ;

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

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}


/* This routine gets called when someone clicks in one of the zmap window items, i.e.
 * a feature, a column etc. It gets passed text which this routine then displays.
 */
static void selectCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;
  ZMapViewSelect vselect = (ZMapViewSelect)view_data ;
  GtkClipboard* clip = NULL;

  /* Display the feature details in the info. panel. */
  zmapControlInfoPanelSetText(zmap, &(vselect->feature_desc)) ;

  /* We also set this to the primary X selection to the secondary_text, confused? */
  if(vselect->secondary_text)
    {
      if((clip = gtk_widget_get_clipboard(GTK_WIDGET(zmap->toplevel), 
                                          GDK_SELECTION_PRIMARY)) != NULL)
        gtk_clipboard_set_text(clip, vselect->secondary_text, -1);
    }

  return ;
}

static void controlDoubleSelectCB(ZMapViewWindow view_window, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data;
  ZMapViewDoubleSelect double_select = (ZMapViewDoubleSelect)view_data;

  zmapControlRemoteAlertClients(zmap, double_select->xml_events, "edit");  

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
  int i;

  for(i = 0; i < split->split_patterns->len; i++)
    {
      ZMapViewWindow tmp_vw = NULL;

      pattern = &(g_array_index(split->split_patterns, ZMapSplitPatternStruct, i));

      zMapAssert(pattern);

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
          zMapAssertNotReached();
        }

      zMapAssert(tmp_vw);

      zmap_view   = zMapViewGetView(tmp_vw);
      zmap_window = zMapViewGetWindow(tmp_vw);

      title = zMapViewGetSequence(zmap_view);

      /* hmmm.... */
      window_container = g_hash_table_lookup(zmap->viewwindow_2_parent, tmp_vw);

      if((new_view_window = zmapControlNewWidgetAndWindowForView(zmap, zmap_view,
                                                                 zmap_window,
                                                                 window_container,
                                                                 pattern->orientation,
                                                                 title)))
        {
          split->touched_window_list = g_list_append(split->touched_window_list, new_view_window);
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

  /* We should test to see if the view is still in the list of views.... */

  zmapControlWindowSetGUIState(zmap) ;


  return ;
}


/* Gets called when a ZMapView dies, this is asynchronous because the view has to kill threads
 * and wait for them to die and also because the ZMapView my die of its own accord.
 * 
 * BUT NOTE we have made a policy decision to kill the whole zmap when the last view
 * goes away.
 * 
 *  */
static void viewKilledCB(ZMapView view, void *app_data, void *view_data)
{
  ZMap zmap = (ZMap)app_data ;

  /* We should test to see if the view is still in the list, if so we should kill it off.... */
  if (findViewInZMap(zmap, view))
    {
      zMapDeleteView(zmap, view) ;

      /* do we need to update the focus window here ??? */


    }
  else
    {
      /* A View has died so we should clean up its stuff.... */
      zmap->view_list = g_list_remove(zmap->view_list, view) ;
    }


  if (!zmap->view_list)
    {
      /* Call the application callback first so that there will be no more interaction with us */
      (*(zmap_cbs_G->destroy))(zmap, zmap->app_data) ;

      killFinal(zmap) ;

      zmap->state = ZMAP_DYING ;
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

void zmapControlInfoOverwrite(void *data, int code, char *format, ...)
{
  ZMap zmap = (ZMap)data ;
  char *callInfo = NULL;
  va_list args;

  g_clear_error(&(zmap->info));

  va_start(args, format);
  callInfo = g_strdup_vprintf(format, args);
  va_end(args);

  zmap->info = g_error_new(g_quark_from_string(__FILE__),
                           code,
                           callInfo
                           );
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




