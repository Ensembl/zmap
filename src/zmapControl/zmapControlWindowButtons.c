/*  File: zmapWindow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapUtilsGUI.h>
#include <zmapControl_P.h>


typedef struct ZoomMenuCBDataStructName
{
  ZMapWindow window ;
} ZoomMenuCBDataStruct, *ZoomMenuCBData ;

/* fMap has 1/10/100/1000 bp per line and whole, here we replicate + ALL DNA */
enum {ZOOM_MAX, ZOOM_ALLDNA, ZOOM_10, ZOOM_100, ZOOM_1000, ZOOM_MIN} ;

enum{ SHOW_DNA, HIDE_DNA, HIDE_3ALL, SHOW_3FEATURES, SHOW_3FT, SHOW_3ALL };


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

static void makeSequenceMenu(GdkEventButton *button_event, ZMapWindow window) ;
static ZMapGUIMenuItem makeMenuSequenceOps(ZMapWindow window,
                                           GList **all_menus,
                                           int *start_index_inout,
                                           ZMapGUIMenuItemCallbackFunc callback_func, gpointer callback_data) ;


static void control_gtk_tooltips_set_tip(GtkTooltips *tooltip, GtkWidget *widget,
					 char *simple, char *shortcut, char *full) ;

static void filterValueChangedCB(GtkSpinButton *spinbutton, gpointer user_data);
static gboolean filterSpinButtonCB(GtkWidget *entry, GdkEvent *event, gpointer user_data);
static GtkWidget *newSpinButton(void) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Will be needed if we go back to complicated align/dna for each block menu system. */
static void fixSubMenuData(gpointer list_data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void seqMenuCB(int menu_item_id, gpointer callback_data) ;




GtkWidget *zmapControlWindowMakeButtons(ZMap zmap)
{
  GtkWidget *hbox,
    *reload_button, *stop_button,
    *hsplit_button, *vsplit_button,
    *zoomin_button, *zoomout_button,
    *filter_button,
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

  zmap->back_button = back_button  = gtk_button_new_with_label("Back");
  gtk_signal_connect(GTK_OBJECT(back_button), "clicked",
		     GTK_SIGNAL_FUNC(backButtonCB), (gpointer)zmap);
  gtk_button_set_focus_on_click(GTK_BUTTON(back_button), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), back_button, FALSE, FALSE, 0) ;

  separator = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 10) ;

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

  separator = gtk_vseparator_new();
  gtk_box_pack_start(GTK_BOX(hbox), separator, FALSE, FALSE, 10) ;

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
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* PLEASE SEE COMMENTS AT END OF FILE, FOR NOW I'M REMOVING THE MENU FOR DNA. */
  g_signal_connect(G_OBJECT(dna_button), "event",
                   G_CALLBACK(sequenceEventCB), (gpointer)zmap);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
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

  /* filter selected column by score */
  zmap->filter_but = filter_button = newSpinButton();
  g_signal_connect(G_OBJECT(filter_button), "value-changed",
                   G_CALLBACK(filterValueChangedCB), (gpointer)zmap);

  g_signal_connect(G_OBJECT(filter_button), "button-press-event",
                   G_CALLBACK(filterSpinButtonCB), (gpointer)zmap);
  g_signal_connect(G_OBJECT(filter_button), "button-release-event",
                   G_CALLBACK(filterSpinButtonCB), (gpointer)zmap);


  gtk_box_pack_start(GTK_BOX(hbox), filter_button, FALSE, FALSE, 0) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* THIS IS NOT A GOOD IDEA....LATER ON WHEN THE FEATURES ARE SHOWN AND WE DON'T INTERCEPT
   * THE STOP BUTTON USER CAN ACCIDENTALLY SELECT IT BY PRESSING THE <ENTER> BUTTON !!! */

  /* Make Stop button the default, its the only thing the user can initially click ! */
  GTK_WIDGET_SET_FLAGS(stop_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(zmap->toplevel), stop_button) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


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

  gtk_tooltips_set_tip(zmap->tooltips, zmap->filter_but,
		       "Filter selected column by score",
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
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		       /* See comments at end of file. */
                       " (right click for more options)"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		       ,
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


void filterSetHighlight(ZMap zmap)
{
  GdkColor white = { 0xffffffff, 0xffff, 0xffff, 0xffff } ;
  GdkColor *fill = &white;

  /* highlight if filtering occrured */
  if(zmap->filter.n_filtered && zmap->filter.window)
    zMapWindowGetFilteredColour(zmap->filter.window,&fill);

  gtk_widget_modify_base ((GtkWidget *) zmap->filter_but, GTK_STATE_NORMAL, fill);
}



/* Set button state according to the overall ZMap state and the current view state. */
void zmapControlWindowSetButtonState(ZMap zmap, ZMapWindowFilter window_filter)
{
  ZMapWindowZoomStatus zoom_status = ZMAP_ZOOM_INIT ;
  gboolean filter, general, revcomp,  unsplit, unlock, stop, reload, frame3, dna, back ;
  ZMapView view = NULL ;
  ZMapWindow window = NULL ;
  ZMapViewState view_state ;

  filter = general = revcomp =  unsplit = unlock = stop = reload = frame3 = dna = back = FALSE ;

  switch(zmap->state)
    {
    case ZMAP_DYING:
      /* Nothing to do currently... */
      break ;
    case ZMAP_VIEWS:
      {

	zMapAssert(zmap->focus_viewwindow) ;

	/* Get focus view status. */
	view = zMapViewGetView(zmap->focus_viewwindow) ;
	window = zMapViewGetWindow(zmap->focus_viewwindow) ;
	view_state = zMapViewGetStatus(view) ;

	switch(view_state)
	  {
	  case ZMAPVIEW_INIT:
	  case ZMAPVIEW_MAPPED:
	    reload = TRUE ;
	    break ;
	  case ZMAPVIEW_CONNECTING:
	  case ZMAPVIEW_CONNECTED:
	  case ZMAPVIEW_LOADING:
	  case ZMAPVIEW_UPDATING:
	    stop = TRUE ;
	    if(!zMapViewGetFeatures(view))       /* can revcomp one column while others are arriving */
	      break ;

	  case ZMAPVIEW_LOADED:
            if(view_state == ZMAPVIEW_LOADED)
	      revcomp = TRUE;
	    general = TRUE ;
            frame3 = TRUE;
            dna = TRUE;
	    if(window_filter)
	      zmap->filter = *window_filter; /* struct copy */
	    else
	      zmap->filter.enable = FALSE;

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

  gtk_widget_set_sensitive(zmap->filter_but, zmap->filter.enable) ;

  {
    //	  GtkEntry *entry = (GtkEntry *) zmap->filter_but;
    GtkAdjustment *adj = gtk_spin_button_get_adjustment ((GtkSpinButton *) zmap->filter_but);
    double range, step;
    double min;
    //	  guint digits = 0;

    /* this is the only place we set the step value */
    /* CanvasFeaturesets remember the current value */
    range = zmap->filter.max - zmap->filter.min;
    step = 1.0;

    while(range / step > 1000.0)
      step *= 10.0;

    while (range / step < 10.0)
      {
	step /= 10;
	//		  digits++;
      }

    if(step < 5.0)
      step = 5.0;

    /* style min and max relate to display not feature scores, we can filter on score less than style min
     * we flag filtering if features are hidden, not if we are on the min score
     */
    min = 0.0;
    if(zmap->filter.min < min)
      zmap->filter.min = min;

    if(zmap->filter.value < min)
      zmap->filter.value = min;
    if(zmap->filter.value > zmap->filter.max)
      zmap->filter.value = zmap->filter.max;

    gtk_adjustment_configure(adj,zmap->filter.value, min, zmap->filter.max, step, 0, 0);

    //	  if(digits)	/*  seems to be interpreted as a vast number */
    //		gtk_spin_button_set_digits ((GtkSpinButton *) zmap->filter_but, digits);

    filterSetHighlight(zmap);
  }

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



static GtkWidget *newSpinButton(void)
{
  GtkWidget *spinner = NULL ;
  GtkAdjustment *spinner_adj ;

  /* default to integer % */
  spinner_adj = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0, 100.0, 1.0, 0.0, 0.0) ;
  spinner = gtk_spin_button_new (spinner_adj, 1.0, 0) ;

  return spinner ;
}


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



/* handle text input and spin buttons */
static void filterValueChangedCB(GtkSpinButton *spinbutton, gpointer user_data)
{
  ZMap zmap = (ZMap) user_data;
  double value;
  
  /* if we don't do this then with a busy column we get the column updateds but the spin button is delayed
   * so we don't get inertaction with the filter score.  It's better this way.
   */
  if(zmap->filter_spin_pressed)
    return;
  
  value = gtk_spin_button_get_value(spinbutton);
  //printf("filter value %f %f\n", zmap->filter.value, value);
  zmap->filter.value = value;

  if(zmap->filter.func)
    {
      ZMapView zmap_view = zMapViewGetView(zmap->focus_viewwindow);
      gboolean highlight_filtered_columns = zMapViewGetHighlightFilteredColumns(zmap_view);
      zmap->filter.n_filtered = zmap->filter.func(&zmap->filter, value, highlight_filtered_columns);
    }
  
  filterSetHighlight(zmap);
}



/* this does not work */
static gboolean filterSpinButtonCB(GtkWidget *spin, GdkEvent *event, gpointer user_data)
{
  gboolean handled = FALSE ;
  ZMap zmap = (ZMap) user_data;

  //printf("filter spin %d\n", event->type == GDK_BUTTON_PRESS ? 1 : event->type == GDK_BUTTON_RELEASE ? 2 : 0);

  switch(event->type)
    {
    case GDK_BUTTON_PRESS:
      zmap->filter_spin_pressed = TRUE;

      break;

    case GDK_BUTTON_RELEASE:
      zmap->filter_spin_pressed = FALSE;
      filterValueChangedCB((GtkSpinButton *) spin, user_data);
      break;

    default:
      handled = FALSE;
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

  zMapWindow3FrameToggle(window) ;

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
    do_aa        = FALSE,
    do_trans = FALSE ;

  zMapAssert(zmap->focus_viewwindow) ;

  window = zMapViewGetWindow(zmap->focus_viewwindow) ;

  zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, do_trans, force_to, force);

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


static void backButtonCB(GtkWidget *wigdet, gpointer data)
{
  ZMap zmap = (ZMap)data;
  ZMapWindow window = NULL;

  window = zMapViewGetWindow(zmap->focus_viewwindow);

  zMapWindowBack(window);

  return ;
}




/*
 *   Zoom menu stuff....
 */

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



/*
 *    Sequence menu stuff (currently this is for translation only).
 */

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

	      zMapAssert(zmap->focus_viewwindow) ;

	      window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	      makeSequenceMenu(button_ev, window) ;

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

static void makeSequenceMenu(GdkEventButton *button_event, ZMapWindow window)
{
  char *menu_title = "" ;
  GList *menu_sets = NULL ;
  static ZMapGUIMenuSubMenuData sub_data = NULL;

  menu_title = "3 Frame menu" ;

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
    {
      sub_data->original_data = window;
    }

  /* Make up the menu. */
  makeMenuSequenceOps(window, &menu_sets, NULL, NULL, sub_data) ;

  zMapGUIMakeMenu(menu_title, menu_sets, button_event) ;

  return ;
}


static ZMapGUIMenuItem makeMenuSequenceOps(ZMapWindow window,
                                           GList **all_menus,
                                           int *start_index_inout,
                                           ZMapGUIMenuItemCallbackFunc callback_func,
                                           gpointer callback_data)
{
  static ZMapGUIMenuItemStruct aa_menu[] =
    {
      {ZMAPGUI_MENU_NORMAL, "None",                           HIDE_3ALL,      seqMenuCB,      NULL},
      {ZMAPGUI_MENU_NORMAL, "Features",                       SHOW_3FEATURES, seqMenuCB,      NULL},
      {ZMAPGUI_MENU_NORMAL, "3 Frame Translation",            SHOW_3FT,       seqMenuCB,      NULL},
      {ZMAPGUI_MENU_NORMAL, "Features + 3 Frame Translation", SHOW_3ALL,      seqMenuCB,      NULL},
      {ZMAPGUI_MENU_NONE,   NULL,                             0,              NULL,           NULL}
    };

  zMapGUIPopulateMenu(aa_menu, start_index_inout, callback_func, callback_data);
  *all_menus = g_list_append(*all_menus, aa_menu);

  return NULL ;
}


static void seqMenuCB(int menu_item_id, gpointer callback_data)
{
  ZMapGUIMenuSubMenuData data = (ZMapGUIMenuSubMenuData)callback_data ;
  ZMapWindow window = NULL ;
  GQuark align_id = 0, block_id = 0 ;
  gboolean force = TRUE, force_to = FALSE, do_dna = FALSE, do_aa = FALSE, do_trans = FALSE ;

  /* MH17: I took the force and do_aa flags out of 3FT options as they are not currently relevant
   * there had been experiment with operating DNA and 3FT in tandem
   * but we prefer the buttons to do 'what they say on the can'
   */

  align_id = data->align_unique_id ;
  block_id = data->block_unique_id ;

  switch(menu_item_id)
    {
    case SHOW_DNA:
      force_to = TRUE;
      do_dna   = TRUE;
      break;
    case HIDE_DNA:
      do_dna   = TRUE;
      break;
    case SHOW_3FEATURES:
    case SHOW_3ALL:
    case SHOW_3FT:
    case HIDE_3ALL:
      break;
    default:
      break;
    }

  /* What on earth is this test about ????? */
  if ((window = data->original_data))
    {
      if (menu_item_id == SHOW_3FEATURES
	  || menu_item_id == SHOW_3FT || menu_item_id == HIDE_3ALL
	  || menu_item_id == SHOW_3ALL)
	{
	  ZMapWindow3FrameMode frame_mode ;

	  switch(menu_item_id)
	    {
	    case HIDE_3ALL:
	      frame_mode = ZMAP_WINDOW_3FRAME_INVALID ;
	      break;
	    case SHOW_3FEATURES:
	      frame_mode = ZMAP_WINDOW_3FRAME_COLS ;
	      break ;
	    case SHOW_3FT:
	      frame_mode = ZMAP_WINDOW_3FRAME_TRANS ;
	      break;
	    case SHOW_3ALL:
	      frame_mode = ZMAP_WINDOW_3FRAME_ALL ;
	      break ;
	    default:
	      zMapAssertNotReached() ;
	      break;
	    }

	  zMapWindow3FrameSetMode(window, frame_mode) ;
	}
      else
	{
	  /* DNA... */
	  zMapWindowToggleDNAProteinColumns(window, align_id, block_id, do_dna, do_aa, do_trans, force_to, force) ;
	}
    }

  return ;
}





