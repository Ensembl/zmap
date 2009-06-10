/*  File: zmapView_P.h
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 * HISTORY:
 * Last edited: Jun 10 16:12 2009 (edgrif)
 * Created: Thu May 13 15:06:21 2004 (edgrif)
 * CVS info:   $Id: zmapView_P.h,v 1.48 2009-06-10 15:13:08 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>
#include <ZMap/zmapServerProtocol.h>
#include <ZMap/zmapThreads.h>
#include <ZMap/zmapView.h>
#include <ZMap/zmapWindow.h>
#include <ZMap/zmapWindowNavigator.h>
#include <ZMap/zmapXRemote.h>



typedef enum
  {
    BLIXEM_NO_FLAG           = 0,
    BLIXEM_OBEY_DNA_SETS     = 1 << 0,
    BLIXEM_OBEY_PROTEIN_SETS = 1 << 1,
    BLIXEM_SINGLE_FEATURE    = 1 << 2,
  } ZMapViewBlixemFlags;



/* IF WE REFACTORED VIEW TO HAVE NO WINDOWS ETC THEN THIS COULD GO.... */
/* We have this because it enables callers to call on a window but us to get the corresponding view. */
typedef struct _ZMapViewWindowStruct
{
  ZMapView parent_view ;
  
  ZMapWindow window ;

} ZMapViewWindowStruct ;



/* Represents a single connection to a data source. */
/* We need to maintain state for our connections otherwise we will try to issue requests that
 * they may not see. curr_request flip-flops between ZMAP_REQUEST_GETDATA and ZMAP_REQUEST_WAIT */
