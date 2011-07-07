/*  Last edited: Jul  6 10:46 2011 (edgrif) */
/*  File: acedbServer.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2011: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Implements the method functions for a zmap server
 *              by making the appropriate calls to the acedb server.
 *
 * Exported functions: See zmapServer.h
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <string.h>
#include <stdio.h>
#include <AceConn.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapGLibUtils.h>
#include <ZMap/zmapConfigIni.h>
#include <ZMap/zmapConfigStrings.h>
#include <ZMap/zmapGFF.h>
#include <ZMap/zmapStyle.h>
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
  GList *methods ;
  GHashTable *method_2_data ;
  GHashTable *styles ;
} LoadableStruct, *Loadable ;


typedef struct
{
  AcedbServer server ;
  GList *feature_set_methods ;
  GList *feature_methods ;
  GList *required_styles ;
  GHashTable *set_2_styles ;
  gboolean error ;
} GetMethodsStylesStruct, *GetMethodsStyles ;


typedef struct
{
  GQuark feature_set_id ;
  char *remark ;
  GHashTable *method_2_feature_set ;
} HashFeatureSetStruct, *HashFeatureSet ;



typedef struct
{
  GQuark feature_set ;
} ZMapColGroupDataStruct, *ZMapColGroupData ;


typedef struct
{
  ZMapServerResponseType result ;
  AcedbServer server ;
  GHashTable *styles ;
  GList *src_feature_set_names;
  GHFunc eachBlock ;
} DoAllAlignBlocksStruct, *DoAllAlignBlocks ;


typedef struct
{
  char *name ;
  char *spec ;
} AcedbColourSpecStruct, *AcedbColourSpec ;


typedef struct
{
  GHashTable *method_2_data ;
  GHashTable *styles;
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


typedef struct
{
  GList *names_list ;

  GQuark feature_set ;

  GList *required_styles ;
} ColGroupNamesStruct, *ColGroupNames ;


typedef ZMapFeatureTypeStyle (*ParseMethodFunc)(char *method_str_in,
						char **end_pos, ZMapColGroupData *col_group_data) ;

typedef gboolean (*ParseMethodNamesFunc)(AcedbServer server, char *method_str_in,
					 char **end_pos, gpointer user_data) ;


/* These provide the interface functions for an acedb server implementation, i.e. you
 * shouldn't change these prototypes without changing all the other server prototypes..... */
static gboolean globalInit(void) ;
static gboolean createConnection(void **server_out,
				 ZMapURL url, char *format,
                                 char *version_str, int timeout) ;
static ZMapServerResponseType openConnection(void *server,ZMapServerReqOpen req_open) ;
static ZMapServerResponseType getInfo(void *server, ZMapServerInfo info) ;
static ZMapServerResponseType getFeatureSetNames(void *server,
						 GList **feature_sets_out,
						 GList *sources,
						 GList **required_styles,
						 GHashTable **featureset_2_stylelist_inout,
						 GHashTable **featureset_2_column_inout,
						 GHashTable **source_2_sourcedata_inout) ;
static ZMapServerResponseType getStyles(void *server, GHashTable **styles_out) ;
static ZMapServerResponseType haveModes(void *server, gboolean *have_mode) ;
static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout) ;
static ZMapServerResponseType setContext(void *server, ZMapFeatureContext feature_context) ;
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context_out) ;
static char *lastErrorMsg(void *server) ;
static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out);
static ZMapServerResponseType closeConnection(void *server_in) ;
static ZMapServerResponseType destroyConnection(void *server) ;


/* general internal routines. */
static ZMapServerResponseType findColStyleTags(AcedbServer server,
					       GList **feature_set_methods_inout,
					       GList **feature_methods_out,
					       GList **required_styles_out,
					       GHashTable **featureset_2_stylelist_inout) ;
static GList *getMethodsLoadable(GList *all_methods, GHashTable *method_2_data, GHashTable *styles) ;
static void loadableCB(gpointer data, gpointer user_data) ;
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string, gboolean find_methods) ;
static void addTypeName(gpointer data, gpointer user_data) ;
static gboolean sequenceRequest(DoAllAlignBlocks get_features, ZMapFeatureBlock feature_block) ;
static gboolean blockDNARequest(AcedbServer server, GHashTable *styles, ZMapFeatureBlock feature_block) ;
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

static gboolean parseTypes(AcedbServer server, GHashTable **styles_out,
			   ParseMethodNamesFunc parse_func_in, gpointer user_data) ;
static ZMapServerResponseType findMethods(AcedbServer server, char *search_str, int *num_found) ;
static ZMapServerResponseType getObjNames(AcedbServer server, GList **style_names_out) ;
static ZMapFeatureTypeStyle parseMethod(char *method_str_in,
					char **end_pos, ZMapColGroupData *col_group_data) ;
static gboolean parseMethodColGroupNames(AcedbServer server, char *method_str_in,
					 char **end_pos, gpointer user_data) ;
static void addMethodCB(gpointer data, gpointer user_data) ;
gint resortStyles(gconstpointer a, gconstpointer b, gpointer user_data) ;
int getFoundObj(char *text) ;

static void eachAlignment(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockSequenceRequest(gpointer key, gpointer data, gpointer user_data) ;
static void eachBlockDNARequest(gpointer key, gpointer data, gpointer user_data);

static char *getAcedbColourSpec(char *acedb_colour_name) ;

static gboolean parseMethodStyleNames(AcedbServer server, char *method_str_in,
				      char **end_pos, gpointer user_data) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void printCB(gpointer data, gpointer user_data) ;
static void stylePrintCB(gpointer data, gpointer user_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static ZMapFeatureTypeStyle parseStyle(char *method_str_in,
				       char **end_pos, ZMapColGroupData *col_group_data) ;
static gboolean getStyleColour(StyleFeatureColours style_colours, char **line_pos) ;
static ZMapServerResponseType doGetSequences(AcedbServer server, GList *sequences_inout) ;
static gboolean getServerInfo(AcedbServer server, ZMapServerInfo info) ;

static int equaliseLists(AcedbServer server, GList **feature_sets_inout, GList *method_names,
			 char *query_name, char *reference_name) ;
static gint quarkCaseCmp(gconstpointer a, gconstpointer b) ;
static void setErrMsg(AcedbServer server, char *new_msg) ;
static void resetErr(AcedbServer server) ;

static char *get_url_query_value(char *full_query, char *key) ;
static gboolean get_url_query_boolean(char *full_query, char *key) ;

static void freeDataCB(gpointer data) ;
static void freeSetCB(gpointer data) ;

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
  acedb_funcs->get_info = getInfo ;
  acedb_funcs->feature_set_names = getFeatureSetNames ;
  acedb_funcs->get_styles = getStyles ;
  acedb_funcs->have_modes = haveModes ;
  acedb_funcs->get_sequence = getSequences ;
  acedb_funcs->set_context = setContext ;
  acedb_funcs->get_features = getFeatures ;
  acedb_funcs->get_context_sequences = getContextSequence ;
  acedb_funcs->errmsg = lastErrorMsg ;
  acedb_funcs->get_status = getStatus ;
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
				 ZMapURL url, char *format,
                                 char *version_str, int timeout)
{
  gboolean result = FALSE ;
  gboolean use_methods = FALSE;
  AcedbServer server ;

  /* Always return a server struct as it contains error message stuff. */
  server = (AcedbServer)g_new0(AcedbServerStruct, 1) ;

  resetErr(server) ;

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

  server->has_new_tags = TRUE ;

  use_methods = get_url_query_boolean(url->query, "use_methods");

  server->has_new_tags = !use_methods;

  if (server->has_new_tags)
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      server->method_2_data = g_hash_table_new_full(NULL, NULL, NULL, freeDataCB) ;


      server->method_2_feature_set = g_hash_table_new_full(NULL, NULL, NULL, freeSetCB) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }

  server->zmap_start = 1;
  server->zmap_end = 0;

  *server_out = (void *)server ;

  if ((server->last_err_status =
       AceConnCreate(&(server->connection), server->host, url->port, url->user, url->passwd, timeout)) == ACECONN_OK)
    result = TRUE ;

  return result ;
}


/* When we open the connection we not only check the acedb version of the server but also
 * set "quiet" mode on so that we can get dna, gff and other stuff back unadulterated by
 * extraneous information. */
// mh17: added sequence_server flag for compatability with pipeServer, it's not used here
static ZMapServerResponseType openConnection(void *server_in, ZMapServerReqOpen req_open)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

  server->zmap_start = req_open->zmap_start;
  server->zmap_end   = req_open->zmap_end;

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



static ZMapServerResponseType getInfo(void *server_in, ZMapServerInfo info)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

  if (getServerInfo(server, info))
    {
      result = ZMAP_SERVERRESPONSE_OK ;
    }
  else
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not get server info because: %s", server->last_err_msg) ;
    }

  return result ;
}



// if we had a mapping given by ZMap, make that take priority: overwrite the ACEDB one
void overlayFeatureSet2Column(GHashTable *method_2_feature_set, GHashTable *featureset_2_column)
{
  GList *iter;
  gpointer key,value;
  ZMapFeatureSetDesc method_set,featureset;

  if(!featureset_2_column)
      return;
  zMap_g_hash_table_iter_init(&iter,method_2_feature_set);
  while(zMap_g_hash_table_iter_next(&iter,&key,&value))
  {
      featureset = (ZMapFeatureSetDesc) g_hash_table_lookup(featureset_2_column,key);
      method_set = (ZMapFeatureSetDesc) value;

      if(featureset)
      {
            method_set->column_id = featureset->column_id;
            if(featureset->feature_set_text)
                  method_set->feature_set_text = g_strdup(featureset->feature_set_text);
      }
      if(!method_set->column_ID)
            method_set->column_ID = method_set->column_id;
      if(!method_set->feature_src_ID)
            method_set->column_ID = method_set->column_ID;
  }
}

// if we had a mapping given by ZMap, make that take priority: overwrite the ACEDB one
void overlaySource2Data(GHashTable *method_2_data, GHashTable *source_2_data)
{
  GList *iter;
  gpointer key,value;
  ZMapFeatureSource method_src,source_data;

  if(!source_2_data)
      return;

  zMap_g_hash_table_iter_init(&iter,method_2_data);
  while(zMap_g_hash_table_iter_next(&iter,&key,&value))
  {
      source_data = (ZMapFeatureSource) g_hash_table_lookup(source_2_data,key);
      method_src  = (ZMapFeatureSource) value;

      if(source_data)
      {
            if(source_data->style_id)
                  method_src->style_id = source_data->style_id;
            if(source_data->source_id)
                  method_src->source_id = source_data->source_id;
            if(source_data->source_text)
                  method_src->source_text = source_data->source_text;
      }
  }
}

