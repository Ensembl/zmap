/*  File: zmapViewServer.hpp
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
 * Description: 
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEWSERVER_H
#define ZMAP_VIEWSERVER_H


#include <ZMap/zmapEnum.hpp>
#include <ZMap/zmapUrl.hpp>
#include <ZMap/zmapThreads.hpp>
#include <ZMap/zmapView.hpp>
#include <ZMap/zmapServerProtocol.hpp>




/* Malcolm has introduced this and put it in the threads package where it is NOT used
 * so I have moved it here....but is this necessary....revisit his logic....
 * Is it necessary.....revisit this...... */
#define ZMAP_VIEW_THREAD_STATE_LIST(_)                                                           \
_(THREAD_STATUS_INVALID,    , "Invalid",       "In valid thread state.",            "") \
_(THREAD_STATUS_OK,         , "OK",            "Thread ok.",               "") \
_(THREAD_STATUS_PENDING,    , "Pending",       "Thread pending....means what ?",       "") \
_(THREAD_STATUS_FAILED,     , "Failed",        "Thread has failed (needs killing ?).", "")

ZMAP_DEFINE_ENUM(ZMapViewThreadStatus, ZMAP_VIEW_THREAD_STATE_LIST) ;




typedef struct ZMapViewSessionServerStructType  *ZMapViewSessionServer ;




// I'm not sure how much of the below needs exposing.....


/* Forward declaration need for viewconnection. */
typedef struct _ZMapViewConnectionStepListStruct *ZMapViewConnectionStepList ;


/* Represents a single connection to a data source.
 * We maintain state for our connections otherwise we will try to issue requests that
 * they may not see. curr_request flip-flops between ZMAP_REQUEST_GETDATA and ZMAP_REQUEST_WAIT */
typedef struct ZMapViewConnectionStructType
{
  ZMapView parent_view ;

  char *url ;						    /* For displaying in error messages etc. */


  ZMapViewSessionServer server ;			    /* Server data. */


  ZMapThreadRequest curr_request ;

  ZMapThread thread ;

  gpointer request_data ;				    /* User data pointer, needed for step implementation. */

  ZMapViewConnectionStepList step_list ;		    /* List of steps required to get data from server. */


  /* UM...IS THIS THE RIGHT PLACE FOR THIS....DOES A VIEWCONNECTION REPRESENT A SINGLE THREAD....
   * CHECK THIS OUT.......
   *  */
  ZMapViewThreadStatus thread_status ;			    /* probably this is badly placed,
							       but not as badly as before
							       at least it has some persistance now */

  gboolean show_warning ;

} ZMapViewConnectionStruct, *ZMapViewConnection ;



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


/* Callbacks returning a boolean should return TRUE if the connection should continue,
 * FALSE if it's failed, in this case the connection is immediately removed from the steplist. */

/* Callback for dispatching step requests. */
typedef gboolean (*StepListDispatchCB)(ZMapViewConnection connection, ZMapServerReqAny req_any) ;

/* Callback for processing step requests. */
typedef gboolean (*StepListProcessDataCB)(ZMapViewConnection connection, ZMapServerReqAny req_any) ;

/* Callback for freeing step requests. */
typedef void (*StepListFreeDataCB)(ZMapServerReqAny req_any) ;


/* Represents a request composed of a list of dispatchable steps
 * which should be executed one at a time, waiting until each step is completed
 * before executing the next. */
typedef struct _ZMapViewConnectionStepListStruct
{
  GList *current ;                                  /* Step being currently managed, set
                                                 to first step on initialisation,
                                                 NULL on termination. */

  GList *steps ;                              /* List of ZMapViewConnectionStep to
                                                 be dispatched. */

  StepListDispatchCB dispatch_func ;                      /* Called prior to dispatching requests. */
  StepListProcessDataCB process_func ;                    /* Called when each request is processed. */
  StepListFreeDataCB free_func ;                    /* Called to free requests. */

  /* we may need a list of all connections here. */
} ZMapViewConnectionStepListStruct ;


