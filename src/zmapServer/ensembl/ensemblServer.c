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

#include <ZMap/zmap.h>

#include <string.h>
#include <stdio.h>
#include <AceConn.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapStyle.h>
#include <ZMap/zmapUrl.h>
#include <zmapServerPrototype.h>
#include <ensemblServer_P.h>
#include <EnsC.h>
#include <SliceAdaptor.h>
#include <SequenceAdaptor.h>
#include <SimpleFeature.h>
#include <DNAAlignFeature.h>
#include <DNAPepAlignFeature.h>
#include <RepeatFeature.h>

#define ENSEMBL_PROTOCOL_STR "Ensembl"                            /* For error messages. */


/* These provide the interface functions for an ensembl server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
                                 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server, ZMapServerReqOpen req_open) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerReqGetServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
                                                 GList **feature_sets_out,
                                                 GList *sources,
                                                 GList **required_styles,
                                                 GHashTable **featureset_2_stylelist_inout,
                                                 GHashTable **featureset_2_column_inout,
                                                 GHashTable **source_2_sourcedata_inout) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles,
                                          ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles,
                                                 ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_in, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_in, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static ZMapServerResponseType doGetSequences(EnsemblServer server, GList *sequences_inout) ;

static void setErrMsg(EnsemblServer server, char *new_msg) ;


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

  return result ;
}

static gboolean createConnection(void **server_out,
                                 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout)
{
  gboolean result = FALSE ;
  //GError *error = NULL ;
  EnsemblServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (EnsemblServer)g_new0(EnsemblServerStruct, 1) ;
  *server_out = (void *)server ;

  server->config_file = g_strdup(config_file) ;
  server->mutex = g_mutex_new() ;

  server->host = g_strdup(url->host) ;
  server->port = url->port ;

  if (url->user)
    server->user = g_strdup(url->user) ;
  else
    server->user = g_strdup("anonymous") ;

  if (url->passwd && url->passwd[0] && url->passwd[0] != '\0')
    server->passwd = g_strdup(url->passwd) ;

  server->db_name = zMapURLGetQueryValue(url->query, "db_name") ;

  /* Initialise the ensembl C API the first time we are called
   * put this but we need to make sure  */
  static gboolean first = TRUE ;

  if (first)
    {
      char *prog_name = g_strdup("zmap") ;
      initEnsC(1, &prog_name) ;
      g_free(prog_name) ;
      first = FALSE ;
    }

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

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

  zMapReturnValIfFail(server && req_open && req_open->sequence_map, result) ;

  g_mutex_lock(server->mutex) ;

  server->sequence = g_strdup(req_open->sequence_map->sequence) ;
  server->zmap_start = req_open->zmap_start ;
  server->zmap_end = req_open->zmap_end ;

  ZMAPSERVER_LOG(Message, ENSEMBL_PROTOCOL_STR, server->host, 
                 "Opening connection for '%s'", server->db_name) ;

  server->dba = DBAdaptor_new(server->host, server->user, server->passwd, server->db_name, server->port, NULL);

  if (server->dba && server->dba->dbc)
    {
      server->slice_adaptor = DBAdaptor_getSliceAdaptor(server->dba);
      server->seq_adaptor = DBAdaptor_getSequenceAdaptor(server->dba);

      server->slice = SliceAdaptor_fetchByRegion(server->slice_adaptor, "chromosome", 
                                                 server->sequence, server->zmap_start, server->zmap_end, 
                                                 STRAND_UNDEF, NULL, 0);

      if (server->slice_adaptor && server->seq_adaptor && server->slice)
        {
          result = ZMAP_SERVERRESPONSE_OK ;
        }
      else
        {
          setErrMsg(server, g_strdup_printf("Failed to get slice for %s (%s %d,%d)", 
                                            server->db_name, server->sequence, server->zmap_start, server->zmap_end)) ;
          ZMAPSERVER_LOG(Warning, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
        }
    }
  else
    {
      setErrMsg(server, g_strdup_printf("Failed to create database connection for %s", server->db_name)) ;
      ZMAPSERVER_LOG(Warning, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
    }

  g_mutex_unlock(server->mutex) ;

  return result ;
}

static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

  if (server && server->dba)
    {
      g_mutex_lock(server->mutex) ;
      char *assembly_type = DBAdaptor_getAssemblyType(server->dba) ;
      g_mutex_unlock(server->mutex) ;

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
          ZMAPSERVER_LOG(Warning, ENSEMBL_PROTOCOL_STR, server->host, "%s", server->last_err_msg) ;
        }
    }

  return result ;
}


