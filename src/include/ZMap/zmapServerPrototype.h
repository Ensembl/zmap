/*  File: zmapServerPrototype.h
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk &
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Describes the interface between zmapServer, the generalised
 *              server interface and the underlying server specific
 *              implementation. Only specific server implementations should
 *              include this header, its not really for general consumption.
 *              
 * HISTORY:
 * Last edited: Jul 29 14:58 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServerPrototype.h,v 1.3 2004-08-02 14:08:30 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_PROTOTYPEP_H
#define ZMAP_SERVER_PROTOTYPEP_H

#include <glib.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapServer.h>				    /* is this ok to go here, think about it... */

/* Define function prototypes for generalised server calls, they all go in
 * the ZMapServerFuncsStruct struct that represents the calls for a server
 * of a particular protocol. */
typedef gboolean (*ZMapServerGlobalFunc) (void) ;
typedef gboolean (*ZMapServerCreateFunc) (void **server_conn,
					  char *host, int port,
					  char *userid, char *passwd, int timeout) ;
typedef gboolean (*ZMapServerOpenFunc)   (void *server_conn) ;
typedef ZMapServerResponseType
                 (*ZMapServerRequestFunc)(void *server_conn, ZMapServerRequestType request,
					  char *sequence, ZMapFeatureContext *feature_context) ;
typedef char *   (*ZMapServerGetErrorMsgFunc)(void *server_conn) ;
typedef gboolean (*ZMapServerCloseFunc)  (void *server_conn) ;
typedef gboolean (*ZMapServerDestroyFunc)(void *server_conn) ;


typedef struct _ZMapServerFuncsStruct
{
  ZMapServerGlobalFunc global_init ;
  ZMapServerCreateFunc create ;
  ZMapServerOpenFunc open ;
  ZMapServerRequestFunc request ;
  ZMapServerGetErrorMsgFunc errmsg ;
  ZMapServerCloseFunc close ;
  ZMapServerDestroyFunc destroy ;
} ZMapServerFuncsStruct, *ZMapServerFuncs ;



/* These are the hard coded function names for specific server implementations.
 * We could make this all more dynamic but this will do for now. */
void acedbGetServerFuncs(ZMapServerFuncs acedb_funcs) ;
void dasGetServerFuncs(ZMapServerFuncs das_funcs) ;

#endif /* !ZMAP_SERVER_PROTOTYPEP_H */
