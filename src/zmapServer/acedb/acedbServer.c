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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapServer.h
 * HISTORY:
 * Last edited: Apr  6 17:47 2006 (rds)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: acedbServer.c,v 1.51 2006-04-06 16:47:27 rds Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <AceConn.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGFF.h>
#include <zmapServerPrototype.h>
#include <acedbServer_P.h>

typedef struct
{
  gboolean first_method ;
  gboolean find_string ;
  gboolean name_list ;
  GString *str ;
} ZMapTypesStringStruct, *ZMapTypesString ;


typedef struct
{
  ZMapServerResponseType result ;
  AcedbServer server ;
  GFunc eachBlock;
} DoAllAlignBlocksStruct, *DoAllAlignBlocks ;


typedef struct
{
  char *name ;
  char *spec ;
} AcedbColourSpecStruct, *AcedbColourSpec ;


/* These provide the interface functions for an acedb server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 zMapURL url, char *format, 
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType getTypes(void *server, GList *requested_types, GList **types) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context) ;
ZMapFeatureContext copyContext(void *server) ;
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static gboolean destroyConnection(void *server) ;


/* general internal routines. */
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string) ;
static void addTypeName(gpointer data, gpointer user_data) ;
static gboolean sequenceRequest(AcedbServer server, ZMapFeatureBlock feature_block) ;
static gboolean dnaRequest(AcedbServer server, ZMapFeatureBlock feature_block) ;
static gboolean getSequenceMapping(AcedbServer server, ZMapFeatureContext feature_context) ;
static gboolean getSMapping(AcedbServer server, char *class,
			    char *sequence, int start, int end,
			    char **parent_class_out, char **parent_name_out,
			    ZMapMapBlock child_to_parent_out) ;
static gboolean getSMapLength(AcedbServer server, char *obj_class, char *obj_name,
			      int *obj_length_out) ;
static gboolean checkServerVersion(AcedbServer server) ;
static gboolean findSequence(AcedbServer server) ;
static gboolean setQuietMode(AcedbServer server) ;
static gboolean parseTypes(AcedbServer server, GList *requested_types, GList **types_out) ;
ZMapFeatureTypeStyle parseMethod(GList *requested_types, char *method_str, char **end_pos) ;
gint resortStyles(gconstpointer a, gconstpointer b, gpointer user_data) ;
int getFoundObj(char *text) ;

static void eachAlignment(GQuark key_id, gpointer data, gpointer user_data) ;

static void eachBlockSequenceRequest(gpointer data, gpointer user_data) ;
static void eachBlockDNARequest(gpointer data, gpointer user_data);

static char *getAcedbColourSpec(char *acedb_colour_name) ;



/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an acedb server. The functions are only visible through
 * this struct. */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs)
{
  acedb_funcs->global_init = globalInit ;
  acedb_funcs->create = createConnection ;
  acedb_funcs->open = openConnection ;
  acedb_funcs->get_types = getTypes ;
  acedb_funcs->set_context = setContext ;
  acedb_funcs->copy_context = copyContext ;
  acedb_funcs->get_features = getFeatures ;
  acedb_funcs->get_sequence = getSequence ;
  acedb_funcs->errmsg = lastErrorMsg ;
  acedb_funcs->close = closeConnection;
  acedb_funcs->destroy = destroyConnection ;

  return ;
}



/* 
 *    Although these routines are static they form the external interface to the acedb server.
 */


/* For stuff that just needs to be done once at the beginning..... */
static gboolean globalInit(void)
{
  gboolean result = TRUE ;

  return result ;
}



static gboolean createConnection(void **server_out,
				 zMapURL url, char *format, 
                                 char *version_str, int timeout)
{
  gboolean result = FALSE ;
  AcedbServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (AcedbServer)g_new0(AcedbServerStruct, 1) ;
  server->last_err_status = ACECONN_OK ;

  server->host = g_strdup(url->host) ;

  server->user_specified_styles = TRUE ;

  /* We need a minimum server version but user can specify a higher one in the config file. */
  if (version_str)
    {
      if (zMapCompareVersionStings(ACEDB_SERVER_MIN_VERSION, version_str))
	server->version_str = g_strdup(version_str) ;
      else
	{
	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
			 "Requested server version was %s but minimum supported is %s.",
			 version_str, ACEDB_SERVER_MIN_VERSION) ;
	  server->version_str = g_strdup(ACEDB_SERVER_MIN_VERSION) ;
	}
    }
  else
    server->version_str = g_strdup(ACEDB_SERVER_MIN_VERSION) ;

  *server_out = (void *)server ;

  if ((server->last_err_status =
       AceConnCreate(&(server->connection), server->host, url->port, url->user, url->passwd, timeout)) == ACECONN_OK)
    result = TRUE ;

  return result ;
}


/* When we open the connection we not only check the acedb version of the server but also
 * set "quiet" mode on so that we can get dna, gff and other stuff back unadulterated by
 * extraneous information. */
