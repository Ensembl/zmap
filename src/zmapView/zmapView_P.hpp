/*  File: zmapView_P.h
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
 * Description:
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_VIEW_P_H
#define ZMAP_VIEW_P_H

#include <glib.h>

#include <ZMap/zmapConfigStanzaStructs.hpp>
#include <ZMap/zmapServerProtocol.hpp>
#include <ZMap/zmapThreadSource.hpp>
#include <ZMap/zmapView.hpp>
#include <ZMap/zmapWindow.hpp>
#include <ZMap/zmapWindowNavigator.hpp>
#include <ZMap/zmapOldSourceServer.hpp>
#include <ZMap/zmapOldStepList.hpp>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <zmapViewServer.hpp>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/*
 * We follow glib convention in error domain naming:
 *          "The error domain is called <NAMESPACE>_<MODULE>_ERROR"
 */
#define ZMAP_VIEW_ERROR g_quark_from_string("ZMAP_VIEW_ERROR")

/*
 * Some error types for use in the view
 */
typedef enum
{
  ZMAPVIEW_ERROR_STATE,
  ZMAPVIEW_ERROR_SOURCES_LOADING,
  ZMAPVIEW_ERROR_SERVERS,
  ZMAPVIEW_ERROR_CONTEXT_EMPTY,        /* no new features found when merging context */
  ZMAPVIEW_ERROR_CONTEXT_SERIOUS,      /* serious error when merging context */
  ZMAPVIEW_ERROR_CREATING_SOURCE,
  ZMAPVIEW_ERROR_CONNECT
} ZMapViewError ;



/* IF WE REFACTORED VIEW TO HAVE NO WINDOWS ETC THEN THIS COULD GO.... */
/* We have this because it enables callers to call on a window but us to get the corresponding view. */
typedef struct _ZMapViewWindowStruct
{
  ZMapView parent_view ;

  ZMapWindow window ;

} ZMapViewWindowStruct ;



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
typedef struct ZMapConnectionDataStructType
{
  /* Processing options. */

  gboolean dynamic_loading ;


  /* Context data. */
  ZMapView view ;

  ZMapViewConnectionStepList step_list ;		    /* List of steps required to get data
                                                               from server. */


  /* database data. */
  ZMapServerReqGetServerInfoStruct session ;


  /* Why are start/end separate...are they not in sequence_map ??? */
  /* gb10: the request region may be different to the view region (in the sequence map) if
     requesting e.g. from the mark. The sequence name may also be different if we're requesting
     from a server where we need to look up the sequence using a different name, e.g. "chr1"
     rather than "chr1-38". Maybe we could make a copy of the sequence map with these updated
     values in it but I'm not sure that's any better (for most sources we don't need to change
     them so will end up with many unnecessary copies of the sequence map) */
  ZMapFeatureSequenceMap sequence_map;
  gint start,end;
  GQuark req_sequence;

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

} ZMapConnectionDataStruct, *ZMapConnectionData ;











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


  /* Multiple screen support. */
  gboolean multi_screen ;
  int blixem_screen ;


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


  /* Data recording which sources are loading and which failed. */
  GList *sources_loading ;                                  /* how many active/queued requests,
                                                               this is not very neat as failures
                                                               are dealt with in a complex way */
  GList *sources_empty ;                                    /* How many sources are empty. */
  GList *sources_failed ;                                   /* A count of the number that failed
                                                               since the last data request, is
                                                               reset on next load command */

  GList *connection_list ;				    /* Of ZMapViewConnection. */
  ZMapNewDataSource sequence_server ;			    /* Which connection to get raw
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

  int int_values[ZMAPINT_NUM_VALUES] ;    /* int values (also accessible from window level) */

  /* Be good to get rid of this window stuff in any restructure..... */
  GList *window_list ;					    /* Of ZMapViewWindow. */


  ZMapWindowNavigator navigator_window ;

  /* Crikey, why is this here...??? Maybe it's ok...but seems more like a window thing..... */
  GList *navigator_set_names;


  /* view spawns blixem processes when requested, kill_blixems flag controls whether they are
   * killed when view dies (default is TRUE). */
  gboolean kill_blixems ;
  GList *spawned_processes ;

  gboolean xremote_client ;               /* true if this zmap is an xremote client */

  GHashTable *cwh_hash ;

  /* These pointers maintain a list of operations for editing the Scratch feature. edit_list
   * points to the start of the whole list. start/end point to the current subset of valid operations
   * that have not been un-done. */
  GList *edit_list ;
  GList *edit_list_start ;
  GList *edit_list_end ;
  gboolean scratch_start_end_set ;     /* TRUE if the scratch-column forward-strand feature
                                        * start/end has been set */

  GHashTable *feature_cache;           /* cache ZMapFeature pointers for scratch column, key
                                        * on feature's unique_id. Gets cleared when pointers
                                        * are invalidated, i.e. revcomp */

  /* Filenames to use for the Save option in standalone ZMap. Gets set either from the input 
   * file or from the first Save As operation. This is an array indexed on the type of export,
   * e.g. features/config/styles */
  GQuark save_file[ZMAPVIEW_EXPORT_NUM_TYPES] ;

