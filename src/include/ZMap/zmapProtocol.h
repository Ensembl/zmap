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
 * Last edited: Sep 17 14:25 2004 (edgrif)
 * Created: Wed Sep 15 11:46:18 2004 (edgrif)
 * CVS info:   $Id: zmapProtocol.h,v 1.2 2004-09-23 13:38:35 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_PROTOCOL_H
#define ZMAP_PROTOCOL_H

#include <glib.h>
#include <ZMap/zmapFeature.h>



/* Requests can be of different types with different input parameters and returning
 * different types of results. */

typedef enum
  {
    ZMAP_PROTOCOLREQUEST_INVALID = 0,

    ZMAP_PROTOCOLREQUEST_SEQUENCE				    /* Get the features. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* these are things I would like to do but have not implemented yet.... */

    ZMAP_PROTOCOLREQUEST_NEWCONTEXT,			    /* Set a new sequence name/start/end. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  } ZMapProtocolRequestType ;






/* ALL request/response structs must include the below fields as their first fields in the struct
 * so that code can look in all such structs to decode them. */
typedef struct
{
  ZMapProtocolRequestType request ;
} ZMapProtocolAnyStruct, *ZMapProtocolAny ;



typedef struct
{
  ZMapProtocolRequestType request ;

  char *sequence ;
  int start, end ;
} ZMapProtocolNewContextStruct, *ZMapProtocolNewContext ;


typedef struct
{
  ZMapProtocolRequestType request ;

  ZMapFeatureContext feature_context_out ;
} ZMapProtocolGetFeaturesStruct, *ZMapProtocoltGetFeatures ;


typedef union
{
  ZMapProtocolAny any ;
  ZMapProtocolNewContext new_context ;
  ZMapProtocoltGetFeatures get_features ;
} ZMapProtocol ;



#endif /* !ZMAP_PROTOCOL_H */
