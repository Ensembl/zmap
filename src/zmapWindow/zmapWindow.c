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
 * Description: Provides interface functions for controlling a data
 *              display window.
 *              
 * Exported functions: See ZMap/zmapWindow.h
 * HISTORY:
 * Last edited: Jul  2 19:06 2004 (edgrif)
 * Created: Thu Jul 24 14:36:27 2003 (edgrif)
 * CVS info:   $Id: zmapWindow.c,v 1.15 2004-07-02 18:24:53 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapFeature.h>
#include <zmapWindow_P.h>
#include <zmapDraw.h>


static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer data) ;



ZMapWindow zMapWindowCreate(GtkWidget *parent_widget, char *sequence,
			    zmapVoidIntCallbackFunc app_routine, void *app_data)
{
  ZMapWindow window ;
  GtkWidget *toplevel, *vbox, *menubar, *button_frame, *connect_frame ;

  window = g_new(ZMapWindowStruct, sizeof(ZMapWindowStruct)) ;

  if (sequence)
    window->sequence = g_strdup(sequence) ;
  window->app_routine = app_routine ;
  window->app_data = app_data ;
  window->zmap_atom = gdk_atom_intern(ZMAP_ATOM, FALSE) ;
  window->parent_widget = parent_widget ;

  /* Make the vbox the toplevel for now, it gets sent the client event, 
   * we may also want to register a "destroy" callback at some time as this
   * may be useful. */
  window->toplevel = toplevel = vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(window->parent_widget), vbox) ;
  gtk_signal_connect(GTK_OBJECT(vbox), "client_event", 
		     GTK_SIGNAL_FUNC(dataEventCB), (gpointer)window) ;

  connect_frame = zmapWindowMakeFrame(window) ;
  gtk_box_pack_start(GTK_BOX(vbox), connect_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;

  return(window) ;
}



/* This routine is called by the code that manages the slave threads, it makes
 * the call to tell the GUI code that there is something to do. This routine
 * does this by sending a synthesized event to alert the GUI that it needs to
 * do some work and supplies the data for the GUI to process via the event struct. */
void zMapWindowDisplayData(ZMapWindow window, void *data)
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


void zMapWindowReset(ZMapWindow window)
{

  /* Dummy code, need code to blank the window..... */

  zMapDebug("%s", "GUI: in window reset...\n") ;

  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			   "", -1) ;			    /* -1 => null terminated string. */

  return ;
}


void zMapWindowDestroy(ZMapWindow window)
{
  zMapDebug("%s", "GUI: in window destroy...\n") ;

  gtk_widget_destroy(window->toplevel) ;

  if (window->sequence)
    g_free(window->sequence) ;

  g_free(window) ;

  return ;
}



/*
 *  ------------------- Internal functions -------------------
 */



/* Called when gtk detects the event sent by signalDataToGUI(), this routine calls
 * the data display routines of ZMap. */
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  ZMapWindow window = (ZMapWindow)cb_data ;

  if (event->type != GDK_CLIENT_EVENT)
    zMapLogFatal("%s", "dataEventCB() received non-GdkEventClient event") ;
  
  if (event->send_event == TRUE && event->message_type == gdk_atom_intern(ZMAP_ATOM, TRUE))
    {
      zmapWindowData window_data = NULL ;
      ZMapWindow window = NULL ;
      gpointer data = NULL ;
      ZMapFeatureContext feature_context ;

      /* Retrieve the data pointer from the event struct */
      memmove(&window_data, &(event->data.b[0]), sizeof(void *)) ;

      window = window_data->window ;

      data = (gpointer)(window_data->data) ;


      /* Can either get data from my dummied up GFF routine or if you set up an acedb server
       * you can get data from there.... just undef the one you want... */

      //#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      feature_context = testGetGFF() ;			    /* Data read from a file... */
      //#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	//      feature_context = (ZMapFeatureContext)data ;	    /* Data from a server... */


      /* ****Remember that someone needs to free the data passed over....****  */


      /* <<<<<<<<<<<<<  ROB, this is where calls to your drawing code need to go  >>>>>>>>>>>> */

      zmapWindowDrawFeatures(feature_context) ;


      g_free(window_data) ;				    /* Free the WindowData struct. */
    }
  else
    {
      zMapLogCritical("%s", "unknown client event in dataEventCB() handler\n") ;
    }


  return ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Previous dummy routine...... */

