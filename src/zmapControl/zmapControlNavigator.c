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
 *              
 * Exported functions: See zmapControl_P.h
 * HISTORY:
 * Last edited: Jul 21 16:13 2004 (edgrif)
 * Created: Thu Jul  8 12:54:27 2004 (edgrif)
 * CVS info:   $Id: zmapControlNavigator.c,v 1.7 2004-07-21 15:14:45 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <zmapControl_P.h>

#define NAV_NO_SCALE "<no data>"

void zmapControlNavigatorCreate(ZMap zmap, GtkWidget *frame)
{
  GtkRequisition req;
  int increment = 200;
  GtkWidget *vscale;

  zmap->navigator = g_new0(ZMapNavStruct, 1);

  gtk_widget_size_request(frame, &req);

  /* Need a vbox so we can add a label with sequence size at the bottom later,
   * we set it to a fixed width so that the text is always visible. */
  zmap->navigator->navVBox = gtk_vbox_new(FALSE, 0);
  gtk_widget_set_usize(GTK_WIDGET(zmap->navigator->navVBox), 150, -2) ;

  /* if we want to change the max, we'll have to destroy and recreate the vscale. */
  zmap->navigator->navVScale = gtk_vscale_new_with_range(0, req.height, increment);
  gtk_box_pack_start(GTK_BOX(zmap->navigator->navVBox), zmap->navigator->navVScale, TRUE, TRUE, 0);

  /* Note how we pack the label at the end of the vbox and set "expand" to FALSE so that it
   * remains small and the vscale expands to fill the rest of the box. */
  zmap->navigator->navLabel = gtk_label_new(NAV_NO_SCALE) ;
  gtk_box_pack_end(GTK_BOX(zmap->navigator->navVBox), zmap->navigator->navLabel, FALSE, TRUE, 0);


  return;
}



/* updates size/range to coords of the new view. */
void zmapControlNavigatorNewView(ZMapNavigator navigator, ZMapMapBlock sequence_to_parent_mapping)
{
  GtkWidget *label;
  gchar *str;

  /* May be called with no sequence to parent mapping so must set default navigator for this. */
  if (sequence_to_parent_mapping)
    {
      navigator->sequence_to_parent = *sequence_to_parent_mapping ; /* n.b. struct copy */
      str = g_strdup_printf("%d", navigator->sequence_to_parent.p2) ;
    }
  else
    {
      navigator->sequence_to_parent.p1 = navigator->sequence_to_parent.p2
	= navigator->sequence_to_parent.c1 = navigator->sequence_to_parent.c2 = 0 ;
      str = g_strdup(NAV_NO_SCALE) ;
    }

  gtk_label_set_text(GTK_LABEL(navigator->navLabel), str);
  g_free(str);

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




