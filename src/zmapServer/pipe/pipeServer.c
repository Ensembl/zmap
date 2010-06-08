/*  File: pipeServer.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *      derived from fileServer.c by Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: These functions provide code to read the output of a script
 *		as though it were a server according to the interface defined
 *          for accessing servers. The aim is to allow ZMap to request
 *		arbritary data from external sources as defined in the config files.
 *
 * NB:	As for the fileServer module the data is read as a single pass and then discarded.
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 * HISTORY:
 * Last edited: Jan 14 10:10 2010 (edgrif)
 * Created: 2009-11-26 12:02:40 (mh17)
 * CVS info:   $Id: pipeServer.c,v 1.24 2010-06-08 08:31:24 mh17 Exp $
 *-------------------------------------------------------------------
 */


/* WARNING, THIS DOES NOT COPE WITH MULTIPLE ALIGNS/BLOCKS AS IT STANDS, TO DO THAT REQUIRES
 * WORK BOTH ON THE GFF PARSER CODE (TO ACCEPT ALIGN/BLOCK ID/COORDS AND ON THIS CODE TO
 * GENERALISE IT MORE TO DEAL WITH BLOCKS...I'LL DO THAT NEXT....EG */



#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapServerPrototype.h>
#include <pipeServer_P.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>


#ifdef MH_NOW_IN_PIPESERVER_STRUCT
/* Used to control getting features in all alignments in all blocks... */
typedef struct
{
  ZMapServerResponseType result ;
  PipeServer server ;
  ZMapGFFParser parser
  GString * gff_line ;
} GetFeaturesStruct, *GetFeatures ;
#endif


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout) ;

static gboolean pipe_server_spawn(PipeServer server,GError **error);
static ZMapServerResponseType openConnection(void *server, gboolean sequence_server) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
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
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static void addMapping(ZMapFeatureContext feature_context, ZMapGFFHeader header) ;
static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlock(gpointer key, gpointer data, gpointer user_data) ;
static gboolean sequenceRequest(PipeServer server, ZMapGFFParser parser, GString* gff_line,
				ZMapFeatureBlock feature_block) ;
static void setLastErrorMsg(PipeServer server, GError **gff_pipe_err_inout) ;

static gboolean getServerInfo(PipeServer server, ZMapServerInfo info) ;
static void setErrMsg(PipeServer server, char *new_msg) ;

static ZMapServerResponseType pipeGetHeader(PipeServer server);
static ZMapServerResponseType pipeGetSequence(PipeServer server);



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
  pipe_funcs->close = closeConnection;
  pipe_funcs->destroy = destroyConnection ;

  return ;
}


/*
 *    Although these routines are static they form the external interface to the pipe server.
 */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;
  return result ;
}



/* Read ZMap application defaults. */
static void getConfiguration(PipeServer server)
{
  ZMapConfigIniContext context;

  if((context = zMapConfigIniContextProvide()))
    {
      char *tmp_string  = NULL;

      /* default script directory to use */
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				       ZMAPSTANZA_APP_SCRIPTS, &tmp_string))
	{
	  server->script_dir = tmp_string;
	}
      else
      {
        server->script_dir = g_get_current_dir();
      }

       /* default directory to use */
      if(zMapConfigIniContextGetString(context, ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
                               ZMAPSTANZA_APP_DATA, &tmp_string))
      {
        server->data_dir = tmp_string;
      }
      else
      {
        server->data_dir = g_get_current_dir();
      }

      zMapConfigIniContextDestroy(context);
    }
}



