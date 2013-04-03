/*  File: zmapManager.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description:
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_MANAGER_H
#define ZMAP_MANAGER_H

#include <ZMap/zmapControl.h>
#include <ZMap/zmapAppRemote.h>


/* Specifies result of trying to add a new zmap to manager. */
typedef enum
 {
   ZMAPMANAGER_ADD_INVALID,
   ZMAPMANAGER_ADD_OK,					    /* Add succeeded, zmap connected. */
   ZMAPMANAGER_ADD_FAIL,				    /* ZMap could not be added. */
   ZMAPMANAGER_ADD_DISASTER				    /* There has been a serious error,
							       caller should abort. */
 } ZMapManagerAddResult ;


/* Opaque type, controls interaction with all current ZMap windows. */
typedef struct _ZMapManagerStruct *ZMapManager ;


/* Callers can specify callback functions which get called with ZMapView which made the call
 * and the applications own data pointer. If the callback is to made for a window event
 * then the ZMapViewWindow where the event took place will be returned as the first
 * parameter. If the callback is for an event that involves the whole view (e.g. destroy)
 * then the ZMapView where the event took place is returned. */
typedef void (*ZMapManagerCallbackFunc)(void *app_data, void * zmap) ;


/* Set of callback routines that allow the caller to be notified when events happen
 * to manager. */
typedef struct
{
  ZMapManagerCallbackFunc zmap_added_func ;
  ZMapManagerCallbackFunc zmap_deleted_func ;
  ZMapManagerCallbackFunc zmap_set_info_func ;		    /* CAN WE DELETE THIS NOW ??? */
  ZMapManagerCallbackFunc quit_req_func ;

  ZMapRemoteAppMakeRequestFunc remote_request_func ;
  void *remote_request_func_data ;
} ZMapManagerCallbacksStruct, *ZMapManagerCallbacks ;



void zMapManagerInit(ZMapManagerCallbacks callbacks) ;
ZMapManager zMapManagerCreate(void *gui_data) ;
ZMapManagerAddResult zMapManagerAdd(ZMapManager zmaps, ZMapFeatureSequenceMap sequence_map,
				    ZMap *zmap_out, ZMapView *view_out, gboolean load_view) ;
gboolean zMapManagerGetDefaultView(ZMapManager zmaps, ZMapAppRemoteViewID view_inout) ;
void zMapManagerDestroyView(ZMapManager zmaps, ZMap zmap, ZMapView view) ;
guint zMapManagerCount(ZMapManager zmaps);
gboolean zMapManagerReset(ZMap zmap) ;
gboolean zMapManagerRaise(ZMap zmap) ;
gboolean zMapManagerProcessRemoteRequest(ZMapManager manager,
					 char *command_name, ZMapAppRemoteViewID view_id, char *request,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;

void zMapManagerKill(ZMapManager zmaps, ZMap zmap) ;
gboolean zMapManagerKillAllZMaps(ZMapManager zmaps);
gboolean zMapManagerDestroy(ZMapManager zmaps) ;

#endif /* !ZMAP_MANAGER_H */
