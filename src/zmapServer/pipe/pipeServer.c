/*  File: pipeServer.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *      derived from fileServer.c by Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: These functions provide code to read the output of a script
 *		as though it were a server according to the interface defined
 *              for accessing servers. The aim is to allow ZMap to request
 *		arbritary data from external sources as defined in the config files.
 *
 *              NB: As for the fileServer module the data is read
 *              in a single pass and then discarded.
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>

#include <sys/types.h>  /* for waitpid() */
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapServerProtocol.h>
#include <pipeServer_P.h>


/* Flag values for pipeArg struct flags. */
typedef enum {PA_DATATYPE_INVALID, PA_DATATYPE_INT, PA_DATATYPE_STRING} PipeArgDataType ;

typedef enum {PA_FLAG_INVALID, PA_FLAG_START, PA_FLAG_END, PA_FLAG_DATASET, PA_FLAG_SEQUENCE} PipeArgFlag ;


typedef struct PipeArgStructType
{
  char *arg ;
  PipeArgDataType type ;
  PipeArgFlag flag ;
} PipeArgStruct, *PipeArg ;


/* Needs removing....why not use an expandable array ???? */
#define PIPE_MAX_ARGS   8    // extra args we add on to the query, including the program and terminating NULL


typedef struct GetFeaturesDataStructType
{
  PipeServer server ;

  ZMapServerResponseType result ;

  char *err_msg ;



} GetFeaturesDataStruct, *GetFeaturesData ;



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
					  ZMapFeatureContext feature_context_out, int *num_features_out) ;
static ZMapServerResponseType getContextSequence(void *server_in,
						 GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static gboolean pipeSpawn(PipeServer server,GError **error);
static void waitForChild(PipeServer server) ;

static ZMapServerResponseType pipeGetHeader(PipeServer server);
static ZMapServerResponseType pipeGetSequence(PipeServer server);

static void getConfiguration(PipeServer server) ;
static gboolean getServerInfo(PipeServer server, ZMapServerReqGetServerInfo info) ;

static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end) ;
static void eachAlignmentGetFeatures(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockGetFeatures(gpointer key, gpointer data, gpointer user_data) ;

static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data) ;

static void setErrorMsgGError(PipeServer server, GError **gff_pipe_err_inout) ;
static void setErrMsg(PipeServer server, char *new_msg) ;





/*
 *                   Globals
 */

PipeArgStruct otter_args[] =
  {
    { "start", PA_DATATYPE_INT, PA_FLAG_START },
    { "end", PA_DATATYPE_INT, PA_FLAG_END },
    { "dataset", PA_DATATYPE_STRING, PA_FLAG_DATASET },
    { "gff_seqname", PA_DATATYPE_STRING, PA_FLAG_SEQUENCE },
    { NULL, 0, 0 }
  };

PipeArgStruct zmap_args[] =
  {
    { "start", PA_DATATYPE_INT, PA_FLAG_START },
    { "end", PA_DATATYPE_INT, PA_FLAG_END },
    //      { "dataset", PA_DATATYPE_STRING,PA_FLAG_DATASET },	may need when mapping available
    { "gff_seqname", PA_DATATYPE_STRING, PA_FLAG_SEQUENCE },
    { NULL, 0, 0 }
  };



/*
 *              External interface routines
 *
 *    Although most of these routines are static they form the external interface to the pipe server.
 *
 */


/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact a pipe server. The functions are only visible through
 * this struct. */
void pipeGetServerFuncs(ZMapServerFuncs pipe_funcs)
{
  pipe_funcs->global_init = globalInit ;
  pipe_funcs->create = createConnection ;
  pipe_funcs->open = openConnection ;
  pipe_funcs->get_info = getInfo ;
  pipe_funcs->feature_set_names = getFeatureSetNames ;
  pipe_funcs->get_styles = getStyles ;
  pipe_funcs->have_modes = haveModes ;
  pipe_funcs->get_sequence = getSequences ;
  pipe_funcs->set_context = setContext ;
  pipe_funcs->get_features = getFeatures ;
  pipe_funcs->get_context_sequences = getContextSequence ;
  pipe_funcs->errmsg = lastErrorMsg ;
  pipe_funcs->get_status = getStatus;
  pipe_funcs->get_connect_state = getConnectState ;
  pipe_funcs->close = closeConnection;
  pipe_funcs->destroy = destroyConnection ;

  return ;
}





/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  return result ;
}



