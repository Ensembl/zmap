/*  File: zmapView_P.h
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description:
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>
#include <ZMap/zmapConfigStanzaStructs.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapWindowNavigator.h>
#include <ZMap/zmapXRemote.h>




/* IF WE REFACTORED VIEW TO HAVE NO WINDOWS ETC THEN THIS COULD GO.... */
/* We have this because it enables callers to call on a window but us to get the corresponding view. */
typedef struct _ZMapViewWindowStruct
{
  ZMapView parent_view ;

  ZMapWindow window ;

} ZMapViewWindowStruct ;




/* 
 * Session data, each connection has various bits of information recorded about it
 * depending on what type of connection it is (http, acedb, pipe etc).
 */

typedef struct ZMapViewSessionServerStructName
{
  ZMapURLScheme scheme ;
  char *url ;
  char *protocol ;
  char *format ;

  union							    /* Keyed by scheme field above. */
  {
    struct
    {
      char *host ;
      int port ;
      char *database ;
    } acedb ;

    struct
    {
      char *path ;
    } file ;

    struct
    {
      char *path ;
      char *query ;
    } pipe ;
  } scheme_data ;

} ZMapViewSessionServerStruct, *ZMapViewSessionServer ;


/* Malcolm has introduced this and put it in the threads package where it is NOT used.
 * Is it necessary.....revisit this...... */
typedef enum
  {
    THREAD_STATUS_INVALID,
    THREAD_STATUS_FAILED,				    /* Thread has failed (and needs killing ?). */
    THREAD_STATUS_PENDING,				    /* ????? */
    THREAD_STATUS_OK					    /* Thread functioning normally. */
  } ZMapViewThreadStatus ;



typedef struct _ZMapViewConnectionStepListStruct *ZMapViewConnectionStepList ;


/* Represents a single connection to a data source. 
 * We maintain state for our connections otherwise we will try to issue requests that
 * they may not see. curr_request flip-flops between ZMAP_REQUEST_GETDATA and ZMAP_REQUEST_WAIT */
