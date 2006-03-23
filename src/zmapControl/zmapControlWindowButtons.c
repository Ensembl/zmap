/*  File: zmapWindow.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Implement the buttons on the zmap.
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Mar 23 11:06 2006 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowButtons.c,v 1.37 2006-03-23 11:07:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>

typedef struct
{
  ZMapWindow window ;
} ZoomMenuCBDataStruct, *ZoomMenuCBData ;


enum {ZOOM_MAX, ZOOM_ALLDNA, ZOOM_10, ZOOM_1000, ZOOM_MIN} ;

static void reloadCB(GtkWidget *widget, gpointer cb_data) ;
static void stopCB(GtkWidget *widget, gpointer cb_data) ;
static void zoomInCB(GtkWindow *widget, gpointer cb_data) ;
static void zoomOutCB(GtkWindow *widget, gpointer cb_data) ;
static void vertSplitPaneCB(GtkWidget *widget, gpointer data) ;
static void horizSplitPaneCB(GtkWidget *widget, gpointer data) ;
static void unlockCB(GtkWidget *widget, gpointer data) ;
static void revcompCB(GtkWidget *widget, gpointer data) ;
static void unsplitWindowCB(GtkWidget *widget, gpointer data) ;
static void columnConfigCB(GtkWidget *widget, gpointer data) ;
static gboolean zoomEventCB(GtkWidget *wigdet, GdkEvent *event, gpointer data);


static void makeZoomMenu(GdkEventButton *button_event, ZMapWindow window) ;
static ZMapGUIMenuItem makeMenuZoomOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
				       gpointer callback_data) ;
static void zoomMenuCB(int menu_item_id, gpointer callback_data) ;



GtkWidget *zmapControlWindowMakeButtons(ZMap zmap)
{
  GtkWidget *hbox,
    *reload_button, *stop_button,
    *hsplit_button, *vsplit_button,
    *zoomin_button, *zoomout_button,
    *unlock_button, *revcomp_button, *unsplit_button, *column_button ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);

  zmap->stop_button = stop_button = gtk_button_new_with_label("Stop") ;
  gtk_signal_connect(GTK_OBJECT(stop_button), "clicked",
		     GTK_SIGNAL_FUNC(stopCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0) ;

  zmap->load_button = reload_button = gtk_button_new_with_label("Reload") ;
  gtk_signal_connect(GTK_OBJECT(reload_button), "clicked",
		     GTK_SIGNAL_FUNC(reloadCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), reload_button, FALSE, FALSE, 0) ;

  zmap->hsplit_button = hsplit_button = gtk_button_new_with_label("H-Split");
  gtk_signal_connect(GTK_OBJECT(hsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(horizSplitPaneCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), hsplit_button, FALSE, FALSE, 0) ;

  zmap->vsplit_button = vsplit_button = gtk_button_new_with_label("V-Split");
  gtk_signal_connect(GTK_OBJECT(vsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(vertSplitPaneCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), vsplit_button, FALSE, FALSE, 0) ;
                                                                                           
  zmap->unsplit_but = unsplit_button = gtk_button_new_with_label("Unsplit") ;
  gtk_signal_connect(GTK_OBJECT(unsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(unsplitWindowCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), unsplit_button, FALSE, FALSE, 0) ;

  zmap->unlock_but = unlock_button = gtk_button_new_with_label("Unlock");
  gtk_signal_connect(GTK_OBJECT(unlock_button), "clicked",
		     GTK_SIGNAL_FUNC(unlockCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), unlock_button, FALSE, FALSE, 0) ;

  zmap->zoomin_but = zoomin_button = gtk_button_new_with_label("Zoom In");
  gtk_signal_connect(GTK_OBJECT(zoomin_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomInCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(zoomin_button), "event",
                   G_CALLBACK(zoomEventCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomin_button, FALSE, FALSE, 0) ;
                                                                                           
  zmap->zoomout_but = zoomout_button = gtk_button_new_with_label("Zoom Out");
  gtk_signal_connect(GTK_OBJECT(zoomout_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomOutCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(zoomout_button), "event",
                   G_CALLBACK(zoomEventCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomout_button, FALSE, FALSE, 0) ;

  zmap->revcomp_but = revcomp_button = gtk_button_new_with_label("Revcomp");
  gtk_signal_connect(GTK_OBJECT(revcomp_button), "clicked",
		     GTK_SIGNAL_FUNC(revcompCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), revcomp_button, FALSE, FALSE, 0) ;

  zmap->column_but = column_button = gtk_button_new_with_label("Columns");
  gtk_signal_connect(GTK_OBJECT(column_button), "clicked",
		     GTK_SIGNAL_FUNC(columnConfigCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), column_button, FALSE, FALSE, 0) ;

  /* Make Stop button the default, its the only thing the user can initially click ! */
  GTK_WIDGET_SET_FLAGS(stop_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(zmap->toplevel), stop_button) ;

  return hbox ;
}