static void vectorGetSimpleFeatureDetails(Vector *features, EnsemblServer server)
{
  gboolean failed = FALSE ;
  int i = 0 ;

  for (i = 0; i < Vector_getNumElement(features) && !failed; ++i) 
    {
      SimpleFeature *sf = Vector_getElementAt(features,i);
      long start = SimpleFeature_getStart(sf);
      long end   = SimpleFeature_getEnd(sf);

      //printf("slice start = %d end = %d\n",start,end);
      SimpleFeature *rsf = (SimpleFeature*)SeqFeature_transform((SeqFeature*)sf,"contig",NULL,NULL);

      if (rsf)
        {
          Analysis *analysis = SimpleFeature_getAnalysis(rsf) ;

          printf("Label %s, Start %ld, End %ld, Score %f, Phase %hhd, EndPhase %hhd, Strand %hhd, Length %ld, Source %s, Feature %s, Module %s\n",
                 SimpleFeature_getDisplayLabel(rsf), SimpleFeature_getStart(rsf),SimpleFeature_getEnd(rsf),
                 SimpleFeature_getScore(rsf), SimpleFeature_getPhase(rsf), SimpleFeature_getEndPhase(rsf),
                 SimpleFeature_getStrand(rsf), SimpleFeature_getLength(rsf),
                 analysis->gffSource, analysis->gffFeature, analysis->module);
        }
      else
        {
          printf("no mapped feature\n");
        }

      if (rsf) 
        {
          SimpleFeature *sf = (SimpleFeature*)SeqFeature_transform((SeqFeature*)rsf, "chromosome", NULL, server->slice);
          sf = (SimpleFeature*)SeqFeature_transfer((SeqFeature*)rsf, server->slice);
        
          if (SimpleFeature_getStart(sf) != start || SimpleFeature_getEnd(sf) != end)
            {
              printf("Remapping to slice produced different coords start %ld v %ld   end %ld v %ld\n", 
                     SimpleFeature_getStart(sf),start, SimpleFeature_getEnd(sf),end);
              failed = TRUE;
            }
        }
    }  
}

static void vectorGetDNAAlignFeatureDetails(Vector *features, EnsemblServer server)
{
  gboolean failed = FALSE ;
  int i = 0 ;

  for (i = 0; i < Vector_getNumElement(features) && !failed; ++i) 
    {
      DNAAlignFeature *sf = Vector_getElementAt(features,i);
      long start = DNAAlignFeature_getStart(sf);
      long end   = DNAAlignFeature_getEnd(sf);

      //printf("slice start = %d end = %d\n",start,end);
      DNAAlignFeature *rsf = (DNAAlignFeature*)SeqFeature_transform((SeqFeature*)sf,"contig",NULL,NULL);

      if (rsf)
        {
          Analysis *analysis = DNAAlignFeature_getAnalysis(rsf) ;

          printf("Hit name %s, Start %ld, End %ld, HitStart %d, HitEnd %d, Score %f, Cigar %s, Strand %hhd, Length %ld, Source %s, Feature %s, Module %s\n",
                 DNAAlignFeature_getHitSeqName(rsf), DNAAlignFeature_getStart(rsf),DNAAlignFeature_getEnd(rsf),
                 DNAAlignFeature_getHitStart(rsf), DNAAlignFeature_getHitEnd(rsf),
                 DNAAlignFeature_getScore(rsf), DNAAlignFeature_getCigarString(rsf),
                 DNAAlignFeature_getStrand(rsf), DNAAlignFeature_getLength(rsf),
                 analysis->gffSource, analysis->gffFeature, analysis->module);
        }
      else
        {
          printf("no mapped feature\n");
        }

      if (rsf) 
        {
          DNAAlignFeature *sf = (DNAAlignFeature*)SeqFeature_transform((SeqFeature*)rsf, "chromosome", NULL, server->slice);
          sf = (DNAAlignFeature*)SeqFeature_transfer((SeqFeature*)rsf, server->slice);
        
          if (DNAAlignFeature_getStart(sf) != start || DNAAlignFeature_getEnd(sf) != end)
            {
              printf("Remapping to slice produced different coords start %ld v %ld   end %ld v %ld\n", 
                     DNAAlignFeature_getStart(sf),start, DNAAlignFeature_getEnd(sf),end);
              failed = TRUE;
            }
        }
    }  
}