/* Represents a sub-step to a connection that forms part of a step. */
typedef struct _ZMapViewConnectionRequestStruct
{
  StepListStepState state ;

  gboolean has_failed ;                             /* Set TRUE if callback returns FALSE. */

  ZMapThreadReply reply ;                           /* Return code from request. */

  gpointer request_data ;                 // pointer to parent connection's data

} ZMapViewConnectionRequestStruct, *ZMapViewConnectionRequest ;


/* Represents a dispatchable step that is part of an overall request. */
typedef struct _ZMapViewConnectionStepStruct
{
  StepListStepState state ;

  StepListActionOnFailureType on_fail ;                   /* What action to take if any part of
                                                 this step fails. */

  gdouble start_time ;                              /* start/end time of step. */
  gdouble end_time ;

  ZMapServerReqType request ;                       /* Type of request. */

  ZMapViewConnectionRequest connection_req ;

} ZMapViewConnectionStepStruct, *ZMapViewConnectionStep ;


/* Struct describing features loaded. */
typedef struct LoadFeaturesDataStructType
{
  char *err_msg;                                            /* from the server mainly */
  gchar *stderr_out;
  gint exit_code;

  gboolean status;                                          /* load sucessful? */


  /* why is this here ????? */
  unsigned long xwid ;                                      /* X Window id for the xremote widg. */

  GList *feature_sets ;
  int start, end ;                                          /* requested coords */


  ZMapFeatureContextMergeStatsStruct merge_stats ;          /* Stats from merge of features into existing
                                                             * context. */


} LoadFeaturesDataStruct, *LoadFeaturesData ;






/* State for a single connection to a data source. */
typedef struct ConnectionDataStructType
{
  /* Processing options. */

  gboolean dynamic_loading ;


  /* Context data. */


  /* database data. */
  ZMapServerReqGetServerInfoStruct session ;


  /* Why are start/end separate...are they not in sequence_map ??? */
  ZMapFeatureSequenceMap sequence_map;
  gint start,end;

  /* Move into loaded_features ? */
  GError *error;
  gchar *stderr_out;
  gint exit_code;
  gboolean status;                                            // load sucessful?

  GList *feature_sets ;

  GList *required_styles ;
  gboolean server_styles_have_mode ;

  GHashTable *column_2_styles ;                                    /* Mapping of each column to all
                                                               the styles it requires. */

  GHashTable *featureset_2_column;                            /* needed by pipeServers */
  GHashTable *source_2_sourcedata;

  ZMapStyleTree *curr_styles ;                                    /* Styles for this context. */
  ZMapFeatureContext curr_context ;

  ZMapServerReqGetFeatures get_features;                    /* features got from the server,
                                                               save for display after checking status */

  /* Oh gosh this is hokey....ugh....... */
  ZMapServerReqType display_after ;                            /* what step to display features after */

  LoadFeaturesData loaded_features ;                            /* List of feature sets loaded for this connection. */

} ConnectionDataStruct, *ConnectionData ;






void zMapServerSetScheme(ZMapViewSessionServer server, ZMapURLScheme scheme) ;
void zMapServerFormatSession(ZMapViewSessionServer server_data, GString *session_text) ;


ZMapViewConnection zMapServerCreateViewConnection(ZMapView zmap_view,
                                                  ZMapViewConnection view_con,
                                                  ZMapFeatureContext context,
                                                  char *url, char *format, int timeout, char *version,
                                                  gboolean req_styles, char *styles_file,
                                                  GList *req_featuresets, GList *req_biotypes,
                                                  gboolean dna_requested,
                                                  gint start,gint end,
                                                  gboolean terminate);
void zMapServerDestroyViewConnection(ZMapView view, ZMapViewConnection view_conn) ;

LoadFeaturesData zMapServerCreateLoadFeatures(GList *feature_sets) ;
LoadFeaturesData zMapServerCopyLoadFeatures(LoadFeaturesData loaded_features_in) ;
void zMapServerDestroyLoadFeatures(LoadFeaturesData loaded_features) ;



#endif /* !ZMAP_VIEWSERVER_H */
