/*  File: fileServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: These functions provide code to read a file as though
 *              it were a server according to the interface defined
 *              for accessing servers. It's crude at the moment but
 *              the aim is to allow zmap to be run from just files
 *              if required. Read the caveats below though...
 *
 * Exported functions: See ZMap/zmapServerPrototype.h
 *
 * iff'ed out 07 Dec 2009 (mgh) - module replaced by pipeServer.c
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>







/* GFF files have a prescribed format which specifically does not include anything
 * about how the features are displayed so the zmap styles must be provided separatly.
 *
 * In addition the code assumes that the gff file describes just one block of features.
 * In theory the file could contain multiple blocks but that is not straight forward
 * as gff provides for multiple sequences with their features in a single file but _not_
 * multiple sub-parts of a single sequence. Hence we do not support that currently.
 *
 *  */

 #if USING_OLD_FILE_SERVER
//mgh: replaced  fileServer with pipeServer

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapServerPrototype.h>
#include <fileServer_P.h>


/* Used to control getting features in all alignments in all blocks... */
typedef struct
{
  ZMapServerResponseType result ;
  FileServer server ;
  ZMapGFFParser parser ;
  GString *gff_line ;

  gboolean sequence_finished ;
} GetFeaturesStruct, *GetFeatures ;



static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout) ;

static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
						 GList **feature_sets_out,
						 GList *sources,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_inout,
						 GHashTable **featureset_2_column_out) ;
static ZMapServerResponseType getStyles(void *server, GData **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server,  ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GData *styles, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GData *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code) ;
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;

static gboolean getSequenceMapping(FileServer server, ZMapFeatureContext feature_context) ;
static void addMapping(ZMapFeatureContext feature_context, ZMapGFFHeader header) ;

static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlock(gpointer key, gpointer data, gpointer user_data) ;
static gboolean featuresRequest(FileServer server, ZMapGFFParser parser, GString *gff_line,
				ZMapFeatureBlock feature_block) ;

static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data) ;

static gboolean getServerInfo(FileServer server, ZMapServerInfo info) ;

static void setLastErrorMsg(FileServer server, GError **gff_file_err_inout) ;
static void setErrMsg(FileServer server, char *new_msg) ;
static gboolean resetToStart(FileServer server) ;




/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact a file server. The functions are only visible through
 * this struct. */
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
  file_funcs->get_status = getStatus ;
  file_funcs->get_connect_state = getConnectState ;
  file_funcs->close = closeConnection;
  file_funcs->destroy = destroyConnection ;

  return ;
}


/*
 *    Although these routines are static they form the external interface to the file server.
 */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;



  return result ;
}


/* Check we can read (and write ??) to the file and open it and perhaps check that it contains
 * a format that we can read.
 *
 * For now we will assume that "host" contains the filename and then we just ignore the other
 * parameters....
 *
 *  */
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout_unused)
{
  gboolean result = TRUE ;
  FileServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (FileServer)g_new0(FileServerStruct, 1) ;
  *server_out = (void *)server ;


  /* We could check both format and version here if we wanted to..... */


  /* This code is a hack to get round the fact that the url code we use in zmapView.c to parse
   * urls from our config file will chop the leading "/" off the file path....which causes the
   * zMapGetPath() call below to construct an incorrect path.... */
  {
    char *tmp_path ;

    if (*(url->path) != '/')
      tmp_path = g_strdup_printf("/%s", url->path) ;
    else
      tmp_path = g_strdup(url->path) ;

    server->file_path = zMapGetPath(tmp_path) ;

    g_free(tmp_path) ;
  }


  return result ;
}


/* The model we use for processing the GFF file is that we open it just once
 * and then close it when the closeConnection is called. If we need to start
 * at the beginning of the file again we just reset the file pointer to the
 * start. */
static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  if (server->gff_file)
    {
      zMapLogWarning("Feature file \"%s\" already open.", server->file_path) ;
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  else
    {
      GError *gff_file_err = NULL ;

      if ((server->gff_file = g_io_channel_new_file(server->file_path, "r", &gff_file_err)))
	{
	  result = ZMAP_SERVERRESPONSE_OK ;
	}
      else
	{
	  setLastErrorMsg(server, &gff_file_err) ;

	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
    }


  return result ;
}


static ZMapServerResponseType getInfo(void *server_in, ZMapServerInfo info)
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
      ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
		     "Could not get server info because: %s", server->last_err_msg) ;
    }

  return result ;
}


