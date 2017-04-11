/*  File: zmapControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
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


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapControlProcessRemoteRequest(ZMap zmap,
					 char *command_name, char *request, gpointer view_id,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

void zMapAddClient(ZMap zmap, void *client);
char *zMapControlRemoteReceiveAccepts(ZMap zmap);
gboolean zMapReset(ZMap zmap) ;
void zMapDestroy(ZMap zmap, GList **destroyed_views_inout) ;

void zMapControlPreferencesUpdateContext(ZMapConfigIniContext context, ZMapConfigIniFileType file_type, gpointer data) ;


#endif /* !ZMAP_CONTROL_H */