/* gb10: The user can get spammed with loads of messages if we have thousands of sources that all
 * fail. For now, just add a simple hack to disable popup warnings after the first one. This gets
 * reset each time the user does a new Import. Longer term the plan is that we will have a window
 * where users can view source statuses so that we don't have to pop up messages at all. */
  bool disable_popups ;

} ZMapViewStruct ;







ZMapViewCallbacks zmapViewGetCallbacks(void) ;

void zmapViewBusyFull(ZMapView zmap_view, gboolean busy, const char *file, const char *function) ;
#define zmapViewBusy(VIEW, BUSY) \
  zmapViewBusyFull((VIEW), (BUSY), __FILE__, __PRETTY_FUNCTION__)

gboolean zmapAnyConnBusy(GList *connection_list) ;

gboolean zmapViewBlixemLocalSequences(ZMapView view, ZMapFeatureBlock block, ZMapHomolType align_type,
				      int offset, int position,
				      ZMapFeatureSet feature_set, GList **local_sequences_out) ;
gboolean zmapViewCallBlixem(ZMapView view, ZMapFeatureBlock block,
			    ZMapHomolType homol_type,
			    int offset,
			    int position,
			    int window_start, int window_end,
			    int mark_start, int mark_end, const bool features_from_mark,
			    ZMapWindowAlignSetType align_set,
			    gboolean isSeq,
			    GList *features, ZMapFeatureSet feature_set,
			    GList *source, GList *local_sequences,
			    GPid *child_pid, gboolean *kill_on_exit) ;

ZMapFeatureContext zmapViewMergeInContext(ZMapView view,
                                          ZMapFeatureContext context, ZMapFeatureContextMergeStats *merge_stats_out) ;
gboolean zmapViewDrawDiffContext(ZMapView view, ZMapFeatureContext *diff_context, ZMapFeature highlight_feature) ;
void zmapViewResetWindows(ZMapView zmap_view, gboolean revcomp);
void zmapViewEraseFromContext(ZMapView replace_me, ZMapFeatureContext context_inout);

gboolean zmapViewPassCommandToAllWindows(ZMapView view, gpointer user_data) ;

/* Context Window Hash (CWH) for the correct timing of the call to zMapFeatureContextDestroy */
GHashTable *zmapViewCWHHashCreate(void);
void zmapViewCWHSetList(GHashTable *ghash, ZMapFeatureContext context, GList *glist);
gboolean zmapViewCWHIsLastWindow(GHashTable *ghash, ZMapFeatureContext context, ZMapWindow window);
gboolean zmapViewCWHRemoveContextWindow(GHashTable *table, ZMapFeatureContext *context,
                                        ZMapWindow window, gboolean *is_only_context);
void zmapViewCWHDestroy(GHashTable **ghash);


LoadFeaturesData zmapViewCreateLoadFeatures(GList *feature_sets) ;
LoadFeaturesData zmapViewCopyLoadFeatures(LoadFeaturesData loaded_features_in) ;
void zmapViewDestroyLoadFeatures(LoadFeaturesData loaded_features) ;



ZMapNewDataSource zmapViewRequestServer(ZMapView view, ZMapNewDataSource view_conn,
                                        ZMapFeatureBlock block_orig, GList *req_featuresets, GList *req_biotypes,
                                        ZMapConfigSource server,
                                        const char *req_sequence, int req_start, int req__end,
                                        gboolean dna_requested, gboolean terminate, gboolean show_warning) ;

