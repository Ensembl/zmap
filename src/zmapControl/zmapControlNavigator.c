/*  File: zmapControlNavigator.c
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
name
 *
 * Description: Implements the navigator window in a zmap.
 *              We tried using a scale widget but you can't set the size
 *              of the thumb with this widget which is useless so for
 *              now we are using the scrollbar which looks naff...sigh.
 *
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Aug  1 12:24 2007 (rds)
 * Created: Thu Jul  8 12:54:27 2004 (edgrif)
 * CVS info:   $Id: zmapControlNavigator.c,v 1.35 2011-03-14 11:35:17 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <math.h>
#include <gtk/gtk.h>
#include <zmapNavigator_P.h>


#define TOPTEXT_NO_SCALE "<no data>"
#define BOTTEXT_NO_SCALE ""



static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data);
static void canvas_value_cb(gpointer user_data, double top, double bottom);
static void canvas_width_cb(gpointer user_data, double left, double right);


static ZMapWindowNavigatorCallbackStruct control_nav_cbs_G = {
  canvas_value_cb, canvas_width_cb
};

/* Create an instance of the navigator, this currently has two scroll bars,
 * one to show the position of the region in its parent assembly and
 * one to show the position of the window within the canvas.
 * The function returns the navigator instance but also its top container
 * widget in top_widg_out. */
ZMapNavigator zMapNavigatorCreate(GtkWidget **top_widg_out, GtkWidget **canvas_out)
{
  ZMapNavigator navigator ;
  GtkObject *adjustment ;
  GtkWidget *pane, *label,
    *locator_frame  = NULL,
    *locator_canvas = NULL,
    *locator_vbox   = NULL,
    *locator_label  = NULL,
    *locator_sw     = NULL;

  if((navigator = g_new0(ZMapNavStruct, 1)))
    {
      navigator->pane = pane = gtk_hpaned_new() ;

      /* Construct the region locator. */

      /* Need a vbox so we can add a label with sequence size at the bottom later,
       * we set it to a fixed width so that the text is always visible. */
      navigator->navVBox = gtk_vbox_new(FALSE, 0);
      gtk_paned_add1(GTK_PANED(pane), navigator->navVBox) ;

      label = gtk_label_new("Region") ;
      gtk_box_pack_start(GTK_BOX(navigator->navVBox), label, FALSE, TRUE, 0);

      navigator->topLabel = gtk_label_new(TOPTEXT_NO_SCALE) ;
      gtk_box_pack_start(GTK_BOX(navigator->navVBox), navigator->topLabel, FALSE, TRUE, 0);


      /* Make the navigator with a default, "blank" adjustment obj. */
      adjustment = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ;

      navigator->navVScroll = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustment)) ;
      gtk_box_pack_start(GTK_BOX(navigator->navVBox), navigator->navVScroll, TRUE, TRUE, 0) ;

      /* Note how we pack the label at the end of the vbox and set "expand" to FALSE so that it
       * remains small and the vscale expands to fill the rest of the box. */
      navigator->botLabel = gtk_label_new(BOTTEXT_NO_SCALE) ;
      gtk_box_pack_end(GTK_BOX(navigator->navVBox), navigator->botLabel, FALSE, TRUE, 0);


      /* Construct the window locator ... */
      locator_vbox  = gtk_vbox_new(FALSE, 0);

      /* A label */
      locator_label = gtk_label_new("Scroll Navigator") ;
      gtk_box_pack_start(GTK_BOX(locator_vbox), locator_label, FALSE, TRUE, 0);

      /* A frame */
      locator_frame = gtk_frame_new(NULL);
      gtk_frame_set_shadow_type(GTK_FRAME(locator_frame), GTK_SHADOW_NONE);
      gtk_box_pack_start(GTK_BOX(locator_vbox), locator_frame, TRUE, TRUE, 0);

      /* A canvas */
      locator_canvas = navigator->locator_widget =
        zMapWindowNavigatorCreateCanvas(&control_nav_cbs_G, navigator);

      locator_sw = gtk_scrolled_window_new(NULL, NULL);
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(locator_sw),
                                     GTK_POLICY_ALWAYS, GTK_POLICY_NEVER);
      gtk_container_add(GTK_CONTAINER(locator_sw), locator_canvas);
      gtk_container_add(GTK_CONTAINER(locator_frame), locator_sw);
      gtk_container_set_border_width(GTK_CONTAINER(locator_sw), 0);
      gtk_container_set_border_width(GTK_CONTAINER(locator_frame), 0);

      /* pack into the pane */
      gtk_paned_add2(GTK_PANED(pane), locator_vbox) ;

      /* Set left hand (region view) pane closed by default. */
      gtk_paned_set_position(GTK_PANED(pane), 0) ;

      g_object_connect(G_OBJECT(pane),
                       "signal::notify::position",
                       G_CALLBACK(paneNotifyPositionCB),
                       (gpointer)navigator,
                       NULL);

      if(canvas_out)
        *canvas_out = locator_canvas;
      if(top_widg_out)
        *top_widg_out = pane ;
    }

  return navigator ;
}


