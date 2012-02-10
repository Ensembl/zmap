/*  Last edited: Dec 16 11:43 2011 (edgrif) */
/*  File: zmapControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 * Description: Interface for creating, controlling and destroying ZMaps.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONTROL_H
#define ZMAP_CONTROL_H


/* It turns out we need some way to refer to zmapviews at this level, while I don't really
 * want to expose all the zmapview stuff I do want the opaque ZMapView type.
 * Think about this some more. */
#include <ZMap/zmapView.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapAppRemote.h>

/* Opaque type, represents an instance of a ZMap. */
typedef struct _ZMapStruct *ZMap ;


/* Applications can register functions that will be called back with their own
 * data and a reference to the zmap that made the callback. */
typedef void (*ZMapCallbackFunc)(ZMap zmap, void *app_data) ;


/* Set of callback routines that allow the caller to be notified when events happen
 * to a ZMap. */
typedef struct _ZMapCallbacksStruct
{
  ZMapCallbackFunc destroy ;				    /* Reports that this zmap instance has
							       been destroyed. */
  ZMapCallbackFunc quit_req ;				    /* Requests application
							       termination. */
  ZMapRemoteAppMakeRequestFunc remote_request_func ;	    /* App level function to call to make requests. */

} ZMapCallbacksStruct, *ZMapCallbacks ;



void zMapInit(ZMapCallbacks callbacks) ;
ZMap zMapCreate(void *app_data, ZMapFeatureSequenceMap seq_map) ;
int zMapNumViews(ZMap zmap) ;
ZMapView zMapAddView(ZMap zmap, ZMapFeatureSequenceMap sequence_map) ;
gboolean zMapConnectView(ZMap zmap, ZMapView view) ;
gboolean zMapLoadView(ZMap zmap, ZMapView view) ;
gboolean zMapStopView(ZMap zmap, ZMapView view) ;
gboolean zMapDeleteView(ZMap zmap, ZMapView view) ;
gboolean zMapRaise(ZMap zmap);
char *zMapGetZMapID(ZMap zmap) ;
char *zMapGetZMapStatus(ZMap zmap) ;
gboolean zMapReset(ZMap zmap) ;
gboolean zMapDestroy(ZMap zmap) ;
gboolean zMapControlProcessRemoteRequest(gpointer local_data,
					 char *command_name, char *request,
					 ZMapRemoteAppReturnReplyFunc app_reply_func, gpointer app_reply_data) ;
void zMapAddClient(ZMap zmap, void *client);
char *zMapControlRemoteReceiveAccepts(ZMap zmap);


#endif /* !ZMAP_CONTROL_H */
