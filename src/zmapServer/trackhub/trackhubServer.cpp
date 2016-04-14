/*  File: trackhubServer.cpp
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
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
 * and was written by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the method functions for a zmap server
 *              by making the appropriate calls to the trackhub server.
 *
 * Exported functions: See zmapServer.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <set>
#include <string>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>
#include <ZMap/zmapConfigIni.hpp>
#include <ZMap/zmapConfigStrings.hpp>
#include <ZMap/zmapGFF.hpp>
#include <ZMap/zmapStyle.hpp>
#include <ZMap/zmapUrl.hpp>
#include <zmapServerPrototype.hpp>
#include <trackhubServer_P.hpp>


using namespace std;


#define TRACKHUB_PROTOCOL_STR "Trackhub"                            /* For error messages. */

/* Data to pass to get-features callback function for each block */
typedef struct GetFeaturesDataStructType
{
  TrackhubServer server ;

  GList *feature_set_names ;
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;
  ZMapStyleTree *feature_styles ;

} GetFeaturesDataStruct, *GetFeaturesData ;


/* Data to pass to get-sequence callback function for each block */
typedef struct GetSequenceDataStructType
{
  TrackhubServer server ;

  ZMapStyleTree &styles ;
} GetSequenceDataStruct, *GetSequenceData ;


/* Struct to pass to callback function for each alignment. It calls the block
 * callback function with the block user data */
typedef struct DoAllAlignBlocksStructType
{
  TrackhubServer server ;

  GHFunc each_block_func ;
  gpointer each_block_data ;

} DoAllAlignBlocksStruct, *DoAllAlignBlocks ;


/* These provide the interface functions for a trackhub server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
                                 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout, pthread_mutex_t *mutex) ;
static ZMapServerResponseType openConnection(void *server, ZMapServerReqOpen req_open) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerReqGetServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
                                                 GList **feature_sets_out,
                                                 GList **biotypes_out,
                                                 GList *sources,
                                                 GList **required_styles,
                                                 GHashTable **featureset_2_stylelist_inout,
                                                 GHashTable **featureset_2_column_inout,
                                                 GHashTable **source_2_sourcedata_inout) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, ZMapStyleTree &styles,
                                          ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in,
                                                 char *sequence_name, int start, int end,
                                                 int *dna_length_out, char **dna_sequence_out) ;
static const char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_in, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_in, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static ZMapServerResponseType doGetSequences(TrackhubServer server, GList *sequences_inout) ;

static void setErrMsg(TrackhubServer server, char *new_msg) ;

static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data) ;

static gboolean getAllSimpleFeatures(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static gboolean getAllDNAAlignFeatures(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static gboolean getAllDNAPepAlignFeatures(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static gboolean getAllRepeatFeatures(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static gboolean getAllTranscripts(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids) ;
static gboolean getAllPredictionTranscripts(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static gboolean getAllGenes(TrackhubServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids) ;

static const char* featureGetSOTerm(SeqFeature *rsf) ;

static ZMapFeature makeFeature(TrackhubServer server,
                               SeqFeature *rsf,
                               const char *feature_name_id,
                               const char *feature_name,
                               ZMapStyleMode feature_mode,
                               const char *source,
                               const char *gene_source,
                               const char *biotype,
                               const int match_start,
                               const int match_end,
                               GetFeaturesData get_features_data,
                               ZMapFeatureBlock feature_block) ;

static ZMapFeature makeFeatureSimple(TrackhubServer server, SimpleFeature *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static ZMapFeature makeFeatureBaseAlign(TrackhubServer server, BaseAlignFeature *rsf, ZMapHomolType homol_type, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static ZMapFeature makeFeatureRepeat(TrackhubServer server, RepeatFeature *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static ZMapFeature makeFeatureTranscript(TrackhubServer server, Transcript *rsf, const char *gene_source, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids) ;
static ZMapFeature makeFeaturePredictionTranscript(TrackhubServer server, PredictionTranscript *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;
static ZMapFeature makeFeatureGene(TrackhubServer server, Gene *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids) ;
static void geneAddTranscripts(TrackhubServer server, Gene *rsf, const char *gene_source, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids) ;

static void transcriptAddExons(TrackhubServer server, ZMapFeature feature, Vector *exons) ;

static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end) ;

static ZMapFeatureSet makeFeatureSet(const char *feature_name_id, GQuark feature_set_id, ZMapStyleMode feature_mode, const char *source, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block) ;

static Slice* getSlice(TrackhubServer server, const char *seq_name, long start, long end, int strand) ;
static char* getSequence(TrackhubServer server, const char *seq_name, long start, long end, int strand) ;

/*
 *             Server interface functions.
 */



