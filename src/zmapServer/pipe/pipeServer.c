/*  File: pipeServer.c
 *  Author: Malcolm Hinsley (mh17@sanger.ac.uk)
 *      derived from pipeServer.c by Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Last edited: Nov 30 09:18 2009 (edgrif)
 * Created: 2009-11-26 12:02:40 (mh17)
 * CVS info:   $Id: pipeServer.c,v 1.3 2009-11-30 10:50:01 edgrif Exp $
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


/* Used to control getting features in all alignments in all blocks... */
typedef struct
{
  ZMapServerResponseType result ;
  PipeServer server ;
  ZMapGFFParser parser ;
  GString
* gff_line ;
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
						 GHashTable **source_2_featureset_out) ;
static ZMapServerResponseType getStyles(void *server, GData **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server,  ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GData *styles, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GData *styles, ZMapFeatureContext feature_context_out) ;
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


/* spawn a new process to run our script ans open a pipe for read to get the output
 *
 *
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

  zMapAssert(url->path) ;

  /* Always return a server struct as it contains error message stuff. */
  server = (PipeServer)g_new0(PipeServerStruct, 1) ;
  *server_out = (void *)server ;


  /* We could check both format and version here if we wanted to..... */


  /* This code is a hack to get round the fact that the url code we use in zmapView.c to parse
   * urls from our config firle will chop the leading "/" off the path....which causes the
   * zMapGetPath() call below to construct an incorrect path.... */
  {
    char *tmp_path = url->path ;

    if (*(url->path) != '/')
      tmp_path = g_strdup_printf("/%s", url->path) ;

    server->script_path = zMapGetPath(tmp_path) ;

    if (tmp_path != url->path)
      g_free(tmp_path) ;
  }


  return result ;
}


static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  if (server->gff_pipe)
    {
      zMapLogWarning("Feature script \"%s\" already active.", server->script_path) ;
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  else
    {
      GError *gff_pipe_err = NULL ;

      if ((server->gff_pipe = g_io_channel_new_file(server->script_path, "r", &gff_pipe_err)))
	{
	  result = ZMAP_SERVERRESPONSE_OK ;
	}
      else
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
      ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, server->script_path,
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
						 GHashTable **source_2_featureset_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;

  setErrMsg(server, g_strdup("Feature Sets cannot be read from GFF stream.")) ;
  ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, server->script_path,
		 "%s", server->last_err_msg) ;

  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;

  return result ;
}



/* We cannot parse the styles from a gff stream, gff simply doesn't have display styles so we
 * just return "unsupported", so if this function is called it will alert the caller that
 * something has gone wrong.
 * 
 *  */
static ZMapServerResponseType getStyles(void *server_in, GData **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  PipeServer server = (PipeServer)server_in ;

  zMapAssert(server) ;

  setErrMsg(server, g_strdup("Reading styles from a GFF stream is not supported.")) ;
  ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, server->script_path,
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
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

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


/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, GData *styles, ZMapFeatureContext feature_context)
{
  PipeServer server = (PipeServer)server_in ;
  GetFeaturesStruct get_features ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
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
  get_features.server = (PipeServer)server_in ;

  get_features.parser = zMapGFFCreateParser() ;
							    /* FALSE => do the real parse. */

  zMapGFFParserInitForFeatures(get_features.parser, styles, FALSE) ;


  get_features.gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be >
							       2k chars. */

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_pipe, get_features.gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL)
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
		  ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
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
		      ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
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
      header = zMapGFFGetHeader(get_features.parser) ;

      addMapping(feature_context, header) ;

      zMapGFFFreeHeader(header) ;


      /* Fetch all the alignment blocks for all the sequences, this all hacky right now as really.
       * we would have to parse and reparse the stream....can be done but not needed this second. */
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


