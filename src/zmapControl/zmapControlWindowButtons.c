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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: Implement the buttons on the zmap.
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Jul  9 17:43 2004 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowButtons.c,v 1.8 2004-07-14 09:07:09 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapWindow.h>
#include <zmapControl_P.h>



static void newCB(GtkWidget *widget, gpointer cb_data) ;
static void loadCB(GtkWidget *widget, gpointer cb_data) ;
static void stopCB(GtkWidget *widget, gpointer cb_data) ;
static void zoomInCB(GtkWindow *widget, gpointer cb_data) ;
static void zoomOutCB(GtkWindow *widget, gpointer cb_data) ;
static void splitPaneCB(GtkWidget *widget, gpointer data) ;
static void splitHPaneCB(GtkWidget *widget, gpointer data) ;
static void quitCB(GtkWidget *widget, gpointer cb_data) ;



GtkWidget *zmapControlWindowMakeButtons(ZMap zmap)
{
  GtkWidget *frame ;
  GtkWidget *hbox, *new_button, *load_button, *stop_button, *quit_button,
    *hsplit_button, *vsplit_button, *zoomin_button, *zoomout_button ;

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  load_button = gtk_button_new_with_label("Load") ;
  gtk_signal_connect(GTK_OBJECT(load_button), "clicked",
		     GTK_SIGNAL_FUNC(loadCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), load_button, FALSE, FALSE, 0) ;

  stop_button = gtk_button_new_with_label("Stop") ;
  gtk_signal_connect(GTK_OBJECT(stop_button), "clicked",
		     GTK_SIGNAL_FUNC(stopCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), stop_button, FALSE, FALSE, 0) ;

  new_button = gtk_button_new_with_label("New") ;
  gtk_signal_connect(GTK_OBJECT(new_button), "clicked",
		     GTK_SIGNAL_FUNC(newCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), new_button, FALSE, FALSE, 0) ;

  hsplit_button = gtk_button_new_with_label("H-Split");
  gtk_signal_connect(GTK_OBJECT(hsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(splitPaneCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), hsplit_button, FALSE, FALSE, 0) ;

  vsplit_button = gtk_button_new_with_label("V-Split");
  gtk_signal_connect(GTK_OBJECT(vsplit_button), "clicked",
		     GTK_SIGNAL_FUNC(splitHPaneCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), vsplit_button, FALSE, FALSE, 0) ;
                                                                                           
  zoomin_button = gtk_button_new_with_label("Zoom In");
  gtk_signal_connect(GTK_OBJECT(zoomin_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomInCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomin_button, FALSE, FALSE, 0) ;
                                                                                           
  zoomout_button = gtk_button_new_with_label("Zoom Out");
  gtk_signal_connect(GTK_OBJECT(zoomout_button), "clicked",
		     GTK_SIGNAL_FUNC(zoomOutCB), (gpointer)zmap);
  gtk_box_pack_start(GTK_BOX(hbox), zoomout_button, FALSE, FALSE, 0) ;
                                                                                           
  quit_button = gtk_button_new_with_label("Quit") ;
  gtk_signal_connect(GTK_OBJECT(quit_button), "clicked",
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), quit_button, FALSE, FALSE, 0) ;

  /* Make Load button the default, its probably what user wants to do mostly. */
  GTK_WIDGET_SET_FLAGS(load_button, GTK_CAN_DEFAULT) ;
  gtk_window_set_default(GTK_WINDOW(zmap->toplevel), load_button) ;


  return frame ;
}




/*
 *  ------------------- Internal functions -------------------
 */

/* These callbacks simple make calls to routines in zmapControl.c, this is because I want all
 * the state handling etc. to be in one file so that its easier to work on. */
static void loadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapFeatureContext feature_context ;
  GtkWidget *topWindow;
  char *title;

  zmapControlLoadCB(zmap) ;

  // only here temporarily until we find out why it's not working in zmapWindow.c
  feature_context = testGetGFF() ;			    /* Data read from a file... */


  /* ROB, THIS MAY NEED TO BE REWRITTEN, WHICH SEQUENCE NAME SHOULD GO IN THE WINDOW TITLE
   * IF MULTIPLE SEQUENCES ARE DISPLAYED ? */
  title = g_strdup_printf("ZMap - %s", feature_context->sequence) ;
  gtk_window_set_title(GTK_WINDOW(zmap->toplevel), title) ;

  zmapWindowDrawFeatures(feature_context) ;

  return ;
}

static void stopCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlResetCB(zmap) ;

  return ;
}

static void newCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  /* The string is for testing only, in the full code there should be a dialog box that enables
   * the user to provide a whole load of info. */
  zmapControlNewCB(zmap, "eds_sequence") ;

  return ;
}

static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlTopLevelKillCB(zmap) ;

  return ;
}

void zoomInCB(GtkWindow *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapPane pane = zmap->focuspane ;

  zMapWindowZoom(zMapViewGetWindow(pane->curr_view_window), 2.0) ;

  return;
}


void zoomOutCB(GtkWindow *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapPane pane = zmap->focuspane ;

  zMapWindowZoom(zMapViewGetWindow(pane->curr_view_window), 0.5) ;

  return;
}


static void splitPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;
  ZMapPane pane ;
  GtkWidget *parent_widget = NULL ;
  ZMapViewWindow view_window ;

  /* this all needs to do view stuff here and return a parent widget............ */
  parent_widget = splitPane(zmap) ;

  pane = zmap->focuspane ;

  view_window = zMapViewAddWindow(zMapViewGetView(pane->curr_view_window), parent_widget) ;

  pane->curr_view_window = view_window ;		    /* new focus window ?? */

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  return ;
}


static void splitHPaneCB(GtkWidget *widget, gpointer data)
{
  ZMap zmap = (ZMap)data ;
  ZMapPane pane ;
  GtkWidget *parent_widget = NULL ;
  ZMapViewWindow view_window ;

  /* this all needs to do view stuff here and return a parent widget............ */
  parent_widget = splitHPane(zmap) ;

  pane = zmap->focuspane ;

  view_window = zMapViewAddWindow(zMapViewGetView(pane->curr_view_window), parent_widget) ;

  pane->curr_view_window = view_window ;		    /* new focus window ?? */

  /* We'll need to update the display..... */
  gtk_widget_show_all(zmap->toplevel) ;

  return ;
}
