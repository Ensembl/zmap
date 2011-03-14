/*  File: zmapWindowFrame.c
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 19 16:55 2004 (edgrif)
 * Created: Thu Apr 29 11:06:06 2004 (edgrif)
 * CVS info:   $Id: zmapWindowFrame.c,v 1.9 2011-03-14 11:35:18 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







/* WARNING, WARNING, THIS FILE IS NOT USED NOW AND IS PROBABLY DEFUNCT NOW............. */



#include <stdlib.h>
#include <stdio.h>
#include <zmapWindow_P.h>



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define GNOME_WINDOW
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef GNOME_WINDOW
#include <libgnomecanvas/libgnomecanvas.h>
#endif /*GNOME_WINDOW*/


extern int test_global, test_overlap ;


static GtkWidget *makeCanvas(void) ;







GtkWidget *zmapWindowMakeFrame(ZMapWindow window)
{
  GtkWidget *frame ;
  GtkWidget *vbox ;
  GtkWidget *scrwin ;

  gchar *frame_title ;

  frame_title = g_strdup_printf("Sequence : %s", window->sequence->sequence) ;

  frame = gtk_frame_new(frame_title);
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.0, 0.0 );
  gtk_container_border_width(GTK_CONTAINER(frame), 5);

  g_free(frame_title) ;

  vbox = gtk_vbox_new(FALSE, 0) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ROB THIS ALL NEEDS SORTING OUT, I THINK THE FRAME HIERACHY IS ALL OUT OF WHACK...*/

  zMapWindowSetVbox(window, vbox) ;
  zMapWindowSetBorderWidth(zMapWindowGetVbox(window), 5);
  gtk_container_add (GTK_CONTAINER (frame), zMapWindowGetVbox(window));
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





  scrwin = gtk_scrolled_window_new(NULL, NULL) ;
  gtk_container_add(GTK_CONTAINER(vbox), scrwin) ;


  /* CODE TO CREATE THE ACTUAL ZMAP SHOULD GO HERE INSTEAD OF THIS DUMMY DATA.......... */
    window->text = makeCanvas() ;
    gtk_container_add(GTK_CONTAINER(scrwin), window->text) ;


  return frame ;
}



/*
 * WARNING: this is hacked up testing code only......
 */



#ifndef GNOME_WINDOW

static GtkWidget *makeCanvas(void)
{
  GtkWidget *text ;

  text = gtk_text_view_new() ;
  gtk_widget_set_usize(text, 500, 500) ;

  return text ;
}

#else

static void makeItems(GtkWidget *canvas) ;

static GtkWidget *makeCanvas(void)
{
  GtkWidget *canvas ;

  canvas = gnome_canvas_new() ;

  gnome_canvas_set_scroll_region(GNOME_CANVAS(canvas), 0, 0, 1999, 1999) ;

  gtk_widget_set_usize(canvas, 500, 500) ;

  makeItems(canvas) ;

  return canvas ;
}



static void makeItems(GtkWidget *canvas)
{
  float draw_incr ;
  GnomeCanvasGroup* root_group ;
  GnomeCanvasItem* item ;
  int i, j, items ;
  double x, y ;

  root_group = gnome_canvas_root(GNOME_CANVAS(canvas)) ;

  items = test_global ;

  if (test_overlap)
    draw_incr = 0.0 ;
  else
    draw_incr = 10.0 ;


  for (i = 0, x = 0.0 ; i < items ; i++, x += draw_incr)
    {
      for (j = 0, y = 0.0 ; j < items ; j++, y += draw_incr)
	{
	  item = gnome_canvas_item_new(root_group,
				       GNOME_TYPE_CANVAS_RECT,
				       "outline_color", "black",
				       "x1", x, "y1", y,
				       "x2", x + 5.0, "y2", y + 5.0,
				       NULL) ;
	}

    }

  return ;
}

#endif /*GNOME_WINDOW*/