static void eachBlockSequence(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    {
      ZMapSequence sequence;
      if(!(sequence = zMapGFFGetSequence(get_features->parser)))
	{
	  GError *error;
	  error = zMapGFFGetError(get_features->parser);
	  setErrMsg(get_features->server,
		    g_strdup_printf("zMapGFFGetSequence() failed, error=%s",
				    error->message));
	  ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, 
			 get_features->server->script_path,
			 "%s", get_features->server->last_err_msg);
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
  GetFeatures get_features = (GetFeatures)user_data ;

  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    g_hash_table_foreach(alignment->blocks, eachBlockSequence, (gpointer)get_features) ;

  return ;
}

/* We don't support this for now... */
static ZMapServerResponseType getContextSequence(void *server_in, GData *styles, ZMapFeatureContext feature_context_out)
{
  PipeServer server = (PipeServer)server_in ;
  GetFeaturesStruct get_features ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  GError *gff_pipe_err = NULL ;
  GError *error = NULL ;

  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (PipeServer)server_in ;

  get_features.parser = zMapGFFCreateParser() ;
							    /* FALSE => do the real parse. */

  zMapGFFParserSetSequenceFlag(get_features.parser);
  get_features.gff_line = g_string_sized_new(2000) ;	    /* Probably not many lines will be >
							       2k chars. */
  g_io_channel_seek_position(server->gff_pipe, 0, G_SEEK_SET, &gff_pipe_err);

  /* Read the header, needed for feature coord range. */
  while ((status = g_io_channel_read_line_string(server->gff_pipe, get_features.gff_line,
						 &terminator_pos,
						 &gff_pipe_err)) == G_IO_STATUS_NORMAL)
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
		  ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
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
		      ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
				     "%s", server->last_err_msg) ;		    }
		  else
		    {
		      setErrMsg(server,
				g_strdup_printf("zMapGFFParseHeader() failed for line %d: %s",
						zMapGFFGetLineNumber(get_features.parser),
						get_features.gff_line->str)) ;
		      ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
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
       * we would have to parse and reparse the stream....can be done but not needed this second. */
      g_hash_table_foreach(feature_context_out->alignments, eachAlignmentSequence, (gpointer)&get_features) ;

    }


  /* Clear up. */
  zMapGFFDestroyParser(get_features.parser) ;
  g_string_free(get_features.gff_line, TRUE) ;


  return get_features.result ;
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

  if (server->gff_pipe && g_io_channel_shutdown(server->gff_pipe, FALSE, &gff_pipe_err) != G_IO_STATUS_NORMAL)
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

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  PipeServer server = (PipeServer)server_in ;
  
  if (server->script_path)
    g_free(server->script_path) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

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

  feature_context->parent_span.x1 = header->features_start ;
  feature_context->parent_span.x2 = header->features_end ;

  /* I don't like having to do this right down here but user is allowed to specify "0" for
   * end coord meaning "to the end of the sequence" and this is where we know the end... */
  if (feature_block->block_to_sequence.t2 == 0)
    feature_block->block_to_sequence.t2 = header->features_end ;

  feature_context->sequence_to_parent.p1 = feature_context->sequence_to_parent.c1
    = feature_block->block_to_sequence.q1 = feature_block->block_to_sequence.t1 ;

  feature_context->sequence_to_parent.p2 = feature_context->sequence_to_parent.c2
    = feature_block->block_to_sequence.q2 = feature_block->block_to_sequence.t2 ;

  feature_context->length = feature_context->sequence_to_parent.c2 - feature_context->sequence_to_parent.c1 + 1;
  
  return ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_pipe_err_inout to NULL */
static void setLastErrorMsg(PipeServer server, GError **gff_pipe_err_inout)
{
  GError *gff_pipe_err ;

  zMapAssert(server && gff_pipe_err_inout && *gff_pipe_err_inout) ;

  gff_pipe_err = *gff_pipe_err_inout ;

  setErrMsg(server, g_strdup_printf("%s %s", server->script_path, gff_pipe_err->message)) ;

  g_error_free(gff_pipe_err) ;

  *gff_pipe_err_inout = NULL ;

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
      if (!sequenceRequest(get_features->server, get_features->parser,
			   get_features->gff_line, feature_block))
	{
	  ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, get_features->server->script_path,
			 "Could not map %s because: %s",
			 g_quark_to_string(get_features->server->req_context->sequence_name),
			 get_features->server->last_err_msg) ;
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
	      ZMAPSERVER_LOG(Critical, PIPE_PROTOCOL_STR, server->script_path,
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
		  ZMAPSERVER_LOG(Warning, PIPE_PROTOCOL_STR, server->script_path,
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
static void setErrMsg(PipeServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}