/* spawn a new process to run our script and open a pipe for read to get the output
 * global config of the default scripts directory is via '[ZMap] scripts-dir=there'
 * to get absolute path you need 4 slashes eg for pipe://<host>/<path>
 * where <host> is null and <path> begins with a /
 * For now we will assume that "host" contains the script name and then we just ignore the other
 * parameters....
 *
 */
static gboolean createConnection(void **server_out,
				 char *config_file, ZMapURL url, char *format,
                                 char *version_str, int timeout_unused)
{
  gboolean result = FALSE ;
  PipeServer server ;


  server = (PipeServer)g_new0(PipeServerStruct, 1) ;

  if ((url->scheme != SCHEME_FILE || url->scheme != SCHEME_PIPE) && (!(url->path) || !(*(url->path))))
    {
      server->last_err_msg = g_strdup_printf("Connection failed because pipe url had wrong scheme or no path: %s.",
					     url->url) ;
      ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
			 "%s", server->last_err_msg) ;

      result = FALSE ;
    }
  else
    {
      char *url_script_path ;

      /* Get configuration parameters. */
      server->config_file = g_strdup(config_file) ;
      getConfiguration(server) ;

      /* Get url parameters. */
      server->url = g_strdup(url->url) ;
      server->scheme = url->scheme ;
      url_script_path = url->path ;

      if (server->scheme == SCHEME_FILE)
	{
	  if (g_path_is_absolute(url_script_path))
	    {
	      server->script_path = g_strdup(url_script_path) ;

	      result = TRUE ;
	    }
	  else
	    {
	      gchar *dir;
	      char *tmp_path ;

	      dir = server->data_dir ;

	      if (dir && *dir)
		{
		  tmp_path = g_strdup_printf("%s/%s", dir, url_script_path) ;		/* NB '//' works as '/' */

		  server->script_path = zMapGetPath(tmp_path) ;

		  g_free(tmp_path) ;

		  result = TRUE ;
		}
	      else
		{
		  server->last_err_msg = g_strdup_printf("Cannot find path to relative file specified by url"
							 " because no data directory was specified in config file,"
							 " url was: \"%s\"",
							 url->url) ;
		  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
				     "%s", server->last_err_msg) ;

		  result = FALSE ;
		}
	    }
	}
      else						    /* SCHEME_PIPE */
	{
	  if (g_path_is_absolute(url_script_path))
	    {
	      server->script_path = g_strdup(url_script_path) ;

	      result = TRUE ;
	    }
	  else
	    {
	      /* Look for an _executable_ script on our path. */
	      if ((server->script_path = g_find_program_in_path(url_script_path)))
		{
		  result = TRUE ;
		}
	      else
		{
		  server->last_err_msg = g_strdup_printf("Cannot find executable script %s in PATH",
							 url_script_path) ;
		  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
				     "%s", server->last_err_msg) ;

		  result = FALSE ;
		}
	    }
	}

      if (result)
	{
	  if (url->query)
	    server->script_args = g_strdup_printf("%s",url->query) ;
	  else
	    server->script_args = g_strdup("") ;

	  if (server->scheme == SCHEME_FILE)
	    server->protocol = FILE_PROTOCOL_STR ;
	  else
	    server->protocol = PIPE_PROTOCOL_STR ;
	}
    }


  /* Always return a server struct as it contains error message stuff. */
  *server_out = (void *)server ;

  return result ;
}




