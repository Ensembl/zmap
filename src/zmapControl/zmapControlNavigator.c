/*  File: zmapControlNavigator.c
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
 * Description: Implements the navigator window in a zmap.
 *              We tried using a scale widget but you can't set the size
 *              of the thumb with this widget which is useless so for
 *              now we are using the scrollbar which looks naff...sigh.
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Jan  5 14:55 2005 (edgrif)
 * Created: Thu Jul  8 12:54:27 2004 (edgrif)
 * CVS info:   $Id: zmapControlNavigator.c,v 1.20 2005-01-07 12:13:46 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapControl_P.h>

#define TOPTEXT_NO_SCALE "<no data>"
#define BOTTEXT_NO_SCALE ""


static void valueCB(GtkAdjustment *adjustment, gpointer user_data) ;




/* AGGGGHHHHH, THIS SHOULD BE AN "OBJECT" IN ITS OWN RIGHT.....LETS TRY AND MAKE IT BETTER */

ZMapNavigator zmapControlNavigatorCreate(GtkWidget **top_widg_out)
{
  ZMapNavigator navigator ;
  GtkWidget *pane ;
  GtkObject *adjustment ;

  GtkWidget *vbox, *label, *scroll ;

  navigator = g_new0(ZMapNavStruct, 1);

  pane = gtk_hpaned_new() ;
  gtk_widget_set_size_request(pane, 100, -1) ;


  /* Construct the region locator. */

  /* Need a vbox so we can add a label with sequence size at the bottom later,
   * we set it to a fixed width so that the text is always visible. */
  navigator->navVBox = gtk_vbox_new(FALSE, 0);
  gtk_paned_add1(GTK_PANED(pane), navigator->navVBox) ;


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


  /* Construct the window locator. Note that we set the update policy to discontinuous so if
   * user drags this scroll bar the position is only reported on button release. This is
   * necessary because the underlying canvas window cannot be remapped rapidly. */
  navigator->wind_vbox = gtk_vbox_new(FALSE, 0);
  gtk_paned_add2(GTK_PANED(pane), navigator->wind_vbox) ;

  navigator->wind_top_label = gtk_label_new(TOPTEXT_NO_SCALE) ;
  gtk_box_pack_start(GTK_BOX(navigator->wind_vbox), navigator->wind_top_label, FALSE, TRUE, 0);

  adjustment = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ; /* "blank" adjustment obj. */
  gtk_signal_connect(GTK_OBJECT(adjustment), "value-changed",
		     GTK_SIGNAL_FUNC(valueCB), (void *)navigator) ;

  navigator->wind_scroll = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustment)) ;
  gtk_range_set_update_policy(GTK_RANGE(navigator->wind_scroll), GTK_UPDATE_DISCONTINUOUS) ;
  gtk_box_pack_start(GTK_BOX(navigator->wind_vbox), navigator->wind_scroll, TRUE, TRUE, 0) ;


  /* Note how we pack the label at the end of the vbox and set "expand" to FALSE so that it
   * remains small and the vscale expands to fill the rest of the box. */
  navigator->wind_bot_label = gtk_label_new(BOTTEXT_NO_SCALE) ;
  gtk_box_pack_end(GTK_BOX(navigator->wind_vbox), navigator->wind_bot_label, FALSE, TRUE, 0);

  navigator->wind_top = navigator->wind_bot = 0.0 ;


  /* Set left hand (region view) pane closed. */
  gtk_paned_set_position(GTK_PANED(pane), 0) ;

  *top_widg_out = pane ;

  return navigator ;
}

void zmapControlNavigatorSetWindowCallback(ZMapNavigator navigator,
					   ZMapNavigatorScrollValue cb_func, void *user_data)
{
  navigator->cb_func = cb_func ;
  navigator->user_data = user_data ;

  return ;
}


void zmapControlNavigatorSetWindowPos(ZMapNavigator navigator, double top_pos, double bot_pos)
{
  GtkObject *window_adjuster ;

  window_adjuster = (GtkObject *)gtk_range_get_adjustment(GTK_RANGE(navigator->wind_scroll)) ;

  GTK_ADJUSTMENT(window_adjuster)->value = top_pos ;
  GTK_ADJUSTMENT(window_adjuster)->page_size = (gdouble)(abs(bot_pos - top_pos) + 1) ;

  gtk_adjustment_changed(GTK_ADJUSTMENT(window_adjuster)) ;

  return ;
}



