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
 * Last edited: Oct  4 17:05 2004 (edgrif)
 * Created: Thu May 13 15:06:21 2004 (edgrif)
 * CVS info:   $Id: zmapView_P.h,v 1.12 2004-10-14 10:25:28 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>
#include <ZMap/zmapConn.h>
#include <ZMap/zmapView.h>


#define DEFAULT_WIDTH 10.0              /* of features being displayed */

/* A "View" is a set of one or more windows that display data retrieved from one or
 * more servers. Note that the "View" windows are _not_ top level windows, they are panes
 * within a container widget that is supplied as a parent of the View then the View
 * is first created.
 * Each View has lists of windows and lists of connections, the view handles these lists
 * using zmapWindow and zmapConnection calls.
 * */
typedef struct _ZMapViewStruct
{
  ZMapViewState state ;					    /* Overall state of the ZMapView. */

  gboolean busy ;					    /* Records when we are busy so can
							       block user interaction. */

  void *app_data ;					    /* Passed back to caller from view
							       callbacks. */

  /* This specifies the "context" of the view, i.e. which section of virtual sequence we are
   * interested in. */
  gchar *sequence ;
  int start, end ;

  guint idle_handle ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GtkWidget *parent_widget ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  GList *window_list ;					    /* Of ZMapViewWindowStruct. */

  GList *connection_list ;


  /* The features....needs thought as to how this updated/constructed..... */
  ZMapFeatureContext features ;

  /* In DAS2 terminology methods are types...easy to change if we don't like the name.
   * These are the stylesheets in effect for the feature sets. */
  GData          *types ;

  ZMapRegion  *zMapRegion; /* the region holding all the SEGS - may be redundant*/

} ZMapViewStruct ;


/* We have this because it enables callers to call on a window but us to get the corresponding view. */
typedef struct _ZMapViewWindowStruct
{
  ZMapView parent_view ;
  
  ZMapWindow window ;
} ZMapViewWindowStruct ;





/* We need to maintain state for our connections otherwise we will try to issue requests that
 * they may not see. curr_request flip-flops between ZMAP_REQUEST_GETDATA and ZMAP_REQUEST_WAIT */
typedef struct _ZMapViewConnectionStruct
{
  ZMapView parent_view ;

  ZMapThreadRequest curr_request ;

  ZMapConnection connection ;


} ZMapViewConnectionStruct, *ZMapViewConnection ;


void zmapViewBusy(ZMapView zmap_view, gboolean busy) ;
gboolean zmapAnyConnBusy(GList *connection_list) ;

int          zMapWindowGetRegionLength (ZMapWindow window);
Coord        zMapWindowGetRegionArea   (ZMapWindow window, int area);
void         zMapWindowSetRegionArea   (ZMapWindow window, Coord area, int num);
gboolean     zMapWindowGetRegionReverse(ZMapWindow window);

#endif /* !ZMAP_VIEW_P_H */