static ZMapServerResponseType openConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  if ((server->last_err_status = AceConnConnect(server->connection)) == ACECONN_OK)
    {
      if (checkServerVersion(server) && setQuietMode(server))
	result = ZMAP_SERVERRESPONSE_OK ;
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
			 "Could open connection because: %s", server->last_err_msg) ;
	}
    }

  return result ;
}


static ZMapServerResponseType getTypes(void *server_in, GList *requested_types, GList **types_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  if (requested_types)
    {
      server->method_str = getMethodString(requested_types, TRUE, FALSE) ;
      server->find_method_str = getMethodString(requested_types, TRUE, TRUE) ;
    }
  else
    server->user_specified_styles = FALSE ;


  if (parseTypes(server, requested_types, types_out))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not get types from server because: %s", server->last_err_msg) ;
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Sadly not all types of data (e.g. Allele, clone_end etc.) in acedb have a method, many
   * are hard-coded which makes our job v. much harder. These types must be added to the
   * requested_types and types_out as appropriate. */
  if (result == ZMAP_SERVERRESPONSE_OK)
    {
      if (requested_types)
	{




	}

      if (types_out)
	{





	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  return result ;
}



/* the struct/param handling will not work in these routines now and needs sorting out.... */

static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  AcedbServer server = (AcedbServer)server_in ;
  gboolean status ;

  server->req_context = feature_context ;


  /*  HERE WE NEED TO ACCEPT A PASSED IN FEATURE CONTEXT AND THEN COPY IT AND FILL IT IN,
      THE COPY CONTEXT NEEDS TO HAPPEN HERE.....*/
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  feature_context = zMapFeatureContextCreate(server->sequence) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (!(status = getSequenceMapping(server, feature_context)))
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not map %s because: %s",
		     g_quark_to_string(server->req_context->sequence_name), server->last_err_msg) ;
      g_free(feature_context) ;
    }
  else
    server->current_context = feature_context ;

  return result ;
}



ZMapFeatureContext copyContext(void *server_in)
{
  AcedbServer server = (AcedbServer)server_in ;
  ZMapFeatureContext feature_context ;

  /* This should be via a feature context "copy" constructor..... */
  feature_context = g_new0(ZMapFeatureContextStruct, 1) ;

  /* this won't be nearly enough...need a deep copy... */
  *feature_context = *(server->current_context) ;	    /* n.b. struct copy. */

  return feature_context ;
}




/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context)
{
  DoAllAlignBlocksStruct get_features ;

  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (AcedbServer)server_in ;
  get_features.server->last_err_status = ACECONN_OK ;
  get_features.eachBlock = eachBlockSequenceRequest;

  /* Fetch all the alignment blocks for all the sequences. */
  g_datalist_foreach(&(feature_context->alignments), eachAlignment, (gpointer)&get_features) ;

  return get_features.result ;
}


/* Get features and/or sequence. */
static ZMapServerResponseType getSequence(void *server_in, ZMapFeatureContext feature_context)
{
  DoAllAlignBlocksStruct get_sequence ;

  get_sequence.result = ZMAP_SERVERRESPONSE_OK;
  get_sequence.server = (AcedbServer)server_in;
  get_sequence.server->last_err_status = ACECONN_OK;
  get_sequence.eachBlock = eachBlockDNARequest;

  g_datalist_foreach(&(feature_context->alignments), eachAlignment, (gpointer)&get_sequence);
  
  return get_sequence.result;
}

static void eachBlockDNARequest(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  DoAllAlignBlocks get_sequence = (DoAllAlignBlocks)user_data ;
#ifdef RDS_DONT_INCLUDE
  ZMapServerResponseType result ;
  AcedbServer server = (AcedbServer)server_in ;
  result = ZMAP_SERVERRESPONSE_OK ;
  server->last_err_status = ACECONN_OK ;
#endif
  /* We should check that there is a sequence context here and report an error if there isn't... */


  /* We should be using the start/end info. in context for the below stuff... */
  if (!dnaRequest(get_sequence->server, feature_block))
    {
      /* If the call failed it may be that the connection failed or that the data coming
       * back had a problem. */
      if (get_sequence->server->last_err_status == ACECONN_OK)
	{
	  get_sequence->result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
      else if (get_sequence->server->last_err_status == ACECONN_TIMEDOUT)
	{
	  get_sequence->result = ZMAP_SERVERRESPONSE_TIMEDOUT ;
	}
      else
	{
	  /* Probably we will want to analyse the response more than this ! */
	  get_sequence->result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	}

      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, get_sequence->server->host,
		     "Could not map %s because: %s",
		     g_quark_to_string(get_sequence->server->req_context->sequence_name),
		     get_sequence->server->last_err_msg) ;
    }


  return ;
}



char *lastErrorMsg(void *server_in)
{
  char *err_msg = NULL ;
  AcedbServer server = (AcedbServer)server_in ;

  zMapAssert(server_in) ;

  if (server->last_err_msg)
    err_msg = server->last_err_msg ;
  else if (server->last_err_status != ACECONN_OK)
    server->last_err_msg = err_msg = 
      g_strdup(AceConnGetErrorMsg(server->connection, server->last_err_status)) ;

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

  g_free(server->host) ;

  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  if (server->version_str)
    g_free(server->version_str) ;

  g_free(server) ;

  return result ;
}




