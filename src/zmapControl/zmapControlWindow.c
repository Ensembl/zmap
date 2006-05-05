/*  File: zmapControlWindow.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: May  5 11:49 2006 (rds)
 * Created: Fri May  7 14:43:28 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindow.c,v 1.24 2006-05-05 11:00:14 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>

static void setTooltips(ZMap zmap) ;
static void makeStatusTooltips(ZMap zmap) ;
static GtkWidget *makeStatusPanel(ZMap zmap) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;


/* Makes the toplevel window and control panels for an individual zmap. */
gboolean zmapControlWindowCreate(ZMap zmap)
{
  gboolean result = TRUE ;
  GtkWidget *toplevel, *vbox, *menubar, *frame, *controls_box, *button_box, *status_box, *info_box ;


  /* Make a tooltips group for the main zmap controls. */
  zmap->tooltips = gtk_tooltips_new() ;



  zmap->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), zmap->zmap_id) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;


  /* We can leave width to default sensibly but height does not because zmap is in a scrolled
   * window. */
  gtk_window_set_default_size(GTK_WINDOW(zmap->toplevel), -1, zmap->window_height) ;


  g_signal_connect(G_OBJECT(toplevel), "realize",
                   G_CALLBACK(zmapControlRemoteInstaller), (gpointer)zmap);

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

  zmap->info_panel = gtk_entry_new() ;
  gtk_box_pack_start(GTK_BOX(controls_box), zmap->info_panel, TRUE, TRUE, 0) ;

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
	ZMapStrand revcomp_strand ;
	char *strand_txt ;
	int start = 0, end = 0 ;
	char *coord_txt = "" ;

	zMapAssert(zmap->focus_viewwindow) ;

	view = zMapViewGetView(zmap->focus_viewwindow) ;

	revcomp_strand = zMapViewGetRevCompStatus(view) ;
	if (revcomp_strand == ZMAPSTRAND_FORWARD)
	  strand_txt = " + " ;
	else
	  strand_txt = " - " ;
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


