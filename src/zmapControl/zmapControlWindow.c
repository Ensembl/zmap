/*  File: zmapControlWindow.c
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
 * Description: Creates the top level window of a ZMap.
 *              
 * Exported functions: See zmapTopWindow_P.h
 * HISTORY:
 * Last edited: Dec 20 16:33 2006 (edgrif)
 * Created: Fri May  7 14:43:28 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindow.c,v 1.30 2006-12-21 12:08:45 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>

static void setTooltips(ZMap zmap) ;
static void makeStatusTooltips(ZMap zmap) ;
static GtkWidget *makeStatusPanel(ZMap zmap) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;

static void myWindowMaximize(GtkWidget *toplevel, ZMap zmap) ;


/* Makes the toplevel window and control panels for an individual zmap. */
gboolean zmapControlWindowCreate(ZMap zmap)
{
  gboolean result = TRUE ;
  GtkWidget *toplevel, *vbox, *menubar, *frame, *controls_box, *button_box, *status_box,
    *info_panel_box, *info_box ;


  /* Make tooltips groups for the main zmap controls and the feature information. */
  zmap->tooltips = gtk_tooltips_new() ;
  zmap->feature_tooltips = gtk_tooltips_new() ;


  zmap->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), zmap->zmap_id) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;


  g_signal_connect(G_OBJECT(toplevel), "realize",
                   G_CALLBACK(zmapControlRemoteInstaller), (gpointer)zmap);

  /* We can leave width to default sensibly but height does not because zmap is in a scrolled
   * window, we try to maximise it to the screen depth but have to do this after window is mapped.
   * SHOULD WE BE REMOVING THIS HANDLER ??? TAKE A LOOK AT THIS LATER.... */
  zmap->map_handler = g_signal_connect(G_OBJECT(toplevel), "map",
				       G_CALLBACK(myWindowMaximize), (gpointer)zmap);


  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)zmap) ;


  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapControlWindowMakeMenuBar(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);

  controls_box = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(frame), controls_box) ;

  info_box = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(controls_box), info_box, FALSE, TRUE, 0);

  button_box = zmapControlWindowMakeButtons(zmap) ;
  gtk_box_pack_start(GTK_BOX(info_box), button_box, FALSE, TRUE, 0);

  status_box = makeStatusPanel(zmap) ;
  gtk_box_pack_end(GTK_BOX(info_box), status_box, FALSE, TRUE, 0) ;

  info_panel_box = zmapControlWindowMakeInfoPanel(zmap) ;
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
  /* We must disconnect the "destroy" callback otherwise we will enter quitCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(zmap->toplevel), (gpointer)zmap) ;

  gtk_widget_destroy(zmap->toplevel) ;

  gtk_object_destroy(GTK_OBJECT(zmap->tooltips)) ;
  gtk_object_destroy(GTK_OBJECT(zmap->feature_tooltips)) ;

  return ;
}



/* Currently only sets forward/revcomp status. */
void zmapControlWindowSetStatus(ZMap zmap)
{
  char *status_text ;


  switch(zmap->state)
    {
    case ZMAP_DYING:
      {
	status_text = "Dying..." ;
	break ;
      }
    case ZMAP_VIEWS:
      {
	ZMapView view ;
	ZMapViewState view_state ;
	gboolean revcomped ;
	char *strand_txt ;
	int start = 0, end = 0 ;
	char *coord_txt = "" ;

	zMapAssert(zmap->focus_viewwindow) ;

	view = zMapViewGetView(zmap->focus_viewwindow) ;

	revcomped = zMapViewGetRevCompStatus(view) ;
	if (revcomped)
	  strand_txt = " - " ;
	else
	  strand_txt = " + " ;
	gtk_label_set_text(GTK_LABEL(zmap->status_revcomp), strand_txt) ;

	if (zMapViewGetFeaturesSpan(view, &start, &end))
	  {
	    coord_txt = g_strdup_printf(" %d  %d ", start, end) ;
	    gtk_label_set_text(GTK_LABEL(zmap->status_coords), coord_txt) ;
	    g_free(coord_txt) ;
	  }

	view_state = zMapViewGetStatus(view) ;
	status_text = zMapViewGetStatusStr(view_state) ;

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
  zmap->status_entry = gtk_entry_new() ;
  gtk_container_add(GTK_CONTAINER(frame), zmap->status_entry) ;

  return status_box ;
}


/* Probably needs renaming to exitCB, Need to check with Ed as not
 * 100% on all possiblities, esp threads... */
static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  /* this is hacky, I don't like this, if we don't do this then end up trying to kill a
   * non-existent top level window because gtk already seems to have done this...  */
  zmap->toplevel = NULL ; 
  
  zmapControlTopLevelKillCB(zmap) ;

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



/* This code was taken from following the code through in gtk_maximise_window() to gdk_window_maximise() etc.
 * But it doesn't work that reliably....under KDE it does the right thing, but not under GNOME
 * where the window is maximised in both directions.
 * 
 * If the window manager supports the _NET_WM_ stuff then we use that to set the window max
 * as it will automatically take into account any menubars etc. created by the window manager.
 * 
 * Otherwise we are back to guessing some kind of size.
 * If someone displays a really short piece of dna this will make the window
 * too big so really we should readjust the window size to fit the sequence
 * but this will be rare.
 * 
 */
static void myWindowMaximize(GtkWidget *toplevel, ZMap zmap)
{
  GdkAtom max_atom_vert ;
  GdkScreen *screen ;

  /* This all needs the tests/tidying that the gtk routines have.... */
  max_atom_vert = gdk_atom_intern("_NET_WM_STATE_MAXIMIZED_VERT", FALSE) ;

  screen = gtk_widget_get_screen(toplevel) ;

  if (gdk_x11_screen_supports_net_wm_hint(screen, max_atom_vert))
    {
      /* We construct an event that the window manager will see that will cause it to correctly
       * maximise the window. */

      GtkWindow *gtk_window = GTK_WINDOW(toplevel) ;
      GdkDisplay *display = gtk_widget_get_display(toplevel) ;
      GdkWindow *window = toplevel->window ;
      GdkWindow *root_window = gtk_widget_get_root_window(toplevel) ;
      XEvent xev ;

      if (gtk_window->frame)
	window = gtk_window->frame;
      else
	window = toplevel->window ;
      
      xev.xclient.type = ClientMessage;
      xev.xclient.serial = 0;
      xev.xclient.send_event = True;
      xev.xclient.window = GDK_WINDOW_XID (window);
      xev.xclient.message_type = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE");
      xev.xclient.format = 32 ;
      xev.xclient.data.l[0] = TRUE ;
      xev.xclient.data.l[1] = gdk_x11_atom_to_xatom_for_display (display, max_atom_vert) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* undefine for a window maximised in both directions.... */
      xev.xclient.data.l[2] = gdk_x11_atom_to_xatom_for_display (display, max_atom_horz) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      xev.xclient.data.l[2] = 0 ;
      xev.xclient.data.l[3] = 0;
      xev.xclient.data.l[4] = 0;
  
      XSendEvent(GDK_WINDOW_XDISPLAY(window),
		 GDK_WINDOW_XID(root_window),
		 False,
		 SubstructureRedirectMask | SubstructureNotifyMask,
		 &xev) ; 
    }
  else
    {
      /* OK, here we just guess some appropriate size, note that the window width is kind
       * of irrelevant, we just set it to be a bit less than it will finally be and the
       * widgets will resize it to the correct width. We don't use gtk_window_set_default_size() 
       * because it doesn't seem to work. */
      int window_width_guess = 300, window_height_guess ;

      window_height_guess = (int)((float)(gdk_screen_get_height(screen)) * ZMAPWINDOW_VERT_PROP) ;

      gtk_window_resize(GTK_WINDOW(toplevel), window_width_guess, window_height_guess) ;
    }

  /* I think we need to disconnect this now otherwise we reset the window height every time we
   * are re-mapped. */
  g_signal_handler_disconnect (zmap->toplevel, zmap->map_handler) ;


  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void my_gtk_window_maximize (GtkWindow *window)
{
  GtkWidget *widget;
  GdkWindow *toplevel;
  
  g_return_if_fail (GTK_IS_WINDOW (window));

  widget = GTK_WIDGET (window);

  window->maximize_initially = TRUE;

  if (window->frame)
    toplevel = window->frame;
  else
    toplevel = widget->window;
  
  if (toplevel != NULL)
    my_gdk_window_maximize (toplevel);
}


static void my_gdk_window_maximize(GdkWindow *window)
{
  g_return_if_fail (GDK_IS_WINDOW (window));

  if (GDK_WINDOW_DESTROYED (window))
    return;

  if (GDK_WINDOW_IS_MAPPED (window))
    gdk_wmspec_change_state (TRUE, window,
			     gdk_atom_intern ("_NET_WM_STATE_MAXIMIZED_VERT", FALSE),
			     gdk_atom_intern ("_NET_WM_STATE_MAXIMIZED_HORZ", FALSE));
  else
    gdk_synthesize_window_state (window,
				 0,
				 GDK_WINDOW_STATE_MAXIMIZED);
}



static void
gdk_wmspec_change_state (gboolean   add,
			 GdkWindow *window,
			 GdkAtom    state1,
			 GdkAtom    state2)
{
  GdkDisplay *display = GDK_WINDOW_DISPLAY (window);
  XEvent xev;
  
#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */  
  
  xev.xclient.type = ClientMessage;
  xev.xclient.serial = 0;
  xev.xclient.send_event = True;
  xev.xclient.window = GDK_WINDOW_XID (window);
  xev.xclient.message_type = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_STATE");
  xev.xclient.format = 32;
  xev.xclient.data.l[0] = add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
  xev.xclient.data.l[1] = gdk_x11_atom_to_xatom_for_display (display, state1);
  xev.xclient.data.l[2] = gdk_x11_atom_to_xatom_for_display (display, state2);
  xev.xclient.data.l[3] = 0;
  xev.xclient.data.l[4] = 0;
  
  XSendEvent (GDK_WINDOW_XDISPLAY (window), GDK_WINDOW_XROOTWIN (window), False,
	      SubstructureRedirectMask | SubstructureNotifyMask,
	      &xev);
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