/* 
 * ---------------------  Internal routines.  ---------------------
 */


/* Make up a string that contains method names in the correct format for an acedb "Find" command
 * (find_string == TRUE) or to be part of an acedb "seqget" command.
 * We may be passed either a list of style names in GQuark form (style_name_list == TRUE)
 * or a list of the actual styles. */
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string)
{
  char *type_names = NULL ;
  ZMapTypesStringStruct types_data ;
  GString *str ;
  gboolean free_string = TRUE ;

  zMapAssert(styles_or_style_names) ;

  str = g_string_new("") ;

  if (find_string)
    str = g_string_append(str, "query find method ") ;
  else
    str = g_string_append(str, "+method ") ;

  types_data.first_method = TRUE ;
  types_data.find_string = find_string ;
  types_data.name_list = style_name_list ;
  types_data.str = str ;

  g_list_foreach(styles_or_style_names, addTypeName, (void *)&types_data) ;

  if (*(str->str))
    free_string = FALSE ;

  type_names = g_string_free(str, free_string) ;


  return type_names ;
}


/* GFunc() callback function, appends style names to a string, its called for lists
 * of either style name GQuarks or lists of style structs. */
static void addTypeName(gpointer data, gpointer user_data)
{
  char *type_name = NULL ;
  GQuark key_id ;
  ZMapTypesString types_data = (ZMapTypesString)user_data ;

  /* We might be passed either a list of style names (as quarks) or a list of the actual styles
   * from which we need to extract the style name. */
  if (types_data->name_list)
    key_id = GPOINTER_TO_UINT(data) ;
  else
    key_id = ((ZMapFeatureTypeStyle)(data))->original_id ;

  type_name = (char *)g_quark_to_string(key_id) ;

  if (!types_data->first_method)
    {
      if (types_data->find_string)
	types_data->str = g_string_append(types_data->str, " OR ") ;
      else
	types_data->str = g_string_append(types_data->str, "|") ;
    }
  else
    types_data->first_method = FALSE ;

  if (types_data->find_string)
    g_string_append_printf(types_data->str, "\"%s\"", type_name) ;
  else
    types_data->str = g_string_append(types_data->str, type_name) ;

  return ;
}





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
static gboolean sequenceRequest(AcedbServer server, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  char *methods = "" ;
  GList *styles ;
  gboolean no_clip = TRUE ;


  /* Get any styles stored in the context. */
  styles = ((ZMapFeatureContext)(feature_block->parent->parent))->styles ;


  /* Did the user specify the styles completely via a styles file ? If so we will
   * need to construct the method string from them. */
  if (server->user_specified_styles == TRUE)
    {
      if (!server->method_str && styles)
	{
	  server->method_str = getMethodString(styles, FALSE, FALSE) ;
	}
    }

  if (server->method_str)
    methods = server->method_str ;

  /* Here we can convert the GFF that comes back, in the end we should be doing a number of
   * calls to AceConnRequest() as the server slices...but that will need a change to my
   * AceConn package.....
   * We make the big assumption that what comes back is a C string for now, this is true
   * for most acedb requests, only images/postscript are not and we aren't asking for them. */
  /* -rawmethods makes sure that the server does _not_ use the GFF_source field in the method obj
   * to output the source field in the gff, we need to see the raw methods.
   * 
   * Note that we specify the methods both for the seqget and the seqfeatures to try and exclude
   * the parent sequence if it is not required, this is actually quite fiddly to do in the acedb
   * code in a way that won't break zmap so we do it here. */

  acedb_request =  g_strdup_printf("gif seqget %s -coords %d %d %s %s ; "
				   "seqfeatures -rawmethods -zmap %s",
				   g_quark_to_string(feature_block->original_id),
				   feature_block->block_to_sequence.q1,
				   feature_block->block_to_sequence.q2,
				   no_clip ? "-noclip" : "",
				   methods,
				   methods) ;
  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len))
      == ACECONN_OK)
    {
      char *next_line ;
      ZMapReadLine line_reader ;
      gboolean inplace = TRUE ;
      char *first_error = NULL ;


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
		{
		  server->last_err_msg = g_strdup_printf("Request from server contained incomplete line: %s",
							 next_line) ;
		  if (!first_error)
		    first_error = next_line ;		    /* Remember first line for later error
							       message.*/
		}
	      else
		{
		  if (first_error)
		    server->last_err_msg = g_strdup_printf("No GFF found in server reply,"
							   "but did find: \"%s\"", first_error) ;
		  else
		    server->last_err_msg = g_strdup_printf("%s", "No GFF found in server reply.") ;
		}

	      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			     "%s", server->last_err_msg) ;
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
		  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
				 "%s", server->last_err_msg) ;

		  if (!first_error)
		    first_error = next_line ;		    /* Remember first line for later error
							       message.*/
		}
	    }
	}
      while (result && *next_line) ;


      if (result)
	{
	  ZMapGFFParser parser ;
	  gboolean free_on_destroy ;
      

	  parser = zMapGFFCreateParser(styles, FALSE) ;


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
			  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
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
			      server->last_err_msg = g_strdup_printf("%s", error->message) ;
			    }
			  else
			    {
			      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
					     "%s", error->message) ;
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
		      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
				     "%s", server->last_err_msg) ;
		    }
		  else
		    result = TRUE ;
		}

	    }
	  while (result && *next_line) ;

	  free_on_destroy = TRUE ;
	  if (result)
	    {
	      if (zMapGFFGetFeatures(parser, feature_block))
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

      g_free(reply) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("Finished parse features\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


    }

  g_free(acedb_request) ;

  return result ;
}


