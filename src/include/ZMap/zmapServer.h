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
 * Last edited: Mar 22 10:42 2004 (edgrif)
 * Created: Wed Aug  6 15:48:47 2003 (edgrif)
 * CVS info:   $Id: zmapServer.h,v 1.2 2004-03-22 13:15:28 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SERVER_H
#define ZMAP_SERVER_H

#include <glib.h>


/* Opaque type, represents a connection to a database server. */
typedef struct _ZMapServerStruct *ZMapServer ;


/* This routine must be called before any other server routines and must only be called once.
 * It is the callers responsibility to make sure this happens. */
gboolean zMapServerGlobalInit(char *protocol, void **server_global_data_out) ;

/* Provide matching Termination routine ???? */


gboolean zMapServerCreateConnection(ZMapServer *server_out, void *server_global_data,
				    char *host, int port, char *protocol,
				    char *userid, char *passwd) ;

gboolean zMapServerOpenConnection(ZMapServer server) ;

gboolean zMapServerRequest(ZMapServer server, char *request, char **reply, int *reply_len) ;

gboolean zMapServerCloseConnection(ZMapServer server) ;

gboolean zMapServerFreeConnection(ZMapServer server) ;

char *zMapServerLastErrorMsg(ZMapServer server) ;


#endif /* !ZMAP_SERVER_H */
