/*  File: zmapConn.h
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jul 29 09:27 2004 (edgrif)
 * Created: Thu Jul 24 14:35:58 2003 (edgrif)
 * CVS info:   $Id: zmapConn.h,v 1.4 2004-07-29 08:45:32 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONN_H
#define ZMAP_CONN_H

/* debugging messages, TRUE for on... */
extern gboolean zmap_thr_debug_G ;


/* If you change these two enumerated types you must update zMapVarGetRequestString() or
 * zMapVarGetReplyString() accordingly. */

/* Requests to a slave thread. */
typedef enum {ZMAP_REQUEST_INIT, ZMAP_REQUEST_WAIT, ZMAP_REQUEST_TIMED_OUT,
	      ZMAP_REQUEST_GETDATA} ZMapThreadRequest ;

/* Replies from a slave thread. */
typedef enum {ZMAP_REPLY_INIT, ZMAP_REPLY_WAIT, ZMAP_REPLY_GOTDATA, ZMAP_REPLY_REQERROR,
	      ZMAP_REPLY_DIED, ZMAP_REPLY_CANCELLED} ZMapThreadReply ;



/* One per connection to a database, an opaque private type. */
typedef struct _ZMapConnectionStruct *ZMapConnection ;


ZMapConnection zMapConnCreate(char *machine, int port, char *protocol, char *sequence) ;

void zMapConnLoadData(ZMapConnection connection, gchar *data) ;
gboolean zMapConnGetReply(ZMapConnection connection, ZMapThreadReply *state) ;
void zMapConnSetReply(ZMapConnection connection, ZMapThreadReply state) ;
gboolean zMapConnGetReplyWithData(ZMapConnection connection, ZMapThreadReply *state,
				  void **data, char **err_msg) ;
void *zMapConnGetThreadid(ZMapConnection connection) ;
char *zMapVarGetRequestString(ZMapThreadRequest signalled_state) ;
char *zMapVarGetReplyString(ZMapThreadReply signalled_state) ;

void zMapConnKill(ZMapConnection connection) ;
void zMapConnDestroy(ZMapConnection connection) ;

#endif /* !ZMAP_CONN_H */
