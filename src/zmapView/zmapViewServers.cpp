/*  File: zmapViewServers.cpp
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
 * Description: Implements setting up connections to server threads.
 *
 * Exported functions: See ZMap/zmapView.hpp
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>

#include <ZMap/zmapGLibUtils.hpp>

#include <zmapView_P.hpp>




typedef struct FindStylesStructType
{
  ZMapStyleTree *all_styles_tree ;
  GHashTable *all_styles_hash ;
  gboolean found_style ;
  GString *missing_styles ;
} FindStylesStruct, *FindStyles ;


typedef struct DrawableDataStructType
{
  char *config_file ;
  gboolean found_style ;
  GString *missing_styles ;
} DrawableDataStruct, *DrawableData ;








static void createColumns(ZMapView view,GList *featuresets) ;
static ZMapConfigSource getSourceFromFeatureset(GHashTable *ghash,GQuark featurequark);
static GHashTable *getFeatureSourceHash(GList *sources) ;

static bool createStepList(ZMapView zmap_view, ZMapNewDataSource view_con, ZMapConnectionData connect_data,
                           ZMapConfigSource server,
                           const ZMapURL urlObj, const char *format, 
                           ZMapFeatureContext context, GList *req_featuresets, GList *req_biotypes,
                           gboolean dna_requested, gboolean req_styles, char *styles_file,
                           gboolean terminate) ;
static gboolean dispatchContextRequests(ZMapServerReqAny req_any, gpointer connection_data) ;
static gboolean processDataRequests(void *user_data, ZMapServerReqAny req_any) ;
static void freeDataRequest(ZMapServerReqAny req_any) ;

static GList *get_required_styles_list(GHashTable *srchash,GList *fsets) ;
static void mergeHashTableCB(gpointer key, gpointer value, gpointer user) ;
static gboolean haveRequiredStyles(ZMapStyleTree &all_styles, GList *required_styles, char **missing_styles_out) ;
static gboolean makeStylesDrawable(char *config_file, ZMapStyleTree &styles, char **missing_styles_out) ;

// Temp....
static void findStyleCB(gpointer data, gpointer user_data) ;
static void drawableCB(ZMapFeatureTypeStyle style, gpointer user_data) ;

static gboolean viewGetFeatures(ZMapView zmap_view,
                                    ZMapServerReqGetFeatures feature_req, ZMapConnectionData connect_data) ;


static bool setUpServerConnectionByScheme(ZMapView zmap_view,
                                          ZMapConfigSource current_server,
                                          bool &terminate,
                                          GError **error) ;



//
//                 External routines
//


/* Set up a connection to a single named server and load features for the whole region.
 * in the view. Does nothing if the server is delayed. Sets the error if there is a problem. */
void zMapViewSetUpServerConnection(ZMapView zmap_view, 
                                   ZMapConfigSource current_server, 
                                   GError **error)
{
  zMapReturnIfFail(zmap_view) ;

  zMapViewSetUpServerConnection(zmap_view, 
                                current_server, 
                                NULL,
                                zmap_view->view_sequence->start,
                                zmap_view->view_sequence->end,
                                zmap_view->thread_fail_silent,
                                error) ;
}




/* Set up a connection to a single named server. Does nothing if the server is delayed.
 * Throws if there is a problem */
void zMapViewSetUpServerConnection(ZMapView zmap_view, 
                                   ZMapConfigSource current_server, 
                                   const char *req_sequence,
                                   const int req_start,
                                   const int req_end,
                                   const bool thread_fail_silent,
                                   GError **error)
{
  zMapReturnIfFail(zmap_view && current_server) ;

  bool terminate = true ;

  if (!current_server->delayed)   // skip if delayed (we'll only request data when asked to)
    {
      /* Check for required fields from config, if not there then we can't connect. */
      if (!current_server->url())
        {
          /* url is absolutely required. Skip if there isn't one. */
          g_set_error(error, ZMAP_VIEW_ERROR, ZMAPVIEW_ERROR_CONNECT,
                      "No url specified in the configuration file so cannot connect to server.") ;
        }
      else
        {
          GList *req_featuresets = NULL ;
          GList *req_biotypes = NULL ;

          if(current_server->featuresets)
            {
              /* req all featuresets  as a list of their quark names. */
              /* we need non canonicalised name to get Capitalised name on the status display */
              req_featuresets = zMapConfigString2QuarkGList(current_server->featuresets,FALSE) ;

              if(!zmap_view->columns_set)
                {
                  createColumns(zmap_view,req_featuresets);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                  g_list_foreach(zmap_view->window_list, invoke_merge_in_names, req_featuresets);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                  zmapViewMergeColNames(zmap_view, req_featuresets) ;
                }
            }

          if(current_server->biotypes)
            {
              /* req all biotypes  as a list of their quark names. */
              req_biotypes = zMapConfigString2QuarkGList(current_server->biotypes,FALSE) ;
            }

          if (setUpServerConnectionByScheme(zmap_view, current_server, terminate, error))
            {
              /* Load the features for the server */
              zmapViewLoadFeatures(zmap_view, NULL, req_featuresets, req_biotypes, current_server,
                                   req_sequence, req_start, req_end, thread_fail_silent,
                                   SOURCE_GROUP_START,TRUE, terminate) ;
            }
        }
    }
}





//
//                 Package routines
//


/* request featuresets from a server, req_featuresets may be null in which case all are requested implicitly */
/* called from zmapViewLoadfeatures() to preserve original function
 * called from zmapViewConnect() to handle autoconfigured file servers,
 * which cannot be delayed as there's no way to fit these into the columns dialog as it currrently exists
 */
