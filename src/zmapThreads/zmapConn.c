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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jan 22 13:52 2004 (edgrif)
 * Created: Thu Jul 24 14:37:18 2003 (edgrif)
 * CVS info:   $Id: zmapConn.c,v 1.2 2004-01-23 13:27:53 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <zmapConn_P.h>

/* Turn on/off all debugging messages for threads. */
gboolean zmap_thr_debug_G = TRUE ;





ZMapConnection zMapConnCreate(char *machine, char *port, char *protocol, char *sequence)
{
  ZMapConnection connection ;
  pthread_t thread_id ;
  pthread_attr_t thread_attr ;
  int status ;

  connection = g_new(ZMapConnectionStruct, sizeof(ZMapConnectionStruct)) ;

  connection->machine = g_strdup(machine) ;
  connection->port = atoi(port) ;
  connection->protocol = g_strdup(protocol) ;
  connection->sequence = g_strdup(sequence) ;

  /* ok to just set state here because we have not started the thread yet.... */
  zmapCondVarCreate(&(connection->request)) ;
  connection->request.state = ZMAP_REQUEST_WAIT ;

  zmapVarCreate(&(connection->reply)) ;
  connection->reply.state = ZMAP_REPLY_WAIT ;


  /* Set the new threads attributes so it will run "detached", we do not want anything from them.
   * when they die, we want them to go away and release their resources. */
  if ((status = pthread_attr_init(&thread_attr)) != 0)
    {
      ZMAPSYSERR(status, "Create thread attibutes") ;
    }

  if ((status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0)
    {
      ZMAPSYSERR(status, "Set thread detached attibute") ;
    }


  /* Create the new thread. */
  if ((status = pthread_create(&thread_id, &thread_attr, zmapNewThread, (void *)connection)) != 0)
    {
      ZMAPSYSERR(status, "Thread creation") ;
    }


  connection->thread_id = thread_id ;


  return connection ;
}


void zMapConnLoadData(ZMapConnection connection)
{
  zmapCondVarSignal(&connection->request, ZMAP_REQUEST_GETDATA) ;

  return ;
}


gboolean zMapConnGetReply(ZMapConnection connection, ZMapThreadReply *state)
{
  gboolean got_value ;
  
  got_value = zmapVarGetValue(&(connection->reply), state) ;
  
  return got_value ;
}



void zMapConnSetReply(ZMapConnection connection, ZMapThreadReply state)
{
  zmapVarSetValue(&(connection->reply), state) ;

  return ;
}


gboolean zMapConnGetReplyWithData(ZMapConnection connection, ZMapThreadReply *state,
				  void **data, char **err_msg)
{
  gboolean got_value ;
  
  got_value = zmapVarGetValueWithData(&(connection->reply), state, data, err_msg) ;
  
  return got_value ;
}





void *zMapConnGetThreadid(ZMapConnection connection)
{
  return (void *)(connection->thread_id) ;
}

/* Kill the thread by cancelling it, as this will asynchronously we cannot release the connections
 * resources in this call. */
void zMapConnKill(ZMapConnection connection)
{
  int status ;

  ZMAP_THR_DEBUG(("GUI: killing and destroying connection for thread %x\n", connection->thread_id)) ;

  /* Really we should signal an exit here...but that lead to deadlocks, think about this bit.. */

  /* Signal the thread to cancel it, we could set a cond var of EXIT but this will take ages
   * for the thread to be cancelled. */
  if ((status = pthread_cancel(connection->thread_id)) != 0)
    {
      ZMAPSYSERR(status, "Thread cancel") ;
    }

  return ;
}


/* Release the connections resources, don't do this until the slave thread has gone. */
void zMapConnDestroy(ZMapConnection connection)
{
  int status ;

  ZMAP_THR_DEBUG(("GUI: destroying connection for thread %x\n", connection->thread_id)) ;

  zmapVarDestroy(&connection->reply) ;
  zmapCondVarDestroy(&(connection->request)) ;

  g_free(connection->machine) ;
  g_free(connection->sequence) ;

  g_free(connection) ;

  return ;
}




/* 
 * ---------------------  Internal routines  ------------------------------
 */