/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact a trackhub server. The functions are only visible through
 * this struct. */
void trackhubGetServerFuncs(ZMapServerFuncs trackhub_funcs)
{
  trackhub_funcs->global_init = globalInit ;
  trackhub_funcs->create = createConnection ;
  trackhub_funcs->open = openConnection ;
  trackhub_funcs->get_info = getInfo ;
  trackhub_funcs->feature_set_names = getFeatureSetNames ;
  trackhub_funcs->get_styles = getStyles ;
  trackhub_funcs->have_modes = haveModes ;
  trackhub_funcs->get_sequence = getSequences ;
  trackhub_funcs->set_context = setContext ;
  trackhub_funcs->get_features = getFeatures ;
  trackhub_funcs->get_context_sequences = getContextSequence ;
  trackhub_funcs->errmsg = lastErrorMsg ;
  trackhub_funcs->get_status = getStatus ;
  trackhub_funcs->get_connect_state = getConnectState ;
  trackhub_funcs->close = closeConnection;
  trackhub_funcs->destroy = destroyConnection ;

  return ;
}


/*
 *    Although these routines are static they form the external interface to the trackhub server.
 */

/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  return result ;
}

static gboolean createConnection(void **server_out,
                                 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout, pthread_mutex_t *mutex)
{
  gboolean result = FALSE ;
  TrackhubServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (TrackhubServer)g_new0(TrackhubServerStruct, 1) ;
  *server_out = (void *)server ;

  server->config_file = g_strdup(config_file) ;

  if (url && url->path)
    server->trackdb_id = g_strdup(url->path) ;

  if (server->trackdb_id)
    {
      result = TRUE ;
    }
  else
    {
      setErrMsg(server, g_strdup("Cannot create connection: no track database ID")) ;
      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->path, "%s", server->last_err_msg) ;
    }

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  TrackhubServer server = (TrackhubServer)server_in ;

  zMapReturnValIfFail(server && req_open && req_open->sequence_map, ZMAP_SERVERRESPONSE_REQFAIL) ;

  server->sequence = g_strdup(req_open->sequence_map->sequence) ;
  server->zmap_start = req_open->zmap_start ;
  server->zmap_end = req_open->zmap_end ;

  if (server->registry->ping())
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      setErrMsg(server, g_strdup("Cannot open connection: failed to contact Track Hub Registry")) ;
      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->path, "%s", server->last_err_msg) ;
    }

  return result ;
}

static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  TrackhubServer server = (TrackhubServer)server_in ;

  if (server)
    {
      double version = server->registry->version() ;

      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->path,
                     "Track Hub Registry version: %s", version) ;

      result = ZMAP_SERVERRESPONSE_OK ;
    }

  return result ;
}


/* This function must do a lot of checking and it is vital this is done well otherwise we end up
 * with styles/methods we don't need or much worse we don't load all the styles that the
 * feature sets require.
 *
 * This function takes a list of names, checks that it can find the corresponding objects
 * and then retrieves those objects.
 *
 *  */
static ZMapServerResponseType getFeatureSetNames(void *server_in,
                                                 GList **feature_sets_inout,
                                                 GList **biotypes_inout,
                                                 GList *sources,
                                                 GList **required_styles_out,
                                                 GHashTable **featureset_2_stylelist_inout,
                                                 GHashTable **featureset_2_column_inout,
                                                 GHashTable **source_2_sourcedata_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  TrackhubServer server = (TrackhubServer)server_in ;

  static GQuark all_quark = 0 ;
  if (!all_quark)
    all_quark = g_quark_from_string("all") ;

  if (feature_sets_inout)
    {
      server->req_featuresets = *feature_sets_inout ;
      server->req_biotypes = *biotypes_inout ;

      if (server->req_featuresets)
        {
          /* We will load only the requested featuresets, unless the list starts with "all" which is
           * an indicator that we still load all featuresets we can find (this is sometimes used
           * if we want to specify default columns such as DNA in the featuresets list). */
          server->req_featuresets_only = ((GQuark)(GPOINTER_TO_INT(server->req_featuresets->data)) != all_quark) ;
        }        
    }

  if (source_2_sourcedata_inout)
    server->source_2_sourcedata = *source_2_sourcedata_inout;

  if (featureset_2_column_inout)
    server->featureset_2_column = *featureset_2_column_inout;

  result = ZMAP_SERVERRESPONSE_OK ;
  return result ;
}

