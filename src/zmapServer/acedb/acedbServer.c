/*  File: zmapServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapServer.h
 * HISTORY:
 * Last edited: Oct  4 10:52 2004 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: acedbServer.c,v 1.12 2004-10-04 12:55:42 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <AceConn.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapServerPrototype.h>
#include <acedbServer_P.h>


static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType setContext(void *server, char *sequence, int start, int end) ;
static ZMapServerResponseType request(void *server_conn, ZMapFeatureContext *feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static gboolean destroyConnection(void *server) ;

static gboolean sequenceRequest(AcedbServer server, ZMapFeatureContext feature_context) ;
static gboolean getSequenceMapping(AcedbServer server, ZMapFeatureContext feature_context) ;
static gboolean getSMapping(AcedbServer server, char *class,
			    char *sequence, int start, int end,
			    char **parent_class_out, char **parent_name_out,
			    ZMapMapBlock child_to_parent_out) ;
static gboolean getSMapLength(AcedbServer server, char *obj_class, char *obj_name,
			      int *obj_length_out) ;


/* 
 *    Although these routines are static they form the external interface to the acedb server.
 */


/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an acedb server. The functions are only visible through
 * this struct. */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs)
{
  acedb_funcs->global_init = globalInit ;
  acedb_funcs->create = createConnection ;
  acedb_funcs->open = openConnection ;
  acedb_funcs->set_context = setContext ;
  acedb_funcs->request = request ;
  acedb_funcs->errmsg = lastErrorMsg ;
  acedb_funcs->close = closeConnection;
  acedb_funcs->destroy = destroyConnection ;

  return ;
}





/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;



  return result ;
}



static gboolean createConnection(void **server_out,
				 char *host, int port,
				 char *userid, char *passwd, int timeout)
{
  gboolean result = FALSE ;
  AcedbServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (AcedbServer)g_new0(AcedbServerStruct, 1) ;
  server->last_err_status = ACECONN_OK ;
  *server_out = (void *)server ;

  if ((server->last_err_status =
       AceConnCreate(&(server->connection), host, port, userid, passwd, timeout)) == ACECONN_OK)
    result = TRUE ;

  return result ;
}


static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status = AceConnConnect(server->connection)) == ACECONN_OK)
    result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}


/* I'm not sure if I need to create a context in the server for acedb, really it would be a keyset
 * or a virtual sequence...don't know if we can do this via gif interface.... */
static ZMapServerResponseType setContext(void *server_in, char *sequence, int start, int end)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  AcedbServer server = (AcedbServer)server_in ;
  gboolean status ;
  ZMapFeatureContext feature_context ;

  server->sequence = g_strdup(sequence) ;
  server->start = start ;
  server->end = end ;


  feature_context = g_new0(ZMapFeatureContextStruct, 1) ;

  if (!(status = getSequenceMapping(server, feature_context)))
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      zMapLogWarning("Could not map %s because: %s", server->sequence, server->last_err_msg) ;
      g_free(feature_context) ;
    }
  else
    server->current_context = feature_context ;

  return result ;
}



static ZMapServerResponseType request(void *server_in, ZMapFeatureContext *feature_context_out)
{
  ZMapServerResponseType result ;
  AcedbServer server = (AcedbServer)server_in ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  gboolean status ;
  ZMapFeatureContext feature_context ;


  /* We should check that there is a feature context here and report an error if there isn't... */

  result = ZMAP_SERVERRESPONSE_OK ;
  server->last_err_status = ACECONN_OK ;

  /* We should be using the start/end info. in context for the below stuff... */

  feature_context = g_new0(ZMapFeatureContextStruct, 1) ;


  *feature_context = *(server->current_context) ;	    /* n.b. struct copy. */

  if ((status = sequenceRequest(server, feature_context)))
    {
      *feature_context_out = feature_context ;
    }
  else
    {
      /* If the call failed it may be that the connection failed or that the data coming
       * back had a problem. */
      if (server->last_err_status == ACECONN_OK)
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
      else if (server->last_err_status == ACECONN_TIMEDOUT)
	{
	  result = ZMAP_SERVERRESPONSE_TIMEDOUT ;
	}
      else
	{
	  /* Probably we will want to analyse the response more than this ! */
	  result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	}

      zMapLogWarning("Could not map %s because: %s", server->sequence, server->last_err_msg) ;
    }

  /* We need a feature context call here which will free stuff correctly...this is not right
     * as there are features etc to do....need a set of feature context calls...... */
  if (!status)
    g_free(feature_context) ;

  return result ;
}



char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  AcedbServer server = (AcedbServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;
  else if (server->last_err_status != ACECONN_OK)
    err_msg = g_strdup(AceConnGetErrorMsg(server->connection, server->last_err_status)) ;

  return err_msg ;
}


