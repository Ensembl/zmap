/*  File: zmapServerProtocol.h
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
 * 	Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *
 * Description: Interface for creating requests and passing them from 
 *              the master thread to slave threads. Requests are via
 *              structs that give all the information/fields for the request/reply.
 *              
 * HISTORY:
 * Last edited: Mar 23 11:36 2009 (edgrif)
 * Created: Wed Feb  2 11:47:16 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocol.h,v 1.21 2009-04-06 13:27:43 edgrif Exp $
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
  _(ZMAP_SERVERREQ_INVALID, , "invalid")				\
    _(ZMAP_SERVERREQ_CREATE, , "create")	/* Create a connection to a  data server. */ \
    _(ZMAP_SERVERREQ_OPEN, , "open")				    /* Open the connection. */ \
    _(ZMAP_SERVERREQ_GETSERVERINFO, , "getserverinfo")			    /* Get server information. */ \
    _(ZMAP_SERVERREQ_FEATURESETS, , "featuresets")	    /* Set/Get the feature sets. */ \
    _(ZMAP_SERVERREQ_STYLES, , "styles")	    /* Set/Get the feature styles. */ \
    _(ZMAP_SERVERREQ_NEWCONTEXT, , "newcontext")    /* Set the context. */ \
    _(ZMAP_SERVERREQ_FEATURES, , "features")	    /* Get the context features. */ \
    _(ZMAP_SERVERREQ_SEQUENCE, , "sequence")	    /* Get the context sequence. */ \
    _(ZMAP_SERVERREQ_GETSEQUENCE, , "getsequence")    /* Get an arbitrary (named) sequence. */ \
    _(ZMAP_SERVERREQ_TERMINATE, , "terminate")    /* Close and destroy the connection. */ 

ZMAP_DEFINE_ENUM(ZMapServerReqType, ZMAP_SERVER_REQ_LIST) ;


/* All requests return one of these responses. */
/* WE SHOULD ADD AN  INVALID  AT THE START BUT REQUIRES CHECKING ALL USE OF  OK  !! */
#define ZMAP_SERVER_RESPONSE_LIST(_)                         \
  _(ZMAP_SERVERRESPONSE_OK, , "ok")				      \
    _(ZMAP_SERVERRESPONSE_BADREQ, , "error in request args")		\
    _(ZMAP_SERVERRESPONSE_UNSUPPORTED, , "unsupported request")		\
    _(ZMAP_SERVERRESPONSE_REQFAIL, , "request failed")			\
    _(ZMAP_SERVERRESPONSE_TIMEDOUT, , "timed out")			\
    _(ZMAP_SERVERRESPONSE_SERVERDIED, , "server died")

ZMAP_DEFINE_ENUM(ZMapServerResponseType, ZMAP_SERVER_RESPONSE_LIST) ;


/* 
 * ALL request/response structs must replicate the generic ZMapServerReqAnyStruct
 * so that they can all be treated as the canonical ZMapServerReqAny.
 */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;
} ZMapServerReqAnyStruct, *ZMapServerReqAny ;



/* Create a connection object. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

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
} ZMapServerReqOpenStruct, *ZMapServerReqOpen ;



/* Request server attributes. NEEDS EXPANDING TO RETURN MORE SERVER INFORMATION */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  char *database_path_out ;

} ZMapServerReqGetServerInfoStruct, *ZMapServerReqGetServerInfo ;



/* Used to specify which feature sets should be retrieved or to get the list of all feature sets
 * available. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  GList *feature_sets_inout ;				    /* List of prespecified features sets or
							       NULL to get all available sets. */

  GList *required_styles_out ;				    /* May be derived from features. */

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

  /* Some styles specify the mode/type of the features they represent (e.g. "transcript like",
   * "text" etc.), zmap requires that the style mode is set otherwise the features
   * referencing that style will _not_ be displayed. This flag records whether the styles
   * from the data source have a mode or whether it needs to be inferred from the features
   * themseleves. This is completely data source dependent. */
  gboolean server_styles_have_mode ;

  GData *styles_out ;					    /* List of prespecified styles or NULL
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

  GData *styles ;					    /* Needed for some features to control
							       how they are fetched. */

  ZMapFeatureContext context ;		    /* Returned feature sets. */
} ZMapServerReqGetFeaturesStruct, *ZMapServerReqGetFeatures ;


/* Used to ask for a specific sequence(s), currently this is targetted at blixem and so some stuff
 * is targetted for that usage, although knowing the selected feature is useful for a number of
 * operations. */
typedef struct
{
  ZMapServerReqType type ;
  ZMapServerResponseType response ;

  ZMapFeature orig_feature ;				    /* The original feature which
							       triggered the request. */

  GList *sequences ;					    /* List of ZMapSequenceStruct which
							       hold name of sequence to be fetched. */

  int flags;
} ZMapServerReqGetSequenceStruct, *ZMapServerReqGetSequence ;


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
} ZMapServerReqUnion ;


/* Enum -> String function decs: const char *zMapXXXX2ExactStr(ZMapXXXXX type);  */
ZMAP_ENUM_AS_EXACT_STRING_DEC(zMapServerReqType2ExactStr, ZMapServerReqType) ;

ZMapServerReqAny zMapServerRequestCreate(ZMapServerReqType request_type, ...) ;
void zMapServerCreateRequestDestroy(ZMapServerReqAny request) ;
ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
					      void *request_in, void **reply_out,
					      char **err_msg_out) ;
ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out) ;
ZMapThreadReturnCode zMapServerDestroyHandler(void **slave_data) ;


#endif /* !ZMAP_PROTOCOL_H */
