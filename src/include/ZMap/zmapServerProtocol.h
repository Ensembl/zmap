/*  File: zmapServerProtocol.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2005
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
 * Last edited: May 16 11:32 2005 (edgrif)
 * Created: Wed Feb  2 11:47:16 2005 (edgrif)
 * CVS info:   $Id: zmapServerProtocol.h,v 1.4 2005-05-18 10:49:21 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PROTOCOL_H
#define ZMAP_PROTOCOL_H

#include <glib.h>
#include <ZMap/zmapUrl.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapThreads.h>

/* Requests can be of different types with different input parameters and returning
 * different types of results. */
typedef enum
  {
    ZMAP_SERVERREQ_INVALID = 0,

    ZMAP_SERVERREQ_OPEN,

    ZMAP_SERVERREQ_OPENLOAD,

    ZMAP_SERVERREQ_TYPES,				    /* Get the feature types. */

    ZMAP_SERVERREQ_FEATURES,				    /* Get the features. */

    ZMAP_SERVERREQ_SEQUENCE,				    /* Get the sequence. */

    ZMAP_SERVERREQ_FEATURE_SEQUENCE,			    /* Get the features + sequence. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* these are things I would like to do but have not implemented yet.... */

    ZMAP_SERVERREQ_NEWCONTEXT,			    /* Set a new sequence name/start/end. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    ZMAP_SERVERREQ_TERMINATE				    /* Close and destroy the connection. */

  } ZMapServerReqType ;




/* ALL request/response structs must include the fields from ZMapServerReqType
 * as their first field in the struct so that code can look in all such structs to decode them. */


/* The canonical request, use the "type" field to detect the contents of the struct. */
typedef struct
{
  ZMapServerReqType type ;
} ZMapServerReqAnyStruct, *ZMapServerReqAny ;


/* Open a connection to a server. */
typedef struct
{
  ZMapServerReqType type ;

  struct url *url ;             /* replaces host, port, protocol and allows more info */
  char *format ;
  int timeout ;
  char *version ;
} ZMapServerReqOpenStruct, *ZMapServerReqOpen ;


/* Find out what types are on a server. */
typedef struct
{
  ZMapServerReqType type ;

  GData *types_out ;					    /* Returned list of available feature types. */
} ZMapServerReqGetTypesStruct, *ZMapServerReqGetTypes ;


/* Set a context/region in a server. */
typedef struct
{
  ZMapServerReqType type ;

  char *sequence ;
  int start, end ;

  GData *types ;					    /* Set of all types that could be fetched. */

} ZMapServerReqNewContextStruct, *ZMapServerReqNewContext ;


/* Get features from a server. */
typedef struct
{
  ZMapServerReqType type ;

  GList *req_types ;					    /* types to retrieve for this request,
							       NULL means get all of them. */

  ZMapFeatureContext feature_context_out ;		    /* Returned feature sets. */
} ZMapServerReqGetFeaturesStruct, *ZMapServerReqGetFeatures ;


/* Open a connection, set up the context and get the features all in one go. */
typedef struct
{
  ZMapServerReqType type ;

  ZMapServerReqOpenStruct open ;

  ZMapServerReqGetTypesStruct types ;

  ZMapServerReqNewContextStruct context ;

  ZMapServerReqGetFeaturesStruct features ;

} ZMapServerReqOpenLoadStruct, *ZMapServerReqOpenLoad ;



/* Can be used to address any struct, do we need this ? not sure..... */
typedef union
{
  ZMapServerReqAny any ;
  ZMapServerReqOpen open ;
  ZMapServerReqOpenLoad open_load ;
  ZMapServerReqGetTypes get_types ;
  ZMapServerReqGetFeatures get_features ;
  ZMapServerReqNewContext new_context ;
} ZMapServerReq ;



ZMapThreadReturnCode zMapServerRequestHandler(void **slave_data,
					      void *request_in, void **reply_out,
					      char **err_msg_out) ;

ZMapThreadReturnCode zMapServerTerminateHandler(void **slave_data, char **err_msg_out) ;


#endif /* !ZMAP_PROTOCOL_H */
