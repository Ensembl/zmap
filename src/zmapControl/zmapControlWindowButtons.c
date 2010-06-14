/*  File: zmapWindow.c
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
 * and was written by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implement the buttons on the zmap.
 *
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Nov  3 12:27 2008 (rds)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowButtons.c,v 1.57 2010-06-14 15:40:12 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>

typedef struct
{
  ZMapWindow window ;
} ZoomMenuCBDataStruct, *ZoomMenuCBData ;

/* fMap has 1/10/100/1000 bp per line and whole, here we replicate + ALL DNA */
enum {ZOOM_MAX, ZOOM_ALLDNA, ZOOM_10, ZOOM_100, ZOOM_1000, ZOOM_MIN} ;

enum{ SHOW_DNA, HIDE_DNA, SHOW_3FT, HIDE_3FT };

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
static void frame3CB(GtkWidget *wigdet, gpointer data);
static void dnaCB(GtkWidget *wigdet, gpointer data);
static gboolean sequenceEventCB(GtkWidget *wigdet, GdkEvent *event, gpointer data);
static void backButtonCB(GtkWidget *wigdet, gpointer data);

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
    *unlock_button, *revcomp_button,
    *unsplit_button, *column_button,
    *frame3_button, *dna_button,
    *back_button, *separator ;

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);

  zmap->stop_button = stop_button = gtk_button_new_with_label("Stop") ;
  gtk_signal_connect(GTK_OBJECT(stop_button), "clicked",
		     GTK_SIGNAL_FUNC(stopCB), (gpointer)zmap) ;
  gtk_button_set_focus_on_click(GTK_BUTTON(stop_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0) ;

  zmap->load_button = reload_button = gtk_button_new_with_label("Reload") ;
  gtk_signal_connect(GTK_OBJECT(reload_button), "clicked",
		     GTK_SIGNAL_FUNC(reloadCB), (gpointer)zmap) ;
  gtk_button_set_focus_on_click(GTK_BUTTON(reload_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), reload_button, FALSE, FALSE, 0) ;

  zmap->hsplit_button = hsplit_button = gtk_button_new_with_label("H-Split");
  gtk_signal_connect(GTK_OBJECT(hsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(horizSplitPaneCB), (gpointer)zmap) ;
  gtk_button_set_focus_on_click(GTK_BUTTON(hsplit_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), hsplit_button, FALSE, FALSE, 0) ;

  zmap->vsplit_button = vsplit_button = gtk_button_new_with_label("V-Split");
  gtk_signal_connect(GTK_OBJECT(vsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(vertSplitPaneCB), (gpointer)zmap) ;
  gtk_button_set_focus_on_click(GTK_BUTTON(vsplit_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), vsplit_button, FALSE, FALSE, 0) ;

  zmap->unsplit_but = unsplit_button = gtk_button_new_with_label("Unsplit") ;
  gtk_signal_connect(GTK_OBJECT(unsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(unsplitWindowCB), (gpointer)zmap) ;
  gtk_button_set_focus_on_click(GTK_BUTTON(unsplit_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), unsplit_button, FALSE, FALSE, 0) ;

  zmap->unlock_but = unlock_button = gtk_button_new_with_label("Unlock");
  gtk_signal_connect(GTK_OBJECT(unlock_button), "clicked",
		     GTK_SIGNAL_FUNC(unlockCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(unlock_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), unlock_button, FALSE, FALSE, 0) ;

  zmap->revcomp_but = revcomp_button = gtk_button_new_with_label("Revcomp");
  gtk_signal_connect(GTK_OBJECT(revcomp_button), "clicked",
		     GTK_SIGNAL_FUNC(revcompCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(revcomp_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), revcomp_button, FALSE, FALSE, 0) ;

  zmap->frame3_but = frame3_button = gtk_button_new_with_label("3 Frame");
  gtk_signal_connect(GTK_OBJECT(frame3_button), "clicked",
		     GTK_SIGNAL_FUNC(frame3CB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(frame3_button), "event",
                   G_CALLBACK(sequenceEventCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(frame3_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), frame3_button, FALSE, FALSE, 0) ;

  zmap->dna_but = dna_button = gtk_button_new_with_label("DNA");
  gtk_signal_connect(GTK_OBJECT(dna_button), "clicked",
		     GTK_SIGNAL_FUNC(dnaCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(dna_button), "event",
                   G_CALLBACK(sequenceEventCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(dna_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), dna_button, FALSE, FALSE, 0) ;

  zmap->column_but = column_button = gtk_button_new_with_label("Columns");
  gtk_signal_connect(GTK_OBJECT(column_button), "clicked",
		     GTK_SIGNAL_FUNC(columnConfigCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(column_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), column_button, FALSE, FALSE, 0) ;

  separator = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 10) ;

  zmap->zoomin_but = zoomin_button = gtk_button_new_with_label("Zoom In");
  gtk_signal_connect(GTK_OBJECT(zoomin_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomInCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(zoomin_button), "event",
                   G_CALLBACK(zoomEventCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(zoomin_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), zoomin_button, FALSE, FALSE, 0) ;

  zmap->zoomout_but = zoomout_button = gtk_button_new_with_label("Zoom Out");
  gtk_signal_connect(GTK_OBJECT(zoomout_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomOutCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(zoomout_button), "event",
                   G_CALLBACK(zoomEventCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(zoomout_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), zoomout_button, FALSE, FALSE, 0) ;

  separator = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 10) ;

  zmap->back_button = back_button  = gtk_button_new_with_label("Back");
  gtk_signal_connect(GTK_OBJECT(back_button), "clicked",
		     GTK_SIGNAL_FUNC(backButtonCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(back_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), back_button, FALSE, FALSE, 0) ;


  /* Make Stop button the default, its the only thing the user can initially click ! */
  GTK_WIDGET_SET_FLAGS(stop_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(zmap->toplevel), stop_button) ;

  return hbox ;
}

static void control_gtk_tooltips_set_tip(GtkTooltips *tooltip, GtkWidget *widget,
					 char *simple, char *shortcut, char *full)
{
  char *simple_with_shortcut = NULL;

  if(shortcut)
    simple_with_shortcut = g_strdup_printf("%s\t[%s]", simple, shortcut);
  else
    simple_with_shortcut = simple;

  gtk_tooltips_set_tip(tooltip, widget, simple_with_shortcut, full);

  if(shortcut && simple_with_shortcut)
    g_free(simple_with_shortcut);

  return ;
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

  control_gtk_tooltips_set_tip(zmap->tooltips, zmap->revcomp_but,
			       "Reverse complement sequence view",
			       "R",
			       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->frame3_but,
		       "Toggle display of Reading Frame columns"
                       " (right click for more options)",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->dna_but,
		       "Toggle display of DNA"
                       " (right click for more options)",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->unsplit_but,
		       "Unsplit selected window",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->column_but,
		       "Column configuration",
		       "") ;
  gtk_tooltips_set_tip(zmap->tooltips, zmap->back_button,
		       "Step back to clear effect of previous action",
		       "Currently only handles zooming and marking") ;

  return ;
}




/* Set button state according to the overall ZMap state and the current view state. */
void zmapControlWindowSetButtonState(ZMap zmap)
{
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT ;
  gboolean general, unsplit, unlock, stop, reload, frame3, dna, back ;

  general = unsplit = unlock = stop = reload = frame3 = dna = back = FALSE ;

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
        case ZMAPVIEW_UPDATING:
	    stop = TRUE ;
	    break ;
	  case ZMAPVIEW_LOADED:
	    general = TRUE ;
            frame3 = TRUE;
            dna = TRUE;
	    /* If we are down to the last view and that view has a single window then
	     * disable unsplit button, stops user accidentally closing whole window. */
	    if ((zmapControlNumViews(zmap) > 1) || (zMapViewNumWindows(zmap->focus_viewwindow) > 1))
	      unsplit = TRUE ;

            /* Turn the DNA/Protein button on/off */
            dna = frame3 = zMapWindowGetDNAStatus(window);

	    zoom_status = zMapWindowGetZoomStatus(window) ;
	    unlock = zMapWindowIsLocked(window) ;
	    back = zMapWindowHasHistory(window);
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
  gtk_widget_set_sensitive(zmap->frame3_but, frame3) ;
  gtk_widget_set_sensitive(zmap->dna_but, dna) ;
  gtk_widget_set_sensitive(zmap->unsplit_but, unsplit) ;
  gtk_widget_set_sensitive(zmap->column_but, general) ;
  gtk_widget_set_sensitive(zmap->back_button, back) ;

  return ;
}



/* We make an assumption in this routine that zoom will only be one of fixed, min, mid or max and
 * that zoom can only go from min to max _via_ the mid state. */
gboolean zmapControlWindowDoTheZoom(ZMap zmap, double zoom)
{
  ZMapWindow window = NULL;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  zMapWindowZoom(window, zoom) ;

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

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *button_ev = (GdkEventButton *)event ;
	ZMap zmap = (ZMap)data ;
	ZMapWindow window ;

	zMapAssert(zmap->focus_viewwindow) ;

	window = zMapViewGetWindow(zmap->focus_viewwindow) ;

        switch(button_ev->button)
          {
          case 3:
	    makeZoomMenu(button_ev, window) ;

            handled = TRUE;
            break;
          default:
            break;
          }

	break;
      }
    default:
      break;
    }

  return handled ;
}


static void frame3CB(GtkWidget *wigdet, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapWindow window ;

  zMapAssert(zmap->focus_viewwindow) ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowToggle3Frame(window) ;

  return ;
}


static void dnaCB(GtkWidget *wigdet, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapWindow window ;
  GQuark align_id = 0, block_id = 0;
  gboolean force = FALSE,
    force_to     = TRUE,
    do_dna       = TRUE,
    do_aa        = FALSE;

  zMapAssert(zmap->focus_viewwindow) ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, force_to, force);

  return ;
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

  zmapControlSplitWindow(zmap, GTK_ORIENTATION_VERTICAL, ZMAPCONTROL_SPLIT_LAST) ;

  return ;
}


static void horizSplitPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;

  zmapControlSplitWindow(zmap, GTK_ORIENTATION_HORIZONTAL, ZMAPCONTROL_SPLIT_LAST) ;

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

  zMapWindowColumnList(window) ;

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
      {ZMAPGUI_MENU_NORMAL, "Max (1 bp line)",           ZOOM_MAX,     zoomMenuCB, NULL},

      {ZMAPGUI_MENU_NORMAL, "10 bp line",                ZOOM_10,      zoomMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "100 bp line",               ZOOM_100,     zoomMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "1000 bp line",              ZOOM_1000,    zoomMenuCB, NULL},

      {ZMAPGUI_MENU_NORMAL, "All DNA",                   ZOOM_ALLDNA,  zoomMenuCB, NULL},

      {ZMAPGUI_MENU_NORMAL, "Min (whole sequence)",      ZOOM_MIN,     zoomMenuCB, NULL},
      {ZMAPGUI_MENU_NONE, NULL,                        0, NULL,       NULL}
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
    case ZOOM_10:
      {
        zoom_factor  = zMapWindowGetZoomMax(window);
        zoom_factor /= 10;
        break;
      }
    case ZOOM_100:
      {
        zoom_factor  = zMapWindowGetZoomMax(window);
        zoom_factor /= 100;
        break;
      }
    case ZOOM_1000:
      {
        zoom_factor  = zMapWindowGetZoomMax(window);
        zoom_factor /= 1000;
        break;
      }
    case ZOOM_ALLDNA:
      {
	zoom_factor = zMapWindowGetZoomMaxDNAInWrappedColumn(window) ;
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

static void seqMenuCB(int menu_item_id, gpointer callback_data)
{
  ZMapGUIMenuSubMenuData data = (ZMapGUIMenuSubMenuData)callback_data;
  ZMapWindow window = NULL;
  GQuark align_id = 0, block_id = 0;
  gboolean force = TRUE,
    force_to     = FALSE,
    do_dna       = FALSE,
    do_aa        = FALSE;

  align_id = data->align_unique_id;
  block_id = data->block_unique_id;

  switch(menu_item_id)
    {
    case SHOW_DNA:
      force_to = TRUE;
      do_dna   = TRUE;
      break;
    case HIDE_DNA:
      do_dna   = TRUE;
      break;
    case SHOW_3FT:
      force_to  = TRUE;
      do_aa     = TRUE;
      break;
    case HIDE_3FT:
      do_aa     = TRUE;
      break;
    default:
      break;
    }

  if((window = data->original_data))
    zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, force_to, force);

  return ;
}

static void fixSubMenuData(gpointer list_data, gpointer user_data)
{
  ZMapGUIMenuItem item = (ZMapGUIMenuItem)list_data, tmp_data = NULL;
  ZMapGUIMenuSubMenuData sub_data = (ZMapGUIMenuSubMenuData)user_data,
    item_data = NULL;

  tmp_data = item;

  /* Step through the array of items, fixing up the data as we go */
  while(tmp_data && tmp_data->name)
    {
      if((item_data = tmp_data->callback_data))
        item_data->original_data = sub_data->original_data;
      tmp_data++;
    }

  return ;
}


static ZMapGUIMenuItem makeMenuSequenceOps(ZMapWindow window,
                                           GList **all_menus,
                                           int *start_index_inout,
                                           ZMapGUIMenuItemCallbackFunc callback_func,
                                           gpointer callback_data, gboolean dna, gboolean protein)
{
  static ZMapGUIMenuItemStruct dna_menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "DNA",          0,         NULL,      NULL},
      {ZMAPGUI_MENU_NORMAL, "DNA/Show All", SHOW_DNA,  seqMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "DNA/Hide All", HIDE_DNA,  seqMenuCB, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,           0,         NULL,      NULL}
    } ;

  static ZMapGUIMenuItemStruct aa_menu[] =
    {
      {ZMAPGUI_MENU_BRANCH, "3 Frame Translation",          0,        NULL,      NULL},
      {ZMAPGUI_MENU_NORMAL, "3 Frame Translation/Show All", SHOW_3FT, seqMenuCB, NULL},
      {ZMAPGUI_MENU_NORMAL, "3 Frame Translation/Hide All", HIDE_3FT, seqMenuCB, NULL},
      {ZMAPGUI_MENU_NONE,   NULL,                           0,        NULL,      NULL}
    };
  static GArray *dna_d_menus = NULL, *aa_d_menus = NULL;

  if (dna && !dna_d_menus)
    {
      ZMapGUIMenuItemStruct each_level_menu[] = {
        {ZMAPGUI_MENU_NORMAL, "Show All", SHOW_DNA, seqMenuCB, NULL},
        {ZMAPGUI_MENU_NORMAL, "Hide All", HIDE_DNA, seqMenuCB, NULL},
        {ZMAPGUI_MENU_NONE,   NULL,       0,        NULL,      NULL}
      };

      dna_d_menus = g_array_new(TRUE, TRUE, sizeof(ZMapGUIMenuItemStruct));
      zMapWindowMenuAlignBlockSubMenus(window,
                                       each_level_menu, each_level_menu,
                                       "DNA", &dna_d_menus);
    }

  if (protein && !aa_d_menus)
    {
      ZMapGUIMenuItemStruct each_level_menu[] = {
        {ZMAPGUI_MENU_NORMAL, "Show All", SHOW_3FT, seqMenuCB, NULL},
        {ZMAPGUI_MENU_NORMAL, "Hide All", HIDE_3FT, seqMenuCB, NULL},
        {ZMAPGUI_MENU_NONE,   NULL,       0,        NULL,      NULL}
      };

      aa_d_menus = g_array_new(TRUE, TRUE, sizeof(ZMapGUIMenuItemStruct));
      zMapWindowMenuAlignBlockSubMenus(window,
                                       each_level_menu, each_level_menu,
                                       "3 Frame Translation", &aa_d_menus);
    }

  if (all_menus)
    {
      if (dna)
	{
	  zMapGUIPopulateMenu(dna_menu, start_index_inout, callback_func, callback_data);
	  *all_menus = g_list_append(*all_menus, dna_menu);

	  *all_menus = g_list_append(*all_menus, dna_d_menus->data);
	}

      if (protein)
	{
	  zMapGUIPopulateMenu(aa_menu, start_index_inout, callback_func, callback_data);
	  *all_menus = g_list_append(*all_menus, aa_menu);

	  *all_menus = g_list_append(*all_menus, aa_d_menus->data);
	}
    }

  /* Here we have to fix up the data for the callbacks
   * We do this so that we get the correct window passed in.
   */
  g_list_foreach(*all_menus, fixSubMenuData, callback_data);

  return NULL ;
}

/* We expect dna or protein to be TRUE. */
static void makeSequenceMenu(GdkEventButton *button_event, ZMapWindow window, gboolean dna, gboolean protein)
{
  char *menu_title = "" ;
  GList *menu_sets = NULL ;
  static ZMapGUIMenuSubMenuData sub_data = NULL;

  if (dna && protein)
    menu_title = "Sequence menu" ;
  else if (dna)
    menu_title = "DNA menu" ;
  else if (protein)
    menu_title = "Translation menu" ;

  /* Set up the data so we pass in the correct window to our seqMenuCB
   * The issue of sub menu data is here as we have to dynamically generate
   * the quarks for the aligns and blocks... i.e. the contents of the
   * ZMapGUIMenuSubMenuData ...
   */
  if(!sub_data)
    {
      sub_data = g_new0(ZMapGUIMenuSubMenuDataStruct, 1);
      sub_data->original_data = window;
    }
  else
    sub_data->original_data = window;

  /* Make up the menu. */
  makeMenuSequenceOps(window, &menu_sets, NULL, NULL, sub_data, dna, protein) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}

static gboolean sequenceEventCB(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gboolean handled = FALSE;

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      {
	GdkEventButton *button_ev = (GdkEventButton *)event ;

        switch(button_ev->button)
          {
          case 3:
	    {
	      ZMap zmap = (ZMap)data ;
	      ZMapWindow window ;
	      gboolean dna = FALSE, protein = FALSE ;

	      zMapAssert(zmap->focus_viewwindow) ;

	      window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	      if (widget == zmap->frame3_but)
		protein = TRUE ;
	      else if (widget == zmap->dna_but)
		dna = TRUE ;

	      if (dna || protein)
		makeSequenceMenu(button_ev, window, dna, protein) ;

            handled = TRUE;
            break ;
	    }
          default:
            break;
          }
	break;
      }
    default:
      break;
    }

  return handled;
}


static void backButtonCB(GtkWidget *wigdet, gpointer data)
{
  ZMap zmap = (ZMap)data;
  ZMapWindow window = NULL;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  zMapWindowBack(window);

  return ;
}

