/*  File: zmapControlWindow.c
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
 * Description: Creates the top level window of a ZMap.
 *              
 * Exported functions: See zmapTopWindow_P.h
 * HISTORY:
 * Last edited: Aug 18 09:38 2004 (rnc)
 * Created: Fri May  7 14:43:28 2004 (edgrif)
 * CVS info:   $Id: zmapControlWindow.c,v 1.9 2004-08-18 14:59:57 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapControl_P.h>


static void quitCB(GtkWidget *widget, gpointer cb_data) ;


gboolean zmapControlWindowCreate(ZMap zmap)
{
  gboolean result = TRUE ;
  GtkWidget *toplevel, *vbox, *menubar, *button_frame;

  zmap->toplevel = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  gtk_window_set_policy(GTK_WINDOW(toplevel), FALSE, TRUE, FALSE ) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), zmap->zmap_id) ;
  gtk_container_border_width(GTK_CONTAINER(toplevel), 5) ;
  gtk_signal_connect(GTK_OBJECT(toplevel), "destroy", 
		     GTK_SIGNAL_FUNC(quitCB), (gpointer)zmap) ;

  vbox = gtk_vbox_new(FALSE, 0) ;
  gtk_container_add(GTK_CONTAINER(toplevel), vbox) ;

  menubar = zmapControlWindowMakeMenuBar(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, TRUE, 0);

  button_frame = zmapControlWindowMakeButtons(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), button_frame, FALSE, TRUE, 0);

  zmap->info_panel = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(vbox), zmap->info_panel, FALSE, TRUE, 0);

  zmap->navview_frame = zmapControlWindowMakeFrame(zmap) ;
  gtk_box_pack_start(GTK_BOX(vbox), zmap->navview_frame, TRUE, TRUE, 0);

  gtk_widget_show_all(toplevel) ;

  return result ;
}



void zmapControlWindowDestroy(ZMap zmap)
{
  /* We must disconnect the "destroy" callback otherwise we will enter quitCB()
   * below and that will try to call our callers destroy routine which has already
   * called this routine...i.e. a circularity which results in attempts to
   * destroy already destroyed windows etc. */
  gtk_signal_disconnect_by_data(GTK_OBJECT(zmap->toplevel), (gpointer)zmap) ;

  gtk_widget_destroy(zmap->toplevel) ;

  return ;
}






/*
 *  ------------------- Internal functions -------------------
 */


static void quitCB(GtkWidget *widget, gpointer cb_data)
{
  ZMap zmap = (ZMap)cb_data ;

  zmapControlTopLevelKillCB(zmap) ;

  return ;
}


/* For now, just moving functions from zmapWindow/zmapcontrol.c */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* zMapDisplay
 * Main entry point for the zmap code.  Called by zMapWindowCreate.
 * The first param is the display window, then two callback routines to 
 * allow zmap to interrogate a data-source. Then a void pointer to a
 * structure used in the process.  Although zmap doesn't need to know
 * directly about this structure, it needs to pass the pointer back
 * during callbacks, so AceDB can use it. 
 *
 * This will all have to change, now we're acedb-independent.
 *
 * We create the display window, then call the Activate 
 * callback routine to get the data, passing it a ZMapRegion in
 * which to create fmap-flavour segs for us to display, then
 * build the columns in the display.
 */

gboolean zMapDisplay(ZMap        zmap,
		     Activate_cb act_cb,
		     Calc_cb     calc_cb,
		     void       *region,
		     char       *seqspec, 
		     char       *fromspec, 
		     gboolean        isOldGraph)
{
  zmap->firstTime = TRUE ;				    /* used in addPane() */

  /* make the window in which to display the data */
  createNavViewWindow(zmap);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  drawWindow(zmap->focuspane) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return TRUE;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




	      
 





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS PROBABLY NEEDS TO BE REWRITTEN AND PUT IN ZMAPDRAW.C, THE WHOLE CONCEPT OF GRAPHHEIGHT IS
 * ALMOST CERTAINLY DEFUNCT NOW....... */

/* Not entirely convinced this is the right place for these
** public functions accessing private structure members
*/
int zMapWindowGetHeight(ZMapWindow window)
{
  return zmap->focuspane->graphHeight;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



