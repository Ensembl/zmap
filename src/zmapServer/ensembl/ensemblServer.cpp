/*  File: ensemblServer.c
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *              by making the appropriate calls to the ensembl server.
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
#include <ensemblServer_P.hpp>

/* ensc-core is compiled as C */
#ifdef __cplusplus
extern "C" {
#endif

#include <Object.h>
#include <EnsC.h>
#include <SliceAdaptor.h>
#include <SequenceAdaptor.h>
#include <SimpleFeature.h>
#include <DNAAlignFeature.h>
#include <DNAPepAlignFeature.h>
#include <RepeatFeature.h>
#include <Transcript.h>
#include <PredictionTranscript.h>

#ifdef __cplusplus
}
#endif

using namespace std;


#define ENSEMBL_PROTOCOL_STR "Ensembl"                            /* For error messages. */


// global struct for the ensembl data source, holds anything for which there needs to be just one
// copy for all server instances.
//
typedef struct GlobalStructType
{
  pthread_mutex_t mutex ;                                   /* lock to protect ensc-core library
                                                             * calls as they are not thread safe,
                                                             * i.e. anything using dba, slice etc. */



} GlobalStruct, *Global ;



/* Data to pass to get-features callback function for each block */
typedef struct GetFeaturesDataStructType
{
  EnsemblServer server ;

  GList *feature_set_names ;
  GHashTable *source_2_sourcedata ;
  GHashTable *featureset_2_column ;
  ZMapStyleTree *feature_styles ;

} GetFeaturesDataStruct, *GetFeaturesData ;


/* Data to pass to get-sequence callback function for each block */
typedef struct GetSequenceDataStructType
{
  EnsemblServer server ;

  ZMapStyleTree &styles ;
} GetSequenceDataStruct, *GetSequenceData ;


/* Struct to pass to callback function for each alignment. It calls the block
 * callback function with the block user data */
typedef struct DoAllAlignBlocksStructType
{
  EnsemblServer server ;

  GHFunc each_block_func ;
  gpointer each_block_data ;

} DoAllAlignBlocksStruct, *DoAllAlignBlocks ;


/* These provide the interface functions for an ensembl server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean createConnection(void **server_out,
                                 char *config_file, ZMapURL url,
                                 char *format,
                                 char *version_str, int timeout, pthread_mutex_t *mutex) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
static gboolean createConnection(void **server_out, ZMapConfigSource config_source) ;

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

static ZMapServerResponseType doGetSequences(EnsemblServer server, GList *sequences_inout) ;

static void setErrMsg(EnsemblServer server, char *new_msg) ;

static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data) ;

static gboolean getAllSimpleFeatures(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static gboolean getAllDNAAlignFeatures(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static gboolean getAllDNAPepAlignFeatures(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static gboolean getAllRepeatFeatures(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static gboolean getAllTranscripts(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids, GError **error) ;
static gboolean getAllPredictionTranscripts(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static gboolean getAllGenes(EnsemblServer server, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids, GError **error) ;

static const char* featureGetSOTerm(SeqFeature *rsf) ;

static ZMapFeature makeFeature(EnsemblServer server,
                               SeqFeature *rsf,
                               const char *feature_name_id,
                               const char *feature_name,
                               ZMapStyleMode feature_mode,
                               const char *source,
                               const char *gene_source,
                               const char *biotype,
                               const long start,
                               const long end,
                               const int match_start,
                               const int match_end,
                               GetFeaturesData get_features_data,
                               ZMapFeatureBlock feature_block,
                               GError **error,
                               set<GQuark> *transcript_ids = NULL) ;

static ZMapFeature makeFeatureSimple(EnsemblServer server, SimpleFeature *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static ZMapFeature makeFeatureBaseAlign(EnsemblServer server, BaseAlignFeature *rsf, ZMapHomolType homol_type, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static ZMapFeature makeFeatureRepeat(EnsemblServer server, RepeatFeature *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static ZMapFeature makeFeatureTranscript(EnsemblServer server, Transcript *rsf, const char *gene_source, const long start, const long end, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids, GError **error) ;
static ZMapFeature makeFeaturePredictionTranscript(EnsemblServer server, PredictionTranscript *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, GError **error) ;
static ZMapFeature makeFeatureGene(EnsemblServer server, Gene *rsf, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids, GError **error) ;
static void geneAddTranscripts(EnsemblServer server, Gene *rsf, const char *gene_source, GetFeaturesData get_features_data, ZMapFeatureBlock feature_block, set<GQuark> &transcript_ids, GError **error) ;

static void transcriptAddExons(EnsemblServer server, ZMapFeature feature, Vector *exons) ;

static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end) ;

static ZMapFeatureSet makeFeatureSet(EnsemblServer server, 
                                     const char *feature_name_id, 
                                     ZMapStyleMode feature_mode, 
                                     GQuark feature_set_id, 
                                     const char *source, 
                                     GetFeaturesData get_features_data, 
                                     ZMapFeatureBlock feature_block) ;

static Slice* getSlice(EnsemblServer server, const char *seq_name, long start, long end, int strand) ;
static char* getSequence(EnsemblServer server, const char *seq_name, long start, long end, int strand) ;





//
//              Globals
//

static GlobalStruct global_init_G ;




/*
 *             Server interface functions.
 */



/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an ensembl server. The functions are only visible through
 * this struct. */
void ensemblGetServerFuncs(ZMapServerFuncs ensembl_funcs)
{
  ensembl_funcs->global_init = globalInit ;
  ensembl_funcs->create = createConnection ;
  ensembl_funcs->open = openConnection ;
  ensembl_funcs->get_info = getInfo ;
  ensembl_funcs->feature_set_names = getFeatureSetNames ;
  ensembl_funcs->get_styles = getStyles ;
  ensembl_funcs->have_modes = haveModes ;
  ensembl_funcs->get_sequence = getSequences ;
  ensembl_funcs->set_context = setContext ;
  ensembl_funcs->get_features = getFeatures ;
  ensembl_funcs->get_context_sequences = getContextSequence ;
  ensembl_funcs->errmsg = lastErrorMsg ;
  ensembl_funcs->get_status = getStatus ;
  ensembl_funcs->get_connect_state = getConnectState ;
  ensembl_funcs->close = closeConnection;
  ensembl_funcs->destroy = destroyConnection ;

  return ;
}


