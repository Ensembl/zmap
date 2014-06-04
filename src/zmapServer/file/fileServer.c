/*  File: fileServer.c
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * originated by
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapServerProtocol.h>

#include <fileServer_P.h>


typedef struct GetFeaturesDataStruct_
{
  FileServer server ;

  ZMapServerResponseType result ;

  char *err_msg ;

} GetFeaturesDataStruct, *GetFeaturesData ;

static const char * PROTOCOL_NAME = "FileServer" ;

/*
 * Interface functions.
 */
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
 GHashTable **featureset_2_column_out,
 GHashTable **source_2_sourcedata_out) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server,  ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles,
  ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in,
 GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;







/*
 * Other internal functions.
 */
static ZMapServerResponseType fileGetHeader(FileServer server);
static ZMapServerResponseType fileGetSequence(FileServer server);

static void getConfiguration(FileServer server) ;
static gboolean getServerInfo(FileServer server, ZMapServerReqGetServerInfo info) ;

static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end) ;
static void eachAlignmentGetFeatures(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data) ;

static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data) ;

static void setErrorMsgGError(FileServer server, GError **gff_file_err_inout) ;
static void setErrMsg(FileServer server, char *new_msg) ;


/*
 *
 */
void fileGetServerFuncs(ZMapServerFuncs file_funcs)
{
  file_funcs->global_init = globalInit ;
  file_funcs->create = createConnection ;
  file_funcs->open = openConnection ;
  file_funcs->get_info = getInfo ;
  file_funcs->feature_set_names = getFeatureSetNames ;
  file_funcs->get_styles = getStyles ;
  file_funcs->have_modes = haveModes ;
  file_funcs->get_sequence = getSequences ;
  file_funcs->set_context = setContext ;
  file_funcs->get_features = getFeatures ;
  file_funcs->get_context_sequences = getContextSequence ;
  file_funcs->errmsg = lastErrorMsg ;
  file_funcs->get_status = getStatus;
  file_funcs->get_connect_state = getConnectState ;
  file_funcs->close = closeConnection;
  file_funcs->destroy = destroyConnection ;

  return ;
}


static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  return result ;
}







/*
 *
 */
static gboolean createConnection(void **server_out,
                                 char *config_file,
                                 ZMapURL url,
                                 char *format,
                                 char *version_str,
                                 int timeout_unused)
{
  gboolean result = FALSE ;
  FileServer server = NULL ;

  server = (FileServer) g_new0(FileServerStruct, 1) ;
  zMapReturnValIfFail(server, result) ;

  server->config_file = NULL ;
  server->url = NULL ;
  server->scheme = SCHEME_INVALID ;
  server->path = NULL ;
  server->data_dir = NULL ;
  server->data_source = NULL ;
  server->result = ZMAP_SERVERRESPONSE_OK ;
  server->error = FALSE ;
  server->last_err_msg = NULL ;
  server->exit_code = 0;
  server->sequence_server = FALSE ;
  server->is_otter = FALSE ;
  server->parser = NULL ;
  server->gff_version = ZMAPGFF_VERSION_UNKNOWN ;
  server->buffer_line = NULL ;
  server->zmap_start = 0 ;
  server->zmap_end = 0 ;
  server->req_context = NULL ;
  server->sequence_map = NULL ;
  server->styles_file = NULL ;
  server->source_2_sourcedata = NULL ;
  server->featureset_2_column = NULL ;

  if ((url->scheme != SCHEME_FILE) && (!(url->path) || !(*(url->path))))
    {
      server->last_err_msg = g_strdup_printf("Connection failed because file url had wrong scheme or no path: %s.",
        url->url) ;
      ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

      result = FALSE ;
    }
  else
    {
      char *url_file_name ;

      /* Get configuration parameters. */
      server->config_file = g_strdup(config_file) ;
      getConfiguration(server) ;

      /* Get url parameters. */
      server->url = g_strdup(url->url) ;
      server->scheme = url->scheme ;

      if (server->scheme == SCHEME_FILE)
        {
          if (g_path_is_absolute(url->path))
            {
              server->path = g_strdup(url->path) ;

              result = TRUE ;
            }
          else
            {
              gchar *dir;
              char *tmp_path ;

              dir = server->data_dir ;

              if (dir && *dir)
                {
                  tmp_path = g_strdup_printf("%s/%s", dir, url->path) ;/* NB '//' works as '/' */

                  server->path = zMapGetPath(tmp_path) ;

                  g_free(tmp_path) ;

                  result = TRUE ;
                }
              else
                {
                  server->last_err_msg = g_strdup_printf("Cannot find path to relative file specified by url"
                               " because no data directory was specified in config file, url was: \"%s\"", url->url) ;
                  ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

                  result = FALSE ;
                }
            }
        }
    }


  /* Always return a server struct as it contains error message stuff. */
  *server_out = (void *)server ;

  return result ;
}




