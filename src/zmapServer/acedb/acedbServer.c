/*  File: acedbServer.c
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Implements the method functions for a zmap server
 *              by making the appropriate calls to the acedb server.
 *              
 * Exported functions: See zmapServer.h
 * HISTORY:
 * Last edited: Jan 28 11:40 2008 (edgrif)
 * Created: Wed Aug  6 15:46:38 2003 (edgrif)
 * CVS info:   $Id: acedbServer.c,v 1.96 2008-01-28 14:49:53 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <stdio.h>
#include <AceConn.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfig.h>
#include <ZMap/zmapConfigDir.h>
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
  GQuark feature_set ;
  GQuark method ;
} ZMapColGroupDataStruct, *ZMapColGroupData ;


typedef struct
{
  ZMapServerResponseType result ;
  AcedbServer server ;
  GHFunc eachBlock ;
} DoAllAlignBlocksStruct, *DoAllAlignBlocks ;


typedef struct
{
  char *name ;
  char *spec ;
} AcedbColourSpecStruct, *AcedbColourSpec ;


typedef struct
{
  GHashTable *method_2_featureset ;
  GList *fetch_methods ;
} MethodFetchStruct, *MethodFetch ;


typedef struct
{
  char *draw ;
  char *fill ;
  char *border ;
} StyleColourStruct, *StyleColour ;

typedef struct
{
  StyleColourStruct normal ;
  StyleColourStruct selected ;
} StyleFeatureColoursStruct, *StyleFeatureColours ;





typedef ZMapFeatureTypeStyle (*ParseMethodFunc)(char *method_str_in,
						char **end_pos, ZMapColGroupData *col_group_data) ;



/* These provide the interface functions for an acedb server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 zMapURL url, char *format, 
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server) ;
static ZMapServerResponseType getStyles(void *server, GData **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType getFeatureSets(void *server, GList **feature_sets_out) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType closeConnection(void *server_in) ;
static gboolean destroyConnection(void *server) ;


/* general internal routines. */
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string, gboolean old_methods) ;
static void addTypeName(gpointer data, gpointer user_data) ;
static gboolean sequenceRequest(AcedbServer server, ZMapFeatureBlock feature_block) ;
static gboolean blockDNARequest(AcedbServer server, ZMapFeatureBlock feature_block) ;
static gboolean getDNARequest(AcedbServer server, char *sequence_name, int start, int end,
			      int *dna_length_out, char **dna_sequence_out) ;
static gboolean getSequenceMapping(AcedbServer server, ZMapFeatureContext feature_context) ;
static gboolean getSMapping(AcedbServer server, char *class,
			    char *sequence, int start, int end,
			    char **parent_class_out, char **parent_name_out,
			    ZMapMapBlock child_to_parent_out) ;
static gboolean getSMapLength(AcedbServer server, char *obj_class, char *obj_name,
			      int *obj_length_out) ;
static gboolean checkServerVersion(AcedbServer server) ;
static gboolean findSequence(AcedbServer server, char *sequence_name) ;
static gboolean setQuietMode(AcedbServer server) ;

static gboolean parseTypes(AcedbServer server, GData **styles_out) ;
static ZMapServerResponseType findMethods(AcedbServer server) ;
static ZMapServerResponseType getStyleNames(AcedbServer server, GList **style_names_out) ;
static ZMapFeatureTypeStyle parseMethod(char *method_str_in,
					char **end_pos, ZMapColGroupData *col_group_data) ;
gint resortStyles(gconstpointer a, gconstpointer b, gpointer user_data) ;
int getFoundObj(char *text) ;

static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockSequenceRequest(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockDNARequest(gpointer key, gpointer data, gpointer user_data);

static char *getAcedbColourSpec(char *acedb_colour_name) ;

static void freeMethodHash(gpointer data) ;
static char *getMethodFetchStr(GList *feature_sets, GHashTable *method_2_featureset) ;
static void methodFetchCB(gpointer data, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printCB(gpointer data, gpointer user_data) ;
static void stylePrintCB(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static void readConfigFile(AcedbServer server) ;
static ZMapFeatureTypeStyle parseStyle(char *method_str_in,
				       char **end_pos, ZMapColGroupData *col_group_data) ;
static gboolean getStyleColour(StyleFeatureColours style_colours, char **line_pos) ;
static ZMapServerResponseType doGetSequences(AcedbServer server, GList *sequences_inout) ;


/* 
 *             Server interface functions. 
 */



/* Compulsory routine, without this we can't do anything....returns a list of functions
 * that can be used to contact an acedb server. The functions are only visible through
 * this struct. */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs)
{
  acedb_funcs->global_init = globalInit ;
  acedb_funcs->create = createConnection ;
  acedb_funcs->open = openConnection ;
  acedb_funcs->get_styles = getStyles ;
  acedb_funcs->have_modes = haveModes ;
  acedb_funcs->get_sequence = getSequences ;
  acedb_funcs->get_feature_sets = getFeatureSets ;
  acedb_funcs->set_context = setContext ;
  acedb_funcs->get_features = getFeatures ;
  acedb_funcs->get_context_sequences = getContextSequence ;
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
  server->port = url->port ;

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


  /* Read the config file for any relevant information... */
  readConfigFile(server) ;


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



static ZMapServerResponseType getStyles(void *server_in, GData **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  server->method_2_featureset = g_hash_table_new_full(NULL, NULL, NULL, freeMethodHash) ;

  if (parseTypes(server, styles_out))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not get types from server because: %s", server->last_err_msg) ;
    }

  return result ;
}


/* acedb Method objects do not usually have any mode information (e.g. transcript etc),
 * Zmap_Style objects do.
 * 
 * We can't test for this until the config file is read which happens when we make
 * the connection. */
static ZMapServerResponseType haveModes(void *server_in, gboolean *have_mode)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  if (server->connection)
    {
      if (server->acedb_styles)
	*have_mode = TRUE ;
      else
	*have_mode = FALSE ;

      result = ZMAP_SERVERRESPONSE_OK ;
    }

  return result ;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  if (server->connection)
    {
      /* For acedb the default is to use the style names as the feature sets. */
      if ((result = doGetSequences(server_in, sequences_inout)) == ZMAP_SERVERRESPONSE_OK)
	{
	  /* Nothing to do I think.... */
	  ;
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
			 "Could not get sequences from server because: %s", server->last_err_msg) ;
	}
    }

  return result ;
}



static ZMapServerResponseType getFeatureSets(void *server_in, GList **feature_sets)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;
  GList *style_names = NULL ;

  /* For acedb the default is to use the style names as the feature sets. */
  if (((result = findMethods(server_in)) == ZMAP_SERVERRESPONSE_OK)
      && ((result = getStyleNames(server, &style_names)) == ZMAP_SERVERRESPONSE_OK))
    {
      *feature_sets = style_names ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not get feature sets from server because: %s", server->last_err_msg) ;
    }

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
    }
  else
    server->current_context = feature_context ;

  return result ;
}



/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, ZMapFeatureContext feature_context)
{
  AcedbServer server = (AcedbServer)server_in ;
  DoAllAlignBlocksStruct get_features ;

  server->current_context = feature_context ;

  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (AcedbServer)server_in ;
  get_features.server->last_err_status = ACECONN_OK ;
  get_features.eachBlock = eachBlockSequenceRequest;



  zMapPrintTimer(NULL, "In thread, getting features") ;

  /* Fetch all the alignment blocks for all the sequences. */
  g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&get_features) ;

  zMapPrintTimer(NULL, "In thread, got features") ;

  return get_features.result ;
}