/*
 *    Although these routines are static they form the external interface to the ensembl server.
 */

/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;
  Global global = &global_init_G ;

  /* Mutex used by the ensembl server to lock calls to the non-thread-safe ensc-core library.
   * We must create the mutex here so we can lock across all threads. */
  if (pthread_mutex_init(&(global->mutex), NULL) != 0)
    {
      zMapLogCritical("%s", "Cannot initialise ensembl, pthread_mutex_init() failed.") ;

      result = FALSE ;
    }

  if (result)
    {
        
      /* Initialise mysql. */
      if (mysql_library_init(0, NULL, NULL))
        {
          zMapLogCritical("%s", "Global initialisation of ensembl server failed.") ;

          result = FALSE ;
        }
    }

  /* Initialise the ensembl C API */
  if (result)
    {
      char *prog_name = g_strdup("zmap") ;

      initEnsC(1, &prog_name) ;

      g_free(prog_name) ;
    }

  return result ;
}


static gboolean createConnection(void **server_out, ZMapConfigSource config_source)
{
  gboolean result = TRUE ;
  zMapReturnValIfFail(config_source && config_source->urlObj(), result) ;

  EnsemblServer server ;
  Global global = &global_init_G ;
  const ZMapURL url = config_source->urlObj() ;

  /* Always return a server struct as it contains error message stuff. */
  server = (EnsemblServer)g_new0(EnsemblServerStruct, 1) ;
  *server_out = (void *)server ;

  server->source = config_source ;
  server->mutex = &(global->mutex) ;


  if (result)
    {
      server->host = g_strdup(url->host) ;
      server->port = url->port ;

      if (url->user)
        server->user = g_strdup(url->user) ;
      else
        server->user = g_strdup("anonymous") ;

      if (url->passwd && url->passwd[0] && url->passwd[0] != '\0')
        server->passwd = g_strdup(url->passwd) ;

      server->db_name = zMapURLGetQueryValue(url->query, "db_name") ;
      server->db_prefix = zMapURLGetQueryValue(url->query, "db_prefix") ;

      if (server->host && server->db_name)
        {
          result = TRUE ;
        }
      else if (!server->host)
        {
          setErrMsg(server, g_strdup("Cannot create connection, required value 'host' is missing from source url")) ;
          ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
        }
      else if (!server->db_name)
        {
          setErrMsg(server, g_strdup("Cannot create connection, required value 'db_name' is missing from source url")) ;
          ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
        }
    }

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  EnsemblServer server = (EnsemblServer)server_in ;

  zMapReturnValIfFail(server && req_open && req_open->sequence_map, ZMAP_SERVERRESPONSE_REQFAIL) ;

  server->sequence = g_strdup(g_quark_to_string(req_open->req_sequence)) ;
  server->zmap_start = req_open->zmap_start ;
  server->zmap_end = req_open->zmap_end ;

  ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host,
                 "Opening connection for '%s'", server->db_name) ;
  server->slice = getSlice(server, server->sequence, server->zmap_start, server->zmap_end, STRAND_UNDEF) ;

  if (server->slice)
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;

      if (!server->last_err_msg)
        setErrMsg(server, g_strdup_printf("Failed to get slice for %s (%s %d,%d)",
                                          server->db_name, server->sequence, server->zmap_start, server->zmap_end)) ;
    }

  return result ;
}

static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

  if (!server->dba)
    {
      pthread_mutex_lock(server->mutex) ;
      server->dba = DBAdaptor_new(server->host, server->user, server->passwd, server->db_name, server->port, NULL);
      pthread_mutex_unlock(server->mutex) ;
    }

  if (server && server->dba && server->dba->dbc)
    {
      pthread_mutex_lock(server->mutex) ;
      char *assembly_type = DBAdaptor_getAssemblyType(server->dba) ;
      pthread_mutex_unlock(server->mutex) ;

      if (assembly_type)
        {
          ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host,
                         "Assembly type %s\n", assembly_type);

          g_free(assembly_type) ;
          result = ZMAP_SERVERRESPONSE_OK ;
        }
      else
        {
          setErrMsg(server, g_strdup_printf("Failed to get assembly type for database %s", server->db_name)) ;
        }
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
  EnsemblServer server = (EnsemblServer)server_in ;

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
  EnsemblServer server = (EnsemblServer)server_in ;

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
  EnsemblServer server = (EnsemblServer)server_in ;

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


static gboolean getAllSimpleFeatures(EnsemblServer server,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block,
                                     GError **error)
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
        {
          makeFeatureSimple(server, rsf, get_features_data, feature_block, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", SimpleFeature_getDisplayLabel(sf));
        }

      //          Object_decRefCount(rsf);
      //          free(rsf);
      //          Object_decRefCount(sf);
      //          free(sf);

      // If it's a fatal error creating the feature then don't try to create any more
      if (zMapFeatureErrorIsFatal(error))
        break ;
    }

  return result;
}

static gboolean getAllDNAAlignFeatures(EnsemblServer server,
                                       GetFeaturesData get_features_data,
                                       ZMapFeatureBlock feature_block,
                                       GError **error)
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
        {
          makeFeatureBaseAlign(server, (BaseAlignFeature*)rsf, ZMAPHOMOL_N_HOMOL, get_features_data, feature_block, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", BaseAlignFeature_getHitSeqName((BaseAlignFeature*)sf));
        }

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}