static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;
  GError *gff_pipe_err = NULL ;

  if (server->gff_pipe)
    {
      char *err_msg ;

      err_msg = g_strdup_printf("Feature script \"%s\" already active.", server->script_path) ;

      gff_pipe_err->message = err_msg ;

      setErrorMsgGError(server, &gff_pipe_err) ;

      zMapLogWarning("%s", err_msg) ;

      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  else
    {
      gboolean status = FALSE ;

      server->zmap_start = req_open->zmap_start;
      server->zmap_end = req_open->zmap_end;

      server->sequence_map = req_open->sequence_map;

      if (server->scheme == SCHEME_FILE)   // could spawn /bin/cat but there is no need
	{
	  if ((server->gff_pipe = g_io_channel_new_file(server->script_path, "r", &gff_pipe_err)))
            status = TRUE ;
	}
      else
	{
	  status = pipeSpawn(server, &gff_pipe_err) ;
	}

      if (!status)
	{
	  /* If it was a file error or failure to exec then set up message. */
	  if (gff_pipe_err)
	    setErrorMsgGError(server, &gff_pipe_err) ;

	  /* we don't know if it died or not
	   * but will find out via the getStatus request
	   * we need DIED status to make this happen
	   */
	  result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	}
      else
	{
	  server->sequence_server = req_open->sequence_server ; /* if not then drop any DNA data */

	  /* Hack...really this function should return version 2 */
	  if (!zMapGFFGetVersionFromGIO(server->gff_pipe, &(server->gff_version)))
	    {
	      server->gff_version = 2 ;
	    }
	  
	  server->parser = zMapGFFCreateParser(server->gff_version,
					       server->sequence_map->sequence, server->zmap_start, server->zmap_end) ;

	  server->gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be > 2k chars. */

	  /* always read it: have to skip over if not wanted
	   * need a flag here to say if this is a sequence server
	   * ignore error response as we want to report open is OK */
          if ((result = pipeGetHeader(server)) == ZMAP_SERVERRESPONSE_OK)
            {
              pipeGetSequence(server);
            }
	}
    }

  server->result = result ;

  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerReqGetServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  if (getServerInfo(server, info))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;

      ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->script_args,
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
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;


  /* THERE'S SOMETHING WRONG HERE IF WE NEED TO GET THESE HERE..... */
  // these are needed by the GFF parser
  server->source_2_sourcedata = *source_2_sourcedata_inout;
  server->featureset_2_column = *featureset_2_column_inout;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  setErrMsg(server, g_strdup("Feature Sets cannot be read from GFF stream.")) ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
		     "%s", server->last_err_msg) ;
  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;

  // can take this warning out as zmapView should now read the styles file globally
  setErrMsg(server, "Reading styles from a GFF stream is not supported.") ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
		     "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}


/* pipeServers use files from a ZMap style file and therefore include modes
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  PipeServer server = (PipeServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  *have_mode = TRUE ;

  return result ;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
  PipeServer server = (PipeServer)server_in ;

  setErrMsg(server, "Reading sequences from a GFF stream is not supported.") ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
		     "%s", server->last_err_msg) ;

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
  PipeServer server = (PipeServer)server_in ;

  server->req_context = feature_context ;


  /* Like the acedb server we should be getting the mapping from the gff data here....
   * then we could set up the block/etc stuff...this is where add mapping should be called from... */



  return result ;
}


/* Get all features on a sequence.
 * we assume we called pipeGetHeader() already and also pipeGetSequence()
 * so we are at the start of the BODY part of the stream. BUT note that
 * there may be no more lines in the file, we handle that as this point
 * as it's only here that the caller has asked for the features. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles,
					  ZMapFeatureContext feature_context, int *num_features_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;
  GetFeaturesDataStruct get_features_data = {NULL} ;
  int num_features ;

  /* Check that there is any more to parse.... */
  if (server->gff_line->len > 0)
    {
      get_features_data.server = server ;
  


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
        /* DEBUG.... */
        GList *style_names ;

        zMap_g_quark_list_print(feature_context->req_feature_set_names) ;

        style_names = zMapStylesGetNames(feature_context->styles) ;
        zMap_g_quark_list_print(style_names) ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      zMapGFFParseSetSourceHash(server->parser, server->featureset_2_column, server->source_2_sourcedata) ;

      zMapGFFParserInitForFeatures(server->parser, styles, FALSE) ;  // FALSE = create features

      zMapGFFSetDefaultToBasic(server->parser, TRUE);


      // default to OK, previous pipeGetSequence() could have set unsupported
      // if no DNA was provided

      server->result = ZMAP_SERVERRESPONSE_OK ;

      /* Get block to parent mapping.  */
      addMapping(feature_context, server->zmap_start, server->zmap_end) ;


      /* Fetch all the features for all the blocks in all the aligns. */
      g_hash_table_foreach(feature_context->alignments, eachAlignmentGetFeatures, &get_features_data) ;


      if ((num_features = zMapGFFParserGetNumFeatures(server->parser)) == 0)
        {
          setErrMsg(server, "No features found.") ;

          ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
                             "%s", server->last_err_msg) ;

          server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
        }
      else
        {
          /* get the list of source featuresets */
          feature_context->src_feature_set_names = zMapGFFGetFeaturesets(server->parser);

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
          {
            GError *error = NULL ;
            server->parser->state == ZMAPGFF_PARSE_BODY;
            zMapFeatureDumpStdOutFeatures(feature_context, &error) ;
          }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

          *num_features_out = num_features ;

          result = server->result ;


          /* WHAT HAPPENED HERE......IS THIS A MEMORY LEAK ?? */
          /* Clear up. -> in destroyConnection() */
          //  zMapGFFDestroyParser(server->parser) ;
          //  g_sring_free(server->gff_line, TRUE) ;
        }
    }

  return result ;
}