typedef struct ZMapViewConnectionStructName
{
  ZMapView parent_view ;

  char *url ;						    /* For displaying in error messages etc. */

  ZMapViewSessionServerStruct session ;			    /* Session data. */

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




/* Holds data about feature sets loaded. */
typedef struct ZMapViewLoadFeaturesDataStructName
{
  char *err_msg;        // from the server mainly
  gchar *stderr_out;
  gint exit_code;
  int num_features;

  GList *feature_sets ;
  int start,end;        // requested coords
  gboolean status;      // load sucessful?
  unsigned long xwid ;  // X Window id for the xremote widg. */

} ZMapViewLoadFeaturesDataStruct, *ZMapViewLoadFeaturesData ;




/* A "View" is a set of one or more windows that display data retrieved from one or
 * more servers. Note that the "View" windows are _not_ top level windows, they are panes
 * within a container widget that is supplied as a parent of the View then the View
 * is first created.zMapFeatureSetCreateID
 * Each View has lists of windows and lists of connections, the view handles these lists
 * using zmapWindow and zmapConnection calls.
 * */
typedef struct _ZMapViewStruct
{
  ZMapViewState state ;					    /* Overall state of the ZMapView. */

  gboolean busy ;					    /* Records when we are busy so can
							       block user interaction. */
  gboolean thread_fail_silent;				    /* don't report failures on screen */
  gboolean serial_load;					    /* load pipe servers in series on startup */


  gboolean remote_control ;


  /* THESE WILL NOT BE NEEDED ANY MORE.. */
  GtkWidget *xremote_widget ;				    /* Widget that receives xremote
							       commands from external program
							       running zmap. */
  unsigned long xwid ;					    /* X Window id for the xremote widg. */


  gulong map_event_handler ;				    /* map event handler id for xremote widget. */

  guint idle_handle ;

  void *app_data ;					    /* Passed back to caller from view
							       callbacks. */

  char *view_name ;					    /* An overall label for the view,
							       defaults to the master sequence name. */

  /* These seem like they are redundant and not sensible anyway...there won't just be one name.... */
  char *view_db_name ;
  char *view_db_title ;


  ZMapFeatureSequenceMap view_sequence ;


  /* This will need to be a full mapping of sequences, blocks etc in the end which will
   * be used both to set up the feature context so the right bits of feature get fetched
   * but also to control the display of the blocks so they appear in the correct positions
   * on the screen. */
  GList *sequence_mapping ;				    /* Of ZMapViewSequenceFetch, i.e. list
							       of sequences to be displayed. */

#ifdef NOT_REQUIRED_ATM
  /* OH YES IT IS......WE DO NEED TO KNOW THIS..... */

  GList *sequence_2_server ;				    /* Some sequences may only be
							       fetchable from some servers. */
#endif /* NOT_REQUIRED_ATM */


  int sources_loading ;					    /* how many active/queued requests,
							       this is not very neat as failures
							       are dealt with in a complex way */
  int sources_failed ;					    /* A count of the number that failed
							       since the last data request, is
							       reset on next load command */


  GList *connection_list ;				    /* Of ZMapViewConnection. */
  ZMapViewConnection sequence_server ;			    /* Which connection to get raw
							       sequence from. */

  /* The features....needs thought as to how this updated/constructed..... */
  ZMapFeatureContext features ;

  ZMapFeatureContextMapStruct context_map;  /* all the data mapping featuresets columns and styles */
  /* it may be good to combine context_map and context
   * but that might mean a lot of work on on the server modules
   */

  /* Original styles, these are all the styles as they were loaded from the server(s).
   * N.B. the list may be updated during the lifetime of the view and hence is always
   * passed into window for all update operations. */
/*  GHashTable *orig_styles ; */

/*  GHashTable *featureset_2_stylelist ;	   Mapping of each feature_set to all the styles
							 * the styles it requires. using a GHashTable of
                                           * GLists of style quark id's.
                                           *
                                           * NB: this stores data from ZMap config
                                           * sections [featureset_styles] _and_ [column_styles]
                                           * _and_ ACEDB
                                           * collisions are merged
                                           * Columns treated as fake featuresets so as to have a style
                                           */

/*  GHashTable *featureset_2_column ;        Mapping of a feature source to a column using ZMapGFFSet
                                           * NB: this contains data from ZMap config
                                           * sections [columns] [Column_description] _and_ ACEDB
                                           */

/*  GHashTable *source_2_sourcedata ;        Mapping of a feature source to its data using ZMapGFFSource
                                           * This consists of style id and description and source id
                                           * NB: the GFFSource.source  (quark) is the GFF_source name
                                           * the hash table is indexed by the featureset name quark
                                           */

/*  GHashTable *columns;                    All the columns that ZMap will display
                                           * stored as ZMapGFFSet
                                           * These may contain several featuresets each
                                           * They are in display order left to right
                                           */
  gboolean columns_set;                   // if set from config style use config only
                                          // else use source featuresets in order as of old

  gboolean highlight_filtered_columns;    /* True if filtered columns should be highlighted */
  
  /* We need to know if the user has done a revcomp for a few reasons to do with coord
   * transforms and the way annotation is done....*/
  gboolean revcomped_features ;


  /* Be good to get rid of this window stuff in any restructure..... */
  GList *window_list ;					    /* Of ZMapViewWindow. */


  ZMapWindowNavigator navigator_window ;


  ZMapXRemoteObj xremote_client;

  /* Crikey, why is this here...??? Maybe it's ok...but seems more like a window thing..... */
  GList *navigator_set_names;


  /* view spawns blixem processes when requested, kill_blixems flag controls whether they are
   * killed when view dies (default is TRUE). */
  gboolean kill_blixems ;
  GList *spawned_processes ;


  GHashTable *cwh_hash ;


} ZMapViewStruct ;







ZMapViewCallbacks zmapViewGetCallbacks(void) ;

void zmapViewBusyFull(ZMapView zmap_view, gboolean busy, const char *file, const char *function) ;
#define zmapViewBusy(VIEW, BUSY) \
  zmapViewBusyFull((VIEW), (BUSY), __FILE__, __PRETTY_FUNCTION__)

gboolean zmapAnyConnBusy(GList *connection_list) ;
char *zmapViewGetStatusAsStr(ZMapViewState state) ;

gboolean zmapViewBlixemLocalSequences(ZMapView view, ZMapFeatureBlock block, ZMapHomolType align_type,
				      int offset, int position,
				      ZMapFeatureSet feature_set, GList **local_sequences_out) ;
gboolean zmapViewCallBlixem(ZMapView view, ZMapFeatureBlock block,
			    ZMapHomolType homol_type,
			    int offset,
			    int position,
			    int window_start, int window_end,
			    int mark_start, int mark_end,
			    ZMapWindowAlignSetType align_set,
			    gboolean isSeq,
			    GList *features, ZMapFeatureSet feature_set,
			    GList *source, GList *local_sequences,
			    GPid *child_pid, gboolean *kill_on_exit) ;

ZMapFeatureContext zmapViewMergeInContext(ZMapView view, ZMapFeatureContext context);
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context, ZMapFeature highlight_feature) ;
void zmapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout);