static ZMapServerResponseType closeConnection(void *server_in)
{
  gboolean result = TRUE ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status = AceConnConnectionOpen(server->connection)) == ACECONN_OK)
    {
      if ((server->last_err_status = AceConnDisconnect(server->connection)) != ACECONN_OK)
	result = FALSE ;
    }

  return result ;
}

static gboolean destroyConnection(void *server_in)
{
  gboolean result = TRUE ;
  AcedbServer server = (AcedbServer)server_in ;

  AceConnDestroy(server->connection) ;			    /* Does not fail. */
  server->connection = NULL ;				    /* Prevents accidental reuse. */

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  g_free(server) ;

  return result ;
}



/* 
 * ---------------------  Internal routines.  ---------------------
 */



/* bit of an issue over returning error messages here.....sort this out as some errors many be
 * aceconn errors, others may be data processing errors, e.g. in GFF etc., e.g.
 * 
 * If we issue a command like this: "gif seqget obj ; seqfeatures" then we have a problem because
 * the server treats this as one command and returns any errors from the seqget() munged on the
 * front of the GFF output from the seqfeatures() like this:
 * 
 * // ERROR -  Sequence:"yk47h9.3" (5' match) and Sequence:"yk47h9.3" (3' match) are in wrong orientation.
 * // ERROR -  Sequence:"AW057380" (5' match) and Sequence:"AW057380" (3' match) are in wrong orientation.
 * // ERROR -  Sequence:"OSTR010G5_1" (5' match) and Sequence:"OSTR010G5_1" (3' match) are in wrong orientation.
 * ##gff-version 2
 * ##source-version sgifaceserver:ACEDB 4.9.27
 * ##date 2004-09-21
 * ##sequence-region F22D3 1 35712 
 * F22D3	Genomic_canonical	region	1	200	.	+	.	Sequence "B0252"
 * 
 * so what to do...agh....
 * 
 * I guess the best thing is to shove the errors out to the log and look for the gff start...
 * 
 */
static gboolean sequenceRequest(AcedbServer server, ZMapFeatureContext feature_context)
{
  gboolean result = FALSE ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  gboolean found_gff_start ;

  /* Here we can convert the GFF that comes back, in the end we should be doing a number of
   * calls to AceConnRequest() as the server slices...but that will need a change to my
   * AceConn package.....
   * We make the big assumption that what comes back is a C string for now, this is true
   * for most acedb requests, only images/postscript are not and we aren't asking for them. */

  acedb_request =  g_strdup_printf("gif seqget %s -coords %d %d ; seqfeatures",
				   server->sequence, server->start, server->end) ;

  server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len) ;
    
  if (server->last_err_status == ACECONN_OK)
    {
      char *next_line ;
      ZMapReadLine line_reader ;
      gboolean inplace = TRUE ;


      line_reader = zMapReadLineCreate((char *)reply, inplace) ;


      /* Look for "##gff-version" at start of line which signals start of GFF, as detailed above
       * there may be errors before the GFF output. */
      result = TRUE ;
      do
	{
	  if (!(result = zMapReadLineNext(line_reader, &next_line)))
	    {
	      /* If the readline fails it may be because of an error or because its reached the
	       * end, if next_line is empty then its reached the end. */
	      if (*next_line)
		server->last_err_msg = g_strdup_printf("Request from server contained incomplete line: %s",
						       next_line) ;
	      else
		server->last_err_msg = g_strdup_printf("%s", "No GFF found in server reply.") ;

	      zMapLogCritical("%s", server->last_err_msg) ;
	    }
	  else
	    {
	      if (g_str_has_prefix((char *)next_line, "##gff-version"))
		{
		  break ;
		}
	      else
		{
		  server->last_err_msg = g_strdup_printf("Bad GFF line: %s",
							 next_line) ;
		  zMapLogCritical("%s", server->last_err_msg) ;
		}
	    }
	}
      while (result && *next_line) ;


      if (result)
	{
	  ZMapGFFParser parser ;
	  gboolean free_on_destroy ;
      

	  parser = zMapGFFCreateParser(FALSE) ;


	  /* We probably won't have to deal with part lines here acedb should only return whole lines
	   * ....but should check for sure...bomb out for now.... */
	  result = TRUE ;
	  do
	    {
	      /* Note that we already have the first line from the loop above. */
	      if (!zMapGFFParseLine(parser, next_line))
		{
		  /* This is a hack, I would like to make the acedb code have a quiet mode but
		   * as usual this is not straight forward and will take a bit of fixing...
		   * The problem for us is that the gff output is terminated with with a couple
		   * of acedb comment lines which then screw up our parsing....so we ignore
		   * lines starting with "//" hoping this doesn't conflict with gff.... */
		  if (!g_str_has_prefix(next_line, "//"))
		    {
		      GError *error = zMapGFFGetError(parser) ;

		      if (!error)
			{
			  server->last_err_msg =
			    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
					    zMapGFFGetLineNumber(parser), next_line) ;
			  zMapLogCritical("%s", server->last_err_msg) ;

			  result = FALSE ;
			}
		      else
			{
			  /* If the error was serious we stop processing and return the error,
			   * otherwise we just log the error. */
			  if (zMapGFFTerminated(parser))
			    {
			      result = FALSE ;
			      server->last_err_msg =
				g_strdup_printf("%s", error->message) ;
			    }
			  else
			    {
			      zMapLogWarning("%s", error->message) ;
			    }
			}
		    }
		}


	      if (!(result = zMapReadLineNext(line_reader, &next_line)))
		{
		  /* If the readline fails it may be because of an error or because its reached the
		   * end, if next_line is empty then its reached the end. */
		  if (*next_line)
		    {
		      server->last_err_msg = g_strdup_printf("Request from server contained incomplete line: %s",
							     next_line) ;
		      zMapLogCritical("%s", server->last_err_msg) ;
		    }
		  else
		    result = TRUE ;
		}

	    }
	  while (result && *next_line) ;

	  free_on_destroy = TRUE ;
	  if (result)
	    {
	      if (zmapGFFGetFeatures(parser, feature_context))
		{
		  free_on_destroy = FALSE ;			    /* Make sure parser does _not_ free our
								       data. ! */
		}
	      else
		result = FALSE ;
	    }

	  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;
	  zMapGFFDestroyParser(parser) ;
	}

      zMapReadLineDestroy(line_reader, FALSE) ;		    /* n.b. don't free string as it is the
							       same as reply which is freed later.*/
    }


  g_free(acedb_request) ;
  g_free(reply) ;

  return result ;
}