/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in,
						 GHashTable *styles, ZMapFeatureContext feature_context_out)
{
  PipeServer server = (PipeServer)server_in ;

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
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;

  return err_msg ;
}


static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code)
{
  PipeServer server = (PipeServer)server_conn ;

  waitForChild(server) ;

  *exit_code = server->exit_code ;

  //can't do this or else it won't be read
  //            if (server->exit_code)
  //                  return ZMAP_SERVERRESPONSE_SERVERDIED;

  return ZMAP_SERVERRESPONSE_OK ;
}


/* Is the pipe server connected ? */
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_conn ;

  if (server->child_pid)
    *connect_state = ZMAP_SERVERCONNECT_STATE_CONNECTED ;
  else
    *connect_state = ZMAP_SERVERCONNECT_STATE_UNCONNECTED ;

  return result ;
}


static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;
  GError *gff_pipe_err = NULL ;

  if (server->child_pid)
    g_spawn_close_pid(server->child_pid) ;

  if (server->parser)
    zMapGFFDestroyParser(server->parser) ;

  if (server->gff_line)
    g_string_free(server->gff_line, TRUE) ;

  if (server->gff_pipe)
    {
      if (g_io_channel_shutdown(server->gff_pipe, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
	{
	  zMapLogCritical("Could not close feature pipe \"%s\"", server->script_path) ;

	  setErrorMsgGError(server, &gff_pipe_err) ;

	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
      else
	{
	  /* this seems to be required to destroy the GIOChannel.... */
	  g_io_channel_unref(server->gff_pipe) ;
	  server->gff_pipe = NULL ;
	}
    }

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;

  if (server->url)
    g_free(server->url) ;

  if (server->script_path)
    g_free(server->script_path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  /* Clear up. -> in destroyConnection() */
  /* crashes...
     if (server->parser)
     zMapGFFDestroyParser(server->parser) ;
     if (server->gff_line)
     g_string_free(server->gff_line, TRUE) ;
  */

  g_free(server) ;

  return result ;
}





/*
 *                            Internal routines
 */


/* Read ZMap application defaults. */
static void getConfiguration(PipeServer server)
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



/*
 * fork and exec the script and read the output via a pipe
 * no data sent to STDIN and STDERR ignored
 * in case of errors or hangups eventually we will time out and an error popped up.
 * downside is limited to not having the data, which is what happens anyway
 */
static char *make_arg(PipeArg pipe_arg, char *prefix, PipeServer server)
{
  char *q = NULL;

  switch(pipe_arg->flag)
    {
    case PA_FLAG_START:
      if (server->zmap_start && server->zmap_end)
	q = g_strdup_printf("%s%s=%d",prefix,pipe_arg->arg,server->zmap_start);
      break;
    case PA_FLAG_END:
      if (server->zmap_start && server->zmap_end)
	q = g_strdup_printf("%s%s=%d",prefix,pipe_arg->arg,server->zmap_end);
      break;
    case PA_FLAG_DATASET:
      /* if NULL will not work but won't crash either */
      if (server->sequence_map->dataset)
	q = g_strdup_printf("%s%s=%s",prefix,pipe_arg->arg,server->sequence_map->dataset);
      break;
    case PA_FLAG_SEQUENCE:
      if (server->sequence_map->sequence)
	q = g_strdup_printf("%s%s=%s",prefix,pipe_arg->arg,server->sequence_map->sequence);
      break;
    default:
      zMapAssertNotReached() ;
      break ;
    }

  return(q);
}


static gboolean pipeSpawn(PipeServer server, GError **error)
{
  gboolean result = FALSE ;
  gchar **argv, **q_args ;
  gint pipe_fd ;
  GError *pipe_error = NULL ;
  int i ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  gint err_fd;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  PipeArg pipe_arg;
  char arg_done = 0;
  char *mm = "--";
  char *minus = mm + 2 ;				    /* this gets set as per the first arg */
  char *p;
  int n_q_args = 0;

  q_args = g_strsplit(server->script_args,"&",0) ;

  if (q_args && *q_args)
    {
      p = q_args[0];
      minus = mm+2;
      if (*p == '-')     /* optional -- allow single as well */
	{
	  p++;
	  minus--;
	}
      if (*p == '-')
	{
	  minus--;
	}
      n_q_args = g_strv_length(q_args);
    }

  argv = (gchar **)g_malloc(sizeof(gchar *) * (PIPE_MAX_ARGS + n_q_args)) ;

  argv[0] = server->script_path; // scripts can get exec'd, as long as they start w/ #!


  for (i = 1 ; q_args && q_args[i-1] ; i++)
    {
      /* if we request from the mark we have to adjust the start/end coordinates
       * these are supplied in the url and really should be configured explicitly
       * ... now they are in [ZMap].  We now work with chromosome coords.
       */
      char *q ;

      p = q_args[i-1] + strlen(minus) ;

      pipe_arg = server->is_otter ? otter_args : zmap_args ;
      for ( ; pipe_arg->type ; pipe_arg++)
	{
	  if (g_ascii_strncasecmp(p, pipe_arg->arg, strlen(pipe_arg->arg)) == 0)
            {
	      arg_done |= pipe_arg->flag ;

	      if ((q = make_arg(pipe_arg, minus, server)))
		{
		  g_free(q_args[i-1]) ;
		  q_args[i-1] = q ;

		  break ;				    /* I believe we should break out here.... */
		}

            }
	}

      argv[i] = q_args[i-1];
    }

  /* add on if not defined already */
  pipe_arg = server->is_otter ? otter_args : zmap_args;
  for(; pipe_arg->type; pipe_arg++)
    {
      if (!(arg_done & pipe_arg->flag))
	{
	  char *q;
	  q = make_arg(pipe_arg,minus,server);
	  if (q)
	    argv[i++] = q;

	}
    }
  argv[i]= NULL ;

  //#define MH17_DEBUG_ARGS
#if defined  MH17_DEBUG_ARGS
  {
    char *x = "";
    int j;

    for(j = 0;argv[j] ;j++)
      {
	x = g_strconcat(x," ",argv[j],NULL);
      }
    zMapLogWarning("pipe server args: %s (%d,%d)",x,server->zmap_start,server->zmap_end);
  }
#endif


  /* Seems that g_spawn_async_with_pipes() is not thread safe so lock round it. */
  zMapThreadForkLock();

  /* After we spawn we need to control when child exits so set G_SPAWN_DO_NOT_REAP_CHILD. */
  if ((result = g_spawn_async_with_pipes(NULL, argv, NULL,
					 G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &server->child_pid,
					 NULL, &pipe_fd, NULL,
					 &pipe_error)))
    {
      /* Set up reading of stdout pipe. */
      server->gff_pipe = g_io_channel_unix_new(pipe_fd) ;
    }
  else
    {
      /* If there was an error then return it. */
      if (error)
	*error = pipe_error ;

      /* didn't run so can't have clean exit code; */
      server->exit_code = EXIT_FAILURE ;
    }

  /* Unlock ! */
  zMapThreadForkUnlock();

  /* Clear up */
  g_free(argv) ;
  if (q_args)
    g_strfreev(q_args) ;


  return result ;
}



static void waitForChild(PipeServer server)
{
  if (server->child_pid)
    {
      GIOStatus gio_status ;
      GError *g_error = NULL ;
      int status ;
      char *termination_msg = NULL ;

      /* Why not flush the channel (i.e. why FALSE) ?....check this out..... */
      gio_status = g_io_channel_shutdown(server->gff_pipe, FALSE, &g_error) ; /* or else it may not exit */
      server->gff_pipe = NULL ;
      if (gio_status == G_IO_STATUS_ERROR || gio_status == G_IO_STATUS_AGAIN)
	{
	  setErrorMsgGError(server, &g_error) ;

	  zMapLogWarning("%s", g_error->message) ;
	}

      /* Wait for child's exit status. */
      waitpid(server->child_pid, &status, 0) ;
      server->child_pid = 0 ;

      /* Get the error if there was one. */
      if (!zMapUtilsProcessHasTerminationError(status, &termination_msg))
	{
	  server->exit_code = EXIT_SUCCESS ;
	}
      else
	{
	  server->exit_code = EXIT_FAILURE ;

	  setErrMsg(server, termination_msg) ;

	  zMapLogWarning("%s", termination_msg) ;

	  g_free(termination_msg) ;
	}
    }

  return ;
}



/* read the header data and exit when we get DNA or features or anything else. */
static ZMapServerResponseType pipeGetHeader(PipeServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;
  gboolean empty_file = TRUE ;
  gboolean done_header = FALSE ;
  ZMapGFFHeaderState header_state = GFF_HEADER_NONE ;	    /* got all the ones we need ? */


  if (server->sequence_server)
    zMapGFFParserSetSequenceFlag(server->parser);  // reset done flag for seq else skip the data

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL)
    {
      empty_file = FALSE ;				    /* We have at least some data. */
      result = ZMAP_SERVERRESPONSE_OK;		    // now we have data default is 'OK'

      *(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

      if (zMapGFFParseHeader(server->parser, server->gff_line->str, &done_header, &header_state))
	{
	  if (done_header)
	    break ;
	  else
	    server->gff_line = g_string_truncate(server->gff_line, 0) ;  /* Reset line to empty. */
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
					    zMapGFFGetLineNumber(server->parser), server->gff_line->str) ;
		  setErrMsg(server, err_msg) ;
		  g_free(err_msg) ;

		  ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->script_args,
				     "%s", server->last_err_msg) ;
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
						server->gff_line->str) ;
		      setErrMsg(server, err_msg) ;
		      g_free(err_msg) ;

		      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->script_args,
					 "%s", server->last_err_msg) ;
		    }
		}

	      result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }

	  break ;
	}
    }


  /* Sometimes the file contains only the gff header and no data, I don't know the reason for this
   * but in this case there's no point in going further. */
  if (header_state == GFF_HEADER_ERROR || empty_file)
    {
      char *err_msg ;

      if (status == G_IO_STATUS_EOF)
      	err_msg = g_strdup_printf("EOF reached while trying to read header, at line %d",
				  zMapGFFGetLineNumber(server->parser)) ;
      else
      	err_msg = g_strdup_printf("Error in GFF header, at line %d",
				  zMapGFFGetLineNumber(server->parser)) ;

      setErrMsg(server, err_msg) ;
      g_free(err_msg) ;


      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path, server->script_args,
			 "%s", server->last_err_msg) ;

      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }

  return result ;
}




