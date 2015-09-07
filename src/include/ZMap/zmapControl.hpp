/*  File: zmapControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2015: Genome Research Ltd.
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface for creating, controlling and destroying ZMaps.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_H
#define ZMAP_CONTROL_H


/* It turns out we need some way to refer to zmapviews at this level, while I don't really
 * want to expose all the zmapview stuff I do want the opaque ZMapView type.
 * Think about this some more. */
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapView.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapWindow.hpp>
#include <ZMap/zmapAppRemote.hpp>

/* Opaque type, represents an instance of a ZMap. */
typedef struct _ZMapStruct *ZMap ;


/* Applications can register functions that will be called back with their own
 * data and a reference to the zmap that made the callback. */
typedef void (*ZMapCallbackFunc)(ZMap zmap, void *app_data) ;
typedef void (*ZMapCallbackViewFunc)(ZMap zmap, ZMapView view, void *app_data) ;


/* Set of callback routines that allow the caller to be notified when events happen
 * to a ZMap. */
typedef struct _ZMapCallbacksStruct
{
  ZMapCallbackViewFunc view_add ;                           /* Reports that zmap/view has been
							       created. */
  ZMapCallbackViewFunc view_destroy ;                       /* Reports that a view within a zmap
                                                               has been destroyed. */
  ZMapCallbackViewFunc destroy ;                            /* Reports that this zmap instance has
							       been destroyed. */
  ZMapCallbackFunc quit_req ;				    /* Requests application
							       termination. */

  /* App level function (+ its data) to call to make remote requests. */
  ZMapRemoteAppMakeRequestFunc remote_request_func ;
  void *remote_request_func_data ;

} ZMapCallbacksStruct, *ZMapCallbacks ;


/* Callback function prototype for zMapControlForAllViews() */
typedef void (*ZMapControlForAllCallbackFunc)(ZMapView view, void *user_data) ;



void zMapInit(ZMapCallbacks callbacks) ;
ZMap zMapCreate(void *app_data, ZMapFeatureSequenceMap sequence_map, GdkCursor *normal_cursor) ;
int zMapNumViews(ZMap zmap) ;
gboolean zMapSetSessionColour(ZMap zmap, GdkColor *session_colour) ;
ZMapViewWindow zMapAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, GError **error) ;
ZMapView zMapControlInsertView(ZMap zmap, ZMapFeatureSequenceMap sequence_map, char **err_msg) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapGetDefaultView(ZMapAppRemoteViewID view_inout) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

gboolean zMapConnectView(ZMap zmap, ZMapView view, GError **error) ;
gboolean zMapLoadView(ZMap zmap, ZMapView view) ;

gboolean zMapStopView(ZMap zmap, ZMapView view) ;
gboolean zMapControlCloseView(ZMap zmap, ZMapView view) ;
void zMapDeleteView(ZMap zmap, ZMapView view, GList **destroyed_views_inout) ;
gpointer zMapControlFindView(ZMap zmap, gpointer view_id) ;
gboolean zMapRaise(ZMap zmap);
char *zMapGetZMapID(ZMap zmap) ;
char *zMapGetZMapStatus(ZMap zmap) ;
void zMapSetCursor(ZMap zmap, GdkCursor *cursor) ;
void zMapControlForAllZMapViews(ZMap zmap, ZMapControlForAllCallbackFunc user_func_cb, void *user_func_data) ;
gboolean zMapCheckIfUnsaved(ZMap zmap) ;
gboolean zMapReset(ZMap zmap) ;
void zMapDestroy(ZMap zmap, GList **destroyed_views_inout) ;

gboolean zMapControlProcessRemoteRequest(ZMap zmap,
					 char *command_name, char *request, gpointer view_id,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
void zMapAddClient(ZMap zmap, void *client);
char *zMapControlRemoteReceiveAccepts(ZMap zmap);
gboolean zMapReset(ZMap zmap) ;
void zMapDestroy(ZMap zmap, GList **destroyed_views_inout) ;

void zMapControlPreferencesUpdateContext(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, gpointer data) ;


#endif /* !ZMAP_CONTROL_H */