static gboolean getAllDNAPepAlignFeatures(EnsemblServer server,
                                          GetFeaturesData get_features_data,
                                          ZMapFeatureBlock feature_block,
                                          GError **error)
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
        {
          makeFeatureBaseAlign(server, (BaseAlignFeature*)rsf, ZMAPHOMOL_X_HOMOL, get_features_data, feature_block, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", BaseAlignFeature_getHitSeqName((BaseAlignFeature*)sf));
        }

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllRepeatFeatures(EnsemblServer server,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block,
                                     GError **error)
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
        {
          makeFeatureRepeat(server, rsf, get_features_data, feature_block, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", RepeatConsensus_getName(RepeatFeature_getConsensus(sf)));
        }

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllTranscripts(EnsemblServer server,
                                  GetFeaturesData get_features_data,
                                  ZMapFeatureBlock feature_block,
                                  set<GQuark> &transcript_ids,
                                  GError **error)
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
        {
          const long start = SeqFeature_getStart((SeqFeature*)rsf) ;
          const long end = SeqFeature_getEnd((SeqFeature*)rsf) ;

          makeFeatureTranscript(server, rsf, NULL, start, end, get_features_data, feature_block, transcript_ids, error) ;
         
          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", Transcript_getSeqRegionName(sf)) ;
        }

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllPredictionTranscripts(EnsemblServer server,
                                            GetFeaturesData get_features_data,
                                            ZMapFeatureBlock feature_block,
                                            GError **error)
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
        {
          makeFeaturePredictionTranscript(server, rsf, get_features_data, feature_block, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", Transcript_getSeqRegionName(sf)) ;
        }

      //      Object_decRefCount(rsf);
      //      free(rsf);
      //        Object_decRefCount(sf);
      //        free(sf);
    }

  return result;
}


static gboolean getAllGenes(EnsemblServer server,
                            GetFeaturesData get_features_data,
                            ZMapFeatureBlock feature_block,
                            set<GQuark> &transcript_ids,
                            GError **error)
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
        {
          makeFeatureGene(server, rsf, get_features_data, feature_block, transcript_ids, error) ;

          // If it's a fatal error creating the feature then don't try to create any more
          if (zMapFeatureErrorIsFatal(error))
            break ;
        }
      else
        {
          printf("Failed to map feature '%s'\n", Gene_getExternalName(sf)) ;
        }

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
  EnsemblServer server = (EnsemblServer)server_in ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      char *sequence = NULL ;

      if ((sequence = getSequence(server, sequence_name, start, end, STRAND_UNDEF)))
        {
          // Why do we do this....because zmap expects lowercase dna...operations like revcomp
          // will fail with uppercase currently.....seems wasteful....but simplifies searches etc.   
          char *tmp = sequence ;

          sequence = g_ascii_strdown(sequence, -1) ;

          g_free(tmp) ;

          *dna_length_out = (end - start) + 1 ;
          *dna_sequence_out = sequence ;
        }
      else
        {
          char *estr;
          estr = g_strdup_printf("Error getting sequence for '%s' (%d to %d)",
                                 server->sequence, server->zmap_start, server->zmap_end) ;

          setErrMsg(server, estr) ;

          ZMAPSERVER_LOG(Warning, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
        }
    }

  return server->result ;
}


/* Return the last error message. */
static const char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  EnsemblServer server = (EnsemblServer)server_in ;

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
  //EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

/* Try to close the connection. */
static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  //EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  EnsemblServer server = (EnsemblServer)server_in ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  if (pthread_mutex_destroy(server->mutex) != 0)
    ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host, "%s",
                   "Cannot destroy connection cleanly, pthread_mutex_destroy() failed.") ;


  //if (server->slice)
  //  Slice_free(server->slice) ;

  if (server->sequence)
    g_free(server->sequence) ;

  if (server->host)
    g_free(server->host) ;

  if (server->passwd)
    g_free(server->passwd) ;

  g_free(server) ;

  return result ;
}


/* For each sequence makes a request to find the sequence and then set it in
 * the input ZMapSequence
 *
 * Function returns ZMAP_SERVERRESPONSE_OK if sequences were found and retrieved,
 * ZMAP_SERVERRESPONSE_REQFAIL otherwise.
 *
 */
static ZMapServerResponseType doGetSequences(EnsemblServer server, GList *sequences_inout)
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
static void setErrMsg(EnsemblServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}


static const char* featureGetSOTerm(SeqFeature *rsf)
{
  const char *SO_accession = NULL ;

  switch (rsf->objectType)
    {
    case CLASS_SEQFEATURE:
      break ;
    case CLASS_EXON: /* fall through */
    case CLASS_STICKYEXON:
      SO_accession = "exon" ;
      break ;
    case CLASS_TRANSCRIPT:
      SO_accession = "transcript" ;
      break ;
    case CLASS_GENE:
      SO_accession = "gene" ;
      break ;
    case CLASS_SIMPLEFEATURE:
      SO_accession = "sequence_feature" ;
      break ;
    case CLASS_INTRON: /* fall through */
    case CLASS_INTRONSUPPORTINGEVIDENCE:
      SO_accession = "intron" ;
      break ;
    case CLASS_REPEATFEATURE:
      SO_accession = "repeat_region" ;
      break ;
    case CLASS_BASEALIGNFEATURE:
      SO_accession = "match" ;
      break ;
    case CLASS_FEATUREPAIR:
      break ;
    case CLASS_DNADNAALIGNFEATURE:
      SO_accession = "nucleotide_match" ;
      break ;
    case CLASS_DNAPEPALIGNFEATURE:
      SO_accession = "protein_match" ;
      break ;
    case CLASS_REPEATCONSENSUS:
      //SO_accession = "consensus" ;
      break ;
    case CLASS_PREDICTIONTRANSCRIPT:
      SO_accession = "transcript" ;
      break ;
    case CLASS_ANNOTATEDSEQFEATURE:
      break ;
    case CLASS_HOMOLOGY:
      SO_accession = "match" ;
      break ;
    case CLASS_SYNTENYREGION:
      SO_accession = "syntenic_region" ;
      break ;
    case CLASS_PREDICTIONEXON:
      SO_accession = "exon" ;
      break ;
    default:
      break ;
    };

  return SO_accession ;
}


static ZMapFeature makeFeatureSimple(EnsemblServer server,
                                     SimpleFeature *rsf,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block,
                                     GError **error)
{
  ZMapFeature feature = NULL ;

  ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_BASIC ;
  const char *feature_name = NULL ;
  const char *featureset_name = NULL ;

  const long start = SeqFeature_getStart((SeqFeature*)rsf) ;
  const long end = SeqFeature_getEnd((SeqFeature*)rsf) ;

  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  if (!feature_name || *feature_name == '\0')
    feature_name = SimpleFeature_getDisplayLabel(rsf) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getLogicName(analysis) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getGFFSource(analysis) ;

  feature = makeFeature(server, (SeqFeature*)rsf, feature_name, feature_name, 
                        feature_mode, featureset_name, NULL, NULL, start, end, 0, 0, 
                        get_features_data, feature_block, error) ;

  return feature ;
}