static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;
  GError *error = NULL ;
  gboolean status = FALSE ;

  /*
   * Test for errors.
   */
  zMapReturnValIfFail(server && (server->scheme == SCHEME_FILE) && req_open , result ) ;

  /*
   * If the server has a file already we set an
   * error and return, doing nothing else.
   */
  if ( server->data_source != NULL )
    {
      char * err_msg = g_strdup("Error in fileServer::openConnection(); server->data_source already set") ;
      error = g_error_new(g_quark_from_string(__FILE__) , 0, "%s", err_msg)  ;
      if (error)
        {
          error->message = err_msg ;
          setErrorMsgGError(server, &error) ;
        }
      zMapLogWarning("%s", err_msg) ;
      server->result = result ;
      return result ;
    }

  /*
   * Get some data from the request.
   */
  server->zmap_start = req_open->zmap_start;
  server->zmap_end = req_open->zmap_end;
  server->sequence_map = req_open->sequence_map;

  /*
   * Create data source object (file or GIOChannel)
   */
  server->data_source = zMapDataSourceCreate(server->path ) ;
  if (server->data_source != NULL )
    status = TRUE ;

  if (!status)
    {
      /* If it was a file error or failure to exec then set up message. */
      if (error)
        setErrorMsgGError(server, &error) ;
      result = ZMAP_SERVERRESPONSE_SERVERDIED ;
      server->result = result ;
      return result ;
    }

  /*
   * Create the Pipe object and attach to the server.
   * Then create the parser.
   */
  server->sequence_server = req_open->sequence_server ;

  /* Get the GFF version */
  int parser_version = zMapGFFGetVersion(server->parser) ;
  if (!parser_version)
    {
      int gff_version = ZMAPGFF_VERSION_UNKNOWN ;
      zMapDataSourceGetGFFVersion(server->data_source, &gff_version ) ;
      server->gff_version = gff_version ;
    }
  else
    server->gff_version = parser_version ;

  if (!server->parser)
    {
      server->parser = zMapGFFCreateParser(server->gff_version,
                                           server->sequence_map->sequence,
                                           server->zmap_start,
                                           server->zmap_end) ;
    }

  /*
   * Set size for a buffer line.
   */
  if (!server->buffer_line)
    {
      server->buffer_line = g_string_sized_new(ZMAPFILESERVER_READBUFFER_SIZE) ;
    }

  /*
   * Read first line from data source.
   */
  zMapDataSourceReadLine(server->data_source, server->buffer_line) ;
  result = fileGetHeader(server) ;
  if (result == ZMAP_SERVERRESPONSE_OK && server->gff_version == ZMAPGFF_VERSION_2)
    fileGetSequence(server) ;

  server->result = result ;

  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  if (getServerInfo(server, info))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;

      ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path,
                     "Could not get server info because: %s", server->last_err_msg) ;
    }

  return result ;
}


/* We could parse out all the "source" fields from the gff stream but I don't have time
 * to do this now. So we just return "unsupported", so if this function is called it
 * will alert the caller that something has gone wrong.
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
  FileServer server = (FileServer)server_in ;

  /* THERE'S SOMETHING WRONG HERE IF WE NEED TO GET THESE HERE..... */
  // these are needed by the GFF parser
  server->source_2_sourcedata = *source_2_sourcedata_inout;
  server->featureset_2_column = *featureset_2_column_inout;

  result = ZMAP_SERVERRESPONSE_OK ;


  return result ;
}



/* We cannot parse the styles from a gff stream, gff simply doesn't have display styles so we
 * just return "unsupported", so if this function is called it will alert the caller that
 * something has gone wrong.
 *
 *  */
