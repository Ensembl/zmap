/*  File: zmapServerProtocol.h
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
 * originally written by:
 *              
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Interface for creating requests and passing them from
 *              the master thread to slave threads. Requests are via
 *              structs that give all the information/fields for the request/reply.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PROTOCOL_H
#define ZMAP_PROTOCOL_H

#include <glib.h>
#include <ZMap/zmapEnum.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapThreads.h>


/* Requests can be of different types with different input parameters and returning
 * different types of results. */
#define ZMAP_SERVER_REQ_LIST(_)                         \
  _(ZMAP_SERVERREQ_INVALID, , "invalid", "", "")				\
    _(ZMAP_SERVERREQ_CREATE, , "create", "create", "Create a connection to a  data server.") \
    _(ZMAP_SERVERREQ_OPEN, , "open", "open", "Open the connection.") \
    _(ZMAP_SERVERREQ_GETSERVERINFO, , "getserverinfo", "getserverinfo", "Get server information.") \
    _(ZMAP_SERVERREQ_FEATURESETS, , "featuresets", "featuresets", "Set/Get the feature sets.") \
    _(ZMAP_SERVERREQ_STYLES, , "styles", "styles", "Set/Get the feature styles.") \
    _(ZMAP_SERVERREQ_NEWCONTEXT, , "newcontext", "newcontext", "Set the context.") \
    _(ZMAP_SERVERREQ_FEATURES, , "features", "features", "Get the context features.") \
    _(ZMAP_SERVERREQ_SEQUENCE, , "sequence", "sequence", "Get the context sequence.") \
    _(ZMAP_SERVERREQ_GETSEQUENCE, , "getsequence", "getsequence", "Get an arbitrary (named) sequence.") \
    _(ZMAP_SERVERREQ_GETSTATUS, , "getstatus", "getstatus", "Get server exit code and STDERR.") \
    _(ZMAP_SERVERREQ_GETCONNECT_STATE, , "getconnect_state", "getconnect_state", "Get server connection state.") \
    _(ZMAP_SERVERREQ_TERMINATE, , "terminate", "terminate", "Close and destroy the connection.")

ZMAP_DEFINE_ENUM(ZMapServerReqType, ZMAP_SERVER_REQ_LIST) ;


/* All requests return one of these responses. */
/* WE SHOULD ADD AN  INVALID  AT THE START BUT REQUIRES CHECKING ALL USE OF  OK  !! */
#define ZMAP_SERVER_RESPONSE_LIST(_)                         \
  _(ZMAP_SERVERRESPONSE_OK, , "ok", "", "")				      \
    _(ZMAP_SERVERRESPONSE_BADREQ, , "error in request args", "", "")		\
    _(ZMAP_SERVERRESPONSE_UNSUPPORTED, , "unsupported request", "", "")		\
    _(ZMAP_SERVERRESPONSE_REQFAIL, , "request failed", "", "")			\
    _(ZMAP_SERVERRESPONSE_TIMEDOUT, , "timed out", "", "")			\
    _(ZMAP_SERVERRESPONSE_SERVERDIED, , "server died", "", "")

ZMAP_DEFINE_ENUM(ZMapServerResponseType, ZMAP_SERVER_RESPONSE_LIST) ;


/* Is server currently connected ? need error state here ??? */
#define ZMAP_SERVER_CONNECT_STATE_LIST(_)                         \
  _(ZMAP_SERVERCONNECT_STATE_INVALID, , "invalid", "", "")				\
    _(ZMAP_SERVERCONNECT_STATE_UNCONNECTED, , "unconnected", "unconnected", "No connection to server.") \
    _(ZMAP_SERVERCONNECT_STATE_CONNECTED, , "connected", "connected", "Connected to server.") \
    _(ZMAP_SERVERCONNECT_STATE_ERROR, , "error", "error", "Server is in error, state undetermined.")

ZMAP_DEFINE_ENUM(ZMapServerConnectStateType, ZMAP_SERVER_CONNECT_STATE_LIST) ;




/*
 * ALL request/response structs must replicate the generic ZMapServerReqAnyStruct
 * so that they can all be treated as the canonical ZMapServerReqAny.
 */
typedef struct ZMapServerReqAnyStructType
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;
} ZMapServerReqAnyStruct, *ZMapServerReqAny ;



/* Create a connection object. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  char *config_file ;

  ZMapURL url ;
  char *format ;
  int timeout ;
  char *version ;

} ZMapServerReqCreateStruct, *ZMapServerReqCreate ;


/* Open the connection to the server. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;
  gboolean sequence_server;         /* get DNA or not? */
  ZMapFeatureSequenceMap sequence_map;  /* which sequence + dataset */
  gint zmap_start,zmap_end;         /* start, end coords based from 1 */
} ZMapServerReqOpenStruct, *ZMapServerReqOpen ;