// read any DNA data at the head of the stream and quit after error or ##end-dna
static ZMapServerResponseType pipeGetSequence(PipeServer server)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;
  gboolean sequence_finished = FALSE;
  gboolean first = TRUE;

  // read the sequence if it's there
  server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

  /* we have already read the first non header line in pipeGetheader */
  do
    {
      if (!first)
	*(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */
      first = FALSE;

      if (!zMapGFFParseSequence(server->parser, server->gff_line->str, &sequence_finished) || sequence_finished)
	break;
    }
  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL);


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
				    server->gff_line->str, error->message) ;
          setErrMsg(server, err_msg) ;
	  g_free(err_msg) ;

          ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->script_args,
			     "%s", server->last_err_msg) ;
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
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockSequence, (gpointer)server) ;

  return ;
}


// "GFF doesn't understand multiple blocks" therefore there can only be one DNA sequence
// so we expect one alignment and one block
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  PipeServer server = (PipeServer) user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)  // no point getting DNA if features are not there
    {
      ZMapSequence sequence;
      if (!(sequence = zMapGFFGetSequence(server->parser)))
	{
	  GError *error;
	  char *estr;

	  error = zMapGFFGetError(server->parser);

	  if (error)
            estr = error->message;
	  else
            estr = "No error reported";

	  setErrMsg(server, estr) ;

	  ZMAPPIPESERVER_LOG(Warning, server->protocol,
			     server->script_path,server->script_args,
			     "%s", server->last_err_msg);
	}
      else
	{
	  ZMapFeatureContext context;
	  ZMapFeatureSet feature_set;

	  if (zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
	    {
	      ZMapFeatureTypeStyle dna_style = NULL;
	      ZMapFeature feature;
	      GHashTable *hash = zMapGFFParserGetStyles(server->parser);

#if 0
	      /* This temp style creation feels wrong, and probably is,
	       * but we don't have the merged in default styles in here,
	       * or so it seems... */
	      dna_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
					  ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

	      feature = zMapFeatureDNACreateFeature(feature_block, dna_style,
						    sequence->sequence, sequence->length);

	      zMapStyleDestroy(dna_style);
#else
	      /* the servers need styles to add DNA and 3FT
	       * they used to create temp style and then destroy these but that's not very good
	       * they don't have styles info directly but this is stored in the parser
	       * during the protocol steps, so i wrote a GFF function to supply that info
	       * Now that features have style ref'd indirectly via the featureset we can't use temp data
	       */

	      if (hash)
		dna_style = g_hash_table_lookup(hash, GUINT_TO_POINTER(g_quark_from_string(ZMAP_FIXED_STYLE_DNA_NAME)));
	      if (dna_style)
		feature = zMapFeatureDNACreateFeature(feature_block, dna_style,
						      sequence->sequence, sequence->length);
#endif
	    }


	  // this is insane: asking a pipe server for 3FT, however some old code might expect it
	  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block,
								  ZMAPFEATURE_STRUCT_CONTEXT) ;

	  /* I'm going to create the three frame translation up front! */
	  if (zMap_g_list_find_quark(context->req_feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
	    {
	      if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
		{
		  ZMapFeatureTypeStyle frame_style = NULL;
		  ZMapFeature feature;
#if 0
		  /* NOTE: this old code has the wrong style name ! */
		  frame_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
						ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

		  zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);

		  zMapStyleDestroy(frame_style);
#else
		  /* the servers need styles to add DNA and 3FT
		   * they used to create temp style and then destroy these but that's not very good
		   * they don't have styles info directly but this is stored in the parser
		   * during the protocol steps, so i wrote a GFF function to supply that info
		   * Now that features have style ref'd indirectly via the featureset we can't use temp data
		   */
		  GHashTable *hash = zMapGFFParserGetStyles(server->parser);

		  if (hash)
		    frame_style = g_hash_table_lookup(hash, GUINT_TO_POINTER(g_quark_from_string(ZMAP_FIXED_STYLE_3FT_NAME)));
		  if (frame_style)
		    feature = zMapFeatureDNACreateFeature(feature_block, frame_style,
							  sequence->sequence, sequence->length);
#endif
		}
	    }

	  g_free(sequence);
	}
    }
  return ;
}