static ZMapServerResponseType getStyles(void *server_in, GHashTable **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  // can take this warning out as zmapView should now read the styles file globally
  setErrMsg(server, "Reading styles from a GFF stream is not supported.") ;
  ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}


/* fileServers use files from a ZMap style file and therefore include modes
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  *have_mode = TRUE ;

  return result ;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
  FileServer server = (FileServer)server_in ;

  setErrMsg(server, "Reading sequences from a GFF stream is not supported.") ;
  ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

  /* errr........why is there code in this file to read sequences then ??? */
  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}




/* We don't check anything here, we could look in the data to check that it matches the context
 * I guess which is essentially what we do for the acedb server.
 * NB: as we process a stream we cannot search backwards or forwards
 */
static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;

  server->req_context = feature_context ;


  /* Like the acedb server we should be getting the mapping from the gff data here....
   * then we could set up the block/etc stuff...this is where add mapping should be called from... */



  return result ;
}


/* Get all features on a sequence.
 * we assume we called fileGetHeader() already and also fileGetSequence()
 * so we are at the start of the BODY part of the stream. BUT note that
 * there may be no more lines in the file, we handle that as this point
 * as it's only here that the caller has asked for the features. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles,
  ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;
  GetFeaturesDataStruct get_features_data = {NULL} ;
  int num_features ;

  /* Check that there is any more to parse.... */
  if (server->buffer_line->len == 0)                           /* GString len is always >= 0 */
    {
      setErrMsg(server, "No features found.") ;

      ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

      result = server->result = ZMAP_SERVERRESPONSE_NODATA ;
    }
  else
    {
      get_features_data.server = server ;

      zMapGFFParseSetSourceHash(server->parser, server->featureset_2_column, server->source_2_sourcedata) ;

      zMapGFFParserInitForFeatures(server->parser, styles, FALSE) ;

      zMapGFFSetDefaultToBasic(server->parser, TRUE);

      server->result = ZMAP_SERVERRESPONSE_OK ;

      /* Get block to parent mapping.  */
      addMapping(feature_context, server->zmap_start, server->zmap_end) ;


      /* Fetch all the features for all the blocks in all the aligns. */
      g_hash_table_foreach(feature_context->alignments, eachAlignmentGetFeatures, &get_features_data) ;


      if ((num_features = zMapGFFParserGetNumFeatures(server->parser)) == 0)
        {
          setErrMsg(server, "No features found.") ;

          ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;

          result = server->result = ZMAP_SERVERRESPONSE_NODATA ;
        }
      else
        {
          /* get the list of source featuresets */
          feature_context->src_feature_set_names = zMapGFFGetFeaturesets(server->parser);

          result = server->result ;
        }
    }

  return result ;
}




/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in,
                                                 GHashTable *styles,
                                                 ZMapFeatureContext feature_context_out)
{
  FileServer server = (FileServer)server_in ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      g_hash_table_foreach(feature_context_out->alignments, eachAlignmentSequence, (gpointer)server) ;

    }

  return server->result ;
}


/* Return the last error message. */
static char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  FileServer server = (FileServer)server_in ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;

  return err_msg ;
}


static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code)
{
  FileServer server = (FileServer)server_conn ;

  *exit_code = server->exit_code ;

  //can't do this or else it won't be read
  //            if (server->exit_code)
  //                  return ZMAP_SERVERRESPONSE_SERVERDIED;

  return ZMAP_SERVERRESPONSE_OK ;
}


/* Is the server connected ? */
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_conn ;

  if (zMapDataSourceIsOpen(server->data_source))
    *connect_state = ZMAP_SERVERCONNECT_STATE_CONNECTED ;

  return result ;
}


static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;
  GError *gff_file_err = NULL ;

  if (server->parser)
    zMapGFFDestroyParser(server->parser) ;

  if (server->buffer_line)
    g_string_free(server->buffer_line, TRUE) ;

  if (server->data_source)
    zMapDataSourceDestroy(&(server->data_source)) ;

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;

  if (server->url)
    g_free(server->url) ;

  if (server->path)
    g_free(server->path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  /* Clear up. -> in destroyConnection() */
  /* crashes...
     if (server->parser)
     zMapGFFDestroyParser(server->parser) ;
     if (server->buffer_line)
     g_string_free(server->buffer_line, TRUE) ;
  */

  g_free(server) ;

  return result ;
}