static ZMapServerResponseType getStyles(void *server_in, GHashTable **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  return result ;
}

static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  return result ;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
  TrackhubServer server = (TrackhubServer)server_in ;

  doGetSequences(server, sequences_inout) ;

  return result ;
}




/* We don't check anything here, we could look in the data to check that it matches the context
 * I guess which is essentially what we do for the acedb server.
 * NB: as we process a stream we cannot search backwards or forwards
 */
static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}


/* Get all features on a sequence. */
static ZMapServerResponseType getFeatures(void *server_in, ZMapStyleTree &styles,
                                          ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  TrackhubServer server = (TrackhubServer)server_in ;

  addMapping(feature_context, server->zmap_start, server->zmap_end) ;

  GetFeaturesDataStruct get_features_data ;

  get_features_data.server = server ;
  get_features_data.feature_set_names = NULL ;
  get_features_data.source_2_sourcedata = server->source_2_sourcedata ;
  get_features_data.featureset_2_column = server->featureset_2_column ;
  get_features_data.feature_styles = &styles ;

  DoAllAlignBlocksStruct all_data ;
  all_data.server = server ;
  all_data.each_block_func = eachBlockGetFeatures ;
  all_data.each_block_data = &get_features_data ;

  /* Fetch all the alignment blocks for all the sequences. */
  g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&all_data) ;

  feature_context->src_feature_set_names = get_features_data.feature_set_names ;

  return result ;
}


static gboolean getAllSimpleFeatures(TrackhubServer server,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;

      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllSimpleFeatures(server->slice, (char*)featureset, NULL, NULL);
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllSimpleFeatures(server->slice, NULL, NULL, NULL);
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      SimpleFeature *sf = (SimpleFeature*)Vector_getElementAt(features,i) ;
      SimpleFeature *rsf = (SimpleFeature*)SeqFeature_transform((SeqFeature*)sf,  (char *)(server->coord_system), NULL ,NULL) ;

      if (rsf)
        makeFeatureSimple(server, rsf, get_features_data, feature_block) ;
      else
        printf("Failed to map feature '%s'\n", SimpleFeature_getDisplayLabel(sf));

      //          Object_decRefCount(rsf);
      //          free(rsf);
      //          Object_decRefCount(sf);
      //          free(sf);
    }

  return result;
}

static gboolean getAllDNAAlignFeatures(TrackhubServer server,
                                       GetFeaturesData get_features_data,
                                       ZMapFeatureBlock feature_block)
{
  gboolean result = TRUE ;

  Vector *features = NULL;
  
  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllDNAAlignFeatures(server->slice, (char*)featureset, NULL, NULL, NULL);
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllDNAAlignFeatures(server->slice, NULL, NULL, NULL, NULL);
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      DNAAlignFeature *sf = (DNAAlignFeature*)Vector_getElementAt(features,i);
      DNAAlignFeature *rsf = (DNAAlignFeature*)SeqFeature_transform((SeqFeature*)sf, (char *)(server->coord_system), NULL, NULL);

      if (rsf)
        makeFeatureBaseAlign(server, (BaseAlignFeature*)rsf, ZMAPHOMOL_N_HOMOL, get_features_data, feature_block) ;
      else
        printf("Failed to map feature '%s'\n", BaseAlignFeature_getHitSeqName((BaseAlignFeature*)sf));

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}

