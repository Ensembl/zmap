/*  File: zmapConn_P.h
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
 * Last edited: Nov 11 14:38 2004 (edgrif)
 * Created: Thu Jul 24 14:36:08 2003 (edgrif)
 * CVS info:   $Id: zmapConn_P.h,v 1.9 2004-11-12 11:58:42 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONN_PRIV_H
#define ZMAP_CONN_PRIV_H


#include <pthread.h>
#include <glib.h>
#include <ZMap/zmapSys.h>
#include <ZMap/zmapConn.h>


/* Seems that Linux defines the time structure used in some pthread calls as "timespec",
 * whereas alphas call it "timespec_t"....deep sigh.... */
#ifdef LINUX
#define TIMESPEC struct timespec
#else
#define TIMESPEC timespec_t
#endif

#define ZMAP_THR_DEBUG(ALL_ARGS_WITH_BRACKETS) \
do { \
     if (zmap_thr_debug_G) \
       printf ALL_ARGS_WITH_BRACKETS ; \
   } while (0)


/* Requests are via a cond var system. */
typedef struct
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  pthread_cond_t cond ;					    /* Slave waits on this. */
  ZMapThreadRequest state ;				    /* Contains request to slave. */
  void *data ;						    /* Contains data for request. */
} ZMapRequestStruct, *ZMapRequest ;

/* Replies via a simpler mutex. */
typedef struct
{
  pthread_mutex_t mutex ;				    /* controls access to this struct. */
  ZMapThreadReply state ;				    /* Contains reply from slave. */
  void *data ;						    /* May also contain data for some replies. */
  gchar *error_msg ;					    /* May contain error message for when
							       thread fails. */
} ZMapReplyStruct, *ZMapReply ;



/* The ZMapConnection, there is one per connection to a database, i.e. one per slave
 * thread. */
typedef struct _ZMapConnectionStruct
{
  /* These are read only after creation of the thread. */
  gchar *machine ;
  int port ;
  gchar *protocol ;
  int timeout ;
  gchar *version ;

  gchar *sequence ;
  int start, end ;
  GData *types ;

  gboolean load_features ;

  pthread_t thread_id ;


  /* These control the communication between the GUI thread and the connection threads,
   * they are mutex locked and the request code makes use of the condition variable to
   * communicate with slave threads. */
  ZMapRequestStruct request ;				    /* GUIs request. */
  ZMapReplyStruct reply ;				    /* Slaves reply. */

} ZMapConnectionStruct ;


/* Thread calls. */
void *zmapNewThread(void *thread_args) ;


/* Request routines. */
void zmapCondVarCreate(ZMapRequest thread_state) ;
void zmapCondVarSignal(ZMapRequest thread_state, ZMapThreadRequest new_state, void *request) ;
void zmapCondVarWait(ZMapRequest thread_state, ZMapThreadRequest waiting_state) ;
ZMapThreadRequest zmapCondVarWaitTimed(ZMapRequest condvar, ZMapThreadRequest waiting_state,
				       TIMESPEC *timeout, gboolean reset_to_waiting, void **data) ;
void zmapCondVarDestroy(ZMapRequest thread_state) ;


/* Reply routines. */
void zmapVarCreate(ZMapReply thread_state) ;
void zmapVarSetValue(ZMapReply thread_state, ZMapThreadReply new_state) ;
gboolean zmapVarGetValue(ZMapReply thread_state, ZMapThreadReply *state_out) ;
void zmapVarSetValueWithData(ZMapReply thread_state, ZMapThreadReply new_state, void *data) ;
void zmapVarSetValueWithError(ZMapReply thread_state, ZMapThreadReply new_state, char *err_msg) ;
gboolean zmapVarGetValueWithData(ZMapReply thread_state, ZMapThreadReply *state_out,
				 void **data_out, char **err_msg_out) ;
void zmapVarDestroy(ZMapReply thread_state) ;


#endif /* !ZMAP_CONN_PRIV_H */