/* Get features and/or sequence. */
static ZMapServerResponseType getContextSequence(void *server_in, ZMapFeatureContext feature_context)
{
  DoAllAlignBlocksStruct get_sequence ;

  get_sequence.result = ZMAP_SERVERRESPONSE_OK;
  get_sequence.server = (AcedbServer)server_in;
  get_sequence.server->last_err_status = ACECONN_OK;
  get_sequence.eachBlock = eachBlockDNARequest;

  g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&get_sequence) ;
  
  return get_sequence.result;
}

static void eachBlockDNARequest(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  DoAllAlignBlocks get_sequence = (DoAllAlignBlocks)user_data ;


  /* We should check that there is a sequence context here and report an error if there isn't... */


  /* We should be using the start/end info. in context for the below stuff... */
  if (!blockDNARequest(get_sequence->server, feature_block))
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

  if (server->method_2_featureset)
    g_hash_table_destroy(server->method_2_featureset) ;

  g_free(server) ;

  return result ;
}




/* 
 *                       Internal routines
 */


/* Make up a string that contains method names in the correct format for an acedb "Find" command
 * (find_string == TRUE) or to be part of an acedb "seqget" command.
 * We may be passed either a list of style names in GQuark form (style_name_list == TRUE)
 * or a list of the actual styles. */
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string, gboolean old_methods)
{
  char *type_names = NULL ;
  ZMapTypesStringStruct types_data ;
  GString *str ;
  gboolean free_string = TRUE ;

  zMapAssert(styles_or_style_names) ;

  str = g_string_new("") ;

  if (find_string)
    {
      if (old_methods)
	str = g_string_append(str, "query find method ") ;
      else
	str = g_string_append(str, "query find zmap_style ") ;
    }
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
    key_id = zMapStyleGetID((ZMapFeatureTypeStyle)data) ;

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
  char *gene_finder_cmds = "seqactions -gf_features no_draw ;" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  char *methods = "" ;
  GData *styles ;
  gboolean no_clip = TRUE ;


  /* Get any styles stored in the context. */
  styles = ((ZMapFeatureContext)(feature_block->parent->parent))->styles ;

  methods = getMethodFetchStr(server->current_context->feature_set_names, server->method_2_featureset) ;


  /* Check for presence of genefinderfeatures method, if present we need to tell acedb to send
   * us the gene finder methods... */
  if ((g_strrstr(methods, g_quark_to_string(zMapStyleCreateID(ZMAP_FIXED_STYLE_GFF_NAME)))))
    server->fetch_gene_finder_features = TRUE ;


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

  zMapPrintTimer(NULL, "In thread, about to ask for features") ;

  acedb_request =  g_strdup_printf("gif seqget %s -coords %d %d %s %s ; "
				   " %s "
				   "seqfeatures -rawmethods -zmap %s",
				   g_quark_to_string(feature_block->original_id),
				   feature_block->block_to_sequence.q1,
				   feature_block->block_to_sequence.q2,
				   no_clip ? "-noclip" : "",
				   methods,
				   (server->fetch_gene_finder_features ?
				    gene_finder_cmds : ""),
				   methods) ;
  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len))
      == ACECONN_OK)
    {
      char *next_line ;
      ZMapReadLine line_reader ;
      gboolean inplace = TRUE ;
      char *first_error = NULL ;

      zMapPrintTimer(NULL, "In thread, got features and about to parse into context") ;

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


      zMapPrintTimer(NULL, "In thread, finished parsing features") ;

    }

  g_free(acedb_request) ;

  return result ;
}


/* bit of an issue over returning error messages here.....sort this out as some errors many be
 * aceconn errors, others may be data processing errors, e.g. in GFF etc., e.g.
 * 
 * 
 */