static ZMapFeature makeFeatureRepeat(EnsemblServer server,
                                     RepeatFeature *rsf,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block,
                                     GError **error)
{
  ZMapFeature feature = NULL ;

  ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_BASIC ;
  const char *feature_name = NULL ;
  const char *featureset_name = NULL ;

  const long start = SeqFeature_getStart((SeqFeature*)rsf) ;
  const long end = SeqFeature_getEnd((SeqFeature*)rsf) ;

  RepeatConsensus *consensus = RepeatFeature_getConsensus(rsf) ;
  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  if ((!feature_name || *feature_name == '\0') && consensus)
    feature_name = RepeatConsensus_getName(consensus) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getLogicName(analysis) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getGFFSource(analysis) ;

  feature = makeFeature(server, (SeqFeature*)rsf, feature_name, feature_name, 
                        feature_mode, featureset_name, NULL, NULL, start, end, 0, 0, 
                        get_features_data, feature_block, error) ;

  return feature ;
}


static ZMapFeature makeFeatureGene(EnsemblServer server,
                                   Gene *rsf,
                                   GetFeaturesData get_features_data,
                                   ZMapFeatureBlock feature_block,
                                   set<GQuark> &transcript_ids, 
                                   GError **error)
{
  ZMapFeature feature = NULL ;

  const char *featureset_name = NULL ;
  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getLogicName(analysis) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getGFFSource(analysis) ;

  geneAddTranscripts(server, rsf, featureset_name, get_features_data, feature_block, transcript_ids, error) ;

  return feature ;
}


static void geneAddTranscripts(EnsemblServer server, 
                               Gene *rsf, 
                               const char *gene_featureset_name,
                               GetFeaturesData get_features_data, 
                               ZMapFeatureBlock feature_block,
                               set<GQuark> &transcript_ids,
                               GError **error)
{
  if (rsf)
    {
      Vector *transcripts = Gene_getAllTranscripts(rsf) ;

      int i = 0 ;
      for (i = 0; i < Vector_getNumElement(transcripts); ++i)
        {
          Transcript *transcript = (Transcript*)Vector_getElementAt(transcripts, i);

          const int offset = server->zmap_start - 1 ;

          const long start = Transcript_getStart(transcript) + offset ;
          const long end = Transcript_getEnd(transcript) + offset ;

          makeFeatureTranscript(server, transcript, 
                                gene_featureset_name, start, end, 
                                get_features_data, feature_block, transcript_ids, error) ;
        }

      //Vector_free(transcripts) ;
    }
}


static ZMapFeature makeFeatureTranscript(EnsemblServer server,
                                         Transcript *rsf,
                                         const char *gene_featureset_name,
                                         const long start,
                                         const long end,
                                         GetFeaturesData get_features_data,
                                         ZMapFeatureBlock feature_block,
                                         set<GQuark> &transcript_ids,
                                         GError **error)
{
  ZMapFeature feature = NULL ;

  ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_TRANSCRIPT ;
  const char *feature_name_id = NULL ;
  const char *feature_name = NULL ;
  const char *featureset_name = NULL ;
  const char *biotype = Transcript_getBiotype(rsf) ;
  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  /* If there are specific biotypes requested, only create this feature if it's in the list */
  if (!server->req_biotypes || 
      (biotype && zMap_g_list_find_quark(server->req_biotypes, g_quark_from_string(biotype))))
    {
      feature_name_id = Transcript_getStableId(rsf);
      feature_name = Transcript_getExternalName(rsf) ;

      if (!feature_name || *feature_name == '\0')
        feature_name = feature_name_id ;

      if (analysis && (!featureset_name || *featureset_name == '\0'))
        featureset_name = Analysis_getLogicName(analysis) ;

      if (analysis && (!featureset_name || *featureset_name == '\0'))
        featureset_name = Analysis_getGFFSource(analysis) ;

      feature = makeFeature(server, (SeqFeature*)rsf, feature_name_id, feature_name, 
                            feature_mode, featureset_name, gene_featureset_name, biotype, start, end, 0, 0, 
                            get_features_data, feature_block, error, &transcript_ids) ;

      if (feature)
        {
          /* If coding region start/end are not already set, these calls set them */
          int coding_region_start = Transcript_getCodingRegionStart(rsf) ;
          int coding_region_end = Transcript_getCodingRegionEnd(rsf) ;

          char coding_region_start_is_set = Transcript_getCodingRegionStartIsSet(rsf) ;
          char coding_region_end_is_set = Transcript_getCodingRegionEndIsSet(rsf) ;

          const int offset = server->zmap_start - 1 ;

          zMapFeatureTranscriptInit(feature);
          zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);

          if (coding_region_start_is_set && coding_region_end_is_set)
            {
              zMapFeatureAddTranscriptCDS(feature, TRUE, coding_region_start + offset, coding_region_end + offset);
            }

          Vector *exons = Transcript_getAllExons(rsf) ;
          transcriptAddExons(server, feature, exons) ;
          //Vector_free(exons) ;

          /* Get list of supporting features and convert it to a list of unique ids */
          Vector *supporting_features = Transcript_getAllSupportingFeatures(rsf);
          GList *evidence_list = NULL ;

          for (int k=0; k < Vector_getNumElement(supporting_features); ++k) 
            {
              BaseAlignFeature *baf = (BaseAlignFeature*)Vector_getElementAt(supporting_features, k) ;

              const char *baf_name = BaseAlignFeature_getHitSeqName(baf) ;
              const int baf_start = BaseAlignFeature_getStart(baf) ; 
              const int baf_end = BaseAlignFeature_getEnd(baf) ; 
              const int baf_hit_start = BaseAlignFeature_getHitStart(baf) ; 
              const int baf_hit_end = BaseAlignFeature_getHitEnd(baf) ; 
              const char baf_strand_c = BaseAlignFeature_getStrand(baf);

              ZMapStrand baf_strand = ZMAPSTRAND_NONE ;

              if (baf_strand_c > 0)
                baf_strand = ZMAPSTRAND_FORWARD ;
              else if (baf_strand_c < 0)
                baf_strand = ZMAPSTRAND_REVERSE ;

              GQuark baf_unique_id = zMapFeatureCreateID(ZMAPSTYLE_MODE_ALIGNMENT,
                                                        baf_name, baf_strand, baf_start, baf_end,
                                                        baf_hit_start, baf_hit_end);
              
              evidence_list = g_list_append(evidence_list, GINT_TO_POINTER(baf_unique_id)) ;
            }

          /* Set the evidence list in the transcript. This takes ownership of the list. */
          zMapFeatureTranscriptSetEvidence(evidence_list, feature) ;
        }
    }

  return feature ;
}