/* We could parse out all the "source" fields from the gff file but I don't have time
 * to do this now. So we just return "unsupported", so if this function is called it
 * will alert the caller that something has gone wrong.
 *
 *  */
static ZMapServerResponseType getFeatureSetNames(void *server_in,
						 GList **feature_sets_inout,
						 GList *sources,
						 GList **required_styles_out,
						 GHashTable **featureset_2_stylelist_inout,
						 GHashTable **featureset_2_column_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  setErrMsg(server, g_strdup("Feature Sets cannot be read from a GFF file.")) ;
  ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
		 "%s", server->last_err_msg) ;


  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;


  return result ;
}



/* We cannot parse the styles from a gff file, gff simply doesn't have display styles so we
 * just return "unsupported", so if this function is called it will alert the caller that
 * something has gone wrong.
 *
 *  */
static ZMapServerResponseType getStyles(void *server_in, GData **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  setErrMsg(server, g_strdup("Reading styles from a GFF file is not supported.")) ;
  ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
		 "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}


/* GFF File styles do not include a mode (e.g. transcript etc) so this function
 * always returns unsupported.
 */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  FileServer server = (FileServer)server_in ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return result ;
}


/* Currently we don't support this even though we could in theory as gff files can
 * contains arbitrary sequences. */
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result =  ZMAP_SERVERRESPONSE_UNSUPPORTED ;


  return result ;
}




/* We don't check anything here, we could look in the file to check that it matches the context
 * I guess which is essentially what we do for the acedb server. */
static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;
  gboolean status ;

  server->req_context = feature_context ;

  if (!(status = getSequenceMapping(server, feature_context)))
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
		     "Could not map %s because: %s",
		     g_quark_to_string(server->req_context->sequence_name), server->last_err_msg) ;
    }
  else
    {
      if (!resetToStart(server))
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
			 "Could not reset to file start %s because: %s",
			 g_quark_to_string(server->req_context->sequence_name), server->last_err_msg) ;
	}
    }


  return result ;
}


/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, GData *styles, ZMapFeatureContext feature_context)
{
  FileServer server = (FileServer)server_in ;
  GetFeaturesStruct get_features = {0} ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_file_err = NULL ;
  ZMapGFFHeader header ;
  GError *error = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    /* DEBUG.... */
    GList *style_names ;

    zMap_g_quark_list_print(feature_context->feature_set_names) ;

    style_names = zMapStylesGetNames(feature_context->styles) ;
    zMap_g_quark_list_print(style_names) ;
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (FileServer)server_in ;

  get_features.parser = zMapGFFCreateParser() ;
  zMapGFFParserInitForFeatures(get_features.parser, styles, FALSE) ;

  get_features.gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be >
							       2k chars. */

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_file, get_features.gff_line,
						 &terminator_pos,
						 &gff_file_err)) == G_IO_STATUS_NORMAL)
    {
      gboolean done_header = FALSE ;

      *(get_features.gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

      if (zMapGFFParseHeader(get_features.parser, get_features.gff_line->str, &done_header))
	{
	  if (done_header)
	    break ;
	  else
	    get_features.gff_line = g_string_truncate(get_features.gff_line, 0) ;
							    /* Reset line to empty. */
	}
      else
	{
	  if (!done_header)
	    {
	      error = zMapGFFGetError(get_features.parser) ;

	      if (!error)
		{
		  /* SHOULD ABORT HERE.... */
		  setErrMsg(server,
			    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
					    zMapGFFGetLineNumber(get_features.parser), get_features.gff_line->str)) ;
		  ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
				 "%s", server->last_err_msg) ;
		}
	      else
		{
		  /* If the error was serious we stop processing and return the error,
		   * otherwise we just log the error. */
		  if (zMapGFFTerminated(get_features.parser))
		    {
		      get_features.result = ZMAP_SERVERRESPONSE_REQFAIL ;
		      setErrMsg(server, g_strdup_printf("%s", error->message)) ;
		    }
		  else
		    {
		      setErrMsg(server,
				g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
						zMapGFFGetLineNumber(get_features.parser),
						get_features.gff_line->str)) ;
		      ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
				     "%s", server->last_err_msg) ;
		    }
		}
	      get_features.result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }

	  break ;
	}
    }

  if (get_features.result == ZMAP_SERVERRESPONSE_OK)
    {
      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the file....can be done but not needed this second. */
      g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&get_features) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
	GError *error = NULL ;

	zMapFeatureDumpStdOutFeatures(feature_context, &error) ;

      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    }


  /* Clear up. */
  zMapGFFDestroyParser(get_features.parser) ;
  g_string_free(get_features.gff_line, TRUE) ;


  return get_features.result ;
}




