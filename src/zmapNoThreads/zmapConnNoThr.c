/*  File: zmapthrcontrol.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapConn.h
 * HISTORY:
 * Last edited: Oct 13 14:49 2004 (edgrif)
 * Created: Thu Jul 24 14:37:18 2003 (edgrif)
 * CVS info:   $Id: zmapConnNoThr.c,v 1.9 2004-10-14 10:18:50 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmapSys.h>
#include <zmapConnNoThr_P.h>

/* temp. */
extern char *testGetData(void) ;

/* Turn on/off all debugging messages for threads. */
gboolean zmap_thr_debug_G = TRUE ;


/* NEEDED */
ZMapConnection zMapConnCreate(char *machine, int port, char *protocol, int timeout,
			      char *sequence, int start, int end, GData *types,
			      gboolean load_features)
{
  ZMapConnection connection ;
  int status ;

  connection = g_new0(ZMapConnectionStruct, 1) ;

  connection->machine = g_strdup(machine) ;
  connection->port = port ;
  connection->sequence = g_strdup(sequence) ;
  connection->thread_id = (void *)connection ;
  connection->request.state = ZMAP_REQUEST_WAIT ;
  connection->reply.state = ZMAP_REPLY_WAIT ;

  return connection ;
}



/* NEEDED */
void zMapConnLoadData(ZMapConnection connection, gchar *data)
{
  connection->request.state = ZMAP_REQUEST_GETDATA ;

  /* call acedb routine to get the data. */
  connection->reply.data = testGetData() ;

  connection->reply.state = ZMAP_REPLY_GOTDATA ;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
gboolean zMapConnGetReply(ZMapConnection connection, ZMapThreadReply *state)
{
  gboolean got_value = TRUE ;
  
  *state = connection->reply.state ;
  
  return got_value ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* NEEDED */
void zMapConnSetReply(ZMapConnection connection, ZMapThreadReply state)
{
  connection->reply.state = state ;

  return ;
}

/* NEEDED */
gboolean zMapConnGetReplyWithData(ZMapConnection connection, ZMapThreadReply *state,
				  void **data, char **err_msg)
{
  gboolean got_value = TRUE ;
  
  *state = connection->reply.state ;
  *data = connection->reply.data ;
  *err_msg = connection->reply.error_msg ;
  
  return got_value ;
}

/* NEEDED */
/* We need to hack this to have the process number or something like that....actually the
 * address of the connection would do..... */
void *zMapConnGetThreadid(ZMapConnection connection)
{
  return connection->thread_id ;
}


/* NEEDED */
/* Kill the thread by cancelling it, as this will asynchronously we cannot release the connections
 * resources in this call. */
void zMapConnKill(ZMapConnection connection)
{
  int status ;

  ZMAP_THR_DEBUG(("GUI: killing and destroying connection for thread %x\n", connection->thread_id)) ;

  /* code here did a pthread_cancel only, so nothing to be done here.... */

  return ;
}


/* NEEDED */
/* Release the connections resources, don't do this until the slave thread has gone. */
void zMapConnDestroy(ZMapConnection connection)
{
  g_free(connection->machine) ;
  g_free(connection->sequence) ;

  g_free(connection) ;

  return ;
}




/* 
 * ---------------------  Internal routines  ------------------------------
 */

