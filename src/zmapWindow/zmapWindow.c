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
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 14 17:40 2003 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.2 2003-11-14 17:40:45 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <zmapWindow_P.h>


static void quitCB(GtkWidget *widget, gpointer cb_data) ;
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;



ZMapWindow zMapWindowCreate(char *machine, int port, char *sequence, 
			    zmapVoidIntCallbackFunc manager_routine, void *manager_data)
{
  ZMapWindow window ;
  GtkWidget *toplevel, *vbox, *menubar, *button_frame, *connect_frame ;
  char *title ;

  window = g_new(ZMapWindowStruct, sizeof(ZMapWindowStruct)) ;

  window->machine = g_strdup(machine) ;
  window->port = port ;
  window->sequence = g_strdup(sequence) ;
  window->manager_routine = manager_routine ;
  window->manager_data = manager_data ;
  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;

  title = g_strdup_printf("ZMap (%s, port %d): %s", machine, port,
			  sequence ? sequence : "") ;

  window->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)window) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "client_event", 
		     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapWindowMakeMenuBar(window) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  button_frame = zmapWindowMakeButtons(window) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_frame, FALSE, TRUE, 0);

  connect_frame = zmapWindowMakeFrame(window) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;


  g_free(title) ;


  return(window) ;
}



/* This routine is called by the code that manages the slave threads, it makes
 * the call to tell the GUI code that there is something to do. This routine
 * then sends this event to alert the GUI that it needs to do some work and
 * will supply the data via the event struct. */
void zMapWindowSignalData(ZMapWindow window, void *data)
{
  GdkEventClient event ;
  GdkAtom zmap_atom ;
  gint ret_val = 0 ;
  zmapWindowData window_data ;

  /* Set up struct to be passed to our callback. */
  window_data = g_new(zmapWindowDataStruct, sizeof(zmapWindowDataStruct)) ;
  window_data->window = window ;
  window_data->data = data ;

  event.type = GDK_CLIENT_EVENT ;
  event.window = NULL ;					    /* no window generates this event. */
  event.send_event = TRUE ;				    /* we sent this event. */
  event.message_type = window->zmap_atom ;		    /* This is our id for events. */
  event.data_format = 8 ;				    /* Not sure about data format here... */

  /* Load the pointer value, not what the pointer points to.... */
  {
    void **dummy ;

    dummy = (void *)&window_data ;
    memmove(&(event.data.b[0]), dummy, sizeof(void *)) ;
  }

  gtk_signal_emit_by_name(GTK_OBJECT(window->toplevel), "client_event",
			  &event, &ret_val) ;

  return ;
}



void zMapWindowDestroy(ZMapWindow window)
{
  ZMAP_DEBUG(("GUI: in window destroy...\n")) ;

  /* We must disconnect the "destroy" callback otherwise we will enter quitCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(window->toplevel), (gpointer)window) ;

  gtk_widget_destroy(window->toplevel) ;

  g_free(window->machine) ;
  g_free(window->sequence) ;

  g_free(window) ;

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMapWindow window = (ZMapWindow)cb_data ;

  (*(window->manager_routine))(window->manager_data, ZMAP_WINDOW_QUIT) ;

  return ;
}



/* Called when gtk detects the event sent by signalDataToGUI(), in the end this
 * routine will call zmap routines to display data etc. */
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  ZMapWindow window = (ZMapWindow)cb_data ;

  if (event->type != GDK_CLIENT_EVENT)
    ZMAPERR("dataEventCB() received non-GdkEventClient event") ;
  
  if (event->send_event == TRUE && event->message_type == gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      char *string = NULL ;

      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;

      window = window_data->window ;
      string = (char *)(window_data->data) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ZMAP_DEBUG(("GUI: got dataEvent, contents: \"%s\"\n", string)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_text_buffer_insert(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			     NULL, string, -1) ;	    /* -1 => insert whole string. */

      g_free(string) ;
      g_free(window_data) ;
    }
  else
    {
      ZMAP_DEBUG(("unknown client event in zmapevent handler\n")) ;
    }


  return ;
}
