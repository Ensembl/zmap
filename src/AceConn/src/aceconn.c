/*  File: aceconn.c
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
 * Description: A C package for accessing the acedb socket server.
 *              
 *              The intention is that this package will be standalone
 *              and not dependent on the rest of acedb so anyone can
 *              take it and use it.
 *              
 *              The code makes use of glib which is an GNU licensed
 *              package of utility functions.
 *              
 *              This file contains just the interface functions for the
 *              package.
 *              
 * Exported functions: See AceConn.h
 * HISTORY:
 * Last edited: Dec 13 11:49 2004 (edgrif)
 * Created: Wed Mar  6 13:55:37 2002 (edgrif)
 * CVS info:   $Id: aceconn.c,v 1.1.1.1 2007-10-03 13:28:00 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <aceconn_.h>


static AceConnStatus doServerClose(AceConnection connection) ;



/**@name Interface Functions
 * The AceConn package has a simple set of interface functions that allow
 * you to open a connection to an acedb server, send requests and receive
 * answers. */

/*@{ Start of interface functions. */


/** Create the an unconnected connection control block.
 *
 * @return ACECONN_OK if connect to server successful, otherwise
 * status indicates what failure was.
 *
 * @param connection_out returned valid connection (opaque type),
 *        untouched unless create succeeds.
 * @param server_netid host network id of server
 * @param server_port servers port on host
 * @param userid user known to acedb server
 * @param passwd users password (will be encrypted before sending).
 * @param timeout time to wait for reply from server before returning
 *        control to application
 */
AceConnStatus AceConnCreate(AceConnection *connection_out,
			    char *server_netid, int server_port,
			    char *userid, char *passwd, int timeout)
{
  AceConnStatus status = ACECONN_OK ;
  AceConnection connection = NULL ;

  /* Could check port number for  server_port < 1024 || server_port > 65535,
   * but probably over kill. */
  if (!connection_out || server_netid == NULL || server_port <= 0
      || userid == NULL || passwd == NULL)
    status = ACECONN_BADARGS ;
  else
    {
      if (!(connection = aceconnCreate(server_netid, server_port, userid, passwd, timeout)))
	status = ACECONN_NOCREATE ;
      else
	*connection_out = connection ;
    }

  return status ;
}

/** Open a connection to the server. Does all the hand shaking necessary to
 * say "hello" to the server, after this acedb requests can be sent to the
 * server.
 *
 * @return ACECONN_OK if connect to server successful, otherwise
 * status indicates what failure was.
 *
 * @param connection_inout server connection
 */
AceConnStatus AceConnConnect(AceConnection connection_inout)
{
  AceConnStatus status = ACECONN_OK ;

  if (connection_inout == NULL) 
    {
      status = ACECONN_INVALIDCONN ;
    }

  if (status == ACECONN_OK)
    {
      status = aceconnServerOpen(connection_inout) ;
    }

  if (status == ACECONN_OK)
    {
      status = aceconnServerHandshake(connection_inout) ;
    }

  /* If there was a problem, then clear up. */
  if (status != ACECONN_OK)
    {
      doServerClose(connection_inout) ;
    }

  return status ;
}



/** Sends an acedb request to the server and then waits for and returns
 * the reply from the server, note that the reply can be NULL if the user
 * has set "quiet" mode on and the request is a command with no response.
 *
 * @return ACECONN_OK if request sent and reply retrieved successfully, otherwise
 * status indicates what failure was.
 *
 * @param connection_out server connection
 * @param request, a C string containing the acedb request, e.g. "find sequence *"
 * @param reply_out, the reply received from the server, <B>note</B> this may be either
 * a C string or binary data as when a postscript image is returned or NULL.
 * @param reply_len_out the length of the data returned, <B>note</B> that for a C string,
 * this <b>includes</b> the terminating null char of the string, if reply_out is NULL
 * then this will be zero.
 */
AceConnStatus AceConnRequest(AceConnection connection,
			     char *request,
			     void **reply_out, int *reply_len_out)
{
  AceConnStatus status = ACECONN_OK ;

  if (!aceconnIsValid(connection))
    {
      status = ACECONN_INVALIDCONN ;
    }

  if (status == ACECONN_OK)
    {
      if (request == NULL || reply_out == NULL || reply_len_out == NULL)
	{
	  status = ACECONN_BADARGS ;
	  aceconnSetErrMsg(connection, ACECONN_BADARGS, "%s", "Bad args to function AceConnRequest()") ;
	}
    }

  if (status == ACECONN_OK)
    {
      status = aceconnServerRequest(connection, request, reply_out, reply_len_out) ;

      /* For a normal request we just send it, but if the request was "quit" */
      /* then we close the connection to the server as well, note that if    */
      /* the close was successful we return a status of "quit" so the caller */
      /* can detect this situation.                                          */
      if (status == ACECONN_OK && (strcmp(request, ACECONN_CLIENT_GOODBYE) == 0))
	{
	  connection->state = ACECONNSTATE_OPEN ;
	  status = aceconnServerClose(connection) ;
	  if (status == ACECONN_OK)
	    {
	      status = ACECONN_QUIT ;
	      aceconnSetErrMsg(connection, ACECONN_QUIT, "%s",
			       "Connection to server closed by \"quit\" request.") ;
	    }
	}
    }

  return status ;
}