/* bit of an issue over returning error messages here.....sort this out as some errors many be
 * aceconn errors, others may be data processing errors, e.g. in GFF etc., e.g.
 * 
 * 
 */
static gboolean dnaRequest(AcedbServer server, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  
  /* Because the acedb "dna" command works on the current keyset, we have to find the sequence
   * first before we can get its dna. A bit poor really but otherwise
   * we will have to add a new code to acedb to do the dna for a named key. */
  if (findSequence(server))
    {
      int block_start, block_end, dna_length;
      block_start = feature_block->block_to_sequence.q1;
      block_end   = feature_block->block_to_sequence.q2;
      /* These block numbers appear correct, but I may have the wrong
       * end of the block_to_sequence stick! */

      /* Here we get all the dna in one go, in the end we should be doing a number of
       * calls to AceConnRequest() as the server slices...but that will need a change to my
       * AceConn package....
       * -u says get the dna as a single unformatted C string.*/
      acedb_request =  g_strdup_printf("dna -u -x1 %d -x2 %d", 
                                       block_start, block_end);
      
      server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len) ;
      if (server->last_err_status == ACECONN_OK)
	{
	  if ((reply_len - 1) != ((dna_length = block_end - block_start + 1)))
	    {
	      server->last_err_msg = g_strdup_printf("DNA request failed (\"%s\"),  "
						     "expected dna length: %d "
						     "returned length: %d",
						     acedb_request,
						     dna_length, (reply_len -1)) ;
	      result = FALSE ;
	    }
	  else
	    {
              /* This is possibly a _little_ hacked ATM */
              ZMapFeature feature = NULL;
              ZMapFeatureSet feature_set = NULL;
              ZMapFeatureTypeStyle style = NULL;
              ZMapFeatureContext context = NULL;

              feature_block->sequence.type     = ZMAPSEQUENCE_DNA ;
              feature_block->sequence.length   = dna_length ;
              feature_block->sequence.sequence = reply ;

              context = (ZMapFeatureContext)(feature_block->parent->parent);

              /* Side stepping this issue for a little longer... */
              /* Ideally we need a style in order to display the dna... */
              if((style = zMapFindStyle(context->styles, g_quark_from_string("dna"))))
                {
                  /* Create the feature set */
                  feature_set = zMapFeatureSetCreate("dna", NULL);
                  /* Create the feature and add sequence to it. */
                  feature      = zMapFeatureCreateEmpty();
                  /* need to augment data too... FOR NOW just dna
                   * NOTE that we give it a strand of NONE because we want it to stay on the forward
                   * strand always... */
                  zMapFeatureAddStandardData(feature, "dna", "dna", "b0250", "sequence", 
                                             ZMAPFEATURE_RAW_SEQUENCE, style,
                                             block_start, block_end,
                                             FALSE, 0.0,
                                             ZMAPSTRAND_NONE, ZMAPPHASE_NONE) ;
                  
                  /* Add the feature to the master_align's, block's list of featuresets. */
                  /* First to our feature set */
                  zMapFeatureSetAddFeature(feature_set, feature);
                  feature_set->style = style;
                  zMapFeatureBlockAddFeatureSet(feature_block, 
                                                feature_set);
                }

              /* Make the link so that getting the sequence is EASY. */
              context->sequence = &(feature_block->sequence);

              /* everything should now be done, result is true */
	      result = TRUE ;
	    }
	}

      g_free(acedb_request) ;
    }

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


  /* We have a special case where the caller can specify  end == 0  meaning "get the sequence
   * up to the end", in this case we explicitly find out what the end is. */
  if (server->req_context->sequence_to_parent.c2 == 0)
    {
      int child_length ;

      /* NOTE that we hard code sequence in here...but what to do...gff does not give back the
       * class of the sequence object..... */
      if (getSMapLength(server, "sequence",
			(char *)g_quark_to_string(server->req_context->sequence_name),
			&child_length))
	server->req_context->sequence_to_parent.c2 =  child_length ;
    }

  if (getSMapping(server, NULL, (char *)g_quark_to_string(server->req_context->sequence_name),
		  server->req_context->sequence_to_parent.c1, server->req_context->sequence_to_parent.c2,
		  &parent_class, &parent_name, &sequence_to_parent)
      && ((server->req_context->sequence_to_parent.c2 - server->req_context->sequence_to_parent.c1 + 1) == (sequence_to_parent.c2 - sequence_to_parent.c1 + 1))
      && getSMapLength(server, parent_class, parent_name, &parent_length))
    {
      feature_context->length = sequence_to_parent.c2 - sequence_to_parent.c1 + 1 ;

      parent_to_self.p1 = parent_to_self.c1 = 1 ;
      parent_to_self.p2 = parent_to_self.c2 = parent_length ;

      feature_context->parent_name = g_quark_from_string(parent_name) ;
      g_free(parent_name) ;

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


/* Makes status request to get acedb server code version. Returns TRUE if server version is
 * same or more recent than ours, returns FALSE if version is older or it call fails
 * failure implies a serious error.
 *
 * Command and output to do this are like this:
 * 
 * acedb> status -code
 *  // ************************************************
 *  // AceDB status at 2004-11-09_15:17:03
 *  // 
 *  // - Code
 *  //             Program: giface
 *  //             Version: ACEDB 4.9.28
 *  //               Build: Nov  9 2004 13:34:42
 *  // 
 *  // ************************************************
 * 
 */
static gboolean checkServerVersion(AcedbServer server)
{
  gboolean result = FALSE ;
  char *command = "status -code" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  if (server->version_str == NULL)
    result = TRUE ;
  else
    {
      acedb_request =  g_strdup_printf("%s", command) ;

      if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						    &reply, &reply_len)) == ACECONN_OK)
	{
	  char *reply_text = (char *)reply ;
	  char *scan_text = reply_text ;
	  char *next_line = NULL ;

	  /* Scan lines for "Version:" and then extract the version, release and update numbers. */
	  while ((next_line = strtok(scan_text, "\n")))
	    {
	      scan_text = NULL ;
	      
	      if (strstr(next_line, "Version:"))
		{
		  /* Parse this string: "//             Version: ACEDB 4.9.28" */
		  char *next ;
		  
		  next = strtok(next_line, ":") ;
		  next = strtok(NULL, " ") ;
		  next = strtok(NULL, " ") ;
		  
		  if (!(result = zMapCompareVersionStings(server->version_str, next)))
		    server->last_err_msg = g_strdup_printf("Server version must be at least %s "
							   "but this server is %s.",
							   server->version_str, next) ;
		  break ;
		}
	    }

	  g_free(reply) ;
	}
    }

  return result ;
}