static ZMapFeature makeFeaturePredictionTranscript(EnsemblServer server,
                                                   PredictionTranscript *rsf,
                                                   GetFeaturesData get_features_data,
                                                   ZMapFeatureBlock feature_block,
                                                   GError **error)
{
  ZMapFeature feature = NULL ;

  ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_TRANSCRIPT ;
  const char *feature_name_id = NULL ;
  const char *feature_name = NULL ;
  const char *featureset_name = NULL ;

  const long start = SeqFeature_getStart((SeqFeature*)rsf) ;
  const long end = SeqFeature_getEnd((SeqFeature*)rsf) ;

  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  feature_name = PredictionTranscript_getStableId(rsf);
  feature_name = PredictionTranscript_getDisplayLabel(rsf) ;

  if (!feature_name || *feature_name == '\0')
    feature_name = feature_name_id ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getLogicName(analysis) ;

  if (analysis && (!featureset_name || *featureset_name == '\0'))
    featureset_name = Analysis_getGFFSource(analysis) ;

  if (!featureset_name || *featureset_name == '\0')
    featureset_name = featureGetSOTerm((SeqFeature*)rsf) ;
  
  feature = makeFeature(server, (SeqFeature*)rsf, feature_name_id, feature_name, 
                        feature_mode, featureset_name, NULL, NULL, start, end, 0, 0, 
                        get_features_data, feature_block, error) ;

  if (feature)
    {
      char coding_region_start_is_set = PredictionTranscript_getCodingRegionStartIsSet(rsf) ;
      char coding_region_end_is_set = PredictionTranscript_getCodingRegionEndIsSet(rsf) ;
      char start_is_set = PredictionTranscript_getStartIsSet(rsf) ;
      char end_is_set = PredictionTranscript_getEndIsSet(rsf) ;
      int coding_region_start = 0;
      int coding_region_end = 0;

      const int offset = server->zmap_start - 1 ;

      /* getCodingRegionStart gives an error if coding_region_start and start 
       * are both unset so we must check first */
      if (coding_region_start_is_set || start_is_set)
        coding_region_start = PredictionTranscript_getCodingRegionStart(rsf) ;

      if (coding_region_end_is_set || end_is_set)
        coding_region_end = PredictionTranscript_getCodingRegionEnd(rsf) ;

      zMapFeatureTranscriptInit(feature);
      zMapFeatureAddTranscriptStartEnd(feature, FALSE, 0, FALSE);

      if (coding_region_start_is_set && coding_region_end_is_set)
        {
          zMapFeatureAddTranscriptCDS(feature, TRUE, coding_region_start + offset, coding_region_end + offset);
        }

      Vector *exons = PredictionTranscript_getAllExons(rsf, 0) ;
      transcriptAddExons(server, feature, exons) ;
      //Vector_free(exons) ;
    }

  return feature ;
}


static void transcriptAddExons(EnsemblServer server, ZMapFeature feature, Vector *exons)
{
  if (feature && exons)
    {
      int i = 0 ;
      for (i = 0; i < Vector_getNumElement(exons); ++i)
        {
          SeqFeature *exon = (SeqFeature*)Vector_getElementAt(exons,i);
          const int offset = server->zmap_start - 1 ;

          ZMapSpanStruct span = {
            (int)exon->start + offset,
            (int)exon->end + offset
          };

          zMapFeatureAddTranscriptExonIntron(feature, &span, NULL) ;

          //zMapLogMessage("Added exon %d, %d (%d, %d)",
          //               span.x1, span.x2, exon->start, exon->end);
        }

      zMapFeatureTranscriptRecreateIntrons(feature) ;
    }
}