/* Tries to smap sequence into whatever its parent is, if the call fails then we set all the
 * mappings in feature_context to be something sensible...we hope....
 */
static gboolean getSequenceMapping(AcedbServer server, ZMapFeatureContext feature_context)
{
  gboolean result = FALSE ;
  char *parent_name = NULL, *parent_class = NULL ;
  ZMapMapBlockStruct sequence_to_parent = {0, 0, 0, 0}, parent_to_self = {0, 0, 0, 0} ;
  int parent_length = 0 ;

  if (getSMapping(server, NULL, server->sequence, server->start, server->end,
		  &parent_class, &parent_name, &sequence_to_parent)
      && getSMapLength(server, parent_class, parent_name, &parent_length))
    {
      parent_to_self.p1 = parent_to_self.c1 = 1 ;
      parent_to_self.p2 = parent_to_self.c2 = parent_length ;

      feature_context->parent = parent_name ;		    /* No need to copy, has been allocated. */

      if (feature_context->sequence_to_parent.p1 < feature_context->sequence_to_parent.p2)
	{
	  feature_context->parent_span.x1 = parent_to_self.p1 ;
	  feature_context->parent_span.x2 = parent_to_self.p2 ;
	}
      else
	{
	  feature_context->parent_span.x1 = parent_to_self.p2 ;
	  feature_context->parent_span.x2 = parent_to_self.p1 ;
	}

      feature_context->sequence_to_parent.c1 = sequence_to_parent.c1 ;
      feature_context->sequence_to_parent.c2 = sequence_to_parent.c2 ;
      feature_context->sequence_to_parent.p1 = sequence_to_parent.p1 ;
      feature_context->sequence_to_parent.p2 = sequence_to_parent.p2 ;
     
      result = TRUE ;
    }

  return result ;
}



/* Gets the mapping of a sequence object into its ultimate SMap parent, uses the
 * smap and smaplength commands to do this. Note that we currently assume that the
 * object is of the sequence class. If the smap call succeeds then returns TRUE,
 * FALSE otherwise (server->last_err_msg is set to show the problem).
 * 
 * To do this we issue the smap request to the server:
 * 
 *        gif smap -from sequence:<seq_name>
 * 
 * and receive a reply in the form:
 * 
 *        SMAP Sequence:<parent_name> 10995378 11034593 1 39216 PERFECT_MAP
 *        // 0 Active Objects
 * 
 * on error we get something like:
 * 
 *        // gif smap error: invalid from object
 *        // 0 Active Objects
 * 
 */