static ZMapServerResponseType getContextSequence(void *server_in, GData *styles,
						 ZMapFeatureContext feature_context_out)
{
  FileServer server = (FileServer)server_in ;
  GetFeaturesStruct get_features = {0} ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_file_err = NULL ;
  GError *error = NULL ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  return ZMAP_SERVERRESPONSE_UNSUPPORTED ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (FileServer)server_in ;

  get_features.parser = zMapGFFCreateParser() ;
  zMapGFFParserSetSequenceFlag(get_features.parser);

  get_features.gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be >
							       2k chars. */
  if (resetToStart(server))
    {
      gboolean done_header = FALSE ;

      /* Read the header, needed for feature coord range. */
      while ((status = g_io_channel_read_line_string(server->gff_file, get_features.gff_line,
						     &terminator_pos,
						     &gff_file_err)) == G_IO_STATUS_NORMAL)
	{
	  *(get_features.gff_line->str + terminator_pos) = '\0' ; /* Remove terminating newline. */

	  if (!done_header)
	    {
	      if (zMapGFFParseHeader(get_features.parser, get_features.gff_line->str, &done_header))
		{
		  if (!done_header)
		    get_features.gff_line = g_string_truncate(get_features.gff_line, 0) ;
							    /* Reset line to empty. */
		}
	      else
		{
		  if (!done_header)
		    {
		      error = zMapGFFGetError(get_features.parser) ;

		      if (!error)
			{
			  /* SHOULD ABORT HERE.... */
			  setErrMsg(server,
				    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
						    zMapGFFGetLineNumber(get_features.parser), get_features.gff_line->str)) ;
			  ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
					 "%s", server->last_err_msg) ;
			}
		      else
			{
			  /* If the error was serious we stop processing and return the error,
			   * otherwise we just log the error. */
			  if (zMapGFFTerminated(get_features.parser))
			    {
			      get_features.result = ZMAP_SERVERRESPONSE_REQFAIL ;
			      setErrMsg(server, g_strdup_printf("GFFTerminated! %s", error->message)) ;
			      ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
					     "%s", server->last_err_msg) ;		    }
			  else
			    {
			      setErrMsg(server,
					g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
							zMapGFFGetLineNumber(get_features.parser),
							get_features.gff_line->str)) ;
			      ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
					     "%s", server->last_err_msg) ;
			    }
			}
		      get_features.result = ZMAP_SERVERRESPONSE_REQFAIL ;
		    }

		  break ;
		}
	    }
	  else
	    {
	      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
	       * we would have to parse and reparse the file....can be done but not needed this second. */
	      g_hash_table_foreach(feature_context_out->alignments, eachAlignmentSequence, (gpointer)&get_features) ;

	      if (get_features.sequence_finished)
		break ;
	    }
	}
    }


  /* Clear up. */
  zMapGFFDestroyParser(get_features.parser) ;
  g_string_free(get_features.gff_line, TRUE) ;


  return get_features.result ;
}


/* THIS ISN'T RIGHT..... */
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
  *exit_code = 0 ;

  return ZMAP_SERVERRESPONSE_OK;
}


/* Is the acedb server connected ? */
static ZMapServerResponseType getConnectState(void *server_conn, ZMapServerConnectStateType *connect_state)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  /* No implementation currently. */

  return result ;
}


