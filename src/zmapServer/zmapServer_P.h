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
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Sep 13 09:28 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServer_P.h,v 1.4 2004-09-13 12:59:02 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_P_H
#define ZMAP_SERVER_P_H

#include <ZMap/zmapServerPrototype.h>
#include <ZMap/zmapServer.h>



/* A connection to a database. */
typedef struct _ZMapServerStruct
{
  char *host ;
  int port ;
  char *protocol ;

  ZMapServerFuncs funcs ;

  void *server_conn ;					    /* opaque type used for server calls. */

  ZMapServerResponseType last_response ;		    /* For errors returned by connection. */
  char *last_error_msg ;


} ZMapServerStruct ;



/* Hard coded for now...sigh....would like to be more dynamic really.... */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs) ;
void dasGetServerFuncs(ZMapServerFuncs das_funcs) ;
void fileGetServerFuncs(ZMapServerFuncs das_funcs) ;

#endif /* !ZMAP_SERVER_P_H */