/*
 *                            Internal routines
 */


/* Read ZMap application defaults. */
static void getConfiguration(FileServer server)
{
  ZMapConfigIniContext context;

  if ((context = zMapConfigIniContextProvide(server->config_file)))
    {
      char *tmp_string  = NULL;

      /* default directory to use */
      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
       ZMAPSTANZA_APP_DATA, &tmp_string))
        {
          server->data_dir = tmp_string;
        }
      else
        {
          server->data_dir = g_get_current_dir();
        }

      if (zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                                             ZMAPSTANZA_APP_CSVER, &tmp_string))
        {
          if (!g_ascii_strcasecmp(tmp_string,"Otter"))
            server->is_otter = TRUE;
        }

      zMapConfigIniContextDestroy(context);
    }

  return ;
}


/* read the header data and exit when we get DNA or features or anything else. */
static ZMapServerResponseType fileGetHeader(FileServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  GIOStatus status ;
  GError *error = NULL ;
  gboolean empty_file = TRUE ;
  gboolean done_header = FALSE ;
  ZMapGFFHeaderState header_state = GFF_HEADER_NONE ;    /* got all the ones we need ? */

  /* reset done flag for seq else skip the data */
  if (server->sequence_server)
    zMapGFFParserSetSequenceFlag(server->parser);


  /* Read the header, needed for feature coord range. */
  do
    {

      empty_file = FALSE ;
      result = ZMAP_SERVERRESPONSE_OK;

      if (zMapGFFParseHeader(server->parser, server->buffer_line->str, &done_header, &header_state))
        {
          if (done_header)
            break ;
        }
      else
        {
          if (!done_header)
            {
              error = zMapGFFGetError(server->parser) ;

              if (!error)
                {
                  char *err_msg ;

                  err_msg = g_strdup_printf("zMapGFFParseHeader() failed with no GError for line %d: %s",
                  zMapGFFGetLineNumber(server->parser), server->buffer_line->str) ;
                  setErrMsg(server, err_msg) ;
                  g_free(err_msg) ;

                  ZMAPSERVER_LOG(Critical, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;
                }
             else
                {
                  /* If the error was serious we stop processing and return the error,
                   * otherwise we just log the error. */
                  if (zMapGFFTerminated(server->parser))
                    {
                      result = ZMAP_SERVERRESPONSE_REQFAIL ;

                      setErrMsg(server, error->message) ;
                    }
                  else
                    {
                      char *err_msg ;

                      err_msg = g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
                      zMapGFFGetLineNumber(server->parser),
                      server->buffer_line->str) ;
                      setErrMsg(server, err_msg) ;
                      g_free(err_msg) ;

                      ZMAPSERVER_LOG(Critical, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;
                    }
                }

              result = ZMAP_SERVERRESPONSE_REQFAIL ;
            }

          break ;
        }

      status = zMapDataSourceReadLine(server->data_source, server->buffer_line) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF ;

    } while ( status == G_IO_STATUS_NORMAL ) ;


  /* Sometimes the file contains nothing or only the gff header and no data, I don't know the reason for this
   * but in this case there's no point in going further. */
  if (header_state == GFF_HEADER_ERROR || empty_file)
    {
      char *err_msg ;

      if (status == G_IO_STATUS_EOF)
        {
          if (empty_file)
            err_msg = g_strdup_printf("%s", "Empty file.") ;
          else
            err_msg = g_strdup_printf("EOF reached while trying to read header, at line %d",
                                      zMapGFFGetLineNumber(server->parser)) ;

          result = ZMAP_SERVERRESPONSE_NODATA ;
        }
      else
        {
          err_msg = g_strdup_printf("Error in GFF header, at line %d",
                                    zMapGFFGetLineNumber(server->parser)) ;

          result = ZMAP_SERVERRESPONSE_REQFAIL ;
        }

      setErrMsg(server, err_msg) ;
      g_free(err_msg) ;

      ZMAPSERVER_LOG(Critical, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;
    }

  return result ;
}




// read any DNA data at the head of the stream and quit after error or ##end-dna
static ZMapServerResponseType fileGetSequence(FileServer server)
{
  GIOStatus status ;
  GError *error = NULL ;
  gboolean sequence_finished = FALSE;

  // read the sequence if it's there
  server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

  /* we have already read the first non header line in fileGetheader */
  do
    {

      if (!zMapGFFParseSequence(server->parser, server->buffer_line->str, &sequence_finished) || sequence_finished)
        break;

      status = zMapDataSourceReadLine(server->data_source, server->buffer_line) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF ;

    } while ( status == G_IO_STATUS_NORMAL );

  error = zMapGFFGetError(server->parser) ;

  if (error)
    {
      /* If the error was serious we stop processing and return the error,
       * otherwise we just log the error. */
      if (zMapGFFTerminated(server->parser))
        {
        server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
        setErrMsg(server, error->message) ;
        }
     else
       {
         char *err_msg ;

          err_msg = g_strdup_printf("zMapGFFParseSequence() failed for line %d: %s - %s",
          zMapGFFGetLineNumber(server->parser),
          server->buffer_line->str, error->message) ;
          setErrMsg(server, err_msg) ;
          g_free(err_msg) ;

          ZMAPSERVER_LOG(Critical, PROTOCOL_NAME, server->path, "%s", server->last_err_msg) ;
        }
      server->result = ZMAP_SERVERRESPONSE_REQFAIL;
    }
  else if (!sequence_finished)
    {
      server->result = ZMAP_SERVERRESPONSE_UNSUPPORTED;
    }

  return(server->result);
}



/* Process all the alignments in a context. */
static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  FileServer server = (FileServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockSequence, (gpointer)server) ;

  return ;
}


/* "GFF doesn't understand multiple blocks" therefore there can only be one
 * DNA sequence
 * so we expect one alignment and one block
 */
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  FileServer server = (FileServer) user_data ;

  if (server->result != ZMAP_SERVERRESPONSE_OK)
    return ;

  ZMapSequence sequence;
  GQuark sequence_name = g_quark_from_string(server->sequence_map->sequence) ;
  if (!(sequence = zMapGFFGetSequence(server->parser, sequence_name)))
    {
      GError *error;
      char *estr;

      error = zMapGFFGetError(server->parser);

      if (error)
        estr = error->message;
      else
        estr = "No error reported";

      setErrMsg(server, estr) ;

      ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", server->last_err_msg);
    }
  else
    {
      /* the servers need styles to add DNA and 3FT
      * they used to create temp style and then destroy these but that's not very good
      * they don't have styles info directly but this is stored in the parser
      * during the protocol steps, so i wrote a GFF function to supply that info
      * Now that features have style ref'd indirectly via the featureset we can't use temp data
      */
      ZMapFeatureContext context;
      ZMapFeatureSet feature_set;
      GHashTable *styles = zMapGFFParserGetStyles(server->parser);

      if (zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
        {
          ZMapFeatureTypeStyle dna_style = NULL;
          ZMapFeature feature;

          if (styles && (dna_style = g_hash_table_lookup(styles, GUINT_TO_POINTER(feature_set->unique_id))))
            feature = zMapFeatureDNACreateFeature(feature_block, dna_style, sequence->sequence, sequence->length);
        }


      // this is insane: asking a file server for 3FT, however some old code might expect it
      /* gb10: ZMap can now be run standalone on a GFF file, so everything, including the DNA
      * for the 3FT, has to come from that file... */
      context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block, ZMAPFEATURE_STRUCT_CONTEXT) ;

      /* I'm going to create the three frame translation up front! */
      if (zMap_g_list_find_quark(context->req_feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
        {
          ZMapFeatureSet translation_fs = NULL;

          if (zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set))
          {
            translation_fs = feature_set;
            ZMapFeatureTypeStyle frame_style = NULL;

            if(styles && (frame_style = zMapFindStyle(styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME))))
              zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);
          }

          if (zMapFeatureORFCreateSet(feature_block, &feature_set))
          {
            ZMapFeatureTypeStyle orf_style = NULL;

            if (styles && (orf_style = zMapFindStyle(styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_ORF_NAME))))
              zMapFeatureORFSetCreateFeatures(feature_set, orf_style, translation_fs);
          }
        }

      /* I'm going to create the show translation up front! */
      if (zMap_g_list_find_quark(context->req_feature_set_names,
                  zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME)))
        {
          if ((zMapFeatureShowTranslationCreateSet(feature_block, &feature_set)))
            {
              ZMapFeatureTypeStyle trans_style = NULL;

              if ((trans_style = zMapFindStyle(styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_SHOWTRANSLATION_NAME))))
                zMapFeatureShowTranslationSetCreateFeatures(feature_set, trans_style) ;
            }
        }

      g_free(sequence);
    }

  return ;
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



