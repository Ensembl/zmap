/*  File: zmapViewDatasourceSteplist.hpp
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2016: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *      Gemma Barson (Sanger Institute, UK) gb10@sanger.ac.uk
 *
 * Description: The "old" way of contacting a server, forces app to
 *              to specify a series of steps that must be performed
 *              to contact and use a server. Too complex. Is being
 *              replaced by the DataSource object.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DATASOURCE_STEPLIST_H
#define ZMAP_DATASOURCE_STEPLIST_H

#include <ZMap/zmapOldSourceServer.hpp>


/*
 *           Control of data request to connections
 *
 * Requests to servers are made as a series of steps specified using these structs.
 * Each step is executed before and the result tested, each connection and the whole
 * step list can be aborted.
 *
 */


/* State of a step and a request within a step. */
typedef enum {STEPLIST_INVALID, STEPLIST_PENDING, STEPLIST_DISPATCHED, STEPLIST_FINISHED} StepListStepState ;


/* Specifies the action that should be taken if a sub-step fails in a step list. */
typedef enum
  {
    REQUEST_ONFAIL_INVALID,
    REQUEST_ONFAIL_CONTINUE,				    /* Continue other remaining steps. */
    REQUEST_ONFAIL_CANCEL_THREAD,			    /* Cancel thread sevicing request. */
    REQUEST_ONFAIL_CANCEL_STEPLIST			    /* Cancel the whole steplist. */
  } StepListActionOnFailureType ;



// Opaque types
typedef struct ZMapViewConnectionStepListStructType *ZMapViewConnectionStepList ;

typedef struct ZMapViewConnectionStepStructType *ZMapViewConnectionStep ;

typedef struct ZMapViewConnectionRequestStructType *ZMapViewConnectionRequest ;



/* Callbacks returning a boolean should return TRUE if the connection should continue,
 * FALSE if it's failed, in this case the connection is immediately removed from the steplist. */

/* Callback for dispatching step requests. */
typedef gboolean (*StepListDispatchCB)(ZMapServerReqAny req_any, gpointer connection_data) ;

/* Callback for processing step requests. */
typedef gboolean (*StepListProcessDataCB)(void *user_data, ZMapServerReqAny req_any) ;


/* Callback for freeing step requests. */
typedef void (*StepListFreeDataCB)(ZMapServerReqAny req_any) ;


ZMapViewConnectionStepList zmapViewStepListCreate(StepListDispatchCB dispatch_func,
						  StepListProcessDataCB request_func,
						  StepListFreeDataCB free_func) ;
ZMapViewConnectionStepList zmapViewStepListCreateFull(StepListDispatchCB dispatch_func,
                                                      StepListProcessDataCB process_func,
                                                      StepListFreeDataCB free_func);
void zmapViewStepListIter(ZMapViewConnectionStepList step_list, ZMapThread thread, gpointer connection_data) ;
void zmapViewStepListStepProcessRequest(ZMapViewConnectionStepList step_list,
                                        void *user_data, ZMapViewConnectionRequest request) ;
ZMapViewConnectionStep zmapViewStepListGetCurrentStep(ZMapViewConnectionStepList step_list) ;
void zmapViewStepListAddStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type,
			     StepListActionOnFailureType on_fail) ;
ZMapViewConnectionRequest zmapViewStepListAddServerReq(ZMapViewConnectionStepList step_list,
						       ZMapServerReqType request_type,
						       gpointer request_data,
                                                       StepListActionOnFailureType on_fail) ;
ZMapViewConnectionRequest zmapViewStepListFindRequest(ZMapViewConnectionStepList step_list,
						      ZMapServerReqType request_type, ZMapNewDataSource connection) ;
gboolean zmapViewStepListAreConnections(ZMapViewConnectionStepList step_list) ;
gboolean zmapViewStepListIsNext(ZMapViewConnectionStepList step_list) ;

void zmapViewStepListStepConnectionDeleteAll(ZMapNewDataSource connection) ;
void zmapViewStepListDestroy(ZMapViewConnectionStepList step_list) ;


bool zMapConnectionIsFinished(ZMapViewConnectionRequest request) ;


StepListActionOnFailureType zMapStepOnFailAction(ZMapViewConnectionStep step) ;
void zMapStepSetState(ZMapViewConnectionStep step, StepListStepState state) ;
ZMapServerReqType zMapStepGetRequest(ZMapViewConnectionStep step) ;

// this seems appallingly named.....
void zmapViewStepDestroy(gpointer data, gpointer user_data) ;




#endif /* !ZMAP_DATASOURCE_STEPLIST_H */
