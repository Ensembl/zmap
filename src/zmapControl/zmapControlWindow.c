/*  File: zmapControlWindow.c
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Creates the top level window of a ZMap.
 *
 * Exported functions: See zmapTopWindow_P.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <string.h>

#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>



static void setTooltips(ZMap zmap) ;
static void makeStatusTooltips(ZMap zmap) ;
static GtkWidget *makeStatusPanel(ZMap zmap) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;

static void myWindowMaximize(GtkWidget *toplevel, ZMap zmap) ;



gboolean zmap_shrink_G = FALSE;




/* 
 *                  Package External routines.
 */


/* Makes the toplevel window and control panels for an individual zmap. */
gboolean zmapControlWindowCreate(ZMap zmap)
{
  gboolean result = TRUE ;
  GtkWidget *toplevel, *vbox, *menubar, *frame, *controls_box, *button_box, *status_box,
    *info_panel_box, *info_box ;

  /* Make tooltips groups for the main zmap controls and the feature information. */
  zmap->tooltips = gtk_tooltips_new() ;
  zmap->feature_tooltips = gtk_tooltips_new() ;

  zmap->toplevel = toplevel = zMapGUIToplevelNew(zMapGetZMapID(zmap), NULL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), zmap_shrink_G, TRUE, FALSE ) ;	// allow shrink for charlie'ss RT 215415
								/* ref to GTK help: it says 'don't allow shrink' */
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;

  /* Only after map-event are we guaranteed that there's a window for us to work with. */
  g_signal_connect(G_OBJECT(toplevel), "map-event",
                   G_CALLBACK(zmapControlRemoteInstaller), (gpointer)zmap);

  /* We can leave width to default sensibly but height does not because zmap is in a scrolled
   * window, we try to maximise it to the screen depth but have to do this after window is mapped.
   * SHOULD WE BE REMOVING THIS HANDLER ??? TAKE A LOOK AT THIS LATER.... */
  zmap->map_handler = g_signal_connect(G_OBJECT(toplevel), "map",
				       G_CALLBACK(myWindowMaximize), (gpointer)zmap);

  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
		     GTK_SIGNAL_FUNC(toplevelDestroyCB), (gpointer)zmap) ;


  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  zmap->event_box_parent = vbox ;

  menubar = zmapControlWindowMakeMenuBar(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

  zmap->button_info_box = controls_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), controls_box) ;

  info_box = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(controls_box), info_box, FALSE, TRUE, 0);

  button_box = zmapControlWindowMakeButtons(zmap) ;
  gtk_box_pack_start(GTK_BOX(info_box), button_box, FALSE, TRUE, 0);

  status_box = makeStatusPanel(zmap) ;
  gtk_box_pack_end(GTK_BOX(info_box), status_box, FALSE, TRUE, 0) ;


  //info_panel_box = zmapControlWindowMakeInfoPanel(zmap) ;
  info_panel_box = zmap->info_panel_vbox = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(controls_box), info_panel_box, FALSE, FALSE, 0) ;

  zmap->navview_frame = zmapControlWindowMakeFrame(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), zmap->navview_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;

  /* Tooltips can only be added to widgets after the widgets have been "shown". */
  setTooltips(zmap) ;
  gtk_tooltips_enable(zmap->tooltips) ;


  return result ;
}