static ZMapFeature makeFeatureBaseAlign(EnsemblServer server,
                                        BaseAlignFeature *rsf,
                                        ZMapHomolType homol_type,
                                        GetFeaturesData get_features_data,
                                        ZMapFeatureBlock feature_block,
                                        GError **error)
{
  ZMapFeature feature = NULL ;
  const char *featureset_name = NULL ;

  const long start = SeqFeature_getStart((SeqFeature*)rsf) ;
  const long end = SeqFeature_getEnd((SeqFeature*)rsf) ;

  Analysis *analysis = SeqFeature_getAnalysis((SeqFeature*)rsf) ;

  /* Create the basic feature. We need to pass some alignment-specific fields */
  ZMapStyleMode feature_mode = ZMAPSTYLE_MODE_ALIGNMENT ;

  const char *feature_name_id = BaseAlignFeature_getHitSeqName(rsf) ;
  const char *feature_name = BaseAlignFeature_getHitSeqName(rsf) ;

  int match_start = BaseAlignFeature_getHitStart(rsf) ;
  int match_end = BaseAlignFeature_getHitEnd(rsf) ;

  if ((!featureset_name || *featureset_name == '\0') && analysis)
    featureset_name = Analysis_getLogicName(analysis) ;

  if ((!featureset_name || *featureset_name == '\0') && analysis)
    featureset_name = Analysis_getGFFSource(analysis) ;

  if (!featureset_name || *featureset_name == '\0')
    featureset_name = BaseAlignFeature_getDbName((BaseAlignFeature*)rsf) ;

  feature = makeFeature(server, (SeqFeature*)rsf, feature_name_id, feature_name,
                        feature_mode, featureset_name, NULL, NULL, start, end, match_start, match_end,
                        get_features_data, feature_block, error) ;

  if (feature)
    {
      /* Add the alignment data */
      GQuark clone_id = 0 ; /*! \todo Add clone id? */
      double percent_id = 0.0 ;
      int match_length = 0 ;
      ZMapStrand match_strand = ZMAPSTRAND_NONE ;
      ZMapPhase match_phase = ZMAPPHASE_NONE;
      GArray *align = NULL ; /* to do */
      unsigned int align_error = 0;
      gboolean has_local_sequence = FALSE ;
      char *sequence  = NULL ;
      char *cigar = NULL ;

      percent_id = BaseAlignFeature_getPercId(rsf) ;
      match_length = match_end - match_start + 1 ; /*! todo: work out from cigar string? */

      if (BaseAlignFeature_getHitStrand(rsf) > 0)
        match_strand = ZMAPSTRAND_FORWARD ;
      else if (BaseAlignFeature_getHitStrand(rsf) < 0)
        match_strand = ZMAPSTRAND_REVERSE ;

      /*! \todo Not sure if we can cast to ZMapPhase here from the seq feature's phase (unsigned char) */
      match_phase = (ZMapPhase)SeqFeature_getPhase((SeqFeature*)rsf) ;

      /* Get the align array from the cigar string */
      cigar = BaseAlignFeature_getCigarString(rsf) ;

      if (cigar)
        {
          if (!zMapFeatureAlignmentString2Gaps(ZMAPALIGN_FORMAT_CIGAR_ENSEMBL,
                                               feature->strand, feature->x1, feature->x2,
                                               match_strand, match_start, match_end,
                                               cigar, &align))
            {
              zMapLogWarning("Error converting cigar string to gaps array: %s", cigar) ;
            }
        }
      else
        {
          zMapLogWarning("%s", "Error getting cigar string") ;
        }

      zMapFeatureAddAlignmentData(feature, clone_id, percent_id, match_start, match_end,
                                  homol_type, match_length, match_strand, match_phase,
                                  align, align_error, has_local_sequence, sequence) ;

      //zMapLogMessage("Added align data: id %f, qstart %d, qend %d, qlen %d, qstrand %d, ph %d, cigar %s",
      //               percent_id, match_start, match_end, match_length, match_strand, match_phase, cigar) ;
    }

  return feature ;
}


/* Check whether we should load the given featureset */
static gboolean loadFeatureset(EnsemblServer server, const char *featureset_name)
{
  gboolean result = TRUE ;

  /* If the req_featuresets list is given then only load features in those
   * featuresets. Otherwise, load all features. */
  if (server->req_featuresets_only)
    {
      result = FALSE ;
      GQuark check_featureset_id = zMapFeatureSetCreateID(featureset_name) ;
      GList *item = server->req_featuresets;

      for ( ; item ; item = item->next)
        {
          GQuark featureset_id = zMapFeatureSetCreateID(g_quark_to_string(((GQuark)GPOINTER_TO_INT(item->data)))) ;
          
          if (featureset_id == check_featureset_id)
            {
              result = TRUE ;
              break ;
            }
        }
    }

  return result ;
}


static ZMapFeature makeFeature(EnsemblServer server, 
                               SeqFeature *rsf, 
                               const char *feature_name_id_in,
                               const char *feature_name_in,
                               ZMapStyleMode feature_mode,
                               const char *featureset_name,
                               const char *gene_featureset_name,
                               const char *biotype_in,
                               const long start,
                               const long end,
                               const int match_start,
                               const int match_end,
                               GetFeaturesData get_features_data,
                               ZMapFeatureBlock feature_block,
                               GError **error,
                               set<GQuark> *transcript_ids)
{
  ZMapFeature feature = NULL ;
  const char *feature_name_id = feature_name_id_in ;
  GQuark unique_id = 0 ;
  const char *feature_name = feature_name_in ;
  char *sequence = NULL ;
  const char *SO_accession = NULL ;
  gboolean has_score = TRUE ;
  double score = 0.0 ;
  ZMapStrand strand = ZMAPSTRAND_NONE ;
  char *featureset_unique_name = NULL ;
  const char *biotype = biotype_in ;

  SO_accession = featureGetSOTerm(rsf) ;

  /* We must have a SO accession and featureset_name. Also check whether we should load this
   * featureset. This checks whether the featureset_name or gene_featureset_name (if given) is in our
   * list of requested featuresets. */
  if (SO_accession && featureset_name && 
      (loadFeatureset(server, featureset_name) || loadFeatureset(server, gene_featureset_name)))
    {
      if (!feature_name_id || *feature_name_id == '\0')
        feature_name_id = featureset_name ;

      if (!feature_name || *feature_name == '\0')
        feature_name = feature_name_id ;

      has_score = TRUE ;
      score = SeqFeature_getScore(rsf) ;

      if (SeqFeature_getStrand(rsf) > 0)
        strand = ZMAPSTRAND_FORWARD ;
      else if (SeqFeature_getStrand(rsf) < 0)
        strand = ZMAPSTRAND_REVERSE ;

      /* If no biotype given, then use the SO type */
      if (!biotype)
        biotype = SO_accession ;

      /* Create the unique id from the db name, feature name and coords (cast away const... ugh) */
      unique_id = zMapFeatureCreateID(feature_mode, (char*)feature_name_id, strand, start, end, match_start, match_end);

      /* If it's a transcript, only add it if it's not been processed already */
      if (!transcript_ids ||
          transcript_ids->find(unique_id) == transcript_ids->end())
        {
          if (transcript_ids)
            transcript_ids->insert(unique_id);

          /* If a prefix is given, add it to the featureset name. If a prefix is not given, then
           * duplicate features from different servers will not be shown because their names will
           * not be unique. Specifying a prefix allows the user to show these duplicate
           * features. Longer term we would make this a configurable option and the server would
           * be taken into account when deciding whether a feature is unique or not. However,
           * that's quite a far-reaching change that needs some thought. */
          /* Another issue is that we can get different feature types with the same featureset
           * name, so ensure that the featureset id is unique by also appending the biotype or SO
           * term. Again, ideally this would be taken care of behind the scenes, so appending this
           * to the featureset name is a quick and dirty fix for now. */
          if (server->db_prefix)
            featureset_unique_name = g_strdup_printf("%s_%s_%s", server->db_prefix, featureset_name, biotype);
          else
            featureset_unique_name = g_strdup_printf("%s_%s", featureset_name, biotype) ;

          /* Find the featureset, or create it if it doesn't exist */
          GQuark featureset_unique_id = zMapFeatureSetCreateID((char*)featureset_unique_name) ;

          ZMapFeatureSet feature_set = (ZMapFeatureSet)zMapFeatureAnyGetFeatureByID((ZMapFeatureAny)feature_block,
                                                                                    featureset_unique_id,
                                                                                    ZMAPFEATURE_STRUCT_FEATURESET) ;

          if (!feature_set)
            {
              feature_set = makeFeatureSet(server, feature_name_id, feature_mode, 
                                           featureset_unique_id, featureset_unique_name, 
                                           get_features_data, feature_block) ;
            }

          if (feature_set)
            {
              /* ok, actually create the feature now */
              feature = zMapFeatureCreateEmpty(error) ;

              /* cast away const... ugh */
              if (feature)
                {
                  zMapFeatureAddStandardData(feature,
                                             (char*)g_quark_to_string(unique_id),
                                             (char*)feature_name,
                                             sequence,
                                             (char*)SO_accession,
                                             feature_mode,
                                             &feature_set->style,
                                             start,
                                             end,
                                             has_score,
                                             score,
                                             strand) ;

                  //zMapLogMessage("Created feature: name %featureset_namesource %s, so %s, mode %d, start %d, end %d, score %f, strand %d",
                  //               feature_name, featureset_name, SO_accession, feature_mode, start, end, score, strand) ;

                  /* add the new feature to the featureset */
                  ZMapFeature existing_feature = (ZMapFeature)g_hash_table_lookup(((ZMapFeatureAny)feature_set)->children, GINT_TO_POINTER(feature_name_id)) ;

                  if (!existing_feature)
                    zMapFeatureSetAddFeature(feature_set, feature) ;
                }
            }
        }
    }
  else if (!SO_accession)
    {
      zMapLogWarning("%s", "Could not create feature [%s]: could not determine SO accession", 
                     feature_name_id ? feature_name_id : "") ;
    }
  else if (!featureset_name)
    {
      zMapLogWarning("%s", "Could not create feature [%s]: could not determine featureset name",
                     feature_name_id ? feature_name_id : "") ;
    }
  else
    {
      /* Ignoring features in this featureset: nothing to do */
    }

  return feature ;
}