static ZMapServerResponseType closeConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;
  GError *gff_file_err = NULL ;

  if (server->gff_file && g_io_channel_shutdown(server->gff_file, FALSE, &gff_file_err) != G_IO_STATUS_NORMAL)
    {
      zMapLogCritical("Could not close feature file \"%s\"", server->file_path) ;

      setLastErrorMsg(server, &gff_file_err) ;

      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  else
    {
      /* this seems to be required to destroy the GIOChannel.... */
      g_io_channel_unref(server->gff_file) ;

      server->gff_file = NULL ;
    }

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;

  if (server->file_path)
    g_free(server->file_path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  g_free(server) ;

  return result ;
}



/*
 * ---------------------  Internal routines.  ---------------------
 */


/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_file_err_inout to NULL */
static void setLastErrorMsg(FileServer server, GError **gff_file_err_inout)
{
  GError *gff_file_err ;

  gff_file_err = *gff_file_err_inout ;

  setErrMsg(server, g_strdup_printf("%s %s", server->file_path, gff_file_err->message)) ;

  g_error_free(gff_file_err) ;

  *gff_file_err_inout = NULL ;

  return ;
}



/* Process all the alignments in a context. */
static void eachAlignment(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlock, (gpointer)get_features) ;

  return ;
}



static void eachBlock(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!featuresRequest(get_features->server, get_features->parser,
			   get_features->gff_line, feature_block))
	{
	  ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, get_features->server->file_path,
			 "Could not map %s because: %s",
			 g_quark_to_string(get_features->server->req_context->sequence_name),
			 get_features->server->last_err_msg) ;
	}
    }

  return ;
}


static gboolean featuresRequest(FileServer server, ZMapGFFParser parser, GString* gff_line,
				ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  gboolean free_on_destroy = FALSE ;
  GError *gff_file_err = NULL ;
  gboolean first ;

  /* Tempting to check that block is inside features_start/end, but it may be a different
   * sequence....we don't really deal with this...  */


  /* The caller may only want a small part of the features in the file so we set the
   * feature start/end from the block, not the gff file start/end. */
  zMapGFFSetFeatureClipCoords(parser,
			      feature_block->block_to_sequence.q1,
			      feature_block->block_to_sequence.q2) ;

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
	      ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
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
		  ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR, server->file_path,
				 "%s", error->message) ;
		}
	    }
	}

      gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

    } while ((status = g_io_channel_read_line_string(server->gff_file, gff_line, &terminator_pos,
						     &gff_file_err)) == G_IO_STATUS_NORMAL) ;


  /* If we reached the end of the file then all is fine, so return features... */
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
      zMapLogWarning("Could not read GFF features from file \"%s\"", server->file_path) ;

      setLastErrorMsg(server, &gff_file_err) ;

      result = FALSE ;
    }

  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;

  return result ;
}



/* Process all the alignments in a context. */
static void eachAlignmentSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockSequence, (gpointer)get_features) ;

  return ;
}


