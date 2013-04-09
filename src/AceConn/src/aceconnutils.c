/*  File: aceconnutils.c
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
 * Description: Internal utility functions for AceConn package.
 *              
 * Exported functions: None
 * HISTORY:
 * Last edited: Mar 17 18:13 2004 (edgrif)
 * Created: Thu Mar  7 09:34:46 2002 (edgrif)
 * CVS info:   $Id: aceconnutils.c,v 1.1.1.1 2007-10-03 13:28:00 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <aceconn_.h>


/* An array of these is used to hold the simple string versions of error messages,
 * see aceconnGetSimpleErrMsg() */
typedef struct
{
  AceConnStatus status ;
  char *message ;
} aceconnSimpleErrMsgStruct, *aceconnSimpleErrMsg ;


/* This string is used as a unique symbol for checking that a connection     */
/* struct is valid. There can only be one of these pointers with this name   */
/* in this namespace so its address is unique and its this address that we   */
/* use for checking. Its most unlikely that any bit of random memory will    */
/* have this bit pattern. Using a string like this also helps in debugging   */
/* as its possible to see the string.                                        */
/* Note that the struct is initialised with this address and then when freed */
/* this part of the struct is cleared to zeros, this is vital because it is  */
/* possible for the memory that a dangling pointer addresses to remain       */
/* unaltered for some time and hence look like a valid pointer.              */

#define ACECONN_CONTROL_BLOCK_STRING "connection_control_block"

static char* CCB_ptr = ACECONN_CONTROL_BLOCK_STRING ;



/* Create a basic control block for an acedb server connection.              */
/*                                                                           */
AceConnection aceconnCreate(char *server_netid, int server_port,
			    char *userid, char *passwd, int timeout)
{
  AceConnection connection = NULL ;

  connection = (AceConnection)g_malloc(sizeof(AceConnRec)) ;
  if (connection)
    {
      connection->valid = &CCB_ptr ;
      connection->state = ACECONNSTATE_NONE ;
      connection->host = server_netid ;
      connection->port = server_port ;
      connection->socket = -1 ;
      if (timeout < 0)
	connection->timeout = ACECONN_DEFAULT_TIMEOUT ;
      else
	connection->timeout = timeout ;
      connection->userid = userid ;
      connection->passwd = passwd ;
      connection->passwd_hash = NULL ;
      connection->nonce_hash = NULL ;
      connection->msg = NULL ;
      connection->last_err_status = ACECONN_OK ;
      connection->last_errmsg = NULL ;
    }

  return connection ;
}


/* Check a control block for validity, we only do this for external calls.   */
/* In some senses checking the string is overkill because we declared it     */
/* "const", but this does not guarantee that it can't be altered which might */
/* indicate big trouble in the program.                                      */
gboolean aceconnIsValid(AceConnection connection)
{
  gboolean valid = FALSE ;

  if (connection
      && connection->valid == &CCB_ptr
      && strcmp(*(connection->valid), ACECONN_CONTROL_BLOCK_STRING) == 0)
    valid = TRUE ;

  return valid ;
}


void aceconnDestroy(AceConnection connection)
{

  if (connection)
    {
      if (connection->passwd_hash)
	g_free(connection->passwd_hash) ;

      aceconnFreeErrMsg(connection) ;

      connection->valid = NULL ;			    /* Vital for valid block checking. */

      g_free(connection) ;
    }

  return ;
}



/* Returns the simple string version of any error status code, returns
 * NULL for ACECONN_OK or any value of status that is non known. */
char *aceconnGetSimpleErrMsg(AceConnStatus status)
{
  char *err_msg = NULL ;
  aceconnSimpleErrMsgStruct err_messages[] =
  {
    {ACECONN_QUIT, "Command to server was \"quit\" so connection is now closed."},
    {ACECONN_INVALIDCONN,    "Connection points to invalid memory."},
    {ACECONN_BADARGS,    "caller has supplied bad args."},
    {ACECONN_NOTOPEN,    "Connection not open."},
    {ACECONN_NOALLOC,    "could not allocate "},
    {ACECONN_NOSOCK,    "socket creation problem."},
    {ACECONN_UNKNOWNHOST,    "server host not known."},
    {ACECONN_NOCONNECT,    "Could not connect to host."},
    {ACECONN_NOSELECT,    "select on socket failed."},
    {ACECONN_HANDSHAKE,    "Handshake to server failed."},
    {ACECONN_READERROR,    "Error in reading from socket."},
    {ACECONN_WRITEERROR,    "Error in writing to socket."},
    {ACECONN_SIGSET,    "Problem with signal setting."},
    {ACECONN_NONBLOCK,    "Non-blocking for socket failed."},
    {ACECONN_TIMEDOUT,    "Connection timed out."},
    {ACECONN_NOCREATE,    "Could not create connection control block."},
    {ACECONN_INTERNAL,    "Dire, internal package error."},
    {ACECONN_OK, NULL}
  } ;
  aceconnSimpleErrMsg curr_err ;

  curr_err = &err_messages[0] ;
  while (curr_err->message != NULL)
    {
      if (curr_err->status == status)
	{
	  err_msg = curr_err->message ;
	  break ;
	}
      else
	curr_err++ ;
    }

  return err_msg ;
}


/* N.B. to do simple strings do this:  aceconnSetErrMsg("%s", string) ;      */
/*                                                                           */
/* Assumed never to fail....                                                 */
void aceconnSetErrMsg(AceConnection connection, AceConnStatus status, char *format, ...)
{
  va_list args ;
  char *err_msg ;

  connection->last_err_status = status ;

  if (connection->last_errmsg != NULL)
    g_free(connection->last_errmsg) ;

  /* Format the supplied error message. */
  va_start(args, format) ;

  err_msg = g_strdup_vprintf(format, args) ;
  
  va_end(args) ;

  /* Prefix with server/port info. */
  connection->last_errmsg = g_strdup_printf("\"%s, port %d\": %s", connection->host, connection->port,
					    err_msg) ;

  /* Get rid of this, no longer needed. */
  g_free(err_msg) ;

  return ;
}

/* Assumed never to fail.                                                    */
void aceconnFreeErrMsg(AceConnection connection)
{
  if (connection->last_errmsg)
    g_free(connection->last_errmsg) ;

  return ;
}




/************************************************************/
/************************************************************/