void zmapViewSetupXRemote(ZMapView view, GtkWidget *widget);
ZMapXRemoteSendCommandError zmapViewRemoteSendCommand(ZMapView view,
						      char *action, GArray *xml_events,
						      ZMapXMLObjTagFunctions start_handlers,
						      ZMapXMLObjTagFunctions end_handlers,
						      gpointer *handler_data);

/* Context Window Hash (CWH) for the correct timing of the call to zMapFeatureContextDestroy */
GHashTable *zmapViewCWHHashCreate(void);
void zmapViewCWHSetList(GHashTable *hash, ZMapFeatureContext context, GList *list);
gboolean zmapViewCWHIsLastWindow(GHashTable *hash, ZMapFeatureContext context, ZMapWindow window);
gboolean zmapViewCWHRemoveContextWindow(GHashTable *table, ZMapFeatureContext *context,
                                        ZMapWindow window, gboolean *is_only_context);
void zmapViewCWHDestroy(GHashTable **hash);

void zmapViewSessionAddServer(ZMapViewSessionServer session_data, ZMapURL url, char *format) ;
void zmapViewSessionAddServerInfo(ZMapViewSessionServer session_data, ZMapServerReqGetServerInfo session) ;
void zmapViewSessionFreeServer(ZMapViewSessionServer session_data) ;

ZMapViewConnection zmapViewRequestServer(ZMapView view, ZMapViewConnection view_conn,
					 ZMapFeatureBlock block_orig, GList *req_featuresets,
					 gpointer server, /* ZMapConfigSource */
					 int req_start, int req__end,
					 gboolean dna_requested, gboolean terminate, gboolean show_warning) ;

ZMapViewConnectionStepList zmapViewStepListCreate(StepListDispatchCB dispatch_func,
						  StepListProcessDataCB request_func,
						  StepListFreeDataCB free_func) ;
void zmapViewStepListAddStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type,
			     StepListActionOnFailureType on_fail) ;
ZMapViewConnectionRequest zmapViewStepListAddServerReq(ZMapViewConnectionStepList step_list,
						       ZMapViewConnection view_con,
						       ZMapServerReqType request_type,
						       gpointer request_data,
                                           StepListActionOnFailureType on_fail) ;
ZMapViewConnectionStepList zmapViewConnectionStepListCreate(StepListDispatchCB dispatch_func,
                                      StepListProcessDataCB process_func,
                                      StepListFreeDataCB free_func);
void zmapViewStepListIter(ZMapViewConnection view_con) ;
void zmapViewStepListStepProcessRequest(ZMapViewConnection view_con, ZMapViewConnectionRequest request) ;
ZMapViewConnectionRequest zmapViewStepListFindRequest(ZMapViewConnectionStepList step_list,
						      ZMapServerReqType request_type, ZMapViewConnection connection) ;