/* Set function + data that Navigator will call each time the position of the region window
 * is changed. */
void zMapNavigatorSetWindowCallback(ZMapNavigator navigator,
				    ZMapNavigatorScrollValue cb_func, void *user_data)
{
  navigator->cb_func = cb_func ;
  navigator->user_data = user_data ;

  return ;
}

/* zmapControl.c:388 calls this! */


/* Set the window adjuster to match the Max Window size of the underlying canvas. This is needed
 * because with a long sequence it is not possible to zoom the whole sequence down to the level
 * of bases without exceeding the maximum window size of X Windows.
 * We need to watch for when the scroll bar shrinks and at this point open the window
 * slider so the user sees it.....then they can use this to navigate when we can't show the whole
 * sequence in one window.
 * Returns an integer which is the width in pixels of the window scrollbar. */
int zMapNavigatorSetWindowPos(ZMapNavigator navigator, double top_pos, double bot_pos)
{
  gboolean always_maximised = FALSE;
  int pane_width = 0 ;
  int seq_start = 0, seq_end = 0;

  seq_start = navigator->sequence_span.x1;
  seq_end   = navigator->sequence_span.x2;

  if(always_maximised || top_pos > seq_start || bot_pos < seq_end)
    pane_width = zMapNavigatorGetMaxWidth(navigator);

  return pane_width;
}



