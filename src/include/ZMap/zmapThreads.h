/*  File: zmapThreads.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: Interface to sub threads of the ZMap GUI thread.
 *
 * HISTORY:
 * Last edited: Mar 20 12:02 2009 (edgrif)
 * Created: Thu Jan 27 11:16:13 2005 (edgrif)
 * CVS info:   $Id: zmapThreads.h,v 1.12 2010-12-10 14:35:49 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_THREAD_H
#define ZMAP_THREAD_H
#include <config.h>
#if defined DARWIN
#include <pthread.h>
#endif

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* WARNING, IN THEORY WE SHOULD INCLUDE THIS AS WE REFERENCE A PTHREAD TYPE BELOW,
 * _BUT_ IF WE DO THEN WE RUN INTO TROUBLE ON THE ALPHAS AS FOR UNKNOWN AND PROBABLY
 * BIZARRE REASONS IT FAILS TO COMPILE COMPLAINING ABOUT "leave" IN THE Window/View
 * HEADERS TO BE ILLEGAL...THEY INCLUDE THIS HEADER...SIGH... WE COULD JUST HACK
 * OUR OWN VERSION OF THE pthread_t type to get round this.... */

#include <pthread.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* The calls need changing to handle a more general
 * mechanism of requests etc.....this interface should not need to know the requests it is
 * servicing.....all the server stuff needs to be removed from the conncreate and these
 * routines should be renamed to  zMapThreadNNNN */


/* We should have a function to access this global.... */
/* debugging messages, TRUE for on... */
extern gboolean zmap_thread_debug_G ;

/* If you change these two enumerated types you must update zMapVarGetRequestString() or
 * zMapVarGetReplyString() accordingly. */

/* Requests to a slave thread. */
typedef enum {ZMAPTHREAD_REQUEST_INIT, ZMAPTHREAD_REQUEST_WAIT, ZMAPTHREAD_REQUEST_TIMED_OUT,
	      ZMAPTHREAD_REQUEST_EXECUTE} ZMapThreadRequest ;


/* Replies from a slave thread. */
/* quit is as requested failed is quit unexpectedly */
typedef enum {ZMAPTHREAD_REPLY_INIT, ZMAPTHREAD_REPLY_WAIT, ZMAPTHREAD_REPLY_GOTDATA, ZMAPTHREAD_REPLY_REQERROR,
	      ZMAPTHREAD_REPLY_DIED, ZMAPTHREAD_REPLY_CANCELLED, ZMAPTHREAD_REPLY_QUIT } ZMapThreadReply ;


/* Return codes from a handler function. */
typedef enum {ZMAPTHREAD_RETURNCODE_OK, ZMAPTHREAD_RETURNCODE_TIMEDOUT,
	      ZMAPTHREAD_RETURNCODE_REQFAIL, ZMAPTHREAD_RETURNCODE_BADREQ,
	      ZMAPTHREAD_RETURNCODE_SERVERDIED, ZMAPTHREAD_RETURNCODE_QUIT } ZMapThreadReturnCode ;


/* One per connection to a thread, an opaque private type. */
typedef struct _ZMapThreadStruct *ZMapThread ;


/* Function to handle requests received by the thread.
 * slave_data will be passed into the handler function each time it is called to provide a
 *            mechanism for the slave to get at its own state data.
 * request is the request from the controlling thread
 * replay  is the reply from the slave code. */
typedef ZMapThreadReturnCode (*ZMapThreadRequestHandlerFunc)(void **slave_data,
							     void *request, void **reply,
							     char **err_msg_out) ;

/* Function that the thread code will call when the thread gets terminated abnormally by an
 * error or thread cancel, this gives the slave code the chance to close down properly. */
typedef ZMapThreadReturnCode (*ZMapThreadTerminateHandler)(void **slave_data, char **err_msg_out) ;

/* Function that the thread code will call when the thread gets terminated abnormally by an
 * error or thread cancel, this gives the slave code the chance to clean up, the slave code
 * should set slave_data to null on returning. */
typedef ZMapThreadReturnCode (*ZMapThreadDestroyHandler)(void **slave_data) ;


void zMapThreadForkLock(void);
void zMapThreadForkUnlock(void);


ZMapThread zMapThreadCreate(ZMapThreadRequestHandlerFunc handler_func,
			    ZMapThreadTerminateHandler terminate_func, ZMapThreadDestroyHandler destroy_func) ;
void zMapThreadRequest(ZMapThread thread, void *request) ;
gboolean zMapThreadGetReply(ZMapThread thread, ZMapThreadReply *state) ;
void zMapThreadSetReply(ZMapThread thread, ZMapThreadReply state) ;
gboolean zMapThreadGetReplyWithData(ZMapThread thread, ZMapThreadReply *state,
				    void **data, char **err_msg) ;
char *zMapThreadGetThreadID(ZMapThread thread) ;
char *zMapThreadGetRequestString(ZMapThreadRequest signalled_state) ;
char *zMapThreadGetReplyString(ZMapThreadReply signalled_state) ;
void zMapThreadKill(ZMapThread thread) ;
gboolean zMapThreadExists(ZMapThread thread);
void zMapThreadDestroy(ZMapThread thread) ;

#endif /* !ZMAP_THREAD_H */
