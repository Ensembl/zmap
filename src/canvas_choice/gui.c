/*  File: gui.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 10 12:10 2009 (rds)
 * Created: Fri Mar 16 09:20:26 2007 (rds)
 * CVS info:   $Id: gui.c,v 1.3 2010-03-04 14:49:04 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <gui.h>
#ifdef RDS_DONT_INCLUDE
static void toplevelDestroyCB(GtkWidget *widget, gpointer cb_data);
#endif /* RDS_DONT_INCLUDE */
static void quitCB(GtkWidget *button, gpointer user_data);


demoGUI demoGUICreate(char *title, demoGUICallbackSet callbacks, gpointer user_data)
{
  demoGUI gui;
  GtkWidget *toplevel, *vbox, *zoom_in, *zoom_out, 
    *exit, *canvas_container, *canvas, *button_bar,
    *frame, *draw_random;

  if((gui = g_new0(demoGUIStruct, 1)))
    {
      /* create toplevel */
      gui->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE);
      gtk_window_set_title(GTK_WINDOW(toplevel), title);
      gtk_container_border_width(GTK_CONTAINER(toplevel), DEMO_PACK_SIZE);
      
      g_signal_connect(G_OBJECT(toplevel), "destroy", 
                       G_CALLBACK(quitCB), (gpointer)gui) ;
      /* create a vbox */
      gui->vbox = vbox = gtk_vbox_new(FALSE, 0) ;
      gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

      /* create buttons */
      button_bar  = gtk_hbutton_box_new();
      draw_random = gtk_button_new_with_label("Random Items");
      zoom_in     = gtk_button_new_with_label("Zoom In");
      zoom_out    = gtk_button_new_with_label("Zoom Out");
      exit        = gtk_button_new_with_label("Exit");

      gtk_container_add(GTK_CONTAINER(button_bar), draw_random);
      gtk_container_add(GTK_CONTAINER(button_bar), zoom_in);
      gtk_container_add(GTK_CONTAINER(button_bar), zoom_out);
      gtk_container_add(GTK_CONTAINER(button_bar), exit);

      gtk_box_pack_start(GTK_BOX(vbox), button_bar, FALSE, FALSE, DEMO_PACK_SIZE);

      /* create container for canvas */
      canvas_container = frame = gtk_frame_new(title);
      gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, DEMO_PACK_SIZE);
      
      /* create canvas */
      if((canvas = (callbacks->create_canvas)(canvas_container, user_data)))
        {
          GdkColor bg;
          GtkWidget *scrolled_window;

          scrolled_window = gtk_scrolled_window_new(NULL, NULL);
          gtk_container_add(GTK_CONTAINER(canvas_container), scrolled_window);

          gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);
          gdk_color_parse("white", &bg);
          gtk_widget_modify_bg(GTK_WIDGET(canvas), GTK_STATE_NORMAL, &bg) ;

        }

      /* add event handlers to buttons */
      g_signal_connect(G_OBJECT(draw_random), "clicked",
                       G_CALLBACK(callbacks->draw_random_items), user_data);
      g_signal_connect(G_OBJECT(zoom_in), "clicked",
                       G_CALLBACK(callbacks->zoom_in), user_data);
      g_signal_connect(G_OBJECT(zoom_out), "clicked",
                       G_CALLBACK(callbacks->zoom_out), user_data);

      g_signal_connect(G_OBJECT(exit), "clicked",
                       G_CALLBACK(quitCB), gui);


      /* record destroy callback and user_data */
      gui->user_destroy_notify = callbacks->data_destroy_notify;
      gui->user_data = user_data;

      /* show it all! */
      gtk_widget_show_all(toplevel);
    }

  return gui;
}

static void quitCB(GtkWidget *button, gpointer user_data)
{
  demoGUI gui = (demoGUI)user_data;

  if(gui->user_data && gui->user_destroy_notify)
    (gui->user_destroy_notify)(gui->user_data);

  g_free(gui);

  demoUtilsExit(EXIT_SUCCESS);

  return ;
}