/* A bit of a lash up for now, we need the parent->child mapping for a sequence and since
 * the code in this file so far simply reads a GFF stream for now, we just fake it by setting
 * everything to be the same for child/parent... */
static void addMapping(ZMapFeatureContext feature_context, int req_start, int req_end)
{
  ZMapFeatureBlock feature_block = NULL;		    // feature_context->master_align->blocks->data ;

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

  if (get_features_data->result == ZMAP_SERVERRESPONSE_OK)
    {
      PipeServer server = get_features_data->server ;
      ZMapGFFParser parser = server->parser ;
      GString* gff_line = server->gff_line ;
      GIOStatus status ;
      gsize terminator_pos = 0 ;
      gboolean free_on_destroy = FALSE ;
      GError *gff_pipe_err = NULL ;
      gboolean first ;

      /* The caller may only want a small part of the features in the stream so we set the
       * feature start/end from the block, not the gff stream start/end. */
      if (server->zmap_end)
        {
          zMapGFFSetFeatureClipCoords(parser,
                                      server->zmap_start,
                                      server->zmap_end) ;
          zMapGFFSetFeatureClip(parser,GFF_CLIP_ALL);       // mh17: needs config added to server stanza for clip type
        }

      first = TRUE ;
      do
        {
          if (first)
            first = FALSE ;
          else
            *(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */

          if (!zMapGFFParseLine(parser, gff_line->str))
            {
              GError *error ;

              error = zMapGFFGetError(parser) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
              /* Surely we don't need to do this....the gff code should always set the error... */

              if (!error)
                {
                  server->last_err_msg =
                    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
                                    zMapGFFGetLineNumber(parser), gff_line->str) ;
                  ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->script_args,
                                     "%s", server->last_err_msg) ;

                  result = FALSE ;
                }
              else
                {
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->script_args,
                                     "%s", error->message) ;

                  /* If the error was serious we stop processing and return the error. */
                  if (zMapGFFTerminated(parser))
                    {
                      get_features_data->result = ZMAP_SERVERRESPONSE_REQFAIL ;

                      setErrMsg(server, error->message) ;

                      break ;
                    }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

            }

          gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

        } while ((status = g_io_channel_read_line_string(server->gff_pipe, gff_line, &terminator_pos,
                                                         &gff_pipe_err)) == G_IO_STATUS_NORMAL) ;


      /* If we reached the end of the stream then all is fine, so return features... */
      free_on_destroy = TRUE ;
      if (status == G_IO_STATUS_EOF)
        {
          if (get_features_data->result == ZMAP_SERVERRESPONSE_OK
              && zMapGFFGetFeatures(parser, feature_block))
            {
              free_on_destroy = FALSE ;			    /* Make sure parser does _not_ free
							       our data ! */
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
                                 server->script_path, err_msg) ;

                  setErrMsg(server, err_msg) ;
                }
            }
        }
      else
        {
          zMapLogWarning("Could not read GFF features from stream \"%s\" with \"%s\".",
                         server->script_path, &gff_pipe_err) ;

          setErrorMsgGError(server, &gff_pipe_err) ;
        }

      zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (!getFeatures(server, server->parser, server->gff_line, feature_block))
	{
	  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,
			     server->script_args,
			     "Could not map %s because: %s",
			     g_quark_to_string(server->req_context->sequence_name),
			     server->last_err_msg) ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Get the features for a block. */