/* spawn a new process to run our script and open a pipe for read to get the output
 * global config of the default scripts directory is via '[ZMap] scripts-dir=there'
 * to get absolute path you need 4 slashes eg for pipe://<host>/<path>
 * where <host> is null and <path> begins with a /
 * For now we will assume that "host" contains the script name and then we just ignore the other
 * parameters....
 *
 *  */
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout_unused)
{
  gboolean result = TRUE ;
  PipeServer server ;
  gchar *dir;

  zMapAssert(url->path) ;

  /* Always return a server struct as it contains error message stuff. */
  server = (PipeServer)g_new0(PipeServerStruct, 1) ;
  *server_out = (void *)server ;

  getConfiguration(server);	// get scripts directory

  server->scheme = url->scheme;
  dir = (server->scheme == SCHEME_PIPE) ? server->script_dir : server->data_dir;
  /* We could check both format and version here if we wanted to..... */


  /* This code is a hack to get round the fact that the url code we use in zmapView.c to parse
   * urls from our config file will chop the leading "/" off the path....which causes the
   * zMapGetPath() call below to construct an incorrect path.... */
  {
    char *tmp_path = url->path ;

    if (*(url->path) != '/')
      tmp_path = g_strdup_printf("%s/%s", dir,url->path) ;

    server->script_path = zMapGetPath(tmp_path) ;

    if (tmp_path != url->path)
      g_free(tmp_path) ;
  }

  if(url->query)
      server->query = g_strdup_printf("%s",url->query);
  else
      server->query = g_strdup("");

  server->protocol = PIPE_PROTOCOL_STR;
  if(server->scheme == SCHEME_FILE)
        server->protocol = FILE_PROTOCOL_STR;

  server->zmap_start = 1;
  server->zmap_end = 0; // default to all of it

  return result ;
}




/*
 * fork and exec the script and read the output via a pipe
 * no data sent to STDIN and STDERR ignored
 * in case of errors or hangups eventually we will time out and an error popped up.
 * downside is limited to not having the data, which is what happens anyway
 */
static gboolean pipe_server_spawn(PipeServer server,GError **error)
{
  gboolean result = FALSE;
  gchar **argv, **q_args, *z_start,*z_end;
  gint pipe_fd;
  GError *pipe_error = NULL;
  int i;
  gint err_fd;

  q_args = g_strsplit(server->query,"&",0);
  argv = (gchar **) g_malloc (sizeof(gchar *) * (PIPE_MAX_ARGS + g_strv_length(q_args)));
  argv[0] = server->script_path; // scripts can get exec'd, as long as they start w/ #!
  for(i = 1;q_args[i-1];i++)
      argv[i] = q_args[i-1];

      // now add on zmap args such as start and end
  if(server->zmap_start || server->zmap_end)
  {
      argv[i++] = z_start = g_strdup_printf("%s=%d",PIPE_ARG_ZMAP_START,server->zmap_start);
      argv[i++] = z_end = g_strdup_printf("%s=%d",PIPE_ARG_ZMAP_END,server->zmap_end);
  }

  argv[i]= NULL;

  result = g_spawn_async_with_pipes(server->script_dir,argv,NULL,G_SPAWN_CHILD_INHERITS_STDIN, // can't not give a flag!
                                    NULL,NULL,&server->child_pid,NULL,&pipe_fd,&err_fd,&pipe_error);
  if(result)
  {
    server->gff_pipe = g_io_channel_unix_new(pipe_fd);
    server->gff_stderr = g_io_channel_unix_new(err_fd);
    g_io_channel_set_flags(server->gff_stderr,G_IO_FLAG_NONBLOCK,&pipe_error);
  }

  g_free(argv);   // strings allocated and freed seperately
  g_strfreev(q_args);
  if(server->zmap_start || server->zmap_end)
  {
      g_free(z_start);
      g_free(z_end);
  }

  if(error)
    *error = pipe_error;
  return(result);
}


/*
 * read stderr from the external source and if non empty display and log some messages
 * use non-blocking i/o so we don't hang ???
 * gets called by setErrMsg() - if the server fails we read STDERR and possibly report why
 * if no failures we ignore STDERR
 * the last message is the one that gets popped up to the user
 * We log all messages except the last as that generally appears twice in the log anyway
 */