ZMapNewDataSource zmapViewRequestServer(ZMapView view, ZMapNewDataSource view_conn,
                                        ZMapFeatureBlock block_orig, GList *req_featuresets, GList *req_biotypes,
                                        ZMapConfigSource server,
                                        const char *req_sequence, int req_start, int req_end,
                                        gboolean dna_requested, gboolean terminate_in, gboolean show_warning)
{
  ZMapFeatureContext context ;
  ZMapFeatureBlock block ;
  gboolean is_pipe = FALSE ;
  gboolean terminate = terminate_in ;


  /* Copy the original context from the target block upwards setting feature set names
   * and the range of features to be copied.
   * We need one for each featureset/ request
   */
  if (block_orig)
    {
      // using this as it may be necessary for Blixem ?
      context = zMapFeatureContextCopyWithParents((ZMapFeatureAny)block_orig) ;
      context->req_feature_set_names = req_featuresets ;

      /* need request coords for ACEDB in case of no data returned so that we can record the actual range. */
      block = zMapFeatureAlignmentGetBlockByID(context->master_align, block_orig->unique_id) ;
      zMapFeatureBlockSetFeaturesCoords(block, req_start, req_end) ;


      if (zMapViewGetRevCompStatus(view))
        {
          /* revcomp our empty context to get external fwd strand coordinates */
          zMapFeatureContextReverseComplement(context);
        }
    }
  else
    {
      /* Create data specific to this step list...and set it in the connection. */
      context = zmapViewCreateContext(view, req_featuresets, NULL) ;
    }

  //printf("request featureset %s from %s\n",g_quark_to_string(GPOINTER_TO_UINT(req_featuresets->data)),server->url);
  zMapStartTimer("LoadFeatureSet", g_quark_to_string(GPOINTER_TO_UINT(req_featuresets->data)));

  /* force pipe servers to terminate, to fix mis-config error that causes a crash (RT 223055) */
  is_pipe =
    g_str_has_prefix(server->url(),"pipe://") ||
    g_str_has_prefix(server->url(),"file://") ||
    g_str_has_prefix(server->url(),"http://") ||
    g_str_has_prefix(server->url(),"https://");

  terminate = terminate || is_pipe ;


  // ERROR HANDLING..........temp while I rearrange....
  if (!server->urlObj())
    {
      zMapLogWarning("GUI: url %s did not parse. Parse error < %s >",
                     server->url(), server->urlError().c_str()) ;
      return(NULL) ;
    }


  // Create the connection data struct....
  ZMapConnectionData connect_data = NULL ;


  /* Set up the connection data for this view connection. */
  connect_data = g_new0(ZMapConnectionDataStruct, 1) ;

  connect_data->view = view ;

  connect_data->curr_context = context ;

  /* ie server->delayed -> called after startup */
  if (terminate)
    connect_data->dynamic_loading = TRUE ;

  connect_data->column_2_styles = zMap_g_hashlist_create() ;
  // better?      connect_data->column_2_styles = zmap_view->context_map.column_2_styles;

  connect_data->featureset_2_column = view->context_map.featureset_2_column;
  connect_data->source_2_sourcedata = view->context_map.source_2_sourcedata;

  // we need to save this to tell otterlace when we've finished
  // it also gets given to threads: when can we free it?
  connect_data->feature_sets = req_featuresets;
  //zMapLogWarning("request %d %s",g_list_length(req_featuresets),g_quark_to_string(GPOINTER_TO_UINT(req_featuresets->data)));


  /* well actually no it's not come kind of iso violation.... */
  /* the bad news is that these two little numbers have to tunnel through three distinct data
     structures and layers of s/w to get to the pipe scripts.  Originally the request
     coordinates were buried in blocks in the context supplied incidentally when requesting
     features after extracting other data from the server.  Obviously done to handle multiple
     blocks but it's another iso 7 violation  */

  connect_data->start = req_start ;
  connect_data->end = req_end ;

  /* likewise this has to get copied through a series of data structs */
  connect_data->sequence_map = view->view_sequence;

  /* gb10: also added the req_sequence here. This is non-null if the sequence name we need to
     look up in the server is different to the sequence name in the view. Otherwise, use the
     sequence name from the sequence map */
  if (req_sequence)
    connect_data->req_sequence = g_quark_from_string(req_sequence) ;
  else if (view->view_sequence)
    connect_data->req_sequence = g_quark_from_string(view->view_sequence->sequence) ;

  connect_data->display_after = ZMAP_SERVERREQ_FEATURES ;

  /* This struct will needs to persist after the viewcon is gone so we can
   * return information to the layer above us about feature loading. */
  connect_data->loaded_features = zmapViewCreateLoadFeatures(NULL) ;


  // If there's no view_con or the view_con is busy then we need to create a new one otherwise we
  // reuse the given one.
  if (!view_conn
      || ((connect_data = (ZMapConnectionData)zMapServerConnectionGetUserData(view_conn))
          && (connect_data->step_list)))
    {
      if (view_conn)
        {
          // it would seem that we never actually get here...how weird.....
          //NEED TO DECIDE WHETHER TO KEEP THIS.....WHEN IS IT USED ???

          zMapDebugPrintf("Reusing a view_conn for \n%s\n", server->url()) ;
          zMapLogWarning("Reusing a view_conn for \n%s\n", server->url()) ;
          zMapMessage("Reusing a view_conn for \n%s\n", server->url()) ;
        }

      view_conn = zMapServerCreateViewConnection(view_conn, connect_data, server->url()) ;
    }

  if (view_conn)
    {
      // Do we need all these args now too...?????
      createStepList(view, view_conn, connect_data,
                     server,
                     server->urlObj(), server->format,
                     context, req_featuresets, req_biotypes,
                     dna_requested, server->req_styles, server->stylesfile,
                     terminate) ;

      // Add this connection to the list of connections to be monitored for data by zmapview
      view->connection_list = g_list_append(view->connection_list, view_conn) ;

      // Now dispatch the first request......
      zmapViewStepListIter(connect_data->step_list, view_conn->thread->GetThread(), view_conn) ;




      /* Why does this need reiniting ? */
      if (!view->sources_loading)
        {
          g_list_free(view->sources_empty) ;
          view->sources_empty = NULL ;

          g_list_free(view->sources_failed) ;
          view->sources_failed = NULL ;
        }

      view->sources_loading = zMap_g_list_insert_list_after(view->sources_loading, req_featuresets,
                                                            g_list_length(view->sources_loading),
                                                            TRUE) ;

      view_conn->show_warning = show_warning ;
    }
  else
    {
      // ok this just records that we couldn't make a connection... not brilliant logic....

      view->sources_failed = zMap_g_list_insert_list_after(view->sources_failed, req_featuresets,
                                                           g_list_length(view->sources_failed),
                                                           TRUE) ;

      zMap_g_list_quark_print(req_featuresets, "req_featuresets", FALSE) ;

      zMapLogWarning("createViewConnection() failed, failed sources now %d",
                     g_list_length(view->sources_failed)) ;
    }

  return view_conn ;
}


/* Set up connections to a list of named servers */
gboolean zmapViewSetUpServerConnections(ZMapView zmap_view, GList *settings_list, GError **error)
{
  gboolean result = TRUE ;

  /* Set up connections to the named servers. */
  if (zmap_view && settings_list)
    {
      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      for (GList *current_server = settings_list; current_server; current_server = g_list_next(current_server))
        {
          GError *tmp_error = NULL ;

          zMapViewSetUpServerConnection(zmap_view, (ZMapConfigSource)current_server->data, &tmp_error) ;

          if (tmp_error)
            {
              /* Just warn about any failures */
              zMapWarning("%s", tmp_error->message) ;
              zMapLogWarning("GUI: %s", tmp_error->message) ;

              g_error_free(tmp_error) ;
              tmp_error = NULL ;
            }
        }
    }

  return result ;
}



/* Loads features within block from the sets req_featuresets that lie within features_start
 * to features_end. The features are fetched from the data sources and added to the existing
 * view. N.B. this is asynchronous because the sources are separate threads and once
 * retrieved the features are added via a gtk event.
 *
 * NOTE req_sources is nominally a list of featuresets.
 * Otterlace could request a featureset that belongs to ACE
 * and then we'd have to find the column for that to find it in the ACE config
 *
 *
 * NOTE block is NULL for startup requests
 */
