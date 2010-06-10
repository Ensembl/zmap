/*  File: zmapControlWindow.c
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Creates the top level window of a ZMap.
 *              
 * Exported functions: See zmapTopWindow_P.h
 * HISTORY:
 * Last edited: Jun 10 10:48 2010 (edgrif)
 * Created: Fri May  7 14:43:28 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindow.c,v 1.38 2010-06-10 09:52:01 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>

static void setTooltips(ZMap zmap) ;
static void makeStatusTooltips(ZMap zmap) ;
static GtkWidget *makeStatusPanel(ZMap zmap) ;
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data) ;

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
 * buttons etc along the top of the window which results in a good horizontal size. The vertical
 * size is more tricky as lots of window managers provide tool bars etc which take up screen
 * space. We try various ways to deal with this:
 * 
 * If the window manager supports _NET_WORKAREA then we get that property and use it to
 * set the height/width.
 * 
 * Otherwise if the window manager supports the _NET_WM_ stuff then we use that to set the window max
 * as it will automatically take into account any menubars etc. created by the window manager.
 * But it doesn't work that reliably....under KDE it does the right thing, but _not_ under GNOME
 * where the window is maximised in both directions which is very annoying. Later versions of
 * GNOME do work correctly though.
 *
 * Otherwise we are back to guessing some kind of size.
 * If someone displays a really short piece of dna this will make the window
 * too big so really we should readjust the window size to fit the sequence
 * but this will be rare.
 * 
 */
static void myWindowMaximize(GtkWidget *toplevel, ZMap zmap)
{
  GdkAtom geometry_atom, workarea_atom, max_atom_vert ;
  GdkScreen *screen ;


  /* Get the atoms for _NET_* properties. */
  geometry_atom = gdk_atom_intern("_NET_DESKTOP_GEOMETRY", FALSE) ;
  workarea_atom = gdk_atom_intern("_NET_WORKAREA", FALSE) ;
  max_atom_vert = gdk_atom_intern("_NET_WM_STATE_MAXIMIZED_VERT", FALSE) ;

  screen = gtk_widget_get_screen(toplevel) ;

  if (gdk_x11_screen_supports_net_wm_hint(screen, geometry_atom)
      && gdk_x11_screen_supports_net_wm_hint(screen, workarea_atom))
    {
      /* We want to get these properties....
       *   _NET_DESKTOP_GEOMETRY(CARDINAL) = 1600, 1200
       *   _NET_WORKAREA(CARDINAL) = 0, 0, 1600, 1154, 0, 0, 1600, 1154,...repeated for all workspaces.
       * 
       * In fact we don't use the geometry (i.e. screen size) but its useful
       * to see it.
       * 
       * When retrieving 32 bit items, these items will be stored in _longs_, this means 
       * that on a 32 bit machine they come back in 32 bits BUT on 64 bit machines they
       * come back in 64 bits.
       * 
       *  */
      int window_width_guess = 300, window_height_guess ;
      gboolean result ;
      GdkWindow *root_window ;
      gulong offset, length ;
      gint pdelete = FALSE ;				    /* Never delete the property data. */
      GdkAtom actual_property_type ;
      gint actual_format, actual_length, field_size, num_fields ;
      guchar *data, *curr ;
      guint width, height, left, top, right, bottom ;

      field_size = sizeof(glong) ;			    /* see comment above re. 32 vs. 64 bits. */

      root_window = gdk_screen_get_root_window(screen) ;

      offset = 0 ;
      num_fields = 2 ;
      length = num_fields * 4 ;				    /* Get two unsigned ints worth of data. */
      actual_format = actual_length = 0 ;
      data = NULL ;
      result = gdk_property_get(root_window,
				geometry_atom,
				GDK_NONE,
				offset,
				length,
				pdelete,
				&actual_property_type,
				&actual_format,
				&actual_length,
				&data) ;

      if (num_fields == actual_length/sizeof(glong))
	{
	  curr = data ;
	  memcpy(&width, curr, field_size) ;
	  memcpy(&height, (curr += field_size), field_size) ;
	  g_free(data) ;
	}

      offset = 0 ;
      num_fields = 4 ;
      length = num_fields * 4 ;				    /* Get four unsigned ints worth of data. */
      actual_format = actual_length = 0 ;
      data = NULL ;
      result = gdk_property_get(root_window,
				workarea_atom,
				GDK_NONE,
				offset,
				length,
				pdelete,
				&actual_property_type,
				&actual_format,
				&actual_length,
				&data) ;

      if (num_fields == actual_length/sizeof(glong))
	{
	  curr = data ;
	  memcpy(&left, curr, field_size) ;
	  memcpy(&top, (curr += field_size), field_size) ;
	  memcpy(&right, (curr += field_size), field_size) ;
	  memcpy(&bottom, (curr += field_size), field_size) ;
	  g_free(data) ;
	}

      window_height_guess = bottom - top ;

      /* We now know the screen size and the work area size so we can set the window accordingly,
       * note how we set the width small knowing that gtk will make it only as big as it needs
       * to be. */
      gtk_window_resize(GTK_WINDOW(toplevel), window_width_guess, window_height_guess) ;
    }
  else if (gdk_x11_screen_supports_net_wm_hint(screen, max_atom_vert))
    {
      /* This code was taken from following the code through in gtk_maximise_window()
       * to gdk_window_maximise() etc.
       * We construct an event that the window manager will see that will cause it to correctly
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
  g_signal_handler_disconnect(zmap->toplevel, zmap->map_handler) ;


  return ;
}