static void vectorGetDNAPepAlignFeatureDetails(Vector *features, EnsemblServer server)
{
  gboolean failed = FALSE ;
  int i = 0 ;

  for (i = 0; i < Vector_getNumElement(features) && !failed; ++i) 
    {
      DNAPepAlignFeature *sf = Vector_getElementAt(features,i);
      long start = DNAPepAlignFeature_getStart(sf);
      long end   = DNAPepAlignFeature_getEnd(sf);

      //printf("slice start = %d end = %d\n",start,end);
      DNAPepAlignFeature *rsf = (DNAPepAlignFeature*)SeqFeature_transform((SeqFeature*)sf,"contig",NULL,NULL);

      if (rsf)
        {
          Analysis *analysis = DNAPepAlignFeature_getAnalysis(rsf) ;

          printf("Start %ld, End %ld, Score %f, Strand %hhd, Length %ld, Source %s, Feature %s, Module %s\n",
                 DNAPepAlignFeature_getStart(rsf),DNAPepAlignFeature_getEnd(rsf),
                 DNAPepAlignFeature_getScore(rsf), 
                 DNAPepAlignFeature_getStrand(rsf), DNAPepAlignFeature_getLength(rsf),
                 analysis->gffSource, analysis->gffFeature, analysis->module);
        }
      else
        {
          printf("no mapped feature\n");
        }

      if (rsf) 
        {
          DNAPepAlignFeature *sf = (DNAPepAlignFeature*)SeqFeature_transform((SeqFeature*)rsf, "chromosome", NULL, server->slice);
          sf = (DNAPepAlignFeature*)SeqFeature_transfer((SeqFeature*)rsf, server->slice);
        
          if (DNAPepAlignFeature_getStart(sf) != start || DNAPepAlignFeature_getEnd(sf) != end)
            {
              printf("Remapping to slice produced different coords start %ld v %ld   end %ld v %ld\n", 
                     DNAPepAlignFeature_getStart(sf),start, DNAPepAlignFeature_getEnd(sf),end);
              failed = TRUE;
            }
        }
    }  
}