static gboolean getSMapping(AcedbServer server, char *class,
			    char *sequence, int start, int end,
			    char **parent_class_out, char **parent_name_out,
			    ZMapMapBlock child_to_parent_out)
{
  gboolean result = FALSE ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  acedb_request = g_strdup_printf("gif smap -coords %d %d -from sequence:%s",
				  start, end, sequence) ;

  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len))
      == ACECONN_OK)
    {
      char *reply_text = (char *)reply ;

      /* this is a hack, none of the acedb commands return an error code so the only way you know
       * if anything has gone wrong is to examine the contents of the text that is returned...
       * in our case if the reply starts with acedb comment chars, this means trouble.. */
      if (g_str_has_prefix(reply_text, "//"))
	{
	  server->last_err_msg = g_strdup_printf("SMap request failed, "
						 "request: \"%s\",  "
						 "reply: \"%s\"", acedb_request, reply_text) ;
	  result = FALSE ;
	}
      else
	{
	  enum {FIELD_BUFFER_LEN = 257} ;
	  char parent_class[FIELD_BUFFER_LEN] = {'\0'}, parent_name[FIELD_BUFFER_LEN] = {'\0'} ;
	  char status[FIELD_BUFFER_LEN] = {'\0'} ;
	  ZMapMapBlockStruct child_to_parent = {0, 0, 0, 0} ;
	  enum {FIELD_NUM = 7} ;
	  char *format_str = "SMAP %256[^:]:%256s%d%d%d%d%256s" ;
	  int fields ;
	  
	  if ((fields = sscanf(reply_text, format_str, &parent_class[0], &parent_name[0],
			       &child_to_parent.p1, &child_to_parent.p2,
			       &child_to_parent.c1, &child_to_parent.c2,
			       &status[0])) != FIELD_NUM)
	    {
	      server->last_err_msg = g_strdup_printf("Could not parse smap data, "
						     "request: \"%s\",  "
						     "reply: \"%s\"", acedb_request, reply_text) ;
	      result = FALSE ;
	    }
	  else
	    {
	      /* Really we need to do more than this, we should check the status of the mapping. */

	      if (parent_class_out)
		*parent_class_out = g_strdup(&parent_class[0]) ;

	      if (parent_name_out)
		*parent_name_out = g_strdup(&parent_name[0]) ;

	      if (child_to_parent_out)
		*child_to_parent_out = child_to_parent ;    /* n.b. struct copy. */

	      result = TRUE ;
	    }
	}

      g_free(reply) ;
    }

  return result ;
}




/* Makes smaplength request to server for a given object. If the smap call succeeds
 * then returns TRUE, FALSE otherwise (server->last_err_msg is set to show the problem).
 * 
 * To do this we issue the smap request to the server:
 * 
 *        smaplength Sequence:CHROMOSOME_V
 * 
 * and receive a reply in the form:
 * 
 *        SMAPLENGTH Sequence:"CHROMOSOME_V" 20922231
 *        // 0 Active Objects
 * 
 * on error we get something like:
 * 
 *        // smaplength error: object "Sequence:CHROMOSOME_V" does not exist.
 *        // 0 Active Objects
 * 
 *  */
static gboolean getSMapLength(AcedbServer server, char *obj_class, char *obj_name,
			      int *obj_length_out)
{
  gboolean result = FALSE ;
  char *command = "smaplength" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  acedb_request =  g_strdup_printf("%s %s:%s", command, obj_class, obj_name) ;

  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len))
      == ACECONN_OK)
    {
      char *reply_text = (char *)reply ;

      /* this is a hack, none of the acedb commands return an error code so the only way you know
       * if anything has gone wrong is to examine the contents of the text that is returned...
       * in our case if the reply starts with acedb comment chars, this means trouble.. */
      if (g_str_has_prefix(reply_text, "//"))
	{
	  server->last_err_msg = g_strdup_printf("%s request failed, "
						 "request: \"%s\",  "
						 "reply: \"%s\"", command, acedb_request, reply_text) ;
	  result = FALSE ;
	}
      else
	{
	  enum {FIELD_BUFFER_LEN = 257} ;
	  char obj_class_name[FIELD_BUFFER_LEN] = {'\0'} ;
	  int length = 0 ;
	  enum {FIELD_NUM = 2} ;
	  char *format_str = "SMAPLENGTH %256s%d" ;
	  int fields ;
	  
	  if ((fields = sscanf(reply_text, format_str, &obj_class_name[0], &length)) != FIELD_NUM)
	    {
	      server->last_err_msg = g_strdup_printf("Could not parse smap data, "
						     "request: \"%s\",  "
						     "reply: \"%s\"", acedb_request, reply_text) ;
	      result = FALSE ;
	    }
	  else
	    {
	      if (obj_length_out)
		*obj_length_out = length ;

	      result = TRUE ;
	    }
	}

      g_free(reply) ;
    }

  return result ;
}