static ZMapFeatureSet makeFeatureSet(EnsemblServer server,
                                     const char *feature_name_id,
                                     ZMapStyleMode feature_mode,
                                     GQuark featureset_unique_id,
                                     const char *featureset_name,
                                     GetFeaturesData get_features_data,
                                     ZMapFeatureBlock feature_block)
{
  ZMapFeatureSet feature_set = NULL ;

  /*
   * Now deal with the featureset_unique_id -> data mapping referred to in the parser.
   */
  GQuark feature_style_id = 0 ;
  ZMapFeatureSource source_data = NULL ;

  if (get_features_data->source_2_sourcedata)
    {
      if (!(source_data = (ZMapFeatureSource)g_hash_table_lookup(get_features_data->source_2_sourcedata, GINT_TO_POINTER(featureset_unique_id))))
        {
          source_data = g_new0(ZMapFeatureSourceStruct,1);
          source_data->source_id = featureset_unique_id;
          source_data->source_text = g_quark_from_string(featureset_name);

          g_hash_table_insert(get_features_data->source_2_sourcedata,GINT_TO_POINTER(featureset_unique_id), source_data);

          zMapLogMessage("Created source_data: %s", g_quark_to_string(featureset_unique_id)) ;
        }

      if (source_data->style_id)
        feature_style_id = zMapStyleCreateID((char *) g_quark_to_string(source_data->style_id)) ;
      else
        feature_style_id = zMapStyleCreateID((char *) g_quark_to_string(source_data->source_id)) ;

      featureset_unique_id = source_data->source_id ;
      source_data->style_id = feature_style_id;
      //zMapLogMessage("Style id = %s", g_quark_to_string(source_data->style_id)) ;
    }
  else
    {
      featureset_unique_id = feature_style_id = zMapStyleCreateID((char*)featureset_name) ;
    }

  ZMapFeatureTypeStyle feature_style = NULL ;

  feature_style = zMapFindFeatureStyle(get_features_data->feature_styles, feature_style_id, feature_mode) ;

  if (feature_style)
    {
      if (source_data)
        source_data->style_id = feature_style_id;

      get_features_data->feature_styles->add_style(feature_style) ;

      if (source_data && feature_style->unique_id != feature_style_id)
        source_data->style_id = feature_style->unique_id;

      feature_set = zMapFeatureSetCreate((char*)g_quark_to_string(featureset_unique_id) , NULL, server->source) ;
      zMapFeatureBlockAddFeatureSet(feature_block, feature_set);
      get_features_data->feature_set_names = g_list_prepend(get_features_data->feature_set_names, GUINT_TO_POINTER(feature_set->unique_id)) ;

      zMapLogMessage("Created feature set: %s", g_quark_to_string(featureset_unique_id)) ;

      feature_set->style = feature_style;
    }
  else
    {
      zMapLogWarning("Error creating feature %s (%s); no feature style found for %s",
                     feature_name_id, g_quark_to_string(featureset_unique_id), g_quark_to_string(feature_style_id)) ;
    }

  return feature_set ;
}


/* Process all the alignments in a context. */
static void eachAlignment(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  DoAllAlignBlocks all_data = (DoAllAlignBlocks)user_data ;

  if (all_data->server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, all_data->each_block_func, all_data->each_block_data) ;

  return ;
}


// Issue a warning if the error is set and frees the error. Returns true if it was a fatal error.
static bool fatalError(GError **error)
{
  bool fatal_error = zMapFeatureErrorIsFatal(error) ;

  if (error && *error)
    {
      zMapCritical("Error loading features for ensembl server: %s", (*error)->message) ;
      g_error_free(*error) ;
      *error = NULL ;

    }

  return fatal_error ;
}


