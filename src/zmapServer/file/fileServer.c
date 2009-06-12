/*  File: fileServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 * Description: These functions provide code to read a file as though
 *              it were a server according to the interface defined
 *              for accessing servers. It's crude at the moment but
 *              the aim is to allow zmap to be run from just files
 *              if required.
 *              
 * Exported functions: See ZMap/zmapServerPrototype.h
 * HISTORY:
 * Last edited: Jun 12 10:52 2009 (edgrif)
 * Created: Fri Sep 10 18:29:18 2004 (edgrif)
 * CVS info:   $Id: fileServer.c,v 1.38 2009-06-12 13:57:07 edgrif Exp $
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
#include <fileServer_P.h>


/* Used to control getting features in all alignments in all blocks... */
typedef struct
{
  ZMapServerResponseType result ;
  FileServer server ;
  ZMapGFFParser parser ;
  GString* gff_line ;
} GetFeaturesStruct, *GetFeatures ;



static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format, 
                                 char *version_str, int timeout) ;

static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
						 GList **feature_sets_out,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_inout) ;
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
static gboolean sequenceRequest(FileServer server, ZMapGFFParser parser, GString* gff_line,
				ZMapFeatureBlock feature_block) ;
static void setLastErrorMsg(FileServer server, GError **gff_file_err_inout) ;

static gboolean getServerInfo(FileServer server, ZMapServerInfo info) ;
static void setErrMsg(FileServer server, char *new_msg) ;




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

  zMapAssert(url->path) ;

  /* Always return a server struct as it contains error message stuff. */
  server = (FileServer)g_new0(FileServerStruct, 1) ;
  *server_out = (void *)server ;


  /* We could check both format and version here if we wanted to..... */


  /* This code is a hack to get round the fact that the url code we use in zmapView.c to parse
   * urls from our config file will chop the leading "/" off the file path....which causes the
   * zMapGetPath() call below to construct an incorrect path.... */
  {
    char *tmp_path = url->path ;

    if (*(url->path) != '/')
      tmp_path = g_strdup_printf("/%s", url->path) ;

    server->file_path = zMapGetPath(tmp_path) ;

    if (tmp_path != url->path)
      g_free(tmp_path) ;
  }


  return result ;
}


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
						 GList **required_styles_out,
						 GHashTable **featureset_2_stylelist_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  zMapAssert(server) ;

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

  zMapAssert(server) ;

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


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}




/* We don't check anything here, we could look in the file to check that it matches the context
 * I guess which is essentially what we do for the acedb server. */
static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;

  server->req_context = feature_context ;


  /* Like the acedb server we should be getting the mapping from the gff file here....
   * then we could set up the block/etc stuff...this is where add mapping should be called from... */



  return result ;
}


/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, GData *styles, ZMapFeatureContext feature_context)
{
  FileServer server = (FileServer)server_in ;
  GetFeaturesStruct get_features ;
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

  get_features.parser = zMapGFFCreateParser(styles, FALSE) ;
							    /* FALSE => do the real parse. */


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
      header = zMapGFFGetHeader(get_features.parser) ;

      addMapping(feature_context, header) ;

      zMapGFFFreeHeader(header) ;


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



/* We don't support this for now... */
static ZMapServerResponseType getContextSequence(void *server_in, GData *styles, ZMapFeatureContext feature_context_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;


  result = ZMAP_SERVERRESPONSE_UNSUPPORTED ;


  return result ;
}


/* Return the last error message. */
static char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  FileServer server = (FileServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;

  return err_msg ;
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


/* A bit of a lash up for now, we need the parent->child mapping for a sequence and since
 * the code in this file so far simply reads a GFF file for now, we just fake it by setting
 * everything to be the same for child/parent... */
static void addMapping(ZMapFeatureContext feature_context, ZMapGFFHeader header)
{
  ZMapFeatureBlock feature_block = NULL;//feature_context->master_align->blocks->data ;

  feature_block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(feature_context->master_align->blocks, 0)) ;

  /* We just override whatever is already there...this may not be the thing to do if there
   * are several files.... */
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
  
  return ;
}



/* Small utility to deal with error messages from the GIOchannel package.
 * Note that we set gff_file_err_inout to NULL */
static void setLastErrorMsg(FileServer server, GError **gff_file_err_inout)
{
  GError *gff_file_err ;

  zMapAssert(server && gff_file_err_inout && *gff_file_err_inout) ;

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
      if (!sequenceRequest(get_features->server, get_features->parser,
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



static gboolean sequenceRequest(FileServer server, ZMapGFFParser parser, GString* gff_line,
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