/* Feature Set names passed to the acedb server _MUST_ be the names of Method objects in the
 * database.
 * If we are using the new method/style tags then the Method objects may contain Column_group
 * tags which are used to combine several feature sets into one and each method that contains
 * features must have a zmap style.
 *
 * This function must do a lot of checking and it is vital this is done well otherwise we end up
 * with styles/methods we don't need or much worse we don't load all the styles that the
 * feature sets require.
 *
 * This function takes a list of names, checks that it can find the corresponding Method objects
 * and then retrieves those methods. It looks in the methods for Column_group tags and uses them
 * to construct a new list of all the feature sets that need to be retrieved from the server.
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
  AcedbServer server = (AcedbServer)server_in ;
  char *method_string = NULL ;
  int num_methods = 0, num_feature_sets ;
  GList *feature_sets, *feature_set_methods, *feature_methods, *col_group_methods,  *method_names ;
  GList *required_styles ;
  GHashTable *featureset_2_stylelist ;

  resetErr(server) ;

  if (server->all_methods)
    {
      g_list_free(server->all_methods) ;
      server->all_methods = NULL ;
    }

  if (server->has_new_tags)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_hash_table_destroy(server->method_2_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      server->method_2_data = g_hash_table_new_full(NULL, NULL, NULL, freeDataCB) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      g_hash_table_destroy(server->method_2_feature_set) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      server->method_2_feature_set = g_hash_table_new_full(NULL, NULL, NULL, freeSetCB) ;
    }


  /* Here we need to find methods for all the given feature set names and then look
   * for column groups and styles if the new tag sets are being used.
   *
   * The list needs to be stored inside the server struct for later use in getting styles..*/

  feature_sets = *feature_sets_inout ;
  feature_set_methods = col_group_methods = required_styles = method_names = NULL ;
  num_feature_sets = g_list_length(feature_sets) ;


  /* 1) find methods to match feature set names, puts methods into acedb's active keyset.
   * (method_string is an acedb query string to find all those methods). */
  method_string = getMethodString(feature_sets, TRUE, TRUE, TRUE) ;

  if ((result = findMethods(server_in, method_string, &num_methods)) != ZMAP_SERVERRESPONSE_OK)
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
		     "Could not find feature set methods in server because: %s", server->last_err_msg) ;
    }
  else if (num_feature_sets != num_methods)
    {
      if (num_feature_sets > num_methods)
	{
	  ZMAPSERVER_LOG(Warning,  ACEDB_PROTOCOL_STR, server->host,
			 "%s", "Some featuresets could not be found.") ;
	}
      else
	{
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "%s", "Too many featuresets found ! Ace Server Query probably incorrect !") ;
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}
    }

  g_free(method_string) ;



  /* 2) Check feature set names against method names, remove any feature sets for which there is
   * no method, we fail if no methods are found. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      if ((result = getObjNames(server, &feature_set_methods)) == ZMAP_SERVERRESPONSE_OK)
	{


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMap_g_list_quark_print(feature_set_methods, "feature_set_methods", FALSE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



	  if (num_feature_sets != num_methods)
	    {
	      if (!equaliseLists(server, &(feature_sets), feature_set_methods, "Feature Sets", "Methods"))
		{
		  result = ZMAP_SERVERRESPONSE_REQFAIL ;
		}
	    }
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "Could not get list of feature set methods from server because: %s",
			 server->last_err_msg) ;
	}
    }


  /* If we are using styles/column groups then extra processing is required, otherwise we just
   * return what methods we have and if possible the list of required styles. The all_methods
   * list is used by acedb in retrieving just those features in a seqget/seqfeatures call. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL && server->has_new_tags)
    {
      featureset_2_stylelist = *featureset_2_stylelist_inout ;

      if ((result = findColStyleTags(server,
				     &feature_sets, &feature_methods,
				     &required_styles, &featureset_2_stylelist))
	  != ZMAP_SERVERRESPONSE_REQFAIL)
	{
	  server->all_methods = feature_methods ;


	  /* hack for now...should really check that they are all in feature_methods. */
	  if (sources)
	    server->all_methods = sources ;


	  *feature_sets_inout = feature_sets ;

	  *required_styles_out = required_styles ;

	  *featureset_2_stylelist_inout = featureset_2_stylelist ;

        overlayFeatureSet2Column(server->method_2_feature_set,*featureset_2_column_inout);
	  *featureset_2_column_inout = server->method_2_feature_set ;

        overlaySource2Data(server->method_2_data,*source_2_sourcedata_inout);
	  *source_2_sourcedata_inout = server->method_2_data ;
	}
    }
  else if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      server->all_methods = feature_set_methods ;

      *feature_sets_inout = feature_sets ;

      *required_styles_out = g_list_copy(feature_set_methods) ;
    }

  return result ;
}




static ZMapServerResponseType getStyles(void *server_in, GHashTable **styles_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

  if (!findMethods(server, NULL, NULL) == ZMAP_SERVERRESPONSE_OK)
    {
      result = ZMAP_SERVERRESPONSE_REQFAIL ;
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Could not find types on server because: %s", server->last_err_msg) ;
    }
  else
    {
      if (parseTypes(server, styles_out, NULL, NULL))
	{
	  result = ZMAP_SERVERRESPONSE_OK ;
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
			 "Could not get types from server because: %s", server->last_err_msg) ;
	}
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

  resetErr(server) ;

  if (server->connection)
    {
      if (server->has_new_tags)
	*have_mode = TRUE ;
      else
	*have_mode = FALSE ;

      result = ZMAP_SERVERRESPONSE_OK ;
    }

  return result ;
}


static ZMapServerResponseType getStatus(void *server_conn, gint *exit_code, gchar **stderr_out)
{
      *exit_code = 0;
      *stderr_out = NULL;
      return ZMAP_SERVERRESPONSE_OK;
}


static ZMapServerResponseType getSequences(void *server_in, GList *sequences_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

  if (!sequences_inout)
    {
      setErrMsg(server, g_strdup("getSequences request made but no sequence names specified.")) ;

      server->last_err_status = ACECONN_BADARGS ;
    }
  else
    {
      if ((result = doGetSequences(server_in, sequences_inout)) != ZMAP_SERVERRESPONSE_OK)
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
			 "Could not get sequences from server because: %s", server->last_err_msg) ;
	}
    }

  return result ;
}



/* the struct/param handling will not work in these routines now and needs sorting out.... */

static ZMapServerResponseType setContext(void *server_in, ZMapFeatureContext feature_context)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  AcedbServer server = (AcedbServer)server_in ;
  gboolean status ;

  resetErr(server) ;

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
    {
      server->current_context = feature_context ;
    }

  return result ;
}