/** Closes the connection to an acedb server. If close is
 * successful then connection is freed, otherwise it is not
 * to allow application to get the error message.
 *
 * @return ACECONN_OK if disconnected successfully, otherwise
 * status indicates what failure was.
 *
 * @param connection server connection (opaque type)
 */
AceConnStatus AceConnDisconnect(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;

  if (!aceconnIsValid(connection))
    status = ACECONN_INVALIDCONN ;

  if (status == ACECONN_OK)
    {
      status = doServerClose(connection) ;
    }

  return status ;
}


/** Simple free of a connection struct.
 *
 * @return nothing
 *
 * @param connection server connection (opaque type)
 */
void AceConnDestroy(AceConnection connection)
{
  aceconnDestroy(connection) ;

  return ;
}

/*@} End of interface functions. */


/**@name Utility Functions
 * Following are all basically utility functions that supplement the Interface
 * functions. */

/*@{ Start of utility functions. */


/** Returns the current version of the AceConn library.
 *
 * @return nothing.
 *
 * @param version, release, update, the version, release and update
 * are returned in these parameters.
 */
void AceConnGetVersion(guint *version, guint *release, guint *update)
{
  if (version)
    *version = ACECONN_VERSION ;
  if (release)
    *release = ACECONN_RELEASE ;
  if (update)
    *update = ACECONN_UPDATE ;

  return ;
}


/** Sets the timeout for all communication with the server. If the server has
 * not responded after "timeout" seconds AceConn returns control to the
 * application, setting a status of ACECONN_TIMEDOUT.
 *
 * @return ACECONN_OK if timeout set correctly, or ACECONN_INVALIDCONN
 * if connection is invalid.
 *
 * @param connection_out server connection (opaque type)
 * @param timeout the new timeout in seconds.
 */
AceConnStatus AceConnSetTimeout(AceConnection connection, int timeout)
{
  AceConnStatus status = ACECONN_OK ;

  if (!aceconnIsValid(connection))
    status = ACECONN_INVALIDCONN ;

  if (status == ACECONN_OK)
    {
      connection->timeout = timeout ;
      status = ACECONN_OK ;
    }

  return status ;
}


/** Checks whether connection to server is open.
 *
 * @return ACECONN_OK if connection is open or ready, ACECONN_INVALIDCONN
 * if connection is invalid otherwise ACECONN_NOTOPEN.
 *
 * @param connection_out server connection (opaque type)
 */
AceConnStatus AceConnConnectionOpen(AceConnection connection)
{
  AceConnStatus status = ACECONN_NOTOPEN ;

  if (!aceconnIsValid(connection))
    status = ACECONN_INVALIDCONN ;
  else if (connection->state != ACECONNSTATE_NONE)
    status = ACECONN_OK ;

  return status ;
}


/** If the connection is valid and contains a last error status
 * that matches err_status then a string is returned containing
 * a more detailed description of the last error detected by the
 * AceConn library. Otherwise a simple text string version of
 * err_status is returned.
 * This call can be used for all errors returned by the AceConn
 * library, e.g. the call works if connection is NULL, invalid
 * or valid.
 * Can be used to obtain a simple text string version of any
 * error status by calling with  connection == NULL.
 * The returned string is internal to the AceConn package and
 * the caller should not alter or free it.
 *
 * @return error string, or NULL if err_status == ACECONN_OK
 *
 * @param connection server connection (opaque type)
 * @param err_status last error status returned by AceConn package
 */
char *AceConnGetErrorMsg(AceConnection connection, AceConnStatus err_status)
{
  char *err_msg = NULL ;

  if (aceconnIsValid(connection) && connection->last_err_status == err_status)
    err_msg = connection->last_errmsg ;
  else
    err_msg = aceconnGetSimpleErrMsg(err_status) ;

  return err_msg ;
}

/** DEPRECATED FUNCTION, use AceConnGetErrorMsg() instead.
 *
 * Returns a string containing a description of the last error
 * encountered by the library, i.e. call will only return a string if
 * an interface call has returned something other than ACECONN_OK and
 * connection is still valid. The returned string is internal to the
 * AceConn package and the caller should not alter or free it.
 *
 * @return error string, or NULL if there has been no error or
 * the connection is invalid.
 *
 * @param connection_out server connection (opaque type)
 */
char *AceConnGetLastErrMsg(AceConnection connection)
{
  char *err_msg = NULL ;

  if (aceconnIsValid(connection))
    err_msg = connection->last_errmsg ;

  return err_msg ;
}

/*@} End of utility functions. */



/*                                                                           */
/* Internal functions.                                                       */
/*                                                                           */

static AceConnStatus doServerClose(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;

  if (status == ACECONN_OK && connection->state != ACECONNSTATE_NONE)
    {
      status = aceconnServerClose(connection) ;
    }

  return status ;
}

