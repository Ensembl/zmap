/*  File: zmapServer_P.h
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Nov 12 10:00 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServer_P.h,v 1.6 2004-11-12 11:56:00 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_P_H
#define ZMAP_SERVER_P_H

#include <ZMap/zmapServer.h>
#include <zmapServerPrototype.h>




/* A connection to a database. */
typedef struct _ZMapServerStruct
{
  /* We keep a copy of these as they are the minimum required for _any_ type of server. */
  char *host ;
  char *protocol ;

  ZMapServerFuncs funcs ;

  void *server_conn ;					    /* opaque type used for server calls. */

  ZMapServerResponseType last_response ;		    /* For errors returned by connection. */
  char *last_error_msg ;


} ZMapServerStruct ;


/* A context for sequence operations. */
typedef struct _ZMapServerContextStruct
{
  ZMapServer server ;

  void *server_conn_context ;				    /* opaque type used for server calls. */

} ZMapServerContextStruct ;



/* Hard coded for now...sigh....would like to be more dynamic really.... */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs) ;
void dasGetServerFuncs(ZMapServerFuncs das_funcs) ;
void fileGetServerFuncs(ZMapServerFuncs das_funcs) ;

#endif /* !ZMAP_SERVER_P_H */
