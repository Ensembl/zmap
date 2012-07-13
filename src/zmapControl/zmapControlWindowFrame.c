/*  File: zmapWindowFrame.c
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 * Exported functions: See XXXXXXXXXXXXX.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







#include <zmapControl_P.h>


static void createNavViewWindow(ZMap zmap, GtkWidget *parent) ;

#if USE_REGION
static void valueCB(void *user_data, double start, double end) ;
#endif

static void pane_position_callback(GObject *pane, GParamSpec *scroll, gpointer user_data);

static gboolean double_to_open(GtkWidget *widget, GdkEventButton *button, gpointer user_data);

GtkWidget *zmapControlWindowMakeFrame(ZMap zmap)
{
  GtkWidget *frame ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_border_width(GTK_CONTAINER(frame), 5) ;

  createNavViewWindow(zmap, frame) ;

#if USE_REGION
  zMapNavigatorSetWindowCallback(zmap->navigator, valueCB, (void *)zmap) ;
#endif

  return frame ;
}

/* createNavViewWindow ***************************************************************
 * Creates the root node in the panesTree (which helps keep track of all the
 * display panels).  The root node has no data, only children.
 */
static void createNavViewWindow(ZMap zmap, GtkWidget *parent)
{
  GtkWidget *nav_top ;

  /* Navigator and views are in an hpane, so the user can adjust the width
   * of the navigator and views. */
  zmap->hpane = gtk_hpaned_new() ;
  gtk_container_add(GTK_CONTAINER(parent), zmap->hpane) ;

  /* Make the navigator which shows user where they are on the sequence. */
  zmap->navigator = zMapNavigatorCreate(&nav_top, &(zmap->nav_canvas)) ;
  gtk_paned_pack1(GTK_PANED(zmap->hpane), nav_top, FALSE, TRUE) ;


  /* This box contains what may be multiple views in paned widgets. */
  zmap->pane_vbox = gtk_vbox_new(FALSE,0) ;

  gtk_paned_pack2(GTK_PANED(zmap->hpane),
		  zmap->pane_vbox, TRUE, TRUE);


  /* Set left hand (sliders) pane closed by default. */
  gtk_paned_set_position(GTK_PANED(zmap->hpane), 0) ;

  g_signal_connect(GTK_OBJECT(zmap->hpane), "button-press-event", GTK_SIGNAL_FUNC(double_to_open), zmap);

  zMapGUIPanedSetMaxPositionHandler(zmap->hpane, G_CALLBACK(pane_position_callback), zmap);

  gtk_widget_show_all(zmap->hpane);

  return ;
}


#if USE_REGION
/* Gets called by navigator when user has moved window locator scroll bar. */
static void valueCB(void *user_data, double start, double end)
{
  ZMap zmap = (ZMap)user_data ;

  if (zmap->state == ZMAP_VIEWS)
    {
      ZMapWindow window = zMapViewGetWindow(zmap->focus_viewwindow) ;

      zMapWindowMove(window, start, end) ;
    }

  return ;
}
#endif

static void pane_position_callback(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMap zmap = (ZMap)user_data;
  gint pos, max;

  /* we need to get the position... */
  pos = gtk_paned_get_position(GTK_PANED(pane));

  /* find the position of the navigator pane + the width of the navigator canvas. */
  max = zMapNavigatorGetMaxWidth(zmap->navigator);

  if(max < pos)
    gtk_paned_set_position(GTK_PANED(pane), max);

  return ;
}

static gboolean double_to_open(GtkWidget *widget, GdkEventButton *button, gpointer user_data)
{
  gboolean handled = FALSE;

  if(GTK_IS_PANED(widget))
    {
      switch(button->type)
	{
	case GDK_2BUTTON_PRESS:
	  {
	    GtkPaned *pane = GTK_PANED(widget);
	    if(button->window == pane->handle)
	      {
		ZMap zmap = (ZMap)user_data;
		int max_position = zMapNavigatorGetMaxWidth(zmap->navigator);
		int cur_position = gtk_paned_get_position(pane);

		if(gdk_pointer_is_grabbed())
		  {
		    gdk_pointer_ungrab(button->time);
		    /* have to poke around in here ;( */
		    pane->in_drag = FALSE;
		  }

		if(cur_position < max_position)
		  gtk_paned_set_position(pane, max_position);
		else if(cur_position >= max_position)
		  gtk_paned_set_position(pane, 0);

#ifdef DIFFERENCE
		g_object_set(G_OBJECT(pane),
			     "position",     max_position,
			     "position_set", TRUE,
			     NULL);
#endif

		handled = TRUE;
	      }
	  }
	  break;
	default:
	  break;
	}
    }

  return handled;
}

