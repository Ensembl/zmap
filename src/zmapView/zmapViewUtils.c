/*  File: zmapViewUtils.c
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
 * Description: Utility functions for a ZMapView.
 *              
 * Exported functions: See ZMap/ZMapView.h for public functions and
 *              zmapView_P.h for private functions.
 * HISTORY:
 * Last edited: Sep 21 10:46 2004 (edgrif)
 * Created: Mon Sep 20 10:29:15 2004 (edgrif)
 * CVS info:   $Id: zmapViewUtils.c,v 1.1 2004-09-23 13:49:43 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapConn.h>
#include <zmapView_P.h>

void zmapViewBusy(ZMapView zmap_view, gboolean busy)
{
  static GdkCursor* busy_cursor = NULL ;
  GList* list_item ;

  if (busy_cursor == NULL)
    {
      busy_cursor = gdk_cursor_new(GDK_WATCH) ;
    }

  zmap_view->busy = busy ;

  printf("zmapViewBusy state is: %s\n", busy ? "on" : "off") ;


  if ((list_item = g_list_first(zmap_view->window_list)))
    {
      do
	{
	  ZMapViewWindow view_window ;
	  GtkWidget *view_widget ;
	  GdkWindow *window = NULL ;

	  view_window = list_item->data ;

	  view_widget = zMapWindowGetWidget(view_window->window) ;

	  if ((window = view_widget->window))
	    {
	      if (busy)
		gdk_window_set_cursor(window, busy_cursor) ;
	      else
		gdk_window_set_cursor(window, NULL) ;		    /* NULL => use parents cursor. */
	    }
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return ;
}



gboolean zmapAnyConnBusy(GList *connection_list)
{
  gboolean result = FALSE ;
  GList* list_item ;
  ZMapViewConnection view_con ;

  if (connection_list)
    {
      list_item = g_list_first(connection_list) ;

      do
	{
	  view_con = list_item->data ;
	  
	  if (view_con->curr_request == ZMAP_REQUEST_GETDATA)
	    {
	      result = TRUE ;
	      break ;
	    }
	}
      while ((list_item = g_list_next(list_item))) ;
    }

  return result ;
}