void zmapViewLoadFeatures(ZMapView view, ZMapFeatureBlock block_orig, 
                          GList *req_sources, GList *req_biotypes,
                          ZMapConfigSource server,
                          const char *req_sequence, int features_start, int features_end,
                          const bool thread_fail_silent,
                          gboolean group_flag, gboolean make_new_connection, gboolean terminate)
{
  GList * sources = NULL;
  GHashTable *ghash = NULL;
  int req_start,req_end;
  gboolean requested = FALSE;
  static gboolean debug_sources = FALSE ;
  gboolean dna_requested = FALSE;
  ZMapNewDataSource view_conn = NULL ;


  /* MH17 NOTE
   * these are forward strand coordinates
   * see commandCB() for code previously here
   * previous design decisions resulted in the feature conetxt being revcomped
   * rather than just the view, and data gets revcomped when received if necessary
   * external interfaces (eg otterlace) have no reason to know if we've turned the view
   * upside down and will always request as forward strand
   * only a request fromn the window can be upside down, and is converted to fwd strand
   * before calling this function
   */
  req_start = features_start;
  req_end = features_end;

  if (server)
    {
      GQuark dna_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME) ;
      GQuark threeft_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME) ;
      GQuark showtrans_quark = zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME) ;

      /* We need the DNA if we are showing DNA, 3FT or ShowTranslation */
      if (req_sources && 
          (zMap_g_list_find_quark(req_sources, dna_quark) ||
           zMap_g_list_find_quark(req_sources, threeft_quark) ||
           zMap_g_list_find_quark(req_sources, showtrans_quark)))
        {
          dna_requested = TRUE ;
        }

      view_conn = zmapViewRequestServer(view, NULL, block_orig, req_sources, req_biotypes, server,
                                        req_sequence, req_start, req_end, dna_requested, terminate, !view->thread_fail_silent);
      if(view_conn)
        requested = TRUE;
    }
  else
    {
      /* OH DEAR...THINK WE MIGHT NEED THE CONFIG FILE HERE TOO.... */

     /* mh17: this is tedious to do for each request esp on startup */
      sources = view->view_sequence->getSources(true) ;
      ghash = getFeatureSourceHash(sources);

      for ( ; req_sources ; req_sources = g_list_next(req_sources))
        {
          GQuark featureset = GPOINTER_TO_UINT(req_sources->data);
          char *unique_name ;
          GQuark unique_id ;

          dna_requested = FALSE;

          zMapDebugPrint(debug_sources, "feature set quark (%d) is: %s", featureset, g_quark_to_string(featureset)) ;

          unique_name = (char *)g_quark_to_string(featureset) ;
          unique_id = zMapFeatureSetCreateID(unique_name) ;

          zMapDebugPrint(debug_sources, "feature set unique quark (%d) is: %s", unique_id, g_quark_to_string(unique_id)) ;

          server = getSourceFromFeatureset(ghash, unique_id) ;

          if (!server && view->context_map.featureset_2_column)
            {
              ZMapFeatureSetDesc GFFset = NULL;

              /* this is for ACEDB where the server featureset list is actually a list of columns
               * so to find the server we need to find the column
               * there is some possibility of collision if mis-configured
               * and what will happen will be no data
               */
              if ((GFFset = (ZMapFeatureSetDesc)g_hash_table_lookup(view->context_map.featureset_2_column, GUINT_TO_POINTER(unique_id))))

                {
                  featureset = GFFset->column_id;
                  server = getSourceFromFeatureset(ghash,featureset);
                }
            }


          if (server)
            {
              GList *req_featuresets = NULL;
              int existing = FALSE;

              //          zMapLogMessage("Load features %s from %s, group = %d",
              //                         g_quark_to_string(featureset),server->url,server->group) ;

              // make a list of one feature only
              req_featuresets = g_list_append(req_featuresets,GUINT_TO_POINTER(featureset));

              //zMapLogWarning("server group %x %x %s",group_flag,server->group,g_quark_to_string(featureset));

              if ((server->group & group_flag))
                {
                  // get all featuresets from this source and remove from req_sources
                  GList *req_src;
                  GQuark fset;
                  ZMapConfigSource fset_server;

                  for(req_src = req_sources->next;req_src;)
                    {
                      fset = GPOINTER_TO_UINT(req_src->data);
                      //            zMapLogWarning("add %s",g_quark_to_string(fset));
                      fset_server = getSourceFromFeatureset(ghash,fset);

                      if (!fset_server && view->context_map.featureset_2_column)
                        {
                          ZMapFeatureSetDesc GFFset = NULL;

                          GFFset = (ZMapFeatureSetDesc)g_hash_table_lookup(view->context_map.featureset_2_column, GUINT_TO_POINTER(fset)) ;
                          if (GFFset)
                            {
                              fset = GFFset->column_id;
                              fset_server = getSourceFromFeatureset(ghash,fset);
                              //                      zMapLogWarning("translate to  %s",g_quark_to_string(fset));
                            }
                        }

                      //if (fset_server) zMapLogMessage("Try %s",fset_server->url);
                      if (fset_server == server)
                        {
                          GList *del;

                          /* prepend faster than append...we don't care about the order */
                          //                      req_featuresets = g_list_prepend(req_featuresets,GUINT_TO_POINTER(fset));
                          /* but we need to add unique columns eg for ext_curated (column) = 100's of featuresets */
                          req_featuresets = zMap_g_list_append_unique(req_featuresets, GUINT_TO_POINTER(fset));

                          // avoid getting ->next from deleted item
                          del = req_src;
                          req_src = req_src->next;
                          //                      zMapLogMessage("use %s",g_quark_to_string(fset));

                          // as req_src is ->next of req_sources we know req_sources is still valid
                          // even though we are removing an item from it
                          // However we still get a retval from g_list remove()
                          req_sources = g_list_remove_link(req_sources,del);
                          // where else is this held: crashes
                          //NB could use delete link if that was ok
                          //                     g_free(del); // free the link; no data to free
                        }
                      else
                        {
                          req_src = req_src->next;
                          //                  zMapLogWarning("skip %s",g_quark_to_string(fset));
                        }
                    }
                }

              // look for server in view->connections list
              if (group_flag & SOURCE_GROUP_DELAYED)
                {
                  GList *view_con_list ;

                  for (view_con_list = view->connection_list ; view_con_list ; view_con_list = g_list_next(view_con_list))
                    {
                      view_conn = (ZMapNewDataSource) view_con_list->data ;

                      if (strcmp(zMapServerGetUrl(view_conn), server->url()) == 0)
                        {
                          existing = TRUE ;
                          break ;
                        }
                    }


                  /* AGH...THIS CODE IS USING THE EXISTENCE OF A PARTICULAR SOURCE TO TEST WHETHER
                   * FEATURE SETS ARE SET UP...UGH.... */
                  /* why? if the source exists ie is persistent (eg ACEDB) then if we get here
                   * then we've already set up the ACEDB columns
                   * so we don't want to do it again
                   */
                  // make the windows have the same list of featuresets so that they display
                  // this function is a deferred load: for existing connections we already have the columns defined
                  // so don't concat new ones on the end.
                  // A better fix would be to merge the data see zMapWindowMergeInFeatureSetNames()
                  if (!view->columns_set && !existing)
                    {
                      createColumns(view,req_featuresets);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                      g_list_foreach(view->window_list, invoke_merge_in_names, req_featuresets);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                      zmapViewMergeColNames(view, req_featuresets) ;
                    }
                }

              /* THESE NEED TO GO WHEN STEP LIST STUFF IS DONE PROPERLY.... */
              // this is an optimisation: the server supports DNA so no point in searching for it
              // if we implement multiple sources then we can remove this
              if ((zMap_g_list_find_quark(req_featuresets, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
                {
                  dna_requested = TRUE ;
                }

              // start a new server connection
              // can optionally use an existing one -> pass in second arg
              view_conn = (make_new_connection ? NULL : (existing ? view_conn : NULL)) ;


              view_conn = zmapViewRequestServer(view, view_conn, block_orig, req_featuresets, req_biotypes,
                                                server, req_sequence, req_start, req_end,
                                                dna_requested,
                                                (!existing && terminate), !view->thread_fail_silent) ;

              if(view_conn)
                requested = TRUE;


              // g_list_free(req_featuresets); no! this list gets used by threads
              req_featuresets = NULL ;
            }
        }
    }

  if (requested)
    {

      /* YES BUT THE USER MAY NOT HAVE REQUESTED IT !!!!!!!!!!! AND WHY BOTHER WITH
       * THIS POINTLESS AND ANNOYING OPTIIMISATION......WHY FIDDLE WITH IT AT ALL..... */
      // this is an optimisation: the server supports DNA so no point in searching for it
      // if we implement multiple sources then we can remove this
      if (dna_requested)        //(zMap_g_list_find_quark(req_featuresets, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
        {
          view->sequence_server  = view_conn ;
        }

      zMapViewShowLoadStatus(view);
    }

  if (ghash)
    g_hash_table_destroy(ghash);

  return ;
}



//
// THESE FUNCTIONS SEEM AN ODD FIT.....oh yes...they are all about reporting
// what has been loaded and what has not....wrong place for these.....
//

/* Create/copy/delete load_features structs. */
LoadFeaturesData zmapViewCreateLoadFeatures(GList *feature_sets)
{
  LoadFeaturesData loaded_features = NULL ;

  loaded_features = g_new0(LoadFeaturesDataStruct, 1) ;

  if (feature_sets)
    loaded_features->feature_sets = g_list_copy(feature_sets) ;

  return loaded_features ;
}

LoadFeaturesData zmapViewCopyLoadFeatures(LoadFeaturesData loaded_features_in)
{
  LoadFeaturesData loaded_features = NULL ;

  loaded_features = g_new0(LoadFeaturesDataStruct, 1) ;

  *loaded_features = *loaded_features_in ;

  if (loaded_features_in->feature_sets)
    loaded_features->feature_sets = g_list_copy(loaded_features_in->feature_sets) ;

  return loaded_features ;
}

void zmapViewDestroyLoadFeatures(LoadFeaturesData loaded_features)
{
  if (loaded_features->feature_sets)
    g_list_free(loaded_features->feature_sets) ;

  if (loaded_features->err_msg)
    g_free(loaded_features->err_msg) ;

  g_free(loaded_features) ;

  return ;
}




//
//             Internal routines
//


/* Utility called by zMapViewSetUpServerConnection to do any scheme-specific set up for the given
 * server. This updates the terminate flag. Returns true if the source should be loaded. */
static bool setUpServerConnectionByScheme(ZMapView zmap_view,
                                          ZMapConfigSource current_server,
                                          bool &terminate,
                                          GError **error)
{
  bool result = true ;
  const ZMapURL zmap_url = current_server->urlObj() ;
  terminate = FALSE ;

  /* URL may be empty for trackhub sources which are just parents of child data tracks */
  if (zmap_url)
    {
      if (zmap_url->scheme == SCHEME_PIPE)
        {
          terminate = TRUE ;
        }
      else if (zmap_url->scheme == SCHEME_TRACKHUB)
        {
          // Don't load trackhub sources directly (they are just parent sources for child sources)
          result = false ;
        }
    }
  else
    {
      result = false ;
    }

  return result ;
}



/* retro fit/ invent the columns config implied by server featuresets= command
 * needed for the status bar and maybe some other things
 */
static void createColumns(ZMapView view,GList *featuresets)
{
  ZMapFeatureColumn col;

  for(;featuresets; featuresets = featuresets->next)
    {
      /* this ought to do a featureset_2_column lookup  and then default to the featureset name */

      GQuark featureset_id = (GQuark)GPOINTER_TO_INT(featuresets->data) ;

      std::map<GQuark, ZMapFeatureColumn>::iterator iter = view->context_map.columns->find(featureset_id) ;

      if (iter == view->context_map.columns->end())
        {
          char *str = (char *) g_quark_to_string(GPOINTER_TO_UINT(featuresets->data));
          col = (ZMapFeatureColumn) g_new0(ZMapFeatureColumnStruct,1);

          col->column_id = GPOINTER_TO_UINT(featuresets->data);
          col->unique_id = zMapFeatureSetCreateID(str);
          col->column_desc = str;
          col->order = zMapFeatureColumnOrderNext(FALSE);

          /* no column specific style possible from servers */
          
          (*view->context_map.columns)[col->unique_id] = col ;
        }
    }

  return ;
}


static ZMapConfigSource getSourceFromFeatureset(GHashTable *ghash, GQuark featurequark)
{
  ZMapConfigSource config_source ;

  config_source = (ZMapConfigSource)g_hash_table_lookup(ghash, GUINT_TO_POINTER(featurequark)) ;

  return config_source ;
}





// create a hash table of feature set names and thier sources
static GHashTable *getFeatureSourceHash(GList *sources)
{
  GHashTable *ghash = NULL;
  ZMapConfigSource src;
  gchar **features,**feats;

  ghash = g_hash_table_new(NULL,NULL);

  // for each source extract featuresets and add a hash to the source
  for(;sources; sources = g_list_next(sources))
    {
      src = (ZMapConfigSource)(sources->data) ;

      if(!src->featuresets)
            continue;
      features = g_strsplit(src->featuresets,";",0); // this will give null entries eg 'aaa ; bbbb' -> 5 strings
      if(!features)
            continue;
      for(feats = features;*feats;feats++)
        {
          GQuark q;
          // the data we want to lookup happens to have been quarked
          if(**feats)
          {
            gchar *feature;

            feature = zMapConfigNormaliseWhitespace(*feats,FALSE);
            if(!feature)
                  continue;

            /* add the user visible version */
            g_hash_table_insert(ghash,GUINT_TO_POINTER(g_quark_from_string(feature)),(gpointer) src);

            /* add a cononical version */
            q =  zMapFeatureSetCreateID(feature);

            if (q != g_quark_from_string(feature))
              g_hash_table_insert(ghash,GUINT_TO_POINTER(q), (gpointer) src);
          }
        }

      g_strfreev(features);
    }

  return(ghash);
}



static bool createStepList(ZMapView zmap_view, ZMapNewDataSource view_con, ZMapConnectionData connect_data,
                           ZMapConfigSource server,
                           const ZMapURL urlObj, const char *format,
                           ZMapFeatureContext context, GList *req_featuresets, GList *req_biotypes,
                           gboolean dna_requested, gboolean req_styles, char *styles_file,
                           gboolean terminate)
{
  bool result = false ;
  ZMapServerReqAny req_any;
  StepListActionOnFailureType on_fail = REQUEST_ONFAIL_CANCEL_THREAD;


  connect_data->step_list = zmapViewStepListCreateFull(dispatchContextRequests,
                                                       processDataRequests,
                                                       freeDataRequest);

  /* Record info. for this session. */
  zmapViewSessionAddServer(view_con->server, urlObj, (char *)format) ;


  /* Set up this connection in the step list. */
  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_CREATE, server) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_CREATE, req_any, on_fail) ;
  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_OPEN) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_OPEN, req_any, on_fail) ;
  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSERVERINFO) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_GETSERVERINFO, req_any, on_fail) ;


  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURESETS, req_featuresets, req_biotypes, NULL) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_FEATURESETS, req_any, on_fail) ;

  if (req_styles || styles_file)
    {
      req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_STYLES, req_styles, styles_file && *styles_file ? styles_file : NULL) ;
      zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_STYLES, req_any, on_fail) ;
    }
  else
    {
      connect_data->curr_styles = &zmap_view->context_map.styles ;
    }


  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_NEWCONTEXT, context) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_NEWCONTEXT, req_any, on_fail) ;


  req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_FEATURES) ;
  zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_FEATURES, req_any, on_fail) ;


  if (dna_requested)
    {
      req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_SEQUENCE) ;
      zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_SEQUENCE, req_any, on_fail) ;
      connect_data->display_after = ZMAP_SERVERREQ_SEQUENCE ;
      /* despite appearing before features in the GFF this gets requested afterwards */
    }

  if (terminate)
    {
      /* MH17 NOTE
       * These calls are here in the order they should be executed in for clarity
       * but the order chosen is defined in zmapViewStepListCreateFull()
       * this code could be reordered without any effect
       * the step list is operated as an array indexed by request type
       */
      req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_GETSTATUS) ;
      zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_GETSTATUS, req_any, on_fail) ;
      connect_data->display_after = ZMAP_SERVERREQ_GETSTATUS;


      // that's odd...there's not close server here....


      req_any = zMapServerRequestCreate(ZMAP_SERVERREQ_TERMINATE) ;
      zmapViewStepListAddServerReq(connect_data->step_list, ZMAP_SERVERREQ_TERMINATE, req_any, on_fail ) ;
    }


  return result ;
}


