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
 * Last edited: Jul 20 15:13 2004 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapControlWindowButtons.c,v 1.11 2004-07-21 08:43:45 edgrif Exp $
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

/* This lot may need to go into a separate file sometime to give more general purpose dialog code. */
static char *getSequenceName(void) ;
static void okCB(GtkWidget *widget, gpointer cb_data) ;
static void cancelCB(GtkWidget *widget, gpointer cb_data) ;
typedef struct
{
  GtkWidget *dialog ;
  GtkWidget *entry ;
  char *new_sequence ;
} ZmapSequenceCallbackStruct, *ZmapSequenceCallback ;





GtkWidget *zmapControlWindowMakeButtons(ZMap zmap)
{
  GtkWidget *frame ;
  GtkWidget *hbox, *new_button, *load_button, *stop_button, *quit_button,
    *hsplit_button, *vsplit_button, *zoomin_button, *zoomout_button,
    *close_button ;

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
  /*
  close_button = gtk_button_new_with_label("Close\nPane") ;
  gtk_signal_connect(GTK_OBJECT(close_button), "clicked",
		     GTK_SIGNAL_FUNC(closePane), (gpointer)zmap) ;
  gtk_box_pack_start(GTK_BOX(hbox), close_button, FALSE, FALSE, 0) ;
  */
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
static void testfunc(GQuark key_id, gpointer data, gpointer user_data);

/* These callbacks simply make calls to routines in zmapControl.c, this is because I want all
 * the state handling etc. to be in one file so that its easier to work on. */
static void loadCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapFeatureContext feature_context ;
  GtkWidget *topWindow;
  char *title;

  zmapControlLoadCB(zmap) ;

  return ;
}

static void stopCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlResetCB(zmap) ;

  return ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void newCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  /* The string is for testing only, in the full code there should be a dialog box that enables
   * the user to provide a whole load of info. */
  zmapControlNewCB(zmap, "Y38H6C") ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static void newCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;
  ZMapPane pane ;
  GtkWidget *parent_widget = NULL ;
  ZMapView view ;
  ZMapViewWindow view_window ;
  char *new_sequence ;

  /* THIS ROUTINE AND THE WHOLE PANE/VIEW INTERFACE NEEDS SOME MAJOR TIDYING UP..... */

  /* Get a new sequence to show.... */
  if ((new_sequence = getSequenceName()))
    {

      /* this all needs to do view stuff here and return a parent widget............ */
      parent_widget = splitPane(zmap) ;

      pane = zmap->focuspane ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* At this point split func does the add call to add a window to an existing view, we want
       * to add a new view and add a window to that..... */
      view_window = zMapViewAddWindow(zMapViewGetView(pane->curr_view_window), parent_widget) ;

      pane->curr_view_window = view_window ;		    /* new focus window ?? */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* THIS USED TO BE HERE AND WE SHOULD GO BACK TO USING IT BUT ALL THIS NEEDS TIDYING
       * UP FIRST.....ESPECIALLY THE PANE STUFF.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      zmapControlNewCB(zmap, "Y38H6C") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* this code cut/pasted from addView() in zmapcontrol...needs clearing up.... */

      if ((view = zMapViewCreate(new_sequence, (void *)zmap))
	  && (view_window = zMapViewAddWindow(view, pane->view_parent_box))
	  && zMapViewConnect(view))
	{
	  /* add to list of views.... */
	  zmap->view_list = g_list_append(zmap->view_list, view) ;
      
	  zmap->focuspane->curr_view_window = view_window ;

	  zmap->firstTime = FALSE ;

	  zmap->state = ZMAP_VIEWS ;


	  /* Look in focus pane and set title bar/navigator etc......really this should all be
	   * in one focus routine......which we need to remove from zmapAddPane..... */


	  /* We've added a view so better update everything... */
	  gtk_widget_show_all(zmap->toplevel) ;
	}
    }


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



static char *getSequenceName(void)
{
  char *sequence_name = NULL ;
  GtkWidget *dialog, *label, *text, *ok_button, *cancel_button ;
  ZmapSequenceCallbackStruct cb_data = {NULL, NULL} ;

  cb_data.dialog = dialog = gtk_dialog_new() ;
 
  gtk_window_set_title(GTK_WINDOW(dialog), "ZMap - Enter New Sequence") ;

  label = gtk_label_new("Sequence Name:") ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0) ;

  cb_data.entry = text = gtk_entry_new() ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), text, TRUE, TRUE, 0) ;

  ok_button = gtk_button_new_with_label("OK") ;
  GTK_WIDGET_SET_FLAGS(ok_button, GTK_CAN_DEFAULT) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), ok_button, FALSE, FALSE, 0) ;
  gtk_widget_grab_default(ok_button) ;
  gtk_signal_connect(GTK_OBJECT(ok_button), "clicked", GTK_SIGNAL_FUNC(okCB), &cb_data) ;

  cancel_button = gtk_button_new_with_label("Cancel") ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), cancel_button, FALSE, FALSE, 0) ;
  gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked", GTK_SIGNAL_FUNC(cancelCB), &cb_data) ;

  gtk_widget_show_all(dialog) ;

  gtk_main() ;

  sequence_name = cb_data.new_sequence ;

  return sequence_name ;
}


static void okCB(GtkWidget *widget, gpointer cb_data)
{
  ZmapSequenceCallback mydata = (ZmapSequenceCallback)cb_data ;
  char *text ;

  if ((text = (char *)gtk_entry_get_text(GTK_ENTRY(mydata->entry)))
      && *text)						    /* entry returns "" for no string. */
    mydata->new_sequence = g_strdup(text) ;

  gtk_widget_destroy(mydata->dialog) ;

  gtk_main_quit() ;

  return ;
}

static void cancelCB(GtkWidget *widget, gpointer cb_data)
{
  ZmapSequenceCallback mydata = (ZmapSequenceCallback)cb_data ;

  gtk_widget_destroy(mydata->dialog) ;

  gtk_main_quit() ;

  return ;
}
