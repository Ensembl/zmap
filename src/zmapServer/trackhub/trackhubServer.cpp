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
static ZMapServerResponseType getFeatures(void *server_in, ZMapStyleTree &styles,
                                          ZMapFeatureContext feature_context) ;
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
static ZMapServerResponseType getContextSequence(void *server_in,
                                                 char *sequence_name, int start, int end,
                                                 int *dna_length_out, char **dna_sequence_out) ;
static const char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_in, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_in, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static void setErrMsg(TrackhubServer server, char *new_msg) ;

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
  server = new TrackhubServerStruct ;
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
      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->trackdb_id, "%s", server->last_err_msg) ;
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

  if (server->registry.ping())
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      setErrMsg(server, g_strdup("Cannot open connection: failed to contact Track Hub Registry")) ;
      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->trackdb_id, "%s", server->last_err_msg) ;
    }

  return result ;
}

static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  TrackhubServer server = (TrackhubServer)server_in ;

  if (server)
    {
      string version = server->registry.version() ;

      ZMAPSERVER_LOG(Message, TRACKHUB_PROTOCOL_STR, server->trackdb_id,
                     "Track Hub Registry version: %s", version.c_str()) ;

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





/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(TrackhubServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}


/* Get all features on a sequence. */
static ZMapServerResponseType getFeatures(void *server_in, ZMapStyleTree &styles,
                                          ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  //TrackhubServer server = (TrackhubServer)server_in ;

  return result ;

}

static ZMapServerResponseType getContextSequence(void *server_in,
                                                 char *sequence_name, int start, int end,
                                                 int *dna_length_out, char **dna_sequence_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  //TrackhubServer server = (TrackhubServer)server_in ;

  return result ;

}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  TrackhubServer server = (TrackhubServer)server_in ;

  delete server ;

  return result ;

}