/* Makes "find sequence x" request to get sequence into current keyset on server.
 * Returns TRUE if sequence found, returns FALSE otherwise.
 *
 * Command and output to do this are like this:
 * 
 * acedb> find sequence b0250
 * 
 * // Found 1 objects in this class
 * // 1 Active Objects
 * 
 * 
 */
static gboolean findSequence(AcedbServer server)
{
  gboolean result = FALSE ;
  char *command = "find sequence" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  acedb_request =  g_strdup_printf("%s %s", command,
				   g_quark_to_string(server->req_context->sequence_name)) ;

  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						&reply, &reply_len)) == ACECONN_OK)
    {
      char *reply_text = (char *)reply ;
      char *scan_text = reply_text ;
      char *next_line = NULL ;

      /* Scan lines for "Found" and then extract the version, release and update numbers. */
      while ((next_line = strtok(scan_text, "\n")))
	{
	  scan_text = NULL ;
	  
	  if (strstr(next_line, "Found"))
	    {
	      /* Parse this string: "// Found 1 objects in this class" */
	      char *next ;
	      int num_obj ;
	      
	      next = strtok(next_line, " ") ;
	      next = strtok(NULL, " ") ;
	      next = strtok(NULL, " ") ;

	      num_obj = atoi(next) ;

	      if (num_obj == 1)
		result = TRUE ;
	      else
		server->last_err_msg = g_strdup_printf("Expected to find \"1\" object with name %s, "
						       "but found %d objects.",
						       g_quark_to_string(server->req_context->sequence_name),
						       num_obj) ;
	      break ;
	    }
	}

      g_free(reply) ;
    }

  return result ;
}


/* Makes "quiet -on" request to stop acedb outputting all sorts of informational junk
 * for every single request. Returns TRUE if request ok, returns FALSE otherwise.
 *
 * (n.b. if the request was successful then there is no output from the command !)
 * 
 */
static gboolean setQuietMode(AcedbServer server)
{
  gboolean result = FALSE ;
  char *command = "quiet -on" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  acedb_request =  g_strdup_printf("%s", command) ;

  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						&reply, &reply_len)) == ACECONN_OK)
    {
      result = TRUE ;

      if (reply_len != 0)
	{
	  zMapLogWarning("Replay to \"%s\" should have been NULL but was: \"%s\"",
			 command, (char *)reply) ;
	  g_free(reply) ;
	}
    }

  return result ;
}




/* Makes requests "find method" and "show -a" to get all methods in a form we can
 * parse, e.g.
 * 
 * The "query find" command is used to find either a requested list of methods
 * or all methods:
 * 
 * acedb> query find method "coding" OR  "genepairs" OR "genefinder"
 *
 * // Found 3 objects
 * // 3 Active Objects
 * acedb>
 * 
 * 
 * Then the "show" command is used to display the methods themselves:
 * 
 * acedb> show -a
 * 
 * Method : "wublastx_briggsae"
 * Remark   "wublastx search of C. elegans genomic clones vs C. briggsae peptides"
 * Colour   LIGHTGREEN
 * Frame_sensitive
 * Show_up_strand
 * Score_by_width
 * Score_bounds     10.000000 100.000000
 * Right_priority   4.750000
 * Join_blocks
 * Blixem_X
 * GFF_source       "wublastx"
 * GFF_feature      "similarity"
 * 
 * <more methods>
 * 
 * // 7 objects dumped
 * // 7 Active Objects
 * 
 * 
 *
 *  */