/* Request server attributes, these are mostly optional. */
typedef struct ZMapServerReqGetServerInfoStructType
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  char *database_name_out ;
  char *database_title_out ;
  char *database_path_out ;

  gboolean request_as_columns;      /* this is really and ACEDB indicator
                                     * it means 'please expand columns into featuresets'
                                     * it's not possible to get the acedb code to do this
                                     * easily with major restructuring
                                     */

} ZMapServerReqGetServerInfoStruct, *ZMapServerReqGetServerInfo ;




/* Used to specify which feature sets should be retrieved or to get the list of all feature sets
 * available. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  GList *feature_sets_inout ;				    /* List of prespecified features sets or
							       NULL to get all available sets,
							       these are columns so maybe we
							       should call them that. */

  GList *sources ;					    /* Optional list of sources, these
							       _must_ have a parent feature_set in
							       feature_sets_inout, if NULL then
							       _all_ the sources derived from
							       feature_sets_inout will be loaded. */

  /* Is this still needed ????? */
  GList *required_styles_out ;				    /* May be derived from features. */


  GHashTable *featureset_2_stylelist_out ;		    /* Mapping of each feature_set to all
							       the styles it requires. */

  GHashTable *featureset_2_column_inout ;			    /* Mapping of a features source to the
							       feature_set it will be placed in. */

  GHashTable *source_2_sourcedata_inout ;			    /* Mapping of features source to its
							       style and other source specific data. */

} ZMapServerReqFeatureSetsStruct, *ZMapServerReqFeatureSets ;



/* Inout struct used and/or to tell a server what styles are available or retrieve styles
 * from a server. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  char *styles_list_in ;

  char *styles_file_in ;

  GList *required_styles_in ;

  gboolean req_styles;

  /* Some styles specify the mode/type of the features they represent (e.g. "transcript like",
   * "text" etc.), zmap requires that the style mode is set otherwise the features
   * referencing that style will _not_ be displayed. This flag records whether the styles
   * from the data source have a mode or whether it needs to be inferred from the features
   * themseleves. This is completely data source dependent. */
  gboolean server_styles_have_mode ;

  GHashTable *styles_out ;					    /* List of prespecified styles or NULL
							       to get all available styles. */
} ZMapServerReqStylesStruct, *ZMapServerReqStyles ;


/* Set a context/region in a server. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  ZMapFeatureContext context ;
} ZMapServerReqNewContextStruct, *ZMapServerReqNewContext ;



/* Get features from a server. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  GHashTable *styles ;					    /* Needed for some features to control
							       how they are fetched. */

  ZMapFeatureContext context ;		    /* Returned feature sets. */

  int num_features;

} ZMapServerReqGetFeaturesStruct, *ZMapServerReqGetFeatures ;


/* Used to ask for a specific sequence(s), currently this is targetted at blixem and so some stuff
 * is probably tailored to that usage, although knowing the selected feature is useful for a number of
 * operations. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  GList *sequences ;					    /* List of ZMapSequenceStruct which
							       hold name of sequence to be fetched. */

  void *caller_data ;

} ZMapServerReqGetSequenceStruct, *ZMapServerReqGetSequence ;


typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  gint exit_code ;
  gchar *stderr_out ;

} ZMapServerReqGetStatusStruct, *ZMapServerReqGetStatus ;


typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  ZMapServerConnectStateType connect_state ;

} ZMapServerReqGetConnectStateStruct, *ZMapServerReqGetConnectState ;


typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

} ZMapServerReqTerminateStruct, *ZMapServerReqTerminate ;


/* Use if you want to include any possible struct in a struct of your own. */
typedef union
{
  ZMapServerReqAnyStruct any ;
  ZMapServerReqCreateStruct create ;
  ZMapServerReqOpenStruct open ;
  ZMapServerReqGetServerInfoStruct get_info ;
  ZMapServerReqFeatureSetsStruct get_featuresets ;
  ZMapServerReqStylesStruct get_styles ;
  ZMapServerReqNewContextStruct get_context ;
  ZMapServerReqGetFeaturesStruct get_features ;
  ZMapServerReqGetSequenceStruct get_sequence;
  ZMapServerReqGetStatusStruct get_status;
  ZMapServerReqTerminateStruct terminate;
} ZMapServerReqUnion ;



/* Enum -> String function decs: const char *zMapXXXX2ExactStr(ZMapXXXXX type);  */
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapServerReqType2ExactStr, ZMapServerReqType) ;

ZMapServerReqAny zMapServerRequestCreate(ZMapServerReqType request_type, ...) ;
void zMapServerRequestDestroy(ZMapServerReqAny request) ;
ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
					      void *request_in, void **reply_out,
					      char **err_msg_out) ;
ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out) ;
ZMapThreadReturnCode zMapServerDestroyHandler(void **slave_data) ;


/* Debug flags. */
extern gboolean zmap_server_feature2style_debug_G;
extern gboolean zmap_server_styles_debug_G;



#endif /* !ZMAP_PROTOCOL_H */