// NB: otterlace would prefer to get all of STDERR, which needs a slight rethink
gchar *pipe_server_get_stderr(PipeServer server)
{
  GIOStatus status ;
  gsize terminator_pos = 0;
  GError *gff_pipe_err = NULL;
  gchar * msg = NULL;
  GString *line;

  if(!server->gff_stderr)
      return(NULL);

  line = g_string_sized_new(2000) ;      /* Probably not many lines will be > 2k chars. */

  while (1)
    {
/* this doesn't work: doesn't hang, but doesn't read the error message anyway
 * there is a race condition on big files (i think) where on notsupported errors we look at stderr
 * and wait for ever as the script didn't finish yet as we didn't read the data.
 *      gc = g_io_channel_get_buffer_condition(server->gff_stderr);
 *      GIOCondition gc;
 *      if(!(gc & G_IO_IN))
 *            break;
 */
      status = g_io_channel_read_line_string(server->gff_stderr, line,
            &terminator_pos,&gff_pipe_err);
      if(status != G_IO_STATUS_NORMAL)
        break;

      if(msg)
            ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,"%s", msg) ;

      *(line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */
      if(terminator_pos > 0)              // can get blank lines at the end
      {
            if(msg)
                  g_free(msg);
            msg = g_strdup(line->str);
      }
    }

    g_string_free(line,TRUE);
    return(msg);
}


static ZMapServerResponseType openConnection(void *server_in, gboolean sequence_server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;
  int retval = FALSE;
  if (server->gff_pipe)
    {
      zMapLogWarning("Feature script \"%s\" already active.", server->script_path) ;
    }
  else
    {
      GError *gff_pipe_err = NULL ;

      if(server->scheme == SCHEME_FILE)   // could spawn /bin/cat but there is no need
      {
        if((server->gff_pipe = g_io_channel_new_file(server->script_path, "r", &gff_pipe_err)))
            retval = TRUE;
      }
      else
      {
        if(pipe_server_spawn(server,&gff_pipe_err))
            retval = TRUE;
      }

      server->sequence_server = sequence_server;         /* if not then drop any DNA data */
      server->parser = zMapGFFCreateParser() ;
      server->gff_line = g_string_sized_new(2000) ;      /* Probably not many lines will be > 2k chars. */


      if(retval)
	  {
	    result = pipeGetHeader(server);
          if(result == ZMAP_SERVERRESPONSE_OK)
            {
              // always read it: have to skip over if not wanted
              // need a flag here to say if this is a sequence server
              // ignore error response as we want to report open is OK
              pipeGetSequence(server);
            }
	  }
	if(!retval || result != ZMAP_SERVERRESPONSE_OK)
	  {
	    setLastErrorMsg(server, &gff_pipe_err) ;
	    result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  }
    }

  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerInfo info)
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
      ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
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

      // these are needed by the GFF parser
  server->source_2_sourcedata = *source_2_sourcedata_inout;
  server->featureset_2_column = *featureset_2_column_inout;

  setErrMsg(server, g_strdup("Feature Sets cannot be read from GFF stream.")) ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
		 "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

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
  setErrMsg(server, g_strdup("Reading styles from a GFF stream is not supported.")) ;
  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path, server->query,
		 "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}


/* GFF stream styles do not include a mode (e.g. transcript etc) so this function
 * always returns unsupported.
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  PipeServer server = (PipeServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
  PipeServer server = (PipeServer)server_in ;

  server->req_context = feature_context ;


  /* Like the acedb server we should be getting the mapping from the gff data here....
   * then we could set up the block/etc stuff...this is where add mapping should be called from... */



  return result ;
}