/* Get Features in all blocks within an alignment. */
static void eachAlignmentGetFeatures(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  GetFeaturesData get_features_data = (GetFeaturesData)user_data ;

  if (get_features_data->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockGetFeatures, (gpointer)get_features_data) ;

  return ;
}


/* Get features in a block */
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  GetFeaturesData get_features_data = (GetFeaturesData)user_data ;

  if (get_features_data->result != ZMAP_SERVERRESPONSE_OK )
    return ;

  FileServer server = get_features_data->server ;
  ZMapGFFParser parser = server->parser ;
  GIOStatus status ;
  gboolean free_on_destroy = FALSE ;
  GError *gff_file_err = NULL ;

  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */
  if (server->zmap_end)
    {
      zMapGFFSetFeatureClipCoords(parser,
                                  server->zmap_start,
                                  server->zmap_end) ;
      zMapGFFSetFeatureClip(parser,GFF_CLIP_ALL);
    }

  do
    {

      if (!zMapGFFParseLine(parser, server->buffer_line->str))
        {
          GError *error ;

          error = zMapGFFGetError(parser) ;

          ZMAPSERVER_LOG(Warning, PROTOCOL_NAME, server->path, "%s", error->message) ;

          /* If the error was serious we stop processing and return the error. */
          if (zMapGFFTerminated(parser))
            {
              get_features_data->result = ZMAP_SERVERRESPONSE_REQFAIL ;

              setErrMsg(server, error->message) ;

              break ;
            }

        }

      status = zMapDataSourceReadLine(server->data_source, server->buffer_line) ? G_IO_STATUS_NORMAL : G_IO_STATUS_EOF ;

    } while ( status == G_IO_STATUS_NORMAL ) ;


  /* If we reached the end of the stream then all is fine, so return features... */
  free_on_destroy = TRUE ;
  if (status == G_IO_STATUS_EOF)
    {
      if (get_features_data->result == ZMAP_SERVERRESPONSE_OK
          && zMapGFFGetFeatures(parser, feature_block))
        {
          free_on_destroy = FALSE ;    /* Make sure parser does _not_ free our data ! */
        }
      else
        {
          char *err_msg ;

          /* If features not ok this has already been reported so just report
           * if can't get features out of parser. */
          if (get_features_data->result == ZMAP_SERVERRESPONSE_OK)
            {
              err_msg = g_strdup("Cannot retrieve features from parser.") ;

              zMapLogWarning("Error in reading or getting features from stream \"%s\": ",
                             server->path, err_msg) ;

              setErrMsg(server, err_msg) ;
            }
        }
    }
  else
    {
      zMapLogWarning("Could not read GFF features from stream \"%s\" with \"%s\".",
                     server->path, &gff_file_err) ;

      setErrorMsgGError(server, &gff_file_err) ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return ;
}



static gboolean getServerInfo(FileServer server, ZMapServerReqGetServerInfo info)
{
  gboolean result = TRUE ;

  info->data_format_out = g_strdup_printf("GFF version %d", server->gff_version) ;

  info->database_path_out = g_strdup(server->path) ;
  info->request_as_columns = FALSE;

  return result ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_file_err_inout to NULL */
static void setErrorMsgGError(FileServer server, GError **gff_file_err_inout)
{
  GError *gff_file_err ;
  static const char *msg = "failed";

  gff_file_err = *gff_file_err_inout ;
  if (gff_file_err)
    msg = gff_file_err->message ;

  setErrMsg(server, msg) ;

  if (gff_file_err)
    g_error_free(gff_file_err) ;

  *gff_file_err_inout = NULL ;

  return ;
}


/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(FileServer server, char *new_msg)
{
  char *error_msg ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  error_msg = g_strdup_printf("error: \"%s\" request: \"%s\",", new_msg, server->path) ;

  server->last_err_msg = error_msg ;

  return ;
}