/* Add tooltips to main zmap buttons. */
void zmapControlButtonTooltips(ZMap zmap)
{

  gtk_tooltips_set_tip(zmap->tooltips, zmap->stop_button,
		       "Stop loading of data, reset zmap",
		       "ZMap data loading is carried out by a separate thread "
		       "which can be halted and reset by clicking this button.") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->load_button,
		       "Reload data after a reset.",
		       "If zmap fails to load, either because you clicked the \"Stop\" "
		       "button or the loading failed, then you ran attempt to reload "
		       "by clicking this button.") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->hsplit_button,
		       "Split window horizontally",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->vsplit_button,
		       "Split window vertically",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->zoomin_but,
		       "Zoom in 2x (right click gives more zoom options)",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->zoomout_but,
		       "Zoom out 2x (right click gives more zoom options)",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->unlock_but,
		       "Unlock zoom/scroll from sibling window",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->revcomp_but,
		       "Reverse complement sequence view",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->unsplit_but,
		       "Unsplit selected window",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->column_but,
		       "Column configuration",
		       "") ;

  return ;
}




/* Set button state according to the overall ZMap state and the current view state. */
void zmapControlWindowSetButtonState(ZMap zmap)
{
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT ;
  gboolean general, unsplit, unlock, stop, reload ;

  general = unsplit = unlock = stop = reload = FALSE ;

  switch(zmap->state)
    {
    case ZMAP_DYING:
      /* Nothing to do currently... */
      break ;
    case ZMAP_VIEWS:
      {
	ZMapView view ;
	ZMapWindow window ;
	ZMapViewState view_state ;

	zMapAssert(zmap->focus_viewwindow) ;

	/* Get focus view status. */
	view = zMapViewGetView(zmap->focus_viewwindow) ;
	window = zMapViewGetWindow(zmap->focus_viewwindow) ;
	view_state = zMapViewGetStatus(view) ;

	switch(view_state)
	  {
	  case ZMAPVIEW_INIT:
	    reload = TRUE ;
	    break ;
	  case ZMAPVIEW_CONNECTING:
	  case ZMAPVIEW_CONNECTED:
	  case ZMAPVIEW_LOADING:
	    stop = TRUE ;
	    break ;
	  case ZMAPVIEW_LOADED:
	    general = TRUE ;

	    /* If we are down to the last view and that view has a single window then
	     * disable unsplit button, stops user accidentally closing whole window. */
	    if ((zmapControlNumViews(zmap) > 1) || (zMapViewNumWindows(zmap->focus_viewwindow) > 1))
	      unsplit = TRUE ;

	    zoom_status = zMapWindowGetZoomStatus(window) ;
	    unlock = zMapWindowIsLocked(window) ;
	    break ;
	  case ZMAPVIEW_RESETTING:
	    /* Nothing to do. */
	    break ;
	  case ZMAPVIEW_DYING:
	    /* Nothing to do. */
	    break ;
	  }

      }
    default:
      {
	/* Nothing to do.... */
      }
    }

  gtk_widget_set_sensitive(zmap->stop_button, stop) ;
  gtk_widget_set_sensitive(zmap->load_button, reload) ;
  gtk_widget_set_sensitive(zmap->hsplit_button, general) ;
  gtk_widget_set_sensitive(zmap->vsplit_button, general) ;
  zmapControlWindowSetZoomButtons(zmap, zoom_status) ;
  gtk_widget_set_sensitive(zmap->unlock_but, unlock) ;
  gtk_widget_set_sensitive(zmap->revcomp_but, general) ;
  gtk_widget_set_sensitive(zmap->unsplit_but, unsplit) ;
  gtk_widget_set_sensitive(zmap->column_but, general) ;

  return ;
}