/* updates size/range to coords of the new view. */
void zmapControlNavigatorNewView(ZMapNavigator navigator, ZMapFeatureContext features)
{
  GtkObject *region_adjuster, *window_adjuster ;
  gchar *region_top_str, *region_bot_str, *window_top_str, *window_bot_str ;

  region_adjuster = (GtkObject *)gtk_range_get_adjustment(GTK_RANGE(navigator->navVScroll)) ;
  window_adjuster = (GtkObject *)gtk_range_get_adjustment(GTK_RANGE(navigator->wind_scroll)) ;

  /* May be called with no sequence to parent mapping so must set default navigator for this. */
  if (features)
    {
      navigator->parent_span = features->parent_span ;	    /* n.b. struct copy. */
      navigator->sequence_to_parent = features->sequence_to_parent ; /* n.b. struct copy. */

      region_top_str = g_strdup_printf("%d", navigator->parent_span.x1) ;
      region_bot_str = g_strdup_printf("%d", navigator->parent_span.x2) ;

      GTK_ADJUSTMENT(region_adjuster)->value = (((gdouble)(navigator->sequence_to_parent.p1
						    + navigator->sequence_to_parent.p2)) / 2.0) ;
      GTK_ADJUSTMENT(region_adjuster)->lower = (gdouble)navigator->parent_span.x1 ;
      GTK_ADJUSTMENT(region_adjuster)->upper = (gdouble)navigator->parent_span.x2 ;
      GTK_ADJUSTMENT(region_adjuster)->step_increment = 10.0 ;	    /* step incr, wild guess... */
      GTK_ADJUSTMENT(region_adjuster)->page_increment = 1000 ;	    /* page incr, wild guess... */
      GTK_ADJUSTMENT(region_adjuster)->page_size = (gdouble)(abs(navigator->sequence_to_parent.p2
							  - navigator->sequence_to_parent.p1) + 1) ;

      window_top_str = g_strdup_printf("%d", navigator->sequence_to_parent.c1) ;
      window_bot_str = g_strdup_printf("%d", navigator->sequence_to_parent.c2) ;

      /* I assume here that we start at the top. */
      GTK_ADJUSTMENT(window_adjuster)->value = (0.0) ;
      GTK_ADJUSTMENT(window_adjuster)->lower = (gdouble)navigator->sequence_to_parent.c1 ;
      GTK_ADJUSTMENT(window_adjuster)->upper = (gdouble)navigator->sequence_to_parent.c2 ;
      GTK_ADJUSTMENT(window_adjuster)->step_increment = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->page_increment = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->page_size = (gdouble)(abs(navigator->sequence_to_parent.c2
								 - navigator->sequence_to_parent.c1) + 1) ;
    }
  else
    {
      navigator->parent_span.x1 = navigator->parent_span.x2 = 0 ;

      navigator->sequence_to_parent.p1 = navigator->sequence_to_parent.p2
	= navigator->sequence_to_parent.c1 = navigator->sequence_to_parent.c2 = 0 ;

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

      GTK_ADJUSTMENT(window_adjuster)->value = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->lower = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->upper = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->step_increment = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->page_increment = 0.0 ;
      GTK_ADJUSTMENT(window_adjuster)->page_size = 0.0 ;
    }

  gtk_adjustment_changed(GTK_ADJUSTMENT(region_adjuster)) ;
  gtk_adjustment_changed(GTK_ADJUSTMENT(window_adjuster)) ;

  gtk_label_set_text(GTK_LABEL(navigator->topLabel), region_top_str) ;
  gtk_label_set_text(GTK_LABEL(navigator->botLabel), region_bot_str) ;

  g_free(region_top_str) ;
  g_free(region_bot_str) ;

  gtk_label_set_text(GTK_LABEL(navigator->wind_top_label), window_top_str) ;
  gtk_label_set_text(GTK_LABEL(navigator->wind_bot_label), window_bot_str) ;

  g_free(window_top_str) ;
  g_free(window_bot_str) ;

  return ;
}







void navChange(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p;
  
  /* code needed.... */

  return ;
}



void navUpdate(GtkAdjustment *adj, gpointer p)
{
  ZMapPane pane = (ZMapPane)p ;
  ZMap zmap = pane->zmap ;

  if (GTK_WIDGET_REALIZED(zmap->navview_frame))
    {

      /*  graphActivate(zMapWindowGetNavigator(window));
      **  graphFitBounds(NULL, &height);
      **  graphBoxDim(pane->scrollBox, &x1, &y1, &x2, &y2);
      
      **  startWind =  zmCoordFromScreen(pane, 0);
      **  endWind =  zmCoordFromScreen(pane, zMapPaneGetHeight(pane));
      
      **  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
      **  endWindf = zMapWindowGetScreenCoord(window, endWind, height);
      **  lenWindf = endWindf - startWindf;
      
      **  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
      
      **  graphBoxShift(pane->scrollBox, x1, startScreenf); */
    }

  return ;
}





static void valueCB(GtkAdjustment *adjustment, gpointer user_data)
{
  ZMapNavigator navigator = (ZMapNavigator)user_data ;

  printf("top: %f, bottom: %f\n", adjustment->value, adjustment->value + adjustment->page_size) ;

  if (navigator->cb_func)
    (*(navigator->cb_func))(navigator->user_data,
			    adjustment->value, adjustment->value + adjustment->page_size) ;

  return ;
}


/* UNUSED CURRENTLY but needed for when user alters the hpane interactively....... */
static void navResize(void)
{

  /* code needed ????? */

  return ;
}