/* Get features in a block */
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  GetFeaturesData get_features_data = (GetFeaturesData)user_data ;
  EnsemblServer server = get_features_data->server ;

  if (server->slice)
    {
      pthread_mutex_lock(server->mutex) ;

      GError *g_error = NULL ; 

      if (!fatalError(&g_error))
        getAllSimpleFeatures(server, get_features_data, feature_block, &g_error) ;

      if (!fatalError(&g_error))      
        getAllDNAAlignFeatures(server, get_features_data, feature_block, &g_error) ;

      if (!fatalError(&g_error))
        getAllDNAPepAlignFeatures(server, get_features_data, feature_block, &g_error) ;

      if (!fatalError(&g_error))
        getAllRepeatFeatures(server, get_features_data, feature_block, &g_error) ;

      if (!fatalError(&g_error))
        getAllPredictionTranscripts(server, get_features_data, feature_block, &g_error) ;

      /* We get transcripts via the gene for genes whose logic_name is in the list of requested
       * featuresets. The transcript logic_name may be different to the gene logic_name so we
       * also look separately for transcripts by  logic_name. This means we may end up fetching
       * the same transcript twice, so we maintain a set of transcript stable_ids that we've seen
       * so that we don't add them twice. We may want to revisit this to fetch ALL genes and
       * therefore we can check all transcripts from there. We need to make sure that this won't cause
       * a performance problem, though. */
      set<GQuark> transcript_ids;
      if (!fatalError(&g_error))
        getAllGenes(server, get_features_data, feature_block, transcript_ids, &g_error) ;

      if (!fatalError(&g_error))
        getAllTranscripts(server, get_features_data, feature_block, transcript_ids, &g_error) ;

      pthread_mutex_unlock(server->mutex) ;
    }

  return ;
}



static Slice* getSliceForCoordSystem(const char *coord_system,
                                     EnsemblServer server,
                                     const char *seq_name,
                                     long start,
                                     long end,
                                     int strand)
{
  Slice *slice = NULL ;

  /* copy seq_name and coord_system because function takes non-const arg... ugh */
  char *seq_name_copy = g_strdup(seq_name);
  char *coord_system_copy = g_strdup(coord_system) ;

  pthread_mutex_lock(server->mutex) ;
  slice = SliceAdaptor_fetchByRegion(server->slice_adaptor, coord_system_copy, seq_name_copy, start, end, strand, NULL, 0);
  pthread_mutex_unlock(server->mutex) ;

  g_free(seq_name_copy);
  g_free(coord_system_copy) ;

  /* Remember the coord system we used */
  if (slice)
    server->coord_system = coord_system ;

  return slice ;
}


static Slice* getSlice(EnsemblServer server,
                       const char *seq_name,
                       long start,
                       long end,
                       int strand)
{
  Slice *slice = NULL ;

  if (!server->dba)
    {
      pthread_mutex_lock(server->mutex) ;
      server->dba = DBAdaptor_new(server->host, server->user, server->passwd, server->db_name, server->port, NULL);
      pthread_mutex_unlock(server->mutex) ;
    }

  if (server->dba && server->dba->dbc)
    {
      if (!server->slice_adaptor)
        {
          pthread_mutex_lock(server->mutex) ;
          server->slice_adaptor = DBAdaptor_getSliceAdaptor(server->dba);
          pthread_mutex_unlock(server->mutex) ;
        }

      if (!slice) 
        {
          slice = getSliceForCoordSystem("toplevel", server, seq_name, start, end, strand) ;
        }

      if (server->slice_adaptor && slice)
        {
          server->result = ZMAP_SERVERRESPONSE_OK ;
        }
      else
        {
          setErrMsg(server, g_strdup_printf("Failed to get slice for %s (%s %d,%d)",
                                            server->db_name, server->sequence, server->zmap_start, server->zmap_end)) ;
        }
    }
  else
    {
      setErrMsg(server, g_strdup_printf("Failed to create database connection for %s", server->db_name)) ;
    }

  return slice ;
}


static char* getSequence(EnsemblServer server,
                         const char *seq_name,
                         long start,
                         long end,
                         int strand)
{
  char *seq = NULL ;

  if (!server->dba)
    {
      pthread_mutex_lock(server->mutex) ;
      server->dba = DBAdaptor_new(server->host, server->user, server->passwd, server->db_name, server->port, NULL);
      pthread_mutex_unlock(server->mutex) ;
    }

  if (server->dba && server->dba->dbc)
    {
      if (!server->slice_adaptor)
        {
          pthread_mutex_lock(server->mutex) ;
          server->slice_adaptor = DBAdaptor_getSliceAdaptor(server->dba);
          pthread_mutex_unlock(server->mutex) ;
        }

      if (!server->seq_adaptor)
        {
          pthread_mutex_lock(server->mutex) ;
          server->seq_adaptor = DBAdaptor_getSequenceAdaptor(server->dba);
          pthread_mutex_unlock(server->mutex) ;
        }

      if (server->slice_adaptor && server->seq_adaptor && server->coord_system)
        {
          /* copy seq_name and coord system because function takes non-const arg... ugh */
          char *seq_name_copy = g_strdup(seq_name) ;
          char *coord_system = g_strdup("chromosome") ;

          pthread_mutex_lock(server->mutex) ;
          Slice *slice = SliceAdaptor_fetchByRegion(server->slice_adaptor, coord_system,
                                                    seq_name_copy, start, end,
                                                    strand, NULL, 0);
          pthread_mutex_unlock(server->mutex) ;

          g_free(seq_name_copy) ;
          g_free(coord_system) ;
          seq_name_copy = NULL ;
          coord_system = NULL ;

          if (slice)
            {
              pthread_mutex_lock(server->mutex) ;

              seq = SequenceAdaptor_fetchBySliceStartEndStrand(server->seq_adaptor, slice, 1, POS_UNDEF, 1);
              Slice_free(slice) ;

              pthread_mutex_unlock(server->mutex) ;
            }
        }
    }


  return seq ;
}
