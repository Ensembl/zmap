/*  File: AceConn.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2002
 *-------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
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
 * Description: Interface for communicating with the AceDB server.
 *              Currently the server is sockets based but this is not
 *              exposed in this interface.
 *              
 *              You should see AceConn.html for full details
 *              of the interface.
 *              
 * HISTORY:
 * Last edited: Oct  3 15:31 2007 (edgrif)
 * Created: Wed Mar  6 13:58:41 2002 (edgrif)
 * CVS info:   $Id: AceConn.h,v 1.2 2007-10-03 14:39:02 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_ACECONN_H
#define DEF_ACECONN_H

#include <glib.h>

/* Library version stuff.                                                    */
#define     ACECONN_VERSION 1
#define     ACECONN_RELEASE 2
#define     ACECONN_UPDATE  0

#define ACECONN_CHECK_VERSION(version, release, update)    \
    (ACECONN_VERSION > (version) || \
     (ACECONN_VERSION == (version) && ACECONN_RELEASE > (release)) || \
     (ACECONN_VERSION == (version) && ACECONN_RELEASE == (release) && \
      ACECONN_UPDATE >= (update)))


/* Opaque handle to a connection to an Acedb socket server.                  */
typedef struct _AceConnRec *AceConnection ;

/* Return codes for the AceConn calls.                                       */
typedef enum {ACECONN_OK = 0,
	      ACECONN_QUIT,				    /* Command to server was "quit" so
							       connection is now closed. */
	      ACECONN_INVALIDCONN,			    /* Connection points to invalid memory. */
	      ACECONN_BADARGS,				    /* caller has supplied bad args. */
	      ACECONN_NOTOPEN,				    /* Connection not open. */
	      ACECONN_NOALLOC,				    /* could not allocate  */
	      ACECONN_NOSOCK,				    /* socket creation problem. */
	      ACECONN_UNKNOWNHOST,			    /* server host not known. */
	      ACECONN_NOCONNECT,			    /* Could not connect to host. */
	      ACECONN_NOSELECT,				    /* select on socket failed. */
	      ACECONN_HANDSHAKE,			    /* Handshake to server failed. */
	      ACECONN_READERROR,			    /* Error in reading from socket. */
	      ACECONN_WRITEERROR,			    /* Error in writing to socket. */
	      ACECONN_SIGSET,				    /* Problem with signal setting. */
	      ACECONN_NONBLOCK,				    /* Non-blocking for socket failed. */
	      ACECONN_TIMEDOUT,				    /* Connection timed out. */
	      ACECONN_NOCREATE,				    /* Could not create connection
							       control block. */
	      ACECONN_INTERNAL				    /* Dire, internal package error. */
} AceConnStatus ;


/* Default time (seconds) to wait for reply from server. */
enum {ACECONN_DEFAULT_TIMEOUT = 120} ;


/* Creates a connection struct. */
AceConnStatus AceConnCreate(AceConnection *connection, char *server_netid, int server_port,
			    char *userid, char *passwd, int timeout) ;


/* Open a connection to the database. */
AceConnStatus AceConnConnect(AceConnection connection) ;

/* Send a request to the database and get the reply. */
AceConnStatus AceConnRequest(AceConnection connection,
			     char *request,
			     void **reply, int *reply_len) ;

/* Close down the connection. */
AceConnStatus AceConnDisconnect(AceConnection connection) ;


/* Free the connection struct. */
void AceConnDestroy(AceConnection connection) ;



/* Utility functions.                                                        */
/*                                                                           */

void AceConnGetVersion(guint *version, guint *release, guint *update) ;

AceConnStatus AceConnSetTimeout(AceConnection connection, int timeout) ;

AceConnStatus AceConnConnectionOpen(AceConnection connection) ;


char *AceConnGetErrorMsg(AceConnection connection, AceConnStatus err_status) ;


/* Deprecated, use AceConnGetErrorMsg() instead. */
char *AceConnGetLastErrMsg(AceConnection connection) ;


#endif /* !defined DEF_ACECONN_H */
