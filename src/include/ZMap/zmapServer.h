/*  File: zmapServer.h
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep 16 13:50 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServer.h,v 1.5 2004-09-17 08:39:26 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_H
#define ZMAP_SERVER_H

#include <glib.h>
#include <ZMap/zmapFeature.h>


/* Opaque type, represents a connection to a database server. */
typedef struct _ZMapServerStruct *ZMapServer ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* I think I don't need this now, too complex......... */

/* Opaque type, represents a sequence context which can be used to get data/features/dna etc.
 * The server is implicit in the context so once you have one of these you do not need to
 * specify the server. */
typedef struct _ZMapServerContextStruct *ZMapServerContext ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* ALL THIS IN zmapProtocol.h now........... */


/* Server requests can be of different types with different input parameters and returning
 * different types of results. */

typedef enum {ZMAP_SERVERREQ_INVALID = 0,
	      ZMAP_SERVERREQ_SETCONTEXT,		    /* Set the sequence name/start/end. */
	      ZMAP_SERVERREQ_SEQUENCE			    /* Get the features. */
} ZMapServerRequestType ;


/* Structs for request parameters and return values. */

typedef struct
{
  char *sequence ;
  int start, end ;
} ZMapServerRequestContextStruct, *ZMapServerRequestContext ;


typedef struct
{
  ZMapFeatureContext feature_context_out ;
} ZMapServerRequestFeaturesStruct, *ZMapServertFeaturesContext ;


typedef union
{
  ZMapServerRequestContext set_context ;
  ZMapServertFeaturesContext get_features ;
} ZMapServerRequest ;


#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Possible responses to a server request. */
typedef enum {ZMAP_SERVERRESPONSE_OK,
	      ZMAP_SERVERRESPONSE_BADREQ, ZMAP_SERVERRESPONSE_REQFAIL,
	      ZMAP_SERVERRESPONSE_TIMEDOUT, ZMAP_SERVERRESPONSE_SERVERDIED} ZMapServerResponseType ;



/* This routine must be called before any other server routines and must only be called once.
 * It is the callers responsibility to make sure this happens. */
gboolean zMapServerGlobalInit(char *protocol, void **server_global_data_out) ;


/* Provide matching Termination routine ???? */


gboolean zMapServerCreateConnection(ZMapServer *server_out, void *server_global_data,
				    char *host, int port, char *protocol,
				    char *userid, char *passwd) ;


ZMapServerResponseType zMapServerOpenConnection(ZMapServer server) ;


ZMapServerResponseType zMapServerSetContext(ZMapServer server, char *sequence, int start, int end) ;


/* The caller doesn't need to see the request...this seems not completely right, revisit later... */
ZMapServerResponseType zMapServerRequest(ZMapServer server, void *request) ;


char *zMapServerLastErrorMsg(ZMapServer server) ;


ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) ;


gboolean zMapServerFreeConnection(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
