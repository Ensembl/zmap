/*  File: zmapView.h
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
 * Description: Interface for controlling a single "view", a view
 *              comprises one or more windowsw which display data
 *              collected from one or more servers. Hence the view
 *              interface controls both windows and connections to
 *              servers.
 *              
 * HISTORY:
 * Last edited: Dec 16 16:42 2004 (edgrif)
 * Created: Thu May 13 14:59:14 2004 (edgrif)
 * CVS info:   $Id: zmapView.h,v 1.15 2004-12-20 10:52:17 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAPVIEW_H
#define ZMAPVIEW_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>


/* Opaque type, represents an instance of a ZMapView. */
typedef struct _ZMapViewStruct *ZMapView ;


/* Opaque type, represents an instance of a ZMapView window. */
typedef struct _ZMapViewWindowStruct *ZMapViewWindow ;


/* Callers can specify callback functions which get called with ZMapView which made the call
 * and the applications own data pointer. If the callback is to made for a window event
 * then the ZMapViewWindow where the event took place will be returned as the first
 * parameter. If the callback is for an event that involves the whole view (e.g. destroy)
 * then the ZMapView where the event took place is returned. */
typedef void (*ZMapViewWindowCallbackFunc)(ZMapViewWindow view_window, void *app_data, void *view_data) ;
typedef void (*ZMapViewCallbackFunc)(ZMapView zmap_view, void *app_data, void *view_data) ;

/* Set of callback routines that allow the caller to be notified when events happen
 * to a window. */
typedef struct _ZMapViewCallbacksStruct
{
  ZMapViewCallbackFunc load_data ;
  ZMapViewWindowCallbackFunc click ;
  ZMapViewWindowCallbackFunc visibility_change ;
  ZMapViewCallbackFunc destroy ;
} ZMapViewCallbacksStruct, *ZMapViewCallbacks ;


/* DOES THIS STATE NEED TO BE EXPOSED LIKE THIS...REVISIT.... */
/* The overall state of the zmapView, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state. */
typedef enum {
  ZMAPVIEW_INIT,					    /* No window(s) and no threads. */
  ZMAPVIEW_NOT_CONNECTED,				    /* Window(s) with no threads. */
  ZMAPVIEW_RUNNING,					    /* Window(S) with threads in normal state. */
  ZMAPVIEW_RESETTING,					    /* Window(S) that is closing its threads
							       and returning to INIT state. */
  ZMAPVIEW_DYING					    /* ZMap is dying for some reason,
							       cannot do anything in this state. */
} ZMapViewState ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* I THINK WE MAY NEED SOMETHING MORE LIKE THIS.... */
typedef enum {
  ZMAPVIEW_INIT,					    /* No window(s) and no threads. */

  ZMAPVIEW_NOT_CONNECTED,				    /* Window(s) with no threads. */
  ZMAPVIEW_CONNECTING,

  ZMAPVIEW_WAITING,					    /* Window(s) + thread(s) in normal
							       state. */
  ZMAPVIEW_DOING_REQUEST,


  ZMAPVIEW_RESETTING,					    /* Window(s) that is closing its threads
							       and returning to INIT state. */



  ZMAPVIEW_DYING,					    /* ZMap is dying for some reason,
							       cannot do anything in this state. */
  ZMAPVIEW_DEAD						    /* DON'T KNOW IF WE NEED THIS.... */
} ZMapViewState ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






void zMapViewInit(ZMapViewCallbacks callbacks) ;
ZMapView zMapViewCreate(char *sequence,	int start, int end, void *app_data) ;
ZMapViewWindow zMapViewAddWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				 ZMapWindow curr_view_window) ;
gboolean zMapViewConnect(ZMapView zmap_view) ;
gboolean zMapViewLoad (ZMapView zmap_view) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Don't exist but will be needed. */
gboolean zMapViewDeleteWindow(ZMapViewWindow view_window) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

gboolean zMapViewReset(ZMapView zmap_view) ;

char *zMapViewGetSequence(ZMapView zmap_view) ;
ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view) ;
ZMapViewState zMapViewGetStatus(ZMapView zmap_view) ;
char *zMapViewGetStatusStr(ZMapViewState zmap_state) ;
ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window) ;
ZMapView zMapViewGetView(ZMapViewWindow view_window) ;
GList *zMapViewGetWindowList(ZMapViewWindow view_window);
void   zMapViewSetWindowList(ZMapViewWindow view_window, GList *list);

gboolean zMapViewDestroy(ZMapView zmap_view) ;

void     zmapViewFeatureDump(ZMapViewWindow view_window, char *file, int format);


#endif /* !ZMAPVIEW_H */