static gboolean getAllDNAPepAlignFeatures(TrackhubServer server,
                                          GetFeaturesData get_features_data,
                                          ZMapFeatureBlock feature_block)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllProteinAlignFeatures(server->slice, (char*)featureset, NULL, NULL, NULL);
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllProteinAlignFeatures(server->slice, NULL, NULL, NULL, NULL);
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      DNAPepAlignFeature *sf = (DNAPepAlignFeature*)Vector_getElementAt(features,i);
      DNAPepAlignFeature *rsf = (DNAPepAlignFeature*)SeqFeature_transform((SeqFeature*)sf, (char *)(server->coord_system), NULL, NULL);

      if (rsf)
        makeFeatureBaseAlign(server, (BaseAlignFeature*)rsf, ZMAPHOMOL_X_HOMOL, get_features_data, feature_block) ;
      else
        printf("Failed to map feature '%s'\n", BaseAlignFeature_getHitSeqName((BaseAlignFeature*)sf));

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllRepeatFeatures(TrackhubServer server,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllRepeatFeatures(server->slice, (char*)featureset, NULL, NULL);
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllRepeatFeatures(server->slice, NULL, NULL, NULL);
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      RepeatFeature *sf = (RepeatFeature*)Vector_getElementAt(features,i);
      RepeatFeature *rsf = (RepeatFeature*)SeqFeature_transform((SeqFeature*)sf, (char *)(server->coord_system), NULL, NULL);

      if (rsf)
        makeFeatureRepeat(server, rsf, get_features_data, feature_block) ;
      else
        printf("Failed to map feature '%s'\n", RepeatConsensus_getName(RepeatFeature_getConsensus(sf)));

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllTranscripts(TrackhubServer server,
                                  GetFeaturesData get_features_data,
                                  ZMapFeatureBlock feature_block,
                                  set<GQuark> &transcript_ids)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllTranscripts(server->slice, 1, (char*)featureset, NULL) ;
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllTranscripts(server->slice, 1, NULL, NULL) ;
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      Transcript *sf = (Transcript*)Vector_getElementAt(features,i);
      Transcript *rsf = (Transcript*)SeqFeature_transform((SeqFeature*)sf, (char *)(server->coord_system), NULL, NULL);

      if (rsf)
        makeFeatureTranscript(server, rsf, NULL, get_features_data, feature_block, transcript_ids) ;
      else
        printf("Failed to map feature '%s'\n", Transcript_getSeqRegionName(sf)) ;

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllPredictionTranscripts(TrackhubServer server,
                                            GetFeaturesData get_features_data,
                                            ZMapFeatureBlock feature_block)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllPredictionTranscripts(server->slice, (char*)featureset, 1, NULL) ;
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllPredictionTranscripts(server->slice, NULL, 1, NULL) ;
    }

  const int num_element = Vector_getNumElement(features) ;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      PredictionTranscript *sf = (PredictionTranscript*)Vector_getElementAt(features,i);
      PredictionTranscript *rsf = (PredictionTranscript*)SeqFeature_transform((SeqFeature*)sf, (char *)(server->coord_system), NULL, NULL);

      if (rsf)
        makeFeaturePredictionTranscript(server, rsf, get_features_data, feature_block) ;
      else
        printf("Failed to map feature '%s'\n", Transcript_getSeqRegionName(sf)) ;

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllGenes(TrackhubServer server,
                            GetFeaturesData get_features_data,
                            ZMapFeatureBlock feature_block,
                            set<GQuark> &transcript_ids)
{
  gboolean result = TRUE ;

  Vector *features = NULL;

  if (server->req_featuresets_only)
    {
      /* Get features for each requested featureset */
      features = Vector_new();
      GList *item = server->req_featuresets;
      for ( ; item ; item = item->next)
        {
          const char *featureset = g_quark_to_string(GPOINTER_TO_INT(item->data));
          Vector *new_features = Slice_getAllGenes(server->slice, (char*)featureset, NULL, 1, NULL, NULL) ;
          Vector_append(features, new_features);
          //Vector_free(new_features);
        }
    }
  else
    {
      /* No specific featuresets requested, so get everything */
      features = Slice_getAllGenes(server->slice, NULL, NULL, 1, NULL, NULL) ;
    }

  const int num_element = features ? Vector_getNumElement(features) : 0;
  int i = 0 ;

  for (i = 0; i < num_element && result; ++i)
    {
      Gene *sf = (Gene*)Vector_getElementAt(features,i);
      char *cs_name = g_strdup("chromosome");
      Gene *rsf = (Gene*)SeqFeature_transform((SeqFeature*)sf,cs_name,NULL,NULL);
      g_free(cs_name);
      cs_name = NULL;

      if (rsf)
        makeFeatureGene(server, rsf, get_features_data, feature_block, transcript_ids) ;
      else
        printf("Failed to map feature '%s'\n", Gene_getExternalName(sf)) ;

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


/* A bit of a lash up for now, we need the parent->child mapping for a sequence and since
 * the code in this file so far simply reads a GFF stream for now, we just fake it by setting
 * everything to be the same for child/parent... */
static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end)
{
  ZMapFeatureBlock feature_block = NULL;    // feature_context->master_align->blocks->data ;

  feature_block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(feature_context->master_align->blocks, 0)) ;

  /* We just override whatever is already there...this may not be the thing to do if there
   * are several streams.... */
  feature_context->parent_name = feature_context->sequence_name ;

  // refer to comment in zmapFeature.h 'Sequences and Block Coordinates'
  // NB at time of writing parent_span not always initialised
  feature_context->parent_span.x1 = 1;
  if (feature_context->parent_span.x2 < req_end)
    feature_context->parent_span.x2 = req_end ;

  // seq range  from parent sequence
  if (!feature_context->master_align->sequence_span.x2)
    {
      feature_context->master_align->sequence_span.x1 = req_start ;
      feature_context->master_align->sequence_span.x2 = req_end ;
    }

  // seq coords for our block NOTE must be block coords not features sub-sequence in case of req from mark
  if (!feature_block->block_to_sequence.block.x2)
    {
      feature_block->block_to_sequence.block.x1 = req_start ;
      feature_block->block_to_sequence.block.x2 = req_end ;
    }

  // parent sequence coordinates if not pre-specified
  if (!feature_block->block_to_sequence.parent.x2)
    {
      feature_block->block_to_sequence.parent.x1 = req_start ;
      feature_block->block_to_sequence.parent.x2 = req_end ;
    }

  return ;
}

