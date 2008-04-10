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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Interface for passing server requests from the master
 *              thread to slave threads. Requests are via structs that
 *              give all the information/fields for the request/reply.
 *              
 * HISTORY:
 * Last edited: Apr 10 09:19 2008 (rds)
 * Created: Wed Feb  2 11:47:16 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocol.h,v 1.14 2008-04-10 08:37:17 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PROTOCOL_H
#define ZMAP_PROTOCOL_H

#include <glib.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapThreads.h>

/* LOTS OF THESE ARE NOT CURRENTLY USED AND SHOULD BE COMMENTED OUT UNTIL THEY ARE... */

/* Requests can be of different types with different input parameters and returning
 * different types of results. */
typedef enum
  {
    ZMAP_SERVERREQ_INVALID = 0,

    ZMAP_SERVERREQ_OPEN,				    /* Open a connection to a data server. */

    ZMAP_SERVERREQ_OPENLOAD,				    /* Open a connection and get
							       features. */

    ZMAP_SERVERREQ_GETSEQUENCE,				    /* Get a specific sequence. */

    ZMAP_SERVERREQ_GETSERVERINFO,			    /* Get server information. */

    ZMAP_SERVERREQ_STYLES,				    /* Set/Get the feature styles. */

    ZMAP_SERVERREQ_FEATURESETS,				    /* Set/Get the feature sets. */

    ZMAP_SERVERREQ_FEATURES,				    /* Get the features. */

    ZMAP_SERVERREQ_SEQUENCE,				    /* Get the sequence. */

    ZMAP_SERVERREQ_FEATURE_SEQUENCE,			    /* Get the features + sequence. */

    ZMAP_SERVERREQ_TERMINATE				    /* Close and destroy the connection. */

  } ZMapServerReqType ;




/* ALL request/response structs must include the fields from ZMapServerReqType
 * as their _FIRST_ field in the struct so that code can look in all such structs to decode them. */


/* The canonical request, use the "type" field to detect the contents of the struct. */
typedef struct
{
  ZMapServerReqType type ;
} ZMapServerReqAnyStruct, *ZMapServerReqAny ;


/* Open a connection to a server. */
typedef struct
{
  ZMapServerReqType type ;

  zMapURL url ;             /* replaces host, port, protocol and allows more info */
  char *format ;
  int timeout ;
  char *version ;
} ZMapServerReqOpenStruct, *ZMapServerReqOpen ;



/* Used to request server attributes. */
typedef struct
{
  ZMapServerReqType type ;

  char *database_path_out ;

} ZMapServerReqGetServerInfoStruct, *ZMapServerReqGetServerInfo ;



/* Used to specify styles (perhaps loaded from users file) or to retrieve styles from the server. */
typedef struct
{
  ZMapServerReqType type ;


  /* Some styles specify the mode/type of the features they represent (e.g. "transcript like",
   * "text" etc.), zmap requires that the style mode is set otherwise the features
   * referencing that style will _not_ be displayed. This flag records whether the styles
   * from the data source have a mode or whether it needs to be inferred from the features
   * themseleves. This is completely data source dependent. */
  gboolean server_styles_have_mode ;

  GData *styles ;					    /* List of prespecified styles or NULL
							       to get all available styles. */
} ZMapServerReqStylesStruct, *ZMapServerReqStyles ;


/* Used to ask for a specific sequence(s), currently this is targetted at blixem and so some stuff
 * is targetted for that usage, although knowing the selected feature is useful for a number of
 * operations. */
typedef struct
{
  ZMapServerReqType type ;

  ZMapFeature orig_feature ;				    /* The original feature which
							       triggered the request. */

  GList *sequences ;					    /* List of ZMapSequenceStruct which
							       hold name of sequence to be fetched. */

  int flags;
} ZMapServerReqGetSequenceStruct, *ZMapServerReqGetSequence ;


/* Used to specify which feature sets should be retrieved or to get the list of all feature sets
 * available. */
typedef struct
{
  ZMapServerReqType type ;

  GList *feature_sets ;					    /* List of prespecified features sets or
							       NULL to get all available sets. */
} ZMapServerReqFeatureSetsStruct, *ZMapServerReqFeatureSets ;



/* Set a context/region in a server. */
typedef struct
{
  ZMapServerReqType type ;

  ZMapFeatureContext context ;
} ZMapServerReqNewContextStruct, *ZMapServerReqNewContext ;


/* Get features from a server. */
typedef struct
{
  ZMapServerReqType type ;

  ZMapFeatureContext feature_context_out ;		    /* Returned feature sets. */
} ZMapServerReqGetFeaturesStruct, *ZMapServerReqGetFeatures ;


/* Open a connection, set up the context and get the features all in one go. */
typedef struct
{
  ZMapServerReqType type ;

  ZMapServerReqOpenStruct open ;

  ZMapServerReqOpenStruct server_info ;

  ZMapServerReqGetServerInfoStruct get_info ;

  ZMapServerReqStylesStruct styles ;

  ZMapServerReqFeatureSetsStruct feature_sets ;

  ZMapServerReqNewContextStruct context ;

  ZMapServerReqGetFeaturesStruct features ;

} ZMapServerReqOpenLoadStruct, *ZMapServerReqOpenLoad ;



/* Can be used to address any struct, do we need this ? not sure..... */
typedef union
{
  ZMapServerReqAny any ;
  ZMapServerReqOpen open ;
  ZMapServerReqOpenLoad open_load ;
  ZMapServerReqGetServerInfo get_info ;
  ZMapServerReqStyles styles ;
  ZMapServerReqFeatureSets feature_sets ;
  ZMapServerReqGetFeatures get_features ;
  ZMapServerReqNewContext new_context ;
} ZMapServerReq ;



ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
					      void *request_in, void **reply_out,
					      char **err_msg_out) ;

ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out) ;


#endif /* !ZMAP_PROTOCOL_H */