/* We make an assumption in this routine that zoom will only be one of fixed, min, mid or max and
 * that zoom can only go from min to max _via_ the mid state. */
gboolean zmapControlWindowDoTheZoom(ZMap zmap, double zoom)
{
  ZMapWindow window = NULL;
  double factor;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  zMapWindowZoom(window, zoom) ;

  factor = 1.0;                 /* Fudge for now */
  zmapControlInfoOverwrite(zmap, 200,
                           "<magnification>%f</magnification>",
                           factor);
  return TRUE;
}


/* Provide user with visual feedback about status of zooming via the sensitivity of
 * the zoom buttons. */
void zmapControlWindowSetZoomButtons(ZMap zmap, ZMapWindowZoomStatus zoom_status)
{

  if (zoom_status == ZMAP_ZOOM_FIXED)
    {
      if (GTK_WIDGET_IS_SENSITIVE(zmap->zoomin_but))
	gtk_widget_set_sensitive(zmap->zoomin_but, FALSE) ;
      if (GTK_WIDGET_IS_SENSITIVE(zmap->zoomout_but))
	gtk_widget_set_sensitive(zmap->zoomout_but, FALSE) ;
    }
  else if (zoom_status == ZMAP_ZOOM_MIN)
    {
      if(!GTK_WIDGET_IS_SENSITIVE(zmap->zoomin_but))
        gtk_widget_set_sensitive(zmap->zoomin_but, TRUE) ;
      if(GTK_WIDGET_IS_SENSITIVE(zmap->zoomout_but))
        gtk_widget_set_sensitive(zmap->zoomout_but, FALSE) ;
    }
  else if (zoom_status == ZMAP_ZOOM_MID)
    {
      if (!GTK_WIDGET_IS_SENSITIVE(zmap->zoomin_but))
	gtk_widget_set_sensitive(zmap->zoomin_but, TRUE) ;
      if (!GTK_WIDGET_IS_SENSITIVE(zmap->zoomout_but))
	gtk_widget_set_sensitive(zmap->zoomout_but, TRUE) ;
    }
  else if (zoom_status == ZMAP_ZOOM_MAX)
    {
      if(GTK_WIDGET_IS_SENSITIVE(zmap->zoomin_but))
        gtk_widget_set_sensitive(zmap->zoomin_but, FALSE) ;
      if (!GTK_WIDGET_IS_SENSITIVE(zmap->zoomout_but)) /* not needed...? */
	gtk_widget_set_sensitive(zmap->zoomout_but, TRUE) ;
    }
  else if (zoom_status == ZMAP_ZOOM_INIT)
    {
      /* IN & OUT should be False until we know which Zoom we are. */
      gtk_widget_set_sensitive(zmap->zoomin_but,  FALSE) ;
      gtk_widget_set_sensitive(zmap->zoomout_but, FALSE) ;
    }
  return ;
}


/*
 *  ------------------- Internal functions -------------------
 */

/* This function implements menus that can be reached by right clicking on either of the
 * zoom main buttons, probably should generalise to handle all menus on buttons. */
static gboolean zoomEventCB(GtkWidget *wigdet, GdkEvent *event, gpointer data)
{
  gboolean handled = FALSE ;
  ZMap zmap = (ZMap)data ;
  ZMapWindow window ;

  zMapAssert(zmap->focus_viewwindow) ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *button_ev = (GdkEventButton *)event ;

        switch(button_ev->button)
          {
          case 3:
	    makeZoomMenu(button_ev, window) ;

            handled = TRUE;
            break;
          default:
            break;
          }
      }
      break;
    default:
      break;
    }

  return handled ;
}


/* These callbacks simply make calls to routines in zmapControl.c, this is because I want all
 * the state handling etc. to be in one file so that its easier to work on. */