void zmapControlWindowDestroy(ZMap zmap)
{
  /* We must disconnect the "destroy" callback otherwise we will enter toplevelDestroyCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(zmap->toplevel), (gpointer)zmap) ;

  gtk_widget_destroy(zmap->toplevel) ;

  gtk_object_destroy(GTK_OBJECT(zmap->tooltips)) ;
  gtk_object_destroy(GTK_OBJECT(zmap->feature_tooltips)) ;

  return ;
}



/* Called to update the user information sections of the zmapControl sections
 * of the display. Currently sets forward/revcomp status and coords. */
void zmapControlWindowSetStatus(ZMap zmap)
{
  char *status_text ;
  char *free_this = NULL;

  switch(zmap->state)
    {
    case ZMAP_DYING:
      {
	status_text = "Dying...";
	break ;
      }
    case ZMAP_VIEWS:
      {
	ZMapView view ;
	ZMapWindow window ;
	gboolean revcomped ;
	char *strand_txt ;
	int start = 0, end = 0 ;
	char *coord_txt = "" ;

	zMapAssert(zmap->focus_viewwindow) ;

	view = zMapViewGetView(zmap->focus_viewwindow) ;

	window = zMapViewGetWindow(zmap->focus_viewwindow) ;

	revcomped = zMapViewGetRevCompStatus(view) ;
	if (revcomped)
	  strand_txt = " - " ;
	else
	  strand_txt = " + " ;
	gtk_label_set_text(GTK_LABEL(zmap->status_revcomp), strand_txt) ;


#if NO_SCOPE_FOR_IMPROVEMENT
//	if (zMapViewGetFeaturesSpan(view, &start, &end))
	  {
	    zmapWindowCoordPairToDisplay(window,
//					 start, end,
					 window->min_coord,window->max_coord,
					 &start, &end) ;
#else
      if(zmapWindowGetCurrentSpan(window,&start,&end))
	{
#endif
          coord_txt = g_strdup_printf(" %d  %d ", start, end) ;
          gtk_label_set_text(GTK_LABEL(zmap->status_coords), coord_txt) ;
          g_free(coord_txt) ;
	}

	free_this = status_text = zMapViewGetStatusStr(view) ;

	break ;
      }
    case ZMAP_INIT:
    default:
      {
	status_text = "" ;
	break ;
      }
    }

  gtk_entry_set_text(GTK_ENTRY(zmap->status_entry), status_text) ;

#if UPDATES_SIBLING_WINDOWS  /* thought to be due to there being one event queue only */

  /* display instantly, else we don't update due to other stuff using idle time */
  /* except it doesn't work */
  {
      GdkWindow * win;

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 14
            win = zmap->status_entry->window ;
#else
      win = gtk_widget_get_window(zmap->status_entry);
#endif
      if(win)
           gdk_window_process_updates(win,TRUE);
  }
#endif

  if (free_this)
    g_free(status_text) ;

  return ;
}






/*
 *  ------------------- Internal functions -------------------
 */


/* this panel displays revcomp, sequence coord and status information for
 * the selected view.
 *
 * Note that the labels have to be parented by an event box because we want
 * them to have tooltips which require a window in order to work and this
 * is provided by the event box. */
static GtkWidget *makeStatusPanel(ZMap zmap)
{
  GtkWidget *status_box, *frame, *event_box ;

  status_box = gtk_hbox_new(FALSE, 0) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  event_box = gtk_event_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), event_box) ;
  zmap->status_revcomp = gtk_label_new(NULL) ;
  gtk_container_add(GTK_CONTAINER(event_box), zmap->status_revcomp) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  event_box = gtk_event_box_new() ;
  gtk_container_add(GTK_CONTAINER(frame), event_box) ;
  zmap->status_coords = gtk_label_new(NULL) ;
  gtk_container_add(GTK_CONTAINER(event_box), zmap->status_coords) ;

  frame = gtk_frame_new(NULL) ;
  gtk_box_pack_start(GTK_BOX(status_box), frame, TRUE, TRUE, 0) ;
  zmap->status_entry = gtk_entry_new();
  gtk_container_add(GTK_CONTAINER(frame), zmap->status_entry) ;

  return status_box ;
}



/* Called when a destroy signal has been sent to the top level window/widget,
 * this may have been by the user clicking a button or maybe by zmapControl
 * code. */
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmap->toplevel = NULL ;

  zmapControlDoKill(zmap) ;

  return ;
}



/* Note that tool tips cannot be set on a widget until that widget has a window,
 * so this routine gets called after the various widgets have been realised.
 * This is a bit of pity as it means we end up splitting creation of widgets from
 * creation of their tooltips. */
static void setTooltips(ZMap zmap)
{

  zmapControlButtonTooltips(zmap) ;

  makeStatusTooltips(zmap) ;


  return ;
}



static void makeStatusTooltips(ZMap zmap)
{

  gtk_tooltips_set_tip(zmap->tooltips, gtk_widget_get_parent(zmap->status_revcomp),
		       "\"+\" = forward complement,\n \"-\"  = reverse complement",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, gtk_widget_get_parent(zmap->status_coords),
		       "start/end coords of displayed sequence",
		       "") ;

  gtk_tooltips_set_tip(zmap->tooltips, zmap->status_entry,
		       "Status of selected view",
		       "") ;

  return ;
}


/* Users have asked us to make the zmap window as big as possible vertically, the horizontal size
 * is not usually an issue, we can set it small and then gtk will expand it to accomodate all the
 * buttons etc along the top of the window which results in a good horizontal size.
 * 
 * We only do this on the first expose, after that it's up to the user to make the window 
 * the size they want.
 * 
 */
static void myWindowMaximize(GtkWidget *toplevel, ZMap zmap)
{
  zMapGUIMaximiseWindow(toplevel) ;

  /* Disconnect otherwise this will be done every tmie the window is re-exposed. */
  g_signal_handler_disconnect(zmap->toplevel, zmap->map_handler) ;

  return ;
}