static gboolean parseTypes(AcedbServer server, GList *requested_types, GList **types_out)
{
  gboolean result = FALSE ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  GList *types = NULL ;

  /* Get all the methods and then filter them if there are requested types. */

  /* Get all the methods into the current keyset on the server. */
  command = "query find method" ;
  acedb_request =  g_strdup_printf("%s", command) ;
  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						&reply, &reply_len)) == ACECONN_OK)
    {
      /* reply is:
       *
       * <blank line>
       * // Found 132 objects in this class
       * // 132 Active Objects
       *
       */
      char *scan_text = (char *)reply ;
      char *next_line = NULL ;
      int num_methods ;

      while ((next_line = strtok(scan_text, "\n")))
	{
	  scan_text = NULL ;

	  if (g_str_has_prefix(next_line, "// "))
	    {
	      num_methods = getFoundObj(next_line) ;
	      if (num_methods > 0)
		result = TRUE ;
	      else
		server->last_err_msg = g_strdup_printf("Expected to find method objects but found none.") ;
	      break ;
	    }
	}

      g_free(reply) ;
      reply = NULL ;
    }

  /* Retrieve the requested methods in text form. */
  if (result == TRUE)
    {
      command = "show -a" ;
      acedb_request =  g_strdup_printf("%s", command) ;
      if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						    &reply, &reply_len)) == ACECONN_OK)
	{
	  int num_types = 0 ;
	  char *method_str = (char *)reply ;
	  char *scan_text = method_str ;
	  char *curr_pos = NULL ;
	  char *next_line = NULL ;

	  while ((next_line = strtok_r(scan_text, "\n", &curr_pos))
		 && !g_str_has_prefix(next_line, "// "))
	    {
	      ZMapFeatureTypeStyle style = NULL ;

	      scan_text = NULL ;
	      
	      if ((style = parseMethod(requested_types, next_line, &curr_pos)))
		{
		  types = g_list_append(types, style) ;
		  num_types++ ;
		}
	    }

	  if (!num_types)
	    result = FALSE ;
	  else
	    {
	      result = TRUE ;
	      *types_out = types ;
	    }

	  g_free(reply) ;
	  reply = NULL ;
	}
    }


  return result ;
}


/* The method string should be of the form:
 *
 * Method : "wublastx_briggsae"
 * Remark	 "wublastx search of C. elegans genomic clones vs C. briggsae peptides"
 * Colour	 LIGHTGREEN
 * Frame_sensitive	
 * Show_up_strand	
 * <white space only line>
 * more methods....
 * 
 * This parses the method using it to create a style struct which it returns.
 * The function also returns a pointer to the blank line that ends the current
 * method. 
 *
 * If the method name is not in the list of methods in requested_types
 * then NULL is returned. NOTE that this is not just dependent on comparing method
 * name to the requested list we have to look in column group as well.
 * 
 */