static void reloadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlLoadCB(zmap) ;

  return ;
}


static void stopCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlResetCB(zmap) ;

  return ;
}


static void zoomInCB(GtkWindow *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlWindowDoTheZoom(zmap, 2.0) ;

  return ;
}


static void zoomOutCB(GtkWindow *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlWindowDoTheZoom(zmap, 0.5) ;

  return ;
}



static void vertSplitPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zmapControlSplitWindow(zmap, GTK_ORIENTATION_VERTICAL) ;

  return ;
}


static void horizSplitPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zmapControlSplitWindow(zmap, GTK_ORIENTATION_HORIZONTAL) ;

  return ;
}


static void unlockCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zMapWindowUnlock(zMapViewGetWindow(zmap->focus_viewwindow)) ;

  zmapControlWindowSetGUIState(zmap) ;

  return ;
}


static void revcompCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zMapViewReverseComplement(zMapViewGetView(zmap->focus_viewwindow)) ;
  
  return ;
}


static void unsplitWindowCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zmapControlClose(zmap) ;

  return ;
}


/* There is quite a question here...should column configuration apply to just the focus window,
 * the focus window plus any windows locked to it or all the windows within the focus windows
 * view.
 * 
 * Following the precedent of zooming and scrolling we apply it to the focus window plus
 * any locked to it.
 *  */
static void columnConfigCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;
  ZMapWindow window ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowColumnConfigure(window) ;

  return ;
}





/* Zoom menu stuff.... */
static void makeZoomMenu(GdkEventButton *button_event, ZMapWindow window)
{
  char *menu_title = "Zoom menu" ;
  GList *menu_sets = NULL ;
  ZoomMenuCBData menu_data ;

  /* Call back stuff.... */
  menu_data = g_new0(ZoomMenuCBDataStruct, 1) ;
  menu_data->window = window ;

  /* Make up the menu. */
  menu_sets = g_list_append(menu_sets, makeMenuZoomOps(NULL, NULL, menu_data)) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}


/* NOTE HOW THE MENUS ARE DECLARED STATIC IN THE VARIOUS ROUTINES TO MAKE SURE THEY STAY
 * AROUND...OTHERWISE WE WILL HAVE TO KEEP ALLOCATING/DEALLOCATING THEM.....
 */
static ZMapGUIMenuItem makeMenuZoomOps(int *start_index_inout,
					  ZMapGUIMenuItemCallbackFunc callback_func,
					  gpointer callback_data)
{
  static ZMapGUIMenuItemStruct menu[] =
    {
      {"Max (1 bp line)",           ZOOM_MAX,     zoomMenuCB, NULL},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {"10 bp line",                ZOOM_10,      zoomMenuCB, NULL},
      {"1000 bp line",              ZOOM_1000,    zoomMenuCB, NULL},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      {"All DNA",                   ZOOM_ALLDNA,  zoomMenuCB, NULL},

      {"Min (whole sequence)",      ZOOM_MIN,     zoomMenuCB, NULL},
      {NULL,                        0, NULL,       NULL}
    } ;

  zMapGUIPopulateMenu(menu, start_index_inout, callback_func, callback_data) ;

  return menu ;
}

static void zoomMenuCB(int menu_item_id, gpointer callback_data)
{
  ZoomMenuCBData menu_data = (ZoomMenuCBData)callback_data ;
  ZMapWindow window = menu_data->window ;
  double zoom_factor = 0.0, curr_factor = 0.0 ;

  curr_factor = zMapWindowGetZoomFactor(window) ;


  switch (menu_item_id)
    {
    case ZOOM_MAX:
      {
	zoom_factor = zMapWindowGetZoomMax(window) ;
	break ;
      }
    case ZOOM_ALLDNA:
      {
	zoom_factor = 0.97 ;
	break ;
      }
    case ZOOM_MIN:
      zoom_factor = zMapWindowGetZoomMin(window) ;
      break ;

    default:
      zMapAssertNotReached() ;				    /* exits... */
      break ;
    }


  zoom_factor = zoom_factor / curr_factor ;


  zMapWindowZoom(window, zoom_factor) ;

  g_free(menu_data) ;

  return ;
}