static void vectorGetRepeatFeatureDetails(Vector *features, EnsemblServer server)
{
  gboolean failed = FALSE ;
  int i = 0 ;

  for (i = 0; i < Vector_getNumElement(features) && !failed; ++i) 
    {
      RepeatFeature *sf = Vector_getElementAt(features,i);
      long start = RepeatFeature_getStart(sf);
      long end   = RepeatFeature_getEnd(sf);

      //printf("slice start = %d end = %d\n",start,end);
      RepeatFeature *rsf = (RepeatFeature*)SeqFeature_transform((SeqFeature*)sf,"contig",NULL,NULL);

      if (rsf)
        {
          Analysis *analysis = RepeatFeature_getAnalysis(rsf) ;

          printf("Start %ld, End %ld, Score %f, Phase %hhd, EndPhase %hhd, Strand %hhd, Source %s, Feature %s, Module %s\n",
                 RepeatFeature_getStart(rsf),RepeatFeature_getEnd(rsf),
                 RepeatFeature_getScore(rsf), RepeatFeature_getPhase(rsf), RepeatFeature_getEndPhase(rsf),
                 RepeatFeature_getStrand(rsf), 
                 analysis->gffSource, analysis->gffFeature, analysis->module);
        }
      else
        {
          printf("no mapped feature\n");
        }

      if (rsf) 
        {
          RepeatFeature *sf = (RepeatFeature*)SeqFeature_transform((SeqFeature*)rsf, "chromosome", NULL, server->slice);
          sf = (RepeatFeature*)SeqFeature_transfer((SeqFeature*)rsf, server->slice);
        
          if (RepeatFeature_getStart(sf) != start || RepeatFeature_getEnd(sf) != end)
            {
              printf("Remapping to slice produced different coords start %ld v %ld   end %ld v %ld\n", 
                     RepeatFeature_getStart(sf),start, RepeatFeature_getEnd(sf),end);
              failed = TRUE;
            }
        }
    }  
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
                                                 GList *sources,
                                                 GList **required_styles_out,
                                                 GHashTable **featureset_2_stylelist_inout,
                                                 GHashTable **featureset_2_column_inout,
                                                 GHashTable **source_2_sourcedata_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

  g_mutex_lock(server->mutex) ;

  SimpleFeatureAdaptor *simple_feature_adaptor = DBAdaptor_getSimpleFeatureAdaptor(server->dba) ;
  DNAAlignFeatureAdaptor *dna_feature_adaptor = DBAdaptor_getDNAAlignFeatureAdaptor(server->dba) ;
  ProteinAlignFeatureAdaptor *pep_feature_adaptor = DBAdaptor_getProteinAlignFeatureAdaptor(server->dba) ;
  RepeatFeatureAdaptor *repeat_feature_adaptor = DBAdaptor_getRepeatFeatureAdaptor(server->dba) ;

  Vector *simple_features =  Slice_getAllSimpleFeatures(server->slice, NULL, NULL, NULL);
  Vector *dna_features =  Slice_getAllDNAAlignFeatures(server->slice, NULL, NULL, NULL, NULL);
  Vector *pep_features =  Slice_getAllProteinAlignFeatures(server->slice, NULL, NULL, NULL, NULL);
  Vector *repeat_features =  Slice_getAllRepeatFeatures(server->slice, NULL, NULL, NULL);

  //vectorGetSimpleFeatureDetails(simple_features, server);
  vectorGetDNAAlignFeatureDetails(dna_features, server);
  //vectorGetDNAPepAlignFeatureDetails(pep_features, server);
  //vectorGetRepeatFeatureDetails(repeat_features, server);

  g_mutex_unlock(server->mutex) ;

  result = ZMAP_SERVERRESPONSE_OK ;
  return result ;
}

static ZMapServerResponseType getStyles(void *server_in, GHashTable **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  EnsemblServer server = (EnsemblServer)server_in ;

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
  EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}


/* Get all features on a sequence.
 * we assume we called pipeGetHeader() already and also pipeGetSequence()
 * so we are at the start of the BODY part of the stream. BUT note that
 * there may be no more lines in the file, we handle that as this point
 * as it's only here that the caller has asked for the features. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles,
                                          ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in,
                                                 GHashTable *styles, ZMapFeatureContext feature_context_out)
{
  EnsemblServer server = (EnsemblServer)server_in ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      //g_hash_table_foreach(feature_context_out->alignments, eachAlignmentSequence, (gpointer)server) ;

    }

  return server->result ;
}


/* Return the last error message. */
static char *lastErrorMsg(void *server_in)
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
  EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

/* Try to close the connection. */
static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  EnsemblServer server = (EnsemblServer)server_in ;

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  EnsemblServer server = (EnsemblServer)server_in ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  if (server->sequence)
    g_free(server->sequence) ;

  if (server->config_file)
    g_free(server->config_file) ;

  if (server->host)
    g_free(server->host) ;

  if (server->passwd)
    g_free(server->passwd) ;

  if (server->mutex)
    g_mutex_free(server->mutex) ;

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

  g_mutex_lock(server->mutex) ;

  /* We need to loop round finding each sequence and then fetching its dna... */
  next_seq = sequences_inout ;
  while (next_seq)
    {
      ZMapSequence sequence = (ZMapSequence)(next_seq->data) ;

      /* chromosome, clone, contig or supercontig */
      char *seq_name = g_strdup(g_quark_to_string(sequence->name)) ;

      Slice *slice = SliceAdaptor_fetchByRegion(server->slice_adaptor, "chromosome", 
                                                seq_name, POS_UNDEF, POS_UNDEF, STRAND_UNDEF, NULL, 0);
      
      char *seq = SequenceAdaptor_fetchBySliceStartEndStrand(server->seq_adaptor, slice, 1, POS_UNDEF, 1);

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

      g_free(seq_name) ;
      next_seq = g_list_next(next_seq) ;
    }

  g_mutex_unlock(server->mutex) ;

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