void zmapViewLoadFeatures(ZMapView view, ZMapFeatureBlock block_orig, GList *req_featuresets, GList *req_biotypes,
                          ZMapConfigSource server,
                          const char *req_sequence, int features_start, int features_end, 
                          const bool thread_fail_silent,
                          gboolean group, gboolean make_new_connection, gboolean terminate) ;

GQuark zmapViewSrc2FSetGetID(GHashTable *source_2_featureset, char *source_name) ;
GList *zmapViewSrc2FSetGetList(GHashTable *source_2_featureset, GList *source_list) ;

ZMapFeatureContext zmapViewCreateContext(ZMapView view, GList *feature_set_names, ZMapFeatureSet feature_set);

ZMapFeatureContext zmapViewCopyContextAll(ZMapFeatureContext context,
                                          ZMapFeature feature,
                                          ZMapFeatureSet feature_set,
                                          GList **feature_list,
                                          ZMapFeature *feature_copy_out) ;

void zmapViewMergeNewFeature(ZMapView view, ZMapFeature feature, ZMapFeatureSet feature_set) ;
gboolean zmapViewMergeNewFeatures(ZMapView view,
                                  ZMapFeatureContext *context, ZMapFeatureContextMergeStats *merge_stats_out,
                                  GList **feature_list) ;
void zmapViewEraseFeatures(ZMapView view, ZMapFeatureContext context, GList **feature_list) ;

/* zmapViewFeatureMask.c */
GList *zMapViewMaskFeatureSets(ZMapView view, GList *feature_set_names);

gboolean zMapViewCollapseFeatureSets(ZMapView view, ZMapFeatureContext diff_context);

/* zmapViewScratch.c */
void zmapViewScratchInit(ZMapView zmap_view,
                         ZMapFeatureSequenceMap sequence, ZMapFeatureContext context, ZMapFeatureBlock block);
void zMapViewToggleScratchColumn(ZMapView view, gboolean force_to, gboolean force);
gboolean zmapViewScratchCopyFeatures(ZMapView zmap_view, GList *features,
                                     const long seq_start, const long seq_end,
                                     ZMapFeatureSubPart subpart, const gboolean use_subfeature);
gboolean zmapViewScratchSetCDS(ZMapView zmap_view, GList *features,
                               const long seq_start, const long seq_end,
                               ZMapFeatureSubPart subpart, const gboolean use_subfeature,
                               const gboolean set_cds_start, const gboolean set_cds_end);
gboolean zmapViewScratchDeleteFeatures(ZMapView zmap_view, GList *features,
                                       const long seq_start, const long seq_end,
                                       ZMapFeatureSubPart subpart, const gboolean use_subfeature);
gboolean zmapViewScratchUndo(ZMapView zmap_view);
gboolean zmapViewScratchRedo(ZMapView zmap_view);
gboolean zmapViewScratchClear(ZMapView zmap_view);
gboolean zmapViewScratchSave(ZMapView zmap_view, ZMapFeature feature);
void zmapViewScratchFeatureGetEvidence(ZMapView view, ZMapWindowGetEvidenceCB evidence_cb, gpointer evidence_cb_data) ;
ZMapFeatureSet zmapViewScratchGetFeatureset(ZMapView view);
ZMapFeature zmapViewScratchGetFeature(ZMapView view);
void zmapViewScratchRemoveFeatures(ZMapView view, GList *feature_list) ;
void zmapViewScratchSaveFeature(ZMapView window, GQuark feature_id) ;
void zmapViewScratchSaveFeatureSet(ZMapView window, GQuark feature_set_id) ;
void zmapViewScratchResetAttributes(ZMapView view) ;


gboolean zmapViewSetUpServerConnections(ZMapView zmap_view, GList *settings_list, GError **error) ;


void zmapViewMergeColNames(ZMapView view, GList *names) ;
ZMapFeatureContextMergeCode zmapJustMergeContext(ZMapView view, ZMapFeatureContext *context_inout,
                                                 ZMapFeatureContextMergeStats *merge_stats_out,
                                                 GList **masked,
                                                 gboolean request_as_columns, gboolean revcomp_if_needed) ;
void zmapJustDrawContext(ZMapView view, ZMapFeatureContext diff_context,
                         GList *masked, ZMapFeature highlight_feature,
                         ZMapConnectionData connect_data) ;





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



ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapViewThreadStatus2ExactStr, ZMapViewThreadStatus) ;




#endif /* !ZMAP_VIEW_P_H */
