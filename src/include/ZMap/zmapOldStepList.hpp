/*  File: zmapViewDatasourceSteplist.hpp
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
