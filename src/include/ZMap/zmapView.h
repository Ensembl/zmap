/*  File: zmapView.h
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
 * Description: Interface for controlling a single "view", a view
 *              comprises one or more windowsw which display data
 *              collected from one or more servers. Hence the view
 *              interface controls both windows and connections to
 *              servers.
 *              
 * HISTORY:
 * Last edited: Feb 19 11:17 2007 (edgrif)
 * Created: Thu May 13 14:59:14 2004 (edgrif)
 * CVS info:   $Id: zmapView.h,v 1.37 2007-03-01 09:11:47 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAPVIEW_H
#define ZMAPVIEW_H

#include <gtk/gtk.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapWindowNavigator.h>


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
  ZMapViewWindowCallbackFunc enter ;
  ZMapViewWindowCallbackFunc leave ;
  ZMapViewCallbackFunc load_data ;
  ZMapViewWindowCallbackFunc focus ;
  ZMapViewWindowCallbackFunc select ;
  ZMapViewWindowCallbackFunc split_to_pattern;
  ZMapViewWindowCallbackFunc visibility_change ;
  ZMapViewCallbackFunc state_change ;
  ZMapViewCallbackFunc destroy ;
} ZMapViewCallbacksStruct, *ZMapViewCallbacks ;


/* Holds a sequence to be fetched, in the end this will include aligns/blocks etc. */
typedef struct
{
  char *sequence ;					    /* Sequence + start/end coords. */
  int start, end ;
} ZMapViewSequenceMapStruct, *ZMapViewSequenceMap ;


/* Holds a sequence to be fetched and the server it should be fetched from. */
typedef struct
{
  char *sequence ;					    /* Sequence + start/end coords. */
  char *server ;					    /* Server to fetch sequence from. */
} ZMapViewSequence2ServerStruct, *ZMapViewSequence2Server ;




/* Holds structs/strings describing the selected item, this data actually comes from the
 * zmapWindow layer and is just passed on through view. */
typedef struct _ZMapViewSelectStruct
{
  ZMapWindowSelectType  type;
  ZMapFeatureDescStruct feature_desc ;
  char                 *secondary_text;
  GArray               *xml_events;
  char                 *zmap_action;
  gboolean              handled;
} ZMapViewSelectStruct, *ZMapViewSelect ;

typedef struct _ZMapViewSplittingStruct
{
  GArray *split_patterns;
  GList *touched_window_list;   /* A list of view_windows affected by the split */
}ZMapViewSplittingStruct, *ZMapViewSplitting;

/* The overall state of the zmapView, we need this because both the zmap window and the its threads
 * will die asynchronously so we need to block further operations while they are in this state. */
typedef enum {

  /* do we need both of these...check.... */
  ZMAPVIEW_INIT,					    /* Window(s), but no threads. */

  ZMAPVIEW_CONNECTING,					    /* Connecting threads. */
  ZMAPVIEW_CONNECTED,					    /* Threads connected, no data yet. */

  ZMAPVIEW_LOADING,					    /* Loading data. */
  ZMAPVIEW_LOADED,					    /* Full view. */

  ZMAPVIEW_RESETTING,					    /* Returning to ZMAPVIEW_NOT_CONNECTED. */

  ZMAPVIEW_DYING					    /* View is dying for some reason,
							       cannot do anything in this state. */
} ZMapViewState ;



void zMapViewInit(ZMapViewCallbacks callbacks) ;
ZMapViewWindow zMapViewCreate(GtkWidget *parent_widget,
			      char *sequence, int start, int end,
			      void *app_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS IS WHERE I'M HEADED...ONE CALL TO PREPARE MULTIPLE SEQUENCES/FEATURES.... */

ZMapViewWindow zMapViewCreateFull(GtkWidget *parent_widget,
				  char *view_name,
				  GList *sequences,	    /* of ZMapViewSequenceFetch */
				  void *app_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

void zMapViewSetupNavigator(ZMapView zmap_view, GtkWidget *canvas_widget);
ZMapViewWindow zMapViewCopyWindow(ZMapView zmap_view, GtkWidget *parent_widget,
				  ZMapWindow copy_window, ZMapWindowLockType window_locking) ;
void zMapViewRemoveWindow(ZMapViewWindow view_window) ;

void zMapViewRedraw(ZMapViewWindow view_window) ;

gboolean zMapViewConnect(ZMapView zmap_view, char *config_str) ;
gboolean zMapViewLoad (ZMapView zmap_view) ;
gboolean zMapViewReset(ZMapView zmap_view) ;
gboolean zMapViewReverseComplement(ZMapView zmap_view) ;
gboolean zMapViewGetRevCompStatus(ZMapView zmap_view) ;
void zMapViewZoom(ZMapView zmap_view, ZMapViewWindow view_window, double zoom) ;
char *zMapViewGetSequence(ZMapView zmap_view) ;
ZMapFeatureContext zMapViewGetFeatures(ZMapView zmap_view) ;
void zMapViewGetVisible(ZMapViewWindow view_window, double *top, double *bottom) ;
ZMapViewState zMapViewGetStatus(ZMapView zmap_view) ;
char *zMapViewGetStatusStr(ZMapViewState zmap_state) ;
gboolean zMapViewGetFeaturesSpan(ZMapView zmap_view, int *start, int *end) ;
ZMapWindow zMapViewGetWindow(ZMapViewWindow view_window) ;
ZMapView zMapViewGetView(ZMapViewWindow view_window) ;
GData *zMapViewGetStyles(ZMapViewWindow view_window) ;
ZMapWindowNavigator zMapViewGetNavigator(ZMapView view);
int zMapViewNumWindows(ZMapViewWindow view_window) ;

GList *zMapViewGetWindowList(ZMapViewWindow view_window);
void   zMapViewSetWindowList(ZMapViewWindow view_window, GList *list);

void zmapViewFeatureDump(ZMapViewWindow view_window, char *file) ;

ZMapFeatureContext zMapViewMergeInContext(ZMapView replace_me, ZMapFeatureContext context_inout);
void zMapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout);

gboolean zMapViewDestroy(ZMapView zmap_view) ;

/* HACK! not really to be used... */
ZMapFeatureContext zMapViewGetContextAsEmptyCopy(ZMapView do_not_use);

#endif /* !ZMAPVIEW_H */