static gboolean blockDNARequest(AcedbServer server, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  ZMapFeatureContext context = NULL ;
  int block_start, block_end, dna_length = 0 ;
  char *dna_sequence = NULL ;

  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block, 
							  ZMAPFEATURE_STRUCT_CONTEXT) ;


  /* belt and braces really...check that dna was actually requested as one of the columns. */
  if (!(zMap_g_list_find_quark(context->feature_set_names,
			      zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
    return result ;


  block_start = feature_block->block_to_sequence.q1 ;
  block_end   = feature_block->block_to_sequence.q2 ;
  /* These block numbers appear correct, but I may have the wrong
   * end of the block_to_sequence stick! */


  /* Because the acedb "dna" command works on the current keyset, we have to find the sequence
   * first before we can get its dna. A bit poor really but otherwise
   * we will have to add a new code to acedb to do the dna for a named key. */
  if ((result = getDNARequest(server,
			      (char *)g_quark_to_string(server->req_context->sequence_name),
			      block_start, block_end,
			      &dna_length, &dna_sequence)))
    {
      ZMapFeature feature = NULL;
      ZMapFeatureSet feature_set = NULL;
      ZMapFeatureTypeStyle style = NULL;

      feature_block->sequence.type     = ZMAPSEQUENCE_DNA ;
      feature_block->sequence.length   = dna_length ;
      feature_block->sequence.sequence = dna_sequence ;

      if ((style = zMapFindStyle(context->styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
	{
	  feature_set = zMapFeatureSetCreate(ZMAP_FIXED_STYLE_DNA_NAME, NULL);
	  feature_set->style = style;
	}

      if (feature_set)
	{
	  const char *sequence = g_quark_to_string(feature_block->original_id);
	  char *feature_name = NULL;
	  feature_name = g_strdup_printf("DNA (%s)", sequence);
	  feature = 
	    zMapFeatureCreateFromStandardData(feature_name,
					      (char *)sequence, 
					      "sequence", 
					      ZMAPFEATURE_RAW_SEQUENCE, 
					      style,
					      block_start, 
					      block_end,
					      FALSE, 0.0,
					      ZMAPSTRAND_NONE, 
					      ZMAPPHASE_NONE) ;
	  zMapFeatureSetAddFeature(feature_set, feature);
	  zMapFeatureBlockAddFeatureSet(feature_block, feature_set);
	  g_free(feature_name);
	}

      /* I'm going to create the three frame translation up front! */
      if (zMap_g_list_find_quark(context->feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
	{
	  if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
	    {
	      zMapFeatureBlockAddFeatureSet(feature_block, feature_set);
	      zMapFeature3FrameTranslationPopulate(feature_set);
	    }
	}
                
      /* everything should now be done, result is true */
      result = TRUE ;
    }

  return result ;
}




/* bit of an issue over returning error messages here.....sort this out as some errors many be
 * aceconn errors, others may be data processing errors, e.g. in GFF etc., e.g.
 * 
 * 
 */
static gboolean getDNARequest(AcedbServer server, char *sequence_name, int start, int end,
			      int *dna_length_out, char **dna_sequence_out)
{
  gboolean result = FALSE ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;


  /* Because the acedb "dna" command works on the current keyset, we have to find the sequence
   * first before we can get its dna. A bit poor really but otherwise
   * we will have to add a new code to acedb to do the dna for a named key. */
  if (findSequence(server, sequence_name))
    {
      int dna_length ;

      /* Here we get all the dna in one go, in the end we should be doing a number of
       * calls to AceConnRequest() as the server slices...but that will need a change to my
       * AceConn package....
       * -u says get the dna as a single line.*/
      acedb_request = g_strdup_printf("dna -u -x1 %d -x2 %d", start, end) ;

      server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len) ;
      if (server->last_err_status == ACECONN_OK)
	{
	  if ((reply_len - 1) != ((dna_length = end - start + 1)))
	    {
	      server->last_err_msg = g_strdup_printf("DNA request failed (\"%s\"),  "
						     "expected dna length: %d "
						     "returned length: %d",
						     acedb_request,
						     dna_length, (reply_len - 1)) ;
	      result = FALSE ;
	    }
	  else
	    {
	      *dna_length_out = dna_length ;
	      *dna_sequence_out = reply ;
                
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

  g_free(acedb_request) ;

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

  g_free(acedb_request) ;

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

      g_free(acedb_request) ;
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
static gboolean findSequence(AcedbServer server, char *sequence_name)
{
  gboolean result = FALSE ;
  char *command = "find sequence" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  acedb_request = g_strdup_printf("%s %s", command, sequence_name) ;

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
		server->last_err_msg = g_strdup_printf("Expected to find \"1\" sequence object with name %s, "
						       "but found %d objects.",
						       sequence_name, num_obj) ;
	      break ;
	    }
	}

      g_free(reply) ;
    }

  g_free(acedb_request) ;

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

  g_free(acedb_request) ;

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
static gboolean parseTypes(AcedbServer server, GData **types_out)
{
  gboolean result = FALSE ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  GData *types = NULL ;


  /* Get all the methods and then filter them if there are requested types. */
  if (findMethods(server) == ZMAP_SERVERRESPONSE_OK)
    {
      command = "show -a" ;
      acedb_request =  g_strdup_printf("%s", command) ;

      if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						    &reply, &reply_len)) == ACECONN_OK)
	{
	  ParseMethodFunc parse_func ;
	  int num_types = 0 ;
	  char *method_str = (char *)reply ;
	  char *scan_text = method_str ;
	  char *curr_pos = NULL ;
	  char *next_line = NULL ;

	  /* Set correct parser. */
	  if (server->acedb_styles)
	    parse_func = parseStyle ;
	  else
	    parse_func = parseMethod ;


	  while ((next_line = strtok_r(scan_text, "\n", &curr_pos))
		 && !g_str_has_prefix(next_line, "// "))
	    {
	      ZMapFeatureTypeStyle style = NULL ;
	      ZMapColGroupData col_group = NULL ;

	      scan_text = NULL ;
	      
	      if ((style = (parse_func)(next_line, &curr_pos, &col_group)))
		{
		  g_datalist_id_set_data(&types, zMapStyleGetUniqueID(style), style) ;
		  num_types++ ;

		  if (col_group)
		    {
		      GList *method_list = NULL ;

		      method_list = g_hash_table_lookup(server->method_2_featureset,
							GINT_TO_POINTER(col_group->feature_set)) ;

		      method_list = g_list_append(method_list, GINT_TO_POINTER(zMapStyleGetID(style))) ;

		      g_hash_table_insert(server->method_2_featureset, 
					  GINT_TO_POINTER(col_group->feature_set), method_list) ;
		      

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		      g_list_foreach(method_list, printCB, NULL) ; /* debug */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		    }
		}
	    }

	  if (!num_types)
	    result = FALSE ;
	  else
	    {
	      result = TRUE ;
	      *types_out = types ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      g_list_foreach(types, stylePrintCB, NULL) ; /* debug */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    }

	  g_free(reply) ;
	  reply = NULL ;
	}

      g_free(acedb_request) ;
    }

  return result ;
}


/* Makes request "query find method" to get all methods into keyset on server.
 * 
 * The "query find" command is used to find all methods:
 * 
 * acedb> query find method
 *
 * // Found 3 objects
 * // 3 Active Objects
 * acedb>
 * 
 * Function returns TRUE if methods were found, FALSE otherwise.
 * 
 *  */
static ZMapServerResponseType findMethods(AcedbServer server)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;


  /* Get all the methods or styles into the current keyset on the server. */
  if (server->acedb_styles)
    command = "query find zmap_style" ;
  else
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
		result = ZMAP_SERVERRESPONSE_OK ;
	      else
		server->last_err_msg = g_strdup_printf("Expected to find %s objects but found none.",
						       (server->acedb_styles ? "ZMap_style" : "Method")) ;
	      break ;
	    }
	}

      g_free(reply) ;
      reply = NULL ;
    }

  g_free(acedb_request) ;

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
 * Acedb had the concept of empty objects, these are objects whose name/class can
 * be looked up but which do not have an instance in the database. The code will NOT
 * produce styles for these objects.
 * 
 * Acedb methods can also contain a "No_display" tag which says "do not display this
 * object at all", if we find this tag we honour it. NOTE however that this can lead
 * to error messages during zmap display if the feature_set erroneously tries to
 * display features with this method.
 * 
 */
ZMapFeatureTypeStyle parseMethod(char *method_str_in,
				 char **end_pos, ZMapColGroupData *col_group_data_out)
{
  ZMapFeatureTypeStyle style = NULL ;
  char *method_str = method_str_in ;
  char *next_line = method_str ;
  char *name = NULL, *remark = NULL, *parent = NULL ;
  char *colour = NULL, *cds_colour = NULL, *outline = NULL, *foreground = NULL, *background = NULL ;
  char *gff_source = NULL, *gff_feature = NULL ;
  char *column_group = NULL, *orig_style = NULL ;
  ZMapStyleOverlapMode default_overlap_mode = ZMAPOVERLAP_OVERLAP, curr_overlap_mode = ZMAPOVERLAP_COMPLETE ;
  double width = -999.0 ;				    /* this is going to cause problems.... */
  gboolean strand_specific = FALSE, show_up_strand = FALSE,
    frame_specific = FALSE, show_only_as_3_frame = FALSE ;
  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;
  gboolean hide_always = FALSE, init_hidden = FALSE ;
  double min_mag = 0.0, max_mag = 0.0 ;
  gboolean score_set = FALSE ;
  double min_score = 0.0, max_score = 0.0 ;
  gboolean score_by_histogram = FALSE ;
  double histogram_baseline = 0.0 ;
  gboolean status = TRUE, outline_flag = FALSE, directional_end = FALSE, gaps = FALSE, join_aligns = FALSE ;
  int obj_lines ;
  int within_align_error = 0, between_align_error = 0 ;


  if (!g_str_has_prefix(method_str, "Method : "))
    return style ;

  obj_lines = 0 ;				    /* Used to detect empty objects. */
  do
    {
      char *tag = NULL ;
      char *line_pos = NULL ;

      if (!(tag = strtok_r(next_line, "\t ", &line_pos)))
	break ;

      /* We don't formally test this but Method _MUST_ be the first line of the acedb output
       * representing an object. */
      if (g_ascii_strcasecmp(tag, "Method") == 0)
	{
	  /* Line format:    Method : "possibly long method name"  */

	  name = strtok_r(NULL, "\"", &line_pos) ;
	  name = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;

	}

      if (g_ascii_strcasecmp(tag, "Remark") == 0)
	{
	  /* Line format:    Remark "possibly quite long bit of text"  */

	  remark = strtok_r(NULL, "\"", &line_pos) ;
	  remark = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Parent") == 0)
	{
	  parent = strtok_r(NULL, "\"", &line_pos) ;
	  parent = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
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
      else if (g_ascii_strcasecmp(tag, "ZMap_mode_text") == 0)
	{
	  mode = ZMAPSTYLE_MODE_TEXT ;
	}
      else if (g_ascii_strcasecmp(tag, "ZMap_mode_basic") == 0)
	{
	  mode = ZMAPSTYLE_MODE_BASIC ;
	}
      else if (g_ascii_strcasecmp(tag, "ZMap_mode_graph") == 0)
	{
	  mode = ZMAPSTYLE_MODE_GRAPH ;
	}
      else if (g_ascii_strcasecmp(tag, "Outline") == 0)
	{
          outline_flag = TRUE;
	}
      else if (g_ascii_strcasecmp(tag, "CDS_colour") == 0)
	{
	  char *tmp_colour ;

	  tmp_colour = strtok_r(NULL, " ", &line_pos) ;

	  /* Is colour one of the standard acedb colours ? It's really an acedb bug if it
	   * isn't.... */
	  if (!(cds_colour = getAcedbColourSpec(tmp_colour)))
	    cds_colour = tmp_colour ;
	  
	  cds_colour = g_strdup(cds_colour) ;
	}

      /* The link between bump mode and what actually happens to the column is not straight
       * forward in acedb, really it should be partly dependant on feature type.... */
      else if (g_ascii_strcasecmp(tag, "Overlap") == 0)
	{
	  default_overlap_mode = ZMAPOVERLAP_OVERLAP ;
	  curr_overlap_mode = ZMAPOVERLAP_COMPLETE ;
	}
      else if (g_ascii_strcasecmp(tag, "Bumpable") == 0
	       || g_ascii_strcasecmp(tag, "Cluster") == 0)
	{
	  default_overlap_mode = curr_overlap_mode = ZMAPOVERLAP_ENDS_RANGE ; 
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
      else if (g_ascii_strcasecmp(tag, "Frame_sensitive") == 0)
	{
	  frame_specific = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "show_only_as_3_frame") == 0)
	{
	  show_only_as_3_frame = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Strand_sensitive") == 0)
	strand_specific = TRUE ;
      else if (g_ascii_strcasecmp(tag, "Show_up_strand") == 0)
	{
	  show_up_strand = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "No_display") == 0)
	{
	  /* Objects that have the No_display tag set should not be shown at all. */
	  hide_always = TRUE ;

	  /* If No_display is the first tag in the models file we end
	   * up breaking here and obj_lines == 1 forcing status =
	   * FALSE later on. */
#ifdef CANT_BREAK_HERE
	  break ;
#endif
	}
      else if (g_ascii_strcasecmp(tag, "init_hidden") == 0)
	{
	  init_hidden = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Directional_ends") == 0)
	{
	  directional_end = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Gapped") == 0)
	{
	  char *value ;

	  gaps = TRUE ;

	  if ((value = strtok_r(NULL, " ", &line_pos)))
	    {
	      if (!(status = zMapStr2Int(value, &within_align_error)))
		{
		  zMapLogWarning("Bad value for \"Gapped\" align error specified in method: %s", name) ;
		  
		  break ;
		}
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Join_aligns") == 0)
	{
	  char *value ;

	  join_aligns = TRUE ;

	  if ((value = strtok_r(NULL, " ", &line_pos)))
	    {
	      if (!(status = zMapStr2Int(value, &between_align_error)))
		{
		  zMapLogWarning("Bad value for \"Join_aligns\" align error specified in method: %s", name) ;
		  
		  break ;
		}
	    }
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
      else if (g_ascii_strcasecmp(tag, "Score_by_histogram") == 0)
	{
	  char *value ;

	  score_by_histogram = TRUE ;

	  if((value = strtok_r(NULL, " ", &line_pos)))
            {
              if (!(status = zMapStr2Double(value, &histogram_baseline)))
                {
                  zMapLogWarning("Bad value for \"Score_by_histogram\" specified in method: %s", name) ;
                  
                  break ;
                }
            }
	}
      else if (g_ascii_strcasecmp(tag, "Score_bounds") == 0)
	{
	  char *value ;

	  score_set = TRUE ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &min_score)))
	    {
	      zMapLogWarning("Bad value for \"Score_bounds\" specified in method: %s", name) ;
	      score_set = FALSE ;
	      break ;
	    }
	  else
	    {
	      value = strtok_r(NULL, " ", &line_pos) ;

	      if (!(status = zMapStr2Double(value, &max_score)))
		{
		  zMapLogWarning("Bad value for \"Score_bounds\" specified in method: %s", name) ;
		  score_set = FALSE ;
		  break ;
		}
	    }
	}
       else if (g_ascii_strcasecmp(tag, "Column_group") == 0)
	{
	  /* Format for this tag:   Column_group "eds_column" "wublastx_fly" */
	  column_group = g_ascii_strdown(strtok_r(NULL, ": \"", &line_pos), -1) ; /* Skip ': "' */
	  orig_style = g_ascii_strdown(strtok_r(NULL, ": \"", &line_pos), -1) ; /* Skip ': "' */

	}

    }
  while (++obj_lines && **end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;

  /* acedb can have empty objects which consist of a first line only. */
  if (obj_lines == 1)
    {
      status = FALSE ;
    }

  
  /* If we failed while processing a method we won't have reached the end of the current
   * method paragraph so we need to skip to the end so the next method can be processed. */
  if (!status)
    {
      while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;
    }


  if (status)
    {
      if (column_group)
	{
	  ZMapColGroupData col_group_data ;

	  col_group_data = g_new0(ZMapColGroupDataStruct, 1) ;
	  col_group_data->feature_set = g_quark_from_string(column_group) ;
	  col_group_data->method = g_quark_from_string(orig_style) ;

	  *col_group_data_out = col_group_data ;
	}
    }

  /* Set some final method stuff and create the ZMap style. */
  if (status)
    {
      /* NOTE that style is created with the method name, NOT the column_group, column
       * names are independent of method names, they may or may not be the same.
       * Also, there is no way of deriving the mode from the acedb method object
       * currently, we have to set it later. */
      style = zMapFeatureTypeCreate(name, remark) ;

      if (mode != ZMAPSTYLE_MODE_INVALID)
	zMapStyleSetMode(style, mode) ;


      /* In acedb methods the colour is interpreted differently according to the type of the
       * feature which we have to intuit here from the GFF type. acedb also has colour names
       * that don't exist in X Windows. */
      if (colour || cds_colour)
	{
	  /* When it comes to colours acedb has some built in rules which we have to simulate,
	   * e.g. "Colour" when applied to transcripts means "outline", not "fill".
	   *  */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* THIS IS REALLY THE PLACE THAT WE SHOULD SET UP THE ACEDB SPECIFIC DISPLAY STUFF... */

	  /* this doesn't work because it messes up the rev. video.... */
	  if (gff_type && (g_ascii_strcasecmp(gff_type, "\"similarity\"") == 0
			   || g_ascii_strcasecmp(gff_type, "\"repeat\"")
			   || g_ascii_strcasecmp(gff_type, "\"experimental\"")))
	    background = colour ;
	  else
	    outline = colour ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* Set default acedb colours. */
	  background = colour ;
	  outline = "black" ;

	  /* If there's a cds then it must be a transcript which are shown in outline. */
	  if (cds_colour
	      || g_ascii_strcasecmp(name, "coding_transcript") == 0)
	    {
	      outline = colour ;
	      background = NULL ;
	    }


	  if (colour)
	    zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
				background, foreground, outline) ;

	  if (cds_colour)
	    zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_CDS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				NULL, NULL, cds_colour) ;
	}


      if (width != -999.0)
	{
	  /* acedb widths are wider on the screen than zmaps, so scale them up. */
	  width = width * ACEDB_MAG_FACTOR ;

	  zMapStyleSetWidth(style, width) ;
	}

      if (parent)
	zMapStyleSetParent(style, parent) ;

      if (min_mag || max_mag)
	zMapStyleSetMag(style, min_mag, max_mag) ;

      /* Note that we require bounds to be set for graphing.... */
      if (score_set)
	{
	  ZMapStyleGraphMode graph_mode = ZMAPSTYLE_GRAPH_HISTOGRAM ; /* Hard coded for now. */

	  if (score_by_histogram)
	    zMapStyleSetGraph(style, graph_mode, min_score, max_score, histogram_baseline) ;
	  else
	    zMapStyleSetScore(style, min_score, max_score) ;
	}

      zMapStyleSetStrandAttrs(style,
			      strand_specific, frame_specific,
			      show_up_strand, show_only_as_3_frame) ;

      zMapStyleInitOverlapMode(style, default_overlap_mode, curr_overlap_mode) ;

      if (gff_source || gff_feature)
	zMapStyleSetGFF(style, gff_source, gff_feature) ;

      zMapStyleSetHideAlways(style, hide_always) ;

      if (init_hidden)
	zMapStyleSetHidden(style, init_hidden) ;

      if(directional_end)
        zMapStyleSetEndStyle(style, directional_end);

      /* Current setting is for gaps to be parsed but they will only
       * be displayed when the feature is bumped. */
      if (gaps)
	zMapStyleSetGappedAligns(style, FALSE, TRUE, within_align_error) ;

      if (join_aligns)
	zMapStyleSetJoinAligns(style, TRUE, between_align_error) ;
    }


  /* Clean up, note g_free() does nothing if given NULL. */
  g_free(name) ;
  g_free(remark) ;
  g_free(parent) ;
  g_free(colour) ;
  g_free(foreground) ;
  g_free(column_group) ;
  g_free(orig_style) ;
  g_free(gff_source) ;
  g_free(gff_feature) ;

  return style ;
}


/* The style string should be of the form:
 *
 * ZMap_style : "Allele"
 * Remark	 "Alleles in WormBase represent small sequence mutations...etc"
 * Colours	 Normal Fill "ORANGE"
 * Width	 1.100000
 * Strand_sensitive	
 * GFF	 Source "Allele"
 * GFF	 Feature "Allele"
 * <white space only line>
 * more styles....
 * 
 * This parses the style using it to create a style struct which it returns.
 * The function also returns a pointer to the blank line that ends the current
 * style. 
 *
 * If the style name is not in the list of styles in requested_types
 * then NULL is returned. NOTE that this is not just dependent on comparing style
 * name to the requested list we have to look in column group as well.
 * 
 * Acedb had the concept of empty objects, these are objects whose name/class can
 * be looked up but which do not have an instance in the database. The code will NOT
 * produce styles for these objects.
 * 
 * Acedb styles can also contain a "No_display" tag which says "do not display this
 * object at all", if we find this tag we honour it. NOTE however that this can lead
 * to error messages during zmap display if the feature_set erroneously tries to
 * display features with this style.
 * 
 */
ZMapFeatureTypeStyle parseStyle(char *style_str_in,
				char **end_pos, ZMapColGroupData *col_group_data_out)
{
  ZMapFeatureTypeStyle style = NULL ;
  gboolean status = TRUE ;
  int obj_lines ;
  char *style_str = style_str_in ;
  char *next_line = style_str ;

  char *name = NULL, *remark = NULL, *parent = NULL,
    *colour = NULL, *foreground = NULL,
    *gff_source = NULL, *gff_feature = NULL,
    *column_group = NULL, *orig_style = NULL ;

  gboolean width_set = FALSE ;
  double width = 0.0 ;


  gboolean strand_set = FALSE, strand_specific = FALSE, show_up_strand = FALSE,
    frame_specific = FALSE, show_only_as_3_frame = FALSE ;

  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;

  ZMapStyleGlyphMode glyph_mode = ZMAPSTYLE_GLYPH_INVALID ;

  gboolean hide_always_set = FALSE, hide_always = FALSE,
    hidden_set = FALSE, hidden = FALSE,
    show_when_empty_set = FALSE, show_when_empty = FALSE ;

  double min_mag = 0.0, max_mag = 0.0 ;

  gboolean directional_end_set = FALSE, directional_end = FALSE ;

  gboolean internal = FALSE, external = FALSE ;
  int within_align_error = 0, between_align_error = 0 ;

  gboolean bump_set = FALSE ;
  char *overlap = NULL ;

  gboolean some_colours = FALSE ;
  StyleFeatureColoursStruct style_colours = {{NULL}, {NULL}} ;
  gboolean some_frame0_colours = FALSE, some_frame1_colours = FALSE, some_frame2_colours = FALSE ;
  StyleFeatureColoursStruct frame0_style_colours = {{NULL}, {NULL}},
    frame1_style_colours = {{NULL}, {NULL}}, frame2_style_colours = {{NULL}, {NULL}} ;
  gboolean some_CDS_colours = FALSE ;
  StyleFeatureColoursStruct CDS_style_colours = {{NULL}, {NULL}} ;

  double min_score = 0.0, max_score = 0.0 ;
  gboolean score_by_width, score_is_percent ;

  gboolean histogram = FALSE ;
  double histogram_baseline = 0.0 ;


  if (!g_str_has_prefix(style_str, "ZMap_style : "))
    return style ;


  obj_lines = 0 ;				    /* Used to detect empty objects. */
  do
    {
      char *tag = NULL ;
      char *line_pos = NULL ;

      if (!(tag = strtok_r(next_line, "\t ", &line_pos)))
	break ;

      /* We don't formally test this but Style _MUST_ be the first line of the acedb output
       * representing an object. */
      if (g_ascii_strcasecmp(tag, "ZMap_style") == 0)
	{
	  /* Line format:    ZMap_style : "possibly long style name"  */

	  name = strtok_r(NULL, "\"", &line_pos) ;
	  name = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("ZMap_style:  %s\n", name) ;		    /* debug... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	}

      if (g_ascii_strcasecmp(tag, "Remark") == 0)
	{
	  /* Line format:    Remark "possibly quite long bit of text"  */

	  remark = strtok_r(NULL, "\"", &line_pos) ;
	  remark = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Parent") == 0)
	{
	  parent = strtok_r(NULL, "\"", &line_pos) ;
	  parent = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Basic") == 0)
	{
	  mode = ZMAPSTYLE_MODE_BASIC ;
	}
      else if (g_ascii_strcasecmp(tag, "Transcript") == 0)
	{
	  char *tmp_next_tag ;

	  mode = ZMAPSTYLE_MODE_TRANSCRIPT ;

	  if ((tmp_next_tag = strtok_r(NULL, " ", &line_pos)))
	    {
	      if (strcmp(tmp_next_tag, "CDS_colour") == 0)
		{
		  gboolean colour_parse ;
		  
		  if ((colour_parse = getStyleColour(&CDS_style_colours, &line_pos)))
		    some_CDS_colours = TRUE ;
		  else
		    zMapLogWarning("Style \"%s\": Bad CDS colour spec: %s", name, next_line) ;
		}
	      /* OTHER THINGS WILL NEED TO BE PARSED HERE AS WE EXPAND THIS.... */
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Alignment") == 0)
	{
	  char *align_type ;
	  char *value ;

	  mode = ZMAPSTYLE_MODE_ALIGNMENT ;

	  if ((align_type = strtok_r(NULL, " ", &line_pos)))
	    {
	      if (g_ascii_strcasecmp(align_type, "Internal") == 0)
		internal = TRUE ;
	      else if (g_ascii_strcasecmp(align_type, "External") == 0)
		external = TRUE ;
	      else
		zMapLogWarning("Style \"%s\": Bad tag \"%s\" for \"Alignment\" specified in style: %s",
			       name, align_type, name) ;

	      if (internal || external)
		{
		  int *target ;

		  if (internal)
		    target = &within_align_error ;
		  else
		    target = &between_align_error ;

		  value = strtok_r(NULL, " ", &line_pos) ;

		  /* If no value is set then the error margin is set to 0 */
		  if (!value)
		    *target = 0 ;
		  else if (!(status = zMapStr2Int(value, target)))
		    {
		      zMapLogWarning("Style \"%s\": Bad error factor for \"Alignment   %s\" specified in style: %s",
				     name, (internal ? "Internal" : "External"), name) ;
		      break ;
		    }
		}
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Plain_text") == 0)
	{
	  mode = ZMAPSTYLE_MODE_TEXT ;
	}
      else if (g_ascii_strcasecmp(tag, "Graph") == 0)
	{
	  char *tmp_next_tag ;

	  mode = ZMAPSTYLE_MODE_GRAPH ;

	  tmp_next_tag = strtok_r(NULL, " ", &line_pos) ;

	  if (g_ascii_strcasecmp(tmp_next_tag, "Histogram") == 0)
	    {
	      histogram = TRUE ;
	    }
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Baseline") == 0)
	    {
	      char *value ;

	      value = strtok_r(NULL, " ", &line_pos) ;

	      if (!(status = zMapStr2Double(value, &histogram_baseline)))
		{
		  zMapLogWarning("Style \"%s\": No value for \"Baseline\".", name) ;
		  break ;
		}
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Glyph") == 0)
	{
	  char *tmp_next_tag ;

	  mode = ZMAPSTYLE_MODE_GLYPH ;

	  tmp_next_tag = strtok_r(NULL, " ", &line_pos) ;

	  if (tmp_next_tag && g_ascii_strcasecmp(tmp_next_tag, "Splice") == 0)
	    {
	      glyph_mode = ZMAPSTYLE_GLYPH_SPLICE ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Column_group") == 0)
	{
	  /* Format for this tag:   Column_group "eds_column" "wublastx_fly" */
	  column_group = g_ascii_strdown(strtok_r(NULL, ": \"", &line_pos), -1) ; /* Skip ': "' */
	  orig_style = g_ascii_strdown(strtok_r(NULL, ": \"", &line_pos), -1) ; /* Skip ': "' */

	}
      else if (g_ascii_strcasecmp(tag, "Colours") == 0)
	{
	  gboolean colour_parse ;

	  if ((colour_parse = getStyleColour(&style_colours, &line_pos)))
	    some_colours = TRUE ;
	  else
	    zMapLogWarning("Style \"%s\": Bad colour spec: %s", name, next_line) ;
	}
      else if (g_ascii_strcasecmp(tag, "Frame_0") == 0
	       || g_ascii_strcasecmp(tag, "Frame_1") == 0
	       || g_ascii_strcasecmp(tag, "Frame_2") == 0)
	{
	  StyleFeatureColours colours = NULL ;
	  gboolean *set = NULL ;

	  if (g_ascii_strcasecmp(tag, "Frame_0") == 0)
	    {
	      colours = &frame0_style_colours ;
	      set = &some_frame0_colours ;
	    }
	  else if (g_ascii_strcasecmp(tag, "Frame_1") == 0)
	    {
	      colours = &frame1_style_colours ;
	      set = &some_frame1_colours ;
	    }
	  else if (g_ascii_strcasecmp(tag, "Frame_2") == 0)
	    {
	      colours = &frame2_style_colours ;
	      set = &some_frame2_colours ;
	    }

	  if (!colours)
	    {
	      zMapLogWarning("Style \"%s\": Bad Frame name: %s", name, next_line) ;
	    }
	  else
	    {
	      gboolean colour_parse ;
	      
	      if ((colour_parse = getStyleColour(colours, &line_pos)))
		*set = TRUE ;
	      else
		zMapLogWarning("Style \"%s\": Bad colour spec: %s", name, next_line) ;
	    }
	}
      /* Bumping types */
      else if (g_ascii_strcasecmp(tag, "Unbumped") == 0)
	{
	  overlap = g_strdup("complete") ;
	  bump_set = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Cluster") == 0)
	{
	  overlap = g_strdup("smartest") ;
	  bump_set = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Compact_Cluster") == 0)
	{
	  overlap = g_strdup("interleave") ;
	  bump_set = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "GFF") == 0)
	{
	  char *gff_type ;

	  gff_type = strtok_r(NULL, " ", &line_pos) ;

	  if (g_ascii_strcasecmp(tag, "Source") == 0)
	    gff_source = g_strdup(strtok_r(NULL, " \"", &line_pos)) ;
	  else if (g_ascii_strcasecmp(tag, "Feature") == 0)
	    gff_feature = g_strdup(strtok_r(NULL, " \"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Width") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if ((status = zMapStr2Double(value, &width)))
	    {
	      width_set = TRUE ;
	    }
	  else
	    {
	      zMapLogWarning("Style \"%s\": No value for \"Width\".", name) ;
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Frame_sensitive") == 0)
	{
	  strand_set = TRUE ;
	  frame_specific = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Show_only_as_3_frame") == 0)
	{
	  strand_set = TRUE ;
	  show_only_as_3_frame = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Strand_sensitive") == 0)
	{
	  strand_set = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Show_up_strand") == 0)
	{
	  show_up_strand = TRUE ;
	  strand_specific = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Always") == 0)
	{
	  /* Objects that have the No_display tag set should not be shown at all. */
	  hide_always_set = hide_always = TRUE ;

	  break ;
	}
      else if (g_ascii_strcasecmp(tag, "Initially") == 0)
	{
	  hidden_set = hidden = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Show_when_empty") == 0)
	{
	  show_when_empty_set = show_when_empty = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Directional_ends") == 0)
	{
	  directional_end_set = directional_end = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Min_mag") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &min_mag)))
	    {
	      zMapLogWarning("Style \"%s\": Bad value for \"Min_mag\"", name) ;
	      
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Max_mag") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &max_mag)))
	    {
	      zMapLogWarning("Style \"%s\": Bad value for \"Max_mag\".", name) ;
	      
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Score_by_width") == 0)
	{
	  score_by_width = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Score_percent") == 0)
	{
	  score_is_percent = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Score_bounds") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if (!(status = zMapStr2Double(value, &min_score)))
	    {
	      zMapLogWarning("Style \"%s\": Bad value for \"Score_bounds\".", name) ;
	      
	      break ;
	    }
	  else
	    {
	      value = strtok_r(NULL, " ", &line_pos) ;

	      if (!(status = zMapStr2Double(value, &max_score)))
		{
		  zMapLogWarning("Style \"%s\": Bad value for \"Score_bounds\".", name) ;
		  
		  break ;
		}
	    }
	}

    }
  while (++obj_lines && **end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;

  /* acedb can have empty objects which consist of a first line only. */
  if (obj_lines == 1)
    {
      status = FALSE ;
    }

  
  /* If we failed while processing a style we won't have reached the end of the current
   * style paragraph so we need to skip to the end so the next style can be processed. */
  if (!status)
    {
      while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;
    }


  if (status)
    {
      if (column_group)
	{
	  ZMapColGroupData col_group_data ;

	  col_group_data = g_new0(ZMapColGroupDataStruct, 1) ;
	  col_group_data->feature_set = g_quark_from_string(column_group) ;
	  col_group_data->method = g_quark_from_string(orig_style) ;

	  *col_group_data_out = col_group_data ;
	}
    }

  /* Create the style and add all the bits to it. */
  if (status)
    {
      /* NOTE that style is created with the style name, NOT the column_group, column
       * names are independent of style names, they may or may not be the same.
       * Also, there is no way of deriving the mode from the acedb style object
       * currently, we have to set it later. */
      style = zMapFeatureTypeCreate(name, remark) ;

      if (mode != ZMAPSTYLE_MODE_INVALID)
	zMapStyleSetMode(style, mode) ;

      if (glyph_mode != ZMAPSTYLE_GLYPH_INVALID)
	zMapStyleSetGlyphMode(style, glyph_mode) ;

      if (parent)
	zMapStyleSetParent(style, parent) ;

      if (some_colours)
	{
	  /* May need to put some checking code here to test which colours set. */
	  zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_NORMAL,
			      style_colours.normal.fill, style_colours.normal.draw, style_colours.normal.border) ;
	  zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_NORMAL, ZMAPSTYLE_COLOURTYPE_SELECTED,
			      style_colours.selected.fill, style_colours.selected.draw, style_colours.selected.border) ;
	}

      if (some_frame0_colours || some_frame1_colours || some_frame2_colours)
	{
	  if (some_frame0_colours && some_frame1_colours && some_frame2_colours)
	    {
	      /* May need to put some checking code here to test which colours set. */
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME0, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame0_style_colours.normal.fill, frame0_style_colours.normal.draw, frame0_style_colours.normal.border) ;
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME0, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  frame0_style_colours.selected.fill, frame0_style_colours.selected.draw, frame0_style_colours.selected.border) ;

	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME1, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame1_style_colours.normal.fill, frame1_style_colours.normal.draw, frame1_style_colours.normal.border) ;
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME1, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  frame1_style_colours.selected.fill, frame1_style_colours.selected.draw, frame1_style_colours.selected.border) ;

	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME2, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame2_style_colours.normal.fill, frame2_style_colours.normal.draw, frame2_style_colours.normal.border) ;
	      zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_FRAME2, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  frame2_style_colours.selected.fill, frame2_style_colours.selected.draw, frame2_style_colours.selected.border) ;
	    }
	  else
	    zMapLogWarning("Style \"%s\": Bad frame colour spec, following were not set:%s%s%s", name, 
			   (some_frame0_colours ? "" : " frame0"),
			   (some_frame1_colours ? "" : " frame1"),
			   (some_frame2_colours ? "" : " frame2")) ;
	}

      if (some_CDS_colours)
	{
	  /* May need to put some checking code here to test which colours set. */
	  zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_CDS, ZMAPSTYLE_COLOURTYPE_NORMAL,
			      CDS_style_colours.normal.fill, CDS_style_colours.normal.draw, CDS_style_colours.normal.border) ;
	  zMapStyleSetColours(style, ZMAPSTYLE_COLOURTARGET_CDS, ZMAPSTYLE_COLOURTYPE_SELECTED,
			      CDS_style_colours.selected.fill, CDS_style_colours.selected.draw, CDS_style_colours.selected.border) ;
	}

      if (width_set)
	zMapStyleSetWidth(style, width) ;


      if (min_mag || max_mag)
	zMapStyleSetMag(style, min_mag, max_mag) ;


      /* OTHER SCORE STUFF MUST BE SET HERE.... */

      if (min_score && max_score)
	zMapStyleSetScore(style, min_score, max_score) ;

      if (strand_set)
	zMapStyleSetStrandAttrs(style,
				strand_specific, frame_specific,
				show_up_strand, show_only_as_3_frame) ;

      if (bump_set)
	zMapStyleSetBump(style, overlap) ;

      if (gff_source || gff_feature)
	zMapStyleSetGFF(style, gff_source, gff_feature) ;

      if (hide_always_set)
	zMapStyleSetHideAlways(style, hide_always) ;

      if (hidden_set)
	zMapStyleSetHidden(style, hidden) ;

      if (show_when_empty_set)
	zMapStyleSetShowWhenEmpty(style, show_when_empty) ;

      if(directional_end_set)
        zMapStyleSetEndStyle(style, directional_end);

      if (internal)
	zMapStyleSetGappedAligns(style, TRUE, TRUE, within_align_error) ;

      if (external)
	zMapStyleSetJoinAligns(style, TRUE, between_align_error) ;
    }


  /* Clean up, note g_free() does nothing if given NULL. */
  g_free(name) ;
  g_free(remark) ;
  g_free(colour) ;
  g_free(foreground) ;
  g_free(column_group) ;
  g_free(orig_style) ;
  g_free(overlap) ;
  g_free(gff_source) ;
  g_free(gff_feature) ;

  return style ;
}


/* Gets a list of all styles by name on the server (this is the list of acedb methods/styles).
 * Returns TRUE and the list if database contained any methods, FALSE otherwise.
 * 
 * 
 * acedb> list
 * 
 * KeySet : Answer_1
 * Method:
 *  cDNA_for_RNAi
 *  code default
 *  Coding
 *  Coding_transcript
 *  Coil
 *  curated
 * 
 * 
 * // 6 object listed
 * // 6 Active Objects
 * acedb>
 * 
 *  */
static ZMapServerResponseType getStyleNames(AcedbServer server, GList **style_names_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;

  /* Get all the methods into the current keyset on the server and then list them. */
  if (findMethods(server) == ZMAP_SERVERRESPONSE_OK)
    {
      char *command ;
      char *acedb_request = NULL ;
      void *reply = NULL ;
      int reply_len = 0 ;

      command = "list" ;
      acedb_request =  g_strdup_printf("%s", command) ;

      if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						    &reply, &reply_len)) == ACECONN_OK)
	{
	  char *scan_text = (char *)reply ;
	  char *next_line = NULL ;
	  gboolean found_method = FALSE ;
	  GList *style_names = NULL ;

	  while ((next_line = strtok(scan_text, "\n")))
	    {
	      scan_text = NULL ;

	      /* Look for start/end of methods list. */
	      if (!found_method && g_str_has_prefix(next_line, "Method:"))
		{
		  found_method = TRUE ;
		  continue ;
		}
	      else if (found_method && (*next_line == '/'))
		break ;

	      if (found_method)
		{
		  /* Watch out...hacky...method names have a space in front of them...sigh... */
		  style_names = g_list_append(style_names,
					      GINT_TO_POINTER(g_quark_from_string(next_line + 1))) ;
		}
	    }


	  if (style_names)
	    {
	      *style_names_out = style_names ;
	      result = ZMAP_SERVERRESPONSE_OK ;
	    }
	  else
	    {
	      server->last_err_msg = g_strdup_printf("No styles found.") ;
	      result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }

	  g_free(reply) ;
	  reply = NULL ;
	}
      else
	result = server->last_err_status ;

      g_free(acedb_request) ;
    }


  return result ;
}



/* GCompareDataFunc () used to resort our list of styles to match users original sorting. */
gint resortStyles(gconstpointer a, gconstpointer b, gpointer user_data)
{
  gint result = 0 ;
  ZMapFeatureTypeStyle style_a = (ZMapFeatureTypeStyle)a, style_b = (ZMapFeatureTypeStyle)b ;
  GList *style_list = (GList *)user_data ;
  gint pos_a, pos_b ;
  
  pos_a = g_list_index(style_list, GUINT_TO_POINTER(zMapStyleGetUniqueID(style_a))) ;
  pos_b = g_list_index(style_list, GUINT_TO_POINTER(zMapStyleGetUniqueID(style_b))) ;
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
static void eachAlignment(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureAlignment alignment = (ZMapFeatureAlignment)data ;
  DoAllAlignBlocks all_data = (DoAllAlignBlocks)user_data ;

  if (all_data->result == ZMAP_SERVERRESPONSE_OK && all_data->eachBlock)
    g_hash_table_foreach(alignment->blocks, all_data->eachBlock, (gpointer)all_data) ;

  return ;
}



static void eachBlockSequenceRequest(gpointer key_id, gpointer data, gpointer user_data)
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





static void freeMethodHash(gpointer data)
{

  /* Doing this screws up later code...sort this out.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  g_list_free((GList *)data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return ;
}



static char *getMethodFetchStr(GList *feature_sets, GHashTable *method_2_featureset)
{
  char *method_fetch_str = NULL ;
  MethodFetchStruct method_data ;

  method_data.method_2_featureset = method_2_featureset ;
  method_data.fetch_methods = NULL ;

  g_list_foreach(feature_sets, methodFetchCB, &method_data) ;

  /* use the routine I've already written to get the final string.... */
  method_fetch_str = getMethodString(method_data.fetch_methods, TRUE, FALSE, FALSE) ;

  return method_fetch_str ;
}


static void methodFetchCB(gpointer data, gpointer user_data)
{
  GQuark feature_set = GPOINTER_TO_INT(data) ;
  MethodFetch method_data = (MethodFetch)user_data ;
  GList *method_list ;

  /* If there are methods that used a column_group to specify a method then look up the
   * method from our hash, otherwise just use the method name. */
  if (method_data->method_2_featureset
      && (method_list = (GList *)g_hash_table_lookup(method_data->method_2_featureset,
						     GINT_TO_POINTER(feature_set))))
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_list_foreach(method_list, printCB, NULL) ;	    /* debug */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      method_data->fetch_methods = g_list_concat(method_data->fetch_methods, method_list) ;
    }
  else
    method_data->fetch_methods = g_list_append(method_data->fetch_methods,
					       GINT_TO_POINTER(feature_set)) ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printCB(gpointer data, gpointer user_data)
{
  GQuark feature_set = GPOINTER_TO_INT(data) ;

  printf("%s\n", g_quark_to_string(feature_set)) ;

  return ;
}


static void stylePrintCB(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  printf("%s (%s)\n", g_quark_to_string(zMapStyleGetID(style)),
	 g_quark_to_string(zMapStyleGetUniqueID(style))) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Read standard zmap config file for acedb specific parameters. */
static void readConfigFile(AcedbServer server)
{
  ZMapConfigStanzaSet server_list = NULL ;
  ZMapConfig config ;
  char *config_file = NULL ;
  gboolean result ;

  config_file = zMapConfigDirGetFile() ;
  if ((config = zMapConfigCreateFromFile(config_file)))
    {
      ZMapConfigStanza server_stanza ;
      ZMapConfigStanzaElementStruct server_elements[] = {{ZMAPSTANZA_SOURCE_URL,     ZMAPCONFIG_STRING, {NULL}},
							 {ZMAPSTANZA_SOURCE_STYLE,  ZMAPCONFIG_BOOL, {NULL}},
							 {NULL, -1, {NULL}}} ;

      /* Set defaults for any element that is not a string. */
      zMapConfigGetStructBool(server_elements, ZMAPSTANZA_SOURCE_STYLE) = FALSE ;

      server_stanza = zMapConfigMakeStanza(ZMAPSTANZA_SOURCE_CONFIG, server_elements) ;

      if (!zMapConfigFindStanzas(config, server_stanza, &server_list))
	result = FALSE ;

      zMapConfigDestroyStanza(server_stanza) ;
      server_stanza = NULL ;
    }

  /* OK, read stuff for this server... */
  if (config && server_list)
    {
      ZMapConfigStanza next_server ;


      /* Current error handling policy is to connect to servers that we can and
       * report errors for those where we fail but to carry on and set up the ZMap
       * as long as at least one connection succeeds. */
      next_server = NULL ;
      while ((next_server = zMapConfigGetNextStanza(server_list, next_server)) != NULL)
	{
	  char *url ;
	  zMapURL url_obj ;
	  int url_parse_error = 0 ;

	  url = zMapConfigGetElementString(next_server, ZMAPSTANZA_SOURCE_URL) ;

	  /* look for our stanza, we only want info. for our server instance. */
	  if (!(url_obj = url_parse(url, &url_parse_error))
	      || strcmp(url_obj->host, server->host) != 0 || url_obj->port != server->port)
	    {
	      continue ;
	    }
	  else
	    {
	      server->acedb_styles = zMapConfigGetElementBool(next_server, ZMAPSTANZA_SOURCE_STYLE) ;
	      



	      break ;					    /* only look at first stanza that is us. */
	    }
	}
    }


  /* clean up. */
  if (server_list)
    zMapConfigDeleteStanzaSet(server_list) ;
  if (config)
    zMapConfigDestroy(config) ;



  return ;
}



static gboolean getStyleColour(StyleFeatureColours style_colours, char **line_pos)
{
  gboolean result = FALSE ;
  char *colour_type ;
  char *colour_target ;
  char *colour ;
  StyleColour style_colour = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((colour_type = strtok_r(NULL, " ", line_pos))
      && (colour_target = strtok_r(NULL, " ", line_pos))
      && (colour = strtok_r(NULL, "\"", line_pos))
      && (colour = strtok_r(NULL, "\"", line_pos)))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    colour_type = strtok_r(NULL, " ", line_pos) ;
  colour_target = strtok_r(NULL, " ", line_pos) ;
  colour = strtok_r(NULL, "\"", line_pos) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  colour = strtok_r(NULL, "\"", line_pos) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (colour)
    {
      if (g_ascii_strcasecmp(colour_type, "Normal") == 0)
	style_colour = &(style_colours->normal) ;
      else if (g_ascii_strcasecmp(colour_type, "Selected") == 0)
	style_colour = &(style_colours->selected) ;

      if (style_colour)
	{
	  result = TRUE ;
	  if (g_ascii_strcasecmp(colour_target, "Draw") == 0)
	    style_colour->draw = g_strdup(colour) ;
	  else if (g_ascii_strcasecmp(colour_target, "Fill") == 0)
	    style_colour->fill = g_strdup(colour) ;
	  else if (g_ascii_strcasecmp(colour_target, "Border") == 0)
	    style_colour->border = g_strdup(colour) ;
	  else
	    result = FALSE ;
	}
    }

  return result ;
}




/* For each sequence makes a request to find the sequence and then to dump its dna:
 * 
 * acedb> find sequence RDS00121111         
 * <blank line>
 * // Found 1 objects in this class
 * acedb> dna -u
 * gactctttgcaggggagaagctccacaacctcagcaaa....etc etc
 * acedb>
 * 
 * 
 * Function returns ZMAP_SERVERRESPONSE_OK if sequences were found and retrieved,
 * ZMAP_SERVERRESPONSE_REQFAIL otherwise.
 * 
 *  */
static ZMapServerResponseType doGetSequences(AcedbServer server, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  GList *next_seq ;
  GString *acedb_request = NULL ;

  acedb_request = g_string_new(NULL) ;

  /* We need to loop round finding each sequence and then fetching its dna... */
  next_seq = sequences_inout ;
  while (next_seq)
    {
      char *command ;
      void *reply = NULL ;
      int reply_len = 0 ;
      ZMapSequence sequence = (ZMapSequence)(next_seq->data) ;


      /* Try to find the sequence... */
      command = "find sequence" ;
      g_string_printf(acedb_request, "%s %s", command, g_quark_to_string(sequence->name)) ;
      
      if ((server->last_err_status = AceConnRequest(server->connection, acedb_request->str,
						    &reply, &reply_len)) == ACECONN_OK)
	{
	  /* reply should be:
	   *
	   * <blank line>
	   * // Found 1 objects in this class
	   * // 1 Active Objects
	   */
	  char *scan_text = (char *)reply ;
	  char *next_line = NULL ;
	  int num_objs ;

	  while ((next_line = strtok(scan_text, "\n")))
	    {
	      scan_text = NULL ;

	      if (g_str_has_prefix(next_line, "// "))
		{
		  num_objs = getFoundObj(next_line) ;
		  if (num_objs == 1)
		    result = ZMAP_SERVERRESPONSE_OK ;
		  else
		    server->last_err_msg = g_strdup_printf("Expected to find 1 sequence object"
							   "named \"%s\" but found %d.",
							   g_quark_to_string(sequence->name), num_objs) ;
		  break ;
		}
	    }

	  g_free(reply) ;
	  reply = NULL ;
	}

      /* All ok ?  Then get the dna.... */
      if (server->last_err_status == ZMAP_SERVERRESPONSE_OK)
	{
	  command = "dna -u" ;
	  g_string_printf(acedb_request, "%s", command) ;
      
	  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request->str,
							&reply, &reply_len)) == ACECONN_OK)
	    {
	      /* reply should be:
	       *
	       * gactctttgcaggggagaagctccacaacctcagcaaa....etc etc
	       */
	      sequence->length = reply_len - 1 ;
	      sequence->sequence = reply ;
	    }
	}

      next_seq = g_list_next(next_seq) ;
    }

  g_string_free(acedb_request, TRUE) ;


  return result ;
}