/* Get features sequence. */
static ZMapServerResponseType getFeatures(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
{
  AcedbServer server = (AcedbServer)server_in ;
  DoAllAlignBlocksStruct get_features ;

  resetErr(server) ;

  server->current_context = feature_context ;

  get_features.result = ZMAP_SERVERRESPONSE_OK ;
  get_features.server = (AcedbServer)server_in ;
  get_features.server->last_err_status = ACECONN_OK ;
  get_features.styles = styles ;
  get_features.src_feature_set_names = NULL;
  get_features.eachBlock = eachBlockSequenceRequest;



  zMapPrintTimer(NULL, "In thread, getting features") ;

  /* Fetch all the alignment blocks for all the sequences. */
  g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&get_features) ;

      /* get the list of source featuresets */
  feature_context->src_feature_set_names = get_features.src_feature_set_names;

#if MH17_this_cant_work
      /* replace the list of requested columns with a list of requested featuresets
       * this is needed to keep track of what featuresets were actually requested.
       *
       * NOTE src_feature_set_names is constructed per block and merged
       * req_feature_set_names is as given and applies to all blocks??
       * NOTE multiple block requests are not currently used and cannot apply to pipe servers
       * It would be much simpler to use single block requests only and if
       * necessary implement multiple block requests from the main ZMap code
       * as this would be compatable with pipeServers
       */
#warning review this if multiple blocks are implemented
  GList *req_names,*req_cols;
  req_cols = feature_context->req_feature_set_names;
  req_names = NULL;
  while(req_cols)
  {

      column = g_hash_table_lookup(server->method_2_feature_set,);
      if(column)
      {
            req_names = g_list_concat(req_names,column->features
      }
      req_cols = g_list_delete_link(req_names,req_cols);

  }
#endif

  zMapPrintTimer(NULL, "In thread, got features") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    GError *error = NULL ;

    zMapFeatureDumpStdOutFeatures(feature_context, styles, &error) ;

  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return get_features.result ;
}


/* Get features and/or sequence. */
static ZMapServerResponseType getContextSequence(void *server_in, GHashTable *styles, ZMapFeatureContext feature_context)
{
  AcedbServer server = (AcedbServer)server_in ;
  DoAllAlignBlocksStruct get_sequence ;

  resetErr(server) ;

  get_sequence.result = ZMAP_SERVERRESPONSE_OK;
  get_sequence.server = server ;
  get_sequence.styles = styles ;
  get_sequence.src_feature_set_names = NULL;
  get_sequence.server->last_err_status = ACECONN_OK;
  get_sequence.eachBlock = eachBlockDNARequest;

  g_hash_table_foreach(feature_context->alignments, eachAlignment, (gpointer)&get_sequence) ;

  return get_sequence.result;
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
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

  if ((server->last_err_status = AceConnConnectionOpen(server->connection)) == ACECONN_OK
      && (server->last_err_status = AceConnDisconnect(server->connection)) == ACECONN_OK)
    result = ZMAP_SERVERRESPONSE_OK ;

  return result ;
}

static ZMapServerResponseType destroyConnection(void *server_in)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  AcedbServer server = (AcedbServer)server_in ;

  resetErr(server) ;

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
 *                       Internal routines
 */


/* Assumes that server has a keyset of methods to be searched for Column_XXX and Style tags.
 * The methods are the "columns" and each method may directly represent features itself or
 * may group together other methods that represent features via the Column tags.
 *
 * This all takes a few stages as below.
 *  */
static ZMapServerResponseType findColStyleTags(AcedbServer server,
					       GList **feature_sets_inout,
					       GList **feature_methods_out,
					       GList **required_styles_out,
					       GHashTable **featureset_2_stylelist_inout)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_OK ;
  GList *feature_sets, *feature_set_methods = NULL, *feature_methods = NULL, *required_styles = NULL ;
  GHashTable *featureset_2_stylelist ;
  char *method_string ;
  int num_orig, num_curr ;


  feature_sets = *feature_sets_inout ;
  featureset_2_stylelist = *featureset_2_stylelist_inout ;

  /*
   * Check methods/styles for columns.
   */

  /* Check that all of the methods either have a column_child tag and no style
   * or have a style and no column_child tag, any that don't will be excluded (n.b. we don't
   * validate the style it may come from another server.) This step also gets all the
   * methods that are required to load features. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      GetMethodsStylesStruct get_sets = {NULL} ;

      num_orig = g_list_length(feature_sets) ;

      if (parseTypes(server, NULL, parseMethodColGroupNames, &(get_sets)))
	{
	  result = ZMAP_SERVERRESPONSE_OK ;

	  feature_set_methods = get_sets.feature_set_methods ;
	  feature_methods = get_sets.feature_methods ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("\n=======================\n") ;
	  zMap_g_list_quark_print(feature_set_methods, "Column feature_sets", FALSE) ;
	  printf("\n=======================\n") ;
	  zMap_g_list_quark_print(feature_methods, "Child methods", FALSE) ;
	  printf("\n=======================\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  num_curr = g_list_length(feature_set_methods) ;

	  /* Check the list of feature sets against valid methods, remove any feature sets
	   * without a valid method, we fail if no methods are found. */
	  if (num_orig != num_curr)
	    {
	      if (!equaliseLists(server, &(feature_sets), feature_set_methods, "Feature Sets", "Valid Methods"))
		{
		  result = ZMAP_SERVERRESPONSE_REQFAIL ;
		}
	    }
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "Could not fetch methods to look for Column_child and Style tags: %s", server->last_err_msg) ;
	}
    }



  /*
   * Check methods/styles for feature sets.
   */

  /* 2) Check that all feature methods have a style, any that don't will be excluded.
   * Also check whether any reference a parent column group. Update hash of both for
   * reference in reading features. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      /* 2a) Get all methods into acedb's active keyset. */
      num_orig = g_list_length(feature_methods) ;

      method_string = getMethodString(feature_methods, TRUE, TRUE, TRUE) ;

      if ((result = findMethods(server, method_string, &num_curr)) != ZMAP_SERVERRESPONSE_OK)
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "Could not find feature set methods in server because: %s", server->last_err_msg) ;
	}
      else if (num_orig != num_curr)
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;

	  if (num_orig > num_curr)
	    ZMAPSERVER_LOG(Warning,  ACEDB_PROTOCOL_STR, server->host,
			   "%s", "Some featuresets could not be found.") ;
	  else
	    ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			   "%s", "Too many featuresets found ! Ace Server Query probably incorrect !") ;
	}

      g_free(method_string) ;
    }


  /* 2b) Check all methods were found, remove any methods that were not, we fail if no methods are found. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      GList *curr_methods = NULL ;

      if ((result = getObjNames(server, &curr_methods)) == ZMAP_SERVERRESPONSE_OK)
	{
	  if (num_orig != num_curr)
	    {
	      if (!equaliseLists(server, &(feature_methods), curr_methods, "Feature Methods", "Methods"))
		{
		  result = ZMAP_SERVERRESPONSE_REQFAIL ;
		}
	    }
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "Could not get list of feature set methods from server because: %s",
			 server->last_err_msg) ;
	}

      g_list_free(curr_methods) ;
    }


  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      GetMethodsStylesStruct get_sets = {NULL} ;

      num_orig = g_list_length(feature_methods) ;

      get_sets.server = server ;
      get_sets.set_2_styles = featureset_2_stylelist ;

      if (parseTypes(server, NULL, parseMethodStyleNames, &(get_sets)))
	{
	  result = ZMAP_SERVERRESPONSE_OK ;

	  required_styles = get_sets.required_styles ;
	  featureset_2_stylelist = get_sets.set_2_styles ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  zMap_g_list_quark_print(required_styles, "feature_sets", FALSE) ;
	  printf("\n=======================\n") ;
	  zMap_g_hashlist_print(featureset_2_stylelist) ;
	  printf("\n=======================\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  num_curr = g_list_length(get_sets.feature_methods) ;

	  if (num_orig != num_curr)
	    {
	      if (!equaliseLists(server, &(feature_methods), get_sets.feature_methods, "Feature Methods", "Methods"))
		{
		  result = ZMAP_SERVERRESPONSE_REQFAIL ;
		}
	    }
	}
      else
	{
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	  ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			 "Could not fetch " COL_CHILD " methods to look for zmap_styles: %s", server->last_err_msg) ;
	}
    }

  /* Return results if all ok. */
  if (result != ZMAP_SERVERRESPONSE_REQFAIL)
    {
      *feature_sets_inout = feature_sets ;
      *feature_methods_out = feature_methods ;
      *required_styles_out = required_styles ;
      *featureset_2_stylelist_inout = featureset_2_stylelist ;
   }

  return result ;
}



static GList *getMethodsLoadable(GList *all_methods, GHashTable *method_2_data, GHashTable *styles)
{
  GList *loadable_methods = NULL ;
  LoadableStruct loadable_data ;

  loadable_data.methods = NULL ;
  loadable_data.method_2_data = method_2_data ;
  loadable_data.styles = styles  ;

  g_list_foreach(all_methods, loadableCB, &loadable_data) ;

  loadable_methods = loadable_data.methods ;

  return loadable_methods ;
}


static void loadableCB(gpointer data, gpointer user_data)
{
  Loadable loadable_data = ( Loadable)user_data ;

  loadable_data->methods = g_list_append(loadable_data->methods, GUINT_TO_POINTER(data)) ;

  return ;
}


/* Make up a string that contains method names in the correct format for an acedb "Find" command
 * (find_string == TRUE) or to be part of an acedb "seqget" command.
 * We may be passed either a list of style names in GQuark form (style_name_list == TRUE)
 * or a list of the actual styles. */
static char *getMethodString(GList *styles_or_style_names,
			     gboolean style_name_list, gboolean find_string, gboolean find_methods)
{
  char *type_names = NULL ;
  ZMapTypesStringStruct types_data ;
  GString *str ;
  gboolean free_string = TRUE ;

  zMapAssert(styles_or_style_names) ;

  str = g_string_new("") ;

  if (find_string)
    {
      if (find_methods)
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
    {
      ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data;
      key_id = zMapStyleGetID(style) ;
    }

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
    {
      g_string_append_printf(types_data->str, "\"%s\"", type_name) ;
    }
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
static gboolean sequenceRequest(DoAllAlignBlocks get_features, ZMapFeatureBlock feature_block)
{
  AcedbServer server = get_features->server;
  GHashTable *styles = get_features->styles;
  gboolean result = FALSE ;
  char *gene_finder_cmds = "seqactions -gf_features no_draw ;" ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  GList *loadable_methods = NULL ;
  char *methods = "" ;
  gboolean no_clip = TRUE ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Get any styles stored in the context. */
  styles = ((ZMapFeatureContext)(feature_block->parent->parent))->styles ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* Exclude any methods that have "deferred loading" set in their styles, if no styles then
   * include all methods. */
  if (server->has_new_tags)
    loadable_methods = getMethodsLoadable(server->all_methods, server->method_2_data, styles) ;
  else
    loadable_methods = g_list_copy(server->all_methods) ;

  methods = getMethodString(loadable_methods, TRUE, FALSE, FALSE) ;

  g_list_free(loadable_methods) ;
  loadable_methods = NULL ;


  /* Check for presence of genefinderfeatures method, if present we need to tell acedb to send
   * us the gene finder methods... */
  if ((zMap_g_ascii_strstrcasecmp(methods, ZMAP_FIXED_STYLE_GFF_NAME)))
    server->fetch_gene_finder_features = TRUE ;


  /* Here we can convert the GFF that comes back, in the end we should be doing a number of
   * calls to AceConnRequest() as the server slices...but that will need a change to my
   * AceConn package.....
   * We make the big assumption that what comes back is a C string for now, this is true
   * for most acedb requests, only images/postscript are not and we aren't asking for them. */
  /* -rawmethods makes sure that the server does _not_ use the GFF_source field in the method obj
   * to output the source field in the gff, we need to see the raw methods.
   * -refseq makes sure that the coords returned are relative to the reference sequence, _not_
   * to the object at the top of the smap tree .
   *
   * Note that we specify the methods both for the seqget and the seqfeatures to try and exclude
   * the parent sequence if it is not required, this is actually quite fiddly to do in the acedb
   * code in a way that won't break zmap so we do it here. */

  zMapPrintTimer(NULL, "In thread, about to ask for features") ;

  acedb_request =  g_strdup_printf("gif seqget %s -coords %d %d %s %s ; "
				   " %s "
				   "seqfeatures -refseq %s -rawmethods -zmap %s",
				   g_quark_to_string(feature_block->original_id),
				   server->zmap_start,
				   server->zmap_end,
				   no_clip ? "-noclip" : "",
				   methods,
				   (server->fetch_gene_finder_features ? gene_finder_cmds : ""),
				   g_quark_to_string(feature_block->original_id),
				   methods) ;

  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request, &reply, &reply_len))
      == ACECONN_OK)
    {
      ZMapReadLine line_reader ;
      gboolean inplace = TRUE ;
      char *first_error = NULL ;
      char *next_line ;
      gsize line_length ;

      zMapPrintTimer(NULL, "In thread, got features and about to parse into context") ;

      line_reader = zMapReadLineCreate((char *)reply, inplace) ;

      /* Look for "##gff-version" at start of line which signals start of GFF, as detailed above
       * there may be errors before the GFF output. */
      result = TRUE ;
      do
	{
	  if (!(result = zMapReadLineNext(line_reader, &next_line, &line_length)))
	    {
	      /* If the readline fails it may be because of an error or because its reached the
	       * end, if next_line is empty then its reached the end. */
	      if (*next_line)
		{
		  setErrMsg(server, g_strdup_printf("Request from server contained incomplete line: %s",
						    next_line)) ;
		  if (!first_error)
		    first_error = next_line ;		    /* Remember first line for later error
							       message.*/
		}
	      else
		{
		  if (first_error)
		    setErrMsg(server,  g_strdup_printf("No GFF found in server reply,"
						       "but did find: \"%s\"", first_error)) ;
		  else
		    setErrMsg(server,  g_strdup_printf("%s", "No GFF found in server reply.")) ;
		}

	      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
			     "%s", server->last_err_msg) ;
	    }
	  else
	    {
	      /* The ace server first gives us all the errors from the seqget/seqfeatures as
	       * comments as then finally we get to the gff. */
	      if (g_str_has_prefix((char *)next_line, "##gff-version"))
		{
		  break ;
		}
	      else
		{
		  if (g_str_has_prefix((char *)next_line, "// ERROR"))
		    {
		      setErrMsg(server,  g_strdup_printf("Error fetching features: %s",
							 next_line)) ;
		      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
				     "%s", server->last_err_msg) ;
		    }
		  else if (g_str_has_prefix((char *)next_line, "//"))
		    {
		      setErrMsg(server,  g_strdup_printf("Information from server: %s",
							 next_line)) ;
		      ZMAPSERVER_LOG(Message, ACEDB_PROTOCOL_STR, server->host,
				     "%s", server->last_err_msg) ;
		    }
		  else
		    {
		      setErrMsg(server,  g_strdup_printf("Bad GFF line: %s",
							 next_line)) ;
		      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
				     "%s", server->last_err_msg) ;
		    }

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


	  /* Set up the parser, if we are doing cols/styles then set hash tables
	   * in parser to map the gff source name to the Feature Set (== Column) and a Style. */
	  parser = zMapGFFCreateParser(g_quark_to_string(feature_block->original_id),
				       server->zmap_start, server->zmap_end) ;
	  zMapGFFParserInitForFeatures(parser, styles, FALSE) ;

	  if (server->has_new_tags)
	    {
	      zMapGFFParseSetSourceHash(parser, server->method_2_feature_set, server->method_2_data) ;
	    }


	  /* We probably won't have to deal with part lines here acedb should only return whole lines
	   * ....but should check for sure...bomb out for now.... */
	  result = TRUE ;
	  do
	    {
	      /* Note that we already have the first line from the loop above. */
	      if (!zMapGFFParseLineLength(parser, next_line, line_length))
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
			  setErrMsg(server,
				    g_strdup_printf("zMapGFFParseLine() failed with no GError for line %d: %s",
						    zMapGFFGetLineNumber(parser), next_line)) ;
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
			      setErrMsg(server,  g_strdup_printf("%s", error->message)) ;
			    }
			  else
			    {
			      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
					     "%s", error->message) ;
			    }
			}
		    }
		}


	      if (!(result = zMapReadLineNext(line_reader, &next_line, &line_length)))
		{
		  /* If the readline fails it may be because of an error or because its reached the
		   * end, if next_line is empty then its reached the end. */
		  if (*next_line)
		    {
		      setErrMsg(server,  g_strdup_printf("Request from server contained incomplete line: %s",
							 next_line)) ;
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
		  GList *src_names;
		  free_on_destroy = FALSE ;	/* Make sure parser does _not_ free our data. ! */

		  /* get the featuresets actually put in the block
		   * and pass upstream, returning a list of featuresets in all blocks
		   */
		  src_names = zMapGFFGetFeaturesets(parser) ;

		  get_features->src_feature_set_names =
		    zMap_g_list_merge(get_features->src_feature_set_names, src_names) ;
		}
	      else
		{
		  result = FALSE ;
		}
	    }

	  zMapGFFSetFreeOnDestroy(parser, free_on_destroy) ;
	  zMapGFFDestroyParser(parser) ;
	}

      zMapReadLineDestroy(line_reader, FALSE) ;		    /* n.b. don't free string as it is the
							       same as reply which is freed later.*/

      g_free(reply) ;


      zMapPrintTimer(NULL, "In thread, finished parsing features") ;

    }

  if(acedb_request)
    g_free(acedb_request) ;

  if(methods)
    g_free(methods);

  return result ;
}