// read the header data and exit when we get DNA or features or anything else
static ZMapServerResponseType pipeGetHeader(PipeServer server)
{
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;

  GError *error = NULL ;

  server->result = ZMAP_SERVERRESPONSE_REQFAIL ;  // to catch empty file

  if(server->sequence_server)
      zMapGFFParserSetSequenceFlag(server->parser);  // reset done flag for seq esle skip the data

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
                                     &terminator_pos,
                                     &gff_pipe_err)) == G_IO_STATUS_NORMAL)
    {
      gboolean done_header = FALSE ;
      server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

      *(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

      if (zMapGFFParseHeader(server->parser, server->gff_line->str, &done_header))
      {
        if (done_header)
          break ;
        else
          server->gff_line = g_string_truncate(server->gff_line, 0) ;
                                              /* Reset line to empty. */
      }
      else
      {
        if (!done_header)
          {
            error = zMapGFFGetError(server->parser) ;

            if (!error)
            {
              /* SHOULD ABORT HERE.... */
              setErrMsg(server,
                      g_strdup_printf("zMapGFFParseHeader() failed with no GError for line %d: %s",
                                  zMapGFFGetLineNumber(server->parser), server->gff_line->str)) ;
              ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
                         "%s", server->last_err_msg) ;
            }
            else
            {
              /* If the error was serious we stop processing and return the error,
               * otherwise we just log the error. */
              if (zMapGFFTerminated(server->parser))
                {
                  server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
                  setErrMsg(server, g_strdup_printf("%s", error->message)) ;
                }
              else
                {
                  setErrMsg(server,
                        g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
                                    zMapGFFGetLineNumber(server->parser),
                                    server->gff_line->str)) ;
                  ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
                             "%s", server->last_err_msg) ;
                }
            }
            server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
          }

        break ;
      }
    }
    return(server->result);
}

// read any DNA data at the head of the stream and quit after error or ##end-dna
static ZMapServerResponseType pipeGetSequence(PipeServer server)
{
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;
  gboolean sequence_finished = FALSE;

      // read the sequence if it's there
  server->result = ZMAP_SERVERRESPONSE_OK;   // now we have data default is 'OK'

  while ((status = g_io_channel_read_line_string(server->gff_pipe, server->gff_line,
                                     &terminator_pos,
                                     &gff_pipe_err)) == G_IO_STATUS_NORMAL)
    {
      *(server->gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

      if(!zMapGFFParseSequence(server->parser, server->gff_line->str, &sequence_finished) || sequence_finished)
            break;
    }

  error = zMapGFFGetError(server->parser) ;

  if(error)
    {
      /* If the error was serious we stop processing and return the error,
      * otherwise we just log the error. */
      if (zMapGFFTerminated(server->parser))
      {
          server->result = ZMAP_SERVERRESPONSE_REQFAIL ;
          setErrMsg(server, g_strdup_printf("%s", error->message)) ;
      }
      else
      {
          setErrMsg(server,g_strdup_printf("zMapGFFParseSequence() failed for line %d: %s - %s",
                        zMapGFFGetLineNumber(server->parser),
                        server->gff_line->str,error->message)) ;
          ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
                  "%s", server->last_err_msg) ;
      }
      server->result = ZMAP_SERVERRESPONSE_REQFAIL;
    }
    else if(!sequence_finished)
    {
      server->result = ZMAP_SERVERRESPONSE_UNSUPPORTED;
    }

    return(server->result);
}


/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
{
  PipeServer server = (PipeServer)server_in ;

  ZMapGFFHeader header ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    /* DEBUG.... */
    GList *style_names ;

    zMap_g_quark_list_print(feature_context->feature_set_names) ;

    style_names = zMapStylesGetNames(feature_context->styles) ;
    zMap_g_quark_list_print(style_names) ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  // we assume we called pipeGetHeader() already and also pipeGetSequence()
  // so we are at the start of the BODY part of the stream

  zMapGFFParseSetSourceHash(server->parser, server->featureset_2_column, server->source_2_sourcedata) ;

  zMapGFFParserInitForFeatures(server->parser, styles, FALSE) ;  // FALSE = create features

  // default to OK, previous pipeGetSequence() could have set unsupported
  // if no DNA was provided

  server->result = ZMAP_SERVERRESPONSE_OK;
    {
      header = zMapGFFGetHeader(server->parser) ;

      addMapping(feature_context, header) ;

      zMapGFFFreeHeader(header) ;


      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      // can only have one alignment and one block ??
      g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)server) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
	GError *error = NULL ;
            server->parser->state == ZMAPGFF_PARSE_BODY;
	zMapFeatureDumpStdOutFeatures(feature_context, &error) ;

      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    }


  /* Clear up. -> in destroyConnection() */
