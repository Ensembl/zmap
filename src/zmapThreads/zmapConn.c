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
 * Last edited: Dec 14 09:36 2004 (edgrif)
 * Created: Thu Jul 24 14:37:18 2003 (edgrif)
 * CVS info:   $Id: zmapConn.c,v 1.16 2004-12-15 14:11:47 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <string.h>
#include <ZMap/zmapUtils.h>
#include <zmapConn_P.h>

/* Turn on/off all debugging messages for threads. */
gboolean zmap_thr_debug_G = FALSE ;


static ZMapConnection createConnection(char *machine, int port, char *protocol,
				       int timeout, char *version,
				       char *sequence, int start, int end, GData *types,
				       ZMapProtocolAny initial_request) ;
static void destroyConnection(ZMapConnection connection) ;


ZMapConnection zMapConnCreate(char *machine, int port, char *protocol, int timeout, char *version,
			      char *sequence, int start, int end, GData *types,
			      ZMapProtocolAny initial_request)
{
  ZMapConnection connection ;
  pthread_t thread_id ;
  pthread_attr_t thread_attr ;
  int status = 0 ;

  connection = createConnection(machine, port, protocol, timeout, version,
				sequence, start, end, types, initial_request) ;

  /* ok to just set state here because we have not started the thread yet.... */
  zmapCondVarCreate(&(connection->request)) ;
  connection->request.state = ZMAP_REQUEST_WAIT ;
  connection->request.data = NULL ;

  zmapVarCreate(&(connection->reply)) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  connection->reply.state = ZMAP_REPLY_INIT ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  connection->reply.state = ZMAP_REPLY_WAIT ;
  connection->reply.data = NULL ;
  connection->reply.error_msg = NULL ;

  /* Set the new threads attributes so it will run "detached", we do not want anything from them.
   * when they die, we want them to go away and release their resources. */
  if (status == 0
      && (status = pthread_attr_init(&thread_attr)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Create thread attibutes") ;
    }

  if (status == 0
      && (status = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Set thread detached attibute") ;
    }


  /* Create the new thread. */
  if (status == 0
      && (status = pthread_create(&thread_id, &thread_attr, zmapNewThread, (void *)connection)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Thread creation") ;
    }

  if (status == 0)
    connection->thread_id = thread_id ;
  else
    {
      /* Ok to just destroy connection here as the thread was not successfully created so
       * there can be no complications with interactions with condvars in connect struct. */
      destroyConnection(connection) ;
      connection = NULL ;
    }

  return connection ;
}



void zMapConnRequest(ZMapConnection connection, void *request)
{
  zmapCondVarSignal(&connection->request, ZMAP_REQUEST_GETDATA, request) ;

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


/* Must be kept in step with declaration of ZMapThreadRequest enums in zmapConn_P.h */
char *zMapVarGetRequestString(ZMapThreadRequest signalled_state)
{
  char *str_states[] = {"ZMAP_REQUEST_INIT", "ZMAP_REQUEST_WAIT", "ZMAP_REQUEST_TIMED_OUT",
			"ZMAP_REQUEST_GETDATA"} ;

  return str_states[signalled_state] ;
}

/* Must be kept in step with declaration of ZMapThreadReply enums in zmapConn_P.h */
char *zMapVarGetReplyString(ZMapThreadReply signalled_state)
{
  char *str_states[] = {"ZMAP_REPLY_INIT", "ZMAP_REPLY_WAIT",
			"ZMAP_REPLY_GOTDATA",  "ZMAP_REPLY_REQERROR",
			"ZMAP_REPLY_DIED", "ZMAP_REPLY_CANCELLED"} ;

  return str_states[signalled_state] ;
}




/* Kill the thread by cancelling it, as this will asynchronously we cannot release the connections
 * resources in this call. */
void zMapConnKill(ZMapConnection connection)
{
  int status ;

  ZMAP_THR_DEBUG(("GUI: killing and destroying connection for thread %x\n", connection->thread_id)) ;

  /* we could signal an exit here by setting a condvar of EXIT...but that might lead to 
   * deadlocks, think about this bit.. */

  /* Signal the thread to cancel it */
  if ((status = pthread_cancel(connection->thread_id)) != 0)
    {
      zMapLogFatalSysErr(status, "%s", "Thread cancel") ;
    }

  return ;
}


/* Release the connections resources, don't do this until the slave thread has gone. */
void zMapConnDestroy(ZMapConnection connection)
{
  ZMAP_THR_DEBUG(("GUI: destroying connection for thread %x\n", connection->thread_id)) ;

  zmapVarDestroy(&connection->reply) ;
  zmapCondVarDestroy(&(connection->request)) ;

  g_free(connection->machine) ;

  g_free(connection) ;

  return ;
}




/* 
 * ---------------------  Internal routines  ------------------------------
 */



static ZMapConnection createConnection(char *machine, int port, char *protocol,
				       int timeout, char *version,
				       char *sequence, int start, int end,  GData *types,
				       ZMapProtocolAny initial_request)
{
  ZMapConnection connection ;

  connection = g_new0(ZMapConnectionStruct, 1) ;

  connection->machine = g_strdup(machine) ;
  connection->port = port ;
  connection->protocol = g_strdup(protocol) ;
  connection->timeout = timeout ;
  if (version)
    connection->version = g_strdup(version) ;

  connection->sequence =  g_strdup(sequence) ;
  connection->start = start ;
  connection->end = end ;
  connection->types = types ;

  connection->initial_request = initial_request ;

  return connection ;
}


/* some care needed in using this....what about the condvars, when can they be freed ?
 * CHECK THIS ALL WORKS.... */
static void destroyConnection(ZMapConnection connection)
{
  g_free(connection->machine) ;
  g_free(connection->protocol) ;

  g_free(connection->sequence) ;

  if (connection->version)
    g_free(connection->version) ;

  /* Setting this to zero prevents subtle bugs where calling code continues
   * to try to reuse a defunct control block. */

  /* Should crash if this returns NULL, need my macros from acedb code.... */
  memset((void *)connection, 0, sizeof(ZMapConnectionStruct)) ;

  g_free(connection) ;

  return ;
}