void zmapViewStepListStepConnectionDeleteAll(ZMapViewConnection connection) ;
gboolean zmapViewStepListAreConnections(ZMapViewConnectionStepList step_list) ;
gboolean zmapViewStepListIsNext(ZMapViewConnectionStepList step_list) ;
void zmapViewStepDestroy(gpointer data, gpointer user_data) ;
void zmapViewStepListDestroy(ZMapViewConnectionStepList step_list) ;

void zmapViewLoadFeatures(ZMapView view, ZMapFeatureBlock block_orig, GList *req_featuresets,
			  ZMapConfigSource server,
			  int features_start, int features_end,
			  gboolean group, gboolean make_new_connection, gboolean terminate) ;


GQuark zmapViewSrc2FSetGetID(GHashTable *source_2_featureset, char *source_name) ;
GList *zmapViewSrc2FSetGetList(GHashTable *source_2_featureset, GList *source_list) ;

/* zmapViewFeatureMask.c */
GList *zMapViewMaskFeatureSets(ZMapView view, GList *feature_set_names);

gboolean zMapViewCollapseFeatureSets(ZMapView view, ZMapFeatureContext diff_context);



#ifdef LOTS_OF_EXONS
#define ZMAP_VIEW_REMOTE_SEND_XML_TEST "<zmap>\
<response handled=\"true\">\
        <notebook>\
<chapter>\
 <page name=\"Exons\">\
    <subsection>\
      <paragraph columns=\"&apos;#&apos; &apos;Start&apos; &apos;End&apos; &apos;Stable ID&apos;\" \
                    type=\"compound_table\" \
            column_types=\"int int int string\">\
        <tagvalue type=\"compound\">1 80362 80403 OTTHUME00001520851</tagvalue>\
        <tagvalue type=\"compound\">2 80795 80954 OTTHUME00000335753</tagvalue>\
        <tagvalue type=\"compound\">3 81504 81627 OTTHUME00000335738</tagvalue>\
        <tagvalue type=\"compound\">4 83218 83298 OTTHUME00000335783</tagvalue>\
        <tagvalue type=\"compound\">5 83449 83511 OTTHUME00000335747</tagvalue>\
        <tagvalue type=\"compound\">6 84245 84366 OTTHUME00000335767</tagvalue>\
        <tagvalue type=\"compound\">7 84480 84640 OTTHUME00001730688</tagvalue>\
        <tagvalue type=\"compound\">8 84887 84922 OTTHUME00000335731</tagvalue>\
        <tagvalue type=\"compound\">9 85578 85660 OTTHUME00000335746</tagvalue>\
        <tagvalue type=\"compound\">10 87728 87773 OTTHUME00000335742</tagvalue>\
        <tagvalue type=\"compound\">11 93642 93780 OTTHUME00000335765</tagvalue>\
        <tagvalue type=\"compound\">12 93908 93970 OTTHUME00000335739</tagvalue>\
        <tagvalue type=\"compound\">13 98506 98634 OTTHUME00000335771</tagvalue>\
        <tagvalue type=\"compound\">14 98889 98961 OTTHUME00000335795</tagvalue>\
        <tagvalue type=\"compound\">15 99107 99216 OTTHUME00000335764</tagvalue>\
        <tagvalue type=\"compound\">16 99438 99518 OTTHUME00000335756</tagvalue>\
        <tagvalue type=\"compound\">17 99766 99853 OTTHUME00000335784</tagvalue>\
        <tagvalue type=\"compound\">18 100127 100316 OTTHUME00000335755</tagvalue>\
        <tagvalue type=\"compound\">19 100431 100557 OTTHUME00000335730</tagvalue>\
        <tagvalue type=\"compound\">20 101031 101180 OTTHUME00000335740</tagvalue>\
        <tagvalue type=\"compound\">21 101529 101603 OTTHUME00000335734</tagvalue>\
        <tagvalue type=\"compound\">22 101813 101956 OTTHUME00000335782</tagvalue>\
        <tagvalue type=\"compound\">23 102159 102206 OTTHUME00001520845</tagvalue>\
        <tagvalue type=\"compound\">24 102453 102526 OTTHUME00000335773</tagvalue>\
        <tagvalue type=\"compound\">25 102761 103017 OTTHUME00000335798</tagvalue>\
      </paragraph>\
    </subsection>\
  </page>\
</chapter>\
        </notebook>\
</response> \
</zmap>"
#endif /* LOTS_OF_EXONS */