//  zMapGFFDestroyParser(server->parser) ;
//  g_sring_free(server->gff_line, TRUE) ;


  return server->result ;
}



// "GFF doesnlt understand multiple blocks" therefore there can only be one DNA sequence
// so we expect one alignment and one block
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  PipeServer server = (PipeServer) user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)  // no point getting DNA if features are not there
    {
      ZMapSequence sequence;
      if(!(sequence = zMapGFFGetSequence(server->parser)))
	{
	  GError *error;
        char *estr;

	  error = zMapGFFGetError(server->parser);
        if(error)
            estr = error->message;
        else
            estr = "No error reported";
	  setErrMsg(server,
		    g_strdup_printf("zMapGFFGetSequence() failed, error=%s",estr));
	  ZMAPPIPESERVER_LOG(Warning, server->protocol,
			 server->script_path,server->query,
			 "%s", server->last_err_msg);
	}
      else
	{
	  ZMapFeatureContext context;
	  ZMapFeatureSet feature_set;

	  if(zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
	    {
	      ZMapFeatureTypeStyle dna_style = NULL;
	      ZMapFeature feature;

	      /* This temp style creation feels wrong, and probably is,
	       * but we don't have the merged in default styles in here,
	       * or so it seems... */
	      dna_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
					  ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

	      feature = zMapFeatureDNACreateFeature(feature_block, dna_style,
						    sequence->sequence, sequence->length);

	      zMapStyleDestroy(dna_style);
	    }

	  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block,
								  ZMAPFEATURE_STRUCT_CONTEXT) ;

	  /* I'm going to create the three frame translation up front! */
	  if (zMap_g_list_find_quark(context->feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
	    {
	      if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
	      {
		  ZMapFeatureTypeStyle frame_style = NULL;

		  frame_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
						ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

		  zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);

		  zMapStyleDestroy(frame_style);
		}
	    }

	  g_free(sequence);
	}
    }
  return ;
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


/*
 * we have pre-read the sequence and simple copy/move the data over if it's there
 */
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out)
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