/* This is _not_ a generalised dispatch function, it handles a sequence of requests that
 * will end up fetching a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean dispatchContextRequests(ZMapServerReqAny req_any, gpointer connection_data)
{
  gboolean result = TRUE ;
  ZMapNewDataSource connection = (ZMapNewDataSource)connection_data ;
  ZMapConnectionData connect_data = (ZMapConnectionData)(connection->request_data) ;

//zMapLogWarning("%s: dispatch %d",connection->url, req_any->type);
  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      break;
    case ZMAP_SERVERREQ_OPEN:
      {
      ZMapServerReqOpen open = (ZMapServerReqOpen) req_any;

      open->sequence_map = connect_data->sequence_map;
      open->req_sequence = connect_data->req_sequence;
      open->zmap_start = connect_data->start;
      open->zmap_end = connect_data->end;
      }
      break;
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {

        break ;
      }
    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;

        feature_sets->featureset_2_stylelist_out = connect_data->column_2_styles ;

        /* MH17: if this is an output parameter why do we set it on dispatch?
         * beacuse it's a preallocated hash table
         */

        /* next 2 are outputs from ACE and inputs to pipeServers */
        feature_sets->featureset_2_column_inout = connect_data->featureset_2_column;
        feature_sets->source_2_sourcedata_inout = connect_data->source_2_sourcedata;

        break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

        /* required styles comes from featuresets call. */
        get_styles->required_styles_in = connect_data->required_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        ZMapServerReqNewContext new_context = (ZMapServerReqNewContext)req_any ;

        new_context->context = connect_data->curr_context ;

        break ;
      }
    case ZMAP_SERVERREQ_FEATURES:
      {
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        get_features->context = connect_data->curr_context ;
        get_features->styles = connect_data->curr_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        get_features->context = connect_data->curr_context ;
        get_features->styles = connect_data->curr_styles ;

        break ;
      }
    case ZMAP_SERVERREQ_GETSTATUS:
      {
            break;
      }
    case ZMAP_SERVERREQ_TERMINATE:
      {
      //ZMapServerReqTerminate terminate = (ZMapServerReqTerminate) req_any ;source_2_sourcedata_inout

      break ;
      }
    default:
      {
        zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;

        break ;
      }
    }

  result = TRUE ;

  return result ;
}



