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
 * Last edited: Sep 16 15:26 2004 (rnc)
 * Created: Thu Jul  8 12:54:27 2004 (edgrif)
 * CVS info:   $Id: zmapControlNavigator.c,v 1.14 2004-09-16 15:34:48 rnc Exp $
 *-------------------------------------------------------------------
 */


#include <zmapControl_P.h>

#define TOPTEXT_NO_SCALE "<no data>"
#define BOTTEXT_NO_SCALE ""

void zmapControlNavigatorCreate(ZMap zmap, GtkWidget *frame)
{
  GtkRequisition req;
  int increment = 200;
  GtkWidget *vscale;
  GtkObject *adjustment ;

  zmap->navigator = g_new0(ZMapNavStruct, 1);

  gtk_widget_size_request(frame, &req);

  /* Need a vbox so we can add a label with sequence size at the bottom later,
   * we set it to a fixed width so that the text is always visible. */
  zmap->navigator->navVBox = gtk_vbox_new(FALSE, 0);
  gtk_widget_set_usize(GTK_WIDGET(zmap->navigator->navVBox), 100, -2) ;

  zmap->navigator->topLabel = gtk_label_new(TOPTEXT_NO_SCALE) ;
  gtk_box_pack_start(GTK_BOX(zmap->navigator->navVBox), zmap->navigator->topLabel, FALSE, TRUE, 0);


  /* Make the navigator with a default, "blank" adjustment obj. */
  adjustment = gtk_adjustment_new(0.0, 0.0, 0.0, 0.0, 0.0, 0.0) ;

  zmap->navigator->navVScroll = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustment)) ;
  gtk_box_pack_start(GTK_BOX(zmap->navigator->navVBox), zmap->navigator->navVScroll, TRUE, TRUE, 0) ;

  /* Note how we pack the label at the end of the vbox and set "expand" to FALSE so that it
   * remains small and the vscale expands to fill the rest of the box. */
  zmap->navigator->botLabel = gtk_label_new(BOTTEXT_NO_SCALE) ;
  gtk_box_pack_end(GTK_BOX(zmap->navigator->navVBox), zmap->navigator->botLabel, FALSE, TRUE, 0);


  return;
}



/* updates size/range to coords of the new view. */
void zmapControlNavigatorNewView(ZMapNavigator navigator, ZMapFeatureContext features)
{
  GtkWidget *label;
  gchar *top_str, *bot_str ;
  GtkObject *adjuster ;

  adjuster = (GtkObject *)gtk_range_get_adjustment(GTK_RANGE(navigator->navVScroll)) ;


  /* May be called with no sequence to parent mapping so must set default navigator for this. */
  if (features)
    {
      navigator->parent_span = features->parent_span ;	    /* n.b. struct copy. */
      navigator->sequence_to_parent = features->sequence_to_parent ; /* n.b. struct copy. */

      top_str = g_strdup_printf("%d", navigator->parent_span.x1) ;
      bot_str = g_strdup_printf("%d", navigator->parent_span.x2) ;

      GTK_ADJUSTMENT(adjuster)->value = (((gdouble)(navigator->sequence_to_parent.p1
						    + navigator->sequence_to_parent.p2)) / 2.0) ;
      GTK_ADJUSTMENT(adjuster)->lower = (gdouble)navigator->parent_span.x1 ;
      GTK_ADJUSTMENT(adjuster)->upper = (gdouble)navigator->parent_span.x2 ;
      GTK_ADJUSTMENT(adjuster)->step_increment = 10.0 ;	    /* step incr, wild guess... */
      GTK_ADJUSTMENT(adjuster)->page_increment = 1000 ;	    /* page incr, wild guess... */
      GTK_ADJUSTMENT(adjuster)->page_size = (gdouble)(abs(navigator->sequence_to_parent.p2
							  - navigator->sequence_to_parent.p1) + 1) ;
    }
  else
    {
      navigator->parent_span.x1 = navigator->parent_span.x2 = 0 ;

      navigator->sequence_to_parent.p1 = navigator->sequence_to_parent.p2
	= navigator->sequence_to_parent.c1 = navigator->sequence_to_parent.c2 = 0 ;

      top_str = g_strdup(TOPTEXT_NO_SCALE) ;
      bot_str = g_strdup(BOTTEXT_NO_SCALE) ;

      GTK_ADJUSTMENT(adjuster)->value = 0.0 ;
      GTK_ADJUSTMENT(adjuster)->lower = 0.0 ;
      GTK_ADJUSTMENT(adjuster)->upper = 0.0 ;
      GTK_ADJUSTMENT(adjuster)->step_increment = 0.0 ;
      GTK_ADJUSTMENT(adjuster)->page_increment = 0.0 ;
      GTK_ADJUSTMENT(adjuster)->page_size = 0.0 ;
    }

  gtk_adjustment_changed(GTK_ADJUSTMENT(adjuster)) ;

  gtk_label_set_text(GTK_LABEL(navigator->topLabel), top_str) ;
  gtk_label_set_text(GTK_LABEL(navigator->botLabel), bot_str) ;

  g_free(top_str) ;
  g_free(bot_str) ;

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
  int height;
  Coord startWind, endWind;
  ScreenCoord startWindf, startScreenf, endWindf, lenWindf;
  float x1, y1, x2, y2;

  if (GTK_WIDGET_REALIZED(zmap->navview_frame))
    {

      //  graphActivate(zMapWindowGetNavigator(window));
      //  graphFitBounds(NULL, &height);
      //  graphBoxDim(pane->scrollBox, &x1, &y1, &x2, &y2);
      
      //  startWind =  zmCoordFromScreen(pane, 0);
      //  endWind =  zmCoordFromScreen(pane, zMapPaneGetHeight(pane));
      
      //  startWindf = zMapWindowGetScreenCoord(window, startWind, height);
      //  endWindf = zMapWindowGetScreenCoord(window, endWind, height);
      //  lenWindf = endWindf - startWindf;
      
      //  startScreenf = startWindf + lenWindf * (adj->value - adj->lower)/(adj->upper - adj->lower) ;
      
      //  graphBoxShift(pane->scrollBox, x1, startScreenf);
    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Currently not called........... */
void navScale(FooCanvas *canvas, float offset, int start, int end)
{
  int x, width = 5, count;
  FooCanvasItem *group;

  group = foo_canvas_item_new(foo_canvas_root(canvas),
			foo_canvas_group_get_type(),
			"x",(double)offset,
			"y",(double)0.0,
			NULL);
 
  for (x = start, count = 1 ; x < end ; x += 10, count++)
    {
      zmapDrawLine(FOO_CANVAS_GROUP(group), offset-5, x, offset, x, "black", 1.0);
      char text[25];
      sprintf(text,"%dk", x);
      if (count == 1)
	zmapDisplayText(FOO_CANVAS_GROUP(group), text, offset + 20, x); 
      if (count > 9) count = 0;
    }
			     
  zmapDrawLine(FOO_CANVAS_GROUP(group), offset+1, 0, offset+1, end, "black", 1.0);

  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* UNUSED CURRENTLY but needed for when user alters the hpane interactively....... */
static void navResize(void)
{

  /* code needed ????? */

  return ;
}