static gboolean getFeatures(PipeServer server, ZMapGFFParser parser, GString* gff_line,
			    ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  gboolean free_on_destroy = FALSE ;
  GError *gff_pipe_err = NULL ;
  gboolean first ;

  /* Tempting to check that block is inside features_start/end, but it may be a different
   * sequence....we don't really deal with this...  */


  /* The caller may only want a small part of the features in the stream so we set the
   * feature start/end from the block, not the gff stream start/end. */

  if (server->zmap_end)
    {
      zMapGFFSetFeatureClipCoords(parser,
				  server->zmap_start,
				  server->zmap_end) ;
      zMapGFFSetFeatureClip(parser,GFF_CLIP_ALL);       // mh17: needs config added to server stanza for clip type
    }

  first = TRUE ;
  do
    {
      if (first)
	first = FALSE ;
      else
	*(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */

      if (!zMapGFFParseLine(parser, gff_line->str))
	{
	  GError *error = zMapGFFGetError(parser) ;

	  if (!error)
	    {
	      server->last_err_msg =
		g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
				zMapGFFGetLineNumber(parser), gff_line->str) ;
	      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->script_args,
				 "%s", server->last_err_msg) ;

	      result = FALSE ;
	    }
	  else
	    {
	      /* If the error was serious we stop processing and return the error,
	       * otherwise we just log the error. */
	      if (zMapGFFTerminated(parser))
		{
		  result = FALSE ;
		  setErrMsg(server, error->message) ;
		}
	      else
		{
		  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->script_args,
				     "%s", error->message) ;
		}
	    }
	}

      gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

    } while ((status = g_io_channel_read_line_string(server->gff_pipe, gff_line, &terminator_pos,
						     &gff_pipe_err)) == G_IO_STATUS_NORMAL) ;


  /* If we reached the end of the stream then all is fine, so return features... */
  free_on_destroy = TRUE ;
  if (status == G_IO_STATUS_EOF)
    {
      if (zMapGFFGetFeatures(parser, feature_block))
	{
	  free_on_destroy = FALSE ;			    /* Make sure parser does _not_ free
							       our data ! */

	  result = TRUE ;
	}
    }
  else
    {
      zMapLogWarning("Could not read GFF features from stream \"%s\"", server->script_path) ;

      setErrorMsgGError(server, &gff_pipe_err) ;

      result = FALSE ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */







static gboolean getServerInfo(PipeServer server, ZMapServerReqGetServerInfo info)
{
  gboolean result = TRUE ;

  info->data_format_out = g_strdup_printf("GFF version %d", server->gff_version) ;

  info->database_path_out = g_strdup(server->script_path) ;
  info->request_as_columns = FALSE;

  return result ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_pipe_err_inout to NULL */
static void setErrorMsgGError(PipeServer server, GError **gff_pipe_err_inout)
{
  GError *gff_pipe_err ;
  char *msg = "failed";

  gff_pipe_err = *gff_pipe_err_inout ;
  if (gff_pipe_err)
    msg = gff_pipe_err->message ;

  setErrMsg(server, msg) ;

  if (gff_pipe_err)
    g_error_free(gff_pipe_err) ;

  *gff_pipe_err_inout = NULL ;

  return ;
}


/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(PipeServer server, char *new_msg)
{
  char *error_msg ;

  waitForChild(server) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  error_msg = g_strdup_printf("SERVER: \"%s\",   REQUEST: \"%s\",   ERROR: \"%s\"",
			      server->script_path, server->script_args, new_msg) ;

  server->last_err_msg = error_msg ;

  return ;
}