/* This is _not_ a generalised processing function, it handles a sequence of replies from
 * a thread that build up a feature context from a source. The steps are interdependent
 * and data from one step must be available to the next. */
static gboolean processDataRequests(void *user_data, ZMapServerReqAny req_any)
{
  gboolean result = TRUE ;
  ZMapNewDataSource view_con = (ZMapNewDataSource)user_data ;
  ZMapConnectionData connect_data = (ZMapConnectionData)(view_con->request_data) ;
  ZMapView zmap_view = connect_data->view ;
  GList *fset;

  /* Process the different types of data coming back. */
  //printf("%s: response to %d was %d\n",view_con->url,req_any->type,req_any->response);
  //zMapLogWarning("%s: response to %d was %d",view_con->url,req_any->type,req_any->response);

  switch (req_any->type)
    {
    case ZMAP_SERVERREQ_CREATE:
      break;

    case ZMAP_SERVERREQ_OPEN:
      {
        break ;
      }
    case ZMAP_SERVERREQ_GETSERVERINFO:
      {
        ZMapServerReqGetServerInfo get_info = (ZMapServerReqGetServerInfo)req_any ;

        connect_data->session = *get_info ;                    /* struct copy. */

        /* Hacky....what if there are several source names etc...we need a flag to say "master" source... */
        if (!(zmap_view->view_db_name))
          zmap_view->view_db_name = connect_data->session.database_name_out ;

        if (!(zmap_view->view_db_title))
          zmap_view->view_db_title = connect_data->session.database_title_out ;

        break ;
      }

    case ZMAP_SERVERREQ_FEATURESETS:
      {
        ZMapServerReqFeatureSets feature_sets = (ZMapServerReqFeatureSets)req_any ;


        if (req_any->response == ZMAP_SERVERRESPONSE_OK)
          {
            /* Got the feature sets so record them in the server context. */
            connect_data->curr_context->req_feature_set_names = feature_sets->feature_sets_inout ;
          }
        else if (req_any->response == ZMAP_SERVERRESPONSE_UNSUPPORTED && feature_sets->feature_sets_inout)
          {
            /* If server doesn't support checking feature sets then just use those supplied. */
            connect_data->curr_context->req_feature_set_names = feature_sets->feature_sets_inout ;

            feature_sets->required_styles_out =
              get_required_styles_list(zmap_view->context_map.source_2_sourcedata,
                                       feature_sets->feature_sets_inout);
          }

        /* NOTE (mh17)
           tasked with handling a GFF file from the command line and no config..
           it became clear that this could not be done simply without reading each file twice
           and the plan moved to generating a config file with a perl script
           the idea being to read the GFF headers before running ZMap.
           But servers need a list of featuresets
           a) so that the request code could work out which servers to use
           b) so that we could precalculate featureset to column mapping and source to source data mapping
           (otherwise all data will be ignored and ZMap will abort from several places)
           which gives the crazy situation of having the read the entire file
           to extract all the featureset names so that we can read the entire file.
           NB files could be remote which is not ideal, and we expect some files to be large

           So i adapted the code to have featureset free servers
           (that can only be requested on startup - can't look one up by featureset if it's not defined)
           and hoped that i'd be able to patch this data in.
           There is a server protocol step to get featureset names
           but that would require processing subsequent steps to find out this information and
           the GFFparser rejects features from sources that have not been preconfigured.
           ie it's tied up in a knot

           Several parts of the display code have been patched to make up featureset to columns etc OTF
           which is in direct confrontation with the design of most of the server and display code,
           which explicitly assumes that this is predefined

           Well it sort of runs but really the server code needs a rewrite.
        */

#if 0
        // defaulted in GFFparser
        /* Not all servers will provide a mapping between feature sets and styles so we add
         * one now which is a straight mapping from feature set name to a style of the same name. */

        if (!(g_hash_table_size(feature_sets->featureset_2_stylelist_out)))
          {
            GList *sets ;

            sets = feature_sets->feature_sets_inout ;
            do
              {
                GQuark feature_set_id, feature_set_name_id;

                /* We _must_ canonicalise here. */
                feature_set_name_id = GPOINTER_TO_UINT(sets->data) ;

                feature_set_id = zMapFeatureSetCreateID((char *)g_quark_to_string(feature_set_name_id)) ;

                zMap_g_hashlist_insert(feature_sets->featureset_2_stylelist_out,
                                       feature_set_id,
                                       GUINT_TO_POINTER(feature_set_id)) ;
                //("processData f2s adds %s to %s\n",g_quark_to_string(feature_set_id),g_quark_to_string(feature_set_id));
              }
            while((sets = g_list_next(sets))) ;
          }
#endif


        /* not all servers provide a source to source data mapping
         * ZMap config can include this info but only if someone provides it
         * make sure there is an entry for each featureset from this server
         */
        for(fset = feature_sets->feature_sets_inout;fset;fset = fset->next)
          {
            ZMapFeatureSource src;
            GQuark fid = zMapStyleCreateID((char *) g_quark_to_string( GPOINTER_TO_UINT(fset->data)));

            if (!(src = (ZMapFeatureSource)g_hash_table_lookup(feature_sets->source_2_sourcedata_inout,GUINT_TO_POINTER(fid))))
              {
                // if entry is missing
                // allocate a new struct and add to the table
                src = g_new0(ZMapFeatureSourceStruct,1);

                src->source_id = GPOINTER_TO_UINT(fset->data);        /* may have upper case */
                src->source_text = src->source_id;

                g_hash_table_insert(feature_sets->source_2_sourcedata_inout, GUINT_TO_POINTER(fid), src) ;
              }
            else
              {
                if(!src->source_id)
                  src->source_id = GPOINTER_TO_UINT(fset->data);
                if(!src->source_text)
                  src->source_text = src->source_id;
                //                if(!src->style_id)
                //                  src->style_id = fid;        defaults to this in zmapGFF-Parser.c
              }
          }

        /* I don't know if we need these, can get from context. */
        connect_data->feature_sets = feature_sets->feature_sets_inout ;
        connect_data->required_styles = feature_sets->required_styles_out ;


#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nadding stylelists:\n");
        zMap_g_hashlist_print(feature_sets->featureset_2_stylelist_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nview styles lists before merge:\n");
        zMap_g_hashlist_print(zmap_view->context_map.column_2_styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

        /* Merge the featureset to style hashses. */
        zMap_g_hashlist_merge(zmap_view->context_map.column_2_styles, feature_sets->featureset_2_stylelist_out) ;

#if 0 //ED_G_NEVER_INCLUDE_THIS_CODE
        printf("\nview styles lists after merge:\n");
        zMap_g_hashlist_print(zmap_view->context_map.column_2_styles) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


        /* If the hashes aren't equal, we had to do a merge.  Need to free the server
         * created hash that will otherwise be left dangling... */
        if (zmap_view->context_map.column_2_styles != feature_sets->featureset_2_stylelist_out)
          {
            zMap_g_hashlist_destroy(feature_sets->featureset_2_stylelist_out);
            feature_sets->featureset_2_stylelist_out = NULL;
          }

        /* merge these in, the base mapping is defined in zMapViewIniGetData() */
        // NB these are not supplied by pipeServers and we assume a 1-1 mapping
        // (see above) of source to display featureset and source to style.
        // See also zmapViewRemoteReceive.c/xml_featureset_start_cb()

        if (!(zmap_view->context_map.featureset_2_column))
          {
            zmap_view->context_map.featureset_2_column = feature_sets->featureset_2_column_inout ;
          }
        else if (feature_sets->featureset_2_column_inout &&
                 zmap_view->context_map.featureset_2_column != feature_sets->featureset_2_column_inout)
          {
            //print_fset2col("merge view",zmap_view->context_map.featureset_2_column);
            //print_fset2col("merge inout",feature_sets->featureset_2_column_inout);
            g_hash_table_foreach(feature_sets->featureset_2_column_inout,
                                 mergeHashTableCB,zmap_view->context_map.featureset_2_column);
          }

        /* we get lower case column names from ACE
         * - patch in upppercased ones from other config if we have it
         * (otterlace should provide config for this)
         */
        {
          GList *iter;
          gpointer key,value;

          zMap_g_hash_table_iter_init(&iter,zmap_view->context_map.featureset_2_column);
          while(zMap_g_hash_table_iter_next(&iter,&key,&value))
            {
              ZMapFeatureSetDesc fset;
              ZMapFeatureSource fsrc;
              ZMapFeatureColumn column;

              fset = (ZMapFeatureSetDesc) value;

              /* construct a reverse col 2 featureset mapping */
              std::map<GQuark, ZMapFeatureColumn>::iterator map_iter = zmap_view->context_map.columns->find(fset->column_id) ;
              
              if(map_iter != zmap_view->context_map.columns->end())
                {
                  column = map_iter->second ;

                  fset->column_ID = column->column_id;      /* upper cased display name */

                  /* construct reverse mapping from column to featureset */
                  /* but don't add featuresets that get virtualised */
                  if(!g_list_find(column->featuresets_unique_ids,key))
                    {
                      fsrc = (ZMapFeatureSource)g_hash_table_lookup(zmap_view->context_map.source_2_sourcedata,key);
                      if(fsrc && !fsrc->maps_to)
                        {
                          /* NOTE this is an ordered list */
                          column->featuresets_unique_ids = g_list_append(column->featuresets_unique_ids,key);
                          /*! \todo #warning this code gets run for all featuresets for every server which is silly */
                        }
                    }
                }

              /* we get hundreds of these from ACE that are not in the config */
              /* and there's no capitalised names */
              /* we could hack in a capitaliser function but it would never be perfect */
              if(!fset->feature_src_ID)
                fset->feature_src_ID = GPOINTER_TO_UINT(key);
            }
        }

        if (!(zmap_view->context_map.source_2_sourcedata))
          {
            zmap_view->context_map.source_2_sourcedata = feature_sets->source_2_sourcedata_inout ;
          }
        else if(feature_sets->source_2_sourcedata_inout &&
                zmap_view->context_map.source_2_sourcedata !=  feature_sets->source_2_sourcedata_inout)
          {
            g_hash_table_foreach(feature_sets->source_2_sourcedata_inout,
                                 mergeHashTableCB,zmap_view->context_map.source_2_sourcedata);
          }


        //print_source_2_sourcedata("got featuresets",zmap_view->context_map.source_2_sourcedata);
        //print_fset2col("got featuresets",zmap_view->context_map.featureset_2_column);
        //print_col2fset("got columns",zmap_view->context_map.columns);

        // MH17: need to think about freeing _inout tables if in != out

        break ;
      }
    case ZMAP_SERVERREQ_STYLES:
      {
        ZMapServerReqStyles get_styles = (ZMapServerReqStyles)req_any ;

        /* Merge the retrieved styles into the views canonical style list. */
        if(get_styles->styles_out)
          {
            char *missing_styles = NULL ;

#if 0
            pointless doing this if we have defaults
              code moved from zmapServerProtocolHandler.c
              besides we should test the view data which may contain global config

              char *missing_styles = NULL;

            /* Make sure that all the styles that are required for the feature sets were found.
             * (This check should be controlled from analysing the number of feature servers or
             * flags set for servers.....) */

            if (!haveRequiredStyles(get_styles->styles_out, get_styles->required_styles_in, &missing_styles))
              {
                *err_msg_out = g_strdup_printf("The following required Styles could not be found on the server: %s",
                                               missing_styles) ;
              }
            if(missing_styles)
              {
                g_free(missing_styles);           /* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */
              }
#endif

            zMapStyleMergeStyles(zmap_view->context_map.styles, get_styles->styles_out, ZMAPSTYLE_MERGE_PRESERVE) ;

            /* need to patch in sub style pointers after merge/ copy */
            zmap_view->context_map.styles.foreach(zMapStyleSetSubStyle, &zmap_view->context_map.styles) ;

            /* test here, where we have global and predefined styles too */

            if (!haveRequiredStyles(zmap_view->context_map.styles, get_styles->required_styles_in, &missing_styles))
              {
                zMapLogWarning("The following required Styles could not be found on the server: %s",missing_styles) ;
              }
            if(missing_styles)
              {
                g_free(missing_styles);           /* haveRequiredStyles return == TRUE doesn't mean missing_styles == NULL */
              }
          }

        /* Store the curr styles for use in creating the context and drawing features. */
        //        connect_data->curr_styles = get_styles->styles_out ;
        /* as the styles in the window get replaced we need to have all of them not the new ones */
        connect_data->curr_styles = &zmap_view->context_map.styles ;

        break ;
      }

    case ZMAP_SERVERREQ_NEWCONTEXT:
      {
        break ;
      }

    case ZMAP_SERVERREQ_FEATURES:
    case ZMAP_SERVERREQ_GETSTATUS:
    case ZMAP_SERVERREQ_SEQUENCE:
      {
        char *missing_styles = NULL ;
        /* features and getstatus combined as they can both display data */
        ZMapServerReqGetFeatures get_features = (ZMapServerReqGetFeatures)req_any ;

        if (req_any->response != ZMAP_SERVERRESPONSE_OK)
          result = FALSE ;

        if (result && req_any->type == ZMAP_SERVERREQ_FEATURES)
          {

            /* WHAT !!!!! THIS DOESN'T SOUND RIGHT AT ALL.... */
#if MH17_ADDED_IN_GFF_PARSER
            if (!(connect_data->server_styles_have_mode)
                && !zMapFeatureAnyAddModesToStyles((ZMapFeatureAny)(connect_data->curr_context),
                                                   zmap_view->context_map.styles))
              {
                     zMapLogWarning("Source %s, inferring Style modes from Features failed.",
                               view_con->url) ;

                result = FALSE ;
              }
#endif

            /* I'm not sure if this couldn't come much earlier actually....something
             * to investigate.... */
            if (!makeStylesDrawable(zmap_view->view_sequence->config_file,
                                    zmap_view->context_map.styles, &missing_styles))
              {
                zMapLogWarning("Failed to make following styles drawable: %s", missing_styles) ;

                result = FALSE ;
              }

            /* Why is this needed....to cache for the getstatus call ???? */
            connect_data->get_features = get_features ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* not ready for these yet.... */

            connect_data->exit_code = get_features->exit_code ;
            connect_data->stderr_out = get_features->stderr_out ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


          }
        else if (result && req_any->type == ZMAP_SERVERREQ_GETSTATUS)
          {
            /* store the exit code and STDERR */
            ZMapServerReqGetStatus get_status = (ZMapServerReqGetStatus)req_any ;

            connect_data->exit_code = get_status->exit_code;
          }


        /* ok...once we are here we can display stuff.... */
        if (result && req_any->type == connect_data->display_after)
          {
            zmapViewSessionAddServerInfo(view_con->server, &(connect_data->session)) ;

            if (connect_data->get_features)  /* may be nul if server died */
              {

                /* Gosh...what a dereference.... */
                zMapStopTimer("LoadFeatureSet",
                              g_quark_to_string(GPOINTER_TO_UINT(connect_data->get_features->context->req_feature_set_names->data)));

                /* can't copy this list after getFeatures as it gets wiped */
                if (!connect_data->feature_sets)        /* (is autoconfigured server/ featuresets not specified) */
                  connect_data->feature_sets = g_list_copy(connect_data->get_features->context->src_feature_set_names) ;


                // NEEDS MOVING BACK INTO zmapView.c.....
                //
                /* Does the merge and issues the request to do the drawing. */
                result = viewGetFeatures(zmap_view, connect_data->get_features, connect_data) ;



              }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* Is this the right place to decrement....dunno.... */
            /* we record succcessful requests, if some fail they will get zapped in checkstateconnections() */
            zmap_view->sources_loading = zMap_g_list_remove_quarks(zmap_view->sources_loading,
                                                                   connect_data->feature_sets) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          }

        break ;
      }
      //    case ZMAP_SERVERREQ_GETSEQUENCE:
      // never appears?? - see commandCB() and processGetSeqRequests() in this file


    case ZMAP_SERVERREQ_TERMINATE:
      {
        break ;
      }

    default:
      {
        zMapLogFatalLogicErr("switch(), unknown value: %d", req_any->type) ;
        result = FALSE ;

        break ;
      }
    }

  return result ;
}

static void freeDataRequest(ZMapServerReqAny req_any)
{
  zMapServerRequestDestroy(req_any) ;

  return ;
}





// get 1-1 mapping of featureset names to style id except when configured differently
static GList *get_required_styles_list(GHashTable *srchash,GList *fsets)
{
  GList *iter;
  GList *styles = NULL;
  ZMapFeatureSource src;
  gpointer key,value;

  zMap_g_hash_table_iter_init(&iter,srchash);
  while(zMap_g_hash_table_iter_next(&iter,&key,&value))
    {
      src = (ZMapFeatureSource)g_hash_table_lookup(srchash, key);
      if(src)
        value = GUINT_TO_POINTER(src->style_id);
      styles = g_list_prepend(styles,value);
    }

  return(styles);
}



static void mergeHashTableCB(gpointer key, gpointer value, gpointer user)
{
  GHashTable *old = (GHashTable *) user;

  if(!g_hash_table_lookup(old,key))
    g_hash_table_insert(old,key,value);
}


// returns whether we have any of the needed styles and lists the ones we don't
static gboolean haveRequiredStyles(ZMapStyleTree &all_styles, GList *required_styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  FindStylesStruct find_data = {NULL} ;

  if(!required_styles)  // MH17: semantics -> don't need styles therefore have those that are required
    return(TRUE);

  find_data.all_styles_tree = &all_styles ;

  g_list_foreach(required_styles, findStyleCB, &find_data) ;

  if (find_data.missing_styles)
    *missing_styles_out = g_string_free(find_data.missing_styles, FALSE) ;

  result = find_data.found_style ;

  return result ;
}




static gboolean makeStylesDrawable(char *config_file, ZMapStyleTree &styles, char **missing_styles_out)
{
  gboolean result = FALSE ;
  DrawableDataStruct drawable_data = {NULL, FALSE, NULL} ;

  drawable_data.config_file = config_file ;

  styles.foreach(drawableCB, &drawable_data) ;

  if (drawable_data.missing_styles)
    *missing_styles_out = g_string_free(drawable_data.missing_styles, FALSE) ;

  result = drawable_data.found_style ;

  return result ;
}


/* GFunc()    */
static void findStyleCB(gpointer data, gpointer user_data)
{
  GQuark style_id = GPOINTER_TO_INT(data) ;
  FindStyles find_data = (FindStyles)user_data ;

  style_id = zMapStyleCreateID((char *)g_quark_to_string(style_id)) ;

  if (find_data->all_styles_hash && zMapFindStyle(find_data->all_styles_hash, style_id))
    find_data->found_style = TRUE ;
  else if (find_data->all_styles_tree && find_data->all_styles_tree->find(style_id))
    find_data->found_style = TRUE;
  else
    {
      if (!(find_data->missing_styles))
        find_data->missing_styles = g_string_sized_new(1000) ;

      g_string_append_printf(find_data->missing_styles, "%s ", g_quark_to_string(style_id)) ;
    }

  return ;
}




/* A callback to make the given style drawable. */
static void drawableCB(ZMapFeatureTypeStyle style, gpointer user_data)
{
  DrawableData drawable_data = (DrawableData)user_data ;

  if (style && zMapStyleIsDisplayable(style))
    {
      if (zMapStyleMakeDrawable(drawable_data->config_file, style))
        {
          drawable_data->found_style = TRUE ;
        }
      else
        {
          if (!(drawable_data->missing_styles))
            drawable_data->missing_styles = g_string_sized_new(1000) ;

          g_string_append_printf(drawable_data->missing_styles, "%s ", g_quark_to_string(style->unique_id)) ;
        }
    }

  return ;
}



static gboolean viewGetFeatures(ZMapView zmap_view, ZMapServerReqGetFeatures feature_req, ZMapConnectionData connect_data)
{
  gboolean result = FALSE ;
  ZMapFeatureContext new_features = NULL ;
  ZMapFeatureContextMergeCode merge_results = ZMAPFEATURE_CONTEXT_ERROR ;
  ZMapFeatureContext diff_context = NULL ;
  GList *masked;


  zMapPrintTimer(NULL, "Got Features from Thread") ;

  /* May be a new context or a merge with an existing one. */
  /*
   * MH17: moved mergeAndDrawContext code here:
   * Error handling is rubbish here...stuff needs to be free whether there is an error or not.
   *
   * new_features should be freed (but not the data...ahhhh actually the merge should
   * free any replicated data...yes, that is what should happen. Then when it comes to
   * the diff we should not free the data but should free all our structs...
   *
   * We should free the context_inout context here....actually better
   * would to have a "free" flag............
   *  */
  if (feature_req->context)
    {
      ZMapFeatureContextMergeStats merge_stats = NULL ;

      new_features = feature_req->context ;

      merge_results = zmapJustMergeContext(zmap_view,
                                           &new_features, &merge_stats,
                                           &masked, connect_data->session.request_as_columns, TRUE) ;

      connect_data->loaded_features->merge_stats = *merge_stats ;

      g_free(merge_stats) ;



      if (merge_results == ZMAPFEATURE_CONTEXT_OK)
        {
          diff_context = new_features ;

          zmapJustDrawContext(zmap_view, diff_context, masked, NULL, connect_data) ;

          result = TRUE ;
        }
      else
        {
          feature_req->context = NULL ;

          if (merge_results == ZMAPFEATURE_CONTEXT_NONE)
            {
              g_set_error(&connect_data->error, ZMAP_VIEW_ERROR, ZMAPVIEW_ERROR_CONTEXT_EMPTY,
                          "Context merge failed because no new features found in new context.") ;

              zMapLogWarning("%s", connect_data->error->message) ;
            }
          else
            {
              g_set_error(&connect_data->error, ZMAP_VIEW_ERROR, ZMAPVIEW_ERROR_CONTEXT_SERIOUS,
                          "Context merge failed, serious error.") ;

              zMapLogCritical("%s", connect_data->error->message) ;
            }

          result = FALSE ;
        }
    }

  return result ;
}







