/*  File: zmapProtocol.h
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
 * Description: 
 * HISTORY:
 * Last edited: Sep 16 10:32 2004 (edgrif)
 * Created: Wed Sep 15 11:46:18 2004 (edgrif)
 * CVS info:   $Id: zmapProtocol.h,v 1.1 2004-09-17 08:47:20 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PROTOCOL_H
#define ZMAP_PROTOCOL_H

#include <glib.h>
#include <ZMap/zmapFeature.h>


/* Server requests can be of different types with different input parameters and returning
 * different types of results. */

typedef enum {
  ZMAP_PROTOCOLREQUEST_INVALID = 0,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Not sure I want to allow this at the moment.... */
  ZMAP_PROTOCOLREQUEST_SETCONTEXT,			    /* Set the sequence name/start/end. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  ZMAP_PROTOCOLREQUEST_SEQUENCE				    /* Get the features. */
} ZMapProtocolRequestType ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Not sure if we need this as we have a reply code from the server anyway.... */

/* Possible responses to a protocol request. */
typedef enum {
  ZMAP_PROTOCOLRESPONSE_OK,
  ZMAP_PROTOCOLRESPONSE_BADREQ,
  ZMAP_PROTOCOLRESPONSE_REQFAIL,
  ZMAP_PROTOCOLRESPONSE_TIMEDOUT,
  ZMAP_PROTOCOLRESPONSE_SERVERDIED
} ZMapProtocolResponseType ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* ALL request/response structs must include the below fields as their first fields in the struct
 * so that code can look in all such structs to decode them. */
typedef struct
{
  ZMapProtocolRequestType request ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapProtocolResponseType response ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

} ZMapProtocolAnyStruct, *ZMapProtocolAny ;



typedef struct
{
  ZMapProtocolRequestType request ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapProtocolResponseType response ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  char *sequence ;
  int start, end ;
} ZMapProtocolSetContextStruct, *ZMapProtocolSetContext ;


typedef struct
{
  ZMapProtocolRequestType request ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  ZMapProtocolResponseType response ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  ZMapFeatureContext feature_context_out ;
} ZMapProtocolGetFeaturesStruct, *ZMapProtocoltGetFeatures ;


typedef union
{
  ZMapProtocolAny any ;
  ZMapProtocolSetContext set_context ;
  ZMapProtocoltGetFeatures get_features ;
} ZMapProtocol ;








#endif /* !ZMAP_PROTOCOL_H */
