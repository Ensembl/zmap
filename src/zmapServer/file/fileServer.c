/*  File: fileServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
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
 * Last edited: Sep 16 11:27 2004 (edgrif)
 * Created: Fri Sep 10 18:29:18 2004 (edgrif)
 * CVS info:   $Id: fileServer.c,v 1.2 2004-09-17 08:38:56 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapServerPrototype.h>
#include <fileServer_P.h>


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType setContext(void *server, char *sequence, int start, int end) ;
static ZMapServerResponseType request(void *server_conn, ZMapFeatureContext *feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server) ;
static gboolean destroyConnection(void *server) ;

static void addMapping(ZMapFeatureContext feature_context) ;


/* 
 *    Although these routines are static they form the external interface to the file server.
 */


/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an file server. The functions are only visible through
 * this struct. */
void fileGetServerFuncs(ZMapServerFuncs file_funcs)
{
  file_funcs->global_init = globalInit ;
  file_funcs->create = createConnection ;
  file_funcs->open = openConnection ;
  file_funcs->set_context = setContext ;
  file_funcs->request = request ;
  file_funcs->errmsg = lastErrorMsg ;
  file_funcs->close = closeConnection;
  file_funcs->destroy = destroyConnection ;

  return ;
}





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
				 char *host, int port,
				 char *userid, char *passwd, int timeout)
{
  gboolean result = TRUE ;
  FileServer server ;

  zMapAssert(host) ;

  /* Always return a server struct as it contains error message stuff. */
  server = (FileServer)g_new0(FileServerStruct, 1) ;
  *server_out = (void *)server ;

  server->file_path = zMapGetPath(host) ;

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  FileServer server = (FileServer)server_in ;

  if (server->gff_file)
    {
      zMapLogWarning("Feature file \"%s\" already open.", server->gff_file) ;
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
  if ((server->gff_file = g_io_channel_new_file(server->file_path, "r", &(server->gff_file_err))))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }

  return result ;
}


/* I'm not sure if I want to create any context here yet.... */
static ZMapServerResponseType setContext(void *server_in, char *sequence, int start, int end)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ; ;
  FileServer server = (FileServer)server_in ;

  server->sequence = g_strdup(sequence) ;
  server->start = start ;
  server->end = end ;

  return result ;
}



static ZMapServerResponseType request(void *server_in, ZMapFeatureContext *feature_context_out)
{
  ZMapServerResponseType result ;
  FileServer server = (FileServer)server_in ;
  char *file_request = NULL ;
  ZMapFeatureContext feature_context = NULL ;
  ZMapGFFParser parser ;
  GString* gff_line ;
  GIOStatus status ;
  gsize terminator_pos = 0 ;
  gboolean free_arrays = FALSE, parse_only = FALSE ;


  result = ZMAP_SERVERRESPONSE_OK ;

  /* Lash up for now, we will later need a  "ZMap request" -> "file request" translator
   * to deal with features etc. etc. */


  gff_line = g_string_sized_new(2000) ;		    /* Probably not many lines will be >
						       2k chars. */

  parser = zMapGFFCreateParser(parse_only) ;

  while ((status = g_io_channel_read_line_string(server->gff_file, gff_line, &terminator_pos,
						 &server->gff_file_err)) == G_IO_STATUS_NORMAL)
    {

      *(gff_line->str + terminator_pos) = '\0' ;	    /* Remove terminating newline. */


      if (!zMapGFFParseLine(parser, gff_line->str))
	{
	  GError *error = zMapGFFGetError(parser) ;

	  if (!error)
	    {
	      zMapLogCritical("parser failed, but did not set a GError for line %d: %s",
			      zMapGFFGetLineNumber(parser), gff_line->str) ;
	    }
	  else
	    zMapLogWarning("%s", (zMapGFFGetError(parser))->message) ;
	}
	  
	  
      gff_line = g_string_truncate(gff_line, 0) ;   /* Reset line to empty. */

    }


  /* If we reached the end of the file then all is fine, otherwise so return features... */
  if (status == G_IO_STATUS_EOF)
    {
      feature_context = g_new0(ZMapFeatureContextStruct, 1) ;
      zmapGFFGetFeatures(parser, feature_context) ;	    /* we ignore success/failure of call... */

      addMapping(feature_context) ;

      *feature_context_out = feature_context ;
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      zMapLogWarning("Could not read GFF features from file \"%s\"", server->gff_file) ;
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }
      

  zMapGFFSetFreeOnDestroy(parser, free_arrays) ;
  zMapGFFDestroyParser(parser) ;

  return result ;
}


/* slightly contorted as we potentially have GErrors from various glib ops. but also our
 * own errors, basically we always want to give the user back a string that they have to free. */
char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  FileServer server = (FileServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;
  else if (server->gff_file_err)
    {
      server->last_err_msg = g_strdup(server->gff_file_err->message) ;
      err_msg = server->last_err_msg ;
    }

  return err_msg ;
}


static gboolean closeConnection(void *server_in)
{
  gboolean result = ZMAP_SERVERRESPONSE_OK ;
  FileServer server = (FileServer)server_in ;

  if (g_io_channel_shutdown(server->gff_file, FALSE, &(server->gff_file_err)) != G_IO_STATUS_NORMAL)
    {
      zMapLogCritical("Could not close feature file \"%s\"", server->gff_file) ;
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
    }

  /* this seems to be required to destroy the GIOChannel.... */
  g_io_channel_unref(server->gff_file) ;

  server->gff_file = NULL ;

  return result ;
}

static gboolean destroyConnection(void *server_in)
{
  gboolean result = TRUE ;
  FileServer server = (FileServer)server_in ;
  
  if (server->file_path)
    g_free(server->file_path) ;

  if (server->gff_file_err)
    g_error_free(server->gff_file_err) ;

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
static void addMapping(ZMapFeatureContext feature_context)
{

  feature_context->parent = g_strdup(feature_context->sequence) ;

  feature_context->parent_span.x1
    = feature_context->sequence_to_parent.p1 = feature_context->sequence_to_parent.c1
    = feature_context->features_to_sequence.p1 ;

  feature_context->parent_span.x2
    = feature_context->sequence_to_parent.p2 = feature_context->sequence_to_parent.c2
    = feature_context->features_to_sequence.p2 ;
  
  return ;
}