/* Called when gtk detects the event sent by signalDataToGUI(), in the end this
 * routine will call zmap routines to display data etc. */
static void dataEventCB(GtkWidget *widget, GdkEventClient *event, gpointer cb_data)
{
  ZMapWindow window = (ZMapWindow)cb_data ;

  if (event->type != GDK_CLIENT_EVENT)
    zMapLogFatal("%s", "dataEventCB() received non-GdkEventClient event") ;
  
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
      zmapDebug("%s", "GUI: got dataEvent, contents: \"%s\"\n", string) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Dummy test code.... */
      gtk_text_buffer_insert(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			     NULL, string, -1) ;	    /* -1 => null terminated string. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->text)),
			       string, -1) ;		    /* -1 => null terminated string. */

      g_free(string) ;
      g_free(window_data) ;
    }
  else
    {
      zMapDebug("%s", "unknown client event in zmapevent handler\n") ;
    }


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
STORE_HANDLE zMapWindowGetHandle(ZMapWindow window)
{
  return window->handle;
}


void zMapWindowSetHandle(ZMapWindow window)
{
  //  window->handle = handleCreate();
  window->handle = NULL;
  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void zMapWindowSetBorderWidth(GtkWidget *container, int width)
{
  gtk_container_border_width(GTK_CONTAINER(container), width);
  return;
}


ScreenCoord zMapWindowGetScaleOffset(ZMapWindow window)
{
  return window->scaleOffset;
}


void zMapWindowSetScaleOffset(ZMapWindow window, ScreenCoord offset)
{
  window->scaleOffset = offset;
  return;
}


Coord zMapWindowGetCoord(ZMapWindow window, char *field)
{
  if (field == "s")
    return window->navStart;
  else
    return window->navEnd;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapWindowSetCoord(ZMapWindow window, char *field, int size)
{
  if (field == "s")
    window->navStart = window->focuspane->zMapRegion->area1 - size;
  else
    window->navEnd = window->focuspane->zMapRegion->area2 + size;
  
  if (window->navStart == window->navEnd)
    window->navEnd = window->navStart + 1;
  
  return;
}


ScreenCoord zMapWindowGetScreenCoord1(ZMapWindow window, int height)
{
  return zMapWindowGetScreenCoord(window, window->focuspane->zMapRegion->area1, height);
}


ScreenCoord zMapWindowGetScreenCoord2(ZMapWindow window, int height)
{
  return zMapWindowGetScreenCoord(window, window->focuspane->zMapRegion->area2, height);
}

ScreenCoord zMapWindowGetScreenCoord(ZMapWindow window, Coord coord, int height)
{
  return height * (coord - window->navStart) / (window->navEnd - window->navStart);
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


InvarCoord zMapWindowGetOrigin(ZMapWindow window)
{
  return window->origin;
}


// ZMapRegion functions

GPtrArray *zMapRegionNewMethods(ZMapRegion *region)
{
  return region->methods = g_ptr_array_new();
}

GPtrArray *zMapRegionGetMethods(ZMapRegion *region)
{
  return region->methods;
}

GPtrArray *zMapRegionGetOldMethods(ZMapRegion *region)
{
  return region->oldMethods;
}

void zMapRegionFreeMethods(ZMapRegion *region)
{
  g_ptr_array_free(region->methods, TRUE);
  return;
}

void zMapRegionFreeOldMethods(ZMapRegion *region)
{
  g_ptr_array_free(region->oldMethods, TRUE);
  return;
}

GArray *zMapRegionNewSegs(ZMapRegion *region)
{
  return region->segs = g_array_new(FALSE, FALSE, sizeof(ZMapFeatureStruct)) ;
}

GArray *zMapRegionGetSegs(ZMapRegion *region)
{
  return region->segs;
}

void zMapRegionFreeSegs(ZMapRegion *region)
{
  free(region->segs);
  return;
}


GArray *zMapRegionGetDNA(ZMapRegion *region)
{
  return region->dna;
}

void zMapRegionFreeDNA(ZMapRegion *region)
{
  g_array_free(region->dna, TRUE);
  return;
}


/****************** end of file ************************************/