ZMapFeatureTypeStyle parseMethod(GList *requested_types, char *method_str_in, char **end_pos)
{
  ZMapFeatureTypeStyle style = NULL ;
  char *method_str = method_str_in ;
  char *scan_text = NULL ;
  char *next_line = method_str ;
  char *name = NULL, *colour = NULL, *outline = NULL, *foreground = NULL, *background = NULL,
    *gff_source = NULL, *gff_feature = NULL, *column_group = NULL, *overlap = NULL ;
  double width = ACEDB_DEFAULT_WIDTH ;
  gboolean strand_specific = FALSE, frame_specific = FALSE, show_up_strand = FALSE ;
  double min_mag = 0.0, max_mag = 0.0 ;
  double min_score = 0.0, max_score = 0.0 ;
  gboolean status = TRUE ;


  if (!g_str_has_prefix(method_str, "Method : "))
    return style ;

  do
    {
      char *tag = NULL ;
      char *value = NULL ;
      char *line_pos = NULL ;

      if (!(tag = strtok_r(next_line, "\t ", &line_pos)))
	break ;

      if (g_ascii_strcasecmp(tag, "Method") == 0)
	{
	  /* Line format:    Method : "possibly very long method name"  */

	  name = strtok_r(NULL, "\"", &line_pos) ;
	  name = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Colour") == 0)
	{
	  char *tmp_colour ;

	  tmp_colour = strtok_r(NULL, " ", &line_pos) ;

	  /* Is colour one of the standard acedb colours ? It's really an acedb bug if it
	   * isn't.... */
	  if (!(colour = getAcedbColourSpec(tmp_colour)))
	    colour = tmp_colour ;
	  
	  colour = g_strdup(colour) ;
	}
      else if (g_ascii_strcasecmp(tag, "CDS_colour") == 0)
	{
	  char *tmp_colour ;

	  tmp_colour = strtok_r(NULL, " ", &line_pos) ;

	  /* Is colour one of the standard acedb colours ? It's really an acedb bug if it
	   * isn't.... */
	  if (!(foreground = getAcedbColourSpec(tmp_colour)))
	    foreground = tmp_colour ;
	  
	  foreground = g_strdup(foreground) ;
	}
      else if (g_ascii_strcasecmp(tag, "Overlap") == 0)
	{
	  overlap = g_strdup("complete") ;
	}
      else if (g_ascii_strcasecmp(tag, "Bumpable") == 0)
	{
	  overlap = g_strdup("overlap") ;
	}
      else if (g_ascii_strcasecmp(tag, "Cluster") == 0)
	{
	  overlap = g_strdup("name") ;
	}
      else if (g_ascii_strcasecmp(tag, "GFF_source") == 0)
	{
	  gff_source = g_strdup(strtok_r(NULL, " \"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "GFF_feature") == 0)
	{
	  gff_feature = g_strdup(strtok_r(NULL, " \"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Width") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &width)))
	    {
	      zMapLogWarning("No value for \"Width\" specified in method: %s", name) ;
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Strand_sensitive") == 0)
	strand_specific = TRUE ;
      else if (g_ascii_strcasecmp(tag, "Frame_sensitive") == 0)
	{
	  frame_specific = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Show_up_strand") == 0)
	{
	  show_up_strand = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Min_mag") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &min_mag)))
	    {
	      zMapLogWarning("Bad value for \"Min_mag\" specified in method: %s", name) ;
	      
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Max_mag") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &max_mag)))
	    {
	      zMapLogWarning("Bad value for \"Max_mag\" specified in method: %s", name) ;
	      
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Score_bounds") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &min_score)))
	    {
	      zMapLogWarning("Bad value for \"Score_bounds\" specified in method: %s", name) ;
	      
	      break ;
	    }
	  else
	    {
	      value = strtok_r(NULL, " ", &line_pos) ;

	      if (!(status = zMapStr2Double(value, &max_score)))
		{
		  zMapLogWarning("Bad value for \"Score_bounds\" specified in method: %s", name) ;
		  
		  break ;
		}
	    }
	}
       else if (g_ascii_strcasecmp(tag, "Column_group") == 0)
	{
	  /* Format for this tag:   Column_group "eds_column" "wublastx_fly" */

	  column_group = g_strdup(strtok_r(NULL, ": \"", &line_pos)) ; /* Skip ': "' */
	}
    }
  while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;

  
  /* If we failed while processing a method we won't have reached the end of the current
   * method paragraph so we need to skip to the end so the next method can be processed. */
  if (!status)
    {
      while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;
    }


  /* If a list of required methods was supplied, make sure that this is one of them,
   * column_group takes precedence, otherwise we default to the actual method name. */
  if (status)
    {
      if (requested_types 
	  && ((column_group && !zMapStyleNameExists(requested_types, column_group))
	      || !zMapStyleNameExists(requested_types, name)))
	{
	  status = FALSE ;
	}
    }


  /* Set some final method stuff and create the ZMap style. */
  if (status)
    {

      /* acedb widths are wider on the screen than zmaps, so scale them up. */
      width = width * ACEDB_MAG_FACTOR ;


      /* In acedb methods the colour is interpreted differently according to the type of the
       * feature which we have to intuit here from the GFF type. acedb also has colour names
       * that don't exist in X Windows. */
      if (colour)
	{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* this doesn't work because it messes up the rev. video.... */
	  if (gff_type && (g_ascii_strcasecmp(gff_type, "\"similarity\"") == 0
			   || g_ascii_strcasecmp(gff_type, "\"repeat\"")
			   || g_ascii_strcasecmp(gff_type, "\"experimental\"")))
	    background = colour ;
	  else
	    outline = colour ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  background = colour ;
	}

      /* NOTE that style is created with the method name, NOT the column_group, column
       * names are independent of method names, they may or may not be the same. */
      style = zMapFeatureTypeCreate(name, 
				    outline, foreground, background,
				    width) ;

      if (min_mag || max_mag)
	zMapStyleSetMag(style, min_mag, max_mag) ;
      
      if (min_score && max_score)
	zMapStyleSetScore(style, min_score, max_score) ;

      zMapStyleSetStrandAttrs(style, strand_specific, frame_specific, show_up_strand) ;

      zMapStyleSetBump(style, overlap) ;

      if (gff_source || gff_feature)
	zMapStyleSetGFF(style, gff_source, gff_feature) ;
    }


  /* Clean up, note g_free() does nothing if given NULL. */
  g_free(name) ;
  g_free(colour) ;
  g_free(foreground) ;
  g_free(column_group) ;
  g_free(overlap) ;
  g_free(gff_source) ;
  g_free(gff_feature) ;

  return style ;
}


/* GCompareDataFunc () used to resort our list of styles to match users original sorting. */
gint resortStyles(gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint result = 0 ;
  ZMapFeatureTypeStyle style_a = (ZMapFeatureTypeStyle)a, style_b = (ZMapFeatureTypeStyle)b ;
  GList *style_list = (GList *)user_data ;
  gint pos_a, pos_b ;
  
  pos_a = g_list_index(style_list, GUINT_TO_POINTER(style_a->unique_id)) ;
  pos_b = g_list_index(style_list, GUINT_TO_POINTER(style_b->unique_id)) ;
  zMapAssert(pos_a >= 0 && pos_b >= 0 && pos_a != pos_b) ;

  if (pos_a < pos_b)
    result = -1 ;
  else
    result = 1 ;

  return result ;
}



/* Parses the string returned by an acedb "find" command, if the command found objects
 * it returns a string of the form:
 *                                   "// Found 1 objects in this class"
 */
int getFoundObj(char *text)
{
  int num_obj = 0 ;

  if (strstr(text, "Found"))
    {
      char *next ;
	      
      next = strtok(text, " ") ;
      next = strtok(NULL, " ") ;
      next = strtok(NULL, " ") ;

      num_obj = atoi(next) ;
    }

  return num_obj ;
}



/* Process all the alignments in a context. */
static void eachAlignment(GQuark key_id, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  DoAllAlignBlocks do_allalignblocks = (DoAllAlignBlocks)user_data ;

  if (do_allalignblocks->result == ZMAP_SERVERRESPONSE_OK && do_allalignblocks->eachBlock)
    g_list_foreach(alignment->blocks, do_allalignblocks->eachBlock, (gpointer)do_allalignblocks) ;

  return ;
}


static void eachBlockSequenceRequest(gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  DoAllAlignBlocks get_features = (DoAllAlignBlocks)user_data ;


  if (get_features->result == ZMAP_SERVERRESPONSE_OK)
    {
      if (!sequenceRequest(get_features->server, feature_block))
	{
	  /* If the call failed it may be that the connection failed or that the data coming
	   * back had a problem. */
	  if (get_features->server->last_err_status == ACECONN_OK)
	    {
	      get_features->result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }
	  else if (get_features->server->last_err_status == ACECONN_TIMEDOUT)
	    {
	      get_features->result = ZMAP_SERVERRESPONSE_TIMEDOUT ;
	    }
	  else
	    {
	      /* Probably we will want to analyse the response more than this ! */
	      get_features->result = ZMAP_SERVERRESPONSE_SERVERDIED ;
	    }

	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, get_features->server->host,
			 "Could not map %s because: %s",
			 g_quark_to_string(get_features->server->req_context->sequence_name),
			 get_features->server->last_err_msg) ;
	}
    }

  return ;
}


/* This table is derived from acedb/w2/graphcolour.c, since acedb colours have not changed
 * in a long time it is unlikely to need updating very often.
 * 
 * The reason for having this function is that acedb colour names do not ALL match the standard
 * colour names in the X11 colour database and so cannot be used as input to the gdk colour
 * functions. I tried to use a proper Xcms colour spec but stupid gdk_color_parse() does
 * not understand these colours so have used the now deprecated "#RRGGBB" format below.
 *  */
static char *getAcedbColourSpec(char *acedb_colour_name)
{
  char *colour_spec = NULL ;
  static AcedbColourSpecStruct colours[] =
    {
      {"WHITE", "#ffffff"},
      {"BLACK", "#000000"},
      {"LIGHTGRAY", "#c8c8c8"},
      {"DARKGRAY", "#646464"},
      {"RED", "#ff0000"},
      {"GREEN", "#00ff00"},
      {"BLUE", "#0000ff"},
      {"YELLOW", "#ffff00"},
      {"CYAN", "#00ffff"},
      {"MAGENTA", "#ff00ff"},
      {"LIGHTRED", "#ffa0a0"},
      {"LIGHTGREEN", "#a0ffa0"},
      {"LIGHTBLUE", "#a0c8ff"},
      {"DARKRED", "#af0000"},
      {"DARKGREEN", "#00af00"},
      {"DARKBLUE", "#0000af"},
      {"PALERED", "#ffe6d2"},
      {"PALEGREEN", "#d2ffd2"},
      {"PALEBLUE", "#d2ebff"},
      {"PALEYELLOW", "#ffffc8"},
      {"PALECYAN", "#c8ffff"},
      {"PALEMAGENTA", "#ffc8ff"},
      {"BROWN", "#a05000"},
      {"ORANGE", "#ff8000"},
      {"PALEORANGE", "#ffdc6e"},
      {"PURPLE", "#c000ff"},
      {"VIOLET", "#c8aaff"},
      {"PALEVIOLET", "#ebd7ff"},
      {"GRAY", "#969696"},
      {"PALEGRAY", "#ebebeb"},
      {"CERISE", "#ff0080"},
      {"MIDBLUE", "#56b2de"},
    } ;
  AcedbColourSpec colour ;

  colour = &(colours[0]) ;
  while (colour->name)
    {
      if (g_ascii_strcasecmp(colour->name, acedb_colour_name) == 0)
	{
	  colour_spec = colour->spec ;
	  break ;
	}

      colour++ ;
    }

  return colour_spec ;
}
