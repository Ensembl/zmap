/*  File: zmapView_P.h
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
 * Description: 
 * HISTORY:
 * Last edited: Jul 13 16:14 2004 (edgrif)
 * Created: Thu May 13 15:06:21 2004 (edgrif)
 * CVS info:   $Id: zmapView_P.h,v 1.5 2004-07-14 09:16:47 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>
#include <ZMap/zmapView.h>


/* A "View" is a set of one or more windows that display data retrieved from one or
 * more servers. Note that the "View" windows are _not_ top level windows, they are panes
 * within a container widget that is supplied as a parent of the View then the View
 * is first created.
 * Each View has lists of windows and lists of connections, the view handles these lists
 * using zmapWindow and zmapConnection calls.
 * */
typedef struct _ZMapViewStruct
{
  ZMapViewState state ;

  gchar *sequence ;

  guint idle_handle ;

  GtkWidget *parent_widget ;

  GList *window_list ;					    /* Of ZMapViewWindowStruct. */

  GList *connection_list ;

  void *app_data ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapViewCallbackFunc app_destroy_cb ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  ZMapRegion  *zMapRegion; /* the region holding all the SEGS - may be redundant*/

} ZMapViewStruct ;


/* We have this because it enables callers to call on a window but us to get the corresponding view. */
typedef struct _ZMapViewWindowStruct
{
  ZMapView parent_view ;
  
  ZMapWindow window ;
} ZMapViewWindowStruct ;



int          zMapWindowGetRegionLength (ZMapWindow window);
Coord        zMapWindowGetRegionArea   (ZMapWindow window, int area);
void         zMapWindowSetRegionArea   (ZMapWindow window, Coord area, int num);
gboolean     zMapWindowGetRegionReverse(ZMapWindow window);

#endif /* !ZMAP_VIEW_P_H */