static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    {
      gboolean sequence_finished ;


      if (zMapGFFParseSequence(get_features->parser, get_features->gff_line->str, &sequence_finished)
	  && sequence_finished)
	{
	  ZMapSequence sequence ;

	  if (!(sequence = zMapGFFGetSequence(get_features->parser)))
	    {
	      GError *error = NULL ;

	      error = zMapGFFGetError(get_features->parser);

	      setErrMsg(get_features->server,
			g_strdup_printf("zMapGFFGetSequence() failed, error=%s",
					error->message));
	      ZMAPSERVER_LOG(Warning, FILE_PROTOCOL_STR,
			     get_features->server->file_path,
			     "%s", get_features->server->last_err_msg);
	    }
	  else
	    {
	      ZMapFeatureContext context ;
	      ZMapFeatureSet feature_set ;

	      get_features->sequence_finished = TRUE ;

	      if (zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
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
    }


  return ;
}




static gboolean getServerInfo(FileServer server, ZMapServerInfo info)
{
  gboolean result = FALSE ;

  result = TRUE ;
  info->database_path = g_strdup(server->file_path) ;

  return result ;
}


/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(FileServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}




/* Tries to smap sequence into whatever its parent is, if the call fails then we set all the
 * mappings in feature_context to be something sensible...we hope....
 */
static gboolean getSequenceMapping(FileServer server, ZMapFeatureContext feature_context)
{
  gboolean result = TRUE ;
  ZMapGFFParser parser ;
  GString *gff_line ;
  gsize terminator_pos = 0 ;
  GError *gff_file_err = NULL ;
  ZMapGFFHeader header ;
  GError *error = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *parent_name = NULL, *parent_class = NULL ;
  ZMapMapBlockStruct sequence_to_parent = {0, 0, 0, 0}, parent_to_self = {0, 0, 0, 0} ;
  int parent_length = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  GIOStatus gio_status ;
  gboolean status ;


  parser = zMapGFFCreateParser() ;

  gff_line = g_string_sized_new(2000) ;			    /* Probably not many lines will be >
							       2k chars. */

  /* Read the header, needed for feature coord range. */
  while ((gio_status = g_io_channel_read_line_string(server->gff_file, gff_line,
						     &terminator_pos,
						     &gff_file_err)) == G_IO_STATUS_NORMAL)
    {
      gboolean done_header = FALSE ;

      *(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */

      if (zMapGFFParseHeader(parser, gff_line->str, &done_header))
	{
	  if (done_header)
	    break ;
	  else
	    gff_line = g_string_truncate(gff_line, 0) ;	    /* Reset line to empty. */
	}
      else
	{
	  if (!done_header)
	    {
	      error = zMapGFFGetError(parser) ;

	      if (!error)
		{
		  /* SHOULD ABORT HERE.... */
		  setErrMsg(server,
			    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
					    zMapGFFGetLineNumber(parser), gff_line->str)) ;
		  ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
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
		      setErrMsg(server,
				g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
						zMapGFFGetLineNumber(parser),
						gff_line->str)) ;
		      ZMAPSERVER_LOG(Critical, FILE_PROTOCOL_STR, server->file_path,
				     "%s", server->last_err_msg) ;
		    }
		}
	    }

	  break ;
	}
    }

  if (gio_status == G_IO_STATUS_NORMAL && result)
    {
      header = zMapGFFGetHeader(parser) ;

      addMapping(feature_context, header) ;

      zMapGFFFreeHeader(header) ;
    }


  /* Clear up. */
  zMapGFFDestroyParser(parser) ;
  g_string_free(gff_line, TRUE) ;

  return result ;
}


/* We insist that the header specifies the range of features required and we just use that
 * for all the mappings. */
static void addMapping(ZMapFeatureContext feature_context, ZMapGFFHeader header)
{
  ZMapFeatureBlock feature_block = NULL;//feature_context->master_align->blocks->data ;

  feature_block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(feature_context->master_align->blocks, 0)) ;

  /* We just override whatever is already there...this may not be the thing to do if there
   * are several files.... */
  feature_context->parent_name = feature_context->sequence_name ;

  /* In theory sequences all start at '1' so we insert that as the start of the parent, best we
   * can do really. */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  feature_context->parent_span.x1 = header->features_start ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  feature_context->parent_span.x1 = 1 ;
  feature_context->parent_span.x2 = header->features_end ;

  feature_context->sequence_to_parent.p1 = feature_context->sequence_to_parent.c1
    = feature_block->block_to_sequence.q1 = feature_block->block_to_sequence.t1 = header->features_start ;

  feature_context->sequence_to_parent.p2 = feature_context->sequence_to_parent.c2
    = feature_block->block_to_sequence.q2 = feature_block->block_to_sequence.t2 = header->features_end ;

  feature_context->length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1;

  return ;
}



/* For some data we need to go back to the start of the file and reread so this function
 * simply resets the file pointer. */
static gboolean resetToStart(FileServer server)
{
  gboolean result = FALSE ;
  GError *error = NULL ;

  if (g_io_channel_seek_position(server->gff_file, 0, G_SEEK_SET, &error) == G_IO_STATUS_NORMAL)
    {
      result = TRUE ;
    }
  else
    {
      setLastErrorMsg(server, &error) ;

      result = FALSE ;
    }

  return result ;
}



#endif