/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in,
                                                 char *sequence_name, int start, int end,
                                                 int *dna_length_out, char **dna_sequence_out)
{
  TrackhubServer server = (TrackhubServer)server_in ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
    }

  return server->result ;
}


/* Return the last error message. */
static const char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  TrackhubServer server = (TrackhubServer)server_in ;

  if (server && server->last_err_msg)
    err_msg = server->last_err_msg ;

  return err_msg ;
}


/* Truly pointless operation revealing that this function is a bad idea..... */
static ZMapServerResponseType getStatus(void *server_in, gint *exit_code)
{
  *exit_code = 0 ;

  return ZMAP_SERVERRESPONSE_OK;
}

/* Is the server connected ? */
static ZMapServerResponseType getConnectState(void *server_in, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  //TrackhubServer server = (TrackhubServer)server_in ;

  return result ;
}

/* Try to close the connection. */
static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  //TrackhubServer server = (TrackhubServer)server_in ;

  return result ;
}



/* For each sequence makes a request to find the sequence and then set it in
 * the input ZMapSequence
 *
 * Function returns ZMAP_SERVERRESPONSE_OK if sequences were found and retrieved,
 * ZMAP_SERVERRESPONSE_REQFAIL otherwise.
 *
 */
static ZMapServerResponseType doGetSequences(TrackhubServer server, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  GList *next_seq ;

  /* We need to loop round finding each sequence and then fetching its dna... */
  next_seq = sequences_inout ;
  while (next_seq)
    {
      ZMapSequence sequence = (ZMapSequence)(next_seq->data) ;

      char *seq_name = g_strdup(g_quark_to_string(sequence->name)) ;

      char *seq = getSequence(server, seq_name, 1, POS_UNDEF, STRAND_UNDEF) ;

      if (seq)
        {
          sequence->sequence = seq ;

          result = ZMAP_SERVERRESPONSE_OK ;
        }
      else
        {
          setErrMsg(server, g_strdup_printf("Failed to fetch %s sequence for object \"%s\".",
                                            (sequence->type == ZMAPSEQUENCE_DNA ? "nucleotide" : "peptide"),
                                            seq_name)) ;

          result = ZMAP_SERVERRESPONSE_REQFAIL ;
        }

      if (seq_name)
        g_free(seq_name) ;

      next_seq = g_list_next(next_seq) ;
    }

  return result ;
}


/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(TrackhubServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}