#ifdef RT_66037
#define ZMAP_VIEW_REMOTE_SEND_XML_TEST "<zmap>\
<response handled=\"true\">\
  <notebook>\
    <chapter>\
      <page name=\"Details\">\
        <subsection name=\"Feature\">\
          <paragraph type=\"tagvalue_table\">\
            <tagvalue name=\"Taxon ID\" type=\"simple\">9606</tagvalue>\
            <tagvalue name=\"Description\" type=\"scrolled_text\">Homo\
sapiens vacuolar protein sorting protein 16 (VPS16) mRNA, complete\
cds.</tagvalue><!--\
          </paragraph>\
        </subsection>\
      </page>\
      <page name=\"Details\">\
        <subsection name=\"Feature\">\
          <paragraph type=\"tagvalue_table\">-->\
            <tagvalue name=\"Evidence for transcript\" type=\"simple\">RP11-12M19.2-001</tagvalue>\
            <tagvalue name=\"Evidence for transcript\" type=\"simple\">RP11-12M19.2-002</tagvalue>\
          </paragraph>\
        </subsection>\
      </page>\
    </chapter>\
</notebook>\
</response>\
</zmap>"
#endif /* RT_66037 */


#ifdef FULL_LACE_EXAMPLE
#define ZMAP_VIEW_REMOTE_SEND_XML_TEST "<zmap>\
<response handled=\"true\">\
        <notebook>\
<chapter>\
  <page name=\"Details\">\
    <subsection name=\"Feature\">\
      <paragraph name=\"Locus\" type=\"tagvalue_table\">\
        <tagvalue name=\"Symbol\" type=\"simple\">MSH5</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">Em:AF134726.13</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">G7</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">NG23</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">MutSH5</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">MGC2939</tagvalue>\
        <tagvalue name=\"Alias\" type=\"simple\">DKFZp434C1615</tagvalue>\
   <tagvalue name=\"Full name\" type=\"simple\">mutS homolog 5 (E. coli)</tagvalue>\
        <tagvalue name=\"Locus Stable ID\" type=\"simple\">OTTHUMG00000031139</tagvalue>\
      </paragraph>\
    </subsection>\
    <subsection name=\"Annotation\">\
      <paragraph type=\"tagvalue_table\">\
        <tagvalue name=\"Transcript Stable ID\" type=\"simple\">OTTHUMT00000268161</tagvalue>\
      </paragraph>\
      <paragraph type=\"tagvalue_table\">\
        <tagvalue name=\"Annotation remark\" type=\"scrolled_text\">two exon splice variations</tagvalue>\
      </paragraph>\
      <paragraph type=\"tagvalue_table\">-->\
        <tagvalue name=\"Evidence for transcript\" type=\"simple\">RP11-12M19.2-001</tagvalue>\
        <tagvalue name=\"Evidence for transcript\" type=\"simple\">RP11-12M19.2-002</tagvalue>\
      </paragraph>\
      <paragraph name=\"Evidence\" \
              columns=\"&apos;Type&apos; &apos;Name&apos;\" \
                 type=\"compound_table\" \
         column_types=\"string string\" >\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:AL046207.1</tagvalue>\
        <tagvalue type=\"compound\">EST Em:BG424936.1</tagvalue>\
        <tagvalue type=\"compound\">cDNA Em:BC041031.1</tagvalue>\
      </paragraph>\
    </subsection>\
  </page>\
  <page name=\"Exons\">\
    <subsection>\
      <paragraph columns=\"&apos;#&apos; &apos;Start&apos; &apos;End&apos; &apos;Stable ID&apos;\" \
                    type=\"compound_table\" \
            column_types=\"int int int string\">\
        <tagvalue type=\"compound\">1 80362 80403 OTTHUME00001520851</tagvalue>\
        <tagvalue type=\"compound\">2 80795 80954 OTTHUME00000335753</tagvalue>\
        <tagvalue type=\"compound\">3 81504 81627 OTTHUME00000335738</tagvalue>\
        <tagvalue type=\"compound\">4 83218 83298 OTTHUME00000335783</tagvalue>\
        <tagvalue type=\"compound\">5 83449 83511 OTTHUME00000335747</tagvalue>\
        <tagvalue type=\"compound\">6 84245 84366 OTTHUME00000335767</tagvalue>\
        <tagvalue type=\"compound\">7 84480 84640 OTTHUME00001730688</tagvalue>\
        <tagvalue type=\"compound\">8 84887 84922 OTTHUME00000335731</tagvalue>\
        <tagvalue type=\"compound\">9 85578 85660 OTTHUME00000335746</tagvalue>\
        <tagvalue type=\"compound\">10 87728 87773 OTTHUME00000335742</tagvalue>\
        <tagvalue type=\"compound\">11 93642 93780 OTTHUME00000335765</tagvalue>\
        <tagvalue type=\"compound\">12 93908 93970 OTTHUME00000335739</tagvalue>\
        <tagvalue type=\"compound\">13 98506 98634 OTTHUME00000335771</tagvalue>\
        <tagvalue type=\"compound\">14 98889 98961 OTTHUME00000335795</tagvalue>\
        <tagvalue type=\"compound\">15 99107 99216 OTTHUME00000335764</tagvalue>\
        <tagvalue type=\"compound\">16 99438 99518 OTTHUME00000335756</tagvalue>\
        <tagvalue type=\"compound\">17 99766 99853 OTTHUME00000335784</tagvalue>\
        <tagvalue type=\"compound\">18 100127 100316 OTTHUME00000335755</tagvalue>\
        <tagvalue type=\"compound\">19 100431 100557 OTTHUME00000335730</tagvalue>\
        <tagvalue type=\"compound\">20 101031 101180 OTTHUME00000335740</tagvalue>\
        <tagvalue type=\"compound\">21 101529 101603 OTTHUME00000335734</tagvalue>\
        <tagvalue type=\"compound\">22 101813 101956 OTTHUME00000335782</tagvalue>\
        <tagvalue type=\"compound\">23 102159 102206 OTTHUME00001520845</tagvalue>\
        <tagvalue type=\"compound\">24 102453 102526 OTTHUME00000335773</tagvalue>\
        <tagvalue type=\"compound\">25 102761 103017 OTTHUME00000335798</tagvalue>\
        <tagvalue type=\"compound\">14 98889 98961 OTTHUME00000335795</tagvalue>\
        <tagvalue type=\"compound\">15 99107 99216 OTTHUME00000335764</tagvalue>\
        <tagvalue type=\"compound\">16 99438 99518 OTTHUME00000335756</tagvalue>\
        <tagvalue type=\"compound\">17 99766 99853 OTTHUME00000335784</tagvalue>\
        <tagvalue type=\"compound\">18 100127 100316 OTTHUME00000335755</tagvalue>\
        <tagvalue type=\"compound\">19 100431 100557 OTTHUME00000335730</tagvalue>\
        <tagvalue type=\"compound\">20 101031 101180 OTTHUME00000335740</tagvalue>\
        <tagvalue type=\"compound\">21 101529 101603 OTTHUME00000335734</tagvalue>\
        <tagvalue type=\"compound\">22 101813 101956 OTTHUME00000335782</tagvalue>\
        <tagvalue type=\"compound\">23 102159 102206 OTTHUME00001520845</tagvalue>\
        <tagvalue type=\"compound\">24 102453 102526 OTTHUME00000335773</tagvalue>\
        <tagvalue type=\"compound\">25 102761 103017 OTTHUME00000335798</tagvalue>\
      </paragraph>\
    </subsection>\
  </page>\
</chapter>\
        </notebook>\
</response> \
</zmap>"
#endif /* FULL_LACE_EXAMPLE */

#endif /* !ZMAP_VIEW_P_H */
