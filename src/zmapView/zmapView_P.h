/*  File: zmapView_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Last edited: Oct 13 15:04 2006 (rds)
 * Created: Thu May 13 15:06:21 2004 (edgrif)
 * CVS info:   $Id: zmapView_P.h,v 1.23 2006-11-08 09:24:59 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapWindowNavigator.h>


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
  /* Record whether this connection will serve up raw sequence or deal with feature edits. */
  gboolean sequence_server ;
  gboolean writeback_server ;

  GList *types ;					    /* The "methods", "types" call them
							       what you will. */

  ZMapView parent_view ;

  ZMapThreadRequest curr_request ;

  ZMapThread thread ;

} ZMapViewConnectionStruct, *ZMapViewConnection ;



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


  /* NOTE TO EG....DO WE NEED THESE HERE ANY MORE ???? */
  /* This specifies the "context" of the view, i.e. which section of virtual sequence we are
   * interested in. */
  gchar *sequence ;
  int start, end ;

  guint idle_handle ;

  GList *window_list ;					    /* Of ZMapViewWindow. */

  int connections_loaded ;				    /* Record of number of connections
							     * loaded so for each reload. */
  GList *connection_list ;				    /* Of ZMapViewConnection. */
  ZMapViewConnection sequence_server ;			    /* Which connection to get raw
							       sequence from. */
  ZMapViewConnection writeback_server ;			    /* Which connection to send edits to. */

  ZMapWindowNavigator navigator_window;

  /* We need to know if the user has done a revcomp for a few reasons to do with coord
   * transforms and the way annotation is done....*/
  gboolean revcomped_features ;

  /* The features....needs thought as to how this updated/constructed..... */
  ZMapFeatureContext features ;

#ifdef RDS_DONT_INCLUDE_UNUSED
  /* In DAS2 terminology methods are types...easy to change if we don't like the name.
   * These are the stylesheets in effect for the feature sets, this set is a merge of all the
   * sets from the various servers. */
  GData *types ;
#endif

  GList *navigator_set_names;

} ZMapViewStruct ;



void zmapViewBusy(ZMapView zmap_view, gboolean busy) ;
gboolean zmapAnyConnBusy(GList *connection_list) ;
char *zmapViewGetStatusAsStr(ZMapViewState state) ;

#endif /* !ZMAP_VIEW_P_H */