/* updates size/range to coords of the new view. */
void zMapNavigatorSetView(ZMapNavigator navigator, ZMapFeatureContext features,
			  double top, double bottom)
{
  GtkObject *region_adjuster;
  gchar *region_top_str, *region_bot_str, *window_top_str, *window_bot_str ;
  region_adjuster = (GtkObject *)gtk_range_get_adjustment(GTK_RANGE(navigator->navVScroll)) ;

  /* May be called with no sequence to parent mapping so must set default navigator for this. */
  if (features)
    {
      navigator->parent_span = features->parent_span ;	    /* n.b. struct copy. */
      navigator->sequence_span = features->master_align->sequence_span;

      region_top_str = g_strdup_printf("%d", navigator->parent_span.x1) ;
      region_bot_str = g_strdup_printf("%d", navigator->parent_span.x2) ;

      /* MH17: this was confused and worked as parent span was equal to the window/ sequence
       * we have no data outside the sequence so no point in scrolling to it
       * however with multiple sparse blocks we do actually want to use the parent span
       * but NOTE with multiple aligns possibly from diff chromosomes or species
       * none of this makes obvious sense
       * maybe we need to set up the navigator with ref to the master align
       * (This needs further thought)
       *
       * for the moment I'm setting up as parent span = range of navigator
       * and initial value middle of the master align
       * with multiple blocks this could result in a featureless screen
       */

      GTK_ADJUSTMENT(region_adjuster)->value = (((gdouble)(navigator->sequence_span.x1
						    + navigator->sequence_span.x2)) / 2.0) ;

      GTK_ADJUSTMENT(region_adjuster)->lower = (gdouble)navigator->sequence_span.x1 ;
      GTK_ADJUSTMENT(region_adjuster)->upper = (gdouble)navigator->sequence_span.x2 ;

      GTK_ADJUSTMENT(region_adjuster)->step_increment = 10.0 ;	    /* step incr, wild guess... */
      GTK_ADJUSTMENT(region_adjuster)->page_increment = 1000 ;	    /* page incr, wild guess... */
      GTK_ADJUSTMENT(region_adjuster)->page_size = (gdouble)(fabs(navigator->sequence_span.x2
								  - navigator->sequence_span.x1) + 1) ;

      window_top_str = g_strdup_printf("%d", navigator->sequence_span.x1) ;
      window_bot_str = g_strdup_printf("%d", navigator->sequence_span.x2) ;

      zMapNavigatorSetWindowPos(navigator, top, bottom) ;
    }
  else
    {
      navigator->parent_span.x1 = navigator->parent_span.x2 = 0 ;
      navigator->sequence_span.x1 = navigator->sequence_span.x2 = 0 ;

      region_top_str = g_strdup(TOPTEXT_NO_SCALE) ;
      region_bot_str = g_strdup(BOTTEXT_NO_SCALE) ;

      GTK_ADJUSTMENT(region_adjuster)->value = 0.0 ;
      GTK_ADJUSTMENT(region_adjuster)->lower = 0.0 ;
      GTK_ADJUSTMENT(region_adjuster)->upper = 0.0 ;
      GTK_ADJUSTMENT(region_adjuster)->step_increment = 0.0 ;
      GTK_ADJUSTMENT(region_adjuster)->page_increment = 0.0 ;
      GTK_ADJUSTMENT(region_adjuster)->page_size = 0.0 ;

      window_top_str = g_strdup(TOPTEXT_NO_SCALE) ;
      window_bot_str = g_strdup(BOTTEXT_NO_SCALE) ;

    }

  gtk_adjustment_changed(GTK_ADJUSTMENT(region_adjuster)) ;

  gtk_label_set_text(GTK_LABEL(navigator->topLabel), region_top_str) ;
  gtk_label_set_text(GTK_LABEL(navigator->botLabel), region_bot_str) ;

  g_free(region_top_str) ;
  g_free(region_bot_str) ;

  return ;
}

int zMapNavigatorGetMaxWidth(ZMapNavigator navigator)
{
  int handle_size = 0;

  gtk_widget_style_get(navigator->pane, "handle-size", &handle_size, NULL);

  return handle_size +
    navigator->left_pane_width +
    navigator->right_pane_width;
}

/* Destroys a navigator instance, note there is not much to do here because we
 * assume that our caller will destroy our parent widget will in turn destroy
 * all our widgets. */
void zMapNavigatorDestroy(ZMapNavigator navigator)
{
  g_free(navigator) ;

  return ;
}



/*
 *              Internal functions
 */

/* This used to be GtkAdjustment *, gpointer user_data... */
static void canvas_value_cb(gpointer user_data, double top, double bottom)
{
  ZMapNavigator navigator = (ZMapNavigator)user_data;

  if (navigator->cb_func)
    (*(navigator->cb_func))(navigator->user_data, top, bottom) ;

  return ;
}

static void canvas_width_cb(gpointer user_data, double left, double right)
{
  ZMapNavigator navigator = (ZMapNavigator)user_data;

  navigator->right_pane_width = (int)(right - left) + 1;

  return ;
}

static void paneNotifyPositionCB(GObject *pane, GParamSpec *scroll, gpointer user_data)
{
  ZMapNavigator navigator = (ZMapNavigator)user_data;
  int pos;

  pos = gtk_paned_get_position(GTK_PANED(navigator->pane));

  navigator->left_pane_width = pos;

  return ;
}

