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
 * Description: Generalised server interface, hides acedb/das/file
 *              details from caller.
 *
 * HISTORY:
 * Last edited: Oct 13 14:27 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServer.h,v 1.9 2004-10-14 10:18:49 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_H
#define ZMAP_SERVER_H

#include <glib.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapProtocol.h>



/* The server interface has basic calls to create/open close/destroy a connection but all other
 * requests are via a generalised request call which accepts args in the protocol defined in
 * ZMap/zmapProtocol.h. This means that the client can just include ZMap/zmapProtocol.h and pass
 * args in that format.  */


/* Opaque type, represents a connection to a database server. */
typedef struct _ZMapServerStruct *ZMapServer ;


/* Possible responses to a server request. */
typedef enum {ZMAP_SERVERRESPONSE_OK,
	      ZMAP_SERVERRESPONSE_BADREQ, ZMAP_SERVERRESPONSE_REQFAIL,
	      ZMAP_SERVERRESPONSE_TIMEDOUT, ZMAP_SERVERRESPONSE_SERVERDIED} ZMapServerResponseType ;


/* Server context, includes the sequence to be fetched, start/end coords and other stuff needed
 * to fetch server data. */
typedef struct
{
  char *sequence ;
  int start ;
  int end ;
  GData *types ;
} ZMapServerSetContextStruct, *ZMapServerSetContext ;




/* This routine must be called before any other server routines and must only be called once.
 * It is the callers responsibility to make sure this happens.
 * Provide matching Termination routine ???? */
gboolean zMapServerGlobalInit(char *protocol, void **server_global_data_out) ;

gboolean zMapServerCreateConnection(ZMapServer *server_out, void *server_global_data,
				    char *host, int port, char *protocol, int timeout,
				    char *userid, char *passwd) ;

ZMapServerResponseType zMapServerOpenConnection(ZMapServer server) ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
ZMapServerResponseType zMapServerSetContext(ZMapServer server, char *sequence, int start, int end) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

ZMapServerResponseType zMapServerSetContext(ZMapServer server, ZMapServerSetContext context) ;



ZMapServerResponseType zMapServerRequest(ZMapServer server, ZMapProtocolAny request) ;

char *zMapServerLastErrorMsg(ZMapServer server) ;

ZMapServerResponseType zMapServerCloseConnection(ZMapServer server) ;

gboolean zMapServerFreeConnection(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