static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;
  GError *gff_pipe_err = NULL ;

  if(server->child_pid)
    g_spawn_close_pid(server->child_pid);

  if(server->parser)
      zMapGFFDestroyParser(server->parser) ;
  if(server->gff_line)
      g_string_free(server->gff_line, TRUE) ;

  if (server->gff_pipe)
  {
    if(g_io_channel_shutdown(server->gff_pipe, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
      {
        zMapLogCritical("Could not close feature pipe \"%s\"", server->script_path) ;

        setLastErrorMsg(server, &gff_pipe_err) ;

        result = ZMAP_SERVERRESPONSE_REQFAIL ;
      }
    else
      {
        /* this seems to be required to destroy the GIOChannel.... */
        g_io_channel_unref(server->gff_pipe) ;
        server->gff_pipe = NULL ;
      }
  }
  if (server->gff_stderr )
  {
    if(g_io_channel_shutdown(server->gff_stderr, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
    {
      zMapLogCritical("Could not close error pipe \"%s\"", server->script_path) ;

      setLastErrorMsg(server, &gff_pipe_err) ;

      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  else
    {
      /* this seems to be required to destroy the GIOChannel.... */
      g_io_channel_unref(server->gff_stderr) ;
      server->gff_stderr = NULL ;
    }
  }
  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;

//printf("pipeserver destroy_connection\n");
  if (server->script_path)
    g_free(server->script_path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  /* Clear up. -> in destroyConnection() */
/* crashes...
  if(server->parser)
      zMapGFFDestroyParser(server->parser) ;
  if(server->gff_line)
      g_string_free(server->gff_line, TRUE) ;
*/

  g_free(server) ;

  return result ;
}



/*
 * ---------------------  Internal routines.  ---------------------
 */


/* A bit of a lash up for now, we need the parent->child mapping for a sequence and since
 * the code in this file so far simply reads a GFF stream for now, we just fake it by setting
 * everything to be the same for child/parent... */
static void addMapping(ZMapFeatureContext feature_context, ZMapGFFHeader header)
{
  ZMapFeatureBlock feature_block = NULL;//feature_context->master_align->blocks->data ;

  feature_block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(feature_context->master_align->blocks, 0)) ;

  /* We just override whatever is already there...this may not be the thing to do if there
   * are several streams.... */
  feature_context->parent_name = feature_context->sequence_name ;

  /* I don't like having to do this right down here but user is allowed to specify "0" for
   * end coord meaning "to the end of the sequence" and this is where we know the end... */

  // refer to comment in zmapFeature.h 'Sequences and Block Coordinates'
  // NB at time of writing parent_span not always initialised
  feature_context->parent_span.x1 = 1;
  if(feature_context->parent_span.x2 < header->features_end)
      feature_context->parent_span.x2 = header->features_end ;

  // seq coords from parent sequence
  feature_context->sequence_to_parent.p1 = 1;
  feature_context->sequence_to_parent.p2 = header->features_end;

   // seq coords for our sequence based from 1
  feature_context->sequence_to_parent.c1 = feature_block->block_to_sequence.q1 = 1;
  feature_context->sequence_to_parent.c2 = feature_block->block_to_sequence.q2
                                         = header->features_end;

  // block coordinates if not pre-specified
  if(feature_block->block_to_sequence.t2 == 0)
  {
      feature_block->block_to_sequence.t1 = header->features_start ;
      feature_block->block_to_sequence.t2 = header->features_end ;
  }

  return ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_pipe_err_inout to NULL */
static void setLastErrorMsg(PipeServer server, GError **gff_pipe_err_inout)
{
  GError *gff_pipe_err ;
  char *msg = "failed";

  zMapAssert(server && gff_pipe_err_inout);

  gff_pipe_err = *gff_pipe_err_inout ;
  if(gff_pipe_err)
    {
      msg = gff_pipe_err->message;
      g_error_free(gff_pipe_err) ;
    }
  setErrMsg(server, g_strdup_printf("%s %s", server->script_path, msg)) ;



  *gff_pipe_err_inout = NULL ;

  return ;
}



/* Process all the alignments in a context. */
static void eachAlignment(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlock, (gpointer)server) ;

  return ;
}



static void eachBlock(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  PipeServer server = (PipeServer)user_data ;

  if (server->result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!sequenceRequest(server, server->parser,
			   server->gff_line, feature_block))
	{
	  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,
                  server->query,
			 "Could not map %s because: %s",
			 g_quark_to_string(server->req_context->sequence_name),
			 server->last_err_msg) ;
	}
    }

  return ;
}


static gboolean sequenceRequest(PipeServer server, ZMapGFFParser parser, GString* gff_line,
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
  if(feature_block->block_to_sequence.t2)
  {
      zMapGFFSetFeatureClipCoords(parser,
			      feature_block->block_to_sequence.t1,
			      feature_block->block_to_sequence.t2) ;
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
	      ZMAPPIPESERVER_LOG(Critical, server->protocol, server->script_path,server->query,
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
		  setErrMsg(server, g_strdup_printf("%s", error->message)) ;
		}
	      else
		{
		  ZMAPPIPESERVER_LOG(Warning, server->protocol, server->script_path,server->query,
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

      setLastErrorMsg(server, &gff_pipe_err) ;

      result = FALSE ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return result ;
}


static gboolean getServerInfo(PipeServer server, ZMapServerInfo info)
{
  gboolean result = FALSE ;

  result = TRUE ;
  info->database_path = g_strdup(server->script_path) ;

  return result ;
}


/* It's possible for us to have reported an error and then another error to come along. */
/* mgh: if we get an error such as pipe broken then let's look at stderr and if there's a message there report it */
static void setErrMsg(PipeServer server, char *new_msg)
{
  gchar *errmsg;

  errmsg = pipe_server_get_stderr(server);
  if(errmsg)
  {
      g_free(new_msg);
      new_msg = errmsg;       // explain the cause of the error not the symptom
  }

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}