static void eachBlockDNARequest(gpointer key, gpointer data, gpointer user_data)
{
  ZMapFeatureBlock feature_block = (ZMapFeatureBlock)data ;
  DoAllAlignBlocks get_sequence = (DoAllAlignBlocks)user_data ;


  /* We should check that there is a sequence context here and report an error if there isn't... */


  /* We should be using the start/end info. in context for the below stuff... */
  if (!blockDNARequest(get_sequence->server, get_sequence->styles, feature_block))
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
		     "Could not get DNA sequence for %s because: %s",
		     g_quark_to_string(get_sequence->server->req_context->sequence_name),
		     get_sequence->server->last_err_msg) ;
    }


  return ;
}



/* bit of an issue over returning error messages here.....sort this out as some errors many be
 * aceconn errors, others may be data processing errors, e.g. in GFF etc., e.g.
 *
 *
 */
static gboolean blockDNARequest(AcedbServer server, GHashTable *styles, ZMapFeatureBlock feature_block)
{
  gboolean result = FALSE ;
  ZMapFeatureContext context = NULL ;
  int block_start, block_end, dna_length = 0 ;
  char *dna_sequence = NULL ;

  context = (ZMapFeatureContext)zMapFeatureGetParentGroup((ZMapFeatureAny)feature_block,
							  ZMAPFEATURE_STRUCT_CONTEXT) ;


  block_start = feature_block->block_to_sequence.block.x1 ;
  block_end   = feature_block->block_to_sequence.block.x2 ;
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
      ZMapFeatureTypeStyle dna_style = NULL;

      if (zMapFeatureDNACreateFeatureSet(feature_block, &feature_set))
	{
	  ZMapFeatureTypeStyle temp_style = NULL;

	  /* This temp style creation feels wrong, and probably is,
	   * but we don't have the merged in default styles in here,
	   * or so it seems... */

	  if (!(dna_style = zMapFindStyle(styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_DNA_NAME))))
	    temp_style = dna_style = zMapStyleCreate(ZMAP_FIXED_STYLE_DNA_NAME,
						     ZMAP_FIXED_STYLE_DNA_NAME_TEXT);

	  feature = zMapFeatureDNACreateFeature(feature_block, dna_style, dna_sequence, dna_length);

	  if (temp_style)
	    zMapStyleDestroy(temp_style);
	}

      /* I'm going to create the three frame translation up front! */
      if (zMap_g_list_find_quark(context->req_feature_set_names, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME)))
	{
	  if ((zMapFeature3FrameTranslationCreateSet(feature_block, &feature_set)))
	    {
	      ZMapFeatureTypeStyle frame_style = NULL;
	      gboolean style_absolutely_required = FALSE;

	      frame_style = zMapFindStyle(styles, zMapStyleCreateID(ZMAP_FIXED_STYLE_3FT_NAME));

	      if(style_absolutely_required && !frame_style)
		zMapLogWarning("Cowardly refusing to create features '%s' without style",
			       ZMAP_FIXED_STYLE_3FT_NAME);
	      else
		zMapFeature3FrameTranslationSetCreateFeatures(feature_set, frame_style);
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
	      setErrMsg(server,  g_strdup_printf("DNA request failed (\"%s\"),  "
						 "expected dna length: %d "
						 "returned length: %d",
						 acedb_request,
						 dna_length, (reply_len - 1))) ;
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
  ZMapMapBlockStruct sequence_to_parent = { {0, 0} , {0, 0}, FALSE };
  int parent_length = 0 ;


  /* We have a special case where the caller can specify  end == 0  meaning "get the sequence
   * up to the end", in this case we explicitly find out what the end is. */
  if (server->zmap_end == 0)
    {
      int child_length ;

      /* NOTE that we hard code sequence in here...but what to do...gff does not give back the
       * class of the sequence object..... */
      if (getSMapLength(server, "sequence",
			(char *)g_quark_to_string(server->req_context->sequence_name),
			&child_length))
	server->zmap_end = child_length ;
    }

  zMapLogWarning("getSequenceMapping %d -> %d",server->zmap_start,server->zmap_end);

  if (getSMapping(server, NULL, (char *)g_quark_to_string(server->req_context->sequence_name),
		  server->zmap_start,server->zmap_end,
		  &parent_class, &parent_name, &sequence_to_parent)
      && ((server->zmap_end - server->zmap_start + 1) ==
	  (sequence_to_parent.block.x2 - sequence_to_parent.block.x1 + 1))

      && getSMapLength(server, parent_class, parent_name, &parent_length))
    {
      feature_context->parent_name = g_quark_from_string(parent_name) ;
      g_free(parent_name) ;

      if(!feature_context->parent_span.x2)
	{
	  feature_context->parent_span.x1 = 1;
	  feature_context->parent_span.x2 = parent_length;
	}

      feature_context->master_align->sequence_span.x1 = sequence_to_parent.parent.x1 ;
      feature_context->master_align->sequence_span.x2 = sequence_to_parent.parent.x2 ;

      /* set up block coords as well in case not supplied in context (req = 1,0) */
      {
	ZMapFeatureAlignment align = feature_context->master_align;
	GHashTable *blocks = align->blocks ;
	ZMapFeatureBlock block ;

	zMapAssert(g_hash_table_size(blocks) == 1) ;

	block = (ZMapFeatureBlock)(zMap_g_hash_table_nth(blocks, 0)) ;

	zMapLogWarning("getSequenceMapping block %d -> %d",
		       block->block_to_sequence.block.x1,block->block_to_sequence.block.x2);
	if(!block->block_to_sequence.parent.x2)
          {
            block->block_to_sequence.parent.x1 = server->zmap_start;   /* = align->sequence_span.x1 ; */
            block->block_to_sequence.parent.x2 = server->zmap_end;     /* = align->sequence_span.x2 ; */
          }
	if(!block->block_to_sequence.block.x2)
          {
            block->block_to_sequence.block.x1 = server->zmap_start;   /* = align->sequence_span.x1 ; */
            block->block_to_sequence.block.x2 = server->zmap_end;     /* = align->sequence_span.x2 ; */
	    zMapLogWarning("getSequenceMapping set block %d -> %d",
			   block->block_to_sequence.block.x1,block->block_to_sequence.block.x2);
          }
	block->block_to_sequence.reversed = FALSE;
      }

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
	  setErrMsg(server,  g_strdup_printf("SMap request failed, "
					     "request: \"%s\",  "
					     "reply: \"%s\"", acedb_request, reply_text)) ;
	  result = FALSE ;
	}
      else
	{
	  enum {FIELD_BUFFER_LEN = 257} ;
	  char parent_class[FIELD_BUFFER_LEN] = {'\0'}, parent_name[FIELD_BUFFER_LEN] = {'\0'} ;
	  char status[FIELD_BUFFER_LEN] = {'\0'} ;
	  ZMapMapBlockStruct child_to_parent =  { {0, 0} , {0, 0}, FALSE } ;
	  enum {FIELD_NUM = 7} ;
	  char *format_str = "SMAP %256[^:]:%256s%d%d%d%d%256s" ;
	  int fields ;

	  if ((fields = sscanf(reply_text, format_str, &parent_class[0], &parent_name[0],
			       &child_to_parent.parent.x1, &child_to_parent.parent.x2,
			       &child_to_parent.block.x1, &child_to_parent.block.x2,
			       &status[0])) != FIELD_NUM)
	    {
	      setErrMsg(server,  g_strdup_printf("Could not parse smap data, "
						 "request: \"%s\",  "
						 "reply: \"%s\"", acedb_request, reply_text)) ;
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
	  setErrMsg(server,  g_strdup_printf("%s request failed, "
					     "request: \"%s\",  "
					     "reply: \"%s\"", command, acedb_request, reply_text)) ;
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
	      setErrMsg(server,  g_strdup_printf("Could not parse smap data, "
						 "request: \"%s\",  "
						 "reply: \"%s\"", acedb_request, reply_text)) ;
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
		    setErrMsg(server,  g_strdup_printf("Server version must be at least %s "
						       "but this server is %s.",
						       server->version_str, next)) ;
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
		setErrMsg(server,  g_strdup_printf("Expected to find \"1\" sequence object with name %s, "
						   "but found %d objects.",
						   sequence_name, num_obj)) ;
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




/* Uses the "status" command to get database and code information, e.g.
 *
 * acedb> status -database
 *  // ************************************************
 *  // AceDB status at 2008-02-08_10:59:02
 *  //
 *  // - Database
 *  //               Title: <undefined>
 *  //                Name: <undefined>
 *  //             Release: 4_0
 *  //           Directory: /nfs/team71/acedb/edgrif/acedb/databases/JAMES.NEWSTYLES/
 *  //             Session: 7
 *  //                User: edgrif
 *  //           Last Save: 2008-02-06_15:47:52
 *  //        Write Access: No
 *  //      Global Address: 3470
 *  //
 *  // ************************************************
 *
 * // 0 Active Objects
 * acedb>
 *
 *
 *  */
static gboolean getServerInfo(AcedbServer server, ZMapServerInfo info)
{
  gboolean result = FALSE ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  /* We could add "status -code" later..... */
  command = "status -database" ;

  info->request_as_columns = TRUE;

  acedb_request =  g_strdup_printf("%s", command) ;
  if ((server->last_err_status = AceConnRequest(server->connection, acedb_request,
						&reply, &reply_len)) == ACECONN_OK)
    {
      char *scan_text = (char *)reply ;
      char *next_line = NULL ;
      char *curr_pos = NULL ;

      while ((next_line = strtok_r(scan_text, "\n", &curr_pos)))
	{
	  scan_text = NULL ;

	  if (strstr(next_line, "Directory") != NULL)
	    {
	      char *target ;
	      char *tag_pos = NULL ;

	      target = strtok_r(next_line, ":", &tag_pos) ;
	      target = strtok_r(NULL, " ", &tag_pos) ;

	      if (target)
		{
		  result = TRUE ;
		  info->database_path = g_strdup(target) ;
		}
	    }
	  else if (strstr(next_line, "Title") != NULL)
	    {
	      char *target ;
	      char *tag_pos = NULL ;

	      target = strtok_r(next_line, ":", &tag_pos) ;

	      if (tag_pos && !(strstr(tag_pos, "<undefined>")))
		{
		  result = TRUE ;
		  info->database_title = g_strstrip(g_strdup(tag_pos)) ;
		}
	    }
	  else if (strstr(next_line, "Name") != NULL)
	    {
	      char *target ;
	      char *tag_pos = NULL ;

	      target = strtok_r(next_line, ":", &tag_pos) ;

	      if (tag_pos && !(strstr(tag_pos, "<undefined>")))
		{
		  result = TRUE ;
		  info->database_name = g_strstrip(g_strdup(tag_pos)) ;
		}
	    }
	}

      g_free(reply) ;
      reply = NULL ;
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
static gboolean parseTypes(AcedbServer server, GHashTable **types_out,
			   ParseMethodNamesFunc parse_func_in, gpointer user_data)
{
  gboolean result = FALSE ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;
  GHashTable *types = NULL ;

  /* Get all the methods and then filter them if there are requested types. */
  command = "show -a" ;
  acedb_request = g_strdup_printf("%s", command) ;

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
      if (!parse_func_in)
	{
	  if (server->has_new_tags)
	    parse_func = parseStyle ;
	  else
	    parse_func = parseMethod ;
	}


      while ((next_line = strtok_r(scan_text, "\n", &curr_pos))
	     && !g_str_has_prefix(next_line, "// "))
	{
	  ZMapFeatureTypeStyle style = NULL ;
	  ZMapColGroupData col_group = NULL ;

	  scan_text = NULL ;

	  if (parse_func_in)
	    {
	      GQuark method_id ;

	      if ((method_id = (parse_func_in)(server, next_line, &curr_pos, user_data)))
		{
		  num_types++ ;
		}
	    }
	  else if ((style = (parse_func)(next_line, &curr_pos, &col_group)))
	    {
	      if (!types)
		types = g_hash_table_new(NULL, NULL) ;

	      g_hash_table_insert(types, GUINT_TO_POINTER(zMapStyleGetUniqueID(style)), (gpointer)style) ;
	      num_types++ ;
	    }

	}

      if (!num_types)
	{
	  result = FALSE ;

	  setErrMsg(server,  g_strdup("Styles found on server but they could not be parsed, check the log.")) ;
	}
      else
	{
	  result = TRUE ;

	  if (!parse_func_in)
	    {
	      *types_out = types ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      g_list_foreach(types, stylePrintCB, NULL) ; /* debug */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	    }
	}

      g_free(reply) ;
      reply = NULL ;
    }

  g_free(acedb_request) ;

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
static ZMapServerResponseType findMethods(AcedbServer server, char *search_str, int *num_found_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;


  if (search_str)
    {
      command = search_str ;
    }
  else
    {
      /* Get all the methods or styles into the current keyset on the server. */
      if (server->has_new_tags)
	command = "query find zmap_style" ;
      else
	command = "query find method" ;
    }

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
		{
		  if (num_found_out)
		    *num_found_out = num_methods ;

		  result = ZMAP_SERVERRESPONSE_OK ;
		}
	      else
		setErrMsg(server,  g_strdup_printf("Expected to find %s objects but found none.",
						   (server->has_new_tags ? "ZMap_style" : "Method"))) ;
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
 * etc
 * Style
 * <white space only lines>
 * more methods....
 *
 * Only called if we are using zmap_styles. This parses the method looking
 * for Style tags, returns FALSE if they are invalid and logs the error.
 *
 * We also record the Remark if there is one.
 *
 * The function also returns a pointer to the blank line that ends the current
 * method.
 *
 */
static gboolean parseMethodStyleNames(AcedbServer server, char *method_str_in,
				      char **end_pos, gpointer user_data)
{
  gboolean result = FALSE ;
  char *method_str = method_str_in ;
  char *next_line = method_str ;
  GetMethodsStyles get_sets = (GetMethodsStyles)user_data ;
  char *name = NULL, *zmap_style = NULL, *col_parent = NULL, *remark = NULL ;
  int obj_lines ;


  /* This should be in parse types really and then parsetypes should move on to next obj. */
  if (!g_str_has_prefix(method_str, "Method : "))
    return FALSE ;


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

	  if (!name)
	    {
	      result = FALSE ;
	      break ;
	    }
	  else
	    {
	      result = TRUE ;
	    }
	}
      else if (server->has_new_tags && g_ascii_strcasecmp(tag, "Style") == 0)
	{
	  /* Format for this tag:   Style     "some_zmap_style" */
	  zmap_style = strtok_r(NULL, "\"", &line_pos) ; /* Skip '"' */
	  zmap_style = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;

	  if (!zmap_style)
	    {
	      result = FALSE ;
	      break ;
	    }
	  else
	    {
	      result = TRUE ;
	    }
	}
      else if (server->has_new_tags && g_ascii_strcasecmp(tag, COL_PARENT) == 0)
	{
	  /* Format for this tag:   Column_parent     "some_method" */
	  if ((col_parent = strtok_r(NULL, "\"", &line_pos))) /* Skip '"' */
	    col_parent = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (server->has_new_tags && g_ascii_strcasecmp(tag, "Remark") == 0)
	{
	  /* Line format:    Remark "possibly quite long bit of text"  */

	  remark = strtok_r(NULL, "\"", &line_pos) ;
	  remark = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}


    }
  while (++obj_lines && **end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;

  /* acedb can have empty objects which consist of a first line only. */
  if (obj_lines == 1)
    {
      result = FALSE ;
    }

  /* If we failed while processing a method we won't have reached the end of the current
   * method paragraph so we need to skip to the end so the next method can be processed. */
  if (!result)
    {
      while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;
    }

  if (result)
    {
      GQuark method_id = 0, style_id = 0, text_id = 0, feature_set_id ;
      ZMapFeatureSource source_data ;

      /* name/style are mandatory, remark is optional. */
      method_id = zMapStyleCreateID(name) ;
      style_id = zMapStyleCreateID(zmap_style) ;
      if (remark)
	text_id = g_quark_from_string(remark) ;

      get_sets->feature_methods = g_list_append(get_sets->feature_methods,
						GINT_TO_POINTER(method_id)) ;

      get_sets->required_styles = g_list_append(get_sets->required_styles,
						GINT_TO_POINTER(style_id)) ;

      if (col_parent)
	feature_set_id = zMapStyleCreateID(col_parent) ;
      else
	feature_set_id = zMapStyleCreateID(name) ;

      zMap_g_hashlist_insert(get_sets->set_2_styles, feature_set_id, GINT_TO_POINTER(style_id)) ;

      /* Record mappings we need later for parsing features. */
      source_data = g_new0(ZMapFeatureSourceStruct, 1) ;
      source_data->source_id = method_id ;
      source_data->style_id = style_id ;
      source_data->source_text = text_id ;

      g_hash_table_insert(server->method_2_data, GINT_TO_POINTER(method_id), source_data) ;
    }
  else
    {
      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
		     "Feature Set \"%s\" : Method obj for feature set does not have a valid ZMap_style"
		     " so this feature set will be excluded.", name) ;
    }


  g_free(name) ;
  g_free(zmap_style) ;
  g_free(remark) ;

  return result ;
}



/* The method string should be of the form:
 *
 * Method : "wublastx_briggsae"
 * Remark	 "wublastx search of C. elegans genomic clones vs C. briggsae peptides"
 * Colour	 LIGHTGREEN
 * Frame_sensitive
 * Show_up_strand
 * etc
 * Column_parent | Column_child
 * Style
 * <white space only lines>
 * more methods....
 *
 * Only called if we are using zmap_styles. Parses the method and:
 *
 * if Column_child and _no_ Style tag found then the name of the child method
 * is added to feature_methods_out and TRUE is returned.
 *
 * or if _no_ Column_XXX tags and a Style tag is found then the name of _this_ method
 * is added to feature_methods_out and TRUE is returned.
 *
 * otherwise returns FALSE if method is a Column_parent or there is an error (logs any errors).
 *
 * The function also returns a pointer to the blank line that ends the current
 * method.
 *
 */
static gboolean parseMethodColGroupNames(AcedbServer server, char *method_str_in,
					 char **end_pos, gpointer user_data)
{
  gboolean result = TRUE ;
  char *method_str = method_str_in ;
  char *next_line = method_str ;
  GetMethodsStyles get_sets = (GetMethodsStyles)user_data ;
  GList *feature_sets, *method_list, *child_list = NULL ;
  char *name = NULL, *column_parent = NULL, *column_child = NULL, *style = NULL, *remark = NULL ;
  int obj_lines = 0 ;					    /* Used to detect empty objects. */


  if (!g_str_has_prefix(method_str, "Method : "))
    {
      get_sets->error = TRUE ;
      return FALSE ;
    }

  feature_sets = get_sets->feature_set_methods ;
  method_list = get_sets->feature_methods ;

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
      else if (g_ascii_strcasecmp(tag, COL_PARENT) == 0)
	{
	  /* Format for this tag:   Column_parent     "some_method_object" */
	  column_parent = strtok_r(NULL, "\"", &line_pos) ; /* Skip '"' */
	  column_parent = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, COL_CHILD) == 0)
	{
	  /* Format for this tag:   Column_child     "some_method_object",
	   * keep a list of all children found. */
	  column_child = strtok_r(NULL, "\"", &line_pos) ;  /* Skip '"' */
	  column_child = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;

	  child_list = g_list_append(child_list, GINT_TO_POINTER(zMapStyleCreateID(column_child))) ;

	  g_free(column_child) ;
	}
      else if (g_ascii_strcasecmp(tag, STYLE) == 0)
	{
	  /* Format for this tag:   Style     "some_style_object" */
	  style = strtok_r(NULL, "\"", &line_pos) ; /* Skip '"' */
	  style = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Remark") == 0)
	{
	  /* Line format:    Remark "possibly quite long bit of text"  */

	  remark = strtok_r(NULL, "\"", &line_pos) ;
	  remark = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
    }
  while (++obj_lines && **end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;


  /* acedb can have empty objects which consist of a first line only. */
  if (obj_lines == 1)
    {
      result = FALSE ;
      get_sets->error = TRUE ;
    }


  if (!result)
    {
      /* If we failed while processing a method we won't have reached the end of the current
       * method paragraph so we need to skip to the end so the next method can be processed. */

      while (**end_pos != '\n' && (next_line = strtok_r(NULL, "\n", end_pos))) ;
    }
  else
    {
      if (column_parent)
	{
	  /* None of these methods should be true "child" methods, they should either be parent
	   * methods or feature set methods in their own right. */

	  result = FALSE ;
	}
      else
	{
	  if ((child_list && !style) || (!child_list && style))
	    {
	      HashFeatureSetStruct hash_data = {0} ;
	      GQuark feature_set_id ;

	      feature_set_id = zMapStyleCreateID(name) ;

	      hash_data.feature_set_id = feature_set_id ;
	      hash_data.remark = remark ;
	      hash_data.method_2_feature_set = server->method_2_feature_set ;

	      /* Add this feature_set to valid list. */
	      feature_sets = g_list_append(feature_sets, GINT_TO_POINTER(feature_set_id)) ;

	      if (!child_list)
		{
		  addMethodCB(GINT_TO_POINTER(zMapStyleCreateID(name)), &hash_data) ;

		  /* This has no children but has it's own features so add it to the feature
		   * method list. */
		  method_list = g_list_append(method_list, GINT_TO_POINTER(g_quark_from_string(name))) ;
		}
	      else
		{
		  g_list_foreach(child_list, addMethodCB, &hash_data) ;

		  /* Add all the child methods to the feature method list. */
		  method_list = g_list_concat(method_list, child_list) ; /* Subsumes child list, no
									    need to free. */

		  child_list = NULL ;
		}

	      get_sets->feature_set_methods = feature_sets ;
	      get_sets->feature_methods = method_list ;
	    }
	  else
	    {
	      zMapLogWarning("Method \"%s\" ignored, Column Methods should either have " COL_CHILD
                             " or " STYLE " tags, not both.", name) ;

	      result = FALSE ;
	    }
	}
    }

  if (child_list)
    g_list_free(child_list) ;
  g_free(name) ;
  g_free(column_parent) ;
  g_free(style) ;

  return result ;
}


/* GFunc() to add child entries to our hash of methods to feature_sets. */
static void addMethodCB(gpointer data, gpointer user_data)
{
  GQuark child_id = GPOINTER_TO_INT(data) ;
  HashFeatureSet hash_data = (HashFeatureSet)user_data ;
  ZMapFeatureSetDesc set_data ;


  set_data = g_new0(ZMapFeatureSetDescStruct, 1) ;
  set_data->column_ID = set_data->column_id = hash_data->feature_set_id ;
  set_data->feature_set_text = hash_data->remark ;

  g_hash_table_insert(hash_data->method_2_feature_set,
		      GINT_TO_POINTER(child_id),
		      set_data) ;

  return ;
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
  ZMapStyleBumpMode default_bump_mode = ZMAPBUMP_OVERLAP, curr_bump_mode = ZMAPBUMP_UNBUMP ;
  double width = -999.0 ;				    /* this is going to cause problems.... */
  gboolean strand_specific = FALSE, show_up_strand = FALSE ;
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID ;
  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;
  gboolean displayable = TRUE ;
  ZMapStyleColumnDisplayState col_state = ZMAPSTYLE_COLDISPLAY_INVALID ;
  double min_mag = 0.0, max_mag = 0.0 ;
  gboolean score_set = FALSE ;
  double min_score = 0.0, max_score = 0.0 ;
  gboolean score_by_histogram = FALSE ;
  double histogram_baseline = 0.0 ;
  gboolean status = TRUE, outline_flag = FALSE, directional_end = FALSE, gaps = FALSE, join_aligns = FALSE ;
  gboolean deferred_flag = FALSE;
  int obj_lines ;
  int between_align_error = 0 ;


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
      else if (g_ascii_strcasecmp(tag, "Immediate") == 0)
	{
	  deferred_flag = FALSE ;
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
      else if (g_ascii_strcasecmp(tag, "Bump") == 0)
	{
	  default_bump_mode = ZMAPBUMP_OVERLAP ;
	  curr_bump_mode = ZMAPBUMP_UNBUMP ;
	}
      else if (g_ascii_strcasecmp(tag, "Bumpable") == 0
	       || g_ascii_strcasecmp(tag, "Cluster") == 0)
	{
	  default_bump_mode = curr_bump_mode = ZMAPBUMP_NAME_BEST_ENDS ;
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
      else if (g_ascii_strcasecmp(tag, "Show_up_strand") == 0)
	show_up_strand = TRUE ;
      else if (g_ascii_strcasecmp(tag, "Frame_sensitive") == 0)
	frame_mode = ZMAPSTYLE_3_FRAME_AS_WELL ;

      else if (g_ascii_strcasecmp(tag, "No_display") == 0)
	{
	  /* Objects that have the No_display tag set should not be shown at all. */
	  displayable = FALSE ;

	  /* If No_display is the first tag in the models file we end
	   * up breaking here and obj_lines == 1 forcing status =
	   * FALSE later on. */
#ifdef CANT_BREAK_HERE
	  break ;
#endif
	}
      else if (g_ascii_strcasecmp(tag, "init_hidden") == 0)
	{
	  col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
	}
      else if (g_ascii_strcasecmp(tag, "Directional_ends") == 0)
	{
	  directional_end = TRUE ;
	}
      else if (g_ascii_strcasecmp(tag, "Gapped") == 0)
	{
	  gaps = TRUE ;
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

  /* Set some final method stuff and create the ZMap style. */
  if (status)
    {
      /* NOTE that style is created with the method name, NOT the column_group, column
       * names are independent of method names, they may or may not be the same.
       * Also, there is no way of deriving the mode from the acedb method object
       * currently, we have to set it later. */
      style = zMapStyleCreate(name, remark) ;

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
	    zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				background, foreground, outline) ;

	  if (cds_colour)
	    {
	      zMapStyleSetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  NULL, NULL, cds_colour) ;
	      g_free(cds_colour);
	      cds_colour = NULL;
	    }
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

      if (strand_specific)
	zMapStyleSetStrandSpecific(style, strand_specific) ;

      if (show_up_strand)
	zMapStyleSetStrandShowReverse(style, show_up_strand) ;

      if (frame_mode)
	zMapStyleSetFrameMode(style, frame_mode) ;

      zMapStyleInitBumpMode(style, default_bump_mode, curr_bump_mode) ;

      if (gff_source || gff_feature)
	zMapStyleSetGFF(style, gff_source, gff_feature) ;

      zMapStyleSetDisplayable(style, displayable) ;

      if (col_state != ZMAPSTYLE_COLDISPLAY_INVALID)
	zMapStyleSetDisplay(style, col_state) ;

      if(directional_end)
        zMapStyleSetEndStyle(style, directional_end);

      /* Current setting is for gaps to be parsed but they will only
       * be displayed when the feature is bumped. */
      if (gaps)
	zMapStyleSetGappedAligns(style, TRUE, FALSE) ;

      if (join_aligns)
	zMapStyleSetJoinAligns(style, between_align_error) ;
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
 * Description   "Alleles in WormBase represent small sequence mutations...etc"
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
  char *name = NULL, *description = NULL, *parent = NULL,
    *colour = NULL, *foreground = NULL,
    *gff_source = NULL, *gff_feature = NULL,
    *column_group = NULL, *orig_style = NULL ;
  gboolean width_set = FALSE ;
  gboolean deferred = FALSE ;
  double width = 0.0 ;
  gboolean strand_specific = FALSE, show_up_strand = FALSE ;
  ZMapStyle3FrameMode frame_mode = ZMAPSTYLE_3_FRAME_INVALID ;
  ZMapStyleMode mode = ZMAPSTYLE_MODE_INVALID ;
//  ZMapStyleGlyphMode glyph_mode = ZMAPSTYLE_GLYPH_INVALID ;
  gboolean displayable_set = TRUE, displayable = TRUE,
    show_when_empty_set = FALSE, show_when_empty = FALSE ;
  ZMapStyleColumnDisplayState col_state = ZMAPSTYLE_COLDISPLAY_INVALID ;
  double min_mag = 0.0, max_mag = 0.0 ;
  gboolean directional_end_set = FALSE, directional_end = FALSE ;
  gboolean allow_misalign = FALSE ;
  gboolean join_aligns = FALSE ;
  int join_align = 0 ;
  gboolean parse_gaps = FALSE, show_gaps = FALSE ;
  gboolean bump_mode_set = FALSE, bump_default_set = FALSE ;
  ZMapStyleBumpMode default_bump_mode = ZMAPBUMP_INVALID, curr_bump_mode = ZMAPBUMP_INVALID ;
  gboolean bump_spacing_set = FALSE ;
  double bump_spacing = 0.0 ;
  gboolean bump_fixed = FALSE ;
  gboolean some_colours = FALSE ;
  StyleFeatureColoursStruct style_colours = {{NULL}, {NULL}} ;
  gboolean some_frame0_colours = FALSE, some_frame1_colours = FALSE, some_frame2_colours = FALSE ;
  StyleFeatureColoursStruct frame0_style_colours = {{NULL}, {NULL}},
    frame1_style_colours = {{NULL}, {NULL}}, frame2_style_colours = {{NULL}, {NULL}} ;
  gboolean some_CDS_colours = FALSE ;
  StyleFeatureColoursStruct CDS_style_colours = {{NULL}, {NULL}} ;
  double min_score = 0.0, max_score = 0.0 ;
  gboolean score_by_width, score_is_percent ;
  gboolean histogram = FALSE, score_set = FALSE ;
  double histogram_baseline = 0.0 ;
  gboolean pfetchable = FALSE ;
  ZMapStyleBlixemType blixem_type = ZMAPSTYLE_BLIXEM_INVALID ;


  if (g_ascii_strncasecmp(style_str, "ZMap_style : ", strlen("ZMap_style : ")) != 0)
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
	}


      if (g_ascii_strcasecmp(tag, "Description") == 0)
	{
	  /* Line format:    Description "possibly quite long bit of text"  */

	  description = strtok_r(NULL, "\"", &line_pos) ;
	  description = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Style_parent") == 0)
	{
	  parent = strtok_r(NULL, "\"", &line_pos) ;
	  parent = g_strdup(strtok_r(NULL, "\"", &line_pos)) ;
	}
      else if (g_ascii_strcasecmp(tag, "Immediate") == 0)
	{
	  deferred = FALSE ;
	}


      /* OK, THIS MODE STUFF IS NO GOOD, WE NEED TO USE THE STYLE CALL THAT WILL HAVE THE LATEST
       * ENUMS BUILT IN OTHERWISE EVERYTIME WE ADD A NEW TYPE ALL THIS FAILS.... */

      /* Grab the mode... */
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
	      if (g_ascii_strcasecmp(tmp_next_tag, "CDS_colour") == 0)
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
	      if (g_ascii_strcasecmp(align_type, "Join_align") == 0)
		{
		  join_aligns = TRUE ;

		  value = strtok_r(NULL, " ", &line_pos) ;

		  if (value && !(status = zMapStr2Int(value, &join_align)))
		    {
		      zMapLogWarning("Style \"%s\": Bad Join_align factor \"Alignment Join_align %s\"",
				     name, value) ;
		      break ;
		    }
		}
	      else if (g_ascii_strcasecmp(align_type, "Parse") == 0)
		parse_gaps = TRUE ;
	      else if (g_ascii_strcasecmp(align_type, "Show") == 0)
		parse_gaps = show_gaps = TRUE ;
	      else if (g_ascii_strcasecmp(align_type, "Allow_misalign") == 0)
		allow_misalign = TRUE ;
	      else if (g_ascii_strcasecmp(align_type, "Pfetchable") == 0)
		pfetchable = TRUE ;
	      else if (g_ascii_strcasecmp(align_type, "Blixem_N") == 0)
		blixem_type = ZMAPSTYLE_BLIXEM_N ;
	      else if (g_ascii_strcasecmp(align_type, "Blixem_X") == 0)
		blixem_type = ZMAPSTYLE_BLIXEM_X ;
	      else
		zMapLogWarning("Style \"%s\": Unknown tag \"%s\" for \"Alignment\" specified in style: %s",
			       name, align_type, name) ;

	    }
	}
      else if (g_ascii_strcasecmp(tag, "Sequence") == 0
	       || g_ascii_strcasecmp(tag, "Peptide") == 0)
	{
	  mode = ZMAPSTYLE_MODE_SEQUENCE ;
	}
      else if (g_ascii_strcasecmp(tag, "Assembly_path") == 0)
	{
	  mode = ZMAPSTYLE_MODE_ASSEMBLY_PATH ;
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
//	      glyph_mode = ZMAPSTYLE_GLYPH_3FRAME_SPLICE ;
	    }
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
      else if (g_ascii_strcasecmp(tag, "Bump_initial") == 0 || g_ascii_strcasecmp(tag, "Bump_default") == 0)
	{
	  char *tmp_next_tag ;
	  ZMapStyleBumpMode *tmp_bump ;

	  if (g_ascii_strcasecmp(tag, "Bump_initial") == 0)
	    {
	      tmp_bump = &curr_bump_mode ;
	      bump_mode_set = TRUE ;
	    }
	  else
	    {
	      tmp_bump = &default_bump_mode ;
	      bump_default_set = TRUE ;
	    }

	  tmp_next_tag = strtok_r(NULL, " ", &line_pos) ;

	  if (g_ascii_strcasecmp(tmp_next_tag, "Unbump") == 0)
	    *tmp_bump = ZMAPBUMP_UNBUMP ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Overlap") == 0)
	    *tmp_bump = ZMAPBUMP_OVERLAP ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Navigator") == 0)
	    *tmp_bump = ZMAPBUMP_NAVIGATOR ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Start_position") == 0)
	    *tmp_bump = ZMAPBUMP_START_POSITION ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Alternating") == 0)
	    *tmp_bump = ZMAPBUMP_ALTERNATING ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "All") == 0)
	    *tmp_bump = ZMAPBUMP_ALL ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Name") == 0)
	    *tmp_bump = ZMAPBUMP_NAME ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Name_interleave") == 0)
	    *tmp_bump = ZMAPBUMP_NAME_INTERLEAVE ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Name_no_interleave") == 0)
	    *tmp_bump = ZMAPBUMP_NAME_NO_INTERLEAVE ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Name_colinear") == 0)
	    *tmp_bump = ZMAPBUMP_NAME_COLINEAR ;
	  else if (g_ascii_strcasecmp(tmp_next_tag, "Name_best_ends") == 0)
	    *tmp_bump = ZMAPBUMP_NAME_BEST_ENDS ;
	  else
	    zMapLogWarning("Style \"%s\": Bad bump spec: %d", name, *tmp_bump) ;
	}
      else if (g_ascii_strcasecmp(tag, "Bump_spacing") == 0)
	{
	  char *value ;

	  value = strtok_r(NULL, " ", &line_pos) ;

	  if ((status = zMapStr2Double(value, &bump_spacing)))
	    {
	      bump_spacing_set = TRUE ;
	    }
	  else
	    {
	      zMapLogWarning("Style \"%s\": No value for \"Bump_spacing\".", name) ;
	      break ;
	    }
	}
      else if (g_ascii_strcasecmp(tag, "Bump_fixed") == 0)
	{
	  bump_fixed = TRUE ;
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
      else if (g_ascii_strcasecmp(tag, "Strand_sensitive") == 0)
	strand_specific = TRUE ;
      else if (g_ascii_strcasecmp(tag, "Show_up_strand") == 0)
	show_up_strand = TRUE ;
      else if (g_ascii_strcasecmp(tag, "Frame_sensitive") == 0)
	frame_mode = ZMAPSTYLE_3_FRAME_AS_WELL ;
      else if (g_ascii_strcasecmp(tag, "Show_only_as_3_frame") == 0)
	frame_mode = ZMAPSTYLE_3_FRAME_ONLY_3 ;
      else if (g_ascii_strcasecmp(tag, "Show_only_as_1_column") == 0)
	frame_mode = ZMAPSTYLE_3_FRAME_ONLY_1 ;
      else if (g_ascii_strcasecmp(tag, "Not_displayable") == 0)
	{
	  /* Objects that have the Not_displayable tag set should not be shown at all. */
	  displayable_set = displayable = FALSE ;

	  break ;
	}
      else if (g_ascii_strcasecmp(tag, "Hide") == 0)
	{
	  col_state = ZMAPSTYLE_COLDISPLAY_HIDE ;
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
	  score_set = TRUE;

	  if (!(status = zMapStr2Double(value, &min_score)))
	    {
	      zMapLogWarning("Style \"%s\": Bad value for \"Score_bounds\".", name) ;
	      score_set = FALSE;
	      break ;
	    }
	  else
	    {
	      value = strtok_r(NULL, " ", &line_pos) ;

	      if (!(status = zMapStr2Double(value, &max_score)))
		{
		  zMapLogWarning("Style \"%s\": Bad value for \"Score_bounds\".", name) ;
		  score_set = FALSE;
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


  /* Create the style and add all the bits to it. */
  if (status)
    {
      /* NOTE that style is created with the style name, NOT the column_group, column
       * names are independent of style names, they may or may not be the same.
       * Also, there is no way of deriving the mode from the acedb style object
       * currently, we have to set it later. */
      style = zMapStyleCreate(name, description) ;

      if (mode != ZMAPSTYLE_MODE_INVALID)
	zMapStyleSetMode(style, mode) ;

//      if (glyph_mode != ZMAPSTYLE_GLYPH_INVALID)
//	zMapStyleSetGlyphMode(style, glyph_mode) ;

      if (parent)
	zMapStyleSetParent(style, parent) ;


      if (some_colours)
	{
	  /* May need to put some checking code here to test which colours set. */
	  zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
			      style_colours.normal.fill, style_colours.normal.draw, style_colours.normal.border) ;
	  zMapStyleSetColours(style, STYLE_PROP_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
			      style_colours.selected.fill, style_colours.selected.draw, style_colours.selected.border) ;
	}

      if (some_frame0_colours || some_frame1_colours || some_frame2_colours)
	{
	  if (some_frame0_colours && some_frame1_colours && some_frame2_colours)
	    {
	      /* May need to put some checking code here to test which colours set. */
	      zMapStyleSetColours(style, STYLE_PROP_FRAME0_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame0_style_colours.normal.fill, frame0_style_colours.normal.draw, frame0_style_colours.normal.border) ;
	      zMapStyleSetColours(style, STYLE_PROP_FRAME0_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  frame0_style_colours.selected.fill, frame0_style_colours.selected.draw, frame0_style_colours.selected.border) ;

	      zMapStyleSetColours(style, STYLE_PROP_FRAME1_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame1_style_colours.normal.fill, frame1_style_colours.normal.draw, frame1_style_colours.normal.border) ;
	      zMapStyleSetColours(style, STYLE_PROP_FRAME1_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
				  frame1_style_colours.selected.fill, frame1_style_colours.selected.draw, frame1_style_colours.selected.border) ;

	      zMapStyleSetColours(style, STYLE_PROP_FRAME2_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
				  frame2_style_colours.normal.fill, frame2_style_colours.normal.draw, frame2_style_colours.normal.border) ;
	      zMapStyleSetColours(style, STYLE_PROP_FRAME2_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
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
	  zMapStyleSetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_NORMAL,
			      CDS_style_colours.normal.fill, CDS_style_colours.normal.draw, CDS_style_colours.normal.border) ;
	  zMapStyleSetColours(style, STYLE_PROP_TRANSCRIPT_CDS_COLOURS, ZMAPSTYLE_COLOURTYPE_SELECTED,
			      CDS_style_colours.selected.fill, CDS_style_colours.selected.draw, CDS_style_colours.selected.border) ;
	}

      if (width_set)
	zMapStyleSetWidth(style, width) ;


      if (min_mag || max_mag)
	zMapStyleSetMag(style, min_mag, max_mag) ;


      /* OTHER SCORE STUFF MUST BE SET HERE.... */

      /* Note that we require bounds to be set for graphing.... */
      if (score_set)
	{
	  ZMapStyleGraphMode graph_mode = ZMAPSTYLE_GRAPH_HISTOGRAM; /* HARD CODED! */

	  if (histogram)
	    zMapStyleSetGraph(style, graph_mode, min_score, max_score, histogram_baseline) ;
	  else if(min_score || max_score)
	    zMapStyleSetScore(style, min_score, max_score) ;
	}

      if (strand_specific)
	zMapStyleSetStrandSpecific(style, strand_specific) ;

      if (show_up_strand)
	zMapStyleSetStrandShowReverse(style, show_up_strand) ;

      if (frame_mode)
	zMapStyleSetFrameMode(style, frame_mode) ;

      if (bump_mode_set || bump_default_set)
	zMapStyleInitBumpMode(style, default_bump_mode, curr_bump_mode) ;

      if (bump_spacing_set)
	zMapStyleSetBumpSpace(style, bump_spacing) ;

      if (bump_fixed)
	zMapStyleSet(style,
		     ZMAPSTYLE_PROPERTY_BUMP_FIXED, bump_fixed,
		     NULL) ;

      if (gff_source || gff_feature)
	zMapStyleSetGFF(style, gff_source, gff_feature) ;

      if (displayable_set)
	zMapStyleSetDisplayable(style, displayable) ;

      if (col_state != ZMAPSTYLE_COLDISPLAY_INVALID)
	zMapStyleSetDisplay(style, col_state) ;

      if (show_when_empty_set)
	zMapStyleSetShowWhenEmpty(style, show_when_empty) ;

      if (directional_end_set)
        zMapStyleSetEndStyle(style, directional_end);

      if (join_aligns)
	zMapStyleSetJoinAligns(style, join_align) ;

      if (parse_gaps)
	zMapStyleSetGappedAligns(style, parse_gaps, show_gaps) ;

      if (pfetchable)
	zMapStyleSetPfetch(style, pfetchable) ;

      /* Should be building the list dynamically.....could do this on the create step using
       * my zMapStyleCreateV() function. */
      if (blixem_type)
	zMapStyleSet(style,
		     ZMAPSTYLE_PROPERTY_ALIGNMENT_BLIXEM, blixem_type,
		     ZMAPSTYLE_PROPERTY_ALIGNMENT_ALLOW_MISALIGN, allow_misalign,
		     NULL) ;
    }


  /* Clean up, note g_free() does nothing if given NULL. */
  g_free(name) ;
  g_free(description) ;
  g_free(colour) ;
  g_free(foreground) ;
  g_free(column_group) ;
  g_free(orig_style) ;
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
static ZMapServerResponseType getObjNames(AcedbServer server, GList **style_names_out)
{
  ZMapServerResponseType result = ZMAP_SERVERRESPONSE_REQFAIL ;
  char *command ;
  char *acedb_request = NULL ;
  void *reply = NULL ;
  int reply_len = 0 ;

  /* List all the methods in the current keyset on the serve. */
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
	  setErrMsg(server,  g_strdup_printf("No styles found.")) ;
	  result = ZMAP_SERVERRESPONSE_REQFAIL ;
	}

      g_free(reply) ;
      reply = NULL ;
    }
  else
    result = server->last_err_status ;

  g_free(acedb_request) ;


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
      if (!sequenceRequest(get_features, feature_block))
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




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void stylePrintCB(gpointer data, gpointer user_data)
{
  ZMapFeatureTypeStyle style = (ZMapFeatureTypeStyle)data ;

  printf("%s (%s)\n", g_quark_to_string(zMapStyleGetID(style)),
	 g_quark_to_string(zMapStyleGetUniqueID(style))) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

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
      if (sequence->type == ZMAPSEQUENCE_DNA)
	command = "find sequence" ;
      else
	command = "find peptide" ;

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
		  if ((num_objs = getFoundObj(next_line)) == 1)
		    {
		      result = ZMAP_SERVERRESPONSE_OK ;
		    }
		  else
		    {
		      setErrMsg(server, g_strdup_printf("Expected to find 1 sequence object"
							"named \"%s\" but found %d.",
							g_quark_to_string(sequence->name), num_objs)) ;

		      result = ZMAP_SERVERRESPONSE_REQFAIL ;
		    }

		  break ;
		}
	    }

	  g_free(reply) ;
	  reply = NULL ;
	}

      /* All ok ?  Then get the sequence.... */
      if (result == ZMAP_SERVERRESPONSE_OK)
	{
	  if (sequence->type == ZMAPSEQUENCE_DNA)
	    command = "dna -u" ;
	  else
	    command = "peptide -u" ;

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

	      result = ZMAP_SERVERRESPONSE_OK ;
	    }
	  else
	    {
	      setErrMsg(server, g_strdup_printf("Failed to fetch %s sequence for object \"%s\".",
						(sequence->type == ZMAPSEQUENCE_DNA ? "nucleotide" : "peptide"),
						g_quark_to_string(sequence->name))) ;

	      result = ZMAP_SERVERRESPONSE_REQFAIL ;
	    }
	}

      next_seq = g_list_next(next_seq) ;
    }

  g_string_free(acedb_request, TRUE) ;


  return result ;
}


/* Checks that each name in query_names_inout can be found in reference_names, if a name
 * is not found then it is removed from query_names_inout (i.e. query_names_inout could be
 * NULL on return).
 *
 * Returns the number of names missing.
 *
 * Note function assumes names occur only once in each list.
 */
static int equaliseLists(AcedbServer server, GList **query_names_inout, GList *reference_names,
			 char *query_name, char *reference_name)
{
  int num_found = 0 ;
  int num_query ;
  GString *missing ;
  GList *curr ;
  GList *query_names ;

  query_names = *query_names_inout ;

  num_query = g_list_length(query_names) ;

  missing = g_string_sized_new(1000) ;

  /* Must loop round by steam because we are removing links. */
  curr = query_names ;
  do
    {
      if ((g_list_find_custom(reference_names, curr->data, quarkCaseCmp)))
	{
	  curr = curr->next ;
	  num_found++ ;
	}
      else
	{
	  GList *tmp ;

	  g_string_append_printf(missing, " \"%s\"", g_quark_to_string(GPOINTER_TO_INT(curr->data))) ;

	  tmp = curr->next ;				    /* Best move on before removing link. */

	  query_names = g_list_delete_link(query_names, curr) ;

	  curr = tmp ;
	}

    } while (curr) ;

  /* Log any missing methods. */
  if (!num_found)
    {
      ZMAPSERVER_LOG(Critical, ACEDB_PROTOCOL_STR, server->host,
		     "Complete %s -> %s mismatch, %d %s specified but none found in %s list. Missing %s were: %s",
		     query_name, reference_name, num_query, query_name, reference_name,
		     query_name, missing->str) ;
    }
  else if (num_found < num_query)
    {
      ZMAPSERVER_LOG(Warning, ACEDB_PROTOCOL_STR, server->host,
		     "Partial %s -> %s mismatch, %d %s specified but only %d found in %s list. Missing %s were: %s",
		     query_name, reference_name, num_query, query_name, num_found, reference_name,
		     query_name, missing->str) ;
    }

  g_string_free(missing, TRUE) ;

  /* Return the names list. */
  *query_names_inout = query_names ;

  return num_found ;
}


/* A GCompareFunc() to compare names in a case independent way in two lists of GQuarks. */
static gint quarkCaseCmp(gconstpointer a, gconstpointer b)
{
  gint result ;

  result = g_ascii_strcasecmp(g_quark_to_string(GPOINTER_TO_INT(a)), g_quark_to_string(GPOINTER_TO_INT(b))) ;

  return result ;
}



/* It's possible for us to have reported an error and then another error to come along. */
static void setErrMsg(AcedbServer server, char *new_msg)
{
  if (server->last_err_msg)
    g_free(server->last_err_msg) ;

  server->last_err_msg = new_msg ;

  return ;
}


/* Reset status/err_msg, needs doing before each command otherwise we can end up seeing the wrong
 * message. */
static void resetErr(AcedbServer server)
{
  if (server->last_err_msg)
    {
      g_free(server->last_err_msg) ;
      server->last_err_msg = NULL ;
    }

  server->last_err_status = ACECONN_OK ;

  return ;
}



static char *get_url_query_value(char *full_query, char *key)
{
  char *value = NULL,
    **split   = NULL,
    **ptr     = NULL ;

  if(full_query != NULL)
    {
      split = ptr = g_strsplit(full_query, "&", 0);

      while(ptr && *ptr != '\0')
	{
	  char **key_value = NULL, **kv_ptr;
	  key_value = kv_ptr = g_strsplit(*ptr, "=", 0);
	  if(key_value[0] && (g_ascii_strcasecmp(key, key_value[0]) == 0))
	    value = g_strdup(key_value[1]);
	  g_strfreev(kv_ptr);
	  ptr++;
	}

      g_strfreev(split);
    }

  return value;
}

static gboolean get_url_query_boolean(char *full_query, char *key)
{
  gboolean result = FALSE;
  char *value = NULL;

  if((value = get_url_query_value(full_query, key)))
    {
      if(g_ascii_strcasecmp("true", value) == 0)
	result = TRUE;
      g_free(value);
    }

  return result;
}


/* A GDestroyNotify() to free the method data structs in the method_2_data hash table. */
static void freeDataCB(gpointer data)
{
  ZMapFeatureSetDesc set_data = (ZMapFeatureSetDesc)data ;

  g_free(set_data) ;

  return ;
}

/* A GDestroyNotify() to free the method data structs in the method_2_data hash table. */
static void freeSetCB(gpointer data)
{
  ZMapFeatureSource source_data = (ZMapFeatureSource)data ;

  g_free(source_data) ;

  return ;
}
