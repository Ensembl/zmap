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
 * Last edited: Feb 13 11:25 2006 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowButtons.c,v 1.32 2006-02-17 14:12:03 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindow.h>
#include <zmapControl_P.h>



static void reloadCB(GtkWidget *widget, gpointer cb_data) ;
static void stopCB(GtkWidget *widget, gpointer cb_data) ;
static void zoomInCB(GtkWindow *widget, gpointer cb_data) ;
static void zoomOutCB(GtkWindow *widget, gpointer cb_data) ;
static void vertSplitPaneCB(GtkWidget *widget, gpointer data) ;
static void horizSplitPaneCB(GtkWidget *widget, gpointer data) ;
static void unlockCB(GtkWidget *widget, gpointer data) ;
static void revcompCB(GtkWidget *widget, gpointer data) ;
static void closeWindowCB(GtkWidget *widget, gpointer data) ;







GtkWidget *zmapControlWindowMakeButtons(ZMap zmap)
{
  GtkWidget *hbox,
    *reload_button,
    *stop_button,
    *hsplit_button, *vsplit_button, *zoomin_button, *zoomout_button,
    *unlock_button, *revcomp_button,
    *close_button ;

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
                                                                                           
  zmap->zoomin_but = zoomin_button = gtk_button_new_with_label("Zoom In");
  gtk_signal_connect(GTK_OBJECT(zoomin_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomInCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomin_button, FALSE, FALSE, 0) ;
                                                                                           
  zmap->zoomout_but = zoomout_button = gtk_button_new_with_label("Zoom Out");
  gtk_signal_connect(GTK_OBJECT(zoomout_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomOutCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomout_button, FALSE, FALSE, 0) ;

  zmap->unlock_but = unlock_button = gtk_button_new_with_label("Unlock");
  gtk_signal_connect(GTK_OBJECT(unlock_button), "clicked",
		     GTK_SIGNAL_FUNC(unlockCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), unlock_button, FALSE, FALSE, 0) ;

  zmap->revcomp_but = revcomp_button = gtk_button_new_with_label("Revcomp");
  gtk_signal_connect(GTK_OBJECT(revcomp_button), "clicked",
		     GTK_SIGNAL_FUNC(revcompCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), revcomp_button, FALSE, FALSE, 0) ;

  zmap->close_but = close_button = gtk_button_new_with_label("Close") ;
  gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
		     GTK_SIGNAL_FUNC(closeWindowCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), close_button, FALSE, FALSE, 0) ;

  /* Make Close button the default, its probably what user wants to do mostly. */
  GTK_WIDGET_SET_FLAGS(close_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(zmap->toplevel), close_button) ;

  return hbox ;
}



/* THE STATE OF SOME BUTTONS REQUIRES THE VIEW STATE TO BE SORTED OUT COMPLETELY.... */
void zmapControlWindowSetButtonState(ZMap zmap)
{
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT ;
  gboolean locked = FALSE ;
  gboolean stop = FALSE, load = FALSE ;
  gboolean general = FALSE ;

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

	view = zMapViewGetView(zmap->focus_viewwindow) ;

	view_state = zMapViewGetStatus(view) ;

	if (view_state == ZMAPVIEW_CONNECTED)
	  {
	    stop = TRUE ;
	    general = FALSE ;
	  }
	else
	  general = TRUE ;

	window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	zoom_status = zMapWindowGetZoomStatus(window) ;
	locked = zMapWindowIsLocked(window) ;
      }
    default:
      {
	/* Nothing to do.... */
      }
    }

  load = !stop ;

  gtk_widget_set_sensitive(zmap->stop_button, stop) ;
  gtk_widget_set_sensitive(zmap->load_button, load) ;
  gtk_widget_set_sensitive(zmap->hsplit_button, general) ;
  gtk_widget_set_sensitive(zmap->vsplit_button, general) ;
  zmapControlWindowSetZoomButtons(zmap, zoom_status) ;
  gtk_widget_set_sensitive(zmap->unlock_but, locked) ;
  gtk_widget_set_sensitive(zmap->revcomp_but, general) ;
  gtk_widget_set_sensitive(zmap->close_but, general) ;

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

  zmapControlWindowSetGUIState(zmap) ;

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

  zmapControlSplitInsertWindow(zmap, NULL, GTK_ORIENTATION_VERTICAL) ;

  return ;
}


static void horizSplitPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zmapControlSplitInsertWindow(zmap, NULL, GTK_ORIENTATION_HORIZONTAL) ;

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


static void closeWindowCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;


  /* Should warn if this is last window for a view.... */
  zmapControlRemoveWindow(zmap) ;


  return ;
}