typedef struct _ZMapViewConnectionStruct
{
  /* I think we need some sort of status here.... */

  char *url ;						    /* For displaying in error messages etc. */

  /* THESE SHOULD GO, THEY NEED TO BE REPLACED BY THE NEW STEP STRUCT STUFF....... */
  /* Record whether this connection will serve up raw sequence or deal with feature edits. */
  gboolean sequence_server ;
  gboolean writeback_server ;
  /*                                                                               */

  ZMapView parent_view ;

  ZMapThreadRequest curr_request ;

  ZMapThread thread ;

  gpointer request_data ;				    /* User data pointer, needed for step implementation. */

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


/* 
 * Callbacks returning a boolean should return TRUE if the connection should continue,
 * FALSE if it's failed, in this case the connection is immediately removed from the steplist.
 *
 */

/* Callback for dispatching step requests. */
typedef gboolean (*StepListDispatchCB)(ZMapViewConnection connection, ZMapServerReqAny req_any) ;

/* Callback for processing step requests. */
typedef gboolean (*StepListProcessDataCB)(ZMapViewConnection connection, ZMapServerReqAny req_any) ;

/* Callback for freeing step requests. */
typedef void (*StepListFreeDataCB)(ZMapViewConnection connection, ZMapServerReqAny req_any) ;


/* Represents a request composed of a list of dispatchable steps
 * which should be executed one at a time, waiting until each step is completed
 * before executing the next. */
typedef struct _ZMapViewConnectionStepListStruct
{
  GList *current ;					    /* Step being currently managed, set
							       to first step on initialisation,
							       NULL on termination. */

  GList *steps ;					    /* List of ZMapViewConnectionStep to
							       be dispatched. */

  StepListDispatchCB dispatch_func ;			    /* Called prior to dispatching requests. */
  StepListProcessDataCB process_func ;			    /* Called when each request is processed. */
  StepListFreeDataCB free_func ;			    /* Called to free requests. */

  /* we may need a list of all connections here. */
} ZMapViewConnectionStepListStruct, *ZMapViewConnectionStepList ;


/* Represents a dispatchable step that is part of an overall request. */
typedef struct _ZMapViewConnectionStepStruct
{
  StepListStepState state ;

  StepListActionOnFailureType on_fail ;			    /* What action to take if any part of
							       this step fails. */

  gdouble start_time ;					    /* start/end time of step. */
  gdouble end_time ;

  ZMapServerReqType request ;				    /* Type of request. */

  GList *connections ;					    /* List of ZMapViewConnectionRequest. */
} ZMapViewConnectionStepStruct, *ZMapViewConnectionStep ;


/* Represents a sub-step to a connection that forms part of a step. */
typedef struct _ZMapViewConnectionRequestStruct
{
  StepListStepState state ;

  gboolean has_failed ;					    /* Set TRUE if callback returns FALSE. */

  ZMapThreadReply reply ;				    /* Return code from request. */

  ZMapViewConnectionStep step ;				    /* Parent step. */

  ZMapViewConnection connection ;

  gpointer request_data ;

} ZMapViewConnectionRequestStruct, *ZMapViewConnectionRequest ;





/* A "View" is a set of one or more windows that display data retrieved from one or
 * more servers. Note that the "View" windows are _not_ top level windows, they are panes
 * within a container widget that is supplied as a parent of the View then the View
 * is first created.
 * Each View has lists of windows and lists of connections, the view handles these lists
 * using zmapWindow and zmapConnection calls.
 * */
typedef struct _ZMapViewStruct
{
  ZMapViewState state ;					    /* Overall state of the ZMapView. */

  gboolean busy ;					    /* Records when we are busy so can
							       block user interaction. */

  GtkWidget *xremote_widget ;				    /* Widget that receives xremote
							       commands from external program
							       running zmap. */
  unsigned long xwid ;					    /* X Window id for the xremote widg. */

  guint idle_handle ;

  void *app_data ;					    /* Passed back to caller from view
							       callbacks. */

  char *view_name ;					    /* An overall label for the view,
							       defaults to the master sequence name. */

  /* THIS NEEDS TO GO I THINK, IT WILL BE REDUNDANT IN THE WORLD OF MULTIPLE BLOCKS ETC....
   * WE NEED A VIEW NAME INSTEAD.... */
  /* NOTE TO EG....DO WE NEED THESE HERE ANY MORE ???? */
  /* This specifies the "context" of the view, i.e. which section of virtual sequence we are
   * interested in. */
  gchar *sequence ;
  int start, end ;


  /* This will need to be a full mapping of sequences, blocks etc in the end which will
   * be used both to set up the feature context so the right bits of feature get fetched
   * but also to control the display of the blocks so they appear in the correct positions
   * on the screen. */
  GList *sequence_mapping ;				    /* Of ZMapViewSequenceFetch, i.e. list
							       of sequences to be displayed. */
#ifdef NOT_REQUIRED_ATM
  GList *sequence_2_server ;				    /* Some sequences may only be
							       fetchable from some servers. */
#endif /* NOT_REQUIRED_ATM */


  int connections_loaded ;				    /* Record of number of connections
							     * loaded so for each reload. */
  GList *connection_list ;				    /* Of ZMapViewConnection. */
  ZMapViewConnection sequence_server ;			    /* Which connection to get raw
							       sequence from. */
  ZMapViewConnection writeback_server ;			    /* Which connection to send edits to. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ActionOnFailureType on_fail ;				    /* Action to take if a sub-step
							       fails. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  ZMapViewConnectionStepList step_list ;		    /* List of steps required to get data from server. */


  /* The features....needs thought as to how this updated/constructed..... */
  ZMapFeatureContext features ;

  /* Original styles, these are all the styles as they were loaded from the server(s).
   * N.B. the list may be updated during the lifetime of the view and hence is always
   * passed into window for all update operations. */
  GData *orig_styles ;

  GHashTable *featureset_2_stylelist ;			    /* Mapping of each feature_set to all
							       the styles it requires. */


  /* We need to know if the user has done a revcomp for a few reasons to do with coord
   * transforms and the way annotation is done....*/
  gboolean revcomped_features ;


  /* Be good to get rid of this window stuff in any restructure..... */
  GList *window_list ;					    /* Of ZMapViewWindow. */
  ZMapWindowNavigator navigator_window ;


  ZMapXRemoteObj xremote_client;

  GList *navigator_set_names;
  
  /* view spawns blixem processes when requested, kill_blixems flag controls whether they are
   * killed when view dies (default is TRUE). */
  gboolean kill_blixems ;
  GList *spawned_processes;

  GHashTable *cwh_hash;


  /* Struct to hold textual information about all connected data sources. */
  ZMapViewSession session_data ;


} ZMapViewStruct ;





void zmapViewBusy(ZMapView zmap_view, gboolean busy) ;
gboolean zmapAnyConnBusy(GList *connection_list) ;
char *zmapViewGetStatusAsStr(ZMapViewState state) ;
gboolean zmapViewBlixemLocalSequences(ZMapView view, ZMapFeature feature, GList **local_sequences_out) ;
gboolean zmapViewCallBlixem(ZMapView view, ZMapFeature feature, GList *local_sequences,
			    ZMapViewBlixemFlags flags, GPid *child_pid, gboolean *kill_on_exit) ;
ZMapFeatureContext zmapViewMergeInContext(ZMapView view, ZMapFeatureContext context);
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context);
void zmapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout);
void zmapViewSetupXRemote(ZMapView view, GtkWidget *widget);
gboolean zmapViewRemoteSendCommand(ZMapView view,
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

void zmapViewSessionAddServer(ZMapViewSession session_data, ZMapURL url, char *format) ;
void zmapViewSessionAddServerInfo(ZMapViewSession session_data, char *database_path) ;
void zmapViewSessionFreeServer(gpointer data, gpointer user_data_unused) ;

ZMapViewConnectionStepList zmapViewStepListCreate(StepListDispatchCB dispatch_func,
						  StepListProcessDataCB request_func,
						  StepListFreeDataCB free_func) ;
void zmapViewStepListAddStep(ZMapViewConnectionStepList step_list, ZMapServerReqType request_type,
			     StepListActionOnFailureType on_fail) ;
ZMapViewConnectionRequest zmapViewStepListAddServerReq(ZMapViewConnectionStepList step_list, 
						       ZMapViewConnection view_con,
						       ZMapServerReqType request_type,
						       gpointer request_data) ;
void zmapViewStepListIter(ZMapViewConnectionStepList step_list) ;
void zmapViewStepListStepProcessRequest(ZMapViewConnectionStepList step_list, ZMapViewConnectionRequest request) ;
ZMapViewConnectionRequest zmapViewStepListFindRequest(ZMapViewConnectionStepList step_list,
						      ZMapServerReqType request_type, ZMapViewConnection connection) ;
void zmapViewStepListStepConnectionDeleteAll(ZMapViewConnectionStepList step_list, ZMapViewConnection connection) ;
gboolean zmapViewStepListAreConnections(ZMapViewConnectionStepList step_list) ;
gboolean zmapViewStepListIsNext(ZMapViewConnectionStepList step_list) ;
void zmapViewStepDestroy(gpointer data, gpointer user_data) ;
void zmapViewStepListDestroy(ZMapViewConnectionStepList step_list) ;

void zmapViewLoadFeatures(ZMapView view, ZMapFeatureBlock block_orig, GList *req_featuresets,
			  int features_start, int features_end) ;



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
